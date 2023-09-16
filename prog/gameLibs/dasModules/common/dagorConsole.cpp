#include <dasModules/aotDagorConsole.h>
#include <dasModules/dasSystem.h>
#include <EASTL/bitvector.h>

#include <util/dag_console.h>
#include <gui/dag_visualLog.h>

#include "daScript/ast/ast.h"
#include "dagorConsoleProcessor.h"


namespace bind_dascript
{
das::StackAllocator &get_shared_stack();

static char dagorConsole_das[] =
#include "dagorConsole.das.inl"
  ;
} // namespace bind_dascript

inline bool ends_with(const eastl::string_view &str, const char *suffix)
{
  const size_t suffixLen = strlen(suffix);
  return str.size() >= suffixLen && 0 == str.compare(str.size() - suffixLen, suffixLen, suffix);
}

struct ClosureInfo
{
  uint64_t mangledNameHash = 0u;
  uint32_t fileNameHash = 0u;
  eastl::string hint;
  eastl::string argsDescription;
  int minParams;
  int maxParams;
  eastl::vector<das::Type> argTypes;
  eastl::bitvector<> nullableArgs;
  eastl::vector<vec4f> defaultArgs;
  bool logAot = true;

  das::Context *context = nullptr;

  ClosureInfo() = default;
  ClosureInfo(uint64_t mnh, uint32_t file_name_hash, eastl::string hint_, eastl::string argsDescription_, int min_params,
    int max_params, eastl::vector<das::Type> &&arg_types, eastl::bitvector<> nullable_args, eastl::vector<vec4f> &&def_args) :
    mangledNameHash(mnh),
    fileNameHash(file_name_hash),
    hint(eastl::move(hint_)),
    argsDescription(eastl::move(argsDescription_)),
    minParams(min_params),
    maxParams(max_params),
    argTypes(eastl::move(arg_types)),
    nullableArgs(eastl::move(nullable_args)),
    defaultArgs(eastl::move(def_args))
  {}
};

eastl::vector<SimpleString> defStrings;
static das::recursive_mutex scriptCommandsMutex;
eastl::hash_map<eastl::string, ClosureInfo> scriptCommands;

struct ConsoleCmdFunctionAnnotation : das::FunctionAnnotation, console::ICommandProcessor
{
  void registerClosure(eastl::string const &n, ClosureInfo &&closure_info)
  {
    das::lock_guard<das::recursive_mutex> lock(scriptCommandsMutex);
    debug("daScript: register console command %s with %d params%s", n.c_str(), closure_info.minParams - 1,
      scriptCommands.find(n) != scriptCommands.end() ? " - replacing existing" : "");
    scriptCommands[n] = eastl::move(closure_info);
  }

  ConsoleCmdFunctionAnnotation() : FunctionAnnotation("console_cmd") {}

  ~ConsoleCmdFunctionAnnotation() override { del_con_proc(this); }

