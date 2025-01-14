#include "daScript/misc/platform.h"

#include "daScript/ast/ast_generate.h"
#include "daScript/ast/ast_expressions.h"
#include "daScript/ast/ast_visitor.h"

namespace das {

    bool isExpressionVariable(const ExpressionPtr & expr, const string & name) {
        if (expr->rtti_isVar()) {
            auto var = static_pointer_cast<ExprVar>(expr);
            return var->name == name;
        }
        return false;
    }

    bool isExpressionVariableDeref(const ExpressionPtr & expr, const string & name) {
        if (expr->rtti_isVar()) {
            auto var = static_pointer_cast<ExprVar>(expr);
            return var->name == name;
        } else if (expr->rtti_isR2V()) {
            auto r2v = static_pointer_cast<ExprRef2Value>(expr);
            if (r2v->subexpr->rtti_isVar()) {
                auto var = static_pointer_cast<ExprVar>(r2v->subexpr);
                return var->name == name;
            }
        }
        return false;
    }

    bool isExpressionNull(const ExpressionPtr & expr) {
        if (expr->rtti_isConstant() && expr->type->isPointer()) {
            auto cptr = static_pointer_cast<ExprConstPtr>(expr);
            return cptr->getValue() == nullptr;
        }
        return false;
    }

#define VERIFY_GENERATED    0
#define LOG_GENERATED       0

    struct CheckLineInfoVisitor : Visitor {
        virtual void preVisitExpression ( Expression * expr ) override {
            Visitor::preVisitExpression(expr);
            if ( expr->rtti_isFakeContext() || expr->rtti_isFakeLineInfo() ) return;
            DAS_ASSERT(expr->at.column && expr->at.line);
        }
        virtual void preVisit ( Structure * var ) override {
            Visitor::preVisit(var);
            DAS_ASSERT(var->at.column && var->at.line);
        }
        virtual void preVisitStructureField ( Structure * var, Structure::FieldDeclaration & decl, bool last ) override {
            Visitor::preVisitStructureField(var,decl,last);
            DAS_ASSERT(decl.at.column && decl.at.line);
        }
        virtual void preVisitLet ( ExprLet * expr, const VariablePtr & var, bool last ) override {
            Visitor::preVisitLet(expr,var,last);
            DAS_ASSERT(var->at.column && var->at.line);
            DAS_ASSERT(expr->atInit.line);
        }
        virtual void preVisitGlobalLet ( const VariablePtr & var ) override {
            Visitor::preVisitGlobalLet(var);
            DAS_ASSERT(var->at.column && var->at.line);
        }
        virtual void preVisit ( Function * fn ) override {
            Visitor::preVisit(fn);
            DAS_ASSERT(fn->at.column && fn->at.line);
            DAS_ASSERT(fn->atDecl.column && fn->atDecl.line);
        }
        virtual void preVisitArgument ( Function * fn, const VariablePtr & var, bool lastArg ) override {
            Visitor::preVisitArgument(fn, var, lastArg);
            DAS_ASSERT(var->at.column && var->at.line);
        }
        virtual void preVisitBlockArgument ( ExprBlock * block, const VariablePtr & var, bool lastArg ) override {
            Visitor::preVisitBlockArgument(block, var, lastArg);
            DAS_ASSERT(var->at.column && var->at.line);
        }
    };

    void verifyGenerated ( const ExpressionPtr & expr ) {
        (void)expr;
#if LOG_GENERATED
        LOG(LogLevel::trace) << "VERIFY:\n" << *expr << "\n";
#endif
#if VERIFY_GENERATED
        CheckLineInfoVisitor vis;
        expr->visit(vis);
#endif
    }

    ExpressionPtr genComment ( const string & comment ) {
        auto call = make_smart<ExprCall>(LineInfo(), "print");
        call->arguments.push_back(make_smart<ExprConstString>(comment));
        return call;
    }

    struct CheckFullyInferred : Visitor {
        TypeDecl * unInferredType = nullptr;
        virtual void preVisit ( TypeDecl * td ) {
            if ( !unInferredType && td->isAutoOrAlias() ) {
                unInferredType = td;
            }
        }
    };

    TypeDecl *isFullyInferredBlock ( ExprBlock * block ) {
        CheckFullyInferred vis;
        block->visit(vis);
        return vis.unInferredType;
    }

    // array comprehension
    //  invoke( $()
    //      let temp : Array<expr->subexpr->type>
    //      for .....
    //          if where ....
    //              push(temp, subexpr)
    //      return temp
    ExpressionPtr generateComprehension ( ExprArrayComprehension * expr, bool tableSyntax ) {
        auto compName = "__acomp_" + to_string(expr->at.line) + "_" + to_string(expr->at.column);
        auto pClosure = make_smart<ExprBlock>();
        pClosure->at = expr->subexpr->at;
        pClosure->returnType = make_smart<TypeDecl>(Type::autoinfer);
        pClosure->generated = true;
        // temp : Array<expr->subexpr->type>
        auto pVar = make_smart<Variable>();
        pVar->generated = true;
        pVar->at = expr->at;
        pVar->name = compName;
        pVar->type = make_smart<TypeDecl>(Type::tArray);
        pVar->type->constant = false;
        pVar->type->removeConstant = true;
        pVar->type->firstType = make_smart<TypeDecl>(*expr->subexpr->type);
        pVar->type->firstType->ref = false;
        pVar->type->firstType->constant = false;
        if ( expr->subexpr->type->isPointer() && expr->subexpr->type->constant ) {
            pVar->type->firstType->firstType->constant = true;
        }
        // let temp
        auto pLet = make_smart<ExprLet>();
        pLet->at = expr->at;
        pLet->atInit = expr->at;
        pLet->visibility = static_pointer_cast<ExprFor>(expr->exprFor)->visibility;
        pLet->variables.push_back(pVar);
        pClosure->list.push_back(pLet);
        // disable lock check
        auto pSetLockCheck = make_smart<ExprCall>(expr->at, "set_verify_array_locks");
        pSetLockCheck->alwaysSafe = true;
        pSetLockCheck->arguments.push_back(make_smart<ExprVar>(expr->at,compName));
        pSetLockCheck->arguments.push_back(make_smart<ExprConstBool>(false));
        pClosure->list.push_back(pSetLockCheck);
        // push(temp, subexpr)
        auto pPushVal = make_smart<ExprVar>();
        pPushVal->at = expr->at;
        pPushVal->name = compName;
        auto pPush = make_smart<ExprCall>();
        pPush->generated = true;
        pPush->at = expr->at;
        pPush->name = expr->subexpr->type->canCopy() ? "push" : "emplace";
        pPush->arguments.push_back(pPushVal);
        pPush->arguments.push_back(expr->subexpr->clone());
        // for ...
        auto pForBlock = make_smart<ExprBlock>();
        pForBlock->at = expr->at;
        pForBlock->inTheLoop = true;
        if ( expr->exprWhere ) {
            // push block
            auto pPushBlock = make_smart<ExprBlock>();
            pPushBlock->at = expr->at;
            pPushBlock->list.push_back(pPush);
            // for .... if where ... push
            auto pIf = make_smart<ExprIfThenElse>();
            pIf->at = expr->at;
            pIf->cond = expr->exprWhere->clone();
            pIf->if_true = pPushBlock;
            pForBlock->list.push_back(pIf);
        } else {
            // for .... push
            pForBlock->list.push_back(pPush);
        }
        auto pFor = static_pointer_cast<ExprFor>(expr->exprFor->clone());
        pFor->body = pForBlock;
        pClosure->list.push_back(pFor);
        // enable lock check
        auto pResetLockCheck = make_smart<ExprCall>(expr->at, "set_verify_array_locks");
        pResetLockCheck->alwaysSafe = true;
        pResetLockCheck->arguments.push_back(make_smart<ExprVar>(expr->at,compName));
        pResetLockCheck->arguments.push_back(make_smart<ExprConstBool>(true));
        pClosure->list.push_back(pResetLockCheck);
        // return temp
        auto pVal = make_smart<ExprVar>();
        pVal->at = expr->at;
        pVal->name = compName;
        auto pRet = make_smart<ExprReturn>();
        pRet->at = expr->at;
        if ( tableSyntax ) {
            auto ttM = make_smart<ExprCall>(expr->at, "to_table_move");
            ttM->arguments.push_back(pVal);
            pRet->subexpr = ttM;
        } else {
            pRet->subexpr = pVal;
        }
        pRet->moveSemantics = true;
        pRet->fromComprehension = true;
        pRet->skipLockCheck = true;
        pClosure->list.push_back(pRet);
        // make block
        auto pMakeBlock = make_smart<ExprMakeBlock>(expr->at,pClosure);
        // invoke
        auto pInvoke = make_smart<ExprInvoke>(expr->at, "invoke");
        pInvoke->arguments.push_back(pMakeBlock);
        return pInvoke;
    }

    // array comprehension
    //  generator( $()
    //      for .....
    //          if where ....
    //              yield subexpr
    //      return false
    ExpressionPtr generateComprehensionIterator ( ExprArrayComprehension * expr ) {
        auto pClosure = make_smart<ExprBlock>();
        pClosure->at = expr->subexpr->at;
        pClosure->returnType = make_smart<TypeDecl>(Type::autoinfer);
        // yield subexpr
        auto pYield = make_smart<ExprYield>(expr->at, expr->subexpr->clone());
        if ( !expr->subexpr->type->canCopy() ) {
            pYield->moveSemantics = true;
            pYield->skipLockCheck = true;
        }
        // for ...
        auto pForBlock = make_smart<ExprBlock>();
        pForBlock->at = expr->at;
        pForBlock->inTheLoop = true;
        if ( expr->exprWhere ) {
            // yield block
            auto pPushBlock = make_smart<ExprBlock>();
            pPushBlock->at = expr->at;
            pPushBlock->list.push_back(pYield);
            // for .... if where ... yield
            auto pIf = make_smart<ExprIfThenElse>();
            pIf->at = expr->at;
            pIf->cond = expr->exprWhere->clone();
            pIf->if_true = pPushBlock;
            pForBlock->list.push_back(pIf);
        } else {
            // for .... yield
            pForBlock->list.push_back(pYield);
        }
        auto pFor = static_pointer_cast<ExprFor>(expr->exprFor->clone());
        pFor->body = pForBlock;
        pClosure->list.push_back(pFor);
        // return false
        auto pRet = make_smart<ExprReturn>();
        pRet->at = expr->at;
        pRet->subexpr = make_smart<ExprConstBool>(expr->at, false);
        pClosure->list.push_back(pRet);
        // make block
        auto pMakeBlock = make_smart<ExprMakeBlock>(expr->at,pClosure);
        // generator
        auto pMkGen = make_smart<ExprMakeGenerator>(expr->at, pMakeBlock);
        pMkGen->iterType = make_smart<TypeDecl>(*expr->subexpr->type);
        return pMkGen;
    }

