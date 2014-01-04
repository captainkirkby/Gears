# ==========================================================================
# Makefile for @home MCU code
#
# David Kirkby, University of California, Irvine, dkirkby@uci.edu
# ==========================================================================

# --------------------------------------------------------------------------
# The name of the top-level program to build:
# Select 'leaf' for the plug-in devices
# Select 'hub' for the usb-nordic interface embedded within a hub
# --------------------------------------------------------------------------
TARGET = device

# --------------------------------------------------------------------------
# The MCU programming method to use:
# Select 'usb' to program an Arduino via its usb-serial bootloader
# Select 'spi' to program an AVR directly via its ISP port using an avrisp2
# Select 'tiny' to program an AVR directly via its ISP port using a usbtiny
# --------------------------------------------------------------------------
METHOD = tiny

# --------------------------------------------------------------------------
# The MCU architecture to build for
# Select '644' for the ATMega644a device in a custom TickTock circuit
# Select '328' for the ATMega328p device in an Arduino or equivalent
# --------------------------------------------------------------------------
DEVICE = 644

# --------------------------------------------------------------------------
# The location of the Arduino installation to use
# --------------------------------------------------------------------------
INSTALL_DIR = /Applications/Arduino.app/Contents/Resources/Java

# ==========================================================================
# Arduino library configuration
# ==========================================================================

# --------------------------------------------------------------------------
# The location of the arduino core library sources
# --------------------------------------------------------------------------
ARDUINO = $(INSTALL_DIR)/hardware/arduino/cores/arduino

# --------------------------------------------------------------------------
# The core C sources to use
# --------------------------------------------------------------------------
CORE_C_SRC = WInterrupts.c wiring.c wiring_analog.c \
wiring_digital.c wiring_pulse.c wiring_shift.c
		
# --------------------------------------------------------------------------
# The core C++ sources to use
# --------------------------------------------------------------------------
CORE_CPP_SRC = CDC.cpp HardwareSerial.cpp HID.cpp IPAddress.cpp new.cpp \
Print.cpp Stream.cpp Tone.cpp USBCore.cpp WMath.cpp WString.cpp

# --------------------------------------------------------------------------
# The local C sources to use
# --------------------------------------------------------------------------
LOCAL_C_SRC = twi.c

# --------------------------------------------------------------------------
# The local C++ sources to use
# --------------------------------------------------------------------------
LOCAL_CPP_SRC = Adafruit_BMP085_U.cpp Wire.cpp

# ==========================================================================
# Target device configuration
# Fuses set using http://www.engbedded.com/fusecalc
# ==========================================================================

VARIANT = standard

ifeq ($(DEVICE),644)
		# Target device is a TickTock custom circuit
		MCU = atmega644
		BUILD_DIR = build.644
		# LOW = SUT0+CKSEK3+CKSEL2+CKSEL1+CKSEL0
		# HIGH = SPIEN+EESAVE+BOOTSZ1+BOOTSZ0+BOOTRST
		# EXTENDED = (none set)
		AVR_FUSES = -u -U lfuse:w:0xe0:m -U hfuse:w:0xd0:m -U efuse:w:0xff:m
		# board is clocked at 10MHz
		F_CPU = 10000000
		# use our own mapping of device pins to Arduino pins
		PINS_INC_DIR = include.644
else ifeq ($(DEVICE),328)
		# Target device is an Arduino
		MCU = atmega328p
		BUILD_DIR = build.328
		# LOW = CKSEL3
		# HIGH = SPIEN+EESAVE+BOOTSZ1+BOOTSZ0+BOOTRST
		# EXTENDED = (none set)
		AVR_FUSES = -u -U lfuse:w:0xf7:m -U hfuse:w:0xd0:m -U efuse:w:0xff:m
		# board is clocked at 16MHz
		F_CPU = 16000000
		# use the standard mapping of device pins to Arduino pins
		PINS_INC_DIR = $(INSTALL_DIR)/hardware/arduino/variants/standard
endif

# ==========================================================================
# Programmer configuration
# ==========================================================================

ifeq ($(METHOD),spi)
		AVRDUDE_PORT = usb
		AVRDUDE_PROGRAMMER = avrisp2
		AVRDUDE_BAUDRATE =
else ifeq ($(METHOD),tiny)
		AVRDUDE_PORT = usb
		AVRDUDE_PROGRAMMER = usbtiny
		AVRDUDE_BAUDRATE =
else
		AVRDUDE_PORT = /dev/tty.usbserial*
		AVRDUDE_PROGRAMMER = stk500v1
		AVRDUDE_BAUDRATE = -b 28800
endif

AVRDUDE_WRITE_FLASH = -U flash:w:$(BUILD_DIR)/$(TARGET).hex

CONFIG_FILE = config.dat
AVRDUDE_WRITE_EEPROM = -U eeprom:w:$(CONFIG_FILE):s

# Add -v option for more verbose output
# Add -V option for faster turnaround: eliminates verify cycle, but can add
# suprious fuse verify errors (which are safe to ignore if there is a final "Fuses OK")
# Add -F to disable the signature byte check

