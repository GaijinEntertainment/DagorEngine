// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include <debug/dag_debug3d.h>
#include <debug/dag_debug3dStates.h>
#include <drv/3d/dag_draw.h>
#include <drv/3d/dag_matricesAndPerspective.h>
#include <drv/3d/dag_shaderConstants.h>
#include <drv/3d/dag_shader.h>
#include <drv/3d/dag_driver.h>
#include <drv/3d/dag_platform.h>
#include <3d/dag_render.h>
#include <math/dag_geomTree.h>
#include <math/dag_capsule.h>
#include <math/dag_TMatrix4more.h>
#include <shaders/dag_shaderBlock.h>
#include <shaders/dag_shaders.h>
#include <generic/dag_carray.h>
#include <math/integer/dag_IPoint2.h>
#include <math/dag_frustum.h>
#include <dag/dag_vector.h>

#if DAGOR_DBGLEVEL < 1 && !FORCE_LINK_DEBUG_LINES

void draw_cached_debug_solid_box(const mat43f *__restrict, const bbox3f *__restrict, int, E3DCOLOR) {}
void draw_cached_debug_quad(vec4f &, vec4f &, vec4f &, vec4f &, E3DCOLOR) {}
void draw_debug_frustum(const Frustum &, E3DCOLOR) {}
void draw_cached_debug_proj_matrix(const TMatrix4 &, E3DCOLOR, E3DCOLOR, E3DCOLOR) {}
void begin_draw_cached_debug_lines(bool, bool, bool) {}
bool begin_draw_cached_debug_lines_ex() { return false; }
void set_cached_debug_lines_wtm(const TMatrix &) {}
void end_draw_cached_debug_lines() {}
void end_draw_cached_debug_lines_ex() {}

void draw_cached_debug_line(const Point3 &, const Point3 &, E3DCOLOR) {}
void draw_cached_debug_line_twocolored(const Point3 &, const Point3 &, E3DCOLOR, E3DCOLOR) {}
bool init_draw_cached_debug_twocolored_shader() { return false; }
void close_draw_cached_debug() {}
void draw_cached_debug_line(const Point3 *, int, E3DCOLOR) {}
void draw_cached_matrix_axis(const TMatrix &, float) {}
void draw_cached_debug_box(const BBox3 &, E3DCOLOR) {}
void draw_cached_debug_box(const Point3 &, const Point3 &, const Point3 &, const Point3 &, E3DCOLOR) {}
void draw_cached_debug_box(const BBox3 &, E3DCOLOR, const TMatrix &) {}
void draw_cached_debug_sphere(const Point3 &, real, E3DCOLOR, int) {}
void draw_cached_debug_circle(const Point3 &, const Point3 &, const Point3 &, real, E3DCOLOR, int) {}
void draw_skeleton_link(const Point3 &, real, E3DCOLOR) {}
void draw_skeleton_tree(const GeomNodeTree &, real, E3DCOLOR, E3DCOLOR) {}
void draw_cached_debug_capsule_w(const Point3 &, const Point3 &, float, E3DCOLOR) {}
void draw_cached_debug_capsule_w(const Capsule &, E3DCOLOR) {}
void draw_cached_debug_capsule_l(const Capsule &, E3DCOLOR) {}
void draw_cached_debug_capsule(const Capsule &, E3DCOLOR, const TMatrix &) {}
void draw_cached_debug_cylinder(const TMatrix &, float, float, E3DCOLOR) {}
void draw_cached_debug_cylinder(const TMatrix &, Point3 &, Point3 &, float, E3DCOLOR) {}
void draw_cached_debug_cylinder_w(Point3 &, Point3 &, float, E3DCOLOR) {}
void draw_cached_debug_cone(const Point3 &, const Point3 &, real, E3DCOLOR, int) {}

void draw_cached_debug_trilist(const Point3 *, int, E3DCOLOR) {}
void draw_cached_debug_hex(const TMatrix &, const Point3 &, real, E3DCOLOR) {}
void draw_cached_debug_quad(const Point3[4], E3DCOLOR) {}
void draw_cached_debug_solid_triangle(const Point3[3], E3DCOLOR) {}

#else

static constexpr int MAX_BLOCKS_TO_REMEMBER = 32;

static carray<IPoint2, MAX_BLOCKS_TO_REMEMBER> shader_blocks;
static int current_block = 0;

static void push_blocks()
{
  if (current_block >= MAX_BLOCKS_TO_REMEMBER)
  {
    logerr("current_block=%d", current_block);
    return;
  }
  int frame_block = ShaderGlobal::getBlock(ShaderGlobal::LAYER_FRAME);
  int scene_block = ShaderGlobal::getBlock(ShaderGlobal::LAYER_SCENE);
  shader_blocks[current_block][0] = frame_block;
  shader_blocks[current_block][1] = scene_block;
  current_block++;
  ShaderGlobal::setBlock(-1, ShaderGlobal::LAYER_FRAME);
}

static void pop_blocks()
{
  ShaderElement::invalidate_cached_state_block(); // reset cache since we changed d3d states/constants directly
  if (!current_block)
    return;
  current_block--;
  ShaderGlobal::setBlock(shader_blocks[current_block][0], ShaderGlobal::LAYER_FRAME);
  ShaderGlobal::setBlock(shader_blocks[current_block][1], ShaderGlobal::LAYER_SCENE);
}


struct CachedDebugLinesData
{
  struct Vertex
  {
    Point3 p;
    E3DCOLOR c;
    Vertex() = default;
    Vertex(const Point3 &p, E3DCOLOR c) : p(p), c(c) {}
  };