    /* a->b(args) is short for invoke(a.b, cast<auto> deref(a), args)  */
    ExprInvoke * makeInvokeMethod ( const LineInfo & at, Expression * a, const string & b ) {
        auto pInvoke = new ExprInvoke(at, "invoke");
        auto pAt = make_smart<ExprField>(at, a->clone(), b);
        pInvoke->arguments.push_back(pAt);
        pInvoke->isInvokeMethod = true;
        auto pCast = make_smart<ExprCast>();
        pCast->at = at;
        pCast->castType = make_smart<TypeDecl>(Type::autoinfer);
        pCast->subexpr = make_smart<ExprPtr2Ref>(at,a);
        pCast->subexpr->alwaysSafe = true;
        pInvoke->arguments.push_back(pCast);
        return pInvoke;
    }

    ExpressionPtr makeDelete ( const VariablePtr & var ) {
        auto eVar = make_smart<ExprVar>(var->at, var->name);
        auto del = make_smart<ExprDelete>(var->at, eVar);
        del->alwaysSafe = true;
        return del;
    }

    // return [[t()]]
    FunctionPtr makeConstructor ( Structure * str, bool isPrivate ) {
        auto fn = make_smart<Function>();
        fn->generated = true;
        fn->privateFunction = isPrivate;
        fn->name = str->name;
        fn->at = fn->atDecl = str->at;
        fn->result = make_smart<TypeDecl>(str);
        if ( str->isClass ) {
            fn->isClassMethod = true;
            fn->classParent = str;
            DAS_ASSERT(fn->classParent);
        }
        if ( str->macroInterface ) fn->macroFunction = true;
        auto block = make_smart<ExprBlock>();
        block->at = str->at;
        auto makeT = make_smart<ExprMakeStruct>(str->at);
        if ( str->isClass ) makeT->alwaysSafe = true;
        makeT->useInitializer = false;
        makeT->nativeClassInitializer = true;
        for ( auto & f : str->fields ) {
            if ( f.init ) {
                makeT->useInitializer = true;
                break;
            }
        }
        makeT->makeType = make_smart<TypeDecl>(str);
        makeT->structs.push_back(make_smart<MakeStruct>());
        auto returnDecl = make_smart<ExprReturn>(str->at,makeT);
        returnDecl->moveSemantics = true;
        block->list.push_back(returnDecl);
        fn->body = block;
        verifyGenerated(fn->body);
        return fn;
    }

    // def clone(a,b:structure)
    //  a.f1 := b.f1
    //  a.f2 := b.f2
    //  ...
    FunctionPtr makeClone ( Structure * str ) {
        auto varA = make_smart<Variable>();
        varA->name = "a";
        varA->type = make_smart<TypeDecl>(str);
        varA->type->isExplicit = true;
        varA->at = str->at;
        auto varB = make_smart<Variable>();
        varB->name = "b";
        varB->type = make_smart<TypeDecl>(str);
        varB->type->constant = str->canCloneFromConst();    // allow to clone from const
        varB->type->implicit = true;
        varB->at = str->at;
        auto fn = make_smart<Function>();
        fn->name = "clone";
        fn->generated = true;
        fn->safeImplicit = true;
        fn->privateFunction = true;
        fn->at = fn->atDecl = str->at;
        fn->result = make_smart<TypeDecl>();
        fn->arguments.push_back(varA);
        fn->arguments.push_back(varB);
        auto block = make_smart<ExprBlock>();
        block->at = str->at;
        for ( auto & fi : str->fields ) {
            auto lA = make_smart<ExprVar>(fi.at, "a");
            auto lAdotF = make_smart<ExprField>(fi.at, lA, fi.name);
            auto lB = make_smart<ExprVar>(fi.at, "b");
            auto lBdotF = make_smart<ExprField>(fi.at, lB, fi.name);
            auto cl = make_smart<ExprClone>(fi.at, lAdotF, lBdotF);
            block->list.push_back(cl);
        }
        fn->body = block;
        verifyGenerated(fn->body);
        return fn;
    }

    void wrapInUnsafe ( const FunctionPtr & func ) {
        auto blk = make_smart<ExprBlock>();
        blk->at = func->body->at;
        auto usa = make_smart<ExprUnsafe>(func->body->at);
        usa->body = func->body;
        blk->list.push_back(usa);
        func->body = blk;
    }

    FunctionPtr generatePointerFinalizer ( const TypeDeclPtr & ptrType, const LineInfo & at ) {
        auto pFunc = make_smart<Function>();
        pFunc->privateFunction = true;
        pFunc->generated = true;
        pFunc->at = pFunc->atDecl = at;
        pFunc->name = "finalize";
        auto THIS0 = make_smart<ExprVar>(at, "__this");
        auto NULLP0 = make_smart<ExprConstPtr>(at);
        auto NEQ = make_smart<ExprOp2>(at, "!=", THIS0, NULLP0);
        auto ifb = make_smart<ExprBlock>();
        ifb->at = at;
        if ( ptrType->firstType && ptrType->firstType->isClass() ) {
            if ( ptrType->firstType->structType->macroInterface ) pFunc->macroFunction = true;
            auto sizvar = make_smart<ExprLet>();              // let __size = class_rtti_size(__this)
            auto vsiz = make_smart<Variable>();
            vsiz->at = at;
            vsiz->name = "__size";
            vsiz->type = make_smart<TypeDecl>(Type::autoinfer);
            vsiz->type->constant = true;
            auto crs = make_smart<ExprCall>(at,"class_rtti_size");
            crs->arguments.push_back(make_smart<ExprVar>(at,"__this"));
            vsiz->init = crs;
            //vsiz->init = make_sm
            sizvar->variables.push_back(vsiz);
            ifb->list.push_back(sizvar);
            auto invk = new ExprInvoke(at, "invoke");           // invoke(__this,__this.__finalize)
            auto THISA = make_smart<ExprVar>(at, "__this");
            auto pAt = make_smart<ExprField>(at, THISA, "__finalize");
            invk->arguments.push_back(pAt);
            auto pCast = make_smart<ExprCast>();
            pCast->at = at;
            pCast->castType = make_smart<TypeDecl>(Type::autoinfer);
            auto THISAA = make_smart<ExprVar>(at, "__this");
            pCast->subexpr = make_smart<ExprPtr2Ref>(at,THISAA);
            pCast->subexpr->alwaysSafe = true;
            invk->arguments.push_back(pCast);
            ifb->list.push_back(invk);
            auto THISA1 = make_smart<ExprVar>(at, "__this");    // delete /*native*/ this, __size
            auto delit1 = make_smart<ExprDelete>(at, THISA1);
            delit1->native = true;
            delit1->sizeexpr = make_smart<ExprVar>(at,"__size");
            ifb->list.push_back(delit1);

        } else {
            auto THISA = make_smart<ExprVar>(at, "__this");     // delete * this
            auto THISR = make_smart<ExprPtr2Ref>(at, THISA);
            auto delit = make_smart<ExprDelete>(at, THISR);
            ifb->list.push_back(delit);
            auto THISA1 = make_smart<ExprVar>(at, "__this");    // delete /*native*/ this
            auto delit1 = make_smart<ExprDelete>(at, THISA1);
            delit1->native = true;
            ifb->list.push_back(delit1);
        }
        auto THISB = make_smart<ExprVar>(at, "__this");    // *THIS = null
        auto NULLP = make_smart<ExprConstPtr>(at);
        auto SETB = make_smart<ExprCopy>(at, THISB, NULLP);
        ifb->list.push_back(SETB);
        auto ife = make_smart<ExprIfThenElse>(at, NEQ, ifb, nullptr);
        auto fb = make_smart<ExprBlock>();
        fb->at = at;
        fb->list.push_back(ife);
        pFunc->body = fb;
        pFunc->result = make_smart<TypeDecl>(Type::tVoid);
        auto cTHIS = make_smart<Variable>();
        cTHIS->name = "__this";
        cTHIS->at = at;
        cTHIS->type = make_smart<TypeDecl>(*ptrType);
        cTHIS->type->constant = false;
        cTHIS->type->removeConstant = true;
        cTHIS->type->ref = true;
        cTHIS->type->removeRef = false;
        cTHIS->type->isExplicit = true;
        pFunc->arguments.push_back(cTHIS);
        wrapInUnsafe(pFunc);
        verifyGenerated(pFunc->body);
        return pFunc;
    }

