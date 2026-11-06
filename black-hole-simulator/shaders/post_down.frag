#version 330 core
// Downsample (4 bilinear taps = 4x4 box) with an optional bright-pass.
uniform sampler2D uTex;
uniform vec2  uSrcTexel;     // 1 / source size
uniform vec2  uDstSize;
uniform float uThreshold;    // <= 0 : plain downsample
out vec4 FragColor;

void main() {
    vec2 uv = gl_FragCoord.xy / uDstSize;
    vec2 o = uSrcTexel * 0.5;
    vec3 c = texture(uTex, uv + vec2(-o.x, -o.y)).rgb
           + texture(uTex, uv + vec2( o.x, -o.y)).rgb
           + texture(uTex, uv + vec2(-o.x,  o.y)).rgb
           + texture(uTex, uv + vec2( o.x,  o.y)).rgb;
    c *= 0.25;
    c = min(c, vec3(40.0));                       // tame fireflies
    if (uThreshold > 0.0) {
        float l = max(c.r, max(c.g, c.b));
        c *= smoothstep(uThreshold, uThreshold + 1.2, l);
    }
    FragColor = vec4(c, 1.0);
}
