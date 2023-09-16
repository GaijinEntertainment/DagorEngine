/** Copyright (c) 2021, NVIDIA CORPORATION.  All rights reserved.*
* NVIDIA CORPORATION and its licensors retain all intellectual property
* and proprietary rights in and to this software, related documentation
* and any modifications thereto.  Any use, reproduction, disclosure or
* distribution of this software and related documentation without an express
* license agreement from NVIDIA CORPORATION is strictly prohibited.*/

#include <windows.h>
#include <TraceLoggingProvider.h>
#include <evntrace.h>
#include <stdlib.h>

#pragma comment(lib, "advapi32.lib")
#pragma comment(lib, "user32.lib")

typedef enum _NVSTATS_LATENCY_MARKER_TYPE
{
    NVSTATS_SIMULATION_START = 0,
    NVSTATS_SIMULATION_END = 1,
    NVSTATS_RENDERSUBMIT_START = 2,
    NVSTATS_RENDERSUBMIT_END = 3,
    NVSTATS_PRESENT_START = 4,
    NVSTATS_PRESENT_END = 5,
    NVSTATS_INPUT_SAMPLE = 6,
    NVSTATS_TRIGGER_FLASH = 7,
    NVSTATS_PC_LATENCY_PING = 8,
} NVSTATS_LATENCY_MARKER_TYPE;

typedef enum _NVSTATS_FLAGS
{
    NVSTATS_NO_PRESENT_MARKERS = 0x00000001,
} NVSTATS_FLAGS;

TRACELOGGING_DECLARE_PROVIDER(g_hReflexStatsComponentProvider);

#define NVSTATS_DEFINE() \
    TRACELOGGING_DEFINE_PROVIDER( \
        g_hReflexStatsComponentProvider, \
        "ReflexStatsTraceLoggingProvider", \
        (0x0d216f06, 0x82a6, 0x4d49, 0xbc, 0x4f, 0x8f, 0x38, 0xae, 0x56, 0xef, 0xab)); \
    UINT   g_ReflexStatsWindowMessage = 0; \
    WORD   g_ReflexStatsVirtualKey = 0; \
    HANDLE g_ReflexStatsQuitEvent = NULL; \
    HANDLE g_ReflexStatsPingThread = NULL; \
    bool   g_ReflexStatsEnable = false; \
    UINT   g_ReflexStatsFlags = 0; \
    DWORD ReflexStatsPingThreadProc(LPVOID lpThreadParameter) \
    { \
        DWORD minPingInterval = 100 /*ms*/; \
        DWORD maxPingInterval = 300 /*ms*/; \
        while (WAIT_TIMEOUT == WaitForSingleObject(g_ReflexStatsQuitEvent, minPingInterval + (rand() % (maxPingInterval - minPingInterval)))) \
        { \
            if (!g_ReflexStatsEnable) \
            { \
                continue; \
            } \
            HWND hWnd = GetForegroundWindow(); \
            if (hWnd) \
            { \
                DWORD dwProcessId = 0; \
                (void)GetWindowThreadProcessId(hWnd, &dwProcessId); \
                if (GetCurrentProcessId() == dwProcessId) \
                { \
                    if ((g_ReflexStatsVirtualKey == VK_F13) || \
                        (g_ReflexStatsVirtualKey == VK_F14) || \
                        (g_ReflexStatsVirtualKey == VK_F15)) \
                    { \
                        TraceLoggingWrite(g_hReflexStatsComponentProvider, "ReflexStatsInput"); \
                        PostMessageW(hWnd, WM_KEYDOWN, g_ReflexStatsVirtualKey, 0x00000001); \
                        PostMessageW(hWnd, WM_KEYUP,   g_ReflexStatsVirtualKey, 0xC0000001); \
                    } \
                    else if (g_ReflexStatsWindowMessage) \
                    { \
                        TraceLoggingWrite(g_hReflexStatsComponentProvider, "ReflexStatsInput"); \
                        PostMessageW(hWnd, g_ReflexStatsWindowMessage, 0, 0); \
                    } \
                    else \
                    { \
                        break; \
                    } \
                } \
            } \
        } \
        return S_OK; \
    } \
    void WINAPI ReflexStatsComponentProviderCb(LPCGUID, ULONG ControlCode, UCHAR, ULONGLONG, ULONGLONG, PEVENT_FILTER_DESCRIPTOR, PVOID) \
    { \
        switch (ControlCode) \
        { \
        case EVENT_CONTROL_CODE_ENABLE_PROVIDER: \
            g_ReflexStatsEnable = true; \
            break; \
        case EVENT_CONTROL_CODE_DISABLE_PROVIDER: \
            g_ReflexStatsEnable = false; \
            break; \
        case EVENT_CONTROL_CODE_CAPTURE_STATE: \
            TraceLoggingWrite(g_hReflexStatsComponentProvider, "ReflexStatsFlags", TraceLoggingUInt32(g_ReflexStatsFlags, "Flags")); \
            break; \
        default: \
            break; \
        } \
    }

