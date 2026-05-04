// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include <dasModules/aotAnimchar.h>
#include <dasModules/aotDagorMath.h>
#include <dasModules/dasMacro.h>
#include <dasModules/dasDataBlock.h>
#include <anim/dag_animBlend.h>
#include <anim/dag_animPostBlendCtrl.h>

DAS_BASE_BIND_ENUM_98(AnimcharVisbits, AnimcharVisbits, VISFLG_WITHIN_RANGE, VISFLG_LOD_CHOSEN, VISFLG_GEOM_TREE_UPDATED,
  VISFLG_MAIN_VISIBLE, VISFLG_MAIN_AND_SHADOW_VISIBLE, VISFLG_COCKPIT_VISIBLE, VISFLG_HMAP_DEFORM, VISFLG_OUTLINE_RENDER, VISFLG_BVH,
  VISFLG_DYNAMIC_MIRROR, VISFLG_RENDER_CUSTOM, VISFLG_SEMI_TRANS_RENDERED, VISFLG_CSM_SHADOW_RENDERED, VISFLG_MAIN_CAMERA_RENDERED,
  VISFLG_SHADOW_CASCADE_0_VISIBLE, VISFLG_SHADOW_CASCADE_LAST_VISIBLE, VISFLG_SHADOW_CASCADE_VISIBLE_MASK, VISFLG_ALL_BITS);

DAS_ANNOTATE_VECTOR(BnlPtrTab, BnlPtrTab)
DAS_ANNOTATE_VECTOR(PbCtrlPtrTab, PbCtrlPtrTab)

struct NameIdPairAnnotation : das::ManagedStructureAnnotation<das::NameIdPair, false>
{
  NameIdPairAnnotation(das::ModuleLibrary &ml) : ManagedStructureAnnotation("NameIdPair", ml)
  {
    cppName = " ::das::NameIdPair";
    addField<DAS_BIND_MANAGED_FIELD(id)>("id");
    addField<DAS_BIND_MANAGED_FIELD(name)>("name");
  }
};

struct AnimcharDebugContextAnnotation : das::ManagedStructureAnnotation<AnimV20::AnimcharDebugContext, false>
{
  AnimcharDebugContextAnnotation(das::ModuleLibrary &ml) : ManagedStructureAnnotation("AnimcharDebugContext", ml)
  {
    cppName = " ::AnimV20::AnimcharDebugContext";
  }
};

struct AnimcharBaseComponentAnnotation : das::ManagedStructureAnnotation<AnimV20::AnimcharBaseComponent, false>
{
  AnimcharBaseComponentAnnotation(das::ModuleLibrary &ml) : ManagedStructureAnnotation("AnimcharBaseComponent", ml)
  {
    cppName = " ::AnimV20::AnimcharBaseComponent";
    addPropertyExtConst<AnimV20::AnimationGraph *(AnimV20::AnimcharBaseComponent::*)(), &AnimV20::AnimcharBaseComponent::getAnimGraph,
      const AnimV20::AnimationGraph *(AnimV20::AnimcharBaseComponent::*)() const, &AnimV20::AnimcharBaseComponent::getAnimGraph>(
      "animGraph", "getAnimGraph");
    addPropertyExtConst<AnimV20::IAnimStateHolder *(AnimV20::AnimcharBaseComponent::*)(),
      &AnimV20::AnimcharBaseComponent::getAnimState, const AnimV20::IAnimStateHolder *(AnimV20::AnimcharBaseComponent::*)() const,
      &AnimV20::AnimcharBaseComponent::getAnimState>("animState", "getAnimState");
    addProperty<DAS_BIND_MANAGED_PROP(getOriginalNodeTree)>("originalNodeTree", "getOriginalNodeTree");
    addPropertyExtConst<GeomNodeTree *(AnimV20::AnimcharBaseComponent::*)(), &AnimV20::AnimcharBaseComponent::getNodeTreePtr,
      const GeomNodeTree *(AnimV20::AnimcharBaseComponent::*)() const, &AnimV20::AnimcharBaseComponent::getNodeTreePtr>("nodeTree",
      "getNodeTreePtr");
    addProperty<DAS_BIND_MANAGED_PROP(createOrReturnExistingDebugContext)>("animcharDebugContext",
      "createOrReturnExistingDebugContext");
  }
};

struct IAnimCharacter2Annotation : das::ManagedStructureAnnotation<AnimV20::IAnimCharacter2, false>
{
  IAnimCharacter2Annotation(das::ModuleLibrary &ml) : ManagedStructureAnnotation("IAnimCharacter2", ml)
  {
    cppName = " ::AnimV20::IAnimCharacter2";

    addProperty<DAS_BIND_MANAGED_PROP(baseComp)>("base", "baseComp");
  }
};

DAS_ANNOTATE_VECTOR(RoNameMapExIdPatchableTab, RoNameMapExIdPatchableTab)

struct RoNameMapExAnnotation : das::ManagedStructureAnnotation<RoNameMapEx, false>
{
  RoNameMapExAnnotation(das::ModuleLibrary &ml) : ManagedStructureAnnotation("RoNameMapEx", ml)
  {
    addField<DAS_BIND_MANAGED_FIELD(id)>("id");
    cppName = " ::RoNameMapEx";
  }
};

struct DynSceneResNameMapResourceAnnotation : das::ManagedStructureAnnotation<DynSceneResNameMapResource, false>
{
  DynSceneResNameMapResourceAnnotation(das::ModuleLibrary &ml) : ManagedStructureAnnotation("DynSceneResNameMapResource", ml)
  {
    cppName = " ::DynSceneResNameMapResource";
    addField<DAS_BIND_MANAGED_FIELD(node)>("node");
  }
};

struct DynamicRenderableSceneLodsResourceAnnotation : das::ManagedStructureAnnotation<DynamicRenderableSceneLodsResource, false>
{
  DynamicRenderableSceneLodsResourceAnnotation(das::ModuleLibrary &ml) :
    ManagedStructureAnnotation("DynamicRenderableSceneLodsResource", ml)
  {
    cppName = " ::DynamicRenderableSceneLodsResource";
    addProperty<DAS_BIND_MANAGED_PROP(getNames)>("names", "getNames");
  }
};

struct DynamicRenderableSceneInstanceAnnotation : das::ManagedStructureAnnotation<DynamicRenderableSceneInstance, false>
{
  DynamicRenderableSceneInstanceAnnotation(das::ModuleLibrary &ml) : ManagedStructureAnnotation("DynamicRenderableSceneInstance", ml)
  {
    cppName = " ::DynamicRenderableSceneInstance";
    addProperty<DAS_BIND_MANAGED_PROP(getNodeCount)>("nodeCount", "getNodeCount");
    addProperty<DAS_BIND_MANAGED_PROP(getConstLodsResource)>("lodsResource", "getLodsResource");
  }
};

struct AnimcharRendComponentAnnotation : das::ManagedStructureAnnotation<AnimV20::AnimcharRendComponent, false>
{
  AnimcharRendComponentAnnotation(das::ModuleLibrary &ml) : ManagedStructureAnnotation("AnimcharRendComponent", ml)
  {
    cppName = " ::AnimV20::AnimcharRendComponent";
    addPropertyExtConst<DynamicRenderableSceneInstance *(AnimV20::AnimcharRendComponent::*)(),
      &AnimV20::AnimcharRendComponent::getSceneInstance,
      const DynamicRenderableSceneInstance *(AnimV20::AnimcharRendComponent::*)() const,
      &AnimV20::AnimcharRendComponent::getSceneInstance>("sceneInstance", "getSceneInstance");
  }
};

struct AnimBlenderAnnotation : das::ManagedStructureAnnotation<AnimV20::AnimBlender, false>
{
  AnimBlenderAnnotation(das::ModuleLibrary &ml) : ManagedStructureAnnotation("AnimBlender", ml)
  {
    cppName = " ::AnimV20::AnimBlender";

    addField<DAS_BIND_MANAGED_FIELD(bnl)>("bnl");
    addField<DAS_BIND_MANAGED_FIELD(pbCtrl)>("pbCtrl");
    addField<DAS_BIND_MANAGED_FIELD(nodeListValid)>("nodeListValid");
  }
};

struct AnimationGraphAnnotation : das::ManagedStructureAnnotation<AnimV20::AnimationGraph, false>
{
  AnimationGraphAnnotation(das::ModuleLibrary &ml) : ManagedStructureAnnotation("AnimationGraph", ml)
  {
    cppName = " ::AnimV20::AnimationGraph";

    addField<DAS_BIND_MANAGED_FIELD(resId)>("resId");
    addProperty<DAS_BIND_MANAGED_PROP(getParamCount)>("paramCount", "getParamCount");
    addProperty<DAS_BIND_MANAGED_PROP(getAnimNodeCount)>("animNodeCount", "getAnimNodeCount");
    addProperty<DAS_BIND_MANAGED_PROP(getStateCount)>("stateCount", "getStateCount");
    addProperty<DAS_BIND_MANAGED_PROP(getRoot)>("root", "getRoot");
    addProperty<DAS_BIND_MANAGED_PROP(getBlender)>("blender", "getBlender");
  }
};

struct IPureAnimStateHolderAnnotation : das::ManagedStructureAnnotation<AnimV20::IPureAnimStateHolder, false>
{
  IPureAnimStateHolderAnnotation(das::ModuleLibrary &ml) : ManagedStructureAnnotation("IPureAnimStateHolder", ml)
  {
    cppName = " ::AnimV20::IPureAnimStateHolder";
  }
};

struct IAnimBlendNodeAnnotation : das::ManagedStructureAnnotation<AnimV20::IAnimBlendNode, false>
{
  IAnimBlendNodeAnnotation(das::ModuleLibrary &ml) : ManagedStructureAnnotation("IAnimBlendNode", ml)
  {
    cppName = " ::AnimV20::IAnimBlendNode";
  }
};

struct AnimBlendNodeLeafAnnotation : das::ManagedStructureAnnotation<AnimV20::AnimBlendNodeLeaf, false>
{
  AnimBlendNodeLeafAnnotation(das::ModuleLibrary &ml) : ManagedStructureAnnotation("AnimBlendNodeLeaf", ml)
  {
    cppName = " ::AnimV20::AnimBlendNodeLeaf";

    addProperty<DAS_BIND_MANAGED_PROP(isAdditive)>("isAdditive");
    addProperty<DAS_BIND_MANAGED_PROP(isAnimationIgnored)>("isAnimationIgnored");
    addProperty<DAS_BIND_MANAGED_PROP(getBnlId)>("bnlId", "getBnlId");
  }
};

struct AnimPostBlendCtrlAnnotation : das::ManagedStructureAnnotation<AnimV20::AnimPostBlendCtrl, false>
{
  AnimPostBlendCtrlAnnotation(das::ModuleLibrary &ml) : ManagedStructureAnnotation("AnimPostBlendCtrl", ml)
  {
    cppName = " ::AnimV20::AnimPostBlendCtrl";
    addProperty<DAS_BIND_MANAGED_PROP(getPbcId)>("pbcId", "getPbcId");
  }
};

struct AnimBlendNodeNullAnnotation : das::ManagedStructureAnnotation<AnimV20::AnimBlendNodeNull, false>
{
  AnimBlendNodeNullAnnotation(das::ModuleLibrary &ml) : ManagedStructureAnnotation("AnimBlendNodeNull", ml)
  {
    cppName = " ::AnimV20::AnimBlendNodeNull";
  }
};

struct AnimBlendNodeContinuousLeafAnnotation : das::ManagedStructureAnnotation<AnimV20::AnimBlendNodeContinuousLeaf, false>
{
  AnimBlendNodeContinuousLeafAnnotation(das::ModuleLibrary &ml) : ManagedStructureAnnotation("AnimBlendNodeContinuousLeaf", ml)
  {
    cppName = " ::AnimV20::AnimBlendNodeContinuousLeaf";

    addField<DAS_BIND_MANAGED_FIELD(paramId)>("paramId");
    addField<DAS_BIND_MANAGED_FIELD(t0)>("t0");
    addField<DAS_BIND_MANAGED_FIELD(dt)>("dt");
    addField<DAS_BIND_MANAGED_FIELD(rate)>("rate");
    addField<DAS_BIND_MANAGED_FIELD(duration)>("duration");
    addField<DAS_BIND_MANAGED_FIELD(syncTime)>("syncTime");
    addField<DAS_BIND_MANAGED_FIELD(avgSpeed)>("avgSpeed");
    addField<DAS_BIND_MANAGED_FIELD(startOffsetEnabled)>("startOffsetEnabled");
    addField<DAS_BIND_MANAGED_FIELD(eoaIrq)>("eoaIrq");
    addField<DAS_BIND_MANAGED_FIELD(canRewind)>("canRewind");

    addProperty<DAS_BIND_MANAGED_PROP(isAdditive)>("isAdditive");
    addProperty<DAS_BIND_MANAGED_PROP(isAnimationIgnored)>("isAnimationIgnored");
  }
};

struct AnimBlendNodeStillLeafAnnotation : das::ManagedStructureAnnotation<AnimV20::AnimBlendNodeStillLeaf, false>
{
  AnimBlendNodeStillLeafAnnotation(das::ModuleLibrary &ml) : ManagedStructureAnnotation("AnimBlendNodeStillLeaf", ml)
  {
    cppName = " ::AnimV20::AnimBlendNodeStillLeaf";

    addField<DAS_BIND_MANAGED_FIELD(ctPos)>("ctPos");

    addProperty<DAS_BIND_MANAGED_PROP(isAdditive)>("isAdditive");
    addProperty<DAS_BIND_MANAGED_PROP(isAnimationIgnored)>("isAnimationIgnored");
  }
};

struct AnimBlendNodeSingleLeafAnnotation : das::ManagedStructureAnnotation<AnimV20::AnimBlendNodeSingleLeaf, false>
{
  AnimBlendNodeSingleLeafAnnotation(das::ModuleLibrary &ml) : ManagedStructureAnnotation("AnimBlendNodeSingleLeaf", ml)
  {
    cppName = " ::AnimV20::AnimBlendNodeSingleLeaf";

    addField<DAS_BIND_MANAGED_FIELD(paramId)>("paramId");
    addField<DAS_BIND_MANAGED_FIELD(t0)>("t0");
    addField<DAS_BIND_MANAGED_FIELD(dt)>("dt");
    addField<DAS_BIND_MANAGED_FIELD(rate)>("rate");
    addField<DAS_BIND_MANAGED_FIELD(duration)>("duration");
    addField<DAS_BIND_MANAGED_FIELD(syncTime)>("syncTime");
    addField<DAS_BIND_MANAGED_FIELD(avgSpeed)>("avgSpeed");
    addField<DAS_BIND_MANAGED_FIELD(startOffsetEnabled)>("startOffsetEnabled");
    addField<DAS_BIND_MANAGED_FIELD(eoaIrq)>("eoaIrq");
    addField<DAS_BIND_MANAGED_FIELD(canRewind)>("canRewind");

    addProperty<DAS_BIND_MANAGED_PROP(isAdditive)>("isAdditive");
    addProperty<DAS_BIND_MANAGED_PROP(isAnimationIgnored)>("isAnimationIgnored");
  }
};

struct AnimBlendCtrl_1axisAnnotation : das::ManagedStructureAnnotation<AnimV20::AnimBlendCtrl_1axis, false>
{
  AnimBlendCtrl_1axisAnnotation(das::ModuleLibrary &ml) : ManagedStructureAnnotation("AnimBlendCtrl_1axis", ml)
  {
    cppName = " ::AnimV20::AnimBlendCtrl_1axis";
  }
};

