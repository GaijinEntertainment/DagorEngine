#include "daScript/misc/platform.h"

#include "daScript/ast/ast.h"
#include "daScript/ast/ast_visitor.h"
#include "daScript/ast/ast_generate.h"

namespace das {

    // Escape analysis and the optimization that uses it are kept as two separate passes:
    //
    //   1. EscapeAnalysisVisitor  - PURE ANALYSIS. Computes, per local pointer, whether it can
    //      escape its scope, and records the result on Variable::does_not_escape. No AST mutation.
    //      The result is a reusable property other optimizations could also consult.
    //
    //   2. ScopeFreeVisitor       - OPTIMIZATION (scope-based deallocation, a la RAII/drop).
    //      Consumes Variable::does_not_escape and emits a deterministic `delete` at the end of the
    //      declaring block, so the object is freed at scope exit instead of waiting for the heap GC.
    //
    // The analysis is conservative: a local is marked does_not_escape only when EVERY use is a
    // field access (the base of an ExprField). Under-approximating the safe uses keeps it sound -
    // any unrecognized use leaves the variable as "escapes" and it is left to the GC.

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

    // a local pointer is a candidate for scope-free iff it is freshly allocated and type-eligible,
    // in a function whose scope-exit semantics are well-defined (not generated/generator/lambda;
    // unsafe excluded as it can hide aliasing the field-base analysis cannot see)
    static bool isEscapeFreeCandidate ( Function * func, ExprLet * expr, const VariablePtr & var ) {
        return func
            && !func->generated && !func->generator && !func->lambda
            && !func->hasUnsafe
            && !expr->inScope && !var->inScope
            && !var->consumed && !var->single_return_via_move && !var->early_out
            && isFreshAlloc(var->init)
            && isEscapeFreePtr(var->type);
    }

    // unwrap pure pointer<->ref / ref->value representation nodes to the variable being
    // dereferenced; returns the exact ExprVar node the traversal will visit
    static ExprVar * derefBaseVar ( Expression * e ) {
        while ( e ) {
            if ( e->rtti_isVar() ) return (ExprVar *) e;
            else if ( e->rtti_isPtr2Ref() ) e = ((ExprPtr2Ref *)e)->subexpr;
            else if ( e->rtti_isRef2Ptr() ) e = ((ExprRef2Ptr *)e)->subexpr;
            else if ( e->rtti_isR2V() ) e = ((ExprRef2Value *)e)->subexpr;
            else break;
        }
        return nullptr;
    }

    // Passing the pointer to such a call cannot let it escape: the callee is a built-in (C++) that is
    // fully pure (no declared side effects, not unsafe, so it can't store the argument anywhere) and
    // whose return can't carry the pointer back out (non-ref, void-or-workhorse). Restricted to
    // built-ins because their SideEffects are declared at bind time and reliable here; script-function
    // side effects are only inferred in a later pass (ast_unused), so script calls stay conservative.
    static bool isEscapeNeutralCall ( ExprCall * call ) {
        auto fn = call->func;
        if ( !fn || !fn->builtIn || fn->sideEffectFlags != 0 || fn->unsafeOperation ) return false;
        auto res = fn->result;
        return res && !res->ref && (res->isVoid() || res->isWorkhorseType());
    }

