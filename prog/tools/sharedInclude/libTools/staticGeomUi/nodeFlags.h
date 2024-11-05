// Copyright (C) Gaijin Games KFT.  All rights reserved.
#pragma once

#include <generic/dag_tab.h>

#include <util/dag_simpleString.h>

#include <libTools/staticGeom/staticGeometry.h>

namespace PropPanel
{
class ContainerPropertyControl;
}

class StaticGeometryContainer;
class StaticGeometryNode;


struct NodesData
{
  bool useDefault;

  struct NodeFlags
  {
    const char *name;
    const char *topLodName;
    const char *linkedRes;

    // flags
    int renderable;
    int collidable;
    int castShadows;
    int castShadowsOnSelf;
    int forceLocalNorm;
    int forceWorldNorm;
    int forceSphericalNorm;
    int fade;
    int fadeNull;
    int billboard;
    int occluder;
    int automaticVisRange;
    int bkFaceDynLight;
    int doNotMixLods;

    int light;  // node lighting. must be StaticGeometryNode::Lighting or -1
    real ltMul; // lightmap multiplier
    int vltMul; // vltmap multiplier

    Point3 forcedNormal;

    real visRange;
    real lodRange;

    int showOccluder;

    NodeFlags();
    NodeFlags(const StaticGeometryNode &node) { set(node); }

    void set(const StaticGeometryNode &node);
    // compare flags and set not equal parameters in 2 (for checks) or MAX_REAL (for reals)
    void compare(const NodeFlags &flags);
  };

  Tab<NodeFlags> flags;

  NodesData() : flags(tmpmem) { clear(); }

  void clear()
  {
    flags.clear();
    useDefault = true;
  }
};


class INodeModifierClient
{
public:
  virtual void onNodeFlagsChanged(int node_idx, int or_flags, int and_flags) = 0;
  virtual void onVisRangeChanged(int node_idx, real vis_range) = 0;
  virtual void onLightingChanged(int node_idx, StaticGeometryNode::Lighting light) = 0;
  virtual void onLtMulChanged(int node_idx, real lt_mul) = 0;
  virtual void onVltMulChanged(int node_idx, int vlt_mul) = 0;
  virtual void onLinkedResChanged(int node_idx, const char *res_name) = 0;
  virtual void onTopLodChanged(int node_idx, const char *top_lod_name) = 0;
  virtual void onLodRangeChanged(int node_idx, int lod_range) = 0;
  virtual void onShowOccludersChanged(int node_idx, bool show_occluders) = 0;
  virtual void onUseDefaultChanged(bool use_default) = 0;
};


struct NodeFlagsSettings
{
  bool showUseDefault;
  bool showAllNodesGrp;

  bool showVisRange;
  bool showAutomaticVisRange;
  bool showRenderable;
  bool showCollidable;
  bool showCastShadows;
  bool showCastShadowsOnSelf;
  bool showFade;
  bool showFadeNull;
  bool showBillboard;
  bool useOccluder;
  bool showOccluder;
  bool showBkFaceDynLight;
  bool showDoNotMixLods;

  bool showForceNormals;
  bool showForcedNormal;

  bool showLighting;
  bool showDefLightRadio;

  bool showLod;
  bool editLodRange;
  bool editLodName;

  bool showLinkedRes;

  bool disableLinkedRes;

  NodeFlagsSettings() :
    showUseDefault(true),
    showAllNodesGrp(true),
    showVisRange(true),
    showAutomaticVisRange(true),
    showRenderable(true),
    showCollidable(true),
    showCastShadows(true),
    showCastShadowsOnSelf(true),
    showFade(true),
    showFadeNull(true),
    showBillboard(true),
    useOccluder(true),
    showOccluder(true),
    showBkFaceDynLight(true),
    showForceNormals(true),
    showForcedNormal(true),
    showLighting(true),
    showLod(true),
    showLinkedRes(true),
    disableLinkedRes(true),
    showDefLightRadio(true),
    showDoNotMixLods(true),
    editLodRange(false),
    editLodName(false)
  {}
};

