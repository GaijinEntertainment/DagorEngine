float   hcam_deformation_quality = 1.0;

int colliders_count = 1;
int xray_part_no = 0;
int xray_tessellation_prepass = 0;
int xray_show_debug_destruction_groups = 0;
float xray_tessellation_prepass_triangle_size = 0.2;
float xray_affector_strength_threshold = 0.0;
buffer xray_simulation_buffer;
buffer xray_tessellated_vertex_buffer;
buffer xray_vertex_group_counts;

hlsl(ps) {
  float3 get_deformation_color(float deform)
  {
    return lerp(lerp(float3(0, 0, 1), float3(0, 1, 0), saturate(2*deform)), float3(1, 0, 0), saturate(2*deform-1));
  }
}

macro INIT_XRAY_DESTRUCTION()
  (vs) {
    xray_simulation_buffer@buf = xray_simulation_buffer hlsl {
      StructuredBuffer<float4> xray_simulation_buffer@buf;
    };
    xray_tessellated_vertex_buffer@buf = xray_tessellated_vertex_buffer hlsl {
      StructuredBuffer<float4> xray_tessellated_vertex_buffer@buf;
    }
    xray_vertex_group_counts@buf = xray_vertex_group_counts hlsl {
      StructuredBuffer<uint> xray_vertex_group_counts@buf;
    }
    hcam_deformation_quality@f1 = (0.3 + 0.7 * hcam_deformation_quality);
    xray_part_no@i1 = (xray_part_no);
    colliders_count_f@f1 = (min(colliders_count+0.1, 30));
    xray_tessellation_prepass@i1 = (xray_tessellation_prepass);
    prepass_triangle_size@f1 = (xray_tessellation_prepass_triangle_size);
    affector_strength_threshold@f1 = (xray_affector_strength_threshold);
    show_debug_destruction_groups@i1 = (xray_show_debug_destruction_groups);
  }

  (ps) {
    colliders_count_f@f1 = (min(colliders_count+0.1, 30));
    xray_simulation_buffer@buf = xray_simulation_buffer hlsl {
      StructuredBuffer<float4> xray_simulation_buffer@buf;
    };
  }

  hlsl {
    #define BUF_STRIDE 3

    #define KINETIC     0
    #define EXPLOSION   1
    #define HEAT        2
    #define SUBCALIBER  3
  }

  hlsl {
    float3 applyCollider(
      inout float accumulatedStrain, float3 world_pos, float3 in_pos,
      float3 col_pos, float3 col_dir, float r1, float r2, float3 normal, int type,
      float explosion_scale)
    {
      float3 vec = world_pos - col_pos;
      float dist = length(vec);

      float rad1 = dot(vec, col_dir);
      float rad2 = sqrt(max(0, pow2(dist) - pow2(rad1)));

      r2 = max(r2, 0.0001);
      r1 = max(r1, 0.0001);

      float coherency = dot(normalize(vec), normal);

      float3 ret = 0;

      BRANCH
      if (type == EXPLOSION)
      {
        float strength2 = saturate(1 - dist/r2);
        float3 expDir1 = 0.15*col_dir;
        float3 expDir2 = 0.3*normalize(vec);
        BRANCH
        if (strength2 > 0.00001)
        {
          ret = pow4(strength2) * 0.4 * expDir1 +
                pow3(strength2) * 0.6 * expDir2 +
                pow4(strength2) * 0.05 * saturate(0.5-abs(coherency)) * normal * sign(dot(normal, float3(1, 1, 1))) *
                sin(11*world_pos.x - 8*world_pos.z+ 8*world_pos.y + 21*dist) *
                pow3(sin(9*world_pos.z - 12*world_pos.x - 5*world_pos.y + 23*dist));
        }
      }
      else
      {
        float3 lengthMul = type != HEAT ? float3(0.07, 2.9, 5.5) : float3(0.2, 1.9, 3.3);
        r2 *= 1 + lengthMul.x * (0.5*sin((rad1 + r1)*lengthMul.y) + 0.5*sin((rad1 + r1)*lengthMul.z));
        float strength1 = (-rad1 < r1) ? 1 : 0;
        float strength2 = saturate(1 - rad2/r2);
        float strength = min(saturate((r2-rad1)/r2), 1-saturate((-rad1-(r1-r2))/r2));
        BRANCH
        if (strength1*strength2*strength > 0.00001)
        {
          float2 randomScale = float2(0.1, 0.02) * (type != HEAT ? float2(frac(in_pos.x*22.1234), frac(in_pos.y*21.1234)) :
                                                                      2*float2(frac(in_pos.x + rad2*62.1234), 4*frac(in_pos.y + rad2*41.1234)));

          float2 holeScale = (type == KINETIC ? float2(0.4, 0.6) : (type == SUBCALIBER ? float2(0.5, 1.2) :float2(0.1, 0.2)));
          float3 expDir1 = normalize(vec - rad1*col_dir);
          float3 expDir2 = col_dir;

          ret = pow2(pow2(strength2)) * strength1 * (holeScale.x + randomScale.x) * expDir1 +
                     pow3(strength2)  * strength1 * (holeScale.y + randomScale.y) * expDir2;
          ret *= strength;
        }
      }

      accumulatedStrain += length(ret*2) * (type != EXPLOSION ? 1 : explosion_scale);

      return r2 * ret * (type != EXPLOSION ? 1 : explosion_scale * 0.8);
    }
  }

  hlsl(vs)
  {
    float getCollidersInfluence(float3 worldPos)
    {
      float ret = 0;
      for (int j = 0; j < int(colliders_count_f); j++)
      {
        float4 colPos_r1 = structuredBufferAt(xray_simulation_buffer, j*BUF_STRIDE + 0);
        float4 colDir_r2 = structuredBufferAt(xray_simulation_buffer, j*BUF_STRIDE + 1);
        float4 type_time = structuredBufferAt(xray_simulation_buffer, j*BUF_STRIDE + 2);
        int type = type_time.x + 0.1f;

        colDir_r2.w = type == 0 ? 0.6 : colDir_r2.w*1.5;
        colDir_r2.w = max(colDir_r2.w, 0.0001);
        colPos_r1.w = max(colPos_r1.w, 0.0001);

        float3 vec = worldPos - colPos_r1.xyz;
        float dist = length(vec);

        float rad1 = dot(vec, colDir_r2.xyz);
        float rad2 = sqrt(max(0, pow2(dist) - pow2(rad1)));

        float strength1 = type != EXPLOSION ? saturate(1 - abs(rad1)/colPos_r1.w)*saturate(-2*rad1) : 1;
        float strength2 = type != EXPLOSION ? saturate(1 - rad2/colDir_r2.w) : saturate(1 - dist/colDir_r2.w);
        float strength = strength1*strength2 * (type != EXPLOSION ? 100 : 0.5);
        ret = max(ret, saturate(strength));
      }
      return ret;
    }

    float getBroadCollidersInfluence(float3 worldPos)
    {
      float influence = 0;
      for (int j = 0; j < int(colliders_count_f); j++)
      {
        float4 colPos_r1 = structuredBufferAt(xray_simulation_buffer, j*BUF_STRIDE + 0);
        float4 colDir_r2 = structuredBufferAt(xray_simulation_buffer, j*BUF_STRIDE + 1);
        float4 type_time = structuredBufferAt(xray_simulation_buffer, j*BUF_STRIDE + 2);
        int type = type_time.x + 0.1f;

        colDir_r2.w = type == 0 ? 0.6 : colDir_r2.w*1.5;
        colDir_r2.w = max(colDir_r2.w, 0.0001);
        colPos_r1.w = max(colPos_r1.w, 0.0001);

        float3 vec = worldPos - colPos_r1.xyz;
        float dist = length(vec);

        float rad1 = dot(vec, colDir_r2.xyz);
        float rad2 = sqrt(max(0, pow2(dist) - pow2(rad1)));

        if (type == EXPLOSION)
          influence = max(influence, saturate(0.5 - dist / (colDir_r2.w)));
        // Multiplying denominator because APFSDS's are too thin
        else if (type == SUBCALIBER)
          influence = max(influence, saturate(1 - rad2 / (colDir_r2.w * 4.f)));
        else
          influence = max(influence, saturate(1 - rad2 / (colDir_r2.w)));

      }
      return influence;
    }

    float3 getStrain(float3 worldPos, float explosion_scale)
    {
      float3 normal = float3(0, 1, 0);
      float3 affector = 0;
      float accumulatedStrain = 0;
      LOOP
      for (int j = 0; j < int(colliders_count_f); j++)
      {
        float4 colPos_r1 = structuredBufferAt(xray_simulation_buffer, j*BUF_STRIDE + 0);
        float4 colDir_r2 = structuredBufferAt(xray_simulation_buffer, j*BUF_STRIDE + 1);
        float4 type_time = structuredBufferAt(xray_simulation_buffer, j*BUF_STRIDE + 2);
        BRANCH
        if (type_time.x == EXPLOSION)
          affector += 0.3*applyCollider(accumulatedStrain, worldPos, worldPos, colPos_r1.xyz, colDir_r2.xyz, colPos_r1.w, colDir_r2.w, normal, type_time.x, explosion_scale);
      }
      return affector;
    }

    float getPrepassStrainFromCollider(int colliderNo, float3 worldPos, float3 normal, float explosion_scale)
    {
      float3 affector = 0;
      float accumulatedStrain = 0;
      float4 colPos_r1 = structuredBufferAt(xray_simulation_buffer, colliderNo*BUF_STRIDE + 0);
      float4 colDir_r2 = structuredBufferAt(xray_simulation_buffer, colliderNo*BUF_STRIDE + 1);
      float4 type_time = structuredBufferAt(xray_simulation_buffer, colliderNo*BUF_STRIDE + 2);
      return length(applyCollider(accumulatedStrain, worldPos, worldPos, colPos_r1.xyz, colDir_r2.xyz, colPos_r1.w, colDir_r2.w, normal, type_time.x, explosion_scale));
    }

    void prepareXrayVSOutput(in VsOutput output, XrayPartParams part_params, out float4 deformation, out float3 pToEye, out float4 pos, uint vertex_id)
    {
      float3 worldPos = world_view_pos - output.pointToEye.xyz;
      deformation = 0;
      deformation.x = part_params.is_simulated >= 0 ? 1 : 0;
      pToEye = output.pointToEye;
      ##if xray_destruction == full_sim
        ##if xray_generate_tessellated_geometry == on && shader == hatching_simple
          pos = float4(worldPos, 1);
          BRANCH
          if (xray_tessellation_prepass == 1)
          {
            deformation.x = max(getCollidersInfluence(worldPos), part_params.is_simulated);
          }
          else
          {
            deformation.x = getBroadCollidersInfluence(worldPos);
          }

          deformation.y = part_params.explosion_scale;
        ##else
          uint triangleId = vertex_id / 3;
          uint triangleVertId = vertex_id % 3;

          // 3 vertices are read from the buffer as we need all the triangle data for destruction calculations
          float3 worldPositions[3];
          for (uint i = 0; i < 3; i++)
          {
            worldPositions[i] = xray_tessellated_vertex_buffer[3 * triangleId + i].xyz;
          }
          float3 centerPos = (worldPositions[0] + worldPositions[1] + worldPositions[2]) / 3;

          uint groupNo = 0;
          uint vertexCount = 0;
          uint numGroups = (1U << uint(colliders_count_f));
          for (uint j = 0; j < numGroups; j++)
          {
            vertexCount += xray_vertex_group_counts[j];
            if (vertex_id < vertexCount)
            {
              groupNo = j;
              break;
            }
          }

          pos = float4(worldPositions[triangleVertId], 1.0);
          BRANCH
          if (show_debug_destruction_groups == 1)
          {
            pos = mulPointTm(worldPositions[triangleVertId], globtm);
            deformation.x = float(groupNo) / (numGroups - 1);
            deformation.y = part_params.explosion_scale;
            deformation.z = xray_vertex_group_counts[2 * numGroups];
            deformation.w = groupNo;
            return;
          }

          float3 affector[3] = {float3(0, 0, 0), float3(0, 0, 0), float3(0, 0, 0)};
          float accumulatedStrain[3] = {0, 0, 0};
          float cracker = 1;
          float crackStrain = 0;

          float3 pointToEye0 = world_view_pos - worldPositions[0];
          float3 pointToEye1 = world_view_pos - worldPositions[1];
          float3 pointToEye2 = world_view_pos - worldPositions[2];
          float3 normal = normalize(cross (pointToEye1 - pointToEye0, pointToEye2 - pointToEye0));

          BRANCH
          if (part_params.is_simulated >= 0)
          {
            LOOP
            for (int j = 0; j < int(colliders_count_f); j++)
            {
              if (!(groupNo & (1U << j)))
                continue;

              float4 colPos_r1 = structuredBufferAt(xray_simulation_buffer, j*BUF_STRIDE + 0);
              float4 colDir_r2 = structuredBufferAt(xray_simulation_buffer, j*BUF_STRIDE + 1);
              float4 type_time = structuredBufferAt(xray_simulation_buffer, j*BUF_STRIDE + 2);
              LOOP
              for (int i = 0; i < 3; ++i)
              {
                affector[i] += applyCollider(accumulatedStrain[i], worldPositions[i], worldPositions[i], colPos_r1.xyz, colDir_r2.xyz, colPos_r1.w, colDir_r2.w, normal, type_time.x,
                  part_params.explosion_scale);
              }

              float3 colPos = colPos_r1.xyz;
              float3 colDir = colDir_r2.xyz;

              colDir_r2.w = max(colDir_r2.w, 0.0001);

              float3 clocker = float3(0, 1, 0);
              clocker = normalize(clocker - colDir*dot(clocker, colDir));

              float3 centerVec = centerPos - colPos;
              centerVec = normalize(centerVec - colDir*dot(centerVec, colDir));

              float dist = length(centerPos - colPos);

              float rad1 = dot(centerPos - colPos, colDir_r2.xyz);
              float rad2 = sqrt(max(0, pow2(dist) - pow2(rad1)));
              float strength = int(type_time.x) != EXPLOSION ? saturate(1 - 2*rad2/colDir_r2.w) : saturate(1 - dist/colDir_r2.w);
              strength *= int(type_time.x) == SUBCALIBER ? 5 : 1;

              float crackParam = 1.5+1.5*dot(clocker, centerVec);
              float crackFloor = floor(crackParam);

              float crackStrain2 = saturate(3*2*abs(crackParam - (crackFloor + 0.5)) - 2.5);

              float cracker2 = 0.8+0.4*(crackFloor/3.0f);

              float crackLerp = saturate(0.1*floor(strength*10.2));

              cracker *= lerp(1, cracker2, crackLerp);
              crackStrain += crackStrain2*crackLerp;
            }
          }

          UNROLL
          for (int k = 0; k < 3; k++)
          {
            affector[k] *= cracker;
          }

          float minSize = min(min(dot(worldPositions[0]-worldPositions[1], worldPositions[0]-worldPositions[1]),
                                  dot(worldPositions[0]-worldPositions[2], worldPositions[0]-worldPositions[2])),
                                  dot(worldPositions[1]-worldPositions[2], worldPositions[1]-worldPositions[2]));

          float maxAffect = max(max(dot(affector[0]-affector[1], affector[0]-affector[1]),
                                    dot(affector[0]-affector[2], affector[0]-affector[2])),
                                    dot(affector[1]-affector[2], affector[1]-affector[2]));

          float3 totalAffect = affector[0] + affector[1] + affector[2];

          affector[triangleVertId] = lerp(affector[triangleVertId], totalAffect/2.0f, saturate((maxAffect-2*minSize)*30));
          pos = mulPointTm(worldPositions[triangleVertId] + affector[triangleVertId], globtm);
          pToEye = world_view_pos - (worldPositions[triangleVertId] + affector[triangleVertId]);
          deformation.x = (crackStrain + 1) * accumulatedStrain[triangleVertId];
          deformation.y = part_params.explosion_scale;
        ##endif
      ##else
        float3 affector = getStrain(worldPos, part_params.explosion_scale);
        pToEye = world_view_pos - (worldPos + affector);
        pos = mulPointTm(worldPos + affector, globtm);
      ##endif
    }

    // Find group number for the vertex based on what destruction colliders affect it, using a bitmask
    // Example:
    // Vertex affected by colliders 0 and 2 -> bitmask 0101 -> group number is 5
    // If vertex is unaffected the group number is 0
    uint findEffectGroupNo(float3 worldPos, float3 normal, float explosionScale)
    {
      uint groupNo = 0;
      for (int j = 0; j < int(colliders_count_f); j++)
      {
            float strain = getPrepassStrainFromCollider(j, worldPos, normal, explosionScale);
            if (strain > affector_strength_threshold)
              groupNo |= (1U << j);
      }
      return groupNo;
    }
  }

  hlsl(ps)
  {
    float getShellOpacity(float3 worldPos)
    {
      float ret = 0;
      for (int j = 0; j < int(colliders_count_f); j++)
      {
        float4 colPos_r1 = structuredBufferAt(xray_simulation_buffer, j*BUF_STRIDE + 0);
        float4 colDir_r2 = structuredBufferAt(xray_simulation_buffer, j*BUF_STRIDE + 1);
        float4 type_time = structuredBufferAt(xray_simulation_buffer, j*BUF_STRIDE + 2);
        int type = type_time.x + 0.1f;

        if (type == EXPLOSION)
          continue;

        colDir_r2.w = colDir_r2.w * (type == HEAT ? 0.1 : 0.3);
        colDir_r2.w = max(0.0001, colDir_r2.w);

        float3 vec = worldPos - colPos_r1.xyz;
        float dist = length(vec);

        float rad1 = dot(vec, colDir_r2.xyz);
        float rad2 = sqrt(max(0, pow2(dist) - pow2(rad1)));

        float strength1 = (-rad1 < colPos_r1.w ? 1 : 0)*saturate((colDir_r2.w-rad1)/colDir_r2.w);
        float strength2 = saturate(1 - rad2/colDir_r2.w);
        float strength = strength1*strength2;
        ret = max(ret, saturate(strength));
      }
      return ret;
    }
  }
