#include "daScript/misc/platform.h"

#include "module_builtin.h"

#include "daScript/ast/ast_serializer.h"
#include "daScript/ast/ast_interop.h"
#include "daScript/ast/ast_handle.h"
#include "daScript/simulate/aot_builtin.h"
#include "daScript/simulate/runtime_profile.h"
#include "daScript/simulate/hash.h"
#include "daScript/simulate/bin_serializer.h"
#include "daScript/simulate/runtime_array.h"
#include "daScript/simulate/runtime_range.h"
#include "daScript/simulate/runtime_string_delete.h"
#include "daScript/simulate/simulate_nodes.h"
#include "daScript/simulate/aot.h"
#include "daScript/simulate/interop.h"
#include "daScript/misc/sysos.h"
#include "daScript/misc/fpe.h"
#include "daScript/simulate/debug_print.h"
#include "../parser/parser_impl.h"

MAKE_TYPE_FACTORY(HashBuilder, HashBuilder)

namespace das
{
#if defined(__clang__)
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Woverloaded-virtual"
#endif

    struct MarkFunctionAnnotation : FunctionAnnotation {
        MarkFunctionAnnotation(const string & na) : FunctionAnnotation(na) { }
        virtual bool apply(ExprBlock *, ModuleGroup &, const AnnotationArgumentList &, string & err) override {
            err = "not supported for block";
            return false;
        }
        virtual bool finalize(ExprBlock *, ModuleGroup &,const AnnotationArgumentList &, const AnnotationArgumentList &, string &) override {
            return true;
        }
        virtual bool finalize(const FunctionPtr &, ModuleGroup &, const AnnotationArgumentList &, const AnnotationArgumentList &, string &) override {
            return true;
        }
    };

    struct MarkFunctionOrBlockAnnotation : FunctionAnnotation {
        MarkFunctionOrBlockAnnotation() : FunctionAnnotation("marker") { }
        virtual bool apply ( const FunctionPtr &, ModuleGroup &, const AnnotationArgumentList &, string & ) override {
            return true;
        }
        virtual bool apply(ExprBlock *, ModuleGroup &, const AnnotationArgumentList &, string &) override {
            return true;
        }
        virtual bool finalize(ExprBlock *, ModuleGroup &,const AnnotationArgumentList &, const AnnotationArgumentList &, string &) override {
            return true;
        }
        virtual bool finalize(const FunctionPtr &, ModuleGroup &, const AnnotationArgumentList &, const AnnotationArgumentList &, string &) override {
            return true;
        }
    };

    struct MacroFunctionAnnotation : MarkFunctionAnnotation {
        MacroFunctionAnnotation() : MarkFunctionAnnotation("_macro") { }
        virtual bool apply(const FunctionPtr & func, ModuleGroup &, const AnnotationArgumentList &, string &) override {
            func->macroInit = true;
            auto program = daScriptEnvironment::getBound()->g_Program;
            program->needMacroModule = true;
            return true;
        };
    };

    struct MacroFnFunctionAnnotation : MarkFunctionAnnotation {
        MacroFnFunctionAnnotation() : MarkFunctionAnnotation("macro_function") { }
        virtual bool apply(const FunctionPtr & func, ModuleGroup &, const AnnotationArgumentList &, string &) override {
            func->macroFunction = true;
            return true;
        };
    };

    struct RequestJitFunctionAnnotation : MarkFunctionAnnotation {
        RequestJitFunctionAnnotation() : MarkFunctionAnnotation("jit") { }
        virtual bool apply(const FunctionPtr & func, ModuleGroup &, const AnnotationArgumentList &, string &) override {
            func->requestJit = true;
            return true;
        };
    };

    struct RequestNoJitFunctionAnnotation : MarkFunctionAnnotation {
        RequestNoJitFunctionAnnotation() : MarkFunctionAnnotation("no_jit") { }
        virtual bool apply(const FunctionPtr & func, ModuleGroup &, const AnnotationArgumentList &, string &) override {
            func->requestNoJit = true;
            return true;
        };
    };

    struct RequestNoDiscardFunctionAnnotation : MarkFunctionAnnotation {
        RequestNoDiscardFunctionAnnotation() : MarkFunctionAnnotation("nodiscard") { }
        virtual bool apply(const FunctionPtr & func, ModuleGroup &, const AnnotationArgumentList &, string &) override {
            func->nodiscard = true;
            return true;
        };
    };

    struct DeprecatedFunctionAnnotation : MarkFunctionAnnotation {
        DeprecatedFunctionAnnotation() : MarkFunctionAnnotation("deprecated") { }
        virtual bool apply(const FunctionPtr & func, ModuleGroup &, const AnnotationArgumentList &, string &) override {
            func->deprecated = true;
            return true;
        };
        virtual bool verifyCall ( ExprCallFunc * call, const AnnotationArgumentList & args,
            const AnnotationArgumentList & /*progArgs */, string & /*err*/ ) override {
            DAS_ASSERT(daScriptEnvironment::getBound()->g_compilerLog);
            (*daScriptEnvironment::getBound()->g_compilerLog) << call->at.describe() << ": *warning* function " << call->func->name << " is deprecated\n";
            if ( auto arg = args.find("message",Type::tString) ) {
                (*daScriptEnvironment::getBound()->g_compilerLog) << "\t" << arg->sValue << "\n";
            }
            return true;
        }
    };

    struct TypeFunctionFunctionAnnotation : MarkFunctionAnnotation {
        TypeFunctionFunctionAnnotation() : MarkFunctionAnnotation("type_function") { }
        virtual bool apply(const FunctionPtr & func, ModuleGroup &, const AnnotationArgumentList &, string & error) override {
            if ( !daScriptEnvironment::getBound()->g_Program->thisModule->addTypeFunction(func->name, true) ) {
                error = "can't add type function. type function " + func->name + " already exists?";
                return false;
            }
            return true;
        };
    };

    void Function::notInferred() {
        inferredSource.clear();
        isFullyInferred = false;
    }

    FunctionPtr Function::setDeprecated(const string & message) {
        deprecated = true; // this is instead of apply above
        AnnotationDeclarationPtr decl = make_smart<AnnotationDeclaration>();
        decl->arguments.push_back(AnnotationArgument("message",message));
        decl->annotation = make_smart<DeprecatedFunctionAnnotation>();
        annotations.push_back(decl);
        return this;
    }

    struct NeverAliasCMRESFunctionAnnotation : MarkFunctionAnnotation {
        NeverAliasCMRESFunctionAnnotation() : MarkFunctionAnnotation("never_alias_cmres") { }
        virtual bool apply(const FunctionPtr & func, ModuleGroup &, const AnnotationArgumentList &, string &) override {
            func->neverAliasCMRES = true;
            return true;
        };
    };

    struct AliasCMRESFunctionAnnotation : MarkFunctionAnnotation {
        AliasCMRESFunctionAnnotation() : MarkFunctionAnnotation("alias_cmres") { }
        virtual bool apply(const FunctionPtr & func, ModuleGroup &, const AnnotationArgumentList &, string &) override {
            func->aliasCMRES = true;
            return true;
        };
    };


    // dummy annotation for optimization hints on functions or blocks
    struct HintFunctionAnnotation : FunctionAnnotation {
        HintFunctionAnnotation() : FunctionAnnotation("hint") { }
        virtual bool apply(const FunctionPtr &, ModuleGroup &, const AnnotationArgumentList &, string &) override {
            return true;
        };
        virtual bool apply(ExprBlock *, ModuleGroup &, const AnnotationArgumentList &, string & ) override {
            return true;
        }
        virtual bool finalize(ExprBlock *, ModuleGroup &,const AnnotationArgumentList &, const AnnotationArgumentList &, string &) override {
            return true;
        }
        virtual bool finalize(const FunctionPtr &, ModuleGroup &, const AnnotationArgumentList &, const AnnotationArgumentList &, string &) override {
            return true;
        }
    };

    struct MakeFunctionUnsafeCallMacro : CallMacro {
        MakeFunctionUnsafeCallMacro() : CallMacro("make_function_unsafe") { }
        virtual ExpressionPtr visit (  Program * prog, Module *, ExprCallMacro * call ) override {
            if ( !call->inFunction ) {
                prog->error("make_function_unsafe can only be used inside a function", "", "", call->at);
                return nullptr;
            }
            call->inFunction->unsafeOperation = true;
            return make_smart<ExprConstBool>(call->at, true);
        }
    };

    struct UnsafeDerefFunctionAnnotation : MarkFunctionAnnotation {
        UnsafeDerefFunctionAnnotation() : MarkFunctionAnnotation("unsafe_deref") { }
        virtual bool apply(const FunctionPtr & func, ModuleGroup &, const AnnotationArgumentList &, string &) override {
            func->unsafeDeref = true;
            return true;
        };
    };

    struct GenericFunctionAnnotation : MarkFunctionAnnotation {
        GenericFunctionAnnotation() : MarkFunctionAnnotation("generic") { }
        virtual bool isGeneric() const override {
            return true;
        }
        virtual bool apply(const FunctionPtr &, ModuleGroup &, const AnnotationArgumentList &, string &) override {
            return true;
        };
    };

    struct NoLintFunctionAnnotation : MarkFunctionAnnotation {
        NoLintFunctionAnnotation() : MarkFunctionAnnotation("no_lint") { }
        virtual bool apply(const FunctionPtr &, ModuleGroup &, const AnnotationArgumentList &, string &) override {
            return true;
        };
    };

    struct ExportFunctionAnnotation : MarkFunctionAnnotation {
        ExportFunctionAnnotation() : MarkFunctionAnnotation("export") { }
        virtual bool apply(const FunctionPtr & func, ModuleGroup &, const AnnotationArgumentList &, string &) override {
            func->exports = true;
            return true;
        };
    };

    struct PInvokeFunctionAnnotation : MarkFunctionAnnotation {
        PInvokeFunctionAnnotation() : MarkFunctionAnnotation("pinvoke") { }
        virtual bool apply(const FunctionPtr & func, ModuleGroup &, const AnnotationArgumentList &, string &) override {
            func->pinvoke = true;
            return true;
        };
    };

    struct SideEffectsFunctionAnnotation : MarkFunctionAnnotation {
        SideEffectsFunctionAnnotation() : MarkFunctionAnnotation("sideeffects") { }
        virtual bool apply(const FunctionPtr & func, ModuleGroup &, const AnnotationArgumentList &, string &) override {
            func->sideEffectFlags |= uint32_t(SideEffects::userScenario);
            return true;
        };
    };

    struct RunAtCompileTimeFunctionAnnotation : MarkFunctionAnnotation {
        RunAtCompileTimeFunctionAnnotation() : MarkFunctionAnnotation("run") { }
        virtual bool apply(const FunctionPtr & func, ModuleGroup &, const AnnotationArgumentList &, string &) override {
            func->hasToRunAtCompileTime = true;
            return true;
        };
    };

    struct UnsafeOpFunctionAnnotation : MarkFunctionAnnotation {
        UnsafeOpFunctionAnnotation() : MarkFunctionAnnotation("unsafe_operation") { }
        virtual bool apply(const FunctionPtr & func, ModuleGroup &, const AnnotationArgumentList &, string &) override {
            func->unsafeOperation = true;
            return true;
        };
    };

    struct UnsafeOutsideOfForFunctionAnnotation : MarkFunctionAnnotation {
        UnsafeOutsideOfForFunctionAnnotation() : MarkFunctionAnnotation("unsafe_outside_of_for") { }
        virtual bool apply(const FunctionPtr & func, ModuleGroup &, const AnnotationArgumentList &, string &) override {
            func->unsafeOutsideOfFor = true;
            return true;
        };
    };

    struct UnsafeWhenNotCloneArray : MarkFunctionAnnotation {
        UnsafeWhenNotCloneArray() : MarkFunctionAnnotation("unsafe_when_not_clone_array") { }
        virtual bool apply(const FunctionPtr & func, ModuleGroup &, const AnnotationArgumentList &, string &) override {
            func->unsafeWhenNotCloneArray = true;
            return true;
        };
    };

    struct NoAotFunctionAnnotation : MarkFunctionAnnotation {
        NoAotFunctionAnnotation() : MarkFunctionAnnotation("no_aot") { }
        virtual bool apply(const FunctionPtr & func, ModuleGroup &, const AnnotationArgumentList &, string &) override {
            func->noAot = true;
            return true;
        };
    };

    struct InitFunctionAnnotation : MarkFunctionAnnotation {
        InitFunctionAnnotation() : MarkFunctionAnnotation("init") { }
        virtual bool apply(const FunctionPtr & func, ModuleGroup &, const AnnotationArgumentList & args, string &) override {
            func->init = true;
            for ( auto & arg : args ) {
                if ( arg.name=="late" && arg.type == Type::tBool ) {
                    func->lateInit = arg.bValue;
                } else if ( arg.name=="tag" || arg.name=="before" || arg.name=="after" ) {
                    func->lateInit = true;
                }
            }
            return true;
        };
        virtual bool finalize(const FunctionPtr & func, ModuleGroup &, const AnnotationArgumentList &, const AnnotationArgumentList &, string & errors) override {
            if ( func->arguments.size() ) {
                errors += "[init] function can't have any arguments";
                return false;
            }
            if ( !func->result->isVoid() ) {
                errors += "[init] function can't return value";
                return false;
            }
            return true;
        }
    };

    struct FinalizeFunctionAnnotation : MarkFunctionAnnotation {
        FinalizeFunctionAnnotation() : MarkFunctionAnnotation("finalize") { }
        virtual bool apply(const FunctionPtr & func, ModuleGroup &, const AnnotationArgumentList & args, string &) override {
            func->shutdown = true;
            for ( auto & arg : args ) {
                if ( arg.name=="late" && arg.type == Type::tBool ) {
                    func->lateShutdown = arg.bValue;
                }
            }
            return true;
        };
        virtual bool finalize(const FunctionPtr & func, ModuleGroup &, const AnnotationArgumentList &, const AnnotationArgumentList &, string & errors) override {
            if ( func->arguments.size() ) {
                errors += "[finalize] function can't have any arguments";
                return false;
            }
            if ( !func->result->isVoid() ) {
                errors += "[finalize] function can't return value";
                return false;
            }
            return true;
        }
    };