class NodeFlagsModfier
{
public:
  NodeFlagsModfier(INodeModifierClient &client, const char *uid);

  inline void setSettings(const NodeFlagsSettings &set) { settings = set; }

  // fills Property Panel
  void fillPanel(PropPanel::ContainerPropertyControl &panel, const NodesData &flags, int start_pid, bool create_nodes_grp = true);
  // return true if changing handled. usually it calls when edit_finished == true
  bool onPPChange(PropPanel::ContainerPropertyControl &panel, int pid);
  // return true if button press handled
  bool onPPBtnPressed(PropPanel::ContainerPropertyControl &panel, int pid);

  void clear();

  inline int getFirstPid() const { return nodesGrpPid; }
  inline int getLastPid() const { return lastNodesGrpPid; }

  inline int getUseDefPid() const { return nodesGrpPid + PID_USE_DEFAULT; }

  inline int getAllNodesGrpPid() const { return nodesGrpPid + PID_ALL_NODES_GRP; }
  inline int getAllVisrangePid() const { return nodesGrpPid + PID_ALL_VISRANGE; }
  inline int getAllRenderablePid() const { return nodesGrpPid + PID_ALL_FLG_RENDERABLE; }
  inline int getAllCollidablePid() const { return nodesGrpPid + PID_ALL_FLG_COLLIDABLE; }
  inline int getAllCastShadowsPid() const { return nodesGrpPid + PID_ALL_FLG_CASTSHADOWS; }
  inline int getAllCastShadowsOnSelfPid() const { return nodesGrpPid + PID_ALL_FLG_CASTSHADOWS_ON_SELF; }
  inline int getAllFadePid() const { return nodesGrpPid + PID_ALL_FLG_FADE; }
  inline int getAllFadeNullPid() const { return nodesGrpPid + PID_ALL_FLG_FADENULL; }
  inline int getAllBillboardPid() const { return nodesGrpPid + PID_ALL_FLG_BILLBOARD_MESH; }
  inline int getAllOccluderPid() const { return nodesGrpPid + PID_ALL_FLG_OCCLUDER; }
  inline int getAllShowOccluderPid() const { return nodesGrpPid + PID_ALL_FLG_SHOW_OCCLUDER; }
  inline int getAllAutoVisrangePid() const { return nodesGrpPid + PID_ALL_FLG_AUTOMATIC_VISRANGE; }
  inline int getAllBackFaceDynlightPid() const { return nodesGrpPid + PID_ALL_FLG_BACK_FACE_DYNLIGHT; }
  inline int getAllDoNotMixLods() const { return nodesGrpPid + PID_ALL_DO_NOT_MIX_LODS; }

  inline int getAllNormalsGrpPid() const { return nodesGrpPid + PID_ALL_NORMALS_GRP; }
  inline int getAllForceWorldPid() const { return nodesGrpPid + PID_ALL_FORCE_WORLD; }
  inline int getAllForceLocalPid() const { return nodesGrpPid + PID_ALL_FORCE_LOCAL; }
  inline int getAllForceSphericalPid() const { return nodesGrpPid + PID_ALL_FORCE_SPHERICAL; }
  inline int getAllForceNonePid() const { return nodesGrpPid + PID_ALL_FORCE_NONE; }
  inline int getAllForceNormalPid() const { return nodesGrpPid + PID_ALL_FORCE_NORMAL; }

  inline int getAllLightingGrpPid() const { return nodesGrpPid + PID_ALL_LIGHTING_GRP; }
  inline int getAllLightingRadioGrpPid() const { return nodesGrpPid + PID_ALL_LIGHTING_RADIO_GRP; }
  inline int getAllLtDefPid() const { return nodesGrpPid + PID_ALL_LT_DEF; }
  inline int getAllLtNonePid() const { return nodesGrpPid + PID_ALL_LT_NONE; }
  inline int getAllLtLightmampPid() const { return nodesGrpPid + PID_ALL_LT_LIGHTMAP; }
  inline int getAllLtLightmampMulPid() const { return nodesGrpPid + PID_ALL_LT_LIGHTMAP_MUL; }
  inline int getAllLtVltmapPid() const { return nodesGrpPid + PID_ALL_LT_VLTMAP; }
  inline int getAllLtVltmapMulPid() const { return nodesGrpPid + PID_ALL_LT_VLTMAP_MUL; }

