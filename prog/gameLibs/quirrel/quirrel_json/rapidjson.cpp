// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include <quirrel/quirrel_json/rapidjson.h>
#include <rapidjson/error/en.h>
#include <rapidjson/reader.h>
#include <squirrel.h>
#include <EASTL/fixed_vector.h>
#include <EASTL/string_view.h>
#include <EASTL/string.h>
#include <EASTL/optional.h>

// only 64-bit SQ integers supported
static_assert(sizeof(SQInteger) == 8);

struct ParserContext : public rapidjson::BaseReaderHandler<rapidjson::UTF8<>, ParserContext> //-V730
{
  void setVal()
  {
    if (curObjType == OT_ARRAY)
      sq_arrayappend(vm, -2);
    else if (curObjType == OT_TABLE)
      sq_rawset(vm, -3);
  }

  bool Null()
  {
    sq_pushnull(vm);
    setVal();
    return true;
  }

  bool Bool(bool b)
  {
    sq_pushbool(vm, b);
    setVal();
    return true;
  }

  bool Int(int i)
  {
    sq_pushinteger(vm, i);
    setVal();
    return true;
  }

  bool Uint(unsigned u)
  {
    sq_pushinteger(vm, u);
    setVal();
    return true;
  }

  bool Int64(int64_t i)
  {
    sq_pushinteger(vm, i);
    setVal();
    return true;
  }

  bool Uint64(uint64_t u)
  {
    // quirrel does't support uint64 type
    sq_pushfloat(vm, SQFloat(u));
    setVal();
    return true;
  }

  bool Double(double d)
  {
    sq_pushfloat(vm, d);
    setVal();
    return true;
  }

  bool String(const char *str, rapidjson::SizeType length, bool /*copy*/)
  {
    sq_pushstring(vm, str, length);
    setVal();
    return true;
  }

  bool StartObject()
  {
    sq_newtable(vm);
    if (curObjType != OT_NULL)
    {
      if (topTypes.full())
      {
        nestedLimitError = true;
        return false;
      }
      topTypes.push_back(curObjType);
    }
    curObjType = OT_TABLE;
    return true;
  }

  bool EndObject(rapidjson::SizeType /*memberCount*/)
  {
    if (!topTypes.empty())
    {
      curObjType = topTypes.back();
      topTypes.pop_back();
    }
    else
      curObjType = OT_NULL;
    setVal();
    return true;
  }

  bool StartArray()
  {
    sq_newarray(vm, 0);
    if (curObjType != OT_NULL)
    {
      if (topTypes.full())
      {
        nestedLimitError = true;
        return false;
      }
      topTypes.push_back(curObjType);
    }
    curObjType = OT_ARRAY;
    return true;
  }

  bool EndArray(rapidjson::SizeType /*elementCount*/)
  {
    if (!topTypes.empty())
    {
      curObjType = topTypes.back();
      topTypes.pop_back();
    }
    else
      curObjType = OT_NULL;
    sq_arrayresize(vm, -1, sq_getsize(vm, -1));
    setVal();
    return true;
  }

  bool Key(const char *str, rapidjson::SizeType length, bool /*copy*/)
  {
    sq_pushstring(vm, str, length);
    return true;
  }

  bool nestedLimitError = false;
  eastl::fixed_vector<SQObjectType, 128, false> topTypes;
  SQObjectType curObjType = OT_NULL;
  HSQUIRRELVM vm;
};

eastl::optional<eastl::string> rapidjson_parse(HSQUIRRELVM vm, eastl::string_view json_txt)
{
  ParserContext pctx;
  pctx.vm = vm;
  rapidjson::Reader reader;
  rapidjson::MemoryStream ss(json_txt.data(), json_txt.size());
  rapidjson::ParseResult pr = reader.Parse(ss, pctx);
  if (pr.IsError())
  {
    if (pctx.nestedLimitError)
      return eastl::string(eastl::string::CtorSprintf{}, "JSON parse error: nested levels limit reached %lu", pctx.topTypes.size());
    else
      return eastl::string(eastl::string::CtorSprintf{}, "JSON parse error: %s (%u)", rapidjson::GetParseError_En(pr.Code()),
        pr.Offset());
  }
  return {};
}

