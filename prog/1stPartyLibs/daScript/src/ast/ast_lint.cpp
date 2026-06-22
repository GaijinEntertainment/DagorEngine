#include "daScript/misc/platform.h"

#include "daScript/ast/ast.h"
#include "daScript/ast/ast_generate.h"
#include "daScript/ast/ast_expressions.h"
#include "daScript/ast/ast_visitor.h"
#include "daScript/simulate/aot_builtin_ast.h"

namespace das {

    struct hash_vec4f {
        uint64_t operator () ( vec4f v ) const {
            uint64_t a[2];
            memcpy(a,&v,sizeof(vec4f));
            return a[0] ^ a[1];
        }
    };

    struct equalto_vec4f {
        bool operator () ( vec4f a, vec4f b ) const {
            return memcmp(&a,&b,sizeof(vec4f))==0;
        }
    };

    struct hash_ccs {
        uint64_t operator () ( const char * str ) const {
            return hash64z(str);
        }
    };

    struct equalto_ccs {
        bool operator () ( const char * a, const char * b ) const {
            return strcmp(a,b)==0;
        }
    };



    bool hasLabels ( const ExprBlock * block ) {
        for ( auto & be : block->list ) {
            if ( be->rtti_isLabel() ) {
                return true;
            }
        }
        return false;
    }

    bool exprReturns ( ExpressionPtr expr ) {
        if ( expr->rtti_isReturn() ) {
            return true;
        } else if ( expr->rtti_isBlock() ) {
            auto block = static_cast<ExprBlock*>(expr);
            if ( hasLabels(block) ) {
                // NOTE: block with labels assumed to always return for now. real world analysis is hard
                return true;
            } else {
                for ( auto & be : block->list ) {
                    if ( be->rtti_isBreak() || be->rtti_isContinue() || be->rtti_isGoto() ) {
                        break;
                    }
                    if ( exprReturns(be) ) {
                        return true;
                    }
                }
            }
        } else if ( expr->rtti_isIfThenElse() ) {
            auto ite = static_cast<ExprIfThenElse*>(expr);
            if ( ite->if_false ) {
                return exprReturns(ite->if_true) && exprReturns(ite->if_false);
            }
        } else if ( expr->rtti_isWith() ) {
            auto wth = static_cast<ExprWith*>(expr);
            return exprReturns(wth->body);
        } else if ( expr->rtti_isWhile() ) {
            auto wh = static_cast<ExprWhile*>(expr);
            return exprReturns(wh->body);
        } else if ( expr->rtti_isFor() ) {
            auto fr = static_cast<ExprFor*>(expr);
            return exprReturns(fr->body);
        } else if ( expr->rtti_isUnsafe() ) {
            auto us = static_cast<ExprUnsafe*>(expr);
            return exprReturns(us->body);
        }
        return false;
    }

   bool exprReturnsOrBreaks ( ExpressionPtr expr ) {
        if ( expr->rtti_isReturn() ) {
            return true;
        } else if ( expr->rtti_isBlock() ) {
            auto block = static_cast<ExprBlock*>(expr);
            if ( hasLabels(block) ) {
                // NOTE: block with labels assumed to always return for now. real world analysis is hard
                return true;
            } else {
                for ( auto & be : block->list ) {
                    if ( be->rtti_isBreak() || be->rtti_isContinue() || be->rtti_isGoto() ) {
                        return true;
                    }
                    if ( exprReturnsOrBreaks(be) ) {
                        return true;
                    }
                }
            }
        } else if ( expr->rtti_isIfThenElse() ) {
            auto ite = static_cast<ExprIfThenElse*>(expr);
            if ( ite->if_false ) {
                return exprReturnsOrBreaks(ite->if_true) && exprReturnsOrBreaks(ite->if_false);
            }
        } else if ( expr->rtti_isWith() ) {
            auto wth = static_cast<ExprWith*>(expr);
            return exprReturnsOrBreaks(wth->body);
        } else if ( expr->rtti_isWhile() ) {
            auto wh = static_cast<ExprWhile*>(expr);
            return exprReturns(wh->body);   // note has its own break
        } else if ( expr->rtti_isFor() ) {
            auto fr = static_cast<ExprFor*>(expr);
            return exprReturns(fr->body);   // note has its own break
        } else if ( expr->rtti_isUnsafe() ) {
            auto us = static_cast<ExprUnsafe*>(expr);
            return exprReturnsOrBreaks(us->body);
        }
        return false;
    }

