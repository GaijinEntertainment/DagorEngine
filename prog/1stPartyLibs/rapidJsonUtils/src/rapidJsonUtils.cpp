#include "rapidJsonUtils/rapidJsonUtils.h"


namespace jsonutils
{
  void set_copy(
    rapidjson::Document &doc,
    rapidjson::Value::StringRefType key,
    const rapidjson::Value &value,
    eastl::string /*out*/ *err_str)
  {
    if (!doc.IsObject())
    {
      if (err_str)
        err_str->sprintf("json value is not object (type=%#x)", doc.GetType());
      return;
    }

    rapidjson::Document::MemberIterator it = doc.FindMember(key);
    if (it != doc.MemberEnd())
    {
      it->value.CopyFrom(value, doc.GetAllocator());
    }
    else
    {
      rapidjson::Value jsonValue(value.GetType());
      jsonValue.CopyFrom(value, doc.GetAllocator());

      doc.AddMember(key, jsonValue, doc.GetAllocator());
    }
  }

  bool is_member_null(const rapidjson::Document &doc, const char *key)
  {
    if (!doc.IsObject())
      return true;

    rapidjson::Document::ConstMemberIterator it = doc.FindMember(key);
    if (it != doc.MemberEnd())
      return it->value.IsNull();

    return false;
  }
}