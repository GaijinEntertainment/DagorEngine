//
// Dagor Engine 6.5 - Game Libraries
// Copyright (C) 2023  Gaijin Games KFT.  All rights reserved
// (for conditions of use see prog/license.txt)
//

#include <sqrat.h>
#include <json/value.h>

// jsoncpp library support

Sqrat::Object jsoncpp_to_quirrel(HSQUIRRELVM vm, const Json::Value &jval);
Json::Value quirrel_to_jsoncpp(Sqrat::Object obj);
