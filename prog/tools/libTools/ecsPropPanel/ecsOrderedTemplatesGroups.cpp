// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include <ecsPropPanel/ecsOrderedTemplatesGroups.h>
#include <daECS/core/entityManager.h>

#include <EASTL/sort.h>

static bool has_nonparent_component(const ecs::Template &ecs_template, const char *comp_name)
{
  return ecs_template.getComponentsMap().find(ECS_HASH_SLOW(comp_name)) != nullptr;
}

static bool has_prefixed_components(const ecs::Template &ecs_template, const char *prefix, int prefix_len)
{
  const auto &db = g_entity_mgr->getTemplateDB();
  const auto &comps = ecs_template.getComponentsMap();
  for (const auto &c : comps)
  {
    const char *compName = db.getComponentName(c.first);
    if (!compName)
      continue;
    const int compNameLen = strlen(compName);
    if (compNameLen >= prefix_len && strncmp(prefix, compName, prefix_len) == 0)
      return true;
  }
  return false;
}

static void get_all_ecs_tags(eastl::vector<eastl::string> &out_tags)
{
  out_tags.clear();

  const auto &db = g_entity_mgr->getTemplateDB();
  const ecs::component_type_t tagHash = ECS_HASH("ecs::Tag").hash;
  HashedKeySet<ecs::component_t> added;

  for (int i = 0; i < db.size(); ++i)
  {
    const ecs::Template *pTemplate = db.getTemplateById(i);
    if (!pTemplate)
      continue;
    const auto &comps = pTemplate->getComponentsMap();
    for (const auto &c : comps)
    {
      auto tp = db.getComponentType(c.first);
      if (tp == tagHash && !added.has_key(c.first))
      {
        added.insert(c.first);
        const char *compName = db.getComponentName(c.first);
        if (compName)
          out_tags.push_back(compName);
      }
    }
  }

  eastl::sort(out_tags.begin(), out_tags.end());
}

ECSOrderedTemplatesGroups::ECSOrderedTemplatesGroups()
{
  addTemplatesGroupRequire("All", "");
  addTemplatesGroupRequire("Singletons", "~singleton");

  eastl::vector<eastl::string> tags;
  get_all_ecs_tags(tags);

  eastl::string groupName;
  for (const auto &tag : tags)
  {
    groupName = "Tag: ";
    groupName += tag;
    addTemplatesGroupRequire(groupName.c_str(), tag.c_str());
  }
}

void ECSOrderedTemplatesGroups::addTemplatesGroupRequire(const char *group_name, const char *require)
{
  auto it = eastl::find_if(orderedTemplatesGroups.begin(), orderedTemplatesGroups.end(),
    [&group_name](const TemplatesGroup &group) { return group.name == group_name; });
  TemplatesGroup &group = (it != orderedTemplatesGroups.end()) ? *it : orderedTemplatesGroups.push_back();
  group.name = group_name;
  group.reqs.push_back(require);
}

void ECSOrderedTemplatesGroups::addTemplatesGroupVariant(const char *group_name, const char *variant)
{
  auto it = eastl::find_if(orderedTemplatesGroups.begin(), orderedTemplatesGroups.end(),
    [&group_name](const TemplatesGroup &group) { return group.name == group_name; });
  TemplatesGroup &group = (it != orderedTemplatesGroups.end()) ? *it : orderedTemplatesGroups.push_back();
  group.name = group_name;
  group.variants.push_back({variant, ""});
}

void ECSOrderedTemplatesGroups::addTemplatesGroupTSuffix(const char *group_name, const char *tsuffix)
{
  auto it = eastl::find_if(orderedTemplatesGroups.begin(), orderedTemplatesGroups.end(),
    [&group_name](const TemplatesGroup &group) { return group.name == group_name; });
  TemplatesGroup &group = (it != orderedTemplatesGroups.end()) ? *it : orderedTemplatesGroups.push_back();
  if (group.variants.empty())
    return;
  group.variants.back().second = tsuffix;
}

