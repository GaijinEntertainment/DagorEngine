// Copyright (C) Gaijin Games KFT.  All rights reserved.
#pragma once

#include <generic/dag_fixedMoveOnlyFunction.h>
#include <util/dag_multicastEvent.h>
#include <EASTL/fixed_function.h>
#include <vecmath/dag_vecMathDecl.h>
#include <render/daFrameGraph/daFG.h>

class DynamicShadowRenderExtender
{
public:
  using DeclarationCallback = void(dafg::Registry registry);
  using ExecutionCallback = void(int updateIndex, int viewIndex);

  struct Extension
  {
    eastl::fixed_function<64, DeclarationCallback> declarationCallback;
    eastl::fixed_function<64, ExecutionCallback> executionCallback;
  };

  class HandleCleanup
  {
    HandleCleanup() : parent(nullptr) {}
    HandleCleanup(DynamicShadowRenderExtender *parent_) : parent(parent_) {}
    HandleCleanup(HandleCleanup &&other) : parent(other.parent) { other.parent = nullptr; }
    HandleCleanup &operator=(HandleCleanup &&other);
    ~HandleCleanup() { close(); }
    void close();

    friend class DynamicShadowRenderExtender;
    DynamicShadowRenderExtender *parent;
  };

  class Handle
  {
    friend class DynamicShadowRenderExtender;
    CallbackToken declare;
    CallbackToken execute;
    HandleCleanup cleanup;
  };

  [[nodiscard]] Handle registerExtension(Extension &&extension);
  void declareAll(dafg::Registry registry);
  void executeAll(int updateIndex, int viewIndex);

  using OnInvalidate = void();
  inline DynamicShadowRenderExtender(dag::FixedMoveOnlyFunction<sizeof(void *), OnInvalidate> &&onInvalidate_) :
    onInvalidate(eastl::move(onInvalidate_))
  {}

  inline ~DynamicShadowRenderExtender()
  {
    // Extensions must be unregistered before destruction.
    G_ASSERT(count == 0);
  }

private:
  MulticastEvent<DeclarationCallback> declarations;
  MulticastEvent<ExecutionCallback> executions;
  size_t count = 0;
  dag::FixedMoveOnlyFunction<sizeof(void *), OnInvalidate> onInvalidate;
};
