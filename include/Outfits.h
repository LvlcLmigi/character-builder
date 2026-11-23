#pragma once

// Saved outfits live as small JSON files in Data/SKSE/Plugins/CharacterStudio/Outfits.
// Under MO2 new files land in the "overwrite" folder; move them into a mod to keep them.
// Items are stored as (plugin name, local form ID), so outfits survive load order changes.

namespace Outfits
{
    struct Item
    {
        std::string   plugin;
        RE::FormID    localFormID{ 0 };
        std::string   name;  // for display only, so missing items can still be named
    };

    struct Outfit
    {
        std::string       name;
        std::vector<Item> items;
    };

    std::filesystem::path Folder();

    // Reloads every outfit file from disk.
    void LoadAll();

    // Copy of the current list, safe to iterate from the menu.
    std::vector<Outfit> List();

    // Saves what the player is wearing now under a_name (overwrites if it exists).
    void SaveCurrent(std::string a_name);

    // Undresses the player and puts on every item of the outfit that still exists.
    void Wear(const Outfit& a_outfit);

    void Delete(const std::string& a_name);
}
