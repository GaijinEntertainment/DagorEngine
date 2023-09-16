#include "rapidjson.h"

#include <daScript/daScript.h>
#include <daScript/simulate/simulate.h>
#include <daScript/simulate/simulate_visit_op.h>


DAS_BASE_BIND_ENUM_98(rapidjson::Type, JsonType,
  kNullType,
  kFalseType,
  kTrueType,
  kObjectType,
  kArrayType,
  kStringType,
  kNumberType)


DAS_BASE_BIND_ENUM_98(rapidjson::ParseErrorCode, ParseErrorCode,
  kParseErrorNone,

  kParseErrorDocumentEmpty,
  kParseErrorDocumentRootNotSingular,

  kParseErrorValueInvalid,

  kParseErrorObjectMissName,
  kParseErrorObjectMissColon,
  kParseErrorObjectMissCommaOrCurlyBracket,

  kParseErrorArrayMissCommaOrSquareBracket,

  kParseErrorStringUnicodeEscapeInvalidHex,
  kParseErrorStringUnicodeSurrogateInvalid,
  kParseErrorStringEscapeInvalid,
  kParseErrorStringMissQuotationMark,
  kParseErrorStringInvalidEncoding,

  kParseErrorNumberTooBig,
  kParseErrorNumberMissFraction,
  kParseErrorNumberMissExponent,

  kParseErrorTermination,
  kParseErrorUnspecificSyntaxError)


struct JsonValueAnnotation final: das::ManagedStructureAnnotation<rapidjson::Value, false>
{
  JsonValueAnnotation(das::ModuleLibrary &ml) : ManagedStructureAnnotation("JsonValue", ml, "rapidjson::Value")
  {
    addProperty<DAS_BIND_MANAGED_PROP(IsNull)>("IsNull", "IsNull");
    addProperty<DAS_BIND_MANAGED_PROP(IsArray)>("IsArray", "IsArray");
    addProperty<DAS_BIND_MANAGED_PROP(IsObject)>("IsObject", "IsObject");
    addProperty<DAS_BIND_MANAGED_PROP(GetType)>("GetType", "GetType");
    // Warning: carefully with binding methods like Empty/ObjectEmpty/MembersCount... here
    // It assumes specific underlying json type of Value instance, will assert/UB if called with wrong type
    // Either add/use type wrapper (i.e. JsonObject) or redirect invalid type to das exception
  }
};

struct JsonDocumentAnnotation final: das::ManagedStructureAnnotation<rapidjson::Document, false>
{
  JsonDocumentAnnotation(das::ModuleLibrary &ml) :
    ManagedStructureAnnotation("JsonDocument", ml, "rapidjson::Document")
  {
    addProperty<DAS_BIND_MANAGED_PROP(IsNull)>("IsNull", "IsNull");
    addProperty<DAS_BIND_MANAGED_PROP(IsArray)>("IsArray", "IsArray");
    addProperty<DAS_BIND_MANAGED_PROP(IsObject)>("IsObject", "IsObject");
    addProperty<DAS_BIND_MANAGED_PROP(GetType)>("GetType", "GetType");
    addProperty<DAS_BIND_MANAGED_PROP(HasParseError)>("HasParseError", "HasParseError");
    addProperty<DAS_BIND_MANAGED_PROP(GetParseError)>("GetParseError", "GetParseError");
    addProperty<DAS_BIND_MANAGED_PROP(GetErrorOffset)>("GetErrorOffset", "GetErrorOffset");
  }
};

template <typename TJsonArray>
struct GenericJsonArrayAnnotation : public das::ManagedStructureAnnotation<TJsonArray, false>
{
  GenericJsonArrayAnnotation(const das::string &n, das::ModuleLibrary &ml, const das::string &cpn)
    : das::ManagedStructureAnnotation<TJsonArray>(n, ml, cpn)
  {
    vecType = das::makeType<rapidjson::Value>(ml);
    vecType->ref = true;
  }

  virtual bool isIterable() const override { return true; }
  virtual das::TypeDeclPtr makeIteratorType(const das::ExpressionPtr &) const override
  {
    return das::make_smart<das::TypeDecl>(*vecType);
  }

  virtual das::SimNode * simulateGetIterator(
    das::Context &context, const das::LineInfo &at, const das::ExpressionPtr &src) const override
  {
    auto rv = src->simulate(context);
    if (src->type->isConst())
      return context.code->makeNode<
        das::SimNode_AnyIterator<const TJsonArray, DasArrayIterator<const TJsonArray>>>(at, rv);
    else
      return context.code->makeNode<
        das::SimNode_AnyIterator<TJsonArray, DasArrayIterator<TJsonArray>>>(at, rv);
  }

