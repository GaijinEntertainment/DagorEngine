// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include <math/dag_geomTree.h>
#include "motion_matching.h"

struct RootMotionAnnotation final : das::ManagedStructureAnnotation<RootMotion, false, false>
{
  RootMotionAnnotation(das::ModuleLibrary &ml) : ManagedStructureAnnotation("RootMotion", ml, "RootMotion")
  {
    addFieldEx("translation", "translation", offsetof(RootMotion, translation), das::makeType<das::float3>(ml));
    addFieldEx("rotation", "rotation", offsetof(RootMotion, rotation), das::makeType<das::float4>(ml));
  }
};

struct AnimationClipAnnotation final : das::ManagedStructureAnnotation<AnimationClip, false, false>
{
  AnimationClipAnnotation(das::ModuleLibrary &ml) : ManagedStructureAnnotation("AnimationClip", ml, "AnimationClip")
  {
    addField<DAS_BIND_MANAGED_FIELD(name)>("name");
    addField<DAS_BIND_MANAGED_FIELD(duration)>("duration");
    addField<DAS_BIND_MANAGED_FIELD(tickDuration)>("tickDuration");
    addField<DAS_BIND_MANAGED_FIELD(looped)>("looped");
    addField<DAS_BIND_MANAGED_FIELD(animTreeTimer)>("animTreeTimer");
    addField<DAS_BIND_MANAGED_FIELD(animTreeTimerCycle)>("animTreeTimerCycle");
    addFieldEx("rootMotion", "rootMotion", offsetof(AnimationClip, rootMotion), das::makeType<dag::Vector<RootMotion>>(ml));
  }
};

struct AnimationDataBaseAnnotation final : das::ManagedStructureAnnotation<AnimationDataBase, false, false>
{
  AnimationDataBaseAnnotation(das::ModuleLibrary &ml) : ManagedStructureAnnotation("AnimationDataBase", ml, "AnimationDataBase")
  {
    addField<DAS_BIND_MANAGED_FIELD(clips)>("clips");
    addField<DAS_BIND_MANAGED_FIELD(tagsPresets)>("tagsPresets");
    addField<DAS_BIND_MANAGED_FIELD(nodesName)>("nodesName");
    addField<DAS_BIND_MANAGED_FIELD(predictionTimes)>("predictionTimes");
    addField<DAS_BIND_MANAGED_FIELD(nodeCount)>("nodeCount");
    addField<DAS_BIND_MANAGED_FIELD(trajectorySize)>("trajectorySize");
    addField<DAS_BIND_MANAGED_FIELD(featuresSize)>("featuresSize");
    addField<DAS_BIND_MANAGED_FIELD(footLockerParamId)>("footLockerParamId");
  }
};

struct AnimationFilterTagsAnnotation final : das::ManagedStructureAnnotation<AnimationFilterTags, false, false>
{
  AnimationFilterTagsAnnotation(das::ModuleLibrary &ml) : ManagedStructureAnnotation("AnimationFilterTags", ml, "AnimationFilterTags")
  {
    addField<DAS_BIND_MANAGED_FIELD(wasChanged)>("wasChanged");
  }
};


struct MotionMatchingControllerAnnotation final : das::ManagedStructureAnnotation<MotionMatchingController, false, false>
{
  MotionMatchingControllerAnnotation(das::ModuleLibrary &ml) :
    ManagedStructureAnnotation("MotionMatchingController", ml, "MotionMatchingController")
  {
    addField<DAS_BIND_MANAGED_FIELD(dataBase)>("dataBase");
    addFieldEx("rootPosition", "rootPosition", offsetof(MotionMatchingController, rootPosition), das::makeType<das::float3>(ml));
    addFieldEx("rootRotation", "rootRotation", offsetof(MotionMatchingController, rootRotation), das::makeType<das::float4>(ml));
    addField<DAS_BIND_MANAGED_FIELD(offset)>("offset");
    addField<DAS_BIND_MANAGED_FIELD(currentAnimation)>("currentAnimation");
    addField<DAS_BIND_MANAGED_FIELD(resultAnimation)>("resultAnimation");
    addField<DAS_BIND_MANAGED_FIELD(currentTags)>("currentTags");
    addField<DAS_BIND_MANAGED_FIELD(useTagsFromAnimGraph)>("useTagsFromAnimGraph");
    addField<DAS_BIND_MANAGED_FIELD(playSpeedMult)>("playSpeedMult");
    addField<DAS_BIND_MANAGED_FIELD(rootSynchronization)>("rootSynchronization");
    addField<DAS_BIND_MANAGED_FIELD(rootAdjustment)>("rootAdjustment");
    addField<DAS_BIND_MANAGED_FIELD(rootClamping)>("rootClamping");
    addField<DAS_BIND_MANAGED_FIELD(rootAdjustmentVelocityRatio)>("rootAdjustmentVelocityRatio");
    addField<DAS_BIND_MANAGED_FIELD(rootAdjustmentPosTime)>("rootAdjustmentPosTime");
    addField<DAS_BIND_MANAGED_FIELD(rootAdjustmentAngVelocityRatio)>("rootAdjustmentAngVelocityRatio");
    addField<DAS_BIND_MANAGED_FIELD(rootAdjustmentRotTime)>("rootAdjustmentRotTime");
    addField<DAS_BIND_MANAGED_FIELD(rootClampingMaxDistance)>("rootClampingMaxDistance");
    addField<DAS_BIND_MANAGED_FIELD(rootClampingMaxAngle)>("rootClampingMaxAngle");
  }
};

