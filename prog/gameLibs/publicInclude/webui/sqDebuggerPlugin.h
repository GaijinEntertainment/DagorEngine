//
// Dagor Engine 6.5 - Game Libraries
// Copyright (C) 2023  Gaijin Games KFT.  All rights reserved
// (for conditions of use see prog/license.txt)
//
#pragma once

namespace webui
{
struct RequestInfo;
}

void on_sqdebug_internal(int debugger_index, webui::RequestInfo *params);

namespace webui
{
template <int N>
void on_sqdebug(RequestInfo *params)
{
  on_sqdebug_internal(N, params);
}
} // namespace webui
