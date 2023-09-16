macro INIT_PIVOT_PAINTER_PARAMS()
  texture pp_position = material.texture[7];
  texture pp_direction = material.texture[8];

  static float wind_noise_speed_base = 0.1;
  static float wind_noise_speed_level_mul = 1.666;

  static float wind_angle_rot_base = 5;
  static float wind_angle_rot_level_mul = 5;
  static float4 wind_per_level_angle_rot_max = (60, 60, 60, 60);

  static float wind_parent_contrib = 0.25;

  static float wind_motion_damp_base = 3;
  static float wind_motion_damp_level_mul = 0.8;
endmacro

/*  Code to perform hierarchical vertex transformations, based on supplied set of
    textures with data on parent pivot positions, direction and index.
    Shamelessly stolen from UE4 Pivot Painter 2
    MaterialFunction'/Engine/Functions/Engine_MaterialFunctions02/PivotPainter2/PivotPainter2FoliageShader.PivotPainter2FoliageShader'
*/
macro USE_PIVOT_PAINTER()
  (vs) {
    pp_pos_tex@static = pp_position;
    pp_dir_tex@static = pp_direction;

    wind_noise_speed_base@f1 = (wind_noise_speed_base);
    wind_noise_speed_level_mul@f1 = (wind_noise_speed_level_mul);

    wind_angle_rot_base@f1 = (wind_angle_rot_base);
    wind_angle_rot_level_mul@f1 = (wind_angle_rot_level_mul);
    wind_per_level_angle_rot_max@f4 = (wind_per_level_angle_rot_max);

    wind_parent_contrib@f1 = (wind_parent_contrib);

    wind_motion_damp_base@f1 = (wind_motion_damp_base);
    wind_motion_damp_level_mul@f1 = (wind_motion_damp_level_mul);
  }

  hlsl(vs) {
    // This should be parameter and be EQUAL to houdini's i@gen max value
    static const int MAX_ALLOWED_HIERARCHY = 4;

    static const int PP_RES_X = 32;
    static const int PP_RES_Y = 64;

    static const float PP_EXTENT = 20.48;

    float3 scaleRotateLocalWorld(float3 v, float3 X, float3 Y, float3 Z)
    {
      return X * v.x + Y * v.y + Z * v.z;
    }

    float3 transformLocalWorld(float3 v, float3 X, float3 Y, float3 Z, float3 P)
    {
      return scaleRotateLocalWorld(v, X, Y, Z) + P;
    }

    float2 getUvByIndex(uint index)
    {
      float2 invRes = 1.0/float2(PP_RES_X, PP_RES_Y);
      float2 idx2D = float2(index % PP_RES_X, index / PP_RES_X);

      return (idx2D + 0.5) * invRes;
    }

    int getIndexByUv(float2 uv)
    {
      int2 idx2D = int2(floor(uv * int2(PP_RES_X, PP_RES_Y)));

      return idx2D[0] + idx2D[1] * PP_RES_X;
    }

    int getParentIndexAndPivotPos(float2 coords, float3 X, float3 Y, float3 Z, float3 P, out float3 pivotPos)
    {
      float4 result = tex2DLodBindless(get_pp_pos_tex(), float4(coords,0,0));

      pivotPos = transformLocalWorld(result.xyz, X, Y, Z, P);

      // Unpack int from float
      int idx = int(result.w + 0.5);

      return idx;
    }

    float4 getParentPivotDirectionAndExtent(float2 coords, float3 X, float3 Y, float3 Z)
    {
      float4 result = tex2DLodBindless(get_pp_dir_tex(), float4(coords,0,0));

      result.xyz = scaleRotateLocalWorld( result.xyz * 2.0 - 1.0, X, Y, Z );
      float len = max(length(result.xyz), 0.01);
      result.xyz /= len;

      result.w *= PP_EXTENT;

      return result;
    }

    struct HierarchyData
    {
      // Maximal hierarchy level found
      int maxHierarchyLevel;

      // Hierarchical attributes, 0 == current level, 1 == parent, 2 == parent of parent, etc.
      float3 branchPivotPos[MAX_ALLOWED_HIERARCHY];
      float4 branchDirExtent[MAX_ALLOWED_HIERARCHY];
    };

    HierarchyData fetchHierarchyData(float2 startCoords, float3 X, float3 Y, float3 Z, float3 P)
    {
      HierarchyData Data;

      float2 currentUV = startCoords;
      Data.maxHierarchyLevel = 0;
      // Iterate starting from SELF, as in HierarchyData
      for(int level = 0; level < MAX_ALLOWED_HIERARCHY; ++level)
      {
        int curIdx = getIndexByUv(currentUV);
        int parentIdx = getParentIndexAndPivotPos(currentUV, X, Y, Z, P, Data.branchPivotPos[level].xyz);
        Data.branchDirExtent[level].xyzw = getParentPivotDirectionAndExtent(currentUV, X, Y, Z);

        // False will happen at trunk level
        if (curIdx != parentIdx)
        {
          Data.maxHierarchyLevel++;

          // Next level UVs
          currentUV = getUvByIndex(parentIdx);
        }
      }

      return Data;
    }

    // Geometric progression based on hierarchy level
    // Probably good idea to normalize to (0, MAX_ALLOWED_HIERARCHY) range
    float getWindSpeedMult(int idx)
    {
      return 4.0 * get_wind_noise_speed_base() * pow(get_wind_noise_speed_level_mul(), idx);
    }

    float getAngleRotationRate(int idx)
    {
      return get_wind_angle_rot_base() + get_wind_angle_rot_level_mul() * idx;
    }

    float getMotionDampeningFalloffRadius(int idx)
    {
      return get_wind_motion_damp_base() * pow(get_wind_motion_damp_level_mul(), idx);
    }

    float4 makeQuat(float3 axis, float angle)
    {
      return float4(axis.x*sin(angle/2),
                    axis.y*sin(angle/2),
                    axis.z*sin(angle/2),
                    cos(angle/2));
    }

    float3 quatRotate(float4 q, float3 v)
    {
      float3 t = 2 * cross(q.xyz, v);
      return v + q.w * t + cross(q.xyz, t);
    }

    void applyWindAnimationOffset(float3 oldPos, inout float3 inOutNormal, HierarchyData input, float time,
                                  out float3 currNewPos, out float3 prevNewPos)
    {
      float currParentAngleAnimation = 0.0, prevParentAngleAnimation = 0.0;
      float3 currOffset = 0.0;
      float3 prevOffset = 0.0;
      float3 normal = inOutNormal;

      // Iterate starting from trunk at 0
      for(int idx=0; idx <= input.maxHierarchyLevel; ++idx)
      {
        // Because HierarchyData and this loop have reversed ordering, we need reversed index
        int revIdx = max(input.maxHierarchyLevel - idx, 0);

        float3 branchPivotPos = input.branchPivotPos[revIdx];
        float3 branchDir = input.branchDirExtent[revIdx].xyz;
        float branchExtent = input.branchDirExtent[revIdx].w;

        float3 branchTipPos = branchPivotPos + branchDir*branchExtent;
        { // evaluate current wind animation
          float3 windDirectionScale = 0.25 * sampleWindCurrentTime(branchTipPos, getWindSpeedMult(idx), 0);
          float windSpeed = length(windDirectionScale);

          float rotationAngleAnimation = 0;
          rotationAngleAnimation += windSpeed * getAngleRotationRate(idx);
          rotationAngleAnimation += saturate(windSpeed) * currParentAngleAnimation * get_wind_parent_contrib();
          rotationAngleAnimation = min(rotationAngleAnimation, get_wind_per_level_angle_rot_max()[idx]);

          float3 dirFromRoot = (oldPos - branchPivotPos);
          float alongBranch = dot(dirFromRoot, branchDir);
          float falloff = getMotionDampeningFalloffRadius(idx) * branchExtent;
          float motionMask = saturate(alongBranch / falloff);

          float rotationAngle = rotationAngleAnimation * motionMask;

          float3 rotationAxis = cross(branchDir, windDirectionScale / max(windSpeed, 0.001));
          rotationAngle *= length(rotationAxis);
          //Protect cross-product from returning 0
          rotationAxis = lerp(branchDir, rotationAxis, saturate(windSpeed / 0.001));
          rotationAxis = normalize(rotationAxis);

          float4 rotQuat = makeQuat(rotationAxis, rotationAngle * PI / 180);
          float3 newPos = quatRotate(rotQuat, oldPos-branchPivotPos) + branchPivotPos;

          normal = quatRotate(rotQuat, normal);
          currOffset += newPos - oldPos;
          currParentAngleAnimation += rotationAngleAnimation;
        }
        { // evaluate previous wind animation
          float3 windDirectionScale = 0.25 * sampleWindPreviousTime(branchTipPos, getWindSpeedMult(idx), 0);
          float windSpeed = length(windDirectionScale);

          float rotationAngleAnimation = 0;
          rotationAngleAnimation += windSpeed * getAngleRotationRate(idx);
          rotationAngleAnimation += saturate(windSpeed) * prevParentAngleAnimation * get_wind_parent_contrib();
          rotationAngleAnimation = min(rotationAngleAnimation, get_wind_per_level_angle_rot_max()[idx]);

          float3 dirFromRoot = (oldPos - branchPivotPos);
          float alongBranch = dot(dirFromRoot, branchDir);
          float falloff = getMotionDampeningFalloffRadius(idx) * branchExtent;
          float motionMask = saturate(alongBranch / falloff);

          float rotationAngle = rotationAngleAnimation * motionMask;

          float3 rotationAxis = cross(branchDir, windDirectionScale / max(windSpeed, 0.001));
          rotationAngle *= length(rotationAxis);
          //Protect cross-product from returning 0
          rotationAxis = lerp(branchDir, rotationAxis, saturate(windSpeed / 0.001));
          rotationAxis = normalize(rotationAxis);

          float4 rotQuat = makeQuat(rotationAxis, rotationAngle * PI / 180);
          float3 newPos = quatRotate(rotQuat, oldPos-branchPivotPos) + branchPivotPos;

          prevOffset += newPos - oldPos;
          prevParentAngleAnimation += rotationAngleAnimation;
        }
      }

      inOutNormal = normal;
      currNewPos = oldPos + currOffset;
      prevNewPos = oldPos + prevOffset;
    }
  }
endmacro
