// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include <debug/shaderVarDump.h>

namespace dafg::shadervar_dump
{

void request(const char *) {}
void begin_frame(const intermediate::Graph &) {}
void after_node(const intermediate::NodeIndex) {}
void end_frame(const intermediate::Graph &) {}

} // namespace dafg::shadervar_dump
