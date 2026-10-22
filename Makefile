CROSS   ?= arm-none-eabi-
CC      := $(CROSS)gcc
OBJCOPY := $(CROSS)objcopy
SIZE    := $(CROSS)size
HOSTCC  ?= gcc

BUILD   := build
VENV    := $(BUILD)/venv
ROBOT   := $(VENV)/bin/robot
MCUFLAGS := -mcpu=cortex-m4 -mthumb -mfloat-abi=soft
TCFLAGS  := $(MCUFLAGS) -std=c11 -Os -g3 -Wall -Wextra -Werror \
            -ffunction-sections -fdata-sections -fno-common
TLDFLAGS := $(MCUFLAGS) -nostartfiles -Wl,--gc-sections \
            -Wl,--print-memory-usage
INCFLAGS := -Icommon/include
UECC_DIR := common/third_party/micro-ecc
UECC_FLAGS := -Wno-unused-parameter
INCFLAGS += -I$(UECC_DIR)

STARTUP      := common/startup/startup_stm32f407.s
COMMON_TSRC  := common/src/crc32.c \
                common/src/image.c \
                common/src/config.c \
                common/src/sha256.c \
                common/src/crypto.c \
                common/hal/uart.c \
                common/hal/flash.c \
                common/hal/flash_hw_stm32f4.c \
                common/hal/config_flash_stm32f4.c

UNITY     := tests/host/unity
HOST_CFLAGS := -std=c11 -Wall -Wextra -Werror -g -O0 \
               -Icommon/include -Itests/host -I$(UNITY) -I$(UECC_DIR)
HOST_SRCS := common/src/crc32.c \
             common/src/image.c \
             common/src/config.c \
             common/src/sha256.c \
             common/src/crypto.c \
             common/hal/flash.c \
             $(UNITY)/unity.c \
             tests/host/fakes/flash_model.c \
             tests/host/test_crc32.c \
             tests/host/test_flash.c \
             tests/host/test_image.c \
             tests/host/test_config.c \
             tests/host/test_sha256.c \
             tests/host/test_crypto.c \
             tests/host/test_main.c

.PHONY: all test coverage renode-test clean

all: $(BUILD)/bootloader.elf $(BUILD)/bootloader.bin \
     $(BUILD)/app.elf $(BUILD)/app.bin

test: $(BUILD)/host_tests
	$(BUILD)/host_tests

coverage: $(BUILD)/uECC_host.o | $(BUILD)
	rm -rf $(BUILD)/cov && mkdir -p $(BUILD)/cov
	$(HOSTCC) $(HOST_CFLAGS) --coverage $(HOST_SRCS) $(BUILD)/uECC_host.o -o $(BUILD)/cov/host_tests
	@$(BUILD)/cov/host_tests
	@cd $(BUILD)/cov && gcov -b host_tests-*.gcno 2>/dev/null > coverage.txt && \
	    echo "--- coverage of common/ sources ---" && \
	    grep -E "File '.*common/" -A 1 coverage.txt | grep -v "^--"

renode-test: $(VENV)/bin/robot $(BUILD)/bootloader.elf $(BUILD)/app.elf \
             $(BUILD)/flash_test.elf $(BUILD)/crypto_test.elf
	ROBOT=$(ROBOT) BUILD_DIR=$(BUILD) scripts/run_renode_tests.sh

clean:
	rm -rf $(BUILD)

$(BUILD):
	mkdir -p $@

$(BUILD)/bootloader.elf: $(STARTUP) $(COMMON_TSRC) $(BUILD)/uECC.o bootloader/src/main.c \
                         bootloader/linker/stm32f407_bootloader.ld | $(BUILD)
	$(CC) $(TCFLAGS) $(INCFLAGS) \
	    -T bootloader/linker/stm32f407_bootloader.ld \
	    -Wl,-Map=$(BUILD)/bootloader.map \
	    $(TLDFLAGS) $(STARTUP) $(COMMON_TSRC) $(BUILD)/uECC.o bootloader/src/main.c -o $@
	$(SIZE) $@

$(BUILD)/bootloader.bin: $(BUILD)/bootloader.elf
	$(OBJCOPY) -O binary $< $@