    struct MarkUsedFunctionAnnotation : MarkFunctionAnnotation {
        MarkUsedFunctionAnnotation() : MarkFunctionAnnotation("unused_argument") { }
        virtual bool apply(const FunctionPtr & func, ModuleGroup &, const AnnotationArgumentList & args, string &) override {
            for ( auto & fnArg : func->arguments ) {
                if ( auto optArg = args.find(fnArg->name, Type::tBool) ) {
                    fnArg->marked_used = optArg->bValue;
                }
            }
            return true;
        };
        virtual bool apply(ExprBlock * block, ModuleGroup &, const AnnotationArgumentList & args, string &) override {
            for ( auto & bArg : block->arguments ) {
                if ( auto optArg = args.find(bArg->name, Type::tBool) ) {
                    bArg->marked_used = optArg->bValue;
                }
            }
            return true;
        };
    };

    struct ArgumentTemplateAnnotation : MarkFunctionAnnotation {
        ArgumentTemplateAnnotation( const string & na ) : MarkFunctionAnnotation(na) {}
        virtual bool isSpecialized() const override { return true; }
        virtual bool apply(const FunctionPtr & fn, ModuleGroup &, const AnnotationArgumentList & decl, string & err) override {
            for ( const auto & arg : decl ) {
                if ( arg.type!=Type::tBool ) {
                    err = "expecting names only";
                    return false;
                }
                if ( !fn->findArgument(arg.name) ) {
                    err = "unknown argument " + arg.name;
                    return false;
                }
            }
            return true;
        }
    };


    struct IsYetAnotherVectorTemplateAnnotation : ArgumentTemplateAnnotation {
        IsYetAnotherVectorTemplateAnnotation() : ArgumentTemplateAnnotation("expect_any_vector") {}
        virtual bool isCompatible ( const FunctionPtr & fn, const vector<TypeDeclPtr> & types,
                const AnnotationDeclaration & decl, string & err  ) const override {
            size_t maxIndex = min(types.size(), fn->arguments.size());
            for ( size_t ai=0; ai!=maxIndex; ++ ai) {
                if ( decl.arguments.find(fn->arguments[ai]->name, Type::tBool) ) {
                    const auto & argT = types[ai];
                    if ( !(argT->isHandle() && argT->annotation && argT->annotation->isYetAnotherVectorTemplate()) ) {
                        err = "argument " + fn->arguments[ai]->name + " is expected to be a vector template";
                        return false;
                    }
                }
            }
            return true;
        }
    };

    struct IsDimAnnotation : ArgumentTemplateAnnotation {
        IsDimAnnotation() : ArgumentTemplateAnnotation("expect_dim") {}
        virtual bool isCompatible ( const FunctionPtr & fn, const vector<TypeDeclPtr> & types,
                const AnnotationDeclaration & decl, string & err  ) const override {
            size_t maxIndex = min(types.size(), fn->arguments.size());
            for ( size_t ai=0; ai!=maxIndex; ++ ai) {
                if ( decl.arguments.find(fn->arguments[ai]->name, Type::tBool) ) {
                    const auto & argT = types[ai];
                    if ( argT->dim.size() == 0 ) {
                        err = "argument " + fn->arguments[ai]->name + " is expected to be a dim []";
                        return false;
                    }
                }
            }
            return true;
        }
    };

    struct IsRefTypeAnnotation : ArgumentTemplateAnnotation {
        IsRefTypeAnnotation() : ArgumentTemplateAnnotation("expect_ref") {}
        virtual bool isCompatible ( const FunctionPtr & fn, const vector<TypeDeclPtr> & types,
                const AnnotationDeclaration & decl, string & err  ) const override {
            size_t maxIndex = min(types.size(), fn->arguments.size());
            for ( size_t ai=0; ai!=maxIndex; ++ ai) {
                if ( decl.arguments.find(fn->arguments[ai]->name, Type::tBool) ) {
                    const auto & argT = types[ai];
                    if ( !argT->isRef() ) {
                        err = "argument " + fn->arguments[ai]->name + " is expected to be a ref type or a reference";
                        return false;
                    }
                }
            }
            return true;
        }
    };

    // totally dummy annotation, needed for comments
    struct CommentAnnotation : StructureAnnotation {
        CommentAnnotation() : StructureAnnotation("comment") {}
        virtual bool touch(const StructurePtr &, ModuleGroup &,
                           const AnnotationArgumentList &, string & ) override {
            return true;
        }
        virtual bool look ( const StructurePtr &, ModuleGroup &,
                           const AnnotationArgumentList &, string & ) override {
            return true;
        }
    };

    struct NoDefaultCtorAnnotation : StructureAnnotation {
        NoDefaultCtorAnnotation() : StructureAnnotation("no_default_initializer") {}
        virtual bool touch(const StructurePtr & ps, ModuleGroup &,
                           const AnnotationArgumentList &, string & ) override {
            ps->genCtor = true;
            ps->noGenCtor = true;
            return true;
        }
        virtual bool look ( const StructurePtr &, ModuleGroup &,
                           const AnnotationArgumentList &, string & ) override {
            return true;
        }
    };

    struct MacroInterfaceAnnotation : StructureAnnotation {
        MacroInterfaceAnnotation() : StructureAnnotation("macro_interface") {}
        virtual bool touch(const StructurePtr & ps, ModuleGroup &,
                           const AnnotationArgumentList &, string & ) override {
            ps->macroInterface = true;
            return true;
        }
        virtual bool look ( const StructurePtr &, ModuleGroup &,
                           const AnnotationArgumentList &, string & ) override {
            return true;
        }
    };


    struct HybridFunctionAnnotation : MarkFunctionAnnotation {
        HybridFunctionAnnotation() : MarkFunctionAnnotation("hybrid") { }
        virtual bool apply(const FunctionPtr & func, ModuleGroup &, const AnnotationArgumentList &, string &) override {
            func->aotHybrid = true;
            return true;
        };
    };

    struct CppAlignmentAnnotation : StructureAnnotation {
        CppAlignmentAnnotation() : StructureAnnotation("cpp_layout") {}
        virtual bool touch(const StructurePtr & ps, ModuleGroup &,
                           const AnnotationArgumentList & args, string & ) override {
            ps->cppLayout = true;
            ps->cppLayoutNotPod = !args.getBoolOption("pod", true);
            return true;
        }
        virtual bool look ( const StructurePtr &, ModuleGroup &,
                           const AnnotationArgumentList &, string & ) override {
            return true;
        }
    };

    struct SafeWhenUninitializedAnnotation : StructureAnnotation {
        SafeWhenUninitializedAnnotation() : StructureAnnotation("safe_when_uninitialized") {}
        virtual bool touch(const StructurePtr & ps, ModuleGroup &,
                           const AnnotationArgumentList &, string & ) override {
            ps->safeWhenUninitialized = true;
            return true;
        }
        virtual bool look ( const StructurePtr &, ModuleGroup &,
                           const AnnotationArgumentList &, string & ) override {
            return true;
        }
    };

    struct LocalOnlyFunctionAnnotation : FunctionAnnotation {
        LocalOnlyFunctionAnnotation() : FunctionAnnotation("local_only") { }
        virtual bool apply ( ExprBlock *, ModuleGroup &, const AnnotationArgumentList &, string & err ) override {
            err = "not a block annotation";
            return false;
        }
        virtual bool finalize ( ExprBlock *, ModuleGroup &, const AnnotationArgumentList &,
                               const AnnotationArgumentList &, string & err ) override {
            err = "not a block annotation";
            return false;
        }
        virtual bool apply ( const FunctionPtr &, ModuleGroup &, const AnnotationArgumentList &, string & ) override {
            return true;
        };
        virtual bool finalize ( const FunctionPtr &, ModuleGroup &, const AnnotationArgumentList &,
                               const AnnotationArgumentList &, string & ) override {
            return true;
        }
        // [local_only ()]
        virtual bool verifyCall ( ExprCallFunc * call, const AnnotationArgumentList & args,
                const AnnotationArgumentList &, string & err ) override {
            if ( !call->func ) {
                err = "unknown function";
                return false;
            }
            for ( size_t i=0, is=call->func->arguments.size(); i!=is; ++i ) {
                auto & farg = call->func->arguments[i];
                if ( auto it = args.find(farg->name, Type::tBool) ) {
                    auto & carg = call->arguments[i];
                    bool isLocalArg = carg->rtti_isMakeLocal() || carg->rtti_isMakeTuple();
                    bool isLocalFArg = it->bValue;
                    if ( isLocalArg != isLocalFArg ) {
                        err = isLocalFArg ? "expecting [[...]]" : "not expecting [[...]]";
                        return false;
                    }
                }
            }
            return true;
        }
    };

    struct PersistentStructureAnnotation : StructureAnnotation {
        PersistentStructureAnnotation() : StructureAnnotation("persistent") {}
        virtual bool touch(const StructurePtr & ps, ModuleGroup &,
                           const AnnotationArgumentList &, string & ) override {
            ps->persistent = true;
            return true;
        }
        virtual bool look ( const StructurePtr & st, ModuleGroup &,
                           const AnnotationArgumentList & args, string & errors ) override {
            bool allPod = true;
            if ( !args.getBoolOption("mixed_heap", false) ) {
                for ( const auto & field : st->fields ) {
                    if ( !field.type->isPod() ) {
                        allPod = false;
                        errors += "\t'" + field.name + ": " + field.type->describe() + "' is not a POD\n";
                    }
                }
            }
            return allPod;
        }
    };


    struct LogicOpAnnotation : FunctionAnnotation {
        LogicOpAnnotation(const string & name) : FunctionAnnotation(name) { }
        virtual bool apply ( const FunctionPtr &, ModuleGroup &, const AnnotationArgumentList &, string & ) override { return true; }
        virtual bool apply(ExprBlock *, ModuleGroup &, const AnnotationArgumentList &, string &) override { return true; }
        virtual bool finalize(ExprBlock *, ModuleGroup &,const AnnotationArgumentList &, const AnnotationArgumentList &, string &) override { return true; }
        virtual bool finalize(const FunctionPtr &, ModuleGroup &, const AnnotationArgumentList &, const AnnotationArgumentList &, string &) override { return true; }
        virtual bool isSpecialized() const override { return true; }
    };

    struct LogicOpNotAnnotation : LogicOpAnnotation {
        LogicOpNotAnnotation() : LogicOpAnnotation("!") { }
        LogicOpNotAnnotation(const AnnotationDeclarationPtr & decl) : LogicOpAnnotation("!"), subexpr(decl) { }
        virtual bool isCompatible ( const FunctionPtr & fun, const vector<TypeDeclPtr> & types, const AnnotationDeclaration &, string & errors  ) const override  {
            if ( !subexpr ) return false;
            return !((FunctionAnnotation *)(subexpr->annotation.get()))->isCompatible(fun, types, *subexpr, errors);
        }
        virtual void appendToMangledName( const FunctionPtr & fun, const AnnotationDeclaration &, string & mangledName ) const override {
            if ( !subexpr ) return;
            string mna ;
            ((FunctionAnnotation *)(subexpr->annotation.get()))->appendToMangledName(fun, *subexpr, mna);
            mangledName = "!(" + mna + ")";
        }
        virtual void log ( TextWriter & ss, const AnnotationDeclaration & ) const override {
            if ( !subexpr ) return;
            ss << "!(";
            subexpr->annotation->log(ss, *subexpr);
            ss << ")";
        }
        virtual void serialize ( AstSerializer & ser ) override {
            ser << subexpr;
        }
        AnnotationDeclarationPtr subexpr;
    };

    struct LogicOp2Annotation : LogicOpAnnotation {
        LogicOp2Annotation ( const string & name ) : LogicOpAnnotation(name) { }
        LogicOp2Annotation ( const string & name, const AnnotationDeclarationPtr & arg0, const AnnotationDeclarationPtr & arg1 )
            : LogicOpAnnotation(name), left(arg0), right(arg1) { }
        virtual void appendToMangledName( const FunctionPtr & fun, const AnnotationDeclaration &, string & mangledName ) const override {
            if ( !left || !right ) return;
            string mna1, mna2;
            if ( left->annotation ) {
                ((FunctionAnnotation *)(left->annotation.get()))->appendToMangledName(fun, *left, mna1);
            } else {
                mna1 = "NULL";
            }
            if ( right->annotation ) {
                ((FunctionAnnotation *)(right->annotation.get()))->appendToMangledName(fun, *right, mna2);
            } else {
                mna2 = "NULL";
            }
            mangledName = "(" + mna1 + name + mna2 + ")";
        }
        virtual void log ( TextWriter & ss, const AnnotationDeclaration & ) const override  {
            if ( !left || !right ) return;
            ss << "(";
            if ( left->annotation ) {
                left->annotation->log(ss, *left);
            } else {
                ss << "NULL";
            }
            ss << " " << name << " ";
            if ( right->annotation ) {
                right->annotation->log(ss, *right);
            } else {
                ss << "NULL";
            }
            ss << ")";
        }
        virtual void serialize ( AstSerializer & ser ) override {
            ser << left << right;
        }
        AnnotationDeclarationPtr left, right;
    };

    struct LogicOpAndAnnotation : LogicOp2Annotation {
        LogicOpAndAnnotation ( ) : LogicOp2Annotation("&&") { }
        LogicOpAndAnnotation ( const AnnotationDeclarationPtr & arg0, const AnnotationDeclarationPtr & arg1 )
            : LogicOp2Annotation("&&",arg0,arg1) { }
        virtual bool isCompatible ( const FunctionPtr & fun, const vector<TypeDeclPtr> & types, const AnnotationDeclaration &, string & errors  ) const override  {
            if ( !left || !right ) return false;
            return ((FunctionAnnotation *)(left->annotation.get()))->isCompatible(fun, types, *left, errors) &&
                ((FunctionAnnotation *)(right->annotation.get()))->isCompatible(fun, types, *right, errors);
        }
    };

    struct LogicOpOrAnnotation : LogicOp2Annotation {
        LogicOpOrAnnotation ( ) : LogicOp2Annotation("||") { }
        LogicOpOrAnnotation ( const AnnotationDeclarationPtr & arg0, const AnnotationDeclarationPtr & arg1 )
            : LogicOp2Annotation("||",arg0,arg1) { }
        virtual bool isCompatible ( const FunctionPtr & fun, const vector<TypeDeclPtr> & types, const AnnotationDeclaration &, string & errors  ) const override  {
            if ( !left || !right ) return false;
            return ((FunctionAnnotation *)(left->annotation.get()))->isCompatible(fun, types, *left, errors) ||
                ((FunctionAnnotation *)(right->annotation.get()))->isCompatible(fun, types, *right, errors);
        }
    };

