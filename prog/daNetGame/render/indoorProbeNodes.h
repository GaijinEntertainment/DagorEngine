// Copyright (C) Gaijin Games KFT.  All rights reserved.
#pragma once

#include <render/indoorProbeManager.h>

class IndoorProbeNodes : public IIndoorProbeNodes
{
public:
  virtual void registerNodes(uint32_t allProbesOnLevel, IndoorProbeManager *owner) override;
  virtual void clearNodes() override
  {
    tryStartUpdateNode = {};
    cpuCheckNode = {};
    completePreviousFrameReadbacksNode = {};
    createVisiblePixelsCountNode = {};
    calculateVisiblePixelsCountNode = {};
    initiateVisiblePixelsCountReadbackNode = {};
    setVisibleBoxesDataNode = {};
  }

private:
  dafg::NodeHandle tryStartUpdateNode;
  dafg::NodeHandle cpuCheckNode;
  dafg::NodeHandle completePreviousFrameReadbacksNode;
  dafg::NodeHandle createVisiblePixelsCountNode;
  dafg::NodeHandle calculateVisiblePixelsCountNode;
  dafg::NodeHandle initiateVisiblePixelsCountReadbackNode;
  dafg::NodeHandle setVisibleBoxesDataNode;
};