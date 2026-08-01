// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include <de3_hmapDebugShadingService.h>

#include "hmapDebugShading.h"
#include "hmapDebugShadingProperties.h"

#include <EASTL/unique_ptr.h>

namespace
{

class HmapDebugShadingServiceImplementation : public IHmapDebugShadingService
{
public:
  void loadGradientsFile(const char *path) override { debugShadingProperties.loadGradientsFile(path); }

  void saveGradientsFile(const char *path) override { debugShadingProperties.saveGradientsFile(path); }

  void loadSettings(const DataBlock &blk) override { debugShadingProperties.loadSettings(blk); }

  void saveSettings(DataBlock &blk) const override { debugShadingProperties.saveSettings(blk); }

  void fillPropertyPanel(PropPanel::ContainerPropertyControl &panel, int pid_start) override
  {
    debugShadingProperties.fillPropertyPanel(panel, pid_start);
  }

  void onChange(int pcb_id, PropPanel::ContainerPropertyControl &panel, int pid_start) override
  {
    if (debugShadingProperties.onPropertyPanelChange(pcb_id, panel, pid_start))
      debugShading.update(getGradientToRender());
  }

  void onClick(int pcb_id, PropPanel::ContainerPropertyControl &panel, int pid_start) override
  {
    if (debugShadingProperties.onPropertyPanelClick(pcb_id, panel, pid_start))
      debugShading.update(getGradientToRender());
  }

  const HeightmapColorGradient *getSelectedGradient() const override { return debugShadingProperties.getSelectedGradient(); }

  bool isShowingGradientColors() const override { return getGradientToRender() != nullptr; }

  const HeightmapColorGradient *getGradientToRender() const
  {
    if (!debugShadingProperties.isShowingGradientColors())
      return nullptr;

    const HeightmapColorGradient *gradient = getSelectedGradient();
    if (!gradient || gradient->keys.empty())
      return nullptr;

    return gradient;
  }

  HeightmapDebugShading debugShading;
  HeightmapDebugShadingProperties debugShadingProperties;
};

eastl::unique_ptr<HmapDebugShadingServiceImplementation> srv;

} // namespace

void init_hmap_debug_shading_service()
{
  G_ASSERT(!srv.get());
  srv.reset(new HmapDebugShadingServiceImplementation());
}

void release_hmap_debug_shading_service() { srv.reset(); }

void *get_hmap_debug_shading_service() { return srv.get(); }
