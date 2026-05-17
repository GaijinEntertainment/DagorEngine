// Copyright (C) Gaijin Games KFT.  All rights reserved.
#pragma once

#include <shaders/shUtils.h>
#include <debug/dag_debug.h>

#if DAGOR_DBGLEVEL > 0
namespace detail
{
static thread_local int div_by_zero_reports = 0;
static constexpr int DIV_BY_ZERO_REPORT_MAX_COUNT = 100;
} // namespace detail

static bool report_real_div_by_zero(intptr_t code_id, const char *tag)
{
  if (++detail::div_by_zero_reports <= detail::DIV_BY_ZERO_REPORT_MAX_COUNT)
  {
    logerr("%s: divide by zero [real] while exec shader code at operand #%d", tag, code_id);
    return true;
  }
  else if (detail::div_by_zero_reports == detail::DIV_BY_ZERO_REPORT_MAX_COUNT)
  {
    logerr("divide by zero [real] repeated %d times, will not report again this session", detail::DIV_BY_ZERO_REPORT_MAX_COUNT);
  }
  return false;
}
static bool report_float4_div_by_zero(intptr_t code_id, int comp_id, const char *tag)
{
  if (++detail::div_by_zero_reports < detail::DIV_BY_ZERO_REPORT_MAX_COUNT)
  {
    logerr("%s: divide by zero [color4[%d]] while exec shader code. stopped at operand #%d", tag, comp_id, code_id);
    return true;
  }
  else if (detail::div_by_zero_reports == detail::DIV_BY_ZERO_REPORT_MAX_COUNT)
  {
    logerr("divide by zero [color4[%d]] repeated %d times, will not report again this session", comp_id,
      detail::DIV_BY_ZERO_REPORT_MAX_COUNT);
  }
  return false;
}
#else
static bool report_real_div_by_zero(intptr_t) { return false; }
static bool report_float4_div_by_zero(intptr_t code_id, int comp_id) { return false; }
#endif
