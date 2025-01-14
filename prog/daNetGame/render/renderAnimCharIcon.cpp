// Copyright (C) Gaijin Games KFT.  All rights reserved.

// #include <ecs/anim/anim.h>
#include <animChar/dag_animCharacter2.h>
#include <anim/dag_animPureInterface.h>
#include <gameRes/dag_gameResources.h>
#include <gameRes/dag_stdGameResId.h>
#include <EASTL/unique_ptr.h>
#include <EASTL/string.h>
#include <EASTL/vector.h>
#include <EASTL/vector_map.h>
#include <EASTL/hash_map.h>
#include <drv/3d/dag_viewScissor.h>
#include <drv/3d/dag_renderTarget.h>
#include <drv/3d/dag_matricesAndPerspective.h>
#include <drv/3d/dag_driver.h>
#include <drv/3d/dag_tex3d.h>
#include <3d/dag_resPtr.h>
#include <math/dag_frustum.h>
#include <math/dag_mathAng.h>
#include <math/dag_mathUtils.h>
#include <math/dag_math3d.h>
#include <ioSys/dag_dataBlock.h>
#include <render/deferredRenderer.h>
#include <shaders/dag_shaderBlock.h>
#include <shaders/dag_dynSceneRes.h>
#include <generic/dag_initOnDemand.h>
#include <memory/dag_framemem.h>

#include <3d/dag_picMgr.h>
#include <3d/dag_texIdSet.h>

#include <shaders/dag_postFxRenderer.h>
#include <perfMon/dag_statDrv.h>
#include "world/global_vars.h"

#include <3d/dag_render.h> //for global state

#include "render/animCharTexReplace.h"

// for async loading
#include <osApiWrappers/dag_cpuJobs.h>
#include <osApiWrappers/dag_critSec.h>
#include <ecs/gameres/commonLoadingJobMgr.h>
#include <ecs/render/animCharUtils.h>
#include <startup/dag_globalSettings.h>

#include <ecs/anim/animchar_visbits.h>
#include "world/dynModelRenderPass.h"
#include "world/dynModelRenderer.h"
#define INSIDE_RENDERER 1
#include "world/private_worldRenderer.h"
#include "world/wrDispatcher.h"
#include <ecs/render/renderPasses.h>
#include "EASTL/string.h"
#include "math/dag_TMatrix.h"

using namespace dynmodel_renderer;
extern int dynamicSceneBlockId;
extern int dynamicSceneTransBlockId;
static void reset_async_load_animchar_queue();

// TODO: This is a copy of set_paint_color function from paint_color.das
//       Remove this copy later, when it will cease to be used in the cpp
static void set_paint_color(const Point4 &paintColor, AnimV20::AnimcharRendComponent &animchar_render)
{
  static int paintColorVarId = get_shader_variable_id("paint_color", true);
  if (paintColorVarId < 0)
    return;

  const float gamma = 2.2f;
  const Color4 color = Color4(pow(paintColor.x, gamma), pow(paintColor.y, gamma), pow(paintColor.z, gamma), paintColor.w);

  recreate_material_with_new_params(animchar_render, [&](ShaderMaterial *mat) { mat->set_color4_param(paintColorVarId, color); });
}

static void set_camouflage_override(const DataBlock &camouflage_override, AnimV20::AnimcharRendComponent &animchar_render)
{
  static int material_idVarId = get_shader_variable_id("material_id", true);
  static int camouflage_scale_and_offsetVarId = get_shader_variable_id("camouflage_scale_and_offset", true);
  static int solid_fill_colorVarId = get_shader_variable_id("solid_fill_color", true);
  static int color_multiplierVarId = get_shader_variable_id("color_multiplier", true);
  static int camouflage_texVarId = get_shader_variable_id("camouflage_tex", true);

  for (int i = 0; i < camouflage_override.blockCount(); ++i)
  {
    const DataBlock *materialBlk = camouflage_override.getBlock(i);
    int materialId = materialBlk->getInt("material_id", -1);
    Point4 camouflageScaleAndOffset = materialBlk->getPoint4("camouflage_scale_and_offset", Point4(1, 1, 0, 0));
    Point4 solidFillColor = materialBlk->getPoint4("solid_fill_color", Point4::ONE);
    float colorMultiplier = materialBlk->getReal("color_multiplier", 1);
    bool forceSolidFill = materialBlk->getBool("force_solid_fill", false);

    recreate_material_with_new_params(animchar_render, "dynamic_sheen_camo", [&](ShaderMaterial *mat) {
      int currentMaterialId = -1;
      mat->getIntVariable(material_idVarId, currentMaterialId);
      if (materialId != currentMaterialId)
        return;
      mat->set_color4_param(camouflage_scale_and_offsetVarId, Color4::xyzw(camouflageScaleAndOffset));
      mat->set_color4_param(solid_fill_colorVarId, Color4::xyzw(solidFillColor));
      mat->set_real_param(color_multiplierVarId, colorMultiplier);
      if (forceSolidFill)
        mat->set_texture_param(camouflage_texVarId, BAD_TEXTUREID);
    });
  }
}

typedef eastl::unique_ptr<GameResource, GameResDeleter> GameResPtr;

struct IconAnimchar
{
  enum
  {
    FULL,
    SILHOUETTE,
    SAME,
    UNKNOWN
  };

  void init(AnimV20::IAnimCharacter2 *animchar_, const DataBlock *blk_)
  {
    animchar = animchar_;
    blk = blk_;
    slotId = AnimCharV20::addSlotId(blk_->getStr("slot", NULL));
    const float scale = blk_->getReal("scale", 1.f);
    scaleV = v_make_vec4f(scale, scale, scale, 1.0f);
    const char *shadingStr = blk->getStr("shading", "full");
    shading = strcmp(shadingStr, "silhouette") == 0 ? SILHOUETTE
              : strcmp(shadingStr, "full") == 0     ? FULL
              : strcmp(shadingStr, "same") == 0     ? SAME
                                                    : UNKNOWN;
    if (shading == UNKNOWN)
      logerr("unknown shading <%s> in animchar <%s> icon", shadingStr, blk->getStr("animchar"));
    silhouette = blk->getE3dcolor("silhouette", 0);
    outline = blk->getE3dcolor("outline", 0);
    calcBBox = blk->getBool("calcBBox", true);
    swapYZ = blk->getBool("swapYZ", true);
    const char *attachTypeStr = blk->getStr("attachType", "slot");
    attachType = dd_stricmp(attachTypeStr, "skeleton") == 0 ? AttachType::SKELETON
                 : dd_stricmp(attachTypeStr, "node") == 0   ? AttachType::NODE
                                                            : AttachType::SLOT;
    parentNode = blk->getStr("parentNode", "");
    relativeTm = blk->getTm("relativeTm", TMatrix::IDENT);
    const DataBlock *hideNodesBlk = blk_->getBlockByNameEx("hideNodes");
    for (int i = 0; i < hideNodesBlk->paramCount(); ++i)
      hideNodeNames.push_back(hideNodesBlk->getStr(i));
  }

