// Copyright (C) Gaijin Games KFT.  All rights reserved.
#pragma once

#include <string>
#include <vector>

#define CFG_GLOBAL_NAME   _T("_global_params")
#define CFG_SETTINGS_NAME _T("_settings")

typedef std::vector<std::wstring> StringVector;

class CfgReader
{
public:
  // lists to keep sections and section names
  StringVector section_names;

  // cfg file name
  std::wstring filename;

  CfgReader() = default;

  CfgReader(const std::wstring &name);

  virtual ~CfgReader() = default;

  // methods to return the lists of section data and section names
  StringVector *GetSectionNames();

  // give the key value for the specified key of a section
  std::wstring GetKeyValue(const std::wstring &key, const std::wstring &sec);

  // set the key value for the specified key of a section
  bool WriteKeyVal(const std::wstring &key, const std::wstring &sec, const TCHAR *new_val);
};

class CfgShader : public CfgReader
{
public:
  StringVector shader_names;

  CfgShader(const std::wstring &name);
  ~CfgShader() override = default;

  StringVector *GetShaderNames();
};
