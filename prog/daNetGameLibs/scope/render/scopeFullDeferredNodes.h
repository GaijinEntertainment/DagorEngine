// Copyright (C) Gaijin Games KFT.  All rights reserved.
#pragma once

#include <render/daFrameGraph/nodeHandle.h>

dafg::NodeHandle makeScopeOpaqueNode();
dafg::NodeHandle makeScopeTransNode();
dafg::NodeHandle makeScopePrepassNode();
dafg::NodeHandle makeScopeLensMaskNode();
dafg::NodeHandle makeScopeHZBMask();
dafg::NodeHandle makeScopeVrsMaskNode();
dafg::NodeHandle makeScopeCutDepthNode();
dafg::NodeHandle makeScopeDownsampleStencilNode(const char *node_name, const char *depth_name, const char *depth_rename_to);
dafg::NodeHandle makeScopeTargetRenameNode();

dafg::NodeHandle makeRenderOpticsPrepassNode();
dafg::NodeHandle makeRenderLensFrameNode();
dafg::NodeHandle makeRenderLensOpticsNode();
dafg::NodeHandle makeRenderCrosshairNode();

dafg::NodeHandle makeAimDofPrepareNode();
dafg::NodeHandle makeAimDofRestoreNode();