  void updateTmWtm(AnimV20::IAnimCharacter2 *parent)
  {
    G_ASSERT(parent);
    G_ASSERT(animchar);

    bool recalc = true;
    const mat44f *wtm = parent->getSlotNodeWtm(slotId);
    const mat44f *tm = parent->getAttachmentTm(slotId);
    mat44f dest;
    if (wtm && tm)
    {
      v_mat44_mul(dest, *wtm, *tm);
      animchar->setTm(dest);
    }
    else if (wtm)
      animchar->setTm(*wtm);
    else
    {
      parent->getTm(dest);
      animchar->setTm(dest);
    }

    if (attachType == AttachType::SKELETON)
    {
      GeomNodeTree &nodeTree = animchar->getNodeTree();
      const GeomNodeTree &parentNodeTree = parent->getNodeTree();
      eastl::fixed_vector<dag::Index16, 32, true, framemem_allocator> nodesWithoutRemap, nodesWithRemap;
      for (dag::Index16 i(0), ie(nodeTree.nodeCount()); i != ie; ++i)
      {
        const char *nodeName = nodeTree.getNodeName(i);
        if (auto nodeIdx = parentNodeTree.findNodeIndex(nodeName))
        {
          nodeTree.getNodeTm(i) = parentNodeTree.getNodeTm(nodeIdx);
          nodeTree.getNodeWtmRel(i) = parentNodeTree.getNodeWtmRel(nodeIdx);
          nodesWithRemap.push_back(i);
        }
        else
          nodesWithoutRemap.push_back(i);
      }
      for (auto nodeNo : nodesWithoutRemap)
      {
        const auto parentNo = nodeTree.getParentNodeIdx(nodeNo);
        if (!parentNo)
          continue;

        for (auto otherNodeNo : nodesWithRemap)
          if (otherNodeNo == parentNo)
          {
            mat44f childWtm;
            v_mat44_mul43(childWtm, nodeTree.getNodeWtmRel(parentNo), nodeTree.getNodeTm(nodeNo));
            nodeTree.getNodeWtmRel(nodeNo) = childWtm;

            nodeTree.calcWtmForBranch(nodeNo);
          }
      }
      if (nodesWithRemap.empty())
      {
        // Skeleton attachments didn't work, use slot attachment.
      }
      else
      {
        // Copy over final transforms, DO NOT use animchar->recalcWtm(), it will
        // overwrite root node's transform, this is not what we want.
        animchar->copyNodes();
        recalc = false;
      }
    }
    else if (attachType == AttachType::NODE)
    {
      const GeomNodeTree &parentNodeTree = parent->getNodeTree();
      auto parentRootNode = parentNodeTree.findINodeIndex("root");
      G_ASSERT_RETURN(parentRootNode, );

      // Get parent wtm
      auto targetNode =
        dd_stricmp(parentNode.c_str(), "root") == 0 ? parentRootNode : parentNodeTree.findINodeIndex(parentNode.c_str());
      G_ASSERT_RETURN(targetNode, );

      // Save wtm into the root
      GeomNodeTree &nodeTree = animchar->getNodeTree();
      G_ASSERT_RETURN(!nodeTree.empty(), );

      // Calculate child wtm = parent wtm * relativeTm
      mat44f childWtm;
      v_mat44_make_from_43cu_unsafe(childWtm, relativeTm.m[0]); // get relative tm
      if (swapYZ)
        eastl::swap(childWtm.col1, childWtm.col2); // swap only cols, ignore rows

      v_mat44_mul43(childWtm, parentNodeTree.getNodeWtmRel(targetNode), childWtm); // multiply by targetNode wtm
      v_mat44_mul43(childWtm, childWtm, nodeTree.getRootTm());                     // multiply by root node tm: usual it is identity

      nodeTree.getRootWtmRel() = childWtm;

      // Recalculate wtm tree
      nodeTree.calcWtmForBranch(dag::Index16(0));
      animchar->copyNodes();
      recalc = false;
    }

    if (recalc)
    {
      animchar->getTm(dest);
      dest.col0 = v_mul(dest.col0, scaleV);
      dest.col1 = v_mul(dest.col1, scaleV);
      dest.col2 = v_mul(dest.col2, scaleV);
      animchar->setTm(dest);
      animchar->recalcWtm();
    }
  }

  AnimV20::IAnimCharacter2 *animchar = nullptr;
  const DataBlock *blk = nullptr; // non-null only when setting things up.
  int slotId = -1;
  vec4f scaleV = v_zero();
  int shading = UNKNOWN;
  E3DCOLOR silhouette = 0;
  E3DCOLOR outline = 0;
  bool calcBBox = true;
  bool swapYZ = true;
  enum struct AttachType
  {
    SLOT,
    SKELETON,
    NODE
  } attachType = AttachType::SLOT;
  eastl::string parentNode = "";
  TMatrix relativeTm = TMatrix::IDENT;
  eastl::vector<SharedTex> managedTextures;
  eastl::vector<eastl::string> hideNodeNames;
};

using IconAnimcharWithAttachments = eastl::vector<IconAnimchar>;

class RenderAnimCharIcon
{
public:
  RenderAnimCharIcon() = default;
  bool match(const DataBlock &pic_props, int &out_w, int &out_h);
  bool render(Texture *to, int x, int y, int w, int h, const DataBlock &info, PICTUREID pid);

  void clearPendReq()
  {
    reset_async_load_animchar_queue();
    WinAutoLock lock(pendReqCs);
    debug(__FUNCTION__);
    for (auto &m : pendReq)
      for (auto &ia : m.second)
        ia.animchar->destroy();
    pendReq.clear();
  }

protected:
  bool renderInternal(Texture *to, int x, int y, int w, int h, const DataBlock &info, PICTUREID pid);
  bool ensureDim(int w, int h);
  void initEnvi();
  bool isLoadedEnvi() const;
  eastl::unique_ptr<DeferredRenderTarget> target;
  UniqueTexHolder finalTarget;
  PostFxRenderer finalAA;
  SharedTex enviPanorama;
  eastl::vector_map<PICTUREID, IconAnimcharWithAttachments> pendReq;
  WinCritSec pendReqCs;
  bool enviInited = false;
  bool fullDeferred = true;
  bool mobileDeferred = false;
  bool isForward = false;

  void clear_to(E3DCOLOR col, Texture *to, int x, int y, int dstw, int dsth) const
  {
    SCOPE_RENDER_TARGET;
    d3d::set_render_target(to, 0);
    d3d::setview(x, y, dstw, dsth, 0, 1);
    d3d::clearview(CLEAR_TARGET, col, 0, 0);
    d3d::resource_barrier({to, RB_RO_SRV | RB_STAGE_PIXEL, 0, 0});
  }
};

bool RenderAnimCharIcon::isLoadedEnvi() const
{
  return (enviPanorama.getTexId() == BAD_TEXTUREID) ? true : prefetch_and_check_managed_texture_loaded(enviPanorama.getTexId(), true);
}

static class RenderAnimCharIconPF final : public PictureManager::PictureRenderFactory
{
public:
  InitOnDemand<RenderAnimCharIcon, false> ctx;

