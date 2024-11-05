// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include <dasModules/dasModulesCommon.h>
#include <matching/types.h>

namespace bind_dascript
{

class MatchingModule final : public das::Module
{
public:
  MatchingModule() : das::Module("matching") // bad module name, needs refactoring
  {
    das::ModuleLibrary lib(this);
    das::addConstant(*this, "INVALID_ROOM_ID", matching::INVALID_ROOM_ID);
    das::addConstant(*this, "INVALID_SESSION_ID", matching::INVALID_SESSION_ID);
    das::addConstant(*this, "INVALID_MEMBER_ID", matching::INVALID_MEMBER_ID);
    das::addConstant(*this, "INVALID_USER_ID", matching::INVALID_USER_ID);
    das::addConstant(*this, "INVALID_SQUAD_ID", matching::INVALID_SQUAD_ID);
    das::addConstant(*this, "INVALID_GROUP_ID", matching::INVALID_GROUP_ID);
    verifyAotReady();
  }
  das::ModuleAotType aotRequire(das::TextWriter &tw) const override
  {
    tw << "#include <dasModules/dasModulesCommon.h>\n";
    tw << "#include <matching/types.h>\n";
    return das::ModuleAotType::cpp;
  }
};

} // namespace bind_dascript

REGISTER_MODULE_IN_NAMESPACE(MatchingModule, bind_dascript);
