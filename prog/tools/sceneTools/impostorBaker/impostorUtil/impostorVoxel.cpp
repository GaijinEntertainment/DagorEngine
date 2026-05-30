// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include "impostorBaker.h"
#include <shaders/pack_impostor_texture.hlsl>
#include <osApiWrappers/dag_direct.h>
#include <vecmath/dag_vecMath.h>
#include <util/dag_bitArray.h>
#include <perfMon/dag_perfTimer.h>
#include <shaders/dag_rendInstRes.h>
#include <shaders/dag_shaderBlock.h>
#include <3d/dag_texIdSet.h>
#include <3d/dag_lockTexture.h>
#include <assets/asset.h>
#include <image/dag_png.h>
#include <workCycle/dag_workCycle.h>
#include <drv/3d/dag_lock.h>
#include <drv/3d/dag_info.h>
#include <drv/3d/dag_viewScissor.h>
#include <drv/3d/dag_rwResource.h>
#include <rendInst/rendInstExtra.h>
#include <ioSys/dag_fileIo.h>
#include <ioSys/dag_zstdIo.h>
#include <libTools/util/makeBindump.h>
#include <libTools/voxel/voxelCache.h>
#include <EASTL/deque.h>
#include <EASTL/sort.h>


#define VOXEL_OVERSCALE        3
#define ZSTD_COMPRESSION_LEVEL 18


static float compute_threshold_length(RenderableInstanceResource *res, float threshold_percent)
{
  G_ASSERT(res);
  ShaderMesh *mesh = res->getMesh()->getMesh()->getMesh();
  G_ASSERT(mesh);
  auto elems = mesh->getAllElems();

  uint32_t numFaces = 0;
  for (auto &elem : elems)
    if (elem.e)
      numFaces += elem.numf;

  Tab<float> triLen(tmpmem);
  triLen.reserve(numFaces);

  for (auto &elem : elems)
  {
    if (!elem.e)
      continue;
    const GlobalVertexData *vd = elem.vertexData;
    uint32_t indexSize = vd->getIbElemSz();
    int stride = vd->getStride();
    eastl::unique_ptr<Sbuffer, DestroyDeleter<Sbuffer>> dstVB, dstIB;
    dstVB.reset(d3d::create_sbuffer(1, vd->getVbSize(), SBCF_USAGE_READ_BACK, 0, "readBackVB"));
    vd->getVB()->copyTo(dstVB.get(), 0, elem.baseVertex * stride, vd->getVbSize());

    uint8_t *memVB;
    if (dstVB->lock(0, dstVB->getSize(), (void **)&memVB, VBLOCK_READONLY))
    {
      Sbuffer *ib = vd->getIB();
      uint8_t *memIB;
      uint32_t count = elem.numf;
      uint32_t start = 0;
      dstIB.reset(d3d::create_sbuffer(1, vd->getIbSize(), SBCF_USAGE_READ_BACK, 0, "readBackIB"));
      if (ib)
        ib->copyTo(dstIB.get(), 0, elem.si * indexSize, vd->getIbSize());
#if DAGOR_DBGLEVEL > 0
      else
        G_ASSERT(0);
#endif
      if (dstIB->lock(0, dstIB->getSize(), (void **)&memIB, VBLOCK_READONLY))
      {
        for (int k = start * 3, i = 0; k < start * 3 + count * 3; k += 3, ++i)
        {
          uint8_t *ip = memIB + k * indexSize;
          uint32_t idx[3];
          idx[0] = indexSize == 2 ? *(uint16_t *)ip : *(uint32_t *)ip;
          idx[1] = indexSize == 2 ? *((uint16_t *)ip + 1) : *((uint32_t *)ip + 1);
          idx[2] = indexSize == 2 ? *((uint16_t *)ip + 2) : *((uint32_t *)ip + 2);
          vec3f vert[3];
          for (int j = 0; j < 3; ++j)
            vert[j] = v_ldu_p3((const float *)(memVB + idx[j] * stride));
          vec3f maxLen = v_length3_sq_x(v_sub(vert[0], vert[1]));
          maxLen = v_max(maxLen, v_length3_sq_x(v_sub(vert[1], vert[2])));
          maxLen = v_max(maxLen, v_length3_sq_x(v_sub(vert[2], vert[0])));
          triLen.push_back(v_extract_x(maxLen));
        }
        dstIB->unlock();
      }
      dstVB->unlock();
    }
  }

  if (triLen.empty())
    return -1;

  eastl::sort(triLen.begin(), triLen.end(), eastl::less<float>{});

  int index = int(floorf(threshold_percent * 0.01f * triLen.size()));
  index = clamp<int>(index, 0, triLen.size() - 1);
  return triLen[index];
}


