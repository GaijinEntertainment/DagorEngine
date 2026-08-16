#include "daScript/misc/platform.h"

#include "daScript/ast/ast.h"
#include "daScript/ast/ast_visitor.h"
#include "daScript/ast/ast_expressions.h"

namespace das {

    // Prefix for every local this pass manufactures. ONE leading underscore, not
    // two: GLSL ES reserves any identifier containing `__`, and WebGL's compiler
    // enforces it (desktop GL drivers do not), so a `__`-prefixed temp made every
    // inlined shader helper fail to compile in the browser while working on the
    // desktop. Emitting a name that is illegal in a target we support is the
    // compiler's problem to avoid, not each backend's to sanitize. A single
    // leading underscore is legal there and matches what the GLSL backend already
    // generates for its own temps (`_for_range_vs`).
    // nextInlineIdIn() parses ids straight back off this prefix, so generator and
    // matcher must never drift — they share this constant and its length.
    static const char * INLINE_TEMP_PREFIX = "_inl";
    static const size_t INLINE_TEMP_PREFIX_LEN = 4;

    // ===== [inline] splicing, automatic block inlining, invoke devirtualization =====
    // Runs in the patch slot (Program::patchAnnotations -> patchInline), after infer and
    // buildAccessFlags, before lint and optimize. Splices are syntax-level: cloned callee
    // statements land in the caller, the pass reports astChanged, and the compiler restarts
    // infer, which re-resolves every ExprVar by name and legalizes types/r2v/temporaries.
    // Multi-return bodies splice: every function-level return in the CLONE becomes a
    // result store - terminal shapes in place, the rest through a generated bool flag
    // with break/guard plumbing (ReturnStoreRewrite; no goto - a label would disqualify
    // the whole caller from CSE and DSE).
    //
    // Three site kinds, two contracts:
    //  * [inline] calls (MUST): fail-closed. A callee shape the inliner can't splice is a
    //    compile error (annotation lint hook -> checkInlineShape, pointing at the
    //    declaration); a call site it can't host is a compile error reported here at the
    //    site. No size budgets, no decline paths. When a callee shape is bad the patch
    //    pass skips it SILENTLY and the lint hook reports it - both run on every compile,
    //    so a skip is never silent overall.
    //  * calls passing a block LITERAL argument (AUTO, optimized builds only): best-effort.
    //    The literal is the user's signal; nobody promised anything, so every gate -
    //    callee shape, [unsafe_operation]/[unsafe_deref], recursion through the splice
    //    graph, private symbols, call position - declines SILENTLY (counted, logged under
    //    log_optimization). `options disable_auto_inline` (or the policy) turns it off.
    //    With `options auto_inline_functions` (opt-in, same policy family) the AUTO
    //    tier also takes UNSIGNALED plain calls and operator sites whose callee is
    //    worth splicing: loop-free and within the `auto_inline_cost` node budget, or
    //    private and referenced exactly once (the body moves, it cannot grow the
    //    program). Generic instances stay out of this tier (explicit flavors and
    //    inferer-manufactured argument shapes are splice hazards - see
    //    autoEligibleCall). [never_inline] opts a function out of every best-effort
    //    tier; combining it with [inline] is an error.
    //    A plain unsafe body (hasUnsafe) splices: compiled callees lost their `unsafe { }`
    //    wrappers to foldUnsafe and would re-error on the caller's re-infer, so the spliced
    //    body rides the result-temp path under a generated statement-position ExprUnsafe -
    //    later-round splices anchor INSIDE that block, keeping every hoisted temp within
    //    its authorization region. The wrapper is generated: the no_unsafe policy and the
    //    caller module's unsafe accounting don't see it (the rewriter also drops
    //    userSaidItsSafe on callee-origin nodes - the callee's module already accounted).
    //  * invoke of a block literal (DEVIRT, same best-effort contract): an invoke whose
    //    block resolves to a same-function literal - directly or through a move-initialized,
    //    never-rewritten local (which is what an auto-spliced callee leaves behind) -
    //    splices the block body in place. Same-frame semantics make this a pure win: the
    //    body already reads the caller's locals directly.
    // The two halves converge over patch rounds: an auto splice moves the callee's invoke
    // next to the literal, the next round devirtualizes it, and any block-literal calls
    // inside the spliced body trigger further waves. Recursion through the combined graph
    // is declined up front, so the rounds terminate.
    //
    // Argument substitution is three-tier:
    //  (A) leaf args (constants; variable reads) substitute textually, no temp. A by-value
    //      param is a snapshot, so a leaf var the spliced body could write through another
    //      `var` by-ref param of the same call (or a global the callee's side effects could
    //      touch) binds a temp instead;
    //  (B) a pure (noSideEffects) non-leaf arg whose param is read at most once and not
    //      under a loop substitutes textually - evaluation can only become conditional
    //      (fewer evaluations of a pure expression), never repeated;
    //  (C) everything else binds a `let/var _inl<N>_arg_<param>` temp at the statement
    //      anchor in call-argument order: impure args evaluate exactly once in call order,
    //      multi-read params bind once, `var` by-value params ARE the mutable local copy.
    //
    // A call in a lazily- or repeatedly-evaluated position (&&/|| right side, ?: arms,
    // elif conditions, while conditions) that needs statements is handled by local
    // lowerings (while(C) -> while(true) if-break; elif -> blockified else; &&/||/?: ->
    // hoist + if-form). Each lowering changes the AST and converges over patch rounds.
    // Positions with no v1 lowering (?? right side, safe-navigation suffixes) error out.

    namespace {

        // ----- tiny callback walk (visitor without the boilerplate) -----

        template <typename TT>
        class LookupVisitor : public Visitor {
        public:
            LookupVisitor ( const TT & cb ) : callback(cb) {}
        protected:
            const TT & callback;
            virtual bool canVisitQuoteSubexpression ( ExprQuote * ) override { return false; }
            virtual void preVisitExpression ( Expression * expr ) override {
                Visitor::preVisitExpression(expr);
                callback(expr);
            }
        };

        template <typename TT>
        void lookupExpressions ( Expression * root, const TT & cb ) {
            LookupVisitor<TT> vis(cb);
            root->visit(vis);
        }

        // ----- callee shape -----

        class InlineShapeScan : public Visitor {
        public:
            InlineShapeScan ( bool blockLiteral = false ) : forBlockLiteral(blockLiteral) {}
            string reason;
            bool bad = false;
        protected:
            bool forBlockLiteral;
            int makeBlockDepth = 0;
            int loopDepth = 0;
            virtual bool canVisitQuoteSubexpression ( ExprQuote * ) override { return false; }
            void flag ( const char * why ) { if ( !bad ) { bad = true; reason = why; } }
            virtual void preVisit ( ExprGoto * expr ) override { Visitor::preVisit(expr); flag("body contains goto"); }
            virtual void preVisit ( ExprLabel * expr ) override { Visitor::preVisit(expr); flag("body contains a label"); }
            virtual void preVisit ( ExprTryCatch * expr ) override { Visitor::preVisit(expr); flag("body contains try/recover"); }
            virtual void preVisit ( ExprYield * expr ) override { Visitor::preVisit(expr); flag("body contains yield"); }
            virtual void preVisit ( ExprFor * expr ) override { Visitor::preVisit(expr); loopDepth ++; }
            virtual ExpressionPtr visit ( ExprFor * expr ) override { loopDepth --; return Visitor::visit(expr); }
            virtual void preVisit ( ExprWhile * expr ) override { Visitor::preVisit(expr); loopDepth ++; }
            virtual ExpressionPtr visit ( ExprWhile * expr ) override { loopDepth --; return Visitor::visit(expr); }
            // splicing a block body inline dissolves the block boundary; a break/continue
            // not bound to a loop inside the body would re-bind to the invoke site's loop
            virtual void preVisit ( ExprBreak * expr ) override {
                Visitor::preVisit(expr);
                if ( forBlockLiteral && !makeBlockDepth && !loopDepth ) flag("body breaks out of the block");
            }
            virtual void preVisit ( ExprContinue * expr ) override {
                Visitor::preVisit(expr);
                if ( forBlockLiteral && !makeBlockDepth && !loopDepth ) flag("body continues out of the block");
            }
            virtual void preVisit ( ExprMakeBlock * expr ) override {
                Visitor::preVisit(expr);
                // a plain block literal stays in-frame and clones safely with the body;
                // lambda and local-function literals lower to generated functions during
                // infer, and cloning those post-infer is not safe
                if ( expr->isLambda || expr->isLocalFunction || !expr->capture.empty() ) {
                    flag("body contains a lambda literal");
                }
                makeBlockDepth ++;
            }
            virtual ExpressionPtr visit ( ExprMakeBlock * expr ) override {
                makeBlockDepth --;
                return Visitor::visit(expr);
            }
            virtual void preVisit ( ExprMakeGenerator * expr ) override { Visitor::preVisit(expr); flag("body contains a generator"); }
            // assume aliases are function-scoped by name and are not renamed by the
            // splice: two spliced bodies (or a body and its caller) declaring the same
            // alias collide (30700, proven: llvm_jit_intrin's opType helpers)
            virtual void preVisit ( ExprAssume * expr ) override { Visitor::preVisit(expr); flag("body contains an assume expression"); }
        };

        bool isPlainIdentifier ( const string & name ) {
            if ( name.empty() ) return false;
            return isalpha(uint8_t(name[0])) || name[0]=='_';
        }

        // operator names whose call sites dispatch through ExprOp1/2/3 - the node kinds
        // the splicer plans. punctuation functions dispatched elsewhere ([] via ExprAt,
        // ?? via null-coalescing, . via properties, clone/finalize via their own nodes)
        // cannot honor the [inline] contract and stay refused
        bool exprOpDispatchedName ( const string & name ) {
            static const das_hash_set<string> ops = {
                // unary (ExprOp1); ++/-- pre, +++/--- post
                "!", "~", "++", "--", "+++", "---",
                // binary (ExprOp2); +/-/* /% shared with unary by arity
                "+", "-", "*", "/", "%", "&", "|", "^", "<<", ">>", "<<<", ">>>",
                "&&", "||", "^^", "==", "!=", ">", "<", ">=", "<=",
                "+=", "-=", "*=", "/=", "%=", "&=", "|=", "^=",
                "<<=", ">>=", "<<<=", ">>>=", "&&=", "||=", "^^=",
                // ternary (ExprOp3)
                "?"
            };
            return ops.find(name) != ops.end();
        }

        // ----- callee param read statistics (for tier B) -----

        struct ParamReadStats {
            das_hash_map<Variable *, int>   readCount;
            das_hash_set<Variable *>        readUnderLoop;
            // read as the argument of a REF parameter: such a read needs real storage, so the splice
            // must bind a temp rather than substitute a value. an implicit const-ref native parameter
            // is the case that bites - it cannot bind a bare value ("can't pass non-ref to ref")
            das_hash_set<Variable *>        readAsRefArg;
        };

        class ParamReadScan : public Visitor {
        public:
            ParamReadScan ( const vector<VariablePtr> & paramVars, ParamReadStats & st ) : stats(st) {
                for ( auto & arg : paramVars ) params.insert(arg);
            }
        protected:
            das_hash_set<Variable *> params;
            ParamReadStats & stats;
            int loopDepth = 0;
            virtual bool canVisitQuoteSubexpression ( ExprQuote * ) override { return false; }
            virtual void preVisit ( ExprFor * expr ) override { Visitor::preVisit(expr); loopDepth ++; }
            virtual ExpressionPtr visit ( ExprFor * expr ) override { loopDepth --; return Visitor::visit(expr); }
            virtual void preVisit ( ExprWhile * expr ) override { Visitor::preVisit(expr); loopDepth ++; }
            virtual ExpressionPtr visit ( ExprWhile * expr ) override { loopDepth --; return Visitor::visit(expr); }
            // a block literal's body may run any number of times - reads inside count as under-loop
            virtual void preVisit ( ExprMakeBlock * expr ) override { Visitor::preVisit(expr); loopDepth ++; }
            virtual ExpressionPtr visit ( ExprMakeBlock * expr ) override { loopDepth --; return Visitor::visit(expr); }
            virtual void preVisit ( ExprVar * expr ) override {
                Visitor::preVisit(expr);
                if ( expr->variable && params.find(expr->variable)!=params.end() ) {
                    stats.readCount[expr->variable] ++;
                    if ( loopDepth ) stats.readUnderLoop.insert(expr->variable);
                }
            }
            void notePos ( ExprCallFunc * call ) {
                if ( !call->func ) return;
                for ( size_t i=0, is=call->arguments.size(); i!=is && i!=call->func->arguments.size(); ++i ) {
                    auto & fa = call->func->arguments[i];
                    if ( !fa->type || !fa->type->isRef() ) continue;
                    Expression * a = call->arguments[i];
                    if ( a && a->rtti_isR2V() ) a = static_cast<ExprRef2Value *>(a)->subexpr;
                    if ( a && a->rtti_isVar() ) {
                        auto v = static_cast<ExprVar *>(a)->variable;
                        if ( v && params.find(v)!=params.end() ) stats.readAsRefArg.insert(v);
                    }
                }
            }
            virtual void preVisit ( ExprCall * expr ) override { Visitor::preVisit(expr); notePos(expr); }
            virtual void preVisit ( ExprOp1 * expr ) override { Visitor::preVisit(expr); notePos(expr); }
            virtual void preVisit ( ExprOp2 * expr ) override { Visitor::preVisit(expr); notePos(expr); }
        };

        // ----- cross-module reference scan (splice gate) -----
        // a spliced body re-resolves its calls and globals in the DESTINATION module.
        // references that stop resolving there fall in two classes. SCOPE needs -
        // private symbols, foreign-module generic instances, require-graph visibility -
        // re-resolve exactly under a generated `with (module <origin>)` wrapper, so
        // those sites splice wrapped. HARD stops no resolution scope can fix: an
        // `explicit`-flavored instance (typing soundness, not resolution - the ==const
        // clone family marks a body TYPED against a locked constness view), and
        // user-spelled `_::` / `__::` dispatch, which binds the PROGRAM module before
        // and after the splice - only bodies this program itself compiled keep their
        // meaning. locked instance names ("__::mod`fn`hash") are exempt from the
        // dispatch stop: the locked-name fallback re-resolves them through the origin
        // generic, and under the wrapper that origin is visible by construction

        struct CrossVerdict {
            string hard;        // symbol that stops the splice outright
            string hardWhy;     // ... with the reason to report
            string scope;       // symbol that needs the with (module) wrapper
        };

        class PrivateUseScan : public Visitor {
        public:
            PrivateUseScan ( Module * calleeModule, Module * destModule, bool programOwnsBody )
                : mod(calleeModule), dest(destModule), underscoreExempt(programOwnsBody) {}
            CrossVerdict verdict;
        protected:
            Module * mod;
            Module * dest;
            bool underscoreExempt;  // callee lives in the program module: its _:: already bound here
            void scopeNeed ( const string & name ) {
                if ( verdict.scope.empty() ) verdict.scope = name;
            }
            void checkName ( const string & name, bool generatedNode, Function * fn ) {
                if ( !verdict.hard.empty() || underscoreExempt ) return;
                size_t ofs = 0;
                if ( name.compare(0,4,"__::")==0 ) ofs = 4;
                else if ( name.compare(0,3,"_::")==0 ) ofs = 3;
                if ( !ofs ) return;
                // locked instance names ("__::mod`fn`hash") re-resolve through the
                // locked-name origin fallback and stay exempt - but that fallback reads
                // fromGeneric, and a constant_expression clone carries none. "_::" has no
                // such fallback: a mangled _:: name (a [template] product, class-method
                // dispatch) binds the program module before AND after the splice -
                // it stops the splice exactly like a user-spelled escape (proven:
                // sqlite_boost's ok() template products, sql tutorial dry-runs)
                if ( ofs==4 && name.find('`', ofs)!=string::npos && fn && fn->fromGeneric ) return;
                // machinery-manufactured _:: whose target re-resolves identically from
                // the destination stays exempt: public, non-generic, module visible
                // (coverage instrumentation - _::add_func_coverage in every body - is
                // the canonical case). compiler-emitted _::finalize / _::clone dispatch
                // resolves against the calling module's overloads BY DESIGN and gates
                // like a user escape (proven: templates_boost finalize, sql tutorials)
                if ( generatedNode && fn && !fn->privateFunction && !fn->fromGeneric
                    && fn->module && dest && dest->isVisibleDirectly(fn->module) ) return;
                verdict.hard = name;
                verdict.hardWhy = "body dispatches '" + name + "' at the call site's module";
            }
            void checkFunc ( Function * fn, const string & callName, bool generatedNode ) {
                checkName(callName, generatedNode, fn);
                if ( !fn || !verdict.hard.empty() ) return;
                auto origin = fn->fromGeneric ? fn->fromGeneric : fn;
                if ( fn->fromGeneric ) {
                    for ( auto & arg : fn->arguments ) {
                        if ( arg->type && (arg->type->explicitConst || arg->type->explicitRef) ) {
                            verdict.hard = origin->name;
                            verdict.hardWhy = "body calls explicit-flavored instance '" + origin->name + "'";
                            return;
                        }
                    }
                    // a foreign-module instance is reachable from the caller's re-infer
                    // only through the origin-generic fallback, which needs the origin's
                    // module visible - the wrapper's perspective, not the destination's
                    if ( fn->module!=dest ) scopeNeed(origin->name);
                }
                if ( fn->privateFunction || origin->privateFunction ) {
                    if ( fn->module!=dest && origin->module!=dest ) scopeNeed(origin->name);
                }
                if ( dest && origin->module && !dest->isVisibleDirectly(origin->module) ) {
                    scopeNeed(origin->name);
                }
            }
            virtual bool canVisitQuoteSubexpression ( ExprQuote * ) override { return false; }
            virtual void preVisit ( ExprCall * expr ) override {
                Visitor::preVisit(expr);
                checkFunc(expr->func, expr->name, expr->generated);
            }
            virtual void preVisit ( ExprOp1 * expr ) override {
                Visitor::preVisit(expr);
                checkFunc(expr->func, "", true);
            }
            virtual void preVisit ( ExprOp2 * expr ) override {
                Visitor::preVisit(expr);
                checkFunc(expr->func, "", true);
            }
            virtual void preVisit ( ExprOp3 * expr ) override {
                Visitor::preVisit(expr);
                checkFunc(expr->func, "", true);
            }
            virtual void preVisit ( ExprAddr * expr ) override {
                Visitor::preVisit(expr);
                checkFunc(expr->func, expr->target, expr->generated);
            }
            virtual void preVisit ( ExprVar * expr ) override {
                Visitor::preVisit(expr);
                if ( !expr->variable || !verdict.hard.empty() ) return;
                checkName(expr->name, expr->generated, nullptr);
                if ( expr->variable->private_variable ) {
                    if ( expr->variable->module!=dest ) scopeNeed(expr->variable->name);
                } else if ( dest && expr->isGlobalVariable() && expr->variable->module
                    && !dest->isVisibleDirectly(expr->variable->module) ) {
                    scopeNeed(expr->variable->name);
                }
            }
        };

        // ----- cloned-body fixup: rename locals, substitute parameter reads -----

        struct ArgSub {
            Expression *    substitute = nullptr;   // tier A/B: clone this at every read
            string          tempName;               // tier C: read this temp instead
        };

