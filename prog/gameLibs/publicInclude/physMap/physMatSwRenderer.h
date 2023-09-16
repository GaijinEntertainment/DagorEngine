//
// Dagor Engine 6.5 - Game Libraries
// Copyright (C) 2023  Gaijin Games KFT.  All rights reserved
// (for conditions of use see prog/license.txt)
//
#pragma once

#include <math/dag_scanLineRasterizer.h>
#include <memory/dag_framemem.h>
#include <physMap/physMap.h> // for PhysMap::BitmapPhysTex<>

template <int Width, int Height>
class RenderDecalMaterials
{
public:
  enum
  {
    WIDTH = Width,
    HEIGHT = Height
  };
  typedef uint8_t MatType;
  struct MatFrameBuffer
  {
    MatFrameBuffer() : materials(NULL), materialsIdx(0), fixedMat(255), currentPhysTex(NULL) {}
    MatFrameBuffer(dag::Span<MatType> &mats) : materials(&mats), materialsIdx(0), fixedMat(255), currentPhysTex(NULL) {}

    dag::Span<MatType> *materials;
    int materialsIdx;
    MatType fixedMat;
    const PhysMap::BitmapPhysTex<uint32_t, 32> *currentPhysTex;

    void operator+=(int ofs) { materialsIdx += ofs; }
    void shade() const { (*materials)[materialsIdx] = fixedMat; }
  };

  RenderDecalMaterials() : materials(new MatType[WIDTH * HEIGHT], WIDTH * HEIGHT)
  {
    ownsMaterials = true;
    resetMats();
    rasterizer.init(MatFrameBuffer(materials));
  }
  RenderDecalMaterials(dag::Span<MatType> in_materials) : materials(in_materials.data(), in_materials.size())
  {
    G_ASSERT(in_materials.size() == WIDTH * HEIGHT);
    ownsMaterials = false;
    resetMats();
    rasterizer.init(MatFrameBuffer(materials));
  }
  virtual ~RenderDecalMaterials()
  {
    if (ownsMaterials)
      delete[] (materials.data());
  }

  void resetMats() { mem_set_ff(materials); }

  static inline void shade(const MatFrameBuffer &pixel, const Point2 &tex_coord)
  {
    if (pixel.currentPhysTex)
    {
      // Wrap
      Point2 texCoord(tex_coord);
      texCoord.x -= floorf(tex_coord.x);
      texCoord.y -= floorf(tex_coord.y);
      IPoint2 texCoordPix = IPoint2::xy(texCoord * (pixel.currentPhysTex->WIDTH - 1));
      if (!(pixel.currentPhysTex->pixels[texCoordPix.y] & (1 << texCoordPix.x)))
        return;
    }
    pixel.shade();
  }

  bool checkPixel(int x, int y) const { return materials[y * WIDTH + x] < 255; }

  inline void renderDecalMesh(const PhysMap &phys_map, const BBox2 &region, const PhysMap::DecalMesh &mesh)
  {
    if (!(region & mesh.box))
      return;

    Point2 regOrigin = region.lim[0];
    Point2 regWidth = region.width();
    Tab<IPoint2> vertices(framemem_ptr());
    Tab<uint8_t> vertices_bits(framemem_ptr());

    if (mesh.box[0].x < region[0].x || mesh.box[0].y < region[0].y || mesh.box[1].x > region[1].x || mesh.box[1].y > region[1].y)
      vertices_bits.resize(mesh.vertices.size());
    else
      vertices_bits.clear();

    vertices.resize(mesh.vertices.size());
    float scaleX = (WIDTH << SR_PRECISE_BITS) / regWidth.x, scaleY = (HEIGHT << SR_PRECISE_BITS) / regWidth.y;
    for (int vi = 0; vi < vertices.size(); ++vi)
    {
      Point2 vert = mesh.vertices[vi] - regOrigin;
      vert.x *= scaleX;
      vert.y *= scaleY;
      vertices[vi] = IPoint2::xy(vert + Point2(0.5 * SR_PRECISE_VALUE, 0.5 * SR_PRECISE_VALUE));

      if (vertices_bits.size())
        vertices_bits[vi] = (vertices[vi].x < 0) | ((vertices[vi].x > (WIDTH << SR_PRECISE_BITS)) << 1) | ((vertices[vi].y < 0) << 2) |
                            ((vertices[vi].y > (HEIGHT << SR_PRECISE_BITS)) << 3);
    }
    for (const PhysMap::DecalMesh::MaterialIndices &indices : mesh.matIndices)
    {
      rasterizer.pixels.fixedMat = indices.matId;
      rasterizer.pixels.currentPhysTex = phys_map.physTextures.data() + indices.bitmapTexId;
      for (int ii = 0; ii < indices.indices.size(); ii += 3)
      {
        if (vertices_bits.size())
        {
          uint8_t bits =
            vertices_bits[indices.indices[ii]] & vertices_bits[indices.indices[ii + 1]] & vertices_bits[indices.indices[ii + 2]];
          if (bits)
            continue;
        }

        carray<IPoint2, 3> verts;
        carray<Point2, 3> texCoords;
        for (int idx = 0; idx < 3; ++idx)
        {
          verts[idx] = vertices[indices.indices[ii + idx]];
          if (!indices.tindices.empty())
            texCoords[idx] = mesh.texCoords[indices.tindices[ii + idx]];
        }
        if (indices.tindices.empty())
          mem_set_0(texCoords);
        rasterizer.template draw_indexed_trianglei<true, IPoint2>(verts.data(), texCoords.data(), 0, 1, 2);
      }
    }
  }

  void renderPhysMap(const PhysMap &phys_map, const BBox2 &region, bool apply_decals = true)
  {
    if (phys_map.parent)
      phys_map.fillMaterialsRegion(region, materials, WIDTH, HEIGHT);
    else
      resetMats();

    if (!apply_decals)
      return;

    // First render usual decals. We'll likely will not have any of them if we
    // have grid decals
    for (const PhysMap::DecalMesh &mesh : phys_map.decals)
      renderDecalMesh(phys_map, region, mesh);

    // Then render grid decals
    Point2 leftTop = (region.lim[0] - phys_map.worldOffset) * phys_map.invGridScale;
    Point2 rightBottom = (region.lim[1] - phys_map.worldOffset) * phys_map.invGridScale;
    IPoint2 leftTopCell = IPoint2(floorf(leftTop.x), floorf(leftTop.y));
    IPoint2 rightBottomCell = IPoint2(ceilf(rightBottom.x), ceilf(rightBottom.y));
    for (int y = max(leftTopCell.y, 0); y < min(rightBottomCell.y, phys_map.gridSz); ++y)
      for (int x = max(leftTopCell.x, 0); x < min(rightBottomCell.x, phys_map.gridSz); ++x)
        for (const PhysMap::DecalMesh &mesh : phys_map.gridDecals[y * phys_map.gridSz + x])
          renderDecalMesh(phys_map, region, mesh);
  }
  dag::ConstSpan<MatType> getMaterials() const { return materials; }

protected:
  ScanLineRasterizer<RenderDecalMaterials, Point2, MatFrameBuffer, Width, Height> rasterizer;
  dag::Span<MatType> materials;
  bool ownsMaterials;
};
