// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include <dasModules/aotAnimchar.h>
#include <dasModules/aotDagorMath.h>
#include <dasModules/dasMacro.h>
#include <dasModules/dasDataBlock.h>
#include <anim/dag_animBlend.h>

struct NameIdPairAnnotation : das::ManagedStructureAnnotation<das::NameIdPair, false>
{
  NameIdPairAnnotation(das::ModuleLibrary &ml) : ManagedStructureAnnotation("NameIdPair", ml)
  {
    cppName = " ::das::NameIdPair";
    addField<DAS_BIND_MANAGED_FIELD(id)>("id");
    addField<DAS_BIND_MANAGED_FIELD(name)>("name");
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

struct AnimationGraphAnnotation : das::ManagedStructureAnnotation<AnimV20::AnimationGraph, false>
{
  AnimationGraphAnnotation(das::ModuleLibrary &ml) : ManagedStructureAnnotation("AnimationGraph", ml)
  {
    cppName = " ::AnimV20::AnimationGraph";

    addProperty<DAS_BIND_MANAGED_PROP(getParamCount)>("paramCount", "getParamCount");
    addProperty<DAS_BIND_MANAGED_PROP(getAnimNodeCount)>("animNodeCount", "getAnimNodeCount");
    addProperty<DAS_BIND_MANAGED_PROP(getStateCount)>("stateCount", "getStateCount");
    addProperty<DAS_BIND_MANAGED_PROP(getRoot)>("root", "getRoot");
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

    addProperty<DAS_BIND_MANAGED_PROP(isAdditive)>("isAdditive");
    addProperty<DAS_BIND_MANAGED_PROP(isAnimationIgnored)>("isAnimationIgnored");
  }
};

struct AnimBlendNodeStillLeafAnnotation : das::ManagedStructureAnnotation<AnimV20::AnimBlendNodeStillLeaf, false>
{
  AnimBlendNodeStillLeafAnnotation(das::ModuleLibrary &ml) : ManagedStructureAnnotation("AnimBlendNodeStillLeaf", ml)
  {
    cppName = " ::AnimV20::AnimBlendNodeStillLeaf";

    addProperty<DAS_BIND_MANAGED_PROP(isAdditive)>("isAdditive");
    addProperty<DAS_BIND_MANAGED_PROP(isAnimationIgnored)>("isAnimationIgnored");
  }
};

struct AnimBlendNodeSingleLeafAnnotation : das::ManagedStructureAnnotation<AnimV20::AnimBlendNodeSingleLeaf, false>
{
  AnimBlendNodeSingleLeafAnnotation(das::ModuleLibrary &ml) : ManagedStructureAnnotation("AnimBlendNodeSingleLeaf", ml)
  {
    cppName = " ::AnimV20::AnimBlendNodeSingleLeaf";

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
  }
};

struct AnimBlendCtrl_ParametricSwitcherAnnotation : das::ManagedStructureAnnotation<AnimV20::AnimBlendCtrl_ParametricSwitcher, false>
{
  AnimBlendCtrl_ParametricSwitcherAnnotation(das::ModuleLibrary &ml) :
    ManagedStructureAnnotation("AnimBlendCtrl_ParametricSwitcher", ml)
  {
    cppName = " ::AnimV20::AnimBlendCtrl_ParametricSwitcher";
  }
};

struct AnimFifo3QueueAnnotation : das::ManagedStructureAnnotation<AnimV20::AnimFifo3Queue, false>
{
  AnimFifo3QueueAnnotation(das::ModuleLibrary &ml) : ManagedStructureAnnotation("AnimFifo3Queue", ml)
  {
    cppName = " ::AnimV20::AnimFifo3Queue";
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

struct AnimcharNodesMat44Annotation : das::ManagedStructureAnnotation<AnimcharNodesMat44, false>
{
  AnimcharNodesMat44Annotation(das::ModuleLibrary &ml) : ManagedStructureAnnotation("AnimcharNodesMat44", ml)
  {
    cppName = " ::AnimcharNodesMat44";
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
    addAnnotation(das::make_smart<NameIdPairAnnotation>(lib));
    addAnnotation(das::make_smart<AnimDataAnnotation>(lib));

    addAnnotation(das::make_smart<IPureAnimStateHolderAnnotation>(lib));
    addAnnotation(das::make_smart<IAnimBlendNodePtrAnnotation>(lib));

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
    add_annotation(this, das::make_smart<AnimBlendCtrl_LinearPolyAnnotation>(lib), iAnimBlendNodeAnn);
    add_annotation(this, das::make_smart<AnimBlendCtrl_ParametricSwitcherAnnotation>(lib), animBlendNodeLeafAnn);
    add_annotation(this, das::make_smart<AnimPostBlendCtrlAnnotation>(lib), iAnimBlendNodeAnn);

    addAnnotation(das::make_smart<AnimFifo3QueueAnnotation>(lib));

    addAnnotation(das::make_smart<AnimBlendCtrl_1axisAnimSliceAnnotation>(lib));
    addAnnotation(das::make_smart<AnimBlendCtrl_RandomSwitcherRandomAnimAnnotation>(lib));
    addAnnotation(das::make_smart<AnimBlendCtrl_LinearPolyAnimPointAnnotation>(lib));
    addAnnotation(das::make_smart<AnimBlendCtrl_ParametricSwitcherItemAnimAnnotation>(lib));
    addAnnotation(das::make_smart<AnimationGraphStateRecAnnotation>(lib));

    addAnnotation(das::make_smart<AnimationGraphAnnotation>(lib));
    addAnnotation(das::make_smart<IAnimStateHolderAnnotation>(lib));
    addAnnotation(das::make_smart<AnimcharBaseComponentAnnotation>(lib));
    addAnnotation(das::make_smart<RoNameMapExIdPatchableTabAnnotation>(lib));
    addAnnotation(das::make_smart<RoNameMapExAnnotation>(lib));
    addAnnotation(das::make_smart<DynSceneResNameMapResourceAnnotation>(lib));
    addAnnotation(das::make_smart<DynamicRenderableSceneLodsResourceAnnotation>(lib));
    addAnnotation(das::make_smart<DynamicRenderableSceneInstanceAnnotation>(lib));
    addAnnotation(das::make_smart<AnimcharRendComponentAnnotation>(lib));
    addAnnotation(das::make_smart<AnimcharNodesMat44Annotation>(lib));
    addAnnotation(das::make_smart<Animate2ndPassCtxAnnotation>(lib));
    das::addExtern<DAS_BIND_FUN(::AnimV20::addEnumValue)>(*this, lib, "animV20_add_enum_value", das::SideEffects::modifyExternal,
      "::AnimV20::addEnumValue");
    das::addExtern<DAS_BIND_FUN(::AnimV20::getEnumValueByName)>(*this, lib, "animV20_get_enum_value_by_name",
      das::SideEffects::accessExternal, "::AnimV20::getEnumValueByName");
    das::addExtern<DAS_BIND_FUN(::AnimV20::getEnumName)>(*this, lib, "animV20_get_enum_name_by_id", das::SideEffects::accessExternal,
      "::AnimV20::getEnumName");
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
    auto sendChangeAnimStateEventExt = das::addExtern<DAS_BIND_FUN(send_change_anim_state_event)>(*this, lib,
      "send_change_anim_state_event", das::SideEffects::modifyExternal, "bind_dascript::send_change_anim_state_event");
    sendChangeAnimStateEventExt->annotations.push_back(
      annotation_declaration(das::make_smart<ConstStringArgCallFunctionAnnotation<1>>()));

    das::addExtern<DAS_BIND_FUN(AnimCharV20::getSlotId)>(*this, lib, "animchar_getSlotId", das::SideEffects::accessExternal,
      "AnimCharV20::getSlotId");
    das::addExtern<DAS_BIND_FUN(AnimCharV20::addSlotId)>(*this, lib, "animchar_addSlotId", das::SideEffects::modifyExternal,
      "AnimCharV20::addSlotId");

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

    using method_getSlotNodeWtm = DAS_CALL_MEMBER(::AnimCharV20::AnimcharBaseComponent::getSlotNodeWtm);
    das::addExtern<DAS_CALL_METHOD(method_getSlotNodeWtm)>(*this, lib, "animchar_getSlotNodeWtm", das::SideEffects::none,
      DAS_CALL_MEMBER_CPP(::AnimCharV20::AnimcharBaseComponent::getSlotNodeWtm));

    using method_getAttachmentTm = DAS_CALL_MEMBER(::AnimCharV20::AnimcharBaseComponent::getAttachmentTm);
    das::addExtern<DAS_CALL_METHOD(method_getAttachmentTm)>(*this, lib, "animchar_getAttachmentTm", das::SideEffects::none,
      DAS_CALL_MEMBER_CPP(::AnimCharV20::AnimcharBaseComponent::getAttachmentTm));

    using method_setTmRel = DAS_CALL_MEMBER(::AnimCharV20::AnimcharBaseComponent::setTmRel);
    das::addExtern<DAS_CALL_METHOD(method_setTmRel)>(*this, lib, "animchar_setTmRel", das::SideEffects::modifyArgument,
      DAS_CALL_MEMBER_CPP(::AnimCharV20::AnimcharBaseComponent::setTmRel));

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


#define DAS_BIND_MEMBER(fn, side_effect, name)                                                       \
  {                                                                                                  \
    using method = DAS_CALL_MEMBER(fn);                                                              \
    das::addExtern<DAS_CALL_METHOD(method)>(*this, lib, name, side_effect, DAS_CALL_MEMBER_CPP(fn)); \
  }

    DAS_BIND_MEMBER(::AnimV20::AnimationGraph::getParamName, das::SideEffects::none, "anim_graph_getParamName")
    DAS_BIND_MEMBER(::AnimV20::AnimationGraph::getParamType, das::SideEffects::none, "anim_graph_getParamType")
    DAS_BIND_MEMBER(::AnimV20::AnimationGraph::getAnimNodeName, das::SideEffects::none, "anim_graph_getAnimNodeName")
    DAS_BIND_MEMBER(::AnimV20::AnimationGraph::getStateName, das::SideEffects::none, "anim_graph_getStateName")
    DAS_BIND_MEMBER(::AnimV20::AnimationGraph::getStateNameByStateIdx, das::SideEffects::none, "anim_graph_getStateNameByStateIdx")
    DAS_BIND_MEMBER(::AnimV20::AnimationGraph::getBlendNodeName, das::SideEffects::none, "anim_graph_getBlendNodeName")

    DAS_BIND_MEMBER(::AnimV20::IAnimStateHolder::getParamIdValid, das::SideEffects::none, "anim_state_holder_getParamIdValid")
    DAS_BIND_MEMBER(::AnimV20::IAnimStateHolder::getParam, das::SideEffects::none, "anim_state_holder_getParam")
    DAS_BIND_MEMBER(::AnimV20::IAnimStateHolder::getParamInt, das::SideEffects::none, "anim_state_holder_getParamInt")
    {
      using method = das::das_call_member<void *(::AnimV20::IAnimStateHolder::*)(int), &::AnimV20::IAnimStateHolder::getInlinePtr>;
      das::addExtern<DAS_CALL_METHOD(method)>(*this, lib, "anim_state_holder_getInlinePtr", das::SideEffects::modifyArgument,
        "das::das_call_member< void *(::AnimV20::IAnimStateHolder::*)(int), &::AnimV20::IAnimStateHolder::getInlinePtr >::invoke");
    }
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


#define ALL_ANIM_CTRLS                          \
  ANIM_CTRL(IAnimBlendNode)                     \
  ANIM_CTRL(AnimBlendNodeNull)                  \
  ANIM_CTRL(AnimBlendNodeStillLeaf)             \
  ANIM_CTRL(AnimBlendNodeContinuousLeaf)        \
  ANIM_CTRL(AnimBlendNodeParametricLeaf)        \
  ANIM_CTRL(AnimBlendNodeSingleLeaf)            \
  ANIM_CTRL(AnimBlendCtrl_1axis)                \
  ANIM_CTRL(AnimBlendCtrl_Fifo3)                \
  ANIM_CTRL(AnimBlendCtrl_RandomSwitcher)       \
  ANIM_CTRL(AnimBlendCtrl_Hub)                  \
  ANIM_CTRL(AnimBlendCtrl_Blender)              \
  ANIM_CTRL(AnimBlendCtrl_BinaryIndirectSwitch) \
  ANIM_CTRL(AnimBlendCtrl_LinearPoly)           \
  ANIM_CTRL(AnimBlendCtrl_ParametricSwitcher)   \
  ANIM_CTRL(AnimBlendNodeLeaf)                  \
  ANIM_CTRL(AnimPostBlendCtrl)
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
