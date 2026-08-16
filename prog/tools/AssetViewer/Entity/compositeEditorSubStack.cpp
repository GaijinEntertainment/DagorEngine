// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include "compositeEditorSubStack.h"
#include <debug/dag_assert.h>

CompositeEditorSubContext &CompositeEditorSubStack::back()
{
  G_ASSERT(!stack.empty());
  return stack.back();
}

const CompositeEditorSubContext &CompositeEditorSubStack::back() const
{
  G_ASSERT(!stack.empty());
  return stack.back();
}

dag::ConstSpan<CompositeEditorSubContext> CompositeEditorSubStack::getFullContext() const { return make_span_const(stack); }

void CompositeEditorSubStack::setPendingCameraTransform(const TMatrix &tm)
{
  pendingCameraTm = tm;
  pendingCameraValid = true;
}

void CompositeEditorSubStack::clearPendingCameraTransform() { pendingCameraValid = false; }

void CompositeEditorSubStack::setPendingUniqueSwap(const String &assetName, unsigned dataBlockId)
{
  pendingUniqueName = assetName;
  pendingUniqueDataBlockId = dataBlockId;
}

void CompositeEditorSubStack::setPendingUniqueSwap(const String &assetName)
{
  G_ASSERT(!stack.empty());
  setPendingUniqueSwap(assetName, stack.back().subCompositeDataBlockId);
}

void CompositeEditorSubStack::clearPendingUniqueSwap()
{
  pendingUniqueName.clear();
  pendingUniqueDataBlockId = 0;
}

void CompositeEditorSubStack::push(DagorAsset *parentAsset, const TMatrix &subCompositeTm, unsigned dataBlockId)
{
  CompositeEditorSubContext ctx;
  ctx.parentAsset = parentAsset;
  ctx.subCompositeTm = subCompositeTm;
  ctx.subCompositeDataBlockId = dataBlockId;
  stack.push_back(eastl::move(ctx));
  entering = true;
}

CompositeEditorSubContext CompositeEditorSubStack::pop()
{
  G_ASSERT(!stack.empty());
  CompositeEditorSubContext ctx = eastl::move(stack.back());
  stack.pop_back();
  return ctx;
}

void CompositeEditorSubStack::setEntering() { entering = true; }

void CompositeEditorSubStack::setReturningEntering()
{
  entering = true;
  returning = true;
}

void CompositeEditorSubStack::abortEnter()
{
  G_ASSERT(entering && !stack.empty());
  stack.pop_back();
  entering = false;
  clearPendingCameraTransform();
}
