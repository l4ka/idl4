## Process this file with automake to produce Makefile.in

if WITH_TESTSUITE
SUB_TEST=	test
endif

if WITH_EXAMPLES
SUB_EXAMPLES=	weaver
endif

SUBDIRS= $(SUB_TEST) $(SUB_EXAMPLES)

EXTRA_DIST = chacmos

dist-hook:
	rm -rf `find $(distdir)/chacmos -name CVS`