  enum
  {
    MAX_LINES = 16 << 10,
    MAX_TRIANGLES = 16 << 10
  };

  dag::Vector<Vertex> lineVertices;
  int numLines = 0;
  dag::Vector<Vertex> triangleVertices;
  int numTriangles = 0;

  dag::Vector<Vertex> lineVertices2Colored;
  int numLines2Colored = 0;
  Ptr<ShaderMaterial> mat2Colored;
  Ptr<ShaderElement> elem2Colored;

  TMatrix wtm;
  TMatrix wtm_prev;
  bool testZ, writeZ, zFuncLess;
  int started = 0;
  debug3d::CachedStates stateMap;

  CachedDebugLinesData()
  {
    wtm.identity();
    wtm_prev.identity();
    testZ = writeZ = true;
    zFuncLess = false;
  }

  void setStates()
  {
    debug3d::StateKey stateKey(testZ, writeZ, zFuncLess ? debug3d::ZFUNC_LESS : debug3d::ZFUNC_GREATER_EQUAL);
    stateMap.setState(stateKey);

    d3d::set_program(d3d::get_debug_program());

    TMatrix4_vec4 gtm;
    d3d::settm(TM_WORLD, &Matrix44::IDENT);
    d3d::getglobtm(gtm);

    process_tm_for_drv_consts(gtm);
    d3d::set_vs_const(0, gtm[0], 4);
  }

  void resetStates() { stateMap.resetState(); }

  bool isLinesFull() { return numLines >= MAX_LINES; }

  bool isLines2ColoredFull() { return numLines2Colored >= MAX_LINES; }

  void flushLines()
  {
    if (numLines <= 0)
      return;

    push_blocks();
    setStates();

    d3d::draw_up(PRIM_LINELIST, numLines, lineVertices.data(), sizeof(lineVertices[0]));
    numLines = 0;
    lineVertices.resize(0);

    resetStates();
    pop_blocks();
  }

  bool isTrianglesFull() { return numTriangles >= MAX_TRIANGLES; }

  void flushTriangles()
  {
    if (numTriangles <= 0)
      return;

    push_blocks();
    setStates();

    d3d::draw_up(PRIM_TRILIST, numTriangles, triangleVertices.data(), sizeof(triangleVertices[0]));
    numTriangles = 0;
    triangleVertices.resize(0);

    resetStates();
    pop_blocks();
  }

  void flushLines2Colored()
  {
    if (numLines2Colored <= 0)
      return;

    if (!elem2Colored)
    {
      // Shouldn't happen unless someone closes the shaders mid-render.
      G_ASSERTF(0, "elem2Colored is nullptr, but numLines2Colored > 0");
      numLines2Colored = 0;
      lineVertices2Colored.resize(0);
      return;
    }

    push_blocks();

    d3d::settm(TM_WORLD, &Matrix44::IDENT);
    elem2Colored->setStates();
    d3d::draw_up(PRIM_LINELIST, numLines2Colored, lineVertices2Colored.data(), sizeof(lineVertices2Colored[0]));
    numLines2Colored = 0;
    lineVertices2Colored.resize(0);

    pop_blocks();
  }

  void flush()
  {
    flushTriangles();
    flushLines();
    flushLines2Colored();
  }

  void addLine(const Point3 &p0, E3DCOLOR c0, const Point3 &p1, E3DCOLOR c1)
  {
    lineVertices.emplace_back(wtm * p0, c0);
    lineVertices.emplace_back(wtm * p1, c1);

    ++numLines;

    if (isLinesFull())
      flush(); // flush with triangles to draw lines always over trianlges
  }

  void addLine(const Point3 *p0, int nm, E3DCOLOR c1)
  {
    for (int i = 0; i < nm - 1; ++i)
    {
      lineVertices.emplace_back(wtm * p0[i], c1);
      lineVertices.emplace_back(wtm * p0[i + 1], c1);

      ++numLines;

      if (isLinesFull())
        flush(); // flush with triangles to draw lines always over trianlges
    }
  }

  void addTriangle(const Point3 &p0, E3DCOLOR c0, const Point3 &p1, E3DCOLOR c1, const Point3 &p2, E3DCOLOR c2)
  {
    triangleVertices.emplace_back(wtm * p0, c0);
    triangleVertices.emplace_back(wtm * p1, c1);
    triangleVertices.emplace_back(wtm * p2, c2);
    ++numTriangles;

    if (isTrianglesFull())
      flushTriangles();
  }

  void addTriangles(const Point3 *points, int tn, E3DCOLOR c)
  {
    for (int i = 0; i < tn; ++i)
    {
      triangleVertices.emplace_back(wtm * points[i * 3 + 0], c);
      triangleVertices.emplace_back(wtm * points[i * 3 + 1], c);
      triangleVertices.emplace_back(wtm * points[i * 3 + 2], c);
      ++numTriangles;
    }

    if (isTrianglesFull())
      flushTriangles();
  }

  void addLine2Colored(const Point3 &p0, const Point3 &p1, E3DCOLOR color_front, E3DCOLOR color_behind)
  {
    if (!elem2Colored)
    {
      addLine(p0, color_front, p1, color_front);
      return;
    }

    // pack 2 colors as 4:4:4:4
    E3DCOLOR colorFront = e3dcolor(color4(color_front) * 15.0f * 16 / 255.0f);
    E3DCOLOR colorBehind = e3dcolor(color4(color_behind) * 15.0f / 255.0f);
    E3DCOLOR packedColor = (colorFront & 0xf0f0f0f0u) | (colorBehind & 0x0f0f0f0fu);

    lineVertices2Colored.emplace_back(wtm * p0, packedColor);
    lineVertices2Colored.emplace_back(wtm * p1, packedColor);

    ++numLines2Colored;

    if (isLines2ColoredFull())
      flushLines2Colored();
  }

