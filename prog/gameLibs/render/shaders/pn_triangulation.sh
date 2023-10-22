// How to use:
// 1. Include INIT_PN_TRIANGULATION(smoothing) a the beginning of the shader. Use PN_TESSELATION to make tesselation code paths.
// 2. Vertex shader needs to put out a HsInput structure. (Mandatory params: pos, normal in WORLD coordinates)
// 3. Between hull and domain shader a user defined HsControlPoint structure is passed. (Mandatory params: HS_CONTROL_POINTS_DECLARATION)
// 4. The shader body in this file will calculate pos and normal, that'll be available in fields_barycentric_values/output
// 5. Duplicate values in hull shader with PROXY_FIELD_VALUE in "void proxy_struct_fields(HsInput input, inout HsControlPoint output)"
// 6. In domain shader interpolate values in "void fields_barycentric_values(const OutputPatch<HsControlPoint, 3> input, inout VsOutput output, float3 uvw)"
//    First call BARYCENTRIC_COORDS_UNPACK(uvw) then use FIELD_BARYCENTRIC_VALUE(field_name)
// 7. Additional parameters can be calculated freely in proxy_struct_fields and fields_barycentric_values (like pointToEye after tesselation)
// 8. After all of this, include USE_PN_TRIANGULATION()

int pn_triangulation = 0;
interval pn_triangulation : pn_triangulation_off < 1, pn_triangulation_on;

macro INIT_PN_TRIANGULATION(smoothing)
hlsl {
  //Workarond for hlslcc bug
  ##if hardware.vulkan
    #pragma spir-v compiler dxc
  ##endif

  #define PN_TESSELATION 1
  ##if smoothing
    #define PN_TRIANGULATION_SMOOTHING 1
  ##endif

  #define GET_INTERPOLATED_INPUT_VALUE(field_name) \
    (input[0].field_name * w + input[1].field_name * u + input[2].field_name * v)

  #define FIELD_BARYCENTRIC_VALUE(field_name) \
    output.field_name = GET_INTERPOLATED_INPUT_VALUE(field_name)

  #define PROXY_FIELD_VALUE(field_name) \
    output.field_name = input.field_name

  struct HsPatchData
  {
    float edges[3] : SV_TessFactor;
    float inside   : SV_InsideTessFactor;
    float3 center  : CENTER;
  };

  #define HS_CONTROL_POINTS_DECLARATION \
    float3 pos1    : POSITION0; \
    float3 pos2    : POSITION1; \
    float3 pos3    : POSITION2; \
    float3 normal1 : NORMAL0;   \
    float3 normal2 : NORMAL1;

  #define BARYCENTRIC_COORDS_UNPACK(uvw) \
    float u = uvw.x; \
    float v = uvw.y; \
    float w = uvw.z;

  #ifndef GET_DRAW_CALL_ID_TRIANGULATION
    #define GET_DRAW_CALL_ID_TRIANGULATION GET_DRAW_CALL_ID
  #endif
}
endmacro

macro USE_PN_TRIANGULATION()
hlsl(hs) {
  //patch constant data
  HsPatchData HullShaderPatchConstant(OutputPatch<HsControlPoint, 3> controlPoints)
  {
    #if SET_UP_MULTIDRAW
      DRAW_CALL_ID = GET_DRAW_CALL_ID_TRIANGULATION;
    #endif
    HsPatchData patch = (HsPatchData)0;
    //calculate Tessellation factors
    patch.edges[0] = calc_tess_factor(patch, controlPoints, 0);
    patch.edges[1] = calc_tess_factor(patch, controlPoints, 1);
    patch.edges[2] = calc_tess_factor(patch, controlPoints, 2);
    patch.inside = max(max(patch.edges[0], patch.edges[1]), patch.edges[2]);
    //calculate center
    float3 center = (controlPoints[0].pos2 + controlPoints[0].pos3) * 0.5 - controlPoints[0].pos1 +
      (controlPoints[1].pos2 + controlPoints[1].pos3) * 0.5 - controlPoints[1].pos1 +
      (controlPoints[2].pos2 + controlPoints[2].pos3) * 0.5 - controlPoints[2].pos1;
    patch.center = center;
    return patch;
  }

  [domain("tri")]
  [outputtopology("triangle_cw")]
  [outputcontrolpoints(3)]
  [partitioning("fractional_odd")]
  [patchconstantfunc("HullShaderPatchConstant")]
  HsControlPoint pn_triangulation_hs(InputPatch<HsInput, 3> inputPatch, uint tid : SV_OutputControlPointID)
  {
    #if SET_UP_MULTIDRAW
      DRAW_CALL_ID = GET_DRAW_CALL_ID_TRIANGULATION;
    #endif
    int next = (1U << tid) & 3; // (tid + 1) % 3
    float3 p1 = inputPatch[tid].pos.xyz;
    float3 p2 = inputPatch[next].pos.xyz;
    float3 n1 = inputPatch[tid].normal.xyz;

    float3 n2 = inputPatch[next].normal.xyz;
    HsControlPoint output;
    //control points positions
    const float FIXED_BIAS = 1e-3f;
    output.pos1 = p1;
#if PN_TRIANGULATION_SMOOTHING
    float pos2Bias = max(abs(dot(p2 - p1, n1)), FIXED_BIAS);
    float pos3Bias = max(abs(dot(p1 - p2, n2)), FIXED_BIAS);
#else
    float pos2Bias = FIXED_BIAS;
    float pos3Bias = FIXED_BIAS;
#endif
    output.pos2 = 2 * p1 + p2 + pos2Bias * n1;
    output.pos3 = 2 * p2 + p1 + pos3Bias * n2;
    //control points normals
    float3 v12 = 4 * dot(p2 - p1, n1 + n2) / dot(p2 - p1, p2 - p1);
    output.normal1 = n1;
    output.normal2 = n1 + n2 - v12 * (p2 - p1);
    proxy_struct_fields(inputPatch[tid], output);
    return output;
  }
}
compile("hs_5_0", "pn_triangulation_hs");

hlsl(ds) {
  [domain("tri")]
  VsOutput pn_triangulation_ds(HsPatchData patchData, const OutputPatch<HsControlPoint, 3> input, float3 uvw : SV_DomainLocation)
  {
    #if SET_UP_MULTIDRAW
      DRAW_CALL_ID = GET_DRAW_CALL_ID_TRIANGULATION;
    #endif
    VsOutput output;
    float u = uvw.x;
    float v = uvw.y;
    float w = uvw.z;
    //output position is weighted combination of all 10 position control points
    float3 pos =
      input[0].pos1 * w * w * w + input[1].pos1 * u * u * u + input[2].pos1 * v * v * v +
      input[0].pos2 * w * w * u + input[0].pos3 * w * u * u + input[1].pos2 * u * u * v +
      input[1].pos3 * u * v * v + input[2].pos2 * v * v * w + input[2].pos3 * v * w * w +
      patchData.center * u * v * w;
    //output normal is weighted combination of all 6 position control points
    float3 nor =
      input[0].normal1 * w * w + input[1].normal1 * u * u + input[2].normal1 * v * v +
      input[0].normal2 * w * u + input[1].normal2 * u * v + input[2].normal2 * v * w;

    //transform and output data
    output.pos = float4(pos, 1);
    output.normal = 0;
    output.normal.xyz = normalize(nor);
    fields_barycentric_values(input, output, uvw);
    output.pos = mulPointTm(output.pos.xyz, globtm);
    return output;
  }
}
compile("ds_5_0", "pn_triangulation_ds");
endmacro