// This file is to be directly included.
// Can be run on SPU.

#include <scene/dag_occlusion.h>
#include <perfMon/dag_statDrv.h>

#define CELL_ORIGIN    (this->origin)
#define NUM_CELLS_X    (this->mapSizeX)
#define NUM_CELLS_Y    (this->mapSizeY)
#define LAND_CELL_SIZE (this->landCellSize)
#define GRID_CELL_SIZE (this->gridCellSize)
#define VIS_RANGE      (this->visRange)
#define MESH_OFFSET    (this->landMeshOffset)

BBox3 LandMeshCullingState::getBBox(int x, int y, float *sphere_radius)
{
  x -= CELL_ORIGIN.x;
  y -= CELL_ORIGIN.y;
  if (x < 0 || x >= mapSizeX || y < 0 || y >= mapSizeY)
    return BBox3();
  int i = x + y * mapSizeX;
  if (sphere_radius)
    *sphere_radius = cellBoundingsRadius[i];
  return cellBoundings[i];
}

bool LandMeshCullingState::calcCellBox(LandMeshManager &provider, int borderX, int borderY, int x0, int y0, int x1, int y1, BBox3 &res)
{
  int mirrorX = mirror_x(borderX, x0, x1);
  int mirrorY = mirror_x(borderY, y0, y1);

  // Mirror cell number.
  if (provider.isInTools() && (mirrorX != borderX || mirrorY != borderY))
    return false;

  // Get world bbox.
  res = this->getBBox(mirrorX, mirrorY);

  if ((mirrorX != borderX) | (mirrorY != borderY))
  {
    float cellSize = LAND_CELL_SIZE;
    res.lim[0].x = cellSize * borderX - GRID_CELL_SIZE * (borderX / (x1 - x0 + 1));
    res.lim[0].z = cellSize * borderY - GRID_CELL_SIZE * (borderY / (y1 - y0 + 1));
    res.lim[1].x = res.lim[0].x + cellSize;
    res.lim[1].z = res.lim[0].z + cellSize;
  }
  return true;
}

void LandMeshCullingState::cullCell(LandMeshManager &provider, int borderX, int borderY, int x0, int y0, int x1, int y1,
  const Frustum &frustum, const Occlusion *occlusion, LandMeshCullingData &data)
{
  if (provider.isInTools() && useExclBox && borderX >= exclBox[0].x && borderY >= exclBox[0].y && borderX < exclBox[1].x &&
      borderY < exclBox[1].y)
    return;
  if (cullMode != NO_CULLING)
  {
    DECL_ALIGN16(BBox3, bbox);
    if (!calcCellBox(provider, borderX, borderY, x0, y0, x1, y1, bbox))
      return;

    if (!renderInBBox.isempty() && !(bbox & renderInBBox))
      return;

    // Test bbox vs. frustum.
    vec4f bmin = v_ld(&bbox[0].x);
    vec4f bmax = v_ldu(&bbox[1].x);
    vec4f center2 = v_add(bmax, bmin);
    vec4f extent2 = v_sub(bmax, bmin);
    if (!frustum.testBoxExtentB(center2, extent2))
      return;
    if (occlusion && occlusion->isOccludedBoxExtent2(center2, extent2))
      return;
  }
  int regioni;
  for (regioni = 0; regioni < data.regionsCount - 1; ++regioni)
    if (regions[regioni] & IPoint2(borderX, borderY))
      break;
  if (data.regions[regioni].head == LANDMESH_INVALID_CELL)
  {
    data.regions[regioni].head = data.count;
    data.regions[regioni].tail = data.count;
  }
  else
  {
    LandMeshCellDesc &tailDesc = data.cells[data.regions[regioni].tail];
    if (tailDesc.borderY == borderY && tailDesc.countY == 0 && tailDesc.borderX + tailDesc.countX + 1 == borderX)
    {
      G_ASSERT(tailDesc.countX < 255);
      tailDesc.countX++;
      return;
    }
    if (tailDesc.borderX == borderX && tailDesc.countX == 0 && tailDesc.borderY + tailDesc.countY + 1 == borderY)
    {
      G_ASSERT(tailDesc.countY < 255);
      tailDesc.countY++;
      return;
    }
  }
  G_ASSERTF(data.count < LANDMESH_MAX_CELLS, "data.count %d >= %d", data.count, LANDMESH_MAX_CELLS);
  if (data.count >= LANDMESH_MAX_CELLS)
    return;
  LandMeshCellDesc &desc = data.cells[data.count];
  data.cells[data.regions[regioni].tail].next = data.count;
  data.regions[regioni].tail = data.count;
  desc.borderX = borderX;
  desc.borderY = borderY;
  desc.countX = desc.countY = 0;
  desc.next = LANDMESH_INVALID_CELL;
  data.count++;
}

