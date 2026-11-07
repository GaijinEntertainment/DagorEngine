// Copyright (C) Gaijin Games KFT.  All rights reserved.
#pragma once

#include <backend/intermediateRepresentation.h>

namespace dafg::shadervar_dump
{

// Signals daFG to dump shadervar changes during next run.
// Called from a console command handler.
void request(const char *custom_filename = nullptr);

// Called once, before the node execution loop.
void begin_frame(const intermediate::Graph &irGraph);

// Called after each node's execution.
void after_node(const intermediate::NodeIndex node_idx);

// Called once, after the node execution loop.
// Writes the JSON file and disarms the capture.
void end_frame(const intermediate::Graph &irGraph);

} // namespace dafg::shadervar_dump