  bool apply(das::ExprBlock *, das::ModuleGroup &, const das::AnnotationArgumentList &, das::string &err) override
  {
    err = "not a block annotation";
    return false;
  }
  bool finalize(das::ExprBlock *, das::ModuleGroup &, const das::AnnotationArgumentList &, const das::AnnotationArgumentList &,
    das::string &err) override
  {
    err = "not a block annotation";
    return false;
  }
  bool apply(const das::FunctionPtr &fn, das::ModuleGroup &, const das::AnnotationArgumentList &, das::string &err) override
  {
    auto program = das::daScriptEnvironment::bound->g_Program;
    if (program->thisModule->isModule)
    {
      err = "console cmd shouldn't be placed in the module. Please move the function to a file without module directive";
      return false;
    }
    fn->exports = true;
    return true;
  };
  bool finalize(const das::FunctionPtr &fn, das::ModuleGroup &, const das::AnnotationArgumentList &argTypes,
    const das::AnnotationArgumentList &, das::string &err) override
  {
    const das::AnnotationArgument *nameAnnotation = argTypes.find("name", das::Type::tString);
    const eastl::string cmdName = nameAnnotation ? nameAnnotation->sValue : fn->name;
    static const char *suffixes[] = {"console.das", "debug.das"};
    bool correctFileName = false;
    for (const char *suffix : suffixes)
      if (ends_with(fn->at.fileInfo->name, suffix))
      {
        correctFileName = true;
        break;
      }
    if (!correctFileName)
      logerr("console cmd (%s) should be placed in the file with name suffix '_debug.das', '_console.das'", cmdName.c_str());
    const das::AnnotationArgument *hintAnnotation = argTypes.find("hint", das::Type::tString);
    const eastl::string hint = hintAnnotation ? hintAnnotation->sValue : "";
    const das::AnnotationArgument *argsDescriptionAnnotation = argTypes.find("args", das::Type::tString);
    const bool generateArgsDesc = argsDescriptionAnnotation == nullptr;
    const das::AnnotationArgument *isHookAnnotation = argTypes.find("is_hook", das::Type::tBool);
    const bool isHook = isHookAnnotation ? isHookAnnotation->bValue : false;

    if (isHook)
      console::register_command_as_hook(cmdName.c_str());

    eastl::string argsDescription = generateArgsDesc ? "" : argsDescriptionAnnotation->sValue;
    eastl::vector<das::Type> types;
    eastl::bitvector<> nullableArgs;
    eastl::vector<vec4f> defaultArgs;
    das::DebugInfoHelper helper;
    types.reserve(fn->arguments.size());
    nullableArgs.reserve(fn->arguments.size());
    int minArgsCount = 0;
    bool optionalPart = false;
    for (auto &arg : fn->arguments)
    {
      if (!(arg->type->isNoHeapType() || (arg->type->isPointer() && arg->type->firstType->isNoHeapType()))) // allow pointers to
                                                                                                            // no-heap types (as
                                                                                                            // optional args)
      {
        err.append_sprintf(" : arg '%s' has unsupported type: %s", arg->name.c_str(), arg->type->describe().c_str());
        return false;
      }
      bool isOptional = false;
      vec4f def;
      if (arg->init)
      {
        if (arg->init->rtti_isConstant())
        {
          if (!arg->init->rtti_isStringConstant())
            def = das::static_pointer_cast<das::ExprConst>(arg->init)->value;
          else
          {
            defStrings.emplace_back(das::static_pointer_cast<das::ExprConstString>(arg->init)->text.c_str());
            def = das::cast<char *>::from(defStrings.back().c_str());
          }
          isOptional = true;
        }
        else if (!arg->init->type->isRefType() && arg->init->noSideEffects && arg->init->rtti_isVar() &&
                 das::static_pointer_cast<das::ExprVar>(arg->init)->isGlobalVariable() &&
                 das::static_pointer_cast<das::ExprVar>(arg->init)->variable->type->constant)
        {
          if (arg->type->ref)
          {
            err.append_sprintf("Unsupported arg type. Default value for %s is ref type \n", arg->name.c_str());
          }
          else
          {
            auto var = das::static_pointer_cast<das::ExprVar>(arg->init)->variable;
            if (var->init->rtti_isConstant())
            {
              if (!var->init->rtti_isStringConstant())
                def = das::static_pointer_cast<das::ExprConst>(var->init)->value;
              else
              {
                defStrings.emplace_back(das::static_pointer_cast<das::ExprConstString>(var->init)->text.c_str());
                def = das::cast<char *>::from(defStrings.back().c_str());
              }
              isOptional = true;
            }
            else
            {
              das::Context ctx;
              auto node = var->init->simulate(ctx);
              ctx.restart();
              vec4f result = ctx.evalWithCatch(node);
              if (ctx.getException())
                err.append_sprintf("default value for %s failed to simulate\n", arg->name.c_str());
              else
                def = result;
              isOptional = true;
            }
          }
        }
        else
        {
          err.append_sprintf(" : arg '%s', unsupported initial value", arg->name.c_str());
          return false;
        }
        defaultArgs.push_back(def);
        optionalPart = true;
      }
      nullableArgs.push_back(false);
      eastl::function<bool(const das::TypeDeclPtr &)> resolveType = [&](const das::TypeDeclPtr &type) -> bool {
        switch (type->baseType)
        {
          case das::tPointer:
          {
            const bool res = resolveType(type->firstType);
            defaultArgs.push_back(v_splats(0));
            nullableArgs.back() = true;
            optionalPart = true;
            isOptional = true;
            return res;
          }
          case das::tBool: break;
          case das::tInt8: break;
          case das::tUInt8: break;
          case das::tInt16: break;
          case das::tUInt16: break;
          case das::tInt64: break;
          case das::tUInt64: break;
          case das::tInt: break;
          case das::tUInt: break;
          case das::tFloat: break;
          case das::tDouble: break;
          case das::tString: break;
          default:
            err.append_sprintf(" : arg '%s' has unsupported type: %s", arg->name.c_str(), arg->type->describe().c_str());
            return false;
        }
        types.push_back(type->baseType);
        return true;
      };
      resolveType(arg->type);
      minArgsCount += isOptional ? 0 : 1;

      if (optionalPart && !isOptional)
      {
        err.append_sprintf(" : arg '%s', expected initial value", arg->name.c_str());
        return false;
      }
      if (generateArgsDesc)
      {
        const bool generateDefValueStr = isOptional && !nullableArgs.back();
        eastl::string defValueStr = generateDefValueStr ? "=" : "";
        if (generateDefValueStr)
        {
          das::TypeInfo *fieldType = helper.makeTypeInfo(nullptr, arg->type);
          defValueStr.append(das::debug_value(defaultArgs.back(), fieldType, das::PrintFlags::fixedFloatingPoint));
        }
        argsDescription.append_sprintf("%c%s%s%c ", isOptional ? '[' : '<', arg->name.c_str(), defValueStr.c_str(),
          isOptional ? ']' : '>');
      }
    }
    if (!err.empty())
      return false;

    if (das::is_in_aot() || das::is_in_completion())
      return true;

    types.shrink_to_fit();
    defaultArgs.shrink_to_fit();

    const uint32_t fnHash = ecs_str_hash(fn->at.fileInfo->name.c_str());

    registerClosure(cmdName,
      ClosureInfo(fn->getMangledNameHash(), fnHash, hint, argsDescription, minArgsCount + 1, (int)fn->arguments.size() + 1,
        eastl::move(types), eastl::move(nullableArgs), eastl::move(defaultArgs)));
    return true;
  }

