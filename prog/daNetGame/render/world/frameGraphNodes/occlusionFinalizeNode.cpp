// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include <render/daFrameGraph/daFG.h>
#include <render/world/cameraViewVisibilityManager.h>
#include <scene/dag_occlusion.h>
#include <3d/dag_stereoIndex.h>
#include <util/dag_console.h>
#include <util/dag_convar.h>
#include "frameGraphNodes.h"


CONSOLE_BOOL_VAL("occlusion", show_occlusion_hzb, false);
CONSOLE_BOOL_VAL("occlusion", show_occludees, false);

// Defined in worldRenderer.cpp
extern ConVarT<bool, false> debug_occlusion;
extern ConVarT<bool, false> stop_occlusion;

dafg::NodeHandle makeOcclusionFinalizeNode()
{
  return dafg::register_node("occlusion_finalize_node", DAFG_PP_NODE_SRC, [](dafg::Registry registry) {
    registry.orderMeAfter("water_early_after_envi_node");
    registry.orderMeBefore("water_late_node");
    auto cameraHndl = registry.readBlob<CameraParams>("current_camera").handle();
    return [cameraHndl] {
      Occlusion *occlusion = cameraHndl.ref().jobsMgr->getOcclusion();
      if (!occlusion)
        return;

      if (debug_occlusion.pullValueChange() && debug_occlusion.get())
      {
        if (!show_occlusion_hzb.get())
          console::print("useful commands: 'occlusion.show_occlusion_hzb'");
      }

      if (show_occlusion_hzb.pullValueChange() && show_occlusion_hzb.get())
      {
        console::print("new texture 'hzb_reprojected' became available for 'render.show_tex'");
        if (!debug_occlusion.get())
          console::print("missing 'occlusion.debug_occlusion'");
      }

      if (show_occlusion_hzb.get())
        occlusion->prepareDebug();
      occlusion->setStoreOccludees(show_occludees.get() && debug_occlusion.get() && !stop_occlusion.get());
    };
  });
}
