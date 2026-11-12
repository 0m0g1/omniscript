#version 450
layout(location=0) in vec2 vUV;
layout(location=1) in vec4 vCol;
layout(push_constant) uniform PC { vec2 uScreen; vec2 uSize; float uTime; } pc;
layout(location=0) out vec4 o;
void main() {
  vec2 p = vUV * 6.0; float t = pc.uTime;
  float v = sin(p.x + t) + sin(p.y * 1.3 - t) + sin((p.x + p.y) * 0.7 + t * 0.5);
  vec3 c = vec3(0.5 + 0.5 * sin(v), 0.5 + 0.5 * sin(v + 2.0), 0.5 + 0.5 * sin(v + 4.0));
  o = vec4(pow(c, vec3(2.2)), 1.0);               // framebuffer is sRGB: output linear
}