struct AnimBlendCtrl_Fifo3Annotation : das::ManagedStructureAnnotation<AnimV20::AnimBlendCtrl_Fifo3, false>
{
  AnimBlendCtrl_Fifo3Annotation(das::ModuleLibrary &ml) : ManagedStructureAnnotation("AnimBlendCtrl_Fifo3", ml)
  {
    cppName = " ::AnimV20::AnimBlendCtrl_Fifo3";
  }
};

struct AnimBlendCtrl_RandomSwitcherAnnotation : das::ManagedStructureAnnotation<AnimV20::AnimBlendCtrl_RandomSwitcher, false>
{
  AnimBlendCtrl_RandomSwitcherAnnotation(das::ModuleLibrary &ml) : ManagedStructureAnnotation("AnimBlendCtrl_RandomSwitcher", ml)
  {
    cppName = " ::AnimV20::AnimBlendCtrl_RandomSwitcher";
  }
};

struct AnimBlendCtrl_HubAnnotation : das::ManagedStructureAnnotation<AnimV20::AnimBlendCtrl_Hub, false>
{
  AnimBlendCtrl_HubAnnotation(das::ModuleLibrary &ml) : ManagedStructureAnnotation("AnimBlendCtrl_Hub", ml)
  {
    cppName = " ::AnimV20::AnimBlendCtrl_Hub";
  }
};

struct AnimBlendCtrl_BlenderAnnotation : das::ManagedStructureAnnotation<AnimV20::AnimBlendCtrl_Blender, false>
{
  AnimBlendCtrl_BlenderAnnotation(das::ModuleLibrary &ml) : ManagedStructureAnnotation("AnimBlendCtrl_Blender", ml)
  {
    cppName = " ::AnimV20::AnimBlendCtrl_Blender";

    addProperty<DAS_BIND_MANAGED_PROP(getBlendTime)>("blendTime", "getBlendTime");
  }
};

struct AnimBlendCtrl_BinaryIndirectSwitchAnnotation
  : das::ManagedStructureAnnotation<AnimV20::AnimBlendCtrl_BinaryIndirectSwitch, false>
{
  AnimBlendCtrl_BinaryIndirectSwitchAnnotation(das::ModuleLibrary &ml) :
    ManagedStructureAnnotation("AnimBlendCtrl_BinaryIndirectSwitch", ml)
  {
    cppName = " ::AnimV20::AnimBlendCtrl_BinaryIndirectSwitch";
  }
};

struct AnimBlendCtrl_SetMotionMatchingTagAnnotation
  : das::ManagedStructureAnnotation<AnimV20::AnimBlendCtrl_SetMotionMatchingTag, false>
{
  AnimBlendCtrl_SetMotionMatchingTagAnnotation(das::ModuleLibrary &ml) :
    ManagedStructureAnnotation("AnimBlendCtrl_SetMotionMatchingTag", ml)
  {
    cppName = " ::AnimV20::AnimBlendCtrl_SetMotionMatchingTag";

    addProperty<DAS_BIND_MANAGED_PROP(getTagName)>("tagName", "getTagName");
  }
};

struct AnimBlendCtrl_LinearPolyAnnotation : das::ManagedStructureAnnotation<AnimV20::AnimBlendCtrl_LinearPoly, false>
{
  AnimBlendCtrl_LinearPolyAnnotation(das::ModuleLibrary &ml) : ManagedStructureAnnotation("AnimBlendCtrl_LinearPoly", ml)
  {
    cppName = " ::AnimV20::AnimBlendCtrl_LinearPoly";
  }
};

struct AnimBlendNodeParametricLeafAnnotation : das::ManagedStructureAnnotation<AnimV20::AnimBlendNodeParametricLeaf, false>
{
  AnimBlendNodeParametricLeafAnnotation(das::ModuleLibrary &ml) : ManagedStructureAnnotation("AnimBlendNodeParametricLeaf", ml)
  {
    cppName = " ::AnimV20::AnimBlendNodeParametricLeaf";

    addField<DAS_BIND_MANAGED_FIELD(paramId)>("paramId");
    addField<DAS_BIND_MANAGED_FIELD(paramId)>("paramLastId");
    addField<DAS_BIND_MANAGED_FIELD(t0)>("t0");
    addField<DAS_BIND_MANAGED_FIELD(dt)>("dt");
    addField<DAS_BIND_MANAGED_FIELD(p0)>("p0");
    addField<DAS_BIND_MANAGED_FIELD(dp)>("dp");
    addField<DAS_BIND_MANAGED_FIELD(paramMulK)>("paramMulK");
    addField<DAS_BIND_MANAGED_FIELD(paramAddK)>("paramAddK");
    addField<DAS_BIND_MANAGED_FIELD(looping)>("looping");
    addField<DAS_BIND_MANAGED_FIELD(updateOnParamChanged)>("updateOnParamChanged");
  }
};

struct AnimBlendCtrl_ParametricSwitcherAnnotation : das::ManagedStructureAnnotation<AnimV20::AnimBlendCtrl_ParametricSwitcher, false>
{
  AnimBlendCtrl_ParametricSwitcherAnnotation(das::ModuleLibrary &ml) :
    ManagedStructureAnnotation("AnimBlendCtrl_ParametricSwitcher", ml)
  {
    cppName = " ::AnimV20::AnimBlendCtrl_ParametricSwitcher";

    addField<DAS_BIND_MANAGED_FIELD(morphTime)>("morphTime");
    addField<DAS_BIND_MANAGED_FIELD(paramId)>("paramId");
    addField<DAS_BIND_MANAGED_FIELD(residualParamId)>("residualParamId");
    addField<DAS_BIND_MANAGED_FIELD(lastAnimParamId)>("lastAnimParamId");
    addField<DAS_BIND_MANAGED_FIELD(fifoParamId)>("fifoParamId");
    addField<DAS_BIND_MANAGED_FIELD(continuousAnimMode)>("continuousAnimMode");
  }
};

struct AnimFifo3QueueAnnotation : das::ManagedStructureAnnotation<AnimV20::AnimFifo3Queue, false>
{
  AnimFifo3QueueAnnotation(das::ModuleLibrary &ml) : ManagedStructureAnnotation("AnimFifo3Queue", ml)
  {
    cppName = " ::AnimV20::AnimFifo3Queue";
    addField<DAS_BIND_MANAGED_FIELD(node)>("node");
    addField<DAS_BIND_MANAGED_FIELD(morphTime)>("morphTime");
    addField<DAS_BIND_MANAGED_FIELD(state)>("state");
    addField<DAS_BIND_MANAGED_FIELD(t0)>("t0");
  }
};

struct AnimBlendCtrl_1axisAnimSliceAnnotation : das::ManagedStructureAnnotation<AnimV20::AnimBlendCtrl_1axis::AnimSlice, false>
{
  AnimBlendCtrl_1axisAnimSliceAnnotation(das::ModuleLibrary &ml) : ManagedStructureAnnotation("AnimBlendCtrl_1axisAnimSlice", ml)
  {
    cppName = " AnimV20::AnimBlendCtrl_1axis::AnimSlice";
    addField<DAS_BIND_MANAGED_FIELD(start)>("start");
    addField<DAS_BIND_MANAGED_FIELD(end)>("end");
    addField<DAS_BIND_MANAGED_FIELD(node)>("node");
  }
  bool canBePlacedInContainer() const override { return true; } // To pass array of AnimSlices
};

struct IAnimStateHolderAnnotation : das::ManagedStructureAnnotation<AnimV20::IAnimStateHolder, false>
{
  IAnimStateHolderAnnotation(das::ModuleLibrary &ml) : ManagedStructureAnnotation("IAnimStateHolder", ml)
  {
    cppName = " ::AnimV20::IAnimStateHolder";

    addProperty<DAS_BIND_MANAGED_PROP(getSize)>("size", "getSize");
  }
};

struct AnimDataAnnotation : das::ManagedStructureAnnotation<::AnimV20::AnimData, false>
{
  AnimDataAnnotation(das::ModuleLibrary &ml) : ManagedStructureAnnotation("AnimData", ml)
  {
    cppName = " ::AnimV20::AnimData";

    addProperty<DAS_BIND_MANAGED_PROP(isAdditive)>("isAdditive");
    addField<DAS_BIND_MANAGED_FIELD(resId)>("resId");
  }
};

struct AnimBlendCtrl_ParametricSwitcherItemAnimAnnotation
  : das::ManagedStructureAnnotation<::AnimV20::AnimBlendCtrl_ParametricSwitcher::ItemAnim, false>
{
  AnimBlendCtrl_ParametricSwitcherItemAnimAnnotation(das::ModuleLibrary &ml) :
    ManagedStructureAnnotation("AnimBlendCtrl_ParametricSwitcherItemAnim", ml)
  {
    cppName = " ::AnimV20::AnimBlendCtrl_ParametricSwitcher::ItemAnim";

    addField<DAS_BIND_MANAGED_FIELD(baseVal)>("baseVal");
    addField<DAS_BIND_MANAGED_FIELD(node)>("node");
  }

  bool canBePlacedInContainer() const override { return true; } // To pass array of ItemAnims
};

struct AnimBlendCtrl_RandomSwitcherRandomAnimAnnotation
  : das::ManagedStructureAnnotation<::AnimV20::AnimBlendCtrl_RandomSwitcher::RandomAnim, false>
{
  AnimBlendCtrl_RandomSwitcherRandomAnimAnnotation(das::ModuleLibrary &ml) :
    ManagedStructureAnnotation("AnimBlendCtrl_RandomSwitcherRandomAnim", ml)
  {
    cppName = " ::AnimV20::AnimBlendCtrl_RandomSwitcher::RandomAnim";

    addField<DAS_BIND_MANAGED_FIELD(maxRepeat)>("maxRepeat");
    addField<DAS_BIND_MANAGED_FIELD(rndWt)>("rndWt");
    addField<DAS_BIND_MANAGED_FIELD(node)>("node");
  }

  bool canBePlacedInContainer() const override { return true; } // To pass array of ItemAnims
};

struct AnimBlendCtrl_LinearPolyAnimPointAnnotation
  : das::ManagedStructureAnnotation<::AnimV20::AnimBlendCtrl_LinearPoly::AnimPoint, false>
{
  AnimBlendCtrl_LinearPolyAnimPointAnnotation(das::ModuleLibrary &ml) :
    ManagedStructureAnnotation("AnimBlendCtrl_LinearPolyAnimPoint", ml)
  {
    cppName = " ::AnimV20::AnimBlendCtrl_LinearPoly::AnimPoint";

    addField<DAS_BIND_MANAGED_FIELD(p0)>("p0");
    addField<DAS_BIND_MANAGED_FIELD(wtPid)>("wtPid");
    addField<DAS_BIND_MANAGED_FIELD(node)>("node");
  }

  bool canBePlacedInContainer() const override { return true; } // To pass array of RandomAnims
};

struct AnimationGraphStateRecAnnotation : das::ManagedStructureAnnotation<::AnimV20::AnimationGraph::StateRec, false>
{
  AnimationGraphStateRecAnnotation(das::ModuleLibrary &ml) : ManagedStructureAnnotation("AnimationGraphStateRec", ml)
  {
    cppName = " ::AnimV20::AnimationGraph::StateRec";

    addField<DAS_BIND_MANAGED_FIELD(nodeId)>("nodeId");
    addField<DAS_BIND_MANAGED_FIELD(morphTime)>("morphTime");
    addField<DAS_BIND_MANAGED_FIELD(forcedStateDur)>("forcedStateDur");
    addField<DAS_BIND_MANAGED_FIELD(forcedStateSpd)>("forcedStateSpd");
    addField<DAS_BIND_MANAGED_FIELD(minTimeScale)>("minTimeScale");
    addField<DAS_BIND_MANAGED_FIELD(maxTimeScale)>("maxTimeScale");
  }

  bool canBePlacedInContainer() const override { return true; } // To pass array of StateRecs
};

struct AnimPostBlendParamFromNodeLocalDataAnnotation
  : das::ManagedStructureAnnotation<::AnimV20::AnimPostBlendParamFromNode::LocalData, false>
{
  AnimPostBlendParamFromNodeLocalDataAnnotation(das::ModuleLibrary &ml) :
    ManagedStructureAnnotation("AnimPostBlendParamFromNodeLocalData", ml)
  {
    cppName = " ::AnimV20::AnimPostBlendParamFromNode::LocalData";
    addField<DAS_BIND_MANAGED_FIELD(lastRefUid)>("lastRefUid");
  }
};

struct AnimPostBlendParamFromNodeAnnotation : das::ManagedStructureAnnotation<AnimV20::AnimPostBlendParamFromNode, false>
{
  AnimPostBlendParamFromNodeAnnotation(das::ModuleLibrary &ml) : ManagedStructureAnnotation("AnimPostBlendParamFromNode", ml)
  {
    cppName = " ::AnimV20::AnimPostBlendParamFromNode";
    addField<DAS_BIND_MANAGED_FIELD(varId)>("varId");
    addField<DAS_BIND_MANAGED_FIELD(slotId)>("slotId");
    addField<DAS_BIND_MANAGED_FIELD(destVarId)>("destVarId");
    addField<DAS_BIND_MANAGED_FIELD(destVarWtId)>("destVarWtId");
    addField<DAS_BIND_MANAGED_FIELD(defVal)>("defVal");
    addFieldEx("nodeName", "nodeName", offsetof(AnimV20::AnimPostBlendParamFromNode, nodeName), das::makeType<char *>(ml));
    addField<DAS_BIND_MANAGED_FIELD(invertVal)>("invertVal");
  }
};

struct AttachGeomNodeCtrlVarIdAnnotation : das::ManagedStructureAnnotation<::AnimV20::AttachGeomNodeCtrl::VarId, false>
{
  AttachGeomNodeCtrlVarIdAnnotation(das::ModuleLibrary &ml) : ManagedStructureAnnotation("AttachGeomNodeCtrlVarId", ml)
  {
    cppName = " ::AnimV20::AttachGeomNodeCtrl::VarId";
    addField<DAS_BIND_MANAGED_FIELD(nodeWtm)>("nodeWtm");
    addField<DAS_BIND_MANAGED_FIELD(wScale)>("wScale");
    addField<DAS_BIND_MANAGED_FIELD(wScaleInverted)>("wScaleInverted");
  }
};

struct AttachGeomNodeCtrlAttachDescAnnotation : das::ManagedStructureAnnotation<::AnimV20::AttachGeomNodeCtrl::AttachDesc, false>
{
  AttachGeomNodeCtrlAttachDescAnnotation(das::ModuleLibrary &ml) : ManagedStructureAnnotation("AttachGeomNodeCtrlAttachDesc", ml)
  {
    cppName = " ::AnimV20::AttachGeomNodeCtrl::AttachDesc";
    addField<DAS_BIND_MANAGED_FIELD(wtm)>("wtm");
    addField<DAS_BIND_MANAGED_FIELD(w)>("w");
  }
};

struct AttachGeomNodeCtrlAnnotation : das::ManagedStructureAnnotation<AnimV20::AttachGeomNodeCtrl, false>
{
  AttachGeomNodeCtrlAnnotation(das::ModuleLibrary &ml) : ManagedStructureAnnotation("AttachGeomNodeCtrl", ml)
  {
    cppName = " ::AnimV20::AttachGeomNodeCtrl";
    addFieldEx("minNodeScale", "minNodeScale", offsetof(AnimV20::AttachGeomNodeCtrl, minNodeScale), das::makeType<das::float3>(ml));
    addFieldEx("maxNodeScale", "maxNodeScale", offsetof(AnimV20::AttachGeomNodeCtrl, maxNodeScale), das::makeType<das::float3>(ml));
    addField<DAS_BIND_MANAGED_FIELD(destVarId)>("destVarId");
    addField<DAS_BIND_MANAGED_FIELD(perAnimStateDataVarId)>("perAnimStateDataVarId");
  }
};