    FunctionPtr generateStructureFinalizer ( const StructurePtr & ls ) {
        auto pFunc = make_smart<Function>();
        pFunc->privateFunction = true;
        pFunc->generated = true;
        pFunc->at = pFunc->atDecl = ls->at;
        pFunc->name = "finalize";
        if ( ls->isClass ) {
            pFunc->isClassMethod = true;
            pFunc->classParent = ls.get();
            DAS_ASSERT(pFunc->classParent);
        }
        if ( ls->macroInterface ) pFunc->macroFunction = true;
        auto fb = make_smart<ExprBlock>();
        fb->at = ls->at;
        // now finalize
        bool needUnsafe = false;
        for ( const auto & fl : ls->fields ) {
            if ( !fl.type->constant && !fl.capturedConstant && fl.type->needDelete() ) {
                if ( !fl.doNotDelete && !fl.capturedRef ) {
                    if ( fl.type->isPointer() && fl.type->firstType && fl.type->firstType->constant ) continue;
                    if ( ls->isLambda && !fl.type->isSafeToDelete() ) continue; // we don't do unsafe delete for lambda
                    auto fva = make_smart<ExprVar>(fl.at, "__this");
                    auto fld = make_smart<ExprField>(fl.at, fva, fl.name);
                    fld->ignoreCaptureConst = true;
                    auto delf = make_smart<ExprDelete>(fl.at, fld);
                    delf->alwaysSafe = true;
                    fb->list.emplace_back(delf);
                    if ( fl.type->isPointer() ) {
                        needUnsafe = true;
                    }
                }
            }
        }
        auto mz = make_smart<ExprMemZero>(ls->at, "memzero");
        auto lvar = make_smart<ExprVar>(ls->at, "__this");
        mz->arguments.push_back(lvar);
        fb->list.push_back(mz);
        pFunc->body = fb;
        pFunc->result = make_smart<TypeDecl>(Type::tVoid);
        auto cTHIS = make_smart<Variable>();
        cTHIS->at = ls->at;
        cTHIS->name = "__this";
        cTHIS->type = make_smart<TypeDecl>(ls);
        cTHIS->type->isExplicit = true;
        pFunc->arguments.push_back(cTHIS);
        if ( needUnsafe ) {
            wrapInUnsafe(pFunc);
        }
        verifyGenerated(pFunc->body);
        return pFunc;
    }

    FunctionPtr generateLambdaFinalizer ( const string & lambdaName, ExprBlock * block,
                                        const StructurePtr & ls ) {
        auto lfn = lambdaName + "`finalizer";
        auto pFunc = make_smart<Function>();
        pFunc->privateFunction = true;
        pFunc->generated = true;
        pFunc->at = pFunc->atDecl = block->at;
        pFunc->name = lfn;
        auto fb = make_smart<ExprBlock>();
        fb->at = block->at;
        // fb->list.push_back(genComment("delete this lambda\n"));
        if ( block->finalList.size() ) {
            auto with = make_smart<ExprWith>(block->at);
            auto THISVAR = make_smart<ExprVar>(block->at, "__this");
            with->with = make_smart<ExprPtr2Ref>(block->at, THISVAR);
            with->with->generated = true;
            auto bbl = make_smart<ExprBlock>();
            with->body = bbl;
            with->body->at = block->at;
            bbl->list.reserve(block->finalList.size());     // copy finally section of the block body
            for ( auto & subexpr : block->finalList ) {
                bbl->list.push_back(subexpr->clone());
            }
            fb->list.push_back(with);
        }
        // delete * this
        auto THISA = make_smart<ExprVar>(block->at, "__this");
        auto THISAP = make_smart<ExprPtr2Ref>(block->at, THISA);
        auto delit = make_smart<ExprDelete>(block->at, THISAP);
        delit->alwaysSafe = true;
        fb->list.push_back(delit);
        // delete this
        auto THISA1 = make_smart<ExprVar>(block->at, "__this");
        auto delit1 = make_smart<ExprDelete>(block->at, THISA1);
        delit1->native = true;
        delit1->alwaysSafe = true;
        fb->list.push_back(delit1);
        pFunc->body = fb;
        pFunc->result = make_smart<TypeDecl>(Type::tVoid);
        auto cTHIS = make_smart<Variable>();
        cTHIS->at = ls->at;
        cTHIS->name = "__this";
        cTHIS->type = make_smart<TypeDecl>(Type::tPointer);
        cTHIS->type->firstType = make_smart<TypeDecl>(ls);
        cTHIS->type->isExplicit = true;
        pFunc->arguments.push_back(cTHIS);
        // wrapInUnsafe(pFunc);
        verifyGenerated(pFunc->body);
        return pFunc;
    }

    FunctionPtr generateLocalFunction ( const string & lambdaName, ExprBlock * block ) {
        auto lfn = lambdaName + "`function";
        auto pFunc = make_smart<Function>();
        pFunc->generated = true;
        pFunc->at = pFunc->atDecl = block->at;
        pFunc->name = lfn;
        pFunc->body = block->clone();
        pFunc->privateFunction = true;
        auto wb = static_pointer_cast<ExprBlock>(pFunc->body);
        wb->blockFlags = 0;
        wb->arguments.clear();
        wb->returnType.reset();
        pFunc->result = make_smart<TypeDecl>(*block->type);
        for ( auto & arg : block->arguments ) {
            auto cA = arg->clone();
            cA->marked_used = true;
            pFunc->arguments.push_back(cA);
        }
        verifyGenerated(pFunc->body);
        return pFunc;
    }

    bool isCaptureAsRef ( const VariablePtr & var ) {
        return var->capture_as_ref;
    }

    FunctionPtr generateLambdaFunction ( const string & lambdaName, ExprBlock * block,
                                        const StructurePtr & ls, const safe_var_set & capt,
                                        const vector<CaptureEntry> & capture, uint32_t genFlags, Program * thisProgram ) {
        auto lfn = lambdaName + "`function";
        auto pFunc = make_smart<Function>();
        pFunc->lambda = true;
        pFunc->generated = true;
        pFunc->at = pFunc->atDecl = block->at;
        pFunc->name = lfn;
        pFunc->privateFunction = true;
        pFunc->requestJit = (genFlags & generator_jit)!=0;
        pFunc->requestNoJit = (genFlags & generator_nojit)!=0;
        auto fb = make_smart<ExprBlock>();
        fb->at = block->at;
        auto with = make_smart<ExprWith>(block->at);
        with->with = make_smart<ExprVar>(block->at, "__this");
        with->with->generated = true;
        with->body = block->clone();
        static_pointer_cast<ExprBlock>(with->body)->finalList.clear();
        if ( genFlags & generator_needYield ) {
            pFunc->generator = true;
            auto bbl = static_pointer_cast<ExprBlock>(with->body);
            // goto __yeild
            auto gvar = make_smart<ExprVar>(block->at, "__yield");
            auto gexpr = make_smart<ExprGoto>(block->at, static_pointer_cast<Expression>(gvar));
            bbl->list.insert(bbl->list.begin(), gexpr);
            // label "0"
            auto lzero = make_smart<ExprLabel>(block->at, pFunc->totalGenLabel);
            bbl->list.insert(bbl->list.begin() + 1, lzero);
            pFunc->totalGenLabel ++;
        }
        auto wb = static_pointer_cast<ExprBlock>(with->body);
        wb->blockFlags = 0;
        wb->arguments.clear();
        wb->returnType.reset();
        fb->list.push_back(with);
        pFunc->body = fb;
        pFunc->result = make_smart<TypeDecl>(*block->type);
        auto cTHIS = make_smart<Variable>();
        cTHIS->generated = true;
        cTHIS->at = block->at;
        cTHIS->name = "__this";
        cTHIS->type = make_smart<TypeDecl>(ls);
        cTHIS->type->isExplicit = true;
        pFunc->arguments.push_back(cTHIS);
        for ( auto & arg : block->arguments ) {
            auto cA = arg->clone();
            cA->marked_used = true;             // to avoid 'unused argument' error
            pFunc->arguments.push_back(cA);
        }
        for ( auto & var : capt ) {
            CaptureMode mode = CaptureMode::capture_any;
            auto it = find_if ( capture.begin(), capture.end(), [&] ( const auto & entry ){
                return entry.name == var->name;
            });
            if ( it != capture.end() ) {
                mode = it->mode;
            }
            if ( isCaptureAsRef(var) || mode==CaptureMode::capture_by_reference ) {
                replaceRef2Ptr(pFunc->body, var->name);
            }
        }
        thisProgram->library.foreach([&](Module * mod){
            for ( auto & cm : mod->captureMacros ) {
                cm->captureFunction(thisProgram, thisProgram->thisModule.get(), ls.get(), pFunc.get());
            }
            return true;
        },"*");
        verifyGenerated(pFunc->body);
        return pFunc;
    }

    StructurePtr generateLambdaStruct ( const string & lambdaName, ExprBlock * block,
                                       const safe_var_set & capt,
                                       const vector<CaptureEntry> & capture, bool needYield ) {
        auto lsn = lambdaName;
        auto pStruct = make_smart<Structure>(lsn);
        pStruct->generated = true;
        pStruct->isLambda = true;
        pStruct->at = block->at;
        auto btd = block->makeBlockType();
        btd->baseType = Type::tFunction;
        btd->constant = false;
        auto thisArg = make_smart<TypeDecl>(pStruct);
        btd->argTypes.insert(btd->argTypes.begin(), thisArg);
        btd->argNames.insert(btd->argNames.begin(), "__this");
        pStruct->fields.emplace_back("__lambda", btd, nullptr, AnnotationArgumentList(), false, block->at);
        pStruct->fields.back().generated = true;
        pStruct->fields.back().type->sanitize();
        auto finFunc = make_smart<TypeDecl>(Type::tFunction);
        auto finArg = make_smart<TypeDecl>(Type::tPointer);
        finArg->firstType = make_smart<TypeDecl>(pStruct);
        finArg->constant = false;
        finArg->removeConstant = true;
        finFunc->argTypes.push_back(finArg);
        finFunc->argNames.push_back("__this");
        finFunc->firstType = make_smart<TypeDecl>(Type::tVoid);
        pStruct->fields.emplace_back("__finalize", finFunc, nullptr, AnnotationArgumentList(), false, block->at);
        pStruct->fields.back().generated = true;
        pStruct->fields.back().type->sanitize();
        if ( needYield ) {
            auto yt = make_smart<TypeDecl>(Type::tInt);
            pStruct->fields.emplace_back("__yield", yt, nullptr, AnnotationArgumentList(), false, block->at);
            auto & fldb = pStruct->fields.back();
            fldb.generated = true;
            fldb.type->sanitize();
        }
        for ( auto var : capt ) {
            auto td = make_smart<TypeDecl>(*var->type);
            td->constant = false;
            CaptureMode mode = CaptureMode::capture_any;
            auto it = find_if ( capture.begin(), capture.end(), [&] ( const auto & entry ){
                return entry.name == var->name;
            });
            if ( it != capture.end() ) {
                mode = it->mode;
            }
            if ( isCaptureAsRef(var) || mode==CaptureMode::capture_by_reference ) {
                td->ref = false;
                auto ptd = make_smart<TypeDecl>(Type::tPointer);
                ptd->firstType = td;
                td = ptd;
                pStruct->fields.emplace_back(var->name, td, nullptr, AnnotationArgumentList(), false, var->at);
                auto & bfld = pStruct->fields.back();
                bfld.capturedConstant = var->type->constant;
                bfld.capturedRef = true;
                bfld.type->sanitize();
            } else {
                td->ref = false;
                pStruct->fields.emplace_back(var->name, td, nullptr, AnnotationArgumentList(), false, var->at);
                auto & bfld = pStruct->fields.back();
                bfld.capturedConstant = var->type->constant;
                if ( mode==CaptureMode::capture_by_move || mode==CaptureMode::capture_by_clone ) {
                    bfld.doNotDelete = true;
                }
                bfld.type->sanitize();
            }
        }
        return pStruct;
    }

