#version 330 core
// UI primitives are drawn as quads; the fragment shader evaluates a signed
// distance (line segment, filled rounded box, or hairline rounded box) so
// every edge is anti-aliased and crisp at any window size.
layout(location = 0) in vec2 aPos;      // logical UI units, origin top-left
layout(location = 1) in vec2 aLocal;    // shape-local coordinates
layout(location = 2) in vec4 aColor;
layout(location = 3) in vec4 aShape;    // x,y,z = shape parameters, w = type
uniform vec2 uScreen;                   // logical screen size
out vec2 vLocal;
out vec4 vColor;
out vec4 vShape;
void main() {
    gl_Position = vec4(aPos.x / uScreen.x * 2.0 - 1.0, 1.0 - aPos.y / uScreen.y * 2.0, 0.0, 1.0);
    vLocal = aLocal;
    vColor = aColor;
    vShape = aShape;
}