float ImpostorBaker::computeVoxelSize(RenderableInstanceLodsResource *res, const char *name, float proj_scale,
  float triangle_threshold)
{
  // scan lod meshes to compute voxel size
  float voxelSize = -1;
  uint32_t numLods = max<int>(res->lods.size() - (res->hasImpostor() ? 2 : 0), 0);
  if (numLods == 0)
  {
    conerror("No usable LoDs (%d total%s) for rendInst %s", res->lods.size(), res->hasImpostor() ? ", has impostor" : "", name);
    return false;
  }

  for (uint32_t lodIndex = 0; lodIndex < numLods; lodIndex++)
  {
    if (!res->lods[lodIndex].scene)
    {
      conerror("LoD %d is not loaded for rendInst %s", lodIndex, name);
      return false;
    }

    float len = compute_threshold_length(res->lods[lodIndex].scene.get(), triangle_threshold);
    if (len <= 0)
    {
      conwarning("LoD %d of rendInst %s has no usable triangles", lodIndex, name);
      continue;
    }

    float vsize = len * 0.5f;                // threshold when triangle is about 2 voxels/pixels
    float range = proj_scale * 0.5f * vsize; // 0.5 is from radius/diameter, see defaults::projScale comment

    if (range < res->lods[lodIndex].range)
    {
      conwarning("LoD %d of rendInst %s has ineffective range=%f (should be < %f)", lodIndex, name, res->lods[lodIndex].range, range);
    }
    else
    {
      // assume we switch to voxels at the lod range
      vsize = res->lods[lodIndex].range * 2 / proj_scale;
    }

    voxelSize = max(voxelSize, vsize);
  }

  return voxelSize;
}


bool ImpostorBaker::beginVoxelBaking(RenderableInstanceLodsResource *res, const IPoint3 &map_size, const Point3 &box_min,
  float voxel_size)
{
  G_ASSERT(get_impostor_texture_mgr());
  G_ASSERT(res);
  G_ASSERT(res->lods.size() > 0);
  G_ASSERT(res->lods[0].scene);
  if (!d3d::is_inited())
  {
    conerror("Drawing with uninitialized d3d");
    return false;
  }

  bakeRes = res;
  bakeMapSize = map_size;
  bakeBoxMin = box_min;
  bakeVoxelSize = voxel_size;

  {
    TextureIdSet usedTexIds;
    res->gatherUsedTex(usedTexIds);
    if (!prefetch_textures(usedTexIds))
    {
      conerror("There was a problem loading %d textures", usedTexIds.size());
      return false;
    }
  }

  const IPoint2 extent = IPoint2(voxelBakeWidth(), voxelBakeHeight()) * VOXEL_OVERSCALE;
  if (!voxelRt or voxelRt->getWidth() < extent.x or voxelRt->getHeight() < extent.y)
  {
    static unsigned int textureCount = 0; // this helps with debugging
    const static unsigned formats[] = {
      TEXFMT_A8R8G8B8 | TEXCF_SRGBREAD | TEXCF_SRGBWRITE, TEXFMT_A8R8G8B8, TEXFMT_A8R8G8B8, TEXFMT_R32F};
    voxelRt.reset();
    voxelRt = eastl::make_unique<DeferredRenderTarget>(nullptr, String(0, "voxel_baker_rt_%d", textureCount), extent.x, extent.y,
      DeferredRT::StereoMode::MonoOrMultipass, 0, 4, formats, GBUF_DEPTH_FMT);
    textureCount++;
  }

  for (int i = 0; i < 4; i++)
    if (!voxelRt->getRt(i, true))
    {
      conerror("Failed to create baking render target %dx%d", P2D(extent));
      return false;
    }

  if (displayExportedImages)
  {
    static uint32_t counter = 0;
    counter++;
    lastExportedAlbedoAlpha = dag::create_tex(nullptr, extent.x, extent.y,
      TEXFMT_A8R8G8B8 | TEXCF_SRGBREAD | TEXCF_SRGBWRITE | TEXCF_RTARGET, 1, String(0, "voxel_display_%u", counter).c_str());
  }

  get_impostor_texture_mgr()->buildRendinstElems();
  return true;
}


void ImpostorBaker::endVoxelBaking() { bakeRes = nullptr; }


bool ImpostorBaker::VoxelReadBackTextures::create(int w, int h)
{
  static uint32_t counter = 0;
  counter++;
  auto flags = TEXCF_RTARGET;
  albedo = dag::create_tex(nullptr, w, h, TEXFMT_A8R8G8B8 | TEXCF_SRGBREAD | TEXCF_SRGBWRITE | flags, 1,
    String(0, "voxel_readback_albedo_%u", counter).c_str());
  if (!albedo)
    return false;
  normal = dag::create_tex(nullptr, w, h, TEXFMT_A8R8G8B8 | flags, 1, String(0, "voxel_readback_normal_%u", counter).c_str());
  if (!normal)
    return false;
  extra = dag::create_tex(nullptr, w, h, TEXFMT_A8R8G8B8 | flags, 1, String(0, "voxel_readback_extra_%u", counter).c_str());
  if (!extra)
    return false;
  depth = dag::create_tex(nullptr, w, h, TEXFMT_R32F | TEXCF_CLEAR_ON_CREATE | flags, 1,
    String(0, "voxel_readback_depth_%u", counter).c_str());
  if (!depth)
    return false;
  return true;
}


template <typename Image>
static void alloc_image(eastl::unique_ptr<Image, tmpmemDeleter> &image, uint32_t w, uint32_t h)
{
  if (!image or image->w != w or image->h != h)
    image.reset(Image::create(w, h, tmpmem));
}


