// Copyright (C) Gaijin Games KFT.  All rights reserved.
#pragma once

#include <dag/dag_vector.h>
#include <generic/dag_span.h>
#include <math/dag_TMatrix.h>
#include <util/dag_string.h>

class DagorAsset;
class IObjEntity;

// Per-level state captured when entering a sub-composite for editing.
struct CompositeEditorSubContext
{
  DagorAsset *parentAsset = nullptr;
  IObjEntity *parentGhostEntity = nullptr; // context entity, owned and destroyed by the stack user
  TMatrix subCompositeTm = TMatrix::IDENT; // world TM of the entered node
  unsigned subCompositeDataBlockId = 0;
};

// Tracks the nesting state for sub-composite editing.
// Plain data container: owns no entities, makes no engine calls.
class CompositeEditorSubStack
{
public:
  bool isEmpty() const { return stack.empty(); }
  int depth() const { return (int)stack.size(); }
  CompositeEditorSubContext &back();
  const CompositeEditorSubContext &back() const;
  dag::ConstSpan<CompositeEditorSubContext> getFullContext() const;

  bool isEntering() const { return entering; }
  // True when returning to a lower nested level; ancestor ghost TMs were already restored by exitSubCompositeEditing().
  bool isReturning() const { return returning; }
  void clearEntering()
  {
    entering = false;
    returning = false;
  }

  bool hasPendingCameraTransform() const { return pendingCameraValid; }
  const TMatrix &getPendingCameraTransform() const { return pendingCameraTm; }
  void setPendingCameraTransform(const TMatrix &tm);
  void clearPendingCameraTransform();

  // Survives a pop(); consumed in CompositeEditor::begin() after returning to parent.
  bool hasPendingUniqueSwap() const { return !pendingUniqueName.empty(); }
  const String &getPendingUniqueName() const { return pendingUniqueName; }
  unsigned getPendingUniqueDataBlockId() const { return pendingUniqueDataBlockId; }
  void setPendingUniqueSwap(const String &assetName, unsigned dataBlockId);
  void setPendingUniqueSwap(const String &assetName); // uses back().subCompositeDataBlockId
  void clearPendingUniqueSwap();

  // Sets entering=true; caller fills back().parentGhostEntity inside begin().
  void push(DagorAsset *parentAsset, const TMatrix &subCompositeTm, unsigned dataBlockId);

  // Returns the popped context; caller is responsible for destroying parentGhostEntity.
  CompositeEditorSubContext pop();

  // Signals begin() to recreate the ghost for the now-top context (exit-to-parent path).
  void setEntering();
  // Like setEntering(), but marks this as a return from a deeper level so begin() skips the ancestor shift.
  void setReturningEntering();

  // Undoes the last push(); only valid when entering=true came from push(), not setEntering().
  void abortEnter();

private:
  dag::Vector<CompositeEditorSubContext> stack;
  bool entering = false;
  bool returning = false;
  TMatrix pendingCameraTm = TMatrix::IDENT;
  bool pendingCameraValid = false;
  String pendingUniqueName;
  unsigned pendingUniqueDataBlockId = 0;
};
