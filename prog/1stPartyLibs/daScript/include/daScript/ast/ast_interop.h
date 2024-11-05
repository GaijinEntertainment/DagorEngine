#pragma once

#include "daScript/ast/ast.h"
#include "daScript/ast/ast_visitor.h"
#include "daScript/simulate/interop.h"
#include "daScript/simulate/jit_abi.h"
#include "daScript/simulate/aot.h"

namespace das
{
    class ExternalFnBase : public BuiltInFunction {
    public:
        ExternalFnBase(const char * name, const char * cppName)
            : BuiltInFunction(name, cppName) {
            callBased = true;
        };
    };

    template<typename F> struct makeFuncArgs;
    template<typename R, typename ...Args> struct makeFuncArgs<R (*)(Args...)> : makeFuncArgs<R (Args...)> {};
    template<typename R, typename ...Args>
    struct makeFuncArgs<R (Args...)> {
        static __forceinline vector<TypeDeclPtr> make ( const ModuleLibrary & lib ) {
            return makeBuiltinArgs<R,Args...>(lib);
        }
    };

#if DAS_SLOW_CALL_INTEROP
    template  <typename FuncT, typename SimNodeT, typename FuncArgT>
#else
    template  <typename FuncT, FuncT fn, typename SimNodeT, typename FuncArgT>
#endif
    class ExternalFn : public ExternalFnBase {
        static_assert ( is_base_of<SimNode_CallBase, SimNodeT>::value, "only call-based nodes allowed" );
    public:
#if DAS_SLOW_CALL_INTEROP
        FuncT fn;
        __forceinline ExternalFn(FuncT fnp, const char * name, const ModuleLibrary & lib, const char * cppName = nullptr)
        : ExternalFnBase(name,cppName), fn(fnp) {
            constructExternal(makeFuncArgs<FuncArgT>::make(lib));
        }
#else
        __forceinline ExternalFn(const char * name, const ModuleLibrary & lib, const char * cppName = nullptr)
        : ExternalFnBase(name,cppName) {
            constructExternal(makeFuncArgs<FuncArgT>::make(lib));
        }
        __forceinline ExternalFn(const char * name, const char * cppName = nullptr)
        : ExternalFnBase(name,cppName) {
        }
#endif
        virtual SimNode * makeSimNode ( Context & context, const vector<ExpressionPtr> & ) override {
            const char * fnName = context.code->allocateName(this->name);
#if DAS_SLOW_CALL_INTEROP
            return context.code->makeNode<SimNodeT>(at, fnName, fn);
#else
            return context.code->makeNode<SimNodeT>(at, fnName);
#endif
        }
        virtual void * getBuiltinAddress() const override {
            return ImplWrapCall<SimNodeT::IS_CMRES, NeedVectorWrap<FuncT>::value, FuncT, fn>::get_builtin_address();
        }
    };

    template  <InteropFunction func, typename RetT, typename ...Args>
    class InteropFn : public BuiltInFunction {
    public:
        __forceinline InteropFn(const char * name, const ModuleLibrary & lib, const char * cppName = nullptr)
            : BuiltInFunction(name,cppName) {
            this->callBased = true;
            this->interopFn = true;
            constructInterop(makeBuiltinArgs<RetT, Args...>(lib));
        }
        virtual SimNode * makeSimNode ( Context & context, const vector<ExpressionPtr> & ) override {
            const char * fnName = context.code->allocateName(this->name);
            return context.code->makeNode<SimNode_InteropFuncCall<func>>(BuiltInFunction::at,fnName);
        }
        virtual void * getBuiltinAddress() const override { return (void *) func; }
    };

    struct defaultTempFn {
        defaultTempFn() = default;
        defaultTempFn ( bool args, bool impl, bool result, bool econst )
            : tempArgs(args), implicitArgs(impl), tempResult(result), explicitConstArgs(econst) {}
        ___noinline bool operator () ( Function * fn ) {
            if ( tempArgs || implicitArgs ) {
                for ( auto & arg : fn->arguments ) {
                    if ( arg->type->isTempType() ) {
                        arg->type->temporary = tempArgs;
                        arg->type->implicit = implicitArgs;
                        arg->type->explicitConst = explicitConstArgs;
                    }
                }
            }
            if ( tempResult ) {
                if ( fn->result->isTempType() ) {
                    fn->result->temporary = true;
                }
            }
            return true;
        }
        bool tempArgs = false;
        bool implicitArgs = true;
        bool tempResult = false;
        bool explicitConstArgs = false;
    };

