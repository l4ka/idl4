## Process this file with automake to produce Makefile.in

CC                          = $(IA64_PREFIX)gcc
CXX                         = $(IA64_PREFIX)gcc
AM_CFLAGS                   = 
AM_CXXFLAGS                 = 
LD                          = $(IA64_PREFIX)ld
AR                          = $(IA64_PREFIX)ar
RANLIB                      = $(IA64_PREFIX)ranlib

APIDIR                      = ../../api/v4
ARCHDIR                     = ../../arch/ia64
GENDIR                      = ../../arch/generic

noinst_LIBRARIES            = libtest_v4_ia64.a
libtest_v4_ia64_a_SOURCES   = $(APIDIR)/fpage.cc $(APIDIR)/main.cc
libtest_v4_ia64_a_SOURCES  += $(ARCHDIR)/crt0-ia64.S $(ARCHDIR)/io.c $(ARCHDIR)/spinlock.c
libtest_v4_ia64_a_SOURCES  += $(GENDIR)/malloc.c $(GENDIR)/printf.c $(GENDIR)/corba.c
libtest_v4_ia64_a_SOURCES  += $(GENDIR)/random.c $(GENDIR)/string.c $(GENDIR)/tools.c

AM_CPPFLAGS                 = -DCONFIG_VERSION_V4 -DCONFIG_ARCH_IA64 \
                              -DIDL4_API=v4 -DIDL4_ARCH=ia64 -fno-builtin \
                              -D__ARCH__=ia64 -D__API__=v4 -O2 -fomit-frame-pointer \
			      -I$(top_srcdir)/src/include/user -Uia64 \
			      -I$(PISTACHIO_IA64_DIR)/include -I. -finline
