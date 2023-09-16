#include <covers/covers.h>
#include <ioSys/dag_lzmaIo.h>
#include <ioSys/dag_zstdIo.h>
#include <debug/dag_debug3d.h>

namespace covers
{
mat44f Cover::calcTm() const
{
  vec3f grL = v_ldu(&groundLeft.x);
  vec3f grR = v_ldu(&groundRight.x);
  vec3f invDir = v_neg(v_ldu(&dir.x));
  vec3f height = v_make_vec4f(0.f, (hLeft + hRight) * 0.5f, 0.f, 0.f);

  bbox3f coverBox;
  v_bbox3_init(coverBox, grR);
  v_bbox3_add_pt(coverBox, grL);
  v_bbox3_add_pt(coverBox, v_add(grL, invDir));
  v_bbox3_add_pt(coverBox, v_add(grR, invDir));
  v_bbox3_add_pt(coverBox, v_add(grL, height));
  v_bbox3_add_pt(coverBox, v_add(grR, height));

  mat44f m;
  m.col0 = v_sub(grR, grL);
  m.col1 = height;
  m.col2 = invDir;
  m.col3 = v_bbox3_center(coverBox);
  return m;
}

bool load(IGenLoad &crd, Tab<Cover> &covers_out)
{
  clear_and_shrink(covers_out);

  unsigned dataSize = crd.readInt();
  unsigned fmt = 0;
  if ((dataSize & 0xC0000000) == 0x40000000)
  {
    fmt = 1;
    dataSize &= ~0xC0000000;
  }

  if (dataSize % sizeof(Cover))
  {
    logerr_ctx("Could not load covers: broken data: data size = %d, cover size = %d", dataSize, sizeof(Cover));
    return false;
  }
  if (!dataSize)
  {
    logerr_ctx("Could not load covers: empty data");
    return false;
  }

  IGenLoad *dcrd = (fmt == 1) ? (IGenLoad *)new (alloca(sizeof(ZstdLoadCB)), _NEW_INPLACE) ZstdLoadCB(crd, crd.getBlockRest())
                              : (IGenLoad *)new (alloca(sizeof(LzmaLoadCB)), _NEW_INPLACE) LzmaLoadCB(crd, crd.getBlockRest());

  covers_out.resize(dataSize / sizeof(Cover));
  dcrd->read(covers_out.data(), dataSize);
  dcrd->ceaseReading();
  dcrd->~IGenLoad();
  return true;
}

bool build(float max_tile_size, uint32_t split, const Tab<Cover> &covers, scene::TiledScene &scene_out)
{
  scene_out.init(10);
  scene_out.reserve(covers.size());
  bbox3f pool;
  pool.bmin = v_neg(V_C_HALF);
  pool.bmax = V_C_HALF;
  scene_out.setPoolBBox(0, pool);

  bbox3f totalSpace;
  v_bbox3_init_empty(totalSpace);
  for (int i = 0; i < covers.size(); i++)
  {
    const Cover &cover = covers[i];

    mat44f m = cover.calcTm();
    v_bbox3_add_pt(totalSpace, m.col3);

    scene::node_index idx = scene_out.allocate(m, 0 /*pool*/, 0 /*flags*/);
    if (scene_out.getNodeIndex(idx) != i)
    {
      logerr_ctx("Could not build covers: node_index %d(%d) != index %d", scene_out.getNodeIndex(idx), idx, i);
      scene_out.destroy(idx);
      return false;
    }
  }

  vec3f size = v_bbox3_size(totalSpace);
  size = v_max(size, v_perm_zwxy(size));
  float sz = v_extract_x(size);
  const float newTileSize = min(max_tile_size, sz / split);
  scene_out.rearrange(newTileSize);
  return true;
}


void draw_cover(const Cover &cover, uint8_t alpha)
{
  Point3 groundLeft = cover.groundLeft;
  Point3 groundRight = cover.groundRight;

  Point3 topLeft = groundLeft;
  Point3 topRight = groundRight;
  topLeft.y += cover.hLeft;
  topRight.y += cover.hRight;

  const E3DCOLOR mainColor(0, 0, 200, alpha);
  const E3DCOLOR nopeColor(200, 0, 0, alpha);
  const E3DCOLOR shootColor(200, 200, 0, alpha);

  draw_cached_debug_line(groundLeft, groundRight, mainColor);
  draw_cached_debug_line(topLeft, topRight, mainColor);

  draw_cached_debug_line(topLeft, groundLeft, cover.hasLeftPos ? mainColor : nopeColor);
  draw_cached_debug_line(topRight, groundRight, cover.hasRightPos ? mainColor : nopeColor);

  const bool isShootCover = cover.hasLeftPos || cover.hasRightPos;
  const bool isHalfCover = !isShootCover && cover.shootLeft != ZERO<Point3>();
  if (!isHalfCover)
  {
    draw_cached_debug_line(topLeft, groundRight, mainColor);
    draw_cached_debug_line(topRight, groundLeft, mainColor);

    if (cover.hasLeftPos)
    {
      Point3 pt1 = topLeft + (topRight - topLeft) * 0.05f;
      Point3 pt2 = groundLeft + (groundRight - groundLeft) * 0.05f;
      draw_cached_debug_line(pt1, pt2, mainColor);
    }

    if (cover.hasRightPos)
    {
      Point3 pt1 = topRight + (topLeft - topRight) * 0.05f;
      Point3 pt2 = groundRight + (groundLeft - groundRight) * 0.05f;
      draw_cached_debug_line(pt1, pt2, mainColor);
    }
  }
  else
  {
    groundLeft.y += cover.hLeft * 0.3f;
    topLeft.y -= cover.hLeft * 0.3f;
    groundRight.y += cover.hRight * 0.3f;
    topRight.y -= cover.hRight * 0.3f;
    draw_cached_debug_line(topLeft, topRight, mainColor);
    draw_cached_debug_line(groundLeft, groundRight, mainColor);
  }

  if (cover.hasLeftPos)
    draw_cached_debug_line(cover.shootLeft, cover.shootLeft + Point3(0.f, 2.f, 0.f), shootColor);
  if (cover.hasRightPos)
    draw_cached_debug_line(cover.shootRight, cover.shootRight + Point3(0.f, 2.f, 0.f), shootColor);
}

void draw_debug(const Tab<Cover> &coversList)
{
  for (const auto &cover : coversList)
    draw_cover(cover, 255);
}

void draw_debug(const scene::TiledScene &scene)
{
  BBox3 box(Point3(-0.5, -0.5, -0.5), Point3(0.5, 0.5, 0.5));
  for (scene::node_index ni : scene)
  {
    alignas(16) TMatrix tm;
    v_mat_43ca_from_mat44(tm.m[0], scene.getNode(ni));
    set_cached_debug_lines_wtm(tm);
    draw_cached_debug_box(box, E3DCOLOR(0, 200, 0, 255));
  }
  set_cached_debug_lines_wtm(TMatrix::IDENT);
}

void draw_debug(const scene::TiledScene &scene, mat44f_cref globtm)
{
  BBox3 box(Point3(-0.5, -0.5, -0.5), Point3(0.5, 0.5, 0.5));
  vec4f pos_distscale = v_make_vec4f(0.f, 0.f, 0.f, 1.f);
  scene.frustumCull<false, true, false>(globtm, pos_distscale, 0, 0, nullptr, [&](scene::node_index, mat44f_cref m, vec4f) {
    alignas(16) TMatrix tm;
    v_mat_43ca_from_mat44(tm.m[0], m);
    set_cached_debug_lines_wtm(tm);
    draw_cached_debug_box(box, E3DCOLOR(0, 200, 0, 255));
  });
  set_cached_debug_lines_wtm(TMatrix::IDENT);
}

void draw_debug(const Tab<Cover> &covers, const scene::TiledScene &scene, mat44f_cref globtm, Point3 *viewPos, float distSq,
  uint8_t alpha)
{
  BBox3 box(Point3(-0.5, -0.5, -0.5), Point3(0.5, 0.5, 0.5));
  vec4f pos_distscale =
    viewPos ? v_make_vec4f(viewPos->x, viewPos->y, viewPos->z, scene::defaultDisappearSq / distSq) : v_make_vec4f(0.f, 0.f, 0.f, 1.f);
  scene.frustumCull<false, true, false>(globtm, pos_distscale, 0, 0, nullptr, [&](scene::node_index idx, mat44f_cref m, vec4f) {
    alignas(16) TMatrix tm;
    v_mat_43ca_from_mat44(tm.m[0], m);
    set_cached_debug_lines_wtm(tm);
    draw_cached_debug_box(box, E3DCOLOR(0, 200, 0, alpha));
    set_cached_debug_lines_wtm(TMatrix::IDENT);
    const Cover &cover = covers[scene.getNodeIndex(idx)];
    if (lengthSq(cover.visibleBox.width()) < 10000)
      draw_cached_debug_box(cover.visibleBox, E3DCOLOR(200, 200, 0, alpha / 2));
    draw_cover(cover, alpha);
  });
}
} // namespace covers
