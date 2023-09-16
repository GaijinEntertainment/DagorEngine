#ifndef SPHERES_VERTICES_HLSL_INCLUDED
#define SPHERES_VERTICES_HLSL_INCLUDED 1

#include "spheres_consts.hlsli"
#include "general_spheres_vertices.hlsl"
float3 get_sphere_vertex_pos(uint vertex_id)
{
  return get_general_sphere_vertex_pos(vertex_id, SPHERE_SLICES, SPHERE_STACKS);
}

#endif