#pragma once

// The Wardrobe knows every piece of armor and clothing in the load order and can
// put it on (or take it off) the player. Anything that touches the game is queued
// onto the game thread with SKSE's task interface, because the menu renders on a
// different thread.

namespace Wardrobe
{
    struct ArmorEntry
    {
        RE::FormID    formID{ 0 };
        std::string   name;
        std::string   plugin;      // file the item comes from, e.g. "Skyrim.esm"
        std::string   slots;       // human-readable biped slots, e.g. "32, 49"
        std::uint32_t slotMask{ 0 };
        bool          playable{ true };
        std::string   searchKey;   // lowercase name + plugin, for fast filtering
    };

    // Builds the catalog. Call once after kDataLoaded.
    void BuildCatalog();

    bool                           IsReady();
    const std::vector<ArmorEntry>& Catalog();
    const std::vector<std::string>& Plugins();

    // Game actions (queued onto the game thread).
    void TryOn(RE::FormID a_formID);
    void TakeOff(RE::FormID a_formID);
    void UndressAll();
    void ReturnBorrowedItems();

    // Reads the armor the player is wearing, then calls a_callback on the game thread.
    void GetWornArmor(std::function<void(std::vector<RE::TESObjectARMO*>)> a_callback);

    // Last status line to show in the menu ("Equipped Iron Armor", ...). Thread-safe.
    std::string LastStatus();
    void        SetStatus(std::string a_status);

    // Turns a slot mask into "32, 49" (Skyrim's biped slot numbers start at 30).
    std::string DescribeSlots(std::uint32_t a_mask);
}
