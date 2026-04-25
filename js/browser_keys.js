/**
 * Called from keyboard_init in keyboard.c
 */
function browser_init_keys(keystate, keylayout) {
    console.log('browser_init_keys', keystate, keylayout);
    self.postMessage({
        type: 'EmulatorInput',
        func: 'setupArculatorKeyboard',
        params: [HEAPU8.buffer, keystate, keylayout]
    });
}

/**
 * Called from mouse_init in input_emscripten.c
 */
function browser_init_mouse(mxPtr, myPtr, buttonsPtr, mouseModePtr) { 
    console.log('browser_init_mouse', mxPtr, myPtr, mouseModePtr);
    self.postMessage({
        type: 'EmulatorInput',
        func: 'setupArculatorMouse',
        params: [HEAPU8.buffer, buttonsPtr, mxPtr, myPtr, mouseModePtr]
    });
}

function notify_mouse_lock_required(needed) {
    console.log('notify_mouse_lock_required', needed);
    self.postMessage({
        type: 'EmulatorInput',
        func: 'setMouseCaptureNeeded',
        params: [needed]
    });
}

mergeInto(LibraryManager.library, {
    browser_init_keys: browser_init_keys,
    browser_init_mouse: browser_init_mouse,
    notify_mouse_lock_required: notify_mouse_lock_required
});


