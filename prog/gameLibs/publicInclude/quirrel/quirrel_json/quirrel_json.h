//
// Dagor Engine 6.5 - Game Libraries
// Copyright (C) Gaijin Games KFT.  All rights reserved.
//
#pragma once

#include <sqrat.h>
#include <EASTL/string.h>
#include <EASTL/string_view.h>
#include <EASTL/optional.h>

class SqModules;

namespace Json
{
class Value;
}

namespace bindquirrel
{
void bind_json_api(SqModules *module_mgr);
}

Sqrat::Object parse_json_to_sqrat(HSQUIRRELVM vm, eastl::string_view json_str, eastl::optional<eastl::string> &err_str);
eastl::string quirrel_to_jsonstr(Sqrat::Object obj, bool pretty = false);