  inline int getAllSetButtonPid() const { return nodesGrpPid + PID_SET_FOR_ALL_BTN; }

  inline int getNodePid(int idx, int pid) const { return nodesGrpPid + PID_NODE + PID_NODE_CNT * idx + pid; }

  inline int getNodeGrpPid(int idx) const { return getNodePid(idx, PID_NODE_GRP); }
  inline int getVisrangePid(int idx) const { return getNodePid(idx, PID_VISRANGE); }
  inline int getRenderablePid(int idx) const { return getNodePid(idx, PID_FLG_RENDERABLE); }
  inline int getCollidablePid(int idx) const { return getNodePid(idx, PID_FLG_COLLIDABLE); }
  inline int getCastShadowsPid(int idx) const { return getNodePid(idx, PID_FLG_CASTSHADOWS); }
  inline int getCastShadowsOnSelfPid(int idx) const { return getNodePid(idx, PID_FLG_CASTSHADOWS_ON_SELF); }
  inline int getFadePid(int idx) const { return getNodePid(idx, PID_FLG_FADE); }
  inline int getFadeNullPid(int idx) const { return getNodePid(idx, PID_FLG_FADENULL); }
  inline int getBillboardPid(int idx) const { return getNodePid(idx, PID_FLG_BILLBOARD_MESH); }
  inline int getOccluderPid(int idx) const { return getNodePid(idx, PID_FLG_OCCLUDER); }
  inline int getShowOccluderPid(int idx) const { return getNodePid(idx, PID_FLG_SHOW_OCCLUDER); }
  inline int getAutoVisrangePid(int idx) const { return getNodePid(idx, PID_FLG_AUTOMATIC_VISRANGE); }
  inline int getBackFaceDynlightPid(int idx) const { return getNodePid(idx, PID_FLG_BACK_FACE_DYNLIGHT); }
  inline int getDoNotMixLods(int idx) const { return getNodePid(idx, PID_FLG_DO_NOT_MIX_LODS); }

  inline int getNormalsGrpPid(int idx) const { return getNodePid(idx, PID_NORMALS_GRP); }
  inline int getForceWorldPid(int idx) const { return getNodePid(idx, PID_FORCE_WORLD); }
  inline int getForceLocalPid(int idx) const { return getNodePid(idx, PID_FORCE_LOCAL); }
  inline int getForceSphericalPid(int idx) const { return getNodePid(idx, PID_FORCE_SPHERICAL); }
  inline int getForceNonePid(int idx) const { return getNodePid(idx, PID_FORCE_NONE); }
  inline int getForceNormalPid(int idx) const { return getNodePid(idx, PID_FORCE_NORMAL); }

  inline int getLightingGrpPid(int idx) const { return getNodePid(idx, PID_LIGHTING_GRP); }
  inline int getLightingRadioGrpPid(int idx) const { return getNodePid(idx, PID_LIGHTING_RADIO_GRP); }
  inline int getLtDefPid(int idx) const { return getNodePid(idx, PID_LT_DEF); }
  inline int getLtNonePid(int idx) const { return getNodePid(idx, PID_LT_NONE); }
  inline int getLtLightmapPid(int idx) const { return getNodePid(idx, PID_LT_LIGHTMAP); }
  inline int getLtLightmapMulPid(int idx) const { return getNodePid(idx, PID_LT_LIGHTMAP_MUL); }
  inline int getLtVltmapPid(int idx) const { return getNodePid(idx, PID_LT_VLTMAP); }
  inline int getLtVltmapMulPid(int idx) const { return getNodePid(idx, PID_LT_VLTMAP_MUL); }

  inline int getLodGrpPid(int idx) const { return getNodePid(idx, PID_LOD_GRP); }
  inline int getLodNamePid(int idx) const { return getNodePid(idx, PID_LOD_NAME); }
  inline int getLodRangePid(int idx) const { return getNodePid(idx, PID_LOD_RANGE); }