void LandMeshCullingState::cullDataWithBbox(LandMeshCullingData &dest_data, const LandMeshCullingData &src_data, const BBox2 &bbox)
{
  IBBox2 ibbox;
  float cellSize = LAND_CELL_SIZE;
  Point3 meshOffset = MESH_OFFSET;
  ibbox[0].x = (int)floorf((bbox[0].x - meshOffset.x) / cellSize);
  ibbox[1].x = (int)ceilf((bbox[1].x - meshOffset.x) / cellSize);
  ibbox[0].y = (int)floorf((bbox[0].y - meshOffset.z) / cellSize);
  ibbox[1].y = (int)ceilf((bbox[1].y - meshOffset.z) / cellSize);

  dest_data.count = 0;
  dest_data.regionsCount = src_data.regionsCount;
  for (int regioni = 0; regioni < dest_data.regionsCount; ++regioni)
    dest_data.regions[regioni].head = LANDMESH_INVALID_CELL;

  for (int regioni = 0; regioni < src_data.regionsCount; ++regioni)
  {
    if (src_data.regions[regioni].head == LANDMESH_INVALID_CELL)
      continue;
    for (int i = src_data.regions[regioni].head; i != LANDMESH_INVALID_CELL; i = src_data.cells[i].next)
    {
      if (ibbox & IPoint2(src_data.cells[i].borderX, src_data.cells[i].borderY))
      {
        LandMeshCellDesc &desc = dest_data.cells[dest_data.count];
        dest_data.cells[dest_data.regions[regioni].tail].next = dest_data.count;
        dest_data.regions[regioni].tail = dest_data.count;
        desc = src_data.cells[i];
        desc.next = LANDMESH_INVALID_CELL;
        dest_data.count++;
      }
    }
  }
}