    ExpressionPtr generateLambdaMakeStruct ( const StructurePtr & ls, const FunctionPtr & lf, const FunctionPtr & lff,
                                            const safe_var_set & capt, const vector<CaptureEntry> & capture, const LineInfo & at,
                                            Program * thisProgram ) {
        auto asc = new ExprAscend();
        asc->at = at;
        asc->needTypeInfo = true;
        auto makeS = make_smart<ExprMakeStruct>();
        // makeS->useInitializer = true;
        makeS->at = at;
        makeS->makeType = make_smart<TypeDecl>(ls);
        auto ms = make_smart<MakeStruct>();
        auto atTHIS = make_smart<ExprAddr>(lf->at, "_::" + lf->name);
        // TODO: expand atTHIS->funcType, so that it points to correct function by type as well
        auto mTHIS = make_smart<MakeFieldDecl>(lf->at, "__lambda", atTHIS, false, false);
        ms->push_back(mTHIS);
        auto atTHISF = make_smart<ExprAddr>(lff->at, "_::" + lff->name);
        auto mTHISF = make_smart<MakeFieldDecl>(lf->at, "__finalize", atTHISF, false, false);
        ms->push_back(mTHISF);
        for ( auto cV : capt ) {
            CaptureMode mode = CaptureMode::capture_any;
            auto it = find_if ( capture.begin(), capture.end(), [&] ( const auto & entry ){
                return entry.name == cV->name;
            });
            if ( it != capture.end() ) {
                mode = it->mode;
            }
            if ( isCaptureAsRef(cV) || mode==CaptureMode::capture_by_reference ) {
                auto varV = make_smart<ExprVar>(cV->at, cV->name);
                auto addrV = make_smart<ExprRef2Ptr>(cV->at, varV);
                addrV->alwaysSafe = true;
                auto mV = make_smart<MakeFieldDecl>(cV->at, cV->name, addrV, false, false);
                ms->push_back(mV);
            } else {
                bool moveS = false;
                bool cloneS = false;
                switch ( mode ) {
                    case CaptureMode::capture_by_clone:     cloneS = true; break;
                    case CaptureMode::capture_by_move:      moveS = true; break;
                    case CaptureMode::capture_any:          moveS = !cV->type->canCopy(); break;
                    default: ;
                }
                auto varV = make_smart<ExprVar>(cV->at, cV->name);
                auto mV = make_smart<MakeFieldDecl>(cV->at, cV->name, varV, moveS, cloneS);
                ms->push_back(mV);
            }
            auto & lexpr = ms->back();
            thisProgram->library.foreach([&](Module * mod){
                for ( auto & cm : mod->captureMacros ) {
                    auto cexpr = cm->captureExpression(thisProgram, thisProgram->thisModule.get(), lexpr->value.get(), cV->type.get());
                    if ( cexpr != nullptr ) {
                        lexpr->value = cexpr;
                    }
                }
                return true;
            },"*");
        }
        makeS->structs.push_back(ms);
        asc->subexpr = makeS;
        asc->ascType = make_smart<TypeDecl>(*ls->fields[0].type);
        asc->ascType->argTypes.erase(asc->ascType->argTypes.begin());
        asc->ascType->argNames.erase(asc->ascType->argNames.begin());
        asc->ascType->baseType = Type::tLambda;
        auto res = ExpressionPtr(asc);
        verifyGenerated(res);
        return res;
    }

    // rename variable to unique name variable

    string aotSuffixNameEx ( const string & funcName, const char * suffix );

    class RenameVar : public Visitor {
    public:
        virtual void preVisit ( ExprBlock * block ) override {
            Visitor::preVisit(block);
            scopes.push_back(block);
        }
        virtual ExpressionPtr visit ( ExprBlock * block ) override {
            scopes.pop_back();
            return Visitor::visit(block);
        }
        virtual void preVisit ( ExprFor * expr ) override {
            Visitor::preVisit(expr);
            if ( scopes.size()==0 ) {   // only top level for loop
                for ( int i=0; i!=expr->iterators.size(); ++i ) {
                    auto & varName = expr->iterators[i];
                    auto & var = expr->iteratorVariables[i];
                    if ( varName[0]!='_' || varName[1]!='_' ) {
                        string newName = "__" + aotSuffixNameEx(varName,"_Var") + "_rename_at_" + to_string(var->at.line) + "_" + to_string(var->at.column);
                        rename[var->name] = newName;
                        var->name = newName;
                        varName = newName;
                    }
                }
            }
        }
        virtual void preVisit ( ExprLet * expr ) override {
            Visitor::preVisit(expr);
            if ( scopes.size()==1 ) {   // only top level block
                for ( auto & var : expr->variables ) {
                    if ( var->name[0]!='_' || var->name[1]!='_' ) {
                        string newName = "__" + aotSuffixNameEx(var->name,"_Var") + "_rename_at_" + to_string(var->at.line) + "_" + to_string(var->at.column);
                        rename[var->name] = newName;
                        var->name = newName;
                    }
                }
            }
        }
        virtual void preVisit ( ExprVar * expr ) override {
            if ( !scopes.size() ) return;
            auto it = rename.find(expr->name);
            if ( it != rename.end() ) {
                expr->name = it->second;
            }
        }
    protected:
        vector<ExprBlock *> scopes;
        das_hash_map<string,string> rename;
    };

    void giveBlockVariablesUniqueNames  ( const ExpressionPtr & expr ) {
        RenameVar rename;
        expr->visit(rename);
    }

    // rename variable
    class RenameBlockArgument : public Visitor {
    public:
        RenameBlockArgument ( const string & name, const string & newName, ExprBlock * block )
            : argName(name), argNewName(newName), renameBlock(block) {

        }
        virtual void preVisit ( ExprBlock * block ) override {
            Visitor::preVisit(block);
            scopes.push_back(block);
        }
        virtual ExpressionPtr visit ( ExprBlock * block ) override {
            scopes.pop_back();
            return Visitor::visit(block);
        }
        virtual void preVisit ( ExprVar * expr ) override {
            if ( !scopes.empty() ) {
                auto thisBlock = scopes.back();
                if ( expr->name==argName && expr->block && renameBlock==thisBlock ) {
                    expr->name = argNewName;
                }
            }
        }
    protected:
        string argName;
        string argNewName;
        ExprBlock * renameBlock;
        vector<ExprBlock *> scopes;
    };

    void renameBlockArgument ( ExprBlock * block, const string & name, const string & newName ) {
        RenameBlockArgument vis(name,newName,block);
        block->visit(vis);
    }

    // replace ref to ptr

    class Ref2PtrVisitor : public Visitor {
    public:
        Ref2PtrVisitor ( const string & n ) : varName(n) {}
        virtual ExpressionPtr visit ( ExprVar * expr ) override {
            if  ( expr->name==varName ) {
                return make_smart<ExprPtr2Ref>(expr->at, expr);
            }
            return Visitor::visit(expr);
        }
    protected:
        string varName;
    };

    void replaceRef2Ptr ( const ExpressionPtr & expr, const string & name ) {
        Ref2PtrVisitor r2ptr(name);
        expr->visit(r2ptr);
    }

    // replace break and continue with 'goto label' for the specific loop

    class BreakAndContinueVisitor : public Visitor {
    public:
        BreakAndContinueVisitor ( int32_t bg, int32_t cg )
            : breakGoto(bg), continueGoto(cg) {
        }
        virtual void preVisit ( ExprWhile * expr ) override {
            Visitor::preVisit(expr);
            depth ++;
        }
        virtual ExpressionPtr visit(ExprWhile *expr) override {
            depth --;
            return Visitor::visit(expr);
        }
        virtual void preVisit ( ExprFor * expr ) override {
            Visitor::preVisit(expr);
            depth ++;
        }
        virtual ExpressionPtr visit(ExprFor *expr) override {
            depth --;
            return Visitor::visit(expr);
        }
        virtual ExpressionPtr visit(ExprBreak *expr) override {
            if ( depth ) return Visitor::visit(expr);
            return make_smart<ExprGoto>(expr->at, breakGoto);
        }
        virtual ExpressionPtr visit(ExprContinue *expr) override {
            if ( depth ) return Visitor::visit(expr);
            return make_smart<ExprGoto>(expr->at, continueGoto);
        }
    protected:
        int32_t breakGoto;
        int32_t continueGoto;
        int     depth = 0;
    };

    void replaceBreakAndContinue ( Expression * expr, int32_t bg, int32_t cg ) {
        BreakAndContinueVisitor rbnc(bg, cg);
        expr->visit(rbnc);
    }

