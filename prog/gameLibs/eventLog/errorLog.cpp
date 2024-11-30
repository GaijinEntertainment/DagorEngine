// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include <debug/dag_assert.h>
#include <debug/dag_fatal.h>
#include <ioSys/dag_memIo.h>
#include <ioSys/dag_zlibIo.h>
#include <math/random/dag_random.h>
#include <memory/dag_framemem.h>
#include <osApiWrappers/dag_miscApi.h>
#include <osApiWrappers/dag_stackHlp.h>
#include <util/dag_hash.h>
#include <EASTL/fixed_vector.h>
#include <EASTL/string.h>
#include <eventLog/errorLog.h>
#include <eventLog/eventLog.h>
#include <util/dag_string.h>
#include <json/value.h>

namespace event_log
{

static Json::Value meta;
static eastl::string collection;

static float fatal_probability = 0.0f;

static uint32_t str_hash_fnv1_skip_digits(const char *s)
{
  uint32_t result = FNV1Params<32>::offset_basis;
  uint32_t c = 0;
  while ((c = *s++) != 0)
  {
    char v = ((c < '0' || c > '9') && c != '.') ? c : ' ';
    result = (result * FNV1Params<32>::prime) ^ v;
  }
  return result;
}

void init_error_log(const ErrorLogInitParams &params)
{
  G_ASSERT_RETURN(params.collection != nullptr, );

  collection = params.collection;
  fatal_probability = params.fatal_probability;
  if (params.game)
    meta["game"] = params.game;
  meta["hint"] = "error";
  meta["content-type"] = "text/deflate";
}

void set_error_log_meta_val(const char *key, const char *value)
{
  if (!value)
    meta.removeMember(value);
  else
    meta[key] = value;
}

static void append_debug_log(eastl::string &out)
{
  const size_t MAX_LOG_SIZE = 30 * 1024;

  Tab<char> log(framemem_ptr());
  log.resize(MAX_LOG_SIZE);
  int logLength = tail_debug_file(log.data(), log.size());
  log.resize(logLength);

  if (!logLength)
    out.append("Could not tail log file");
  else
    out.insert(out.end(), log.data(), log.data() + logLength);
}

static uint32_t calc_error_hash(const char *error_message)
{
  const char *variablesBegin = strstr(error_message, "\nLOCALS\n");
  uint32_t result = 0;
  if (variablesBegin)
  {
    result = mem_hash_fnv1(error_message, variablesBegin - error_message);
    const char *variablesEnd = strstr(variablesBegin, "\n\n");
    if (variablesEnd)
      result ^= str_hash_fnv1_skip_digits(variablesEnd);
  }
  else
  {
    result = str_hash_fnv1_skip_digits(error_message);
  }

  return result;
}

static const char *map_severity(LogSeverity sev)
{
  switch (sev)
  {
    case LogSeverity::Critical: return "critical";
    case LogSeverity::Normal: return "normal";
    default: return "low";
  }
}

void send_error_log(const char *error_message, ErrorLogSendParams const &params)
{
  G_ASSERT_RETURN(error_message && *error_message, );

  if (collection.empty() || !event_log::is_enabled())
    return;

  const char *currentCollection = collection.c_str();
  if (params.collection)
    currentCollection = params.collection;

  if (fatal_probability > 0.0)
  {
    if (gfrnd() > fatal_probability)
    {
      DAG_FATAL(error_message);
      return;
    }
  }

  constexpr int MAX_ERRORS = 32;
  static eastl::fixed_vector<uint32_t, MAX_ERRORS, false> error_hashes;
  if (error_hashes.size() == MAX_ERRORS) // too many errors already sent
    return;
  uint32_t errorHash = params.exMsgHash ? params.exMsgHash : calc_error_hash(error_message);

  if (eastl::find(error_hashes.begin(), error_hashes.end(), errorHash) != error_hashes.end())
    return;
  error_hashes.push_back(errorHash);

  eastl::string eventStr = error_message;

  if (params.dump_call_stack)
  {
    stackhelp::CallStackCaptureStore<48> stack;
    stackhelp::ext::CallStackCaptureStore extStack;
    stack.capture();
    extStack.capture();
    eventStr.append(get_call_stack_str(stack, extStack));
  }

  Json::Value jsheader = meta;
  jsheader["hash"] = errorHash;
  jsheader["severity"] = map_severity(params.severity);

  for (const eastl::string &key : params.meta.getMemberNames())
    jsheader[key] = params.meta[key];

  if (params.attach_game_log)
  {
    eventStr.append("\nGAME LOG:\n");
    append_debug_log(eventStr);
  }

  DynamicMemGeneralSaveCB buf(tmpmem, eventStr.size());
  {
    ZlibSaveCB zlib(buf, ZlibSaveCB::CL_BestSpeed + 6);
    zlib.write(eventStr.c_str(), eventStr.length());
    zlib.finish();
  }

  static constexpr int MAX_UDP_LENGTH = 900;
  if (buf.size() > MAX_UDP_LENGTH)
  {
    (params.instant_send ? event_log::send_http_instant : event_log::send_http)(currentCollection, buf.data(), buf.size(), &jsheader);
  }
  else
    event_log::send_udp(currentCollection, buf.data(), buf.size(), &jsheader);
}

} // namespace event_log
