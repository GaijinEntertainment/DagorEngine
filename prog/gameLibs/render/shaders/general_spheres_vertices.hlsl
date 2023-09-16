#ifndef GENERAL_SPHERES_VERTICES_HLSL_INCLUDED
#define GENERAL_SPHERES_VERTICES_HLSL_INCLUDED 1

float3 get_general_sphere_vertex_pos(uint vertex_id, uint sphere_slices, uint sphere_stacks)
{
  const uint QUAD_INDICES = 6;
  uint inQuadIdx = vertex_id % QUAD_INDICES;
  uint slice = (vertex_id / QUAD_INDICES) % sphere_slices;
  uint stack = vertex_id / QUAD_INDICES / sphere_slices;
  if (inQuadIdx == 1 || inQuadIdx == 3 || inQuadIdx == 4) slice++;
  if (inQuadIdx == 2 || inQuadIdx == 5 || inQuadIdx == 4) stack++;
  float stackSin, stackCos, sliceSin, sliceCos;
  sincos((PI * stack) / float(sphere_stacks), stackSin, stackCos);
  sincos((2 * PI * slice) / float(sphere_slices), sliceSin, sliceCos);
  float3 pos = float3(stackSin * sliceCos, stackCos, stackSin * sliceSin);
  return pos;
}

#endif