## Process this file with automake to produce Makefile.in

APIDIR                      = ../../api/v2
ARCHDIR                     = ../../arch/ia32
GENDIR                      = ../../arch/generic

noinst_LIBRARIES            = libtest_v2_ia32.a
libtest_v2_ia32_a_SOURCES   = $(APIDIR)/fpage.c $(APIDIR)/main.c
libtest_v2_ia32_a_SOURCES  += $(APIDIR)/pager.c $(ARCHDIR)/crt0-ia32.S $(ARCHDIR)/spinlock.c
libtest_v2_ia32_a_SOURCES  += $(GENDIR)/malloc.c $(GENDIR)/printf.c $(GENDIR)/corba.c
libtest_v2_ia32_a_SOURCES  += $(GENDIR)/random.c $(GENDIR)/string.c putc.c

AM_CPPFLAGS                 = -DCONFIG_VERSION_V2 -DCONFIG_ARCH_IA32 \
                              -DIDL4_API=v2 -DIDL4_ARCH=ia32 -Wall -W \
			      -I$(top_srcdir)/src/include/user \
			      -I$(FIASCO_DIR)/l4/include/x86/ \
			      -I$(FIASCO_DIR)/l4/include/x86/l4v2/

AM_CFLAGS                   = -DCONFIG_VERSION_V2 -DCONFIG_ARCH_IA32 \
			      -I$(top_srcdir)/src/include/user \
			      -I$(FIASCO_DIR)/l4/include/x86/ \
			      -I$(FIASCO_DIR)/l4/include/x86/l4v2/

