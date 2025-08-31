// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include <animChar/dag_animCharacter2.h>
#include <anim/dag_animBlendCtrl.h>
#include <animChar/dag_animate2ndPass.h>
namespace AnimV20
{
int addEnumValue(const char *) { G_ASSERT_RETURN(false, 0); }
int getEnumValueByName(const char *) { G_ASSERT_RETURN(false, 0); }
const char *getEnumName(int) { G_ASSERT_RETURN(false, ""); }
void AnimcharBaseComponent::act(float, bool) { G_ASSERT(0); }
void AnimcharBaseComponent::recalcWtm() { G_ASSERT(0); }
void AnimcharBaseComponent::getTm(mat44f &) const { G_ASSERT(0); }
void AnimcharBaseComponent::getTm(TMatrix &) const { G_ASSERT(0); }
void AnimcharBaseComponent::setTm(const TMatrix &, bool) { G_ASSERT(0); }
void AnimcharBaseComponent::setTm(const Point3 &, const Point3 &, const Point3 &) { G_ASSERT(0); }
void AnimcharBaseComponent::setPostController(IAnimCharPostController *) { G_ASSERT(0); }
void AnimcharBaseComponent::calcAnimWtm(bool) { G_ASSERT(0); }
bool AnimcharBaseComponent::initAttachmentTmAndNodeWtm(int, mat44f &) const { G_ASSERT_RETURN(false, false); }
const mat44f *AnimcharBaseComponent::getSlotNodeWtm(int) const { G_ASSERT_RETURN(false, nullptr); }
const mat44f *AnimcharBaseComponent::getAttachmentTm(int) const { G_ASSERT_RETURN(false, nullptr); }
int AnimcharBaseComponent::setAttachedChar(int, attachment_uid_t, AnimcharBaseComponent *, bool) { G_ASSERT_RETURN(false, 0); }
int AnimcharBaseComponent::getAttachmentSlotsCount() const { G_ASSERT_RETURN(false, 0); }
int AnimcharBaseComponent::getAttachmentSlotId(const int) const { G_ASSERT_RETURN(false, 0); }
AnimcharBaseComponent *AnimcharBaseComponent::getAttachedChar(const int) const { G_ASSERT_RETURN(false, nullptr); }
void AnimcharBaseComponent::releaseAttachment(int) { G_ASSERT(0); }
void AnimcharBaseComponent::reset() { G_ASSERT(0); }
void AnimcharBaseComponent::resetFastPhysWtmOfs(const vec3f) { G_ASSERT(0); }
void AnimcharBaseComponent::setFastPhysSystemGravityDirection(const Point3 &) { G_ASSERT(0); }
void AnimcharBaseComponent::updateFastPhys(const float) { G_ASSERT(0); }
bool AnimV20::AnimcharRendComponent::calcWorldBox(bbox3f &, const AnimcharFinalMat44 &, bool) const { G_ASSERT_RETURN(false, false); }
vec4f AnimV20::AnimcharRendComponent::prepareSphere(const AnimcharFinalMat44 &) const { G_ASSERT_RETURN(false, vec4f()); }
const DataBlock *AnimcharBaseComponent::getDebugBlenderState(bool) { G_ASSERT_RETURN(false, nullptr); }
AnimV20::AnimBlender::TlsContext &AnimV20::AnimBlender::selectCtx(intptr_t (*)(int, intptr_t, intptr_t, intptr_t, void *), void *)
{
  G_ASSERT(0);
  static TlsContext dummy;
  return dummy;
}
void AnimcharBaseComponent::forcePostRecalcWtm(real) { G_ASSERT(0); }
void AnimcharBaseComponent::cloneTo(AnimcharBaseComponent *, bool) const { G_ASSERT(0); }

void AnimationGraph::enqueueState(IPureAnimStateHolder &, dag::ConstSpan<StateRec>, float, float) { G_ASSERT(0); }
void AnimationGraph::setStateSpeed(IPureAnimStateHolder &, dag::ConstSpan<StateRec>, float) { G_ASSERT(0); }
int AnimationGraph::getParamId(const char *, int) const { G_ASSERT_RETURN(false, 0); }

bool AnimBlendCtrl_Fifo3::isEnqueued(IPureAnimStateHolder &, IAnimBlendNode *) { G_ASSERT_RETURN(false, false); }
void AnimBlendCtrl_Fifo3::enqueueState(IPureAnimStateHolder &, IAnimBlendNode *, real, real) { G_ASSERT(0); }
int AnimBlendCtrl_ParametricSwitcher::getAnimForRange(real) { G_ASSERT_RETURN(false, 0); }

int AnimCommonStateHolder::getParamInt(int) const { G_ASSERT_RETURN(false, 0); }
void AnimCommonStateHolder::setParamInt(int, int) { G_ASSERT(0); }
int AnimCommonStateHolder::getParamFlags(int, int) const { G_ASSERT_RETURN(false, 0); }
void AnimCommonStateHolder::setParamFlags(int, int, int) { G_ASSERT(0); }
float AnimCommonStateHolder::getParamEffTimeScale(int) const { G_ASSERT_RETURN(false, 0.f); }
int AnimCommonStateHolder::getTimeScaleParamId(int) const { G_ASSERT_RETURN(false, 0); }
void AnimCommonStateHolder::setTimeScaleParamId(int, int) { G_ASSERT(0); }
void AnimCommonStateHolder::advance(float) { G_ASSERT(0); }
void AnimCommonStateHolder::term() { G_ASSERT(0); }

void AnimBlender::buildNodeList() { G_ASSERT(0); }
} // namespace AnimV20

void Animate2ndPass::release() { G_ASSERT(0); }

namespace AnimCharV20
{
int getSlotId(const char *) { G_ASSERT_RETURN(false, 0); }
int addSlotId(const char *) { G_ASSERT_RETURN(false, 0); }
const char *getSlotName(const int) { G_ASSERT_RETURN(false, nullptr); }
} // namespace AnimCharV20