    ExpressionPtr generateYield( ExprYield * expr, const FunctionPtr & func ) {
        const auto & yarg = func->arguments[1];
        // TODO: verify yield type so that error is 'yield' error, not copy or move error
        auto LabelX = func->totalGenLabel ++;
        auto blk = make_smart<ExprBlock>();
        blk->isCollapseable = true;
        blk->at = expr->at;
        bool makeRef = false;
        if ( func->arguments.size()==2 ) { // starts with _ryield
            const auto & argn = func->arguments[1]->name;
            if ( argn.length()>=7 ) {
                makeRef = memcmp ( argn.c_str(), "_ryield", 7 ) ==  0;
            }
        }
        if ( expr->moveSemantics ) {
            // TODO: error on makeRef + moveSemantics
            // result <- a
            auto mto = make_smart<ExprVar>(expr->at, yarg->name);
            auto mfr = expr->subexpr->clone();
            auto mve = make_smart<ExprMove>(expr->at, mto, mfr);
            mve->skipLockCheck = expr->skipLockCheck;
            blk->list.push_back(mve);
        } else {
            // result = a
            auto cto = make_smart<ExprVar>(expr->at, yarg->name);
            auto cfr = expr->subexpr->clone();
            if ( makeRef ) {
                cfr = make_smart<ExprRef2Ptr>(expr->at, cfr);
                cfr->alwaysSafe = true;
            }
            auto cpy = make_smart<ExprCopy>(expr->at, cto, cfr);
            cpy->allowCopyTemp = true;  // this is for generators which return temp# values
            blk->list.push_back(cpy);
        }
        // yield = X
        auto yyx = make_smart<ExprVar>(expr->at, "__yield");
        auto clx = make_smart<ExprConstInt>(expr->at, LabelX);
        auto cpy = make_smart<ExprCopy>(expr->at, yyx, clx);
        blk->list.push_back(cpy);
        // return true
        auto btr = make_smart<ExprConstBool>(expr->at, true);
        auto rex = make_smart<ExprReturn>(expr->at, btr);
        rex->fromYield = true;
        blk->list.push_back(rex);
        auto lbx = make_smart<ExprLabel>(expr->at, LabelX,
                                          "yield at line " + to_string(expr->at.line));
        blk->list.push_back(lbx);
        verifyGenerated(blk);
        return blk;
    }

    ExpressionPtr replaceGeneratorLet ( ExprLet * expr, const FunctionPtr & func, ExprBlock * scope ) {
        auto blk = make_smart<ExprBlock>();
        blk->at = expr->at;
        blk->isCollapseable = true;
        auto capture = func->arguments[0]->type->structType;
        DAS_ASSERT(capture && "generator first argument is lambda capture");
        for ( auto & var : expr->variables ) {
            auto vtd = make_smart<TypeDecl>(*var->type);
            bool isRef = vtd->ref;
            if ( isRef ) {
                auto pvtd = make_smart<TypeDecl>(Type::tPointer);
                pvtd->firstType = vtd;
                vtd->ref = false;
                vtd = pvtd;
                replaceRef2Ptr(scope, var->name);
            } else {
                vtd->constant = false;
            }
            capture->fields.emplace_back(var->name,
                                         vtd,
                                         nullptr,
                                         AnnotationArgumentList(),
                                         false,
                                         expr->at);
            auto & fldb = capture->fields.back();
            if ( isRef || var->do_not_delete ) {
                fldb.doNotDelete = true;
            }
            fldb.capturedConstant = var->type->constant;
            auto cvar = make_smart<ExprVar>(var->at, func->arguments[0]->name);
            auto lvar = make_smart<ExprField>(var->at, cvar, var->name);
            lvar->ignoreCaptureConst = true;
            if ( var->init ) {
                auto rini = var->init->clone();
                if ( isRef ) {
                    auto arini = make_smart<ExprRef2Ptr>(expr->at, rini);
                    arini->alwaysSafe = true;
                    rini = arini;
                }
                if ( var->init_via_clone ) {
                    auto cln = make_smart<ExprClone>(var->at, lvar, rini);
                    blk->list.push_back(cln);
                } else if ( var->init_via_move ) {
                    auto mve = make_smart<ExprMove>(var->at, lvar, rini);
                    blk->list.push_back(mve);
                } else {
                    auto cpy = make_smart<ExprCopy>(var->at, lvar, rini);
                    blk->list.push_back(cpy);
                }
            } else {
                auto mz = make_smart<ExprMemZero>(var->at, "memzero");
                mz->arguments.push_back(lvar);
                blk->list.push_back(mz);
            }
        }
        verifyGenerated(blk);
        return blk;
    }

    ExpressionPtr replaceGeneratorIfThenElse ( ExprIfThenElse * expr, const FunctionPtr & func ) {
        auto blk = make_smart<ExprBlock>();
        blk->at = expr->at;
        blk->isCollapseable = true;
        if ( expr->if_false ) {
            auto else_label = func->totalGenLabel ++;
            auto end_label = func->totalGenLabel ++;
            auto gtel = make_smart<ExprGoto>(expr->at, else_label);
            auto btel = make_smart<ExprBlock>();
            btel->at = expr->at;
            btel->list.push_back(gtel);
            auto ncnd = make_smart<ExprOp1>(expr->cond->at, "!", expr->cond->clone());
            auto ifnc = make_smart<ExprIfThenElse>(expr->at, ncnd, btel, nullptr);
            blk->list.push_back(ifnc);
            auto ift = expr->if_true->clone();
            if ( ift->rtti_isBlock() ){
                auto iftb = static_pointer_cast<ExprBlock>(ift);
                iftb->isCollapseable = true;
                giveBlockVariablesUniqueNames(ift);
            }
            blk->list.push_back(ift);
            auto gten = make_smart<ExprGoto>(expr->at, end_label);
            blk->list.push_back(gten);
            auto elsel = make_smart<ExprLabel>(expr->at, else_label,
                                                "else if at line " + to_string(expr->at.line));
            blk->list.push_back(elsel);
            auto iff = expr->if_false->clone();
            if ( iff->rtti_isBlock() ){
                auto iffb = static_pointer_cast<ExprBlock>(iff);
                iffb->isCollapseable = true;
                giveBlockVariablesUniqueNames(iff);
            }
            blk->list.push_back(iff);
            auto enddl = make_smart<ExprLabel>(expr->at, end_label,
                                                "end if at line " + to_string(expr->at.line));
            blk->list.push_back(enddl);
        } else {
            auto end_label = func->totalGenLabel ++;
            auto gtel = make_smart<ExprGoto>(expr->at, end_label);
            auto btel = make_smart<ExprBlock>();
            btel->at = expr->at;
            btel->list.push_back(gtel);
            auto ncnd = make_smart<ExprOp1>(expr->cond->at, "!", expr->cond->clone());
            auto ifnc = make_smart<ExprIfThenElse>(expr->at, ncnd, btel, nullptr);
            blk->list.push_back(ifnc);
            auto ift = expr->if_true->clone();
            if ( ift->rtti_isBlock() ){
                auto iftb = static_pointer_cast<ExprBlock>(ift);
                iftb->isCollapseable = true;
                giveBlockVariablesUniqueNames(ift);
            }
            blk->list.push_back(ift);
            auto enddl = make_smart<ExprLabel>(expr->at, end_label,
                                                "end if at line " + to_string(expr->at.line));
            blk->list.push_back(enddl);
        }
        verifyGenerated(blk);
        return blk;
    }

    ExpressionPtr replaceGeneratorWhile ( ExprWhile * expr, const FunctionPtr & func ) {
        auto begin_loop_label = func->totalGenLabel ++;
        auto end_loop_label = func->totalGenLabel ++;
        smart_ptr<ExprBlock> bodyBlock;
        if ( expr->body->rtti_isBlock() ) {
            bodyBlock = static_pointer_cast<ExprBlock>(expr->body->clone());
            giveBlockVariablesUniqueNames(bodyBlock);
            replaceBreakAndContinue(bodyBlock.get(), end_loop_label, begin_loop_label);
        }
        auto blk = make_smart<ExprBlock>();
        blk->at = expr->at;
        blk->isCollapseable = true;
        auto bll = make_smart<ExprLabel>(expr->at, begin_loop_label,
                                          "begin while at line " + to_string(expr->at.line));
        blk->list.push_back(bll);
        auto gtel = make_smart<ExprGoto>(expr->at, end_loop_label);
        auto btel = make_smart<ExprBlock>();
        btel->at = expr->at;
        btel->list.push_back(gtel);
        auto ncnd = make_smart<ExprOp1>(expr->cond->at, "!", expr->cond->clone());
        auto ifnc = make_smart<ExprIfThenElse>(expr->at, ncnd, btel, nullptr);
        blk->list.push_back(ifnc);
        if ( bodyBlock ) {
            for ( auto & bse : bodyBlock->list ) {
                blk->list.push_back(bse->clone());
            }
        } else {
            blk->list.push_back(expr->body->clone());
        }
        auto gbeg = make_smart<ExprGoto>(expr->at, begin_loop_label);
        blk->list.push_back(gbeg);
        auto ell = make_smart<ExprLabel>(expr->at, end_loop_label,
                                          "end while at line " + to_string(expr->at.line));
        blk->list.push_back(ell);
        if ( bodyBlock && !bodyBlock->finalList.empty() ) { // finally, if we have it
            for ( auto & fse : bodyBlock->finalList ) {
                blk->list.push_back(fse->clone());
            }
        }
        verifyGenerated(blk);
        return blk;
    }