    // CFG-aware super-call count. Each expression produces:
    //   - fallsThrough/fallLo/fallHi: count along the path that reaches the end of expr
    //   - hasExits/exitLo/exitHi: min/max super count over all early-exit (return) paths
    //     terminating INSIDE expr. break/continue/goto don't escape the function so they
    //     stop block iteration but don't add to function-level exits.
    // For loops, inner exits (return) DO propagate; super inside a loop body collapses to
    // {0, INT_MAX} unbounded since iteration count isn't statically known.
    // Closures (ExprMakeBlock) are NOT descended into — a super() inside a defer/lambda
    // is asynchronous and isn't the chain call we're enforcing.
    static const int kSuperUnbounded = INT_MAX;
    // Matcher for the chain call the CFG count is looking for.
    // Ctor mode (mangled non-empty): super(...) post-infer is a direct call to
    // Parent`Parent, possibly _::-qualified.
    // Finalizer mode (finalizeParent set): `delete super.self` post-infer is
    // `_::finalize(cast<Parent>(self))` — match the resolved finalize call whose
    // single argument casts `self` to the immediate parent. A hand-written
    // `delete cast<Parent>(self)` lowers to the identical AST and counts the same;
    // a cast to a deeper ancestor does NOT count (it would skip the parent's field
    // slice now that generated finalizers chain level by level).
    struct SuperChainMatch {
        string      mangled;
        Structure * finalizeParent = nullptr;
        bool operator() ( ExprCall * call ) const {
            if ( !mangled.empty() ) {
                return call->name == mangled || call->name == ("_::" + mangled);
            }
            if ( !call->func || call->func->name != "finalize" ) return false;
            if ( call->arguments.size() != 1 ) return false;
            auto arg = call->arguments[0];
            if ( !arg || !arg->rtti_isCast() ) return false;
            auto cast = static_cast<ExprCast*>(arg);
            if ( !cast->castType || cast->castType->structType != finalizeParent ) return false;
            auto sub = cast->subexpr;
            return sub && sub->rtti_isVar() && static_cast<ExprVar*>(sub)->name == "self";
        }
    };
    struct SuperCount {
        bool    fallsThrough;
        int     fallLo, fallHi;
        bool    hasExits;
        int     exitLo, exitHi;
        static SuperCount onlyFall ( int lo, int hi ) { return {true, lo, hi, false, 0, 0}; }
        static SuperCount onlyExit ( int lo, int hi ) { return {false, 0, 0, true, lo, hi}; }
        static SuperCount dead     ()                  { return {false, 0, 0, false, 0, 0}; }
    };
    static int safeAdd ( int a, int b ) {
        if ( a == kSuperUnbounded || b == kSuperUnbounded ) return kSuperUnbounded;
        return a + b;
    }
    static int safeMax ( int a, int b ) {
        if ( a == kSuperUnbounded || b == kSuperUnbounded ) return kSuperUnbounded;
        return a > b ? a : b;
    }
    static void mergeExit ( SuperCount & out, int lo, int hi ) {
        if ( !out.hasExits ) { out.hasExits = true; out.exitLo = lo; out.exitHi = hi; }
        else { out.exitLo = (lo < out.exitLo) ? lo : out.exitLo; out.exitHi = safeMax(hi, out.exitHi); }
    }
    // CFG-agnostic scan: any matching super call anywhere in this subtree?
    // Used at loop boundaries to defend against paths that countSuperCalls collapses
    // via break/continue/goto (those terminate dead() and erase prefix super counts on
    // the way out, so e.g. `if(x){ super(); break }` would otherwise look super-free).
    // Closures are still skipped (super inside defer/lambda isn't the chain call).
    bool subtreeHasSuperCall ( const ExpressionPtr & expr, const SuperChainMatch & chainCall ) {
        if ( !expr ) return false;
        if ( expr->rtti_isCall() ) {
            auto call = static_cast<ExprCall*>(expr);
            if ( chainCall(call) ) return true;
            if ( call->func && call->func->module && call->func->module->name == "$"
                && call->func->name == "builtin_try_recover"
                && call->arguments.size() >= 2
                && call->arguments[0]->rtti_isMakeBlock() ) {
                auto mb = static_cast<ExprMakeBlock*>(call->arguments[0]);
                return subtreeHasSuperCall(mb->block, chainCall);
            }
            return false;
        }
        if ( expr->rtti_isBlock() ) {
            for ( auto & be : static_cast<ExprBlock*>(expr)->list ) {
                if ( subtreeHasSuperCall(be, chainCall) ) return true;
            }
            return false;
        }
        if ( expr->rtti_isIfThenElse() ) {
            auto ite = static_cast<ExprIfThenElse*>(expr);
            return subtreeHasSuperCall(ite->if_true, chainCall)
                || subtreeHasSuperCall(ite->if_false, chainCall);
        }
        if ( expr->rtti_isWith() )   return subtreeHasSuperCall(static_cast<ExprWith*>(expr)->body, chainCall);
        if ( expr->rtti_isUnsafe() ) return subtreeHasSuperCall(static_cast<ExprUnsafe*>(expr)->body, chainCall);
        if ( expr->rtti_isWhile() )  return subtreeHasSuperCall(static_cast<ExprWhile*>(expr)->body, chainCall);
        if ( expr->rtti_isFor() )    return subtreeHasSuperCall(static_cast<ExprFor*>(expr)->body, chainCall);
        if ( expr->__rtti && strcmp(expr->__rtti, "ExprTryCatch") == 0 ) {
            // Mirror countSuperCalls: only the try block contributes to chain semantics.
            // The recover block's super (if any) is irrelevant to the chain check.
            auto tc = static_cast<ExprTryCatch*>(expr);
            return subtreeHasSuperCall(tc->try_block, chainCall);
        }
        return false;
    }
    SuperCount countSuperCalls ( const ExpressionPtr & expr, const SuperChainMatch & chainCall ) {
        if ( !expr ) return SuperCount::onlyFall(0, 0);
        if ( expr->rtti_isCall() ) {
            auto call = static_cast<ExprCall*>(expr);
            if ( chainCall(call) ) {
                return SuperCount::onlyFall(1, 1);
            }
            // JIT-mode rewrite of try/recover: ast_infer_type_op.cpp:1015 emits
            // `builtin_try_recover(make_block(try), make_block(catch))` and the typer
            // appends `context` + `at` (final arity = 4). Match the resolved function on
            // its source module ($) to avoid colliding with any user shadow. Count super
            // in the try block only; the recover block is fatal-panic-with-diagnostics,
            // irrelevant to chain semantics (mirrors the direct ExprTryCatch handler below).
            if ( call->func && call->func->module && call->func->module->name == "$"
                && call->func->name == "builtin_try_recover"
                && call->arguments.size() >= 2
                && call->arguments[0]->rtti_isMakeBlock() ) {
                auto mb = static_cast<ExprMakeBlock*>(call->arguments[0]);
                return countSuperCalls(mb->block, chainCall);
            }
            return SuperCount::onlyFall(0, 0);
        }
        if ( expr->rtti_isReturn() ) {
            // Early-exit at this point with the prefix accumulated so far; no super
            // here, no fall-through. The caller (block walker) folds the prefix in.
            return SuperCount::onlyExit(0, 0);
        }
        if ( expr->rtti_isBreak() || expr->rtti_isContinue() || expr->rtti_isGoto() ) {
            // Don't escape the function — they stop block iteration (no fall-through) but
            // contribute no function-level exit path.
            return SuperCount::dead();
        }
        if ( expr->rtti_isBlock() ) {
            auto blk = static_cast<ExprBlock*>(expr);
            int prefix_lo = 0, prefix_hi = 0;
            SuperCount out = SuperCount::onlyFall(0, 0); // exits accumulated as we go
            out.hasExits = false;
            bool fallenThrough = true;
            for ( auto & be : blk->list ) {
                auto sub = countSuperCalls(be, chainCall);
                if ( sub.hasExits ) {
                    mergeExit(out, safeAdd(prefix_lo, sub.exitLo), safeAdd(prefix_hi, sub.exitHi));
                }
                if ( !sub.fallsThrough ) {
                    fallenThrough = false;
                    break;
                }
                prefix_lo = safeAdd(prefix_lo, sub.fallLo);
                prefix_hi = safeAdd(prefix_hi, sub.fallHi);
            }
            out.fallsThrough = fallenThrough;
            out.fallLo = prefix_lo;
            out.fallHi = prefix_hi;
            return out;
        }
        if ( expr->rtti_isIfThenElse() ) {
            auto ite = static_cast<ExprIfThenElse*>(expr);
            auto t = countSuperCalls(ite->if_true, chainCall);
            // Treat absent else as an empty fall-through branch with 0 super calls.
            SuperCount e = ite->if_false ? countSuperCalls(ite->if_false, chainCall)
                                         : SuperCount::onlyFall(0, 0);
            SuperCount out = SuperCount::dead();
            if ( t.hasExits ) mergeExit(out, t.exitLo, t.exitHi);
            if ( e.hasExits ) mergeExit(out, e.exitLo, e.exitHi);
            if ( t.fallsThrough && e.fallsThrough ) {
                out.fallsThrough = true;
                out.fallLo = (t.fallLo < e.fallLo) ? t.fallLo : e.fallLo;
                out.fallHi = safeMax(t.fallHi, e.fallHi);
            } else if ( t.fallsThrough ) {
                out.fallsThrough = true; out.fallLo = t.fallLo; out.fallHi = t.fallHi;
            } else if ( e.fallsThrough ) {
                out.fallsThrough = true; out.fallLo = e.fallLo; out.fallHi = e.fallHi;
            }
            return out;
        }
        if ( expr->rtti_isWith() ) {
            auto wth = static_cast<ExprWith*>(expr);
            return countSuperCalls(wth->body, chainCall);
        }
        if ( expr->rtti_isUnsafe() ) {
            auto us = static_cast<ExprUnsafe*>(expr);
            return countSuperCalls(us->body, chainCall);
        }
        if ( expr->rtti_isWhile() || expr->rtti_isFor() ) {
            ExpressionPtr body = expr->rtti_isWhile()
                ? static_cast<ExprWhile*>(expr)->body
                : static_cast<ExprFor*>(expr)->body;
            auto inner = countSuperCalls(body, chainCall);
            // Conservatively: loop iterates 0..N times. If body contains ANY super anywhere
            // (including paths that terminate via break/continue, which countSuperCalls
            // collapses to dead()), the loop's fall-through count is unbounded. Inner exits
            // (return) still propagate as function-level exit paths.
            SuperCount out = SuperCount::onlyFall(0, 0);
            if ( subtreeHasSuperCall(body, chainCall) ) {
                out.fallLo = 0; out.fallHi = kSuperUnbounded;
            }
            if ( inner.hasExits ) mergeExit(out, inner.exitLo, inner.exitHi);
            return out;
        }
        // try / recover: the try block is the normal-flow path — count super there.
        // The recover block is for diagnostics-before-exit (daslang panic is fatal); a
        // super call in recover never establishes invariants for a usable object, so skip
        // it. ExprTryCatch has no rtti_is* — dispatch via __rtti.
        if ( expr->__rtti && strcmp(expr->__rtti, "ExprTryCatch") == 0 ) {
            auto tc = static_cast<ExprTryCatch*>(expr);
            return countSuperCalls(tc->try_block, chainCall);
        }
        return SuperCount::onlyFall(0, 0);
    }
    // Reduces a function-body SuperCount to {min, max} over ALL completed CFG paths
    // (fall-through + every early-exit). The function-level lint compares this to {1, 1}.
    struct SuperCountReduced { int lo; int hi; };
    SuperCountReduced reduceSuperCount ( const SuperCount & c ) {
        if ( !c.fallsThrough && !c.hasExits ) return {0, 0}; // dead body (panic-only)
        if ( c.fallsThrough && !c.hasExits ) return {c.fallLo, c.fallHi};
        if ( !c.fallsThrough && c.hasExits ) return {c.exitLo, c.exitHi};
        int lo = (c.fallLo < c.exitLo) ? c.fallLo : c.exitLo;
        int hi = safeMax(c.fallHi, c.exitHi);
        return {lo, hi};
    }

