#pragma once

#include <debug/dag_log.h>

#include "driver.h"
#include "d3d12_utils.h"
/*!
* @file
* @brief This file contains function used to manage different DirectX error codes.
*/

namespace drv3d_dx12
{
//@cond
#define D3D12_ERROR_INVALID_HOST_EXE_SDK_VERSION _HRESULT_TYPEDEF_(0x887E0003L)
//@endcond

/**
 * @brief Logs memory information in case of out-of-memory error.
 */
void report_oom_info();

/**
 * @brief Sets API's last error code to the requested value.
 * 
 * @param [in] error The code new value.
 */
void set_last_error(HRESULT error);

/**
 * @brief Returns API's last error code value.
 * 
 * @return The code value.
 */
HRESULT get_last_error_code();

/**
 * @brief Translates DirectX error code to an error message.
 * 
 * @param [in] ec   The error code.
 * @return          A character array containing null-terminated error message.
 */
inline const char *dxgi_error_code_to_string(HRESULT ec)
{
    //@cond
#define ENUM_CASE(Name) \
  case Name: return #Name
  switch (ec)
  {
    ENUM_CASE(E_FAIL); // returned by init code if a step fails in a fatal way
    ENUM_CASE(DXGI_ERROR_INVALID_CALL);
    ENUM_CASE(DXGI_ERROR_NOT_FOUND);
    ENUM_CASE(DXGI_ERROR_MORE_DATA);
    ENUM_CASE(DXGI_ERROR_UNSUPPORTED);
    ENUM_CASE(DXGI_ERROR_DEVICE_REMOVED);
    ENUM_CASE(DXGI_ERROR_DEVICE_HUNG);
    ENUM_CASE(DXGI_ERROR_DEVICE_RESET);
    ENUM_CASE(DXGI_ERROR_WAS_STILL_DRAWING);
    ENUM_CASE(DXGI_ERROR_FRAME_STATISTICS_DISJOINT);
    ENUM_CASE(DXGI_ERROR_GRAPHICS_VIDPN_SOURCE_IN_USE);
    ENUM_CASE(DXGI_ERROR_DRIVER_INTERNAL_ERROR);
    ENUM_CASE(DXGI_ERROR_NONEXCLUSIVE);
    ENUM_CASE(DXGI_ERROR_NOT_CURRENTLY_AVAILABLE);
    ENUM_CASE(DXGI_ERROR_REMOTE_CLIENT_DISCONNECTED);
    ENUM_CASE(DXGI_ERROR_REMOTE_OUTOFMEMORY);
    ENUM_CASE(DXGI_ERROR_ACCESS_LOST);
    ENUM_CASE(DXGI_ERROR_WAIT_TIMEOUT);
    ENUM_CASE(DXGI_ERROR_SESSION_DISCONNECTED);
    ENUM_CASE(DXGI_ERROR_RESTRICT_TO_OUTPUT_STALE);
    ENUM_CASE(DXGI_ERROR_CANNOT_PROTECT_CONTENT);
    ENUM_CASE(DXGI_ERROR_ACCESS_DENIED);
    ENUM_CASE(DXGI_ERROR_NAME_ALREADY_EXISTS);
    ENUM_CASE(DXGI_STATUS_UNOCCLUDED);
    ENUM_CASE(DXGI_STATUS_DDA_WAS_STILL_DRAWING);
    ENUM_CASE(DXGI_ERROR_MODE_CHANGE_IN_PROGRESS);
    ENUM_CASE(E_INVALIDARG);
    ENUM_CASE(E_OUTOFMEMORY);
#if _TARGET_PC_WIN
    ENUM_CASE(D3D12_ERROR_ADAPTER_NOT_FOUND);
    ENUM_CASE(D3D12_ERROR_DRIVER_VERSION_MISMATCH);
    ENUM_CASE(D3D12_ERROR_INVALID_HOST_EXE_SDK_VERSION);
#endif
  }
#undef ENUM_CASE
  // @endcond
  return "";
}

/**
 * @brief Checks expression result code for error. 
 * 
 * @param [in] result   The expression result code.
 * @param [in] expr     The string representation of the checked expression.
 * @param [in] file     The name of the file containing the expression.
 * @param [in] line     The number of the line containing the expression.
 * @return              The result code.
 * 
 * Whatever the check result is, the function does not report OOM info. 
 * In case of error, it is logged (via \c ERROR level) and API's last error code is set accordingly.
 */
inline HRESULT dx12_check_result_no_oom_report(HRESULT result, const char *DAGOR_HAS_LOGS(expr), const char *DAGOR_HAS_LOGS(file),
  int DAGOR_HAS_LOGS(line))
{
  if (SUCCEEDED(result))
    return result;

  set_last_error(result);

  auto resultStr = dxgi_error_code_to_string(result);
  if ('\0' == resultStr[0])
  {
    logerr("%s returned unknown return code %u, %s %u", expr, result, file, line);
  }
  else
  {
    logerr("%s returned %s, %s %u", expr, resultStr, file, line);
  }

  return result;
}

/**
 * @brief Checks result code for out-of-memory error.
 * 
 * @param [in] result   result code
 * @return               \c true if \b result is the out-of-memory error code.
 */
inline bool is_oom_error_code(HRESULT result) { return E_OUTOFMEMORY == result; }

/**
 * @brief Checks an expression result code for error.
 * 
 * @param [in] result   The expression result code
 * @param [in] expr     The string representation of the checked expression.
 * @param [in] file     The name of the file containing the expression.
 * @param [in] line     The number of the line containing the expression.
 * @return              The result code
 *
 * In case of error, it is logged (via \c ERROR level) and API's last error code is set accordingly. Also, the OOM info is reported.
 */
inline HRESULT dx12_check_result(HRESULT result, const char *DAGOR_HAS_LOGS(expr), const char *DAGOR_HAS_LOGS(file),
  int DAGOR_HAS_LOGS(line))
{
  if (SUCCEEDED(result))
    return result;

  if (is_oom_error_code(result))
  {
    report_oom_info();
  }

  set_last_error(result);

  auto resultStr = dxgi_error_code_to_string(result);
  if ('\0' == resultStr[0])
  {
    logerr("%s returned unknown return code %u, %s %u", expr, result, file, line);
  }
  else
  {
    logerr("%s returned %s, %s %u", expr, resultStr, file, line);
  }

  return result;
}

/**
 * @brief Checks if an error code corresponds to a recoverable error.
 * 
 * @param [in] error    The error code
 * @return              \c true if \b error corresponds to a recoverable (not device-related) error
 *
 * If \b error code is \c success, returns \c true.
 */
inline bool is_recoverable_error(HRESULT error)
{
  switch (error)
  {
    default: return true;
    // any device error is not recoverable
    case DXGI_ERROR_DEVICE_REMOVED:
    case DXGI_ERROR_DEVICE_HUNG:
    case DXGI_ERROR_DEVICE_RESET: return false;
  }
}

/**
 * @brief Checks an expression result code for error. 
 * 
 * @param [in] result   The expression result code.
 * @param [in] expr     The checked expression string representation.
 * @param [in] file     The name of the file containing the expression.
 * @param [in] line     The number of the line containing the expression.
 * @return              The result code.
 *
 * Whatever the check result is, the OOM info is not reported.
 * In case of error, it is logged (via \c DEBUG level) and API's last error code is set accordingly.
 */
inline HRESULT dx12_debug_result(HRESULT result, const char *DAGOR_HAS_LOGS(expr), const char *DAGOR_HAS_LOGS(file),
  int DAGOR_HAS_LOGS(line))
{
  if (SUCCEEDED(result))
    return result;

  set_last_error(result);

  auto resultStr = dxgi_error_code_to_string(result);
  if ('\0' == resultStr[0])
  {
    logdbg("%s returned unknown return code %u, %s %u", expr, result, file, line);
  }
  else
  {
    logdbg("%s returned %s, %s %u", expr, resultStr, file, line);
  }

  return result;
}

/**
 * @brief Checks an expression result code for error. 
 * 
 * @param [in]  result  The expression result code.
 * @return              The result code.
 *
 * Whatever the check result is, the OOM info is not reported.
 * In case of error, it is logged (via \c DEBUG level) and API's last error code is set accordingly.
 */
#define DX12_DEBUG_RESULT(r)drv3d_dx12::dx12_debug_result(r, #r, __FILE__, __LINE__) 

/**
 * @brief Checks expression result code for error.
 * 
 * @param [in] result   The expression result code.
 * @return              \c true if \b result is \c success.
 *
 * Whatever the result is, the OOM info is not reported.
 * In case of error, it is logged (via \c DEBUG level) and API's last error code is set accordingly.
 */
#define DX12_DEBUG_OK(r)     SUCCEEDED(DX12_DEBUG_RESULT(r))

/**
 * @brief Checks an expression result code for error. Whatever the result is, the OOM info is not reported.
 * 
 * @param [in] result   The expression result code.
 * @return              \c true if \b result is \c failure.
 *
 * In case of error, it is logged (via \c DEBUG level) and API's last error code is set accordingly.
 */
#define DX12_DEBUG_FAIL(r)   FAILED(DX12_DEBUG_RESULT(r))

/**
 * @brief Checks an expression result code for error.
 * 
 * @param [in] result   The expression result code.
 * @return              The result code.
 *
 * In case of error, it is logged (via \c ERROR level) and API's last error code is set accordingly. Also, the OOM info is reported.
 */
#define DX12_CHECK_RESULT(r) drv3d_dx12::dx12_check_result(r, #r, __FILE__, __LINE__)

/**
 * @brief Checks an expression result code.
 * 
 * @param [in] result   The expression result code.
 * @return              \c true if \b result is \c success.
 *
 * In case of error, it is logged (via \c ERROR level) and API's last error code is set accordingly. Also, the OOM info is reported.
 */
#define DX12_CHECK_OK(r)     SUCCEEDED(DX12_CHECK_RESULT(r))

/**
 * @brief Checks an expression result code.
 * 
 * @param [in] result   The expression result code.
 * @return              \c true if \b result is \c failure.
 *
 * In case of error, it is logged (via \c ERROR level) and API's last error code is set accordingly. Also, the OOM info is reported.
 */
#define DX12_CHECK_FAIL(r)   FAILED(DX12_CHECK_RESULT(r))

/**
* @def DX12_EXIT_ON_FAIL(r)
* 
 * @brief Checks an expression result code.
 * 
 * @param [in] result   The expression result code.
 *
 * In case of error, it is logged (via \c ERROR level) and API's last error code is set accordingly. 
 * Also, the OOM info is reported. Afterwards, application execution is aborted.
 * (In fact it is not, abortion is not implemented and this macro is equal to DX12_CHECK_RESULT)
 */

#define DX12_EXIT_ON_FAIL(r) \
if (DX12_CHECK_FAIL(r))    \
{                          \
    /* no-op */              \
}

/**
 * @brief Checks an expression result code for error.
 * 
 * @param [in] result   The expression result code.
 * @return              The result code.
 *
 * Whatever the check result is, the function does not report OOM info.
 * In case of error, it is logged (via \c ERROR level) and API's last error code is set accordingly.
 */
#define DX12_CHECK_RESULT_NO_OOM_CHECK(r) drv3d_dx12::dx12_check_result_no_oom_report(r, #r, __FILE__, __LINE__)

/**
 * @brief Logs resource allocation error info.
 * 
 * @param [in] desc The resource description.
 * 
 * The error is reported via \c ERROR level.
 */
inline void report_resource_alloc_info_error(const D3D12_RESOURCE_DESC &desc)
{
  logerr("DX12: Error while querying resource allocation info, resource desc: %s, %u, %u x %u x "
         "%u, %u, %s, %u by %u, %u, %08X",
    to_string(desc.Dimension), desc.Alignment, desc.Width, desc.Height, desc.DepthOrArraySize, desc.MipLevels,
    dxgi_format_name(desc.Format), desc.SampleDesc.Count, desc.SampleDesc.Quality, desc.Layout, desc.Flags);
}

} // namespace drv3d_dx12
