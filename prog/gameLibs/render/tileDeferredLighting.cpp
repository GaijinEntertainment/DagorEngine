#include <render/tileDeferredLighting.h>
#include <3d/dag_drv3d.h>
#include <3d/dag_tex3d.h>
#include <shaders/dag_shaders.h>
#include <vecmath/dag_vecMath.h>
#include <math/dag_vecMathCompatibility.h>
#include <math/dag_TMatrix4.h>
#include <perfMon/dag_statDrv.h>

#define GLOBAL_VARS_LIST \
  VAR(allLights)         \
  VAR(far_depth)         \
  VAR(lightsCount)       \
  VAR(viewClip)          \
  VAR(rendering_res)     \
  VAR(lights_indices_size)

#define GLOBAL_VARS_OPT_LIST   \
  VAR(near_depth)              \
  VAR(all_lights_view_pos_rad) \
  VAR(all_lights_box_center)   \
  VAR(all_lights_box_extent)   \
  VAR(view_lights_box_bmin)    \
  VAR(view_lights_box_bmax)

#define VAR(a) static int a##VarId = -1;
GLOBAL_VARS_LIST
GLOBAL_VARS_OPT_LIST
#undef VAR

TileDeferredLighting::TileDeferredLighting()
{
#define VAR(a) a##VarId = get_shader_variable_id(#a);
  GLOBAL_VARS_LIST
#undef VAR
#define VAR(a) a##VarId = get_shader_variable_id(#a, true);
  GLOBAL_VARS_OPT_LIST
#undef VAR
  width = height = 0;
  currentAllLightsId = 0;
}

void TileDeferredLighting::closeAllLights()
{
  ShaderGlobal::set_texture(allLightsVarId, BAD_TEXTUREID);
  for (int i = 0; i < allLights.size(); ++i)
    allLights[i].close();
}

void TileDeferredLighting::initAllLights()
{
  closeAllLights();
  for (int i = 0; i < allLights.size(); ++i)
  {
    String name(128, "allLights%d", i);
    allLights[i].set(d3d::create_tex(NULL, OmniLightsManager::MAX_LIGHTS * 2, 1, TEXFMT_A32B32G32R32F | TEXCF_DYNAMIC, 1, name), name);
    allLights[i].getTex2D()->texfilter(TEXFILTER_POINT);
  }
}

bool TileDeferredLighting::init(int w, int h)
{
  width = w;
  height = h;
  int classifyW = w / TILE_SIZE, classifyH = h / TILE_SIZE;

  ShaderGlobal::set_color4(lights_indices_sizeVarId, classifyW, classifyH, 1.0f / classifyW, 1.0f / classifyH);

  ArrayTexture *lightsArray = d3d::create_array_tex(classifyW, classifyH, MAX_LIGHTS_PER_TILE / 4, TEXCF_RTARGET, 1, "lights_indices");
  if (!lightsArray)
    return false;
  lightsIndicesArray.set(lightsArray, "lights_indices");
  lightsIndicesArray.getArrayTex()->texaddr(TEXADDR_CLAMP);
  lightsIndicesArray.getArrayTex()->texfilter(TEXFILTER_POINT);

  prepareLightCount.init("prepare_lights_count");
  initAllLights();
  mem_set_0(allLightsCount);
  currentAllLightsId = 0;
  return true;
}

void TileDeferredLighting::close()
{
  closeAllLights();
  lightsIndicesArray.close();
  prepareLightCount.clear();
  ShaderGlobal::set_texture(allLightsVarId, BAD_TEXTUREID);
}

void TileDeferredLighting::classifyPointLights(const TextureIDPair &far_depth, const TextureIDPair &close_depth)
{
  if (!allLightsCount[currentAllLightsId])
    return;
  TIME_D3D_PROFILE(classify_point_lights);
  Driver3dRenderTarget prevRT;
  d3d::get_render_target(prevRT);

  // debug("lights = %d", rawLightsIn.size());
  ShaderGlobal::set_texture(near_depthVarId, close_depth.getId());
  ShaderGlobal::set_texture(far_depthVarId, far_depth.getId());

  TMatrix4_vec4 projTm;
  d3d::gettm(TM_PROJ, &projTm);
  ShaderGlobal::set_color4(viewClipVarId, projTm[0][0], -projTm[1][1], projTm[2][0], projTm[2][1]);


  d3d::set_render_target();
  for (int i = 0; i < MAX_LIGHTS_PER_TILE / 4; ++i)
    d3d::set_render_target(i, lightsIndicesArray.getArrayTex(), i, 0);

  far_depth.getTex2D()->texaddr(TEXADDR_CLAMP);
  far_depth.getTex2D()->texmiplevel(3, 3);
  G_ASSERT(far_depth.getTex2D()->level_count() >= 3);
  if (close_depth.getTex2D())
  {
    close_depth.getTex2D()->texaddr(TEXADDR_CLAMP);
    close_depth.getTex2D()->texmiplevel(3, 3);
    G_ASSERT(close_depth.getTex2D()->level_count() >= 3);
  }
#if DAGOR_DBGLEVEL > 0
  TextureInfo dinfo;
  far_depth.getTex2D()->getinfo(dinfo, 3);
  TextureInfo linfo;
  lightsIndicesArray.getArrayTex()->getinfo(linfo, 0);
  G_ASSERTF(dinfo.w == linfo.w && dinfo.h == linfo.h, "dinfo.w = %d, linfo.w=%d", dinfo.w, linfo.w);
#endif
  ShaderGlobal::set_color4(rendering_resVarId, width, height, 1.0 / width, 1.0 / height);

  prepareLightCount.render();

  d3d::set_render_target(prevRT);
  far_depth.getTex2D()->texmiplevel(-1, -1);
  far_depth.getTex2D()->texaddr(TEXADDR_BORDER);

  if (close_depth.getTex2D())
  {
    close_depth.getTex2D()->texmiplevel(-1, -1);
    close_depth.getTex2D()->texaddr(TEXADDR_BORDER);
  }

  lightsIndicesArray.setVar();
}

