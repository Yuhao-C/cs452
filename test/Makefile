#
# Makefile for CS 452 unit test
#

# C preprocessor flags
CPPFLAGS = -I../include -I../user/include -I../track/include -I../calibration/include -MMD -MP

# C++ compiler flags
# -g: include debug information for gdb
# -S: only compile and emit assembly
# -fPIC: emit position-independent code
# -Wall: report all warnings
# -mcpu=arm920t: generate code for the 920t architecture
# -msoft-float: no FP co-processor
CXXFLAGS = -g -Wall

# CXXFLAGS += -DENABLE_DISPLAY=0 -DENABLE_OPT=1 -DENABLE_CACHE=1 -DSENDER_FIRST=0

# c: create archive, if necessary
# r: insert with replacement
# s: add index to archive
# ARFLAGS = crs

# -static: force static linking
# -e: set entry point
# -nmagic: no page alignment
# -T: use linker script
# LDFLAGS = -static -e main -nmagic -T linker.ld -L ./lib -L $(XLIBDIR2) -L $(XLIBDIR1) 

# LDLIBS = -lstdc++ -lc -lgcc

# CXXSRC := $(shell find . -path './test' -prune -o -name '*.cc' -print)
# ASMSRC := $(shell find . -name '*.S')

all: main

# calibration/include/train_data.h: ./calibration/data/trains.json
#	./calibration/calib_gen.py $^ $@

main: main.cc ../track/src/seg_data.cc ../track/src/track_data.cc ../user/tasks/marklin/train.cc

# -include $(CXXSRC:%.cc=%.d) $(ASMSRC:%.S=%.d)

.PHONY: clean
clean:
	-rm main

# .PHONY: install
# install: all
# 	install kern/kmain.elf /u/cs452/tftp/ARM/$(USER)

# .PHONY: uninstall
# uninstall:
# 	-rm /u/cs452/tftp/ARM/$(USER)/kmain.elf
