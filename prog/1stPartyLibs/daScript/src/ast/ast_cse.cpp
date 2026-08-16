#include "daScript/misc/platform.h"

#include "daScript/ast/ast.h"
#include "daScript/ast/ast_visitor.h"
#include "daScript/ast/ast_expressions.h"
#include "daScript/ast/ast_cfg.h"
#include "daScript/misc/anyhash.h"

#include "ast_alias.h"

namespace das {

    // ===== Common-subexpression elimination (value reuse) =====
    // Replaces a pure expression E with a read of a local t when a statement-level
    // `let t = E` / `t = E` is available on every path to the occurrence (forward
    // availability over the statement-level CFG, ast_cfg.h) and t's declaration is
    // lexically visible there. No temps are created and no statements are inserted -
    // the pass only reuses values the program already named, so every rewrite strictly
    // removes work. Reuse is bit-exact (the reused value came from the same pure
    // computation over unchanged inputs), so there is no fast_math gate.
    //
    // Soundness model - two leaf disciplines (shared chain model in ast_alias.h):
    //  * VALUE leaves are ExprLet locals and function arguments of by-value types where
    //    every occurrence provably cannot modify the variable: a value read (r2v), a
    //    read-through (r2cr without write - const-ref passes, read chains), or the base
    //    of a statement-level ExprCopy store (a visible kill). Any other occurrence -
    //    addr(), mutable-ref pass, op-assign, move/clone target - disqualifies the
    //    variable. Pairs over value leaves die only on visible stores to their leaves;
    //  * MEMORY leaves are Field/At/Swizzle chains (ast_alias.h) rooted at any non-ref
    //    local or argument. They are never disqualified; instead a pair that reads
    //    memory is killed by any visible store through its base, by any store through
    //    an untracked target (pointer paths, block arguments - the statement goes
    //    OPAQUE), and by any call that may write memory the pass cannot see (anything
    //    beyond modifyArgument, whose writes stamp the argument chains at the call
    //    site). Writes through loop variables kill the loop source's base; stores
    //    through global or argument bases kill all argument-rooted pairs (an argument
    //    may alias a global or another argument, but never a local);
    //  * candidate expressions are side-effect-free trees of Op1/Op2/Op3/Call/Cast/
    //    Swizzle/Field/At over leaves and constants;
    //  * pointer/handle-returning calls are excluded even when flagged pure: a factory
    //    like clone_type returns a fresh, uniquely-owned node per evaluation - value-
    //    equal is not identity-equal (found live in daslib/linq_fold_common);
    //  * pairs (E -> t) come from `let t = E` and statement-level `t = E` where t is a
    //    clean VALUE leaf; make-block interiors neither generate pairs nor get
    //    rewritten (their execution count is unknown), but their stores and calls do
    //    contribute kills to the statement that owns the literal;
    //  * a rewrite additionally requires t's declaration to be lexically visible at the
    //    occurrence - dataflow availability alone is not enough: a pair born in a
    //    closed `{ }` scope may still be "available" after it, while t's stack slot is
    //    already reused by a sibling scope;
    //  * functions carrying control flow the CFG does not model are skipped whole:
    //    goto/label (also covers lowered generators), try/recover, and non-empty
    //    finally - same rule as DSE (ast_dse.cpp).

    namespace {

        // call-side effects that may write memory invisibly at the call site.
        // modifyArgument is visible (argument chains are write-stamped at the call by
        // propagateModifiedArguments); unsafe alone writes nothing; captureString
        // retains string data but writes no tracked memory. accessExternal shares the
        // modifyExternal bit, so pointer READS pay the conservative price
        constexpr uint32_t CSE_OPAQUE_CALL_EFFECTS =
              uint32_t(SideEffects::modifyExternal)
            | uint32_t(SideEffects::accessGlobal)
            | uint32_t(SideEffects::invoke)
            | uint32_t(SideEffects::userScenario);

        bool cseIsMove ( const Expression * e ) {
            return e->__rtti && strcmp(e->__rtti,"ExprMove")==0;
        }

        struct CseAnchor {
            ExprBlock * block = nullptr;
            int         index = -1;     // statement index within the block
        };