    static bool escapeDecided ( Variable * var ) {
        return var->does_not_escape || var->escapes_return || var->escapes_argument
            || var->escapes_global;
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

    // ===== Pass 1: escape analysis (classifies each candidate into the escape-kind result) =====
    class EscapeAnalysisVisitor : public Visitor {
    public:
        bool anyChanged = false;
        EscapeAnalysisVisitor ( TextWriter * logs_ ) : logs(logs_) {}
    protected:
        virtual bool canVisitFunction ( Function * fun ) override {
            return !fun->stub && !fun->isTemplate;
        }
        void record ( Variable * var, EscapeKind kind ) {
            recordEscape(var, kind);
            logEscape(logs, func, var, kind);
            anyChanged = true;
        }
        virtual void preVisit ( Function * fun ) override {
            Visitor::preVisit(fun);
            func = fun;
            candidates.clear();
            safeBase.clear();
            returnDepth = 0;
            argDepth = 0;
        }
        virtual FunctionPtr visit ( Function * fun ) override {
            // a candidate with no escape kind recorded provably does not escape
            for ( auto var : candidates ) {
                if ( escapeDecided(var) ) continue;
                record(var, EscapeKind::DoesNotEscape);
            }
            func = nullptr;
            return Visitor::visit(fun);
        }
        virtual VariablePtr visitLet ( ExprLet * expr, const VariablePtr & var, bool last ) override {
            // only analyze still-undecided candidates (a decided one keeps its frozen result)
            if ( !escapeDecided(var) && isEscapeFreeCandidate(func, expr, var) ) {
                candidates.insert(var);
            }
            return Visitor::visitLet(expr,var,last);
        }
        virtual void preVisit ( ExprReturn * expr ) override { Visitor::preVisit(expr); returnDepth++; }
        virtual ExpressionPtr visit ( ExprReturn * expr ) override { returnDepth--; return Visitor::visit(expr); }
        virtual void preVisitCallArg ( ExprCall * call, Expression * arg, bool last ) override {
            Visitor::preVisitCallArg(call, arg, last); argDepth++;
            // a pointer passed directly to a pure, non-aliasing-return call can't escape through it
            if ( isEscapeNeutralCall(call) ) {
                if ( auto v = derefBaseVar(arg) ) safeBase.insert(v);
            }
        }
        virtual ExpressionPtr visitCallArg ( ExprCall * call, Expression * arg, bool last ) override {
            argDepth--; return Visitor::visitCallArg(call, arg, last);
        }
        virtual void preVisit ( ExprField * expr ) override {
            Visitor::preVisit(expr);
            if ( auto v = derefBaseVar(expr->value) ) safeBase.insert(v);
        }
        virtual ExpressionPtr visit ( ExprVar * expr ) override {
            // a use that is not a field-access base leaks the pointer value: classify how
            if ( expr->variable && candidates.find(expr->variable)!=candidates.end()
                 && safeBase.find(expr)==safeBase.end() ) {
                auto var = expr->variable;
                if (returnDepth > 0) {
                    record(var, EscapeKind::Return);
                } else if (argDepth > 0) {
                    record(var, EscapeKind::Argument);
                } else {
                    record(var, EscapeKind::Global);
                }
            }
            return Visitor::visit(expr);
        }
        Function * func = nullptr;
        TextWriter * logs = nullptr;
        int returnDepth = 0;
        int argDepth = 0;
        das_set<Variable *> candidates;
        das_set<Expression *> safeBase;
    };

    // ===== Pass 2: scope-free optimization (consumes Variable::does_not_escape) =====
    // upper bound on a pointee we are willing to move onto the (fixed-size) stack frame; anything
    // larger stays on the heap (and is still freed at scope exit) so a big struct can't blow the stack
    static constexpr uint32_t MAX_STACK_ALLOC_SIZE = 256;

    // set the stack-allocation flag on the fresh-alloc init expression; returns true if it changed it.
    // only the shapes the codegen tiers can stack-allocate: a plain `new T()` (no initializer call,
    // no dim) over a plain struct, and `new T(f=v)` which lowers to an ascend over a make-struct.
    // capped by size. classes / constructors / handles are excluded: their construction has semantics
    // (ctor invocation, rtti) that the plain in-frame build does not reproduce (breaks under JIT).
    static bool setAllocateOnStack ( Expression * init ) {
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
                if ( forceStack && setAllocateOnStack(var->init) ) {
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
            // GC-style raw free (no finalizer), matching what the heap GC would do for this garbage.
            auto call = new ExprCall(var->at, "_::builtin_scope_free");
            call->arguments.push_back(new ExprVar(var->at, var->name));
            call->arguments.push_back(new ExprConstUInt(var->at, var->type->firstType->getSizeOf()));
            call->alwaysSafe = true;
            block->finalList.insert(block->finalList.begin(), call);
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

    bool Program::escapeAnalysis(TextWriter & logs) {
        auto forceStack = options.getBoolOption("force_allocate_on_stack", policies.force_allocate_on_stack);
        if ( !options.getBoolOption("force_escape_free", policies.force_escape_free) && !forceStack ) return false;
        auto logEscape = options.getBoolOption("log_escape_analysis", policies.log_escape_analysis);
        EscapeAnalysisVisitor ev(logEscape ? &logs : nullptr);
        visit(ev);
        return ev.anyChanged;
    }

    bool Program::scopeFreeOptimization(TextWriter & logs) {
        auto forceStack = options.getBoolOption("force_allocate_on_stack", policies.force_allocate_on_stack);
        if ( !options.getBoolOption("force_escape_free", policies.force_escape_free) && !forceStack ) return false;
        auto logEscape = options.getBoolOption("log_escape_analysis", policies.log_escape_analysis);
        ScopeFreeVisitor ev(logEscape ? &logs : nullptr, forceStack);
        visit(ev);
        return ev.anyWork;
    }

}
