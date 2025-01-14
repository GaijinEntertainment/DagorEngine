#include "daScript/misc/platform.h"

#include "daScript/ast/ast.h"
#include "daScript/ast/ast_visitor.h"

namespace das {


    class TrackVariableFlags : public Visitor {
    protected:
        // global let
        virtual void preVisitGlobalLet ( const VariablePtr & var ) override {
            Visitor::preVisitGlobalLet(var);
            var->access_extern = false;
            var->access_init = false;
            var->access_get = false;
            var->access_ref = false;
            var->access_pass = false;
        }
        virtual void preVisitGlobalLetInit ( const VariablePtr & var, Expression * init ) override {
            Visitor::preVisitGlobalLetInit(var, init);
            var->access_init = true;
        }
        // let
        virtual void preVisitLet ( ExprLet * let, const VariablePtr & var, bool last ) override {
            Visitor::preVisitLet(let, var, last);
            var->access_extern = false;
            var->access_init = false;
            var->access_get = false;
            var->access_ref = false;
            var->access_pass = false;
        }
        virtual void preVisitLetInit ( ExprLet * let, const VariablePtr & var, Expression * init ) override {
            Visitor::preVisitLetInit(let, var, init);
            var->access_init = true;
        }
        // function arguments
        virtual void preVisitArgument ( Function * fn, const VariablePtr & var, bool lastArg ) override {
            Visitor::preVisitArgument(fn, var, lastArg);
            var->access_extern = true;
            var->access_init = false;
            var->access_get = false;
            var->access_ref = false;
            var->access_pass = false;
        }
        virtual void preVisitArgumentInit ( Function * fn, const VariablePtr & var, Expression * init ) override {
            Visitor::preVisitArgumentInit(fn, var, init);
            var->access_init = true;
        }
        // block
        virtual void preVisitBlockArgument ( ExprBlock * block, const VariablePtr & var, bool lastArg ) override {
            Visitor::preVisitBlockArgument(block, var, lastArg);
            var->access_extern = true;
            var->access_init = false;
            var->access_get = false;
            var->access_ref = false;
            var->access_pass = false;
        }
        // for loop sources
        virtual void preVisitFor ( ExprFor *, const VariablePtr & var, bool ) override {
            var->access_init = true;
        }
        // var
        virtual void preVisit ( ExprVar * expr ) override {
            Visitor::preVisit(expr);
            if ( expr->write ) {
                expr->variable->access_ref = true;
            } else {
                if ( !expr->r2v ) {
                    expr->variable->access_pass = true;
                }
                expr->variable->access_get = true;
            }
        }
    };

    /*
     TODO:
        cond ? a : b
        a ?? b
        ?.
        ->
     */