endmacro

macro USE_XRAY_DESTRUCTION()

  hlsl {
    #define FIELD_BARYCENTRIC_VALUE(field_name) \
      output.field_name = (input[0].field_name * w + input[1].field_name * u + input[2].field_name * v)

    #define PROXY_FIELD_VALUE(field_name) \
      output.field_name = input.field_name

    struct HsPatchData
    {
      float edges[3] : SV_TessFactor;
      float inside   : SV_InsideTessFactor;
    };

    #define BARYCENTRIC_COORDS_UNPACK(uvw) \
      float u = uvw.x; \
      float v = uvw.y; \
      float w = uvw.z;
  }

  hlsl {
    struct HsControlPoint
    {
      float3 pos : POSITION0;
      int id: TEXCOORD0;
      float3 pointToEye : TEXCOORD1;
      ##if shader == hatching_vcolor
      float4 color : TEXCOORD2;
      ##endif
      ##if shader == hatching || shader == hatching_vcolor || shader == hatching_sphere
      float3 worldNormal : TEXCOORD4;
      ##endif
      float4 deformation : TEXCOORD6;
      float4 screenPos : TEXCOORD3;
      ##if xray_generate_tessellated_geometry == on && shader == hatching_simple
      float3 localPos : TEXCOORD2;
      ##endif
    };
  }

  hlsl(hs) {
    void proxy_struct_fields(VsOutput input, inout HsControlPoint output)
    {
      PROXY_FIELD_VALUE(pointToEye);
      ##if shader == hatching_vcolor
        PROXY_FIELD_VALUE(color);
      ##endif
      ##if shader == hatching || shader == hatching_vcolor || shader == hatching_sphere
        PROXY_FIELD_VALUE(worldNormal);
      ##endif
      PROXY_FIELD_VALUE(deformation);
      PROXY_FIELD_VALUE(screenPos);
      PROXY_FIELD_VALUE(id);
      ##if xray_generate_tessellated_geometry == on && shader == hatching_simple
      PROXY_FIELD_VALUE(localPos);
      ##endif
    }
  }

  hlsl(ds) {
    void fields_barycentric_values(const OutputPatch<HsControlPoint, 3> input, inout VsOutput out_struct, float3 uvw : SV_DomainLocation)
    {
      VsOutput output;
      output.pos = out_struct.pos;
      BARYCENTRIC_COORDS_UNPACK(uvw)
      FIELD_BARYCENTRIC_VALUE(pointToEye);
      out_struct.pointToEye = output.pointToEye;
      ##if shader == hatching_vcolor
        FIELD_BARYCENTRIC_VALUE(color);
        out_struct.color = output.color;
      ##endif
      ##if shader == hatching || shader == hatching_vcolor || shader == hatching_sphere
        FIELD_BARYCENTRIC_VALUE(worldNormal);
        out_struct.worldNormal = output.worldNormal;
      ##endif
      FIELD_BARYCENTRIC_VALUE(deformation);
      out_struct.deformation = output.deformation;
      FIELD_BARYCENTRIC_VALUE(screenPos);
      out_struct.screenPos = output.screenPos;
      out_struct.id = input[0].id;
      ##if xray_generate_tessellated_geometry == on && shader == hatching_simple
      FIELD_BARYCENTRIC_VALUE(localPos);
      out_struct.localPos = output.localPos;
      ##endif
    }
  }

  hlsl(hs) {
    //helper functions
    float edgeLod(float3 pos1, float3 pos2, float deformation1, float deformation2)
    {
      BRANCH
      if (xray_tessellation_prepass == 1) // Tessellate everything down to predetermined size
        return length(pos1 - pos2) * 1.0 / prepass_triangle_size;
      else                                // Tessellate more where there is deformation
        return lerp(0, 10, saturate(max(deformation1, deformation2))) * hcam_deformation_quality;
    }

    void HullShaderCalcTessFactor(inout HsPatchData patch,
      OutputPatch<HsControlPoint, 3> controlPoints, uint tid : SV_InstanceID)
    {
      int next = (1U << tid) & 3U; // (tid + 1) % 3
      next = (1U << next) & 3U;

      patch.edges[tid] = max(1, edgeLod(controlPoints[tid].pointToEye, controlPoints[next].pointToEye,
                                        controlPoints[tid].deformation.x, controlPoints[next].deformation.x));

      if (xray_tessellation_prepass != 1)
        patch.edges[tid] = min(patch.edges[tid], 8);
    }

    //patch constant data
    HsPatchData HullShaderPatchConstant(OutputPatch<HsControlPoint, 3> controlPoints)
    {
      HsPatchData patch = (HsPatchData)0;
      //calculate Tessellation factors
      HullShaderCalcTessFactor(patch, controlPoints, 0);
      HullShaderCalcTessFactor(patch, controlPoints, 1);
      HullShaderCalcTessFactor(patch, controlPoints, 2);
      patch.inside = (patch.edges[0] + patch.edges[1] + patch.edges[2]) / 3.0;
      return patch;
    }

    [domain("tri")]
    [outputtopology("triangle_cw")]
    [outputcontrolpoints(3)]
    [partitioning("integer")]
    [patchconstantfunc("HullShaderPatchConstant")]
    HsControlPoint triangulation_hs(InputPatch<VsOutput, 3> inputPatch, uint tid : SV_OutputControlPointID)
    {
      HsControlPoint output;
      output.pos = inputPatch[tid].pos.xyz;
      proxy_struct_fields(inputPatch[tid], output);
      return output;
    }
  }
  compile("hs_5_0", "triangulation_hs");

  hlsl(ds) {
    [domain("tri")]
    VsOutput triangulation_ds(HsPatchData patchData, const OutputPatch<HsControlPoint, 3> input, float3 uvw : SV_DomainLocation)
    {
      VsOutput output;
      float u = uvw.x;
      float v = uvw.y;
      float w = uvw.z;

      float3 pos = input[0].pos * u + input[1].pos * v + input[2].pos * w;
      output.pos = float4(pos, 1);
      fields_barycentric_values(input, output, uvw);
      return output;
    }
  }
  compile("ds_5_0", "triangulation_ds");

  if (shader != hatching && shader != hatching_vcolor)
  {
  hlsl(gs) {
    RWStructuredBuffer<uint> xray_group_vertex_counts : register(u3);
    RWStructuredBuffer<float4> xray_tess_temp_vb : register(u4);
    RWStructuredBuffer<uint> indirect_buffer_vertex_counts : register(u5);
    RWStructuredBuffer<float4> xray_tess_vb : register(u6);

    // Precalculate the final destruction effect group sizes that will be used when splitting geometry into groups
    void fillAffectGroupSizes(float3 worldPos, float3 normal, float explosionScale)
    {
      uint groupNo = findEffectGroupNo(worldPos, normal, explosionScale);
      uint old = 0;
      InterlockedAdd(xray_group_vertex_counts[groupNo], 3, old);
    }

    // Fill in the vertices according to their effect groups
    void fillGroupsWithGeometry(VsOutput input[3], float3 normal)
    {
      float3 worldCenter = ((input[0].pos + input[1].pos + input[2].pos) / 3).xyz;
      uint groupNo = findEffectGroupNo(worldCenter, normal, input[0].deformation.y);

      uint groupStart = 0;
      for (uint i = 0; i < groupNo; i++)
      {
        groupStart += xray_group_vertex_counts[i];
      }

      uint numColliders = uint(colliders_count_f);
      if (show_debug_destruction_groups)
      {
        uint old_total = 0;
        InterlockedAdd(xray_group_vertex_counts[2 * (1U << numColliders)], 3, old_total);
      }

      uint old = 0;
      InterlockedAdd(xray_group_vertex_counts[(1U << numColliders) + groupNo], 3, old);
      for (int l = 0; l < 3; l++)
      {
        xray_tess_vb[groupStart + old + l] = float4(input[l].pos.xyz, 1.0);
      }
    }

    #define DRAW_INDIRECT_NUM_ARGS 4

    #define OUTLINE_POLYCOUNT 1
    [maxvertexcount(OUTLINE_POLYCOUNT*3)]
    void outline_destruction_gs(triangle VsOutput input[3], inout TriangleStream<VsOutput> triStream) // SV_InstanceID
    {
      int indirect_offset = xray_part_no;
      uint at = 0;
      BRANCH
      if (xray_tessellation_prepass == 1)
      {
        InterlockedAdd(indirect_buffer_vertex_counts[DRAW_INDIRECT_NUM_ARGS * indirect_offset], 3, at);
        BRANCH
        if (at == 0)
        {
          indirect_buffer_vertex_counts[DRAW_INDIRECT_NUM_ARGS * indirect_offset + 1] = 1;
          indirect_buffer_vertex_counts[DRAW_INDIRECT_NUM_ARGS * indirect_offset + 2] = 0;
          indirect_buffer_vertex_counts[DRAW_INDIRECT_NUM_ARGS * indirect_offset + 3] = 0;
        }
      }
      else
      {
        InterlockedAdd(indirect_buffer_vertex_counts[xray_part_no], 3, at);
      }

      float3 worldCenter = ((input[0].pos + input[1].pos + input[2].pos) / 3).xyz;
      float3 pointToEye0 = world_view_pos - input[0].pos.xyz;
      float3 pointToEye1 = world_view_pos - input[1].pos.xyz;
      float3 pointToEye2 = world_view_pos - input[2].pos.xyz;
      float3 normal = normalize(cross(pointToEye1 - pointToEye0, pointToEye2 - pointToEye0));

      BRANCH
      if (xray_tessellation_prepass == 0) // Fill in vertex group sizes
      {
        fillAffectGroupSizes(worldCenter, normal, input[0].deformation.y);
      }
      else if (xray_tessellation_prepass == 1) // Write local positions to temp buffer that will be used in second pass
      {
        for (int l = 0; l < 3; l++)
        {
          xray_tess_temp_vb[at + l] = float4(input[l].localPos, 1.0);
        }
      }
      else if (xray_tessellation_prepass == 2) // Write final tessellated geometry to buffers that will be used in render
      {
        fillGroupsWithGeometry(input, normal);
      }

      UNROLL
      for (int i = 0; i < 3; i++)
      {
        triStream.Append(input[i]);
      }
      triStream.RestartStrip();
    }
  }
  compile("target_gs", "outline_destruction_gs");
  }
