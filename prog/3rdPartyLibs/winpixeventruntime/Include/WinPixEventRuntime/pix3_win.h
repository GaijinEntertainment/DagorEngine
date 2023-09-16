// Copyright (c) Microsoft Corporation. All rights reserved.

/*==========================================================================;
 *
 *  Copyright (C) Microsoft Corporation.  All Rights Reserved.
 *
 *  File:       PIX3_win.h
 *  Content:    PIX include file
 *              Don't include this file directly - use pix3.h
 *
 ****************************************************************************/

#pragma once

#ifndef _PIX3_H_
#error Don't include this file directly - use pix3.h
#endif

#ifndef _PIX3_WIN_H_
#define _PIX3_WIN_H_

// PIXEventsThreadInfo is defined in PIXEventsCommon.h
struct PIXEventsThreadInfo;

extern "C" PIXEventsThreadInfo* WINAPI PIXGetThreadInfo() noexcept;

#if defined(USE_PIX) && defined(USE_PIX_SUPPORTED_ARCHITECTURE)
// Notifies PIX that an event handle was set as a result of a D3D12 fence being signaled.
// The event specified must have the same handle value as the handle
// used in ID3D12Fence::SetEventOnCompletion.
extern "C" void WINAPI PIXNotifyWakeFromFenceSignal(_In_ HANDLE event);
#endif

// The following defines denote the different metadata values that have been used
// by tools to denote how to parse pix marker event data. The first two values
// are legacy values.
#define WINPIX_EVENT_UNICODE_VERSION 0
#define WINPIX_EVENT_ANSI_VERSION 1
#define WINPIX_EVENT_PIX3BLOB_VERSION 2

#define D3D12_EVENT_METADATA WINPIX_EVENT_PIX3BLOB_VERSION

__forceinline UINT64 PIXGetTimestampCounter()
{
    LARGE_INTEGER time = {};
    QueryPerformanceCounter(&time);
    return static_cast<UINT64>(time.QuadPart);
}

enum PIXHUDOptions
{
    PIX_HUD_SHOW_ON_ALL_WINDOWS = 0x1,
    PIX_HUD_SHOW_ON_TARGET_WINDOW_ONLY = 0x2,
    PIX_HUD_SHOW_ON_NO_WINDOWS = 0x4
};
DEFINE_ENUM_FLAG_OPERATORS(PIXHUDOptions);

#if defined(USE_PIX_SUPPORTED_ARCHITECTURE) && defined(USE_PIX)

#if WINAPI_FAMILY_PARTITION(WINAPI_PARTITION_DESKTOP | WINAPI_PARTITION_SYSTEM | WINAPI_PARTITION_GAMES)

namespace PixImpl
{
    __forceinline void * GetFunctionPtr(LPCSTR fnName) noexcept
    {
        HMODULE module = GetModuleHandleW(L"WinPixGpuCapturer.dll");
        if (module == NULL)
        {
            return nullptr;
        }

        auto fn = GetProcAddress(module, fnName);
        if (fn == nullptr)
        {
            return nullptr;
        }

        return fn;
    }
}

#include <shlobj.h>
#include <strsafe.h>
#include <knownfolders.h>

#define PIXERRORCHECK(value)  do {                      \
                                 if (FAILED(value))     \
                                     return nullptr;    \
                                 } while(0)

__forceinline HMODULE PIXLoadLatestWinPixGpuCapturerLibrary()
{
    HMODULE libHandle{};

    if (GetModuleHandleExW(0, L"WinPixGpuCapturer.dll", &libHandle))
    {
        return libHandle;
    }

    LPWSTR programFilesPath = nullptr;
    if (FAILED(SHGetKnownFolderPath(FOLDERID_ProgramFiles, KF_FLAG_DEFAULT, NULL, &programFilesPath)))
    {
        CoTaskMemFree(programFilesPath);
        return nullptr;
    }

    wchar_t pixSearchPath[MAX_PATH];

    if (FAILED(StringCchCopyW(pixSearchPath, MAX_PATH, programFilesPath)))
    {
        CoTaskMemFree(programFilesPath);
        return nullptr;
    }
    CoTaskMemFree(programFilesPath);

    PIXERRORCHECK(StringCchCatW(pixSearchPath, MAX_PATH, L"\\Microsoft PIX\\*"));

    WIN32_FIND_DATAW findData;
    bool foundPixInstallation = false;
    wchar_t newestVersionFound[MAX_PATH];
    wchar_t output[MAX_PATH];
    wchar_t possibleOutput[MAX_PATH];

    HANDLE hFind = FindFirstFileW(pixSearchPath, &findData);
    if (hFind != INVALID_HANDLE_VALUE)
    {
        do
        {
            if (((findData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) == FILE_ATTRIBUTE_DIRECTORY) &&
                (findData.cFileName[0] != '.'))
            {
                if (!foundPixInstallation || wcscmp(newestVersionFound, findData.cFileName) <= 0)
                {
                    // length - 1 to get rid of the wildcard character in the search path
                    PIXERRORCHECK(StringCchCopyNW(possibleOutput, MAX_PATH, pixSearchPath, wcslen(pixSearchPath) - 1));
                    PIXERRORCHECK(StringCchCatW(possibleOutput, MAX_PATH, findData.cFileName));
                    PIXERRORCHECK(StringCchCatW(possibleOutput, MAX_PATH, L"\\WinPixGpuCapturer.dll"));

                    DWORD result = GetFileAttributesW(possibleOutput);

                    if (result != INVALID_FILE_ATTRIBUTES && !(result & FILE_ATTRIBUTE_DIRECTORY))
                    {
                        foundPixInstallation = true;
                        PIXERRORCHECK(StringCchCopyW(newestVersionFound, _countof(newestVersionFound), findData.cFileName));
                        PIXERRORCHECK(StringCchCopyW(output, _countof(possibleOutput), possibleOutput));
                    }
                }
            }
        } while (FindNextFileW(hFind, &findData) != 0);
    }

    FindClose(hFind);

    if (!foundPixInstallation)
    {
        SetLastError(ERROR_FILE_NOT_FOUND);
        return nullptr;
    }

    return LoadLibraryW(output);
}

