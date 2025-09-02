// Copyright (C) Gaijin Games KFT.  All rights reserved.
#pragma once

#include "driver.h"
#include "device_context_cmd.h"

namespace drv3d_dx12
{
#if !DX12_FIXED_EXECUTION_MODE
enum class CommandExecutionMode
{
  CONCURRENT = 0,
  CONCURRENT_SYNCED,
  IMMEDIATE,
  IMMEDIATE_FLUSH,
};
#endif

class SyncedCommandStore final : private AnyCommandStore
{
private:
  using BaseType = AnyCommandStore;

  class ExecutionSemaphoreSynced
  {
  private:
    std::atomic<int32_t> activePrimaryCommandsCount = 0;

  public:
    void reset();
    void onPushPrimaryCommand();
    void onConsumePrimaryCommand();
  };

#if !DX12_FIXED_EXECUTION_MODE || DX12_FIXED_SYNCED_EXECUTION_MODE
  ExecutionSemaphoreSynced executionSemaphoreSynced;
#endif
#if !DX12_FIXED_EXECUTION_MODE
  bool syncExecutionCommands = false;
#endif

  template <typename Cmd>
  void onPushCommand()
  {
    if constexpr (!std::decay_t<Cmd>::is_primary)
      return;
#if !DX12_FIXED_EXECUTION_MODE
    if (!syncExecutionCommands)
      return;
#endif
#if !DX12_FIXED_EXECUTION_MODE || DX12_FIXED_SYNCED_EXECUTION_MODE
    executionSemaphoreSynced.onPushPrimaryCommand();
#endif
  }

  template <typename ParamType, typename = void>
  struct DeviceContextCmdTypeExtractor
  {
    using type = ParamType;
  };

  template <typename ParamType>
  struct DeviceContextCmdTypeExtractor<ParamType, std::void_t<decltype(std::declval<ParamType>().cmd)>>
  {
    using type = std::decay_t<decltype(std::declval<ParamType>().cmd)>;
  };

  template <typename Param>
  void onConsumeCommand()
  {
    using ParamType = std::decay_t<Param>;
    using CmdType = typename DeviceContextCmdTypeExtractor<ParamType>::type;
    if constexpr (!CmdType::is_primary)
      return;
#if !DX12_FIXED_EXECUTION_MODE
    if (!syncExecutionCommands)
      return;
#endif
#if !DX12_FIXED_EXECUTION_MODE || DX12_FIXED_SYNCED_EXECUTION_MODE
    executionSemaphoreSynced.onConsumePrimaryCommand();
#endif
  }

  template <typename Param>
  static constexpr BaseType::ConsumerNotificationPolicy get_notifaction_policy()
  {
    return eastl::decay_t<Param>::is_primary ? ConsumerNotificationPolicy::WhenWaiting : ConsumerNotificationPolicy::Never;
  }

public:
  using BaseType::resize;
  using BaseType::waitToVisit;
  using BaseType::waitUntilEmpty;

  using ConsumerNotificationPolicy = BaseType::ConsumerNotificationPolicy;

  SyncedCommandStore() : BaseType() {}
  SyncedCommandStore(const SyncedCommandStore &) = delete;
  void operator=(const SyncedCommandStore &) = delete;

  BaseType &getBase() { return *this; }

#if !DX12_FIXED_EXECUTION_MODE
  void setExecutionMode(CommandExecutionMode mode);
#endif

  template <class U>
  uint32_t tryVisitAllAndDestroy(U &&visitor)
  {
    return BaseType::tryVisitAllAndDestroy([this, &visitor](auto &&value) {
      onConsumeCommand<decltype(value)>();
      visitor(std::forward<decltype(value)>(value));
    });
  }

  template <typename T>
  void pushBack(T &&v, ConsumerNotificationPolicy notification_policy = get_notifaction_policy<T>())
  {
    onPushCommand<std::decay_t<T>>();
    BaseType::pushBack(std::forward<T>(v), notification_policy);
  }

  template <typename T, typename P0>
  void pushBack(T &&v, const P0 *p0, size_t p0_count)
  {
    onPushCommand<T>();
    BaseType::pushBack(std::forward<T>(v), p0, p0_count, get_notifaction_policy<T>());
  }

  template <typename T, typename G0>
  void pushBack(T &&v, size_t p0_count, G0 g0)
  {
    onPushCommand<T>();
    BaseType::pushBack(std::forward<T>(v), p0_count, g0, get_notifaction_policy<T>());
  }

  template <typename T, typename P0, typename P1>
  void pushBack(const T &v, const P0 *p0, size_t p0_count, const P1 *p1, size_t p1_count)
  {
    onPushCommand<T>();
    BaseType::pushBack<T, P0, P1>(v, p0, p0_count, p1, p1_count, get_notifaction_policy<T>());
  }

  template <typename T, typename P0, typename P1, typename G>
  void pushBack(const T &v, size_t p0_count, G g)
  {
    onPushCommand<T>();
    BaseType::pushBack<T, P0, P1, G>(v, p0_count, g, get_notifaction_policy<T>());
  }
};
} // namespace drv3d_dx12