        // free variable names of an expression - used to decide which block-literal
        // arguments can actually capture a substituted caller expression
        // every declaration name in the caller: function arguments, lets, for iterators,
        // block-literal arguments. infer resolves LOCALS BEFORE BLOCK ARGUMENTS, so a spliced
        // can_shadow block argument (kept name, see LocalNameCollect) loses to any same-named
        // caller declaration - reads inside the block silently bind the caller's variable
        class DeclNameCollect : public Visitor {
        public:
            das_hash_set<string> * names = nullptr;
        protected:
            virtual bool canVisitQuoteSubexpression ( ExprQuote * ) override { return false; }
            void add ( const VariablePtr & var ) {
                names->insert(var->name);
                if ( !var->aka.empty() ) names->insert(var->aka);
            }
            virtual void preVisitLet ( ExprLet * let, const VariablePtr & var, bool last ) override {
                Visitor::preVisitLet(let, var, last);
                add(var);
            }
            virtual void preVisit ( ExprFor * expr ) override {
                Visitor::preVisit(expr);
                for ( auto & var : expr->iteratorVariables ) add(var);
            }
            virtual void preVisitBlockArgument ( ExprBlock * block, const VariablePtr & var, bool lastArg ) override {
                Visitor::preVisitBlockArgument(block, var, lastArg);
                add(var);
            }
        };

        class FreeNameCollect : public Visitor {
        public:
            das_hash_set<string> * names = nullptr;
        protected:
            virtual bool canVisitQuoteSubexpression ( ExprQuote * ) override { return false; }
            virtual void preVisit ( ExprVar * expr ) override {
                Visitor::preVisit(expr);
                names->insert(expr->name);
            }
        };

        // the callee's local declarations, so reads can be renamed by name
        // (infer re-resolves every ExprVar by name; consistent renaming is all it takes)
        class LocalNameCollect : public Visitor {
        public:
            das_hash_map<string, string> * rename = nullptr;
            // can_shadow block-argument names are semantic (a host ECS resolves the component
            // by the argument name), so they are never renamed; the splice DECLINES instead
            // when a substituted caller expression could be captured by one (see call sites).
            // collected here so the caller can run that check.
            das_hash_set<string> * canShadowArgs = nullptr;
            string prefix;
        protected:
            virtual bool canVisitQuoteSubexpression ( ExprQuote * ) override { return false; }
            void renameVar ( const string & name, const string & aka ) {
                if ( rename->find(name)==rename->end() ) (*rename)[name] = prefix + name;
                if ( !aka.empty() && rename->find(aka)==rename->end() ) (*rename)[aka] = prefix + name;
            }
            virtual void preVisitLet ( ExprLet * let, const VariablePtr & var, bool last ) override {
                Visitor::preVisitLet(let, var, last);
                renameVar(var->name, var->aka);
            }
            virtual void preVisit ( ExprFor * expr ) override {
                Visitor::preVisit(expr);
                for ( auto & var : expr->iteratorVariables ) renameVar(var->name, var->aka);
            }
            // block-literal arguments rename too: after the splice they sit in the caller's
            // scope chain, and an unrenamed argument could shadow a caller local (an error)
            virtual void preVisitBlockArgument ( ExprBlock * block, const VariablePtr & var, bool lastArg ) override {
                Visitor::preVisitBlockArgument(block, var, lastArg);
                // a plain argument must rename: after the splice it can shadow a caller-side
                // name, which is 30701. a can_shadow argument keeps its (semantic) name.
                if ( var->can_shadow ) {
                    if ( canShadowArgs ) {
                        canShadowArgs->insert(var->name);
                        if ( !var->aka.empty() ) canShadowArgs->insert(var->aka);
                    }
                    return;
                }
                renameVar(var->name, var->aka);
            }
        };

        // rewrites a cloned callee subtree for splicing:
        //  * parameter reads become the planned substitution or a temp read
        //  * local declarations and their reads get the _inl<N>_ prefix
        //  * r2v flags are cleared: a callee from another module is post-optimize
        //    (ref folding turned r2v wrappers into flags), and the flag would survive
        //    re-infer inconsistently - infer re-derives value reads from scratch
        class InlineBodyRewriter : public Visitor {
        public:
            das_hash_map<Variable *, ArgSub> * paramSub = nullptr;  // keyed by ORIGINAL callee param
            das_hash_map<string, string> * rename = nullptr;
            LineInfo tempAt;    // call site location for manufactured temp reads
            bool clearUserUnsafe = false;   // splicing a function callee (not a devirt literal)
        protected:
            // scope gate for the name-based rename. the map holds every declaration in the body,
            // including nested block-literal arguments, but a rename may only apply to references
            // under that declaration's lexical scope: a reference bound OUTSIDE the body (caller
            // code around a devirted literal) can spell the same name - a can_shadow block argument
            // makes that legal - and renaming it points it at a variable that does not exist there.
            // per-name declaration stack; true = renamed (the rename applies within its scope),
            // false = KEPT - a can_shadow block argument keeps its (semantic) name and must
            // SHIELD it: references under the argument's scope bind the argument, so an outer
            // renamed declaration of the same name may not capture them. innermost entry wins.
            das_hash_map<string, vector<bool>> activeDecl;
            vector<vector<string>> declFrames { {} };
            void activateDecl ( const string & name, bool renamed ) {
                activeDecl[name].push_back(renamed);
                declFrames.back().push_back(name);
            }
            void pushDeclFrame () { declFrames.emplace_back(); }
            void popDeclFrame () {
                for ( auto & n : declFrames.back() ) activeDecl[n].pop_back();
                declFrames.pop_back();
            }
            bool renameActive ( const string & name ) const {
                auto it = activeDecl.find(name);
                return it!=activeDecl.end() && !it->second.empty() && it->second.back();
            }
            virtual bool canVisitQuoteSubexpression ( ExprQuote * ) override { return false; }
            virtual void preVisitExpression ( Expression * expr ) override {
                Visitor::preVisitExpression(expr);
                // callee-origin `unsafe(...)` stays authorized via alwaysSafe; dropping the
                // user-said bit keeps the CALLER module's unsafe accounting (lint anyUnsafe,
                // no_unsafe policy) unchanged by the splice - the callee's module already
                // accounted for its own unsafe. substituted caller expressions are grafted
                // after this visit and keep theirs
                if ( clearUserUnsafe ) expr->userSaidItsSafe = false;
            }
            virtual ExpressionPtr visit ( ExprVar * expr ) override {
                // cloned ExprVar nodes still point at the ORIGINAL callee variables
                // (ExprVar::clone copies the raw pointer) - param matching is exact
                if ( expr->variable ) {
                    auto it = paramSub->find(expr->variable);
                    if ( it != paramSub->end() ) {
                        if ( it->second.substitute ) return it->second.substitute->clone();
                        return new ExprVar(tempAt, it->second.tempName);
                    }
                }
                auto rit = rename->find(expr->name);
                if ( rit != rename->end() && renameActive(expr->name) ) {
                    expr->name = rit->second;
                    expr->variable = nullptr;   // infer re-resolves by name
                }
                expr->r2v = false;
                return Visitor::visit(expr);
            }
            virtual ExpressionPtr visit ( ExprRef2Value * expr ) override {
                // a substituted value expression under an r2v wrapper leaves R2V(value);
                // drop the wrapper, the subexpression is already a value
                if ( expr->subexpr && expr->subexpr->type && !expr->subexpr->type->ref ) {
                    return expr->subexpr;
                }
                return Visitor::visit(expr);
            }
            virtual void preVisit ( ExprField * expr ) override { Visitor::preVisit(expr); expr->r2v = false; }
            virtual void preVisit ( ExprSafeField * expr ) override { Visitor::preVisit(expr); expr->r2v = false; }
            virtual void preVisit ( ExprAsVariant * expr ) override { Visitor::preVisit(expr); expr->r2v = false; }
            virtual void preVisit ( ExprSafeAsVariant * expr ) override { Visitor::preVisit(expr); expr->r2v = false; }
            virtual void preVisit ( ExprSwizzle * expr ) override { Visitor::preVisit(expr); expr->r2v = false; }
            virtual void preVisit ( ExprAt * expr ) override { Visitor::preVisit(expr); expr->r2v = false; }
            virtual void preVisit ( ExprSafeAt * expr ) override { Visitor::preVisit(expr); expr->r2v = false; }
            virtual void preVisitLet ( ExprLet * let, const VariablePtr & var, bool last ) override {
                Visitor::preVisitLet(let, var, last);
                auto rit = rename->find(var->name);
                if ( rit != rename->end() ) { activateDecl(var->name, true); var->name = rit->second; }
                if ( !var->aka.empty() ) {
                    auto ait = rename->find(var->aka);
                    if ( ait != rename->end() ) { activateDecl(var->aka, true); var->aka = ait->second; }
                }
                // a compiled (cross-module) callee has been through allocateStack, which
                // aliases the returned local's storage to the function's CMRES slot. the
                // spliced copy lives in the CALLER's frame, where that aliasing is stale -
                // simulate would write the local through the caller's (possibly null)
                // cmres. present virgin state; the caller's own allocation re-derives
                var->aliasCMRES = false;
            }
            virtual void preVisit ( ExprFor * expr ) override {
                Visitor::preVisit(expr);
                pushDeclFrame();
                for ( size_t i=0, is=expr->iterators.size(); i!=is; ++i ) {
                    auto rit = rename->find(expr->iterators[i]);
                    if ( rit != rename->end() ) { activateDecl(expr->iterators[i], true); expr->iterators[i] = rit->second; }
                }
                for ( auto & var : expr->iteratorVariables ) {
                    auto rit = rename->find(var->name);
                    if ( rit != rename->end() ) var->name = rit->second;
                }
            }
            virtual ExpressionPtr visit ( ExprFor * expr ) override {
                popDeclFrame();
                return Visitor::visit(expr);
            }
            virtual void preVisit ( ExprBlock * block ) override {
                Visitor::preVisit(block);
                pushDeclFrame();
            }
            virtual ExpressionPtr visit ( ExprBlock * block ) override {
                popDeclFrame();
                return Visitor::visit(block);
            }
            virtual void preVisitBlockArgument ( ExprBlock * block, const VariablePtr & var, bool lastArg ) override {
                Visitor::preVisitBlockArgument(block, var, lastArg);
                if ( !block->annotations.empty() && !var->isAccessUnused() ) var->marked_used = true;
                // a can_shadow argument keeps its (semantic) name even when a same-named
                // renamable declaration put that name in the map - the map is name-keyed
                // and cannot tell them apart, the flag on the variable can
                if ( var->can_shadow ) {
                    activateDecl(var->name, false);
                    if ( !var->aka.empty() ) activateDecl(var->aka, false);
                    return;
                }
                auto rit = rename->find(var->name);
                if ( rit != rename->end() ) { activateDecl(var->name, true); var->name = rit->second; }
                else { activateDecl(var->name, false); }
                if ( !var->aka.empty() ) {
                    auto ait = rename->find(var->aka);
                    if ( ait != rename->end() ) { activateDecl(var->aka, true); var->aka = ait->second; }
                    else { activateDecl(var->aka, false); }
                }
            }
        };

        // replace one specific node (by identity) inside a statement
        class ReplaceNode : public Visitor {
        public:
            Expression * what = nullptr;
            ExpressionPtr with = nullptr;
            bool done = false;
        protected:
            virtual ExpressionPtr visitExpression ( Expression * expr ) override {
                if ( expr==what ) { done = true; return with; }
                return Visitor::visitExpression(expr);
            }
        };

        // ----- statement anchors (same scheme as the CSE prescan) -----

        struct StmtAnchor {
            ExprBlock * block = nullptr;
            int         index = -1;
        };

        enum class SiteKind {
            MustCall,   // call to an [inline] function: fail-closed contract, errors
            AutoCall,   // call with a block-literal argument: best-effort, silent declines
            Devirt      // invoke of a block literal (direct or let-bound): best-effort
        };

        struct PlannedSite {
            Expression * callLike = nullptr;    // ExprCall or ExprInvoke
            Expression * stmt = nullptr;
            StmtAnchor  anchor;
            SiteKind    kind = SiteKind::MustCall;
            string      withModule;             // innermost `with (module ...)` at the site
        };

        // annotations other than inert markers may carry call-site semantics (verifyCall,
        // transform, per-call codegen) that splicing the call away would bypass
        bool annotatedCallee ( Function * fn, string & why ) {
            for ( auto & ann : fn->annotations ) {
                if ( !ann->annotation ) continue;
                auto & nm = ann->annotation->name;
                if ( nm=="export" || nm=="unused_argument" ) continue;
                why = "annotated function [" + nm + "]";
                return true;
            }
            return false;
        }

        // ----- worthiness metric (heuristic plain-call candidacy) -----

        struct BodyCost {
            int     nodes = 0;
            bool    hasLoop = false;
            int64_t stackBytes = 0;     // sum of local let sizes - the frame the body owns
        };

        BodyCost bodyCost ( Expression * body ) {
            BodyCost bc;
            lookupExpressions(body, [&](Expression * e) {
                bc.nodes ++;
                if ( e->rtti_isFor() || e->rtti_isWhile() ) bc.hasLoop = true;
                if ( e->rtti_isLet() ) {
                    auto let = static_cast<ExprLet *>(e);
                    for ( auto & v : let->variables ) {
                        if ( v->type && !v->type->isAutoOrAlias() ) bc.stackBytes += v->type->getSizeOf64();
                    }
                }
            });
            return bc;
        }

        // splicing rehomes callee locals into the caller frame PERMANENTLY (spliced
        // slots are not reused across sites), so unbounded splicing inflates frames
        // past documented `options stack` contracts (proven: dasLLAMA's respond()
        // chain overflowed its documented 64K under default-on). three bounds:
        // a callee owning more than CALLEE_STACK bytes of locals is not worth a
        // heuristic splice; a caller whose frame already holds more than
        // CALLER_FRAME bytes gets no heuristic sites; and one round may add at
        // most CALLER_GROWTH bytes to any single caller
        constexpr int64_t AUTO_INLINE_CALLEE_STACK_BYTES  = 512;
        constexpr int64_t AUTO_INLINE_CALLER_FRAME_BYTES  = 4096;
        constexpr int64_t AUTO_INLINE_CALLER_GROWTH_BYTES = 2048;

        // best-effort candidacy configuration for one patch round. blockLiterals is the
        // pre-existing auto tier (block-literal call sites + devirt); functions is the
        // opt-in heuristic tier over plain calls (auto_inline_functions), bounded by the
        // node budget (auto_inline_cost). both caches live for one round - splices
        // change bodies, so nothing carries across rounds
        struct AutoInlineCfg {
            bool blockLiterals = false;
            bool functions = false;
            int  budget = 32;
            Module * thisModule = nullptr;              // heuristic tier is same-module-only
            das_hash_map<Function *, bool> * worthCache = nullptr;
            das_hash_set<Function *> * budgetExempt = nullptr;  // private, referenced exactly once
        };

        // worth splicing unsignaled: small enough that per-site growth stays bounded,
        // and loop-free - a loop body's runtime dwarfs the call overhead while the
        // splice inflates code. a private called-exactly-once callee is exempt from
        // both: its body MOVES rather than duplicates (removeUnusedSymbols reaps the
        // husk), so size cannot grow no matter what it holds
        bool worthAutoInline ( Function * fn, const AutoInlineCfg & cfg ) {
            if ( cfg.budgetExempt && cfg.budgetExempt->find(fn)!=cfg.budgetExempt->end() ) return true;
            if ( !cfg.worthCache ) return false;
            auto it = cfg.worthCache->find(fn);
            if ( it != cfg.worthCache->end() ) return it->second;
            BodyCost bc = bodyCost(fn->body);
            bool ok = !bc.hasLoop && bc.nodes <= cfg.budget
                && bc.stackBytes <= AUTO_INLINE_CALLEE_STACK_BYTES;
            (*cfg.worthCache)[fn] = ok;
            return ok;
        }

        // the unsignaled heuristic tier, per callee. generic INSTANCES stay out (the
        // signaled block-literal tier keeps them, behind its proven flavor gates):
        // instances carry splice hazards plain functions can't have - explicit
        // const/ref-locked parameter flavors, and inferer-manufactured argument
        // shapes: to_array_move's heap-mode array literal spliced into a generated
        // `let <-` re-infers under the in-place make-local protocol and corrupts
        // the array header (probe-verified: garbage sums, SIGSEGV)
        bool autoEligibleFn ( Function * fn, const AutoInlineCfg & cfg ) {
            if ( !fn || fn->mustInline || fn->builtIn ) return false;
            if ( !fn->body || fn->isTemplate ) return false;
            if ( fn->neverInline ) return false;    // explicit opt-out, both tiers
            // macro-MANUFACTURED functions (qmacro products, helper factories,
            // generated finalizers) carry bespoke resolution and shape invariants
            // the splicer can't see - they never splice into callers. macros can
            // also mark manufactured functions [never_inline] explicitly
            if ( fn->generated ) return false;
            if ( !cfg.functions || fn->fromGeneric ) return false;
            // same-module only: a transplanted body is NOT context-free by language
            // design - `_::` dispatch (clone/finalize, every `:=`/`delete`) resolves in
            // the calling module deliberately, and infer's name-based probes (generic
            // operators, the jit `.[]` handle-index probe) consult the destination
            // module's visible-function set. Cross-module inlining is the author's
            // explicit [inline] contract, not a heuristic's call
            if ( fn->module != cfg.thisModule ) return false;
            return worthAutoInline(fn, cfg);
        }

        bool autoEligibleCall ( ExprCall * call, const AutoInlineCfg & cfg ) {
            if ( !call->func || call->func->mustInline || call->func->builtIn ) return false;
            if ( !call->func->body || call->func->isTemplate ) return false;
            if ( call->func->neverInline ) return false;    // explicit opt-out, both tiers
            if ( cfg.blockLiterals ) {
                for ( auto & a : call->arguments ) {
                    if ( a->rtti_isMakeBlock() ) return true;
                }
            }
            return autoEligibleFn(call->func, cfg);
        }

        // a call-like splice edge: a direct call, or an operator-overload site. eligibility
        // for the auto tier differs by node kind (block-literal candidacy is per-call), so
        // the walkers test with autoEligibleCall for calls and autoEligibleFn for ops.
        // rtti_isOp2 is true for copy/move/clone too - their func, when set at all, never
        // passes the eligibility gates, so the over-approximation is harmless
        Function * callLikeFunc ( Expression * expr ) {
            if ( expr->rtti_isCall() ) return static_cast<ExprCall *>(expr)->func;
            if ( expr->rtti_isOp1() || expr->rtti_isOp2() || expr->rtti_isOp3() ) {
                return static_cast<ExprCallFunc *>(expr)->func;
            }
            return nullptr;
        }

