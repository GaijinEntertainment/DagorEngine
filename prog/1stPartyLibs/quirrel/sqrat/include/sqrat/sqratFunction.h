// Sqrat: altered version by Gaijin Games KFT
// sqratFunction: Quirrel Function Wrapper
//

//
// Copyright (c) 2009 Brandon Jones
// Copyirght 2011 Li-Cheng (Andy) Tai
//
// This software is provided 'as-is', without any express or implied
// warranty. In no event will the authors be held liable for any damages
// arising from the use of this software.
//
// Permission is granted to anyone to use this software for any purpose,
// including commercial applications, and to alter it and redistribute it
// freely, subject to the following restrictions:
//
//  1. The origin of this software must not be misrepresented; you must not
//  claim that you wrote the original software. If you use this software
//  in a product, an acknowledgment in the product documentation would be
//  appreciated but is not required.
//
//  2. Altered source versions must be plainly marked as such, and must not be
//  misrepresented as being the original software.
//
//  3. This notice may not be removed or altered from any source
//  distribution.
//

#pragma once
#if !defined(_SQRAT_SQFUNC_H_)
#define _SQRAT_SQFUNC_H_

#include <squirrel.h>
#include "sqratObject.h"
#include "sqratUtil.h"

namespace Sqrat {


class Function  {

    friend class TableBase;
    friend class Table;
    friend class ArrayBase;
    friend struct Var<Function>;

private:

    HSQUIRRELVM vm;
    HSQOBJECT env, obj;

public:
    Function() {
        vm = NULL;
        sq_resetobject(&env);
        sq_resetobject(&obj);
    }

    Function(const Function& sf) : vm(sf.vm), env(sf.env), obj(sf.obj) {
        sq_addref(vm, &env);
        sq_addref(vm, &obj);
    }

    Function(Function&& sf) : vm(sf.vm), env(sf.env), obj(sf.obj)
    {
      sf.vm = nullptr; // don't try release in moved from state
    }

    Function(const Function&&)=delete;

    /////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// Constructs a Function from a slot in an Object
    ///
    /// \param e    Object that potentially contains a Squirrel function in a slot
    /// \param slot Name of the slot to look for the Squirrel function in
    ///
    /////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    Function(const Object& e, const SQChar* slot) : vm(e.GetVM()), env(e.GetObject()) {
        sq_addref(vm, &env);
        Object so = e.GetSlot(slot);
        obj = so.GetObject();
        sq_addref(vm, &obj);

        SQObjectType value_type = so.GetType();
        if (value_type != OT_CLOSURE && value_type != OT_NATIVECLOSURE
          && value_type != OT_CLASS && value_type != OT_NULL) {
            // Note that classes can also be considered functions in Squirrel
            SQRAT_ASSERTF(0, _SC("function not found in slot"));
        }
    }

