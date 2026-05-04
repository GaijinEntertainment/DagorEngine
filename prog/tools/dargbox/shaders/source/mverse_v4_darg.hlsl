// PS3 XMB Wave Knot v4 -- Darg HLSL
// Converted from mverse_v4.glsl
//
// Customizable via darg_params0..2 (all optional, defaults used if zero):
//   params0: (knot_x, knot_y, flow_dir_rad, num_waves)
//   params1: (speed, spark_intensity, wave_width, wave_hue_deg)
//   params2: (bg_hue_deg, bg_amount, 0, 0)
//
// Defaults: knot at (0.88, 0.60), vertical flow, 14 waves, orange on red bg

#define PI 3.14159265359

// GLSL-compatible mod (always non-negative)
float glsl_mod(float x, float y) { return x - y * floor(x / y); }

float hash11(float p)
{
    p = frac(p * 0.1031);
    p *= p + 33.33;
    p *= p + p;
    return frac(p);
}

float hash21(float2 p)
{
    float3 p3 = frac(float3(p.xyx) * 0.1031);
    p3 += dot(p3, p3.yzx + 33.33);
    return frac((p3.x + p3.y) * p3.z);
}

float3 hsv2rgb(float3 c)
{
    float4 K = float4(1.0, 2.0 / 3.0, 1.0 / 3.0, 3.0);
    float3 p = abs(frac(c.xxx + K.xyz) * 6.0 - K.www);
    return c.z * lerp(K.xxx, clamp(p - K.xxx, 0.0, 1.0), c.y);
}