        // one walk per function: for every statement its (block, index) anchor, and
        // every splice site tagged with the statement that anchors it. a call in a
        // while/elif condition is tagged with the while/if statement itself; the
        // splice step lowers those constructs first when statements are needed.
        class InlineCollect : public Visitor {
        public:
            InlineCollect ( const AutoInlineCfg & cfg ) : autoCfg(cfg) {}
            vector<PlannedSite> sites;      // in visit order: within a block, increasing index
        protected:
            const AutoInlineCfg & autoCfg;
            vector<StmtAnchor> blockStack;
            vector<string> withModules;     // enclosing module-flavored with scopes
            virtual bool canVisitQuoteSubexpression ( ExprQuote * ) override { return false; }
            virtual void preVisit ( ExprBlock * blk ) override {
                Visitor::preVisit(blk);
                blockStack.push_back(StmtAnchor{blk, -1});
            }
            virtual ExpressionPtr visit ( ExprBlock * blk ) override {
                blockStack.pop_back();
                return Visitor::visit(blk);
            }
            virtual void preVisit ( ExprWith * expr ) override {
                Visitor::preVisit(expr);
                if ( expr->isModuleWith() ) withModules.push_back(expr->moduleName);
            }
            virtual ExpressionPtr visit ( ExprWith * expr ) override {
                if ( expr->isModuleWith() ) withModules.pop_back();
                return Visitor::visit(expr);
            }
            virtual void preVisitBlockExpression ( ExprBlock * block, Expression * expr ) override {
                Visitor::preVisitBlockExpression(block, expr);
                if ( !blockStack.empty() && blockStack.back().block==block ) blockStack.back().index++;
            }
            void plan ( Expression * expr, SiteKind kind ) {
                // the anchoring statement is the innermost open block's CURRENT
                // statement - not the last statement any nested block visited
                // (an elif condition visits after the outer if's then-block)
                PlannedSite site;
                site.callLike = expr;
                site.kind = kind;
                if ( !withModules.empty() ) site.withModule = withModules.back();
                if ( !blockStack.empty() && blockStack.back().index >= 0
                    && blockStack.back().index < int(blockStack.back().block->list.size()) ) {
                    site.anchor = blockStack.back();
                    site.stmt = site.anchor.block->list[site.anchor.index];
                }
                sites.push_back(site);
            }
            virtual void preVisit ( ExprCall * expr ) override {
                Visitor::preVisit(expr);
                if ( expr->func && expr->func->mustInline ) {
                    plan(expr, SiteKind::MustCall);
                } else if ( autoEligibleCall(expr, autoCfg) ) {
                    plan(expr, SiteKind::AutoCall);
                }
            }
            // operator-overload sites. dispatch is exact-typed, so these never fire
            // for ExprCopy/ExprMove/ExprClone (which derive from ExprOp2)
            void planOp ( ExprCallFunc * expr ) {
                if ( expr->func && expr->func->mustInline ) {
                    plan(expr, SiteKind::MustCall);
                } else if ( autoEligibleFn(expr->func, autoCfg) ) {
                    plan(expr, SiteKind::AutoCall);
                }
            }
            virtual void preVisit ( ExprOp1 * expr ) override { Visitor::preVisit(expr); planOp(expr); }
            virtual void preVisit ( ExprOp2 * expr ) override { Visitor::preVisit(expr); planOp(expr); }
            virtual void preVisit ( ExprOp3 * expr ) override { Visitor::preVisit(expr); planOp(expr); }
            virtual void preVisit ( ExprInvoke * expr ) override {
                Visitor::preVisit(expr);
                if ( !autoCfg.blockLiterals || expr->isInvokeMethod || expr->arguments.empty() ) return;
                Expression * a0 = expr->arguments[0];
                if ( a0->rtti_isR2V() ) a0 = static_cast<ExprRef2Value *>(a0)->subexpr;
                if ( a0->rtti_isMakeBlock() || a0->rtti_isVar() ) plan(expr, SiteKind::Devirt);
            }
        };

        // let-bound block literals: a variable move-initialized from a block literal and
        // never REBOUND still holds that literal at every invoke. write flags are fresh
        // at patch time (buildAccessFlags runs right before patchAnnotations); an invoke
        // stamps a write on its non-const block argument, but invoking cannot rebind the
        // holder to a different literal, so arg0 occurrences stay clean for this purpose.
        // each binding remembers its module-with context: a literal is caller code and
        // must keep resolving where it was written - devirt at an invoke under a
        // DIFFERENT `with (module ...)` scope would re-home it, so those sites decline
        struct BlockBinding {
            ExprMakeBlock * literal = nullptr;
            string          withModule;
        };
        class BlockBindingScan : public Visitor {
        public:
            das_hash_map<Variable *, BlockBinding> binding;
            das_hash_set<Variable *> disq;
        protected:
            das_hash_set<Expression *> invokeArg0;
            vector<string> withModules;
            virtual bool canVisitQuoteSubexpression ( ExprQuote * ) override { return false; }
            virtual void preVisit ( ExprWith * expr ) override {
                Visitor::preVisit(expr);
                if ( expr->isModuleWith() ) withModules.push_back(expr->moduleName);
            }
            virtual ExpressionPtr visit ( ExprWith * expr ) override {
                if ( expr->isModuleWith() ) withModules.pop_back();
                return Visitor::visit(expr);
            }
            virtual void preVisitLet ( ExprLet * let, const VariablePtr & var, bool last ) override {
                Visitor::preVisitLet(let, var, last);
                if ( var->init && var->init->rtti_isMakeBlock() ) {
                    if ( binding.find(var)!=binding.end() ) disq.insert(var);  // paranoia: one binding only
                    binding[var] = BlockBinding{static_cast<ExprMakeBlock *>(var->init),
                        withModules.empty() ? string() : withModules.back()};
                }
            }
            virtual void preVisit ( ExprInvoke * expr ) override {
                Visitor::preVisit(expr);
                if ( expr->arguments.empty() ) return;
                Expression * a0 = expr->arguments[0];
                if ( a0->rtti_isR2V() ) a0 = static_cast<ExprRef2Value *>(a0)->subexpr;
                if ( a0->rtti_isVar() ) invokeArg0.insert(a0);
            }
            virtual void preVisit ( ExprVar * expr ) override {
                Visitor::preVisit(expr);
                if ( expr->variable && expr->write
                    && invokeArg0.find(expr)==invokeArg0.end() ) disq.insert(expr->variable);
            }
        };

        // ----- eager-position and prefix analysis within one statement -----

        // children of `e` in evaluation order. lazyKind: 0 = evaluated eagerly exactly
        // once, 1 = lazy with a v1 lowering (&&, ||, ?:), 2 = lazy without one (??,
        // safe navigation). statement kinds list only their same-statement expressions
        // (an if statement lists its condition; nested blocks anchor their own statements)
        void childrenInEvalOrder ( Expression * e, vector<pair<Expression *,int>> & out ) {
            out.clear();
            if ( e->rtti_isOp3() ) {
                // a user-overloaded op3 evaluates as a call - every operand eager;
                // only the builtin form selects its arms lazily
                auto op3 = static_cast<ExprOp3 *>(e);
                int lazyArm = (!op3->func || op3->func->builtIn) ? 1 : 0;
                out.emplace_back(op3->subexpr, 0);
                out.emplace_back(op3->left, lazyArm);
                out.emplace_back(op3->right, lazyArm);
            } else if ( e->rtti_isOp2() ) {
                // covers copy/move/clone too: destination address first, then the value.
                // &&/|| (and their &&=/||= assignment forms) short-circuit only in
                // builtin form - a user overload is a call
                auto op2 = static_cast<ExprOp2 *>(e);
                bool lazyRight = (op2->op=="&&" || op2->op=="||"
                        || op2->op=="&&=" || op2->op=="||=")
                    && (!op2->func || op2->func->builtIn);
                out.emplace_back(op2->left, 0);
                out.emplace_back(op2->right, lazyRight ? 1 : 0);
            } else if ( e->rtti_isNullCoalescing() ) {
                auto nc = static_cast<ExprNullCoalescing *>(e);
                out.emplace_back(nc->subexpr, 0);
                out.emplace_back(nc->defaultValue, 2);
            } else if ( e->rtti_isSafeField() ) {
                out.emplace_back(static_cast<ExprSafeField *>(e)->value, 0);
            } else if ( e->rtti_isSafeAt() ) {
                auto sa = static_cast<ExprSafeAt *>(e);
                out.emplace_back(sa->subexpr, 0);
                out.emplace_back(sa->index, 2);
            } else if ( e->rtti_isOp1() ) {
                out.emplace_back(static_cast<ExprOp1 *>(e)->subexpr, 0);
            } else if ( e->rtti_isR2V() ) {
                out.emplace_back(static_cast<ExprRef2Value *>(e)->subexpr, 0);
            } else if ( e->rtti_isCall() ) {
                auto c = static_cast<ExprCall *>(e);
                for ( auto & a : c->arguments ) out.emplace_back(a, 0);
            } else if ( e->rtti_isInvoke() ) {
                auto inv = static_cast<ExprInvoke *>(e);
                for ( auto & a : inv->arguments ) out.emplace_back(a, 0);
            } else if ( e->rtti_isStringBuilder() ) {
                auto sb = static_cast<ExprStringBuilder *>(e);
                for ( auto & a : sb->elements ) out.emplace_back(a, 0);
            } else if ( e->rtti_isReturn() ) {
                auto r = static_cast<ExprReturn *>(e);
                if ( r->subexpr ) out.emplace_back(r->subexpr, 0);
            } else if ( e->rtti_isLet() ) {
                auto let = static_cast<ExprLet *>(e);
                for ( auto & v : let->variables ) {
                    if ( v->init ) out.emplace_back(v->init, 0);
                }
            } else if ( e->rtti_isIfThenElse() ) {
                out.emplace_back(static_cast<ExprIfThenElse *>(e)->cond, 0);
            } else if ( e->rtti_isFor() ) {
                auto f = static_cast<ExprFor *>(e);
                for ( auto & s : f->sources ) out.emplace_back(s, 0);
            } else if ( e->rtti_isWith() ) {
                auto wi = static_cast<ExprWith *>(e);
                if ( wi->with ) out.emplace_back(wi->with, 0);
            } else if ( e->rtti_isField() ) {
                out.emplace_back(static_cast<ExprField *>(e)->value, 0);
            } else if ( e->rtti_isAt() ) {
                auto at = static_cast<ExprAt *>(e);
                out.emplace_back(at->subexpr, 0);
                out.emplace_back(at->index, 0);
            } else if ( e->rtti_isCast() ) {
                out.emplace_back(static_cast<ExprCast *>(e)->subexpr, 0);
            } else if ( e->rtti_isUnsafe() ) {
                out.emplace_back(static_cast<ExprUnsafe *>(e)->body, 0);
            }
        }

        // mirrors lint's 30250 detection (ast_lint.cpp preVisit(ExprAt)): two non-deref
        // ref lookups into the same table - by textual identity - within one statement.
        // lint runs AFTER the patch slot, so splicing a call out of such a statement
        // would erase the diagnostic while keeping the UB; those sites decline instead.
        // scope matches lint's: same-statement expressions only (childrenInEvalOrder),
        // nested block statements are their own lint scope
        void scanStatementTableAts ( Expression * e, das_hash_set<uint64_t> & seen, bool & collides ) {
            if ( !e || collides ) return;
            if ( e->rtti_isAt() ) {
                auto at = static_cast<ExprAt *>(e);
                if ( !at->underDeref && at->subexpr->type && at->subexpr->type->isGoodTableType() ) {
                    auto h = hash64z(at->subexpr->describe().c_str());
                    if ( !seen.insert(h).second ) { collides = true; return; }
                }
            }
            vector<pair<Expression *,int>> kids;
            childrenInEvalOrder(e, kids);
            for ( auto & k : kids ) scanStatementTableAts(k.first, seen, collides);
        }
        bool statementHasTableLookupCollision ( Expression * stmt ) {
            das_hash_set<uint64_t> seen;
            bool collides = false;
            scanStatementTableAts(stmt, seen, collides);
            return collides;
        }

        // conservative purity for tier-B substitution and prefix hoisting. the
        // per-expression noSideEffects flags are computed by checkSideEffects()
        // during optimize - at patch time they are stale or unset - but function
        // level sideEffectFlags ARE fresh (buildAccessFlags runs right before the
        // patch slot), so derive purity from the tree + callee flags directly.
        // a global READ counts as impure: reordering it across a global write is
        // observable, and both tier B and prefix hoisting move evaluation
        bool inlinePure ( Expression * e ) {
            if ( !e ) return true;
            if ( e->rtti_isConstant() ) return true;
            if ( e->rtti_isMakeBlock() ) return true;   // making a block writes nothing (and
                                                        // hoisting one would copy a non-copyable)
            if ( e->rtti_isTypeDecl() ) return true;    // type<...> witness: a compile-time tag
            if ( e->rtti_isUnsafe() ) return inlinePure(static_cast<ExprUnsafe *>(e)->body);
            if ( e->rtti_isVar() ) {
                // a bare (non-R2V) variable is an LVALUE - a reference to storage,
                // not a value read. Storage identity is stable for BOTH a local and
                // a global (global storage never moves), so referencing one is pure;
                // hoisting a global array/struct being indexed would only make a
                // pointless reference temp - and for a global array that temp is a
                // local dynamic array the shader backends cannot represent at all.
                // A global's VALUE read (which CAN change mid-inline) stays impure at
                // the R2V case below, where it is snapshotted.
                return true;
            }
            if ( e->rtti_isR2V() ) {
                auto sub = static_cast<ExprRef2Value *>(e)->subexpr;
                // a global's value can be mutated between the inline site and the
                // spliced body, so its rvalue read is snapshotted (kept impure)
                if ( sub && sub->rtti_isVar() ) {
                    auto ev = static_cast<ExprVar *>(sub);
                    if ( (ev->variable || ev->type) && ev->isGlobalVariable() ) return false;
                }
                return inlinePure(sub);
            }
            if ( e->rtti_isField() ) return inlinePure(static_cast<ExprField *>(e)->value);
            if ( e->rtti_isSafeField() ) return inlinePure(static_cast<ExprSafeField *>(e)->value);
            if ( e->rtti_isAsVariant() ) return inlinePure(static_cast<ExprAsVariant *>(e)->value);
            if ( e->rtti_isSafeAsVariant() ) return inlinePure(static_cast<ExprSafeAsVariant *>(e)->value);
            if ( e->rtti_isSwizzle() ) return inlinePure(static_cast<ExprSwizzle *>(e)->value);
            if ( e->rtti_isCast() ) return inlinePure(static_cast<ExprCast *>(e)->subexpr);
            if ( e->rtti_isAt() ) {
                auto at = static_cast<ExprAt *>(e);
                return inlinePure(at->subexpr) && inlinePure(at->index);
            }
            if ( e->rtti_isSafeAt() ) {
                auto at = static_cast<ExprSafeAt *>(e);
                return inlinePure(at->subexpr) && inlinePure(at->index);
            }
            if ( e->rtti_isNullCoalescing() ) {
                auto nc = static_cast<ExprNullCoalescing *>(e);
                return inlinePure(nc->subexpr) && inlinePure(nc->defaultValue);
            }
            const uint32_t impureMask =
                  uint32_t(SideEffects::userScenario)
                | uint32_t(SideEffects::modifyExternal)
                | uint32_t(SideEffects::modifyArgument)
                | uint32_t(SideEffects::accessGlobal)
                | uint32_t(SideEffects::invoke);
            if ( e->rtti_isOp1() ) {
                auto op = static_cast<ExprOp1 *>(e);
                if ( op->func && (op->func->sideEffectFlags & impureMask) ) return false;
                return inlinePure(op->subexpr);
            }
            if ( e->rtti_isOp2() ) {
                auto op = static_cast<ExprOp2 *>(e);
                if ( op->func && (op->func->sideEffectFlags & impureMask) ) return false;
                return inlinePure(op->left) && inlinePure(op->right);
            }
            if ( e->rtti_isOp3() ) {
                auto op = static_cast<ExprOp3 *>(e);
                if ( op->func && (op->func->sideEffectFlags & impureMask) ) return false;
                return inlinePure(op->subexpr) && inlinePure(op->left) && inlinePure(op->right);
            }
            if ( e->rtti_isCall() ) {
                auto c = static_cast<ExprCall *>(e);
                if ( !c->func || (c->func->sideEffectFlags & impureMask) ) return false;
                for ( auto & a : c->arguments ) {
                    if ( !inlinePure(a) ) return false;
                }
                return true;
            }
            return false;   // unrecognized - conservatively impure
        }

        // a callee that writes nothing (no var by-ref params, no external state, no
        // globals - accessGlobal does not distinguish reads from writes - no invokes)
        // cannot invalidate anything an argument expression read - textual
        // substitution is then order-safe for any pure argument
        bool calleeWriteFree ( Function * fn ) {
            const uint32_t writeMask =
                  uint32_t(SideEffects::userScenario)
                | uint32_t(SideEffects::modifyExternal)
                | uint32_t(SideEffects::modifyArgument)
                | uint32_t(SideEffects::accessGlobal)
                | uint32_t(SideEffects::invoke);
            return (fn->sideEffectFlags & writeMask)==0;
        }

        // the arguments and parameters of one splice site: a direct call binds the whole
        // argument list, an invoke binds arguments[1..] against the block's parameters
        struct SiteArgs {
            const vector<ExpressionPtr> *  args = nullptr;
            size_t                         ofs = 0;
            const vector<VariablePtr> *    params = nullptr;
            size_t count () const { return params->size(); }
            Expression * arg ( size_t i ) const { return (*args)[ofs+i]; }
            const VariablePtr & param ( size_t i ) const { return (*params)[i]; }
        };

        // for a callee that DOES write: a substituted argument may only read storage
        // the spliced body provably cannot touch - plain local by-value variables that
        // are not passed by mutable reference in this very call and whose address was
        // never taken. no field / index / deref reads at all (they may reach heap or
        // aliased storage). constants and operations over such leaves are fine
        bool argReadsOnlyPrivateLocals ( Expression * e, const SiteArgs & sa ) {
            if ( !e ) return true;
            if ( e->rtti_isConstant() ) return true;
            if ( e->rtti_isVar() ) {
                auto v = static_cast<ExprVar *>(e);
                if ( !v->variable ) return false;
                if ( v->isGlobalVariable() ) return false;
                if ( v->variable->access_ref ) return false;    // address taken somewhere
                auto vt = v->variable->type;
                if ( vt && (vt->ref || vt->isRefType()) ) return false;
                for ( size_t oi=0, ois=sa.count(); oi!=ois; ++oi ) {
                    auto & OP = sa.param(oi);
                    bool oRef = OP->type->ref || OP->type->isRefType();
                    if ( !oRef || OP->type->constant ) continue;
                    Expression * oleaf = sa.arg(oi);
                    if ( oleaf->rtti_isR2V() ) oleaf = static_cast<ExprRef2Value *>(oleaf)->subexpr;
                    if ( oleaf->rtti_isVar() && static_cast<ExprVar *>(oleaf)->variable==v->variable ) return false;
                }
                return true;
            }
            if ( e->rtti_isR2V() ) return argReadsOnlyPrivateLocals(static_cast<ExprRef2Value *>(e)->subexpr, sa);
            if ( e->rtti_isCast() ) return argReadsOnlyPrivateLocals(static_cast<ExprCast *>(e)->subexpr, sa);
            if ( e->rtti_isOp1() ) return argReadsOnlyPrivateLocals(static_cast<ExprOp1 *>(e)->subexpr, sa);
            if ( e->rtti_isOp2() ) {
                auto op = static_cast<ExprOp2 *>(e);
                return argReadsOnlyPrivateLocals(op->left, sa)
                    && argReadsOnlyPrivateLocals(op->right, sa);
            }
            if ( e->rtti_isOp3() ) {
                auto op = static_cast<ExprOp3 *>(e);
                return argReadsOnlyPrivateLocals(op->subexpr, sa)
                    && argReadsOnlyPrivateLocals(op->left, sa)
                    && argReadsOnlyPrivateLocals(op->right, sa);
            }
            if ( e->rtti_isCall() ) {
                auto c = static_cast<ExprCall *>(e);
                for ( auto & a : c->arguments ) {
                    if ( !argReadsOnlyPrivateLocals(a, sa) ) return false;
                }
                return true;
            }
            return false;
        }

        bool exprContains ( Expression * root, Expression * what ) {
            if ( root==what ) return true;
            vector<pair<Expression *,int>> kids;
            childrenInEvalOrder(root, kids);
            for ( auto & k : kids ) {
                if ( k.first && exprContains(k.first, what) ) return true;
            }
            return false;
        }

        enum class SitePosition { Eager, Lazy, NotFound, Unsupported };

