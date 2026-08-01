// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include <max.h>
#include <vector>
#include <string>
#include "cfg.h"
#include "common.h"

CfgReader::CfgReader() {}

CfgReader::CfgReader(const std::wstring &name) : filename(name) {}

CfgReader::~CfgReader() {}

StringVector *CfgReader::GetSectionNames()
{
  static const int MAX_BUF_FOR_SEC_NAMES = 8192;
  static TCHAR buff[MAX_BUF_FOR_SEC_NAMES];
  section_names.clear();

  int size = MaxSDK::Util::GetPrivateProfileSectionNames(buff, MAX_BUF_FOR_SEC_NAMES, filename.c_str());

  std::wstring section;
  for (int i = 0; i < size; i++)
  {
    if (buff[i] != '\0')
    {
      section += buff[i];
    }
    else
    {
      if (!section.empty())
      {
        section_names.push_back(section);
      }
      section.erase();
    }
  }

  return &section_names;
}

std::wstring CfgReader::GetKeyValue(const std::wstring &key, const std::wstring &sec)
{
  static TCHAR buff[MAX_CFG_DATA];
  // get the info from the .ini file
  MaxSDK::Util::GetPrivateProfileString(sec.c_str(), key.c_str(), _T(""), buff, 255, filename.c_str());

  return std::wstring(buff);
}


bool CfgReader::WriteKeyVal(const std::wstring &key, const std::wstring &sec, const TCHAR *new_val)
{
  bool ret = MaxSDK::Util::WritePrivateProfileString(sec.c_str(), key.c_str(), new_val, filename.c_str()) ? true : false;
  return ret;
}


CfgShader::CfgShader(const std::wstring &name) : CfgReader(name) {}

CfgShader::~CfgShader() {}

void CfgShader::GetCfgFilename(const TCHAR *cfg, TCHAR *filename) { _tcscpy_s(filename, MAX_PATH, get_cfg_filename(cfg).c_str()); }

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