    struct LogicOpXOrAnnotation : LogicOp2Annotation {
        LogicOpXOrAnnotation ( ) : LogicOp2Annotation("^^") { }
        LogicOpXOrAnnotation ( const AnnotationDeclarationPtr & arg0, const AnnotationDeclarationPtr & arg1 )
            : LogicOp2Annotation("^^",arg0,arg1) { }
        virtual bool isCompatible ( const FunctionPtr & fun, const vector<TypeDeclPtr> & types, const AnnotationDeclaration &, string & errors  ) const override {
            if ( !left || !right ) return false;
            return ((FunctionAnnotation *)(left->annotation.get()))->isCompatible(fun, types, *left, errors) !=
                ((FunctionAnnotation *)(right->annotation.get()))->isCompatible(fun, types, *right, errors);
        }
    };

    AnnotationPtr newLogicAnnotation ( LogicAnnotationOp op, const AnnotationDeclarationPtr & arg0, const AnnotationDeclarationPtr & arg1 ) {
        switch ( op ) {
        case LogicAnnotationOp::Not:    return make_smart<LogicOpNotAnnotation>(arg0);
        case LogicAnnotationOp::And:    return make_smart<LogicOpAndAnnotation>(arg0,arg1);
        case LogicAnnotationOp::Or:     return make_smart<LogicOpOrAnnotation>(arg0,arg1);
        case LogicAnnotationOp::Xor:    return make_smart<LogicOpXOrAnnotation>(arg0,arg1);
        }
        return nullptr;
    }

    AnnotationPtr newLogicAnnotation ( LogicAnnotationOp op ) {
        switch ( op ) {
        case LogicAnnotationOp::Not:    return make_smart<LogicOpNotAnnotation>();
        case LogicAnnotationOp::And:    return make_smart<LogicOpAndAnnotation>();
        case LogicAnnotationOp::Or:     return make_smart<LogicOpOrAnnotation>();
        case LogicAnnotationOp::Xor:    return make_smart<LogicOpXOrAnnotation>();
        }
        return nullptr;
    }

#if defined(__clang__)
#pragma clang diagnostic pop
#endif

    template <typename intT>
    struct EnumIterator : Iterator {
        EnumIterator ( EnumInfo * ei, LineInfo * at ) : Iterator(at), info(ei) {}
        virtual bool first ( Context &, char * _value ) override {
            count = 0;
            range_to = int32_t(info->count);
            if ( range_to ) {
                intT * value = (intT *) _value;
                *value      = intT(info->fields[count++]->value);
                return true;
            } else {
                return false;
            }
        }
        virtual bool next  ( Context &, char * _value ) override {
            if ( range_to != count ) {
                intT * value = (intT *) _value;
                *value = intT(info->fields[count++]->value);
                return true;
            } else {
                return false;
            }
        }
        virtual void close ( Context & context, char * ) override {
            context.freeIterator((char *)this, debugInfo);
        }
        EnumInfo *  info = nullptr;
        int32_t     count = 0;
        int32_t     range_to = 0;
    };

    struct CountIterator : Iterator {
        CountIterator ( int32_t _start, int32_t _step, LineInfo * at ) : Iterator(at), start(_start), step(_step) {}
        virtual bool first ( Context &, char * _value ) override {
            int32_t * value = (int32_t *) _value;
            *value = start;
            return true;
        }
        virtual bool next  ( Context &, char * _value ) override {
            int32_t * value = (int32_t *) _value;
            *value += step;
            return true;
        }
        virtual void close ( Context & context, char * ) override {
            context.freeIterator((char *)this, debugInfo);
        }
        int32_t start = 0;
        int32_t step = 0;
    };

    TSequence<int32_t> builtin_count ( int32_t start, int32_t step, Context * context, LineInfoArg * at ) {
        char * iter = context->allocateIterator(sizeof(CountIterator), "count iterator", at);
        new (iter) CountIterator(start, step, at);
        return TSequence<int>((Iterator *)iter);
    }

    TSequence<uint32_t> builtin_ucount ( uint32_t start, uint32_t step, Context * context, LineInfoArg * at ) {
        char * iter = context->allocateIterator(sizeof(CountIterator), "ucount iterator", at);
        new (iter) CountIterator(start, step, at);
        return TSequence<int>((Iterator *)iter);
    }

    // core functions

    void builtin_throw ( char * text, Context * context, LineInfoArg * at ) {
        context->throw_error_at(at, "%s", text);
    }

    void builtin_print ( char * text, Context * context, LineInfoArg * at ) {
        context->to_out(at, text);
    }

    void builtin_feint ( char *, Context *, LineInfoArg * ) {
        // this function intentionally does nothing. its a fair replacement for the print, where we don't want print
    }

    void builtin_error ( char * text, Context * context, LineInfoArg * at ) {
        context->to_err(at, text);
    }

    vec4f builtin_breakpoint ( Context & context, SimNode_CallBase * call, vec4f * ) {
        context.breakPoint(call->debugInfo);
        return v_zero();
    }

    void builtin_stackwalk ( bool args, bool vars, Context * context, LineInfoArg * lineInfo ) {
        context->stackWalk(lineInfo, args, vars);
    }

    void builtin_terminate ( Context * context, LineInfoArg * at ) {
        context->throw_error_at(at, "terminate");
    }

    int builtin_table_size ( const Table & arr ) {
        return arr.size;
    }

    int builtin_table_capacity ( const Table & arr ) {
        return arr.capacity;
    }

    void builtin_table_clear ( Table & arr, Context * context, LineInfoArg * at ) {
        table_clear(*context, arr, at);
    }

    vec4f builtin_table_reserve ( Context & context, SimNode_CallBase * call, vec4f * args ) {
        // table, size
        if ( !call->types ) {
            context.throw_error_at(call->debugInfo, "missing type info");
        }
        auto ttype = call->types[0];
        if ( !ttype->firstType || !ttype->secondType ) {
            context.throw_error_at(call->debugInfo, "expecting table type");
        }
        Type baseType = call->types[0]->firstType->type;
        uint32_t valueTypeSize = call->types[0]->secondType->size;
        Table & tab = cast<Table&>::to(args[0]);
        uint32_t newCapacity = cast<uint32_t>::to(args[1]);
        table_reserve_impl(context, tab, baseType, newCapacity, valueTypeSize, &call->debugInfo);
        return v_zero();
    }


    struct HashBuilderAnnotation : ManagedStructureAnnotation <HashBuilder,false> {
        HashBuilderAnnotation(ModuleLibrary & ml)
            : ManagedStructureAnnotation ("HashBuilder", ml) {
        }
    };

    vec4f _builtin_hash ( Context & context, SimNode_CallBase * call, vec4f * args ) {
        auto uhash = hash_value(context, args[0], call->types[0]);
        return cast<uint64_t>::from(uhash);
    }

    void heap_stats ( Context & ctx, uint64_t * bytes ) {
        bytes[0] = ctx.heap->bytesAllocated();
        bytes[1] = ctx.stringHeap->bytesAllocated();
    }

    urange64 heap_allocation_stats ( Context * context ) {
        return urange64 ( context->heap->getTotalBytesAllocated(), context->heap->getTotalBytesDeleted() );
    }

    uint64_t heap_allocation_count ( Context * context ) {
        return context->heap->getTotalAllocations();
    }

    urange64 string_heap_allocation_stats ( Context * context ) {
        return urange64 ( context->stringHeap->getTotalBytesAllocated(), context->stringHeap->getTotalBytesDeleted() );
    }

    uint64_t string_heap_allocation_count ( Context * context ) {
        return context->stringHeap->getTotalAllocations();
    }

    uint64_t heap_bytes_allocated ( Context * context ) {
        return context->heap->bytesAllocated();
    }

    int32_t heap_depth ( Context * context ) {
        return (int32_t) context->heap->depth();
    }

    uint64_t string_heap_bytes_allocated ( Context * context ) {
        return context->stringHeap->bytesAllocated();
    }

    int32_t string_heap_depth ( Context * context ) {
        return (int32_t) context->stringHeap->depth();
    }

    void string_heap_report ( Context * context, LineInfoArg * info ) {
        context->stringHeap->report();
        context->reportAnyHeap(info, true, false, false, false);
    }

    bool is_intern_strings ( Context * context ) {
        return context->stringHeap->isIntern();
    }

    void heap_collect ( bool sheap, bool validate, Context * context, LineInfoArg * info ) {
        if ( !context->persistent ) {
            context->throw_error_at(info, "heap collection is not allowed in this context, needs 'options persistent'");
        }
        if ( !context->gcEnabled ) {
            context->throw_error_at(info, "heap collection is not allowed in this context, needs 'options gc'");
        }
        context->collectHeap(info, sheap, validate);
    }

    void heap_report ( Context * context, LineInfoArg * info ) {
        context->heap->report();
        context->reportAnyHeap(info, false, true, false, false);
    }

    void memory_report ( bool errOnly, Context * context, LineInfoArg * info ) {
        /*
        context->stringHeap->report();
        context->heap->report();
        */
        context->reportAnyHeap(info,true,true,false,errOnly);
    }

    void builtin_table_lock ( Table & arr, Context * context, LineInfoArg * at ) {
        table_lock(*context, arr, at);
    }

    void builtin_table_unlock ( Table & arr, Context * context, LineInfoArg * at ) {
        table_unlock(*context, arr, at);
    }

    void builtin_table_lock_mutable ( const Table & arr, Context * context, LineInfoArg * at ) {
        table_lock(*context, const_cast<Table&>(arr), at);
    }

    void builtin_table_unlock_mutable ( const Table & arr, Context * context, LineInfoArg * at ) {
        table_unlock(*context, const_cast<Table&>(arr), at);
    }

    void builtin_table_clear_lock ( const Table & arr, Context * ) {
        const_cast<Table&>(arr).hopeless = 0;
    }

    bool builtin_iterator_first ( Sequence & it, void * data, Context * context, LineInfoArg * at ) {
        if ( !it.iter ) context->throw_error_at(at, "calling first on empty iterator");
        else if ( it.iter->isOpen ) context->throw_error_at(at, "calling first on already open iterator");
        it.iter->isOpen = true;
        return it.iter->first(*context, (char *)data);
    }

    bool builtin_iterator_next ( Sequence & it, void * data, Context * context, LineInfoArg * at ) {
        if ( !it.iter ) context->throw_error_at(at, "calling next on empty iterator");
        else if ( !it.iter->isOpen ) context->throw_error_at(at, "calling next on a non-open iterator");
        return it.iter->next(*context, (char *)data);
    }

    void builtin_iterator_close ( Sequence & it, void * data, Context * context ) {
        if ( it.iter ) {
            it.iter->close(*context, (char *)&data);
            it.iter = nullptr;
        }
    }

    void builtin_iterator_delete ( Sequence & it, Context * context ) {
        if ( it.iter ) {
            it.iter->close(*context, nullptr);
            it.iter = nullptr;
        }
    }

    bool builtin_iterator_iterate ( Sequence & it, void * value, Context * context ) {
        if ( !it.iter ) {
            return false;
        } else if ( !it.iter->isOpen) {
            if ( !it.iter->first(*context, (char *)value) ) {
                it.iter->close(*context, (char *)value);
                it.iter = nullptr;
                return false;
            } else {
                it.iter->isOpen = true;
                return true;
            }
        } else {
            if ( !it.iter->next(*context, (char *)value) ) {
                it.iter->close(*context, (char *)value);
                it.iter = nullptr;
                return false;
            } else {
                return true;
            }
        }
    }

    void builtin_make_good_array_iterator ( Sequence & result, const Array & arr, int stride, Context * context, LineInfoArg * at ) {
        char * iter = context->allocateIterator(sizeof(GoodArrayIterator), "array<> iterator", at);
        new (iter) GoodArrayIterator((Array *)&arr, stride, at);
        result = { (Iterator *) iter };
    }

    void builtin_make_fixed_array_iterator ( Sequence & result, void * data, int size, int stride, Context * context, LineInfoArg * at ) {
        char * iter = context->allocateIterator(sizeof(FixedArrayIterator), "fixed array iterator", at);
        new (iter) FixedArrayIterator((char *)data, size, stride, at);
        result = { (Iterator *) iter };
    }

    void builtin_make_range_iterator ( Sequence & result, range rng, Context * context, LineInfoArg * at ) {
        char * iter = context->allocateIterator(sizeof(RangeIterator<range>), "range iterator", at);
        new (iter) RangeIterator<range>(rng, at);
        result = { (Iterator *) iter };
    }

    void builtin_make_urange_iterator ( Sequence & result, urange rng, Context * context, LineInfoArg * at ) {
        char * iter = context->allocateIterator(sizeof(RangeIterator<urange>), "range iterator", at);
        new (iter) RangeIterator<urange>(rng, at);
        result = { (Iterator *) iter };
    }

    void builtin_make_range64_iterator ( Sequence & result, range64 rng, Context * context, LineInfoArg * at ) {
        char * iter = context->allocateIterator(sizeof(RangeIterator<range64>), "range64 iterator", at);
        new (iter) RangeIterator<range64>(rng, at);
        result = { (Iterator *) iter };
    }

    void builtin_make_urange64_iterator ( Sequence & result, urange64 rng, Context * context, LineInfoArg * at ) {
        char * iter = context->allocateIterator(sizeof(RangeIterator<urange64>), "urange64 iterator", at);
        new (iter) RangeIterator<urange64>(rng, at);
        result = { (Iterator *) iter };
    }

