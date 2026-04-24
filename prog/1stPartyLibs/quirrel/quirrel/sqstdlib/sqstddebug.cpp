#include <squirrel.h>
#include <sqstddebug.h>
#include <sqstdaux.h>
#include <string.h>
#include <assert.h>
#include <squirrel/sqvm.h>
#include <squirrel/sqstate.h>
#include <squirrel/sqobject.h>
#include <squirrel/sqfuncproto.h>
#include <squirrel/sqclosure.h>
#include <squirrel/sqtable.h>
#include <squirrel/sqarray.h>
#include <squirrel/sqclass.h>
#include <squirrel/compiler/sqtypeparser.h>
#include <chrono>


static SQInteger debug_seterrorhandler(HSQUIRRELVM v)
{
  sq_seterrorhandler(v);
  return 0;
}

static SQInteger debug_setdebughook(HSQUIRRELVM v)
{
  sq_setdebughook(v);
  return 0;
}

extern SQInteger __sq_getcallstackinfos(HSQUIRRELVM v, SQInteger level);

static SQInteger debug_getstackinfos(HSQUIRRELVM v)
{
  SQInteger level;
  sq_getinteger(v, -1, &level);
  return __sq_getcallstackinfos(v, level);
}

static SQInteger debug_getlocals(HSQUIRRELVM v)
{
    SQInteger level = 1;
    SQBool includeInternal = false;

    if (sq_gettop(v) >= 2)
        sq_getinteger(v, 2, &level);
    if (sq_gettop(v) >= 3)
        sq_getbool(v, 3, &includeInternal);

    sq_newtable(v);
    const char *name = NULL;

    SQInteger seq=0;
    SQInteger prevTop = sq_gettop(v);
    while ((name = sq_getlocal(v, level, seq))) {
        ++seq;
        if (!includeInternal && (name[0] == '@' || strcmp(name, "this")==0 || strcmp(name, "vargv")==0)) {
            sq_pop(v, 1);
            continue;
        }

        sq_pushstring(v, name, -1);
        sq_push(v, -2);
        sq_newslot(v, -4, SQFalse);
        sq_pop(v, 1);
    }
    return 1;
}


static SQInteger debug_get_stack_top(HSQUIRRELVM v)
{
    sq_pushinteger(v, sq_gettop(v));
    return 1;
}

#ifndef NO_GARBAGE_COLLECTOR
static SQInteger debug_collectgarbage(HSQUIRRELVM v)
{
    sq_pushinteger(v, sq_collectgarbage(v));
    return 1;
}
static SQInteger debug_resurrectunreachable(HSQUIRRELVM v)
{
    sq_resurrectunreachable(v);
    return 1;
}
#endif

static SQInteger debug_getbuildinfo(HSQUIRRELVM v)
{
  sq_newtable(v);
  sq_pushstring(v, "version", -1);
  sq_pushstring(v, SQUIRREL_VERSION, -1);
  sq_newslot(v, -3, SQFalse);
  sq_pushstring(v, "charsize", -1);
  sq_pushinteger(v, sizeof(char));
  sq_newslot(v, -3, SQFalse);
  sq_pushstring(v, "intsize", -1);
  sq_pushinteger(v, sizeof(SQInteger));
  sq_newslot(v, -3, SQFalse);
  sq_pushstring(v, "floatsize", -1);
  sq_pushinteger(v, sizeof(SQFloat));
  sq_newslot(v, -3, SQFalse);
  sq_pushstring(v, "gc", -1);
#ifndef NO_GARBAGE_COLLECTOR
  sq_pushstring(v, "enabled", -1);
#else
  sq_pushstring(v, "disabled", -1);
#endif // NO_GARBAGE_COLLECTOR
  sq_newslot(v, -3, SQFalse);
  return 1;
}

static SQInteger debug_doc(HSQUIRRELVM v)
{
    HSQOBJECT subject;
    sq_getstackobj(v, 2, &subject);

    SQObjectPtr value;
    SQObjectPtr key;
    key._type = OT_USERPOINTER;
    switch(sq_type(subject))
    {
        case OT_CLOSURE:
            key._unVal.pUserPointer = (void *)_closure(subject)->_function;
            break;
        case OT_NATIVECLOSURE:
            key._unVal.pUserPointer = (void *)_nativeclosure(subject)->_function;
            break;
        case OT_INSTANCE:
            key._unVal.pUserPointer = (void *)_instance(subject)->_class;
            break;
        default:
            key._unVal.pUserPointer = subject._unVal.pUserPointer;
            break;
    }

    if (!_table(_ss(v)->doc_objects)->Get(key, value))
    {
        sq_pushnull(v);
        return 1;
    }

    sq_pushobject(v, value);
    return 1;
}


