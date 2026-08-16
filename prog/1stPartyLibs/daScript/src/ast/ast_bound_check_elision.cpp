#include "daScript/misc/platform.h"

#include "daScript/ast/ast.h"
#include "daScript/ast/ast_visitor.h"
#include "daScript/ast/ast_cfg.h"
#include "daScript/ast/ast_bound_check_elision.h"

#include <set>
#include <algorithm>
#include <iterator>
#include <optional>

namespace das {

    // ===== bound-check elision: CFG-based range facts =====
    // For each function we run a forward "must" dataflow over its CFG. A fact is `0 <= idx < BOUND`
    // where BOUND is a compile-time constant or length(arrayVar). Facts are genned by loop induction
    // (for i in range(..)) at the loop-body block and by branch guards (if (i<length(a)) ...) on the
    // taken edge; merged by INTERSECTION at joins; killed by any array mutation (any resize/rebind -
    // we assume everything may alias everything) or by reassigning the index. An ExprAt whose
    // (index, array) matches a live fact - plus a constant index provably in a fixed dim - is marked
    // noBoundCheck. Conservative and provable-only: when unsure, no fact, so the runtime check stays.
    namespace {
        static std::optional<int64_t> fixedDim ( const TypeDeclPtr & t ) {
            if ( !t ) return std::nullopt;
            if ( t->baseType==Type::tFixedArray && t->fixedDim>0 ) return int64_t(t->fixedDim);
            if ( t->isVectorType() ) return int64_t(t->getVectorDim());
            return std::nullopt;
        }
        static std::optional<int64_t> constIntValue ( const Expression * e ) {
            if ( !e->rtti_isConstant() || !e->type ) return std::nullopt;
            switch ( e->type->baseType ) {
            case Type::tInt:    return int64_t(((ExprConstInt *)e)->getValue());
            case Type::tInt8:   return int64_t(((ExprConstInt8 *)e)->getValue());
            case Type::tInt16:  return int64_t(((ExprConstInt16 *)e)->getValue());
            case Type::tInt64:  return ((ExprConstInt64 *)e)->getValue();
            case Type::tUInt:   return int64_t(((ExprConstUInt *)e)->getValue());
            case Type::tUInt8:  return int64_t(((ExprConstUInt8 *)e)->getValue());
            case Type::tUInt16: return int64_t(((ExprConstUInt16 *)e)->getValue());
            case Type::tUInt64: return int64_t(((ExprConstUInt64 *)e)->getValue());
            default:            return std::nullopt;
            }
        }
        // (lo, hi)
        static std::optional<std::pair<int64_t,int64_t>> constRange ( const Expression * src ) {
            if ( src->rtti_isConstant() && src->type && src->type->isRange() ) {
                if ( src->type->baseType==Type::tRange ) {
                    auto r = ((ExprConstRange *)src)->getValue(); return std::pair<int64_t,int64_t>{r.from, r.to};
                } else if ( src->type->baseType==Type::tRange64 ) {
                    auto r = ((ExprConstRange64 *)src)->getValue(); return std::pair<int64_t,int64_t>{r.from, r.to};
                }
                return std::nullopt;
            }
            if ( src->rtti_isCall() ) {
                auto call = (ExprCall *) src;
                if ( call->name=="range" || call->name=="urange" || call->name=="range64" || call->name=="urange64" ) {
                    if ( call->arguments.size()==1 ) {
                        if ( auto hi = constIntValue(call->arguments[0]) ) return std::pair<int64_t,int64_t>{0, *hi};
                    } else if ( call->arguments.size()==2 ) {
                        auto lo = constIntValue(call->arguments[0]);
                        auto hi = constIntValue(call->arguments[1]);
                        if ( lo && hi ) return std::pair<int64_t,int64_t>{*lo, *hi};
                    }
                }
            }
            return std::nullopt;
        }
        static Variable * lengthOfVar ( const Expression * e ) {
            if ( !e->rtti_isCall() ) return nullptr;
            auto call = (ExprCall *) e;
            if ( call->name!="length" || call->arguments.size()!=1 ) return nullptr;
            auto a = call->arguments[0];
            if ( !a->rtti_isVar() ) return nullptr;
            return ((ExprVar *)a)->variable;
        }
        // (lo, lenOf)
        static std::optional<std::pair<int64_t,Variable*>> symbolicLenRange ( const Expression * src ) {
            if ( !src->rtti_isCall() ) return std::nullopt;
            auto call = (ExprCall *) src;
            if ( call->name!="range" && call->name!="urange" ) return std::nullopt;
            if ( call->arguments.size()==1 ) {
                if ( auto lenOf = lengthOfVar(call->arguments[0]) ) return std::pair<int64_t,Variable*>{0, lenOf};
            } else if ( call->arguments.size()==2 ) {
                auto lenOf = lengthOfVar(call->arguments[1]);
                auto lo = constIntValue(call->arguments[0]);
                if ( lenOf && lo ) return std::pair<int64_t,Variable*>{*lo, lenOf};
            }
            return std::nullopt;
        }
        static bool isUnsignedVar ( const Variable * v ) {
            if ( !v || !v->type ) return false;
            switch ( v->type->baseType ) {
            case Type::tUInt: case Type::tUInt8: case Type::tUInt16: case Type::tUInt64: return true;
            default: return false;
            }
        }
        static Expression * unwrap ( Expression * e ) {
            while (e) {
                if ( e->rtti_isR2V() ) { e = ((ExprRef2Value *)e)->subexpr; continue; }
                if ( e->rtti_isCast() ) { e = ((ExprCast *)e)->subexpr; continue; }
                if ( e->rtti_isCall() ) {
                    auto c = (ExprCall *)e;
                    // non-truncating numeric casts preserve a non-negative value (int8/int16 would truncate)
                    if ( c->arguments.size()==1 && (c->name=="int"||c->name=="uint"||c->name=="int64"||c->name=="uint64") ) {
                        e = c->arguments[0]; continue;
                    }
                }
                return e;
            }
            return e;
        }
        static Variable * asVar ( Expression * e ) {
            e = unwrap(e);
            return (e && e->rtti_isVar()) ? ((ExprVar *)e)->variable : nullptr;
        }
        // a comparison bound: either length(lv), or the constant cv
        struct Bound { bool isLen; Variable * lv; int64_t cv; };
        static std::optional<Bound> boundOf ( Expression * e ) {
            e = unwrap(e);
            if ( auto lv = lengthOfVar(e) ) return Bound{true, lv, 0};
            if ( auto cv = constIntValue(e) ) return Bound{false, nullptr, *cv};
            return std::nullopt;
        }

