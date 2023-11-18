#include <3d/dag_drv3d.h>
#include <3d/dag_drv3dReset.h>
#include <render/primitiveObjects.h>
#include <math/dag_bounds3.h>
#include <math/dag_color.h>
#include <math/dag_capsule.h>
#include <math/dag_Quat.h>
#include <math/dag_TMatrix.h>
#include <math/dag_mathUtils.h>
#include <3d/dag_materialData.h>
#include <shaders/dag_shaders.h>
#include <render/debug3dSolid.h>

#include <ioSys/dag_dataBlock.h>
#include <3d/dag_resPtr.h>

static CompiledShaderChannelId chan[1] = {
  {SCTYPE_FLOAT3, SCUSAGE_POS, 0, 0},
};
#define WORLD_REG 8
static inline void set_world(const TMatrix &tm)
{
  d3d::set_vs_const1(WORLD_REG + 0, tm.getcol(0).x, tm.getcol(0).y, tm.getcol(0).z, 0);
  d3d::set_vs_const1(WORLD_REG + 1, tm.getcol(1).x, tm.getcol(1).y, tm.getcol(1).z, 0);
  d3d::set_vs_const1(WORLD_REG + 2, tm.getcol(2).x, tm.getcol(2).y, tm.getcol(2).z, 0);
  d3d::set_vs_const1(WORLD_REG + 3, tm.getcol(3).x, tm.getcol(3).y, tm.getcol(3).z, 1);
}
#define COLOR_REG 12

static ShaderMaterial *debugCollisionMat = NULL;
static dynrender::RElem debugCollisionElem;
static dynrender::RElem sphereElem;
static dynrender::RElem capsuleCapElem;
static dynrender::RElem capsuleCylElem;

static ShaderMaterial *debugCollisionMatShaded = NULL;
static dynrender::RElem debugCollisionElemShaded;
SharedTexHolder debugTex;
static shaders::OverrideStateId flip_face_ovid;

void close_debug_solid()
{
  if (debugCollisionMat)
  {
    debugCollisionMat->delRef();
    debugCollisionMat = NULL;
  }
  debugCollisionElem.shElem = nullptr;
  del_d3dres(sphereElem.vb);
  del_d3dres(sphereElem.ib);
  sphereElem.shElem = nullptr;
  del_d3dres(capsuleCapElem.vb);
  del_d3dres(capsuleCapElem.ib);
  capsuleCapElem.shElem = nullptr;
  del_d3dres(capsuleCylElem.vb);
  del_d3dres(capsuleCylElem.ib);
  capsuleCylElem.shElem = nullptr;

  if (debugCollisionMatShaded)
  {
    debugCollisionMatShaded->delRef();
    debugCollisionMatShaded = NULL;
  }
  debugCollisionElemShaded.shElem = nullptr;
  debugTex.close();
  shaders::overrides::destroy(flip_face_ovid);
}

static bool init()
{
  if (debugCollisionMat)
    return true;
  MaterialData matNull;
  matNull.className = "debug_ri";
  debugCollisionMat = new_shader_material(matNull);
  G_ASSERT(debugCollisionMat);
  if (!debugCollisionMat)
    return false;
  debugCollisionMat->addRef();
  if (!debugCollisionMat->checkChannels(chan, sizeof(chan) / sizeof(chan[0])))
  {
    fatal("invalid channels for this material!");
    return false;
  }

  debugCollisionElem.shElem = debugCollisionMat->make_elem();

  if (!flip_face_ovid)
  {
    shaders::OverrideState state;
    state.set(shaders::OverrideState::FLIP_CULL);
    flip_face_ovid = shaders::overrides::create(state);
  }
  return true;
}

static bool init_shaded()
{
  if (debugCollisionMatShaded)
    return true;
  MaterialData matNull;
  matNull.className = "debug_ri_shaded";
  debugCollisionMatShaded = new_shader_material(matNull);
  G_ASSERT(debugCollisionMatShaded);
  if (!debugCollisionMatShaded)
    return false;
  debugCollisionMatShaded->addRef();
  if (!debugCollisionMatShaded->checkChannels(chan, sizeof(chan) / sizeof(chan[0])))
  {
    fatal("invalid channels for this material!");
    return false;
  }
  debugCollisionElemShaded.shElem = debugCollisionMatShaded->make_elem();

  const auto *texName = ::dgs_get_game_params()->getStr("debugRiTexture", "camo_test_checker");
  const auto texSize = ::dgs_get_game_params()->getInt("debugRiTextureSize", 1);
  const auto wireColor = ::dgs_get_game_params()->getPoint4("debugRiWireColor", Point4(1, 0, 0, 1));
  debugTex = dag::get_tex_gameres(texName, "debug_triplanar_tex");
  if (debugTex.getTexId() == BAD_TEXTUREID)
  {
    fatal("Couldn't get tex gameres %s", texName);
    return false;
  }

  const auto debug_triplanar_tex_sizeVarID = get_shader_variable_id("debug_triplanar_tex_size", false);
  const auto debug_ri_wire_colorVarID = get_shader_variable_id("debug_ri_wire_color", false);
  ShaderGlobal::set_int(debug_triplanar_tex_sizeVarID, texSize);
  ShaderGlobal::set_color4(debug_ri_wire_colorVarID, wireColor);

  if (!flip_face_ovid)
  {
    shaders::OverrideState state;
    state.set(shaders::OverrideState::FLIP_CULL);
    flip_face_ovid = shaders::overrides::create(state);
  }
  return true;
}