  bool isYetAnotherVectorTemplate() const override { return true; }

  virtual das::SimNode * simulateGetAt(
    das::Context &context, const das::LineInfo &at, const das::TypeDeclPtr &,
    const das::ExpressionPtr &rv, const das::ExpressionPtr &idx, uint32_t ofs) const override
  {
    auto rNode = rv->simulate(context), idxNode = idx->simulate(context);
    if (rv->type->isConst())
      return context.code->makeNode<SimNodeAtArray<const TJsonArray>>(
        at, rNode, idxNode, ofs);
    else
      return context.code->makeNode<SimNodeAtArray<TJsonArray>>(
        at, rNode, idxNode, ofs);
  }
  virtual bool isIndexable(const das::TypeDeclPtr &indexType) const override { return indexType->isIndex(); }
  virtual das::TypeDeclPtr makeIndexType(const das::ExpressionPtr &, const das::ExpressionPtr &) const override
  {
    return das::make_smart<das::TypeDecl>(*vecType);
  }

  template<class Array>
  struct SimNodeAtArray : das::SimNode_At
  {
    DAS_PTR_NODE;
    SimNodeAtArray(const das::LineInfo &at, das::SimNode *rv, das::SimNode *idx, uint32_t ofs)
        : SimNode_At(at, rv, idx, 0, ofs, 0) {}
    __forceinline char * compute(das::Context &context)
    {
      Array *pValue = (Array *) value->evalPtr(context);
      uint32_t idx = das::cast<uint32_t>::to(index->eval(context));
      if (EASTL_LIKELY(idx < pValue->Size()))
        return ((char *)(&(*pValue)[idx])) + offset;
      context.throw_error_at(debugInfo, "Array index %d out of range %d", idx, pValue->Size());
      return nullptr;
    }
    virtual das::SimNode * visit(das::SimVisitor &vis) override
    {
      using namespace das;
      using TT = Array;
      V_BEGIN();
      V_OP_TT(AtJsonArray);
      V_SUB(value);
      V_SUB(index);
      V_ARG(offset);
      V_END();
    }
  };

  template <class Array>
  struct DasArrayIterator : das::Iterator
  {
    DasArrayIterator(Array *ar) : array(ar) {}
    virtual bool first(das::Context &, char* _value) override
    {
      if (!array->Size())
        return false;
      iterator_type *value = (iterator_type *) _value;
      *value = &(*array)[0];
      end = array->end();
      return true;
    }
    virtual bool next(das::Context &, char * _value) override
    {
      iterator_type *value = (iterator_type *)_value;
      ++(*value);
      return *value != end;
    }
    virtual void close(das::Context &context, char *_value ) override
    {
      if (_value)
      {
        iterator_type *value = (iterator_type *) _value;
        *value = nullptr;
      }
      context.heap->free((char *)this, sizeof(DasArrayIterator<Array>));
    }
    Array *array = nullptr; // Can be unioned with end
    typedef decltype(array->begin()) iterator_type;
    iterator_type end;
  };

  das::TypeDeclPtr vecType;
};

struct JsonArrayAnnotation final
  : GenericJsonArrayAnnotation<rapidjson::Value::Array>
{
  JsonArrayAnnotation(das::ModuleLibrary &ml)
    : GenericJsonArrayAnnotation("JsonArray", ml, "rapidjson::Value::Array")
  {
    addProperty<DAS_BIND_MANAGED_PROP(Size)>("Size", "Size");
    addProperty<DAS_BIND_MANAGED_PROP(Empty)>("Empty", "Empty");
  }
};

struct JsonConstArrayAnnotation final
  : GenericJsonArrayAnnotation<rapidjson::Value::ConstArray>
{
  JsonConstArrayAnnotation(das::ModuleLibrary &ml)
    : GenericJsonArrayAnnotation("JsonConstArray", ml, "rapidjson::Value::ConstArray")
  {
    addProperty<DAS_BIND_MANAGED_PROP(Size)>("Size", "Size");
    addProperty<DAS_BIND_MANAGED_PROP(Empty)>("Empty", "Empty");
  }
};

class Module_RapidJson final : public das::Module
{
public:
  das::ModuleAotType aotRequire(das::TextWriter &tw) const override
  {
    tw << "#include <daScriptModules/rapidjson/rapidjson.h>\n";
    return das::ModuleAotType::cpp;
  }