bool ECSOrderedTemplatesGroups::testEcsTemplateCondition(const ecs::Template &ecs_template, const eastl::string &condition,
  const TemplatesGroup &group, int depth)
{
  bool invert = false;
  bool result = false;

  const char *cond = condition.c_str();
  if (cond[0] == '!')
  {
    invert = true;
    ++cond;
  }

  if (cond[0] == '~')
  {
    ++cond;
    if (strcmp(cond, "singleton") == 0)
    {
      result = ecs_template.isSingleton();
    }
    else if (strcmp(cond, "hastags") == 0) // only in this template (not parents)
    {
      const auto &db = g_entity_mgr->getTemplateDB();
      const ecs::component_type_t tagHash = ECS_HASH("ecs::Tag").hash;

      const auto &comps = ecs_template.getComponentsMap();
      for (const auto &c : comps)
      {
        if (db.getComponentType(c.first) == tagHash)
        {
          result = true;
          break;
        }
      }
    }
    else if (strcmp(cond, "fitprev") == 0)
    {
      if (depth > 0)
        return false; // avoid cyclic

      result = false;
      for (const auto &checkgroup : orderedTemplatesGroups)
      {
        if (&checkgroup == &group)
          break;
        if (checkgroup.reqs.empty() && checkgroup.variants.empty())
          continue;
        if (testEcsTemplateByTemplatesGroup(ecs_template, nullptr, checkgroup, 1))
        {
          result = true;
          break;
        }
      }
    }
    else if (strcmp(cond, "noncreatable") == 0)
    {
      result = ecs_template.hasComponent(ECS_HASH("nonCreatableObj"), g_entity_mgr->getTemplateDB().data());
    }
  }
  else if (cond[0] == '\0')
  {
    result = true;
  }
  else if (cond[0] == '@') // only in this template (for lone template picks tags)
  {
    result = has_nonparent_component(ecs_template, cond + 1);
  }
  else if (cond[0] == '?') // has component with specific string value
  {
    ++cond;
    eastl::string cond_str;
    while (*cond != '\0' && *cond != '=')
      cond_str += *cond++;
    if (*cond == '=')
    {
      ++cond;
      const ecs::ChildComponent &comp =
        ecs_template.getComponent(ECS_HASH_SLOW(cond_str.c_str()), g_entity_mgr->getTemplateDB().data());
      if (comp.is<ecs::string>())
      {
        const char *comp_str = comp.get<ecs::string>().c_str();
        result = comp_str && strcmp(comp_str, cond) == 0;
      }
    }
  }
  else if (condition.back() == '*')
  {
    const int condLen = strlen(cond) - 1;
    const auto &dbdata = g_entity_mgr->getTemplateDB().data();
    dbdata.iterate_template_parents(ecs_template, [&](const ecs::Template &tmpl) {
      if (!result && has_prefixed_components(tmpl, cond, condLen))
        result = true;
    });
  }
  else
  {
    result = ecs_template.hasComponent(ECS_HASH_SLOW(cond), g_entity_mgr->getTemplateDB().data());
  }

  return invert ? !result : result;
}

bool ECSOrderedTemplatesGroups::testEcsTemplateByTemplatesGroup(const ecs::Template &ecs_template,
  eastl::vector<eastl::string> *out_variants, const TemplatesGroup &group, int depth)
{
  if (out_variants)
    out_variants->clear();

  if (!ecs_template.canInstantiate())
    return false;

  for (auto &condition : group.reqs)
    if (!testEcsTemplateCondition(ecs_template, condition, group, depth))
      return false;

  if (group.variants.empty())
  {
    if (out_variants)
      out_variants->push_back("");
    return true;
  }

  for (auto &variant : group.variants)
  {
    if (testEcsTemplateCondition(ecs_template, variant.first, group, depth))
    {
      if (out_variants)
      {
        if (eastl::find(out_variants->begin(), out_variants->end(), variant.second) == out_variants->end())
          out_variants->push_back(variant.second);
      }
      else
        return true;
    }
  }

  if (out_variants && !out_variants->empty())
    return true;

  return false;
}

dag::Vector<String> ECSOrderedTemplatesGroups::getEcsTemplates(const char *group_name)
{
  const TemplatesGroup *pTemplatesGroup = nullptr;

  for (auto &it : orderedTemplatesGroups)
  {
    if (!group_name || group_name[0] == '\0' || it.name == group_name)
    {
      pTemplatesGroup = &it;
      break;
    }
  }

  const ecs::TemplateDB &db = g_entity_mgr->getTemplateDB();

  dag::Vector<String> templates;
  templates.reserve(db.size());

  bool filterNonCreatable = true;
  if (pTemplatesGroup && !pTemplatesGroup->reqs.empty() && pTemplatesGroup->reqs[0] == "~noncreatable")
    filterNonCreatable = false;

  eastl::vector<eastl::string> variants;
  for (auto &it : db)
  {
    if (strchr(it.getName(), '+')) // avoid combined templates
      continue;
    if (filterNonCreatable && it.hasComponent(ECS_HASH("nonCreatableObj"), db.data()))
      continue;
    if (!pTemplatesGroup)
      templates.push_back(String(it.getName()));
    else
    {
      if (!testEcsTemplateByTemplatesGroup(it, &variants, *pTemplatesGroup, 0))
        continue;
      for (const auto &variant : variants)
        if (variant[0] == '+')
          templates.push_back(String::mk_str_cat(it.getName(), variant.c_str()));
        else
          templates.push_back(String::mk_str_cat(variant.c_str(), it.getName()));
    }
  }

  eastl::sort(templates.begin(), templates.end(), [](const String &a, const String &b) { return strcmp(a, b) < 0; });
  return templates;
}

dag::Vector<String> ECSOrderedTemplatesGroups::getEcsTemplatesGroups()
{
  dag::Vector<String> groups;
  groups.reserve(orderedTemplatesGroups.size());
  for (const TemplatesGroup &templateGroup : orderedTemplatesGroups)
    groups.push_back(String(templateGroup.name.c_str()));

  return groups;
}
