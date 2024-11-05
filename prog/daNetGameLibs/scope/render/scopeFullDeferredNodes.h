// Copyright (C) Gaijin Games KFT.  All rights reserved.
#pragma once

#include <render/daBfg/nodeHandle.h>

dabfg::NodeHandle makeScopeOpaqueNode();
dabfg::NodeHandle makeScopePrepassNode();
dabfg::NodeHandle makeScopeLensMaskNode();
dabfg::NodeHandle makeScopeVrsMaskNode();
dabfg::NodeHandle makeScopeCutDepthNode();
dabfg::NodeHandle makeScopeTargetRenameNode();

dabfg::NodeHandle makeRenderLensFrameNode();
dabfg::NodeHandle makeRenderLensOpticsNode();
dabfg::NodeHandle makeRenderCrosshairNode();

dabfg::NodeHandle makeAimDofPrepareNode();
dabfg::NodeHandle makeAimDofRestoreNode();