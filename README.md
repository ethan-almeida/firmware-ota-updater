# Secure OTA Firmware Update System for STM32 (Renode-Simulated)

A production-grade Over-the-Air (OTA) firmware update system for the
STM32F407, developed and tested entirely against Renode's
hardware-accurate simulation. The final system provides a secure
bootloader with ECDSA signature verification, A/B flash banking with
automatic rollback, and a UART update transport — all driven by a
host-side TDD test loop plus Renode integration tests.

## Status

| Block | Scope | State |
|---|---|---|
| 1 | Build system, test harness, Renode bring-up | ✅ done |
| 2 | Register layer, flash driver, image format | ✅ done |
| 3 | SHA-256 + ECDSA verify, signing pipeline | ✅ done |
| 4 | Secure boot core: A/B policy, trial boot, rollback | ✅ done |
| 5 | UART OTA transport, fault-injection matrix, docs | ⬜ |

## Layout

```
bootloader/   Stage-0 bootloader (linked at 0x08000000, 64K)
app/          Application image, linked per slot:
                slot A at 0x08010000 (448K), slot B at 0x08080000 (256K)
common/       Shared code: register headers, startup, HAL, pure logic
  include/    Public headers (stm32f407.h, flash.h, image.h, config.h,
              crypto.h, boot.h, image_copy.h, ...)
  hal/        Register-level drivers (flash, uart, STM32 backends)
  src/        Hardware-independent logic (unit-tested on host)
  startup/    Cortex-M4 vector table + reset handler
  third_party/ Vendored micro-ecc (ECDSA P-256 signature verification)
tests/
  host/       Unity unit tests, run on the host via `make test`
    fakes/    Peripheral models used by the host tests
  renode/     Robot Framework integration tests, run via `make renode-test`
    fw/       Test firmware images executed inside Renode
  vectors/    Shared sign/verify artifacts: test key pair, payload, vector cases
scripts/      Tooling: keygen.py, sign_image.py, bin2c.py, vector generator,
              boot scenario generator
```

## Flash map (STM32F407, 1 MB, sector-aligned)

| Region | Address | Sectors | Size |
|---|---|---|---|
| Bootloader | `0x08000000` | 0–3 | 64K |
| Slot A (app) | `0x08010000` | 4–7 | 448K |
| Slot B (OTA staging) | `0x08080000` | 8–9 | 256K |
| Config record A | `0x080C0000` | 10 | 128K |
| Config record B | `0x080E0000` | 11 | 128K |

The two config records live in separate erase sectors so a record can
be rewritten (erase + program) without touching the other one — power
loss at any point leaves the previous record intact.

Each slot holds one signed image laid out as a 512-byte header area
(124-byte `image_header_t` followed by `0xFF` pad) and then the payload
at offset 512. The payload offset is 512-byte aligned so the
application's vector table — and therefore `SCB->VTOR` — lands on a
legally aligned address when booting in place from either slot.

## Secure boot flow

```
Reset → bootloader
  ├─ load config record (newest valid of the two, CRC-checked)
  ├─ validate each slot: header CRC → slot address match → SHA-256 → ECDSA
  ├─ policy decision (pure function, exhaustively unit-tested):
  │    pending update valid & trials left  → boot it (trial++ committed first)
  │    pending invalid / too old / exhausted → clear pending, boot active
  │    no bootable image                  → recovery mode (stay in bootloader)
  ├─ write shared-RAM handshake {magic, slot, trial flag, version}
  └─ set VTOR, switch MSP, jump
app:
  └─ if handshake says "trial boot" → confirm() writes config
     (active = booted slot, pending cleared, min_version raised)
```

The application is linked twice (once per slot) so either slot can be
executed in place; the image header's `load_address`/`entry_address`
must equal the slot base plus `IMAGE_PAYLOAD_OFFSET` (512), which is
where each link script places the vector table. `min_version` provides anti-rollback: a signed image
older than the last confirmed version is refused at boot even though its
signature is valid. Three consecutive unconfirmed boots of a pending
image cause a rollback to the previously active slot.

## Prerequisites

- `arm-none-eabi-gcc` (cross toolchain)
- `gcc`, `make`
- `python3` (venv module) — a project venv with `robotframework` is
  bootstrapped automatically into `build/venv`
- [Renode](https://renode.io) 1.17+ on `PATH`

## Building and testing

```sh
make               # cross-compile bootloader + app images into build/
make test          # host unit test suite (the TDD loop)
make coverage      # host tests under gcov (reports common/ coverage)
make renode-test   # Renode integration tests, headless
make clean
```

## Development approach

Development follows red→green→refactor TDD:

1. Write a failing Unity test for the next piece of logic (`make test`).
2. Implement the minimum to make it green.
3. Refactor with the suite as a safety net.

Renode integration tests are the outer acceptance layer: every block
adds on-target tests (boot flow, flash programming, signature
verification, rollback, fault injection). The Renode flash model's
behaviour is characterised in `tests/renode/flash_spike.robot` so the
flash driver is written against verified, not assumed, semantics.