  bool init2ColoredShader()
  {
    if (elem2Colored)
      return true;

    mat2Colored = new_shader_material_by_name_optional("debug_line_twocolored");
    if (mat2Colored)
      elem2Colored = mat2Colored->make_elem();
    if (!elem2Colored)
      logerr("draw_cached_debug_line_twocolored shader init failed, it'll fall back to draw_cached_debug_line");
    else
      debug("draw_cached_debug_line_twocolored is now initialized.");
    return (bool)elem2Colored;
  }

  void close2ColoredShaders()
  {
    elem2Colored = {};
    mat2Colored = {};
  }
};

static CachedDebugLinesData cdld;

bool init_draw_cached_debug_twocolored_shader() { return cdld.init2ColoredShader(); }

void close_draw_cached_debug()
{
  cdld.close2ColoredShaders();
  cdld.stateMap.clearStateOverrides();
}

void begin_draw_cached_debug_lines(bool test_z, bool write_z, bool z_func_less)
{
  cdld.testZ = test_z;
  cdld.writeZ = write_z;
  cdld.zFuncLess = z_func_less;
  cdld.wtm.identity();

  cdld.started++;
}

bool begin_draw_cached_debug_lines_ex()
{
  TMatrix wtm;
  d3d::gettm(TM_WORLD, wtm);

  if (cdld.started)
  {
    if (memcmp(&wtm, &cdld.wtm, sizeof(wtm)) == 0) //-V1014
      return false;
    ::cdld.wtm_prev = ::cdld.wtm;
    ::cdld.wtm = wtm;
    cdld.started++;
    return true;
  }

  ::cdld.wtm_prev = ::cdld.wtm;
  ::cdld.wtm = wtm;
  cdld.started++;
  return true;
}


void set_cached_debug_lines_wtm(const TMatrix &wtm) { ::cdld.wtm = wtm; }


void draw_cached_debug_line(const Point3 &p0, const Point3 &p1, E3DCOLOR color) { ::cdld.addLine(p0, color, p1, color); }

void draw_cached_debug_line_twocolored(const Point3 &p0, const Point3 &p1, E3DCOLOR color_front, E3DCOLOR color_behind)
{
  ::cdld.addLine2Colored(p0, p1, color_front, color_behind);
}


void draw_cached_debug_line(const Point3 *p0, int num, E3DCOLOR color) { ::cdld.addLine(p0, num, color); }


void end_draw_cached_debug_lines()
{
  ::cdld.flush();
  cdld.started--;
}
void end_draw_cached_debug_lines_ex()
{
  if (cdld.started > 1)
  {
    cdld.started--;
    ::cdld.wtm = ::cdld.wtm_prev;
  }
  else
    end_draw_cached_debug_lines();
}

void draw_cached_matrix_axis(const TMatrix &tm, float axis_len)
{
  const Point3 &p0 = tm.getcol(3);
  draw_cached_debug_line(p0, p0 + tm.getcol(0) * axis_len, E3DCOLOR(255, 0, 0));
  draw_cached_debug_line(p0, p0 + tm.getcol(1) * axis_len, E3DCOLOR(0, 255, 0));
  draw_cached_debug_line(p0, p0 + tm.getcol(2) * axis_len, E3DCOLOR(0, 0, 255));
}

void draw_cached_debug_box(const BBox3 &box, E3DCOLOR color)
{
  draw_cached_debug_line(box.lim[0], Point3(box.lim[1].x, box.lim[0].y, box.lim[0].z), color);
  draw_cached_debug_line(box.lim[0], Point3(box.lim[0].x, box.lim[1].y, box.lim[0].z), color);
  draw_cached_debug_line(box.lim[0], Point3(box.lim[0].x, box.lim[0].y, box.lim[1].z), color);
  draw_cached_debug_line(Point3(box.lim[1].x, box.lim[1].y, box.lim[0].z), Point3(box.lim[1].x, box.lim[0].y, box.lim[0].z), color);
  draw_cached_debug_line(Point3(box.lim[1].x, box.lim[1].y, box.lim[0].z), Point3(box.lim[0].x, box.lim[1].y, box.lim[0].z), color);
  draw_cached_debug_line(Point3(box.lim[1].x, box.lim[1].y, box.lim[0].z), Point3(box.lim[1].x, box.lim[1].y, box.lim[1].z), color);
  draw_cached_debug_line(Point3(box.lim[1].x, box.lim[0].y, box.lim[1].z), Point3(box.lim[1].x, box.lim[0].y, box.lim[0].z), color);
  draw_cached_debug_line(Point3(box.lim[1].x, box.lim[0].y, box.lim[1].z), Point3(box.lim[0].x, box.lim[0].y, box.lim[1].z), color);
  draw_cached_debug_line(Point3(box.lim[1].x, box.lim[0].y, box.lim[1].z), Point3(box.lim[1].x, box.lim[1].y, box.lim[1].z), color);
  draw_cached_debug_line(Point3(box.lim[0].x, box.lim[1].y, box.lim[1].z), Point3(box.lim[0].x, box.lim[1].y, box.lim[0].z), color);
  draw_cached_debug_line(Point3(box.lim[0].x, box.lim[1].y, box.lim[1].z), Point3(box.lim[0].x, box.lim[0].y, box.lim[1].z), color);
  draw_cached_debug_line(Point3(box.lim[0].x, box.lim[1].y, box.lim[1].z), Point3(box.lim[1].x, box.lim[1].y, box.lim[1].z), color);
}


