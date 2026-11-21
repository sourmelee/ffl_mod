#!/usr/bin/env python3
"""Offline verification for the user-provided Switch candidate libjniproxy.so.
Requires Python 3.10+ and readelf (binutils). No game assets are distributed.
"""
from __future__ import annotations
import argparse
import hashlib
import json
from pathlib import Path
import re
import subprocess
import sys

PROFILE = Path(__file__).with_name("switch_119_profile.json")

def verify(binary: Path, profile: dict) -> list[str]:
    data = binary.read_bytes()
    sha = hashlib.sha256(data).hexdigest()
    if sha != profile["sha256"]:
        raise ValueError(f"SHA-256 mismatch; refusing unknown game library: {sha}")
    if data[:6] != b"\x7fELF\x02\x01" or int.from_bytes(data[18:20], "little") != 183:
        raise ValueError("Expected little-endian ELF64 AArch64 library")
    symbols = subprocess.check_output(
        ["readelf", "-W", "--dyn-syms", str(binary)], text=True)
    relocations = subprocess.check_output(
        ["readelf", "-W", "-r", str(binary)], text=True)
    output = []
    for key, spec in profile["symbols"].items():
        symbol = re.escape(spec["elf_symbol"])
        sym_hits = [int(m.group(1), 16) for line in symbols.splitlines()
                    if (m := re.search(
                        r"^\s*\d+:\s*([0-9a-f]+)\s+\d+\s+FUNC.*\s"
                        + symbol + r"(?:\s|$)", line))]
        rel_hits = [int(m.group(1), 16) for line in relocations.splitlines()
                    if (m := re.search(
                        r"^([0-9a-f]+)\s+\S+\s+R_AARCH64_JUMP_SLOT\s+\S+\s+"
                        + symbol + r"(?:\s|$)", line))]
        if sym_hits != [int(spec["entry"], 16)] or rel_hits != [int(spec["jump_slot"], 16)]:
            raise ValueError(f"{key}: symbol or relocation does not match the profile")
        output.append(f"{key:20} entry={spec['entry']}  JUMP_SLOT={spec['jump_slot']}")
    return output

def main() -> int:
    parser = argparse.ArgumentParser()
    parser.add_argument("so", type=Path)
    args = parser.parse_args()
    try:
        result = verify(args.so, json.loads(PROFILE.read_text(encoding="utf-8")))
        print(f"PASS: verified {len(result)} native targets and relocations")
        print("\n".join(result))
        return 0
    except (OSError, ValueError, subprocess.CalledProcessError) as exc:
        print(f"FAIL: {exc}", file=sys.stderr)
        return 1

if __name__ == "__main__":
    raise SystemExit(main())
