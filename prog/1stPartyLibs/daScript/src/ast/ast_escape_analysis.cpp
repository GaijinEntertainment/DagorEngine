#include "daScript/misc/platform.h"

#include "daScript/ast/ast.h"
#include "daScript/ast/ast_visitor.h"
#include "daScript/ast/ast_generate.h"
#include "daScript/ast/ast_cfg.h"
#include "daScript/ast/ast_escape_analysis.h"
#include "daScript/misc/string_writer.h"
#include <cstdint>

namespace das {

    // Escape analysis and the optimization that consumes it are two separate passes, so the analysis
    // result (the Variable escape flags) stands on its own and the AST mutation is separable:
    //
    //   ANALYSIS (escapeAnalysis) - three pieces with a clear data boundary between them:
    //     1. EscapeGraph        - PURE DATA: a flow graph over pointer-valued variables + per-function
    //                             return summaries. No logic.
    //     2. EscapeGraphBuilder - one whole-program walk that POPULATES an EscapeGraph (sinks, flow
    //                             edges, return summaries, work lists). No propagation.
    //     3. EscapeSolver       - CONSUMES the graph: two monotone fixpoints (escape + fresh-owned),
    //                             then writes the result onto Variable::does_not_escape / escape_* /
    //                             escape_no_stack. No AST walking.
    //
    //   OPTIMIZATION (scopeFreeOptimization, ScopeFreeVisitor) - consumes Variable::does_not_escape and
    //     emits a deterministic free at the end of the declaring block (scope exit), instead of leaving
    //     the object to the heap GC. Its "did the AST change" result drives a re-infer.
    //
    // The analysis is conservative: a use is "safe" only when recognized (field-base read, escape-
    // neutral argument, copy alias, forwarded ownership). Any unrecognized use leaks the pointer, so
    // under-approximating the safe uses keeps it sound.

    static bool isFreshAlloc ( Expression * init ) {
        // `new T()` is an ExprNew; `new T(field=val)` lowers to ExprAscend over ExprMakeStruct.
        // both hand the local a freshly-allocated, uniquely-owned heap object.
        return init && ( init->rtti_isNewExpr() || init->rtti_isAscend() );
    }

    // type shape we can scope-free: a freshly-allocated, non-smart pointer to a daslang struct.
    static bool isEscapeFreePtr ( const TypeDeclPtr & typ ) {
        if ( !typ ) return false;
        if ( typ->ref || typ->constant ) return false;
        if ( typ->baseType != Type::tPointer ) return false;
        if ( typ->smartPtr ) return false;
        if ( !typ->firstType || typ->firstType->baseType != Type::tStructure ) return false;
        return typ->canDelete();
    }

    // shared gates for a scope-free candidate local: a freeable pointer-to-struct in a function
    // whose scope-exit semantics are well-defined (not generated/generator/lambda; unsafe excluded
    // as it can hide aliasing the field-base analysis cannot see)
    static bool isFreeCandidateShape ( Function * func, ExprLet * expr, const VariablePtr & var ) {
        return func
            && !func->generated && !func->generator && !func->lambda
            && !func->hasUnsafe
            && !expr->inScope && !var->inScope
            && !var->consumed && !var->single_return_via_move && !var->early_out
            && isEscapeFreePtr(var->type);
    }

    // a local pointer is a candidate for scope-free iff it is freshly allocated and type-eligible
    static bool isEscapeFreeCandidate ( Function * func, ExprLet * expr, const VariablePtr & var ) {
        return isFreeCandidateShape(func, expr, var) && isFreshAlloc(var->init);
    }

    // strip pure pointer<->ref / ref->value representation nodes, returning the underlying expression
    static Expression * peelRefNodes ( Expression * e ) {
        while ( e ) {
            if ( e->rtti_isPtr2Ref() ) e = ((ExprPtr2Ref *)e)->subexpr;
            else if ( e->rtti_isRef2Ptr() ) e = ((ExprRef2Ptr *)e)->subexpr;
            else if ( e->rtti_isR2V() ) e = ((ExprRef2Value *)e)->subexpr;
            else break;
        }
        return e;
    }

    // the exact ExprVar a value dereferences to (after peeling representation nodes), or null
    static ExprVar * derefBaseVar ( Expression * e ) {
        e = peelRefNodes(e);
        return ( e && e->rtti_isVar() ) ? (ExprVar *) e : nullptr;
    }

    // the called function, if the expression is a direct call (after peeling), or null
    static Function * derefCallee ( Expression * e ) {
        e = peelRefNodes(e);
        return ( e && e->rtti_isCallFunc() ) ? ((ExprCallFunc *)e)->func : nullptr;
    }

    // positional index of `arg` in the call's argument list (~0 if not found)
    static size_t callArgIndex ( ExprLooksLikeCall * call, Expression * arg ) {
        for ( size_t i=0; i!=call->arguments.size(); ++i ) {
            if ( call->arguments[i]==arg ) return i;
        }
        return ~size_t(0);
    }

    // a by-value (non-ref) pointer to a daslang struct - the value whose escape we can track
    static bool isPointerToStruct ( const TypeDeclPtr & typ ) {
        return typ && !typ->ref && typ->baseType==Type::tPointer && !typ->smartPtr
            && typ->firstType && typ->firstType->baseType==Type::tStructure;
    }

    static bool isParamEscapeCandidate ( const VariablePtr & var ) {
        return isPointerToStruct(var->type);
    }

    // a function whose body we can soundly analyze for parameter escape: visible body, no hidden
    // aliasing via unsafe, not a generated / generator / lambda shape the field-base analysis can't model
    static bool isParamAnalyzableFunc ( Function * func ) {
        return func && !func->builtIn && !func->stub && !func->isTemplate
            && !func->generated && !func->generator && !func->lambda && !func->hasUnsafe;
    }

