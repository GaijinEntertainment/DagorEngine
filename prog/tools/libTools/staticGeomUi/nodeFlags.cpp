#include <libTools/staticGeom/staticGeometry.h>
#include <libTools/staticGeomUi/nodeFlags.h>

#include <propPanel2/c_panel_base.h>

#include <util/dag_globDef.h>

#include <debug/dag_debug.h>


#define MINIMAL_LIGHT_MUL 0.01


//==================================================================================================
NodesData::NodeFlags::NodeFlags() :
  name(NULL),
  renderable(0),
  collidable(0),
  castShadows(0),
  castShadowsOnSelf(0),
  forceLocalNorm(0),
  forceWorldNorm(0),
  forceSphericalNorm(0),
  fade(0),
  fadeNull(0),
  billboard(0),
  occluder(0),
  automaticVisRange(1),
  bkFaceDynLight(0),
  doNotMixLods(0),
  visRange(-1),
  linkedRes(NULL),
  topLodName(NULL),
  lodRange(-1),
  light(StaticGeometryNode::LIGHT_DEFAULT)
{}


//==================================================================================================
void NodesData::NodeFlags::set(const StaticGeometryNode &node)
{
  name = node.name;
  topLodName = node.topLodName;
  linkedRes = node.linkedResource;

  renderable = node.flags & StaticGeometryNode::FLG_RENDERABLE ? 1 : 0;
  collidable = node.flags & StaticGeometryNode::FLG_COLLIDABLE ? 1 : 0;
  castShadows = node.flags & StaticGeometryNode::FLG_CASTSHADOWS ? 1 : 0;
  castShadowsOnSelf = node.flags & StaticGeometryNode::FLG_CASTSHADOWS_ON_SELF ? 1 : 0;
  forceLocalNorm = node.flags & StaticGeometryNode::FLG_FORCE_LOCAL_NORMALS ? 1 : 0;
  forceWorldNorm = node.flags & StaticGeometryNode::FLG_FORCE_WORLD_NORMALS ? 1 : 0;
  forceSphericalNorm = node.flags & StaticGeometryNode::FLG_FORCE_SPHERICAL_NORMALS ? 1 : 0;
  fade = node.flags & StaticGeometryNode::FLG_FADE ? 1 : 0;
  fadeNull = node.flags & StaticGeometryNode::FLG_FADENULL ? 1 : 0;
  billboard = node.flags & StaticGeometryNode::FLG_BILLBOARD_MESH ? 1 : 0;
  occluder = node.flags & StaticGeometryNode::FLG_OCCLUDER ? 1 : 0;
  automaticVisRange = node.flags & StaticGeometryNode::FLG_AUTOMATIC_VISRANGE ? 1 : 0;
  bkFaceDynLight = node.flags & StaticGeometryNode::FLG_BACK_FACE_DYNAMIC_LIGHTING ? 1 : 0;
  doNotMixLods = node.flags & StaticGeometryNode::FLG_DO_NOT_MIX_LODS ? 1 : 0;

  forcedNormal = node.normalsDir;

  light = node.lighting;

  ltMul = node.ltMul;
  vltMul = node.vltMul;

  visRange = node.visRange;
  lodRange = node.lodRange;
}


//==================================================================================================
void NodesData::NodeFlags::compare(const NodeFlags &flags)
{
  if (renderable != flags.renderable)
    renderable = 2;

  if (collidable != flags.collidable)
    collidable = 2;

  if (castShadows != flags.castShadows)
    castShadows = 2;

  if (castShadowsOnSelf != flags.castShadowsOnSelf)
    castShadowsOnSelf = 2;

  if (forceLocalNorm != flags.forceLocalNorm)
    forceLocalNorm = 2;

  if (forceWorldNorm != flags.forceWorldNorm)
    forceWorldNorm = 2;

  if (forceSphericalNorm != flags.forceSphericalNorm)
    forceSphericalNorm = 2;

  if (fade != flags.fade)
    fade = 2;

  if (fadeNull != flags.fadeNull)
    fadeNull = 2;

  if (billboard != flags.billboard)
    billboard = 2;

  if (occluder != flags.occluder)
    occluder = 2;

  if (automaticVisRange != flags.automaticVisRange)
    automaticVisRange = 2;

  if (bkFaceDynLight != flags.bkFaceDynLight)
    bkFaceDynLight = 2;

  if (doNotMixLods != flags.doNotMixLods)
    doNotMixLods = 2;

  if (visRange != flags.visRange)
    visRange = MAX_REAL;

  if (light != flags.light)
    light = -1;

  if (ltMul != flags.ltMul)
    ltMul = MAX_REAL;

  if (vltMul != flags.vltMul)
    vltMul = -1;

  if (forceLocalNorm != flags.forceLocalNorm)
    forceLocalNorm = 2;

  if (forceWorldNorm != flags.forceWorldNorm)
    forceWorldNorm = 2;

  if (forceSphericalNorm != flags.forceSphericalNorm)
    forceSphericalNorm = 2;

  if (forcedNormal.x != flags.forcedNormal.x)
    forcedNormal.x = MAX_REAL;
  if (forcedNormal.y != flags.forcedNormal.y)
    forcedNormal.y = MAX_REAL;
  if (forcedNormal.z != flags.forcedNormal.z)
    forcedNormal.z = MAX_REAL;
}