  using das::FunctionAnnotation::simulate;

  bool simulate(das::Context *ctx, das::SimFunction *fn) override
  {
    if (ctx->category.value != uint32_t(das::ContextCategory::none) &&
        ctx->category.value != uint32_t(das::ContextCategory::debugger_attached))
      return true;
    if (!fn->code || !fn->code->debugInfo.fileInfo)
    {
      logerr("console_cmd: unable to bind function '%s' without %s. Compile error?", fn->name,
        !fn->code ? "code" : "file information");
      return false;
    }
    const uint32_t fnHash = ecs_str_hash(fn->code->debugInfo.fileInfo->name.c_str());
    das::lock_guard<das::recursive_mutex> lock(scriptCommandsMutex);
    for (auto &command : scriptCommands)
      if (command.second.mangledNameHash == fn->mangledNameHash && command.second.fileNameHash == fnHash)
      {
        command.second.context = ctx;
        break;
      }
    return true;
  }

  void destroy() override {}

  bool processCommand(const char *argv[], int argc) override
  {
    das::lock_guard<das::recursive_mutex> lock(scriptCommandsMutex);
    if (ICommandProcessor::cmdCollector)
    {
      for (auto const &cmd : scriptCommands)
        console::collector_cmp(argv[0], argc, cmd.first.c_str(), cmd.second.minParams, cmd.second.maxParams, cmd.second.hint.c_str(),
          cmd.second.argsDescription.c_str());
      return false;
    }
    auto it = scriptCommands.find(argv[0]);
    if (it == scriptCommands.end())
      return false;
    ClosureInfo &closureInfo = it->second;
    if (argc < closureInfo.minParams || argc > closureInfo.maxParams)
    {
      console::print_d("wrong argTypes count, got %u, expected %u..%u", argc - 1, closureInfo.minParams - 1,
        closureInfo.maxParams - 1);
      if (!closureInfo.hint.empty())
        console::print_d("%s %s", closureInfo.argsDescription.c_str(), closureInfo.hint.c_str());
      return true;
    }
    if (!closureInfo.context || closureInfo.mangledNameHash == 0u)
    {
      console::print_d("console command is not initialized, probably code issue");
      return true;
    }
    if (closureInfo.logAot)
      if (das::SimFunction *fn = closureInfo.context->fnByMangledName(closureInfo.mangledNameHash))
      {
        debug("daScript: %s registered console command %s", fn->aot ? "AOT:" : "INTERPRET:", fn->name);
        closureInfo.logAot = false;
      }

    das::SimFunction *fn = closureInfo.context->fnByMangledName(closureInfo.mangledNameHash);
    if (EASTL_UNLIKELY(!fn))
      logerr("Unable to find console cmd function <%@> in context %p", closureInfo.mangledNameHash, (void *)closureInfo.context);

    eastl::vector<int> intArgs;
    const int reserveArgsSize = closureInfo.maxParams;
    intArgs.reserve(reserveArgsSize);
    eastl::vector<int64_t> longArgs;
    longArgs.reserve(reserveArgsSize);
    eastl::vector<float> floatArgs;
    floatArgs.reserve(reserveArgsSize);
    eastl::vector<bool> boolArgs;
    boolArgs.reserve(reserveArgsSize);
    eastl::vector<eastl::string> stringArgs;
    stringArgs.reserve(reserveArgsSize);
    vec4f *args = (vec4f *)(alloca((closureInfo.maxParams - 1) * sizeof(vec4f)));
    for (int i = 0; i < closureInfo.maxParams - 1; ++i)
    {
      if (i + 1 >= argc)
      {
        args[i] = closureInfo.defaultArgs[i + 1 - closureInfo.minParams];
        continue;
      }
      das::Type baseType = closureInfo.argTypes[i];
      const bool nullable = closureInfo.nullableArgs[i];
      switch (baseType)
      {
        case das::tBool:
        {
          boolArgs.push_back(to_bool(argv[i + 1]));
          args[i] = nullable ? das::cast<bool *>::from(&boolArgs.back()) : das::cast<bool>::from(boolArgs.back());
        }
        break;
        case das::tInt8:
        case das::tUInt8:
        case das::tInt16:
        case das::tUInt16:
        case das::tInt:
        case das::tUInt:
        {
          intArgs.push_back(to_int(argv[i + 1]));
          args[i] = nullable ? das::cast<int *>::from(&intArgs.back()) : das::cast<int>::from(intArgs.back());
        }
        break;
        case das::tInt64:
        case das::tUInt64:
        {
          longArgs.push_back(atoll(argv[i + 1]));
          args[i] = nullable ? das::cast<int64_t *>::from(&longArgs.back()) : das::cast<int64_t>::from(longArgs.back());
        }
        break;
        case das::tFloat:
        case das::tDouble:
        {
          floatArgs.push_back(to_real(argv[i + 1]));
          args[i] = nullable ? das::cast<float *>::from(&floatArgs.back()) : das::cast<float>::from(floatArgs.back());
        }
        break;
        case das::tString:
        {
          stringArgs.push_back(argv[i + 1]);
          auto str = stringArgs.back().data();
          args[i] = nullable ? das::cast<const char **>::from(&str) : das::cast<const char *>::from(str);
        }
        break;
        default: G_ASSERTF(0, "unsupported arg type '%@' in context command", baseType);
      }
    }

    closureInfo.context->tryRestartAndLock();
    if (!closureInfo.context->ownStack)
    {
      das::SharedStackGuard guard(*closureInfo.context, bind_dascript::get_shared_stack());
      closureInfo.context->evalWithCatch(fn, args);
    }
    else
    {
      closureInfo.context->evalWithCatch(fn, args);
    }
    if (auto exp = closureInfo.context->getException())
    {
      closureInfo.context->stackWalk(&closureInfo.context->exceptionAt, true, true);
      logerr("console_cmd: %s", exp);
    }
    closureInfo.context->unlock();

    return true;
  }
};

