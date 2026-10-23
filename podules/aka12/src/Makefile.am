# Makefile.am for AKA12

AUTOMAKE_OPTIONS = subdir-objects

if BUILD_PODULES

lib_LTLIBRARIES = libaka12.la
noinst_SCRIPTS =
CLEANFILES =

if OS_WINDOWS
CLEANFILES += $(noinst_SCRIPTS)
noinst_SCRIPTS += ../aka12$(SO_EXT)
../aka12$(SO_EXT): libaka12.la
	cp .libs/libaka12-0$(SO_EXT) ../aka12$(SO_EXT)
libaka12_la_LDFLAGS = -no-undefined
else
CLEANFILES += $(noinst_SCRIPTS)
noinst_SCRIPTS += ../aka12$(SO_EXT)
../aka12$(SO_EXT): libaka12.la
	cp .libs/libaka12$(SO_EXT) ../aka12$(SO_EXT)
endif

amrefresh:	

libaka12_la_SOURCES = ../../common/misc/6522.c ../../common/uart/scc2691.c aka12.c

if OS_WINDOWS
libaka12_la_SOURCES += ../../common/midi/midi_win.c
endif
if OS_LINUX
libaka12_la_SOURCES += ../../common/midi/midi_alsa.c
endif
if OS_MACOSX
libaka12_la_SOURCES += ../../common/midi/midi_null.c
endif

libaka12_la_CFLAGS = -I../../../src -I../../common/midi -I../../common/misc -I../../common/uart
libaka12_la_LIBADD = @LIBS@

if OS_WINDOWS
libaka12_la_LIBADD += -lwinmm
endif

endif