        struct PathScan {
            vector<Expression *> lazyOps;   // lazy ops on the path, outermost LAST (pushed on unwind)
        };

        SitePosition findPath ( Expression * root, Expression * call, PathScan & scan ) {
            if ( root==call ) return SitePosition::Eager;
            vector<pair<Expression *,int>> kids;
            childrenInEvalOrder(root, kids);
            for ( auto & k : kids ) {
                if ( !k.first ) continue;
                if ( !exprContains(k.first, call) ) continue;
                auto sub = findPath(k.first, call, scan);
                if ( sub==SitePosition::NotFound || sub==SitePosition::Unsupported ) return sub;
                if ( k.second==2 ) return SitePosition::Unsupported;
                if ( k.second==1 ) {
                    scan.lazyOps.push_back(root);
                    return SitePosition::Lazy;
                }
                return sub;
            }
            return SitePosition::NotFound;
        }

        // maximal impure subexpressions evaluated strictly before `call` within `root`,
        // in evaluation order. hoisting them (whole, internal laziness intact) keeps the
        // spliced arg temps from jumping ahead of earlier side effects
        void collectImpurePrefix ( Expression * root, Expression * call, vector<Expression *> & prefix ) {
            if ( root==call ) return;
            vector<pair<Expression *,int>> kids;
            childrenInEvalOrder(root, kids);
            for ( auto & k : kids ) {
                if ( !k.first ) continue;
                if ( exprContains(k.first, call) ) {
                    collectImpurePrefix(k.first, call, prefix);
                    return;
                }
                if ( !inlinePure(k.first) ) prefix.push_back(k.first);
            }
        }

        // manufactured `let/var name [&] = init` (or an uninitialized declaration)
        ExprLet * makeTemp ( const LineInfo & at, const string & name, Expression * init,
                             bool isConst, bool isRef, bool viaMove, bool viaClone = false ) {
            auto var = new Variable();
            var->name = name;
            var->at = at;
            var->generated = true;
            var->type = new TypeDecl(Type::autoinfer);
            var->type->constant = isConst;
            // autoinfer inherits constness from the init; a mutable temp initialized
            // from a constant must strip it (the `-const` operator) or writes fail
            var->type->removeConstant = !isConst;
            var->type->ref = isRef;
            var->init = init;
            var->init_via_move = viaMove;
            var->init_via_clone = viaClone;
            auto let = new ExprLet();
            let->at = at;
            let->atInit = init ? init->at : at;
            let->variables.push_back(var);
            return let;
        }

        ExprLet * makeUninitDecl ( const LineInfo & at, const string & name, const TypeDecl * type ) {
            auto var = new Variable();
            var->name = name;
            var->at = at;
            var->generated = true;
            var->type = new TypeDecl(*type);
            var->type->constant = false;
            var->type->ref = false;
            var->type->safeWhenUninitialized = true;    // assigned before any read by construction
            auto let = new ExprLet();
            let->at = at;
            let->variables.push_back(var);
            return let;
        }

        // ----- multi-return rewrite on the spliced clone -----
        // a callee may return from several places; on the CLONE (the standalone
        // function keeps its shape) every function-level return becomes a store into
        // the result temp. two tiers:
        //  * every return TERMINAL (no loop on its ancestor path, and transitively
        //    the last statement of every enclosing list - control after it is
        //    already function exit): stores substitute in place, zero overhead.
        //    ReturnCanonicalization manufactures these shapes ahead of candidacy.
        //  * otherwise a generated `_inl<N>_ret : bool` guards the fallthrough:
        //    a return becomes `store; flag = true` (+ `break` under a loop), an
        //    inner loop that may have flagged gets `if (flag) break` right after it
        //    (the cascade), and at non-loop levels the statement tail wraps in
        //    `if (!flag) { ... }`. break unwinds block finallys and closes iterators
        //    exactly like return did, and the store runs before the unwind - the
        //    same order as the real return. NO goto: a label would disqualify the
        //    whole caller from CSE and DSE (both bail on ExprLabel).
        // block literals are never descended into - their returns exit the block.

        enum class RetExit { None, Maybe, Always };

        struct ReturnStoreRewrite {
            string resName;     // empty = void subject
            string flagName;
            bool flagUsed = false;

            void apply ( ExprBlock * body ) {
                if ( blockTerminal(body, true, false) ) {
                    rewriteTerminalBlock(body);
                } else {
                    // any non-terminal shape stores through the flag on every return
                    flagUsed = true;
                    transformBlock(body, false);
                }
            }

            // pure prescan for the shape gates: the flag tier truncates or wraps the
            // statement tail of the block holding a return, and doing that to a
            // finalList-carrying block can hide declarations its finally references
            // by name (re-infer resolves finally references by name). those bodies
            // decline up front - the terminal tier rewrites in place and is exempt
            static bool finallyWrapHazard ( ExprBlock * body ) {
                if ( blockTerminal(body, true, false) ) return false;
                return hazardBlock(body);
            }

            static bool stmtHasReturn ( Expression * s ) {
                if ( s->rtti_isReturn() ) return true;
                if ( s->rtti_isIfThenElse() ) {
                    auto ite = static_cast<ExprIfThenElse *>(s);
                    if ( stmtHasReturn(ite->if_true) ) return true;
                    return ite->if_false && stmtHasReturn(ite->if_false);
                }
                if ( s->rtti_isBlock() ) {
                    auto blk = static_cast<ExprBlock *>(s);
                    for ( auto & st : blk->list ) {
                        if ( stmtHasReturn(st) ) return true;
                    }
                    return false;
                }
                if ( s->rtti_isFor() ) {
                    auto f = static_cast<ExprFor *>(s);
                    return f->body && stmtHasReturn(f->body);
                }
                if ( s->rtti_isWhile() ) {
                    auto w = static_cast<ExprWhile *>(s);
                    return w->body && stmtHasReturn(w->body);
                }
                if ( s->rtti_isUnsafe() ) {
                    auto u = static_cast<ExprUnsafe *>(s);
                    return u->body && stmtHasReturn(u->body);
                }
                if ( s->rtti_isWith() ) {
                    auto w = static_cast<ExprWith *>(s);
                    return w->body && stmtHasReturn(w->body);
                }
                return false;   // block literals keep their returns - not ours
            }

            static bool hazardStmt ( Expression * s ) {
                if ( s->rtti_isIfThenElse() ) {
                    auto ite = static_cast<ExprIfThenElse *>(s);
                    if ( hazardStmt(ite->if_true) ) return true;
                    return ite->if_false && hazardStmt(ite->if_false);
                }
                if ( s->rtti_isBlock() ) return hazardBlock(static_cast<ExprBlock *>(s));
                if ( s->rtti_isFor() ) {
                    auto f = static_cast<ExprFor *>(s);
                    return f->body && hazardStmt(f->body);
                }
                if ( s->rtti_isWhile() ) {
                    auto w = static_cast<ExprWhile *>(s);
                    return w->body && hazardStmt(w->body);
                }
                if ( s->rtti_isUnsafe() ) {
                    auto u = static_cast<ExprUnsafe *>(s);
                    return u->body && hazardStmt(u->body);
                }
                if ( s->rtti_isWith() ) {
                    auto w = static_cast<ExprWith *>(s);
                    return w->body && hazardStmt(w->body);
                }
                return false;
            }

            static bool hazardBlock ( ExprBlock * blk ) {
                auto & list = blk->list;
                for ( size_t i=0, n=list.size(); i!=n; ++i ) {
                    if ( !blk->finalList.empty() && i+1<n && stmtHasReturn(list[i]) ) return true;
                    if ( hazardStmt(list[i]) ) return true;
                }
                return false;
            }

            // the result store, mirroring the call's own return semantics: `return <- x`
            // moves, `return x` copies. a move store is illegal from a copyable rvalue -
            // `return <- g(...)` of a pointer result must store as a COPY (identical
            // semantics for an rvalue; ExprMove rejects it, 30941). only a PROVEN
            // copyable rvalue demotes: an untyped subexpr (a same-round chained splice's
            // manufactured result-temp read) keeps the move - those reads are
            // references, and demoting a non-copyable to a copy is 30950
            Expression * makeStore ( ExprReturn * ret ) const {
                if ( resName.empty() ) return ret->subexpr; // void: keep the side effects, if any
                if ( !ret->subexpr ) return nullptr;        // defensive - infer rejects value-less returns
                bool storeAsCopy = ret->moveSemantics
                    && ret->subexpr->type
                    && !ret->subexpr->type->ref
                    && ret->subexpr->type->canCopy();
                if ( ret->moveSemantics && !storeAsCopy ) {
                    return new ExprMove(ret->at, new ExprVar(ret->at, resName), ret->subexpr);
                }
                return new ExprCopy(ret->at, new ExprVar(ret->at, resName), ret->subexpr);
            }

        protected:

            // ----- terminality prescan (read-only) -----

            static bool stmtTerminal ( Expression * stmt, bool tail, bool underLoop ) {
                if ( stmt->rtti_isReturn() ) return tail && !underLoop;
                if ( stmt->rtti_isIfThenElse() ) {
                    auto ite = static_cast<ExprIfThenElse *>(stmt);
                    if ( !armTerminal(ite->if_true, tail, underLoop) ) return false;
                    if ( ite->if_false && !armTerminal(ite->if_false, tail, underLoop) ) return false;
                    return true;
                }
                if ( stmt->rtti_isBlock() ) {
                    return blockTerminal(static_cast<ExprBlock *>(stmt), tail, underLoop);
                }
                if ( stmt->rtti_isFor() ) {
                    auto f = static_cast<ExprFor *>(stmt);
                    return !f->body || stmtTerminal(f->body, false, true);
                }
                if ( stmt->rtti_isWhile() ) {
                    auto w = static_cast<ExprWhile *>(stmt);
                    return !w->body || stmtTerminal(w->body, false, true);
                }
                if ( stmt->rtti_isUnsafe() ) {
                    auto u = static_cast<ExprUnsafe *>(stmt);
                    return !u->body || stmtTerminal(u->body, tail, underLoop);
                }
                if ( stmt->rtti_isWith() ) {
                    auto w = static_cast<ExprWith *>(stmt);
                    return !w->body || stmtTerminal(w->body, tail, underLoop);
                }
                // returns are statements; no other statement kind hosts one
                // (block literals keep theirs - they exit the block, not the function)
                return true;
            }

            static bool armTerminal ( Expression * arm, bool tail, bool underLoop ) {
                if ( arm->rtti_isReturn() ) return tail && !underLoop;  // naked canonicalized arm
                return stmtTerminal(arm, tail, underLoop);
            }

            static bool blockTerminal ( ExprBlock * blk, bool tail, bool underLoop ) {
                for ( size_t i=0, n=blk->list.size(); i!=n; ++i ) {
                    if ( !stmtTerminal(blk->list[i], tail && (i==n-1), underLoop) ) return false;
                }
                return true;
            }

            // ----- terminal tier: in-place stores -----

            void rewriteTerminalBlock ( ExprBlock * blk ) {
                auto & list = blk->list;
                for ( size_t i=0; i<list.size(); ) {
                    if ( list[i]->rtti_isReturn() ) {
                        auto store = makeStore(static_cast<ExprReturn *>(list[i]));
                        if ( store ) { list[i] = store; ++i; }
                        else list.erase(list.begin()+i);    // bare void return
                        continue;
                    }
                    rewriteTerminalStmt(list[i]);
                    ++i;
                }
            }

            void rewriteTerminalArm ( ExpressionPtr & arm ) {
                if ( arm->rtti_isReturn() ) {
                    auto store = makeStore(static_cast<ExprReturn *>(arm));
                    auto blk = new ExprBlock();
                    blk->at = arm->at;
                    if ( store ) blk->list.push_back(store);
                    arm = blk;
                    return;
                }
                rewriteTerminalStmt(arm);
            }

            void rewriteTerminalStmt ( ExpressionPtr & slot ) {
                Expression * s = slot;
                if ( s->rtti_isIfThenElse() ) {
                    auto ite = static_cast<ExprIfThenElse *>(s);
                    rewriteTerminalArm(ite->if_true);
                    if ( ite->if_false ) rewriteTerminalArm(ite->if_false);
                } else if ( s->rtti_isBlock() ) {
                    rewriteTerminalBlock(static_cast<ExprBlock *>(s));
                } else if ( s->rtti_isUnsafe() ) {
                    auto u = static_cast<ExprUnsafe *>(s);
                    if ( u->body ) rewriteTerminalStmt(u->body);
                } else if ( s->rtti_isWith() ) {
                    auto w = static_cast<ExprWith *>(s);
                    if ( w->body ) rewriteTerminalStmt(w->body);
                }
                // loops hold no returns here (terminality proved it) - nothing to descend for
            }

            // ----- flag tier -----

            Expression * flagRead ( const LineInfo & at ) const {
                return new ExprVar(at, flagName);
            }
            Expression * flagSet ( const LineInfo & at ) const {
                return new ExprCopy(at, new ExprVar(at, flagName), new ExprConstBool(at, true));
            }

            RetExit transformBlock ( ExprBlock * blk, bool inLoop ) {
                auto & list = blk->list;
                RetExit overall = RetExit::None;
                for ( size_t i=0; i<list.size(); ++i ) {
                    RetExit st = transformStmt(list[i], inLoop);
                    if ( st==RetExit::Always ) {
                        list.resize(i+1);   // anything after is dead
                        return RetExit::Always;
                    }
                    if ( st!=RetExit::Maybe ) continue;
                    overall = RetExit::Maybe;
                    if ( i+1 >= list.size() ) break;
                    if ( inLoop ) {
                        // the cascade: control continuing past a flagged inner exit
                        // (an inner loop's break) leaves this loop too. for a non-loop
                        // Maybe statement the returning paths already broke - the
                        // guard is dead there and folds away
                        auto brkBlk = new ExprBlock();
                        brkBlk->at = list[i]->at;
                        brkBlk->list.push_back(new ExprBreak(list[i]->at));
                        list.insert(list.begin()+i+1,
                            new ExprIfThenElse(list[i]->at, flagRead(list[i]->at), brkBlk, nullptr));
                        ++i;    // past the guard
                        continue;
                    }
                    // non-loop level: the tail runs only when no return fired
                    auto tailBlk = new ExprBlock();
                    tailBlk->at = list[i+1]->at;
                    tailBlk->list.assign(list.begin()+i+1, list.end());
                    list.resize(i+1);
                    RetExit tailSt = transformBlock(tailBlk, false);
                    list.push_back(new ExprIfThenElse(tailBlk->at,
                        new ExprOp1(tailBlk->at, "!", flagRead(tailBlk->at)), tailBlk, nullptr));
                    // flagged paths returned, and an always-returning tail covers the rest
                    return tailSt==RetExit::Always ? RetExit::Always : RetExit::Maybe;
                }
                return overall;
            }

            RetExit transformArm ( ExpressionPtr & arm, bool inLoop ) {
                if ( arm->rtti_isReturn() ) {   // naked canonicalized arm - re-block and retry
                    auto blk = new ExprBlock();
                    blk->at = arm->at;
                    blk->list.push_back(arm);
                    arm = blk;
                }
                return transformStmt(arm, inLoop);
            }

            RetExit transformStmt ( ExpressionPtr & slot, bool inLoop ) {
                Expression * s = slot;
                if ( s->rtti_isReturn() ) {
                    auto ret = static_cast<ExprReturn *>(s);
                    auto blk = new ExprBlock();
                    blk->at = ret->at;
                    if ( auto store = makeStore(ret) ) blk->list.push_back(store);
                    blk->list.push_back(flagSet(ret->at));
                    if ( inLoop ) blk->list.push_back(new ExprBreak(ret->at));
                    slot = blk;
                    return RetExit::Always;
                }
                if ( s->rtti_isIfThenElse() ) {
                    auto ite = static_cast<ExprIfThenElse *>(s);
                    RetExit a = transformArm(ite->if_true, inLoop);
                    RetExit b = ite->if_false ? transformArm(ite->if_false, inLoop) : RetExit::None;
                    if ( a==RetExit::Always && ite->if_false && b==RetExit::Always ) return RetExit::Always;
                    if ( a!=RetExit::None || b!=RetExit::None ) return RetExit::Maybe;
                    return RetExit::None;
                }
                if ( s->rtti_isBlock() ) {
                    return transformBlock(static_cast<ExprBlock *>(s), inLoop);
                }
                if ( s->rtti_isFor() ) {
                    auto f = static_cast<ExprFor *>(s);
                    if ( f->body && f->body->rtti_isBlock() ) {
                        if ( transformBlock(static_cast<ExprBlock *>(f->body), true)!=RetExit::None ) {
                            return RetExit::Maybe;  // may have exited via a flagged break
                        }
                    }
                    return RetExit::None;
                }
                if ( s->rtti_isWhile() ) {
                    auto w = static_cast<ExprWhile *>(s);
                    if ( w->body && w->body->rtti_isBlock() ) {
                        if ( transformBlock(static_cast<ExprBlock *>(w->body), true)!=RetExit::None ) {
                            return RetExit::Maybe;
                        }
                    }
                    return RetExit::None;
                }
                if ( s->rtti_isUnsafe() ) {
                    auto u = static_cast<ExprUnsafe *>(s);
                    if ( u->body ) return transformStmt(u->body, inLoop);
                    return RetExit::None;
                }
                if ( s->rtti_isWith() ) {
                    auto w = static_cast<ExprWith *>(s);
                    if ( w->body ) return transformStmt(w->body, inLoop);
                    return RetExit::None;
                }
                return RetExit::None;
            }
        };

    } // anonymous namespace

    // ----- the [inline] shape contract -----

    bool checkInlineShape ( Function * fn, string & err ) {
        if ( fn->isTemplate ) return true;      // instances are checked instead
        if ( fn->builtIn || !fn->body ) { err = "[inline] requires a function with a das body"; return false; }
        if ( !fn->body->rtti_isBlock() ) { err = "[inline] requires a block body"; return false; }
        if ( fn->generator ) { err = "[inline] does not support generators"; return false; }
        if ( fn->lambda ) { err = "[inline] does not support lambdas"; return false; }
        if ( fn->isClassMethod || fn->classParent ) { err = "[inline] does not support class methods"; return false; }
        if ( fn->isCustomProperty || fn->propertyFunction ) { err = "[inline] does not support property functions"; return false; }
        // a generic instance is named `origin`<hash> - judge by the origin's name.
        // ExprOp-dispatched operator overloads splice; punctuation functions whose
        // call sites are other node kinds ([], ??, properties) can't honor the contract
        const string & plainName = fn->fromGeneric ? fn->fromGeneric->name : fn->name;
        if ( !isPlainIdentifier(plainName) && !exprOpDispatchedName(plainName) ) {
            err = "[inline] does not support operator '" + plainName + "' - its call sites do not splice";
            return false;
        }
        if ( fn->result && !fn->result->isVoid() ) {
            if ( fn->result->ref ) {
                err = "[inline] result must be by-value (or void)";
                return false;
            }
            // `#` is a call-boundary lifetime annotation; the splice dissolves that
            // boundary, and the result store would copy a temporary (30915)
            if ( fn->result->temporary ) {
                err = "[inline] does not support a temporary (#) result";
                return false;
            }
            // a refType result splices through a manufactured uninitialized (zeroed)
            // temp that the trailing return fully writes before any read; a type whose
            // C++ constructor must run cannot take that path
            if ( fn->result->isRefType() && fn->result->hasNonTrivialCtor() ) {
                err = "[inline] result type requires nontrivial construction";
                return false;
            }
        }
        for ( auto & arg : fn->arguments ) {
            if ( !arg->type ) continue;
            if ( arg->type->temporary ) { err = "[inline] does not support temporary (#) parameter '" + arg->name + "'"; return false; }
            if ( arg->type->implicit ) { err = "[inline] does not support implicit parameter '" + arg->name + "'"; return false; }
            // a smart-pointer argument temp would need inscope/move discipline the
            // splicer does not manufacture (strict_smart_pointers rejects the copy)
            if ( arg->type->smartPtr ) { err = "[inline] does not support smart-pointer parameter '" + arg->name + "'"; return false; }
        }
        if ( fn->result && fn->result->smartPtr ) { err = "[inline] does not support a smart-pointer result"; return false; }
        auto body = static_cast<ExprBlock *>(fn->body);
        // a function-level finally stays refused: it runs against the spliced result
        // stores in the caller's frame, an interaction the v1 probe already caught
        // misordering (glob_count drifted). block-level finally INSIDE the body keeps
        // its own block and splices fine
        if ( !body->finalList.empty() ) { err = "[inline] does not support a function-level finally"; return false; }
        InlineShapeScan scan;
        fn->body->visit(scan);
        if ( scan.bad ) { err = "[inline] " + scan.reason; return false; }
        if ( ReturnStoreRewrite::finallyWrapHazard(body) ) {
            err = "[inline] early returns conflict with a finally section";
            return false;
        }
        return true;
    }