        // A proven bound: 0 <= idx < (lenOf ? length(lenOf) : hi).
        struct UB {
            Variable * idx = nullptr;
            Variable * lenOf = nullptr;
            int64_t    hi = 0;
            struct Hash {
                size_t operator () ( const UB & u ) const {
                    size_t h = das::hash<Variable *>()(u.idx);
                    h = h*31 + das::hash<Variable *>()(u.lenOf);
                    return h*31 + das::hash<int64_t>()(u.hi);
                }
            };
            bool operator < ( const UB & o ) const {
                if ( idx != o.idx ) return idx < o.idx;
                if ( lenOf != o.lenOf ) return lenOf < o.lenOf;
                return hi < o.hi;
            }
            bool operator == ( const UB & o ) const {
                return idx==o.idx && lenOf==o.lenOf && hi==o.hi;
            }
        };
        struct FactSet {
            das_set<UB, UB::Hash> ub;   // proven idx < bound
            das_set<Variable *>   nn;   // proven idx >= 0
            template <typename ST>
            static bool setEq ( const ST & a, const ST & b ) {
                if ( a.size() != b.size() ) return false;
                for ( auto & e : a ) if ( !b.count(e) ) return false;
                return true;
            }
            bool operator == ( const FactSet & o ) const { return setEq(ub,o.ub) && setEq(nn,o.nn); }
        };
        static FactSet meet ( const FactSet & a, const FactSet & b ) {
            FactSet r;
            for ( auto & u : a.ub ) if ( b.ub.count(u) ) r.ub.insert(u);
            for ( auto & v : a.nn ) if ( b.nn.count(v) ) r.nn.insert(v);
            return r;
        }