    ExpressionPtr replaceGeneratorFor ( ExprFor * expr, const FunctionPtr & func ) {
        auto begin_loop_label = func->totalGenLabel ++;
        auto mid_loop_label = func->totalGenLabel ++;
        auto end_loop_label = func->totalGenLabel ++;
        smart_ptr<ExprFor> forCopy;
        smart_ptr<ExprBlock> bodyBlock;
        if ( expr->body->rtti_isBlock() ) {
            forCopy = static_pointer_cast<ExprFor>(expr->clone());
            bodyBlock = static_pointer_cast<ExprBlock>(forCopy->body);
            giveBlockVariablesUniqueNames(forCopy);
            replaceBreakAndContinue(bodyBlock.get(), end_loop_label, mid_loop_label);
            expr = forCopy.get();
        }
        auto blk = make_smart<ExprBlock>();
        blk->at = expr->at;
        blk->isCollapseable = true;
        auto gtel = make_smart<ExprGoto>(expr->at, end_loop_label);
        auto btel = make_smart<ExprBlock>();
        btel->at = expr->at;
        btel->list.push_back(gtel);
        // names
        string loopVar = "_loop_at_" + to_string(expr->at.line) + "_" + to_string(expr->at.column);
        vector<string> srcNames, pVarNames;
        for ( size_t si=0, sis=expr->sources.size(); si!=sis; ++si ) {
            srcNames.push_back("_source_" + to_string(si) + "_at_" + to_string(expr->at.line) + "_" + to_string(expr->at.column));
            pVarNames.push_back("_pvar_" + to_string(si) + "_at_" + to_string(expr->at.line) + "_" + to_string(expr->at.column));
        }
        auto leqt = make_smart<ExprLet>();
        leqt->at = expr->at;
        leqt->atInit = expr->at;
        leqt->visibility = expr->visibility;
        auto lvar = make_smart<Variable>();
        lvar->generated = true;
        lvar->at = expr->at;
        lvar->name = loopVar;
        lvar->type = make_smart<TypeDecl>(Type::tBool);
        lvar->init = make_smart<ExprConstBool>(expr->at, true);
        leqt->variables.push_back(lvar);
        blk->list.push_back(leqt);
        // sources
        for ( size_t si=0, sis=expr->sources.size(); si!=sis; ++si ) {
            const string & srcName = srcNames[si];
            const string & pVarName = pVarNames[si];
            const string & srcVarName = expr->iterators[si];
            const auto & src = expr->sources[si];
            const auto & iterv = expr->iteratorVariables[si];
            // let src0 = each(blah) or let src0 = blah if its iterator
            auto seqt = make_smart<ExprLet>();
            seqt->at = expr->at;
            seqt->atInit = expr->at;
            seqt->visibility = expr->visibility;
            auto svar = make_smart<Variable>();
            svar->generated = true;
            svar->at = expr->at;
            svar->name = srcName;
            svar->type = make_smart<TypeDecl>(Type::autoinfer);
            svar->init_via_move = true;
            if ( src->type->isGoodIteratorType() ) {
                svar->init = src->clone();
            } else {
                auto ceach = make_smart<ExprCall>(expr->at, "each");
                ceach->generated = true;
                ceach->alwaysSafe = true;
                ceach->arguments.push_back(src->clone());
                svar->init = ceach;
            }
            seqt->variables.push_back(svar);
            blk->list.push_back(seqt);
            // let it0 : type_of_iterable
            auto srci = make_smart<ExprLet>();
            srci->at = expr->at;
            srci->atInit = expr->at;
            srci->visibility = expr->visibility;
            auto srcv = make_smart<Variable>();
            srcv->at = iterv->at;
            srcv->name = srcVarName;
            if ( iterv->type->ref ) {
                srcv->do_not_delete = true;
                srcv->type = make_smart<TypeDecl>(Type::tPointer);
                srcv->type->firstType = make_smart<TypeDecl>(*iterv->type);
                srcv->type->firstType->constant |= src->type->constant;
                srcv->type->firstType->ref = false;
                if ( bodyBlock ) {
                    replaceRef2Ptr(bodyBlock, iterv->name);
                } else {
                    replaceRef2Ptr(expr, iterv->name);
                }
            } else {
                srcv->type = make_smart<TypeDecl>(*iterv->type);
                srcv->type->constant |= src->type->constant;
            }
            srci->variables.push_back(srcv);
            blk->list.push_back(srci);
            // let pvar0 = reinterpret_cast<void?>(addr(it0))
            auto vit0 = make_smart<ExprVar>(expr->at, srcVarName);
            auto adri = make_smart<ExprRef2Ptr>(expr->at, vit0);
            adri->alwaysSafe = true;
            auto pvoid = make_smart<TypeDecl>(Type::tPointer);
            pvoid->firstType = make_smart<TypeDecl>(Type::tVoid);
            auto rein = make_smart<ExprCast>(expr->at, adri, pvoid);
            rein->reinterpret = true;
            rein->alwaysSafe = true;
            auto veqt = make_smart<ExprLet>();
            veqt->at = expr->at;
            veqt->atInit = expr->at;
            veqt->visibility = expr->visibility;
            auto vvar = make_smart<Variable>();
            vvar->generated = true;
            vvar->at = expr->at;
            vvar->name = pVarName;
            vvar->type = make_smart<TypeDecl>(*pvoid);
            vvar->init = rein;
            veqt->variables.push_back(vvar);
            blk->list.push_back(veqt);
            // loop &= _builtin_iterator_first(it0,pvar0)
            auto cbif = make_smart<ExprCall>(expr->at, "_builtin_iterator_first");
            cbif->generated = true;
            cbif->arguments.push_back(make_smart<ExprVar>(expr->at, srcName));
            cbif->arguments.push_back(make_smart<ExprVar>(expr->at, pVarName));
            auto lande = make_smart<ExprOp2>(expr->at,"&&=",
                                              make_smart<ExprVar>(expr->at,loopVar),cbif);
            blk->list.push_back(lande);
        }
        auto bll = make_smart<ExprLabel>(expr->at, begin_loop_label,
                                          "begin for at line " + to_string(expr->at.line));
        blk->list.push_back(bll);
        auto ncnd = make_smart<ExprOp1>(expr->at, "!", make_smart<ExprVar>(expr->at,loopVar));
        auto ifnc = make_smart<ExprIfThenElse>(expr->at, ncnd, btel, nullptr);
        blk->list.push_back(ifnc);
        if ( bodyBlock ) {
            for ( auto & bse : bodyBlock->list ) {
                blk->list.push_back(bse->clone());
            }
        } else {
            blk->list.push_back(expr->body->clone());
        }
        auto mll = make_smart<ExprLabel>(expr->at, mid_loop_label,
                                          "continue for at line " + to_string(expr->at.line));
        blk->list.push_back(mll);
        // loop &= _builtin_iterator_next(it0,pvar0)
        for ( size_t si=0, sis=expr->sources.size(); si!=sis; ++si ) {
            const string & srcName = srcNames[si];
            const string & pVarName = pVarNames[si];
            auto cbif = make_smart<ExprCall>(expr->at, "_builtin_iterator_next");
            cbif->generated = true;
            cbif->arguments.push_back(make_smart<ExprVar>(expr->at, srcName));
            cbif->arguments.push_back(make_smart<ExprVar>(expr->at, pVarName));
            auto lande = make_smart<ExprOp2>(expr->at,"&&=",
                                              make_smart<ExprVar>(expr->at,loopVar),cbif);
            blk->list.push_back(lande);
        }
        auto gbeg = make_smart<ExprGoto>(expr->at, begin_loop_label);
        blk->list.push_back(gbeg);
        auto ell = make_smart<ExprLabel>(expr->at, end_loop_label,
                                          "end for at line " + to_string(expr->at.line));
        blk->list.push_back(ell);
        if ( bodyBlock && !bodyBlock->finalList.empty() ) { // finally, if we have it
            for ( auto & fse : bodyBlock->finalList ) {
                blk->list.push_back(fse->clone());
            }
        }
        // loop &= _builtin_iterator_close(it0,pvar0)
        for ( size_t si=0, sis=expr->sources.size(); si!=sis; ++si ) {
            const string & srcName = srcNames[si];
            const string & pVarName = pVarNames[si];
            auto cbif = make_smart<ExprCall>(expr->at, "_builtin_iterator_close");
            cbif->generated = true;
            cbif->arguments.push_back(make_smart<ExprVar>(expr->at, srcName));
            cbif->arguments.push_back(make_smart<ExprVar>(expr->at, pVarName));
            blk->list.push_back(cbif);
        }
        verifyGenerated(blk);
        return blk;
    }

    FunctionPtr makeCloneTuple ( const LineInfo & at, const TypeDeclPtr & tupleType ) {
        DAS_ASSERT(tupleType->isTuple() && "can only clone tuple");
        auto fn = make_smart<Function>();
        fn->generated = true;
        fn->safeImplicit = true;
        fn->privateFunction = true;
        fn->name = "clone";
        fn->at = fn->atDecl = at;
        fn->result = make_smart<TypeDecl>(Type::tVoid);
        auto arg0 = make_smart<Variable>();
        arg0->at = at;
        arg0->name = "dest";
        arg0->type = make_smart<TypeDecl>(*tupleType);
        arg0->type->constant = false;
        arg0->type->ref = false;
        fn->arguments.push_back(arg0);
        auto arg1 = make_smart<Variable>();
        arg1->at = at;
        arg1->name = "src";
        arg1->type = make_smart<TypeDecl>(*tupleType);
        arg1->type->constant = true;
        arg1->type->ref = false;
        arg1->type->implicit = true;
        fn->arguments.push_back(arg1);
        auto block = make_smart<ExprBlock>();
        block->at = at;
        for ( size_t argi=0, argis=tupleType->argTypes.size(); argi!=argis; ++argi ) {
            string argn = "_" + to_string(argi);
            auto lv = make_smart<ExprVar>(at, "dest");
            auto lf = make_smart<ExprField>(at, lv, argn);
            auto rv = make_smart<ExprVar>(at, "src");
            auto rf = make_smart<ExprField>(at, rv, argn);
            auto cl = make_smart<ExprClone>(at, lf, rf);
            block->list.push_back(cl);
        }
        fn->body = block;
        verifyGenerated(fn->body);
        return fn;
    }

    FunctionPtr generateTupleFinalizer ( const LineInfo & at, const TypeDeclPtr & tupleType ) {
        DAS_ASSERT(tupleType->isTuple() && "can only finalize tuple");
        auto fn = make_smart<Function>();
        fn->privateFunction = true;
        fn->generated = true;
        fn->name = "finalize";
        fn->at = fn->atDecl = at;
        fn->result = make_smart<TypeDecl>(Type::tVoid);
        auto arg0 = make_smart<Variable>();
        arg0->at = at;
        arg0->name = "__this";
        arg0->type = make_smart<TypeDecl>(*tupleType);
        arg0->type->constant = false;
        arg0->type->ref = false;
        arg0->type->isExplicit = true;
        fn->arguments.push_back(arg0);
        auto block = make_smart<ExprBlock>();
        block->at = at;
        bool needUnsafe = false;
        for ( size_t argi=0, argis=tupleType->argTypes.size(); argi!=argis; ++argi ) {
            if ( !tupleType->argTypes[argi]->constant && tupleType->argTypes[argi]->needDelete() ) {
                if ( tupleType->isPointer() && tupleType->firstType && tupleType->firstType->constant ) continue;
                string argn = "_" + to_string(argi);
                auto lv = make_smart<ExprVar>(at, "__this");
                auto lf = make_smart<ExprField>(at, lv, argn);
                auto cl = make_smart<ExprDelete>(at, lf);
                cl->alwaysSafe = true;
                block->list.push_back(cl);
                if ( tupleType->argTypes[argi]->isPointer() ) {
                    needUnsafe = true;
                }
            }
        }
        auto mz = make_smart<ExprMemZero>(at, "memzero");
        auto lvar = make_smart<ExprVar>(at, "__this");
        mz->arguments.push_back(lvar);
        block->list.push_back(mz);
        fn->body = block;
        if ( needUnsafe ) {
            wrapInUnsafe(fn);
        }
        verifyGenerated(fn->body);
        return fn;
    }