    // can a pointer passed at positional `argIndex` of this call escape through the callee? returns
    // true when it CANNOT (escape-neutral for that one argument). built-ins are judged by their
    // declared side effects + a return that can't carry the pointer out; script functions by the
    // interprocedural per-parameter result computed in ParamEscapeAnalysis (which already folds in the
    // return / global-store / store-into-another-arg / transitive-call channels).
    static bool isArgEscapeNeutral ( Function * fn, size_t argIndex ) {
        if ( !fn || fn->unsafeOperation ) return false;
        if ( fn->builtIn ) {
            if ( fn->sideEffectFlags != 0 ) return false;
            auto res = fn->result;
            return res && !res->ref && (res->isVoid() || res->isWorkhorseType());
        }
        if ( argIndex >= fn->arguments.size() ) return false;
        return fn->arguments[argIndex]->does_not_escape;
    }

    static bool finalListFreesVar ( ExprBlock * block, Variable * var ) {
        class VarRefFinder : public Visitor {
        public:
            VarRefFinder ( Variable * v ) : var(v) {}
            bool found = false;
        protected:
            virtual ExpressionPtr visit ( ExprVar * e ) override {
                if ( e->variable == var ) found = true;
                return Visitor::visit(e);
            }
            Variable * var;
        };

        VarRefFinder finder(var);
        for ( auto & fe : block->finalList ) {
            fe->visit(finder);
            if ( finder.found ) return true;
        }
        return false;
    }

    enum class EscapeKind {
        DoesNotEscape,
        Return,
        Argument,
        Global
    };

    static void recordEscape ( Variable * var, EscapeKind kind ) {
        switch ( kind ) {
            case EscapeKind::DoesNotEscape: var->does_not_escape = true;  break;
            case EscapeKind::Return:        var->escapes_return = true;   break;
            case EscapeKind::Argument:      var->escapes_argument = true; break;
            case EscapeKind::Global:        var->escapes_global = true;   break;
        }
    }

    static const char * escapeKindName ( EscapeKind kind ) {
        switch ( kind ) {
            case EscapeKind::DoesNotEscape: return "does not escape";
            case EscapeKind::Return:        return "escapes via return";
            case EscapeKind::Argument:      return "escapes via argument";
            case EscapeKind::Global:        return "escapes via store";
        }
        return "?";
    }

    static void logEscape ( TextWriter * logs, Function * func, Variable * var, EscapeKind kind ) {
        if ( !logs ) return;
        if ( !var->at.empty() && var->at.fileInfo ) {
            *logs << var->at.fileInfo->name << ":" << var->at.line << ":" << var->at.column << " ";
        }
        *logs << "escape analysis: '" << var->name << "' " << escapeKindName(kind)
              << " in '" << func->module->name << "::" << func->name << "'\n";
    }

    // ===== Escape graph =====
    // The product of one whole-program walk: a flow graph over pointer-valued variables (parameters
    // and locals alike) plus per-function return summaries. PURE DATA - no analysis logic. Built by
    // EscapeGraphBuilder, consumed by EscapeSolver.
    //
    // Model: Choi et al., "Escape Analysis for Java" (OOPSLA'99) connection graph + the three-state
    // NoEscape/ArgEscape/GlobalEscape lattice; backward flow-insensitive propagation as in Gay &
    // Steensgaard, "Fast Escape Analysis and Stack Allocation" (CC'00).
    struct EscapeGraph {
        // SEEDS - direct facts read straight off the syntax:
        das_hash_map<Variable *, EscapeKind>           sinks;        // a var used in a leaking position, + how
        das_hash_map<Function *, bool>                 freshOwned;   // optimistic: f returns a uniquely-owned fresh object
        // EDGES - relations the solver propagates to a fixpoint:
        das_hash_map<Variable *, vector<Variable *>>   revDep;       // escapes(src) |= escapes(dst); revDep[dst] = {src...}
        das_hash_map<Function *, vector<Function *>>   retForward;   // freshOwned(f) &= freshOwned(g) for `return g(...)`
        // WORK LISTS - what to materialize once the fixpoint settles:
        vector<pair<Variable *, Function *>>           paramFn;      // analyzable params       -> Variable::does_not_escape
        vector<pair<Variable *, Function *>>           freshLocalFn; // fresh-alloc local owners -> EscapeKind

        struct CallResultLet { Variable * var; Function * callee; Function * fn; };
        vector<CallResultLet>                          callResultLets; // `var c = f(...)` candidates for ownership transfer
        das_set<Variable *>                            aliasedLocal; // copied/captured local   -> not stack-relocatable
    };

    // ===== Escape graph builder =====
    // ONE pass over analyzed function bodies. Its only job is to POPULATE an EscapeGraph: record
    // direct escape sinks, alias/argument flow edges, per-function return summaries, and the
    // variables to materialize. It performs NO propagation - that is the solver's job.
    //   input:  the set of functions we are allowed to analyze
    //   output: a populated EscapeGraph (returned by the static build() entry point)
    class EscapeGraphBuilder : public Visitor {
    public:
        static EscapeGraph build ( Program * prog, const das_set<Function *> & analyzed ) {
            EscapeGraphBuilder builder(analyzed);
            prog->visit(builder);
            return move(builder.graph);
        }
    protected:
        EscapeGraphBuilder ( const das_set<Function *> & analyzed_ ) : analyzed(analyzed_) {}

        // ---- recording helpers (the only writers of `graph`) ----
        void sink ( Variable * v, EscapeKind kind ) {                  // a direct leak (seed)
            if ( graph.sinks.find(v)==graph.sinks.end() ) graph.sinks[v] = kind;
        }
        void flowEdge ( Variable * src, Variable * dst ) {             // escapes(src) |= escapes(dst)
            graph.revDep[dst].push_back(src);
        }

        // ---- classification helpers ----
        EscapeKind depthKind () const {                                // how an unrecognized use leaks, by context
            if ( returnDepth > 0 ) return EscapeKind::Return;
            if ( argDepth > 0 ) return EscapeKind::Argument;
            return EscapeKind::Global;
        }

