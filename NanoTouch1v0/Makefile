###############################################################################
# Makefile for the project Nanotouch
###############################################################################

## General Flags
PROJECT = Nanotouch
TARGET = Nanotouch.elf
CC = avr-gcc

# Change if your programmer is different
AVRDUDE_PROGRAMMER = usbtiny
#AVRDUDE_PROGRAMMER = stk500v1
AVRDUDE_PORT = com5	   # programmer connected to serial device

# enter the target CPU frequency
AVR_FREQ   = 16000000L

MCU_TARGET = atmega32u4

AVRDUDE = avrdude
AVRDUDE_FLAGS = -p $(MCU_TARGET) -P $(AVRDUDE_PORT) -c $(AVRDUDE_PROGRAMMER) -p $(MCU_TARGET)

## Options common to compile, link and assembly rules
COMMON = -mmcu=$(MCU_TARGET)

override CFLAGS = -g -Wall -Os -mmcu=$(MCU_TARGET) -DF_CPU=$(AVR_FREQ) $(DEFS)  -ffunction-sections  -gdwarf-2 
override CFLAGS2 = -g -Wall -O2 -mmcu=$(MCU_TARGET) -DF_CPU=$(AVR_FREQ) $(DEFS)  -ffunction-sections  -gdwarf-2 

## Assembly specific flags
ASMFLAGS = $(COMMON)
ASMFLAGS += $(CFLAGS)
ASMFLAGS += -x assembler-with-cpp -Wa,-gdwarf2

## Linker flags
LDFLAGS = $(COMMON)
LDFLAGS += -Wl,-Map=Nanotouch.map 
LDFLAGS += -Wl,-gc-sections 


## Intel Hex file production flags
HEX_FLASH_FLAGS = -R .eeprom

HEX_EEPROM_FLAGS = -j .eeprom
HEX_EEPROM_FLAGS += --set-section-flags=.eeprom="alloc,load"
HEX_EEPROM_FLAGS += --change-section-lma .eeprom=0 --no-change-warnings

## Objects explicitly added by the user
LINKONLYOBJECTS = 

## Include Directories
INCLUDES = -I. -Iusbdrv

## Build
all: $(TARGET) Nanotouch.hex Nanotouch.eep Nanotouch.lss size

## Compile sources
OBJECTS = Nanotouch.o OLED.o mmc.o microfat2.o Application.o File.o Tunnel.o Doomed.o

//COMPILE = avr-gcc -Wall -Os -I. -Iusbdrv -mmcu=$(MCU_TARGET) -DF_CPU=16000000 -DDEBUG_LEVEL=0

## main
.S.o:
	$(CC) -x assembler-with-cpp -c $< -o $@	
.c.o:
	$(CC) $(INCLUDES) $(CFLAGS) -c  $<
.cpp.o:
	$(CC) $(INCLUDES) $(CFLAGS) -c  $<

	
##Link
Nanotouch.elf: $(OBJECTS)
	 $(CC) $(LDFLAGS) $(OBJECTS) $(LINKONLYOBJECTS) $(LIBDIRS) $(LIBS) -o $(TARGET)

%.hex: $(TARGET)
	avr-objcopy -O ihex $(HEX_FLASH_FLAGS)  $< $@

%.eep: $(TARGET)
	-avr-objcopy $(HEX_EEPROM_FLAGS) -O ihex $< $@ || exit 0

%.lss: $(TARGET)
	avr-objdump -h -S $< > $@

size: ${TARGET}
	@echo
	@avr-size -C --mcu=${MCU} ${TARGET}

## Clean target
.PHONY: clean
clean:
	-rm -rf $(OBJECTS) Nanotouch.elf dep/* Nanotouch.hex Nanotouch.eep Nanotouch.lss Nanotouch.map

program: $(TARGET).hex
	$(AVRDUDE) $(AVRDUDE_FLAGS) -B 1 -u -U flash:w:$(TARGET).hex
	
## Other dependencies
-include $(shell mkdir dep 2>/dev/null) $(wildcard dep/*)

