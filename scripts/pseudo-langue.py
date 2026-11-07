#!/usr/bin/env python3
"""Pseudo-langue de contrôle (L11-T19, D11-14).

Usage : python3 -I scripts/pseudo-langue.py <entrée.ts> <sortie.ts>

Copie le .ts produit par lupdate en remplissant chaque <translation> par « ⟦ » + source + « ⟧ » (formes plurielles
comprises) : les marqueurs %1, %n et les balises de la source sont gardés tels quels, l'attribut type="unfinished"
est retiré. Écrit en UTF-8 sans BOM, fins de ligne LF. Le résultat ne sert qu'aux essais : jamais livré.
"""
import re
import sys

OPEN = "⟦"
CLOSE = "⟧"

MESSAGE = re.compile(r"<message\b.*?</message>", re.S)
SOURCE = re.compile(r"<source>(.*?)</source>", re.S)
TRANSLATION = re.compile(r"<translation\b[^>]*?(?:/>|>.*?</translation>)", re.S)
NUMERUS = re.compile(r"\bnumerus=\"yes\"")


def mark(source: str) -> str:
    return OPEN + source + CLOSE


def convert_message(block: str) -> str:
    found = SOURCE.search(block)
    if not found:
        return block
    marked = mark(found.group(1))
    head = block[: block.index(">") + 1]  # balise ouvrante <message …>
    if NUMERUS.search(head):
        replacement = "<translation>\n<numerusform>%s</numerusform>\n<numerusform>%s</numerusform>\n</translation>" % (
            marked,
            marked,
        )
    else:
        replacement = "<translation>%s</translation>" % marked
    return TRANSLATION.sub(lambda _m: replacement, block, count=1)


def convert(text: str) -> str:
    text = MESSAGE.sub(lambda m: convert_message(m.group(0)), text)
    # La langue cible est la pseudo-langue : lrelease n'en tire aucune règle de pluriel du français.
    return re.sub(r'(<TS\b[^>]*\blanguage=")[^"]*(")', r"\g<1>xx\g<2>", text, count=1)


def main(argv: list) -> int:
    if len(argv) != 3:
        sys.stderr.write("usage : pseudo-langue.py <entrée.ts> <sortie.ts>\n")
        return 2
    with open(argv[1], "rb") as handle:
        text = handle.read().decode("utf-8")
    if text.startswith("﻿"):
        text = text[1:]
    text = text.replace("\r\n", "\n")
    with open(argv[2], "wb") as handle:
        handle.write(convert(text).encode("utf-8"))
    return 0


if __name__ == "__main__":
    sys.exit(main(sys.argv))