//==================================================================================================
NodeFlagsModfier::NodeFlagsModfier(INodeModifierClient &c, const char *uid) : client(c), grpId(uid) {}


//==================================================================================================
void NodeFlagsModfier::fillPanel(PropertyContainerControlBase &panel, const NodesData &flags, int start_pid, bool create_nodes_grp)
{
  nodesGrpPid = start_pid;

  if (flags.flags.size())
  {
    PropertyContainerControlBase *nodesGroup = NULL, *maxGroup = &panel;

    if (create_nodes_grp)
    {
      nodesGroup = maxGroup = panel.createGroup(start_pid, "Nodes");
      G_ASSERT(maxGroup);
    }

    if (settings.showUseDefault)
    {
      maxGroup->createCheckBox(getUseDefPid(), "Use default", flags.useDefault);
    }

    if (settings.showAllNodesGrp)
      fillAllNodesGrp(*maxGroup, flags);

    nodesCnt = flags.flags.size();
    if (nodesCnt > 128)
      nodesCnt = 128;
    for (int i = 0; i < nodesCnt; ++i)
      fillNodeGrp(*maxGroup, flags, i);

    if (nodesGroup)
      panel.setBool(start_pid, true);

    if (flags.flags.size() > nodesCnt)
      panel.createStatic(-1, String(64, "Other %d nodes omitted", flags.flags.size() - nodesCnt));
  }
}


//==================================================================================================
int NodeFlagsModfier::getAllForceIdx(const NodesData::NodeFlags &flg) const
{
  if (!flg.forceLocalNorm && !flg.forceWorldNorm && !flg.forceSphericalNorm)
    return PID_ALL_FORCE_NONE;
  else
  {
    if (flg.forceLocalNorm == 1)
      return PID_ALL_FORCE_LOCAL;
    else if (flg.forceWorldNorm == 1)
      return PID_ALL_FORCE_WORLD;
    else if (flg.forceSphericalNorm == 1)
      return PID_ALL_FORCE_SPHERICAL;
  }

  return -1;
}


//==================================================================================================
int NodeFlagsModfier::getForceIdx(const NodesData::NodeFlags &flg) const
{
  if (!flg.forceLocalNorm && !flg.forceWorldNorm && !flg.forceSphericalNorm)
    return PID_FORCE_NONE;
  else
  {
    if (flg.forceLocalNorm == 1)
      return PID_FORCE_LOCAL;
    else if (flg.forceWorldNorm == 1)
      return PID_FORCE_WORLD;
    else if (flg.forceSphericalNorm == 1)
      return PID_FORCE_SPHERICAL;
  }

  return -1;
}


