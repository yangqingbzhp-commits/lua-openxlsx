#!/usr/bin/env python3
"""
api_inventory.py — extract the public API surface from the OpenXLSX headers.

Used to audit the Lua binding coverage (see docs/API_COVERAGE.md).

Approach (best effort, not a C++ parser):
  1. strip comments, string literals and preprocessor lines
  2. find class/struct blocks by brace matching
  3. at class-body depth 1, track public/private/protected sections
  4. collect declarations terminated by ';' and record them as members

Templates, macros and unusual formatting may be reported approximately - the
output is a checklist, not a compiler.

Usage:
    tools/api_inventory.py [-o OUTPUT] [HEADER_DIR]
"""
from __future__ import annotations

import argparse
import pathlib
import re
import sys

DEFAULT_HEADER_DIR = (
    pathlib.Path(__file__).resolve().parent.parent
    / "third_party"
    / "OpenXLSX"
    / "OpenXLSX"
    / "headers"
)

# headers that intentionally are not exposed to Lua
SKIP_HEADERS = {
    "IZipArchive.hpp",
    "XLCommandQuery.hpp",
    "XLContentTypes.hpp",
    "XLRelationships.hpp",
    "XLSharedStrings.hpp",
    "XLXmlData.hpp",
    "XLXmlFile.hpp",
    "XLXmlParser.hpp",
    "XLXmlParserForwardDeclarations.hpp",
    "XLZipArchive.hpp",
    "XLConstants.hpp",
    "XLIterator.hpp",
    "XLDrawing.hpp",
    "XLRowData.hpp",
}

NOISE_PREFIXES = ("typedef ", "using ", "friend ", "static_assert", "#", "template<")
SKIP_KEYWORDS = {"return", "if", "while", "for", "switch", "catch"}


def strip_comments_and_strings(text: str) -> str:
    out = []
    i, n = 0, len(text)
    while i < n:
        two = text[i : i + 2]
        if two == "/*":
            j = text.find("*/", i + 2)
            i = n if j < 0 else j + 2
            out.append(" ")
        elif two == "//":
            j = text.find("\n", i)
            i = n if j < 0 else j
        elif text[i] in "\"'":
            quote = text[i]
            i += 1
            while i < n and text[i] != quote:
                if text[i] == "\\":
                    i += 1
                i += 1
            i += 1
            out.append('""')
        else:
            out.append(text[i])
            i += 1
    return "".join(out)


def clean_lines(text: str) -> list[str]:
    lines = []
    for line in strip_comments_and_strings(text).splitlines():
        line = line.rstrip()
        if not line.strip() or line.lstrip().startswith("#"):
            continue
        lines.append(line)
    return lines


CLASS_RE = re.compile(r"\b(class|struct)\s+(?:\w+_EXPORT\s+)?(\w+)")
SECTION_RE = re.compile(r"^\s*(public|private|protected)\s*:")
MEMBER_NAME_RE = re.compile(r"([A-Za-z_]\w*|~[A-Za-z_]\w*|operator\s*\S+)\s*\(")


def member_name(decl: str) -> str | None:
    decl = " ".join(decl.split())
    if not decl or decl.startswith(NOISE_PREFIXES):
        return None
    if "(" not in decl:
        return None
    head = decl.split("(", 1)[0]
    if any(head.strip().startswith(kw) for kw in SKIP_KEYWORDS):
        return None
    m = MEMBER_NAME_RE.search(head + "(")
    if not m:
        return None
    name = m.group(1).strip()
    if name.startswith("operator"):
        name = "operator" + re.sub(r"\s+", "_", name[len("operator"):].strip()).strip("_")
        return name
    return name


def parse_header(path: pathlib.Path) -> dict[str, list[str]]:
    lines = clean_lines(path.read_text(encoding="utf-8", errors="replace"))
    classes: dict[str, list[str]] = {}

    class_depth = 0          # brace depth of the current class body
    current: str | None = None
    pending_class: str | None = None
    visibility = "private"
    depth = 0
    pending: list[str] = []

    def flush_pending():
        nonlocal pending
        if current is not None and visibility == "public" and pending:
            name = member_name(" ".join(pending))
            if name:
                classes[current].append(name)
        pending = []

    for line in lines:
        stripped = line.strip()

        if current is None:
            if pending_class is None:
                m = CLASS_RE.search(line)
                # a class declaration ends with ';' (forward declaration) - skip those
                if m and ";" not in line:
                    pending_class = m.group(2)
                    classes.setdefault(pending_class, [])
            depth += line.count("{") - line.count("}")
            if pending_class is not None and "{" in line:
                current = pending_class
                class_depth = depth
                pending_class = None
                visibility = "private"
                pending = []
            continue

        # ---- inside a class body ----
        if not pending and depth == class_depth:
            sm = SECTION_RE.match(line)
            if sm:
                visibility = sm.group(1)
                depth += line.count("{") - line.count("}")
                continue

        pending.append(stripped)
        depth += line.count("{") - line.count("}")

        if stripped.endswith(";") and depth == class_depth:
            flush_pending()
        elif depth < class_depth:
            pending = []
            current = None
            visibility = "private"
            class_depth = 0

    return classes


def main() -> int:
    ap = argparse.ArgumentParser(description=__doc__)
    ap.add_argument("header_dir", nargs="?", default=str(DEFAULT_HEADER_DIR))
    ap.add_argument("-o", "--output", default=None)
    args = ap.parse_args()

    header_dir = pathlib.Path(args.header_dir)
    if not header_dir.is_dir():
        print(f"error: header directory not found: {header_dir}", file=sys.stderr)
        return 1

    lines: list[str] = []
    total = 0
    for header in sorted(header_dir.glob("*.hpp")):
        classes = parse_header(header)
        classes = {c: sorted(set(m)) for c, m in classes.items() if m}
        if not classes:
            continue
        suffix = "  [NOT exposed to Lua: internal infrastructure]" if header.name in SKIP_HEADERS else ""
        lines.append(f"## {header.name}{suffix}")
        for cls, methods in classes.items():
            lines.append(f"### {cls} ({len(methods)} public members)")
            for name in methods:
                lines.append(f"  - {name}")
            total += len(methods)
        lines.append("")

    lines.append(f"# total public members listed: {total}")
    out = "\n".join(lines)
    if args.output:
        pathlib.Path(args.output).write_text(out + "\n", encoding="utf-8")
        print(f"written {args.output} ({total} members)")
    else:
        print(out)
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
