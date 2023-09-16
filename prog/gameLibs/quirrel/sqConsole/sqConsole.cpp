#include <util/dag_console.h>
#include <quirrel/sqConsole/sqConsole.h>
#include <quirrel/sqConsole/sqPrintCollector.h>
#include <quirrel/quirrel_json/quirrel_json.h>
#include <quirrel/bindQuirrelEx/bindQuirrelEx.h>
#include <quirrel/sqModules/sqModules.h>
#include <memory/dag_framemem.h>
#include <sqrat.h>
#include <EASTL/map.h>
#include <EASTL/hash_map.h>
#include <EASTL/vector.h>
#include <assert.h>

#include <squirrel/sqvm.h>
#include <squirrel/sqstate.h>
#include <squirrel/sqstring.h>
#include <squirrel/sqfuncproto.h>
#include <squirrel/sqclosure.h>


static SQInteger stub_debug_table_data(HSQUIRRELVM v)
{
  SQPRINTFUNCTION pf = sq_getprintfunc(v);
  if (pf)
  {
    if (SQ_SUCCEEDED(sq_tostring(v, 2)))
    {
      const SQChar *s = nullptr;
      G_VERIFY(SQ_SUCCEEDED(sq_getstring(v, -1, &s)));
      const char *quotes = (sq_gettype(v, 2) == OT_STRING) ? "\"" : "";
      pf(v, "%s%s%s\n\nWARNING: Object printing function is not defined for this VM (use setObjPrintFunc)", quotes, s, quotes);
    }
    else
      pf(v, "[ERROR]");
  }

  return 0;
}


static SQInteger sq_debug_print_to_output_str(HSQUIRRELVM vm)
{
  if (SQ_SUCCEEDED(sq_tostring(vm, 2)))
  {
    const SQChar *str;
    if (SQ_SUCCEEDED(sq_getstring(vm, -1, &str)))
    {
      SQPrintCollector::sq_printf(vm, "%s", str);
      return 0;
    }
  }
  return SQ_ERROR;
}


static Sqrat::Object str_to_sqobj_via_json(HSQUIRRELVM vm, String const &str)
{
  eastl::optional<eastl::string> errStr;
  Sqrat::Object result = parse_json_to_sqrat(vm, {str.c_str(), (size_t)str.length()}, errStr);
  if (errStr.has_value())
    logwarn("faield to parse json: %s\n%s", errStr.value().c_str(), str.c_str());
  return result;
}


static Sqrat::Object make_func_obj(HSQUIRRELVM vm, SQFUNCTION func, const char *name, int paramscheck = 0,
  const char *typemask = nullptr)
{
  SqStackChecker stackCheck(vm);
  sq_newclosure(vm, func, 0);
  if (name)
    sq_setnativeclosurename(vm, -1, name);
  if (paramscheck)
    sq_setparamscheck(vm, paramscheck, typemask);

  HSQOBJECT obj;
  sq_getstackobj(vm, -1, &obj);
  Sqrat::Object res(obj, vm);
  sq_pop(vm, 1);
  return res;
}


class SqConsoleProcessor : public console::ICommandProcessor
{
private:
  struct ClosureInfo
  {
    Sqrat::Function closure;
    SQInteger nParams;
    SQInteger nFreevars;
    eastl::string description;
    eastl::string argsDescription;
  };


public:
  SqConsoleProcessor() = default;
  SqConsoleProcessor(SqConsoleProcessor &&rhs)
  {
    sqvm = rhs.sqvm;
#ifdef EASTL_MOVE
    scriptCommands = EASTL_MOVE(rhs.scriptCommands);
#else
    scriptCommands = rhs.scriptCommands;
#endif
  }
  SqConsoleProcessor(const SqConsoleProcessor &) = delete;


  ~SqConsoleProcessor() { del_con_proc(this); }


  void init(SqModules *module_mgr, const char *cmd)
  {
    sqvm = module_mgr->getVM();
    if (cmd && *cmd)
      execCmd = cmd;

    interactiveEnv = Sqrat::Table(module_mgr->getVM());
    module_mgr->bindRequireApi(interactiveEnv.GetObject());
    module_mgr->bindBaseLib(interactiveEnv.GetObject());

    Sqrat::Table exports(sqvm);
    exports.SquirrelFunc("register_command", sqRegisterCommand, -2, ".csss")
      .SquirrelFunc("setObjPrintFunc", setObjPrintFunc, 2, ".c|o")
      .Func("command", console::command);

    module_mgr->addNativeModule("console", exports);

    add_con_proc(this);
  }