  inline int getLinkedResGrpPid(int idx) const { return getNodePid(idx, PID_LINKED_RESOURCE_GRP); }
  inline int getLinkedResPid(int idx) const { return getNodePid(idx, PID_LINKED_RESOURCE); }

  inline int getLastNodePid(int idx) const { return nodesGrpPid + PID_NODE + PID_NODE_CNT * (idx + 1); }

private:
  int nodesGrpPid;
  int lastNodesGrpPid;
  int nodesCnt;

  INodeModifierClient &client;
  SimpleString grpId;

  NodeFlagsSettings settings;

  enum
  {
    PID_USE_DEFAULT = 1,

    PID_ALL_NODES_GRP,
    PID_ALL_VISRANGE,
    PID_ALL_FLG_RENDERABLE,
    PID_ALL_FLG_COLLIDABLE,
    PID_ALL_FLG_CASTSHADOWS,
    PID_ALL_FLG_CASTSHADOWS_ON_SELF,
    PID_ALL_FLG_FADE,
    PID_ALL_FLG_FADENULL,
    PID_ALL_FLG_BILLBOARD_MESH,
    PID_ALL_FLG_OCCLUDER,
    PID_ALL_FLG_SHOW_OCCLUDER,
    PID_ALL_FLG_AUTOMATIC_VISRANGE,
    PID_ALL_FLG_BACK_FACE_DYNLIGHT,
    PID_ALL_DO_NOT_MIX_LODS,

    PID_ALL_NORMALS_GRP,
    PID_ALL_FORCE_WORLD,
    PID_ALL_FORCE_LOCAL,
    PID_ALL_FORCE_SPHERICAL,
    PID_ALL_FORCE_NONE,
    PID_ALL_FORCE_NORMAL,

    PID_ALL_LIGHTING_GRP,
    PID_ALL_LIGHTING_RADIO_GRP,
    PID_ALL_LT_DEF,
    PID_ALL_LT_NONE,
    PID_ALL_LT_LIGHTMAP,
    PID_ALL_LT_LIGHTMAP_MUL,
    PID_ALL_LT_VLTMAP,
    PID_ALL_LT_VLTMAP_MUL,

    PID_SET_FOR_ALL_BTN,

    PID_NODE,
    PID_NODE_GRP = 0,
    PID_VISRANGE,
    PID_FLG_RENDERABLE,
    PID_FLG_COLLIDABLE,
    PID_FLG_CASTSHADOWS,
    PID_FLG_CASTSHADOWS_ON_SELF,
    PID_FLG_FADE,
    PID_FLG_FADENULL,
    PID_FLG_BILLBOARD_MESH,
    PID_FLG_OCCLUDER,
    PID_FLG_SHOW_OCCLUDER,
    PID_FLG_AUTOMATIC_VISRANGE,
    PID_FLG_BACK_FACE_DYNLIGHT,
    PID_FLG_DO_NOT_MIX_LODS,

    PID_NORMALS_GRP,
    PID_FORCE_WORLD,
    PID_FORCE_LOCAL,
    PID_FORCE_SPHERICAL,
    PID_FORCE_NONE,
    PID_FORCE_NORMAL,

    PID_LIGHTING_GRP,
    PID_LIGHTING_RADIO_GRP,
    PID_LT_DEF,
    PID_LT_NONE,
    PID_LT_LIGHTMAP,
    PID_LT_LIGHTMAP_MUL,
    PID_LT_VLTMAP,
    PID_LT_VLTMAP_MUL,

    PID_LOD_GRP,
    PID_LOD_NAME,
    PID_LOD_RANGE,

    PID_LINKED_RESOURCE_GRP,
    PID_LINKED_RESOURCE,

    PID_NODE_CNT,
  };

  int getAllForceIdx(const NodesData::NodeFlags &flg) const;
  int getForceIdx(const NodesData::NodeFlags &flg) const;
  void fillAllNodesGrp(PropPanel::ContainerPropertyControl &panel, const NodesData &flags);
  void fillNodeGrp(PropPanel::ContainerPropertyControl &panel, const NodesData &flags, int idx);
};
