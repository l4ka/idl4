## Process this file with automake to produce Makefile.in

APIDIR                      = ../../api/x0
ARCHDIR                     = ../../arch/arm
GENDIR                      = ../../arch/generic
CC                          = $(ARM_PREFIX)gcc
LD                          = $(ARM_PREFIX)ld
AR                          = $(ARM_PREFIX)ar
RANLIB                      = $(ARM_PREFIX)ranlib

noinst_LIBRARIES            = libtest_x0_arm.a
libtest_x0_arm_a_SOURCES    = $(APIDIR)/fpage.c $(APIDIR)/main.c $(APIDIR)/pager.c
libtest_x0_arm_a_SOURCES   += $(ARCHDIR)/crt0-arm.S $(ARCHDIR)/spinlock.c
libtest_x0_arm_a_SOURCES   += $(GENDIR)/malloc.c $(GENDIR)/printf.c $(GENDIR)/corba.c
libtest_x0_arm_a_SOURCES   += $(GENDIR)/random.c $(GENDIR)/string.c $(GENDIR)/tools.c putc.c

AM_CPPFLAGS                 = -DCONFIG_VERSION_X0 -DCONFIG_ARCH_ARM \
                              -DIDL4_API=x0 -Uarm -DIDL4_ARCH=arm -Wall -W -Wno-unused \
			      -I$(top_srcdir)/src/include/user \
			      -I$(HAZELNUT_DIR)/apps/include

fpage.o main.o pager.o: upd_link

upd_link:
	(cd $(HAZELNUT_DIR)/apps/include/l4;rm arch;ln -s arm/ arch)
