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

struct CmdBase
{
  inline static constexpr bool is_primary = true;
  inline static constexpr bool use_device = true;
};

template <class T>
concept CmdConcept = eastl::is_base_of_v<CmdBase, T>;

struct CommandRequirement
{
  const bool isPrimary : 1;
  const bool useDevice : 1;

  template <class T>
  using CmdType = eastl::decay_t<eastl::remove_pointer_t<T>>;

  template <CmdConcept T>
  CommandRequirement(const T &) : isPrimary{CmdType<T>::is_primary}, useDevice{CmdType<T>::use_device}
  {}
  template <CmdConcept T, typename P0, typename P1>
  CommandRequirement(const ExtendedVariant2<T, P0, P1> &) : isPrimary{T::is_primary}, useDevice{T::use_device}
  {}
  template <CmdConcept T, typename P0>
  CommandRequirement(const ExtendedVariant<T, P0> &) : isPrimary{T::is_primary}, useDevice{T::use_device}
  {}
};

#define DX12_CONTEXT_COMMAND_COMMAND_DATA(isPrimary) \
  eastl::conditional_t<isPrimary, debug::call_stack::CommandData, debug::call_stack::null::CommandData>

#define DX12_CONTEXT_COMMAND_IS_PRIMARY(isPrimary) inline static constexpr bool is_primary = isPrimary;

#define DX12_BEGIN_CONTEXT_COMMAND(isPrimary, name)                        \
  struct Cmd##name : CmdBase, DX12_CONTEXT_COMMAND_COMMAND_DATA(isPrimary) \
  {                                                                        \
    DX12_CONTEXT_COMMAND_IS_PRIMARY(isPrimary)

#define DX12_BEGIN_CONTEXT_COMMAND_EXT_1(isPrimary, name, param0Type, param0Name) \
  struct Cmd##name : CmdBase, DX12_CONTEXT_COMMAND_COMMAND_DATA(isPrimary)        \
  {                                                                               \
    DX12_CONTEXT_COMMAND_IS_PRIMARY(isPrimary)

#define DX12_BEGIN_CONTEXT_COMMAND_EXT_2(isPrimary, name, param0Type, param0Name, param1Type, param1Name) \
  struct Cmd##name : CmdBase, DX12_CONTEXT_COMMAND_COMMAND_DATA(isPrimary)                                \
  {                                                                                                       \
    DX12_CONTEXT_COMMAND_IS_PRIMARY(isPrimary)

#define DX12_END_CONTEXT_COMMAND \
  }                              \
  ;

#define DX12_CONTEXT_COMMAND_PARAM(type, name)             type name;
#define DX12_CONTEXT_COMMAND_PARAM_ARRAY(type, name, size) type name[size];
#define DX12_CONTEXT_COMMAND_USE_DEVICE(value)             inline static constexpr bool use_device = value;
#include "device_context_cmd.inc.h"


#define DX12_BEGIN_CONTEXT_COMMAND(isPrimary, name)                               Cmd##name,
#define DX12_BEGIN_CONTEXT_COMMAND_EXT_1(isPrimary, name, param0Type, param0Name) ExtendedVariant<Cmd##name, param0Type>,
#define DX12_BEGIN_CONTEXT_COMMAND_EXT_2(isPrimary, name, param0Type, param0Name, param1Type, param1Name) \
  ExtendedVariant2<Cmd##name, param0Type, param1Type>,

using AnyCommandPack = TypePack<
#include "device_context_cmd.inc.h"
  void>;

using AnyCommandStore = ConcurrentVariantRingBuffer<AnyCommandPack>;
}
