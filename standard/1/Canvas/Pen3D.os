// ---------------------------------------------------------------------------------------------
// Pen3D (GL windows only: ready == false on Vulkan/software windows and every call is a no-op)
// ---------------------------------------------------------------------------------------------
struct Pen3D {
    h: void* = nullptr;
    prog: OGL.Program;
    view: float* = nullptr; proj: float* = nullptr; vp: float* = nullptr; tmp: float* = nullptr;
    lightX: float = 0.4; lightY: float = 0.8; lightZ: float = 0.5;
    w: int = 0; ht: int = 0;
    ready: bool = false;

    init(handle: void*) => bool {
        this.h = handle;
        this.lightX = 0.4; this.lightY = 0.8; this.lightZ = 0.5;       // Win memsets us to 0, so set defaults here
        if (!this.prog.build(vs3d(), fs3d())) return false;
        this.view = CMem.malloc(64) as float*; this.proj = CMem.malloc(64) as float*;
        this.vp = CMem.malloc(64) as float*; this.tmp = CMem.malloc(64) as float*;
        matIdentity(this.view); matIdentity(this.proj); matIdentity(this.vp);
        this.ready = true;
        return true;
    }

    free() => void {
        if (!this.ready) return;
        G.glfwMakeContextCurrent(this.h);
        this.prog.free();
        CMem.free(this.view as void*); CMem.free(this.proj as void*); CMem.free(this.vp as void*); CMem.free(this.tmp as void*);
        this.ready = false;
    }

    size(w: int, hh: int) => void { this.w = w; this.ht = hh; }

    // Clears colour + depth.
    clear(r: float, g: float, b: float, a: float) => void {
        if (!this.ready) return;
        G.glfwMakeContextCurrent(this.h);
        OGL.viewport(this.w, this.ht);
        GL.glEnable(OGL.DEPTH_TEST);
        GL.glDepthMask(1 as u8);
        OGL.clear(r, g, b, a);
    }

    // Eye/target in world space, y up. Aspect comes from the window.
    camera(ex: float, ey: float, ez: float, tx: float, ty: float, tz: float, fovDeg: float) => void {
        if (!this.ready) return;
        let aspect: float = 1.0;
        if (this.ht > 0) aspect = (this.w as float) / (this.ht as float);
        matLookAt(this.view, ex, ey, ez, tx, ty, tz);
        matPerspective(this.proj, fovDeg, aspect, 0.1, 500.0);
        matMul(this.vp, this.proj, this.view);
    }

    light(x: float, y: float, z: float) => void { this.lightX = x; this.lightY = y; this.lightZ = z; }

    // Interleaved pos(3) normal(3) uv(2). idx may be nullptr / 0 for non-indexed.
    createMesh(out: OGL.Mesh*, verts: float*, floatCount: int, idx: u32*, idxCount: int) => void {
        if (!this.ready) return;
        G.glfwMakeContextCurrent(this.h);
        let layout = CMem.malloc(12) as int*;
        layout[0] = 3; layout[1] = 3; layout[2] = 2;
        out.uploadLayout(verts, floatCount, layout, 3);
        CMem.free(layout as void*);
        if (idxCount > 0) out.setIndices(idx, idxCount);
    }

    // Unit cube centred on the origin (24 verts, 36 indices).
    createCube(out: OGL.Mesh*) => void {
        if (!this.ready) return;
        let v = CMem.malloc(24 * 8 * 4) as float*;
        let ix = CMem.malloc(36 * 4) as u32*;
        // face: normal, u axis, v axis
        this.face(v, 0, 1.0, 0.0, 0.0,  0.0, 0.0, 0.0 - 1.0,  0.0, 1.0, 0.0);
        this.face(v, 1, 0.0 - 1.0, 0.0, 0.0,  0.0, 0.0, 1.0,  0.0, 1.0, 0.0);
        this.face(v, 2, 0.0, 1.0, 0.0,  1.0, 0.0, 0.0,  0.0, 0.0, 0.0 - 1.0);
        this.face(v, 3, 0.0, 0.0 - 1.0, 0.0,  1.0, 0.0, 0.0,  0.0, 0.0, 1.0);
        this.face(v, 4, 0.0, 0.0, 1.0,  1.0, 0.0, 0.0,  0.0, 1.0, 0.0);
        this.face(v, 5, 0.0, 0.0, 0.0 - 1.0,  0.0 - 1.0, 0.0, 0.0,  0.0, 1.0, 0.0);
        let f: int = 0;
        while (f < 6) {
            let b: u32 = (f * 4) as u32;
            let o: int = f * 6;
            ix[o] = b; ix[o + 1] = b + 1; ix[o + 2] = b + 2; ix[o + 3] = b; ix[o + 4] = b + 2; ix[o + 5] = b + 3;
            f = f + 1;
        }
        this.createMesh(out, v, 24 * 8, ix, 36);
        CMem.free(v as void*); CMem.free(ix as void*);
    }

    face(v: float*, f: int, nx: float, ny: float, nz: float, ux: float, uy: float, uz: float, vx: float, vy: float, vz: float) => void {
        let k: int = 0;
        while (k < 4) {
            let su: float = 0.0 - 0.5; let sv: float = 0.0 - 0.5;
            if (k == 1 || k == 2) su = 0.5;
            if (k == 2 || k == 3) sv = 0.5;
            let o: int = (f * 4 + k) * 8;
            v[o] = nx * 0.5 + ux * su + vx * sv;
            v[o + 1] = ny * 0.5 + uy * su + vy * sv;
            v[o + 2] = nz * 0.5 + uz * su + vz * sv;
            v[o + 3] = nx; v[o + 4] = ny; v[o + 5] = nz;
            v[o + 6] = su + 0.5; v[o + 7] = sv + 0.5;
            k = k + 1;
        }
    }

    // model: float*(16) column-major. Colour is 0xRRGGBBAA.
    draw(m: OGL.Mesh*, model: float*, c: u32) => void { this.drawTextured(m, model, c, 0); }

    // tex = GL texture id, 0 = untextured (colour only). Texture is multiplied by the colour.
    drawTextured(m: OGL.Mesh*, model: float*, c: u32, tex: u32) => void {
        if (!this.ready) return;
        G.glfwMakeContextCurrent(this.h);
        OGL.viewport(this.w, this.ht);
        GL.glEnable(OGL.DEPTH_TEST);
        GL.glDepthFunc(OGL.LESS);
        GL.glDisable(OGL.SCISSOR_TEST);
        OGL.enableBlend();
        this.prog.use();
        this.prog.setMat4("uVP", this.vp);
        this.prog.setMat4("uModel", model);
        this.prog.set4f("uColor", chan(c, 24), chan(c, 16), chan(c, 8), chan(c, 0));
        this.prog.set3f("uLight", this.lightX, this.lightY, this.lightZ);
        if (tex != 0) {
            OGL.glActiveTexture(OGL.TEXTURE0);
            GL.glBindTexture(OGL.TEXTURE_2D, tex);
            this.prog.set1i("uTex", 0);
            this.prog.set1i("uUseTex", 1);
        } else { this.prog.set1i("uUseTex", 0); }
        m.draw();
        OGL.glBindVertexArray(0);
        GL.glDisable(OGL.DEPTH_TEST);
    }
}