    FunctionPtr makeCloneVariant ( const LineInfo & at, const TypeDeclPtr & variantType ) {
        DAS_ASSERT(variantType->isVariant() && "can only clone variant");
        auto fn = make_smart<Function>();
        fn->generated = true;
        fn->safeImplicit = true;
        fn->privateFunction = true;
        fn->name = "clone";
        fn->at = fn->atDecl = at;
        fn->result = make_smart<TypeDecl>(Type::tVoid);
        auto arg0 = make_smart<Variable>();
        arg0->at = at;
        arg0->name = "dest";
        arg0->type = make_smart<TypeDecl>(*variantType);
        arg0->type->constant = false;
        arg0->type->ref = false;
        fn->arguments.push_back(arg0);
        auto arg1 = make_smart<Variable>();
        arg1->at = at;
        arg1->name = "src";
        arg1->type = make_smart<TypeDecl>(*variantType);
        arg1->type->constant = true;
        arg1->type->ref = false;
        arg1->type->implicit = true;
        fn->arguments.push_back(arg1);
        auto block = make_smart<ExprBlock>();
        block->at = at;
        smart_ptr<ExprIfThenElse> topIf, lastIf;
        for ( size_t argi=0, argis=variantType->argTypes.size(); argi!=argis; ++argi ) {
            const string & argn = variantType->argNames[argi];
            auto cb = make_smart<ExprBlock>();
            cb->at = at;
            auto vd = make_smart<ExprVar>(at, "dest");
            auto vi = make_smart<ExprConstInt>(at, int32_t(argi));
            auto svi = make_smart<ExprCall>(at, "set_variant_index");
            svi->alwaysSafe = true;
            svi->arguments.push_back(vd);
            svi->arguments.push_back(vi);
            cb->list.push_back(svi);
            auto lv = make_smart<ExprVar>(at, "dest");
            auto lf = make_smart<ExprField>(at, lv, argn);
            lf->alwaysSafe = true;
            auto rv = make_smart<ExprVar>(at, "src");
            auto rf = make_smart<ExprField>(at, rv, argn);
            rf->alwaysSafe = true;
            auto cl = make_smart<ExprClone>(at, lf, rf);
            cb->list.push_back(cl);
            auto av = make_smart<ExprVar>(at, "src");
            auto isv = make_smart<ExprIsVariant>(at, av, argn);
            auto thisIf = make_smart<ExprIfThenElse>(at, isv, cb, nullptr);
            if ( lastIf ) {
                lastIf->if_false = thisIf;
                lastIf = thisIf;
                thisIf.reset();
            } else {
                topIf = lastIf = thisIf;
            }
        }
        if (topIf) block->list.push_back(topIf);
        fn->body = block;
        verifyGenerated(fn->body);
        return fn;
    }

    FunctionPtr generateVariantFinalizer ( const LineInfo & at, const TypeDeclPtr & variantType ) {
        DAS_ASSERT(variantType->isVariant() && "can only finalize variant");
        auto fn = make_smart<Function>();
        fn->privateFunction = true;
        fn->generated = true;
        fn->name = "finalize";
        fn->at = fn->atDecl = at;
        fn->result = make_smart<TypeDecl>(Type::tVoid);
        auto arg0 = make_smart<Variable>();
        arg0->at = at;
        arg0->name = "__this";
        arg0->type = make_smart<TypeDecl>(*variantType);
        arg0->type->constant = false;
        arg0->type->ref = false;
        arg0->type->isExplicit = true;
        fn->arguments.push_back(arg0);
        auto block = make_smart<ExprBlock>();
        block->at = at;
        smart_ptr<ExprIfThenElse> topIf, lastIf;
        bool needUnsafe = false;
        for ( size_t argi=0, argis=variantType->argTypes.size(); argi!=argis; ++argi ) {
            if ( !variantType->argTypes[argi]->constant && variantType->argTypes[argi]->needDelete() ) {
                if ( variantType->argTypes[argi]->isPointer() && variantType->argTypes[argi]->firstType
                    && variantType->argTypes[argi]->firstType->constant ) continue;
                const string & argn = variantType->argNames[argi];
                auto lv = make_smart<ExprVar>(at, "__this");
                auto lf = make_smart<ExprField>(at, lv, argn);
                lf->alwaysSafe = true;
                auto cl = make_smart<ExprDelete>(at, lf);
                cl->alwaysSafe = true;
                auto cb = make_smart<ExprBlock>();
                cb->at = at;
                cb->list.push_back(cl);
                auto av = make_smart<ExprVar>(at, "__this");
                auto isv = make_smart<ExprIsVariant>(at, av, argn);
                auto thisIf = make_smart<ExprIfThenElse>(at, isv, cb, nullptr);
                if ( lastIf ) {
                    lastIf->if_false = thisIf;
                    lastIf = thisIf;
                    thisIf.reset();
                } else {
                    topIf = lastIf = thisIf;
                }
                if ( variantType->argTypes[argi]->isPointer() ) {
                    needUnsafe = true;
                }
            }
        }
        if (topIf) block->list.push_back(topIf);
        auto mz = make_smart<ExprMemZero>(at, "memzero");
        auto lvar = make_smart<ExprVar>(at, "__this");
        mz->arguments.push_back(lvar);
        block->list.push_back(mz);
        fn->body = block;
        if ( needUnsafe ) {
            wrapInUnsafe(fn);
        }
        verifyGenerated(fn->body);
        return fn;
    }

    FunctionPtr makeCloneSmartPtr ( const LineInfo & at, const TypeDeclPtr & left, const TypeDeclPtr & right ) {
        DAS_ASSERT(left->isPointer() && left->smartPtr && right->isPointer() && "can only clone smart-ptr <- any-ptr");
        DAS_ASSERT(left->firstType && left->firstType->annotation && "can only clone smart handled types");
        auto fn = make_smart<Function>();
        fn->generated = true;
        fn->safeImplicit = true;
        fn->privateFunction = true;
        fn->name = "clone";
        fn->at = fn->atDecl = at;
        fn->result = make_smart<TypeDecl>(Type::tVoid);
        auto arg0 = make_smart<Variable>();
        arg0->at = at;
        arg0->name = "dest";
        arg0->type = make_smart<TypeDecl>(*left);
        arg0->type->constant = false;
        arg0->type->ref = true;
        fn->arguments.push_back(arg0);
        auto arg1 = make_smart<Variable>();
        arg1->at = at;
        arg1->name = "src";
        arg1->type = make_smart<TypeDecl>(*right);
        arg1->type->constant = true;
        arg1->type->ref = false;
        arg1->type->implicit = true;
        fn->arguments.push_back(arg1);
        auto block = make_smart<ExprBlock>();
        block->at = at;
        auto lv = make_smart<ExprVar>(at, "dest");
        auto rv = make_smart<ExprVar>(at, "src");
        auto cl = make_smart<ExprCall>(at, left->firstType->annotation->getSmartAnnotationCloneFunction());
        DAS_ASSERT(cl->name.length() && "expecting clone name");
        cl->arguments.push_back(lv);
        cl->arguments.push_back(rv);
        block->list.push_back(cl);
        fn->body = block;
        verifyGenerated(fn->body);
        return fn;
    }

    class LocationSwapVisitor : public Visitor {
    public:
        LocationSwapVisitor( const LineInfo & na )
            : Visitor(), newAt(na) {
        }
    protected:
        virtual void preVisitExpression ( Expression * expr ) override {
            Visitor::preVisitExpression(expr);
            expr->at = newAt;
        }
    protected:
        LineInfo    newAt;
    };

    class SetGeneratedVisitor : public Visitor {
    public:
        SetGeneratedVisitor( bool setGenerated )
            : Visitor(), generated(setGenerated) {
        }
    protected:
        virtual void preVisitExpression ( Expression * expr ) override {
            Visitor::preVisitExpression(expr);
            expr->generated = generated;
        }
        virtual void preVisitArgument ( Function * fn, const VariablePtr & var, bool lastArg ) override {
            Visitor::preVisitArgument(fn, var, lastArg);
            var->generated = generated;
        }
        virtual void preVisitLet ( ExprLet * let, const VariablePtr & var, bool last ) override {
            Visitor::preVisitLet(let, var, last);
            var->generated = generated;
        }
        virtual void preVisitGlobalLet ( const VariablePtr & var) override {
            Visitor::preVisitGlobalLet(var);
            var->generated = generated;
        }
        virtual void preVisitFor ( ExprFor * expr, const VariablePtr & var, bool last ) override {
            Visitor::preVisitFor(expr, var, last);
            var->generated = generated;
        }
    protected:
        bool    generated;
    };

    ExpressionPtr forceAt ( const ExpressionPtr & expr, const LineInfo & at ) {
        LocationSwapVisitor swapAt(at);
        return expr->visit(swapAt);
    }

    ExpressionPtr forceGenerated ( const ExpressionPtr & expr, bool setGenerated ) {
        SetGeneratedVisitor setGen(setGenerated);
        return expr->visit(setGen);
    }

    void minPoint ( uint32_t & line, uint32_t & column, uint32_t LINE, uint32_t COLUMN ) {
        if ( line==LINE ) {
            column = das::min(column, COLUMN);
        } else if ( line>LINE ) {
            line = LINE;
            column = COLUMN;
        }
    }

    void maxPoint ( uint32_t & line, uint32_t & column, uint32_t LINE, uint32_t COLUMN ) {
        if ( line==LINE ) {
            column = das::max(column, COLUMN);
        } else if ( line<LINE ) {
            line = LINE;
            column = COLUMN;
        }
    }

