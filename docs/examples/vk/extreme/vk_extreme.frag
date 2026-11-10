#version 450
// GPU stress test: animated Mandelbulb, raymarched, with `load` jittered samples per pixel.
// Push constants (16 bytes, must match PushConstants in vk_extreme.os).
layout(push_constant) uniform PC {
    float resX;
    float resY;
    float time;
    float load;      // samples per pixel (ramped up over time by the host)
} pc;

layout(location = 0) in vec2 uv;
layout(location = 0) out vec4 outColor;

const int   MAX_STEPS = 160;
const float MAX_DIST  = 6.0;

float hash(vec2 p)
{
    return fract(sin(dot(p, vec2(127.1, 311.7))) * 43758.5453123);
}

// Distance estimator for the Mandelbulb of power P. `trap` returns an orbit trap for colouring.
float de(vec3 p, float P, out float trap)
{
    vec3 z = p;
    float dr = 1.0;
    float r = 0.0;
    trap = 1e9;
    for (int i = 0; i < 12; i++) {
        r = length(z);
        if (r > 2.0) break;
        float theta = acos(clamp(z.z / r, -1.0, 1.0)) * P;
        float phi   = atan(z.y, z.x) * P;
        dr = pow(r, P - 1.0) * P * dr + 1.0;
        float zr = pow(r, P);
        z = zr * vec3(sin(theta) * cos(phi), sin(phi) * sin(theta), cos(theta)) + p;
        trap = min(trap, dot(z, z));
    }
    return 0.5 * log(r) * r / dr;
}

vec3 normalAt(vec3 p, float P)
{
    float t;
    vec2 e = vec2(0.0008, -0.0008);
    return normalize(
        e.xyy * de(p + e.xyy, P, t) +
        e.yyx * de(p + e.yyx, P, t) +
        e.yxy * de(p + e.yxy, P, t) +
        e.xxx * de(p + e.xxx, P, t));
}

vec3 shade(vec2 fragCoord, vec2 res, float P)
{
    vec2 ndc = (2.0 * fragCoord - res) / res.y;
    ndc.y = -ndc.y;

    float a = pc.time * 0.25;
    float radius = 2.3 + 0.35 * sin(pc.time * 0.2);
    vec3 ro = vec3(radius * cos(a), 0.55 * sin(pc.time * 0.31), radius * sin(a));
    vec3 fw = normalize(-ro);
    vec3 rt = normalize(cross(vec3(0.0, 1.0, 0.0), fw));
    vec3 up = cross(fw, rt);
    vec3 rd = normalize(ndc.x * rt + ndc.y * up + 1.6 * fw);

    float t = 0.0;
    float trap = 0.0;
    int steps = 0;
    bool hit = false;
    for (int i = 0; i < MAX_STEPS; i++) {
        float d = de(ro + rd * t, P, trap);
        steps = i;
        if (d < 0.0003 * t) { hit = true; break; }
        t += d * 0.85;
        if (t > MAX_DIST) break;
    }

    vec3 bg = mix(vec3(0.02, 0.02, 0.06), vec3(0.10, 0.06, 0.18), 0.5 + 0.5 * ndc.y);
    if (!hit) {
        // soft glow where the ray grazed the fractal
        return bg + vec3(0.9, 0.5, 0.2) * pow(float(steps) / float(MAX_STEPS), 3.0) * 0.8;
    }

    vec3 p = ro + rd * t;
    vec3 n = normalAt(p, P);
    vec3 l = normalize(vec3(0.6, 0.8, -0.3));
    float diff = clamp(dot(n, l), 0.0, 1.0);
    float rim  = pow(1.0 - clamp(dot(n, -rd), 0.0, 1.0), 3.0);
    float ao   = 1.0 - float(steps) / float(MAX_STEPS);
    vec3 pal   = 0.5 + 0.5 * cos(6.2831853 * (sqrt(trap) * 0.8 + vec3(0.0, 0.33, 0.67)));
    vec3 col   = pal * (0.15 + 0.85 * diff) * ao + rim * vec3(0.4, 0.5, 0.9) * 0.5;
    return mix(col, bg, clamp(t / MAX_DIST, 0.0, 1.0) * 0.6);
}

void main()
{
    vec2 res = vec2(pc.resX, pc.resY);
    vec2 fc = uv * res;
    float P = 8.0 + 2.5 * sin(pc.time * 0.15);

    int n = max(int(pc.load), 1);
    vec3 acc = vec3(0.0);
    for (int s = 0; s < n; s++) {
        vec2 j = vec2(hash(fc + float(s) * 1.37), hash(fc.yx + float(s) * 2.71)) - 0.5;
        acc += shade(fc + j, res, P);
    }
    vec3 col = acc / float(n);
    col = pow(col, vec3(1.0 / 2.2));
    outColor = vec4(col, 1.0);
}
