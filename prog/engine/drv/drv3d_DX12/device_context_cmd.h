// Copyright (C) Gaijin Games KFT.  All rights reserved.
#pragma once

#include "debug/call_stack.h"
#include "variant_vector.h"
#include "versioned_com_ptr.h"

// Dependencies of commands
#include "device_queue.h"
#include "extra_data_arrays.h"
#include "fsr_args.h"
#include "fsr2_wrapper.h"
#include "info_types.h"
#include "pipeline.h"
#include "query_manager.h"
#include "resource_memory_heap.h"
#include "swapchain.h"
#include "viewport_state.h"
#include "xess_wrapper.h"
#include <3d/dag_amdFsr.h>
#include <3d/dag_nvFeatures.h>
#include <drv/3d/dag_commands.h>
#include <drv/3d/dag_decl.h>
#include <drv/3d/dag_heap.h>
#include <osApiWrappers/dag_events.h>


namespace drv3d_dx12
{
#define DX12_BEGIN_CONTEXT_COMMAND(name)            \
  struct Cmd##name : debug::call_stack::CommandData \
  {

#define DX12_BEGIN_CONTEXT_COMMAND_EXT_1(name, param0Type, param0Name) \
  struct Cmd##name : debug::call_stack::CommandData                    \
  {

#define DX12_BEGIN_CONTEXT_COMMAND_EXT_2(name, param0Type, param0Name, param1Type, param1Name) \
  struct Cmd##name : debug::call_stack::CommandData                                            \
  {

#define DX12_END_CONTEXT_COMMAND \
  }                              \
  ;

#define DX12_CONTEXT_COMMAND_PARAM(type, name)             type name;
#define DX12_CONTEXT_COMMAND_PARAM_ARRAY(type, name, size) type name[size];
#include "device_context_cmd.inc.h"
#undef DX12_BEGIN_CONTEXT_COMMAND
#undef DX12_BEGIN_CONTEXT_COMMAND_EXT_1
#undef DX12_BEGIN_CONTEXT_COMMAND_EXT_2
#undef DX12_END_CONTEXT_COMMAND
#undef DX12_CONTEXT_COMMAND_PARAM
#undef DX12_CONTEXT_COMMAND_PARAM_ARRAY

#if _TARGET_XBOX
#include "device_context_xbox.h"
#endif

using AnyCommandPack = TypePack<
#define DX12_BEGIN_CONTEXT_COMMAND(name)                               Cmd##name,
#define DX12_BEGIN_CONTEXT_COMMAND_EXT_1(name, param0Type, param0Name) ExtendedVariant<Cmd##name, param0Type>,
#define DX12_BEGIN_CONTEXT_COMMAND_EXT_2(name, param0Type, param0Name, param1Type, param1Name) \
  ExtendedVariant2<Cmd##name, param0Type, param1Type>,
#define DX12_END_CONTEXT_COMMAND
#define DX12_CONTEXT_COMMAND_PARAM(type, name)
#define DX12_CONTEXT_COMMAND_PARAM_ARRAY(type, name, size)
#include "device_context_cmd.inc.h"
#undef DX12_BEGIN_CONTEXT_COMMAND
#undef DX12_BEGIN_CONTEXT_COMMAND_EXT_1
#undef DX12_BEGIN_CONTEXT_COMMAND_EXT_2
#undef DX12_END_CONTEXT_COMMAND
#undef DX12_CONTEXT_COMMAND_PARAM
#undef DX12_CONTEXT_COMMAND_PARAM_ARRAY
  void>;

using AnyCommandStore = VariantRingBuffer<AnyCommandPack>;
}