endmacro

macro XRAY_DESTRUCTION_PS()
  USE_XRAY_RENDER()
  USE_XRAY_RENDER_DISSOLVE()
  hlsl(ps) {
    half4 hatching_ps(VsOutput input) : SV_Target
    {
      float3 worldNormal = get_world_normal(input);
      // PARAMS_ID by some reason can go beyound ranges, clamp as workarround
      XrayPartParams part_params = partParams[clamp(PARAMS_ID, 0, MAX_XRAY_PARTS-1)];

      float4 dissolveColor;
      if (apply_xray_dissolve(input.screenPos, part_params, dissolveColor))
        return dissolveColor;

      ##if xray_destruction == ps_only
        float3 normal = normalize(float3(1, 1, 1));
        float3 worldPos = world_view_pos.xyz - input.pointToEye.xyz;
        worldPos = lerp(worldPos,
                        0.02 * floor(50 * worldPos) + 0.02 * smoothstep(0, 1, frac(50 * worldPos)),
                        0.5);
        float3 affector = float3(0, 0, 0);
        float accumulatedStrain = 0;
        BRANCH
        if (input.deformation.x > 0)
        {
          LOOP
          for (int j = 0; j < int(colliders_count_f); j++)
          {
            float4 colPos_r1 = structuredBufferAt(xray_simulation_buffer, j*BUF_STRIDE + 0);
            float4 colDir_r2 = structuredBufferAt(xray_simulation_buffer, j*BUF_STRIDE + 1);
            float4 type_time = structuredBufferAt(xray_simulation_buffer, j*BUF_STRIDE + 2);
            affector += 0.2 * applyCollider(accumulatedStrain, worldPos, worldPos, colPos_r1.xyz, colDir_r2.xyz, colPos_r1.w, colDir_r2.w, normal, type_time.x,
              part_params.explosion_scale);
          }
          input.deformation.x = accumulatedStrain;
        }

        normal = normalize(cross(normalize(ddx(input.pointToEye + affector)), normalize(ddy(input.pointToEye + affector))));
        worldNormal = normalize(lerp(worldNormal, normal, 0.7 * saturate(accumulatedStrain * 5)));
      ##endif

      BRANCH
      if (input.deformation.x > 0 || part_params.is_simulated > 0)
        if (getShellOpacity(world_view_pos.xyz - input.pointToEye) >= 0.1)
          discard;


      // Armor parts
      ##if is_xray_normal_map_mode == is_xray_normal_map_mode_on
        return half4(dot(worldNormal, normalize(float3(1, 1, 1))) * 0.5 + 0.5, input.deformation.x, 1.0, 1.0);

      // Modules
      ##else
        float4 baseColor = xray_lighting(input.pointToEye, worldNormal, part_params);
        float3 deformationColor = get_deformation_color(input.deformation.x);

        float maxHatching = max(max(part_params.hatching_fresnel.r, part_params.hatching_fresnel.b),
                                part_params.hatching_fresnel.g);

        float3 color = lerp(saturate(deformationColor * 2),
                            lerp(part_params.hatching_color.rgb, part_params.hatching_fresnel.rgb, 1 - saturate(maxHatching * 10)),
                            deformationColor.z - lerp(0.3, 0.1, 1 - saturate(maxHatching * 10)));

        float lighting =  0.8 + 0.2 * dot(worldNormal, normalize(float3(1, 1, 1)));
        float4 finalDeformationColor = float4(color * lighting, part_params.hatching_color.a);
        return lerp(finalDeformationColor, baseColor, deformationColor.z);
      ##endif
    }
  }
  compile("target_ps", "hatching_ps");
endmacro