void LandMeshCullingState::frustumCulling(LandMeshManager &provider, LandMeshCullingData &data, const IBBox2 *regions_array,
  int regions_count, const HeightmapFrustumCullingInfo &fi)
{
  TIME_PROFILE(lmesh_frustum_cull);

  int x0, x1, y0, y1;
  IPoint2 startCell;
  IPoint2 lt, rb;

  lt.x = CELL_ORIGIN.x;
  lt.y = CELL_ORIGIN.y;
  rb.x = lt.x + NUM_CELLS_X - 1;
  rb.y = lt.y + NUM_CELLS_Y - 1;

  float cellSize = LAND_CELL_SIZE;
  if (!renderInBBox.isempty())
  {
    Point3 meshOffset = MESH_OFFSET;
    x0 = (int)floorf((renderInBBox[0].x - meshOffset.x) / cellSize);
    x1 = (int)floorf((renderInBBox[1].x - meshOffset.x) / cellSize);
    y0 = (int)floorf((renderInBBox[0].z - meshOffset.z) / cellSize);
    y1 = (int)floorf((renderInBBox[1].z - meshOffset.z) / cellSize);
    startCell = IPoint2((x0 + x1) / 2, (y0 + y1) / 2);
  }
  else
  {
    vec4f cellSizeV = v_splats(cellSize);
    vec4f invalid, invalid2;
    vec4f farplanepos = three_plane_intersection(fi.frustum.camPlanes[Frustum::FARPLANE], fi.frustum.camPlanes[Frustum::TOP],
      fi.frustum.camPlanes[Frustum::LEFT], invalid);
    G_ASSERT(v_test_vec_x_eqi_0(invalid));

    vec4f far_dist = v_abs(v_plane_dist(fi.frustum.camPlanes[Frustum::NEARPLANE], farplanepos));
    vec4f camerapos = three_plane_intersection(fi.frustum.camPlanes[Frustum::RIGHT], fi.frustum.camPlanes[Frustum::TOP],
      fi.frustum.camPlanes[Frustum::LEFT], invalid);

    far_dist = v_sel(v_length3(v_sub(camerapos, farplanepos)), far_dist, invalid);

    vec4f farplanepos2 = three_plane_intersection(fi.frustum.camPlanes[Frustum::FARPLANE], fi.frustum.camPlanes[Frustum::TOP],
      fi.frustum.camPlanes[Frustum::RIGHT], invalid2);
    G_ASSERT(v_test_vec_x_eqi_0(invalid2));
    // implicit assumption, that rendering raius is maximum of two: distance from znear plane to zfar/left/top,
    //   and 0.5*length(zfar/left/top, zfar/right/top) (since we are more likely to half wide resoultion).
    //   can be easily expanded to additional max(0.5*length(zfar/left/top, zfar/left/bottom))

    far_dist = v_max(far_dist, v_mul(v_length3_x(v_sub(farplanepos, farplanepos2)), V_C_HALF));
    far_dist = v_div_x(far_dist, cellSizeV);
    vec4i maxDistV = v_cvt_ceili(far_dist);
    // invalid = v_or(invalid, invalid2);
    // maxDistV = v_or(maxDistV, v_cast_vec4i(v_or(invalid, invalid2)));
    int max_dist_cells = v_extract_xi(maxDistV);

    int vis_range = VIS_RANGE;
    if (vis_range < 0)
      vis_range = max_dist_cells;
    else
      vis_range = min(max_dist_cells, vis_range);
    if (vis_range < 0)
    {
      x0 = lt.x - numBorderCellsXNeg;
      x1 = rb.x + numBorderCellsXPos;
      y0 = lt.y - numBorderCellsZNeg;
      y1 = rb.y + numBorderCellsZPos;
      startCell = centerCell;
    }
    else
    {
      x0 = centerCell.x - vis_range;
      x1 = centerCell.x + vis_range;

      y0 = centerCell.y - vis_range;
      y1 = centerCell.y + vis_range;
      startCell = centerCell;
    }
  }

  IBBox2 cellBox;
  cellBox[0] = IPoint2(max(x0, lt.x - numBorderCellsXNeg), max(y0, lt.y - numBorderCellsZNeg));
  cellBox[1] = IPoint2(min(x1, rb.x + numBorderCellsXPos), min(y1, rb.y + numBorderCellsZPos));
  int maxRadius =
    max(max(startCell.x - cellBox[0].x, cellBox[1].x - startCell.x), max(startCell.y - cellBox[0].y, cellBox[1].y - startCell.y));

  data.count = 0;
  data.regionsCount = regions_array && regions_count ? regions_count : 1;
  if (cellBox.lim[1].x < cellBox.lim[0].x || cellBox.lim[1].y < cellBox.lim[0].y)
    return;

  G_ASSERT(regions_count <= MAX_LAND_MESH_REGIONS);
  memcpy(regions, regions_array, sizeof(IBBox2) * regions_count);
  for (int regioni = 0; regioni < data.regionsCount; ++regioni)
    data.regions[regioni].head = LANDMESH_INVALID_CELL;

  if (cullMode == NO_CULLING)
  {
    // todo: if (data.regionsCount == 1) we can add one huge cell at cellBox.lim[0] with width of cellBox
    if (data.regionsCount == 1)
    {
      data.regions[0].head = 0;
      data.regions[0].tail = 0;
      LandMeshCellDesc &desc = data.cells[0];
      desc.borderX = cellBox.lim[0].x;
      desc.borderY = cellBox.lim[0].y;
      desc.countX = (cellBox.lim[1].x - cellBox.lim[0].x);
      desc.countY = (cellBox.lim[1].y - cellBox.lim[0].y);
      desc.next = LANDMESH_INVALID_CELL;
      data.count = 1;
    }
    else
    {
      for (int y = cellBox.lim[0].y; y <= cellBox.lim[1].y; y++)
        for (int x = cellBox.lim[0].x; x <= cellBox.lim[1].x; x++)
          cullCell(provider, x, y, lt.x, lt.y, rb.x, rb.y, fi.frustum, fi.occlusion, data);
    }
  }
  else
  {
    // front-to-back sorting and cullingMng
    if (!provider.isInTools() && useExclBox && provider.getHmapHandler() && fi.min_tank_lod >= 0)
    {
      provider.getHmapHandler()->frustumCulling(data.heightmapData, fi);
    }
    startCell.x = max(cellBox.lim[0].x, min(cellBox.lim[1].x, startCell.x));
    startCell.y = max(cellBox.lim[0].y, min(cellBox.lim[1].y, startCell.y));
    cullCell(provider, startCell.x, startCell.y, lt.x, lt.y, rb.x, rb.y, fi.frustum, fi.occlusion, data);

    for (int radius = 1; radius <= maxRadius; ++radius)
    {
      int minX = startCell.x - radius, maxX = startCell.x + radius;
      int minY = startCell.y - radius, maxY = startCell.y + radius;

      if (minY >= cellBox.lim[0].y && minY <= cellBox.lim[1].y)
        for (int x = max(minX, cellBox.lim[0].x); x <= min(maxX, cellBox.lim[1].x); x++)
          cullCell(provider, x, minY, lt.x, lt.y, rb.x, rb.y, fi.frustum, fi.occlusion, data);

      if (maxY <= cellBox.lim[1].y && maxY >= cellBox.lim[0].y)
        for (int x = max(minX, cellBox.lim[0].x); x <= min(maxX, cellBox.lim[1].x); x++)
          cullCell(provider, x, maxY, lt.x, lt.y, rb.x, rb.y, fi.frustum, fi.occlusion, data);

      if (minX >= cellBox.lim[0].x && minX <= cellBox.lim[1].x)
        for (int y = max(minY + 1, cellBox.lim[0].y); y <= min(maxY - 1, cellBox.lim[1].y); y++)
          cullCell(provider, minX, y, lt.x, lt.y, rb.x, rb.y, fi.frustum, fi.occlusion, data);

      if (maxX <= cellBox.lim[1].x && maxX >= cellBox.lim[0].x)
        for (int y = max(minY + 1, cellBox.lim[0].y); y <= min(maxY - 1, cellBox.lim[1].y); y++)
          cullCell(provider, maxX, y, lt.x, lt.y, rb.x, rb.y, fi.frustum, fi.occlusion, data);
    }
  }
}