Sqrat::Object rapidjson_to_quirrel(HSQUIRRELVM vm, const rapidjson::Value &jval)
{
  if (jval.IsObject())
  {
    Sqrat::Table sqobj(vm);
    rapidjson::Value::ConstObject obj = jval.GetObject();
    for (auto &[k, v] : obj)
      sqobj.SetValue(eastl::string_view{k.GetString(), k.GetStringLength()}, rapidjson_to_quirrel(vm, v));
    return sqobj;
  }
  if (jval.IsArray())
  {
    rapidjson::Value::ConstArray ar = jval.GetArray();
    Sqrat::Array sqarray(vm);
    for (const rapidjson::Value &arrEl : ar)
      sqarray.Append(rapidjson_to_quirrel(vm, arrEl));
    return sqarray;
  }
  if (jval.IsString())
    return Sqrat::Object(jval.GetString(), vm, jval.GetStringLength());
  if (jval.IsInt())
    return Sqrat::Object(jval.GetInt(), vm);
  if (jval.IsInt64())
    return Sqrat::Object(jval.GetInt64(), vm);
  if (jval.IsUint())
    return Sqrat::Object(jval.GetUint(), vm);
  if (jval.IsUint64())
    return Sqrat::Object(jval.GetUint64(), vm);
  if (jval.IsDouble())
    return Sqrat::Object(jval.GetDouble(), vm);
  if (jval.IsBool())
    return Sqrat::Object(jval.GetBool(), vm);
  return Sqrat::Object(vm);
}

rapidjson::Value quirrel_to_rapidjson_val(Sqrat::Object obj, rapidjson::Document::AllocatorType &allocator);

static rapidjson::Value sqval_to_json(Sqrat::Object val, rapidjson::Document::AllocatorType &allocator)
{
  switch (val.GetType())
  {
    case OT_NULL: return rapidjson::Value();
    case OT_INTEGER: return rapidjson::Value(val.Cast<int64_t>());
    case OT_FLOAT: return rapidjson::Value(val.Cast<float>());
    case OT_STRING:
    {
      auto str = val.GetVar<const SQChar *>();
      return rapidjson::Value(str.value, str.valueLen, allocator);
    }
    case OT_BOOL: return rapidjson::Value(val.Cast<bool>());
    case OT_USERDATA: return rapidjson::Value("<userdata>");
    case OT_CLOSURE: return rapidjson::Value("<closure>");
    case OT_NATIVECLOSURE: return rapidjson::Value("<nativeclosure>");
    case OT_GENERATOR: return rapidjson::Value("<generator>");
    case OT_USERPOINTER: return rapidjson::Value("<userpointer>");
    case OT_FUNCPROTO: return rapidjson::Value("<funcproto>");
    case OT_THREAD: return rapidjson::Value("<thread>");
    case OT_WEAKREF: return rapidjson::Value("<weakref>");
    case OT_TABLE:
    case OT_CLASS:
    case OT_ARRAY:
    case OT_INSTANCE: return quirrel_to_rapidjson_val(val, allocator);
    default: return rapidjson::Value("<unsupported type>");
  }
}

