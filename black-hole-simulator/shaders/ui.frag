#version 330 core
in vec2 vLocal;
in vec4 vColor;
in vec4 vShape;
uniform float uScale;                   // framebuffer pixels per logical unit
out vec4 FragColor;

void main() {
    float d;
    if (vShape.w < 0.5) {
        // stroke: capsule around the segment (0,0)-(L,0), half width in vShape.y
        float L = vShape.x;
        d = length(vec2(vLocal.x - clamp(vLocal.x, 0.0, L), vLocal.y)) - vShape.y;
    } else {
        // rounded box centred on the origin: half size (x,y), corner radius z
        vec2 q = abs(vLocal) - vShape.xy + vShape.z;
        d = length(max(q, 0.0)) + min(max(q.x, q.y), 0.0) - vShape.z;
        if (vShape.w > 1.5) d = abs(d) - 0.5 / uScale;     // hairline outline
    }
    float a = clamp(0.5 - d * uScale, 0.0, 1.0);            // 1 pixel anti-aliasing
    FragColor = vec4(vColor.rgb, vColor.a * a);
}