struct BoneInertialInfoAnnotation final : das::ManagedStructureAnnotation<BoneInertialInfo, false, false>
{
  BoneInertialInfoAnnotation(das::ModuleLibrary &ml) : ManagedStructureAnnotation("BoneInertialInfo", ml, "BoneInertialInfo")
  {
    addFieldEx("position", "position", offsetof(BoneInertialInfo, position), das::makeType<dag::Vector<das::float4>>(ml));
    addFieldEx("rotation", "rotation", offsetof(BoneInertialInfo, rotation), das::makeType<dag::Vector<das::float4>>(ml));
  }
};

struct FeatureWeightsAnnotation final : das::ManagedStructureAnnotation<FeatureWeights, false, false>
{
  FeatureWeightsAnnotation(das::ModuleLibrary &ml) : ManagedStructureAnnotation("FeatureWeights", ml, "FeatureWeights")
  {
    addField<DAS_BIND_MANAGED_FIELD(rootPositions)>("rootPositions");
    addField<DAS_BIND_MANAGED_FIELD(rootDirections)>("rootDirections");
    addField<DAS_BIND_MANAGED_FIELD(nodePositions)>("nodePositions");
    addField<DAS_BIND_MANAGED_FIELD(nodeVelocities)>("nodeVelocities");
  }
};

struct TagPresetAnnotation final : das::ManagedStructureAnnotation<TagPreset, false, false>
{
  TagPresetAnnotation(das::ModuleLibrary &ml) : ManagedStructureAnnotation("TagPreset", ml, "TagPreset")
  {
    addField<DAS_BIND_MANAGED_FIELD(weights)>("weights");
    addField<DAS_BIND_MANAGED_FIELD(requiredTagIdx)>("requiredTagIdx");
    addField<DAS_BIND_MANAGED_FIELD(animationBlendTime)>("animationBlendTime");
    addField<DAS_BIND_MANAGED_FIELD(linearVelocityViscosity)>("linearVelocityViscosity");
    addField<DAS_BIND_MANAGED_FIELD(angularVelocityViscosity)>("angularVelocityViscosity");
    addField<DAS_BIND_MANAGED_FIELD(metricaToleranceMin)>("metricaToleranceMin");
    addField<DAS_BIND_MANAGED_FIELD(metricaToleranceMax)>("metricaToleranceMax");
    addField<DAS_BIND_MANAGED_FIELD(metricaToleranceDecayTime)>("metricaToleranceDecayTime");
    addField<DAS_BIND_MANAGED_FIELD(presetBlendTime)>("presetBlendTime");
  }
};

struct FootLockerIKCtrlLegDataAnnotation final : das::ManagedStructureAnnotation<AnimV20::FootLockerIKCtrl::LegData, false, false>
{
  FootLockerIKCtrlLegDataAnnotation(das::ModuleLibrary &ml) :
    ManagedStructureAnnotation("FootLockerIKCtrlLegData", ml, "AnimV20::FootLockerIKCtrl::LegData")
  {
    addField<DAS_BIND_MANAGED_FIELD(lockedPosition)>("lockedPosition");
    addField<DAS_BIND_MANAGED_FIELD(posOffset)>("posOffset");
    addField<DAS_BIND_MANAGED_FIELD(ankleVerticalMove)>("ankleVerticalMove");
    addField<DAS_BIND_MANAGED_FIELD(ankleTargetMove)>("ankleTargetMove");
    addField<DAS_BIND_MANAGED_FIELD(isLocked)>("isLocked");
    addField<DAS_BIND_MANAGED_FIELD(needLock)>("needLock");
  }
  bool canBePlacedInContainer() const override { return true; }
};

