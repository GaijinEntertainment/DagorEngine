// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include <render/rtdl/rtdl_api.h>

namespace rtdl
{
void initialize() {}
void teardown() {}
bool is_initialized() { return false; }

void register_nodes(dafg::NameSpace, const NodeParams &, dag::Vector<dafg::NodeHandle> &) {}
} // namespace rtdl