    // here we propagate r2cr flag
    //  a.@b    ->  $a.@b
    //  a@[b]   ->  $a@[b]
    //  a.b.@c  ->  $a.$b.@c
    //  a = 5   ->  #a = 5
    //  a.b = 5 ->  #a#.b=5
    //  a[b]=3  ->  #a#[b]=3
    class TrackFieldAndAtFlags : public Visitor {
        das_hash_set<const Function *>   asked;
        FunctionPtr             func;
    public:
        void MarkSideEffects ( Module & mod ) {
            for ( auto & fn : mod.functions.each() ) {
                if (!fn->builtIn) {
                    fn->knownSideEffects = false;
                    fn->sideEffectFlags &= ~uint32_t(SideEffects::inferredSideEffects);
                }
            }
            for ( auto & fn : mod.functions.each() ) {
                if (!fn->builtIn && !fn->knownSideEffects) {
                    asked.clear();
                    getSideEffects(fn);
                }
            }
            for ( auto & var : mod.globals.each() ) {
                if ( var->init ) {
                    TrackVariableFlags vaf;
                    var->init = var->init->visit(vaf);
                }
            }
        }
    protected:
        void propagateAt ( ExprAt * at ) {
            if ( at->subexpr->type->isHandle() && at->subexpr->type->annotation->isIndexMutable(at->index->type.get()) ) {
                propagateWrite(at->subexpr.get());
            } else if ( at->subexpr->type->isGoodTableType() ) {
                propagateWrite(at->subexpr.get());  // note: this makes it so tab[foo] modifies itself
            } else {
                propagateRead(at->subexpr.get());
            }
            propagateRead(at->index.get());
        }
        bool isConstCast ( const TypeDeclPtr & td ) const {
            if ( td->constant ) return true;        // cast<foo const> is const
            if ( td->baseType==Type::tPointer ) {
                if ( !td->firstType ) return true;  // void? cast is ok
                if ( td->firstType && td->firstType->constant ) return true; // cast<foo const?> is const
            } else {
                if ( !td->isRefType() ) return true;    // cast<int> is const
            }
            return false;
        }
        void propagateRead ( Expression * expr ) {
            if ( expr->rtti_isVar() ) {
                auto var = (ExprVar *) expr;
                var->r2cr = true;
                if ( var->variable->source ) {
                    propagateRead(var->variable->source.get());
                }
            } else if ( expr->rtti_isField() || expr->rtti_isSafeField()
                       || expr->rtti_isAsVariant() || expr->rtti_isIsVariant()
                       || expr->rtti_isSafeAsVariant() ) {
                auto field = (ExprField *) expr;
                field->r2cr = true;
                propagateRead(field->value.get());
                if ( func && field->value->type->isPointer() ) func->sideEffectFlags |= uint32_t(SideEffects::accessExternal);
            } else if ( expr->rtti_isSwizzle() ) {
                auto swiz = (ExprSwizzle *) expr;
                swiz->r2cr = true;
                propagateRead(swiz->value.get());
            } else if ( expr->rtti_isAt() ) {
                auto at = (ExprAt *) expr;
                at->r2cr = true;
                propagateAt(at);
            } else if ( expr->rtti_isSafeAt() ) {
                auto at = (ExprSafeAt *) expr;
                at->r2cr = true;
                propagateRead(at->subexpr.get());
                propagateRead(at->index.get());
            } else if ( expr->rtti_isOp3() ) {
                auto op3 = (ExprOp3 *) expr;
                propagateRead(op3->left.get());
                propagateRead(op3->right.get());
            } else if ( expr->rtti_isNullCoalescing() ) {
                auto nc = (ExprNullCoalescing *) expr;
                propagateRead(nc->subexpr.get());
                propagateRead(nc->defaultValue.get());
            } else if ( expr->rtti_isCast() ) {
                auto ca = (ExprCast *) expr;
                if ( isConstCast(ca->castType) ) {
                    propagateRead(ca->subexpr.get());
                } else {
                    propagateWrite(ca->subexpr.get());
                }
            } else if ( expr->rtti_isRef2Ptr() ) {
                auto rr = (ExprRef2Ptr *)expr;
                propagateRead(rr->subexpr.get());
            } else if ( expr->rtti_isPtr2Ref() ) {
                auto rr = (ExprPtr2Ref *)expr;
                propagateRead(rr->subexpr.get());
                if ( func ) func->sideEffectFlags |= uint32_t(SideEffects::accessExternal);
            } else if ( expr->rtti_isR2V() ) {
                auto rr = (ExprRef2Value *)expr;
                propagateRead(rr->subexpr.get());
            }
            // TODO:
            //  propagate read to call or expr-like-call???
            //  do we need to?
        }
        void propagateWrite ( Expression * expr ) {
            if ( expr->rtti_isVar() ) {
                auto var = (ExprVar *) expr;
                var->write = true;
                if ( var->variable->source ) {
                    propagateWrite(var->variable->source.get());
                }
            } else if ( expr->rtti_isField() || expr->rtti_isSafeField()
                       || expr->rtti_isAsVariant() || expr->rtti_isSafeAsVariant() ) {
                auto field = (ExprField *) expr;
                //if ( !field->value->type->isPointer() ) {
                    field->write = true;
                    propagateWrite(field->value.get());
                //} else {
                //    propagateRead(field->value.get());
                //}
                if ( func && field->value->type->isPointer() ) func->sideEffectFlags |= uint32_t(SideEffects::modifyExternal);
            } else if ( expr->rtti_isSwizzle() ) {
                auto swiz = (ExprSwizzle *) expr;
                swiz->write = true;
                propagateWrite(swiz->value.get());
            } else if ( expr->rtti_isAt() || expr->rtti_isSafeAt() ) {
                auto at = (ExprAt *) expr;
                at->write = true;
                propagateWrite(at->subexpr.get());
            } else if ( expr->rtti_isOp3() ) {
                auto op3 = (ExprOp3 *) expr;
                propagateWrite(op3->left.get());
                propagateWrite(op3->right.get());
            } else if ( expr->rtti_isNullCoalescing() ) {
                auto nc = (ExprNullCoalescing *) expr;
                propagateWrite(nc->subexpr.get());
                propagateWrite(nc->defaultValue.get());
            } else if ( expr->rtti_isCast() ) {
                auto ca = (ExprCast *) expr;
                propagateWrite(ca->subexpr.get());
            } else if ( expr->rtti_isRef2Ptr() ) {
                auto rr = (ExprRef2Ptr *)expr;
                propagateWrite(rr->subexpr.get());
            } else if ( expr->rtti_isPtr2Ref() ) {
                auto rr = (ExprPtr2Ref *)expr;
                propagateWrite(rr->subexpr.get());
                if ( func ) func->sideEffectFlags |= uint32_t(SideEffects::modifyExternal);
            } else if ( expr->rtti_isR2V() ) {
                auto rr = (ExprRef2Value *)expr;
                propagateWrite(rr->subexpr.get());
            }
            // TODO:
            //  propagate write to call or expr-like-call???
            //  do we need to?
        }
        uint32_t getSideEffects ( const FunctionPtr & fnc ) {
            if ( fnc->builtIn || fnc->knownSideEffects ) {
                return fnc->sideEffectFlags;
            }
            if ( asked.find(fnc.get())!=asked.end() ) {
                return fnc->sideEffectFlags;   // assume no extra side effects
            }
            asked.insert(fnc.get());
            auto sfn = func;
            func = fnc;
            fnc->visit(*this);
            TrackVariableFlags vaf;
            fnc->visit(vaf);
            func = sfn;
            // now, for the side-effects
            uint32_t flags = fnc->sideEffectFlags;
            if (fnc->useGlobalVariables.size()) {
                flags |= uint32_t(SideEffects::accessGlobal);
            }
            // it has side effects, if it writes to its arguments
            for ( auto & arg : fnc->arguments ) {
                if ( arg->access_ref ) {
                    flags |= uint32_t(SideEffects::modifyArgument);
                }
            }
        // string capture
            // if it calls string capture, it captures string, or has string builder at all
            // note - having string builder is there because we git rid of temp string in the string builder. if we had better place to put it, we could get rid of this
            if ( fnc->callCaptureString || fnc->captureString || fnc->hasStringBuilder ) {
                flags |= uint32_t(SideEffects::captureString);
            }
            // it captures strings if it touches globals with strings
            if ( !(flags & uint32_t(SideEffects::captureString)) ) {
                for ( auto & gv : fnc->useGlobalVariables ) {
                    if ( !gv->type->constant && gv->type->hasStringData() ) {
                        flags |= uint32_t(SideEffects::captureString);
                        break;
                    }
                }
            }
            // it captures strings if it returns a string
            if ( !(flags & uint32_t(SideEffects::captureString)) ) {
                if ( fnc->result->hasStringData() ) {
                    flags |= uint32_t(SideEffects::captureString);
                }
            }
            // it captures strings if it writes to an argument, that is a string
            if ( !(flags & uint32_t(SideEffects::captureString)) ) {
                for ( auto & arg : fnc->arguments ) {
                    if ( !arg->type->constant && arg->access_ref ) {
                        if ( arg->type->ref && arg->type->isString() ) {    // string &, otherwise we can write but its not a capture
                            flags |= uint32_t(SideEffects::captureString);
                            break;
                        } else if ( arg->type->hasStringData() ) {          // [[ string ]]
                            flags |= uint32_t(SideEffects::captureString);
                            break;
                        }
                    }
                }
            }
            // append side effects of the functions it calls
            for ( auto & depF : fnc->useFunctions ) {
                auto dep = depF;
                if ( dep != fnc ) {
                    uint32_t depFlags = getSideEffects(dep);
                    depFlags &= ~uint32_t(SideEffects::modifyArgument);
                    flags |= depFlags;
                }
            }
            fnc->knownSideEffects = true;
            if ( flags & uint32_t(SideEffects::captureString) ) {
                fnc->captureString = true;
                flags &= ~uint32_t(SideEffects::captureString);
            }
            fnc->sideEffectFlags |= flags;
            return flags;
        }
    protected:
        virtual bool canVisitStructureFieldInit ( Structure * ) override { return false; }
        virtual bool canVisitArgumentInit ( Function * , const VariablePtr &, Expression * ) override { return false; }
        virtual bool canVisitQuoteSubexpression ( ExprQuote * ) override { return false; }
    // Variable initializatoin
        virtual void preVisitLetInit ( ExprLet * let, const VariablePtr & var, Expression * init ) override {
            Visitor::preVisitLetInit(let, var, init);
            // TODO:
            //  at some point we should do better data trackng for this type of aliasing
            if ( var->type->ref ) propagateWrite(init);
        }
    // addr of expression
        virtual void preVisit ( ExprRef2Ptr * expr ) override {
            Visitor::preVisit(expr);
            // TODO:
            //  at some point we should do better data trackng for this type of aliasing
            propagateWrite(expr);
        }
    // source in the For loop
        virtual void preVisitForSource ( ExprFor * expr, Expression * subexpr, bool last ) override {
            Visitor::preVisitForSource(expr, subexpr, last);
            if (subexpr->type->isConst()) {
                propagateRead(subexpr);
            } else {
                propagateWrite(subexpr);    // we really don't know, but we assume that it will write
            }
        }
    // ExprField
        virtual void preVisit ( ExprField * expr ) override {
            Visitor::preVisit(expr);
            propagateRead(expr->value.get());
        }
    // ExprSafeField
        virtual void preVisit ( ExprSafeField * expr ) override {
            Visitor::preVisit(expr);
            propagateRead(expr->value.get());
        }
    // ExprIsVariant
        virtual void preVisit ( ExprIsVariant * expr ) override {
            Visitor::preVisit(expr);
            propagateRead(expr->value.get());
        }
    // ExprAsVariant
        virtual void preVisit ( ExprAsVariant * expr ) override {
            Visitor::preVisit(expr);
            propagateRead(expr->value.get());
        }
    // ExprSafeAsVariant
        virtual void preVisit ( ExprSafeAsVariant * expr ) override {
            Visitor::preVisit(expr);
            propagateRead(expr->value.get());
        }
    // ExprAt
        virtual void preVisit ( ExprAt * expr ) override {
            Visitor::preVisit(expr);
            propagateAt(expr);
        }
    // ExprSafeAt
        virtual void preVisit ( ExprSafeAt * expr ) override {
            Visitor::preVisit(expr);
            propagateRead(expr->subexpr.get());
        }
    // ExprMove
        virtual void preVisit ( ExprMove * expr ) override {
            Visitor::preVisit(expr);
            propagateWrite(expr->left.get());
            propagateWrite(expr->right.get());
        }
    // ExprCopy
        virtual void preVisit ( ExprCopy * expr ) override {
            Visitor::preVisit(expr);
            propagateWrite(expr->left.get());
            propagateRead(expr->right.get());
        }
    // ExprClone
        virtual void preVisit ( ExprClone * expr ) override {
            Visitor::preVisit(expr);
            propagateWrite(expr->left.get());
            propagateRead(expr->right.get());
        }
    // Op1
        virtual void preVisit ( ExprOp1 * expr ) override {
            Visitor::preVisit(expr);
            auto sef = getSideEffects(expr->func);
            if ( sef & uint32_t(SideEffects::modifyArgument) ) {
                propagateWrite(expr->subexpr.get());
            }
        }
    // Op2
        virtual void preVisit ( ExprOp2 * expr ) override {
            Visitor::preVisit(expr);
            auto sef = getSideEffects(expr->func);
            if ( sef & uint32_t(SideEffects::modifyArgument) ) {
                auto leftT = expr->left->type;
                if ( leftT->isRefOrPointer() && !leftT->constant ) {
                    propagateWrite(expr->left.get());
                }
                auto rightT = expr->right->type;
                if ( rightT->isRefOrPointer() && !rightT->constant ) {
                    propagateWrite(expr->right.get());
                }
            }
        }
    // Op3
        virtual void preVisit ( ExprOp3 * expr ) override {
            Visitor::preVisit(expr);
            auto sef = expr->func ? getSideEffects(expr->func) : 0;
            if ( sef & uint32_t(SideEffects::modifyArgument) ) {
                auto condT = expr->subexpr->type;
                if ( condT->isRefOrPointer() && !condT->constant ) {
                    propagateWrite(expr->subexpr.get());
                }
                auto leftT = expr->left->type;
                if ( leftT->isRefOrPointer() && !leftT->constant ) {
                    propagateWrite(expr->left.get());
                }
                auto rightT = expr->right->type;
                if ( rightT->isRefOrPointer() && !rightT->constant ) {
                    propagateWrite(expr->right.get());
                }
            }
        }
    // Return
        virtual void preVisit ( ExprReturn * expr ) override {
            Visitor::preVisit(expr);
            // TODO:
            //  at some point we should do better data trackng for this type of aliasing
            if ( expr->returnReference || expr->moveSemantics ) propagateWrite(expr->subexpr.get());
            else if ( expr->subexpr ) propagateRead(expr->subexpr.get());
        }
    // New
        virtual void preVisit ( ExprNew * expr ) override {
            Visitor::preVisit(expr);
            bool newExternal = false;
            auto NT = expr->typeexpr;
            if ( NT->baseType==Type::tHandle ) {
                newExternal = true;
            }
            if ( newExternal ) {
                func->sideEffectFlags |= uint32_t(SideEffects::modifyExternal);
            }
            if ( expr->initializer ) {
                // if modified, modify CALL
                auto sef = getSideEffects(expr->func);
                if ( sef & uint32_t(SideEffects::modifyArgument) ) {
                    for ( size_t ai=0, ais=expr->arguments.size(); ai!=ais; ++ai ) {
                        const auto & argT = expr->func->arguments[ai]->type;
                        if ( argT->canWrite() ) {
                            if ( !expr->func->builtIn ) {
                                if ( expr->func->knownSideEffects ) {
                                    propagateWrite(expr->arguments[ai].get());
                                }
                            } else {
                                if ( expr->func->modifyArgument ) {
                                    propagateWrite(expr->arguments[ai].get());
                                }
                            }
                        }
                    }
                }
            }
        }
    // Delete
        virtual void preVisit ( ExprDelete * expr ) override {
            Visitor::preVisit(expr);
            propagateWrite(expr->subexpr.get());
            bool deleteExternal = false;
            auto NT = expr->subexpr->type;
            if ( NT->baseType==Type::tHandle ) {
                deleteExternal = true;
            } else if ( NT->baseType==Type::tPointer && NT->firstType && NT->firstType->isHandle() ) {
                deleteExternal = true;
            }
            if ( deleteExternal ) {
                func->sideEffectFlags |= uint32_t(SideEffects::modifyExternal);
            }
        }
    // Call
        virtual void preVisit ( ExprCall * expr ) override {
            Visitor::preVisit(expr);
            // if modified, modify NEW
            auto sef = getSideEffects(expr->func);
            if ( sef & uint32_t(SideEffects::modifyArgument) ) {
                for ( size_t ai=0, ais=expr->arguments.size(); ai!=ais; ++ai ) {
                    const auto & argT = expr->func->arguments[ai]->type;
                    if ( argT->canWrite() ) {
                        if ( !expr->func->builtIn ) {
                            if ( expr->func->knownSideEffects ) {
                                propagateWrite(expr->arguments[ai].get());
                            }
                        } else {
                            if ( expr->func->modifyArgument ) {
                                propagateWrite(expr->arguments[ai].get());
                            }
                        }
                    }
                }
            }
        }
    // LooksLikeCall
        virtual void preVisit ( ExprLooksLikeCall * expr ) override {
            Visitor::preVisit(expr);
            for ( size_t ai=0, ais=expr->arguments.size(); ai!=ais; ++ai ) {
                const auto & argT = expr->arguments[ai]->type;
                if ( argT->isRefOrPointer() && !argT->constant ) {
                    propagateWrite(expr->arguments[ai].get());
                }
            }
        }
    // Invoke
        virtual void preVisit(ExprInvoke * expr) override{
            Visitor::preVisit(expr);
            if ( func ) {
                func->sideEffectFlags |= uint32_t(SideEffects::invoke);
            }
            for ( size_t ai=0, ais=expr->arguments.size(); ai!=ais; ++ai ) {
                const auto & argT = expr->arguments[ai]->type;
                if ( argT->isRefOrPointer() && !argT->constant ) {
                    propagateWrite(expr->arguments[ai].get());
                }
            }
        }
    // Debug
        virtual void preVisit(ExprDebug * expr) override {
            Visitor::preVisit(expr);
            if (func) {
                func->sideEffectFlags |= uint32_t(SideEffects::modifyExternal);
            }
        }
    // MemZero
        virtual void preVisit ( ExprMemZero * expr ) override {
            Visitor::preVisit(expr);
            propagateWrite(expr->arguments[0].get());
        }
    // make array
        virtual void preVisit ( ExprMakeArray * expr ) override {
            Visitor::preVisit(expr);
            if (!expr->values.empty()) {
                const bool canCopy = expr->values[0]->type->canCopy();
                for (auto value : expr->values) {
                    if (canCopy) {
                        propagateRead(value.get());
                    } else {
                        propagateWrite(value.get());
                    }
                }
            }
        }
    // make tuple
        virtual void preVisit ( ExprMakeTuple * expr ) override {
            Visitor::preVisit(expr);
            for (auto value : expr->values) {
                if (value->type->canCopy()) {
                    propagateRead(value.get());
                } else {
                    propagateWrite(value.get());
                }
            }
        }
    // MakeStruct
        virtual void preVisit ( ExprMakeStruct * expr ) override {
            Visitor::preVisit(expr);
            for (auto st : expr->structs) {
                for (auto mfd : *st) {
                    if (mfd->moveSemantics) {
                        propagateWrite(mfd->value.get());
                    } else {
                        propagateRead(mfd->value.get());
                    }
                }
            }
        }
    // make variant
        virtual void preVisit ( ExprMakeVariant * expr ) override {
            Visitor::preVisit(expr);
            for (auto mfd : expr->variants) {
                 if (mfd->moveSemantics) {
                    propagateWrite(mfd->value.get());
                } else {
                    propagateRead(mfd->value.get());
                }
            }
        }
    // addr
        virtual void preVisit ( ExprAddr * expr ) override {
            Visitor::preVisit(expr);
            expr->func->addressTaken = true;
        }
    };


