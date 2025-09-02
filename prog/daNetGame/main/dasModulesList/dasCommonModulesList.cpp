// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include <daScript/misc/platform.h>
#include <daScript/daScriptModule.h>
#include <daScript/ast/ast.h>

extern void pull_render_das();
extern void pull_input_das();
extern void pull_client_das();
extern void pull_dedicated_das();
extern void pull_imgui_das();
extern void pull_ui_das();
extern void pull_editor_das();
extern void pull_sound_das();
extern void pull_sound_net_das();
extern void pull_game_das();

void pull_das()
{
#if DAGOR_DBGLEVEL > 0 && _TARGET_PC
  // for web debugger
  NEED_MODULE(WebSocketModule)
#endif
#if DAGOR_DBGLEVEL > 0
  NEED_MODULE(Module_Debugger)
#endif
  NEED_MODULE(DagorTime)
  NEED_MODULE(DagorBase64)
  NEED_MODULE(DagorRandom)
  NEED_MODULE(DagorMathUtils)
  NEED_MODULE(GameMathModule)
  NEED_MODULE(DagorConsole)
  NEED_MODULE(Module_dasQUIRREL)
  NEED_MODULE(DagorQuirrelModule)
  NEED_MODULE(DagorFiles)
  NEED_MODULE(DagorFindFiles)
  NEED_MODULE(DagorResources)
  NEED_MODULE(GridModule)
  NEED_MODULE(Module_JsonWriter)
  NEED_MODULE(Module_RapidJson)
  NEED_MODULE(JsonUtilsModule)
  NEED_MODULE(DngAppModule)
  NEED_MODULE(DngLevelModule)
  NEED_MODULE(DngGameObjectModule)
  NEED_MODULE(GamePhysModule)
  NEED_MODULE(GeomNodeTreeModule)
  NEED_MODULE(PhysDeclModule)
  NEED_MODULE(AnimV20)
  NEED_MODULE(CollRes)
  NEED_MODULE(RendInstModule)
  NEED_MODULE(DacollModule)
  NEED_MODULE(BitStreamModule)
  NEED_MODULE(NetModule)
  NEED_MODULE(DngNetModule)
  NEED_MODULE(DngNetPhysModule)
  NEED_MODULE(ZonesModule)
  NEED_MODULE(GpuReadbackQueryModule)
  NEED_MODULE(LandMeshModule)
  NEED_MODULE(CollisionTracesModule)
  NEED_MODULE(GridCollisionModule)
  NEED_MODULE(DngActorModule)
  NEED_MODULE(PlayerModule)
  NEED_MODULE(GameTimersModule)
  NEED_MODULE(LagCatcherModule)
  NEED_MODULE(PhysVarsModule)
  NEED_MODULE(StreamingModule)
  NEED_MODULE(ECSGlobalTagsModule)
  NEED_MODULE(CurrentCircuitModule)
  NEED_MODULE(DagorDebug3DModule)
  NEED_MODULE(DagorDebug3DSolidModule)
  NEED_MODULE(DngPhysModule)
  NEED_MODULE(ActionModule)
  NEED_MODULE(DaProfilerModule)
  NEED_MODULE(RegExpModule)
  NEED_MODULE(StatsdModule)
  NEED_MODULE(AnimatedPhysModule)
  NEED_MODULE(RiDestrModule)
  NEED_MODULE(RendInstPhysModule)
  NEED_MODULE(PhysMatModule)
  NEED_MODULE(SmokeOccluderModule)
  NEED_MODULE(CapsuleApproximationModule)
  NEED_MODULE(EffectorDataModule)
  NEED_MODULE(DaphysModule)
  NEED_MODULE(EcsUtilsModule)
  NEED_MODULE(MatchingModule)
  NEED_MODULE(PropsRegistryModule)
  NEED_MODULE(NetPropsRegistryModule)
  NEED_MODULE(NetstatModule)
  NEED_MODULE(ReplayModule)
  NEED_MODULE(DngDacollModule)
  NEED_MODULE(DeferToActModule)
  NEED_MODULE(CompressionModule)

  pull_render_das();
  pull_input_das();
  pull_client_das();
  pull_ui_das();
  pull_dedicated_das();
  pull_imgui_das();
  pull_editor_das();
  pull_sound_das();
  pull_sound_net_das();
  das::pull_all_auto_registered_modules();
  pull_game_das();
  das::Module::Initialize();
}

void require_project_specific_debugger_modules()
{
#if DAGOR_DBGLEVEL > 0 && _TARGET_PC
  NEED_MODULE(DngAppModule);
  NEED_MODULE(Module_Debugger);
  NEED_MODULE(Module_FIO)
  NEED_MODULE(Module_Network)
  NEED_MODULE(Module_JobQue)
  NEED_MODULE(Module_UriParser)
#endif
} //