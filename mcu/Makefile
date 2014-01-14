#################################################################################
## Makefile for microcontroller firmware. Adapted from the avr-project template.
## Authors: David & Dylan Kirkby / Jan 2014.
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

TARGET = firmware
OBJECTS = main.o

# symbolic targets:
all: $(TARGET).hex

.c.o:
	$(COMPILE) -c $< -o $@

.S.o:
	$(COMPILE) -x assembler-with-cpp -c $< -o $@
# "-x assembler-with-cpp" should not be necessary since this is the default
# file type for the .S (with capital S) extension. However, upper case
# characters are not always preserved on Windows. To ensure WinAVR
# compatibility define the file type manually.

.c.s:
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
