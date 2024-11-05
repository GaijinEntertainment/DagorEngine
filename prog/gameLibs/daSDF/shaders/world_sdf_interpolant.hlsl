#ifndef WORLD_SDF_INTERPOLANT_INCLUDED
#define WORLD_SDF_INTERPOLANT_INCLUDED 1
  struct WorldSDFInterpolant
  {
    float3 worldPos;
    uint axis;
    bool isTwoSided;
    #if RASTERIZE_PRIMS
      float3 va,vb,vc;
    #else
      bool isBackFace;
    #endif
  };
#endif