function video_renderer_init(main_window) {
    console.log('video_renderer_init', main_window);

    let gl = Module['canvas'].getContext('webgl2', 
        { alpha: false, antialias: false, desynchronized: true });

    if (!gl) {
        console.error('WebGL2 not supported in worker');
        return;
    }

    function compileShader(type, source) {
        const shader = gl.createShader(type);
        gl.shaderSource(shader, source);
        gl.compileShader(shader);
        if (!gl.getShaderParameter(shader, gl.COMPILE_STATUS)) {
            console.error(gl.getShaderInfoLog(shader));
            gl.deleteShader(shader);
            return null;
        }
        return shader;
    }

    function createProgram(vs, fs) {
        const prog = gl.createProgram();
        gl.attachShader(prog, vs);
        gl.attachShader(prog, fs);
        gl.linkProgram(prog);
        if (!gl.getProgramParameter(prog, gl.LINK_STATUS)) {
            console.error(gl.getProgramInfoLog(prog));
            return null;
        }
        return prog;
    }

    // Vertex Shader: Simple pass-through quad
    const VS_SOURCE = `#version 300 es
        in vec2 a_position;
        out vec2 v_texCoord;
        void main() {
            gl_Position = vec4(a_position, 0.0, 1.0);
            v_texCoord = (a_position + 1.0) / 2.0;
            v_texCoord.y = 1.0 - v_texCoord.y; // Flip Y for canvas space
        }
    `;

    // Fragment Shader: Sample the texture
    const FS_SOURCE = `#version 300 es
        precision mediump float;
        uniform sampler2D u_texture;
        in vec2 v_texCoord;
        out vec4 outColor;
        void main() {
            outColor = texture(u_texture, v_texCoord);
        }
    `;

    // Everything the context holds, made again if it's lost and restored
    function createResources() {
        const vs = compileShader(gl.VERTEX_SHADER, VS_SOURCE);
        const fs = compileShader(gl.FRAGMENT_SHADER, FS_SOURCE);
        let program = createProgram(vs, fs);
        gl.useProgram(program);

        const quad = new Float32Array([
            -1, -1,  1, -1, -1,  1,     // |\
            -1,  1,  1, -1,  1,  1,     //    \|
        ]);
        const vao = gl.createVertexArray();
        gl.bindVertexArray(vao);
        const buffer = gl.createBuffer();
        gl.bindBuffer(gl.ARRAY_BUFFER, buffer);
        gl.bufferData(gl.ARRAY_BUFFER, quad, gl.STATIC_DRAW);
        const posLoc = gl.getAttribLocation(program, 'a_position');
        gl.enableVertexAttribArray(posLoc);
        gl.vertexAttribPointer(posLoc, 2, gl.FLOAT, false, 0, 0);

        let texture = gl.createTexture();
        gl.bindTexture(gl.TEXTURE_2D, texture);
        gl.texParameteri(gl.TEXTURE_2D, gl.TEXTURE_MIN_FILTER, gl.NEAREST);
        gl.texParameteri(gl.TEXTURE_2D, gl.TEXTURE_MAG_FILTER, gl.NEAREST);
        gl.texParameteri(gl.TEXTURE_2D, gl.TEXTURE_WRAP_S, gl.CLAMP_TO_EDGE);
        gl.texParameteri(gl.TEXTURE_2D, gl.TEXTURE_WRAP_T, gl.CLAMP_TO_EDGE);
        return texture;
    }

    // GL calls do nothing while the context is lost, so frames are just dropped
    Module['canvas'].addEventListener('webglcontextlost', e => {
        console.warn('WebGL context lost');
        e.preventDefault();     // otherwise it's never restored
    });
    Module['canvas'].addEventListener('webglcontextrestored', () => {
        console.log('WebGL context restored');
        Module.renderer.texture = createResources();
        // The next update_texture sets up the new texture and the viewport
        Module.renderer.textureWidth = null;
    });

    Module.renderer = {
        gl: gl,
        texture: createResources(),
        textureWidth: null,
        textureHeight: null,
        dblScan: false,
        xOffset: null,
        yOffset: null,
        bufferPointer: null,
        view: null,
        // from vidc.c create_bitmap
        MAX_WIDTH: 2048,
        MAX_HEIGHT: 2048,
    }

    return 1;
}

function update_window_size(w,h) {
    if (w != Module.canvas.width || h != Module.canvas.height) {
        Module.canvas.width = w;
        Module.canvas.height = h; 
    }
}

function update_texture(bufferPointer, x, y, w, h) {
    // The view also depends on where the frame bitmap is, so rebuild it if that moves
    if (w !== Module.renderer.textureWidth || h !== Module.renderer.textureHeight ||
        bufferPointer !== Module.renderer.bufferPointer || x !== Module.renderer.xOffset || y !== Module.renderer.yOffset) {
        console.log('resize texture', x, y, w, h);
        Module.renderer.textureWidth = w;
        Module.renderer.textureHeight = h;        
        Module.renderer.bufferPointer = bufferPointer;
        Module.renderer.xOffset = x;
        Module.renderer.yOffset = y;
      
        let gl = Module.renderer.gl;

        gl.viewport(0, 0, w, Module.renderer.dblScan ? h*2 : h);
        gl.bindTexture(gl.TEXTURE_2D, Module.renderer.texture);
        gl.texImage2D(gl.TEXTURE_2D, 0, gl.RGBA, w, h, 0, gl.RGBA, gl.UNSIGNED_BYTE, null);
        gl.pixelStorei(gl.UNPACK_ROW_LENGTH, Module.renderer.MAX_WIDTH);
        gl.pixelStorei(gl.UNPACK_SKIP_PIXELS, x);

        Module.renderer.view = new Uint8Array(Module.HEAPU8.buffer, 
            bufferPointer + y * Module.renderer.MAX_WIDTH * 4, // skip top pixels
            Module.renderer.MAX_WIDTH * h * 4
        );
    }
}

function render(src_x, src_y, src_w, src_h, dblScan) {
    if (Module.renderer.dblScan != dblScan) {
        Module.renderer.dblScan = dblScan;
        let h = dblScan ? Module.renderer.textureHeight * 2 : Module.renderer.textureHeight;
        Module.renderer.gl.viewport(0, 0, Module.renderer.textureWidth, h);
    }
    
    let gl = Module.renderer.gl;
    gl.texImage2D(gl.TEXTURE_2D, 0, gl.RGBA, Module.renderer.textureWidth, Module.renderer.textureHeight, 0, gl.RGBA, gl.UNSIGNED_BYTE, Module.renderer.view);
    gl.drawArrays(gl.TRIANGLES, 0, 6);

    /*Screenshots: Module.screenshotRequests is a list of callbacks waiting for
      the next frame (see arculator-worker.js). The canvas is the size of the
      frame, with double-scanned lines doubled, and convertToBlob copies it at
      once, before the drawing buffer is presented and cleared. Each callback
      gets the promise of a PNG Blob.*/
    if (Module.screenshotRequests && Module.screenshotRequests.length) {
        let png;
        try {
            png = Module.canvas.convertToBlob({ type: 'image/png' });
        } catch (e) {
            png = Promise.reject(e);    // e.g. the context is lost
        }
        for (let callback of Module.screenshotRequests.splice(0))
            callback(png);
    }
}


mergeInto(LibraryManager.library, {
    video_renderer_init: video_renderer_init,
    emscripten_render_update: update_texture,
    emscripten_render_present: render,
    emscripten_update_window_size: update_window_size
});