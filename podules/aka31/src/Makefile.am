# Makefile.am for AKA31

AUTOMAKE_OPTIONS = subdir-objects

if BUILD_PODULES

lib_LTLIBRARIES = libaka31.la
noinst_SCRIPTS =
CLEANFILES =

if OS_WINDOWS
CLEANFILES += $(noinst_SCRIPTS)
noinst_SCRIPTS += ../aka31$(SO_EXT)
../aka31$(SO_EXT): libaka31.la
	cp .libs/libaka31-0$(SO_EXT) ../aka31$(SO_EXT)
libaka31_la_LDFLAGS = -no-undefined
else
CLEANFILES += $(noinst_SCRIPTS)
noinst_SCRIPTS += ../aka31$(SO_EXT)
../aka31$(SO_EXT): libaka31.la
	cp .libs/libaka31$(SO_EXT) ../aka31$(SO_EXT)
endif

amrefresh:

libaka31_la_SOURCES = aka31.c d71071l.c ../../common/scsi/hdd_file.c ../../common/scsi/scsi.c ../../common/scsi/scsi_cd.c ../../common/scsi/scsi_config.c ../../common/scsi/scsi_hd.c wd33c93a.c ../../common/sound/sound_out_sdl2.c

if OS_WINDOWS
libaka31_la_SOURCES += ../../common/cdrom/cdrom-windows-ioctl.c
endif
if OS_LINUX
libaka31_la_SOURCES += ../../common/cdrom/cdrom-linux-ioctl.c
endif
if OS_MACOSX
libaka31_la_SOURCES += ../../common/cdrom/cdrom-osx-ioctl.c
endif

libaka31_la_CFLAGS = $(SDL_CFLAGS) -I../../../src -I../../common/cdrom -I../../common/sound -I../../common/scsi
libaka31_la_LIBADD = @LIBS@

endif
