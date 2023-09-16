#include "daScript/misc/platform.h"

#include "module_builtin.h"

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
            auto program = daScriptEnvironment::bound->g_Program;
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

    struct DeprecatedFunctionAnnotation : MarkFunctionAnnotation {
        DeprecatedFunctionAnnotation() : MarkFunctionAnnotation("deprecated") { }
        virtual bool apply(const FunctionPtr & func, ModuleGroup &, const AnnotationArgumentList &, string &) override {
            func->deprecated = true;
            return true;
        };
        virtual bool verifyCall ( ExprCallFunc * call, const AnnotationArgumentList & args,
            const AnnotationArgumentList & /*progArgs */, string & /*err*/ ) override {
            DAS_ASSERT(daScriptEnvironment::bound->g_compilerLog);
            (*daScriptEnvironment::bound->g_compilerLog) << call->at.describe() << ": *warning* function " << call->func->name << " is deprecated\n";
            if ( auto arg = args.find("message",Type::tString) ) {
                (*daScriptEnvironment::bound->g_compilerLog) << "\t" << arg->sValue << "\n";
            }
            return true;
        }
    };

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

    struct UnsafeDerefFunctionAnnotation : MarkFunctionAnnotation {
        UnsafeDerefFunctionAnnotation() : MarkFunctionAnnotation("unsafe_deref") { }
        virtual bool apply(const FunctionPtr & func, ModuleGroup &, const AnnotationArgumentList &, string &) override {
            func->unsafeDeref = true;
            return true;
        };
    };

    struct SkipLockCheckFunctionAnnotation : MarkFunctionAnnotation {
        SkipLockCheckFunctionAnnotation() : MarkFunctionAnnotation("skip_lock_check") { }
        virtual bool apply(const FunctionPtr & func, ModuleGroup &, const AnnotationArgumentList &, string &) override {
            func->skipLockCheck = true;
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
        virtual bool apply(const FunctionPtr & func, ModuleGroup &, const AnnotationArgumentList &, string &) override {
            func->shutdown = true;
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

    struct IsYetAnotherVectorTemplateAnnotation : MarkFunctionAnnotation {
        IsYetAnotherVectorTemplateAnnotation() : MarkFunctionAnnotation("expect_any_vector") {}
        virtual bool isSpecialized() const override { return true; }
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


    struct SkipLockCheckStructureAnnotation : StructureAnnotation {
        SkipLockCheckStructureAnnotation() : StructureAnnotation("skip_field_lock_check") {}
        virtual bool touch(const StructurePtr & ps, ModuleGroup &,
                           const AnnotationArgumentList &, string & ) override {
            ps->skipLockCheck = true;
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
                        errors += "\t" + field.name + " : " + field.type->describe() + " is not a pod\n";
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
        AnnotationDeclarationPtr subexpr;
    };

    struct LogicOp2Annotation : LogicOpAnnotation {
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
        AnnotationDeclarationPtr left, right;
    };

    struct LogicOpAndAnnotation : LogicOp2Annotation {
        LogicOpAndAnnotation ( const AnnotationDeclarationPtr & arg0, const AnnotationDeclarationPtr & arg1 )
            : LogicOp2Annotation("&&",arg0,arg1) { }
        virtual bool isCompatible ( const FunctionPtr & fun, const vector<TypeDeclPtr> & types, const AnnotationDeclaration &, string & errors  ) const override  {
            if ( !left || !right ) return false;
            return ((FunctionAnnotation *)(left->annotation.get()))->isCompatible(fun, types, *left, errors) &&
                ((FunctionAnnotation *)(right->annotation.get()))->isCompatible(fun, types, *right, errors);
        }
    };

    struct LogicOpOrAnnotation : LogicOp2Annotation {
        LogicOpOrAnnotation ( const AnnotationDeclarationPtr & arg0, const AnnotationDeclarationPtr & arg1 )
            : LogicOp2Annotation("||",arg0,arg1) { }
        virtual bool isCompatible ( const FunctionPtr & fun, const vector<TypeDeclPtr> & types, const AnnotationDeclaration &, string & errors  ) const override  {
            if ( !left || !right ) return false;
            return ((FunctionAnnotation *)(left->annotation.get()))->isCompatible(fun, types, *left, errors) ||
                ((FunctionAnnotation *)(right->annotation.get()))->isCompatible(fun, types, *right, errors);
        }
    };

    struct LogicOpXOrAnnotation : LogicOp2Annotation {
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

#if defined(__clang__)
#pragma clang diagnostic pop
#endif

    template <typename intT>
    struct EnumIterator : Iterator {
        EnumIterator ( EnumInfo * ei ) : info(ei) {}
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
            context.heap->free((char *)this, sizeof(EnumIterator<intT>));
        }
        EnumInfo *  info = nullptr;
        int32_t     count = 0;
        int32_t     range_to = 0;
    };

    struct CountIterator : Iterator {
        CountIterator ( int32_t _start, int32_t _step ) : start(_start), step(_step) {};
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
            context.heap->free((char *)this, sizeof(CountIterator));
        }
        int32_t start = 0;
        int32_t step = 0;
    };

    TSequence<int32_t> builtin_count ( int32_t start, int32_t step, Context * context ) {
        char * iter = context->heap->allocate(sizeof(CountIterator));
        context->heap->mark_comment(iter, "count iterator");
        new (iter) CountIterator(start, step);
        return TSequence<int>((Iterator *)iter);
    }

    TSequence<uint32_t> builtin_ucount ( uint32_t start, uint32_t step, Context * context ) {
        char * iter = context->heap->allocate(sizeof(CountIterator));
        context->heap->mark_comment(iter, "ucount iterator");
        new (iter) CountIterator(start, step);
        return TSequence<int>((Iterator *)iter);
    }

    // core functions

    void builtin_throw ( char * text, Context * context, LineInfoArg * at ) {
        context->throw_error_at(at, "%s", text);
    }

    void builtin_print ( char * text, Context * context, LineInfoArg * at ) {
        context->to_out(at, text);
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

    void string_heap_collect ( bool validate, Context * context, LineInfoArg * info ) {
        context->collectStringHeap(info,validate);
    }

    void string_heap_report ( Context * context, LineInfoArg * info ) {
        context->stringHeap->report();
        context->reportAnyHeap(info, true, false, false, false);
    }

    void heap_collect ( bool sheap, bool validate, Context * context, LineInfoArg * info ) {
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

    void builtin_table_lock ( const Table & arr, Context * context, LineInfoArg * at ) {
        table_lock(*context, const_cast<Table&>(arr), at);
    }

    void builtin_table_unlock ( const Table & arr, Context * context, LineInfoArg * at ) {
        table_unlock(*context, const_cast<Table&>(arr), at);
    }

    void builtin_table_clear_lock ( const Table & arr, Context * ) {
        const_cast<Table&>(arr).hopeless = 0;
    }

    bool builtin_iterator_first ( const Sequence & it, void * data, Context * context, LineInfoArg * at ) {
        if ( !it.iter ) context->throw_error_at(at, "calling first on empty iterator");
        else if ( it.iter->isOpen ) context->throw_error_at(at, "calling first on already open iterator");
        it.iter->isOpen = true;
        return it.iter->first(*context, (char *)data);
    }

    bool builtin_iterator_next ( const Sequence & it, void * data, Context * context, LineInfoArg * at ) {
        if ( !it.iter ) context->throw_error_at(at, "calling next on empty iterator");
        else if ( !it.iter->isOpen ) context->throw_error_at(at, "calling next on a non-open iterator");
        return it.iter->next(*context, (char *)data);
    }

    void builtin_iterator_close ( const Sequence & it, void * data, Context * context ) {
        if ( it.iter ) {
            it.iter->close(*context, (char *)&data);
            ((Sequence&)it).iter = nullptr;
        }
    }

    void builtin_iterator_delete ( const Sequence & it, Context * context ) {
        if ( it.iter ) {
            it.iter->close(*context, nullptr);
            ((Sequence&)it).iter = nullptr;
        }
    }

    bool builtin_iterator_iterate ( const Sequence & it, void * value, Context * context ) {
        if ( !it.iter ) {
            return false;
        } else if ( !it.iter->isOpen) {
            if ( !it.iter->first(*context, (char *)value) ) {
                it.iter->close(*context, (char *)value);
                ((Sequence&)it).iter = nullptr;
                return false;
            } else {
                it.iter->isOpen = true;
                return true;
            }
        } else {
            if ( !it.iter->next(*context, (char *)value) ) {
                it.iter->close(*context, (char *)value);
                ((Sequence&)it).iter = nullptr;
                return false;
            } else {
                return true;
            }
        }
    }

    void builtin_make_good_array_iterator ( Sequence & result, const Array & arr, int stride, Context * context ) {
        char * iter = context->heap->allocate(sizeof(GoodArrayIterator));
        context->heap->mark_comment(iter, "array<> iterator");
        new (iter) GoodArrayIterator((Array *)&arr, stride);
        result = { (Iterator *) iter };
    }

    void builtin_make_fixed_array_iterator ( Sequence & result, void * data, int size, int stride, Context * context ) {
        char * iter = context->heap->allocate(sizeof(FixedArrayIterator));
        context->heap->mark_comment(iter, "fixed array iterator");
        new (iter) FixedArrayIterator((char *)data, size, stride);
        result = { (Iterator *) iter };
    }

    void builtin_make_range_iterator ( Sequence & result, range rng, Context * context ) {
        char * iter = context->heap->allocate(sizeof(RangeIterator<range>));
        context->heap->mark_comment(iter, "range iterator");
        new (iter) RangeIterator<range>(rng);
        result = { (Iterator *) iter };
    }

    vec4f builtin_make_enum_iterator ( Context & context, SimNode_CallBase * call, vec4f * args ) {
        if ( !call->types ) context.throw_error_at(call->debugInfo, "missing type info");
        auto itinfo = call->types[0];
        if ( itinfo->type != Type::tIterator ) context.throw_error_at(call->debugInfo, "not an iterator");
        auto tinfo = itinfo->firstType;
        if ( !tinfo ) context.throw_error_at(call->debugInfo, "missing iterator type info");
        if ( tinfo->type!=Type::tEnumeration && tinfo->type!=Type::tEnumeration8
            && tinfo->type!=Type::tEnumeration16 ) {
            context.throw_error_at(call->debugInfo, "not an iterator of enumeration");
        }
        auto einfo = tinfo->enumType;
        if ( !einfo ) context.throw_error_at(call->debugInfo, "missing enumeraiton type info");
        char * iter = nullptr;
        switch ( tinfo->type ) {
        case Type::tEnumeration:
            iter = context.heap->allocate(sizeof(EnumIterator<int32_t>));
            context.heap->mark_comment(iter, "enum iterator");
            new (iter) EnumIterator<int32_t>(einfo);
            break;
        case Type::tEnumeration8:
            iter = context.heap->allocate(sizeof(EnumIterator<int8_t>));
            context.heap->mark_comment(iter, "enum8 iterator");
            new (iter) EnumIterator<int8_t>(einfo);
            break;
        case Type::tEnumeration16:
            iter = context.heap->allocate(sizeof(EnumIterator<int16_t>));
            context.heap->mark_comment(iter, "enum16 iterator");
            new (iter) EnumIterator<int16_t>(einfo);
            break;
        default:
            DAS_ASSERT(0 && "how???");
        }
        Sequence * seq = cast<Sequence *>::to(args[0]);
        seq->iter = (Iterator *) iter;
        return v_zero();
    }

    void builtin_make_string_iterator ( Sequence & result, char * str, Context * context ) {
        char * iter = context->heap->allocate(sizeof(StringIterator));
        context->heap->mark_comment(iter, "string iterator");
        new (iter) StringIterator(str);
        result = { (Iterator *) iter };
    }

    struct NilIterator : Iterator {
        virtual bool first ( Context &, char * ) override { return false; }
        virtual bool next  ( Context &, char * ) override { return false; }
        virtual void close ( Context & context, char * ) override {
            context.heap->free((char *)this, sizeof(NilIterator));
        }
    };

    void builtin_make_nil_iterator ( Sequence & result, Context * context ) {
        char * iter = context->heap->allocate(sizeof(NilIterator));
        context->heap->mark_comment(iter, "nil iterator");
        new (iter) NilIterator();
        result = { (Iterator *) iter };
    }

    struct LambdaIterator : Iterator {
        using lambdaFunc = bool (*) (Context *,void*, char*);
        LambdaIterator ( Context & context, const Lambda & ll, int st ) : lambda(ll), stride(st) {
            SimFunction ** fnMnh = (SimFunction **) lambda.capture;
            if (!fnMnh) context.throw_error("invoke null lambda");
            simFunc = *fnMnh;
            if (!simFunc) context.throw_error("invoke null function");
            aotFunc = (lambdaFunc) simFunc->aotFunction;
        }
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
            context.heap->free((char *)this, sizeof(LambdaIterator));
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

    void builtin_make_lambda_iterator ( Sequence & result, const Lambda lambda, int stride, Context * context ) {
        char * iter = context->heap->allocate(sizeof(LambdaIterator));
        context->heap->mark_comment(iter, "lambda iterator");
        new (iter) LambdaIterator(*context, lambda, stride);
        result = { (Iterator *) iter };
    }

    void resetProfiler( Context * context ) {
        context->resetProfiler();
    }

    void dumpProfileInfo( Context * context ) {
        LOG tp(LogLevel::debug);
        context->collectProfileInfo(tp);
    }

    char * collectProfileInfo( Context * context ) {
        TextWriter tout;
        context->collectProfileInfo(tout);
        return context->stringHeap->allocateString(tout.str());
    }

    void builtin_array_free ( Array & dim, int szt, Context * __context__, LineInfoArg * at ) {
        if ( dim.data ) {
            if ( !dim.lock || dim.hopeless ) {
                uint32_t oldSize = dim.capacity*szt;
                __context__->heap->free(dim.data, oldSize);
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
            if ( !tab.lock || tab.hopeless ) {
                uint32_t oldSize = tab.capacity*(szk+szv+sizeof(uint64_t));
                __context__->heap->free(tab.data, oldSize);
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
        if ( daScriptEnvironment::bound && daScriptEnvironment::bound->g_Program ) {
            return daScriptEnvironment::bound->g_Program->isCompiling || daScriptEnvironment::bound->g_Program->isSimulating;
        }
        return false;

    }

    bool is_compiling_macros ( ) {
        if ( daScriptEnvironment::bound && daScriptEnvironment::bound->g_Program ) {
            return daScriptEnvironment::bound->g_Program->isCompilingMacros;
        }
        return false;
    }

    bool is_compiling_macros_in_module ( char * name ) {
        if ( daScriptEnvironment::bound && daScriptEnvironment::bound->g_Program ) {
            if ( !daScriptEnvironment::bound->g_Program->isCompilingMacros ) return false;
            if ( daScriptEnvironment::bound->g_Program->thisModule->name != to_rts(name) ) return false;
            if ( isInDebugAgentCreation() ) return false;
            return true;
        }
        return true;
    }

    bool is_reporting_compilation_errors ( ) {
        if ( daScriptEnvironment::bound && daScriptEnvironment::bound->g_Program ) {
            return daScriptEnvironment::bound->g_Program->reportingInferErrors;
        }
        return false;
    }

    // static storage

    DAS_THREAD_LOCAL das_hash_map<uint64_t, void*> g_static_storage;

    void gc0_save_ptr ( char * name, void * data, Context * context, LineInfoArg * line ) {
        uint64_t hash = hash_function ( *context, name );
        if ( g_static_storage.find(hash)!=g_static_storage.end() ) {
            context->throw_error_at(line, "gc0 already there %s (or hash collision)", name);
        }
        g_static_storage[hash] = data;
    }

    void gc0_save_smart_ptr ( char * name, smart_ptr_raw<void> data, Context * context, LineInfoArg * line ) {
        gc0_save_ptr(name, data.get(), context, line);
    }

    void * gc0_restore_ptr ( char * name, Context * context ) {
        uint64_t hash = hash_function ( *context, name );
        auto it = g_static_storage.find(hash);
        if ( it!=g_static_storage.end() ) {
            void * res = it->second;
            g_static_storage.erase(it);
            return res;
        } else {
            return nullptr;
        }
    }

    smart_ptr_raw<void> gc0_restore_smart_ptr ( char * name, Context * context ) {
        return gc0_restore_ptr(name,context);
    }

    void gc0_reset() {
        decltype(g_static_storage) dummy;
        swap ( g_static_storage, dummy );
    }

    __forceinline void i_das_ptr_inc ( void * & ptr, int stride ) {
        ptr = (char*) ptr + stride;
    }

    __forceinline void i_das_ptr_dec ( void * & ptr, int stride ) {
        ptr = (char*) ptr - stride;
    }

    __forceinline void * i_das_ptr_add ( void * ptr, int value, int stride ) {
        return (char*) ptr + value * stride;
    }

    __forceinline void * i_das_ptr_sub ( void * & ptr, int value, int stride ) {
        return (char*) ptr - value * stride;
    }

    __forceinline void i_das_ptr_set_add ( void * & ptr, int value, int stride ) {
        ptr = (char*) ptr + value * stride;
    }

    __forceinline void i_das_ptr_set_sub ( void * & ptr, int value, int stride ) {
        ptr = (char*) ptr + value * stride;
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
        ft->argNames = { "escapeString", "namesAndDimensions", "typeQualifiers", "refAddresses", "humanReadable", "singleLine" };
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
        auto sres = context.stringHeap->allocateString(ssw.str());
        return cast<char *>::from(sres);
    }

    vec4f builtin_json_sprint ( Context & context, SimNode_CallBase * call, vec4f * args ) {
        auto typeInfo = call->types[0];
        auto res = args[0];
        auto humanReadable = cast<bool>::to(args[1]);
        auto ssw = debug_json_value(res, typeInfo, humanReadable);
        auto sres = context.stringHeap->allocateString(ssw);
        return cast<char *>::from(sres);
    }

    Array  g_CommandLineArguments;

    void setCommandLineArguments ( int argc, char * argv[] ) {
        g_CommandLineArguments.data = (char *) argv;
        g_CommandLineArguments.capacity = argc;
        g_CommandLineArguments.size = argc;
        g_CommandLineArguments.lock = 1;
        g_CommandLineArguments.flags = 0;
    }

    void getCommandLineArguments( Array & arr ) {
        arr = g_CommandLineArguments;
    }

    char * builtin_das_root ( Context * context ) {
        return context->stringHeap->allocateString(getDasRoot());
    }

    char * to_das_string(const string & str, Context * ctx) {
        return ctx->stringHeap->allocateString(str);
    }

    char * pass_string(char * str) {
        return str;
    }

    void set_das_string(string & str, const char * bs) {
        str = bs ? bs : "";
    }

    void set_string_das(char * & bs, const string & str, Context * ctx ) {
        bs = ctx->stringHeap->allocateString(str);
    }

    void peek_das_string(const string & str, const TBlock<void,TTemporary<const char *>> & block, Context * context, LineInfoArg * at) {
        vec4f args[1];
        args[0] = cast<const char *>::from(str.c_str());
        context->invoke(block, args, nullptr, at);
    }

    char * builtin_string_clone ( const char *str, Context * context ) {
        const uint32_t strLen = stringLengthSafe ( *context, str );
        if (!strLen)
            return nullptr;
        return context->stringHeap->allocateString(str, strLen);
    }

    void builtin_temp_array ( void * data, int size, const Block & block, Context * context, LineInfoArg * at ) {
        Array arr;
        arr.data = (char *) data;
        arr.size = arr.capacity = size;
        arr.lock = 1;
        arr.flags = 0;
        vec4f args[1];
        args[0] = cast<Array &>::from(arr);
        context->invoke(block, args, nullptr, at);
    }

    void builtin_make_temp_array ( Array & arr, void * data, int size ) {
        arr.data = (char *) data;
        arr.size = arr.capacity = size;
        arr.lock = 0;
        arr.flags = 0;
    }

    void toLog ( int level, const char * text, Context * context, LineInfoArg * at ) {
        logger(level, getLogMarker(level), text, context, at);
    }

    void toCompilerLog ( const char * text, Context * context, LineInfoArg * at ) {
        if ( daScriptEnvironment::bound->g_compilerLog ) {
            if ( text ) {
                (*daScriptEnvironment::bound->g_compilerLog) << text;
            }
        } else {
            context->throw_error_at(at, "can only write to compiler log during compilation");
        }
    }

    bool is_in_aot ( ) {
        return daScriptEnvironment::bound ? daScriptEnvironment::bound->g_isInAot : false;
    }

    bool is_in_completion ( ) {
        if ( daScriptEnvironment::bound && daScriptEnvironment::bound->g_Program ) {
            return daScriptEnvironment::bound->g_Program->policies.completion;
        }
        return false;
    }

    bool is_folding ( ) {
        if ( daScriptEnvironment::bound && daScriptEnvironment::bound->g_Program ) {
            return daScriptEnvironment::bound->g_Program->folding;
        }
        return false;
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
#endif TRY_MAIN_LOOP
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

    bool das_jit_enabled ( Context * context, LineInfoArg * at ) {
        if ( !context->thisProgram ) context->throw_error_at(at, "can only query for jit during compilation");
        return context->thisProgram->policies.jit;
    }

#define STR_DSTR_REG(OPNAME,EXPR) \
    addExtern<DAS_BIND_FUN(OPNAME##_str_dstr)>(*this, lib, #EXPR, SideEffects::none, DAS_TOSTRING(OPNAME##_str_dstr)); \
    addExtern<DAS_BIND_FUN(OPNAME##_dstr_str)>(*this, lib, #EXPR, SideEffects::none, DAS_TOSTRING(OPNAME##_dstr_str));

    void Module_BuiltIn::addRuntime(ModuleLibrary & lib) {
        // printer flags
        addAlias(makePrintFlags());
        // unescape macro
        addReaderMacro(make_smart<UnescapedStringMacro>());
        // function annotations
        addAnnotation(make_smart<CommentAnnotation>());
        addAnnotation(make_smart<MacroInterfaceAnnotation>());
        addAnnotation(make_smart<SkipLockCheckStructureAnnotation>());
        addAnnotation(make_smart<MarkFunctionOrBlockAnnotation>());
        addAnnotation(make_smart<CppAlignmentAnnotation>());
        addAnnotation(make_smart<GenericFunctionAnnotation>());
        addAnnotation(make_smart<MacroFunctionAnnotation>());
        addAnnotation(make_smart<MacroFnFunctionAnnotation>());
        addAnnotation(make_smart<HintFunctionAnnotation>());
        addAnnotation(make_smart<RequestJitFunctionAnnotation>());
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
        addAnnotation(make_smart<NoAotFunctionAnnotation>());
        addAnnotation(make_smart<InitFunctionAnnotation>());
        addAnnotation(make_smart<FinalizeFunctionAnnotation>());
        addAnnotation(make_smart<HybridFunctionAnnotation>());
        addAnnotation(make_smart<UnsafeDerefFunctionAnnotation>());
        addAnnotation(make_smart<SkipLockCheckFunctionAnnotation>());
        addAnnotation(make_smart<MarkUsedFunctionAnnotation>());
        addAnnotation(make_smart<LocalOnlyFunctionAnnotation>());
        addAnnotation(make_smart<PersistentStructureAnnotation>());
        addAnnotation(make_smart<IsYetAnotherVectorTemplateAnnotation>());
        addAnnotation(make_smart<HashBuilderAnnotation>(lib));
        // typeinfo macros
        addTypeInfoMacro(make_smart<ClassInfoMacro>());
        // command line arguments
        addExtern<DAS_BIND_FUN(builtin_das_root)>(*this, lib, "get_das_root",
            SideEffects::accessExternal,"builtin_das_root")
                ->arg("context");
        addExtern<DAS_BIND_FUN(getCommandLineArguments)>(*this, lib, "builtin_get_command_line_arguments",
            SideEffects::accessExternal,"getCommandLineArguments")
                ->arg("arguments");
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
                ->args({"start","step","context"});
        fnCount->arguments[0]->init = make_smart<ExprConstInt>(0);  // start=0
        fnCount->arguments[1]->init = make_smart<ExprConstInt>(1);  // step=0
        auto fnuCount = addExtern<DAS_BIND_FUN(builtin_ucount),SimNode_ExtFuncCallAndCopyOrMove>(*this, lib, "ucount",
            SideEffects::none, "builtin_ucount")
                ->args({"start","step","context"});
        fnuCount->arguments[0]->init = make_smart<ExprConstUInt>(0);  // start=0
        fnuCount->arguments[1]->init = make_smart<ExprConstUInt>(1);  // step=0
        // make-iterator functions
        addExtern<DAS_BIND_FUN(builtin_make_good_array_iterator)>(*this, lib,  "_builtin_make_good_array_iterator",
            SideEffects::modifyArgumentAndExternal, "builtin_make_good_array_iterator")
                ->args({"iterator","array","stride","context"});
        addExtern<DAS_BIND_FUN(builtin_make_fixed_array_iterator)>(*this, lib,  "_builtin_make_fixed_array_iterator",
            SideEffects::modifyArgumentAndExternal, "builtin_make_fixed_array_iterator")
                ->args({"iterator","data","size","stride","context"});;
        addExtern<DAS_BIND_FUN(builtin_make_range_iterator)>(*this, lib,  "_builtin_make_range_iterator",
            SideEffects::modifyArgumentAndExternal, "builtin_make_range_iterator")
                ->args({"iterator","range","context"});;
        addExtern<DAS_BIND_FUN(builtin_make_string_iterator)>(*this, lib,  "_builtin_make_string_iterator",
            SideEffects::modifyArgumentAndExternal, "builtin_make_string_iterator")
                ->args({"iterator","string","context"});
        addExtern<DAS_BIND_FUN(builtin_make_nil_iterator)>(*this, lib,  "_builtin_make_nil_iterator",
            SideEffects::modifyArgumentAndExternal, "builtin_make_nil_iterator")
                ->args({"iterator","context"});;
        addExtern<DAS_BIND_FUN(builtin_make_lambda_iterator)>(*this, lib,  "_builtin_make_lambda_iterator",
            SideEffects::modifyArgumentAndExternal, "builtin_make_lambda_iterator")
                ->args({"iterator","lambda","stride","context"});
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
        addExtern<DAS_BIND_FUN(builtin_error)>(*this, lib, "error",
            SideEffects::modifyExternal, "builtin_error")
                ->args({"text","context","at"});
        addInterop<builtin_sprint,char *,vec4f,PrintFlags>(*this, lib, "sprint",
            SideEffects::modifyExternal, "builtin_sprint")
                ->args({"value","flags"});
        addInterop<builtin_json_sprint,char *,vec4f,bool>(*this, lib, "sprint_json",
            SideEffects::modifyExternal, "builtin_json_sprint")
                ->args({"value","humanReadable"});
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
                ->arg("context");
        // variant
        addExtern<DAS_BIND_FUN(variant_index)>(*this, lib, "variant_index", SideEffects::none, "variant_index");
        addExtern<DAS_BIND_FUN(set_variant_index)>(*this, lib, "set_variant_index",
            SideEffects::modifyArgument, "set_variant_index")
                ->args({"variant","index"})->unsafeOperation = true;
        // heap
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
        auto shcol = addExtern<DAS_BIND_FUN(string_heap_collect)>(*this, lib, "string_heap_collect",
            SideEffects::modifyExternal, "string_heap_collect")
                ->args({"validate","context","at"});
        shcol->unsafeOperation = true;
        shcol->arguments[0]->init = make_smart<ExprConstBool>(false);
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
        addInterop<_builtin_hash,uint64_t,vec4f>(*this, lib, "hash",
            SideEffects::none, "_builtin_hash")
                ->arg("data");
        addExtern<DAS_BIND_FUN(hash64z)>(*this, lib, "hash",
            SideEffects::none, "hash64z")
                ->arg("data");
        // hash builder
        addExtern<DAS_BIND_FUN(builtin_build_hash)>(*this, lib, "build_hash",
                SideEffects::modifyExternal,"builtin_build_hash_T")->args({"block","context","lineinfo"})->setAotTemplate();
        using method_hashBuilder_writeStr = DAS_CALL_MEMBER(HashBuilder::writeStr);
        addExtern< DAS_CALL_METHOD(method_hashBuilder_writeStr) >(*this, lib, "write", SideEffects::modifyArgument,
            DAS_CALL_MEMBER_CPP(HashBuilder::writeStr));
        // locks
        addInterop<builtin_verify_locks,void,vec4f>(*this, lib, "_builtin_verify_locks",
            SideEffects::modifyArgumentAndExternal, "builtin_verify_locks")
                ->arg("anything");
        addExtern<DAS_BIND_FUN(builtin_set_verify_array_locks)>(*this, lib, "set_verify_array_locks",
            SideEffects::modifyArgument, "builtin_set_verify_array_locks")
                ->args({"array","check"})->unsafeOperation = true;
        addExtern<DAS_BIND_FUN(builtin_set_verify_table_locks)>(*this, lib, "set_verify_table_locks",
            SideEffects::modifyArgument, "builtin_set_verify_table_locks")
                ->args({"table","check"})->unsafeOperation = true;
        addExtern<DAS_BIND_FUN(builtin_set_verify_context)>(*this, lib, "set_verify_context_locks",
            SideEffects::modifyExternal, "builtin_set_verify_context")
                ->args({"check","context"})->unsafeOperation = true;
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
        addExtern<DAS_BIND_FUN(builtin_table_unlock)>(*this, lib, "__builtin_table_unlock",
            SideEffects::modifyArgumentAndExternal, "builtin_table_unlock")
                ->args({"table","context","at"});
        addExtern<DAS_BIND_FUN(builtin_table_clear_lock)>(*this, lib, "__builtin_table_clear_lock",
            SideEffects::modifyArgumentAndExternal, "builtin_table_clear_lock")
                ->args({"table","context"});
        addExtern<DAS_BIND_FUN(builtin_table_keys)>(*this, lib, "__builtin_table_keys",
            SideEffects::modifyArgumentAndExternal, "builtin_table_keys")
                ->args({"iterator","table","stride","context"});
        addExtern<DAS_BIND_FUN(builtin_table_values)>(*this, lib, "__builtin_table_values",
            SideEffects::modifyArgumentAndExternal, "builtin_table_values")
                ->args({"iterator","table","stride","context"});
        // array and table free
        addExtern<DAS_BIND_FUN(builtin_array_free)>(*this, lib, "__builtin_array_free",
            SideEffects::modifyArgumentAndExternal, "builtin_array_free")
                ->args({"array","stride","context","at"});
        addExtern<DAS_BIND_FUN(builtin_table_free)>(*this, lib, "__builtin_table_free",
            SideEffects::modifyArgumentAndExternal, "builtin_table_free")
                ->args({"table","sizeOfKey","sizeOfValue","context","at"});
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
        addExtern<DAS_BIND_FUN(das_aligned_alloc16)> (*this, lib, "malloc",
            SideEffects::modifyExternal, "das_aligned_alloc16")
                ->args({"size"})->unsafeOperation = true;
        addExtern<DAS_BIND_FUN(das_aligned_free16)> (*this, lib, "free",
            SideEffects::modifyExternal, "das_aligned_free16")
                ->args({"ptr"})->unsafeOperation = true;
        addExtern<DAS_BIND_FUN(das_aligned_memsize)> (*this, lib, "malloc_usable_size",
            SideEffects::none, "das_aligned_memsize")
                ->args({"ptr"})->unsafeOperation = true;
        // pointer ari
        auto idpi = addExtern<DAS_BIND_FUN(i_das_ptr_inc)>(*this, lib, "i_das_ptr_inc", SideEffects::modifyArgument, "das_ptr_inc");
        idpi->unsafeOperation = true;
        idpi->firstArgReturnType = true;
        idpi->noPointerCast = true;
        auto idpd = addExtern<DAS_BIND_FUN(i_das_ptr_dec)>(*this, lib, "i_das_ptr_dec", SideEffects::modifyArgument, "das_ptr_dec");
        idpd->unsafeOperation = true;
        idpd->firstArgReturnType = true;
        idpd->noPointerCast = true;
        auto idpa = addExtern<DAS_BIND_FUN(i_das_ptr_add)>(*this, lib, "i_das_ptr_add", SideEffects::none, "das_ptr_add");
        idpa->unsafeOperation = true;
        idpa->firstArgReturnType = true;
        idpa->noPointerCast = true;
        auto idps = addExtern<DAS_BIND_FUN(i_das_ptr_sub)>(*this, lib, "i_das_ptr_sub", SideEffects::none, "das_ptr_sub");
        idps->unsafeOperation = true;
        idps->firstArgReturnType = true;
        idps->noPointerCast = true;
        auto idpsa = addExtern<DAS_BIND_FUN(i_das_ptr_set_add)>(*this, lib, "i_das_ptr_set_add", SideEffects::modifyArgument, "das_ptr_set_add");
        idpsa->unsafeOperation = true;
        idpsa->firstArgReturnType = true;
        idpsa->noPointerCast = true;
        auto idpss = addExtern<DAS_BIND_FUN(i_das_ptr_set_sub)>(*this, lib, "i_das_ptr_set_sub", SideEffects::modifyArgument, "das_ptr_set_sub");
        idpss->unsafeOperation = true;
        idpss->firstArgReturnType = true;
        idpss->noPointerCast = true;
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
        addAnnotation(make_smart<DasStringTypeAnnotation>());
        addExtern<DAS_BIND_FUN(to_das_string)>(*this, lib, "string",
            SideEffects::none, "to_das_string")
                ->args({"source","context"});
        addExtern<DAS_BIND_FUN(pass_string)>(*this, lib, "string",
            SideEffects::none, "pass_string")
                ->args({"source"});
        addExtern<DAS_BIND_FUN(set_das_string)>(*this, lib, "clone",
            SideEffects::modifyArgument,"set_das_string")
                ->args({"target","src"});
        addExtern<DAS_BIND_FUN(set_string_das)>(*this, lib, "clone",
            SideEffects::modifyArgument,"set_string_das")
                ->args({"target","src","context"});
        addExtern<DAS_BIND_FUN(peek_das_string)>(*this, lib, "peek",
            SideEffects::modifyExternal,"peek_das_string_T")
                ->args({"src","block","context","line"})->setAotTemplate();
        addExtern<DAS_BIND_FUN(builtin_string_clone)>(*this, lib, "clone_string",
            SideEffects::none, "builtin_string_clone")
                ->args({"src","context"});
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
        addExtern<DAS_BIND_FUN(is_in_aot)>(*this, lib, "is_in_aot",
            SideEffects::worstDefault, "is_in_aot");
        // completion
        addExtern<DAS_BIND_FUN(is_in_completion)>(*this, lib, "is_in_completion",
            SideEffects::worstDefault, "is_in_completion");
        // folding
        addExtern<DAS_BIND_FUN(is_folding)>(*this, lib, "is_folding",
            SideEffects::worstDefault, "is_folding");
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
        // clz, ctz, popcnt
        addExtern<DAS_BIND_FUN(uint32_clz)>(*this, lib, "clz", SideEffects::none, "uint32_clz")->arg("bits");
        addExtern<DAS_BIND_FUN(uint64_clz)>(*this, lib, "clz", SideEffects::none, "uint64_clz")->arg("bits");
        addExtern<DAS_BIND_FUN(uint32_ctz)>(*this, lib, "ctz", SideEffects::none, "uint32_ctz")->arg("bits");
        addExtern<DAS_BIND_FUN(uint64_ctz)>(*this, lib, "ctz", SideEffects::none, "uint64_ctz")->arg("bits");
        addExtern<DAS_BIND_FUN(uint32_popcount)>(*this, lib, "popcnt", SideEffects::none, "uint32_popcount")->arg("bits");
        addExtern<DAS_BIND_FUN(uint64_popcount)>(*this, lib, "popcnt", SideEffects::none, "uint64_popcount")->arg("bits");
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
        addExtern<DAS_BIND_FUN(das_jit_enabled)>(*this, lib, "jit_enabled",
            SideEffects::none, "das_jit_enabled")
                ->args({"context","at"});
    }
}