    struct permanentArgFn : defaultTempFn {
        permanentArgFn() : defaultTempFn(false,false,false,false) {}
    };

    struct temporaryArgFn : defaultTempFn {
        temporaryArgFn() : defaultTempFn(true,false,false,false) {}
    };

    struct explicitConstArgFn : defaultTempFn {
        explicitConstArgFn() : defaultTempFn(false,true,false,true) {}
    };

    template  <typename CType, typename ...Args>
    class BuiltIn_PlacementNew : public BuiltInFunction {
    public:
        __forceinline BuiltIn_PlacementNew(const char * fn, const ModuleLibrary & lib, const char * cna = nullptr)
        : BuiltInFunction(fn,cna), fnName(fn) {
            this->modifyExternal = true;
            this->isTypeConstructor = true;
            this->copyOnReturn = true;
            this->moveOnReturn = true;
            construct(makeBuiltinArgs<CType,Args...>(lib));
        }
        virtual SimNode * makeSimNode ( Context & context, const vector<ExpressionPtr> & ) override {
            return context.code->makeNode<SimNode_PlacementNew<CType,Args...>>(at,fnName);
        }
        const char * fnName = nullptr;
        static void placementNewFunc ( CType * cmres, Args... args ) {
            new (cmres) CType(args...);
        }
        virtual void * getBuiltinAddress() const override { return (void *) &placementNewFunc; }
    };

    template  <typename CType, typename ...Args>
    class BuiltIn_Using : public BuiltInFunction {
    public:
        __forceinline BuiltIn_Using(const ModuleLibrary & lib, const char * cppName)
        : BuiltInFunction("using","das_using") {
            this->cppName = string("das_using<") + cppName + ">::use";
            this->aotTemplate = true;
            this->modifyExternal = true;
            this->invoke = true;
            this->jitContextAndLineInfo = true; // we need context and line info for usingFunc
            vector<TypeDeclPtr> args = makeBuiltinArgs<void,Args...>(lib);
            auto argT = makeType<CType>(lib);
            if ( !argT->canCopy() && !argT->canMove() ) {
                args.emplace_back(makeType<const TBlock<void,TExplicit<CType>>>(lib));
            } else {
                args.emplace_back(makeType<const TBlock<void,TTemporary<TExplicit<CType>>>>(lib));
            }
            construct(args);
        }
        virtual SimNode * makeSimNode ( Context & context, const vector<ExpressionPtr> & ) override {
            return context.code->makeNode<SimNode_Using<CType,Args...>>(at);
        }
        static void usingFunc ( Args... args, TBlock<void,TTemporary<TExplicit<CType>>> && block, Context * context, LineInfo * at ) {
            CType value(args...);
            vec4f bargs[1];
            bargs[0] = cast<CType *>::from(&value);
            context->invoke(block,bargs,nullptr,at);
        }
        virtual void * getBuiltinAddress() const override { return (void *) &usingFunc; }
    };

