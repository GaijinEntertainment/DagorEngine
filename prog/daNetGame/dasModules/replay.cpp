// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include "replay.h"
#include <daScriptModules/rapidjson/rapidjson.h>
#include "net/replay.h"
#include "main/gameLoad.h" // for sceneload::UserGameModeContext

namespace bind_dascript
{

bool load_replay_meta_info(
  const char *path, const das::TBlock<void, const das::TTemporary<rapidjson::Document>> &blk, das::Context *ctx, das::LineInfoArg *at)
{
  return ::load_replay_meta_info(path, [&](const rapidjson::Document &doc) {
    vec4f arg = das::cast<const rapidjson::Document *>::from(&doc);
    ctx->invoke(blk, &arg, nullptr, at);
  });
}

void replay_play(const char *path, float start_time) { ::replay_play(path, start_time, {}); }

class ReplayModule final : public das::Module
{
public:
  ReplayModule() : das::Module("replay")
  {
    das::ModuleLibrary lib(this);

#define RPL_ADD_EXTERN(fn) \
  das::addExtern<DAS_BIND_FUN(bind_dascript::fn)>(*this, lib, #fn, das::SideEffects::accessExternal, "bind_dascript::" #fn)

    RPL_ADD_EXTERN(replay_get_play_file);
    RPL_ADD_EXTERN(replay_clear_current_play_file);
    RPL_ADD_EXTERN(replay_get_play_start_time);
    RPL_ADD_EXTERN(is_replay_playing);
    RPL_ADD_EXTERN(replay_play)->args({"path", "start_time"});
    das::addExtern<DAS_BIND_FUN(get_currently_playing_replay_meta_info), das::SimNode_ExtFuncCallRef>(*this, lib,
      "get_currently_playing_replay_meta_info", das::SideEffects::accessExternal, "get_currently_playing_replay_meta_info");
    RPL_ADD_EXTERN(load_replay_meta_info);

    verifyAotReady();
  }
  virtual das::ModuleAotType aotRequire(das::TextWriter &tw) const override
  {
    tw << "#include \"dasModules/replay.h\"\n";
    return das::ModuleAotType::cpp;
  }
};

} // namespace bind_dascript
REGISTER_MODULE_IN_NAMESPACE(ReplayModule, bind_dascript);