//==================================================================================================
void NodeFlagsModfier::fillAllNodesGrp(PropertyContainerControlBase &panel, const NodesData &flags)
{
  PropertyContainerControlBase *maxGroup = NULL;
  PropertyContainerControlBase *radGrp = NULL;

  maxGroup = panel.createGroup(getAllNodesGrpPid(), "Set for all nodes");
  G_ASSERT(maxGroup);

  bool doEnable = !flags.useDefault;

  NodesData::NodeFlags flg = flags.flags[0];

  int forceNormals = getAllForceIdx(flg);

  for (int i = 1; i < flags.flags.size(); ++i)
  {
    flg.compare(flags.flags[i]);

    if (getAllForceIdx(flags.flags[i]) != forceNormals)
      forceNormals = -1;
  }

  if (settings.showVisRange)
  {
    maxGroup->createEditFloat(getAllVisrangePid(), "Visibility range:", (flg.visRange != MAX_REAL) ? (float)flg.visRange : 0, 2,
      doEnable);
  }

  if (settings.showAutomaticVisRange)
  {
    maxGroup->createCheckBox(getAllAutoVisrangePid(), "Automatic visibility range", (bool)flg.automaticVisRange, doEnable);
    maxGroup->createIndent();
  }

  if (settings.showRenderable)
  {
    maxGroup->createCheckBox(getAllRenderablePid(), "Renderable", (bool)flg.renderable, doEnable);
  }
  if (settings.showCollidable)
  {
    maxGroup->createCheckBox(getAllCollidablePid(), "Collidable", (bool)flg.collidable, doEnable);
  }
  if (settings.showCastShadows)
  {
    maxGroup->createCheckBox(getAllCastShadowsPid(), "Cast shadows", (bool)flg.castShadows, doEnable);
  }
  if (settings.showCastShadowsOnSelf)
  {
    maxGroup->createCheckBox(getAllCastShadowsOnSelfPid(), "Self shadows", (bool)flg.castShadowsOnSelf, doEnable);
  }
  if (settings.showFade)
  {
    maxGroup->createCheckBox(getAllFadePid(), "Fade", (bool)flg.fade, doEnable);
  }
  if (settings.showFadeNull)
  {
    maxGroup->createCheckBox(getAllFadeNullPid(), "Fade null", (bool)flg.fadeNull, doEnable);
  }
  if (settings.showBillboard)
  {
    maxGroup->createCheckBox(getAllBillboardPid(), "Billboard", (bool)flg.billboard, false);
  }
  if (settings.showOccluder)
  {
    maxGroup->createCheckBox(getAllOccluderPid(), "Occluder", (bool)flg.occluder, doEnable);
  }
  if (settings.showBkFaceDynLight)
  {
    maxGroup->createCheckBox(getAllBackFaceDynlightPid(), "Back face dynamic lighting", (bool)flg.bkFaceDynLight, doEnable);
  }
  if (settings.showDoNotMixLods)
  {
    maxGroup->createCheckBox(getAllDoNotMixLods(), "Do not mix LODs", (bool)flg.doNotMixLods, doEnable);
  }

  maxGroup->createIndent();

  if (settings.showForceNormals)
  {
    radGrp = maxGroup->createRadioGroup(getAllNormalsGrpPid(), "Force normals:");
    G_ASSERT(radGrp);
    radGrp->setEnabledById(getAllNormalsGrpPid(), forceNormals);

    radGrp->createRadio(PID_ALL_FORCE_NONE, "Do not force", doEnable);
    radGrp->createRadio(PID_ALL_FORCE_LOCAL, "Force local", doEnable);
    radGrp->createRadio(PID_ALL_FORCE_WORLD, "Force world", doEnable);
    radGrp->createRadio(PID_ALL_FORCE_SPHERICAL, "Force spherical", doEnable);
  }

  if (settings.showForcedNormal)
  {
    if (flg.forcedNormal.x == MAX_REAL)
      flg.forcedNormal.x = 0;
    if (flg.forcedNormal.y == MAX_REAL)
      flg.forcedNormal.y = 0;
    if (flg.forcedNormal.z == MAX_REAL)
      flg.forcedNormal.z = 0;

    maxGroup->createPoint3(getAllForceNormalPid(), "Forced normal:", flg.forcedNormal, 2, false);
  }

  if (settings.showLighting)
  {
    PropertyContainerControlBase *ltGrp = maxGroup->createGroup(getAllLightingGrpPid(), "Lighting");
    G_ASSERT(ltGrp);

    radGrp = ltGrp->createRadioGroup(getAllLightingRadioGrpPid(), NULL);
    radGrp->setEnabledById(getAllLightingRadioGrpPid(), flg.light);
    G_ASSERT(radGrp);

    if (settings.showDefLightRadio)
    {
      radGrp->createRadio(StaticGeometryNode::LIGHT_DEFAULT, "Default", doEnable);
    }

    radGrp->createRadio(StaticGeometryNode::LIGHT_NONE, "None", doEnable);
    radGrp->createRadio(StaticGeometryNode::LIGHT_LIGHTMAP, "Lightmap", doEnable);
    radGrp->createEditFloat(getAllLtLightmampMulPid(), "Lightmap mul:", (flg.ltMul != MAX_REAL) ? flg.ltMul : 0, 2, doEnable);
    radGrp->createRadio(StaticGeometryNode::LIGHT_VLTMAP, "Vltmap", doEnable);
    radGrp->createEditInt(getAllLtVltmapMulPid(), "Vertex light mul:", flg.vltMul, doEnable);

    ltGrp->setBoolValue(true);
  }

  maxGroup->createButton(getAllSetButtonPid(), "Set for all nodes", doEnable);
  maxGroup->setBoolValue(true);

  lastNodesGrpPid = getAllSetButtonPid();
}


