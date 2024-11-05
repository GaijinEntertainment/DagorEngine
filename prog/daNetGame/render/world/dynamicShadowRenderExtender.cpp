// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include "dynamicShadowRenderExtender.h"

using Handle = DynamicShadowRenderExtender::Handle;
using HandleCleanup = DynamicShadowRenderExtender::HandleCleanup;

Handle DynamicShadowRenderExtender::registerExtension(Extension &&extension)
{
  Handle result;
  result.declare = declarations.subscribe(extension.declarationCallback);
  result.execute = executions.subscribe(extension.executionCallback);
  result.cleanup = HandleCleanup(this);
  ++count;
  onInvalidate();
  return result;
}

void DynamicShadowRenderExtender::declareAll(dabfg::Registry registry) { declarations.fire(registry); }

void DynamicShadowRenderExtender::executeAll(int updateIndex, int viewIndex) { executions.fire(updateIndex, viewIndex); }

void HandleCleanup::close()
{
  if (parent)
  {
    parent->onInvalidate();
    G_ASSERT(parent->count > 0);
    --parent->count;
  }
}

HandleCleanup &HandleCleanup::operator=(HandleCleanup &&other)
{
  close();
  parent = other.parent;
  other.parent = nullptr;
  return *this;
}