  virtual void registered() override { ctx.demandInit(); }
  virtual void unregistered() override
  {
    ctx->clearPendReq();
    ctx.demandDestroy();
  }
  virtual void clearPendReq() override { ctx ? ctx->clearPendReq() : (void)0; }
  virtual bool match(const DataBlock &pic_props, int &out_w, int &out_h) override
  {
    return !ctx || ctx->match(pic_props, out_w, out_h);
  }
  virtual bool render(Texture *to, int x, int y, int w, int h, const DataBlock &info, PICTUREID pid) override
  {
    return !ctx || ctx->render(to, x, y, w, h, info, pid);
  }
} render_animchar_icon_factory;

static inline float deg_to_fov(float deg) { return 1.f / tanf(DEG_TO_RAD * 0.5f * deg); }

static void get_animchar_tight_matrix(const DataBlock &info,
  IconAnimcharWithAttachments &iconAnimchar,
  float startHkAspect,
  TMatrix &finalViewTm,
  float &fovInTan,
  float &fovHkInTan,
  float &zn,
  float &zf,
  float &scaleX,
  float &scaleY,
  float &centerX,
  float &centerY)
{
  Quat quat;
  euler_to_quat(-DegToRad(info.getReal("yaw", 0)), -DegToRad(info.getReal("pitch", 0)), DegToRad(info.getReal("roll", 0)), quat);
  TMatrix viewItm = quat_to_matrix(quat);

  const char *animation = info.getStr("animation", "");
  AnimV20::AnimationGraph *iconAnimGraph = iconAnimchar[0].animchar->getAnimGraph();
  if (strcmp(animation, "") != 0 && iconAnimGraph)
  {
    const int stateIdx = iconAnimGraph->getStateIdx(animation);
    if (stateIdx < 0)
    {
      logerr("[AnimCharIcon] Failed to find anim state '%s' for animchar '%s'", animation, info.getStr("animchar"));
    }
    else
    {
      iconAnimGraph->enqueueState(*iconAnimchar[0].animchar->getAnimState(), iconAnimGraph->getState(stateIdx));
      AnimV20::IAnimBlendNode *node = iconAnimGraph->getBlendNodePtr(animation);
      if (node)
        node->seek(*iconAnimchar[0].animchar->getAnimState(), 0);
      iconAnimchar[0].animchar->doRecalcAnimAndWtm();
    }
  }

  TMatrix wtm;
  iconAnimchar[0].animchar->getTm(wtm);
  BBox3 localBox = iconAnimchar[0].animchar->getVisualResource()->getLocalBoundingBox();
  for (size_t i = 1; i < iconAnimchar.size(); ++i)
  {
    if (!iconAnimchar[i].calcBBox)
      continue;
    iconAnimchar[i].updateTmWtm(iconAnimchar[0].animchar);
    TMatrix cWtm;
    iconAnimchar[i].animchar->getTm(cWtm);
    BBox3 cBox = iconAnimchar[i].animchar->getVisualResource()->getLocalBoundingBox();
    localBox += inverse(wtm) * cWtm * cBox;
  }
  const float boxRad = length(localBox.width()) * 0.5f;
  const float dist = max(info.getReal("distance", info.getReal("distanceScale", 4.f) * boxRad), 1 * boxRad + 0.01f);
  const Point3 worldViewPos = localBox.center() - dist * viewItm.getcol(2);
  viewItm.setcol(3, worldViewPos);
  TMatrix viewTm = orthonormalized_inverse(viewItm);
  finalViewTm = viewTm * inverse(wtm);

  fovInTan = deg_to_fov(info.getReal("fov", 90.f)), fovHkInTan = fovInTan * startHkAspect;
  zn = max(dist - boxRad * 2.f - 1.f, 0.01f), zf = dist + boxRad * 2.f + 1.f;
  TMatrix4 projTm = matrix_perspective(fovInTan, fovHkInTan, zn, zf);
  TMatrix4_vec4 globTm = TMatrix4(finalViewTm) * projTm;

  vec4f clipScreenBox;

  bbox3f wbox = v_ldu_bbox3(localBox);
  if (v_screen_size_b(wbox.bmin, wbox.bmax, v_zero(), clipScreenBox, (const mat44f &)globTm))
  {
    alignas(16) Point4 clipBox;
    v_st(&clipBox.x, clipScreenBox);

    scaleX = 1. / (0.5 * (clipBox.y - clipBox.x)), scaleY = 1. / (0.5 * (clipBox.w - clipBox.z));
    centerX = 0.5 * (clipBox.x + clipBox.y);
    centerY = 0.5 * (clipBox.z + clipBox.w);
  }
  else
  {
    scaleX = scaleY = 1;
    centerX = centerY = 0;
  }
}

#define GLOBAL_VARS_ANIMCHAR_ICONS_LIST \
  VAR(wetness_params)                   \
  VAR(icon_envi_panorama)               \
  VAR(icon_envi_panorama_exposure)      \
  VAR(icon_ssaa)                        \
  VAR(icon_outline_color)               \
  VAR(icon_silhouette_color)            \
  VAR(icon_albedo_gbuf)                 \
  VAR(icon_albedo_gbuf_samplerstate)    \
  VAR(icon_normal_gbuf)                 \
  VAR(icon_normal_gbuf_samplerstate)    \
  VAR(icon_material_gbuf)               \
  VAR(icon_material_gbuf_samplerstate)  \
  VAR(icon_depth_gbuf)                  \
  VAR(icon_depth_gbuf_samplerstate)     \
  VAR(icon_light_dir)                   \
  VAR(icon_light_color)                 \
  VAR(view_scale_ofs)                   \
  VAR(projtm_psf_0)                     \
  VAR(projtm_psf_1)                     \
  VAR(projtm_psf_2)                     \
  VAR(projtm_psf_3)                     \
  VAR(render_to_icon)                   \
  VAR(background_alpha)                 \
  VAR(animchar_icons_final_samplerstate)

#define VAR(a) static int a##VarId = -1;
GLOBAL_VARS_ANIMCHAR_ICONS_LIST
#undef VAR

void RenderAnimCharIcon::initEnvi()
{
  if (enviInited)
    return;
  enviPanorama.close();
  const char *iconPanorama = dgs_get_game_params()->getStr("icon_render_panorama", nullptr); //"icon_render_panorama_tex_d"
  if (iconPanorama != nullptr && *iconPanorama != '\0')
    enviPanorama = dag::get_tex_gameres(iconPanorama);
  ShaderGlobal::set_real(icon_envi_panorama_exposureVarId, dgs_get_game_params()->getReal("icon_envi_panorama_exposure", 64.f));
  enviInited = true;
}

