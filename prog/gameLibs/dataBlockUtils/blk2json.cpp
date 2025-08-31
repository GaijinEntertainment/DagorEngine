// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include <dataBlockUtils/blk2json.h>

#include <ioSys/dag_dataBlock.h>
#include <json/json.h>
#include <math/dag_math3d.h>
#include <math/dag_e3dColor.h>
#include <util/dag_string.h>
#include <memory/dag_framemem.h>
#include <math/integer/dag_IPoint4.h>

static bool check_scheme_block_match(const char *expr, const char *value)
{
  // probably it worth using real regexp
  if (strcmp(expr, "*") == 0)
    return true;

  const char *p = expr;
  size_t l = strlen(value);
  for (const char *s = strstr(p, value); s; s = strstr(p, value))
  {
    if ((s == expr || s[-1] == '|') && (s[l] == '|' || s[l] == 0))
      return true;
    p += l;
  }
  return false;
}


bool blk_to_json(const DataBlock &blk, Json::Value &json, const DataBlock &scheme)
{
  for (int iSubBlock = 0, nSubBlocks = blk.blockCount(); iSubBlock < nSubBlocks; ++iSubBlock)
  {
    const DataBlock *b = blk.getBlock(iSubBlock);
    const char *name = b->getBlockName();

    const DataBlock *childScheme = NULL;
    for (int iSchemeSub = 0, nSchemeSubs = scheme.blockCount(); iSchemeSub < nSchemeSubs; ++iSchemeSub)
    {
      const DataBlock *sb = scheme.getBlock(iSchemeSub);
      if (check_scheme_block_match(sb->getBlockName(), name))
      {
        childScheme = sb;
        break;
      }
    }
    if (!childScheme)
      childScheme = &DataBlock::emptyBlock;
    // DEBUG_CTX("'%s' subblock scheme = '%s'", name, childScheme->getBlockName());

    Json::Value child;
    if (!blk_to_json(*b, child, *childScheme))
      return false;

    const char *jsonArrayName = childScheme->getStr("_json_array_name", NULL);
    if (jsonArrayName)
    {
      Json::Value &arr = json[jsonArrayName];
      child["_block_name"] = name;
      arr[arr.size()] = child;
    }
    else if (json.isMember(name))
    {
      LOGERR_CTX("Duplicate block %s", name);
      return false;
    }
    else
    {
      json[name] = child;
    }
  }

  for (int iParam = 0, nParams = blk.paramCount(); iParam < nParams; ++iParam)
  {
    const char *name = blk.getParamName(iParam);
    Json::Value child;

    int tp = blk.getParamType(iParam);
    const char *typeSuffix = NULL;
    switch (tp)
    {
      case DataBlock::TYPE_STRING:
        child = blk.getStr(iParam);
        typeSuffix = ":t";
        break;
      case DataBlock::TYPE_INT:
        child = blk.getInt(iParam);
        typeSuffix = ":i";
        break;
      case DataBlock::TYPE_INT64:
        child = Json::Int64(blk.getInt64(iParam));
        typeSuffix = ":i64";
        break;
      case DataBlock::TYPE_REAL:
        child = blk.getReal(iParam);
        typeSuffix = ":r";
        break;
      case DataBlock::TYPE_POINT2:
      {
        Point2 p2 = blk.getPoint2(iParam);
        child[1] = p2.y;
        child[0] = p2.x;
        typeSuffix = ":p2";
        break;
      }
      case DataBlock::TYPE_POINT3:
      {
        Point3 p3 = blk.getPoint3(iParam);
        child[2] = p3.z;
        child[1] = p3.y;
        child[0] = p3.x;
        typeSuffix = ":p3";
        break;
      }
      case DataBlock::TYPE_POINT4:
      {
        Point4 p4 = blk.getPoint4(iParam);
        child[3] = p4.w;
        child[2] = p4.z;
        child[1] = p4.y;
        child[0] = p4.x;
        typeSuffix = ":p4";
        break;
      }
      case DataBlock::TYPE_IPOINT2:
      {
        IPoint2 ip2 = blk.getIPoint2(iParam);
        child[1] = ip2.y;
        child[0] = ip2.x;
        typeSuffix = ":ip2";
        break;
      }
      case DataBlock::TYPE_IPOINT3:
      {
        IPoint3 ip3 = blk.getIPoint3(iParam);
        child[2] = ip3.z;
        child[1] = ip3.y;
        child[0] = ip3.x;
        typeSuffix = ":ip3";
        break;
      }
      case DataBlock::TYPE_IPOINT4:
      {
        IPoint4 ip4 = blk.getIPoint4(iParam);
        child[3] = ip4.w;
        child[2] = ip4.z;
        child[1] = ip4.y;
        child[0] = ip4.x;
        typeSuffix = ":ip4";
        break;
      }
      case DataBlock::TYPE_BOOL:
        child = blk.getBool(iParam);
        typeSuffix = ":b";
        break;
      case DataBlock::TYPE_E3DCOLOR:
      {
        E3DCOLOR c = blk.getE3dcolor(iParam);
        child[3] = c.a;
        child[2] = c.b;
        child[1] = c.g;
        child[0] = c.r;
        typeSuffix = ":c";
        break;
      }
      case DataBlock::TYPE_MATRIX:
      {
        TMatrix m = blk.getTm(iParam);
        for (int col = 0; col < 4; ++col)
        {
          Json::Value j;
          Point3 p3 = m.getcol(col);
          j[2] = p3.z;
          j[1] = p3.y;
          j[0] = p3.x;
          child[col] = j;
        }
        typeSuffix = ":m";
        break;
      }
      default: G_ASSERT(!"Unexpected type"); return false;
    }

    String fullName(128, "%s%s", name, typeSuffix);

    bool isArray = scheme.findParam(name) >= 0;

    if (isArray)
    {
      Json::Value &arr = json[fullName.str()];
      arr[arr.size()] = child;
    }
    else if (json.isMember(fullName))
    {
      LOGERR_CTX("Duplicate param '%s' in block '%s'", fullName.str(), blk.getBlockName());
      // G_ASSERT(!"Child param already exists");
      return false;
    }
    else
      json[fullName] = child;
  }

  return true;
}


