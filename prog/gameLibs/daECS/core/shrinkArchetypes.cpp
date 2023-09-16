// #include <perfMon/dag_perfTimer.h>
#include <daECS/core/entityManager.h>
#include <memory/dag_framemem.h>

namespace ecs
{

uint32_t EntityManager::defragTemplates()
{
  G_ASSERT_RETURN(!isConstrainedMTMode(), 0);
  G_ASSERT_RETURN(nestedQuery == 0, 0);
  eastl::bitvector<framemem_allocator> templUsed(templates.size(), false);
  for (const auto &ei : entDescs.entDescs)
  {
    if (ei.archetype == INVALID_ARCHETYPE)
      continue;
    templUsed.set(ei.template_id, true);
  }

  for (auto &ci : delayedCreationQueue)
    for (auto &cr : ci)
      if (!cr.isToDestroy())
        templUsed.set(cr.templ, true);
  G_ASSERT(templUsed.size() == templates.size());
  SmallTab<template_t, framemem_allocator> remapTemplates(templates.size());
  uint32_t used = 0;
  for (uint32_t i = 0, e = templates.size(); i != e; ++i)
  {
    if (templUsed.test(i, false))
      remapTemplates[i] = used++;
    else
      remapTemplates[i] = INVALID_TEMPLATE_INDEX;
  }
  const uint32_t unused = templates.size() - used;
  if (!unused)
    return 0;
  // defrag templates
  // remove unused templates
  templates.remap(remapTemplates.begin(), used, true, archetypes, dataComponents, componentTypes);

  // remap all old templates
  for (auto &t : templateDB.instantiatedTemplates)
  {
    if (t.t == INVALID_TEMPLATE_INDEX)
      continue;
    G_ASSERT(t.t < remapTemplates.size());
    t.t = remapTemplates[t.t];
  }
  for (auto &ci : delayedCreationQueue)
    for (auto &cr : ci)
      if (!cr.isToDestroy())
      {
        G_ASSERT(cr.templ < remapTemplates.size());
        cr.templ = remapTemplates[cr.templ];
        G_ASSERT(cr.templ != INVALID_TEMPLATE_INDEX && cr.templ < templates.size());
      }
  for (auto &ei : entDescs.entDescs)
  {
    if (ei.archetype == INVALID_ARCHETYPE)
      continue;
    G_ASSERTF(ei.template_id < remapTemplates.size() && remapTemplates[ei.template_id] != INVALID_TEMPLATE_INDEX &&
                remapTemplates[ei.template_id] < templates.size(),
      "eid=%d template was %d -> %d, total new %d", make_eid(&ei - entDescs.entDescs.begin(), ei.generation), ei.template_id,
      remapTemplates[ei.template_id], templates.size());
    ei.template_id = remapTemplates[ei.template_id];
  }
  // todo: send to net broadcast event with changes
  return unused;
}

uint32_t EntityManager::defragArchetypes()
{
  if (!archetypes.size())
    return 0;
  if (!defragTemplates())
    return 0;
  performTrackChanges(true); // try to track changes first. only needed for scheduled archetype changes. We remap scheduled anyway, but
                             // this could create more archetypes, so just do it
  eastl::bitvector<framemem_allocator> archUsed(archetypes.size(), false);

  for (uint32_t i = 0, e = templates.size(); i != e; ++i)
    archUsed.set(templates.getTemplate(i).archetype, true);

  uint32_t used = 0;
  SmallTab<archetype_t, framemem_allocator> remapArchetypes(archetypes.size());
  for (uint32_t i = 0, e = remapArchetypes.size(); i != e; ++i)
  {
    if (archUsed.test(i, false))
    {
      remapArchetypes[i] = used++;
    }
    else
      remapArchetypes[i] = INVALID_ARCHETYPE;
  }

  const uint32_t unused = archetypes.size() - used;
  if (!unused)
    return 0;
  // remove unused archetypes
  archetypes.remap(remapArchetypes.begin(), used);
  convertArchetypeScheduledChanges();
  if (!archetypeTrackingQueue.empty())
  {
    TrackedChangeArchetype archetypeTrackingQueue2;
    for (auto scheduled : archetypeTrackingQueue)
    {
      G_STATIC_ASSERT(sizeof(archetype_t) + sizeof(component_index_t) <= sizeof(scheduled));
      const archetype_t archetype = (scheduled & eastl::numeric_limits<archetype_t>::max());
      const component_index_t cidx = (scheduled >> (8 * sizeof(archetype_t)));
      if (remapArchetypes[archetype] != INVALID_ARCHETYPE)
        archetypeTrackingQueue2.insert(remapArchetypes[archetype] | (cidx << (8 * sizeof(archetype_t))));
    }
    eastl::swap(archetypeTrackingQueue, archetypeTrackingQueue2);
  }

  // remap archetypes
  for (uint32_t i = 0, e = templates.size(); i != e; ++i)
  {
    const archetype_t arch = remapArchetypes[templates.getTemplate(i).archetype];
    templates.getTemplate(i).archetype = arch;
    G_ASSERT(arch != INVALID_ARCHETYPE);
  }
  for (auto &ei : entDescs.entDescs)
  {
    if (ei.archetype == INVALID_ARCHETYPE)
      continue;
    const archetype_t arch = remapArchetypes[ei.archetype];
    ei.archetype = arch;
    G_ASSERT(arch != INVALID_ARCHETYPE);
  }
  invalidatePersistentQueries();
  updateAllQueriesInternal();
  return unused;
}

} // namespace ecs