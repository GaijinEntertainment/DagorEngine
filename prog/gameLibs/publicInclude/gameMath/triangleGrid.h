//
// Dagor Engine 6.5 - Game Libraries
// Copyright (C) 2023  Gaijin Games KFT.  All rights reserved
// (for conditions of use see prog/license.txt)
//
#pragma once

#include <vecmath/dag_vecMath.h>
#include <generic/dag_carray.h>
#include <math/dag_traceRayTriangle.h>

// requires at least GridSize*GridSize*2 bytes of stack
template <int GridSize, int MAX_TRI_COUNT>
void arrange_triangle_grid(vec3f bbox_min, vec3f bbox_max, const vec3f *__restrict triangles, int count, // each triangle is {vert0,
                                                                                                         // edge0, edge1}
  uint16_t *__restrict gridTri,   // gridTri is array of sufficient size, but no more than MAX_TRI_COUNT*(GridSize*GridSize). each
                                  // gridTri element is triangle
  uint16_t *__restrict gridStart, // gridStart is array of (GridSize*GridSize+1) size; gridStart[x+z*GridSize] is start index in array
                                  // of gridTri, while gridStart[GridSize*GridSize] - is total number of grid indices
  uint16_t *__restrict gridTriTemp) // temp array, should be of size MAX_TRI_COUNT*(GridSize*GridSize)
{
  vec4f worldGridScaleXZ = v_perm_xzxz(v_div(v_splats(GridSize), v_sub(bbox_max, bbox_min)));

  vec4f worldOriginXZ = v_perm_xzxz(bbox_min);
  vec4f maxGrid = v_splats(GridSize - 1), zero = v_zero();
  const vec3f *endTri = triangles + count * 3;
  carray<uint16_t, GridSize * GridSize> usedGrid;
  mem_set_0(usedGrid);

  for (int tri = 0; triangles != endTri; triangles += 3, tri++)
  {
    bbox3f tribox;
    vec4f vert0 = triangles[0];
    v_bbox3_init(tribox, vert0);
    v_bbox3_add_pt(tribox, v_add(triangles[1], vert0));
    v_bbox3_add_pt(tribox, v_add(triangles[2], vert0));
    // v_bbox3_add_pt(tribox, triangles[1]);
    // v_bbox3_add_pt(tribox, triangles[2]);

    vec4f triBboxXZ = v_perm_xzac(tribox.bmin, tribox.bmax);
    triBboxXZ = v_sub(triBboxXZ, worldOriginXZ);
    /*debug("tribox = (%g %g) .. (%g %g)",
      v_extract_x(v_mul(triBboxXZ, worldGridScaleXZ)),
      v_extract_y(v_mul(triBboxXZ, worldGridScaleXZ)),
      v_extract_z(v_mul(triBboxXZ, worldGridScaleXZ)),
      v_extract_w(v_mul(triBboxXZ, worldGridScaleXZ)));*/
    triBboxXZ = v_max(v_min(v_mul(triBboxXZ, worldGridScaleXZ), maxGrid), zero); // we do not check, if trinagle is out of the box - we
                                                                                 // considered it is always within
    vec4i triBboxXZi = v_cvt_vec4i(triBboxXZ);
    DECL_ALIGN16(int, region[4]);
    v_sti(region, triBboxXZi);
    int gridI = region[1] * GridSize + region[0];
    int gridStrideOfs = GridSize - (region[2] - region[0] + 1);
    for (int z = region[1]; z <= region[3]; ++z, gridI += gridStrideOfs)
      for (int x = region[0]; x <= region[2]; ++x, gridI++)
      {
        gridTriTemp[gridI * MAX_TRI_COUNT + usedGrid[gridI]] = tri;
        usedGrid[gridI]++;
      }
  }
  uint16_t *startGridTri = gridTri;
  for (int z = 0, gridI = 0; z < GridSize; ++z)
    for (int x = 0; x < GridSize; ++x, gridI++)
    {
      int usedGridCnt = usedGrid[gridI];
      memcpy(gridTri, &gridTriTemp[gridI * MAX_TRI_COUNT], usedGridCnt * sizeof(uint16_t));
      // usedGrid[gridI] = (usedGrid[gridI]+3)&(~3);
      /*uint16_t lastTri = gridTriTemp[gridI*MAX_TRI_COUNT + usedGridCnt-1];
      for (;usedGridCnt&3; ++usedGridCnt)
        gridTri[usedGridCnt] = lastTri;*/
      gridStart[gridI] = gridTri - startGridTri;
      gridTri += usedGridCnt;
    }
  gridStart[GridSize * GridSize] = gridTri - startGridTri;
}

// requires (MAX_TRI_COUNT+1)*GridSize*GridSize*2 bytes of stack
template <int GridSize, int MAX_TRI_COUNT>
void arrange_triangle_grid_stack(vec3f bbox_min, vec3f bbox_max, const vec3f *__restrict triangles, int count, // each triangle is
                                                                                                               // {vert0, edge0, edge1}
  uint16_t *__restrict gridTri, uint16_t *__restrict gridCount)
{
  carray<uint16_t, GridSize * GridSize * MAX_TRI_COUNT> gridTriTemp;
  arrange_triangle_grid<GridSize, MAX_TRI_COUNT>(bbox_min, bbox_max, triangles, count, gridTri, gridCount,
    gridTriTemp.data()); // each triangle is {vert0, edge0, edge1}
}

/*template <int GridSize>
void trace_grid_multiray(vec3f bbox_min, vec3f bbox_max,
          const vec3f *triangles, int count, // each triangle is {vert0, edge0, edge1}
          const uint16_t *gridTri,//gridTri is array of sufficient size, but no more than MAX_TRI_COUNT*(GridSize*GridSize). each
gridTri element is triangle const uint16_t *gridStart,//gridStart is array of (GridSize*GridSize+1) size; gridStart[x+z*GridSize] is
start index in array of gridTri, while gridStart[GridSize*GridSize] - is total number of grid indices vec4f rayBoxXZ,//xzxz vec4f *rays
          )//temp array, should be of size MAX_TRI_COUNT*(GridSize*GridSize)
{
}*/