    // true when `fn` is reachable from its own body through direct calls (or operator
    // sites) to [inline] functions - splicing such a cycle would never terminate
    static bool inlineGraphReaches ( Function * target, Function * cur, das_hash_set<Function *> & visited ) {
        if ( !cur->body ) return false;
        bool found = false;
        lookupExpressions(cur->body, [&](Expression * expr) {
            if ( found ) return;
            Function * fn = callLikeFunc(expr);
            if ( !fn || !fn->mustInline ) return;
            if ( fn==target ) { found = true; return; }
            if ( visited.insert(fn).second ) {
                if ( inlineGraphReaches(target, fn, visited) ) found = true;
            }
        });
        return found;
    }

    bool checkInlineRecursion ( Function * fn, string & err ) {
        if ( fn->isTemplate || !fn->body ) return true;
        das_hash_set<Function *> visited;
        if ( inlineGraphReaches(fn, fn, visited) ) {
            err = "[inline] function is recursive through the inline graph";
            return false;
        }
        return true;
    }

    // ----- best-effort shape gates (auto inlining and devirtualization decline, never error) -----

    namespace {

        // any local declared as a reference (`let & =` / `var & =`), including the
        // manufactured argument references of an earlier splice round
        bool bindsLocalRef ( Expression * root ) {
            bool found = false;
            lookupExpressions(root, [&](Expression * e) {
                if ( found || !e->rtti_isLet() ) return;
                auto let = static_cast<ExprLet *>(e);
                for ( auto & v : let->variables ) {
                    if ( v->type && v->type->ref ) { found = true; return; }
                }
            });
            return found;
        }

        // a call (or invoke) passing a `#` argument where the resolved target's parameter
        // is neither `#` nor implicit. the binding is tolerated on the original node (the
        // genericFunction short-circuit) and the locked-name fallback re-resolves it after
        // a splice - but a spliced `#` view of container storage runs under the CALLER's
        // optimizer scope, where the temp's real lifetime is no longer fenced by the call
        // boundary (proven value corruption: linq's lazy group_by pipeline). a subtree
        // carrying one cannot ride a splice
        bool reinferFragile ( Expression * root ) {
            bool fragile = false;
            lookupExpressions(root, [&](Expression * e) {
                if ( fragile ) return;
                if ( e->rtti_isCall() ) {
                    auto c = static_cast<ExprCall *>(e);
                    if ( !c->func ) return;
                    size_t n = das::min(c->arguments.size(), c->func->arguments.size());
                    for ( size_t i=0; i!=n; ++i ) {
                        auto & at = c->arguments[i]->type;
                        auto & pt = c->func->arguments[i]->type;
                        if ( at && pt && at->temporary && !pt->temporary && !pt->implicit ) {
                            fragile = true;
                            return;
                        }
                    }
                } else if ( e->rtti_isInvoke() ) {
                    auto inv = static_cast<ExprInvoke *>(e);
                    if ( inv->arguments.empty() ) return;
                    auto & bt = inv->arguments[0]->type;
                    if ( !bt || bt->argTypes.empty() ) return;
                    size_t n = das::min(inv->arguments.size()-1, bt->argTypes.size());
                    for ( size_t i=0; i!=n; ++i ) {
                        auto & at = inv->arguments[i+1]->type;
                        auto & pt = bt->argTypes[i];
                        if ( at && pt && at->temporary && !pt->temporary && !pt->implicit ) {
                            fragile = true;
                            return;
                        }
                    }
                }
            });
            return fragile;
        }

        // a block literal the devirtualizer can splice in place: a plain in-frame block
        // with a single-exit body and a non-reference (or void) result. splicing dissolves
        // the block boundary, so a break/continue not bound to a loop inside the body is out
        bool checkDevirtShape ( ExprMakeBlock * mkb, string & why ) {
            if ( mkb->isLambda || mkb->isLocalFunction || !mkb->capture.empty() ) { why = "lambda literal"; return false; }
            if ( !mkb->block || !mkb->block->rtti_isBlock() ) { why = "no block body"; return false; }
            auto blk = static_cast<ExprBlock *>(mkb->block);
            bool isVoid = !blk->returnType || blk->returnType->isVoid();
            if ( !isVoid && blk->returnType->ref ) {
                why = "result is not by-value";
                return false;
            }
            if ( !isVoid && blk->returnType->temporary ) {
                why = "temporary result";
                return false;
            }
            if ( !isVoid && blk->returnType->isRefType() && blk->returnType->hasNonTrivialCtor() ) {
                why = "result requires nontrivial construction";
                return false;
            }
            for ( auto & arg : blk->arguments ) {
                if ( !arg->type ) { why = "unresolved parameter"; return false; }
                if ( arg->type->temporary ) { why = "temporary (#) parameter"; return false; }
                if ( arg->type->implicit ) { why = "implicit parameter"; return false; }
                if ( arg->type->smartPtr ) { why = "smart-pointer parameter"; return false; }
            }
            if ( blk->returnType && blk->returnType->smartPtr ) { why = "smart-pointer result"; return false; }
            if ( reinferFragile(mkb) ) { why = "body carries a temporary-flavored call"; return false; }
            InlineShapeScan scan(true);
            blk->visit(scan);
            if ( scan.bad ) { why = scan.reason; return false; }
            if ( ReturnStoreRewrite::finallyWrapHazard(blk) ) { why = "early returns conflict with a finally section"; return false; }
            return true;
        }

        // recursion over the whole splice graph - [inline] calls, auto-eligible calls,
        // and operator sites alike - would re-manufacture eligible sites every round
        bool spliceGraphReaches ( Function * target, Function * cur, const AutoInlineCfg & cfg,
                das_hash_set<Function *> & visited ) {
            if ( !cur->body ) return false;
            bool found = false;
            lookupExpressions(cur->body, [&](Expression * expr) {
                if ( found ) return;
                Function * fn = callLikeFunc(expr);
                if ( !fn ) return;
                if ( !fn->mustInline ) {
                    bool eligible = expr->rtti_isCall()
                        ? autoEligibleCall(static_cast<ExprCall *>(expr), cfg)
                        : autoEligibleFn(fn, cfg);
                    if ( !eligible ) return;
                }
                if ( fn==target ) { found = true; return; }
                if ( visited.insert(fn).second ) {
                    if ( spliceGraphReaches(target, fn, cfg, visited) ) found = true;
                }
            });
            return found;
        }

    } // anonymous namespace

    // ----- the patch pass -----

    namespace {

        struct InlinePatch {
            Program * program = nullptr;
            TextWriter * logs = nullptr;
            bool logOpt = false;
            bool mustEnabled = true;
            AutoInlineCfg autoCfg;
            bool changed = false;
            int inlined = 0;        // [inline] sites
            int inlinedAuto = 0;    // auto block-literal call sites
            int devirted = 0;       // invoke-of-literal sites
            int declined = 0;       // best-effort sites the pass passed on
            das_hash_map<ExprBlock *, ParamReadStats> statsCache;
            das_hash_map<Function *, bool> shapeCache;
            das_hash_map<Function *, pair<bool,string>> autoOkCache;
            das_hash_map<ExprMakeBlock *, pair<bool,string>> devirtShapeCache;
            das_hash_map<Function *, CrossVerdict> privateUseCache;
            das_hash_map<Function *, bool> cycleCache;

            // reachability of `fn` from its own body over the RUNTIME call graph
            // (every call, not just splice-eligible ones), capped: cycles are short
            // in practice, and beyond the cap we assume acyclic - the per-round
            // growth bounds still apply. splices only move already-reachable calls
            // inline, so a cached verdict stays valid across rounds (a stale `true`
            // after a cycle splices away merely declines conservatively)
            bool callerOnCallCycle ( Function * fn ) {
                auto it = cycleCache.find(fn);
                if ( it != cycleCache.end() ) return it->second;
                das_hash_set<Function *> visited;
                vector<Function *> work;
                auto push = [&](Function * f) {
                    if ( f && f->body && visited.size()<128 && visited.insert(f).second ) {
                        work.push_back(f);
                    }
                };
                push(fn);
                bool found = false;
                while ( !work.empty() && !found ) {
                    Function * cur = work.back();
                    work.pop_back();
                    lookupExpressions(cur->body, [&](Expression * e) {
                        if ( found ) return;
                        Function * callee = callLikeFunc(e);
                        if ( !callee ) return;
                        if ( callee==fn ) { found = true; return; }
                        push(callee);
                    });
                }
                cycleCache[fn] = found;
                return found;
            }

            bool calleeShapeOk ( Function * fn ) {
                auto it = shapeCache.find(fn);
                if ( it != shapeCache.end() ) return it->second;
                string err;
                bool ok = checkInlineShape(fn, err) && checkInlineRecursion(fn, err);
                shapeCache[fn] = ok;
                return ok;
            }

            // best-effort callee gate: [inline]'s shape contract, plus no recursion
            // through the combined splice graph. a plain unsafe body (hasUnsafe) is fine:
            // it splices under a generated `unsafe { }` wrapper
            bool autoCalleeOk ( Function * fn, string & why ) {
                auto it = autoOkCache.find(fn);
                if ( it != autoOkCache.end() ) { why = it->second.second; return it->second.first; }
                bool ok = true;
                if ( !checkInlineShape(fn, why) ) {
                    ok = false;
                } else if ( fn->unsafeOperation || fn->unsafeDeref ) {
                    // [unsafe_operation]/[unsafe_deref] carry call-site and runtime
                    // semantics (re-infer re-stamps deref null-check elision from the
                    // CALLER's unsafeDeref flag) that a splice would change
                    ok = false;
                    why = "unsafe operation";
                } else if ( reinferFragile(fn->body) ) {
                    ok = false;
                    why = "body carries a temporary-flavored call";
                } else if ( bindsLocalRef(fn->body) ) {
                    // a `let & =` in the body re-binds its initializer under the caller's
                    // scope, and a CHAINED splice can substitute the initializer's leaf
                    // with a non-addressable expression (proven: decs' req_hash ->
                    // compile_request -> lookup_request chain, where a previously
                    // manufactured arg reference ended up bound to a temporary - 31019).
                    // note this also catches this pass's own manufactured references,
                    // stopping re-splice chains at the first reference-binding body
                    ok = false;
                    why = "body binds a local reference";
                } else if ( annotatedCallee(fn, why) ) {
                    ok = false;     // an annotation may carry call-site semantics
                                    // (verifyCall & co) that a splice would bypass
                } else {
                    // a type<...> witness naming the generic's own alias (type<TT>) stays
                    // symbolic in the instance body and cannot re-resolve after a splice
                    bool aliasWitness = false;
                    lookupExpressions(fn->body, [&](Expression * expr) {
                        if ( aliasWitness || !expr->rtti_isTypeDecl() ) return;
                        auto td = static_cast<ExprTypeDecl *>(expr);
                        if ( td->typeexpr && td->typeexpr->isAutoOrAlias() ) aliasWitness = true;
                    });
                    if ( aliasWitness ) {
                        ok = false;
                        why = "body carries a generic type witness";
                    } else {
                        das_hash_set<Function *> visited;
                        if ( spliceGraphReaches(fn, fn, autoCfg, visited) ) {
                            ok = false;
                            why = "recursive through the inline graph";
                        }
                    }
                }
                autoOkCache[fn] = pair<bool,string>(ok, why);
                return ok;
            }

            bool devirtShapeOk ( ExprMakeBlock * mkb, string & why ) {
                auto it = devirtShapeCache.find(mkb);
                if ( it != devirtShapeCache.end() ) { why = it->second.second; return it->second.first; }
                bool ok = checkDevirtShape(mkb, why);
                devirtShapeCache[mkb] = pair<bool,string>(ok, why);
                return ok;
            }

            const ParamReadStats & paramStats ( ExprBlock * body, const vector<VariablePtr> & params ) {
                auto it = statsCache.find(body);
                if ( it != statsCache.end() ) return it->second;
                auto & st = statsCache[body];
                ParamReadScan scan(params, st);
                body->visit(scan);
                return st;
            }

            const CrossVerdict & privateUse ( Function * fn, Module * originModule ) {
                auto it = privateUseCache.find(fn);
                if ( it != privateUseCache.end() ) return it->second;
                PrivateUseScan scan(originModule, program->thisModule.get(),
                    fn->module==program->thisModule.get());
                fn->body->visit(scan);
                return privateUseCache[fn] = scan.verdict;
            }

            // next free _inl counter across a subtree; names are function-scoped and
            // later rounds keep splicing into the same function, so continue the sequence
            int nextInlineIdIn ( Expression * body ) {
                int mx = 0;
                auto consider = [&]( const string & name ) {
                    if ( name.compare(0, INLINE_TEMP_PREFIX_LEN, INLINE_TEMP_PREFIX)==0 ) {
                        int id = atoi(name.c_str()+INLINE_TEMP_PREFIX_LEN);
                        if ( id >= mx ) mx = id + 1;
                    }
                };
                lookupExpressions(body, [&](Expression * expr) {
                    if ( expr->rtti_isLet() ) {
                        auto let = static_cast<ExprLet *>(expr);
                        for ( auto & v : let->variables ) consider(v->name);
                    } else if ( expr->rtti_isFor() ) {
                        auto f = static_cast<ExprFor *>(expr);
                        for ( auto & v : f->iteratorVariables ) consider(v->name);
                    }
                });
                return mx;
            }
            int nextInlineId ( Function * fn ) { return nextInlineIdIn(fn->body); }

            void error ( const string & what, const LineInfo & at ) {
                program->error(what, "", "", at, CompilationError::invalid_annotation);
            }

            // a MUST site that can't splice fails the compile; a best-effort site
            // declines silently (counted, logged under log_optimization)
            void siteFail ( const PlannedSite & site, const string & what, const LineInfo & at ) {
                if ( site.kind==SiteKind::MustCall ) {
                    error(what, at);
                } else {
                    decline(site, what, at);
                }
            }

            void decline ( const PlannedSite & site, const string & why, const LineInfo & at ) {
                declined ++;
                if ( logOpt && logs ) {
                    *logs << (site.kind==SiteKind::Devirt ? "DEVIRT DECLINED at " : "AUTO-INLINE DECLINED at ")
                          << at.describe() << ": " << why << "\n";
                }
            }

            void completeSite ( const PlannedSite & site, Function * caller, const string & subjName,
                    const string & scopeModule = string() ) {
                switch ( site.kind ) {
                    case SiteKind::MustCall: inlined ++; break;
                    case SiteKind::AutoCall: inlinedAuto ++; break;
                    case SiteKind::Devirt:   devirted ++; break;
                }
                if ( logOpt && logs ) {
                    const char * verb = site.kind==SiteKind::MustCall ? "INLINE "
                        : site.kind==SiteKind::AutoCall ? "AUTO-INLINE " : "DEVIRT ";
                    *logs << verb << subjName << " into " << caller->getMangledName();
                    if ( !scopeModule.empty() ) *logs << " under with (module " << scopeModule << ")";
                    *logs << " at " << site.callLike->at.describe() << "\n";
                }
                changed = true;
            }

            void processFunction ( Function * fn );
        };