static DataBlock::ParamType get_blk_param_type(const char *name, size_t len, const char **suffix)
{
#define CHECK_PTYPE(s, t)      \
  if (strcmp(*suffix, s) == 0) \
    return DataBlock::t;

  if (len >= 2)
  {
    *suffix = name + len - 2;
    CHECK_PTYPE(":t", TYPE_STRING);
    CHECK_PTYPE(":i", TYPE_INT);
    CHECK_PTYPE(":r", TYPE_REAL);
    CHECK_PTYPE(":b", TYPE_BOOL);
    CHECK_PTYPE(":c", TYPE_E3DCOLOR);
    CHECK_PTYPE(":m", TYPE_MATRIX);
  }

  if (len >= 3)
  {
    *suffix = name + len - 3;
    CHECK_PTYPE(":p2", TYPE_POINT2);
    CHECK_PTYPE(":p3", TYPE_POINT3);
    CHECK_PTYPE(":p4", TYPE_POINT4);
  }

  if (len >= 4)
  {
    *suffix = name + len - 4;
    CHECK_PTYPE(":ip2", TYPE_IPOINT2);
    CHECK_PTYPE(":ip3", TYPE_IPOINT3);
    CHECK_PTYPE(":ip4", TYPE_IPOINT4);
    CHECK_PTYPE(":i64", TYPE_INT64);
  }

#undef CHECK_PTYPE

  return DataBlock::TYPE_NONE;
}