struct AnimPostBlendAlignCtrlAnnotation : das::ManagedStructureAnnotation<AnimV20::AnimPostBlendAlignCtrl, false>
{
  AnimPostBlendAlignCtrlAnnotation(das::ModuleLibrary &ml) : ManagedStructureAnnotation("AnimPostBlendAlignCtrl", ml)
  {
    cppName = " ::AnimV20::AnimPostBlendAlignCtrl";
    addFieldEx("targetNodeName", "targetNodeName", offsetof(AnimV20::AnimPostBlendAlignCtrl, targetNodeName),
      das::makeType<char *>(ml));
    addFieldEx("srcNodeName", "srcNodeName", offsetof(AnimV20::AnimPostBlendAlignCtrl, srcNodeName), das::makeType<char *>(ml));
    addField<DAS_BIND_MANAGED_FIELD(srcSlotId)>("srcSlotId");
    addField<DAS_BIND_MANAGED_FIELD(paramId)>("paramId");
    addField<DAS_BIND_MANAGED_FIELD(wScaleVarId)>("wScaleVarId");
    addField<DAS_BIND_MANAGED_FIELD(wScaleInverted)>("wScaleInverted");
    addField<DAS_BIND_MANAGED_FIELD(useLocal)>("useLocal");
    addField<DAS_BIND_MANAGED_FIELD(binaryWt)>("binaryWt");
    addField<DAS_BIND_MANAGED_FIELD(copyPos)>("copyPos");
    addField<DAS_BIND_MANAGED_FIELD(copyBlendResultToSrc)>("copyBlendResultToSrc");
  }
};

struct AnimPostBlendAlignExCtrlAnnotation : das::ManagedStructureAnnotation<AnimV20::AnimPostBlendAlignExCtrl, false>
{
  AnimPostBlendAlignExCtrlAnnotation(das::ModuleLibrary &ml) : ManagedStructureAnnotation("AnimPostBlendAlignExCtrl", ml)
  {
    cppName = " ::AnimV20::AnimPostBlendAlignExCtrl";
    addFieldEx("hlpNodeName", "hlpNodeName", offsetof(AnimV20::AnimPostBlendAlignExCtrl, hlpNodeName), das::makeType<char *>(ml));
    addField<DAS_BIND_MANAGED_FIELD(varId)>("varId");
  }
};

struct AnimPostBlendRotateCtrlAnnotation : das::ManagedStructureAnnotation<AnimV20::AnimPostBlendRotateCtrl, false>
{
  AnimPostBlendRotateCtrlAnnotation(das::ModuleLibrary &ml) : ManagedStructureAnnotation("AnimPostBlendRotateCtrl", ml)
  {
    cppName = " ::AnimV20::AnimPostBlendRotateCtrl";
    addField<DAS_BIND_MANAGED_FIELD(kMul)>("kMul");
    addField<DAS_BIND_MANAGED_FIELD(kAdd)>("kAdd");
    addField<DAS_BIND_MANAGED_FIELD(kCourseAdd)>("kCourseAdd");
    addField<DAS_BIND_MANAGED_FIELD(varId)>("varId");
    addField<DAS_BIND_MANAGED_FIELD(paramId)>("paramId");
    addField<DAS_BIND_MANAGED_FIELD(axisCourseParamId)>("axisCourseParamId");
    addField<DAS_BIND_MANAGED_FIELD(kMul2)>("kMul2");
    addField<DAS_BIND_MANAGED_FIELD(kAdd2)>("kAdd2");
    addField<DAS_BIND_MANAGED_FIELD(addParam2Id)>("addParam2Id");
    addField<DAS_BIND_MANAGED_FIELD(addParam3Id)>("addParam3Id");
    addField<DAS_BIND_MANAGED_FIELD(leftTm)>("leftTm");
    addField<DAS_BIND_MANAGED_FIELD(swapYZ)>("swapYZ");
    addField<DAS_BIND_MANAGED_FIELD(relRot)>("relRot");
    addField<DAS_BIND_MANAGED_FIELD(saveScale)>("saveScale");
    addField<DAS_BIND_MANAGED_FIELD(axisFromCharIndex)>("axisFromCharIndex");
  }
};

struct AnimPostBlendRotateAroundCtrlAnnotation : das::ManagedStructureAnnotation<AnimV20::AnimPostBlendRotateAroundCtrl, false>
{
  AnimPostBlendRotateAroundCtrlAnnotation(das::ModuleLibrary &ml) : ManagedStructureAnnotation("AnimPostBlendRotateAroundCtrl", ml)
  {
    cppName = " ::AnimV20::AnimPostBlendRotateAroundCtrl";
    addField<DAS_BIND_MANAGED_FIELD(kMul)>("kMul");
    addField<DAS_BIND_MANAGED_FIELD(kAdd)>("kAdd");
    addField<DAS_BIND_MANAGED_FIELD(varId)>("varId");
    addField<DAS_BIND_MANAGED_FIELD(paramId)>("paramId");
  }
};

struct AnimPostBlendScaleCtrlAnnotation : das::ManagedStructureAnnotation<AnimV20::AnimPostBlendScaleCtrl, false>
{
  AnimPostBlendScaleCtrlAnnotation(das::ModuleLibrary &ml) : ManagedStructureAnnotation("AnimPostBlendScaleCtrl", ml)
  {
    cppName = " ::AnimV20::AnimPostBlendScaleCtrl";
    addField<DAS_BIND_MANAGED_FIELD(varId)>("varId");
    addField<DAS_BIND_MANAGED_FIELD(paramId)>("paramId");
    addField<DAS_BIND_MANAGED_FIELD(defaultValue)>("defaultValue");
  }
};

struct AnimPostBlendMoveCtrlAnnotation : das::ManagedStructureAnnotation<AnimV20::AnimPostBlendMoveCtrl, false>
{
  AnimPostBlendMoveCtrlAnnotation(das::ModuleLibrary &ml) : ManagedStructureAnnotation("AnimPostBlendMoveCtrl", ml)
  {
    cppName = " ::AnimV20::AnimPostBlendMoveCtrl";
    addField<DAS_BIND_MANAGED_FIELD(kMul)>("kMul");
    addField<DAS_BIND_MANAGED_FIELD(kAdd)>("kAdd");
    addField<DAS_BIND_MANAGED_FIELD(kCourseAdd)>("kCourseAdd");
    addField<DAS_BIND_MANAGED_FIELD(varId)>("varId");
    addField<DAS_BIND_MANAGED_FIELD(paramId)>("paramId");
    addField<DAS_BIND_MANAGED_FIELD(axisCourseParamId)>("axisCourseParamId");
    addField<DAS_BIND_MANAGED_FIELD(additive)>("additive");
    addField<DAS_BIND_MANAGED_FIELD(saveOtherAxisMove)>("saveOtherAxisMove");
  }
};

struct AnimPostBlendCondHideCtrlAnnotation : das::ManagedStructureAnnotation<AnimV20::AnimPostBlendCondHideCtrl, false>
{
  AnimPostBlendCondHideCtrlAnnotation(das::ModuleLibrary &ml) : ManagedStructureAnnotation("AnimPostBlendCondHideCtrl", ml)
  {
    cppName = " ::AnimV20::AnimPostBlendCondHideCtrl";
    addField<DAS_BIND_MANAGED_FIELD(varId)>("varId");
  }
};

struct AnimPostBlendAimCtrlAnnotation : das::ManagedStructureAnnotation<AnimV20::AnimPostBlendAimCtrl, false>
{
  AnimPostBlendAimCtrlAnnotation(das::ModuleLibrary &ml) : ManagedStructureAnnotation("AnimPostBlendAimCtrl", ml)
  {
    cppName = " ::AnimV20::AnimPostBlendAimCtrl";
    addField<DAS_BIND_MANAGED_FIELD(varId)>("varId");
    addField<DAS_BIND_MANAGED_FIELD(curYawId)>("curYawId");
    addField<DAS_BIND_MANAGED_FIELD(curPitchId)>("curPitchId");
    addField<DAS_BIND_MANAGED_FIELD(curWorldYawId)>("curWorldYawId");
    addField<DAS_BIND_MANAGED_FIELD(curWorldPitchId)>("curWorldPitchId");
    addField<DAS_BIND_MANAGED_FIELD(targetYawId)>("targetYawId");
    addField<DAS_BIND_MANAGED_FIELD(targetPitchId)>("targetPitchId");
    addField<DAS_BIND_MANAGED_FIELD(yawSpeedId)>("yawSpeedId");
    addField<DAS_BIND_MANAGED_FIELD(pitchSpeedId)>("pitchSpeedId");
    addField<DAS_BIND_MANAGED_FIELD(yawSpeedMulId)>("yawSpeedMulId");
    addField<DAS_BIND_MANAGED_FIELD(pitchSpeedMulId)>("pitchSpeedMulId");
    addField<DAS_BIND_MANAGED_FIELD(yawAccelId)>("yawAccelId");
    addField<DAS_BIND_MANAGED_FIELD(pitchAccelId)>("pitchAccelId");
    addField<DAS_BIND_MANAGED_FIELD(prevYawId)>("prevYawId");
    addField<DAS_BIND_MANAGED_FIELD(prevPitchId)>("prevPitchId");
    addField<DAS_BIND_MANAGED_FIELD(hasStabId)>("hasStabId");
    addField<DAS_BIND_MANAGED_FIELD(stabYawId)>("stabYawId");
    addField<DAS_BIND_MANAGED_FIELD(stabPitchId)>("stabPitchId");
    addField<DAS_BIND_MANAGED_FIELD(stabErrorId)>("stabErrorId");
    addField<DAS_BIND_MANAGED_FIELD(stabYawMultId)>("stabYawMultId");
    addField<DAS_BIND_MANAGED_FIELD(stabPitchMultId)>("stabPitchMultId");
  }
};

struct ApbParamCtrlAnnotation : das::ManagedStructureAnnotation<AnimV20::ApbParamCtrl, false>
{
  ApbParamCtrlAnnotation(das::ModuleLibrary &ml) : ManagedStructureAnnotation("ApbParamCtrl", ml)
  {
    cppName = " ::AnimV20::ApbParamCtrl";
    addField<DAS_BIND_MANAGED_FIELD(wtPid)>("wtPid");
  }
};

struct DefClampParamCtrlAnnotation : das::ManagedStructureAnnotation<AnimV20::DefClampParamCtrl, false>
{
  DefClampParamCtrlAnnotation(das::ModuleLibrary &ml) : ManagedStructureAnnotation("DefClampParamCtrl", ml)
  {
    cppName = " ::AnimV20::DefClampParamCtrl";
    addField<DAS_BIND_MANAGED_FIELD(wtPid)>("wtPid");
  }
};

struct ApbAnimateCtrlAnnotation : das::ManagedStructureAnnotation<AnimV20::ApbAnimateCtrl, false>
{
  ApbAnimateCtrlAnnotation(das::ModuleLibrary &ml) : ManagedStructureAnnotation("ApbAnimateCtrl", ml)
  {
    cppName = " ::AnimV20::ApbAnimateCtrl";
    addField<DAS_BIND_MANAGED_FIELD(varId)>("varId");
  }
};

struct LegsIKCtrlAnnotation : das::ManagedStructureAnnotation<AnimV20::LegsIKCtrl, false>
{
  LegsIKCtrlAnnotation(das::ModuleLibrary &ml) : ManagedStructureAnnotation("LegsIKCtrl", ml)
  {
    cppName = " ::AnimV20::LegsIKCtrl";
    addField<DAS_BIND_MANAGED_FIELD(varId)>("varId");
    addField<DAS_BIND_MANAGED_FIELD(crawlKneeOffsetVec)>("crawlKneeOffsetVec");
    addField<DAS_BIND_MANAGED_FIELD(crawlFootOffset)>("crawlFootOffset");
    addField<DAS_BIND_MANAGED_FIELD(crawlFootAngle)>("crawlFootAngle");
    addField<DAS_BIND_MANAGED_FIELD(crawlMaxRay)>("crawlMaxRay");
    addField<DAS_BIND_MANAGED_FIELD(alwaysSolve)>("alwaysSolve");
    addField<DAS_BIND_MANAGED_FIELD(isCrawl)>("isCrawl");
  }
};

struct MultiChainFABRIKCtrlAnnotation : das::ManagedStructureAnnotation<AnimV20::MultiChainFABRIKCtrl, false>
{
  MultiChainFABRIKCtrlAnnotation(das::ModuleLibrary &ml) : ManagedStructureAnnotation("MultiChainFABRIKCtrl", ml)
  {
    cppName = " ::AnimV20::MultiChainFABRIKCtrl";
    addField<DAS_BIND_MANAGED_FIELD(bodyChainEffectorVarId)>("bodyChainEffectorVarId");
    addField<DAS_BIND_MANAGED_FIELD(bodyChainEffectorEnd)>("bodyChainEffectorEnd");
    addField<DAS_BIND_MANAGED_FIELD(perAnimStateDataVarId)>("perAnimStateDataVarId");
  }
};

struct AnimPostBlendNodeLookatCtrlAnnotation : das::ManagedStructureAnnotation<AnimV20::AnimPostBlendNodeLookatCtrl, false>
{
  AnimPostBlendNodeLookatCtrlAnnotation(das::ModuleLibrary &ml) : ManagedStructureAnnotation("AnimPostBlendNodeLookatCtrl", ml)
  {
    cppName = " ::AnimV20::AnimPostBlendNodeLookatCtrl";
    addField<DAS_BIND_MANAGED_FIELD(varId)>("varId");
    addField<DAS_BIND_MANAGED_FIELD(paramXId)>("paramXId");
    addField<DAS_BIND_MANAGED_FIELD(paramYId)>("paramYId");
    addField<DAS_BIND_MANAGED_FIELD(paramZId)>("paramZId");
    addField<DAS_BIND_MANAGED_FIELD(leftTm)>("leftTm");
    addField<DAS_BIND_MANAGED_FIELD(swapYZ)>("swapYZ");
  }
};

struct AnimPostBlendNodeLookatNodeCtrlAnnotation : das::ManagedStructureAnnotation<AnimV20::AnimPostBlendNodeLookatNodeCtrl, false>
{
  AnimPostBlendNodeLookatNodeCtrlAnnotation(das::ModuleLibrary &ml) : ManagedStructureAnnotation("AnimPostBlendNodeLookatNodeCtrl", ml)
  {
    cppName = " ::AnimV20::AnimPostBlendNodeLookatNodeCtrl";
    addFieldEx("targetNode", "targetNode", offsetof(AnimV20::AnimPostBlendNodeLookatNodeCtrl, targetNode), das::makeType<char *>(ml));
    addFieldEx("lookatNodeName", "lookatNodeName", offsetof(AnimV20::AnimPostBlendNodeLookatNodeCtrl, lookatNodeName),
      das::makeType<char *>(ml));
    addFieldEx("upNodeName", "upNodeName", offsetof(AnimV20::AnimPostBlendNodeLookatNodeCtrl, upNodeName), das::makeType<char *>(ml));
    addField<DAS_BIND_MANAGED_FIELD(lookatAxis)>("lookatAxis");
    addField<DAS_BIND_MANAGED_FIELD(upAxis)>("upAxis");
    addField<DAS_BIND_MANAGED_FIELD(varId)>("varId");
    addField<DAS_BIND_MANAGED_FIELD(valid)>("valid");
    addField<DAS_BIND_MANAGED_FIELD(upVector)>("upVector");
  }
};

