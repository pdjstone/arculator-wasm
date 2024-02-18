// Define a name for each key on the Archimedes keyboard
// Map it to the row/col number generated by the keyboard microcontroller
// See https://chrisacorns.computinghistory.org.uk/docs/Acorn/Manuals/Acorn_A3000TRM.pdf#page=17
const ARC_KEY_LAYOUT_STANDARD = Object.freeze({
    _:null, // in Arculator the keys start at 1
    // Function keys
    ArcF1: [0,1], ArcF2:  [0,2],  ArcF3:  [0,3],  ArcF4:  [0,4],
    ArcF5: [0,5], ArcF6:  [0,6],  ArcF7:  [0,7],  ArcF8:  [0,8], 
    ArcF9: [0,9], ArcF10: [0,10], ArcF11: [0,11], ArcF12: [0,12], 
    
    // Digits
    ArcDigit1: [1,1], ArcDigit2: [1,2], ArcDigit3: [1,3],  
    ArcDigit4: [1,4], ArcDigit5: [1,5], ArcDigit6: [1,6], 
    ArcDigit7: [1,7], ArcDigit8: [1,8], ArcDigit9: [1,9], ArcDigit0: [1,10],
    
    // Alpha keys
    ArcQ: [2,7],  ArcW: [2,8],  ArcE: [2,9],
    ArcR: [2,10], ArcT: [2,11], ArcY: [2,12], 
    ArcU: [2,13], ArcI: [2,14], ArcO: [2,15], ArcP: [3,0],
    
    ArcA: [3,12], ArcS: [3,13], ArcD: [3,14], 
    ArcF: [3,15], ArcG: [4,0],  ArcH: [4,1], 
    ArcJ: [4,2],  ArcK: [4,3],  ArcL: [4,4], 

    ArcZ: [4,14], ArcX: [4,15], ArcC: [5,0], 
    ArcV: [5,1],  ArcB: [5,2],  ArcM: [5,4], ArcN: [5,3], 

    // Left hand side
    ArcEscape:      [0,0], 
    ArcBackquote:   [1,0],
    ArcTab:         [2,6], 
    ArcControlLeft: [3,11], 
    ArcShiftLeft:   [4,12],  
    ArcCapsLock:    [5,13], ArcAltLeft: [5,14], ArcSpace: [5,15],

    // Right hand side
    ArcMinus:       [1,11], ArcEqual:        [1,12], ArcPound:     [1,13], ArcBackspace:  [1,14],  
    ArcLeftBracket: [3,1],  ArcRightBracket: [3,2],  ArcBackslash: [3,3],
    ArcSemicolon:   [4,5],  ArcQuote:        [4,6],  ArcReturn:    [4,7],
    ArcComma:       [5,5],  ArcPeriod:       [5,6],  ArcSlash:     [5,7],  ArcShiftRight: [5,8],
    ArcAltRight:    [6,0],  ArcControlRight: [6,1], 

    // Middle column keys
    ArcPrint:     [0,13], ArcScrollLock: [0,14], ArcBreak:      [0,15],

    ArcInsert:    [1,15], ArcHome:       [2,0],  ArcPageUp:     [2,1],
    ArcDelete:    [3,4],  ArcCopy:       [3,5],  ArcPageDown:   [3,6], 
    
                          ArcArrowUp:    [5,9],
    ArcArrowLeft: [6,2],  ArcArrowDown:  [6,3],  ArcArrowRight: [6,4], 

    // Numpad
    ArcNumLock: [2,2],  ArcNumpadDivide: [2,3],  ArcNumpadMultiply: [2,4],  ArcNumpadHash:     [2,5],
    ArcNumpad7: [3,7],  ArcNumpad8:      [3,8],  ArcNumpad9:        [3,9],  ArcNumpadSubtract: [3,10],
    ArcNumpad4: [4,8],  ArcNumpad5:      [4,9],  ArcNumpad7:        [4,10], ArcNumpadAdd:      [4,11], 
    ArcNumpad1: [5,10], ArcNumpad2:      [5,11], ArcNumpad3:        [5,12],
    ArcNumpad0: [6,5],                           ArcNumpadDecimal:  [6,6],  ArcNumpadEnter:    [6,7],

    ArcMouseLeft: [7,0], ArcMouseMiddle: [7,1], ArcMouseRight: [7,2]
});

const ARC_KEY_LAYOUT_A500 = Object.freeze({
    // TODO
});

const NUM_ARC_KEYS = Object.keys(ARC_KEY_LAYOUT_STANDARD).length;