    vec4f builtin_make_enum_iterator ( Context & context, SimNode_CallBase * call, vec4f * args ) {
        if ( !call->types ) context.throw_error_at(call->debugInfo, "missing type info");
        auto itinfo = call->types[0];
        if ( itinfo->type != Type::tIterator ) context.throw_error_at(call->debugInfo, "not an iterator");
        auto tinfo = itinfo->firstType;
        if ( !tinfo ) context.throw_error_at(call->debugInfo, "missing iterator type info");
        if ( tinfo->type!=Type::tEnumeration && tinfo->type!=Type::tEnumeration8
            && tinfo->type!=Type::tEnumeration16 && tinfo->type!=Type::tEnumeration64 ) {
            context.throw_error_at(call->debugInfo, "not an iterator of enumeration");
        }
        auto einfo = tinfo->enumType;
        if ( !einfo ) context.throw_error_at(call->debugInfo, "missing enumeraiton type info");
        char * iter = nullptr;
        switch ( tinfo->type ) {
        case Type::tEnumeration:
            iter = context.allocateIterator(sizeof(EnumIterator<int32_t>), "enum iterator", &call->debugInfo);
            new (iter) EnumIterator<int32_t>(einfo, &call->debugInfo);
            break;
        case Type::tEnumeration8:
            iter = context.allocateIterator(sizeof(EnumIterator<int8_t>), "enum8 iterator", &call->debugInfo);
            new (iter) EnumIterator<int8_t>(einfo, &call->debugInfo);
            break;
        case Type::tEnumeration16:
            iter = context.allocateIterator(sizeof(EnumIterator<int16_t>), "enum16 iterator", &call->debugInfo);
            new (iter) EnumIterator<int16_t>(einfo, &call->debugInfo);
            break;
        case Type::tEnumeration64:
            iter = context.allocateIterator(sizeof(EnumIterator<int64_t>), "enum64 iterator", &call->debugInfo);
            new (iter) EnumIterator<int64_t>(einfo, &call->debugInfo);
            break;
        default:
            DAS_ASSERT(0 && "how???");
        }
        Sequence * seq = cast<Sequence *>::to(args[0]);
        seq->iter = (Iterator *) iter;
        return v_zero();
    }

    void builtin_make_string_iterator ( Sequence & result, char * str, Context * context, LineInfoArg * at ) {
        char * iter = context->allocateIterator(sizeof(StringIterator), "string iterator", at);
        new (iter) StringIterator(str, at);
        result = { (Iterator *) iter };
    }

    struct NilIterator : Iterator {
        NilIterator(LineInfo * at) : Iterator(at) {}
        virtual bool first ( Context &, char * ) override { return false; }
        virtual bool next  ( Context &, char * ) override { return false; }
        virtual void close ( Context & context, char * ) override {
            context.freeIterator((char *)this, debugInfo);
        }
    };

    void builtin_make_nil_iterator ( Sequence & result, Context * context, LineInfoArg * at ) {
        char * iter = context->allocateIterator(sizeof(NilIterator), "nil iterator", at);
        new (iter) NilIterator(at);
        result = { (Iterator *) iter };
    }

    struct LambdaIterator : Iterator {
        using lambdaFunc = bool (*) (Context *,void*, char*);
        LambdaIterator ( Context & context, const Lambda & ll, int st, LineInfo * at ) : Iterator(at), lambda(ll), stride(st) {
            SimFunction ** fnMnh = (SimFunction **) lambda.capture;
            if (!fnMnh) context.throw_error("invoke null lambda");
            simFunc = *fnMnh;
            if (!simFunc) context.throw_error("invoke null function");
            aotFunc = (lambdaFunc) simFunc->aotFunction;
        }

        DAS_SUPPRESS_UB
        __forceinline bool InvokeLambda ( Context & context, char * ptr ) {
            if ( aotFunc ) {
                return (*aotFunc) ( &context, lambda.capture, ptr );
            } else {
                vec4f argValues[4] = {
                    cast<Lambda>::from(lambda),
                    cast<char *>::from(ptr)
                };
                auto res = context.call(simFunc, argValues, 0);
                return cast<bool>::to(res);
            }
        }
        virtual bool first ( Context & context, char * ptr ) override {
            memset(ptr, 0, stride);
            return InvokeLambda(context, ptr);
        }
        virtual bool next  ( Context & context, char * ptr ) override {
            return InvokeLambda(context, ptr);
        }
        virtual void close ( Context & context, char * ) override {
            SimFunction ** fnMnh = (SimFunction **) lambda.capture;
            SimFunction * finFunc = fnMnh[1];
            if (!finFunc) context.throw_error("generator finalizer is a null function");
            vec4f argValues[1] = {
                cast<void *>::from(lambda.capture)
            };
            auto flags = context.stopFlags; // need to save stop flags, we can be in the middle of some return or something
            context.call(finFunc, argValues, 0);
            context.freeIterator((char *)this, debugInfo);
            context.stopFlags = flags;
        }
        virtual void walk ( DataWalker & walker ) override {
            walker.beforeLambda(&lambda, lambda.getTypeInfo());
            walker.walk(lambda.capture, lambda.getTypeInfo());
            walker.afterLambda(&lambda, lambda.getTypeInfo());
        }
        Lambda          lambda;
        SimFunction *   simFunc = nullptr;
        lambdaFunc      aotFunc = nullptr;
        int             stride = 0;
    };

    void builtin_make_lambda_iterator ( Sequence & result, const Lambda lambda, int stride, Context * context, LineInfoArg * at ) {
        char * iter = context->allocateIterator(sizeof(LambdaIterator), "lambda iterator", at);
        new (iter) LambdaIterator(*context, lambda, stride, at);
        result = { (Iterator *) iter };
    }

    void resetProfiler( Context * context ) {
        context->resetProfiler();
    }

    void dumpProfileInfo( Context * context ) {
        LOG tp(LogLevel::debug);
        context->collectProfileInfo(tp);
    }

    char * collectProfileInfo( Context * context, LineInfoArg * at ) {
        TextWriter tout;
        context->collectProfileInfo(tout);
        return context->allocateString(tout.str(), at);
    }

    void builtin_array_free ( Array & dim, int szt, Context * __context__, LineInfoArg * at ) {
        if ( dim.data ) {
            if ( !dim.isLocked() || dim.hopeless ) {
                uint32_t oldSize = dim.capacity*szt;
                __context__->free(dim.data, oldSize, at);
            } else {
                __context__->throw_error_at(at, "can't delete locked array");
            }
            if ( dim.hopeless ) {
                memset ( &dim, 0, sizeof(Array) );
                dim.hopeless = true;
            } else {
                memset ( &dim, 0, sizeof(Array) );
            }
        }
    }

    void builtin_table_free ( Table & tab, int szk, int szv, Context * __context__, LineInfoArg * at ) {
        if ( tab.data ) {
            if ( !tab.isLocked() || tab.hopeless ) {
                uint32_t oldSize = tab.capacity*(szk+szv+sizeof(TableHashKey));
                __context__->free(tab.data, oldSize, at);
            } else {
                __context__->throw_error_at(at, "can't delete locked table");
            }
            if ( tab.hopeless ) {
                memset ( &tab, 0, sizeof(Table) );
                tab.hopeless = true;
            } else {
                memset ( &tab, 0, sizeof(Table) );
            }
        }
    }

    __forceinline bool smart_ptr_is_valid (const smart_ptr_raw<void> dest ) {
        if (!dest.get()) return true;
        ptr_ref_count * t = (ptr_ref_count *) dest.get();
        return t->is_valid();
    }

    __forceinline void validate_smart_ptr ( void * ptr, Context * context, LineInfoArg * at ) {
        if ( !ptr ) return;
        ptr_ref_count * t = (ptr_ref_count *) ptr;
        if ( !t->is_valid() ) context->throw_error_at(at, "invalid smart pointer %p", ptr);
    }

    void builtin_smart_ptr_move_new ( smart_ptr_raw<void> & dest, smart_ptr_raw<void> src, Context * context, LineInfoArg * at ) {
        validate_smart_ptr(src.ptr, context, at);
        validate_smart_ptr(dest.ptr, context, at);
        ptr_ref_count * t = (ptr_ref_count *) dest.ptr;
        dest.ptr = src.ptr;
        src.ptr = nullptr;
        if ( t ) t->delRef();
    }

    void builtin_smart_ptr_move_ptr ( smart_ptr_raw<void> & dest, const void * src, Context * context, LineInfoArg * at ) {
        validate_smart_ptr((void *)src, context, at);
        validate_smart_ptr(dest.ptr, context, at);
        ptr_ref_count * t = (ptr_ref_count *) dest.ptr;
        dest.ptr = (void *) src;
        if ( t ) t->delRef();
    }

    void builtin_smart_ptr_move ( smart_ptr_raw<void> & dest, smart_ptr_raw<void> & src, Context * context, LineInfoArg * at ) {
        validate_smart_ptr(src.ptr, context, at);
        validate_smart_ptr(dest.ptr, context, at);
        ptr_ref_count * t = (ptr_ref_count *) dest.ptr;
        dest.ptr = src.ptr;
        src.ptr = nullptr;
        if ( t ) t->delRef();
    }

    void builtin_smart_ptr_clone_ptr ( smart_ptr_raw<void> & dest, const void * src, Context * context, LineInfoArg * at ) {
        validate_smart_ptr((void *)src, context, at);
        validate_smart_ptr(dest.ptr, context, at);
        ptr_ref_count * t = (ptr_ref_count *) dest.ptr;
        dest.ptr = (void *) src;
        if ( src ) ((ptr_ref_count *) src)->addRef();
        if ( t ) t->delRef();
    }

    void builtin_smart_ptr_clone ( smart_ptr_raw<void> & dest, const smart_ptr_raw<void> src, Context * context, LineInfoArg * at ) {
        validate_smart_ptr(src.ptr, context, at);
        validate_smart_ptr(dest.ptr, context, at);
        ptr_ref_count * t = (ptr_ref_count *) dest.ptr;
        dest.ptr = src.ptr;
        if ( src.ptr ) ((ptr_ref_count *) src.ptr)->addRef();
        if ( t ) t->delRef();
    }

    uint32_t builtin_smart_ptr_use_count ( const smart_ptr_raw<void> src, Context * context, LineInfoArg * at ) {
        validate_smart_ptr(src.ptr, context, at);
        ptr_ref_count * psrc = (ptr_ref_count *) src.ptr;
        return psrc ? psrc->use_count() : 0;
    }

    struct ClassInfoMacro : TypeInfoMacro {
        ClassInfoMacro() : TypeInfoMacro("rtti_classinfo") {}
        virtual TypeDeclPtr getAstType ( ModuleLibrary & lib, const ExpressionPtr &, string & ) override {
            return typeFactory<void *>::make(lib);
        }
        virtual SimNode * simluate ( Context * context, const ExpressionPtr & expr, string & )  override {
            auto exprTypeInfo = static_pointer_cast<ExprTypeInfo>(expr);
            TypeInfo * typeInfo = context->thisHelper->makeTypeInfo(nullptr, exprTypeInfo->typeexpr);
            return context->code->makeNode<SimNode_TypeInfo>(expr->at, typeInfo);
        }
        virtual void aotPrefix ( TextWriter & ss, const ExpressionPtr & ) override {
            ss << "(void *)(&";
        }
        virtual void aotSuffix ( TextWriter & ss, const ExpressionPtr & ) override {
            ss << ")";
        }
        virtual bool aotNeedTypeInfo ( const ExpressionPtr & ) const override {
            return true;
        }
    };

    bool is_compiling (  ) {
        if ( daScriptEnvironment::getBound() && daScriptEnvironment::getBound()->g_Program ) {
            return daScriptEnvironment::getBound()->g_Program->isCompiling || daScriptEnvironment::getBound()->g_Program->isSimulating;
        }
        return false;

    }

    DAS_API uint64_t get_context_share_counter ( Context * context ) {
        return (uint64_t) context->code.use_count();
    }

    bool is_compiling_macros ( ) {
        if ( daScriptEnvironment::getBound() && daScriptEnvironment::getBound()->g_Program ) {
            return daScriptEnvironment::getBound()->g_Program->isCompilingMacros;
        }
        return false;
    }

    bool is_compiling_macros_in_module ( char * name ) {
        if ( daScriptEnvironment::getBound() && daScriptEnvironment::getBound()->g_Program ) {
            if ( !daScriptEnvironment::getBound()->g_Program->isCompilingMacros ) return false;
            if ( daScriptEnvironment::getBound()->g_Program->thisModule->name != to_rts(name) ) return false;
            if ( isInDebugAgentCreation() ) return false;
            return true;
        }
        return true;
    }

    bool is_reporting_compilation_errors ( ) {
        if ( daScriptEnvironment::getBound() && daScriptEnvironment::getBound()->g_Program ) {
            return daScriptEnvironment::getBound()->g_Program->reportingInferErrors;
        }
        return false;
    }

    // static storage

    using TStaticStorage = das_hash_map<uint64_t, void*>;
    DAS_THREAD_LOCAL(TStaticStorage) g_static_storage;

    void gc0_save_ptr ( char * name, void * data, Context * context, LineInfoArg * line ) {
        uint64_t hash = hash_function ( *context, name );
        if ( g_static_storage->find(hash)!=g_static_storage->end() ) {
            context->throw_error_at(line, "gc0 already there %s (or hash collision)", name);
        }
        (*g_static_storage)[hash] = data;
    }

    void gc0_save_smart_ptr ( char * name, smart_ptr_raw<void> data, Context * context, LineInfoArg * line ) {
        gc0_save_ptr(name, data.get(), context, line);
    }

    void * gc0_restore_ptr ( char * name, Context * context ) {
        uint64_t hash = hash_function ( *context, name );
        auto it = g_static_storage->find(hash);
        if ( it!=g_static_storage->end() ) {
            void * res = it->second;
            g_static_storage->erase(it);
            return res;
        } else {
            return nullptr;
        }
    }

    smart_ptr_raw<void> gc0_restore_smart_ptr ( char * name, Context * context ) {
        return gc0_restore_ptr(name,context);
    }

    void gc0_reset() {
        TStaticStorage dummy;
        swap ( *g_static_storage, dummy );
    }

    __forceinline void i_das_ptr_inc ( void * & ptr, int stride ) {
        ptr = (char*) ptr + stride;
    }

    __forceinline void i_das_ptr_dec ( void * & ptr, int stride ) {
        ptr = (char*) ptr - stride;
    }