void draw_cached_debug_box(const Point3 &p0, const Point3 &ax, const Point3 &ay, const Point3 &az, E3DCOLOR color)
{
  draw_cached_debug_line(p0, p0 + ax, color);
  draw_cached_debug_line(p0, p0 + ay, color);
  draw_cached_debug_line(p0, p0 + az, color);
  draw_cached_debug_line(p0 + ax + ay, p0 + ax, color);
  draw_cached_debug_line(p0 + ax + ay, p0 + ay, color);
  draw_cached_debug_line(p0 + ax + ay, p0 + ax + ay + az, color);
  draw_cached_debug_line(p0 + ax + az, p0 + ax, color);
  draw_cached_debug_line(p0 + ax + az, p0 + az, color);
  draw_cached_debug_line(p0 + ax + az, p0 + ax + ay + az, color);
  draw_cached_debug_line(p0 + ay + az, p0 + ay, color);
  draw_cached_debug_line(p0 + ay + az, p0 + az, color);
  draw_cached_debug_line(p0 + ay + az, p0 + ax + ay + az, color);
}

void draw_cached_debug_box(const BBox3 &box, E3DCOLOR color, const TMatrix &tm)
{
  const Point3 p0 = tm * box[0];
  const Point3 p1x = tm * Point3(box[1].x, box[0].y, box[0].z);
  const Point3 p1y = tm * Point3(box[0].x, box[1].y, box[0].z);
  const Point3 p1z = tm * Point3(box[0].x, box[0].y, box[1].z);
  draw_cached_debug_box(p0, p1x - p0, p1y - p0, p1z - p0, color);
}

void draw_cached_debug_sphere(const Point3 &c, real rad, E3DCOLOR col, int segs)
{
  if (rad < 0)
    return;

  const int MAX_SEGS = 64;

  if (segs < 3)
    segs = 3;
  else if (segs > MAX_SEGS)
    segs = MAX_SEGS;

  Point2 pt[MAX_SEGS];

  for (int i = 0; i < segs; ++i)
  {
    real a = i * TWOPI / segs;
    pt[i] = Point2(cosf(a), sinf(a)) * rad;
  }

  for (int i = 0; i < segs; ++i)
  {
    const Point2 &p0 = pt[i == 0 ? segs - 1 : i - 1];
    const Point2 &p1 = pt[i];
    ::draw_cached_debug_line(Point3(p0.x, p0.y, 0) + c, Point3(p1.x, p1.y, 0) + c, col);
    ::draw_cached_debug_line(Point3(p0.x, 0, p0.y) + c, Point3(p1.x, 0, p1.y) + c, col);
    ::draw_cached_debug_line(Point3(0, p0.x, p0.y) + c, Point3(0, p1.x, p1.y) + c, col);
  }

  ::draw_cached_debug_line(c - Point3(rad, 0, 0), c + Point3(rad, 0, 0), col);
  ::draw_cached_debug_line(c - Point3(0, rad, 0), c + Point3(0, rad, 0), col);
  ::draw_cached_debug_line(c - Point3(0, 0, rad), c + Point3(0, 0, rad), col);
}

void draw_cached_debug_sphere_outline_clipped(const Point3 &camera, const Point3 &center, const real R, const Point4 &clipPlane,
  E3DCOLOR col, int segs)
{
  if (R <= 0 || segs < 3)
    return;

  const Point3 v = center - camera;
  const float d2 = v.lengthSq();
  if (d2 <= R * R)
    return;

  const float k = (R * R) / d2;
  const float Rtan = R * sqrt(1.0f - k);
  const Point3 Ctan = center - k * v;

  const Point3 n(clipPlane.x, clipPlane.y, clipPlane.z);
  Point3 forward, tangent, binormal;
  ortho_normalize(v, Point3(0, 1, 0), forward, tangent, binormal);

  const int MAX_SEGS = 64;
  Point3 pt[MAX_SEGS];
  segs = eastl::min(segs, MAX_SEGS);
  for (int32_t i = 0; i < segs; ++i)
  {
    float angle = i * TWOPI / segs;
    float s, c;
    sincos(angle, s, c);
    pt[i] = tangent * c * Rtan + binormal * s * Rtan;
  }
  for (int32_t i = 0; i < segs; ++i)
  {
    Point3 a = Ctan + pt[i];
    Point3 b = Ctan + pt[(i + 1) % segs];
    float dist_a = dot(a, n) + clipPlane.w;
    float dist_b = dot(b, n) + clipPlane.w;

    if (dist_a < 0.0f && dist_b < 0.0f)
    {
      continue;
    }
    else if (dist_a >= 0.0f && dist_b >= 0.0f)
    {
      ::draw_cached_debug_line(a, b, col);
    }
    else
    {
      Point3 c = a + (dist_a / (dist_a - dist_b)) * (b - a);
      ::draw_cached_debug_line((dist_a >= 0.0f) ? a : b, c, col);
    }
  }
}