static rapidjson::Document sqval_to_json_doc(Sqrat::Object val)
{
  auto DocFromString = [](eastl::string_view str) {
    rapidjson::Document doc(rapidjson::Type::kStringType);
    doc.SetString(str.data(), str.size());
    return doc;
  };

  switch (val.GetType())
  {
    case OT_NULL: return rapidjson::Document();
    case OT_INTEGER:
    {
      rapidjson::Document doc(rapidjson::Type::kNumberType);
      doc.SetInt64(val.Cast<int64_t>());
      return doc;
    }
    case OT_FLOAT:
    {
      rapidjson::Document doc(rapidjson::Type::kNumberType);
      doc.SetFloat(val.Cast<float>());
      return doc;
    }
    case OT_STRING:
    {
      auto str = val.GetVar<const SQChar *>();
      rapidjson::Document doc(rapidjson::Type::kStringType);
      doc.SetString(str.value, str.valueLen, doc.GetAllocator());
      return doc;
    }
    case OT_BOOL:
    {
      if (val.Cast<bool>())
        return rapidjson::Document(rapidjson::Type::kTrueType);
      else
        return rapidjson::Document(rapidjson::Type::kFalseType);
    }
    case OT_USERDATA: return DocFromString("<userdata>");
    case OT_CLOSURE: return DocFromString("<closure>");
    case OT_NATIVECLOSURE: return DocFromString("<nativeclosure>");
    case OT_GENERATOR: return DocFromString("<generator>");
    case OT_USERPOINTER: return DocFromString("<userpointer>");
    case OT_FUNCPROTO: return DocFromString("<funcproto>");
    case OT_THREAD: return DocFromString("<thread>");
    case OT_WEAKREF: return DocFromString("<weakref>");
    default: return DocFromString("<unsupported type>");
  }
  return {};
}

rapidjson::Value quirrel_to_rapidjson_val(Sqrat::Object obj, rapidjson::Document::AllocatorType &allocator)
{
  HSQUIRRELVM vm = obj.GetVM();
  SQObjectType ot = obj.GetType();
  if (ot == OT_ARRAY)
  {
    rapidjson::Value jval(rapidjson::Type::kArrayType);
    rapidjson::Value::Array jarr = jval.GetArray();
    Sqrat::Object::iterator iter;
    while (obj.Next(iter))
      jarr.PushBack(sqval_to_json(Sqrat::Object(iter.getValue(), vm), allocator), allocator);
    return jval;
  }
  else if (ot == OT_TABLE || ot == OT_CLASS || ot == OT_INSTANCE)
  {
    rapidjson::Value jval(rapidjson::Type::kObjectType);
    rapidjson::Value::Object jobj = jval.GetObject();

    Sqrat::Object::iterator iter;
    while (obj.Next(iter))
    {
      Sqrat::Object sqKey(iter.getKey(), vm);
      rapidjson::Value v = sqval_to_json(Sqrat::Object(iter.getValue(), vm), allocator);
      if (sqKey.GetType() == OT_STRING) // non-string keys skipped
      {
        rapidjson::Value nameVal(iter.getName(), allocator);
        jobj.AddMember(nameVal, v, allocator);
      }
    }
    return jval;
  }
  return sqval_to_json(obj, allocator);
}

// We write almost copy-paste of two convert functions because UGLY object model in rapidjson
rapidjson::Document quirrel_to_rapidjson(Sqrat::Object obj)
{
  HSQUIRRELVM vm = obj.GetVM();
  SQObjectType ot = obj.GetType();
  if (ot == OT_ARRAY)
  {
    rapidjson::Document jval(rapidjson::Type::kArrayType);
    rapidjson::Document::AllocatorType &allocator = jval.GetAllocator();
    rapidjson::Value::Array jarr = jval.GetArray();
    Sqrat::Object::iterator iter;
    while (obj.Next(iter))
      jarr.PushBack(sqval_to_json(Sqrat::Object(iter.getValue(), vm), allocator), allocator);
    return jval;
  }
  else if (ot == OT_TABLE || ot == OT_CLASS || ot == OT_INSTANCE)
  {
    rapidjson::Document jval(rapidjson::Type::kObjectType);
    rapidjson::Document::AllocatorType &allocator = jval.GetAllocator();
    rapidjson::Value::Object jobj = jval.GetObject();

    Sqrat::Object::iterator iter;
    while (obj.Next(iter))
    {
      Sqrat::Object sqKey(iter.getKey(), vm);
      rapidjson::Value v = sqval_to_json(Sqrat::Object(iter.getValue(), vm), allocator);
      if (sqKey.GetType() == OT_STRING) // non-string keys skipped
      {
        rapidjson::Value nameVal(iter.getName(), allocator);
        jobj.AddMember(nameVal, v, allocator);
      }
    }
    return jval;
  }
  return sqval_to_json_doc(obj);
}