static int nparamscheck_to_required_args(int nparamscheck)
{
    if (nparamscheck==0) // No param check
        return 0;
    return abs(nparamscheck)-1; // Don't include `this`
}

static SQInteger debug_get_function_info_table(HSQUIRRELVM v)
{
    HSQOBJECT subject;
    sq_getstackobj(v, 2, &subject);

    SQObjectPtr nullVal;
    SQObjectPtr docObject;
    SQObjectPtr value;
    SQObjectPtr key;
    key._type = OT_USERPOINTER;
    SQFunctionType ft(_ss(v));
    bool initialized = false;
    bool native = false;

    switch (sq_type(subject))
    {
        case OT_CLOSURE:
        {
            SQFunctionProto *f = _closure(subject)->_function;
            key._unVal.pUserPointer = (void *)_closure(subject)->_function;
            _table(_ss(v)->doc_objects)->Get(key, docObject);

            ft.functionName = f->_name;
            ft.returnTypeMask = f->_result_type_mask;
            ft.objectTypeMask = f->_param_type_masks[0];
            ft.requiredArgs = f->_nparameters - f->_ndefaultparams - 1;
            ft.pure = f->_purefunction;
            ft.nodiscard = f->_nodiscard;

            int cnt = f->_nparameters - (f->_varparams ? 1 : 0);
            for (SQInteger n = 1; n < cnt; n++) {
                ft.argNames.push_back(f->_parameters[n]);
                ft.argTypeMask.push_back(f->_param_type_masks[n]);
                ft.defaultValues.push_back(nullVal);
                // TODO: defaultValues
            }

            if (f->_varparams) {
                ft.ellipsisArgTypeMask = f->_param_type_masks[f->_nparameters - 1];
            }

            initialized = true;
            break;
        }

        case OT_NATIVECLOSURE: {
            native = true;

            key._unVal.pUserPointer = (void *)_nativeclosure(subject)->_function;
            _table(_ss(v)->doc_objects)->Get(key, docObject);

            key._unVal.pUserPointer = (void *)((size_t)(void *)_nativeclosure(subject)->_function ^ ~size_t(0));
            if (_table(_ss(v)->doc_objects)->Get(key, value)) {
                SQInteger errorPos = -1;
                SQObjectPtr errorString;
                if (sq_isstring(value)) {
                   if (sq_parse_function_type_string(v, _stringval(value), ft, errorPos, errorString)) {
                       initialized = true;
                   }
                   else {
                       // TODO: raise errorString
                   }
                }
                break;
            }
            else {
                SQNativeClosure *f = _nativeclosure(subject);

                ft.functionName = f->_name;
                ft.returnTypeMask = ~0u;
                ft.objectTypeMask = f->_typecheck.size() > 0 ? f->_typecheck[0] : (_RT_INSTANCE | _RT_TABLE | _RT_CLASS | _RT_USERDATA | _RT_NULL);
                ft.requiredArgs = nparamscheck_to_required_args(f->_nparamscheck);
                ft.pure = f->_purefunction;
                ft.nodiscard = f->_nodiscard;

                int cnt = (ft.requiredArgs > f->_typecheck.size()) ? ft.requiredArgs : f->_typecheck.size();

                for (SQInteger n = 1; n < cnt; n++) {
                    char buf[16] = { 0 };
                    snprintf(buf, sizeof(buf), "arg%d", int(n));
                    ft.argNames.push_back(SQObjectPtr(SQString::Create(_ss(v), buf, -1)));
                    ft.argTypeMask.push_back(n < f->_typecheck.size() ? f->_typecheck[n] : ~0u);
                    ft.defaultValues.push_back(nullVal);
                    // TODO: defaultValues
                }

                if (f->_nparamscheck < 0) {
                    ft.ellipsisArgTypeMask = ~0u;
                }

                initialized = true;
                break;
            }
        }

        default:
            break;
    }

    if (!initialized) {
        sq_pushnull(v);
        return 1;
    }

    SQTable *res = SQTable::Create(_ss(v), 12);

    #define SET_SLOT(name, value) \
        res->NewSlot(SQObjectPtr(SQString::Create(_ss(v), name, -1)), SQObjectPtr(value))

    SET_SLOT("functionName", ft.functionName);
    SET_SLOT("returnTypeMask", (int)ft.returnTypeMask);
    SET_SLOT("objectTypeMask", (int)ft.objectTypeMask);
    SET_SLOT("ellipsisArgTypeMask", (int)ft.ellipsisArgTypeMask);
    SET_SLOT("requiredArgs", ft.requiredArgs);
    SET_SLOT("native", native);
    SET_SLOT("pure", ft.pure);
    SET_SLOT("nodiscard", ft.nodiscard);
    SET_SLOT("doc", docObject);

    SQObjectPtr argNames(SQArray::Create(_ss(v), ft.argNames.size()));
    for (SQInteger n = 0; n < ft.argNames.size(); n++)
        _array(argNames)->Set((SQInteger)n, ft.argNames[n]);
    SET_SLOT("argNames", argNames);

    SQObjectPtr argTypeMask(SQArray::Create(_ss(v), ft.argTypeMask.size()));
    for (SQInteger n = 0; n < ft.argTypeMask.size(); n++)
        _array(argTypeMask)->Set((SQInteger)n, SQObjectPtr((int)ft.argTypeMask[n]));
    SET_SLOT("argTypeMask", argTypeMask);

    /*SQObjectPtr defaultValues(SQArray::Create(_ss(v), ft.defaultValues.size()));
    for (SQInteger n = 0; n < ft.defaultValues.size(); n++)
        _array(defaultValues)->Set((SQInteger)n, ft.defaultValues[n]);
    SET_SLOT("defaultValues", defaultValues);*/

    #undef SET_SLOT

    v->Push(SQObjectPtr(res));
    return 1;
}

