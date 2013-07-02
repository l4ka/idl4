## Process this file with automake to produce Makefile.in

noinst_LIBRARIES=	libidl.a
libidl_a_SOURCES=	context.cc import.cc parser.yy scanner.ll
BUILT_SOURCES=		parser.h

AM_LFLAGS=		-o$(LEX_OUTPUT_ROOT).c
AM_YFLAGS=		-pidl -d

CLEANFILES=		parser.cc parser.h parser.hh scanner.cc

dist-hook:
	rm -f $(distdir)/parser.h $(distdir)/parser.hh $(distdir)/parser.cc $(distdir)/scanner.cc

.hh.h:
	cp $< $@