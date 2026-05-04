// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include "a2d.h"

#include <EASTL/string.h>
#include <EASTL/vector_set.h>
#include <EASTL/string_hash_map.h>

#include "../av_plugin.h"
#include "../av_tree.h"
#include "../av_appwnd.h"

#include <de3_interface.h>
#include <de3_lodController.h>
#include <de3_animCtrl.h>

#include <assetsGui/av_selObjDlg.h>
#include <assets/asset.h>
#include <assets/assetPlugin.h>

#include <gameRes/dag_gameResources.h>
#include <gameRes/dag_stdGameResId.h>
#include <gui/dag_stdGuiRender.h>
#include <gui/dag_stdGuiRenderEx.h>
#include <propPanel/control/container.h>

#include <osApiWrappers/dag_clipboard.h>
#include <anim/dag_animChannels.h>
#include <anim/dag_animKeyInterp.h>
#include <anim/dag_simpleNodeAnim.h>
#include <math/dag_mathUtils.h>
#include <math/dag_mathAng.h>
#include <math/dag_capsule.h>
#include <libTools/util/blkUtil.h>
#include <util/dag_delayedAction.h>

#include <debug/dag_debug3d.h>

enum // gui pids
{
  PID_KEY_LABELS_GROUP,
  PID_KEY_LABELS_RADIO,
  PID_PROP_GRP,
  PID_PLAY_GRP,
  PID_CHAR_NAME,
  PID_KEY_TRACKBAR,
  PID_PLAY_PROGR,
  PID_PLAY_ANIM,
  PID_PLAY_DUR,
  PID_PLAY_CUR_TIME,
  PID_PLAY_MUL,
  PID_PLAY_PARAMS_RESET,
  PID_PLAY_LOOP,
  PID_SHOW_SKEL,
  PID_SHOW_INVIS,
  PID_DISABLE_ADDITIVITY,
  PID_SHOW_SELECTED_NODE_TRAJECTORY,
  PID_SELECTED_NODE_TRAJECTORY_GROUP,
  PID_SELECTED_NODE_TRAJECTORY_IS_WORLD,
  PID_SELECTED_NODE_TRAJECTORY_TIME_STEP,
  PID_SELECTED_NODE_TRAJECTORY_TIME_MIN,
  PID_SELECTED_NODE_TRAJECTORY_TIME_MAX,
  PID_SELECTED_NODE_TRAJECTORY_SET_MIN_FROM_CURRENT,
  PID_SELECTED_NODE_TRAJECTORY_SET_MAX_FROM_CURRENT,
  PID_SELECTED_NODE_TRAJECTORY_DRAW_ALPHA,
  PID_SELECTED_NODE_TRAJECTORY_DRAW_AXIS_SIZE,
  PID_SELECTED_NODE_TRAJECTORY_DRAW_AXIS_X,
  PID_SELECTED_NODE_TRAJECTORY_DRAW_AXIS_Y,
  PID_SELECTED_NODE_TRAJECTORY_DRAW_AXIS_Z,
  PID_SELECTED_NODE_INFO_GROUP,
  PID_SELECTED_NODE_TM_POS,
  PID_SELECTED_NODE_TM_ROT,
  PID_SELECTED_NODE_TM_SCALE,
  PID_SELECTED_NODE_TM_COPY,
  PID_SELECTED_NODE_WTM_POS,
  PID_SELECTED_NODE_WTM_ROT,
  PID_SELECTED_NODE_WTM_SCALE,
  PID_SELECTED_NODE_WTM_COPY,
  PID_SELECTED_NODE_NAME,
  PID_ANIM_NODES_GROUP,
  PID_ANIM_NODES_GROUP_LAST = PID_ANIM_NODES_GROUP + 1024
};

enum // prs masks
{
  ANIM_NODE_POS = 1 << 0,
  ANIM_NODE_ROT = 1 << 1,
  ANIM_NODE_SCL = 1 << 2
};

enum // selected node trajectory draw flags
{
  TRAJECTORY_FLAGS_DRAW_X = 1 << 0,
  TRAJECTORY_FLAGS_DRAW_Y = 1 << 1,
  TRAJECTORY_FLAGS_DRAW_Z = 1 << 2,
};

extern String a2d_last_ref_model;

static float getMinPoint(const Point3 &p3, real clamp)
{
  real m = p3.x < p3.y ? p3.x : p3.y;
  m = m < p3.z ? m : p3.z;
  return m < clamp ? clamp : m;
}

static void drawSkeletonLinks(const GeomNodeTree &tree, dag::Index16 n, ILodController &ctrl)
{
  Point3 pos = tree.getNodeWposRelScalar(n);
  for (int i = 0; i < tree.getChildCount(n); i++)
  {
    auto cn = tree.getChildNodeIdx(n, i);
    int nodeIdx = ctrl.getNamedNodeIdx(tree.getNodeName(cn));
    if (nodeIdx >= 0)
    {
      draw_cached_debug_line(pos, tree.getNodeWposRelScalar(cn), E3DCOLOR(255, 255, 0, 127));
      draw_cached_debug_sphere(pos, 0.01f, E3DCOLOR(0, 0, 255, 127));
    }
    else
      draw_cached_debug_sphere(pos, 0.01f, E3DCOLOR(255, 0, 0, 127));
    drawSkeletonLinks(tree, cn, ctrl);
  }
  if (tree.getChildCount(n) == 0)
    draw_cached_debug_sphere(pos, 0.005f, E3DCOLOR(0, 0, 255, 127));
}

static void drawExportedNodes(const GeomNodeTree &tree, const dag::Vector<A2dPlugin::AnimNode> &animNodes, int selected_node)
{
  for (int i = 0; i < animNodes.size(); ++i)
  {
    TMatrix tm;
    E3DCOLOR color;
    if (animNodes[i].skelIdx)
    {
      tree.getNodeWtmScalar(animNodes[i].skelIdx, tm);
      color = E3DCOLOR(0, 0, 255, 127);
    }
    else
    {
      tm = animNodes[i].tm;
      color = E3DCOLOR(255, 127, 0, 127);
    }
    draw_cached_debug_sphere(tm.col[3], 0.01f, color);
    const float axisLen = i == selected_node ? 0.1f : 0.02f;
    const int axisAlpha = i == selected_node ? 255 : selected_node >= 0 ? 16 : 127;
    draw_cached_debug_line(tm.col[3], tm.col[3] + tm.col[0] * axisLen, E3DCOLOR(255, 0, 0, axisAlpha));
    draw_cached_debug_line(tm.col[3], tm.col[3] + tm.col[1] * axisLen, E3DCOLOR(0, 255, 0, axisAlpha));
    draw_cached_debug_line(tm.col[3], tm.col[3] + tm.col[2] * axisLen, E3DCOLOR(0, 0, 255, axisAlpha));
  }
}

