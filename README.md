Arculator WASM
==============

A port of [Arculator](https://github.com/sarah-walker-pcem/arculator) (an [Acorn Archimedes](https://en.wikipedia.org/wiki/Acorn_Archimedes) emulator) to WebAssembly using Emscripten.

What works:

* Graphics
* Sound
* Mouse
* Keyboard
* HostFS (via Emscripten MemFS)
* Loading disc images (via MemFS)

Not working or tested:

* Podules
* Hard drives

To build and test the Emscripten version:

* Install [Emscripten](https://emscripten.org/docs/getting_started/downloads.html) which contains the whole build environment
* Run `make -j8 serve`
* (or `make -j8 serve DEBUG=1` if you want a slower debug build)
* Open [http://localhost:8000/build/wasm/arculator.html](http://localhost:8000/build/arculator.html) to see the default Emscripten front-end which should boot RISC OS 3.

You can also build a native equivalent by running `make -j8 native DEBUG=1`.

We're working on a better front-end at the [Archimedes Live](https://github.com/pdjstone/archimedes-live) project. Join us!

Upstream Arculator
------------------

Licensed under GPL v2.0, see [COPYING](COPYING) for more details.

### Upstream contribution policy
AI generated/assisted code is not allowed in Arculator. Pull Requests with AI generated code will be rejected. Code subsequently discovered to be AI generated/assisted will be removed without warning.