bool RenderAnimCharIcon::ensureDim(int w, int h)
{
  unsigned texFmt[3] = {TEXFMT_A8R8G8B8 | TEXCF_SRGBREAD | TEXCF_SRGBWRITE, TEXFMT_A2B10G10R10, TEXFMT_A8R8G8B8};
  uint32_t expectedRtNum = countof(texFmt);
  fullDeferred = true;
  mobileDeferred = false;

  const IRenderWorld *irw = get_world_renderer();
  if (irw == nullptr)
    return false;

  const WorldRenderer &wr = *(WorldRenderer *)irw;
  if (!wr.hasFeature(FeatureRenderFlags::FULL_DEFERRED))
  {
    fullDeferred = false;
    if (wr.hasFeature(FeatureRenderFlags::MOBILE_DEFERRED))
    {
      mobileDeferred = true;
      isForward = false;
      if (wr.hasFeature(FeatureRenderFlags::MOBILE_SIMPLIFIED_MATERIALS))
      {
        expectedRtNum = 2;
        texFmt[0] = TEXFMT_R8G8B8A8;
        texFmt[1] = TEXFMT_R8G8;
      }
      else
      {
        expectedRtNum = 3;
        texFmt[0] = TEXFMT_R8G8B8A8;
        texFmt[1] = TEXFMT_R8G8B8A8;
        texFmt[2] = TEXFMT_R8G8;
      }
    }
    else if (wr.hasFeature(FeatureRenderFlags::FORWARD_RENDERING))
    {
      isForward = true;
      expectedRtNum = 1;
      texFmt[0] = TEXFMT_R11G11B10F;
    }
    else
    {
      expectedRtNum = 2;
      texFmt[0] = TEXFMT_R11G11B10F;
      texFmt[1] = TEXFMT_R11G11B10F;
    }
  }
  if (target && target->getWidth() >= w && target->getHeight() >= h && expectedRtNum == target->getRtNum())
    return true;

  // Release previous target
  target.reset();

  target =
    eastl::make_unique<DeferredRenderTarget>("deferred_immediate_resolve", fullDeferred ? "animchar_icons" : "animchar_icons_thin_g",
      w, h, DeferredRT::StereoMode::MonoOrMultipass, 0, expectedRtNum, texFmt, TEXFMT_DEPTH32);
  d3d::SamplerHandle gbufferSampler = d3d::request_sampler({});

  finalTarget.close();
  if (!isForward)
  {
    finalTarget = dag::create_tex(NULL, w, h, TEXCF_RTARGET | TEXFMT_A16B16G16R16F, 1, "animchar_icons_final");
    finalTarget->disableSampler();
    finalAA.init("simple_gui_tonemap");
  }

#define VAR(a) a##VarId = get_shader_variable_id(#a, true);
  GLOBAL_VARS_ANIMCHAR_ICONS_LIST
#undef VAR

  ShaderGlobal::set_sampler(icon_albedo_gbuf_samplerstateVarId, gbufferSampler);
  ShaderGlobal::set_sampler(icon_normal_gbuf_samplerstateVarId, gbufferSampler);
  ShaderGlobal::set_sampler(icon_material_gbuf_samplerstateVarId, gbufferSampler);
  {
    d3d::SamplerInfo smpInfo;
    smpInfo.address_mode_u = smpInfo.address_mode_v = smpInfo.address_mode_w = d3d::AddressMode::Clamp;
    smpInfo.filter_mode = d3d::FilterMode::Point;
    d3d::SamplerHandle gbufferDepthSampler = d3d::request_sampler(smpInfo);
    ShaderGlobal::set_sampler(icon_depth_gbuf_samplerstateVarId, gbufferDepthSampler);
  }
  {
    d3d::SamplerInfo smpInfo;
    smpInfo.address_mode_u = smpInfo.address_mode_v = smpInfo.address_mode_w = d3d::AddressMode::Clamp;
    ShaderGlobal::set_sampler(animchar_icons_final_samplerstateVarId, d3d::request_sampler(smpInfo));
  }
  return true;
}

static void set_view_proj_shaders(TMatrix4 projTm, TMatrix4 globtm, const Point3 &world_view_pos)
{
  projTm = projTm.transpose();
  ShaderGlobal::set_color4(projtm_psf_0VarId, Color4(projTm[0]));
  ShaderGlobal::set_color4(projtm_psf_1VarId, Color4(projTm[1]));
  ShaderGlobal::set_color4(projtm_psf_2VarId, Color4(projTm[2]));
  ShaderGlobal::set_color4(projtm_psf_3VarId, Color4(projTm[3]));
  globtm = globtm.transpose();
  ShaderGlobal::set_color4(globtm_psf_0VarId, Color4(globtm[0]));
  ShaderGlobal::set_color4(globtm_psf_1VarId, Color4(globtm[1]));
  ShaderGlobal::set_color4(globtm_psf_2VarId, Color4(globtm[2]));
  ShaderGlobal::set_color4(globtm_psf_3VarId, Color4(globtm[3]));
  ShaderGlobal::set_color4(world_view_posVarId, world_view_pos.x, world_view_pos.y, world_view_pos.z, 1);
}

// we actually need hash_map to keep persistency of pointers!
static eastl::hash_map<eastl::string, GameResPtr> loadingResourcesForMap;
static volatile int loadingResourcesInFlight = 0;

struct LoadOneGameResForIconJob : public cpujobs::IJob
{
  const eastl::string *name;
  GameResPtr &ptr;
  LoadOneGameResForIconJob(const eastl::string &n, GameResPtr &p) : name(&n), ptr(p)
  {
    interlocked_increment(loadingResourcesInFlight);
  }
  virtual void doJob()
  {
    // debug("do job %s", name->c_str());
    GameResHandle handle = GAMERES_HANDLE_FROM_STRING(name->c_str());
    ptr.reset(::get_one_game_resource_ex(handle, CharacterGameResClassId));
    interlocked_decrement(loadingResourcesInFlight);
    // debug("job done %s", name->c_str());
    name = nullptr;
  }
  virtual void releaseJob()
  {
    if (name) // decrement only for removed jobs (when doJob() was not called)
      interlocked_decrement(loadingResourcesInFlight);
    delete this;
  }
  virtual unsigned getJobTag() { return _MAKE4C('loGR'); };
};

enum class AsynLoadAnimcharResult
{
  NOT_EXIST,
  LOADED,
  LOADING
};


static AsynLoadAnimcharResult sync_load_animchar(
  const char *name, GameResPtr &ret, GameResource *(*get_game_res_cb)(GameResHandle, unsigned) = &get_one_game_resource_ex)
{
  // debug("sync load %s", name);
  GameResHandle handle = GAMERES_HANDLE_FROM_STRING(name);
  ret.reset(get_game_res_cb(handle, CharacterGameResClassId));
  // debug("loaded %s %p", name, ret.get());
  return ret.get() ? AsynLoadAnimcharResult::LOADED : AsynLoadAnimcharResult::NOT_EXIST;
};