        void InlinePatch::processFunction ( Function * fn ) {
            // a caller with call-site-semantic annotations ([template] & co) may be
            // CLONED per call site by its macro - a spliced body loses its
            // instantiation-site resolution (proven: sqlite_boost's [template]
            // try_create_table lost the user module's generated
            // _sql_create_table_sql after a splice). heuristic sites stay off
            // inside such bodies; the block-literal tier keeps shipping behavior
            AutoInlineCfg fnCfg = autoCfg;
            if ( fnCfg.functions ) {
                string why;
                if ( annotatedCallee(fn, why) ) {
                    fnCfg.functions = false;
                } else if ( fn->fromGeneric && fn->fromGeneric->module
                    && fn->fromGeneric->module != program->thisModule.get() ) {
                    // a generic INSTANCE from another module re-infers its whole body
                    // under THIS module's visibility after a splice - references legal
                    // at the origin (sqlite_boost::try_exec inside sql_migrate's
                    // _migrate_inner instance) stop resolving. cross-origin instances
                    // keep their bodies untouched
                    fnCfg.functions = false;
                } else if ( bodyCost(fn->body).stackBytes > AUTO_INLINE_CALLER_FRAME_BYTES ) {
                    // a frame already this large is one deep-chain hop away from an
                    // `options stack` overflow - stop growing it (recomputed per round,
                    // so accumulated splices shut the door behind themselves)
                    fnCfg.functions = false;
                } else if ( callerOnCallCycle(fn) ) {
                    // a recursive caller multiplies every spliced byte by its live
                    // stack depth - the frame bounds assume one instance (proven:
                    // ast_boost's walk_and_convert family, mutually recursive with
                    // its nine multi-return helpers, overflowed the macro context
                    // stack once they spliced in)
                    fnCfg.functions = false;
                }
            }
            InlineCollect collect(fnCfg);
            fn->body->visit(collect);
            if ( collect.sites.empty() ) return;
            // let-bound block literals this function's invokes may resolve to
            das_hash_map<Variable *, BlockBinding> bindings;
            if ( autoCfg.blockLiterals ) {
                BlockBindingScan bscan;
                fn->body->visit(bscan);
                for ( auto & kv : bscan.binding ) {
                    if ( bscan.disq.find(kv.first)==bscan.disq.end() ) bindings[kv.first] = kv.second;
                }
            }
            int inlineId = nextInlineId(fn);
            das_hash_set<string> callerDeclNames;
            {
                DeclNameCollect dnc;
                dnc.names = &callerDeclNames;
                fn->body->visit(dnc);
                for ( auto & arg : fn->arguments ) {
                    callerDeclNames.insert(arg->name);
                    if ( !arg->aka.empty() ) callerDeclNames.insert(arg->aka);
                }
            }
            // per-round frame-growth budget (see AUTO_INLINE_CALLER_GROWTH_BYTES);
            // conservatively charged at the gate, so later-declined sites still count
            int64_t grownBytes = 0;
            // a splice shifts the indices of later statements in the same block
            das_hash_map<ExprBlock *, int> indexShift;
            for ( auto & site : collect.sites ) {
                auto callLike = site.callLike;
                if ( site.kind==SiteKind::MustCall && !mustEnabled ) continue;
                // ----- resolve the splice subject -----
                Function * calleeFn = nullptr;          // MustCall / AutoCall
                ExprMakeBlock * literal = nullptr;      // Devirt
                string subjName;
                if ( site.kind==SiteKind::Devirt ) {
                    auto inv = static_cast<ExprInvoke *>(callLike);
                    Expression * a0 = inv->arguments[0];
                    if ( a0->rtti_isR2V() ) a0 = static_cast<ExprRef2Value *>(a0)->subexpr;
                    if ( a0->rtti_isMakeBlock() ) {
                        literal = static_cast<ExprMakeBlock *>(a0);
                    } else if ( a0->rtti_isVar() ) {
                        auto v = static_cast<ExprVar *>(a0);
                        if ( v->variable ) {
                            auto bit = bindings.find(v->variable);
                            if ( bit!=bindings.end() ) {
                                if ( bit->second.withModule != site.withModule ) {
                                    // splicing would re-home the literal's names into the
                                    // invoke's `with (module ...)` scope (or out of its own)
                                    decline(site, "block literal binding crosses a module resolution scope", callLike->at);
                                    continue;
                                }
                                literal = bit->second.literal;
                                // the splice eats this read, usually the holder's only one. lint runs
                                // after the patch slot but before optimize reaps the dead `let`, so it
                                // would report LINT002 on a variable the user demonstrably uses
                                v->variable->marked_used = true;
                            }
                        }
                    }
                    if ( !literal ) continue;   // not a traceable block literal
                    subjName = "block";
                } else {
                    calleeFn = static_cast<ExprCallFunc *>(callLike)->func;  // ExprCall or ExprOp1/2/3
                    subjName = calleeFn->name;
                }
                if ( !site.stmt || !site.anchor.block ) {
                    siteFail(site, "can't inline " + subjName + " here: the call is outside a function statement", callLike->at);
                    continue;
                }
                int anchorIndex = site.anchor.index + indexShift[site.anchor.block];
                // an earlier action this round may have rewritten this site's statement
                // (var+if lowering), moved its subtree into a hoist temp, or detached it
                // by cloning (arg temps, grafts). a site whose call is no longer live at
                // its recorded statement is skipped; the next patch round collects the
                // moved/cloned call at its new home. liveness mirrors exactly what the
                // processing below can reach: eval-order positions, a while condition,
                // or an elif condition still in unblockified chain form
                if ( anchorIndex < 0 || anchorIndex >= int(site.anchor.block->list.size())
                    || site.anchor.block->list[anchorIndex] != site.stmt ) {
                    continue;
                }
                bool siteLive = exprContains(site.stmt, callLike);
                if ( !siteLive && site.stmt->rtti_isWhile() ) {
                    siteLive = exprContains(static_cast<ExprWhile *>(site.stmt)->cond, callLike);
                }
                if ( !siteLive && site.stmt->rtti_isIfThenElse() ) {
                    auto lite = static_cast<ExprIfThenElse *>(site.stmt);
                    while ( lite->if_false && lite->if_false->rtti_isIfThenElse() ) {
                        auto inner = static_cast<ExprIfThenElse *>(lite->if_false);
                        if ( exprContains(inner->cond, callLike) ) { siteLive = true; break; }
                        lite = inner;
                    }
                }
                if ( !siteLive ) continue;
                // a statement lint will reject as 30250 (same-table lookup collision)
                // must reach lint intact - splicing would erase the error, not the UB.
                // MUST sites keep the [inline] contract (master semantics)
                if ( site.kind!=SiteKind::MustCall && statementHasTableLookupCollision(site.stmt) ) {
                    decline(site, "the statement has a potential table lookup collision (error 30250 preserved for lint)", callLike->at);
                    continue;
                }
                if ( site.kind==SiteKind::AutoCall && calleeFn ) {
                    int64_t calleeBytes = bodyCost(calleeFn->body).stackBytes;
                    if ( grownBytes + calleeBytes > AUTO_INLINE_CALLER_GROWTH_BYTES ) {
                        decline(site, "caller frame growth budget exhausted this round", callLike->at);
                        continue;
                    }
                    grownBytes += calleeBytes;
                }
                // ----- subject shape gates -----
                ExprBlock * body = nullptr;
                TypeDecl * subjResult = nullptr;    // null = void result
                SiteArgs sa;
                vector<ExpressionPtr> opArgs;   // operator sites: operands as an argument vector
                bool needScope = false;         // wrap the spliced body in with (module <origin>)
                string scopeModule;
                if ( site.kind==SiteKind::Devirt ) {
                    string why;
                    if ( !devirtShapeOk(literal, why) ) { decline(site, why, callLike->at); continue; }
                    auto blk = static_cast<ExprBlock *>(literal->block);
                    auto inv = static_cast<ExprInvoke *>(callLike);
                    if ( inv->arguments.size() != blk->arguments.size()+1 ) { decline(site, "argument count mismatch", callLike->at); continue; }
                    body = blk;
                    if ( blk->returnType && !blk->returnType->isVoid() ) subjResult = blk->returnType;
                    sa.args = &inv->arguments; sa.ofs = 1; sa.params = &blk->arguments;
                } else {
                    if ( site.kind==SiteKind::MustCall ) {
                        if ( !calleeShapeOk(calleeFn) ) continue;   // lint reports at the declaration
                    } else {
                        string why;
                        if ( !autoCalleeOk(calleeFn, why) ) { decline(site, subjName + ": " + why, callLike->at); continue; }
                    }
                    if ( callLike->rtti_isCall() ) {
                        auto call = static_cast<ExprCall *>(callLike);
                        if ( call->arguments.size() != calleeFn->arguments.size() ) continue; // not fully inferred - defensive
                        sa.args = &call->arguments;
                    } else {
                        // an operator site: view the operands as an argument vector. read-only
                        // by construction - args are cloned into temps or substitution plans,
                        // and the op node itself is replaced wholesale at the end
                        if ( callLike->rtti_isOp1() ) {
                            opArgs.push_back(static_cast<ExprOp1 *>(callLike)->subexpr);
                        } else if ( callLike->rtti_isOp2() ) {
                            auto op2 = static_cast<ExprOp2 *>(callLike);
                            opArgs.push_back(op2->left);
                            opArgs.push_back(op2->right);
                        } else {
                            auto op3 = static_cast<ExprOp3 *>(callLike);
                            opArgs.push_back(op3->subexpr);
                            opArgs.push_back(op3->left);
                            opArgs.push_back(op3->right);
                        }
                        if ( opArgs.size() != calleeFn->arguments.size() ) continue; // defensive
                        sa.args = &opArgs;
                    }
                    // a generic instance lives in the CALLER's module - judge its home
                    // (and the reachability of its references) by the generic's origin
                    Module * originModule = calleeFn->fromGeneric
                        ? calleeFn->fromGeneric->module : calleeFn->module;
                    // a site under a `with (module ...)` scope resolves at THAT module:
                    // only a body from the same module keeps its meaning spliced bare
                    // there (a wrap for any other module would fight the enclosing scope).
                    // user-written scopes may spell a require path - compare the last leg
                    string siteWith = site.withModule;
                    auto slash = siteWith.find_last_of('/');
                    if ( slash != string::npos ) siteWith.erase(0, slash+1);
                    if ( !siteWith.empty() && siteWith != originModule->name ) {
                        siteFail(site, "can't inline " + subjName + " here: the call site resolves inside with (module "
                            + site.withModule + ")", callLike->at);
                        continue;
                    }
                    if ( originModule != program->thisModule.get() ) {
                        auto & verdict = privateUse(calleeFn, originModule);
                        if ( !verdict.hard.empty() ) {
                            siteFail(site, "can't inline " + subjName + " across modules: " + verdict.hardWhy, callLike->at);
                            continue;
                        }
                        // resolution-only needs splice under a generated
                        // `with (module <origin>)` - see the wrap below. a site already
                        // inside with (module <origin>) splices bare: the enclosing scope
                        // is the resolution context (wrapping again would re-wrap interior
                        // sites every round - the patch pass would never converge)
                        needScope = !verdict.scope.empty()
                            && siteWith != originModule->name;
                        if ( needScope ) scopeModule = originModule->name;
                    }
                    body = static_cast<ExprBlock *>(calleeFn->body);
                    if ( calleeFn->result && !calleeFn->result->isVoid() ) subjResult = calleeFn->result;
                    sa.ofs = 0; sa.params = &calleeFn->arguments;
                }
                // a callee body may itself contain manufactured _inl locals (interiors
                // splice before their callers within a round, and the counter is
                // function-scoped) - the site id must clear those too, or the rename map
                // captures this site's synthesized names (the result store would retarget
                // the callee's interior temp: a wrong-type error, or worse, a silent
                // uninitialized read when the types happen to agree)
                {
                    int calleeNext = nextInlineIdIn(body);
                    if ( calleeNext > inlineId ) inlineId = calleeNext;
                }
                // ----- classify arguments -----
                auto & stats = paramStats(body, *sa.params);
                bool isVoid = subjResult==nullptr;
                // a move return (`return <- x`) never substitutes as an expression: the
                // substituted read would COPY where the callee moved (an error for
                // non-copyables, a semantic change otherwise) - it takes the result-temp
                // path, which preserves the move. an unsafe body also takes the result-temp
                // path: its generated `unsafe { }` wrapper must be a statement, so that
                // every later-round splice inside it anchors INSIDE the wrapper's block
                // (nothing unsafe can hoist out of its authorization region)
                // a scope-needing body always takes the statement path: the with (module)
                // wrapper is a statement, and a pure graft would re-resolve at the caller
                bool exprBody = body->list.size()==1 && body->list.back()->rtti_isReturn()
                    && !static_cast<ExprReturn *>(body->list.back())->moveSemantics
                    && !(calleeFn && calleeFn->hasUnsafe)
                    && !needScope;
                bool writeFree = calleeFn ? calleeWriteFree(calleeFn) : false;  // a block body is judged unknown
                das_hash_map<Variable *, ArgSub> paramSub;
                vector<ExpressionPtr> temps;
                bool argFlavorFail = false;
                for ( size_t ai=0, ais=sa.count(); ai!=ais; ++ai ) {
                    Expression * A = sa.arg(ai);
                    auto & P = sa.param(ai);
                    ArgSub sub;
                    bool byRefParam = P->type->ref || P->type->isRefType();
                    bool varParam = !P->type->constant;
                    Expression * leafA = A;
                    if ( leafA->rtti_isR2V() ) leafA = static_cast<ExprRef2Value *>(leafA)->subexpr;
                    bool leafConst = leafA->rtti_isConstant();
                    bool leafVar = leafA->rtti_isVar();
                    // a non-copyable CONST source cannot move into its temp (30940) - the
                    // original call only ever BOUND it; the temp clone-initializes instead
                    // a by-value temp is a local, and infer rejects a local of a type that cannot be
                    // one: a non-local annotation needs unsafe (31020), and a type that neither
                    // copies nor moves can only be built by its constructor (30199). Refusing the
                    // site keeps the call as a call - manufacturing the temp anyway would report
                    // those errors against a generated name the author cannot act on.
                    auto tempTypeIsLocal = [&]( Expression * init, bool ref ) {
                        if ( ref || !init->type ) return true;
                        // infer exempts block types from the isLocal test - a block local is legal,
                        // it just has to be initialized with a make-block - so leave those alone
                        if ( init->type->isGoodBlockType() ) return !fn->generator;
                        if ( !init->type->isLocal() ) return false;
                        return init->type->canCopy() || init->type->canMove()
                            || !init->type->hasNonTrivialCtor();
                    };
                    auto makeArgTemp = [&]( Expression * init, bool cnst, bool ref, bool viaMove ) {
                        if ( !tempTypeIsLocal(init, ref) ) {
                            siteFail(site, "can't inline " + subjName + ": argument '" + P->name
                                + "' needs a temporary, and " + init->type->describe()
                                + " can't be a local variable", callLike->at);
                            argFlavorFail = true;
                            return;
                        }
                        string tname = INLINE_TEMP_PREFIX + to_string(inlineId) + "_arg_" + P->name;
                        bool viaClone = viaMove && init->type && init->type->constant;
                        temps.push_back(makeTemp(callLike->at, tname, init, cnst, ref, viaMove && !viaClone, viaClone));
                        sub.tempName = tname;
                    };
                    if ( leafA->rtti_isTypeDecl() ) {
                        // type<...> witness: a compile-time tag - a temp would "use" it.
                        // a resolved witness carries its type by pointer and crosses a
                        // module scope safely; an alias would re-resolve in the wrong module
                        if ( needScope ) {
                            auto td = static_cast<ExprTypeDecl *>(leafA);
                            if ( td->typeexpr && td->typeexpr->isAutoOrAlias() ) {
                                siteFail(site, "can't inline " + subjName + ": type witness argument '"
                                    + P->name + "' can't cross the module scope", callLike->at);
                                argFlavorFail = true;
                                break;
                            }
                        }
                        sub.substitute = leafA;
                        paramSub[P] = sub;
                        continue;
                    }
                    // an iterator argument BORROWS whatever storage its expression built
                    // (`each(<temporary array>)` being the canonical case). the real call
                    // keeps that storage alive for the whole call statement; a manufactured
                    // arg temp shrinks it to the temp's own statement, and the spliced body
                    // then iterates freed memory (proven: linq fold cascade - empty results,
                    // heap crash; latent for v1's devirt too). only a plain variable read is
                    // borrow-neutral - anything composed declines
                    if ( A->type && A->type->baseType==Type::tIterator && !leafVar ) {
                        siteFail(site, "can't inline " + subjName + ": iterator argument '"
                            + P->name + "' borrows call-scoped storage", callLike->at);
                        argFlavorFail = true;
                        break;
                    }
                    // a substitution (or an inferred temp) carries the ARGUMENT's exact type
                    // into the body, where the call boundary coerced it to the PARAM's type.
                    // beyond top-level ref/const (which the machinery navigates), a flavor
                    // change is a SOUNDNESS boundary, not just a registry artifact: an
                    // element-const view of mutable storage spliced inline hands the
                    // optimizer const types it is licensed to trust (proven value
                    // miscompile: linq's lazy group_by pipeline lost bucket elements),
                    // and an already-typed block literal cannot re-match a re-typed
                    // parameter (block argTypes compare strictly on re-infer). the
                    // locked-name fallback heals RESOLUTION, not these; best-effort
                    // sites decline, MUST keeps its pre-existing contract
                    if ( site.kind!=SiteKind::MustCall && A->type && P->type ) {
                        // gc_local: comparison-only temporaries, freed at scope exit
                        // instead of lingering on the gc root until the next sweep
                        auto stripTop = [](const TypeDecl * t) {
                            auto c = new TypeDecl(*t);
                            c->ref = false;
                            c->constant = false;
                            return gc_local<TypeDecl>(c);
                        };
                        if ( !stripTop(A->type)->isSameType(*stripTop(P->type), RefMatters::yes,
                                ConstMatters::yes, TemporaryMatters::yes, AllowSubstitute::no, false) ) {
                            decline(site, subjName + ": argument '" + P->name + "' flavor '"
                                + A->type->describe() + "' vs parameter '" + P->type->describe() + "'", callLike->at);
                            argFlavorFail = true;
                            break;
                        }
                    }
                    // a spliced literal's locals rename and re-infer with the site
                    if ( site.kind!=SiteKind::MustCall && leafA->rtti_isMakeBlock() && reinferFragile(leafA) ) {
                        decline(site, subjName + ": block argument carries a temporary-flavored call", callLike->at);
                        argFlavorFail = true;
                        break;
                    }
                    if ( byRefParam ) {
                        // a substitution must not change the argument's CONST VIEW: the
                        // body was typed against the param's constness, and a non-const
                        // var replacing a const param read re-types body subtrees the
                        // optimizer already trusts (the `:=` clone family being the
                        // proven case). when the param is more const than the leaf,
                        // bind a const reference instead. under a module-scope wrap a
                        // GLOBAL var's unqualified name would re-resolve in the callee's
                        // module - it binds a reference outside the wrap instead
                        bool sameConstView = !P->type->constant || (leafA->type && leafA->type->constant);
                        bool globalUnderScope = needScope && leafVar
                            && static_cast<ExprVar *>(leafA)->isGlobalVariable();
                        if ( leafVar && sameConstView && !globalUnderScope ) {
                            sub.substitute = leafA;             // same aliasing as the call itself
                        } else if ( leafVar ) {
                            // const-widened var: bind the const reference explicitly - a bare
                            // var arg often carries no ref flag, and falling through would
                            // materialize (and MOVE a non-copyable) instead of aliasing
                            auto init = A->clone();
                            init->alwaysSafe = true;            // generated binding to real storage
                            makeArgTemp(init, !varParam, true, false);
                        } else if ( leafA->rtti_isMakeBlock() ) {
                            // a block literal read once outside loops substitutes textually,
                            // keeping shapes a holder can't reproduce (temporary-typed block
                            // parameters do not survive a `var <-` binding). multi-read and
                            // under-loop literals bind a holder; devirtualization takes over.
                            // under a module-scope wrap the literal is caller code and MUST
                            // bind a holder outside - a `#` parameter can't, so it declines
                            int reads = 0;
                            auto rit = stats.readCount.find(P);
                            if ( rit != stats.readCount.end() ) reads = rit->second;
                            bool underLoop = stats.readUnderLoop.find(P)!=stats.readUnderLoop.end();
                            if ( reads<=1 && !underLoop && !needScope ) {
                                sub.substitute = leafA;
                            } else if ( needScope && P->type->temporary ) {
                                siteFail(site, "can't inline " + subjName + ": block argument '"
                                    + P->name + "' can't bind outside the module scope", callLike->at);
                                argFlavorFail = true;
                                break;
                            } else {
                                makeArgTemp(A->clone(), false, false, true);
                            }
                        } else if ( A->type && A->type->ref ) {
                            // lvalue chain (field/index), or a var whose const view the
                            // param widens: bind a reference once, like the call did
                            auto init = A->clone();
                            init->alwaysSafe = true;            // generated binding to real storage
                            makeArgTemp(init, !varParam, true, false);
                        } else {
                            // rvalue into a ref param: materialize the value. constness
                            // follows the param (a var-ref param - e.g. a consumed iterator -
                            // needs a mutable holder), and a block always binds `var`: a const
                            // holder const-propagates into the invoke, rejecting the block's
                            // own var parameters
                            bool nonCopyable = A->type && !A->type->canCopy();
                            bool blockValue = A->type && A->type->baseType==Type::tBlock;
                            makeArgTemp(A->clone(), !varParam && !blockValue, false, nonCopyable);
                        }
                    } else if ( varParam ) {
                        // a mutable by-value param IS a local copy
                        makeArgTemp(A->clone(), false, false, A->type && !A->type->canCopy());
                    } else if ( leafConst ) {
                        if ( stats.readAsRefArg.find(P)!=stats.readAsRefArg.end() ) {
                            makeArgTemp(A->clone(), true, false, false);
                        } else
                        // an enum constant re-resolves its enumeration by name - under a
                        // module-scope wrap that name lands in the wrong module; bind it out
                        if ( needScope && leafA->type && leafA->type->isEnumT() ) {
                            makeArgTemp(A->clone(), true, false, false);
                        } else {
                            sub.substitute = leafA;
                        }
                    } else if ( leafVar ) {
                        auto av = static_cast<ExprVar *>(leafA);
                        // snapshot hazard: the spliced body could write the same storage
                        // through a `var` by-ref param of this very call, or through globals
                        bool hazard = false;
                        if ( av->variable ) {
                            for ( size_t oi=0; oi!=ais; ++oi ) {
                                if ( oi==ai ) continue;
                                auto & OP = sa.param(oi);
                                bool oRef = OP->type->ref || OP->type->isRefType();
                                if ( !oRef || OP->type->constant ) continue;
                                Expression * oleaf = sa.arg(oi);
                                if ( oleaf->rtti_isR2V() ) oleaf = static_cast<ExprRef2Value *>(oleaf)->subexpr;
                                if ( oleaf->rtti_isVar() && static_cast<ExprVar *>(oleaf)->variable==av->variable ) {
                                    hazard = true;
                                    break;
                                }
                            }
                            if ( !hazard && (av->isGlobalVariable() || av->variable->access_ref) ) {
                                // a global, or a local whose address escaped: the spliced
                                // body's writes could reach it, breaking snapshot semantics
                                hazard = !writeFree;
                            }
                            // a global's unqualified name would re-resolve inside the wrap
                            if ( needScope && av->isGlobalVariable() ) hazard = true;
                        }
                        // substituting a MUTABLE variable for a const parameter re-flavors
                        // every read of it inside the body. the body was typed against the
                        // const view, and an ==const-locked generic instance it calls (the
                        // push/clone family) no longer matches its own locked name on
                        // re-infer - 30341 at a call the author never wrote. bind a const
                        // temp instead, exactly as the by-ref path does for this case
                        bool constWidened = P->type->constant && !(leafA->type && leafA->type->constant);
                        if ( hazard || constWidened ) {
                            makeArgTemp(A->clone(), true, false, false);
                        } else {
                            sub.substitute = leafA;
                        }
                    } else {
                        // tier B: pure, read at most once, never under a loop. a composed
                        // caller expression may carry calls and globals that must keep
                        // resolving at the caller - under a module-scope wrap it binds out
                        int reads = 0;
                        auto rit = stats.readCount.find(P);
                        if ( rit != stats.readCount.end() ) reads = rit->second;
                        bool underLoop = stats.readUnderLoop.find(P)!=stats.readUnderLoop.end();
                        bool orderSafe = writeFree || argReadsOnlyPrivateLocals(A, sa);
                        bool needsStorage = stats.readAsRefArg.find(P)!=stats.readAsRefArg.end()
                            && !(A->type && A->type->ref);
                        if ( reads<=1 && !underLoop && inlinePure(A) && orderSafe && !needScope && !needsStorage ) {
                            sub.substitute = A;
                        } else {
                            makeArgTemp(A->clone(), true, false, A->type && !A->type->canCopy());
                        }
                    }
                    paramSub[P] = sub;
                }
                if ( argFlavorFail ) continue;
                bool needStatements = !temps.empty() || !exprBody;
                if ( !needStatements ) {
                    // ----- pure graft: no statements, no anchors, legal in any position -----
                    // an expression body declares no LOCALS, but a block literal inside it declares
                    // ARGUMENTS, and after the graft those sit in the caller's scope chain. infer
                    // re-resolves every ExprVar by name, so an unrenamed one captures a substituted
                    // argument expression that happens to spell the same name: 30701 for an ordinary
                    // block, and for a can_shadow block (host query blocks, linq_boost) SILENTLY the
                    // wrong variable, since can_shadow is what switches 30701 off. same collector the
                    // statement path below uses
                    das_hash_set<string> argHazard;
                    {
                        // every actual argument, not just tier substitutes: a block-literal
                        // argument moves into the body wholesale, and its free names can be
                        // captured the same way
                        FreeNameCollect fnc;
                        fnc.names = &argHazard;
                        for ( size_t ai=0, ais=sa.count(); ai!=ais; ++ai ) sa.arg(ai)->visit(fnc);
                    }
                    das_hash_map<string, string> rename;
                    das_hash_set<string> canShadowArgs;
                    LocalNameCollect pnames;
                    pnames.rename = &rename;
                    pnames.canShadowArgs = &canShadowArgs;
                    pnames.prefix = INLINE_TEMP_PREFIX + to_string(inlineId) + "_l_";
                    body->visit(pnames);
                    // a substituted caller expression under a can_shadow block argument of the
                    // same name re-resolves to the block's variable - silently, that is what
                    // can_shadow means - and the argument cannot be renamed (its name is the
                    // component). no rename is correct here; do not inline this site.
                    bool captureRisk = false;
                    for ( auto & csn : canShadowArgs ) {
                        if ( argHazard.count(csn) || callerDeclNames.count(csn) ) { captureRisk = true; break; }
                    }
                    if ( captureRisk ) {
                        siteFail(site, "can't inline " + subjName
                            + ": a substituted expression would be captured by a can_shadow block argument", callLike->at);
                        continue;
                    }
                    InlineBodyRewriter rewriter;
                    rewriter.paramSub = &paramSub;
                    rewriter.rename = &rename;
                    rewriter.tempAt = callLike->at;
                    rewriter.clearUserUnsafe = calleeFn != nullptr;
                    auto ret = static_cast<ExprReturn *>(body->list.back());
                    ExpressionPtr callReplacement = nullptr;
                    if ( ret->subexpr ) {
                        callReplacement = ret->subexpr->clone()->visit(rewriter);
                    }
                    if ( site.stmt==callLike ) {
                        auto & list = site.anchor.block->list;
                        if ( callReplacement ) {
                            list[anchorIndex] = callReplacement;
                        } else {
                            list.erase(list.begin()+anchorIndex);
                            indexShift[site.anchor.block] -= 1;
                        }
                    } else {
                        ReplaceNode rn;
                        rn.what = callLike;
                        rn.with = callReplacement;
                        if ( !callReplacement ) {
                            siteFail(site, "can't inline void " + subjName + " here: the call is not a statement", callLike->at);
                            continue;
                        }
                        auto & slot = site.anchor.block->list[anchorIndex];
                        slot = slot->visit(rn);
                        if ( !rn.done ) {
                            error("internal error: inlined call not found in its statement", callLike->at);
                            continue;
                        }
                    }
                    completeSite(site, fn, subjName);
                    inlineId ++;
                    continue;
                }
                // ----- statements needed: check the position, lower if necessary -----
                if ( site.stmt->rtti_isWhile() ) {
                    auto wh = static_cast<ExprWhile *>(site.stmt);
                    if ( exprContains(wh->cond, callLike) && wh->body->rtti_isBlock() ) {
                        // while(C) -> while(true) { if (!(C)) break; ... } - the condition
                        // becomes a statement-anchored expression, next round splices there
                        auto brkBlock = new ExprBlock();
                        brkBlock->at = wh->cond->at;
                        brkBlock->list.push_back(new ExprBreak(wh->cond->at));
                        auto guard = new ExprIfThenElse(wh->cond->at,
                            new ExprOp1(wh->cond->at, "!", wh->cond), brkBlock, nullptr);
                        auto oldBody = static_cast<ExprBlock *>(wh->body);
                        oldBody->list.insert(oldBody->list.begin(), guard);
                        wh->cond = new ExprConstBool(wh->cond->at, true);
                        changed = true;
                        continue;
                    }
                }
                if ( site.stmt->rtti_isIfThenElse() ) {
                    // call in an elif condition: blockify that else so the inner if
                    // becomes an anchorable statement of its own
                    auto ite = static_cast<ExprIfThenElse *>(site.stmt);
                    bool lowered = false;
                    while ( ite->if_false && ite->if_false->rtti_isIfThenElse() ) {
                        auto inner = static_cast<ExprIfThenElse *>(ite->if_false);
                        if ( exprContains(inner->cond, callLike) || exprContains(ite->if_false, callLike) ) {
                            auto blk = new ExprBlock();
                            blk->at = inner->at;
                            blk->list.push_back(ite->if_false);
                            ite->if_false = blk;
                            lowered = true;
                            break;
                        }
                        ite = inner;
                    }
                    if ( lowered ) {
                        changed = true;
                        continue;
                    }
                }
                PathScan path;
                auto pos = findPath(site.stmt, callLike, path);
                if ( pos==SitePosition::NotFound ) {
                    siteFail(site, "can't inline " + subjName + " here: unsupported call position", callLike->at);
                    continue;
                }
                if ( pos==SitePosition::Unsupported ) {
                    siteFail(site, "can't inline " + subjName + " inside ?? or a safe-navigation suffix - hoist the call into its own statement", callLike->at);
                    continue;
                }
                if ( pos==SitePosition::Lazy ) {
                    auto lazy = path.lazyOps.back();    // outermost lazy op on the path
                    // `a &&= call()` / `a ||= call()` evaluate the RHS conditionally and
                    // produce no value, so the temp-hoist path below cannot apply; lower
                    // the statement itself to if-form: `if (a) a = call()` (negated for
                    // ||=). the LHS is then read twice, so only a plain variable
                    // qualifies - anything else declines and keeps the call
                    if ( lazy->rtti_isOp2() ) {
                        auto sop = static_cast<ExprOp2 *>(lazy);
                        if ( sop->op=="&&=" || sop->op=="||=" ) {
                            if ( site.stmt!=lazy || !sop->left->rtti_isVar() ) {
                                siteFail(site, "can't inline " + subjName + " in the right side of " + sop->op + " - hoist the call into its own statement", callLike->at);
                                continue;
                            }
                            auto & list = site.anchor.block->list;
                            auto thenBlk = new ExprBlock();
                            thenBlk->at = sop->at;
                            thenBlk->list.push_back(new ExprCopy(sop->at, sop->left->clone(), sop->right));
                            ExpressionPtr cond = sop->left->clone();
                            if ( sop->op=="||=" ) cond = new ExprOp1(sop->at, "!", cond);
                            list[anchorIndex] = new ExprIfThenElse(sop->at, cond, thenBlk, nullptr);
                            changed = true;
                            continue;
                        }
                    }
                    // a ternary lowers by SPLITTING into bare arm stores (the fresh hoist
                    // rides the same var+if path a round later), and a bare store loses
                    // the per-arm coercion the ternary provided - a void? panic arm into
                    // a typed pointer (the macro-generated `as` guard being the canonical
                    // case), null literals, derived views - and copies a ref result by
                    // value. only same-typed, by-value arms survive the split; anything
                    // else keeps the call
                    if ( lazy->rtti_isOp3() ) {
                        auto op3 = static_cast<ExprOp3 *>(lazy);
                        auto & lt = op3->left->type;
                        auto & rt = op3->right->type;
                        bool armsMatch = lt && rt && lazy->type && !lazy->type->ref
                            && !lt->isVoid() && !rt->isVoid()
                            && !lt->isVoidPointer() && !rt->isVoidPointer()
                            && lt->isSameType(*rt, RefMatters::no, ConstMatters::no, TemporaryMatters::yes);
                        if ( !armsMatch ) {
                            siteFail(site, "can't inline " + subjName + " inside a mixed-type ternary - hoist the call into its own statement", callLike->at);
                            continue;
                        }
                    }
                    // when the lazy op is the whole init of a single GENERATED temp (a
                    // hoist from a previous round), rewrite it to var + if form in place.
                    // a USER declaration never takes this branch - replacing it would drop
                    // its metadata (constness, aka, ref-ness); it hoists to a fresh temp
                    // instead, and the next round rewrites that temp
                    Variable * rootVar = nullptr;
                    if ( site.stmt->rtti_isLet() ) {
                        auto let = static_cast<ExprLet *>(site.stmt);
                        if ( let->variables.size()==1 && let->variables[0]->init==lazy
                            && let->variables[0]->generated ) {
                            rootVar = let->variables[0];
                        }
                    }
                    auto & list = site.anchor.block->list;
                    if ( !rootVar ) {
                        string tname = INLINE_TEMP_PREFIX + to_string(inlineId) + "_low";
                        inlineId ++;
                        // a non-const temp: a `var p = <temp>` consumer of pointer type
                        // rejects initialization from a const reference. a non-copyable
                        // lazy result (ternary of arrays) must move into the hoist
                        bool viaMove = lazy->type && !lazy->type->canCopy();
                        bool viaClone = viaMove && lazy->type->constant;   // move-from-const is 30940
                        auto hoist = makeTemp(callLike->at, tname, lazy, false, false, viaMove && !viaClone, viaClone);
                        ReplaceNode rn;
                        rn.what = lazy;
                        rn.with = new ExprVar(callLike->at, tname);
                        list[anchorIndex] = list[anchorIndex]->visit(rn);
                        if ( !rn.done ) {
                            error("internal error: lazy operator not found in its statement", callLike->at);
                            continue;
                        }
                        list.insert(list.begin()+anchorIndex, hoist);
                        indexShift[site.anchor.block] += 1;
                        changed = true;
                        continue;
                    }
                    if ( !lazy->type ) {
                        siteFail(site, "can't inline " + subjName + " here: missing type on the lazy operator", callLike->at);
                        continue;
                    }
                    // the arms store into an uninitialized declaration of the operator's type, which
                    // a type whose C++ constructor must run cannot take (30316)
                    if ( !lazy->type->ref && lazy->type->hasNonTrivialCtor() ) {
                        siteFail(site, "can't inline " + subjName + " here: lazy operator of type "
                            + lazy->type->describe() + " requires nontrivial construction", callLike->at);
                        continue;
                    }
                    vector<ExpressionPtr> replacement;
                    replacement.push_back(makeUninitDecl(site.stmt->at, rootVar->name, lazy->type));
                    auto readT = [&]() { return new ExprVar(site.stmt->at, rootVar->name); };
                    // the arm stores mirror the hoist's own init semantics: a temp that was
                    // move-initialized (non-copyable lazy result) moves from the selected arm
                    auto armStore = [&]( const LineInfo & at, Expression * src ) -> Expression * {
                        if ( rootVar->init_via_move ) return new ExprMove(at, readT(), src);
                        return new ExprCopy(at, readT(), src);
                    };
                    if ( lazy->rtti_isOp3() ) {
                        auto op3 = static_cast<ExprOp3 *>(lazy);
                        auto thenBlk = new ExprBlock();
                        thenBlk->at = op3->at;
                        thenBlk->list.push_back(armStore(op3->at, op3->left));
                        auto elseBlk = new ExprBlock();
                        elseBlk->at = op3->at;
                        elseBlk->list.push_back(armStore(op3->at, op3->right));
                        replacement.push_back(new ExprIfThenElse(op3->at, op3->subexpr, thenBlk, elseBlk));
                    } else if ( lazy->rtti_isOp2() ) {
                        auto op2 = static_cast<ExprOp2 *>(lazy);
                        replacement.push_back(new ExprCopy(op2->at, readT(), op2->left));
                        auto thenBlk = new ExprBlock();
                        thenBlk->at = op2->at;
                        thenBlk->list.push_back(new ExprCopy(op2->at, readT(), op2->right));
                        ExpressionPtr cond = readT();
                        if ( op2->op=="||" ) cond = new ExprOp1(op2->at, "!", cond);
                        replacement.push_back(new ExprIfThenElse(op2->at, cond, thenBlk, nullptr));
                    } else {
                        siteFail(site, "can't inline " + subjName + " here: unsupported lazy operator", callLike->at);
                        continue;
                    }
                    list.erase(list.begin()+anchorIndex);
                    list.insert(list.begin()+anchorIndex, replacement.begin(), replacement.end());
                    indexShift[site.anchor.block] += int(replacement.size()) - 1;
                    changed = true;
                    continue;
                }
                // ----- eager position: hoist the impure prefix, splice temps and body -----
                // ALL declinable checks run BEFORE the prefix hoist mutates the statement: a
                // `continue` after ReplaceNode has rewritten the statement discards the pending
                // temp declarations but keeps the _inl<N>_pre* references, and the orphaned
                // ExprVar later fails infer (30838) or segfaults the access-flags pass
                vector<Expression *> prefix;
                collectImpurePrefix(site.stmt, callLike, prefix);
                bool prefixFailed = false;
                for ( auto pe : prefix ) {
                    if ( pe->type && pe->type->isVoid() ) {
                        // a void expression can only BE the statement, and then it has no prefix
                        siteFail(site, "can't inline " + subjName + " here: void expression in the evaluation prefix", callLike->at);
                        prefixFailed = true;
                        break;
                    }
                    if ( pe->type && pe->type->baseType==Type::tIterator && !pe->rtti_isVar() ) {
                        // same borrow hazard as iterator arguments: the hoist statement
                        // would outlive the storage the iterator expression borrowed
                        siteFail(site, "can't inline " + subjName + " here: iterator expression in the evaluation prefix", callLike->at);
                        prefixFailed = true;
                        break;
                    }
                }
                if ( prefixFailed ) continue;
                // the statement path stores into an UNINITIALIZED result temp, and that temp is a
                // local: a type whose C++ constructor must run cannot be declared that way (30316).
                // the [inline] and block shape checks only cover a refType result, so a by-value
                // result with a nontrivial ctor reaches here
                if ( subjResult && !subjResult->ref && subjResult->hasNonTrivialCtor() ) {
                    siteFail(site, "can't inline " + subjName + ": result type "
                        + subjResult->describe() + " requires nontrivial construction", callLike->at);
                    continue;
                }
                das_hash_set<string> argHazard;
                {
                    // every actual argument, not just tier substitutes: a block-literal
                    // argument moves into the body wholesale, and its free names can be
                    // captured the same way. collected before the prefix hoist rewrites the
                    // arguments - conservative: a name the hoist would have moved to statement
                    // level still counts as hazardous
                    FreeNameCollect fnc;
                    fnc.names = &argHazard;
                    for ( size_t ai=0, ais=sa.count(); ai!=ais; ++ai ) sa.arg(ai)->visit(fnc);
                }
                das_hash_map<string, string> rename;
                das_hash_set<string> canShadowArgs;
                LocalNameCollect names;
                names.rename = &rename;
                names.canShadowArgs = &canShadowArgs;
                // renamed locals live in the _l_ sub-namespace so a callee local named
                // `res` (or `arg_x`, `pre0`, `low`) can never collide with the
                // manufactured _res/_arg_*/_pre*/_low temps of the same site
                names.prefix = INLINE_TEMP_PREFIX + to_string(inlineId) + "_l_";
                body->visit(names);
                bool captureRisk = false;
                for ( auto & csn : canShadowArgs ) {
                    if ( argHazard.count(csn) || callerDeclNames.count(csn) ) { captureRisk = true; break; }
                }
                if ( captureRisk ) {
                    siteFail(site, "can't inline " + subjName
                        + ": a substituted expression would be captured by a can_shadow block argument", callLike->at);
                    continue;
                }
                vector<ExpressionPtr> splice;
                int preIdx = 0;
                for ( auto pe : prefix ) {
                    string tname = INLINE_TEMP_PREFIX + to_string(inlineId) + "_pre" + to_string(preIdx++);
                    // non-const temps throughout: the hoisted expression was a non-const
                    // rvalue (or keeps its lvalue constness via the reference binding), and
                    // a const temp read would no longer match var pointer params downstream
                    if ( pe->type && pe->type->ref ) {
                        // lvalue: bind a reference once - evaluation order and identity
                        // both survive (writes through the temp land in the original place)
                        pe->alwaysSafe = true;          // generated binding to real storage
                        splice.push_back(makeTemp(pe->at, tname, pe, pe->type->constant, true, false));
                    } else if ( pe->type && !pe->type->canCopy() ) {
                        // move the rvalue; a CONST non-copyable rvalue clone-initializes (30940)
                        bool viaClone = pe->type->constant;
                        splice.push_back(makeTemp(pe->at, tname, pe, false, false, !viaClone, viaClone));
                    } else {
                        splice.push_back(makeTemp(pe->at, tname, pe, false, false, false));
                    }
                    ReplaceNode rn;
                    rn.what = pe;
                    rn.with = new ExprVar(pe->at, tname);
                    auto & slot = site.anchor.block->list[anchorIndex];
                    slot = slot->visit(rn);
                    if ( !rn.done ) {
                        error("internal error: prefix expression not found in its statement", callLike->at);
                        prefixFailed = true;
                        break;
                    }
                }
                if ( prefixFailed ) continue;
                // body
                InlineBodyRewriter rewriter;
                rewriter.paramSub = &paramSub;
                rewriter.rename = &rename;
                rewriter.tempAt = callLike->at;
                rewriter.clearUserUnsafe = calleeFn != nullptr;
                ExpressionPtr callReplacement = nullptr;
                if ( exprBody ) {
                    auto ret = static_cast<ExprReturn *>(body->list.back());
                    if ( ret->subexpr ) {
                        callReplacement = ret->subexpr->clone()->visit(rewriter);
                    }
                    for ( auto & t : temps ) splice.push_back(t);
                } else {
                    auto bodyClone = static_cast<ExprBlock *>(body->clone());
                    if ( site.kind==SiteKind::Devirt ) {
                        // the literal's parameters became substitutions and its result
                        // becomes the result temp: the spliced copy is a plain scope block
                        bodyClone->arguments.clear();
                        bodyClone->annotations.clear();
                        bodyClone->returnType = nullptr;
                        bodyClone->isClosure = false;
                        bodyClone->isLambdaBlock = false;
                        bodyClone->hasReturn = false;
                        bodyClone->copyOnReturn = false;
                        bodyClone->moveOnReturn = false;
                    }
                    // every function-level return in the clone becomes a result store
                    // (terminal shapes in place, the rest through a generated bool flag -
                    // see ReturnStoreRewrite). the standalone callee keeps its shape
                    ReturnStoreRewrite rsr;
                    rsr.flagName = INLINE_TEMP_PREFIX + to_string(inlineId) + "_ret";
                    if ( !isVoid ) {
                        rsr.resName = INLINE_TEMP_PREFIX + to_string(inlineId) + "_res";
                        splice.push_back(makeUninitDecl(callLike->at, rsr.resName, subjResult));
                    }
                    rsr.apply(bodyClone);
                    if ( !isVoid ) {
                        callReplacement = new ExprVar(callLike->at, rsr.resName);
                    }
                    ExpressionPtr spliced = bodyClone->visit(rewriter);
                    if ( calleeFn && calleeFn->hasUnsafe ) {
                        // a compiled callee lost its `unsafe { }` wrappers to foldUnsafe, and
                        // the spliced body re-infers in the caller: re-authorize it under a
                        // generated wrapper. generated = invisible to the no_unsafe policy
                        // and to the caller module's unsafe accounting (ast_lint.cpp)
                        auto wrap = new ExprUnsafe(callLike->at);
                        wrap->body = spliced;
                        wrap->generated = true;
                        spliced = wrap;
                    }
                    if ( needScope ) {
                        // module-flavored with: the body re-resolves as if written in the
                        // callee's module - privates, foreign instances, and its require
                        // graph land exactly where the callee's own compile put them. arg
                        // and prefix temps stay OUTSIDE (caller expressions must keep
                        // resolving at the caller - the mirror rule); param-temp, result
                        // and flag reads inside are locals, lexical and unaffected.
                        // generated = exempt from the with_module_is_unsafe policy. the
                        // unsafe wrapper stays INSIDE so later-round splices keep
                        // anchoring within their authorization region
                        ExprBlock * host = nullptr;
                        if ( spliced->rtti_isBlock() ) {
                            host = static_cast<ExprBlock *>(spliced);
                        } else {
                            host = new ExprBlock();
                            host->at = callLike->at;
                            host->list.push_back(spliced);
                        }
                        auto scopeWrap = new ExprWith(callLike->at);
                        scopeWrap->moduleName = scopeModule;
                        scopeWrap->generated = true;
                        scopeWrap->body = host;
                        spliced = scopeWrap;
                    }
                    if ( !temps.empty() || rsr.flagUsed ) {
                        auto scope = new ExprBlock();
                        scope->at = callLike->at;
                        for ( auto & t : temps ) scope->list.push_back(t);
                        if ( rsr.flagUsed ) {
                            scope->list.push_back(makeTemp(callLike->at, rsr.flagName,
                                new ExprConstBool(callLike->at, false), false, false, false));
                        }
                        scope->list.push_back(spliced);
                        spliced = scope;
                    }
                    if ( spliced->rtti_isBlock() && !(subjResult && subjResult->ref) ) spliced->generated = true;
                    splice.push_back(spliced);
                }
                auto & list = site.anchor.block->list;
                list.insert(list.begin()+anchorIndex, splice.begin(), splice.end());
                indexShift[site.anchor.block] += int(splice.size());
                int stmtIndex = anchorIndex + int(splice.size());
                if ( list[stmtIndex]==callLike ) {
                    // the call IS the statement
                    if ( callReplacement ) {
                        list[stmtIndex] = callReplacement;
                    } else {
                        list.erase(list.begin()+stmtIndex);
                        indexShift[site.anchor.block] -= 1;
                    }
                } else {
                    ReplaceNode rn;
                    rn.what = callLike;
                    rn.with = callReplacement;
                    list[stmtIndex] = list[stmtIndex]->visit(rn);
                    if ( !rn.done ) {
                        error("internal error: inlined call not found in its statement", callLike->at);
                        continue;
                    }
                }
                completeSite(site, fn, subjName, scopeModule);
                inlineId ++;
            }
        }

