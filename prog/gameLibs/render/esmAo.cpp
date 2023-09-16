#include <render/esmAo.h>
#include <3d/dag_drv3d.h>
#include <3d/dag_quadIndexBuffer.h>

ECS_REGISTER_BOXED_TYPE(EsmAoManager, nullptr);

static int esm_ao_target_sizeVarId = -1;
static int esm_ao_view_posVarId = -1;

bool EsmAoManager::init(int resolution, float esm_exp)
{
  Sbuffer *buf = d3d_buffers::create_one_frame_cb(dag::buffers::cb_array_reg_count<EsmAoDecal>(MAX_ESM_AO_DECALS), "esm_ao_decals");
  if (!buf)
    return false;

  esmAoDecalsBuf.set(buf, "esm_ao_decals");
  esmAoDecalsBuf.setVar();

  index_buffer::init_box();
  esmShadows.init(resolution, resolution, MAX_TEXTURES, esm_exp);
  esmAoDecalsRenderer.init("esm_ao_decal_render", nullptr, 0, "esm_ao_decal_render", false);
  initAoDecalsStateId();

  esm_ao_target_sizeVarId = get_shader_variable_id("esm_ao_target_size");
  esm_ao_view_posVarId = get_shader_variable_id("esm_ao_view_pos");

  return true;
}

void EsmAoManager::close()
{
  index_buffer::release_box();
  esmShadows.close();
  esmAoDecalsBuf.close();
  esmAoDecalsRenderer.close();
}

int EsmAoManager::getTexId(const char *id_name)
{
  int id = idNames.getNameId(id_name);
  if (id < 0 && idNames.nameCount() < MAX_TEXTURES)
    id = idNames.addNameId(id_name);
  return id;
}

void EsmAoManager::beginRenderTexture(int tex_id, const Point3 &pos, const Point3 &frustum_size)
{
  G_ASSERT_RETURN(tex_id >= 0 && tex_id < MAX_TEXTURES, );

  TMatrix invViewTm = getInvViewTm(pos, Point3(1, 0, 0), Point3(0, -1, 0));
  TMatrix viewTm = orthonormalized_inverse(invViewTm);
  d3d::settm(TM_PROJ, getProjTm(frustum_size));
  d3d::settm(TM_VIEW, viewTm);
  esmShadows.beginRenderSlice(tex_id);
}

void EsmAoManager::endRenderTexture() { esmShadows.endRenderSlice(); }

void EsmAoManager::clearDecals() { decalCnt = 0; }

void EsmAoManager::addDecal(int tex_id, const Point3 &pos, const Point3 &up, const Point3 &frustum_dir, const Point3 &frustum_size)
{
  G_ASSERT_RETURN(tex_id >= 0 && tex_id < MAX_TEXTURES, );
  if (decalCnt >= MAX_ESM_AO_DECALS)
    return;

  TMatrix invViewTm = getInvViewTm(pos, up, frustum_dir);
  esmAoDecals[decalCnt].tmRow0 = float3(2.0f * safeinv(frustum_size.x) * invViewTm.getcol(0));
  esmAoDecals[decalCnt].tmRow1 = float3(2.0f * safeinv(-frustum_size.y) * invViewTm.getcol(1));
  esmAoDecals[decalCnt].tmRow2 = float3(2.0f * safeinv(-frustum_size.z) * invViewTm.getcol(2));
  esmAoDecals[decalCnt].center = float3(pos);
  esmAoDecals[decalCnt].texSliceId = tex_id;
  ++decalCnt;
}

void EsmAoManager::applyAoDecals(int target_width, int target_height, const Point3 &view_pos)
{
  if (decalCnt <= 0)
    return;
  esmAoDecalsBuf.getBuf()->updateData(0, sizeof(*esmAoDecals.data()) * decalCnt, esmAoDecals.data(), VBLOCK_DISCARD);
  ShaderGlobal::set_color4(esm_ao_target_sizeVarId, Color4(target_width, target_height, 0.0f, 0.0f));
  ShaderGlobal::set_color4(esm_ao_view_posVarId, view_pos.x, view_pos.y, view_pos.z, 0.0f);
  shaders::overrides::set(aoDecalsStateId);
  esmAoDecalsRenderer.shader->setStates();
  index_buffer::use_box();
  d3d::drawind_instanced(PRIM_TRILIST, 0, 12, 0, decalCnt);
  shaders::overrides::reset();
}

TMatrix EsmAoManager::getInvViewTm(const Point3 &center, const Point3 &up, const Point3 &frustum_dir)
{
  TMatrix invViewTm;
  invViewTm.setcol(2, normalize(frustum_dir));
  invViewTm.setcol(0, normalize(cross(up, invViewTm.getcol(2))));
  invViewTm.setcol(1, cross(invViewTm.getcol(2), invViewTm.getcol(0)));
  invViewTm.setcol(3, center);
  return invViewTm;
}

TMatrix EsmAoManager::getProjTm(const Point3 &frustum_size)
{
  float invFrustumZ = safeinv(frustum_size.z);
  TMatrix projTm = TMatrix::IDENT;
  projTm[0][0] = safeinv(frustum_size.x * 0.5f);
  projTm[1][1] = safeinv(frustum_size.y * 0.5f);
  projTm[2][2] = -invFrustumZ;
  projTm[3][2] = 0.5f;
  return projTm;
}

void EsmAoManager::initAoDecalsStateId()
{
  shaders::OverrideState state;
  state.set(shaders::OverrideState::FLIP_CULL);
  state.set(shaders::OverrideState::BLEND_OP);
  state.set(shaders::OverrideState::BLEND_SRC_DEST);
  state.blendOp = BLENDOP_MIN;
  state.sblend = BLEND_ONE;
  state.dblend = BLEND_ONE;
  aoDecalsStateId = shaders::overrides::create(state);
}