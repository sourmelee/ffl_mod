#!/usr/bin/env python3
"""Reproducibly prepare an experimental Vita loader source tree.
No copyrighted Android binaries are copied into the generated project.
"""
from pathlib import Path
import argparse
import shutil

HERE = Path(__file__).resolve()
ROOT = HERE.parents[2]
REQUIRED = {
    "platforms/vita/ffd_vita_profile.h": "ffd_vita_profile.h",
    "platforms/vita/ffd_vita_nav_adapter.h": "ffd_vita_nav_adapter.h",
    "platforms/vita/ffd_vita_wide_adapter.h": "ffd_vita_wide_adapter.h",
    "portable/ffd_nav.h": "ffd_nav.h",
    "portable/ffd_display_transform.h": "ffd_display_transform.h",
    "src/display_geometry.h": "display_geometry.h",
}
RULES = [
    ('so_module main_mod;',
     'so_module main_mod;\n'
     '#include "ffd_vita_nav_adapter.h"\n'
     '#include "ffd_vita_wide_adapter.h"'),
    ('void patch_game(void) {\n\t\n}',
     'void patch_game(void) {\n'
     '\tfv_install_navigation(&main_mod);\n'
     '\tfv_install_widescreen(&main_mod);\n}'),
    ('\t\tSceCtrlData pad;\n'
     '\t\tsceCtrlPeekBufferPositive(0, &pad, 1);\n'
     '\t\tfakeInput(SCE_CTRL_LEFT, 78.0f, 389.0f, 5);',
     '\t\tSceCtrlData pad;\n'
     '\t\tsceCtrlPeekBufferPositive(0, &pad, 1);\n'
     '\t\tint enhanced_controls = fv_pad_frame(pad.buttons);\n'
     '\t\tfakeInput(SCE_CTRL_LEFT, 78.0f, 389.0f, 5);'),
    ('\t\tfakeInput(SCE_CTRL_LEFT, 78.0f, 389.0f, 5);',
     '\t\tif (!enhanced_controls) {\n'
     '\t\t\tfakeInput(SCE_CTRL_LEFT, 78.0f, 389.0f, 5);'),
    ('\t\tfakeInput(SCE_CTRL_CROSS, 1.0f, 1.0f, 10);',
     '\t\t\tfakeInput(SCE_CTRL_CROSS, 1.0f, 1.0f, 10);\n'
     '\t\t}\n'
     '\t\tfakeInput(SCE_CTRL_TRIANGLE, 846.0f, 37.0f, 9);'),
]
# The preceding replacement would duplicate the original Triangle line.
RULES[-1] = ('\t\tfakeInput(SCE_CTRL_TRIANGLE, 846.0f, 37.0f, 9);\n'
             '\t\tfakeInput(SCE_CTRL_CROSS, 1.0f, 1.0f, 10);',
             '\t\t\tfakeInput(SCE_CTRL_CROSS, 1.0f, 1.0f, 10);\n'
             '\t\t}\n'
             '\t\t/* Triangle remains a legacy menu shortcut. */\n'
             '\t\tfakeInput(SCE_CTRL_TRIANGLE, 846.0f, 37.0f, 9);')
RULES += [
    ('\t\tMainActivity_render(fake_env, NULL);',
     '\t\tfv_send_touch(MainActivity_touch, fake_env, i);\n'
     '\t\tfv_before_render();\n'
     '\t\tMainActivity_render(fake_env, NULL);'),
]

def patch(text):
    if 'fv_install_navigation(&main_mod)' in text:
        raise ValueError("loader already patched")
    for old, new in RULES:
        if text.count(old) != 1:
            raise ValueError(f"expected one upstream anchor, found {text.count(old)}: {old[:90]!r}")
        text = text.replace(old, new, 1)
    return text