static AsynLoadAnimcharResult async_load_animchar(const char *name, GameResPtr &ret)
{
  if (!is_main_thread()) // we are not in main thread, load synced
  {
    // debug("not main thread syn %s", name);
    return sync_load_animchar(name, ret);
  }
  // check if resource exist at all
  // we are in main thread and resource is not loaded.

  // if (loadingResourcesForMap.find_as(name) != loadingResourcesForMap.end())//already schedule to load
  //   return AsynLoadAnimcharResult::LOADING;
  auto inserted = loadingResourcesForMap.emplace(name, GameResPtr());
  if (!inserted.second) // already schedule to load or loaded
  {
    // debug("already scheduled %s", name);
    if (inserted.first->second)
    {
      // debug("just loaded %s", name);
      ret = eastl::move(inserted.first->second);
      loadingResourcesForMap.erase(inserted.first); // remove from loading map
      return AsynLoadAnimcharResult::LOADED;
    }
    return AsynLoadAnimcharResult::LOADING;
  }
  GameResHandle handle = GAMERES_HANDLE_FROM_STRING(name);
  if (is_game_resource_loaded(handle, CharacterGameResClassId)) // already loaded
  {
    // debug("already loaded %s", name);
    loadingResourcesForMap.erase(inserted.first); // remove from loading map
    return sync_load_animchar(name, ret, &get_game_resource_ex);
  }
  // load with loading job manager. we can also launch independent thread, but why bother
  // debug("async load %s", name);
  cpujobs::add_job(ecs::get_common_loading_job_mgr(), new LoadOneGameResForIconJob(inserted.first->first, inserted.first->second));
  return AsynLoadAnimcharResult::LOADING;
}

static AsynLoadAnimcharResult sync_load_animchar_with_attachments(const DataBlock &blk, IconAnimcharWithAttachments &res)
{
  res.clear();
  const DataBlock *attachmentsBlk = blk.getBlockByNameEx("attachments");
  res.resize(attachmentsBlk->blockCount() + 1);
  GameResPtr gameRes;
  AsynLoadAnimcharResult ret = sync_load_animchar(blk.getStr("animchar"), gameRes);
  res.front().init((AnimV20::IAnimCharacter2 *)gameRes.get(), &blk);
  if (ret == AsynLoadAnimcharResult::NOT_EXIST)
    return ret;
  for (int i = 0; i < attachmentsBlk->blockCount(); ++i)
  {
    const DataBlock *aBlk = attachmentsBlk->getBlock(i);
    gameRes.reset();
    AsynLoadAnimcharResult attRet = sync_load_animchar(aBlk->getStr("animchar"), gameRes);
    res[i + 1].init((AnimV20::IAnimCharacter2 *)gameRes.get(), aBlk);
    if (attRet == AsynLoadAnimcharResult::NOT_EXIST)
      return attRet;
  }
  return ret;
}

static AsynLoadAnimcharResult async_load_animchar_with_attachments(const DataBlock &blk, IconAnimcharWithAttachments &res)
{
  res.clear();
  const DataBlock *attachmentsBlk = blk.getBlockByNameEx("attachments");
  res.resize(attachmentsBlk->blockCount() + 1);
  GameResPtr gameRes;
  AsynLoadAnimcharResult ret = async_load_animchar(blk.getStr("animchar"), gameRes);
  res.front().init((AnimV20::IAnimCharacter2 *)gameRes.get(), &blk);
  if (ret == AsynLoadAnimcharResult::NOT_EXIST)
    return ret;
  for (int i = 0; i < attachmentsBlk->blockCount(); ++i)
  {
    const DataBlock *aBlk = attachmentsBlk->getBlock(i);
    gameRes.reset();
    AsynLoadAnimcharResult attRet = async_load_animchar(aBlk->getStr("animchar"), gameRes);
    res[i + 1].init((AnimV20::IAnimCharacter2 *)gameRes.get(), aBlk);
    if (attRet == AsynLoadAnimcharResult::NOT_EXIST)
      return attRet;
    else if (attRet != ret)
      ret = AsynLoadAnimcharResult::LOADING;
  }
  return ret;
}

static void reset_async_load_animchar_queue()
{
  cpujobs::remove_jobs_by_tag(ecs::get_common_loading_job_mgr(), _MAKE4C('loGR'));
  if (interlocked_acquire_load(loadingResourcesInFlight))
  {
    debug("%s: waiting while loadingResourcesInFlight=%d -> 0", __FUNCTION__, interlocked_acquire_load(loadingResourcesInFlight));
    while (interlocked_acquire_load(loadingResourcesInFlight) != 0)
      sleep_msec(1);
  }

  if (loadingResourcesForMap.size())
    debug("%s: loadingResourcesForMap.clear (size=%d)", __FUNCTION__, loadingResourcesForMap.size());
  loadingResourcesForMap.clear();
}

bool RenderAnimCharIcon::match(const DataBlock &pic_props, int &out_w, int &out_h)
{
  if (!pic_props.getStr("animchar", NULL))
    return false;
  out_w = max((int)1, pic_props.getInt("w", 64));
  out_h = max((int)1, pic_props.getInt("h", 64));
  if (pic_props.getBool("autocrop", false))
  {
#if DAGOR_DBGLEVEL > 0
    // this is causing sync load!!! change API
    if (is_main_thread())
      logerr("autocrop match pic <%s> casing stall", pic_props.getStr("animchar"));
#endif
    IconAnimcharWithAttachments iconAnimchar;
    AsynLoadAnimcharResult ret = sync_load_animchar_with_attachments(pic_props, iconAnimchar);
    if (ret != AsynLoadAnimcharResult::LOADED)
      return true;
    for (auto &ia : iconAnimchar)
      if (!ia.animchar)
        return true;
    TMatrix finalViewTm;
    float fovInTan, fovHkInTan, zn, zf, scaleX, scaleY, centerX, centerY;
    get_animchar_tight_matrix(pic_props, iconAnimchar, float(out_w) / out_h, finalViewTm, fovInTan, fovHkInTan, zn, zf, scaleX, scaleY,
      centerX, centerY);
    if (scaleX > scaleY)
      out_w = max(int(ceilf(out_h) * scaleY / scaleX), (int)1);
    else
      out_h = max(int(ceilf(out_w) * scaleX / scaleY), (int)1);
  }
  return true;
}

