## Process this file with automake to produce Makefile.in

CC                          = $(MIPS64_PREFIX)gcc
CXX                         = $(MIPS64_PREFIX)gcc
AM_CFLAGS                   = 
AM_CXXFLAGS                 = -fno-exceptions
LD                          = $(MIPS64_PREFIX)ld
AR                          = $(MIPS64_PREFIX)ar
RANLIB                      = $(MIPS64_PREFIX)ranlib

APIDIR                      = ../../api/v4
ARCHDIR                     = ../../arch/mips64
GENDIR                      = ../../arch/generic

noinst_LIBRARIES            = libtest_v4_mips64.a
libtest_v4_mips64_a_SOURCES  = $(APIDIR)/fpage.cc $(APIDIR)/main.cc
libtest_v4_mips64_a_SOURCES += $(ARCHDIR)/crt0-mips64.S $(ARCHDIR)/io.c $(ARCHDIR)/spinlock.c
libtest_v4_mips64_a_SOURCES += $(GENDIR)/malloc.c $(GENDIR)/printf.c $(GENDIR)/corba.c
libtest_v4_mips64_a_SOURCES += $(GENDIR)/random.c $(GENDIR)/string.c $(GENDIR)/tools.c

AM_CPPFLAGS                 = -DCONFIG_VERSION_V4 -DCONFIG_ARCH_MIPS64 \
                              -DIDL4_API=v4 -DIDL4_ARCH=mips64 -Wall -W -Wno-format\
			      -I$(top_srcdir)/src/include/user -D__ARCH__=mips64 -D__API__=v4 -freg-struct-return\
			      -I. -I$(PISTACHIO_MIPS64_DIR)/include \
                              -EB -O3 -G 0 -mcpu=r4000 -mlong64 -mips3 -mno-abicalls -non_shared
