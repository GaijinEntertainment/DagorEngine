include "shader_global.sh"
include "gi_initial_ambient.sh"
include "dagi_scene_common_write.sh"
include "dagi_scene_voxels_common_25d.sh"
include "giHelpers/trees_above_common.sh"
include "giHelpers/voxelize_gs.sh"
include "giHelpers/common_collision_render.sh"
include_optional "optional_walls_windows.sh"

define_macro_if_not_defined OPTIONAL_SKIP_RAY_WINDOWS(code)
  SKIP_RAY_WINDOWS(code)
endmacro
define_macro_if_not_defined OPTIONAL_USE_WALLS(code)
  USE_WALLS(code)
endmacro

int gi_voxelize_with_instancing;
interval gi_voxelize_with_instancing:off<1, on;

int gi_voxelize_with_colors = 1;
macro GET_ALBEDO_25D(code)
  if (gi_quality != only_ao)
  {
    INIT_VOXELS_25D(code)
    USE_VOXELS_25D(code)
    SAMPLE_VOXELS_25D(code)
    hlsl(code) {
      void get_albedo_25d(float3 worldPos, inout float3 diffuseColor, out float3 emission)
      {
        int2 coordXZ = scene25dWorldPosToCoord(worldPos.xz);
        emission = 0;
        if (all(coordXZ>=0 && coordXZ < VOXEL_25D_RESOLUTION_X))
        {
          uint2 wrapCoord2 = wrapSceneVoxel25dCoord(coordXZ);
          float floorHt = getVoxel25dFloorCoord(wrapCoord2);
          float coordY = worldPos.y*getScene25dVoxelInvSizeY() - floorHt;
          uint3 coord = uint3(wrapCoord2, coordY);
          if (coordY >= 0 && coordY < VOXEL_25D_RESOLUTION_Y && getRawVoxel25dBufDataAlpha(coord))
            getVoxel25dColor(coord, diffuseColor, emission);
        }
      }
    }
  }
endmacro

