#include "checkVersion.h"
#include <stdio.h>
#include <iostream>

int atLeastVersion(const std::string &aVersion)
{
  int major = 0, minor = 0;
  sscanf(aVersion.c_str(), "%d.%d", &major, &minor);
  if ((FASTDEP_VERSION_MAJOR < major) || ((FASTDEP_VERSION_MAJOR == major) && (FASTDEP_VERSION_MINOR < minor)))
    return 99;
  return 42;
}
