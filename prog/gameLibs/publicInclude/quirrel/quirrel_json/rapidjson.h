//
// Dagor Engine 6.5 - Game Libraries
// Copyright (C) Gaijin Games KFT.  All rights reserved.
//
#pragma once

#include <sqrat.h>
#include <rapidjson/document.h>

// rapidjson library support

Sqrat::Object rapidjson_to_quirrel(HSQUIRRELVM vm, const rapidjson::Value &jval);
rapidjson::Document quirrel_to_rapidjson(Sqrat::Object obj);