        // one walk: (a) bail on constructs the CFG does not model, (b) collect candidate
        // leaves and their declaration anchors, (c) disqualify any VALUE variable with
        // an occurrence that can modify it, (d) record the lexical anchor of every
        // expression the CFG carries as a statement (block statements, if/while
        // conditions, for sources, with subjects) for the scope-visibility check
        class CsePrescan : public Visitor {
        public:
            bool eligible = true;
            vector<Variable *> locals;                      // ExprLet locals of by-value type, in order
            vector<Variable *> allLocals;                   // every non-ref ExprLet local (chain bases)
            das_hash_set<Variable *> disq;                  // VALUE variables with a modifying-capable use
            das_hash_set<Expression *> storeBaseVars;       // base ExprVar nodes of statement-level store chains
            das_hash_map<Variable *, CseAnchor> declAt;     // where each local's ExprLet sits
            das_hash_map<Expression *, CseAnchor> anchorOf; // lexical anchor of potential CFG statements
            das_hash_map<ExprBlock *, CseAnchor> blockParent;
        protected:
            vector<CseAnchor> blockStack;
            Expression * curStmt = nullptr;
            CseAnchor curAnchor () const {
                return blockStack.empty() ? CseAnchor() : blockStack.back();
            }
            virtual bool canVisitQuoteSubexpression ( ExprQuote * ) override { return false; }
            virtual void preVisit ( ExprBlock * blk ) override {
                Visitor::preVisit(blk);
                if ( !blk->finalList.empty() ) eligible = false;
                blockParent[blk] = curAnchor();
                blockStack.push_back(CseAnchor{blk, -1});
            }
            virtual ExpressionPtr visit ( ExprBlock * blk ) override {
                blockStack.pop_back();
                return Visitor::visit(blk);
            }
            virtual void preVisitBlockExpression ( ExprBlock * block, Expression * expr ) override {
                Visitor::preVisitBlockExpression(block, expr);
                if ( !blockStack.empty() && blockStack.back().block==block ) blockStack.back().index++;
                anchorOf[expr] = curAnchor();
                curStmt = expr;
            }
            virtual void preVisit ( ExprTryCatch * expr ) override {
                Visitor::preVisit(expr);
                eligible = false;
            }
            virtual void preVisit ( ExprGoto * expr ) override {
                Visitor::preVisit(expr);
                eligible = false;
            }
            virtual void preVisit ( ExprLabel * expr ) override {
                Visitor::preVisit(expr);
                eligible = false;
            }
            virtual void preVisit ( ExprYield * expr ) override {
                Visitor::preVisit(expr);
                eligible = false;
            }
            virtual void preVisit ( ExprCopy * expr ) override {
                Visitor::preVisit(expr);
                if ( expr==curStmt ) {
                    AliasChain ch;
                    if ( aliasChainOf(expr->left, ch) ) storeBaseVars.insert(ch.baseVar);
                }
            }
            virtual void preVisitLet ( ExprLet * let, const VariablePtr & var, bool last ) override {
                Visitor::preVisitLet(let, var, last);
                if ( var->type && !var->type->ref ) {
                    allLocals.push_back(var);
                    declAt[var] = curAnchor();
                    if ( !var->type->isRefType() ) locals.push_back(var);
                }
            }
            virtual void preVisit ( ExprVar * expr ) override {
                Visitor::preVisit(expr);
                bool readOnly = expr->r2v || (expr->r2cr && !expr->write);
                if ( !readOnly && storeBaseVars.find(expr)==storeBaseVars.end() ) {
                    disq.insert(expr->variable);
                }
            }
            virtual void preVisit ( ExprIfThenElse * expr ) override {
                Visitor::preVisit(expr);
                anchorOf[expr->cond] = curAnchor();
            }
            virtual void preVisit ( ExprWhile * expr ) override {
                Visitor::preVisit(expr);
                anchorOf[expr->cond] = curAnchor();
            }
            virtual void preVisit ( ExprFor * expr ) override {
                Visitor::preVisit(expr);
                for ( auto & src : expr->sources ) anchorOf[src] = curAnchor();
            }
            virtual void preVisit ( ExprWith * expr ) override {
                Visitor::preVisit(expr);
                if ( expr->with ) anchorOf[expr->with] = curAnchor();
            }
        };

        struct CseUniverse {
            das_hash_set<Variable *> valueLeaves;   // v1 discipline: every occurrence read-only or visible store
            das_hash_set<Variable *> memBases;      // chain bases: non-ref locals and arguments
            das_hash_set<Variable *> argVars;       // function arguments (may alias each other and globals)
        };

