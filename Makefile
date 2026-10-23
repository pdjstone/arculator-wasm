# Replaces autotools for emscripten build.

SHELL := bash
BUILD_TAG := $(shell echo `git rev-parse --short HEAD`-`[[ -n $$(git status -s) ]] && echo 'dirty' || echo 'clean'` on `date --rfc-3339=seconds`)

CC             := gcc
CFLAGS         := -D_REENTRANT -Wall -Werror -DBUILD_TAG="${BUILD_TAG}"
# podules-unix.c looks for external podules as <name>/<name>$(SO_EXT)
CFLAGS         += -DSO_EXT='".so"'
CFLAGS_WASM    := -sUSE_ZLIB=1
LINKFLAGS      := -lz -lm -ldl
EM_MODULE_JS_API  := logReadFiles,locateFile,monitorRunDependencies,noInitialRun,onRuntimeInitialized,preRun,postRun,print,printErr,setStatus,canvas
EM_RUNTIME_API := ccall,FS,callMain,HEAPU8,HEAPU32
LINKFLAGS_WASM := -sMODULARIZE -sEXPORT_ES6 -sEXPORT_NAME=ArculatorWasm -sSHARED_MEMORY -sINCOMING_MODULE_JS_API=${EM_MODULE_JS_API} -sALLOW_MEMORY_GROWTH=1 -sFORCE_FILESYSTEM -sEXPORTED_RUNTIME_METHODS=${EM_RUNTIME_API} -lidbfs.js 
LINKFLAGS_WASM += --js-library js/browser_keys.js --js-library js/video.js --js-library js/audio.js
DATA           := ddnoise
ifdef DEBUG
  CFLAGS += -D_DEBUG -DDEBUG_LOG -O3
#  --source-map-base not needed if .map is in the same dir as .wasm
  LINKFLAGS_WASM += -gsource-map -g2
  BUILD_TAG +=  (DEBUG)
  $(info ❗BUILD_TAG="${BUILD_TAG}")
  DATA += roms/riscos311/ros311 roms/arcrom_ext cmos arc.cfg
else
  CFLAGS += -O3 -flto
  LINKFLAGS += -flto
  $(info ❗BUILD_TAG="${BUILD_TAG}")
  $(info ❗Re-run make with DEBUG=1 if you want a debug build)
endif


OBJS := 82c711 82c711_fdc \
	arm bmu cmos colourcard config cp15 \
	debugger debugger_swis ddnoise \
	disc disc_adf disc_mfm_common disc_hfe \
	ds2401 eterna fdi2raw fpa g16 g332 \
	hostfs ide ide_a3in ide_config ide_idea jscli \
	ide_riscdev ide_zidefs ide_zidefs_a3k ide_hccs_a3k \
	input_emscripten ioc ioeb joystick keyboard \
	lc main mem memc podules printer \
	riscdev_hdfc romload sound_emscripten \
	st506 st506_akd52 timer vidc video_emscripten wd1770 \
	joystick_emscripten hostfs-unix podules-unix \
	emscripten_main emscripten-console hostfs_emscripten
# The wasm build does its sound filtering and resampling in an AudioWorklet
OBJS_NATIVE := $(addprefix build/native/,$(addsuffix .o,${OBJS} sound))
OBJS_WASM   := $(addprefix build/wasm/,$(addsuffix .o,${OBJS} sound_worklet))

######################################################################
all:	native wasm extnrom

clean:
	rm -rf build

######################################################################
native:	build/native/arculator

build/native/arculator: ${OBJS_NATIVE}
	${CC} ${OBJS_NATIVE} -o $@ ${LINKFLAGS}

build/native/%.o: src/%.c
	@mkdir -p $(@D)
	${CC} -c ${CFLAGS} $< -o $@

######################################################################
wasm:	$(addprefix build/wasm/arculator.,js wasm)

build/wasm/arculator.wasm: build/wasm/arculator.js
build/wasm/arculator.js: ${OBJS_WASM} js/browser_keys.js js/video.js js/audio.js
	emcc ${LINKFLAGS} ${LINKFLAGS_WASM} ${OBJS_WASM} --embed-file ddnoise -o $@

# build/wasm/arculator.data.js: build/wasm/arculator.data
# build/wasm/arculator.data: ${DATA}
# 	${EMSDK}/upstream/emscripten/tools/file_packager $@ --js-output=$@.js --preload $^

build/wasm/%.o: src/%.c
	@mkdir -p $(@D)
	emcc -c ${CFLAGS} ${CFLAGS_WASM} $< -o $@

######################################################################
# Extension ROM with the HostFS and JSCLI support modules. The modules are only
# reassembled (needs arm-none-eabi binutils) when their source changes.
PYTHON := python3
EXTNROM_MODULES := src/hostfs/hostfs,ffa src/hostfs/hostfsfiler,ffa \
	src/jscli/jscli,ffa src/jscli/jscliwimp,ffa

.PHONY: extnrom
extnrom: roms/arcrom_ext

roms/arcrom_ext: scripts/make_extnrom.py ${EXTNROM_MODULES}
	${PYTHON} scripts/make_extnrom.py -o $@ ${EXTNROM_MODULES}

src/hostfs/%,ffa: src/hostfs/%.s
	$(MAKE) -C src/hostfs $*,ffa

src/jscli/%,ffa: src/jscli/%.s src/jscli/jscli-defs.s
	$(MAKE) -C src/jscli $*,ffa

######################################################################
roms/riscos311/ros311:
	curl -s http://b-em.bbcmicro.com/arculator/Arculator_V2.2_Linux.tar.gz | tar xz roms

