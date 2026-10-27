/**
 * Called from sound_dev_init in sound_emscripten.c. Hands the shared audio
 * rings (see src/sound_ring.h) to the main thread, which creates the
 * AudioContext and AudioWorklet that read from them. HEAPU8.buffer is a
 * SharedArrayBuffer because the module is built with -sSHARED_MEMORY, and the
 * static data the pointers refer to stays at the same address if the heap
 * grows. (Growing makes HEAPU8.buffer a new, larger SharedArrayBuffer, but
 * views on the old one still see the same memory, so the views made here stay
 * valid.)
 */
function browser_init_audio(ctrlPtr, resetPtr, eventPtr, recordPtr, ddPtr) {
    console.log('browser_init_audio', ctrlPtr, resetPtr, eventPtr, recordPtr, ddPtr);
    self.postMessage({
        type: 'AudioInit',
        heap: HEAPU8.buffer,
        ctrlPtr: ctrlPtr,
        resetPtr: resetPtr,
        eventPtr: eventPtr,
        recordPtr: recordPtr,
        ddPtr: ddPtr
    });
}

mergeInto(LibraryManager.library, {
    browser_init_audio: browser_init_audio
});