        // a side-effect-free call returning a pointer/handle may still be a FACTORY
        // (clone_type & co return a fresh node each evaluation): results are
        // value-equal but not identity-equal, and reuse would alias something the
        // caller treats as uniquely owned. Builtin pointer operators (p + i) are
        // arithmetic, not allocation, and stay eligible.
        bool cseIdentityProducing ( Expression * e, Function * func ) {
            if ( !e->type ) return true;
            if ( !e->type->isPointer() && e->type->baseType!=Type::tHandle ) return false;
            if ( e->rtti_isCall() ) return true;
            return func && !func->builtIn;
        }

        struct CseTreeInfo {
            vector<Variable *>  mentions;           // leaf variables and chain bases the tree reads
            bool                readsMemory = false;
            bool                argRooted = false;
        };

        bool cseAllowedTree ( Expression * e, const CseUniverse & U, CseTreeInfo & info );

        // a read chain over a tracked base is a MEMORY leaf: its value is stable until
        // a store through the base or an opaque statement (both kill the pair)
        bool cseChainLeaf ( Expression * e, const CseUniverse & U, CseTreeInfo & info ) {
            AliasChain ch;
            if ( !aliasChainOf(e, ch) ) return false;
            if ( ch.anyWrite ) return false;
            if ( U.memBases.find(ch.base)==U.memBases.end() ) return false;
            for ( auto idx : ch.indices ) {
                if ( !cseAllowedTree(idx, U, info) ) return false;
            }
            info.mentions.push_back(ch.base);
            info.readsMemory = true;
            if ( U.argVars.find(ch.base)!=U.argVars.end() ) info.argRooted = true;
            return true;
        }

        // whitelisted pure value tree over clean leaves, read chains, and constants;
        // collects the variables the tree mentions
        bool cseAllowedTree ( Expression * e, const CseUniverse & U, CseTreeInfo & info ) {
            if ( !e ) return false;
            if ( e->rtti_isConstant() ) return true;
            if ( e->rtti_isVar() ) {
                auto v = static_cast<ExprVar *>(e);
                if ( v->write || !v->variable ) return false;
                if ( U.valueLeaves.find(v->variable)==U.valueLeaves.end() ) return false;
                info.mentions.push_back(v->variable);
                return true;
            }
            if ( e->rtti_isR2V() ) {
                return cseAllowedTree(static_cast<ExprRef2Value *>(e)->subexpr, U, info);
            }
            if ( e->rtti_isCopy() || e->rtti_isClone() || e->rtti_isSequence() ) return false;
            if ( e->rtti_isField() || e->rtti_isAt() ) {
                return cseChainLeaf(e, U, info);
            }
            if ( e->rtti_isOp3() ) {
                auto op = static_cast<ExprOp3 *>(e);
                if ( cseIdentityProducing(e, op->func) ) return false;
                return cseAllowedTree(op->subexpr, U, info)
                    && cseAllowedTree(op->left, U, info)
                    && cseAllowedTree(op->right, U, info);
            }
            if ( e->rtti_isOp2() ) {    // ExprMove's write-flagged left is caught by the ExprVar rule
                auto op = static_cast<ExprOp2 *>(e);
                if ( cseIdentityProducing(e, op->func) ) return false;
                return cseAllowedTree(op->left, U, info)
                    && cseAllowedTree(op->right, U, info);
            }
            if ( e->rtti_isOp1() ) {
                auto op = static_cast<ExprOp1 *>(e);
                if ( cseIdentityProducing(e, op->func) ) return false;
                return cseAllowedTree(op->subexpr, U, info);
            }
            if ( e->rtti_isCall() ) {
                auto c = static_cast<ExprCall *>(e);
                if ( !c->func ) return false;
                if ( cseIdentityProducing(e, c->func) ) return false;
                for ( auto & arg : c->arguments ) {
                    if ( cseAllowedTree(arg, U, info) ) continue;
                    // a read chain passed by reference to a pure callee (length(s.arr))
                    if ( cseChainLeaf(arg, U, info) ) continue;
                    return false;
                }
                return true;
            }
            if ( e->rtti_isCast() ) {
                return cseAllowedTree(static_cast<ExprCast *>(e)->subexpr, U, info);
            }
            if ( e->rtti_isSwizzle() ) {
                auto s = static_cast<ExprSwizzle *>(e);
                if ( s->write ) return false;
                if ( cseAllowedTree(s->value, U, info) ) return true;   // swizzle over a value tree
                return cseChainLeaf(e, U, info);                        // swizzle link over a chain
            }
            return false;   // unrecognized class - conservatively out
        }

