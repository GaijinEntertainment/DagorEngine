include "land_block_inc.sh"
include "land_block_common.sh"
include "displacement_inc.sh"
include "wetness_inc.sh"
include "puddles_inc.sh"
include "toroidal_heightmap.sh"
include "water_heightmap.sh"

texture perlin_tex;

float4 water_ripples_origin_delta = (0, 0, 0, 0);
float water_ripples_damping = 0.95;
int water_ripples_drop_count = 1;

int water_ripples_puddles = 0;
interval water_ripples_puddles: water_ripples_puddles_off < 1, water_ripples_puddles_on;

shader water_ripples_update
{
  cull_mode = none;
  z_test = false;
  z_write = false;

  (ps) {
    origin_size_psize@f4 = (water_ripples_origin.x, water_ripples_origin.z, water_ripples_size, water_ripples_pixel_size);
    delta_damp_dcount@f4 = (water_ripples_origin_delta.x, water_ripples_origin_delta.z, water_ripples_damping, water_ripples_drop_count);
    water_ripples_info_texture@smp2d = water_ripples_info_texture;
  }

  LAND_MASK_HEIGHT()
  USE_PUDDLES_WETNESS()
  hlsl(ps) {
    #define water_heightmap_pages_samplerstate water_ripples_info_texture_samplerstate
  }
  INIT_WATER_HEIGHTMAP(ps)
  USE_WATER_HEIGHTMAP(ps)

  if (water_ripples_puddles == water_ripples_puddles_on)
  {
    (ps) { perlin_tex@smp2d = perlin_tex; }

    if (render_with_normalmap == render_displacement)
    {
      INIT_HEIGHTMAP_ZEROLEVEL(ps)
      USE_HEIGHTMAP_ZEROLEVEL(ps)
      INIT_TOROIDAL_HEIGHTMAP(ps)
      USE_TOROIDAL_HEIGHTMAP()
    }
  }

  POSTFX_VS_TEXCOORD(1, texcoord)

  hlsl(ps) {

    float4 dropData[53]: register(c21);
    // x,y - pos or size
    // z - radius
    // w - strength
    float4 dropDataLast: register(c74);

    #undef WETNESS_WATER_BORDER
    #define WETNESS_WATER_BORDER 0.01
    #define RIPPLE_WAVE_SPEED 2.0 // where 2.0 is maxium
    #define RIPPLES_DAMPING_WATER 0.99

    void water_ripples_update_ps(VsOutput input, out float2 info : SV_Target0, out float2 normal : SV_Target1)
    {
      float wSize = origin_size_psize.z;
      float2 wCenter = origin_size_psize.xy;
      float2 worldPos = wCenter + wSize * (input.texcoord - float2(0.5, 0.5));

      float landHeight = get_land_mask_height(float3(worldPos.x, 0.0, worldPos.y)).x;
      float waterHeight = get_wetness_water_level();
      get_water_height(worldPos, waterHeight);
      float waterBelow = saturate((waterHeight - landHeight) / WETNESS_WATER_BORDER);

##if water_ripples_puddles == water_ripples_puddles_on
      float basePerlin;
      sample_perlin_wetness(float3(worldPos.x, 0.0, worldPos.y), perlin_tex, basePerlin);
      float height = 0;
  ##if render_with_normalmap == render_displacement
      height = sample_tor_height(worldPos.xy, heightmap_zerolevel) - heightmap_zerolevel;
      // normalize values to -0.5, 0.5
      height = (min(height / max(heightmap_zerolevel, 0.00001), 0.0) + max(height / max(1.0 - heightmap_zerolevel, 0.00001), 0)) * 0.5;
      height = lerp(height, 1.0, waterBelow);
  ##endif
      float wetness = get_puddles_wetness(float3(worldPos.x, 0, worldPos.y), float3(0, 1, 0), 0.0, basePerlin, 1, 0.0, 0.0, height);
##else
      float wetness = 0.0;
##endif
      wetness = lerp(wetness, 1.0, waterBelow);

      info = float2(0, 0);
      normal = float2(0.5, 0.5);
      float dTop = 0, dLeft = 0, dRight = 0, dBottom = 0;
      float infoAdd = 0.0;
      BRANCH
      if (wetness > get_base_wetness_max_level())
      {
        float deltaXY = origin_size_psize.w;
        float2 dx = float2(deltaXY, 0.0);
        float2 dy = float2(0.0, deltaXY);
        float2 tcOffset = input.texcoord + delta_damp_dcount.xy;
        info = tex2Dlod(water_ripples_info_texture, float4(tcOffset, 0, 0)).rg;
        dRight = tex2Dlod(water_ripples_info_texture, float4(tcOffset + dx, 0, 0)).r;
        dBottom = tex2Dlod(water_ripples_info_texture, float4(tcOffset + dy, 0, 0)).r;
        dLeft = tex2Dlod(water_ripples_info_texture, float4(tcOffset - dx, 0, 0)).r;
        dTop = tex2Dlod(water_ripples_info_texture, float4(tcOffset - dy, 0, 0)).r;
        normal = normalize(float3(-(dRight - dLeft), -(dBottom - dTop), deltaXY * wSize)).xy;
        normal = normal * 0.5 + 0.5;

        uint dropCount = delta_damp_dcount.w;
        LOOP
        for (uint dropNo = 0; dropNo < dropCount;)
        {
          bool isSolidBox = dropData[dropNo].z < 0;
          float2 wPos = dropData[dropNo].xy;
          float radius = abs(dropData[dropNo]).z;
          float strength = lerp(0.0, dropData[dropNo].w, saturate((wetness - get_base_wetness_max_level()) / (1.0 - get_base_wetness_max_level())));
          float2 center = (wPos - wCenter) / wSize + float2(0.5, 0.5);

          BRANCH
          if (isSolidBox)
          {
            float2 localX = dropData[dropNo + 1].xy;
            float2 localY = dropData[dropNo + 1].zw;
            float2 hSize = float2(length(localX), length(localY));
            localX /= hSize.x;
            localY /= hSize.y;
            float2 p0 = float2(hSize.x, -hSize.y);
            float2 p1 = float2(hSize.x, hSize.y);
            float2 t = input.texcoord.xy - center;
            t = float2(dot(t, localX), dot(t, localY));

            float d = 0, wave = 0;
            #define make_smooth_line(k0, k1) \
              d = t.y > k0.y && t.y < k1.y ? t.x - k0.x : 0.0; \
              d += t.y <= k0.y ? length(t - k0) : 0.0; \
              d += t.y >= k1.y ? length(t - k1) : 0.0; \
              wave = 1.0 - saturate(abs(d / radius));
            make_smooth_line(p0, p1);
            infoAdd += wave * strength;
            ##if hardware.vulkan
            // currently needed, either nvidia driver or hlslcc mess the branch up if its flattened
            BRANCH
            ##endif
            if (t.x < (p0.x - radius) && t.x > (-hSize.x + radius) && t.y > (p0.y + radius) && t.y < (p1.y - radius))
            {
              info = float2(0, 0);
              dTop = 0, dLeft = 0, dRight = 0, dBottom = 0;
              infoAdd = 0.0;
              normal = float2(0.5, 0.5);
            }
            dropNo += 2;
          }
          else
          {
            float wave = saturate(length(center - input.texcoord.xy) / radius);
            wave = cos(wave * PI * 0.5);
            infoAdd += wave * strength;
            dropNo += 1;
          }
        }
      }

      float average = (
        dLeft +
        dTop +
        dRight +
        dBottom
      ) * 0.25;
      info.g += (average - info.r) * RIPPLE_WAVE_SPEED;
      info.g *= lerp(delta_damp_dcount.z, RIPPLES_DAMPING_WATER, waterBelow);
      info.r += info.g + infoAdd;
    }
  }

  compile("target_ps", "water_ripples_update_ps");
}