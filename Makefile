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

# -g: include debug information for gdb
# -S: only compile and emit assembly
# -fPIC: emit position-independent code
# -Wall: report all warnings
# -mcpu=arm920t: generate code for the 920t architecture
# -msoft-float: no FP co-processor
CFLAGS = -g -fPIC -Wall -mcpu=arm920t -msoft-float -I./include -I./user/include

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

all: build build/lib/libklib.a build/bin/kmain.elf install

build:
	mkdir -p build
	mkdir -p build/lib
	mkdir -p build/bin
	mkdir -p build/user

build/task_kern.s: kern/task/task_kern.cc include/kern/task.h
	$(CXX) -S $(CXXFLAGS) -o $@ $<

build/task_kern.o: build/task_kern.s
	$(AS) $(ASFLAGS) -o $@ $<

build/task_user.o: kern/task/task_user.S include/user/task.h
	$(CXX) $(CXXFLAGS) -c -o $@ $<

build/syscall.s: kern/syscall/syscall.cc include/kern/syscall.h
	$(CXX) -S $(CXXFLAGS) -o $@ $<

build/syscall.o: build/syscall.s
	$(AS) $(ASFLAGS) -o $@ $<

build/exception.o: kern/syscall/exception.S
	$(AS) $(ASFLAGS) -o $@ $<

build/priority_queues.s: kern/task/priority_queues.cc include/kern/task.h
	$(CXX) -S $(CXXFLAGS) -o $@ $<

build/priority_queues.o: build/priority_queues.s
	$(AS) $(ASFLAGS) -o $@ $<

build/task_descriptor.s: kern/task/task_descriptor.cc include/kern/task.h
	$(CXX) -S $(CXXFLAGS) -o $@ $<

build/task_descriptor.o: build/task_descriptor.s
	$(AS) $(ASFLAGS) -o $@ $<

build/bwio.s: lib/bwio.c include/lib/bwio.h
	$(CXX) -S $(CXXFLAGS) -o $@ $<

build/bwio.o: build/bwio.s
	$(AS) $(ASFLAGS) -o $@ $<

build/assert.s: lib/assert.cc include/lib/assert.h
	$(CXX) -S $(CXXFLAGS) -o $@ $<

build/assert.o: build/assert.s
	$(AS) $(ASFLAGS) -o $@ $<

build/sys.s: kern/lib/sys.cc include/kern/sys.h
	$(CXX) -S $(CXXFLAGS) -o $@ $<

build/sys.o: build/sys.s
	$(AS) $(ASFLAGS) -o $@ $<

build/lib/libklib.a: build/bwio.o build/assert.o build/sys.o
	$(AR) $(ARFLAGS) $@ $^

build/kmain.s: kern/kmain.cc
	$(CXX) -S $(CXXFLAGS) -o $@ $<

build/kmain.o: build/kmain.s
	$(AS) $(ASFLAGS) -o $@ $<

build/user/k1.s: user/tasks/k1.cc user/include/k1.h
	$(CXX) -S $(CXXFLAGS) -o $@ $<

build/user/k1.o: build/user/k1.s
	$(AS) $(ASFLAGS) -o $@ $<

build/user/boot.s: user/boot.cc user/include/boot.h
	$(CXX) -S $(CXXFLAGS) -o $@ $<

build/user/boot.o: build/user/boot.s
	$(AS) $(ASFLAGS) -o $@ $<

build/bin/kmain.elf: build/kmain.o build/task_kern.o build/task_user.o build/syscall.o build/exception.o build/priority_queues.o build/task_descriptor.o build/user/boot.o build/user/k1.o
	$(LD) $(LDFLAGS) -o $@ $^ -lklib -lstdc++ -lc -lgcc

clean:
	-rm -rf build/

install:
	cp build/bin/kmain.elf /u/cs452/tftp/ARM/$(USER) && chmod o+r /u/cs452/tftp/ARM/$(USER)/kmain.elf
