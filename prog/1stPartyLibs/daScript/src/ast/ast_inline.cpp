#include "daScript/misc/platform.h"

#include "daScript/ast/ast.h"
#include "daScript/ast/ast_visitor.h"
#include "daScript/ast/ast_expressions.h"

namespace das {

    // ONE leading underscore, not two: GLSL ES reserves any identifier containing `__`
    // (WebGL enforces it), and inlined shader helpers must survive the GLSL backend
    static const char * INLINE_TEMP_PREFIX = "_inl";

    // joining a manufactured prefix (always '_'-terminated) straight onto a name that
    // itself starts with '_' would spell `__` at the seam - the same GLSL ES reservation
    // the prefix above avoids. Such names are routine here: nested inlining renames the
    // inner site's own _inl<M>_* temps, and a callee local or parameter may be
    // '_'-spelled by the user. Leading underscores re-encode as a 'u' run between the
    // namespace and the stripped name (`_inl1_l_` + `_inl0_arg_a` -> `_inl1_lu_inl0_arg_a`),
    // which stays injective: the plain arm's tail never starts with '_', so no 'u' run
    // followed by '_' can collide across arms, and equal runs imply equal tails
    static string joinInlineName ( const string & prefix, const string & name ) {
        DAS_ASSERT(!prefix.empty() && prefix.back()=='_');
        if ( name.empty() || name[0]!='_' ) return prefix + name;
        size_t k = 0;
        while ( k<name.size() && name[k]=='_' ) ++k;
        return prefix.substr(0,prefix.size()-1) + string(k,'u') + "_" + name.substr(k);
    }

    // ===== [inline] splicing, auto inlining, invoke-block inlining =====
    // runs in the patch slot (Program::patchAnnotations -> patchInline), after infer and
    // buildAccessFlags, before lint and optimize. splices are syntax-level: cloned callee
    // statements land in the caller, the pass reports astChanged, and the restarted infer
    // re-resolves every name and legalizes types. three site kinds, two contracts:
    //  1. MustCall - a call to an [inline] function. fail-closed: a shape or position the
    //     splicer can't host is a compile error (bad callee shapes report at the
    //     declaration through the annotation lint hook)
    //  2. AutoCall - best-effort, optimized builds only, declines silently (counted,
    //     logged under log_optimization): calls passing a block LITERAL, plus - behind
    //     `options auto_inline_functions` - plain calls and operator sites whose callee
    //     fits the auto-inline budget. [never_inline] opts out of every best-effort tier
    //  3. InvokeBlock - invoke of a block literal, direct or through a never-rebound
    //     let-bound holder: the block body inlines in place
    // the tiers converge over rounds: an auto splice moves the callee's invoke next to
    // the literal, the next round inlines the block. argument tiers (classifyArguments):
    //  A. leaf args substitute textually;  B. a pure arg read once outside loops
    //     substitutes;  C. everything else binds a `let _inl<N>_arg_*` temp at the anchor
    // a call in a conditionally-evaluated position lowers first (tryLowerCallPosition);
    // `??` and safe-navigation have no lowering and refuse
    // `options never_inline` takes the whole module out of the game: the pass skips its
    // bodies (instances of its generics included, wherever they land) and its functions
    // refuse as callees everywhere - a MustCall on them degrades to a plain call

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

        class CanInlineScan : public Visitor {
        public:
            CanInlineScan ( bool blockLiteral = false ) : forBlockLiteral(blockLiteral) {}
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
                // a plain block literal clones with the body; lambda/local-function
                // literals lowered to generated functions - cloning those post-infer is not safe
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
            // assume aliases are function-scoped by name and not renamed by the splice -
            // two spliced bodies declaring the same alias collide (30700)
            virtual void preVisit ( ExprAssume * expr ) override { Visitor::preVisit(expr); flag("body contains an assume expression"); }
        };


        // operator names dispatched through ExprOp1/2/3 - the node kinds the splicer plans.
        // punctuation dispatched elsewhere ([] via ExprAt, ??, properties) stays refused
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

        // where the function's code was written: the root generic for an instance
        // (getOriginPtr walks the fromGeneric chain), the function itself otherwise
        Function * originOf ( Function * fn ) {
            auto origin = fn->getOriginPtr();
            return origin ? origin : fn;
        }

        // `options never_inline` - the function's home module opted out (an instance
        // answers for its origin's module too): it never splices, in either direction
        bool moduleNeverInlines ( Function * fn ) {
            if ( fn->module && fn->module->neverInline ) return true;
            if ( fn->fromGeneric ) {
                auto om = originOf(fn)->module;
                if ( om && om->neverInline ) return true;
            }
            return false;
        }

        // ----- callee param read statistics (for tier B) -----

        struct ParamReadStats {
            das_hash_map<Variable *, int>   readCount;
            das_hash_set<Variable *>        readUnderLoop;
            das_hash_set<Variable *>        readAsRefArg;   // handed onward to a REF parameter - needs real storage, not a value
        };