static bool add_blk_param_from_json(DataBlock &blk, const Json::Value &key, const Json::Value &value)
{
  const char *strKey = key.asCString();
  size_t keyLen = strlen(strKey);
  String blkKey;

  const char *parTypeSuffix = NULL;
  DataBlock::ParamType blkParamType = get_blk_param_type(strKey, keyLen, &parTypeSuffix);

  if (blkParamType == DataBlock::TYPE_STRING)
  {
    if (value.type() != Json::stringValue)
    {
      LOGERR_CTX("%s type = %d", strKey, value.type());
      G_ASSERT(!"Invalid type for string");
      return false;
    }
    blkKey = strKey;
    blkKey[keyLen - 2] = 0;
    blk.addStr(blkKey, value.asCString());
  }
  else if (blkParamType == DataBlock::TYPE_INT)
  {
    if (value.type() != Json::intValue && value.type() != Json::uintValue && value.type() != Json::realValue)
    {
      G_ASSERT(!"Invalid type for int");
      return false;
    }
    blkKey = strKey;
    blkKey[keyLen - 2] = 0;
    blk.addInt(blkKey, value.asInt());
  }
  else if (blkParamType == DataBlock::TYPE_INT64)
  {
    if (value.type() != Json::intValue && value.type() != Json::uintValue && value.type() != Json::realValue)
    {
      G_ASSERT(!"Invalid type for int64");
      return false;
    }
    blkKey = strKey;
    blkKey[keyLen - 2] = 0;
    blk.addInt(blkKey, value.asInt64());
  }
  else if (blkParamType == DataBlock::TYPE_REAL)
  {
    if (value.type() != Json::intValue && value.type() != Json::uintValue && value.type() != Json::realValue)
    {
      G_ASSERT(!"Invalid type for real");
      return false;
    }
    blkKey = strKey;
    blkKey[keyLen - 2] = 0;
    blk.addReal(blkKey, value.asFloat());
  }
  else if (blkParamType == DataBlock::TYPE_BOOL)
  {
    if (value.type() != Json::booleanValue)
    {
      G_ASSERT(!"Invalid type for boolean");
      return false;
    }
    blkKey = strKey;
    blkKey[keyLen - 2] = 0;
    blk.addBool(blkKey, value.asBool());
  }
  else if (blkParamType == DataBlock::TYPE_POINT2)
  {
    if (value.type() != Json::arrayValue || value.size() != 2)
    {
      G_ASSERT(!"Invalid type for Point2");
      return false;
    }
    blkKey = strKey;
    blkKey[keyLen - 3] = 0;
    blk.addPoint2(blkKey, Point2(value[0].asFloat(), value[1].asFloat()));
  }
  else if (blkParamType == DataBlock::TYPE_POINT3)
  {
    if (value.type() != Json::arrayValue || value.size() != 3)
    {
      G_ASSERT(!"Invalid type for Point3");
      return false;
    }
    blkKey = strKey;
    blkKey[keyLen - 3] = 0;
    blk.addPoint3(blkKey, Point3(value[0].asFloat(), value[1].asFloat(), value[2].asFloat()));
  }
  else if (blkParamType == DataBlock::TYPE_POINT4)
  {
    if (value.type() != Json::arrayValue || value.size() != 4)
    {
      G_ASSERT(!"Invalid type for Point4");
      return false;
    }
    blkKey = strKey;
    blkKey[keyLen - 3] = 0;
    blk.addPoint4(blkKey, Point4(value[0].asFloat(), value[1].asFloat(), value[2].asFloat(), value[3].asFloat()));
  }
  else if (blkParamType == DataBlock::TYPE_IPOINT2)
  {
    if (value.type() != Json::arrayValue || value.size() != 2)
    {
      G_ASSERT(!"Invalid type for IPoint2");
      return false;
    }
    blkKey = strKey;
    blkKey[keyLen - 4] = 0;
    blk.addIPoint2(blkKey, IPoint2(value[0].asInt(), value[1].asInt()));
  }
  else if (blkParamType == DataBlock::TYPE_IPOINT3)
  {
    if (value.type() != Json::arrayValue || value.size() != 3)
    {
      G_ASSERT(!"Invalid type for IPoint3");
      return false;
    }
    blkKey = strKey;
    blkKey[keyLen - 4] = 0;
    blk.addIPoint3(blkKey, IPoint3(value[0].asInt(), value[1].asInt(), value[2].asInt()));
  }
  else if (blkParamType == DataBlock::TYPE_IPOINT4)
  {
    if (value.type() != Json::arrayValue || value.size() != 4)
    {
      G_ASSERT(!"Invalid type for IPoint4");
      return false;
    }
    blkKey = strKey;
    blkKey[keyLen - 4] = 0;
    blk.addIPoint4(blkKey, IPoint4(value[0].asInt(), value[1].asInt(), value[2].asInt(), value[3].asInt()));
  }
  else if (blkParamType == DataBlock::TYPE_E3DCOLOR)
  {
    if (value.type() != Json::arrayValue || (value.size() != 3 && value.size() != 4))
    {
      G_ASSERT(!"Invalid type for E3DCOLOR");
      return false;
    }
    blkKey = strKey;
    blkKey[keyLen - 2] = 0;
    int a = (value.size() != 4) ? value[3].asInt() : 255;
    blk.addE3dcolor(blkKey, E3DCOLOR(value[0].asInt(), value[1].asInt(), value[2].asInt(), a));
  }
  else if (blkParamType == DataBlock::TYPE_MATRIX)
  {
    if (value.type() != Json::arrayValue || value.size() != 4)
    {
      DEBUG_CTX("%s", value.toStyledString().c_str());
      DEBUG_CTX("key = %s, type = %d, size = %d", strKey, value.type(), value.size());
      G_ASSERT(!"Invalid type for TMatrix");
      return false;
    }
    TMatrix tm;
    for (int col = 0; col < 4; ++col)
    {
      const Json::Value &c = value[col];
      if (c.type() != Json::arrayValue || c.size() != 3)
      {
        DEBUG_CTX("type = %d, size = %d", c.type(), c.size());
        G_ASSERT(!"Invalid col type for TMatrix");
        return false;
      }

      tm.setcol(col, Point3(c[0].asFloat(), c[1].asFloat(), c[2].asFloat()));
    }
    blkKey = strKey;
    blkKey[keyLen - 2] = 0;
    blk.addTm(blkKey, tm);
  }
  else
  {
    LOGERR_CTX("Unknown type of parameter '%s'", key.asCString());
    G_ASSERTF(0, "Unknown parameter type");
    return false;
  }
  return true;
}


