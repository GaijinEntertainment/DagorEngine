#include "daScript/misc/platform.h"

#include "daScript/ast/ast.h"
#include "daScript/ast/ast_visitor.h"

namespace das {

    class UnsafeFolding : public PassVisitor {
    protected:
        virtual ExpressionPtr visit ( ExprUnsafe * expr ) {
            return expr->body;
        }
    };

    void Program::foldUnsafe() {
        UnsafeFolding context;
        visit(context);
    }


    // this folds the following, by setting r2v flag on expressions
    //  r2v(var)            = @var
    //  r2v(expr.field)     = expr.@field
    //  r2v(expr[index])    = expr@[index]
    //  r2v(a ? b : c)      = a ? r2v(b) : r2v(c)
    //  r2v(cast(x))        = cast(r2v(x))
    class RefFolding : public PassVisitor {
    protected:
        virtual ExpressionPtr visit ( ExprRef2Value * expr ) override {
            if (expr->type->baseType == Type::tHandle) {
                return Visitor::visit(expr);
            }
            if ( expr->subexpr->rtti_isCast() ) {
                reportFolding();
                auto ecast = static_pointer_cast<ExprCast>(expr->subexpr);
                auto nr2v = make_smart<ExprRef2Value>();
                nr2v->at = expr->at;
                nr2v->subexpr = ecast->subexpr;
                nr2v->type = make_smart<TypeDecl>(*nr2v->subexpr->type);
                nr2v->type->ref = false;
                ecast->subexpr = nr2v;
                ecast->type->ref = false;
                return ecast;
            } else if ( expr->subexpr->rtti_isVar() ) {
                if ( expr->subexpr->type->isHandle() ) {
                    return Visitor::visit(expr);
                } else {
                    reportFolding();
                    auto evar = static_pointer_cast<ExprVar>(expr->subexpr);
                    evar->r2v = true;
                    evar->type->ref = false;
                    return evar;
                }
            } else if ( expr->subexpr->rtti_isField() ) {
                reportFolding();
                auto efield = static_pointer_cast<ExprField>(expr->subexpr);
                efield->r2v = true;
                efield->type->ref = false;
                return efield;
            } else if ( expr->subexpr->rtti_isAsVariant() ) {
                reportFolding();
                auto efield = static_pointer_cast<ExprAsVariant>(expr->subexpr);
                efield->r2v = true;
                efield->type->ref = false;
                return efield;
            } else if ( expr->subexpr->rtti_isSafeAsVariant() ) {
                reportFolding();
                auto efield = static_pointer_cast<ExprSafeAsVariant>(expr->subexpr);
                efield->r2v = true;
                efield->type->ref = false;
                return efield;
            } else if ( expr->subexpr->rtti_isSwizzle() ) {
                reportFolding();
                auto eswiz = static_pointer_cast<ExprSwizzle>(expr->subexpr);
                eswiz->r2v = true;
                eswiz->type->ref = false;
                if (!TypeDecl::isSequencialMask(eswiz->fields)) {
                    eswiz->value = Expression::autoDereference(eswiz->value);
                }
                return eswiz;
            } else if ( expr->subexpr->rtti_isSafeField() ) {
                DAS_ASSERTF(false, "we should not be here. R2V of ?. is strange indeed");
                reportFolding();
                auto efield = static_pointer_cast<ExprSafeField>(expr->subexpr);
                efield->r2v = true;
                efield->type->ref = false;
                return efield;
            } else if ( expr->subexpr->rtti_isAt() ) {
                reportFolding();
                auto eat = static_pointer_cast<ExprAt>(expr->subexpr);
                eat->r2v = true;
                eat->type->ref = false;
                return eat;
            } else if ( expr->subexpr->rtti_isSafeAt() ) {
                DAS_ASSERTF(false, "we should not be here. R2V of ?[ is strange indeed");
                reportFolding();
                auto eat = static_pointer_cast<ExprSafeAt>(expr->subexpr);
                eat->r2v = true;
                eat->type->ref = false;
                return eat;
            } else if ( expr->subexpr->rtti_isOp3() ) {
                reportFolding();
                auto op3 = static_pointer_cast<ExprOp3>(expr->subexpr);
                op3->left = Expression::autoDereference(op3->left);
                op3->right = Expression::autoDereference(op3->right);
                op3->type->ref = false;
                return expr->subexpr;
            } else if ( expr->subexpr->rtti_isNullCoalescing() ) {
                reportFolding();
                auto nc = static_pointer_cast<ExprNullCoalescing>(expr->subexpr);
                nc->defaultValue = Expression::autoDereference(nc->defaultValue);
                nc->type->ref = false;
                return nc;
            } else {
                return Visitor::visit(expr);
            }
        }
    };

