#!/usr/bin/env python3
import argparse
import hashlib
import struct
import zlib
from pathlib import Path

from cryptography.hazmat.primitives import hashes, serialization
from cryptography.hazmat.primitives.asymmetric import ec, utils

IMAGE_MAGIC = 0x3141544F
IMAGE_HEADER_VERSION = 1
HEADER_STRUCT = struct.Struct("<IIIIII32s64s")
HEADER_CRC_SIZE = 4
IMAGE_HEADER_SIZE = HEADER_STRUCT.size + HEADER_CRC_SIZE
IMAGE_PAYLOAD_OFFSET = 512
HEADER_PAD_BYTE = b"\xFF"
SHA256_LEN = 32
SIGNATURE_LEN = 64


def load_private_key(path):
    pem = Path(path).read_bytes()
    return serialization.load_pem_private_key(pem, password=None)


def sign_digest(private_key, digest):
    der = private_key.sign(
        digest,
        ec.ECDSA(utils.Prehashed(hashes.SHA256()), deterministic_signing=True),
    )
    r, s = utils.decode_dss_signature(der)
    return r.to_bytes(32, "big") + s.to_bytes(32, "big")


def verify_digest(public_key, digest, signature):
    r = int.from_bytes(signature[:32], "big")
    s = int.from_bytes(signature[32:], "big")
    der = utils.encode_dss_signature(r, s)
    try:
        public_key.verify(der, digest, ec.ECDSA(utils.Prehashed(hashes.SHA256())))
        return True
    except Exception:
        return False


def build_signed_image(private_key, payload, version, load_address, entry_address):
    digest = hashlib.sha256(payload).digest()
    signature = sign_digest(private_key, digest)

    body = HEADER_STRUCT.pack(
        IMAGE_MAGIC,
        IMAGE_HEADER_VERSION,
        version,
        len(payload),
        load_address,
        entry_address,
        digest,
        signature,
    )
    crc = zlib.crc32(body) & 0xFFFFFFFF
    header_area = body + struct.pack("<I", crc)
    header_area += HEADER_PAD_BYTE * (IMAGE_PAYLOAD_OFFSET - len(header_area))
    return header_area + payload


def parse_int(value):
    return int(value, 0)


def main():
    parser = argparse.ArgumentParser(description="Sign a firmware image with ECDSA P-256.")
    parser.add_argument("--key", required=True, help="private key PEM")
    parser.add_argument("--in", dest="input", required=True, help="payload binary")
    parser.add_argument("--out", required=True, help="output path for the signed image")
    parser.add_argument("--version", type=parse_int, required=True, help="monotonic firmware version")
    parser.add_argument("--load", type=parse_int, required=True, help="load address")
    parser.add_argument("--entry", type=parse_int, required=True, help="entry address")
    args = parser.parse_args()

    private_key = load_private_key(args.key)
    payload = Path(args.input).read_bytes()

    signed = build_signed_image(private_key, payload, args.version, args.load, args.entry)

    digest = hashlib.sha256(payload).digest()
    signature = signed[HEADER_STRUCT.size - SIGNATURE_LEN:HEADER_STRUCT.size]
    if not verify_digest(private_key.public_key(), digest, signature):
        raise SystemExit("self-verification failed")

    crc_stored = struct.unpack("<I", signed[IMAGE_HEADER_SIZE - 4:IMAGE_HEADER_SIZE])[0]
    if crc_stored != (zlib.crc32(signed[:IMAGE_HEADER_SIZE - 4]) & 0xFFFFFFFF):
        raise SystemExit("header crc mismatch")

    Path(args.out).write_bytes(signed)
    print(f"signed: {args.out} ({len(signed)} bytes, payload {len(payload)} bytes, version {args.version})")


if __name__ == "__main__":
    main()
