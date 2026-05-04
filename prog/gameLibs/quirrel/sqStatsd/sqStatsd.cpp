// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include <sqmodules/sqmodules.h>
#include <sqrat.h>
#include <sqstdaux.h>
#include <statsd/statsd.h>

#include <EASTL/fixed_vector.h>
#include <memory/dag_framemem.h>


// Owns backing storage (string refs + MetricTag pairs) for a statsd::MetricTags
// view parsed out of a script table. Must outlive any statsd::* call that uses
// the view.
struct ScriptMetricTags
{
  eastl::fixed_vector<Sqrat::Var<const char *>, 6, true, framemem_allocator> strings;
  eastl::fixed_vector<statsd::MetricTag, 3, true, framemem_allocator> pairs;

  operator statsd::MetricTags() const { return make_span_const(pairs); }
};


// Parses an optional tags table at stack slot `idx`. Absent or explicit null
// leaves `out` empty and returns SQ_OK. On malformed input, throws a script
// error and returns SQ_ERROR.
static SQRESULT read_tags(HSQUIRRELVM vm, SQInteger idx, ScriptMetricTags &out)
{
  if (idx > sq_gettop(vm) || sq_gettype(vm, idx) == OT_NULL)
    return SQ_OK;
  if (sq_gettype(vm, idx) != OT_TABLE) // checked by the typemask, but just in case
    return sq_throwerror(vm, "tags must be a table");

  Sqrat::Var<Sqrat::Table> tbl(vm, idx);
  Sqrat::Object::iterator it;
  while (tbl.value.Next(it))
  {
    Sqrat::Object key(it.getKey(), vm);
    if (key.GetType() != OT_STRING)
      return sq_throwerror(vm, "tags keys must be strings");

    out.strings.emplace_back(key.GetVar<const char *>());
    const char *k = out.strings.back().value;

    Sqrat::Object val(it.getValue(), vm);
    const SQObjectType vt = val.GetType();
    if (vt != OT_STRING && vt != OT_INTEGER && vt != OT_BOOL && vt != OT_FLOAT)
      return sqstd_throwerrorf(vm, "tags[%s] must be string, integer, bool, or float", k);

    out.strings.emplace_back(val.GetVar<const char *>());
    const char *v = out.strings.back().value;
    out.pairs.push_back({k, v});
  }
  return SQ_OK;
}


namespace bindquirrel
{

void bind_statsd(SqModules *module_mgr)
{
  Sqrat::Table ns(module_mgr->getVM());

  ///@module statsd

  ns
    /* qdox
      @function send_counter
      @param metric_name s
      @param value i
      @param tags t {}: tags for metrics
    */
    .SquirrelFuncDeclString(
      +[](HSQUIRRELVM vm) -> SQInteger {
        Sqrat::Var<const char *> stat(vm, 2);
        Sqrat::Var<int> value(vm, 3);
        ScriptMetricTags tags;
        if (SQ_FAILED(read_tags(vm, 4, tags)))
          return SQ_ERROR;
        statsd::counter(stat.value, value.value, tags);
        return 0;
      },
      "send_counter(metric_name: string, value: int, [tags: table]): null")
    /* qdox
      @function send_gauge
      @param metric_name s
      @param value i
      @param tags t {}: tags for metrics
    */
    .SquirrelFuncDeclString(
      +[](HSQUIRRELVM vm) -> SQInteger {
        Sqrat::Var<const char *> stat(vm, 2);
        Sqrat::Var<int> value(vm, 3);
        ScriptMetricTags tags;
        if (SQ_FAILED(read_tags(vm, 4, tags)))
          return SQ_ERROR;
        statsd::gauge(stat.value, value.value, tags);
        return 0;
      },
      "send_gauge(metric_name: string, value: int, [tags: table]): null")
    /* qdox
      @function send_gauge_dec decrement gauge
      @param metric_name s
      @param value i
      @param tags t {}: tags for metrics
    */
    .SquirrelFuncDeclString(
      +[](HSQUIRRELVM vm) -> SQInteger {
        Sqrat::Var<const char *> stat(vm, 2);
        Sqrat::Var<int> value(vm, 3);
        ScriptMetricTags tags;
        if (SQ_FAILED(read_tags(vm, 4, tags)))
          return SQ_ERROR;
        statsd::gauge_dec(stat.value, value.value, tags);
        return 0;
      },
      "send_gauge_dec(metric_name: string, value: int, [tags: table]): null")
    /* qdox
      @function send_gauge_inc increment gauge
      @param metric_name s
      @param value f
      @param tags t {}: tags for metrics
    */
    .SquirrelFuncDeclString(
      +[](HSQUIRRELVM vm) -> SQInteger {
        Sqrat::Var<const char *> stat(vm, 2);
        Sqrat::Var<float> value(vm, 3);
        ScriptMetricTags tags;
        if (SQ_FAILED(read_tags(vm, 4, tags)))
          return SQ_ERROR;
        statsd::gauge_inc(stat.value, value.value, tags);
        return 0;
      },
      "send_gauge_inc(metric_name: string, value: float, [tags: table]): null")
    /* qdox
      @function send_profile
      @param metric_name s
      @param value i
      @param tags t {}: tags for metrics
    */
    .SquirrelFuncDeclString(
      +[](HSQUIRRELVM vm) -> SQInteger {
        Sqrat::Var<const char *> stat(vm, 2);
        Sqrat::Var<int> value(vm, 3);
        ScriptMetricTags tags;
        if (SQ_FAILED(read_tags(vm, 4, tags)))
          return SQ_ERROR;
        statsd::profile(stat.value, (long)value.value, tags);
        return 0;
      },
      "send_profile(metric_name: string, value: int, [tags: table]): null")
    /* qdox
      @function send_histogram
      @param metric_name s
      @param value i
      @param tags t {}: tags for metrics
    */
    .SquirrelFuncDeclString(
      +[](HSQUIRRELVM vm) -> SQInteger {
        Sqrat::Var<const char *> stat(vm, 2);
        Sqrat::Var<int> value(vm, 3);
        ScriptMetricTags tags;
        if (SQ_FAILED(read_tags(vm, 4, tags)))
          return SQ_ERROR;
        statsd::histogram(stat.value, (long)value.value, tags);
        return 0;
      },
      "send_histogram(metric_name: string, value: int, [tags: table]): null");

  module_mgr->addNativeModule("statsd", ns);
}

} // namespace bindquirrel