  void clearScriptObjects()
  {
    scriptCommands.clear();
    objPrintFunc.Release();
  }


private:
  void registerClosure(eastl::string const &name, ClosureInfo &closure_info) { scriptCommands[name] = closure_info; }


  bool processCommand(const char *argv[], int argc) override { return executeScript(argv, argc) || callScriptHandler(argv, argc); }


  bool callScriptHandler(const char *argv[], int argc)
  {
    if (ICommandProcessor::cmdCollector)
    {
      for (auto const &cmd : scriptCommands)
        console::collector_cmp(argv[0], argc, cmd.first.c_str(), cmd.second.nParams, cmd.second.nParams,
          cmd.second.description.c_str(), cmd.second.argsDescription.c_str());
      return false;
    }
    auto it = scriptCommands.find(argv[0]);
    if (it == scriptCommands.end())
      return false;

    ClosureInfo &cinfo = it->second;
    int found = console::collector_cmp(argv[0], argc, argv[0], cinfo.nParams, cinfo.nParams, cinfo.description.c_str(),
      cinfo.argsDescription.c_str());
    if (found != 1)
      return false;

    String params(framemem_ptr());
    params += '[';
    for (int i = 1; i < argc; i++)
    {
      params += argv[i];
      if (i < argc - 1)
        params += ", ";
    }
    params += ']';

    Sqrat::Object sqParams = str_to_sqobj_via_json(sqvm, params);

    eastl::vector<HSQOBJECT> sqArray;
    if (sqParams.GetType() == OT_ARRAY)
    {
      Sqrat::Array array(sqParams);
      int n = array.Length();
      sqArray.resize(n);
      for (int i = 0; i < n; ++i)
        sqArray[i] = array[i];
    }

    SQPrintCollector printCollector(sqvm);
    if (!cinfo.closure.ExecuteDynArgs(sqArray.data(), sqArray.size()))
      console::error("failed to execute command: %s", printCollector.output.str());
    else
      console::print_d(printCollector.output.str());


    return true;
  }


  bool executeScript(const char *argv[], int argc)
  {
    if (argc == 0 || !sqvm || !execCmd)
      return false;

    int found = console::collector_cmp(argv[0], argc, execCmd, 2, 100);
    if (found != 1 || ICommandProcessor::cmdCollector)
      return false;

    String code(framemem_ptr());
    for (int i = 1; i < argc; i++)
    {
      code += argv[i];
      if (i < argc - 1)
        code += " ";
    }

    HSQUIRRELVM vm = sqvm;
    SqStackChecker stackCheck(vm);
    Sqrat::string errMsg;
    SQPrintCollector printCollector(vm);

    String scriptSrc(framemem_ptr());
    scriptSrc.printf(0, "return (@() (\n\
      %s\n\
    ))()",
      code.c_str());

    stackCheck.check();


    HSQOBJECT bindings = interactiveEnv;

    if (SQ_SUCCEEDED(sq_compilebuffer(vm, scriptSrc.c_str(), scriptSrc.length(), "interactive", true, &bindings)))
    {
      HSQOBJECT scriptClosure;
      sq_getstackobj(vm, -1, &scriptClosure);
      Sqrat::Function func(vm, Sqrat::Object(vm), scriptClosure);
      Sqrat::Object result;
      if (func.Evaluate(result))
      {
        Sqrat::Table objFormatSettings(vm);
        objFormatSettings.SetValue("silentMode", true);
        objFormatSettings.SetValue("printFn", make_func_obj(vm, sq_debug_print_to_output_str, "console_print_collect", 2));

        Sqrat::Function debugTableDataSq = objPrintFunc;
        if (debugTableDataSq.IsNull())
        {
          debug("SqConsole: object printing function not found for this VM, 'tostring()' will be used instead");
          Sqrat::Object stubFunc = make_func_obj(vm, stub_debug_table_data, "stubDebugTableData", -2, nullptr);
          debugTableDataSq = Sqrat::Function(vm, Sqrat::Object(vm), stubFunc);
        }

        if (!debugTableDataSq.IsNull())
          debugTableDataSq.Execute(result, objFormatSettings);
      }
      sq_pop(vm, 1); // script closure
    }

    stackCheck.check();

    console::print_d(printCollector.output.str());

    if (!errMsg.empty())
      console::error(errMsg.c_str());

    return true;
  }

  void destroy() override {}

  static SQInteger sqRegisterCommand(HSQUIRRELVM vm);
  static SQInteger setObjPrintFunc(HSQUIRRELVM vm);


private:
  HSQUIRRELVM sqvm;
  Sqrat::Function objPrintFunc;
  Sqrat::Table interactiveEnv;
  String execCmd;

