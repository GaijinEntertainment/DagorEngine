// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include <jsonUtils/jsonUtils.h>
#include <jsonUtils/decodeJwt.h>

#include <ioSys/dag_zlibIo.h>
#include <ioSys/dag_zstdIo.h>
#include <ioSys/dag_memIo.h>
#include <b64/cdecode.h>
#include <digitalSignature/digitalSignatureCheck.h>
#include <EASTL/string.h>
#include <memory/dag_framemem.h>
#include <rapidjson/document.h>
#include <rapidjson/writer.h>
#include <rapidJsonUtils/rapidJsonUtils.h>


typedef eastl::basic_string<char, framemem_allocator> TmpStr;

static const char *const ZIP_HDR_ZLIB = "DEF";
static const char *const ZIP_HDR_ZSTD = "ZSTD";


static bool convert_base64url_decode(TmpStr &result, const char *asyms = nullptr)
{
  for (char &ch : result)
  {
    if ((ch >= 'A' && ch <= 'Z') || (ch >= 'a' && ch <= 'z') || (ch >= '0' && ch <= '9') || ch == '+' || ch == '/' || ch == '=')
      ;
    else if (ch == '-')
      ch = '+';
    else if (ch == '_')
      ch = '/';
    else if (asyms && strchr(asyms, ch))
      ;
    else
      return false;
  }
  return true;
}

static bool decode_b64(const char *ptr, int size, TmpStr &res, const char *asyms = nullptr)
{
  TmpStr convertedBlock(ptr, size);
  if (!convert_base64url_decode(convertedBlock, asyms))
    return false;
  base64_decodestate b64st;
  base64_init_decodestate(&b64st);
  res.resize(convertedBlock.size());
  int len = base64_decode_block((const int8_t *)convertedBlock.c_str(), convertedBlock.size(), (int8_t *)res.begin(), &b64st);
  res.resize(len);
  return true;
}


static inline bool decompress_jwt_block(IGenLoad &load, TmpStr &dst)
{
  char buf[4096];
  int s = load.tryRead(buf, sizeof(buf));
  for (; s > 0; s = load.tryRead(buf, sizeof(buf)))
    dst.append(buf, s);
  load.ceaseReading();
  return s >= 0;
}


static inline bool decode_jwt_block_internal(TmpStr &jsonStr, const char *ptr, int size, jsonutils::JwtCompressionType comprType)
{
  if (comprType == jsonutils::JwtCompressionType::NONE)
    return decode_b64(ptr, size, jsonStr);

  TmpStr payloadStr;
  if (!decode_b64(ptr, size, payloadStr))
    return false;
  InPlaceMemLoadCB crd(payloadStr.c_str(), (int)payloadStr.size());
  jsonStr.reserve(payloadStr.size() * 10);
  bool result = false;
  const char *comprName = "unknown";
  if (comprType == jsonutils::JwtCompressionType::ZLIB)
  {
    comprName = "zlib";
    ZlibLoadCB zlibLoad(crd, (int)payloadStr.size(), /*raw_inflate*/ false, /*fatal_errors*/ false);
    result = decompress_jwt_block(zlibLoad, jsonStr);
  }
  else if (comprType == jsonutils::JwtCompressionType::ZSTD)
  {
    comprName = "zstd";
    ZstdLoadCB zstdLoad(crd, (int)payloadStr.size());
    result = decompress_jwt_block(zstdLoad, jsonStr);
  }
  if (!result)
  {
    logerr("%s (%s) failed to decompress base64 encoded block of %d bytes:\n%.*s", __FUNCTION__, comprName, size, min(size, 16 << 10),
      ptr);
    return false;
  }
  return true;
}

bool jsonutils::decode_jwt_block(const char *ptr, int size, rapidjson::Document &json, JwtCompressionType comprType)
{
  TmpStr jsonStr;
  if (!decode_jwt_block_internal(jsonStr, ptr, size, comprType))
    return false;
  json.Parse(jsonStr.c_str(), jsonStr.size());
  return !json.HasParseError() && json.IsObject(); // JWT token is expected to be an object (json table)
}

static bool verify_signature(const char *data, int size, const char *sign, const char *publicKey)
{
  TmpStr signBin, publicKeyDer;
  if (!decode_b64(sign, strlen(sign), signBin))
    return false;
  if (!decode_b64(publicKey, strlen(publicKey), publicKeyDer, "\r\n \t"))
    return false;

  const void *buffers[2] = {data, nullptr};
  unsigned bufferSizes[2] = {unsigned(size), 0};

  return verify_digital_signature_with_key_and_algo("SHA256", buffers, bufferSizes, (const unsigned char *)signBin.c_str(),
    signBin.size(), (const unsigned char *)publicKeyDer.c_str(), publicKeyDer.size());
}

jsonutils::DecodeJwtResult jsonutils::decode_jwt(const char *token, const char *publicKey, rapidjson::Document &headerJson,
  rapidjson::Document &payloadJson)
{
  const char *payload = strchr(token, '.');
  if (!payload)
    return DecodeJwtResult::INVALID_TOKEN;

  const char *sign = strchr(++payload, '.');
  if (!sign)
    return DecodeJwtResult::INVALID_TOKEN;

  ++sign;

  if (!decode_jwt_block(token, payload - token - 1, headerJson) || !headerJson.IsObject())
    return DecodeJwtResult::WRONG_HEADER;

  if (strcmp(jsonutils::get_or(headerJson, "typ", ""), "JWT") != 0)
    return DecodeJwtResult::INVALID_TOKEN_TYPE;

  if (strcmp(jsonutils::get_or(headerJson, "alg", ""), "RS256") != 0)
    return DecodeJwtResult::INVALID_TOKEN_SIGNATURE_ALGORITHM;

  JwtCompressionType comprType = JwtCompressionType::NONE;
  if (const char *zip = jsonutils::get_or<const char *>(headerJson, "zip", nullptr))
  {
    if (strcmp(zip, ZIP_HDR_ZLIB) == 0)
      comprType = JwtCompressionType::ZLIB;
    else if (strcmp(zip, ZIP_HDR_ZSTD) == 0)
      comprType = JwtCompressionType::ZSTD;
    else
      return DecodeJwtResult::INVALID_PAYLOAD_COMPRESSION;
  }

  if (!decode_jwt_block(payload, sign - payload - 1, payloadJson, comprType))
    return DecodeJwtResult::WRONG_PAYLOAD;

  if (!verify_signature(token, sign - token - 1, sign, publicKey))
    return DecodeJwtResult::SIGNATURE_VERIFICATION_FAILED;

  return DecodeJwtResult::OK;
}
