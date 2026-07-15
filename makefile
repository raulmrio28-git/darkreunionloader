##############################################################################################
# Start of default section
#

TRGT = arm-none-eabi-
CC   = $(TRGT)gcc 
CP   = $(TRGT)objcopy
AS   = $(TRGT)gcc -x assembler-with-cpp
HEX  = $(CP) -O ihex
BIN  = $(CP) -O binary
OBJDUMP = $(TRGT)objdump

MCU  = arm7tdmi

# List all default C defines here, like -D_DEBUG=1
DDEFS =

# List all default ASM defines here, like -D_DEBUG=1
DADEFS =

# List all default directories to look for include files here
DINCDIR =

# List the default directory to look for the libraries here
DLIBDIR =

# List all default libraries here
DLIBS =

#
# End of default section
##############################################################################################

##############################################################################################
# Start of user section
#

# Define project name here
#DRL - rename project 20260712
PROJECT = dark_reunion
#DRL - rename project 20260712

# Define linker script file here
LDSCRIPT = build/ram.ld

# List all user C define here, like -D_DEBUG=1
UDEFS =

# Define ASM defines here
UADEFS =

# List C source files here
# SRC  = main.c dcc/dn_dcc_proto.c minilzo/minilzo.c lwmem/lwmem.c flash/cfi/cfi.c
DEVICES = flash/mmap/mmap.c
CONTROLLERS = 
ADD_DEPS = 
PLATFORM = default

# Additional deps
ifeq ($(LZO), 1)
ADD_DEPS += minilzo/minilzo.c
DDEFS += -DHAVE_MINILZO=1
else
DDEFS += -DHAVE_MINILZO=0
endif

ifeq ($(LZ4), 1)
ADD_DEPS += lz4/lz4_fs.c
DDEFS += -DHAVE_LZ4=1
else
DDEFS += -DHAVE_LZ4=0
endif

ifeq ($(LWMEM), 1)
ADD_DEPS += lwmem/lwmem.c
DDEFS += -DHAVE_LWMEM=1
DADEFS += -DHAVE_LWMEM=1
else
DDEFS += -DHAVE_LWMEM=0
DADEFS += -DHAVE_LWMEM=0
endif

# Devices
#DRL - modify CFI to NOR 20260713
ifeq ($(NOR_FLASH), 1)
DDEFS += -DUSE_NOR_FLASH
DADEFS += -DUSE_NOR_FLASH
DEVICES += flash/nor/nor.c
DEVICES += flash/nor/nor_cfi.c
DEVICES += flash/nor/nor_samsung.c
DEVICES += flash/nor/nor_amd_spansion.c
DEVICES += flash/nor/nor_intel_numonyx.c
DEVICES += flash/nor/nor_toshiba.c
DEVICES += flash/nor/nor_mitsubishi_renesas.c
#DRL - modify CFI to NOR 20260713
endif

ifdef NAND_CONTROLLER
#DRL - add NAND support 20260715
DDEFS += -DUSE_NAND_FLASH
DADEFS += -DUSE_NAND_FLASH
#DRL - add NAND support 20260715
DEVICES += flash/nand/nand.c
CONTROLLERS += flash/nand/controller/$(NAND_CONTROLLER).c
endif

ifdef ONENAND_CONTROLLER
DEVICES += flash/onenand/onenand.c
CONTROLLERS += flash/onenand/controller/$(ONENAND_CONTROLLER).c
endif

ifdef SUPERAND_CONTROLLER
DEVICES += flash/superand/superand.c
CONTROLLERS += flash/superand/controller/$(SUPERAND_CONTROLLER).c
endif

ifeq ($(MCU), xscale)
DDEFS += -DCPU_XSCALE
DADEFS += -DCPU_XSCALE
endif

ifeq ($(USE_ICACHE), 1)
DADEFS += -DUSE_ICACHE=1
else
DADEFS += -DUSE_ICACHE=0
endif

ifeq ($(BP_LOADER), 1)
DADEFS += -DUSE_BREAKPOINTS=1
DDEFS += -DUSE_BREAKPOINTS=1
else
DADEFS += -DUSE_BREAKPOINTS=0
DDEFS += -DUSE_BREAKPOINTS=0
endif

#DRL - add platform-specific stuff for I/O 20260712
SRC = main.c dcc/memory.c dcc/bitutils.c dcc/lwprintf.c io/common.c io/packet_io.c cmds.c plat/$(PLATFORM).c id.c $(DEVICES) $(CONTROLLERS) $(ADD_DEPS)

ifeq ($(PLATFORM), qcom/qsc60x0)
DADEFS += -DPLATFORM_USES_USB=1
DDEFS += -DPLATFORM_USES_USB=1
DADEFS += -DPLATFORM_USES_UART=1
DDEFS += -DPLATFORM_USES_UART=1
SRC += io/qsc60x0_usb.c io/qsc60x0_uart.c 
endif
#DRL - add platform-specific stuff for I/O 20260712