template <typename Image>
static bool texture_to_image(BaseTexture *texture, eastl::unique_ptr<Image, tmpmemDeleter> &image)
{
  G_ASSERT(texture);
  G_ASSERT(image);
  auto locked = lock_texture<const typename eastl::remove_pointer<decltype(image->getPixels())>::type>(texture, 0, TEXLOCK_READ);
  if (!bool(locked))
    return false;
  uint32_t w = locked.getWidthInElems();
  uint32_t h = locked.getHeightInElems();
  G_ASSERT(image->w <= w and image->h <= h);
  for (uint32_t y = 0; y < image->h; y++)
    locked.readElems(make_span(image->getPixels() + image->w * y, image->w), y, 0, image->w);
  return true;
}


void ImpostorBaker::getVoxelFaceParams(uint32_t face_id, uint32_t &view_w, uint32_t &view_h, uint32_t &view_d, IPoint3 &dir_x,
  IPoint3 &dir_y, IPoint3 &dir_z) const
{
  switch (face_id)
  {
    case 0:
    case 1:
      // ZY
      view_w = bakeMapSize.z;
      view_h = bakeMapSize.y;
      view_d = bakeMapSize.x;
      dir_x = IPoint3(0, 0, int((face_id & 1) * 2) - 1);
      dir_y = IPoint3(0, 1, 0);
      dir_z = IPoint3(1 - int((face_id & 1) * 2), 0, 0);
      break;
    case 2:
    case 3:
      // ZX
      view_w = bakeMapSize.z;
      view_h = bakeMapSize.x;
      view_d = bakeMapSize.y;
      dir_x = IPoint3(0, 0, 1 - int((face_id & 1) * 2));
      dir_y = IPoint3(1, 0, 0);
      dir_z = IPoint3(0, 1 - int((face_id & 1) * 2), 0);
      break;
    case 4:
    case 5:
      // XY
      view_w = bakeMapSize.x;
      view_h = bakeMapSize.y;
      view_d = bakeMapSize.z;
      dir_x = IPoint3(1 - int((face_id & 1) * 2), 0, 0);
      dir_y = IPoint3(0, 1, 0);
      dir_z = IPoint3(0, 0, 1 - int((face_id & 1) * 2));
      break;
    default:
      G_ASSERT(false);
      view_w = 0;
      view_h = 0;
      view_d = 0;
      dir_x = IPoint3(1, 0, 0);
      dir_y = IPoint3(0, 1, 0);
      dir_z = IPoint3(0, 0, 1);
  }
}

void ImpostorBaker::getVoxelFaceParams(uint32_t face_id, uint32_t &view_w, uint32_t &view_h, uint32_t &view_d, Point3 &dir_x,
  Point3 &dir_y, Point3 &dir_z) const
{
  IPoint3 dx, dy, dz;
  getVoxelFaceParams(face_id, view_w, view_h, view_d, dx, dy, dz);
  dir_x = point3(dx);
  dir_y = point3(dy);
  dir_z = point3(dz);
}


