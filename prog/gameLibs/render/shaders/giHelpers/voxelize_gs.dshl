float4 voxelize_box0 always_referenced;
float4 voxelize_box1 always_referenced;

float4 voxelize_world_to_rasterize_space_mul;
float4 voxelize_world_to_rasterize_space_add;

macro INIT_VOXELIZE_GS()
  (vs) {
    voxelize_box0@f3 = voxelize_box0;//todo: move to block
    voxelize_box1@f4 = voxelize_box1;//todo: move to block
  }
endmacro

macro TO_VOXELIZE_SPACE(code)
  (code) {
    voxelize_world_to_rasterize_space_mul@f3 = voxelize_world_to_rasterize_space_mul;
    voxelize_world_to_rasterize_space_add@f3 = voxelize_world_to_rasterize_space_add;
  }
endmacro

macro USE_VOXELIZE_SPACE(code)
  hlsl(code) {
    float4 worldPosToVoxelSpace(float3 worldPos, uint axis)
    {
      float3 boxPos = worldPos*voxelize_world_to_rasterize_space_mul + voxelize_world_to_rasterize_space_add;
      float4 clipPos;
      clipPos.xyz = (axis == 2 ? boxPos.xzy : ((axis == 1) ? boxPos.xyz : boxPos.yzx));
      clipPos.z = clipPos.z*0.5 + 0.5;
      clipPos.w = 1;
      return clipPos;
    }
  }
endmacro

macro VOXELIZE_GS(write_world_normal, clamp_world_pos)
  hlsl {
    struct GsOutput
    {
      VsOutput v;
      nointerpolation float3 aaBBMin    : TEXCOORD10;
      nointerpolation float3 aaBBMax    : TEXCOORD11;
    };
  }

  hlsl(gs) {
    #define CLAMP_WORLD_POS clamp_world_pos
    #define WRITE_WORLD_NORMAL write_world_normal
    #include <voxelize_gs.hlsl>
  }
  hlsl(ps) {
    #define VOXELIZE_DISCARD_PRIM\
      VsOutput input = ps_input.v;\
      {\
        float3 worldPos = world_view_pos.xyz - input.pointToEye.xyz;\
        if (any(worldPos < ps_input.aaBBMin) || any(worldPos > ps_input.aaBBMax))\
          discard;\
      }
  }
  compile("target_gs", "voxelize_gs");
endmacro
