# Makefile.am for Lark

AUTOMAKE_OPTIONS = subdir-objects

if BUILD_PODULES

lib_LTLIBRARIES = liblark.la
noinst_SCRIPTS =
CLEANFILES =

if OS_WINDOWS
CLEANFILES += $(noinst_SCRIPTS)
noinst_SCRIPTS += ../lark$(SO_EXT)
../lark$(SO_EXT): liblark.la
	cp .libs/liblark-0$(SO_EXT) ../lark$(SO_EXT)
liblark_la_LDFLAGS = -no-undefined
else
CLEANFILES += $(noinst_SCRIPTS)
noinst_SCRIPTS += ../lark$(SO_EXT)
../lark$(SO_EXT): liblark.la
	cp .libs/liblark$(SO_EXT) ../lark$(SO_EXT)
endif

amrefresh:

liblark_la_SOURCES = ../../common/uart/16550.c ad1848.c am7202a.c lark.c

if OS_WINDOWS
liblark_la_SOURCES += ../../common/midi/midi_win.c ../../common/sound/sound_wavein.c ../../common/sound/sound_out_sdl2.c
endif
if OS_LINUX
liblark_la_SOURCES += ../../common/midi/midi_alsa.c ../../common/sound/sound_alsain.c ../../common/sound/sound_out_sdl2.c
endif
if OS_MACOSX
liblark_la_SOURCES += ../../common/midi/midi_null.c ../../common/sound/sound_in_null.c ../../common/sound/sound_out_sdl2.c
endif

liblark_la_CFLAGS = $(SDL_CFLAGS) -I../../../src -I../../common/midi -I../../common/sound -I../../common/uart
liblark_la_LIBADD = @LIBS@

if OS_WINDOWS
liblark_la_LIBADD += -lwinmm
endif

endif
