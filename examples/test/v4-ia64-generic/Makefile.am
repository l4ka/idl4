## Process this file with automake to produce Makefile.in

IDL4=				    $(top_builddir)/src/idl4
IDLFLAGS=			    -dtest -dparanoid
TESTDIR=                            $(top_srcdir)/examples/test
IDL=				    $(TESTDIR)/idl/test.idl
LINK_ADDR=			    0x00300000
CC=                                 $(IA64_PREFIX)gcc
LD=                                 $(IA64_PREFIX)ld
LIBGCC=				    $(shell $(CC) -print-libgcc-file-name)

test_v4_ia64_generic_BIN=           test_v4_ia64_generic
nodist_test_v4_ia64_generic_SOURCES=v4_ia64_generic.c
test_v4_ia64_generic_IDLFLAGS=      $(IDLFLAGS) -iv4 -pgeneric -w64
test_v4_ia64_generic_IDLFLAGS+=     -DV4 -DCONFIG_API_V4
test_v4_ia64_generic_IDLFLAGS+=     -DCONFIG_VERSION_V4 -DCONFIG_ARCH_IA64
test_v4_ia64_generic_IDLFLAGS+=     -I$(PISTACHIO_IA64_DIR)/include
test_v4_ia64_generic_LDADD=         $(top_builddir)/src/test/glue/v4-ia64/crt0-ia64.o \
                                    -L$(top_builddir)/src/test/glue/v4-ia64 -ltest_v4_ia64 \
                                    -L$(PISTACHIO_IA64_DIR)/lib -ll4 -lgcc
test_v4_ia64_generic_DEPENDENCIES=  $(top_builddir)/src/test/glue/v4-ia64/libtest_v4_ia64.a
test_v4_ia64_generic_CFLAGS=        -I$(PISTACHIO_IA64_DIR)/include -fomit-frame-pointer -DCONFIG_ARCH_IA64 \
                                    -finline -fno-builtin-memcpy
test_v4_ia64_generic_LDFLAGS=       -N -Ttext=$(LINK_ADDR) --defsym __textstart=$(LINK_ADDR) -L$(dir $(LIBGCC))
test_v4_ia64_generic_LINK=          $(LD) -o test_v4_ia64_generic

bindir=				    $(TESTSUITE_DIR)
bin_PROGRAMS=			    $(test_v4_ia64_generic_BIN) 

CLEANFILES=			    v4_ia64_generic_client.h v4_ia64_generic_server.h v4_ia64_generic.c

INCLUDES=			    -I$(top_srcdir)/examples/test/idl -I$(top_srcdir)/src/include/user -I$(srcdir)
AM_CFLAGS=			    -Wall -nostdinc -funsigned-char
AM_CFLAGS+=                         -O2 -finline -fno-builtin -ffreestanding
AM_CFLAGS+=			    -W -Wno-unused -funsigned-bitfields
AM_CFLAGS+=			    -fomit-frame-pointer 
AM_CFLAGS+= 			    -Wno-traditional -Wshadow -Wpointer-arith
AM_CFLAGS+= 			    -Wbad-function-cast -Wcast-align -Wwrite-strings
AM_CFLAGS+= 			    -Wsign-compare -Wstrict-prototypes -Wno-conversion
AM_CFLAGS+= 			    -Wno-missing-prototypes -Wno-missing-declarations -Wmissing-noreturn
AM_CFLAGS+= 			    -Wredundant-decls -Wnested-externs -Winline -Wlong-long

$(IDL4):
	(cd $(top_builddir)/src && $(MAKE))

$(top_builddir)/src/test/glue/v4-ia64/libtest_v4_ia64.a:
	(cd $(top_builddir)/src/test/glue/v4-ia64 && $(MAKE))

v4_ia64_generic_client.h: $(IDL) $(IDL4)
	$(IDL4) $(test_v4_ia64_generic_IDLFLAGS) -c -h $@ $(IDL) 

v4_ia64_generic_server.h: $(IDL) $(IDL4)
	$(IDL4) $(test_v4_ia64_generic_IDLFLAGS) -s -h $@ $(IDL) 

v4_ia64_generic.c: v4_ia64_generic_client.h v4_ia64_generic_server.h $(IDL) $(IDL4)
	$(IDL4) $(test_v4_ia64_generic_IDLFLAGS) -t -o $@ $(IDL)

diff: v4_ia64_generic_client.h v4_ia64_generic_server.h v4_ia64_generic.c
	diff reference/v4_ia64_generic_client.h v4_ia64_generic_client.h || true
	diff reference/v4_ia64_generic_server.h v4_ia64_generic_server.h || true
	diff reference/v4_ia64_generic.c v4_ia64_generic.c || true