#undef PIXERRORCHECK

__forceinline HRESULT WINAPI PIXSetTargetWindow(HWND hwnd)
{
    typedef void(WINAPI* SetGlobalTargetWindowFn)(HWND);

    auto fn = (SetGlobalTargetWindowFn)PixImpl::GetFunctionPtr("SetGlobalTargetWindow");
    if (fn == nullptr)
    {
        return HRESULT_FROM_WIN32(GetLastError());
    }

    fn(hwnd);
    return S_OK;
}

__forceinline HRESULT WINAPI PIXGpuCaptureNextFrames(PCWSTR fileName, UINT32 numFrames)
{
    typedef HRESULT(WINAPI* CaptureNextFrameFn)(PCWSTR, UINT32);

    auto fn = (CaptureNextFrameFn)PixImpl::GetFunctionPtr("CaptureNextFrame");
    if (fn == nullptr)
    {
        return HRESULT_FROM_WIN32(GetLastError());
    }

    return fn(fileName, numFrames);
}

extern "C"  __forceinline HRESULT WINAPI PIXBeginCapture2(DWORD captureFlags, _In_opt_ const PPIXCaptureParameters captureParameters)
{
    if (captureFlags == PIX_CAPTURE_GPU)
    {
        typedef HRESULT(WINAPI* BeginProgrammaticGpuCaptureFn)(const PPIXCaptureParameters);

        auto fn = (BeginProgrammaticGpuCaptureFn)PixImpl::GetFunctionPtr("BeginProgrammaticGpuCapture");
        if (fn == nullptr)
        {
            return HRESULT_FROM_WIN32(GetLastError());
        }

        return fn(captureParameters);
    }
    else
    {
        return E_NOTIMPL;
    }
}

extern "C"  __forceinline HRESULT WINAPI PIXEndCapture(BOOL discard)
{
    UNREFERENCED_PARAMETER(discard);

    typedef HRESULT(WINAPI* EndProgrammaticGpuCaptureFn)(void);

    auto fn = (EndProgrammaticGpuCaptureFn)PixImpl::GetFunctionPtr("EndProgrammaticGpuCapture");
    if (fn == NULL)
    {
        return HRESULT_FROM_WIN32(GetLastError());
    }

    return fn();
}

__forceinline HRESULT WINAPI PIXForceD3D11On12()
{
    typedef HRESULT (WINAPI* ForceD3D11On12Fn)(void);

    auto fn = (ForceD3D11On12Fn)PixImpl::GetFunctionPtr("ForceD3D11On12");
    if (fn == NULL)
    {
        return HRESULT_FROM_WIN32(GetLastError());
    }

    return fn();
}

__forceinline HRESULT WINAPI PIXSetHUDOptions(PIXHUDOptions hudOptions)
{
    typedef HRESULT(WINAPI* SetHUDOptionsFn)(PIXHUDOptions);

    auto fn = (SetHUDOptionsFn)PixImpl::GetFunctionPtr("SetHUDOptions");
    if (fn == NULL)
    {
        return HRESULT_FROM_WIN32(GetLastError());
    }

    return fn(hudOptions);
}

#else
__forceinline HMODULE PIXLoadLatestWinPixGpuCapturerLibrary()
{
    return nullptr;
}
__forceinline HRESULT WINAPI PIXSetTargetWindow(HWND)
{
    return E_NOTIMPL;
}

__forceinline HRESULT WINAPI PIXGpuCaptureNextFrames(PCWSTR, UINT32)
{
    return E_NOTIMPL;
}
extern "C"  __forceinline HRESULT WINAPI PIXBeginCapture2(DWORD, _In_opt_ const PPIXCaptureParameters)
{
    return E_NOTIMPL;
}
extern "C"  __forceinline HRESULT WINAPI PIXEndCapture(BOOL)
{
    return E_NOTIMPL;
}
__forceinline HRESULT WINAPI PIXForceD3D11On12()
{
    return E_NOTIMPL;
}
__forceinline HRESULT WINAPI PIXSetHUDOptions(PIXHUDOptions)
{
    return E_NOTIMPL;
}
#endif // WINAPI_PARTITION

#endif // USE_PIX_SUPPORTED_ARCHITECTURE || USE_PIX

#endif //_PIX3_WIN_H_