        // ---- per-function setup ----
        virtual bool canVisitFunction ( Function * fun ) override {
            return analyzed.find(fun)!=analyzed.end();
        }
        virtual void preVisit ( Function * fun ) override {
            func = fun; returnDepth = 0; argDepth = 0; makeBlockDepth = 0;
            safeBase.clear();
            for ( auto & arg : fun->arguments )
                if ( isParamEscapeCandidate(arg) ) graph.paramFn.push_back({arg, fun});
            graph.freshOwned[fun] = isPointerToStruct(fun->result);    // optimistic; a non-fresh return revokes it
        }
        virtual FunctionPtr visit ( Function * fun ) override { func = nullptr; return fun; }

        // ---- variables to materialize ----
        virtual VariablePtr visitLet ( ExprLet * expr, const VariablePtr & var, bool ) override {
            if ( isEscapeFreeCandidate(func, expr, var) ) {
                graph.freshLocalFn.push_back({var, func});                            // `var x = new T`
            } else if ( isFreeCandidateShape(func, expr, var) ) {
                if ( auto callee = derefCallee(var->init) )                           // `var c = f(...)`
                    graph.callResultLets.push_back({var, callee, func});
            }
            return var;
        }

        // ---- SAFE (non-escaping) uses: pre-marked BEFORE the inner ExprVar is visited ----
        // a plain copy `var x = y` makes x an alias of y: y escapes only if x does (flow edge), and a
        // copied-from LOCAL can no longer be stack-relocated (the alias would dangle on relocation)
        virtual void preVisitLetInit ( ExprLet *, const VariablePtr & var, Expression * init ) override {
            if ( !isPointerToStruct(var->type) || var->init_via_move || var->init_via_clone || isFreshAlloc(init) ) return;
            if ( auto src = derefBaseVar(init) ) {
                if ( (src->local || src->argument) && isPointerToStruct(src->variable->type) ) {
                    safeBase.insert(src);
                    flowEdge(src->variable, var);
                    if ( src->local ) graph.aliasedLocal.insert(src->variable);
                }
            }
        }
        virtual void preVisit ( ExprField * expr ) override {          // a field read never leaks the base
            if ( auto v = derefBaseVar(expr->value) ) safeBase.insert(v);
        }
        virtual void preVisit ( ExprSafeField * expr ) override {      // `p?.x` is a distinct hook from ExprField
            if ( auto v = derefBaseVar(expr->value) ) safeBase.insert(v);
        }
        virtual void preVisit ( ExprOp2 * expr ) override {            // a null-guard `p == null` doesn't leak p
            if ( isArgEscapeNeutral(expr->func, 0) ) { if ( auto v = derefBaseVar(expr->left) ) safeBase.insert(v); }
            if ( isArgEscapeNeutral(expr->func, 1) ) { if ( auto v = derefBaseVar(expr->right) ) safeBase.insert(v); }
        }
        virtual void preVisitCallArg ( ExprCall * call, Expression * arg, bool ) override {
            argDepth++;
            auto src = derefBaseVar(arg);
            if ( !src ) return;
            auto fn = call->func;
            auto idx = callArgIndex(call, arg);
            if ( fn && !fn->builtIn && !fn->unsafeOperation && analyzed.find(fn)!=analyzed.end()
                 && idx < fn->arguments.size() && isParamEscapeCandidate(fn->arguments[idx]) ) {
                safeBase.insert(src);                                  // analyzed callee: arg escapes iff its param does
                flowEdge(src->variable, fn->arguments[idx]);
            } else if ( isArgEscapeNeutral(call->func, idx) ) {
                safeBase.insert(src);                                  // builtin / opaque callee that can't retain it
            }
            // otherwise the argument leaks: handled as a direct sink in visit(ExprVar)
        }
        virtual ExpressionPtr visitCallArg ( ExprCall *, Expression * arg, bool ) override {
            argDepth--; return arg;
        }
        // a string builder ("{x}") formats each element into a FRESH string by a read-only walk of its
        // value; it never retains the pointer, so a base used only here does not escape - like a field
        // read. (A custom string-cast lowers to a separate call element, handled by preVisitCallArg.)
        virtual void preVisitStringBuilderElement ( ExprStringBuilder *, Expression * expr, bool ) override {
            if ( auto v = derefBaseVar(expr) ) safeBase.insert(v);
        }

        // ---- scope tracking that affects classification ----
        virtual void preVisit ( ExprReturn * expr ) override {
            returnDepth++;
            if ( !func || !isPointerToStruct(func->result) ) return;
            auto e = peelRefNodes(expr->subexpr);
            if ( e && isFreshAlloc(e) ) {
                // returns a freshly-allocated object - keeps fresh-owned candidacy (no-op)
            } else if ( e && e->rtti_isCallFunc() && ((ExprCallFunc *)e)->func ) {
                graph.retForward[func].push_back(((ExprCallFunc *)e)->func);
            } else {
                graph.freshOwned[func] = false;   // parameter / global / field / null / ... - not fresh-owned
            }
        }
        virtual ExpressionPtr visit ( ExprReturn * expr ) override { returnDepth--; return expr; }
        // a block captures by reference and cannot be stored/returned: its captures do not escape, but
        // (like a copy alias) the reference forbids stack relocation of the captured pointee
        virtual void preVisit ( ExprMakeBlock * ) override { makeBlockDepth++; }
        virtual ExpressionPtr visit ( ExprMakeBlock * expr ) override { makeBlockDepth--; return expr; }

        // ---- the leak classifier: any tracked use not pre-marked safe is a direct sink ----
        virtual ExpressionPtr visit ( ExprVar * expr ) override {
            const auto trackedVar = expr->variable && (expr->local || expr->argument)
                && isPointerToStruct(expr->variable->type);
            if ( trackedVar ) {
                if ( makeBlockDepth > 0 && expr->local ) graph.aliasedLocal.insert(expr->variable);
                if ( safeBase.find(expr)==safeBase.end() ) sink(expr->variable, depthKind());
            }
            return expr;
        }

