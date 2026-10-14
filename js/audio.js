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
    self.__arcAudioCtrl = new Int32Array(HEAPU8.buffer, ctrlPtr, 32);
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

/**
 * Publish a counter in the ring's control block. The emulator writes the audio
 * data with ordinary stores, then calls this: Atomics.store orders those writes
 * before the new counter value becomes visible to the worklet's Atomics.load.
 */
function browser_publish_audio(ctrlPtr, index, value) {
    Atomics.store(self.__arcAudioCtrl, index, value);
}

/**
 * Publish three counters, in order. The worklet reads them in the opposite
 * order, so if it sees the later ones it sees everything that went with the
 * earlier ones.
 */
function browser_publish_audio3(ctrlPtr, index1, value1, index2, value2, index3, value3) {
    const ctrl = self.__arcAudioCtrl;
    Atomics.store(ctrl, index1, value1);
    Atomics.store(ctrl, index2, value2);
    Atomics.store(ctrl, index3, value3);
}

mergeInto(LibraryManager.library, {
    browser_init_audio: browser_init_audio,
    browser_publish_audio: browser_publish_audio,
    browser_publish_audio3: browser_publish_audio3
});