static void drawTrajectory(const dag::Vector<TMatrix> &trajectory, float axis_length, int alpha, int draw_flags)
{
  if (draw_flags & TRAJECTORY_FLAGS_DRAW_X)
    draw_cached_debug_line(trajectory[0].col[3], trajectory[0].col[3] + trajectory[0].col[0] * axis_length,
      E3DCOLOR(255, 0, 0, alpha));
  if (draw_flags & TRAJECTORY_FLAGS_DRAW_Y)
    draw_cached_debug_line(trajectory[0].col[3], trajectory[0].col[3] + trajectory[0].col[1] * axis_length,
      E3DCOLOR(0, 255, 0, alpha));
  if (draw_flags & TRAJECTORY_FLAGS_DRAW_Z)
    draw_cached_debug_line(trajectory[0].col[3], trajectory[0].col[3] + trajectory[0].col[2] * axis_length,
      E3DCOLOR(0, 0, 255, alpha));

  for (int i = 1; i < trajectory.size(); ++i)
  {
    draw_cached_debug_line(trajectory[i - 1].getcol(3), trajectory[i].getcol(3), E3DCOLOR(255, 255, 255, alpha));
    if (draw_flags & TRAJECTORY_FLAGS_DRAW_X)
      draw_cached_debug_line(trajectory[i].col[3], trajectory[i].col[3] + trajectory[i].col[0] * axis_length,
        E3DCOLOR(255, 0, 0, alpha));
    if (draw_flags & TRAJECTORY_FLAGS_DRAW_Y)
      draw_cached_debug_line(trajectory[i].col[3], trajectory[i].col[3] + trajectory[i].col[1] * axis_length,
        E3DCOLOR(0, 255, 0, alpha));
    if (draw_flags & TRAJECTORY_FLAGS_DRAW_Z)
      draw_cached_debug_line(trajectory[i].col[3], trajectory[i].col[3] + trajectory[i].col[2] * axis_length,
        E3DCOLOR(0, 0, 255, alpha));
  }
}

struct A2dNodeStats
{
  int pos_cnt = -1;
  int pos_max_keys = 0;
  int pos_total_keys = 0;
  int rot_cnt = -1;
  int rot_max_keys = 0;
  int rot_total_keys = 0;
  int scl_cnt = -1;
  int scl_max_keys = 0;
  int scl_total_keys = 0;
};

static A2dNodeStats getNodesParamsFromA2d(AnimV20::AnimData &a2d)
{
  using namespace AnimV20;

  A2dNodeStats s;

  AnimDataChan *adcPos = a2d.dumpData.getChanPoint3(CHTYPE_POSITION);
  AnimDataChan *adcRot = a2d.dumpData.getChanQuat(CHTYPE_ROTATION);
  AnimDataChan *adcScl = a2d.dumpData.getChanPoint3(CHTYPE_SCALE);

  if (adcPos)
  {
    s.pos_cnt = adcPos->nodeNum;
    auto n = adcPos->getNumKeys();
    if (s.pos_max_keys < n)
      s.pos_max_keys = n;
    s.pos_total_keys += n * adcPos->nodeNum;
  }
  if (adcRot)
  {
    s.rot_cnt = adcRot->nodeNum;
    auto n = adcRot->getNumKeys();
    if (s.rot_max_keys < n)
      s.rot_max_keys = n;
    s.rot_total_keys += n * adcRot->nodeNum;
  }
  if (adcScl)
  {
    s.scl_cnt = adcScl->nodeNum;
    auto n = adcScl->getNumKeys();
    if (s.scl_max_keys < n)
      s.scl_max_keys = n;
    s.scl_total_keys += n * adcScl->nodeNum;
  }

  return s;
}

static eastl::map<eastl::string, int> getNodesNamesFromA2d(AnimV20::AnimData &a2d)
{
  using namespace AnimV20;

  AnimDataChan *adcPos = a2d.dumpData.getChanPoint3(CHTYPE_POSITION);
  AnimDataChan *adcRot = a2d.dumpData.getChanQuat(CHTYPE_ROTATION);
  AnimDataChan *adcScl = a2d.dumpData.getChanPoint3(CHTYPE_SCALE);

  A2dNodeStats s = getNodesParamsFromA2d(a2d);

  eastl::map<eastl::string, int> nodeNames{};
  for (int i = 0; i < s.pos_cnt; ++i)
  {
    eastl::string name = adcPos->nodeName[i].get();
    nodeNames[name] = ANIM_NODE_POS;
  }
  for (int i = 0; i < s.rot_cnt; ++i)
  {
    eastl::string name = adcRot->nodeName[i].get();
    nodeNames[name] = nodeNames[name] | ANIM_NODE_ROT;
  }
  for (int i = 0; i < s.scl_cnt; ++i)
  {
    eastl::string name = adcScl->nodeName[i].get();
    nodeNames[name] = nodeNames[name] | ANIM_NODE_SCL;
  }
  return nodeNames;
}

A2dPlugin::RefDynmodel::RefDynmodel(const char *ref_name) :
  refTypeName{ref_name},
  refTypeId{get_app().getAssetMgr().getAssetTypeId(ref_name)},
  modelName{""},
  entity{nullptr},
  origTree{nullptr},
  ctrl{nullptr}
{
  G_ASSERTF(refTypeId >= 0, "Unknown reference type %s", ref_name);
}

A2dPlugin::A2dPlugin() :
  self{this},
  asset{nullptr},
  a2d{nullptr},
  ref{"dynModel"},
  currAnimProgress{-1.f},
  animTimeMul{1.f},
  animTimeMin{0},
  animTimeMax{0},
  selectedNodeTrajectoryTimeMin{0},
  selectedNodeTrajectoryTimeMax{0},
  selectedNodeTrajectoryTimeStep{1},
  selectedNodeTrajectoryAlpha{128},
  selectedNodeTrajectoryDrawFlags{0},
  selectedNodeTrajectoryAxisSize{0.01f},
  loopedPlay{false},
  drawSkeleton{false},
  drawA2dnodes{false},
  disableAdditivity{false},
  drawSelectedNodeTrajectory{false},
  drawSelectedNodeTrajectoryIsWorld{true}
{
  initScriptPanelEditor("a2d.scheme.nut", "a2d by scheme");
}

