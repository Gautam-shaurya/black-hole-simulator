#version 330 core
layout(location = 0) in vec3  aPos;
layout(location = 1) in vec4  aColor;   // rgb + brightness
layout(location = 2) in float aSize;    // world-space size (SU)

uniform mat4  uViewProj;
uniform vec3  uCamPos;
uniform float uRs;
uniform float uPixelScale;   // pixels per world unit at distance 1
uniform float uBright;

out vec4 vColor;

void main() {
    vec4 clip = uViewProj * vec4(aPos, 1.0);
    gl_Position = clip;

    // Hide particles that are BEHIND the black hole's shadow (impact parameter
    // below the critical value 3*sqrt(3)/2 rs): their light would be captured.
    vec3 toP = aPos - uCamPos;
    float L = length(toP);
    vec3 rd = toP / L;
    float tca = dot(-uCamPos, rd);              // distance along the ray to closest approach
    float b = length(uCamPos + rd * tca);       // impact parameter relative to the hole
    float bcrit = 2.598 * uRs;
    float vis = 1.0;
    if (tca > 0.0 && tca < L) vis = smoothstep(bcrit * 0.97, bcrit * 1.10, b);

    gl_PointSize = clamp(aSize * uPixelScale / max(clip.w, 0.01), 1.5, 22.0);
    vColor = vec4(aColor.rgb * uBright, aColor.a * vis);
}
