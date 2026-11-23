#include "UI.h"

#include "Outfits.h"
#include "Wardrobe.h"

#include "SKSEMenuFramework.h"

namespace UI
{
    namespace
    {
        namespace ImGui = ImGuiMCP;

        // Showing tens of thousands of rows at once would slow the menu down, so the
        // list stops here and asks the player to narrow the search.
        constexpr std::size_t kMaxRows = 400;

        struct SlotFilter
        {
            const char*   label;
            std::uint32_t mask;  // 0 = any slot
        };

        // Slot numbers follow the Creation Kit (bit 0 = slot 30).
        constexpr SlotFilter kSlotFilters[] = {
            { "Any slot", 0 },
            { "Head / hair (30, 31, 42)", (1u << 0) | (1u << 1) | (1u << 12) },
            { "Body (32)", 1u << 2 },
            { "Hands (33)", 1u << 3 },
            { "Feet (37)", 1u << 7 },
            { "Jewelry (35, 36)", (1u << 5) | (1u << 6) },
            { "Shields (39)", 1u << 9 },
            { "Modded slots (44-61)", 0xFFFC0000u & ~(1u << 31) },
        };

        struct WardrobeState
        {
            char        search[128]{};
            int         pluginIndex{ -1 };  // -1 = all mods
            int         slotIndex{ 0 };
            bool        showNonPlayable{ false };
            char        outfitName[64]{};

            // Cached filter result; rebuilt only when a filter changes.
            bool                     dirty{ true };
            std::vector<std::size_t> rows;
        } g_wardrobe;

        std::string ToLower(std::string_view a_text)
        {
            std::string out(a_text);
            std::ranges::transform(out, out.begin(), [](unsigned char c) { return static_cast<char>(std::tolower(c)); });
            return out;
        }

        void RebuildRows()
        {
            const auto& catalog = Wardrobe::Catalog();
            const auto& plugins = Wardrobe::Plugins();
            const auto  search = ToLower(g_wardrobe.search);
            const auto* plugin = g_wardrobe.pluginIndex >= 0 ? &plugins[g_wardrobe.pluginIndex] : nullptr;
            const auto  slotMask = kSlotFilters[g_wardrobe.slotIndex].mask;

            g_wardrobe.rows.clear();
            for (std::size_t i = 0; i < catalog.size(); ++i) {
                const auto& item = catalog[i];
                if (!g_wardrobe.showNonPlayable && !item.playable) {
                    continue;
                }
                if (plugin && item.plugin != *plugin) {
                    continue;
                }
                if (slotMask && (item.slotMask & slotMask) == 0) {
                    continue;
                }
                if (!search.empty() && item.searchKey.find(search) == std::string::npos) {
                    continue;
                }
                g_wardrobe.rows.push_back(i);
            }
            g_wardrobe.dirty = false;
        }

        void DrawStatus()
        {
            auto status = Wardrobe::LastStatus();
            if (!status.empty()) {
                ImGui::Separator();
                ImGui::TextWrapped("%s", status.c_str());
            }
        }

        void DrawFilters()
        {
            const auto& plugins = Wardrobe::Plugins();

            ImGui::SetNextItemWidth(260.0f);
            if (ImGui::InputTextWithHint("##search", "Search name or mod...", g_wardrobe.search, sizeof(g_wardrobe.search))) {
                g_wardrobe.dirty = true;
            }

            ImGui::SameLine();
            ImGui::SetNextItemWidth(240.0f);
            const char* pluginPreview = g_wardrobe.pluginIndex >= 0 ? plugins[g_wardrobe.pluginIndex].c_str() : "All mods";
            if (ImGui::BeginCombo("##plugin", pluginPreview)) {
                if (ImGui::Selectable("All mods", g_wardrobe.pluginIndex < 0)) {
                    g_wardrobe.pluginIndex = -1;
                    g_wardrobe.dirty = true;
                }
                for (int i = 0; i < static_cast<int>(plugins.size()); ++i) {
                    if (ImGui::Selectable(plugins[i].c_str(), g_wardrobe.pluginIndex == i)) {
                        g_wardrobe.pluginIndex = i;
                        g_wardrobe.dirty = true;
                    }
                }
                ImGui::EndCombo();
            }

            ImGui::SameLine();
            ImGui::SetNextItemWidth(200.0f);
            if (ImGui::BeginCombo("##slot", kSlotFilters[g_wardrobe.slotIndex].label)) {
                for (int i = 0; i < static_cast<int>(std::size(kSlotFilters)); ++i) {
                    if (ImGui::Selectable(kSlotFilters[i].label, g_wardrobe.slotIndex == i)) {
                        g_wardrobe.slotIndex = i;
                        g_wardrobe.dirty = true;
                    }
                }
                ImGui::EndCombo();
            }

            ImGui::SameLine();
            if (ImGui::Checkbox("Show non-playable", &g_wardrobe.showNonPlayable)) {
                g_wardrobe.dirty = true;
            }
        }