bool json_to_blk(const Json::Value &json, DataBlock &blk, const DataBlock &scheme, int nest_level)
{
  for (Json::ValueConstIterator it = json.begin(); it != json.end(); ++it)
  {
    const Json::Value &key = it.key();
    const Json::Value &value = *it;

    if (strcmp(key.asCString(), "_block_name") == 0)
      continue;

    if (!key.isString())
    {
      LOGERR_CTX("JSON key %s is not a string", key.toStyledString().c_str());
      return false;
    }

    switch (value.type())
    {
      case Json::nullValue:
      {
        blk.addNewBlock(key.asCString());
        break;
      }

      case Json::objectValue:
      {
        DataBlock *b = blk.addNewBlock(key.asCString());
        const DataBlock *childScheme = scheme.getBlockByNameEx(key.asCString());
        // DEBUG_CTX("{%d} scheme for %s %s", nest_level, key.asCString(), childScheme!=&DataBlock::emptyBlock ? "found" : "not
        // found");
        if (!json_to_blk(value, *b, *childScheme, nest_level + 1))
        {
          LOGERR_CTX("%s subblock conversion failed", key.asCString());
          return false;
        }
        break;
      }

      case Json::arrayValue:
      {
        const char *parTypeSuffix = NULL;
        const char *strKey = key.asCString();
        DataBlock::ParamType paramType = get_blk_param_type(strKey, strlen(strKey), &parTypeSuffix);

        String strPureKey;
        strPureKey.setStr(strKey, parTypeSuffix - strKey);
        bool isParamArray = (paramType != DataBlock::TYPE_NONE) && (scheme.findParam(strPureKey) >= 0);

        if (paramType != DataBlock::TYPE_NONE && !isParamArray)
        {
          if (!add_blk_param_from_json(blk, key, value))
          {
            LOGERR_CTX("add_blk_param_from_json() failed");
            return false;
          }
        }
        else
        {
          const DataBlock *childScheme = NULL;
          for (int iSchemeBlock = 0, nSchemeBlocks = scheme.blockCount(); iSchemeBlock < nSchemeBlocks; ++iSchemeBlock)
          {
            const DataBlock *b = scheme.getBlock(iSchemeBlock);
            const char *arrName = b->getStr("_json_array_name", NULL);
            if (arrName && strcmp(arrName, key.asCString()) == 0)
            {
              childScheme = b;
              break;
            }
          }
          if (!childScheme)
            childScheme = &DataBlock::emptyBlock;
          // DEBUG_CTX("[%d] scheme for %s %s", nest_level, key.asCString(), childScheme!=&DataBlock::emptyBlock ? "found" : "not
          // found");

          for (size_t iItem = 0, nItems = value.size(); iItem < nItems; ++iItem)
          {
            const Json::Value &item = value[Json::ArrayIndex(iItem)];
            if (isParamArray)
            {
              if (!add_blk_param_from_json(blk, key, item))
              {
                LOGERR_CTX("add_blk_param_from_json() failed");
                return false;
              }
            }
            else
            {
              if (!item.isObject())
              {
                LOGERR_CTX("Object expected for subblock '%s'", key.asCString());
                return false;
              }
              if (!item.isMember("_block_name"))
              {
                LOGERR_CTX("No _block_name specified");
                return false;
              }
              const char *childName = item["_block_name"].asCString();
              DataBlock *blkChild = blk.addNewBlock(childName);
              if (!json_to_blk(item, *blkChild, *childScheme, nest_level + 1))
              {
                LOGERR_CTX("Child block '%s'/'%s' conversion failed", key.asCString(), childName);
                return false;
              }
            }
          }
        }
        break;
      }

      default:
        if (!add_blk_param_from_json(blk, key, value))
        {
          LOGERR_CTX("add_blk_param_from_json() failed");
          return false;
        }
        break;
    }
  }

  return true;
}


