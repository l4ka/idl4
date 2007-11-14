## Process this file with automake to produce Makefile.in

IDL4=				    $(top_builddir)/src/idl4
IDLFLAGS=			    -dtest -dparanoid
TESTDIR=                            $(top_srcdir)/examples/test
IDL=				    $(TESTDIR)/idl/test.idl
LINK_ADDR=			    0x00300000
CC=                                 $(IA64_PREFIX)gcc
LD=                                 $(IA64_PREFIX)ld
LIBGCC=				    $(shell $(CC) -print-libgcc-file-name)

test_v4_ia64_BIN=                   test_v4_ia64
nodist_test_v4_ia64_SOURCES=        v4_ia64.c
test_v4_ia64_IDLFLAGS=              $(IDLFLAGS) -iv4 -pia64 -w64
test_v4_ia64_IDLFLAGS+=             -DV4 -DCONFIG_API_V4
test_v4_ia64_IDLFLAGS+=             -DCONFIG_VERSION_V4 -DCONFIG_ARCH_IA64
test_v4_ia64_IDLFLAGS+=             -I$(PISTACHIO_IA64_DIR)/include
test_v4_ia64_LDADD=                 $(top_builddir)/src/test/glue/v4-ia64/crt0-ia64.o \
                                    -L$(top_builddir)/src/test/glue/v4-ia64 -ltest_v4_ia64 \
                                    -L$(PISTACHIO_IA64_DIR)/lib -ll4 -lgcc
test_v4_ia64_DEPENDENCIES=          $(top_builddir)/src/test/glue/v4-ia64/libtest_v4_ia64.a
test_v4_ia64_CFLAGS=                -I$(PISTACHIO_IA64_DIR)/include -fomit-frame-pointer -DCONFIG_ARCH_IA64
test_v4_ia64_LDFLAGS=               -N -Ttext=$(LINK_ADDR) --defsym __textstart=$(LINK_ADDR) -L$(dir $(LIBGCC))
test_v4_ia64_LINK=                  $(LD) -o test_v4_ia64

bindir=				    $(TESTSUITE_DIR)
bin_PROGRAMS=			    $(test_v4_ia64_BIN) 

CLEANFILES=			    v4_ia64_client.h v4_ia64_server.h v4_ia64.c

INCLUDES=			    -I$(top_srcdir)/examples/test/idl -I$(top_srcdir)/src/include/user -I$(srcdir)
AM_CFLAGS=			    -Wall -nostdinc -funsigned-char -finline
#AM_CFLAGS+=                        -O9
AM_CFLAGS+=			    -W -Wno-unused -funsigned-bitfields
AM_CFLAGS+=			    -fomit-frame-pointer -Uia64
AM_CFLAGS+= 			    -Wno-traditional -Wshadow -Wpointer-arith
AM_CFLAGS+= 			    -Wbad-function-cast -Wcast-align -Wwrite-strings
AM_CFLAGS+= 			    -Wsign-compare -Wstrict-prototypes -Wno-conversion
AM_CFLAGS+= 			    -Wno-missing-prototypes -Wno-missing-declarations -Wmissing-noreturn
AM_CFLAGS+= 			    -Wredundant-decls -Wnested-externs -Winline -Wlong-long

$(IDL4):
	(cd $(top_builddir)/src && $(MAKE))

$(top_builddir)/src/test/glue/v4-ia64/libtest_v4_ia64.a:
	(cd $(top_builddir)/src/test/glue/v4-ia64 && $(MAKE))

v4_ia64_client.h: $(IDL) $(IDL4)
	$(IDL4) $(test_v4_ia64_IDLFLAGS) -c -h $@ $(IDL) 

v4_ia64_server.h: $(IDL) $(IDL4)
	$(IDL4) $(test_v4_ia64_IDLFLAGS) -s -h $@ $(IDL) 

v4_ia64.c: v4_ia64_client.h v4_ia64_server.h $(IDL) $(IDL4)
	$(IDL4) $(test_v4_ia64_IDLFLAGS) -t -o $@ $(IDL)

diff: v4_ia64_client.h v4_ia64_server.h v4_ia64.c
	diff reference/v4_ia64_client.h v4_ia64_client.h || true
	diff reference/v4_ia64_server.h v4_ia64_server.h || true
	diff reference/v4_ia64.c v4_ia64.c || true
