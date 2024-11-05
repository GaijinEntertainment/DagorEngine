// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include <dasModules/aotDagorEditor.h>

namespace bind_dascript
{

struct EntityObjAnnotation : das::ManagedStructureAnnotation<EntityObj, false>
{
  EntityObjAnnotation(das::ModuleLibrary &ml) : ManagedStructureAnnotation("EntityObj", ml)
  {
    cppName = " ::EntityObj";
    addProperty<DAS_BIND_MANAGED_PROP(getEid)>("eid", "getEid");
    addProperty<DAS_BIND_MANAGED_PROP(isValid)>("isValid");
  }
};

struct UndoSystemAnnotation final : das::ManagedStructureAnnotation<UndoSystem, false>
{
  UndoSystemAnnotation(das::ModuleLibrary &ml) : ManagedStructureAnnotation("UndoSystem", ml) { cppName = " ::UndoSystem"; }
};

class DagorEditorModule final : public das::Module
{
public:
  DagorEditorModule() : das::Module("DagorEditor")
  {
    das::ModuleLibrary lib(this);
    addBuiltinDependency(lib, require("ecs"));
    addBuiltinDependency(lib, require("DagorDataBlock"));
    addAnnotation(das::make_smart<EntityObjAnnotation>(lib));
    addAnnotation(das::make_smart<UndoSystemAnnotation>(lib));

    das::addExtern<DAS_BIND_FUN(::EntityObjEditor::saveComponent)>(*this, lib, "entity_obj_editor_saveComponent",
      das::SideEffects::accessExternal, "::EntityObjEditor::saveComponent");

    das::addExtern<DAS_BIND_FUN(::EntityObjEditor::saveAddTemplate)>(*this, lib, "entity_obj_editor_saveAddTemplate",
      das::SideEffects::accessExternal, "::EntityObjEditor::saveAddTemplate");

    das::addExtern<DAS_BIND_FUN(::EntityObjEditor::saveDelTemplate)>(*this, lib, "entity_obj_editor_saveDelTemplate",
      das::SideEffects::accessExternal, "::EntityObjEditor::saveDelTemplate");

    das::addExtern<DAS_BIND_FUN(bind_dascript::get_scene_file_path)>(*this, lib, "entity_obj_editor_getSceneFilePath",
      das::SideEffects::accessExternal, "bind_dascript::get_scene_file_path");

    das::addExtern<DAS_BIND_FUN(bind_dascript::create_entity_direct)>(*this, lib, "entity_object_editor_createEntityDirect",
      das::SideEffects::modifyExternal, "bind_dascript::create_entity_direct");

    das::addExtern<DAS_BIND_FUN(bind_dascript::create_entity_direct_riextra)>(*this, lib,
      "entity_object_editor_createEntityDirectRIExtra", das::SideEffects::modifyExternal,
      "bind_dascript::create_entity_direct_riextra");

    das::addExtern<DAS_BIND_FUN(bind_dascript::add_entity_to_editor)>(*this, lib, "entity_object_editor_addEntity",
      das::SideEffects::modifyExternal, "bind_dascript::add_entity_to_editor");

    das::addExtern<DAS_BIND_FUN(bind_dascript::entity_object_editor_cloneEntity)>(*this, lib, "entity_object_editor_cloneEntity",
      das::SideEffects::modifyExternal, "bind_dascript::entity_object_editor_cloneEntity");

    das::addExtern<DAS_BIND_FUN(bind_dascript::add_entity_to_editor_with_undo)>(*this, lib, "entity_object_editor_addEntityWithUndo",
      das::SideEffects::modifyExternal, "bind_dascript::add_entity_to_editor_with_undo");

    das::addExtern<DAS_BIND_FUN(bind_dascript::destroy_entity_direct)>(*this, lib, "entity_object_editor_destroyEntityDirect",
      das::SideEffects::modifyExternal, "bind_dascript::destroy_entity_direct");

    das::addExtern<DAS_BIND_FUN(bind_dascript::entity_object_editor_setEditMode)>(*this, lib, "entity_object_editor_setEditMode",
      das::SideEffects::modifyExternal, "bind_dascript::entity_object_editor_setEditMode");

    das::addExtern<DAS_BIND_FUN(bind_dascript::entity_object_editor_selectEntity)>(*this, lib, "entity_object_editor_selectEntity",
      das::SideEffects::modifyExternal, "bind_dascript::entity_object_editor_selectEntity");

    das::addExtern<DAS_BIND_FUN(bind_dascript::entity_object_editor_getFirstSelectedEntity)>(*this, lib,
      "entity_object_editor_getFirstSelectedEntity", das::SideEffects::accessExternal,
      "bind_dascript::entity_object_editor_getFirstSelectedEntity");

    das::addExtern<DAS_BIND_FUN(bind_dascript::entity_object_selectAll)>(*this, lib, "entity_object_selectAll",
      das::SideEffects::modifyExternal, "bind_dascript::entity_object_selectAll");

    das::addExtern<DAS_BIND_FUN(bind_dascript::entity_object_unselectAll)>(*this, lib, "entity_object_unselectAll",
      das::SideEffects::modifyExternal, "bind_dascript::entity_object_unselectAll");

    das::addExtern<DAS_BIND_FUN(bind_dascript::entity_object_editor_zoomAndCenter)>(*this, lib, "entity_object_editor_zoomAndCenter",
      das::SideEffects::modifyExternal, "bind_dascript::entity_object_editor_zoomAndCenter");

    das::addExtern<DAS_BIND_FUN(bind_dascript::entity_obj_editor_for_each_entity)>(*this, lib, "entity_obj_editor_for_each_entity",
      das::SideEffects::modifyExternal, "bind_dascript::entity_obj_editor_for_each_entity");

    das::addExtern<DAS_BIND_FUN(::is_editor_activated)>(*this, lib, "is_editor_activated", das::SideEffects::accessExternal,
      "::is_editor_activated");
    das::addExtern<DAS_BIND_FUN(::is_editor_free_camera_enabled)>(*this, lib, "is_editor_free_camera_enabled",
      das::SideEffects::accessExternal, "::is_editor_free_camera_enabled");

    das::addExtern<DAS_BIND_FUN(::init_entity_object_editor)>(*this, lib, "init_entity_object_editor",
      das::SideEffects::modifyExternal, "::init_entity_object_editor");

    das::addExtern<DAS_BIND_FUN(bind_dascript::entity_object_editor_removeEntity)>(*this, lib, "entity_object_editor_removeEntity",
      das::SideEffects::modifyExternal, "bind_dascript::entity_object_editor_removeEntity")
      ->args({"eid", "use_undo"});

    das::addExtern<DAS_BIND_FUN(bind_dascript::entity_object_editor_updateObjectsList)>(*this, lib,
      "entity_object_editor_updateObjectsList", das::SideEffects::modifyExternal,
      "bind_dascript::entity_object_editor_updateObjectsList");

    das::addExtern<DAS_BIND_FUN(bind_dascript::save_entity_from_editor_to_blk)>(*this, lib, "save_entity_from_editor_to_blk",
      das::SideEffects::modifyArgumentAndAccessExternal, "bind_dascript::save_entity_from_editor_to_blk");

    das::addExtern<DAS_BIND_FUN(bind_dascript::get_undo_system)>(*this, lib, "get_undo_system", das::SideEffects::accessExternal,
      "bind_dascript::get_undo_system");

    using method_UndoSystem_begin = DAS_CALL_MEMBER(UndoSystem::begin);
    das::addExtern<DAS_CALL_METHOD(method_UndoSystem_begin)>(*this, lib, "begin", das::SideEffects::modifyArgument,
      DAS_CALL_MEMBER_CPP(UndoSystem::begin))
      ->args({"undo_system"});

    using method_UndoSystem_accept = DAS_CALL_MEMBER(UndoSystem::accept);
    das::addExtern<DAS_CALL_METHOD(method_UndoSystem_accept)>(*this, lib, "accept", das::SideEffects::modifyArgument,
      DAS_CALL_MEMBER_CPP(UndoSystem::accept))
      ->args({"undo_system", "operation_name"});

    using method_UndoSystem_cancel = DAS_CALL_MEMBER(UndoSystem::cancel);
    das::addExtern<DAS_CALL_METHOD(method_UndoSystem_cancel)>(*this, lib, "cancel", das::SideEffects::modifyArgument,
      DAS_CALL_MEMBER_CPP(UndoSystem::cancel))
      ->args({"undo_system"});

    using method_UndoSystem_is_holding = DAS_CALL_MEMBER(UndoSystem::is_holding);
    das::addExtern<DAS_CALL_METHOD(method_UndoSystem_is_holding)>(*this, lib, "is_holding", das::SideEffects::none,
      DAS_CALL_MEMBER_CPP(UndoSystem::is_holding))
      ->args({"undo_system"});

    using method_UndoSystem_can_undo = DAS_CALL_MEMBER(UndoSystem::can_undo);
    das::addExtern<DAS_CALL_METHOD(method_UndoSystem_can_undo)>(*this, lib, "can_undo", das::SideEffects::none,
      DAS_CALL_MEMBER_CPP(UndoSystem::can_undo))
      ->args({"undo_system"});

    using method_UndoSystem_can_redo = DAS_CALL_MEMBER(UndoSystem::can_redo);
    das::addExtern<DAS_CALL_METHOD(method_UndoSystem_can_redo)>(*this, lib, "can_redo", das::SideEffects::none,
      DAS_CALL_MEMBER_CPP(UndoSystem::can_redo))
      ->args({"undo_system"});

    using method_UndoSystem_clear = DAS_CALL_MEMBER(UndoSystem::clear);
    das::addExtern<DAS_CALL_METHOD(method_UndoSystem_clear)>(*this, lib, "clear", das::SideEffects::modifyArgument,
      DAS_CALL_MEMBER_CPP(UndoSystem::clear))
      ->args({"undo_system"});

    using method_UndoSystem_get_undo_name = DAS_CALL_MEMBER(UndoSystem::get_undo_name);
    das::addExtern<DAS_CALL_METHOD(method_UndoSystem_get_undo_name)>(*this, lib, "get_undo_name", das::SideEffects::worstDefault,
      DAS_CALL_MEMBER_CPP(UndoSystem::get_undo_name))
      ->args({"undo_system", "i"});

    das::addConstant(*this, "CM_OBJED_MODE_SELECT", (int)CM_OBJED_MODE_SELECT);
    das::addConstant(*this, "CM_OBJED_MODE_MOVE", (int)CM_OBJED_MODE_MOVE);
    das::addConstant(*this, "CM_OBJED_MODE_SURF_MOVE", (int)CM_OBJED_MODE_SURF_MOVE);
    das::addConstant(*this, "CM_OBJED_MODE_ROTATE", (int)CM_OBJED_MODE_ROTATE);
    das::addConstant(*this, "CM_OBJED_MODE_SCALE", (int)CM_OBJED_MODE_SCALE);
    das::addConstant(*this, "CM_OBJED_DROP", (int)CM_OBJED_DROP);
    das::addConstant(*this, "CM_OBJED_DELETE", (int)CM_OBJED_DELETE);
    das::addConstant(*this, "CM_OBJED_MODE_CREATE_ENTITY", (int)CM_OBJED_MODE_CREATE_ENTITY);
    das::addConstant(*this, "CM_OBJED_MODE_POINT_ACTION", (int)CM_OBJED_MODE_POINT_ACTION);

    verifyAotReady();
  }
  virtual das::ModuleAotType aotRequire(das::TextWriter &tw) const override
  {
    tw << "#include <dasModules/aotDagorEditor.h>\n";
    return das::ModuleAotType::cpp;
  }
};
} // namespace bind_dascript

REGISTER_MODULE_IN_NAMESPACE(DagorEditorModule, bind_dascript);
