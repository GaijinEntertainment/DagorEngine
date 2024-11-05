// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include <contentUpdater/version.h>
#include <contentUpdater/fsUtils.h>

#include <debug/dag_assert.h>
#include <util/dag_baseDef.h>

#include <stdio.h>


extern int get_updater_minor_version_digits_count();

const uint32_t updater::Version::MINOR_VERSION_MASK = (1u << (uint32_t(get_updater_minor_version_digits_count()) * 8u)) - 1u;
const uint32_t updater::Version::MAJOR_VERSION_MASK = ~updater::Version::MINOR_VERSION_MASK;


updater::Version::String::String(uint32_t version)
{
  eastl::fill(buffer.begin(), buffer.end(), 0);

  SNPRINTF(buffer.data(), buffer.size(), "%d.%d.%d.%d", (version >> 24) & 0xFF, (version >> 16) & 0xFF, (version >> 8) & 0xFF,
    version & 0xFF);
}


updater::Version::Version(const char *str) : Version()
{
  Array data;
  if (str && *str && ::sscanf(str, "%u.%u.%u.%u", &data[0], &data[1], &data[2], &data[3]) == data.size())
    value = (data[0] << 24) | (data[1] << 16) | (data[2] << 8) | data[3];
}

updater::Version::Version(const Array &arr) : value((arr[0] << 24) | (arr[1] << 16) | (arr[2] << 8) | arr[3]) {}


namespace updater
{
void write_remote_version_file(const char *folder, Version remote_version)
{
  const eastl::string doneFile{eastl::string::CtorSprintf{}, "%s/remote.version", folder};
  updater::fs::write_file_with_content(doneFile.c_str(), remote_version.to_string());
}
} // namespace updater

updater::Version updater::read_remote_version_file(const char *folder)
{
  const eastl::string doneFile{eastl::string::CtorSprintf{}, "%s/remote.version", folder};
  return updater::Version{updater::fs::read_file_content(doneFile.c_str()).c_str()};
}
