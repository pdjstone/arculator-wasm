# Makefile.am for AKA05

AUTOMAKE_OPTIONS = subdir-objects

if BUILD_PODULES

lib_LTLIBRARIES = libaka05.la
noinst_SCRIPTS =
CLEANFILES =

if OS_WINDOWS
CLEANFILES += $(noinst_SCRIPTS)
noinst_SCRIPTS += ../aka05$(SO_EXT)
../aka05$(SO_EXT): libaka05.la
	cp .libs/libaka05-0$(SO_EXT) ../aka05$(SO_EXT)
libaka05_la_LDFLAGS = -no-undefined
else
CLEANFILES += $(noinst_SCRIPTS)
noinst_SCRIPTS += ../aka05$(SO_EXT)
../aka05$(SO_EXT): libaka05.la
	cp .libs/libaka05$(SO_EXT) ../aka05$(SO_EXT)
endif

amrefresh:	

libaka05_la_SOURCES = aka05.c

libaka05_la_CFLAGS = -I../../../src
libaka05_la_LIBADD = @LIBS@

endif
