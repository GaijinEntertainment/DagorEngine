#include "daScript/misc/platform.h"

#include "daScript/misc/env_cfg.h"
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
#include "daScript/misc/gc_node.h"
#include "daScript/ast/ast_typedecl.h"
#include <inttypes.h>
#if defined(__EMSCRIPTEN__)
#include <malloc.h>
#include <emscripten/stack.h>
#include <unistd.h>
extern "C" char __heap_base;
#endif
#include "daScript/simulate/debug_print.h"
#include "../parser/parser_impl.h"

// arm64 CPU-feature detection headers — MUST be at file scope, not inside `namespace das`.
// Guarded to arm64, so no other target's translation unit is affected by anything here.
#if defined(__aarch64__) || defined(_M_ARM64) || defined(_M_ARM64EC)
    #if defined(_WIN32)
        // IsProcessorFeaturePresent + the PF_ARM_* constants; nothing else in this TU's include
        // graph pulls windows.h in, so without this the arm64 Windows build does not compile.
        #ifndef NOMINMAX
            #define NOMINMAX
        #endif
        #ifndef WIN32_LEAN_AND_MEAN
            #define WIN32_LEAN_AND_MEAN
        #endif
        #include <windows.h>
    #elif defined(__APPLE__)
        #include <sys/sysctl.h>
    #elif defined(__linux__)
        #include <sys/auxv.h>
        #if __has_include(<asm/hwcap.h>)
            #include <asm/hwcap.h>
        #endif
        // kernels predating a given extension omit the macro; the bit positions are ABI-stable
        #ifndef HWCAP_ASIMDDP
            #define HWCAP_ASIMDDP  (1u << 20)
        #endif
        #ifndef HWCAP_ASIMDRDM
            #define HWCAP_ASIMDRDM (1u << 12)
        #endif
        #ifndef HWCAP_ASIMDHP
            #define HWCAP_ASIMDHP  (1u << 10)
        #endif
        #ifndef HWCAP_ATOMICS
            #define HWCAP_ATOMICS  (1u << 8)
        #endif
        #ifndef HWCAP_CRC32
            #define HWCAP_CRC32    (1u << 7)
        #endif
        #ifndef HWCAP_SVE
            #define HWCAP_SVE      (1u << 22)
        #endif
        #ifndef HWCAP_SHA3
            #define HWCAP_SHA3     (1u << 17)
        #endif
        #ifndef HWCAP2_I8MM
            #define HWCAP2_I8MM    (1u << 13)
        #endif
        #ifndef HWCAP2_BF16
            #define HWCAP2_BF16    (1u << 14)
        #endif
    #endif
#endif

#if (defined(__x86_64__) || defined(_M_X64) || defined(__i386) || defined(_M_IX86)) && !defined(_M_ARM64EC)
#if defined(_MSC_VER)
#include <intrin.h>     // __cpuidex, _xgetbv
#else
#include <cpuid.h>      // __get_cpuid_count
#endif
#endif

MAKE_TYPE_FACTORY(HashBuilder, HashBuilder)

namespace das
{
#if defined(__clang__)
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Woverloaded-virtual"
#endif

    struct MarkFunctionAnnotation : FunctionAnnotation {
        MarkFunctionAnnotation(const string & na) : FunctionAnnotation(na) { }
        virtual bool apply(ExprBlock *, ModuleGroup &, AnnotationArgumentList &, string & err) override {
            err = "not supported for block";
            return false;
        }
        virtual bool finalize(ExprBlock *, ModuleGroup &, AnnotationArgumentList &, const AnnotationArgumentList &, string &) override {
            return true;
        }
        virtual bool finalize(const FunctionPtr &, ModuleGroup &, AnnotationArgumentList &, const AnnotationArgumentList &, string &) override {
            return true;
        }
    };

    struct MarkFunctionOrBlockAnnotation : FunctionAnnotation {
        MarkFunctionOrBlockAnnotation() : FunctionAnnotation("marker") { }
        virtual bool apply ( const FunctionPtr &, ModuleGroup &, AnnotationArgumentList &, string & ) override {
            return true;
        }
        virtual bool apply(ExprBlock *, ModuleGroup &, AnnotationArgumentList &, string &) override {
            return true;
        }
        virtual bool finalize(ExprBlock *, ModuleGroup &, AnnotationArgumentList &, const AnnotationArgumentList &, string &) override {
            return true;
        }
        virtual bool finalize(const FunctionPtr &, ModuleGroup &, AnnotationArgumentList &, const AnnotationArgumentList &, string &) override {
            return true;
        }
    };

    struct MacroFunctionAnnotation : MarkFunctionAnnotation {
        MacroFunctionAnnotation() : MarkFunctionAnnotation("_macro") { }
        virtual bool apply(const FunctionPtr & func, ModuleGroup &, AnnotationArgumentList &, string &) override {
            func->macroInit = true;
            auto program = daScriptEnvironment::getBound()->g_Program;
            program->needMacroModule = true;
            return true;
        };
    };

    struct MacroFnFunctionAnnotation : MarkFunctionAnnotation {
        MacroFnFunctionAnnotation() : MarkFunctionAnnotation("macro_function") { }
        virtual bool apply(const FunctionPtr & func, ModuleGroup &, AnnotationArgumentList &, string &) override {
            func->macroFunction = true;
            return true;
        };
    };

    // [clone(paramName1, paramName2, ...)] — marker for daslang-side PERF024 lint.
    // Declares that the callee unconditionally clones the named parameters internally,
    // so callers passing clone_expression(X) at those positions are redundantly cloning.
    // Pure metadata; args stored in func->annotations for lint consumption.
    struct CloneFunctionAnnotation : MarkFunctionAnnotation {
        CloneFunctionAnnotation() : MarkFunctionAnnotation("clone") { }
        virtual bool apply(const FunctionPtr &, ModuleGroup &, AnnotationArgumentList &, string &) override {
            return true;
        };
    };

    // [hot_path] / [no_alloc] / [no_env] / [no_io] / [cold_path] — markers for the daslang-side
    // PERF026-028 lint. Declaring a contract must not drag the checker (and daslib/ast behind it)
    // into every build of the code under contract, so these are registered here and cost nothing;
    // daslib/perf_lint reads the annotation NAMES off func->annotations wherever lint actually runs.
    struct HotPathFunctionAnnotation : MarkFunctionAnnotation {
        HotPathFunctionAnnotation(const string & na) : MarkFunctionAnnotation(na) { }
        virtual bool apply(const FunctionPtr &, ModuleGroup &, AnnotationArgumentList &, string &) override {
            return true;
        };
    };

    struct UnsafeHeapCollectAnnotation : MarkFunctionAnnotation {
        UnsafeHeapCollectAnnotation() : MarkFunctionAnnotation("unsafe_heap_collect") { }
        virtual bool apply(const FunctionPtr &, ModuleGroup &, AnnotationArgumentList &, string &) override {
            return true;
        };
        virtual bool apply(ExprBlock *, ModuleGroup &, AnnotationArgumentList &, string &) override {
            return true;
        };
        virtual bool finalize(ExprBlock * block, ModuleGroup &, AnnotationArgumentList & args, const AnnotationArgumentList &, string &) override {
            auto fn = block->inFunction;
            if ( !fn || !fn->lambda ) return true;
            for ( auto & ann : fn->annotations ) {
                if ( ann->annotation == this ) return true;
            }
            auto decl = new AnnotationDeclaration();
            decl->annotation = this;
            decl->arguments = args;
            decl->at = block->at;
            fn->annotations.push_back(decl);
            return true;
        }
    };

    // [arch(at="<doc>.md#<anchor>")] — binds this function to a section of an architecture
    // document; the LINT026 pass in utils/lint/main.das resolves the citation against the tree.
    // Same doctrine as the hot_path family above: a citation must not drag its checker into every
    // build of the cited code, so the marker is registered here and costs nothing. Compile owns
    // the citation's SHAPE only — whether it RESOLVES is the lint driver's question. Repeat the
    // annotation to cite more than one section.
    //
    // A das structure or class takes the same spelling: a module holds one annotation per NAME
    // (Module::addAnnotation is keyed by it), so "arch" cannot also be a StructureAnnotation, and
    // the structure path — which silently ignores an annotation that is not a StructureAnnotation —
    // accepts the citation unvalidated. LINT026 shape-checks structure citations in its stead.
    static bool verifyArchCitation ( const AnnotationArgumentList & args, string & err ) {
        if ( args.size()!=1 || args[0].name!="at" || args[0].type!=Type::tString ) {
            err = "expecting exactly one argument, at=\"<doc>.md#<anchor>\"";
            return false;
        }
        const string & at = args[0].sValue;
        auto hash = at.find('#');
        if ( hash==string::npos || hash==0 || hash==at.length()-1 ) {
            err = "citation \"" + at + "\" is not \"<doc>.md#<anchor>\" — a document path and an anchor, both non-empty, separated by #";
            return false;
        }
        return true;
    }

    struct ArchFunctionAnnotation : MarkFunctionAnnotation {
        ArchFunctionAnnotation() : MarkFunctionAnnotation("arch") { }
        virtual bool apply(const FunctionPtr &, ModuleGroup &, AnnotationArgumentList & args, string & err) override {
            return verifyArchCitation(args, err);
        };
    };

    struct RequestJitFunctionAnnotation : MarkFunctionAnnotation {
        RequestJitFunctionAnnotation() : MarkFunctionAnnotation("jit") { }
        virtual bool apply(const FunctionPtr & func, ModuleGroup &, AnnotationArgumentList &, string &) override {
            func->requestJit = true;
            return true;
        };
    };

    struct RequestNoJitFunctionAnnotation : MarkFunctionAnnotation {
        RequestNoJitFunctionAnnotation() : MarkFunctionAnnotation("no_jit") { }
        virtual bool apply(const FunctionPtr & func, ModuleGroup &, AnnotationArgumentList &, string &) override {
            func->requestNoJit = true;
            return true;
        };
    };

    // [inline] - a contract, not a hint: every direct call site gets the body spliced in
    // (Program::patchInline, ast_inline.cpp), and a shape the inliner can't handle is a
    // compilation error reported here at lint (post-infer, so generics check per-instantiation)
    struct InlineFunctionAnnotation : MarkFunctionAnnotation {
        InlineFunctionAnnotation() : MarkFunctionAnnotation("inline") { }
        virtual bool apply(const FunctionPtr & func, ModuleGroup &, AnnotationArgumentList &, string & err) override {
            if ( func->neverInline ) {
                err = "[inline] conflicts with [never_inline]";
                return false;
            }
            func->mustInline = true;
            return true;
        };
        virtual bool lint(const FunctionPtr & func, ModuleGroup &, AnnotationArgumentList &,
                const AnnotationArgumentList &, string & err) override {
            return canFunctionInline(func, err) && isInlineRecursionFree(func, err);
        }
    };

    // [never_inline] - keep this function, or this block literal, out of best-effort (auto) inlining
    struct NeverInlineFunctionAnnotation : MarkFunctionAnnotation {
        NeverInlineFunctionAnnotation() : MarkFunctionAnnotation("never_inline") { }
        virtual bool apply(const FunctionPtr & func, ModuleGroup &, AnnotationArgumentList &, string & err) override {
            if ( func->mustInline ) {
                err = "[never_inline] conflicts with [inline]";
                return false;
            }
            func->neverInline = true;
            return true;
        };
        // on a block literal the marker rides block->annotations; the inline pass
        // checks it at the invoke-block gate (canBlockInline, ast_inline.cpp)
        virtual bool apply(ExprBlock *, ModuleGroup &, AnnotationArgumentList &, string &) override {
            return true;
        }
    };

    struct RequestNoDiscardFunctionAnnotation : MarkFunctionAnnotation {
        RequestNoDiscardFunctionAnnotation() : MarkFunctionAnnotation("nodiscard") { }
        virtual bool apply(const FunctionPtr & func, ModuleGroup &, AnnotationArgumentList &, string &) override {
            func->nodiscard = true;
            return true;
        };
    };

    // [temp_string_result] - a contract: the result is always a fresh string allocation (or null),
    // never a passthrough of an input, never retained by the callee. Lets the temp-string reclaim
    // pass queue the result on the 1-slot dispose queue when it dies in the consuming call.
    struct TempStringResultFunctionAnnotation : MarkFunctionAnnotation {
        TempStringResultFunctionAnnotation() : MarkFunctionAnnotation("temp_string_result") { }
        virtual bool apply(const FunctionPtr & func, ModuleGroup &, AnnotationArgumentList &, string &) override {
            func->tempStringResult = true;
            return true;
        };
        virtual bool lint(const FunctionPtr & func, ModuleGroup &, AnnotationArgumentList &,
                const AnnotationArgumentList &, string & err) override {
            // post-infer, so an inferred (auto) return type is concrete by now. The contract is
            // a BY-VALUE fresh allocation: a reference or temporary result points into storage
            // the callee still owns, which is exactly what the flag promises never happens
            if ( !func->result || !func->result->isString() || func->result->ref || func->result->temporary ) {
                err = "[temp_string_result] requires a function that returns a string by value (not string& or string#)";
                return false;
            }
            return true;
        }
    };

