    flushGL() => void {
        let n: u64 = this.cmds.len;
        if (n == 0 || this.w == 0 || this.ht == 0) { this.verts.len = 0; this.idx.len = 0; this.cmds.len = 0; return; }
        G.glfwMakeContextCurrent(this.h);
        OGL.viewport(this.w, this.ht);
        GL.glDisable(OGL.DEPTH_TEST);
        GL.glDisable(OGL.CULL_FACE);
        OGL.enableBlend();
        GL.glEnable(OGL.SCISSOR_TEST);
        OGL.glBindVertexArray(this.vao);
        OGL.glBindBuffer(OGL.ARRAY_BUFFER, this.vbo);
        OGL.glBufferData(OGL.ARRAY_BUFFER, (this.verts.len * 4) as i64, this.verts.data as void*, OGL.DYNAMIC_DRAW);
        OGL.glBindBuffer(OGL.ELEMENT_ARRAY_BUFFER, this.ebo);
        OGL.glBufferData(OGL.ELEMENT_ARRAY_BUFFER, (this.idx.len * 4) as i64, this.idx.data as void*, OGL.DYNAMIC_DRAW);
        let t: float = G.glfwGetTime() as float;
        let i: u64 = 0;
        while (i < n) {
            let d: DrawCmd = this.cmds.data[i];
            let p: u32 = this.prog.id;
            if (d.prog != 0) p = d.prog;
            OGL.glUseProgram(p);
            OGL.glUniform2f(OGL.glGetUniformLocation(p, "uScreen"), this.w as float, this.ht as float);
            if (d.prog == 0) {
                OGL.glActiveTexture(OGL.TEXTURE0);
                GL.glBindTexture(OGL.TEXTURE_2D, d.tex);
                OGL.glUniform1i(OGL.glGetUniformLocation(p, "uTex"), 0);
            } else {
                OGL.glUniform2f(OGL.glGetUniformLocation(p, "uSize"), d.rw, d.rh);
                OGL.glUniform1f(OGL.glGetUniformLocation(p, "uTime"), t);
            }
            if (d.clipOn != 0) { GL.glScissor(d.cx, this.ht - d.cy - d.ch, d.cw, d.ch); }
            else { GL.glScissor(0, 0, this.w, this.ht); }
            GL.glDrawElements(OGL.TRIANGLES, d.count as int, OGL.UNSIGNED_INT, ((d.first * 4) as i64) as void*);
            i = i + 1;
        }
        GL.glDisable(OGL.SCISSOR_TEST);
        OGL.glBindVertexArray(0);
        this.verts.len = 0; this.idx.len = 0; this.cmds.len = 0; this.vcount = 0;
    }

