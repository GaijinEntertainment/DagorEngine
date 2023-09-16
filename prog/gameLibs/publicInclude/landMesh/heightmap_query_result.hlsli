#ifndef HEIGHTMAP_QUERY_RESULT_HLSL_INCLUDED
#define HEIGHTMAP_QUERY_RESULT_HLSL_INCLUDED 1

struct HeightmapQueryResult
{
  // TODO: pack it more efficiently (16 bit for height + 8 offset + 8 deform, and use heightmap offset+scale for conversion)
  float heightNoOffset;
  float heightWithOffset;
  float heightWithOffsetDeform;
  float _padding; // it's faster this way on some platforms (eg. Xbox), by around 2 times (!), BUT: should use 32 or 64 bits with conversion instead
};

#endif