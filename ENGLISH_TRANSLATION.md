# English integration: 1.1.15-en1

This fork now wires the reviewed translations into the native hooks and Android
interfaces. It retains Japanese support in the same universal game installation.
Original game data and the OBB are unchanged.

The source also now includes automatic 4:3 display support; see
`DISPLAY_4X3.md` for its behavior, tests, and remaining Android verification.

## Implemented

- English recognition for the Magus, For debugging, and Recollect guards;
  the original Japanese UTF-8/SJIS checks remain available.
- English selections for 23 native names, descriptions, help strings, Dash
  labels, and the fourth configuration page. Already-patched checks use the
  same selected strings. Recollect's cost is reapplied if the table reloads.
- Migration of Styx's recognized default name, preserving other nicknames.
- 161 English story text/speaker overrides across the relevant banks, including
  four 6 JP -> 7 JP message corrections. The overlay changes only non-owning
  pointer arrays and restores original pointers when leaving English.
- Map-name lookup supplies Challenge Dungeon for the previously empty English
  record 165. The owning map-name table is untouched.
- English default and Japanese Android resources for all 68 manager entries,
  referenced by 76 Kotlin call sites plus the application label.
- 19 bilingual injected save-dialog strings at 23 original call sites. A helper
  inside the game's injected DEX uses the running game's language snapshot;
  otherwise it uses the manager's explicit locale or the Android locale.
- Manager release downloads now use `sourmelee/ffl_mod` rather than upstream.
  Compatibility and payload hash checks remain enabled.

The ELF import resolver finds and validates SetMessageList/GetMapName imports
by dynamic symbol and expected function pointer. It adds no guessed GOT offsets.
The existing mod's supported-binary checks still apply. Other mods' unexpected
nonempty text is preserved rather than overwritten.

## Validation completed locally

- Generated data/resources match the reviewed translation inputs.
- The native source passed ARM64 C compile/static analysis with Clang 19.1.5.
- Actual C overlay tests compiled and ran with MSVC 14.44 on the host: all 16
  bank layouts, all 161 overrides, all 23 native translations, source fingerprints,
  bounds/ownership guards, idempotence, language restoration, map-name lookup,
  and ELF import checks passed. This is not an Android execution test.
- The complete manager APK compiled, dexed, linked, aligned, and signed using the
  installed SDK 36.1/build-tools 36.1.0, Kotlin 2.2.20, JDK 21, and cached
  apksig 9.2.1. APK v2/v3 signatures verified; English/Japanese labels and
  min SDK 24 / target SDK 34 were inspected in the final APK.
- Save-dialog helper references and invoke register bounds passed static checks.

## Build and playtest status

`out/dist/fflmod-manager-1.1.15-en1-debug.apk` is a working **development manager
build**. It is not an installed English game patch. It uses a locally generated
debug signing certificate, separate from an upstream signed manager. The fork
needs a matching release payload before its download/install flow can obtain
this new English game mod. No release has been published.

A complete ARM64 `.so` and injected `.dex` were **not built locally**. The full
Clang driver and apktool/smali assembler are missing, and tool downloads fail
with DNS resolution errors. The installed Clang analysis tool can validate C
but cannot link the Android shared library. The installed Android Studio smali
plugin supplies an editor parser, not the required assembler.

Gradle also could not connect to its local daemon. The offline manager build
used `tools/build-manager-local.ps1` instead. Its cached tool versions differ
from the unchanged normal Gradle 8.7 / AGP 8.5.2 / Kotlin 1.9.24 configuration.

ADB could not start/connect to its server. No Android installation, screenshot,
save migration, menu inspection, or gameplay verification was performed.
All runtime changes remain unverified on the supported Android binary.

## Finish the standard build

On a host with clang/lld, Java, Python, and the pinned apktool JAR:

```sh
APKTOOL=/path/to/apktool_2.10.0.jar sh tools/build.sh - out/native
python3 tools/mkpatch.py out/native/work out/payload 1.1.15-en1
cd manager
gradle :app:assembleDebug --no-daemon
```

The source-only `-` build needs no APK or OBB as input. The build workflow now
has a payload job that runs the host tests, verifies the pinned apktool SHA-256,
and builds `.so`, `.dex`, and `files.json` artifacts. This workflow has not been
pushed or run remotely. It does not publish releases. The separate existing
release workflow still requires the project's normal release process.

The `-en1` suffix is treated as a prerelease by that existing release workflow.
If distributing it that way, enable prerelease updates in the manager by
long-pressing its version display. Do not mark this untested build stable.

## Translation maintenance

For the 110 original source strings, edit column D in
`translations/japanese_strings.csv`, then run:

```sh
node tools/generate_localization.mjs
node tools/generate_localization.mjs --check
```

This regenerates `src/english_text.h`, manager resources, and `FFLModText.smali`.
For restored dialogue, speakers, the map name, and JP award messages, edit column
D in `translations/completion_review/mod_related_translations.csv` and regenerate.
For its duplicated original source entries, `japanese_strings.csv` takes
precedence. Retain IDs, Japanese originals, and control tags. The override JSON
provides bank/slot/expected-original metadata; it is not the English editing input.
The original 352-entry audit separates the 136 mod-related entries from 216
supplemental entries; those supplemental translations are not automatically
inserted into unrelated base-game scenes.

The earlier extraction/completion tools describe the **unlocalized baseline**.
They now stop before overwriting its reviewed catalog on this integrated fork.
Use the generator/checks above for ordinary translation edits. The older audit's
coverage figures are provenance, not a new claim of a full Android playthrough.

Before release, test English and Japanese; language changes; an existing save
with a custom nickname; Styx/Blue Magic/Alarm; Recollect's cost and Slow help;
configuration page 4; restored moogle entrances; challenge and ghost-ship text;
coordinate/Karma captions; and both save import/export flows. Confirm wrapping,
font fit, page breaks, and dialogue control tags on an actual device.