//==================================================================================================
void NodeFlagsModfier::fillNodeGrp(PropertyContainerControlBase &panel, const NodesData &flags, int idx)
{
  PropertyContainerControlBase *maxGroup = NULL;
  PropertyContainerControlBase *radGrp = NULL;

  const NodesData::NodeFlags &flg = flags.flags[idx];
  bool doEnable = !flags.useDefault;

  maxGroup = panel.createGroup(getNodeGrpPid(idx), flg.name);
  G_ASSERT(maxGroup);

  if (settings.showVisRange)
  {
    maxGroup->createEditFloat(getVisrangePid(idx), "Visibility range:", (flg.visRange != MAX_REAL) ? flg.visRange : 0, 2,
      doEnable && !flg.automaticVisRange);
  }

  if (settings.showAutomaticVisRange)
  {
    maxGroup->createCheckBox(getAutoVisrangePid(idx), "Automatic visibility ranget", (bool)flg.automaticVisRange, doEnable);
    maxGroup->createIndent();
  }

  if (settings.showRenderable)
  {
    maxGroup->createCheckBox(getRenderablePid(idx), "Renderable", (bool)flg.renderable, doEnable);
  }
  if (settings.showCollidable)
  {
    maxGroup->createCheckBox(getCollidablePid(idx), "Collidable", (bool)flg.collidable, doEnable);
  }
  if (settings.showCastShadows)
  {
    maxGroup->createCheckBox(getCastShadowsPid(idx), "Cast shadows", (bool)flg.castShadows, doEnable);
  }
  if (settings.showCastShadowsOnSelf)
  {
    maxGroup->createCheckBox(getCastShadowsOnSelfPid(idx), "Self shadows", (bool)flg.castShadowsOnSelf, doEnable);
  }
  if (settings.showFade)
  {
    maxGroup->createCheckBox(getFadePid(idx), "Fade", (bool)flg.fade, doEnable);
  }
  if (settings.showFadeNull)
  {
    maxGroup->createCheckBox(getFadeNullPid(idx), "Fade null", (bool)flg.fadeNull, doEnable);
  }
  if (settings.showBillboard)
  {
    maxGroup->createCheckBox(getBillboardPid(idx), "Billboard", (bool)flg.billboard, false);
  }
  if (settings.showOccluder)
  {
    maxGroup->createCheckBox(getOccluderPid(idx), "Occluder", (bool)flg.occluder, doEnable);
  }
  if (settings.showBkFaceDynLight)
  {
    maxGroup->createCheckBox(getBackFaceDynlightPid(idx), "Back face dynamic lighting", (bool)flg.bkFaceDynLight, doEnable);
  }
  if (settings.showDoNotMixLods)
  {
    maxGroup->createCheckBox(getDoNotMixLods(idx), "Do not mix LODs", (bool)flg.doNotMixLods, doEnable);
  }

  maxGroup->createIndent();

  if (settings.showForceNormals)
  {
    radGrp = maxGroup->createRadioGroup(getNormalsGrpPid(idx), "Force normals:");
    radGrp->setEnabledById(getNormalsGrpPid(idx), getForceIdx(flg));
    G_ASSERT(radGrp);

    radGrp->createRadio(PID_FORCE_NONE, "Do not force", doEnable);
    radGrp->createRadio(PID_FORCE_LOCAL, "Force local", doEnable);
    radGrp->createRadio(PID_FORCE_WORLD, "Force world", doEnable);
    radGrp->createRadio(PID_FORCE_SPHERICAL, "Force spherical", doEnable);

    maxGroup->setInt(getNormalsGrpPid(idx), getForceIdx(flg));

    if (settings.showForcedNormal)
    {
      maxGroup->createPoint3(getForceNormalPid(idx), "Forced normal:", flg.forcedNormal, 2, false);
    }
  }

  PropertyContainerControlBase *subGrp = NULL;

  if (settings.showLighting)
  {
    subGrp = maxGroup->createGroup(getLightingGrpPid(idx), "Lighting");
    G_ASSERT(subGrp);

    radGrp = subGrp->createRadioGroup(getLightingRadioGrpPid(idx), NULL);
    radGrp->setEnabledById(getLightingRadioGrpPid(idx), flg.light);
    G_ASSERT(radGrp);

    if (settings.showDefLightRadio)
    {
      radGrp->createRadio(StaticGeometryNode::LIGHT_DEFAULT, "Default", doEnable);
    }

    radGrp->createRadio(StaticGeometryNode::LIGHT_NONE, "None", doEnable);

    radGrp->createRadio(StaticGeometryNode::LIGHT_LIGHTMAP, "Lightmap", doEnable);

    radGrp->createEditFloat(getLtLightmapMulPid(idx), "Lightmap mul:", flg.ltMul, 2, doEnable);

    radGrp->createRadio(StaticGeometryNode::LIGHT_VLTMAP, "Vltmap", doEnable);

    radGrp->createEditInt(getLtVltmapMulPid(idx), "Vertex light mul:", flg.vltMul, doEnable);

    subGrp->setInt(getLightingRadioGrpPid(idx), flg.light);
    subGrp->setBoolValue(true);
  }

  if (settings.showLod && (settings.editLodRange || settings.editLodName || (flg.topLodName && *flg.topLodName)))
  {
    subGrp = maxGroup->createGroup(getLodGrpPid(idx), "LOD");
    G_ASSERT(subGrp);

    if (settings.editLodName)
    {
      subGrp->createEditBox(getLodNamePid(idx), "Top LOD name:", flg.topLodName);
    }
    else
      subGrp->createStatic(getLodNamePid(idx), flg.topLodName);

    subGrp->createEditFloat(getLodRangePid(idx), "LOD range:", flg.lodRange);

    subGrp->setBoolValue(true);
  }

  if (settings.showLinkedRes)
  {
    if (!settings.disableLinkedRes || (settings.disableLinkedRes && flg.linkedRes && *flg.linkedRes))
    {
      subGrp = maxGroup->createGroup(getLinkedResGrpPid(idx), "Linked resource");
      G_ASSERT(subGrp);

      if (settings.disableLinkedRes)
        subGrp->createStatic(getLinkedResPid(idx), flg.linkedRes);
      else
      {
        subGrp->createEditBox(getLinkedResPid(idx), "Linked resource name:", flg.linkedRes);
      }

      subGrp->setBoolValue(true);
    }
  }

  maxGroup->setBoolValue(true);
  lastNodesGrpPid = getLastNodePid(idx);
}