    /////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// Constructs a Function from two Squirrel objects (one is the environment object and the other is the function object)
    ///
    /// \param v VM that the function will exist in
    /// \param e Squirrel object that should represent the environment of the function
    /// \param o Squirrel object that should already represent a Squirrel function
    ///
    /////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    Function(HSQUIRRELVM v, HSQOBJECT e, HSQOBJECT o) : vm(v), env(e), obj(o) {
        sq_addref(vm, &env);
        sq_addref(vm, &obj);
    }

    ~Function() {
        Release();
    }

    Function& operator=(const Function& sf) {
      if (&sf != this)
      {
        Release();
        vm = sf.vm;
        env = sf.env;
        obj = sf.obj;
        sq_addref(vm, &env);
        sq_addref(vm, &obj);
      }
      return *this;
    }

    Function& operator=(Function&& sf)
    {
      if (&sf != this)
      {
        Release();
        vm = sf.vm;
        env = sf.env;
        obj = sf.obj;
        sf.vm = nullptr; // don't try release in moved from state
      }
      return *this;
    }

    bool IsNull() const {
        return sq_isnull(obj);
    }

    HSQOBJECT GetEnv() const {
        return env;
    }

    HSQOBJECT& GetEnv() {
        return env;
    }

    HSQOBJECT GetFunc() const {
        return obj;
    }

    HSQOBJECT& GetFunc() {
        return obj;
    }

    HSQUIRRELVM GetVM() const {
        return vm;
    }

    HSQUIRRELVM& GetVM() {
        return vm;
    }

    void Release() {
        if (vm)
        {
          sq_release(vm, &env);
          sq_release(vm, &obj);
          sq_resetobject(&env);
          sq_resetobject(&obj);
          vm = nullptr;
        }
    }

    template<class R>
    bool EvaluateDynArgs(HSQOBJECT const* args, size_t args_count, R& ret) const {
        if (!vm)
            return false;
        SQInteger top = sq_gettop(vm);

        sq_pushobject(vm, obj);
        sq_pushobject(vm, env);

        for (size_t i = 0; i < args_count; ++i)
          sq_pushobject(vm, args[i]);

        HSQUIRRELVM savedVm = vm; // vm can be nulled in sq_call()
        SQRESULT result = sq_call(vm, args_count+1, true, SQTrue);
        if (SQ_FAILED(result)) {
            ReportCallError();
            sq_settop(savedVm, top);
            return false;
        }

        ret = Var<R>(savedVm, -1).value;
        sq_settop(savedVm, top);
        return true;
    }

    bool ExecuteDynArgs(HSQOBJECT const* args, size_t args_count) const {
        SQRAT_ASSERT(vm);
        if (!vm)
            return false;
        SQInteger top = sq_gettop(vm);

        sq_pushobject(vm, obj);
        sq_pushobject(vm, env);

        for (size_t i = 0; i < args_count; ++i)
          sq_pushobject(vm, args[i]);

        HSQUIRRELVM savedVm = vm; // vm can be nulled in sq_call()
        SQRESULT result = sq_call(vm, args_count + 1, false, SQTrue);
        if (SQ_FAILED(result))
            ReportCallError();

        sq_settop(savedVm, top);

        return SQ_SUCCEEDED(result);
    }

    template<typename... ArgsAndRet>
    bool Evaluate(ArgsAndRet&&... args_and_ret) const {
        SQRAT_ASSERT(vm);
        if (!vm)
            return false;
        static constexpr size_t nArgs = sizeof...(ArgsAndRet) - 1;

        SQInteger top = sq_gettop(vm);

        sq_pushobject(vm, obj);
        sq_pushobject(vm, env);

        PushArgsWithoutRet(SQRAT_STD::forward<ArgsAndRet>(args_and_ret)...);

        HSQUIRRELVM savedVm = vm; // vm can be nulled in sq_call()
        SQRESULT result = sq_call(vm, nArgs + 1, true, SQTrue);
        if (SQ_FAILED(result)) {
            ReportCallError();

            sq_settop(savedVm, top);
            return false;
        }

        typedef typename SQRAT_STD::remove_reference<
                            vargs::TailElem_t<ArgsAndRet...>>::type R;

        R& ret = vargs::tail(SQRAT_STD::forward<ArgsAndRet>(args_and_ret)...);
        ret = Var<R>(savedVm, -1).value;
        sq_settop(savedVm, top);
        return true;
    }

    template <typename... Args>
    bool Execute(Args const&... args) const {
        SQRAT_ASSERT(vm);
        if (!vm)
            return false;
        static constexpr size_t nArgs = sizeof...(Args);
        SQInteger top = sq_gettop(vm);

        sq_pushobject(vm, obj);
        sq_pushobject(vm, env);

        PushArgs(args...);

        HSQUIRRELVM savedVm = vm; // vm can be nulled in sq_call()
        SQRESULT result = sq_call(vm, nArgs + 1, false, SQTrue);
        if (SQ_FAILED(result))
            ReportCallError();

        sq_settop(savedVm, top);

        return SQ_SUCCEEDED(result);
    }

    template <typename... Args>
    bool operator()(Args const&... args) const {
        return Execute(args...);
    }

    bool IsEqual(const Sqrat::Function &so) const {
        if (IsNull() && so.IsNull()) // Nulls may have uninitialized vm
            return true;
        if (GetVM() != so.GetVM())
            return false;

        SQInteger res = 0;
        return sq_direct_cmp(vm, &obj, &so.obj, &res) && (res==0);
    }

