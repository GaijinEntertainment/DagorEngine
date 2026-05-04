// Converted from Shadertoy: https://www.shadertoy.com/view/tXy3DK
// Author: int_45h (MIT-like license)
// Uses procedural hash (Dave Hoskins) instead of texture-based noise.

#define THRESHOLD .99
#define DUST
#define MIN_DIST .13
#define MAX_DIST 40.
#define MAX_DRAWS 40

// --- Procedural hash (Dave Hoskins) ---
// https://www.shadertoy.com/view/XdGfRR
float hash12(float2 p)
{
    uint2 q = uint2(int2(p)) * uint2(1597334673u, 3812015801u);
    uint n = (q.x ^ q.y) * 1597334673u;
    return float(n) * 2.328306437080797e-10;
}

// Value noise via cubic hermite interpolation
float value2d(float2 p)
{
    float2 pg = floor(p), pc = p - pg;
    pc *= pc * pc * (3.0 - 2.0 * pc);
    float2 k = float2(0, 1);
    return lerp(
        lerp(hash12(pg + k.xx), hash12(pg + k.yx), pc.x),
        lerp(hash12(pg + k.xy), hash12(pg + k.yy), pc.x),
        pc.y
    );
}

// --- Starfield ---
// Based on xaot88's starfield: https://www.shadertoy.com/view/Md2SR3
float get_stars_rough(float2 p)
{
    float s = smoothstep(THRESHOLD, 1.0, hash12(p));
    if (s >= THRESHOLD)
        s = pow((s - THRESHOLD) / (1.0 - THRESHOLD), 10.0);
    return s;
}

// Cubic hermite interpolated starfield
float get_stars(float2 p, float a, float t)
{
    float2 pg = floor(p), pc = p - pg;
    float2 k = float2(0, 1);
    pc *= pc * pc * (3.0 - 2.0 * pc);

    float s = lerp(
        lerp(get_stars_rough(pg + k.xx), get_stars_rough(pg + k.yx), pc.x),
        lerp(get_stars_rough(pg + k.xy), get_stars_rough(pg + k.yy), pc.x),
        pc.y
    );
    return smoothstep(a, a + t, s) * pow(value2d(p * 0.1 + darg_time) * 0.5 + 0.5, 8.3);
}

// Sample the starfield at different sizes to fake depth
float get_dust(float2 p, float2 size, float f)
{
    float2 ar = float2(darg_resolution.x / darg_resolution.y, 1.0);
    float2 pp = p * size * ar;
    return
        pow(0.64 + 0.46 * cos(p.x * 6.28), 1.7) *
        f *
    (
        get_stars(0.1 * pp + darg_time * float2(20.0, -10.1), 0.11, 0.71) * 4.0 +
        get_stars(0.2 * pp + darg_time * float2(30.0, -10.1), 0.1, 0.31) * 5.0 +
        get_stars(0.32 * pp + darg_time * float2(40.0, -10.1), 0.1, 0.91) * 2.0
    );
}

// --- SDF wave surface ---
float sdf(float3 p)
{
    p *= 2.0;
    float o =
        4.2 * sin(0.05 * p.x + darg_time * 0.25) +
        (0.04 * p.z) *
        sin(p.x * 0.11 + darg_time) *
        2.0 * sin(p.z * 0.2 + darg_time) *
        value2d(
            float2(0.03, 0.4) * p.xz + float2(darg_time * 0.5, 0)
        );
    return abs(dot(p, normalize(float3(0, 1, 0.05))) + 2.5 + o * 0.5);
}

float3 get_normal(float3 p)
{
    float2 k = float2(1, -1);
    float t = 0.001;
    return normalize(
        k.xyy * sdf(p + t * k.xyy) +
        k.yyx * sdf(p + t * k.yyx) +
        k.yxy * sdf(p + t * k.yxy) +
        k.xxx * sdf(p + t * k.xxx)
    );
}

// Enhanced sphere tracing raymarch
// Returns float2(alpha, glow)
float2 raymarch(float3 o, float3 d, float omega)
{
    float t = 0.0;
    float a = 0.0;
    float g = MAX_DIST;
    float dt = 0.0;
    float sl = 0.0;
    float emin = 0.03;
    float ed = emin;
    int dr = 0;
    bool hit = false;

    [loop]
    for (int i = 0; i < 100; i++)
    {
        float3 p = o + d * t;

        float ndt = sdf(p);
        if (abs(dt) + abs(ndt) < sl)
        {
            sl -= omega * sl;
            omega = 1.0;
        }
        else
            sl = ndt * omega;

        dt = ndt;
        t += sl;

        // Glow
        g = (t > 10.0) ? min(g, abs(dt)) : MAX_DIST;

        t += dt;
        if (t >= MAX_DIST) break;
        if (dt < MIN_DIST)
        {
            if (dr > MAX_DRAWS) break;
            dr++;

            float f = smoothstep(0.09, 0.11, (p.z * 0.9) / 100.0);
            if (!hit)
            {
                a = 0.01;
                hit = true;
            }
            ed = 2.0 * max(emin, abs(ndt));
            a += 0.0135 * f;
            t += ed;
        }
    }

    g /= 3.0;
    return float2(a, max(1.0 - g, 0.0));
}

float4 darg_ps(float2 uv)
{
    // Ray setup: origin at 0, direction through pixel
    float3 o = float3(0, 0, 0);
    float3 d = float3(
        (uv - 0.5) * float2(darg_resolution.x / darg_resolution.y, 1.0),
        1.0
    );

    float2 mg = raymarch(o, d, 1.2);
    float m = mg.x;

    // Reddish 4-point gradient background
    float3 c = lerp(
        lerp(float3(0.7, 0.2, 0.2), float3(0.4, 0.1, 0.1), uv.x),
        lerp(float3(0.45, 0.1, 0.1), float3(0.8, 0.3, 0.5), uv.x),
        uv.y
    );
    // Blend with white based on wave alpha
    c = lerp(c, float3(1.0, 1.0, 1.0), m);

    // Dust / starfield
    #ifdef DUST
    c += get_dust(uv, float2(2000.0, 2000.0), mg.y) * 0.3;
    #endif

    return float4(c, darg_opacity);
}
