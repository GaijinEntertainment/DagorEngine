// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include <perfMon/dag_visClipMesh.h>
#include <debug/dag_debug3d.h>

// #include <util/dag_console.h>
#include <drv/3d/dag_renderStates.h>
#include <drv/3d/dag_draw.h>
#include <drv/3d/dag_vertexIndexBuffer.h>
#include <drv/3d/dag_matricesAndPerspective.h>
#include <drv/3d/dag_shaderConstants.h>
#include <drv/3d/dag_shader.h>
#include <drv/3d/dag_driver.h>
#include <drv/3d/dag_buffers.h>
#include <drv/3d/dag_info.h>
#include <3d/dag_render.h>
#include <shaders/dag_shaderBlock.h>
#include <gui/dag_stdGuiRender.h>
#include <scene/dag_physMat.h>
#include <scene/dag_frtdump.h>
#include <scene/dag_frtdumpMgr.h>
#include <generic/dag_span.h>
#include <generic/dag_initOnDemand.h>
#include <obsolete/dag_cfg.h>
#include <sceneRay/dag_sceneRay.h>
#include <math/dag_TMatrix4.h>
#include <math/dag_TMatrix4more.h>
#include <math/dag_bounds2.h>
#include <debug/dag_debug.h>
#include <shaders/dag_overrideStates.h>

static shaders::UniqueOverrideStateId clipMeshState;

// visualization clip mesh modes
enum
{
  VISCLIPMESH_INVALIDMODE,
  VISCLIPMESH_WIREFRAME,
  VISCLIPMESH_TRANSPARENT,
  VISCLIPMESH_DEFAULT = VISCLIPMESH_WIREFRAME
};


////////////////////////////////////////////////////////////////////////////////////////////////////
static bool vcm_enable = false;
static bool vcm_zenable = true;
static int vcm_mode = VISCLIPMESH_INVALIDMODE;
static real vcm_rad = 5.0f;
static bool vcm_use = true;
static bool vcm_lines = true;

static bool displayInfo = false;

////////////////////////////////////////////////////////////////////////////////////////////////////
static Vbuffer *vcm_vb = NULL;
static Ibuffer *vcm_ib = NULL;

bool create_visclipmesh(CfgReader &cfg, bool for_game)
{
  cfg.getdiv("visclipmesh");

  vcm_use = cfg.getbool("visclipmesh_use", true);
  if (!vcm_use)
    return false; //< it doesn't mean anything

  vcm_enable = cfg.getbool("visclipmesh_enable", true);

  vcm_zenable = cfg.getbool("visclipmesh_ztest", true);

  vcm_rad = cfg.getreal("visclipmesh_radius", 75.0f);

  const char *str = cfg.getstr("visclipmesh_mode", "trans");

  if (!strcmp(str, "trans"))
    vcm_mode = VISCLIPMESH_TRANSPARENT;
  else if (!strcmp(str, "wire"))
    vcm_mode = VISCLIPMESH_WIREFRAME;
  else
    vcm_enable = false;

  if (for_game)
  {
    vcm_mode = VISCLIPMESH_TRANSPARENT;
    vcm_lines = false;
  }

  vcm_vb = d3d::create_vb((MAX_VISCLIPMESH_FACETS * 6) * sizeof(VisClipMeshVertex), SBCF_DYNAMIC, __FILE__);
  d3d_err(vcm_vb);
  vcm_ib = d3d::create_ib(MAX_VISCLIPMESH_FACETS * 3 * sizeof(uint16_t), SBCF_DYNAMIC, "visClipMesh_ib");
  d3d_err(vcm_ib);

  // vcm_consoleproc.demandInit();
  // add_con_proc(vcm_consoleproc);

  if (!vcm_enable)
    return false;

  shaders::OverrideState state;

  if (!vcm_zenable)
    state.set(shaders::OverrideState::Z_TEST_DISABLE);
  state.set(shaders::OverrideState::Z_WRITE_DISABLE);
  state.set(shaders::OverrideState::BLEND_SRC_DEST_A);
  state.sblenda = BLEND_ZERO;
  state.dblenda = BLEND_ZERO;

  if (vcm_mode == VISCLIPMESH_TRANSPARENT)
  {
    state.set(shaders::OverrideState::BLEND_SRC_DEST);
    state.sblend = BLEND_SRCALPHA;
    state.dblend = BLEND_INVSRCALPHA;

    state.dblenda = BLEND_INVSRCALPHA;
  }

  state.set(shaders::OverrideState::Z_BIAS);
  state.zBias = -0.00001;

  clipMeshState = shaders::overrides::create(state);

  return true;
}

