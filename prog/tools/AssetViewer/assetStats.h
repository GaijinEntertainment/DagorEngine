#pragma once

struct AssetStats
{
  struct GeometryStat
  {
    GeometryStat() { clear(); }

    void clear()
    {
      meshNodeCount = 0;
      meshTriangleCount = 0;
      boxNodeCount = 0;
      sphereNodeCount = 0;
      capsuleNodeCount = 0;
      convexNodeCount = 0;
      convexVertexCount = 0;
    }

    unsigned meshNodeCount;
    unsigned meshTriangleCount;
    unsigned boxNodeCount;
    unsigned sphereNodeCount;
    unsigned capsuleNodeCount;
    unsigned convexNodeCount;
    unsigned convexVertexCount;
  };

  enum class AssetType
  {
    None,
    Entity,
    Collision,
  };

  AssetStats() { clear(); }

  void clear()
  {
    assetType = AssetType::None;
    trianglesRenderable = 0;
    physGeometry.clear();
    traceGeometry.clear();
    materialCount = 0;
    textureCount = 0;
    currentLod = -1;
    mixedLod = false;
  }

  AssetType assetType;
  unsigned trianglesRenderable; // amount of renderable triangles, taken from the current LOD
  GeometryStat physGeometry;
  GeometryStat traceGeometry;
  unsigned materialCount;
  unsigned textureCount;
  int currentLod;
  bool mixedLod;
};
