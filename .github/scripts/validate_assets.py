#!/usr/bin/env python3
"""Asset validation for MinecraftDeWish.

Checks (run from repository root):
  * No spaces in asset paths under Content/.
  * No forbidden name suffixes (_New, _Final, _Copy, _vN, ...).
  * Texture (PNG) dimensions are power-of-two and within budget.
  * Texture file size budget.
  * Mesh/source asset naming prefixes (SM_, SK_, BP_, ...).
  * FBX / blend naming.

Exit code 1 if any ERROR is found; warnings do not fail the build.
"""

from __future__ import annotations

import argparse
import os
import re
import sys
from pathlib import Path

try:
    from PIL import Image
    HAS_PIL = True
except Exception:  # pragma: no cover
    HAS_PIL = False

CONTENT = Path("Content")

# Third-party / engine-template / generated folders are excluded from strict
# checks: they are not authored by the team and cannot be renamed safely.
EXCLUDE_DIRS = {
    "Reference",
    "Developers",
    "Collections",
    "Mannequins",
    "Patrix_Texture_Pack",
    "__ExternalActors__",
    "__ExternalObjects__",
}

# Known engine-template assets that legitimately carry copy/new suffixes.
ALLOWLIST = {
    "FirstPerson/Anims/ABP_FP_Copy.uasset",
}

FORBIDDEN_SUFFIXES = re.compile(
    r"(_New|_Final|_Copy|_Old|_Backup|_Temp|_tmp|_v\d+| \(\d+\))$",
    re.IGNORECASE,
)
MAX_TEXTURE_DIM = 4096
MAX_TEXTURE_BYTES = 64 * 1024 * 1024  # 64 MB per file
MAX_FBX_BYTES = 256 * 1024 * 1024     # 256 MB per file
VALID_PREFIXES = {
    "Blueprints": ("BP_",),
    "UI": ("WBP_", "BP_"),
    "Materials": ("M_", "MI_", "MF_"),
    "Textures": ("T_",),
    "Import": ("SM_", "SK_", "T_", "M_"),
}


errors: list[str] = []
warnings: list[str] = []


def err(msg: str) -> None:
    errors.append(msg)


def warn(msg: str) -> None:
    warnings.append(msg)


def is_power_of_two(n: int) -> bool:
    return n > 0 and (n & (n - 1)) == 0


def check_name(path: Path) -> None:
    rel = path.relative_to(CONTENT).as_posix()
    if rel in ALLOWLIST:
        return
    name = path.stem
    if " " in str(path.relative_to(CONTENT)):
        err(f"SPACE IN PATH: {path}")
    if FORBIDDEN_SUFFIXES.search(name):
        err(f"FORBIDDEN SUFFIX: {path}")
    # prefix heuristic by top-level folder
    parts = path.relative_to(CONTENT).parts
    if len(parts) >= 2:
        top = parts[0]
        prefixes = VALID_PREFIXES.get(top)
        if prefixes and not any(name.startswith(p) for p in prefixes):
            # only warn; many legitimate assets use other names
            warn(f"MISSING EXPECTED PREFIX {prefixes} in {top}/: {path.name}")


def check_png(path: Path) -> None:
    size = path.stat().st_size
    if size > MAX_TEXTURE_BYTES:
        err(f"TEXTURE TOO LARGE ({size} bytes > {MAX_TEXTURE_BYTES}): {path}")
    if not HAS_PIL:
        warn(f"Pillow unavailable; skipped dimension check for {path}")
        return
    try:
        with Image.open(path) as im:
            w, h = im.size
    except Exception as exc:  # pragma: no cover
        err(f"UNREADABLE PNG {path}: {exc}")
        return
    if not (is_power_of_two(w) and is_power_of_two(h)):
        warn(f"NON-POWER-OF-TWO TEXTURE {w}x{h}: {path}")
    if w > MAX_TEXTURE_DIM or h > MAX_TEXTURE_DIM:
        err(f"TEXTURE OVER DIM BUDGET {w}x{h} > {MAX_TEXTURE_DIM}: {path}")


def check_fbx(path: Path) -> None:
    if path.stat().st_size > MAX_FBX_BYTES:
        err(f"FBX TOO LARGE: {path}")


def check_umap_uasset(path: Path) -> None:
    # binary; only name checks already done
    pass


def main() -> int:
    if not CONTENT.exists():
        print("Content/ not found; nothing to validate.")
        return 0

    for root, dirs, files in os.walk(CONTENT):
        # prune excluded dirs in-place
        dirs[:] = [d for d in dirs if d not in EXCLUDE_DIRS]
        for fn in files:
            p = Path(root) / fn
            rel = p.relative_to(CONTENT)
            # skip engine-generated folders
            if any(part.startswith("__") for part in rel.parts):
                continue
            if any(part in EXCLUDE_DIRS for part in rel.parts):
                continue
            check_name(p)
            ext = p.suffix.lower()
            if ext == ".png":
                check_png(p)
            elif ext == ".fbx":
                check_fbx(p)
            elif ext in (".uasset", ".umap"):
                check_umap_uasset(p)

    print(f"Validated assets under {CONTENT}/")
    print(f"  errors:   {len(errors)}")
    print(f"  warnings: {len(warnings)}")
    for e in errors:
        print(f"ERROR: {e}")
    for w in warnings[:50]:
        print(f"WARN:  {w}")
    if len(warnings) > 50:
        print(f"... {len(warnings) - 50} more warnings omitted")

    return 1 if errors else 0


if __name__ == "__main__":
    sys.exit(main())