static SQInteger debug_get_function_decl_string(HSQUIRRELVM v)
{
    HSQOBJECT subject;
    sq_getstackobj(v, 2, &subject);

    SQObjectPtr nullVal;
    SQObjectPtr value;
    SQObjectPtr key;
    key._type = OT_USERPOINTER;
    switch (sq_type(subject))
    {
        case OT_CLOSURE:
        {
            SQFunctionProto *f = _closure(subject)->_function;
            SQFunctionType ft(_ss(v));

            ft.functionName = f->_name;
            ft.returnTypeMask = f->_result_type_mask;
            ft.objectTypeMask = f->_param_type_masks[0];
            ft.requiredArgs = f->_nparameters - f->_ndefaultparams - 1;
            ft.pure = f->_purefunction;
            ft.nodiscard = f->_nodiscard;

            int cnt = f->_nparameters - (f->_varparams ? 1 : 0);
            for (SQInteger n = 1; n < cnt; n++) {
                ft.argNames.push_back(f->_parameters[n]);
                ft.argTypeMask.push_back(f->_param_type_masks[n]);
                ft.defaultValues.push_back(nullVal);
                // TODO: defaultValues
            }

            if (f->_varparams) {
                ft.ellipsisArgTypeMask = f->_param_type_masks[f->_nparameters - 1];
            }

            sq_pushobject(v, sq_stringify_function_type(v, ft));
            return 1;
        }

        case OT_NATIVECLOSURE:
            key._unVal.pUserPointer = (void *)((size_t)(void *)_nativeclosure(subject)->_function ^ ~size_t(0));
            if (_table(_ss(v)->doc_objects)->Get(key, value)) {
                sq_pushobject(v, value);
                return 1;
            }
            else {
                SQNativeClosure *f = _nativeclosure(subject);
                SQFunctionType ft(_ss(v));

                ft.functionName = f->_name;
                ft.returnTypeMask = ~0u;
                ft.objectTypeMask = f->_typecheck.size() > 0 ? f->_typecheck[0] : (_RT_INSTANCE | _RT_TABLE | _RT_CLASS | _RT_USERDATA | _RT_NULL);
                ft.requiredArgs = nparamscheck_to_required_args(f->_nparamscheck);
                ft.pure = f->_purefunction;
                ft.nodiscard = f->_nodiscard;

                int nArgsFromMask = f->_typecheck.size() ? f->_typecheck.size()-1 : 0; // Exclude `this`
                int nArgsDisplayed = f->_nparamscheck >= 0 ? ft.requiredArgs : // for fixed args count ignore mask length
                                    (ft.requiredArgs > nArgsFromMask) ? ft.requiredArgs : nArgsFromMask;

                for (SQInteger n = 1; n <= nArgsDisplayed; n++) {
                    char buf[16] = { 0 };
                    snprintf(buf, sizeof(buf), "arg%d", int(n));
                    ft.argNames.push_back(SQObjectPtr(SQString::Create(_ss(v), buf, -1)));
                    ft.argTypeMask.push_back(n < f->_typecheck.size() ? f->_typecheck[n] : ~0u);
                    ft.defaultValues.push_back(nullVal);
                    // TODO: defaultValues
                }

                if (f->_nparamscheck < 0) {
                    ft.ellipsisArgTypeMask = ~0u;
                }

                sq_pushobject(v, sq_stringify_function_type(v, ft));
                return 1;
            }
            break;

        default:
            break;
    }

    sq_pushnull(v);
    return 1;
}

