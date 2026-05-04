// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include <daScript/daScript.h>
#include <dasModules/aotPathFinder.h>

namespace bind_dascript
{

class RebuildNavMeshModule final : public das::Module
{
public:
  RebuildNavMeshModule() : das::Module("rebuildNavMesh")
  {
    das::ModuleLibrary lib(this);

    das::addExtern<DAS_BIND_FUN(pathfinder::rebuildNavMesh_init)>(*this, lib, "rebuildNavMesh_init", das::SideEffects::modifyExternal,
      "pathfinder::rebuildNavMesh_init");

    das::addExtern<void (*)(const char *, float), &pathfinder::rebuildNavMesh_setup>(*this, lib, "rebuildNavMesh_setup",
      das::SideEffects::modifyExternal, "pathfinder::rebuildNavMesh_setup");

    das::addExtern<void (*)(const char *, const Point2 &), &pathfinder::rebuildNavMesh_setup>(*this, lib, "rebuildNavMesh_setup",
      das::SideEffects::modifyExternal, "pathfinder::rebuildNavMesh_setup");

    das::addExtern<DAS_BIND_FUN(pathfinder::rebuildNavMesh_addBBox)>(*this, lib, "rebuildNavMesh_addBBox",
      das::SideEffects::modifyExternal, "pathfinder::rebuildNavMesh_addBBox");

    das::addExtern<DAS_BIND_FUN(pathfinder::rebuildNavMesh_update)>(*this, lib, "rebuildNavMesh_update",
      das::SideEffects::modifyExternal, "pathfinder::rebuildNavMesh_update");

    das::addExtern<DAS_BIND_FUN(pathfinder::rebuildNavMesh_getProgress)>(*this, lib, "rebuildNavMesh_getProgress",
      das::SideEffects::modifyExternal, "pathfinder::rebuildNavMesh_getProgress");

    das::addExtern<DAS_BIND_FUN(pathfinder::rebuildNavMesh_getTotalTiles)>(*this, lib, "rebuildNavMesh_getTotalTiles",
      das::SideEffects::modifyExternal, "pathfinder::rebuildNavMesh_getTotalTiles");

    das::addExtern<DAS_BIND_FUN(pathfinder::rebuildNavMesh_saveToFile)>(*this, lib, "rebuildNavMesh_saveToFile",
      das::SideEffects::modifyExternal, "pathfinder::rebuildNavMesh_saveToFile");

    das::addExtern<DAS_BIND_FUN(pathfinder::rebuildNavMesh_close)>(*this, lib, "rebuildNavMesh_close",
      das::SideEffects::modifyExternal, "pathfinder::rebuildNavMesh_close");

    verifyAotReady();
  }
  virtual das::ModuleAotType aotRequire(das::TextWriter &tw) const override
  {
    tw << "#include <dasModules/aotPathFinder.h>\n";
    return das::ModuleAotType::cpp;
  }
};

} // namespace bind_dascript
REGISTER_MODULE_IN_NAMESPACE(RebuildNavMeshModule, bind_dascript);
