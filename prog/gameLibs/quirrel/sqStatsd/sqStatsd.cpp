#include <sqModules/sqModules.h>
#include <sqrat.h>
#include <statsd/statsd.h>

#include <EASTL/fixed_vector.h>
#include <memory/dag_framemem.h>

namespace Sqrat
{
template <>
struct Var<statsd::MetricTags>
{
  statsd::MetricTags value;
  eastl::fixed_vector<Var<const char *>, 6, true, framemem_allocator> values;
  eastl::fixed_vector<statsd::MetricTag, 3, true, framemem_allocator> proxy_values;

  Var(HSQUIRRELVM vm, SQInteger idx)
  {
    Var<Sqrat::Table> tbl(vm, idx);

    Sqrat::Object::iterator iter;
    while (tbl.value.Next(iter))
    {
      Sqrat::Object key(iter.getKey(), vm);
      G_ASSERT_RETURN(key.GetType() == OT_STRING, );
      values.emplace_back(key.GetVar<const char *>());
      const char *keystr = values.back().value;

      Sqrat::Object tblVal(iter.getValue(), vm);

      SQObjectType otype = tblVal.GetType();
      G_ASSERT_RETURN((otype == OT_STRING || otype == OT_INTEGER || otype == OT_BOOL || otype == OT_FLOAT), );

      values.emplace_back(tblVal.GetVar<const char *>());
      const char *valuestr = values.back().value;
      proxy_values.push_back({keystr, valuestr});
    }

    value = proxy_values;
  }
};
} // end namespace Sqrat

namespace bindquirrel
{

void bind_statsd(SqModules *module_mgr)
{
  Sqrat::Table ns(module_mgr->getVM());

  ///@module statsd

  ns
    /**
      @function send_counter
      @param metric_name s
      @param value i
      @param tags t {}: tags for metrics
    */
    .SquirrelFunc(
      "send_counter",
      [](HSQUIRRELVM vm) -> SQInteger {
        Sqrat::Var<const char *> stat(vm, 2);
        Sqrat::Var<int> value(vm, 3);
        Sqrat::Var<statsd::MetricTags> sqtags(vm, 4);
        statsd::counter(stat.value, value.value, sqtags.value);
        return 1;
      },
      -3, ".sit")
    /**
      @function send_gauge
      @param metric_name s
      @param value i
      @param tags t {}: tags for metrics
    */
    .SquirrelFunc(
      "send_gauge",
      [](HSQUIRRELVM vm) -> SQInteger {
        Sqrat::Var<const char *> stat(vm, 2);
        Sqrat::Var<int> value(vm, 3);
        Sqrat::Var<statsd::MetricTags> sqtags(vm, 4);
        statsd::gauge(stat.value, value.value, sqtags.value);
        return 1;
      },
      -3, ".sit")
    /**
      @function send_gauge_dec decrement gauge
      @param metric_name s
      @param value i
      @param tags t {}: tags for metrics
    */
    .SquirrelFunc(
      "send_gauge_dec",
      [](HSQUIRRELVM vm) -> SQInteger {
        Sqrat::Var<const char *> stat(vm, 2);
        Sqrat::Var<int> value(vm, 3);
        Sqrat::Var<statsd::MetricTags> sqtags(vm, 4);
        statsd::gauge_dec(stat.value, value.value, sqtags.value);
        return 1;
      },
      -3, ".sit")
    /**
      @function send_gauge_inc increment gauge
      @param metric_name s
      @param value f
      @param tags t {}: tags for metrics
    */
    .SquirrelFunc(
      "send_gauge_inc",
      [](HSQUIRRELVM vm) -> SQInteger {
        Sqrat::Var<const char *> stat(vm, 2);
        Sqrat::Var<float> value(vm, 3);
        Sqrat::Var<statsd::MetricTags> sqtags(vm, 4);
        statsd::gauge_inc(stat.value, value.value, sqtags.value);
        return 1;
      },
      -3, ".sft")
    /**
      @function send_profile
      @param metric_name s
      @param value i
      @param tags t {}: tags for metrics
    */
    .SquirrelFunc(
      "send_profile",
      [](HSQUIRRELVM vm) -> SQInteger {
        Sqrat::Var<const char *> stat(vm, 2);
        Sqrat::Var<int> value(vm, 3);
        Sqrat::Var<statsd::MetricTags> sqtags(vm, 4);
        statsd::profile(stat.value, value.value, sqtags.value);
        return 1;
      },
      -3, ".sit")
    /**
      @function send_histogram
      @param metric_name s
      @param value i
      @param tags t {}: tags for metrics
    */
    .SquirrelFunc(
      "send_histogram",
      [](HSQUIRRELVM vm) -> SQInteger {
        Sqrat::Var<const char *> stat(vm, 2);
        Sqrat::Var<int> value(vm, 3);
        Sqrat::Var<statsd::MetricTags> sqtags(vm, 4);
        statsd::histogram(stat.value, value.value, sqtags.value);
        return 1;
      },
      -3, ".sit");

  module_mgr->addNativeModule("statsd", ns);
}

} // namespace bindquirrel