A2dPlugin::~A2dPlugin()
{
  end();
  ref.reset();
}

bool A2dPlugin::begin(DagorAsset *srcAsset)
{
  disableAdditivity = false;
  if (spEditor && srcAsset)
    spEditor->load(srcAsset);
  if (ref.modelName.empty())
    ref.modelName = a2d_last_ref_model;
  asset = srcAsset;
  a2d = (AnimV20::AnimData *)::get_game_resource_ex(asset->getName(), Anim2DataGameResClassId);
  ref.reload(ref.modelName, asset);
  if (!a2d)
    return false;
  setupAnimNodesForRefSkeleton();
  if (!a2d->dumpData.noteTrack.empty())
    setPoseFromTrackLabel(0);

  animTimeMin = 0;
  animTimeMax = 0;
  int lastTrackIdx = a2d->dumpData.noteTrack.size() - 1;
  for (const auto &track : a2d->dumpData.noteTrack)
    animTimeMax = track.time > animTimeMax ? track.time : animTimeMax;
  currAnimProgress = loopedPlay ? 0.f : -1.f;
  selectedNode = -1;
  animTimeMul = safediv(1.0f, animTimeMax / (float)AnimV20::TIME_TicksPerSec);

  // On plugin init these values are zeroed so we can check for zeroes here to set sane initial values.
  // Beging is also called on asset reload and we dont want to reload values in this case, just clamp if out of range.
  if (selectedNodeTrajectoryTimeMin == 0)
    selectedNodeTrajectoryTimeMin = animTimeMin;
  if (selectedNodeTrajectoryTimeMax == 0)
    selectedNodeTrajectoryTimeMax = animTimeMax;

  selectedNodeTrajectoryTimeMin = clamp(selectedNodeTrajectoryTimeMin, animTimeMin, animTimeMax);
  selectedNodeTrajectoryTimeMax = clamp(selectedNodeTrajectoryTimeMax, animTimeMin, animTimeMax);

  updateSelectedNodeTrajectory();

  return true;
}

bool A2dPlugin::end()
{
  if (spEditor)
    spEditor->destroyPanel();
  asset = nullptr;
  ::release_game_resource_ex(a2d, Anim2DataGameResClassId);
  a2d = nullptr;
  ref.reset();
  animNodes.clear();

  return true;
}

void A2dPlugin::processAnimProgress(float dt)
{
  bool inProgress = (currAnimProgress < 1.f && currAnimProgress >= 0.f) || loopedPlay;
  if (inProgress)
    if (loopedPlay)
      currAnimProgress = fmodf(currAnimProgress + dt * animTimeMul, 1.f);
    else
      currAnimProgress = currAnimProgress + dt * animTimeMul;
  else
    currAnimProgress = -1.f;
}

bool A2dPlugin::checkOrderOfTrackLabels() const
{
  int curr{0};
  for (const auto &track : a2d->dumpData.noteTrack)
    if (track.time < curr)
      return false;
    else
      curr = track.time;
  return true;
}

bool A2dPlugin::checkDuplicatedTrackLabels() const
{
  eastl::string_hash_map<const char *> tracks{};
  for (const auto &track : a2d->dumpData.noteTrack)
    tracks.insert(track.name.get());
  return tracks.size() == a2d->dumpData.noteTrack.size();
}

void A2dPlugin::RefDynmodel::reset()
{
  if (ctrl)
    ctrl->setSkeletonForRender(nullptr);
  destroy_it(entity);
  entity = nullptr;
  tree = GeomNodeTree();
  ctrl = nullptr;
  origTree = nullptr;
}
void A2dPlugin::RefDynmodel::reload(const SimpleString &model_name, DagorAsset *asset)
{
  reset();
  if (model_name.empty())
    return;

  modelName = model_name;
  DagorAsset *modelAsset = asset->getMgr().findAsset(modelName, refTypeId);
  if (!modelAsset)
  {
    getMainConsole().addMessage(ILogWriter::ERROR, "Loading ref model %s failed", modelName);
    modelName.clear();
    clear_and_shrink(a2d_last_ref_model);
    return;
  }
  entity = DAEDITOR3.createEntity(*modelAsset);
  entity->setSubtype(DAEDITOR3.registerEntitySubTypeId("single_ent"));

  ctrl = entity->queryInterface<ILodController>();
  if (!ctrl)
    return;
  ctrl->setCurLod(0);

  origTree = ctrl->getSkeleton();
  if (!origTree)
    return;
  tree = *origTree;
  ctrl->setSkeletonForRender(&tree);
}

void A2dPlugin::setupAnimNodesForRefSkeleton()
{
  animNodes.clear();
  if (!a2d)
    return;

  eastl::map<eastl::string, int> nodeNames = getNodesNamesFromA2d(*a2d);
  animNodes.reserve(nodeNames.size());
  for (const auto &pair : nodeNames)
  {
    const char *nodeName = pair.first.c_str();
    SimpleNodeAnim anim;
    if (!anim.init(a2d, nodeName))
      continue;
    AnimNode &animNode = animNodes.push_back();
    animNode.name = nodeName;
    animNode.skelIdx = ref.ready() ? ref.tree.findNodeIndex(nodeName) : dag::Index16();
    animNode.anim = anim;
    animNode.prsMask = pair.second;
  }
}

static void blend_add_anim(mat44f &dest_tm, const mat44f &base_tm, const TMatrix &add_tm_s)
{
  vec4f p0, r0, s0;
  vec4f p1, r1, s1;
  mat44f add_tm;
  add_tm.col0 = v_and(v_ldu(add_tm_s.m[0]), V_CI_MASK1110);
  add_tm.col1 = v_and(v_ldu(add_tm_s.m[1]), V_CI_MASK1110);
  add_tm.col2 = v_and(v_ldu(add_tm_s.m[2]), V_CI_MASK1110);
  add_tm.col3 = v_perm_xyzd(v_ldu(add_tm_s.m[3]), V_C_ONE);

  v_mat4_decompose(base_tm, p0, r0, s0);
  v_mat4_decompose(add_tm, p1, r1, s1);
  v_mat44_compose(dest_tm, v_add(p0, p1), v_quat_mul_quat(r0, r1), v_mul(s0, s1));
}

void A2dPlugin::setPoseFromTrackLabel(int note_track_num)
{
  if (!ref.ready() || !a2d)
    return;
  if (note_track_num < 0 || note_track_num >= a2d->dumpData.noteTrack.size())
  {
    DAEDITOR3.conNote("Requested note track num %d not found", note_track_num);
    return;
  }
  int a2dTime = a2d->dumpData.noteTrack[note_track_num].time;
  updateAnimNodes(a2dTime);
}

