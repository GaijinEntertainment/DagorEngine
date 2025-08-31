// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include <userSystemInfo/systemInfo.h>
#include <dasModules/aotSystemInfo.h>

MAKE_TYPE_FACTORY(VideoInfo, VideoInfo);
MAKE_TYPE_FACTORY(CpuInfo, CpuInfo);

struct VideoInfoAnnotation : public das::ManagedStructureAnnotation<VideoInfo, false>
{
  VideoInfoAnnotation(das::ModuleLibrary &ml) : ManagedStructureAnnotation("VideoInfo", ml)
  {
    addField<DAS_BIND_MANAGED_FIELD(desktopResolution)>("desktopResolution");
    addField<DAS_BIND_MANAGED_FIELD(videoCard)>("videoCard");
    addField<DAS_BIND_MANAGED_FIELD(videoVendor)>("videoVendor");
  }
};

struct CpuInfoAnnotation : public das::ManagedStructureAnnotation<CpuInfo, false>
{
  CpuInfoAnnotation(das::ModuleLibrary &ml) : ManagedStructureAnnotation("CpuInfo", ml)
  {
    addField<DAS_BIND_MANAGED_FIELD(cpu)>("cpu");
    addField<DAS_BIND_MANAGED_FIELD(cpuFreq)>("cpuFreq");
    addField<DAS_BIND_MANAGED_FIELD(cpuVendor)>("cpuVendor");
    addField<DAS_BIND_MANAGED_FIELD(cpuSeriesCores)>("cpuSeriesCores");
    addField<DAS_BIND_MANAGED_FIELD(numCores)>("numCores");
  }
};

namespace bind_dascript
{
void get_video_info(const das::TBlock<void, const das::TTemporary<const VideoInfo>> &block, das::Context *context,
  das::LineInfoArg *at)
{
  String desktopResolution, videoCard, videoVendor;
  systeminfo::get_video_info(desktopResolution, videoCard, videoVendor);
  VideoInfo result = {desktopResolution.c_str(), videoCard.c_str(), videoVendor.c_str()};
  vec4f arg = das::cast<VideoInfo &>::from(result);
  context->invoke(block, &arg, nullptr, at);
}

void get_cpu_info(const das::TBlock<void, const das::TTemporary<const CpuInfo>> &block, das::Context *context, das::LineInfoArg *at)
{
  String cpu, cpuFreq, cpuVendor, cpuSeriesCores;
  int numCores;
  systeminfo::get_cpu_info(cpu, cpuFreq, cpuVendor, cpuSeriesCores, numCores);
  CpuInfo result = {cpu.c_str(), cpuFreq.c_str(), cpuVendor.c_str(), cpuSeriesCores.c_str(), numCores};
  vec4f arg = das::cast<CpuInfo &>::from(result);
  context->invoke(block, &arg, nullptr, at);
}

class SystemInfoModule final : public das::Module
{
public:
  SystemInfoModule() : das::Module("SystemInfo")
  {
    das::ModuleLibrary lib(this);
    addAnnotation(das::make_smart<VideoInfoAnnotation>(lib));
    addAnnotation(das::make_smart<CpuInfoAnnotation>(lib));

    das::addExtern<DAS_BIND_FUN(get_video_info)>(*this, lib, "get_video_info", das::SideEffects::accessExternal,
      "bind_dascript::get_video_info");
    das::addExtern<DAS_BIND_FUN(get_cpu_info)>(*this, lib, "get_cpu_info", das::SideEffects::accessExternal,
      "bind_dascript::get_cpu_info");

    verifyAotReady();
  }
  virtual das::ModuleAotType aotRequire(das::TextWriter &tw) const override
  {
    tw << "#include \"dasModules/aotSystemInfo.h\"\n";
    return das::ModuleAotType::cpp;
  }
};
} // namespace bind_dascript

REGISTER_MODULE_IN_NAMESPACE(SystemInfoModule, bind_dascript);