void draw_cached_debug_circle(const Point3 &c, const Point3 &a1, const Point3 &a2, real rad, E3DCOLOR col, int segs)
{
  if (rad < 0)
    return;

  const int MAX_SEGS = 64;

  if (segs < 3)
    segs = 3;
  else if (segs > MAX_SEGS)
    segs = MAX_SEGS;

  Point2 pt[MAX_SEGS];

  for (int i = 0; i < segs; ++i)
  {
    real a = i * TWOPI / segs;
    pt[i] = Point2(cosf(a), sinf(a)) * rad;
  }

  for (int i = 0; i < segs; ++i)
  {
    const Point2 &p0 = pt[i == 0 ? segs - 1 : i - 1];
    const Point2 &p1 = pt[i];
    ::draw_cached_debug_line(c + a1 * p0.x + a2 * p0.y, c + a1 * p1.x + a2 * p1.y, col);
  }

  ::draw_cached_debug_line(c - a1 * rad, c + a1 * rad, col);
  ::draw_cached_debug_line(c - a2 * rad, c + a2 * rad, col);
}


// ZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZ//


void draw_skeleton_link(const Point3 &target, real radius, E3DCOLOR link_color)
{
  ::draw_cached_debug_line(Point3(-radius, 0, 0), target, link_color);
  ::draw_cached_debug_line(Point3(+radius, 0, 0), target, link_color);
  ::draw_cached_debug_line(Point3(0, -radius, 0), target, link_color);
  ::draw_cached_debug_line(Point3(0, +radius, 0), target, link_color);
  ::draw_cached_debug_line(Point3(0, 0, -radius), target, link_color);
  ::draw_cached_debug_line(Point3(0, 0, +radius), target, link_color);
}


void draw_skeleton_tree(const GeomNodeTree &tree, real radius, E3DCOLOR point_color, E3DCOLOR link_color)
{
  for (dag::Index16 i(1), ie = dag::Index16(tree.nodeCount()); i < ie; ++i)
  {
    TMatrix tm;
    tree.getNodeWtmScalar(i, tm);
    ::set_cached_debug_lines_wtm(tm);
    ::draw_cached_debug_sphere(Point3(0, 0, 0), radius, point_color, 16);

    if (!tree.getChildCount(i))
      continue;

    for (int j = 0; j < tree.getChildCount(i); ++j)
      ::draw_skeleton_link(tree.getNodeLposScalar(tree.getChildNodeIdx(i, j)), radius, link_color);
  }
}

void draw_cached_debug_capsule_w(const Point3 &wp0, const Point3 &wp1, float rad, E3DCOLOR c)
{
  draw_cached_debug_sphere(wp0, rad, c);
  draw_cached_debug_sphere(wp1, rad, c);
  draw_cached_debug_line(wp0, wp1, c);
  Point3 wdir = normalize(wp1 - wp0);
  Point3 d, n;
  if (rabs(wdir.x) >= .999f)
    d = Point3(0, 1, 0) % wdir;
  else
    d = wdir % Point3(1, 0, 0);
  d.normalize();
  d *= rad;
  n = normalize(d % wdir) * rad;
  draw_cached_debug_line(wp0 + d, wp1 + d, c);
  draw_cached_debug_line(wp0 - d, wp1 - d, c);
  draw_cached_debug_line(wp0 + n, wp1 + n, c);
  draw_cached_debug_line(wp0 - n, wp1 - n, c);
}
void draw_cached_debug_capsule_w(const Capsule &cap, E3DCOLOR c) { draw_cached_debug_capsule_w(cap.a, cap.b, cap.r, c); }

void draw_cached_debug_capsule(const Capsule &cap, E3DCOLOR c, const TMatrix &wtm)
{
  Point3 p0 = wtm * cap.a, p1 = wtm * cap.b;
  draw_cached_debug_sphere(p0, cap.r, c);
  draw_cached_debug_sphere(p1, cap.r, c);
  draw_cached_debug_line(p0, p1, c);
  Point3 ld = normalize(p1 - p0);
  Point3 d, n;
  if (rabs(ld.x) >= .999f)
    d = Point3(0, 1, 0) % ld;
  else
    d = ld % Point3(1, 0, 0);
  d.normalize();
  d *= cap.r;
  n = normalize(d % ld) * cap.r;
  draw_cached_debug_line(p0 + d, p1 + d, c);
  draw_cached_debug_line(p0 - d, p1 - d, c);
  draw_cached_debug_line(p0 + n, p1 + n, c);
  draw_cached_debug_line(p0 - n, p1 - n, c);
}

void draw_cached_debug_cylinder(const TMatrix &tm, float rad, float height, E3DCOLOR c)
{
  constexpr int numSegments = 32;
  float invNumSegments = 1.f / float(numSegments);
  for (int i = 0; i < numSegments; ++i)
  {
    Point3 pos00 = Point3(sinf((i + 0) * invNumSegments * TWOPI) * rad, -height * 0.5f, cosf((i + 0) * invNumSegments * TWOPI) * rad);
    Point3 pos01 = Point3(sinf((i + 1) * invNumSegments * TWOPI) * rad, -height * 0.5f, cosf((i + 1) * invNumSegments * TWOPI) * rad);
    Point3 pos10 = Point3(sinf((i + 0) * invNumSegments * TWOPI) * rad, +height * 0.5f, cosf((i + 0) * invNumSegments * TWOPI) * rad);
    Point3 pos11 = Point3(sinf((i + 1) * invNumSegments * TWOPI) * rad, +height * 0.5f, cosf((i + 1) * invNumSegments * TWOPI) * rad);
    draw_cached_debug_line(tm * pos00, tm * pos01, c);
    draw_cached_debug_line(tm * pos10, tm * pos11, c);
    draw_cached_debug_line(tm * pos00, tm * pos10, c);
  }
}

