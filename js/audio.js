/**
 * Called from sound_dev_init in sound_emscripten.c. Hands the shared audio ring
 * (see src/sound_ring.h) to the main thread, which creates the AudioContext
 * and AudioWorklet that read from it. HEAPU8.buffer is a SharedArrayBuffer
 * because the module is built with -sSHARED_MEMORY, and the static data the
 * pointers refer to stays at the same address if the heap grows.
 */
function browser_init_audio(ctrlPtr, audioPtr, ddPtr) {
    console.log('browser_init_audio', ctrlPtr, audioPtr, ddPtr);
    self.postMessage({
        type: 'AudioInit',
        heap: HEAPU8.buffer,
        ctrlPtr: ctrlPtr,
        audioPtr: audioPtr,
        ddPtr: ddPtr
    });
}

/**
 * Publish a counter in the ring's control block. The emulator writes the audio
 * data with ordinary stores, then calls this: Atomics.store orders those writes
 * before the new counter value becomes visible to the worklet's Atomics.load.
 */
function browser_publish_audio(ctrlPtr, index, value) {
    Atomics.store(new Int32Array(HEAPU8.buffer, ctrlPtr, 32), index, value);
}

mergeInto(LibraryManager.library, {
    browser_init_audio: browser_init_audio,
    browser_publish_audio: browser_publish_audio
});
