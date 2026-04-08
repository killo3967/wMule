#!/usr/bin/env python3
"""Regenera el catálogo POT de wMule desde el código fuente."""

from __future__ import annotations

import argparse
import shutil
import subprocess
import sys
from pathlib import Path


SCRIPT_DIR = Path(__file__).resolve().parent
PROJECT_ROOT = SCRIPT_DIR.parent


def generate_pot(project_root: Path, pot_path: Path) -> None:
    xgettext = shutil.which("xgettext")
    if not xgettext:
        raise RuntimeError("No se encontró xgettext en PATH.")

    cmd = [
        xgettext,
        "--from-code=UTF-8",
        "--force-po",
        f"--output={pot_path}",
        "--files-from=po/POTFILES.in",
        "--keyword=_",
        "--keyword=wxTRANSLATE",
        "--keyword=wxPLURAL:1,2",
    ]
    result = subprocess.run(cmd, cwd=project_root, check=False)
    if result.returncode != 0:
        raise RuntimeError(f"xgettext devolvió {result.returncode}")


def main(argv: list[str]) -> int:
    parser = argparse.ArgumentParser(description="Regenera el archivo POT del proyecto")
    parser.add_argument(
        "--project-root",
        default=str(PROJECT_ROOT),
        help="Raíz del proyecto",
    )
    parser.add_argument(
        "--pot",
        default=None,
        help="Ruta del POT de salida (por defecto: po/amule.pot)",
    )
    args = parser.parse_args(argv)

    project_root = Path(args.project_root).resolve()
    pot_path = Path(args.pot) if args.pot else project_root / "po" / "amule.pot"
    generate_pot(project_root, pot_path)
    print(f"[i18n] POT regenerado en {pot_path}")
    return 0


if __name__ == "__main__":
    raise SystemExit(main(sys.argv[1:]))
