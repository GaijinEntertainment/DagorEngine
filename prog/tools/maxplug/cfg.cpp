// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include <max.h>
#include <vector>
#include <string>
#include "cfg.h"
#include "common.h"

CfgReader::CfgReader(const std::wstring &name) : filename(name) {}

StringVector *CfgReader::GetSectionNames()
{
  section_names.clear();

  MaxSDK::Array<MCHAR> buf;
  const DWORD size = MaxSDK::Util::GetPrivateProfileSectionNames(buf, filename.c_str());
  const MCHAR *names = buf.asArrayPtr();

  // one run of null-terminated names
  for (DWORD i = 0; i < size;)
  {
    const size_t len = wcslen(names + i);
    if (len)
      section_names.emplace_back(names + i, len);
    i += DWORD(len) + 1;
  }

  return &section_names;
}

std::wstring CfgReader::GetKeyValue(const std::wstring &key, const std::wstring &sec)
{
  MaxSDK::Array<MCHAR> buf;
  if (!MaxSDK::Util::GetPrivateProfileString(sec.c_str(), key.c_str(), _T(""), buf, filename.c_str()))
    return std::wstring();

  return std::wstring(buf.asArrayPtr());
}


bool CfgReader::WriteKeyVal(const std::wstring &key, const std::wstring &sec, const TCHAR *new_val)
{
  bool ret = MaxSDK::Util::WritePrivateProfileString(sec.c_str(), key.c_str(), new_val, filename.c_str()) ? true : false;
  return ret;
}


CfgShader::CfgShader(const std::wstring &name) : CfgReader(name) {}

StringVector *CfgShader::GetShaderNames()
{

  GetSectionNames();

  shader_names.clear();

  for (int pos = 0; pos < section_names.size(); ++pos)
  {
    if (section_names.at(pos).compare(CFG_GLOBAL_NAME) != 0 && section_names.at(pos).compare(CFG_SETTINGS_NAME) != 0)
      shader_names.push_back(section_names.at(pos));
  }

  return &shader_names;
}
