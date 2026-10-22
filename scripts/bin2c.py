#!/usr/bin/env python3
import argparse
from pathlib import Path


def format_array(symbol, data):
    rows = []
    for offset in range(0, len(data), 12):
        chunk = data[offset:offset + 12]
        rows.append("    " + ", ".join(f"0x{b:02X}" for b in chunk) + ",")
    body = "\n".join(rows)
    return (
        "#include <stddef.h>\n"
        "#include <stdint.h>\n"
        "\n"
        f"const uint8_t {symbol}[] = {{\n{body}\n}};\n"
        f"const size_t {symbol}_len = sizeof({symbol});\n"
    )


def main():
    parser = argparse.ArgumentParser(description="Convert a binary file into a C array source file.")
    parser.add_argument("--in", dest="input", required=True, help="input binary")
    parser.add_argument("--out", required=True, help="output .c file")
    parser.add_argument("--symbol", required=True, help="C symbol name")
    args = parser.parse_args()

    data = Path(args.input).read_bytes()
    Path(args.out).parent.mkdir(parents=True, exist_ok=True)
    Path(args.out).write_text(format_array(args.symbol, data))
    print(f"wrote {args.out} ({len(data)} bytes)")


if __name__ == "__main__":
    main()