        EscapeGraph graph;                            // the product being built
        const das_set<Function *> & analyzed;         // input: functions we may analyze
        Function * func = nullptr;                    // walk state below
        int returnDepth = 0, argDepth = 0, makeBlockDepth = 0;
        das_set<ExprVar *> safeBase;                  // ExprVar uses already accounted as non-leaking
    };

    // ===== Escape solver =====
    // Consumes an EscapeGraph and produces the final per-variable result. Two monotone fixpoints over
    // the graph's edges, then write-back onto the AST:
    //   1. escape:      propagate the escape bit backward along flow edges (worklist).
    //   2. fresh-owned: revoke a function's fresh-owned summary if any forwarded callee is not.
    //   3. materialize: Variable::does_not_escape (params + freeable locals), EscapeKind, and
    //      escape_no_stack (freeable but copied/captured -> freed at scope exit, not relocated).
    class EscapeSolver {
    public:
        EscapeSolver ( EscapeGraph g, TextWriter * logs_ ) : graph(move(g)), logs(logs_) {
            for ( auto & s : graph.sinks ) esc[s.first] = s.second;   // seed the escape fixpoint
        }
        bool solveAndApply () {
            solveEscape();
            solveFreshOwned();
            // `var c = f(...)` where f returns a uniquely-owned fresh object and c does not itself
            // escape the caller: ownership transfers, so the caller frees c at scope exit.
            for ( auto & cr : graph.callResultLets ) {
                if ( graph.freshOwned[cr.callee] && !escapes(cr.var) )
                    graph.freshLocalFn.push_back({cr.var, cr.fn});
            }
            return materialize();
        }
    protected:
        // a variable's escape verdict IS its EscapeKind: DoesNotEscape (the map default) means it does
        // not escape; any other kind means it does, and records how. No separate boolean needed.
        EscapeKind escapeOf ( Variable * v ) { return esc[v]; }
        bool escapes ( Variable * v ) { return escapeOf(v) != EscapeKind::DoesNotEscape; }

        // backward reachability: a var escapes if any value it flowed into escapes
        void solveEscape () {
            vector<Variable *> work;
            for ( auto & kv : esc ) if ( kv.second != EscapeKind::DoesNotEscape ) work.push_back(kv.first);
            while ( !work.empty() ) {
                auto v = work.back(); work.pop_back();
                auto it = graph.revDep.find(v);
                if ( it==graph.revDep.end() ) continue;
                auto vkind = esc[v];   // capture BEFORE touching esc[src], whose insert may rehash esc
                for ( auto src : it->second ) {
                    auto & k = esc[src];
                    if ( k==EscapeKind::DoesNotEscape ) { k = vkind; work.push_back(src); }
                }
            }
        }
        // a function is fresh-owned iff every return is fresh or forwards another fresh-owned function
        void solveFreshOwned () {
            bool changed = true;
            while ( changed ) {
                changed = false;
                for ( auto & kv : graph.retForward ) {
                    if ( !graph.freshOwned[kv.first] ) continue;
                    for ( auto g : kv.second )
                        if ( !graph.freshOwned[g] ) { graph.freshOwned[kv.first] = false; changed = true; break; }
                }
            }
        }
        // write the settled result onto the AST
        bool materialize () {
            bool any = false;
            for ( auto & pf : graph.paramFn ) {
                bool e = escapes(pf.first);
                pf.first->does_not_escape = !e;
                if ( e && logs ) logParam(pf.second, pf.first);
                any = true;
            }
            for ( auto & lf : graph.freshLocalFn ) {
                auto kind = escapeOf(lf.first);
                recordEscape(lf.first, kind);
                // freeable but copied/captured: free at scope exit, but do NOT stack-relocate (the
                // surviving alias/reference would dangle once the pointee moves into the frame)
                if ( kind==EscapeKind::DoesNotEscape && graph.aliasedLocal.find(lf.first)!=graph.aliasedLocal.end() ) {
                    lf.first->escape_no_stack = true;
                }
                logEscape(logs, lf.second, lf.first, kind);
                any = true;
            }
            return any;
        }
        void logParam ( Function * fn, Variable * var ) {
            if ( !var->at.empty() && var->at.fileInfo )
                *logs << var->at.fileInfo->name << ":" << var->at.line << ":" << var->at.column << " ";
            *logs << "escape analysis: parameter '" << var->name << "' escapes in '"
                  << fn->module->name << "::" << fn->name << "'\n";
        }

        EscapeGraph                          graph;   // input graph (also holds the promoted free list)
        das_hash_map<Variable *, EscapeKind> esc;     // per-variable escape fixpoint state (default = DoesNotEscape)
        TextWriter *                         logs = nullptr;
    };

    // ===== Scope-free optimization (consumes Variable::does_not_escape) =====

