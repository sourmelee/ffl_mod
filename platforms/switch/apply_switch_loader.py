#!/usr/bin/env python3
"""Apply the opt-in, source-only Switch navigation experiment to ffd_nx.

No proprietary .so/OBB files are read or copied by this script.
Run --dry-run first. Refuses unknown upstream source without guessing.
"""
from __future__ import annotations
import argparse
from pathlib import Path
import shutil
import sys

ROOT = Path(__file__).resolve().parents[2]
SOURCES = (
    "portable/ffd_nav.h",
    "portable/ffd_display_transform.h",
    "src/display_geometry.h",
    "platforms/switch/ffd_switch_profile.h",
    "platforms/switch/ffd_switch_menu_layout.h",
    "platforms/switch/ffd_switch_nav_experiment.h",
)

def change(source: str, old: str, new: str) -> str:
    count = source.count(old)
    if count != 1:
        raise RuntimeError(f"expected exactly one upstream anchor, got {count}: {old[:95]!r}")
    return source.replace(old, new, 1)

def patch(content: str) -> str:
    content = change(content, '#include "opensles.h"',
                     '#include "opensles.h"\n#include "../platforms/switch/ffd_switch_nav_experiment.h"')
    content = change(content, '  so_finalize(&game_mod);',
                     '  ffd_nx_install_navigation(&game_mod); /* optional and fail-closed */\n  so_finalize(&game_mod);')
    content = change(content, '  int dn    = (d & HidNpadButton_Down)  || ls.y < -DZ;',
                     '  int dn    = (d & HidNpadButton_Down)  || ls.y < -DZ;\n'
                     '  int nxnav = ffd_nx_navigation_input((d & HidNpadButton_ZL) != 0,\n'
                     '      left, right, up, dn, (d & HidNpadButton_A) != 0, screen_width, screen_height);')
    for key in ("left", "right", "up", "dn"):
        content = change(content, f'  ADD_PT({key},', f'  ADD_PT({key} && !nxnav,')
    content = change(content, '  if (cur_active) ADD_PT(a, 4, cur_x, cur_y);',
                     '  if (nxnav) ADD_PT(a, 4, ffd_nx_nav_x, ffd_nx_nav_y);\n'
                     '  else if (cur_active) ADD_PT(a, 4, cur_x, cur_y);')
    content = change(content, '  if (!cur_active)\n    return;\n  const int cx = (int)cur_x;\n  const int cy_gl = screen_height - (int)cur_y;',
                     '  if (!cur_active && !ffd_nx_nav_active)\n    return;\n'
                     '  const int cx = (int)(ffd_nx_nav_active ? ffd_nx_nav_x : cur_x);\n'
                     '  const int cy_gl = screen_height - (int)(ffd_nx_nav_active ? ffd_nx_nav_y : cur_y);')
    content = change(content, '    update_input();\n    e_render(fake_env, thiz);',
                     '    update_input();\n    ffd_nx_begin_render();\n    e_render(fake_env, thiz);')
    return content

def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("ffd_nx", type=Path, help="a checked-out NaGaa95/ffd_nx repository")
    ap.add_argument("--dry-run", action="store_true")
    a = ap.parse_args()
    root = a.ffd_nx.resolve()
    main_path = root / "source/main.c"
    if not main_path.is_file() or not (root / "source/so_util.h").is_file():
        ap.error("not a recognized ffd_nx checkout")
    for src in SOURCES:
        if not (ROOT / src).is_file():
            ap.error(f"missing patch source: {ROOT / src}")
    data = main_path.read_text(encoding="utf-8")
    if '#include "../platforms/switch/ffd_switch_nav_experiment.h"' in data:
        ap.error("already patched; refusing to apply twice")
    output = patch(data)
    if a.dry_run:
        print("OK: all ten upstream anchors verified; no files modified")
        return
    backup = main_path.with_suffix(".c.ffd-bak")
    if backup.exists():
        ap.error(f"backup already exists: {backup}")
    shutil.copy2(main_path, backup)
    for source in SOURCES:
        dst = root / source
        dst.parent.mkdir(parents=True, exist_ok=True)
        shutil.copy2(ROOT / source, dst)
    main_path.write_text(output, encoding="utf-8")
    print("Patched ffd_nx sources. Review git diff before compilation.")
    print("Experimental feature remains OFF unless enhanced.txt contains game_aware_navigation=1")

if __name__ == "__main__":
    main()
