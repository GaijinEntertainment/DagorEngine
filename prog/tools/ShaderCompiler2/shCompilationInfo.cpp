// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include <shaders/dag_shaders.h>
#include <shaders/shUtils.h>
#include <osApiWrappers/dag_direct.h>
#include "shLog.h"
#include "shCompilationInfo.h"

void ShHardwareOptions::appendOptsTo(String &fname) const
{
  auto str = d3d::as_ps_string(fshVersion);
  G_ASSERT(str[0] != '\0');
  fname.aprintf(8, ".%s", str);
}


void ShHardwareOptions::dumpInfo() const { sh_debug(SHLOG_INFO, "Shader hardware options: ps=%s", ShUtils::fsh_version(fshVersion)); }


void ShCompilationInfo::buildDebugInfoDirListing(const char *dir, std::initializer_list<const char *> required_extensions)
{
  currentDebugInfoDirListing.clear();
  completeDebugInfoPackListing.clear();
  NameMap stemMap;
  eastl::string pattern = eastl::string{dir} + "*";
  // No comparator, all sets contain literal elements of required_extensions => pointer comparison is fine
  dag::Vector<dag::VectorSet<const char *>> foundExtensionsPerName{};
  for (const alefind_t &ff : dd_find_iterator(pattern.c_str(), DA_FILE))
  {
    currentDebugInfoDirListing.addNameId(ff.name);
    eastl::string stem;
    eastl::string ext;
    if (const char *p = strrchr(ff.name, '.'))
    {
      stem = eastl::string{ff.name, static_cast<eastl::string::size_type>(p - ff.name)};
      ext = eastl::string{p};
    }
    else
    {
      stem = ff.name;
      ext = "";
    }
    int nid = stemMap.addNameId(stem.c_str());
    if (foundExtensionsPerName.size() <= nid)
      foundExtensionsPerName.resize(nid + 1);
    for (const char *reqext : required_extensions)
      if (streq(reqext, ext.c_str()))
      {
        foundExtensionsPerName[nid].emplace(reqext);
        break;
      }
  }
  iterate_names_in_id_order(stemMap, [&](int nid, const char *name) {
    if (foundExtensionsPerName[nid].size() == required_extensions.size())
      completeDebugInfoPackListing.addNameId(name);
  });
}
