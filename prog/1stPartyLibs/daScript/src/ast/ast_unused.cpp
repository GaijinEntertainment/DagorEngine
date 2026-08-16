#include "daScript/misc/platform.h"

#include "daScript/ast/ast.h"
#include "daScript/ast/ast_visitor.h"

namespace das {


    class TrackVariableFlags : public Visitor {
    protected:
        virtual bool canVisitFunction ( Function * fun ) override {
            return !fun->stub && !fun->isTemplate;    // we don't do a thing with templates
        }
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

    // temp-string-result propagation: a das function is a fresh-string producer iff every
    // function-level return yields a fresh producer - a [temp_string_result] call, a string
    // builder, or a ternary of qualifying branches. No locals in v1: `return s` never
    // qualifies, which buys the no-retention property without escape analysis
    static bool isFreshStringExpr ( Expression * expr ) {
        if ( !expr ) return false;
        if ( expr->rtti_isStringBuilder() ) return true;
        if ( expr->rtti_isCall() ) {
            auto c = static_cast<ExprCall *>(expr);
            return c->func && c->func->tempStringResult;
        }
        if ( expr->rtti_isOp3() ) {
            auto op3 = static_cast<ExprOp3 *>(expr);
            return isFreshStringExpr(op3->left) && isFreshStringExpr(op3->right);
        }
        return false;
    }

    class CheckFreshStringReturns : public Visitor {
    public:
        bool allFresh = true;
        // runs on one pre-gated (non-template, non-stub) function; inits cannot hold
        // function-level returns and quotes are inert - skip both outright
        virtual bool canVisitStructureFieldInit ( Structure * ) override { return false; }
        virtual bool canVisitArgumentInit ( Function * , const VariablePtr &, Expression * ) override { return false; }
        virtual bool canVisitQuoteSubexpression ( ExprQuote * ) override { return false; }
        virtual void preVisit ( ExprReturn * expr ) override {
            Visitor::preVisit(expr);
            if ( expr->returnInBlock ) return;      // a block return yields the block, not the function
            if ( !isFreshStringExpr(expr->subexpr) ) allFresh = false;
        }
    };

