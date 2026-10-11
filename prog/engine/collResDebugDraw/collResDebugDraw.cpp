// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include <debug/dag_collResDebugDraw.h>
#include <gameRes/dag_collisionResource.h>
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
#include <scene/dag_physMat.h>
#include <obsolete/dag_cfg.h>
#include <math/dag_TMatrix4.h>
#include <math/dag_TMatrix4more.h>
#include <math/dag_e3dColor.h>
#include <math/dag_Point3.h>
#include <vecmath/dag_vecMath.h>
#include <debug/dag_debug.h>
#include <shaders/dag_overrideStates.h>

#define MAX_VISCLIPMESH_FACETS 1024

struct VisClipMeshVertex
{
  Point3 pos;
  E3DCOLOR color;
};

static shaders::UniqueOverrideStateId clipMeshState;

// visualization clip mesh modes
enum
{
  VISCLIPMESH_INVALIDMODE,
  VISCLIPMESH_WIREFRAME,
  VISCLIPMESH_TRANSPARENT,
};

static int vcm_mode = VISCLIPMESH_INVALIDMODE;
static real vcm_rad = 5.0f;

static Vbuffer *vcm_vb = NULL;

void init_collres_debug_draw(CfgReader &cfg)
{
  cfg.getdiv("visclipmesh");

  if (!cfg.getbool("visclipmesh_use", true))
    return;

  vcm_rad = cfg.getreal("visclipmesh_radius", 75.0f);

  const char *str = cfg.getstr("visclipmesh_mode", "trans");

  if (!strcmp(str, "trans"))
    vcm_mode = VISCLIPMESH_TRANSPARENT;
  else if (!strcmp(str, "wire"))
    vcm_mode = VISCLIPMESH_WIREFRAME;
  else
    return;

  if (!cfg.getbool("visclipmesh_enable", true))
    return;

  vcm_vb = d3d::create_vb((MAX_VISCLIPMESH_FACETS * 6) * sizeof(VisClipMeshVertex), SBCF_DYNAMIC, __FILE__, RESTAG_DEBUG);
  d3d_err(vcm_vb);

  shaders::OverrideState state;

  if (!cfg.getbool("visclipmesh_ztest", true))
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
}

void term_collres_debug_draw()
{
  del_d3dres(vcm_vb);
  shaders::overrides::destroy(clipMeshState);
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


float get_collres_debug_draw_radius() { return vcm_rad; }

struct VcmPart
{
  VisClipMeshVertex *vcmv = nullptr;
  int faces = 0;
  bool lines = false;
};
static void vcm_lock_part(VcmPart &part)
{
  d3d_err(vcm_vb->lockEx(0, 0, &part.vcmv, VBLOCK_WRITEONLY | VBLOCK_DISCARD));
  part.faces = 0;
}
// the visitor writes each face's verts in draw order, so the part needs no index buffer
static void vcm_draw_part(const VcmPart &part)
{
  d3d_err(vcm_vb->unlock());
  if (!part.faces)
    return;
  d3d::draw(part.lines ? PRIM_LINELIST : PRIM_TRILIST, 0, part.faces * (part.lines ? 3 : 1));
}

void draw_collres_debug(const CollisionResource &res, bbox3f_cref box, uint16_t behavior_filter, bool wire)
{
  if (!vcm_vb)
    return;

  start_render_clipmesh();
  d3d::setvsrc(0, vcm_vb, sizeof(VisClipMeshVertex));

  VcmPart part;
  part.lines = wire;
  vcm_lock_part(part);
  res.visitTrianglesInBox(box, behavior_filter, [&part](vec3f a, vec3f b, vec3f c, int phys_mat_id, int) {
    if (part.faces == MAX_VISCLIPMESH_FACETS)
    {
      vcm_draw_part(part);
      vcm_lock_part(part);
    }
    // a face whose node carries no own material answers PHYSMAT_INVALID, which the table refuses
    const E3DCOLOR color = PhysMat::getMaterial(phys_mat_id < 0 ? PHYSMAT_DEFAULT : phys_mat_id).vcm_color;
    Point3 p3[3];
    v_stu_p3(&p3[0].x, a);
    v_stu_p3(&p3[1].x, b);
    v_stu_p3(&p3[2].x, c);
    for (int i = 0; i < 3; ++i)
    {
      part.vcmv->pos = p3[i];
      part.vcmv->color = color;
      part.vcmv++;
      if (part.lines)
      {
        part.vcmv->pos = p3[(i > 0) ? (i - 1) : 2];
        part.vcmv->color = color;
        part.vcmv++;
      }
    }
    ++part.faces;
    return false; //-V657 draw everything the box holds
  });
  vcm_draw_part(part);
  end_render_clipmesh();
}
