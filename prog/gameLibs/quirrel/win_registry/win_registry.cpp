// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include <quirrel/win_registry/win_registry.h>

#include <shlwapi.h>
#include <util/dag_string.h>
#include <EASTL/string.h>

#include <sqModules/sqModules.h>


namespace bindquirrel
{

static String retBuf;

//==================================================================================================
static bool regKeyExists(int root, const char *path)
{
  HKEY key;
  if (::RegOpenKeyEx((HKEY)root, path, 0, KEY_QUERY_VALUE, &key) == ERROR_SUCCESS)
  {
    ::RegCloseKey(key);
    return true;
  }

  return false;
}


//==================================================================================================
bool reg_value_exists(int root, const char *path, const char *name)
{
  bool ret = false;
  HKEY key;

  if (!::RegOpenKeyEx((HKEY)root, path, 0, KEY_QUERY_VALUE, &key))
  {
    ret = !::RegQueryValueEx(key, name, 0, NULL, NULL, NULL);

    ::RegCloseKey(key);
  }
  return ret;
}


//==================================================================================================
const char *get_reg_string(int root, const char *path, const char *name, const char *def)
{
  HKEY key;
  bool doRetBuf = false;

  if (::RegOpenKeyEx((HKEY)root, path, 0, KEY_QUERY_VALUE, &key) == ERROR_SUCCESS)
  {
    DWORD type = 0;
    DWORD size = 0;

    if (!::RegQueryValueEx(key, name, 0, &type, NULL, &size))
    {
      if (type == REG_SZ)
      {
        retBuf.resize(size);

        if (!::RegQueryValueEx(key, name, 0, &type, (BYTE *)retBuf.str(), &size))
          doRetBuf = true;
      }
    }

    ::RegCloseKey(key);
  }

  return (doRetBuf ? retBuf.str() : def);
}


//==================================================================================================
bool set_reg_string(int root, const char *path, const char *name, const char *val)
{
  HKEY key;

  if (::RegCreateKeyEx((HKEY)root, path, 0, NULL, REG_OPTION_NON_VOLATILE, KEY_WRITE, NULL, &key, NULL) == ERROR_SUCCESS)
    return !::RegSetValueEx(key, name, 0, REG_SZ, (const BYTE *)val, ::strlen(val) + 1);

  return false;
}


//==================================================================================================
bool set_reg_int(int root, const char *path, const char *name, int val)
{
  HKEY key;

  G_STATIC_ASSERT(sizeof(val) == sizeof(DWORD)); // we are writing REG_DWORD, so binary sizes should match
  if (::RegCreateKeyEx((HKEY)root, path, 0, NULL, REG_OPTION_NON_VOLATILE, KEY_WRITE, NULL, &key, NULL) == ERROR_SUCCESS)
    return !::RegSetValueEx(key, name, 0, REG_DWORD, (BYTE *)&val, sizeof(val));

  return false;
}


//==================================================================================================
int get_reg_int(int root, const char *path, const char *name, int def)
{

  bool ok = false;
  HKEY key;
  DWORD ret = def;

  if (::RegOpenKeyEx((HKEY)root, path, 0, KEY_QUERY_VALUE, &key) == ERROR_SUCCESS)
  {
    DWORD type = 0, size = 0;
    if (::RegQueryValueEx(key, name, 0, &type, NULL, NULL) == ERROR_SUCCESS && type == REG_DWORD)
    {
      size = sizeof(ret);
      if (::RegQueryValueEx(key, name, 0, &type, (BYTE *)&ret, &size) == ERROR_SUCCESS)
        ok = true;
    }

    ::RegCloseKey(key);
  }

  return (ok ? ret : def);
}


//==================================================================================================
bool set_reg_int64(int root, const char *path, const char *name, __int64 val)
{
  HKEY key;

  if (::RegCreateKeyEx((HKEY)root, path, 0, NULL, REG_OPTION_NON_VOLATILE, KEY_WRITE, NULL, &key, NULL) == ERROR_SUCCESS)
    return !::RegSetValueEx(key, name, 0, REG_QWORD, (BYTE *)&val, sizeof(val));

  return false;
}


//==================================================================================================
__int64 get_reg_int64(int root, const char *path, const char *name, __int64 def)
{

  bool ok = false;
  HKEY key;
  __int64 ret = def;

  if (::RegOpenKeyEx((HKEY)root, path, 0, KEY_QUERY_VALUE, &key) == ERROR_SUCCESS)
  {
    DWORD type = 0;
    DWORD size = 0;

    if (!::RegQueryValueEx(key, name, 0, &type, NULL, &size))
    {
      if (type == REG_QWORD)
      {
        if (!::RegQueryValueEx(key, name, 0, &type, (BYTE *)&ret, &size))
          ok = true;
      }
    }

    ::RegCloseKey(key);
  }

  return (ok ? ret : def);
}


//==================================================================================================
static bool deleteRegKey(int root, const char *path)
{
  bool ret = false;

  HKEY key;
  String keyPath(path);

  if (keyPath[keyPath.length() - 1] == '\\')
    keyPath[keyPath.length() - 1] = 0;

  char *keyName = strrchr(keyPath, '\\');
  if (!keyName)
    return false;

  *(keyName++) = 0;

  if (!keyName)
    return false;

  if (::RegOpenKeyEx((HKEY)root, keyPath, 0, KEY_SET_VALUE, &key) == ERROR_SUCCESS)
  {
    if (::SHDeleteKey(key, keyName) == ERROR_SUCCESS)
      ret = true;

    ::RegCloseKey(key);
  }

  return ret;
}


//==================================================================================================
bool delete_reg_value(int root, const char *path, const char *name)
{
  bool ret = false;
  HKEY key;

  if (::RegOpenKeyEx((HKEY)root, path, 0, KEY_SET_VALUE, &key) == ERROR_SUCCESS)
  {
    if (::RegDeleteValue(key, name) == ERROR_SUCCESS)
      ret = true;

    ::RegCloseKey(key);
  }
  return ret;
}

void register_windows_registry_module(SqModules *module_mgr)
{
  HSQUIRRELVM vm = module_mgr->getVM();
  Sqrat::Table win_registry(vm);

  ///@module win.registry
  win_registry //
    .Func("regKeyExists", regKeyExists)
    .Func("regValueExists", reg_value_exists)
    .Func("getRegString", get_reg_string)
    .Func("getRegInt", get_reg_int)
    .Func("deleteRegKey", deleteRegKey)
    .Func("deleteRegValue", delete_reg_value)
    .Func("setRegString", set_reg_string)
    .Func("setRegInt", set_reg_int)
    .SetValue("HKEY_CLASSES_ROOT", (SQInteger)(HKEY_CLASSES_ROOT))
    .SetValue("HKEY_CURRENT_CONFIG", (SQInteger)(HKEY_CURRENT_CONFIG))
    .SetValue("HKEY_CURRENT_USER", (SQInteger)(HKEY_CURRENT_USER))
    .SetValue("HKEY_LOCAL_MACHINE", (SQInteger)(HKEY_LOCAL_MACHINE))
    .SetValue("HKEY_USERS", (SQInteger)(HKEY_USERS))
    /**/;

  module_mgr->addNativeModule("win.registry", win_registry);
}

} // namespace bindquirrel
