// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include <render/daBfg/bfg.h>
#include <scene/dag_occlusion.h>
#include <3d/dag_stereoIndex.h>
#include <util/dag_convar.h>

#include "frameGraphNodes.h"


CONSOLE_BOOL_VAL("occlusion", show_occlusion_hzb, false);
CONSOLE_BOOL_VAL("occlusion", show_occludees, false);

// Defined in worldRenderer.cpp
extern ConVarT<bool, false> debug_occlusion;
extern ConVarT<bool, false> stop_occlusion;

dabfg::NodeHandle makeOcclusionFinalizeNode()
{
  return dabfg::register_node("occlusion_finalize_node", DABFG_PP_NODE_SRC, [](dabfg::Registry registry) {
    registry.orderMeAfter("water_early_after_envi_node");
    registry.orderMeBefore("water_late_node");
    auto occlusionHndl = registry.read("current_occlusion").blob<Occlusion *>().handle();
    return [occlusionHndl] {
      Occlusion *occlusion = occlusionHndl.ref();
      if (!occlusion)
        return;
      if (show_occlusion_hzb.get())
        occlusion->prepareDebug();
      occlusion->setStoreOccludees(show_occludees.get() && debug_occlusion.get() && !stop_occlusion.get());
    };
  });
}
