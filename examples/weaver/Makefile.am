## Process this file with automake to produce Makefile.in

WEAVERLIBS=	$(top_builddir)/src/base/lib/libbase.a \
		$(top_builddir)/src/cast/libcast.a \
		$(top_builddir)/src/fe/c++/libcxx.a \
		$(top_builddir)/src/fe/lib/libcompiler.a

bin_PROGRAMS=	weaver
weaver_DEPENDENCIES= $(WEAVERLIBS)
weaver_SOURCES=	main.cc

weaver_LDADD=	-L$(top_builddir)/src/base/lib -lbase \
		-L$(top_builddir)/src/fe/c++ -lcxx \
		-L$(top_builddir)/src/fe/lib -lcompiler \
		-L$(top_builddir)/src/cast -lcast

EXTRA_DIST=	example.c WHAT-IS-THIS

$(top_builddir)/src/base/lib/libbase.a:
	(cd $(top_builddir)/src/base/lib; make libbase.a)

$(top_builddir)/src/cast/libcast.a:
	(cd $(top_builddir)/src/cast; make libcast.a)
        
$(top_builddir)/src/fe/c++/libcxx.a:
	(cd $(top_builddir)/src/fe/c++; make libcxx.a)
        
$(top_builddir)/src/fe/lib/libcompiler.a:
	(cd $(top_builddir)/src/fe/lib; make libcompiler.a)
