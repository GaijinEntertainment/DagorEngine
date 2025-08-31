// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include <quirrel/sqModules/sqModules.h>
#include <hash/md5.h>
#include <hash/sha1.h>
#include <hash/sha256.h>
#include <hash/crc32.h>
#include <util/dag_strUtil.h>
#include <osApiWrappers/dag_files.h>
#include <uuidv7/uuidv7.h>


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

static String calculate_sha256_and_convert_hex_to_string(const unsigned char *data, int data_length)
{
  sha256_context sha256ctx;

  static constexpr int HASH_SZ = 32;
  unsigned char sha256_output[HASH_SZ] = {0};

  sha256_starts(&sha256ctx);
  sha256_update(&sha256ctx, data, data_length);
  sha256_finish(&sha256ctx, sha256_output);

  String sha256_str;
  data_to_str_hex(sha256_str, sha256_output, sizeof(sha256_output));
  return sha256_str;
}

static SQInteger sq_sha256(HSQUIRRELVM vm)
{
  const char *dataStr;
  SQInteger dataStrLen;
  sq_getstringandsize(vm, 2, &dataStr, &dataStrLen);

  String sha256_str = calculate_sha256_and_convert_hex_to_string((const unsigned char *)dataStr, dataStrLen);
  sq_pushstring(vm, sha256_str, sha256_str.length());
  return 1;
}

// compute sha256 for given file_name
static SQInteger sq_file_sha256(HSQUIRRELVM vm)
{
  const char *fileName = nullptr;
  sq_getstring(vm, 2, &fileName);

  file_ptr_t file = df_open(fileName, DF_READ);
  if (!file)
  {
    return sq_throwerror(vm, String(0, "Failed to open file '%s'", fileName));
  }
  int fileLength = 0;
  const void *fileData = df_mmap(file, &fileLength);
  if (!fileData)
  {
    df_close(file);
    return sq_throwerror(vm, String(0, "Failed to read file '%s'", fileName));
  }
  String sha256_str = calculate_sha256_and_convert_hex_to_string((const unsigned char *)fileData, fileLength);
  df_unmap(fileData, fileLength);
  df_close(file);

  sq_pushstring(vm, sha256_str, sha256_str.length());
  return 1;
}

static uint32_t calc_crc32_for_sq_params(HSQUIRRELVM vm)
{
  const char *dataStr;
  SQInteger dataStrLen;
  sq_getstringandsize(vm, 2, &dataStr, &dataStrLen);
  SQInteger initSum = 0xFFFFFFFF;
  if (sq_gettop(vm) > 2)
    sq_getinteger(vm, 3, &initSum);
  return calc_crc32((const unsigned char *)dataStr, dataStrLen, initSum);
}

static SQInteger sq_crc32(HSQUIRRELVM vm)
{
  uint32_t crc32_output = calc_crc32_for_sq_params(vm);

  String crc32_str;
  crc32_str.printf(8, "%08x", crc32_output);

  sq_pushstring(vm, crc32_str, crc32_str.length());
  return 1;
}

static SQInteger sq_crc32_int(HSQUIRRELVM vm)
{
  uint32_t crc32_output = calc_crc32_for_sq_params(vm);

  sq_pushinteger(vm, crc32_output);
  return 1;
}

static SQInteger sq_create_guid(HSQUIRRELVM vm)
{
  uint8_t uuid[16];
  uuidv7(uuid);

  char uuid_str[UUID_STRING_BUFFER_LENGTH];
  uuidv7_snprintf(uuid, uuid_str, sizeof(uuid_str));

  sq_pushstring(vm, uuid_str, UUID_STRING_BUFFER_LENGTH - 1);
  return 1;
}

void register_hash(SqModules *module_mgr)
{
  Sqrat::Table exports(module_mgr->getVM());

  ///@module hash
  exports //
    .SquirrelFunc("md5", sq_md5, 2, ".s")
    .SquirrelFunc("sha1", sq_sha1, 2, ".s")
    .SquirrelFunc("sha256", sq_sha256, 2, ".s")
    .SquirrelFunc("file_sha256", sq_file_sha256, 2, ".s")
    .SquirrelFunc("crc32", sq_crc32, -2, ".si")
    .SquirrelFunc("crc32_int", sq_crc32_int, -2, ".si")
    .SquirrelFunc("create_guid", sq_create_guid, 1, ".s")
    /**/;

  module_mgr->addNativeModule("hash", exports);
}


} // namespace bindquirrel
