#version 450
// Fullscreen triangle, no vertex buffers. uv covers [0,1] inside the visible area, (0,0) = top-left.
layout(location = 0) out vec2 uv;

void main()
{
    vec2 p = vec2(float((gl_VertexIndex << 1) & 2), float(gl_VertexIndex & 2));
    uv = p;
    gl_Position = vec4(p * 2.0 - 1.0, 0.0, 1.0);
}
