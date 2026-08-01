// Copyright (C) Gaijin Games KFT.  All rights reserved.
#pragma once

#include <debug/visualization/structuresUser.h>
#include <frontend/internalRegistry.h>
#include <backend/intermediateRepresentation.h>

namespace dafg::visualization::usergraph
{

struct CaptureBoundary
{
  NodeNameId nameId = NodeNameId::Invalid;
  int comboIndex = UNKNOWN_INDEX;
  eastl::string searchInput;

  void clear()
  {
    nameId = NodeNameId::Invalid;
    comboIndex = UNKNOWN_INDEX;
    searchInput.clear();
  }
};

struct GpuCapture
{
  CaptureBoundary captureStart;
  CaptureBoundary captureEnd;

  intermediate::MultiplexingIndex captureExtentFilter = intermediate::MultiplexingIndex::Invalid;
  intermediate::MultiplexingIndex lastLabeledExtentFilter = intermediate::MultiplexingIndex::Invalid;

  dag::Vector<intermediate::MultiplexingIndex> distinctExtents;
  eastl::string captureExtentLabel;
  multiplexing::Extents lastLabeledExtents{};

  bool nodeRowPopupOpenedThisFrame = false;
  int captureCount = 1;
  bool focusGpuCaptureWindow = false;
};

} // namespace dafg::visualization::usergraph
