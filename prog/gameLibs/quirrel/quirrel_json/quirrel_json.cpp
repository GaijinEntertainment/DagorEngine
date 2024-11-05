// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include <quirrel/quirrel_json/quirrel_json.h>
#include <quirrel/sqModules/sqModules.h>
#include <quirrel/sqStackChecker.h>
#include <sqstdblob.h>
#include <stddef.h>
#include <EASTL/unique_ptr.h>
#include <util/dag_finally.h>

#include <sqstdlib/sqstdblobimpl.h> // Quirrel standard library internals

#include "direct_json/direct_json.h"


eastl::string quirrel_to_jsonstr(Sqrat::Object obj, bool pretty)
{
  sq_pushobject(obj.GetVM(), obj.GetObject());
  FINALLY([&] { sq_poptop(obj.GetVM()); });
  return directly_convert_quirrel_val_to_json_string(obj.GetVM(), -1, pretty);
}


Sqrat::Object parse_json_to_sqrat(HSQUIRRELVM vm, eastl::string_view json_str, eastl::optional<eastl::string> &err_str)
{
  SqStackChecker chk(vm);

  eastl::string errorString;
  if (!parse_json_directly_to_quirrel(vm, json_str.begin(), json_str.end(), errorString))
  {
    sq_poptop(vm);
    err_str = eastl::move(errorString);
    return {};
  }

  HSQOBJECT obj;
  sq_getstackobj(vm, -1, &obj);
  Sqrat::Object res(obj, vm);
  sq_poptop(vm);
  return res;
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


static SQInteger api_parse_json_direct_from_zstd_stream(HSQUIRRELVM vm)
{
  SQUserPointer blobData;
  SQInteger index = 2;
  if (SQ_FAILED(sqstd_getblob(vm, index, &blobData)))
    return sqstd_throwerrorf(vm, "Expected Blob");
  SQInteger blobLen = sqstd_getblobsize(vm, index);

  if (blobLen <= 0)
    return sqstd_throwerrorf(vm, "Blob is empty");

  eastl::string errorString;
  if (!parse_json_zstd_stream_directly_to_quirrel(vm, blobData, size_t(blobLen), errorString))
  {
    logwarn("Failed to parse json from zstd stream: %s", errorString.c_str());
    return sqstd_throwerrorf(vm, "Failed to parse json from zstd stream: %s", errorString.c_str());
  }
  return 1;
}


static SQInteger api_object_to_json_string_direct(HSQUIRRELVM vm)
{
  // quirrel function signature: function object_to_json_string(json_object, pretty = false)
  SQBool pretty = false;
  if (sq_gettop(vm) > 2)
    sq_getbool(vm, 3, &pretty);
  eastl::string doc = directly_convert_quirrel_val_to_json_string(vm, 2, pretty);
  sq_pushstring(vm, doc.c_str(), doc.size());
  return 1;
}


static SQInteger api_object_to_zstd_json_direct(HSQUIRRELVM vm)
{
  SQUserPointer data = sqstd_createblob(vm, 0);
  if (!data)
    return sqstd_throwerrorf(vm, "Failed to create blob");

  SQBlob *blob = nullptr;
  if (SQ_FAILED(sq_getinstanceup(vm, -1, (SQUserPointer *)&blob, nullptr)))
    return sqstd_throwerrorf(vm, "Failed to get blob instance");

  auto writeFunc = [](const void *data, size_t size, void *user_data) {
    SQBlob *blob = (SQBlob *)user_data;
    blob->Write(data, size);
  };

  directly_convert_quirrel_val_to_zstd_json(vm, 2, writeFunc, blob);

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
    .SquirrelFunc("parse_json_from_zstd_stream", api_parse_json_direct_from_zstd_stream, 2, ".x")
    /**
      @function parse_json_from_zstd_stream
      @param blob s : blob with ztsd stream that would be parsed to json
      @return o|s|n|t|a : quirrel object
    */
    .SquirrelFunc("object_to_json_string", api_object_to_json_string_direct, -2, "..b")
    /**
      @function object_to_json_string
      @param object o|s|n|t|a|b : object that will be converted to json string
      @return s : json string
    */
    .SquirrelFunc("object_to_zstd_json", api_object_to_zstd_json_direct, -2, "..")
    /**
      @function object_to_json_string
      @param object o|s|n|t|a|b : object that will be converted to json
      @return b : blob with compressed json string
    */
    ;
  module_mgr->addNativeModule("json", ns);
}

} // namespace bindquirrel