struct AnimPostBlendEffFromAttachementAnnotation : das::ManagedStructureAnnotation<AnimV20::AnimPostBlendEffFromAttachement, false>
{
  AnimPostBlendEffFromAttachementAnnotation(das::ModuleLibrary &ml) : ManagedStructureAnnotation("AnimPostBlendEffFromAttachement", ml)
  {
    cppName = " ::AnimV20::AnimPostBlendEffFromAttachement";
    addField<DAS_BIND_MANAGED_FIELD(varId)>("varId");
    addField<DAS_BIND_MANAGED_FIELD(namedSlotId)>("namedSlotId");
    addField<DAS_BIND_MANAGED_FIELD(varSlotId)>("varSlotId");
    addField<DAS_BIND_MANAGED_FIELD(ignoreZeroWt)>("ignoreZeroWt");
  }
};

struct AnimPostBlendNodeEffectorFromChildIKAnnotation
  : das::ManagedStructureAnnotation<AnimV20::AnimPostBlendNodeEffectorFromChildIK, false>
{
  AnimPostBlendNodeEffectorFromChildIKAnnotation(das::ModuleLibrary &ml) :
    ManagedStructureAnnotation("AnimPostBlendNodeEffectorFromChildIK", ml)
  {
    cppName = " ::AnimV20::AnimPostBlendNodeEffectorFromChildIK";
    addField<DAS_BIND_MANAGED_FIELD(varSlotId)>("varSlotId");
    addField<DAS_BIND_MANAGED_FIELD(localDataVarId)>("localDataVarId");
    addField<DAS_BIND_MANAGED_FIELD(resetEffByValId)>("resetEffByValId");
    addField<DAS_BIND_MANAGED_FIELD(resetEffInvVal)>("resetEffInvVal");
    addFieldEx("parentNodeName", "parentNodeName", offsetof(AnimV20::AnimPostBlendNodeEffectorFromChildIK, parentNodeName),
      das::makeType<char *>(ml));
    addFieldEx("childNodeName", "childNodeName", offsetof(AnimV20::AnimPostBlendNodeEffectorFromChildIK, childNodeName),
      das::makeType<char *>(ml));
    addFieldEx("srcVarName", "srcVarName", offsetof(AnimV20::AnimPostBlendNodeEffectorFromChildIK, srcVarName),
      das::makeType<char *>(ml));
    addFieldEx("destVarName", "destVarName", offsetof(AnimV20::AnimPostBlendNodeEffectorFromChildIK, destVarName),
      das::makeType<char *>(ml));
  }
};

struct AnimPostBlendMatVarFromNodeAnnotation : das::ManagedStructureAnnotation<AnimV20::AnimPostBlendMatVarFromNode, false>
{
  AnimPostBlendMatVarFromNodeAnnotation(das::ModuleLibrary &ml) : ManagedStructureAnnotation("AnimPostBlendMatVarFromNode", ml)
  {
    cppName = " ::AnimV20::AnimPostBlendMatVarFromNode";
    addField<DAS_BIND_MANAGED_FIELD(varId)>("varId");
    addField<DAS_BIND_MANAGED_FIELD(destSlotId)>("destSlotId");
    addField<DAS_BIND_MANAGED_FIELD(srcSlotId)>("srcSlotId");
    addField<DAS_BIND_MANAGED_FIELD(destVarWtId)>("destVarWtId");
    addFieldEx("destVarName", "destVarName", offsetof(AnimV20::AnimPostBlendMatVarFromNode, destVarName), das::makeType<char *>(ml));
    addFieldEx("srcNodeName", "srcNodeName", offsetof(AnimV20::AnimPostBlendMatVarFromNode, srcNodeName), das::makeType<char *>(ml));
  }
};

struct AnimPostBlendNodesFromAttachementAnnotation : das::ManagedStructureAnnotation<AnimV20::AnimPostBlendNodesFromAttachement, false>
{
  AnimPostBlendNodesFromAttachementAnnotation(das::ModuleLibrary &ml) :
    ManagedStructureAnnotation("AnimPostBlendNodesFromAttachement", ml)
  {
    cppName = " ::AnimV20::AnimPostBlendNodesFromAttachement";
    addField<DAS_BIND_MANAGED_FIELD(varId)>("varId");
    addField<DAS_BIND_MANAGED_FIELD(namedSlotId)>("namedSlotId");
    addField<DAS_BIND_MANAGED_FIELD(varSlotId)>("varSlotId");
    addField<DAS_BIND_MANAGED_FIELD(copyWtm)>("copyWtm");
    addField<DAS_BIND_MANAGED_FIELD(wScaleInverted)>("wScaleInverted");
    addField<DAS_BIND_MANAGED_FIELD(wScaleVarId)>("wScaleVarId");
  }
};

struct AnimPostBlendCompoundRotateShiftAnnotation : das::ManagedStructureAnnotation<AnimV20::AnimPostBlendCompoundRotateShift, false>
{
  AnimPostBlendCompoundRotateShiftAnnotation(das::ModuleLibrary &ml) :
    ManagedStructureAnnotation("AnimPostBlendCompoundRotateShift", ml)
  {
    cppName = " ::AnimV20::AnimPostBlendCompoundRotateShift";
    addField<DAS_BIND_MANAGED_FIELD(localVarId)>("localVarId");
    addFieldEx("targetNode", "targetNode", offsetof(AnimV20::AnimPostBlendCompoundRotateShift, targetNode), das::makeType<char *>(ml));
    addFieldEx("alignAsNode", "alignAsNode", offsetof(AnimV20::AnimPostBlendCompoundRotateShift, alignAsNode),
      das::makeType<char *>(ml));
    addFieldEx("moveAlongNode", "moveAlongNode", offsetof(AnimV20::AnimPostBlendCompoundRotateShift, moveAlongNode),
      das::makeType<char *>(ml));
  }
};

struct AnimPostBlendSetParamAnnotation : das::ManagedStructureAnnotation<AnimV20::AnimPostBlendSetParam, false>
{
  AnimPostBlendSetParamAnnotation(das::ModuleLibrary &ml) : ManagedStructureAnnotation("AnimPostBlendSetParam", ml)
  {
    cppName = " ::AnimV20::AnimPostBlendSetParam";
    addField<DAS_BIND_MANAGED_FIELD(destSlotId)>("destSlotId");
    addField<DAS_BIND_MANAGED_FIELD(destVarId)>("destVarId");
    addField<DAS_BIND_MANAGED_FIELD(srcVarId)>("srcVarId");
    addFieldEx("destVarName", "destVarName", offsetof(AnimV20::AnimPostBlendSetParam, destVarName), das::makeType<char *>(ml));
    addField<DAS_BIND_MANAGED_FIELD(val)>("val");
    addField<DAS_BIND_MANAGED_FIELD(minWeight)>("minWeight");
    addField<DAS_BIND_MANAGED_FIELD(pullParamValueFromSlot)>("pullParamValueFromSlot");
  }
};

struct AnimPostBlendTwistCtrlAnnotation : das::ManagedStructureAnnotation<AnimV20::AnimPostBlendTwistCtrl, false>
{
  AnimPostBlendTwistCtrlAnnotation(das::ModuleLibrary &ml) : ManagedStructureAnnotation("AnimPostBlendTwistCtrl", ml)
  {
    cppName = " ::AnimV20::AnimPostBlendTwistCtrl";
    addFieldEx("node0", "node0", offsetof(AnimV20::AnimPostBlendTwistCtrl, node0), das::makeType<char *>(ml));
    addFieldEx("node1", "node1", offsetof(AnimV20::AnimPostBlendTwistCtrl, node1), das::makeType<char *>(ml));
    addField<DAS_BIND_MANAGED_FIELD(varId)>("varId");
    addField<DAS_BIND_MANAGED_FIELD(angDiff)>("angDiff");
    addField<DAS_BIND_MANAGED_FIELD(optional)>("optional");
  }
};

struct AnimPostBlendEyeCtrlAnnotation : das::ManagedStructureAnnotation<AnimV20::AnimPostBlendEyeCtrl, false>
{
  AnimPostBlendEyeCtrlAnnotation(das::ModuleLibrary &ml) : ManagedStructureAnnotation("AnimPostBlendEyeCtrl", ml)
  {
    cppName = " ::AnimV20::AnimPostBlendEyeCtrl";
    addField<DAS_BIND_MANAGED_FIELD(varId)>("varId");
    addField<DAS_BIND_MANAGED_FIELD(horizontalReactionFactor)>("horizontalReactionFactor");
    addField<DAS_BIND_MANAGED_FIELD(blinkingReactionFactor)>("blinkingReactionFactor");
    addField<DAS_BIND_MANAGED_FIELD(verticalReactionFactor)>("verticalReactionFactor");
    addFieldEx("eyeDirectionNodeName", "eyeDirectionNodeName", offsetof(AnimV20::AnimPostBlendEyeCtrl, eyeDirectionNodeName),
      das::makeType<char *>(ml));
    addFieldEx("eyelidStartTopNodeName", "eyelidStartTopNodeName", offsetof(AnimV20::AnimPostBlendEyeCtrl, eyelidStartTopNodeName),
      das::makeType<char *>(ml));
    addFieldEx("eyelidStartBottomNodeName", "eyelidStartBottomNodeName",
      offsetof(AnimV20::AnimPostBlendEyeCtrl, eyelidStartBottomNodeName), das::makeType<char *>(ml));
    addFieldEx("eyelidHorizontalReactionNodeName", "eyelidHorizontalReactionNodeName",
      offsetof(AnimV20::AnimPostBlendEyeCtrl, eyelidHorizontalReactionNodeName), das::makeType<char *>(ml));
    addFieldEx("eyelidVerticalTopReactionNodeName", "eyelidVerticalTopReactionNodeName",
      offsetof(AnimV20::AnimPostBlendEyeCtrl, eyelidVerticalTopReactionNodeName), das::makeType<char *>(ml));
    addFieldEx("eyelidVerticalBottomReactionNodeName", "eyelidVerticalBottomReactionNodeName",
      offsetof(AnimV20::AnimPostBlendEyeCtrl, eyelidVerticalBottomReactionNodeName), das::makeType<char *>(ml));
    addFieldEx("eyelidBlinkSourceNodeName", "eyelidBlinkSourceNodeName",
      offsetof(AnimV20::AnimPostBlendEyeCtrl, eyelidBlinkSourceNodeName), das::makeType<char *>(ml));
    addFieldEx("eyelidBlinkTopNodeName", "eyelidBlinkTopNodeName", offsetof(AnimV20::AnimPostBlendEyeCtrl, eyelidBlinkTopNodeName),
      das::makeType<char *>(ml));
    addFieldEx("eyelidBlinkBottomNodeName", "eyelidBlinkBottomNodeName",
      offsetof(AnimV20::AnimPostBlendEyeCtrl, eyelidBlinkBottomNodeName), das::makeType<char *>(ml));
  }
};

struct DeltaRotateShiftCtrlAnnotation : das::ManagedStructureAnnotation<AnimV20::DeltaRotateShiftCtrl, false>
{
  DeltaRotateShiftCtrlAnnotation(das::ModuleLibrary &ml) : ManagedStructureAnnotation("DeltaRotateShiftCtrl", ml)
  {
    cppName = " ::AnimV20::DeltaRotateShiftCtrl";
    addField<DAS_BIND_MANAGED_FIELD(localVarId)>("localVarId");
    addField<DAS_BIND_MANAGED_FIELD(relativeToOrig)>("relativeToOrig");
    addFieldEx("targetNode", "targetNode", offsetof(AnimV20::DeltaRotateShiftCtrl, targetNode), das::makeType<char *>(ml));
  }
};

struct DeltaAnglesCtrlAnnotation : das::ManagedStructureAnnotation<AnimV20::DeltaAnglesCtrl, false>
{
  DeltaAnglesCtrlAnnotation(das::ModuleLibrary &ml) : ManagedStructureAnnotation("DeltaAnglesCtrl", ml)
  {
    cppName = " ::AnimV20::DeltaAnglesCtrl";
    addField<DAS_BIND_MANAGED_FIELD(varId)>("varId");
    addField<DAS_BIND_MANAGED_FIELD(destRotateVarId)>("destRotateVarId");
    addField<DAS_BIND_MANAGED_FIELD(destPitchVarId)>("destPitchVarId");
    addField<DAS_BIND_MANAGED_FIELD(destLeanVarId)>("destLeanVarId");
    addFieldEx("nodeName", "nodeName", offsetof(AnimV20::DeltaAnglesCtrl, nodeName), das::makeType<char *>(ml));
    addField<DAS_BIND_MANAGED_FIELD(fwdAxisIdx)>("fwdAxisIdx");
    addField<DAS_BIND_MANAGED_FIELD(sideAxisIdx)>("sideAxisIdx");
    addField<DAS_BIND_MANAGED_FIELD(scaleR)>("scaleR");
    addField<DAS_BIND_MANAGED_FIELD(scaleP)>("scaleP");
    addField<DAS_BIND_MANAGED_FIELD(scaleL)>("scaleL");
    addField<DAS_BIND_MANAGED_FIELD(invFwd)>("invFwd");
    addField<DAS_BIND_MANAGED_FIELD(invSide)>("invSide");
  }
};

struct FootLockerIKCtrlAnnotation : das::ManagedStructureAnnotation<AnimV20::FootLockerIKCtrl, false>
{
  FootLockerIKCtrlAnnotation(das::ModuleLibrary &ml) : ManagedStructureAnnotation("FootLockerIKCtrl", ml)
  {
    cppName = " ::AnimV20::FootLockerIKCtrl";
    addField<DAS_BIND_MANAGED_FIELD(unlockViscosity)>("unlockViscosity");
    addField<DAS_BIND_MANAGED_FIELD(maxReachScale)>("maxReachScale");
    addField<DAS_BIND_MANAGED_FIELD(unlockRadius)>("unlockRadius");
    addField<DAS_BIND_MANAGED_FIELD(unlockWhenUnreachableRadius)>("unlockWhenUnreachableRadius");
    addField<DAS_BIND_MANAGED_FIELD(toeNodeHeight)>("toeNodeHeight");
    addField<DAS_BIND_MANAGED_FIELD(ankleNodeHeight)>("ankleNodeHeight");
    addField<DAS_BIND_MANAGED_FIELD(maxFootUp)>("maxFootUp");
    addField<DAS_BIND_MANAGED_FIELD(maxFootDown)>("maxFootDown");
    addField<DAS_BIND_MANAGED_FIELD(maxToeMoveUp)>("maxToeMoveUp");
    addField<DAS_BIND_MANAGED_FIELD(maxToeMoveDown)>("maxToeMoveDown");
    addField<DAS_BIND_MANAGED_FIELD(footRaisingSpeed)>("footRaisingSpeed");
    addField<DAS_BIND_MANAGED_FIELD(footInclineViscosity)>("footInclineViscosity");
    addField<DAS_BIND_MANAGED_FIELD(maxAnkleAnlgeCos)>("maxAnkleAnlgeCos");
    addField<DAS_BIND_MANAGED_FIELD(maxHipMoveDown)>("maxHipMoveDown");
    addField<DAS_BIND_MANAGED_FIELD(hipMoveViscosity)>("hipMoveViscosity");
    addField<DAS_BIND_MANAGED_FIELD(legsDataVarId)>("legsDataVarId");
    addField<DAS_BIND_MANAGED_FIELD(hipMoveDownVarId)>("hipMoveDownVarId");
  }
};