    class RemoveUnusedLocalVariables : public PassVisitor {
    protected:
        virtual bool canVisitStructureFieldInit ( Structure * ) override { return false; }
        virtual bool canVisitArgumentInit ( Function * , const VariablePtr &, Expression * ) override { return false; }
        virtual bool canVisitQuoteSubexpression ( ExprQuote * ) override { return false; }
    // ExprLet
        virtual VariablePtr visitLet ( ExprLet * let, const VariablePtr & var, bool last ) override {
            if ( !var->access_get && !var->access_ref && !var->access_init && !var->access_pass ) {
                reportFolding();
                return nullptr;
            }
            if ( !var->access_ref && !var->access_pass && var->init && var->init->constexpression ) {
                reportFolding();
                return nullptr;
            }
            if ( !var->access_ref && !var->access_pass && !var->init && var->type->isFoldable() ) {
                // uninitialized read-only foldable var is const 0
                reportFolding();
                return nullptr;
            }
            if ( !var->access_ref && !var->access_pass && !var->access_get && var->init->noSideEffects ) {
                reportFolding();
                return nullptr;
            }
            return Visitor::visitLet(let,var,last);
        }
    // ExprVar
        virtual ExpressionPtr visit ( ExprVar * expr ) override {
            if ( !expr->variable->access_ref && !expr->variable->access_extern ) {
                if ( expr->r2v ) {
                    if ( expr->variable->init ) {
                        if ( expr->variable->init->constexpression ) {
                            if ( !expr->isGlobalVariable() || expr->variable->type->isConst() ) {
                                reportFolding();
                                auto cle = expr->variable->init->clone();
                                if ( !cle->type ) {
                                    cle->type = make_smart<TypeDecl>(*expr->variable->init->type);
                                }
                                return cle;
                            }
                        }
                    } else {
                        if ( expr->type->isFoldable() && !expr->variable->access_init && (expr->variable->type->constant || !expr->isGlobalVariable()) ) {
                            if ( expr->type->isEnumT() ) {
                                auto cfv = expr->type->enumType->find(0, "");
                                if ( !cfv.empty() ) {
                                    reportFolding();
                                    auto exprV = make_smart<ExprConstEnumeration>(expr->at, cfv, make_smart<TypeDecl>(*expr->type));
                                    exprV->type = expr->type->enumType->makeEnumType();
                                    exprV->type->constant = true;
                                    exprV->value = v_zero();
                                    return exprV;
                                }
                            } else if ( expr->type->baseType==Type::tString ) {
                                reportFolding();
                                auto exprV = make_smart<ExprConstString>(expr->at);
                                exprV->type = make_smart<TypeDecl>(Type::tString);
                                exprV->type->constant = true;
                                return exprV;
                            } else {
                                reportFolding();
                                auto exprV = Program::makeConst(expr->at, expr->type, v_zero());
                                exprV->type = make_smart<TypeDecl>(*expr->type);
                                exprV->type->constant = true;
                                return exprV;
                            }
                        }
                    }
                }
            }
            return Visitor::visit(expr);
        }
    // ExprFor
        virtual ExpressionPtr visit ( ExprFor * expr ) override {
            // TODO: how do we determine, if iteration count is not used?
            //  also, how do we determine, if native iterator has side-effect?
            if ( expr->allowIteratorOptimization ) {
                auto itI = expr->iterators.begin();
                auto itA = expr->iteratorsAt.begin();
                auto itV = expr->iteratorVariables.begin();
                auto itS = expr->sources.begin();
                while ( itV != expr->iteratorVariables.end() ) {
                    auto & var = *itV;
                    if ( !var->access_ref && !var->access_get && (expr->sources.size()>1) ) {   // we need to leave at least 1 variable
                        itI = expr->iterators.erase(itI);
                        itA = expr->iteratorsAt.erase(itA);
                        itV = expr->iteratorVariables.erase(itV);
                        itS = expr->sources.erase(itS);
                        reportFolding();
                    } else {
                        itI ++;
                        itA ++;
                        itV ++;
                        itS ++;
                    }
                }
            }
            return Visitor::visit(expr);
        }
    };

    // program

    void Program::buildAccessFlags(TextWriter &) {
        markSymbolUse(true,false,true,nullptr);
        // determine function side-effects
        TrackFieldAndAtFlags faf;
        faf.MarkSideEffects(*thisModule);
    }

    bool Program::optimizationUnused(TextWriter & logs) {
        buildAccessFlags(logs);
        // remove itselft
        RemoveUnusedLocalVariables context;
        visit(context);
        return context.didAnything();
    }
}

