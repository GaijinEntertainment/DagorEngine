#include "mat_util.h"

#include <obsolete/dag_cfg.h>

#include <stdio.h>


static char valBuff[128];


static void changeScriptParam(String &script, const char *param, const char *val, bool erase)
{
  CfgReader cfg;
  cfg.getdiv_text(String("[x]\r\n") + script, "x");

  script.clear();
  CfgDiv &div = cfg;

  bool paramSet = false;
  const char *curVal = NULL;

  for (int i = 0; i < div.var.size(); ++i)
  {
    bool doIgnore = false;

    CfgVar &v = div.var[i];

    if (v.id && v.val)
    {
      curVal = v.val;

      if (!stricmp(v.id, param))
      {
        paramSet = true;
        doIgnore = erase;
        curVal = val;
      }

      if (!doIgnore)
        script.aprintf(512, "%s=%s\r\n", v.id, curVal);
    }
  }

  if (!erase && !paramSet)
    script.aprintf(512, "%s=%s\r\n", param, val);
}


void set_script_param(String &script, const char *param, const char *val) { changeScriptParam(script, param, val, false); }


void set_script_param_int(String &script, const char *param, int val)
{
  _snprintf(valBuff, sizeof(valBuff), "%i", val);
  ::set_script_param(script, param, valBuff);
}


void set_script_param_real(String &script, const char *param, real val)
{
  _snprintf(valBuff, sizeof(valBuff), "%g", val);
  ::set_script_param(script, param, valBuff);
}


void erase_script_param(String &script, const char *param) { changeScriptParam(script, param, NULL, true); }


String get_script_param(const String &script, const char *param)
{
  CfgReader cfg;
  cfg.getdiv_text(String("[x]\r\n") + script, "x");

  CfgDiv &div = cfg;

  for (int i = 0; i < div.var.size(); ++i)
  {
    CfgVar &v = div.var[i];
    if (v.id && v.val && !stricmp(v.id, param))
      return String(v.val);
  }

  return String();
}
