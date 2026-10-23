# Makefile.am for Ultimate CD-ROM

AUTOMAKE_OPTIONS = subdir-objects

if BUILD_PODULES

lib_LTLIBRARIES = libultimatecdrom.la
noinst_SCRIPTS =
CLEANFILES =

if OS_WINDOWS
CLEANFILES += $(noinst_SCRIPTS)
noinst_SCRIPTS += ../ultimatecdrom$(SO_EXT)
../ultimatecdrom$(SO_EXT): libultimatecdrom.la
	cp .libs/libultimatecdrom-0$(SO_EXT) ../ultimatecdrom$(SO_EXT)
libultimatecdrom_la_LDFLAGS = -no-undefined
else
CLEANFILES += $(noinst_SCRIPTS)
noinst_SCRIPTS += ../ultimatecdrom$(SO_EXT)
../ultimatecdrom$(SO_EXT): libultimatecdrom.la
	cp .libs/libultimatecdrom$(SO_EXT) ../ultimatecdrom$(SO_EXT)
endif

amrefresh:

libultimatecdrom_la_SOURCES = mitsumi.c ultimatecdrom.c ../../common/sound/sound_out_sdl2.c

if OS_WINDOWS
libultimatecdrom_la_SOURCES += ../../common/cdrom/cdrom-windows-ioctl.c
endif
if OS_LINUX
libultimatecdrom_la_SOURCES += ../../common/cdrom/cdrom-linux-ioctl.c
endif
if OS_MACOSX
libultimatecdrom_la_SOURCES += ../../common/cdrom/cdrom-osx-ioctl.c
endif

libultimatecdrom_la_CFLAGS = $(SDL_CFLAGS) -I../../../src -I../../common/cdrom -I../../common/sound
libultimatecdrom_la_LIBADD = @LIBS@

endif
