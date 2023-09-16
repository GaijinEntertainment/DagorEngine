#include <contentUpdater/version.h>

#include <debug/dag_assert.h>
#include <util/dag_baseDef.h>

#include <stdio.h>


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