#include <sqModules/sqModules.h>
#include <sqrat.h>
#include <squirrel.h>
#include <quirrel/quirrel_json/quirrel_json.h>
#include <quirrel/quirrel_json/rapidjson.h>
#include <jsonUtils/decodeJwt.h>


namespace bindquirrel
{
static SQInteger decode_jwt(HSQUIRRELVM vm)
{
  Sqrat::Var<const char *> token(vm, 2);
  Sqrat::Var<const char *> publicKey(vm, 3);

  rapidjson::Document header, payload;
  jsonutils::DecodeJwtResult res = jsonutils::decode_jwt(token.value, publicKey.value, header, payload);
  Sqrat::Table table(vm);
  if (res == jsonutils::DecodeJwtResult::OK)
  {
    table.SetValue("header", rapidjson_to_quirrel(vm, header));
    table.SetValue("payload", rapidjson_to_quirrel(vm, payload));
  }
  else
  {
    static const char *result_strings[] = {"OK", "INVALID_TOKEN", "WRONG_HEADER", "INVALID_TOKEN_TYPE",
      "INVALID_TOKEN_SIGNATURE_ALGORITHM", "INVALID_PAYLOAD_COMPRESSION", "WRONG_PAYLOAD", "SIGNATURE_VERIFICATION_FAILED"};
    G_STATIC_ASSERT(countof(result_strings) == (size_t)jsonutils::DecodeJwtResult::NUM);
    table.SetValue("error", result_strings[(size_t)res]);
  }

  sq_pushobject(vm, table.GetObject());
  return 1;
}

void bind_jwt(SqModules *module_mgr)
{
  Sqrat::Table nsTbl(module_mgr->getVM());
  ///@module jwt
  nsTbl.SquirrelFunc("decode", decode_jwt, 3, ".ss")
    /**
    @param jwt_string s
    @param key s
    @return t : return decoded jwt table with {header=<json_value>, payload=<jsonvalue>} or {error=string}

    Error can be one of following::

      "OK",
      "INVALID_TOKEN",
      "WRONG_HEADER",
      "INVALID_TOKEN_TYPE",
      "INVALID_TOKEN_SIGNATURE_ALGORITHM",
      "INVALID_PAYLOAD_COMPRESSION",
      "WRONG_PAYLOAD",
      "SIGNATURE_VERIFICATION_FAILED"
    */
    ;
  module_mgr->addNativeModule("jwt", nsTbl);
}
} // namespace bindquirrel
