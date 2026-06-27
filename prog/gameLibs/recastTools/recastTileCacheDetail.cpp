// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include <recastTools/recastTileCacheDetail.h>

#include <Recast.h>
#include <RecastAlloc.h>
#include <string.h>

namespace recastnavmesh
{

namespace
{

struct NonOwningPolyMesh : public rcPolyMesh
{
  // This is just a view. Detour owns the memory.
  ~NonOwningPolyMesh()
  {
    verts = nullptr;
    polys = nullptr;
    regs = nullptr;
    flags = nullptr;
    areas = nullptr;
  }
};

static rcCompactHeightfield *build_compact_heightfield_from_layer(const dtTileCacheLayer &layer, const dtNavMeshCreateParams &params)
{
  const dtTileCacheLayerHeader *header = layer.header;
  if (!header)
    return nullptr;

  const int width = header->width;
  const int height = header->height;
  const int cellCount = width * height;
  int spanCount = 0;
  for (int i = 0; i < cellCount; ++i)
    if (layer.areas[i] != DT_TILECACHE_NULL_AREA)
      ++spanCount;

  if (!spanCount)
    return nullptr;

  // Rebuild the heightfield subset needed by rcBuildPolyMeshDetail from the compressed layer.
  rcCompactHeightfield *chf = rcAllocCompactHeightfield();
  if (!chf)
    return nullptr;

  chf->width = width;
  chf->height = height;
  chf->spanCount = spanCount;
  chf->walkableHeight = (int)params.walkableHeight;
  chf->walkableClimb = (int)params.walkableClimb;
  chf->borderSize = 0;
  chf->maxDistance = 0;
  chf->maxRegions = 1;
  rcVcopy(chf->bmin, params.bmin);
  rcVcopy(chf->bmax, params.bmax);
  chf->cs = params.cs;
  chf->ch = params.ch;

  chf->cells = (rcCompactCell *)rcAlloc(sizeof(rcCompactCell) * cellCount, RC_ALLOC_PERM);
  chf->spans = (rcCompactSpan *)rcAlloc(sizeof(rcCompactSpan) * spanCount, RC_ALLOC_PERM);
  chf->areas = (unsigned char *)rcAlloc(sizeof(unsigned char) * spanCount, RC_ALLOC_PERM);
  if (!chf->cells || !chf->spans || !chf->areas)
  {
    rcFreeCompactHeightfield(chf);
    return nullptr;
  }
  memset(chf->cells, 0, sizeof(rcCompactCell) * cellCount);
  memset(chf->spans, 0, sizeof(rcCompactSpan) * spanCount);
  memset(chf->areas, 0, sizeof(unsigned char) * spanCount);

  int spanIdx = 0;
  for (int y = 0; y < height; ++y)
  {
    for (int x = 0; x < width; ++x)
    {
      const int idx = x + y * width;
      rcCompactCell &cell = chf->cells[idx];
      if (layer.areas[idx] == DT_TILECACHE_NULL_AREA)
      {
        cell.index = spanIdx;
        cell.count = 0;
        continue;
      }

      cell.index = spanIdx;
      cell.count = 1;

      rcCompactSpan &span = chf->spans[spanIdx];
      span.y = layer.heights[idx];
      span.reg = 1;
      span.h = 1;
      for (int dir = 0; dir < 4; ++dir)
        rcSetCon(span, dir, RC_NOT_CONNECTED);
      chf->areas[spanIdx] = layer.areas[idx];
      ++spanIdx;
    }
  }

  for (int y = 0; y < height; ++y)
  {
    for (int x = 0; x < width; ++x)
    {
      const int idx = x + y * width;
      const rcCompactCell &cell = chf->cells[idx];
      if (!cell.count)
        continue;

      rcCompactSpan &span = chf->spans[cell.index];
      const unsigned char con = layer.cons[idx] & 0x0f;
      for (int dir = 0; dir < 4; ++dir)
      {
        if ((con & (1 << dir)) == 0)
          continue;
        const int nx = x + rcGetDirOffsetX(dir);
        const int ny = y + rcGetDirOffsetY(dir);
        if ((unsigned)nx >= (unsigned)width || (unsigned)ny >= (unsigned)height)
          continue;
        const rcCompactCell &nei = chf->cells[nx + ny * width];
        if (nei.count)
          rcSetCon(span, dir, 0);
      }
    }
  }

  return chf;
}

static void fill_poly_mesh_view(NonOwningPolyMesh &mesh, const dtNavMeshCreateParams &params, Tab<unsigned short> &regs,
  float maxEdgeError)
{
  regs.resize(params.polyCount);
  for (int i = 0; i < params.polyCount; ++i)
    regs[i] = 1;

  mesh.verts = const_cast<unsigned short *>(params.verts);
  mesh.polys = const_cast<unsigned short *>(params.polys);
  mesh.regs = regs.data();
  mesh.flags = const_cast<unsigned short *>(params.polyFlags);
  mesh.areas = const_cast<unsigned char *>(params.polyAreas);
  mesh.nverts = params.vertCount;
  mesh.npolys = params.polyCount;
  mesh.maxpolys = params.polyCount;
  mesh.nvp = params.nvp;
  rcVcopy(mesh.bmin, params.bmin);
  rcVcopy(mesh.bmax, params.bmax);
  mesh.cs = params.cs;
  mesh.ch = params.ch;
  mesh.borderSize = 0;
  mesh.maxEdgeError = maxEdgeError;
}

} // namespace

TileCacheDetailStorage::~TileCacheDetailStorage() { clear(); }

void TileCacheDetailStorage::clear()
{
  rcFreeCompactHeightfield(chf);
  chf = nullptr;
  rcFreePolyMeshDetail(dmesh);
  dmesh = nullptr;
  clear_and_shrink(regs);
}

void TileCacheDetailStorage::apply(dtNavMeshCreateParams &params) const
{
  if (!dmesh)
    return;

  params.detailMeshes = dmesh->meshes;
  params.detailVerts = dmesh->verts;
  params.detailVertsCount = dmesh->nverts;
  params.detailTris = dmesh->tris;
  params.detailTriCount = dmesh->ntris;
}

bool TileCacheDetailStorage::build(const dtTileCacheLayer &layer, const dtNavMeshCreateParams &params, float max_edge_error,
  float detail_sample_dist, float detail_sample_max_error)
{
  rcContext ctx(false);
  return build(ctx, layer, params, max_edge_error, detail_sample_dist, detail_sample_max_error);
}

bool TileCacheDetailStorage::build(rcContext &ctx, const dtTileCacheLayer &layer, const dtNavMeshCreateParams &params,
  float max_edge_error, float detail_sample_dist, float detail_sample_max_error)
{
  clear();

  chf = build_compact_heightfield_from_layer(layer, params);
  if (!chf)
    return false;

  NonOwningPolyMesh pmesh;
  fill_poly_mesh_view(pmesh, params, regs, max_edge_error);

  dmesh = rcAllocPolyMeshDetail();
  if (!dmesh)
  {
    clear();
    return false;
  }

  if (!rcBuildPolyMeshDetail(&ctx, pmesh, *chf, detail_sample_dist, detail_sample_max_error, *dmesh))
  {
    clear();
    return false;
  }

  return true;
}

} // namespace recastnavmesh
