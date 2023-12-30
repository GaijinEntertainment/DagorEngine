#include <osApiWrappers/dag_dynLib.h>

#if _TARGET_PC_LINUX | _TARGET_APPLE | _TARGET_ANDROID
#include <dlfcn.h>
#else
#include <supp/_platform.h>
#endif
#if _TARGET_PC_WIN | _TARGET_XBOX
#include <malloc.h>
#endif

// Load dynamic library. Return NULL if failed.
void *os_dll_load(const char *filename)
{
#if _TARGET_PC_LINUX
  return ::dlopen(filename, RTLD_LAZY | RTLD_DEEPBIND); // RTLD_DEEPBIND requires GLIBC 2.3.4 (introduced in 2004-12-29)

#elif _TARGET_APPLE | _TARGET_ANDROID
  return ::dlopen(filename, RTLD_LAZY);

#elif _TARGET_PC_WIN | _TARGET_XBOX

  int fn_slen = i_strlen(filename);
  wchar_t *fn_u16 = (wchar_t *)alloca((fn_slen + 1) * sizeof(wchar_t));
  if (MultiByteToWideChar(CP_UTF8, MB_ERR_INVALID_CHARS, filename, fn_slen + 1, fn_u16, fn_slen + 1) == 0)
    MultiByteToWideChar(CP_ACP, 0, filename, fn_slen + 1, fn_u16, fn_slen + 1);
  return (void *const)(::LoadLibraryExW(fn_u16, NULL, 0));

#else
  return NULL;
#endif
}

const char *os_dll_get_last_error_str()
{
#if _TARGET_PC_LINUX | _TARGET_APPLE | _TARGET_ANDROID
  return dlerror();
#elif _TARGET_PC_WIN | _TARGET_XBOX
  DWORD errorCode = GetLastError();

  // Note that this is a static buffer, so we must call this function only from one thread
  static char message[1024];
  FormatMessage(FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS | FORMAT_MESSAGE_MAX_WIDTH_MASK, NULL, errorCode,
    MAKELANGID(LANG_ENGLISH, SUBLANG_DEFAULT), (LPTSTR)&message, sizeof(message), NULL);
  return message;
#else
  return "n/a";
#endif
}

// Get function address from dynamic library. Return NULL if failed.
void *os_dll_get_symbol(void *handle, const char *function)
{
#if _TARGET_PC_LINUX | _TARGET_APPLE | _TARGET_ANDROID

  return ::dlsym(handle, function);

#elif _TARGET_PC_WIN | _TARGET_XBOX

  return ::GetProcAddress(HMODULE(handle), function);

#else
  return NULL;
#endif
}


// Close dynamic library.
bool os_dll_close(void *handle)
{
#if _TARGET_PC_LINUX | _TARGET_APPLE | _TARGET_ANDROID

  return ::dlclose(handle) == 0;

#elif _TARGET_PC_WIN | _TARGET_XBOX

  return ::FreeLibrary(HMODULE(handle)) != 0;

#else
  return false;
#endif
}

#define EXPORT_PULL dll_pull_osapiwrappers_dynLib
#include <supp/exportPull.h>
