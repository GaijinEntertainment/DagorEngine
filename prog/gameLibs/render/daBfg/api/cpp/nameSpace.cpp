#include <render/daBfg/nameSpace.h>

#include <EASTL/fixed_string.h>

#include <debug/dag_assert.h>
#include <runtime/backend.h>


namespace dabfg
{

NameSpace::NameSpace() : nameId{NodeTracker::get().registry.knownNames.root()} {}

NameSpace::NameSpace(NameSpaceNameId nid) : nameId(nid) {}

NameSpace NameSpace::operator/(const char *child_name) const
{
  return {NodeTracker::get().registry.knownNames.addNameId<NameSpaceNameId>(nameId, child_name)};
}

void NameSpace::setResolution(const char *type_name, IPoint2 value)
{
  auto &registry = NodeTracker::get().registry;

  const auto id = registry.knownNames.addNameId<AutoResTypeNameId>(nameId, type_name);
  const AutoResType newResolution = AutoResType{value, value};
  if (!registry.autoResTypes.isMapped(id) || registry.autoResTypes[id].staticResolution != newResolution.staticResolution ||
      registry.autoResTypes[id].dynamicResolution != newResolution.dynamicResolution)
  {
    Backend::get().markStageDirty(CompilationStage::REQUIRES_RESOURCE_SCHEDULING);
    registry.autoResTypes.set(id, newResolution);
  }
}

void NameSpace::setDynamicResolution(const char *type_name, IPoint2 value)
{
  auto &registry = NodeTracker::get().registry;

  const auto id = registry.knownNames.addNameId<AutoResTypeNameId>(nameId, type_name);
  if (id == AutoResTypeNameId::Invalid || !registry.autoResTypes.isMapped(id))
  {
    logerr("Tried to set dynamic resolution for daBfg auto-res type %s that wasn't set yet!"
           " Please call NameSpace::setResolution(\"%s\", ...)!",
      type_name, type_name);
    return;
  }
  registry.autoResTypes[id].dynamicResolution = value;
  // We can't immediately change resolution for all textures because history textures will lose
  // their data. So update counter here and decrease it by one when change resolution only for
  // current frame textures.
  registry.autoResTypes[id].dynamicResolutionCountdown = ResourceScheduler::SCHEDULE_FRAME_WINDOW;
}

void NameSpace::fillSlot(NamedSlot slot, NameSpace res_name_space, const char *res_name)
{
  auto &registry = NodeTracker::get().registry;
  const ResNameId slotNameId = registry.knownNames.addNameId<ResNameId>(nameId, slot.name);
  const ResNameId resNameId = registry.knownNames.addNameId<ResNameId>(res_name_space.nameId, res_name);
  registry.resourceSlots.set(slotNameId, SlotData{resNameId});

  Backend::get().markStageDirty(CompilationStage::REQUIRES_NAME_RESOLUTION);
}

void NameSpace::updateExternallyConsumedResourceSet(eastl::span<const char *const> res_names)
{
  auto &registry = NodeTracker::get().registry;

  registry.sinkExternalResources.clear();
  for (auto name : res_names)
    registry.sinkExternalResources.insert(registry.knownNames.addNameId<ResNameId>(nameId, name));

  Backend::get().markStageDirty(CompilationStage::REQUIRES_IR_GENERATION);
}

void NameSpace::markResourceExternallyConsumed(const char *res_name)
{
  auto &registry = NodeTracker::get().registry;

  registry.sinkExternalResources.insert(registry.knownNames.addNameId<ResNameId>(nameId, res_name));

  Backend::get().markStageDirty(CompilationStage::REQUIRES_IR_GENERATION);
}

void NameSpace::unmarkResourceExternallyConsumed(const char *res_name)
{
  auto &registry = NodeTracker::get().registry;

  registry.sinkExternalResources.erase(registry.knownNames.addNameId<ResNameId>(nameId, res_name));

  Backend::get().markStageDirty(CompilationStage::REQUIRES_IR_GENERATION);
}


} // namespace dabfg
