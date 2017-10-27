
# Output directors to store intermediate compiled files
# relative to the project directory
BUILD_BASE = build
FW_BASE = firmware
ESPTOOL = esptool.py

# Programing speed.
#BAUD ?= 115200
#BAUD ?= 230400
#BAUD ?= 460800
#BAUD ?= 460800
#BAUD ?= 921600
#BAUD ?= 1500000
BAUD ?= 3000000

# The name for the target project.
TARGET = esp_eb

# The linker script used for the above linkier step.
LD_SCRIPT = eagle.app.v6.ld

# we create two different files for uploading into the flash
# these are the names and options to generate them
FW_1 = 0x00000
FW_2 = 0x10000

# Build flavor: 0 = release, 1 - debug.
FLAVOR ?= 0
DEBUG_ON ?= 0

# The path to USB to serial adapter.
ESPPORT ?= /dev/ttyUSB0

# The Espressif SDK root ditectory path.
SDK_BASE ?= $(HOME)/lib/esp-open-sdk/sdk

# Setup compiler, linker.
CCFLAGS += -Os -ffunction-sections -fno-jump-tables
AR = xtensa-lx106-elf-ar
CC = xtensa-lx106-elf-gcc
LD = xtensa-lx106-elf-gcc
NM = xtensa-lx106-elf-nm
CPP = xtensa-lx106-elf-cpp
OBJCOPY = xtensa-lx106-elf-objcopy
UNAME_S := $(shell uname -s)

# Which modules (subdirectories) of the project to include in compiling.
MODULES = example/lib/esp_tim/esp_tim example/lib/esp_pin/esp_pin esp_eb example

# The extra include directories. Absolute paths.
EXTRA_INCDIR =

# Libraries used in this project, mainly provided by the SDK.
LIBS = c gcc hal phy pp net80211 lwip wpa main ssl

# Compiler flags using during compilation of source files.
CFLAGS = -Os -Wpointer-arith -Wundef -Werror -Wl,-EL -fno-inline-functions -nostdlib -mlongcalls -mtext-section-literals  -D__ets__ -DICACHE_FLASH

# Linker flags used to generate the main object file.
LDFLAGS = -nostdlib -Wl,--no-check-sections -u call_user_start -Wl,-static

ifeq ($(FLAVOR),1)
    CFLAGS += -O0
    LDFLAGS += -O0
endif

ifeq ($(FLAVOR),0)
    CFLAGS += -O2
    LDFLAGS += -O2
endif

# Various paths from the SDK used in this project.
SDK_LIBDIR	= lib
SDK_LDDIR	= ld
SDK_INCDIR	= include include/json

##############################################
#### No user configurable options below here.
##############################################
FW_TOOL ?= $(ESPTOOL)
SRC_DIR := $(MODULES)
BUILD_DIR := $(addprefix $(BUILD_BASE)/,$(MODULES))

SDK_LIBDIR := $(addprefix $(SDK_BASE)/,$(SDK_LIBDIR))
SDK_INCDIR := $(addprefix -I$(SDK_BASE)/,$(SDK_INCDIR))
SRC := $(foreach sdir,$(SRC_DIR),$(wildcard $(sdir)/*.c))
OBJ := $(patsubst %.c,$(BUILD_BASE)/%.o,$(SRC))

LIBS := $(addprefix -l,$(LIBS))
APP_AR := $(addprefix $(BUILD_BASE)/,$(TARGET)_app.a)
TARGET_OUT := $(addprefix $(BUILD_BASE)/,$(TARGET).out)

LD_SCRIPT := $(addprefix -T$(SDK_BASE)/$(SDK_LDDIR)/,$(LD_SCRIPT))

INCDIR := $(addprefix -I,$(SRC_DIR))
EXTRA_INCDIR := $(addprefix -I,$(EXTRA_INCDIR))
MODULE_INCDIR := $(addsuffix /include,$(INCDIR))

FW_FILE_1 := $(addprefix $(FW_BASE)/,$(FW_1).bin)
FW_FILE_2 := $(addprefix $(FW_BASE)/,$(FW_2).bin)

V ?= $(VERBOSE)
ifeq ("$(V)","1")
Q :=
vecho := @true
else
Q := @
vecho := @echo
endif

vpath %.c $(SRC_DIR)

define compile-objects
$1/%.o: %.c
	$(vecho) "CC $$<"
	$(Q) $(CC) $(INCDIR) $(MODULE_INCDIR) $(EXTRA_INCDIR) $(SDK_INCDIR) -DDEBUG_ON=$(DEBUG_ON) $(CFLAGS) -c $$< -o $$@
endef

.PHONY: all checkdirs clean

all: checkdirs $(TARGET_OUT) $(FW_FILE_1) $(FW_FILE_2)

$(FW_FILE_1): $(TARGET_OUT)
	$(vecho) "FW $@"
	$(ESPTOOL) elf2image $< -o $(FW_BASE)/
	
$(FW_FILE_2): $(TARGET_OUT)
	$(vecho) "FW $@"
	$(ESPTOOL) elf2image $< -o $(FW_BASE)/

$(TARGET_OUT): $(APP_AR)
	$(vecho) "LD $@"
	$(Q) $(LD) -L$(SDK_LIBDIR) $(LD_SCRIPT) $(LDFLAGS) -Wl,--start-group $(LIBS) $(APP_AR) -Wl,--end-group -o $@

$(APP_AR): $(OBJ)
	$(vecho) "AR $@"
	$(Q) $(AR) cru $@ $^

checkdirs: $(BUILD_DIR) $(FW_BASE)

$(BUILD_DIR):
	$(Q) mkdir -p $@

firmware:
	$(Q) mkdir -p $@

flash: $(FW_FILE_1)  $(FW_FILE_2)
	$(ESPTOOL) -p $(ESPPORT) -b $(BAUD) write_flash $(FW_1) $(FW_FILE_1) $(FW_2) $(FW_FILE_2)

test: flash
	screen $(ESPPORT) 115200

rebuild: clean all

clean:
	$(Q) rm -f $(APP_AR)
	$(Q) rm -f $(TARGET_OUT)
	$(Q) rm -rf $(BUILD_DIR)
	$(Q) rm -rf $(BUILD_BASE)
	$(Q) rm -f $(FW_FILE_1)
	$(Q) rm -f $(FW_FILE_2)
	$(Q) rm -rf $(FW_BASE)

$(foreach bdir,$(BUILD_DIR),$(eval $(call compile-objects,$(bdir))))