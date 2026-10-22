#!/usr/bin/env python3
import argparse
import hashlib
import struct
import sys
import zlib
from pathlib import Path

sys.path.insert(0, str(Path(__file__).parent))

from sign_image import build_signed_image, load_private_key, verify_digest  # noqa: E402

SLOT_A_BASE = 0x08010000
SLOT_B_BASE = 0x08080000

IMAGE_PAYLOAD_OFFSET = 512

SLOT_A_LOAD = SLOT_A_BASE + IMAGE_PAYLOAD_OFFSET
SLOT_B_LOAD = SLOT_B_BASE + IMAGE_PAYLOAD_OFFSET

CONFIG_MAGIC = 0x31474643
IMAGE_HEADER_SIZE = 124
SIGNATURE_OFFSET = 56


def build_config_record(active_slot, pending_slot, trial_count, min_version, sequence=0):
    body = struct.pack(
        "<6I",
        CONFIG_MAGIC,
        sequence,
        active_slot,
        pending_slot,
        trial_count,
        min_version,
    )
    crc = zlib.crc32(body) & 0xFFFFFFFF
    return body + struct.pack("<I", crc)


def check_signed(key, signed):
    public_key = load_private_key(key).public_key()
    image_size = struct.unpack("<I", signed[12:16])[0]
    digest = hashlib.sha256(signed[IMAGE_PAYLOAD_OFFSET:]).digest()
    signature = signed[SIGNATURE_OFFSET:SIGNATURE_OFFSET + 64]
    if not verify_digest(public_key, digest, signature):
        raise SystemExit("self-verification failed for generated image")
    if image_size + IMAGE_PAYLOAD_OFFSET != len(signed):
        raise SystemExit("image size mismatch")


def main():
    parser = argparse.ArgumentParser(description="Generate secure boot scenario assets.")
    parser.add_argument("--key", required=True, help="private key PEM")
    parser.add_argument("--app-a", required=True, help="slot A linked application binary")
    parser.add_argument("--app-b", required=True, help="slot B linked application binary")
    parser.add_argument("--unconfirmed", required=True, help="slot B linked non-confirming binary")
    parser.add_argument("--out-dir", required=True, help="output directory")
    args = parser.parse_args()

    out_dir = Path(args.out_dir)
    out_dir.mkdir(parents=True, exist_ok=True)

    key = load_private_key(args.key)
    app_a = Path(args.app_a).read_bytes()
    app_b = Path(args.app_b).read_bytes()
    unconfirmed = Path(args.unconfirmed).read_bytes()

    signed_a = build_signed_image(key, app_a, 1, SLOT_A_LOAD, SLOT_A_LOAD)
    signed_b = build_signed_image(key, app_b, 2, SLOT_B_LOAD, SLOT_B_LOAD)
    signed_unconfirmed = build_signed_image(key, unconfirmed, 2, SLOT_B_LOAD, SLOT_B_LOAD)
    old_b = build_signed_image(key, app_b, 1, SLOT_B_LOAD, SLOT_B_LOAD)

    tampered = bytearray(signed_b)
    tampered[IMAGE_PAYLOAD_OFFSET + 8] ^= 0x01

    assets = {
        "signed_app_a.bin": signed_a,
        "signed_app_b.bin": signed_b,
        "signed_unconfirmed_b.bin": signed_unconfirmed,
        "old_app_b.bin": old_b,
        "tampered_app_b.bin": bytes(tampered),
        "config_pending_b.bin": build_config_record(0, 1, 0, 0),
        "config_pending_b_minver2.bin": build_config_record(0, 1, 0, 2),
    }

    for name, data in assets.items():
        (out_dir / name).write_bytes(data)
        print(f"{name}: {len(data)} bytes")

    for name, signed in (
        ("signed_app_a.bin", signed_a),
        ("signed_app_b.bin", signed_b),
        ("signed_unconfirmed_b.bin", signed_unconfirmed),
        ("old_app_b.bin", old_b),
    ):
        check_signed(args.key, signed)

    print("self-verification of signed scenario images passed")


if __name__ == "__main__":
    main()
