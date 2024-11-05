//
// Dagor Engine 6.5 - Game Libraries
// Copyright (C) Gaijin Games KFT.  All rights reserved.
//
#pragma once

#include <sqrat.h>
#include <json/value.h>

// jsoncpp library support

Sqrat::Object jsoncpp_to_quirrel(HSQUIRRELVM vm, const Json::Value &jval);
Json::Value quirrel_to_jsoncpp(Sqrat::Object obj);