namespace bind_dascript
{
class MotionMatchingModule final : public das::Module
{
public:
  MotionMatchingModule() : das::Module("MotionMatching")
  {
    das::ModuleLibrary lib(this);
    addAnnotation(das::make_smart<RootMotionAnnotation>(lib));
    addAnnotation(das::make_smart<FeatureWeightsAnnotation>(lib));
    addAnnotation(das::make_smart<TagPresetAnnotation>(lib));
    addAnnotation(das::make_smart<AnimationClipAnnotation>(lib));
    addAnnotation(das::make_smart<AnimationDataBaseAnnotation>(lib));
    addAnnotation(das::make_smart<AnimationFilterTagsAnnotation>(lib));
    addAnnotation(das::make_smart<BoneInertialInfoAnnotation>(lib));
    addAnnotation(das::make_smart<MotionMatchingControllerAnnotation>(lib));
    addAnnotation(das::make_smart<FootLockerIKCtrlLegDataAnnotation>(lib));

    das::typeFactory<TagPresetVector>::make(lib);

    using method_getTagsCount = DAS_CALL_MEMBER(AnimationDataBase::getTagsCount);
    das::addExtern<DAS_CALL_METHOD(method_getTagsCount)>(*this, lib, "getTagsCount", das::SideEffects::none,
      DAS_CALL_MEMBER_CPP(AnimationDataBase::getTagsCount));
    using method_getTagIdx = DAS_CALL_MEMBER(AnimationDataBase::getTagIdx);
    das::addExtern<DAS_CALL_METHOD(method_getTagIdx)>(*this, lib, "getTagIdx", das::SideEffects::none,
      DAS_CALL_MEMBER_CPP(AnimationDataBase::getTagIdx));
    using method_getTagName = DAS_CALL_MEMBER(AnimationDataBase::getTagName);
    das::addExtern<DAS_CALL_METHOD(method_getTagName)>(*this, lib, "getTagName", das::SideEffects::none,
      DAS_CALL_MEMBER_CPP(AnimationDataBase::getTagName));
    using method_hasAnimGraphTags = DAS_CALL_MEMBER(AnimationDataBase::hasAnimGraphTags);
    das::addExtern<DAS_CALL_METHOD(method_hasAnimGraphTags)>(*this, lib, "hasAnimGraphTags", das::SideEffects::none,
      DAS_CALL_MEMBER_CPP(AnimationDataBase::hasAnimGraphTags));
    using method_hasAvailableAnimations = DAS_CALL_MEMBER(AnimationDataBase::hasAvailableAnimations);
    das::addExtern<DAS_CALL_METHOD(method_hasAvailableAnimations)>(*this, lib, "hasAvailableAnimations", das::SideEffects::none,
      DAS_CALL_MEMBER_CPP(AnimationDataBase::hasAvailableAnimations));
    using method_changePBCWeightOverride = DAS_CALL_MEMBER(AnimationDataBase::changePBCWeightOverride);
    das::addExtern<DAS_CALL_METHOD(method_changePBCWeightOverride)>(*this, lib, "changePBCWeightOverride",
      das::SideEffects::modifyArgument, DAS_CALL_MEMBER_CPP(AnimationDataBase::changePBCWeightOverride));

    using method_requireTag = DAS_CALL_MEMBER(AnimationFilterTags::requireTag);
    das::addExtern<DAS_CALL_METHOD(method_requireTag)>(*this, lib, "requireTag", das::SideEffects::modifyArgument,
      DAS_CALL_MEMBER_CPP(AnimationFilterTags::requireTag));
    using method_excludeTag = DAS_CALL_MEMBER(AnimationFilterTags::excludeTag);
    das::addExtern<DAS_CALL_METHOD(method_excludeTag)>(*this, lib, "excludeTag", das::SideEffects::modifyArgument,
      DAS_CALL_MEMBER_CPP(AnimationFilterTags::excludeTag));
    using method_removeTag = DAS_CALL_MEMBER(AnimationFilterTags::removeTag);
    das::addExtern<DAS_CALL_METHOD(method_removeTag)>(*this, lib, "removeTag", das::SideEffects::modifyArgument,
      DAS_CALL_MEMBER_CPP(AnimationFilterTags::removeTag));
    using method_isTagRequired = DAS_CALL_MEMBER(AnimationFilterTags::isTagRequired);
    das::addExtern<DAS_CALL_METHOD(method_isTagRequired)>(*this, lib, "isTagRequired", das::SideEffects::none,
      DAS_CALL_MEMBER_CPP(AnimationFilterTags::isTagRequired));
    using method_isTagExcluded = DAS_CALL_MEMBER(AnimationFilterTags::isTagExcluded);
    das::addExtern<DAS_CALL_METHOD(method_isTagExcluded)>(*this, lib, "isTagExcluded", das::SideEffects::none,
      DAS_CALL_MEMBER_CPP(AnimationFilterTags::isTagExcluded));

    using method_playAnimation = DAS_CALL_MEMBER(MotionMatchingController::playAnimation);
    das::addExtern<DAS_CALL_METHOD(method_playAnimation)>(*this, lib, "playAnimation", das::SideEffects::modifyArgument,
      DAS_CALL_MEMBER_CPP(MotionMatchingController::playAnimation));
    using method_getCurrentClip = DAS_CALL_MEMBER(MotionMatchingController::getCurrentClip);
    das::addExtern<DAS_CALL_METHOD(method_getCurrentClip)>(*this, lib, "getCurrentClip", das::SideEffects::none,
      DAS_CALL_MEMBER_CPP(MotionMatchingController::getCurrentClip));
    using method_getCurrentFrame = DAS_CALL_MEMBER(MotionMatchingController::getCurrentFrame);
    das::addExtern<DAS_CALL_METHOD(method_getCurrentFrame)>(*this, lib, "getCurrentFrame", das::SideEffects::none,
      DAS_CALL_MEMBER_CPP(MotionMatchingController::getCurrentFrame));
    using method_getCurrentFrameProgress = DAS_CALL_MEMBER(MotionMatchingController::getCurrentFrameProgress);
    das::addExtern<DAS_CALL_METHOD(method_getCurrentFrameProgress)>(*this, lib, "getCurrentFrameProgress", das::SideEffects::none,
      DAS_CALL_MEMBER_CPP(MotionMatchingController::getCurrentFrameProgress));
    using method_hasActiveAnimation = DAS_CALL_MEMBER(MotionMatchingController::hasActiveAnimation);
    das::addExtern<DAS_CALL_METHOD(method_hasActiveAnimation)>(*this, lib, "hasActiveAnimation", das::SideEffects::none,
      DAS_CALL_MEMBER_CPP(MotionMatchingController::hasActiveAnimation));

    das::addExtern<DAS_BIND_FUN(clearAnimations)>(*this, lib, "clearAnimations", das::SideEffects::modifyArgument, "clearAnimations");
    das::addExtern<DAS_BIND_FUN(get_root_node_id)>(*this, lib, "get_root_node_id", das::SideEffects::none, "get_root_node_id");
    das::addExtern<DAS_BIND_FUN(init_feature_weights)>(*this, lib, "init_feature_weights", das::SideEffects::modifyArgument,
      "init_feature_weights");
    das::addExtern<DAS_BIND_FUN(commit_feature_weights)>(*this, lib, "commit_feature_weights", das::SideEffects::modifyArgument,
      "commit_feature_weights");
    das::addExtern<DAS_BIND_FUN(get_features_sizes)>(*this, lib, "get_features_sizes", das::SideEffects::none, "get_features_sizes");

    das::addExtern<DAS_BIND_FUN(bind_dascript::anim_state_holder_get_foot_locker_legs)>(*this, lib,
      "anim_state_holder_get_foot_locker_legs", das::SideEffects::worstDefault,
      "bind_dascript::anim_state_holder_get_foot_locker_legs");
    das::addExtern<DAS_BIND_FUN(bind_dascript::anim_state_holder_iterate_foot_locker_legs_const)>(*this, lib,
      "anim_state_holder_iterate_foot_locker_legs_const", das::SideEffects::worstDefault,
      "bind_dascript::anim_state_holder_iterate_foot_locker_legs_const");
    das::addExtern<DAS_BIND_FUN(bind_dascript::get_post_blend_controller_idx)>(*this, lib, "get_post_blend_controller_idx",
      das::SideEffects::none, "bind_dascript::get_post_blend_controller_idx");
    das::addExtern<DAS_BIND_FUN(bind_dascript::simple_spring_damper_exact_q)>(*this, lib, "simple_spring_damper_exact_q",
      das::SideEffects::modifyArgument, "bind_dascript::simple_spring_damper_exact_q");

    das::addConstant<int>(*this, "TRAJECTORY_DIMENSION", TRAJECTORY_DIMENSION);
    das::addConstant<int>(*this, "NODE_DIMENSION", NODE_DIMENSION);
    das::addConstant<das::float4>(*this, "FORWARD_DIRECTION", FORWARD_DIRECTION);
    verifyAotReady();
  }
  das::ModuleAotType aotRequire(das::TextWriter &tw) const override
  {
    tw << "#include \"motion_matching/dasModules/motion_matching.h\"\n";
    return das::ModuleAotType::cpp;
  }
};
} // namespace bind_dascript

AUTO_REGISTER_MODULE_IN_NAMESPACE(MotionMatchingModule, bind_dascript);

size_t MotionMatchingModule_pull = 0;