float4 darg_ps(float2 uv)
{
    // Read customizable parameters with defaults
    float knot_x    = darg_params0.x != 0.0 ? darg_params0.x : 0.88;
    float knot_y    = darg_params0.y != 0.0 ? darg_params0.y : 0.60;
    float flow_dir  = darg_params0.z != 0.0 ? darg_params0.z : 1.5708;
    float num_waves = darg_params0.w != 0.0 ? darg_params0.w : 14.0;

    float speed      = darg_params1.x != 0.0 ? darg_params1.x : 0.37;
    float spark_int  = darg_params1.y;  // 0 = off (default)
    float wave_width = darg_params1.z != 0.0 ? darg_params1.z : 0.64;
    float wave_hue   = darg_params1.w != 0.0 ? darg_params1.w / 360.0 : 0.07;

    float bg_hue = darg_params2.x != 0.0 ? darg_params2.x / 360.0 : 1.0;
    float bg_amt = darg_params2.y != 0.0 ? darg_params2.y : 0.39;

    float2 knot_pos = float2(knot_x, knot_y);
    float aspect = darg_resolution.x / darg_resolution.y;
    float2 fragCoord = uv * darg_resolution;

    // -- Background gradient --
    float2 bgUV = uv - knot_pos;
    bgUV.x *= aspect;
    float bgDist = length(bgUV);
    float3 bgDark  = hsv2rgb(float3(bg_hue, 0.6, 0.02)) * bg_amt;
    float3 bgMid   = hsv2rgb(float3(bg_hue, 0.5, 0.07)) * bg_amt;
    float3 bgLight = hsv2rgb(float3(bg_hue + 0.05, 0.4, 0.12)) * bg_amt;
    float3 bg = lerp(bgMid, bgDark, smoothstep(0.0, 0.8, bgDist));
    float2 bg2UV = uv - float2(knot_pos.x + 0.2, knot_pos.y - 0.15);
    bg2UV.x *= aspect;
    bg += bgLight * exp(-dot(bg2UV, bg2UV) * 3.0) * 0.5;
    float neb = hash21(floor(fragCoord * 0.15)) * 0.3 + hash21(floor(fragCoord * 0.05)) * 0.7;
    bg += hsv2rgb(float3(bg_hue + 0.1, 0.3, 0.015 * bg_amt)) * neb;
    float3 col = bg;

    // -- Coords --
    float2 p = uv - 0.5;
    p.x *= aspect;
    float2 knot = knot_pos - 0.5;
    knot.x *= aspect;
    float ca = cos(flow_dir), sa = sin(flow_dir);
    float2x2 rot = float2x2(ca, -sa, sa, ca);
    float2 rp = mul(rot, p);
    float2 rk = mul(rot, knot);
    float along = rp.x, perp = rp.y, kAlong = rk.x, kPerp = rk.y;
    float t = darg_time * speed;

    // -- Ribbons --
    [loop]
    for (float i = 0.0; i < 40.0; i++)
    {
        if (i >= num_waves) break;
        float id = i / num_waves;
        float s1 = hash11(i * 17.31), s2 = hash11(i * 23.71 + 7.0), s3 = hash11(i * 11.13 + 3.0);
        float spread = (id - 0.5) * 2.0 + (s1 - 0.5) * 0.5;
        float dAlong = along - kAlong;
        float pinch = 1.0 - exp(-2.2 * dAlong * dAlong);
        float tightPinch = exp(-10.0 * dAlong * dAlong);

        float ribbonBase = kPerp + spread * (0.06 * wave_width + 0.35 * wave_width * pinch);

        float phase = along * (3.0 + s1 * 2.5) - t * (0.25 + s2 * 0.35);
        float wave = sin(phase) * 0.035 * (0.4 + pinch) * wave_width
                   + sin(phase * 2.1 + s1 * 5.0 - t * 0.18) * 0.018 * (0.3 + pinch) * wave_width
                   + sin(phase * 0.6 + s3 * 4.0 + t * 0.12) * 0.022 * pinch * wave_width
                   + sin(along * 9.0 + i * PI / num_waves + t * 0.4) * 0.065 * tightPinch * wave_width;
        float ribbonY = ribbonBase + wave;
        float d = abs(perp - ribbonY);

        float coreW = 0.0012 + 0.0006 * tightPinch;
        float core = smoothstep(coreW, coreW * 0.2, d);
        float haloR = 0.003 * (1.0 + 0.8 * tightPinch);
        float halo = exp(-d * d / (haloR * haloR)) * 0.35;

        float3 rCol = hsv2rgb(float3(wave_hue + s1 * 0.06 - 0.03, 0.85 - 0.15 * tightPinch, 1.0));
        float kB = 1.0 + 1.2 * tightPinch;
        float dots = pow(max(sin(along * (40.0 + s1 * 30.0) - t * (3.0 + s2 * 4.0)), 0.0), 8.0);
        float dotI = 0.25 + 0.75 * dots;

        col += rCol * (core * 1.2 + halo) * kB * dotI;

        // Sparks (runtime check, skipped when spark_int ~ 0)
        if (spark_int > 0.01)
        {
            float sparkA = 0.0;
            [loop]
            for (float si = 0.0; si < 5.0; si++)
            {
                float ss1 = hash11(i * 100.0 + si * 37.0 + 5.0);
                float ss2 = hash11(i * 100.0 + si * 53.0 + 11.0);
                float sPA = along - (t * (0.15 + ss1 * 0.35) + ss2 * 10.0);
                float per = 0.8 + ss1 * 1.2;
                float sL = glsl_mod(sPA, per) - per * 0.5;
                float sD2 = sL * sL + d * d;
                float sR = 0.0006 + 0.0004 * spark_int;
                sparkA += (exp(-sD2 / (sR * sR)) * 2.5 + exp(-(pow(sL + 0.012 + 0.008 * ss1, 2.0) * 8.0 + d * d * 400.0)) * step(0.0, sL + 0.012) * 0.8) * (0.5 + 0.5 * ss1);
            }
            col += lerp(rCol, float3(1.0, 0.92, 0.7), 0.55) * sparkA * spark_int;
        }

        // Nodes
        float nF = 3.5 + s1 * 2.5;
        float nP = along * nF - t * (0.08 + s2 * 0.12);
        float nOn = smoothstep(0.94, 0.97, sin(nP));
        float nS = smoothstep(0.008, 0.0024, abs(glsl_mod(along / (1.0 / nF), 1.0) - 0.5)) * smoothstep(0.003, 0.0009, d);
        col += rCol * 0.75 * nOn * nS;
    }

    // -- Vignette + tonemap --
    col *= 1.0 - 0.45 * pow(length(uv - 0.5) * 1.4, 2.0);
    col = 1.0 - exp(-col * 2.0);
    col.b += 0.008 * (1.0 - col.b);

    return float4(col, darg_opacity);
}
