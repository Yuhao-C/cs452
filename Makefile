#
# Makefile for CS 452 kernel
#
XDIR=/u/cs452/public/xdev
XBINDIR=$(XDIR)/bin
XLIBDIR1=$(XDIR)/arm-none-eabi/lib
XLIBDIR2=$(XDIR)/lib/gcc/arm-none-eabi/9.2.0
STDCXX_INCLUDE_DIR=$(XDIR)/arm-none-eabi/include/c++/9.2.0/

CC = $(XBINDIR)/arm-none-eabi-gcc
CXX = $(XBINDIR)/arm-none-eabi-g++
AR = $(XBINDIR)/arm-none-eabi-ar
AS = $(XBINDIR)/arm-none-eabi-as
LD = $(XBINDIR)/arm-none-eabi-ld

# C preprocessor flags
CPPFLAGS = -I./include -I./user/include -I./track/include -I./calibration/include -MMD -MP

# C++ compiler flags
# -g: include debug information for gdb
# -S: only compile and emit assembly
# -fPIC: emit position-independent code
# -Wall: report all warnings
# -mcpu=arm920t: generate code for the 920t architecture
# -msoft-float: no FP co-processor
CXXFLAGS = -g -fPIC -Wall -mcpu=arm920t -msoft-float -fno-rtti -fno-exceptions -O3

CXXFLAGS += -DENABLE_DISPLAY=0 -DENABLE_OPT=1 -DENABLE_CACHE=1 -DSENDER_FIRST=0

# c: create archive, if necessary
# r: insert with replacement
# s: add index to archive
ARFLAGS = crs

# -static: force static linking
# -e: set entry point
# -nmagic: no page alignment
# -T: use linker script
LDFLAGS = -static -e main -nmagic -T linker.ld -L ./lib -L $(XLIBDIR2) -L $(XLIBDIR1) 

LDLIBS = -lstdc++ -lc -lgcc

CXXSRC := $(shell find . -path './test' -prune -o -name '*.cc' -print)
ASMSRC := $(shell find . -name '*.S')

all: kern/kmain.elf

calibration/include/train_data.h: ./calibration/data/trains.json
	./calibration/calib_gen.py $^ $@

kern/kmain.elf: $(CXXSRC:%.cc=%.o) $(ASMSRC:%.S=%.o)
	$(LD) $(LDFLAGS) -o $@ $^ $(LDLIBS)

-include $(CXXSRC:%.cc=%.d) $(ASMSRC:%.S=%.d)

.PHONY: clean
clean:
	-find . -name '*.o' -delete
	-find . -name '*.d' -delete
	-find . -name '*.elf' -delete

.PHONY: install
install: all
	install kern/kmain.elf /u/cs452/tftp/ARM/$(USER)

.PHONY: uninstall
uninstall:
	-rm /u/cs452/tftp/ARM/$(USER)/kmain.elf