void LandMeshCullingState::copyLandmeshState(LandMeshManager &provider, LandMeshRenderer &renderer)
{
  // LandMeshManager state
  cellBoundings = &(provider.cellBoundings[0]);
  cellBoundingsCount = provider.cellBoundings.size();
  cellBoundingsRadius = &(provider.cellBoundingsRadius[0]);
  cellBoundingsRadiusCount = provider.cellBoundingsRadius.size();

  origin = provider.origin;
  mapSizeX = provider.mapSizeX;
  mapSizeY = provider.mapSizeY;

  landCellSize = provider.landCellSize;
  gridCellSize = provider.gridCellSize;

  cullMode = provider.cullingState.cullMode;
  landMeshOffset = provider.getOffset();
  exclBox = provider.cullingState.exclBox;
  useExclBox = provider.cullingState.useExclBox;


  // LandMeshManager state
  visRange = provider.getVisibilityRangeCells();
  if (visRange > 0)
    visRange *= renderer.scaleVisRange;


  // LandMeshRenderer state
  centerCell = renderer.centerCell;

  numBorderCellsXPos = renderer.numBorderCellsXPos;
  numBorderCellsXNeg = renderer.numBorderCellsXNeg;
  numBorderCellsZPos = renderer.numBorderCellsZPos;
  numBorderCellsZNeg = renderer.numBorderCellsZNeg;

  renderInBBox = renderer.renderInBBox;
}
