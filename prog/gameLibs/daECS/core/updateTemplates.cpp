// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include <daECS/core/entityManager.h>
#include <memory/dag_framemem.h>
#include <daECS/core/coreEvents.h>
#include <util/dag_fixedBitArray.h>
#include <util/dag_stlqsort.h>
#include <perfMon/dag_cpuFreq.h>
#include "ecsInternal.h"
#include "tokenize_const_string.h"
#include <util/dag_finally.h>

namespace ecs
{

bool EntityManager::is_son_of(const Template &t, const int p, const TemplateDB &db, int rec_depth)
{
  G_FAST_ASSERT(++rec_depth < 1024);
  G_UNUSED(rec_depth); // max recursion depth (sanity check)
  for (auto &parent : t.getParents())
  {
    if (p == parent)
      return true;
    if (is_son_of(db.getTemplateRefById(parent), p, db, rec_depth))
      return true;
  }
  return false;
}

void EntityManager::updateEntitiesWithTemplate(template_t oldT, template_t newTemp, bool update_templ_values)
{
  // Step4. recreate all entities of that template with new template!
  const uint32_t oldArchetype = templates.getTemplate(oldT).archetype;
  const uint32_t newArchetype = templates.getTemplate(newTemp).archetype;
  eastl::vector<EntityComponentRef> changedComponents; // bit vector of components being changed in new template
  bool changedTemplValues = false;
  if (update_templ_values)
  {
    // if component has changed in new template vs old template
    changedComponents.push_back(EntityComponentRef());

    for (uint32_t oldCid = 1, ac = archetypes.getComponentsCount(oldArchetype); oldCid < ac; ++oldCid)
    {
      changedComponents.push_back(EntityComponentRef());
      G_ASSERT(oldCid + 1 == changedComponents.size());
      const component_index_t cidx = archetypes.getComponent(oldArchetype, oldCid);
      const archetype_component_id newCid = archetypes.getArchetypeComponentId(newArchetype, cidx);
      if (newCid == INVALID_ARCHETYPE_COMPONENT_ID)
        continue;

      const uint32_t oldOfs = archetypes.getComponentInitialOfs(oldArchetype, oldCid);
      const void *oldTemplateData = templates.getTemplateData(oldT, oldOfs, oldCid);
      if (!oldTemplateData)
        continue;
      const uint32_t newOfs = archetypes.getComponentInitialOfs(newArchetype, newCid);
      const void *newTemplateData = templates.getTemplateData(newTemp, newOfs, newCid);
      if (!newTemplateData)
        continue;

      DataComponent dataComponentInfo = dataComponents.getComponentById(cidx);
      ComponentType typeInfo = componentTypes.getTypeInfo(dataComponentInfo.componentType);
      // we assume, that shared components should not be updated on template change/modify.
      // that is not always true, but we can't differentiate change made in shared component by some Entity with a change in template
      //  (and that is universally used pattern for all shared components, as 'deferred common initialization')
      // so we just assume components are same
      if (typeInfo.flags & COMPONENT_TYPE_CREATE_ON_TEMPL_INSTANTIATE)
        continue;

      if (is_pod(typeInfo.flags))
      {
        if (memcmp(newTemplateData, oldTemplateData, typeInfo.size) == 0)
          continue;
      }
      else
      {
        ComponentTypeManager *ctm = getComponentTypes().getTypeManager(dataComponentInfo.componentType);
        if (!ctm || ctm->is_equal(newTemplateData, oldTemplateData))
          continue;
      }
      changedComponents[oldCid] =
        EntityComponentRef((void *)newTemplateData, dataComponentInfo.componentTypeName, dataComponentInfo.componentType, cidx);
      changedTemplValues = true;
    }
  }
  if (oldArchetype != newArchetype || update_templ_values)
  {
    // 4.1 we have to change content of entities of that type!
    // debug("entities %d", archetypes.getArchetype(oldArchetype).manager.getTotalEntities());

    for (auto *manager = &archetypes.getArchetype(oldArchetype).manager; manager->getTotalEntities();
         manager = &archetypes.getArchetype(oldArchetype).manager)
    {
      for (int ci = (int)manager->getChunksCount() - 1; ci >= 0; --ci)
      {
        for (int ei = manager->getChunkUsed(ci) - 1; ei >= 0; --ei)
        {
          EntityId eid = *(EntityId *)manager->getDataUnsafe(0, sizeof(EntityId), ci, ei);
          // now recreate
          ComponentsInitializer init;
          if (changedTemplValues)
          {
            for (uint32_t ai = 1, ac = archetypes.getComponentsCount(oldArchetype); ai < ac; ++ai)
            {
              if (changedComponents[ai].isNull())
                continue;
              if (!isEntityComponentSameAsTemplate(eid, ai - 1))
                continue;
              // not same!
              const component_index_t cIndex = changedComponents[ai].getComponentId();
              init[HashedConstString{NULL, dataComponents.getComponentTpById(cIndex)}] = ChildComponent(changedComponents[ai]);
              init.back().cIndex = cIndex; // to remove useless validateInitializer
            }
            // validateInitializer(newTemp, init);
          }

          createEntityInternal(eid, newTemp, eastl::move(init), ComponentsMap(), create_entity_async_cb_t());
          if (oldArchetype != newArchetype)
          {
            goto end_loop; // break out of loop. we can't be sure anything good happens in ES handlers on recreate. there can be new
                           // entities appearing
          }
        }
      }
      if (oldArchetype == newArchetype)
        break;
    end_loop:;
    }
  }
}

TemplateDB::AddResult EntityManager::updateTemplate(Template &&update_templ, dag::ConstSpan<const char *> *pnames,
  bool update_templ_values)
{
  // add mutex?
  const int existingId = templateDB.getTemplateIdByName(update_templ.getName());
  TemplateDB::AddResult r = addTemplate(eastl::move(update_templ), pnames);
  if (r != TemplateDB::AR_DUP)
    return existingId >= 0 && r == TemplateDB::AR_OK ? TemplateDB::AR_SAME : r;
  update_templ.resolveFlags(templateDB);
  update_templ.name = templateDB.templates[existingId].getName(); //

  // let's start with one template only
  auto updateEntities = [&](size_t i) {
    const template_t oldT = templateDB.instantiatedTemplates[i].t;
    if (oldT == INVALID_TEMPLATE_INDEX) // nothing to do
      return;
    templateDB.instantiatedTemplates[i].t = INVALID_TEMPLATE_INDEX;

    // Step3. instantiate new template
    const template_t newTemp = instantiateTemplate(i);
    if (newTemp == INVALID_TEMPLATE_INDEX)
    {
      logerr("can't instantiate template %s??", templateDB.templates[i].getName());
      templateDB.instantiatedTemplates[i].t = oldT;
      return;
    }
    updateEntitiesWithTemplate(oldT, newTemp, update_templ_values);
  };

  templateDB.templates[existingId] = eastl::move(update_templ); // replace THE template
  updateEntities(existingId);
  // change all templates that extends THAT
  for (size_t i = 0, e = templateDB.size(); i < e; ++i)
  {
    if (is_son_of(templateDB.templates[i], existingId, templateDB))
    {
      // we found some template to update
      updateEntities(i);
    }
  }

  // Step 6. we have to update all values that are same as in old template to new template
  return TemplateDB::AR_DUP;
}

void EntityManager::removeTemplateInternal(const uint32_t *to_remove, const uint32_t cnt)
{
  if (!cnt)
    return;
  DAECS_EXT_ASSERT(cnt == 1 || eastl::is_sorted(to_remove, to_remove + cnt));
  for (auto i = to_remove + cnt - 1, e = to_remove - 1; i != e; --i)
  {
    templateDB.templates.erase(templateDB.templates.begin() + *i);
    templateDB.instantiatedTemplates.erase(templateDB.instantiatedTemplates.begin() + *i);
    templateDB.templatesIds.erase(*i);
  }

  for (auto &p : templates.templateDbId)
  {
    if (p < to_remove[0])
      continue;
    if (p > to_remove[cnt - 1])
    {
      p -= cnt;
      continue;
    }
    auto it = eastl::lower_bound(to_remove, to_remove + cnt, p);
    G_FAST_ASSERT(it != (to_remove + cnt));
    if (p == *it)
      p = ~0u;
    else
      p -= it - to_remove;
  }

  for (size_t i = 0, e = templateDB.templates.size(); i < e; ++i)
  {
    auto &t = templateDB.templates[i];
    for (auto pi = t.parents.end() - 1, pe = t.parents.begin() - 1; pi != pe; --pi)
    {
      auto &p = *pi;
      if (p < to_remove[0])
      {
        continue;
      }
      if (p > to_remove[cnt - 1])
      {
        p -= cnt;
        continue;
      }
      auto it = eastl::lower_bound(to_remove, to_remove + cnt, p);
      G_FAST_ASSERT(it != (to_remove + cnt));
      if (p == *it)
        t.parents.erase(pi);
      else
        p -= it - to_remove;
    }
  }
}

EntityManager::RemoveTemplateResult EntityManager::removeTemplate(const char *name)
{
  const int id = templateDB.getTemplateIdByName(name);
  if (id < 0)
    return RemoveTemplateResult::NotFound;
  SmallTab<uint32_t, framemem_allocator> toRemove;
  for (size_t i = 0, e = templateDB.templates.size(); i < e; ++i)
  {
    if (i == id || is_son_of(templateDB.templates[i], id, templateDB))
    {
      const template_t t = templateDB.instantiatedTemplates[i].t;
      if ((t != INVALID_TEMPLATE_INDEX) && archetypes.getArchetype(templates.getTemplate(t).archetype).manager.getTotalEntities() != 0)
        return RemoveTemplateResult::HasEntities;
      toRemove.push_back(i);
    }
  }
  removeTemplateInternal(toRemove.begin(), toRemove.size());
  return RemoveTemplateResult::Removed;
}

bool EntityManager::updateTemplates(ecs::TemplateRefs &trefs, bool update_templ_values, uint32_t tag,
  eastl::function<void(const char *, EntityManager::UpdateTemplateResult)> cb)
{
  // first - check if we can remove templates
  bool errors = false;
  if (trefs.getEmptyCount() != 0)
  {
    errors = true;
    trefs.reportBrokenParents([&](const char *t, const char *p) {
      G_UNUSED(p);
      logerr("template <%s> has invalid parent <%s>", t, p);
      cb(t, UpdateTemplateResult::InvalidParents);
    });
  }
  if (!trefs.isTopoSorted())
  {
    trefs.reportLoops([&](const char *t, const char *p) {
      G_UNUSED(p);
      logerr("template <%s> has loop at parent <%s>", t, p);
      cb(t, UpdateTemplateResult::InvalidParents);
      errors = true;
    });
  }
  for (auto &t : trefs)
    if (!TemplateDB::is_valid_template_name(t.getName()))
    {
      cb(t.getName(), UpdateTemplateResult::InvalidName);
      errors = true;
    }

  trefs.finalize(tag);
  eastl::vector_set<uint32_t, eastl::less<uint32_t>, framemem_allocator> templatesToRemove; // Note: this also handle dups so vector
                                                                                            // can't be used here
  bool instantiatedTemplatesChanged = false;

  for (size_t ti = 0, te = templateDB.templates.size(); ti != te; ++ti)
  {
    auto &templ = templateDB.templates[ti];
    if (templ.tag != tag) // we don't remove templates of different tag
      continue;
    auto trefIt = trefs.find(templ.getName());
    if (trefIt != trefs.end()) // found in new database, we don't have to remove it
      continue;
    // we have to remove it and it's children
    for (size_t i = 0, e = templateDB.templates.size(); i < e; ++i)
    {
      if (i != ti && !is_son_of(templateDB.templates[i], ti, templateDB))
        continue;
      const template_t t = templateDB.instantiatedTemplates[i].t;
      if ((t == INVALID_TEMPLATE_INDEX) || archetypes.getArchetype(templates.getTemplate(t).archetype).manager.getTotalEntities() == 0)
      {
        instantiatedTemplatesChanged |= (t != INVALID_TEMPLATE_INDEX);
        templatesToRemove.insert(i);
        continue;
      }
      // but we can't
      cb(templ.getName(), UpdateTemplateResult::RemoveHasEntities);
      errors = true;
    }
  }

  // no check if we can update/add templates
  eastl::bitvector<framemem_allocator> noChange(trefs.size());
  uint32_t noChangeCount = 0;
  for (size_t tid = 0, tidE = trefs.size(); tid != tidE; ++tid)
  {
    const Template &trefTemplate = trefs.templates[tid];
    if (!trefTemplate.isValid())
    {
      noChange.set(tid, true);
      noChangeCount++;
      continue;
    }
    if (!TemplateDB::is_valid_template_name(trefTemplate.getName()))
    {
      cb(trefTemplate.getName(), UpdateTemplateResult::InvalidName);
      errors = true;
    }
    const int id = templateDB.getTemplateIdByName(trefTemplate.getName());
    if (id < 0) // new template, can be added
      continue;
    const Template &existing = templateDB.templates[id];
    if (existing.tag != tag) // we can't update template of different tag
    {
      cb(trefTemplate.getName(), UpdateTemplateResult::DifferentTag);
      errors = true;
    }
    if (existing.parents.size() == trefTemplate.parents.size() && existing.isEqual(trefTemplate, getComponentTypes()))
    {
      bool same = true;
      for (size_t pi = 0, pe = existing.parents.size(); pi != pe && same; ++pi)
      {
        const uint32_t trefP = trefTemplate.parents[pi];
        if (trefP >= trefs.size() || !trefs.templates[trefP].isValid())
        {
          cb(trefTemplate.getName(), UpdateTemplateResult::InvalidParents);
          errors = true;
        }
        else if (!noChange.test(trefP, false) ||
                 strcmp(templateDB.getTemplateRefById(existing.parents[pi]).getName(), trefs.templatesIds.getName(trefP)) != 0) //-V575
          same = false;
      }
      if (same)
      {
        cb(trefTemplate.getName(), UpdateTemplateResult::Same);
        noChange.set(tid, true);
        noChangeCount++;
        continue;
      }
    }
    if (!trefTemplate.canInstantiate() && existing.canInstantiate()) // has changed it's ability to instantiate
    {
      const template_t t = templateDB.instantiatedTemplates[id].t;
      if ((t != INVALID_TEMPLATE_INDEX) &&
          archetypes.getArchetype(templates.getTemplate(t).archetype).manager.getTotalEntities() != 0) // we can't change template to
                                                                                                       // non instantiatable
      {
        trefTemplate.reportInvalidDependencies(templateDB);
        cb(trefTemplate.getName(), UpdateTemplateResult::RemoveHasEntities);
        errors = true;
      }
    }

    // template exists, gonna be updated
  }

  if (errors) // we can't update this data base, it is broken
    return false;
  if (templatesToRemove.empty() && noChangeCount == trefs.size()) // nothing to do
    return true;

  FINALLY([this, &instantiatedTemplatesChanged]() {
    if (instantiatedTemplatesChanged) // not sure if we should do that now
      defragArchetypes();
  });

  for (auto &t : templatesToRemove)
    cb(trefs.begin()[t].getName(), UpdateTemplateResult::Removed);

  templateDB.addComponentNames(trefs);
  removeTemplateInternal(templatesToRemove.begin(), templatesToRemove.size()); // first remove all templates
  eastl::vector_set<uint32_t, eastl::less<uint32_t>, framemem_allocator> templatesToUpdate;
  for (size_t tid = 0, tidE = trefs.size(); tid != tidE; ++tid)
  {
    if (noChange.test(tid, true))
      continue;
    Template &updateTempl = trefs.templates[tid];

    for (int pi = 0, pe = updateTempl.parents.size(); pi != pe; ++pi)
    {
      const char *parentName = trefs.templatesIds.getName(updateTempl.parents[pi]);
      const int pid = templateDB.getTemplateIdByName(parentName);
      if (pid < 0)
      {
        logerr("this was not supposed to happen. add template <%s> failed, due to broken ref to %s", updateTempl.getName(),
          parentName);
        cb(updateTempl.getName(), UpdateTemplateResult::InvalidParents);
        updateTempl.parents.erase(updateTempl.parents.begin() + pi);
        pe--;
        pi--;
      }
      else
        updateTempl.parents[pi] = pid;
    }

    const int existingId = templateDB.getTemplateIdByName(updateTempl.getName());
    if (existingId < 0)
    {
      TemplateDB::AddResult r = templateDB.addTemplateWithParents(eastl::move(updateTempl), tag);
      if (r != TemplateDB::AR_OK)
      {
        logerr("this was not supposed to happen. add template <%s> failed, due to %d", updateTempl.getName(), (int)r);
        cb(updateTempl.getName(), UpdateTemplateResult::Unknown);
      }
      else
        cb(updateTempl.getName(), UpdateTemplateResult::Added);
      continue;
    }
    // all these should be one line!
    updateTempl.name = templateDB.templates[existingId].getName(); // update name, ugly
    templateDB.templates[existingId] = eastl::move(updateTempl);   // replace THE template
    //--

    templatesToUpdate.insert((uint32_t)existingId);
  }

  if (templatesToUpdate.empty())
    return true;

  eastl::vector_set<uint32_t, eastl::less<uint32_t>, framemem_allocator> allTemplatesToUpdate = templatesToUpdate;
  for (auto id : templatesToUpdate)
  {
    for (uint32_t i = 0, e = templateDB.templates.size(); i < e; ++i)
      if (is_son_of(templateDB.templates[i], id, templateDB))
        allTemplatesToUpdate.insert(i);
  }
  for (auto i : allTemplatesToUpdate)
  {
    cb(templateDB.templates[i].getName(), UpdateTemplateResult::Updated);
    // let's start with one template only
    const template_t oldT = templateDB.instantiatedTemplates[i].t;
    if (oldT == INVALID_TEMPLATE_INDEX) // nothing to do
      continue;
    templateDB.instantiatedTemplates[i].t = INVALID_TEMPLATE_INDEX;
    instantiatedTemplatesChanged = true;

    // Step3. instantiate new template
    const template_t newTemp = instantiateTemplate(i);
    if (newTemp == INVALID_TEMPLATE_INDEX)
    {
      logerr("can't instantiate template %s??", templateDB.templates[i].getName());
      templateDB.instantiatedTemplates[i].t = oldT;
      cb(templateDB.templates[i].getName(), UpdateTemplateResult::Unknown);
    }
    else
    {
      updateEntitiesWithTemplate(oldT, newTemp, update_templ_values);
    }
  }

  return true;
}

}; // namespace ecs
