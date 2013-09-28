#MPIDE=/opt/mpide
TOOLCHAIN_PREFIX=$(MPIDE)/hardware/pic32/compiler/pic32-tools
CC=$(TOOLCHAIN_PREFIX)/bin/pic32-gcc
CXX=$(TOOLCHAIN_PREFIX)/bin/pic32-g++
AR=$(TOOLCHAIN_PREFIX)/bin/pic32-ar

CPUTYPE=32MX795F512L
VARIANT=Max32

CFLAGS=-O2 -c -mno-smart-io -w -fno-exceptions -ffunction-sections -fdata-sections \
			 -g -mdebugger -Wcast-align -fno-short-double -mprocessor=$(CPUTYPE) \
			 -DF_CPU=80000000L -DARDUINO=23 -D_BOARD_MEGA_ -DMPIDEVER=0x01000202 \
			 -DMPIDE=23 -Ivariants/$(VARIANT) -I.


SRC_C=$(wildcard *.c) 
SRC_CPP=$(wildcard *.cpp)
SRC_S=$(wildcard *.S)

OBJ_C=$(patsubst %.c,%.o,$(SRC_C))
OBJ_CPP=$(patsubst %.cpp,%.o,$(SRC_CPP))
OBJ_S=$(patsubst %.S,%.o,$(SRC_S))

all: core.a

%.o: %.S
	$(CXX) $(CFLAGS) $< -o $@

%.o: %.c
	$(CC) $(CFLAGS) $< -o $@

%.o: %.cpp
	$(CXX) $(CFLAGS) $< -o $@

core.a: $(OBJ_S) $(OBJ_C) $(OBJ_CPP)
	$(AR) rcs core.a *.o

.PHONY: clean
clean:
	-rm *.o
	-rm core.a
