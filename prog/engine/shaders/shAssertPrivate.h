// Copyright (C) Gaijin Games KFT.  All rights reserved.
#pragma once

#include <shaders/dag_shAssert.h>
#include <3d/dag_resPtr.h>
#include <3d/dag_ringCPUQueryLock.h>
#include <EASTL/vector.h>
#include <EASTL/vector_set.h>
#include <EASTL/array.h>

struct ScriptedShadersBinDumpOwner;

namespace shader_assert
{

struct AssertionContext
{
  AssertionContext() = default;

  AssertionContext(AssertionContext &&) = default;
  AssertionContext &operator=(AssertionContext &&) = default;

  AssertionContext(AssertionContext const &) = delete;
  AssertionContext &operator=(AssertionContext const &) = delete;

  ~AssertionContext() { close(); }

#if DAGOR_DBGLEVEL > 0

  static constexpr uint32_t RING_BUFFER_SIZE = 4;
  static constexpr uint32_t MAX_STACK_SIZE = 64;
  using stack_t = eastl::array<void *, MAX_STACK_SIZE>;
  using stacks_on_frame_t = eastl::vector<stack_t>;

  UniqueBuf assertionBuffer;
  UniqueBuf assertionInfoBuffer;
  RingCPUBufferLock assertionRingBuffer;
  eastl::vector_set<int> failedByClass;
  eastl::array<stacks_on_frame_t, RING_BUFFER_SIZE> stacksOnFrames;
  uint32_t currentFrame = 0;

  void init(ScriptedShadersBinDumpOwner const &dump_owner);
  void close();
  void reset(ScriptedShadersBinDumpOwner const &dump_owner);
  void readback(ScriptedShadersBinDumpOwner const &dump_owner);
  void bind(int shader_class_id, ScriptedShadersBinDumpOwner const &dump_owner);

  bool active() const { return bool(assertionBuffer); }

#else

  void init(ScriptedShadersBinDumpOwner const &) {}
  void close() {}
  void reset(ScriptedShadersBinDumpOwner const &) {}
  void readback(ScriptedShadersBinDumpOwner const &) {}
  void bind(int, ScriptedShadersBinDumpOwner const &) {}
  bool active() const { return false; }

#endif
};

} // namespace shader_assert