shader voxelize_collision
{
  cull_mode = none;

  z_write = false;
  z_test = false;
  if (gi_voxelize_with_instancing != off)
  {
    hlsl {
      #define VOXELIZE_WITH_INSTANCING 1
    }
  }

  hlsl {
    struct VsOutput
    {
      centroid VS_OUT_POSITION(pos)
      noperspective centroid float3 pointToEye    : TEXCOORD0;
      nointerpolation float3 diffuseColor         : TEXCOORD2;
      #if !VOXELIZE_WITH_INSTANCING
      nointerpolation float3 worldNormal          : TEXCOORD1;
      #endif
    };
    #define FILTER_BY_NORMAL_WEIGHT 0.05
    #define HARD_GI_VIGNETTE 1
  }

  SSGE_SCENE_COMMON_INIT_WRITE_PS()
  SSGE_SCENE_COMMON_WRITE_PS()
  LIGHT_INITIAL_VOXELS()

  OPTIONAL_SKIP_RAY_WINDOWS(ps)
  OPTIONAL_USE_WALLS(ps)
  (ps){
    gi_voxelize_with_colors@i1 = (gi_voxelize_with_colors);
  }

  hlsl {
    #define world_view_pos float3(0,0,0)
  }

  GET_ALBEDO_25D(ps)
  if (gi_voxelize_with_instancing == off)
  {
    if (hardware.metal)
    {
      dont_render;
    }
    INIT_VOXELIZE_GS()
    VOXELIZE_GS(1, 0)
  } else
  {
    hlsl {
      #define GsOutput VsOutput
    }
    TO_VOXELIZE_SPACE(vs)
    USE_VOXELIZE_SPACE(vs)
  }
  hlsl(ps) {

    void gi_collision_ps(GsOutput ps_input INPUT_VFACE)
    {
      #if !VOXELIZE_WITH_INSTANCING
      VOXELIZE_DISCARD_PRIM
      half3 worldNormal = normalize(input.worldNormal);//-normalize(cross(ddx(worldPos), ddy(worldPos)))
      #else
      GsOutput input = ps_input;
      half3 worldNormal = normalize(cross(ddy(input.pointToEye), ddx(input.pointToEye)));
      worldNormal.xyz = MUL_VFACE(worldNormal.xyz);
      #endif

      float3 worldPos = world_view_pos - input.pointToEye;
      int3 unwrappedCoord = sceneWorldPosToCoord(worldPos, getCurrentCascade());
      //todo: for collision it make sense to set scissor for each axis, and then issue 3 calls, rather then everything in one call
      //with that we can allow HW rasterizer to help us discard pixels faster
      if (!(any(unwrappedCoord.xzy < scene_voxels_unwrapped_invalid_start.xyz) || any(unwrappedCoord.xzy >= scene_voxels_unwrapped_invalid_end.xyz)))//safety first. that's just faster
      {
        float3 voxelCenter = currentSceneVoxelCenter(worldPos);
        //todo: check triangle/voxel intersection
        if (!inWindow(voxelCenter))
        {
          //todo: do not sample colors if we have only_ao - as we don't write them
          half3 litColor = 0;
          BRANCH
          if (gi_voxelize_with_colors)//we can't create variant as well
          {
            float3 diffuseColor = input.diffuseColor.rgb, emission = 0;
          ##if gi_quality != only_ao
            get_albedo_25d(worldPos, diffuseColor, emission);
          ##endif
            float3 filteredWorldPos = worldPos;
            ssgi_pre_filter_worldPos(filteredWorldPos, ssgi_ambient_volmap_crd_to_world0_xyz(1).y);
            litColor = getDiffuseLighting(worldPos, filteredWorldPos, worldNormal, diffuseColor, emission);
          }
          //writeSceneVoxelDataSafe(worldPos,litColor,1);
          //we write data unsafe, as we implemented safety in the begining of shader
          writeVoxelData(wrapSceneVoxelCoord(unwrappedCoord, getCurrentCascade()), getCurrentCascade(), litColor, 1. / SCENE_VOXELS_ALPHA_SCALE);
        }
      }
    }
  }

  (vs) {
    voxelize_box_min@f3 = voxelize_box0;
    voxelize_box_sz@f3 = (1./voxelize_box1.x, 1./voxelize_box1.y, 1./voxelize_box1.z, 0);
  }

  COMMON_COLLISION_RENDER()

  hlsl(vs) {

    #include <dagi_voxels_consts.hlsli>
    VsOutput gi_collision_vs(VsInput input, uint id : SV_InstanceID)
    {
      VsOutput output;
      float3 diffuseColor = 0.06;

      float3 localPos = input.pos;
      uint inst = id;
      #if USE_MULTI_DRAW
        inst = input.id;
      #endif
      uint baseInst = get_immediate_dword_0();
      uint finalInstId = baseInst + inst;
      uint matrixOffset = finalInstId;
      #if VOXELIZE_WITH_INSTANCING
        uint angle = (finalInstId%3);
        //if we VOXELIZE_WITH_INSTANCING, we have 3 instances per same object (for projections using instances), so just remove reminder
        matrixOffset -= angle;
      #else
        matrixOffset *= 3;
      #endif
      float3 worldLocalX = float3(structuredBufferAt(matrices, matrixOffset).x, structuredBufferAt(matrices, matrixOffset + 1).x, structuredBufferAt(matrices, matrixOffset + 2).x);
      float3 worldLocalY = float3(structuredBufferAt(matrices, matrixOffset).y, structuredBufferAt(matrices, matrixOffset + 1).y, structuredBufferAt(matrices, matrixOffset + 2).y);
      float3 worldLocalZ = float3(structuredBufferAt(matrices, matrixOffset).z, structuredBufferAt(matrices, matrixOffset + 1).z, structuredBufferAt(matrices, matrixOffset + 2).z);
      float3 worldLocalPos = float3(structuredBufferAt(matrices, matrixOffset).w, structuredBufferAt(matrices, matrixOffset + 1).w, structuredBufferAt(matrices, matrixOffset + 2).w);
      float3 worldPos = localPos.x * worldLocalX + localPos.y * worldLocalY + localPos.z * worldLocalZ + worldLocalPos;
      #if VOXELIZE_WITH_INSTANCING
      output.pos = worldPosToVoxelSpace(worldPos, angle);
      /*float3 boxPos = worldPos*voxelize_world_to_rasterize_space_mul + voxelize_world_to_rasterize_space_add;
      output.pos.xy = boxPos.yz;
      if (angle == 1)
        output.pos.xy = boxPos.xy;
      if (angle == 2)
        output.pos.xy = boxPos.xz;
      output.pos.zw = float2(0.5,1);*/
      #else
      output.worldNormal = 0;
      output.pos = mul(float4(worldPos, 1), globtm);
      #endif
      output.pointToEye = world_view_pos - worldPos;
      output.diffuseColor = diffuseColor.rgb;

      return output;
    }
  }

  compile("target_ps", "gi_collision_ps");
  if (gi_voxelize_with_instancing == off)
  {
    compile("target_vs_for_gs", "gi_collision_vs");
  } else
  {
    compile("target_vs", "gi_collision_vs");
  }
}

