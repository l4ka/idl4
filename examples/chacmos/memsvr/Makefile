OBJ_SRC   = $(wildcard *.c)
LINK_ADDR = 0x00240000
SERVER    = memsvr pager
CLIENT	  = generic task directory pager
CRT0      = x86
LIBS      = std
TARGET	  = $(notdir $(shell "pwd"))

include	../Makeconf

all:	$(TARGET)
