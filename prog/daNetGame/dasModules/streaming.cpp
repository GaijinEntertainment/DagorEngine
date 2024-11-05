// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include <daScript/daScript.h>
#include <streaming/streaming.h>

namespace bind_dascript
{

class StreamingModule final : public das::Module
{
public:
  StreamingModule() : das::Module("streaming")
  {
    das::ModuleLibrary lib(this);
    lib.addBuiltInModule();
    das::addExtern<DAS_BIND_FUN(streaming::is_streaming_now)>(*this, lib, "is_streaming_now", das::SideEffects::accessExternal,
      "::streaming::is_streaming_now");
    verifyAotReady();
  }

  virtual das::ModuleAotType aotRequire(das::TextWriter &tw) const override
  {
    tw << "#include <streaming/streaming.h>\n";
    return das::ModuleAotType::cpp;
  }
};

} // namespace bind_dascript

REGISTER_MODULE_IN_NAMESPACE(StreamingModule, bind_dascript)
