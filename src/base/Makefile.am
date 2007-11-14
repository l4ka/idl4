## Process this file with automake to produce Makefile.in

SUBDIRS=		lib

if NEED_GETOPT_LONG
GETOPT_SRC=		getopt.c getopt1.c
endif

noinst_LIBRARIES=	libui.a
libui_a_SOURCES=	main.cc tools.cc $(GETOPT_SRC)

EXTRA_DIST=		getopt.c getopt1.c getopt_gnu.h
