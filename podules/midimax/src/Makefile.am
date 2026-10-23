# Makefile.am for MIDI Max

AUTOMAKE_OPTIONS = subdir-objects

if BUILD_PODULES

lib_LTLIBRARIES = libmidimax.la
noinst_SCRIPTS =
CLEANFILES =

if OS_WINDOWS
CLEANFILES += $(noinst_SCRIPTS)
noinst_SCRIPTS += ../midimax$(SO_EXT)
../midimax$(SO_EXT): libmidimax.la
	cp .libs/libmidimax-0$(SO_EXT) ../midimax$(SO_EXT)
libmidimax_la_LDFLAGS = -no-undefined
else
CLEANFILES += $(noinst_SCRIPTS)
noinst_SCRIPTS += ../midimax$(SO_EXT)
../midimax$(SO_EXT): libmidimax.la
	cp .libs/libmidimax$(SO_EXT) ../midimax$(SO_EXT)
endif

amrefresh:	

# MIDI Max
libmidimax_la_SOURCES = ../../common/uart/16550.c midimax.c

libmidimax_la_CFLAGS = -I../../../src -I../../common/midi -I../../common/uart

if OS_WINDOWS
libmidimax_la_SOURCES += ../../common/midi/midi_win.c 
endif
if OS_LINUX
libmidimax_la_SOURCES += ../../common/midi/midi_alsa.c 
endif
if OS_MACOSX
libmidimax_la_SOURCES += ../../common/midi/midi_null.c 
endif

if OS_WINDOWS
libmidimax_la_LIBADD = -lwinmm
endif

endif