private:
    template<class Arg, typename... Tail>
    void PushArgs(Arg const& arg, Tail const&... tail) const
    {
      PushVar(vm, arg);
      PushArgs(tail...);
    }

    void PushArgs() const
    {
    }

    template<class Arg, typename... Tail>
    void PushArgsWithoutRet(Arg const& arg, Tail const&... tail) const
    {
      PushVar(vm, arg);
      PushArgsWithoutRet(tail...);
    }

    template<class R>
    void PushArgsWithoutRet(R const&) const
    {
    }

    void ReportCallError() const
    {
        if (!vm)
            return;
        SQPRINTFUNCTION errpf = sq_geterrorfunc(vm);
        if (!errpf)
            return;
        sq_pushobject(vm, obj);
        if (SQ_SUCCEEDED(sq_getclosurename(vm, -1)))
        {
            const SQChar *name;
            if (SQ_SUCCEEDED(sq_getstring(vm, -1, &name)))
                errpf(vm, _SC("Failed to call squirrel function %s\n"), name);
            sq_pop(vm, 1);
        }
        sq_pop(vm, 1);
    }
};


/// Used to get and push Function instances to and from the stack as references (functions are always references in Squirrel)
template<>
struct Var<Function> {

    Function value; ///< The actual value of get operations

    /////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// Attempts to get the value off the stack at idx as a Function
    ///
    /// \param vm  Target VM
    /// \param idx Index trying to be read
    ///
    /// \remarks
    /// Initializes environment to null
    ///
    /////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    Var(HSQUIRRELVM vm, SQInteger idx) {
        HSQOBJECT sqEnv;
        HSQOBJECT sqValue;
        sq_resetobject(&sqEnv);
        SQRAT_VERIFY(SQ_SUCCEEDED(sq_getstackobj(vm, idx, &sqValue)));
        value = Function(vm, sqEnv, sqValue);

        SQObjectType value_type = sq_gettype(vm, idx);
        if (value_type != OT_CLOSURE && value_type != OT_NATIVECLOSURE
          && value_type != OT_CLASS && value_type != OT_NULL)
        {
            SQRAT_ASSERTF(0, FormatTypeError(vm, idx, _SC("closure")).c_str());
        }
    }

    /// \remarks
    /// Initializes environment to given object
    Var(HSQUIRRELVM vm, SQInteger idx, HSQOBJECT env) {
        HSQOBJECT sqValue;
        SQRAT_VERIFY(SQ_SUCCEEDED(sq_getstackobj(vm, idx, &sqValue)));
        value = Function(vm, env, sqValue);

        SQObjectType value_type = sq_gettype(vm, idx);
        if (value_type != OT_CLOSURE && value_type != OT_NATIVECLOSURE
          && value_type != OT_CLASS && value_type != OT_NULL)
        {
            SQRAT_ASSERTF(0, FormatTypeError(vm, idx, _SC("closure")).c_str());
        }
    }

    /// Called by Sqrat::PushVar to put a Function on the stack
    static void push(HSQUIRRELVM vm, const Function& value) {
        sq_pushobject(vm, value.GetFunc());
    }


    static const SQChar * getVarTypeName() { return _SC("closure"); }
    static bool check_type(HSQUIRRELVM vm, SQInteger idx) {
        SQObjectType type = sq_gettype(vm, idx);
        return type == OT_CLOSURE || type == OT_NATIVECLOSURE || type == OT_NULL || type == OT_CLASS;
    }
};

template<>
struct Var<Function&> : Var<Function> {Var(HSQUIRRELVM vm, SQInteger idx) : Var<Function>(vm, idx) {}};

template<>
struct Var<const Function&> : Var<Function> {Var(HSQUIRRELVM vm, SQInteger idx) : Var<Function>(vm, idx) {}};

}

DAG_DECLARE_RELOCATABLE(Sqrat::Function);

#endif