    struct DeprecatedFunctionAnnotation : MarkFunctionAnnotation {
        DeprecatedFunctionAnnotation() : MarkFunctionAnnotation("deprecated") { }
        virtual bool apply(const FunctionPtr & func, ModuleGroup &, AnnotationArgumentList &, string &) override {
            func->deprecated = true;
            return true;
        };
        virtual bool verifyCall ( ExprCallFunc * call, AnnotationArgumentList & args,
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
        virtual bool apply(const FunctionPtr & func, ModuleGroup &, AnnotationArgumentList &, string & error) override {
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
        AnnotationDeclarationPtr decl = new AnnotationDeclaration();
        decl->arguments.push_back(AnnotationArgument("message",message));
        decl->annotation = new DeprecatedFunctionAnnotation();
        annotations.push_back(decl);
        return this;
    }

    struct NeverAliasCMRESFunctionAnnotation : MarkFunctionAnnotation {
        NeverAliasCMRESFunctionAnnotation() : MarkFunctionAnnotation("never_alias_cmres") { }
        virtual bool apply(const FunctionPtr & func, ModuleGroup &, AnnotationArgumentList &, string &) override {
            func->neverAliasCMRES = true;
            return true;
        };
    };

    struct AliasCMRESFunctionAnnotation : MarkFunctionAnnotation {
        AliasCMRESFunctionAnnotation() : MarkFunctionAnnotation("alias_cmres") { }
        virtual bool apply(const FunctionPtr & func, ModuleGroup &, AnnotationArgumentList &, string &) override {
            func->aliasCMRES = true;
            return true;
        };
    };


    // dummy annotation for optimization hints on functions or blocks
    struct HintFunctionAnnotation : FunctionAnnotation {
        HintFunctionAnnotation() : FunctionAnnotation("hint") { }
        virtual bool apply(const FunctionPtr &, ModuleGroup &, AnnotationArgumentList &, string &) override {
            return true;
        };
        virtual bool apply(ExprBlock *, ModuleGroup &, AnnotationArgumentList &, string & ) override {
            return true;
        }
        virtual bool finalize(ExprBlock *, ModuleGroup &, AnnotationArgumentList &, const AnnotationArgumentList &, string &) override {
            return true;
        }
        virtual bool finalize(const FunctionPtr &, ModuleGroup &, AnnotationArgumentList &, const AnnotationArgumentList &, string &) override {
            return true;
        }
    };

    struct MakeFunctionUnsafeCallMacro : CallMacro {
        MakeFunctionUnsafeCallMacro() : CallMacro("make_function_unsafe") { }
        virtual ExpressionPtr visit (  Program * prog, Module *, ExprCallMacro * call ) override {
            if ( !call->inFunction ) {
                prog->error("make_function_unsafe can only be used inside a function", "", "", call->at, CompilationError::invalid_macro_context);
                return nullptr;
            }
            call->inFunction->unsafeOperation = true;
            return new ExprConstBool(call->at, true);
        }
    };

    struct UnsafeDerefFunctionAnnotation : MarkFunctionAnnotation {
        UnsafeDerefFunctionAnnotation() : MarkFunctionAnnotation("unsafe_deref") { }
        virtual bool apply(const FunctionPtr & func, ModuleGroup &, AnnotationArgumentList &, string &) override {
            func->unsafeDeref = true;
            return true;
        };
    };

    struct GenericFunctionAnnotation : MarkFunctionAnnotation {
        GenericFunctionAnnotation() : MarkFunctionAnnotation("generic") { }
        virtual bool isGeneric() const override {
            return true;
        }
        virtual bool apply(const FunctionPtr &, ModuleGroup &, AnnotationArgumentList &, string &) override {
            return true;
        };
    };

    struct NoLintFunctionAnnotation : MarkFunctionAnnotation {
        NoLintFunctionAnnotation() : MarkFunctionAnnotation("no_lint") { }
        virtual bool apply(const FunctionPtr &, ModuleGroup &, AnnotationArgumentList &, string &) override {
            return true;
        };
    };

    struct ExportFunctionAnnotation : MarkFunctionAnnotation {
        ExportFunctionAnnotation() : MarkFunctionAnnotation("export") { }
        virtual bool apply(const FunctionPtr & func, ModuleGroup &, AnnotationArgumentList &, string &) override {
            func->exports = true;
            return true;
        };
    };

    struct PInvokeFunctionAnnotation : MarkFunctionAnnotation {
        PInvokeFunctionAnnotation() : MarkFunctionAnnotation("pinvoke") { }
        virtual bool apply(const FunctionPtr & func, ModuleGroup &, AnnotationArgumentList &, string &) override {
            func->pinvoke = true;
            return true;
        };
    };

    struct SideEffectsFunctionAnnotation : MarkFunctionAnnotation {
        SideEffectsFunctionAnnotation() : MarkFunctionAnnotation("sideeffects") { }
        virtual bool apply(const FunctionPtr & func, ModuleGroup &, AnnotationArgumentList &, string &) override {
            func->sideEffectFlags |= uint32_t(SideEffects::userScenario);
            return true;
        };
    };

    struct RunAtCompileTimeFunctionAnnotation : MarkFunctionAnnotation {
        RunAtCompileTimeFunctionAnnotation() : MarkFunctionAnnotation("run") { }
        virtual bool apply(const FunctionPtr & func, ModuleGroup &, AnnotationArgumentList &, string &) override {
            func->hasToRunAtCompileTime = true;
            return true;
        };
    };

    struct UnsafeOpFunctionAnnotation : MarkFunctionAnnotation {
        UnsafeOpFunctionAnnotation() : MarkFunctionAnnotation("unsafe_operation") { }
        virtual bool apply(const FunctionPtr & func, ModuleGroup &, AnnotationArgumentList &, string &) override {
            func->unsafeOperation = true;
            return true;
        };
    };

    struct UnsafeOutsideOfForFunctionAnnotation : MarkFunctionAnnotation {
        UnsafeOutsideOfForFunctionAnnotation() : MarkFunctionAnnotation("unsafe_outside_of_for") { }
        virtual bool apply(const FunctionPtr & func, ModuleGroup &, AnnotationArgumentList &, string &) override {
            func->unsafeOutsideOfFor = true;
            return true;
        };
    };

    struct UnsafeWhenNotCloneArray : MarkFunctionAnnotation {
        UnsafeWhenNotCloneArray() : MarkFunctionAnnotation("unsafe_when_not_clone_array") { }
        virtual bool apply(const FunctionPtr & func, ModuleGroup &, AnnotationArgumentList &, string &) override {
            func->unsafeWhenNotCloneArray = true;
            return true;
        };
    };

    struct NoAotFunctionAnnotation : MarkFunctionAnnotation {
        NoAotFunctionAnnotation() : MarkFunctionAnnotation("no_aot") { }
        virtual bool apply(const FunctionPtr & func, ModuleGroup &, AnnotationArgumentList &, string &) override {
            func->noAot = true;
            return true;
        };
    };

    struct InitFunctionAnnotation : MarkFunctionAnnotation {
        InitFunctionAnnotation() : MarkFunctionAnnotation("init") { }
        virtual bool apply(const FunctionPtr & func, ModuleGroup &, AnnotationArgumentList & args, string &) override {
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
        virtual bool finalize(const FunctionPtr & func, ModuleGroup &, AnnotationArgumentList &, const AnnotationArgumentList &, string & errors) override {
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
        virtual bool apply(const FunctionPtr & func, ModuleGroup &, AnnotationArgumentList & args, string &) override {
            func->shutdown = true;
            for ( auto & arg : args ) {
                if ( arg.name=="late" && arg.type == Type::tBool ) {
                    func->lateShutdown = arg.bValue;
                }
            }
            return true;
        };
        virtual bool finalize(const FunctionPtr & func, ModuleGroup &, AnnotationArgumentList &, const AnnotationArgumentList &, string & errors) override {
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
        virtual bool apply(const FunctionPtr & func, ModuleGroup &, AnnotationArgumentList & args, string &) override {
            for ( auto & fnArg : func->arguments ) {
                if ( auto optArg = args.find(fnArg->name, Type::tBool) ) {
                    fnArg->marked_used = optArg->bValue;
                }
            }
            return true;
        };
        virtual bool apply(ExprBlock * block, ModuleGroup &, AnnotationArgumentList & args, string &) override {
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
        virtual bool apply(const FunctionPtr & fn, ModuleGroup &, AnnotationArgumentList & decl, string & err) override {
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
                    if ( argT->baseType != Type::tFixedArray ) {
                        err = "argument " + fn->arguments[ai]->name + " is expected to be a fixed size array";
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
                           AnnotationArgumentList &, string & ) override {
            return true;
        }
        virtual bool look ( const StructurePtr &, ModuleGroup &,
                           AnnotationArgumentList &, string & ) override {
            return true;
        }
    };

    struct NoDefaultCtorAnnotation : StructureAnnotation {
        NoDefaultCtorAnnotation() : StructureAnnotation("no_default_initializer") {}
        virtual bool touch(const StructurePtr & ps, ModuleGroup &,
                           AnnotationArgumentList &, string & ) override {
            ps->genCtor = true;
            ps->noGenCtor = true;
            return true;
        }
        virtual bool look ( const StructurePtr &, ModuleGroup &,
                           AnnotationArgumentList &, string & ) override {
            return true;
        }
    };

    struct MacroInterfaceAnnotation : StructureAnnotation {
        MacroInterfaceAnnotation() : StructureAnnotation("macro_interface") {}
        virtual bool touch(const StructurePtr & ps, ModuleGroup &,
                           AnnotationArgumentList &, string & ) override {
            ps->macroInterface = true;
            return true;
        }
        virtual bool look ( const StructurePtr &, ModuleGroup &,
                           AnnotationArgumentList &, string & ) override {
            return true;
        }
    };


    struct HybridFunctionAnnotation : MarkFunctionAnnotation {
        HybridFunctionAnnotation() : MarkFunctionAnnotation("hybrid") { }
        virtual bool apply(const FunctionPtr & func, ModuleGroup &, AnnotationArgumentList &, string &) override {
            func->aotHybrid = true;
            return true;
        };
    };

    struct CppAlignmentAnnotation : StructureAnnotation {
        CppAlignmentAnnotation() : StructureAnnotation("cpp_layout") {}
        virtual bool touch(const StructurePtr & ps, ModuleGroup &,
                           AnnotationArgumentList & args, string & ) override {
            ps->cppLayout = true;
            ps->cppLayoutNotPod = !args.getBoolOption("pod", true);
            return true;
        }
        virtual bool look ( const StructurePtr &, ModuleGroup &,
                           AnnotationArgumentList &, string & ) override {
            return true;
        }
    };

    struct SafeWhenUninitializedAnnotation : StructureAnnotation {
        SafeWhenUninitializedAnnotation() : StructureAnnotation("safe_when_uninitialized") {}
        virtual bool touch(const StructurePtr & ps, ModuleGroup &,
                           AnnotationArgumentList &, string & ) override {
            ps->safeWhenUninitialized = true;
            return true;
        }
        virtual bool look ( const StructurePtr &, ModuleGroup &,
                           AnnotationArgumentList &, string & ) override {
            return true;
        }
    };

    struct LocalOnlyFunctionAnnotation : FunctionAnnotation {
        LocalOnlyFunctionAnnotation() : FunctionAnnotation("local_only") { }
        virtual bool apply ( ExprBlock *, ModuleGroup &, AnnotationArgumentList &, string & err ) override {
            err = "not a block annotation";
            return false;
        }
        virtual bool finalize ( ExprBlock *, ModuleGroup &, AnnotationArgumentList &,
                               const AnnotationArgumentList &, string & err ) override {
            err = "not a block annotation";
            return false;
        }
        virtual bool apply ( const FunctionPtr &, ModuleGroup &, AnnotationArgumentList &, string & ) override {
            return true;
        };
        virtual bool finalize ( const FunctionPtr &, ModuleGroup &, AnnotationArgumentList &,
                               const AnnotationArgumentList &, string & ) override {
            return true;
        }
        // [local_only ()]
        virtual bool verifyCall ( ExprCallFunc * call, AnnotationArgumentList & args,
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
                           AnnotationArgumentList &, string & ) override {
            ps->persistent = true;
            return true;
        }
        virtual bool look ( const StructurePtr & st, ModuleGroup &,
                           AnnotationArgumentList & args, string & errors ) override {
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
        virtual bool apply ( const FunctionPtr &, ModuleGroup &, AnnotationArgumentList &, string & ) override { return true; }
        virtual bool apply(ExprBlock *, ModuleGroup &, AnnotationArgumentList &, string &) override { return true; }
        virtual bool finalize(ExprBlock *, ModuleGroup &, AnnotationArgumentList &, const AnnotationArgumentList &, string &) override { return true; }
        virtual bool finalize(const FunctionPtr &, ModuleGroup &, AnnotationArgumentList &, const AnnotationArgumentList &, string &) override { return true; }
        virtual bool isSpecialized() const override { return true; }
    };

    struct LogicOpNotAnnotation : LogicOpAnnotation {
        LogicOpNotAnnotation() : LogicOpAnnotation("!") { }
        LogicOpNotAnnotation(const AnnotationDeclarationPtr & decl) : LogicOpAnnotation("!"), subexpr(decl) { }
        virtual bool isCompatible ( const FunctionPtr & fun, const vector<TypeDeclPtr> & types, const AnnotationDeclaration &, string & errors  ) const override  {
            if ( !subexpr ) return false;
            return !((FunctionAnnotation *)(subexpr->annotation))->isCompatible(fun, types, *subexpr, errors);
        }
        virtual void appendToMangledName( const FunctionPtr & fun, const AnnotationDeclaration &, string & mangledName ) const override {
            if ( !subexpr ) return;
            string mna ;
            ((FunctionAnnotation *)(subexpr->annotation))->appendToMangledName(fun, *subexpr, mna);
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
        virtual void gc_collect ( gc_root * target, gc_root * from ) override {
            Annotation::gc_collect(target, from);
            if ( subexpr ) subexpr->gc_collect(target, from);
        }
        AnnotationDeclarationPtr subexpr = nullptr;
    };

    struct LogicOp2Annotation : LogicOpAnnotation {
        LogicOp2Annotation ( const string & name ) : LogicOpAnnotation(name) { }
        LogicOp2Annotation ( const string & name, const AnnotationDeclarationPtr & arg0, const AnnotationDeclarationPtr & arg1 )
            : LogicOpAnnotation(name), left(arg0), right(arg1) { }
        virtual void appendToMangledName( const FunctionPtr & fun, const AnnotationDeclaration &, string & mangledName ) const override {
            if ( !left || !right ) return;
            string mna1, mna2;
            if ( left->annotation ) {
                ((FunctionAnnotation *)(left->annotation))->appendToMangledName(fun, *left, mna1);
            } else {
                mna1 = "NULL";
            }
            if ( right->annotation ) {
                ((FunctionAnnotation *)(right->annotation))->appendToMangledName(fun, *right, mna2);
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
        virtual void gc_collect ( gc_root * target, gc_root * from ) override {
            Annotation::gc_collect(target, from);
            if ( left ) left->gc_collect(target, from);
            if ( right ) right->gc_collect(target, from);
        }
        AnnotationDeclarationPtr left = nullptr, right = nullptr;
    };

    struct LogicOpAndAnnotation : LogicOp2Annotation {
        LogicOpAndAnnotation ( ) : LogicOp2Annotation("&&") { }
        LogicOpAndAnnotation ( const AnnotationDeclarationPtr & arg0, const AnnotationDeclarationPtr & arg1 )
            : LogicOp2Annotation("&&",arg0,arg1) { }
        virtual bool isCompatible ( const FunctionPtr & fun, const vector<TypeDeclPtr> & types, const AnnotationDeclaration &, string & errors  ) const override  {
            if ( !left || !right ) return false;
            return ((FunctionAnnotation *)(left->annotation))->isCompatible(fun, types, *left, errors) &&
                ((FunctionAnnotation *)(right->annotation))->isCompatible(fun, types, *right, errors);
        }
    };

    struct LogicOpOrAnnotation : LogicOp2Annotation {
        LogicOpOrAnnotation ( ) : LogicOp2Annotation("||") { }
        LogicOpOrAnnotation ( const AnnotationDeclarationPtr & arg0, const AnnotationDeclarationPtr & arg1 )
            : LogicOp2Annotation("||",arg0,arg1) { }
        virtual bool isCompatible ( const FunctionPtr & fun, const vector<TypeDeclPtr> & types, const AnnotationDeclaration &, string & errors  ) const override  {
            if ( !left || !right ) return false;
            return ((FunctionAnnotation *)(left->annotation))->isCompatible(fun, types, *left, errors) ||
                ((FunctionAnnotation *)(right->annotation))->isCompatible(fun, types, *right, errors);
        }
    };

    struct LogicOpXOrAnnotation : LogicOp2Annotation {
        LogicOpXOrAnnotation ( ) : LogicOp2Annotation("^^") { }
        LogicOpXOrAnnotation ( const AnnotationDeclarationPtr & arg0, const AnnotationDeclarationPtr & arg1 )
            : LogicOp2Annotation("^^",arg0,arg1) { }
        virtual bool isCompatible ( const FunctionPtr & fun, const vector<TypeDeclPtr> & types, const AnnotationDeclaration &, string & errors  ) const override {
            if ( !left || !right ) return false;
            return ((FunctionAnnotation *)(left->annotation))->isCompatible(fun, types, *left, errors) !=
                ((FunctionAnnotation *)(right->annotation))->isCompatible(fun, types, *right, errors);
        }
    };

    AnnotationPtr newLogicAnnotation ( LogicAnnotationOp op, const AnnotationDeclarationPtr & arg0, const AnnotationDeclarationPtr & arg1 ) {
        switch ( op ) {
        case LogicAnnotationOp::Not:    return new LogicOpNotAnnotation(arg0);
        case LogicAnnotationOp::And:    return new LogicOpAndAnnotation(arg0,arg1);
        case LogicAnnotationOp::Or:     return new LogicOpOrAnnotation(arg0,arg1);
        case LogicAnnotationOp::Xor:    return new LogicOpXOrAnnotation(arg0,arg1);
        }
        return nullptr;
    }

    AnnotationPtr newLogicAnnotation ( LogicAnnotationOp op ) {
        switch ( op ) {
        case LogicAnnotationOp::Not:    return new LogicOpNotAnnotation();
        case LogicAnnotationOp::And:    return new LogicOpAndAnnotation();
        case LogicAnnotationOp::Or:     return new LogicOpOrAnnotation();
        case LogicAnnotationOp::Xor:    return new LogicOpXOrAnnotation();
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

    char * builtin_get_stackwalk ( bool args, bool vars, bool outOfScope, bool topOnly, Context * context, LineInfoArg * lineInfo ) {
        return context->allocateString(context->getStackWalk(lineInfo, args, vars, outOfScope, topOnly), lineInfo);
    }

    void builtin_terminate ( Context * context, LineInfoArg * at ) {
        context->throw_error_at(at, "terminate");
    }

    // string length lives in the base module next to empty(), not in strings: both are
    // reachable with no require. The int-returning forms carry the same always-on guard
    // as array/table length -- strings past INT_MAX are rare but reachable via fmap, and
    // returning a wrapped negative is worse than stopping when long_length() exists.
    // Context is unused now that the guard replaced stringLengthSafe; it stays in the
    // signature so embedders calling this directly keep compiling.
    int builtin_string_length ( const char * str, Context * ) {
        const size_t len = str ? strlen(str) : 0;
        DAS_VERIFYF(len <= size_t(INT32_MAX), "string length %llu exceeds INT_MAX; use long_length() instead", (unsigned long long)len);
        return int(len);
    }

    int64_t builtin_string_long_length ( const char * str ) {
        const size_t len = str ? strlen(str) : 0;
        DAS_VERIFYF(len <= size_t(INT64_MAX), "string length %llu exceeds INT64_MAX", (unsigned long long)len);
        return int64_t(len);
    }

    int32_t builtin_ext_string_length ( const string & str ) {
        DAS_VERIFYF(str.length() <= size_t(INT32_MAX), "string length %llu exceeds INT_MAX; use long_length() instead", (unsigned long long)str.length());
        return int32_t(str.length());
    }

    int64_t builtin_ext_string_long_length ( const string & str ) {
        DAS_VERIFYF(str.length() <= size_t(INT64_MAX), "string length %llu exceeds INT64_MAX", (unsigned long long)str.length());
        return int64_t(str.length());
    }

    int builtin_table_size ( const Table & arr ) {
        DAS_VERIFYF(arr.size <= uint64_t(INT32_MAX), "table size %llu exceeds INT_MAX; use long_length() instead", (unsigned long long)arr.size);
        return int(arr.size);
    }

    bool builtin_table_empty ( const Table & arr ) {
        return arr.size == 0;
    }

    int builtin_table_capacity ( const Table & arr ) {
        DAS_VERIFYF(arr.capacity <= uint64_t(INT32_MAX), "table capacity %llu exceeds INT_MAX; use long_capacity() instead", (unsigned long long)arr.capacity);
        return int(arr.capacity);
    }

    int64_t builtin_table_long_size ( const Table & arr ) {
        DAS_VERIFYF(arr.size <= uint64_t(INT64_MAX), "table size %llu exceeds INT64_MAX", (unsigned long long)arr.size);
        return int64_t(arr.size);
    }

    int64_t builtin_table_long_capacity ( const Table & arr ) {
        DAS_VERIFYF(arr.capacity <= uint64_t(INT64_MAX), "table capacity %llu exceeds INT64_MAX", (unsigned long long)arr.capacity);
        return int64_t(arr.capacity);
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
        uint64_t newCapacity = cast<uint64_t>::to(args[1]);
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

    // src/builtin/ARCHITECTURE.md#native-memory-metrics
    uint64_t native_thread_stack_size() {
#if defined(__EMSCRIPTEN__)
        return uint64_t(emscripten_stack_get_base() - emscripten_stack_get_end());
#else
        return 0;
#endif
    }
    urange64 native_allocator_stats() {
#if defined(__EMSCRIPTEN__)
        auto info = mallinfo();
        return urange64(uint64_t(info.uordblks), uint64_t(info.fordblks));
#else
        return urange64(0, 0);
#endif
    }
    urange64 native_allocator_extent() {
#if defined(__EMSCRIPTEN__)
        return urange64(uint64_t(uintptr_t(&__heap_base)), uint64_t(uintptr_t(sbrk(0))));
#else
        return urange64(0, 0);
#endif
    }
    urange64 context_idle_fork_memory(int kind, Context * context) { return context->getIdleForkMemory(kind); }
    urange64 context_memory_sizes(Context * context) {
        return urange64(context->stack.size(), context->getGlobalSize());
    }

    void heap_stats ( Context & ctx, uint64_t * bytes ) {
        bytes[0] = ctx.heap->bytesAllocated();
        bytes[1] = ctx.stringHeap->bytesAllocated();
    }

    urange64 heap_operation_counts(bool strings, Context * context) {
        auto * heap = strings ? static_cast<AnyHeapAllocator *>(context->stringHeap.get()) : context->heap.get();
        return urange64(heap->getTotalFrees(), heap->getTotalReallocations());
    }
    bool heap_collect_if_needed(Context * context, LineInfoArg * at) { return context->collectHeapIfMostlyFree(at); }
    urange64 gc_collection_stats(Context * context) { return urange64(context->gcCollections, context->gcTotalUsec); }
    urange64 gc_pause_stats(Context * context) { return urange64(context->gcLastUsec, context->gcPeakUsec); }
    urange64 gc_reclaimed_stats(Context * context) { return urange64(context->gcHeapReclaimed, context->gcStringReclaimed); }
    int64_t gc_last_collection_tick(Context * context) { return context->gcLastTick; }
    urange64 gc_allocation_budget(Context * context) { return urange64(context->gcHeapBudget, context->gcStringBudget); }
    void set_gc_allocation_budget(uint64_t heapBytes, uint64_t stringBytes, Context * context, LineInfoArg * at) {
        if (!heapBytes || !stringBytes) context->throw_error_at(at, "GC allocation budgets must be positive");
        context->gcHeapBudget = heapBytes; context->gcStringBudget = stringBytes;
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

    uint64_t heap_total_allocated ( Context * context ) {
        return context->heap->totalAlignedMemoryAllocated();
    }

    uint64_t max_unreserved_size ( Context * context ) {
        return context->maxUnreservedSize;
    }

    void set_max_unreserved_size ( uint64_t bytes, Context * context ) {
        context->maxUnreservedSize = bytes;
    }

    int32_t heap_depth ( Context * context ) {
        return (int32_t) context->heap->depth();
    }

    uint64_t string_heap_bytes_allocated ( Context * context ) {
        return context->stringHeap->bytesAllocated();
    }

    uint64_t string_heap_total_allocated ( Context * context ) {
        return context->stringHeap->totalAlignedMemoryAllocated();
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

    // gc_node root introspection
    uint64_t gc_thread_root_count() {
        return gc_root::gc_get_thread_root().gc_count;
    }

    uint64_t gc_active_root_count() {
        auto root = gc_root::gc_get_active_root();
        return root ? root->gc_count : 0;
    }

    void gc_thread_root_report() {
        gc_root::gc_get_thread_root().gc_report();
    }

    uint64_t gc_total_id() {
        return gc_root::gc_next_id.load(std::memory_order_relaxed);
    }

    // Detailed report: dumps the LAST N nodes on the thread root as TypeDecl with location info.
    // New nodes are appended at the tail, so the tail shows the most recently leaked nodes.
    // Note: leaked nodes may have dangling sub-type pointers, so we only print safe fields.
    void gc_thread_root_report_detailed( uint64_t max_nodes ) {
        auto & root = gc_root::gc_get_thread_root();
        DAS_FATAL_LOG("gc_thread_root: count=%" PRIu64 " (showing last %" PRIu64 ")\n",
            root.gc_count, max_nodes < root.gc_count ? max_nodes : root.gc_count);
        auto node = root.gc_last;
        uint64_t count = 0;
        gc_node * start = node;
        while ( start && count < max_nodes ) {
            start = start->gc_prev;
            count++;
        }
        node = start ? start->gc_next : root.gc_first;
        uint64_t idx = root.gc_count > max_nodes ? root.gc_count - max_nodes : 0;
        while ( node ) {
            auto td = static_cast<TypeDecl *>(node);
            // Only print safe fields — describe() may crash on dangling sub-type pointers
            const char * locFile = (td->at.fileInfo) ? td->at.fileInfo->name.c_str() : nullptr;
            if ( locFile ) {
                DAS_FATAL_LOG("  [%" PRIu64 "] id=%" PRIu64 " baseType=%d at %s:%" PRIi32 "\n",
                    idx, td->gc_id, int(td->baseType), locFile, td->at.line);
            } else {
                DAS_FATAL_LOG("  [%" PRIu64 "] id=%" PRIu64 " baseType=%d (no location)\n",
                    idx, td->gc_id, int(td->baseType));
            }
            node = node->gc_next;
            idx++;
        }
    }

    int32_t builtin_frame_position ( Context *, LineInfoArg * info ) {
        // the call site's frame position (see LINEINFO_FRAME_POS_TAG); 0 when the program
        // carries no positions (no gc/debugger)
        return ( info && (info->last_line & LINEINFO_FRAME_POS_TAG) ) ? int32_t(info->last_column) : 0;
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

    void builtin_table_set_scratch ( Table & tab, bool value, Context * ) {
        tab.scratch = value;
    }

    bool builtin_table_is_scratch ( const Table & tab ) {
        return tab.scratch;
    }

    void builtin_table_tag ( Table & tab, const char * name, Context * context ) {
        // Debug helper: tag the table's current heap block with `name` so it shows
        // up in heap reports under that name. Requires `options track_allocations`
        // (the heap's mark_comment is a no-op otherwise). The tag is preserved
        // across realloc by TableHash::reserveInternal, which reads the previous
        // tag before overwriting with the generic "table" default. `name` is
        // stored as-is in bigStuffComment; the caller owns its lifetime. The
        // common case is a daslang literal (constStringHeap, never swept);
        // dynamic daslang strings live in stringHeap, whose GC is skipped while
        // track_allocations is on (see Context::collectHeap).
        if ( tab.data && name ) context->heap->mark_comment(tab.data, name);
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

    void builtin_make_fixed_array_iterator ( Sequence & result, void * data, int64_t size, int stride, Context * context, LineInfoArg * at ) {
        // Negative size would underflow into a huge uint64 and the iterator would run past
        // the array bounds — clamp to empty instead. Same pattern as builtin_make_temp_array_i64.
        char * iter = context->allocateIterator(sizeof(FixedArrayIterator), "fixed array iterator", at);
        new (iter) FixedArrayIterator((char *)data, size < 0 ? uint64_t(0) : uint64_t(size), uint32_t(stride), at);
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
                uint64_t oldSize = dim.capacity*uint64_t(szt);
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
        } else {
            // no backing store to free, but delete still resets the scratch mark - the one
            // flag settable on a dataless container; lock state must survive untouched
            dim.scratch = false;
        }
    }

    void builtin_table_free ( Table & tab, int szk, int szv, Context * __context__, LineInfoArg * at ) {
        if ( tab.data ) {
            if ( !tab.isLocked() || tab.hopeless ) {
                uint64_t oldSize = tab.capacity*(uint64_t(szk)+uint64_t(szv)) + tab.capacity*tableHashSlotBytes(tab);
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
        } else {
            tab.scratch = false;
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
        virtual TypeDeclPtr getAstType ( ModuleLibrary & lib, ExpressionPtr, string & ) override {
            return typeFactory<void *>::make(lib);
        }
        virtual SimNode * simluate ( Context * context, ExpressionPtr expr, string & )  override {
            auto exprTypeInfo = static_cast<ExprTypeInfo*>(expr);
            TypeInfo * typeInfo = context->thisHelper->makeTypeInfo(nullptr, exprTypeInfo->typeexpr);
            return context->code->makeNode<SimNode_TypeInfo>(expr->at, typeInfo);
        }
        virtual void aotPrefix ( TextWriter & ss, ExpressionPtr ) override {
            ss << "(void *)(&";
        }
        virtual void aotSuffix ( TextWriter & ss, ExpressionPtr ) override {
            ss << ")";
        }
        virtual bool aotNeedTypeInfo ( ExpressionPtr ) const override {
            return true;
        }
    };

    bool is_compiling (  ) {
        if ( daScriptEnvironment::getBound() && daScriptEnvironment::getBound()->g_Program ) {
            return daScriptEnvironment::getBound()->g_Program->isCompiling || daScriptEnvironment::getBound()->g_Program->isSimulating;
        }
        return false;

    }

    bool is_standalone_exe ( ) {
        return false;
    }

    // DAS_SAFE_HASH of the host binary (anyhash.h). When 0, string hashing uses the
    // fast 16-bit-over-read load; the JIT can then inline a matching hash. When 1
    // (e.g. ASAN), the runtime reads byte-by-byte and the JIT must not over-read, so
    // it falls back to the runtime call instead of emitting the fast intrinsic.
    bool is_safe_hash ( ) {
        return DAS_SAFE_HASH != 0;
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
            return new ExprConstString(expr->at,expr->sequence);
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
        auto ft = new TypeDecl(Type::tBitfield, cppBindingLineInfo());
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

    string commandLineArgumentOccurrences ( const string & flag ) {
        string out;
        auto argv = (char **) g_CommandLineArguments.data;
        uint32_t argc = g_CommandLineArguments.size;
        string eq = flag + "=";
        for ( uint32_t i=0; i<argc; ++i ) {
            const char * a = argv[i] ? argv[i] : "";
            // NUL-separated: an argv string cannot contain NUL, so no two argument lists share a stream
            if ( flag == a ) {
                out += a;
                out += '\0';
                if ( i+1<argc && argv[i+1] ) out += argv[i+1];
                out += '\0';
            } else if ( strncmp(a, eq.c_str(), eq.size())==0 ) {
                out += a;
                out += '\0';
            }
        }
        return out;
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

    char * builtin_executable_path ( Context * context, LineInfoArg * at ) {
        string exe = getExecutablePathUtf8();
        return exe.empty() ? nullptr : context->allocateString(exe, at);
    }

    char * builtin_shared_module_extension ( Context * context, LineInfoArg * at ) {
#ifdef NDEBUG
        return context->allocateString(".shared_module", at);
#else
        return context->allocateString("_debug.shared_module", at);
#endif
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

    void set_das_string_bytes(string & str, const TArray<uint8_t> & bytes) {
        str.assign((const char *)bytes.data, bytes.size);
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
        // Negative size would wrap into a huge uint capacity; clamp to empty array instead.
        array_mark_locked(arr, (char *)data, size < 0 ? uint64_t(0) : uint64_t(size));
        vec4f args[1];
        args[0] = cast<Array &>::from(arr);
        context->invoke(block, args, nullptr, at);
    }

    void builtin_make_temp_array ( Array & arr, void * data, int size ) {
        // Negative size would underflow to huge uint64 — clamp to empty array instead.
        array_mark_locked(arr, (char *)data, size < 0 ? uint64_t(0) : uint64_t(size));
    }

    void builtin_make_temp_array_i64 ( Array & arr, void * data, int64_t size ) {
        // Negative size would underflow to huge uint64 — clamp to empty array instead.
        array_mark_locked(arr, (char *)data, size < 0 ? uint64_t(0) : uint64_t(size));
    }

    // release a borrowed (mark_locked) array view WITHOUT freeing the storage — the bytes
    // belong to someone else (a file mapping, another allocator). The inverse of
    // builtin_make_temp_array for views that outlive a temp scope (the model-image loader
    // resets its borrowed plane views through this before unmapping).
    void builtin_forget_temp_array ( Array & arr, Context * context, LineInfoArg * at ) {
        if ( !array_forget_locked(arr) ) {
            context->throw_error_at(at, "forget_temp_array: not a borrowed (locked) array view");
        }
    }

    static int logMinLevel () {
        const char * envRaw = get_dasenv_log_level();
        if ( !envRaw || !envRaw[0] ) return int(LogLevel::warning);
        char env[16];
        size_t n = 0;
        for ( ; envRaw[n] && n < sizeof(env)-1; ++n ) env[n] = char(tolower(uint8_t(envRaw[n])));
        env[n] = 0;
        if ( strcmp(env,"trace")==0 )    return int(LogLevel::trace);
        if ( strcmp(env,"debug")==0 )    return int(LogLevel::debug);
        if ( strcmp(env,"info")==0 )     return int(LogLevel::info);
        if ( strcmp(env,"warning")==0 )  return int(LogLevel::warning);
        if ( strcmp(env,"error")==0 )    return int(LogLevel::error);
        if ( strcmp(env,"critical")==0 ) return int(LogLevel::critical);
        return atoi(env);
    }

    void toLog ( int level, const char * text, Context * context, LineInfoArg * at ) {
        if ( level < logMinLevel() ) return;
        context->to_out(at, level, text);
    }

    void diagnosticsToStderr() {
        // For a program whose stdout is a protocol rather than its own output:
        // a leak dump or a fatal landing there is read as a frame, and the
        // reader is gone before anyone sees the text.
        textPrinterToStderr();
    }

    bool diagnosticsToFile ( const char * path ) {
        // The same, for a program whose stderr nobody keeps: a launcher that
        // records neither stream leaves the death rattle nowhere at all, so it
        // goes to a file the program names and its owner reads.
        return path ? textPrinterToFile(path) : false;
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

    bool das_is_exceptions_enabled() {
        #if DAS_ENABLE_EXCEPTIONS
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

    bool is_in_lint_check ( ) {
        if ( daScriptEnvironment::getBound() && daScriptEnvironment::getBound()->g_Program ) {
            return daScriptEnvironment::getBound()->g_Program->policies.lint_check;
        }
        return false;
    }

    bool is_building_documentation ( ) {
        if ( daScriptEnvironment::getBound() && daScriptEnvironment::getBound()->g_Program ) {
            return daScriptEnvironment::getBound()->g_Program->policies.building_documentation;
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
        // Copy into context string heap so daslang owns the returned pointer.
        auto allocFromMod = [&](Module * mod) -> const char * {
            if ( !mod || mod->fileName.empty() ) return nullptr;
            return context ? context->allocateString(mod->fileName.data(), uint32_t(mod->fileName.size()), nullptr) : nullptr;
        };
        if ( context && context->thisProgram ) {
            string modName = name ? name : "";
            if ( modName.empty() ) {
                // return the main module's file name
                if ( const char * res = allocFromMod(context->thisProgram->thisModule.get()) ) return res;
            } else {
                // search program's library for named module
                Module * found = nullptr;
                context->thisProgram->library.foreach([&](Module * mod) {
                    if ( mod->name == modName && !mod->fileName.empty() ) {
                        found = mod;
                        return false;
                    }
                    return true;
                }, modName);
                if ( const char * res = allocFromMod(found) ) return res;
            }
        }
        // fallback to global module list
        return allocFromMod(Module::require(name ? name : ""));
    }

// emscripten browser-loop for eval_main_loop: drives a daslang block once per
// requestAnimationFrame instead of a blocking while loop. Enables standalone
// cross-compiled graphics apps (glfw/opengl) to run in the browser.
#define TRY_MAIN_LOOP   1

#ifdef __EMSCRIPTEN__
#include <emscripten.h>
#if TRY_MAIN_LOOP
    // Heap-allocated and the block copied BY VALUE so both survive the
    // emscripten_set_main_loop_arg(..., simulate_infinite_loop=true) stack
    // unwind: that flag throws to unwind the C++ stack (so builtin_main_loop
    // never returns), which would otherwise destroy a stack-local arg + the
    // caller's TBlock. The daslang context stack the block captures is separate
    // and persists across the unwind, so the copied Block stays valid.
    struct MainLoopArg {
        Context * context;
        LineInfoArg * at;
        Block block;
    };

    void main_loop_arg ( void * arg ) {
        auto mla = (MainLoopArg *) arg;
        vec4f args[1];
        args[0] = v_zero();
        if ( !cast<bool>::to(mla->context->invoke(mla->block, args, nullptr, mla->at)) ) {
            emscripten_cancel_main_loop();
        }
    }
#endif
#endif

    void builtin_main_loop ( const TBlock<bool> & block, Context * context, LineInfoArg * at ) {
#ifndef __EMSCRIPTEN__
        vec4f args[1];
        args[0] = v_zero();
        while ( true ) {
            auto res = context->invoke(block, args, nullptr, at);
            if ( !cast<bool>::to(res) ) break;
        }
#else
#if TRY_MAIN_LOOP
        // simulate_infinite_loop=true → never returns, so the standalone-exe
        // entry's jit_shutdown() after the daslang main is never reached and the
        // runtime/context stay alive for the rAF callbacks. arg leaks by design
        // (lives for the whole program). 0 = browser rAF cadence.
        auto * arg = new MainLoopArg{ context, at, block };
        emscripten_set_main_loop_arg(main_loop_arg, arg, 0, true);
#endif
#endif
    }

    bool das_is_jit_function ( const Func func ) {
        auto simfn = func.PTR;
        if ( !simfn ) return false;
        return simfn->code && simfn->code->rtti_node_isJit();
    }

    bool das_is_aot_function ( const Func func ) {
        auto simfn = func.PTR;
        return simfn && simfn->aot;
    }

    bool das_jit_enabled ( Context * context, LineInfoArg * at ) {
        if ( !context->thisProgram ) context->throw_error_at(at, "can only query for jit during compilation");
        auto & policies = context->thisProgram->policies;
        return policies.jit_enabled && !policies.jit_emit_object;
    }

    bool das_aot_enabled ( Context * context, LineInfoArg * at ) {
        if ( !context->thisProgram ) context->throw_error_at(at, "can only query for aot during compilation");
        return context->thisProgram->policies.aot;
    }

#define STR_DSTR_REG(OPNAME,EXPR) \
    addExternInline<DAS_BIND_FUN(OPNAME##_str_dstr)>(*this, lib, #EXPR, SideEffects::none, DAS_TOSTRING(OPNAME##_str_dstr)); \
    addExternInline<DAS_BIND_FUN(OPNAME##_dstr_str)>(*this, lib, #EXPR, SideEffects::none, DAS_TOSTRING(OPNAME##_dstr_str));

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

    // The --jit-target triple this compile emits for, "" on a native compile. Read from the
    // command line (`--jit-target=<triple>` or `--jit-target <triple>` after `--`), already in
    // g_CommandLineArguments at process start, so it is valid at .das_module-initialize time,
    // before the jit codegen macro runs. A property of the whole compile (the triple is fixed
    // before the first module parses), so the folds below are compile-time constants: an artifact
    // for another machine decides its tiers from the target. das_get_cross_platform_name maps the
    // triple to a platform name - only the wasm triples are mapped today (-> "emscripten"), which
    // lets a .das_module register a target-native module only when cross-compiling for that
    // target (dasOpenGL registers its wasm GLES3 module for emscripten; a desktop run keeps the
    // pure-das opengl.das).
    const char * das_get_target_triple() {
        char ** argv = (char **) g_CommandLineArguments.data;
        uint64_t n = g_CommandLineArguments.size;
        const char * found = "";   // the LAST occurrence wins, as every other argv reader here takes it
        for ( uint64_t i=0; i<n; ++i ) {
            const char * a = argv[i];
            if ( !a ) continue;
            if ( strncmp(a, "--jit-target=", 13)==0 ) {
                found = a + 13;
            } else if ( strcmp(a, "--jit-target")==0 && i+1<n && argv[i+1] ) {
                found = argv[i+1];
            }
        }
        return found;
    }

    const char * das_get_cross_platform_name() {
        const char * triple = das_get_target_triple();
        if ( !triple[0] ) return "";
        if ( strncmp(triple, "wasm", 4)==0 || strstr(triple, "emscripten") ) return "emscripten";
        return "";
    }

    // x86, arm, etc
    const char * das_get_architecture_name() {
        #if defined(_M_ARM64EC)
            return "arm64ec";
        #elif defined(__x86_64__) || defined(_M_X64)
            return "x86_64";
        #elif defined(__i386) || defined(_M_IX86)
            return "x86";
        #elif defined(__aarch64__)
            return "arm64";
        #elif defined(__arm__) || defined(_M_ARM)
            return "arm";
        #elif defined(__wasm64__)
            return "wasm64";
        #elif defined(__EMSCRIPTEN__) || defined(__wasm32__)
            return "wasm32";
        #else
            return "unknown";
        #endif
    }

    // the architecture the compile EMITS for: the target triple's on a cross-compile, the host's
    // otherwise. get_architecture_name is a #if on the host compiler, so a kernel-tier gate that
    // asks it registers the host's tiers into a foreign artifact; this is the one such a gate asks.
    const char * das_get_target_architecture_name() {
        const char * triple = das_get_target_triple();
        if ( !triple[0] ) return das_get_architecture_name();
        if ( strncmp(triple, "wasm64", 6)==0 ) return "wasm64";
        if ( strncmp(triple, "wasm", 4)==0 ) return "wasm32";
        if ( strncmp(triple, "x86_64", 6)==0 || strncmp(triple, "amd64", 5)==0 ) return "x86_64";
        if ( strncmp(triple, "aarch64", 7)==0 || strncmp(triple, "arm64", 5)==0 ) return "arm64";
        if ( strncmp(triple, "i386", 4)==0 || strncmp(triple, "i686", 4)==0 ) return "x86";
        if ( strncmp(triple, "arm", 3)==0 || strncmp(triple, "thumb", 5)==0 ) return "arm";
        return "unknown";
    }

#if defined(__aarch64__) || defined(_M_ARM64) || defined(_M_ARM64EC)
    #if defined(__APPLE__)
        static bool das_sysctl_flag ( const char * name ) {
            int val = 0; size_t sz = sizeof(val);
            return sysctlbyname(name, &val, &sz, nullptr, 0)==0 && val!=0;
        }
    #endif

    // Optional arm64 extensions, by LLVM target-feature name. Unknown names are false, as on x86 —
    // a kernel gated on one must never register where its instructions would trap.
    static bool das_arm64_feature ( const char * f ) {
    #if defined(__APPLE__)
        if ( strcmp(f, "dotprod")==0 )  return das_sysctl_flag("hw.optional.arm.FEAT_DotProd");
        if ( strcmp(f, "i8mm")==0 )     return das_sysctl_flag("hw.optional.arm.FEAT_I8MM");
        if ( strcmp(f, "bf16")==0 )     return das_sysctl_flag("hw.optional.arm.FEAT_BF16");
        if ( strcmp(f, "fullfp16")==0 ) return das_sysctl_flag("hw.optional.arm.FEAT_FP16");
        if ( strcmp(f, "lse")==0 )      return das_sysctl_flag("hw.optional.arm.FEAT_LSE");
        if ( strcmp(f, "rdm")==0 )      return das_sysctl_flag("hw.optional.arm.FEAT_RDM");
        if ( strcmp(f, "sha3")==0 )     return das_sysctl_flag("hw.optional.arm.FEAT_SHA3");
        if ( strcmp(f, "crc")==0 )      return das_sysctl_flag("hw.optional.armv8_crc32");
        if ( strcmp(f, "sve")==0 )      return das_sysctl_flag("hw.optional.arm.FEAT_SVE");
        return false;
    #elif defined(__linux__)
        const unsigned long hw  = getauxval(AT_HWCAP);
        const unsigned long hw2 = getauxval(AT_HWCAP2);
        if ( strcmp(f, "dotprod")==0 )  return (hw  & HWCAP_ASIMDDP) != 0;
        if ( strcmp(f, "i8mm")==0 )     return (hw2 & HWCAP2_I8MM) != 0;
        if ( strcmp(f, "bf16")==0 )     return (hw2 & HWCAP2_BF16) != 0;
        if ( strcmp(f, "fullfp16")==0 ) return (hw  & HWCAP_ASIMDHP) != 0;
        if ( strcmp(f, "lse")==0 )      return (hw  & HWCAP_ATOMICS) != 0;
        if ( strcmp(f, "rdm")==0 )      return (hw  & HWCAP_ASIMDRDM) != 0;
        if ( strcmp(f, "sha3")==0 )     return (hw  & HWCAP_SHA3) != 0;
        if ( strcmp(f, "crc")==0 )      return (hw  & HWCAP_CRC32) != 0;
        if ( strcmp(f, "sve")==0 )      return (hw  & HWCAP_SVE) != 0;
        return false;
    #elif defined(_WIN32)
        #ifdef PF_ARM_V82_DP_INSTRUCTIONS_AVAILABLE    // newer than the CRC32 bit below
        if ( strcmp(f, "dotprod")==0 )
            return IsProcessorFeaturePresent(PF_ARM_V82_DP_INSTRUCTIONS_AVAILABLE) != 0;
        #endif
        if ( strcmp(f, "crc")==0 )
            return IsProcessorFeaturePresent(PF_ARM_V8_CRC32_INSTRUCTIONS_AVAILABLE) != 0;
        return false;
    #else
        (void)f;
        return false;
    #endif
    }
#endif

#if (defined(__x86_64__) || defined(_M_X64) || defined(__i386) || defined(_M_IX86)) && !defined(_M_ARM64EC)
    static void das_cpuidex ( int leaf, int subleaf, int * regs ) {
        #if defined(_MSC_VER)
            __cpuidex(regs, leaf, subleaf);
        #else
            unsigned int a=0, b=0, c=0, d=0;
            __get_cpuid_count(unsigned(leaf), unsigned(subleaf), &a, &b, &c, &d);
            regs[0]=int(a); regs[1]=int(b); regs[2]=int(c); regs[3]=int(d);
        #endif
    }
    static uint64_t das_xgetbv0 () {
        #if defined(_MSC_VER)
            return _xgetbv(0);
        #else
            unsigned int eax, edx;
            __asm__ volatile ( "xgetbv" : "=a"(eax), "=d"(edx) : "c"(0u) );
            return (uint64_t(edx) << 32u) | eax;
        #endif
    }
#endif

    // Runtime CPU-feature truth for kernel/backend gating — e.g. the dasLLAMA x64 backends register
    // (and the JIT emits AVX2 intrinsics) only when cpu_supports("avx2"). On x86/x86_64 this is cpuid
    // plus the OSXSAVE/XGETBV dance: a feature reports true only when the CPU has it AND the OS saves
    // the register state it needs (ymm for avx*, zmm/opmask for avx512*). Fail-closed by design:
    // unknown feature names, and every name on a non-x86 architecture, return false — a backend gated
    // on cpu_supports can never register where its instructions can't execute.
    bool das_cpu_supports ( const char * feature ) {
        if ( !feature ) return false;
#if defined(__aarch64__) || defined(_M_ARM64) || defined(_M_ARM64EC)
        // arm64 has no cpuid: macOS answers via sysctl, Linux via the AT_HWCAP auxv words, Windows
        // via IsProcessorFeaturePresent. Names are the LLVM target-feature spellings, so they match
        // what DAS_JIT_ARM64_FORCE_FEATURES and `llc -mattr` take verbatim (same rule as AMX above).
        if ( strcmp(feature, "neon")==0 ) return true;      // baseline on every arm64
        if ( strcmp(feature, "fp-armv8")==0 ) return true;  // ditto
        return das_arm64_feature(feature);
#elif defined(__x86_64__) || defined(_M_X64) || defined(__i386) || defined(_M_IX86)
        int r0[4] = {0,0,0,0};
        das_cpuidex(0, 0, r0);                                  // r0[0] = max basic leaf
        int r1[4] = {0,0,0,0};
        if ( r0[0] >= 1 ) das_cpuidex(1, 0, r1);
        int r7[4] = {0,0,0,0};
        if ( r0[0] >= 7 ) das_cpuidex(7, 0, r7);
        int r71[4] = {0,0,0,0};
        if ( r0[0] >= 7 && r7[0] >= 1 ) das_cpuidex(7, 1, r71); // leaf 7 eax = max subleaf
        const bool osxsave = (r1[2] & (1<<27)) != 0;
        const uint64_t xcr0 = osxsave ? das_xgetbv0() : 0;
        const bool os_ymm = (xcr0 & 0x6) == 0x6;                // OS saves xmm+ymm state
        const bool os_zmm = (xcr0 & 0xE6) == 0xE6;              // + opmask/zmm state
        const bool os_amx = (xcr0 & 0x60000) == 0x60000;        // + XTILECFG/XTILEDATA state
        if ( strcmp(feature, "sse4.2")==0 )     return (r1[2] & (1<<20)) != 0;
        if ( strcmp(feature, "fma")==0 )        return os_ymm && (r1[2] & (1<<12)) != 0;
        if ( strcmp(feature, "f16c")==0 )       return os_ymm && (r1[2] & (1<<29)) != 0;
        if ( strcmp(feature, "avx")==0 )        return os_ymm && (r1[2] & (1<<28)) != 0;
        if ( strcmp(feature, "avx2")==0 )       return os_ymm && (r7[1] & (1<<5)) != 0;
        if ( strcmp(feature, "avxvnni")==0 )    return os_ymm && (r71[0] & (1<<4)) != 0;
        if ( strcmp(feature, "avxvnniint8")==0 ) return os_ymm && (r71[3] & (1<<4)) != 0;
        if ( strcmp(feature, "avx512f")==0 )    return os_zmm && (r7[1] & (1<<16)) != 0;
        if ( strcmp(feature, "avx512bw")==0 )   return os_zmm && (r7[1] & (1<<30)) != 0;
        if ( strcmp(feature, "avx512vl")==0 )   return os_zmm && (r7[1] & (1u<<31)) != 0;
        if ( strcmp(feature, "avx512vnni")==0 ) return os_zmm && (r7[2] & (1<<11)) != 0;
        if ( strcmp(feature, "avx512vbmi")==0 ) return os_zmm && (r7[2] & (1<<1)) != 0;
        // AMX names use the LLVM hyphen spelling so cpuid names == target-feature names
        // (DAS_JIT_X64_FORCE_FEATURES / llc -mattr pass them through verbatim). XCR0 tile
        // bits are kernel-boot truth; the per-process XTILEDATA grant (Linux arch_prctl)
        // is a separate, runtime step — the dasLLAMA amx witness does it, not this query.
        if ( strcmp(feature, "amx-tile")==0 )   return os_amx && (r7[3] & (1<<24)) != 0;
        if ( strcmp(feature, "amx-int8")==0 )   return os_amx && (r7[3] & (1<<24)) != 0 && (r7[3] & (1<<25)) != 0;
#endif
        return false;
    }

    void Module_BuiltIn::addRuntime(ModuleLibrary & lib) {
        // printer flags
        addAlias(makePrintFlags());
        // unescape macro
        addReaderMacro(new UnescapedStringMacro());
        // function annotations
        addAnnotation(new CommentAnnotation());
        addAnnotation(new NoDefaultCtorAnnotation());
        addAnnotation(new MacroInterfaceAnnotation());
        addAnnotation(new MarkFunctionOrBlockAnnotation());
        addAnnotation(new CppAlignmentAnnotation());
        addAnnotation(new SafeWhenUninitializedAnnotation());
        addAnnotation(new GenericFunctionAnnotation());
        addAnnotation(new MacroFunctionAnnotation());
        addAnnotation(new MacroFnFunctionAnnotation());
        addAnnotation(new CloneFunctionAnnotation());
        addAnnotation(new HotPathFunctionAnnotation("hot_path"));
        addAnnotation(new UnsafeHeapCollectAnnotation());
        addAnnotation(new HotPathFunctionAnnotation("no_alloc"));
        addAnnotation(new HotPathFunctionAnnotation("no_env"));
        addAnnotation(new HotPathFunctionAnnotation("no_io"));
        addAnnotation(new HotPathFunctionAnnotation("cold_path"));
        addAnnotation(new ArchFunctionAnnotation());
        addAnnotation(new HintFunctionAnnotation());
        addAnnotation(new RequestJitFunctionAnnotation());
        addAnnotation(new RequestNoJitFunctionAnnotation());
        addAnnotation(new InlineFunctionAnnotation());
        addAnnotation(new NeverInlineFunctionAnnotation());
        addAnnotation(new RequestNoDiscardFunctionAnnotation());
        addAnnotation(new TempStringResultFunctionAnnotation());
        addAnnotation(new DeprecatedFunctionAnnotation());
        addAnnotation(new AliasCMRESFunctionAnnotation());
        addAnnotation(new NeverAliasCMRESFunctionAnnotation());
        addAnnotation(new ExportFunctionAnnotation());
        addAnnotation(new PInvokeFunctionAnnotation());
        addAnnotation(new NoLintFunctionAnnotation());
        addAnnotation(new SideEffectsFunctionAnnotation());
        addAnnotation(new RunAtCompileTimeFunctionAnnotation());
        addAnnotation(new UnsafeOpFunctionAnnotation());
        addAnnotation(new UnsafeOutsideOfForFunctionAnnotation());
        addAnnotation(new UnsafeWhenNotCloneArray());
        addAnnotation(new NoAotFunctionAnnotation());
        addAnnotation(new InitFunctionAnnotation());
        addAnnotation(new FinalizeFunctionAnnotation());
        addAnnotation(new HybridFunctionAnnotation());
        addAnnotation(new UnsafeDerefFunctionAnnotation());
        addAnnotation(new MarkUsedFunctionAnnotation());
        addAnnotation(new LocalOnlyFunctionAnnotation());
        addAnnotation(new PersistentStructureAnnotation());
        addAnnotation(new IsYetAnotherVectorTemplateAnnotation());
        addAnnotation(new IsDimAnnotation());
        addAnnotation(new IsRefTypeAnnotation());
        addAnnotation(new HashBuilderAnnotation(lib));
        addAnnotation(new TypeFunctionFunctionAnnotation());
        // and call macro
        {
            auto newM = new MakeFunctionUnsafeCallMacro();
            ownedCallMacros.push_back(unique_ptr<CallMacro>(newM));
            addCallMacro(newM->name, [this, newM](const LineInfo & at) -> ExprLooksLikeCall * {
                auto ecm = new ExprCallMacro(at, newM->name);
                ecm->macro = newM;
                newM->module = this;
                return ecm;
            });
        }
        // string
        addAnnotation(new DasStringTypeAnnotation());
        // typeinfo macros
        addTypeInfoMacro(new ClassInfoMacro());
        // command line arguments
        addExternInline<DAS_BIND_FUN(builtin_das_root)>(*this, lib, "get_das_root",
            SideEffects::accessExternal,"builtin_das_root")
                ->args({"context","at"})->setTempStringResult();
        addExternInline<DAS_BIND_FUN(builtin_executable_path)>(*this, lib, "get_executable_path",
            SideEffects::accessExternal,"builtin_executable_path")
                ->args({"context","at"})->setTempStringResult();
        addExternInline<DAS_BIND_FUN(builtin_shared_module_extension)>(*this, lib, "shared_module_extension",
            SideEffects::none,"builtin_shared_module_extension")
                ->args({"context","at"})->setTempStringResult();
        addExternInline<DAS_BIND_FUN(builtin_get_das_version)>(*this, lib, "get_das_version",
            SideEffects::none,"builtin_get_das_version")
                ->args({"context","at"})->setTempStringResult();
        addExternInline<DAS_BIND_FUN(getCommandLineArguments)>(*this, lib, "builtin_get_command_line_arguments",
            SideEffects::accessExternal,"getCommandLineArguments")
                ->arg("arguments");
        addExternInline<DAS_BIND_FUN(is_standalone_exe)>(*this, lib, "is_standalone_exe",
            SideEffects::accessExternal, "is_standalone_exe");
        addExternInline<DAS_BIND_FUN(is_safe_hash)>(*this, lib, "is_safe_hash",
            SideEffects::none, "is_safe_hash");
        addExternInline<DAS_BIND_FUN(withCommandLineArguments)>(*this, lib,  "with_argv",
            SideEffects::invoke, "withArgv")
                ->args({"new_arguments", "block","context","line"});
        // compile-time functions
        addExternInline<DAS_BIND_FUN(is_compiling)>(*this, lib, "is_compiling",
            SideEffects::accessExternal, "is_compiling");
        addExternInline<DAS_BIND_FUN(is_compiling_macros)>(*this, lib, "is_compiling_macros",
            SideEffects::accessExternal, "is_compiling_macros");
        addExternInline<DAS_BIND_FUN(is_compiling_macros_in_module)>(*this, lib, "is_compiling_macros_in_module",
            SideEffects::accessExternal, "is_compiling_macros_in_module")
                ->args({"name"});
        addExternInline<DAS_BIND_FUN(is_reporting_compilation_errors)>(*this, lib, "is_reporting_compilation_errors",
            SideEffects::accessExternal, "is_reporting_compilation_errors");
        addExternInline<DAS_BIND_FUN(get_context_share_counter)>(*this, lib, "get_context_share_counter",
            SideEffects::accessExternal, "get_context_share_counter")
                ->arg("context");
        // iterator functions
        addExternInline<DAS_BIND_FUN(builtin_iterator_first)>(*this, lib, "_builtin_iterator_first",
            SideEffects::modifyArgumentAndExternal, "builtin_iterator_first")
                ->args({"iterator","data","context","at"});
        addExternInline<DAS_BIND_FUN(builtin_iterator_next)>(*this, lib,  "_builtin_iterator_next",
            SideEffects::modifyArgumentAndExternal, "builtin_iterator_next")
                ->args({"iterator","data","context","at"});
        addExternInline<DAS_BIND_FUN(builtin_iterator_close)>(*this, lib, "_builtin_iterator_close",
            SideEffects::modifyArgumentAndExternal, "builtin_iterator_close")
                ->args({"iterator","data","context"});
        addExternInline<DAS_BIND_FUN(builtin_iterator_delete)>(*this, lib, "_builtin_iterator_delete",
            SideEffects::modifyArgumentAndExternal, "builtin_iterator_delete")
                ->args({"iterator","context"});
        addExternInline<DAS_BIND_FUN(builtin_iterator_iterate)>(*this, lib, "_builtin_iterator_iterate",
            SideEffects::modifyArgumentAndExternal, "builtin_iterator_iterate")
                ->args({"iterator","data","context"});
        addExternInline<DAS_BIND_FUN(builtin_iterator_empty)>(*this, lib, "empty",
            SideEffects::modifyArgumentAndExternal, "builtin_iterator_empty")
                ->arg("iterator");
        // count and ucount iterators
        auto fnCount = addExtern<DAS_BIND_FUN(builtin_count),SimNode_ExtFuncCallAndCopyOrMove>(*this, lib, "count",
            SideEffects::modifyExternal, "builtin_count")
                ->args({"start","step","context","at"})->setNoDiscard();
        fnCount->arguments[0]->init = new ExprConstInt(0);  // start=0
        fnCount->arguments[1]->init = new ExprConstInt(1);  // step=0
        auto fnuCount = addExtern<DAS_BIND_FUN(builtin_ucount),SimNode_ExtFuncCallAndCopyOrMove>(*this, lib, "ucount",
            SideEffects::none, "builtin_ucount")
                ->args({"start","step","context","at"})->setNoDiscard();
        fnuCount->arguments[0]->init = new ExprConstUInt(0);  // start=0
        fnuCount->arguments[1]->init = new ExprConstUInt(1);  // step=0
        // make-iterator functions
        addExternInline<DAS_BIND_FUN(builtin_make_good_array_iterator)>(*this, lib,  "_builtin_make_good_array_iterator",
            SideEffects::modifyArgumentAndExternal, "builtin_make_good_array_iterator")
                ->args({"iterator","array","stride","context","at"});
        addExternInline<DAS_BIND_FUN(builtin_make_fixed_array_iterator)>(*this, lib,  "_builtin_make_fixed_array_iterator",
            SideEffects::modifyArgumentAndExternal, "builtin_make_fixed_array_iterator")
                ->args({"iterator","data","size","stride","context","at"});
        addExternInline<DAS_BIND_FUN(builtin_make_range_iterator)>(*this, lib,  "_builtin_make_range_iterator",
            SideEffects::modifyArgumentAndExternal, "builtin_make_range_iterator")
                ->args({"iterator","range","context","at"});
        addExternInline<DAS_BIND_FUN(builtin_make_urange_iterator)>(*this, lib,  "_builtin_make_urange_iterator",
            SideEffects::modifyArgumentAndExternal, "builtin_make_urange_iterator")
                ->args({"iterator","range","context","at"});
        addExternInline<DAS_BIND_FUN(builtin_make_range64_iterator)>(*this, lib,  "_builtin_make_range64_iterator",
            SideEffects::modifyArgumentAndExternal, "builtin_make_range64_iterator")
                ->args({"iterator","range","context","at"});
        addExternInline<DAS_BIND_FUN(builtin_make_urange64_iterator)>(*this, lib,  "_builtin_make_urange64_iterator",
            SideEffects::modifyArgumentAndExternal, "builtin_make_urange64_iterator")
                ->args({"iterator","range","context","at"});
        addExternInline<DAS_BIND_FUN(builtin_make_string_iterator)>(*this, lib,  "_builtin_make_string_iterator",
            SideEffects::modifyArgumentAndExternal, "builtin_make_string_iterator")
                ->args({"iterator","string","context","at"})->setCaptureString();
        addExternInline<DAS_BIND_FUN(builtin_make_nil_iterator)>(*this, lib,  "_builtin_make_nil_iterator",
            SideEffects::modifyArgumentAndExternal, "builtin_make_nil_iterator")
                ->args({"iterator","context", "at"});
        addExternInline<DAS_BIND_FUN(builtin_make_lambda_iterator)>(*this, lib,  "_builtin_make_lambda_iterator",
            SideEffects::modifyArgumentAndExternal, "builtin_make_lambda_iterator")
                ->args({"iterator","lambda","stride","context","at"});
        addInterop<builtin_make_enum_iterator,void,vec4f>(*this, lib, "_builtin_make_enum_iterator",
            SideEffects::modifyArgumentAndExternal, "builtin_make_enum_iterator")
                ->arg("iterator");
        // functions
        addExternInline<DAS_BIND_FUN(builtin_throw)>(*this, lib, "panic",
            SideEffects::modifyExternal, "builtin_throw")
                ->args({"text","context","at"});
        addExternInline<DAS_BIND_FUN(builtin_print)>(*this, lib, "print",
            SideEffects::modifyExternal, "builtin_print")
                ->args({"text","context","at"});
        addExternInline<DAS_BIND_FUN(builtin_feint)>(*this, lib, "feint",
            SideEffects::modifyExternal, "builtin_feint")
                ->args({"text","context","at"});
        addExternInline<DAS_BIND_FUN(builtin_error)>(*this, lib, "error",
            SideEffects::modifyExternal, "builtin_error")
                ->args({"text","context","at"});
        addInterop<builtin_sprint,char *,vec4f,PrintFlags>(*this, lib, "sprint",
            SideEffects::modifyExternal, "builtin_sprint")
                ->args({"value","flags"})->setTempStringResult();
        addInterop<builtin_json_sprint,char *,vec4f,bool>(*this, lib, "sprint_json",
            SideEffects::modifyExternal, "builtin_json_sprint")
                ->args({"value","humanReadable"})->setTempStringResult();
        addInterop<builtin_json_sscan,bool,char *,vec4f>(*this, lib, "sscan_json",
            SideEffects::modifyArgumentAndExternal, "builtin_json_sscan")
                ->args({"json","value"});
        addExternInline<DAS_BIND_FUN(builtin_terminate)>(*this, lib, "terminate",
            SideEffects::modifyExternal, "terminate")
                ->args({"context","at"});
        addInterop<builtin_breakpoint,void>(*this, lib, "breakpoint", SideEffects::modifyExternal, "breakpoint");
        // stackwalk
        auto fnsw = addExternInline<DAS_BIND_FUN(builtin_stackwalk)>(*this, lib, "stackwalk",
            SideEffects::modifyExternal, "builtin_stackwalk")
                ->args({"args","vars","context","lineinfo"});
        fnsw->arguments[0]->init = new ExprConstBool(true);
        fnsw->arguments[1]->init = new ExprConstBool(true);
        auto fngsw = addExternInline<DAS_BIND_FUN(builtin_get_stackwalk)>(*this, lib, "get_stackwalk",
            SideEffects::accessExternal, "builtin_get_stackwalk")
                ->args({"args","vars","out_of_scope","top_only","context","lineinfo"})->setTempStringResult();
        fngsw->arguments[0]->init = new ExprConstBool(true);
        fngsw->arguments[1]->init = new ExprConstBool(true);
        fngsw->arguments[2]->init = new ExprConstBool(false);
        fngsw->arguments[3]->init = new ExprConstBool(false);
        // profiler
        addExternInline<DAS_BIND_FUN(resetProfiler)>(*this, lib, "reset_profiler",
            SideEffects::modifyExternal, "resetProfiler")
                ->arg("context");
        addExternInline<DAS_BIND_FUN(dumpProfileInfo)>(*this, lib, "dump_profile_info",
            SideEffects::modifyExternal, "dumpProfileInfo")
                ->arg("context");
        addExternInline<DAS_BIND_FUN(collectProfileInfo)>(*this, lib, "collect_profile_info",
            SideEffects::modifyExternal, "collectProfileInfo")
                ->args({"context","at"})->setTempStringResult();
        // variant
        addExternInline<DAS_BIND_FUN(variant_index)>(*this, lib, "variant_index", SideEffects::none, "variant_index");
        addExternInline<DAS_BIND_FUN(set_variant_index)>(*this, lib, "set_variant_index",
            SideEffects::modifyArgument, "set_variant_index")
                ->args({"variant","index"})->unsafeOperation = true;
        // gc_node root introspection
        addExternInline<DAS_BIND_FUN(gc_thread_root_count)>(*this, lib, "gc_thread_root_count",
            SideEffects::accessExternal, "gc_thread_root_count");
        addExternInline<DAS_BIND_FUN(gc_active_root_count)>(*this, lib, "gc_active_root_count",
            SideEffects::accessExternal, "gc_active_root_count");
        addExternInline<DAS_BIND_FUN(gc_thread_root_report)>(*this, lib, "gc_thread_root_report",
            SideEffects::modifyExternal, "gc_thread_root_report");
        auto gcrd = addExternInline<DAS_BIND_FUN(gc_thread_root_report_detailed)>(*this, lib, "gc_thread_root_report_detailed",
            SideEffects::modifyExternal, "gc_thread_root_report_detailed")
                ->arg("max_nodes");
        gcrd->arguments[0]->init = new ExprConstUInt64(20);
        addExternInline<DAS_BIND_FUN(gc_total_id)>(*this, lib, "gc_total_id",
            SideEffects::accessExternal, "gc_total_id");
        // heap
        addExternInline<DAS_BIND_FUN(heap_allocation_stats)>(*this, lib, "heap_allocation_stats",
            SideEffects::modifyExternal, "heap_allocation_stats")
                ->arg("context");
        addExternInline<DAS_BIND_FUN(heap_allocation_count)>(*this, lib, "heap_allocation_count",
            SideEffects::modifyExternal, "heap_allocation_count")
                ->arg("context");
        addExternInline<DAS_BIND_FUN(string_heap_allocation_stats)>(*this, lib, "string_heap_allocation_stats",
            SideEffects::modifyExternal, "string_heap_allocation_stats")
                ->arg("context");
        addExternInline<DAS_BIND_FUN(string_heap_allocation_count)>(*this, lib, "string_heap_allocation_count",
            SideEffects::modifyExternal, "string_heap_allocation_count")
                ->arg("context");
        addExternInline<DAS_BIND_FUN(heap_bytes_allocated)>(*this, lib, "heap_bytes_allocated",
            SideEffects::modifyExternal, "heap_bytes_allocated")
                ->arg("context");
        addExternInline<DAS_BIND_FUN(heap_total_allocated)>(*this, lib, "heap_total_allocated",
            SideEffects::modifyExternal, "heap_total_allocated")
                ->arg("context");
        addExternInline<DAS_BIND_FUN(max_unreserved_size)>(*this, lib, "max_unreserved_size",
            SideEffects::modifyExternal, "max_unreserved_size")
                ->arg("context");
        addExternInline<DAS_BIND_FUN(set_max_unreserved_size)>(*this, lib, "set_max_unreserved_size",
            SideEffects::modifyExternal, "set_max_unreserved_size")
                ->args({"bytes","context"});
        addExternInline<DAS_BIND_FUN(heap_depth)>(*this, lib, "heap_depth",
            SideEffects::modifyExternal, "heap_depth")
                ->arg("context");
        addExternInline<DAS_BIND_FUN(string_heap_bytes_allocated)>(*this, lib, "string_heap_bytes_allocated",
            SideEffects::modifyExternal, "string_heap_bytes_allocated")
                ->arg("context");
        addExternInline<DAS_BIND_FUN(string_heap_total_allocated)>(*this, lib, "string_heap_total_allocated",
            SideEffects::modifyExternal, "string_heap_total_allocated")
                ->arg("context");
        addExternInline<DAS_BIND_FUN(string_heap_depth)>(*this, lib, "string_heap_depth",
            SideEffects::modifyExternal, "string_heap_depth")
                ->arg("context");
        addExternInline<DAS_BIND_FUN(gc_collection_stats)>(*this, lib, "gc_collection_stats", SideEffects::accessExternal, "gc_collection_stats")->arg("context");
        addExternInline<DAS_BIND_FUN(native_thread_stack_size)>(*this, lib, "native_thread_stack_size", SideEffects::accessExternal, "native_thread_stack_size");
        addExternInline<DAS_BIND_FUN(native_allocator_stats)>(*this, lib, "native_allocator_stats", SideEffects::accessExternal, "native_allocator_stats");
        addExternInline<DAS_BIND_FUN(native_allocator_extent)>(*this, lib, "native_allocator_extent", SideEffects::accessExternal, "native_allocator_extent");
        addExternInline<DAS_BIND_FUN(context_idle_fork_memory)>(*this, lib, "context_idle_fork_memory", SideEffects::accessExternal, "context_idle_fork_memory")->args({"kind", "context"});
        addExternInline<DAS_BIND_FUN(context_memory_sizes)>(*this, lib, "context_memory_sizes", SideEffects::accessExternal, "context_memory_sizes")->arg("context");
        addExternInline<DAS_BIND_FUN(gc_pause_stats)>(*this, lib, "gc_pause_stats", SideEffects::accessExternal, "gc_pause_stats")->arg("context");
        addExternInline<DAS_BIND_FUN(gc_reclaimed_stats)>(*this, lib, "gc_reclaimed_stats", SideEffects::accessExternal, "gc_reclaimed_stats")->arg("context");
        addExternInline<DAS_BIND_FUN(gc_last_collection_tick)>(*this, lib, "gc_last_collection_tick", SideEffects::accessExternal, "gc_last_collection_tick")->arg("context");
        addExternInline<DAS_BIND_FUN(gc_allocation_budget)>(*this, lib, "gc_allocation_budget", SideEffects::accessExternal, "gc_allocation_budget")->arg("context");
        addExternInline<DAS_BIND_FUN(heap_operation_counts)>(*this, lib, "heap_operation_counts", SideEffects::accessExternal, "heap_operation_counts")->args({"strings", "context"});
        addExternInline<DAS_BIND_FUN(set_gc_allocation_budget)>(*this, lib, "set_gc_allocation_budget", SideEffects::modifyExternal, "set_gc_allocation_budget")->args({"heap_bytes", "string_bytes", "context", "at"});
        auto conditionalCollect = addExternInline<DAS_BIND_FUN(heap_collect_if_needed)>(*this, lib, "heap_collect_if_needed", SideEffects::modifyExternal, "heap_collect_if_needed");
        conditionalCollect->args({"context", "at"});
        conditionalCollect->unsafeOperation = true;
        conditionalCollect->needCallerStackFrame = true;
        auto hcol = addExternInline<DAS_BIND_FUN(heap_collect)>(*this, lib, "heap_collect",
                SideEffects::modifyExternal, "heap_collect")
                    ->args({"string_heap","validate","context","at"});
        hcol->unsafeOperation = true;
        // the collector walks caller frames and a fastcall caller has none - this is the seed
        hcol->needCallerStackFrame = true;
        hcol->arguments[0]->init = new ExprConstBool(true);
        hcol->arguments[1]->init = new ExprConstBool(false);
        addExternInline<DAS_BIND_FUN(builtin_frame_position)>(*this, lib, "frame_position",
            SideEffects::accessExternal, "builtin_frame_position")
                ->args({"context","at"});
        addExternInline<DAS_BIND_FUN(string_heap_report)>(*this, lib, "string_heap_report",
            SideEffects::modifyExternal, "string_heap_report")
                ->args({"context","line"});
        addExternInline<DAS_BIND_FUN(heap_report)>(*this, lib, "heap_report",
            SideEffects::modifyExternal, "heap_report")
                ->args({"context","line"});
        addExternInline<DAS_BIND_FUN(memory_report)>(*this, lib, "memory_report",
            SideEffects::modifyExternal, "memory_report")
                ->args({"errorsOnly","context","lineinfo"});
        addExternInline<DAS_BIND_FUN(is_intern_strings)>(*this, lib, "is_intern_strings",
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
        addExternInline<DAS_BIND_FUN(hash64z)>(*this, lib, "hash", SideEffects::none, "hash64z")->arg("data");
        addExternInline<DAS_BIND_FUN(_builtin_hash_int8)>(*this, lib, "hash", SideEffects::none, "_builtin_hash_int8")->arg("value");
        addExternInline<DAS_BIND_FUN(_builtin_hash_uint8)>(*this, lib, "hash", SideEffects::none, "_builtin_hash_uint8")->arg("value");
        addExternInline<DAS_BIND_FUN(_builtin_hash_int16)>(*this, lib, "hash", SideEffects::none, "_builtin_hash_int16")->arg("value");
        addExternInline<DAS_BIND_FUN(_builtin_hash_uint16)>(*this, lib, "hash", SideEffects::none, "_builtin_hash_uint16")->arg("value");
        addExternInline<DAS_BIND_FUN(_builtin_hash_int32)>(*this, lib, "hash", SideEffects::none, "_builtin_hash_int32")->arg("value");
        addExternInline<DAS_BIND_FUN(_builtin_hash_uint32)>(*this, lib, "hash", SideEffects::none, "_builtin_hash_uint32")->arg("value");
        addExternInline<DAS_BIND_FUN(_builtin_hash_int64)>(*this, lib, "hash", SideEffects::none, "_builtin_hash_int64")->arg("value");
        addExternInline<DAS_BIND_FUN(_builtin_hash_uint64)>(*this, lib, "hash", SideEffects::none, "_builtin_hash_uint64")->arg("value");
        addExternInline<DAS_BIND_FUN(_builtin_hash_ptr)>(*this, lib, "hash", SideEffects::none, "_builtin_hash_ptr")->arg("value");
        addExternInline<DAS_BIND_FUN(_builtin_hash_float)>(*this, lib, "hash", SideEffects::none, "_builtin_hash_float")->arg("value");
        addExternInline<DAS_BIND_FUN(_builtin_hash_double)>(*this, lib, "hash", SideEffects::none, "_builtin_hash_double")->arg("value");
        addExternInline<DAS_BIND_FUN(_builtin_hash_das_string)>(*this, lib, "hash", SideEffects::none, "_builtin_hash_das_string")->arg("value");
        addExternInline<DAS_BIND_FUN(hash_combine64)>(*this, lib, "hash_combine64", SideEffects::none, "hash_combine64")
            ->args({"hash","value"});
        // table functions
        addExternInline<DAS_BIND_FUN(builtin_table_clear)>(*this, lib, "_builtin_table_clear",
            SideEffects::modifyArgument, "builtin_table_clear")
                ->args({"table","context","at"});
        addExternInline<DAS_BIND_FUN(builtin_table_size)>(*this, lib, "length",
            SideEffects::none, "builtin_table_size")
                ->arg("table");
        addExternInline<DAS_BIND_FUN(builtin_table_empty)>(*this, lib, "empty",
            SideEffects::none, "builtin_table_empty")
                ->arg("table");
        addExternInline<DAS_BIND_FUN(builtin_table_capacity)>(*this, lib, "capacity",
            SideEffects::none, "builtin_table_capacity")
                ->arg("table");
        addExternInline<DAS_BIND_FUN(builtin_table_long_size)>(*this, lib, "long_length",
            SideEffects::none, "builtin_table_long_size")
                ->arg("table");
        addExternInline<DAS_BIND_FUN(builtin_table_long_capacity)>(*this, lib, "long_capacity",
            SideEffects::none, "builtin_table_long_capacity")
                ->arg("table");
        addExternInline<DAS_BIND_FUN(builtin_table_lock)>(*this, lib, "__builtin_table_lock",
            SideEffects::modifyArgumentAndExternal, "builtin_table_lock")
                ->args({"table","context","at"});
        addExternInline<DAS_BIND_FUN(builtin_table_lock_mutable)>(*this, lib, "__builtin_table_lock_mutable",
            SideEffects::modifyArgumentAndExternal, "builtin_table_lock_mutable")
                ->args({"table","context","at"});
        addExternInline<DAS_BIND_FUN(builtin_table_unlock)>(*this, lib, "__builtin_table_unlock",
            SideEffects::modifyArgumentAndExternal, "builtin_table_unlock")
                ->args({"table","context","at"});
        addExternInline<DAS_BIND_FUN(builtin_table_unlock_mutable)>(*this, lib, "__builtin_table_unlock_mutable",
            SideEffects::modifyArgumentAndExternal, "builtin_table_unlock_mutable")
                ->args({"table","context","at"});
        addExternInline<DAS_BIND_FUN(builtin_table_clear_lock)>(*this, lib, "__builtin_table_clear_lock",
            SideEffects::modifyArgumentAndExternal, "builtin_table_clear_lock")
                ->args({"table","context"});
        addExternInline<DAS_BIND_FUN(builtin_table_tag)>(*this, lib, "tag_table",
            SideEffects::modifyExternal, "builtin_table_tag")
                ->args({"table","name","context"});
        // scratch: same contract as the array flag; the bit lives in the shared Array flags word
        addExternInline<DAS_BIND_FUN(builtin_table_set_scratch)>(*this, lib, "set_scratch",
            SideEffects::modifyArgument, "builtin_table_set_scratch")
                ->args({"table","value","context"})->unsafeOperation = true;
        addExternInline<DAS_BIND_FUN(builtin_table_is_scratch)>(*this, lib, "is_scratch",
            SideEffects::none, "builtin_table_is_scratch")
                ->arg("table");
        addExternInline<DAS_BIND_FUN(builtin_table_keys)>(*this, lib, "__builtin_table_keys",
            SideEffects::modifyArgumentAndExternal, "builtin_table_keys")
                ->args({"iterator","table","stride","context","at"});
        addExternInline<DAS_BIND_FUN(builtin_table_values)>(*this, lib, "__builtin_table_values",
            SideEffects::modifyArgumentAndExternal, "builtin_table_values")
                ->args({"iterator","table","stride","context","at"});
        addExternInline<DAS_BIND_FUN(builtin_table_get_key)>(*this, lib, "__builtin_table_get_key",
            SideEffects::modifyExternal, "builtin_table_get_key")
                ->args({"result","table","value_ptr","value_stride","key_stride","context","at"});
        addInterop<builtin_table_reserve,void,vec4f,uint64_t>(*this, lib, "__builtin_table_reserve",
            SideEffects::modifyArgumentAndExternal, "builtin_table_reserve")
                ->args({"table","size"});
        // array and table free
        addExternInline<DAS_BIND_FUN(builtin_array_free)>(*this, lib, "__builtin_array_free",
            SideEffects::modifyArgumentAndExternal, "builtin_array_free")
                ->args({"array","stride","context","at"});
        addExternInline<DAS_BIND_FUN(builtin_table_free)>(*this, lib, "__builtin_table_free",
            SideEffects::modifyArgumentAndExternal, "builtin_table_free")
                ->args({"table","sizeOfKey","sizeOfValue","context","at"});
        // local collection
        addInterop<builtin_collect_local_and_zero,void,vec4f,uint32_t>(*this, lib, "builtin_collect_local_and_zero",
            SideEffects::modifyArgumentAndExternal, "builtin_collect_local_and_zero")
                ->args({"anything","sizeOfAnything"})->unsafeOperation = true;
        addInterop<builtin_collect_local,void,vec4f,uint32_t>(*this, lib, "builtin_collect_local",
            SideEffects::modifyArgumentAndExternal, "builtin_collect_local")
                ->args({"anything","sizeOfAnything"})->unsafeOperation = true;
        addInterop<builtin_scope_free,void,vec4f,uint32_t>(*this, lib, "builtin_scope_free",
            SideEffects::modifyArgumentAndExternal, "builtin_scope_free")
                ->args({"pointer","sizeOfPointee"})->unsafeOperation = true;
        // table expressions
        addCall<ExprErase>("__builtin_table_erase");
        addCall<ExprSetInsert>("__builtin_table_set_insert");
        addCall<ExprFind>("__builtin_table_find");
        addCall<ExprKeyExists>("__builtin_table_key_exists");
        // blocks
        addCall<ExprInvoke>("invoke");
        // smart ptr stuff
        addExternInline<DAS_BIND_FUN(builtin_smart_ptr_move_new)>(*this, lib, "move_new",
            SideEffects::modifyArgument, "builtin_smart_ptr_move_new")
                ->args({"dest","src","context","at"});
        addExternInline<DAS_BIND_FUN(builtin_smart_ptr_move_ptr)>(*this, lib, "move",
            SideEffects::modifyArgument, "builtin_smart_ptr_move_ptr")
                ->args({"dest","src","context","at"});
        addExternInline<DAS_BIND_FUN(builtin_smart_ptr_move)>(*this, lib, "move",
            SideEffects::modifyArgument, "builtin_smart_ptr_move")
                ->args({"dest","src","context","at"});
        addExternInline<DAS_BIND_FUN(builtin_smart_ptr_clone_ptr)>(*this, lib, "smart_ptr_clone",
            SideEffects::modifyArgument, "builtin_smart_ptr_clone_ptr")
                ->args({"dest","src","context","at"});
        addExternInline<DAS_BIND_FUN(builtin_smart_ptr_clone)>(*this, lib, "smart_ptr_clone",
            SideEffects::modifyArgument, "builtin_smart_ptr_clone")
                ->args({"dest","src","context","at"});
        addExternInline<DAS_BIND_FUN(builtin_smart_ptr_use_count)>(*this, lib, "smart_ptr_use_count",
            SideEffects::none, "builtin_smart_ptr_use_count")
                ->args({"ptr","context","at"});
        addExternInline<DAS_BIND_FUN(smart_ptr_is_valid)>(*this, lib, "smart_ptr_is_valid",
            SideEffects::none, "smart_ptr_is_valid")
                ->arg("dest");
        addExternInline<DAS_BIND_FUN(equ_sptr_sptr)>(*this, lib, "==", SideEffects::none, "equ_sptr_sptr");
        addExternInline<DAS_BIND_FUN(nequ_sptr_sptr)>(*this, lib, "!=", SideEffects::none, "nequ_sptr_sptr");
        addExternInline<DAS_BIND_FUN(equ_ptr_sptr)>(*this, lib, "==", SideEffects::none, "equ_ptr_sptr");
        addExternInline<DAS_BIND_FUN(nequ_ptr_sptr)>(*this, lib, "!=", SideEffects::none, "nequ_ptr_sptr");
        addExternInline<DAS_BIND_FUN(equ_sptr_ptr)>(*this, lib, "==", SideEffects::none, "equ_sptr_ptr");
        addExternInline<DAS_BIND_FUN(nequ_sptr_ptr)>(*this, lib, "!=", SideEffects::none, "nequ_sptr_ptr");
        // gc0
        addExternInline<DAS_BIND_FUN(gc0_save_ptr)>(*this, lib, "gc0_save_ptr",
            SideEffects::modifyExternal, "gc0_save_ptr")
                ->args({"name","data","context","line"});
        addExternInline<DAS_BIND_FUN(gc0_save_smart_ptr)>(*this, lib, "gc0_save_smart_ptr",
            SideEffects::modifyExternal, "gc0_save_smart_ptr")
                ->args({"name","data","context","line"});
        addExternInline<DAS_BIND_FUN(gc0_restore_ptr)>(*this, lib, "gc0_restore_ptr",
            SideEffects::accessExternal, "gc0_restore_ptr")
                ->args({"name","context"});
        addExternInline<DAS_BIND_FUN(gc0_restore_smart_ptr)>(*this, lib, "gc0_restore_smart_ptr",
            SideEffects::accessExternal, "gc0_restore_smart_ptr")
                ->args({"name","context"});
        addExternInline<DAS_BIND_FUN(gc0_reset)>(*this, lib, "gc0_reset",
            SideEffects::modifyExternal, "gc0_reset");
        // memops — explicit function-pointer types pick the non-const overloads (aot.h grew
        // const-source twins for the emitted das_cast<void const *> calls; the das-side extern
        // signatures must stay unchanged so no AOT hash churns)
        addExternInline<void (*)(void *, void *, int), das_memcpy>(*this, lib, "memcpy",
            SideEffects::modifyArgumentAndExternal, "das_memcpy")
                ->args({"left","right","size"})->unsafeOperation = true;
        addExternInline<int (*)(void *, void *, int), das_memcmp>(*this, lib, "memcmp",
            SideEffects::none, "das_memcmp")
                ->args({"left","right","size"})->unsafeOperation = true;
        // unsigned and 64-bit size overloads. Without these a size that is already
        // uint/int64/uint64 can only reach memcpy through int(...), which truncates
        // above 2GB -- silently copying the wrong number of bytes.
        addExternInline<void (*)(void *, void *, uint32_t), das_memcpy>(*this, lib, "memcpy",
            SideEffects::modifyArgumentAndExternal, "das_memcpy")
                ->args({"left","right","size"})->unsafeOperation = true;
        addExternInline<void (*)(void *, void *, int64_t), das_memcpy>(*this, lib, "memcpy",
            SideEffects::modifyArgumentAndExternal, "das_memcpy")
                ->args({"left","right","size"})->unsafeOperation = true;
        addExternInline<void (*)(void *, void *, uint64_t), das_memcpy>(*this, lib, "memcpy",
            SideEffects::modifyArgumentAndExternal, "das_memcpy")
                ->args({"left","right","size"})->unsafeOperation = true;
        addExternInline<int (*)(void *, void *, uint32_t), das_memcmp>(*this, lib, "memcmp",
            SideEffects::none, "das_memcmp")
                ->args({"left","right","size"})->unsafeOperation = true;
        addExternInline<int (*)(void *, void *, int64_t), das_memcmp>(*this, lib, "memcmp",
            SideEffects::none, "das_memcmp")
                ->args({"left","right","size"})->unsafeOperation = true;
        addExternInline<int (*)(void *, void *, uint64_t), das_memcmp>(*this, lib, "memcmp",
            SideEffects::none, "das_memcmp")
                ->args({"left","right","size"})->unsafeOperation = true;
        addExternInline<void (*)(void *, void *, int), das_memmove>(*this, lib, "memmove",
            SideEffects::modifyArgumentAndExternal, "das_memmove")
                ->args({"left","right","size"})->unsafeOperation = true;
        addExternInline<void (*)(void *, void *, uint32_t), das_memmove>(*this, lib, "memmove",
            SideEffects::modifyArgumentAndExternal, "das_memmove")
                ->args({"left","right","size"})->unsafeOperation = true;
        addExternInline<void (*)(void *, void *, int64_t), das_memmove>(*this, lib, "memmove",
            SideEffects::modifyArgumentAndExternal, "das_memmove")
                ->args({"left","right","size"})->unsafeOperation = true;
        addExternInline<void (*)(void *, void *, uint64_t), das_memmove>(*this, lib, "memmove",
            SideEffects::modifyArgumentAndExternal, "das_memmove")
                ->args({"left","right","size"})->unsafeOperation = true;
        addExternInline<DAS_BIND_FUN(das_memset8)>(*this, lib, "memset8",
            SideEffects::modifyArgumentAndExternal, "das_memset8")
                ->args({"left","value","count"})->unsafeOperation = true;
        addExternInline<DAS_BIND_FUN(das_memset16)>(*this, lib, "memset16",
            SideEffects::modifyArgumentAndExternal, "das_memset16")
                ->args({"left","value","count"})->unsafeOperation = true;
        addExternInline<DAS_BIND_FUN(das_memset32)>(*this, lib, "memset32",
            SideEffects::modifyArgumentAndExternal, "das_memset32")
                ->args({"left","value","count"})->unsafeOperation = true;
        addExternInline<DAS_BIND_FUN(das_memset64)>(*this, lib, "memset64",
            SideEffects::modifyArgumentAndExternal, "das_memset64")
                ->args({"left","value","count"})->unsafeOperation = true;
        addExternInlineEx<void(*)(void *,uint4,int32_t),DAS_BIND_FUN(das_memset128u)>(*this, lib, "memset128",
            SideEffects::modifyArgumentAndExternal, "das_memset128u")
                ->args({"left","value","count"})->unsafeOperation = true;
        addExternInline<DAS_BIND_FUN(builtin_das_aligned_alloc16)> (*this, lib, "malloc",
            SideEffects::modifyExternal, "builtin_das_aligned_alloc16")
                ->args({"size"})->unsafeOperation = true;
        addExternInline<DAS_BIND_FUN(das_aligned_free16)> (*this, lib, "free",
            SideEffects::modifyExternal, "das_aligned_free16")
                ->args({"ptr"})->unsafeOperation = true;
        addExternInline<DAS_BIND_FUN(das_aligned_memsize)> (*this, lib, "malloc_usable_size",
            SideEffects::none, "das_aligned_memsize")
                ->args({"ptr"})->unsafeOperation = true;
        // pointer ari
        PointerOp(addExternInline<DAS_BIND_FUN(i_das_ptr_inc)>(*this, lib, "i_das_ptr_inc", SideEffects::modifyArgument, "das_ptr_inc"));
        PointerOp(addExternInline<DAS_BIND_FUN(i_das_ptr_dec)>(*this, lib, "i_das_ptr_dec", SideEffects::modifyArgument, "das_ptr_dec"));
        // int32
        PointerOp(addExternInline<DAS_BIND_FUN(i_das_ptr_add_int32)>(*this, lib, "i_das_ptr_add", SideEffects::none, "das_ptr_add_int32"));
        PointerOp(addExternInline<DAS_BIND_FUN(i_das_ptr_sub_int32)>(*this, lib, "i_das_ptr_sub", SideEffects::none, "das_ptr_sub_int32"));
        PointerOp(addExternInline<DAS_BIND_FUN(i_das_ptr_set_add_int32)>(*this, lib, "i_das_ptr_set_add", SideEffects::modifyArgument, "das_ptr_set_add_int32"));
        PointerOp(addExternInline<DAS_BIND_FUN(i_das_ptr_set_sub_int32)>(*this, lib, "i_das_ptr_set_sub", SideEffects::modifyArgument, "das_ptr_set_sub_int32"));
        // int64
        PointerOp(addExternInline<DAS_BIND_FUN(i_das_ptr_add_int64)>(*this, lib, "i_das_ptr_add", SideEffects::none, "das_ptr_add_int64"));
        PointerOp(addExternInline<DAS_BIND_FUN(i_das_ptr_sub_int64)>(*this, lib, "i_das_ptr_sub", SideEffects::none, "das_ptr_sub_int64"));
        PointerOp(addExternInline<DAS_BIND_FUN(i_das_ptr_set_add_int64)>(*this, lib, "i_das_ptr_set_add", SideEffects::modifyArgument, "das_ptr_set_add_int64"));
        PointerOp(addExternInline<DAS_BIND_FUN(i_das_ptr_set_sub_int64)>(*this, lib, "i_das_ptr_set_sub", SideEffects::modifyArgument, "das_ptr_set_sub_int64"));
        // uint32
        PointerOp(addExternInline<DAS_BIND_FUN(i_das_ptr_add_uint32)>(*this, lib, "i_das_ptr_add", SideEffects::none, "das_ptr_add_uint32"));
        PointerOp(addExternInline<DAS_BIND_FUN(i_das_ptr_sub_uint32)>(*this, lib, "i_das_ptr_sub", SideEffects::none, "das_ptr_sub_uint32"));
        PointerOp(addExternInline<DAS_BIND_FUN(i_das_ptr_set_add_uint32)>(*this, lib, "i_das_ptr_set_add", SideEffects::modifyArgument, "das_ptr_set_add_uint32"));
        PointerOp(addExternInline<DAS_BIND_FUN(i_das_ptr_set_sub_uint32)>(*this, lib, "i_das_ptr_set_sub", SideEffects::modifyArgument, "das_ptr_set_sub_uint32"));
        // uint64
        PointerOp(addExternInline<DAS_BIND_FUN(i_das_ptr_add_uint64)>(*this, lib, "i_das_ptr_add", SideEffects::none, "das_ptr_add_uint64"));
        PointerOp(addExternInline<DAS_BIND_FUN(i_das_ptr_sub_uint64)>(*this, lib, "i_das_ptr_sub", SideEffects::none, "das_ptr_sub_uint64"));
        PointerOp(addExternInline<DAS_BIND_FUN(i_das_ptr_set_add_uint64)>(*this, lib, "i_das_ptr_set_add", SideEffects::modifyArgument, "das_ptr_set_add_uint64"));
        PointerOp(addExternInline<DAS_BIND_FUN(i_das_ptr_set_sub_uint64)>(*this, lib, "i_das_ptr_set_sub", SideEffects::modifyArgument, "das_ptr_set_sub_uint64"));
        // diff
        addExternInline<DAS_BIND_FUN(i_das_ptr_diff)>(*this, lib, "i_das_ptr_diff", SideEffects::none, "i_das_ptr_diff");
        // rtti
        addExternInline<DAS_BIND_FUN(class_rtti_size)>(*this, lib, "class_rtti_size",
            SideEffects::none, "class_rtti_size")
                ->arg("ptr");
        // profile
        addExternInline<DAS_BIND_FUN(builtin_profile)>(*this,lib,"profile",
            SideEffects::modifyExternal, "builtin_profile")
                ->args({"count","category","block","context","line"});
        // das string binding
        addExternInline<DAS_BIND_FUN(to_das_string)>(*this, lib, "string",
            SideEffects::none, "to_das_string")
                ->args({"source","context","at"})->setTempStringResult();
        addExternInline<DAS_BIND_FUN(pass_string)>(*this, lib, "string",
            SideEffects::none, "pass_string", permanentArgFn())
                ->args({"source"})->setCaptureString();
        addExternInline<DAS_BIND_FUN(clone_pass_string)>(*this, lib, "string",
            SideEffects::none, "clone_pass_string", temporaryArgFn())
                ->args({"source","context","at"})->setTempStringResult();
        addExternInline<DAS_BIND_FUN(set_das_string)>(*this, lib, "clone",
            SideEffects::modifyArgument,"set_das_string")
                ->args({"target","src"});
        addExternInline<DAS_BIND_FUN(set_das_string_bytes)>(*this, lib, "clone",
            SideEffects::modifyArgument,"set_das_string_bytes")
                ->args({"target","bytes"});
        addExternInline<DAS_BIND_FUN(set_string_das)>(*this, lib, "clone",
            SideEffects::modifyArgument,"set_string_das")
                ->args({"target","src","context","at"});
        addExternInline<DAS_BIND_FUN(peek_das_string)>(*this, lib, "peek",
            SideEffects::modifyExternal,"peek_das_string_T")
                ->args({"src","block","context","line"})->setAotTemplate();
        addExternInline<DAS_BIND_FUN(builtin_string_clone)>(*this, lib, "clone_string",
            SideEffects::none, "builtin_string_clone")
                ->args({"src","context","at"})->setTempStringResult();
        // das-string
        addExternInline<DAS_BIND_FUN(das_str_equ)>(*this, lib, "==", SideEffects::none, "das_str_equ");
        addExternInline<DAS_BIND_FUN(das_str_nequ)>(*this, lib, "!=", SideEffects::none, "das_str_nequ");
        // string emptiness and length. length lived in the strings module, which made
        // `length(str)` need a require that `empty(str)` did not -- these belong together.
        addExternInline<DAS_BIND_FUN(builtin_empty)>(*this, lib, "empty",
            SideEffects::none, "builtin_empty")->arg("str");
        addExternInline<DAS_BIND_FUN(builtin_empty_das_string)>(*this, lib, "empty",
            SideEffects::none, "builtin_empty_das_string")->arg("str");
        addExternInline<DAS_BIND_FUN(builtin_string_length)>(*this, lib, "length",
            SideEffects::none, "builtin_string_length")->args({"str","context"});
        addExternInline<DAS_BIND_FUN(builtin_ext_string_length)>(*this, lib, "length",
            SideEffects::none, "builtin_ext_string_length")->arg("str");
        addExternInline<DAS_BIND_FUN(builtin_string_long_length)>(*this, lib, "long_length",
            SideEffects::none, "builtin_string_long_length")->arg("str");
        addExternInline<DAS_BIND_FUN(builtin_ext_string_long_length)>(*this, lib, "long_length",
            SideEffects::none, "builtin_ext_string_long_length")->arg("str");
        // das-string extra
        STR_DSTR_REG(  eq,==);
        STR_DSTR_REG( neq,!=);
        STR_DSTR_REG(gteq,>=);
        STR_DSTR_REG(lseq,<=);
        STR_DSTR_REG(  ls,<);
        STR_DSTR_REG(  gt,>);
        // temp array out of mem
        auto bta = addExternInline<DAS_BIND_FUN(builtin_temp_array)>(*this, lib, "_builtin_temp_array",
            SideEffects::invoke, "builtin_temp_array")
                ->args({"data","size","block","context","line"});
        bta->unsafeOperation = true;
        bta->privateFunction = true;
        auto bmta = addExternInline<DAS_BIND_FUN(builtin_make_temp_array)>(*this, lib, "_builtin_make_temp_array",
            SideEffects::modifyArgument, "builtin_make_temp_array")
                ->args({"array","data","size"});
        bmta->unsafeOperation = true;
        auto bmta64 = addExternInline<DAS_BIND_FUN(builtin_make_temp_array_i64)>(*this, lib, "_builtin_make_temp_array_i64",
            SideEffects::modifyArgument, "builtin_make_temp_array_i64")
                ->args({"array","data","size"});
        bmta64->unsafeOperation = true;
        auto bfta = addExternInline<DAS_BIND_FUN(builtin_forget_temp_array)>(*this, lib, "_builtin_forget_temp_array",
            SideEffects::modifyArgument, "builtin_forget_temp_array")
                ->args({"array","context","line"});
        bfta->unsafeOperation = true;
        // migrate data
        addExternInline<DAS_BIND_FUN(das_is_dll_build)>(*this, lib, "das_is_dll_build",
            SideEffects::worstDefault, "das_is_dll_build");
        addExternInline<DAS_BIND_FUN(das_is_exceptions_enabled)>(*this, lib, "das_is_exceptions_enabled",
            SideEffects::worstDefault, "das_is_exceptions_enabled");
        addExternInline<DAS_BIND_FUN(is_in_aot)>(*this, lib, "is_in_aot",
            SideEffects::worstDefault, "is_in_aot");
        addExternInline<DAS_BIND_FUN(set_aot)>(*this, lib, "set_aot",
            SideEffects::worstDefault, "set_aot");
        addExternInline<DAS_BIND_FUN(reset_aot)>(*this, lib, "reset_aot",
            SideEffects::worstDefault, "reset_aot");
        // completion
        addExternInline<DAS_BIND_FUN(is_in_completion)>(*this, lib, "is_in_completion",
            SideEffects::worstDefault, "is_in_completion");
        // lint
        addExternInline<DAS_BIND_FUN(is_in_lint_check)>(*this, lib, "is_in_lint_check",
            SideEffects::worstDefault, "is_in_lint_check");

        addExternInline<DAS_BIND_FUN(is_building_documentation)>(*this, lib, "is_building_documentation",
            SideEffects::worstDefault, "is_building_documentation");
        // folding
        addExternInline<DAS_BIND_FUN(is_folding)>(*this, lib, "is_folding",
            SideEffects::worstDefault, "is_folding");
        // compiling file
        addExternInline<DAS_BIND_FUN(compiling_file_name)>(*this, lib, "compiling_file_name",
            SideEffects::accessExternal, "compiling_file_name");
        addExternInline<DAS_BIND_FUN(compiling_module_name)>(*this, lib, "compiling_module_name",
            SideEffects::accessExternal, "compiling_module_name");
        addExternInline<DAS_BIND_FUN(get_module_file_name)>(*this, lib, "get_module_file_name",
            SideEffects::accessExternal, "get_module_file_name")->args({"name", "context"})->setTempStringResult();
        // logger
        addExternInline<DAS_BIND_FUN(toLog)>(*this, lib, "to_log",
            SideEffects::modifyExternal, "toLog")->args({"level", "text", "context", "at"});
        addExternInline<DAS_BIND_FUN(diagnosticsToStderr)>(*this, lib, "diagnostics_to_stderr",
            SideEffects::modifyExternal, "diagnosticsToStderr");
        addExternInline<DAS_BIND_FUN(diagnosticsToFile)>(*this, lib, "diagnostics_to_file",
            SideEffects::modifyExternal, "diagnosticsToFile")->args({"path"});
        addExternInline<DAS_BIND_FUN(toCompilerLog)>(*this, lib, "to_compiler_log",
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
        addExternInline<DAS_BIND_FUN(uint32_clz)>(*this, lib, "clz", SideEffects::none, "uint32_clz")->arg("bits");
        addExternInline<DAS_BIND_FUN(uint64_clz)>(*this, lib, "clz", SideEffects::none, "uint64_clz")->arg("bits");
        addExternInline<DAS_BIND_FUN(uint32_ctz)>(*this, lib, "ctz", SideEffects::none, "uint32_ctz")->arg("bits");
        addExternInline<DAS_BIND_FUN(uint64_ctz)>(*this, lib, "ctz", SideEffects::none, "uint64_ctz")->arg("bits");
        addExternInline<DAS_BIND_FUN(uint32_popcount)>(*this, lib, "popcnt", SideEffects::none, "uint32_popcount")->arg("bits");
        addExternInline<DAS_BIND_FUN(uint64_popcount)>(*this, lib, "popcnt", SideEffects::none, "uint64_popcount")->arg("bits");
        addExternInline<DAS_BIND_FUN(mul_u64_u64)>(*this, lib, "mul128", SideEffects::none, "mul_u64_u64")->args({"a","b"});
        // string using
        addUsing<das::string>(*this, lib, "das::string");
        // try-recover
        // main-loop
        addExternInline<DAS_BIND_FUN(builtin_main_loop)>(*this, lib, "eval_main_loop",
            SideEffects::invoke, "builtin_main_loop")
                ->args({"block","context","at"});
        // jit
        addExternInline<DAS_BIND_FUN(das_is_jit_function)>(*this, lib, "is_jit_function",
                SideEffects::worstDefault, "das_is_jit_function")
                    ->args({"function"});
        addExternInline<DAS_BIND_FUN(das_is_aot_function)>(*this, lib, "is_aot_function",
                SideEffects::worstDefault, "das_is_aot_function")
                    ->args({"function"});
        addExternInline<DAS_BIND_FUN(das_jit_enabled)>(*this, lib, "jit_enabled",
            SideEffects::none, "das_jit_enabled")
                ->args({"context","at"});
        // aot
        addExternInline<DAS_BIND_FUN(das_aot_enabled)>(*this, lib, "aot_enabled",
            SideEffects::none, "das_aot_enabled")
                ->args({"context","at"});
        // bitfield
        // `__bit_set` has additional raw-integer overloads in aot.h (for handle-bound
        // bitfield fields like Function::flags, which is uint32_t on the C++ side) —
        // disambiguate the runtime binding to the Bitfield& variant explicitly.
        using BitSetFn   = void(*)(Bitfield&,   Bitfield,   bool);
        using BitSet8Fn  = void(*)(Bitfield8&,  Bitfield8,  bool);
        using BitSet16Fn = void(*)(Bitfield16&, Bitfield16, bool);
        using BitSet64Fn = void(*)(Bitfield64&, Bitfield64, bool);
        addExternInline<BitSetFn, static_cast<BitSetFn>(__bit_set)>(*this, lib, "__bit_set",
            SideEffects::modifyArgument, "__bit_set")
                ->args({"value","mask","on"});
        addExternInline<BitSet8Fn, static_cast<BitSet8Fn>(__bit_set8)>(*this, lib, "__bit_set",
            SideEffects::modifyArgument, "__bit_set8")
                ->args({"value","mask","on"});
        addExternInline<BitSet16Fn, static_cast<BitSet16Fn>(__bit_set16)>(*this, lib, "__bit_set",
            SideEffects::modifyArgument, "__bit_set16")
                ->args({"value","mask","on"});
        addExternInline<BitSet64Fn, static_cast<BitSet64Fn>(__bit_set64)>(*this, lib, "__bit_set",
            SideEffects::modifyArgument, "__bit_set64")
                ->args({"value","mask","on"});
        // platform and architecture
        addExternInline<DAS_BIND_FUN(das_get_platform_name)>(*this, lib, "get_platform_name",
            SideEffects::none, "das_get_platform_name");
        // whether THIS binary's float ops were compiled relaxed (-ffast-math / /fp:fast)
#if defined(__FAST_MATH__) || defined(_M_FP_FAST)
        addConstant(*this, "HOST_FAST_MATH", true);
#else
        addConstant(*this, "HOST_FAST_MATH", false);
#endif

        // Same C++ query, but registered with a side effect so the optimizer never const-folds it.
        // get_platform_name() (SideEffects::none) folds at compile time to the HOST platform, which is
        // wrong inside a wasm cross-compile (host is e.g. windows, the code will RUN under emscripten).
        // get_running_platform_name() stays a real call, so in the cross-compiled binary it resolves to
        // the wasm runtime's das_get_platform_name -> "emscripten". Use it for runtime "where am I
        // actually executing" decisions; use get_platform_name() for compile-time / macro decisions.
        addExternInline<DAS_BIND_FUN(das_get_platform_name)>(*this, lib, "get_running_platform_name",
            SideEffects::accessExternal, "das_get_platform_name");
        // SideEffects::none so it FOLDS at compile time -> usable in static_if. The cross-compile
        // target is a compile-time property (the --jit-target triple is fixed for the whole compile),
        // so folding to it is correct: e.g. static_if(get_platform_name()=="emscripten" ||
        // get_cross_platform_name()=="emscripten") picks the WebGL2 draw path and drops the desktop-only
        // glDrawElementsBaseVertex branch before symbol resolution. Folds to "" on a non-cross run, so a
        // desktop compile keeps the BaseVertex branch. (get_running_platform_name stays accessExternal —
        // it must NOT fold; it reports where the code actually executes.)
        addExternInline<DAS_BIND_FUN(das_get_cross_platform_name)>(*this, lib, "get_cross_platform_name",
            SideEffects::none, "das_get_cross_platform_name");
        addExternInline<DAS_BIND_FUN(das_get_architecture_name)>(*this, lib, "get_architecture_name",
            SideEffects::none, "das_get_architecture_name");
        // the two target-side folds (see the definitions): SideEffects::none like the pair above,
        // so a kernel-tier [init] gate and a static_if see the artifact's target, not the host
        addExternInline<DAS_BIND_FUN(das_get_target_triple)>(*this, lib, "get_target_triple",
            SideEffects::none, "das_get_target_triple");
        addExternInline<DAS_BIND_FUN(das_get_target_architecture_name)>(*this, lib, "get_target_architecture_name",
            SideEffects::none, "das_get_target_architecture_name");
        // accessExternal (NOT ::none) on purpose: CPU features are a property of the RUNNING box,
        // so this must never const-fold into AOT artifacts built on a different machine.
        addExternInline<DAS_BIND_FUN(das_cpu_supports)>(*this, lib, "cpu_supports",
            SideEffects::accessExternal, "das_cpu_supports")->args({"feature"});
        // fmt
        addExternInline<DAS_BIND_FUN(fmt_i8)>(*this, lib, "fmt",
            SideEffects::none, "fmt_i8")->args({"format","value","context","at"})->setTempStringResult();
        addExternInline<DAS_BIND_FUN(fmt_u8)>(*this, lib, "fmt",
            SideEffects::none, "fmt_u8")->args({"format","value","context","at"})->setTempStringResult();
        addExternInline<DAS_BIND_FUN(fmt_i16)>(*this, lib, "fmt",
            SideEffects::none, "fmt_i16")->args({"format","value","context","at"})->setTempStringResult();
        addExternInline<DAS_BIND_FUN(fmt_u16)>(*this, lib, "fmt",
            SideEffects::none, "fmt_u16")->args({"format","value","context","at"})->setTempStringResult();
        addExternInline<DAS_BIND_FUN(fmt_i32)>(*this, lib, "fmt",
            SideEffects::none, "fmt_i32")->args({"format","value","context","at"})->setTempStringResult();
        addExternInline<DAS_BIND_FUN(fmt_u32)>(*this, lib, "fmt",
            SideEffects::none, "fmt_u32")->args({"format","value","context","at"})->setTempStringResult();
        addExternInline<DAS_BIND_FUN(fmt_i64)>(*this, lib, "fmt",
            SideEffects::none, "fmt_i64")->args({"format","value","context","at"})->setTempStringResult();
        addExternInline<DAS_BIND_FUN(fmt_u64)>(*this, lib, "fmt",
            SideEffects::none, "fmt_u64")->args({"format","value","context","at"})->setTempStringResult();
        addExternInline<DAS_BIND_FUN(fmt_f)>(*this, lib, "fmt",
            SideEffects::none, "fmt_f")->args({"format","value","context","at"})->setTempStringResult();
        addExternInline<DAS_BIND_FUN(fmt_d)>(*this, lib, "fmt",
            SideEffects::none, "fmt_d")->args({"format","value","context","at"})->setTempStringResult();
    }
}
