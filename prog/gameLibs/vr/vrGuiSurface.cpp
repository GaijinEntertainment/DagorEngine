// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include <vr/vrGuiSurface.h>

#include <debug/dag_assert.h>
#include <util/dag_stdint.h>
#include <util/dag_string.h>
#include <ioSys/dag_dataBlock.h>
#include <shaders/dag_shaders.h>
#include <drv/3d/dag_vertexIndexBuffer.h>
#include <drv/3d/dag_driver.h>
#include <drv/3d/dag_buffers.h>
#include <drv/3d/dag_resetDevice.h>
#include <3d/dag_materialData.h>
#include <startup/dag_globalSettings.h>
#include <util/dag_globDef.h>
#include <math/dag_rayIntersectSphere.h>
#include <math/dag_TMatrix.h>
#include <math/dag_mathUtils.h>
#include <math/dag_plane3.h>

namespace
{

struct RenderParams
{
  ShaderMaterial *mat = nullptr;
  dynrender::RElem re;
  Vbuffer *vb = nullptr;
  Ibuffer *ib = nullptr;

  int texVarId = -1;
  float depth = 5.0f;
  float vFov = DegToRad(100.f);
  float aspect = 16.f / 9.f;
  int width = 1920;
  int height = 1080;
  vrgui::SurfaceCurvature curvature = vrgui::SurfaceCurvature::VRGUI_SURFACE_SPHERE;
  float planeScale = 5.f;

  void applySettings(const char *blk_name, int w, int h, vrgui::SurfaceCurvature c);
};


void RenderParams::applySettings(const char *blk_name, int w, int h, vrgui::SurfaceCurvature c)
{
  if (const DataBlock *gp = ::dgs_get_game_params())
    if (const DataBlock *p = gp->getBlockByName(blk_name))
    {
      depth = p->getReal("uiDepth", depth);
      vFov *= p->getReal("uiVFovScale", 0.9f);
      planeScale = p->getReal("planeScale", planeScale);
      if (c == vrgui::VRGUI_SURFACE_DEFAULT)
      {
        const char *value = p->getStr("curvature", "sphere");
        if (strcmp(value, "sphere") == 0)
          curvature = vrgui::VRGUI_SURFACE_SPHERE;
        else if (strcmp(value, "plane") == 0)
          curvature = vrgui::VRGUI_SURFACE_PLANE;
        else if (strcmp(value, "cylinder") == 0)
          curvature = vrgui::VRGUI_SURFACE_CYLINDER;
      }
      else
        curvature = c;
    }
  width = w;
  height = h;
  aspect = width / (float)height;
  debug("[VRUI] size: %dx%d (%.4g), fov %.3g, depth %.3g, curve %d", width, height, aspect, vFov, depth, curvature);
}

bool is_point_within_gui(const RenderParams &rp, Point2 p) { return 0 <= p.x && p.x <= rp.width && 0 <= p.y && p.y <= rp.height; }

Point3 map_to_sphere(const RenderParams &rp, const Point2 &point)
{
  const float phi = (point.x - 0.5f) * rp.vFov * rp.aspect;
  const float theta = HALFPI + (0.5f - point.y) * rp.vFov;
  return Point3(-rp.depth, 0.0f, 0.0f) + rp.depth * Point3((1.0f - sin(theta) * cos(phi)), cos(theta), sin(theta) * sin(phi));
}


bool intersect_sphere(const RenderParams &rp, const Point3 &pos, const Point3 &dir, Point2 &point_in_gui, Point3 &intersection)
{
  const Point3 surfaceOrigin = Point3{0.f, 0.0f, 0.0f};
  const float radius = rp.depth;
  const float distanceToSurface = rayIntersectSphereDist(pos, dir, surfaceOrigin, radius);
  if (distanceToSurface < 0)
    return false;
  intersection = pos + distanceToSurface * dir;
  const float phi = safe_atan2(intersection.z, intersection.x) - HALFPI;
  const float theta = acosf(intersection.y / radius) - HALFPI;
  point_in_gui.x = rp.width * (.5f - phi / (rp.vFov * rp.aspect));
  point_in_gui.y = rp.height * (.5f + theta / rp.vFov);
  return is_point_within_gui(rp, point_in_gui);
}


Point3 map_to_plane(const RenderParams &rp, const Point2 &point)
{
  const float y = rp.planeScale * (point.y - .5f);
  const float z = rp.planeScale * rp.aspect * (point.x - .5f);
  return Point3{-rp.depth, y, z};
}


bool intersect_plane(const RenderParams &rp, const Point3 &pos, const Point3 &dir, Point2 &point_in_gui, Point3 &intersection)
{
  const Plane3 plane{Point3{0.f, 0.f, -1.f}, rp.depth};
  intersection = plane.calcLineIntersectionPoint(pos, rp.depth * dir);
  if (intersection == pos)
    return false;

  point_in_gui.x = rp.width * (intersection.x / (rp.planeScale * rp.aspect) + .5f);
  point_in_gui.y = rp.height * (.5f - intersection.y / rp.planeScale);
  return is_point_within_gui(rp, point_in_gui);
}


Point3 map_to_cylinder(const RenderParams &rp, const Point2 &point)
{
  const float u = (point.x - 0.5f) * rp.aspect * rp.vFov - HALFPI;
  const float v = (point.y - 0.5f) * rp.vFov;
  return rp.depth * Point3{sin(u), v, cos(u)};
}


bool intersect_cylinder(const RenderParams &rp, const Point3 &pos, const Point3 &dir, Point2 &point_in_gui, Point3 &intersection)
{
  const float a = dir.x * dir.x + dir.z * dir.z;
  const float b = 2 * (dir.x * pos.x + dir.z * pos.z);
  const float c = pos.x * pos.x + pos.z * pos.z - rp.depth * rp.depth;
  float roots[2];
  if (!solve_square_equation(a, b, c, roots))
    return false;

  const float hit = (roots[0] < 0.f) ? roots[1] : roots[0];
  intersection = pos + hit * dir;
  const float phi = safe_atan2(intersection.z, intersection.x) - HALFPI;
  const float theta = atanf(intersection.y / rp.depth);
  point_in_gui.x = rp.width * (.5f - phi / (rp.vFov * rp.aspect));
  point_in_gui.y = rp.height * (.5f - theta / rp.vFov);
  return is_point_within_gui(rp, point_in_gui);
}

} // namespace


