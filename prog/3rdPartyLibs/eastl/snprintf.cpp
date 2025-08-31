#include <EASTL/string.h>
#include <stdio.h>
#include <wchar.h>

#if !EASTL_EASTDC_VSNPRINTF
#error "Not EASTL_EASTDC_VSNPRINTF configuration not supported"
#endif

namespace EA
{
namespace StdC
{

int Vsnprintf(char* pDestination, size_t n, const char* pFormat, va_list arguments)
{
  return vsnprintf(pDestination, n, pFormat, arguments);
}

int Vsnprintf(char16_t* pDestination, size_t n, const char16_t* pFormat, va_list arguments)
{
  return vswprintf((wchar_t*)pDestination, n, (const wchar_t*)pFormat, arguments);
}

int Vsnprintf(char32_t* pDestination, size_t n, const char32_t* pFormat, va_list arguments)
{
  return vswprintf((wchar_t*)pDestination, n, (const wchar_t*)pFormat, arguments);
}

#if defined(EA_CHAR8_UNIQUE) && EA_CHAR8_UNIQUE
int Vsnprintf(char8_t *pDestination, size_t n, const char8_t *pFormat, va_list arguments)
{
  return vsnprintf((char*)pDestination, n, (const char*)pFormat, arguments);
}
#endif

#if defined(EA_WCHAR_UNIQUE) && EA_WCHAR_UNIQUE
int Vsnprintf(wchar_t* pDestination, size_t n, const wchar_t* pFormat, va_list arguments)
{
  return vswprintf(pDestination, n, pFormat, arguments);
}
#endif

} // namespace StdC
} // namespace EA