    bool needAvoidNullPtr ( const TypeDeclPtr & type, bool allowDim ) {
        if ( !type ) {
            return false;
        }
        auto t = type;
        if ( t->baseType==Type::tFixedArray ) {
            if ( !allowDim ) {
                return false;
            }
            while ( t->baseType==Type::tFixedArray && t->firstType ) t = t->firstType;
        }
        if ( auto * ann = (TypeAnnotation *) t->isPointerToAnnotation() ) {
            if ( ann->avoidNullPtr() ) {
                return true;
            }
        }
        return false;
    }

    struct PathToLoop {
        const Variable * var = nullptr;
        const Function * func = nullptr;
    };

    bool detectLoop ( vector<PathToLoop> & path, const Variable * var, das_hash_set<const Variable*> & visitedVar, das_hash_set<const Function*> & visitedFunc );
    bool detectLoop ( vector<PathToLoop> & path, const Function * func, das_hash_set<const Variable*> & visitedVar, das_hash_set<const Function*> & visitedFunc );

    // we are going to detect, if global variable uses itself during initialization (via other variables or functions)
    bool detectLoop ( vector<PathToLoop> & path, const Variable * var, das_hash_set<const Variable*> & visitedVar, das_hash_set<const Function*> & visitedFunc ) {
        if ( var == path[0].var && path.size()>1 ) {
            // we are back to the original variable, we have a loop
            return true;
        }
        if ( visitedVar.find(var) != visitedVar.end() ) {
            return false;
        }
        visitedVar.insert(var);
        for ( auto & use : var->useGlobalVariables ) {
            path.push_back({use,nullptr});
            if ( detectLoop(path, use, visitedVar, visitedFunc) ) {
                return true;
            }
            path.pop_back();
        }
        for ( auto & use : var->useFunctions ) {
            path.push_back({nullptr,use});
            if ( detectLoop(path, use, visitedVar, visitedFunc) ) {
                return true;
            }
            path.pop_back();
        }
        return false;
    }

    bool detectLoop ( vector<PathToLoop> & path, const Function * func, das_hash_set<const Variable*> & visitedVar, das_hash_set<const Function*> & visitedFunc ) {
        if ( visitedFunc.find(func) != visitedFunc.end() ) {
            return false;
        }
        visitedFunc.insert(func);
        for ( auto & use : func->useGlobalVariables ) {
            path.push_back({use,nullptr});
            if ( detectLoop(path, use, visitedVar, visitedFunc) ) {
                return true;
            }
            path.pop_back();
        }
        for ( auto & use : func->useFunctions ) {
            path.push_back({nullptr,use});
            if ( detectLoop(path, use, visitedVar, visitedFunc) ) {
                return true;
            }
            path.pop_back();
        }
        return false;
    }

    class LintVisitor : public Visitor {
        bool checkOnlyFastAot;
        bool checkAotSideEffects;
        bool checkNoGlobalHeap;
        bool checkNoGlobalVariables;
        bool checkNoGlobalVariablesAtAll;
        bool checkUnusedArgument;
        bool checkUnusedBlockArgument;
        bool checkUnsafe;
        bool checkDeprecated;
        bool disableInit;
        bool noLocalClassMembers;
        bool noWritingToNameless;
    public:
        LintVisitor ( const ProgramPtr & prog ) : program(prog) {
            checkOnlyFastAot = program->options.getBoolOption("only_fast_aot", program->policies.only_fast_aot);
            checkAotSideEffects = program->options.getBoolOption("aot_order_side_effects", program->policies.aot_order_side_effects);
            checkNoGlobalHeap = program->options.getBoolOption("no_global_heap", program->policies.no_global_heap);
            checkNoGlobalVariables = program->options.getBoolOption("no_global_variables", program->policies.no_global_variables);
            checkNoGlobalVariablesAtAll = program->options.getBoolOption("no_global_variables_at_all", program->policies.no_global_variables_at_all);
            checkUnusedArgument = program->options.getBoolOption("no_unused_function_arguments", program->policies.no_unused_function_arguments);
            checkUnusedBlockArgument = program->options.getBoolOption("no_unused_block_arguments", program->policies.no_unused_block_arguments);
            checkUnsafe = program->policies.no_unsafe || program->thisModule->doNotAllowUnsafe;
            checkDeprecated = program->options.getBoolOption("no_deprecated", program->policies.no_deprecated);
            disableInit = prog->options.getBoolOption("no_init", prog->policies.no_init);
            noLocalClassMembers = prog->options.getBoolOption("no_local_class_members", prog->policies.no_local_class_members);
            noWritingToNameless = prog->options.getBoolOption("no_writing_to_nameless", prog->policies.no_writing_to_nameless);
        }
    public:
        void reportUnsafeTypeExpressions() {
            for ( auto expr : usedTypeExprs ) {
                program->error("type expression result is used, and not just passed",
                    "consider default<" + expr->type->describe() + ">", "",
                        expr->at, CompilationError::invalid_type_expression);
            }
        }
    protected:

        bool jitEnabled() const {
            return program->policies.jit_enabled && (!func || !func->requestNoJit);
        }

