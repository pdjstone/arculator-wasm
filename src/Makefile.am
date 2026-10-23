# Makefile.am for Arculator

AUTOMAKE_OPTIONS = subdir-objects
bin_PROGRAMS = arculator
noinst_SCRIPTS =
CLEANFILES =

if OS_WINDOWS
CLEANFILES += $(noinst_SCRIPTS)
noinst_SCRIPTS += ../arculator.exe
../arculator.exe: arculator.exe
	cp .libs/arculator.exe ..
else
CLEANFILES += $(noinst_SCRIPTS)
noinst_SCRIPTS += ../arculator
../arculator: arculator
	cp arculator ..
endif

amrefresh:

# Special wxWidgets files
wx.res: wx.rc
	$(WX_RESCOMP) -i wx.rc --input-format=rc -o wx.res -O coff

wx-resources.cc : arculator.xrc
	-wxrc -c arculator.xrc -o wx-resources.cc

# Arculator
arculator_SOURCES = 82c711.c 82c711_fdc.c arm.c bmu.c cmos.c colourcard.c config.c cp15.c ddnoise.c \
 debugger.c debugger_swis.c disc.c disc_adf.c disc_apd.c disc_fdi.c disc_hfe.c disc_jfd.c disc_mfm_common.c disc_scp.c ds2401.c \
 eterna.c fdi2raw.c fpa.c g16.c g332.c hostfs.c ide.c ide_a3in.c ide_config.c ide_hccs_a3k.c ide_idea.c \
 ide_riscdev.c ide_zidefs.c ide_zidefs_a3k.c ioc.c ioeb.c joystick.c keyboard.c lc.c main.c \
 mem.c memc.c podules.c printer.c riscdev_hdfc.c romload.c sound.c sound_sdl2.c st506.c st506_akd52.c \
 timer.c vidc.c wd1770.c wx-app.cc wx-config.cc wx-config_sel.cc wx-hd_conf.cc wx-console.cc \
 wx-hd_new.cc wx-joystick-config.cc wx-main.cc wx-podule-config.cc wx-resources.cc wx-sdl2-joystick.c

arculator_CFLAGS = $(WX_CFLAGS) $(SDL_CFLAGS) $(X_CFLAGS) -O0 -g3
arculator_CXXFLAGS = $(WX_CXXFLAGS) $(SDL_CFLAGS) $(X_CFLAGS) -O0 -g3
arculator_LDADD = @LIBS@

if OS_WINDOWS
arculator_SOURCES += hostfs-win.c input_sdl2.c podules-win.c video_sdl2.c wx-win32.c
else
if UI_WX
arculator_SOURCES += input_wx.c video_wx.c wx-mainwindow.cc
else
arculator_SOURCES += input_sdl2.c video_sdl2.c wx-sdl2.c
endif
arculator_SOURCES += hostfs-unix.c podules-unix.c
arculator_LDADD += -ldl -lX11
endif