    // here we propagate r2cr flag
    //  a.@b    ->  $a.@b
    //  a@[b]   ->  $a@[b]
    //  a.b.@c  ->  $a.$b.@c
    //  a = 5   ->  #a = 5
    //  a.b = 5 ->  #a#.b=5
    //  a[b]=3  ->  #a#[b]=3
    class TrackFieldAndAtFlags : public Visitor {
        das_hash_set<const Function *>   asked;
        FunctionPtr             func = nullptr;
    public:
    // access_info is informational only (lint/refactor consumers) — reset here, not in
    // TrackVariableFlags, so the phase-2 access_flags rebuild can't interleave with it
        virtual void preVisitLet ( ExprLet * let, const VariablePtr & var, bool last ) override {
            Visitor::preVisitLet(let, var, last);
            var->access_info = 0;
        }
        virtual void preVisitArgument ( Function * fn, const VariablePtr & var, bool lastArg ) override {
            Visitor::preVisitArgument(fn, var, lastArg);
            var->access_info = 0;
        }
        virtual void preVisitBlockArgument ( ExprBlock * block, const VariablePtr & var, bool lastArg ) override {
            Visitor::preVisitBlockArgument(block, var, lastArg);
            var->access_info = 0;
        }
        void MarkSideEffects ( Module & mod ) {
            // function bodies stamp access_info on globals too (preVisitLet only covers locals
            // and arguments) — clear here or the bits go sticky across buildAccessFlags rounds
            for ( auto & var : mod.globals.each() ) {
                var->access_info = 0;
            }
            for ( auto & fn : mod.functions.each() ) {
                if (!fn->isTemplate && !fn->builtIn) {
                    fn->knownSideEffects = false;
                    fn->sideEffectFlags &= ~uint32_t(SideEffects::inferredSideEffects);
                }
            }
            for ( auto & fn : mod.functions.each() ) {
                if (!fn->isTemplate && !fn->builtIn && !fn->knownSideEffects) {
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
            if ( at->subexpr->type->isHandle() && at->subexpr->type->annotation->isIndexMutable(at->index->type) ) {
                propagateWrite(at->subexpr);
            } else if ( at->subexpr->type->isGoodTableType() ) {
                propagateWrite(at->subexpr);  // note: this makes it so tab[foo] modifies itself
            } else {
                propagateRead(at->subexpr);
            }
            propagateRead(at->index);
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
                if ( var->variable->loop_source ) {
                    propagateRead(var->variable->loop_source);
                }
            } else if ( expr->rtti_isField() || expr->rtti_isSafeField()
                       || expr->rtti_isAsVariant() || expr->rtti_isIsVariant()
                       || expr->rtti_isSafeAsVariant() ) {
                auto field = (ExprField *) expr;
                field->r2cr = true;
                propagateRead(field->value);
                if ( func && field->value->type->isPointer() ) func->sideEffectFlags |= uint32_t(SideEffects::accessExternal);
            } else if ( expr->rtti_isSwizzle() ) {
                auto swiz = (ExprSwizzle *) expr;
                swiz->r2cr = true;
                propagateRead(swiz->value);
            } else if ( expr->rtti_isAt() ) {
                auto at = (ExprAt *) expr;
                at->r2cr = true;
                propagateAt(at);
            } else if ( expr->rtti_isSafeAt() ) {
                auto at = (ExprSafeAt *) expr;
                at->r2cr = true;
                propagateRead(at->subexpr);
                propagateRead(at->index);
            } else if ( expr->rtti_isOp3() ) {
                auto op3 = (ExprOp3 *) expr;
                propagateRead(op3->left);
                propagateRead(op3->right);
            } else if ( expr->rtti_isNullCoalescing() ) {
                auto nc = (ExprNullCoalescing *) expr;
                propagateRead(nc->subexpr);
                propagateRead(nc->defaultValue);
            } else if ( expr->rtti_isCast() ) {
                auto ca = (ExprCast *) expr;
                if ( isConstCast(ca->castType) ) {
                    propagateRead(ca->subexpr);
                } else {
                    propagateWrite(ca->subexpr);
                }
            } else if ( expr->rtti_isRef2Ptr() ) {
                auto rr = (ExprRef2Ptr *)expr;
                propagateRead(rr->subexpr);
            } else if ( expr->rtti_isPtr2Ref() ) {
                auto rr = (ExprPtr2Ref *)expr;
                propagateRead(rr->subexpr);
                if ( func ) func->sideEffectFlags |= uint32_t(SideEffects::accessExternal);
            } else if ( expr->rtti_isR2V() ) {
                auto rr = (ExprRef2Value *)expr;
                propagateRead(rr->subexpr);
            } else if ( expr->rtti_isUnsafe() ) {
                propagateRead(((ExprUnsafe *) expr)->body);
            }
        }
        // a write through a local pointer variable is a write through whatever storage its
        // initializer aliases — chase the init so the write reaches the true root (argument,
        // addr(...), another alias). without this a `let q = p; write-through-q` function is
        // judged pure and its calls are DCE'd, silently dropping the write (#3311 family);
        // auto-inline's generated `let _inl*_arg_*` bindings are exactly this shape.
        // conservative on purpose: a rebinding write (`q = null`) chases too — that only
        // over-marks, never drops. globals are excluded (covered by accessGlobal tracking).
        void propagateWriteThroughPointerAlias ( ExprVar * var ) {
            const auto & vv = var->variable;
            if ( !vv || vv->global ) return;
            const auto & vt = vv->type;
            if ( !vt || !vt->isPointer() || vt->ref ) return;
            if ( vv->init ) propagateWrite(vv->init);
        }
        void propagateWrite ( Expression * expr ) {
            if ( expr->rtti_isVar() ) {
                auto var = (ExprVar *) expr;
                var->write = true;
                if ( var->variable->loop_source ) {
                    propagateWrite(var->variable->loop_source);
                }
                propagateWriteThroughPointerAlias(var);
            } else if ( expr->rtti_isField() || expr->rtti_isSafeField()
                       || expr->rtti_isAsVariant() || expr->rtti_isSafeAsVariant() ) {
                auto field = (ExprField *) expr;
                //if ( !field->value->type->isPointer() ) {
                    field->write = true;
                    propagateWrite(field->value);
                //} else {
                //    propagateRead(field->value);
                //}
                if ( func && field->value->type->isPointer() ) func->sideEffectFlags |= uint32_t(SideEffects::modifyExternal);
            } else if ( expr->rtti_isSwizzle() ) {
                auto swiz = (ExprSwizzle *) expr;
                swiz->write = true;
                propagateWrite(swiz->value);
            } else if ( expr->rtti_isAt() || expr->rtti_isSafeAt() ) {
                auto at = (ExprAt *) expr;
                at->write = true;
                propagateWrite(at->subexpr);
            } else if ( expr->rtti_isOp3() ) {
                auto op3 = (ExprOp3 *) expr;
                propagateWrite(op3->left);
                propagateWrite(op3->right);
            } else if ( expr->rtti_isNullCoalescing() ) {
                auto nc = (ExprNullCoalescing *) expr;
                propagateWrite(nc->subexpr);
                propagateWrite(nc->defaultValue);
            } else if ( expr->rtti_isCast() ) {
                auto ca = (ExprCast *) expr;
                propagateWrite(ca->subexpr);
            } else if ( expr->rtti_isRef2Ptr() ) {
                auto rr = (ExprRef2Ptr *)expr;
                propagateWrite(rr->subexpr);
            } else if ( expr->rtti_isPtr2Ref() ) {
                auto rr = (ExprPtr2Ref *)expr;
                propagateWrite(rr->subexpr);
                if ( func ) func->sideEffectFlags |= uint32_t(SideEffects::modifyExternal);
            } else if ( expr->rtti_isR2V() ) {
                auto rr = (ExprRef2Value *)expr;
                propagateWrite(rr->subexpr);
            } else if ( expr->rtti_isCallFunc() ) {
                auto call = (ExprCallFunc *) expr;
                // firstArgReturnType (pointer arithmetic: i_das_ptr_add/sub/etc) returns a
                // pointer aliasing arguments[0]'s pointee, so a write through the result is a
                // write through arguments[0]. Without this the write is lost across an
                // offset-pointer helper call and the caller is wrongly judged pure (issue #3321)
                if ( call->func && (call->func->propertyFunction || call->func->isCustomProperty
                                    || call->func->firstArgReturnType) ) {
                    propagateWrite(call->arguments[0]);
                }
            } else if ( expr->rtti_isUnsafe() ) {
                propagateWrite(((ExprUnsafe *) expr)->body);
            }
        }
        void propagateWriteViaCopyOrMove ( Expression * expr ) {
            if ( expr->rtti_isVar() ) {
                auto var = (ExprVar *) expr;
                var->write = true;
                if ( var->variable->loop_source ) {
                    propagateWrite(var->variable->loop_source);    /// this went to variable, we done via copy or move
                }
                propagateWriteThroughPointerAlias(var);
            } else if ( expr->rtti_isField() || expr->rtti_isSafeField()
                       || expr->rtti_isAsVariant() || expr->rtti_isSafeAsVariant() ) {
                auto field = (ExprField *) expr;
                //if ( !field->value->type->isPointer() ) {
                    field->write = true;
                    propagateWriteViaCopyOrMove(field->value);
                //} else {
                //    propagateRead(field->value);
                //}
                if ( func && field->value->type->isPointer() ) func->sideEffectFlags |= uint32_t(SideEffects::modifyExternal);
            } else if ( expr->rtti_isSwizzle() ) {
                auto swiz = (ExprSwizzle *) expr;
                swiz->write = true;
                propagateWriteViaCopyOrMove(swiz->value);
            } else if ( expr->rtti_isAt() || expr->rtti_isSafeAt() ) {
                auto at = (ExprAt *) expr;
                at->write = true;
                propagateWriteViaCopyOrMove(at->subexpr);
            } else if ( expr->rtti_isOp3() ) {
                auto op3 = (ExprOp3 *) expr;
                propagateWriteViaCopyOrMove(op3->left);
                propagateWriteViaCopyOrMove(op3->right);
            } else if ( expr->rtti_isNullCoalescing() ) {
                auto nc = (ExprNullCoalescing *) expr;
                propagateWriteViaCopyOrMove(nc->subexpr);
                propagateWriteViaCopyOrMove(nc->defaultValue);
            } else if ( expr->rtti_isCast() ) {
                auto ca = (ExprCast *) expr;
                propagateWriteViaCopyOrMove(ca->subexpr);
            } else if ( expr->rtti_isRef2Ptr() ) {
                auto rr = (ExprRef2Ptr *)expr;
                propagateWriteViaCopyOrMove(rr->subexpr);
            } else if ( expr->rtti_isPtr2Ref() ) {
                auto rr = (ExprPtr2Ref *)expr;
                propagateWriteViaCopyOrMove(rr->subexpr);
                if ( func ) func->sideEffectFlags |= uint32_t(SideEffects::modifyExternal);
            } else if ( expr->rtti_isR2V() ) {
                auto rr = (ExprRef2Value *)expr;
                propagateWriteViaCopyOrMove(rr->subexpr);
            } else if ( expr->rtti_isCallFunc() ) {
                auto call = (ExprCallFunc *) expr;
                call->write = true;
                if ( call->func && (call->func->propertyFunction || call->func->isCustomProperty
                                    || call->func->firstArgReturnType) ) {
                    propagateWriteViaCopyOrMove(call->arguments[0]);
                }
            } else if ( expr->rtti_isUnsafe() ) {
                propagateWriteViaCopyOrMove(((ExprUnsafe *) expr)->body);
            }
        }
        // informational: argument appeared in a mutable-ref slot. peels the same shapes as
        // propagateWrite, but stamps the root variable directly (no expression-level flag)
        void propagatePassMutable ( Expression * expr ) {
            if ( expr->rtti_isVar() ) {
                auto var = (ExprVar *) expr;
                var->variable->access_info_pass_mutable = true;
                if ( var->variable->loop_source ) {
                    propagatePassMutable(var->variable->loop_source);
                }
            } else if ( expr->rtti_isField() || expr->rtti_isSafeField()
                       || expr->rtti_isAsVariant() || expr->rtti_isSafeAsVariant() ) {
                propagatePassMutable(((ExprField *) expr)->value);
            } else if ( expr->rtti_isSwizzle() ) {
                propagatePassMutable(((ExprSwizzle *) expr)->value);
            } else if ( expr->rtti_isAt() || expr->rtti_isSafeAt() ) {
                propagatePassMutable(((ExprAt *) expr)->subexpr);
            } else if ( expr->rtti_isOp3() ) {
                auto op3 = (ExprOp3 *) expr;
                propagatePassMutable(op3->left);
                propagatePassMutable(op3->right);
            } else if ( expr->rtti_isNullCoalescing() ) {
                auto nc = (ExprNullCoalescing *) expr;
                propagatePassMutable(nc->subexpr);
                propagatePassMutable(nc->defaultValue);
            } else if ( expr->rtti_isCast() ) {
                propagatePassMutable(((ExprCast *) expr)->subexpr);
            } else if ( expr->rtti_isRef2Ptr() ) {
                propagatePassMutable(((ExprRef2Ptr *) expr)->subexpr);
            } else if ( expr->rtti_isPtr2Ref() ) {
                propagatePassMutable(((ExprPtr2Ref *) expr)->subexpr);
            } else if ( expr->rtti_isR2V() ) {
                propagatePassMutable(((ExprRef2Value *) expr)->subexpr);
            } else if ( expr->rtti_isCallFunc() ) {
                auto call = (ExprCallFunc *) expr;
                if ( call->func && (call->func->propertyFunction || call->func->isCustomProperty
                                    || call->func->firstArgReturnType) ) {
                    propagatePassMutable(call->arguments[0]);
                }
            } else if ( expr->rtti_isUnsafe() ) {
                propagatePassMutable(((ExprUnsafe *) expr)->body);
            }
        }
        void markPassMutableArguments ( const Function * fn, const vector<ExpressionPtr> & arguments ) {
            // unlike the modifyArgument loops below this runs on every resolved call — guard
            // against shapes where the argument list and the signature disagree in length
            for ( size_t ai=0, ais=das::min(arguments.size(), fn->arguments.size()); ai!=ais; ++ai ) {
                const auto & argT = fn->arguments[ai]->type;
                if ( argT->isRef() && !argT->constant ) {
                    propagatePassMutable(arguments[ai]);
                }
            }
        }
        void markPassMutableOperand ( const Function * fn, size_t index, Expression * operand ) {
            if ( index >= fn->arguments.size() ) return;
            const auto & argT = fn->arguments[index]->type;
            if ( argT->isRef() && !argT->constant ) {
                propagatePassMutable(operand);
            }
        }
        // a modifyArgument callee writes through SOME argument — mark which of the call-site
        // arguments inherit that write (so the caller's own modifyArgument / DCE is correct)
        void propagateModifiedArguments ( const Function * fn, const vector<ExpressionPtr> & arguments, bool inCycle ) {
            // bound by the signature too (matching markPassMutableArguments) — argVar indexes
            // fn->arguments[ai], so a call shape with more args than params can't go OOB
            for ( size_t ai=0, ais=das::min(arguments.size(), fn->arguments.size()); ai!=ais; ++ai ) {
                const auto & argVar = fn->arguments[ai];
                const auto & argT = argVar->type;
                if ( inCycle ) {
                    // recursive callee still mid-analysis: conservative signature rule
                    if ( argT->canWrite() ) propagateWrite(arguments[ai]);
                } else if ( !fn->builtIn ) {
                    // canWrite() is false for a `const`-pointee/`const`-ref argument, but a callee
                    // can still write through it by const-stripping (reinterpret to mutable). Its
                    // per-argument access_ref records that real write, so honor it — otherwise the
                    // forwarded write is invisible and the call is wrongly DCE'd (issue #3311).
                    const bool argWritten = argT->canWrite()
                        || ( argVar->access_ref && argT->isRefOrPointer() );
                    if ( fn->knownSideEffects && argWritten ) propagateWrite(arguments[ai]);
                } else {
                    if ( argT->canWrite() && fn->modifyArgument ) propagateWrite(arguments[ai]);
                }
            }
        }
        // a pointer value copied out of a const variable is itself const (`Foo? const`) and no
        // longer matches a mutable-pointee destination (error 30915/30343) — flowing a pointer
        // into such a slot demands the source variable stay mutable. const-pointer (`Foo? const`),
        // void?, and const-pointee (`Foo const?`) slots all accept a const source, so they don't
        void markPassMutablePointerSink ( const TypeDeclPtr & slotType, Expression * source ) {
            if ( !slotType || slotType->baseType!=Type::tPointer || slotType->constant ) return;
            if ( !slotType->firstType || slotType->firstType->baseType==Type::tVoid ) return;
            if ( slotType->firstType->constant ) return;
            propagatePassMutable(source);
        }
        // make-struct/variant/tuple/array destinations are not worth a per-field type lookup —
        // conservatively treat any pointer value flowing into one as a mutable-pointee sink
        void markPassMutablePointerFlow ( Expression * source ) {
            const auto & st = source->type;
            if ( !st || st->baseType!=Type::tPointer ) return;
            propagatePassMutable(source);
        }
        uint32_t getSideEffects ( const FunctionPtr & fnc ) {
            if ( fnc->stub || fnc->isTemplate || fnc->builtIn || fnc->knownSideEffects ) {
                return fnc->sideEffectFlags;
            }
            if ( asked.find(fnc)!=asked.end() ) {
                return fnc->sideEffectFlags;   // assume no extra side effects
            }
            asked.insert(fnc);
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
            // temp-string-result: computed bottom-up (callees resolved by the dep loop above,
            // pessimistic on cycles via `asked`); the [temp_string_result] annotation is a
            // manual override and is never cleared here
            if ( !fnc->tempStringResult && fnc->result && fnc->result->isString() && !fnc->result->ref ) {
                CheckFreshStringReturns cfr;
                fnc->visit(cfr);
                if ( cfr.allFresh ) fnc->tempStringResult = true;
            }
            // may-queue-temp-string: executing this body may hit a queue site - a builder that
            // marking may make temp, a call that wrapping may make a site, or (via invoke) code
            // we cannot see. A caller holding a PARKED temp across such a call gets it flushed
            // while live - the interprocedural half of the one-site rule. Bottom-up like
            // captureString; same cycle exposure (the `asked` early-return under-reports)
            if ( !fnc->mayQueueTempString ) {
                bool mayQueue = fnc->hasStringBuilder || (flags & uint32_t(SideEffects::invoke)) != 0;
                if ( !mayQueue ) {
                    for ( auto & depF : fnc->useFunctions ) {
                        if ( depF == fnc ) continue;
                        if ( depF->mayQueueTempString || depF->tempStringResult
                            || (depF->builtIn && depF->invoke) ) {
                            mayQueue = true;
                            break;
                        }
                    }
                }
                if ( mayQueue ) fnc->mayQueueTempString = true;
            }
            fnc->sideEffectFlags |= flags;
            return flags;
        }
    protected:
        virtual bool canVisitFunction ( Function * fun ) override {
            return !fun->stub && !fun->isTemplate;    // we don't do a thing with templates
        }
        virtual bool canVisitStructureFieldInit ( Structure * ) override { return false; }
        virtual bool canVisitArgumentInit ( Function * , const VariablePtr &, Expression * ) override { return false; }
        virtual bool canVisitQuoteSubexpression ( ExprQuote * ) override { return false; }
    // Variable initializatoin
        virtual void preVisitLetInit ( ExprLet * let, const VariablePtr & var, Expression * init ) override {
            Visitor::preVisitLetInit(let, var, init);
            if ( var->init_via_move ) {
                propagateWrite(init);
            } else if ( var->type->ref ) {
                // TODO:
                //  at some point we should do better data trackng for this type of aliasing
                propagateWrite(init);
            } else {
                propagateRead(init);
            }
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
            propagateRead(expr->value);
        }
    // ExprSafeField
        virtual void preVisit ( ExprSafeField * expr ) override {
            Visitor::preVisit(expr);
            propagateRead(expr->value);
        }
    // ExprIsVariant
        virtual void preVisit ( ExprIsVariant * expr ) override {
            Visitor::preVisit(expr);
            propagateRead(expr->value);
        }
    // ExprAsVariant
        virtual void preVisit ( ExprAsVariant * expr ) override {
            Visitor::preVisit(expr);
            propagateRead(expr->value);
        }
    // ExprSafeAsVariant
        virtual void preVisit ( ExprSafeAsVariant * expr ) override {
            Visitor::preVisit(expr);
            propagateRead(expr->value);
        }
    // ExprAt
        virtual void preVisit ( ExprAt * expr ) override {
            Visitor::preVisit(expr);
            propagateAt(expr);
        }
    // ExprSafeAt
        virtual void preVisit ( ExprSafeAt * expr ) override {
            Visitor::preVisit(expr);
            propagateRead(expr->subexpr);
        }
    // ExprMove
        virtual void preVisit ( ExprMove * expr ) override {
            Visitor::preVisit(expr);
            propagateWriteViaCopyOrMove(expr->left);
            propagateWrite(expr->right);
        }
    // ExprCopy
        virtual void preVisit ( ExprCopy * expr ) override {
            Visitor::preVisit(expr);
            propagateWriteViaCopyOrMove(expr->left);
            propagateRead(expr->right);
            markPassMutablePointerSink(expr->left->type, expr->right);
        }
    // ExprClone
        virtual void preVisit ( ExprClone * expr ) override {
            Visitor::preVisit(expr);
            propagateWrite(expr->left);
            propagateRead(expr->right);
            markPassMutablePointerSink(expr->left->type, expr->right);
        }
    // Op1
        virtual void preVisit ( ExprOp1 * expr ) override {
            Visitor::preVisit(expr);
            auto sef = getSideEffects(expr->func);
            markPassMutableOperand(expr->func, 0, expr->subexpr);
            if ( sef & uint32_t(SideEffects::modifyArgument) ) {
                propagateWrite(expr->subexpr);
            }
        }
    // Op2
        virtual void preVisit ( ExprOp2 * expr ) override {
            Visitor::preVisit(expr);
            auto sef = getSideEffects(expr->func);
            markPassMutableOperand(expr->func, 0, expr->left);
            markPassMutableOperand(expr->func, 1, expr->right);
            if ( sef & uint32_t(SideEffects::modifyArgument) ) {
                auto leftT = expr->left->type;
                if ( leftT->isRefOrPointer() && !leftT->constant ) {
                    propagateWrite(expr->left);
                }
                auto rightT = expr->right->type;
                if ( rightT->isRefOrPointer() && !rightT->constant ) {
                    propagateWrite(expr->right);
                }
            }
        }
    // Op3
        virtual void preVisit ( ExprOp3 * expr ) override {
            Visitor::preVisit(expr);
            auto sef = expr->func ? getSideEffects(expr->func) : 0;
            if ( expr->func ) {
                markPassMutableOperand(expr->func, 0, expr->subexpr);
                markPassMutableOperand(expr->func, 1, expr->left);
                markPassMutableOperand(expr->func, 2, expr->right);
            }
            if ( sef & uint32_t(SideEffects::modifyArgument) ) {
                auto condT = expr->subexpr->type;
                if ( condT->isRefOrPointer() && !condT->constant ) {
                    propagateWrite(expr->subexpr);
                }
                auto leftT = expr->left->type;
                if ( leftT->isRefOrPointer() && !leftT->constant ) {
                    propagateWrite(expr->left);
                }
                auto rightT = expr->right->type;
                if ( rightT->isRefOrPointer() && !rightT->constant ) {
                    propagateWrite(expr->right);
                }
            }
        }
    // Return
        virtual void preVisit ( ExprReturn * expr ) override {
            Visitor::preVisit(expr);
            // TODO:
            //  at some point we should do better data trackng for this type of aliasing
            if ( expr->returnReference || expr->moveSemantics ) {
                propagateWrite(expr->subexpr);
            } else if ( expr->subexpr ) {
                propagateRead(expr->subexpr);
                if ( func ) markPassMutablePointerSink(func->result, expr->subexpr);
            }
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
                markPassMutableArguments(expr->func, expr->arguments);
                // if modified, modify CALL
                auto sef = getSideEffects(expr->func);
                // see ExprCall: a recursive initializer call is still mid-analysis here
                const bool inCycle = !expr->func->knownSideEffects
                    && asked.find(expr->func) != asked.end();
                if ( inCycle || (sef & uint32_t(SideEffects::modifyArgument)) ) {
                    propagateModifiedArguments(expr->func, expr->arguments, inCycle);
                }
            }
        }
    // Delete
        virtual void preVisit ( ExprDelete * expr ) override {
            Visitor::preVisit(expr);
            propagateWrite(expr->subexpr);
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
            // finalize calls are desugared from ExprDelete — always propagate write
            if ( expr->func && expr->func->name == "finalize" && expr->arguments.size()==1 ) {
                propagateWrite(expr->arguments[0]);
            }
            // call with no resolved function — nothing to propagate; the typer
            // either left this as an unresolved generic (error elsewhere) or it's
            // a desugar artifact carrying no side-effect contract.
            if ( !expr->func ) {
                return;
            }
            markPassMutableArguments(expr->func, expr->arguments);
            // if modified, modify NEW
            auto sef = getSideEffects(expr->func);
            // a recursive (or mutually-recursive) callee is still mid-analysis here, so
            // its modifyArgument / knownSideEffects are not established yet — fall back to
            // the conservative signature rule (a writable-ref argument may be written),
            // exactly as unresolved calls and invokes already do (issue #3033)
            const bool inCycle = !expr->func->knownSideEffects
                && asked.find(expr->func) != asked.end();
            if ( inCycle || (sef & uint32_t(SideEffects::modifyArgument)) ) {
                propagateModifiedArguments(expr->func, expr->arguments, inCycle);
            }
        }
    // LooksLikeCall
        virtual void preVisit ( ExprLooksLikeCall * expr ) override {
            Visitor::preVisit(expr);
            for ( size_t ai=0, ais=expr->arguments.size(); ai!=ais; ++ai ) {
                const auto & argT = expr->arguments[ai]->type;
                if ( argT->isRefOrPointer() && !argT->constant ) {
                    propagateWrite(expr->arguments[ai]);
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
                    propagateWrite(expr->arguments[ai]);
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
            propagateWrite(expr->arguments[0]);
        }
    // make array
        virtual void preVisit ( ExprMakeArray * expr ) override {
            Visitor::preVisit(expr);
            if (!expr->values.empty()) {
                const bool canCopy = expr->values[0]->type->canCopy();
                for (auto value : expr->values) {
                    if (canCopy) {
                        propagateRead(value);
                    } else {
                        propagateWrite(value);
                    }
                    markPassMutablePointerFlow(value);
                }
            }
        }
    // make tuple
        virtual void preVisit ( ExprMakeTuple * expr ) override {
            Visitor::preVisit(expr);
            for (auto value : expr->values) {
                if (value->type->canCopy()) {
                    propagateRead(value);
                } else {
                    propagateWrite(value);
                }
                markPassMutablePointerFlow(value);
            }
        }
    // MakeStruct
        virtual void preVisit ( ExprMakeStruct * expr ) override {
            Visitor::preVisit(expr);
            for (auto st : expr->structs) {
                for (auto mfd : *st) {
                    if (mfd->moveSemantics) {
                        propagateWrite(mfd->value);
                    } else {
                        propagateRead(mfd->value);
                    }
                    markPassMutablePointerFlow(mfd->value);
                }
            }
        }
    // make variant
        virtual void preVisit ( ExprMakeVariant * expr ) override {
            Visitor::preVisit(expr);
            for (auto mfd : expr->variants) {
                 if (mfd->moveSemantics) {
                    propagateWrite(mfd->value);
                } else {
                    propagateRead(mfd->value);
                }
                markPassMutablePointerFlow(mfd->value);
            }
        }
    // addr
        virtual void preVisit ( ExprAddr * expr ) override {
            Visitor::preVisit(expr);
            expr->func->addressTaken = true;
        }
    // if-then-else
        virtual void preVisit ( ExprIfThenElse * expr ) override {
            Visitor::preVisit(expr);
            propagateRead(expr->cond);
        }
    };


    class RemoveUnusedLocalVariables : public PassVisitor {
    public:
        using PassVisitor::PassVisitor;
        using PassVisitor::visit;
    protected:
        virtual bool canVisitFunction ( Function * fun ) override {
            return funcIsDirty(fun) && !fun->stub && !fun->isTemplate;    // we don't do a thing with templates
        }
        virtual bool canVisitStructure ( Structure * /*st*/ ) override { return false; }
        virtual bool canVisitStructureFieldInit ( Structure * /*var*/ ) override { return false; }
        virtual bool canVisitArgumentInit ( Function * /*fun*/, const VariablePtr & /*var*/, Expression * /*init*/ ) override { return false; }
        virtual bool canVisitQuoteSubexpression ( ExprQuote * /*expr*/ ) override { return false; }
        virtual bool canVisitGlobalVariable ( Variable * /*fun*/ ) override { return false; }
        virtual bool canVisitEnumeration ( Enumeration * /*en*/ ) override { return false; }

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
                                    cle->type = new TypeDecl(*expr->variable->init->type);
                                }
                                // the let coerced the init to the variable's type - a null
                                // literal (void?) into a typed pointer being the canonical
                                // case - and no infer runs after this substitution: the
                                // propagated read must carry the VARIABLE's type, or a
                                // downstream consumer sees void? where float? stood
                                // (SimulateVisitor::visit(ExprSafeAt*) crashed on the null
                                // firstType). isSameType can't gate this - void pointers
                                // match any pointer there - so test the shape directly
                                if ( cle->type->isVoidPointer()
                                    && expr->variable->type->isPointer()
                                    && expr->variable->type->firstType
                                    && !expr->variable->type->firstType->isVoid() ) {
                                    bool wasConst = cle->type->constant;
                                    cle->type = new TypeDecl(*expr->variable->type);
                                    cle->type->ref = false;
                                    cle->type->constant = wasConst;
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
                                    auto exprV = new ExprConstEnumeration(expr->at, cfv, new TypeDecl(*expr->type));
                                    exprV->type = expr->type->enumType->makeEnumType();
                                    exprV->type->constant = true;
                                    exprV->value = v_zero();
                                    return exprV;
                                }
                            } else if ( expr->type->baseType==Type::tString ) {
                                reportFolding();
                                auto exprV = new ExprConstString(expr->at);
                                exprV->type = new TypeDecl(Type::tString);
                                exprV->type->constant = true;
                                return exprV;
                            } else {
                                auto exprV = Program::makeConst(expr->at, expr->type, v_zero());
                                if ( exprV ) {   // null for lattice vectors (not foldable; unreachable belt-and-suspenders)
                                    reportFolding();
                                    exprV->type = new TypeDecl(*expr->type);
                                    exprV->type->constant = true;
                                    return exprV;
                                }
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

    bool Program::optimizationUnused(TextWriter & logs, int32_t round) {
        buildAccessFlags(logs);
        // remove itselft
        RemoveUnusedLocalVariables context(round);
        visit(context);
        return context.didAnything();
    }
}

