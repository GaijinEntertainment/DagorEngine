#pragma once


#include <humanInput/dag_hiComposite.h>
#include <util/dag_string.h>
#include <ctype.h>

static inline bool localize_composite_joy_elem(const char *name, String &out_str, HumanInput::ICompositeLocalization *loc,
  HumanInput::ICompositeLocalization::LocalizationType loc_type)
{
  if (!loc)
    return false;

  const char *suffix = "";
  if (loc_type == loc->LOC_TYPE_BUTTON && strncmp(name, "Button", 6) == 0 && (name[6] == ' ' || isdigit(name[6])))
    suffix = name + 6;
  else if (loc_type == loc->LOC_TYPE_AXIS && strncmp(name, "Axis", 4) == 0 && (name[4] == ' ' || isdigit(name[4])))
    suffix = name + 4;
  else if (loc_type == loc->LOC_TYPE_POV && strncmp(name, "POV", 3) == 0 && (name[3] == ' ' || isdigit(name[3])))
    suffix = name + 3;
  else
    loc_type = loc->LOC_TYPE_XINP;
  out_str.printf(0, "%s%s", loc->getLocalizedName(loc_type, name), suffix);
  return true;
}

static inline bool localize_composite_joy_dev(const char *name, String &out_str, HumanInput::ICompositeLocalization *loc)
{
  const char *lname = loc ? loc->getLocalizedName(loc->LOC_TYPE_DEVICE, name) : NULL;
  if (lname && *lname)
  {
    out_str = lname;
    return true;
  }

#if _TARGET_PC_WIN
  wchar_t tmp_w16[128];
  MultiByteToWideChar(CP_ACP, 0, name, -1, tmp_w16, countof(tmp_w16));
  out_str.resize(wcslen(tmp_w16) * 3 + 2);
  WideCharToMultiByte(CP_UTF8, 0, tmp_w16, -1, out_str, out_str.size() - 1, NULL, NULL);
  out_str.updateSz();
  return true;
#else
  return false;
#endif
}
