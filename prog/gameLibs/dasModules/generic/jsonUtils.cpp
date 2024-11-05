// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include <dasModules/aotJsonUtils.h>
#include <daScriptModules/rapidjson/rapidjson.h>


DAS_BASE_BIND_ENUM(jsonutils::DecodeJwtResult, DecodeJwtResult, OK, INVALID_TOKEN, WRONG_HEADER, INVALID_TOKEN_TYPE,
  INVALID_TOKEN_SIGNATURE_ALGORITHM, INVALID_PAYLOAD_COMPRESSION, WRONG_PAYLOAD, SIGNATURE_VERIFICATION_FAILED)

DAS_BASE_BIND_ENUM(jsonutils::JwtCompressionType, JwtCompressionType, NONE, ZLIB, ZSTD)


namespace bind_dascript
{

class JsonUtilsModule final : public das::Module
{
public:
  JsonUtilsModule() : das::Module("JsonUtils")
  {
    das::ModuleLibrary lib(this);
    addBuiltinDependency(lib, require("rapidjson"));

    addEnumeration(das::make_smart<EnumerationDecodeJwtResult>());
    addEnumeration(das::make_smart<EnumerationJwtCompressionType>());

    das::addExtern<DAS_BIND_FUN(bind_dascript::load_json_from_file)>(*this, lib, "load_json_from_file", das::SideEffects::worstDefault,
      "bind_dascript::load_json_from_file");
    das::addExtern<DAS_BIND_FUN(bind_dascript::save_json_to_file)>(*this, lib, "save_json_to_file", das::SideEffects::modifyExternal,
      "bind_dascript::save_json_to_file");
    das::addExtern<DAS_BIND_FUN(jsonutils::decode_jwt_block)>(*this, lib, "decode_jwt_block", das::SideEffects::modifyArgument,
      "jsonutils::decode_jwt_block");
    das::addExtern<DAS_BIND_FUN(bind_dascript::decode_jwt)>(*this, lib, "decode_jwt", das::SideEffects::modifyArgument,
      "bind_dascript::decode_jwt");

    verifyAotReady();
  }

  das::ModuleAotType aotRequire(das::TextWriter &tw) const override
  {
    tw << "#include <dasModules/aotJsonUtils.h>\n";
    return das::ModuleAotType::cpp;
  }
};
} // namespace bind_dascript
REGISTER_MODULE_IN_NAMESPACE(JsonUtilsModule, bind_dascript);
