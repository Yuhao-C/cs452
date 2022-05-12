#
# Makefile for CS 452 kernel
#
XDIR=/u/cs452/public/xdev
XBINDIR=$(XDIR)/bin
XLIBDIR1=$(XDIR)/arm-none-eabi/lib
XLIBDIR2=$(XDIR)/lib/gcc/arm-none-eabi/9.2.0
STDCXX_INCLUDE_DIR=$(XDIR)/arm-none-eabi/include/c++/9.2.0/

CXX = $(XBINDIR)/arm-none-eabi-g++
AR = $(XBINDIR)/arm-none-eabi-ar
AS = $(XBINDIR)/arm-none-eabi-as
LD = $(XBINDIR)/arm-none-eabi-ld

# -g: include debug information for gdb
# -S: only compile and emit assembly
# -fPIC: emit position-independent code
# -Wall: report all warnings
# -mcpu=arm920t: generate code for the 920t architecture
# -msoft-float: no FP co-processor
CFLAGS = -g -S -fPIC -Wall -mcpu=arm920t -msoft-float -I./include -I$(STDCXX_INCLUDE_DIR) 

CXXFLAGS = $(CFLAGS) -fno-rtti -fno-exceptions

# c: create archive, if necessary
# r: insert with replacement
# s: add index to archive
ARFLAGS = crs

# -static: force static linking
# -e: set entry point
# -nmagic: no page alignment
# -T: use linker script
LDFLAGS = -static -e main -nmagic -T linker.ld -L ./build/lib -L . -L $(XLIBDIR2) -L $(XLIBDIR1) 

# all: libnwio.a main.elf
all: build build/lib/libbwio.a build/bin/main.elf install

build:
	mkdir -p build && mkdir -p build/lib && mkdir -p build/bin

build/bwio.s: lib/bwio.c
	$(CXX) -S $(CXXFLAGS) -o $@ $<

build/bwio.o: build/bwio.s
	$(AS) $(ASFLAGS) -o $@ $<

build/lib/libbwio.a: build/bwio.o
	$(AR) $(ARFLAGS) $@ $<

build/main.s: startup/main.cc
	$(CXX) -S $(CXXFLAGS) -o $@ $<

build/main.o: build/main.s
	$(AS) $(ASFLAGS) -o $@ $<

build/bin/main.elf: build/main.o
	$(LD) $(LDFLAGS) -o $@ $< -lbwio -lstdc++ -lc -lgcc

clean:
	-rm -rf build/

install:
	cp build/bin/main.elf /u/cs452/tftp/ARM/$(USER) && chmod o+r /u/cs452/tftp/ARM/$(USER)/main.elf
