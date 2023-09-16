include "hardware_defines.sh"
include "postfx_inc.sh"
include "shader_global.sh"

texture hdr_tex;
float paper_white_nits = 200;
float hdr_brightness = 1;
float hdr_shadows = 1;

shader decode_hdr_to_sdr
{

    cull_mode = none;
    z_test = false;
    z_write = false;
    no_ablend;

    (ps) {
        hdr_tex@tex2d = hdr_tex;
        hdr_params@f4 = (paper_white_nits, paper_white_nits * 100, max(-0.5, 1 - hdr_shadows), 1 / (hdr_brightness + 0.01));
    }

    ENABLE_ASSERT(ps)
    POSTFX_VS(1)

    hlsl(ps)
    {
        /*
        Shader: Try to get the SDR part of HDR content
        From: https://raw.githubusercontent.com/VoidXH/Cinema-Shader-Pack/master/Shaders/HDR%20to%20SDR.hlsl
        */
        // Configuration ---------------------------------------------------------------
        const static float peakLuminance = hdr_params.x; // Peak playback screen luminance in nits
        const static float maxCLL = hdr_params.y; // Maximum content light level in nits
        // -----------------------------------------------------------------------------

        // Precalculated values
        const static float gain = maxCLL / peakLuminance;

        // PQ constants
        const static float m1inv = 16384 / 2610.0;
        const static float m2inv = 32 / 2523.0;
        const static float c1 = 3424 / 4096.0;
        const static float c2 = 2413 / 128.0;
        const static float c3 = 2392 / 128.0;

        inline float3 pq2lin(float3 pq) { // Returns luminance in nits
            float3 p = pow(pq, m2inv);
            float3 d = max(p - c1, 0) / (c2 - c3 * p);
            return pow(d, m1inv) * gain;
        }

        float4 decode_hdr(float4 screenpos : VPOS) : SV_Target0 {
            float3 pxval = texelFetch(hdr_tex, screenpos.xy, 0).xyz;
        ##if hardware.metal
            return float4(min((float3)1.f, sqrt(pq2lin(pxval).bgr)), 1.f);
        ##else
            return float4(min((float3)1.f, sqrt(pxval)), 1.f);
        ##endif
        }
    }
    compile("target_ps", "decode_hdr");
}