#Makefile for ATmegaBOOT
# E.Lins, 18.7.2005
# $Id$

# program name should not be changed...
PROGRAM    = ATmegaBOOT_644P

# enter the target CPU frequency
AVR_FREQ   = 10000000L

# Number of times the LED flashes
NUM_LED_FLASHES = 4

# Baud rate for UART communication
BAUD_RATE  = 19200

MCU_TARGET = atmega644p
# Get this number from the database
LDSECTION  = --section-start=.text=0xE000	

OBJ        = $(PROGRAM).o
OPTIMIZE   = -Os

DEFS       = '-DNUM_LED_FLASHES=$(NUM_LED_FLASHES)' '-DBAUD_RATE=$(BAUD_RATE)' '-D__AVR_ATmega644A__'
LIBS       = 

CC         = avr-gcc


# Override is only needed by avr-lib build system.

override CFLAGS        = -g -Wall $(OPTIMIZE) -mmcu=$(MCU_TARGET) -DF_CPU=$(AVR_FREQ) $(DEFS)
override LDFLAGS       = -Wl,$(LDSECTION)
#override LDFLAGS       = -Wl,-Map,$(PROGRAM).map,$(LDSECTION)

OBJCOPY        = avr-objcopy
OBJDUMP        = avr-objdump

# all: CFLAGS += '-DMAX_TIME_COUNT=8000000L>>1' -DADABOOT
all: CFLAGS += '-DMAX_TIME_COUNT=F_CPU>>3' -DADABOOT
all: $(PROGRAM).hex

$(PROGRAM).hex: $(PROGRAM).elf
	$(OBJCOPY) -j .text -j .data -O ihex $< $@
	
$(PROGRAM).elf: $(OBJ)
	$(CC) $(CFLAGS) $(LDFLAGS) -o $@ $^ $(LIBS)
	
$(OBJ): ATmegaBOOT.c
	avr-gcc $(CFLAGS) $(LDFLAGS) -c -g -Os -Wall -mmcu=$(MCU_TARGET) ATmegaBOOT.c -o $(PROGRAM).o

%.lst: %.elf
	$(OBJDUMP) -h -S $< > $@

%.srec: %.elf
	$(OBJCOPY) -j .text -j .data -O srec $< $@

%.bin: %.elf
	$(OBJCOPY) -j .text -j .data -O binary $< $@

flash:
	avrdude -c usbtiny -P usb -p atmega644 -U flash:w:ATmegaBOOT_644P.hex:i

erase:
	avrdude -c usbtiny -P usb -p atmega644 -U flash

read:
	avrdude -c usbtiny -P usb -p atmega644 -U flash:r:flash.hex:r

clean:
	rm -rf *.o *.elf *.lst *.map *.sym *.lss *.eep *.srec *.bin *.hex
	
