#include <windows.h>
#include <vector>
#include <string>
#include "cfg.h"

CfgReader::CfgReader() {}

CfgReader::CfgReader(std::string name) : filename(name) {}

CfgReader::~CfgReader() {}

StringVector *CfgReader::GetSectionKeys(std::string sec)
{

  char buff[MAX_CFG_DATA];
  section_keys.clear();
  int size = ::GetPrivateProfileSection((LPCTSTR)sec.c_str(), buff, MAX_CFG_DATA, (LPCTSTR)filename.c_str());

  std::string data;
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
        size_t pos = data.find("=");
        if (std::string::npos != pos)
        {
          std::string key = data.substr(0, pos);
          section_keys.push_back(key);
        }
      }
      data.erase();
    }
  }

  return &section_keys;
}

StringVector *CfgReader::GetSectionDatas(std::string sec)
{

  char buff[MAX_CFG_DATA];
  section_datas.clear();
  int size = ::GetPrivateProfileSection((LPCTSTR)sec.c_str(), buff, MAX_CFG_DATA, (LPCTSTR)filename.c_str());

  std::string data;
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
  char buff[MAX_CFG_DATA];
  section_names.clear();

  int size = ::GetPrivateProfileSectionNames(buff, MAX_CFG_DATA, (LPCTSTR)filename.c_str());

  std::string section;
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

bool CfgReader::FindSection(std::string sec)
{
  char buff[MAX_CFG_DATA];
  std::string csAux;
  // get the info from the .ini file
  // return if we could retrieve any info from that section
  return (::GetPrivateProfileString((LPCTSTR)sec.c_str(), NULL, "", buff, 90, (LPCTSTR)filename.c_str()) > 0);
}

std::string CfgReader::GetKeyValue(std::string key, std::string sec)
{
  char buff[MAX_CFG_DATA];
  // get the info from the .ini file
  ::GetPrivateProfileString((LPCTSTR)sec.c_str(), (LPCTSTR)key.c_str(), "", buff, 255, (LPCTSTR)filename.c_str());

  return std::string(buff);
}


bool CfgReader::WriteKeyVal(std::string key, std::string sec, const char *new_val)
{
  bool ret = ::WritePrivateProfileString((LPCTSTR)sec.c_str(), (LPCTSTR)key.c_str(), (LPCTSTR)new_val, (LPCTSTR)filename.c_str())
               ? true
               : false;
  return ret;
}


std::string CfgReader::Replace(std::string str, std::string dst, std::string src)
{
  std::string res;

  res = str;

  size_t pos = 0;

  while ((pos = res.find(dst, pos)) != std::string::npos)
  {
    res.replace(pos, dst.length(), src);
    pos += src.length();
  }

  return res;
}

std::string CfgReader::Remove(std::string str, std::string seq)
{
  std::string res;

  res = str;

  size_t pos = 0;

  while ((pos = res.find(seq, pos)) != std::string::npos)
  {
    int len = 1;
    size_t at = pos;
    while ((at = res.find(seq, at + seq.length())) == pos + len * seq.length() && at != std::string::npos)
    {
      ++len;
    }

    res.replace(pos, len * seq.length(), seq);
    pos += seq.length();
  }

  return res;
}

CfgShader::CfgShader(std::string name) : CfgReader(name) {}

CfgShader::~CfgShader() {}

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

StringVector *CfgShader::GetShaderParams(std::string shader)
{
  GetSectionKeys(shader);
  shader_params.clear();
  shader_params = section_keys;

  return &shader_params;
}

std::string CfgShader::GetParamOwner(std::string param)
{
  GetGlobalParams();
  size_t pos = 0;

  for (pos = 0; pos < global_params.size(); ++pos)
  {
    std::string str = global_params.at(pos);
    if (global_params.at(pos).compare(param) == 0)
      return CFG_GLOBAL_NAME;
  }

  GetShaderNames();
  for (pos = 0; pos < shader_names.size(); ++pos)
  {
    std::string str = shader_names.at(pos);
    GetShaderParams(shader_names.at(pos));
    for (int par = 0; par < shader_params.size(); ++par)
    {
      std::string str = shader_params.at(par);
      if (shader_params.at(par).compare(param) == 0)
        return shader_names.at(pos);
    }
  }

  return std::string();
}

std::string CfgShader::GetParamValue(std::string owner, std::string param) { return GetKeyValue(param, owner); }

int CfgShader::GetParamType(std::string owner, std::string param)
{
  std::string value = GetKeyValue(param, owner);
  int type = 0;

  if (value.find(CFG_PARAM_TEXT) != std::string::npos)
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

int CfgShader::GetParamStatus(std::string owner, std::string param)
{
  std::string value = GetKeyValue(param, owner);

  return (owner.compare(CFG_GLOBAL_NAME) != std::string::npos) ? CFG_SHADER : CFG_GLOBAL;
}

int CfgShader::GetParamMode(std::string owner, std::string param)
{
  std::string value = GetKeyValue(param, owner);

  return (value.find(CFG_PARAM_OPTIONAL) != std::string::npos) ? CFG_OPTIONAL : CFG_COMMON;
}

StringVector CfgShader::GetParamData(std::string owner, std::string param)
{
  std::string value = GetKeyValue(param, owner);

  size_t start = value.find("(");
  size_t end = value.find(")");

  if (start == std::string::npos || end == std::string::npos)
    return StringVector();

  value = value.substr(start + 1, end - start - 1);

  StringVector datas;

  start = 0;
  end = 0;

  while (start != std::string::npos && !value.empty())
  {
    start = end > 0 ? end + 1 : end;
    if (start >= value.length())
      break;
    end = value.find(" ", start);
    if (end == std::string::npos)
      end = value.length();
    std::string data = value.substr(start, end - start);
    if (!data.empty())
      datas.push_back(data);
  }

  return datas;
}