        // candidate root: a real operation yielding a by-value result, side-effect free
        bool cseCandidateRoot ( Expression * e ) {
            if ( !e || !e->type ) return false;
            if ( !e->noSideEffects ) return false;
            if ( !(e->rtti_isOp1() || e->rtti_isOp2() || e->rtti_isOp3()
                || e->rtti_isCall() || e->rtti_isCast() || e->rtti_isSwizzle()
                || e->rtti_isField() || e->rtti_isAt()) ) return false;
            if ( e->rtti_isCopy() || e->rtti_isClone() || e->rtti_isSequence() ) return false;
            if ( e->type->ref || e->type->isRefType() ) return false;
            if ( e->type->baseType==Type::tVoid ) return false;
            return true;
        }

        // bucketing companion to Expression::sameAs - collisions are fine (verified
        // by sameAs), a hash mismatch for sameAs-equal trees is not
        uint64_t cseHash ( Expression * e ) {
            if ( !e ) return 0;
            uint64_t h = hash_blockz64((const uint8_t *) e->__rtti);
            auto mix = [&]( uint64_t v ) { h = (h ^ v) * 1099511628211ul; };
            if ( e->rtti_isConstant() ) {
                auto c = static_cast<ExprConst *>(e);
                mix(uint64_t(c->baseType));
                if ( e->rtti_isStringConstant() ) {
                    mix(hash_blockz64((const uint8_t *) static_cast<ExprConstString *>(e)->text.c_str()));
                } else {
                    mix(hash_block64((const uint8_t *) &c->value, sizeof(vec4f)));
                }
            } else if ( e->rtti_isVar() ) {
                mix(uint64_t(uintptr_t(static_cast<ExprVar *>(e)->variable)));
            } else if ( e->rtti_isR2V() ) {
                mix(cseHash(static_cast<ExprRef2Value *>(e)->subexpr));
            } else if ( e->rtti_isField() ) {
                auto f = static_cast<ExprField *>(e);
                mix(hash_blockz64((const uint8_t *) f->name.c_str()));
                mix(cseHash(f->value));
            } else if ( e->rtti_isAt() ) {
                auto a = static_cast<ExprAt *>(e);
                mix(cseHash(a->subexpr));
                mix(cseHash(a->index));
            } else if ( e->rtti_isOp3() ) {
                auto op = static_cast<ExprOp3 *>(e);
                mix(hash_blockz64((const uint8_t *) op->op.c_str()));
                mix(uint64_t(uintptr_t(op->func)));
                mix(cseHash(op->subexpr));
                mix(cseHash(op->left));
                mix(cseHash(op->right));
            } else if ( e->rtti_isOp2() ) {
                auto op = static_cast<ExprOp2 *>(e);
                mix(hash_blockz64((const uint8_t *) op->op.c_str()));
                mix(uint64_t(uintptr_t(op->func)));
                mix(cseHash(op->left));
                mix(cseHash(op->right));
            } else if ( e->rtti_isOp1() ) {
                auto op = static_cast<ExprOp1 *>(e);
                mix(hash_blockz64((const uint8_t *) op->op.c_str()));
                mix(uint64_t(uintptr_t(op->func)));
                mix(cseHash(op->subexpr));
            } else if ( e->rtti_isCall() ) {
                auto c = static_cast<ExprCall *>(e);
                mix(uint64_t(uintptr_t(c->func)));
                for ( auto & arg : c->arguments ) mix(cseHash(arg));
            } else if ( e->rtti_isCast() ) {
                auto c = static_cast<ExprCast *>(e);
                if ( c->castType ) mix(uint64_t(c->castType->baseType));
                mix(cseHash(c->subexpr));
            } else if ( e->rtti_isSwizzle() ) {
                auto s = static_cast<ExprSwizzle *>(e);
                if ( !s->fields.empty() ) mix(hash_block64((const uint8_t *) s->fields.data(), s->fields.size()));
                mix(cseHash(s->value));
            }
            return h;
        }

        struct CsePair {
            Expression *        expr = nullptr;    // canonical E
            Variable *          var = nullptr;     // clean local holding E's value
            uint64_t            hash = 0;
            vector<Variable *>  mentions;           // leaf variables and chain bases E reads
            bool                readsMemory = false;
            bool                argRooted = false;
        };

