
float3 gravSafeNormalize(float3 v)
{
  const float eps = 0.00001;
  // Just need to avoid zero vectors
  return normalize(v + float3(eps, eps, eps));
}

struct GravityZoneUnpack {
  float3 center;
  float3 size;
  float3 dir;
  float3x3 itm;
  float weight;
  uint shape;
  uint type;
};


GravityZoneUnpack unpack_gravity_zone(GravZonesPacked inp)
{
  GravityZoneUnpack res;

  res.itm = float3x3(inp.itm0, inp.itm1, inp.itm2);
  res.center = float3(inp.center_x, inp.center_y, inp.center_z);
  res.size = f16tof32(inp.size_dir >> 16u);
  res.dir = f16tof32(inp.size_dir);
  res.shape = (inp.extra__weight >> 18u) & 3u;
  res.type = (inp.extra__weight >> 16u) & 3u;
  res.weight = f16tof32(inp.extra__weight);

  return res;
}

struct GravityZoneUnpackPrecise {
  float3 center;
  float3 size;
  float3 dir;
  float3 boxTopLsInv;
  float3 boxBotLsInv;
  float3x3 itm;
  float weight;
  float level;
  uint shape;
  uint type;
};


GravityZoneUnpackPrecise unpack_gravity_zone(GravZonesPackedPrecise inp)
{
  GravityZoneUnpackPrecise res;

  res.itm = float3x3(inp.itm0, inp.itm1, inp.itm2);
  res.center = inp.center;
  res.size = f16tof32(inp.size_dir >> 16u);
  res.dir = f16tof32(inp.size_dir);
  res.boxTopLsInv = float3(f16tof32(inp.box_TLS_BLS_x >> 16u), f16tof32(inp.box_TLS_BLS_y >> 16u), f16tof32(inp.box_TLS_BLS_z >> 16u));
  res.boxBotLsInv = float3(f16tof32(inp.box_TLS_BLS_x), f16tof32(inp.box_TLS_BLS_y), f16tof32(inp.box_TLS_BLS_z));
  res.shape = (inp.extra >> 18u) & 3u;
  res.type = (inp.extra >> 16u) & 3u;
  res.level = f16tof32(inp.extra);
  res.weight = inp.weight;

  return res;
}

//Flattened switch is faster then early returns
float3 gravity_force(uint type, float3 dir, float3 rpos, float3 pos_ls)
{
  float3 retDir = float3(0.0f, 0.0f, 0.0f);
  float3 rdir = gravSafeNormalize(rpos);
  FLATTEN
  switch(type)
  {
    case GRAVITY_ZONE_TYPE_LINEAR:
    {
      retDir = dir;
      break;
    }
    case GRAVITY_ZONE_TYPE_CYLINDRICAL:
    {
      retDir = gravSafeNormalize(dir * pos_ls.z - rpos);
      break;
    }
    case GRAVITY_ZONE_TYPE_RADIAL:
    {
      retDir = -rdir;
      break;
    }
    case GRAVITY_ZONE_TYPE_RADIAL_INVERTED:
    {
      retDir = rdir;
      break;
    }
    default: break;
  }

  return retDir;
}


float calc_gravity_for_sphere(float3 size, float weight, float dist_sq)
{
  float dist_to_weight_mul = size.x;
  float dist_to_weight_add = size.y;
  float outerRadiusSq = size.z;
  if (dist_sq > 1e-6f && dist_sq < outerRadiusSq) {
    float dist = sqrt(dist_sq);
    return weight * saturate(dist * dist_to_weight_mul + dist_to_weight_add);
  }
  return 0.0f;
}


float calc_gravity_for_box_fast(float3 size, float weight, float3 pos_ls)
{
  if (all(pos_ls <= size))
  {
    return weight;
  }
  return 0.0f;
}

float calc_gravity_for_box_precise(float3 size, float3 topLS, float3 botLS, float weight, float3 pos_ls)
{
  if (all(pos_ls <= size ))
  {
    float3 k = min(min((size  - pos_ls) * topLS, (size  - pos_ls) * botLS), float3(1, 1, 1));
    return k.x * k.y * k.z * weight;
  }
  return 0.0f;
}

float calc_gravity_for_cylinder(float3 size, float weight, float3 pos_ls)
{
  float length = size.x;
  float radiusSq = size.y;
  bool isOkByLen = abs(pos_ls.z) <= length;
  bool isOkByRad = lengthSq(float2(pos_ls.x, pos_ls.y)) <= radiusSq;
  return isOkByLen && isOkByRad ? weight : 0.0f;
}

float3 up_slerp(float3 up, float t, float3 lastUp)
{
  FLATTEN
  if (t >= 0.98)
    return up;
  // Works well in all tested cases
  return gravSafeNormalize(lerp(lastUp, up, t));
}

float3 get_gravity_dir_impl(float3 world_pos, Texture3D<float4> tex, SamplerState tex_samplerstate, float4 grid_offset, float4 extra_offset, out float confidence)
{
  float2 tcUW = world_pos.xz*grid_offset.x + grid_offset.yz;
  float tcV = (world_pos.y + extra_offset.z) * extra_offset.w;
  float3 tc = float3(tcUW.x, tcV, tcUW.y);

  confidence = 1.0f;

  if(any(abs(tc * 2 - 1) > 1)) return float3(0, -1, 0);

  float4 samp = tex3Dlod(tex, float4(tc - float3(extra_offset.x, 0, extra_offset.y), 0));
  confidence = samp.a;

  return  samp.rgb * 2 - 1.0f;
}