static const char *extract_key_and_type_hint(const char *key, DataBlock::ParamType &typeHint)
{
  typeHint = DataBlock::TYPE_NONE;

  const char *colonPos = strrchr(key, ':');
  if (!colonPos)
    return key;

  ptrdiff_t keyLen = colonPos - key;
  G_ASSERT(keyLen >= 0);
  if (!keyLen)
    return key;

  const char *suffixDummy = nullptr;
  typeHint = get_blk_param_type(colonPos, strlen(colonPos), &suffixDummy);
  if (typeHint == DataBlock::TYPE_NONE)
    return key;

  char *keyWoType = (char *)framemem_ptr()->alloc(keyLen + 1);
  strncpy(keyWoType, key, keyLen);
  keyWoType[keyLen] = 0;
  return keyWoType;
}


static bool is_json_array_of_numbers(Json::Value const &val, size_t expectedSize)
{
  if (val.size() != expectedSize)
    return false;
  for (size_t i = 0; i < expectedSize; i++)
  {
    const Json::Value &subVal = val[Json::ArrayIndex(i)];
    if (!subVal.isIntegral() && !subVal.isDouble())
      return false;
  }
  return true;
}


static bool is_json_array_a_matrix(Json::Value const &val)
{
  if (val.size() != 4)
    return false;
  for (int i = 0; i < 4; i++)
  {
    const Json::Value &subVal = val[Json::ArrayIndex(i)];
    if (!subVal.isArray())
      return false;
    if (!is_json_array_of_numbers(subVal, 3))
      return false;
  }
  return true;
}


void json_table_extract_value(DataBlock &obj, const char *key, Json::Value const &val);

static void json_table_extract_array_value(DataBlock &obj, const char *key, const char *keyWoType, DataBlock::ParamType typeHint,
  Json::Value const &val)
{
  if (is_json_array_of_numbers(val, 2))
  {
    if (typeHint == DataBlock::TYPE_POINT2)
    {
      Point2 p2;
      p2.x = (float)val[0].asDouble();
      p2.y = (float)val[1].asDouble();
      obj.addPoint2(keyWoType, p2);
      return;
    }

    if (typeHint == DataBlock::TYPE_IPOINT2)
    {
      IPoint2 ip2;
      ip2.x = (int)val[0].asLargestInt();
      ip2.y = (int)val[1].asLargestInt();
      obj.addIPoint2(keyWoType, ip2);
      return;
    }
  }
  else if (is_json_array_of_numbers(val, 3))
  {
    if (typeHint == DataBlock::TYPE_POINT3)
    {
      Point3 p3;
      p3.x = (float)val[0].asDouble();
      p3.y = (float)val[1].asDouble();
      p3.z = (float)val[2].asDouble();
      obj.addPoint3(keyWoType, p3);
      return;
    }
    if (typeHint == DataBlock::TYPE_IPOINT3)
    {
      IPoint3 ip3;
      ip3.x = (int)val[0].asLargestInt();
      ip3.y = (int)val[1].asLargestInt();
      ip3.z = (int)val[2].asLargestInt();
      obj.addIPoint3(keyWoType, ip3);
      return;
    }
    if (typeHint == DataBlock::TYPE_IPOINT4)
    {
      IPoint4 ip4;
      ip4.x = (int)val[0].asLargestInt();
      ip4.y = (int)val[1].asLargestInt();
      ip4.z = (int)val[2].asLargestInt();
      ip4.w = (int)val[3].asLargestInt();
      obj.addIPoint4(keyWoType, ip4);
      return;
    }
  }
  else if (is_json_array_of_numbers(val, 4))
  {
    if (typeHint == DataBlock::TYPE_POINT4)
    {
      Point4 p4;
      p4.x = (float)val[0].asDouble();
      p4.y = (float)val[1].asDouble();
      p4.z = (float)val[2].asDouble();
      p4.w = (float)val[3].asDouble();
      obj.addPoint4(keyWoType, p4);
      return;
    }

    if (typeHint == DataBlock::TYPE_E3DCOLOR)
    {
      E3DCOLOR c;
      c.r = (unsigned char)val[0].asLargestUInt();
      c.g = (unsigned char)val[1].asLargestUInt();
      c.b = (unsigned char)val[2].asLargestUInt();
      c.a = (unsigned char)val[3].asLargestUInt();
      obj.addE3dcolor(keyWoType, c);
      return;
    }
  }
  else if (typeHint == DataBlock::TYPE_MATRIX && is_json_array_a_matrix(val))
  {
    TMatrix m;
    for (int i = 0; i < 4; i++)
    {
      const Json::Value &subVal = val[Json::ArrayIndex(i)];
      Point3 p3;
      p3.x = (float)subVal[0].asDouble();
      p3.y = (float)subVal[1].asDouble();
      p3.z = (float)subVal[2].asDouble();
      m.setcol(i, p3);
    }
    obj.addTm(keyWoType, m);
    return;
  }

  const size_t nItems = val.size();
  for (size_t i = 0; i < nItems; ++i)
    json_table_extract_value(obj, key, val[Json::ArrayIndex(i)]);
}