        struct CseStmtInfo {
            vector<int>         gen;                // pairs this statement (re)establishes
            vector<Variable *>  declVars;           // let-declared here (kill, before gen)
            vector<Variable *>  storeVars;          // visibly stored-through bases in subtree (kill, before gen)
            bool                opaque = false;     // may write memory invisibly: kill every memory pair
            bool                killArgs = false;   // store through a global/argument base: kill argument-rooted pairs
        };

        // per-statement effect collection, including make-block interiors (a block
        // argument body runs - zero or more times - during the statement that owns the
        // literal, and invoke-carrying calls at other statements are opaque anyway)
        class CseEffects : public Visitor {
        public:
            CseEffects ( const CseUniverse & u ) : U(u) {}
            vector<Variable *> kills;
            bool opaque = false;
            bool killArgs = false;
        protected:
            const CseUniverse & U;
            das_hash_set<Expression *> spine;   // chain links already classified from their top
            void killBase ( Variable * v ) {
                kills.push_back(v);
                if ( U.argVars.find(v)!=U.argVars.end() ) killArgs = true;
            }
            bool tracked ( Variable * v ) const {
                return U.memBases.find(v)!=U.memBases.end()
                    || U.valueLeaves.find(v)!=U.valueLeaves.end();
            }
            // a store target: visible store through a tracked base is a kill; a loop
            // variable forwards to its source's base; a global (which an argument may
            // alias, but a local cannot) kills argument-rooted pairs; anything else -
            // pointer paths, block arguments - makes the statement opaque
            void classifyWrite ( Expression * e ) {
                AliasChain ch;
                if ( !aliasChainOf(e, ch, &spine) ) { opaque = true; return; }
                if ( tracked(ch.base) ) {
                    killBase(ch.base);
                } else if ( ch.base->loop_source ) {
                    classifyWrite(ch.base->loop_source);
                } else if ( ch.baseVar->isGlobalVariable() ) {
                    killArgs = true;
                } else {
                    opaque = true;
                }
            }
            void writeRoot ( Expression * expr, bool w ) {
                if ( !w ) return;
                if ( spine.find(expr)!=spine.end() ) return;
                classifyWrite(expr);
            }
            virtual bool canVisitQuoteSubexpression ( ExprQuote * ) override { return false; }
            virtual void preVisit ( ExprCopy * expr ) override {
                Visitor::preVisit(expr);
                classifyWrite(expr->left);
            }
            virtual void preVisit ( ExprMove * expr ) override {
                Visitor::preVisit(expr);
                classifyWrite(expr->left);  // the zeroed source arrives via its write flags
            }
            virtual void preVisit ( ExprClone * expr ) override {
                Visitor::preVisit(expr);    // deep clones lower to calls during infer; a surviving
                classifyWrite(expr->left);  // ExprClone is a flat builtin copy
            }
            virtual void preVisit ( ExprDelete * expr ) override {
                Visitor::preVisit(expr);    // finalize zeroes the target (write flags may not cover it)
                classifyWrite(expr->subexpr);
            }
            virtual void preVisit ( ExprRef2Ptr * expr ) override {
                Visitor::preVisit(expr);
                // taking the address writes nothing by itself; later writes through the
                // escaped pointer arrive as pointer-path stores or impure calls (opaque)
                AliasChain ch;
                if ( aliasChainOf(expr->subexpr, ch, &spine) ) {
                    if ( tracked(ch.base) ) killBase(ch.base);
                }
            }
            virtual void preVisit ( ExprVar * expr ) override {
                Visitor::preVisit(expr);
                writeRoot(expr, expr->write);
            }
            virtual void preVisit ( ExprField * expr ) override {
                Visitor::preVisit(expr);
                writeRoot(expr, expr->write);
            }
            virtual void preVisit ( ExprSwizzle * expr ) override {
                Visitor::preVisit(expr);
                writeRoot(expr, expr->write);
            }
            virtual void preVisit ( ExprAt * expr ) override {
                Visitor::preVisit(expr);
                writeRoot(expr, expr->write);
            }
            virtual void preVisitExpression ( Expression * expr ) override {
                Visitor::preVisitExpression(expr);
                if ( !expr->rtti_isCallLikeExpr() || expr->noSideEffects ) return;
                if ( expr->rtti_isCopy() || expr->rtti_isClone() || cseIsMove(expr) ) return;  // classified above
                if ( expr->rtti_isCallFunc() ) {
                    auto func = static_cast<ExprCallFunc *>(expr)->func;
                    if ( func && !(func->sideEffectFlags & CSE_OPAQUE_CALL_EFFECTS) ) return;
                }
                opaque = true;  // invoke, unresolved, or a callee that may write unseen memory
            }
        };