        void verifyOnlyFastAot ( Function * _func, const LineInfo & at ) {
            if ( !checkOnlyFastAot ) return;
            if ( _func && _func->builtIn ) {
                auto bif = (BuiltInFunction *) _func;
                if ( bif->cppName.empty() ) {
                    program->error(_func->describe() + " has no cppName while onlyFastAot option is set", "", "", at,
                                   CompilationError::missing_function_name );
                }
            }
        }
        bool isValidModuleName(const string & str) const {
            return !isCppKeyword(str.c_str());
        }
        virtual void preVisitModule ( Module * mod ) override {
            Visitor::preVisitModule(mod);
            if ( !mod->name.empty() && !isValidModuleName(mod->name) ) {
                program->error("invalid module name '" + mod->name + "'", "", "",
                    LineInfo(), CompilationError::invalid_module_name );
            }
        }
        void lintType ( TypeDecl * td ) {
            if ( td->firstType ) lintType(td->firstType);
            if ( td->secondType ) lintType(td->secondType);
            for ( auto & arg : td->argTypes ) lintType(arg);
        }
        virtual void preVisit ( TypeDecl * td ) override {
            Visitor::preVisit(td);
            lintType(td);
        }
        virtual void preVisitAlias ( TypeDecl * td, const string & name ) override {
            Visitor::preVisitAlias(td,name);
            if ( !td->isAuto() ) {
                if ( td->getSizeOf64()>0x7fffffff ) {
                    program->error("alias '" + name + "' is too big", "", "",
                        td->at, CompilationError::exceeds_type_alias );
                }
            }
        }
        virtual bool canVisitStructure ( Structure * st ) override {
            return !st->isTemplate;     // not a thing with templates
        }
        virtual void preVisit ( Structure * var ) override {
            Visitor::preVisit(var);
            if ( var->getSizeOf64()>0x7fffffff ) {
                program->error("structure '" + var->name + "' is too big", "", "",
                    var->at, CompilationError::exceeds_structure );
            }
        }
        virtual void preVisitExpression ( Expression * expr ) override {
            if ( expr->alwaysSafe && expr->userSaidItsSafe && !expr->generated ) {
                if (func == nullptr) {
                    // we're in global scope
                    anyUnsafe = true;
                    if ( checkUnsafe ) {
                        program->error("unsafe in global initializer.", "unsafe are prohibited by CodeOfPolicies", "",
                            expr->at, CompilationError::unsafe_global);
                    }
                    return;
                }
                auto origin = func->getOrigin();
                if ( !(origin && origin->generated) && !func->generated )
                {
                    auto fnMod = origin ? origin->module : func->module;
                    if ( fnMod == program->thisModule.get() ) {
                        anyUnsafe = true;
                        if ( checkUnsafe ) {
                            program->error("unsafe function '" + func->getMangledName() + "'", "unsafe functions are prohibited by CodeOfPolicies", "",
                                expr->at, CompilationError::unsafe_function);
                        }
                    }
                }
            }
        }
        virtual void preVisitStructureField ( Structure * var, Structure::FieldDeclaration & decl, bool last ) override {
            Visitor::preVisitStructureField(var, decl, last);
            if ( noLocalClassMembers ) {
                if ( !decl.type->ref && decl.type->hasClasses() ) {
                    program->error("class can't contain local class declarations", decl.name + ": " + decl.type->describe(), "",
                        decl.at, CompilationError::cant_field_class);
                }
            }
        }
        virtual void preVisitGlobalLet ( const VariablePtr & var ) override {
            Visitor::preVisitGlobalLet(var);
            if ( checkNoGlobalVariables && !var->generated ) {
                if ( checkNoGlobalVariablesAtAll ) {
                    program->error("variable '" + var->name + "' is disabled via option no_global_variables_at_all", "", "",
                        var->at, CompilationError::cant_global );
                } else if ( !var->type->isConst() ) {
                    program->error("variable '" + var->name + "' is not a constant, which is disabled via option no_global_variables", "", "",
                        var->at, CompilationError::cant_global );
                }
            }
            if ( checkNoGlobalHeap ) {
                if ( !var->type->isNoHeapType() ) { // note: this is too dangerous to allow even with generated
                    program->error("variable '" + var->name + "' uses heap, which is disabled via option no_global_heap", "", "",
                        var->at, CompilationError::cant_global );
                }
            }
            if ( !var->init ) {
                if ( needAvoidNullPtr(var->type,true) ) {
                    program->error("global variable of type '" + var->type->describe() + "' needs to be initialized to avoid null pointer", "", "",
                        var->at, CompilationError::missing_global);
                }
            } else {
                if ( needAvoidNullPtr(var->type,false) && var->init->rtti_isNullPtr() ) {
                    program->error("global variable of type '" + var->type->describe() + "' can't be initialized with null", "", "",
                        var->init->at, CompilationError::cant_global);
                }
            }
            if ( var->type->getSizeOf64()>0x7fffffff ) {
                program->error("global variable '" + var->name + "' is too big", "", "",
                    var->at,CompilationError::exceeds_global);
            }
        }
        virtual void preVisitGlobalLetInit ( const VariablePtr & var, Expression * that ) override {
            Visitor::preVisitGlobalLetInit(var,that);
            globalVar = var;
            das_hash_set<const Variable*> visitedVar;
            das_hash_set<const Function*> visitedFunc;
            vector<PathToLoop> path;
            path.push_back(PathToLoop{var, nullptr});
            if ( detectLoop(path, var, visitedVar, visitedFunc) ) {
                TextWriter ss;
                for ( auto & step : path ) {
                    if ( step.var ) {
                        ss << "->" << step.var->name;
                    } else if ( step.func ) {
                        ss << "->" << step.func->getMangledName();
                    }
                }
                program->error("global variable initialization loop", ss.str(), "",
                    var->at, CompilationError::recursion_global);
            }
            tableLookupCollision.push_back(das_hash_set<uint64_t>());
        }
        virtual ExpressionPtr visitGlobalLetInit ( const VariablePtr & var, Expression * that ) override {
            tableLookupCollision.pop_back();
            if ( disableInit && !var->init->rtti_isConstant() ) {   // we double check here, if it made it past infer
                program->error("[init] is disabled in the options or CodeOfPolicies", "", "",
                        var->at, CompilationError::cant_global);
            }
            globalVar->index = -3; // initialized. -1 by default
            globalVar = nullptr;
            return Visitor::visitGlobalLetInit(var,that);;
        }
        virtual void preVisit(ExprVar * expr) override {
            Visitor::preVisit(expr);
            if ( globalVar && expr->isGlobalVariable() ) {
                if ( expr->variable->index!=-3 ) {
                    if ( expr->variable->module==globalVar->module ) {
                        program->error("global variable " + expr->name + " is initialized after " + globalVar->name + " (" + to_string(expr->variable->index) + ")",
                            "", "", expr->at, CompilationError::invalid_global);
                    }
                }
            }
        }

