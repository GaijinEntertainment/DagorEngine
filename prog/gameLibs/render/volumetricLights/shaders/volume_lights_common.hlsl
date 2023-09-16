#ifndef VOLUME_LIGHTS_COMMON_INCLUDED
#define VOLUME_LIGHTS_COMMON_INCLUDED 1


#define HEIGHTMAP_LOD 6 // 6 is enough detail, maybe even 7, but 8 is not

#define UNDERWATER_HEIGHT_BIAS 1.0 // a bias in meters, volfog fades away linearly below water level
#define CLOUDS_HEIGHT_BIAS 20.0 // a bias in meters, volfog fades away linearly above ((clouds start altitude) - bias)


//fixme: move to constant buffer! otherwise it is suboptimal
#define SAMPLE_NUM 8
static const float POISSON_Z_SAMPLES[SAMPLE_NUM] =
{
  0.0363777900792f,
  0.535357845902f,
  0.28433478804f,
  0.782194955996f,
  0.413117762656f,
  0.912773671043f,
  0.657693177748f,
  0.159355512717f
};
static const float2 POISSON_SAMPLES[SAMPLE_NUM] =
{
  float2(0.228132254148f, 0.67232428631f),
  float2(0.848556554824f, 0.135723477704f),
  float2(0.74820789575f, 0.63965073852f),
  float2(0.472544801767f, 0.351474129111f),
  float2(0.962881642535f, 0.387342871273f),
  float2(0.0875977149838f, 0.896250211998f),
  float2(0.203231652569f, 0.12436704431f),
  float2(0.56452806916f, 0.974024350484f),
};

float3 calc_jittered_tc(uint3 dtId, uint frame_id, float3 inv_resolution)
{
  uint2 randhash = pcg3d_hash_16bit(dtId).xy;
  float3 screenTcJittered = dtId + float3(POISSON_SAMPLES[(randhash.x + frame_id) % SAMPLE_NUM], POISSON_Z_SAMPLES[(randhash.y + frame_id) % SAMPLE_NUM]);
  screenTcJittered *= inv_resolution;
  return screenTcJittered;
}


float volfog_get_mieG0() { return 0.55; }
float volfog_get_mieG1() { return -0.4; }

float calc_sun_phase(float3 view_dir_norm, float3 from_sun_direction)
{
  //for high mie we need self shadow from media. Instead we change mie extrincity
  return phaseSchlickTwoLobe(dot(-from_sun_direction, view_dir_norm), 1.5*volfog_get_mieG0(), 1.5*volfog_get_mieG1());
}

float depth_to_volume_pos(float linear_depth, float inv_range)
{
  return sqrt(saturate(linear_depth * inv_range));
}
float volume_pos_to_depth(float volume_z, float range)
{
  return (volume_z*volume_z)*range;
}

bool calc_height_fog_coeffs(HeightFogNode node, float3 world_pos, float above_ground_height, out float2 coeffs)
{
  coeffs = float2(
    node.ground_fog_density*(above_ground_height + node.ground_fog_offset),
    node.height_fog_scale*(world_pos.y - node.height_fog_layer_max)
  );
  static const float MAX_HEIGHT_COEFF = 11.0; // min density mul: 2^(-MAX_HEIGHT_COEFF)
  return all(coeffs < MAX_HEIGHT_COEFF);
}

float4 calc_height_fog_base(HeightFogNode node, float3 world_pos,
  float above_ground_height, float raw_perlin_noise, float effect, float2 coeffs)
{
  float perlinNoise = saturate((raw_perlin_noise*node.perlin_one_over_one_minus_threshold + node.perlin_scaled_threshold));
  float2 coeffsExp = saturate(exp2(-coeffs));
  float nodeDensity = (node.constant_density + perlinNoise * node.perlin_density) * coeffsExp.x * coeffsExp.y;
  nodeDensity *= effect;
  return float4(node.color*nodeDensity, nodeDensity);
}

#define EPS_DIR (1e-6)

float calc_water_hit_dist_impl(float3 view_vec, float3 world_view_pos, float water_level)
{
  FLATTEN
  if (view_vec.y > -EPS_DIR) // even inside water, so volfog is visible through water
    return MAX_RAYMARCH_FOG_DIST;
  float targetWaterLevel = water_level - UNDERWATER_HEIGHT_BIAS;
  return (targetWaterLevel - world_view_pos.y) / view_vec.y;
}
#define calc_water_hit_dist(view_vec) calc_water_hit_dist_impl(view_vec, world_view_pos.xyz, water_level)

float calc_clouds_hit_dist_impl(float3 view_vec, float3 world_view_pos, float clouds_start_alt)
{
  FLATTEN
  if (view_vec.y < EPS_DIR) // with camera above clouds, we can still show volfog below clouds
    return MAX_RAYMARCH_FOG_DIST;
  return (clouds_start_alt - world_view_pos.y) / view_vec.y;
}
#define calc_clouds_hit_dist(view_vec) calc_clouds_hit_dist_impl(view_vec, world_view_pos.xyz, nbs_clouds_start_altitude2_meters)

#define calc_ray_dist_restriction(view_vec) min(calc_water_hit_dist(view_vec), calc_clouds_hit_dist(view_vec))

void apply_underwater_modifier_impl(inout float4 media, float3 world_pos, float water_level)
{
  media *= saturate(1 + (world_pos.y - water_level) / UNDERWATER_HEIGHT_BIAS);
}

void apply_clouds_modifier_impl(inout float4 media, float3 world_pos, float clouds_start_alt)
{
  media *= saturate((clouds_start_alt - world_pos.y) / CLOUDS_HEIGHT_BIAS);
}

void apply_froxel_fading_modifier_impl(inout float4 media, float3 screen_tc, float froxel_range, float2 fading_params)
{
#if !IS_DISTANT_FOG
  float depth = volume_pos_to_depth(screen_tc.z, froxel_range);
  float fadingW = saturate(depth*fading_params.x + fading_params.y);
  fadingW*=fadingW*fadingW; // a sharper falloff to make the fading more linear after integration
  media *= fadingW;
#endif
}

void apply_volfog_modifiers_impl(inout float4 media, float3 screen_tc, float3 world_pos,
  float water_level, float clouds_start_alt, float froxel_range, float2 fading_params)
{
  apply_underwater_modifier_impl(media, world_pos, water_level);
  apply_clouds_modifier_impl(media, world_pos, clouds_start_alt);
  apply_froxel_fading_modifier_impl(media, screen_tc, froxel_range, fading_params);
}

#define apply_volfog_modifiers(media, screen_tc, world_pos) \
  apply_volfog_modifiers_impl(media, screen_tc, world_pos, water_level, nbs_clouds_start_altitude2_meters, volfog_froxel_range_params.x, froxel_fog_fading_params.xy)

// froxel stuff:

// layers > last_active_froxel_layer are (completely) occluded
uint calc_last_active_froxel_layer(Texture2D<float4> occlusion_tex, uint2 dtId)
{
  return uint(texelFetch(occlusion_tex, dtId, 0).x * 255);
}
bool is_voxel_id_occluded_cached(uint occlusion_layer, uint3 dtId)
{
  return occlusion_layer < dtId.z;
}
bool is_voxel_id_occluded_impl(Texture2D<float4> occlusion_tex, uint3 dtId)
{
  uint occlusionLayer = calc_last_active_froxel_layer(occlusion_tex, dtId.xy);
  return is_voxel_id_occluded_cached(occlusionLayer, dtId);
}
#define is_voxel_id_occluded(dtId) is_voxel_id_occluded_impl(volfog_occlusion, dtId)


#endif