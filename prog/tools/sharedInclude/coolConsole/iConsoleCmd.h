#ifndef __GAIJIN_COOL_CONSOLE_ICONSOLE_CMD__
#define __GAIJIN_COOL_CONSOLE_ICONSOLE_CMD__
#pragma once


#include <generic/dag_span.h>
#include <math/dag_Point3.h>

#include <stdlib.h>


class IConsoleCmd
{
public:
  virtual bool onConsoleCommand(const char *cmd, dag::ConstSpan<const char *> params) = 0;
  virtual const char *onConsoleCommandHelp(const char *cmd) = 0;

protected:
  // some useful functions
  float strToFloat(const char *str) const { return atof(str); }

  Point3 strToPoint3(dag::ConstSpan<const char *> params, int idx) const
  {
    if (idx + 3 <= params.size())
      return Point3(strToFloat(params[idx + 0]), strToFloat(params[idx + 1]), strToFloat(params[idx + 2]));

    return Point3(0, 0, 0);
  }

  bool strToBool(const char *str) const
  {
    return (!stricmp(str, "1") || !stricmp(str, "true") || !stricmp(str, "yes") || !stricmp(str, "on"));
  }
};


#endif //__GAIJIN_COOL_CONSOLE_ICONSOLE_CMD__