namespace vrgui
{

static const char *MAT_NAME = "vr_gui_surface";
static const char *BLK_NAME = "oculusGui";
static const char *TEX_VAR_NAME = "source_tex";
static const int VRGUI_GRID_SIZE = 32;

static RenderParams render_params;

static void fill_buffers()
{
  if (!render_params.vb)
    return;

  {
    struct Vertex
    {
      Point3 pos;
      Point2 texcoord;
    };
    G_ASSERT(sizeof(Vertex) == render_params.re.stride);
    Vertex *vertices;
    render_params.vb->lock(0, render_params.re.numVert * render_params.re.stride, (void **)&vertices, VBLOCK_WRITEONLY);

    for (unsigned int gridY = 0; gridY <= VRGUI_GRID_SIZE; gridY++)
      for (unsigned int gridX = 0; gridX <= VRGUI_GRID_SIZE; gridX++)
      {
        float tx0 = (float)gridX / VRGUI_GRID_SIZE;
        float ty0 = (float)gridY / VRGUI_GRID_SIZE;
        Point3 pos = map_to_surface(Point2(tx0, ty0));

        *vertices = {pos, {tx0, 1.0f - ty0}};
        vertices++;
      }

    render_params.vb->unlock();
  }

  {
    unsigned short *indices;
    render_params.ib->lock(0, render_params.re.numPrim * 3 * sizeof(short), &indices, VBLOCK_WRITEONLY);

    for (unsigned int gridY = 0; gridY < VRGUI_GRID_SIZE; gridY++)
      for (unsigned int gridX = 0; gridX < VRGUI_GRID_SIZE; gridX++)
      {
        indices[0] = gridX + gridY * (VRGUI_GRID_SIZE + 1);
        indices[1] = gridX + gridY * (VRGUI_GRID_SIZE + 1) + 1;
        indices[2] = gridX + (gridY + 1) * (VRGUI_GRID_SIZE + 1);
        indices[3] = indices[2];
        indices[4] = indices[1];
        indices[5] = gridX + (gridY + 1) * (VRGUI_GRID_SIZE + 1) + 1;

        indices += 6;
      }

    render_params.ib->unlock();
  }
}


bool init_surface(int ui_width, int ui_height, SurfaceCurvature curvature)
{
  G_ASSERTF_RETURN(render_params.mat == nullptr, false, "[VRUI] already inited");
  render_params = RenderParams{};
  render_params.applySettings(BLK_NAME, ui_width, ui_height, curvature);

  render_params.mat = new_shader_material_by_name(MAT_NAME);
  render_params.mat->addRef();
  render_params.texVarId = ::get_shader_variable_id(TEX_VAR_NAME);

  CompiledShaderChannelId channels[2] = {{SCTYPE_FLOAT3, SCUSAGE_POS, 0, 0}, {SCTYPE_FLOAT2, SCUSAGE_TC, 0, 0}};
  G_ASSERT(render_params.mat->checkChannels(channels, countof(channels)));

  auto &re = render_params.re;

  re.shElem = render_params.mat->make_elem();
  re.vDecl = dynrender::addShaderVdecl(channels, countof(channels));
  G_ASSERT_RETURN(render_params.re.vDecl != BAD_VDECL, false);
  re.stride = dynrender::getStride(channels, countof(channels));
  re.minVert = 0;
  re.numVert = (VRGUI_GRID_SIZE + 1) * (VRGUI_GRID_SIZE + 1);
  re.startIndex = 0;
  re.numPrim = VRGUI_GRID_SIZE * VRGUI_GRID_SIZE * 2;

  render_params.vb = d3d::create_vb(re.numVert * re.stride, 0, "vrGuiVb");
  G_ASSERTF_RETURN(render_params.vb != nullptr, false, "[VRUI] failed to create vertex buffer");

  render_params.ib = d3d::create_ib(re.numPrim * 3 * sizeof(short), 0, "vrGuiIb");
  G_ASSERTF_RETURN(render_params.ib != nullptr, false, "[VRUI] failed to create index buffer");
  fill_buffers();

  return true;
}


Point3 map_to_surface(const Point2 &point)
{
  switch (render_params.curvature)
  {
    case VRGUI_SURFACE_SPHERE: return map_to_sphere(render_params, point);
    case VRGUI_SURFACE_CYLINDER: return map_to_cylinder(render_params, point);
    case VRGUI_SURFACE_PLANE: return map_to_plane(render_params, point);

    case VRGUI_SURFACE_DEFAULT: break; // MUST not happen, but keep compilers quit
  }

  G_ASSERTF(0, "Must not be reachable: all curvature types already processed");
  return {0.f, 0.f, 0.f};
}


bool intersect(const TMatrix &pointer, Point2 &point_in_gui, Point3 &intersection)
{
  return intersect(pointer.getcol(3), pointer.getcol(2), point_in_gui, intersection);
}


bool intersect(const Point3 &pos, const Point3 &dir, Point2 &point_in_gui, Point3 &intersection)
{
  switch (render_params.curvature)
  {
    case VRGUI_SURFACE_SPHERE: return intersect_sphere(render_params, pos, dir, point_in_gui, intersection);
    case VRGUI_SURFACE_CYLINDER: return intersect_cylinder(render_params, pos, dir, point_in_gui, intersection);
    case VRGUI_SURFACE_PLANE: return intersect_plane(render_params, pos, dir, point_in_gui, intersection);

    case VRGUI_SURFACE_DEFAULT: break; // MUST not happen, but keep compilers quit
  }

  G_ASSERTF(0, "Must not be reachable: all curvature types already processed");
  return false;
}


float get_ui_depth() { return render_params.depth; }


void render_to_surface(TEXTUREID tex_id)
{
  G_ASSERT(render_params.mat);
  if (render_params.mat != nullptr)
  {
    // debug("[XR] rendering to %d/%p", tex_id, render_params.mat);
    ShaderGlobal::set_texture(render_params.texVarId, tex_id);
    d3d::setvdecl(render_params.re.vDecl);
    d3d::setvsrc(0, render_params.vb, render_params.re.stride);
    d3d::setind(render_params.ib);
    render_params.re.shElem->render(0, render_params.re.numVert, 0, render_params.re.numPrim);
    ShaderGlobal::set_texture(render_params.texVarId, BAD_TEXTUREID); // Workaround 'that wasnt properly removed'.
  }
}


void destroy_surface()
{
  if (render_params.mat)
  {
    render_params.mat->delRef();
    render_params.mat = nullptr;
    render_params.re.shElem = nullptr;
  }
  del_d3dres(render_params.vb);
  del_d3dres(render_params.ib);
}


bool is_inited() { return !!render_params.mat; }

void get_ui_surface_params(int &ui_width, int &ui_height, SurfaceCurvature &type)
{
  ui_width = render_params.width;
  ui_height = render_params.height;
  type = render_params.curvature;
}


static void vr_gui_after_reset_device(bool) { fill_buffers(); }

REGISTER_D3D_AFTER_RESET_FUNC(vr_gui_after_reset_device);

} // namespace vrgui