enum SHAPE_TYPE
{
  SHAPE_SPHERE,
  SHAPE_HEMISPHERE,
  SHAPE_CYLINDER
};

static void create_shape_relem(dynrender::RElem &relem, SHAPE_TYPE shape)
{
  const int slicesCount = 16;
  const int stacksCount = shape == SHAPE_HEMISPHERE ? 8 : 16;
  uint32_t vertexCount = 0;
  uint32_t faceCount = 0;
  if (shape == SHAPE_SPHERE || shape == SHAPE_HEMISPHERE)
    calc_sphere_vertex_face_count(slicesCount, stacksCount, shape == SHAPE_HEMISPHERE, vertexCount, faceCount);
  else if (shape == SHAPE_CYLINDER)
    calc_cylinder_vertex_face_count(2, stacksCount, vertexCount, faceCount);
  uint32_t vbSize = vertexCount * sizeof(Point3);
  uint32_t ibSize = faceCount * sizeof(uint16_t) * 3;

  // Create the mesh
  Vbuffer *vb = d3d::create_vb(vbSize, 0, "solid_sphere");
  Ibuffer *ib = d3d::create_ib(ibSize, 0, "solid_sphere_ib");

  uint8_t *vertices = NULL;
  uint8_t *indices = NULL;

  if (!vb->lock(0, vbSize, (void **)&vertices, VBLOCK_WRITEONLY))
  {
    del_d3dres(vb);
    del_d3dres(ib);
    return;
  }

  if (!ib->lock(0, ibSize, (void **)&indices, VBLOCK_WRITEONLY))
  {
    vb->unlock();
    del_d3dres(vb);
    del_d3dres(ib);
    return;
  }

  if (shape == SHAPE_SPHERE || shape == SHAPE_HEMISPHERE)
    create_sphere_mesh(dag::Span<uint8_t>(vertices, vbSize), dag::Span<uint8_t>(indices, ibSize), 1.0f, slicesCount, stacksCount,
      sizeof(Point3), false, false, false, shape == SHAPE_HEMISPHERE);
  else if (shape == SHAPE_CYLINDER)
    create_cylinder_mesh(dag::Span<uint8_t>(vertices, vbSize), dag::Span<uint8_t>(indices, ibSize), 1.0f, 1.0f, 2, stacksCount,
      sizeof(Point3), false, false, false);

  vb->unlock();
  ib->unlock();

  relem.numVert = vertexCount;
  relem.numPrim = faceCount;
  relem.stride = sizeof(Point3);
  relem.vDecl = dynrender::addShaderVdecl(chan, sizeof(chan) / sizeof(chan[0]));
  relem.ib = ib;
  relem.vb = vb;
}

void draw_debug_solid_sphere(const Point3 &sphere_c, float sphere_r, const TMatrix &instance_tm, const Color4 &color, bool shaded)
{
  if (!(shaded ? init_shaded() : init()))
    return;
  if (sphereElem.vb == NULL)
    create_shape_relem(sphereElem, SHAPE_SPHERE);
  if (!sphereElem.vb)
    return;

  sphereElem.shElem = shaded ? debugCollisionElemShaded.shElem : debugCollisionElem.shElem;

  TMatrix tm(sphere_r);
  tm.setcol(3, sphere_c);

  set_world(instance_tm * tm);
  d3d::set_vs_const1(COLOR_REG, color.r, color.g, color.b, color.a);
  sphereElem.render();
  ShaderElement::invalidate_cached_state_block();
}

