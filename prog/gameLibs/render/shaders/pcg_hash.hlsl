// https://www.reedbeta.com/blog/hash-functions-for-gpu-rendering/
// BETTER alternative to wang hash
#ifndef PCG_HASH_INCLUDED
#define PCG_HASH_INCLUDED 1

uint pcg_hash(uint input)
{
  uint state = input * 747796405u + 2891336453u;
  uint word = ((state >> ((state >> 28u) + 4u)) ^ state) * 277803737u;
  return (word >> 22u) ^ word;
}

// https://jcgt.org/published/0009/03/02/paper.pdf

uint2 pcg2d(uint2 v)
{
  v = v * 1664525u + 1013904223u;

  v.x += v.y * 1664525u;
  v.y += v.x * 1664525u;

  v = v ^ (v>>16u);

  v.x += v.y * 1664525u;
  v.y += v.x * 1664525u;

  v = v ^ (v>>16u);

  return v;
}

uint3 pcg3d_hash_32bit(uint3 v)
{
  v = v * 1664525u + 1013904223u;
  v.x += v.y*v.z; v.y += v.z*v.x; v.z += v.x*v.y;
  v ^= v >> 16u;
  v.x += v.y*v.z; v.y += v.z*v.x; v.z += v.x*v.y;
  return v;
}

uint4 pcg4d_hash_32bit(uint4 v)
{
  v = v * 1664525u + 1013904223u;
  v.x += v.y*v.w; v.y += v.z*v.x; v.z += v.x*v.y; v.w += v.y*v.z;
  v ^= v >> 16u;
  v.x += v.y*v.w; v.y += v.z*v.x; v.z += v.x*v.y; v.w += v.y*v.z;
  return v;
}

//returns only lower 16 bits hashed [0..0xffff], but works faster than 32 bit version
//the LESS components you use the faster it is (up to 8 ALU for one component, +2 per each next component)
uint3 pcg3d_hash_16bit(uint3 v)
{
  // Linear congruential step.
  v = v * 1664525u + 1013904223u;
  v.x += v.y*v.z; v.y += v.z*v.x; v.z += v.x*v.y;
  v.x += v.y*v.z; v.y += v.z*v.x; v.z += v.x*v.y;
  return v >> 16u;
}

//returns only lower 16 bits hashed [0..0xffff], but works faster than 32 bit version
//the LESS components you use the faster it is
uint4 pcg4d_hash_16bit(uint4 v)
{
  v = v * 1664525u + 1013904223u;
  v.x += v.y*v.w; v.y += v.z*v.x; v.z += v.x*v.y; v.w += v.y*v.z;
  v.x += v.y*v.w; v.y += v.z*v.x; v.z += v.x*v.y; v.w += v.y*v.z;
  return v >> 16u;
}


#endif