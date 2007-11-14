## Process this file with automake to produce Makefile.in

noinst_LIBRARIES=	libcxx.a
libcxx_a_SOURCES=	context.cc parser.yy scanner.ll symtab.cc

AM_LFLAGS=		-o$(LEX_OUTPUT_ROOT).c
AM_YFLAGS=		-pcpp -d

CLEANFILES=		parser.cc parser.h scanner.cc

dist-hook:
	rm -f $(distdir)/parser.h $(distdir)/parser.cc $(distdir)/scanner.cc
