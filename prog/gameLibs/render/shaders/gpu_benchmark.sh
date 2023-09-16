include "hardware_defines.sh"

texture gpu_benchmark_tex;

int gpu_benchmark_hmap = 0;
interval gpu_benchmark_hmap: off < 1, on;

shader gpu_benchmark
{
  z_test = true;
  cull_mode = ccw;

  if (gpu_benchmark_hmap == on)
  {
    z_write = true;
    no_ablend;
  }
  else
  {
    z_write = false;
    blend_src = sa; blend_dst = one;
    blend_asrc = one; blend_adst = zero;
  }

  channel float4 pos = pos;
  channel float4 tc[0]=tc[0];

  hlsl(vs) {
    struct VsInput {
      float4 pos: POSITION;
      float4 tc: TEXCOORD0;
      HW_BASE_VERTEX_ID_OPTIONAL
      HW_VERTEX_ID
    };
  }

  hlsl {
    struct VsOutput
    {
      VS_OUT_POSITION(pos)
      float4 pos_tc: TEXCOORD0;
      float3 worldPos: TEXCOORD1;
      float4 uv: TEXCOORD2;
    };
  }

  (vs) {
    globtm@f44=globtm;
  }

  hlsl(vs) {
    #include "noise/Perlin2D.hlsl"

    VsOutput gpu_benchmark_vs(VsInput input)
    {
      USE_VERTEX_ID_WITHOUT_BASE_OFFSET(input)
      uint instanceNo = input.vertexId / 4;
      uint vertexNo = input.vertexId % 4;

      float2 vpos = float2(vertexNo % 2, vertexNo / 2);
      FLATTEN
      if ((vertexNo / 2) % 2 == 0)
        vpos.x = 1 - vpos.x;
      float2 pos = vpos - 0.5;

      float2 perlinPos = input.pos.xz * 0.1;
      float perlin = noise_Perlin2D(perlinPos);
      perlin += noise_Perlin2D(perlinPos * 2) * 0.5;
      perlin += noise_Perlin2D(perlinPos * 4) * 0.25;
      perlin += noise_Perlin2D(perlinPos * 8) * 0.125;
      perlin += noise_Perlin2D(perlinPos * 16) * 0.0625;
      perlin /= 1.9375;
      perlin = smoothstep(-1.0, 1.0, perlin);
      float3 worldPos = input.pos.xyz + float3(0, perlin * input.pos.w, 0);

      VsOutput output;
      output.pos = mul(float4(worldPos, 1), globtm);
      output.pos_tc = float4(pos.xy, vpos);
      output.worldPos = worldPos;
      output.uv = input.tc;
      return output;
    }
  }
  compile("target_vs", "gpu_benchmark_vs");

  (ps) {
    gpu_benchmark_tex@smp2d = gpu_benchmark_tex;
  }

  hlsl(ps) {
    #include "noise/Cellular3D.hlsl"

    float4 gpu_benchmark_ps(VsOutput input): SV_Target
    {
      float4 color = tex2D(gpu_benchmark_tex, input.uv.xy);
      float3 worldNormal = normalize(cross(normalize(ddx(input.worldPos)), normalize(ddy(input.worldPos))));
      float diff = dot(worldNormal, normalize(float3(1, 1.25, 0.75)));
      if (diff < 0)
        diff = -diff * 0.25;

      float noise = noise_Cellular3D(input.worldPos);
      diff *= (noise * 0.5 + 0.5);

      return float4(diff.xxx * color.rgb, input.uv.w * (color.a * 0.5 + 0.5));
    }
  }

  compile("target_ps", "gpu_benchmark_ps");
}