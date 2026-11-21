# Install and launch the experimental Switch and PS Vita ports

**Release:** [Portable Console Enhancements — Switch + Vita Preview](https://github.com/sourmelee/ffl_mod/releases/tag/console-preview-2026-11-21)  
**Status:** Both console packages successfully cross-compiled. **Neither has yet been verified on physical hardware.** They may fail to start, crash, or behave incorrectly even with the experimental features disabled. Preserve your existing working binaries and saves.

These are experimental modifications of **[NaGaa95's original Switch port](https://github.com/NaGaa95/ffd_nx)** and **[Rinnegatamante's original Vita port](https://github.com/Rinnegatamante/ffd-vita)**, with input/widescreen work adapted from **[fflmod's original Android mod](https://github.com/fflmod/ffl_mod)**. For complete credits and licenses, see [CREDITS](PORTABLE_CONSOLE_CREDITS.md) and [NOTICES](PORTABLE_CONSOLE_NOTICES.txt).

## Before starting

- You need an already homebrew-enabled **Nintendo Switch** or **PlayStation Vita** suitable for the original homebrew port. These packages do *not* enable homebrew or bypass console protections.
- You must supply your **own legally obtained game files**, extracted from a compatible Android APK and its associated game-data package. The release does **not** include `libjniproxy.so`, `main.obb`, or Square Enix assets. **Do not substitute the Android 1.2.0 ARM64 library from FFLmod for these older console-targeted builds.**
- **Switch:** upstream port targets the original Android **1.1.9 ARM64** library (`lib/arm64-v8a/libjniproxy.so`). The feature hooks in this preview were profiled against the specific user-supplied ELF (SHA-256 `4c6dec4cec9cb58dddcb85a7dd70e289356d55564902949da2a21844b38ef93e`). The ELF's Android application version was not independently verified.
- **Vita:** upstream port targets Android **1.1.3 ARM 32-bit**, from APK path `lib/armeabi/libjniproxy.so` (profile SHA-256 `37a5e9fb3b5038f662f8ed67bca9004a4af9d7b540b98309931066a33d30a79f`).
- Back up saves and any previously working NRO/VPK before replacing an installation. Do not test this build on your only save.

## Nintendo Switch — installation and launch

Based on [NaGaa95's original instructions](https://github.com/NaGaa95/ffd_nx#how-to-install).

1. Download **[ffd_nx_enhanced_experimental.nro](https://github.com/sourmelee/ffl_mod/releases/download/console-preview-2026-11-21/ffd_nx_enhanced_experimental.nro)** from the release. You can rename the copy to `ffd_nx.nro` to match the original installation convention.
2. On the Switch microSD card, create `/switch/ffd/` if it does not exist.
3. Extract **`lib/arm64-v8a/libjniproxy.so`** from your compatible, legally obtained Android APK and place it at `/switch/ffd/libjniproxy.so`.
4. Extract **`assets/main.obb`** to `/switch/ffd/main.obb`. If your legally obtained package provides the OBB separately, place the corresponding `main.obb` there instead.
5. Copy the APK's **`res/raw/` directory** into `/switch/ffd/res/raw/`; the upstream loader expects such files as `moogle.png`, `r_chocobo.png` and `person.anmb2`.
6. Put the experimental NRO into the same directory. If replacing a previous installation, rename the old NRO first so you can restore it.
7. Launch Homebrew Menu with **full application memory**, not Album/applet mode. On common Atmosphère setups, hold **R while launching an installed game** to open Homebrew Menu in application mode, then select the FFD NRO. A configured forwarder may also work. This full-memory requirement is explicit in the upstream Switch README.
8. After boot, use **Menu → Config → Controls → Fix** in the game to make the original on-screen controls work with the loader's physical-button mappings.

Expected SD layout:

```text
/switch/ffd/
├── ffd_nx.nro              # renamed experimental NRO
├── libjniproxy.so          # YOUR Android 1.1.9-targeted ARM64 engine
├── main.obb                # YOUR legally obtained data
├── res/
│   └── raw/
│       ├── moogle.png
│       ├── r_chocobo.png
│       └── person.anmb2
└── enhanced.txt            # optional; start with both values 0
```

Other files inside `res/raw/` should be preserved. The port may create `config.txt` next to the NRO upon first launch. The regular Switch loader supports D-pad/left stick for movement, A to confirm, X/+ for menu, B/− to cancel, and the right stick as a virtual menu cursor.

**Experimental Switch options:** Create `/switch/ffd/enhanced.txt` as a plain UTF-8 text file:

```ini
game_aware_navigation=0
experimental_widescreen=0
```

After proving the unmodified behavior works, set **only** `game_aware_navigation=1` for a separate test. Its experimental mode uses **ZL + D-pad/left stick** to navigate discovered menus and **ZL + A** to activate the selected item. Test that alone before changing `experimental_widescreen` to `1` (with navigation returned to `0`). The original Switch `config.txt` is distinct from `enhanced.txt`; do not put these two experimental keys into the former.

## PlayStation Vita — installation and launch

Based on [Rinnegatamante's original installation guide](https://github.com/Rinnegatamante/ffd-vita#setup-instructions-for-end-users).

1. Download **[ffd_vita_enhanced_experimental.vpk](https://github.com/sourmelee/ffl_mod/releases/download/console-preview-2026-11-21/ffd_vita_enhanced_experimental.vpk)** from the release.
2. On a Vita already configured for homebrew, install the required kernel plugins **[kubridge](https://github.com/TheOfficialFloW/kubridge/releases)** and **[FdFix](https://github.com/TheOfficialFloW/FdFix/releases)** according to their instructions. Add their paths under **`*KERNEL`** in the taiHEN configuration that your setup actually uses (often `ur0:tai/config.txt`, though some installations use `ux0:tai/config.txt`). For example, if your plugin files are stored at `ur0:tai/`:

   ```ini
   *KERNEL
   ur0:tai/kubridge.skprx
   ur0:tai/fd_fix.skprx
   ```

   **Important:** The original Vita port specifically says **not** to install `fd_fix.skprx` if you are using **rePatch**. Don't duplicate plugin entries or edit an inactive taiHEN config. Reload/reboot as required by your homebrew setup after installing kernel plugins.
3. Ensure **`libshacccg.suprx`** is installed. The original Vita loader checks for `ur0:/data/libshacccg.suprx` or `ur0:/data/external/libshacccg.suprx`. Follow the [shader-compiler setup reference linked by the original Vita port](https://samilops2.gitbook.io/vita-troubleshooting-guide/shader-compiler/extract-libshacccg.suprx) if needed.
4. Transfer the VPK to your Vita using your usual USB/FTP transfer method; open it with **VitaShell** and choose **Install**. It installs a normal LiveArea application called *Final Fantasy Dimensions* (the upstream Vita project uses title ID `FFDIMEN01`). A previously installed copy can be overwritten, so back it up first.
5. Create the Vita data folder **`ux0:data/ffd/`**.
6. Extract the compatible **32-bit `lib/armeabi/libjniproxy.so`** from your legally obtained Android **1.1.3** game APK to **`ux0:data/ffd/libjniproxy.so`**.
7. Copy the **`res/` directory** from the APK beneath **`ux0:data/ffd/`**, preserving subdirectories and other assets. The original loader reads files beneath `ux0:data/ffd/res/raw...`.
8. Place your legally obtained game OBB at **`ux0:data/ffd/main.obb`** (rename the corresponding OBB file to `main.obb` when necessary).
9. From the Vita **LiveArea**, launch *Final Fantasy Dimensions*. Once in the world map, the original port recommends **Menu → Config → Controls → Fix** for physical gamepad controls.

Expected data layout:

```text
ux0:data/ffd/
├── libjniproxy.so          # YOUR Android 1.1.3 ARM32 engine
├── main.obb                # YOUR legally obtained game data
├── res/                    # preserve full APK resource structure
│   └── raw/
│       ├── moogle.png
│       ├── r_chocobo.png
│       └── person.anmb2
└── ffdvita_enhanced.cfg    # optional; start with both values 0
```

**Experimental Vita options:** Create the UTF-8 text file **`ux0:data/ffd/ffdvita_enhanced.cfg`**:

```ini
game_aware_navigation=0
experimental_widescreen=0
```

Both features are off by default. Only after verifying the baseline, set `game_aware_navigation=1` while leaving widescreen at 0. This attempts **L + D-pad** menu selection and **L + Cross** confirmation on real in-game widget candidates. Next, turn navigation off and test `experimental_widescreen=1` separately. The widescreen code on Vita is an **incomplete projection experiment**, not verified full expanded-field rendering.

## Initial verification and rollback

**Suggested test order on each console:** (1) boot with both options 0; (2) title/start/load/save; (3) field movement; (4) controller navigation only in title, field menu, equipment/shop, and battle; (5) widescreen only, inspecting field edges, map camera/culling, backgrounds, dialogue, mini-map, HUD, touch alignment and effects. Don't enable both experiments together until each has passed its own checks.

If a console crashes, returns to Homebrew Menu/LiveArea, shows corrupt graphics, or ignores controls:

- Confirm **correct architecture, compatible original game version, and exact paths**, including file extensions. A 64-bit Switch library cannot load in the 32-bit Vita loader, and neither port is interchangeable with the 1.2.0 FFLmod Android binary.
- Set both feature flags back to `0` (or remove the experimental config file) and relaunch. This only disables *optional features*; it cannot repair an incompatible game library or bugs in the experimental wrapper.
- For Switch, launch with full memory rather than Album/applet mode.
- For Vita, verify active kernel plugin configuration, kubridge and FdFix/rePatch compatibility, and the shader compiler module.
- Restore your previous **working NRO or VPK** if needed. Keep backups of saves and data; a previous Vita installation uses the same app title ID, so switching between builds may affect the same save storage.

**Do not distribute the extracted Android game files.** These builds are unofficial community experiments, without endorsement by Square Enix or by the original port authors. Please retain upstream [credits](PORTABLE_CONSOLE_CREDITS.md) and [license notices](PORTABLE_CONSOLE_NOTICES.txt) when redistributing the modified wrappers.
