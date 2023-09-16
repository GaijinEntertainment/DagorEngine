include "heightmap_common.sh"
include "clipmap_common.sh"

macro USE_VOXELS_HEIGHTMAP_HEIGHT_25D(code)
  USE_HMAP_HOLES_ZONES(code)
  hlsl(code) {
    float ssgi_get_heightmap_2d_height_25d(float2 worldPosXZ)
    {
      float result = -2;
      if (!checkHeightmapHolesZones(float3(worldPosXZ.x, 0, worldPosXZ.y)))
        result = decode_height(getHeightLowLod(calcTcLow(worldPosXZ), 0));
      return result;
    }
    float ssgi_get_heightmap_2d_height_25d_volmap(float2 worldPosXZ)
    {
      return ssgi_get_heightmap_2d_height_25d(worldPosXZ);
    }
    float3 ssgi_get_heightmap_2d_height_25d_normal(float2 worldPosXZ)
    {
      return getNormalLod(calcTcLow(worldPosXZ), 0);
    }
  }
endmacro

macro INIT_VOXELS_HEIGHTMAP_HELPERS(code)
  INIT_WORLD_HEIGHTMAP(code)
  USE_HEIGHTMAP_COMMON(code)
  INIT_HMAP_HOLES_ZONES(code)
  USE_HMAP_HOLES_ZONES(code)
  hlsl(code) {
    float ssgi_get_heightmap_2d_height(float3 worldPos)
    {
      float result = worldPos.y - 100;
      if (!checkHeightmapHolesZones(worldPos))
        result =  decode_height(getHeightLowLod(calcTcLow(worldPos.xz), 0));
      return result;
    }
  }
endmacro

macro INIT_VOXELS_HEIGHTMAP_HEIGHT_25D(code)
  INIT_WORLD_HEIGHTMAP_BASE(code)
  USE_HEIGHTMAP_COMMON_BASE(code)
  INIT_HMAP_HOLES_ZONES(code)
  USE_VOXELS_HEIGHTMAP_HEIGHT_25D(code)
endmacro

macro USE_VOXELS_HEIGHTMAP_ALBEDO_25D(code)
  hlsl(code) {
    float3 ssgi_get_heightmap_2d_height_25d_diffuse(float2 worldPosXZ)
    {
      return sampleFallBackAlbdeoLod0(float3(worldPosXZ,0).xzy);
    }
  }
endmacro

macro INIT_VOXELS_HEIGHTMAP_ALBEDO_25D(code)
  INIT_CLIPMAP_FALLBACK_ALBEDO(code)
  USE_CLIPMAP_FALLBACK_ALBEDO(code)
  USE_VOXELS_HEIGHTMAP_ALBEDO_25D(code)
endmacro
