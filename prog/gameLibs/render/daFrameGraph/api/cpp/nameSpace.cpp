// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include <render/daFrameGraph/nameSpace.h>

#include <EASTL/fixed_string.h>

#include <debug/dag_assert.h>
#include <runtime/runtime.h>
#include <common/genericPoint.h>


namespace dafg
{

NameSpace::NameSpace() :
  nameId{Runtime::isInitialized() ? Runtime::get().getInternalRegistry().knownNames.root() : static_cast<NameSpaceNameId>(0)}
{}

NameSpace::NameSpace(NameSpaceNameId nid) : nameId(nid) {}

NameSpace NameSpace::operator/(const char *child_name) const
{
  if (!Runtime::isInitialized())
    return NodeTracker::pre_register_name_space(nameId, child_name);
  return {Runtime::get().getInternalRegistry().knownNames.addNameId<NameSpaceNameId>(nameId, child_name)};
}

template <class T>
void NameSpace::setResolution(const char *type_name, T value)
{
  auto &registry = Runtime::get().getInternalRegistry();

  const auto id = registry.knownNames.addNameId<AutoResTypeNameId>(nameId, type_name);

  AutoResTypeData &resData = registry.autoResTypes.get(id);

  if (!eastl::holds_alternative<eastl::monostate>(resData.values) && !eastl::holds_alternative<ResolutionValues<T>>(resData.values))
  {
    // Very unlikely to actually happen, so this message is enough
    logerr("daFG: attempted to set auto-resolution '%s' to a value of a different type! Ignoring this set!", type_name);
    return;
  }

  // No-op, skip
  if (auto values = eastl::get_if<ResolutionValues<T>>(&resData.values);
      values && values->staticResolution == value && values->dynamicResolution == value)
    return;

  Runtime::get().onStaticResolutionChange();
  resData.values = ResolutionValues<T>{value, value, value};
  resData.dynamicResolutionCountdown = 0;
}

template void NameSpace::setResolution<IPoint2>(const char *, IPoint2);
template void NameSpace::setResolution<IPoint3>(const char *, IPoint3);

template <class T>
void NameSpace::setDynamicResolution(const char *type_name, T value)
{
  auto &registry = Runtime::get().getInternalRegistry();

  const auto id = registry.knownNames.addNameId<AutoResTypeNameId>(nameId, type_name);
  if (id == AutoResTypeNameId::Invalid || !registry.autoResTypes.isMapped(id))
  {
    logerr("daFG: Tried to set dynamic resolution for auto-res type %s that wasn't set yet!"
           " Please call NameSpace::setResolution(\"%s\", ...)!",
      type_name, type_name);
    return;
  }

  auto &resData = registry.autoResTypes[id];
  if (!eastl::holds_alternative<ResolutionValues<T>>(resData.values))
  {
    // Very unlikely to actually happen, so this message is enough
    logerr("daFG: attempted to set dynamic auto-resolution '%s' to a value of a different type! Ignoring this set!", type_name);
    return;
  }
  auto &values = eastl::get<ResolutionValues<T>>(resData.values);

  if (DAGOR_UNLIKELY(any_greater(value, values.staticResolution)))
  {
    logerr("daFG: Tried to set dynamic resolution '%s' to %@, which is bigger than the static resolution %@!", type_name, value,
      values.staticResolution);
    return;
  }

  if (DAGOR_UNLIKELY(!all_greater(value, 0)))
  {
    logerr("daFG: Tried to set dynamic resolution '%s' to %@, which is invalid!", type_name, value);
    return;
  }

  // Noop, no need to update the countdown and run more useless code.
  if (values.dynamicResolution == value)
    return;

  values.dynamicResolution = value;
  // We can't immediately change resolution for all textures because history textures will lose
  // their data. So update counter here and decrease it by one when change resolution only for
  // current frame textures.
  resData.dynamicResolutionCountdown = ResourceScheduler::SCHEDULE_FRAME_WINDOW;
}

template void NameSpace::setDynamicResolution<IPoint2>(const char *, IPoint2);
template void NameSpace::setDynamicResolution<IPoint3>(const char *, IPoint3);

void NameSpace::fillSlot(NamedSlot slot, NameSpace res_name_space, const char *res_name)
{
  auto &registry = Runtime::get().getInternalRegistry();
  const ResNameId slotNameId = registry.knownNames.addNameId<ResNameId>(nameId, slot.name);
  const ResNameId resNameId = registry.knownNames.addNameId<ResNameId>(res_name_space.nameId, res_name);

  auto &slots = registry.resourceSlots;
  ResNameId prevResNameId = ResNameId::Invalid;
  if (slots.isMapped(slotNameId) && slots[slotNameId].has_value())
    prevResNameId = slots[slotNameId].value().contents;
  slots.set(slotNameId, SlotData{resNameId, prevResNameId});

  Runtime::get().markStageDirty(CompilationStage::REQUIRES_NAME_RESOLUTION);
}

void NameSpace::updateExternallyConsumedResourceSet(eastl::span<const char *const> res_names)
{
  auto &registry = Runtime::get().getInternalRegistry();

  registry.sinkExternalResources.clear();
  for (auto name : res_names)
    registry.sinkExternalResources.insert(registry.knownNames.addNameId<ResNameId>(nameId, name));

  Runtime::get().markStageDirty(CompilationStage::REQUIRES_IR_GRAPH_BUILD);
}

void NameSpace::markResourceExternallyConsumed(const char *res_name)
{
  auto &registry = Runtime::get().getInternalRegistry();

  registry.sinkExternalResources.insert(registry.knownNames.addNameId<ResNameId>(nameId, res_name));

  Runtime::get().markStageDirty(CompilationStage::REQUIRES_IR_GRAPH_BUILD);
}

void NameSpace::unmarkResourceExternallyConsumed(const char *res_name)
{
  auto &registry = Runtime::get().getInternalRegistry();

  registry.sinkExternalResources.erase(registry.knownNames.addNameId<ResNameId>(nameId, res_name));

  Runtime::get().markStageDirty(CompilationStage::REQUIRES_IR_GRAPH_BUILD);
}


} // namespace dafg