    void addExternFunc(Module& mod, const FunctionPtr & fx, bool isCmres, SideEffects seFlags);

#if DAS_SLOW_CALL_INTEROP
    template <typename FuncT, FuncT fn, template <typename FuncTT> class SimNodeT = SimNode_ExtFuncCall, typename QQ = defaultTempFn>
#else
    template <typename FuncT, FuncT fn, template <typename FuncTT, FuncTT fnt> class SimNodeT = SimNode_ExtFuncCall>
#endif
    inline auto addExternProperty ( Module & mod, const ModuleLibrary & lib, const char * name, const char * cppName = nullptr,
                                    bool explicitConst=false, SideEffects sideEffects = SideEffects::none ) {
#if DAS_SLOW_CALL_INTEROP
        using SimNodeType = SimNodeT<FuncT>;
        auto fnX = make_smart<ExternalFn<FuncT, SimNodeType, FuncT>>(fn, name, lib, cppName);
#else
        using SimNodeType = SimNodeT<FuncT, fn>;
        auto fnX = make_smart<ExternalFn<FuncT, fn, SimNodeType, FuncT>>(name, lib, cppName);
#endif
        defaultTempFn tempFn;
        tempFn(fnX.get());
        fnX->arguments[0]->type->explicitConst = explicitConst;
        fnX->setSideEffects(sideEffects);
        fnX->propertyFunction = true;
        DAS_ASSERTF(!fnX->result->isSmartPointer(), "property function can't return smart pointer %s::%s", mod.name.c_str(), name);
        mod.addFunction(fnX,true);  // yes, this one can fail. same C++ bound property can be in multiple classes before or after refactor
        return fnX;
    }

#if DAS_SLOW_CALL_INTEROP
    template <typename FuncT, FuncT fn, template <typename FuncTT> class SimNodeT = SimNode_ExtFuncCall, typename QQ = defaultTempFn>
#else
    template <typename ArgType, int ArgConst, typename RetType, typename FuncT, FuncT fn, template <typename FuncTT, FuncTT fnt> class SimNodeT = SimNode_ExtFuncCall>
#endif
    inline auto addExternPropertyForType ( Module & mod, const ModuleLibrary & lib, const char * name, const char * cppName = nullptr,
                                    bool explicitConst=false, SideEffects sideEffects = SideEffects::none) {
#if DAS_SLOW_CALL_INTEROP
        using SimNodeType = SimNodeT<FuncT>;
        auto fnX = make_smart<ExternalFn<FuncT, fn, SimNodeType, FuncT>>(name, cppName);
#else
        using SimNodeType = SimNodeT<FuncT, fn>;
        auto fnX = make_smart<ExternalFn<FuncT, fn, SimNodeType, FuncT>>(name, cppName);
#endif
        vector<TypeDeclPtr> types(2);
        types[0] = makeType<RetType>(lib);
        types[1] = makeType<ArgType>(lib);
        types[1]->constant = ArgConst;
        fnX->constructExternal(types);
        defaultTempFn tempFn;
        tempFn(fnX.get());
        fnX->arguments[0]->type->explicitConst = explicitConst;
        fnX->setSideEffects(sideEffects);
        fnX->propertyFunction = true;
        DAS_ASSERTF(!fnX->result->isSmartPointer(), "property function can't return smart pointer %s::%s", mod.name.c_str(), name);
        mod.addFunction(fnX,true);  // yes, this one can fail. same C++ bound property can be in multiple classes before or after refactor
        return fnX;
    }

#if DAS_SLOW_CALL_INTEROP
    template <typename FuncT, FuncT fn, template <typename FuncTT> class SimNodeT = SimNode_ExtFuncCall, typename QQ = defaultTempFn>
#else
    template <typename FuncT, FuncT fn, template <typename FuncTT, FuncTT fnt> class SimNodeT = SimNode_ExtFuncCall, typename QQ = defaultTempFn>
#endif
    inline auto addExtern ( Module & mod, const ModuleLibrary & lib, const char * name, SideEffects seFlags,
                                  const char * cppName = nullptr, QQ && tempFn = QQ() ) {
#if DAS_SLOW_CALL_INTEROP
        using SimNodeType = SimNodeT<FuncT>;
        auto fnX = make_smart<ExternalFn<FuncT, SimNodeType, FuncT>>(fn, name, lib, cppName);
#else
        using SimNodeType = SimNodeT<FuncT, fn>;
        auto fnX = make_smart<ExternalFn<FuncT, fn, SimNodeType, FuncT>>(name, lib, cppName);
#endif

        tempFn(fnX.get());
        addExternFunc(mod, fnX, SimNodeType::IS_CMRES, seFlags);
        return fnX;
    }

#if DAS_SLOW_CALL_INTEROP
    template <typename FuncT, FuncT fn, template <typename FuncTT> class SimNodeT = SimNode_ExtFuncCall, typename QQ = defaultTempFn>
#else
    template <typename FuncT, FuncT fn, template <typename FuncTT, FuncTT fnt> class SimNodeT = SimNode_ExtFuncCall, typename QQ = defaultTempFn>
#endif
    inline auto makeExtern ( const ModuleLibrary & lib, const char * name,
                                const char * cppName = nullptr, QQ && tempFn = QQ() ) {
#if DAS_SLOW_CALL_INTEROP
        using SimNodeType = SimNodeT<FuncT>;
        auto fnX = make_smart<ExternalFn<FuncT, SimNodeType, FuncT>>(fn, name, lib, cppName);
#else
        using SimNodeType = SimNodeT<FuncT, fn>;
        auto fnX = make_smart<ExternalFn<FuncT, fn, SimNodeType, FuncT>>(name, lib, cppName);
#endif

        tempFn(fnX.get());

        if (!SimNodeType::IS_CMRES) {
            if (fnX->result->isRefType() && !fnX->result->ref) {
                DAS_FATAL_ERROR(
                    "addExtern(%s)::failed\n"
                    "  this function should be bound with addExtern<DAS_BIND_FUNC(%s), SimNode_ExtFuncCallAndCopyOrMove>\n"
                    "  likely cast<> is implemented for the return type, and it should not\n",
                    fnX->name.c_str(), fnX->name.c_str());
            }
        }

        return fnX;
    }

#if DAS_SLOW_CALL_INTEROP
    template <typename FuncArgT, typename FuncT, FuncT fn, template <typename FuncTT> class SimNodeT = SimNode_ExtFuncCall>
#else
    template <typename FuncArgT, typename FuncT, FuncT fn, template <typename FuncTT, FuncTT fnt> class SimNodeT = SimNode_ExtFuncCall, typename QQ = defaultTempFn>
#endif
    inline auto addExternEx ( Module & mod, const ModuleLibrary & lib, const char * name, SideEffects seFlags,
                                  const char * cppName = nullptr, QQ && tempFn = QQ() ) {
#if DAS_SLOW_CALL_INTEROP
        using SimNodeType = SimNodeT<FuncT>;
        auto fnX = make_smart<ExternalFn<FuncT, SimNodeType, FuncArgT>>(fn, name, lib, cppName);
#else
        using SimNodeType = SimNodeT<FuncT, fn>;
        auto fnX = make_smart<ExternalFn<FuncT, fn, SimNodeType, FuncArgT>>(name, lib, cppName);
#endif
        tempFn(fnX.get());
        addExternFunc(mod, fnX, SimNodeType::IS_CMRES, seFlags);
        return fnX;
    }

#if DAS_SLOW_CALL_INTEROP
    template <typename FuncT, FuncT fn, template <typename FuncTT> class SimNodeT = SimNode_ExtFuncCallRef, typename QQ = defaultTempFn>
#else
    template <typename FuncT, FuncT fn, template <typename FuncTT, FuncTT fnt> class SimNodeT = SimNode_ExtFuncCallRef, typename QQ = defaultTempFn>
#endif
    inline auto addExternTempRef ( Module & mod, const ModuleLibrary & lib, const char * name, SideEffects seFlags,
        const char * cppName = nullptr, QQ && tempFn = QQ() )
    {
#if DAS_SLOW_CALL_INTEROP
        using SimNodeType = SimNodeT<FuncT>;
        auto fnX = make_smart<ExternalFn<FuncT, SimNodeType, FuncT>>(fn, name, lib, cppName);
#else
        using SimNodeType = SimNodeT<FuncT, fn>;
        auto fnX = make_smart<ExternalFn<FuncT, fn, SimNodeType, FuncT>>(name, lib, cppName);
#endif
        tempFn(fnX.get());
        fnX->result->temporary = true;
        addExternFunc(mod, fnX, SimNodeType::IS_CMRES, seFlags);
        return fnX;
    }

