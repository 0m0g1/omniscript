// standard/1/Canvas.os
// Tier 7: the display root. Canvas creates windows; every Win owns one GL backend and two pens:
//   Canvas  app root: createWindow(), frame(), present(), time(), quit()
//   Win     one native window + context. Input (mouse/keys/typed text, with per-frame edges) lives here.
//   Pen2D   batched 2D drawing: fillRect, rect, line, circle, triangle, text, image, clip, shaderRect.
//           Everything draws through ONE texture atlas (font + white pixel) so a whole GUI is 1 draw call.
//           2D is flushed in present(), so it always lands on top of 3D drawn earlier in the frame.
//   Pen3D   immediate 3D: meshes, camera, lit shader, textures, depth. Draws on the call.
// Gui.os sits on a Pen2D + Win and never touches GL.
//
// Backends (one per window, chosen at creation; Pen2D code is identical on all three):
//   API_GL        OpenGL 3.3 core. Windows share textures. Pen2D + Pen3D.
//   API_VULKAN    Vulkan 1.1 through Vulkan.os: one shared device (initCore) + one swapchain per window (attach).
//                 Pen2D only; for 3D use win.vk (a Vulkan.os target) directly. Needs shaders/*.spv next to the
//                 working directory (or app.shaderDir).
//   API_SOFTWARE  CPU rasteriser into an RGBA buffer (pen.pixels()); shown in a GL window by a texture blit, or
//                 used headless via Pen2D.initSoftware() with no window at all. Custom 2D shaders are skipped.
// Every layer also works on its own: Window.os, OpenGL.os, Vulkan.os are used exactly as before, and a Pen2D
// can be driven without Canvas: pen.init(glWindow) / pen.initVulkan(&vk, dir) / pen.initSoftware(w, h).
// Portable images: pen.createImage(rgba, w, h) -> id for pen.image(); portable shaders: pen.createShaderBoth().
// Colours are 0xRRGGBBAA. Not compiled/run yet. No `return` inside loop bodies (compiler quirk).
module W = import "standard/1/Window.os";
module G = import "standard/1/c/glfw.os";
module GL = import "standard/1/c/opengl.os";
module OGL = import "standard/1/OpenGL.os";
module F = import "standard/1/FontData.os";
module L = import "standard/1/List.os";
module LF = import "standard/1/ListF32.os";
module LU = import "standard/1/ListU32.os";
module VK = import "standard/1/Vulkan.os";
module VKC = import "standard/1/c/vulkan.os";
module CMem = import "standard/1/c/mem.os";
module Console = import "standard/1/Console.os";

// Implementation is split under standard/1/Canvas/; `include` pastes each file in place (order matters).
include "standard/1/Canvas/Constants.os";
include "standard/1/Canvas/Math.os";
include "standard/1/Canvas/Shaders.os";
include "standard/1/Canvas/Util.os";
include "standard/1/Canvas/Types.os";
include "standard/1/Canvas/Pen2D.os";
include "standard/1/Canvas/Pen3D.os";
include "standard/1/Canvas/Win.os";
include "standard/1/Canvas/Root.os";
