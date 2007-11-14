OBJ_SRC   = $(wildcard *.c) trampoline.S
LINK_ADDR = 0x00800000
SERVER    = booter pager
CLIENT	  = generic file memory directory creator pager
CRT0      = x86
LIBS      = std
TARGET	  = $(notdir $(shell "pwd"))

include	../Makeconf

all:	$(TARGET)
