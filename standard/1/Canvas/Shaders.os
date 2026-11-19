// ---------------------------------------------------------------------------------------------
// shaders
// ---------------------------------------------------------------------------------------------
function vs2d() => char* {
    return "#version 330 core\nlayout(location=0) in vec2 aPos;\nlayout(location=1) in vec2 aUV;\nlayout(location=2) in vec4 aCol;\nuniform vec2 uScreen;\nout vec2 vUV;\nout vec4 vCol;\nvoid main() {\n  vec2 p = aPos / uScreen * 2.0 - 1.0;\n  gl_Position = vec4(p.x, -p.y, 0.0, 1.0);\n  vUV = aUV; vCol = aCol;\n}\n";
}
function fs2d() => char* {
    return "#version 330 core\nin vec2 vUV;\nin vec4 vCol;\nuniform sampler2D uTex;\nout vec4 o;\nvoid main() { o = texture(uTex, vUV) * vCol; }\n";
}
// User 2D shaders: write only the fragment shader. You get vUV (0..1 across the rect), vCol,
// uSize (rect size in px), uTime (seconds). Output to `o`.
function fs2dPrefix() => char* {
    return "#version 330 core\nin vec2 vUV;\nin vec4 vCol;\nuniform vec2 uSize;\nuniform float uTime;\nout vec4 o;\n";
}
function vs3d() => char* {
    return "#version 330 core\nlayout(location=0) in vec3 aPos;\nlayout(location=1) in vec3 aN;\nlayout(location=2) in vec2 aUV;\nuniform mat4 uVP;\nuniform mat4 uModel;\nout vec3 vN;\nout vec2 vUV;\nvoid main() {\n  vN = mat3(uModel) * aN; vUV = aUV;\n  gl_Position = uVP * uModel * vec4(aPos, 1.0);\n}\n";
}
function fs3d() => char* {
    return "#version 330 core\nin vec3 vN;\nin vec2 vUV;\nuniform vec4 uColor;\nuniform vec3 uLight;\nuniform sampler2D uTex;\nuniform int uUseTex;\nout vec4 o;\nvoid main() {\n  float d = max(dot(normalize(vN), normalize(uLight)), 0.0);\n  vec4 base = uColor;\n  if (uUseTex != 0) base *= texture(uTex, vUV);\n  o = vec4(base.rgb * (0.25 + 0.75 * d), base.a);\n}\n";
}