        // DFS postorder over the splice graph inside this module, [inline] functions
        // first: a chain f -> g -> h splices already-inlined bodies in a single round
        void orderInlineFunctions ( Module * thisMod, Function * fn, const AutoInlineCfg & cfg,
                vector<Function *> & order, das_hash_set<Function *> & seen ) {
            if ( !fn->body || fn->isTemplate || fn->stub ) return;
            if ( fn->module != thisMod ) return;
            if ( !seen.insert(fn).second ) return;
            lookupExpressions(fn->body, [&](Expression * expr) {
                Function * callee = callLikeFunc(expr);
                if ( !callee ) return;
                bool eligible = callee->mustInline || (expr->rtti_isCall()
                    ? autoEligibleCall(static_cast<ExprCall *>(expr), cfg)
                    : autoEligibleFn(callee, cfg));
                if ( eligible ) {
                    orderInlineFunctions(thisMod, callee, cfg, order, seen);
                }
            });
            order.push_back(fn);
        }

        // ----- return canonicalization (pre-pass) -----

        // optimized builds canonicalize early-exit shapes before candidates are evaluated:
        //  (1) tail-else synthesis: `if (c) { ...; return }` followed by a tail moves the
        //      tail into a synthesized else arm (what CondFolding does for nested blocks
        //      at optimize time; here it also covers the function's top block);
        //  (2) `if (c) return a; else return b;` folds to `return c ? a : b`.
        // together they turn early-exit bodies into terminal-return shapes the splicer
        // stores in place with zero overhead (non-canonical shapes still splice, through
        // the generated flag - see ReturnStoreRewrite). new nodes are untyped: the pass
        // reports astChanged and the
        // restarted infer legalizes them, the same protocol as a splice. the optimize-time
        // CondFolding copy stays - it serves compiles this pre-pass never sees (auto
        // inlining off) and macro-generated post-infer shapes.
        class ReturnCanonicalization : public Visitor {
        public:
            bool changed = false;
        protected:
            virtual bool canVisitQuoteSubexpression ( ExprQuote * ) override { return false; }
            // extract `return ...` from an if arm: a naked return (an inner fold leaves one
            // behind in an elif chain), or a block holding exactly one return. blocks with
            // a finally section never qualify - the fold would drop it. CMRES makes stay
            // behind: the make-local protocol doesn't survive under an Op3 arm
            static ExprReturn * armReturn ( Expression * arm ) {
                ExprReturn * ret = nullptr;
                if ( arm->rtti_isReturn() ) {
                    ret = static_cast<ExprReturn *>(arm);
                } else if ( arm->rtti_isBlock() ) {
                    auto blk = static_cast<ExprBlock *>(arm);
                    if ( blk->list.size()==1 && blk->finalList.empty() && blk->list.back()->rtti_isReturn() ) {
                        ret = static_cast<ExprReturn *>(blk->list.back());
                    }
                }
                if ( ret && ret->subexpr && ret->subexpr->rtti_isMakeLocal() ) ret = nullptr;
                return ret;
            }
            // the restarted infer expects parser shapes, where an if arm is a block. an
            // inner fold leaves a naked return as the arm of an elif chain; when the outer
            // if does not itself fold away, re-block the arm
            static void reblockArm ( ExpressionPtr & arm ) {
                if ( arm && arm->rtti_isReturn() ) {
                    auto blk = new ExprBlock();
                    blk->at = arm->at;
                    blk->list.push_back(arm);
                    arm = blk;
                }
            }
            virtual ExpressionPtr visit ( ExprIfThenElse * expr ) override {
                if ( expr->if_false && !expr->isStatic && !expr->doNotFold ) {
                    ExprReturn * lr = armReturn(expr->if_true);
                    ExprReturn * rr = armReturn(expr->if_false);
                    if ( lr && rr && lr->moveSemantics==rr->moveSemantics ) {
                        // a same-walk inner fold leaves untyped subexprs; decline those -
                        // the arm folds on the next patch round, once infer has typed it.
                        // return coercion is per-arm but ternary arms must match EACH OTHER,
                        // and the merged arm loses per-arm coercions: `return derived?` vs
                        // `return base?` stays an if, and so does `return null` (the null
                        // literal to typed pointer coercion doesn't survive under an Op3 arm)
                        if ( lr->subexpr && rr->subexpr
                            && lr->subexpr->type && rr->subexpr->type
                            && !lr->subexpr->type->isRef() && !rr->subexpr->type->isRef()
                            && !lr->subexpr->type->isVoid()
                            && !lr->subexpr->type->isVoidPointer() && !rr->subexpr->type->isVoidPointer()
                            && lr->subexpr->type->isSameType(*rr->subexpr->type, RefMatters::no, ConstMatters::no, TemporaryMatters::yes) ) {
                            auto ternary = new ExprOp3(expr->at, "?", expr->cond, lr->subexpr, rr->subexpr);
                            auto ret = new ExprReturn(expr->at, ternary);
                            ret->moveSemantics = lr->moveSemantics;
                            changed = true;
                            return ret;
                        } else if ( !lr->subexpr && !rr->subexpr && inlinePure(expr->cond) ) {
                            // both arms are bare returns and evaluating the cond does nothing
                            changed = true;
                            return lr;
                        }
                    }
                }
                reblockArm(expr->if_true);
                reblockArm(expr->if_false);
                return Visitor::visit(expr);
            }
            virtual ExpressionPtr visit ( ExprBlock * block ) override {
                // a finally section references block locals BY NAME; nesting tail
                // declarations under a synthesized else would hide them from it when
                // infer re-resolves (the optimize-time copy of this transform survives
                // only because post-infer references are by pointer)
                if ( !block->finalList.empty() ) return Visitor::visit(block);
                // tail-else synthesis, reversed order so one walk handles a whole batch
                bool any = false;
                for ( int i = int(block->list.size()) - 2; i>=0; i-- ) {
                    auto expr = block->list[i];
                    if ( !expr->rtti_isIfThenElse() ) continue;
                    auto ite = static_cast<ExprIfThenElse *>(expr);
                    if ( ite->if_false || ite->isStatic || ite->doNotFold || !ite->if_true->rtti_isBlock() ) continue;
                    auto tb = static_cast<ExprBlock *>(ite->if_true);
                    if ( tb->list.empty() ) continue;
                    auto lastE = tb->list.back();
                    if ( lastE->rtti_isReturn() || lastE->rtti_isBreak() || lastE->rtti_isContinue() ) {
                        auto fb = new ExprBlock();
                        fb->at = block->list[i+1]->at;
                        fb->list.assign(block->list.begin()+i+1, block->list.end());
                        ite->if_false = fb;
                        block->list.resize(i+1);
                        any = true;
                    }
                }
                if ( any ) changed = true;
                return Visitor::visit(block);
            }
        };