        void DrawCatalogTable()
        {
            const auto& catalog = Wardrobe::Catalog();
            const auto  shown = std::min(g_wardrobe.rows.size(), kMaxRows);

            ImGui::Text("%zu items match", g_wardrobe.rows.size());
            if (g_wardrobe.rows.size() > kMaxRows) {
                ImGui::SameLine();
                ImGui::TextDisabled("(showing the first %zu, narrow the search to see more)", kMaxRows);
            }

            const auto flags = ImGui::ImGuiTableFlags_RowBg | ImGui::ImGuiTableFlags_BordersOuter | ImGui::ImGuiTableFlags_BordersV |
                               ImGui::ImGuiTableFlags_ScrollY | ImGui::ImGuiTableFlags_Resizable;
            if (!ImGui::BeginTable("##catalog", 4, flags, ImGui::ImVec2{ 0.0f, 360.0f })) {
                return;
            }

            ImGui::TableSetupColumn("Name");
            ImGui::TableSetupColumn("Mod");
            ImGui::TableSetupColumn("Slots", ImGui::ImGuiTableColumnFlags_WidthFixed, 90.0f);
            ImGui::TableSetupColumn("##actions", ImGui::ImGuiTableColumnFlags_WidthFixed, 130.0f);
            ImGui::TableHeadersRow();

            for (std::size_t r = 0; r < shown; ++r) {
                const auto& item = catalog[g_wardrobe.rows[r]];
                ImGui::PushID(static_cast<int>(item.formID));
                ImGui::TableNextRow();

                ImGui::TableNextColumn();
                ImGui::TextUnformatted(item.name.c_str());
                ImGui::TableNextColumn();
                ImGui::TextUnformatted(item.plugin.c_str());
                ImGui::TableNextColumn();
                ImGui::TextUnformatted(item.slots.c_str());
                ImGui::TableNextColumn();
                if (ImGui::Button("Try on")) {
                    Wardrobe::TryOn(item.formID);
                }
                ImGui::SameLine();
                if (ImGui::Button("Off")) {
                    Wardrobe::TakeOff(item.formID);
                }

                ImGui::PopID();
            }
            ImGui::EndTable();
        }

        void __stdcall RenderWardrobe()
        {
            if (!Wardrobe::IsReady()) {
                ImGui::TextUnformatted("The wardrobe is still loading. Load a save, then open this menu again.");
                return;
            }

            DrawFilters();
            if (g_wardrobe.dirty) {
                RebuildRows();
            }
            DrawCatalogTable();

            if (ImGui::Button("Undress")) {
                Wardrobe::UndressAll();
            }
            ImGui::SameLine();
            if (ImGui::Button("Return borrowed items")) {
                Wardrobe::ReturnBorrowedItems();
            }
            if (ImGui::IsItemHovered()) {
                ImGui::SetTooltip("Removes items that the Wardrobe added to your inventory this session so you could try them on.");
            }

            ImGui::SeparatorText("Save what I'm wearing");
            ImGui::SetNextItemWidth(260.0f);
            ImGui::InputTextWithHint("##outfitName", "Outfit name", g_wardrobe.outfitName, sizeof(g_wardrobe.outfitName));
            ImGui::SameLine();
            ImGui::BeginDisabled(g_wardrobe.outfitName[0] == '\0');
            if (ImGui::Button("Save outfit")) {
                Outfits::SaveCurrent(g_wardrobe.outfitName);
                g_wardrobe.outfitName[0] = '\0';
            }
            ImGui::EndDisabled();

            DrawStatus();
        }

        void __stdcall RenderOutfits()
        {
            if (ImGui::Button("Reload from disk")) {
                Outfits::LoadAll();
            }
            ImGui::SameLine();
            ImGui::TextDisabled("%s", Outfits::Folder().string().c_str());

            const auto outfits = Outfits::List();
            if (outfits.empty()) {
                ImGui::TextWrapped("No saved outfits yet. Dress up in the Wardrobe tab, then use \"Save outfit\".");
                DrawStatus();
                return;
            }

            const auto flags = ImGui::ImGuiTableFlags_RowBg | ImGui::ImGuiTableFlags_BordersOuter | ImGui::ImGuiTableFlags_BordersV;
            if (ImGui::BeginTable("##outfits", 3, flags)) {
                ImGui::TableSetupColumn("Outfit");
                ImGui::TableSetupColumn("Items", ImGui::ImGuiTableColumnFlags_WidthFixed, 60.0f);
                ImGui::TableSetupColumn("##actions", ImGui::ImGuiTableColumnFlags_WidthFixed, 130.0f);
                ImGui::TableHeadersRow();

                for (const auto& outfit : outfits) {
                    ImGui::PushID(outfit.name.c_str());
                    ImGui::TableNextRow();

                    ImGui::TableNextColumn();
                    ImGui::TextUnformatted(outfit.name.c_str());
                    if (ImGui::IsItemHovered()) {
                        std::string tooltip;
                        for (const auto& item : outfit.items) {
                            tooltip += std::format("{} ({})\n", item.name, item.plugin);
                        }
                        ImGui::SetTooltip("%s", tooltip.c_str());
                    }
                    ImGui::TableNextColumn();
                    ImGui::Text("%zu", outfit.items.size());
                    ImGui::TableNextColumn();
                    if (ImGui::Button("Wear")) {
                        Outfits::Wear(outfit);
                    }
                    ImGui::SameLine();
                    if (ImGui::Button("Delete")) {
                        Outfits::Delete(outfit.name);
                    }

                    ImGui::PopID();
                }
                ImGui::EndTable();
            }

            DrawStatus();
        }
    }

    void Register()
    {
        if (!SKSEMenuFramework::IsInstalled()) {
            logger::warn("SKSE Menu Framework is not installed; Character Studio has no menu");
            return;
        }

        SKSEMenuFramework::SetSection(MOD_NAME);
        SKSEMenuFramework::AddSectionItem("Wardrobe", RenderWardrobe);
        SKSEMenuFramework::AddSectionItem("Saved Outfits", RenderOutfits);
        logger::info("Registered Character Studio menu");
    }
}
