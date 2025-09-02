// Copyright (C) Gaijin Games KFT.  All rights reserved.
#pragma once

#include <dag/dag_vector.h>
#include <util/dag_string.h>

#include <EASTL/string.h>
#include <EASTL/vector.h>

namespace ecs
{
class Template;
}

class ECSOrderedTemplatesGroups
{
public:
  struct TemplatesGroup
  {
    eastl::string name;
    eastl::vector<eastl::string> reqs;
    eastl::vector<eastl::pair<eastl::string, eastl::string>> variants;
  };

  ECSOrderedTemplatesGroups();

  bool testEcsTemplateCondition(const ecs::Template &ecs_template, const eastl::string &condition, const TemplatesGroup &group,
    int depth);

  dag::Vector<String> getEcsTemplates(const char *group_name);
  dag::Vector<String> getEcsTemplatesGroups();

private:
  void addTemplatesGroupRequire(const char *group_name, const char *require);
  void addTemplatesGroupVariant(const char *group_name, const char *variant);
  void addTemplatesGroupTSuffix(const char *group_name, const char *tsuffix);

  bool testEcsTemplateByTemplatesGroup(const ecs::Template &ecs_template, eastl::vector<eastl::string> *out_variants,
    const TemplatesGroup &group, int depth);

  eastl::vector<TemplatesGroup> orderedTemplatesGroups;
};