        // vars written (rebound / mutable-ref passed) in a straight-line stmt; element containers
        // (v[i]=...) are excused - they don't change v.
        struct WriteScan : Visitor {
            das_set<Variable *> * out = nullptr;
            das_set<Expression *> excused;
            virtual void preVisit ( ExprAt * e ) override {
                if ( e->subexpr && e->subexpr->rtti_isVar() ) excused.insert(e->subexpr);
            }
            virtual void preVisit ( ExprVar * e ) override {
                if ( e->write && e->variable && excused.find(e)==excused.end() ) out->insert(e->variable);
            }
            static void writtenVars ( Expression * e, das_set<Variable *> & out ) {
                if ( !e ) return;
                WriteScan s; s.out = &out; e->visit(s);
            }
        };
        static bool arrayMutatedIn ( Expression * e ) {
            das_set<Variable *> w; WriteScan::writtenVars(e, w);
            for ( auto v : w ) if ( v->type && v->type->isGoodArrayType() ) return true;
            return false;
        }

        // facts genned by a for-loop's induction variables (valid throughout the loop body block)
        static void loopGen ( ExprFor * fr, FactSet & out ) {
            if ( !fr ) return;
            bool bodyMut = arrayMutatedIn(fr->body);   // a length bound is loop-carried: needs a mutation-free body
            auto n = fr->iteratorVariables.size()<fr->sources.size() ? fr->iteratorVariables.size() : fr->sources.size();
            for ( size_t i=0; i<n; ++i ) {
                auto v = fr->iteratorVariables[i];
                if ( !v ) continue;
                if ( auto r = constRange(fr->sources[i]); r && r->first>=0 ) {
                    out.ub.insert(UB{v, nullptr, r->second}); out.nn.insert(v);
                } else if ( auto s = symbolicLenRange(fr->sources[i]); s && s->first>=0 && !bodyMut ) {
                    out.ub.insert(UB{v, s->second, 0}); out.nn.insert(v);
                }
            }
        }

        // facts that hold on the edge where `cond` is `isTrue`
        static void condFacts ( Expression * cond, bool isTrue, FactSet & out ) {
            if ( !cond || !cond->rtti_isOp2() ) return;
            auto op2 = (ExprOp2 *) cond;
            int pred = 0;   // 1:<  2:<=  3:>  4:>=
            if ( op2->name=="<" ) pred=1; else if ( op2->name=="<=" ) pred=2;
            else if ( op2->name==">" ) pred=3; else if ( op2->name==">=" ) pred=4;
            if ( !pred ) return;
            if ( !isTrue ) pred = (pred==1)?4 : (pred==2)?3 : (pred==3)?2 : 1;   // negate on the false edge
            Expression * L = op2->left; Expression * R = op2->right;
            Variable * vL = asVar(L); Variable * vR = asVar(R);
            auto bL = boundOf(L); auto bR = boundOf(R);
            // upper bound  idx < UPPER
            if ( pred==1 && vL && bR ) out.ub.insert(UB{vL, bR->isLen?bR->lv:nullptr, bR->isLen?0:bR->cv});              // v < R
            if ( pred==3 && vR && bL ) out.ub.insert(UB{vR, bL->isLen?bL->lv:nullptr, bL->isLen?0:bL->cv});              // L > v => v < L
            if ( pred==2 && vL && bR && !bR->isLen ) out.ub.insert(UB{vL, nullptr, bR->cv+1});                          // v <= c => v < c+1
            if ( pred==4 && vR && bL && !bL->isLen ) out.ub.insert(UB{vR, nullptr, bL->cv+1});                          // c >= v => v < c+1
            // lower bound  idx >= 0
            if ( pred==4 && vL && bR && !bR->isLen && bR->cv>=0 )  out.nn.insert(vL);                                   // v >= c>=0
            if ( pred==2 && vR && bL && !bL->isLen && bL->cv>=0 )  out.nn.insert(vR);                                   // c<=v, c>=0
            if ( pred==3 && vL && bR && !bR->isLen && bR->cv>=-1 ) out.nn.insert(vL);                                   // v > c>=-1
            if ( pred==1 && vR && bL && !bL->isLen && bL->cv>=-1 ) out.nn.insert(vR);                                   // c<v, c>=-1
        }

