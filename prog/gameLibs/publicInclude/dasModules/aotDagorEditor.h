//
// Dagor Engine 6.5 - Game Libraries
// Copyright (C) Gaijin Games KFT.  All rights reserved.
//
#pragma once

#include <daScript/daScript.h>
#include <dasModules/dasModulesCommon.h>
#include <daEditorE/editorCommon/entityEditor.h>
#include <daEditorE/editorCommon/entityObj.h>
#include <daEditorE/editorCommon/inGameEditor.h>
#include <dasModules/aotEcs.h>
#include <dasModules/dasDataBlock.h>

MAKE_TYPE_FACTORY(EntityObj, EntityObj);
MAKE_TYPE_FACTORY(UndoSystem, UndoSystem);

extern EntityObjEditor *get_entity_obj_editor();
extern void entity_obj_editor_for_each_entity(EntityObjEditor &, eastl::fixed_function<sizeof(void *), void(EntityObj *)>);

namespace bind_dascript
{

inline void save_entity_from_editor_to_blk(const ecs::EntityId eid, DataBlock &blk)
{
  if (!::get_entity_obj_editor())
    return;
  get_entity_obj_editor()->saveEntityToBlk(eid, blk);
}

inline const char *get_scene_file_path()
{
  if (!::get_entity_obj_editor())
    return "";
  return ::get_entity_obj_editor()->getSceneFilePath();
}

inline UndoSystem *get_undo_system()
{
  if (!::get_entity_obj_editor())
    return nullptr;
  return ::get_entity_obj_editor()->getUndoSystem();
}

inline ecs::EntityId create_entity_direct(const char *template_name, const TMatrix &tm)
{
  EntCreateData data(template_name, &tm);
  return ::get_entity_obj_editor()->createEntityDirect(&data);
}
inline ecs::EntityId create_entity_direct_riextra(const char *template_name, const TMatrix &tm, const char *riex_name)
{
  EntCreateData data(template_name, &tm, riex_name);
  return ::get_entity_obj_editor()->createEntityDirect(&data);
}
inline void add_entity_to_editor(ecs::EntityId eid) { ::get_entity_obj_editor()->addEntity(eid); }
inline void add_entity_to_editor_with_undo(ecs::EntityId eid) { ::get_entity_obj_editor()->addEntityEx(eid, true); }
inline void destroy_entity_direct(ecs::EntityId eid) { ::get_entity_obj_editor()->destroyEntityDirect(eid); }
inline void entity_object_editor_setEditMode(int mode) { ::get_entity_obj_editor()->setEditMode(mode); }
inline void entity_object_editor_selectEntity(ecs::EntityId eid, bool selected)
{
  ::get_entity_obj_editor()->selectEntity(eid, selected);
}
inline void entity_object_editor_removeEntity(ecs::EntityId eid, bool use_undo)
{
  if (!::get_entity_obj_editor())
    return;
  if (EditableObject *editableObject = ::get_entity_obj_editor()->getObjectByEid(eid))
    ::get_entity_obj_editor()->removeObject(editableObject, use_undo);
}
inline void entity_object_editor_updateObjectsList()
{
  if (EntityObjEditor *objEd = ::get_entity_obj_editor())
  {
    objEd->removeInvalidObjects();
    objEd->refreshEids();
  }
}
inline ecs::EntityId entity_object_editor_cloneEntity(ecs::EntityId eid, bool use_undo)
{
  if (!::get_entity_obj_editor())
    return ecs::INVALID_ENTITY_ID;
  if (EditableObject *editableObject = ::get_entity_obj_editor()->getObjectByEid(eid))
    if (EntityObj *clonedEntity = RTTI_cast<EntityObj>(get_entity_obj_editor()->cloneObject(editableObject, use_undo)))
      return clonedEntity->getEid();
  return ecs::INVALID_ENTITY_ID;
}
inline ecs::EntityId entity_object_editor_getFirstSelectedEntity() { return ::get_entity_obj_editor()->getFirstSelectedEntity(); }
inline void entity_object_editor_zoomAndCenter() { ::get_entity_obj_editor()->zoomAndCenter(); }
inline void entity_object_selectAll() { ::get_entity_obj_editor()->selectAll(); }
inline void entity_object_unselectAll() { ::get_entity_obj_editor()->unselectAll(); }

inline void entity_obj_editor_for_each_entity(const das::TBlock<void, const das::TTemporary<EntityObj>> &block, das::Context *context,
  das::LineInfoArg *at)
{
  if (EntityObjEditor *editor = get_entity_obj_editor())
  {
    vec4f args[1];
    context->invokeEx(
      block, args, nullptr,
      [&](das::SimNode *code) {
        struct
        {
          das::Context *context;
          vec4f *args;
          das::SimNode *code;
        } ctx = {context, args, code};
        entity_obj_editor_for_each_entity(*editor, [&ctx](EntityObj *obj) {
          ctx.args[0] = das::cast<EntityObj &>::from(*obj);
          ctx.code->eval(*ctx.context);
        });
      },
      at);
  }
}
} // namespace bind_dascript