        bool canonicalizeReturns ( Function * fn, const AutoInlineCfg & cfg ) {
            if ( !fn->body || !fn->body->rtti_isBlock() ) return false;
            if ( fn->generator || fn->isTemplate ) return false;
            if ( fn->neverInline ) return false;    // never a candidate - don't reshape
            // the rewrite costs a re-infer round on every module that has one, so only
            // functions that can plausibly become splice candidates qualify: [inline]
            // (every site splices, and canonical shapes splice flag-free), a block
            // parameter (block-literal-arg candidacy), or - heuristic tier on - a body
            // that pre-screens close to the worthiness budget (25% slack: folding can
            // shrink a body into budget) or a budget-exempt private single-call callee
            bool hasBlockParam = false;
            for ( auto & arg : fn->arguments ) {
                if ( arg->type && arg->type->baseType==Type::tBlock ) { hasBlockParam = true; break; }
            }
            if ( !hasBlockParam && !fn->mustInline ) {
                if ( !cfg.functions ) return false;
                bool exempt = cfg.budgetExempt
                    && cfg.budgetExempt->find(fn)!=cfg.budgetExempt->end();
                if ( !exempt ) {
                    BodyCost bc = bodyCost(fn->body);
                    if ( bc.hasLoop || bc.nodes > cfg.budget + cfg.budget/4 ) return false;
                }
            }
            // goto can target a label in a tail this pass would move into an else arm; the
            // feature is rare enough that any use opts the whole function out
            bool hasGotoOrLabel = false;
            lookupExpressions(fn->body, [&](Expression * e) {
                if ( e->rtti_isGoto() || e->rtti_isLabel() ) hasGotoOrLabel = true;
            });
            if ( hasGotoOrLabel ) return false;
            bool any = false;
            for ( int round = 0; round!=16; ++round ) {   // converges in 2-3; the cap is paranoia
                ReturnCanonicalization pass;
                fn->body = fn->body->visit(pass);
                if ( !pass.changed ) break;
                any = true;
            }
            return any;
        }

    } // anonymous namespace

    bool Program::patchInline() {
        bool mustEnabled = !options.getBoolOption("disable_inline", policies.disable_inline);
        // best-effort inlining is an optimization: it stays out of unoptimized builds
        bool autoEnabled = getOptimize()
            && !options.getBoolOption("disable_auto_inline", policies.disable_auto_inline);
        // the heuristic tier over plain calls is opt-in on top of the block-literal tier.
        // strict aliasing mode (`options no_aliasing`) reports call-result aliasing as
        // errors; splicing those calls away would change which programs compile, so the
        // heuristic tier stands down there - diagnostics must not depend on a perf knob
        bool autoFns = autoEnabled
            && options.getBoolOption("auto_inline_functions", policies.auto_inline_functions)
            && !options.getBoolOption("no_aliasing", policies.no_aliasing);
        das_hash_map<Function *, bool> worthCache;
        das_hash_set<Function *> budgetExempt;
        AutoInlineCfg autoCfg;
        autoCfg.blockLiterals = autoEnabled;
        autoCfg.functions = autoFns;
        autoCfg.budget = options.getIntOption("auto_inline_cost", policies.auto_inline_cost);
        autoCfg.thisModule = thisModule.get();
        autoCfg.worthCache = &worthCache;
        autoCfg.budgetExempt = &budgetExempt;
        // private functions referenced exactly once - by a plain call in a function
        // body - are budget-exempt: the splice moves the body rather than duplicating
        // it (removeUnusedSymbols reaps the husk). private is what makes the count
        // sound: every possible reference is inside this module, in front of us now.
        // any other reference kind (operator call, @@, an initializer) disqualifies
        if ( autoFns && !failed() ) {
            das_hash_map<Function *, int> refs;         // every reference, anywhere
            das_hash_map<Function *, int> callSites;    // plain-call sites in function bodies
            auto interesting = [&](Function * f) -> bool {
                // fromGeneric excluded: instances are STAMPED private by instantiation,
                // which would sweep library generics into the exemption - the heuristic
                // tier keeps instances out entirely (see autoEligibleCall)
                return f && f->privateFunction && !f->fromGeneric && f->module==thisModule.get()
                    && f->body && !f->builtIn && !f->isTemplate
                    && !f->addr && !f->addressTaken && !f->mustInline && !f->neverInline
                    && !f->exports && !f->init && !f->shutdown && !f->lateInit
                    && !f->macroFunction;
            };
            auto scanRefs = [&](Expression * root, bool inBody) {
                lookupExpressions(root, [&](Expression * e) {
                    Function * f = nullptr;
                    bool plainCall = false;
                    if ( e->rtti_isCall() ) { f = static_cast<ExprCall *>(e)->func; plainCall = true; }
                    else if ( e->rtti_isOp1() ) f = static_cast<ExprOp1 *>(e)->func;
                    else if ( e->rtti_isOp2() ) f = static_cast<ExprOp2 *>(e)->func;
                    else if ( e->rtti_isOp3() ) f = static_cast<ExprOp3 *>(e)->func;
                    else if ( e->rtti_isAddr() ) f = static_cast<ExprAddr *>(e)->func;
                    if ( !f || !interesting(f) ) return;
                    refs[f] ++;
                    if ( inBody && plainCall ) callSites[f] ++;
                });
            };
            thisModule->functions.foreach([&](auto fn) {
                if ( fn->body ) scanRefs(fn->body, true);
            });
            for ( auto & var : thisModule->globals.each() ) {
                if ( var->init ) scanRefs(var->init, false);
            }
            for ( auto & st : thisModule->structures.each() ) {
                for ( auto & fld : st->fields ) {
                    if ( fld.init ) scanRefs(fld.init, false);
                }
            }
            for ( auto & kv : refs ) {
                if ( kv.second==1 && callSites[kv.first]==1 ) budgetExempt.insert(kv.first);
            }
        }
        // canonicalize early-exit shapes ahead of candidacy: single exit is an inline shape
        // requirement, and the rewrite must settle (re-infer) before anything splices on
        // top of it. gated with auto inlining rather than bare optimization:
        // `disable_auto_inline` is the one knob that promises "no patch-slot reshaping"
        // to shape-pinning tests and macros
        if ( autoEnabled && !failed() ) {
            bool canon = false;
            thisModule->functions.foreach([&](auto fn) {
                canon |= canonicalizeReturns(fn, autoCfg);
            });
            if ( canon ) return true;
        }
        // cheap gate: any [inline] function visible at all?
        bool anyInline = false;
        if ( mustEnabled ) {
            library.foreach([&](Module * mod) -> bool {
                if ( !anyInline ) {
                    mod->functions.find_first([&](auto fn) {
                        if ( fn->mustInline ) { anyInline = true; return true; }
                        return false;
                    });
                }
                return true;
            }, "*");
        }
        if ( !anyInline && !autoEnabled ) return false;
        InlinePatch patch;
        patch.program = this;
        patch.logs = daScriptEnvironment::getBound()->g_compilerLog;
        patch.logOpt = options.getBoolOption("log_optimization", policies.log_optimization);
        patch.mustEnabled = mustEnabled && anyInline;
        patch.autoCfg = autoCfg;
        vector<Function *> order;
        das_hash_set<Function *> seen;
        thisModule->functions.foreach([&](auto fn) {
            if ( fn->mustInline ) orderInlineFunctions(thisModule.get(), fn, autoCfg, order, seen);
        });
        thisModule->functions.foreach([&](auto fn) {
            orderInlineFunctions(thisModule.get(), fn, autoCfg, order, seen);
        });
        for ( auto fn : order ) {
            patch.processFunction(fn);
            if ( failed() ) break;
        }
        if ( patch.logOpt && patch.logs
            && (patch.inlined || patch.inlinedAuto || patch.devirted || patch.declined) ) {
            *patch.logs << "INLINE: " << patch.inlined << " must + " << patch.inlinedAuto
                        << " auto + " << patch.devirted << " devirt site(s), "
                        << patch.declined << " declined in module " << thisModule->name << "\n";
        }
        return patch.changed;
    }

}