#DRL - add platform-specific stuff for I/O (QSC60x5) 20260712
ifeq ($(PLATFORM), qcom/qsc60x5)
DADEFS += -DPLATFORM_USES_USB=1
DDEFS += -DPLATFORM_USES_USB=1
DADEFS += -DPLATFORM_USES_UART=1
DDEFS += -DPLATFORM_USES_UART=1
SRC += io/qsc60x5_usb.c io/qsc60x5_uart.c 
endif
#DRL - add platform-specific stuff for I/O (QSC60x5) 20260712

# List ASM source files here
ASRC = crt.s

# List all user directories here
UINCDIR =

# List the user directory to look for the libraries here
ULIBDIR =

# List all user libraries here
ULIBS =

# Define optimisation level here
OPT = -O2

#
# End of user defines
##############################################################################################


INCDIR  = $(patsubst %,-I%,$(DINCDIR) $(UINCDIR))
LIBDIR  = $(patsubst %,-L%,$(DLIBDIR) $(ULIBDIR))
DEFS    = $(DDEFS) $(UDEFS)
ADEFS   = $(DADEFS) $(UADEFS)
OBJS    = $(ASRC:.s=.o) $(SRC:.c=.o)
LIBS    = $(DLIBS) $(ULIBS)
MCFLAGS = -mcpu=$(MCU)

ASFLAGS = $(MCFLAGS) -g -gdwarf-2 -Wa,-amhls=$(<:.s=.lst) $(ADEFS) -c
CPFLAGS = $(MCFLAGS) -fPIC -fPIE -I . $(OPT) -gdwarf-2 -mthumb-interwork -fomit-frame-pointer -Wall -Wstrict-prototypes -fverbose-asm -Wa,-ahlms=$(<:.c=.lst) $(DEFS) -c
LDFLAGS = $(MCFLAGS) -fPIC -fPIE -nostartfiles -T$(LDSCRIPT) -Wl,-Map=build/$(PROJECT).map,--cref,--no-warn-mismatch $(LIBDIR)

# Generate dependency information
#CPFLAGS += -MD -MP -MF .dep/$(@F).d

#
# makefile rules
#

all: $(OBJS) $(PROJECT).elf $(PROJECT).hex $(PROJECT).bin $(PROJECT).lst

%.o : %.c
	$(CC) -c $(CPFLAGS) -I . $(INCDIR) $< -o $@

%.o : %.s
	$(AS) -c $(ASFLAGS) $< -o $@

%elf: $(OBJS)
	$(CC) $(OBJS) $(LDFLAGS) $(LIBS) -o build/$@

%hex: %elf
	$(HEX) build/$< build/$@

%bin: %elf
	$(BIN) build/$< build/$@

%.lst: %.elf
	$(OBJDUMP) -h -S build/$< > build/$@

clean:
	-rm -f $(OBJS)
	-rm -f build/$(PROJECT).elf
	-rm -f build/$(PROJECT).map
	-rm -f build/$(PROJECT).hex
	-rm -f build/$(PROJECT).bin
	-rm -f build/$(PROJECT).lst
	-rm -f $(SRC:.c=.c.bak)
	-rm -f $(SRC:.c=.lst)
	-rm -f $(ASRC:.s=.s.bak)
	-rm -f $(ASRC:.s=.lst)
	-rm -fR .dep

help:
#DRL - modify CFI to NOR 20260713
#DRL - add platform-specific stuff for I/O 20260712
	@echo Dumpnow DCC Loader
	@echo 	LZO=1 = Enable LZO Compression
	@echo 	LZ4=1 = Enable LZ4 Compression
	@echo 	LWMEM=1 = Enable LWMEM memory management
	@echo 	PLATFORM=(name) Select chipset platform
	@echo 	MCU=(MCU) = Select CPU architecture
	@echo 	NOR_FLASH=1 = Enable NOR handling
	@echo 	NAND_CONTROLLER=(name) = Enable NAND controller
	@echo 	ONENAND_CONTROLLER=(name) = Enable OneNAND controller
	@echo 	SUPERAND_CONTROLLER=(name) = Enable SuperAND controller
	@echo 	USE_ICACHE=1 = Use instruction cache (ARM9 and later)
	@echo 	BP_LOADER=1 = If the chipset have broken DCC Support, compiling as Breakpoint-based loader might help
#DRL - add platform-specific stuff for I/O 20260712
#DRL - modify CFI to NOR 20260713
#
# Include the dependency files, should be the last of the makefile
#
#-include $(shell mkdir .dep 2>/dev/null) $(wildcard .dep/*)

# *** EOF ***
