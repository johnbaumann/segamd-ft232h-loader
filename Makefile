# Sample Makefile for Marsdev (No SGDK, With Newlib)

# Default paths, can be overridden by setting MARSDEV before calling make
MARSDEV ?= /opt/toolchains/mars
ifeq ($(OS),Windows_NT)
# Replace \ with / in MARSDEV path for Windows compatibility
MARSDEV := $(subst \,/,$(MARSDEV))
endif

MARSBIN  = $(MARSDEV)/m68k-elf/bin

# GCC and Binutils
CC   = $(MARSBIN)/m68k-elf-gcc
CXX  = $(MARSBIN)/m68k-elf-g++
AS   = $(MARSBIN)/m68k-elf-as
LD   = $(MARSBIN)/m68k-elf-ld
NM   = $(MARSBIN)/m68k-elf-nm
OBJC = $(MARSBIN)/m68k-elf-objcopy

# Some files needed are in a versioned directory
GCC_VER := $(shell $(CC) -dumpversion)

# Need the LTO plugin path so NM can dump our symbol table
PLUGIN   = $(MARSDEV)/m68k-elf/libexec/gcc/m68k-elf/$(GCC_VER)
LTO_SO   = liblto_plugin.so
ifeq ($(OS),Windows_NT)
	LTO_SO = liblto_plugin-0.dll
endif

# Includes: Local + GCC + Newlib
INCS     = -Isrc -Ires -Iinc
INCS    += -I$(MARSDEV)/m68k-elf/lib/gcc/m68k-elf/$(GCC_VER)/include
#INCS    += -I$(MARSDEV)/m68k-elf/m68k-elf/include

# Libraries: GCC + Newlib
LIBS     = -L$(MARSDEV)/m68k-elf/lib/gcc/m68k-elf/$(GCC_VER) -lgcc
#LIBS    += -L$(MARSDEV)/m68k-elf/m68k-elf/lib -lnosys

# Any C or C++ standard should be fine here as long as GCC supports it
# GCC 10+ by default fails to link if you don't declare header variables extern,
# and -fcommon reverts to the old behavior (needed to compile SGDK)
CCFLAGS  = -m68000 -Wall -Wextra -std=c99 -ffreestanding# -g
CXXFLAGS = -m68000 -Wall -Wextra -std=c++17 -ffreestanding -g

# Another useful GAS flag is --bitwise-or, but it breaks SGDK
ASFLAGS  = -m68000 --register-prefix-optional

# -nostartfiles disables the c runtime startup code
# -nostdlib unsets -lc, so when using Newlib don't set that option
LDFLAGS  = -Wl,-Map=ft232h-loader.map -T megadrive.ld -nostdlib -g

# Collect source files in the src directory
CS    = $(wildcard src/*.c)
CPPS  = $(wildcard src/*.cpp)
SS    = $(wildcard src/*.s)
OBJS  = $(CS:.c=.o)
OBJS += $(CPPS:.cpp=.o)
OBJS += $(SS:.s=.o)

ASMO  = $(RESS:.res=.o)
ASMO += $(Z80S:.s80=.o)
ASMO += $(CS:%.c=asmout/%.s)

.PHONY: all release asm debug
all: release

release: OPTIONS  = -O3 -fno-web -fno-gcse -fomit-frame-pointer #-fno-unit-at-a-time # Makes release larger, restore this option if something breaks >_>
release: OPTIONS += -fshort-enums -flto -fuse-linker-plugin
release: ft232h-loader.bin symbol.txt

asm: OPTIONS  = -O3 -fno-web -fno-gcse -fno-unit-at-a-time -fomit-frame-pointer
asm: OPTIONS += -fshort-enums
asm: asm-dir $(ASMO)

# Gens-KMod, BlastEm and UMDK support GDB tracing, enabled by this target
debug: OPTIONS = -g -O0 -DDEBUG -DKDEBUG
debug: ft232h-loader.bin symbol.txt

# Generates a symbol table that is very helpful in debugging crashes
# Cross reference symbol.txt with the addresses displayed in the crash handler
symbol.txt: ft232h-loader.bin
	@echo "Creating symbol.txt"
	@$(NM) --plugin=$(PLUGIN)/$(LTO_SO) -n ft232h-loader.elf > symbol.txt

%.bin: %.elf
	@echo "Stripping ELF header..."
	@$(OBJC) -O binary $< temp.bin
	@dd if=temp.bin of=$@ bs=8K conv=sync
	@rm -f temp.bin

%.elf: $(OBJS)
	@echo "Linking $@"
	@$(CC) -o $@ $(LDFLAGS) $(OBJS) $(LIBS)

%.o: %.c
	@echo "CC $<"
	@$(CC) $(CCFLAGS) $(OPTIONS) $(INCS) -c $< -o $@

%.o: %.cpp
	@echo "CXX $<"
	@$(CXX) $(CXXFLAGS) $(OPTIONS) $(INCS) -c $< -o $@

%.o: %.s 
	@echo "AS $<"
	@$(AS) $(ASFLAGS) $< -o $@

.PRECIOUS: %.elf

# For asm target
asm-dir:
	mkdir -p asmout/src

asmout/%.s: %.c
	$(CC) $(CCFLAGS) $(OPTIONS) $(INCS) -S $< -o $@

.PHONY: clean
clean:
	rm -f $(OBJS) ft232h-loader.bin ft232h-loader.elf ft232h-loader.map symbol.txt boot.o
	rm -rf asmout