struct AnimPostBlendHasAttachmentAnnotation : das::ManagedStructureAnnotation<AnimV20::AnimPostBlendHasAttachment, false>
{
  AnimPostBlendHasAttachmentAnnotation(das::ModuleLibrary &ml) : ManagedStructureAnnotation("AnimPostBlendHasAttachment", ml)
  {
    cppName = " ::AnimV20::AnimPostBlendHasAttachment";
    addField<DAS_BIND_MANAGED_FIELD(slotId)>("slotId");
    addField<DAS_BIND_MANAGED_FIELD(destVarId)>("destVarId");
    addField<DAS_BIND_MANAGED_FIELD(invertVal)>("invertVal");
  }
};

struct AnimPostBlendHumanAimCtrlAnnotation : das::ManagedStructureAnnotation<AnimV20::AnimPostBlendHumanAimCtrl, false>
{
  AnimPostBlendHumanAimCtrlAnnotation(das::ModuleLibrary &ml) : ManagedStructureAnnotation("AnimPostBlendHumanAimCtrl", ml)
  {
    cppName = " ::AnimV20::AnimPostBlendHumanAimCtrl";
    addFieldEx("rotateAroundNodeName", "rotateAroundNodeName", offsetof(AnimV20::AnimPostBlendHumanAimCtrl, rotateAroundNodeName),
      das::makeType<char *>(ml));
    addFieldEx("targetNodeName", "targetNodeName", offsetof(AnimV20::AnimPostBlendHumanAimCtrl, targetNodeName),
      das::makeType<char *>(ml));
    addFieldEx("alignNodeName", "alignNodeName", offsetof(AnimV20::AnimPostBlendHumanAimCtrl, alignNodeName),
      das::makeType<char *>(ml));
    addField<DAS_BIND_MANAGED_FIELD(stateVarId)>("stateVarId");
    addField<DAS_BIND_MANAGED_FIELD(pitchVarId)>("pitchVarId");
    addField<DAS_BIND_MANAGED_FIELD(yawVarId)>("yawVarId");
    addField<DAS_BIND_MANAGED_FIELD(rollVarId)>("rollVarId");
    addField<DAS_BIND_MANAGED_FIELD(offsetXVar)>("offsetXVar");
    addField<DAS_BIND_MANAGED_FIELD(offsetYVar)>("offsetYVar");
    addField<DAS_BIND_MANAGED_FIELD(offsetZVar)>("offsetZVar");
  }
};

struct AnimPostBlendTwoBonesIKAnnotation : das::ManagedStructureAnnotation<AnimV20::AnimPostBlendTwoBonesIK, false>
{
  AnimPostBlendTwoBonesIKAnnotation(das::ModuleLibrary &ml) : ManagedStructureAnnotation("AnimPostBlendTwoBonesIK", ml)
  {
    cppName = " ::AnimV20::AnimPostBlendTwoBonesIK";
    addField<DAS_BIND_MANAGED_FIELD(varId)>("varId");
  }
};

struct AnimcharNodesMat44Annotation : das::ManagedStructureAnnotation<AnimcharNodesMat44, false>
{
  AnimcharNodesMat44Annotation(das::ModuleLibrary &ml) : ManagedStructureAnnotation("AnimcharNodesMat44", ml)
  {
    cppName = " ::AnimcharNodesMat44";

    addFieldEx("wofs", "wofs", offsetof(AnimcharNodesMat44, wofs), das::makeType<das::float4>(ml));
    addFieldEx("bsph", "bsph", offsetof(AnimcharNodesMat44, bsph), das::makeType<das::float4>(ml));
  }
};

struct IAnimBlendNodePtrAnnotation : das::ManagedStructureAnnotation<IAnimBlendNodePtr, false>
{
  IAnimBlendNodePtrAnnotation(das::ModuleLibrary &ml) : ManagedStructureAnnotation("IAnimBlendNodePtr", ml)
  {
    cppName = " ::IAnimBlendNodePtr";
    // addProperty<DAS_BIND_MANAGED_PROP(get)>("get");
  }
  bool canBePlacedInContainer() const override { return true; }
};

struct AnimBlendNodeLeafPtrAnnotation : das::ManagedStructureAnnotation<AnimBlendNodeLeafPtr, false>
{
  AnimBlendNodeLeafPtrAnnotation(das::ModuleLibrary &ml) : ManagedStructureAnnotation("AnimBlendNodeLeafPtr", ml)
  {
    cppName = " ::AnimBlendNodeLeafPtr";
  }
  bool canBePlacedInContainer() const override { return true; }
};

struct AnimPostBlendCtrlPtrAnnotation : das::ManagedStructureAnnotation<AnimPostBlendCtrlPtr, false>
{
  AnimPostBlendCtrlPtrAnnotation(das::ModuleLibrary &ml) : ManagedStructureAnnotation("AnimPostBlendCtrlPtr", ml)
  {
    cppName = " ::AnimPostBlendCtrlPtr";
  }
  bool canBePlacedInContainer() const override { return true; }
};

struct Animate2ndPassCtxAnnotation : das::ManagedStructureAnnotation<Animate2ndPassCtx, false>
{
  Animate2ndPassCtxAnnotation(das::ModuleLibrary &ml) : ManagedStructureAnnotation("Animate2ndPassCtx", ml)
  {
    cppName = " ::Animate2ndPassCtx";
  }
};

