#include "jsonWriter.h"
#include "dataWalkerWrapper.h"
#include "serializationHelpers.h"
#include "aotJsonWriter.h"


// make code checker happy
#define DECL_DAS_MODULE_CLASS(N) class Module_##N

struct JsonWriterAnnotation : das::ManagedStructureAnnotation<RapidJsonWriter, false>
{
  explicit JsonWriterAnnotation(das::ModuleLibrary & ml):
      ManagedStructureAnnotation<RapidJsonWriter, false>("JsonWriter", ml, "RapidJsonWriter")
  {
    addProperty<DAS_BIND_MANAGED_PROP(das_result)>("result", "das_result");
    addProperty<DAS_BIND_MANAGED_PROP(das_getLastError)>("lastError", "das_getLastError");
  }

  bool isLocal() const override { return false; }
  bool canCopy() const override { return false; }
  bool canMove() const override { return false; }
  bool canClone() const override { return false; }
};


namespace das_jsonwriter
{
  bool jw_start_obj(RapidJsonWriter & jw, das::Context & context, das::SimNode_CallBase * call, vec4f * args)
  {
    return jw.das_ctx_wrap(context, &RapidJsonWriter::startObj);
  }


  bool jw_end_obj(RapidJsonWriter & jw, das::Context & context, das::SimNode_CallBase * call, vec4f * args)
  {
    return jw.das_ctx_wrap(context, &RapidJsonWriter::endObj);
  }


  bool jw_start_array(RapidJsonWriter & jw, das::Context & context, das::SimNode_CallBase * call, vec4f * args)
  {
    return jw.das_ctx_wrap(context, &RapidJsonWriter::startArray);
  }


  bool jw_end_array(RapidJsonWriter & jw, das::Context & context, das::SimNode_CallBase * call, vec4f * args)
  {
    return jw.das_ctx_wrap(context, &RapidJsonWriter::endArray);
  }


  bool jw_reset(RapidJsonWriter & jw, das::Context & context, das::SimNode_CallBase * call, vec4f * args)
  {
    jw.reset();
    return false;
  }


  bool jw_key(RapidJsonWriter & jw, das::Context & context, das::SimNode_CallBase * call, vec4f * args)
  {
    const char * val = das::cast<const char *>::to(args[1]);
    auto keyFn = static_cast<bool (RapidJsonWriter::*)(const char *)>(&RapidJsonWriter::key);
    return jw.das_ctx_wrap(context, keyFn, val ? val : "");
  }


  bool write_value_internal(RapidJsonWriter & jw, das::Context & context, das::TypeInfo * info, vec4f val)
  {
    if (info->type == das::tPointer && das::cast<char *>::to(val) == nullptr)
    {
      return jw.null();
    }

    das::DataWalkerWrapper<das::JsonWriteSerializationHelper<RapidJsonWriter>> writer(context, das::ref(jw));
    das::string oldLastError = jw.getLastError();
    jw.setLastError("");
    writer.process(val, info, false);
    if (jw.getLastError().empty())
    {
      jw.setLastError(oldLastError);
      return true;
    }
    return false;
  }

  bool jw_value(RapidJsonWriter & jw, das::Context & context, das::SimNode_CallBase * call, vec4f * args)
  {
    G_ASSERT(call->nArguments == 2);
    das::TypeInfo * info = call->types[1];
    G_ASSERT(info);
    return write_value_internal(jw, context, info, args[1]);
  }


  bool jw_append(RapidJsonWriter & jw, das::Context & context, das::SimNode_CallBase * call, vec4f * args)
  {
    G_ASSERT(call->nArguments == 3);
    das::TypeInfo * info = call->types[2];
    G_ASSERT(info);
    const char * key = das::cast<const char *>::to(args[1]);
    if (!key)
      key = "";
    if (!jw.das_ctx_wrap(context, &RapidJsonWriter::key, key))
      return false;
    return write_value_internal(jw, context, info, args[2]);
  }

  vec4f jw_writer(das::Context & context, das::SimNode_CallBase * call, vec4f * args)
  {
    RapidJsonWriter writer(&context);
    das::Block *block = das::cast<das::Block *>::to(args[0]);
    vec4f argForCall = das::cast<RapidJsonWriter *>::from(&writer);
    context.invoke(*block, &argForCall, nullptr, &call->debugInfo);
    return v_zero();
  }
}


