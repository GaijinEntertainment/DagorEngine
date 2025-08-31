// Copyright (C) Gaijin Games KFT.  All rights reserved.
#pragma once

#include <d3dcommon.h>
#include <dxgi.h>
#include <dxgitype.h>
#include <dxgi1_6.h>
#include <d3d12.h>
#include <d3dx12.h>


using DXGIFactory = IDXGIFactory7;
using DXGIAdapter = IDXGIAdapter4;
using DXGISwapChain = IDXGISwapChain3;
using D3DDevice = ID3D12Device5;
using D3DGraphicsCommandList = ID3D12GraphicsCommandList2;
using D3DCopyCommandList = ID3D12GraphicsCommandList;

#define WIN_FUNC
#define WIN_MEMBER
#define XBOX_FUNC   inline constexpr
#define XBOX_MEMBER inline static constexpr

#define FRAME_PIPELINE_TOKEN uint64_t

// on PC we only lock down the execution mode on release builds
#define DX12_FIXED_EXECUTION_MODE          DAGOR_DBGLEVEL == 0
#define DX12_FIXED_SYNCED_EXECUTION_MODE   0
// FG generates barriers for nodes that may not be used, but still generate split barriers with the needed
// state. The driver can not optimize those barriers away, so we have to disable split barriers for now.
#define DX12_ALLOW_SPLIT_BARRIERS          0
#define DX12_WHATCH_IN_FLIGHT_BARRIERS     DAGOR_DBGLEVEL > 0
#define DX12_VALIDATE_INPUT_LAYOUT_USES    DAGOR_DBGLEVEL > 0
#define DX12_TRACK_ACTIVE_DRAW_EVENTS      DAGOR_DBGLEVEL > 0
#define DX12_VALIDATE_USER_BARRIERS        DAGOR_DBGLEVEL > 0
#define DX12_AUTOMATIC_BARRIERS            1
#define DX12_PROCESS_USER_BARRIERS         1
#define DX12_RECORD_TIMING_DATA            1
#define DX12_CAPTURE_AFTER_LONG_FRAMES     (DX12_RECORD_TIMING_DATA && (DAGOR_DBGLEVEL > 0))
#define DX12_REPORT_PIPELINE_CREATE_TIMING 0

// Not possible to run with set to 0 on Windows.
#define DX12_USE_AUTO_PROMOTE_AND_DECAY 1

#define DX12_ENABLE_CONST_BUFFER_DESCRIPTORS 1

#define DX12_SELECTABLE_CALL_STACK_CAPTURE 1

#define DX12_ENABLE_COMMAND_LIST_LOGGER  0
#define DX12_ENABLE_FRAME_COMMAND_LOGGER (DAGOR_DBGLEVEL > 0)

#define DX12_VALIDATE_STREAM_CB_USAGE_WITHOUT_INITIALIZATION (DAGOR_DBGLEVEL > 0)

#define DX12_VALIDATA_COPY_COMMAND_LIST     (DAGOR_DBGLEVEL > 0)
#define DX12_VALIDATE_COMPUTE_COMMAND_LIST  (DAGOR_DBGLEVEL > 0)
#define DX12_VALIDATE_RAYTRACE_COMMAND_LIST (DAGOR_DBGLEVEL > 0)
#define DX12_VALIDATE_GRAPHICS_COMMAND_LIST (DAGOR_DBGLEVEL > 0)
#define DX12_VALIDATE_BINDLESS_TYPES        (DAGOR_DBGLEVEL > 0)

#define DX12_VALIDATE_BARRIER_TRANSITION (DAGOR_DBGLEVEL > 0)

#define DX12_PROCESS_USER_BARRIERS_DEFAULT 1

#define DX12_REMOVE_DEVICE_HOTKEY (DAGOR_DBGLEVEL > 0)

#define DX12_USE_ESRAM 0
