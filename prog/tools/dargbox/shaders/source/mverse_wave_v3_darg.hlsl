// PS3 XMB Wave Knot v3 -- Darg HLSL
// Converted from mverse_wave_v3.glsl
//
// Customizable via darg_params0..1 (all optional, defaults used if zero):
//   params0: (knot_x, knot_y, flow_dir_rad, num_waves)
//   params1: (speed, glow_intensity, 0, 0)

#define PI 3.14159265359

float hash(float n) { return frac(sin(n) * 43758.5453123); }

float4 darg_ps(float2 uv)
{
    // Read customizable parameters with defaults
    float knot_x    = darg_params0.x != 0.0 ? darg_params0.x : 0.8;
    float knot_y    = darg_params0.y != 0.0 ? darg_params0.y : 0.6;
    float flow_dir  = PI/2;//darg_params0.z;  // 0 = horizontal
    float num_waves = darg_params0.w != 0.0 ? darg_params0.w : 15.0;

    float speed          = darg_params1.x != 0.0 ? darg_params1.x : 0.4;
    float glow_intensity = 0.0;//darg_params1.y != 0.0 ? darg_params1.y : 0.0;

    float2 knot_pos = float2(knot_x, knot_y);
    float aspect = darg_resolution.x / darg_resolution.y;
    float2 fragCoord = uv * darg_resolution;

    float2 p = uv - 0.5;
    p.x *= aspect;

    float2 knot = knot_pos - 0.5;
    knot.x *= aspect;

    float ca = cos(flow_dir), sa = sin(flow_dir);
    float2x2 rot = float2x2(ca, -sa, sa, ca);
    float2 rp = mul(rot, p);
    float2 rk = mul(rot, knot);

    float along  = rp.x;
    float perp   = rp.y;
    float kAlong = rk.x;
    float kPerp  = rk.y;

    float t = darg_time * speed;

    float3 col = float3(0.0, 0.0, 0.0);

    [loop]
    for (float i = 0.0; i < 40.0; i++)
    {
        if (i >= num_waves) break;

        float id = i / num_waves;
        float seed  = hash(i * 17.31);
        float seed2 = hash(i * 23.71 + 7.0);
        float seed3 = hash(i * 11.13 + 3.0);

        float spread = (id - 0.5) * 2.0 + (seed - 0.5) * 0.6;

        float dAlong = along - kAlong;
        float pinch = 1.0 - exp(-1.8 * dAlong * dAlong);
        float tightPinch = exp(-8.0 * dAlong * dAlong);

        float ribbonBase = kPerp + spread * (0.15 + 0.55 * pinch);

        float flowPhase = along * (2.5 + seed * 2.0) - t * (0.3 + seed2 * 0.4);
        float wave = sin(flowPhase) * 0.04 * (0.5 + pinch);
        wave += sin(flowPhase * 2.3 + seed * 6.0 - t * 0.2) * 0.02 * (0.3 + pinch);
        wave += sin(flowPhase * 0.7 + seed3 * 4.0 + t * 0.15) * 0.03 * pinch;

        float crossWave = sin(along * 8.0 + i * PI / num_waves + t * 0.5) * 0.08 * tightPinch;
        wave += crossWave;

        float ribbonY = ribbonBase + wave;
        float d = abs(perp - ribbonY);

        float thickness = 0.0018 + 0.001 * tightPinch;
        float core = smoothstep(thickness, 0.0, d);

        float glowRadius = (0.002 + 0.015 * glow_intensity) * (0.0 + 0.5 * tightPinch);
        float ribbonGlow = exp(-d * d / (glowRadius * glowRadius));

        float3 baseColor;
        baseColor.r = 1.0;
        baseColor.g = 0.35 + seed * 0.12 + 0.15 * tightPinch;
        baseColor.b = 0.02 + seed2 * 0.08;

        float knotBrightness = 1.0 + 2.0 * tightPinch;

        float particlePhase = along * (30.0 + seed * 20.0) - t * (2.0 + seed * 3.0);
        float particles = pow(0.5 + 0.5 * sin(particlePhase), 3.0);
        float particleIntensity = 0.3 + 0.7 * particles;

        float nodePhase = along * (10.0 + seed * 3.0) - t * (0.1 + seed2 * 0.15);
        float node = smoothstep(0.99, 0.995, sin(nodePhase));
        float nodeGlow = node * exp(-d * d / (0.00003 + 0.001 * glow_intensity));

        float intensity = (core * 1.5 + ribbonGlow * 0.6) * knotBrightness * particleIntensity;
        intensity += nodeGlow * 2.0;

        col += baseColor * intensity;
    }

    // Vignette
    col *= 1.0 - 0.4 * length(uv - 0.5);

    // Tonemap
    col = 1.0 - exp(-col * 1.5);
    col += float3(-0.01, -0.005, 0.015) * (1.0 - col);

    return float4(col, darg_opacity);
}
