#include "Wardrobe.h"

namespace Wardrobe
{
    namespace
    {
        std::vector<ArmorEntry>  g_catalog;
        std::vector<std::string> g_plugins;
        std::atomic<bool>        g_ready{ false };

        std::mutex  g_statusLock;
        std::string g_status;

        // Items the Wardrobe added to the player's inventory so they could be tried on.
        // Only touched on the game thread.
        std::unordered_set<RE::FormID> g_borrowed;

        std::string ToLower(std::string_view a_text)
        {
            std::string out(a_text);
            std::ranges::transform(out, out.begin(), [](unsigned char c) { return static_cast<char>(std::tolower(c)); });
            return out;
        }

        RE::PlayerCharacter* Player()
        {
            return RE::PlayerCharacter::GetSingleton();
        }

        void QueueOnGameThread(std::function<void()> a_task)
        {
            if (auto tasks = SKSE::GetTaskInterface()) {
                tasks->AddTask(std::move(a_task));
            }
        }

        std::int32_t CountInInventory(RE::PlayerCharacter* a_player, RE::TESBoundObject* a_object)
        {
            auto counts = a_player->GetInventoryCounts([a_object](RE::TESBoundObject& a_item) { return &a_item == a_object; });
            auto it = counts.find(a_object);
            return it != counts.end() ? it->second : 0;
        }
    }

    std::string DescribeSlots(std::uint32_t a_mask)
    {
        std::string out;
        for (std::uint32_t bit = 0; bit < 32; ++bit) {
            if (a_mask & (1u << bit)) {
                if (!out.empty()) {
                    out += ", ";
                }
                out += std::to_string(30 + bit);
            }
        }
        return out.empty() ? "-" : out;
    }

    void BuildCatalog()
    {
        auto dataHandler = RE::TESDataHandler::GetSingleton();
        if (!dataHandler) {
            logger::error("TESDataHandler not available; Wardrobe catalog is empty");
            return;
        }

        std::vector<ArmorEntry>         catalog;
        std::unordered_set<std::string> plugins;

        for (auto armor : dataHandler->GetFormArray<RE::TESObjectARMO>()) {
            if (!armor) {
                continue;
            }

            // Skip things that are not really wearable: unnamed forms are usually
            // race skins or helper items added by other mods.
            const char* name = armor->GetName();
            if (!name || name[0] == '\0') {
                continue;
            }

            ArmorEntry entry;
            entry.formID = armor->GetFormID();
            entry.name = name;
            if (auto file = armor->GetFile(0)) {
                entry.plugin = std::string(file->GetFilename());
            } else {
                entry.plugin = "(created in game)";
            }
            entry.slotMask = armor->GetSlotMask().underlying();
            entry.slots = DescribeSlots(entry.slotMask);
            entry.playable = (armor->GetFormFlags() & RE::TESObjectARMO::RecordFlags::kNonPlayable) == 0;
            entry.searchKey = ToLower(entry.name + " " + entry.plugin);

            plugins.insert(entry.plugin);
            catalog.push_back(std::move(entry));
        }

        std::ranges::sort(catalog, [](const ArmorEntry& a, const ArmorEntry& b) { return a.name < b.name; });

        g_plugins.assign(plugins.begin(), plugins.end());
        std::ranges::sort(g_plugins);
        g_catalog = std::move(catalog);
        g_ready = true;

        logger::info("Wardrobe catalog built: {} items from {} plugins", g_catalog.size(), g_plugins.size());
    }

    bool IsReady() { return g_ready; }

    const std::vector<ArmorEntry>& Catalog() { return g_catalog; }

    const std::vector<std::string>& Plugins() { return g_plugins; }

    std::string LastStatus()
    {
        std::scoped_lock lock(g_statusLock);
        return g_status;
    }

    void SetStatus(std::string a_status)
    {
        logger::info("{}", a_status);
        std::scoped_lock lock(g_statusLock);
        g_status = std::move(a_status);
    }

    void TryOn(RE::FormID a_formID)
    {
        QueueOnGameThread([a_formID] {
            auto player = Player();
            auto armor = RE::TESForm::LookupByID<RE::TESObjectARMO>(a_formID);
            if (!player || !armor) {
                SetStatus("Could not find that item");
                return;
            }

            if (CountInInventory(player, armor) <= 0) {
                player->AddObjectToContainer(armor, nullptr, 1, nullptr);
                g_borrowed.insert(a_formID);
            }

            RE::ActorEquipManager::GetSingleton()->EquipObject(player, armor);
            SetStatus(std::format("Wearing {}", armor->GetName()));
        });
    }

    void TakeOff(RE::FormID a_formID)
    {
        QueueOnGameThread([a_formID] {
            auto player = Player();
            auto armor = RE::TESForm::LookupByID<RE::TESObjectARMO>(a_formID);
            if (!player || !armor) {
                return;
            }
            RE::ActorEquipManager::GetSingleton()->UnequipObject(player, armor);
            SetStatus(std::format("Took off {}", armor->GetName()));
        });
    }

    void GetWornArmor(std::function<void(std::vector<RE::TESObjectARMO*>)> a_callback)
    {
        QueueOnGameThread([callback = std::move(a_callback)] {
            std::vector<RE::TESObjectARMO*> worn;
            if (auto player = Player()) {
                auto inventory = player->GetInventory([](RE::TESBoundObject& a_item) { return a_item.IsArmor(); });
                for (auto& [object, data] : inventory) {
                    auto& entry = data.second;
                    if (entry && entry->IsWorn()) {
                        if (auto armor = object->As<RE::TESObjectARMO>()) {
                            worn.push_back(armor);
                        }
                    }
                }
            }
            callback(std::move(worn));
        });
    }

    void UndressAll()
    {
        GetWornArmor([](std::vector<RE::TESObjectARMO*> a_worn) {
            auto player = Player();
            auto equipManager = RE::ActorEquipManager::GetSingleton();
            for (auto armor : a_worn) {
                equipManager->UnequipObject(player, armor);
            }
            SetStatus(std::format("Took off {} items", a_worn.size()));
        });
    }

    void ReturnBorrowedItems()
    {
        QueueOnGameThread([] {
            auto player = Player();
            if (!player) {
                return;
            }
            std::size_t returned = 0;
            for (auto formID : g_borrowed) {
                if (auto armor = RE::TESForm::LookupByID<RE::TESObjectARMO>(formID)) {
                    if (CountInInventory(player, armor) > 0) {
                        player->RemoveItem(armor, 1, RE::ITEM_REMOVE_REASON::kRemove, nullptr, nullptr);
                        ++returned;
                    }
                }
            }
            g_borrowed.clear();
            SetStatus(std::format("Returned {} borrowed items", returned));
        });
    }
}
