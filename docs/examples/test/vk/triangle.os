// examples/vk/triangle.os: Vulkan: vertex-buffer triangle + rotating textured quad (push constant),
// resize-safe. Needs shaders/*.spv next to the working directory (glslangValidator -V x -o x.spv).
module W = import "standard/1/Window.os";
module VK = import "standard/1/Vulkan.os";
module CMem = import "standard/1/c/mem.os";

function main() => int {
    if (!W.init()) return 1;
    let win: W.Window;
    if (!win.openVulkan(800, 600, "OmniScript Vulkan")) { W.terminate(); return 2; }

    let vk: VK.Vulkan;
    if (!vk.initEx(&win, "omniscript-demo", true)) { win.close(); W.terminate(); return 3; }

    // triangle: vec2 pos + vec3 colour
    let tv = CMem.malloc(15 * 4) as float*;
    tv[0] = -0.5; tv[1] = 0.5; tv[2] = 1.0; tv[3] = 0.0; tv[4] = 0.0;
    tv[5] = 0.5;  tv[6] = 0.5; tv[7] = 0.0; tv[8] = 1.0; tv[9] = 0.0;
    tv[10] = 0.0; tv[11] = -0.5; tv[12] = 0.0; tv[13] = 0.0; tv[14] = 1.0;
    let triBuf: VK.Buffer;
    if (!vk.createDeviceBuffer(&triBuf, tv as void*, 60, VK.VK_BUFFER_USAGE_VERTEX_BUFFER_BIT)) return 4;

    // quad: vec2 pos + vec2 uv, indexed
    let qv = CMem.malloc(16 * 4) as float*;
    qv[0] = -0.4; qv[1] = -0.4; qv[2] = 0.0; qv[3] = 0.0;
    qv[4] =  0.4; qv[5] = -0.4; qv[6] = 1.0; qv[7] = 0.0;
    qv[8] =  0.4; qv[9] =  0.4; qv[10] = 1.0; qv[11] = 1.0;
    qv[12] = -0.4; qv[13] = 0.4; qv[14] = 0.0; qv[15] = 1.0;
    let qi = CMem.malloc(24) as u32*;
    qi[0] = 0; qi[1] = 1; qi[2] = 2; qi[3] = 2; qi[4] = 3; qi[5] = 0;
    let quadV: VK.Buffer; let quadI: VK.Buffer;
    if (!vk.createDeviceBuffer(&quadV, qv as void*, 64, VK.VK_BUFFER_USAGE_VERTEX_BUFFER_BIT)) return 5;
    if (!vk.createDeviceBuffer(&quadI, qi as void*, 24, VK.VK_BUFFER_USAGE_INDEX_BUFFER_BIT)) return 6;

    // checkerboard texture 8x8 RGBA
    let px = CMem.malloc(8 * 8 * 4) as u8*;
    let y: int = 0;
    while (y < 8) {
        let x: int = 0;
        while (x < 8) {
            let on: bool = ((x + y) % 2) == 0;
            let o: int = (y * 8 + x) * 4;
            if (on) { px[o] = 240; px[o + 1] = 200; px[o + 2] = 60; } else { px[o] = 40; px[o + 1] = 40; px[o + 2] = 90; }
            px[o + 3] = 255;
            x = x + 1;
        }
        y = y + 1;
    }
    let tex: VK.Texture;
    if (!vk.createTexture(&tex, px, 8, 8)) return 7;

    let triVS: u64 = vk.loadShader("shaders/tri.vert.spv");
    let triFS: u64 = vk.loadShader("shaders/tri.frag.spv");
    let texVS: u64 = vk.loadShader("shaders/tex.vert.spv");
    let texFS: u64 = vk.loadShader("shaders/tex.frag.spv");
    if (triVS == 0 || triFS == 0 || texVS == 0 || texFS == 0) return 8;

    let f1 = CMem.malloc(8) as u32*; f1[0] = VK.VK_FORMAT_R32G32_SFLOAT; f1[1] = VK.VK_FORMAT_R32G32B32_SFLOAT;
    let o1 = CMem.malloc(8) as u32*; o1[0] = 0; o1[1] = 8;
    let f2 = CMem.malloc(8) as u32*; f2[0] = VK.VK_FORMAT_R32G32_SFLOAT; f2[1] = VK.VK_FORMAT_R32G32_SFLOAT;
    let o2 = CMem.malloc(8) as u32*; o2[0] = 0; o2[1] = 8;
    let triPipe: VK.Pipeline; let texPipe: VK.Pipeline;
    if (!vk.createPipeline(&triPipe, triVS, triFS, 20, 2, f1, o1, false, 0)) return 9;
    if (!vk.createPipeline(&texPipe, texVS, texFS, 16, 2, f2, o2, true, 4)) return 10;

    let t: float = 0.0;
    while (!win.shouldClose()) {
        W.pollEvents();
        if (win.keyDown(W.KEY_ESCAPE)) win.requestClose();
        if (vk.beginFrame()) {
            vk.bindPipeline(&triPipe);
            vk.bindVertexBuffer(&triBuf);
            vk.draw(3);

            t = W.time() as float;
            vk.bindPipeline(&texPipe);
            vk.bindTexture(&texPipe, &tex);
            vk.pushConstants(&texPipe, &t as void*, 4);
            vk.bindVertexBuffer(&quadV);
            vk.bindIndexBuffer(&quadI, VK.VK_INDEX_U32);
            vk.drawIndexed(6);
            vk.endFrame();
        }
    }

    vk.waitIdle();
    vk.destroyPipeline(&triPipe); vk.destroyPipeline(&texPipe);
    vk.destroyShader(triVS); vk.destroyShader(triFS); vk.destroyShader(texVS); vk.destroyShader(texFS);
    vk.destroyTexture(&tex);
    vk.destroyBuffer(&triBuf); vk.destroyBuffer(&quadV); vk.destroyBuffer(&quadI);
    vk.free();
    win.close();
    W.terminate();
    return 0;
}
