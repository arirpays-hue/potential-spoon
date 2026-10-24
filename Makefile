# SPDX-License-Identifier: CC0-1.0
TARGET		:= jumper_ds_27worlds
BUILD		:= build
SOURCES		:= source
INCLUDES	:= source

ifeq ($(strip $(DEVKITPRO)),)
$(error "Please set DEVKITPRO in your environment. export DEVKITPRO=<path>/devkitpro")
endif

include $(DEVKITPRO)/devkitARM/base_rules

ARCH	:= -mthumb -march=armv5te -mtune=arm946e-s -ffast-math
CFLAGS	:= -Wall -O2 $(ARCH) -I$(CURDIR)/$(INCLUDES)
LDFLAGS	:= -specs=ds_arm9.specs $(ARCH)
LIBS	:= -lnds9

CFILES	:= $(foreach dir,$(SOURCES),$(notdir $(wildcard $(dir)/*.c)))
OFILES	:= $(CFILES:.c=.o)

export VPATH := $(foreach dir,$(SOURCES),$(CURDIR)/$(dir))

.PHONY: all clean

all: $(TARGET).nds

$(TARGET).nds: $(TARGET).elf
	@echo "Creating NDS ROM..."
	@ndstool -c $(TARGET).nds -9 $(TARGET).elf

$(TARGET).elf: $(OFILES)
	@echo "Linking $(TARGET).elf..."
	@$(CC) $(LDFLAGS) $(OFILES) $(LIBS) -o $@

%.o: %.c
	@echo Compiling $<
	@$(CC) $(CFLAGS) -c $< -o $@

clean:
	@rm -fr $(BUILD) *.o *.elf *.nds
