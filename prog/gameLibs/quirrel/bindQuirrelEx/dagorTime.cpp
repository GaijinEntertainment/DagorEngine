// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include <bindQuirrelEx/bindQuirrelEx.h>
#include <sqModules/sqModules.h>
#include <perfMon/dag_cpuFreq.h>
#include <time.h>

#if _TARGET_C1 | _TARGET_C2

#endif

#include <sqrat.h>

#if _TARGET_PC_WIN | _TARGET_XBOX
static struct tm *localtime_r(const time_t *ts, struct tm *tm)
{
  localtime_s(tm, ts);
  return tm;
}
#elif _TARGET_C1 | _TARGET_C2





#endif

static time_t get_local_unixtime() { return time(NULL); }

static SQInteger unixtime_to_local_timetbl(HSQUIRRELVM vm)
{
  SQInteger timestamp = 0;
  sq_getinteger(vm, 2, &timestamp);
  Sqrat::Table ret(vm);

  time_t time_ = (time_t)timestamp;
  struct tm t;
  localtime_r(&time_, &t);

  ret.SetValue("year", (int)t.tm_year + 1900);
  ret.SetValue("month", (int)t.tm_mon);
  ret.SetValue("day", (int)t.tm_mday);
  ret.SetValue("hour", (int)t.tm_hour);
  ret.SetValue("min", (int)t.tm_min);
  ret.SetValue("sec", (int)t.tm_sec);
  ret.SetValue("dayOfWeek", t.tm_wday);

  sq_pushobject(vm, ret);
  return 1;
}

static SQInteger local_timetbl_to_unixtime(HSQUIRRELVM vm)
{
  Sqrat::Table sq_tm = Sqrat::Var<Sqrat::Table>(vm, 2).value;
  struct tm tm;
  tm.tm_year = sq_tm.GetSlotValue("year", 1970) - 1900;
  tm.tm_mon = sq_tm.GetSlotValue("month", 0);
  tm.tm_mday = sq_tm.GetSlotValue("day", 1);
  tm.tm_hour = sq_tm.GetSlotValue("hour", 0);
  tm.tm_min = sq_tm.GetSlotValue("min", 0);
  tm.tm_sec = sq_tm.GetSlotValue("sec", 0);
  tm.tm_isdst = -1;
  sq_pushinteger(vm, ::mktime(&tm));
  return 1;
}

static SQInteger unixtime_to_utc_timetbl(HSQUIRRELVM vm)
{
  SQInteger timestamp = 0;
  sq_getinteger(vm, 2, &timestamp);
  Sqrat::Table ret(vm);

  time_t rawTimeT = (time_t)timestamp;
  struct tm t;
#if _TARGET_PC_WIN | _TARGET_XBOX
  ::gmtime_s(&t, &rawTimeT);
#elif _TARGET_C1 | _TARGET_C2

#else
  ::gmtime_r(&rawTimeT, &t);
#endif

  ret.SetValue("year", (int)t.tm_year + 1900);
  ret.SetValue("month", (int)t.tm_mon);
  ret.SetValue("day", (int)t.tm_mday);
  ret.SetValue("hour", (int)t.tm_hour);
  ret.SetValue("min", (int)t.tm_min);
  ret.SetValue("sec", (int)t.tm_sec);
  ret.SetValue("dayOfWeek", t.tm_wday);

  sq_pushobject(vm, ret);
  return 1;
}

static SQInteger utc_timetbl_to_unixtime(HSQUIRRELVM vm)
{
  Sqrat::Table sq_tm = Sqrat::Var<Sqrat::Table>(vm, 2).value;
  struct tm t;
  memset(&t, 0, sizeof(tm));
  t.tm_year = sq_tm.GetSlotValue("year", 1970) - 1900;
  t.tm_mon = sq_tm.GetSlotValue("month", 0);
  t.tm_mday = sq_tm.GetSlotValue("day", 1);
  t.tm_hour = sq_tm.GetSlotValue("hour", 0);
  t.tm_min = sq_tm.GetSlotValue("min", 0);
  t.tm_sec = sq_tm.GetSlotValue("sec", 0);
#if _TARGET_PC_WIN | _TARGET_XBOX
  sq_pushinteger(vm, ::_mkgmtime(&t));
#else
  sq_pushinteger(vm, ::timegm(&t));
#endif
  return 1;
}

static eastl::string format_unixtime(const char *fmt, time_t ts)
{
  struct tm tm;
  localtime_r(&ts, &tm);
  char buf[64];
  strftime(buf, sizeof(buf), fmt, &tm);
  return buf;
}

namespace bindquirrel
{

void bind_dagor_time(SqModules *module_mgr)
{
  Sqrat::Table nsTbl(module_mgr->getVM());

  ///@module dagor.time
  /// Date/time getters and converters.
  nsTbl //
    .Func("ref_time_ticks", ref_time_ticks)
    .Func("get_time_usec", get_time_usec)
    .Func("get_time_msec", get_time_msec)
    ///@brief Returns the number of milliseconds that have passed since the client was started.
    ///@return i : Milliseconds since client start
    .Func("get_local_unixtime", get_local_unixtime)
    ///@brief Returns UNIX timestamp from the local machine clock. Don't trust it too much, because user can set a fake time on his
    /// machine manually.
    ///@return i : UNIX timestamp from local machine clock
    .Func("format_unixtime", format_unixtime)
    ///@brief Formats UNIX timestamp as a local time string, using the C++ function strftime().
    ///@param format s : Format string (see strftime() C++ function manual)
    ///@param ts i : UNIX timestamp to format
    ///@return s : Formatted string
    .SquirrelFunc("unixtime_to_local_timetbl", unixtime_to_local_timetbl, 2, ".i")
    ///@brief Converts UNIX timestamp to Local time table.
    ///@param ts i : UNIX timestamp
    ///@return t : Local time table
    .SquirrelFunc("local_timetbl_to_unixtime", local_timetbl_to_unixtime, 2, ".t")
    ///@brief Converts Local time table to UNIX timestamp.
    ///@param localTimeTbl t : Local time table
    ///@return i : UNIX timestamp
    .SquirrelFunc("unixtime_to_utc_timetbl", unixtime_to_utc_timetbl, 2, ".i")
    ///@brief Converts UNIX timestamp to UTC time table.
    ///@param ts i : UNIX timestamp
    ///@return t : UTC time table
    .SquirrelFunc("utc_timetbl_to_unixtime", utc_timetbl_to_unixtime, 2, ".t")
    ///@brief Converts UTC time table to UNIX timestamp.
    ///@param utcTimeTbl t : UTC time table
    ///@return i : UNIX timestamp
    /**/;

  module_mgr->addNativeModule("dagor.time", nsTbl);
}

} // namespace bindquirrel