        static void applyKill ( Expression * stmt, FactSet & f ) {
            das_set<Variable *> w; WriteScan::writtenVars(stmt, w);
            bool mutArr = false;
            for ( auto v : w ) if ( v->type && v->type->isGoodArrayType() ) { mutArr = true; break; }
            if ( mutArr ) {   // any array may alias any length bound
                for ( auto it=f.ub.begin(); it!=f.ub.end(); ) { if ( it->lenOf ) it=f.ub.erase(it); else ++it; }
            }
            for ( auto v : w ) {
                for ( auto it=f.ub.begin(); it!=f.ub.end(); ) { if ( it->idx==v ) it=f.ub.erase(it); else ++it; }
                f.nn.erase(v);
            }
        }

        // mark ExprAt nodes in a straight-line stmt against the facts holding at that point.
        // does not descend into deferred bodies (lambdas/blocks) - their facts are not this scope's.
        struct MarkScan : Visitor {
            const FactSet * f = nullptr;
            TextWriter *    logs = nullptr;   // when set, report each elided access
            const char *    fnName = "";
            int lam = 0;
            void report ( ExprAt * e, const char * why ) {
                if ( logs ) *logs << "[bound-check-elision] " << fnName << ": " << e->at.describe()
                                  << " " << e->describe() << " (" << why << ")\n";
            }
            virtual void preVisit ( ExprMakeBlock * e ) override { Visitor::preVisit(e); lam++; }
            virtual ExpressionPtr visit ( ExprMakeBlock * e ) override { lam--; return Visitor::visit(e); }
            virtual void preVisit ( ExprAt * e ) override {
                Visitor::preVisit(e);
                if ( lam || e->noBoundCheck ) return;
                if ( auto dim = fixedDim(e->subexpr->type) ) {
                    if ( auto c = constIntValue(e->index) ) { if ( *c>=0 && *c<*dim ) { e->noBoundCheck = true; report(e,"const index"); } return; }
                    auto v = asVar(e->index);
                    if ( v && (isUnsignedVar(v) || f->nn.count(v)) )
                        for ( const auto & u : f->ub ) if ( u.idx==v && !u.lenOf && u.hi<=*dim ) { e->noBoundCheck = true; report(e,"induction/guard in fixed dim"); break; }
                } else if ( e->subexpr->type && e->subexpr->type->isGoodArrayType() ) {
                    auto v = asVar(e->index); auto a = asVar(e->subexpr);
                    if ( v && a && (isUnsignedVar(v) || f->nn.count(v)) )
                        for ( const auto & u : f->ub ) if ( u.idx==v && u.lenOf==a ) { e->noBoundCheck = true; report(e,"index bounded by length"); break; }
                }
            }
            static void mark ( Expression * stmt, const FactSet & f, TextWriter * logs, const char * fnName ) {
                MarkScan ms; ms.f = &f; ms.logs = logs; ms.fnName = fnName; stmt->visit(ms);
            }
        };

        struct AbcAnalysis {
            const Cfg & cfg;
            TextWriter * logs = nullptr;   // when set, report each elided access
            explicit AbcAnalysis ( const Cfg & c ) : cfg(c) {}

            static bool isBranch ( CfgBlock * b ) { return b->succ.size()==2 && !b->stmts.empty(); }

            FactSet transfer ( const FactSet & in, CfgBlock * b ) const {
                FactSet f = in;
                loopGen(b->loopSource, f);
                for ( auto s : b->stmts ) applyKill(s, f);
                return f;
            }
            FactSet edgeOut ( CfgBlock * from, CfgBlock * to, const FactSet & out ) const {
                FactSet r = out;
                if ( isBranch(from) ) {
                    bool isTrue = ( from->succ[0]==to );
                    bool isFalse = ( from->succ[1]==to );
                    if ( isTrue != isFalse ) condFacts(from->stmts.back(), isTrue, r);   // skip if ambiguous (both)
                }
                return r;
            }

