## Process this file with automake to produce Makefile.in

IDL4=				    $(top_builddir)/src/idl4
IDLFLAGS=			    -dtest -dparanoid
TESTDIR=                            $(top_srcdir)/examples/test
IDL=				    $(TESTDIR)/idl/test.idl
LINK_ADDR=			    0x00A00000

test_v4_ia32_BIN=		    test_v4_ia32
nodist_test_v4_ia32_SOURCES= 	    v4_ia32.c
test_v4_ia32_IDLFLAGS=		    $(IDLFLAGS) -iv4 -pia32 -DV4 -DCONFIG_API_V4
test_v4_ia32_IDLFLAGS+=		    -DCONFIG_VERSION_V4 -DCONFIG_ARCH_IA32 
test_v4_ia32_IDLFLAGS+=		    -I$(PISTACHIO_IA32_DIR)/include -ffastcall
test_v4_ia32_LDADD=		    $(top_builddir)/src/test/glue/v4-ia32/crt0-ia32.o \
				    -L$(PISTACHIO_IA32_DIR)/lib/ -ll4 \
				    -L$(top_builddir)/src/test/glue/v4-ia32/ -ltest_v4_ia32
test_v4_ia32_DEPENDENCIES=	    $(top_builddir)/src/test/glue/v4-ia32/libtest_v4_ia32.a
test_v4_ia32_CFLAGS=		    -fomit-frame-pointer -DCONFIG_VERSION_V4 -DCONFIG_API_V4\
				    -I$(PISTACHIO_IA32_DIR)/include -DCONFIG_ARCH_IA32
test_v4_ia32_LDFLAGS=		    -N -Ttext=$(LINK_ADDR) --defsym __textstart=$(LINK_ADDR)
test_v4_ia32_LINK=		    $(LD) -o test_v4_ia32

bindir=				    $(TESTSUITE_DIR)
bin_PROGRAMS=			    $(test_v4_ia32_BIN)

CLEANFILES=			    v4_ia32_client.h v4_ia32_server.h v4_ia32.c

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

$(top_builddir)/src/test/glue/v4-ia32/libtest_v4_ia32.a:
	(cd $(top_builddir)/src/test/glue/v4-ia32 && $(MAKE) libtest_v4_ia32.a)

$(top_builddir)/src/test/glue/v4-ia32/crt0-ia32.o:
	(cd $(top_builddir)/src/test/glue/v4-ia32/ && $(MAKE) crt0-ia32.o)

v4_ia32_client.h: $(IDL) $(IDL4)
	$(IDL4) $(test_v4_ia32_IDLFLAGS) -c -h $@ $(IDL) 

v4_ia32_server.h: $(IDL) $(IDL4)
	$(IDL4) $(test_v4_ia32_IDLFLAGS) -s -h $@ $(IDL) 

v4_ia32.c: v4_ia32_client.h v4_ia32_server.h $(IDL) $(IDL4)
	$(IDL4) $(test_v4_ia32_IDLFLAGS) -t -o $@ $(IDL)

diff: v4_ia32_client.h v4_ia32_server.h v4_ia32.c
	diff reference/v4_ia32_client.h v4_ia32_client.h || true
	diff reference/v4_ia32_server.h v4_ia32_server.h || true
	diff reference/v4_ia32.c v4_ia32.c || true