static SQInteger format_call_stack_string(HSQUIRRELVM v)
{
  SQRESULT r = sqstd_formatcallstackstring(v);
  return SQ_SUCCEEDED(r) ? 1 : SQ_ERROR;
}


static SQInteger debug_type_mask_to_string(HSQUIRRELVM v)
{
  SQInteger typeMask = 0;
  sq_getinteger(v, 2, &typeMask);

  char buf[2048];
  sq_stringify_type_mask(buf, sizeof(buf), typeMask);

  sq_pushstring(v, buf, -1);

  return 1;
}


static SQUnsignedInteger32 sq_debug_time_msec()
{
  return SQUnsignedInteger32(std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::steady_clock::now().time_since_epoch()).count());
}


static bool default_quirrel_watchdog_hook(HSQUIRRELVM v, bool kick)
{
  if (_ss(v)->watchdog_threshold_msec && !kick)
  {
    SQUnsignedInteger32 curTime = sq_debug_time_msec();
    if (curTime - _ss(v)->watchdog_last_alive_time_msec > _ss(v)->watchdog_threshold_msec)
    {
      _ss(v)->watchdog_last_alive_time_msec = curTime;
      return false;
    }
    return true;
  }

  if (kick)
  {
    _ss(v)->watchdog_last_alive_time_msec = sq_debug_time_msec();
    return true;
  }

  return true;
}

SQInteger debug_script_watchdog_kick(HSQUIRRELVM v)
{
  sq_kick_watchdog(v);
  return SQ_OK;
}

SQInteger debug_set_script_watchdog_timeout_msec(HSQUIRRELVM v)
{
  SQInteger timeoutMsec = 0;
  sq_getinteger(v, 2, &timeoutMsec);
  SQInteger prevTimeout = sq_set_watchdog_timeout_msec(v, timeoutMsec);
  sq_pushinteger(v, prevTimeout);
  return 1;
}


static const SQRegFunctionFromStr debuglib_funcs[] = {
    { debug_seterrorhandler, "seterrorhandler(handler: function|null)", "Installs the given function as the VM error handler; null clears it" },
    { debug_setdebughook, "setdebughook(hook: function|null)", "Installs the given function as the VM debug hook; null clears it" },
    { debug_getstackinfos, "getstackinfos(level: int): table|null", "Returns call stack information for the given stack level" },
    { format_call_stack_string, "format_call_stack_string(): string", "Returns a formatted string describing the current call stack" },
    { debug_getlocals, "getlocals([level: int, include_internal: bool]): table", "Returns a table of local variables at the given stack level" },
    { debug_get_stack_top, "get_stack_top(): int", "Returns the current VM stack top index" },
    { debug_script_watchdog_kick, "script_watchdog_kick()", "Resets the script watchdog timer" },
    { debug_set_script_watchdog_timeout_msec, "set_script_watchdog_timeout_msec(timeout_msec: int): int", "Sets the script watchdog timeout in milliseconds and returns the previous value" },
    { debug_get_function_decl_string, "get_function_decl_string(func: function): string|null", "Returns a function declaration string" },
    { debug_type_mask_to_string, "type_mask_to_string(mask: int): string", "Convert type mask to human-readable string" },
    { debug_get_function_info_table, "get_function_info_table(func: function): table|null", "Returns meta information about a function as table" },
    { debug_doc, "doc(subject: table|function|instance|class): string|null", "Returns a documentation string for a function, class, or table" },
#ifndef NO_GARBAGE_COLLECTOR
    { debug_collectgarbage, "collectgarbage(): int", "Runs the garbage collector and returns the number of reclaimed objects" },
    { debug_resurrectunreachable, "resurrectunreachable(): array|null", "Resurrects unreachable objects for inspection" },
#endif
    { debug_getbuildinfo, "getbuildinfo(): table", "Returns a table describing the Quirrel build (version, sizes, GC status)" },
    { NULL, NULL, NULL }
};


SQRESULT sqstd_register_debuglib(HSQUIRRELVM v)
{
  SQInteger i = 0;
  while (debuglib_funcs[i].f) {
    sq_new_closure_slot_from_decl_string(v, debuglib_funcs[i].f, 0, debuglib_funcs[i].declstring, debuglib_funcs[i].docstring);
    i++;
  }

  SQWATCHDOGHOOK prevHook = sq_set_watchdog_hook(v, default_quirrel_watchdog_hook);
  if (prevHook != nullptr)
    sq_set_watchdog_hook(v, prevHook);

  return SQ_OK;
}
