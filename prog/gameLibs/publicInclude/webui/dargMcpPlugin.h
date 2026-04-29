//
// Dagor Engine 6.5 - Game Libraries
// Copyright (C) Gaijin Games KFT.  All rights reserved.
//
#pragma once

#include <webui/httpserver.h>

namespace darg
{
struct IGuiScene;
}

namespace webui
{
extern HttpPlugin darg_mcp_http_plugins[];
void set_darg_mcp_scene_provider(darg::IGuiScene *(*provider)());
} // namespace webui
