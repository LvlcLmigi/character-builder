# Character Studio

An in-game character studio for Skyrim Special Edition / Anniversary Edition. It works on top of the mods you already have (skin, hair, body, armor, poses) instead of replacing them:

1. **Build** your character with RaceMenu, as usual.
2. **Dress** them from a searchable wardrobe of every armor and clothing item in your load order.
3. **Save** looks as outfits (and later, full character cards).
4. **Pose and animate** them with pose packs and animations you have installed. (This is on the roadmap.)

Character Studio is an SKSE plugin with its menu inside [SKSE Menu Framework](https://github.com/QTR-Modding/SKSE-Menu-Framework-3) (install it from Nexus Mods). It never includes or redistributes any other mod's files. It only references items that are already in your load order.

## Status

**Milestone 1: Wardrobe** (this version)

- **Wardrobe** tab: search every armor and clothing item by name or mod, filter by mod or body slot, then **Try on** or take items **Off**. If you don't own an item, it is borrowed (added to your inventory), and **Return borrowed items** removes those items again.
- **Save outfit**: saves what you're wearing now as a named outfit.
- **Saved Outfits** tab: **Wear** a saved outfit with one click, or **Delete** it. If an outfit uses items from a mod you've since removed, those items are listed as missing.

## Roadmap

| Milestone | What it adds |
|---|---|
| 2. Character cards | One file per character: a RaceMenu preset, a default outfit and favorite poses. Load it all at once. |
| 3. Pose browser | Lists the poses and idles from installed pose packs, with play, freeze and free camera for screenshots. |
| 4. Models | Applies outfits and poses to followers and NPCs, not only the player. |
| 5. Scenes | Several posed characters together, plus animation playlists. |

## Requirements (to play)

- Skyrim SE 1.5.97 or AE (any 1.6.x version, including the latest)
- [SKSE64](https://skse.silverlock.org/)
- [Address Library for SKSE Plugins](https://www.nexusmods.com/skyrimspecialedition/mods/32444)
- [SKSE Menu Framework](https://github.com/QTR-Modding/SKSE-Menu-Framework-3) (install it from Nexus Mods). Its hotkey opens the menu, and Character Studio shows up as its own section.

## Building (Windows)

### One-time setup

1. Install **Visual Studio 2022** (Community is fine) with the **Desktop development with C++** workload. It includes CMake and Ninja.
2. Install **vcpkg** and set the `VCPKG_ROOT` environment variable to its folder:
   ```powershell
   git clone https://github.com/microsoft/vcpkg C:\dev\vcpkg
   C:\dev\vcpkg\bootstrap-vcpkg.bat
   setx VCPKG_ROOT C:\dev\vcpkg
   ```
3. Clone this repo **with submodules** (CommonLibSSE-NG is a submodule):
   ```powershell
   git clone --recursive https://github.com/LvlcLmigi/character-builder
   ```
   If you already cloned it without `--recursive`, run `git submodule update --init --recursive`.
4. **Optional, but recommended with MO2:** create an empty mod in MO2 called `Character Studio`, then point the build at its folder so every build lands there automatically:
   ```powershell
   setx CHARACTER_STUDIO_MO2_MOD "C:\Path\To\MO2\mods\Character Studio"
   ```
   Restart Visual Studio or your terminal after you use `setx`.

### Build

Open the folder in Visual Studio (**File → Open → Folder**), pick the **Release** preset and choose **Build → Build All**. From a *Developer PowerShell for VS 2022*, you can instead run:

```powershell
cmake --preset release
cmake --build --preset release
```

The first build takes a while because vcpkg compiles the dependencies and CommonLibSSE-NG. Later builds are quick.

The output is `build/release/CharacterStudio.dll`. If you set `CHARACTER_STUDIO_MO2_MOD`, it is copied to `<that mod>/SKSE/Plugins/` for you. Otherwise, copy it there yourself.

### Test in game

1. Enable the `Character Studio` mod in MO2 and launch through **SKSE** from MO2.
2. Load a save and open SKSE Menu Framework with its hotkey. Then go to **Character Studio → Wardrobe**.
3. The log is at `Documents\My Games\Skyrim Special Edition\SKSE\CharacterStudio.log`.

Saved outfits are written to `Data/SKSE/Plugins/CharacterStudio/Outfits/*.json`. Under MO2, new files land in the **overwrite** folder. To keep them, move them into a mod (for example, the `Character Studio` mod itself).

## Project layout

```
src/Plugin.cpp     SKSE entry point; hooks up load messages
src/Wardrobe.cpp   armor catalog, try on / take off, borrowed items
src/Outfits.cpp    save / load / wear outfits (JSON)
src/UI.cpp         the menu tabs (ImGui via SKSE Menu Framework)
include/           headers and the precompiled header
vendor/            SKSE Menu Framework API header (MIT)
extern/            CommonLibSSE-NG (git submodule)
```

## Credits

- [CommonLibSSE-NG](https://github.com/alandtse/CommonLibVR/tree/ng), the reverse-engineered Skyrim library (GPL-3.0 with a modding exception)
- [SKSE Menu Framework](https://github.com/QTR-Modding/SKSE-Menu-Framework-3) by Thiago099; the API header is vendored under MIT