    template <InteropFunction func, typename RetT, typename ...Args>
    inline auto addInterop ( Module & mod, const ModuleLibrary & lib, const char * name, SideEffects seFlags,
                                   const char * cppName = nullptr ) {
        auto fnX = make_smart<InteropFn<func, RetT, Args...>>(name, lib, cppName);
        addExternFunc(mod, fnX, true, seFlags);
        return fnX;
    }

    template <typename CType, typename ...Args>
    inline auto addCtor ( Module & mod, const ModuleLibrary & lib, const char * name, const char * cppName = nullptr ) {
        auto fn = make_smart<BuiltIn_PlacementNew<CType,Args...>>(name,lib,cppName);
        DAS_ASSERT(fn->result->isRefType() && "can't add ctor to by-value types");
        mod.addFunction(fn);
        return fn;
    }

    template <typename CType, typename ...Args>
    inline auto addUsing ( Module & mod, const ModuleLibrary & lib, const char * cppName ) {
        auto fn = make_smart<BuiltIn_Using<CType,Args...>>(lib,cppName);
        mod.addFunction(fn);
        return fn;
    }

    template <typename CType, typename ...Args>
    inline auto addCtorAndUsing ( Module & mod, const ModuleLibrary & lib, const char * name, const char * cppName ) {
        auto fn = make_smart<BuiltIn_PlacementNew<CType,Args...>>(name,lib,cppName);
        DAS_ASSERT(fn->result->isRefType() && "can't add ctor to by-value types");
        mod.addFunction(fn);
        mod.addFunction(make_smart<BuiltIn_Using<CType,Args...>>(lib,cppName));
        return fn;
    }

