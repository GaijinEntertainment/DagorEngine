// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include <CoreFoundation/CoreFoundation.h>
#include <Foundation/Foundation.h>
#include <IOKit/IOKitLib.h>
#include <sys/sysctl.h>
#include <Metal/Metal.h>

#include <drv/3d/dag_driver.h>
#include <drv/3d/dag_info.h>
#include <drv/3d/dag_commands.h>
#include <osApiWrappers/dag_localConv.h>
#include <util/dag_string.h>
#include <EASTL/optional.h>

static bool str_control_entry(const char *key, String &out_str)
{
  size_t size = 0;
  if (sysctlbyname(key, NULL, &size, NULL, 0) == -1)
    return false;

  if (char *lower = (char*)memalloc(size, tmpmem))
  {
    sysctlbyname(key, lower, &size, NULL, 0);
    lower = dd_strlwr(lower);
    out_str.setStr(lower, size - 1);
    memfree(lower, tmpmem);
  }

  return true;
}

bool mac_is_web_gpu_driver()
{
  return false;
}

bool mac_get_model(String &out_str)
{
  return str_control_entry("hw.model", out_str);
}
