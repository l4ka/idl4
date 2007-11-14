## Process this file with automake to produce Makefile.in

IDL4=				    $(top_builddir)/src/idl4
IDLFLAGS=			    -dtest -dparanoid
TESTDIR=                            $(top_srcdir)/examples/test
IDL=				    $(TESTDIR)/idl/test.idl
LINK_ADDR=			    0x00A00000

test_v2_ia32_generic_BIN=           test_v2_ia32_generic
nodist_test_v2_ia32_generic_SOURCES=v2_ia32_generic.c
test_v2_ia32_generic_IDLFLAGS=	    $(IDLFLAGS) -iv2 -pgeneric -w32 -DV2 -DCONFIG_API_V2
test_v2_ia32_generic_IDLFLAGS+=	    -DCONFIG_VERSION_V2 -DCONFIG_ARCH_IA32 
test_v2_ia32_generic_IDLFLAGS+=	    -I$(FIASCO_DIR)/l4/include/x86/ -I$(FIASCO_DIR)/l4/include/x86/l4v2/ -D__GNUC__
test_v2_ia32_generic_LDADD=         $(top_builddir)/src/test/glue/v2-ia32/crt0-ia32.o \
				    -L$(top_builddir)/src/test/glue/v2-ia32/ -ltest_v2_ia32
test_v2_ia32_generic_DEPENDENCIES=  $(top_builddir)/src/test/glue/v2-ia32/libtest_v2_ia32.a
test_v2_ia32_generic_CFLAGS=        -I$(FIASCO_DIR)/l4/include/x86/ -I$(FIASCO_DIR)/l4/include/x86/l4v2/
test_v2_ia32_generic_LDFLAGS=       -N -Ttext=$(LINK_ADDR) --defsym __textstart=$(LINK_ADDR)
test_v2_ia32_generic_LINK=          $(LD) -o test_v2_ia32_generic

bindir=				    $(TESTSUITE_DIR)
bin_PROGRAMS=			    $(test_v2_ia32_generic_BIN)

CLEANFILES=			    v2_ia32_generic_client.h v2_ia32_generic_server.h v2_ia32_generic.c

INCLUDES=			    -I$(TESTDIR)/idl -I$(top_srcdir)/src/include/user -I$(TESTDIR)
AM_CFLAGS=			    -Wall -nostdinc -ffreestanding
AM_CFLAGS+=			    -march=i686 -mcpu=i686 -O9 -funsigned-char
AM_CFLAGS+=			    -W -Wno-unused -funsigned-bitfields 
AM_CFLAGS+=			    -fomit-frame-pointer 
AM_CFLAGS+= 			    -Wno-traditional -Wshadow -Wpointer-arith
AM_CFLAGS+= 			    -Wbad-function-cast -Wcast-align -Wwrite-strings
AM_CFLAGS+= 			    -Wsign-compare -Wstrict-prototypes -Wno-conversion
AM_CFLAGS+= 			    -Wno-missing-prototypes -Wno-missing-declarations -Wmissing-noreturn
AM_CFLAGS+= 			    -Wredundant-decls -Wnested-externs -Winline -Wlong-long

$(IDL4):
	(cd $(top_builddir)/src && $(MAKE))

$(top_builddir)/src/test/glue/v2-ia32/libtest_v2_ia32.a:
	(cd $(top_builddir)/src/test/glue/v2-ia32 && $(MAKE) libtest_v2_ia32.a)

$(top_builddir)/src/test/glue/v2-ia32/crt0-ia32.o:
	(cd $(top_builddir)/src/test/glue/v2-ia32/ && $(MAKE) crt0-ia32.o)

v2_ia32_generic_client.h: $(IDL) $(IDL4)
	$(IDL4) $(test_v2_ia32_generic_IDLFLAGS) -c -h $@ $(IDL) 

v2_ia32_generic_server.h: $(IDL) $(IDL4)
	$(IDL4) $(test_v2_ia32_generic_IDLFLAGS) -s -h $@ $(IDL) 

v2_ia32_generic.c: v2_ia32_generic_client.h v2_ia32_generic_server.h $(IDL) $(IDL4)
	$(IDL4) $(test_v2_ia32_generic_IDLFLAGS) -t -o $@ $(IDL)

diff: v2_ia32_generic_client.h v2_ia32_generic_server.h v2_ia32_generic.c
	diff reference/v2_ia32_generic_client.h v2_ia32_generic_client.h || true
	diff reference/v2_ia32_generic_server.h v2_ia32_generic_server.h || true
	diff reference/v2_ia32_generic.c v2_ia32_generic.c || true
