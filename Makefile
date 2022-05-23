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
CPPFLAGS = -I./include -I./user/include

# C++ compiler flags
# -g: include debug information for gdb
# -S: only compile and emit assembly
# -fPIC: emit position-independent code
# -Wall: report all warnings
# -mcpu=arm920t: generate code for the 920t architecture
# -msoft-float: no FP co-processor
CXXFLAGS = -g -fPIC -Wall -mcpu=arm920t -msoft-float -fno-rtti -fno-exceptions

# c: create archive, if necessary
# r: insert with replacement
# s: add index to archive
ARFLAGS = crs

# -static: force static linking
# -e: set entry point
# -nmagic: no page alignment
# -T: use linker script
LDFLAGS = -static -e main -nmagic -T linker.ld -L ./lib -L $(XLIBDIR2) -L $(XLIBDIR1) 

LDLIBS = -lklib -lstdc++ -lc -lgcc

all: lib/libklib.a kern/kmain.elf

kern/task/task_kern.o: kern/task/task_kern.cc include/kern/task.h

kern/task/task_user.o: kern/task/task_user.S include/user/task.h

kern/syscall/syscall.o: kern/syscall/syscall.cc include/kern/syscall.h

kern/syscall/exception.o: kern/syscall/exception.S

kern/task/priority_queues.o: kern/task/priority_queues.cc include/kern/task.h

kern/task/task_descriptor.o: kern/task/task_descriptor.cc include/kern/task.h

lib/bwio.o: lib/bwio.cc include/lib/bwio.h

lib/assert.o: lib/assert.cc include/lib/assert.h

kern/lib/sys.o: kern/lib/sys.cc include/kern/sys.h

lib/libklib.a: lib/bwio.o lib/assert.o kern/lib/sys.o
	$(AR) $(ARFLAGS) $@ $^

kern/kmain.o: kern/kmain.cc

user/tasks/k1.o: user/tasks/k1.cc user/include/k1.h

user/boot.o: user/boot.cc user/include/boot.h

kern/kmain.elf: kern/kmain.o kern/task/task_kern.o kern/task/task_user.o kern/syscall/syscall.o kern/syscall/exception.o kern/task/priority_queues.o kern/task/task_descriptor.o user/boot.o user/tasks/k1.o
	$(LD) $(LDFLAGS) -o $@ $^ $(LDLIBS)

.PHONY: clean
clean:
	-rm -rf build/ kern/*/*.o kern/*.o lib/*.o user/*/*.o user/*.o kern/*.elf

.PHONY: install
install: all
	install kern/kmain.elf /u/cs452/tftp/ARM/$(USER)

.PHONY: uninstall
uninstall:
	-rm /u/cs452/tftp/ARM/$(USER)/kmain.elf
