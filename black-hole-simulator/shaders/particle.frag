#version 330 core
in vec4 vColor;
uniform float uIsPoint;   // 1 = point sprite, 0 = trail line
out vec4 FragColor;

void main() {
    float a = vColor.a;
    if (uIsPoint > 0.5) {
        float d = length(gl_PointCoord - vec2(0.5)) * 2.0;
        if (d > 1.0) discard;
        a *= pow(1.0 - d, 1.6);                // soft glowing sprite
    }
    FragColor = vec4(vColor.rgb * a, 1.0);      // additive blending (ONE, ONE)
}
