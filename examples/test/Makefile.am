## Process this file with automake to produce Makefile.in

if WITH_HAZELNUT
if WITH_TOOLCHAIN_IA32
SUB_X0_IA32 = x0-ia32 x0-ia32-generic
endif
if WITH_TOOLCHAIN_ARM
SUB_X0_ARM = x0-arm-generic
endif
endif

if WITH_PISTACHIO_IA32
if WITH_TOOLCHAIN_IA32
SUB_V4_IA32 = v4-ia32 v4-ia32-generic v4-ia32-cs
endif
endif

if WITH_PISTACHIO_IA64
if WITH_TOOLCHAIN_IA64
SUB_V4_IA64 = v4-ia64 v4-ia64-generic
endif
endif

#if WITH_PISTACHIO_ARM
#if WITH_TOOLCHAIN_ARM
#SUB_V4_ARM = v4-arm
#endif
#endif

if WITH_PISTACHIO_MIPS64
if WITH_TOOLCHAIN_MIPS64
SUB_V4_MIPS64 = v4-mips64-generic
endif
endif

if WITH_PISTACHIO_ALPHA
if WITH_TOOLCHAIN_ALPHA
SUB_V4_ALPHA = v4-alpha-generic
endif
endif

if WITH_PISTACHIO_POWERPC
if WITH_TOOLCHAIN_POWERPC
SUB_V4_POWERPC = v4-powerpc-generic
endif
endif

if WITH_FIASCO
if WITH_TOOLCHAIN_IA32
SUB_V2_IA32 = v2-ia32 v2-ia32-generic
endif
endif

SUBDIRS= idl $(SUB_V2_IA32) $(SUB_X0_IA32) $(SUB_X0_ARM) $(SUB_V4_IA32) $(SUB_V4_IA64) $(SUB_V4_ALPHA) $(SUB_V4_MIPS64) $(SUB_V4_POWERPC)