namespace bind_dascript
{
static char animchar_das[] =
#include "animchar.das.inl"
  ;
class AnimV20 final : public das::Module
{
public:
  AnimV20() : das::Module("AnimV20")
  {
    das::ModuleLibrary lib(this);
    addBuiltinDependency(lib, require("math"));
    addBuiltinDependency(lib, require("ecs"));
    addBuiltinDependency(lib, require("DagorMath"));
    addBuiltinDependency(lib, require("GeomNodeTree"));
    addBuiltinDependency(lib, require("PhysDecl"));
    addEnumeration(das::make_smart<EnumerationAnimcharVisbits>());
    addAnnotation(das::make_smart<NameIdPairAnnotation>(lib));
    addAnnotation(das::make_smart<AnimDataAnnotation>(lib));

    addAnnotation(das::make_smart<IPureAnimStateHolderAnnotation>(lib));
    addAnnotation(das::make_smart<IAnimBlendNodePtrAnnotation>(lib));
    addAnnotation(das::make_smart<AnimBlendNodeLeafPtrAnnotation>(lib));
    addAnnotation(das::make_smart<AnimPostBlendCtrlPtrAnnotation>(lib));

    das::typeFactory<BnlPtrTab>::make(lib);
    das::typeFactory<PbCtrlPtrTab>::make(lib);

    auto iAnimBlendNodeAnn = das::make_smart<IAnimBlendNodeAnnotation>(lib);
    auto animBlendNodeLeafAnn = das::make_smart<AnimBlendNodeLeafAnnotation>(lib);
    auto animBlendNodeContinuousLeafAnn = das::make_smart<AnimBlendNodeContinuousLeafAnnotation>(lib);

    addAnnotation(iAnimBlendNodeAnn);
    add_annotation(this, animBlendNodeLeafAnn, iAnimBlendNodeAnn);
    add_annotation(this, animBlendNodeContinuousLeafAnn, animBlendNodeLeafAnn);
    add_annotation(this, das::make_smart<AnimBlendNodeNullAnnotation>(lib), iAnimBlendNodeAnn);
    add_annotation(this, das::make_smart<AnimBlendNodeStillLeafAnnotation>(lib), animBlendNodeLeafAnn);
    add_annotation(this, das::make_smart<AnimBlendNodeParametricLeafAnnotation>(lib), animBlendNodeLeafAnn);
    add_annotation(this, das::make_smart<AnimBlendNodeSingleLeafAnnotation>(lib), animBlendNodeContinuousLeafAnn);
    add_annotation(this, das::make_smart<AnimBlendCtrl_1axisAnnotation>(lib), iAnimBlendNodeAnn);
    add_annotation(this, das::make_smart<AnimBlendCtrl_Fifo3Annotation>(lib), iAnimBlendNodeAnn);
    add_annotation(this, das::make_smart<AnimBlendCtrl_RandomSwitcherAnnotation>(lib), iAnimBlendNodeAnn);
    add_annotation(this, das::make_smart<AnimBlendCtrl_HubAnnotation>(lib), iAnimBlendNodeAnn);
    add_annotation(this, das::make_smart<AnimBlendCtrl_BlenderAnnotation>(lib), iAnimBlendNodeAnn);
    add_annotation(this, das::make_smart<AnimBlendCtrl_BinaryIndirectSwitchAnnotation>(lib), iAnimBlendNodeAnn);
    add_annotation(this, das::make_smart<AnimBlendCtrl_SetMotionMatchingTagAnnotation>(lib), iAnimBlendNodeAnn);
    add_annotation(this, das::make_smart<AnimBlendCtrl_LinearPolyAnnotation>(lib), iAnimBlendNodeAnn);
    add_annotation(this, das::make_smart<AnimBlendCtrl_ParametricSwitcherAnnotation>(lib), animBlendNodeLeafAnn);
    add_annotation(this, das::make_smart<AnimPostBlendCtrlAnnotation>(lib), iAnimBlendNodeAnn);

    addAnnotation(das::make_smart<AnimFifo3QueueAnnotation>(lib));

    addAnnotation(das::make_smart<AnimBlendCtrl_1axisAnimSliceAnnotation>(lib));
    addAnnotation(das::make_smart<AnimBlendCtrl_RandomSwitcherRandomAnimAnnotation>(lib));
    addAnnotation(das::make_smart<AnimBlendCtrl_LinearPolyAnimPointAnnotation>(lib));
    addAnnotation(das::make_smart<AnimBlendCtrl_ParametricSwitcherItemAnimAnnotation>(lib));
    addAnnotation(das::make_smart<AnimationGraphStateRecAnnotation>(lib));

    addAnnotation(das::make_smart<AnimPostBlendParamFromNodeLocalDataAnnotation>(lib));
    addAnnotation(das::make_smart<AnimPostBlendParamFromNodeAnnotation>(lib));
    addAnnotation(das::make_smart<AttachGeomNodeCtrlVarIdAnnotation>(lib));
    addAnnotation(das::make_smart<AttachGeomNodeCtrlAttachDescAnnotation>(lib));
    addAnnotation(das::make_smart<AttachGeomNodeCtrlAnnotation>(lib));

    addAnnotation(das::make_smart<AnimPostBlendAlignCtrlAnnotation>(lib));
    addAnnotation(das::make_smart<AnimPostBlendAlignExCtrlAnnotation>(lib));
    addAnnotation(das::make_smart<AnimPostBlendRotateCtrlAnnotation>(lib));
    addAnnotation(das::make_smart<AnimPostBlendRotateAroundCtrlAnnotation>(lib));
    addAnnotation(das::make_smart<AnimPostBlendScaleCtrlAnnotation>(lib));
    addAnnotation(das::make_smart<AnimPostBlendMoveCtrlAnnotation>(lib));
    addAnnotation(das::make_smart<AnimPostBlendCondHideCtrlAnnotation>(lib));
    addAnnotation(das::make_smart<AnimPostBlendAimCtrlAnnotation>(lib));
    addAnnotation(das::make_smart<ApbParamCtrlAnnotation>(lib));
    addAnnotation(das::make_smart<DefClampParamCtrlAnnotation>(lib));
    addAnnotation(das::make_smart<ApbAnimateCtrlAnnotation>(lib));
    addAnnotation(das::make_smart<LegsIKCtrlAnnotation>(lib));
    addAnnotation(das::make_smart<MultiChainFABRIKCtrlAnnotation>(lib));
    addAnnotation(das::make_smart<AnimPostBlendNodeLookatCtrlAnnotation>(lib));
    addAnnotation(das::make_smart<AnimPostBlendNodeLookatNodeCtrlAnnotation>(lib));
    addAnnotation(das::make_smart<AnimPostBlendEffFromAttachementAnnotation>(lib));
    addAnnotation(das::make_smart<AnimPostBlendNodeEffectorFromChildIKAnnotation>(lib));
    addAnnotation(das::make_smart<AnimPostBlendMatVarFromNodeAnnotation>(lib));
    addAnnotation(das::make_smart<AnimPostBlendNodesFromAttachementAnnotation>(lib));
    addAnnotation(das::make_smart<AnimPostBlendCompoundRotateShiftAnnotation>(lib));
    addAnnotation(das::make_smart<AnimPostBlendSetParamAnnotation>(lib));
    addAnnotation(das::make_smart<AnimPostBlendTwistCtrlAnnotation>(lib));
    addAnnotation(das::make_smart<AnimPostBlendEyeCtrlAnnotation>(lib));
    addAnnotation(das::make_smart<DeltaRotateShiftCtrlAnnotation>(lib));
    addAnnotation(das::make_smart<DeltaAnglesCtrlAnnotation>(lib));
    addAnnotation(das::make_smart<FootLockerIKCtrlAnnotation>(lib));
    addAnnotation(das::make_smart<AnimPostBlendHasAttachmentAnnotation>(lib));
    addAnnotation(das::make_smart<AnimPostBlendHumanAimCtrlAnnotation>(lib));
    addAnnotation(das::make_smart<AnimPostBlendTwoBonesIKAnnotation>(lib));

    addAnnotation(das::make_smart<AnimBlenderAnnotation>(lib));
    addAnnotation(das::make_smart<AnimationGraphAnnotation>(lib));
    addAnnotation(das::make_smart<IAnimStateHolderAnnotation>(lib));
    addAnnotation(das::make_smart<AnimcharDebugContextAnnotation>(lib));
    addAnnotation(das::make_smart<AnimcharBaseComponentAnnotation>(lib));
    addAnnotation(das::make_smart<RoNameMapExIdPatchableTabAnnotation>(lib));
    addAnnotation(das::make_smart<RoNameMapExAnnotation>(lib));
    addAnnotation(das::make_smart<DynSceneResNameMapResourceAnnotation>(lib));
    addAnnotation(das::make_smart<DynamicRenderableSceneLodsResourceAnnotation>(lib));
    addAnnotation(das::make_smart<DynamicRenderableSceneInstanceAnnotation>(lib));
    addAnnotation(das::make_smart<AnimcharRendComponentAnnotation>(lib));
    addAnnotation(das::make_smart<IAnimCharacter2Annotation>(lib));
    addAnnotation(das::make_smart<AnimcharNodesMat44Annotation>(lib));
    addAnnotation(das::make_smart<Animate2ndPassCtxAnnotation>(lib));
    das::addExtern<DAS_BIND_FUN(::AnimV20::addEnumValue)>(*this, lib, "animV20_add_enum_value", das::SideEffects::modifyExternal,
      "::AnimV20::addEnumValue");
    das::addExtern<DAS_BIND_FUN(::AnimV20::getEnumValueByName)>(*this, lib, "animV20_get_enum_value_by_name",
      das::SideEffects::accessExternal, "::AnimV20::getEnumValueByName");
    das::addExtern<DAS_BIND_FUN(::AnimV20::getEnumName)>(*this, lib, "animV20_get_enum_name_by_id", das::SideEffects::accessExternal,
      "::AnimV20::getEnumName");
    das::addExtern<DAS_BIND_FUN(::AnimResManagerV20::add_bn)>(*this, lib, "AnimResManagerV20_add_bn", das::SideEffects::worstDefault,
      "::AnimResManagerV20::add_bn");
    das::addExtern<DAS_BIND_FUN(::AnimResManagerV20::add_bnl)>(*this, lib, "AnimResManagerV20_add_bnl", das::SideEffects::worstDefault,
      "::AnimResManagerV20::add_bnl");
    das::addExtern<DAS_BIND_FUN(animchar_get_res_name)>(*this, lib, "animchar_get_res_name", das::SideEffects::none,
      "bind_dascript::animchar_get_res_name")
      ->arg("animchar");
    das::addExtern<DAS_BIND_FUN(animchar_act)>(*this, lib, "animchar_act", das::SideEffects::modifyArgument,
      "bind_dascript::animchar_act");
    das::addExtern<DAS_BIND_FUN(animchar_copy_nodes)>(*this, lib, "animchar_copy_nodes", das::SideEffects::modifyArgument,
      "bind_dascript::animchar_copy_nodes");
    das::addExtern<DAS_BIND_FUN(ronamemapex_get_name_id)>(*this, lib, "ronamemapex_get_name_id", das::SideEffects::none,
      "bind_dascript::ronamemapex_get_name_id");
    using method_getNodeId = DAS_CALL_MEMBER(DynamicRenderableSceneInstance::getNodeId);
    das::addExtern<DAS_CALL_METHOD(method_getNodeId)>(*this, lib, "scene_instance_getNodeId", das::SideEffects::none,
      DAS_CALL_MEMBER_CPP(DynamicRenderableSceneInstance::getNodeId));
    das::addExtern<DAS_BIND_FUN(scene_instance_show_node)>(*this, lib, "scene_instance_show_node", das::SideEffects::modifyArgument,
      "bind_dascript::scene_instance_show_node");
    using method_setNodeOpacity = DAS_CALL_MEMBER(::DynamicRenderableSceneInstance::setNodeOpacity);
    das::addExtern<DAS_CALL_METHOD(method_setNodeOpacity)>(*this, lib, "scene_instance_setNodeOpacity",
      das::SideEffects::modifyArgument, DAS_CALL_MEMBER_CPP(::DynamicRenderableSceneInstance::setNodeOpacity));
    das::addExtern<DAS_BIND_FUN(scene_instance_is_node_visible)>(*this, lib, "scene_instance_is_node_visible", das::SideEffects::none,
      "bind_dascript::scene_instance_is_node_visible");
    das::addExtern<DAS_BIND_FUN(scene_instance_get_local_bounding_box)>(*this, lib, "scene_instance_get_local_bounding_box",
      das::SideEffects::modifyArgument, "bind_dascript::scene_instance_get_local_bounding_box");
    das::addExtern<DAS_BIND_FUN(calc_world_box)>(*this, lib, "calc_world_box", das::SideEffects::modifyArgument,
      "bind_dascript::calc_world_box");
    using method_getNodeWtm = DAS_CALL_MEMBER(DynamicRenderableSceneInstance::getNodeWtm);
    das::addExtern<DAS_CALL_METHOD(method_getNodeWtm), das::SimNode_ExtFuncCallRef>(*this, lib, "scene_instance_getNodeWtm",
      das::SideEffects::none, DAS_CALL_MEMBER_CPP(DynamicRenderableSceneInstance::getNodeWtm));
    using method_setNodeWtm = das::das_call_member<void (DynamicRenderableSceneInstance::*)(uint32_t, const TMatrix &),
      &::DynamicRenderableSceneInstance::setNodeWtm>;
    das::addExtern<DAS_CALL_METHOD(method_setNodeWtm)>(*this, lib, "scene_instance_setNodeWtm", das::SideEffects::modifyArgument,
      "das_call_member<void(DynamicRenderableSceneInstance::*)(uint32_t, const TMatrix &), "
      "&::DynamicRenderableSceneInstance::setNodeWtm>::invoke");
    using method_clearNodeCollapser =
      das::das_call_member<void (DynamicRenderableSceneInstance::*)(), &::DynamicRenderableSceneInstance::clearNodeCollapser>;
    das::addExtern<DAS_CALL_METHOD(method_clearNodeCollapser)>(*this, lib, "scene_instance_clearNodeCollapser",
      das::SideEffects::modifyArgument,
      "das_call_member<void(DynamicRenderableSceneInstance::*)(), "
      "&::DynamicRenderableSceneInstance::clearNodeCollapser>::invoke");
    using method_markNodeCollapserNode = das::das_call_member<void (DynamicRenderableSceneInstance::*)(uint32_t),
      &::DynamicRenderableSceneInstance::markNodeCollapserNode>;
    das::addExtern<DAS_CALL_METHOD(method_markNodeCollapserNode)>(*this, lib, "scene_instance_markNodeCollapserNode",
      das::SideEffects::modifyArgument,
      "das_call_member<void(DynamicRenderableSceneInstance::*)(uint32_t), "
      "&::DynamicRenderableSceneInstance::markNodeCollapserNode>::invoke");
    auto sendChangeAnimStateEventExt = das::addExtern<DAS_BIND_FUN(send_change_anim_state_event)>(*this, lib,
      "send_change_anim_state_event", das::SideEffects::modifyExternal, "bind_dascript::send_change_anim_state_event");
    sendChangeAnimStateEventExt->annotations.push_back(
      annotation_declaration(das::make_smart<ConstStringArgCallFunctionAnnotation<1>>()));

    das::addExtern<DAS_BIND_FUN(AnimCharV20::getSlotId)>(*this, lib, "animchar_getSlotId", das::SideEffects::accessExternal,
      "AnimCharV20::getSlotId");
    das::addExtern<DAS_BIND_FUN(AnimCharV20::addSlotId)>(*this, lib, "animchar_addSlotId", das::SideEffects::modifyExternal,
      "AnimCharV20::addSlotId");
    das::addExtern<DAS_BIND_FUN(AnimCharV20::getSlotName)>(*this, lib, "animchar_getSlotName", das::SideEffects::accessExternal,
      "AnimCharV20::getSlotName");

    using method_setTraceContext = DAS_CALL_MEMBER(::AnimCharV20::AnimcharBaseComponent::setTraceContext);
    das::addExtern<DAS_CALL_METHOD(method_setTraceContext)>(*this, lib, "animchar_setTraceContext", das::SideEffects::modifyArgument,
      DAS_CALL_MEMBER_CPP(::AnimCharV20::AnimcharBaseComponent::setTraceContext));

    das::addExtern<DAS_BIND_FUN(anim_graph_getStateIdx)>(*this, lib, "anim_graph_getStateIdx", das::SideEffects::none,
      "bind_dascript::anim_graph_getStateIdx");
    das::addExtern<DAS_BIND_FUN(anim_graph_getNodeId)>(*this, lib, "anim_graph_getNodeId", das::SideEffects::modifyArgument,
      "bind_dascript::anim_graph_getNodeId");
    das::addExtern<DAS_BIND_FUN(anim_graph_getBlendNodeId)>(*this, lib, "anim_graph_getBlendNodeId", das::SideEffects::none,
      "bind_dascript::anim_graph_getBlendNodeId");
    das::addExtern<DAS_BIND_FUN(anim_graph_getParamId)>(*this, lib, "anim_graph_getParamId", das::SideEffects::none,
      "bind_dascript::anim_graph_getParamId");
    using method_getParamId =
      das::das_call_member<int (::AnimCharV20::AnimationGraph::*)(const char *) const, &::AnimCharV20::AnimationGraph::getParamId>;
    das::addExtern<DAS_CALL_METHOD(method_getParamId)>(*this, lib, "anim_graph_getParamId", das::SideEffects::none,
      "das_call_member<int(::AnimCharV20::AnimationGraph::*)(const char *) const, "
      "&::AnimCharV20::AnimationGraph::getParamId>::invoke");
    das::addExtern<DAS_BIND_FUN(anim_graph_getFifo3NodePtr)>(*this, lib, "anim_graph_getFifo3NodePtr",
      das::SideEffects::modifyArgument, "bind_dascript::anim_graph_getFifo3NodePtr");

    das::addExtern<DAS_BIND_FUN(anim_graph_enqueueState)>(*this, lib, "anim_graph_enqueueState", das::SideEffects::modifyArgument,
      "bind_dascript::anim_graph_enqueueState");
    das::addExtern<DAS_BIND_FUN(anim_graph_setStateSpeed)>(*this, lib, "anim_graph_setStateSpeed", das::SideEffects::modifyArgument,
      "bind_dascript::anim_graph_setStateSpeed");
    das::addExtern<DAS_BIND_FUN(anim_graph_enqueueNode)>(*this, lib, "anim_graph_enqueueNode", das::SideEffects::modifyArgument,
      "bind_dascript::anim_graph_enqueueNode");
    das::addExtern<DAS_BIND_FUN(anim_graph_getParamNames)>(*this, lib, "anim_graph_getParamNames", das::SideEffects::worstDefault,
      "bind_dascript::anim_graph_getParamNames");
    das::addExtern<DAS_BIND_FUN(anim_graph_getAnimNodeNames)>(*this, lib, "anim_graph_getAnimNodeNames",
      das::SideEffects::worstDefault, "bind_dascript::anim_graph_getAnimNodeNames");
    das::addExtern<DAS_BIND_FUN(anim_graph_getStRec)>(*this, lib, "anim_graph_getStRec", das::SideEffects::worstDefault,
      "bind_dascript::anim_graph_getStRec");

    das::addExtern<DAS_BIND_FUN(animchar_getDebugBlenderState)>(*this, lib, "animchar_getDebugBlenderState",
      das::SideEffects::accessExternal, "bind_dascript::animchar_getDebugBlenderState");

    das::addExtern<DAS_BIND_FUN(animchar_getAnimBlendNodeWeights)>(*this, lib, "animchar_getAnimBlendNodeWeights",
      das::SideEffects::accessExternal, "bind_dascript::animchar_getAnimBlendNodeWeights");

    das::addExtern<DAS_BIND_FUN(AnimFifo3Queue_get_node)>(*this, lib, "AnimFifo3Queue_get_node", das::SideEffects::modifyArgument,
      "bind_dascript::AnimFifo3Queue_get_node");

    das::addExtern<DAS_BIND_FUN(AnimBlendCtrl_1axis_getChildren)>(*this, lib, "AnimBlendCtrl_1axis_getChildren",
      das::SideEffects::worstDefault, "bind_dascript::AnimBlendCtrl_1axis_getChildren");

    das::addExtern<DAS_BIND_FUN(AnimData_get_source_anim_data)>(*this, lib, "AnimData_get_source_anim_data",
      das::SideEffects::accessExternal, "bind_dascript::AnimData_get_source_anim_data");

    das::addExtern<DAS_BIND_FUN(AnimBlendNodeLeaf_get_anim<::AnimV20::AnimBlendNodeLeaf>)>(*this, lib, "AnimBlendNodeLeaf_get_anim",
      das::SideEffects::modifyArgument, "bind_dascript::AnimBlendNodeLeaf_get_anim< ::AnimV20::AnimBlendNodeLeaf>");

    das::addExtern<DAS_BIND_FUN(AnimBlendNodeLeaf_get_anim<::AnimV20::AnimBlendNodeContinuousLeaf>)>(*this, lib,
      "AnimBlendNodeContinuousLeaf_get_anim", das::SideEffects::modifyArgument,
      "bind_dascript::AnimBlendNodeLeaf_get_anim< ::AnimV20::AnimBlendNodeContinuousLeaf>");

    das::addExtern<DAS_BIND_FUN(AnimBlendNodeLeaf_get_anim<::AnimV20::AnimBlendNodeParametricLeaf>)>(*this, lib,
      "AnimBlendNodeParametricLeaf_get_anim", das::SideEffects::modifyArgument,
      "bind_dascript::AnimBlendNodeLeaf_get_anim< ::AnimV20::AnimBlendNodeParametricLeaf>");

    das::addExtern<DAS_BIND_FUN(AnimBlendNodeLeaf_get_anim<::AnimV20::AnimBlendNodeSingleLeaf>)>(*this, lib,
      "AnimBlendNodeSingleLeaf_get_anim", das::SideEffects::modifyArgument,
      "bind_dascript::AnimBlendNodeLeaf_get_anim< ::AnimV20::AnimBlendNodeSingleLeaf>");

    das::addExtern<DAS_BIND_FUN(AnimBlendNodeLeaf_get_anim<::AnimV20::AnimBlendNodeStillLeaf>)>(*this, lib,
      "AnimBlendNodeStillLeaf_get_anim", das::SideEffects::modifyArgument,
      "bind_dascript::AnimBlendNodeLeaf_get_anim< ::AnimV20::AnimBlendNodeStillLeaf>");

    das::addExtern<DAS_BIND_FUN(AnimBlendCtrl_ParametricSwitcher_getChildren)>(*this, lib,
      "AnimBlendCtrl_ParametricSwitcher_getChildren", das::SideEffects::worstDefault,
      "bind_dascript::AnimBlendCtrl_ParametricSwitcher_getChildren");

    das::addExtern<DAS_BIND_FUN(AnimBlendCtrl_ParametricSwitcherItemAnim_getStart)>(*this, lib,
      "AnimBlendCtrl_ParametricSwitcherItemAnim_getStart", das::SideEffects::none,
      "bind_dascript::AnimBlendCtrl_ParametricSwitcherItemAnim_getStart");

    das::addExtern<DAS_BIND_FUN(AnimBlendCtrl_ParametricSwitcherItemAnim_getEnd)>(*this, lib,
      "AnimBlendCtrl_ParametricSwitcherItemAnim_getEnd", das::SideEffects::none,
      "bind_dascript::AnimBlendCtrl_ParametricSwitcherItemAnim_getEnd");

    das::addExtern<DAS_BIND_FUN(AnimBlendCtrl_RandomSwitcher_getChildren)>(*this, lib, "AnimBlendCtrl_RandomSwitcher_getChildren",
      das::SideEffects::worstDefault, "bind_dascript::AnimBlendCtrl_RandomSwitcher_getChildren");

    das::addExtern<DAS_BIND_FUN(IAnimBlendNodePtr_get)>(*this, lib, "get", das::SideEffects::modifyArgument,
      "bind_dascript::IAnimBlendNodePtr_get");

    das::addExtern<DAS_BIND_FUN(AnimBlendNodeLeafPtr_get)>(*this, lib, "get", das::SideEffects::modifyArgument,
      "bind_dascript::AnimBlendNodeLeafPtr_get");

    das::addExtern<DAS_BIND_FUN(AnimPostBlendCtrlPtr_get)>(*this, lib, "get", das::SideEffects::modifyArgument,
      "bind_dascript::AnimPostBlendCtrlPtr_get");

    das::addExtern<DAS_BIND_FUN(AnimBlendCtrl_Hub_getChildren)>(*this, lib, "AnimBlendCtrl_Hub_getChildren",
      das::SideEffects::worstDefault, "bind_dascript::AnimBlendCtrl_Hub_getChildren");

    das::addExtern<DAS_BIND_FUN(AnimBlendCtrl_Hub_getDefNodeWt)>(*this, lib, "AnimBlendCtrl_Hub_getDefNodeWt",
      das::SideEffects::worstDefault, "bind_dascript::AnimBlendCtrl_Hub_getDefNodeWt");

    das::addExtern<DAS_BIND_FUN(AnimBlendCtrl_Blender_getChildren)>(*this, lib, "AnimBlendCtrl_Blender_getChildren",
      das::SideEffects::worstDefault, "bind_dascript::AnimBlendCtrl_Blender_getChildren");

    das::addExtern<DAS_BIND_FUN(AnimBlendCtrl_BinaryIndirectSwitch_getChildren)>(*this, lib,
      "AnimBlendCtrl_BinaryIndirectSwitch_getChildren", das::SideEffects::worstDefault,
      "bind_dascript::AnimBlendCtrl_BinaryIndirectSwitch_getChildren");

    das::addExtern<DAS_BIND_FUN(AnimBlendCtrl_LinearPoly_getChildren)>(*this, lib, "AnimBlendCtrl_LinearPoly_getChildren",
      das::SideEffects::worstDefault, "bind_dascript::AnimBlendCtrl_LinearPoly_getChildren");

    das::addExtern<DAS_BIND_FUN(AttachGeomNodeCtrl_getNodeNames)>(*this, lib, "AttachGeomNodeCtrl_getNodeNames",
      das::SideEffects::worstDefault, "bind_dascript::AttachGeomNodeCtrl_getNodeNames");

    using method_getResName = DAS_CALL_MEMBER(::AnimCharV20::AnimcharBaseComponent::getResName);
    das::addExtern<DAS_CALL_METHOD(method_getResName)>(*this, lib, "animchar_getResName", das::SideEffects::none,
      DAS_CALL_MEMBER_CPP(::AnimCharV20::AnimcharBaseComponent::getResName));

    using method_recalcWtm =
      das::das_call_member<void (::AnimCharV20::AnimcharBaseComponent::*)(), &::AnimCharV20::AnimcharBaseComponent::recalcWtm>;
    das::addExtern<DAS_CALL_METHOD(method_recalcWtm)>(*this, lib, "animchar_recalc_wtm", das::SideEffects::modifyArgument,
      "das_call_member<void(::AnimCharV20::AnimcharBaseComponent::*)(), &::AnimCharV20::AnimcharBaseComponent::recalcWtm>::invoke");

    using method_getTm = das::das_call_member<void (::AnimCharV20::AnimcharBaseComponent::*)(TMatrix &) const,
      &::AnimCharV20::AnimcharBaseComponent::getTm>;
    das::addExtern<DAS_CALL_METHOD(method_getTm)>(*this, lib, "animchar_get_tm", das::SideEffects::modifyArgument,
      "das_call_member<void(::AnimCharV20::AnimcharBaseComponent::*)(TMatrix &) const, "
      "&::AnimCharV20::AnimcharBaseComponent::getTm>::invoke");

    das::addExtern<DAS_BIND_FUN(animchar_setRagdollPostController)>(*this, lib, "animchar_setPostController",
      das::SideEffects::modifyArgument, "bind_dascript::animchar_setRagdollPostController");
    das::addExtern<DAS_BIND_FUN(animchar_resetRagdollPostController)>(*this, lib, "animchar_resetPostController",
      das::SideEffects::modifyArgument, "bind_dascript::animchar_resetRagdollPostController");

    using method_setTm = das::das_call_member<void (::AnimCharV20::AnimcharBaseComponent::*)(const TMatrix &, bool),
      &::AnimCharV20::AnimcharBaseComponent::setTm>;
    das::addExtern<DAS_CALL_METHOD(method_setTm)>(*this, lib, "animchar_set_tm", das::SideEffects::modifyArgument,
      "das_call_member<void(::AnimCharV20::AnimcharBaseComponent::*)(const TMatrix &, bool), "
      "&::AnimCharV20::AnimcharBaseComponent::setTm>::invoke");
    using method_setTm2 =
      das::das_call_member<void (::AnimCharV20::AnimcharBaseComponent::*)(const Point3 &, const Point3 &, const Point3 &),
        &::AnimCharV20::AnimcharBaseComponent::setTm>;
    das::addExtern<DAS_CALL_METHOD(method_setTm2)>(*this, lib, "animchar_set_tm", das::SideEffects::modifyArgument,
      "das_call_member<void(::AnimCharV20::AnimcharBaseComponent::*)(const Point3 &, const Point3 &, const Point3 &), "
      "&::AnimCharV20::AnimcharBaseComponent::setTm>::invoke");

    using method_setTmWithOfs = DAS_CALL_MEMBER(::AnimCharV20::AnimcharBaseComponent::setTmWithOfs);
    das::addExtern<DAS_CALL_METHOD(method_setTmWithOfs)>(*this, lib, "animchar_setTmWithOfs", das::SideEffects::modifyArgument,
      DAS_CALL_MEMBER_CPP(::AnimCharV20::AnimcharBaseComponent::setTmWithOfs));

    using method_cloneTo = DAS_CALL_MEMBER(::AnimCharV20::AnimcharBaseComponent::cloneTo);
    das::addExtern<DAS_CALL_METHOD(method_cloneTo)>(*this, lib, "animchar_cloneTo", das::SideEffects::modifyArgument,
      DAS_CALL_MEMBER_CPP(::AnimCharV20::AnimcharBaseComponent::cloneTo));

    using method_forcePostRecalcWtm = DAS_CALL_MEMBER(::AnimCharV20::AnimcharBaseComponent::forcePostRecalcWtm);
    das::addExtern<DAS_CALL_METHOD(method_forcePostRecalcWtm)>(*this, lib, "animchar_forcePostRecalcWtm",
      das::SideEffects::modifyArgument, DAS_CALL_MEMBER_CPP(::AnimCharV20::AnimcharBaseComponent::forcePostRecalcWtm));

    using method_doRecalcAnimAndWtm = DAS_CALL_MEMBER(::AnimCharV20::AnimcharBaseComponent::doRecalcAnimAndWtm);
    das::addExtern<DAS_CALL_METHOD(method_doRecalcAnimAndWtm)>(*this, lib, "animchar_doRecalcAnimAndWtm",
      das::SideEffects::modifyArgument, DAS_CALL_MEMBER_CPP(::AnimCharV20::AnimcharBaseComponent::doRecalcAnimAndWtm));

    using method_initAttachmentTmAndNodeWtm = DAS_CALL_MEMBER(::AnimCharV20::AnimcharBaseComponent::initAttachmentTmAndNodeWtm);
    das::addExtern<DAS_CALL_METHOD(method_initAttachmentTmAndNodeWtm)>(*this, lib, "animchar_initAttachmentTmAndNodeWtm",
      das::SideEffects::modifyArgument, DAS_CALL_MEMBER_CPP(::AnimCharV20::AnimcharBaseComponent::initAttachmentTmAndNodeWtm));

    das::addExtern<DAS_BIND_FUN(animchar_getSlotNodeIdx)>(*this, lib, "animchar_getSlotNodeIdx", das::SideEffects::none,
      "bind_dascript::animchar_getSlotNodeIdx");

    using method_getSlotNodeWtm = DAS_CALL_MEMBER(::AnimCharV20::AnimcharBaseComponent::getSlotNodeWtm);
    das::addExtern<DAS_CALL_METHOD(method_getSlotNodeWtm)>(*this, lib, "animchar_getSlotNodeWtm", das::SideEffects::none,
      DAS_CALL_MEMBER_CPP(::AnimCharV20::AnimcharBaseComponent::getSlotNodeWtm));

    using method_getAttachmentTm = DAS_CALL_MEMBER(::AnimCharV20::AnimcharBaseComponent::getAttachmentTm);
    das::addExtern<DAS_CALL_METHOD(method_getAttachmentTm)>(*this, lib, "animchar_getAttachmentTm", das::SideEffects::none,
      DAS_CALL_MEMBER_CPP(::AnimCharV20::AnimcharBaseComponent::getAttachmentTm));

    using method_setAttachedChar = DAS_CALL_MEMBER(::AnimCharV20::AnimcharBaseComponent::setAttachedChar);
    das::addExtern<DAS_CALL_METHOD(method_setAttachedChar)>(*this, lib, "animchar_setAttachedChar", das::SideEffects::worstDefault,
      DAS_CALL_MEMBER_CPP(::AnimCharV20::AnimcharBaseComponent::setAttachedChar));

    using method_getAttachmentSlotsCount = DAS_CALL_MEMBER(::AnimCharV20::AnimcharBaseComponent::getAttachmentSlotsCount);
    das::addExtern<DAS_CALL_METHOD(method_getAttachmentSlotsCount)>(*this, lib, "animchar_getAttachmentSlotsCount",
      das::SideEffects::none, DAS_CALL_MEMBER_CPP(::AnimCharV20::AnimcharBaseComponent::getAttachmentSlotsCount));

    using method_getAttachmentSlotId = DAS_CALL_MEMBER(::AnimCharV20::AnimcharBaseComponent::getAttachmentSlotId);
    das::addExtern<DAS_CALL_METHOD(method_getAttachmentSlotId)>(*this, lib, "animchar_getAttachmentSlotId", das::SideEffects::none,
      DAS_CALL_MEMBER_CPP(::AnimCharV20::AnimcharBaseComponent::getAttachmentSlotId));

    using method_getAttachedChar = DAS_CALL_MEMBER(::AnimCharV20::AnimcharBaseComponent::getAttachedChar);
    das::addExtern<DAS_CALL_METHOD(method_getAttachedChar)>(*this, lib, "animchar_getAttachedChar", das::SideEffects::none,
      DAS_CALL_MEMBER_CPP(::AnimCharV20::AnimcharBaseComponent::getAttachedChar));

    using method_getAttachmentUid = DAS_CALL_MEMBER(::AnimCharV20::AnimcharBaseComponent::getAttachmentUid);
    das::addExtern<DAS_CALL_METHOD(method_getAttachmentUid)>(*this, lib, "animchar_getAttachmentUid", das::SideEffects::none,
      DAS_CALL_MEMBER_CPP(::AnimCharV20::AnimcharBaseComponent::getAttachmentUid));

    using method_releaseAttachment = DAS_CALL_MEMBER(::AnimCharV20::AnimcharBaseComponent::releaseAttachment);
    das::addExtern<DAS_CALL_METHOD(method_releaseAttachment)>(*this, lib, "animchar_releaseAttachment", das::SideEffects::worstDefault,
      DAS_CALL_MEMBER_CPP(::AnimCharV20::AnimcharBaseComponent::releaseAttachment));

    using method_setTmRel = DAS_CALL_MEMBER(::AnimCharV20::AnimcharBaseComponent::setTmRel);
    das::addExtern<DAS_CALL_METHOD(method_setTmRel)>(*this, lib, "animchar_setTmRel", das::SideEffects::modifyArgument,
      DAS_CALL_MEMBER_CPP(::AnimCharV20::AnimcharBaseComponent::setTmRel));

    using method_reset = DAS_CALL_MEMBER(::AnimCharV20::AnimcharBaseComponent::reset);
    das::addExtern<DAS_CALL_METHOD(method_reset)>(*this, lib, "animchar_reset", das::SideEffects::modifyArgument,
      DAS_CALL_MEMBER_CPP(::AnimCharV20::AnimcharBaseComponent::reset));

    using method_setFastPhysSystemGravityDirection =
      DAS_CALL_MEMBER(::AnimCharV20::AnimcharBaseComponent::setFastPhysSystemGravityDirection);
    das::addExtern<DAS_CALL_METHOD(method_setFastPhysSystemGravityDirection)>(*this, lib, "animchar_setFastPhysSystemGravityDirection",
      das::SideEffects::modifyArgument, DAS_CALL_MEMBER_CPP(::AnimCharV20::AnimcharBaseComponent::setFastPhysSystemGravityDirection));

    using method_updateFastPhys = DAS_CALL_MEMBER(::AnimCharV20::AnimcharBaseComponent::updateFastPhys);
    das::addExtern<DAS_CALL_METHOD(method_updateFastPhys)>(*this, lib, "animchar_updateFastPhys", das::SideEffects::modifyArgument,
      DAS_CALL_MEMBER_CPP(::AnimCharV20::AnimcharBaseComponent::updateFastPhys));

    using method_resetFastPhysWtmOfs = DAS_CALL_MEMBER(::AnimCharV20::AnimcharBaseComponent::resetFastPhysWtmOfs);
    das::addExtern<DAS_CALL_METHOD(method_resetFastPhysWtmOfs)>(*this, lib, "animchar_resetFastPhysWtmOfs",
      das::SideEffects::modifyArgument, DAS_CALL_MEMBER_CPP(::AnimCharV20::AnimcharBaseComponent::resetFastPhysWtmOfs));

    das::addExtern<DAS_BIND_FUN(animchar_render_prepareSphere)>(*this, lib, "animchar_render_prepareSphere", das::SideEffects::none,
      "bind_dascript::animchar_render_prepareSphere");
    das::addExtern<DAS_BIND_FUN(animchar_render_calcWorldBox), das::SimNode_ExtFuncCallAndCopyOrMove>(*this, lib,
      "animchar_render_calcWorldBox", das::SideEffects::none, "bind_dascript::animchar_render_calcWorldBox");
    das::addExtern<DAS_BIND_FUN(animchar_render_prepareSphereAndCalcBox)>(*this, lib, "animchar_render_prepareSphereAndCalcBox",
      das::SideEffects::modifyArgument, "bind_dascript::animchar_render_prepareSphereAndCalcBox");

    using method_setPbcOverride = DAS_CALL_MEMBER(::AnimV20::AnimcharDebugContext::setPbcOverride);
    das::addExtern<DAS_CALL_METHOD(method_setPbcOverride)>(*this, lib, "animchar_debug_setPbcOverride",
      das::SideEffects::modifyArgument, DAS_CALL_MEMBER_CPP(::AnimV20::AnimcharDebugContext::setPbcOverride));

    using method_disablePbcOverride = DAS_CALL_MEMBER(::AnimV20::AnimcharDebugContext::disablePbcOverride);
    das::addExtern<DAS_CALL_METHOD(method_disablePbcOverride)>(*this, lib, "animchar_debug_disablePbcOverride",
      das::SideEffects::modifyArgument, DAS_CALL_MEMBER_CPP(::AnimV20::AnimcharDebugContext::disablePbcOverride));

    using method_getPbcOverrideEnabled = DAS_CALL_MEMBER(::AnimV20::AnimcharDebugContext::getPbcOverrideEnabled);
    das::addExtern<DAS_CALL_METHOD(method_getPbcOverrideEnabled)>(*this, lib, "animchar_debug_getPbcOverrideEnabled",
      das::SideEffects::modifyArgument, DAS_CALL_MEMBER_CPP(::AnimV20::AnimcharDebugContext::getPbcOverrideEnabled));

    using method_getPbcOverrideValue = DAS_CALL_MEMBER(::AnimV20::AnimcharDebugContext::getPbcOverrideValue);
    das::addExtern<DAS_CALL_METHOD(method_getPbcOverrideValue)>(*this, lib, "animchar_debug_getPbcOverrideValue",
      das::SideEffects::modifyArgument, DAS_CALL_MEMBER_CPP(::AnimV20::AnimcharDebugContext::getPbcOverrideValue));

    das::addExtern<DAS_BIND_FUN(AnimcharNodesMat44_getWtms)>(*this, lib, "AnimcharNodesMat44_getWtms",
      das::SideEffects::accessExternal, "bind_dascript::AnimcharNodesMat44_getWtms");

#define DAS_BIND_MEMBER(fn, side_effect, name)                                                       \
  {                                                                                                  \
    using method = DAS_CALL_MEMBER(fn);                                                              \
    das::addExtern<DAS_CALL_METHOD(method)>(*this, lib, name, side_effect, DAS_CALL_MEMBER_CPP(fn)); \
  }

    DAS_BIND_MEMBER(::AnimV20::AnimationGraph::getParamName, das::SideEffects::none, "anim_graph_getParamName")
    DAS_BIND_MEMBER(::AnimV20::AnimationGraph::getParamType, das::SideEffects::none, "anim_graph_getParamType")
    DAS_BIND_MEMBER(::AnimV20::AnimationGraph::getAnimNodeName, das::SideEffects::none, "anim_graph_getAnimNodeName")
    using method_getStateName = DAS_CALL_MEMBER(::AnimCharV20::AnimationGraph::getStateName);
    das::addExternTempRef<DAS_CALL_METHOD(method_getStateName), das::SimNode_ExtFuncCall>(*this, lib, "anim_graph_getStateName",
      das::SideEffects::accessExternal, DAS_CALL_MEMBER_CPP(::AnimCharV20::AnimationGraph::getStateName));
    DAS_BIND_MEMBER(::AnimV20::AnimationGraph::getStateNameByStateIdx, das::SideEffects::none, "anim_graph_getStateNameByStateIdx")
    DAS_BIND_MEMBER(::AnimV20::AnimationGraph::getBlendNodeName, das::SideEffects::none, "anim_graph_getBlendNodeName")

    DAS_BIND_MEMBER(::AnimV20::IAnimStateHolder::getParamIdValid, das::SideEffects::none, "anim_state_holder_getParamIdValid")
    DAS_BIND_MEMBER(::AnimV20::IAnimStateHolder::getParam, das::SideEffects::none, "anim_state_holder_getParam")
    DAS_BIND_MEMBER(::AnimV20::IAnimStateHolder::getParamInt, das::SideEffects::none, "anim_state_holder_getParamInt")
    {
      using method = das::das_call_member<void *(::AnimV20::IAnimStateHolder::*)(int), &::AnimV20::IAnimStateHolder::getInlinePtr>;
      das::addExtern<DAS_CALL_METHOD(method)>(*this, lib, "anim_state_holder_getInlinePtr", das::SideEffects::modifyArgument,
        "das::das_call_member< void *(::AnimV20::IAnimStateHolder::*)(int), &::AnimV20::IAnimStateHolder::getInlinePtr >::invoke");
      using const_method =
        das::das_call_member<const void *(::AnimV20::IAnimStateHolder::*)(int) const, &::AnimV20::IAnimStateHolder::getInlinePtr>;
      das::addExtern<DAS_CALL_METHOD(const_method)>(*this, lib, "anim_state_holder_getInlinePtrConst", das::SideEffects::none,
        "das::das_call_member< const void *(::AnimV20::IAnimStateHolder::*)(int) const, "
        "&::AnimV20::IAnimStateHolder::getInlinePtr >::invoke");
    }
    DAS_BIND_MEMBER(::AnimV20::IAnimStateHolder::getInlinePtrMaxSz, das::SideEffects::none, "anim_state_holder_getInlinePtrMaxSz")
    DAS_BIND_MEMBER(::AnimV20::IAnimStateHolder::getTimeScaleParamId, das::SideEffects::none, "anim_state_holder_getTimeScaleParamId")
    DAS_BIND_MEMBER(::AnimV20::IAnimStateHolder::getParamEffTimeScale, das::SideEffects::none,
      "anim_state_holder_getParamEffTimeScale")
    DAS_BIND_MEMBER(::AnimV20::IAnimStateHolder::getParamFlags, das::SideEffects::none, "anim_state_holder_getParamFlags")
    // DAS_BIND_MEMBER(::AnimV20::IAnimStateHolder::getParamScalarPtr, das::SideEffects::none, "anim_state_holder_getParamScalarPtr")
    DAS_BIND_MEMBER(::AnimV20::IAnimStateHolder::setParam, das::SideEffects::modifyArgument, "anim_state_holder_setParam")
    DAS_BIND_MEMBER(::AnimV20::IAnimStateHolder::setParamInt, das::SideEffects::modifyArgument, "anim_state_holder_setParamInt")
    DAS_BIND_MEMBER(::AnimV20::IAnimStateHolder::setTimeScaleParamId, das::SideEffects::modifyArgument,
      "anim_state_holder_setTimeScaleParamId")
    DAS_BIND_MEMBER(::AnimV20::IAnimStateHolder::setParamFlags, das::SideEffects::modifyArgument, "anim_state_holder_setParamFlags")
    DAS_BIND_MEMBER(::AnimV20::IAnimStateHolder::advance, das::SideEffects::modifyArgument, "anim_state_holder_advance")
    DAS_BIND_MEMBER(::AnimV20::IAnimStateHolder::term, das::SideEffects::worstDefault, "anim_state_holder_term")
    DAS_BIND_MEMBER(::AnimV20::IAnimStateHolder::init, das::SideEffects::worstDefault, "anim_state_holder_init")
    das::addExtern<DAS_BIND_FUN(anim_state_holder_dumpStateText)>(*this, lib, "anim_state_holder_dumpStateText",
      das::SideEffects::none, "bind_dascript::anim_state_holder_dumpStateText");

    DAS_BIND_MEMBER(::AnimV20::AnimBlendCtrl_1axis::getParamId, das::SideEffects::none, "anim_blend_node_getParamId")
    DAS_BIND_MEMBER(::AnimV20::AnimBlendCtrl_Fifo3::getParamId, das::SideEffects::none, "anim_blend_node_getParamId")
    DAS_BIND_MEMBER(::AnimV20::AnimBlendCtrl_Fifo3::isEnqueued, das::SideEffects::modifyArgument, "anim_blend_ctrl_fifo3_isEnqueued")
    DAS_BIND_MEMBER(::AnimV20::AnimBlendCtrl_Fifo3::enqueueState, das::SideEffects::modifyArgument,
      "anim_blend_ctrl_fifo3_enqueueState")
    DAS_BIND_MEMBER(::AnimV20::AnimBlendCtrl_RandomSwitcher::getParamId, das::SideEffects::none, "anim_blend_node_getParamId")
    DAS_BIND_MEMBER(::AnimV20::AnimBlendCtrl_RandomSwitcher::getRepParamId, das::SideEffects::none, "anim_blend_node_getRepParamId")

    DAS_BIND_MEMBER(::AnimV20::AnimBlendCtrl_ParametricSwitcher::getParamId, das::SideEffects::none, "anim_blend_node_getParamId")
    DAS_BIND_MEMBER(::AnimV20::AnimBlendCtrl_Hub::getParamId, das::SideEffects::none, "anim_blend_node_getParamId")
    DAS_BIND_MEMBER(::AnimV20::AnimBlendCtrl_Blender::getParamId, das::SideEffects::none, "anim_blend_node_getParamId")
    DAS_BIND_MEMBER(::AnimV20::AnimBlendCtrl_BinaryIndirectSwitch::getParamId, das::SideEffects::none, "anim_blend_node_getParamId")
    DAS_BIND_MEMBER(::AnimV20::AnimBlendCtrl_LinearPoly::getParamId, das::SideEffects::none, "anim_blend_node_getParamId")
    DAS_BIND_MEMBER(::AnimV20::AnimBlendNodeParametricLeaf::getParamId, das::SideEffects::none, "anim_blend_node_getParamId")
    DAS_BIND_MEMBER(::AnimV20::AnimBlendNodeContinuousLeaf::getParamId, das::SideEffects::none, "anim_blend_node_getParamId")
    DAS_BIND_MEMBER(::AnimV20::AnimBlendNodeStillLeaf::getPos, das::SideEffects::none, "anim_blend_node_getPos")


#define ALL_ANIM_CTRLS                            \
  ANIM_CTRL(IAnimBlendNode)                       \
  ANIM_CTRL(AnimBlendNodeNull)                    \
  ANIM_CTRL(AnimBlendNodeStillLeaf)               \
  ANIM_CTRL(AnimBlendNodeContinuousLeaf)          \
  ANIM_CTRL(AnimBlendNodeParametricLeaf)          \
  ANIM_CTRL(AnimBlendNodeSingleLeaf)              \
  ANIM_CTRL(AnimBlendCtrl_1axis)                  \
  ANIM_CTRL(AnimBlendCtrl_Fifo3)                  \
  ANIM_CTRL(AnimBlendCtrl_RandomSwitcher)         \
  ANIM_CTRL(AnimBlendCtrl_Hub)                    \
  ANIM_CTRL(AnimBlendCtrl_Blender)                \
  ANIM_CTRL(AnimBlendCtrl_BinaryIndirectSwitch)   \
  ANIM_CTRL(AnimBlendCtrl_SetMotionMatchingTag)   \
  ANIM_CTRL(AnimBlendCtrl_LinearPoly)             \
  ANIM_CTRL(AnimBlendCtrl_ParametricSwitcher)     \
  ANIM_CTRL(AnimBlendNodeLeaf)                    \
  ANIM_CTRL(AnimPostBlendCtrl)                    \
  ANIM_CTRL(AnimPostBlendAlignCtrl)               \
  ANIM_CTRL(AnimPostBlendAlignExCtrl)             \
  ANIM_CTRL(AnimPostBlendRotateCtrl)              \
  ANIM_CTRL(AnimPostBlendRotateAroundCtrl)        \
  ANIM_CTRL(AnimPostBlendScaleCtrl)               \
  ANIM_CTRL(AnimPostBlendMoveCtrl)                \
  ANIM_CTRL(AnimPostBlendCondHideCtrl)            \
  ANIM_CTRL(AnimPostBlendAimCtrl)                 \
  ANIM_CTRL(ApbParamCtrl)                         \
  ANIM_CTRL(DefClampParamCtrl)                    \
  ANIM_CTRL(ApbAnimateCtrl)                       \
  ANIM_CTRL(LegsIKCtrl)                           \
  ANIM_CTRL(DeltaAnglesCtrl)                      \
  ANIM_CTRL(MultiChainFABRIKCtrl)                 \
  ANIM_CTRL(AnimPostBlendNodeLookatCtrl)          \
  ANIM_CTRL(AnimPostBlendNodeLookatNodeCtrl)      \
  ANIM_CTRL(AnimPostBlendEffFromAttachement)      \
  ANIM_CTRL(AnimPostBlendNodesFromAttachement)    \
  ANIM_CTRL(AnimPostBlendParamFromNode)           \
  ANIM_CTRL(AnimPostBlendMatVarFromNode)          \
  ANIM_CTRL(AnimPostBlendCompoundRotateShift)     \
  ANIM_CTRL(AnimPostBlendSetParam)                \
  ANIM_CTRL(AnimPostBlendTwistCtrl)               \
  ANIM_CTRL(AnimPostBlendNodeEffectorFromChildIK) \
  ANIM_CTRL(DeltaRotateShiftCtrl)                 \
  ANIM_CTRL(FootLockerIKCtrl)                     \
  ANIM_CTRL(AnimPostBlendHasAttachment)           \
  ANIM_CTRL(AnimPostBlendHumanAimCtrl)            \
  ANIM_CTRL(AnimPostBlendTwoBonesIK)              \
  ANIM_CTRL(AttachGeomNodeCtrl)
#define ANIM_CTRL(type) das::addConstant(*this, #type "CID", ::AnimV20::type##CID.id);
    ALL_ANIM_CTRLS
#undef ANIM_CTRL

    DAS_BIND_MEMBER(::AnimV20::IAnimBlendNode::class_name, das::SideEffects::none, "anim_blend_node_class_name");
    DAS_BIND_MEMBER(::AnimV20::IAnimBlendNode::getDuration, das::SideEffects::none, "anim_blend_node_getDuration");
    DAS_BIND_MEMBER(::AnimV20::IAnimBlendNode::getAvgSpeed, das::SideEffects::none, "anim_blend_node_getAvgSpeed");
    DAS_BIND_MEMBER(::AnimV20::IAnimBlendNode::getTimeScaleParamId, das::SideEffects::none, "anim_blend_node_getTimeScaleParamId");
    DAS_BIND_MEMBER(::AnimV20::IAnimBlendNode::getAnimNodeId, das::SideEffects::none, "anim_blend_node_getAnimNodeId");
    DAS_BIND_MEMBER(::AnimV20::IAnimBlendNode::resume, das::SideEffects::modifyArgument, "anim_blend_node_resume");
    das::addExtern<DAS_BIND_FUN(bind_dascript::anim_blend_node_isSubOf)>(*this, lib, "anim_blend_node_isSubOf", das::SideEffects::none,
      "bind_dascript::anim_blend_node_isSubOf");

    using anim_graph_getBlendNodePtr = das::das_call_member<::AnimV20::IAnimBlendNode *(::AnimV20::AnimationGraph::*)(int) const,
      &::AnimV20::AnimationGraph::getBlendNodePtr>;
    das::addExtern<DAS_CALL_METHOD(anim_graph_getBlendNodePtr)>(*this, lib, "anim_graph_getBlendNodePtr",
      das::SideEffects::accessExternal,
      "das::das_call_member< ::AnimV20::IAnimBlendNode *(::AnimV20::AnimationGraph::*)(int) const,"
      " &::AnimV20::AnimationGraph::getBlendNodePtr>::invoke");

    using Animate2ndPassCtx_release = DAS_CALL_MEMBER(::Animate2ndPassCtx::release);
    das::addExtern<DAS_CALL_METHOD(Animate2ndPassCtx_release)>(*this, lib, "animate_2nd_pass_ctx_release",
      das::SideEffects::modifyArgument, DAS_CALL_MEMBER_CPP(::Animate2ndPassCtx::release));

    das::addConstant(*this, "PT_Reserved", (uint32_t)::AnimV20::AnimCommonStateHolder::PT_Reserved);
    das::addConstant(*this, "PT_ScalarParam", (uint32_t)::AnimV20::AnimCommonStateHolder::PT_ScalarParam);
    das::addConstant(*this, "PT_ScalarParamInt", (uint32_t)::AnimV20::AnimCommonStateHolder::PT_ScalarParamInt);
    das::addConstant(*this, "PT_TimeParam", (uint32_t)::AnimV20::AnimCommonStateHolder::PT_TimeParam);
    das::addConstant(*this, "PT_InlinePtr", (uint32_t)::AnimV20::AnimCommonStateHolder::PT_InlinePtr);
    das::addConstant(*this, "PT_InlinePtrCTZ", (uint32_t)::AnimV20::AnimCommonStateHolder::PT_InlinePtrCTZ);
    das::addConstant(*this, "PT_Fifo3", (uint32_t)::AnimV20::AnimCommonStateHolder::PT_Fifo3);
    das::addConstant(*this, "PT_Effector", (uint32_t)::AnimV20::AnimCommonStateHolder::PT_Effector);
    das::addConstant(*this, "PF_Paused", (uint32_t)::AnimV20::AnimCommonStateHolder::PF_Paused);
    das::addConstant(*this, "PF_Changed", (uint32_t)::AnimV20::AnimCommonStateHolder::PF_Changed);

#undef DAS_BIND_MEMBER
#undef ALL_ANIM_CTRLS

    auto pType = das::make_smart<das::TypeDecl>(das::Type::tUInt);
    pType->alias = "animchar_visbits_t";
    addAlias(pType);

    compileBuiltinModule("animchar.das", (unsigned char *)animchar_das, sizeof(animchar_das));
    verifyAotReady();
  }

  virtual das::ModuleAotType aotRequire(das::TextWriter &tw) const override
  {
    tw << "#include <dasModules/aotAnimchar.h>\n";
    return das::ModuleAotType::cpp;
  }
};
} // namespace bind_dascript

REGISTER_MODULE_IN_NAMESPACE(AnimV20, bind_dascript);
