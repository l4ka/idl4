## Process this file with automake to produce Makefile.in

CC                          = $(ALPHA_PREFIX)gcc
CXX                         = $(ALPHA_PREFIX)gcc
AM_CFLAGS                    = -mcpu=21064 -mno-fp-regs -mno-soft-float
AM_CXXFLAGS                  = -fno-exceptions -mcpu=21064 -mno-fp-regs -mno-soft-float
LD                          = $(ALPHA_PREFIX)ld
AR                          = $(ALPHA_PREFIX)ar
RANLIB                      = $(ALPHA_PREFIX)ranlib

APIDIR                      = ../../api/v4
ARCHDIR                     = ../../arch/alpha
GENDIR                      = ../../arch/generic

noinst_LIBRARIES            = libtest_v4_alpha.a
libtest_v4_alpha_a_SOURCES  = $(APIDIR)/fpage.cc $(APIDIR)/main.cc 
libtest_v4_alpha_a_SOURCES += $(ARCHDIR)/crt0-alpha.S $(ARCHDIR)/io.c $(ARCHDIR)/spinlock.c
libtest_v4_alpha_a_SOURCES += $(GENDIR)/malloc.c $(GENDIR)/printf.c $(GENDIR)/corba.c
libtest_v4_alpha_a_SOURCES += $(GENDIR)/random.c $(GENDIR)/string.c $(GENDIR)/tools.c
libtest_v4_alpha_a_SOURCES += $(ARCHDIR)/__divlu.S $(ARCHDIR)/__divqu.S $(ARCHDIR)/__remlu.S $(ARCHDIR)/__remqu.S

AM_CPPFLAGS                 = -DCONFIG_VERSION_V4 -DCONFIG_ARCH_ALPHA \
                              -DIDL4_API=v4 -DIDL4_ARCH=alpha -Wall -W -Wno-format\
			      -I$(top_srcdir)/src/include/user -D__ARCH__=alpha -D__API__=v4 -freg-struct-return\
			      -I. -I$(PISTACHIO_ALPHA_DIR)/include
