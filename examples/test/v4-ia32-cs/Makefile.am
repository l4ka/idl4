## Process this file with automake to produce Makefile.in

IDL4=				    $(top_builddir)/src/idl4
IDLFLAGS=			    -dtest -dparanoid -fno-fastcall -mc++
TESTDIR=                            $(top_srcdir)/examples/test
IDL=				    $(TESTDIR)/idl/test.idl
LINK_ADDR=			    0x00A00000

test_v4_ia32_cs_BIN=		    test_v4_ia32_cs
nodist_test_v4_ia32_cs_SOURCES=	    v4_ia32_cs.c
test_v4_ia32_cs_IDLFLAGS=	    $(IDLFLAGS) -iv4 -pia32 -DV4 -DCONFIG_API_V4
test_v4_ia32_cs_IDLFLAGS+=	    -DCONFIG_VERSION_V4 -DCONFIG_ARCH_IA32 
test_v4_ia32_cs_IDLFLAGS+=	    -I$(PISTACHIO_IA32_DIR)/include
test_v4_ia32_cs_LDADD=		    $(top_builddir)/src/test/glue/v4-ia32/crt0-ia32.o \
				    -L$(PISTACHIO_IA32_DIR)/lib/ -ll4 \
				    -L$(top_builddir)/src/test/glue/v4-ia32/ -ltest_v4_ia32
test_v4_ia32_cs_DEPENDENCIES=	    $(top_builddir)/src/test/glue/v4-ia32/libtest_v4_ia32.a
test_v4_ia32_cs_CFLAGS=		    -fomit-frame-pointer -DCONFIG_VERSION_V4 -DCONFIG_API_V4\
				    -I$(PISTACHIO_IA32_DIR)/include -DCONFIG_ARCH_IA32
test_v4_ia32_cs_LDFLAGS=	    -N -Ttext=$(LINK_ADDR) --defsym __textstart=$(LINK_ADDR)
test_v4_ia32_cs_LINK=		    $(LD) -o test_v4_ia32_cs

bindir=				    $(TESTSUITE_DIR)
bin_PROGRAMS=			    $(test_v4_ia32_cs_BIN)

CLEANFILES=			    v4_ia32_cs_client.h v4_ia32_cs_server.h v4_ia32_cs.c

INCLUDES=			    -I$(TESTDIR)/idl -I$(top_srcdir)/src/include/user -I$(TESTDIR)
AM_CFLAGS=			    -Wall -nostdinc -fno-builtin
AM_CFLAGS+=			    -march=i686 -mcpu=i686 -funsigned-char
AM_CFLAGS+=			    -W -Wno-unused -funsigned-bitfields 
AM_CFLAGS+=			    -fomit-frame-pointer -xc++ -fno-exceptions
AM_CFLAGS+= 			    -Wtraditional -Wshadow -Wpointer-arith
AM_CFLAGS+= 			    -Wcast-align -Wwrite-strings
AM_CFLAGS+= 			    -Wsign-compare -Wstrict-prototypes -Wconversion
AM_CFLAGS+= 			    -Wmissing-prototypes -Wmissing-noreturn
AM_CFLAGS+= 			    -Wredundant-decls -Winline -Wlong-long

$(IDL4):
	(cd $(top_builddir)/src && $(MAKE))

$(top_builddir)/src/test/glue/v4-ia32/libtest_v4_ia32.a:
	(cd $(top_builddir)/src/test/glue/v4-ia32 && $(MAKE) libtest_v4_ia32.a)

$(top_builddir)/src/test/glue/v4-ia32/crt0-ia32.o:
	(cd $(top_builddir)/src/test/glue/v4-ia32/ && $(MAKE) crt0-ia32.o)

v4_ia32_cs_client.h: $(IDL) $(IDL4)
	$(IDL4) $(test_v4_ia32_cs_IDLFLAGS) -c -h $@ $(IDL) 

v4_ia32_cs_server.h: $(IDL) $(IDL4)
	$(IDL4) $(test_v4_ia32_cs_IDLFLAGS) -s -h $@ $(IDL) 

v4_ia32_cs.c: v4_ia32_cs_client.h v4_ia32_cs_server.h $(IDL) $(IDL4)
	$(IDL4) $(test_v4_ia32_cs_IDLFLAGS) -t -o $@ $(IDL)

diff: v4_ia32_cs_client.h v4_ia32_cs_server.h v4_ia32_cs.c
	diff reference/v4_ia32_cs_client.h v4_ia32_cs_client.h || true
	diff reference/v4_ia32_cs_server.h v4_ia32_cs_server.h || true
	diff reference/v4_ia32_cs.c v4_ia32_cs.c || true