void draw_debug_solid_capsule(const Capsule &capsule, const TMatrix &instance_tm, const Color4 &color, bool shaded)
{
  if (!(shaded ? init_shaded() : init()))
    return;
  if (capsuleCapElem.vb == NULL)
    create_shape_relem(capsuleCapElem, SHAPE_HEMISPHERE);
  if (!capsuleCapElem.vb)
    return;

  if (capsuleCylElem.vb == NULL)
    create_shape_relem(capsuleCylElem, SHAPE_CYLINDER);

  capsuleCylElem.shElem = shaded ? debugCollisionElemShaded.shElem : debugCollisionElem.shElem;
  capsuleCapElem.shElem = shaded ? debugCollisionElemShaded.shElem : debugCollisionElem.shElem;

  Point3 dir = capsule.b - capsule.a;
  float height = dir.length();
  dir.normalize();
  Quat rot = quat_rotation_arc(Point3(0, 1, 0), dir);
  TMatrix offs = TMatrix::IDENT;
  offs.setcol(3, (capsule.a + capsule.b) / 2);
  TMatrix worldTm = instance_tm * offs * quat_to_matrix(rot);

  TMatrix tm(capsule.r);
  TMatrix rotX;
  rotX.rotxTM(PI);
  tm *= rotX;
  tm.setcol(3, Point3(0, -height / 2, 0));
  set_world(worldTm * tm);
  capsuleCapElem.render();

  tm = TMatrix(capsule.r);
  tm.setcol(3, Point3(0, height / 2, 0));
  set_world(worldTm * tm);
  capsuleCapElem.render();

  tm = TMatrix::IDENT;
  tm[0][0] = capsule.r;
  tm[1][1] = height;
  tm[2][2] = capsule.r;
  set_world(worldTm * tm);
  d3d::set_vs_const1(COLOR_REG, color.r, color.g, color.b, color.a);
  capsuleCylElem.render();
  ShaderElement::invalidate_cached_state_block();
}

void draw_debug_solid_mesh(const uint16_t *indices, int faces_count, const float *xyz_pos, int vertex_size, int vertices_count,
  const TMatrix &tm, const Color4 &color, bool shaded, DrawSolidMeshCull cull)
{
  if (!(shaded ? init_shaded() : init()))
    return;
  if (cull == DrawSolidMeshCull::FLIP)
    shaders::overrides::set(flip_face_ovid);
  (shaded ? debugCollisionElemShaded : debugCollisionElem).shElem->setStates(0, true);
  set_world(tm);
  d3d::set_vs_const1(COLOR_REG, color.r, color.g, color.b, color.a);
  d3d::drawind_up(PRIM_TRILIST, 0, vertices_count, faces_count, (uint16_t *)indices, (void *)xyz_pos, vertex_size);
  ShaderElement::invalidate_cached_state_block();
  if (cull == DrawSolidMeshCull::FLIP)
    shaders::overrides::reset();
}

void draw_debug_solid_cube(const BBox3 &cube, const TMatrix &instance_tm, const Color4 &color, bool shaded)
{
  Point3 vertices[8];
  // todo: replace with scaled matrix
  for (int i = 0; i < countof(vertices); ++i)
    vertices[i] = cube.point(i);

  static const uint16_t box_indices[36] = {
    0, 3, 1, 0, 2, 3, 0, 1, 5, 0, 5, 4, 0, 4, 6, 0, 6, 2, 1, 7, 5, 1, 3, 7, 4, 5, 7, 4, 7, 6, 2, 7, 3, 2, 6, 7};

  draw_debug_solid_mesh(box_indices, countof(box_indices) / 3, &vertices[0].x, sizeof(Point3), countof(vertices), instance_tm, color,
    shaded);
}

void draw_debug_solid_cone(const Point3 pos, Point3 norm, float radius, float height, int segments, const Color4 &color)
{
  if (radius <= 0)
    return;
  segments = segments < 3 ? min((int)max(ceil(radius * PI), 6.f), 48) : segments;
  const size_t vertexCount = segments + 2;
  const size_t indexCount = segments * 6;
  dag::Vector<Point3> vertices(vertexCount);
  dag::Vector<uint16_t> indices;
  indices.reserve(indexCount);

  norm.normalize(); // Just in case
  Point3 noise = fabs(dot(norm, Point3(1, 0, 0))) < 0.5 ? Point3(1, 0, 0) : Point3(0, 1, 0);
  Point3 ax1 = cross(norm, norm + noise);
  Point3 ax2 = cross(norm, ax1);
  for (int i = 0; i < segments; i++)
  {
    float angle = i * 2.f * PI / segments;
    float s, c;
    sincos(angle, s, c);
    Point3 p = pos + ax1 * c * radius + ax2 * s * radius;
    vertices[i] = p;
    int i2 = (i + 1) % segments;
    indices.push_back(i2);
    indices.push_back(i);
    indices.push_back(vertexCount - 1);
    indices.push_back(i);
    indices.push_back(i2);
    indices.push_back(vertexCount - 2);
  }

  vertices[vertexCount - 1] = pos + norm * height;
  vertices[vertexCount - 2] = pos;

  draw_debug_solid_mesh(indices.data(), indexCount / 3, &vertices[0].x, sizeof(Point3), vertexCount, TMatrix::IDENT, color);
}

static void debug_solid_after_reset_device(bool) { close_debug_solid(); }

REGISTER_D3D_AFTER_RESET_FUNC(debug_solid_after_reset_device);
