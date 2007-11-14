## Process this file with automake to produce Makefile.in

IDL4=				    $(top_builddir)/src/idl4
IDLFLAGS=			    -dtest -dparanoid
TESTDIR=                            $(top_srcdir)/examples/test
IDL=				    $(TESTDIR)/idl/test.idl
LINK_ADDR=			    0x00A00000

test_x0_ia32_BIN=		    test_x0_ia32
nodist_test_x0_ia32_SOURCES= 	    x0_ia32.c
test_x0_ia32_IDLFLAGS=		    $(IDLFLAGS) -ix0 -pia32 -DX0 -DCONFIG_API_X0
test_x0_ia32_IDLFLAGS+=		    -DCONFIG_VERSION_X0 -DCONFIG_ARCH_IA32 
test_x0_ia32_IDLFLAGS+=		    -I$(HAZELNUT_DIR)/apps/include -ffastcall
test_x0_ia32_LDADD=		    $(top_builddir)/src/test/glue/x0-ia32/crt0-ia32.o \
				    -L$(top_builddir)/src/test/glue/x0-ia32/ -ltest_x0_ia32
test_x0_ia32_DEPENDENCIES=	    $(top_builddir)/src/test/glue/x0-ia32/libtest_x0_ia32.a
test_x0_ia32_CFLAGS=		    -fomit-frame-pointer -DCONFIG_VERSION_X0 -DCONFIG_API_X0\
				    -DCONFIG_ARCH_IA32
test_x0_ia32_LDFLAGS=		    -N -Ttext=$(LINK_ADDR) --defsym __textstart=$(LINK_ADDR)
test_x0_ia32_LINK=		    $(LD) -o test_x0_ia32

bindir=				    $(TESTSUITE_DIR)
bin_PROGRAMS=			    $(test_x0_ia32_BIN)

CLEANFILES=			    x0_ia32_client.h x0_ia32_server.h x0_ia32.c

INCLUDES=			    -I$(TESTDIR)/idl -I$(top_srcdir)/src/include/user -I$(TESTDIR) -I$(HAZELNUT_DIR)/apps/include
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

$(top_builddir)/src/test/glue/x0-ia32/libtest_x0_ia32.a:
	(cd $(top_builddir)/src/test/glue/x0-ia32 && $(MAKE) libtest_x0_ia32.a)

$(top_builddir)/src/test/glue/x0-ia32/crt0-ia32.o:
	(cd $(top_builddir)/src/test/glue/x0-ia32/ && $(MAKE) crt0-ia32.o)

x0_ia32_client.h: $(IDL) $(IDL4)
	(cd $(HAZELNUT_DIR)/apps/include/l4;rm -f arch;ln -s x86/ arch)
	$(IDL4) $(test_x0_ia32_IDLFLAGS) -c -h $@ $(IDL) 

x0_ia32_server.h: $(IDL) $(IDL4)
	(cd $(HAZELNUT_DIR)/apps/include/l4;rm -f arch;ln -s x86/ arch)
	$(IDL4) $(test_x0_ia32_IDLFLAGS) -s -h $@ $(IDL) 

x0_ia32.c: x0_ia32_client.h x0_ia32_server.h $(IDL) $(IDL4)
	(cd $(HAZELNUT_DIR)/apps/include/l4;rm -f arch;ln -s x86/ arch)
	$(IDL4) $(test_x0_ia32_IDLFLAGS) -t -o $@ $(IDL)

diff: x0_ia32_client.h x0_ia32_server.h x0_ia32.c
	diff reference/x0_ia32_client.h x0_ia32_client.h || true
	diff reference/x0_ia32_server.h x0_ia32_server.h || true
	diff reference/x0_ia32.c x0_ia32.c || true

xdiff: x0_ia32_client.h x0_ia32_server.h x0_ia32.c
	$(CPP) -P $(INCLUDES) reference/x0_ia32_client.h >old
	$(CPP) -P $(INCLUDES) x0_ia32_client.h >new
	diff old new || true
	$(CPP) -P $(INCLUDES) reference/x0_ia32_server.h >old
	$(CPP) -P $(INCLUDES) x0_ia32_server.h >new
	diff old new || true
	$(CPP) -P $(INCLUDES) reference/x0_ia32.c >old
	$(CPP) -P $(INCLUDES) x0_ia32.c >new
	diff old new || true
	rm -f old new
