// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include "../av_plugin.h"
#include "../av_appwnd.h"
#include "../av_viewportWindow.h"
#include <assets/asset.h>
#include <de3_interface.h>
#include <de3_lodController.h>
#include <de3_animCtrl.h>
#include <de3_objEntity.h>
#include <de3_rendInstGen.h>
#include <de3_randomSeed.h>
#include <de3_entityGatherTex.h>
#include <rendInst/rendInstExtra.h>
#include <rendInst/rendInstExtraAccess.h>
#include <anim/dag_animBlend.h>
#include <anim/dag_animBlendCtrl.h>
#include <anim/dag_animPostBlendCtrl.h>
#include <anim/dag_animIrq.h>
#include <animChar/dag_animCharacter2.h>
#include <gameRes/dag_gameResources.h>
#include <shaders/dag_rendInstRes.h>
#include <shaders/dag_dynSceneRes.h>
#include <3d/dag_texPackMgr2.h>
#include <3d/dag_texIdSet.h>
#include <generic/dag_initOnDemand.h>
#include <osApiWrappers/dag_clipboard.h>
#include <util/dag_simpleString.h>
#include <util/dag_globDef.h>
#include <util/dag_texMetaData.h>
#include <util/dag_bitarray.h>
#include <math/random/dag_random.h>
#include <math/dag_geomTree.h>
#include <debug/dag_debug.h>
#include <debug/dag_debug3d.h>
#include <debug/dag_textMarks.h>

#include <propPanel/control/container.h>
#include <propPanel/c_control_event_handler.h>
#include <winGuiWrapper/wgw_input.h>
#include <libTools/util/conTextOutStream.h>
#include <libTools/util/strUtil.h>
#include <impostorUtil/impostorBaker.h>
#include <regExp/regExp.h>
#include <ecs/core/entityManager.h>

#include "../physObj/phys.h"        // for ragdoll
#include "../collision/collision.h" // for colliders

#include <math/dag_rayIntersectBox.h>                      // for position gizmo
#include <libTools/util/positionReconstructionFromDepth.h> // for position gizmo (surface "raycast")

#include "assetStatsFiller.h"
#include "compositeEditor.h"
#include "compositeEditorViewport.h"
#include "entityMatEditor.h"
#include "entityViewPluginInterface.h"
#include "objPropEditor.h"
#include "../av_cm.h"
#include "../assetStats.h"
#include <drv/3d/dag_matricesAndPerspective.h>
#include <EASTL/bonus/fixed_ring_buffer.h>

extern void export_texture_as_dds(const char *fn, DagorAsset &a);
extern void rimgr_set_force_lod_no(int lod_no);

static bool trace_ray_enabled = true;
static float coll_plane_ht_front = 0.0, coll_plane_ht_rear = 0.05;
static constexpr int SAVE_LAST_STATES_COUNT = 4;
static const int auto_inst_seed0 = mem_hash_fnv1(ZERO_PTR<const char>(), 12);
namespace texmgr_internal
{
extern TexQL max_allowed_q;
}

class PositionGizmoWrapper
{
  static constexpr float POSITION_GIZMO_SIZE = 0.5f;
  static constexpr float POSITION_GIZMO_RAYCAST_LINE_WIDTH = 0.02f;
  static constexpr float POSITION_GIZMO_RAYCAST_CENTER_WIDTH = 0.05f;

  // this variant is missing from dag_rayIntersectBox.h -> TODO: refactor it
  __forceinline static bool ray_intersect_box(const Point3 &p0, const Point3 &dir, const BBox3 &box, float &out_t)
  {
    return ray_intersect_wa_box0(p0 - box[0], dir, box[1] - box[0], out_t);
  }

  static bool get_position_from_depth(IGenViewportWnd *wnd, int x, int y, Point3 &result_pos)
  {
    int sx, sy;
    wnd->getViewportSize(sx, sy);

    // TODO: these are soon to be deprecated
    TMatrix viewRot;
    d3d::gettm(TM_VIEW, viewRot);
    TMatrix4 projTm;
    d3d::gettm(TM_PROJ, &projTm);

    // downsampled_far_depth_tex is already downsampled and ready to use
    return position_reconstruction_from_depth::get_position_from_depth(viewRot, projTm,
      get_managed_texture_id("downsampled_far_depth_tex"), Point2((float)x / sx, (float)y / sy), result_pos);
  }

public:
  PositionGizmoWrapper() { reset(); }

  void reset()
  {
    positionGizmoDragType = PositionGizmoDragType::PG_DRAG_TYPE_NONE;
    showGizmo = false;
    gizmoPos = Point3(0, 0, 0);
  }

  void render(float rotX, float rotY) const
  {
    if (showGizmo)
    {
      TMatrix tm;
      tm = rotyTM(rotY * DEG_TO_RAD) * rotxTM(rotX * DEG_TO_RAD);

      Point3 pointArr[] = {gizmoPos + Point3(POSITION_GIZMO_SIZE, 0, 0), gizmoPos + Point3(0, POSITION_GIZMO_SIZE, 0),
        gizmoPos + Point3(0, 0, POSITION_GIZMO_SIZE)};
      E3DCOLOR baseColorArr[] = {E3DCOLOR(255, 0, 0), E3DCOLOR(0, 255, 0), E3DCOLOR(0, 0, 255)};
      E3DCOLOR occludedColorArr[] = {E3DCOLOR(110, 0, 0), E3DCOLOR(0, 110, 0), E3DCOLOR(0, 0, 110)};

      begin_draw_cached_debug_lines(false, false);
      set_cached_debug_lines_wtm(tm);
      for (int i = 0; i < 3; i++)
        draw_cached_debug_line(gizmoPos, pointArr[i], occludedColorArr[i]);
      end_draw_cached_debug_lines();

      begin_draw_cached_debug_lines();
      set_cached_debug_lines_wtm(tm);
      for (int i = 0; i < 3; i++)
        draw_cached_debug_line(gizmoPos, pointArr[i], baseColorArr[i]);
      end_draw_cached_debug_lines();
    }
  }

  void onDragPress(float rotX, float rotY, IGenViewportWnd *wnd, int x, int y)
  {
    if (!showGizmo)
      return;
    Point3 dir, pos;
    wnd->clientToWorld(Point2(x, y), pos, dir);

    TMatrix tm;
    tm = rotxTM(-rotX * DEG_TO_RAD) * rotyTM(-rotY * DEG_TO_RAD);
    pos = tm * pos;
    dir = tm % dir;

    static constexpr float lw = POSITION_GIZMO_RAYCAST_LINE_WIDTH;
    static constexpr float s0 = POSITION_GIZMO_RAYCAST_CENTER_WIDTH;
    static constexpr float s1 = POSITION_GIZMO_SIZE;

    const BBox3 centerBox = BBox3(gizmoPos, POSITION_GIZMO_RAYCAST_CENTER_WIDTH * 2);
    const BBox3 xBox = BBox3(gizmoPos + Point3(s0, -lw, -lw), gizmoPos + Point3(s1, lw, lw));
    const BBox3 yBox = BBox3(gizmoPos + Point3(-lw, s0, -lw), gizmoPos + Point3(lw, s1, lw));
    const BBox3 zBox = BBox3(gizmoPos + Point3(-lw, -lw, s0), gizmoPos + Point3(lw, lw, s1));

    positionGizmoDragType = PositionGizmoDragType::PG_DRAG_TYPE_NONE;
    float dragTimeArr[4];
    for (int i = 0; i < 4; ++i)
      dragTimeArr[i] = 100; // max dist
    bool hit0 = ray_intersect_box(pos, dir, centerBox, dragTimeArr[0]);
    bool hit1 = ray_intersect_box(pos, dir, xBox, dragTimeArr[1]);
    bool hit2 = ray_intersect_box(pos, dir, yBox, dragTimeArr[2]);
    bool hit3 = ray_intersect_box(pos, dir, zBox, dragTimeArr[3]);
    if (hit0 || hit1 || hit2 || hit3)
    {
      int minId = eastl::min_element(&dragTimeArr[0], &dragTimeArr[4]) - &dragTimeArr[0];
      float minT = dragTimeArr[minId];
      positionGizmoDragType = static_cast<PositionGizmoDragType>(minId);
      dragRelativeDist = minT / pos.length();
      dragStartPos = pos + dir * minT;
      dragOffset = gizmoPos - dragStartPos;
    }
    else
    {
      Point3 surfacePos;
      if (get_position_from_depth(wnd, x, y, surfacePos))
      {
        positionGizmoDragType = PositionGizmoDragType::PG_DRAG_TYPE_SURFACE;
        gizmoPos = dragStartPos = tm * surfacePos;
        dragOffset = Point3(0, 0, 0);
      }
    }
  }

  void onDragRelease() { positionGizmoDragType = PositionGizmoDragType::PG_DRAG_TYPE_NONE; }

  bool onDragMove(float rotX, float rotY, IGenViewportWnd *wnd, int x, int y)
  {
    if (!showGizmo || positionGizmoDragType == PositionGizmoDragType::PG_DRAG_TYPE_NONE)
      return false;
    TMatrix tm;
    tm = rotxTM(-rotX * DEG_TO_RAD) * rotyTM(-rotY * DEG_TO_RAD);

    if (positionGizmoDragType == PositionGizmoDragType::PG_DRAG_TYPE_SURFACE)
    {
      Point3 surfacePos;
      if (get_position_from_depth(wnd, x, y, surfacePos))
      {
        gizmoPos = dragStartPos = tm * surfacePos;
        dragOffset = Point3(0, 0, 0);
        return true;
      }
    }

    Point3 dir, pos;
    wnd->clientToWorld(Point2(x, y), pos, dir);
    pos = tm * pos;
    dir = tm % dir;

    Point3 relPos = pos + dir * (pos.length() * dragRelativeDist) - dragStartPos;
    switch (positionGizmoDragType)
    {
      case PositionGizmoDragType::PG_DRAG_TYPE_AXIS_X: gizmoPos = dragStartPos + Point3(relPos.x, 0, 0); break;
      case PositionGizmoDragType::PG_DRAG_TYPE_AXIS_Y: gizmoPos = dragStartPos + Point3(0, relPos.y, 0); break;
      case PositionGizmoDragType::PG_DRAG_TYPE_AXIS_Z: gizmoPos = dragStartPos + Point3(0, 0, relPos.z); break;
      case PositionGizmoDragType::PG_DRAG_TYPE_CENTER: gizmoPos = dragStartPos + relPos;
      default: // justincase
        return false;
    }
    gizmoPos += dragOffset;
    return true;
  }

  const Point3 &getPosition() const { return gizmoPos; }
  void setPosition(const Point3 &pos) { gizmoPos = pos; }

  bool isVisible() const { return showGizmo; }
  void setVisible(bool visible) { showGizmo = visible; }

private:
  enum PositionGizmoDragType
  {
    // must be these exact values (for onDragPress)
    PG_DRAG_TYPE_NONE = -1,
    PG_DRAG_TYPE_CENTER = 0,
    PG_DRAG_TYPE_AXIS_X = 1,
    PG_DRAG_TYPE_AXIS_Y = 2,
    PG_DRAG_TYPE_AXIS_Z = 3,
    PG_DRAG_TYPE_SURFACE = 4,
  };

  PositionGizmoDragType positionGizmoDragType;
  Point3 gizmoPos;
  Point3 dragStartPos;
  Point3 dragOffset;
  float dragRelativeDist;
  bool showGizmo;
};

