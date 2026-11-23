#include "Outfits.h"

#include "Wardrobe.h"

namespace Outfits
{
    namespace
    {
        std::mutex          g_lock;
        std::vector<Outfit> g_outfits;

        // Keeps file names safe on Windows: letters, digits, space, '-' and '_'.
        std::string SafeFileName(const std::string& a_name)
        {
            std::string out;
            for (unsigned char c : a_name) {
                out += (std::isalnum(c) || c == ' ' || c == '-' || c == '_') ? static_cast<char>(c) : '_';
            }
            return out.empty() ? "Outfit" : out;
        }

        std::filesystem::path PathFor(const std::string& a_name)
        {
            return Folder() / (SafeFileName(a_name) + ".json");
        }

        std::optional<Outfit> ReadFile(const std::filesystem::path& a_path)
        {
            try {
                std::ifstream  file(a_path);
                nlohmann::json json = nlohmann::json::parse(file);

                Outfit outfit;
                outfit.name = json.value("name", a_path.stem().string());
                for (const auto& item : json.value("items", nlohmann::json::array())) {
                    Item entry;
                    entry.plugin = item.at("plugin").get<std::string>();
                    entry.localFormID = static_cast<RE::FormID>(std::stoul(item.at("formID").get<std::string>(), nullptr, 16));
                    entry.name = item.value("name", "");
                    outfit.items.push_back(std::move(entry));
                }
                return outfit;
            } catch (const std::exception& e) {
                logger::warn("Could not read outfit {}: {}", a_path.string(), e.what());
                return std::nullopt;
            }
        }

        bool WriteFile(const Outfit& a_outfit)
        {
            nlohmann::json items = nlohmann::json::array();
            for (const auto& item : a_outfit.items) {
                items.push_back({
                    { "plugin", item.plugin },
                    { "formID", std::format("0x{:08X}", item.localFormID) },
                    { "name", item.name },
                });
            }
            nlohmann::json json = {
                { "version", 1 },
                { "name", a_outfit.name },
                { "items", items },
            };

            try {
                std::filesystem::create_directories(Folder());
                std::ofstream file(PathFor(a_outfit.name));
                file << json.dump(2);
                return file.good();
            } catch (const std::exception& e) {
                logger::error("Could not save outfit {}: {}", a_outfit.name, e.what());
                return false;
            }
        }
    }

    std::filesystem::path Folder()
    {
        return std::filesystem::path("Data") / "SKSE" / "Plugins" / "CharacterStudio" / "Outfits";
    }

    void LoadAll()
    {
        std::vector<Outfit> loaded;
        std::error_code     ec;
        if (std::filesystem::exists(Folder(), ec)) {
            for (const auto& file : std::filesystem::directory_iterator(Folder(), ec)) {
                if (file.path().extension() == ".json") {
                    if (auto outfit = ReadFile(file.path())) {
                        loaded.push_back(std::move(*outfit));
                    }
                }
            }
        }
        std::ranges::sort(loaded, [](const Outfit& a, const Outfit& b) { return a.name < b.name; });

        std::scoped_lock lock(g_lock);
        g_outfits = std::move(loaded);
        logger::info("Loaded {} outfits", g_outfits.size());
    }

    std::vector<Outfit> List()
    {
        std::scoped_lock lock(g_lock);
        return g_outfits;
    }

    void SaveCurrent(std::string a_name)
    {
        Wardrobe::GetWornArmor([name = std::move(a_name)](std::vector<RE::TESObjectARMO*> a_worn) {
            Outfit outfit;
            outfit.name = name;
            for (auto armor : a_worn) {
                auto file = armor->GetFile(0);
                if (!file) {
                    continue;  // created at runtime (e.g. enchanted by the player); cannot be saved by ID
                }
                outfit.items.push_back({ std::string(file->GetFilename()), armor->GetLocalFormID(), armor->GetName() });
            }

            if (outfit.items.empty()) {
                Wardrobe::SetStatus("Nothing to save: you are not wearing any armor or clothing");
                return;
            }

            if (WriteFile(outfit)) {
                LoadAll();
                Wardrobe::SetStatus(std::format("Saved outfit \"{}\" ({} items)", outfit.name, outfit.items.size()));
            } else {
                Wardrobe::SetStatus(std::format("Could not save outfit \"{}\"; see CharacterStudio.log", outfit.name));
            }
        });
    }

    void Wear(const Outfit& a_outfit)
    {
        auto dataHandler = RE::TESDataHandler::GetSingleton();
        if (!dataHandler) {
            return;
        }

        std::vector<std::string> missing;
        std::vector<RE::FormID>  found;
        for (const auto& item : a_outfit.items) {
            if (auto armor = dataHandler->LookupForm<RE::TESObjectARMO>(item.localFormID, item.plugin)) {
                found.push_back(armor->GetFormID());
            } else {
                missing.push_back(std::format("{} ({})", item.name, item.plugin));
            }
        }

        Wardrobe::UndressAll();
        for (auto formID : found) {
            Wardrobe::TryOn(formID);
        }

        std::string status = std::format("Wearing outfit \"{}\"", a_outfit.name);
        if (!missing.empty()) {
            status += std::format(". Missing {} items: ", missing.size());
            for (std::size_t i = 0; i < missing.size(); ++i) {
                status += (i ? ", " : "") + missing[i];
            }
        }
        // Queued after the equips, so this message is the one left on screen.
        SKSE::GetTaskInterface()->AddTask([status] { Wardrobe::SetStatus(status); });
    }

    void Delete(const std::string& a_name)
    {
        std::error_code ec;
        std::filesystem::remove(PathFor(a_name), ec);
        LoadAll();
        Wardrobe::SetStatus(std::format("Deleted outfit \"{}\"", a_name));
    }
}
