//CLAMP_WORLD_POS = 1 is for voxelizing to 2.5d cascade
//CLAMP_WORLD_POS = 0 is for voxelizing to 3d cascade
//WRITE_WORLD_NORMAL = 1 is incoming normal is not present in vertex
//WRITE_WORLD_NORMAL = 1, CLAMP_WORLD_POS= 0 - collision to 3Dcascade
//WRITE_WORLD_NORMAL = 1, CLAMP_WORLD_POS= 1 - collision to 2.5Dcascade
//WRITE_WORLD_NORMAL = 0, CLAMP_WORLD_POS= 0 - 3D RI/real geom
//WRITE_WORLD_NORMAL = 0, CLAMP_WORLD_POS= 1 - 2.5D RI/real geom
[maxvertexcount(3)]
void voxelize_gs(triangle VsOutput input[3], inout TriangleStream<GsOutput> triStream)
{
  GsOutput p[3] = {
    (GsOutput)0,
    (GsOutput)0,
    (GsOutput)0
  };
  p[0].v=input[0];p[1].v=input[1];p[2].v=input[2];

  float3 normal = normalize(cross(input[2].pointToEye.xyz - input[0].pointToEye.xyz, input[1].pointToEye.xyz - input[0].pointToEye.xyz));
  //todo: render in space of triangle, to maximize it's area and simplify code
  //todo: conservative?
  float3 absNormal = abs(normal);
  uint angle = 0;
  if (absNormal.y > absNormal.x && absNormal.y > absNormal.z)
    angle = 1;
  else if (absNormal.z > absNormal.x && absNormal.z > absNormal.y)
    angle = 2;
  float3 box0 = world_view_pos-voxelize_box0;
  #if CLAMP_WORLD_POS
  box0.y = max3(input[0].pointToEye.y, input[1].pointToEye.y, input[2].pointToEye.y);
  #endif
  float3 boxPos0 = (box0-input[0].pointToEye.xyz)*voxelize_box1.xyz, boxPos1 = (box0-input[1].pointToEye.xyz)*voxelize_box1.xyz, boxPos2 = (box0-input[2].pointToEye.xyz)*voxelize_box1.xyz;
  boxPos0 = boxPos0 * 2-1;
  boxPos1 = boxPos1 * 2-1;
  boxPos2 = boxPos2 * 2-1;
  #if WRITE_WORLD_NORMAL
  p[0].v.worldNormal = p[1].v.worldNormal = p[2].v.worldNormal = normal;
  #endif
  p[0].v.pos = float4(boxPos0.yz,0.5,1);
  p[1].v.pos = float4(boxPos1.yz,0.5,1);
  p[2].v.pos = float4(boxPos2.yz,0.5,1);
  if (angle > 0)
  {
    p[0].v.pos.xy = angle == 1 ? boxPos0.xz : boxPos0.yx;
    p[1].v.pos.xy = angle == 1 ? boxPos1.xz : boxPos1.yx;
    p[2].v.pos.xy = angle == 1 ? boxPos2.xz : boxPos2.yx;
  }
  //p[0].v.pos = float4(boxPos0[u],boxPos0[v],0.5,1);//won't work on vulkan!! HLSLcc bug
  //p[1].v.pos = float4(boxPos1[u],boxPos1[v],0.5,1);//won't work on vulkan!! HLSLcc bug
  //p[2].v.pos = float4(boxPos2[u],boxPos2[v],0.5,1);//won't work on vulkan!! HLSLcc bug

  // voxel space to NDC space
  const float voxel_to_ndc = voxelize_box1.w;
  const float additionalMove = 1.0 * voxel_to_ndc;
  BRANCH
  if (additionalMove > 0.0)
  {
    //instead of this, we'd better use MSAA target or Forced Sample Count if available.
    const float2 delta_10 = normalize(p[1].v.pos.xy - p[0].v.pos.xy);
    const float2 delta_21 = normalize(p[2].v.pos.xy - p[1].v.pos.xy);
    const float2 delta_02 = normalize(p[0].v.pos.xy - p[2].v.pos.xy);
    // Move vertices for conservative rasterization.
    p[0].v.pos.xy += additionalMove * normalize(delta_02 - delta_10);
    p[1].v.pos.xy += additionalMove * normalize(delta_10 - delta_21);
    p[2].v.pos.xy += additionalMove * normalize(delta_21 - delta_02);
    //--
  }

  float3 maxPos = world_view_pos-min(min(input[0].pointToEye.xyz, input[1].pointToEye.xyz), input[2].pointToEye.xyz);
  float3 minPos = world_view_pos-max(max(input[0].pointToEye.xyz, input[1].pointToEye.xyz), input[2].pointToEye.xyz);
  p[0].aaBBMin = p[1].aaBBMin = p[2].aaBBMin = minPos;
  p[0].aaBBMax = p[1].aaBBMax = p[2].aaBBMax = maxPos;
  //we actually have to check box/triangle intersection
  //so we need to pass it to PIXEL shader and then implement check there
  //todo:

  triStream.Append(p[0]);
  triStream.Append(p[1]);
  triStream.Append(p[2]);
}