void delete_visclipmesh(void)
{
  if (!vcm_use)
    return;

  del_d3dres(vcm_vb);
  del_d3dres(vcm_ib);

  shaders::overrides::destroy(clipMeshState);
}

void render_visclipmesh_info(bool need_start_render)
{
  if (!vcm_use)
    return;

  if (!vcm_enable || !displayInfo)
    return;

  if (need_start_render)
    StdGuiRender::start_render();

  const real borderSize = 10.0f;
  const real rowSpacing = 10.0f;
  const real pictureWidth = 20.0f;
  const Point2 startPos = Point2(20.0f, 20.0f);

  const int matCount = PhysMat::physMatCount();
  const real rowWidth = StdGuiRender::get_char_bbox_u('W').width().x * 20 + borderSize + pictureWidth;
  const real matNameOfs = borderSize * 2 + pictureWidth;
  const real bfColOfs = StdGuiRender::get_char_bbox_u('W').width().x * 10 + borderSize * 2 + pictureWidth;
  const real rowHeight = StdGuiRender::get_char_bbox_u('A').width().y;

  const E3DCOLOR backColor = E3DCOLOR(200, 200, 200, 200);
  const E3DCOLOR sepColor = E3DCOLOR(0, 0, 0, 200);

  // back
  StdGuiRender::solid_frame(startPos.x, startPos.y, startPos.x + rowWidth + borderSize * 2,
    startPos.y + (rowHeight + rowSpacing) * (matCount - 1) + rowHeight + borderSize * 2, 1, backColor, sepColor);

  Point2 rowPos = Point2(startPos.x + borderSize, startPos.y + borderSize);
  for (int i = 0; i < matCount; i++)
  {
    // separator
    if (i < matCount - 1)
    {
      StdGuiRender::set_color(sepColor);
      StdGuiRender::draw_line(rowPos.x, rowPos.y + rowHeight + rowSpacing / 2, rowPos.x + rowWidth,
        rowPos.y + rowHeight + rowSpacing / 2);
    }

    const PhysMat::MaterialData &mat = PhysMat::getMaterial(i);

    // picture
    StdGuiRender::solid_frame(rowPos.x, rowPos.y, rowPos.x + pictureWidth, rowPos.y + rowHeight, 1, mat.vcm_color, sepColor);

    // name with shadow
    StdGuiRender::set_color(E3DCOLOR(0, 0, 0, 255));
    StdGuiRender::goto_xy(rowPos.x + matNameOfs + 1, rowPos.y + rowHeight + 1);
    StdGuiRender::draw_strf("%d: %s", i, (const char *)mat.name, PhysMat::getBFClassName(mat.bfid));

    StdGuiRender::goto_xy(rowPos.x + bfColOfs + 1, rowPos.y + rowHeight + 1);
    StdGuiRender::draw_str(PhysMat::getBFClassName(mat.bfid));

    StdGuiRender::set_color(mat.vcm_color);
    StdGuiRender::goto_xy(rowPos.x + matNameOfs, rowPos.y + rowHeight);
    StdGuiRender::draw_strf("%d: %s", i, (const char *)mat.name, PhysMat::getBFClassName(mat.bfid));

    StdGuiRender::goto_xy(rowPos.x + bfColOfs, rowPos.y + rowHeight);
    StdGuiRender::draw_str(PhysMat::getBFClassName(mat.bfid));

    rowPos.y += rowHeight + rowSpacing;
  }

  if (need_start_render)
    StdGuiRender::end_render();
}