        virtual void preVisit(ExprDelete * expr) override {
            Visitor::preVisit(expr);
            if ( needAvoidNullPtr(expr->subexpr->type,true) ) {
                program->error("can't delete " + expr->subexpr->type->describe() + ", it will create null pointer", "", "",
                    expr->subexpr->at, CompilationError::cant_expression);
            }

        }
        virtual void preVisit(ExprLet * expr) override {
            Visitor::preVisit(expr);
            // macro genearted invisible variable
            // DAS_ASSERT(expr->visibility.line);
            for (const auto & var : expr->variables) {
                if ( !var->init ) {
                    if ( needAvoidNullPtr(var->type,true) ) {
                        program->error("local variable of type " + var->type->describe() + " needs to be initialized to avoid null pointer", "", "",
                            var->at, CompilationError::missing_local);
                    }
                } else {
                    if ( needAvoidNullPtr(var->type,false) && var->init->rtti_isNullPtr() ) {
                        program->error("local variable of type " + var->type->describe() + " can't be initialized with null", "", "",
                            var->init->at, CompilationError::cant_local);
                    }
                }
                if ( var->type->getSizeOf64()>0x7fffffff ) {
                    program->error("local variable " + var->name + " is too big", "", "",
                        var->at,CompilationError::exceeds_local);
                }
            }
        }
        virtual void preVisit ( ExprReturn * expr ) override {
            Visitor::preVisit(expr);
            if ( expr->returnType && needAvoidNullPtr(expr->returnType,false) && expr->subexpr->rtti_isNullPtr() ) {
                program->error("can't return null", "", "",
                    expr->subexpr->at, CompilationError::cant_result);
            }
        }
        void verifyToTableMove ( ExprCall * expr ) {
            if ( expr->arguments[0]->rtti_isMakeArray() ) {
                auto ma = static_cast<ExprMakeArray *>(expr->arguments[0]);
                if ( ma->values.size()==0 ) return;
                auto recType = ma->recordType ? ma->recordType : ma->makeType;
                if ( recType->isTuple() ) {
                    if ( recType->argTypes[0]->isString() ) {
                        das_set<const char *,hash_ccs,equalto_ccs> seen;
                        for ( const auto & arg : ma->values ) {
                            if ( arg->rtti_isMakeTuple() ) {
                                auto mt = static_cast<ExprMakeTuple *>(arg);
                                if ( mt->values.size()==2 ) {   // its redundant
                                    if ( mt->values[0]->rtti_isStringConstant() ) {
                                        auto mc = static_cast<ExprConstString *>(mt->values[0]);
                                        if ( seen.find(mc->text.c_str())==seen.end() ) {
                                            seen.insert(mc->text.c_str());
                                        } else {
                                            program->error("duplicate key in string=> table initialization", "", "",
                                                mt->values[0]->at, CompilationError::already_declared_table);
                                        }
                                    }
                                }
                            }
                        }
                    } else {
                        das_set<vec4f,hash_vec4f,equalto_vec4f> seen;
                        for ( const auto & arg : ma->values ) {
                            if ( arg->rtti_isMakeTuple() ) {
                                auto mt = static_cast<ExprMakeTuple *>(arg);
                                if ( mt->values.size()==2 ) {   // its redundant
                                    if ( mt->values[0]->rtti_isConstant() ) {
                                        auto mc = static_cast<ExprConst *>(mt->values[0]);
                                        if ( seen.find(mc->value)==seen.end() ) {
                                            seen.insert(mc->value);
                                        } else {
                                            program->error("duplicate key in table initialization", "", "",
                                                mt->values[0]->at, CompilationError::already_declared_table);
                                        }
                                    }
                                }
                            }
                        }
                    }
                } else {
                    if ( recType->isString() ) {
                        das_set<const char *,hash_ccs,equalto_ccs> seen;
                        for ( const auto & arg : ma->values ) {
                            if ( arg->rtti_isStringConstant() ) {
                                auto mc = static_cast<ExprConstString *>(arg);
                                if ( seen.find(mc->text.c_str())==seen.end() ) {
                                    seen.insert(mc->text.c_str());
                                } else {
                                    program->error("duplicate key in string=> set initialization", "", "",
                                        arg->at, CompilationError::already_declared_table);
                                }
                            }
                        }
                    } else {
                        das_set<vec4f,hash_vec4f,equalto_vec4f> seen;
                        for ( const auto & arg : ma->values ) {
                            if ( arg->rtti_isConstant() ) {
                                auto mc = static_cast<ExprConst *>(arg);
                                if ( seen.find(mc->value)==seen.end() ) {
                                    seen.insert(mc->value);
                                } else {
                                    program->error("duplicate key in set initialization", "", "",
                                        arg->at, CompilationError::already_declared_table);
                                }
                            }
                        }
                    }
                }
            }
        }
        void verifyNoWrite ( ExprCallFunc * expr ) {
            // TODO: add support for ExprInvoke
            if ( noWritingToNameless ) {
                if ( expr->write && !(expr->type->ref || expr->type->isPointer()) ) {
                    program->error("dead write is prohibited by CodeOfPolicies", "\tin " + expr->describe(), "",
                        expr->at, CompilationError::cant_expression);
                }
            }
        }
        virtual void preVisit ( ExprCall * expr ) override {
            Visitor::preVisit(expr);
            if ( !expr->func ) {
                // a resolved ExprCall must have a non-null func by this stage;
                // reaching here means the call was produced after type inference
                // and infer was never re-run on it. common causes:
                //  - a structure or function annotation patch() mutated the AST
                //    but did not set astChanged=true, so the parse loop did not
                //    re-infer;
                //  - a pass / optimization macro modified the AST and returned
                //    false from apply(), suppressing the re-infer cycle;
                //  - a substitution macro (call / reader / typeinfo / variant /
                //    type / etc.) produced a node, but the call site forgot to
                //    forward it as a substitute so type inference never sees it.
                program->error("internal compilation error, call reached lint with unresolved func",
                    "this typically means the AST was modified after type inference "
                    "without signalling that infer needs to run again - check the "
                    "annotation patch() / pass apply() / substitution macro that produced this node",
                    "", expr->at, CompilationError::internal_function_not_resolved_yet);
                return;
            }
            verifyOnlyFastAot(expr->func, expr->at);
            verifyNoWrite(expr);
            if ( checkDeprecated && expr->func->deprecated ) {
                string message = "";
                for ( auto & ann : expr->func->annotations ) {
                    if ( ann->annotation->rtti_isFunctionAnnotation() ) {
                        auto fnAnn = static_cast<FunctionAnnotation*>(ann->annotation);
                        if ( fnAnn->name=="deprecated" ) {
                            for ( auto & arg : ann->arguments ) {
                                if ( arg.name=="message" && arg.type==Type::tString ) {
                                    message = arg.sValue;
                                }
                            }
                            break;
                        }
                    }
                }
                program->error("function " + expr->func->getMangledName() + " is deprecated.","deprecated functions are prohibited by CodeOfPolicies", message,
                    expr->at, CompilationError::cant_function);
            }
            for ( const auto & annDecl : expr->func->annotations ) {
                auto ann = annDecl->annotation;
                if ( ann->rtti_isFunctionAnnotation() ) {
                    auto fnAnn = static_cast<FunctionAnnotation*>(ann);
                    string err;
                    if ( !fnAnn->verifyCall(expr, annDecl->arguments, program->options, err) ) {
                        program->error("call annotated by " + fnAnn->name + " failed", err, "",
                                       expr->at, CompilationError::runtime_function_annotation);
                    }
                }
            }
            if ( checkAotSideEffects ) {
                if ( expr->arguments.size()>1 ) {
                    for ( auto & arg : expr->arguments ) {
                        if ( !arg->noNativeSideEffects ) {
                            program->error("side effects may affect function " + expr->func->name + " evaluation order", "", "",
                                expr->at, CompilationError::invalid_function_argument );
                            break;
                        }
                    }
                }
            }
            if (expr->name == "builtin_try_recover") {
                DAS_ASSERTF(expr->arguments.size() == 4,
                    "builtin_try_recover somehow called with wrong number of arguments = %i (%i), expected 2.",
                    int(expr->arguments.size() - 2), int(expr->arguments.size()));
                if (!expr->arguments.at(0)->rtti_isMakeBlock() ||
                    !expr->arguments.at(1)->rtti_isMakeBlock()) {
                    program->error("builtin_try_recover shouldn't be called directly.",
                        "", "Use `try { ... } recover { ... }` instead.",
                        expr->at, CompilationError::invalid_function_argument );
                } else if (exprReturns(static_cast<ExprMakeBlock*>(expr->arguments.front())->block)) {
                    program->error("try { ... } recover { ... } can't have return inside in jit mode",
                        "This feature is not implemented yet.", "",
                        expr->at, CompilationError::cant_result );
                }
            }
            for ( size_t i=0, is=expr->arguments.size(); i!=is; ++i ) {
                const auto & arg = expr->arguments[i];
                const auto & funArg = expr->func->arguments[i];
                const auto & argType = funArg->type;
                if ( needAvoidNullPtr(argType,false) && arg->rtti_isNullPtr() ) {
                    program->error("can't pass null to function " + expr->func->describeName() + " argument " + funArg->name , "", "",
                        arg->at, CompilationError::cant_argument);
                }
            }
            if ( expr->func->fromGeneric && expr->func->fromGeneric->module->name=="builtin" ) {
                if ( expr->func->fromGeneric->name=="to_table_move" ) {
                    verifyToTableMove(expr);
                }
            }
        }
        virtual ExpressionPtr visit ( ExprInvoke * expr ) override {
            Visitor::visit(expr);
            if ( expr->isInvokeMethod ) {
                auto arg0 = expr->arguments[0];
                if ( arg0->rtti_isR2V() ) {
                    arg0 = static_cast<ExprRef2Value*>(arg0)->subexpr;
                }
                if ( arg0->rtti_isField() ) {
                    auto field = static_cast<ExprField*>(arg0);
                    if ( field->value->rtti_isTypeDecl() ) {
                        usedTypeExprs.erase(field->value);
                    }
                }
            }
            return expr;
        }
        virtual ExpressionPtr visit ( ExprCall * expr ) override {
            Visitor::visit(expr);
            if ( expr->func ) {
                size_t argIndex = 0;
                for ( auto & arg : expr->func->arguments ) {
                    if ( arg->isAccessDummy() ) {
                        auto & earg = expr->arguments[argIndex];
                        if ( earg->rtti_isTypeDecl() ) {
                            usedTypeExprs.erase(earg);
                        }
                    }
                    ++argIndex;
                }
            }
            return expr;
        }
        virtual void preVisit ( ExprOp1 * expr ) override {
            Visitor::preVisit(expr);
            verifyOnlyFastAot(expr->func, expr->at);
            verifyNoWrite(expr);
        }
        virtual void preVisit ( ExprOp2 * expr ) override {
            Visitor::preVisit(expr);
            verifyOnlyFastAot(expr->func, expr->at);
            verifyNoWrite(expr);
            if ( checkAotSideEffects ) {
                if ( !expr->left->noNativeSideEffects || !expr->right->noNativeSideEffects ) {
                    program->error("side effects may affect evaluation order", "", "", expr->at,
                                   CompilationError::invalid_expression );
                }
            }
        }
        virtual void preVisit ( ExprOp3 * expr ) override {
            Visitor::preVisit(expr);
            verifyOnlyFastAot(expr->func, expr->at);
            if ( checkAotSideEffects ) {
                if ( !expr->subexpr->noNativeSideEffects || !expr->left->noNativeSideEffects || !expr->right->noNativeSideEffects ) {
                    program->error("side effects may affect evaluation order", "", "", expr->at,
                                   CompilationError::invalid_expression );
                }
            }
        }
        string getNamelessHint ( ExpressionPtr left, ExpressionPtr right, const string & op ) const {
            if ( left->rtti_isMakeTuple() ) {
                auto mkt = static_cast<ExprMakeTuple*>(left);
                ExprVar * firstField = nullptr;
                for ( const auto & val : mkt->values ) {
                    auto value = val;
                    if ( value->rtti_isR2V() ) {
                        value = static_cast<ExprRef2Value*>(value)->subexpr;
                    }
                    if ( value->rtti_isField() ) {
                        auto field = static_cast<ExprField*>(value);
                        if ( field->value->rtti_isVar() ) {
                            auto var = static_cast<ExprVar*>(field->value);
                            if ( var->variable->type->isTuple() ) {
                                if ( !firstField ) {
                                    firstField = var;
                                } else if ( firstField->variable != var->variable ) {
                                    return "";
                                }
                            } else {
                                return "";
                            }
                        } else {
                            return "";
                        }
                    } else {
                        return "";
                    }
                }
                return "did u mean " + firstField->name + " " + op + " " + right->describe();
            }
            return "";
        }
        virtual void preVisit ( ExprCopy * expr ) override {
            Visitor::preVisit(expr);
            verifyOnlyFastAot(expr->func, expr->at);
            // @E1 = E2, E2 side effects are before E1 side effects
            /*
            if ( checkAotSideEffects ) {
                if ( !expr->left->noNativeSideEffects || !expr->right->noNativeSideEffects ) {
                    program->error("side effects may affect copy evaluation order", expr->at,
                                   CompilationError::aot_side_effects );
                }
            }
            */
            if ( needAvoidNullPtr(expr->left->type,false) && expr->right->rtti_isNullPtr() ) {
                program->error("can't assign null pointer to " + expr->left->type->describe(), "", "",
                    expr->right->at, CompilationError::cant_expression);
            }
            if ( noWritingToNameless && expr->left->rtti_isMakeLocal() ) {
                program->error("dead assignment to a temporary value, which is prohibited by CodeOfPolicies",
                    getNamelessHint(expr->left, expr->right, "="), "",
                    expr->left->at, CompilationError::cant_expression);
            }
        }
        virtual void preVisit ( ExprMove * expr ) override {
            Visitor::preVisit(expr);
            verifyOnlyFastAot(expr->func, expr->at);
            if ( checkAotSideEffects ) {
                if ( !expr->left->noNativeSideEffects || !expr->right->noNativeSideEffects ) {
                    program->error("side effects may affect move evaluation order", "", "",
                        expr->at, CompilationError::invalid_expression );
                }
            }
            if ( needAvoidNullPtr(expr->left->type,false) && expr->right->rtti_isNullPtr() ) {
                program->error("can't assign null pointer to " + expr->left->type->describe(), "", "",
                    expr->right->at, CompilationError::cant_expression);
            }
            if ( noWritingToNameless && expr->left->rtti_isMakeLocal() ) {
                program->error("dead move to a temporary value, which is prohibited by CodeOfPolicies",
                    getNamelessHint(expr->left, expr->right, "<-"), "",
                        expr->left->at, CompilationError::cant_expression);
            }
        }
        virtual void preVisit ( ExprClone * expr ) override {
            Visitor::preVisit(expr);
            verifyOnlyFastAot(expr->func, expr->at);
            if ( checkAotSideEffects ) {
                if ( !expr->left->noNativeSideEffects || !expr->right->noNativeSideEffects ) {
                    program->error("side effects may affect clone evaluation order", "", "",
                        expr->at, CompilationError::invalid_expression );
                }
            }
        }
        virtual void preVisit ( ExprAscend * expr ) override {
            Visitor::preVisit(expr);
            if ( expr->subexpr->type->getSizeOf64()>0x7fffffff ) {
                program->error("can't ascend type which is too big", "", "",
                    expr->at, CompilationError::exceeds_type);
            }
            if ( !expr->subexpr->type->getSizeOf64() ) {
                program->error("can't ascend (to heap) type of size 0",  "", "",
                    expr->at, CompilationError::invalid_type);
            }
        }
        virtual void preVisit ( ExprNew * expr ) override {
            Visitor::preVisit(expr);
            if ( expr->typeexpr->getSizeOf64()>0x7fffffff ) {
                program->error("can't new to a type that is too big", "", "",
                    expr->at, CompilationError::exceeds_type);
            }
            if ( !expr->typeexpr->getSizeOf64() ) {
                program->error("can't new (to heap) type of size 0",  "", "",
                    expr->at, CompilationError::invalid_type);
            }
        }
        virtual void preVisit ( ExprAssert * expr ) override {
            Visitor::preVisit(expr);
            if ( !expr->isVerify && !expr->arguments[0]->noSideEffects ) {
                program->error("assert expressions can't have side-effects (use verify instead)", "", "",
                    expr->at, CompilationError::invalid_expression);
            }
        }
        virtual void preVisit ( ExprUnsafe * expr ) override {
            if ( !expr->generated ) {
                DAS_ASSERTF(func, "internal compiler error: ExprUnsafe should always be inside function");
                auto origin = func->getOrigin();
                if ( !(origin && origin->generated) && !func->generated )
                {
                    auto fnMod = origin ? origin->module : func->module;
                    if ( fnMod == program->thisModule.get() ) {
                        anyUnsafe = true;
                        if ( checkUnsafe ) {
                            program->error("unsafe function " + func->getMangledName(), "unsafe functions are prohibited by CodeOfPolicies", "",
                                expr->at, CompilationError::unsafe_function);
                        }
                    }
                }
            }
        }
        virtual bool canVisitFunction ( Function * fun ) override {
            return !fun->stub && !fun->isTemplate;    // we don't do a thing with templates
        }
        virtual bool canVisitArgumentInit ( Function *, const VariablePtr &, Expression * ) override {
            return false;
        }
        virtual void preVisit ( Function * fn ) override {
            Visitor::preVisit(fn);
            func = fn;
            if ( !fn->result->isVoid() && !fn->result->isAuto() ) {
                if ( !exprReturns(fn->body) ) {
                    program->error("not all control paths return value",  "", "",
                        fn->at, CompilationError::missing_function_result);
                }
            }
            if ( !fn->safeImplicit ) {
                for ( auto arg : fn->arguments ) {
                    if ( hasImplicit(arg->type) ) {
                        auto origin = fn->getOrigin();
                        auto fnMod = origin ? origin->module : fn->module;
                        if ( fnMod == program->thisModule.get() ) {
                            anyUnsafe = true;
                            if ( checkUnsafe ) {
                                program->error("implicit argument " + arg->name,  "implicit is unsafe and is prohibited by the CodeOfPolicies", "",
                                    fn->at, CompilationError::unsafe_argument);
                            }
                        }
                    }
                }
            }
            for ( auto & ann : fn->annotations ) {
                if ( ann->annotation->rtti_isFunctionAnnotation() ) {
                    auto fann = static_cast<FunctionAnnotation*>(ann->annotation);
                    string err;
                    if ( !fann->lint(fn, *program->thisModuleGroup, ann->arguments, program->options, err) ) {
                        program->error("function annotation lint failed\n", err, "", fn->at, CompilationError::runtime_function_annotation );
                    }
                }
            }
            if ( (fn->init | fn->shutdown) && disableInit ) { // we double-check here. we check in the infer first, but this here is for the case where macro does it later
                program->error("[init] is disabled in the options or CodeOfPolicies",  "", "",
                    fn->at, CompilationError::cant_function);
            }
        }
        virtual FunctionPtr visit ( Function * fn ) override {
            // Derived class ctor: every CFG path must call super(...) exactly once,
            // and only when ANY ancestor has a user-defined ctor to chain to. The
            // walk-up matches super(...)'s own walk-up semantics — an empty
            // intermediate (synth chain ctor) propagates the invariant requirement.
            if ( fn->isClassMethod && fn->classParent && fn->classParent->parent
                && !fn->generated
                && fn->name == fn->classParent->name + "`" + fn->classParent->name ) {
                Structure * chainAncestor = findChainCtorAncestor(fn->classParent);
                if ( chainAncestor ) {
                    SuperChainMatch match;
                    match.mangled = chainAncestor->name + "`" + chainAncestor->name;
                    auto count = reduceSuperCount(countSuperCalls(fn->body, match));
                    if ( count.lo == 0 ) {
                        program->error("class constructor " + fn->name + " does not call super(...) on every control-flow path",
                            "", "call super(...) (or super." + chainAncestor->name + "(...)) exactly once per path",
                            fn->at, CompilationError::missing_function_body);
                    } else if ( count.hi != count.lo || count.hi > 1 ) {
                        program->error("class constructor " + fn->name + " calls super(...) more than once on some path",
                            "", "super(...) must be called exactly once per control-flow path",
                            fn->at, CompilationError::missing_function_body);
                    }
                }
            }
            // Derived class finalizer: when ANY ancestor has a user-defined finalizer,
            // every CFG path must `delete super.self` exactly once — same chain
            // invariant as ctors. An empty intermediate parent is fine: the chain
            // call resolves to its generated finalizer, which chains up in turn.
            if ( fn->isClassMethod && fn->classParent && fn->classParent->parent
                && !fn->generated
                && fn->name == "finalize"
                && findChainFinalizerAncestor(fn->classParent) ) {
                SuperChainMatch match;
                match.finalizeParent = fn->classParent->parent;
                auto count = reduceSuperCount(countSuperCalls(fn->body, match));
                if ( count.lo == 0 ) {
                    program->error("class finalizer of " + fn->classParent->name + " does not call delete super.self on every control-flow path",
                        "", "delete super.self exactly once per path",
                        fn->at, CompilationError::missing_function_body);
                } else if ( count.hi != count.lo || count.hi > 1 ) {
                    program->error("class finalizer of " + fn->classParent->name + " calls delete super.self more than once on some path",
                        "", "delete super.self must be called exactly once per control-flow path",
                        fn->at, CompilationError::missing_function_body);
                }
            }
            func = nullptr;
            return Visitor::visit(fn);
        }
        virtual void preVisitArgument ( Function * fn, const VariablePtr & var, bool lastArg ) override {
            Visitor::preVisitArgument(fn, var, lastArg);
            if ( checkUnusedArgument ) {
                if ( !var->marked_used && var->isAccessUnused() ) {
                    program->error("unused function argument " + var->name, "",
                          "use [unused_argument(" + var->name + ")] if intentional",
                        var->at, CompilationError::invalid_function_argument);
                }
            }
            if ( var->type->getSizeOf64()>0x7fffffff ) {
                program->error("argument variable " + var->name + " is too big", "", "",
                    var->at,CompilationError::exceeds_argument);
            }
        }
        virtual void preVisit ( ExprBlock * block ) override {
            Visitor::preVisit(block);
            if ( block->isClosure ) {
                if (  !block->returnType->isVoid() && !block->returnType->isAuto() ) {
                    if ( !exprReturns(block) ) {
                        program->error("not all control paths of the block return value",  "", "",
                            block->at, CompilationError::missing_block_result);
                    }
                }
            }
            if (jitEnabled() && !block->finalList.empty() && block->hasExitByLabel) {
                program->error("jit blocks can't have finally and goto", "", "",
                    block->at, CompilationError::cant_block);
            }
        }
        virtual void preVisitBlockArgument ( ExprBlock * block, const VariablePtr & var, bool lastArg ) override {
            Visitor::preVisitBlockArgument(block, var, lastArg);
            if ( checkUnusedBlockArgument ) {
                if ( !var->marked_used && var->isAccessUnused() ) {
                    program->error("unused block argument " + var->name, "",
                          "use [unused_argument(" + var->name + ")] if intentional",
                        var->at, CompilationError::invalid_block_argument);
                }
            }
            if ( var->type->getSizeOf64()>0x7fffffff ) {
                program->error("block argument variable " + var->name + " is too big", "", "",
                    var->at,CompilationError::exceeds_argument);
            }
        }
        virtual void preVisit ( ExprMakeStruct * mks ) override {
            Visitor::preVisit(mks);
            if ( mks->constructor && mks->constructor->arguments.size() ) {
                program->error("default arguments of constructors can't be used in make declarations", "its not yet implemented", "",
                    mks->at, CompilationError::cant_argument_structure);
            }
        }
        virtual void preVisit ( ExprTypeDecl * expr ) override {
            Visitor::preVisit(expr);
            if ( expr->alwaysSafe ) return;
            usedTypeExprs.insert(expr);
        }
        virtual void preVisitBlockExpression ( ExprBlock * block, Expression * expr ) override {
            Visitor::preVisitBlockExpression(block, expr);
            tableLookupCollision.push_back(das_hash_set<uint64_t>());
            if ( expr->rtti_isOp2() ) {
                auto op2 = static_cast<ExprOp2 *>(expr);
                if ( op2->func && op2->func->builtIn && op2->func->sideEffectFlags==0 ) {
                    program->error("top level no side effect operation " + op2->op, "", "",
                        expr->at, CompilationError::invalid_expression);
                }
            }
        }
        virtual ExpressionPtr visitBlockExpression ( ExprBlock * block, Expression * expr ) override {
            tableLookupCollision.pop_back();
            return Visitor::visitBlockExpression(block,expr);
        }