int TileDeferredLighting::preparePointLights(
  const StaticTab<OmniLightsManager::RawLight, OmniLightsManager::MAX_LIGHTS> &visibleLights)
{
  if (!lightsIndicesArray.getArrayTex())
    return 0;

  if (!visibleLights.size())
  {
    ShaderGlobal::set_texture(lightsIndicesArray.getVarId(), BAD_TEXTUREID);
    ShaderGlobal::set_int(lightsCountVarId, 0);
    return 0;
  }
  TIME_PROFILE(classifyPointLights);

  TMatrix4_vec4 viewTm;
  d3d::gettm(TM_VIEW, &viewTm);
  bbox3f lightBox, worldLightBox;
  v_bbox3_init_empty(lightBox);
  v_bbox3_init_empty(worldLightBox);
  mat44f viewtm = *(mat44f *)&viewTm;
  for (int i = 0; i < visibleLights.size(); ++i)
  {
    vec4f pos_rad = v_ld(&visibleLights[i].pos_radius.x);
    v_bbox3_add_pt(worldLightBox, v_add(pos_rad, v_splat_w(pos_rad)));
    v_bbox3_add_pt(worldLightBox, v_sub(pos_rad, v_splat_w(pos_rad)));
    vec4f viewpos = v_mat44_mul_vec3p(viewtm, pos_rad);
    v_bbox3_add_pt(lightBox, v_add(viewpos, v_splat_w(pos_rad)));
    v_bbox3_add_pt(lightBox, v_sub(viewpos, v_splat_w(pos_rad)));
  }
  vec4f v_all_lights_view_pos_and_rad =
    v_perm_xyzd(v_bbox3_center(lightBox), v_length3(v_sub(lightBox.bmax, v_bbox3_center(lightBox))));
  Point3_vec4 all_lights_view_pos_and_rad;
  v_st(&all_lights_view_pos_and_rad.x, v_all_lights_view_pos_and_rad);

  ShaderGlobal::set_color4(all_lights_view_pos_radVarId, Color4(&all_lights_view_pos_and_rad.x));
  CvtStorage s;
  ShaderGlobal::set_color4(all_lights_box_centerVarId, Color4::xyzw(cvt_point4(v_bbox3_center(worldLightBox), s)));
  ShaderGlobal::set_color4(all_lights_box_extentVarId,
    Color4::xyzw(cvt_point4(v_sub(worldLightBox.bmax, v_bbox3_center(worldLightBox)), s)));

  ShaderGlobal::set_color4(view_lights_box_bminVarId, Color4::xyzw(cvt_point4(lightBox.bmin, s)));
  ShaderGlobal::set_color4(view_lights_box_bmaxVarId, Color4::xyzw(cvt_point4(lightBox.bmax, s)));

  int stride;
  OmniLightsManager::RawLight *lights;
  int nextAllLightsId = (currentAllLightsId + 1) % allLights.size();
  if (allLights[nextAllLightsId].getTex2D()->lockimg((void **)&lights, stride, 0, TEXLOCK_DISCARD | TEXLOCK_WRITE))
  {
    memcpy(lights, &visibleLights[0], data_size(visibleLights));
    allLights[nextAllLightsId].getTex2D()->unlockimg();
    allLightsCount[nextAllLightsId] = visibleLights.size();
    currentAllLightsId = nextAllLightsId;
  }

  ShaderGlobal::set_texture(allLightsVarId, allLights[currentAllLightsId].getId());
  ShaderGlobal::set_int(lightsCountVarId, allLightsCount[currentAllLightsId]);
  return 1;
}
