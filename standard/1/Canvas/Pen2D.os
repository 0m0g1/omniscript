struct Pen2D {
    h: void* = nullptr;                 // GLFW window (to make its context current)
    verts: LF.ListF32;                  // x y u v r g b a  per vertex
    idx: LU.ListU32;
    cmds: L.List<DrawCmd>;
    prog: OGL.Program;
    vao: u32 = 0; vbo: u32 = 0; ebo: u32 = 0;
    atlas: u32 = 0;
    vcount: u32 = 0;
    w: int = 0; ht: int = 0;
    clipStack: int* = nullptr;          // 16 levels x 4 ints
    clipDepth: int = 0;
    clearR: float = 0.1; clearG: float = 0.1; clearB: float = 0.12; clearA: float = 1.0;
    ready: bool = false;
    backend: int = 0;                   // API_GL / API_VULKAN / API_SOFTWARE
    // portable image table (Vulkan + software): id = slot + 1, slot 0 is the font atlas (id 1)
    imgUsed: u8* = nullptr; imgW: int* = nullptr; imgH: int* = nullptr; imgPx: u8** = nullptr;
    // software backend
    swBuf: u8* = nullptr; swW: int = 0; swH: int = 0;
    // vulkan backend
    vk: VK.Vulkan* = nullptr;
    vkPipe: VK.Pipeline;
    vkShaders: VK.Pipeline* = nullptr; vkShaderCount: int = 0;
    vkTex: VK.Texture* = nullptr;
    vkVb: VK.Buffer* = nullptr; vkIb: VK.Buffer* = nullptr;
    vkPc: float* = nullptr;
    shaderDir: char* = nullptr;
    // widgets: created with createButton/createSlider/..., updated in Win.beginFrame, drawn in Win.endFrame
    inp: PenInput;
    widgets: Widget** = nullptr;
    wcount: int = 0;
    activeW: Widget* = nullptr;         // widget the mouse went down on
    focusW: Widget* = nullptr;          // focused widget (any focusable kind)
    kbFocus: bool = false;              // focus arrived by keyboard (Tab): draw the focus ring
    clip: char* = nullptr; clipLen: int = 0;     // in-app clipboard for textbox copy/cut/paste
    noDrag: bool = false;               // a double-click word selection is active: ignore drag until release
    wantsMouse: bool = false;           // true while the mouse is over any widget: game code can ignore the click

    include "standard/1/Canvas/Pen2D/Init.os";
    include "standard/1/Canvas/Pen2D/Images.os";
    include "standard/1/Canvas/Pen2D/Frame.os";
    include "standard/1/Canvas/Pen2D/Shaders.os";
    include "standard/1/Canvas/Pen2D/Clip.os";
    include "standard/1/Canvas/Pen2D/Batch.os";
    include "standard/1/Canvas/Pen2D/Shapes.os";
    include "standard/1/Canvas/Pen2D/Text.os";
    include "standard/1/Canvas/Pen2D/Widgets.os";
    include "standard/1/Canvas/Pen2D/TextBox.os";
    include "standard/1/Canvas/Pen2D/WidgetUpdate.os";
    include "standard/1/Canvas/Pen2D/Vulkan.os";
    include "standard/1/Canvas/Pen2D/Software.os";
    include "standard/1/Canvas/Pen2D/GL.os";
}
