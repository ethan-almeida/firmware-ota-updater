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

STARTUP      := common/startup/startup_stm32f407.s
COMMON_TSRC  := common/src/crc32.c \
                common/src/image.c \
                common/src/config.c \
                common/hal/uart.c \
                common/hal/flash.c \
                common/hal/flash_hw_stm32f4.c \
                common/hal/config_flash_stm32f4.c

UNITY     := tests/host/unity
HOST_CFLAGS := -std=c11 -Wall -Wextra -Werror -g -O0 \
               -Icommon/include -Itests/host -I$(UNITY)
HOST_SRCS := common/src/crc32.c \
             common/src/image.c \
             common/src/config.c \
             common/hal/flash.c \
             $(UNITY)/unity.c \
             tests/host/fakes/flash_model.c \
             tests/host/test_crc32.c \
             tests/host/test_flash.c \
             tests/host/test_image.c \
             tests/host/test_config.c \
             tests/host/test_main.c

.PHONY: all test coverage renode-test clean

all: $(BUILD)/bootloader.elf $(BUILD)/bootloader.bin \
     $(BUILD)/app.elf $(BUILD)/app.bin

test: $(BUILD)/host_tests
	$(BUILD)/host_tests

coverage: | $(BUILD)
	rm -rf $(BUILD)/cov && mkdir -p $(BUILD)/cov
	$(HOSTCC) $(HOST_CFLAGS) --coverage $(HOST_SRCS) -o $(BUILD)/cov/host_tests
	@$(BUILD)/cov/host_tests
	@cd $(BUILD)/cov && gcov -b host_tests-*.gcno 2>/dev/null > coverage.txt && \
	    echo "--- coverage of common/ sources ---" && \
	    grep -E "File '.*common/" -A 1 coverage.txt | grep -v "^--"

renode-test: $(VENV)/bin/robot $(BUILD)/bootloader.elf $(BUILD)/app.elf $(BUILD)/flash_test.elf
	ROBOT=$(ROBOT) BUILD_DIR=$(BUILD) scripts/run_renode_tests.sh

clean:
	rm -rf $(BUILD)

$(BUILD):
	mkdir -p $@

$(BUILD)/bootloader.elf: $(STARTUP) $(COMMON_TSRC) bootloader/src/main.c \
                         bootloader/linker/stm32f407_bootloader.ld | $(BUILD)
	$(CC) $(TCFLAGS) $(INCFLAGS) \
	    -T bootloader/linker/stm32f407_bootloader.ld \
	    -Wl,-Map=$(BUILD)/bootloader.map \
	    $(TLDFLAGS) $(STARTUP) $(COMMON_TSRC) bootloader/src/main.c -o $@
	$(SIZE) $@

$(BUILD)/bootloader.bin: $(BUILD)/bootloader.elf
	$(OBJCOPY) -O binary $< $@

$(BUILD)/app.elf: $(STARTUP) $(COMMON_TSRC) app/src/main.c \
                  app/linker/stm32f407_app.ld | $(BUILD)
	$(CC) $(TCFLAGS) $(INCFLAGS) \
	    -T app/linker/stm32f407_app.ld \
	    -Wl,-Map=$(BUILD)/app.map \
	    $(TLDFLAGS) $(STARTUP) $(COMMON_TSRC) app/src/main.c -o $@
	$(SIZE) $@

$(BUILD)/app.bin: $(BUILD)/app.elf
	$(OBJCOPY) -O binary $< $@

$(BUILD)/flash_test.elf: $(STARTUP) $(COMMON_TSRC) tests/renode/fw/flash_test_main.c \
                         bootloader/linker/stm32f407_bootloader.ld | $(BUILD)
	$(CC) $(TCFLAGS) $(INCFLAGS) \
	    -T bootloader/linker/stm32f407_bootloader.ld \
	    -Wl,-Map=$(BUILD)/flash_test.map \
	    $(TLDFLAGS) $(STARTUP) $(COMMON_TSRC) tests/renode/fw/flash_test_main.c -o $@
	$(SIZE) $@

$(BUILD)/host_tests: $(HOST_SRCS) | $(BUILD)
	$(HOSTCC) $(HOST_CFLAGS) $(HOST_SRCS) -o $@

$(VENV)/bin/robot:
	python3 -m venv $(VENV)
	$(VENV)/bin/pip install --quiet robotframework