    bool doesExprTerminates ( const ExpressionPtr & expr ) {
        if ( !expr ) return false;
        if ( expr->rtti_isBlock() ) {
            auto pBlock = static_pointer_cast<ExprBlock>(expr);
            for ( auto & e : pBlock->list ) {
                if ( e->rtti_isReturn() || e->rtti_isBreak() || e->rtti_isContinue() ) {
                    return true;
                }
                if ( e->rtti_isBlock() ) {
                    if ( doesExprTerminates(e) ) {
                        return true;
                    }
                }
            }
        } else if ( expr->rtti_isIfThenElse() ) {
            auto pIte = static_pointer_cast<ExprIfThenElse>(expr);
            if ( doesExprTerminates(pIte->if_true) && doesExprTerminates(pIte->if_false) ) {
                return true;
            }
        } else if ( expr->rtti_isWith() ) {
            auto pWith = static_pointer_cast<ExprWith>(expr);
            if ( pWith->body && doesExprTerminates(pWith->body) ) {
                return true;
            }
        }
        return false;
    }

    class BlockFolding : public PassVisitor {
    protected:
        das_set<int32_t> labels;
        bool allLabels = false;
    protected:
        bool hasLabels ( vector<ExpressionPtr> & blockList ) const {
            for ( auto & expr : blockList ) {
                if ( !expr ) continue;
                if ( expr->rtti_isLabel() ) {
                    return true;
                } else  if ( expr->rtti_isBlock() ) {
                    auto pBlock = static_pointer_cast<ExprBlock>(expr);
                    if ( !pBlock->isClosure ) {
                        if ( hasLabels(pBlock->list) ) return true;
                    }
                }
            }
            return false;
        }
        void collect ( vector<ExpressionPtr> & list, vector<ExpressionPtr> & blockList ) {
            bool stopAtExit = !hasLabels(blockList);
            bool skipTilLabel = false;
            for ( auto & expr : blockList ) {
                if ( !expr ) continue;
                if ( expr->rtti_isLabel() ) {
                    auto lexpr = static_pointer_cast<ExprLabel>(expr);
                    if ( allLabels || (labels.find(lexpr->label)!=labels.end()) ) {
                        list.push_back(expr);
                        skipTilLabel = false;
                    }
                    continue;
                }
                if ( skipTilLabel ) continue;
                if ( expr->rtti_isGoto() ) {
                    list.push_back(expr);
                    skipTilLabel = true;
                    continue;
                }
                if ( expr->rtti_isBreak() || expr->rtti_isReturn() || expr->rtti_isContinue() ) {
                    if ( stopAtExit ) {
                        list.push_back(expr);
                        break;
                    } else {
                        list.push_back(expr);
                        skipTilLabel = true;
                        continue;
                    }
                }
                if ( expr->rtti_isIfThenElse() && doesExprTerminates(expr) ) {
                    if ( stopAtExit ) {
                        list.push_back(expr);
                        break;
                    } else {
                        list.push_back(expr);
                        skipTilLabel = true;
                        continue;
                    }
                }
                if ( expr->rtti_isBlock() ) {
                    auto pBlock = static_pointer_cast<ExprBlock>(expr);
                    if ( !pBlock->isClosure && !pBlock->finalList.size() ) {
                        collect(list, pBlock->list);
                    } else {
                        list.push_back(expr);
                    }
                } else if ( expr->rtti_isWith() ) {
                    auto pWith = static_pointer_cast<ExprWith>(expr);
                    if ( pWith->body ) {
                        list.push_back(pWith->body);
                    }
                } else if ( expr->rtti_isAssume() ) {
                    // do nothing with assume
                } else {
                    list.push_back(expr);
                }
            }
        }
    protected:
        virtual void preVisit ( ExprGoto * expr ) override {
            Visitor::preVisit(expr);
            if ( expr->subexpr ) {
                allLabels = true;
            } else {
                labels.insert(expr->label);
            }
        }
    // ExprBlock
        virtual ExpressionPtr visit ( ExprBlock * block ) override {
            vector<ExpressionPtr> list;
            collect(list, block->list);
            if ( list!=block->list ) {
                swap ( block->list, list );
                reportFolding();
            }
            vector<ExpressionPtr> finalList;
            collect(finalList, block->finalList);
            if ( finalList!=block->finalList ) {
                swap ( block->finalList, finalList );
                reportFolding();
            }
            return Visitor::visit(block);
        }
    // ExprLet
        virtual ExpressionPtr visit ( ExprLet * let ) override {
            if ( let->variables.size()==0 ) {
                reportFolding();
                return nullptr;
            }
            return Visitor::visit(let);
        }
    // function
        virtual FunctionPtr visit ( Function * func ) override {
            labels.clear();
            allLabels = false;
            if ( func->body && func->result->isVoid() ) {   // remove trailing return on the void function
                if ( func->body->rtti_isBlock() ) {
                    auto block = static_pointer_cast<ExprBlock>(func->body);
                    if ( block->list.size() && block->list.back()->rtti_isReturn() ) {
                        block->list.resize(block->list.size()-1);
                        reportFolding();
                        return Visitor::visit(func);
                    }
                }
            }
            return Visitor::visit(func);
        }
    };

