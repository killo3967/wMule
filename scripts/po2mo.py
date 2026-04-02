#!/usr/bin/env python3
"""Minimal msgfmt replacement to compile .po catalogs into .mo files.

Derived from the CPython Tools/i18n/msgfmt.py implementation (PSF license).
This version keeps the dependency surface tiny so it can run on stock
Windows/Python setups bundled with wMule tooling.
"""

from __future__ import annotations

import argparse
import ast
import io
import os
import struct
import sys
from typing import Dict, List


class PoEntry:
    __slots__ = ("context", "msgid", "msgid_plural", "msgstr", "msgstr_plural", "fuzzy")

    def __init__(self) -> None:
        self.context: List[str] = []
        self.msgid: List[str] = []
        self.msgid_plural: List[str] = []
        self.msgstr: List[str] = []
        self.msgstr_plural: Dict[int, List[str]] = {}
        self.fuzzy: bool = False


def _unquote(token: str) -> str:
    token = token.strip()
    if not token:
        return ""
    try:
        return ast.literal_eval(token)
    except Exception as exc:  # pragma: no cover - defensive guard
        raise ValueError(f"Cadena inválida en archivo .po: {token}") from exc


def _flush_entry(entry: PoEntry, catalog: Dict[str, str], *, keep_fuzzy: bool) -> None:
    if not entry.msgid:
        return
    if entry.fuzzy and not keep_fuzzy:
        entry.__init__()
        return

    msgid = "".join(entry.msgid)
    if entry.context:
        msgctxt = "".join(entry.context)
        msgid = f"{msgctxt}\x04{msgid}"

    if entry.msgid_plural:
        forms = []
        for idx in sorted(entry.msgstr_plural):
            forms.append("".join(entry.msgstr_plural[idx]))
        value = "\x00".join(forms)
    else:
        value = "".join(entry.msgstr)

    catalog[msgid] = value
    entry.__init__()


def parse_po(path: str, *, keep_fuzzy: bool = False) -> Dict[str, str]:
    catalog: Dict[str, str] = {}
    entry = PoEntry()
    section = None

    with open(path, "r", encoding="utf-8") as fh:
        for raw_line in fh:
            line = raw_line.rstrip("\n")
            if line.startswith("#,"):
                if "fuzzy" in line:
                    entry.fuzzy = True
                continue
            if line.startswith("#"):
                continue
            if not line.strip():
                _flush_entry(entry, catalog, keep_fuzzy=keep_fuzzy)
                section = None
                continue

            if line.startswith("msgctxt"):
                section = "msgctxt"
                entry.context = [_unquote(line.split(None, 1)[1])]
            elif line.startswith("msgid_plural"):
                section = "msgid_plural"
                entry.msgid_plural = [_unquote(line.split(None, 1)[1])]
            elif line.startswith("msgid"):
                section = "msgid"
                entry.msgid = [_unquote(line.split(None, 1)[1])]
            elif line.startswith("msgstr["):
                section = "msgstr_plural"
                idx_token = line[len("msgstr[") :].split("]", 1)[0]
                idx = int(idx_token)
                entry.msgstr_plural[idx] = [_unquote(line.split(None, 1)[1])]
                section = f"msgstr_plural_{idx}"
            elif line.startswith("msgstr"):
                section = "msgstr"
                entry.msgstr = [_unquote(line.split(None, 1)[1])]
            else:
                # Continuation line
                if section == "msgid":
                    entry.msgid.append(_unquote(line))
                elif section == "msgid_plural":
                    entry.msgid_plural.append(_unquote(line))
                elif section == "msgctxt":
                    entry.context.append(_unquote(line))
                elif section == "msgstr":
                    entry.msgstr.append(_unquote(line))
                elif section and section.startswith("msgstr_plural_"):
                    idx = int(section.rsplit("_", 1)[1])
                    entry.msgstr_plural[idx].append(_unquote(line))
                else:
                    raise ValueError(f"Formato desconocido en {path}: {line}")

    _flush_entry(entry, catalog, keep_fuzzy=keep_fuzzy)
    return catalog


def write_mo(messages: Dict[str, str], destination: str) -> None:
    items = sorted(messages.items())
    ids = bytearray()
    strs = bytearray()
    offsets_ids: List[tuple[int, int]] = []
    offsets_strs: List[tuple[int, int]] = []

    for msgid, msgstr in items:
        msgid_bytes = msgid.encode("utf-8")
        ids_offset = len(ids)
        ids.extend(msgid_bytes + b"\0")
        offsets_ids.append((len(msgid_bytes), ids_offset))

        msgstr_bytes = msgstr.encode("utf-8")
        strs_offset = len(strs)
        strs.extend(msgstr_bytes + b"\0")
        offsets_strs.append((len(msgstr_bytes), strs_offset))

    n = len(items)
    header_size = 7 * 4
    table_ids_offset = header_size
    table_strs_offset = table_ids_offset + n * 8
    ids_data_offset = table_strs_offset + n * 8
    strs_data_offset = ids_data_offset + len(ids)

    with open(destination, "wb") as out:
        out.write(
            struct.pack(
                "Iiiiiii", 0x950412DE, 0, n, table_ids_offset, table_strs_offset, 0, 0
            )
        )

        for length, offset in offsets_ids:
            out.write(struct.pack("II", length, ids_data_offset + offset))

        for length, offset in offsets_strs:
            out.write(struct.pack("II", length, strs_data_offset + offset))

        out.write(ids)
        out.write(strs)


def compile_catalog(source: str, target: str, *, keep_fuzzy: bool = False) -> None:
    catalog = parse_po(source, keep_fuzzy=keep_fuzzy)
    os.makedirs(os.path.dirname(target), exist_ok=True)
    write_mo(catalog, target)


def main(argv: List[str]) -> int:
    parser = argparse.ArgumentParser(
        description="Convierte un archivo .po en .mo (gettext)"
    )
    parser.add_argument("source", help="Archivo .po de entrada")
    parser.add_argument("target", help="Archivo .mo de salida")
    parser.add_argument(
        "--keep-fuzzy",
        action="store_true",
        dest="keep_fuzzy",
        help="Incluir entradas marcadas como fuzzy",
    )
    args = parser.parse_args(argv)

    compile_catalog(args.source, args.target, keep_fuzzy=args.keep_fuzzy)
    return 0


if __name__ == "__main__":
    raise SystemExit(main(sys.argv[1:]))