AVRDUDE_FLAGS = -v -V -C $(INSTALL_DIR)/hardware/tools/avr/etc/avrdude.conf \
		-p $(MCU) -P $(AVRDUDE_PORT) -c $(AVRDUDE_PROGRAMMER) $(AVRDUDE_BAUDRATE)

# ==========================================================================

FORMAT = ihex

# Name of this Makefile (used for "make depend").
MAKEFILE = Makefile

# Debugging format.
# Native formats for AVR-GCC's -g are stabs [default], or dwarf-2.
# AVR (extended) COFF requires stabs, plus an avr-objcopy run.
DEBUG = stabs

OPT = s

# Place -D or -U options here
CDEFS = -DF_CPU=$(F_CPU) -DARDUINO=100
CXXDEFS = -DF_CPU=$(F_CPU) -DARDUINO=100

# Place -I options here
CINCS = -I$(ARDUINO) -I$(PINS_INC_DIR)
CXXINCS = -I$(ARDUINO) -I$(PINS_INC_DIR)

# Compiler flag to set the C Standard level.
# c89   - "ANSI" C
# gnu89 - c89 plus GCC extensions
# c99   - ISO C99 standard (not yet fully implemented)
# gnu99 - c99 plus GCC extensions
CSTANDARD = -std=gnu99
CDEBUG = -g$(DEBUG)
CWARN = -Wall -Wstrict-prototypes
CTUNING = -funsigned-char -funsigned-bitfields -fpack-struct -fshort-enums
#CEXTRA = -Wa,-adhlns=$(<:.c=.lst)

CFLAGS = $(CDEBUG) $(CDEFS) $(CINCS) -O$(OPT) $(CWARN) $(CSTANDARD) $(CEXTRA)
CXXFLAGS = $(CDEFS) $(CINCS) -O$(OPT)
#ASFLAGS = -Wa,-adhlns=$(<:.S=.lst),-gstabs 
LDFLAGS = -lm



# Program settings
AVR_TOOLS_PATH = $(INSTALL_DIR)/hardware/tools/avr/bin
CC = $(AVR_TOOLS_PATH)/avr-gcc
CXX = $(AVR_TOOLS_PATH)/avr-g++
OBJCOPY = $(AVR_TOOLS_PATH)/avr-objcopy
OBJDUMP = $(AVR_TOOLS_PATH)/avr-objdump
AR  = $(AVR_TOOLS_PATH)/avr-ar
SIZE = $(AVR_TOOLS_PATH)/avr-size
NM = $(AVR_TOOLS_PATH)/avr-nm
AVRDUDE = $(AVR_TOOLS_PATH)/avrdude
REMOVE = rm -f
MV = mv -f

# Combine all necessary flags and optional flags.
# Add target processor to flags.
ALL_CFLAGS = -mmcu=$(MCU) -I. $(CFLAGS)
ALL_CXXFLAGS = -mmcu=$(MCU) -I. $(CXXFLAGS)
ALL_ASFLAGS = -mmcu=$(MCU) -I. -x assembler-with-cpp $(ASFLAGS)

# --------------------------------------------------------------------------
# Default target.
# --------------------------------------------------------------------------
all: $(BUILD_DIR)
		$(SIZE) --target=$(FORMAT) $(BUILD_DIR)/$(TARGET).hex

# --------------------------------------------------------------------------
# Special target to initialize MCU fuses
# --------------------------------------------------------------------------
fuses:
		$(AVRDUDE) $(AVRDUDE_FLAGS) $(AVR_FUSES)

# --------------------------------------------------------------------------
# Special target to load config data into EEPROM. The following make
# symbols must be defined on the command line when using this target:
#
#  SERIAL_NUMBER=... 32-bit, decimal or hex (0x...)
# --------------------------------------------------------------------------
config:
		./config.py $(SERIAL_NUMBER) > $(CONFIG_FILE)
		$(AVRDUDE) $(AVRDUDE_FLAGS) $(AVRDUDE_WRITE_EEPROM)
		rm $(CONFIG_FILE)

# --------------------------------------------------------------------------
# Rules for building the library
# --------------------------------------------------------------------------

CORE_C_OBJ = $(addprefix $(BUILD_DIR)/obj/, $(CORE_C_SRC:%.c=%.o))
CORE_CPP_OBJ = $(addprefix $(BUILD_DIR)/obj/, $(CORE_CPP_SRC:%.cpp=%.o))
LOCAL_C_OBJ = $(addprefix $(BUILD_DIR)/obj/, $(LOCAL_C_SRC:%.c=%.o))
LOCAL_CPP_OBJ = $(addprefix $(BUILD_DIR)/obj/, $(LOCAL_CPP_SRC:%.cpp=%.o))

$(CORE_C_OBJ) : $(BUILD_DIR)/obj/%.o : $(ARDUINO)/%.c
		$(CC) -c $(ALL_CFLAGS) $< -o $@

$(CORE_CPP_OBJ) : $(BUILD_DIR)/obj/%.o : $(ARDUINO)/%.cpp
		$(CXX) -c $(ALL_CXXFLAGS) $< -o $@

