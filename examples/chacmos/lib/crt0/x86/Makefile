OBJ_SRC   = $(wildcard *.S)

include ../../../Makeconf $(wildcard .depend)

all:	$(OBJ)
	cp crt0-x86.o ../..

clean::
	rm -f ../../crt0-x86.o