void draw_cached_debug_cylinder(const TMatrix &tm, Point3 &a, Point3 &b, float rad, E3DCOLOR c)
{
  Point3 axis = normalize(a - b);
  Point3 up(0, 1, 0);
  Point3 diff = axis - up;
  if (abs(diff.x) < FLT_EPSILON && abs(diff.y) < FLT_EPSILON && abs(diff.z) < FLT_EPSILON)
    up = Point3(1, 0, 0);

  Point3 perp_x = normalize(cross(axis, up));
  Point3 perp_z = normalize(cross(axis, perp_x));
  real height = length(a - b);

  constexpr int numSegments = 32;
  float invNumSegments = 1.f / float(numSegments);
  for (int i = 0; i < numSegments; ++i)
  {
    Point3 pos00 = sinf((i + 0) * invNumSegments * TWOPI) * rad * perp_x - height * 0.5f * axis +
                   cosf((i + 0) * invNumSegments * TWOPI) * rad * perp_z;
    Point3 pos01 = sinf((i + 1) * invNumSegments * TWOPI) * rad * perp_x - height * 0.5f * axis +
                   cosf((i + 1) * invNumSegments * TWOPI) * rad * perp_z;
    Point3 pos10 = sinf((i + 0) * invNumSegments * TWOPI) * rad * perp_x + height * 0.5f * axis +
                   cosf((i + 0) * invNumSegments * TWOPI) * rad * perp_z;
    Point3 pos11 = sinf((i + 1) * invNumSegments * TWOPI) * rad * perp_x + height * 0.5f * axis +
                   cosf((i + 1) * invNumSegments * TWOPI) * rad * perp_z;
    draw_cached_debug_line(tm * pos00, tm * pos01, c);
    draw_cached_debug_line(tm * pos10, tm * pos11, c);
    draw_cached_debug_line(tm * pos00, tm * pos10, c);
  }
}

void draw_cached_debug_cylinder_w(Point3 &a, Point3 &b, float rad, E3DCOLOR c)
{
  Point3 axis = normalize(a - b);
  Point3 up(0, 1, 0);
  Point3 diff = axis - up;
  if (abs(diff.x) < FLT_EPSILON && abs(diff.y) < FLT_EPSILON && abs(diff.z) < FLT_EPSILON)
    up = Point3(1, 0, 0);

  Point3 perp_x = normalize(cross(axis, up));
  Point3 perp_z = normalize(cross(axis, perp_x));
  real height = length(a - b);

  Point3 middle = (a + b) * 0.5;

  constexpr int numSegments = 32;
  float invNumSegments = 1.f / float(numSegments);
  for (int i = 0; i < numSegments; ++i)
  {
    Point3 pos00 = middle + sinf((i + 0) * invNumSegments * TWOPI) * rad * perp_x - height * 0.5f * axis +
                   cosf((i + 0) * invNumSegments * TWOPI) * rad * perp_z;
    Point3 pos01 = middle + sinf((i + 1) * invNumSegments * TWOPI) * rad * perp_x - height * 0.5f * axis +
                   cosf((i + 1) * invNumSegments * TWOPI) * rad * perp_z;
    Point3 pos10 = middle + sinf((i + 0) * invNumSegments * TWOPI) * rad * perp_x + height * 0.5f * axis +
                   cosf((i + 0) * invNumSegments * TWOPI) * rad * perp_z;
    Point3 pos11 = middle + sinf((i + 1) * invNumSegments * TWOPI) * rad * perp_x + height * 0.5f * axis +
                   cosf((i + 1) * invNumSegments * TWOPI) * rad * perp_z;
    draw_cached_debug_line(pos00, pos01, c);
    draw_cached_debug_line(pos10, pos11, c);
    draw_cached_debug_line(pos00, pos10, c);
  }
}

void draw_cached_debug_cone(const Point3 &p0, const Point3 &p1, real angleRad, E3DCOLOR col, int segs)
{
  Point3 dir = p1 - p0;
  const float length = dir.length();
  dir *= safeinv(length);
  Point3 norm;
  if (rabs(dir.x) >= FLT_EPSILON)
    norm = Point3(0, dir.z, -dir.y); // dir % Point3(1,0,0)
  else
    norm = Point3(dir.z, 0, -dir.x); // Point3(0, 1, 0) % dir
  norm.normalize();
  ::draw_cached_debug_line(p0, p0 + dir * length, col);
  const float rad = length * tanf(angleRad);

  const float angleStep = 2 * PI / segs;
  Point3 lastPoint = ZERO<Point3>();
  Point3 firstPoint = ZERO<Point3>();
  for (int i = 0; i < segs; ++i)
  {
    const Quat quaternion = Quat(dir, i * angleStep);
    const Point3 newDir = quaternion * norm;
    const Point3 newPos = p0 + newDir * rad;
    const Point3 pushedPoint = newPos + dir * length;
    ::draw_cached_debug_line(p0, pushedPoint, col);
    if (i > 0)
      ::draw_cached_debug_line(lastPoint, pushedPoint, col);
    else
      firstPoint = newPos + dir * length;
    lastPoint = newPos + dir * length;
  }
  ::draw_cached_debug_line(lastPoint, firstPoint, col);
}

void draw_cached_debug_trilist(const Point3 *p, int tn, E3DCOLOR c) { cdld.addTriangles(p, tn, c); }

