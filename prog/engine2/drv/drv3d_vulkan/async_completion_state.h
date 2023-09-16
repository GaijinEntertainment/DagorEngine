
#pragma once
#include <util/dag_stdint.h>
#include <atomic>

namespace drv3d_vulkan
{
struct ContextBackend;

class AsyncCompletionState
{
  enum class State : uint32_t
  {
    INITIAL,
    PENDING,
    COMPLETED,
  };
  std::atomic<State> state{State::INITIAL};

public:
  static constexpr int CLEANUP_DESTROY = 0;

  template <int Tag>
  void onDelayedCleanupBackend(drv3d_vulkan::ContextBackend &)
  {}

  template <int Tag>
  void onDelayedCleanupFrontend()
  {}

  template <int Tag>
  void onDelayedCleanupFinish()
  {
    delete this;
  }

  AsyncCompletionState() = default;
  ~AsyncCompletionState() = default;

  AsyncCompletionState(const AsyncCompletionState &) = delete;
  AsyncCompletionState &operator=(const AsyncCompletionState &) = delete;

  bool isPendingOrCompleted() const { return State::INITIAL != state.load(std::memory_order_seq_cst); }
  bool isPendingOrCompletedFast() const { return State::INITIAL != state.load(std::memory_order_relaxed); }

  bool isCompleted() const { return State::COMPLETED == state.load(std::memory_order_seq_cst); }
  bool isCompletedFast() const { return State::COMPLETED == state.load(std::memory_order_relaxed); }

  bool isPending() const { return State::PENDING == state.load(std::memory_order_seq_cst); }
  bool isPendingFast() const { return State::PENDING == state.load(std::memory_order_relaxed); }

  // only called by owning thread, no sync needed
  void reset() { state.store(State::INITIAL, std::memory_order_relaxed); }
  // may be called by different thread than checking thread, needs sequential consistency
  void completed() { state.store(State::COMPLETED, std::memory_order_seq_cst); }
  // called on owning thread, no sync needed
  void pending() { state.store(State::PENDING, std::memory_order_relaxed); }

  const char *resTypeString() { return "AsyncCompletionState"; }
};

} // namespace drv3d_vulkan