        // per-statement occurrence matcher: top-down, skips make-block interiors
        // (their execution count is unknown) and the inside of matched subtrees
        class CseMatch : public Visitor {
        public:
            CseMatch ( const vector<CsePair> & ps, const das_hash_map<uint64_t, vector<int>> & bh,
                       const CsePrescan & sc, das_hash_map<Expression *, Variable *> & pl )
                : pairs(ps), byHash(bh), scan(sc), plan(pl) {}
            void setStatement ( const vector<uint64_t> & av, const CseStmtInfo * si, const CseAnchor & anch ) {
                avail = &av;
                anchored = anch.block!=nullptr;
                anchor = anch;
                touched.clear();
                stmtOpaque = false;
                stmtKillArgs = false;
                if ( si ) {
                    for ( auto v : si->declVars ) touched.insert(v);
                    for ( auto v : si->storeVars ) touched.insert(v);
                    stmtOpaque = si->opaque;
                    stmtKillArgs = si->killArgs;
                }
                suppress = 0;
            }
        protected:
            const vector<CsePair> & pairs;
            const das_hash_map<uint64_t, vector<int>> & byHash;
            const CsePrescan & scan;
            das_hash_map<Expression *, Variable *> & plan;
            const vector<uint64_t> * avail = nullptr;
            das_hash_set<Variable *> touched;   // vars this statement declares or stores to
            CseAnchor anchor;
            bool anchored = false;
            bool stmtOpaque = false;
            bool stmtKillArgs = false;
            int suppress = 0;
            bool scopeValid ( Variable * v ) const {
                auto dit = scan.declAt.find(v);
                if ( dit==scan.declAt.end() ) return false;
                CseAnchor a = anchor;
                while ( a.block ) {
                    if ( a.block==dit->second.block ) return a.index > dit->second.index;
                    auto pit = scan.blockParent.find(a.block);
                    if ( pit==scan.blockParent.end() ) return false;
                    a = pit->second;
                }
                return false;
            }
            bool match ( Expression * expr, Variable *& out ) const {
                if ( !anchored || !avail ) return false;
                if ( !cseCandidateRoot(expr) ) return false;
                auto bit = byHash.find(cseHash(expr));
                if ( bit==byHash.end() ) return false;
                for ( int pi : bit->second ) {
                    if ( !(((*avail)[pi>>6] >> (pi&63)) & 1) ) continue;
                    auto & P = pairs[pi];
                    // no reuse across a statement that writes P's inputs or holder -
                    // intra-statement evaluation order is not modeled
                    if ( P.readsMemory && stmtOpaque ) continue;
                    if ( P.argRooted && stmtKillArgs ) continue;
                    if ( touched.find(P.var)!=touched.end() ) continue;
                    bool touchedMention = false;
                    for ( auto m : P.mentions ) {
                        if ( touched.find(m)!=touched.end() ) { touchedMention = true; break; }
                    }
                    if ( touchedMention ) continue;
                    if ( !scopeValid(P.var) ) continue;
                    if ( !P.expr->sameAs(expr) ) continue;
                    out = P.var;
                    return true;
                }
                return false;
            }
            virtual bool canVisitQuoteSubexpression ( ExprQuote * ) override { return false; }
            virtual bool canVisitMakeBlockBody ( ExprMakeBlock * ) override { return false; }
            virtual void preVisitExpression ( Expression * expr ) override {
                Visitor::preVisitExpression(expr);
                if ( suppress ) { suppress++; return; }
                Variable * v = nullptr;
                if ( match(expr, v) ) {
                    plan[expr] = v;
                    suppress = 1;
                }
            }
            virtual ExpressionPtr visitExpression ( Expression * expr ) override {
                if ( suppress ) suppress--;
                return Visitor::visitExpression(expr);
            }
        };

