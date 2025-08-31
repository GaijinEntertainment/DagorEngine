// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include <fstream>
#include <osApiWrappers/dag_direct.h>
#include <perfMon/dag_cpuFreq.h>
#include <daScript/daScript.h>
#include <daScript/daScriptModule.h>
#include <ecs/scripts/dasEs.h>
#include <dasModules/dasFsFileAccess.h>

void foo() { dd_get_fname(""); } //== pull in directoryService.obj

void require_project_specific_modules()
{
  if (!das::Module::require("DagorMath"))
  {
    NEED_MODULE(DagorMath);
  }
  if (!das::Module::require("DagorDataBlock"))
  {
    NEED_MODULE(DagorDataBlock);
  }
  if (!das::Module::require("DagorSystem"))
  {
    NEED_MODULE(DagorSystem);
  }
  if (!das::Module::require("ecs"))
  {
    NEED_MODULE(ECS);
  }

  NEED_MODULE(DagorTexture3DModule)
  NEED_MODULE(DagorResPtr)
  NEED_MODULE(DagorDriver3DModule)
  NEED_MODULE(DagorShaders)
  NEED_MODULE(PriorityShadervarModule)
  NEED_MODULE(DagorStdGuiRender)
  NEED_MODULE(DagorTime)
  NEED_MODULE(DagorBase64)
  NEED_MODULE(DagorMathUtils)
  NEED_MODULE(GameMathModule)
  NEED_MODULE(DagorConsole)
  NEED_MODULE(Module_dasQUIRREL)
  NEED_MODULE(DagorQuirrelModule)
  NEED_MODULE(DagorInputModule)
  NEED_MODULE(DagorRandom)
  NEED_MODULE(DagorFiles)
  NEED_MODULE(DagorFindFiles)
  NEED_MODULE(DagorResources)
  NEED_MODULE(DagorEditorModule)
  NEED_MODULE(GridModule)
  NEED_MODULE(Module_JsonWriter)
  NEED_MODULE(Module_RapidJson)
  NEED_MODULE(JsonUtilsModule)
  NEED_MODULE(RegExpModule)
  NEED_MODULE(DngAppModule)
  NEED_MODULE(DngLevelModule)
  NEED_MODULE(DngGameObjectModule)
  NEED_MODULE(FxModule)
  NEED_MODULE(SceneModule)
  NEED_MODULE(WorldRendererModule)
  NEED_MODULE(GamePhysModule)
  NEED_MODULE(GeomNodeTreeModule)
  NEED_MODULE(PhysDeclModule)
  NEED_MODULE(AnimV20)
  NEED_MODULE(CollRes)
  NEED_MODULE(RendInstModule)
  NEED_MODULE(DacollModule)
  NEED_MODULE(ProjectiveDecalsModule)
  NEED_MODULE(BitStreamModule)
  NEED_MODULE(NetModule)
  NEED_MODULE(DngNetModule)
  NEED_MODULE(ClientNetModule)
  NEED_MODULE(DngNetPhysModule)
  NEED_MODULE(SoundEventModule)
  NEED_MODULE(SoundStreamModule)
  NEED_MODULE(SoundSystemModule)
  NEED_MODULE(SoundHashModule)
  NEED_MODULE(SoundPropsModule)
  NEED_MODULE(SoundNetPropsModule)
  NEED_MODULE(ZonesModule)
  NEED_MODULE(GpuReadbackQueryModule)
  NEED_MODULE(HeightmapQueryManagerModule)
  NEED_MODULE(PortalRendererModule)
  NEED_MODULE(RenderLibsAllowedModule)
  NEED_MODULE(LandMeshModule)
  NEED_MODULE(CollisionTracesModule)
  NEED_MODULE(GridCollisionModule)
  NEED_MODULE(DngActorModule)
  NEED_MODULE(PlayerModule)
  NEED_MODULE(GameTimersModule)
  NEED_MODULE(LagCatcherModule)
  NEED_MODULE(DaSkiesModule)
  NEED_MODULE(PhysVarsModule)
  NEED_MODULE(StreamingModule)
  NEED_MODULE(ECSGlobalTagsModule)
  NEED_MODULE(CurrentCircuitModule)
  NEED_MODULE(DagorDebug3DModule)
  NEED_MODULE(DagorDebug3DSolidModule)
  NEED_MODULE(DngPhysModule)
  NEED_MODULE(ActionModule)
  NEED_MODULE(DaProfilerModule)
  NEED_MODULE(EcsUtilsModule)
  NEED_MODULE(StatsdModule)
  NEED_MODULE(AnimatedPhysModule)
  NEED_MODULE(SmokeOccluderModule)
  NEED_MODULE(RiDestrModule)
  NEED_MODULE(RendInstPhysModule)
  NEED_MODULE(DngUIModule)
  NEED_MODULE(ModuleDarg);
  NEED_MODULE(DngCameraModule)
  NEED_MODULE(PhysMatModule)
  NEED_MODULE(CapsuleApproximationModule)
  NEED_MODULE(EffectorDataModule)
  NEED_MODULE(TouchInputModule)
  NEED_MODULE(DaphysModule)
  NEED_MODULE(CameraShakerModule)
  NEED_MODULE(DngCameraShakerModule)
  NEED_MODULE(WebSocketModule)
  NEED_MODULE(Module_dasIMGUI)
  NEED_MODULE(Module_dasIMGUI_NODE_EDITOR)
  NEED_MODULE(DagorImguiModule)
  NEED_MODULE(ForceFeedbackRumbleModule)
  NEED_MODULE(PropsRegistryModule)
  NEED_MODULE(NetPropsRegistryModule)
  NEED_MODULE(NetstatModule)
  NEED_MODULE(ClipmapDecalsModule)
  NEED_MODULE(ReplayModule)
  NEED_MODULE(VideoPlayer)
  NEED_MODULE(DagorMaterials)
  NEED_MODULE(DngDacollModule)
  NEED_MODULE(DeferToActModule)
  NEED_MODULE(DaFgCoreModule)
  NEED_MODULE(ResourceSlotCoreModule)
  NEED_MODULE(SystemInfoModule)
  NEED_MODULE(AuthModule)
  NEED_MODULE(MatchingModule)
  NEED_MODULE(DngMatchingModule)
  NEED_MODULE(CompressionModule)
  das::pull_all_auto_registered_modules();
  extern void pull_game_das();
  pull_game_das();
}

void require_project_specific_debugger_modules() {}
