#version 330 core
// ============================================================================
//  Black hole ray marcher  (approximate Schwarzschild light bending)
//
//  One ray is traced per pixel from the camera. The ray is bent step by step
//  with the classic "Binet" acceleration for light around a Schwarzschild
//  black hole (units: Schwarzschild radius rs = 1):
//
//        a = -1.5 * h^2 * p / |p|^5          h = |p x v|  (conserved)
//
//  This reproduces the real photon sphere (r = 1.5 rs), the shadow radius
//  (2.6 rs) and the correct weak-field deflection angle 2 rs / b.
//  It is NOT a full general-relativistic simulation (no spin, no polarisation,
//  disk is an infinitely thin emitter, colours are an artistic blackbody).
// ============================================================================
out vec4 FragColor;

uniform vec2  uRes;
uniform vec3  uCamPos;                 // simulation units
uniform vec3  uCamRight, uCamUp, uCamFwd;
uniform float uTanHalfFov;
uniform float uPixelAngle;             // angular size of one pixel (radians)
uniform float uRs;                     // Schwarzschild radius (SU)
uniform float uTime;                   // simulation time / rs
uniform float uSkyTime;                // simulation time, for sky drift/twinkle
uniform float uDiskIn, uDiskOut;       // in units of rs
uniform float uDiskBright;
uniform float uDiskSpeed;
uniform float uLens;                   // 0 = straight rays, 1 = Schwarzschild
uniform float uStars;                  // 1 = starfield + nebula visible

const int   MAX_STEPS = 280;
const float LAYERS = 16.0;

// ---------------------------------------------------------------- hashing ---
float hash12(vec2 p) {
    vec3 p3 = fract(vec3(p.xyx) * 0.1031);
    p3 += dot(p3, p3.yzx + 33.33);
    return fract((p3.x + p3.y) * p3.z);
}
vec2 hash22(vec2 p) {
    vec3 p3 = fract(vec3(p.xyx) * vec3(0.1031, 0.1030, 0.0973));
    p3 += dot(p3, p3.yzx + 33.33);
    return fract((p3.xx + p3.yz) * p3.zy);
}
float hash13(vec3 p) {
    p = fract(p * 0.1031);
    p += dot(p, p.zyx + 31.32);
    return fract((p.x + p.y) * p.z);
}
float vnoise3(vec3 p) {
    vec3 i = floor(p);
    vec3 f = fract(p);
    f = f * f * f * (f * (f * 6.0 - 15.0) + 10.0);   // quintic fade: smoother gradients
    float a = mix(hash13(i),                  hash13(i + vec3(1,0,0)), f.x);
    float b = mix(hash13(i + vec3(0,1,0)),    hash13(i + vec3(1,1,0)), f.x);
    float c = mix(hash13(i + vec3(0,0,1)),    hash13(i + vec3(1,0,1)), f.x);
    float d = mix(hash13(i + vec3(0,1,1)),    hash13(i + vec3(1,1,1)), f.x);
    return mix(mix(a, b, f.y), mix(c, d, f.y), f.z);
}
float fbm3(vec3 p) {
    float a = 0.5, s = 0.0;
    for (int i = 0; i < 4; ++i) {
        s += a * vnoise3(p);
        p = p * 2.03 + vec3(1.7, 9.2, 3.1);
        a *= 0.5;
    }
    return s;
}

// -------------------------------------------------------------- colours ----
// Artistic blackbody: t ~ 0.3 deep red, 0.5 orange, 0.8 yellow-white, >1 blue-white
vec3 heatColor(float t) {
    vec3 c = mix(vec3(0.20, 0.015, 0.0), vec3(1.00, 0.27, 0.02), smoothstep(0.0, 0.35, t));
    c = mix(c, vec3(1.00, 0.72, 0.22), smoothstep(0.30, 0.65, t));
    c = mix(c, vec3(1.00, 0.93, 0.80), smoothstep(0.60, 0.95, t));
    c = mix(c, vec3(0.62, 0.76, 1.00), smoothstep(0.95, 1.50, t));
    return c;
}

