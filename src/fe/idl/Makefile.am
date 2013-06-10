## Process this file with automake to produce Makefile.in

noinst_LIBRARIES=	libidl.a
libidl_a_SOURCES=	context.cc import.cc parser.yy scanner.ll

AM_LFLAGS=		-o$(LEX_OUTPUT_ROOT).c
AM_YFLAGS=		-pidl -d

CLEANFILES=		parser.cc parser.hh scanner.cc

dist-hook:
	rm -f $(distdir)/parser.hh $(distdir)/parser.cc $(distdir)/scanner.cc
