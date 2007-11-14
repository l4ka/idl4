## Process this file with automake to produce Makefile.in

DIST_SUBDIRS=	src doc examples
SUBDIRS=	src
EXTRA_DIST=	depcomp
bin_SCRIPTS=	idl4-config

docs:
	(cd $(top_builddir)/doc && $(MAKE))

test:
	(cd $(top_builddir)/examples/test && $(MAKE))

weaver:
	(cd $(top_builddir)/examples/weaver && $(MAKE))

DISTCLEANFILES= config.log config.status 
MAINTAINERCLEANFILES= aclocal.m4 depcomp mkinstalldirs stamp-h.in stamp-h1 install-sh missing config.h.in configure Makefile.in compile $(PACKAGE)-$(VERSION).tar.gz

