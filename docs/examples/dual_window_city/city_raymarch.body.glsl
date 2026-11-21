// Raymarched neon city: GLSL body for Pen2D.createShaderBoth.
// Uses only vUV (0..1 across the rect) and uTime. No preprocessor lines, so it can be one string literal.
const float S = 1.25;
const float ASPECT = 1.3333333;
const float TAU = 6.2831853;

float hgt(vec2 c, float t) {
    float d = length(c);
    float h = 0.9 + 0.55 * sin(d * 0.85 - t * 2.4) + 0.35 * sin(c.x * 0.5 + t * 1.3) * cos(c.y * 0.5 - t * 0.9);
    return max(h, 0.15);
}

float sdBox(vec3 p, vec3 b) {
    vec3 q = abs(p) - b;
    return length(max(q, 0.0)) + min(max(q.x, max(q.y, q.z)), 0.0);
}

mat2 rot(float a) {
    float c = cos(a);
    float s = sin(a);
    return mat2(c, -s, s, c);
}

float city(vec3 p, float t, out float hue) {
    vec2 base = floor(p.xz / S);
    float best = 1e9;
    hue = 0.0;
    for (int j = 0; j < 2; j++) {
        for (int i = 0; i < 2; i++) {
            vec2 k = base + vec2(float(i), float(j));
            if (abs(k.x) > 10.0 || abs(k.y) > 10.0) continue;
            vec2 c = k * S;
            float h = hgt(c, t);
            float d = sdBox(vec3(p.x - c.x, p.y - h * 0.5, p.z - c.y), vec3(S * 0.39, h * 0.5, S * 0.39));
            if (d < best) { best = d; hue = length(c) * 0.035 + t * 0.05 + h * 0.12; }
        }
    }
    return best;
}

float props(vec3 p, float t, out float hue) {
    vec3 q = p - vec3(0.0, 6.5 + 0.6 * sin(t * 1.5), 0.0);
    q.xz = rot(t * 0.9) * q.xz;
    q.yz = rot(t * 0.6) * q.yz;
    float d = sdBox(q, vec3(1.3));
    hue = t * 0.12;
    for (int s = 0; s < 4; s++) {
        float a = t * 1.4 + float(s) * 1.5707963;
        vec3 sp = p - vec3(5.0 * cos(a), 6.5 + 1.8 * sin(a * 2.0), 5.0 * sin(a));
        sp.xz = rot(t * 2.0) * sp.xz;
        float ds = sdBox(sp, vec3(0.45));
        if (ds < d) { d = ds; hue = t * 0.12 + float(s) * 0.25; }
    }
    return d;
}

float mapScene(vec3 p, float t, out float hue) {
    float h1;
    float h2;
    float a = city(p, t, h1);
    float b = props(p, t, h2);
    float d = a;
    hue = h1;
    if (b < d) { d = b; hue = h2; }
    if (p.y < d) { d = p.y; hue = -1.0; }
    return d;
}

vec3 palette(float h) {
    return 0.5 + 0.5 * sin(TAU * (h + vec3(0.0, 0.333, 0.667)));
}

vec3 calcNormal(vec3 p, float t) {
    float e = 0.004;
    float hh;
    vec2 k = vec2(1.0, -1.0);
    return normalize(k.xyy * mapScene(p + k.xyy * e, t, hh) + k.yyx * mapScene(p + k.yyx * e, t, hh) + k.yxy * mapScene(p + k.yxy * e, t, hh) + k.xxx * mapScene(p + k.xxx * e, t, hh));
}

void main() {
    float t = uTime;
    vec2 uv = vec2((vUV.x - 0.5) * ASPECT, 0.5 - vUV.y);
    float ang = t * 0.15;
    vec3 ro = vec3(27.0 * sin(ang), 13.0, 27.0 * cos(ang));
    vec3 f = normalize(vec3(0.0, 1.2, 0.0) - ro);
    vec3 r = normalize(cross(f, vec3(0.0, 1.0, 0.0)));
    vec3 u = cross(r, f);
    float focal = 0.5 / tan(radians(55.0) * 0.5);
    vec3 rd = normalize(uv.x * r + uv.y * u + focal * f);

    vec3 bg = vec3(0.015, 0.02, 0.05);
    float dist = 0.0;
    float hue = 0.0;
    float glow = 0.0;
    bool hit = false;
    for (int i = 0; i < 110; i++) {
        vec3 p = ro + rd * dist;
        float d = mapScene(p, t, hue);
        glow += exp(-d * 2.5) * 0.006;
        if (d < 0.002 + 0.0015 * dist) { hit = true; break; }
        dist += d * 0.85;
        if (dist > 80.0) break;
    }

    vec3 col = bg + vec3(0.25, 0.1, 0.4) * glow;
    if (hit) {
        vec3 p = ro + rd * dist;
        vec3 n = calcNormal(p, t);
        vec3 l = normalize(vec3(cos(t * 0.4), 0.9, sin(t * 0.4)));
        float diff = max(dot(n, l), 0.0) * 0.75 + 0.25;
        vec3 base;
        if (hue < -0.5) {
            vec2 gp = abs(fract(p.xz / S + 0.5) - 0.5);
            float line = smoothstep(0.03, 0.0, min(gp.x, gp.y));
            base = vec3(0.03, 0.04, 0.08) + vec3(0.1, 0.5, 0.8) * line * 0.5;
            diff = 1.0;
        } else {
            base = palette(hue);
        }
        float rim = pow(1.0 - max(dot(n, -rd), 0.0), 3.0);
        col = base * diff + base * rim * 0.5 + vec3(0.25, 0.1, 0.4) * glow;
        col = mix(col, bg, 1.0 - exp(-dist * 0.018));
    }
    o = vec4(sqrt(clamp(col, 0.0, 1.0)), 1.0);
}
