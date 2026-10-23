# Makefile.am for Oak SCSI

AUTOMAKE_OPTIONS = subdir-objects

if BUILD_PODULES

lib_LTLIBRARIES = liboak_scsi.la
noinst_SCRIPTS =
CLEANFILES =

if OS_WINDOWS
CLEANFILES += $(noinst_SCRIPTS)
noinst_SCRIPTS += ../oak_scsi$(SO_EXT)
../oak_scsi$(SO_EXT): liboak_scsi.la
	cp .libs/liboak_scsi-0$(SO_EXT) ../oak_scsi$(SO_EXT)
liboak_scsi_la_LDFLAGS = -no-undefined
else
CLEANFILES += $(noinst_SCRIPTS)
noinst_SCRIPTS += ../oak_scsi$(SO_EXT)
../oak_scsi$(SO_EXT): liboak_scsi.la
	cp .libs/liboak_scsi$(SO_EXT) ../oak_scsi$(SO_EXT)
endif
amrefresh:

liboak_scsi_la_SOURCES = oak_scsi.c ncr5380.c ../../common/scsi/hdd_file.c ../../common/scsi/scsi.c ../../common/scsi/scsi_cd.c ../../common/scsi/scsi_config.c ../../common/scsi/scsi_hd.c ../../common/sound/sound_out_sdl2.c ../../common/eeprom/93c06.c

if OS_WINDOWS
liboak_scsi_la_SOURCES += ../../common/cdrom/cdrom-windows-ioctl.c
endif
if OS_LINUX
liboak_scsi_la_SOURCES += ../../common/cdrom/cdrom-linux-ioctl.c
endif
if OS_MACOSX
liboak_scsi_la_SOURCES += ../../common/cdrom/cdrom-osx-ioctl.c
endif

liboak_scsi_la_CFLAGS = $(SDL_CFLAGS) -I../../../src -I../../common/cdrom -I../../common/eeprom -I../../common/sound -I../../common/scsi
liboak_scsi_la_LIBADD = @LIBS@

endif