//==================================================================================================
bool NodeFlagsModfier::onPPChange(PropertyContainerControlBase &panel, int pid)
{
  if (pid == getUseDefPid())
  {
    bool useDef = panel.getBool(pid);

    // TODO: probably Panel parameters enable/disable

    client.onUseDefaultChanged(useDef);
    return true;
  }

  if (pid >= getFirstPid() && pid < getNodeGrpPid(0))
    return true;

  if (pid < getNodeGrpPid(0) || pid > getLastPid())
    return false;

  const int idx = pid - nodesGrpPid - PID_NODE;
  const int nodeIdx = idx / PID_NODE_CNT;
  int flagId = idx - nodeIdx * PID_NODE_CNT;

  if (pid == getNormalsGrpPid(nodeIdx) || pid == getLightingRadioGrpPid(nodeIdx))
  {
    flagId = panel.getInt(pid);
    if (flagId == RADIO_SELECT_NONE)
      return false;

    switch (flagId)
    {
      case PID_FORCE_WORLD:
        client.onNodeFlagsChanged(nodeIdx, StaticGeometryNode::FLG_FORCE_WORLD_NORMALS,
          ~(StaticGeometryNode::FLG_FORCE_LOCAL_NORMALS | StaticGeometryNode::FLG_FORCE_SPHERICAL_NORMALS));
        return true;

      case PID_FORCE_LOCAL:
        client.onNodeFlagsChanged(nodeIdx, StaticGeometryNode::FLG_FORCE_LOCAL_NORMALS,
          ~(StaticGeometryNode::FLG_FORCE_WORLD_NORMALS | StaticGeometryNode::FLG_FORCE_SPHERICAL_NORMALS));
        return true;

      case PID_FORCE_SPHERICAL:
        client.onNodeFlagsChanged(nodeIdx, StaticGeometryNode::FLG_FORCE_SPHERICAL_NORMALS,
          ~(StaticGeometryNode::FLG_FORCE_WORLD_NORMALS | StaticGeometryNode::FLG_FORCE_LOCAL_NORMALS));
        return true;

      case PID_FORCE_NONE:
        client.onNodeFlagsChanged(nodeIdx, 0,
          ~(StaticGeometryNode::FLG_FORCE_WORLD_NORMALS | StaticGeometryNode::FLG_FORCE_LOCAL_NORMALS |
            StaticGeometryNode::FLG_FORCE_SPHERICAL_NORMALS));
        return true;


      case PID_LT_DEF: client.onLightingChanged(nodeIdx, StaticGeometryNode::LIGHT_DEFAULT); return true;

      case PID_LT_NONE: client.onLightingChanged(nodeIdx, StaticGeometryNode::LIGHT_NONE); return true;

      case PID_LT_LIGHTMAP: client.onLightingChanged(nodeIdx, StaticGeometryNode::LIGHT_LIGHTMAP); return true;

      case PID_LT_VLTMAP: client.onLightingChanged(nodeIdx, StaticGeometryNode::LIGHT_VLTMAP); return true;
    }

    return false;
  }

  if (nodeIdx >= nodesCnt)
    return false;

  int iVal;
  real rVal;
  String sVal;

  switch (flagId)
  {
    case PID_FLG_RENDERABLE:
      iVal = panel.getBool(pid);
      G_ASSERT(iVal != -1);

      iVal ? client.onNodeFlagsChanged(nodeIdx, StaticGeometryNode::FLG_RENDERABLE, ~0)
           : client.onNodeFlagsChanged(nodeIdx, 0, ~StaticGeometryNode::FLG_RENDERABLE);
      return true;

    case PID_FLG_COLLIDABLE:
      iVal = panel.getBool(pid);
      G_ASSERT(iVal != -1);

      iVal ? client.onNodeFlagsChanged(nodeIdx, StaticGeometryNode::FLG_COLLIDABLE, ~0)
           : client.onNodeFlagsChanged(nodeIdx, 0, ~StaticGeometryNode::FLG_COLLIDABLE);
      return true;

    case PID_FLG_CASTSHADOWS:
      iVal = panel.getBool(pid);
      G_ASSERT(iVal != -1);

      iVal ? client.onNodeFlagsChanged(nodeIdx, StaticGeometryNode::FLG_CASTSHADOWS, ~0)
           : client.onNodeFlagsChanged(nodeIdx, 0, ~StaticGeometryNode::FLG_CASTSHADOWS);
      return true;

    case PID_FLG_CASTSHADOWS_ON_SELF:
      iVal = panel.getBool(pid);
      G_ASSERT(iVal != -1);

      iVal ? client.onNodeFlagsChanged(nodeIdx, StaticGeometryNode::FLG_CASTSHADOWS_ON_SELF, ~0)
           : client.onNodeFlagsChanged(nodeIdx, 0, ~StaticGeometryNode::FLG_CASTSHADOWS_ON_SELF);
      return true;

    case PID_FLG_FADE:
      iVal = panel.getBool(pid);
      G_ASSERT(iVal != -1);

      iVal ? client.onNodeFlagsChanged(nodeIdx, StaticGeometryNode::FLG_FADE, ~0)
           : client.onNodeFlagsChanged(nodeIdx, 0, ~StaticGeometryNode::FLG_FADE);
      return true;

    case PID_FLG_FADENULL:
      iVal = panel.getBool(pid);
      G_ASSERT(iVal != -1);

      iVal ? client.onNodeFlagsChanged(nodeIdx, StaticGeometryNode::FLG_FADENULL, ~0)
           : client.onNodeFlagsChanged(nodeIdx, 0, ~StaticGeometryNode::FLG_FADENULL);
      return true;

    case PID_FLG_OCCLUDER:
      iVal = panel.getBool(pid);
      G_ASSERT(iVal != -1);

      iVal ? client.onNodeFlagsChanged(nodeIdx, StaticGeometryNode::FLG_OCCLUDER, ~0)
           : client.onNodeFlagsChanged(nodeIdx, 0, ~StaticGeometryNode::FLG_OCCLUDER);
      return true;

    case PID_FLG_AUTOMATIC_VISRANGE:
      iVal = panel.getBool(pid);
      G_ASSERT(iVal != -1);

      if (settings.showVisRange)
        panel.setEnabledById(getVisrangePid(nodeIdx), !(bool)iVal);

      iVal ? client.onNodeFlagsChanged(nodeIdx, StaticGeometryNode::FLG_AUTOMATIC_VISRANGE, ~0)
           : client.onNodeFlagsChanged(nodeIdx, 0, ~StaticGeometryNode::FLG_AUTOMATIC_VISRANGE);

      return true;

    case PID_FLG_BACK_FACE_DYNLIGHT:
      iVal = panel.getBool(pid);
      G_ASSERT(iVal != -1);

      iVal ? client.onNodeFlagsChanged(nodeIdx, StaticGeometryNode::FLG_BACK_FACE_DYNAMIC_LIGHTING, ~0)
           : client.onNodeFlagsChanged(nodeIdx, 0, ~StaticGeometryNode::FLG_BACK_FACE_DYNAMIC_LIGHTING);
      return true;

    case PID_FLG_DO_NOT_MIX_LODS:
      iVal = panel.getBool(pid);
      G_ASSERT(iVal != -1);

      iVal ? client.onNodeFlagsChanged(nodeIdx, StaticGeometryNode::FLG_DO_NOT_MIX_LODS, ~0)
           : client.onNodeFlagsChanged(nodeIdx, 0, ~StaticGeometryNode::FLG_DO_NOT_MIX_LODS);
      return true;

    case PID_VISRANGE:
      rVal = panel.getFloat(pid);
      if (rVal != MAX_REAL)
        client.onVisRangeChanged(nodeIdx, rVal);
      return true;

    case PID_LT_LIGHTMAP_MUL:
      rVal = panel.getFloat(pid);
      if (rVal != MAX_REAL && rVal > MINIMAL_LIGHT_MUL)
        client.onLtMulChanged(nodeIdx, rVal);
      return true;

    case PID_LT_VLTMAP_MUL:
      iVal = panel.getInt(pid);
      if (iVal > 0)
        client.onVltMulChanged(nodeIdx, iVal);
      return true;

    case PID_LINKED_RESOURCE:
      sVal = panel.getText(pid);
      client.onLinkedResChanged(nodeIdx, sVal);
      return true;

    case PID_LOD_NAME:
      sVal = panel.getText(pid);
      client.onTopLodChanged(nodeIdx, sVal);
      break;

    case PID_LOD_RANGE:
      iVal = panel.getInt(pid);
      if (iVal >= 0)
        client.onLodRangeChanged(nodeIdx, iVal);
      break;
  }

  return false;
}


