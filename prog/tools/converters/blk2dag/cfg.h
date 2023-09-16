#ifndef __CFGREADER_H
#define __CFGREADER_H
#pragma once

#include <string>
#include <vector>

#define MAX_CFG_DATA      2048
#define CFG_GLOBAL_NAME   "_global_params"
#define CFG_SETTINGS_NAME "_settings"

#define CFG_PARAM_TEXT  "text "
#define CFG_PARAM_REAL  "real "
#define CFG_PARAM_RANGE "real("
#define CFG_PARAM_ENUM  "enum("
#define CFG_PARAM_COLOR "color "

#define CFG_PARAM_GLOBAL   "global"
#define CFG_PARAM_OPTIONAL "optional"
#define CFG_PARAM_COMMON   "common"

#define CFG_PARAM_GLOBAL_   0
#define CFG_PARAM_OPTIONAL_ 1
#define CFG_PARAM_COMMON_   2

#define CFG_GLOBAL 0
#define CFG_SHADER 1

#define CFG_COMMON   0
#define CFG_OPTIONAL 1

#define CFG_TEXT  0
#define CFG_REAL  1
#define CFG_RANGE 2
#define CFG_ENUM  3
#define CFG_COLOR 4

typedef std::vector<std::string> StringVector;

class CfgReader
{
public:
  // lists to keep sections and section data
  StringVector section_names;
  StringVector section_datas;
  StringVector section_keys;

  // cfg file name
  std::string filename;

  CfgReader();

  CfgReader(std::string name);

  ~CfgReader();

  // methods to return the lists of section data and section names
  StringVector *GetSectionNames();
  StringVector *GetSectionDatas(std::string sec);

  // get all section keys
  StringVector *GetSectionKeys(std::string sec);

  // check if the section exists in the file
  bool FindSection(std::string sec);

  // give the key value for the specified key of a section
  std::string GetKeyValue(std::string key, std::string sec);

  // set the key value for the specified key of a section
  bool WriteKeyVal(std::string key, std::string sec, const char *new_val);

  // replace all substrings in string
  static std::string Replace(std::string str, std::string dst, std::string src);
  // remove all sequence in string
  static std::string Remove(std::string str, std::string seq);
};

class CfgShader : public CfgReader
{
public:
  StringVector shader_names;

  StringVector global_params;
  StringVector shader_params;
  StringVector settings_params;

  CfgShader(std::string name);
  virtual ~CfgShader();

  // get config file name from plugin folder.
  // static void GetCfgFilename(const char* cfg, TCHAR* filename);

  StringVector *GetShaderNames();

  StringVector *GetGlobalParams();
  StringVector *GetSettingsParams();
  StringVector *GetShaderParams(std::string shader);

  std::string GetParamOwner(std::string param);
  std::string GetParamValue(std::string owner, std::string param);
  int GetParamType(std::string owner, std::string param);
  int GetParamStatus(std::string owner, std::string param);
  int GetParamMode(std::string owner, std::string param);
  StringVector GetParamData(std::string owner, std::string param);
};

#endif // __CFGREADER_H