    // set the stack-allocation flag on the fresh-alloc init expression; returns true if it changed it.
    // only the shapes the codegen tiers can stack-allocate: a plain `new T()` (no initializer call,
    // no dim) over a plain struct, and `new T(f=v)` which lowers to an ascend over a make-struct.
    // capped by size. classes / constructors / handles are excluded: their construction has semantics
    // (ctor invocation, rtti) that the plain in-frame build does not reproduce (breaks under JIT).
    static bool setAllocateOnStack ( Expression * init ) {
        // upper bound on a pointee we are willing to move onto the (fixed-size) stack frame
        static constexpr uint32_t MAX_STACK_ALLOC_SIZE = 256;
        if ( init && init->rtti_isNewExpr() ) {
            auto en = (ExprNew *) init;
            auto st = en->typeexpr ? en->typeexpr->structType : nullptr;
            bool persistent = st && st->persistent;
            bool isClass = st && st->isClass;
            // use the non-asserting 64-bit size: a malformed/oversized type (e.g. in invalid_types.das)
            // yields a huge value that simply fails the cap, instead of tripping the size<=0x7fffffff assert
            bool fits = en->typeexpr && en->typeexpr->getBaseSizeOf64() <= uint64_t(MAX_STACK_ALLOC_SIZE);
            if ( !en->initializer && en->typeexpr && en->typeexpr->baseType!=Type::tFixedArray && !persistent && !isClass && fits && !en->allocate_on_stack ) {
                en->allocate_on_stack = true; return true;
            }
        } else if ( init && init->rtti_isAscend() ) {
            auto ea = (ExprAscend *) init;
            bool plainStruct = false;
            if ( ea->subexpr && ea->subexpr->rtti_isMakeStruct() ) {
                auto mks = (ExprMakeStruct *) ea->subexpr;
                auto mkT = mks->makeType;
                while ( mkT && mkT->baseType==Type::tFixedArray && mkT->firstType ) mkT = mkT->firstType;
                auto st = mkT ? mkT->structType : nullptr;
                // note: isNewClass means "make-struct produced by a `new`" (set for plain structs too),
                // NOT "is a class" - the class signal is the constructor / forceClass / structType->isClass
                plainStruct = !mks->constructor && !mks->isNewHandle
                    && !mks->forceClass && !( st && st->isClass );
            }
            auto subT = ea->subexpr ? ea->subexpr->type : nullptr;
            while ( subT && subT->baseType==Type::tFixedArray && subT->firstType ) subT = subT->firstType;
            bool persistent = subT && subT->structType && subT->structType->persistent;
            bool fits = ea->subexpr && ea->subexpr->type && ea->subexpr->type->getSizeOf64() <= uint64_t(MAX_STACK_ALLOC_SIZE);
            if ( plainStruct && !persistent && fits && !ea->allocate_on_stack ) {
                ea->allocate_on_stack = true; return true;
            }
        }
        return false;
    }

    static bool initIsAllocatedOnStack ( Expression * init ) {
        if ( init && init->rtti_isNewExpr() ) return ((ExprNew *)init)->allocate_on_stack;
        if ( init && init->rtti_isAscend() ) return ((ExprAscend *)init)->allocate_on_stack;
        return false;
    }

    // GC-style raw free (no finalizer), matching what the heap GC would do for this garbage.
    static ExprCall * makeScopeFreeCall ( Variable * var ) {
        auto call = new ExprCall(var->at, "_::builtin_scope_free");
        call->arguments.push_back(new ExprVar(var->at, var->name));
        call->arguments.push_back(new ExprConstUInt(var->at, var->type->firstType->getSizeOf()));
        call->alwaysSafe = true;
        return call;
    }

    class ScopeFreeVisitor : public Visitor {
    public:
        bool anyWork = false;
        ScopeFreeVisitor ( TextWriter * logs_, bool forceStack_ ) : logs(logs_), forceStack(forceStack_) {}
    protected:
        virtual bool canVisitFunction ( Function * fun ) override {
            return !fun->stub && !fun->isTemplate;
        }
        virtual void preVisit ( Function * fun ) override {
            Visitor::preVisit(fun);
            func = fun;
            pending.clear();
        }
        virtual FunctionPtr visit ( Function * fun ) override {
            func = nullptr;
            return Visitor::visit(fun);
        }
        virtual void preVisit ( ExprBlock * block ) override {
            Visitor::preVisit(block);
            blocks.push_back(block);
        }
        virtual ExpressionPtr visit ( ExprBlock * block ) override {
            for ( auto & p : pending ) {
                if ( p.second != block ) continue;
                if ( finalListFreesVar(block, p.first) ) continue;   // already emitted on a prior pass
                emitScopeFree(block, p.first);
            }
            blocks.pop_back();
            return Visitor::visit(block);
        }
        virtual VariablePtr visitLet ( ExprLet * expr, const VariablePtr & var, bool last ) override {
            if ( var->does_not_escape && !blocks.empty() ) {
                // put the pointee in the stack frame instead of the heap; idempotent so the
                // notInferred re-infer loop terminates once the flag is already set
                if ( forceStack && !var->escape_no_stack && setAllocateOnStack(var->init) ) {
                    anyWork = true;
                    func->notInferred();
                }
                // emit the scope-free only when it frees something real: a heap shell (not stack-
                // allocated) or owned heap members (arrays/tables) inside the pointee. a stack-
                // allocated POD frees nothing, so skip it - else every scope pays an interop call.
                bool stacked = forceStack && initIsAllocatedOnStack(var->init);
                bool ownsHeap = !( var->type->firstType && var->type->firstType->isNoHeapType() );
                if ( !stacked || ownsHeap ) {
                    pending.push_back({var, blocks.back()});
                }
            }
            return Visitor::visitLet(expr,var,last);
        }
        void emitScopeFree ( ExprBlock * block, Variable * var ) {
            if ( var->type->firstType->getSizeOf64() > 0x7fffffffull ) return;  // skip pointees over the 2^31 (32-bit) size limit - oversized/invalid types that error out anyway
            anyWork = true;
            func->notInferred();
            block->finalList.insert(block->finalList.begin(), makeScopeFreeCall(var));
            if ( logs ) {
                if ( !var->at.empty() && var->at.fileInfo ) {
                    *logs << var->at.fileInfo->name << ":" << var->at.line << ":" << var->at.column << " ";
                }
                *logs << "scope-free applied to '" << var->name << "' in '" << func->module->name << "::" << func->name << "'\n";
            }
        }
        Function * func = nullptr;
        TextWriter * logs = nullptr;
        bool forceStack = false;
        vector<ExprBlock *> blocks;
        vector<pair<Variable *, ExprBlock *>> pending;
    };

