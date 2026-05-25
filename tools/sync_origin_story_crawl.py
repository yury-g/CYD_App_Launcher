#!/usr/bin/env python3
"""Sync or verify Origin Story crawl text across firmware and render tooling."""

from __future__ import annotations

import argparse
import ast
import re
from pathlib import Path


ROOT = Path(__file__).resolve().parents[1]
CRAWL_TEXT = ROOT / "docs" / "origin-story-crawl.txt"
FIRMWARE_SOURCE = ROOT / "PulseSensor_CYD.ino"
RENDER_SOURCE = ROOT / "tools" / "render_app3_origin_crawl_mock.py"


def read_crawl_lines() -> list[str]:
    return CRAWL_TEXT.read_text().splitlines()


def cpp_quote(line: str) -> str:
    return '"' + line.replace("\\", "\\\\").replace('"', '\\"') + '"'


def py_quote(line: str) -> str:
    return repr(line)


def firmware_block(lines: list[str]) -> str:
    body = ",\n".join(f"  {cpp_quote(line)}" for line in lines)
    return f"const char* const APP3_ORIGIN_CRAWL_LINES[APP3_ORIGIN_CRAWL_LINE_COUNT] = {{\n{body}\n}};"


def render_block(lines: list[str]) -> str:
    body = ",\n".join(f"    {py_quote(line)}" for line in lines)
    return f"CRAWL_LINES = [\n{body}\n]"


def extract_firmware_lines(source: str) -> list[str]:
    match = re.search(
        r"const char\* const APP3_ORIGIN_CRAWL_LINES\[APP3_ORIGIN_CRAWL_LINE_COUNT\] = \{\n(?P<body>.*?)\n\};",
        source,
        re.S,
    )
    if not match:
        raise SystemExit("Could not find APP3_ORIGIN_CRAWL_LINES block")
    return re.findall(r'"((?:\\.|[^"\\])*)"', match.group("body"))


def extract_render_lines(source: str) -> list[str]:
    match = re.search(r"CRAWL_LINES = \[(?P<body>.*?)\n\]", source, re.S)
    if not match:
        raise SystemExit("Could not find CRAWL_LINES block")
    return ast.literal_eval("[" + match.group("body") + "\n]")


def write_synced(lines: list[str]) -> None:
    firmware = FIRMWARE_SOURCE.read_text()
    firmware = re.sub(
        r"#define APP3_ORIGIN_CRAWL_LINE_COUNT \d+",
        f"#define APP3_ORIGIN_CRAWL_LINE_COUNT {len(lines)}",
        firmware,
    )
    firmware = re.sub(
        r"const char\* const APP3_ORIGIN_CRAWL_LINES\[APP3_ORIGIN_CRAWL_LINE_COUNT\] = \{\n.*?\n\};",
        firmware_block(lines),
        firmware,
        flags=re.S,
    )
    FIRMWARE_SOURCE.write_text(firmware)

    render = RENDER_SOURCE.read_text()
    render = re.sub(r"CRAWL_LINES = \[.*?\n\]", render_block(lines), render, flags=re.S)
    RENDER_SOURCE.write_text(render)


def check_synced(lines: list[str]) -> None:
    firmware = FIRMWARE_SOURCE.read_text()
    render = RENDER_SOURCE.read_text()
    firmware_lines = extract_firmware_lines(firmware)
    render_lines = extract_render_lines(render)

    if firmware_lines != lines:
        raise SystemExit("Firmware Origin Story crawl text is out of sync; run with --write")
    if render_lines != lines:
        raise SystemExit("Render Origin Story crawl text is out of sync; run with --write")
    if f"#define APP3_ORIGIN_CRAWL_LINE_COUNT {len(lines)}" not in firmware:
        raise SystemExit("APP3_ORIGIN_CRAWL_LINE_COUNT does not match docs/origin-story-crawl.txt")


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--write", action="store_true", help="Rewrite firmware and render crawl text from docs/origin-story-crawl.txt")
    args = parser.parse_args()

    lines = read_crawl_lines()
    if args.write:
        write_synced(lines)
    check_synced(lines)
    print(f"Origin Story crawl text is synced ({len(lines)} lines)")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
