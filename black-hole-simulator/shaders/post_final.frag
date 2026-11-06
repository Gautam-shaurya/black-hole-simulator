#version 330 core
// Final composite: bloom + exposure + ACES tone mapping + colour grading +
// vignette + subtle chromatic aberration + film grain + cinematic letterbox.
uniform sampler2D uScene;
uniform sampler2D uBloom0, uBloom1, uBloom2, uBloom3, uBloom4;
uniform vec2  uRes;
uniform float uBloomStrength;
uniform float uExposure;
uniform float uVignette;
uniform float uCinematic;     // 0..1 smooth blend
uniform float uLetterbox;     // fraction of the height covered by each bar
uniform float uTime;
out vec4 FragColor;

vec3 aces(vec3 x) {           // Narkowicz ACES fit
    return clamp((x * (2.51 * x + 0.03)) / (x * (2.43 * x + 0.59) + 0.14), 0.0, 1.0);
}
// Interleaved gradient noise: cheap, evenly distributed, ideal for dithering
float ign(vec2 p) { return fract(52.9829189 * fract(dot(p, vec2(0.06711056, 0.00583715)))); }

void main() {
    vec2 uv = gl_FragCoord.xy / uRes;
    vec2 cc = uv - 0.5;
    float dist = length(cc);

    // subtle lens chromatic aberration, stronger toward the edges
    float ca = (0.0005 + 0.0010 * uCinematic) * dist;
    vec3 hdr;
    hdr.r = texture(uScene, uv + cc * ca).r;
    hdr.g = texture(uScene, uv).g;
    hdr.b = texture(uScene, uv - cc * ca).b;

    vec3 bloom = texture(uBloom0, uv).rgb * 0.50
               + texture(uBloom1, uv).rgb * 0.45
               + texture(uBloom2, uv).rgb * 0.40
               + texture(uBloom3, uv).rgb * 0.35
               + texture(uBloom4, uv).rgb * 0.30;
    hdr += bloom * uBloomStrength;

    vec3 c = aces(hdr * uExposure);

    // atmospheric grading: cool shadows, warm highlights (stronger in cinematic mode)
    float lum = dot(c, vec3(0.2126, 0.7152, 0.0722));
    float gstr = 0.5 + 0.5 * uCinematic;
    c *= mix(vec3(1.0), vec3(0.93, 1.00, 1.10), (1.0 - smoothstep(0.0, 0.30, lum)) * gstr);
    c *= mix(vec3(1.0), vec3(1.05, 0.99, 0.93), smoothstep(0.35, 1.0, lum) * gstr);
    c = pow(c, vec3(1.0 / 2.2));

    float vig = 1.0 - uVignette * smoothstep(0.25, 0.85, dist * 1.35);
    c *= vig;

    // Triangular-PDF dither (+-1.5 / 255) removes banding in the dark gradients and
    // the soft disk falloff; a very faint extra grain appears in cinematic mode only.
    vec2 fp = gl_FragCoord.xy + floor(fract(uTime * 0.37) * 64.0) * vec2(17.0, 31.0);
    float tri = ign(fp) + ign(fp.yx * 1.37 + 11.0) - 1.0;
    c += tri * (1.5 / 255.0) + tri * 0.004 * uCinematic;

    if (uv.y < uLetterbox || uv.y > 1.0 - uLetterbox) c = vec3(0.0);
    FragColor = vec4(max(c, vec3(0.0)), 1.0);
}
