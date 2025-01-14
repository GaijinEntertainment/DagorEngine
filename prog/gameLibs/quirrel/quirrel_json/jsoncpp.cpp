// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include <quirrel/quirrel_json/jsoncpp.h>
#include <sqstdblob.h>
#include <json/json.h>


#define BLOB_KEY "!blob!"


static eastl::string json_val_to_string(Json::Value const &value)
{
  using namespace Json;
  switch (value.type())
  {
    case nullValue: return "null"; break;
    case intValue: return valueToString(value.asLargestInt()); break;
    case uintValue: return valueToString(value.asLargestUInt()); break;
    case realValue: return valueToString(value.asDouble()); break;
    case stringValue: return value.asString(); break;
    case booleanValue: return valueToString(value.asBool()); break;
    default: return "";
  }
}

Sqrat::Object jsoncpp_to_quirrel(HSQUIRRELVM vm, const Json::Value &obj)
{
  if (obj.isNull())
    return Sqrat::Object(vm);
  if (obj.isObject())
  {
    if (obj.isMember(BLOB_KEY))
    {
      auto s = obj[BLOB_KEY].asString();
      auto p = sqstd_createblob(vm, s.size());
      memcpy(p, s.c_str(), s.size());
      Sqrat::Var<Sqrat::Object> blob(vm, -1);
      sq_pop(vm, 1);
      return blob.value;
    }
    else
    {
      Sqrat::Table sqobj(vm);
      for (auto it = obj.begin(), ite = obj.end(); it != ite; ++it)
        sqobj.SetValue(it.memberName(), jsoncpp_to_quirrel(vm, *it));
      return sqobj;
    }
  }
  if (obj.isArray())
  {
    Sqrat::Array array(vm);
    for (Json::ArrayIndex i = 0, sz = obj.size(); i < sz; ++i)
      array.Append(jsoncpp_to_quirrel(vm, obj[i]));
    return array;
  }
  if (obj.isBool()) // isBool should be checked before isIntegral
    return Sqrat::Object(obj.asBool(), vm);
  if (obj.isString())
  {
    auto s = obj.asString();
    return Sqrat::Object(s.c_str(), vm, s.size());
  }
  if (obj.isIntegral())
    return Sqrat::Object(obj.asLargestInt(), vm);
  if (obj.isDouble())
    return Sqrat::Object(obj.asDouble(), vm);
  return Sqrat::Object(vm);
}

static Json::Value sqval_to_json(Sqrat::Object val)
{
  switch (val.GetType())
  {
    case OT_NULL: return Json::Value::null;
    case OT_INTEGER: return Json::Value(Json::Value::Int64(val.Cast<int64_t>()));
    case OT_FLOAT: return Json::Value(val.Cast<float>());
    case OT_STRING:
    {
      auto str = val.GetVar<const SQChar *>();
      return Json::Value(str.value, str.value + str.valueLen);
    }
    case OT_BOOL: return Json::Value(val.Cast<bool>());
    case OT_USERDATA: return Json::Value("<userdata>");
    case OT_CLOSURE: return Json::Value("<closure>");
    case OT_NATIVECLOSURE: return Json::Value("<nativeclosure>");
    case OT_GENERATOR: return Json::Value("<generator>");
    case OT_USERPOINTER: return Json::Value("<userpointer>");
    case OT_FUNCPROTO: return Json::Value("<funcproto>");
    case OT_THREAD: return Json::Value("<thread>");
    case OT_WEAKREF: return Json::Value("<weakref>");
    case OT_TABLE:
    case OT_CLASS:
    case OT_ARRAY:
    case OT_INSTANCE: return quirrel_to_jsoncpp(val);
    default: return Json::Value("<unsupported type>");
  }
}

Json::Value quirrel_to_jsoncpp(Sqrat::Object obj)
{
  HSQUIRRELVM vm = obj.GetVM();
  SQObjectType ot = obj.GetType();
  if (ot == OT_ARRAY)
  {
    Json::Value jarr(Json::arrayValue);
    Sqrat::Object::iterator iter;
    while (obj.Next(iter))
      jarr.append(sqval_to_json(Sqrat::Object(iter.getValue(), vm)));
    return jarr;
  }
  else if (ot == OT_TABLE || ot == OT_CLASS || ot == OT_INSTANCE)
  {
    Json::Value jobj(Json::objectValue);

    const char *ptr = nullptr;
    SQInteger size = 0;
    Sqrat::Var<Sqrat::Object>::push(vm, obj);
    sqstd_getblob(vm, -1, (SQUserPointer *)&ptr);
    if (ptr)
      size = sqstd_getblobsize(vm, -1);
    sq_pop(vm, 1);

    if (ptr && size >= 0)
    {
      // export blob
      jobj[BLOB_KEY] = Json::Value(ptr, ptr + size);
    }
    else
    {
      Sqrat::Object::iterator iter;
      bool reportedInvalidKey = false;
      while (obj.Next(iter))
      {
        Sqrat::Object sqKey(iter.getKey(), vm);
        if (sqKey.GetType() == OT_STRING) // fast path (without key copy)
        {
          const char *jkey = iter.getName();
          jobj[jkey] = sqval_to_json(Sqrat::Object(iter.getValue(), vm));
        }
        else
        {
          auto type = sqKey.GetType();
          eastl::string jkey = json_val_to_string(sqval_to_json(eastl::move(sqKey)));
          if (!reportedInvalidKey)
          {
            reportedInvalidKey = true;

            const SQChar *stack = nullptr;
            if (SQ_SUCCEEDED(sqstd_formatcallstackstring(vm)))
            {
              G_VERIFY(SQ_SUCCEEDED(sq_getstring(vm, -1, &stack)));
              if (!stack)
                stack = "[no quirrel stack]";
            }

            logerr("non-string (%s '%s') key in quirrel_to_jsoncpp()\n%s", sq_objtypestr(type), jkey.c_str(),
              stack ? stack : "[no quirrel stack]");

            if (stack)
              sq_pop(vm, 1);
          }
          jobj[jkey] = sqval_to_json(Sqrat::Object(iter.getValue(), vm));
        }
      }
    }

    return jobj;
  }
  return sqval_to_json(obj);
}
