#include <quirrel/base64/base64.h>

#include <util/dag_base64.h>
#include <util/dag_string.h>
#include <ioSys/dag_dataBlockUtils.h>
#include <quirrel_json/quirrel_json.h>
#include <EASTL/string.h>

#include <sqModules/sqModules.h>
#include <sqstdblob.h>


static eastl::string encode_base64(const char *text)
{
  G_ASSERT(text);

  Base64 b64Coder;
  b64Coder.encode((const uint8_t *)text, strlen(text));
  return {b64Coder.c_str()};
}

static eastl::string decode_base64(const char *text)
{
  Base64 b64Coder(text);
  String result;
  b64Coder.decode(result);
  return {result.str()};
}

static eastl::string obj_to_base64(Sqrat::Object obj)
{
  eastl::string jsonStr = quirrel_to_jsonstr(obj);

  Base64 b64Coder;
  b64Coder.encode((const uint8_t *)jsonStr.data(), (int)jsonStr.size());
  return {b64Coder.c_str()};
}

static eastl::string blk_to_base64(const DataBlock *blk) { return {::pack_blk_to_base64(*blk).str()}; }


static SQInteger encode_blob(HSQUIRRELVM vm)
{
  SQUserPointer data = nullptr;
  SQInteger size = -1;
  if (SQ_FAILED(sqstd_getblob(vm, 2, &data)) || (size = sqstd_getblobsize(vm, 2)) < 0)
    return sq_throwerror(vm, "Invalid blob");

  Base64 b64Coder;
  b64Coder.encode((const uint8_t *)data, size);
  sq_pushstring(vm, b64Coder.c_str(), -1);
  return 1;
}


namespace bindquirrel
{

void bind_base64_utils(SqModules *mgr)
{
  ///@module base64
  Sqrat::Table b64(mgr->getVM());
  b64.Func("encodeString", encode_base64)
    .Func("decodeString", decode_base64)
    .Func("encodeJson", obj_to_base64)
    .Func("encodeBlk", blk_to_base64)
    .SquirrelFunc("encodeBlob", encode_blob, 2, ".x");

  mgr->addNativeModule("base64", b64);
}

} // namespace bindquirrel
