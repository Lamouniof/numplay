# NumTycoon: business management / city tycoon for NumWorks.
Q ?= @
CC = arm-none-eabi-gcc
NWLINK ?= npx --yes -- nwlink@1.0.0
BUILD_DIR = output
STRIP ?= arm-none-eabi-strip --strip-unneeded
GAME = src/main.c
HEADERS = $(wildcard src/*.h) ../common/epsilon_app.h ../common/epsilon_files.h Makefile
CFLAGS = -std=gnu11 $(shell $(NWLINK) eadk-cflags-device) -O2 -Wall -Wextra -Wno-unused-parameter -fno-math-errno -ffast-math -fno-tree-loop-distribute-patterns -flto -fno-fat-lto-objects -fwhole-program -fvisibility=internal -ffunction-sections -fdata-sections
LDFLAGS = -Wl,--relocatable -nostartfiles --specs=nano.specs -Wl,-e,main -Wl,-u,eadk_app_name -Wl,-u,eadk_app_icon -Wl,-u,eadk_api_level -Wl,--gc-sections -flinker-output=nolto-rel
.PHONY: build check clean module sim-module
build: $(BUILD_DIR)/numtycoon.nwa
check: $(BUILD_DIR)/numtycoon.nwa
	$(Q) $(NWLINK) nwa-bin --ram-length 153676 $< $(BUILD_DIR)/numtycoon.bin
	@echo "BIN $$(wc -c < $(BUILD_DIR)/numtycoon.bin) bytes"
$(BUILD_DIR):
	$(Q) mkdir -p $@
$(BUILD_DIR)/icon.o: src/icon.png | $(BUILD_DIR)
	$(Q) $(NWLINK) png-icon-o $< $@
$(BUILD_DIR)/numtycoon.nwa: $(GAME) $(HEADERS) $(BUILD_DIR)/icon.o
	$(Q) $(CC) $(CFLAGS) $(LDFLAGS) $(GAME) $(BUILD_DIR)/icon.o -lm -o $@
	$(Q) $(STRIP) $@
module: $(BUILD_DIR)/module.o
$(BUILD_DIR)/module.o: $(GAME) $(HEADERS) | $(BUILD_DIR)
	$(Q) $(CC) $(CFLAGS) -Wl,--relocatable -nostartfiles -nostdlib -Wl,-e,main -flinker-output=nolto-rel $(GAME) -o $@
sim-module: $(BUILD_DIR)/sim-module.o
$(BUILD_DIR)/sim-module.o: $(GAME) $(HEADERS) | $(BUILD_DIR)
	$(Q) mkdir -p $(BUILD_DIR)/sim
	$(Q) for f in $(basename $(notdir $(GAME))); do cc -std=gnu11 -O2 -fPIC $(shell $(NWLINK) eadk-cflags-simulator) -Dmain=np_numtycoon_main -c src/$$f.c -o $(BUILD_DIR)/sim/$$f.o || exit 1; done
	$(Q) ld -r $(BUILD_DIR)/sim/*.o -exported_symbol _np_numtycoon_main -o $@
clean:
	$(Q) rm -rf $(BUILD_DIR)
