## Process this file with automake to produce Makefile.in

SUBDIRS=	aoi arch base be cast fe test include

IDL4LIBS=	$(top_builddir)/src/aoi/libaoi.a \
		$(top_builddir)/src/arch/libarch.a \
		$(top_builddir)/src/arch/v2/libv2.a \
		$(top_builddir)/src/arch/v2/ia32/libv2i.a \
		$(top_builddir)/src/arch/v4/libv4.a \
		$(top_builddir)/src/arch/v4/ia32/libv4i.a \
		$(top_builddir)/src/arch/v4/ia64/libv4m.a \
		$(top_builddir)/src/arch/x0/libx0.a \
		$(top_builddir)/src/arch/x0/ia32/libx0i.a \
		$(top_builddir)/src/base/libui.a \
		$(top_builddir)/src/base/lib/libbase.a \
		$(top_builddir)/src/be/libbe.a \
		$(top_builddir)/src/be/ops/libops.a \
		$(top_builddir)/src/be/types/libtypes.a \
		$(top_builddir)/src/cast/libcast.a \
		$(top_builddir)/src/fe/c++/libcxx.a \
		$(top_builddir)/src/fe/idl/libidl.a \
		$(top_builddir)/src/fe/lib/libcompiler.a

bin_PROGRAMS=	idl4
idl4_SOURCES=	base/version.cc
idl4_DEPENDENCIES= $(IDL4LIBS)
idl4_LDADD=	-L$(top_builddir)/src/base -lui \
		-L$(top_builddir)/src/base/lib -lbase \
		-L$(top_builddir)/src/aoi -laoi \
		-L$(top_builddir)/src/fe/idl -lidl \
		-L$(top_builddir)/src/fe/c++ -lcxx \
		-L$(top_builddir)/src/fe/lib -lcompiler \
		-L$(top_builddir)/src/be -lbe \
		-L$(top_builddir)/src/cast -lcast \
		-L$(top_builddir)/src/arch/x0 -lx0 \
		-L$(top_builddir)/src/arch/x0/ia32 -lx0i \
		-L$(top_builddir)/src/arch/v2 -lv2 \
		-L$(top_builddir)/src/arch/v2/ia32 -lv2i \
		-L$(top_builddir)/src/arch/v4 -lv4 \
		-L$(top_builddir)/src/arch/v4/ia32 -lv4i \
		-L$(top_builddir)/src/arch/v4/ia64 -lv4m \
		-L$(top_builddir)/src/arch -larch \
		-L$(top_builddir)/src/be/types -ltypes \
		-L$(top_builddir)/src/be/ops -lops -lstdc++

AM_CPPFLAGS=	-D__USER__="\"$(USER)@`hostname`\"" \
		-D__GCCVERSION__="\"`$(CXX) -v 2>&1 | tail -1`\"" \
		-D__CPP__="\"`which cpp`\""

EXTRA_DIST=	include/check.h \
		include/aoi.h \
		include/base.h \
		include/be.h \
		include/cast.h \
		include/arch/dummy.h \
		include/arch/v4.h \
		include/arch/x0.h \
		include/arch/v2.h \
		include/cross.h \
		include/dump.h \
		include/globals.h \
		include/ms.h \
		include/ops.h \
		include/fe/compiler.h \
		include/fe/c++.h \
		include/fe/errors.h \
		include/fe/idl.h \
                include/user/idl4/test.h \
                include/user/idl4/api/v2/test.h \
                include/user/idl4/api/x0/test.h \
                include/user/idl4/api/v4/test.h \
                include/user/idl4/Makefile.in

wc:
	(find . \( -name "*.[chS]" -or -name "*.cc" -or -name "*.ll" -or -name "*.yy" \) -and -not -name "*getopt*" -and -not -name "parser.cc" -and -not -name "scanner.cc" -and -not -name "parser.h") | xargs wc -l