    class CondFolding : public PassVisitor {
    protected:
        Function * func = nullptr;
        virtual void preVisit ( Function * f ) override {
            Visitor::preVisit(f);
            func = f;
        }
        virtual FunctionPtr visit ( Function * f ) override {
            func = nullptr;
            return Visitor::visit(f);
        }
        virtual ExpressionPtr visit ( ExprIfThenElse * expr ) override {
            // if ( func && func->generator ) return Visitor::visit(expr);
            // if (cond) return x; else return y; => (cond ? x : y)
            if (expr->if_false) {
                smart_ptr<ExprReturn> lr, rr;
                if (expr->if_true->rtti_isBlock()) {
                    auto tb = static_pointer_cast<ExprBlock>(expr->if_true);
                    if (tb->list.size() == 1 && tb->list.back()->rtti_isReturn()) {
                        lr = static_pointer_cast<ExprReturn>(tb->list.back());
                        if ( lr->subexpr && lr->subexpr->rtti_isMakeLocal() ) {
                            lr.reset();   // we don't touch CMRES stuff
                        }
                    }
                }
                if (expr->if_false->rtti_isBlock()) {
                    auto fb = static_pointer_cast<ExprBlock>(expr->if_false);
                    if (fb->list.size() == 1 && fb->list.back()->rtti_isReturn()) {
                        rr = static_pointer_cast<ExprReturn>(fb->list.back());
                        if ( rr->subexpr && rr->subexpr->rtti_isMakeLocal() ) {
                            rr.reset();   // we don't touch CMRES stuff
                        }
                    }
                }
                if (lr && rr) {
                    if ( lr->moveSemantics != rr->moveSemantics ) {
                        lr.reset(); // move semantics must match
                        rr.reset();
                    } else if ( lr->subexpr && rr->subexpr && (lr->subexpr->type->isRef() || rr->subexpr->type->isRef()) ) {
                        lr.reset(); // ref types must match
                        rr.reset();
                    }
                }
                if (lr && rr) {
                    if ( lr->subexpr ) {
                        auto newCond = make_smart<ExprOp3>(expr->at, "?", expr->cond, lr->subexpr, rr->subexpr);
                        newCond->type = make_smart<TypeDecl>(*lr->subexpr->type);
                        auto newRet = make_smart<ExprReturn>(expr->at, newCond);
                        newRet->moveSemantics = lr->moveSemantics;
                        reportFolding();
                        return newRet;
                    } else {
                        // this is actually if ( a ) return; else return;
                        reportFolding();
                        return lr;
                    }
                }
            }
            return Visitor::visit(expr);
        }
        // ExprBlock
        virtual ExpressionPtr visit ( ExprBlock * block ) override {
            if ( func && func->generator ) return Visitor::visit(block);
            /*
            if ( cond )
                ...
                break or return or continue
            b
                =>
            if ( cond )
                ...
                break or return or continue
            else
                b
            */
            if (!block->isClosure && block->list.size() > 1) {
                for ( int i=0, is=int(block->list.size())-1; i!=is; ++i ) {
                    auto expr = block->list[i];
                    if (expr != block->list.back()) {
                        if (expr->rtti_isIfThenElse()) {
                            auto ite = static_pointer_cast<ExprIfThenElse>(expr);
                            if (!ite->if_false) {
                                if (ite->if_true->rtti_isBlock()) {
                                    auto tb = static_pointer_cast<ExprBlock>(ite->if_true);
                                    if ( tb->list.size() ) {
                                        auto lastE = tb->list.back();
                                        if (lastE->rtti_isReturn() || lastE->rtti_isBreak() || lastE->rtti_isContinue()) {
                                            vector<ExpressionPtr> tail;
                                            tail.insert(tail.begin(), block->list.begin() + i + 1, block->list.end());
                                            auto fb = make_smart<ExprBlock>();
                                            fb->at = tail.front()->at;
                                            swap(fb->list, tail);
                                            ite->if_false = fb;
                                            block->list.resize(i + 1);
                                            reportFolding();
                                            return Visitor::visit(block);
                                        }
                                    }
                                }
                            }
                        }
                    }
                }
            }
            return Visitor::visit(block);
        }
    };

    // program

    bool Program::optimizationRefFolding() {
        bool any = false, anything = false;
        do {
            RefFolding context;
            visit(context);
            any = context.didAnything();
            anything |= any;
        } while ( any );
        return anything;
    }

    bool Program::optimizationBlockFolding() {
        BlockFolding context;
        visit(context);
        return context.didAnything();
    }

    bool Program::optimizationCondFolding() {
        CondFolding context;
        visit(context);
        return context.didAnything();
    }

}

