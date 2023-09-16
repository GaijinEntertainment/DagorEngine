#include <max.h>
#include <vector>
#include <string>
#include "cfg.h"

#if MAX_RELEASE >= 21000
#define NSP MaxSDK::Util
#else
#define NSP
#endif

CfgReader::CfgReader() {}

CfgReader::CfgReader(M_STD_STRING name) : filename(name) {}

CfgReader::~CfgReader() {}

StringVector *CfgReader::GetSectionKeys(M_STD_STRING sec)
{

  TCHAR buff[MAX_CFG_DATA];
  section_keys.clear();
  int size = NSP::GetPrivateProfileSection(sec.c_str(), buff, MAX_CFG_DATA, filename.c_str());

  M_STD_STRING data;
  for (int i = 0; i < size; i++)
  {
    if (buff[i] != '\0')
    {
      data += buff[i];
    }
    else
    {
      if (!data.empty())
      {
        size_t pos = data.find(_T("="));
        if (M_STD_STRING::npos != pos)
        {
          M_STD_STRING key = data.substr(0, pos);
          section_keys.push_back(key);
        }
      }
      data.erase();
    }
  }

  return &section_keys;
}

StringVector *CfgReader::GetSectionDatas(M_STD_STRING sec)
{

  TCHAR buff[MAX_CFG_DATA];
  section_datas.clear();
  int size = NSP::GetPrivateProfileSection(sec.c_str(), buff, MAX_CFG_DATA, filename.c_str());

  M_STD_STRING data;
  for (int i = 0; i < size; i++)
  {
    if (buff[i] != '\0')
    {
      data += buff[i];
    }
    else
    {
      if (!data.empty())
      {
        section_datas.push_back(data);
      }
      data.erase();
    }
  }

  return &section_datas;
}