class EntityViewPlugin : public IGenEditorPlugin,
                         public PropPanel::ControlEventHandler,
                         public AnimV20::IGenericIrq,
                         public IConsoleCmd,
                         public IEntityViewPluginInterface
{
public:
  enum
  {
    PID_LODS_GROUP = 100,
    PID_FPSCAM_GROUP,
    PID_GENERATE_SEED,
    PID_SEED,
    PID_SHOW_OCCL_BOX,
    PID_ROTATE_X,
    PID_ROTATE_Y,
    PID_FORCE_SPD,
    PID_SHOW_FQ_TEX,
    PID_TEX_BASE,
    PID_TEX_GROUP = PID_TEX_BASE + 256,
    PID_NODES_BASE = PID_TEX_GROUP + 16,
    PID_NODES_GROUP = PID_NODES_BASE + 1024,
    PID_ANIM_SHOW_SKELETON,
    PID_ANIM_SHOW_SKELETON_NAMES,
    PID_ANIM_NODE_AXIS_LEN,
    PID_ANIM_SHOW_EFFECTORS,
    PID_ANIM_SHOW_IKSOL,
    PID_ANIM_SHOW_WABB,
    PID_ANIM_SHOW_BSPH,
    PID_ANIM_LOG_IRQ,
    PID_ANIM_ACT_RESTART,
    PID_ANIM_ACT_PAUSE,
    PID_ANIM_ACT_SLOWMO,
    PID_ANIM_RAGDOLL_GROUP,
    PID_ANIM_RAGDOLL_START,
    PID_ANIM_RAGDOLL_SPRING_FACTOR,
    PID_ANIM_RAGDOLL_DAMPER_FACTOR,
    PID_ANIM_RAGDOLL_BULLET_IMPULSE,
    PID_ANIM_CHARDEP_GROUP,
    PID_ANIM_CHARDEP_PY_OFS,
    PID_ANIM_CHARDEP_P_SCL,
    PID_ANIM_CHARDEP_S_SCL,
    PID_ANIM_DUMP_DEBUG,
    PID_ANIM_DUMP_UNUSED_BN,
    PID_ANIM_SET_NODE,
    PID_ANIM_STATES_GROUP,
    PID_ANIM_STATES_A_GROUP = PID_ANIM_STATES_GROUP + 256,
    PID_ANIM_STATE0,
    PID_ANIM_STATE_LAST = PID_ANIM_STATE0 + 1024,
    PID_ANIM_PARAMS_GROUP,
    PID_ANIM_PARAM0,
    PID_ANIM_PARAM_LAST = PID_ANIM_PARAM0 + 4096,
    PID_ANIM_TRACE_GROUP,
    PID_ANIM_TRACE_CP_HT_F,
    PID_ANIM_TRACE_CP_HT_R,
    PID_ANIM_TRACE_ENABLED,
    PID_PN_TRIANGULATION,
    PID_MASK_NODES_GROUP,
    PID_MASK_NODES_UPDATE_BTN,
    PID_MASK_NODES_UNCHECK_ALL,
    PID_MASK_NODES,
    PID_SHOW_POSITION_GIZMO,
    PID_POSITION_GIZMO,
    PID_GENERATE_PER_INSTANCE_SEED,
    PID_PER_INSTANCE_SEED,
    PID_SHOW_COLLIDER,
    PID_SHOW_COLLIDER_BBOX,
    PID_SHOW_COLLIDER_PHYS_COLLIDABLE,
    PID_SHOW_COLLIDER_TRACEABLE,
    PID_MATERIAL_EDITOR_GROUP,
    PID_OBJECT_GROUP,
    PID_COMMON_OBJECT_PROPERTIES_GROUP,
    PID_OBJECT_PROPERTIES_BUTTON,
    PID_ANIMATION_GROUP,
    PID_ANIM_TREE_IMGUI
  };

  enum
  {
    PID_LOD_AUTO_CHOOSE = 1,
    PID_LOD_FIRST,
    PID_FPSCAM_DEF = 1,
    PID_FPSCAM_FIRST,
  };
  struct RiEx
  {
    rendinst::riex_handle_t handle;
    int resIdx;
    RenderableInstanceLodsResource *res;
    SimpleString name;
    IObjEntity *vEntity;
    struct ImpostorData
    {
      bool enabled = false;
      // The textures are not used themselves, just the id
      TEXTUREID textureId_ar;
      TEXTUREID textureId_nt;
      TEXTUREID textureId_asad;
    } impostor;
  } riex;
  struct FpsCameraView
  {
    SimpleString name;
    SimpleString node, slot;
    SimpleString pitchParam, yawParam;
    Point3 ofs, fwdDir, upDir;
    bool fixedDir;
    float zn, zf;
    FastNameMap hideNodes;
  };

  void applyMaskToNodes(ILodController *lod_ctrl, RegExp &re, bool is_enabled, RegExp &re_exclude, bool use_exclude_reg_exp)
  {
    const int nodesCount = lod_ctrl->getNamedNodeCount();
    for (int i = 0; i < nodesCount; ++i)
    {
      const char *nodeName = lod_ctrl->getNamedNode(i);
      if (nodeName && re.test(nodeName))
      {
        if (use_exclude_reg_exp && re_exclude.test(nodeName))
          continue;
        lod_ctrl->setNamedNodeVisibility(i, is_enabled);
      }
    }
  }

  void applyMask(int index, ILodController *lod_ctrl, RegExp &re, PropPanel::ContainerPropertyControl *panel)
  {
    const DataBlock *maskBlk = nodeFilterMasksBlk.getBlock(index);
    const char *maskText = maskBlk->getStr("name", nullptr);
    const char *excludeText = maskBlk->getStr("exclude", nullptr);
    if (maskText && re.compile(maskText, ""))
    {
      RegExp reExclude;
      const bool useExclude = excludeText && reExclude.compile(excludeText, "");
      applyMaskToNodes(lod_ctrl, re, panel->getBool(PID_MASK_NODES + index), reExclude, useExclude);
    }
    else
      debug("bad regexp: '%s'", maskText);
  }

  void updateAllMaskFilters(PropPanel::ContainerPropertyControl *panel)
  {
    const int blockCount = nodeFilterMasksBlk.blockCount();
    RegExp re;
    ILodController *iLodCtrl = entity->queryInterface<ILodController>();
    if (iLodCtrl)
    {
      for (int i = 0; i < blockCount; ++i)
        applyMask(i, iLodCtrl, re, panel);
    }
  }

  EntityViewPlugin() : entity(NULL)
  {
    riex.resIdx = -1;
    riex.handle = rendinst::RIEX_HANDLE_NULL;
    riex.res = NULL;
    riex.vEntity = NULL;
    occluderBox.setempty();
    showOcclBox = true;
    showSkeleton = showEffectors = showIKSolution = false;
    showSkeletonNodeNames = false;
    showWABB = false;
    showBSPH = false;
    logIrqs = false;
    rotX = rotY = 0;
    lastAnimStateSet = -1;
    lastAnimStatesSet.clear();
    animPersCoursePid = animPersCourseDeltaPid = -1;
    lastForceAnimSet = 0;
    fpsCamViewId = PID_FPSCAM_DEF;
    fpsCamViewLastWtm.identity();
    pnTriangulation = true;
    updatePnTriangulation();

    ::get_app().getConsole().registerCommand("animchar.blender", this);
    ::get_app().getConsole().registerCommand("animchar.unusedBlendNodes", this);
    ::get_app().getConsole().registerCommand("animchar.nodemask", this);
    ::get_app().getConsole().registerCommand("animchar.vars", this);

    DataBlock appBlk(::get_app().getWorkspace().getAppPath());
    animCharVarSetts = *appBlk.getBlockByNameEx("animCharView")->getBlockByNameEx("vars");
    nodeFilterMasksBlk = *appBlk.getBlockByNameEx("nodeFilterMasks");

    for (int i = 0, nid = appBlk.getNameId("fps_view"); i < appBlk.getBlockByNameEx("animCharView")->blockCount(); i++)
    {
      const DataBlock &b = *appBlk.getBlockByNameEx("animCharView")->getBlock(i);
      if (b.getBlockNameId() != nid)
        continue;
      FpsCameraView &v = fpsCamView.push_back();
      v.name = b.getStr("name", NULL);
      v.node = b.getStr("node", NULL);
      v.slot = b.getStr("slot", NULL);
      v.pitchParam = b.getStr("pitch_param", NULL);
      v.yawParam = b.getStr("yaw_param", NULL);
      v.fixedDir = b.getBool("fixed_dir", false);
      v.ofs = b.getPoint3("camOfs", Point3(0, 0, 0));
      v.fwdDir = b.getPoint3("camAxis", Point3(0, 0, 1));
      v.upDir = b.getPoint3("camUpAxis", Point3(0, 1, 0));
      v.zn = b.getReal("znear", 0.1);
      v.zf = b.getReal("zfar", 1000);
      if (const DataBlock *b2 = b.getBlockByName("hideNodes"))
        for (int j = 0; j < b2->paramCount(); j++)
          if (b2->getParamType(j) == b2->TYPE_STRING)
            v.hideNodes.addNameId(b2->getStr(j));
      if (v.name.empty() || v.node.empty())
      {
        DAEDITOR3.conError("bad %s: name=%s node=%s", "fps_view", v.name, v.node);
        fpsCamView.pop_back();
      }
    }

    forceRiExtra =
      appBlk.getBlockByNameEx("assets")->getBlockByNameEx("build")->getBlockByNameEx("rendInst")->getBool("forceRiExtra", false);
    if (forceRiExtra)
      DAEDITOR3.conNote("rendInst uses forceRiExtra");
  }
  ~EntityViewPlugin()
  {
    destroy_it(entity);
    releaseRiExtra();
  }

  virtual const char *getInternalName() const { return "entityViewer"; }

  virtual void registered() { physsimulator::init(); }
  virtual void unregistered() { physsimulator::close(); }

  virtual void loadSettings(const DataBlock &settings) override { matEditor.loadSettings(settings); }

  virtual void saveSettings(DataBlock &settings) const override { matEditor.saveSettings(settings); }

  virtual bool begin(DagorAsset *asset)
  {
    static int rendInstEntityClassId = DAEDITOR3.getAssetTypeId("rendInst");

    if (texmgr_internal::max_allowed_q != TQL_stub)
      texmgr_internal::max_allowed_q = TQL_uhq;
    phys_bullet_delete_ragdoll(ragdoll);
    lastAnimStateSet = -1;
    lastAnimStatesSet.clear();
    animPersCoursePid = -1;
    animPersCourseDeltaPid = -1;
    lastForceAnimSet = 0;
    occluderBox.setempty();
    occluderQuad.clear();
    bool bad_ri = false;
    riex.impostor = {};
    assetName = asset ? asset->getName() : "";
    if (asset && asset->getType() == rendInstEntityClassId)
    {
      int nid_lod = asset->props.getNameId("lod");
      int nid_impostor = asset->props.getNameId("impostor");
      int lod_cnt = 0;
      if (nid_lod > 0)
      {
        for (int i = 0; i < asset->props.blockCount(); i++)
        {
          const int blockNameId = asset->props.getBlock(i)->getBlockNameId();
          if (blockNameId == nid_lod)
            lod_cnt++;
          else if (blockNameId == nid_impostor)
          {
            riex.impostor.enabled = true;
          }
        }
      }

      riex.resIdx = rendinst::addRIGenExtraResIdx(asset->getName(), -1, -1, rendinst::AddRIFlag::UseShadow);
      riex.res = rendinst::getRIGenExtraRes(riex.resIdx);
      bad_ri = !riex.res;
      if (!bad_ri && !riex.res->hasImpostor() &&
          (forceRiExtra || lod_cnt > 2 || riex.res->getOccluderBox() || riex.res->getOccluderQuadPts()))
      {
        mat33f m3;
        v_mat33_make_rot_cw_zyx(m3, v_make_vec4f(-rotX * DEG_TO_RAD, -rotY * DEG_TO_RAD, 0, 0));
        mat44f tm;
        v_mat44_ident(tm);
        tm.set33(m3);
        riex.handle = rendinst::addRIGenExtra44(riex.resIdx, false, tm, false, -1, -1, 1, &auto_inst_seed0);
        riex.name = asset->getName();
        if (riex.res->getOccluderBox())
        {
          occluderBox = *riex.res->getOccluderBox();
        }
        else if (riex.res->getOccluderQuadPts())
        {
          occluderQuad = make_span_const(riex.res->getOccluderQuadPts(), 4);
        }
        // create real entity (to query interfaces), but located at (1e6,0,1e6) and so invisible
        riex.vEntity = DAEDITOR3.createEntity(*asset);
        if (riex.vEntity)
        {
          TMatrix tm;
          tm.identity();
          tm.setcol(3, Point3(1e6, 0, 1e6));
          riex.vEntity->setTm(tm);
        }
      }
      else
      {
        riex.resIdx = -1;
        riex.res = NULL;
      }
    }
    entity = (asset && !riex.res && !bad_ri) ? DAEDITOR3.createEntity(*asset) : NULL;
    skeletonRef = asset ? asset->props.getStr("ref_skeleton", NULL) : NULL;

    matEditor.begin(asset, entity ? entity : riex.vEntity);
    objPropEditor.begin(asset, entity ? entity : riex.vEntity);
    get_app().getCompositeEditor().begin(asset, entity);

    /*
    if (!get_app().getScriptChangeFlag())
    {
      String fn(64, "%s.scheme.nut", asset->getTypeStr());
      initScriptPanelEditor(fn.str(), "entity by scheme");
      if (spEditor && asset)
        spEditor->load(asset);
    }
    //*/

    if (IRendInstGenService *rigenSrv = EDITORCORE->queryEditorInterface<IRendInstGenService>())
      rigenSrv->discardRIGenRect(0, 0, 64, 64);

    if (asset && riex.res && riex.impostor.enabled && riex.res->lods.size())
    {
      riex.impostor.textureId_ar = ::get_tex_gameres(String(0, "%s_as", asset->getName()), false);
      riex.impostor.textureId_nt = ::get_tex_gameres(String(0, "%s_nt", asset->getName()), false);
      riex.impostor.textureId_asad = ::get_tex_gameres(String(0, "%s_adas", asset->getName()), false);
    }

    if (asset && asset->getType() == rendInstEntityClassId)
    {
      const DagorAssetMgr &assetMgr = asset->getMgr();
      static int collisionAtype = assetMgr.getAssetTypeId("collision");
      eastl::string collisionAssetName(asset->getName());
      collisionAssetName.append("_collision");
      if (DagorAsset *collisionAsset = assetMgr.findAsset(collisionAssetName.c_str(), collisionAtype))
      {
        InitCollisionResource(*collisionAsset, &collisionResource, &collisionResourceNodeTree);
        DAEDITOR3.conNote("Found and inited collision '%s'", collisionAssetName.c_str());
      }
      else
        DAEDITOR3.conNote("Tried to init collision but no asset '%s' was found.", collisionAssetName.c_str());
    }

    if (entity)
    {
      entity->setSubtype(DAEDITOR3.registerEntitySubTypeId("single_ent"));
      updateRot();

      ILodController *iLodCtrl = entity->queryInterface<ILodController>();
      if (iLodCtrl)
        iLodCtrl->setCurLod(-1);

      if (IAnimCharController *animCtrl = entity->queryInterface<IAnimCharController>())
        if (animCtrl->getAnimGraph())
          if (const char *nm = asset->props.getStr("ref_anim", NULL))
            for (int i = 0, ie = animCtrl->getAnimNodeCount(); i < ie; i++)
              if (strcmp(animCtrl->getAnimNodeName(i), nm) == 0)
              {
                lastForceAnimSet = i + 1;
                break;
              }

      if (IRandomSeedHolder *irsh = entity->queryInterface<IRandomSeedHolder>())
        irsh->setPerInstanceSeed(auto_inst_seed0);

      fillPluginPanel();
    }
    else if (riex.res)
    {
      if (ILodController *iLodCtrl = riex.vEntity->queryInterface<ILodController>())
        iLodCtrl->setCurLod(-1);
      fillPluginPanel();
    }
    if (IAnimCharController *animCtrl = entity ? entity->queryInterface<IAnimCharController>() : NULL)
      if (AnimV20::AnimationGraph *ag = animCtrl->getAnimGraph())
      {
        int anim_nodes_cnt = 0;
        for (int i = 0; i < ag->getAnimNodeCount(); i++)
          if (ag->getBlendNodePtr(i) && !ag->getBlendNodePtr(i)->isSubOf(AnimV20::AnimBlendNodeNullCID))
            anim_nodes_cnt++;
        int param_cnt = 0;
        iterate_names(ag->getParamNames(), [&](int id, const char *) {
          if (ag->getParamType(id) != AnimV20::IPureAnimStateHolder::PT_Reserved)
            param_cnt++;
        });
        DAEDITOR3.conNote("animchar %s: %d animNodes (%d with NULLs),  %d BNLs,  %d PBCs\n"
                          "  %d params, stateSz=%d bytes\n"
                          "  %d states (%d channels)",
          asset->getName(), anim_nodes_cnt, ag->getAnimNodeCount(), ag->getBNLCount(), ag->getPBCCount(), param_cnt,
          animCtrl->getAnimState()->getSize(), animCtrl->getStatesCount(), ag->getStDest().size());
        if (auto *ac = animCtrl->getAnimChar())
        {
          for (int irq_id = 0; irq_id < 1024; irq_id++)
            if (AnimV20::getIrqName(irq_id))
              ac->registerIrqHandler(irq_id, this);
            else
              break;
        }
        if (animCtrl->getAnimChar()->getPhysicsResource())
          physsimulator::begin(NULL, physsimulator::PHYS_BULLET, 1, physsimulator::SCENE_TYPE_GROUP, 0.0f, coll_plane_ht_front,
            coll_plane_ht_rear);
      }

    return true;
  }
  virtual bool end()
  {
    // getCompositeEditor().end() must be called even if matEditor.end() returns with false, so this comes first.
    // Otherwise there could be an assert in the next getCompositeEditor().begin().
    if (!get_app().getCompositeEditor().end())
      return false;

    if (!matEditor.end())
      return false;

    fpsCamViewId = PID_FPSCAM_DEF;
    if (spEditor)
      spEditor->destroyPanel();
    if (IAnimCharController *animCtrl = entity ? entity->queryInterface<IAnimCharController>() : NULL)
      if (auto *ac = animCtrl->getAnimChar())
        ac->unregisterIrqHandler(-1, this);
    destroy_it(entity);
    phys_bullet_delete_ragdoll(ragdoll);
    releaseRiExtra();
    texNames.reset();
    lastForceAnimSet = 0;

    if (physsimulator::getPhysWorld())
    {
      physsimulator::disconnectSpring();
      physsimulator::end();
    }
    springConnected = false;
    positionGizmoWrapper.reset();

    clearAssetStats();
    ReleaseCollisionResource(&collisionResource, &collisionResourceNodeTree);

    if (currentAnimcharEid)
      g_entity_mgr->destroyEntity(currentAnimcharEid);
    if (texmgr_internal::max_allowed_q != TQL_stub)
      texmgr_internal::max_allowed_q = TQL_high;
    return true;
  }

  virtual void registerMenuAccelerators() override { compositeEditorViewport.registerMenuAccelerators(); }

  virtual void handleViewportAcceleratorCommand(IGenViewportWnd &wnd, unsigned id) override
  {
    compositeEditorViewport.handleViewportAcceleratorCommand(id, wnd, entity);
  }

  void changeLod(int lod)
  {
    IObjEntity *objEntity = entity ? entity : riex.vEntity;
    ILodController *lodCtrl = objEntity ? objEntity->queryInterface<ILodController>() : nullptr;
    if (lodCtrl)
    {
      PropPanel::ContainerPropertyControl *pluginPanel = getPluginPanel();

      if (lod >= 0)
      {
        const int lodCount = lodCtrl->getLodCount();
        if (lod >= lodCount)
          lod = lodCount - 1;

        if (pluginPanel)
          pluginPanel->setInt(PID_LODS_GROUP, PID_LOD_FIRST + lod);

        lodCtrl->setCurLod(lod);
      }
      else
      {
        if (pluginPanel)
        {
          if (lodCtrl->getLodCount() > 1)
            pluginPanel->setInt(PID_LODS_GROUP, PID_LOD_AUTO_CHOOSE);
          else
            pluginPanel->setInt(PID_LODS_GROUP, PID_LOD_FIRST);
        }

        lodCtrl->setCurLod(-1);
      }
    }
    else
    {
      rimgr_set_force_lod_no(lod);
    }
  }

  virtual void handleKeyPress(IGenViewportWnd *wnd, int vk, int modif) override
  {
    if (vk >= '0' && vk <= '9' && modif == 0)
      changeLod(vk - '0');
    else if (vk >= wingw::V_NUMPAD0 && vk <= wingw::V_NUMPAD9 && modif == 0)
      changeLod(vk - wingw::V_NUMPAD0);
    else if ((vk == wingw::V_BACK || vk == wingw::V_DECIMAL) && modif == 0)
      changeLod(-1);
  }

  virtual bool reloadOnAssetChanged(const DagorAsset *changed_asset)
  {
    if (IAnimCharController *animCtrl = entity ? entity->queryInterface<IAnimCharController>() : NULL)
      return animCtrl->hasReferencesTo(changed_asset);
    return false;
  }
  virtual bool reloadAsset(DagorAsset *asset)
  {
    if (!get_app().getCompositeEditor().expectingAssetReload())
      DAEDITOR3.conNote("reloading asset %s", asset->getName());
    DataBlock s;
    saveAnimState(s);
    end();
    if (asset->getType() == DAEDITOR3.getAssetTypeId("rendInst"))
      if (IRendInstGenService *rigenSrv = EDITORCORE->queryEditorInterface<IRendInstGenService>())
        rigenSrv->createRtRIGenData(-1024, -1024, 32, 8, 8, 8, {}, -1024, -1024);
    free_unused_game_resources();
    begin(asset);
    loadAnimState(s);
    return true;
  }

  virtual bool haveToolPanel() override { return true; }

  void saveAnimState(DataBlock &state)
  {
    if (!entity)
      return;
    IAnimCharController *animCtrl = entity->queryInterface<IAnimCharController>();
    if (!animCtrl || !animCtrl->getAnimGraph())
      return;
    AnimV20::IAnimStateHolder &st = *animCtrl->getAnimState();

    iterate_names(animCtrl->getAnimGraph()->getParamNames(), [&](int i, const char *nm) {
      switch (animCtrl->getAnimGraph()->getParamType(i))
      {
        case AnimV20::IPureAnimStateHolder::PT_ScalarParam:
        case AnimV20::IPureAnimStateHolder::PT_TimeParam: state.setReal(nm, st.getParam(i)); break;
        case AnimV20::IPureAnimStateHolder::PT_ScalarParamInt: state.setInt(nm, st.getParamInt(i)); break;
      }
    });

    int savedStatesCount = 0;
    for (const auto lastState : lastAnimStatesSet)
      for (int i = 0; i < animCtrl->getStatesCount(); i++)
        if (lastState == animCtrl->getStateIdx(animCtrl->getStateName(i)))
        {
          state.setStr(String(0, "__lastState_%d", savedStatesCount), animCtrl->getStateName(i));
          savedStatesCount += 1;
        }

    if (int anim_p1 = getPluginPanel()->getInt(PID_ANIM_SET_NODE))
      state.setStr("__lastForceAnimNode", animCtrl->getAnimNodeName(anim_p1 - 1));
  }
  void loadAnimState(const DataBlock &state)
  {
    if (!entity)
      return;
    IAnimCharController *animCtrl = entity->queryInterface<IAnimCharController>();
    if (!animCtrl || !animCtrl->getAnimGraph())
      return;
    AnimV20::IAnimStateHolder &st = *animCtrl->getAnimState();

    iterate_names(animCtrl->getAnimGraph()->getParamNames(), [&](int i, const char *nm) {
      switch (animCtrl->getAnimGraph()->getParamType(i))
      {
        case AnimV20::IPureAnimStateHolder::PT_ScalarParam:
        case AnimV20::IPureAnimStateHolder::PT_TimeParam: st.setParam(i, state.getReal(nm, st.getParam(i))); break;
        case AnimV20::IPureAnimStateHolder::PT_ScalarParamInt: st.setParamInt(i, state.getInt(nm, st.getParamInt(i))); break;
      }
    });

    for (int i = 0; i < SAVE_LAST_STATES_COUNT; i++)
      if (const char *nm = state.getStr(String(0, "__lastState_%d", i), NULL))
      {
        int stateIdx = animCtrl->getStateIdx(nm);
        animCtrl->enqueueState(stateIdx);
        lastAnimStatesSet.push_back(stateIdx);
      }

    if (const char *nm = state.getStr("__lastForceAnimNode", NULL))
    {
      int id = animCtrl->getAnimGraph()->getParamNames().getNameId(nm);
      if (id >= 0)
      {
        getPluginPanel()->setInt(PID_ANIM_SET_NODE, lastForceAnimSet = id + 1);
        animCtrl->enqueueAnimNode(nm);
      }
    }
  }

  void releaseRiExtra()
  {
    if (riex.res)
    {
      if (rendinst::getRIGenExtraResIdx(riex.name) >= 0)
        rendinst::delRIGenExtra(riex.handle);
      riex.handle = rendinst::RIEX_HANDLE_NULL;
      riex.resIdx = -1;
      riex.res = NULL;
      riex.name = NULL;
      destroy_it(riex.vEntity);
    }
  }

  virtual void clearObjects() {}
  virtual void onSaveLibrary() { matEditor.saveAllChanges(); }

  virtual void onLoadLibrary() {}

  virtual bool getSelectionBox(BBox3 &box) const
  {
    if (compositeEditorViewport.getSelectionBox(entity, box))
      return true;
    if (riex.res)
    {
      box = riex.res->bbox;
      return true;
    }
    if (!entity)
      return false;
    box = entity->getBbox();
    return true;
  }

  virtual void actObjects(float dt)
  {
    if (getPluginPanel())
      if (IAnimCharController *animCtrl = entity ? entity->queryInterface<IAnimCharController>() : NULL)
      {
        if (physsimulator::getPhysWorld())
          if (!animCtrl->isPaused())
            physsimulator::simulate(animCtrl->getTimeScale() * dt);

        AnimV20::AnimationGraph *anim = animCtrl->getAnimGraph();
        AnimV20::IAnimStateHolder *st = animCtrl->getAnimState();
        if (anim && st)
        {
          iterate_names(anim->getParamNames(), [&](int i, const char *paramName) {
            if (i < enumParamMask.size() && enumParamMask.get(i))
            {
              PropPanel::PropertyControlBase *c = getPluginPanel()->getById(PID_ANIM_PARAM0 + i);
              int v = 0;
              if (anim->getParamType(i) == AnimV20::IPureAnimStateHolder::PT_ScalarParam)
                v = (int)st->getParam(i);
              else if (anim->getParamType(i) == AnimV20::IPureAnimStateHolder::PT_ScalarParamInt)
                v = st->getParamInt(i);
              else
                DAEDITOR3.conError("param[%d] <%s> type=%d unexpected", i, paramName, anim->getParamType(i));
              if (const DataBlock *b = enumParamBlk.getBlockByName(paramName))
                for (int j = 0; j < b->blockCount(); j++)
                  if (b->getBlock(j)->getInt("_enumValue") == v)
                  {
                    if (c && c->getIntValue() != j)
                      c->setIntValue(j);
                    break;
                  }
            }
            else if (i < boolParamMask.size() && boolParamMask.get(i))
            {
              PropPanel::PropertyControlBase *c = getPluginPanel()->getById(PID_ANIM_PARAM0 + i);
              bool v = false;
              if (anim->getParamType(i) == AnimV20::IPureAnimStateHolder::PT_ScalarParam)
                v = st->getParam(i) > 0.5f;
              else if (anim->getParamType(i) == AnimV20::IPureAnimStateHolder::PT_ScalarParamInt)
                v = st->getParamInt(i) >= 1;
              else
                DAEDITOR3.conError("param[%d] <%s> type=%d unexpected", i, paramName, anim->getParamType(i));
              if (c && c->getBoolValue() != v)
                c->setBoolValue(v);
            }
            else if (anim->getParamType(i) == AnimV20::IPureAnimStateHolder::PT_ScalarParam)
            {
              PropPanel::PropertyControlBase *c = getPluginPanel()->getById(PID_ANIM_PARAM0 + i);
              float v = st->getParam(i);
              if (c && c->getFloatValue() != v)
                c->setFloatValue(v);
            }
            else if (anim->getParamType(i) == AnimV20::IPureAnimStateHolder::PT_ScalarParamInt)
            {
              PropPanel::PropertyControlBase *c = getPluginPanel()->getById(PID_ANIM_PARAM0 + i);
              int v = st->getParamInt(i);
              if (c && c->getIntValue() != v)
                c->setIntValue(v);
            }
          });

          if (animPersCourseDeltaPid > 0)
            updateRot();
        }
      }
  }
  virtual void beforeRenderObjects()
  {
    if (physsimulator::getPhysWorld())
      physsimulator::beforeRender();
    if (fpsCamViewId != PID_FPSCAM_DEF && CCameraElem::getCamera() == CCameraElem::MAX_CAMERA)
      if (IGenViewportWnd *vp = EDITORCORE->getViewport(0))
        if (IAnimCharController *animCtrl = entity ? entity->queryInterface<IAnimCharController>() : NULL)
        {
          FpsCameraView &fvc = fpsCamView[fpsCamViewId - PID_FPSCAM_FIRST];
          auto _ac = animCtrl->getAnimChar();
          auto ac = _ac ? &_ac->baseComp() : NULL;

          if (!fvc.slot.empty() && ac)
            ac = ac->getAttachedChar(AnimCharV20::getSlotId(fvc.slot));
          if (!ac)
            return;

          if (auto n = ac->findINodeIndex(fvc.node))
          {
            const mat44f &n_wtm = ac->getNodeTree().getNodeWtmRel(n);
            const Point3 &nx = as_point3(&n_wtm.col0);
            const Point3 &ny = as_point3(&n_wtm.col1);
            const Point3 &nz = as_point3(&n_wtm.col2);
            const Point3 &np = as_point3(&n_wtm.col3);
            if (fvc.fixedDir)
              vp->setCameraDirection(normalize(nx * fvc.fwdDir.x + ny * fvc.fwdDir.y + nz * fvc.fwdDir.z),
                normalize(nx * fvc.upDir.x + ny * fvc.upDir.y + nz * fvc.upDir.z));
            else if (!fvc.pitchParam.empty() || !fvc.yawParam.empty())
            {
              AnimV20::AnimationGraph *anim = animCtrl->getAnimGraph();
              AnimV20::IAnimStateHolder *st = animCtrl->getAnimState();
              if (anim && st)
              {
                float yaw = 0, pitch = 0;
                if (!fvc.pitchParam.empty())
                  pitch = -st->getParam(anim->getParamId(fvc.pitchParam)) * DEG_TO_RAD;
                if (!fvc.yawParam.empty())
                  yaw = -st->getParam(anim->getParamId(fvc.yawParam)) * DEG_TO_RAD;

                TMatrix tm;
                tm = rotyTM(yaw) * rotxTM(pitch);
                vp->setCameraDirection(tm * Point3(0, 0, -1), tm * Point3(0, 1, 0));
              }
            }
            vp->setCameraPos(np + nx * fvc.ofs.x + ny * fvc.ofs.y + nz * fvc.ofs.z);
          }
        }
  }

  void clearAssetStats()
  {
    AssetViewerViewportWindow *viewport = static_cast<AssetViewerViewportWindow *>(EDITORCORE->getCurrentViewport());
    if (viewport)
      viewport->getAssetStats().clear();
  }

  void fillAssetStats()
  {
    AssetViewerViewportWindow *viewport = static_cast<AssetViewerViewportWindow *>(EDITORCORE->getCurrentViewport());
    if (!viewport || !viewport->needShowAssetStats())
      return;

    AssetStats &stats = viewport->getAssetStats();
    stats.clear();
    stats.assetType = AssetStats::AssetType::Entity;

    TMatrix cameraTm;
    viewport->getCameraTransform(cameraTm);
    const Point3 cameraPos = cameraTm.getcol(3);

    AssetStatsFiller assetStatsFiller(stats);

    if (entity)
      assetStatsFiller.fillAssetStatsFromObjEntity(*entity, cameraPos);
    else if (riex.res)
      stats.currentLod = assetStatsFiller.fillAssetStatsFromRenderableInstanceLodsResource(*riex.res, cameraPos.length());

    if (collisionResource)
      assetStatsFiller.fillAssetCollisionStats(*collisionResource);

    assetStatsFiller.finalizeStats();
  }

  virtual void renderObjects()
  {
    IGenViewportWnd *wnd = EDITORCORE->getRenderViewport();
    if (wnd)
      compositeEditorViewport.renderObjects(*wnd, entity);

    fillAssetStats();
  }

  virtual void renderTransObjects()
  {
    if (showOcclBox && (!occluderBox.isempty() || occluderQuad.size()))
    {
      TMatrix tm;
      tm = rotyTM(rotY * DEG_TO_RAD) * rotxTM(rotX * DEG_TO_RAD);
      begin_draw_cached_debug_lines(false, false);
      set_cached_debug_lines_wtm(tm);
      if (occluderQuad.size())
      {
        draw_cached_debug_line(occluderQuad.data(), 4, 0x80808000);
        draw_cached_debug_line(occluderQuad[3], occluderQuad[0], 0x80808000);
      }
      else
        draw_cached_debug_box(occluderBox, 0x80808080);
      end_draw_cached_debug_lines();

      begin_draw_cached_debug_lines();
      set_cached_debug_lines_wtm(tm);
      if (occluderQuad.size())
      {
        draw_cached_debug_line(occluderQuad.data(), 4, 0xFFFFFF00);
        draw_cached_debug_line(occluderQuad[3], occluderQuad[0], 0xFFFFFF00);
      }
      else
        draw_cached_debug_box(occluderBox, 0xFFFFFFFF);
      end_draw_cached_debug_lines();
    }

    positionGizmoWrapper.render(rotX, rotY);

    if (IAnimCharController *animCtrl = entity ? entity->queryInterface<IAnimCharController>() : NULL)
    {
      AnimV20::AnimationGraph *anim = animCtrl->getAnimGraph();
      AnimV20::IAnimStateHolder *st = animCtrl->getAnimState();
      if (showSkeleton && animCtrl->getAnimChar())
      {
        animCtrl->getAnimChar()->getNodeTree().partialCalcWtm(dag::Index16(animCtrl->getAnimChar()->getNodeTree().nodeCount()));
        begin_draw_cached_debug_lines(false, false);
        set_cached_debug_lines_wtm(TMatrix::IDENT);
        DynamicRenderableSceneInstance *scn = animCtrl->getAnimChar()->getSceneInstance();
        const GeomNodeTree &tree = animCtrl->getAnimChar()->getNodeTree();
        if (!tree.empty())
          for (int i = 0; i < tree.getChildCount(dag::Index16(0)); i++)
            drawSkeletonLinks(tree, tree.getChildNodeIdx(dag::Index16(0), i), scn, nodeAxisLen, showSkeletonNodeNames);
        end_draw_cached_debug_lines();
      }
      if (showEffectors && anim)
      {
        begin_draw_cached_debug_lines(false, false);
        set_cached_debug_lines_wtm(TMatrix::IDENT);
        iterate_names(anim->getParamNames(), [&](int id, const char *nm) {
          if (anim->getParamType(id) == AnimV20::IPureAnimStateHolder::PT_Effector)
          {
            int id = anim->getParamId(nm, AnimV20::IAnimStateHolder::PT_Effector);
            int id2 = anim->getParamId(String(0, "%s.m", nm), AnimV20::IAnimStateHolder::PT_InlinePtr);
            AnimV20::IAnimStateHolder::EffectorVar eff = st->getParamEffector(id);
            if (eff.type == eff.T_useEffector)
              draw_cached_debug_sphere(Point3(eff.x, eff.y, eff.z), 0.005, E3DCOLOR(255, 0, 0, 255));
            else if (eff.nodeId)
              draw_cached_debug_sphere(animCtrl->getAnimChar()->getNodeTree().getNodeWposRelScalar(eff.nodeId), 0.005,
                E3DCOLOR(255, 128, 0, 255));

            if (id2 >= 0)
            {
              AnimV20::AttachGeomNodeCtrl::AttachDesc &ad = AnimV20::AttachGeomNodeCtrl::getAttachDesc(*st, id2);
              if (ad.w > 0)
              {
                draw_cached_debug_line(ad.wtm.getcol(3), ad.wtm.getcol(3) + ad.wtm.getcol(0) * 0.1, E3DCOLOR(255, 0, 0, ad.w * 255));
                draw_cached_debug_line(ad.wtm.getcol(3), ad.wtm.getcol(3) + ad.wtm.getcol(1) * 0.1, E3DCOLOR(0, 255, 0, ad.w * 255));
                draw_cached_debug_line(ad.wtm.getcol(3), ad.wtm.getcol(3) + ad.wtm.getcol(2) * 0.1, E3DCOLOR(0, 0, 255, ad.w * 255));
              }
            }
          }
        });

        TMatrix wtm;
        entity->getTm(wtm);
        draw_cached_debug_line(wtm.getcol(3), wtm.getcol(3) + (rotyTM(rotY * DEG_TO_RAD) % Point3(1, 0, 0)),
          E3DCOLOR(255, 128, 128, 255));
        if (IAnimCharController *animCtrl = entity->queryInterface<IAnimCharController>())
          if (animPersCourseDeltaPid > 0)
          {
            float fullRotY = rotY + animCtrl->getAnimState()->getParam(animPersCourseDeltaPid);
            draw_cached_debug_line(wtm.getcol(3), wtm.getcol(3) + (rotyTM(fullRotY * DEG_TO_RAD) % Point3(1, 0, 0)),
              E3DCOLOR(128, 128, 255, 255));
          }

        end_draw_cached_debug_lines();
      }
      if (showIKSolution && anim)
      {
        iterate_names(anim->getParamNames(), [&](int id, const char *nm) {
          if (anim->getParamType(id) == AnimV20::IPureAnimStateHolder::PT_InlinePtrCTZ)
          {
            if (!trail_strcmp(nm, "$dbg0"))
              return;
            int id0 = anim->getParamId(nm, AnimV20::IAnimStateHolder::PT_InlinePtrCTZ);
            int id1 = anim->getParamId(String(0, "%.*s1", strlen(nm) - 1, nm), AnimV20::IAnimStateHolder::PT_InlinePtrCTZ);
            auto *c0 = id0 >= 0 ? (AnimV20::MultiChainFABRIKCtrl::debug_state_t *)st->getInlinePtr(id0) : nullptr;
            auto *c1 = id1 >= 0 ? (AnimV20::MultiChainFABRIKCtrl::debug_state_t *)st->getInlinePtr(id1) : nullptr;

            if (c0 || c1)
            {
              begin_draw_cached_debug_lines(false, false);
              TMatrix wtm;
              entity->getTm(wtm);
              wtm.setcol(0, Point3(1, 0, 0));
              wtm.setcol(1, Point3(0, 1, 0));
              wtm.setcol(2, Point3(0, 0, 1));

              set_cached_debug_lines_wtm(wtm);
              if (c0)
                drawIKStructure(*c0, true);
              if (c1)
                drawIKStructure(*c1, false);
              set_cached_debug_lines_wtm(TMatrix::IDENT);
              end_draw_cached_debug_lines();
            }
          }
        });
      }
      if (showWABB)
      {
        bbox3f wbb;
        if (animCtrl->getAnimChar()->calcWorldBox(wbb))
        {
          Point3_vec4 b[2];
          v_st(&b[0].x, wbb.bmin);
          v_st(&b[1].x, wbb.bmax);
          begin_draw_cached_debug_lines(false, false);
          set_cached_debug_lines_wtm(TMatrix::IDENT);
          draw_cached_debug_box(BBox3(b[0], b[1]), 0xFF8080FF);
          end_draw_cached_debug_lines();
        }
      }
      if (showBSPH)
      {
        begin_draw_cached_debug_lines(false, false);
        set_cached_debug_lines_wtm(TMatrix::IDENT);
        draw_cached_debug_sphere(animCtrl->getAnimChar()->getBoundingSphere().c, animCtrl->getAnimChar()->getBoundingSphere().r,
          0xFFFF8080);
        end_draw_cached_debug_lines();
      }
    }
    if (physsimulator::getPhysWorld())
      physsimulator::renderTrans(true, false, false, false, false, false);

    if (showCollider && collisionResource)
      RenderCollisionResource(*collisionResource, collisionResourceNodeTree, showColliderBbox, showColliderPhysCollidable,
        showColliderTraceable);

    compositeEditorViewport.renderTransObjects(entity);
  }
  static void drawIKStructure(Tab<Tab<vec3f>> &chains, bool initial)
  {
    E3DCOLOR color(initial ? 128 : 255, initial ? 128 : 255, initial ? 128 : 255, 255);

    for (int i = 0; i < chains.size(); i++)
      for (int j = 0; j + 1 < chains[i].size(); j++)
        draw_cached_debug_line(as_point3(&chains[i][j]), as_point3(&chains[i][j + 1]), color);
    if (!initial)
      for (int i = 0; i < chains.size(); i++)
        if (chains[i].size())
          draw_cached_debug_sphere(as_point3(&chains[i].back()), 0.01, E3DCOLOR(255, 0, 0, 255));
  }
  static void drawSkeletonLinks(const GeomNodeTree &tree, dag::Index16 n, DynamicRenderableSceneInstance *scn, float axis_len,
    bool showNames)
  {
    const Point3 pos = tree.getNodeWposRelScalar(n);

    const char *name = tree.getNodeName(n);

    if (scn && scn->getNodeId(name) >= 0)
      draw_cached_debug_sphere(pos, 0.01, E3DCOLOR(0, 0, 255, 255));

    if (axis_len > 0.f)
    {
      mat44f nodeWtm = tree.getNodeWtmRel(n);
      draw_cached_debug_line(pos, pos + normalize(as_point3(&nodeWtm.col0)) * axis_len, E3DCOLOR_MAKE(255, 0, 0, 255));
      draw_cached_debug_line(pos, pos + normalize(as_point3(&nodeWtm.col1)) * axis_len, E3DCOLOR_MAKE(0, 255, 0, 255));
      draw_cached_debug_line(pos, pos + normalize(as_point3(&nodeWtm.col2)) * axis_len, E3DCOLOR_MAKE(0, 0, 255, 255));
    }

    if (showNames)
      add_debug_text_mark(tree.getNodeWposScalar(n), name);

    const size_t childCount = tree.getChildCount(n);
    for (int i = 0; i < childCount; i++)
    {
      auto cn = tree.getChildNodeIdx(n, i);

      draw_cached_debug_line(pos, tree.getNodeWposRelScalar(cn), E3DCOLOR(255, 255, 0, 255));
      drawSkeletonLinks(tree, cn, scn, axis_len, showNames);
    }
  }

  virtual bool supportAssetType(const DagorAsset &asset) const
  {
    return strcmp(asset.getTypeStr(), "composit") == 0 || strcmp(asset.getTypeStr(), "rendInst") == 0 ||
           (asset.getFileNameId() < 0 && strcmp(asset.getTypeStr(), "fx") == 0) || strcmp(asset.getTypeStr(), "efx") == 0 ||
           strcmp(asset.getTypeStr(), "animChar") == 0 || strcmp(asset.getTypeStr(), "gameObj") == 0 ||
           strcmp(asset.getTypeStr(), "dynModel") == 0;
  }
  virtual bool supportEditing() const { return false; }

  virtual void fillPropPanel(PropPanel::ContainerPropertyControl &panel)
  {
    panel.setEventHandler(this);
    IObjEntity *_ent = entity ? entity : riex.vEntity;

    PropPanel::ContainerPropertyControl *grpMaterialList = panel.createGroupHorzFlow(PID_MATERIAL_EDITOR_GROUP, "Material editor");
    if (grpMaterialList)
    {
      matEditor.fillPropPanel(*grpMaterialList);
      grpMaterialList->setBoolValue(true);
    }

    PropPanel::ContainerPropertyControl *grpSpec = panel.createGroupHorzFlow(PID_OBJECT_GROUP, "Object group");
    grpSpec->setEventHandler(this);

    ILodController *iLodCtrl = _ent ? _ent->queryInterface<ILodController>() : nullptr;
    {
      PropPanel::ContainerPropertyControl *commonPanel = grpSpec->createGroup(PID_COMMON_OBJECT_PROPERTIES_GROUP, "Common");
      objPropEditor.fillPropPanel(PID_OBJECT_PROPERTIES_BUTTON, *commonPanel);

      commonPanel->createCheckBox(PID_SHOW_POSITION_GIZMO, String("Show position gizmo"), positionGizmoWrapper.isVisible());
      commonPanel->createPoint3(PID_POSITION_GIZMO, "", positionGizmoWrapper.getPosition(), 3, positionGizmoWrapper.isVisible());
      commonPanel->createSeparator();

      commonPanel->createTrackFloat(PID_ROTATE_X, "Rotate around X, deg", rotX, -180, 180, 1);
      commonPanel->createTrackFloat(PID_ROTATE_Y, "Rotate around Y, deg", rotY, -180, 180, 1);
      if (entity && entity->queryInterface<IAnimCharController>())
        commonPanel->createTrackFloat(PID_FORCE_SPD, "Force speed, m/s", forceSpd, -1, 20, 0.1);
      commonPanel->createCheckBox(PID_PN_TRIANGULATION, String(0, "PN-triangulation"), pnTriangulation);
      commonPanel->createIndent();

      if (!_ent)
        return;

      if (!skeletonRef.empty())
      {
        commonPanel->createStatic(0, "Skeleton ref");
        commonPanel->createStatic(0, skeletonRef);
        commonPanel->createIndent();
      }

      if (iLodCtrl && iLodCtrl->getLodCount())
      {
        const int lodsCount = iLodCtrl->getLodCount();
        const bool moreThanOne = lodsCount > 1;
        PropPanel::ContainerPropertyControl *rg = commonPanel->createRadioGroup(PID_LODS_GROUP, "Presentation lods:");

        if (moreThanOne)
          rg->createRadio(PID_LOD_AUTO_CHOOSE, "auto choose");

        String buf;
        for (int i = 0; i < lodsCount; i++)
        {
          buf.printf(64, "lod_%d [%.2f m]", i, iLodCtrl->getLodRange(i));
          rg->createRadio(PID_LOD_FIRST + i, buf);
        }

        commonPanel->setInt(PID_LODS_GROUP, (moreThanOne ? PID_LOD_AUTO_CHOOSE : PID_LOD_FIRST));
        if (iLodCtrl->getTexQLCount() == 2)
        {
          commonPanel->createIndent();
          commonPanel->createCheckBox(PID_SHOW_FQ_TEX, "Use full quality tex", iLodCtrl->getTexQL());
        }
      }

      if (riex.res && (riex.res->getOccluderBox() || riex.res->getOccluderQuadPts()))
        commonPanel->createCheckBox(PID_SHOW_OCCL_BOX, String(0, "Show occluder %s", occluderQuad.size() ? "quad" : "box"),
          showOcclBox);

      if (entity)
        if (IRandomSeedHolder *irsh = _ent->queryInterface<IRandomSeedHolder>())
        {
          commonPanel->createButton(PID_GENERATE_SEED, "Generate random seed");
          commonPanel->createTrackInt(PID_SEED, "Random seed", irsh->getSeed(), 0, 32767, 1);
          commonPanel->createIndent();
        }

      if (IRandomSeedHolder *irsh = _ent->queryInterface<IRandomSeedHolder>())
      {
        commonPanel->createButton(PID_GENERATE_PER_INSTANCE_SEED, "Generate per-instance seed");
        commonPanel->createTrackInt(PID_PER_INSTANCE_SEED, "Per-instance seed", irsh->getPerInstanceSeed(), 0, 32767, 1);
        commonPanel->createIndent();
      }

      if (collisionResource)
      {
        commonPanel->createSeparator();
        commonPanel->createCheckBox(PID_SHOW_COLLIDER, String("Show collider"), showCollider);
        commonPanel->createCheckBox(PID_SHOW_COLLIDER_BBOX, String("Show bounding box"), showColliderBbox, showCollider);
        commonPanel->createCheckBox(PID_SHOW_COLLIDER_PHYS_COLLIDABLE, String("Show phys collidable"), showColliderPhysCollidable,
          showCollider);
        commonPanel->createCheckBox(PID_SHOW_COLLIDER_TRACEABLE, String("Show traceable"), showColliderTraceable, showCollider);
      }
    }
    if (IAnimCharController *animCtrl = _ent->queryInterface<IAnimCharController>())
    {
      PropPanel::ContainerPropertyControl *animPanel = grpSpec->createGroup(PID_ANIMATION_GROUP, "Animation");
      if (fpsCamView.size() && animCtrl->getAnimChar())
      {
        Tab<int> cam_ids;
        for (int i = 0, ie = fpsCamView.size(); i < ie; ++i)
          if (!fpsCamView[i].slot.empty() && animCtrl->getAnimChar()->getSlotNodeWtm(AnimCharV20::getSlotId(fpsCamView[i].slot)))
            cam_ids.push_back(i);
          else if (fpsCamView[i].slot.empty() && animCtrl->getAnimChar()->findINodeIndex(fpsCamView[i].node))
            cam_ids.push_back(i);
          else if (fpsCamViewId == PID_FPSCAM_FIRST + i)
            fpsCamViewId = PID_FPSCAM_DEF;

        if (cam_ids.size() > 0)
        {
          animPanel->createSeparator();
          PropPanel::ContainerPropertyControl *rg = animPanel->createRadioGroup(PID_FPSCAM_GROUP, "Apply FPS camera:");
          rg->createRadio(PID_FPSCAM_DEF, "-- no camera --");
          for (int i = 0; i < cam_ids.size(); ++i)
            rg->createRadio(PID_FPSCAM_FIRST + cam_ids[i], fpsCamView[cam_ids[i]].name);
          animPanel->setInt(PID_FPSCAM_GROUP, fpsCamViewId);
        }
      }
      else
        fpsCamViewId = PID_FPSCAM_DEF;

      animPanel->createSeparator();
      if (animCtrl->getAnimChar())
      {
        const GeomNodeTree &t = animCtrl->getAnimChar()->getNodeTree();
        animPanel->createCheckBox(PID_ANIM_SHOW_SKELETON,
          String(0, "Show skeleton (%d total, %d important)", t.nodeCount(), t.importantNodeCount()), showSkeleton);
        animPanel->createCheckBox(PID_ANIM_SHOW_SKELETON_NAMES, "Show skeleton node names", showSkeletonNodeNames);
        animPanel->createEditFloat(PID_ANIM_NODE_AXIS_LEN, String(0, "Axis len", nodeAxisLen));
      }
      animPanel->createCheckBox(PID_ANIM_SHOW_EFFECTORS, "Show effectors", showEffectors);
      animPanel->createCheckBox(PID_ANIM_SHOW_IKSOL, "Show FABRIK solution", showIKSolution);
      animPanel->createCheckBox(PID_ANIM_SHOW_WABB, "Show WABB", showWABB);
      animPanel->createCheckBox(PID_ANIM_SHOW_BSPH, "Show BSphere", showBSPH);
      animPanel->createCheckBox(PID_ANIM_LOG_IRQ, "Log IRQs to console", logIrqs);
      animPanel->createButton(PID_ANIM_ACT_RESTART, "restart", true);
      animPanel->createButton(PID_ANIM_ACT_PAUSE, animCtrl->isPaused() ? "resume" : "pause", true, false);
      animPanel->createButton(PID_ANIM_ACT_SLOWMO, animCtrl->getTimeScale() != 1.0f ? "normal" : "slow-mo", true, false);

      animPanel->createButton(PID_ANIM_DUMP_DEBUG, "Debug anim state");
      animPanel->createButton(PID_ANIM_DUMP_UNUSED_BN, "Dump unused blend nodes");

      if (g_entity_mgr && g_entity_mgr->getTemplateDB().getTemplateByName("animchar_base"))
      {
        animPanel->createButton(PID_ANIM_TREE_IMGUI, "Anim tree imgui");
      }

      if (animCtrl->getAnimChar() && animCtrl->getAnimChar()->getPhysicsResource())
      {
        PropPanel::ContainerPropertyControl &rdGrp = *animPanel->createGroup(PID_ANIM_RAGDOLL_GROUP, "Ragdoll test");
        rdGrp.createButton(PID_ANIM_RAGDOLL_START, ragdoll ? "stop" : "START");
        rdGrp.createStatic(0, "Spring settings (activated with LMB)");
        rdGrp.createEditFloat(PID_ANIM_RAGDOLL_SPRING_FACTOR, "Spring factor, N/m", physsimulator::springFactor);
        rdGrp.createEditFloat(PID_ANIM_RAGDOLL_DAMPER_FACTOR, "Damper factor", physsimulator::damperFactor);
        rdGrp.createStatic(0, "Impulse settings (activated with RMB)");
        rdGrp.createEditFloat(PID_ANIM_RAGDOLL_BULLET_IMPULSE, "Bullet impulse, kg*m/s", ragdollBulletImpulse);
        animPanel->setBool(PID_ANIM_RAGDOLL_GROUP, true);
      }
      if (animCtrl->getAnimChar())
      {
        float pyOfs, pxScale, pyScale, pzScale, sScale;
        if (animCtrl->getAnimChar()->getCharDepScales(pyOfs, pxScale, pyScale, pzScale, sScale))
        {
          PropPanel::ContainerPropertyControl &cdcGrp =
            *animPanel->createGroup(PID_ANIM_CHARDEP_GROUP, "Char-dependant customization");
          cdcGrp.createEditFloat(PID_ANIM_CHARDEP_S_SCL, "SCL scale", sScale);
          cdcGrp.createPoint3(PID_ANIM_CHARDEP_P_SCL, "POS scale", Point3(pxScale, pyScale, pzScale));
          cdcGrp.createEditFloat(PID_ANIM_CHARDEP_PY_OFS, "POS Y ofs", pyOfs);
          animPanel->setBool(PID_ANIM_CHARDEP_GROUP, true);
          cdcGrp.setMinMaxStep(PID_ANIM_CHARDEP_PY_OFS, -2, 2, 0.01);
          cdcGrp.setMinMaxStep(PID_ANIM_CHARDEP_P_SCL, 0.8, 1.2, 0.01);
          cdcGrp.setMinMaxStep(PID_ANIM_CHARDEP_S_SCL, 0.8, 1.2, 0.01);
        }
      }

      Tab<String> anims(tmpmem);
      anims.push_back() = "-- none --";
      for (int i = 0, ie = animCtrl->getAnimNodeCount(); i < ie; ++i)
      {
        anims.push_back() = animCtrl->getAnimNodeName(i);
        if (anims.back().empty())
          anims.back() = "##";
      }
      animPanel->createSortedCombo(PID_ANIM_SET_NODE, "Force anim node", anims, lastForceAnimSet);

      AnimV20::AnimationGraph *anim = animCtrl->getAnimGraph();
      AnimV20::IAnimStateHolder *st = animCtrl->getAnimState();
      if (anim && anim->getStateCount())
      {
        PropPanel::ContainerPropertyControl &animGrp = *animPanel->createGroup(PID_ANIM_STATES_GROUP, "Anim states");
        const int stateCount = anim->getStateCount();

        // fill tagged real states to separate groups
        for (int t = 0; t < anim->getTagsCount(); t++)
        {
          PropPanel::ContainerPropertyControl &animGrpT =
            (t == 0) ? animGrp : *animGrp.createGroup(PID_ANIM_STATES_GROUP + t, String(0, "\"%s\" states", anim->getTagName(t)));
          for (int i = 0; i < stateCount; ++i)
            if (!anim->isStateNameAlias(i) && anim->getStateNameTag(i) == t)
              animGrpT.createButtonLText(PID_ANIM_STATE0 + anim->getStateIdx(anim->getStateName(i)), anim->getStateName(i));
        }

        // fill states aliases to separate group
        {
          PropPanel::ContainerPropertyControl *animGrpA = nullptr;
          for (int i = 0; i < stateCount; i++)
          {
            if (!anim->isStateNameAlias(i))
              continue;

            if (!animGrpA)
              animGrpA = animGrp.createGroup(PID_ANIM_STATES_A_GROUP, "State aliases");

            animGrpA->createButton(PID_ANIM_STATE0 + anim->getStateIdx(anim->getStateName(i)), anim->getStateName(i));
          }

          if (animGrpA)
            animPanel->setBool(PID_ANIM_STATES_A_GROUP, true);
        }
        animPanel->setBool(PID_ANIM_STATES_GROUP, true);
      }

      if (anim && st)
      {
        animPersCoursePid = anim->getParamId("pers_course", AnimV20::IAnimStateHolder::PT_ScalarParam);
        animPersCourseDeltaPid = anim->getParamId("pers_course:delta", AnimV20::IAnimStateHolder::PT_ScalarParam);
        bool has_params = false;
        for (int i = 0; i < anim->getParamCount(); i++)
          if (anim->getParamType(i) == AnimV20::IPureAnimStateHolder::PT_ScalarParam ||
              anim->getParamType(i) == AnimV20::IPureAnimStateHolder::PT_ScalarParamInt)
          {
            has_params = true;
            break;
          }

        enumParamBlk.clearData();
        enumParamMask.clear();
        boolParamMask.clear();
        if (has_params)
        {
          PropPanel::ContainerPropertyControl &astGrp = *animPanel->createGroup(PID_ANIM_PARAMS_GROUP, "Anim params");
          enumParamMask.resize(anim->getParamCount());
          boolParamMask.resize(anim->getParamCount());
          iterate_names_in_lexical_order(anim->getParamNames(), [&](int i, const char *nm) {
            if (nm[0] == ':')
              return;

            const DataBlock *param_b = animCharVarSetts.getBlockByName(nm);
            if (const char *enum_cls = param_b ? param_b->getStr("enum", NULL) : NULL)
            {
              const DataBlock &bEnum = anim->getEnumList(enum_cls);
              if (!bEnum.blockCount())
                DAEDITOR3.conError("empty enum <%s>", enum_cls);

              enumParamBlk.addNewBlock(&bEnum, nm);
              Tab<String> enumNm(tmpmem);
              int value = 0;
              if (anim->getParamType(i) == AnimV20::IPureAnimStateHolder::PT_ScalarParam)
                value = (int)st->getParam(i);
              else if (anim->getParamType(i) == AnimV20::IPureAnimStateHolder::PT_ScalarParamInt)
                value = st->getParamInt(i);

              int sel_idx = -1;
              for (int pi = 0; pi < bEnum.blockCount(); pi++)
              {
                const DataBlock *paramBlk = bEnum.getBlock(pi);
                enumNm.push_back() = paramBlk->getBlockName();
                if (paramBlk->getInt("_enumValue") == value)
                  sel_idx = pi;
              }
              astGrp.createSortedCombo(PID_ANIM_PARAM0 + i, nm, enumNm, sel_idx);
              enumParamMask.set(i);
              return;
            }
            if (param_b && param_b->getBool("bool", false))
            {
              bool v = false;
              if (anim->getParamType(i) == AnimV20::IPureAnimStateHolder::PT_ScalarParam)
                v = st->getParam(i) > 0.5f;
              else if (anim->getParamType(i) == AnimV20::IPureAnimStateHolder::PT_ScalarParamInt)
                v = st->getParamInt(i) >= 1;

              astGrp.createCheckBox(PID_ANIM_PARAM0 + i, nm, v);
              boolParamMask.set(i);
              return;
            }
            if (anim->getParamType(i) == AnimV20::IPureAnimStateHolder::PT_ScalarParam)
            {
              if (param_b && param_b->getBool("trackbar", false))
                astGrp.createTrackFloat(PID_ANIM_PARAM0 + i, nm, st->getParam(i), param_b->getReal("min", 0),
                  param_b->getReal("max", 1), param_b->getReal("step", 0.01));
              else
                astGrp.createEditFloat(PID_ANIM_PARAM0 + i, nm, st->getParam(i));
            }
            else if (anim->getParamType(i) == AnimV20::IPureAnimStateHolder::PT_ScalarParamInt)
              astGrp.createEditInt(PID_ANIM_PARAM0 + i, nm, st->getParamInt(i));
            if (param_b)
              astGrp.setMinMaxStep(PID_ANIM_PARAM0 + i, param_b->getReal("min"), param_b->getReal("max"), param_b->getReal("step"));
          });
        }
      }

      if (animCtrl->getAnimChar())
      {
        PropPanel::ContainerPropertyControl &traceGrp = *animPanel->createGroup(PID_ANIM_TRACE_GROUP, "Legs IK test");
        traceGrp.createEditFloat(PID_ANIM_TRACE_CP_HT_F, "Floor H, front side", coll_plane_ht_front);
        traceGrp.createEditFloat(PID_ANIM_TRACE_CP_HT_R, "Floor H, rear side", coll_plane_ht_rear);
        traceGrp.createCheckBox(PID_ANIM_TRACE_ENABLED, "Traces enabled", trace_ray_enabled);
      }
    }

    if (IEntityGatherTex *iegt = _ent->queryInterface<IEntityGatherTex>())
    {
      TextureIdSet texId;
      int lods = iLodCtrl ? iLodCtrl->getLodCount() : 0;

      texNames.reset();
      for (int l = -1; l < lods && l < 16; l++)
      {
        texId.reset();
        if (!iegt->gatherTextures(texId, l))
          continue;
        String caption;
        if (l == -1)
          caption.printf(0, "All used textures (%d tex)", texId.size());
        else
          caption.printf(0, " Lod%d textures (%d tex)", l, texId.size());
        PropPanel::ContainerPropertyControl &texGrp = *grpSpec->createGroup(PID_TEX_GROUP + l + 1, caption);
        for (TEXTUREID tid : texId)
        {
          TextureMetaData tmd;
          String tex_stor;
          String nm(tmd.decode(get_managed_texture_name(tid), &tex_stor));
          if (nm.suffix("*"))
            nm.pop_back();

          int id = texNames.addNameId(nm);
          if (::get_app().getAssetMgr().findAsset(nm, ::get_app().getAssetMgr().getTexAssetTypeId()))
          {
            texGrp.createButton(PID_TEX_BASE + id, nm);
            if (PropPanel::PropertyControlBase *btn = texGrp.getById(PID_TEX_BASE + id))
              btn->setTooltip(nm.c_str());
          }
          else
            texGrp.createStatic(0, nm);
        }
        if (l >= 0)
          grpSpec->setBool(PID_TEX_GROUP + l + 1, true);
      }
      if (riex.impostor.enabled)
      {
        String caption;
        caption.printf(0, " Impostor textures (3 tex)");
        PropPanel::ContainerPropertyControl &texGrp = *grpSpec->createGroup(PID_TEX_GROUP + lods + 1, caption);
        auto addTex = [&, this](const TEXTUREID &texId, const char *nm) {
          if (texId != BAD_TEXTUREID)
          {
            int id = texNames.addNameId(nm);
            texGrp.createButton(PID_TEX_BASE + id, nm);
            if (PropPanel::PropertyControlBase *btn = texGrp.getById(PID_TEX_BASE + id))
              btn->setTooltip(nm);
          }
          else
          {
            texGrp.createStatic(0, nm);
          }
        };
        addTex(riex.impostor.textureId_ar, String(0, "%s_as", riex.name.c_str()).c_str());
        addTex(riex.impostor.textureId_nt, String(0, "%s_nt", riex.name.c_str()).c_str());
        addTex(riex.impostor.textureId_asad, String(0, "%s_adas", riex.name.c_str()).c_str());
        grpSpec->setBool(PID_TEX_GROUP + lods + 1, true);
      }
    }
    if (iLodCtrl && iLodCtrl->getNamedNodeCount())
    {
      PropPanel::ContainerPropertyControl &nodeGrp =
        *grpSpec->createGroup(PID_NODES_GROUP, String(0, "Node visibility (%d)", iLodCtrl->getNamedNodeCount()));
      int hidden_nodes = 0;
      for (int i = 0; i < iLodCtrl->getNamedNodeCount(); i++)
        if (const char *nm = iLodCtrl->getNamedNode(i))
        {
          if (i + PID_NODES_BASE < PID_NODES_GROUP)
            nodeGrp.createCheckBox(PID_NODES_BASE + i, nm, iLodCtrl->getNamedNodeVisibility(i));
          else
            nodeGrp.createStatic(0, nm);
        }
        else
          hidden_nodes++;
      if (hidden_nodes)
        nodeGrp.createStatic(0, String(0, "and %d helper nodes/bones", hidden_nodes));

      PropPanel::ContainerPropertyControl &nodesFilterByMask =
        *grpSpec->createGroup(PID_MASK_NODES_GROUP, String(0, "Filter nodes by name mask"));

      nodesFilterByMask.createButton(PID_MASK_NODES_UPDATE_BTN, String(0, "Update nodes visibility"));
      nodesFilterByMask.createButton(PID_MASK_NODES_UNCHECK_ALL, String(0, "Uncheck all"));

      const int blockCount = nodeFilterMasksBlk.blockCount();
      for (int i = 0; i < blockCount; ++i)
      {
        const DataBlock *maskBlk = nodeFilterMasksBlk.getBlock(i);
        const char *maskText = maskBlk->getStr("name", "");
        const char *excludeText = maskBlk->getStr("exclude", nullptr);
        const String filterText = excludeText ? String(32, "%s (excl. '%s')", maskText, excludeText) : String(32, "%s", maskText);
        nodesFilterByMask.createCheckBox(PID_MASK_NODES + i, filterText.str(), maskBlk->getBool("isEnabled", false));
      }

      nodesFilterByMask.setBoolValue(true);
    }
  }

  virtual void postFillPropPanel() { get_app().getCompositeEditor().fillCompositeTree(); }

  virtual void onChange(int pcb_id, PropPanel::ContainerPropertyControl *panel)
  {
    if (pcb_id == PID_LODS_GROUP && (entity || riex.vEntity))
    {
      IObjEntity *objEntity = entity ? entity : riex.vEntity;
      ILodController *iLodCtrl = objEntity ? objEntity->queryInterface<ILodController>() : nullptr;
      if (!iLodCtrl)
        return;

      const int n = panel->getInt(PID_LODS_GROUP);
      if (PropPanel::RADIO_SELECT_NONE == n)
        return;

      iLodCtrl->setCurLod(n <= PID_LOD_AUTO_CHOOSE ? -1 : n - PID_LOD_FIRST);
    }
    else if (pcb_id == PID_SEED)
    {
      if (IRandomSeedHolder *irsh = entity ? entity->queryInterface<IRandomSeedHolder>() : nullptr)
        irsh->setSeed(panel->getInt(pcb_id));
    }

    else if (pcb_id == PID_PER_INSTANCE_SEED)
    {
      if (entity)
      {
        if (IRandomSeedHolder *irsh = entity->queryInterface<IRandomSeedHolder>())
          irsh->setPerInstanceSeed(panel->getInt(pcb_id));
      }
      else if (riex.res && riex.handle != rendinst::RIEX_HANDLE_NULL)
        rendinst::set_riextra_instance_seed(riex.handle, panel->getInt(pcb_id) ? panel->getInt(pcb_id) : auto_inst_seed0);
    }

    else if (pcb_id == PID_FORCE_SPD)
    {
      forceSpd = panel->getFloat(pcb_id);
      updateSpd();
    }
    else if (pcb_id == PID_ROTATE_Y)
    {
      rotY = panel->getFloat(pcb_id);
      updateRot();
    }
    else if (pcb_id == PID_ROTATE_X)
    {
      rotX = panel->getFloat(pcb_id);
      updateRot();
    }
    else if (pcb_id == PID_SHOW_FQ_TEX)
    {
      if (ILodController *iLodCtrl = entity ? entity->queryInterface<ILodController>() : nullptr)
      {
        if (texmgr_internal::max_allowed_q != TQL_stub && texmgr_internal::max_allowed_q >= TQL_base)
          texmgr_internal::max_allowed_q = panel->getBool(pcb_id) ? TQL_uhq : TQL_base;
        iLodCtrl->setTexQL(panel->getBool(pcb_id) ? 1 : 0);
        ddsx::tex_pack2_perform_delayed_data_loading();
      }
    }
    else if (pcb_id == PID_SHOW_OCCL_BOX)
      showOcclBox = panel->getBool(pcb_id);
    else if (pcb_id == PID_SHOW_POSITION_GIZMO)
    {
      bool visible = panel->getBool(pcb_id);
      positionGizmoWrapper.setVisible(visible);
      panel->setEnabledById(PID_POSITION_GIZMO, visible);
    }
    else if (pcb_id == PID_POSITION_GIZMO)
      positionGizmoWrapper.setPosition(panel->getPoint3(pcb_id));
    else if (pcb_id == PID_ANIM_SHOW_SKELETON)
      showSkeleton = panel->getBool(pcb_id);
    else if (pcb_id == PID_ANIM_SHOW_SKELETON_NAMES)
      showSkeletonNodeNames = panel->getBool(pcb_id);
    else if (pcb_id == PID_ANIM_NODE_AXIS_LEN)
      nodeAxisLen = panel->getFloat(pcb_id);
    else if (pcb_id == PID_ANIM_SHOW_EFFECTORS)
      showEffectors = panel->getBool(pcb_id);
    else if (pcb_id == PID_ANIM_SHOW_IKSOL)
      showIKSolution = panel->getBool(pcb_id);
    else if (pcb_id == PID_ANIM_SHOW_WABB)
      showWABB = panel->getBool(pcb_id);
    else if (pcb_id == PID_ANIM_SHOW_BSPH)
      showBSPH = panel->getBool(pcb_id);
    else if (pcb_id == PID_ANIM_LOG_IRQ)
      logIrqs = panel->getBool(pcb_id);
    else if (pcb_id >= PID_ANIM_PARAM0 && pcb_id < PID_ANIM_PARAM_LAST)
    {
      if (IAnimCharController *animCtrl = entity ? entity->queryInterface<IAnimCharController>() : NULL)
      {
        AnimV20::AnimationGraph *anim = animCtrl->getAnimGraph();
        AnimV20::IAnimStateHolder *st = animCtrl->getAnimState();
        if (anim && st)
        {
          int i = pcb_id - PID_ANIM_PARAM0;

          if (enumParamMask.get(i))
          {
            int v = panel->getInt(pcb_id);
            if (const DataBlock *b = enumParamBlk.getBlockByName(anim->getParamName(i)))
              if (v >= 0 && v < b->blockCount())
              {
                if (anim->getParamType(i) == AnimV20::IPureAnimStateHolder::PT_ScalarParam)
                  st->setParam(i, b->getBlock(v)->getInt("_enumValue"));
                else if (anim->getParamType(i) == AnimV20::IPureAnimStateHolder::PT_ScalarParamInt)
                  st->setParamInt(i, b->getBlock(v)->getInt("_enumValue"));
              }
          }
          else if (boolParamMask.get(i))
          {
            if (anim->getParamType(i) == AnimV20::IPureAnimStateHolder::PT_ScalarParam)
              st->setParam(i, panel->getBool(pcb_id) ? 1.0f : 0.0f);
            else if (anim->getParamType(i) == AnimV20::IPureAnimStateHolder::PT_ScalarParamInt)
              st->setParamInt(i, panel->getBool(pcb_id) ? 1 : 0);
          }
          else if (anim->getParamType(i) == AnimV20::IPureAnimStateHolder::PT_ScalarParam)
          {
            float v = panel->getFloat(pcb_id);
            st->setParam(i, v);
          }
          else if (anim->getParamType(i) == AnimV20::IPureAnimStateHolder::PT_ScalarParamInt)
            st->setParamInt(i, panel->getInt(pcb_id));

          if (i == animPersCoursePid)
          {
            panel->setFloat(PID_ROTATE_Y, rotY = panel->getFloat(pcb_id));
            updateRot();
          }
        }
      }
    }
    else if (pcb_id == PID_ANIM_SET_NODE)
    {
      if (IAnimCharController *animCtrl = entity ? entity->queryInterface<IAnimCharController>() : NULL)
      {
        int sel = (lastForceAnimSet = panel->getInt(pcb_id)) - 1;
        animCtrl->enqueueAnimNode(sel < 0 ? NULL : animCtrl->getAnimNodeName(sel));
      }
    }
    else if (pcb_id >= PID_NODES_BASE && pcb_id < PID_NODES_GROUP)
    {
      if (ILodController *iLodCtrl = entity->queryInterface<ILodController>())
        iLodCtrl->setNamedNodeVisibility(pcb_id - PID_NODES_BASE, panel->getBool(pcb_id));
    }
    else if (pcb_id == PID_FPSCAM_GROUP)
      changeFpsCamView(panel->getInt(pcb_id));
    else if (pcb_id == PID_PN_TRIANGULATION)
    {
      pnTriangulation = panel->getBool(pcb_id);
      updatePnTriangulation();
    }
    else if (pcb_id == PID_ANIM_RAGDOLL_SPRING_FACTOR)
      physsimulator::springFactor = panel->getFloat(pcb_id);
    else if (pcb_id == PID_ANIM_RAGDOLL_DAMPER_FACTOR)
      physsimulator::damperFactor = panel->getFloat(pcb_id);
    else if (pcb_id == PID_ANIM_RAGDOLL_BULLET_IMPULSE)
      ragdollBulletImpulse = panel->getFloat(pcb_id);
    else if (pcb_id == PID_ANIM_CHARDEP_PY_OFS || pcb_id == PID_ANIM_CHARDEP_P_SCL || pcb_id == PID_ANIM_CHARDEP_S_SCL)
    {
      if (IAnimCharController *animCtrl = entity ? entity->queryInterface<IAnimCharController>() : NULL)
        if (animCtrl->getAnimChar())
        {
          if (pcb_id == PID_ANIM_CHARDEP_S_SCL)
          {
            animCtrl->getAnimChar()->applyCharDepScale(panel->getFloat(PID_ANIM_CHARDEP_S_SCL));
            float pyOfs, pxScale, pyScale, pzScale, sScale;
            if (animCtrl->getAnimChar()->getCharDepScales(pyOfs, pxScale, pyScale, pzScale, sScale))
            {
              panel->setFloat(PID_ANIM_CHARDEP_PY_OFS, pyOfs);
              panel->setPoint3(PID_ANIM_CHARDEP_P_SCL, Point3(pxScale, pyScale, pzScale));
            }
          }
          else
            animCtrl->getAnimChar()->updateCharDepScales(panel->getFloat(PID_ANIM_CHARDEP_PY_OFS),
              panel->getPoint3(PID_ANIM_CHARDEP_P_SCL).x, panel->getPoint3(PID_ANIM_CHARDEP_P_SCL).y,
              panel->getPoint3(PID_ANIM_CHARDEP_P_SCL).z, panel->getFloat(PID_ANIM_CHARDEP_S_SCL));
        }
    }
    else if (pcb_id == PID_ANIM_TRACE_CP_HT_F || pcb_id == PID_ANIM_TRACE_CP_HT_R)
    {
      coll_plane_ht_front = panel->getFloat(PID_ANIM_TRACE_CP_HT_F);
      coll_plane_ht_rear = panel->getFloat(PID_ANIM_TRACE_CP_HT_R);
      if (!ragdoll)
      {
        physsimulator::end();
        physsimulator::begin(NULL, physsimulator::PHYS_BULLET, 1, physsimulator::SCENE_TYPE_GROUP, 0.0f, coll_plane_ht_front,
          coll_plane_ht_rear);
      }
    }
    else if (pcb_id == PID_ANIM_TRACE_ENABLED)
      trace_ray_enabled = panel->getBool(pcb_id);
    else if (pcb_id == PID_SHOW_COLLIDER)
    {
      showCollider = panel->getBool(pcb_id);
      panel->setEnabledById(PID_SHOW_COLLIDER_BBOX, showCollider);
      panel->setEnabledById(PID_SHOW_COLLIDER_PHYS_COLLIDABLE, showCollider);
      panel->setEnabledById(PID_SHOW_COLLIDER_TRACEABLE, showCollider);
    }
    else if (pcb_id == PID_SHOW_COLLIDER_BBOX)
      showColliderBbox = panel->getBool(pcb_id);
    else if (pcb_id == PID_SHOW_COLLIDER_PHYS_COLLIDABLE)
      showColliderPhysCollidable = panel->getBool(pcb_id);
    else if (pcb_id == PID_SHOW_COLLIDER_TRACEABLE)
      showColliderTraceable = panel->getBool(pcb_id);
    else if (pcb_id >= PID_MASK_NODES)
    {
      if (ILodController *iLodCtrl = entity->queryInterface<ILodController>())
      {
        RegExp re;
        applyMask(pcb_id - PID_MASK_NODES, iLodCtrl, re, panel);
      }
    }
  }
  virtual void onClick(int pcb_id, PropPanel::ContainerPropertyControl *panel)
  {
    if (pcb_id == PID_GENERATE_SEED && entity)
      if (IRandomSeedHolder *irsh = entity->queryInterface<IRandomSeedHolder>())
      {
        panel->setInt(PID_SEED, grnd() & 0x7FFF);
        irsh->setSeed(panel->getInt(PID_SEED));
      }

    if (pcb_id == PID_GENERATE_PER_INSTANCE_SEED && (entity || (riex.res && riex.handle != rendinst::RIEX_HANDLE_NULL)))
    {
      panel->setInt(PID_PER_INSTANCE_SEED, grnd() & 0x7FFF);
      int seed = panel->getInt(PID_PER_INSTANCE_SEED);
      if (entity)
      {
        if (IRandomSeedHolder *irsh = entity->queryInterface<IRandomSeedHolder>())
          irsh->setPerInstanceSeed(seed);
      }
      else
        rendinst::set_riextra_instance_seed(riex.handle, seed ? seed : auto_inst_seed0);
    }

    if (pcb_id >= PID_TEX_BASE && pcb_id < PID_TEX_GROUP)
    {
      const char *tex_nm = texNames.getName(pcb_id - PID_TEX_BASE);
      clipboard::set_clipboard_ansi_text(tex_nm);
      DagorAsset *tex_a = ::get_app().getAssetMgr().findAsset(tex_nm, ::get_app().getAssetMgr().getTexAssetTypeId());
      if (tex_a->props.getBool("convert", false) || (tex_a->isVirtual() && !tex_a->getSrcFileName()))
      {
        if (wingw::message_box(wingw::MBS_QUEST | wingw::MBS_YESNO, "Texture asset",
              "Texture asset name \"%s\" is copied to clipboard.\nDo you want also export it as DDS?",
              tex_nm) == PropPanel::DIALOG_ID_YES)
        {
          String fn(0, "%s/%s.dds", ::get_app().getWorkspace().getSdkDir(), tex_nm);
          dd_simplify_fname_c(fn);
          fn = wingw::file_save_dlg(EDITORCORE->getWndManager()->getMainWindow(), "Export texture to DDS",
            "DDS format|*.dds|All files|*.*", "dds", ::get_app().getWorkspace().getSdkDir(), fn);

          if (!fn.empty())
            export_texture_as_dds(fn, *tex_a);
        }
      }
      else
        wingw::message_box(wingw::MBS_OK, "Texture asset", "Texture asset name \"%s\" is copied to clipboard.", tex_nm);
    }

    if (pcb_id >= PID_ANIM_STATE0 && pcb_id < PID_ANIM_STATE_LAST)
    {
      if (IAnimCharController *animCtrl = entity ? entity->queryInterface<IAnimCharController>() : NULL)
      {
        panel->setInt(PID_ANIM_SET_NODE, lastForceAnimSet = 0);
        animCtrl->enqueueAnimNode(NULL);
        int stateIdx = pcb_id - PID_ANIM_STATE0;
        animCtrl->enqueueState(stateIdx, forceSpd);
        lastAnimStatesSet.push_back(stateIdx);
      }
    }

    if (pcb_id == PID_ANIM_ACT_RESTART)
      if (IAnimCharController *animCtrl = entity ? entity->queryInterface<IAnimCharController>() : NULL)
      {
        stopRagdoll();
        panel->setInt(PID_ANIM_SET_NODE, lastForceAnimSet = 0);
        animCtrl->enqueueAnimNode(NULL);
        animCtrl->restart();
        panel->setCaption(PID_ANIM_RAGDOLL_START, ragdoll ? "stop" : "START");
      }

    if (pcb_id == PID_ANIM_ACT_PAUSE)
      if (IAnimCharController *animCtrl = entity ? entity->queryInterface<IAnimCharController>() : NULL)
      {
        animCtrl->setPaused(!animCtrl->isPaused());
        panel->setCaption(pcb_id, animCtrl->isPaused() ? "resume" : "pause");
      }
    if (pcb_id == PID_ANIM_ACT_SLOWMO)
      if (IAnimCharController *animCtrl = entity ? entity->queryInterface<IAnimCharController>() : NULL)
      {
        animCtrl->setTimeScale(animCtrl->getTimeScale() == 1.f ? 0.1f : 1.0f);
        panel->setCaption(pcb_id, animCtrl->getTimeScale() != 1.f ? "normal" : "slow-mo");
      }
    if (pcb_id == PID_ANIM_RAGDOLL_START)
    {
      if (!ragdoll)
        startRagdoll();
      else
        stopRagdoll();
      panel->setCaption(pcb_id, ragdoll ? "stop" : "START");
    }
    if (pcb_id == PID_ANIM_DUMP_DEBUG)
      if (IAnimCharController *animCtrl = entity ? entity->queryInterface<IAnimCharController>() : NULL)
      {
        Tab<const char *> params;
        if (wingw::is_key_pressed(wingw::V_CONTROL))
          params.push_back("no_skel");
        if (wingw::is_key_pressed(wingw::V_SHIFT))
          params.push_back("tm");
        DAEDITOR3.conWarning("--- animchar blend-state dump ---");
        onConsoleCommand("animchar.blender", params);
        ::get_app().getConsole().showConsole();
      }
    if (pcb_id == PID_ANIM_DUMP_UNUSED_BN)
      if (IAnimCharController *animCtrl = entity ? entity->queryInterface<IAnimCharController>() : NULL)
      {
        Tab<const char *> params;
        DAEDITOR3.conWarning("--- animchar unused blend nodes dump ---");
        onConsoleCommand("animchar.unusedBlendNodes", params);
        ::get_app().getConsole().showConsole();
      }
    if (pcb_id == PID_MASK_NODES_UPDATE_BTN)
    {
      updateAllMaskFilters(panel);
    }
    else if (pcb_id == PID_MASK_NODES_UNCHECK_ALL)
    {
      const int blockCount = nodeFilterMasksBlk.blockCount();
      for (int i = 0; i < blockCount; ++i)
        panel->setBool(PID_MASK_NODES + i, false);

      updateAllMaskFilters(panel);
    }
    if (pcb_id == PID_OBJECT_PROPERTIES_BUTTON)
      objPropEditor.onClick();

    if (pcb_id == PID_ANIM_TREE_IMGUI)
    {
      if (currentAnimcharEid)
        g_entity_mgr->destroyEntity(currentAnimcharEid);
      else
      {
        ecs::ComponentsInitializer list;
        list[ECS_HASH("animchar__res")] = assetName;
        currentAnimcharEid = g_entity_mgr->createEntityAsync("animchar_base+anim_tree_viewer", eastl::move(list));
      }
    }
  }

  virtual bool onConsoleCommand(const char *cmd, dag::ConstSpan<const char *> params)
  {
    if (stricmp(cmd, "animchar.blender") == 0)
    {
      bool tm = false, terse = false;
      bool no_skel = false, no_bn = false;
      for (int i = 0; i < params.size(); i++)
        if (strcmp(params[i], "tm") == 0)
          tm = true;
        else if (strcmp(params[i], "terse") == 0)
          terse = true;
        else if (strcmp(params[i], "no_skel") == 0)
          no_skel = true;
        else if (strcmp(params[i], "no_bn") == 0)
          no_bn = true;
        else
        {
          DAEDITOR3.conError("unknown option %s", params[i]);
          return true;
        }

      if (IAnimCharController *animCtrl = entity ? entity->queryInterface<IAnimCharController>() : NULL)
      {
        if (AnimV20::IAnimCharacter2 *ac = animCtrl->getAnimChar())
        {
          ac->createDebugBlenderContext(!terse);
          if (const DataBlock *b = ac->getDebugBlenderState(tm))
          {
            DataBlock bcopy;
            if (no_skel || no_bn)
            {
              bcopy = *b;
              if (no_skel)
                bcopy.removeBlock("skeleton");
              if (no_bn)
                bcopy.removeBlock("animNodes");
              b = &bcopy;
            }
            ConTextOutStream cwr(::get_app().getConsole());
            dblk::export_to_json_text_stream(*b, cwr, true);
          }
          else
            DAEDITOR3.conError("cannot get debug blend state for animChar");
        }
        else
          DAEDITOR3.conError("cannot get animChar interface");
      }
      else
        DAEDITOR3.conError("current asset is not animChar");
      return true;
    }
    if (stricmp(cmd, "animchar.unusedBlendNodes") == 0)
    {
      IAnimCharController *animCtrl = entity ? entity->queryInterface<IAnimCharController>() : NULL;
      if (!animCtrl)
      {
        DAEDITOR3.conError("current asset is not animChar");
        return true;
      }

      AnimV20::IAnimCharacter2 *ac = animCtrl->getAnimChar();
      if (!ac)
      {
        DAEDITOR3.conError("cannot get animChar interface");
        return true;
      }

      AnimV20::AnimationGraph *graph = ac->getAnimGraph();
      if (!graph)
      {
        DAEDITOR3.conError("cannot get animGraph for animChar");
        return true;
      }

      eastl::hash_map<AnimV20::IAnimBlendNode *, bool> usedNodes;
      graph->getUsedBlendNodes(usedNodes);

      for (int i = 0; i < graph->getAnimNodeCount(); i++)
        if (AnimV20::IAnimBlendNode *node = graph->getBlendNodePtr(i))
          if (usedNodes.find(node) == usedNodes.end() && !node->isSubOf(AnimV20::AnimBlendNodeNullCID))
            DAEDITOR3.getCon().addMessage(ILogWriter::NOTE, "unused node: %s", graph->getBlendNodeName(node));

      return true;
    }
    if (stricmp(cmd, "animchar.nodemask") == 0)
    {
      if (IAnimCharController *animCtrl = entity ? entity->queryInterface<IAnimCharController>() : NULL)
      {
        if (AnimV20::IAnimCharacter2 *ac = animCtrl->getAnimChar())
        {
          if (const DataBlock *b = ac->getDebugNodemasks())
          {
            if (!params.size())
            {
              DAEDITOR3.conNote("animchar: %d nodemasks:", b->blockCount());
              for (int i = 0; i < b->blockCount(); i++)
                DAEDITOR3.conNote("  %s", b->getBlock(i)->getBlockName());
            }
            else
            {
              if (strcmp(params[0], "@all") == 0)
              {
                DAEDITOR3.conNote("animchar: %d nodemasks", b->blockCount());
                ConTextOutStream cwr(::get_app().getConsole());
                dblk::export_to_json_text_stream(*b, cwr, true);
              }
              else if (const DataBlock *b2 = b->getBlockByName(params[0]))
              {
                DAEDITOR3.conNote("animchar: nodemask %s", b2->getBlockName());
                ConTextOutStream cwr(::get_app().getConsole());
                dblk::export_to_json_text_stream(*b2, cwr, true);
              }
              else
                DAEDITOR3.conError("missing nodemask named <%s>", params[0]);
            }
          }
          else
            DAEDITOR3.conError("cannot get debug blend state for animChar");
        }
        else
          DAEDITOR3.conError("cannot get animChar interface");
      }
      else
        DAEDITOR3.conError("current asset is not animChar");
    }
    if (stricmp(cmd, "animchar.vars") == 0)
    {
      using namespace AnimV20;
      if (IAnimCharController *animCtrl = entity ? entity->queryInterface<IAnimCharController>() : NULL)
      {
        AnimationGraph *anim = animCtrl->getAnimGraph();
        IAnimStateHolder *st = animCtrl->getAnimState();
        if (anim && st)
        {
          int total_cnt = 0, listed_cnt = 0;
          DAEDITOR3.conNote("animState=%p  %d words, sz=%d", st, anim->getParamNames().nameCount(), st->getSize());
          iterate_names(anim->getParamNames(), [&](int id, const char *name) {
            if (anim->getParamType(id) == AnimCommonStateHolder::PT_Reserved)
              return;
            total_cnt++;
            if (params.size() && !strstr(name, params[0]))
              return;
            listed_cnt++;
            switch (anim->getParamType(id))
            {
              case AnimCommonStateHolder::PT_ScalarParam: DAEDITOR3.conNote("[%d] %40s float=%g", id, name, st->getParam(id)); break;
              case AnimCommonStateHolder::PT_ScalarParamInt:
                DAEDITOR3.conNote("[%d] %40s int  =%d", id, name, st->getParamInt(id));
                break;
              case AnimCommonStateHolder::PT_TimeParam: DAEDITOR3.conNote("[%d] %40s timer=%g", id, name, st->getParam(id)); break;

              case AnimCommonStateHolder::PT_InlinePtr:
              case AnimCommonStateHolder::PT_InlinePtrCTZ:
              case AnimCommonStateHolder::PT_Fifo3:
              case AnimCommonStateHolder::PT_Effector:
                DAEDITOR3.conNote("[%d] %40s inline(T:%d)=%p, %d", id, name, anim->getParamType(id), st->getInlinePtr(id),
                  st->getInlinePtrMaxSz(id));
                break;
            }
          });
          DAEDITOR3.conNote("listed %d params (of %d total)", listed_cnt, total_cnt);
        }
      }
    }
    return false;
  }
  virtual const char *onConsoleCommandHelp(const char *cmd)
  {
    if (stricmp(cmd, "animchar.blender") == 0)
      return "animchar.blender [tm] [terse] [no_skel] [no_bn]\n"
             "  dumps current per-node animstate for current animChar asset";
    if (stricmp(cmd, "animchar.unusedBlendNodes") == 0)
      return "animchar.unusedBlendNodes\n"
             "  dumps blend nodes not referenced by root node or any states for current animChar asset";
    if (stricmp(cmd, "animchar.nodemask") == 0)
      return "animchar.nodemask [mask_name|@all]\n"
             "  dumps list of nodemasks or nodes for specified mask_name";
    if (stricmp(cmd, "animchar.vars") == 0)
      return "animchar.vars [subst_filter]\n"
             "  dumps (optinally filtered) animState's var values for current animChar asset";
    return NULL;
  }

  virtual intptr_t irq(int type, intptr_t p1, intptr_t p2, intptr_t p3)
  {
    if (logIrqs)
      if (IAnimCharController *animCtrl = entity ? entity->queryInterface<IAnimCharController>() : NULL)
        if (auto *ag = animCtrl->getAnimGraph())
          DAEDITOR3.conNote("irq%d (%s) from \"%s\" at frame %d (tick=%d), at %.3f sec (animtime)", type, AnimV20::getIrqName(type),
            ag->getBlendNodeName((AnimV20::IAnimBlendNode *)(void *)p1), p2 / (AnimV20::TIME_TicksPerSec / 30), p2,
            animCtrl->getAnimState() ? animCtrl->getAnimState()->getParam(ag->PID_GLOBAL_TIME) : 0);
    return AnimV20::GIRQR_NoResponse;
  }

  void updateSpd()
  {
    if (IAnimCharController *animCtrl = entity ? entity->queryInterface<IAnimCharController>() : NULL)
      for (const auto lastState : lastAnimStatesSet)
        animCtrl->enqueueState(lastState, forceSpd);
  }

  void updateRot()
  {
    if (entity)
    {
      float fullRotY = rotY;
      if (IAnimCharController *animCtrl = entity->queryInterface<IAnimCharController>())
      {
        if (animPersCoursePid > 0 && fabsf(animCtrl->getAnimState()->getParam(animPersCoursePid) - rotY) > 1e-3)
          animCtrl->getAnimState()->setParam(animPersCoursePid, rotY);
        if (animPersCourseDeltaPid > 0)
          fullRotY += animCtrl->getAnimState()->getParam(animPersCourseDeltaPid);
      }
      entity->setTm(rotyTM(fullRotY * DEG_TO_RAD) * rotxTM(rotX * DEG_TO_RAD));
      if (IAnimCharController *animCtrl = entity->queryInterface<IAnimCharController>())
      {
        if (AnimV20::IAnimStateHolder *st = animCtrl->getAnimState())
          st->advance(0);
        if (AnimV20::IAnimCharacter2 *ac = animCtrl->getAnimChar())
          ac->doRecalcAnimAndWtm();
      }
    }
    else if (riex.res && riex.handle != rendinst::RIEX_HANDLE_NULL)
    {
      mat33f m3;
      v_mat33_make_rot_cw_zyx(m3, v_make_vec4f(-rotX * DEG_TO_RAD, -rotY * DEG_TO_RAD, 0, 0));
      mat44f tm;
      v_mat44_ident(tm);
      tm.set33(m3);
      rendinst::moveRIGenExtra44(riex.handle, tm, false, false);
    }
  }

  void updatePnTriangulation()
  {
    static int object_tess_factorVarId = ::get_shader_variable_id("object_tess_factor", true);
    ShaderGlobal::set_real(object_tess_factorVarId, pnTriangulation ? 1.0f : 0.0f);
  }

  void changeFpsCamView(int id)
  {
    if (id == fpsCamViewId)
      return;
    IAnimCharController *animCtrl = entity ? entity->queryInterface<IAnimCharController>() : NULL;
    if (fpsCamViewId == PID_FPSCAM_DEF)
    {
      // save camera
      if (IGenViewportWnd *vp = EDITORCORE->getViewport(0))
      {
        vp->getCameraTransform(fpsCamViewLastWtm);
        vp->getZnearZfar(fpsCamViewLastZn, fpsCamViewLastZf);
      }
    }
    fpsCamViewId = id;

    if (animCtrl && animCtrl->getAnimChar())
      if (DynamicRenderableSceneInstance *scn = animCtrl->getAnimChar()->getSceneInstance())
        for (int i = 0; i < scn->getNodeCount(); i++)
          scn->showNode(i, true);

    if (fpsCamViewId == PID_FPSCAM_DEF)
    {
      // restore camera
      if (IGenViewportWnd *vp = EDITORCORE->getViewport(0))
      {
        vp->setCameraTransform(fpsCamViewLastWtm);
        EDITORCORE->setViewportZnearZfar(fpsCamViewLastZn, fpsCamViewLastZf);
      }
    }
    else
    {
      // apply fps camera settings
      FpsCameraView &fvc = fpsCamView[fpsCamViewId - PID_FPSCAM_FIRST];
      if (animCtrl && animCtrl->getAnimChar())
        if (DynamicRenderableSceneInstance *scn = animCtrl->getAnimChar()->getSceneInstance())
        {
          iterate_names(fvc.hideNodes, [&](int, const char *name) {
            int id = scn->getNodeId(name);
            if (id >= 0)
              scn->showNode(id, false);
          });
        }
      EDITORCORE->setViewportZnearZfar(fvc.zn, fvc.zf);
    }
  }
  void startRagdoll()
  {
    IAnimCharController *animCtrl = entity ? entity->queryInterface<IAnimCharController>() : NULL;
    auto *animChar = animCtrl ? animCtrl->getAnimChar() : NULL;
    if (!animChar || ragdoll)
      return;

    ragdoll = phys_bullet_start_ragdoll(animChar, ZERO<Point3>());
    physsimulator::simulate(0.001);
  }
  void stopRagdoll()
  {
    IAnimCharController *animCtrl = entity ? entity->queryInterface<IAnimCharController>() : NULL;
    auto *animChar = animCtrl ? animCtrl->getAnimChar() : NULL;
    if (!animChar || !ragdoll)
      return;
    animChar->setPostController(NULL);
    phys_bullet_delete_ragdoll(ragdoll);

    physsimulator::end();
    physsimulator::begin(NULL, physsimulator::PHYS_BULLET, 1, physsimulator::SCENE_TYPE_GROUP, 0.0f, coll_plane_ht_front,
      coll_plane_ht_rear);
  }

  virtual bool handleMouseLBPress(IGenViewportWnd *wnd, int x, int y, bool inside, int buttons, int key_modif)
  {
    if (!inside)
      return false;
    compositeEditorViewport.handleMouseLBPress(wnd, x, y, entity);
    positionGizmoWrapper.onDragPress(rotX, rotY, wnd, x, y);
    if (!physsimulator::getPhysWorld())
      return false;

    Point3 dir, world;
    wnd->clientToWorld(Point2(x, y), world, dir);
    springConnected = physsimulator::connectSpring(world, dir);
    return springConnected;
  }

  virtual bool handleMouseLBRelease(IGenViewportWnd *wnd, int x, int y, bool inside, int buttons, int key_modif)
  {
    positionGizmoWrapper.onDragRelease();
    if (!springConnected || !physsimulator::getPhysWorld())
      return false;

    springConnected = false;
    return physsimulator::disconnectSpring();
  }

  virtual bool handleMouseMove(IGenViewportWnd *wnd, int x, int y, bool inside, int buttons, int key_modif)
  {
    if (positionGizmoWrapper.onDragMove(rotX, rotY, wnd, x, y))
      getPluginPanel()->setPoint3(PID_POSITION_GIZMO, positionGizmoWrapper.getPosition());

    if (!springConnected || !physsimulator::getPhysWorld())
      return false;

    Point3 dir, world;
    wnd->clientToWorld(Point2(x, y), world, dir);
    physsimulator::setSpringCoord(world, dir);
    return true;
  }

  virtual bool handleMouseRBPress(IGenViewportWnd *wnd, int x, int y, bool inside, int buttons, int key_modif)
  {
    if (!physsimulator::getPhysWorld())
      return false;

    Point3 dir, world;
    wnd->clientToWorld(Point2(x, y), world, dir);
    return physsimulator::shootAtObject(world, dir, ragdollBulletImpulse);
  }

  // IEntityViewPluginInterface
  virtual bool isMouseOverSelectedCompositeSubEntity(IGenViewportWnd *wnd, int x, int y, IObjEntity *main_entity) override
  {
    if (!main_entity)
      return false;

    IObjEntity *selectedEntity = CompositeEditorViewport::getSelectedSubEntity(entity);
    if (!selectedEntity)
      return false;

    IObjEntity *hitEntity = compositeEditorViewport.getHitSubEntity(wnd, x, y, *main_entity);
    return hitEntity == selectedEntity;
  }

private:
  PositionGizmoWrapper positionGizmoWrapper;
  IObjEntity *entity;
  SimpleString skeletonRef;
  BBox3 occluderBox;
  Tab<Point3> occluderQuad;
  bool showOcclBox;
  bool showSkeleton, showEffectors, showIKSolution;
  bool showSkeletonNodeNames;
  float nodeAxisLen = 0.0;
  bool showWABB;
  bool showBSPH;
  bool logIrqs;
  bool pnTriangulation;
  float rotX, rotY;
  eastl::fixed_ring_buffer<int, SAVE_LAST_STATES_COUNT> lastAnimStatesSet = {-1, -1, -1, -1};
  int lastAnimStateSet, animPersCoursePid, animPersCourseDeltaPid;
  int lastForceAnimSet;
  FastNameMapEx texNames;
  DataBlock animCharVarSetts;
  DataBlock enumParamBlk;
  DataBlock nodeFilterMasksBlk;
  EntityMaterialEditor matEditor;
  ObjectPropertyEditor objPropEditor;
  Bitarray enumParamMask;
  Bitarray boolParamMask;
  Tab<FpsCameraView> fpsCamView;
  int fpsCamViewId;
  TMatrix fpsCamViewLastWtm;
  float fpsCamViewLastZn = 0.1, fpsCamViewLastZf = 1000.0;
  bool forceRiExtra;
  float forceSpd = -1.f;
  void *ragdoll = NULL;
  bool springConnected = false;
  float ragdollBulletImpulse = 2;

  bool showCollider = false;
  bool showColliderBbox = false;
  bool showColliderPhysCollidable = false;
  bool showColliderTraceable = false;
  CollisionResource *collisionResource = NULL;
  GeomNodeTree *collisionResourceNodeTree = NULL;
  eastl::string assetName;
  ecs::EntityId currentAnimcharEid;
  CompositeEditorViewport compositeEditorViewport;
};