bool ImpostorBaker::bakeVoxels(dag::Span<VoxelBakeRequest> requests)
{
  // create readback textures as needed
  for (int ri = 0; ri < requests.size(); ri++)
  {
    auto &req = requests[ri];
    if (req.readback.depth)
      continue;
    if (!req.readback.create(voxelBakeWidth() * VOXEL_OVERSCALE, voxelBakeHeight() * VOXEL_OVERSCALE))
    {
      conerror("Failed to create baking readback textures");
      return false;
    }
  }

  d3d::GpuAutoLock gpuLock;
  SCOPE_VIEW_PROJ_MATRIX;
  int rendinstSceneBlockId = ShaderGlobal::getBlockId("rendinst_scene");
  int depthProjVarId = get_shader_variable_id("impostor_depth_proj", true);
  int depthSliceVarId = get_shader_variable_id("impostor_depth_slice", true);

  // render requested views
  {
    TIME_D3D_PROFILE(bake_voxels_render);
    SCOPE_RENDER_TARGET;
    get_impostor_texture_mgr()->start_rendering_slices(voxelRt.get());

    for (int ri = 0; ri < requests.size(); ri++)
    {
      auto &req = requests[ri];
      G_ASSERT(req.faceId < 6);

      TMatrix viewToModel;
      uint32_t viewW, viewH, viewD;
      getVoxelFaceParams(req.faceId, viewW, viewH, viewD, viewToModel.col[0], viewToModel.col[1], viewToModel.col[2]);

      if (!d3d::clear_rt({voxelRt->getRt(3), 0, 0}, make_clear_value(1e30f, 0.0f, 0.0f, 0.0f)))
      {
        conerror("Failed to clear depth RT");
        return false;
      }

      // optional clearing for debugging
      for (int i = 0; i < 3; i++)
        d3d::clear_rt({voxelRt->getRt(i), 0, 0}, make_clear_value(0, 0, 0, 0));

      voxelRt->setRt();
      d3d::clearview(CLEAR_ZBUFFER | CLEAR_STENCIL, 0, 0, 0);

      d3d::setview(0, 0, viewW * VOXEL_OVERSCALE, viewH * VOXEL_OVERSCALE, 0, 1);
      d3d::setscissor(0, 0, viewW * VOXEL_OVERSCALE, viewH * VOXEL_OVERSCALE);

      alloc_image(req.albedo, viewW * VOXEL_OVERSCALE, viewH * VOXEL_OVERSCALE);
      alloc_image(req.normal, viewW * VOXEL_OVERSCALE, viewH * VOXEL_OVERSCALE);
      alloc_image(req.extra, viewW * VOXEL_OVERSCALE, viewH * VOXEL_OVERSCALE);
      alloc_image(req.depth, viewW * VOXEL_OVERSCALE, viewH * VOXEL_OVERSCALE);

      float wd = viewW * bakeVoxelSize;
      float ht = viewH * bakeVoxelSize;
      float dh = viewD * bakeVoxelSize;

      Point3 vp = bakeBoxMin;
      vp += abs(viewToModel.getcol(0)) * (wd * 0.5f);
      vp += abs(viewToModel.getcol(1)) * (ht * 0.5f);
      if (req.faceId & 1)
        vp += abs(viewToModel.getcol(2)) * dh;
      viewToModel.setcol(3, vp);

      float zn = req.offset * bakeVoxelSize / VOXEL_OVERSCALE;
      G_ASSERT(zn < dh);
      float zf = dh + bakeVoxelSize;
      Point3 zproj = viewToModel.col[2] * (VOXEL_OVERSCALE / bakeVoxelSize);

      ShaderGlobal::set_float4(depthProjVarId, zproj, 1 - dot(zproj, viewToModel.col[3]));
      ShaderGlobal::set_texture(depthSliceVarId, req.readback.depth.getTex2D());

      // debug("!!! faceId=%d", req.faceId);
      // for (int i = 0; i < 4; i++)
      //   debug("!!! %f %f %f", P3D(viewToModel.col[i]));
      get_impostor_texture_mgr()->render_slice_voxels(viewToModel, wd, ht, zn, zf, bakeRes, rendinst::RenderPass::ImpostorVoxel,
        rendinstSceneBlockId);

      ShaderGlobal::set_texture(depthSliceVarId, nullptr);

      // copy to readback textures
      d3d::resource_barrier({voxelRt->getRt(0), RB_RO_COPY_SOURCE | RB_RO_BLIT_SOURCE, 0, 0});
      if (!d3d::stretch_rect(voxelRt->getRt(0), req.readback.albedo.getTex2D()))
      {
        conerror("Failed to copy albedo RT");
        return false;
      }

      d3d::resource_barrier({voxelRt->getRt(1), RB_RO_COPY_SOURCE | RB_RO_BLIT_SOURCE, 0, 0});
      if (!d3d::stretch_rect(voxelRt->getRt(1), req.readback.normal.getTex2D()))
      {
        conerror("Failed to copy normal RT");
        return false;
      }
      if (ri == 0 and displayExportedImages)
        d3d::stretch_rect(voxelRt->getRt(1), lastExportedAlbedoAlpha.getTex2D());

      d3d::resource_barrier({voxelRt->getRt(2), RB_RO_COPY_SOURCE | RB_RO_BLIT_SOURCE, 0, 0});
      if (!d3d::stretch_rect(voxelRt->getRt(2), req.readback.extra.getTex2D()))
      {
        conerror("Failed to copy extra RT");
        return false;
      }

      d3d::resource_barrier({voxelRt->getRt(3), RB_RO_COPY_SOURCE | RB_RO_BLIT_SOURCE, 0, 0});
      if (!d3d::stretch_rect(voxelRt->getRt(3), req.readback.depth.getTex2D()))
      {
        conerror("Failed to copy depth RT");
        return false;
      }
    }

    get_impostor_texture_mgr()->end_rendering_slices();
  }

  // get rendered images
  d3d::driver_command(Drv3dCommand::D3D_FLUSH);

  for (int ri = 0; ri < requests.size(); ri++)
  {
    auto &req = requests[ri];

    if (!texture_to_image(req.readback.albedo.getTex2D(), req.albedo))
    {
      conerror("failed to lock albedo readback texture: %s", d3d::get_last_error());
      return false;
    }
    if (!texture_to_image(req.readback.normal.getTex2D(), req.normal))
    {
      conerror("failed to lock normal readback texture: %s", d3d::get_last_error());
      return false;
    }
    if (!texture_to_image(req.readback.extra.getTex2D(), req.extra))
    {
      conerror("failed to lock extra readback texture: %s", d3d::get_last_error());
      return false;
    }
    if (!texture_to_image(req.readback.depth.getTex2D(), req.depth))
    {
      conerror("failed to lock depth readback texture: %s", d3d::get_last_error());
      return false;
    }
  }

  return true;
}


struct VoxelBitmap
{
  Bitarray map;
  IPoint3 size = IPoint3::ZERO;

  VoxelBitmap() : map(tmpmem) {}
  VoxelBitmap(const IPoint3 &s) : map(tmpmem), size(s) { map.resize(s.x * s.y * s.z); }

  void reset() { map.reset(); }

  inline bool outside(int x, int y, int z) const
  {
    return uint32_t(x) >= uint32_t(size.x) or uint32_t(y) >= uint32_t(size.y) or uint32_t(z) >= uint32_t(size.z);
  }

