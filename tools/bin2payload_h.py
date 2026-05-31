#!/usr/bin/env python3
"""Convert a raw RCM payload .bin into include/payload.h."""

from __future__ import annotations

import argparse
from pathlib import Path


def emit_header(data: bytes, out_path: Path) -> None:
    lines = [
        "#pragma once",
        "",
        "#include <stdint.h>",
        "",
        f"static const uint32_t rcm_payload_len = {len(data)}u;",
        "static const uint8_t rcm_payload[] = {",
    ]

    for offset in range(0, len(data), 12):
        chunk = data[offset : offset + 12]
        rendered = ", ".join(f"0x{byte:02x}" for byte in chunk)
        lines.append(f"    {rendered},")

    lines.extend(["};", ""])
    out_path.write_text("\n".join(lines), encoding="utf-8")


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("payload", type=Path, help="Input payload .bin")
    parser.add_argument(
        "output",
        type=Path,
        nargs="?",
        default=Path("include/payload.h"),
        help="Output header path",
    )
    args = parser.parse_args()

    data = args.payload.read_bytes()
    if not data:
        raise SystemExit("input payload is empty")

    args.output.parent.mkdir(parents=True, exist_ok=True)
    emit_header(data, args.output)
    print(f"wrote {args.output} ({len(data)} bytes)")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
