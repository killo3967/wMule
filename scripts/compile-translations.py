#!/usr/bin/env python3
"""Compila los .po a .mo y los sincroniza con assets/locale."""

from __future__ import annotations

import argparse
import shutil
import sys
from pathlib import Path


SCRIPT_DIR = Path(__file__).resolve().parent
PROJECT_ROOT = SCRIPT_DIR.parent

sys.path.insert(0, str(SCRIPT_DIR))

from po2mo import compile_catalog  # noqa: E402


def main(argv: list[str]) -> int:
    parser = argparse.ArgumentParser(description="Compila los .po y sincroniza los .mo")
    parser.add_argument(
        "--project-root",
        default=str(PROJECT_ROOT),
        help="Raíz del proyecto",
    )
    parser.add_argument(
        "--build-dir",
        default=None,
        help="Directorio de build (opcional)",
    )
    parser.add_argument(
        "--configs",
        nargs="+",
        default=["Debug", "Release"],
        help="Configuraciones de build a sincronizar",
    )
    parser.add_argument(
        "--copy-to-build",
        action="store_true",
        help="Copia los catálogos al árbol de build",
    )
    args = parser.parse_args(argv)

    project_root = Path(args.project_root).resolve()
    build_dir = (
        Path(args.build_dir).resolve() if args.build_dir else project_root / "build"
    )
    po_dir = project_root / "po"
    assets_locale = project_root / "assets" / "locale"
    po_files = sorted(po_dir.glob("*.po"))

    if not po_files:
        print("[i18n] No se encontraron archivos .po.")
        return 0

    for po_path in po_files:
        lang = po_path.stem
        target_dir = assets_locale / lang / "LC_MESSAGES"
        target_dir.mkdir(parents=True, exist_ok=True)
        for domain in ("wmule", "amule"):
            compile_catalog(str(po_path), str(target_dir / f"{domain}.mo"))

    if args.copy_to_build:
        for cfg in args.configs:
            for lang_dir in assets_locale.glob("*"):
                if not lang_dir.is_dir():
                    continue
                destination = build_dir / "src" / cfg / "locale" / lang_dir.name
                destination.mkdir(parents=True, exist_ok=True)
                shutil.copytree(lang_dir, destination, dirs_exist_ok=True)

    print(f"[i18n] Compilación completada para {len(po_files)} idiomas.")
    return 0


if __name__ == "__main__":
    raise SystemExit(main(sys.argv[1:]))
