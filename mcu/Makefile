#################################################################################
## Makefile for microcontroller firmware. Adapted from the avr-project template.
## Authors: David & Dylan Kirkby / Jan 2014.
##
## Compiles sources into firmware.hex:
## % make
##
## Burns firmware.hex into flash:
## % make flash
##
## Writes assembly for main.c to build/main.s:
## % make build/main.s
##
#################################################################################

# The target device for cross compiling
DEVICE     = atmega644a
# The target device for uploads via avrdude
# Normally this would be the same as DEVICE, but the 644a is a special case
# since avrdude does not recognize it, so we instead pick a device with the
# same hardware signature but different avr-gcc attributes.
UPLOAD_DEVICE = atmega644

# The target device's clock speed in Hz
CLOCK      = 10000000

# Select programmer from the following list
# usbtiny.........Program an AVR directly via its ISP port using a USBTinyISP
# arduino.........Program an AVR with an Arduino acting as an ISP
# bootloader......Program with a bootloader
PROGRAMMER = bootloader

# Select serial port format
# ubuntu ........./dev/ttyUSB*
# OS X .........../dev/tty.usbserial*
PORT = /dev/ttyUSB*

# Thanks engbedded AVR fuse calculator!
FUSES = -U lfuse:w:0xe0:m -U hfuse:w:0xd0:m -U efuse:w:0xfc:m

ifeq ($(PROGRAMMER),usbtiny)
	PROGRAMMER_ARGS = -c usbtiny -P usb
else ifeq ($(PROGRAMMER),arduino)
	PROGRAMMER_ARGS = -c stk500v1 -P $(PORT) -b 19200
else ifeq ($(PROGRAMMER),bootloader)
	PROGRAMMER_ARGS = -c arduino -P $(PORT) -b 19200
else
	PROGRAMMER_ARGS = -c usbtiny -P usb
endif

# run cat ../IRLength.txt to capture the value for CIRCULAR_BUFFER_LENGTH
IRLENGTH := $(shell cat ../IRLength.txt)

AVRDUDE = avrdude $(PROGRAMMER_ARGS) -p $(UPLOAD_DEVICE)
COMPILE = avr-gcc -Wall -Os -std=c99 -DF_CPU=$(CLOCK) -DCIRCULAR_BUFFER_LENGTH=$(IRLENGTH) -mmcu=$(DEVICE)

SOURCES = main.c
OBJECTS = $(addprefix build/, $(SOURCES:%.c=%.o))
ASSEMS = $(addprefix build/, $(SOURCES:%.c=%.s))
TARGET = build/firmware

# run the commitInfo.py script to capture our git status
COMMIT_INFO := $(shell ./commitInfo.py)

# symbolic targets:
all: $(TARGET).hex

# source dependencies
build/main.o build/main.s : main.c LED.h IR.h UART.h TWI.h BMP180.h packet.h

# rule for compiling .c into a corresponding .o under build/
$(OBJECTS) : build/%.o : %.c
	$(COMPILE) -c $< -o $@ -DCOMMIT_INFO="$(COMMIT_INFO)"

# rule for compiling .c into a corresponding .s under build/
$(ASSEMS) : build/%.s : %.c
	$(COMPILE) -S $< -o $@

flash:	all
	$(AVRDUDE) -U flash:w:$(TARGET).hex:i

# Only use with usbtiny
fuse:
	$(AVRDUDE) $(FUSES)

# Only use with usbtiny
serialNumber:
	$(AVRDUDE) -U eeprom:w:config.srec:s

# if you use a bootloader, change the command below appropriately:
load: all
	bootloadHID $(TARGET).hex

# file targets:
$(TARGET).elf: $(OBJECTS)
	$(COMPILE) -o $(TARGET).elf $(OBJECTS)

$(TARGET).hex: $(TARGET).elf
	rm -f $(TARGET).hex
	avr-objcopy -j .text -j .data -O ihex $(TARGET).elf $(TARGET).hex
	avr-size --format=avr --mcu=$(DEVICE) $(TARGET).elf
# If you have an EEPROM section, you must also create a hex file for the
# EEPROM and add it to the "flash" target.

# Targets for code debugging and analysis:
disasm:	$(TARGET).elf
	avr-objdump -d $(TARGET).elf

cpp:
	$(COMPILE) -E $(TARGET).c

clean:
	rm -rf build/*.o build/*.elf build/*.lst build/*.map build/*.sym build/*.lss build/*.eep build/*.srec build/*.bin build/*.hex