static void start_render_clipmesh(void)
{
  G_ASSERT(ShaderGlobal::getBlock(ShaderGlobal::LAYER_FRAME) == -1);

  shaders::overrides::set(clipMeshState);

  d3d::set_program(d3d::get_debug_program());

  TMatrix4_vec4 gtm;
  d3d::settm(TM_WORLD, &Matrix44::IDENT);
  d3d::getglobtm(gtm);

  process_tm_for_drv_consts(gtm);
  d3d::set_vs_const(0, gtm[0], 4);

  if (vcm_mode == VISCLIPMESH_WIREFRAME)
    d3d::setwire(1);
}

static void end_render_clipmesh(void)
{

  shaders::overrides::reset();

  if (vcm_mode == VISCLIPMESH_WIREFRAME)
    d3d::setwire(::grs_draw_wire);
  d3d::set_program(BAD_PROGRAM);
}


float get_vcm_rad() { return vcm_rad; }

void set_vcm_rad(float rad) { vcm_rad = rad; }

bool is_vcm_visible() { return vcm_enable; }
void set_vcm_visible(bool visible) { vcm_enable = visible; }

int set_vcm_draw_type(int type)
{
  int prev = vcm_lines;
  vcm_lines = type;
  return prev;
}

//==============================================================================
void render_visclipmesh(const FastRtDump &cliprtr, const Point3 &pos)
{
  if (!vcm_use || !vcm_enable || !vcm_vb || !cliprtr.isDataValid())
    return;

  StaticSceneRayTracer &rt = cliprtr.getRt();
  Tab<int> faces(tmpmem);

  if (!rt.getFaces(faces, pos, vcm_rad) || faces.empty())
    return;

  start_render_clipmesh();
  d3d::setvsrc(0, vcm_vb, sizeof(VisClipMeshVertex));

  int firstFace = 0;
  const int partCount = (faces.size() + MAX_VISCLIPMESH_FACETS - 1) / MAX_VISCLIPMESH_FACETS;

  for (int partId = 1; partId <= partCount; ++partId)
  {
    VisClipMeshVertex *vcmv = NULL;
    const int lastFace = partId * MAX_VISCLIPMESH_FACETS;
    const int faceCount = (faces.size() < lastFace) ? faces.size() : lastFace;

    d3d_err(vcm_vb->lockEx(0, 0, &vcmv, VBLOCK_WRITEONLY | VBLOCK_DISCARD));

    for (int faceId = firstFace; faceId < faceCount; ++faceId)
    {
      const PhysMat::MaterialData &mat = PhysMat::getMaterial(cliprtr.getFacePmid(faces[faceId]));

      E3DCOLOR color = mat.vcm_color;

      for (int i = 0; i < 3; ++i)
      {
        vcmv->pos = rt.verts(rt.faces(faces[faceId]).v[i]);
        vcmv->color = color;
        vcmv++;

        if (vcm_lines)
        {
          vcmv->pos = rt.verts(rt.faces(faces[faceId]).v[(i > 0) ? (i - 1) : 2]);
          vcmv->color = color;
          vcmv++;
        }
      }

      // vcmv += 3;
    }

    d3d_err(vcm_vb->unlock());
    if (vcm_lines)
      d3d::draw(PRIM_LINELIST, 0, (faceCount - firstFace) * 3);
    else
      d3d::draw(PRIM_TRILIST, 0, faceCount - firstFace);

    firstFace = faceCount;
  }

  end_render_clipmesh();
}

