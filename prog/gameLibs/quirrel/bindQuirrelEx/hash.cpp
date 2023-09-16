#include <quirrel/sqModules/sqModules.h>
#include <hash/md5.h>
#include <hash/sha1.h>
#include <util/dag_strUtil.h>


namespace bindquirrel
{

static SQInteger sq_md5(HSQUIRRELVM vm)
{
  const char *dataStr;
  SQInteger dataStrLen;
  sq_getstringandsize(vm, 2, &dataStr, &dataStrLen);

  md5_state_t md5s;
  ::md5_init(&md5s);

  static constexpr int HASH_SZ = 16;
  unsigned char out_hash[HASH_SZ] = {0};

  ::md5_append(&md5s, (md5_byte_t *)dataStr, dataStrLen);
  ::md5_finish(&md5s, out_hash);

  String md5_str;
  data_to_str_hex(md5_str, out_hash, sizeof(out_hash));

  sq_pushstring(vm, md5_str, md5_str.length());
  return 1;
}

static SQInteger sq_sha1(HSQUIRRELVM vm)
{
  const char *dataStr;
  SQInteger dataStrLen;
  sq_getstringandsize(vm, 2, &dataStr, &dataStrLen);

  sha1_context sha1ctx;
  ::sha1_starts(&sha1ctx);

  static constexpr int HASH_SZ = 20;
  unsigned char sha1_output[HASH_SZ] = {0};

  sha1_starts(&sha1ctx);
  sha1_update(&sha1ctx, (const unsigned char *)dataStr, dataStrLen);
  sha1_finish(&sha1ctx, sha1_output);

  String sha1_str;
  data_to_str_hex(sha1_str, sha1_output, sizeof(sha1_output));

  sq_pushstring(vm, sha1_str, sha1_str.length());
  return 1;
}

void register_hash(SqModules *module_mgr)
{
  Sqrat::Table exports(module_mgr->getVM());

  ///@module hash
  exports // comments to supress clang-format and allow qdox to generate doc
    .SquirrelFunc("md5", sq_md5, 2, ".s")
    .SquirrelFunc("sha1", sq_sha1, 2, ".s");

  module_mgr->addNativeModule("hash", exports);
}


} // namespace bindquirrel
