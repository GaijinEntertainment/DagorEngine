//
// Dagor Engine 6.5 - Game Libraries
// Copyright (C) Gaijin Games KFT.  All rights reserved.
//
#pragma once

#include <EASTL/string_view.h>


namespace websocketjsonrpc::details
{


eastl::string_view url_to_hostname(eastl::string_view url);


} // namespace websocketjsonrpc::details
