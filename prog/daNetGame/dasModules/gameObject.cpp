// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include "gameObject.h"

struct GameObjectsAnnotation : das::ManagedStructureAnnotation<GameObjects, false>
{
  GameObjectsAnnotation(das::ModuleLibrary &ml) : ManagedStructureAnnotation("GameObjects", ml)
  {
    cppName = " ::GameObjects";

    addField<DAS_BIND_MANAGED_FIELD(ladders)>("ladders");
    addField<DAS_BIND_MANAGED_FIELD(indoors)>("indoors");
    addField<DAS_BIND_MANAGED_FIELD(sounds)>("sounds");
  }
};

struct TiledSceneAnnotation : das::ManagedStructureAnnotation<scene::TiledScene, false>
{
  TiledSceneAnnotation(das::ModuleLibrary &ml) : ManagedStructureAnnotation("TiledScene", ml) { cppName = " ::scene::TiledScene"; }
};

namespace bind_dascript
{
class DngGameObjectModule final : public das::Module
{
public:
  DngGameObjectModule() : das::Module("gameObject")
  {
    das::ModuleLibrary lib(this);
    addBuiltinDependency(lib, require("ecs"));
    addAnnotation(das::make_smart<TiledSceneAnnotation>(lib));
    addAnnotation(das::make_smart<GameObjectsAnnotation>(lib));
    das::addExtern<DAS_BIND_FUN(das_find_scene_game_objects)>(*this, lib, "find_scene_game_objects", das::SideEffects::accessExternal,
      "::bind_dascript::das_find_scene_game_objects");
    das::addExtern<DAS_BIND_FUN(das_for_scene_game_objects_in_box)>(*this, lib, "for_scene_game_objects",
      das::SideEffects::accessExternal, "::bind_dascript::das_for_scene_game_objects_in_box");
    das::addExtern<DAS_BIND_FUN(das_get_scene_game_objects_by_name)>(*this, lib, "get_scene_game_objects_by_name",
      das::SideEffects::accessExternal, "::bind_dascript::das_get_scene_game_objects_by_name");
    das::addExtern<DAS_BIND_FUN(das_game_objects_scene_get_node_pos)>(*this, lib, "game_objects_scene_get_node_pos",
      das::SideEffects::accessExternal, "::bind_dascript::das_game_objects_scene_get_node_pos");
    das::addExtern<DAS_BIND_FUN(das_for_scene_game_object_types)>(*this, lib, "for_scene_game_object_types",
      das::SideEffects::accessExternal, "::bind_dascript::das_for_scene_game_object_types");
    das::addExtern<DAS_BIND_FUN(das_find_ladder)>(*this, lib, "find_ladder", das::SideEffects::modifyArgumentAndAccessExternal,
      "::bind_dascript::das_find_ladder");
    das::addExtern<DAS_BIND_FUN(das_enum_sounds_in_box)>(*this, lib, "enum_sounds_in_box", das::SideEffects::accessExternal,
      "::bind_dascript::das_enum_sounds_in_box");

    das::addExtern<DAS_BIND_FUN(tiled_scene_getNodesCount)>(*this, lib, "tiled_scene_getNodesCount", das::SideEffects::none,
      "::bind_dascript::tiled_scene_getNodesCount");
    das::addExtern<DAS_BIND_FUN(tiled_scene_getNodeFromIndex)>(*this, lib, "tiled_scene_getNodeFromIndex", das::SideEffects::none,
      "::bind_dascript::tiled_scene_getNodeFromIndex");
    das::addExtern<DAS_BIND_FUN(tiled_scene_getNode), das::SimNode_ExtFuncCallRef>(*this, lib, "tiled_scene_getNode",
      das::SideEffects::none, "::bind_dascript::tiled_scene_getNode");
    das::addExtern<DAS_BIND_FUN(tiled_scene_getNodesAliveCount)>(*this, lib, "tiled_scene_getNodesAliveCount", das::SideEffects::none,
      "::bind_dascript::tiled_scene_getNodesAliveCount");
    das::addExtern<DAS_BIND_FUN(tiled_scene_iterate)>(*this, lib, "tiled_scene_iterate", das::SideEffects::accessExternal,
      "::bind_dascript::tiled_scene_iterate");
    das::addExtern<DAS_BIND_FUN(tiled_scene_boxCull)>(*this, lib, "tiled_scene_boxCull", das::SideEffects::accessExternal,
      "::bind_dascript::tiled_scene_boxCull");

    das::addExtern<DAS_BIND_FUN(create_ladders_scene)>(*this, lib, "create_ladders_scene", das::SideEffects::modifyArgument,
      "create_ladders_scene")
      ->args({"game_objects"});

    using method_TiledScene_destroy = DAS_CALL_MEMBER(scene::TiledScene::destroy);
    das::addExtern<DAS_CALL_METHOD(method_TiledScene_destroy)>(*this, lib, "destroy", das::SideEffects::modifyArgument,
      DAS_CALL_MEMBER_CPP(scene::TiledScene::destroy))
      ->args({"this", "node"});

    using method_TiledScene_allocate = DAS_CALL_MEMBER(scene::TiledScene::allocate);
    das::addExtern<DAS_CALL_METHOD(method_TiledScene_allocate)>(*this, lib, "allocate", das::SideEffects::modifyArgument,
      DAS_CALL_MEMBER_CPP(scene::TiledScene::allocate))
      ->args({"this", "transform", "pool", "flags"});

    using method_TiledScene_reallocate = DAS_CALL_MEMBER(scene::TiledScene::reallocate);
    das::addExtern<DAS_CALL_METHOD(method_TiledScene_reallocate)>(*this, lib, "reallocate", das::SideEffects::modifyArgument,
      DAS_CALL_MEMBER_CPP(scene::TiledScene::reallocate))
      ->args({"this", "node", "transform", "pool", "flags"});

    using method_TiledScene_setTransform = DAS_CALL_MEMBER(scene::TiledScene::setTransform);
    das::addExtern<DAS_CALL_METHOD(method_TiledScene_setTransform)>(*this, lib, "setTransform", das::SideEffects::modifyArgument,
      DAS_CALL_MEMBER_CPP(scene::TiledScene::setTransform))
      ->args({"this", "node", "transform", "do_not_wait"});

    using method_TiledScene_setFlags = DAS_CALL_MEMBER(scene::TiledScene::setFlags);
    das::addExtern<DAS_CALL_METHOD(method_TiledScene_setFlags)>(*this, lib, "setFlags", das::SideEffects::modifyArgument,
      DAS_CALL_MEMBER_CPP(scene::TiledScene::setFlags))
      ->args({"this", "node", "flags"});

    using method_TiledScene_unsetFlags = DAS_CALL_MEMBER(scene::TiledScene::unsetFlags);
    das::addExtern<DAS_CALL_METHOD(method_TiledScene_unsetFlags)>(*this, lib, "unsetFlags", das::SideEffects::modifyArgument,
      DAS_CALL_MEMBER_CPP(scene::TiledScene::unsetFlags))
      ->args({"this", "node", "flags"});
    verifyAotReady();
  }

  virtual das::ModuleAotType aotRequire(das::TextWriter &tw) const override
  {
    tw << "#include \"dasModules/gameObject.h\"\n";
    return das::ModuleAotType::cpp;
  }
};
} // namespace bind_dascript
REGISTER_MODULE_IN_NAMESPACE(DngGameObjectModule, bind_dascript);
