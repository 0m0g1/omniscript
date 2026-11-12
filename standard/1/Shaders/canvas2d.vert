#version 450
layout(location=0) in vec2 aPos;
layout(location=1) in vec2 aUV;
layout(location=2) in vec4 aCol;
layout(push_constant) uniform PC { vec2 uScreen; vec2 uSize; float uTime; } pc;
layout(location=0) out vec2 vUV;
layout(location=1) out vec4 vCol;
void main() {
  vec2 p = aPos / pc.uScreen * 2.0 - 1.0;
  gl_Position = vec4(p, 0.0, 1.0);
  vUV = aUV; vCol = aCol;
}
