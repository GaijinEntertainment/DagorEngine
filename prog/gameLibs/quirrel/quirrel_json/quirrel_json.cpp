#include <quirrel/quirrel_json/quirrel_json.h>
#include <quirrel/sqModules/sqModules.h>
#include <stddef.h>
#include <EASTL/unique_ptr.h>
#include <util/dag_finally.h>

#include "direct_json/direct_json.h"

eastl::optional<eastl::string> rapidjson_parse(HSQUIRRELVM vm, eastl::string_view json_str);

Sqrat::Object parse_json_to_sqrat(HSQUIRRELVM vm, eastl::string_view json_str, eastl::optional<eastl::string> &err_str)
{
  eastl::optional<eastl::string> errStr = rapidjson_parse(vm, json_str);
  if (errStr.has_value())
  {
    err_str = eastl::move(errStr.value());
    return {};
  }
  HSQOBJECT obj;
  sq_getstackobj(vm, -1, &obj);
  Sqrat::Object res(obj, vm);
  sq_poptop(vm);
  return res;
}

eastl::string quirrel_to_jsonstr(Sqrat::Object obj, bool pretty)
{
  sq_pushobject(obj.GetVM(), obj.GetObject());
  FINALLY([&] { sq_poptop(obj.GetVM()); });
  return directly_convert_quirrel_val_to_json_string(obj.GetVM(), -1, pretty);
}

static SQInteger api_parse_json_direct(HSQUIRRELVM vm)
{
  eastl::string errorString;
  const char *jsonTxt;
  SQInteger len;
  G_VERIFY(SQ_SUCCEEDED(sq_getstringandsize(vm, 2, &jsonTxt, &len)));
  if (!parse_json_directly_to_quirrel(vm, jsonTxt, jsonTxt + len, errorString))
  {
    logwarn("Failed to parse json: %s\n%.1000s", errorString.c_str(), jsonTxt);
    return sqstd_throwerrorf(vm, "Failed to parse json: %.1000s", errorString.c_str());
  }
  return 1;
}

static SQInteger api_json_to_string_direct(HSQUIRRELVM vm)
{
  // quirrel function signature: function json_to_string(json_object, pretty = true)
  SQBool pretty = true;
  if (sq_gettop(vm) > 2)
    sq_getbool(vm, 3, &pretty);
  eastl::string doc = directly_convert_quirrel_val_to_json_string(vm, 2, pretty);
  sq_pushstring(vm, doc.c_str(), doc.size());
  return 1;
}

static SQInteger api_parse_json_rapid(HSQUIRRELVM vm)
{
  const char *jsonTxt;
  SQInteger len;
  G_VERIFY(SQ_SUCCEEDED(sq_getstringandsize(vm, 2, &jsonTxt, &len)));
  eastl::optional<eastl::string> errStr = rapidjson_parse(vm, {jsonTxt, size_t(len)});
  if (errStr.has_value())
  {
    logwarn("Failed to parse json: %.1000s", errStr.value().c_str());
    return sqstd_throwerrorf(vm, "Failed to parse json: %.1000s", errStr.value().c_str());
  }
  return 1;
}

namespace bindquirrel
{

void bind_json_api(SqModules *module_mgr)
{
  Sqrat::Table ns(module_mgr->getVM());
  ///@module json
  ns.SquirrelFunc("parse_json", api_parse_json_direct, 2, ".s")
    /**
      @function parse_json
      @param string s : string that would be parsed to json
      @return o|s|n|t|a : quirrel object
    */
    .SquirrelFunc("parse_json_rapid", api_parse_json_rapid, 2, ".s")
    /**
      @function parse_json_rapid parse json with rapidJson. DO NOT USE IT!. it is slower!
      @param string s : string that would be parsed to json
      @return o|s|n|t|a|b : quirrel object
    */
    .SquirrelFunc("json_to_string", api_json_to_string_direct, -2, "..b");
  /**
    @function json_to_string
    @param object o|s|n|t|a|b : object that will be converted to json string
    @return s : json string
  */
  module_mgr->addNativeModule("json", ns);
}

} // namespace bindquirrel
