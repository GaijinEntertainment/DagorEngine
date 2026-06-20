// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include <animChar/dag_animCharacter2.h>
#include <anim/dag_animBlendCtrl.h>
#include <anim/dag_animDecl.h>
#include <animChar/dag_animate2ndPass.h>
namespace AnimV20
{
int addEnumValue(const char *) { G_ASSERT_RETURN(false, 0); }
int getEnumValueByName(const char *) { G_ASSERT_RETURN(false, 0); }
const char *getEnumName(int) { G_ASSERT_RETURN(false, ""); }
AnimV20::AnimGraphStateHolder::AnimGraphStateHolder(const AnimGraphStateHolder &st) :
  graph(st.graph),
  paramNames(st.paramNames),
  paramTypes(st.paramTypes),
  val(st.val),
  valTimeInd(st.valTimeInd),
  valFifo3Ind(st.valFifo3Ind)
{
  G_ASSERT(0);
}
void AnimcharBaseComponent::act(float, bool) { G_ASSERT(0); }
void AnimcharBaseComponent::recalcWtm() { G_ASSERT(0); }
void AnimcharBaseComponent::getTm(mat44f &) const { G_ASSERT(0); }
void AnimcharBaseComponent::getTm(TMatrix &) const { G_ASSERT(0); }
void AnimcharBaseComponent::setTm(const TMatrix &, bool) { G_ASSERT(0); }
void AnimcharBaseComponent::setTm(const Point3 &, const Point3 &, const Point3 &) { G_ASSERT(0); }
void AnimcharBaseComponent::setPostController(IAnimCharPostController *) { G_ASSERT(0); }
void AnimcharBaseComponent::calcAnimWtm(bool) { G_ASSERT(0); }
bool AnimcharBaseComponent::initAttachmentTmAndNodeWtm(int, mat44f &) const { G_ASSERT_RETURN(false, false); }
dag::Index16 AnimcharBaseComponent::getSlotNodeIdx(int) const { G_ASSERT_RETURN(false, dag::Index16()); }
const mat44f *AnimcharBaseComponent::getSlotNodeWtm(int) const { G_ASSERT_RETURN(false, nullptr); }
const mat44f *AnimcharBaseComponent::getAttachmentTm(int) const { G_ASSERT_RETURN(false, nullptr); }
int AnimcharBaseComponent::setAttachedChar(int, attachment_uid_t, AnimcharBaseComponent &, bool) { G_ASSERT_RETURN(false, 0); }
int AnimcharBaseComponent::getAttachmentSlotsCount() const { G_ASSERT_RETURN(false, 0); }
int AnimcharBaseComponent::getAttachmentSlotId(const int) const { G_ASSERT_RETURN(false, 0); }
AnimcharBaseComponent *AnimcharBaseComponent::getAttachedChar(const int) const { G_ASSERT_RETURN(false, nullptr); }
void AnimcharBaseComponent::releaseAttachment(int) { G_ASSERT(0); }
void AnimcharBaseComponent::reset() { G_ASSERT(0); }
void AnimcharBaseComponent::resetFastPhysWtmOfs(const vec3f) { G_ASSERT(0); }
void AnimcharBaseComponent::setFastPhysSystemGravityDirection(const Point3 &) { G_ASSERT(0); }
void AnimcharBaseComponent::updateFastPhys(const float) { G_ASSERT(0); }
AnimV20::AnimcharDebugContext *AnimV20::AnimcharBaseComponent::createOrReturnExistingDebugContext()
{
  G_ASSERT_RETURN(false, nullptr);
}
void AnimV20::AnimcharDebugContext::disablePbcOverride(int) { G_ASSERT(0); }
void AnimV20::AnimcharDebugContext::setPbcOverride(int, float) { G_ASSERT(0); }
bool AnimV20::AnimcharDebugContext::getPbcOverrideEnabled(int) { G_ASSERT_RETURN(false, false); }
float AnimV20::AnimcharDebugContext::getPbcOverrideValue(int) { G_ASSERT_RETURN(false, 0); }
bool AnimV20::AnimcharRendComponent::calcWorldBox(bbox3f &, const AnimcharFinalMat44 &, bool) const { G_ASSERT_RETURN(false, false); }
vec4f AnimV20::AnimcharRendComponent::prepareSphere(const AnimcharFinalMat44 &) const { G_ASSERT_RETURN(false, vec4f()); }
vec4f AnimV20::AnimcharRendComponent::prepareSphereAndCalcBox(bbox3f &, const AnimcharFinalMat44 &) const
{
  G_ASSERT_RETURN(false, vec4f());
}
const DataBlock *AnimcharBaseComponent::getDebugBlenderState(bool) { G_ASSERT_RETURN(false, nullptr); }
AnimV20::AnimBlender::TlsContext &AnimV20::AnimBlender::selectCtx(intptr_t (*)(int, intptr_t, intptr_t, intptr_t, void *), void *)
{
  G_ASSERT(0);
  static TlsContext dummy;
  return dummy;
}
void AnimcharBaseComponent::forcePostRecalcWtm(real) { G_ASSERT(0); }
void AnimcharBaseComponent::cloneTo(AnimcharBaseComponent *, bool) const { G_ASSERT(0); }

void AnimationGraph::enqueueState(AnimGraphStateHolder &, dag::ConstSpan<StateRec>, float, float) { G_ASSERT(0); }
int AnimationGraph::addParamId(const char *, int) { G_ASSERT_RETURN(0, 0); }
int AnimationGraph::addInlinePtrParamId(const char *, size_t, int) { G_ASSERT_RETURN(0, 0); }
void AnimationGraph::setStateSpeed(AnimGraphStateHolder &, dag::ConstSpan<StateRec>, float) { G_ASSERT(0); }
int AnimationGraph::getParamId(const char *, int) const { G_ASSERT_RETURN(false, 0); }
int AnimationGraph::registerBlendNode(AnimV20::IAnimBlendNode *, char const *, char const *) { G_ASSERT_RETURN(0, 0); }

void AnimBlendCtrl_Hub::addBlendNode(IAnimBlendNode *, bool, real) { G_ASSERT(0); }

void register_blend_node_creator(blend_node_creator_t) {}
bool create_blend_node_from_creators(AnimationGraph &, const DataBlock &) { G_ASSERT_RETURN(0, false); }

bool AnimBlendCtrl_Fifo3::isEnqueued(AnimGraphStateHolder &, IAnimBlendNode *) { G_ASSERT_RETURN(false, false); }
void AnimBlendCtrl_Fifo3::enqueueState(AnimGraphStateHolder &, IAnimBlendNode *, real, FifoMorphType) { G_ASSERT(0); }
int AnimBlendCtrl_ParametricSwitcher::getAnimForRange(real) { G_ASSERT_RETURN(false, 0); }

int AnimGraphStateHolder::getParamInt(int) const { G_ASSERT_RETURN(false, 0); }
void AnimGraphStateHolder::setParamInt(int, int) { G_ASSERT(0); }
int AnimGraphStateHolder::getParamFlags(int, int) const { G_ASSERT_RETURN(false, 0); }
void AnimGraphStateHolder::setParamFlags(int, int, int) { G_ASSERT(0); }
float AnimGraphStateHolder::getParamEffTimeScale(int) const { G_ASSERT_RETURN(false, 0.f); }
int AnimGraphStateHolder::getTimeScaleParamId(int) const { G_ASSERT_RETURN(false, 0); }
void AnimGraphStateHolder::setTimeScaleParamId(int, int) { G_ASSERT(0); }
void AnimGraphStateHolder::advance(float) { G_ASSERT(0); }
void AnimGraphStateHolder::term() { G_ASSERT(0); }
void AnimGraphStateHolder::init() { G_ASSERT(0); }

void AnimBlender::buildNodeList() { G_ASSERT(0); }
void AnimGraphStateHolder::dumpStateText(String &) const { G_ASSERT(0); }

bool IAnimBlendNode::isAnimNodeNameValid(const char *) { G_ASSERT_RETURN(0, false); }
AnimPostBlendCtrl::AnimPostBlendCtrl(AnimV20::AnimationGraph &g) : graph(g), pbcId(-1) { G_ASSERT(0); }
AnimPostBlendCtrl::~AnimPostBlendCtrl() { G_ASSERT(0); }
void AnimPostBlendCtrl::buildBlendingList(BlendCtx &, real) { G_ASSERT(0); }

} // namespace AnimV20

void Animate2ndPass::release() { G_ASSERT(0); }

namespace AnimResManagerV20
{
void add_bnl(AnimationGraph &, const DataBlock &, AnimData *, bool, bool, const char *) { G_ASSERT(0); }

void add_bn(AnimationGraph &, const DataBlock &, const char *) { G_ASSERT(0); }

} // namespace AnimResManagerV20

namespace AnimCharV20
{
int getSlotId(const char *) { G_ASSERT_RETURN(false, 0); }
int addSlotId(const char *) { G_ASSERT_RETURN(false, 0); }
const char *getSlotName(const int) { G_ASSERT_RETURN(false, nullptr); }
} // namespace AnimCharV20
