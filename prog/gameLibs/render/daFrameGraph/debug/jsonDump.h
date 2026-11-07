// Copyright (C) Gaijin Games KFT.  All rights reserved.
#pragma once

#include <frontend/internalRegistry.h>
#include <backend/intermediateRepresentation.h>
#include <backend/nodeStateDeltas.h>
#include <backend/resourceScheduling/barrierScheduler.h>
#include <backend/resourceScheduling/heapLayout.h>


namespace dafg::state_dump
{

struct DumpSources
{
  const InternalRegistry &registry;
  const intermediate::Graph &ir_graph;
  const sd::NodeStateDeltas &state_deltas;
  const BarrierScheduler::EventsCollection &events;
  const Layout &layout;
};

void dump(const DumpSources &dump_src, const char *custom_filename = nullptr);

} // namespace dafg::state_dump