// ------------------------------------------------------------ background ---
// One layer of procedural stars. The direction is mapped onto a cube face and
// a jittered grid places at most one star per cell. Because this is evaluated
// with the LENSED ray direction, the stars are bent / duplicated / stretched.
vec3 starLayer(vec3 d, float N, float density, float seed, float sizeBoost, float gain) {
    vec3 ad = abs(d);
    vec2 uv; float fid;
    if (ad.x >= ad.y && ad.x >= ad.z) { uv = d.yz / ad.x; fid = d.x > 0.0 ? 0.0 : 1.0; }
    else if (ad.y >= ad.z)            { uv = d.xz / ad.y; fid = d.y > 0.0 ? 2.0 : 3.0; }
    else                              { uv = d.xy / ad.z; fid = d.z > 0.0 ? 4.0 : 5.0; }

    vec2 g = (uv * 0.5 + 0.5) * N;
    vec2 cell = floor(g);
    vec2 f = g - cell;
    vec2 key = cell + vec2(fid * 37.7 + seed, fid * 11.3 + seed * 3.1);

    float h = hash12(key);
    if (h > density) return vec3(0.0);

    vec2 sp  = 0.2 + 0.6 * hash22(key + 17.0);
    float mag = hash12(key * 1.37 + 4.2);
    float rad = uPixelAngle * N * 0.5 * (1.0 + dot(uv, uv)) * (0.85 + sizeBoost * mag * mag);
    float dd = length(f - sp) / rad;
    float s = exp(-dd * dd * 1.4);

    float temp = hash12(key + 91.0);
    vec3 tint = mix(vec3(1.0, 0.72, 0.52), vec3(0.62, 0.76, 1.0), temp);
    tint = mix(tint, vec3(1.0), 0.45);
    float twinkle = 0.88 + 0.12 * sin(uSkyTime * 0.9 + h * 120.0);
    return tint * s * (0.10 + 1.6 * mag * mag * mag * mag) * twinkle * gain;
}

vec3 nebula(vec3 d) {
    // low-frequency, wide smoothsteps => no hard edges / banding
    float a = smoothstep(0.50, 0.92, fbm3(d * 1.5 + vec3(3.1, 1.2, 7.7)));
    float b = smoothstep(0.52, 0.94, fbm3(d * 2.2 + vec3(-8.4, 5.5, 2.1)));
    float c = smoothstep(0.56, 0.96, fbm3(d * 3.8 + vec3(1.0, -3.3, 9.9)));
    vec3 col = a * vec3(0.040, 0.050, 0.200)
             + b * vec3(0.150, 0.035, 0.210)
             + c * vec3(0.045, 0.060, 0.115);
    return col * 0.95;
}

vec3 background(vec3 d) {
    if (uStars < 0.5) return vec3(0.0);
    // very slow rotation of the whole sky = "slowly moving" background
    float a = uSkyTime * 0.0025;
    float ca = cos(a), sa = sin(a);
    d = vec3(ca * d.x + sa * d.z, d.y, -sa * d.x + ca * d.z);

    vec3 col = nebula(d);
    col += starLayer(d,  40.0, 0.30,  1.0, 1.6, 1.3);
    col += starLayer(d, 110.0, 0.24, 13.0, 0.8, 0.9);
    col += starLayer(d, 260.0, 0.18, 29.0, 0.3, 0.6);
    return col;
}

// ------------------------------------------------------------------ disk ---
// Returns (turbulence, sparkle) for one rotating ring-layer. Each layer
// rotates rigidly at the Keplerian speed of its centre radius, so neighbouring
// layers shear against each other (differential rotation) but the pattern
// never winds up into unresolvable detail.
vec2 layerPattern(float li, float rr, float phi) {
    float rc = uDiskIn + (li + 0.5) / LAYERS * (uDiskOut - uDiskIn);
    float om = inversesqrt(2.0 * rc * rc * rc) * uDiskSpeed;
    float a = phi + om * uTime + li * 2.399;        // disk spins toward -phi
    float angScale = 0.8 + 0.25 * rr;               // keep streak aspect sensible
    vec3 q = vec3(cos(a) * angScale, sin(a) * angScale, rr * 4.5 + li * 7.13);

    // "heat haze": slow domain warp that makes the streaks shimmer
    vec2 w = vec2(vnoise3(q * 0.9 + vec3(0.0, 0.0, uTime * 0.20)),
                  vnoise3(q * 0.9 + vec3(5.2, 1.3, -uTime * 0.17)));
    q.xy += (w - 0.5) * 0.9;

    float n = fbm3(q);
    float sp = pow(vnoise3(vec3(cos(a) * 9.0, sin(a) * 9.0, rr * 40.0) + q * 0.5), 10.0) * 3.0;
    return vec2(n, sp);
}

