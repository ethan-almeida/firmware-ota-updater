#!/usr/bin/env python3
import argparse
from pathlib import Path

from cryptography.hazmat.primitives import serialization
from cryptography.hazmat.primitives.asymmetric import ec


def generate_keypair():
    key = ec.generate_private_key(ec.SECP256R1())
    private_pem = key.private_bytes(
        encoding=serialization.Encoding.PEM,
        format=serialization.PrivateFormat.PKCS8,
        encryption_algorithm=serialization.NoEncryption(),
    )
    numbers = key.public_key().public_numbers()
    public_raw = numbers.x.to_bytes(32, "big") + numbers.y.to_bytes(32, "big")
    return private_pem, public_raw


def public_raw_from_pem(private_pem):
    key = serialization.load_pem_private_key(private_pem, password=None)
    numbers = key.public_key().public_numbers()
    return numbers.x.to_bytes(32, "big") + numbers.y.to_bytes(32, "big")


def write_header(path, public_raw):
    body = ", ".join(f"0x{b:02X}" for b in public_raw)
    text = (
        "#ifndef IMAGE_PUBKEY_H\n"
        "#define IMAGE_PUBKEY_H\n"
        "\n"
        "#include <stdint.h>\n"
        "\n"
        f"static const uint8_t image_public_key[64] = {{ {body} }};\n"
        "\n"
        "#endif\n"
    )
    Path(path).write_text(text)


def main():
    parser = argparse.ArgumentParser(description="Generate an ECDSA P-256 key pair.")
    parser.add_argument("--private", required=True, help="output path for the private key PEM")
    parser.add_argument("--public", required=True, help="output path for the raw 64-byte public key")
    parser.add_argument("--header", help="optional output path for a C header embedding the public key")
    args = parser.parse_args()

    private_pem, public_raw = generate_keypair()

    Path(args.private).parent.mkdir(parents=True, exist_ok=True)
    Path(args.private).write_bytes(private_pem)
    Path(args.public).write_bytes(public_raw)

    if args.header:
        write_header(args.header, public_raw)

    print(f"private key: {args.private}")
    print(f"public key:  {args.public} ({len(public_raw)} bytes)")


if __name__ == "__main__":
    main()