void A2dPlugin::updateSelectedNodeTrajectory()
{
  selectedNodeTrajectory.clear();

  if (!drawSelectedNodeTrajectory || selectedNode == -1)
    return;

  if (!drawSelectedNodeTrajectoryIsWorld)
    for (int a2d_time = selectedNodeTrajectoryTimeMin; a2d_time < selectedNodeTrajectoryTimeMax;
         a2d_time += selectedNodeTrajectoryTimeStep)
    {
      TMatrix tm;
      animNodes[selectedNode].anim.calcAnimTm(tm, a2d_time);

      selectedNodeTrajectory.push_back(tm);
    }
  else
  {
    //
    // NOTE: It does not seem like performance is bad at this point (even though when scrubbing time
    // limits on long animations small fps drop is noticable). If it becomes a problem at any point
    // there are easy things that can be improved here:
    // - When draw range is lowered - just throw away stored trajectory points. No need to recalc from
    //   trajectory at all.
    // - When draw range is increased - only recalculate the delta and not the whole trajectory.
    //
    // And we probably should avoid overwriting reference tree alltogether.
    //
    if (!animNodes[selectedNode].skelIdx)
      return;

    // Collect animNodes we need to update from animation to get correct WTM for selected node.
    dag::Vector<AnimNode *> animNodesToUpdate;
    for (dag::Index16 skeletonNodeIndex = animNodes[selectedNode].skelIdx; skeletonNodeIndex;
         skeletonNodeIndex = ref.tree.getParentNodeIdx(skeletonNodeIndex))
    {
      auto it = eastl::find_if(animNodes.begin(), animNodes.end(),
        [skeletonNodeIndex](const A2dPlugin::AnimNode &node) { return node.skelIdx == skeletonNodeIndex; });
      if (it == animNodes.end())
        break;
      animNodesToUpdate.push_back(it);
    }

    // For each frame we want to draw calculate final node WTM and store it for rendering.
    // We use ref tree to store this data so we need to be sure we restore it afterwards to playing
    // state.
    for (int a2d_time = selectedNodeTrajectoryTimeMin; a2d_time < selectedNodeTrajectoryTimeMax;
         a2d_time += selectedNodeTrajectoryTimeStep)
    {
      for (AnimNode *animNode : animNodesToUpdate)
        animNode->anim.calcAnimTm(animNode->tm, a2d_time);
      for (const AnimNode *animNode : animNodesToUpdate)
        ref.updateTreeNodeFromAnimNode(*animNode, a2d->isAdditive());
      ref.tree.invalidateWtm();
      ref.tree.partialCalcWtm(animNodes[selectedNode].skelIdx);

      TMatrix tm;
      ref.tree.getNodeWtmScalar(animNodes[selectedNode].skelIdx, tm);

      selectedNodeTrajectory.push_back(tm);
    }

    // Restore reference tree to state before updateSelectedNodeTrajectory was called.
    auto panel = getPluginPanel();
    if (panel)
      updateAnimNodes(lerp(animTimeMin, animTimeMax, saturate(panel->getFloat(PID_PLAY_PROGR))));
  }
}

void A2dPlugin::updateAnimNodes(int a2d_time)
{
  if (!a2d)
    return;
  for (AnimNode &animNode : animNodes)
    animNode.anim.calcAnimTm(animNode.tm, a2d_time);
  ref.updateNodeTree(animNodes, a2d->isAdditive());
}

void A2dPlugin::RefDynmodel::updateTreeNodeFromAnimNode(const AnimNode &animNode, bool additive)
{
  if (!animNode.skelIdx)
    return;
  TMatrix tm = animNode.tm;
  mat44f &node_tm = tree.getNodeTm(animNode.skelIdx);
  if (additive)
  {
    blend_add_anim(node_tm, origTree->getNodeTm(animNode.skelIdx), tm);
    return;
  }
  if (lengthSq(tm.getcol(3)) < 1e-4)
    tm.setcol(3, as_point3(&node_tm.col3));
  tree.setNodeTmScalar(animNode.skelIdx, tm);
}

void A2dPlugin::RefDynmodel::updateNodeTree(const dag::Vector<AnimNode> &animNodes, bool additive)
{
  if (!ready())
    return;

  for (const AnimNode &animNode : animNodes)
    updateTreeNodeFromAnimNode(animNode, additive);

  tree.invalidateWtm();
  tree.calcWtm();
  entity->setTm(TMatrix::IDENT);
}

void A2dPlugin::resetAnimProgress(float defined_progress)
{
  if (currAnimProgress > 0.f)
    currAnimProgress = -1.f;
  else
    currAnimProgress = defined_progress < 1.f ? defined_progress : 0.f;
}

bool A2dPlugin::supportAssetType(const DagorAsset &asset) const { return strcmp(asset.getTypeStr(), "a2d") == 0; }

void A2dPlugin::selectAnimNode(const Point3 &p, const Point3 &dir)
{
  selectedNode = -1;
  if (!ref.ready())
  {
    updateSelectedNodeTrajectory();
    return;
  }

  const Point3 zero3{0.f, 0.f, 0.f};

  BBox3 bbox;
  bbox.setempty();
  ref.tree.calcWorldBox(bbox);
  float radius = getMinPoint(bbox.width(), 0.5f) * 0.05f;

  for (int i = 0; i < animNodes.size(); ++i)
  {
    Capsule capsule(zero3, zero3, radius);
    if (animNodes[i].skelIdx)
      capsule.transform(ref.tree.getNodeWtmRel(animNodes[i].skelIdx));
    else
      capsule.transform(animNodes[i].tm);
    float maxt = 1000.f;
    if (capsule.rayHit(p, dir, maxt))
    {
      selectedNode = i;
      updateSelectedNodeTrajectory();
      break;
    }
  }
}

bool A2dPlugin::handleMouseLBPress(IGenViewportWnd *wnd, int x, int y, bool inside, int buttons, int key_modif)
{
  if (!inside || !ref.ready())
    return false;

  Point3 dir, world;
  wnd->clientToWorld(Point2(x, y), world, dir);

  selectAnimNode(world, dir);
  return selectedNode >= 0;
}

