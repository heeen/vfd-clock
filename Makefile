SDK_BASE   ?= ../esp-open-sdk/sdk
SDK_LIBDIR  = lib
SDK_INCDIR  = include

RBOOTBASE    ?= raburton-esp8266
ESPTOOL2     ?= $(RBOOTBASE)/esptool2/esptool2
RBOOT        ?= $(RBOOTBASE)/rboot/firmware/rboot.bin
RBOOT_INCDIR ?= $(RBOOTBASE)/rboot

FW_SECTS      = .text .data .rodata
FW_USER_ARGS  = -quiet -bin -boot2

XTENSA_BINDIR ?= $(addprefix $(PWD)/,"../esp-open-sdk/xtensa-lx106-elf/bin")

ESPTOOL ?= ./esptool.py
ESPPORT ?= /dev/ttyUSB1
ESPBAUD ?= 115200

CC := $(addprefix $(XTENSA_BINDIR)/,xtensa-lx106-elf-gcc)
LD := $(addprefix $(XTENSA_BINDIR)/,xtensa-lx106-elf-gcc)

BUILD_DIR = build
FIRMW_DIR = firmware
SRC_DIR   = user

SDK_LIBDIR := $(addprefix $(SDK_BASE)/,$(SDK_LIBDIR))
SDK_INCDIR := $(addprefix -I$(SDK_BASE)/,$(SDK_INCDIR))

LIBS    = c gcc hal phy net80211 lwip wpa main pp
CFLAGS  = -Os -g -O2 -Wpointer-arith -Wundef -Werror -Wl,-EL -fno-inline-functions -nostdlib -mlongcalls -mtext-section-literals  -D__ets__ -DICACHE_FLASH
LDFLAGS = -nostdlib -Wl,--no-check-sections -u call_user_start -Wl,-static

LIBS		:= $(addprefix -l,$(LIBS))

.SECONDARY:
.PHONY: all clean

C_FILES = $(wildcard $(SRC_DIR)/*.c)
O_FILES = $(patsubst $(SRC_DIR)/%.c,$(BUILD_DIR)/%.o,$(C_FILES))

all: $(BUILD_DIR) $(FIRMW_DIR) $(FIRMW_DIR)/rom0.bin $(FIRMW_DIR)/rom1.bin rboot esptool2

$(BUILD_DIR)/%.o: $(SRC_DIR)/%.c
	@echo "CC $<"
	@$(CC) -I$(SRC_DIR) $(SDK_INCDIR) -I$(RBOOT_INCDIR) $(CFLAGS) -o $@ -c $<

$(BUILD_DIR)/%.elf: $(O_FILES)
	@echo "LD $(notdir $@)"
	@$(LD) -L$(SDK_LIBDIR) -T$(notdir $(basename $@)).ld $(LDFLAGS) -Wl,--start-group $(LIBS) $^ -Wl,--end-group -o $@

$(FIRMW_DIR)/%.bin: $(BUILD_DIR)/%.elf
	@echo "FW $(notdir $@)"
	@$(ESPTOOL2) $(FW_USER_ARGS) $^ $@ $(FW_SECTS)

$(BUILD_DIR):
	@mkdir -p $@

$(FIRMW_DIR):
	@mkdir -p $@

clean:
	@echo "RM $(BUILD_DIR) $(FIRMW_DIR)"
	@rm -rf $(BUILD_DIR)
	@rm -rf $(FIRMW_DIR)

$(ESPTOOL2):
	make -C $(RBOOTBASE)/esptool2

$(RBOOT): $(ESPTOOL2)
	make -C $(RBOOTBASE)/rboot

rboot: $(RBOOT)

esptool2: $(ESPTOOL2)

flash: $(RBOOT) $(FIRMW_DIR)/rom0.bin $(FIRMW_DIR)/rom1.bin
	$(ESPTOOL) --port $(ESPPORT) --baud $(ESPBAUD) write_flash 0x00000 $(RBOOT) 0x02000 $(FIRMW_DIR)/rom0.bin 0x82000 $(FIRMW_DIR)/rom1.bin