$(BUILD)/app.elf: $(STARTUP) $(COMMON_TSRC) $(BUILD)/uECC.o app/src/main.c \
                  app/linker/stm32f407_app.ld | $(BUILD)
	$(CC) $(TCFLAGS) $(INCFLAGS) \
	    -T app/linker/stm32f407_app.ld \
	    -Wl,-Map=$(BUILD)/app.map \
	    $(TLDFLAGS) $(STARTUP) $(COMMON_TSRC) $(BUILD)/uECC.o app/src/main.c -o $@
	$(SIZE) $@

$(BUILD)/app.bin: $(BUILD)/app.elf
	$(OBJCOPY) -O binary $< $@

$(BUILD)/flash_test.elf: $(STARTUP) $(COMMON_TSRC) $(BUILD)/uECC.o tests/renode/fw/flash_test_main.c \
                         bootloader/linker/stm32f407_bootloader.ld | $(BUILD)
	$(CC) $(TCFLAGS) $(INCFLAGS) \
	    -T bootloader/linker/stm32f407_bootloader.ld \
	    -Wl,-Map=$(BUILD)/flash_test.map \
	    $(TLDFLAGS) $(STARTUP) $(COMMON_TSRC) $(BUILD)/uECC.o tests/renode/fw/flash_test_main.c -o $@
	$(SIZE) $@

GEN_DIR := $(BUILD)/gen

$(GEN_DIR):
	mkdir -p $@

$(GEN_DIR)/signed_test_image.bin: scripts/sign_image.py tests/vectors/test_private.pem \
                                  tests/vectors/payload.bin | $(GEN_DIR)
	python3 scripts/sign_image.py --key tests/vectors/test_private.pem \
	    --in tests/vectors/payload.bin --out $@ \
	    --version 1 --load 0x08010000 --entry 0x08010000

$(GEN_DIR)/test_image_blob.c: $(GEN_DIR)/signed_test_image.bin scripts/bin2c.py | $(GEN_DIR)
	python3 scripts/bin2c.py --in $< --out $@ --symbol test_signed_image

$(GEN_DIR)/test_pubkey_blob.c: tests/vectors/test_public_key.bin scripts/bin2c.py | $(GEN_DIR)
	python3 scripts/bin2c.py --in $< --out $@ --symbol test_public_key

$(BUILD)/crypto_test.elf: $(STARTUP) $(COMMON_TSRC) $(BUILD)/uECC.o \
                          tests/renode/fw/crypto_test_main.c \
                          $(GEN_DIR)/test_image_blob.c $(GEN_DIR)/test_pubkey_blob.c \
                          bootloader/linker/stm32f407_bootloader.ld | $(BUILD)
	$(CC) $(TCFLAGS) $(INCFLAGS) \
	    -T bootloader/linker/stm32f407_bootloader.ld \
	    -Wl,-Map=$(BUILD)/crypto_test.map \
	    $(TLDFLAGS) $(STARTUP) $(COMMON_TSRC) $(BUILD)/uECC.o \
	    tests/renode/fw/crypto_test_main.c \
	    $(GEN_DIR)/test_image_blob.c $(GEN_DIR)/test_pubkey_blob.c -o $@
	$(SIZE) $@

$(BUILD)/host_tests: $(HOST_SRCS) $(BUILD)/uECC_host.o | $(BUILD)
	$(HOSTCC) $(HOST_CFLAGS) $(HOST_SRCS) $(BUILD)/uECC_host.o -o $@

$(BUILD)/uECC_host.o: $(UECC_DIR)/uECC.c | $(BUILD)
	$(HOSTCC) $(HOST_CFLAGS) $(UECC_FLAGS) -I$(UECC_DIR) -c $< -o $@

$(BUILD)/uECC.o: $(UECC_DIR)/uECC.c | $(BUILD)
	$(CC) $(TCFLAGS) $(UECC_FLAGS) -I$(UECC_DIR) -c $< -o $@

$(VENV)/bin/robot:
	python3 -m venv $(VENV)
	$(VENV)/bin/pip install --quiet robotframework