    template <typename CType, typename IType>   // this is for the multiple inheritance
    void with_interface ( CType & shape, const TBlock<void,IType> & block, Context * context, LineInfoArg * at ) {
        das_invoke<void>::invoke<IType&>(context,at,block,shape);
    }

    template <typename ET>
    inline void addEnumFlagOps ( Module & mod, ModuleLibrary & lib, const string & cppName ) {
        using method_not = das_operator_enum_NOT<ET>;
        addExtern<ET (*)(ET a),method_not::invoke>(mod, lib, "~", SideEffects::none,
            ("das_operator_enum_NOT<" + cppName + ">::invoke").c_str());
        using method_or = das_operator_enum_OR<ET>;
        addExtern<ET (*)(ET,ET),method_or::invoke>(mod, lib, "|", SideEffects::none,
            ("das_operator_enum_OR<" + cppName + ">::invoke").c_str());
        using method_xor = das_operator_enum_XOR<ET>;
        addExtern<ET (*)(ET,ET),method_xor::invoke>(mod, lib, "^", SideEffects::none,
            ("das_operator_enum_XOR<" + cppName + ">::invoke").c_str());
        using method_and = das_operator_enum_AND<ET>;
        addExtern<ET (*)(ET,ET),method_and::invoke>(mod, lib, "&", SideEffects::none,
            ("das_operator_enum_AND<" + cppName + ">::invoke").c_str());
        using method_and_and = das_operator_enum_AND_AND<ET>;
        addExtern<bool (*)(ET,ET),method_and_and::invoke>(mod, lib, "&&", SideEffects::none,
            ("das_operator_enum_AND_AND<" + cppName + ">::invoke").c_str());
        using method_or_equ = das_operator_enum_OR_EQU<ET>;
        addExtern<void (*)(ET&,ET),method_or_equ::invoke>(mod, lib, "|=", SideEffects::modifyArgument,
            ("das_operator_enum_OR_EQU<" + cppName + ">::invoke").c_str());
        using method_xor_equ = das_operator_enum_XOR_EQU<ET>;
        addExtern<void (*)(ET&,ET),method_xor_equ::invoke>(mod, lib, "^=", SideEffects::modifyArgument,
            ("das_operator_enum_XOR_EQU<" + cppName + ">::invoke").c_str());
        using method_and_equ = das_operator_enum_AND_EQU<ET>;
        addExtern<void (*)(ET&,ET),method_and_equ::invoke>(mod, lib, "&=", SideEffects::modifyArgument,
            ("das_operator_enum_AND_EQU<" + cppName + ">::invoke").c_str());
    }
}