bool RenderAnimCharIcon::renderInternal(Texture *to, int x, int y, int dstw, int dsth, const DataBlock &info, PICTUREID pid)
{
  IconAnimcharWithAttachments iconAnimchar;
  {
    WinAutoLock lock(pendReqCs);
    auto it = pendReq.find(pid);
    if (it != pendReq.end())
      iconAnimchar = it->second;
  }

  int w = dstw, h = dsth;
  const int xScale = clamp(info.getInt("ssaaX", (512 + w / 2) / w), (int)1, (int)4); // immediate mode aa.
  const int yScale = clamp(info.getInt("ssaaY", (512 + h / 2) / h), (int)1, (int)4); // immediate mode aa.
  const int ssaa = (xScale + yScale) / 2;
  // for bigger images just render image several times and accumulate, very easy to do
  // todo: support accumulate mode aa
  w *= ssaa;
  h *= ssaa;
  if (!ensureDim(w, h))
    return false;

  if (iconAnimchar.empty())
  {
    auto missingAnimChar = [&](const IconAnimchar &iconAnimchar) {
      logerr("missing animchar <%s>", iconAnimchar.blk->getStr("animchar"));
#if DAGOR_DBGLEVEL > 0
      clear_to(0xFF00FFFF, to, x, y, dstw, dsth);
#else
      clear_to(0, to, x, y, dstw, dsth);
#endif
      return true;
    };

    AsynLoadAnimcharResult ret = async_load_animchar_with_attachments(info, iconAnimchar);
    if (ret == AsynLoadAnimcharResult::LOADING)
      return false;
    if (ret == AsynLoadAnimcharResult::NOT_EXIST)
      return missingAnimChar(iconAnimchar[0]);
    for (auto &ia : iconAnimchar)
      if (!ia.animchar || !ia.animchar->getVisualResource())
        return missingAnimChar(ia);

    for (auto &ia : iconAnimchar)
    {
      ia.animchar = ia.animchar->clone(false);

      const DataBlock *objTexReplaceRulesBlk = ia.blk->getBlockByNameEx("objTexReplaceRules");
      const DataBlock *objTexSetRulesBlk = ia.blk->getBlockByNameEx("objTexSetRules");
      uint32_t rulesBlkCount = max(objTexSetRulesBlk->blockCount(), objTexReplaceRulesBlk->blockCount());
      for (int i = 0; i < rulesBlkCount; ++i)
      {
        eastl::vector<const char *, framemem_allocator> replStrings;
        eastl::vector<const char *, framemem_allocator> setStrings;

        if (i < objTexReplaceRulesBlk->blockCount())
        {
          const DataBlock *replaceRuleBlock = objTexReplaceRulesBlk->getBlock(i);
          for (int j = 0; j < replaceRuleBlock->paramCount(); ++j)
            replStrings.push_back(replaceRuleBlock->getStr(j));
        }
        if (i < objTexSetRulesBlk->blockCount())
        {
          const DataBlock *setRuleBlock = objTexSetRulesBlk->getBlock(i);
          for (int j = 0; j < setRuleBlock->paramCount(); ++j)
            setStrings.push_back(setRuleBlock->getStr(j));
        }

        if (!replStrings.empty() || !setStrings.empty())
        {
          G_ASSERT(replStrings.size() % 2 == 0);
          G_ASSERT(setStrings.size() % 3 == 0);

          AnimCharTexReplace texReplaceCtx(ia.animchar->rendComp(), ia.blk->getStr("animchar"));
          for (size_t i = 0; i < replStrings.size(); i += 2)
          {
            texReplaceCtx.replaceTex(replStrings[i], replStrings[i + 1]);
            eastl::string tmp(replStrings[i + 1]);
            if (tmp.back() == '*') // remove last star
              tmp.pop_back();
            ia.managedTextures.emplace_back(dag::get_tex_gameres(tmp.c_str()));
          }
          for (size_t i = 0; i < setStrings.size(); i += 3)
          {
            texReplaceCtx.replaceTexByShaderVar(setStrings[i], setStrings[i + 1], setStrings[i + 2]);
            eastl::string tmp(setStrings[i + 1]);
            if (tmp.back() == '*') // remove last star
              tmp.pop_back();
            ia.managedTextures.emplace_back(dag::get_tex_gameres(tmp.c_str()));
          }
          texReplaceCtx.apply(ia.animchar->baseComp(), /*create_instance*/ true);
        }
      }

      if (!ia.animchar->getSceneInstance())
        ia.animchar->rendComp().setVisualResource(ia.animchar->rendComp().getVisualResource(), true,
          ia.animchar->baseComp().getNodeTree());
      // this code should be removed after march 2022 major
      if (ia.blk->paramExists("blendfactor"))
      {
        float blendFactor = ia.blk->getReal("blendfactor");
        static int skinBlendVarId = get_shader_variable_id("skin_blend", true);
        recreate_material_with_new_params(ia.animchar->rendComp(), [&](ShaderMaterial *mat) {
          if (strcmp(mat->getShaderClassName(), "dynamic_blended_skin") == 0)
            mat->set_real_param(skinBlendVarId, blendFactor);
        });
      }
      if (ia.blk->paramExists("paintColor"))
      {
        Point4 paintColor = ia.blk->getPoint4("paintColor");
        set_paint_color(paintColor, ia.animchar->rendComp());
      }
      if (const DataBlock *camouflageOverrideBlk = ia.blk->getBlockByName("camouflage_override"))
        set_camouflage_override(*camouflageOverrideBlk, ia.animchar->rendComp());
      const DataBlock *shaderColors = ia.blk->getBlockByNameEx("shaderColors");
      if (!shaderColors->isEmpty())
      {
        eastl::vector<eastl::pair<int, Color4>, framemem_allocator> colors;
        for (int i = 0; i < shaderColors->paramCount(); ++i)
        {
          const char *shaderColorName = shaderColors->getParamName(i);
          const Point4 shaderColorValue = shaderColors->getPoint4(i);
          const int varId = get_shader_variable_id(shaderColorName, true);
          if (varId >= 0)
            colors.emplace_back(varId, Color4(&shaderColorValue.x));
        }
        if (!colors.empty())
        {
          recreate_material_with_new_params(ia.animchar->rendComp(), [&](ShaderMaterial *mat) {
            for (const eastl::pair<int, Color4> &color : colors)
              mat->set_color4_param(color.first, color.second);
          });
        }
      }

      ia.blk = nullptr;
    }

    // store animchar for later use and avoid data race
    IconAnimcharWithAttachments dup_animchar_to_destroy = iconAnimchar;
    {
      WinAutoLock lock(pendReqCs);
      auto it = pendReq.lower_bound(pid);
      if (it == pendReq.end() || it->first != pid)
      {
        pendReq.insert(it, eastl::make_pair(pid, iconAnimchar));
        dup_animchar_to_destroy.clear();
      }
      else
        iconAnimchar = it->second;
    }
    for (auto &ia : dup_animchar_to_destroy)
      ia.animchar->destroy();
  }

  G_ASSERT(!iconAnimchar.empty());

  // prefetch needed textures and prevent rendering until textures loaded
  {
    TextureIdSet usedTexIds;
    for (auto &ia : iconAnimchar)
    {
      ia.animchar->getVisualResource()->updateReqLod(0);
      ia.animchar->getVisualResource()->gatherUsedTex(usedTexIds);
    }
    if (!prefetch_and_check_managed_textures_loaded(usedTexIds))
      return false;
    for (auto &ia : iconAnimchar)
      if (ia.animchar->getVisualResource()->getQlBestLod() > 0)
        return false;
  }
  initEnvi();
  if (!isLoadedEnvi())
    return false;
  TIME_D3D_PROFILE(icon_render);

  // HATE this save-restore shit
  const Color4 oldWorldViewPos = ShaderGlobal::get_color4(world_view_posVarId);
  const int prevBlock = ShaderGlobal::getBlock(ShaderGlobal::LAYER_FRAME);
  ShaderElement::invalidate_cached_state_block();
  const Color4 oldWetnessParams = ShaderGlobal::get_color4(wetness_paramsVarId);
  ShaderGlobal::set_color4(wetness_paramsVarId, 0, -3000, 0, 0);
  SCOPED_SET_TEXTURE(icon_envi_panoramaVarId, enviPanorama);
  float exposure = const_frame_exposureVarId >= 0 ? ShaderGlobal::get_real(const_frame_exposureVarId) : 1;
  ShaderGlobal::set_real(const_frame_exposureVarId, 1.0f); // exposure is not needed
  ShaderGlobal::setBlock(globalConstBlockId, ShaderGlobal::LAYER_GLOBAL_CONST);

  STATE_GUARD_0(ShaderGlobal::set_int(disable_dynmat_paramsVarId, VALUE), 1);

  const float sun_zenith = 90 - info.getReal("zenith", mobileDeferred ? 45 : 65);
  const float sun_azimuth = info.getReal("azimuth", mobileDeferred ? -90 : -40);

  const Point3 lightDir(sinf(DegToRad(sun_zenith)) * cosf(DegToRad(sun_azimuth)),
    sinf(DegToRad(sun_zenith)) * sinf(DegToRad(sun_azimuth)), cosf(DegToRad(sun_zenith)));
  const Color3 lightCol = color3(info.getE3dcolor("sun", 0xFFFFFFFF)) * info.getReal("brightness", 4);

  Color4 prevSunDir = ShaderGlobal::get_color4(from_sun_directionVarId);
  Color4 prevSunColor = ShaderGlobal::get_color4(sun_color_0VarId);
  if (fullDeferred || mobileDeferred)
  {
    ShaderGlobal::set_color4(icon_light_dirVarId, -lightDir.x, -lightDir.z, -lightDir.y, 0);
    ShaderGlobal::set_color4(icon_light_colorVarId, lightCol.r, lightCol.g, lightCol.b, 0);
  }
  else // We apply shading while write to gbuffer, so need to override global sun settings
  {
    ShaderGlobal::set_color4(from_sun_directionVarId, -lightDir.x, -lightDir.z, -lightDir.y, 0);
    ShaderGlobal::set_color4(sun_color_0VarId, lightCol.r, lightCol.g, lightCol.b, 0);
  }

  bool ret = true;

  {
    SCOPE_RENDER_TARGET;
    SCOPE_VIEW_PROJ_MATRIX;
    const float scale = info.getReal("scale", 1.f);
    const Point2 ofs = info.getPoint2("offset", Point2(0, 0));

    TMatrix viewTm;
    float fovInTan, fovHkInTan, zn, zf, scaleX, scaleY, centerX, centerY;
    get_animchar_tight_matrix(info, iconAnimchar, float(w) / h, viewTm, fovInTan, fovHkInTan, zn, zf, scaleX, scaleY, centerX,
      centerY);
    scaleX *= scale;
    scaleY *= scale;
    scaleX = min(scaleX, scaleY);
    scaleY = scaleX; // because we can't change aspect ratio

    const char *animation = info.getStr("animation", "");
    const bool recalcAnimation = info.getBool("recalcAnimation", false);
    if (recalcAnimation)
      for (size_t i = 1; i < iconAnimchar.size(); ++i)
      {
        const int charUid = 1; // Some post blend controllers rely on this uid to run expensive logic once when attachment changes. By
                               // default it is 0, so we need any non 0 number here to run that logic.
        iconAnimchar[0].animchar->setAttachedChar(iconAnimchar[i].slotId, charUid, &iconAnimchar[i].animchar->baseComp(), false);
      }
    if (strcmp(animation, "") != 0)
    {
      int stateIdx = iconAnimchar[0].animchar->getAnimGraph()->getStateIdx(animation);
      iconAnimchar[0].animchar->getAnimGraph()->enqueueState(*iconAnimchar[0].animchar->getAnimState(),
        iconAnimchar[0].animchar->getAnimGraph()->getState(stateIdx));
      AnimV20::IAnimBlendNode *node = iconAnimchar[0].animchar->getAnimGraph()->getBlendNodePtr(animation);
      if (node)
        node->seek(*iconAnimchar[0].animchar->getAnimState(), 0);

      iconAnimchar[0].animchar->setTm(viewTm);
      iconAnimchar[0].animchar->doRecalcAnimAndWtm();
    }
    else
    {
      iconAnimchar[0].animchar->setTm(viewTm);
      if (recalcAnimation)
        iconAnimchar[0].animchar->doRecalcAnimAndWtm();
      else
        iconAnimchar[0].animchar->recalcWtm();
    }

    for (size_t i = 1; i < iconAnimchar.size(); ++i)
      iconAnimchar[i].updateTmWtm(iconAnimchar[0].animchar);
    for (auto &ia : iconAnimchar)
    {
      DynamicRenderableSceneInstance *scene = ia.animchar->getSceneInstance();
      scene->setCurrentLod(0);
      ia.animchar->rendComp().setNodes(scene, ia.animchar->rendComp().getNodeMap(), ia.animchar->getFinalWtm());

      iterate_names(scene->getLodsResource()->getNames().node, [&scene](int id, const char *name) {
        if (String(name).suffix("_dmg"))
          scene->showNode(id, false);
      });
    }

    for (int i = 0; i < iconAnimchar.size(); ++i)
    {
      const int hideNodesCount = iconAnimchar[i].hideNodeNames.size();
      if (hideNodesCount == 0)
        continue;
      DynamicRenderableSceneInstance *scene = iconAnimchar[i].animchar->getSceneInstance();
      const RoNameMapEx &names = scene->getLodsResource()->getNames().node;
      for (int j = 0; j < hideNodesCount; ++j)
      {
        const int nodeId = names.getNameId(iconAnimchar[i].hideNodeNames[j].c_str());
        if (nodeId >= 0)
        {
          scene->setNodeOpacity(nodeId, 0.f);
          scene->showNode(nodeId, false);
        }
      }
    }

    if (isForward)
      ShaderGlobal::set_sampler(get_shader_variable_id("static_shadow_tex_samplerstate", true), d3d::request_sampler({}));

    for (size_t i = 0; i < iconAnimchar.size();)
    {
      int nextI = i + 1;

      DynModelRenderingState &state = dynmodel_renderer::get_immediate_state();

      target->setRt();
      d3d::clearview(CLEAR_TARGET | CLEAR_ZBUFFER, 0, 0, 0);
      d3d::setview(0, 0, w, h, 0, 1);

      TMatrix4 projTm;
      d3d::settm(TM_VIEW, TMatrix::IDENT);
      d3d::setpersp(Driver3dPerspective(scaleX * fovInTan, scaleY * fovHkInTan, zn, zf, ofs.x * scale - centerX * scaleX,
                      ofs.y * scale - centerY * scaleY),
        &projTm);
      const TMatrix4 &globTm = projTm;

      extern void set_inv_globtm_to_shader(const TMatrix4 &viewTm, const TMatrix4 &projTm, bool optional);
      set_inv_globtm_to_shader(TMatrix4(TMatrix::IDENT), projTm, true);
      // AnimCharV20::prepareFrustum(false, Frustum((mat44f&)globtm));
      ShaderGlobal::set_int(dyn_model_render_passVarId, eastl::to_underlying(dynmodel::RenderPass::Color));
      // if (!animchar->beforeRender())
      //   logerr("not visible in icon! <%s>", info.getStr("animchar"));

      set_view_proj_shaders(projTm, globTm, Point3(0, 0, 0));
      ShaderGlobal::setBlock(globalFrameBlockId, ShaderGlobal::LAYER_FRAME);
      {
        SCENE_LAYER_GUARD(dynamicSceneBlockId);
        Point4 zero(0, 0, 0, 0);
        state.process_animchar(0, ShaderMesh::STG_imm_decal, iconAnimchar[i].animchar->getSceneInstance(), &zero, 1, false);
        for (; nextI < iconAnimchar.size(); ++nextI)
        {
          if (iconAnimchar[nextI].shading == IconAnimchar::SAME)
            state.process_animchar(0, ShaderMesh::STG_imm_decal, iconAnimchar[nextI].animchar->getSceneInstance(), &zero, 1, false);
          else
            break;
        }
        if (!state.empty())
        {
          state.prepareForRender();
          const DynamicBufferHolder *buffer = state.requestBuffer(BufferType::OTHER);
          if (buffer)
          {
            state.setVars(buffer->buffer.getBufId());
            SCENE_LAYER_GUARD(dynamicSceneBlockId);
            state.render(buffer->curOffset);
          }
          else
            ret = false;
        }
        // SCENE_LAYER_GUARD(dynamicSceneBlockId);
        // animchar->render();
      }
      // animchar->renderTrans();//would require resolve to additional linear target then postfx. Not hard to do, but shaders currently
      // use fog as well...

      if (!isForward)
      {
        d3d::set_render_target(finalTarget.getTex2D(), 0);
        d3d::setview(0, 0, w, h, 0, 1);
      }
      else
      {
        if (!to)
          d3d::set_render_target();
        else
          d3d::set_render_target(to, 0);
        d3d::setview(x, y, dstw, dsth, 0, 1);
        WRDispatcher::getShadowsManager().restoreShadowSampler();
      }

      if (i == 0)
        d3d::clearview(CLEAR_TARGET, info.getE3dcolor("backgroundColor", 0), 0, 0);
      ShaderGlobal::set_color4(view_scale_ofsVarId, w / float(target->getWidth()), h / float(target->getHeight()), 0, 0);

      ShaderGlobal::set_texture(icon_albedo_gbufVarId, target->getRtId(0));
      ShaderGlobal::set_texture(icon_normal_gbufVarId, target->getRtId(1));
      ShaderGlobal::set_texture(icon_material_gbufVarId, target->getRtId(2));
      ShaderGlobal::set_texture(icon_depth_gbufVarId, target->getDepthId());
      ShaderGlobal::set_color4(icon_outline_colorVarId,
        iconAnimchar[i].outline.u == 0 ? Color4(0, 0, 0, 2) : color4(iconAnimchar[i].outline));
      ShaderGlobal::set_color4(icon_silhouette_colorVarId,
        iconAnimchar[i].shading == IconAnimchar::SILHOUETTE ? color4(iconAnimchar[i].silhouette) : Color4(0, 0, 0, 2));
      d3d::resource_barrier({target->getRt(0), RB_RO_SRV | RB_STAGE_PIXEL, 0, 0});
      if (target->getRt(1))
        d3d::resource_barrier({target->getRt(1), RB_RO_SRV | RB_STAGE_PIXEL, 0, 0});
      if (target->getRt(2))
        d3d::resource_barrier({target->getRt(2), RB_RO_SRV | RB_STAGE_PIXEL, 0, 0});
      d3d::resource_barrier({target->getDepth(), RB_RO_SRV | RB_STAGE_PIXEL, 0, 0});
      if (target->getResolveShading())
        target->getResolveShading()->render();

      ShaderGlobal::set_int(render_to_iconVarId, 1);
      ShaderGlobal::set_real(background_alphaVarId, info.getReal("backgroundAlpha", 0.5f));
      {
        d3d::set_depth(target->getDepth(), DepthAccess::RW);
        d3d::setview(0, 0, w, h, 0, 1);
        state.clear();
        SCENE_LAYER_GUARD(dynamicSceneTransBlockId);
        Point4 zero(0, 0, 0, 0);
        uint32_t stage = ShaderMesh::STG_trans;
        state.process_animchar(stage, stage, iconAnimchar[i].animchar->getSceneInstance(), &zero, 1, false);
        for (int j = i + 1; j < nextI; j++)
          state.process_animchar(stage, stage, iconAnimchar[j].animchar->getSceneInstance(), &zero, 1, false);
        if (!state.empty())
        {
          state.prepareForRender();
          const DynamicBufferHolder *buffer = state.requestBuffer(BufferType::OTHER);
          if (buffer)
          {
            state.setVars(buffer->buffer.getBufId());
            state.render(buffer->curOffset);
          }
          else
            ret = false;
        }
      }
      ShaderGlobal::set_int(render_to_iconVarId, 0);
      i = nextI;
    }

    if (!isForward)
    {
      if (!to)
        d3d::set_render_target();
      else
        d3d::set_render_target(to, 0);
      d3d::setview(x, y, dstw, dsth, 0, 1);
      d3d::clearview(CLEAR_TARGET, 0, 0, 0);
      d3d::resource_barrier({finalTarget.getTex2D(), RB_RO_SRV | RB_STAGE_PIXEL, 0, 0});
      finalTarget.setVar();
      ShaderGlobal::set_color4(view_scale_ofsVarId, w / float(target->getWidth()), h / float(target->getHeight()), 0, 0);
      ShaderGlobal::set_color4(icon_ssaaVarId, ssaa, ssaa, -x, -y);
      finalAA.render();
      ShaderGlobal::setBlock(prevBlock, ShaderGlobal::LAYER_FRAME);
    }
    if (to)
      d3d::resource_barrier({to, RB_RO_SRV | RB_STAGE_PIXEL, 0, 0});

    {
      WinAutoLock lock(pendReqCs);
      auto it = pendReq.find(pid);
      if (it != pendReq.end())
        pendReq.erase(it);
      else
        iconAnimchar.clear();
    }

    for (auto &ia : iconAnimchar)
      ia.animchar->destroy();
    iconAnimchar.clear();
  }
  // restore all variables
  // todo: RAII!!
  if (!fullDeferred)
  {
    ShaderGlobal::set_color4(from_sun_directionVarId, prevSunDir);
    ShaderGlobal::set_color4(sun_color_0VarId, prevSunColor);
  }
  ShaderGlobal::set_color4(world_view_posVarId, oldWorldViewPos);
  ShaderGlobal::set_color4(wetness_paramsVarId, oldWetnessParams);
  ShaderGlobal::set_real(const_frame_exposureVarId, exposure);

  return ret;
}

bool RenderAnimCharIcon::render(Texture *to, int x, int y, int dstw, int dsth, const DataBlock &info, PICTUREID pid)
{
  if (!renderInternal(to, x, y, dstw, dsth, info, pid))
  {
    clear_to(0, to, x, y, dstw, dsth);
    return false;
  }
  return true;
}

#include <daECS/core/componentType.h>
ECS_DEF_PULL_VAR(renderAnimCharIcon);
