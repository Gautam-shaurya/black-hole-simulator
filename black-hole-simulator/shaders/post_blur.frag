#version 330 core
// Separable 9-tap Gaussian blur.
uniform sampler2D uTex;
uniform vec2 uDir;       // texel step along the blur axis
uniform vec2 uDstSize;
out vec4 FragColor;

void main() {
    vec2 uv = gl_FragCoord.xy / uDstSize;
    const float w[5] = float[5](0.227027, 0.1945946, 0.1216216, 0.054054, 0.016216);
    vec3 c = texture(uTex, uv).rgb * w[0];
    for (int i = 1; i < 5; ++i) {
        c += texture(uTex, uv + uDir * float(i)).rgb * w[i];
        c += texture(uTex, uv - uDir * float(i)).rgb * w[i];
    }
    FragColor = vec4(c, 1.0);
}
