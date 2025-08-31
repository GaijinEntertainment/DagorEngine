// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include <daScript/daScript.h>
#include <dasModules/aotCompression.h>
#include <dasModules/aotBitStream.h>

namespace bind_dascript
{
class CompressionModule final : public das::Module
{
public:
  CompressionModule() : das::Module("CompressionUtils")
  {
    das::ModuleLibrary lib(this);

    addBuiltinDependency(lib, require("BitStream"));

    das::addExtern<DAS_BIND_FUN(das_compress_data)>(*this, lib, "compress_data", das::SideEffects::modifyArgumentAndExternal,
      "::bind_dascript::das_compress_data");
    das::addExtern<DAS_BIND_FUN(das_decompress_data)>(*this, lib, "decompress_data", das::SideEffects::modifyArgumentAndExternal,
      "::bind_dascript::das_decompress_data");

    verifyAotReady();
  }
  virtual das::ModuleAotType aotRequire(das::TextWriter &tw) const override
  {
    tw << "#include <dasModules/aotCompression.h>\n";
    tw << "#include <dasModules/aotBitStream.h>\n";
    return das::ModuleAotType::cpp;
  }
};
} // namespace bind_dascript
REGISTER_MODULE_IN_NAMESPACE(CompressionModule, bind_dascript);