  inline uint32_t bitIndex(int x, int y, int z) const { return (y * size.z + z) * size.x + x; }

  bool get(int x, int y, int z) const
  {
    if (outside(x, y, z))
      return false;
    return map[bitIndex(x, y, z)];
  }
  inline bool get(const IPoint3 &p) const { return get(P3D(p)); }

  void set(int x, int y, int z, bool v = true)
  {
    if (outside(x, y, z))
      return;
    map.set(bitIndex(x, y, z), v);
  }
  inline void set(const IPoint3 &p, bool v = true) { set(P3D(p), v); }

  void fill(int x0, int y0, int z0, int x1, int y1, int z1, bool v)
  {
    if (x0 > x1)
      eastl::swap(x0, x1);
    if (y0 > y1)
      eastl::swap(y0, y1);
    if (z0 > z1)
      eastl::swap(z0, z1);

    x0 = max(x0, 0);
    x1 = min(x1, size.x - 1);
    y0 = max(y0, 0);
    y1 = min(y1, size.y - 1);
    z0 = max(z0, 0);
    z1 = min(z1, size.z - 1);

    for (int y = y0; y <= y1; y++)
      for (int z = z0; z <= z1; z++)
      {
        uint32_t b0 = bitIndex(x0, y, z);
        uint32_t b1 = bitIndex(x1, y, z);
        map.fill(b0, b1, v);
      }
  }
  inline void fill(const IPoint3 &b0, const IPoint3 &b1, bool v) { fill(P3D(b0), P3D(b1), v); }

  void floodClear(int seed_x, int seed_y, int seed_z)
  {
    if (outside(seed_x, seed_y, seed_z))
      return;

    uint32_t numFilled = 0;
    eastl::deque<IPoint3> queue;
    queue.emplace_back(seed_x, seed_y, seed_z);

    while (!queue.empty())
    {
      auto [x, y, z] = queue.front();
      queue.pop_front();
      if (!map[bitIndex(x, y, z)])
        continue;

      int x0 = x;
      for (; x0 > 0 and map[bitIndex(x0 - 1, y, z)]; x0--) {}
      int x1 = x;
      for (; x1 + 1 < size.x and map[bitIndex(x1 + 1, y, z)]; x1++) {}

      map.fill(bitIndex(x0, y, z), bitIndex(x1, y, z), false);
      numFilled += x1 - x0 + 1;

      if (y > 0)
      {
        y--;
        for (x = x0; x <= x1; x++)
          if (map[bitIndex(x, y, z)])
          {
            queue.emplace_back(x, y, z);
            for (x++; x <= x1 and map[bitIndex(x, y, z)]; x++) {}
          }
        y++;
      }

      if (y + 1 < size.y)
      {
        y++;
        for (x = x0; x <= x1; x++)
          if (map[bitIndex(x, y, z)])
          {
            queue.emplace_back(x, y, z);
            for (x++; x <= x1 and map[bitIndex(x, y, z)]; x++) {}
          }
        y--;
      }

      if (z > 0)
      {
        z--;
        for (x = x0; x <= x1; x++)
          if (map[bitIndex(x, y, z)])
          {
            queue.emplace_back(x, y, z);
            for (x++; x <= x1 and map[bitIndex(x, y, z)]; x++) {}
          }
        z++;
      }

      if (z + 1 < size.z)
      {
        z++;
        for (x = x0; x <= x1; x++)
          if (map[bitIndex(x, y, z)])
          {
            queue.emplace_back(x, y, z);
            for (x++; x <= x1 and map[bitIndex(x, y, z)]; x++) {}
          }
        z--;
      }
    }
  }

  void clearOutside()
  {
    // yx
    for (int z : {0, size.z - 1})
      for (int y = 0; y < size.y; y++)
        for (int x = 0; x < size.x;)
        {
          bool v = map[bitIndex(x, y, z)];
          int x0 = x;
          if (v)
          {
            for (x++; x < size.x and map[bitIndex(x, y, z)]; x++) {}
            floodClear(x0, y, z);
          }
          for (x++; x < size.x and !map[bitIndex(x, y, z)]; x++) {}
        }
    // zx
    for (int y : {0, size.y - 1})
      for (int z = 0; z < size.z; z++)
        for (int x = 0; x < size.x;)
        {
          bool v = map[bitIndex(x, y, z)];
          int x0 = x;
          if (v)
          {
            for (x++; x < size.x and map[bitIndex(x, y, z)]; x++) {}
            floodClear(x0, y, z);
          }
          for (x++; x < size.x and !map[bitIndex(x, y, z)]; x++) {}
        }
    // yz
    for (int x : {0, size.x - 1})
      for (int y = 0; y < size.y; y++)
        for (int z = 0; z < size.z;)
        {
          bool v = map[bitIndex(x, y, z)];
          int z0 = z;
          if (v)
          {
            for (z++; z < size.z and map[bitIndex(x, y, z)]; z++) {}
            floodClear(x, y, z0);
          }
          for (z++; z < size.z and !map[bitIndex(x, y, z)]; z++) {}
        }
  }

  void save(mkbindump::BinDumpSaveCB &cwr) { cwr.writeTabData32e(make_span_const(map.getPtr(), map.dataSize() >> 2)); }
};