        void analyzeCse ( Function * fn, das_hash_map<Expression *, Variable *> & plan ) {
            CsePrescan scan;
            fn->body->visit(scan);
            if ( !scan.eligible ) return;
            CseUniverse U;
            // value leaves: clean by-value locals and arguments
            for ( auto v : scan.locals ) {
                if ( scan.disq.find(v)==scan.disq.end() ) U.valueLeaves.insert(v);
            }
            for ( auto & arg : fn->arguments ) {
                U.argVars.insert(arg);
                if ( arg->type && !arg->type->ref && !arg->type->isRefType()
                    && scan.disq.find(arg)==scan.disq.end() ) U.valueLeaves.insert(arg);
            }
            // chain bases: every non-ref local and argument
            for ( auto v : scan.allLocals ) U.memBases.insert(v);
            for ( auto & arg : fn->arguments ) {
                if ( arg->type && !arg->type->ref ) U.memBases.insert(arg);
            }
            if ( U.valueLeaves.empty() && U.memBases.empty() ) return;
            Cfg cfg = buildCfg(fn);
            // pair and per-statement gen/kill collection
            vector<CsePair> pairs;
            das_hash_map<uint64_t, vector<int>> byHash;
            das_hash_map<Expression *, CseStmtInfo> stmtInfo;
            das_hash_set<Variable *> stmtKills;
            auto addPair = [&]( Expression * stmt, CseStmtInfo & SI, Expression * E, Variable * v ) {
                if ( !E || !cseCandidateRoot(E) ) return;
                CseTreeInfo info;
                if ( !cseAllowedTree(E, U, info) ) return;
                if ( SI.opaque && info.readsMemory ) return;
                if ( SI.killArgs && info.argRooted ) return;
                for ( auto m : info.mentions ) {
                    if ( m==v ) return;     // t = f(t): stale the moment the store lands
                    if ( stmtKills.find(m)!=stmtKills.end() ) return;   // input stored this statement
                }
                uint64_t h = cseHash(E);
                auto & bucket = byHash[h];
                for ( int pi : bucket ) {
                    if ( pairs[pi].var==v && pairs[pi].expr->sameAs(E) ) {
                        SI.gen.push_back(pi);
                        return;
                    }
                }
                int idx = int(pairs.size());
                pairs.push_back(CsePair{E, v, h, info.mentions, info.readsMemory, info.argRooted});
                bucket.push_back(idx);
                SI.gen.push_back(idx);
            };
            for ( auto b : cfg.blocks ) {
                for ( auto stmt : b->stmts ) {
                    auto & SI = stmtInfo[stmt];
                    CseEffects fx(U);
                    stmt->visit(fx);
                    SI.storeVars = fx.kills;
                    SI.opaque = fx.opaque;
                    SI.killArgs = fx.killArgs;
                    stmtKills.clear();
                    for ( auto v : SI.storeVars ) stmtKills.insert(v);
                    if ( stmt->rtti_isLet() ) {
                        auto let = static_cast<ExprLet *>(stmt);
                        for ( auto & pv : let->variables ) {
                            SI.declVars.push_back(pv);
                            if ( U.valueLeaves.find(pv)==U.valueLeaves.end() ) continue;
                            if ( pv->init_via_move || pv->init_via_clone ) continue;
                            addPair(stmt, SI, pv->init, pv);
                        }
                    } else if ( stmt->rtti_isCopy() ) {
                        auto cp = static_cast<ExprCopy *>(stmt);
                        if ( cp->left->rtti_isVar() ) {
                            auto v = static_cast<ExprVar *>(cp->left)->variable;
                            if ( U.valueLeaves.find(v)!=U.valueLeaves.end()
                                && scan.declAt.find(v)!=scan.declAt.end() ) {
                                addPair(stmt, SI, cp->right, v);
                            }
                        }
                    }
                }
            }
            if ( pairs.empty() ) return;
            das_hash_map<Variable *, vector<int>> pairsTouchingVar;
            for ( int i=0, is=int(pairs.size()); i!=is; ++i ) {
                pairsTouchingVar[pairs[i].var].push_back(i);
                for ( auto m : pairs[i].mentions ) pairsTouchingVar[m].push_back(i);
            }
            size_t np = pairs.size();
            size_t nw = (np + 63) / 64;
            size_t nb = cfg.blocks.size();
            vector<uint64_t> memMask(nw, 0), argMask(nw, 0);
            for ( int i=0, is=int(pairs.size()); i!=is; ++i ) {
                if ( pairs[i].readsMemory ) memMask[i>>6] |= 1ull<<(i&63);
                if ( pairs[i].argRooted ) argMask[i>>6] |= 1ull<<(i&63);
            }
            auto killVar = [&]( Variable * v, vector<uint64_t> & avail ) {
                auto it = pairsTouchingVar.find(v);
                if ( it==pairsTouchingVar.end() ) return;
                for ( int pi : it->second ) avail[pi>>6] &= ~(1ull<<(pi&63));
            };
            // kills first, then gen: a store's rhs was computed before the store
            // landed, and pairs whose inputs die in their own statement were
            // rejected at creation
            auto applyStmt = [&]( Expression * stmt, vector<uint64_t> & avail ) {
                auto it = stmtInfo.find(stmt);
                if ( it==stmtInfo.end() ) return;
                auto & SI = it->second;
                if ( SI.opaque ) for ( size_t w=0; w!=nw; ++w ) avail[w] &= ~memMask[w];
                if ( SI.killArgs ) for ( size_t w=0; w!=nw; ++w ) avail[w] &= ~argMask[w];
                for ( auto v : SI.declVars ) killVar(v, avail);
                for ( auto v : SI.storeVars ) killVar(v, avail);
                for ( int g : SI.gen ) avail[g>>6] |= 1ull<<(g&63);
            };
            // forward availability to fixpoint: IN = intersection over predecessors,
            // non-entry blocks start at the universe set
            vector<vector<uint64_t>> IN(nb), OUT(nb);
            for ( size_t i=0; i!=nb; ++i ) {
                IN[i].assign(nw, ~0ull);
                OUT[i].assign(nw, ~0ull);
            }
            IN[cfg.entry->id].assign(nw, 0);
            vector<uint64_t> cur(nw);
            bool changed = true;
            while ( changed ) {
                changed = false;
                for ( auto b : cfg.blocks ) {
                    if ( b!=cfg.entry && !b->pred.empty() ) {
                        for ( size_t w=0; w!=nw; ++w ) cur[w] = ~0ull;
                        for ( auto p : b->pred ) {
                            auto & pout = OUT[p->id];
                            for ( size_t w=0; w!=nw; ++w ) cur[w] &= pout[w];
                        }
                        if ( cur!=IN[b->id] ) { IN[b->id] = cur; changed = true; }
                    }
                    cur = IN[b->id];
                    for ( auto stmt : b->stmts ) applyStmt(stmt, cur);
                    if ( cur!=OUT[b->id] ) { OUT[b->id] = cur; changed = true; }
                }
            }
            // final scan: mark rewritable occurrences
            CseMatch matcher(pairs, byHash, scan, plan);
            for ( auto b : cfg.blocks ) {
                cur = IN[b->id];
                for ( auto stmt : b->stmts ) {
                    auto sit = stmtInfo.find(stmt);
                    auto ait = scan.anchorOf.find(stmt);
                    matcher.setStatement(cur, sit!=stmtInfo.end() ? &sit->second : nullptr,
                        ait!=scan.anchorOf.end() ? ait->second : CseAnchor());
                    stmt->visit(matcher);
                    applyStmt(stmt, cur);
                }
            }
        }