StringVector *CfgReader::GetSectionNames()
{
  static const int MAX_BUF_FOR_SEC_NAMES = 8192;
  TCHAR buff[MAX_BUF_FOR_SEC_NAMES];
  section_names.clear();

  int size = NSP::GetPrivateProfileSectionNames(buff, MAX_BUF_FOR_SEC_NAMES, filename.c_str());

  M_STD_STRING section;
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

BOOL CfgReader::FindSection(M_STD_STRING sec)
{
  TCHAR buff[MAX_CFG_DATA];
  M_STD_STRING csAux;
  // get the info from the .ini file
  // return if we could retrieve any info from that section
  return (NSP::GetPrivateProfileString(sec.c_str(), NULL, _T(""), buff, 90, filename.c_str()) > 0);
}

M_STD_STRING CfgReader::GetKeyValue(M_STD_STRING key, M_STD_STRING sec)
{
  TCHAR buff[MAX_CFG_DATA];
  // get the info from the .ini file
  NSP::GetPrivateProfileString(sec.c_str(), key.c_str(), _T(""), buff, 255, filename.c_str());

  return M_STD_STRING(buff);
}


bool CfgReader::WriteKeyVal(M_STD_STRING key, M_STD_STRING sec, const TCHAR *new_val)
{
  bool ret = NSP::WritePrivateProfileString(sec.c_str(), key.c_str(), new_val, filename.c_str()) ? true : false;
  return ret;
}


M_STD_STRING CfgReader::Replace(M_STD_STRING str, M_STD_STRING dst, M_STD_STRING src)
{
  M_STD_STRING res;

  res = str;

  size_t pos = 0;

  while ((pos = res.find(dst, pos)) != M_STD_STRING::npos)
  {
    res.replace(pos, dst.length(), src);
    pos += src.length();
  }

  return res;
}

M_STD_STRING CfgReader::Remove(M_STD_STRING str, M_STD_STRING seq)
{
  M_STD_STRING res;

  res = str;

  size_t pos = 0;

  while ((pos = res.find(seq, pos)) != M_STD_STRING::npos)
  {
    int len = 1;
    size_t at = pos;
    while ((at = res.find(seq, at + seq.length())) == pos + len * seq.length() && at != M_STD_STRING::npos)
    {
      ++len;
    }

    res.replace(pos, len * seq.length(), seq);
    pos += seq.length();
  }

  return res;
}

CfgShader::CfgShader(M_STD_STRING name) : CfgReader(name) {}

CfgShader::~CfgShader() {}

void CfgShader::GetCfgFilename(const TCHAR *cfg, TCHAR *filename)
{
  ::GetModuleFileName(hInstance, filename, MAX_PATH);
  TCHAR drive[_MAX_DRIVE];
  TCHAR dir[_MAX_DIR];
  TCHAR fName[_MAX_FNAME];
  TCHAR ext[_MAX_EXT];
  _tsplitpath(filename, drive, dir, fName, ext);
  _tcscpy(filename, drive);
  _tcscat(filename, dir);
  _tcscat(filename, _T("../")); // search one dir up first
  _tcscat(filename, cfg);
  if (FILE *fp = _tfopen(filename, _T("rb")))
    fclose(fp);
  else
  {
    // if not found then get in module dir
    _tcscpy(filename, drive);
    _tcscat(filename, dir);
    _tcscat(filename, cfg);
  }
}

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

StringVector *CfgShader::GetGlobalParams()
{
  GetSectionKeys(CFG_GLOBAL_NAME);
  global_params.clear();
  global_params = section_keys;

  return &global_params;
}

StringVector *CfgShader::GetSettingsParams()
{
  GetSectionKeys(CFG_SETTINGS_NAME);
  settings_params.clear();
  settings_params = section_keys;

  return &settings_params;
}

StringVector *CfgShader::GetShaderParams(M_STD_STRING shader)
{
  GetSectionKeys(shader);
  shader_params.clear();
  shader_params = section_keys;

  return &shader_params;
}

M_STD_STRING CfgShader::GetParamOwner(M_STD_STRING param)
{
  GetGlobalParams();
  size_t pos = 0;

  for (pos = 0; pos < global_params.size(); ++pos)
  {
    M_STD_STRING str = global_params.at(pos);
    if (global_params.at(pos).compare(param) == 0)
      return CFG_GLOBAL_NAME;
  }

  GetShaderNames();
  for (pos = 0; pos < shader_names.size(); ++pos)
  {
    M_STD_STRING str = shader_names.at(pos);
    GetShaderParams(shader_names.at(pos));
    for (int par = 0; par < shader_params.size(); ++par)
    {
      M_STD_STRING str = shader_params.at(par);
      if (shader_params.at(par).compare(param) == 0)
        return shader_names.at(pos);
    }
  }

  return M_STD_STRING();
}

M_STD_STRING CfgShader::GetParamValue(M_STD_STRING owner, M_STD_STRING param) { return GetKeyValue(param, owner); }

int CfgShader::GetParamType(M_STD_STRING owner, M_STD_STRING param)
{
  M_STD_STRING value = GetKeyValue(param, owner);
  int type = 0;

  if (value.find(CFG_PARAM_TEXT) != M_STD_STRING::npos)
  {
    type = CFG_TEXT;
  }
  else if (value.find(CFG_PARAM_REAL) == 0)
  {
    type = CFG_REAL;
  }
  else if (value.find(CFG_PARAM_COLOR) == 0)
  {
    type = CFG_COLOR;
  }
  else if (value.find(CFG_PARAM_RANGE) == 0)
  {
    type = CFG_RANGE;
  }
  else if (value.find(CFG_PARAM_ENUM) == 0)
  {
    type = CFG_ENUM;
  }

  return type;
}

int CfgShader::GetParamStatus(M_STD_STRING owner, M_STD_STRING param)
{
  M_STD_STRING value = GetKeyValue(param, owner);

  return (owner.compare(CFG_GLOBAL_NAME) != M_STD_STRING::npos) ? CFG_SHADER : CFG_GLOBAL;
}

int CfgShader::GetParamMode(M_STD_STRING owner, M_STD_STRING param)
{
  M_STD_STRING value = GetKeyValue(param, owner);

  return (value.find(CFG_PARAM_OPTIONAL) != M_STD_STRING::npos) ? CFG_OPTIONAL : CFG_COMMON;
}

StringVector CfgShader::GetParamData(M_STD_STRING owner, M_STD_STRING param)
{
  M_STD_STRING value = GetKeyValue(param, owner);

  size_t start = value.find(_T("("));
  size_t end = value.find(_T(")"));

  if (start == M_STD_STRING::npos || end == M_STD_STRING::npos)
    return StringVector();

  value = value.substr(start + 1, end - start - 1);

  StringVector datas;

  start = 0;
  end = 0;

  while (start != M_STD_STRING::npos && !value.empty())
  {
    start = end > 0 ? end + 1 : end;
    if (start >= value.length())
      break;
    end = value.find(_T(" "), start);
    if (end == M_STD_STRING::npos)
      end = value.length();
    M_STD_STRING data = value.substr(start, end - start);
    if (!data.empty())
      datas.push_back(data);
  }

  return datas;
}