//==================================================================================================
bool NodeFlagsModfier::onPPBtnPressed(PropertyContainerControlBase &panel, int pid)
{
  if (pid != getAllSetButtonPid())
    return false;

  Tab<int> setPids(tmpmem);

  int check;
  int orFlags = 0;
  int andFlags = 0;

  check = panel.getBool(getAllRenderablePid());
  if (check == 1)
  {
    orFlags |= StaticGeometryNode::FLG_RENDERABLE;
    setPids.push_back(PID_FLG_RENDERABLE);
  }
  else if (!check)
  {
    andFlags |= StaticGeometryNode::FLG_RENDERABLE;
    setPids.push_back(-PID_FLG_RENDERABLE);
  }

  check = panel.getBool(getAllCollidablePid());
  if (check == 1)
  {
    orFlags |= StaticGeometryNode::FLG_COLLIDABLE;
    setPids.push_back(PID_FLG_COLLIDABLE);
  }
  else if (!check)
  {
    andFlags |= StaticGeometryNode::FLG_COLLIDABLE;
    setPids.push_back(-PID_FLG_COLLIDABLE);
  }

  check = panel.getBool(getAllCastShadowsPid());
  if (check == 1)
  {
    orFlags |= StaticGeometryNode::FLG_CASTSHADOWS;
    setPids.push_back(PID_FLG_CASTSHADOWS);
  }
  else if (!check)
  {
    andFlags |= StaticGeometryNode::FLG_CASTSHADOWS;
    setPids.push_back(-PID_FLG_CASTSHADOWS);
  }

  check = panel.getBool(getAllCastShadowsOnSelfPid());
  if (check == 1)
  {
    orFlags |= StaticGeometryNode::FLG_CASTSHADOWS_ON_SELF;
    setPids.push_back(PID_FLG_CASTSHADOWS_ON_SELF);
  }
  else if (!check)
  {
    andFlags |= StaticGeometryNode::FLG_CASTSHADOWS_ON_SELF;
    setPids.push_back(-PID_FLG_CASTSHADOWS_ON_SELF);
  }

  check = panel.getBool(getAllFadePid());
  if (check == 1)
  {
    orFlags |= StaticGeometryNode::FLG_FADE;
    setPids.push_back(PID_FLG_FADE);
  }
  else if (!check)
  {
    andFlags |= StaticGeometryNode::FLG_FADE;
    setPids.push_back(-PID_FLG_FADE);
  }

  check = panel.getBool(getAllFadeNullPid());
  if (check == 1)
  {
    orFlags |= StaticGeometryNode::FLG_FADENULL;
    setPids.push_back(PID_FLG_FADENULL);
  }
  else if (!check)
  {
    andFlags |= StaticGeometryNode::FLG_FADENULL;
    setPids.push_back(-PID_FLG_FADENULL);
  }

  check = panel.getBool(getAllOccluderPid());
  if (check == 1)
  {
    orFlags |= StaticGeometryNode::FLG_OCCLUDER;
    setPids.push_back(PID_FLG_OCCLUDER);
  }
  else if (!check)
  {
    andFlags |= StaticGeometryNode::FLG_OCCLUDER;
    setPids.push_back(PID_FLG_OCCLUDER);
  }

  check = panel.getBool(getAllAutoVisrangePid());
  if (check == 1)
  {
    orFlags |= StaticGeometryNode::FLG_AUTOMATIC_VISRANGE;
    setPids.push_back(PID_FLG_AUTOMATIC_VISRANGE);
  }
  else if (!check)
  {
    andFlags |= StaticGeometryNode::FLG_AUTOMATIC_VISRANGE;
    setPids.push_back(-PID_FLG_AUTOMATIC_VISRANGE);
  }

  check = panel.getBool(getAllBackFaceDynlightPid());
  if (check == 1)
  {
    orFlags |= StaticGeometryNode::FLG_BACK_FACE_DYNAMIC_LIGHTING;
    setPids.push_back(PID_FLG_BACK_FACE_DYNLIGHT);
  }
  else if (!check)
  {
    andFlags |= StaticGeometryNode::FLG_BACK_FACE_DYNAMIC_LIGHTING;
    setPids.push_back(-PID_FLG_BACK_FACE_DYNLIGHT);
  }

  check = panel.getBool(getAllDoNotMixLods());
  if (check == 1)
  {
    orFlags |= StaticGeometryNode::FLG_DO_NOT_MIX_LODS;
    setPids.push_back(PID_FLG_DO_NOT_MIX_LODS);
  }
  else if (!check)
  {
    andFlags |= StaticGeometryNode::FLG_DO_NOT_MIX_LODS;
    setPids.push_back(-PID_FLG_DO_NOT_MIX_LODS);
  }

  PropertyControlBase *grp = panel.getById(getAllNormalsGrpPid());

  if (grp)
  {
    const int norm = panel.getInt(getAllNormalsGrpPid());
    switch (norm)
    {
      case PID_ALL_FORCE_NONE:
        andFlags |= StaticGeometryNode::FLG_FORCE_LOCAL_NORMALS | StaticGeometryNode::FLG_FORCE_WORLD_NORMALS |
                    StaticGeometryNode::FLG_FORCE_SPHERICAL_NORMALS;
        setPids.push_back(PID_FORCE_NONE);
        break;

      case PID_ALL_FORCE_WORLD:
        andFlags |= StaticGeometryNode::FLG_FORCE_LOCAL_NORMALS | StaticGeometryNode::FLG_FORCE_SPHERICAL_NORMALS;
        orFlags |= StaticGeometryNode::FLG_FORCE_WORLD_NORMALS;
        setPids.push_back(PID_FORCE_WORLD);
        break;

      case PID_ALL_FORCE_LOCAL:
        andFlags |= StaticGeometryNode::FLG_FORCE_WORLD_NORMALS | StaticGeometryNode::FLG_FORCE_SPHERICAL_NORMALS;
        orFlags |= StaticGeometryNode::FLG_FORCE_LOCAL_NORMALS;
        setPids.push_back(PID_FORCE_LOCAL);
        break;

      case PID_ALL_FORCE_SPHERICAL:
        andFlags |= StaticGeometryNode::FLG_FORCE_LOCAL_NORMALS | StaticGeometryNode::FLG_FORCE_WORLD_NORMALS;
        orFlags |= StaticGeometryNode::FLG_FORCE_SPHERICAL_NORMALS;
        setPids.push_back(PID_FORCE_SPHERICAL);
        break;
    }
  }

  andFlags = ~andFlags;

  float visRange = panel.getFloat(getAllVisrangePid());
  float ltMul = panel.getFloat(getAllLtLightmampMulPid());
  int vltMul = panel.getInt(getAllLtVltmapMulPid());

  grp = panel.getById(getAllLightingRadioGrpPid());

  int lighting = -1;

  if (grp)
    lighting = panel.getInt(getAllLightingRadioGrpPid());

  for (int i = 0; i < nodesCnt; ++i)
  {
    client.onNodeFlagsChanged(i, orFlags, andFlags);

    if (visRange != MAX_REAL)
    {
      client.onVisRangeChanged(i, visRange);
      setPids.push_back(PID_VISRANGE);
    }

    if (lighting != RADIO_SELECT_NONE)
    {
      client.onLightingChanged(i, (StaticGeometryNode::Lighting)lighting);

      switch (lighting)
      {
        case StaticGeometryNode::LIGHT_DEFAULT: setPids.push_back(PID_LT_DEF); break;
        case StaticGeometryNode::LIGHT_NONE: setPids.push_back(PID_LT_NONE); break;
        case StaticGeometryNode::LIGHT_LIGHTMAP: setPids.push_back(PID_LT_LIGHTMAP); break;
        case StaticGeometryNode::LIGHT_VLTMAP: setPids.push_back(PID_LT_VLTMAP); break;
        default: fatal("Unknown node lighting");
      }
    }

    if (ltMul != MAX_REAL && ltMul > MINIMAL_LIGHT_MUL)
    {
      client.onLtMulChanged(i, ltMul);
      setPids.push_back(PID_LT_LIGHTMAP_MUL);
    }

    if (vltMul != -1 && vltMul > 0)
    {
      client.onVltMulChanged(i, vltMul);
      setPids.push_back(PID_LT_VLTMAP_MUL);
    }

    for (int j = 0; j < setPids.size(); ++j)
    {
      int pid = setPids[j];
      bool doSet = true;
      if (pid < 0)
      {
        pid = -pid;
        doSet = false;
      }

      switch (pid)
      {
        case PID_FLG_RENDERABLE:
        case PID_FLG_COLLIDABLE:
        case PID_FLG_CASTSHADOWS:
        case PID_FLG_CASTSHADOWS_ON_SELF:
        case PID_FLG_FADE:
        case PID_FLG_FADENULL:
        case PID_FLG_OCCLUDER:
        case PID_FLG_AUTOMATIC_VISRANGE:
        case PID_FLG_BACK_FACE_DYNLIGHT:
        case PID_FLG_DO_NOT_MIX_LODS:

        case PID_FORCE_WORLD:
        case PID_FORCE_LOCAL:
        case PID_FORCE_SPHERICAL:
        case PID_FORCE_NONE:

        case PID_LT_DEF:
        case PID_LT_NONE:
        case PID_LT_LIGHTMAP:
        case PID_LT_VLTMAP:
        {
          int iVal = doSet ? 1 : 0;
          panel.setInt(getNodePid(i, pid), iVal);
          break;
        }

        case PID_VISRANGE: panel.setInt(getNodePid(i, pid), visRange); break;

        case PID_LT_LIGHTMAP_MUL: panel.setInt(getNodePid(i, pid), ltMul); break;

        case PID_LT_VLTMAP_MUL: panel.setInt(getNodePid(i, pid), vltMul); break;
      }
    }
  }

  return true;
}
