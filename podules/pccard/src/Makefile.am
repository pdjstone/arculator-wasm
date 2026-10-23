# Makefile.am for Aleph One 386/486PC Expansion Card

AUTOMAKE_OPTIONS = subdir-objects

if BUILD_PODULES

lib_LTLIBRARIES = libpccard.la
noinst_SCRIPTS =
CLEANFILES =

if OS_WINDOWS
CLEANFILES += $(noinst_SCRIPTS)
noinst_SCRIPTS += ../pccard$(SO_EXT)
../pccard$(SO_EXT): libpccard.la
	cp .libs/libpccard-0$(SO_EXT) ../pccard$(SO_EXT)
libpccard_la_LDFLAGS = -no-undefined
else
CLEANFILES += $(noinst_SCRIPTS)
noinst_SCRIPTS += ../pccard$(SO_EXT)
../pccard$(SO_EXT): libpccard.la
	cp .libs/libpccard$(SO_EXT) ../pccard$(SO_EXT)
endif

amrefresh:	

libpccard_la_SOURCES = pccard_podule.c
libpccard_la_SOURCES += pcem/808x.c pcem/386.c pcem/386_common.c pcem/386_dynarec.c pcem/cpu.c pcem/cpu_tables.c pcem/diva.c pcem/dma.c pcem/io.c pcem/lpt.c pcem/mem.c pcem/pc.c pcem/pic.c pcem/pit.c pcem/scamp.c pcem/serial.c pcem/timer.c pcem/x86seg.c pcem/x87.c pcem/x87_timings.c
libpccard_la_SOURCES += libco/libco.c

libpccard_la_CFLAGS = -I../../../src
libpccard_la_LIBADD = @LIBS@

endif