buffer gi_voxelization_vbuffer;
buffer gi_voxelization_ibuffer;

shader voxelize_collision_cs
{
  SSGE_SCENE_COMMON_INIT_WRITE(cs)
  SSGE_SCENE_COMMON_WRITE(cs)
  LIGHT_INITIAL_VOXELS_BASE(cs)

  OPTIONAL_SKIP_RAY_WINDOWS(cs)
  OPTIONAL_USE_WALLS(cs)

  hlsl {
    #define world_view_pos float3(0,0,0)
  }

  GET_ALBEDO_25D(cs)

  (cs) {immediate_dword_count = 4;}
  (cs) {
    matrices@buf = collision_voxelization_tm hlsl { StructuredBuffer<float4> matrices@buf; };
    gi_voxelization_vbuffer@buf = gi_voxelization_vbuffer hlsl { ByteAddressBuffer gi_voxelization_vbuffer@buf; };
    gi_voxelization_ibuffer@buf = gi_voxelization_ibuffer hlsl { ByteAddressBuffer gi_voxelization_ibuffer@buf; };
    voxelize_box_min@f3 = voxelize_box0;
    voxelize_box_max@f3 = (voxelize_box0.x + 1./voxelize_box1.x, voxelize_box0.y + 1./voxelize_box1.y, voxelize_box0.z + 1./voxelize_box1.z, 0);
    gi_voxelize_with_colors@i1 = (gi_voxelize_with_colors);
  }

  hlsl(cs) {

    void getAlbedo(float3 worldPos, inout float3 diffuseColor, out float3 emission)
    {
      emission = 0;
    ##if gi_quality != only_ao
      if (gi_voxelize_with_colors)
      {
        get_albedo_25d(worldPos, diffuseColor, emission);
      }
    ##endif
    }
    void write_voxel(float3 worldPos, float3 worldNormal, float3 diffuseColor)
    {
      float3 voxelCenter = currentSceneVoxelCenter(worldPos);
      //todo: check triangle/voxel intersection
      if (inWindow(voxelCenter))
        return;
      half3 litColor = 0;
      if (gi_voxelize_with_colors)
      {
        float3 emission = 0;
        getAlbedo(worldPos, diffuseColor, emission);
        float3 filteredWorldPos = worldPos;
        ssgi_pre_filter_worldPos(filteredWorldPos, ssgi_ambient_volmap_crd_to_world0_xyz(1).y);
        litColor = getDiffuseLighting(worldPos, filteredWorldPos, worldNormal, diffuseColor, emission);
      }
      writeSceneVoxelDataSafe(worldPos,litColor,1);
    }

    void write_voxel_center(float3 voxelCenter, float3 worldPos, float3 worldNormal, float3 diffuseColor)
    {
      if (inWindow(voxelCenter))
        return;

      half3 litColor = 0;
      if (gi_voxelize_with_colors)
      {
        float3 emission = 0;
        getAlbedo(worldPos, diffuseColor, emission);
        float3 filteredWorldPos = worldPos;
        ssgi_pre_filter_worldPos(filteredWorldPos, ssgi_ambient_volmap_crd_to_world0_xyz(1).y);
        litColor = getDiffuseLighting(worldPos, filteredWorldPos, worldNormal, diffuseColor, emission);
      }
      writeSceneVoxelDataSafe(worldPos,litColor,1);
    }

    void write_voxel_center_albedo(float3 voxelCenter, float3 worldPos, float3 worldNormal, float3 diffuseColor, float3 emission)
    {
      if (inWindow(voxelCenter))
        return;
      half3 litColor = 0;
      if (gi_voxelize_with_colors)
      {
        float3 filteredWorldPos = worldPos;
        ssgi_pre_filter_worldPos(filteredWorldPos, ssgi_ambient_volmap_crd_to_world0_xyz(1).y);
        litColor = getDiffuseLighting(worldPos, filteredWorldPos, worldNormal, diffuseColor, emission);
      }
      writeSceneVoxelDataSafe(worldPos,litColor,1);
    }


    bool intersects_tri_aabb_onto_axis(float3 v0, float3 v1, float3 v2, float extents, float3 axis)
    {
      // project all 3 vertices of the triangle onto the axis
      float p0 = dot(v0, axis);
      float p1 = dot(v1, axis);
      float p2 = dot(v2, axis);

      // project the AABB onto the axis
      float r = extents * abs(axis.x) + extents * abs(axis.y) + extents * abs(axis.z);
      float minP = min(p0, min(p1, p2));
      float maxP = max(p0, max(p1, p2));
      return !((maxP < -r) || (r < minP));
    }

    bool intersects_plane_aabb(float3 normal, float distance, float3 center, float extents)
    {
      float r = extents * abs(normal.x) + extents * abs(normal.y) + extents * abs(normal.z);
      float s = dot(normal, center) - distance;

      return abs(s) <= r;
    }

    bool intersects_tri_aabb(float4 plane, float3 va, float3 vb, float3 vc, float3 center, float extents)
    {
      if (!intersects_plane_aabb(plane.xyz, plane.w, center, extents))
        return false;
      float p0, p1, p2, r;

      // translate the triangle as conceptually moving the AABB to origin
      float3 v0 = va - center,
      v1 = vb - center,
      v2 = vc - center;
      if (any(max(v0, max(v1, v2)) < -extents) || any(min(v0, min(v1, v2)) > extents))
        return false;

      // compute the edge vectors of the triangle
      // get the lines between the points as vectors
      float3 f0 = v1 - v0,
      f1 = v2 - v1,
      f2 = v0 - v2;

      // cross products of triangle edges & aabb edges
      // AABB normals are the x (1, 0, 0), y (0, 1, 0), z (0, 0, 1) axis.
      // so we can get the cross products between triangle edge vectors and AABB normals without calculation
      float3 a00 = float3(0, -f0.z, f0.y), // cross product of X and f0
      a01 = float3(0, -f1.z, f1.y), // X and f1
      a02 = float3(0, -f2.z, f2.y), // X and f2
      a10 = float3(f0.z, 0, -f0.x), // Y and f0
      a11 = float3(f1.z, 0, -f1.x), // Y and f1
      a12 = float3(f2.z, 0, -f2.x), // Y and f2
      a20 = float3(-f0.y, f0.x, 0), // Z and f0
      a21 = float3(-f1.y, f1.x, 0), // Z and f1
      a22 = float3(-f2.y, f2.x, 0); // Z and f2

      // Test 9 axes
      if (
          !intersects_tri_aabb_onto_axis(v0, v1, v2, extents, a00) ||
          !intersects_tri_aabb_onto_axis(v0, v1, v2, extents, a01) ||
          !intersects_tri_aabb_onto_axis(v0, v1, v2, extents, a02) ||
          !intersects_tri_aabb_onto_axis(v0, v1, v2, extents, a10) ||
          !intersects_tri_aabb_onto_axis(v0, v1, v2, extents, a11) ||
          !intersects_tri_aabb_onto_axis(v0, v1, v2, extents, a12) ||
          !intersects_tri_aabb_onto_axis(v0, v1, v2, extents, a20) ||
          !intersects_tri_aabb_onto_axis(v0, v1, v2, extents, a21) ||
          !intersects_tri_aabb_onto_axis(v0, v1, v2, extents, a22)
      )
      {
          return false;
      }

      // Test triangle normal
      return true;
    }

    float3 getVertex(uint ia, uint vbOffset)
    {
      ia = (vbOffset + ia)*2*4;
      uint2 a = uint2(loadBuffer(gi_voxelization_vbuffer, ia), loadBuffer(gi_voxelization_vbuffer, ia + 4));
      return float3(f16tof32(a.x), f16tof32(a.x>>16), f16tof32(a.y));
    }

    bool getTriangle(
      uint firstIndex, uint vbOffset,
      inout float3 va, inout float3 vb, inout float3 vc)
    {
      uint ibAt = firstIndex>>1;
      uint2 a = uint2(loadBuffer(gi_voxelization_ibuffer, ibAt*4), loadBuffer(gi_voxelization_ibuffer, ibAt*4+4));
      int ia = a.x&0xFFFF, ib = a.x>>16, ic = a.y&0xFFFF, id = a.y>>16;
      if (ib == ic)
        return false;
      FLATTEN
      if (firstIndex&1)
      {
        ia = ib;
        ib = ic;
        ic = id;
      }

      va = getVertex(ia, vbOffset);
      vb = getVertex(ib, vbOffset);
      vc = getVertex(ic, vbOffset);
      return true;
    }

    [numthreads(64, 1, 1)]
    void gi_collision_cs(uint id : SV_DispatchThreadID)
    {
      uint trianglesInstCount = get_immediate_dword_2();
      uint trianglesCount = trianglesInstCount&((1U<<20)-1);
      uint instCount = trianglesInstCount>>20;
      uint tri = id%trianglesCount;
      uint instanceNo = id/trianglesCount;
      if (instanceNo >= instCount)
        return;
      uint indexOffset = get_immediate_dword_0();
      uint vertexOffset = get_immediate_dword_1();

      if (tri >= trianglesCount)
        return;

      uint matrixOffset = (get_immediate_dword_3()+instanceNo)*3;

      float3 worldLocalX = float3(matrices[matrixOffset].x, matrices[matrixOffset + 1].x, matrices[matrixOffset + 2].x);
      float3 worldLocalY = float3(matrices[matrixOffset].y, matrices[matrixOffset + 1].y, matrices[matrixOffset + 2].y);
      float3 worldLocalZ = float3(matrices[matrixOffset].z, matrices[matrixOffset + 1].z, matrices[matrixOffset + 2].z);
      float3 worldLocalPos = float3(matrices[matrixOffset].w, matrices[matrixOffset + 1].w, matrices[matrixOffset + 2].w);
      //writeSceneVoxelDataSafe(worldLocalPos,0.1,1);
      float3 va, vb, vc;
      if (!getTriangle(tri*3+indexOffset, vertexOffset, va, vb, vc))
        return;
      va = va.x * worldLocalX + va.y * worldLocalY + va.z * worldLocalZ + worldLocalPos;
      vb = vb.x * worldLocalX + vb.y * worldLocalY + vb.z * worldLocalZ + worldLocalPos;
      vc = vc.x * worldLocalX + vc.y * worldLocalY + vc.z * worldLocalZ + worldLocalPos;
      float3 triBoxMin = min(min(va, vb), vc), triBoxMax = max(max(va, vb), vc);
      if (any(triBoxMax<voxelize_box_min) || any(triBoxMin>voxelize_box_max))//no need to even try
        return;
      float3 edgeBA = vb-va, edgeAC=va-vc;
      float3 worldNormal = normalize(cross(edgeBA, edgeAC));
      float3 diffuseColor = 0.06;
      uint cascade = getCurrentCascade();
      float3 origin = getSceneVoxelOrigin(cascade), voxelSize = getSceneVoxelSize(cascade);
      float3 clampedTriBoxMin = max(triBoxMin, voxelize_box_min), clampedTriBoxMax = min(triBoxMax, voxelize_box_max);
      //to be calculated once!
      //int3 minUpdateCoord = scene_voxels_unwrapped_invalid_start.xyz, maxUpdateCoord = scene_voxels_unwrapped_invalid_end.xyz;
      //--to be calculated once!
      int3 minUnWrappedCoord = max(sceneWorldPosToCoord(clampedTriBoxMin, cascade), scene_voxels_unwrapped_invalid_start.xyz),
           maxUnWrappedCoord = min(sceneWorldPosToCoord(clampedTriBoxMax, cascade), scene_voxels_unwrapped_invalid_end.xyz);
      uint3 coordWidth = maxUnWrappedCoord-minUnWrappedCoord;
      if (all(coordWidth==0))
      {
        write_voxel((va+vb+vc)*1./3, worldNormal, diffuseColor);
        return;
      }

      coordWidth+=1;
      float4 normalPlane = float4(worldNormal, dot(worldNormal, va));

      float3 emission;
      getAlbedo((va+vb+vc)*1./3, diffuseColor, emission);

      float3 voxelCenterStart = sceneCoordToWorldPos(minUnWrappedCoord, cascade);
      LOOP
      for (uint i = 0, e = coordWidth.x*coordWidth.y*coordWidth.z; i<e; ++i)
      {
        uint3 cCoordOfs = uint3(i%coordWidth.x, (i/coordWidth.x)%coordWidth.y, i/(coordWidth.x*coordWidth.y));
        int3 unwrappedCoord = cCoordOfs+minUnWrappedCoord;
        uint3 coord = wrapSceneVoxelCoord(unwrappedCoord, cascade);
        //write_voxel(sceneCoordToWorldPos(unwrappedCoord, cascade), worldNormal, diffuseColor);
        float3 voxelCenter = voxelCenterStart+cCoordOfs.xzy*voxelSize;
        float3 worldPos = voxelCenter;
        if (intersects_tri_aabb(normalPlane, va, vb, vc, worldPos, voxelSize.x*0.5))
          write_voxel_center_albedo(voxelCenter, worldPos, worldNormal, diffuseColor, emission);
          //write_voxel_center(voxelCenter, worldPos, worldNormal, diffuseColor);
      }
    }
    #if 0
    [numthreads(32, 1, 1)]
    void gi_collision_cs_thread(uint3 id : SV_GroupID, uint threadId:SV_GroupThreadID)
    {
      int3 startInvalid = int3(scene_voxels_invalid_start.xyz);
      uint4 invalidWidth = uint4(scene_voxels_invalid_width);
      uint cascade = getCurrentCascade();
      //int3 coord = (startInvalid + int3(id));
      //float3 voxelCenter = sceneCoordToWorldPos(coord, cascade);
      int3 coord = startInvalid.xzy + int3(id);
      float3 voxelSize = getSceneVoxelSize(cascade);
      float3 voxelCenter = voxelSize*(float3(coord.xzy)+0.5);
      //voxelCenter = voxelize_box_min + (id.xzy+0.5)*voxelSize;
      //if (voxelCenter.y<10 && all((coord&1)==0))
      //writeSceneVoxelDataSafe(voxelCenter,frac(abs(coord)*0.05),1);
      // return;
      float3 voxelExtents = 0.5*getSceneVoxelSize(cascade);
      float3 voxelMin = voxelCenter-voxelExtents, voxelMax = voxelCenter+voxelExtents;

      uint instances = get_immediate_dword_0();
      uint instancesToProceed = (instances+31)/32, instanceFrom = threadId*instancesToProceed, instanceTo = min(instances, instanceFrom+instancesToProceed);
      //for (uint i = instanceFrom; i < instanceTo; ++i)
      for (uint i = 0; i < instances; ++i)
      {
        uint matrixOffset = i*4;
        float3 worldLocalX = float3(matrices[matrixOffset].x, matrices[matrixOffset + 1].x, matrices[matrixOffset + 2].x);
        float3 worldLocalY = float3(matrices[matrixOffset].y, matrices[matrixOffset + 1].y, matrices[matrixOffset + 2].y);
        float3 worldLocalZ = float3(matrices[matrixOffset].z, matrices[matrixOffset + 1].z, matrices[matrixOffset + 2].z);
        float3 worldLocalPos = float3(matrices[matrixOffset].w, matrices[matrixOffset + 1].w, matrices[matrixOffset + 2].w);
        //todo: check box!!!
        uint indexOffset = matrices[matrixOffset+3].x, vertexOffset = matrices[matrixOffset+3].y, triCount = matrices[matrixOffset+3].z;
        for (uint tri = indexOffset, e = triCount*3+indexOffset; tri < e; tri+=3)
        {
          float3 va, vb, vc;
          getTriangle(tri, vertexOffset, va, vb, vc);
          va = va.x * worldLocalX + va.y * worldLocalY + va.z * worldLocalZ + worldLocalPos;
          vb = vb.x * worldLocalX + vb.y * worldLocalY + vb.z * worldLocalZ + worldLocalPos;
          vc = vc.x * worldLocalX + vc.y * worldLocalY + vc.z * worldLocalZ + worldLocalPos;
          float3 triBoxMin = min(min(va, vb), vc), triBoxMax = max(max(va, vb), vc);
          if (any(triBoxMax<voxelMin) || any(triBoxMin>voxelMax))//no need to even try
            continue;
          float3 edgeBA = vb-va, edgeAC=va-vc;
          float3 worldNormal = normalize(cross(edgeBA, edgeAC));
          float4 normalPlane = float4(worldNormal, dot(worldNormal, va));
          float3 diffuseColor = 0.06;
          float3 worldPos = voxelCenter;//todo: fixme
          //write_voxel(voxelCenter, worldNormal, diffuseColor);
          //write_voxel((va + vb + vc)/3, worldNormal, diffuseColor);
          if (intersects_tri_aabb(normalPlane, va, vb, vc, worldPos, voxelExtents.x))
            write_voxel_center(voxelCenter, worldPos, worldNormal, diffuseColor);
        }
      }
      //write_voxel(va, worldNormal, diffuseColor);
      //writeSceneVoxelDataSafe((va+vb+vc)*1./3,float3(1,0,0),1);
      //write_voxel((va+vb+vc)*1./3, worldNormal, float3(1,0,0));
    }
    #endif
  }
  compile("target_cs", "gi_collision_cs");
}

