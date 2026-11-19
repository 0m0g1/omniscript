    // Needs the window's GL context current and OpenGL.load() done.
    init(handle: void*) => bool {
        this.h = handle;
        this.backend = API_GL;
        this.clearR = 0.1; this.clearG = 0.1; this.clearB = 0.12; this.clearA = 1.0;
        if (!this.prog.build(vs2d(), fs2d())) return false;
        OGL.glGenVertexArrays(1, &this.vao);
        OGL.glGenBuffers(1, &this.vbo);
        OGL.glGenBuffers(1, &this.ebo);
        OGL.glBindVertexArray(this.vao);
        OGL.glBindBuffer(OGL.ARRAY_BUFFER, this.vbo);
        OGL.glVertexAttribPointer(0, 2, OGL.FLOAT, 0, 32, 0 as i64 as void*);
        OGL.glEnableVertexAttribArray(0);
        OGL.glVertexAttribPointer(1, 2, OGL.FLOAT, 0, 32, 8 as i64 as void*);
        OGL.glEnableVertexAttribArray(1);
        OGL.glVertexAttribPointer(2, 4, OGL.FLOAT, 0, 32, 16 as i64 as void*);
        OGL.glEnableVertexAttribArray(2);
        OGL.glBindBuffer(OGL.ELEMENT_ARRAY_BUFFER, this.ebo);
        OGL.glBindVertexArray(0);

        let px: u8* = F.buildAtlas();
        let t: OGL.Texture;
        t.fromRGBA(px, F.ATLAS_W, F.ATLAS_H, false);
        GL.glTexParameteri(OGL.TEXTURE_2D, OGL.TEXTURE_MAG_FILTER, OGL.NEAREST);
        GL.glTexParameteri(OGL.TEXTURE_2D, OGL.TEXTURE_MIN_FILTER, OGL.NEAREST);
        CMem.free(px as void*);
        this.atlas = t.id;
        this.clipStack = CMem.malloc(64 * 4) as int*;
        this.ready = true;
        return true;
    }

    // Shared setup for the non-GL backends: clip stack + image table.
    initTables() => void {
        this.clearR = 0.1; this.clearG = 0.1; this.clearB = 0.12; this.clearA = 1.0;
        this.clipStack = CMem.malloc(64 * 4) as int*;
        this.imgUsed = CMem.malloc(MAX_IMAGES as u64) as u8*;
        this.imgW = CMem.malloc((MAX_IMAGES * 4) as u64) as int*;
        this.imgH = CMem.malloc((MAX_IMAGES * 4) as u64) as int*;
        this.imgPx = CMem.malloc((MAX_IMAGES * 8) as u64) as u8**;
        CMem.memset(this.imgUsed as void*, 0, MAX_IMAGES as u64);
        CMem.memset(this.imgW as void*, 0, (MAX_IMAGES * 4) as u64);
        CMem.memset(this.imgH as void*, 0, (MAX_IMAGES * 4) as u64);
        CMem.memset(this.imgPx as void*, 0, (MAX_IMAGES * 8) as u64);
        this.atlas = 1;
        this.imgUsed[0] = 1 as u8;
    }

    // ---- software backend: no window, no GL. Draw, then read pixels() (RGBA8, row 0 = top). ----
    initSoftware(w: int, hh: int) => bool {
        this.backend = API_SOFTWARE;
        this.initTables();
        this.imgPx[0] = F.buildAtlas();
        this.imgW[0] = F.ATLAS_W; this.imgH[0] = F.ATLAS_H;
        this.swResize(w, hh);
        this.ready = true;
        return true;
    }

    swResize(w: int, hh: int) => void {
        if (w <= 0 || hh <= 0) return;
        if (this.swBuf != nullptr && this.swW == w && this.swH == hh) return;
        if (this.swBuf != nullptr) CMem.free(this.swBuf as void*);
        this.swBuf = CMem.malloc((w * hh * 4) as u64) as u8*;
        CMem.memset(this.swBuf as void*, 0, (w * hh * 4) as u64);
        this.swW = w; this.swH = hh;
    }

    pixels() => u8* { return this.swBuf; }
    pixelWidth() => int { return this.swW; }
    pixelHeight() => int { return this.swH; }

    // ---- vulkan backend: vk must already be a presentation target (Vulkan.init or Vulkan.attach) ----
    // dir = folder holding canvas2d.vert.spv / canvas2d.frag.spv (default "shaders/").
    initVulkan(target: VK.Vulkan*, dir: char*) => bool {
        this.backend = API_VULKAN;
        this.vk = target;
        this.shaderDir = dir;
        if (dir == nullptr) this.shaderDir = "shaders/";
        this.initTables();
        let vsPath: char* = catStr(this.shaderDir, "canvas2d.vert.spv");
        let fsPath: char* = catStr(this.shaderDir, "canvas2d.frag.spv");
        let vs: u64 = target.loadShader(vsPath);
        let fs: u64 = target.loadShader(fsPath);
        CMem.free(vsPath as void*); CMem.free(fsPath as void*);
        if (vs == 0 || fs == 0) return false;
        let formats = CMem.malloc(12) as u32*;
        let offsets = CMem.malloc(12) as u32*;
        formats[0] = VK.VK_FORMAT_R32G32_SFLOAT; formats[1] = VK.VK_FORMAT_R32G32_SFLOAT; formats[2] = VK.VK_FORMAT_R32G32B32A32_SFLOAT;
        offsets[0] = 0; offsets[1] = 8; offsets[2] = 16;
        let ok: bool = target.createPipeline(&this.vkPipe, vs, fs, 32, 3, formats, offsets, true, 20);
        CMem.free(formats as void*); CMem.free(offsets as void*);
        target.destroyShader(vs); target.destroyShader(fs);
        if (!ok) return false;
        this.vkVb = CMem.malloc(2 * sizeof(VK.Buffer)) as VK.Buffer*;
        this.vkIb = CMem.malloc(2 * sizeof(VK.Buffer)) as VK.Buffer*;
        CMem.memset(this.vkVb as void*, 0, 2 * sizeof(VK.Buffer));
        CMem.memset(this.vkIb as void*, 0, 2 * sizeof(VK.Buffer));
        this.vkTex = CMem.malloc((MAX_IMAGES as u64) * sizeof(VK.Texture)) as VK.Texture*;
        CMem.memset(this.vkTex as void*, 0, (MAX_IMAGES as u64) * sizeof(VK.Texture));
        this.vkShaders = CMem.malloc(16 * sizeof(VK.Pipeline)) as VK.Pipeline*;
        CMem.memset(this.vkShaders as void*, 0, 16 * sizeof(VK.Pipeline));
        this.vkPc = CMem.malloc(20) as float*;
        let px: u8* = F.buildAtlas();
        let good: bool = target.createTexture(&this.vkTex[0], px, F.ATLAS_W, F.ATLAS_H);
        CMem.free(px as void*);
        if (!good) return false;
        this.ready = true;
        return true;
    }
