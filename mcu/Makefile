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

PROGRAMMER = -c usbtiny -P usb
##FUSES      = -U hfuse:w:0xd9:m -U lfuse:w:0x24:m

AVRDUDE = avrdude $(PROGRAMMER) -p $(UPLOAD_DEVICE)
COMPILE = avr-gcc -Wall -Os -DF_CPU=$(CLOCK) -mmcu=$(DEVICE)

SOURCES = main.c
OBJECTS = $(addprefix build/, $(SOURCES:%.c=%.o))
ASSEMS = $(addprefix build/, $(SOURCES:%.c=%.s))
TARGET = build/firmware

# symbolic targets:
all: $(TARGET).hex

# source dependencies
build/main.o build/main.s : main.c LED.h UART.h

# rule for compiling .c into a corresponding .o under build/
$(OBJECTS) : build/%.o : %.c
	$(COMPILE) -c $< -o $@

# rule for compiling .c into a corresponding .s under build/
$(ASSEMS) : build/%.s : %.c
	$(COMPILE) -S $< -o $@

flash:	all
	$(AVRDUDE) -U flash:w:$(TARGET).hex:i

#fuse:
#	$(AVRDUDE) $(FUSES)

# if you use a bootloader, change the command below appropriately:
load: all
	bootloadHID $(TARGET).hex

clean:
	rm -f $(TARGET).hex $(TARGET).elf $(OBJECTS)

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