void draw_cached_debug_hex(const TMatrix &view_itm, const Point3 &pos, real rad, E3DCOLOR c)
{
  push_blocks();
  cdld.setStates();
  CachedDebugLinesData::Vertex v[6];
  Point3 ax = rad * view_itm.getcol(0), ay = rad * view_itm.getcol(1);

  v[0].p = pos + ax * cosf(3 * PI / 3) + ay * sinf(3 * PI / 3);
  v[0].c = c;
  v[1].p = pos + ax * cosf(4 * PI / 3) + ay * sinf(4 * PI / 3);
  v[1].c = c;
  v[2].p = pos + ax * cosf(2 * PI / 3) + ay * sinf(2 * PI / 3);
  v[2].c = c;
  v[3].p = pos + ax * cosf(5 * PI / 3) + ay * sinf(5 * PI / 3);
  v[3].c = c;
  v[4].p = pos + ax * cosf(1 * PI / 3) + ay * sinf(1 * PI / 3);
  v[4].c = c;
  v[5].p = pos + ax * cosf(0 * PI / 3) + ay * sinf(0 * PI / 3);
  v[5].c = c;
  d3d::draw_up(PRIM_TRISTRIP, 4, v, sizeof(v[0]));
  cdld.resetStates();
  pop_blocks();
}
void draw_cached_debug_quad(const Point3 p[4], E3DCOLOR c)
{
  push_blocks();
  cdld.setStates();
  CachedDebugLinesData::Vertex v[4];
  v[0].p = p[0];
  v[0].c = c;
  v[1].p = p[1];
  v[1].c = c;
  v[2].p = p[2];
  v[2].c = c;
  v[3].p = p[3];
  v[3].c = c;
  d3d::draw_up(PRIM_TRISTRIP, 2, v, sizeof(v[0]));
  cdld.resetStates();
  pop_blocks();
}

void draw_cached_debug_solid_triangle(const Point3 p[3], E3DCOLOR c) { cdld.addTriangle(p[0], c, p[1], c, p[2], c); }

void draw_cached_line_strip(const TMatrix &tm, dag::ConstSpan<Point2> v, const float width, const E3DCOLOR &color)
{
  if (v.size() < 2)
    return;

  Point3 vertices[4];
  Point2 edge1 = normalize(v[1] - v[0]);
  Point2 norm = Point2(edge1.y, -edge1.x);
  vertices[0].set_xy0(v[0] - norm * width);
  vertices[1].set_xy0(v[0] + norm * width);
  vertices[0] = tm * vertices[0];
  vertices[1] = tm * vertices[1];

  for (int32_t i = 1; i < v.size(); i++)
  {
    edge1 = normalize(v[i] - v[i - 1]);
    norm = Point2(edge1.y, -edge1.x);

    float len = width;
    if (i < v.size() - 1)
    {
      Point2 edge2 = normalize(v[i + 1] - v[i]);
      Point2 miterNorm = normalize(Point2(edge1.y + edge2.y, -edge1.x - edge2.x));
      len = width / dot(norm, miterNorm);
      norm = miterNorm;
    }

    vertices[2].set_xy0(v[i] - norm * len);
    vertices[3].set_xy0(v[i] + norm * len);
    vertices[2] = tm * vertices[2];
    vertices[3] = tm * vertices[3];
    cdld.addTriangle(vertices[0], color, vertices[1], color, vertices[2], color);
    cdld.addTriangle(vertices[2], color, vertices[1], color, vertices[3], color);
    vertices[0] = vertices[2];
    vertices[1] = vertices[3];
  }
}

void draw_cached_debug_solid_box(const mat43f *__restrict tm, const bbox3f *__restrict box, int count, E3DCOLOR c)
{
  push_blocks();
  cdld.setStates();
  static constexpr int MAX_BOXES = 64;
  carray<CachedDebugLinesData::Vertex, 12 * 3 * MAX_BOXES> vertices;
  CachedDebugLinesData::Vertex *vert = vertices.data();
  for (int i = 0; i < vertices.size(); ++i)
    vertices[i].c = c;
  for (int i = 0; i < count; ++i)
  {
    Point3_vec4 corners[8];
    v_st(&corners[0].x, v_mat43_mul_vec3p(tm[i], v_bbox3_point(box[i], 1)));
    v_st(&corners[1].x, v_mat43_mul_vec3p(tm[i], v_bbox3_point(box[i], 5)));
    v_st(&corners[2].x, v_mat43_mul_vec3p(tm[i], v_bbox3_point(box[i], 7)));
    v_st(&corners[3].x, v_mat43_mul_vec3p(tm[i], v_bbox3_point(box[i], 3)));
    v_st(&corners[4].x, v_mat43_mul_vec3p(tm[i], v_bbox3_point(box[i], 0)));
    v_st(&corners[5].x, v_mat43_mul_vec3p(tm[i], v_bbox3_point(box[i], 4)));
    v_st(&corners[6].x, v_mat43_mul_vec3p(tm[i], v_bbox3_point(box[i], 6)));
    v_st(&corners[7].x, v_mat43_mul_vec3p(tm[i], v_bbox3_point(box[i], 2)));
    vert[0].p = corners[1];
    vert[1].p = corners[0];
    vert[2].p = corners[2];
    vert[3].p = corners[3];
    vert[4].p = corners[2];
    vert[5].p = corners[0];
    vert += 6;

    vert[0].p = corners[4];
    vert[1].p = corners[5];
    vert[2].p = corners[6];
    vert[3].p = corners[6];
    vert[4].p = corners[7];
    vert[5].p = corners[4];
    vert += 6;

    vert[0].p = corners[7];
    vert[1].p = corners[6];
    vert[2].p = corners[2];
    vert[3].p = corners[2];
    vert[4].p = corners[3];
    vert[5].p = corners[7];
    vert += 6;

    vert[0].p = corners[5];
    vert[1].p = corners[4];
    vert[2].p = corners[1];
    vert[3].p = corners[0];
    vert[4].p = corners[1];
    vert[5].p = corners[4];
    vert += 6;

    vert[0].p = corners[6];
    vert[1].p = corners[5];
    vert[2].p = corners[2];
    vert[3].p = corners[1];
    vert[4].p = corners[2];
    vert[5].p = corners[5];
    vert += 6;

    vert[0].p = corners[4];
    vert[1].p = corners[7];
    vert[2].p = corners[3];
    vert[3].p = corners[3];
    vert[4].p = corners[0];
    vert[5].p = corners[4];
    vert += 6;
    if ((i % MAX_BOXES) == MAX_BOXES - 1 && i < count - 1)
      d3d::draw_up(PRIM_TRILIST, MAX_BOXES * 12, vert = vertices.data(), sizeof(CachedDebugLinesData::Vertex));
  }
  d3d::draw_up(PRIM_TRILIST, (count % MAX_BOXES) * 12, vertices.data(), sizeof(CachedDebugLinesData::Vertex));
  cdld.resetStates();
  pop_blocks();
}

