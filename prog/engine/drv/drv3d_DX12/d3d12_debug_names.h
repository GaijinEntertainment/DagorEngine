#pragma once

#include <EASTL/algorithm.h>
#include <util/dag_globDef.h>

#include "driver.h"

/*!
* @file
* @brief This file contains functions used to manage resource names.
*/
namespace drv3d_dx12
{

/**
 * @brief Converts a sequence of characters to a corresponding null-terminated sequence of wide characters.
 * 
 * @param [in]  str         The pointer to the sequence of characters.
 * @param [out] buf         The address of the buffer to store the converted wide character string.
 * @param [in]  max_len     The number of characters to convert.
 * @return                  A pointer to the start of \b buf, where the converted wide character sequence is stored.
 */
inline wchar_t *lazyToWchar(const char *str, wchar_t *buf, size_t max_len)
{
  auto ed = str + max_len - 1;
  auto at = buf;
  for (; *str && str != ed; ++str, ++at)
    *at = *str;
  *at = L'\0';
  return buf;
}


/**
 * @brief Returns the resource name.
 * 
 * @tparam N            The size of the character buffer.
 * 
 * @param [in]  res     A pointer to the resource.
 * @param [out] cbuf    The address of the character buffer for the name .
 * @return              A pointer to the start of \b cbuf, where the resource name is stored.
 * 
 * @note This is intended for debug only, this is possibly slow, so use with care!
 */
template <size_t N>
inline char *get_resource_name(ID3D12Resource *res, char (&cbuf)[N])
{
#if !_TARGET_XBOXONE
  wchar_t wcbuf[N];
  UINT cnt = sizeof(wcbuf);
  res->GetPrivateData(WKPDID_D3DDebugObjectNameW, &cnt, wcbuf);
  eastl::copy(wcbuf, wcbuf + cnt / sizeof(wchar_t), cbuf);
  cbuf[min<uint32_t>(cnt, N - 1)] = '\0';
#else
  G_UNUSED(res);
  cbuf[0] = 0;
#endif
  return cbuf;
}

/**
* @def DX12_SET_DEBUG_OBJ_NAME(obj, name)
* 
* @brief Sets name for an object.
* 
* @param [in] obj   A pointer to the object to set name for.
* @param [in] name  The name to set.
* @return           Operation result code.
* 
* @note The macro if defined only if \c DX12_DOES_SET_DEBUG_NAMES is defined.
*/

#if DX12_DOES_SET_DEBUG_NAMES


#define DX12_SET_DEBUG_OBJ_NAME(obj, name) obj->SetName(name)
#else
#define DX12_SET_DEBUG_OBJ_NAME(obj, name)
#endif

} // namespace drv3d_dx12
