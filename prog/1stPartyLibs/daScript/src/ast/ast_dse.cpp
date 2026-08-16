#include "daScript/misc/platform.h"

#include "daScript/ast/ast.h"
#include "daScript/ast/ast_visitor.h"
#include "daScript/ast/ast_expressions.h"
#include "daScript/ast/ast_cfg.h"

#include "ast_alias.h"

namespace das {

    // ===== Dead-store elimination =====
    // Removes a statement-level `x = <rhs>` / `x.field.path = <rhs>` when the target is
    // a chain (ast_alias.h) over a plain ExprLet local, the rhs is pure, and no path
    // from the store reaches a read overlapping the chain before the next full
    // overwrite. A dead store with an IMPURE rhs keeps the rhs as a bare statement
    // (the store dies, the call stays). Also strips a dead pure declaration init
    // (`var x = <rhs>` where every path overwrites x before reading it) down to the
    // plain zero-init declaration. Liveness runs backward over the statement-level
    // CFG (ast_cfg.h), one bit per stored chain.
    //
    // Soundness model (deliberately narrow):
    //  * bases are ExprLet locals (never arguments - argument writes are the caller's
    //    to observe). Every occurrence of a base must be a value read (r2v), a
    //    read-through (r2cr without write - const-ref passes, read chains), on the
    //    spine of a statement-level ExprCopy store, or on the left spine of a
    //    statement-level op-assign / increment (read-modify-write: uses its chains,
    //    kills nothing, never a removal candidate); anything else (addr(), mutable-ref
    //    pass, move/clone, delete) disqualifies the variable. A local whose
    //    every use is classified cannot escape, so no call can read it behind the
    //    pass's back;
    //  * removal candidates are stores through EXACT chains (fields only, no index or
    //    swizzle links) whose target type is by-value - such an ExprCopy is a plain
    //    value write with no copy hooks. A store through an inexact chain
    //    (s.arr[i] = v) kills nothing and reads its exact prefix; a store to a chain
    //    keeps every strict-prefix store alive (partial overwrite observes the rest);
    //  * chain identity is the field-name path; two chains overlap when one path is a
    //    prefix of the other (an inexact link truncates the known path, widening the
    //    overlap conservatively);
    //  * a store inside a make-block body is never a removal candidate or a kill - any
    //    occurrence inside the owning statement makes the overlapped chains live there
    //    (the body runs zero or more times during that statement, and blocks cannot
    //    escape it);
    //  * functions carrying control flow the CFG does not model are skipped whole:
    //    goto/label (also covers lowered generators), try/recover, and non-empty
    //    finally (it also runs on an early return the CFG routes straight to exit).
    // Local stores are unobservable after a panic (locals die with the frame; finally
    // is skipped on panic by design), so removal needs no panic-path modeling.

    namespace {

        bool isOpAssign ( const string & op ) {
            return op=="+=" || op=="-=" || op=="*=" || op=="/=" || op=="%="
                || op=="&=" || op=="|=" || op=="^=" || op=="||=" || op=="&&=" || op=="^^="
                || op=="<<=" || op==">>=" || op=="<<<=" || op==">>>=";
        }