struct VoxelBakingFace
{
  using Pixel = voxelcache::Pixel;

  struct Coord
  {
    uint16_t x, y;
    float z;
  };

  Tab<Pixel> pixels;
  Tab<Coord> coords;
  Tab<uint32_t> sortedIndex;
  Tab<uint32_t> pixelIndex;
  uint16_t nx = 0, ny = 0;

  VoxelBakingFace() : pixels(tmpmem), coords(tmpmem), sortedIndex(tmpmem), pixelIndex(tmpmem) {}

  void init(uint32_t x, uint32_t y)
  {
    nx = x;
    ny = y;
    uint32_t num = x * y;
    pixels.reserve(num);
    coords.reserve(num);
  }

  uint32_t addPixel(uint32_t x, uint32_t y, float z)
  {
    G_ASSERT((x >> 16) == 0 and (y >> 16) == 0);
    uint32_t i = pixels.size();
    pixels.push_back();
    coords.emplace_back(x, y, z);
    return i;
  }

  void buildIndex()
  {
    G_ASSERT(pixels.size() == coords.size());
    sortedIndex.resize(coords.size());
    for (uint32_t i = 0, n = sortedIndex.size(); i < n; i++)
      sortedIndex[i] = i;

    eastl::sort(sortedIndex.begin(), sortedIndex.end(), [this](uint32_t ai, uint32_t bi) -> bool {
      auto a = coords[ai];
      auto b = coords[bi];
      return a.y == b.y ? (a.x == b.x ? a.z < b.z : a.x < b.x) : a.y < b.y;
    });

    pixelIndex.resize_noinit(nx * ny);
    uint32_t si = 0;
    for (uint32_t y = 0, pi = 0, ns = sortedIndex.size(); y < ny; y++)
      for (uint32_t x = 0; x < nx; x++, pi++)
      {
        pixelIndex[pi] = si;
        while (si < ns and coords[sortedIndex[si]].x == x and coords[sortedIndex[si]].y == y)
          si++;
      }
    G_ASSERT(si == sortedIndex.size());
  }

  void save(mkbindump::BinDumpSaveCB &cwr)
  {
    G_ASSERT(pixelIndex.size() == nx * ny);
    G_ASSERT(pixels.size() == coords.size());
    G_ASSERT(pixels.size() == sortedIndex.size());

    cwr.writeInt32e(nx);
    cwr.writeInt32e(ny);

    mkbindump::PatchTabRef indexTab, depthTab, pixelTab;
    indexTab.reserveTab(cwr);
    depthTab.reserveTab(cwr);
    pixelTab.reserveTab(cwr);

    indexTab.startData(cwr, pixelIndex.size() + 1);
    cwr.writeTabData32e(make_span_const(pixelIndex));
    cwr.writeInt32e(pixels.size());

    depthTab.startData(cwr, sortedIndex.size());
    for (uint32_t i : sortedIndex)
      cwr.writeFloat32e(coords[i].z);

    pixelTab.startData(cwr, sortedIndex.size());
    for (uint32_t i : sortedIndex)
      cwr.writeRaw(&pixels[i], sizeof(Pixel));

    indexTab.finishTab(cwr);
    depthTab.finishTab(cwr);
    pixelTab.finishTab(cwr);
  }
};


template <typename F>
static void dump_voxel_map(const char *filename, const IPoint3 &size, F &&func)
{
  uint32_t nx = max(uint32_t(sqrtf(size.y)), 1u);
  uint32_t ny = (size.y + nx - 1) / nx;
  eastl::unique_ptr<TexImage32, tmpmemDeleter> image(TexImage32::create((size.z + 1) * nx, (size.x + 1) * ny, tmpmem));
  auto pixels = image->getPixels();
  const uint32_t wd = image->w;
  uint32_t fx = 0;
  for (uint32_t y = 0; y < size.y; y++)
  {
    auto ptr = pixels + fx * (size.z + 1);
    for (uint32_t x = 0; x < size.x; x++, ptr += wd)
    {
      auto p = ptr;
      for (uint32_t z = 0; z < size.z; z++, p++)
      {
        p->u = 0;
        p->a = 255;
        func(*p, x, y, z);
      }
      p->u = 0;
    }
    memset(ptr, 0, (size.z + 1) * 4);

    fx++;
    if (fx >= nx)
    {
      fx = 0;
      pixels += wd * (size.x + 1);
    }
  }
  save_png32(filename, image->getPixels(), wd, image->h, wd * 4, nullptr, 0);
}


static uint32_t profile_lapse_usec(uint64_t &reft)
{
  auto t = profile_ref_ticks();
  auto dt = t - reft;
  reft = t;
  return profile_usec_from_ticks_delta(dt);
}