static void json_table_extract_object_value(DataBlock &obj, const char *key, const char *keyWoType, DataBlock::ParamType typeHint,
  Json::Value const &val)
{
  // this format is used by charserver's blk_to_bson_strict converter
  if (val.size() == 1)
  {
    Json::ValueConstIterator it = val.begin();
    const Json::Value &subValue = *it;
    bool canExtractAsComplexObj = false;
    switch (typeHint)
    {
      case DataBlock::TYPE_POINT2:
      case DataBlock::TYPE_IPOINT2: canExtractAsComplexObj = is_json_array_of_numbers(subValue, 2); break;
      case DataBlock::TYPE_POINT3:
      case DataBlock::TYPE_IPOINT3: canExtractAsComplexObj = is_json_array_of_numbers(subValue, 3); break;
      case DataBlock::TYPE_IPOINT4:
      case DataBlock::TYPE_POINT4: canExtractAsComplexObj = is_json_array_of_numbers(subValue, 4); break;
      default: break;
    }
    if (canExtractAsComplexObj)
    {
      json_table_extract_array_value(obj, key, keyWoType, typeHint, subValue);
      return;
    }
  }

  DataBlock *child = obj.addNewBlock(key);
  json_to_blk(val, *child);
}


void json_table_extract_value(DataBlock &obj, const char *key, Json::Value const &val)
{
  DataBlock::ParamType typeHint = DataBlock::TYPE_NONE;
  const char *keyWoType = extract_key_and_type_hint(key, typeHint);

  if (val.isObject())
  {
    json_table_extract_object_value(obj, key, keyWoType, typeHint, val);
  }
  else if (val.isArray())
  {
    json_table_extract_array_value(obj, key, keyWoType, typeHint, val);
  }
  else if (val.isBool())
  {
    if (typeHint == DataBlock::TYPE_BOOL)
    {
      obj.addBool(keyWoType, val.asBool());
    }
    else
    {
      obj.addBool(key, val.asBool());
    }
  }
  else if (val.isIntegral())
  {
    if (typeHint == DataBlock::TYPE_REAL)
    {
      obj.addReal(keyWoType, (float)val.asDouble());
    }
    else if (typeHint == DataBlock::TYPE_INT)
    {
      obj.addInt(keyWoType, (int)val.asLargestInt());
    }
    else
    {
      obj.addInt(key, (int)val.asLargestInt());
    }
  }
  else if (val.isDouble())
  {
    if (typeHint == DataBlock::TYPE_REAL)
    {
      obj.addReal(keyWoType, (float)val.asDouble());
    }
    else if (typeHint == DataBlock::TYPE_INT)
    {
      obj.addInt(keyWoType, (int)val.asLargestInt());
    }
    else
    {
      obj.addReal(key, (float)val.asDouble());
    }
  }
  else if (val.isString())
  {
    if (typeHint == DataBlock::TYPE_STRING)
    {
      obj.addStr(keyWoType, val.asCString());
    }
    else
    {
      obj.addStr(key, val.asCString());
    }
  }

  if (typeHint != DataBlock::TYPE_NONE)
  {
    framemem_ptr()->free((void *)keyWoType);
  }
}

void json_to_blk(const Json::Value &json, DataBlock &blk)
{
  if (json.isArray())
    return;

  for (Json::ValueConstIterator it = json.begin(); it != json.end(); ++it)
  {
    const Json::Value &key = it.key();
    const Json::Value &value = *it;
    json_table_extract_value(blk, key.asCString(), value);
  }
}