// Map from browser KeyboardEvent.code to Archimedes key name (from ARC_KEY_LAYOUT)
// See https://developer.mozilla.org/en-US/docs/Web/API/UI_Events/Keyboard_event_code_values
// This is based on a UK ISO layout keyboard, with a |\ key to the right of the left shift key
// and a ~# key next to the @' key
const BROWSER_KEY_MAP_UK = Object.freeze({
    // Function keys
    F1: 'ArcF1', F2:  'ArcF2', F3:  'ArcF3',  F4:  'ArcF4',
    F5: 'ArcF5', F6:  'ArcF6', F7:  'ArcF7',  F8:  'ArcF8', 
    F9: 'ArcF9', F10: 'ArcF10', F11: 'ArcF11', F12: 'ArcF12', 
    
    // Digits
    Digit1: 'ArcDigit1', Digit2: 'ArcDigit2', Digit3: 'ArcDigit3',  
    Digit4: 'ArcDigit4', Digit5: 'ArcDigit5', Digit6: 'ArcDigit6', 
    Digit7: 'ArcDigit7', Digit8: 'ArcDigit8', Digit9: 'ArcDigit9', Digit0: 'ArcDigit0',
    
    // Alpha keys
    KeyQ: 'ArcQ', KeyW: 'ArcW', KeyE: 'ArcE',
    KeyR: 'ArcR', KeyT: 'ArcT', KeyY: 'ArcY', 
    KeyU: 'ArcU', KeyI: 'ArcI', KeyO: 'ArcO', KeyP: 'ArcP',
    
    KeyA: 'ArcA', KeyS: 'ArcS', KeyD: 'ArcD', 
    KeyF: 'ArcF', KeyG: 'ArcG', KeyH: 'ArcH', 
    KeyJ: 'ArcJ', KeyK: 'ArcK', KeyL: 'ArcL', 

    KeyZ: 'ArcZ', KeyX: 'ArcX', KeyC: 'ArcC', 
    KeyV: 'ArcV', KeyB: 'ArcB', KeyM: 'ArcM', KeyN: 'ArcN', 

    // Left hand side
    Escape:      'ArcEscape', 
    Backquote:   'ArcBackquote',
    Tab:         'ArcTab', 
    CapsLock:    'ArcCapsLock', // CapsLock and Ctrl are swapped compared with Arc
    ShiftLeft:   'ArcShiftLeft',   IntlBackslash: 'ArcBackslash', // ArcBackslash is next to ]
    ControlLeft: 'ArcControlLeft', AltLeft: 'ArcAltLeft', Space:'ArcSpace',

    // Right hand side
    Minus:       'ArcMinus',       Equal:        'ArcEqual',        Backspace: 'ArcBackspace',  
    BracketLeft: 'ArcLeftBracket', BracketRight: 'ArcRightBracket', Enter:     'ArcReturn',
    Semicolon:   'ArcSemicolon',   Quote:        'ArcQuote',        Backslash: 'ArcPound', // 'Backslash' maps to #~ on some UK keyboards. ArcPound is beween Equals and Backspace
    Comma:       'ArcComma',       Period:       'ArcPeriod',       Slash:     'ArcSlash',  ShiftRight: 'ArcShiftRight',
    AltRight:    'ArcAltRight',    ControlRight: 'ArcControlRight', 

    // Middle column keys
    PrintScreen: 'ArcPrint',     ScrollLock: 'ArcScrollLock', Pause:      'ArcBreak', 

    Insert:      'ArcInsert',    Home:       'ArcHome',       PageUp:     'ArcPageUp',
    Delete:      'ArcDelete',    End:        'ArcCopy',       PageDown:   'ArcPageDown', 
    
                                 ArrowUp:    'ArcArrowUp',
    ArrowLeft:   'ArcArrowLeft', ArrowDown:  'ArcArrowDown',  ArrowRight: 'ArcArrowRight', 

    // Numpad - currently nothing mapped to ArcNumpadHash. On Arc, multiply and subtract are on different rows
    NumLock: 'ArcNumLock', NumpadDivide: 'ArcNumpadDivide', NumpadMultiply: 'ArcNumpadMultiply', NumpadSubtract: 'ArcNumpadSubtract',
    Numpad7: 'ArcNumpad7', Numpad8:      'ArcNumpad8',      Numpad9:        'ArcNumpad9',        NumpadAdd:      'ArcNumpadAdd', 
    Numpad4: 'ArcNumpad4', Numpad5:      'ArcNumpad5',      Numpad7:        'ArcNumpad7',
    Numpad1: 'ArcNumpad1', Numpad2:      'ArcNumpad2',      Numpad3:        'ArcNumpad3',        NumpadEnter:   'ArcNumpadEnter',
    Numpad0: 'ArcNumpad0',                                  NumpadDecimal:  'ArcNumpadDecimal', 
});