        virtual void preVisitBlockFinalExpression ( ExprBlock * block, Expression * expr ) override {
            Visitor::preVisitBlockFinalExpression(block, expr);
            tableLookupCollision.push_back(das_hash_set<uint64_t>());
            if ( expr->rtti_isOp2() ) {
                auto op2 = static_cast<ExprOp2 *>(expr);
                if ( op2->func && op2->func->builtIn && op2->func->sideEffectFlags==0 ) {
                    program->error("top level no side effect operation " + op2->op, "", "",
                        expr->at, CompilationError::invalid_expression);
                }
            }
        }
        virtual ExpressionPtr visitBlockFinalExpression (  ExprBlock * block, Expression * expr ) override {
            tableLookupCollision.pop_back();
            return Visitor::visitBlockFinalExpression(block,expr);
        }
        virtual void preVisit ( ExprAt * expr ) override {
            Visitor::preVisit(expr);
            // we look for table at, and check 'subexpr' of it, which is the table.
            // if it collides with anything - its an error
            if ( !expr->underDeref && expr->subexpr->type->isGoodTableType() ) {
                auto subexprText = expr->subexpr->describe();
                auto subexprHash = hash64z(subexprText.c_str());
                auto it = tableLookupCollision.back().find(subexprHash);
                if ( it!=tableLookupCollision.back().end() ) {
                    if ( program->policies.temp_table_lint_warning ) {
                        (*daScriptEnvironment::getBound()->g_compilerLog) << expr->subexpr->at.describe() << ": *warning* potential table lookup collision for " << subexprText << "\n";
                    } else {
                        program->error("potential table lookup collision for " + subexprText, "",
                                "tab[key1] = tab[key2], or fun(tab[key1],tab[key2]) scenarios may produce undefined behavior",
                                expr->subexpr->at, CompilationError::invalid_table_expression);
                    }
                } else {
                    tableLookupCollision.back().insert(subexprHash);
                }
            }
        }
    public:
        ProgramPtr program;
        Function * func = nullptr;
        Variable * globalVar = nullptr;
        bool anyUnsafe = false;
        das_hash_set<Expression *> usedTypeExprs;
        vector<das_hash_set<uint64_t>> tableLookupCollision;
    };