    class EncloseVisitor : public Visitor {
    public:
        EncloseVisitor() {}
    protected:
        virtual void preVisitExpression ( Expression * expr ) override {
            Visitor::preVisitExpression(expr);
            expandEnclosure(expr->at);
            if ( expr->rtti_isCallLikeExpr() ) {
                auto ellc = static_cast<ExprLooksLikeCall *>(expr);
                expandEnclosure(ellc->atEnclosure);
            }
        }

        void expandEnclosure ( const LineInfo & at ) {
            if ( ! at.empty() ) {
                if ( first ) {
                    enclosure = at;
                    first = false;
                } else {
                    minPoint(enclosure.line, enclosure.column, at.line, at.column);
                    maxPoint(enclosure.last_line, enclosure.last_column, at.last_line, at.last_column);
                }
            }
        }
    public:
        LineInfo    enclosure;
        bool        first = true;
    };

    LineInfo encloseAt ( const ExpressionPtr & expr ) {
        EncloseVisitor enc;
        expr->visit(enc);
        return enc.enclosure;
    }

    void modifyToClassMember ( Function * func, Structure * baseClass, bool isExplicit, bool isConstant ) {
        // first argument is this
        auto argT = make_smart<TypeDecl>(baseClass);
        argT->constant = isConstant;
        argT->isExplicit = isExplicit;
        auto argV = make_smart<Variable>();
        argV->name = "self";
        argV->type = argT;
        argV->at = func->at;
        argV->generated = true;
        argV->capture_as_ref = true;
        func->arguments.insert(func->arguments.begin(), argV);
        // with self ...
        auto block = make_smart<ExprBlock>();
        block->at = func->at;
        auto wth = make_smart<ExprWith>();
        wth->at = func->at;
        auto wvar = make_smart<ExprVar>(func->at,"self");
        wvar->generated = true;
        wth->with = wvar;
        wth->body = func->body;
        block->list.push_back(wth);
        // and done
        func->body = block;
        func->isClassMethod = true;
        func->classParent = baseClass;
        DAS_ASSERT(func->classParent);
        verifyGenerated(func->body);
    }

    FunctionPtr makeClassConstructor ( Structure * baseClass, Function * method ) {
        // make a function
        auto func = make_smart<Function>();
        func->generated = true;
        func->at = method->at;
        func->atDecl = method->at;
        func->name = baseClass->name;
        func->result = make_smart<TypeDecl>(baseClass);
        func->isClassMethod = true;
        func->classParent = baseClass;
        DAS_ASSERT(func->classParent);
        if ( baseClass->macroInterface ) func->macroFunction = true;
        auto block = make_smart<ExprBlock>();
        block->at = func->at;
        func->body = block;
        for ( auto & arg : method->arguments ) {
            func->arguments.push_back(arg->clone());
        }
        // lef self = [[Foo()]]
        auto makeT = make_smart<ExprMakeStruct>(baseClass->at);
        makeT->alwaysSafe = true;
        makeT->at = func->at;
        makeT->useInitializer = true;
        makeT->nativeClassInitializer = true;
        makeT->makeType = make_smart<TypeDecl>(baseClass);
        makeT->structs.push_back(make_smart<MakeStruct>());
        auto letS = make_smart<ExprLet>();
        letS->at = func->at;
        letS->atInit = func->at;
        letS->visibility = func->atDecl;
        letS->alwaysSafe = true;    // this is due to local class variable
        auto argT = make_smart<TypeDecl>(baseClass);
        argT->constant = false;
        auto argV = make_smart<Variable>();
        argV->name = "self";
        argV->type = argT;
        argV->at = func->at;
        argV->generated = true;
        argV->capture_as_ref = true;
        argV->init = makeT;
        argV->init_via_move = true;
        letS->variables.push_back(argV);
        block->list.push_back(letS);
        // call Foo`Foo(self,args)
        auto cll = make_smart<ExprCall>(func->at,baseClass->name+"`"+baseClass->name);
        cll->arguments.push_back(make_smart<ExprVar>(func->at,"self"));
        for ( auto & arg : method->arguments ) {
            cll->arguments.push_back(make_smart<ExprVar>(func->at,arg->name));
        }
        block->list.push_back(cll);
        // return self
        auto selfV = make_smart<ExprVar>(baseClass->at,"self");
        selfV->at = func->at;
        auto returnDecl = make_smart<ExprReturn>(baseClass->at,selfV);
        returnDecl->at = func->at;
        returnDecl->moveSemantics = true;
        block->list.push_back(returnDecl);
        // and done
        func->body = block;
        verifyGenerated(func->body);
        return func;
    }

    void makeClassRtti ( Structure * baseClass ) {
        ExpressionPtr finit = make_smart<ExprTypeInfo>(baseClass->at, "rtti_classinfo", make_smart<TypeDecl>(baseClass));
        if ( baseClass->parent ) {
            auto fd = (Structure::FieldDeclaration *) baseClass->findField("__rtti");
            fd->init = finit;
            fd->parentType = fd->type->isAuto();
            fd->generated = true;
        } else {
            auto pvoid = make_smart<TypeDecl>(Type::tPointer);
            pvoid->firstType = make_smart<TypeDecl>(Type::tVoid);
            baseClass->fields.emplace_back(
                "__rtti",
                pvoid,
                finit,
                AnnotationArgumentList(),
                false,
                baseClass->at
            );
        }
    }

    FunctionPtr makeClassFinalize ( Structure * baseClass ) {
        // add __finalize field
        auto fname = baseClass->name + "'__finalize";
        ExpressionPtr finit = make_smart<ExprAddr>(baseClass->at, "_::" + fname);
        if ( baseClass->parent ) {
            auto fd = (Structure::FieldDeclaration *) baseClass->findField("__finalize");
            fd->init = make_smart<ExprCast>(baseClass->at, finit, make_smart<TypeDecl>(Type::autoinfer));
            fd->parentType = fd->type->isAuto();
            fd->generated = true;
        } else {
            baseClass->fields.emplace_back(
                "__finalize",
                make_smart<TypeDecl>(Type::autoinfer),
                finit,
                AnnotationArgumentList(),
                false,
                baseClass->at
            );
            baseClass->fields.back().generated = true;
        }
        // make a function
        auto func = make_smart<Function>();
        func->generated = true;
        func->at = baseClass->at;
        func->atDecl = baseClass->at;
        func->name = fname;
        func->result = make_smart<TypeDecl>(Type::tVoid);
        func->isClassMethod = true;
        func->classParent = baseClass;
        DAS_ASSERT(func->classParent);
        if ( baseClass->macroInterface ) func->macroFunction = true;
        auto block = make_smart<ExprBlock>();
        block->at = func->at;
        func->body = block;
        // only one argument, and its 'self'
        auto argT = make_smart<TypeDecl>(baseClass);
        argT->constant = false;
        auto argV = make_smart<Variable>();
        argV->name = "self";
        argV->type = argT;
        argV->at = func->at;
        argV->generated = true;
        argV->capture_as_ref = true;
        func->arguments.push_back(argV);
        // delete
        auto vself = make_smart<ExprVar>(func->at, "self");
        auto edel = make_smart<ExprDelete>(func->at, vself);
        edel->alwaysSafe = true;
        block->list.push_back(edel);
        // and done
        verifyGenerated(func->body);
        return func;
    }

    ExpressionPtr convertToCloneExpr ( ExprMakeStruct * expr, int index, MakeFieldDecl * decl ) {
        bool needIndex = expr->structs.size()>1;
        DAS_ASSERT(expr->block->rtti_isMakeBlock());
        auto mkb = static_pointer_cast<ExprMakeBlock>(expr->block);
        DAS_ASSERT(mkb->block->rtti_isBlock());
        auto blk = static_pointer_cast<ExprBlock>(mkb->block);
        DAS_ASSERT(blk->arguments.size()==1);
        auto selfName = blk->arguments[0]->name;
        if ( !needIndex ) {
            auto vself = make_smart<ExprVar>(decl->at, selfName);
            auto fdecl = make_smart<ExprField>(decl->at, vself, decl->name);
            auto op2c = make_smart<ExprClone>(decl->at, fdecl, decl->value->clone());
            return op2c;
        } else {
            auto vself = make_smart<ExprVar>(decl->at, selfName);
            auto cidx = make_smart<ExprConstInt>(decl->at, index);
            auto vat = make_smart<ExprAt>(decl->at, vself, cidx);
            auto fdecl = make_smart<ExprField>(decl->at, vat, decl->name);
            auto op2c = make_smart<ExprClone>(decl->at, fdecl, decl->value->clone());
            return op2c;
        }
    }

    ExpressionPtr makeStructWhereBlock ( ExprMakeStruct * mks ) {
        // make a block
        auto block = make_smart<ExprBlock>();
        block->at = mks->at;
        block->isClosure = true;
        block->returnType = make_smart<TypeDecl>(Type::tVoid);
        // only one argument, and its 'self'
        auto argT = make_smart<TypeDecl>(*(mks->makeType));
        argT->constant = false;
        if ( mks->structs.size() > 1 ) {
            argT->dim.push_back(int32_t(mks->structs.size()));
        }
        auto argV = make_smart<Variable>();
        argV->name = "__self__" + to_string(mks->at.line) + "_" + to_string(mks->at.column);
        argV->type = argT;
        argV->at = mks->at;
        argV->generated = true;
        block->arguments.push_back(argV);
        // make-block
        auto mkb = make_smart<ExprMakeBlock>(mks->at,block,false);
        // and done
        verifyGenerated(mkb);
        return mkb;
    }

    void assignDefaultArguments ( Function * func ) {
        for ( auto & arg : func->arguments ) {
            if ( arg->type->baseType==Type::fakeContext ) {
                arg->init = make_smart<ExprFakeContext>(arg->at);
                arg->init->generated = true;
            } else if ( arg->type->baseType==Type::fakeLineInfo ) {
                arg->init = make_smart<ExprFakeLineInfo>(arg->at);
                arg->init->generated = true;
            }
        }
    }
}

