#!/usr/bin/env python3
import argparse
import hashlib
import struct
import zlib
from pathlib import Path

import sign_image

PAYLOAD_LEN = 256
LOAD_ADDRESS = 0x08010000
ENTRY_ADDRESS = 0x08010000
FIRMWARE_VERSION = 1
BODY_SIZE = sign_image.HEADER_STRUCT.size
FLIP_TARGETS = {
    "payload": sign_image.IMAGE_PAYLOAD_OFFSET,
    "digest": BODY_SIZE - sign_image.SIGNATURE_LEN - sign_image.SHA256_LEN,
    "signature": BODY_SIZE - sign_image.SIGNATURE_LEN,
}


def make_payload():
    return bytes((i * 7 + 3) & 0xFF for i in range(PAYLOAD_LEN))


def public_raw(private_key):
    numbers = private_key.public_key().public_numbers()
    return numbers.x.to_bytes(32, "big") + numbers.y.to_bytes(32, "big")


def flip_byte(data, offset):
    mutated = bytearray(data)
    mutated[offset] ^= 0x01
    return bytes(mutated)


def reseal_header(image):
    body = image[:sign_image.IMAGE_HEADER_SIZE - 4]
    crc = zlib.crc32(body) & 0xFFFFFFFF
    return body + struct.pack("<I", crc) + image[sign_image.IMAGE_HEADER_SIZE:]


def build_cases(private_key, payload):
    signed = sign_image.build_signed_image(
        private_key, payload, FIRMWARE_VERSION, LOAD_ADDRESS, ENTRY_ADDRESS
    )
    pubkey = public_raw(private_key)
    flipped_pubkey = flip_byte(pubkey, 0)

    cases = [
        ("OK", pubkey, signed),
        ("DIGEST", pubkey, flip_byte(signed, FLIP_TARGETS["payload"])),
        ("DIGEST", pubkey, reseal_header(flip_byte(signed, FLIP_TARGETS["digest"]))),
        ("SIGNATURE", pubkey, reseal_header(flip_byte(signed, FLIP_TARGETS["signature"]))),
        ("SIGNATURE", flipped_pubkey, signed),
        ("IMAGE", pubkey, signed[:100]),
        ("IMAGE", pubkey, signed[:sign_image.IMAGE_PAYLOAD_OFFSET + 200]),
        ("IMAGE", pubkey, signed + b"\x00"),
        ("IMAGE", pubkey, flip_byte(signed, FLIP_TARGETS["digest"])),
    ]
    return cases


def cross_check(private_key, signed, payload):
    digest = hashlib.sha256(payload).digest()
    signature = signed[
        sign_image.HEADER_STRUCT.size - sign_image.SIGNATURE_LEN:
        sign_image.HEADER_STRUCT.size
    ]
    if not sign_image.verify_digest(private_key.public_key(), digest, signature):
        raise SystemExit("cross-check failed")


def main():
    parser = argparse.ArgumentParser(
        description="Generate shared verification vectors consumed by host tests."
    )
    parser.add_argument("--key", required=True, help="private key PEM (test key)")
    parser.add_argument("--vectors-dir", required=True, help="output directory")
    args = parser.parse_args()

    vectors_dir = Path(args.vectors_dir)
    vectors_dir.mkdir(parents=True, exist_ok=True)

    private_key = sign_image.load_private_key(args.key)
    payload = make_payload()
    signed = sign_image.build_signed_image(
        private_key, payload, FIRMWARE_VERSION, LOAD_ADDRESS, ENTRY_ADDRESS
    )
    cross_check(private_key, signed, payload)

    cases = build_cases(private_key, payload)

    lines = ["# expected pubkey_hex image_hex"]
    for expected, pubkey, image in cases:
        lines.append(
            f"{expected} {pubkey.hex()} {image.hex()}"
        )

    (vectors_dir / "payload.bin").write_bytes(payload)
    (vectors_dir / "test_public_key.bin").write_bytes(public_raw(private_key))
    (vectors_dir / "verify_cases.txt").write_text("\n".join(lines) + "\n")

    print(f"payload:  {vectors_dir / 'payload.bin'} ({len(payload)} bytes)")
    print(f"pubkey:   {vectors_dir / 'test_public_key.bin'}")
    print(f"vectors:  {vectors_dir / 'verify_cases.txt'} ({len(cases)} cases)")


if __name__ == "__main__":
    main()
