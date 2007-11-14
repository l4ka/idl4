## Process this file with automake to produce Makefile.in

APIDIR                      = ../../api/x0
ARCHDIR                     = ../../arch/ia32
GENDIR                      = ../../arch/generic

noinst_LIBRARIES            = libtest_x0_ia32.a
libtest_x0_ia32_a_SOURCES   = $(APIDIR)/fpage.c $(APIDIR)/main.c
libtest_x0_ia32_a_SOURCES  += $(APIDIR)/pager.c $(ARCHDIR)/crt0-ia32.S $(ARCHDIR)/spinlock.c
libtest_x0_ia32_a_SOURCES  += $(GENDIR)/malloc.c $(GENDIR)/printf.c $(GENDIR)/corba.c
libtest_x0_ia32_a_SOURCES  += $(GENDIR)/random.c $(GENDIR)/string.c putc.c

AM_CPPFLAGS                 = -DCONFIG_VERSION_X0 -DCONFIG_ARCH_IA32 \
                              -DIDL4_API=x0 -DIDL4_ARCH=ia32 -Wall -W \
			      -I$(top_srcdir)/src/include/user \
			      -I$(HAZELNUT_DIR)/apps/include

fpage.o main.o pager.o: upd_link

upd_link:
	(cd $(HAZELNUT_DIR)/apps/include/l4;rm arch;ln -s x86/ arch)
