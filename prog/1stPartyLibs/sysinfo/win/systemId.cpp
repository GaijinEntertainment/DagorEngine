#include "../sysinfo.h"

#include <windows.h>

namespace systeminfo
{

eastl::string get_system_id()
{
  const char cryptoKey[] = "SOFTWARE\\Microsoft\\Cryptography";
  const char guidKey[] = "MachineGuid";
#if _TARGET_64BIT
  const int access = KEY_READ;
#else
  const int access = KEY_READ|KEY_WOW64_64KEY;
#endif

  eastl::string result(eastl::string::CtorDoNotInitialize{}, MAX_PATH-1);

  HKEY key = NULL;
  DWORD size = MAX_PATH;

  if (::RegOpenKeyEx(HKEY_LOCAL_MACHINE, cryptoKey, 0, access, &key) == ERROR_SUCCESS &&
      ::RegQueryValueEx(key, guidKey, 0, NULL, (BYTE*)result.data(), &size) == ERROR_SUCCESS)
    result.force_size(size-1); // returned size includes terminating null, string's - does not
  else
    result.clear();

  if (key != NULL)
    ::RegCloseKey(key);

  return result;
}

} // namespace systeminfo
