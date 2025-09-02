// Copyright (C) Gaijin Games KFT.  All rights reserved.
#pragma once

#include <string>
#include <vector>

#define MAX_CFG_DATA      2048
#define CFG_GLOBAL_NAME   _T("_global_params")
#define CFG_SETTINGS_NAME _T("_settings")

#define CFG_PARAM_TEXT  _T("text")
#define CFG_PARAM_BOOL  _T("bool")
#define CFG_PARAM_INT   _T("int")
#define CFG_PARAM_REAL  _T("real")
#define CFG_PARAM_ENUM  _T("enum")
#define CFG_PARAM_COLOR _T("color")

#define CFG_PARAM_GLOBAL   _T("global")
#define CFG_PARAM_OPTIONAL _T("optional")
#define CFG_PARAM_COMMON   _T("common")

#define CFG_PARAM_GLOBAL_   0
#define CFG_PARAM_OPTIONAL_ 1
#define CFG_PARAM_COMMON_   2

#define CFG_GLOBAL 0
#define CFG_SHADER 1

#define CFG_COMMON   0
#define CFG_OPTIONAL 1

#define CFG_UNKNOWN (-1)
#define CFG_TEXT    0
#define CFG_BOOL    1
#define CFG_INT     2
#define CFG_REAL    3
#define CFG_ENUM    4
#define CFG_COLOR   5

typedef std::vector<M_STD_STRING> StringVector;

class CfgReader
{
public:
  // lists to keep sections and section data
  StringVector section_names;
  StringVector section_datas;
  StringVector section_keys;

  // cfg file name
  M_STD_STRING filename;

  CfgReader();

  CfgReader(M_STD_STRING name);

  ~CfgReader();

  // methods to return the lists of section data and section names
  StringVector *GetSectionNames();
  StringVector *GetSectionDatas(M_STD_STRING sec);

  // get all section keys
  StringVector *GetSectionKeys(M_STD_STRING sec);

  // check if the section exists in the file
  BOOL FindSection(M_STD_STRING sec);

  // give the key value for the specified key of a section
  M_STD_STRING GetKeyValue(M_STD_STRING key, M_STD_STRING sec);

  // set the key value for the specified key of a section
  bool WriteKeyVal(M_STD_STRING key, M_STD_STRING sec, const TCHAR *new_val);

  // replace all substrings in string
  static M_STD_STRING Replace(M_STD_STRING str, M_STD_STRING dst, M_STD_STRING src);
  // remove all sequence in string
  static M_STD_STRING Remove(M_STD_STRING str, M_STD_STRING seq);
};

class CfgShader : public CfgReader
{
public:
  StringVector shader_names;

  StringVector global_params;
  StringVector shader_params;
  StringVector settings_params;

  CfgShader(M_STD_STRING name);
  virtual ~CfgShader();

  // get config file name from plugin folder.
  static void GetCfgFilename(const TCHAR *cfg, TCHAR *filename);

  StringVector *GetShaderNames();

  StringVector *GetGlobalParams();
  StringVector *GetSettingsParams();
  StringVector *GetShaderParams(M_STD_STRING shader);

  M_STD_STRING GetParamOwner(M_STD_STRING param);
  M_STD_STRING GetParamValue(M_STD_STRING owner, M_STD_STRING param);
  int GetParamType(M_STD_STRING owner, M_STD_STRING param);
  int GetParamStatus(M_STD_STRING owner, M_STD_STRING param);
  int GetParamMode(M_STD_STRING owner, M_STD_STRING param);
  StringVector GetParamData(M_STD_STRING owner, M_STD_STRING param);
};