namespace bind_dascript
{
ConsoleProcessorRegistry consoleProcessorFunctions;
das::recursive_mutex consoleProcessorFunctionsMutex;

class DagorConsole final : public das::Module
{
public:
  das::smart_ptr<ConsoleCmdFunctionAnnotation> consoleCmdFunctionAnnotation;
  das::smart_ptr<ConsoleProcessorFunctionAnnotation> consoleProcessorFunctionAnnotation;

  DagorConsole() : das::Module("DagorConsole")
  {
    das::ModuleLibrary lib(this);
    addBuiltinDependency(lib, require("DagorMath"));

    addAnnotation(das::make_smart<ConsoleProcessorHintAnnotation>(lib));
    das::typeFactory<ConsoleProcessorHints>::make(lib);

    consoleCmdFunctionAnnotation = das::make_smart<ConsoleCmdFunctionAnnotation>();
    addAnnotation(consoleCmdFunctionAnnotation);
    consoleProcessorFunctionAnnotation =
      das::make_smart<ConsoleProcessorFunctionAnnotation>(consoleProcessorFunctions, consoleProcessorFunctionsMutex);
    addAnnotation(consoleProcessorFunctionAnnotation);
    das::onDestroyCppDebugAgent(name.c_str(), [](das::Context *ctx) {
      {
        das::lock_guard<das::recursive_mutex> lock(consoleProcessorFunctionsMutex);
        for (auto &pair : consoleProcessorFunctions)
          if (pair.second.context == ctx)
            pair.second.context = nullptr;
      }
      {
        das::lock_guard<das::recursive_mutex> lock(scriptCommandsMutex);
        for (auto &pair : scriptCommands)
          if (pair.second.context == ctx)
            pair.second.context = nullptr;
      }
    });

    das::addExtern<void (*)(const char *), &console::print>(*this, lib, "console_print", das::SideEffects::modifyExternal,
      "::console::print");
    das::addExtern<DAS_BIND_FUN(console::command)>(*this, lib, "console_command", das::SideEffects::modifyExternal,
      "::console::command");
    das::addExtern<void (*)(const char *, E3DCOLOR, int), &visuallog::logmsg>(*this, lib, "_builtin_visual_log",
      das::SideEffects::modifyExternal, "::visuallog::logmsg");

    das::addExtern<DAS_BIND_FUN(bind_dascript::add_hint)>(*this, lib, "add_hint", das::SideEffects::modifyArgument,
      "::bind_dascript::add_hint");

    das::addExtern<DAS_BIND_FUN(console::pop_front_history_command)>(*this, lib, "console_pop_front_history_command",
      das::SideEffects::modifyExternal, "::console::pop_front_history_command");

    das::addExtern<DAS_BIND_FUN(console::top_history_command)>(*this, lib, "console_top_history_command",
      das::SideEffects::accessExternal, "::console::top_history_command");

    das::addExtern<DAS_BIND_FUN(console::add_history_command)>(*this, lib, "console_add_history_command",
      das::SideEffects::modifyExternal, "::console::add_history_command");

    das::addExtern<DAS_BIND_FUN(bind_dascript::console_get_command_history)>(*this, lib, "console_get_command_history",
      das::SideEffects::accessExternal, "::bind_dascript::console_get_command_history");

    das::addExtern<DAS_BIND_FUN(bind_dascript::console_get_edit_text_before_modify)>(*this, lib, "console_get_edit_text_before_modify",
      das::SideEffects::accessExternal, "::bind_dascript::console_get_edit_text_before_modify");

    compileBuiltinModule("dagorConsole.das", (unsigned char *)dagorConsole_das, sizeof(dagorConsole_das));
    verifyAotReady();
  }
  virtual das::ModuleAotType aotRequire(das::TextWriter &tw) const override
  {
    tw << "#include <dasModules/aotDagorConsole.h>\n";
    return das::ModuleAotType::cpp;
  }
};

void das_add_con_proc_dagor_console()
{
  if (das::Module *mod = das::Module::require("DagorConsole"))
  {
    DagorConsole *dagorConsole = (DagorConsole *)mod;
    ::add_con_proc(dagorConsole->consoleCmdFunctionAnnotation.get());
    ::add_con_proc(dagorConsole->consoleProcessorFunctionAnnotation.get());
  }
  else
  {
    logerr("DagorConsole module is not loaded");
  }
}

} // namespace bind_dascript

REGISTER_MODULE_IN_NAMESPACE(DagorConsole, bind_dascript)