bool ImpostorBaker::exportVoxelCache(DagorAsset *asset, const ImpostorOptions &options)
{
  conlog("Baking voxel impostor for %s", asset->getName());
  auto reft = profile_ref_ticks();
  int resIdx = rendinst::addRIGenExtraResIdx(asset->getName(), -1, -1, rendinst::AddRIFlag::UseShadow);
  RenderableInstanceLodsResource *res = rendinst::getRIGenExtraRes(resIdx);
  if (res == nullptr)
  {
    conerror("Could not load rendInst: %s", asset->getName());
    return false;
  }

  auto vblk = asset->props.getBlockByName("voxel_impostor");
  if (!vblk)
    vblk = &defaultVoxelParams;

  float projScale = vblk->getReal("projScale", defaultVoxelParams.getReal("projScale", defaults::projScale));
  float triangleThreshold =
    vblk->getReal("triangleThreshold", defaultVoxelParams.getReal("triangleThreshold", defaults::triangleThreshold));
  float voxelSize = vblk->getReal("voxelSize", defaultVoxelParams.getReal("voxelSize", defaults::voxelSize));
  int maxMapSize = vblk->getInt("maxMapSize", defaultVoxelParams.getInt("maxMapSize", defaults::maxMapSize));
  conlog("Loaded RI in %d us", profile_lapse_usec(reft));

  // compute voxel/map size
  if (voxelSize <= 0)
  {
    voxelSize = computeVoxelSize(res, asset->getName(), projScale, triangleThreshold);
    conlog("Computed voxel size in %d us", profile_lapse_usec(reft));
    if (voxelSize <= 0)
    {
      conerror("Failed to compute voxel size for rendInst %s", asset->getName());
      return false;
    }
  }

  Point3 width = res->bbox.width();
  float maxBound = max(max(width.x, width.y), width.z);
  int maxSize = int(ceilf(maxBound / voxelSize));
  if (maxSize > maxMapSize)
  {
    float vs = maxBound / maxMapSize;
    conwarning("Clamped voxel size to %f (from %f) because of max map size %d, for rendInst %s", vs, voxelSize, maxMapSize,
      asset->getName());
    voxelSize = vs;
  }

  auto mapSize = ipoint3(ceil(width / voxelSize));
  conlog("Voxel map %dx%dx%d, voxel %f for rendInst %s", P3D(mapSize), voxelSize, asset->getName());

  const Point3 boxMin = res->bbox.boxMin();
  if (!beginVoxelBaking(res, mapSize, boxMin, voxelSize))
  {
    endVoxelBaking();
    conerror("Failed to bake voxels for rendInst %s", asset->getName());
    return false;
  }

  // allocate voxel maps
  VoxelBitmap solid(mapSize * VOXEL_OVERSCALE);
  carray<VoxelBakingFace, 6> surface;
  IBBox3 ibbox;

  int reqCounter = 0;
  eastl::vector<VoxelBakeRequest> requests(6);
  for (int i = 0; i < 6; i++)
  {
    requests[i].faceId = i;
    requests[i].offset = 0;

    uint32_t viewW, viewH, viewD;
    IPoint3 dirx, diry, dirz;
    getVoxelFaceParams(i, viewW, viewH, viewD, dirx, diry, dirz);
    surface[i].init(viewW * VOXEL_OVERSCALE, viewH * VOXEL_OVERSCALE);
  }

  int stepCounter = 0;
  while (!requests.empty() and stepCounter <= maxMapSize * VOXEL_OVERSCALE)
  {
    if (!bakeVoxels(make_span(requests)))
    {
      endVoxelBaking();
      conerror("Failed to bake voxels for rendInst %s", asset->getName());
      return false;
    }

    if (0)
    {
      // dump rendered images for debugging
      for (auto &r : requests)
      {
        save_png32(String(0, "%s_%02u_a_%c.png", asset->getName(), reqCounter, "LRBTKF"[r.faceId]), r.albedo->getPixels(), r.albedo->w,
          r.albedo->h, r.albedo->w * 4, nullptr, 0);
        save_png32(String(0, "%s_%02u_n_%c.png", asset->getName(), reqCounter, "LRBTKF"[r.faceId]), r.normal->getPixels(), r.normal->w,
          r.normal->h, r.normal->w * 4, nullptr, 0);
        save_png32(String(0, "%s_%02u_e_%c.png", asset->getName(), reqCounter, "LRBTKF"[r.faceId]), r.extra->getPixels(), r.extra->w,
          r.extra->h, r.extra->w * 4, nullptr, 0);
        reqCounter++;
      }
    }

    // process results
    for (int ri = requests.size() - 1; ri >= 0; ri--)
    {
      auto &r = requests[ri];
      auto &surf = surface[r.faceId];

      uint32_t viewW, viewH, viewD;
      IPoint3 dirx, diry, dirz;
      getVoxelFaceParams(r.faceId, viewW, viewH, viewD, dirx, diry, dirz);
      viewW *= VOXEL_OVERSCALE;
      viewH *= VOXEL_OVERSCALE;
      viewD *= VOXEL_OVERSCALE;
      IPoint3 origin = IPoint3::ZERO;
      if (abs(dirx) != dirx)
        origin -= dirx * (viewW - 1);
      if (abs(diry) != diry)
        origin -= diry * (viewH - 1);
      if (r.faceId & 1)
        origin += abs(dirz) * viewD;

      G_ASSERT(r.albedo->w == viewW and r.albedo->h == viewH);
      auto albedoPtr = r.albedo->getPixels();
      auto normalPtr = r.normal->getPixels();
      auto extraPtr = r.extra->getPixels();
      auto depthPtr = r.depth->getPixels();

      bool empty = true;
      for (int y = viewH - 1; y >= 0; y--)
      {
        for (uint32_t x = 0; x < viewW; x++, albedoPtr++, normalPtr++, extraPtr++, depthPtr++)
        {
          float d = *depthPtr - 1;
          if (d > viewD)
            continue;
          empty = false;

          int z = int(floorf(d)) + (r.faceId & 1);
          IPoint3 pos = dirx * x + diry * y + dirz * z + origin;

          const uint32_t pi =
            surf.addPixel(abs(dirx) * pos, abs(diry) * pos, dot(point3(dirz) * d + point3(origin), point3(abs(dirz))));
          auto &p = surf.pixels[pi];

          auto albedo = *albedoPtr;
          auto normal = *normalPtr;
          auto extra = *extraPtr;

          p.albedoR = albedo.r;
          p.albedoG = albedo.g;
          p.albedoB = albedo.b;
          p.alpha = albedo.a;

          p.normalX = normal.r;
          p.normalY = normal.g;
          p.normalZ = normal.b;
          p.coloring = normal.a;

          p.smoothness = extra.r;
          p.reflectance = extra.g;
          p.metalness = extra.b;
          p.translucency = extra.a;

          // debug("!!! step=%d face=%d (%d %d) d=%g z=%d (%d %d %d)", stepCounter, r.faceId, x, y, d, z, P3D(pos));
          solid.set(pos);
          ibbox += pos;
        }
      }

      if (0)
      {
        // dump voxel map for debugging
        dump_voxel_map(String(0, "_%s_%02u_3d.png", asset->getName(), stepCounter), solid.size,
          [&solid](auto &p, auto x, auto y, auto z) { p.g = solid.get(x, y, z) ? 255 : 0; });
      }

      if (empty)
        requests.erase(requests.begin() + ri);
    }

    stepCounter++;
    if (displayExportedImages)
      dagor_work_cycle(); // we expect that current scene will call drawLastExportedImage() from its drawScene()
  }

  endVoxelBaking();
  conlog("Baked voxels in %d us", profile_lapse_usec(reft));

  if (ibbox.isEmpty())
  {
    conerror("No voxels baked for rendInst %s, could be unsupported shaders", asset->getName());
    return false;
  }

  for (auto &s : surface)
    s.buildIndex();
  conlog("Built pixel index in %d us", profile_lapse_usec(reft));

  VoxelBitmap inside(solid);
  inside.map.bitNot();
  inside.clearOutside();
  conlog("Filled inside voxels in %d us", profile_lapse_usec(reft));

  if (0)
  {
    dump_voxel_map(String(0, "_%s_final.png", asset->getName()), solid.size, [&solid, &inside](auto &p, auto x, auto y, auto z) {
      p.g = solid.get(x, y, z) ? 255 : 0;
      p.r = inside.get(x, y, z) ? 255 : 0;
    });
  }

  String outFolder = getAssetFolder(asset);
  ::dd_mkpath(outFolder.c_str());

  String filename(0, "%s%s_%s", outFolder.c_str(), asset->getName(), voxelcache::VOXEL_CACHE_NAME);

  try
  {
    mkbindump::BinDumpSaveCB cwr(1 << 20, _MAKE4C('PC'), false);
    cwr.writeFloat64e(double(voxelSize) / VOXEL_OVERSCALE);
    cwr.writeFloat64e(voxelSize);
    cwr.writeInt32e(solid.size.x);
    cwr.writeInt32e(solid.size.y);
    cwr.writeInt32e(solid.size.z);
    cwr.writeFloat32e(boxMin.x);
    cwr.writeFloat32e(boxMin.y);
    cwr.writeFloat32e(boxMin.z);
    cwr.writeInt32e(ibbox[0].x);
    cwr.writeInt32e(ibbox[0].y);
    cwr.writeInt32e(ibbox[0].z);
    cwr.writeInt32e(ibbox[1].x);
    cwr.writeInt32e(ibbox[1].y);
    cwr.writeInt32e(ibbox[1].z);

    auto pointersOffset = cwr.tell();
    for (auto &s : surface)
      cwr.writePtr64e(0);

    mkbindump::PatchTabRef insideTab;
    insideTab.reserveTab(cwr);

    for (auto &s : surface)
    {
      cwr.writePtr64eAt(cwr.tell(), pointersOffset + 8 * (&s - surface.data()));
      s.save(cwr);
    }

    insideTab.startData(cwr, inside.map.dataSize() >> 2);
    inside.save(cwr);
    insideTab.finishTab(cwr);

    FullFileSaveCB fileIO(filename);
    fileIO.writeIntP<4>(voxelcache::VERSION); // version
    auto dumpSize = cwr.getSize();
    fileIO.writeIntP<4>(dumpSize);

    MemoryLoadCB memReader(cwr.getMem(), false);
    zstd_stream_compress_data(fileIO, memReader, dumpSize, ZSTD_COMPRESSION_LEVEL);
  }
  catch (IGenSave::SaveException)
  {
    conerror("error writing voxel cache file %s", filename.c_str());
    return false;
  }

  conlog("Saved voxel cache in %d us", profile_lapse_usec(reft));
  return true;
}