    // ANALYSIS: build the escape graph from one whole-program walk, solve it, and write the result
    // onto the AST (Variable::does_not_escape / escape_* / escape_no_stack). No AST mutation - the
    // flags ARE the analysis result, consumed by scopeFreeOptimization (and reusable by anything else).
    bool escapeAnalysis(Program * program, TextWriter & logs) {
        auto & options = program->options;
        auto & policies = program->policies;
        auto forceStack = options.getBoolOption("force_allocate_on_stack", policies.force_allocate_on_stack);
        if ( !options.getBoolOption("force_escape_free", policies.force_escape_free) && !forceStack ) return false;
        auto logEscape = options.getBoolOption("log_escape_analysis", policies.log_escape_analysis);
        das_set<Function *> analyzed;
        program->thisModule->functions.foreach([&](auto & fn){
            if ( isParamAnalyzableFunc(fn) ) analyzed.insert(fn);
        });
        EscapeGraph graph = EscapeGraphBuilder::build(program, analyzed);
        return EscapeSolver(move(graph), logEscape ? &logs : nullptr).solveAndApply();
    }

    // ===== Flow-sensitive escape (partial escape analysis) - entirely file-local =====
    namespace {
        // Per-block dataflow facts for one function's tracked objects, filled by analyzeFlow: a FORWARD
        // escape flow ("has this object escaped on some path to here?", merge = union) and a BACKWARD
        // liveness ("is it read on some path forward from here?"). Two separate fixpoints (opposite
        // directions) write into one result. Only escapedOut is kept for escape (the entry value is a
        // transient folded into the fixpoint); liveness keeps both ends (both are read downstream).
        // b is always a block of this cfg (from cfg.blocks / pred / succ, never null) with a dense id in
        // [0, size), so the accessors need no null / bounds guard.
        struct FlowFacts {
            struct PerBlock {
                das_set<Variable *> escapedOut;   // objects escaped at block exit
                das_set<Variable *> liveIn;       // objects live at block entry
                das_set<Variable *> liveOut;      // objects live at block exit
            };
            vector<Variable *>   objects;         // tracked fresh-alloc pointer locals
            vector<PerBlock>     block;           // [block id] -> that block's escape + liveness sets
            bool escapedAtExit ( CfgBlock * b, Variable * o ) const {
                return block[b->id].escapedOut.find(o)!=block[b->id].escapedOut.end();
            }
            bool liveAtEntry ( CfgBlock * b, Variable * o ) const {
                return block[b->id].liveIn.find(o)!=block[b->id].liveIn.end();
            }
            bool liveAtExit ( CfgBlock * b, Variable * o ) const {
                return block[b->id].liveOut.find(o)!=block[b->id].liveOut.end();
            }
        };

        // the whole-body facts a single FunctionScanner walk produces
        struct FunctionScan {
            das_set<Variable *>                     tracked;    // dedup set of fresh-alloc pointer locals
            vector<Variable *>                      objects;    // same, in discovery order
        };

        // ONE whole-body walk: collects the tracked fresh-alloc pointer locals (dedup set + ordered list).
        struct FunctionScanner : Visitor {
            FunctionScan & fs;
            FunctionScanner ( FunctionScan & fs )
                : fs(fs) {}

            static FunctionScan scanFunction ( Function * fn ) {
                FunctionScan r;
                if ( fn && fn->body ) {
                    FunctionScanner fs(r);
                    fn->body->visit(fs);
                }
                return r;
            }

            virtual VariablePtr visitLet ( ExprLet *, const VariablePtr & var, bool ) override {
                if ( isPointerToStruct(var->type) && isFreshAlloc(var->init) && fs.tracked.insert(var).second ) {
                    fs.objects.push_back(var);
                }
                return var;
            }
        };


        // the per-block sets a single BlockScanner walk produces, indexed by block id
        struct BlockScan {
            vector<das_set<Variable *>> escapedOut;   // escape-gen (forward escape flow's seed)
            vector<das_set<Variable *>> uses;         // upward-exposed reads (liveness gen)
            vector<das_set<Variable *>> defs;         // declarations (liveness kill)
        };

        // ONE per-block walk (statements IN ORDER), for the block's tracked vars:
        //  - escaped: escape-gen - a use that is NOT a safe field-base / escape-neutral arg / string-
        //    builder element; the forward escape flow's seed.
        //  - defs: vars declared here; uses: upward-exposed reads (read before declared here, so `defs`
        //    doubles as "declared so far") - the backward liveness's gen/kill. (Confines a variable's
        //    live range to its declaration; otherwise it would leak past the decl into outer scopes.)
        struct BlockScanner : Visitor {
            const das_set<Variable *> & tracked;
            das_set<ExprVar *> safeBase;

            BlockScan &res;
            uint32_t b_id;

            // scan every block once (one BlockScanner per block), returning the per-block escape-gen / uses / defs.
            static BlockScan scanBlocks ( const Cfg & cfg, const das_set<Variable *> & tracked ) {
                BlockScan r;
                auto nb = cfg.blocks.size();
                r.escapedOut.resize(nb);
                r.uses.resize(nb);
                r.defs.resize(nb);
                for ( auto b : cfg.blocks ) {
                    BlockScanner sc(tracked, r, b->id);
                    for ( auto s : b->stmts ) {
                        s->visit(sc);
                    }
                }
                return r;
            }

            explicit BlockScanner ( const das_set<Variable *> & t, BlockScan &res_, uint32_t b_id_ ) : tracked(t), res(res_), b_id(b_id_) {}
            virtual void preVisit ( ExprField * e ) override {
                if ( auto v = derefBaseVar(e->value) ) safeBase.insert(v);
            }
            virtual void preVisit ( ExprSafeField * e ) override {
                if ( auto v = derefBaseVar(e->value) ) safeBase.insert(v);
            }
            virtual void preVisit ( ExprOp2 * e ) override {
                if ( isArgEscapeNeutral(e->func, 0) ) { if ( auto v = derefBaseVar(e->left) ) safeBase.insert(v); }
                if ( isArgEscapeNeutral(e->func, 1) ) { if ( auto v = derefBaseVar(e->right) ) safeBase.insert(v); }
            }
            virtual void preVisitCallArg ( ExprCall * call, Expression * arg, bool ) override {
                if ( isArgEscapeNeutral(call->func, callArgIndex(call, arg)) )
                    if ( auto v = derefBaseVar(arg) ) safeBase.insert(v);
            }
            virtual void preVisitStringBuilderElement ( ExprStringBuilder *, Expression * e, bool ) override {
                if ( auto v = derefBaseVar(e) ) safeBase.insert(v);
            }
            virtual VariablePtr visitLet ( ExprLet *, const VariablePtr & var, bool ) override {
                if ( tracked.find(var)!=tracked.end() ) res.defs[b_id].insert(var);
                return var;
            }
            virtual ExpressionPtr visit ( ExprVar * e ) override {
                if ( e->variable && tracked.find(e->variable)!=tracked.end() ) {
                    if ( safeBase.find(e)==safeBase.end() ) res.escapedOut[b_id].insert(e->variable);   // escape-gen
                    if ( res.defs[b_id].find(e->variable)==res.defs[b_id].end() ) res.uses[b_id].insert(e->variable);    // upward-exposed use
                }
                return e;
            }
        };
    }