def main():
    parser = argparse.ArgumentParser()
    parser.add_argument("vita_dir", type=Path)
    parser.add_argument("--dry-run", action="store_true")
    args = parser.parse_args()
    directory = args.vita_dir.resolve()
    loader = directory / "loader/main.c"
    if not loader.is_file() or not (directory / "CMakeLists.txt").is_file():
        parser.error("expected a Rinnegatamante/ffd-vita source checkout")
    try:
        modified = patch(loader.read_text())
    except ValueError as exc:
        parser.error(str(exc))
    for path, name in REQUIRED.items():
        if not (ROOT / path).is_file():
            parser.error(f"patch source missing: {path}")
        if (directory / "loader" / name).exists():
            parser.error(f"patch destination already exists: {name}")
    if args.dry_run:
        print("PASS: all source anchors and supporting headers verified")
        return
    shutil.copy2(loader, loader.with_suffix(".c.ffd-bak"))
    for src, dest in REQUIRED.items():
        dst = directory / "loader" / dest
        if dest == "ffd_display_transform.h":
            # The shared header normally lives in portable/, so its original
            # relative include must be rewritten for loader/ placement.
            data = (ROOT / src).read_text()
            needle = '#include "../src/display_geometry.h"'
            if data.count(needle) != 1:
                parser.error("unexpected portable display include")
            dst.write_text(data.replace(needle, '#include "display_geometry.h"'))
        else:
            shutil.copy2(ROOT / src, dst)
    # Upstream dialog.c omits string.h. Modern softfp GCC rejects the
    # resulting implicit memset declaration; make the fix only in this clone.
    dialog = directory / "loader" / "dialog.c"
    dialog_text = dialog.read_text()
    if "#include <string.h>" not in dialog_text:
        dialog.write_text('#include <string.h>\n' + dialog_text)
    # Upstream 2024 compatibility with current VitaSDK headers: avoid
    # collision with libc's clock_gettime and the changed vitaGL draw API.
    needle = "int clock_gettime(int clk_ik, struct timespec *t) {"
    if modified.count(needle) != 1:
        parser.error("unexpected clock_gettime shim")
    modified = modified.replace(needle,
        "int ffd_clock_gettime(int clk_ik, struct timespec *t) {")
    modified = modified.replace('&clock_gettime },', '&ffd_clock_gettime },')
    needle = 'vglDrawObjects(GL_TRIANGLE_STRIP, 4, GL_TRUE);'
    if modified.count(needle) != 1:
        parser.error("unexpected vitaGL draw API")
    modified = modified.replace(needle, 'vglDrawObjects(GL_TRIANGLE_STRIP, 4);')
    loader.write_text(modified)
    # The contemporary softfp SDK no longer ships the obsolete bridge stub.
    # The direct Vita SDK imports are supplied by the other platform stubs.
    cmake_file = directory / "CMakeLists.txt"
    cmake_data = cmake_file.read_text()
    cmake_data = cmake_data.replace("  SceLibcBridge_stub\n", "")
    # Modern libsndfile pulls in optional codecs; C++ static libraries
    # also need stdc++ after their final consumers (static link order).
    marker = "  SceMotion_stub\n)"
    replacement = ("  SceMotion_stub\n"
                   "  mpg123\n"
                   "  mp3lame\n"
                   "  opus\n"
                   "  stdc++\n"
                   "  m\n)")
    if cmake_data.count(marker) != 1:
        parser.error("unexpected Vita CMake link library footer")
    cmake_file.write_text(cmake_data.replace(marker, replacement))
    so_util = directory / "loader" / "so_util.c"
    original = so_util.read_text()
    if "void *vglGetProcAddress(const char *name);" not in original:
        insert = '#include "so_util.h"'
        if original.count(insert) != 1:
            parser.error("unexpected so_util includes")
        so_util.write_text(original.replace(insert,
            insert + '\n/* Legacy vitaGL resolver declaration. */\n'
            'void *vglGetProcAddress(const char *name);'))
    print("Experimental Vita 1.1.3 source patched. Both features off by default.")

if __name__ == "__main__":
    main()