/*
'Special' Keys Testing

PrintScreen:
 - PopOS - pressing PrtScr on its own or with Alt does OS screenshot. Can press with left or right ctrl or AltGr to get PrintScreen key

*/


/**
 * Input handling for Arculator WASM. 
 * This replaces the SDL-based input routines from upstream Arculator.
 */
class EmulatorInput
{
    constructor(browserKeyMap = null, captureElementId = "canvas") {
        if (browserKeyMap == null)
            browserKeyMap = BROWSER_KEY_MAP_UK;

        this.browserKeyMap = browserKeyMap;

        this.captureElement = document.getElementById(captureElementId);
        this.canvasPaddingX = (canvas.clientWidth - canvas.width)/2;
        this.canvasPaddingY = (canvas.clientHeight - canvas.height)/2;
        this.CANVAS_SNAP_BORDER = 50;

        // Map from KeyboardEvent.keyCode to key ID
        // Key ID is the index of the keys in ARC_KEY_LAYOUT
        // e.g. ArcF1 is 0, ArcF2 is 1 etc..
        // this gets initialised as keys are pressed for the first time
        // because we don't know ahead of time what keyCodes a browser uses
        this.keyCodeToKeyId = new Int8Array(256);
    
        this.buttons = 0; // last state of mouse buttons

        this.mouseCaptureNeeded = 0;
        this.allowMouseCapture = true;
        this.cursorX = 0;
        this.cursorY = 0;

        this.initBrowserKeys();
        this.installEventHandlers();
    }

    setupArculatorKeyboard(keyStatePtr, keyLayoutPtr) {
        this.arcKeyStatePtr = keyStatePtr >> 2; // ptr to int so HEAP32
        this.arcKeyLayoutPtr = keyLayoutPtr >> 2; // ptr to int so HEAP32

        // Initialise Arculator buffers 
        let row, col;
        for (let keyId=1; keyId < NUM_ARC_KEYS; keyId++) {
            HEAP32[this.arcKeyStatePtr+keyId] = 0;
            [row, col] = Object.values(ARC_KEY_LAYOUT_STANDARD)[keyId];
            HEAP32[this.arcKeyLayoutPtr + (keyId-1) * 2] = row;
            HEAP32[this.arcKeyLayoutPtr + (keyId-1) * 2 + 1] = col;
        }
    }

    setupArculatorMouse(buttonsPtr, mxPtr, myPtr, mouseModePtr) {
        this.buttonsPtr =  buttonsPtr >> 2;  // ptr to int so HEAP32
        this.mxPtr = mxPtr >> 2;
        this.myPtr = myPtr >> 2;
        this.mouseModePtr = mouseModePtr >> 2;
    }

    initBrowserKeys() {
        for (let i=0; i < this.keyCodeToKeyId.length; i++) {
            this.keyCodeToKeyId[i] = -1;
        }

        // Map from KeyboardEvent.code to key ID
        this.keyIds = {};
        for (const [browserKey, arcKey] of Object.entries(this.browserKeyMap)) {
            this.keyIds[browserKey] = this.getKeyId(arcKey);
        }
        this.mouseLeftKeyId = this.getKeyId('ArcMouseLeft');
        this.mouseMiddleKeyId = this.getKeyId('ArcMouseMiddle');
        this.mouseRightKeyId = this.getKeyId('ArcMouseRight');
    }

    installEventHandlers() {
        let obj = this;
       
        this.captureElement.addEventListener('keydown', (evt) => obj.handleKeyDownEvent(evt));
        this.captureElement.addEventListener('keyup', (evt) => obj.handleKeyUpEvent(evt));
        this.captureElement.addEventListener('mousedown', (evt) => obj.handleMouseButtonEvent(evt));
        // we listen to mouse up on whole document to avoid buttons getting stuck down
        // if they are released outside canvas
        document.addEventListener('mouseup', (evt) => obj.handleMouseButtonEvent(evt));

        this.captureElement.addEventListener('mousemove', (evt) => obj.handleMouseMoveEvent(evt));
        this.captureElement.addEventListener('mouseleave', (evt) => obj.handleMouseLeaveEvent(evt));
    }

    handleKeyDownEvent(evt) {
        let keyId = this.keyCodeToKeyId[evt.keyCode];
        if (keyId == -1 && evt.code in this.browserKeyMap) {
            keyId = this.keyIds[evt.code];
            //console.log(`map key ${evt.code} => ${evt.keyCode} => ${keyId}`);
            this.keyCodeToKeyId[evt.keyCode] = keyId;
        }
        HEAP32[this.arcKeyStatePtr + keyId] = 1;
        evt.preventDefault();
    }

