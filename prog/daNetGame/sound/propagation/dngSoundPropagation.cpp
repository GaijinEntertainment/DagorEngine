// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include "sound/propagation/dngSoundPropagation.h"
#include <gameLibs/soundPropagationHost/dacoll/occlusionBackend_api.h>
#include <soundSystem/occlusionGPU.h>
#include <ioSys/dag_dataBlock.h>
#include <debug/dag_log.h>
#include <gui/dag_imgui.h>
#include <imgui/imgui.h>

namespace dngsound
{
void install_propagation(const DataBlock &sndblk)
{
  if (!sndblk.getBool("occlusionCPUInit", false))
    return;
  snd_propagation_host::BackendParams params;
  params.materialsBlkPath = sndblk.getStr("soundMaterialsBlk", "config/sound_materials.blk");
  // not debug_occlusion_gpu: it is on by default, so the library's views would always draw
  params.config.debugDrawCvar = "snd.debug";
  if (!snd_propagation_host::install(params))
    logerr("[dngSound] sound propagation did not start, the occlusion the sound settings select stays");
}

void uninstall_propagation() { snd_propagation_host::uninstall(); }
} // namespace dngsound

#if DAGOR_DBGLEVEL > 0
// a window of its own: the library knows nothing of dacoll or phys materials
static void sound_propagation_host_imgui()
{
  if (!snd_propagation::is_inited())
  {
    ImGui::Text("the propagation backend is off: occlusionCPUInit is not set, or it did not start (see the log)");
    return;
  }
  const snd_propagation_host::Counters &c = snd_propagation_host::counters();
  ImGui::Text("bundles %llu, segments %llu, %.1f us per segment", (unsigned long long)c.bundleCount(),
    (unsigned long long)c.segmentCount(), c.usecPerSegment());
  ImGui::Text("hits %llu, default material %.1f %%", (unsigned long long)c.hitCount(), c.defaultMaterialFraction() * 100.f);
  ImGui::Text("segments that lost hits to a cap %.2f %%", c.cappedSegmentFraction() * 100.f);
  if (ImGui::Button("reset"))
    snd_propagation_host::reset_counters();
}
REGISTER_IMGUI_WINDOW("Sound", "Sound propagation host", sound_propagation_host_imgui);

// through the blob layer's lock: the library's update writes this state on the sound thread
static void sound_propagation_imgui()
{
  if (!snd_propagation::is_inited())
  {
    ImGui::Text("the propagation backend is off: occlusionCPUInit is not set, or it did not start (see the log)");
    return;
  }
  sndsys::occlusion_gpu::debug_render_imgui();
}
REGISTER_IMGUI_WINDOW("Sound", "Sound propagation", sound_propagation_imgui);
#endif