// Emission + opacity of the disk at a plane crossing point.
vec4 diskSample(vec3 hit, vec3 v) {
    float rr = length(hit.xz);
    float phi = atan(hit.z, hit.x);

    // --- procedural texture, blending two neighbouring layers at the seams
    float s = clamp((rr - uDiskIn) / (uDiskOut - uDiskIn), 0.0, 0.9999) * LAYERS;
    float li = floor(s);
    float fr = s - li;
    vec2 pat = layerPattern(li, rr, phi);
    if (fr > 0.7 && li < LAYERS - 1.0)
        pat = mix(pat, layerPattern(li + 1.0, rr, phi), smoothstep(0.7, 1.0, fr));
    float tex = clamp((pat.x - 0.22) * 2.1, 0.0, 1.4);

    // --- temperature profile  T ~ r^(-3/4)  (thin-disk scaling)
    float T0 = pow(uDiskIn / rr, 0.75);

    // --- relativistic Doppler beaming + gravitational redshift
    float beta = sqrt(0.5 / max(rr - 1.0, 0.05));            // orbital speed / c
    vec3 tang = vec3(hit.z, 0.0, -hit.x) / rr;               // direction of motion
    float D = sqrt(1.0 - beta * beta) / (1.0 + beta * dot(tang, v));
    float g = sqrt(max(1.0 - 1.0 / rr, 0.0));
    float shift = D * g;
    float Tobs = T0 * shift;

    vec3 col = heatColor(Tobs * (0.62 + 0.30 * tex));
    float I = (0.12 + 1.9 * pow(Tobs, 3.0)) * (0.28 + 0.95 * tex) + pat.y * 0.7 * Tobs;
    col *= I * uDiskBright;

    float edge = smoothstep(uDiskIn, uDiskIn + 0.25, rr) *
                 (1.0 - smoothstep(uDiskOut - 3.0, uDiskOut, rr));
    float alpha = clamp(edge * (0.45 + 0.55 * tex), 0.0, 0.97);
    return vec4(col * edge, alpha);
}

// ------------------------------------------------------------------ main ---
void main() {
    vec2 ndc = (gl_FragCoord.xy * 2.0 - uRes) / uRes.y;
    vec3 v = normalize(uCamFwd + (ndc.x * uCamRight + ndc.y * uCamUp) * uTanHalfFov);
    vec3 p = uCamPos / uRs;                    // work in units of rs

    vec3  hv = cross(p, v);
    float h2 = dot(hv, hv);                    // conserved "angular momentum"^2

    vec3  col = vec3(0.0);
    vec3  glow = vec3(0.0);                    // photon-ring glow (escaping rays only)
    float T = 1.0;                             // transmittance
    bool captured = false, escaped = false;

    for (int i = 0; i < MAX_STEPS; ++i) {
        float r = length(p);
        if (r < 1.0) { captured = true; break; }
        if (r > 45.0 && dot(p, v) > 0.0) { escaped = true; break; }

        float dt = clamp(0.06 * r, 0.02, 1.5);

        // --- soft volumetric haze around the disk plane (heat glow)
        float rxz = length(p.xz);
        if (rxz > uDiskIn * 0.8 && rxz < uDiskOut) {
            float hgt = 0.05 * rxz + 0.04;
            float gz = exp(-p.y * p.y / (hgt * hgt));
            float Tg = pow(uDiskIn / rxz, 0.75);
            col += T * heatColor(0.60 * Tg) * gz * dt * 0.10 * Tg * Tg * Tg * uDiskBright;
        }

        // --- photon ring: bright thin glow where light orbits near r = 1.5 rs,
        //     plus a faint cool "gravitational boundary" just outside it.
        float dr = (r - 1.5) / 0.10;
        float dr2 = (r - 1.75) / 0.55;
        glow += T * dt * min(uLens, 1.0) *
                (vec3(1.00, 0.80, 0.52) * exp(-dr * dr) * 0.60 * (0.6 + 0.4 * uDiskBright) +
                 vec3(0.10, 0.22, 0.55) * exp(-dr2 * dr2) * 0.05);

        // --- bend the ray
        float r2 = r * r;
        vec3 acc = -1.5 * uLens * h2 * p / (r2 * r2 * r);
        vec3 vn = normalize(v + acc * dt);
        vec3 pn = p + 0.5 * (v + vn) * dt;

        // --- crossing of the equatorial plane (y = 0) => accretion disk
        if (p.y * pn.y < 0.0) {
            float t = p.y / (p.y - pn.y);
            vec3 hit = mix(p, pn, t);
            float rr = length(hit.xz);
            if (rr > uDiskIn && rr < uDiskOut) {
                vec4 d = diskSample(hit, vn);
                col += T * d.rgb * d.a;
                T *= (1.0 - d.a);
                if (T < 0.01) break;
            }
        }
        p = pn;
        v = vn;
    }

    if (!captured) {
        col += glow;
        if (escaped) col += T * background(normalize(v));
    }
    FragColor = vec4(max(col, vec3(0.0)), 1.0);
}