    struct Option {
        const char *    name;
        Type            type;
    } g_allOptions [] = {
    // lint
        "lint",                         Type::tBool,
        "no_writing_to_nameless",       Type::tBool,
    // memory
        "heap_size_limit",              Type::tInt,
        "string_heap_size_limit",       Type::tInt,
        "gc",                           Type::tBool,
    // coverage
        "no_coverage",                  Type::tBool,
    // aot
        "no_aot",                       Type::tBool,
        "aot_prologue",                 Type::tBool,
    // logging
        "log",                          Type::tBool,
        "log_optimization_passes",      Type::tBool,
        "log_optimization",             Type::tBool,
        "log_stack",                    Type::tBool,
        "log_init",                     Type::tBool,
        "log_symbol_use",               Type::tBool,
        "log_var_scope",                Type::tBool,
        "log_nodes",                    Type::tBool,
        "log_nodes_aot_hash",           Type::tBool,
        "log_mem",                      Type::tBool,
        "log_debug_mem",                Type::tBool,
        "log_aot",                      Type::tBool,
        "log_infer_passes",             Type::tBool,
        "log_require",                  Type::tBool,
        "log_generics",                 Type::tBool,
        "log_mn_hash",                  Type::tBool,
        "log_gmn_hash",                 Type::tBool,
        "log_ad_hash",                  Type::tBool,
        "log_aliasing",                 Type::tBool,
        "print_ref",                    Type::tBool,
        "print_var_access",             Type::tBool,
        "print_c_style",                Type::tBool,
        "print_use",                    Type::tBool,
    // optimization
        "optimize",                     Type::tBool,
        "no_optimization",              Type::tBool,
        "fusion",                       Type::tBool,
        "remove_unused_symbols",        Type::tBool,
    // language
        "always_export_initializer",    Type::tBool,
        "infer_time_folding",           Type::tBool,
        "disable_run",                  Type::tBool,
        "indenting",                    Type::tInt,
        "no_unsafe_uninitialized_structures", Type::tBool,
    // version_2_syntax
        "gen2",                         Type::tBool,
    // deprecated
        "skip_lock_checks",             Type::tBool,
    };

