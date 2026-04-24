//
// Dagor Engine 6.5 - Game Libraries
// Copyright (C) Gaijin Games KFT.  All rights reserved.
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
#define SW_RASTER_PREPROCESS_VERTICES 0

  inline void renderDecalMesh(const PhysMap &phys_map, const BBox2 &region, const PhysMap::DecalMesh &mesh)
  {
    if (region[0].x > mesh.box[1].x || region[0].y > mesh.box[1].y || region[1].x < mesh.box[0].x || region[1].y < mesh.box[0].y)
      return;

    Point2 regOrigin = region.lim[0];
    Point2 regWidth = region.width();
    Point2 scale((WIDTH << SR_PRECISE_BITS) / regWidth.x, (HEIGHT << SR_PRECISE_BITS) / regWidth.y);
    Point2 relOriginHalfOfs = mul(-regOrigin, scale) + Point2(0.5f, 0.5f);
#if SW_RASTER_PREPROCESS_VERTICES
    Tab<IPoint2> vertices(framemem_ptr());
    vertices.resize(mesh.vertices.size());

    Tab<uint8_t> vertices_bits(framemem_ptr());

    if (mesh.box[0].x < region[0].x || mesh.box[0].y < region[0].y || mesh.box[1].x > region[1].x || mesh.box[1].y > region[1].y)
      vertices_bits.resize(mesh.vertices.size());
    for (int vi = 0, vie = vertices.size(); vi < vie; ++vi)
    {
      IPoint2 v = IPoint2::xy(mul(mesh.vertices[vi], scale) + relOriginHalfOfs);
      vertices[vi] = v;
      if (!vertices_bits.empty())
        vertices_bits[vi] =
          (v.x < 0) | ((v.x > (WIDTH << SR_PRECISE_BITS)) << 1) | ((v.y < 0) << 2) | ((v.y > (HEIGHT << SR_PRECISE_BITS)) << 3);
    }
#else
    vec4f vScale = v_make_vec4f(scale.x, scale.y, scale.x, scale.y),
          vOfs = v_make_vec4f(relOriginHalfOfs.x, relOriginHalfOfs.y, relOriginHalfOfs.x, relOriginHalfOfs.y);
    vec4f vRegion = v_make_vec4f(region[1].x, region[1].y, -region[0].x, -region[0].y);
#endif
    for (const PhysMap::DecalMesh::MaterialIndices &indices : mesh.matIndices)
    {
      rasterizer.pixels.fixedMat = indices.matId;
      rasterizer.pixels.currentPhysTex = (indices.bitmapTexId >= 0 && indices.bitmapTexId < phys_map.physTextures.size())
                                           ? phys_map.physTextures.data() + indices.bitmapTexId
                                           : NULL;
      for (int ii = 0, iie = indices.indices.size(); ii < iie; ii += 3)
      {
        int inds[3] = {indices.indices[ii], indices.indices[ii + 1], indices.indices[ii + 2]};
#if SW_RASTER_PREPROCESS_VERTICES
        if (!vertices_bits.empty())
        {
          if (vertices_bits[inds[0]] & vertices_bits[inds[1]] & vertices_bits[inds[2]])
            continue;
        }
        carray<IPoint2, 3> verts;
        verts[0] = vertices[inds[0]];
        verts[1] = vertices[inds[1]];
        verts[2] = vertices[inds[2]];
#else
        vec4f v01 = v_perm_xyab(v_ldu_half(&mesh.vertices[inds[0]].x), v_ldu_half(&mesh.vertices[inds[1]].x));
        vec4f v2 = v_ldu_half(&mesh.vertices[inds[2]].x);
        vec4f vMMinax = v_perm_xyab(v_min(v_min(v01, v2), v_perm_zwxy(v01)), v_neg(v_max(v_max(v01, v2), v_perm_zwxy(v01))));
        if (v_check_xyzw_any_true(v_cmp_gt(vMMinax, vRegion)))
          continue;
        alignas(16) carray<IPoint2, 3> verts;
        v_sti(&verts[0].x, v_cvt_vec4i(v_madd(v01, vScale, vOfs)));
        v_stui_half(&verts[2].x, v_cvt_vec4i(v_madd(v2, vScale, vOfs)));
#endif

        carray<Point2, 3> texCoords;
        if (mesh.flags & mesh.TINDICES_SAME_AS_INDICES)
        {
          texCoords[0] = mesh.texCoords[inds[0]];
          texCoords[1] = mesh.texCoords[inds[1]];
          texCoords[2] = mesh.texCoords[inds[2]];
        }
        else if (!indices.tindices.empty())
        {
          texCoords[0] = mesh.texCoords[indices.tindices[ii + 0]];
          texCoords[1] = mesh.texCoords[indices.tindices[ii + 1]];
          texCoords[2] = mesh.texCoords[indices.tindices[ii + 2]];
        }
        else
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

    if (!apply_decals || region.isempty())
      return;

    // First render usual decals. We'll likely will not have any of them if we
    // have grid decals
    for (const PhysMap::DecalMesh &mesh : phys_map.decals)
      renderDecalMesh(phys_map, region, mesh);

    // Then render grid decals
    Point2 leftTop = (region.lim[0] - phys_map.worldOffset) * phys_map.invGridScale;
    Point2 rightBottom = (region.lim[1] - phys_map.worldOffset) * phys_map.invGridScale;
    IPoint2 leftTopCell = max(IPoint2(0, 0), IPoint2::xy(leftTop));
    IPoint2 rightBottomCell = min(IPoint2(phys_map.gridSz - 1, phys_map.gridSz - 1), IPoint2::xy(floor(rightBottom)));
    for (int y = leftTopCell.y, i = y * phys_map.gridSz + leftTopCell.x,
             strideOfs = phys_map.gridSz - (rightBottomCell.x - leftTopCell.x + 1);
         y <= rightBottomCell.y; ++y, i += strideOfs)
      for (int x = leftTopCell.x; x <= rightBottomCell.x; ++x, ++i)
        for (const PhysMap::DecalMesh &mesh : phys_map.gridDecals[i])
          renderDecalMesh(phys_map, region, mesh);
  }
  dag::ConstSpan<MatType> getMaterials() const { return materials; }

protected:
  ScanLineRasterizer<RenderDecalMaterials, Point2, MatFrameBuffer, Width, Height> rasterizer;
  dag::Span<MatType> materials;
  bool ownsMaterials;
};
