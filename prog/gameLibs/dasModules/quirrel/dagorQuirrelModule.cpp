// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include <dasModules/aotDagorQuirrel.h>

namespace bind_dascript
{

class DagorQuirrelModule final : public das::Module
{
  das::ModuleLibrary lib;
  bool initialized = false;

public:
  DagorQuirrelModule() : das::Module("DagorQuirrel") {}

  bool initDependencies() override
  {
    if (initialized)
      return true;
    initialized = true;

    lib.addModule(this);
    lib.addBuiltInModule();
    auto quirrelModule = Module::require("quirrel");
    if (!quirrelModule || !quirrelModule->initDependencies())
      return false;
    addBuiltinDependency(lib, require("ecs"));
    addBuiltinDependency(lib, require("DagorMath"));
    addBuiltinDependency(lib, quirrelModule);

#define BIND_TYPE(C, CN)                                                                                                             \
  das::addExtern<DAS_BIND_FUN(bind_dascript::PushInstanceCopy<C>)>(*this, lib, "PushInstanceCopy", das::SideEffects::accessExternal, \
    "bind_dascript::PushInstanceCopy< " CN " >");                                                                                    \
  das::addExtern<DAS_BIND_FUN(bind_dascript::GetInstance<C>)>(*this, lib, "GetInstance", das::SideEffects::accessExternal,           \
    "bind_dascript::GetInstance< " CN " >");

#define TYPE(C) BIND_TYPE(C, "::bind_dascript::" #C)
    ECS_LIST_TYPE_LIST
#undef TYPE
#define TYPE(C) BIND_TYPE(C, #C)
    TYPE(TMatrix)
    TYPE(Quat)
    TYPE(BSphere3)
    TYPE(BBox3)
    TYPE(E3DCOLOR)
    TYPE(Color3)
    TYPE(Color4)
    TYPE(Point2)
    TYPE(Point3)
    TYPE(Point4)
    TYPE(IPoint2)
    TYPE(IPoint3)
    TYPE(IPoint4)
#undef TYPE
#undef BIND_TYPE

    verifyAotReady();
    return true;
  }
  virtual das::ModuleAotType aotRequire(das::TextWriter &tw) const override
  {
    tw << "#include <dasModules/aotDagorQuirrel.h>\n";
    return das::ModuleAotType::cpp;
  }
};
} // namespace bind_dascript

REGISTER_MODULE_IN_NAMESPACE(DagorQuirrelModule, bind_dascript);
