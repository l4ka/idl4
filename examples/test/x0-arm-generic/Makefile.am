## Process this file with automake to produce Makefile.in
AUTOMAKE_OPTIONS= 		    -Wno-portability
IDL4=				    $(top_builddir)/src/idl4
IDLFLAGS=			    -dtest -dparanoid
TESTDIR=                            $(top_srcdir)/examples/test
IDL=				    $(TESTDIR)/idl/test.idl
LINK_ADDR=			    0x00A00000
CC=                                 $(ARM_PREFIX)gcc
LD=                                 $(ARM_PREFIX)ld
LIBGCC=				    $(shell $(CC) -print-libgcc-file-name)

test_x0_arm_generic_BIN=	    test_x0_arm_generic
nodist_test_x0_arm_generic_SOURCES=x0_arm_generic.c
test_x0_arm_generic_IDLFLAGS=	    $(IDLFLAGS) -ix0 -pgeneric -w32 -DX0 -DCONFIG_API_X0
test_x0_arm_generic_IDLFLAGS+=	    -DCONFIG_VERSION_X0 -DCONFIG_ARCH_ARM
test_x0_arm_generic_IDLFLAGS+=	    -I$(HAZELNUT_DIR)/apps/include
test_x0_arm_generic_LDADD=         $(top_builddir)/src/test/glue/x0-arm/crt0-arm.o \
                                    -L$(top_builddir)/src/test/glue/x0-arm/ -ltest_x0_arm -lgcc
test_x0_arm_generic_DEPENDENCIES=  $(top_builddir)/src/test/glue/x0-arm/libtest_x0_arm.a
test_x0_arm_generic_CFLAGS=	    -fomit-frame-pointer -DCONFIG_VERSION_X0 -DCONFIG_API_X0\
                                    -I$(HAZELNUT_DIR)/apps/include -DCONFIG_ARCH_ARM
test_x0_arm_generic_LDFLAGS=       -N -Ttext=$(LINK_ADDR) --defsym __textstart=$(LINK_ADDR) -L$(dir $(LIBGCC))
test_x0_arm_generic_LINK=          $(LD) -o test_x0_arm_generic

bindir=				    $(TESTSUITE_DIR)
bin_PROGRAMS=			    $(test_x0_arm_generic_BIN)

CLEANFILES=			    x0_arm_generic_client.h x0_arm_generic_server.h x0_arm_generic.c

INCLUDES=			    -I$(TESTDIR)/idl -I$(top_srcdir)/src/include/user -I$(TESTDIR)
AM_CFLAGS=			    -Wall -nostdinc
AM_CFLAGS+=			    -O9 -funsigned-char
AM_CFLAGS+=			    -W -Wno-unused -funsigned-bitfields -fno-builtin -ffreestanding
AM_CFLAGS+=			    -fomit-frame-pointer 
AM_CFLAGS+= 			    -Wshadow -Wpointer-arith
AM_CFLAGS+= 			    -Wbad-function-cast -Wcast-align -Wwrite-strings
AM_CFLAGS+= 			    -Wsign-compare -Wstrict-prototypes -Wno-conversion
AM_CFLAGS+= 			    -Wno-missing-prototypes -Wno-missing-declarations -Wmissing-noreturn
AM_CFLAGS+= 			    -Wredundant-decls -Wnested-externs -Winline -Wlong-long

$(IDL4):
	(cd $(top_builddir)/src && $(MAKE))

$(top_builddir)/src/test/glue/x0-arm/libtest_x0_arm.a:
	(cd $(top_builddir)/src/test/glue/x0-arm && $(MAKE) libtest_x0_arm.a)

$(top_builddir)/src/test/glue/x0-arm/crt0-arm.o:
	(cd $(top_builddir)/src/test/glue/x0-arm/ && $(MAKE) crt0-arm.o)

x0_arm_generic_client.h: $(IDL) $(IDL4)
	(cd $(HAZELNUT_DIR)/apps/include/l4;rm arch;ln -s arm/ arch)
	$(IDL4) $(test_x0_arm_generic_IDLFLAGS) -c -h $@ $(IDL) 

x0_arm_generic_server.h: $(IDL) $(IDL4)
	(cd $(HAZELNUT_DIR)/apps/include/l4;rm arch;ln -s arm/ arch)
	$(IDL4) $(test_x0_arm_generic_IDLFLAGS) -s -h $@ $(IDL) 

x0_arm_generic.c: x0_arm_generic_client.h x0_arm_generic_server.h $(IDL) $(IDL4)
	(cd $(HAZELNUT_DIR)/apps/include/l4;rm arch;ln -s arm/ arch)
	$(IDL4) $(test_x0_arm_generic_IDLFLAGS) -t -o $@ $(IDL)

diff: x0_arm_generic_client.h x0_arm_generic_server.h x0_arm_generic.c
	diff reference/x0_arm_generic_client.h x0_arm_generic_client.h || true
	diff reference/x0_arm_generic_server.h x0_arm_generic_server.h || true
	diff reference/x0_arm_generic.c x0_arm_generic.c || true