shader voxelize_trees_lit
{
  supports none;
  supports global_frame;
  z_test = false;
  z_write = false;
  cull_mode = none;

  hlsl {
    struct VsOutput
    {
      VS_OUT_POSITION(pos)
      float2 texcoord:TEXCOORD0;
    };
  }
  hlsl(vs) {
    VsOutput trees_depth_to_voxels_vs(uint vertexId : SV_VertexID)
    {
      VsOutput output;
      float2 inpos = get_fullscreen_inpos(vertexId);
      output.pos = float4(inpos.xy, 1, 1);
      output.texcoord = inpos.xy*float2(0.5, -0.5) + 0.5;
      return output;
    }
  }
  //INIT_WORLD_HEIGHTMAP_PS()
  SSGE_SCENE_COMMON_INIT_WRITE_PS()
  SSGE_SCENE_COMMON_WRITE_PS()
  LIGHT_INITIAL_VOXELS()
  OPTIONAL_SKIP_RAY_WINDOWS(ps)
  (ps) {
    voxelize_box0@f3 = voxelize_box0;
    voxelize_box1@f3 = (1/voxelize_box1.x,1/voxelize_box1.y,1/voxelize_box1.z,0);
  }
  USE_HEIGHTMAP_COMMON_BASE(ps)
  USE_TREES_ABOVE(ps)
  hlsl(ps) {
    void trees_depth_to_voxels_ps(VsOutput input)
    {
      float2 worldPosXZ = voxelize_box1.xz*input.texcoord + voxelize_box0.xz;
      float3 worldPos; half3 diffuseColor;
      if (!trees_world_pos_to_worldPos(worldPosXZ, worldPos, diffuseColor, false))
        return;
      //{
      //  diffuseColor = float3(1,0,0); worldPos = float3(worldPosXZ, 72).xzy;
      //  //return;
      //}
      float3 translucencyColor = diffuseColor;
      half3 litColor = getDiffuseLighting(worldPos, worldPos, float3(0,1,0), diffuseColor, 0) +
                      getTranslucentLighting(translucencyColor, worldPos, float3(0,1,0));
      //half3 litColor = half3(1,0,0);
      //worldPos = float3(worldPosXZ, 72).xzy;
      if (!inWindow(currentSceneVoxelCenter(worldPos)))
        writeSceneVoxelDataSafe(worldPos, litColor, 0.7);

      worldPos.y += getSceneVoxelSize(getCurrentCascade()).y;//this one is pure speculation, based on that we render tree from below
      if (!inWindow(currentSceneVoxelCenter(worldPos)))
        writeSceneVoxelDataSafe(worldPos, litColor, 0.5);//since it is speculation, make it more transparent
    }
  }

  compile("target_vs", "trees_depth_to_voxels_vs");
  compile("target_ps", "trees_depth_to_voxels_ps");
}