    // int32_t
    __forceinline void * i_das_ptr_add_int32 ( void * ptr, int32_t value, int32_t stride )     { return (char*) ptr + value * stride; }
    __forceinline void * i_das_ptr_sub_int32 ( void * & ptr, int32_t value, int32_t stride )   { return (char*) ptr - value * stride; }
    __forceinline void i_das_ptr_set_add_int32 ( void * & ptr, int32_t value, int32_t stride ) { ptr = (char*) ptr + value * stride; }
    __forceinline void i_das_ptr_set_sub_int32 ( void * & ptr, int32_t value, int32_t stride ) { ptr = (char*) ptr - value * stride; }
    // int64_t
    __forceinline void * i_das_ptr_add_int64 ( void * ptr, int64_t value, int32_t stride )     { return (char*) ptr + value * stride; }
    __forceinline void * i_das_ptr_sub_int64 ( void * & ptr, int64_t value, int32_t stride )   { return (char*) ptr - value * stride; }
    __forceinline void i_das_ptr_set_add_int64 ( void * & ptr, int64_t value, int32_t stride ) { ptr = (char*) ptr + value * stride; }
    __forceinline void i_das_ptr_set_sub_int64 ( void * & ptr, int64_t value, int32_t stride ) { ptr = (char*) ptr - value * stride; }
    // uint32_t
    __forceinline void * i_das_ptr_add_uint32 ( void * ptr, uint32_t value, int32_t stride )     { return (char*) ptr + value * stride; }
    __forceinline void * i_das_ptr_sub_uint32 ( void * & ptr, uint32_t value, int32_t stride )   { return (char*) ptr - value * stride; }
    __forceinline void i_das_ptr_set_add_uint32 ( void * & ptr, uint32_t value, int32_t stride ) { ptr = (char*) ptr + value * stride; }
    __forceinline void i_das_ptr_set_sub_uint32 ( void * & ptr, uint32_t value, int32_t stride ) { ptr = (char*) ptr - value * stride; }
    // uint64_t
    __forceinline void * i_das_ptr_add_uint64 ( void * ptr, uint64_t value, int32_t stride )     { return (char*) ptr + value * stride; }
    __forceinline void * i_das_ptr_sub_uint64 ( void * & ptr, uint64_t value, int32_t stride )   { return (char*) ptr - value * stride; }
    __forceinline void i_das_ptr_set_add_uint64 ( void * & ptr, uint64_t value, int32_t stride ) { ptr = (char*) ptr + value * stride; }
    __forceinline void i_das_ptr_set_sub_uint64 ( void * & ptr, uint64_t value, int32_t stride ) { ptr = (char*) ptr - value * stride; }

    void* builtin_das_aligned_alloc16(uint64_t size) {
        return das_aligned_alloc16((size_t)size);
    }

    Module_BuiltIn::~Module_BuiltIn() {
        gc0_reset();
    }

    struct UnescapedStringMacro : public ReaderMacro {
        UnescapedStringMacro ( ) : ReaderMacro("_esc") {}
        virtual ExpressionPtr visit ( Program *, Module *, ExprReader * expr ) override {
            return make_smart<ExprConstString>(expr->at,expr->sequence);
        }
        virtual bool accept ( Program *, Module *, ExprReader * re, int Ch, const LineInfo & ) override {
            if ( Ch==-1 ) return false;
            re->sequence.push_back(char(Ch));
            auto sz = re->sequence.size();
            if ( sz>5 ) {
                auto tail = re->sequence.c_str() + sz - 5;
                if ( strcmp(tail,"%_esc")==0 ) {
                    re->sequence.resize(sz - 5);
                    return false;
                }
            }
            return true;
        }
    };

    TypeDeclPtr makePrintFlags() {
        auto ft = make_smart<TypeDecl>(Type::tBitfield);
        ft->alias = "print_flags";
        ft->argNames = { "escapeString", "namesAndDimensions", "typeQualifiers", "refAddresses", "singleLine", "fixedPoint", "fullTypeInfo" };
        return ft;
    }

    template <>
    struct typeFactory<PrintFlags> {
        static TypeDeclPtr make(const ModuleLibrary &) {
            return makePrintFlags();
        }
    };

    vec4f builtin_sprint ( Context & context, SimNode_CallBase * call, vec4f * args ) {
        TextWriter ssw;
        auto typeInfo = call->types[0];
        auto res = args[0];
        auto flags = cast<uint32_t>::to(args[1]);
        ssw << debug_type(typeInfo) << " = " << debug_value(res, typeInfo, PrintFlags(flags));
        auto sres = context.allocateString(ssw.str(),&call->debugInfo);
        return cast<char *>::from(sres);
    }

    vec4f builtin_json_sprint ( Context & context, SimNode_CallBase * call, vec4f * args ) {
        auto typeInfo = call->types[0];
        auto res = args[0];
        auto humanReadable = cast<bool>::to(args[1]);
        auto ssw = debug_json_value(res, typeInfo, humanReadable);
        auto sres = context.allocateString(ssw,&call->debugInfo);
        return cast<char *>::from(sres);
    }

    vec4f builtin_json_sscan ( Context & context, SimNode_CallBase * call, vec4f * args ) {
        auto json = cast<char *>::to(args[0]);
        if ( !json ) return cast<bool>::from(false);
        auto typeInfo = call->types[1];
        char * dst;
        if ( typeInfo->flags & TypeInfo::flag_refType ) {
            dst = cast<char *>::to(args[1]);
        } else {
            dst = (char *)&args[1];
        }
        uint32_t jsonLen = uint32_t(strlen(json));
        bool ok = debug_json_scan(context, dst, typeInfo, json, jsonLen, &call->debugInfo);
        return cast<bool>::from(ok);
    }

    Array  g_CommandLineArguments;

    void setCommandLineArguments ( int argc, char * argv[] ) {
        array_mark_locked(g_CommandLineArguments, (char *)argv, uint32_t(argc));
    }

    void getCommandLineArguments( Array & arr ) {
        arr = g_CommandLineArguments;
    }

    void withCommandLineArguments( const Array & arr, const TBlock<void> & body, Context * context, LineInfoArg * at ) {
        auto prev = g_CommandLineArguments;
        g_CommandLineArguments = arr;
        context->invoke(body, nullptr, nullptr, at);
        g_CommandLineArguments = prev;
    }

    char * builtin_das_root ( Context * context, LineInfoArg * at ) {
        return context->allocateString(getDasRoot(), at);
    }

    char * builtin_get_das_version ( Context * context, LineInfoArg * at ) {
        char buf[32];
        snprintf(buf, sizeof(buf), "%d.%d.%d", DAS_VERSION_MAJOR, DAS_VERSION_MINOR, DAS_VERSION_PATCH);
        return context->allocateString(string(buf), at);
    }

    char * to_das_string(const string & str, Context * ctx, LineInfoArg * at) {
        return ctx->allocateString(str, at);
    }

    const char * pass_string(const char * str) {
        return str;
    }

    char * clone_pass_string(char * str, Context * ctx, LineInfoArg * at ) {
        if ( !str ) return nullptr;
        return ctx->allocateString(str, at);
    }

    void set_das_string(string & str, const char * bs) {
        str = bs ? bs : "";
    }

    void set_string_das(char * & bs, const string & str, Context * ctx, LineInfoArg * at ) {
        bs = ctx->allocateString(str, at);
    }

    void peek_das_string(const string & str, const TBlock<void,TTemporary<const char *>> & block, Context * context, LineInfoArg * at) {
        vec4f args[1];
        args[0] = cast<const char *>::from(str.c_str());
        context->invoke(block, args, nullptr, at);
    }

    char * builtin_string_clone ( const char *str, Context * context, LineInfoArg * at ) {
        const uint32_t strLen = stringLengthSafe ( *context, str );
        if (!strLen)
            return nullptr;
        return context->allocateString(str, strLen, at);
    }

    void builtin_temp_array ( void * data, int size, const Block & block, Context * context, LineInfoArg * at ) {
        Array arr;
        array_mark_locked(arr, (char *)data, uint32_t(size));
        vec4f args[1];
        args[0] = cast<Array &>::from(arr);
        context->invoke(block, args, nullptr, at);
    }

    void builtin_make_temp_array ( Array & arr, void * data, int size ) {
        array_mark_locked(arr, (char *)data, uint32_t(size));
    }

    void toLog ( int level, const char * text, Context * context, LineInfoArg * at ) {
        context->to_out(at, level, text);
    }

    void toCompilerLog ( const char * text, Context * context, LineInfoArg * at ) {
        if ( daScriptEnvironment::getBound()->g_compilerLog ) {
            if ( text ) {
                (*daScriptEnvironment::getBound()->g_compilerLog) << text;
            }
        } else {
            context->throw_error_at(at, "can only write to compiler log during compilation");
        }
    }

    bool das_is_dll_build() {
        #if DAS_ENABLE_DLL
        return true;
        #else
        return false;
        #endif
    }

    bool is_in_aot ( ) {
        return daScriptEnvironment::getBound() ? daScriptEnvironment::getBound()->g_isInAot : false;
    }

    void set_aot ( ) {
        assert(daScriptEnvironment::getBound());
        daScriptEnvironment::getBound()->g_isInAot = true;
    }
    void reset_aot ( ) {
        assert(daScriptEnvironment::getBound());
        daScriptEnvironment::getBound()->g_isInAot = false;
    }

    bool is_in_completion ( ) {
        if ( daScriptEnvironment::getBound() && daScriptEnvironment::getBound()->g_Program ) {
            return daScriptEnvironment::getBound()->g_Program->policies.completion;
        }
        return false;
    }

    bool is_folding ( ) {
        if ( daScriptEnvironment::getBound() && daScriptEnvironment::getBound()->g_Program ) {
            return daScriptEnvironment::getBound()->g_Program->folding;
        }
        return false;
    }

    const char * compiling_file_name ( ) {
        return daScriptEnvironment::getBound() ? daScriptEnvironment::getBound()->g_compilingFileName : nullptr;
    }

    const char * compiling_module_name ( ) {
        return daScriptEnvironment::getBound() ? daScriptEnvironment::getBound()->g_compilingModuleName : nullptr;
    }

    const char * get_module_file_name ( const char * name, Context * context ) {
        if ( context && context->thisProgram ) {
            string modName = name ? name : "";
            if ( modName.empty() ) {
                // return the main module's file name
                auto mod = context->thisProgram->thisModule.get();
                if ( mod && !mod->fileName.empty() ) return mod->fileName.c_str();
            } else {
                // search program's library for named module
                const char * result = nullptr;
                context->thisProgram->library.foreach([&](Module * mod) {
                    if ( mod->name == modName && !mod->fileName.empty() ) {
                        result = mod->fileName.c_str();
                        return false;
                    }
                    return true;
                }, modName);
                if ( result ) return result;
            }
        }
        // fallback to global module list
        auto mod = Module::require(name ? name : "");
        if ( mod && !mod->fileName.empty() ) {
            return mod->fileName.c_str();
        }
        return nullptr;
    }

// remove define to enable emscripten version
#define TRY_MAIN_LOOP   0

#ifdef _EMSCRIPTEN_
#if TRY_MAIN_LOOP
    struct MainLoopArg {
        Context * context;
        LineInfoArg * at;
        Block * block;
    };

    void main_loop_arg ( void * arg ) {
        auto mla = (MainLoopArg *) arg;
        vec4f args[1];
        args[0] = v_zero();
        if ( !cast<bool>::to(mla->context->invoke(*mla->block, args, nullptr, mla->at)) ) {
            emscripten_cancel_main_loop();
        }
    }
#endif
#endif

    void builtin_main_loop ( const TBlock<bool> & block, Context * context, LineInfoArg * at ) {
#ifndef _EMSCRIPTEN_
        vec4f args[1];
        args[0] = v_zero();
        while ( true ) {
            auto res = context->invoke(block, args, nullptr, at);
            if ( !cast<bool>::to(res) ) break;
        }
#else
#if TRY_MAIN_LOOP
    MainLoopArg arg;
    arg.context = context;
    arg.at = at;
    arg.block = &block;
    emscripten_set_main_loop_arg(main_loop_arg, &arg, 60, true);
#endif
#endif
    }

    bool das_is_jit_function ( const Func func ) {
        auto simfn = func.PTR;
        if ( !simfn ) return false;
        return simfn->code && simfn->code->rtti_node_isJit();
    }

    bool das_jit_enabled ( Context * context, LineInfoArg * at ) {
        if ( !context->thisProgram ) context->throw_error_at(at, "can only query for jit during compilation");
        return context->thisProgram->policies.jit_enabled;
    }

