// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include <sqrat.h>
#include <sqModules/sqModules.h>
#include <ctime>
#include <cmath>
#include <cstring>

#include <iso8601Time/iso8601Time.h>

static long long iso8601_parse(HSQUIRRELVM v)
{
  Sqrat::Var<const char *> strSq(v, 2);
  return iso8601_parse(strSq.value);
}

static SQInteger iso8601_parse_sec(HSQUIRRELVM v)
{
  long long timeMsec = iso8601_parse(v);
  if (!timeMsec)
  {
    sq_pushnull(v);
    return 1;
  }
  sq_pushinteger(v, timeMsec / 1000LL);
  return 1;
}


static SQInteger iso8601_parse_msec(HSQUIRRELVM v)
{
  long long timeMsec = iso8601_parse(v);
  if (!timeMsec)
  {
    sq_pushnull(v);
    return 1;
  }
  sq_pushinteger(v, timeMsec);
  return 1;
}

static SQInteger iso8601_format_sec(HSQUIRRELVM v)
{
  Sqrat::Var<long long> timeSq(v, 2);
  long long timeSec = timeSq.value;
  char buffer[64];
  iso8601_format(timeSec * 1000LL, buffer, sizeof(buffer));
  sq_pushstring(v, buffer, -1);
  return 1;
}


static SQInteger iso8601_format_msec(HSQUIRRELVM v)
{
  Sqrat::Var<long long> timeSq(v, 2);
  long long timeMsec = timeSq.value;
  char buffer[64];
  iso8601_format(timeMsec, buffer, sizeof(buffer));
  sq_pushstring(v, buffer, -1);
  return 1;
}


namespace bindquirrel
{

void register_iso8601_time(SqModules *module_mgr)
{
  HSQUIRRELVM vm = module_mgr->getVM();
  Sqrat::Table exports(vm);

  ///@module dagor.iso8601
  /// Functions for converting between timestamps as a string in ISO-8601 format (like "YYYY-MM-DDTHH:MM:SSZ") and
  /// timestamps as an integer (UNIX timestamp).
  exports
    .SquirrelFunc("parse_msec", iso8601_parse_msec, 2, ".s")
    ///@brief Converts ISO-8601 timestamp string to UNIX timestamp in milliseconds (!) integer.
    ///@param iso8601str s : ISO-8601 timestamp
    ///@return i : UNIX timestamp in milliseconds
    .SquirrelFunc("parse_unix_time", iso8601_parse_sec, 2, ".s")
    ///@brief Converts ISO-8601 timestamp string to UNIX timestamp integer.
    ///@param iso8601str s : ISO-8601 timestamp
    ///@return i : UNIX timestamp
    .SquirrelFunc("format_msec", iso8601_format_msec, 2, ".i")
    ///@brief Converts UNIX timestamp in milliseconds (!) integer to ISO-8601 timestamp string.
    ///@param ts i : UNIX timestamp in milliseconds
    ///@return s : ISO-8601 timestamp
    .SquirrelFunc("format_unix_time", iso8601_format_sec, 2, ".i")
    ///@brief Converts UNIX timestamp integer to ISO-8601 timestamp string.
    ///@param ts i : UNIX timestamp
    ///@return s : ISO-8601 timestamp
    ;

  module_mgr->addNativeModule("dagor.iso8601", exports);
}

} // namespace bindquirrel