        // one walk: (a) bail on constructs the CFG does not model, (b) collect locals,
        // (c) disqualify any variable with an occurrence that is neither a read nor on
        // the spine of a statement-level store / op-assign
        class DsePrescan : public Visitor {
        public:
            bool eligible = true;
            vector<Variable *> locals;                  // every non-ref ExprLet local, in order
            das_hash_set<Variable *> disq;              // variables with an unclassified occurrence
            das_hash_set<Expression *> storeSpines;     // nodes on statement-level store-left / op-assign-left chains
        protected:
            Expression * curStmt = nullptr;
            virtual bool canVisitQuoteSubexpression ( ExprQuote * ) override { return false; }
            virtual void preVisit ( ExprBlock * blk ) override {
                Visitor::preVisit(blk);
                if ( !blk->finalList.empty() ) eligible = false;
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
            virtual void preVisitBlockExpression ( ExprBlock * block, Expression * expr ) override {
                Visitor::preVisitBlockExpression(block, expr);
                curStmt = expr;
            }
            virtual void preVisit ( ExprCopy * expr ) override {
                Visitor::preVisit(expr);
                if ( expr==curStmt ) {
                    AliasChain ch;
                    aliasChainOf(expr->left, ch, &storeSpines);
                }
            }
            virtual void preVisit ( ExprOp2 * expr ) override {
                Visitor::preVisit(expr);
                if ( expr==curStmt && isOpAssign(expr->op) ) {
                    AliasChain ch;
                    aliasChainOf(expr->left, ch, &storeSpines);
                }
            }
            virtual void preVisit ( ExprOp1 * expr ) override {
                Visitor::preVisit(expr);
                if ( expr==curStmt && (expr->op=="++" || expr->op=="--"
                    || expr->op=="+++" || expr->op=="---") ) {
                    AliasChain ch;
                    aliasChainOf(expr->subexpr, ch, &storeSpines);
                }
            }
            virtual void preVisitLet ( ExprLet * let, const VariablePtr & var, bool last ) override {
                Visitor::preVisitLet(let, var, last);
                if ( var->type && !var->type->ref ) {
                    locals.push_back(var);
                }
            }
            virtual void preVisit ( ExprVar * expr ) override {
                Visitor::preVisit(expr);
                bool readOnly = expr->r2v || (expr->r2cr && !expr->write);
                if ( !readOnly && storeSpines.find(expr)==storeSpines.end() ) {
                    disq.insert(expr->variable);
                }
            }
        };

        struct DseChain {
            Variable *              base = nullptr;
            vector<const char *>    path;               // exact field path (candidates only)
        };

        // resolve chain occurrences in a subtree to candidate uses: every maximal chain
        // rooted at a tracked base - reads and interior-block stores alike - touches
        // the candidates its known prefix overlaps
        class DseReads : public Visitor {
        public:
            DseReads ( const das_hash_map<Variable *, vector<int>> & bb, const vector<DseChain> & cs,
                       vector<int> & out, const das_hash_set<Expression *> * skip )
                : byBase(bb), chains(cs), uses(out), skipSpine(skip) {}
        protected:
            const das_hash_map<Variable *, vector<int>> & byBase;
            const vector<DseChain> & chains;
            vector<int> & uses;
            const das_hash_set<Expression *> * skipSpine;
            das_hash_set<Expression *> seen;
            void chainRoot ( Expression * expr ) {
                if ( seen.find(expr)!=seen.end() ) return;
                if ( skipSpine && skipSpine->find(expr)!=skipSpine->end() ) return;
                AliasChain ch;
                if ( !aliasChainOf(expr, ch, &seen) ) return;
                auto bit = byBase.find(ch.base);
                if ( bit==byBase.end() ) return;
                size_t rlen = aliasChainExactLen(ch.path);
                for ( int ci : bit->second ) {
                    auto & C = chains[ci];
                    size_t n = das::min<size_t>(rlen, C.path.size());
                    if ( aliasPathAgree(ch.path, C.path, n) ) uses.push_back(ci);
                }
            }
            virtual bool canVisitQuoteSubexpression ( ExprQuote * ) override { return false; }
            virtual void preVisit ( ExprVar * expr ) override     { Visitor::preVisit(expr); chainRoot(expr); }
            virtual void preVisit ( ExprField * expr ) override   { Visitor::preVisit(expr); chainRoot(expr); }
            virtual void preVisit ( ExprSwizzle * expr ) override { Visitor::preVisit(expr); chainRoot(expr); }
            virtual void preVisit ( ExprAt * expr ) override      { Visitor::preVisit(expr); chainRoot(expr); }
        };

        struct DseLetInit {
            int         cand = -1;
            Variable *  var = nullptr;
        };