    static struct VerifyOptionsOnStartup {
        VerifyOptionsOnStartup() {
            bool failed = false;
            for ( const auto & opt : g_allOptions ) {
                if ( !isValidBuiltinName(opt.name) ) {
                    DAS_FATAL_ERROR("%s - invalid option. expecting snake_case\n", opt.name);
                    failed = true;
                }
            }
            DAS_VERIFYF(!failed, "verifyOptions failed");
        }
    } g_verifyOptionsOnStartup;

    vector<pair<string,Type>> getCodeOfPolicyOptions();

    void Program::lint ( TextWriter & /*logs*/, ModuleGroup & libGroup ) {
        if (!options.getBoolOption("lint", !policies.no_lint)) {
            return;
        }
        // note: build access flags is now called before patchAnnotations, and is no longer needed in lint
        // TextWriter logs; buildAccessFlags(logs);
        checkSideEffects();
        // lint it
        LintVisitor lintV(this);
        visit(lintV);
        lintV.reportUnsafeTypeExpressions();
        unsafe = lintV.anyUnsafe;
        // check for invalid options
        das_map<string,Type> ao;
        for ( const auto & opt : g_allOptions ) {
            auto it = ao.find(opt.name);
            if ( it != ao.end() ) {
                error("internal error: option '" + string(opt.name) + "' is already defined",
                    "", "", LineInfo(), CompilationError::internal_options);
            } else {
                ao[opt.name] = opt.type;
            }
        }
        for ( const auto & opt : getCodeOfPolicyOptions() ) {
            auto it = ao.find(opt.first);
            if ( it != ao.end() ) {
                error("internal error: option '" + opt.first + "' is already defined",
                    "", "", LineInfo(), CompilationError::internal_options);
            } else {
                ao[opt.first] = opt.second;
            }
        }
        for ( const auto & opt : options ) {
            Type optT = Type::none;
            auto it = ao.find(opt.name);
            if ( it != ao.end() ) {
                optT = it->second;
            } else {
                for ( auto & mod : libGroup.modules ) {
                    auto ot = mod->getOptionType(opt.name);
                    if ( ot!=Type::none ) {
                        optT = ot;
                        break;
                    }
                }
            }
            if ( optT!=Type::none && optT!=opt.type ) {
                error("invalid option type for '" + opt.name
                      + "', unexpected '" + das_to_string(opt.type)
                      + "', expecting '" + das_to_string(optT) + "'", "", "",
                        LineInfo(), CompilationError::invalid_options);
            } else if ( optT==Type::none ){
                if ( opt.name[0]!='_' ) {
                    error("invalid option '" + opt.name + "'",  "", "",
                        LineInfo(), CompilationError::invalid_options);
                } else {
                    // custom user option (name starts with '_'), we don't care what's in there
                    continue;
                }
            }
        }
        set<Module *> lints;
        Module::foreach([&](Module * mod) -> bool {
            DAS_ASSERT ( mod!=thisModule.get() );
            for ( const auto & pm : mod->globalLintMacros ) {
                pm->apply(this, thisModule.get());
            }
            return true;
        });
        libGroup.foreach([&](Module * mod) -> bool {
            if ( thisModule->isVisibleDirectly(mod) && mod!=thisModule.get() ) {
                for ( const auto & pm : mod->lintMacros ) {
                    pm->apply(this, thisModule.get());
                }
            }
            return true;
        },"*");
    }

    class InferLintVisitor : public Visitor {
    public:
        InferLintVisitor ( const ProgramPtr & prog ) : program(prog) {
        }
    public:
        virtual ExpressionPtr visitExpression ( Expression * expr ) override {
            if ( !expr->type ) return expr;
            if ( expr->rtti_isTypeDecl() ) return expr;
            if ( expr->type->isAliasOrExpr() ) {
                program->error("internal error. leaking alias or expression type",expr->type->describe(),"",
                    expr->at,CompilationError::internal_type);
            }
            return expr;
        }
    public:
        ProgramPtr program;

    };

    void Program::inferLint(TextWriter &) {
        if ( !policies.verify_infer_types ) return;
        InferLintVisitor lintV(this);
        visit(lintV);
    }
}