  eastl::hash_map<eastl::string, ClosureInfo> scriptCommands;

  friend void create_script_console_processor(SqModules *module_mgr, const char *cmd);
};

static eastl::map<HSQUIRRELVM, SqConsoleProcessor> sq_console_processors;

void create_script_console_processor(SqModules *module_mgr, const char *cmd)
{
  HSQUIRRELVM vm = module_mgr->getVM();
  auto ins = sq_console_processors.insert(vm);
  if (ins.second) // if inserted
    ins.first->second.init(module_mgr, cmd);
  else
  {
    SqConsoleProcessor &sqcp = ins.first->second;
    if ((!cmd != sqcp.execCmd.empty()) || (cmd && !sqcp.execCmd.empty() && sqcp.execCmd != cmd))
      G_ASSERTF(0, "Console processor is already registered for this VM with different command: %s vs %s", cmd ? cmd : "<null>",
        !sqcp.execCmd.empty() ? sqcp.execCmd.c_str() : "<null>");
  }
}

void destroy_script_console_processor(HSQUIRRELVM vm) { sq_console_processors.erase(vm); }


void clear_script_console_processor(HSQUIRRELVM vm)
{
  auto it = sq_console_processors.find(vm);
  if (it != sq_console_processors.end())
    it->second.clearScriptObjects();
}


SQInteger SqConsoleProcessor::sqRegisterCommand(HSQUIRRELVM vm)
{
  auto sconIt = sq_console_processors.find(vm);
  if (sconIt == sq_console_processors.end())
    return sq_throwerror(vm, "Script console is not active for this VM");

  SQInteger nargs = sq_gettop(vm);
  ClosureInfo closureInfo;
  if (SQ_FAILED(sq_getclosureinfo(vm, 2, &closureInfo.nParams, &closureInfo.nFreevars)))
    return SQ_ERROR;

  HSQOBJECT closureObj;
  sq_getstackobj(vm, 2, &closureObj);

  closureInfo.closure = Sqrat::Function(vm, Sqrat::Object(vm), closureObj);

  if (SQ_FAILED(sq_getclosurename(vm, 2)))
    return SQ_ERROR;
  const char *closureName = nullptr;
  bool anon = false;
  if (SQ_FAILED(sq_getstring(vm, -1, &closureName)) || !closureName || closureName[0] == '(')
    anon = true;

  eastl::string cmd;
  if (nargs > 2)
  {
    const char *arg = nullptr;
    G_VERIFY(SQ_SUCCEEDED(sq_getstring(vm, 3, &arg)));
    if (!arg || !*arg)
      return sq_throwerror(vm, "Command name must not be empty");

    cmd = arg;
  }
  else if (!anon)
    cmd = closureName;
  else
    return sq_throwerror(vm, "sqRegisterCommand: you must specify name for anonymous closure");

#if DAGOR_DBGLEVEL > 0
  if (nargs > 3)
  {
    const char *arg = nullptr;
    G_VERIFY(SQ_SUCCEEDED(sq_getstring(vm, 4, &arg)));
    closureInfo.description = arg;
  }
#endif

#if DAGOR_DBGLEVEL > 0
  if (nargs > 4)
  {
    const char *arg = nullptr;
    G_VERIFY(SQ_SUCCEEDED(sq_getstring(vm, 5, &arg)));
    closureInfo.argsDescription = arg;
  }
  else if (sq_isclosure(closureObj))
  {
    eastl::string argsDesc;
    SQFunctionProto *sq_function_proto = closureObj._unVal.pClosure->_function;
    for (int i = 1, n = sq_function_proto->_nparameters; i < n; ++i)
    {
      SQObjectPtr parameter = sq_function_proto->_parameters[i];
      if (sq_isstring(parameter))
      {
        const char *paramName = _stringval(parameter);
        argsDesc.append_sprintf("<%s> ", paramName);
      }
    }
    closureInfo.argsDescription = argsDesc;
  }
#endif

  sconIt->second.registerClosure(cmd, closureInfo);
  return 0;
}


SQInteger SqConsoleProcessor::setObjPrintFunc(HSQUIRRELVM vm)
{
  auto sconIt = sq_console_processors.find(vm);
  if (sconIt == sq_console_processors.end())
    return sq_throwerror(vm, "Script console is not active for this VM");

  SqConsoleProcessor &self = sconIt->second;

  Sqrat::Var<Sqrat::Object> func(vm, 2);
  if (!func.value.IsNull())
    self.objPrintFunc = Sqrat::Function(vm, Sqrat::Object(vm), func.value);
  else
    self.objPrintFunc.Release();
  return 0;
}