//==============================================================================
void render_visclipmesh(StaticSceneRayTracer &rt, const Point3 &pos)
{
  if (!vcm_use || !vcm_enable || !vcm_vb)
    return;

  G_UNUSED(pos);

  start_render_clipmesh();
  d3d::setvsrc(0, vcm_vb, sizeof(VisClipMeshVertex));
  d3d::setind(vcm_ib);

  int firstFace = 0;
  int facesCount = rt.getFacesCount();

  const int partCount = (facesCount + MAX_VISCLIPMESH_FACETS - 1) / MAX_VISCLIPMESH_FACETS;

  for (int partId = 1; partId <= partCount; ++partId)
  {
    VisClipMeshVertex *vcmv = NULL;
    uint16_t *indices = NULL;
    const int lastFace = partId * MAX_VISCLIPMESH_FACETS;
    const int faceCount = (facesCount < lastFace) ? facesCount : lastFace;

    d3d_err(vcm_vb->lockEx(0, 0, &vcmv, VBLOCK_WRITEONLY | VBLOCK_DISCARD));
    d3d_err(vcm_ib->lock(0, 0, (void **)&indices, VBLOCK_WRITEONLY | VBLOCK_DISCARD));

    uint16_t vertexNo = 0;

    for (int faceId = firstFace; faceId < faceCount; ++faceId)
    {
      E3DCOLOR color = E3DCOLOR(255, 100, 100, 128);

      for (int i = 0; i < 3; ++i)
      {
        vcmv->pos = rt.verts(rt.faces(faceId).v[i]);
        vcmv->color = color;
        vcmv++;
        *(indices) = vertexNo;
        indices++;

        vertexNo++;

        if (vcm_lines)
        {
          vcmv->pos = rt.verts(rt.faces(faceId).v[(i > 0) ? (i - 1) : 2]);
          vcmv->color = color;
          vcmv++;
        }
      }
    }

    d3d_err(vcm_vb->unlock());
    d3d_err(vcm_ib->unlock());
    if (vcm_lines)
      d3d::draw(PRIM_LINELIST, 0, (faceCount - firstFace) * 3);
    else
      d3d::drawind(PRIM_TRILIST, 0, (faceCount - firstFace), 0);

    firstFace = faceCount;
  }

  end_render_clipmesh();
}

//==============================================================================
void render_visclipmesh(const FastRtDumpManager &cliprtr, const Point3 &pos)
{
  if (!vcm_use || !vcm_enable || !vcm_vb)
    return;

  dag::ConstSpan<FastRtDump *> rtdumps = cliprtr.getRt();

  if (!rtdumps.size())
    return;

  Tab<int> faces(tmpmem);

  start_render_clipmesh();

  for (int i = 0; i < rtdumps.size(); ++i)
  {
    faces.clear();

    const FastRtDump &rtdump = *rtdumps[i];

    if (!rtdump.isDataValid())
      continue;
    if (!rtdump.isActive())
      continue;

    StaticSceneRayTracer &rt = rtdump.getRt();

    if (!rt.getFaces(faces, pos, vcm_rad) || faces.empty())
      continue;

    d3d::setvsrc(0, vcm_vb, sizeof(VisClipMeshVertex));

    int firstFace = 0;
    const int partCount = (faces.size() + MAX_VISCLIPMESH_FACETS - 1) / MAX_VISCLIPMESH_FACETS;

    for (int partId = 1; partId <= partCount; ++partId)
    {
      VisClipMeshVertex *vcmv = NULL;
      const int lastFace = partId * MAX_VISCLIPMESH_FACETS;
      const int faceCount = (faces.size() < lastFace) ? faces.size() : lastFace;

      d3d_err(vcm_vb->lockEx(0, 0, &vcmv, VBLOCK_WRITEONLY | VBLOCK_DISCARD));

      for (int faceId = firstFace; faceId < faceCount; ++faceId)
      {
        const PhysMat::MaterialData &mat = PhysMat::getMaterial(rtdump.getFacePmid(faces[faceId]));

        E3DCOLOR color = mat.vcm_color;

        if (vcm_mode != VISCLIPMESH_TRANSPARENT)
          color.a = 0;

        for (int j = 0; j < 3; ++j)
        {
          vcmv->pos = rt.verts(rt.faces(faces[faceId]).v[j]);
          vcmv->color = color;
          vcmv++;

          if (vcm_lines)
          {
            vcmv->pos = rt.verts(rt.faces(faces[faceId]).v[(j > 0) ? (j - 1) : 2]);
            vcmv->color = color;
            vcmv++;
          }
        }

        // vcmv += 3;
      }

      d3d_err(vcm_vb->unlock());
      if (vcm_lines)
        d3d::draw(PRIM_LINELIST, 0, (faceCount - firstFace) * 3);
      else
        d3d::draw(PRIM_TRILIST, 0, faceCount - firstFace);

      firstFace = faceCount;
    }
  }

  end_render_clipmesh();
}
