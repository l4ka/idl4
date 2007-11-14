## Process this file with automake to produce Makefile.in

APIDIR                      = ../../api/v4
ARCHDIR                     = ../../arch/ia32
GENDIR                      = ../../arch/generic

noinst_LIBRARIES            = libtest_v4_ia32.a
libtest_v4_ia32_a_SOURCES   = $(APIDIR)/fpage.cc
libtest_v4_ia32_a_SOURCES  += $(APIDIR)/main.cc $(ARCHDIR)/crt0-ia32.S $(ARCHDIR)/spinlock.c
libtest_v4_ia32_a_SOURCES  += $(GENDIR)/corba.c $(GENDIR)/malloc.c $(GENDIR)/printf.c
libtest_v4_ia32_a_SOURCES  += $(GENDIR)/random.c $(GENDIR)/string.c putc.c

AM_CPPFLAGS                 = -DCONFIG_VERSION_V4 -DCONFIG_ARCH_IA32 \
                              -DIDL4_API=v4 -DIDL4_ARCH=ia32 -Wall -W -O2 -fno-builtin\
			      -I$(top_srcdir)/src/include/user -fno-exceptions \
			      -I$(PISTACHIO_IA32_DIR)/include
