// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include <dasModules/aotDagorBase64.h>

namespace bind_dascript
{

class DagorBase64 final : public das::Module
{
public:
  DagorBase64() : das::Module("DagorBase64")
  {
    das::ModuleLibrary lib(this);
    das::addExtern<DAS_BIND_FUN(encode_base64)>(*this, lib, "encode_base64", das::SideEffects::none, "bind_dascript::encode_base64");
    das::addExtern<DAS_BIND_FUN(decode_base64)>(*this, lib, "decode_base64", das::SideEffects::none, "bind_dascript::decode_base64");
    verifyAotReady();
  }
  virtual das::ModuleAotType aotRequire(das::TextWriter &tw) const override
  {
    tw << "#include <dasModules/aotDagorBase64.h>\n";
    return das::ModuleAotType::cpp;
  }
};

} // namespace bind_dascript

REGISTER_MODULE_IN_NAMESPACE(DagorBase64, bind_dascript);