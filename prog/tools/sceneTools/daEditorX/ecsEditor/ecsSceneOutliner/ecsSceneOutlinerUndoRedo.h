// Copyright (C) Gaijin Games KFT.  All rights reserved.
#pragma once

#include <daECS/scene/scene.h>
#include <daECS/core/entityId.h>
#include <EditorCore/ec_rendEdObject.h>
#include <libTools/util/undo.h>
#include <util/dag_string.h>
#include "ecsSceneOutlinerPanel.h"

class ECSSceneObject;
class ECSEntityObject;

class AddImportUndoRedo : public UndoRedoObject
{
public:
  explicit AddImportUndoRedo(ECSSceneObject *sobj, ObjectEditor &editor) noexcept;

  void restore([[maybe_unused]] bool save_redo) override;
  void redo() override;

  size_t size() override { return sizeof(*this); }
  void accepted() override {}
  void get_description(String &s) override { s = "AddImportUndoRedo"; }

private:
  Ptr<ECSSceneObject> sobj;
  ObjectEditor &editor;
};

class ECSSceneOutlinerPanel::SetEntityParentUndoRedo : public UndoRedoObject
{
public:
  explicit SetEntityParentUndoRedo(ECSSceneObject *new_parent, ECSEntityObject *eobj, ECSSceneOutlinerPanel &owner) noexcept;

  void restore([[maybe_unused]] bool save_redo) override;
  void redo() override;

  size_t size() override { return sizeof(*this); }
  void accepted() override {}
  void get_description(String &s) override { s = "SetEntityParentUndoRedo"; }

private:
  Ptr<ECSSceneObject> prevParent;
  Ptr<ECSSceneObject> newParent;
  Ptr<ECSEntityObject> eobj;
  uint32_t prevOrder = 0;
  ECSSceneOutlinerPanel &owner;
};

class ECSSceneOutlinerPanel::SetSceneParentUndoRedo : public UndoRedoObject
{
public:
  explicit SetSceneParentUndoRedo(ECSSceneObject *new_parent, ECSSceneObject *sobj, ECSSceneOutlinerPanel &owner) noexcept;

  void restore(bool save_redo) override;
  void redo() override;

  size_t size() override { return sizeof(*this); }
  void accepted() override {}
  void get_description(String &s) override { s = "SetSceneParentUndoRedo"; }

private:
  Ptr<ECSSceneObject> prevParent;
  Ptr<ECSSceneObject> newParent;
  Ptr<ECSSceneObject> sobj;
  uint32_t prevOrder = 0;
  ECSSceneOutlinerPanel &owner;
};

class ECSSceneOutlinerPanel::SetEntityOrderUndoRedo : public UndoRedoObject
{
public:
  explicit SetEntityOrderUndoRedo(uint32_t prev_order, uint32_t new_order, ECSEntityObject *eobj,
    ECSSceneOutlinerPanel &owner) noexcept;

  void restore(bool save_redo) override;
  void redo() override;

  size_t size() override { return sizeof(*this); }
  void accepted() override {}
  void get_description(String &s) override { s = "SetEntityOrderUndoRedo"; }

private:
  uint32_t prevOrder = 0;
  uint32_t newOrder = 0;
  Ptr<ECSEntityObject> eobj;
  ECSSceneOutlinerPanel &owner;
};

class ECSSceneOutlinerPanel::SetSceneOrderUndoRedo : public UndoRedoObject
{
public:
  explicit SetSceneOrderUndoRedo(uint32_t prev_order, uint32_t new_order, ECSSceneObject *sobj, ECSSceneOutlinerPanel &owner) noexcept;

  void restore(bool save_redo) override;
  void redo() override;

  size_t size() override { return sizeof(*this); }
  void accepted() override {}
  void get_description(String &s) override { s = "SetSceneOrderUndoRedo"; }

private:
  uint32_t prevOrder = 0;
  uint32_t newOrder = 0;
  Ptr<ECSSceneObject> sobj;
  ECSSceneOutlinerPanel &owner;
};