void draw_debug_frustum(const Frustum &fr, E3DCOLOR col)
{
  push_blocks();
  cdld.setStates();
  Point3 p[4];
  vec4f pntList[8];
  fr.generateAllPointFrustm(pntList);
  for (int z = 0; z < 2; ++z)
  {
    for (int j = 0; j < 2; ++j)
    {
      int i = j * 2;
      p[0] = as_point3(&pntList[(i << 1)]);
      p[1] = as_point3(&pntList[(i << 1) | 1]);
      p[2] = as_point3(&pntList[(((i + 1) % 4) << 1)]);
      p[3] = as_point3(&pntList[(((i + 1) % 4) << 1) + 1]);
      col.b = j * 70;
      draw_debug_quad(p, col);
    }

    for (int j = 0; j < 2; ++j)
    {
      int i = j;
      p[0] = as_point3(&pntList[(i << 1)]);
      p[1] = as_point3(&pntList[(i << 1) | 1]);
      p[2] = as_point3(&pntList[(((i + 2) % 4) << 1)]);
      p[3] = as_point3(&pntList[(((i + 2) % 4) << 1) + 1]);
      col.b = (j + 2) * 70;
      draw_debug_quad(p, col);
    }
    col.a /= 2;

    if (z == 0)
    {
      cdld.resetStates();
      cdld.zFuncLess = true;
      cdld.setStates();
      cdld.zFuncLess = false;
    }
  }
  cdld.resetStates();
  pop_blocks();
}

void draw_cached_debug_quad(vec4f &p0, vec4f &p1, vec4f &p2, vec4f &p3, E3DCOLOR col)
{
  alignas(16) Point3 p[5];
  v_st(&p[0].x, p0);
  v_stu(&p[1].x, p1);
  v_stu(&p[2].x, p3);
  v_stu(&p[3].x, p2);
  draw_cached_debug_quad(p, col);
}

void draw_cached_debug_proj_matrix(const TMatrix4 &tm, E3DCOLOR nearplaneColor, E3DCOLOR farplaneColor, E3DCOLOR sideColor)
{
  TMatrix4 invTm;

  float det;
  if (!inverse44(tm, invTm, det))
    return;

  Point4 pt[8] = {
    {-1, 1, 0, 1}, {1, 1, 0, 1}, {1, -1, 0, 1}, {-1, -1, 0, 1}, {-1, 1, 1, 1}, {1, 1, 1, 1}, {1, -1, 1, 1}, {-1, -1, 1, 1}};

  for (int i = 0; i < 8; ++i)
    pt[i] = pt[i] * invTm;

  for (int i = 0; i < 8; ++i)
    pt[i] /= pt[i].w;

  ::draw_cached_debug_line((Point3 &)pt[0], (Point3 &)pt[4], sideColor);
  ::draw_cached_debug_line((Point3 &)pt[1], (Point3 &)pt[5], sideColor);
  ::draw_cached_debug_line((Point3 &)pt[2], (Point3 &)pt[6], sideColor);
  ::draw_cached_debug_line((Point3 &)pt[3], (Point3 &)pt[7], sideColor);

  ::draw_cached_debug_line((Point3 &)pt[0], (Point3 &)pt[1], nearplaneColor);
  ::draw_cached_debug_line((Point3 &)pt[1], (Point3 &)pt[2], nearplaneColor);
  ::draw_cached_debug_line((Point3 &)pt[2], (Point3 &)pt[3], nearplaneColor);
  ::draw_cached_debug_line((Point3 &)pt[3], (Point3 &)pt[0], nearplaneColor);

  ::draw_cached_debug_line((Point3 &)pt[4], (Point3 &)pt[5], farplaneColor);
  ::draw_cached_debug_line((Point3 &)pt[5], (Point3 &)pt[6], farplaneColor);
  ::draw_cached_debug_line((Point3 &)pt[6], (Point3 &)pt[7], farplaneColor);
  ::draw_cached_debug_line((Point3 &)pt[7], (Point3 &)pt[4], farplaneColor);
}

#endif