$(LOCAL_C_OBJ) : $(BUILD_DIR)/obj/%.o : ./%.c
		$(CC) -c $(ALL_CFLAGS) $< -o $@

$(LOCAL_CPP_OBJ) : $(BUILD_DIR)/obj/%.o : ./%.cpp
		$(CXX) -c $(ALL_CXXFLAGS) $< -o $@

LIBOBJ = $(CORE_C_OBJ) $(CORE_CPP_OBJ) $(LOCAL_C_OBJ) $(LOCAL_CPP_OBJ)

# --------------------------------------------------------------------------

# --------------------------------------------------------------------------
# Check amount of statically allocated SRAM being used.
# Look for the offset of the _end symbol which should be less than
# $800 = 2048 bytes. The difference between these is the available
# space for heap and stack allocation.
# --------------------------------------------------------------------------
sram : elf
		$(NM) -n $(BUILD_DIR)/$(TARGET).elf

# Add 'asm' to to this list to always create an assembly listing
##$(BUILD_DIR): elf hex asm
$(BUILD_DIR): elf hex

elf: $(BUILD_DIR)/$(TARGET).elf
hex: $(BUILD_DIR)/$(TARGET).hex
eep: $(BUILD_DIR)/$(TARGET).eep
lss: $(BUILD_DIR)/$(TARGET).lss 
sym: $(BUILD_DIR)/$(TARGET).sym

asm:
		$(CC) -S $(ALL_CXXFLAGS) utilities.cpp -o $(BUILD_DIR)/utilties.s

# --------------------------------------------------------------------------
# Re-compile the target and library if any headers changes
# --------------------------------------------------------------------------
HEADERS := $(wildcard *.h)

$(BUILD_DIR)/$(TARGET).elf : $(HEADERS)

$(BUILD_DIR)/core.a: $(HEADERS)

# Program the device.  
upload: $(BUILD_DIR)/$(TARGET).hex
		$(AVRDUDE) $(AVRDUDE_FLAGS) $(AVRDUDE_WRITE_FLASH)


# Convert ELF to COFF for use in debugging / simulating in AVR Studio or VMLAB.
COFFCONVERT=$(OBJCOPY) --debugging \
--change-section-address .data-0x800000 \
--change-section-address .bss-0x800000 \
--change-section-address .noinit-0x800000 \
--change-section-address .eeprom-0x810000 


coff: $(BUILD_DIR)/$(TARGET).elf
		$(COFFCONVERT) -O coff-avr $(BUILD_DIR)/$(TARGET).elf $(TARGET).cof


extcoff: $(TARGET).elf
		$(COFFCONVERT) -O coff-ext-avr $(BUILD_DIR)/$(TARGET).elf $(TARGET).cof


.SUFFIXES: .elf .hex .eep .lss .sym

.elf.hex:
		$(OBJCOPY) -O $(FORMAT) -R .eeprom $< $@

.elf.eep:
		-$(OBJCOPY) -j .eeprom --set-section-flags=.eeprom="alloc,load" \
		--change-section-lma .eeprom=0 -O $(FORMAT) $< $@

# Create extended listing file from ELF output file.
.elf.lss:
		$(OBJDUMP) -h -S $< > $@

# Create a symbol table from ELF output file.
.elf.sym:
		$(NM) -n $< > $@

# Link: create ELF output file from library.
$(BUILD_DIR)/$(TARGET).elf: $(TARGET).cpp $(BUILD_DIR)/core.a
		$(CC) $(ALL_CXXFLAGS) -o $@ $(TARGET).cpp -D COMMIT_INFO="$(shell ./commitInfo.py)" -L. $(BUILD_DIR)/core.a $(LDFLAGS)

$(BUILD_DIR)/core.a: $(LIBOBJ)
		@for i in $(LIBOBJ); do echo $(AR) rcs $(BUILD_DIR)/core.a $$i; $(AR) rcs $(BUILD_DIR)/core.a $$i; done



# Compile: create object files from C++ source files.
.cpp.o:
		$(CXX) -c $(ALL_CXXFLAGS) $< -o $@ 

# Compile: create object files from C source files.
.c.o:
		$(CC) -c $(ALL_CFLAGS) $< -o $@ 


# Compile: create assembler files from C source files.
.c.s:
		$(CC) -S $(ALL_CFLAGS) $< -o $@


# Assemble: create object files from assembler source files.
.S.o:
		$(CC) -c $(ALL_ASFLAGS) $< -o $@

# Clean project.
clean:
		$(REMOVE) $(BUILD_DIR)/$(TARGET).hex $(BUILD_DIR)/$(TARGET).eep $(BUILD_DIR)/$(TARGET).cof $(BUILD_DIR)/$(TARGET).elf \
		$(BUILD_DIR)/$(TARGET).map $(BUILD_DIR)/$(TARGET).sym $(BUILD_DIR)/$(TARGET).lss $(BUILD_DIR)/core.a \
		$(LIBOBJ) $(SRC:.c=.s) $(SRC:.c=.d) $(CXXSRC:.cpp=.s) $(CXXSRC:.cpp=.d)

.PHONY:        all $(BUILD_DIR) elf hex eep lss sym program coff extcoff clean