    // Compute all per-block flow facts: the forward escape flow + the backward liveness, into one
    // FlowFacts. Takes the already-collected tracked set / objects; scanBlocks supplies the per-block
    // gen/kill in a single walk, then the two fixpoints run (independent - opposite directions).
    static FlowFacts analyzeFlow ( const Cfg & cfg, const das_set<Variable *> & tracked, const vector<Variable *> & objects ) {
        FlowFacts f;
        auto nb = cfg.blocks.size();
        f.block.resize(nb);
        f.objects = objects;
        if ( tracked.empty() ) return f;

        auto [escapedGen, uses, defs] = BlockScanner::scanBlocks(cfg, tracked);
        for ( auto b : cfg.blocks ) {
            f.block[b->id].escapedOut = move(escapedGen[b->id]);  // seed escape-gen
        }

        // ---- forward escape flow ----
        // escapedOut[B] (already seeded with escape-gen) |= union of preds' escapedOut. Monotone -> a size
        // change is the only signal needed; iterate to stable.
        bool changed = true;
        while ( changed ) {
            changed = false;
            for ( auto b : cfg.blocks ) {
                auto & out = f.block[b->id].escapedOut;
                auto before = out.size();
                for ( auto p : b->pred ) {
                    for ( auto o : f.block[p->id].escapedOut ) {
                        out.insert(o);
                    }
                }
                if ( out.size()!=before ) changed = true;
            }
        }

        // ---- backward liveness ----
        //   liveOut[B] = union of liveIn over B's successors
        //   liveIn[B]  = uses[B] | (liveOut[B] \ defs[B])
        // iterated against the edges until the sets stop growing.
        changed = true;
        while ( changed ) {
            changed = false;
            for ( auto b : cfg.blocks ) {
                auto & out = f.block[b->id].liveOut;
                for ( auto s : b->succ ) {
                    for ( auto o : f.block[s->id].liveIn ) {
                        out.insert(o);
                    }
                }
                auto & in = f.block[b->id].liveIn;
                auto before = in.size();
                for ( auto o : uses[b->id] ) {
                    in.insert(o);
                }
                for ( auto o : out ) {
                    if ( defs[b->id].find(o)==defs[b->id].end() ) {
                        in.insert(o);
                    }
                }
                if ( in.size()!=before ) changed = true;
            }
        }
        return f;
    }

    // Combine the escape flow and liveness into the set of free sites: for each object, the CFG blocks
    // at whose SCOPE EXIT it can be freed. A block qualifies when the object is dead and not-escaped
    // leaving it (so freeing there is safe on that path) AND it just died there - either it is live
    // entering the block (its last use is inside) or it was dead on entry but live at EVERY
    // predecessor's exit (it died on every incoming edge). That "just died here" condition is the death
    // frontier: it makes each path free the object exactly once, with no double-free at a later join.
    // Returns object -> blocks; partialEscapeFree then inserts a builtin_scope_free at each.
    static das_hash_map<Variable *, vector<CfgBlock *>>
    partialFreeSites ( const Cfg & cfg, const FlowFacts & f ) {
        das_hash_map<Variable *, vector<CfgBlock *>> sites;
        for ( auto b : cfg.blocks ) {
            for ( auto o : f.objects ) {
                if ( f.liveAtExit(b, o) ) continue;                // still live leaving b -> not dead here
                if ( f.escapedAtExit(b, o) ) continue;             // escaped on this path -> owned elsewhere
                // b is on the death frontier for o iff o dies by the end of b: either o is live ENTERING b
                // and dead leaving it (its last use is in b), or o was dead-on-entry but live at EVERY
                // predecessor's exit (it died on every incoming edge). Both mean a single free at b's
                // scope exit covers this path with no double-free (a later block has o already dead).
                bool diesHere = f.liveAtEntry(b, o);
                if ( !diesHere ) {
                    if ( b->pred.empty() ) continue;
                    diesHere = true;
                    for ( auto p : b->pred ) {
                        if ( !f.liveAtExit(p, o) ) {
                            diesHere = false; break;
                        }
                    }
                }
                if ( diesHere ) sites[o].push_back(b);
            }
        }
        return sites;
    }

    namespace {
        // pre-scan every candidate target list once for the variables it already frees, keyed by the list,
        // so the insertion loop checks a set instead of re-scanning per candidate. The loop maintains this
        // as it inserts, keeping idempotence within one pass (two sites sharing a list) and across re-infers.
        das_hash_map<const vector<Expression *> *, das_set<Variable *>>
        collectFreedVars ( const das_hash_map<Variable *, vector<CfgBlock *>> & sites ) {
            das_hash_map<const vector<Expression *> *, das_set<Variable *>> freedIn;
            for ( const auto & kv : sites ) {
                for ( auto b : kv.second ) {
                    const vector<Expression *> * lp = b->astHead ? &b->astHead->finalList
                        : ( b->contOwner ? &b->contOwner->list : nullptr );
                    if ( !lp || freedIn.find(lp)!=freedIn.end() ) continue;
                    auto & freed = freedIn[lp];
                    for ( auto s : *lp ) {
                        if ( !s->rtti_isCall() ) continue;
                        auto call = static_cast<ExprCall *>(s);
                        if ( call->name.find("builtin_scope_free")==string::npos || call->arguments.empty() ) continue;
                        auto a0 = call->arguments[0];
                        if ( a0->rtti_isVar() ) freed.insert(static_cast<ExprVar *>(a0)->variable);
                    }
                }
            }
            return freedIn;
        }
    }

