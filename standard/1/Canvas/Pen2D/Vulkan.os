    // ---- backends: draw everything recorded this frame ----
    flush() => void {
        if (this.backend == API_VULKAN) { this.flushVulkan(); }
        else if (this.backend == API_SOFTWARE) { this.flushSoftware(); }
        else { this.flushGL(); }
    }

    reset() => void {
        this.verts.len = 0; this.idx.len = 0; this.cmds.len = 0; this.vcount = 0;
    }

    freeVulkan() => void {
        let vk: VK.Vulkan* = this.vk;
        vk.waitIdle();
        let i: int = 0;
        while (i < 2) {
            vk.destroyBuffer(&this.vkVb[i]); vk.destroyBuffer(&this.vkIb[i]);
            i = i + 1;
        }
        i = 0;
        while (i < MAX_IMAGES) {
            if (this.imgUsed[i] != 0) vk.destroyTexture(&this.vkTex[i]);
            i = i + 1;
        }
        i = 0;
        while (i < this.vkShaderCount) { vk.destroyPipeline(&this.vkShaders[i]); i = i + 1; }
        vk.destroyPipeline(&this.vkPipe);
        CMem.free(this.vkVb as void*); CMem.free(this.vkIb as void*); CMem.free(this.vkTex as void*);
        CMem.free(this.vkShaders as void*); CMem.free(this.vkPc as void*);
    }

    // Grows slot's host-visible vertex/index buffers (safe: that slot's fence was waited in beginFrame).
    vkEnsure(slot: u32, vbytes: u64, ibytes: u64) => bool {
        let vk: VK.Vulkan* = this.vk;
        let vb: VK.Buffer* = &this.vkVb[slot];
        let ib: VK.Buffer* = &this.vkIb[slot];
        if (vb.size < vbytes) {
            vk.destroyBuffer(vb);
            let want: u64 = vbytes * 2;
            if (want < 65536) want = 65536;
            if (!vk.createBuffer(vb, want, VK.VK_BUFFER_USAGE_VERTEX_BUFFER_BIT, VK.VK_MEMORY_HOST_VISIBLE_COHERENT)) return false;
        }
        if (ib.size < ibytes) {
            vk.destroyBuffer(ib);
            let want2: u64 = ibytes * 2;
            if (want2 < 32768) want2 = 32768;
            if (!vk.createBuffer(ib, want2, VK.VK_BUFFER_USAGE_INDEX_BUFFER_BIT, VK.VK_MEMORY_HOST_VISIBLE_COHERENT)) return false;
        }
        return true;
    }

    flushVulkan() => void {
        let vk: VK.Vulkan* = this.vk;
        vk.setClear(this.clearR, this.clearG, this.clearB, this.clearA);
        if (!vk.beginFrame()) { this.reset(); return; }       // minimised / swapchain rebuilt: skip this frame
        let n: u64 = this.cmds.len;
        let okBuf: bool = false;
        if (n > 0) {
            let slot: u32 = vk.frame;
            let vbytes: u64 = this.verts.len * 4;
            let ibytes: u64 = this.idx.len * 4;
            okBuf = this.vkEnsure(slot, vbytes, ibytes);
            if (okBuf) {
                vk.writeBuffer(&this.vkVb[slot], this.verts.data as void*, vbytes);
                vk.writeBuffer(&this.vkIb[slot], this.idx.data as void*, ibytes);
                vk.bindVertexBuffer(&this.vkVb[slot]);
                vk.bindIndexBuffer(&this.vkIb[slot], VK.VK_INDEX_U32);
                let t: float = G.glfwGetTime() as float;
                let ew: int = vk.extent.width as int;
                let eh: int = vk.extent.height as int;
                let i: u64 = 0;
                while (i < n) {
                    let d: DrawCmd = this.cmds.data[i];
                    let pipe: VK.Pipeline* = &this.vkPipe;
                    if (d.prog != 0) pipe = &this.vkShaders[(d.prog as int) - 1];
                    vk.bindPipeline(pipe);
                    this.vkPc[0] = ew as float; this.vkPc[1] = eh as float;
                    this.vkPc[2] = d.rw; this.vkPc[3] = d.rh; this.vkPc[4] = t;
                    vk.pushConstants(pipe, this.vkPc as void*, 20);
                    if (d.prog == 0) vk.bindTexture(pipe, &this.vkTex[(d.tex as int) - 1]);
                    let sc: VK.VkRect2D;
                    let x0: int = 0; let y0: int = 0; let x1: int = ew; let y1: int = eh;
                    if (d.clipOn != 0) {
                        x0 = clampi(d.cx, 0, ew); y0 = clampi(d.cy, 0, eh);
                        x1 = clampi(d.cx + d.cw, 0, ew); y1 = clampi(d.cy + d.ch, 0, eh);
                    }
                    sc.offset.x = x0; sc.offset.y = y0;
                    sc.extent.width = (x1 - x0) as u32; sc.extent.height = (y1 - y0) as u32;
                    VKC.vkCmdSetScissor(vk.curCmd, 0, 1, &sc as void*);
                    VKC.vkCmdDrawIndexed(vk.curCmd, d.count, 1, d.first, 0, 0);
                    i = i + 1;
                }
            }
        }
        vk.endFrame();
        this.reset();
    }
