#version 450

layout(location = 0) out vec4 FragColor;

// Push constants: 12 bytes (res.x, res.y, time)
layout(push_constant) uniform PC {
    vec2 u_resolution;
    float u_time;
};

float hash(vec2 p) {
    return fract(sin(dot(p, vec2(12.9898, 78.233))) * 43758.5453);
}

float starField(vec2 uv, float density, float brightness, float twinkle) {
    vec2 p = floor(uv * density);
    float r = hash(p);
    float star = smoothstep(0.95, 1.0,
        pow(1.0 - distance(fract(uv * density), vec2(0.5)), 20.0));

    float twinkleFactor = (sin(u_time * (r * 2.0 + 1.0) + r * 10.0) * 0.5 + 0.5) * twinkle;
    return star * brightness * (0.8 + twinkleFactor * 0.2) * r;
}

vec3 hsv2rgb(vec3 c) {
    vec4 K = vec4(1.0, 2.0 / 3.0, 1.0 / 3.0, 3.0);
    vec3 p = abs(fract(c.xxx + K.xyz) * 6.0 - K.www);
    return c.z * mix(K.xxx, clamp(p - K.xxx, 0.0, 1.0), c.y);
}

void main() {
    // Vulkan's gl_FragCoord has Y pointing down; flip to match the OpenGL look
    vec2 fc = vec2(gl_FragCoord.x, u_resolution.y - gl_FragCoord.y);
    vec2 uv = (fc * 2.0 - u_resolution) / u_resolution.y;

    vec3 color = vec3(0.0);

    float nebula1 = sin(uv.x * 3.0 + u_time * 0.5) * cos(uv.y * 2.0 + u_time * 0.3);
    float nebula2 = sin(uv.x * 5.0 - u_time * 0.7) * cos(uv.y * 4.0 - u_time * 0.4);
    float nebula3 = sin(uv.x * 2.0 + uv.y * 3.0 + u_time * 0.6);

    color += vec3(0.1, 0.02, 0.2) * (nebula1 * 0.5 + 0.5);
    color += vec3(0.05, 0.1, 0.3) * (nebula2 * 0.3 + 0.3);
    color += vec3(0.2, 0.05, 0.1) * (nebula3 * 0.2 + 0.2);
    color += vec3(0.02, 0.03, 0.1);

    vec2 starUV = (uv + 1.0) * 0.5;
    starUV.x *= u_resolution.x / u_resolution.y;

    float stars1 = starField(starUV, 15.0, 1.2, 0.8);
    float stars2 = starField(starUV, 8.0, 1.0, 0.6);
    float stars3 = starField(starUV, 4.0, 1.5, 0.9);
    float stars4 = starField(starUV, 2.0, 2.0, 1.2);

    float greenStars  = starField(starUV * 1.3, 6.0, 0.8, 0.5);
    float orangeStars = starField(starUV * 0.7, 3.5, 0.9, 0.6);
    float purpleStars = starField(starUV * 1.1, 5.5, 0.7, 0.4);
    float cyanStars   = starField(starUV * 0.9, 4.5, 0.6, 0.7);

    color += stars1 * vec3(1.0, 0.9, 0.8);
    color += stars2 * vec3(0.9, 0.9, 1.0);
    color += stars3 * vec3(1.0, 1.0, 0.7);
    color += stars4 * vec3(1.0, 0.8, 0.6);

    color += greenStars * vec3(0.3, 1.0, 0.4);
    color += orangeStars * vec3(1.0, 0.5, 0.1);
    color += purpleStars * vec3(0.8, 0.3, 1.0);
    color += cyanStars * vec3(0.2, 0.8, 1.0);

    float pinkStars   = starField(starUV * 1.4, 7.0, 0.5, 0.3);
    float yellowStars = starField(starUV * 0.6, 3.0, 0.7, 0.5);

    color += pinkStars * vec3(1.0, 0.4, 0.8);
    color += yellowStars * vec3(1.0, 1.0, 0.2);

    float dist = length(uv);
    vec3 gradientColor = hsv2rgb(vec3(dist * 0.3 + u_time * 0.1, 0.4, 0.8));
    color *= 1.0 - 0.2 * dist + 0.1 * gradientColor;

    float angle = atan(uv.y, uv.x);
    float radius = length(uv);
    float star = abs(sin(6.0 * angle + u_time));
    float intensity = smoothstep(0.4, 0.0, abs(radius - star * 0.3));

    vec3 rainbowColor = hsv2rgb(vec3(angle / 6.28318 + u_time * 0.2, 0.8, 1.0));
    color += intensity * rainbowColor * 2.0;

    float sparkle1 = sin(uv.x * 20.0 + u_time * 3.0) * cos(uv.y * 20.0 + u_time * 2.0);
    float sparkle2 = sin(uv.x * 15.0 - u_time * 2.5) * cos(uv.y * 15.0 - u_time * 3.5);

    color += vec3(0.02, 0.01, 0.03) * sparkle1 * sparkle1;
    color += vec3(0.01, 0.03, 0.02) * sparkle2 * sparkle2;

    color *= 0.5;

    FragColor = vec4(color, 1.0);
}