        class ParamReadScan : public Visitor {
        public:
            ParamReadScan ( const vector<VariablePtr> & paramVars, ParamReadStats & into ) : stats(into) {
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
        // a spliced body re-resolves in the DESTINATION module; the scan sorts references
        // that stop resolving there into CrossVerdict's two severities

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
                // locked names re-resolve through the fromGeneric fallback - exempt (a
                // clone without fromGeneric is not). a mangled "_::" ([template] products,
                // class-method dispatch) has no fallback and binds the program module
                // before AND after the splice - it stops the splice like a user escape
                if ( ofs==4 && name.find('`', ofs)!=string::npos && fn && fn->fromGeneric ) return;
                // machinery-manufactured _:: that re-resolves identically from the
                // destination (public, non-generic, visible - e.g. coverage's
                // _::add_func_coverage) is exempt. generated _::finalize/_::clone resolves
                // against the CALLING module by design and gates like a user escape
                if ( generatedNode && fn && !fn->privateFunction && !fn->fromGeneric
                    && fn->module && dest && dest->isVisibleDirectly(fn->module) ) return;
                verdict.hard = name;
                verdict.hardWhy = "body dispatches '" + name + "' at the call site's module";
            }
            void checkFunc ( Function * fn, const string & callName, bool generatedNode ) {
                checkName(callName, generatedNode, fn);
                if ( !fn || !verdict.hard.empty() ) return;
                auto origin = originOf(fn);
                if ( fn->fromGeneric ) {
                    for ( auto & arg : fn->arguments ) {
                        if ( arg->type && (arg->type->explicitConst || arg->type->explicitRef) ) {
                            verdict.hard = origin->name;
                            verdict.hardWhy = "body calls explicit-flavored instance '" + origin->name + "'";
                            return;
                        }
                    }
                    // a foreign instance re-resolves only through the origin-generic
                    // fallback, which needs the origin module visible - hence the wrapper
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

        // every declaration name in the caller. infer resolves LOCALS BEFORE BLOCK
        // ARGUMENTS, so a spliced can_shadow argument loses to any same-named caller
        // declaration - reads inside the block would silently bind the caller's variable
        class DeclNameCollect : public Visitor {
        public:
            DeclNameCollect ( das_hash_set<string> & nn ) : names(nn) {}
        protected:
            das_hash_set<string> & names;
            virtual bool canVisitQuoteSubexpression ( ExprQuote * ) override { return false; }
            void add ( const VariablePtr & var ) {
                names.insert(var->name);
                if ( !var->aka.empty() ) names.insert(var->aka);
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

        // free variable names of an expression (the can_shadow capture-check input)
        class FreeNameCollect : public Visitor {
        public:
            FreeNameCollect ( das_hash_set<string> & nn ) : names(nn) {}
        protected:
            das_hash_set<string> & names;
            virtual bool canVisitQuoteSubexpression ( ExprQuote * ) override { return false; }
            virtual void preVisit ( ExprVar * expr ) override {
                Visitor::preVisit(expr);
                names.insert(expr->name);
            }
        };

        // the callee's local declarations, so reads can be renamed by name
        // (infer re-resolves every ExprVar by name; consistent renaming is all it takes)
        class LocalNameCollect : public Visitor {
        public:
            LocalNameCollect ( das_hash_map<string, string> & r, das_hash_set<string> & cs, const string & p )
                : rename(r), canShadowArgs(cs), prefix(p) {}
        protected:
            das_hash_map<string, string> & rename;
            // can_shadow names are semantic (an ECS resolves the component by the
            // argument name) - never renamed; colliding sites decline instead
            das_hash_set<string> & canShadowArgs;
            string prefix;
            virtual bool canVisitQuoteSubexpression ( ExprQuote * ) override { return false; }
            void renameVar ( const string & name, const string & aka ) {
                if ( rename.find(name)==rename.end() ) rename[name] = joinInlineName(prefix, name);
                if ( !aka.empty() && rename.find(aka)==rename.end() ) rename[aka] = joinInlineName(prefix, name);
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
                    canShadowArgs.insert(var->name);
                    if ( !var->aka.empty() ) canShadowArgs.insert(var->aka);
                    return;
                }
                renameVar(var->name, var->aka);
            }
        };

        // rewrites a cloned callee subtree for splicing: parameter reads become their
        // planned substitution or temp read, locals rename, and r2v flags clear - a
        // compiled callee's flags are post-optimize, the restarted infer re-derives them
        class InlineBodyRewriter : public Visitor {
        public:
            InlineBodyRewriter ( das_hash_map<Variable *, ArgSub> & ps, das_hash_map<string, string> & rn,
                const LineInfo & at, bool clearUnsafe )
                : paramSub(ps), rename(rn), tempAt(at), clearUserUnsafe(clearUnsafe) {}
        protected:
            das_hash_map<Variable *, ArgSub> & paramSub;    // keyed by ORIGINAL callee param
            das_hash_map<string, string> & rename;
            LineInfo tempAt;                // call site location for manufactured temp reads
            bool clearUserUnsafe = false;   // splicing a function callee (not an invoke-block literal)
            enum class DeclKind {
                Renamed,        // renamed within its scope
                KeepsName,      // a can_shadow argument keeps its semantic name - and shields it from outer renamed declarations
            };
            das_hash_map<string, vector<DeclKind>> activeDecl;  // scope gate for the name-based rename: per-name stack, innermost wins
            vector<vector<string>> declFrames { {} };
            void activateDecl ( const string & name, DeclKind kind ) {
                activeDecl[name].push_back(kind);
                declFrames.back().push_back(name);
            }
            void pushDeclFrame () { declFrames.emplace_back(); }
            void popDeclFrame () {
                for ( auto & n : declFrames.back() ) activeDecl[n].pop_back();
                declFrames.pop_back();
            }
            bool renameActive ( const string & name ) const {
                auto it = activeDecl.find(name);
                return it!=activeDecl.end() && !it->second.empty() && it->second.back()==DeclKind::Renamed;
            }
            virtual bool canVisitQuoteSubexpression ( ExprQuote * ) override { return false; }
            virtual void preVisitExpression ( Expression * expr ) override {
                Visitor::preVisitExpression(expr);
                // keep the CALLER module's unsafe accounting unchanged - the callee's module
                // already accounted, and callee-origin unsafe stays authorized via alwaysSafe
                if ( clearUserUnsafe ) expr->userSaidItsSafe = false;
            }
            virtual ExpressionPtr visit ( ExprVar * expr ) override {
                // a cloned ExprVar still points at the ORIGINAL callee variable - param matching is exact
                if ( expr->variable ) {
                    auto it = paramSub.find(expr->variable);
                    if ( it != paramSub.end() ) {
                        if ( it->second.substitute ) return it->second.substitute->clone();
                        return new ExprVar(tempAt, it->second.tempName);
                    }
                }
                auto rit = rename.find(expr->name);
                if ( rit != rename.end() && renameActive(expr->name) ) {
                    expr->name = rit->second;
                    expr->variable = nullptr;   // infer re-resolves by name
                }
                expr->r2v = false;
                return Visitor::visit(expr);
            }
            virtual ExpressionPtr visit ( ExprRef2Value * expr ) override {
                // a substituted VALUE under an r2v wrapper: drop the wrapper
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
                auto rit = rename.find(var->name);
                if ( rit != rename.end() ) { activateDecl(var->name, DeclKind::Renamed); var->name = rit->second; }
                if ( !var->aka.empty() ) {
                    auto ait = rename.find(var->aka);
                    if ( ait != rename.end() ) { activateDecl(var->aka, DeclKind::Renamed); var->aka = ait->second; }
                }
                // a compiled callee's returned local may alias the CMRES slot (allocateStack) -
                // stale in the caller's frame; present virgin state, the caller re-derives
                var->aliasCMRES = false;
            }
            virtual void preVisit ( ExprFor * expr ) override {
                Visitor::preVisit(expr);
                pushDeclFrame();
                for ( size_t i=0, is=expr->iterators.size(); i!=is; ++i ) {
                    auto rit = rename.find(expr->iterators[i]);
                    if ( rit != rename.end() ) { activateDecl(expr->iterators[i], DeclKind::Renamed); expr->iterators[i] = rit->second; }
                }
                for ( auto & var : expr->iteratorVariables ) {
                    auto rit = rename.find(var->name);
                    if ( rit != rename.end() ) var->name = rit->second;
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
                // the rename map is name-keyed and cannot tell a can_shadow argument from a
                // renamable local - the flag on the variable can
                if ( var->can_shadow ) {
                    activateDecl(var->name, DeclKind::KeepsName);
                    if ( !var->aka.empty() ) activateDecl(var->aka, DeclKind::KeepsName);
                    return;
                }
                auto rit = rename.find(var->name);
                if ( rit != rename.end() ) { activateDecl(var->name, DeclKind::Renamed); var->name = rit->second; }
                else { activateDecl(var->name, DeclKind::KeepsName); }
                if ( !var->aka.empty() ) {
                    auto ait = rename.find(var->aka);
                    if ( ait != rename.end() ) { activateDecl(var->aka, DeclKind::Renamed); var->aka = ait->second; }
                    else { activateDecl(var->aka, DeclKind::KeepsName); }
                }
            }
        };

        // attribution only: stack traces and at-keyed macros read the splice as the call
        // site; liveness gating uses frame positions (LocalVariableInfo::openPos). runs
        // BEFORE InlineBodyRewriter so substituted caller expressions keep their own at
        class SpliceAtStamp : public Visitor {
        public:
            SpliceAtStamp ( const LineInfo & at ) : callAt(at) {}
            LineInfo callAt;
            bool markGenerated = true;
        protected:
            virtual bool canVisitQuoteSubexpression ( ExprQuote * ) override { return false; }
            virtual void preVisitExpression ( Expression * expr ) override {
                Visitor::preVisitExpression(expr);
                expr->at = callAt;
                // a spliced clone is compiler-inserted: the original body is linted at its
                // definition, and at-keyed macros must not re-analyze the copy (the stamp
                // makes it look host-authored, defeating their foreign-file heuristics).
                // temp INITS are exempt - they are caller-authored argument/prefix
                // expressions merely relocated, and marking them would hide lint findings
                // on user code
                if ( markGenerated ) expr->generated = true;
            }
        };

        // replace one specific node (by identity) inside a statement
        class ReplaceNode : public Visitor {
        public:
            ReplaceNode ( Expression * w, const ExpressionPtr & r ) : what(w), with(r) {}
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

        // where a splice inserts its manufactured statements (arg/result temps, the body)
        struct StmtAnchor {
            ExprBlock * block = nullptr;    // the statement list hosting the site
            int         index = -1;         // the site's statement position in block->list at collect time
        };

        enum class SiteKind {
            MustCall,   // call to an [inline] function: fail-closed contract, errors
            AutoCall,   // call with a block-literal argument: best-effort, silent declines
            InvokeBlock // invoke of a block literal (direct or let-bound): best-effort, inlines the block
        };

        struct PlannedSite {
            Expression * callLike = nullptr;    // ExprCall or ExprInvoke
            Expression * stmt = nullptr;        // the anchored statement: anchor.block->list[anchor.index] at collect time
            StmtAnchor  anchor;
            SiteKind    kind = SiteKind::MustCall;
            string      withModule;             // innermost `with (module ...)` at the site
        };

        // annotations other than inert markers may carry call-site semantics (verifyCall,
        // transform, per-call codegen) that splicing the call away would bypass
        bool isAnnotated ( Function * fn, string & why ) {
            for ( auto & ann : fn->annotations ) {
                if ( !ann->annotation ) continue;
                auto & nm = ann->annotation->name;
                if ( nm=="export" || nm=="unused_argument" ) continue;
                why = "annotated function [" + nm + "]";
                return true;
            }
            return false;
        }

        // ----- auto-inline budget (heuristic plain-call candidacy) -----

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

        // spliced locals join the caller frame PERMANENTLY (slots are not reused across
        // sites), so unbounded splicing overflows documented `options stack` contracts
        constexpr int64_t AUTO_INLINE_CALLEE_STACK_BYTES  = 512;
        constexpr int64_t AUTO_INLINE_CALLER_FRAME_BYTES  = 4096;
        constexpr int64_t AUTO_INLINE_CALLER_GROWTH_BYTES = 2048;

        // best-effort candidacy config; caches live one patch round - splices change bodies
        struct AutoInlineCfg {
            bool blockLiterals = false;     // block-literal call sites + invoke-block sites
            bool functions = false;         // heuristic tier over plain calls (auto_inline_functions)
            int  budget = 32;               // heuristic node budget (auto_inline_cost)
            Module * thisModule = nullptr;  // heuristic tier is same-module-only
            das_hash_map<Function *, bool> * budgetCache = nullptr;
            das_hash_set<Function *> * budgetExempt = nullptr;  // private, referenced exactly once: moves, not duplicates
        };

        bool fitsAutoInlineBudget ( Function * fn, const AutoInlineCfg & cfg ) {
            if ( cfg.budgetExempt && cfg.budgetExempt->find(fn)!=cfg.budgetExempt->end() ) return true;
            if ( !cfg.budgetCache ) return false;
            auto it = cfg.budgetCache->find(fn);
            if ( it != cfg.budgetCache->end() ) return it->second;
            BodyCost bc = bodyCost(fn->body);
            bool ok = !bc.hasLoop && bc.nodes <= cfg.budget     // a loop's runtime dwarfs the call overhead
                && bc.stackBytes <= AUTO_INLINE_CALLEE_STACK_BYTES;
            (*cfg.budgetCache)[fn] = ok;
            return ok;
        }

        bool autoEligibleFn ( Function * fn, const AutoInlineCfg & cfg ) {
            if ( !fn || fn->mustInline || fn->builtIn ) return false;
            if ( !fn->body || fn->isTemplate ) return false;
            if ( fn->neverInline ) return false;
            if ( fn->generated ) return false;
            if ( !cfg.functions || fn->fromGeneric ) return false;
            if ( fn->module != cfg.thisModule ) return false;   // cross-module is [inline]'s contract, not a heuristic's call
            return fitsAutoInlineBudget(fn, cfg);
        }

        bool autoEligibleCall ( ExprCall * call, const AutoInlineCfg & cfg ) {
            if ( !call->func || call->func->mustInline || call->func->builtIn ) return false;
            if ( !call->func->body || call->func->isTemplate ) return false;
            if ( call->func->neverInline ) return false;    // explicit opt-out, both tiers
            if ( cfg.blockLiterals ) {
                for ( auto & a : call->arguments ) {
                    if ( a->rtti_isMakeBlock() ) return true;   // we look for block variables in BlockBindingScan
                }
            }
            return autoEligibleFn(call->func, cfg);
        }

        // the func behind a call-like splice edge: a direct call or an ExprOp1/2/3 site.
        // rtti_isOp2 is true for copy/move/clone too - their func never passes the
        // eligibility gates, so the over-approximation is harmless
        Function * callLikeFunc ( Expression * expr ) {
            if ( expr->rtti_isCall() ) return static_cast<ExprCall *>(expr)->func;
            if ( expr->rtti_isOp1() || expr->rtti_isOp2() || expr->rtti_isOp3() ) {
                return static_cast<ExprCallFunc *>(expr)->func;
            }
            return nullptr;
        }

        // collects splice sites, each tagged with its (block, index) statement anchor
        class InlineCollect : public Visitor {
        public:
            InlineCollect ( const AutoInlineCfg & cfg, TextWriter * log = nullptr ) : cfg(cfg), logs(log) {}
            vector<PlannedSite> sites;      // in visit order: within a block, increasing index
        protected:
            const AutoInlineCfg & cfg;
            TextWriter * logs = nullptr;    // log_optimization sink; a degraded [inline] contract must be visible
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
                if ( auto callee = callLikeFunc(expr) ) {
                    if ( moduleNeverInlines(callee) ) {         // stays a plain call
                        if ( logs && kind==SiteKind::MustCall ) {
                            *logs << "INLINE  declined " << callee->name << " - `options never_inline` module " << callee->module->name << "\n";
                        }
                        return;
                    }
                }
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
                } else if ( autoEligibleCall(expr, cfg) ) {
                    plan(expr, SiteKind::AutoCall);
                }
            }
            // operator-overload sites. dispatch is exact-typed, so these never fire
            // for ExprCopy/ExprMove/ExprClone (which derive from ExprOp2)
            void planOp ( ExprCallFunc * expr ) {
                if ( expr->func && expr->func->mustInline ) {
                    plan(expr, SiteKind::MustCall);
                } else if ( autoEligibleFn(expr->func, cfg) ) {
                    plan(expr, SiteKind::AutoCall);
                }
            }
            virtual void preVisit ( ExprOp1 * expr ) override { Visitor::preVisit(expr); planOp(expr); }
            virtual void preVisit ( ExprOp2 * expr ) override { Visitor::preVisit(expr); planOp(expr); }
            virtual void preVisit ( ExprOp3 * expr ) override { Visitor::preVisit(expr); planOp(expr); }
            virtual void preVisit ( ExprInvoke * expr ) override {
                Visitor::preVisit(expr);
                if ( !cfg.blockLiterals || expr->isInvokeMethod || expr->arguments.empty() ) return;
                Expression * a0 = expr->arguments[0];
                if ( a0->rtti_isR2V() ) a0 = static_cast<ExprRef2Value *>(a0)->subexpr;
                if ( a0->rtti_isMakeBlock() || a0->rtti_isVar() ) plan(expr, SiteKind::InvokeBlock);  // a var arg0 traces via BlockBindingScan, or skips
            }
        };

        // let-bound block literals: a variable move-initialized from a literal and never
        // REBOUND still holds it at every invoke (an invoke writes its block argument but
        // cannot rebind the holder, so invoke arg0 reads don't disqualify). each binding
        // remembers its module-with scope: a literal must keep resolving where it was written
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

        enum class EvalKind {
            Eager,          // evaluated exactly once, unconditionally
            Conditional,    // conditionally, with a lowering (&&, ||, ?:)
            Unlowerable,    // conditionally, without one (??, safe navigation)
        };

        // children of `e` in evaluation order. statement kinds list only their
        // same-statement expressions (an if statement lists its condition; nested
        // blocks anchor their own statements)
        void childrenInEvalOrder ( Expression * e, vector<pair<Expression *,EvalKind>> & out ) {
            out.clear();
            if ( e->rtti_isOp3() ) {
                // a user-overloaded op3 evaluates as a call - every operand eager;
                // only the builtin form selects its arms lazily
                auto op3 = static_cast<ExprOp3 *>(e);
                EvalKind condArm = (!op3->func || op3->func->builtIn) ? EvalKind::Conditional : EvalKind::Eager;
                out.emplace_back(op3->subexpr, EvalKind::Eager);
                out.emplace_back(op3->left, condArm);
                out.emplace_back(op3->right, condArm);
            } else if ( e->rtti_isOp2() ) {
                // covers copy/move/clone too: destination address first, then the value.
                // &&/|| (and their &&=/||= assignment forms) short-circuit only in
                // builtin form - a user overload is a call
                auto op2 = static_cast<ExprOp2 *>(e);
                bool condRight = (op2->op=="&&" || op2->op=="||"
                        || op2->op=="&&=" || op2->op=="||=")
                    && (!op2->func || op2->func->builtIn);
                out.emplace_back(op2->left, EvalKind::Eager);
                out.emplace_back(op2->right, condRight ? EvalKind::Conditional : EvalKind::Eager);
            } else if ( e->rtti_isNullCoalescing() ) {
                auto nc = static_cast<ExprNullCoalescing *>(e);
                out.emplace_back(nc->subexpr, EvalKind::Eager);
                out.emplace_back(nc->defaultValue, EvalKind::Unlowerable);
            } else if ( e->rtti_isSafeField() ) {
                out.emplace_back(static_cast<ExprSafeField *>(e)->value, EvalKind::Eager);
            } else if ( e->rtti_isSafeAt() ) {
                auto sa = static_cast<ExprSafeAt *>(e);
                out.emplace_back(sa->subexpr, EvalKind::Eager);
                out.emplace_back(sa->index, EvalKind::Unlowerable);
            } else if ( e->rtti_isOp1() ) {
                out.emplace_back(static_cast<ExprOp1 *>(e)->subexpr, EvalKind::Eager);
            } else if ( e->rtti_isR2V() ) {
                out.emplace_back(static_cast<ExprRef2Value *>(e)->subexpr, EvalKind::Eager);
            } else if ( e->rtti_isCall() ) {
                auto c = static_cast<ExprCall *>(e);
                for ( auto & a : c->arguments ) out.emplace_back(a, EvalKind::Eager);
            } else if ( e->rtti_isInvoke() ) {
                auto inv = static_cast<ExprInvoke *>(e);
                for ( auto & a : inv->arguments ) out.emplace_back(a, EvalKind::Eager);
            } else if ( e->rtti_isStringBuilder() ) {
                auto sb = static_cast<ExprStringBuilder *>(e);
                for ( auto & a : sb->elements ) out.emplace_back(a, EvalKind::Eager);
            } else if ( e->rtti_isReturn() ) {
                auto r = static_cast<ExprReturn *>(e);
                if ( r->subexpr ) out.emplace_back(r->subexpr, EvalKind::Eager);
            } else if ( e->rtti_isLet() ) {
                auto let = static_cast<ExprLet *>(e);
                for ( auto & v : let->variables ) {
                    if ( v->init ) out.emplace_back(v->init, EvalKind::Eager);
                }
            } else if ( e->rtti_isIfThenElse() ) {
                out.emplace_back(static_cast<ExprIfThenElse *>(e)->cond, EvalKind::Eager);
            } else if ( e->rtti_isFor() ) {
                auto f = static_cast<ExprFor *>(e);
                for ( auto & s : f->sources ) out.emplace_back(s, EvalKind::Eager);
            } else if ( e->rtti_isWith() ) {
                auto wi = static_cast<ExprWith *>(e);
                if ( wi->with ) out.emplace_back(wi->with, EvalKind::Eager);
            } else if ( e->rtti_isField() ) {
                out.emplace_back(static_cast<ExprField *>(e)->value, EvalKind::Eager);
            } else if ( e->rtti_isAt() ) {
                auto at = static_cast<ExprAt *>(e);
                out.emplace_back(at->subexpr, EvalKind::Eager);
                out.emplace_back(at->index, EvalKind::Eager);
            } else if ( e->rtti_isCast() ) {
                out.emplace_back(static_cast<ExprCast *>(e)->subexpr, EvalKind::Eager);
            } else if ( e->rtti_isUnsafe() ) {
                out.emplace_back(static_cast<ExprUnsafe *>(e)->body, EvalKind::Eager);
            }
        }

        // mirrors lint's 30250 detection: two non-deref ref lookups into the same table
        // (by textual identity) within one statement. lint runs AFTER the patch slot -
        // splicing such a statement would erase the diagnostic while keeping the UB
        void scanStatementTableAts ( Expression * e, das_hash_set<uint64_t> & seen, bool & collides ) {
            if ( !e || collides ) return;
            if ( e->rtti_isAt() ) {
                auto at = static_cast<ExprAt *>(e);
                if ( !at->underDeref && at->subexpr->type && at->subexpr->type->isGoodTableType() ) {
                    auto h = hash64z(at->subexpr->describe().c_str());
                    if ( !seen.insert(h).second ) { collides = true; return; }
                }
            }
            vector<pair<Expression *,EvalKind>> kids;
            childrenInEvalOrder(e, kids);
            for ( auto & k : kids ) scanStatementTableAts(k.first, seen, collides);
        }
        bool statementHasTableLookupCollision ( Expression * stmt ) {
            das_hash_set<uint64_t> seen;
            bool collides = false;
            scanStatementTableAts(stmt, seen, collides);
            return collides;
        }

        // an lvalue chain whose root is a global variable, walked through field/index/cast views
        bool isGlobalRooted ( Expression * e ) {
            for ( ;; ) {
                if ( !e ) return false;
                if ( e->rtti_isVar() ) {
                    auto ev = static_cast<ExprVar *>(e);
                    return (ev->variable || ev->type) && ev->isGlobalVariable();
                }
                if ( e->rtti_isField() ) e = static_cast<ExprField *>(e)->value;
                else if ( e->rtti_isSafeField() ) e = static_cast<ExprSafeField *>(e)->value;
                else if ( e->rtti_isAsVariant() ) e = static_cast<ExprAsVariant *>(e)->value;
                else if ( e->rtti_isSafeAsVariant() ) e = static_cast<ExprSafeAsVariant *>(e)->value;
                else if ( e->rtti_isSwizzle() ) e = static_cast<ExprSwizzle *>(e)->value;
                else if ( e->rtti_isAt() ) e = static_cast<ExprAt *>(e)->subexpr;
                else if ( e->rtti_isSafeAt() ) e = static_cast<ExprSafeAt *>(e)->subexpr;
                else if ( e->rtti_isCast() ) e = static_cast<ExprCast *>(e)->subexpr;
                else if ( e->rtti_isR2V() ) e = static_cast<ExprRef2Value *>(e)->subexpr;
                else return false;
            }
        }

        // may this expression's evaluation move (earlier, later, or become conditional)?
        // node-level noSideEffects flags are stale at patch time; function sideEffectFlags
        // are fresh (buildAccessFlags runs right before the patch slot) - derive from the tree
        bool isReorderSafe ( Expression * e ) {
            if ( !e ) return true;
            if ( e->rtti_isConstant() ) return true;
            if ( e->rtti_isMakeBlock() ) return true;   // making a block writes nothing (and
                                                        // hoisting one would copy a non-copyable)
            if ( e->rtti_isTypeDecl() ) return true;    // type<...> witness: a compile-time tag
            if ( e->rtti_isUnsafe() ) return isReorderSafe(static_cast<ExprUnsafe *>(e)->body);
            if ( e->rtti_isVar() ) return true;     // lvalue: storage identity is stable - only the R2V load below is time-sensitive
            if ( e->rtti_isR2V() ) {
                auto sub = static_cast<ExprRef2Value *>(e)->subexpr;
                if ( isGlobalRooted(sub) ) return false;    // the value can change mid-inline - snapshot it
                return isReorderSafe(sub);
            }
            if ( e->rtti_isField() ) return isReorderSafe(static_cast<ExprField *>(e)->value);
            if ( e->rtti_isSafeField() ) return isReorderSafe(static_cast<ExprSafeField *>(e)->value);
            if ( e->rtti_isAsVariant() ) return isReorderSafe(static_cast<ExprAsVariant *>(e)->value);
            if ( e->rtti_isSafeAsVariant() ) return isReorderSafe(static_cast<ExprSafeAsVariant *>(e)->value);
            if ( e->rtti_isSwizzle() ) return isReorderSafe(static_cast<ExprSwizzle *>(e)->value);
            if ( e->rtti_isCast() ) return isReorderSafe(static_cast<ExprCast *>(e)->subexpr);
            if ( e->rtti_isAt() ) {
                auto at = static_cast<ExprAt *>(e);
                return isReorderSafe(at->subexpr) && isReorderSafe(at->index);
            }
            if ( e->rtti_isSafeAt() ) {
                auto at = static_cast<ExprSafeAt *>(e);
                return isReorderSafe(at->subexpr) && isReorderSafe(at->index);
            }
            if ( e->rtti_isNullCoalescing() ) {
                auto nc = static_cast<ExprNullCoalescing *>(e);
                return isReorderSafe(nc->subexpr) && isReorderSafe(nc->defaultValue);
            }
            const uint32_t sideEffectsMask =
                  uint32_t(SideEffects::userScenario)
                | uint32_t(SideEffects::modifyExternal)
                | uint32_t(SideEffects::modifyArgument)
                | uint32_t(SideEffects::accessGlobal)
                | uint32_t(SideEffects::invoke);
            if ( e->rtti_isOp1() ) {
                auto op = static_cast<ExprOp1 *>(e);
                if ( op->func && (op->func->sideEffectFlags & sideEffectsMask) ) return false;
                return isReorderSafe(op->subexpr);
            }
            if ( e->rtti_isOp2() ) {
                auto op = static_cast<ExprOp2 *>(e);
                if ( op->func && (op->func->sideEffectFlags & sideEffectsMask) ) return false;
                return isReorderSafe(op->left) && isReorderSafe(op->right);
            }
            if ( e->rtti_isOp3() ) {
                auto op = static_cast<ExprOp3 *>(e);
                if ( op->func && (op->func->sideEffectFlags & sideEffectsMask) ) return false;
                return isReorderSafe(op->subexpr) && isReorderSafe(op->left) && isReorderSafe(op->right);
            }
            if ( e->rtti_isCall() ) {
                auto c = static_cast<ExprCall *>(e);
                if ( !c->func || (c->func->sideEffectFlags & sideEffectsMask) ) return false;
                for ( auto & a : c->arguments ) {
                    if ( !isReorderSafe(a) ) return false;
                }
                return true;
            }
            return false;   // unrecognized - conservatively unsafe to reorder
        }

        // a callee that writes nothing (no var by-ref params, no external state, no
        // globals - accessGlobal does not split reads from writes - no invokes) cannot
        // invalidate anything an argument expression read
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

        // for a callee that DOES write: a substituted argument may only read storage the
        // body provably cannot touch - plain by-value locals not passed by mutable ref in
        // this very call, address never taken. field/index/deref reads may reach aliased heap
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
                    auto & otherParam = sa.param(oi);
                    bool oRef = otherParam->type->ref || otherParam->type->isRefType();
                    if ( !oRef || otherParam->type->constant ) continue;
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
            vector<pair<Expression *,EvalKind>> kids;
            childrenInEvalOrder(root, kids);
            for ( auto & k : kids ) {
                if ( k.first && exprContains(k.first, what) ) return true;
            }
            return false;
        }

        enum class SitePosition { Eager, Conditional, NotFound, Unsupported };

        struct PathScan {
            vector<Expression *> conditionalOps;    // on the path, outermost LAST (pushed on unwind)
        };

        SitePosition findPath ( Expression * root, Expression * call, PathScan & scan ) {
            if ( root==call ) return SitePosition::Eager;
            vector<pair<Expression *,EvalKind>> kids;
            childrenInEvalOrder(root, kids);
            for ( auto & k : kids ) {
                if ( !k.first ) continue;
                if ( !exprContains(k.first, call) ) continue;
                auto sub = findPath(k.first, call, scan);
                if ( sub==SitePosition::NotFound || sub==SitePosition::Unsupported ) return sub;
                if ( k.second==EvalKind::Unlowerable ) return SitePosition::Unsupported;
                if ( k.second==EvalKind::Conditional ) {
                    scan.conditionalOps.push_back(root);
                    return SitePosition::Conditional;
                }
                return sub;
            }
            return SitePosition::NotFound;
        }

        // maximal side-effecting subexpressions evaluated strictly before `call`, in
        // evaluation order - hoisting them whole keeps the spliced temps from jumping
        // ahead of earlier effects (and a global read from crossing a later write)
        void collectSideEffectPrefix ( Expression * root, Expression * call, vector<Expression *> & prefix ) {
            if ( root==call ) return;
            vector<pair<Expression *,EvalKind>> kids;
            childrenInEvalOrder(root, kids);
            for ( auto & k : kids ) {
                if ( !k.first ) continue;
                if ( exprContains(k.first, call) ) {
                    collectSideEffectPrefix(k.first, call, prefix);
                    return;
                }
                if ( !isReorderSafe(k.first) ) prefix.push_back(k.first);
            }
        }

        // manufactured `let/var name [&] = init` (or an uninitialized declaration)
        ExprLet * makeTemp ( const LineInfo & at, const string & name, Expression * init,
                             bool isConst, bool isRef, bool viaMove ) {
            auto var = new Variable();
            var->name = name;
            var->at = at;
            var->generated = true;
            var->type = new TypeDecl(Type::autoinfer, at);
            var->type->constant = isConst;
            var->type->removeConstant = !isConst;
            var->type->ref = isRef;
            var->init = init;
            // a move from a CONST source is 30940 - it clone-initializes instead
            bool viaClone = viaMove && init && init->type && init->type->constant;
            var->init_via_move = viaMove && !viaClone;
            var->init_via_clone = viaClone;
            auto let = new ExprLet();
            let->at = at;
            // visibility stays closed until the init ran
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
            if ( !var->type->at.fileInfo ) var->type->at = at;
            var->type->constant = false;
            var->type->ref = false;
            var->type->safeWhenUninitialized = true;    // assigned before any read by construction
            auto let = new ExprLet();
            let->at = at;
            let->variables.push_back(var);
            return let;
        }

        // a copy with top-level ref/const erased - the two flavors the splice machinery adapts
        gc_local<TypeDecl> stripConstRef ( const TypeDecl * t ) {
            auto c = new TypeDecl(*t);
            c->ref = false;
            c->constant = false;
            return gc_local<TypeDecl>(c);
        }

        // an ExprVar on a refType parameter leaves type->ref unset (implicit ref via
        // isRefType), and casts copy that flag - so a cast/reinterpret chain rooted in
        // a parameter reads flag-false yet names caller storage
        // a cast reinterprets its subexpression's storage without creating any of its own,
        // so what the argument IS is always the thing under the casts
        Expression * peelCasts ( Expression * e ) {
            while ( e && e->rtti_isCast() ) e = static_cast<ExprCast *>(e)->subexpr;
            return e;
        }

        bool rootsInVar ( Expression * e ) {
            auto root = peelCasts(e);
            return root && root->rtti_isVar();
        }

        // shapes that always produce a fresh value, never a view of caller storage. a
        // ref-returning call types ref=true and binds a reference before reaching here; the
        // ternary derives from ExprCallFunc yet SELECTS an arm, so it yields a view
        bool isProvenRvalue ( Expression * e ) {
            e = peelCasts(e);
            if ( !e || e->rtti_isOp3() ) return false;
            return e->rtti_isCallFunc() || e->rtti_isInvoke()
                || e->rtti_isMakeStruct() || e->rtti_isMakeArray() || e->rtti_isMakeTuple()
                || e->rtti_isMakeVariant();
        }

        // a by-value temp is a local, and infer rejects a local of a type that cannot be
        // one (31020/30199) - refusing keeps the call a call, instead of erroring on a generated name
        bool tempTypeIsLocal ( Expression * init, bool ref, bool callerIsGenerator ) {
            if ( ref || !init->type ) return true;
            // block locals are legal (initialized with a make-block) - except in a generator,
            // whose top-level locals lift into the capture structure where a block would dangle
            if ( init->type->isGoodBlockType() ) return !callerIsGenerator;
            if ( !init->type->isLocal() ) return false;
            return init->type->canCopy() || init->type->canMove()
                || !init->type->hasNonTrivialCtor();
        }

        // ----- multi-return rewrite on the spliced clone -----
        // every function-level return in the CLONE becomes a result store (the standalone
        // callee keeps its shape). terminal returns (control after them is already
        // function exit) store in place, zero overhead:
        //      if (c) { return A } else { return B }   =>   if (c) { _res = A } else { _res = B }
        // any other shape stores through a generated `_inl<N>_ret : bool`:
        //      while (w) { if (d) { return B } }; return C
        //  =>  while (w) { if (d) { _res = B; _ret = true; break } }
        //      if (!_ret) { _res = C }
        // break unwinds block finallys and closes iterators exactly like return did, and
        // the store runs before the unwind. NO goto: a label would disqualify the whole
        // caller from CSE and DSE

        enum class RetExit { None, Maybe, Always };

        struct ReturnStoreRewrite {
            ReturnStoreRewrite ( const string & res, const string & flag ) : resName(res), flagName(flag) {}
            string resName;     // empty = void subject
            string flagName;
            bool flagUsed = false;
            void apply ( ExprBlock * body ) {
                if ( blockTerminal(body, true, false) ) {
                    rewriteTerminalBlock(body);
                } else {
                    flagUsed = true;
                    transformBlock(body, false);
                }
            }
            static bool hasEarlyReturnWithFinally ( ExprBlock * body ) {
                if ( blockTerminal(body, true, false) ) return false;
                return blockHasEarlyReturnWithFinally(body);
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
                    for ( auto & stmt : blk->list ) {
                        if ( stmtHasReturn(stmt) ) return true;
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
            static bool stmtHasEarlyReturnWithFinally ( Expression * s ) {
                if ( s->rtti_isIfThenElse() ) {
                    auto ite = static_cast<ExprIfThenElse *>(s);
                    if ( stmtHasEarlyReturnWithFinally(ite->if_true) ) return true;
                    return ite->if_false && stmtHasEarlyReturnWithFinally(ite->if_false);
                }
                if ( s->rtti_isBlock() ) return blockHasEarlyReturnWithFinally(static_cast<ExprBlock *>(s));
                if ( s->rtti_isFor() ) {
                    auto f = static_cast<ExprFor *>(s);
                    return f->body && stmtHasEarlyReturnWithFinally(f->body);
                }
                if ( s->rtti_isWhile() ) {
                    auto w = static_cast<ExprWhile *>(s);
                    return w->body && stmtHasEarlyReturnWithFinally(w->body);
                }
                if ( s->rtti_isUnsafe() ) {
                    auto u = static_cast<ExprUnsafe *>(s);
                    return u->body && stmtHasEarlyReturnWithFinally(u->body);
                }
                if ( s->rtti_isWith() ) {
                    auto w = static_cast<ExprWith *>(s);
                    return w->body && stmtHasEarlyReturnWithFinally(w->body);
                }
                return false;
            }
            static bool blockHasEarlyReturnWithFinally ( ExprBlock * blk ) {
                auto & list = blk->list;
                for ( size_t i=0, n=list.size(); i!=n; ++i ) {
                    // a return mid-list of a finally-carrying block: the flag tier would wrap the
                    // tail, hiding declarations the finally resolves by name on re-infer
                    if ( !blk->finalList.empty() && i+1<n && stmtHasReturn(list[i]) ) return true;
                    if ( stmtHasEarlyReturnWithFinally(list[i]) ) return true;
                }
                return false;
            }
            // return x   =>   _res = x        return <- x   =>   _res <- x
            // a PROVEN copyable rvalue demotes the move to a copy (ExprMove rejects
            // rvalues, 30941); an unproven one keeps it - a wrong demote is 30950
            Expression * makeStore ( ExprReturn * ret ) const {
                if ( resName.empty() ) return ret->subexpr; // void: keep the side effects, if any
                if ( !ret->subexpr ) return nullptr;        // defensive - infer rejects value-less returns
                bool storeAsCopy = ret->moveSemantics
                    && ret->subexpr->type
                    && !ret->subexpr->type->ref
                    && ret->subexpr->type->canCopy();
                if ( ret->moveSemantics && !storeAsCopy ) {
                    return new ExprMove(ret->at, new ExprVar(ret->at, resName), ret->subexpr, true);
                }
                return new ExprCopy(ret->at, new ExprVar(ret->at, resName), ret->subexpr, true);
            }
        protected:
            // ----- terminality prescan (read-only) -----
            static bool stmtTerminal ( Expression * stmt, bool tail, bool underLoop ) {  // every return under stmt already falls out of the function
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
                return true;    // no other statement kind hosts a function-level return (block literals keep theirs)
            }
            static bool armTerminal ( Expression * arm, bool tail, bool underLoop ) {    // same, for an if arm (may be a naked return)
                if ( arm->rtti_isReturn() ) return tail && !underLoop;
                return stmtTerminal(arm, tail, underLoop);
            }
            static bool blockTerminal ( ExprBlock * blk, bool tail, bool underLoop ) {  // same, for every statement of a block
                for ( size_t i=0, n=blk->list.size(); i!=n; ++i ) {
                    if ( !stmtTerminal(blk->list[i], tail && (i==n-1), underLoop) ) return false;
                }
                return true;
            }
            // ----- terminal tier: in-place stores -----
            // return X   =>   _res = X   in place (a bare void return just disappears)
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
            // a naked canonicalized arm: return X   =>   { _res = X }
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
            void rewriteTerminalStmt ( ExpressionPtr & slot ) {   // same, descending the statement shells
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
                return new ExprCopy(at, new ExprVar(at, flagName), new ExprConstBool(at, true), true);
            }
            // in a loop:  S_ret; REST   =>   S_ret'; if (_ret) { break }; REST
            // elsewhere:  S_ret; TAIL   =>   S_ret'; if (!_ret) { TAIL }
            RetExit transformBlock ( ExprBlock * blk, bool inLoop ) {
                auto & list = blk->list;
                RetExit overall = RetExit::None;
                for ( size_t i=0; i<list.size(); ++i ) {
                    RetExit exitKind = transformStmt(list[i], inLoop);
                    if ( exitKind==RetExit::Always ) {
                        list.resize(i+1);   // anything after is dead
                        return RetExit::Always;
                    }
                    if ( exitKind!=RetExit::Maybe ) continue;
                    overall = RetExit::Maybe;
                    if ( i+1 >= list.size() ) break;
                    if ( inLoop ) {
                        // control past a flagged inner exit leaves this loop too; for a
                        // non-loop Maybe the returning paths already broke - the guard is
                        // dead there and folds away
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
                    RetExit tailExit = transformBlock(tailBlk, false);
                    list.push_back(new ExprIfThenElse(tailBlk->at,
                        new ExprOp1(tailBlk->at, "!", flagRead(tailBlk->at)), tailBlk, nullptr));
                    // flagged paths returned, and an always-returning tail covers the rest
                    return tailExit==RetExit::Always ? RetExit::Always : RetExit::Maybe;
                }
                return overall;
            }
            RetExit transformArm ( ExpressionPtr & arm, bool inLoop ) {   // same, re-blocking a naked arm first
                if ( arm->rtti_isReturn() ) {   // naked canonicalized arm - re-block and retry
                    auto blk = new ExprBlock();
                    blk->at = arm->at;
                    blk->list.push_back(arm);
                    arm = blk;
                }
                return transformStmt(arm, inLoop);
            }
            // return X   =>   { _res = X; _ret = true; (break when in a loop) }
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

    bool canFunctionInline ( Function * fn, string & err ) {
        if ( fn->isTemplate ) return true;      // instances are checked instead
        if ( fn->builtIn || !fn->body ) { err = "[inline] requires a function with a das body"; return false; }
        if ( !fn->body->rtti_isBlock() ) { err = "[inline] requires a block body"; return false; }
        if ( fn->generator ) { err = "[inline] does not support generators"; return false; }
        if ( fn->lambda ) { err = "[inline] does not support lambdas"; return false; }
        if ( fn->isClassMethod || fn->classParent ) { err = "[inline] does not support class methods"; return false; }
        if ( fn->isCustomProperty || fn->propertyFunction ) { err = "[inline] does not support property functions"; return false; }
        // ExprOp-dispatched operator overloads splice; punctuation functions whose
        // call sites are other node kinds ([], ??, properties) can't honor the contract
        const string & plainName = originOf(fn)->name;
        if ( !isPlainIdentifier(plainName) && !exprOpDispatchedName(plainName) ) {
            err = "[inline] does not support operator '" + plainName + "' - its call sites do not splice";
            return false;
        }
        if ( fn->result && !fn->result->isVoid() ) {
            if ( fn->result->ref ) { err = "[inline] result must be by-value (or void)"; return false; }
            // the splice dissolves the call boundary a `#` result's lifetime is fenced to (30915)
            if ( fn->result->temporary ) { err = "[inline] does not support a temporary (#) result"; return false; }
            // the result temp is manufactured uninitialized (zeroed) - a required C++ ctor would never run
            if ( fn->result->isRefType() && fn->result->hasNonTrivialCtor() ) { err = "[inline] result type requires nontrivial construction"; return false; }
        }
        for ( auto & arg : fn->arguments ) {
            if ( !arg->type ) continue;
            if ( arg->type->temporary ) { err = "[inline] does not support temporary (#) parameter '" + arg->name + "'"; return false; }
            if ( arg->type->implicit ) { err = "[inline] does not support implicit parameter '" + arg->name + "'"; return false; }
            // the arg temp would need inscope/move discipline the splicer does not manufacture
            if ( arg->type->smartPtr ) { err = "[inline] does not support smart-pointer parameter '" + arg->name + "'"; return false; }
        }
        if ( fn->result && fn->result->smartPtr ) { err = "[inline] does not support a smart-pointer result"; return false; }
        auto body = static_cast<ExprBlock *>(fn->body);
        // block-level finally inside the body splices fine; a function-level one would run against the spliced result stores
        if ( !body->finalList.empty() ) { err = "[inline] does not support a function-level finally"; return false; }
        CanInlineScan scan;
        fn->body->visit(scan);
        if ( scan.bad ) { err = "[inline] " + scan.reason; return false; }
        if ( ReturnStoreRewrite::hasEarlyReturnWithFinally(body) ) { err = "[inline] early returns conflict with a finally section"; return false; }
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

    bool isInlineRecursionFree ( Function * fn, string & err ) {
        if ( fn->isTemplate || !fn->body ) return true;
        das_hash_set<Function *> visited;
        if ( inlineGraphReaches(fn, fn, visited) ) { err = "[inline] function is recursive through the inline graph"; return false; }
        return true;
    }

    // ----- best-effort shape gates (auto inlining and block inlining decline, never error) -----

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

        // a `#` argument into a non-`#`, non-implicit parameter: legal on the original
        // node, but spliced, the temp's lifetime is no longer fenced by the call boundary
        bool hasTempArgumentMismatch ( Expression * root ) {
            bool mismatch = false;
            lookupExpressions(root, [&](Expression * e) {
                if ( mismatch ) return;
                if ( e->rtti_isCall() ) {
                    auto c = static_cast<ExprCall *>(e);
                    if ( !c->func ) return;
                    size_t n = das::min(c->arguments.size(), c->func->arguments.size());
                    for ( size_t i=0; i!=n; ++i ) {
                        auto & at = c->arguments[i]->type;
                        auto & pt = c->func->arguments[i]->type;
                        if ( at && pt && at->temporary && !pt->temporary && !pt->implicit ) {
                            mismatch = true;
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
                            mismatch = true;
                            return;
                        }
                    }
                }
            });
            return mismatch;
        }

        // a block literal that can inline in place: a plain in-frame block
        // with a single-exit body and a non-reference (or void) result. splicing dissolves
        // the block boundary, so a break/continue not bound to a loop inside the body is out
        bool canBlockInline ( ExprMakeBlock * mkb, string & why ) {
            if ( mkb->isLambda || mkb->isLocalFunction || !mkb->capture.empty() ) { why = "lambda literal"; return false; }
            if ( !mkb->block || !mkb->block->rtti_isBlock() ) { why = "no block body"; return false; }
            auto blk = static_cast<ExprBlock *>(mkb->block);
            for ( auto & ann : blk->annotations ) {
                if ( ann->annotation && ann->annotation->name=="never_inline" ) { why = "block is [never_inline]"; return false; }
            }
            bool isVoid = !blk->returnType || blk->returnType->isVoid();
            if ( !isVoid && blk->returnType->ref ) { why = "result is not by-value"; return false; }
            if ( !isVoid && blk->returnType->temporary ) { why = "temporary result"; return false; }
            if ( !isVoid && blk->returnType->isRefType() && blk->returnType->hasNonTrivialCtor() ) { why = "result requires nontrivial construction"; return false; }
            for ( auto & arg : blk->arguments ) {
                if ( !arg->type ) { why = "unresolved parameter"; return false; }
                if ( arg->type->temporary ) { why = "temporary (#) parameter"; return false; }
                if ( arg->type->implicit ) { why = "implicit parameter"; return false; }
                if ( arg->type->smartPtr ) { why = "smart-pointer parameter"; return false; }
            }
            if ( blk->returnType && blk->returnType->smartPtr ) { why = "smart-pointer result"; return false; }
            if ( hasTempArgumentMismatch(mkb) ) { why = "body carries a temporary-flavored call"; return false; }
            CanInlineScan scan(true);
            blk->visit(scan);
            if ( scan.bad ) { why = scan.reason; return false; }
            if ( ReturnStoreRewrite::hasEarlyReturnWithFinally(blk) ) { why = "early returns conflict with a finally section"; return false; }
            return true;
        }

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

        // over the combined graph: [inline] calls AND auto-eligible calls/operator sites -
        // splicing a cycle would re-manufacture eligible sites every round
        bool isRecursiveThroughInlineGraph ( Function * fn, const AutoInlineCfg & cfg ) {
            das_hash_set<Function *> visited;
            return spliceGraphReaches(fn, fn, cfg, visited);
        }

    } // anonymous namespace

    // ----- the patch pass -----

    namespace {

        struct InlinePatch {
            InlinePatch ( Program * prog, TextWriter * log, bool logOptimization, bool must, const AutoInlineCfg & cfg )
                : program(prog), logs(log), logOpt(logOptimization), mustEnabled(must), moduleAutoCfg(cfg) {}
            Program * program = nullptr;
            TextWriter * logs = nullptr;
            bool logOpt = false;
            bool mustEnabled = true;
            AutoInlineCfg moduleAutoCfg;
            bool changed = false;
            int inlined = 0;        // [inline] sites
            int inlinedAuto = 0;    // auto block-literal call sites
            int invokeBlocks = 0;   // invoke-of-literal sites
            int declined = 0;       // best-effort sites the pass passed on
            das_hash_map<ExprBlock *, ParamReadStats> statsCache;
            das_hash_map<Function *, bool> shapeCache;
            das_hash_map<Function *, pair<bool,string>> autoOkCache;
            das_hash_map<ExprMakeBlock *, pair<bool,string>> invokeBlockShapeCache;
            das_hash_map<Function *, CrossVerdict> privateUseCache;
            das_hash_map<Function *, bool> cycleCache;

            // is `fn` reachable from its own body over the RUNTIME call graph (every call,
            // not just splice-eligible)? capped - beyond it assume acyclic. cached verdicts
            // stay valid across rounds (a stale `true` merely declines conservatively)
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
                bool ok = canFunctionInline(fn, err) && isInlineRecursionFree(fn, err);
                shapeCache[fn] = ok;
                return ok;
            }

            bool autoCalleeOk ( Function * fn, string & why ) {
                auto it = autoOkCache.find(fn);
                if ( it != autoOkCache.end() ) { why = it->second.second; return it->second.first; }
                bool ok = true;
                if ( !canFunctionInline(fn, why) ) {
                    ok = false;
                } else if ( fn->unsafeOperation || fn->unsafeDeref ) {  // changes call-site semantics; plain hasUnsafe is fine (generated wrapper)
                    ok = false;
                    why = "unsafe operation";
                } else if ( hasTempArgumentMismatch(fn->body) ) {
                    ok = false;
                    why = "body carries a temporary-flavored call";
                } else if ( bindsLocalRef(fn->body) ) {     // a chained splice can re-bind it to a non-addressable initializer (31019)
                    ok = false;
                    why = "body binds a local reference";
                } else if ( isAnnotated(fn, why) ) {
                    ok = false;
                } else {
                    string genericAlias;    // type<TT> naming the generic's own alias
                    lookupExpressions(fn->body, [&](Expression * expr) {
                        if ( !genericAlias.empty() || !expr->rtti_isTypeDecl() ) return;
                        auto td = static_cast<ExprTypeDecl *>(expr);
                        if ( td->typeexpr && td->typeexpr->isAutoOrAlias() ) genericAlias = td->typeexpr->describe();
                    });
                    if ( !genericAlias.empty() ) {
                        ok = false;
                        why = "body carries a generic alias type<" + genericAlias + ">";
                    } else if ( isRecursiveThroughInlineGraph(fn, moduleAutoCfg) ) {
                        ok = false;
                        why = "recursive through the inline graph";
                    }
                }
                autoOkCache[fn] = pair<bool,string>(ok, why);
                return ok;
            }

            bool invokeBlockShapeOk ( ExprMakeBlock * mkb, string & why ) {
                auto it = invokeBlockShapeCache.find(mkb);
                if ( it != invokeBlockShapeCache.end() ) { why = it->second.second; return it->second.first; }
                bool ok = canBlockInline(mkb, why);
                invokeBlockShapeCache[mkb] = pair<bool,string>(ok, why);
                return ok;
            }

            const ParamReadStats & paramStats ( ExprBlock * body, const vector<VariablePtr> & params ) {
                auto it = statsCache.find(body);
                if ( it != statsCache.end() ) return it->second;
                auto & stats = statsCache[body];
                ParamReadScan scan(params, stats);
                body->visit(scan);
                return stats;
            }

            const CrossVerdict & privateUse ( Function * fn, Module * originModule ) {
                auto it = privateUseCache.find(fn);
                if ( it != privateUseCache.end() ) return it->second;
                PrivateUseScan scan(originModule, program->thisModule.get(),
                    fn->module==program->thisModule.get());
                fn->body->visit(scan);
                return privateUseCache[fn] = scan.verdict;
            }

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
                    *logs << (site.kind==SiteKind::InvokeBlock ? "INVOKE-BLOCK DECLINED at " : "AUTO-INLINE DECLINED at ")
                          << at.describe() << ": " << why << "\n";
                }
            }

            void recordCompletedSite ( const PlannedSite & site, Function * caller, const string & subjName,
                    const string & scopeModule = string() ) {
                switch ( site.kind ) {
                    case SiteKind::MustCall: inlined ++; break;
                    case SiteKind::AutoCall: inlinedAuto ++; break;
                    case SiteKind::InvokeBlock: invokeBlocks ++; break;
                }
                if ( logOpt && logs ) {
                    const char * verb = site.kind==SiteKind::MustCall ? "INLINE "
                        : site.kind==SiteKind::AutoCall ? "AUTO-INLINE " : "INVOKE-BLOCK ";
                    *logs << verb << subjName << " into " << caller->getMangledName();
                    if ( !scopeModule.empty() ) *logs << " under with (module " << scopeModule << ")";
                    *logs << " at " << site.callLike->at.describe() << "\n";
                }
                changed = true;
            }

            // per-caller working state shared by every site splice in one round
            struct CallerSpliceState {
                das_hash_map<Variable *, BlockBinding> bindings;    // let-bound block literals invokes may resolve to
                das_hash_set<string> callerDeclNames;               // caller decl names (can_shadow capture check)
                int64_t grownBytes = 0;                             // frame bytes the heuristic tier charged this round
                das_hash_map<ExprBlock *, int> indexShift;          // statement index drift per block after splices
            };

            // what a site splices: a callee function, or an invoke's block literal
            struct SpliceSubject {
                Function *      fn = nullptr;       // MustCall / AutoCall
                ExprMakeBlock * literal = nullptr;  // InvokeBlock
                string          name;
            };

            // the gated inputs one splice consumes: the body to clone, its result type,
            // the argument/parameter view, and the module-scope decision
            struct SpliceInputs {
                ExprBlock * body = nullptr;
                TypeDecl *  result = nullptr;   // null = void
                SiteArgs    sa;
                vector<ExpressionPtr> opArgs;   // operator sites: operands as an argument vector (sa.args points here)
                bool        needScope = false;  // wrap the spliced body in with (module <origin>)
                string      scopeModule;
            };

            AutoInlineCfg callerAutoCfg ( Function * caller );
            bool resolveSubject ( const PlannedSite & site, CallerSpliceState & state, SpliceSubject & subj );
            int liveAnchorIndex ( const PlannedSite & site, CallerSpliceState & state );
            bool canInlineSubjectAtSite ( const PlannedSite & site, const SpliceSubject & subj, SpliceInputs & in );
            // how every argument reaches the spliced body

            struct ArgPlan {
                das_hash_map<Variable *, ArgSub> paramSub;  // keyed by the ORIGINAL callee param
                vector<ExpressionPtr> temps;                // `let _inl<N>_arg_*`, in call-argument order
            };

            bool tryBindArgTemp ( const PlannedSite & site, const string & subjName, const VariablePtr & param,
                bool callerIsGenerator, Expression * init, bool cnst, bool ref, bool viaMove,
                vector<ExpressionPtr> & temps, ArgSub & sub );
            void graftExpressionBody ( Function * caller, const PlannedSite & site, const SpliceSubject & subj,
                const SpliceInputs & in, ArgPlan & plan, CallerSpliceState & state, int anchorIndex );
            bool tryLowerCallPosition ( const PlannedSite & site, const SpliceSubject & subj,
                CallerSpliceState & state, int anchorIndex );
            void spliceStatements ( Function * caller, const PlannedSite & site, const SpliceSubject & subj,
                const SpliceInputs & in, ArgPlan & plan, CallerSpliceState & state, int anchorIndex, bool exprBody );
            bool classifyArguments ( Function * caller, const PlannedSite & site, const SpliceSubject & subj, const SpliceInputs & in, ArgPlan & plan );
            void processSite ( Function * caller, const PlannedSite & site, CallerSpliceState & state );
            void processFunction ( Function * caller );
        };

        AutoInlineCfg InlinePatch::callerAutoCfg ( Function * caller ) {
            AutoInlineCfg callerCfg = moduleAutoCfg;
            if ( !callerCfg.functions ) return callerCfg;
            string why;
            if ( isAnnotated(caller, why) ) {           // like [template] or [some_fancy_macro]
                callerCfg.functions = false;
            } else if ( caller->fromGeneric && originOf(caller)->module
                && originOf(caller)->module != program->thisModule.get() ) {
                callerCfg.functions = false;
            } else if ( bodyCost(caller->body).stackBytes > AUTO_INLINE_CALLER_FRAME_BYTES ) {
                callerCfg.functions = false;
            } else if ( callerOnCallCycle(caller) ) {       // recursive or recursive-ish
                callerCfg.functions = false;
            }
            return callerCfg;
        }

        void InlinePatch::processFunction ( Function * caller ) {
            if ( moduleNeverInlines(caller) ) return;   // the instance body is that module's code: leave it alone
            AutoInlineCfg callerCfg = callerAutoCfg(caller);
            InlineCollect collect(callerCfg, logOpt ? logs : nullptr);
            caller->body->visit(collect);
            if ( collect.sites.empty() ) return;
            CallerSpliceState state;
            if ( moduleAutoCfg.blockLiterals ) {
                BlockBindingScan bscan;
                caller->body->visit(bscan);
                for ( auto & kv : bscan.binding ) {
                    if ( bscan.disq.find(kv.first)==bscan.disq.end() ) state.bindings[kv.first] = kv.second;
                }
            }
            {
                DeclNameCollect dnc(state.callerDeclNames);
                caller->body->visit(dnc);
                for ( auto & arg : caller->arguments ) {
                    state.callerDeclNames.insert(arg->name);
                    if ( !arg->aka.empty() ) state.callerDeclNames.insert(arg->aka);
                }
            }
            for ( auto & site : collect.sites ) {
                processSite(caller, site, state);
            }
        }

        // false = skip silently: an invoke whose arg0 does not trace to a spliceable literal
        bool InlinePatch::resolveSubject ( const PlannedSite & site, CallerSpliceState & state, SpliceSubject & subj ) {
            auto callLike = site.callLike;
            if ( site.kind==SiteKind::InvokeBlock ) {
                auto inv = static_cast<ExprInvoke *>(callLike);
                Expression * a0 = inv->arguments[0];
                if ( a0->rtti_isR2V() ) a0 = static_cast<ExprRef2Value *>(a0)->subexpr;
                if ( a0->rtti_isMakeBlock() ) {
                    subj.literal = static_cast<ExprMakeBlock *>(a0);
                } else if ( a0->rtti_isVar() ) {
                    auto v = static_cast<ExprVar *>(a0);
                    if ( v->variable ) {
                        auto bit = state.bindings.find(v->variable);
                        if ( bit!=state.bindings.end() ) {
                            if ( bit->second.withModule != site.withModule ) {
                                decline(site, "block literal binding crosses a module resolution scope", callLike->at);
                                return false;
                            }
                            subj.literal = bit->second.literal;
                            // the splice eats this read, usually the holder's only one. lint runs
                            // after the patch slot but before optimize reaps the dead `let`, so it
                            // would report LINT002 on a variable the user demonstrably uses
                            v->variable->marked_used = true;
                        }
                    }
                }
                if ( !subj.literal ) return false;
                subj.name = "block";
            } else {
                subj.fn = static_cast<ExprCallFunc *>(callLike)->func;  // ExprCall or ExprOp1/2/3
                subj.name = subj.fn->name;
            }
            return true;
        }

        // the site's statement index in its anchor block, or -1 to skip (next round re-collects)
        int InlinePatch::liveAnchorIndex ( const PlannedSite & site, CallerSpliceState & state ) {
            auto callLike = site.callLike;
            int anchorIndex = site.anchor.index + state.indexShift[site.anchor.block];     // if the block shifted, adjust the index
            if ( anchorIndex < 0 || anchorIndex >= int(site.anchor.block->list.size())
                || site.anchor.block->list[anchorIndex] != site.stmt ) {
                return -1;      // an earlier splice this round rewrote or detached the statement
            }
            bool siteLive = exprContains(site.stmt, callLike);
            if ( !siteLive && site.stmt->rtti_isWhile() ) {             // the call may sit in the while condition
                siteLive = exprContains(static_cast<ExprWhile *>(site.stmt)->cond, callLike);
            }
            if ( !siteLive && site.stmt->rtti_isIfThenElse() ) {        // or in an elif condition down the if_false chain
                auto lite = static_cast<ExprIfThenElse *>(site.stmt);
                while ( lite->if_false && lite->if_false->rtti_isIfThenElse() ) {
                    auto inner = static_cast<ExprIfThenElse *>(lite->if_false);
                    if ( exprContains(inner->cond, callLike) ) { siteLive = true; break; }
                    lite = inner;
                }
            }
            return siteLive ? anchorIndex : -1;
        }

        // false = this site does not splice: declined, failed (reported), or a defensive skip
        bool InlinePatch::canInlineSubjectAtSite ( const PlannedSite & site, const SpliceSubject & subj, SpliceInputs & in ) {
            auto callLike = site.callLike;
            if ( site.kind==SiteKind::InvokeBlock ) {
                string why;
                if ( !invokeBlockShapeOk(subj.literal, why) ) { decline(site, why, callLike->at); return false; }
                auto blk = static_cast<ExprBlock *>(subj.literal->block);
                auto inv = static_cast<ExprInvoke *>(callLike);
                if ( inv->arguments.size() != blk->arguments.size()+1 ) { decline(site, "argument count mismatch", callLike->at); return false; }
                in.body = blk;
                if ( blk->returnType && !blk->returnType->isVoid() ) in.result = blk->returnType;
                in.sa.args = &inv->arguments; in.sa.ofs = 1; in.sa.params = &blk->arguments;
            } else {
                Function * calleeFn = subj.fn;
                if ( site.kind==SiteKind::MustCall ) {
                    if ( !calleeShapeOk(calleeFn) ) return false;   // lint reports at the declaration
                } else {
                    string why;
                    if ( !autoCalleeOk(calleeFn, why) ) { decline(site, subj.name + ": " + why, callLike->at); return false; }
                }
                if ( callLike->rtti_isCall() ) {
                    auto call = static_cast<ExprCall *>(callLike);
                    if ( call->arguments.size() != calleeFn->arguments.size() ) return false; // impossible post-infer (defaults are materialized); guards the classifier's lockstep arg/param walk
                    in.sa.args = &call->arguments;
                } else {    // operator arguments flatten to opArgs
                    if ( callLike->rtti_isOp1() ) {
                        in.opArgs.push_back(static_cast<ExprOp1 *>(callLike)->subexpr);
                    } else if ( callLike->rtti_isOp2() ) {
                        auto op2 = static_cast<ExprOp2 *>(callLike);
                        in.opArgs.push_back(op2->left);
                        in.opArgs.push_back(op2->right);
                    } else {
                        auto op3 = static_cast<ExprOp3 *>(callLike);
                        in.opArgs.push_back(op3->subexpr);
                        in.opArgs.push_back(op3->left);
                        in.opArgs.push_back(op3->right);
                    }
                    if ( in.opArgs.size() != calleeFn->arguments.size() ) return false; // a defaulted operator overload has no operand slot to pad; skip, don't walk out of lockstep
                    in.sa.args = &in.opArgs;
                }
                Module * originModule = originOf(calleeFn)->module;
                // a user-written `with (module ...)` may spell a require path; module names are bare
                string siteWith = site.withModule;
                auto slash = siteWith.find_last_of('/');
                if ( slash != string::npos ) siteWith.erase(0, slash+1);   // foo/bar/farr -> farr
                if ( !siteWith.empty() && siteWith != originModule->name ) {
                    siteFail(site, "can't inline " + subj.name + " here: the call site resolves inside with (module "
                        + site.withModule + ")", callLike->at);
                    return false;
                }
                if ( originModule != program->thisModule.get() ) {
                    auto & verdict = privateUse(calleeFn, originModule);
                    if ( !verdict.hard.empty() ) {
                        siteFail(site, "can't inline " + subj.name + " across modules: " + verdict.hardWhy, callLike->at);
                        return false;
                    }
                    // scope-only needs splice under a generated with (module <origin>);
                    // a site already inside that scope splices bare - re-wrapping its
                    // interior sites every round would never converge
                    in.needScope = !verdict.scope.empty()
                        && siteWith != originModule->name;
                    if ( in.needScope ) in.scopeModule = originModule->name;
                }
                in.body = static_cast<ExprBlock *>(calleeFn->body);
                if ( calleeFn->result && !calleeFn->result->isVoid() ) in.result = calleeFn->result;
                in.sa.ofs = 0; in.sa.params = &calleeFn->arguments;
            }
            return true;
        }

        // manufactures the `let _inl<N>_arg_<param>` temp and plans its read
        bool InlinePatch::tryBindArgTemp ( const PlannedSite & site, const string & subjName, const VariablePtr & param,
                bool callerIsGenerator, Expression * init, bool cnst, bool ref, bool viaMove,
                vector<ExpressionPtr> & temps, ArgSub & sub ) {
            if ( !tempTypeIsLocal(init, ref, callerIsGenerator) ) {
                siteFail(site, "can't inline " + subjName + ": argument '" + param->name
                    + "' needs a temporary, and " + init->type->describe()
                    + " can't be a local variable", site.callLike->at);
                return false;
            }
            string tname = joinInlineName(INLINE_TEMP_PREFIX + to_string(program->thisModule->inlineTempIndex) + "_arg_", param->name);
            temps.push_back(makeTemp(site.callLike->at, tname, init, cnst, ref, viaMove));
            sub.tempName = tname;
            return true;
        }

        // plans how every argument reaches the spliced body - tier A/B substitution or a
        // manufactured `_inl<N>_arg_*` temp. false is always reported: a compile error
        // on a MUST site, a silent decline otherwise
        bool InlinePatch::classifyArguments ( Function * caller, const PlannedSite & site, const SpliceSubject & subj, const SpliceInputs & in, ArgPlan & plan ) {
            auto callLike = site.callLike;
            const string & subjName = subj.name;
            auto & sa = in.sa;
            bool needScope = in.needScope;
            auto & paramSub = plan.paramSub;
            auto & stats = paramStats(in.body, *sa.params);
            bool writeFree = subj.fn ? calleeWriteFree(subj.fn) : false;   // a block body is judged unknown
            for ( size_t ai=0, ais=sa.count(); ai!=ais; ++ai ) {
                Expression * arg = sa.arg(ai);
                auto & param = sa.param(ai);
                ArgSub sub;
                bool byRefParam = param->type->ref || param->type->isRefType();
                bool varParam = !param->type->constant;
                Expression * argLeaf = arg;
                if ( argLeaf->rtti_isR2V() ) argLeaf = static_cast<ExprRef2Value *>(argLeaf)->subexpr;
                bool leafConst = argLeaf->rtti_isConstant();
                bool leafVar = argLeaf->rtti_isVar();
                auto makeArgTemp = [&]( Expression * init, bool cnst, bool ref, bool viaMove ) {   // binds the per-argument context
                    return tryBindArgTemp(site, subjName, param, caller->generator, init, cnst, ref, viaMove, plan.temps, sub);
                };
                if ( argLeaf->rtti_isTypeDecl() ) {   // type<...> witness: a compile-time tag - a temp would "use" it
                    if ( needScope ) {              // a generic alias can't cross the scope; a resolved witness travels by pointer
                        auto td = static_cast<ExprTypeDecl *>(argLeaf);
                        if ( td->typeexpr && td->typeexpr->isAutoOrAlias() ) {
                            siteFail(site, "can't inline " + subjName + ": generic alias argument '"
                                + param->name + "' can't cross the module scope", callLike->at);
                            return false;
                        }
                    }
                    sub.substitute = argLeaf;
                    paramSub[param] = sub;
                    continue;
                }
                if ( arg->type && arg->type->baseType==Type::tIterator && !leafVar ) {
                    siteFail(site, "can't inline " + subjName + ": iterator argument '"
                        + param->name + "' borrows call-scoped storage", callLike->at);
                    return false;
                }
                // past top-level ref/const, the argument's type must EQUAL the parameter's -
                // a substitution would hand the body a deeper flavor (element const, temp-ness)
                // than it was typed against, and the optimizer trusts types
                if ( site.kind!=SiteKind::MustCall && arg->type && param->type ) {
                    if ( !stripConstRef(arg->type)->isSameType(*stripConstRef(param->type), RefMatters::yes,
                            ConstMatters::yes, TemporaryMatters::yes, AllowSubstitute::no, false) ) {
                        decline(site, subjName + ": argument '" + param->name + "' flavor '"
                            + arg->type->describe() + "' vs parameter '" + param->type->describe() + "'", callLike->at);
                        return false;
                    }
                }
                // the literal's body splices too, losing the call-boundary lifetime fence a `#` call needs
                if ( site.kind!=SiteKind::MustCall && argLeaf->rtti_isMakeBlock() && hasTempArgumentMismatch(argLeaf) ) {
                    decline(site, subjName + ": block argument carries a temporary-flavored call", callLike->at);
                    return false;
                }
                if ( byRefParam ) {
                    bool sameConstView = !param->type->constant || (argLeaf->type && argLeaf->type->constant);
                    bool globalUnderScope = needScope && leafVar
                        && static_cast<ExprVar *>(argLeaf)->isGlobalVariable();
                    if ( leafVar && sameConstView && !globalUnderScope ) {
                        sub.substitute = argLeaf;             // same aliasing as the call itself
                    } else if ( leafVar ) {                 // const-widened var: bind a const reference - falling through would materialize
                        auto init = arg->clone();
                        init->alwaysSafe = true;            // generated binding to real storage
                        if ( !makeArgTemp(init, !varParam, true, false) ) return false;
                    } else if ( argLeaf->rtti_isMakeBlock() ) {
                        // a once-read literal substitutes (a `#` block parameter does not survive a
                        // holder binding); multi-read binds a holder and block inlining takes over
                        int reads = 0;
                        auto rit = stats.readCount.find(param);
                        if ( rit != stats.readCount.end() ) reads = rit->second;
                        bool underLoop = stats.readUnderLoop.find(param)!=stats.readUnderLoop.end();
                        if ( reads<=1 && !underLoop && !needScope ) {
                            sub.substitute = argLeaf;
                        } else if ( needScope && param->type->temporary ) {
                            siteFail(site, "can't inline " + subjName + ": block argument '"
                                + param->name + "' can't bind outside the module scope", callLike->at);
                            return false;
                        } else {
                            if ( !makeArgTemp(arg->clone(), false, false, true) ) return false;
                        }
                    } else if ( arg->type && (arg->type->ref || rootsInVar(argLeaf)) ) {  // lvalue chain: bind a reference once, like the call did
                        auto init = arg->clone();
                        init->alwaysSafe = true;            // generated binding to real storage
                        if ( !makeArgTemp(init, !varParam, true, false) ) return false;
                    } else if ( varParam && !isProvenRvalue(argLeaf) ) {
                        siteFail(site, "can't inline " + subjName + ": argument '" + param->name
                            + "' may alias caller storage the callee writes", callLike->at);
                        return false;
                    } else {
                        // rvalue into a ref param: materialize. a block holder must be var -
                        // const would propagate into the invoke, rejecting the block's own var params
                        bool nonCopyable = arg->type && !arg->type->canCopy();
                        bool blockValue = arg->type && arg->type->baseType==Type::tBlock;
                        if ( !makeArgTemp(arg->clone(), !varParam && !blockValue, false, nonCopyable) ) return false;
                    }
                } else if ( varParam ) {                    // a mutable by-value param IS the local copy
                    if ( !makeArgTemp(arg->clone(), false, false, arg->type && !arg->type->canCopy()) ) return false;
                } else if ( leafConst ) {
                    if ( stats.readAsRefArg.find(param)!=stats.readAsRefArg.end() ) {
                        if ( !makeArgTemp(arg->clone(), true, false, false) ) return false;
                    } else if ( needScope && argLeaf->type && argLeaf->type->isEnumT() ) {  // an enum re-resolves by name - the wrong module under the wrap
                        if ( !makeArgTemp(arg->clone(), true, false, false) ) return false;
                    } else {
                        sub.substitute = argLeaf;
                    }
                } else if ( leafVar ) {
                    auto av = static_cast<ExprVar *>(argLeaf);
                    // snapshot hazard: the body could write the same storage through a var by-ref param or a global
                    bool hazard = false;
                    if ( av->variable ) {
                        for ( size_t oi=0; oi!=ais; ++oi ) {
                            if ( oi==ai ) continue;
                            auto & otherParam = sa.param(oi);
                            bool oRef = otherParam->type->ref || otherParam->type->isRefType();
                            if ( !oRef || otherParam->type->constant ) continue;
                            Expression * oleaf = sa.arg(oi);
                            if ( oleaf->rtti_isR2V() ) oleaf = static_cast<ExprRef2Value *>(oleaf)->subexpr;
                            if ( oleaf->rtti_isVar() && static_cast<ExprVar *>(oleaf)->variable==av->variable ) {
                                hazard = true;
                                break;
                            }
                        }
                        if ( !hazard && (av->isGlobalVariable() || av->variable->access_ref) ) {
                            hazard = !writeFree;
                        }
                        if ( needScope && av->isGlobalVariable() ) hazard = true;  // the unqualified name would re-resolve inside the wrap
                    }
                    bool constWidened = param->type->constant && !(argLeaf->type && argLeaf->type->constant);  // an ==const-locked callee stops matching on re-infer (30341)
                    if ( hazard || constWidened ) {
                        if ( !makeArgTemp(arg->clone(), true, false, false) ) return false;
                    } else {
                        sub.substitute = argLeaf;
                    }
                } else {
                    // tier B: pure, read at most once, never under a loop
                    int reads = 0;
                    auto rit = stats.readCount.find(param);
                    if ( rit != stats.readCount.end() ) reads = rit->second;
                    bool underLoop = stats.readUnderLoop.find(param)!=stats.readUnderLoop.end();
                    bool canCalleeInvalidate = !writeFree && !argReadsOnlyPrivateLocals(arg, sa);
                    bool needsStorage = stats.readAsRefArg.find(param)!=stats.readAsRefArg.end()
                        && !(arg->type && arg->type->ref);
                    if ( reads<=1 && !underLoop && isReorderSafe(arg) && !canCalleeInvalidate && !needScope && !needsStorage ) {
                        sub.substitute = arg;
                    } else {
                        if ( !makeArgTemp(arg->clone(), true, false, arg->type && !arg->type->canCopy()) ) return false;
                    }
                }
                paramSub[param] = sub;
            }
            return true;
        }

        // ----- pure graft: no statements, no anchors, legal in any position -----
        //      def foo(a, b : int) => a + b
        //      let x = foo(p, 2) * k   =>   let x = (p + 2) * k
        // the call node is replaced in place. a can_shadow block argument in the body
        // keeps its semantic name, so a same-named substituted caller expression would
        // SILENTLY re-bind to it after the splice - the check below refuses such sites
        void InlinePatch::graftExpressionBody ( Function * caller, const PlannedSite & site, const SpliceSubject & subj,
                const SpliceInputs & in, ArgPlan & plan, CallerSpliceState & state, int anchorIndex ) {
            auto callLike = site.callLike;
            auto & inlineId = program->thisModule->inlineTempIndex;
            das_hash_set<string> argFreeNames;
            {
                // every actual argument: a block-literal argument moves in wholesale, free names included
                FreeNameCollect fnc(argFreeNames);
                for ( size_t ai=0, ais=in.sa.count(); ai!=ais; ++ai ) in.sa.arg(ai)->visit(fnc);
            }
            das_hash_map<string, string> rename;
            das_hash_set<string> canShadowArgs;
            LocalNameCollect pnames(rename, canShadowArgs, INLINE_TEMP_PREFIX + to_string(inlineId) + "_l_");
            in.body->visit(pnames);
            for ( auto & csn : canShadowArgs ) {
                if ( argFreeNames.count(csn) || state.callerDeclNames.count(csn) ) {
                    siteFail(site, "can't inline " + subj.name
                        + ": a substituted expression would be captured by a can_shadow block argument", callLike->at);
                    return;
                }
            }
            InlineBodyRewriter rewriter(plan.paramSub, rename, callLike->at, subj.fn != nullptr);
            auto ret = static_cast<ExprReturn *>(in.body->list.back());
            ExpressionPtr callReplacement = nullptr;
            if ( ret->subexpr ) {
                auto cloned = ret->subexpr->clone();
                if ( subj.fn ) {    // callee-origin lines; a literal's body is already caller code
                    SpliceAtStamp stamp(callLike->at);
                    cloned = cloned->visit(stamp);
                }
                callReplacement = cloned->visit(rewriter);
            }
            if ( site.stmt==callLike ) {
                auto & list = site.anchor.block->list;
                if ( callReplacement ) {
                    list[anchorIndex] = callReplacement;
                } else {
                    list.erase(list.begin()+anchorIndex);
                    state.indexShift[site.anchor.block] -= 1;
                }
            } else {
                if ( !callReplacement ) {
                    siteFail(site, "can't inline void " + subj.name + " here: the call is not a statement", callLike->at);
                    return;
                }
                ReplaceNode rn(callLike, callReplacement);
                auto & slot = site.anchor.block->list[anchorIndex];
                slot = slot->visit(rn);
                if ( !rn.done ) {
                    error("internal error: inlined call not found in its statement", callLike->at);
                    return;
                }
            }
            recordCompletedSite(site, caller, subj.name);
            inlineId ++;
        }

        // a statement-needing splice in a conditionally- or repeatedly-evaluated position must wait: rewrite
        // the position into a splice-friendly shape and let the next round splice there.
        // true = the site was consumed (lowered, or refused with a report); false = the
        // position is eager - the splice proceeds
        bool InlinePatch::tryLowerCallPosition ( const PlannedSite & site, const SpliceSubject & subj,
                CallerSpliceState & state, int anchorIndex ) {
            auto callLike = site.callLike;
            const string & subjName = subj.name;
            auto & inlineId = program->thisModule->inlineTempIndex;
            auto & indexShift = state.indexShift;
            if ( site.stmt->rtti_isWhile() ) {
                auto wh = static_cast<ExprWhile *>(site.stmt);
                if ( exprContains(wh->cond, callLike) && wh->body->rtti_isBlock() ) {
                    // while(C) -> while(true) { if (!(C)) break; ... } - next round splices in the body
                    auto brkBlock = new ExprBlock();
                    brkBlock->at = wh->cond->at;
                    brkBlock->list.push_back(new ExprBreak(wh->cond->at));
                    auto guard = new ExprIfThenElse(wh->cond->at,
                        new ExprOp1(wh->cond->at, "!", wh->cond), brkBlock, nullptr);
                    auto oldBody = static_cast<ExprBlock *>(wh->body);
                    oldBody->list.insert(oldBody->list.begin(), guard);
                    wh->cond = new ExprConstBool(wh->cond->at, true);
                    changed = true;
                    return true;
                }
            }
            if ( site.stmt->rtti_isIfThenElse() ) {
                // if (a) ... elif (C) ... -> if (a) ... else { if (C) ... } - the inner if
                // becomes an anchorable statement; next round splices at its condition
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
                    return true;
                }
            }
            PathScan path;
            auto pos = findPath(site.stmt, callLike, path);
            if ( pos==SitePosition::NotFound ) {
                siteFail(site, "can't inline " + subjName + " here: the call sits in a position the splicer can't reach - hoist the call into its own statement", callLike->at);
                return true;
            }
            if ( pos==SitePosition::Unsupported ) {
                siteFail(site, "can't inline " + subjName + " inside ?? or a safe-navigation suffix - hoist the call into its own statement", callLike->at);
                return true;
            }
            if ( pos==SitePosition::Conditional ) {
                auto condOp = path.conditionalOps.back();   // outermost conditional op on the path
                const string opName = condOp->rtti_isOp2() ? static_cast<ExprOp2 *>(condOp)->op : "?:";
                // &&=/||= produce no value, so the temp hoist below cannot apply - lower to
                // `if (a) a = call()`. the LHS is then read twice: only a plain variable qualifies
                if ( condOp->rtti_isOp2() ) {
                    auto sop = static_cast<ExprOp2 *>(condOp);
                    if ( sop->op=="&&=" || sop->op=="||=" ) {
                        if ( site.stmt!=condOp || !sop->left->rtti_isVar() ) {
                            siteFail(site, "can't inline " + subjName + " in the right side of " + sop->op + " - hoist the call into its own statement", callLike->at);
                            return true;
                        }
                        auto & list = site.anchor.block->list;
                        auto thenBlk = new ExprBlock();
                        thenBlk->at = sop->at;
                        thenBlk->list.push_back(new ExprCopy(sop->at, sop->left->clone(), sop->right, true));
                        ExpressionPtr cond = sop->left->clone();
                        if ( sop->op=="||=" ) cond = new ExprOp1(sop->at, "!", cond);
                        list[anchorIndex] = new ExprIfThenElse(sop->at, cond, thenBlk, nullptr);
                        changed = true;
                        return true;
                    }
                }
                // splitting into arm stores loses the ternary's per-arm coercion (void? panic
                // arms, null literals) and would copy a ref result - only same-typed by-value arms survive
                if ( condOp->rtti_isOp3() ) {
                    auto op3 = static_cast<ExprOp3 *>(condOp);
                    auto & lt = op3->left->type;
                    auto & rt = op3->right->type;
                    bool armsMatch = lt && rt && condOp->type && !condOp->type->ref
                        && !lt->isVoid() && !rt->isVoid()
                        && !lt->isVoidPointer() && !rt->isVoidPointer()
                        && lt->isSameType(*rt, RefMatters::no, ConstMatters::no, TemporaryMatters::yes);
                    if ( !armsMatch ) {
                        siteFail(site, "can't inline " + subjName + " inside a mixed-type ternary - hoist the call into its own statement", callLike->at);
                        return true;
                    }
                }
                // rewrite var+if in place only for a GENERATED hoist temp - a user declaration
                // keeps its metadata (constness, aka), hoists fresh, and rewrites next round
                Variable * rootVar = nullptr;
                if ( site.stmt->rtti_isLet() ) {
                    auto let = static_cast<ExprLet *>(site.stmt);
                    if ( let->variables.size()==1 && let->variables[0]->init==condOp
                        && let->variables[0]->generated ) {
                        rootVar = let->variables[0];
                    }
                }
                auto & list = site.anchor.block->list;
                if ( !rootVar ) {
                    string tname = INLINE_TEMP_PREFIX + to_string(inlineId) + "_low";
                    inlineId ++;
                    // non-const: a `var p = <temp>` pointer consumer rejects a const reference
                    auto hoist = makeTemp(callLike->at, tname, condOp, false, false, condOp->type && !condOp->type->canCopy());
                    ReplaceNode rn(condOp, new ExprVar(callLike->at, tname));
                    list[anchorIndex] = list[anchorIndex]->visit(rn);
                    if ( !rn.done ) {
                        error("internal error: conditional operator not found in its statement", callLike->at);
                        return true;
                    }
                    list.insert(list.begin()+anchorIndex, hoist);
                    indexShift[site.anchor.block] += 1;
                    changed = true;
                    return true;
                }
                if ( !condOp->type ) {
                    siteFail(site, "can't inline " + subjName + ": '" + opName + "' carries no type - hoist the call into its own statement", callLike->at);
                    return true;
                }
                // the arms store into an uninitialized declaration (30316)
                if ( !condOp->type->ref && condOp->type->hasNonTrivialCtor() ) {
                    siteFail(site, "can't inline " + subjName + ": '" + opName + "' result type "
                        + condOp->type->describe() + " requires nontrivial construction - hoist the call into its own statement", callLike->at);
                    return true;
                }
                vector<ExpressionPtr> replacement;
                replacement.push_back(makeUninitDecl(site.stmt->at, rootVar->name, condOp->type));
                auto readT = [&]() { return new ExprVar(site.stmt->at, rootVar->name); };
                auto armStore = [&]( const LineInfo & at, Expression * src ) -> Expression * {   // mirror the hoist's own init semantics
                    if ( rootVar->init_via_move ) return new ExprMove(at, readT(), src, true);
                    return new ExprCopy(at, readT(), src, true);
                };
                if ( condOp->rtti_isOp3() ) {
                    // let t = c ? A : call()   =>   var t; if (c) { t = A } else { t = call() }
                    auto op3 = static_cast<ExprOp3 *>(condOp);
                    auto thenBlk = new ExprBlock();
                    thenBlk->at = op3->at;
                    thenBlk->list.push_back(armStore(op3->at, op3->left));
                    auto elseBlk = new ExprBlock();
                    elseBlk->at = op3->at;
                    elseBlk->list.push_back(armStore(op3->at, op3->right));
                    replacement.push_back(new ExprIfThenElse(op3->at, op3->subexpr, thenBlk, elseBlk));
                } else if ( condOp->rtti_isOp2() ) {
                    // let t = a && call()   =>   var t; t = a; if (t) { t = call() }   (|| negates the guard)
                    auto op2 = static_cast<ExprOp2 *>(condOp);
                    replacement.push_back(armStore(op2->at, op2->left));
                    auto thenBlk = new ExprBlock();
                    thenBlk->at = op2->at;
                    thenBlk->list.push_back(armStore(op2->at, op2->right));
                    ExpressionPtr cond = readT();
                    if ( op2->op=="||" ) cond = new ExprOp1(op2->at, "!", cond);
                    replacement.push_back(new ExprIfThenElse(op2->at, cond, thenBlk, nullptr));
                } else {
                    siteFail(site, "can't inline " + subjName + " inside '" + opName + "' - hoist the call into its own statement", callLike->at);
                    return true;
                }
                list.erase(list.begin()+anchorIndex);
                list.insert(list.begin()+anchorIndex, replacement.begin(), replacement.end());
                indexShift[site.anchor.block] += int(replacement.size()) - 1;
                changed = true;
                return true;
            }
            return false;
        }

        // ----- eager position: hoist the side-effecting prefix, splice temps and body -----
        //      let x = g() + foo(h(), 2)   =>   let _inl0_pre0 = g()
        //                                       let _inl0_arg_a = h()
        //                                       var _inl0_res : int
        //                                       { foo's body: a -> _inl0_arg_a, b -> 2, returns -> _inl0_res }
        //                                       let x = _inl0_pre0 + _inl0_res
        // every declinable check runs BEFORE the prefix hoist mutates the statement - a
        // later decline would orphan the _pre reads (30838, or a segfault in access flags)
        void InlinePatch::spliceStatements ( Function * caller, const PlannedSite & site, const SpliceSubject & subj,
                const SpliceInputs & in, ArgPlan & plan, CallerSpliceState & state, int anchorIndex, bool exprBody ) {
            auto callLike = site.callLike;
            Function * calleeFn = subj.fn;              // null for InvokeBlock
            const string & subjName = subj.name;
            auto & inlineId = program->thisModule->inlineTempIndex;
            auto & indexShift = state.indexShift;
            auto & temps = plan.temps;
            vector<Expression *> prefix;
            collectSideEffectPrefix(site.stmt, callLike, prefix);
            for ( auto pe : prefix ) {
                if ( pe->type && pe->type->isVoid() ) {
                    siteFail(site, "can't inline " + subjName + " here: void expression in the evaluation prefix", callLike->at);
                    return;
                }
                if ( pe->type && pe->type->baseType==Type::tIterator && !pe->rtti_isVar() ) {
                    // same borrow hazard as iterator arguments
                    siteFail(site, "can't inline " + subjName + " here: iterator expression in the evaluation prefix", callLike->at);
                    return;
                }
            }
            // the shape checks only cover a refType result - a by-value nontrivial ctor reaches here (30316)
            if ( in.result && !in.result->ref && in.result->hasNonTrivialCtor() ) {
                siteFail(site, "can't inline " + subjName + ": result type "
                    + in.result->describe() + " requires nontrivial construction", callLike->at);
                return;
            }
            das_hash_set<string> argFreeNames;
            {
                // every actual argument, free names included; collected before the prefix hoist rewrites them
                FreeNameCollect fnc(argFreeNames);
                for ( size_t ai=0, ais=in.sa.count(); ai!=ais; ++ai ) in.sa.arg(ai)->visit(fnc);
            }
            das_hash_map<string, string> rename;
            das_hash_set<string> canShadowArgs;
            // the _l_ sub-namespace keeps callee locals clear of this site's _res/_arg_*/_pre*/_low temps
            LocalNameCollect names(rename, canShadowArgs, INLINE_TEMP_PREFIX + to_string(inlineId) + "_l_");
            in.body->visit(names);
            for ( auto & csn : canShadowArgs ) {
                if ( argFreeNames.count(csn) || state.callerDeclNames.count(csn) ) {
                    siteFail(site, "can't inline " + subjName
                        + ": a substituted expression would be captured by a can_shadow block argument", callLike->at);
                    return;
                }
            }
            vector<ExpressionPtr> splice;
            // stamping is gated on calleeFn throughout - an invoke-block literal is
            // caller code with honest lines already
            SpliceAtStamp stamp(callLike->at);
            int preIdx = 0;
            for ( auto pe : prefix ) {
                string tname = INLINE_TEMP_PREFIX + to_string(inlineId) + "_pre" + to_string(preIdx++);
                auto peAt = pe->at;                 // the host-side read keeps the real position
                ExprLet * tl = nullptr;
                // non-const throughout: a const temp read would stop matching var pointer params downstream
                if ( pe->type && pe->type->ref ) {
                    // lvalue: bind a reference - identity survives, writes land in the original place
                    pe->alwaysSafe = true;          // generated binding to real storage
                    // callLike->at, not pe->at: the temp must be in scope at the splice's park point
                    tl = makeTemp(callLike->at, tname, pe, pe->type->constant, true, false);
                } else {
                    tl = makeTemp(callLike->at, tname, pe, false, false, pe->type && !pe->type->canCopy());
                }
                if ( calleeFn ) {
                    stamp.markGenerated = false;    // caller-authored init, only relocated
                    tl->visit(stamp);
                    stamp.markGenerated = true;
                }
                splice.push_back(tl);
                ReplaceNode rn(pe, new ExprVar(peAt, tname));
                auto & slot = site.anchor.block->list[anchorIndex];
                slot = slot->visit(rn);
                if ( !rn.done ) {
                    error("internal error: prefix expression not found in its statement", callLike->at);
                    return;
                }
            }
            InlineBodyRewriter rewriter(plan.paramSub, rename, callLike->at, calleeFn != nullptr);
            ExpressionPtr callReplacement = nullptr;
            if ( calleeFn ) {
                stamp.markGenerated = false;    // caller-authored arg inits, only relocated
                for ( auto & t : temps ) t->visit(stamp);   // arg temps read as the call site too
                stamp.markGenerated = true;
            }
            if ( exprBody ) {
                auto ret = static_cast<ExprReturn *>(in.body->list.back());
                if ( ret->subexpr ) {
                    auto cloned = ret->subexpr->clone();
                    if ( calleeFn ) cloned = cloned->visit(stamp);
                    callReplacement = cloned->visit(rewriter);
                }
                for ( auto & t : temps ) splice.push_back(t);
            } else {
                auto bodyClone = static_cast<ExprBlock *>(in.body->clone());
                if ( calleeFn ) bodyClone->visit(stamp);
                if ( site.kind==SiteKind::InvokeBlock ) {
                    // parameters became substitutions, the result becomes the result temp - a plain scope block remains
                    bodyClone->arguments.clear();
                    bodyClone->annotations.clear();
                    bodyClone->returnType = nullptr;
                    bodyClone->isClosure = false;
                    bodyClone->isLambdaBlock = false;
                    bodyClone->hasReturn = false;
                    bodyClone->copyOnReturn = false;
                    bodyClone->moveOnReturn = false;
                }
                ReturnStoreRewrite rsr(in.result ? INLINE_TEMP_PREFIX + to_string(inlineId) + "_res" : string(),
                    INLINE_TEMP_PREFIX + to_string(inlineId) + "_ret");
                // _res goes FIRST: a no-init decl zero-fills its slot, so the walk reads
                // zeros until a return-store writes it
                if ( in.result ) {
                    auto resLet = makeUninitDecl(callLike->at, rsr.resName, in.result);
                    splice.insert(splice.begin(), resLet);
                }
                rsr.apply(bodyClone);
                if ( in.result ) {
                    callReplacement = new ExprVar(callLike->at, rsr.resName);
                }
                ExpressionPtr spliced = bodyClone->visit(rewriter);
                if ( calleeFn && calleeFn->hasUnsafe ) {    // spliced => unsafe { spliced } - foldUnsafe ate the callee's own wrappers
                    auto wrap = new ExprUnsafe(callLike->at);
                    wrap->body = spliced;
                    wrap->generated = true;
                    spliced = wrap;
                }
                if ( in.needScope ) {                       // spliced => with (module <origin>) { spliced }
                    // arg and prefix temps stay OUTSIDE (caller code resolves at the caller); the
                    // unsafe wrapper stays INSIDE so later splices anchor within its authorization
                    ExprBlock * host = nullptr;
                    if ( spliced->rtti_isBlock() ) {
                        host = static_cast<ExprBlock *>(spliced);
                    } else {
                        host = new ExprBlock();
                        host->at = callLike->at;
                        host->list.push_back(spliced);
                    }
                    auto scopeWrap = new ExprWith(callLike->at);
                    scopeWrap->moduleName = in.scopeModule;
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
                if ( spliced->rtti_isBlock() && !(in.result && in.result->ref) ) spliced->generated = true;
                splice.push_back(spliced);
            }
            auto & list = site.anchor.block->list;
            list.insert(list.begin()+anchorIndex, splice.begin(), splice.end());
            indexShift[site.anchor.block] += int(splice.size());
            int stmtIndex = anchorIndex + int(splice.size());
            if ( list[stmtIndex]==callLike ) {
                if ( callReplacement ) {
                    list[stmtIndex] = callReplacement;
                } else {
                    list.erase(list.begin()+stmtIndex);
                    indexShift[site.anchor.block] -= 1;
                }
            } else {
                ReplaceNode rn(callLike, callReplacement);
                list[stmtIndex] = list[stmtIndex]->visit(rn);
                if ( !rn.done ) {
                    error("internal error: inlined call not found in its statement", callLike->at);
                    return;
                }
            }
            recordCompletedSite(site, caller, subjName, in.scopeModule);
            inlineId ++;
        }

        // every early return is a completed action, a MUST-site error, or a silent
        // best-effort decline
        void InlinePatch::processSite ( Function * caller, const PlannedSite & site, CallerSpliceState & state ) {
            auto & inlineId = program->thisModule->inlineTempIndex;
            auto callLike = site.callLike;
            if ( site.kind==SiteKind::MustCall && !mustEnabled ) return;
            SpliceSubject subj;
            if ( !resolveSubject(site, state, subj) ) return;
            Function * calleeFn = subj.fn;              // null for InvokeBlock
            const string & subjName = subj.name;
            if ( !site.stmt || !site.anchor.block ) {
                siteFail(site, "can't inline " + subjName + ": the call has no anchoring statement in a function body - hoist the call into its own statement", callLike->at);
                return;
            }
            int anchorIndex = liveAnchorIndex(site, state);
            if ( anchorIndex < 0 ) return;
            // lint runs after the patch slot - splicing would erase the 30250 error, not the UB
            if ( site.kind!=SiteKind::MustCall && statementHasTableLookupCollision(site.stmt) ) {
                decline(site, "the statement has a potential table lookup collision (error 30250 preserved for lint)", callLike->at);
                return;
            }
            if ( site.kind==SiteKind::AutoCall && calleeFn ) {
                int64_t calleeBytes = bodyCost(calleeFn->body).stackBytes;
                if ( state.grownBytes + calleeBytes > AUTO_INLINE_CALLER_GROWTH_BYTES ) {
                    decline(site, "caller frame growth budget exhausted this round", callLike->at);
                    return;
                }
                state.grownBytes += calleeBytes;
            }
            SpliceInputs in;
            if ( !canInlineSubjectAtSite(site, subj, in) ) return;
            // a cross-module callee body carries _inl locals minted by its own module's
            // counter - start above them, or the rename map captures this site's names
            if ( calleeFn && calleeFn->module && calleeFn->module->inlineTempIndex > inlineId ) {
                inlineId = calleeFn->module->inlineTempIndex;
            }
            ArgPlan plan;
            if ( !classifyArguments(caller, site, subj, in, plan) ) return;
            bool exprBody = in.body->list.size()==1 && in.body->list.back()->rtti_isReturn()
                && !static_cast<ExprReturn *>(in.body->list.back())->moveSemantics    // a substituted read would COPY where the callee moved
                && !(calleeFn && calleeFn->hasUnsafe)   // the generated unsafe wrapper must be a statement
                && !in.needScope;                       // so must the with (module) wrapper
            bool needStatements = !plan.temps.empty() || !exprBody;
            if ( !needStatements ) {
                graftExpressionBody(caller, site, subj, in, plan, state, anchorIndex);
                return;
            }
            if ( tryLowerCallPosition(site, subj, state, anchorIndex) ) return;
            spliceStatements(caller, site, subj, in, plan, state, anchorIndex, exprBody);
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
        //  (1) if (c) { ...; return }; TAIL          =>   if (c) { ...; return } else { TAIL }
        //  (2) if (c) { return a } else { return b } =>   return c ? a : b
        // together they turn early-exit bodies into terminal-return shapes the splicer
        // stores in place with zero overhead. new nodes are untyped - the restarted infer
        // legalizes them, the same protocol as a splice. the optimize-time CondFolding
        // copy stays: it serves compiles this pre-pass never sees
        class ReturnCanonicalization : public Visitor {
        public:
            bool changed = false;
        protected:
            virtual bool canVisitQuoteSubexpression ( ExprQuote * ) override { return false; }
            // the arm's single `return ...`, or null. a finally-carrying block never
            // qualifies (the fold would drop the finally); neither does a CMRES make
            // (the make-local protocol doesn't survive under an Op3 arm)
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
            // infer expects parser shapes - an if arm is a block. an inner fold can leave
            // a naked return as an elif arm; re-block it when the outer if did not fold
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
                        // the merged ternary loses per-arm return coercion, so the arms must
                        // already match EACH OTHER (`return derived?` vs `return base?` stays
                        // an if, so does `return null`); untyped arms fold on the next round
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
                        } else if ( !lr->subexpr && !rr->subexpr && isReorderSafe(expr->cond) ) {
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
                // a finally resolves block locals BY NAME on re-infer - a synthesized else
                // would nest the tail's declarations out of its sight
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
            if ( fn->neverInline ) return false;
            if ( moduleNeverInlines(fn) ) return false;
            // the rewrite costs a re-infer round, so only plausible splice candidates
            // qualify: [inline], a block parameter, or - heuristic tier on - a body near
            // the auto-inline budget (25% slack: folding can shrink it in) or budget-exempt
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
                if ( hasGotoOrLabel ) return;
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
        // deliberately option-only, no CodeOfPolicies field: a policy would stamp every
        // shared module compiled under it, and the bit outlives the program that set it
        if ( options.getBoolOption("never_inline", false) ) {
            thisModule->neverInline = true;     // downstream compiles check the bit (moduleNeverInlines)
            return false;
        }
        bool mustEnabled = !options.getBoolOption("disable_inline", policies.disable_inline);
        // the debugger needs true frames and true lines - same reason it disables fastcall
        bool autoEnabled = getOptimize() && !getDebugger()
            && !options.getBoolOption("disable_auto_inline", policies.disable_auto_inline);
        // under `options no_aliasing` call-result aliasing is an ERROR; splicing calls away
        // would change which programs compile - diagnostics must not depend on a perf knob
        bool autoFns = autoEnabled
            && options.getBoolOption("auto_inline_functions", policies.auto_inline_functions)
            && !options.getBoolOption("no_aliasing", policies.no_aliasing);
        das_hash_map<Function *, bool> budgetCache;
        das_hash_set<Function *> budgetExempt;
        AutoInlineCfg autoCfg;
        autoCfg.blockLiterals = autoEnabled;
        autoCfg.functions = autoFns;
        autoCfg.budget = options.getIntOption("auto_inline_cost", policies.auto_inline_cost);
        autoCfg.thisModule = thisModule.get();
        autoCfg.budgetCache = &budgetCache;
        autoCfg.budgetExempt = &budgetExempt;
        // private functions referenced exactly once - by a plain call in a body - are
        // budget-exempt: the splice MOVES the body (removeUnusedSymbols reaps the husk).
        // private is what makes the count sound: every possible reference is in front
        // of us now. any other reference kind (operator site, @@, initializer) disqualifies
        if ( autoFns && !failed() ) {
            das_hash_map<Function *, int> refs;         // every reference, anywhere
            das_hash_map<Function *, int> callSites;    // plain-call sites in function bodies
            auto exemptCandidate = [&](Function * f) -> bool {
                // instances are STAMPED private by instantiation - exempting them would
                // sweep library generics in (the heuristic tier keeps instances out anyway)
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
                    if ( !f || !exemptCandidate(f) ) return;
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
            for ( auto & structure : thisModule->structures.each() ) {
                for ( auto & fld : structure->fields ) {
                    if ( fld.init ) scanRefs(fld.init, false);
                }
            }
            for ( auto & kv : refs ) {
                if ( kv.second==1 && callSites[kv.first]==1 ) budgetExempt.insert(kv.first);
            }
        }
        // canonicalize ahead of candidacy; the reshape must settle through re-infer before
        // anything splices on top. gated on auto inlining: `disable_auto_inline` is the one
        // knob that promises "no patch-slot reshaping" to shape-pinning tests and macros
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
        InlinePatch patch(this, daScriptEnvironment::getBound()->g_compilerLog,
            options.getBoolOption("log_optimization", policies.log_optimization),
            mustEnabled && anyInline, autoCfg);
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
            && (patch.inlined || patch.inlinedAuto || patch.invokeBlocks || patch.declined) ) {
            *patch.logs << "INLINE: " << patch.inlined << " must + " << patch.inlinedAuto
                        << " auto + " << patch.invokeBlocks << " invoke-block site(s), "
                        << patch.declined << " declined in module " << thisModule->name << "\n";
        }
        return patch.changed;
    }

}