static unsigned char builtin_das[] = R"DAS(
options indenting = 4
require jsonwriter

[generic]
def jw_key_value(var jw : JsonWriter; k : string const; v)
    jw_key(jw, k)
    jw_value(jw, v)

[generic]
def jw_key_obj(var jw : JsonWriter; name : string const; blk : block<():void>)
    jw_key(jw, name)
    jw_start_obj(jw)
    invoke(blk)
    jw_end_obj(jw)

[generic]
def jw_key_array(var jw : JsonWriter; name : string const; blk : block<():void>)
    jw_key(jw, name)
    jw_start_array(jw)
    invoke(blk)
    jw_end_array(jw)

[generic]
def jw_obj(var jw : JsonWriter; blk : block<():void>)
    jw_start_obj(jw)
    invoke(blk)
    jw_end_obj(jw)

[generic]
def jw_array(var jw : JsonWriter; blk : block<():void>)
    jw_start_array(jw)
    invoke(blk)
    jw_end_array(jw)

[generic]
def jw_writer(cb : block<(var JsonWriter):void>)
    __builtin_jw_writer(cb)
)DAS";


DECL_DAS_MODULE_CLASS(JsonWriter) : public das::Module
{
public:
  das::ModuleAotType aotRequire(das::TextWriter & tw) const override
  {
    tw << "#include <daScriptModules/rapidjson/jsonWriter.h>\n";
    tw << "#include <daScriptModules/rapidjson/aotJsonWriter.h>\n";
    return das::ModuleAotType::cpp;
  }

  Module_JsonWriter() : Module("jsonwriter")
  {
    das::ModuleLibrary lib(this);
    lib.addBuiltInModule();

    using namespace das_jsonwriter;

    addAnnotation(das::make_smart<JsonWriterAnnotation>(lib));
    das::addInterop<checked_interop<jw_reset, 1>, void, RapidJsonWriter &>(
        *this, lib, "jw_reset", das::SideEffects::modifyArgument,
        "das_jsonwriter::checked_interop<das_jsonwriter::jw_reset, 1>");

    das::addInterop<checked_interop<jw_start_obj, 1>, bool, RapidJsonWriter &>(
        *this, lib, "jw_start_obj", das::SideEffects::modifyArgument,
        "das_jsonwriter::checked_interop<das_jsonwriter::jw_start_obj, 1>");
    das::addInterop<checked_interop<jw_end_obj, 1>, bool, RapidJsonWriter &>(
        *this, lib, "jw_end_obj", das::SideEffects::modifyArgument,
        "das_jsonwriter::checked_interop<das_jsonwriter::jw_end_obj, 1>");
    das::addInterop<checked_interop<jw_start_array, 1>, bool, RapidJsonWriter &>(
        *this, lib, "jw_start_array", das::SideEffects::modifyArgument,
        "das_jsonwriter::checked_interop<das_jsonwriter::jw_start_array, 1>");
    das::addInterop<checked_interop<jw_end_array, 1>, bool, RapidJsonWriter &>(
        *this, lib, "jw_end_array", das::SideEffects::modifyArgument,
        "das_jsonwriter::checked_interop<das_jsonwriter::jw_end_array, 1>");

    das::addInterop<checked_interop<jw_key, 2>, bool, RapidJsonWriter &, const char *>(
        *this, lib, "jw_key", das::SideEffects::modifyArgument,
        "das_jsonwriter::checked_interop<das_jsonwriter::jw_key, 2>");

    das::addInterop<checked_interop<jw_value, 2>, bool, RapidJsonWriter &, vec4f>(
        *this, lib, "jw_value", das::SideEffects::modifyArgument,
        "das_jsonwriter::checked_interop<das_jsonwriter::jw_value, 2>");

    das::addInterop<checked_interop<jw_append, 3>, bool, RapidJsonWriter &, const char *, vec4f>(
        *this, lib, "jw_append", das::SideEffects::modifyArgument,
        "das_jsonwriter::checked_interop<das_jsonwriter::jw_append, 3>");

    das::addInterop<jw_writer, vec4f, vec4f>(
        *this, lib, "__builtin_jw_writer", das::SideEffects::modifyExternal,
        "das_jsonwriter::jw_writer");

    compileBuiltinModule("builtin_json_writer.das", builtin_das, sizeof(builtin_das));

    verifyAotReady();
  }
};


REGISTER_MODULE(Module_JsonWriter);
