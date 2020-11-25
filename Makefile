PWD = $(shell pwd)

# debug build?
DEBUG = 1
# optimization
OPT = -O3

COPY_TO_ARM=./copy.sh

PREFIX=arm-none-eabi-

ifdef GCC_PATH
CC = $(GCC_PATH)/$(PREFIX)gcc
AS = $(GCC_PATH)/$(PREFIX)gcc -x assembler-with-cpp
CP = $(GCC_PATH)/$(PREFIX)objcopy
SZ = $(GCC_PATH)/$(PREFIX)size
else
CC = $(PREFIX)gcc
AS = $(PREFIX)gcc -x assembler-with-cpp
CP = $(PREFIX)objcopy
SZ = $(PREFIX)size
endif
HEX = $(CP) -O ihex
BIN = $(CP) -O binary -S

BUILD_DIR = $(PWD)/app
SOURCE_DIR = $(PWD)

BUILD_LOG = $(BUILD_DIR)/log
ERR_LOG = $(BUILD_DIR)/err

# Project name
TARGET = Four_Channels
OUTPATH = $(PWD)/app
PROJ_PATH = $(PWD)

ASM_SOURCES =  \
startup_stm32f405xx.s

SFILES = $(ASM_SOURCES)

COBJS += $(CFILES:%.c=$(BUILD_DIR)/%.o)
CDEPS += $(CFILES:%.c=$(BUILD_DIR)/%.d)
SOBJS += $(SFILES:%.s=$(BUILD_DIR)/%.o)

# cpu
CPU = -mcpu=cortex-m4

# fpu
FPU = -mfpu=fpv4-sp-d16

# float-abi
FLOAT-ABI = -mfloat-abi=hard

# mcu
MCU = $(CPU) -mthumb $(FPU) $(FLOAT-ABI)

# macros for gcc
# AS defines
AS_DEFS = 

# C defines
C_DEFS =  \
-DUSE_HAL_DRIVER \
-DSTM32F405xx

ASFLAGS = $(MCU) $(AS_DEFS) $(OPT) -Wall -fdata-sections -ffunction-sections
CFLAGS = $(MCU) $(C_DEFS) $(OPT) -Wall -fdata-sections -ffunction-sections -std=gnu99

ifeq ($(DEBUG), 1)
CFLAGS += -g -gdwarf-2
endif

include $(PROJ_PATH)/project/module.mk
include $(PROJ_PATH)/Drivers/module.mk

# link script
LDSCRIPT = STM32F405RGTx_FLASH.ld

# libraries
LIBS = -lc -lm -lnosys 
LIBDIR = 
LDFLAGS = $(MCU) -specs=nano.specs -T$(LDSCRIPT) $(LIBDIR) $(LIBS) -Wl,-Map=$(BUILD_DIR)/$(TARGET).map,--cref -Wl,--gc-sections


.PHONY: clean

# default action: build all
all: $(BUILD_DIR)/$(TARGET).elf $(BUILD_DIR)/$(TARGET).hex $(BUILD_DIR)/$(TARGET).bin
	@mkdir -p $(BUILD_DIR)
	@echo Done

$(BUILD_DIR)/$(TARGET).elf: $(COBJS) $(SOBJS) Makefile
	@echo Linking...
	@if [ -e "$@" ]; then rm -f "$@"; fi
	$(CC) $(SOBJS) $(COBJS) $(LDFLAGS) -o $@
	$(SZ) $@

$(BUILD_DIR)/%.hex: $(BUILD_DIR)/%.elf | $(BUILD_DIR)
	$(HEX) $< $@
	
$(BUILD_DIR)/%.bin: $(BUILD_DIR)/%.elf | $(BUILD_DIR)
	$(BIN) $< $@	

include .rule.mk
clean:
	rm -rf $(BUILD_DIR)

project_clen:
	rm -rf $(BUILD_DIR)/Project
flash:
	st-flash --reset write app/Four_Channels.bin 0x8000000||exit 0
	st-flash --reset write app/Four_Channels.bin 0x8000000||exit 1
	@echo Flash done.

#JLinkExe -device STM32F405RG -if SWD -speed 16000 jlinkbat