    // the CFG models if/while/for and fall-through finalList, but NOT finalList on an early-exit edge
    // (return/break/continue still run it) nor try/recover bodies. partialEscapeFree frees at flow-
    // determined points, so an incomplete CFG could free before a use the CFG never saw - bail out for
    // any function carrying those constructs (conservative: such objects fall back to scope-exit / GC).
    static bool cfgMayBeIncomplete ( Function * fn ) {
        struct Scan : Visitor {
            bool found = false;
            virtual void preVisit ( ExprBlock * b ) override {
                Visitor::preVisit(b);
                if ( !b->finalList.empty() ) found = true;
            }
            virtual void preVisit ( ExprTryCatch * e ) override {
                Visitor::preVisit(e);
                found = true;
            }
        };
        Scan s;
        fn->body->visit(s);
        return s.found;
    }

    static bool partialEscapeFree ( Function * fn, const Cfg * cfgIn, TextWriter * logs ) {
        if ( !fn || !fn->body || !fn->body->rtti_isBlock() ) return false;
        if ( fn->generated || fn->generator || fn->lambda || fn->hasUnsafe ) return false;
        if ( cfgMayBeIncomplete(fn) ) return false;   // finally / try-recover -> CFG can't be trusted
        if ( !cfgIn ) return false;
        const Cfg & cfg = *cfgIn;
        auto fs = FunctionScanner::scanFunction(fn);              // tracked fresh-alloc pointer locals
        if ( fs.objects.empty() ) return false;
        auto facts = analyzeFlow(cfg, fs.tracked, fs.objects);
        auto sites = partialFreeSites(cfg, facts);
        auto freedIn = collectFreedVars(sites);
        bool any = false;
        for ( const auto & [o, v] : sites ) {
            if ( o->does_not_escape ) continue;            // freed at scope exit already -> no double free
            if ( !isEscapeFreePtr(o->type) ) continue;     // not a deletable plain pointer-to-struct
            if ( o->type->firstType->getSizeOf64() > 0x7fffffffull ) continue;
            for ( auto b : v ) {
                // pick the insertion target. A block that opens a lexical scope frees at its scope exit
                // (finalList - runs after the last use, also on a `return`). A continuation block frees as
                // a statement before its first statement, on the fall-through path only - and only for an
                // edge-death (o already dead at entry), else that statement could precede a use of o.
                ExprBlock * tgt = nullptr;
                bool intoFinalList = false;
                if ( b->astHead ) {
                    tgt = b->astHead; intoFinalList = true;
                } else if ( b->contOwner && b->contBefore && !facts.liveAtEntry(b, o) ) {
                    tgt = b->contOwner; intoFinalList = false;
                } else {
                    continue;                              // no clean AST insertion point
                }
                auto & list = intoFinalList ? tgt->finalList : tgt->list;
                auto & freed = freedIn[&list];
                if ( freed.find(o)!=freed.end() ) continue;
                size_t at = list.size();                   // finalList: append (back); continuation: before anchor
                if ( !intoFinalList ) {
                    at = 0; bool found = false;
                    for ( ; at<list.size(); ++at ) if ( list[at]==b->contBefore ) { found = true; break; }
                    if ( !found ) continue;                // anchor no longer present
                }
                list.insert(list.begin() + at, makeScopeFreeCall(o));
                freed.insert(o);
                fn->notInferred();
                any = true;
                if ( logs ) {
                    if ( !o->at.empty() && o->at.fileInfo )
                        *logs << o->at.fileInfo->name << ":" << o->at.line << ":" << o->at.column << " ";
                    *logs << "partial-escape free of '" << o->name << "' on a non-escaping path in '"
                          << fn->module->name << "::" << fn->name << "'";
                    auto & fat = intoFinalList ? tgt->at : b->contBefore->at;   // where the free is inserted
                    if ( !fat.empty() && fat.fileInfo )
                        *logs << " (freed at " << fat.fileInfo->name << ":" << fat.line << ":" << fat.column << ")";
                    *logs << "\n";
                }
            }
        }
        return any;
    }

    // OPTIMIZATION: consume the analysis result, emitting scope-exit frees / stack relocation. Returns
    // whether the AST changed, so the caller re-infers the inserted scope_free calls.
    bool scopeFreeOptimization(Program * program, const ProgramCfg * pcfg, TextWriter & logs) {
        auto & options = program->options;
        auto & policies = program->policies;
        auto forceStack = options.getBoolOption("force_allocate_on_stack", policies.force_allocate_on_stack);
        if ( !options.getBoolOption("force_escape_free", policies.force_escape_free) && !forceStack ) return false;
        auto logEscape = options.getBoolOption("log_escape_analysis", policies.log_escape_analysis);
        ScopeFreeVisitor sfv(logEscape ? &logs : nullptr, forceStack);
        program->visit(sfv);
        bool anyWork = sfv.anyWork;
        // flow-sensitive pass (opt-in via force_partial_escape_free): free objects on the paths where they
        // don't escape (the ones the scope-exit pass left to GC because they escape on SOME other path).
        // reads the shared CFG built by the caller; without it (analysis disabled) only the simple EA runs.
        if ( pcfg && options.getBoolOption("force_partial_escape_free", policies.force_partial_escape_free) ) {
            program->thisModule->functions.foreach([&](auto & fn){
                if ( partialEscapeFree(fn, pcfg->forFunction(fn), logEscape ? &logs : nullptr) ) anyWork = true;
            });
        }
        return anyWork;
    }

}
