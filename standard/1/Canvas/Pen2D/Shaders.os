    // ---- user 2D shaders ----
    // fragBody: GLSL fragment `void main() { ... o = vec4(...); }` (see fs2dPrefix for what's in scope).
    // Returns a program id, 0 on failure (OpenGL.lastError() has the log).
    createShader(fragBody: char*) => u32 {
        if (this.backend != API_GL) return 0;
        G.glfwMakeContextCurrent(this.h);
        let src: char* = catStr(fs2dPrefix(), fragBody);
        let p: u32 = OGL.program(vs2d(), src);
        CMem.free(src as void*);
        return p;
    }
    freeShader(p: u32) => void {
        if (p == 0) return;
        if (this.backend == API_GL) { G.glfwMakeContextCurrent(this.h); OGL.glDeleteProgram(p); }
    }

    // Vulkan: load a fragment shader compiled to SPIR-V (see shaders/plasma.frag). It gets location 0 vUV,
    // location 1 vCol and the push block { vec2 uScreen; vec2 uSize; float uTime; }. Returns 0 elsewhere / on failure.
    // Shaders live until the pen is freed (up to 16).
    createShaderSpv(fragSpvPath: char*) => u32 {
        if (this.backend != API_VULKAN || this.vkShaderCount >= 16) return 0;
        let vsPath: char* = catStr(this.shaderDir, "canvas2d.vert.spv");
        let vs: u64 = this.vk.loadShader(vsPath);
        CMem.free(vsPath as void*);
        let fs: u64 = this.vk.loadShader(fragSpvPath);
        if (vs == 0 || fs == 0) { this.vk.destroyShader(vs); this.vk.destroyShader(fs); return 0; }
        let formats = CMem.malloc(12) as u32*;
        let offsets = CMem.malloc(12) as u32*;
        formats[0] = VK.VK_FORMAT_R32G32_SFLOAT; formats[1] = VK.VK_FORMAT_R32G32_SFLOAT; formats[2] = VK.VK_FORMAT_R32G32B32A32_SFLOAT;
        offsets[0] = 0; offsets[1] = 8; offsets[2] = 16;
        let ok: bool = this.vk.createPipeline(&this.vkShaders[this.vkShaderCount], vs, fs, 32, 3, formats, offsets, false, 20);
        CMem.free(formats as void*); CMem.free(offsets as void*);
        this.vk.destroyShader(vs); this.vk.destroyShader(fs);
        if (!ok) return 0;
        this.vkShaderCount = this.vkShaderCount + 1;
        return this.vkShaderCount as u32;
    }

    // Portable: GLSL fragment body for GL, SPIR-V path for Vulkan; software returns 0 (shaderRect is skipped).
    createShaderBoth(glFragBody: char*, vkFragSpvPath: char*) => u32 {
        if (this.backend == API_GL) return this.createShader(glFragBody);
        if (this.backend == API_VULKAN) return this.createShaderSpv(vkFragSpvPath);
        return 0;
    }