static InitOnDemand<EntityViewPlugin> plugin;

static void __stdcall reinit_cb_dummy(void *) {}

static bool animchar_trace_static_ray_down(const Point3 &from, float max_t, float &out_t, intptr_t ctx)
{
  if (!trace_ray_enabled)
    return false;
  float plane_h = ((from.z < 0) ? coll_plane_ht_front : coll_plane_ht_rear);
  if (from.y < plane_h || from.y - plane_h > max_t)
    return false;
  out_t = from.y - plane_h;
  return true;
}
static bool animchar_trace_static_ray_dir(const Point3 &from, Point3 dir, float max_t, float &out_t, intptr_t ctx)
{
  // not implemented
  return false;
}
static bool animchar_trace_static_multiray(dag::Span<AnimCharV20::LegsIkRay> traces, bool down, intptr_t ctx)
{
  if (!trace_ray_enabled)
    return false;
  bool hit = false;
  for (auto &ray : traces)
    if (down)
      hit |= animchar_trace_static_ray_down(ray.pos, ray.t, ray.t, ctx);
    else
      hit |= animchar_trace_static_ray_dir(ray.pos, ray.dir, ray.t, ray.t, ctx);
  return hit;
}

void init_plugin_entities()
{
  if (!IEditorCoreEngine::get()->checkVersion())
  {
    debug("incorrect version!");
    return;
  }

  ::plugin.demandInit();

  IEditorCoreEngine::get()->registerPlugin(::plugin);
  if (IRendInstGenService *rigenSrv = EDITORCORE->queryEditorInterface<IRendInstGenService>())
  {
    rigenSrv->createRtRIGenData(-1024, -1024, 32, 8, 8, 8, {}, -1024, -1024);
    rigenSrv->setReinitCallback(&reinit_cb_dummy, NULL);
  }
  AnimCharV20::trace_static_multiray = animchar_trace_static_multiray;

  get_app().getCompositeEditor().setEntityViewPluginInterface(*plugin);
}
