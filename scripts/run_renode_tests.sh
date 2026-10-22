#!/usr/bin/env bash

set -u

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
REPO_DIR="$(dirname "$SCRIPT_DIR")"
cd "$REPO_DIR"

ROBOT="${ROBOT:-build/venv/bin/robot}"
BUILD_DIR="${BUILD_DIR:-build}"
PORT="${RENODE_ROBOT_PORT:-9999}"
RESULTS_DIR="$BUILD_DIR/renode-results"

if ! command -v renode >/dev/null 2>&1; then
    echo "ERROR: renode not found in PATH" >&2
    exit 1
fi

if [ ! -x "$ROBOT" ]; then
    echo "ERROR: robot binary not found at $ROBOT (run 'make renode-test' to bootstrap it)" >&2
    exit 1
fi

mkdir -p "$BUILD_DIR"

renode --disable-gui --hide-log --robot-server-port "$PORT" \
    >"$BUILD_DIR/renode-server.log" 2>&1 &
RENODE_PID=$!
trap 'kill "$RENODE_PID" 2>/dev/null; wait "$RENODE_PID" 2>/dev/null' EXIT

ready=0
for _ in $(seq 1 60); do
    if (exec 3<>"/dev/tcp/127.0.0.1/$PORT") 2>/dev/null; then
        exec 3>&- 3<&-
        ready=1
        break
    fi
    if ! kill -0 "$RENODE_PID" 2>/dev/null; then
        echo "ERROR: Renode exited early; log follows:" >&2
        cat "$BUILD_DIR/renode-server.log" >&2
        exit 1
    fi
    sleep 0.5
done

if [ "$ready" -ne 1 ]; then
    echo "ERROR: Renode robot server did not come up on port $PORT" >&2
    exit 1
fi

"$ROBOT" --outputdir "$RESULTS_DIR" \
    --variable RENODE_URL:"http://127.0.0.1:$PORT/" \
    --variable BOOTLOADER_ELF:"$REPO_DIR/$BUILD_DIR/bootloader.elf" \
    --variable APP_ELF:"$REPO_DIR/$BUILD_DIR/app.elf" \
    --variable FLASH_TEST_ELF:"$REPO_DIR/$BUILD_DIR/flash_test.elf" \
    --variable CRYPTO_TEST_ELF:"$REPO_DIR/$BUILD_DIR/crypto_test.elf" \
    tests/renode/
status=$?

if [ "$status" -eq 0 ]; then
    echo ""
    echo "Renode integration tests: ALL PASSED"
else
    echo ""
    echo "Renode integration tests: FAILED (robot exit code $status)" >&2
    echo "See $RESULTS_DIR/log.html" >&2
fi
exit "$status"