void A2dPlugin::onClick(int pcb_id, PropPanel::ContainerPropertyControl *panel)
{
  if (!getPluginPanel())
    return;

  if (pcb_id == PID_CHAR_NAME)
  {
    SelectAssetDlgOptions dlgOptions;
    dlgOptions.selectableAssetTypes.push_back(ref.refTypeId);
    dlgOptions.initiallySelectedAsset = ref.modelName;
    dlgOptions.dialogCaption.printf(64, "Select %s", ref.refTypeName);
    dlgOptions.selectButtonCaption.printf(64, "Select %s", ref.refTypeName);
    dlgOptions.resetButtonCaption.printf(64, "Reset %s", ref.refTypeName);
    dlgOptions.positionBesideWindow = "Properties";

    const eastl::optional<String> result = DAEDITOR3.selectAsset(dlgOptions);
    if (!result.has_value())
      return;

    ref.modelName = result.value();
    a2d_last_ref_model = ref.modelName;
    ref.reload(ref.modelName, asset);
    setupAnimNodesForRefSkeleton();
    panel->setCaption(PID_CHAR_NAME, ref.modelName.empty() ? "Select reference dynmodel" : ref.modelName);

    if (!a2d->dumpData.noteTrack.empty())
      setPoseFromTrackLabel(0);

    updateSelectedNodeTrajectory();
  }
  else if (pcb_id == PID_PLAY_PARAMS_RESET && ref.ready())
  {
    animTimeMul = safediv(1.0f, animTimeMax / (float)AnimV20::TIME_TicksPerSec);

    panel->setFloat(PID_PLAY_MUL, animTimeMul);
    panel->setFloat(PID_PLAY_DUR, safediv(1.f, animTimeMul));
    panel->setFloat(PID_PLAY_CUR_TIME, panel->getFloat(PID_PLAY_PROGR) * panel->getFloat(PID_PLAY_DUR));
  }
  else if (pcb_id == PID_PLAY_ANIM && ref.ready())
  {
    loopedPlay = false;
    resetAnimProgress(getPluginPanel()->getFloat(PID_PLAY_PROGR));
  }
  else if (pcb_id == PID_PLAY_LOOP && ref.ready())
  {
    loopedPlay = !loopedPlay;
    currAnimProgress = loopedPlay ? panel->getFloat(PID_PLAY_PROGR) : -1.f;
  }
  else if (pcb_id == PID_SELECTED_NODE_TRAJECTORY_SET_MIN_FROM_CURRENT && ref.ready())
  {
    selectedNodeTrajectoryTimeMin = lerp(animTimeMin, animTimeMax, saturate(panel->getFloat(PID_PLAY_PROGR)));
    panel->setInt(PID_SELECTED_NODE_TRAJECTORY_TIME_MIN, selectedNodeTrajectoryTimeMin);
    updateSelectedNodeTrajectory();
  }
  else if (pcb_id == PID_SELECTED_NODE_TRAJECTORY_SET_MAX_FROM_CURRENT && ref.ready())
  {
    selectedNodeTrajectoryTimeMax = lerp(animTimeMin, animTimeMax, saturate(panel->getFloat(PID_PLAY_PROGR)));
    panel->setInt(PID_SELECTED_NODE_TRAJECTORY_TIME_MAX, selectedNodeTrajectoryTimeMax);
    updateSelectedNodeTrajectory();
  }
  else if ((pcb_id == PID_SELECTED_NODE_TM_COPY || pcb_id == PID_SELECTED_NODE_WTM_COPY) && ref.ready() && selectedNode >= 0 &&
           selectedNode < animNodes.size() && animNodes[selectedNode].skelIdx)
  {
    TMatrix tm = animNodes[selectedNode].tm;
    if (pcb_id == PID_SELECTED_NODE_WTM_COPY)
      ref.tree.getNodeWtmScalar(animNodes[selectedNode].skelIdx, tm);

    DataBlock nblk;
    nblk.setTm("m", tm);
    clipboard::set_clipboard_utf8_text(blk_util::blkTextData(nblk).c_str());
  }
  else if (pcb_id >= PID_ANIM_NODES_GROUP && pcb_id <= PID_ANIM_NODES_GROUP_LAST)
  {
    size_t index = pcb_id - PID_ANIM_NODES_GROUP;
    if (index < animNodes.size())
    {
      selectedNode = index;
      updateSelectedNodeTrajectory();
    }
  }
}

void A2dPlugin::actObjects(float dt)
{
  processAnimProgress(dt);
  auto panel = getPluginPanel();
  if (ref.ready() && isAnimInProgress())
  {
    int animTime = lerp(animTimeMin, animTimeMax, currAnimProgress);
    updateAnimNodes(animTime);
    if (panel)
    {
      panel->setFloat(PID_PLAY_PROGR, currAnimProgress);
      panel->setFloat(PID_PLAY_CUR_TIME, currAnimProgress * panel->getFloat(PID_PLAY_DUR));
    }
  }

  if (panel && selectedNode >= 0 && selectedNode < animNodes.size() && animNodes[selectedNode].skelIdx)
  {
    TMatrix &tm = animNodes[selectedNode].tm;
    Point3 tmScale = Point3(length(tm.getcol(0)), length(tm.getcol(1)), length(tm.getcol(2)));
    float tmDet = tm.det();
    if (tmDet < 0.0)
      tmScale.z *= -1.0f;
    Point3 tmAng;
    matrix_to_euler(tm, tmAng.x, tmAng.y, tmAng.z);

    TMatrix wtm;
    ref.tree.getNodeWtmScalar(animNodes[selectedNode].skelIdx, wtm);
    Point3 wtmScale = Point3(length(wtm.getcol(0)), length(wtm.getcol(1)), length(wtm.getcol(2)));
    float wtmDet = wtm.det();
    if (wtmDet < 0.0)
      wtmScale.z *= -1.0f;
    Point3 wtmAng;
    matrix_to_euler(tm, wtmAng.x, wtmAng.y, wtmAng.z);

    panel->setPoint3(PID_SELECTED_NODE_TM_POS, tm.getcol(3));
    panel->setPoint3(PID_SELECTED_NODE_TM_ROT, tmAng);
    panel->setPoint3(PID_SELECTED_NODE_TM_SCALE, tmScale);

    panel->setPoint3(PID_SELECTED_NODE_WTM_POS, wtm.getcol(3));
    panel->setPoint3(PID_SELECTED_NODE_WTM_ROT, wtmAng);
    panel->setPoint3(PID_SELECTED_NODE_WTM_SCALE, wtmScale);
  }
}

static void asset_notify_changed(DagorAsset *asset)
{
  get_app().getAssetMgr().callAssetBaseChangeNotifications(dag::ConstSpan<DagorAsset *>((DagorAsset *const *)&asset, 1),
    dag::ConstSpan<DagorAsset *>(), dag::ConstSpan<DagorAsset *>());
  get_app().getAssetMgr().callAssetChangeNotifications(*asset, asset->getNameId(), asset->getType());
}