  Module_RapidJson() : Module("rapidjson")
  {
    das::ModuleLibrary lib(this);
    addBuiltinDependency(lib, require("jsonwriter"));

    addEnumeration(das::make_smart<EnumerationJsonType>());
    addEnumeration(das::make_smart<EnumerationParseErrorCode>());

    auto jsonValueAnn = das::make_smart<JsonValueAnnotation>(lib);
    addAnnotation(jsonValueAnn);

    auto jsonDocumentAnn = das::make_smart<JsonDocumentAnnotation>(lib);
    addAnnotation(jsonDocumentAnn);
    jsonDocumentAnn->from(jsonValueAnn.get());

    addAnnotation(das::make_smart<JsonArrayAnnotation>(lib));
    addAnnotation(das::make_smart<JsonConstArrayAnnotation>(lib));

    das::addUsing<rapidjson::Value>(*this, lib, "rapidjson::Value");
    das::addUsing<rapidjson::Value, rapidjson::Type>(*this, lib, "rapidjson::Value");
    das::addUsing<rapidjson::Document>(*this, lib, "rapidjson::Document");
    das::addUsing<rapidjson::Document, rapidjson::Type>(*this, lib, "rapidjson::Document");

#define JMETH(m, mn, se)\
    using method_##m = DAS_CALL_MEMBER(rapidjson::Value::m);\
    das::addExtern<DAS_CALL_METHOD(method_##m)>(*this, lib, mn, das::SideEffects::se, DAS_CALL_MEMBER_CPP(rapidjson::Value::m))
#define JFNEX(f, fn, se, fncpp) das::addExtern<DAS_BIND_FUN(f)>(*this, lib, fn, das::SideEffects::se, fncpp)
#define JFN(f, fn, se) JFNEX(bind_dascript::f, fn, se, "bind_dascript::" fn)

    JMETH(SetNull, "SetNull", modifyArgument);
    JFN(PopBack, "PopBack", modifyArgument);
    JFN(Swap, "Swap", modifyArgument);
    JFN(SetObject, "SetObject", modifyArgumentAndExternal);
    JFN(SetArray, "SetArray", modifyArgumentAndExternal);

    das::addExtern<DAS_BIND_FUN(bind_dascript::document_Parse)>(*this, lib, "Parse",
      das::SideEffects::modifyArgument, "bind_dascript::document_Parse");

    das::addExtern<DAS_BIND_FUN(rapidjson::GetParseError_En)>(*this, lib, "GetParseError_En",
      das::SideEffects::none, "rapidjson::GetParseError_En");

#define JSON_TYPE_LIST\
    T(int32_t)\
    T(uint32_t)\
    T(int64_t)\
    T(uint64_t)\
    T(float)\
    T(bool)

    JFNEX(bind_dascript::PushBack<const rapidjson::Value&>, "PushBack", modifyArgument,
          "bind_dascript::PushBack<const rapidjson::Value&>");
    JFNEX(bind_dascript::PushBack<const char*>, "PushBack", modifyArgument,
          "bind_dascript::PushBack<const char*>");
#define T(t) JFNEX(bind_dascript::PushBackVal<t>, "PushBack", modifyArgument, "bind_dascript::PushBackVal");
JSON_TYPE_LIST
#undef T

    JFNEX(bind_dascript::AddMember<const rapidjson::Value&>, "AddMember", modifyArgument,
          "bind_dascript::AddMember<const rapidjson::Value&>");
    JFNEX(bind_dascript::AddMember<const char*>, "AddMember", modifyArgument,
          "bind_dascript::AddMember<const char*>");
#define T(t) JFNEX(bind_dascript::AddMemberVal<t>, "AddMember", modifyArgument, "bind_dascript::AddMemberVal");
JSON_TYPE_LIST
#undef T

    JFN(GetArray<rapidjson::Value>, "GetArray", modifyArgumentAndAccessExternal);
    JFN(GetArray<const rapidjson::Value>, "GetArray", accessExternal);

    JFN(FindMember<rapidjson::Value>, "FindMember", modifyArgumentAndAccessExternal);
    JFN(FindMember<const rapidjson::Value>, "FindMember", accessExternal);

    JFNEX(jsonutils::get_or<const char*>, "json_get_or", accessExternal, "jsonutils::get_or<const char*>");
#define T(t) JFNEX(jsonutils::get_or<t>, "json_get_or", accessExternal, "jsonutils::get_or");
JSON_TYPE_LIST
#undef T

    JFNEX(jsonutils::value_or<const char*>, "json_value_or", none, "jsonutils::value_or<const char*>");
#define T(t) JFNEX(jsonutils::value_or<t>, "json_value_or", none, "jsonutils::value_or");
JSON_TYPE_LIST
#undef T

    JFN(json_stringify, "json_stringify", accessExternal);
    JFN(json_stringify_pretty, "json_stringify_pretty", accessExternal);

    verifyAotReady();
  }
};

REGISTER_MODULE(Module_RapidJson);
