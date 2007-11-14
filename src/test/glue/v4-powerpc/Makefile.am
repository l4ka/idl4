## Process this file with automake to produce Makefile.in

CC                          = $(POWERPC_PREFIX)gcc
CXX                         = $(POWERPC_PREFIX)gcc
AM_CFLAGS                   = 
AM_CXXFLAGS                 = -fno-exceptions -O2
LD                          = $(POWERPC_PREFIX)ld
AR                          = $(POWERPC_PREFIX)ar
RANLIB                      = $(POWERPC_PREFIX)ranlib

APIDIR                      = ../../api/v4
ARCHDIR                     = ../../arch/powerpc
GENDIR                      = ../../arch/generic

noinst_LIBRARIES            = libtest_v4_powerpc.a
libtest_v4_powerpc_a_SOURCES  = $(APIDIR)/fpage.cc $(APIDIR)/main.cc
libtest_v4_powerpc_a_SOURCES += $(ARCHDIR)/crt0-powerpc.S $(ARCHDIR)/io.c $(ARCHDIR)/spinlock.c
libtest_v4_powerpc_a_SOURCES += $(GENDIR)/malloc.c $(GENDIR)/printf.c $(GENDIR)/corba.c
libtest_v4_powerpc_a_SOURCES += $(GENDIR)/random.c $(GENDIR)/string.c $(GENDIR)/tools.c

AM_CPPFLAGS                 = -DCONFIG_VERSION_V4 -DCONFIG_ARCH_POWERPC \
                              -DIDL4_API=v4 -DIDL4_ARCH=powerpc -Wall -W -Wno-format\
			      -I$(top_srcdir)/src/include/user -D__ARCH__=powerpc -D__API__=v4 -freg-struct-return\
			      -I. -I$(PISTACHIO_POWERPC_DIR)/include