    bool das_aot_enabled ( Context * context, LineInfoArg * at ) {
        if ( !context->thisProgram ) context->throw_error_at(at, "can only query for aot during compilation");
        return context->thisProgram->policies.aot;
    }

#define STR_DSTR_REG(OPNAME,EXPR) \
    addExtern<DAS_BIND_FUN(OPNAME##_str_dstr)>(*this, lib, #EXPR, SideEffects::none, DAS_TOSTRING(OPNAME##_str_dstr)); \
    addExtern<DAS_BIND_FUN(OPNAME##_dstr_str)>(*this, lib, #EXPR, SideEffects::none, DAS_TOSTRING(OPNAME##_dstr_str));

    void PointerOp ( const FunctionPtr & idpi ) {
        idpi->unsafeOperation = true;
        idpi->firstArgReturnType = true;
        idpi->noPointerCast = true;
    }

    // windows, darwin, linux, etc
    const char * das_get_platform_name() {
        #if defined(_WIN32) || defined(_WIN64)
            return "windows";
        #elif defined(__APPLE__) || defined(__MACH__)
            return "darwin";
        #elif defined(__linux__)
            return "linux";
        #elif defined(__EMSCRIPTEN__)
            return "emscripten";
        #else
            return "unknown";
        #endif
    }

    // x86, arm, etc
    const char * das_get_architecture_name() {
        #if defined(__x86_64__) || defined(_M_X64)
            return "x86_64";
        #elif defined(__i386) || defined(_M_IX86)
            return "x86";
        #elif defined(__aarch64__)
            return "arm64";
        #elif defined(__arm__) || defined(_M_ARM)
            return "arm";
        #elif defined(__EMSCRIPTEN__)
            return "wasm32";
        #else
            return "unknown";
        #endif
    }

    void Module_BuiltIn::addRuntime(ModuleLibrary & lib) {
        // printer flags
        addAlias(makePrintFlags());
        // unescape macro
        addReaderMacro(make_smart<UnescapedStringMacro>());
        // function annotations
        addAnnotation(make_smart<CommentAnnotation>());
        addAnnotation(make_smart<NoDefaultCtorAnnotation>());
        addAnnotation(make_smart<MacroInterfaceAnnotation>());
        addAnnotation(make_smart<MarkFunctionOrBlockAnnotation>());
        addAnnotation(make_smart<CppAlignmentAnnotation>());
        addAnnotation(make_smart<SafeWhenUninitializedAnnotation>());
        addAnnotation(make_smart<GenericFunctionAnnotation>());
        addAnnotation(make_smart<MacroFunctionAnnotation>());
        addAnnotation(make_smart<MacroFnFunctionAnnotation>());
        addAnnotation(make_smart<HintFunctionAnnotation>());
        addAnnotation(make_smart<RequestJitFunctionAnnotation>());
        addAnnotation(make_smart<RequestNoJitFunctionAnnotation>());
        addAnnotation(make_smart<RequestNoDiscardFunctionAnnotation>());
        addAnnotation(make_smart<DeprecatedFunctionAnnotation>());
        addAnnotation(make_smart<AliasCMRESFunctionAnnotation>());
        addAnnotation(make_smart<NeverAliasCMRESFunctionAnnotation>());
        addAnnotation(make_smart<ExportFunctionAnnotation>());
        addAnnotation(make_smart<PInvokeFunctionAnnotation>());
        addAnnotation(make_smart<NoLintFunctionAnnotation>());
        addAnnotation(make_smart<SideEffectsFunctionAnnotation>());
        addAnnotation(make_smart<RunAtCompileTimeFunctionAnnotation>());
        addAnnotation(make_smart<UnsafeOpFunctionAnnotation>());
        addAnnotation(make_smart<UnsafeOutsideOfForFunctionAnnotation>());
        addAnnotation(make_smart<UnsafeWhenNotCloneArray>());
        addAnnotation(make_smart<NoAotFunctionAnnotation>());
        addAnnotation(make_smart<InitFunctionAnnotation>());
        addAnnotation(make_smart<FinalizeFunctionAnnotation>());
        addAnnotation(make_smart<HybridFunctionAnnotation>());
        addAnnotation(make_smart<UnsafeDerefFunctionAnnotation>());
        addAnnotation(make_smart<MarkUsedFunctionAnnotation>());
        addAnnotation(make_smart<LocalOnlyFunctionAnnotation>());
        addAnnotation(make_smart<PersistentStructureAnnotation>());
        addAnnotation(make_smart<IsYetAnotherVectorTemplateAnnotation>());
        addAnnotation(make_smart<IsDimAnnotation>());
        addAnnotation(make_smart<IsRefTypeAnnotation>());
        addAnnotation(make_smart<HashBuilderAnnotation>(lib));
        addAnnotation(make_smart<TypeFunctionFunctionAnnotation>());
        // and call macro
        {
            CallMacroPtr newM = make_smart<MakeFunctionUnsafeCallMacro>();
            addCallMacro(newM->name, [this, newM](const LineInfo & at) -> ExprLooksLikeCall * {
                auto ecm = new ExprCallMacro(at, newM->name);
                ecm->macro = newM.get();
                newM->module = this;
                return ecm;
            });
        }
        // string
        addAnnotation(make_smart<DasStringTypeAnnotation>());
        // typeinfo macros
        addTypeInfoMacro(make_smart<ClassInfoMacro>());
        // command line arguments
        addExtern<DAS_BIND_FUN(builtin_das_root)>(*this, lib, "get_das_root",
            SideEffects::accessExternal,"builtin_das_root")
                ->args({"context","at"});
        addExtern<DAS_BIND_FUN(builtin_get_das_version)>(*this, lib, "get_das_version",
            SideEffects::none,"builtin_get_das_version")
                ->args({"context","at"});
        addExtern<DAS_BIND_FUN(getCommandLineArguments)>(*this, lib, "builtin_get_command_line_arguments",
            SideEffects::accessExternal,"getCommandLineArguments")
                ->arg("arguments");
        addExtern<DAS_BIND_FUN(withCommandLineArguments)>(*this, lib,  "with_argv",
            SideEffects::invoke, "withArgv")
                ->args({"new_arguments", "block","context","line"});
        // compile-time functions
        addExtern<DAS_BIND_FUN(is_compiling)>(*this, lib, "is_compiling",
            SideEffects::accessExternal, "is_compiling");
        addExtern<DAS_BIND_FUN(is_compiling_macros)>(*this, lib, "is_compiling_macros",
            SideEffects::accessExternal, "is_compiling_macros");
        addExtern<DAS_BIND_FUN(is_compiling_macros_in_module)>(*this, lib, "is_compiling_macros_in_module",
            SideEffects::accessExternal, "is_compiling_macros_in_module")
                ->args({"name"});
        addExtern<DAS_BIND_FUN(is_reporting_compilation_errors)>(*this, lib, "is_reporting_compilation_errors",
            SideEffects::accessExternal, "is_reporting_compilation_errors");
        addExtern<DAS_BIND_FUN(get_context_share_counter)>(*this, lib, "get_context_share_counter",
            SideEffects::accessExternal, "get_context_share_counter")
                ->arg("context");
        // iterator functions
        addExtern<DAS_BIND_FUN(builtin_iterator_first)>(*this, lib, "_builtin_iterator_first",
            SideEffects::modifyArgumentAndExternal, "builtin_iterator_first")
                ->args({"iterator","data","context","at"});
        addExtern<DAS_BIND_FUN(builtin_iterator_next)>(*this, lib,  "_builtin_iterator_next",
            SideEffects::modifyArgumentAndExternal, "builtin_iterator_next")
                ->args({"iterator","data","context","at"});
        addExtern<DAS_BIND_FUN(builtin_iterator_close)>(*this, lib, "_builtin_iterator_close",
            SideEffects::modifyArgumentAndExternal, "builtin_iterator_close")
                ->args({"iterator","data","context"});
        addExtern<DAS_BIND_FUN(builtin_iterator_delete)>(*this, lib, "_builtin_iterator_delete",
            SideEffects::modifyArgumentAndExternal, "builtin_iterator_delete")
                ->args({"iterator","context"});
        addExtern<DAS_BIND_FUN(builtin_iterator_iterate)>(*this, lib, "_builtin_iterator_iterate",
            SideEffects::modifyArgumentAndExternal, "builtin_iterator_iterate")
                ->args({"iterator","data","context"});
        addExtern<DAS_BIND_FUN(builtin_iterator_empty)>(*this, lib, "empty",
            SideEffects::modifyArgumentAndExternal, "builtin_iterator_empty")
                ->arg("iterator");
        // count and ucount iterators
        auto fnCount = addExtern<DAS_BIND_FUN(builtin_count),SimNode_ExtFuncCallAndCopyOrMove>(*this, lib, "count",
            SideEffects::modifyExternal, "builtin_count")
                ->args({"start","step","context","at"})->setNoDiscard();
        fnCount->arguments[0]->init = make_smart<ExprConstInt>(0);  // start=0
        fnCount->arguments[1]->init = make_smart<ExprConstInt>(1);  // step=0
        auto fnuCount = addExtern<DAS_BIND_FUN(builtin_ucount),SimNode_ExtFuncCallAndCopyOrMove>(*this, lib, "ucount",
            SideEffects::none, "builtin_ucount")
                ->args({"start","step","context","at"})->setNoDiscard();
        fnuCount->arguments[0]->init = make_smart<ExprConstUInt>(0);  // start=0
        fnuCount->arguments[1]->init = make_smart<ExprConstUInt>(1);  // step=0
        // make-iterator functions
        addExtern<DAS_BIND_FUN(builtin_make_good_array_iterator)>(*this, lib,  "_builtin_make_good_array_iterator",
            SideEffects::modifyArgumentAndExternal, "builtin_make_good_array_iterator")
                ->args({"iterator","array","stride","context","at"});
        addExtern<DAS_BIND_FUN(builtin_make_fixed_array_iterator)>(*this, lib,  "_builtin_make_fixed_array_iterator",
            SideEffects::modifyArgumentAndExternal, "builtin_make_fixed_array_iterator")
                ->args({"iterator","data","size","stride","context","at"});
        addExtern<DAS_BIND_FUN(builtin_make_range_iterator)>(*this, lib,  "_builtin_make_range_iterator",
            SideEffects::modifyArgumentAndExternal, "builtin_make_range_iterator")
                ->args({"iterator","range","context","at"});
        addExtern<DAS_BIND_FUN(builtin_make_urange_iterator)>(*this, lib,  "_builtin_make_urange_iterator",
            SideEffects::modifyArgumentAndExternal, "builtin_make_urange_iterator")
                ->args({"iterator","range","context","at"});
        addExtern<DAS_BIND_FUN(builtin_make_range64_iterator)>(*this, lib,  "_builtin_make_range64_iterator",
            SideEffects::modifyArgumentAndExternal, "builtin_make_range64_iterator")
                ->args({"iterator","range","context","at"});
        addExtern<DAS_BIND_FUN(builtin_make_urange64_iterator)>(*this, lib,  "_builtin_make_urange64_iterator",
            SideEffects::modifyArgumentAndExternal, "builtin_make_urange64_iterator")
                ->args({"iterator","range","context","at"});
        addExtern<DAS_BIND_FUN(builtin_make_string_iterator)>(*this, lib,  "_builtin_make_string_iterator",
            SideEffects::modifyArgumentAndExternal, "builtin_make_string_iterator")
                ->args({"iterator","string","context","at"})->setCaptureString();
        addExtern<DAS_BIND_FUN(builtin_make_nil_iterator)>(*this, lib,  "_builtin_make_nil_iterator",
            SideEffects::modifyArgumentAndExternal, "builtin_make_nil_iterator")
                ->args({"iterator","context", "at"});
        addExtern<DAS_BIND_FUN(builtin_make_lambda_iterator)>(*this, lib,  "_builtin_make_lambda_iterator",
            SideEffects::modifyArgumentAndExternal, "builtin_make_lambda_iterator")
                ->args({"iterator","lambda","stride","context","at"});
        addInterop<builtin_make_enum_iterator,void,vec4f>(*this, lib, "_builtin_make_enum_iterator",
            SideEffects::modifyArgumentAndExternal, "builtin_make_enum_iterator")
                ->arg("iterator");
        // functions
        addExtern<DAS_BIND_FUN(builtin_throw)>(*this, lib, "panic",
            SideEffects::modifyExternal, "builtin_throw")
                ->args({"text","context","at"});
        addExtern<DAS_BIND_FUN(builtin_print)>(*this, lib, "print",
            SideEffects::modifyExternal, "builtin_print")
                ->args({"text","context","at"});
        addExtern<DAS_BIND_FUN(builtin_feint)>(*this, lib, "feint",
            SideEffects::modifyExternal, "builtin_feint")
                ->args({"text","context","at"});
        addExtern<DAS_BIND_FUN(builtin_error)>(*this, lib, "error",
            SideEffects::modifyExternal, "builtin_error")
                ->args({"text","context","at"});
        addInterop<builtin_sprint,char *,vec4f,PrintFlags>(*this, lib, "sprint",
            SideEffects::modifyExternal, "builtin_sprint")
                ->args({"value","flags"});
        addInterop<builtin_json_sprint,char *,vec4f,bool>(*this, lib, "sprint_json",
            SideEffects::modifyExternal, "builtin_json_sprint")
                ->args({"value","humanReadable"});
        addInterop<builtin_json_sscan,bool,char *,vec4f>(*this, lib, "sscan_json",
            SideEffects::modifyArgumentAndExternal, "builtin_json_sscan")
                ->args({"json","value"});
        addExtern<DAS_BIND_FUN(builtin_terminate)>(*this, lib, "terminate",
            SideEffects::modifyExternal, "terminate")
                ->args({"context","at"});
        addInterop<builtin_breakpoint,void>(*this, lib, "breakpoint", SideEffects::modifyExternal, "breakpoint");
        // stackwalk
        auto fnsw = addExtern<DAS_BIND_FUN(builtin_stackwalk)>(*this, lib, "stackwalk",
            SideEffects::modifyExternal, "builtin_stackwalk")
                ->args({"args","vars","context","lineinfo"});
        fnsw->arguments[0]->init = make_smart<ExprConstBool>(true);
        fnsw->arguments[1]->init = make_smart<ExprConstBool>(true);
        // profiler
        addExtern<DAS_BIND_FUN(resetProfiler)>(*this, lib, "reset_profiler",
            SideEffects::modifyExternal, "resetProfiler")
                ->arg("context");
        addExtern<DAS_BIND_FUN(dumpProfileInfo)>(*this, lib, "dump_profile_info",
            SideEffects::modifyExternal, "dumpProfileInfo")
                ->arg("context");
        addExtern<DAS_BIND_FUN(collectProfileInfo)>(*this, lib, "collect_profile_info",
            SideEffects::modifyExternal, "collectProfileInfo")
                ->args({"context","at"});
        // variant
        addExtern<DAS_BIND_FUN(variant_index)>(*this, lib, "variant_index", SideEffects::none, "variant_index");
        addExtern<DAS_BIND_FUN(set_variant_index)>(*this, lib, "set_variant_index",
            SideEffects::modifyArgument, "set_variant_index")
                ->args({"variant","index"})->unsafeOperation = true;
        // heap
        addExtern<DAS_BIND_FUN(heap_allocation_stats)>(*this, lib, "heap_allocation_stats",
            SideEffects::modifyExternal, "heap_allocation_stats")
                ->arg("context");
        addExtern<DAS_BIND_FUN(heap_allocation_count)>(*this, lib, "heap_allocation_count",
            SideEffects::modifyExternal, "heap_allocation_count")
                ->arg("context");
        addExtern<DAS_BIND_FUN(string_heap_allocation_stats)>(*this, lib, "string_heap_allocation_stats",
            SideEffects::modifyExternal, "string_heap_allocation_stats")
                ->arg("context");
        addExtern<DAS_BIND_FUN(string_heap_allocation_count)>(*this, lib, "string_heap_allocation_count",
            SideEffects::modifyExternal, "string_heap_allocation_count")
                ->arg("context");
        addExtern<DAS_BIND_FUN(heap_bytes_allocated)>(*this, lib, "heap_bytes_allocated",
            SideEffects::modifyExternal, "heap_bytes_allocated")
                ->arg("context");
        addExtern<DAS_BIND_FUN(heap_depth)>(*this, lib, "heap_depth",
            SideEffects::modifyExternal, "heap_depth")
                ->arg("context");
        addExtern<DAS_BIND_FUN(string_heap_bytes_allocated)>(*this, lib, "string_heap_bytes_allocated",
            SideEffects::modifyExternal, "string_heap_bytes_allocated")
                ->arg("context");
        addExtern<DAS_BIND_FUN(string_heap_depth)>(*this, lib, "string_heap_depth",
            SideEffects::modifyExternal, "string_heap_depth")
                ->arg("context");
        auto hcol = addExtern<DAS_BIND_FUN(heap_collect)>(*this, lib, "heap_collect",
                SideEffects::modifyExternal, "heap_collect")
                    ->args({"string_heap","validate","context","at"});
        hcol->unsafeOperation = true;
        hcol->arguments[0]->init = make_smart<ExprConstBool>(true);
        hcol->arguments[1]->init = make_smart<ExprConstBool>(false);
        addExtern<DAS_BIND_FUN(string_heap_report)>(*this, lib, "string_heap_report",
            SideEffects::modifyExternal, "string_heap_report")
                ->args({"context","line"});
        addExtern<DAS_BIND_FUN(heap_report)>(*this, lib, "heap_report",
            SideEffects::modifyExternal, "heap_report")
                ->args({"context","line"});
        addExtern<DAS_BIND_FUN(memory_report)>(*this, lib, "memory_report",
            SideEffects::modifyExternal, "memory_report")
                ->args({"errorsOnly","context","lineinfo"});
        addExtern<DAS_BIND_FUN(is_intern_strings)>(*this, lib, "is_intern_strings",
            SideEffects::modifyExternal, "is_intern_strings")
                ->arg("context");
        // binary serializer
        addInterop<_builtin_binary_load,void,vec4f,const Array &>(*this,lib,"_builtin_binary_load",
            SideEffects::modifyArgumentAndExternal, "_builtin_binary_load")
                ->args({"data","array"});
        addInterop<_builtin_binary_save,void,const vec4f,const Block &>(*this, lib, "_builtin_binary_save",
            SideEffects::modifyExternal, "_builtin_binary_save")
                ->args({"data","block"});
        // function-like expresions
        addCall<ExprAssert>         ("assert",false);
        addCall<ExprAssert>         ("verify",true);
        addCall<ExprStaticAssert>   ("static_assert");
        addCall<ExprStaticAssert>   ("concept_assert");
        addCall<ExprDebug>          ("debug");
        addCall<ExprMemZero>        ("memzero");
        // hash
        addInterop<_builtin_hash,uint64_t,vec4f>(*this, lib, "hash", SideEffects::none, "_builtin_hash")->arg("data");
        addExtern<DAS_BIND_FUN(hash64z)>(*this, lib, "hash", SideEffects::none, "hash64z")->arg("data");
        addExtern<DAS_BIND_FUN(_builtin_hash_int8)>(*this, lib, "hash", SideEffects::none, "_builtin_hash_int8")->arg("value");
        addExtern<DAS_BIND_FUN(_builtin_hash_uint8)>(*this, lib, "hash", SideEffects::none, "_builtin_hash_uint8")->arg("value");
        addExtern<DAS_BIND_FUN(_builtin_hash_int16)>(*this, lib, "hash", SideEffects::none, "_builtin_hash_int16")->arg("value");
        addExtern<DAS_BIND_FUN(_builtin_hash_uint16)>(*this, lib, "hash", SideEffects::none, "_builtin_hash_uint16")->arg("value");
        addExtern<DAS_BIND_FUN(_builtin_hash_int32)>(*this, lib, "hash", SideEffects::none, "_builtin_hash_int32")->arg("value");
        addExtern<DAS_BIND_FUN(_builtin_hash_uint32)>(*this, lib, "hash", SideEffects::none, "_builtin_hash_uint32")->arg("value");
        addExtern<DAS_BIND_FUN(_builtin_hash_int64)>(*this, lib, "hash", SideEffects::none, "_builtin_hash_int64")->arg("value");
        addExtern<DAS_BIND_FUN(_builtin_hash_uint64)>(*this, lib, "hash", SideEffects::none, "_builtin_hash_uint64")->arg("value");
        addExtern<DAS_BIND_FUN(_builtin_hash_ptr)>(*this, lib, "hash", SideEffects::none, "_builtin_hash_ptr")->arg("value");
        addExtern<DAS_BIND_FUN(_builtin_hash_float)>(*this, lib, "hash", SideEffects::none, "_builtin_hash_float")->arg("value");
        addExtern<DAS_BIND_FUN(_builtin_hash_double)>(*this, lib, "hash", SideEffects::none, "_builtin_hash_double")->arg("value");
        addExtern<DAS_BIND_FUN(_builtin_hash_das_string)>(*this, lib, "hash", SideEffects::none, "_builtin_hash_das_string")->arg("value");
        // table functions
        addExtern<DAS_BIND_FUN(builtin_table_clear)>(*this, lib, "_builtin_table_clear",
            SideEffects::modifyArgument, "builtin_table_clear")
                ->args({"table","context","at"});
        addExtern<DAS_BIND_FUN(builtin_table_size)>(*this, lib, "length",
            SideEffects::none, "builtin_table_size")
                ->arg("table");
        addExtern<DAS_BIND_FUN(builtin_table_capacity)>(*this, lib, "capacity",
            SideEffects::none, "builtin_table_capacity")
                ->arg("table");
        addExtern<DAS_BIND_FUN(builtin_table_lock)>(*this, lib, "__builtin_table_lock",
            SideEffects::modifyArgumentAndExternal, "builtin_table_lock")
                ->args({"table","context","at"});
        addExtern<DAS_BIND_FUN(builtin_table_lock_mutable)>(*this, lib, "__builtin_table_lock_mutable",
            SideEffects::modifyArgumentAndExternal, "builtin_table_lock_mutable")
                ->args({"table","context","at"});
        addExtern<DAS_BIND_FUN(builtin_table_unlock)>(*this, lib, "__builtin_table_unlock",
            SideEffects::modifyArgumentAndExternal, "builtin_table_unlock")
                ->args({"table","context","at"});
        addExtern<DAS_BIND_FUN(builtin_table_unlock_mutable)>(*this, lib, "__builtin_table_unlock_mutable",
            SideEffects::modifyArgumentAndExternal, "builtin_table_unlock_mutable")
                ->args({"table","context","at"});
        addExtern<DAS_BIND_FUN(builtin_table_clear_lock)>(*this, lib, "__builtin_table_clear_lock",
            SideEffects::modifyArgumentAndExternal, "builtin_table_clear_lock")
                ->args({"table","context"});
        addExtern<DAS_BIND_FUN(builtin_table_keys)>(*this, lib, "__builtin_table_keys",
            SideEffects::modifyArgumentAndExternal, "builtin_table_keys")
                ->args({"iterator","table","stride","context","at"});
        addExtern<DAS_BIND_FUN(builtin_table_values)>(*this, lib, "__builtin_table_values",
            SideEffects::modifyArgumentAndExternal, "builtin_table_values")
                ->args({"iterator","table","stride","context","at"});
        addExtern<DAS_BIND_FUN(builtin_table_get_key)>(*this, lib, "__builtin_table_get_key",
            SideEffects::modifyExternal, "builtin_table_get_key")
                ->args({"result","table","value_ptr","value_stride","key_stride","context","at"});
        addInterop<builtin_table_reserve,void,vec4f,uint32_t>(*this, lib, "__builtin_table_reserve",
            SideEffects::modifyArgumentAndExternal, "builtin_table_reserve")
                ->args({"table","size"});
        // array and table free
        addExtern<DAS_BIND_FUN(builtin_array_free)>(*this, lib, "__builtin_array_free",
            SideEffects::modifyArgumentAndExternal, "builtin_array_free")
                ->args({"array","stride","context","at"});
        addExtern<DAS_BIND_FUN(builtin_table_free)>(*this, lib, "__builtin_table_free",
            SideEffects::modifyArgumentAndExternal, "builtin_table_free")
                ->args({"table","sizeOfKey","sizeOfValue","context","at"});
        // local collection
        addInterop<builtin_collect_local_and_zero,void,vec4f,uint32_t>(*this, lib, "builtin_collect_local_and_zero",
            SideEffects::modifyArgumentAndExternal, "builtin_collect_local_and_zero")
                ->args({"anything","sizeOfAnything"})->unsafeOperation = true;
        // table expressions
        addCall<ExprErase>("__builtin_table_erase");
        addCall<ExprSetInsert>("__builtin_table_set_insert");
        addCall<ExprFind>("__builtin_table_find");
        addCall<ExprKeyExists>("__builtin_table_key_exists");
        // blocks
        addCall<ExprInvoke>("invoke");
        // smart ptr stuff
        addExtern<DAS_BIND_FUN(builtin_smart_ptr_move_new)>(*this, lib, "move_new",
            SideEffects::modifyArgument, "builtin_smart_ptr_move_new")
                ->args({"dest","src","context","at"});
        addExtern<DAS_BIND_FUN(builtin_smart_ptr_move_ptr)>(*this, lib, "move",
            SideEffects::modifyArgument, "builtin_smart_ptr_move_ptr")
                ->args({"dest","src","context","at"});
        addExtern<DAS_BIND_FUN(builtin_smart_ptr_move)>(*this, lib, "move",
            SideEffects::modifyArgument, "builtin_smart_ptr_move")
                ->args({"dest","src","context","at"});
        addExtern<DAS_BIND_FUN(builtin_smart_ptr_clone_ptr)>(*this, lib, "smart_ptr_clone",
            SideEffects::modifyArgument, "builtin_smart_ptr_clone_ptr")
                ->args({"dest","src","context","at"});
        addExtern<DAS_BIND_FUN(builtin_smart_ptr_clone)>(*this, lib, "smart_ptr_clone",
            SideEffects::modifyArgument, "builtin_smart_ptr_clone")
                ->args({"dest","src","context","at"});
        addExtern<DAS_BIND_FUN(builtin_smart_ptr_use_count)>(*this, lib, "smart_ptr_use_count",
            SideEffects::none, "builtin_smart_ptr_use_count")
                ->args({"ptr","context","at"});
        addExtern<DAS_BIND_FUN(smart_ptr_is_valid)>(*this, lib, "smart_ptr_is_valid",
            SideEffects::none, "smart_ptr_is_valid")
                ->arg("dest");
        addExtern<DAS_BIND_FUN(equ_sptr_sptr)>(*this, lib, "==", SideEffects::none, "equ_sptr_sptr");
        addExtern<DAS_BIND_FUN(nequ_sptr_sptr)>(*this, lib, "!=", SideEffects::none, "nequ_sptr_sptr");
        addExtern<DAS_BIND_FUN(equ_ptr_sptr)>(*this, lib, "==", SideEffects::none, "equ_ptr_sptr");
        addExtern<DAS_BIND_FUN(nequ_ptr_sptr)>(*this, lib, "!=", SideEffects::none, "nequ_ptr_sptr");
        addExtern<DAS_BIND_FUN(equ_sptr_ptr)>(*this, lib, "==", SideEffects::none, "equ_sptr_ptr");
        addExtern<DAS_BIND_FUN(nequ_sptr_ptr)>(*this, lib, "!=", SideEffects::none, "nequ_sptr_ptr");
        // gc0
        addExtern<DAS_BIND_FUN(gc0_save_ptr)>(*this, lib, "gc0_save_ptr",
            SideEffects::modifyExternal, "gc0_save_ptr")
                ->args({"name","data","context","line"});
        addExtern<DAS_BIND_FUN(gc0_save_smart_ptr)>(*this, lib, "gc0_save_smart_ptr",
            SideEffects::modifyExternal, "gc0_save_smart_ptr")
                ->args({"name","data","context","line"});
        addExtern<DAS_BIND_FUN(gc0_restore_ptr)>(*this, lib, "gc0_restore_ptr",
            SideEffects::accessExternal, "gc0_restore_ptr")
                ->args({"name","context"});
        addExtern<DAS_BIND_FUN(gc0_restore_smart_ptr)>(*this, lib, "gc0_restore_smart_ptr",
            SideEffects::accessExternal, "gc0_restore_smart_ptr")
                ->args({"name","context"});
        addExtern<DAS_BIND_FUN(gc0_reset)>(*this, lib, "gc0_reset",
            SideEffects::modifyExternal, "gc0_reset");
        // memops
        addExtern<DAS_BIND_FUN(das_memcpy)>(*this, lib, "memcpy",
            SideEffects::modifyArgumentAndExternal, "das_memcpy")
                ->args({"left","right","size"})->unsafeOperation = true;
        addExtern<DAS_BIND_FUN(das_memcmp)>(*this, lib, "memcmp",
            SideEffects::none, "das_memcmp")
                ->args({"left","right","size"})->unsafeOperation = true;
        addExtern<DAS_BIND_FUN(das_memset8)>(*this, lib, "memset8",
            SideEffects::modifyArgumentAndExternal, "das_memset8")
                ->args({"left","value","count"})->unsafeOperation = true;
        addExtern<DAS_BIND_FUN(das_memset16)>(*this, lib, "memset16",
            SideEffects::modifyArgumentAndExternal, "das_memset16")
                ->args({"left","value","count"})->unsafeOperation = true;
        addExtern<DAS_BIND_FUN(das_memset32)>(*this, lib, "memset32",
            SideEffects::modifyArgumentAndExternal, "das_memset32")
                ->args({"left","value","count"})->unsafeOperation = true;
        addExtern<DAS_BIND_FUN(das_memset64)>(*this, lib, "memset64",
            SideEffects::modifyArgumentAndExternal, "das_memset64")
                ->args({"left","value","count"})->unsafeOperation = true;
        addExternEx<void(*)(void *,uint4,int32_t),DAS_BIND_FUN(das_memset128u)>(*this, lib, "memset128",
            SideEffects::modifyArgumentAndExternal, "das_memset128u")
                ->args({"left","value","count"})->unsafeOperation = true;
        addExtern<DAS_BIND_FUN(builtin_das_aligned_alloc16)> (*this, lib, "malloc",
            SideEffects::modifyExternal, "builtin_das_aligned_alloc16")
                ->args({"size"})->unsafeOperation = true;
        addExtern<DAS_BIND_FUN(das_aligned_free16)> (*this, lib, "free",
            SideEffects::modifyExternal, "das_aligned_free16")
                ->args({"ptr"})->unsafeOperation = true;
        addExtern<DAS_BIND_FUN(das_aligned_memsize)> (*this, lib, "malloc_usable_size",
            SideEffects::none, "das_aligned_memsize")
                ->args({"ptr"})->unsafeOperation = true;
        // pointer ari
        PointerOp(addExtern<DAS_BIND_FUN(i_das_ptr_inc)>(*this, lib, "i_das_ptr_inc", SideEffects::modifyArgument, "das_ptr_inc"));
        PointerOp(addExtern<DAS_BIND_FUN(i_das_ptr_dec)>(*this, lib, "i_das_ptr_dec", SideEffects::modifyArgument, "das_ptr_dec"));
        // int32
        PointerOp(addExtern<DAS_BIND_FUN(i_das_ptr_add_int32)>(*this, lib, "i_das_ptr_add", SideEffects::none, "das_ptr_add_int32"));
        PointerOp(addExtern<DAS_BIND_FUN(i_das_ptr_sub_int32)>(*this, lib, "i_das_ptr_sub", SideEffects::none, "das_ptr_sub_int32"));
        PointerOp(addExtern<DAS_BIND_FUN(i_das_ptr_set_add_int32)>(*this, lib, "i_das_ptr_set_add", SideEffects::modifyArgument, "das_ptr_set_add_int32"));
        PointerOp(addExtern<DAS_BIND_FUN(i_das_ptr_set_sub_int32)>(*this, lib, "i_das_ptr_set_sub", SideEffects::modifyArgument, "das_ptr_set_sub_int32"));
        // int64
        PointerOp(addExtern<DAS_BIND_FUN(i_das_ptr_add_int64)>(*this, lib, "i_das_ptr_add", SideEffects::none, "das_ptr_add_int64"));
        PointerOp(addExtern<DAS_BIND_FUN(i_das_ptr_sub_int64)>(*this, lib, "i_das_ptr_sub", SideEffects::none, "das_ptr_sub_int64"));
        PointerOp(addExtern<DAS_BIND_FUN(i_das_ptr_set_add_int64)>(*this, lib, "i_das_ptr_set_add", SideEffects::modifyArgument, "das_ptr_set_add_int64"));
        PointerOp(addExtern<DAS_BIND_FUN(i_das_ptr_set_sub_int64)>(*this, lib, "i_das_ptr_set_sub", SideEffects::modifyArgument, "das_ptr_set_sub_int64"));
        // uint32
        PointerOp(addExtern<DAS_BIND_FUN(i_das_ptr_add_uint32)>(*this, lib, "i_das_ptr_add", SideEffects::none, "das_ptr_add_uint32"));
        PointerOp(addExtern<DAS_BIND_FUN(i_das_ptr_sub_uint32)>(*this, lib, "i_das_ptr_sub", SideEffects::none, "das_ptr_sub_uint32"));
        PointerOp(addExtern<DAS_BIND_FUN(i_das_ptr_set_add_uint32)>(*this, lib, "i_das_ptr_set_add", SideEffects::modifyArgument, "das_ptr_set_add_uint32"));
        PointerOp(addExtern<DAS_BIND_FUN(i_das_ptr_set_sub_uint32)>(*this, lib, "i_das_ptr_set_sub", SideEffects::modifyArgument, "das_ptr_set_sub_uint32"));
        // uint64
        PointerOp(addExtern<DAS_BIND_FUN(i_das_ptr_add_uint64)>(*this, lib, "i_das_ptr_add", SideEffects::none, "das_ptr_add_uint64"));
        PointerOp(addExtern<DAS_BIND_FUN(i_das_ptr_sub_uint64)>(*this, lib, "i_das_ptr_sub", SideEffects::none, "das_ptr_sub_uint64"));
        PointerOp(addExtern<DAS_BIND_FUN(i_das_ptr_set_add_uint64)>(*this, lib, "i_das_ptr_set_add", SideEffects::modifyArgument, "das_ptr_set_add_uint64"));
        PointerOp(addExtern<DAS_BIND_FUN(i_das_ptr_set_sub_uint64)>(*this, lib, "i_das_ptr_set_sub", SideEffects::modifyArgument, "das_ptr_set_sub_uint64"));
        // diff
        addExtern<DAS_BIND_FUN(i_das_ptr_diff)>(*this, lib, "i_das_ptr_diff", SideEffects::none, "i_das_ptr_diff");
        // rtti
        addExtern<DAS_BIND_FUN(class_rtti_size)>(*this, lib, "class_rtti_size",
            SideEffects::none, "class_rtti_size")
                ->arg("ptr");
        // profile
        addExtern<DAS_BIND_FUN(builtin_profile)>(*this,lib,"profile",
            SideEffects::modifyExternal, "builtin_profile")
                ->args({"count","category","block","context","line"});
        // das string binding
        addExtern<DAS_BIND_FUN(to_das_string)>(*this, lib, "string",
            SideEffects::none, "to_das_string")
                ->args({"source","context","at"});
        addExtern<DAS_BIND_FUN(pass_string)>(*this, lib, "string",
            SideEffects::none, "pass_string", permanentArgFn())
                ->args({"source"})->setCaptureString();
        addExtern<DAS_BIND_FUN(clone_pass_string)>(*this, lib, "string",
            SideEffects::none, "clone_pass_string", temporaryArgFn())
                ->args({"source","context","at"});
        addExtern<DAS_BIND_FUN(set_das_string)>(*this, lib, "clone",
            SideEffects::modifyArgument,"set_das_string")
                ->args({"target","src"});
        addExtern<DAS_BIND_FUN(set_string_das)>(*this, lib, "clone",
            SideEffects::modifyArgument,"set_string_das")
                ->args({"target","src","context","at"});
        addExtern<DAS_BIND_FUN(peek_das_string)>(*this, lib, "peek",
            SideEffects::modifyExternal,"peek_das_string_T")
                ->args({"src","block","context","line"})->setAotTemplate();
        addExtern<DAS_BIND_FUN(builtin_string_clone)>(*this, lib, "clone_string",
            SideEffects::none, "builtin_string_clone")
                ->args({"src","context","at"});
        // das-string
        addExtern<DAS_BIND_FUN(das_str_equ)>(*this, lib, "==", SideEffects::none, "das_str_equ");
        addExtern<DAS_BIND_FUN(das_str_nequ)>(*this, lib, "!=", SideEffects::none, "das_str_nequ");
        // string emptiness
        addExtern<DAS_BIND_FUN(builtin_empty)>(*this, lib, "empty",
            SideEffects::none, "builtin_empty")->arg("str");
        addExtern<DAS_BIND_FUN(builtin_empty_das_string)>(*this, lib, "empty",
            SideEffects::none, "builtin_empty_das_string")->arg("str");
        // das-string extra
        STR_DSTR_REG(  eq,==);
        STR_DSTR_REG( neq,!=);
        STR_DSTR_REG(gteq,>=);
        STR_DSTR_REG(lseq,<=);
        STR_DSTR_REG(  ls,<);
        STR_DSTR_REG(  gt,>);
        // temp array out of mem
        auto bta = addExtern<DAS_BIND_FUN(builtin_temp_array)>(*this, lib, "_builtin_temp_array",
            SideEffects::invoke, "builtin_temp_array")
                ->args({"data","size","block","context","line"});
        bta->unsafeOperation = true;
        bta->privateFunction = true;
        auto bmta = addExtern<DAS_BIND_FUN(builtin_make_temp_array)>(*this, lib, "_builtin_make_temp_array",
            SideEffects::modifyArgument, "builtin_make_temp_array")
                ->args({"array","data","size"});
        bmta->unsafeOperation = true;
        // migrate data
        addExtern<DAS_BIND_FUN(das_is_dll_build)>(*this, lib, "das_is_dll_build",
            SideEffects::worstDefault, "das_is_dll_build");
        addExtern<DAS_BIND_FUN(is_in_aot)>(*this, lib, "is_in_aot",
            SideEffects::worstDefault, "is_in_aot");
        addExtern<DAS_BIND_FUN(set_aot)>(*this, lib, "set_aot",
            SideEffects::worstDefault, "set_aot");
        addExtern<DAS_BIND_FUN(reset_aot)>(*this, lib, "reset_aot",
            SideEffects::worstDefault, "reset_aot");
        // completion
        addExtern<DAS_BIND_FUN(is_in_completion)>(*this, lib, "is_in_completion",
            SideEffects::worstDefault, "is_in_completion");
        // folding
        addExtern<DAS_BIND_FUN(is_folding)>(*this, lib, "is_folding",
            SideEffects::worstDefault, "is_folding");
        // compiling file
        addExtern<DAS_BIND_FUN(compiling_file_name)>(*this, lib, "compiling_file_name",
            SideEffects::accessExternal, "compiling_file_name");
        addExtern<DAS_BIND_FUN(compiling_module_name)>(*this, lib, "compiling_module_name",
            SideEffects::accessExternal, "compiling_module_name");
        addExtern<DAS_BIND_FUN(get_module_file_name)>(*this, lib, "get_module_file_name",
            SideEffects::accessExternal, "get_module_file_name")->args({"name", "context"});
        // logger
        addExtern<DAS_BIND_FUN(toLog)>(*this, lib, "to_log",
            SideEffects::modifyExternal, "toLog")->args({"level", "text", "context", "at"});
        addExtern<DAS_BIND_FUN(toCompilerLog)>(*this, lib, "to_compiler_log",
            SideEffects::modifyExternal, "toCompilerLog")->args({"text","context","at"});
        // log levels
        addConstant<int>(*this, "LOG_CRITICAL", LogLevel::critical);
        addConstant<int>(*this, "LOG_ERROR",    LogLevel::error);
        addConstant<int>(*this, "LOG_WARNING",  LogLevel::warning);
        addConstant<int>(*this, "LOG_INFO",     LogLevel::info);
        addConstant<int>(*this, "LOG_DEBUG",    LogLevel::debug);
        addConstant<int>(*this, "LOG_TRACE",    LogLevel::trace);
        // separators
        addConstant(*this, "VEC_SEP",   DAS_PRINT_VEC_SEPARATROR);
        // clz, ctz, popcnt. mul
        addExtern<DAS_BIND_FUN(uint32_clz)>(*this, lib, "clz", SideEffects::none, "uint32_clz")->arg("bits");
        addExtern<DAS_BIND_FUN(uint64_clz)>(*this, lib, "clz", SideEffects::none, "uint64_clz")->arg("bits");
        addExtern<DAS_BIND_FUN(uint32_ctz)>(*this, lib, "ctz", SideEffects::none, "uint32_ctz")->arg("bits");
        addExtern<DAS_BIND_FUN(uint64_ctz)>(*this, lib, "ctz", SideEffects::none, "uint64_ctz")->arg("bits");
        addExtern<DAS_BIND_FUN(uint32_popcount)>(*this, lib, "popcnt", SideEffects::none, "uint32_popcount")->arg("bits");
        addExtern<DAS_BIND_FUN(uint64_popcount)>(*this, lib, "popcnt", SideEffects::none, "uint64_popcount")->arg("bits");
        addExtern<DAS_BIND_FUN(mul_u64_u64)>(*this, lib, "mul128", SideEffects::none, "mul_u64_u64")->args({"a","b"});
        // string using
        addUsing<das::string>(*this, lib, "das::string");
        // try-recover
        addExtern<DAS_BIND_FUN(builtin_try_recover)>(*this, lib, "builtin_try_recover",
            SideEffects::invoke, "builtin_try_recover")
                ->args({"try_block","catch_block","context","at"});
        // main-loop
        addExtern<DAS_BIND_FUN(builtin_main_loop)>(*this, lib, "eval_main_loop",
            SideEffects::invoke, "builtin_main_loop")
                ->args({"block","context","at"});
        // jit
        addExtern<DAS_BIND_FUN(das_is_jit_function)>(*this, lib, "is_jit_function",
                SideEffects::worstDefault, "das_is_jit_function")
                    ->args({"function"});
        addExtern<DAS_BIND_FUN(das_jit_enabled)>(*this, lib, "jit_enabled",
            SideEffects::none, "das_jit_enabled")
                ->args({"context","at"});
        // aot
        addExtern<DAS_BIND_FUN(das_aot_enabled)>(*this, lib, "aot_enabled",
            SideEffects::none, "das_aot_enabled")
                ->args({"context","at"});
        // bitfield
        addExtern<DAS_BIND_FUN(__bit_set)>(*this, lib, "__bit_set",
            SideEffects::modifyArgument, "__bit_set")
                ->args({"value","mask","on"});
        addExtern<DAS_BIND_FUN(__bit_set8)>(*this, lib, "__bit_set",
            SideEffects::modifyArgument, "__bit_set8")
                ->args({"value","mask","on"});
        addExtern<DAS_BIND_FUN(__bit_set16)>(*this, lib, "__bit_set",
            SideEffects::modifyArgument, "__bit_set16")
                ->args({"value","mask","on"});
        addExtern<DAS_BIND_FUN(__bit_set64)>(*this, lib, "__bit_set",
            SideEffects::modifyArgument, "__bit_set64")
                ->args({"value","mask","on"});
        // platform and architecture
        addExtern<DAS_BIND_FUN(das_get_platform_name)>(*this, lib, "get_platform_name",
            SideEffects::none, "das_get_platform_name");
        addExtern<DAS_BIND_FUN(das_get_architecture_name)>(*this, lib, "get_architecture_name",
            SideEffects::none, "das_get_architecture_name");
        // fmt
        addExtern<DAS_BIND_FUN(fmt_i8)>(*this, lib, "fmt",
            SideEffects::none, "fmt_i8")->args({"format","value","context","at"});
        addExtern<DAS_BIND_FUN(fmt_u8)>(*this, lib, "fmt",
            SideEffects::none, "fmt_u8")->args({"format","value","context","at"});
        addExtern<DAS_BIND_FUN(fmt_i16)>(*this, lib, "fmt",
            SideEffects::none, "fmt_i16")->args({"format","value","context","at"});
        addExtern<DAS_BIND_FUN(fmt_u16)>(*this, lib, "fmt",
            SideEffects::none, "fmt_u16")->args({"format","value","context","at"});
        addExtern<DAS_BIND_FUN(fmt_i32)>(*this, lib, "fmt",
            SideEffects::none, "fmt_i32")->args({"format","value","context","at"});
        addExtern<DAS_BIND_FUN(fmt_u32)>(*this, lib, "fmt",
            SideEffects::none, "fmt_u32")->args({"format","value","context","at"});
        addExtern<DAS_BIND_FUN(fmt_i64)>(*this, lib, "fmt",
            SideEffects::none, "fmt_i64")->args({"format","value","context","at"});
        addExtern<DAS_BIND_FUN(fmt_u64)>(*this, lib, "fmt",
            SideEffects::none, "fmt_u64")->args({"format","value","context","at"});
        addExtern<DAS_BIND_FUN(fmt_f)>(*this, lib, "fmt",
            SideEffects::none, "fmt_f")->args({"format","value","context","at"});
        addExtern<DAS_BIND_FUN(fmt_d)>(*this, lib, "fmt",
            SideEffects::none, "fmt_d")->args({"format","value","context","at"});
    }
}