static void asset_temp_disable_additivity_and_notify_changed(DagorAsset *asset)
{
  asset->props.setBool("makeAdditive", false);
  asset_notify_changed(asset);
  asset->props.setBool("makeAdditive", true);
}

void A2dPlugin::onChange(int pid, PropPanel::ContainerPropertyControl *panel)
{
  switch (pid)
  {
    case PID_KEY_LABELS_RADIO:
      panel->setInt(PID_KEY_TRACKBAR, panel->getInt(pid));

      if (ref.ready() && !isAnimInProgress())
      {
        int noteTrackNum = panel->getInt(PID_KEY_TRACKBAR);
        if (noteTrackNum >= 0 && noteTrackNum < a2d->dumpData.noteTrack.size())
        {
          const float progress =
            safediv(float(a2d->dumpData.noteTrack[noteTrackNum].time - animTimeMin), float(animTimeMax - animTimeMin));
          panel->setFloat(PID_PLAY_PROGR, progress);
          panel->setFloat(PID_PLAY_CUR_TIME, panel->getFloat(PID_PLAY_PROGR) * panel->getFloat(PID_PLAY_DUR));
        }
        setPoseFromTrackLabel(noteTrackNum);
      }
      break;
    case PID_KEY_TRACKBAR:
      if (ref.ready() && !isAnimInProgress())
        setPoseFromTrackLabel(panel->getInt(PID_KEY_TRACKBAR));
      break;
    case PID_PLAY_DUR:
      animTimeMul = safediv(1.0f, panel->getFloat(PID_PLAY_DUR));
      panel->setFloat(PID_PLAY_MUL, animTimeMul);
      panel->setFloat(PID_PLAY_CUR_TIME, panel->getFloat(PID_PLAY_PROGR) * panel->getFloat(PID_PLAY_DUR));
      break;
    case PID_PLAY_MUL:
      animTimeMul = panel->getFloat(PID_PLAY_MUL);
      panel->setFloat(PID_PLAY_DUR, safediv(1.f, animTimeMul));
      panel->setFloat(PID_PLAY_CUR_TIME, panel->getFloat(PID_PLAY_PROGR) * panel->getFloat(PID_PLAY_DUR));
      break;
    case PID_PLAY_PROGR:
      currAnimProgress = -1.f;
      loopedPlay = false;
      updateAnimNodes(lerp(animTimeMin, animTimeMax, saturate(panel->getFloat(PID_PLAY_PROGR))));
      panel->setFloat(PID_PLAY_CUR_TIME, panel->getFloat(PID_PLAY_PROGR) * panel->getFloat(PID_PLAY_DUR));
      break;
    case PID_SHOW_SKEL: drawSkeleton = panel->getBool(PID_SHOW_SKEL); break;
    case PID_SHOW_INVIS: drawA2dnodes = panel->getBool(PID_SHOW_INVIS); break;
    case PID_SHOW_SELECTED_NODE_TRAJECTORY:
      drawSelectedNodeTrajectory = panel->getBool(PID_SHOW_SELECTED_NODE_TRAJECTORY);
      updateSelectedNodeTrajectory();
      break;
    case PID_SELECTED_NODE_TRAJECTORY_IS_WORLD:
      drawSelectedNodeTrajectoryIsWorld = panel->getBool(PID_SELECTED_NODE_TRAJECTORY_IS_WORLD);
      updateSelectedNodeTrajectory();
      break;
    case PID_SELECTED_NODE_TRAJECTORY_TIME_STEP:
      selectedNodeTrajectoryTimeStep = max(1, panel->getInt(PID_SELECTED_NODE_TRAJECTORY_TIME_STEP));
      updateSelectedNodeTrajectory();
      break;
    case PID_SELECTED_NODE_TRAJECTORY_TIME_MIN:
      selectedNodeTrajectoryTimeMin = min(animTimeMax, max(animTimeMin, panel->getInt(PID_SELECTED_NODE_TRAJECTORY_TIME_MIN)));
      updateSelectedNodeTrajectory();
      break;
    case PID_SELECTED_NODE_TRAJECTORY_TIME_MAX:
      selectedNodeTrajectoryTimeMax = min(animTimeMax, max(animTimeMin, panel->getInt(PID_SELECTED_NODE_TRAJECTORY_TIME_MAX)));
      updateSelectedNodeTrajectory();
      break;
    case PID_SELECTED_NODE_TRAJECTORY_DRAW_ALPHA:
      selectedNodeTrajectoryAlpha = min(255, max(0, panel->getInt(PID_SELECTED_NODE_TRAJECTORY_DRAW_ALPHA)));
      break;
    case PID_SELECTED_NODE_TRAJECTORY_DRAW_AXIS_SIZE:
      selectedNodeTrajectoryAxisSize = panel->getFloat(PID_SELECTED_NODE_TRAJECTORY_DRAW_AXIS_SIZE);
      break;
    case PID_SELECTED_NODE_TRAJECTORY_DRAW_AXIS_X:
      if (panel->getBool(PID_SELECTED_NODE_TRAJECTORY_DRAW_AXIS_X))
        selectedNodeTrajectoryDrawFlags |= TRAJECTORY_FLAGS_DRAW_X;
      else
        selectedNodeTrajectoryDrawFlags &= ~TRAJECTORY_FLAGS_DRAW_X;
      break;
    case PID_SELECTED_NODE_TRAJECTORY_DRAW_AXIS_Y:
      if (panel->getBool(PID_SELECTED_NODE_TRAJECTORY_DRAW_AXIS_Y))
        selectedNodeTrajectoryDrawFlags |= TRAJECTORY_FLAGS_DRAW_Y;
      else
        selectedNodeTrajectoryDrawFlags &= ~TRAJECTORY_FLAGS_DRAW_Y;
      break;
    case PID_SELECTED_NODE_TRAJECTORY_DRAW_AXIS_Z:
      if (panel->getBool(PID_SELECTED_NODE_TRAJECTORY_DRAW_AXIS_Z))
        selectedNodeTrajectoryDrawFlags |= TRAJECTORY_FLAGS_DRAW_Z;
      else
        selectedNodeTrajectoryDrawFlags &= ~TRAJECTORY_FLAGS_DRAW_Z;
      break;
    case PID_DISABLE_ADDITIVITY:
      disableAdditivity = panel->getBool(PID_DISABLE_ADDITIVITY);
      if (disableAdditivity)
      {
        if (a2d->isAdditive())
          add_delayed_callback((delayed_callback)&asset_temp_disable_additivity_and_notify_changed, asset);
      }
      else
        add_delayed_callback((delayed_callback)&asset_notify_changed, asset);
      break;
    default: break;
  }
}

