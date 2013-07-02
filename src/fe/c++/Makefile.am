## Process this file with automake to produce Makefile.in

noinst_LIBRARIES=	libcxx.a
libcxx_a_SOURCES=	context.cc parser.yy scanner.ll symtab.cc
BUILT_SOURCES=		parser.h

AM_LFLAGS=		-o$(LEX_OUTPUT_ROOT).c
AM_YFLAGS=		-pcpp -d

CLEANFILES=		parser.cc parser.h parser.hh scanner.cc

dist-hook:
	rm -f $(distdir)/parser.h $(distdir)/parser.hh $(distdir)/parser.cc $(distdir)/scanner.cc

.hh.h:
	cp $< $@