        class EliminateCse : public PassVisitor {
        public:
            using PassVisitor::PassVisitor;
            using PassVisitor::visit;
        protected:
            das_hash_map<Expression *, Variable *> plan;
            virtual bool canVisitFunction ( Function * fun ) override {
                if ( fun->stub || fun->isTemplate || !funcIsDirty(fun) ) return false;
                if ( !fun->body || !fun->body->rtti_isBlock() ) return false;
                plan.clear();
                analyzeCse(fun, plan);
                return !plan.empty();
            }
            virtual bool canVisitStructure ( Structure * ) override { return false; }
            virtual bool canVisitStructureFieldInit ( Structure * ) override { return false; }
            virtual bool canVisitArgumentInit ( Function *, const VariablePtr &, Expression * ) override { return false; }
            virtual bool canVisitQuoteSubexpression ( ExprQuote * ) override { return false; }
            virtual bool canVisitGlobalVariable ( Variable * ) override { return false; }
            virtual bool canVisitEnumeration ( Enumeration * ) override { return false; }
            virtual ExpressionPtr visitExpression ( Expression * expr ) override {
                auto it = plan.find(expr);
                if ( it==plan.end() ) return Visitor::visitExpression(expr);
                auto var = it->second;
                auto evar = new ExprVar(expr->at, var->name);
                evar->variable = var;
                evar->local = true;
                evar->r2v = true;
                evar->type = new TypeDecl(*var->type);
                evar->type->ref = false;
                reportFolding();
                return evar;
            }
        };
    }

    // program

    bool Program::optimizationCSE(int32_t round) {
        if ( options.getBoolOption("disable_cse", policies.disable_cse) ) return false;
        checkSideEffects();
        EliminateCse pass(round);
        visit(pass);
        return pass.didAnything();
    }
}