#define NVSTATS_INIT(vk, flags) \
    if (((vk) == 0) && (g_ReflexStatsWindowMessage == 0)) \
    { \
        g_ReflexStatsWindowMessage = RegisterWindowMessageW(L"NVIDIA_Reflex_PC_Latency_Ping"); \
    } \
    g_ReflexStatsVirtualKey = (vk); \
    g_ReflexStatsFlags = (flags); \
    if (!g_ReflexStatsQuitEvent) \
    { \
        g_ReflexStatsQuitEvent = CreateEventW(NULL, 1, 0, NULL); \
    } \
    if (g_ReflexStatsQuitEvent) \
    { \
        TraceLoggingRegisterEx(g_hReflexStatsComponentProvider, ReflexStatsComponentProviderCb, NULL); \
	    TraceLoggingWrite(g_hReflexStatsComponentProvider, "ReflexStatsInit"); \
        if (!g_ReflexStatsPingThread) \
        { \
            g_ReflexStatsPingThread = CreateThread(NULL, 0, (LPTHREAD_START_ROUTINE)ReflexStatsPingThreadProc, NULL, 0, NULL); \
        } \
    }

#define NVSTATS_MARKER(mrk,frid) TraceLoggingWrite(g_hReflexStatsComponentProvider, "ReflexStatsEvent", TraceLoggingUInt32((mrk), "Marker"), TraceLoggingUInt64((frid), "FrameID"))
#define NVSTATS_MARKER_V2(mrk,frid) TraceLoggingWrite(g_hReflexStatsComponentProvider, "ReflexStatsEventV2", TraceLoggingUInt32((mrk), "Marker"), TraceLoggingUInt64((frid), "FrameID"), TraceLoggingUInt32(g_ReflexStatsFlags, "Flags"))

#define NVSTATS_SHUTDOWN() \
    if (g_ReflexStatsPingThread) \
    { \
        if (g_ReflexStatsQuitEvent) \
        { \
            SetEvent(g_ReflexStatsQuitEvent); \
        } \
        (void)WaitForSingleObject(g_ReflexStatsPingThread, 1000); \
        g_ReflexStatsPingThread = NULL; \
    } \
    TraceLoggingWrite(g_hReflexStatsComponentProvider, "ReflexStatsShutdown"); \
    TraceLoggingUnregister(g_hReflexStatsComponentProvider); \
    if (g_ReflexStatsQuitEvent) \
    { \
        CloseHandle(g_ReflexStatsQuitEvent); \
        g_ReflexStatsQuitEvent = NULL; \
    }

#define NVSTATS_IS_PING_MSG_ID(msgId) ((msgId) == g_ReflexStatsWindowMessage)

extern "C" UINT   g_ReflexStatsWindowMessage;
extern "C" WORD   g_ReflexStatsVirtualKey;
extern "C" HANDLE g_ReflexStatsQuitEvent;
extern "C" HANDLE g_ReflexStatsPingThread;
extern "C" bool   g_ReflexStatsEnable;
extern "C" UINT   g_ReflexStatsFlags;

DWORD ReflexStatsPingThreadProc(LPVOID lpThreadParameter);
void WINAPI ReflexStatsComponentProviderCb(LPCGUID, ULONG ControlCode, UCHAR, ULONGLONG, ULONGLONG, PEVENT_FILTER_DESCRIPTOR, PVOID);