            void run () {
                size_t n = cfg.blocks.size();
                if ( !n ) return;
                // universe of all facts that could ever be genned - the optimistic top for intersection.
                FactSet U;
                for ( auto b : cfg.blocks ) {
                    loopGen(b->loopSource, U);
                    if ( isBranch(b) ) { condFacts(b->stmts.back(), true, U); condFacts(b->stmts.back(), false, U); }
                }
                vector<FactSet> OUT(n, U);
                auto inOf = [&]( CfgBlock * b ) -> FactSet {
                    if ( b==cfg.entry || b->pred.empty() ) return FactSet();
                    FactSet in; bool first = true;
                    for ( auto p : b->pred ) {
                        FactSet eo = edgeOut(p, b, OUT[p->id]);
                        if ( first ) { in = eo; first = false; } else in = meet(in, eo);
                    }
                    return in;
                };
                bool changed = true;
                while ( changed ) {
                    changed = false;
                    for ( auto b : cfg.blocks ) {
                        FactSet out = transfer(inOf(b), b);
                        if ( !(out==OUT[b->id]) ) { OUT[b->id] = out; changed = true; }
                    }
                }
                // mark: replay the transfer per block, marking accesses against the facts at each stmt.
                for ( auto b : cfg.blocks ) {
                    FactSet cur = inOf(b);
                    loopGen(b->loopSource, cur);
                    for ( auto s : b->stmts ) {
                        applyKill(s, cur);          // kill first, then mark: an access in a mutating stmt is not elided
                        MarkScan::mark(s, cur, logs, cfg.func ? cfg.func->name.c_str() : "?");
                    }
                }
            }
        };

        // [hint(unsafe_range_check)] override: mark every access in the body (skips nested lambdas - separate scopes)
        struct MarkAllScan : Visitor {
            TextWriter * logs = nullptr;
            const char * fnName = "";
            int lam = 0;
            virtual void preVisit ( ExprMakeBlock * e ) override { Visitor::preVisit(e); lam++; }
            virtual ExpressionPtr visit ( ExprMakeBlock * e ) override { lam--; return Visitor::visit(e); }
            virtual void preVisit ( ExprAt * e ) override {
                Visitor::preVisit(e);
                if ( lam || e->noBoundCheck ) return;
                e->noBoundCheck = true;
                if ( logs ) *logs << "[bound-check-elision] " << fnName << ": " << e->at.describe()
                                  << " " << e->describe() << " (unsafe_range_check hint)\n";
            }
            static void mark ( Function * fn, TextWriter * logs ) {
                if ( !fn->body || !fn->body->rtti_isBlock() ) return;
                MarkAllScan s; s.logs = logs; s.fnName = fn->name.c_str(); fn->body->visit(s);
            }
        };
    }

    static bool hasUnsafeRangeCheckHint ( const FunctionPtr & fn ) {
        for ( auto & ann : fn->annotations )
            if ( ann->annotation->name=="hint" && ann->arguments.getBoolOption("unsafe_range_check", false) ) return true;
        return false;
    }

    void markNoBoundCheck ( Program * program, const ProgramCfg * pcfg, TextWriter & logs ) {
        if ( !program || !program->thisModule ) return;
        const bool doElide = program->options.getBoolOption("bound_check_elision", false);
        const bool doLog = program->options.getBoolOption("log_bound_check_elision", false);
        program->thisModule->functions.foreach([&](auto & fn){
            // manual override: [hint(unsafe_range_check)] marks every access in the function (the same hint the JIT honors)
            if ( hasUnsafeRangeCheckHint(fn) ) { MarkAllScan::mark(fn, doLog ? &logs : nullptr); return; }
            // proven elision over the CFG (opt-in via bound_check_elision)
            if ( doElide && pcfg ) {
                if ( auto c = pcfg->forFunction(fn) ) {
                    AbcAnalysis a(*c);
                    a.logs = doLog ? &logs : nullptr;
                    a.run();
                }
            }
        });
    }

}
