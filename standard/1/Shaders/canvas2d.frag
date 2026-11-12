#version 450
layout(location=0) in vec2 vUV;
layout(location=1) in vec4 vCol;
layout(set=0, binding=0) uniform sampler2D uTex;
layout(location=0) out vec4 o;
void main() {
  vec4 t = texture(uTex, vUV);                    // texture is sRGB: already linear here
  vec3 tint = pow(vCol.rgb, vec3(2.2));           // vertex colours are display values
  o = vec4(t.rgb * tint, t.a * vCol.a);
}