String getFileSizeText(long long bytes)
{
  const int num_names = 4;
  const char *names[num_names] = {"Bytes", "KiloBytes", "MegaBytes", "GigaBytes"};

  long long bytesRounded = bytes;
  long long pow = 1;
  for (int i = 0; i < num_names; i++)
  {
    if (bytesRounded < 1024l)
      return String(-1, "%d.%d %s", bytesRounded, bytes - bytesRounded * pow, names[i]);
    bytesRounded /= 1024l;
    pow *= 1024l;
  }

  return String(-1, "%d.%d %s", bytesRounded, bytes - bytesRounded * pow, names[num_names - 1]);
}

void A2dPlugin::fillPropPanel(PropPanel::ContainerPropertyControl &panel)
{
  using namespace AnimV20;

  panel.clear();

  if (!asset || !a2d)
  {
    panel.createStatic(-1, String(64, "Error while loading the asset or animation."));
    return;
  }

  panel.setEventHandler(this);
  panel.createIndent();

  PropPanel::ContainerPropertyControl &propGrp = *panel.createGroup(PID_PROP_GRP, "A2d properties");
  PropPanel::ContainerPropertyControl &playGrp = *panel.createGroup(PID_PLAY_GRP, "A2d preview");
  PropPanel::ContainerPropertyControl &trajectoryGrp =
    *panel.createGroup(PID_SELECTED_NODE_TRAJECTORY_GROUP, "Selected node trajectory");
  PropPanel::ContainerPropertyControl &nodeInfoGrp = *panel.createGroup(PID_SELECTED_NODE_INFO_GROUP, "Selected node info");
  PropPanel::ContainerPropertyControl &labelsGrp = *panel.createGroup(PID_KEY_LABELS_GROUP, "Note tracks");
  PropPanel::ContainerPropertyControl &nodesGrp = *panel.createGroup(PID_ANIM_NODES_GROUP, "Anim nodes");

  propGrp.createStatic(-1, String(64, "Anim size: %s", getFileSizeText(a2d->getA2dDataSize())));
  propGrp.createStatic(-1, String(64, "Is additive: %s", a2d->isAdditive() ? "yes" : "no"));
  propGrp.createCheckBox(PID_DISABLE_ADDITIVITY, "Disable additivity", disableAdditivity);
  propGrp.createStatic(-1, String(64, "Note tracks: %d", a2d->dumpData.noteTrack.size()));
  playGrp.createSeparator();
  constexpr float maxAnimMult = 3.f;
  playGrp.createTrackFloat(PID_PLAY_DUR, "Anim duration: ", safediv(1.0f, animTimeMul), safediv(1.0f, maxAnimMult), 60.0f, 0.01f);
  playGrp.createTrackFloat(PID_PLAY_MUL, "Anim multiplier: ", animTimeMul, 0.f, maxAnimMult, 0.01, 1);

  if (a2d->dumpData.noteTrack.size() > 0)
  {
    int tracksCnt = a2d->dumpData.noteTrack.size() - 1;
    String caption = String(64, "Show note track (0-%d):", tracksCnt);
    labelsGrp.createTrackInt(PID_KEY_TRACKBAR, caption, 0, 0, tracksCnt, 1, 1);
  }
  playGrp.createTrackFloat(PID_PLAY_PROGR, "Anim progress: ", 0.f, 0.f, 1.f, 0.01, 1);
  playGrp.createEditFloat(PID_PLAY_CUR_TIME, "Seconds: ", 0.0f, 2, false);
  playGrp.createButton(PID_PLAY_PARAMS_RESET, "Reset play params");
  playGrp.createCheckBox(PID_SHOW_SKEL, "Show skeleton nodes", drawSkeleton);
  playGrp.createCheckBox(PID_SHOW_INVIS, "Show exported nodes", drawA2dnodes);
  playGrp.createSeparator();
  playGrp.createButton(PID_PLAY_ANIM, "Play single anim");
  playGrp.createButton(PID_PLAY_LOOP, "Play looped anim");
  playGrp.setButtonPictures(PID_PLAY_ANIM, "play");
  playGrp.createButton(PID_CHAR_NAME, ref.modelName.empty() ? "Select reference dynmodel" : ref.modelName);

  trajectoryGrp.createCheckBox(PID_SHOW_SELECTED_NODE_TRAJECTORY, "Show selected node trajectory", drawSelectedNodeTrajectory);
  trajectoryGrp.createCheckBox(PID_SELECTED_NODE_TRAJECTORY_IS_WORLD, "World coordiantes", drawSelectedNodeTrajectoryIsWorld);
  trajectoryGrp.createTrackInt(PID_SELECTED_NODE_TRAJECTORY_TIME_STEP, "Time step", selectedNodeTrajectoryTimeStep, 1, 1024, 1);
  trajectoryGrp.createTrackInt(PID_SELECTED_NODE_TRAJECTORY_TIME_MIN, "Start tick", selectedNodeTrajectoryTimeMin, animTimeMin,
    animTimeMax, 1);
  trajectoryGrp.createButton(PID_SELECTED_NODE_TRAJECTORY_SET_MIN_FROM_CURRENT, "Set start tick from current anim progress");
  trajectoryGrp.createTrackInt(PID_SELECTED_NODE_TRAJECTORY_TIME_MAX, "End tick", selectedNodeTrajectoryTimeMax, animTimeMin,
    animTimeMax, 1);
  trajectoryGrp.createButton(PID_SELECTED_NODE_TRAJECTORY_SET_MAX_FROM_CURRENT, "Set end tick from current anim progress");
  trajectoryGrp.createSeparator();
  trajectoryGrp.createTrackInt(PID_SELECTED_NODE_TRAJECTORY_DRAW_ALPHA, "Color alpha", selectedNodeTrajectoryAlpha, 0, 255, 1);
  trajectoryGrp.createTrackFloat(PID_SELECTED_NODE_TRAJECTORY_DRAW_AXIS_SIZE, "Axis length", selectedNodeTrajectoryAxisSize, 0.00001f,
    1.0f, 0.01f);
  trajectoryGrp.createCheckBox(PID_SELECTED_NODE_TRAJECTORY_DRAW_AXIS_X, "Axis X",
    selectedNodeTrajectoryDrawFlags & TRAJECTORY_FLAGS_DRAW_X);
  trajectoryGrp.createCheckBox(PID_SELECTED_NODE_TRAJECTORY_DRAW_AXIS_Y, "Axis Y",
    selectedNodeTrajectoryDrawFlags & TRAJECTORY_FLAGS_DRAW_Y);
  trajectoryGrp.createCheckBox(PID_SELECTED_NODE_TRAJECTORY_DRAW_AXIS_Z, "Axis Z",
    selectedNodeTrajectoryDrawFlags & TRAJECTORY_FLAGS_DRAW_Z);

  nodeInfoGrp.createPoint3(PID_SELECTED_NODE_TM_POS, "TM Position", Point3(), 2, false);
  nodeInfoGrp.createPoint3(PID_SELECTED_NODE_TM_ROT, "TM Rotation", Point3(), 2, false);
  nodeInfoGrp.createPoint3(PID_SELECTED_NODE_TM_SCALE, "TM Scale", Point3(), 2, false);
  nodeInfoGrp.createButton(PID_SELECTED_NODE_TM_COPY, "Copy TM string to clipboard");
  nodeInfoGrp.createPoint3(PID_SELECTED_NODE_WTM_POS, "WTM Position", Point3(), 2, false);
  nodeInfoGrp.createPoint3(PID_SELECTED_NODE_WTM_ROT, "WTM Rotation", Point3(), 2, false);
  nodeInfoGrp.createPoint3(PID_SELECTED_NODE_WTM_SCALE, "WTM Scale", Point3(), 2, false);
  nodeInfoGrp.createButton(PID_SELECTED_NODE_WTM_COPY, "Copy WTM string to clipboard");

  if (!checkOrderOfTrackLabels())
    labelsGrp.createStatic(-1, String(64, "Warning: incorrect label order"));
  if (!checkDuplicatedTrackLabels())
    labelsGrp.createStatic(-1, String(64, "Warning: duplicated labels"));
  if (a2d->dumpData.noteTrack.empty())
    labelsGrp.createStatic(-1, String(64, "Warning: note labels empty"));

  PropPanel::ContainerPropertyControl &ntGrp = *labelsGrp.createRadioGroup(PID_KEY_LABELS_RADIO, "");
  for (int i = 0; i < a2d->dumpData.noteTrack.size(); ++i)
  {
    static const int TICKS_PER_FRAME = TIME_TicksPerSec / 30;
    const float progress = safediv(float(a2d->dumpData.noteTrack[i].time - animTimeMin), float(animTimeMax - animTimeMin));
    ntGrp.createRadio(i, String(64, "\"%s\"  frame #%d (tick=%d, progress=%.3f)", a2d->dumpData.noteTrack[i].name,
                           a2d->dumpData.noteTrack[i].time / TICKS_PER_FRAME, a2d->dumpData.noteTrack[i].time, progress));
  }
  panel.setInt(PID_KEY_LABELS_RADIO, 0);

  A2dNodeStats stats = getNodesParamsFromA2d(*a2d);
  propGrp.createStatic(-1, String(64, "Anim nodes: P %d, R %d, S %d", stats.pos_cnt, stats.rot_cnt, stats.scl_cnt));
  propGrp.createStatic(-1,
    String(64, "Max keys per chan: P %d, R %d, S %d", stats.pos_max_keys, stats.rot_max_keys, stats.scl_max_keys));
  propGrp.createStatic(-1,
    String(64, "Avg keys per chan: P %d, R %d, S %d", stats.pos_cnt != 0 ? stats.pos_total_keys / stats.pos_cnt : 0,
      stats.rot_cnt != 0 ? stats.rot_total_keys / stats.rot_cnt : 0, stats.scl_cnt != 0 ? stats.scl_total_keys / stats.scl_cnt : 0));
  propGrp.createStatic(-1,
    String(64, "Total keys count: P %d, R %d, S %d", stats.pos_total_keys, stats.rot_total_keys, stats.scl_total_keys));
  propGrp.createIndent();

  int i{0};
  for (const AnimNode &node : animNodes)
  {
    int prs = node.prsMask;
    String prsStr(8, "(%c%c%c)", prs & ANIM_NODE_POS ? 'P' : '_', prs & ANIM_NODE_ROT ? 'R' : '_', prs & ANIM_NODE_SCL ? 'S' : '_');
    nodesGrp.createButtonLText(PID_ANIM_NODES_GROUP + i, String(64, "%s %s", prsStr, node.name.c_str()));
    ++i;
  }
}