        struct DseStmt {
            vector<int> kills;          // candidates fully overwritten by this statement
            vector<int> uses;           // candidates whose chains this statement touches
            int         removeIdx = -1; // candidate this store establishes (removable when dead)
            bool        rhsPure = false;
            vector<DseLetInit> letInits;    // strippable declaration inits (removable when dead)
        };

        void analyzeDeadStores ( Function * fn, das_hash_set<Expression *> & dead,
                                 das_hash_set<Expression *> & deadKeepRhs,
                                 das_hash_set<Variable *> & deadInits ) {
            DsePrescan scan;
            fn->body->visit(scan);
            if ( !scan.eligible ) return;
            das_hash_set<Variable *> bases;
            for ( auto v : scan.locals ) {
                if ( scan.disq.find(v)==scan.disq.end() ) bases.insert(v);
            }
            if ( bases.empty() ) return;
            Cfg cfg = buildCfg(fn);
            // candidate enumeration: exact by-value chain stores, then strippable
            // declaration inits of bases that carry at least one store
            vector<DseChain> chains;
            das_hash_map<Variable *, vector<int>> byBase;
            auto findChain = [&]( Variable * base, const vector<const char *> & path ) -> int {
                for ( int ci : byBase[base] ) {
                    if ( chains[ci].path.size()==path.size()
                        && aliasPathAgree(chains[ci].path, path, path.size()) ) return ci;
                }
                return -1;
            };
            auto storeTarget = [&]( Expression * stmt, AliasChain & ch, das_hash_set<Expression *> * spine ) -> bool {
                if ( !stmt->rtti_isCopy() ) return false;
                auto cp = static_cast<ExprCopy *>(stmt);
                if ( !aliasChainOf(cp->left, ch, spine) ) return false;
                return bases.find(ch.base)!=bases.end();
            };
            for ( auto b : cfg.blocks ) {
                for ( auto stmt : b->stmts ) {
                    AliasChain ch;
                    if ( !storeTarget(stmt, ch, nullptr) ) continue;
                    if ( !ch.exact ) continue;
                    auto cp = static_cast<ExprCopy *>(stmt);
                    if ( !cp->left->type || cp->left->type->isRefType() ) continue;
                    if ( findChain(ch.base, ch.path)>=0 ) continue;
                    int idx = int(chains.size());
                    chains.push_back(DseChain{ch.base, ch.path});
                    byBase[ch.base].push_back(idx);
                }
            }
            if ( chains.empty() ) return;
            vector<const char *> emptyPath;
            for ( auto v : scan.locals ) {
                if ( byBase.find(v)==byBase.end() ) continue;   // no stores: unused-var territory
                if ( !v->init || v->init_via_move || v->init_via_clone ) continue;
                if ( v->type->isRefType() ) continue;
                if ( !v->init->noSideEffects ) continue;
                if ( findChain(v, emptyPath)>=0 ) continue;
                int idx = int(chains.size());
                chains.push_back(DseChain{v, emptyPath});
                byBase[v].push_back(idx);
            }
            // per-statement gen/kill resolution
            size_t nw = (chains.size() + 63) / 64;
            struct BlockInfo {
                vector<DseStmt>  stmts;
                vector<uint64_t> liveIn, liveOut;
            };
            vector<BlockInfo> binfo(cfg.blocks.size());
            for ( size_t bi=0, bis=cfg.blocks.size(); bi!=bis; ++bi ) {
                auto b = cfg.blocks[bi];
                auto & BI = binfo[bi];
                BI.liveIn.resize(nw, 0);
                BI.liveOut.resize(nw, 0);
                BI.stmts.resize(b->stmts.size());
                for ( size_t si=0, sis=b->stmts.size(); si!=sis; ++si ) {
                    auto stmt = b->stmts[si];
                    auto & SI = BI.stmts[si];
                    das_hash_set<Expression *> spine;
                    AliasChain ch;
                    if ( storeTarget(stmt, ch, &spine) ) {
                        size_t known = aliasChainExactLen(ch.path);
                        for ( int ci : byBase[ch.base] ) {
                            auto & C = chains[ci];
                            if ( ch.exact ) {
                                // full overwrite of every extension; a strict-prefix
                                // store stays observable through the untouched rest
                                if ( C.path.size()>=ch.path.size()
                                    && aliasPathAgree(C.path, ch.path, ch.path.size()) ) {
                                    SI.kills.push_back(ci);
                                    if ( C.path.size()==ch.path.size() ) SI.removeIdx = ci;
                                } else if ( C.path.size()<ch.path.size()
                                    && aliasPathAgree(C.path, ch.path, C.path.size()) ) {
                                    SI.uses.push_back(ci);
                                }
                            } else {
                                // partial write through an unknown element: touches
                                // everything its known prefix overlaps, kills nothing
                                size_t n = das::min<size_t>(known, C.path.size());
                                if ( aliasPathAgree(ch.path, C.path, n) ) SI.uses.push_back(ci);
                            }
                        }
                        SI.rhsPure = static_cast<ExprCopy *>(stmt)->right->noSideEffects;
                    } else if ( stmt->rtti_isLet() ) {
                        auto let = static_cast<ExprLet *>(stmt);
                        for ( auto & pv : let->variables ) {
                            auto bit = byBase.find(pv);
                            if ( bit==byBase.end() ) continue;
                            for ( int ci : bit->second ) {
                                SI.kills.push_back(ci); // nothing before the declaration observes it
                                if ( chains[ci].path.empty() && pv->init
                                    && !pv->init_via_move && !pv->init_via_clone
                                    && pv->init->noSideEffects ) {
                                    SI.letInits.push_back(DseLetInit{ci, pv});
                                }
                            }
                        }
                    }
                    DseReads reads(byBase, chains, SI.uses, spine.empty() ? nullptr : &spine);
                    stmt->visit(reads);
                }
            }
            // backward liveness to fixpoint. blocks are numbered in creation order, so
            // reverse order approximates reverse topological order and converges fast
            vector<uint64_t> live(nw);
            auto applyBackward = [&]( DseStmt & SI, vector<uint64_t> & lv ) {
                for ( int k : SI.kills ) lv[k>>6] &= ~(1ull<<(k&63));
                for ( int u : SI.uses ) lv[u>>6] |= 1ull<<(u&63);
            };
            bool changed = true;
            while ( changed ) {
                changed = false;
                for ( size_t bi=cfg.blocks.size(); bi-->0; ) {
                    auto b = cfg.blocks[bi];
                    auto & BI = binfo[bi];
                    for ( auto & w : BI.liveOut ) w = 0;
                    for ( auto s : b->succ ) {
                        auto & sin = binfo[s->id].liveIn;
                        for ( size_t w=0; w!=nw; ++w ) BI.liveOut[w] |= sin[w];
                    }
                    live = BI.liveOut;
                    for ( size_t si=BI.stmts.size(); si-->0; ) applyBackward(BI.stmts[si], live);
                    if ( live != BI.liveIn ) { BI.liveIn = live; changed = true; }
                }
            }
            // final scan: a store whose chain is dead after it and whose rhs is pure is
            // removed - it neither kills (already dead) nor gens (rhs not evaluated),
            // which lets a chain of dead stores fall in a single pass. a dead pure
            // declaration init is stripped to the plain zero-init declaration
            auto isDead = [&]( const vector<uint64_t> & lv, int ci ) {
                return ((lv[ci>>6] >> (ci&63)) & 1)==0;
            };
            vector<uint64_t> liveInit(nw);
            for ( size_t bi=0, bis=cfg.blocks.size(); bi!=bis; ++bi ) {
                auto b = cfg.blocks[bi];
                auto & BI = binfo[bi];
                live = BI.liveOut;
                for ( size_t si=BI.stmts.size(); si-->0; ) {
                    auto & SI = BI.stmts[si];
                    if ( SI.removeIdx>=0 && isDead(live, SI.removeIdx) ) {
                        if ( SI.rhsPure ) {
                            dead.insert(b->stmts[si]);
                            continue;   // the statement vanishes whole: no kills, no uses
                        }
                        // impure rhs: the store dies, the rhs stays as a bare statement -
                        // no kills (the write is gone), uses stay (the rhs still evaluates).
                        // NOT for an operator-topped rhs: a bare `a + b` statement trips the
                        // top-level no-side-effect lint when a reused shared module re-lints
                        // its optimized AST (the operator node itself is a pure builtin even
                        // when an operand is not) - that store stays whole
                        auto rhs = static_cast<ExprCopy *>(b->stmts[si])->right;
                        if ( !rhs->rtti_isOp2() && !rhs->rtti_isOp1() ) {
                            deadKeepRhs.insert(b->stmts[si]);
                            for ( int u : SI.uses ) live[u>>6] |= 1ull<<(u&63);
                            continue;
                        }
                    }
                    if ( !SI.letInits.empty() ) {
                        // a later init in the same `let` may read an earlier variable -
                        // judge deadness with the statement's own uses folded in
                        liveInit = live;
                        for ( int u : SI.uses ) liveInit[u>>6] |= 1ull<<(u&63);
                        for ( auto & li : SI.letInits ) {
                            if ( isDead(liveInit, li.cand) ) deadInits.insert(li.var);
                        }
                    }
                    applyBackward(SI, live);
                }
            }
        }