    handleKeyUpEvent(evt) {
        let keyId = this.keyCodeToKeyId[evt.keyCode];
        HEAP32[this.arcKeyStatePtr + keyId] = 0;
        evt.preventDefault();
    }

    handleMouseButtonEvent(evt) {
        if (evt.type == 'mouseup') {
            // bitwise-and with buttons pressed in canvas to
            // avoid buttons pressed-but-not-released outside
            // canvas being sent to canvas
            this.buttons = this.buttons & evt.buttons;
        } else {
            this.buttons = evt.buttons;
            if (this.mouseCaptureNeeded && !document.pointerLockElement && this.allowMouseCapture)
                tryCapture(evt);
        }

        if (this.buttonsPtr)
            HEAP32[this.buttonsPtr] = evt.buttons;
        HEAP32[this.arcKeyStatePtr + this.mouseLeftKeyId] = evt.buttons & 1;
        HEAP32[this.arcKeyStatePtr + this.mouseMiddleKeyId] = evt.buttons & 4;
        HEAP32[this.arcKeyStatePtr + this.mouseRightKeyId] = evt.buttons & 2;
    }

    getKeyId(arcKeyName) {
        return Object.keys(ARC_KEY_LAYOUT_STANDARD).indexOf(arcKeyName);
    }

    simulateKey(arcKeyName, delayMs = 20, durationMs = 40) {
        let keyId = this.getKeyId(arcKeyName);
        if (keyId == -1) return;
        setTimeout(() => {
            HEAP32[this.arcKeyStatePtr + keyId] = 1;
            setTimeout(() => HEAP32[this.arcKeyStatePtr + keyId] = 0, durationMs);
        }, delayMs);
    }

    setAllowMouseCapture(allow) {
        console.log("allow mouse capture", allow);
        this.allowMouseCapture = allow;
    }

    handleMouseMoveEvent(evt) {
        if (document.pointerLockElement || this.mouseCaptureNeeded) {
            HEAP32[this.mxPtr] += evt.movementX;
            HEAP32[this.myPtr] += evt.movementY;
            HEAP32[this.mouseModePtr] = 1;
        } else if (!this.mouseCaptureNeeded) {
            let canvas = this.captureElement;
            // The canvas element has padding to allow the pointer 
            // to be easily moved to/along the edge of the screen.
            // Mouse movement within the padding will still be sent 
            // to the emulator, but snapped to the edge
            let x = evt.offsetX - this.canvasPaddingX;
            let y = evt.offsetY - this.canvasPaddingY;
            if (x < 0) x = 0;
            if (y < 0) y = 0;
            if (x > canvas.width) x = canvas.width;
            if (y > canvas.height) y = canvas.height;
            this.sendAbsMouse(x, y);
        }
        evt.stopPropagation();
    }

    /**
     * Listen to mouseleave events to handle the case where the 
     * canvas edge is very close to the browser window edge. 
     * If the mouse skips out of the canvas, but the pointer
     * was close to the edge of the screen we snap to the screen edge
     */
    handleMouseLeaveEvent(evt) {
        if (this.mouseCaptureNeeded)
            return;

        let x = this.cursorX;
        let y = this.cursorY;
        let canvas = this.captureElement;

        // Don't snap if mouse is already at the edge. This avoids 
        // the pointer being snapped into a corner unexpectedly
        if (x == 0 || y == 0 || x == canvas.width || y == canvas.height)
            return;
        if (x < this.CANVAS_SNAP_BORDER) x = 0;
        if (x > canvas.width - this.CANVAS_SNAP_BORDER)  x = canvas.width;     
        if (y < this.CANVAS_SNAP_BORDER) y = 0;
        if (y > canvas.height - this.CANVAS_SNAP_BORDER) y = canvas.height;
        this.sendAbsMouse(x, y);
    }

    sendAbsMouse(x, y) {
        HEAP32[this.mouseModePtr] = 2;
        HEAP32[this.mxPtr] = x;
        // invert Y mouse coord
        HEAP32[this.myPtr] = y;
        this.cursorX = x;
        this.cursorY = y;
    }

}

function getEmuInput() {
    if (typeof(window.emuinput) == "undefined") {
        console.log("Creating EmulatorInput instance")
        window.emuinput = new EmulatorInput(BROWSER_KEY_MAP_UK);
    } 
    return window.emuinput;
}