void A2dPlugin::renderTransObjects()
{
  if (!ref.ready())
    return;

  begin_draw_cached_debug_lines(false, false);
  set_cached_debug_lines_wtm(TMatrix::IDENT);

  if (drawSkeleton && !ref.tree.empty())
    for (int i = 0; i < ref.tree.getChildCount(dag::Index16(0)); i++)
      drawSkeletonLinks(ref.tree, ref.tree.getChildNodeIdx(dag::Index16(0), i), *ref.ctrl);
  if (drawA2dnodes)
    drawExportedNodes(ref.tree, animNodes, selectedNode);
  if (drawSelectedNodeTrajectory && selectedNodeTrajectory.size() > 1)
    drawTrajectory(selectedNodeTrajectory, selectedNodeTrajectoryAxisSize, selectedNodeTrajectoryAlpha,
      selectedNodeTrajectoryDrawFlags);

  end_draw_cached_debug_lines();
}

void A2dPlugin::handleViewportPaint(IGenViewportWnd *wnd)
{
  if (selectedNode >= 0)
  {
    Point3 worldPos;
    if (animNodes[selectedNode].skelIdx)
      worldPos = ref.tree.getNodeWposRelScalar(animNodes[selectedNode].skelIdx);
    else
      worldPos = animNodes[selectedNode].tm.col[3];
    Point2 pos{};
    float z{};
    if (!wnd->worldToClient(worldPos, pos, &z) || (z < 0.001f))
      return;

    const char *name = animNodes[selectedNode].name;
    StdGuiRender::set_font(0);
    StdGuiRender::set_color(COLOR_BLACK);
    StdGuiRender::draw_strf_to(pos.x + 1, pos.y + 1, name);
    StdGuiRender::set_color(COLOR_LTGREEN);
    StdGuiRender::draw_strf_to(pos.x, pos.y, name);
  }
  drawInfo(wnd);
}
bool A2dPlugin::getSelectionBox(BBox3 &box) const
{
  if (!ref.entity)
    return false;
  box = ref.entity->getBbox();
  return true;
}