        class EliminateDeadStores : public PassVisitor {
        public:
            using PassVisitor::PassVisitor;
            using PassVisitor::visit;
        protected:
            das_hash_set<Expression *> dead;
            das_hash_set<Expression *> deadKeepRhs;
            das_hash_set<Variable *> deadInits;
            virtual bool canVisitFunction ( Function * fun ) override {
                if ( fun->stub || fun->isTemplate || !funcIsDirty(fun) ) return false;
                if ( !fun->body || !fun->body->rtti_isBlock() ) return false;
                dead.clear();
                deadKeepRhs.clear();
                deadInits.clear();
                analyzeDeadStores(fun, dead, deadKeepRhs, deadInits);
                return !dead.empty() || !deadKeepRhs.empty() || !deadInits.empty();
            }
            virtual bool canVisitStructure ( Structure * ) override { return false; }
            virtual bool canVisitStructureFieldInit ( Structure * ) override { return false; }
            virtual bool canVisitArgumentInit ( Function *, const VariablePtr &, Expression * ) override { return false; }
            virtual bool canVisitQuoteSubexpression ( ExprQuote * ) override { return false; }
            virtual bool canVisitGlobalVariable ( Variable * ) override { return false; }
            virtual bool canVisitEnumeration ( Enumeration * ) override { return false; }
            virtual ExpressionPtr visitBlockExpression ( ExprBlock * block, Expression * expr ) override {
                if ( dead.find(expr)!=dead.end() ) {
                    reportFolding();
                    return nullptr;
                }
                if ( deadKeepRhs.find(expr)!=deadKeepRhs.end() ) {
                    reportFolding();
                    return static_cast<ExprCopy *>(expr)->right;
                }
                return Visitor::visitBlockExpression(block, expr);
            }
            virtual ExpressionPtr visit ( ExprLet * let ) override {
                for ( auto & pv : let->variables ) {
                    if ( deadInits.find(pv)!=deadInits.end() ) {
                        pv->init = nullptr;
                        reportFolding();
                    }
                }
                return Visitor::visit(let);
            }
        };
    }

    // program

    bool Program::optimizationDeadStores(int32_t round) {
        if ( options.getBoolOption("disable_dse", policies.disable_dse) ) return false;
        checkSideEffects();
        EliminateDeadStores pass(round);
        visit(pass);
        return pass.didAnything();
    }
}
