# The Emscripten version the wasm builds are tested with. Emscripten releases
# often change behaviour, so builds stop if emcc is a different version.
# To build with another one anyway (e.g. to try a new release), set
# ALLOW_ANY_EMCC=1. To change the pinned version, edit this line, and check
# the build and the page still work.
#
# Included by this Makefile and the top-level one (for nspark). Make the
# emcc rules depend on it order-only (`| check-emcc`), so it runs first even
# with -j, without making anything out of date.
EMSCRIPTEN_VERSION := 7.0.0

.PHONY: check-emcc
check-emcc:
	@v=$$(emcc -dumpversion 2>/dev/null); \
	if [ "$$v" != "$(EMSCRIPTEN_VERSION)" ]; then \
	  if [ -n "$(ALLOW_ANY_EMCC)" ]; then \
	    echo "warning: emcc is '$${v:-not found}', not $(EMSCRIPTEN_VERSION) (ALLOW_ANY_EMCC is set)" >&2; \
	  else \
	    echo "error: emcc is '$${v:-not found}', but this build needs Emscripten $(EMSCRIPTEN_VERSION)." >&2; \
	    echo "  emsdk install $(EMSCRIPTEN_VERSION) && emsdk activate $(EMSCRIPTEN_VERSION) && source emsdk_env.sh" >&2; \
	    echo "  or set ALLOW_ANY_EMCC=1 to build with it anyway." >&2; \
	    exit 1; \
	  fi; \
	fi
