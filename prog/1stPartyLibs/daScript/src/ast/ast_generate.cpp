#include "daScript/misc/platform.h"

#include "daScript/ast/ast_generate.h"
#include "daScript/ast/ast_expressions.h"
#include "daScript/ast/ast_visitor.h"

namespace das {

    bool isExpressionVariable(ExpressionPtr expr, const string & name) {
        if (expr->rtti_isVar()) {
            auto var = static_cast<ExprVar*>(expr);
            return var->name == name;
        }
        return false;
    }

    bool isExpressionVariableDeref(ExpressionPtr expr, const string & name) {
        if (expr->rtti_isVar()) {
            auto var = static_cast<ExprVar*>(expr);
            return var->name == name;
        } else if (expr->rtti_isR2V()) {
            auto r2v = static_cast<ExprRef2Value*>(expr);
            if (r2v->subexpr->rtti_isVar()) {
                auto var = static_cast<ExprVar*>(r2v->subexpr);
                return var->name == name;
            }
        }
        return false;
    }

    bool isExpressionNull(ExpressionPtr expr) {
        if (expr->rtti_isConstant() && expr->type->isPointer()) {
            auto cptr = static_cast<ExprConstPtr*>(expr);
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

    void verifyGenerated ( ExpressionPtr expr ) {
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
        auto call = new ExprCall(LineInfo(), "print");
        call->arguments.push_back(new ExprConstString(comment));
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
        auto pClosure = new ExprBlock();
        pClosure->at = expr->subexpr->at;
        pClosure->returnType = new TypeDecl(Type::autoinfer);
        pClosure->generated = true;
        // temp : Array<expr->subexpr->type>
        auto pVar = new Variable();
        pVar->generated = true;
        pVar->can_shadow = true;
        pVar->at = expr->at;
        pVar->name = compName;
        pVar->type = new TypeDecl(Type::tArray);
        pVar->type->constant = false;
        pVar->type->removeConstant = true;
        pVar->type->firstType = new TypeDecl(*expr->subexpr->type);
        pVar->type->firstType->ref = false;
        pVar->type->firstType->constant = false;
        if ( expr->subexpr->type->isPointer() && expr->subexpr->type->constant ) {
            if ( pVar->type->firstType->firstType ) {
                pVar->type->firstType->firstType->constant = true;
            }
        }
        // let temp
        auto pLet = new ExprLet();
        pLet->at = expr->at;
        pLet->atInit = expr->at;
        pLet->visibility = static_cast<ExprFor*>(expr->exprFor)->visibility;
        pLet->variables.push_back(pVar);
        pClosure->list.push_back(pLet);
        // push(temp, subexpr)
        auto pPushVal = new ExprVar();
        pPushVal->at = expr->at;
        pPushVal->name = compName;
        auto pPush = new ExprCall();
        pPush->generated = true;
        pPush->at = expr->at;
        pPush->name = expr->subexpr->type->canCopy() ? "push" : "emplace";
        pPush->arguments.push_back(pPushVal);
        pPush->arguments.push_back(expr->subexpr->clone());
        // for ...
        auto pForBlock = new ExprBlock();
        pForBlock->at = expr->at;
        pForBlock->inTheLoop = true;
        if ( expr->exprWhere ) {
            // push block
            auto pPushBlock = new ExprBlock();
            pPushBlock->at = expr->at;
            pPushBlock->list.push_back(pPush);
            // for .... if where ... push
            auto pIf = new ExprIfThenElse();
            pIf->at = expr->at;
            pIf->cond = expr->exprWhere->clone();
            pIf->if_true = pPushBlock;
            pForBlock->list.push_back(pIf);
        } else {
            // for .... push
            pForBlock->list.push_back(pPush);
        }
        auto pFor = static_cast<ExprFor*>(expr->exprFor->clone());
        pFor->body = pForBlock;
        pClosure->list.push_back(pFor);
        // return temp
        auto pVal = new ExprVar();
        pVal->at = expr->at;
        pVal->name = compName;
        auto pRet = new ExprReturn();
        pRet->at = expr->at;
        if ( tableSyntax ) {
            auto ttM = new ExprCall(expr->at, "to_table_move");
            ttM->arguments.push_back(pVal);
            pRet->subexpr = ttM;
        } else {
            pRet->subexpr = pVal;
        }
        pRet->moveSemantics = true;
        pRet->fromComprehension = true;
        pClosure->list.push_back(pRet);
        // make block
        auto pMakeBlock = new ExprMakeBlock(expr->at,pClosure);
        pMakeBlock->alwaysSafe = expr->alwaysSafe;
        // invoke
        auto pInvoke = new ExprInvoke(expr->at, "invoke");
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
        auto pClosure = new ExprBlock();
        pClosure->at = expr->subexpr->at;
        pClosure->returnType = new TypeDecl(Type::autoinfer);
        // yield subexpr
        auto pYield = new ExprYield(expr->at, expr->subexpr->clone());
        if ( !expr->subexpr->type->canCopy() ) {
            pYield->moveSemantics = true;
        }
        // for ...
        auto pForBlock = new ExprBlock();
        pForBlock->at = expr->at;
        pForBlock->inTheLoop = true;
        if ( expr->exprWhere ) {
            // yield block
            auto pPushBlock = new ExprBlock();
            pPushBlock->at = expr->at;
            pPushBlock->list.push_back(pYield);
            // for .... if where ... yield
            auto pIf = new ExprIfThenElse();
            pIf->at = expr->at;
            pIf->cond = expr->exprWhere->clone();
            pIf->if_true = pPushBlock;
            pForBlock->list.push_back(pIf);
        } else {
            // for .... yield
            pForBlock->list.push_back(pYield);
        }
        auto pFor = static_cast<ExprFor*>(expr->exprFor->clone());
        pFor->body = pForBlock;
        pClosure->list.push_back(pFor);
        // return false
        auto pRet = new ExprReturn();
        pRet->at = expr->at;
        pRet->subexpr = new ExprConstBool(expr->at, false);
        pClosure->list.push_back(pRet);
        // make block
        auto pMakeBlock = new ExprMakeBlock(expr->at,pClosure);
        // generator
        auto pMkGen = new ExprMakeGenerator(expr->at, pMakeBlock);
        pMkGen->iterType = new TypeDecl(*expr->subexpr->type);
        pMkGen->alwaysSafe = expr->alwaysSafe;
        return pMkGen;
    }

    /* a->b(args) is short for invoke(a.b, cast<auto> deref(a), args)  */
    ExprInvoke * makeInvokeMethod ( const LineInfo & at, Expression * a, const string & b ) {
        auto pInvoke = new ExprInvoke(at, "invoke");
        auto pAt = new ExprField(at, a, b);
        pInvoke->arguments.push_back(pAt);
        pInvoke->isInvokeMethod = true;
        auto pTypeAuto = new ExprTypeDecl(at, new TypeDecl(Type::autoinfer));
        pTypeAuto->typeexpr->at = at;
        pInvoke->arguments.push_back(pTypeAuto);
        return pInvoke;
    }

    /* a->b(args) is short for invoke(type<callStruct>.b, cast<auto> deref(a), args)  */
    ExprInvoke * makeInvokeMethod ( const LineInfo & at, Structure * callStruct, Expression * a, const string & b ) {
        auto pInvoke = new ExprInvoke(at, "invoke");
        auto callType = new TypeDecl(Type::tStructure);
        callType->at = at;
        callType->structType = callStruct;
        auto callTypeExpr = new ExprTypeDecl(at, callType);
        auto pAt = new ExprField(at, callTypeExpr, b);
        pInvoke->arguments.push_back(pAt);
        pInvoke->isInvokeMethod = true;
        auto pCast = new ExprCast();
        pCast->at = at;
        pCast->castType = new TypeDecl(Type::autoinfer);
        pCast->subexpr = new ExprPtr2Ref(at,a);
        pCast->subexpr->alwaysSafe = true;
        pInvoke->arguments.push_back(pCast);
        return pInvoke;
    }

    ExpressionPtr makeDelete ( const VariablePtr & var ) {
        auto eVar = new ExprVar(var->at, var->name);
        auto del = new ExprDelete(var->at, eVar);
        del->alwaysSafe = true;
        return del;
    }

    // return [[t()]]
    FunctionPtr makeConstructor ( Structure * str, bool isPrivate ) {
        auto fn = new Function();
        fn->generated = true;
        fn->privateFunction = isPrivate;
        fn->name = str->name;
        fn->at = fn->atDecl = str->at;
        fn->result = new TypeDecl(str);
        if ( str->isClass ) {
            fn->isClassMethod = true;
            fn->classParent = str;
            DAS_ASSERT(fn->classParent);
        }
        if ( str->macroInterface ) fn->macroFunction = true;
        auto block = new ExprBlock();
        block->at = str->at;
        auto makeT = new ExprMakeStruct(str->at);
        if ( str->isClass ) makeT->alwaysSafe = true;
        makeT->useInitializer = false;
        makeT->nativeClassInitializer = true;
        for ( auto & f : str->fields ) {
            if ( f.init ) {
                makeT->useInitializer = true;
                break;
            }
        }
        makeT->makeType = new TypeDecl(str);
        makeT->structs.push_back(new MakeStruct());
        auto returnDecl = new ExprReturn(str->at,makeT);
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
        auto varA = new Variable();
        varA->name = "a";
        varA->type = new TypeDecl(str);
        varA->type->isExplicit = true;
        varA->at = str->at;
        auto varB = new Variable();
        varB->name = "b";
        varB->type = new TypeDecl(str);
        varB->type->constant = str->canCloneFromConst();    // allow to clone from const
        varB->type->implicit = true;
        varB->at = str->at;
        auto fn = new Function();
        fn->name = "clone";
        fn->generated = true;
        fn->safeImplicit = true;
        fn->privateFunction = true;
        fn->at = fn->atDecl = str->at;
        fn->result = new TypeDecl();
        fn->arguments.push_back(varA);
        fn->arguments.push_back(varB);
        auto block = new ExprBlock();
        block->at = str->at;
        for ( auto & fi : str->fields ) {
            auto lA = new ExprVar(fi.at, "a");
            auto lAdotF = new ExprField(fi.at, lA, fi.name);
            auto lB = new ExprVar(fi.at, "b");
            auto lBdotF = new ExprField(fi.at, lB, fi.name);
            auto cl = new ExprClone(fi.at, lAdotF, lBdotF);
            block->list.push_back(cl);
        }
        fn->body = block;
        verifyGenerated(fn->body);
        return fn;
    }

    void wrapInUnsafe ( const FunctionPtr & func ) {
        auto blk = new ExprBlock();
        blk->at = func->body->at;
        auto usa = new ExprUnsafe(func->body->at);
        usa->body = func->body;
        blk->list.push_back(usa);
        func->body = blk;
    }

    FunctionPtr generatePointerFinalizer ( const TypeDeclPtr & ptrType, const LineInfo & at ) {
        auto pFunc = new Function();
        pFunc->privateFunction = true;
        pFunc->generated = true;
        pFunc->at = pFunc->atDecl = at;
        pFunc->name = "finalize";
        auto THIS0 = new ExprVar(at, "__this");
        auto NULLP0 = new ExprConstPtr(at);
        auto NEQ = new ExprOp2(at, "!=", THIS0, NULLP0);
        auto ifb = new ExprBlock();
        ifb->at = at;
        if ( ptrType->firstType && ptrType->firstType->isClass() ) {
            if ( ptrType->firstType->structType->macroInterface ) pFunc->macroFunction = true;
            auto sizvar = new ExprLet();              // let __size = class_rtti_size(__this)
            sizvar->at = at;
            auto vsiz = new Variable();
            vsiz->at = at;
            vsiz->name = "__size";
            vsiz->type = new TypeDecl(Type::autoinfer);
            vsiz->type->constant = true;
            auto crs = new ExprCall(at,"class_rtti_size");
            crs->arguments.push_back(new ExprVar(at,"__this"));
            vsiz->init = crs;
            //vsiz->init = make_sm
            sizvar->variables.push_back(vsiz);
            ifb->list.push_back(sizvar);
            auto invk = new ExprInvoke(at, "invoke");           // invoke(__this,__this.__finalize)
            auto THISA = new ExprVar(at, "__this");
            auto pAt = new ExprField(at, THISA, "__finalize");
            invk->arguments.push_back(pAt);
            auto pCast = new ExprCast();
            pCast->at = at;
            pCast->castType = new TypeDecl(Type::autoinfer);
            auto THISAA = new ExprVar(at, "__this");
            pCast->subexpr = new ExprPtr2Ref(at,THISAA);
            pCast->subexpr->alwaysSafe = true;
            invk->arguments.push_back(pCast);
            ifb->list.push_back(invk);
            auto THISA1 = new ExprVar(at, "__this");    // delete /*native*/ this, __size
            auto delit1 = new ExprDelete(at, THISA1);
            delit1->native = true;
            delit1->sizeexpr = new ExprVar(at,"__size");
            ifb->list.push_back(delit1);

        } else {
            auto THISA = new ExprVar(at, "__this");     // delete * this
            auto THISR = new ExprPtr2Ref(at, THISA);
            auto delit = new ExprDelete(at, THISR);
            ifb->list.push_back(delit);
            auto THISA1 = new ExprVar(at, "__this");    // delete /*native*/ this
            auto delit1 = new ExprDelete(at, THISA1);
            delit1->native = true;
            ifb->list.push_back(delit1);
        }
        auto THISB = new ExprVar(at, "__this");    // *THIS = null
        auto NULLP = new ExprConstPtr(at);
        auto SETB = new ExprCopy(at, THISB, NULLP);
        ifb->list.push_back(SETB);
        auto ife = new ExprIfThenElse(at, NEQ, ifb, nullptr);
        auto fb = new ExprBlock();
        fb->at = at;
        fb->list.push_back(ife);
        pFunc->body = fb;
        pFunc->result = new TypeDecl(Type::tVoid);
        auto cTHIS = new Variable();
        cTHIS->name = "__this";
        cTHIS->at = at;
        cTHIS->type = new TypeDecl(*ptrType);
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
        auto pFunc = new Function();
        pFunc->privateFunction = true;
        pFunc->generated = true;
        pFunc->at = pFunc->atDecl = ls->at;
        pFunc->name = "finalize";
        if ( ls->isClass ) {
            pFunc->isClassMethod = true;
            pFunc->classParent = ls;
            DAS_ASSERT(pFunc->classParent);
        }
        if ( ls->macroInterface ) pFunc->macroFunction = true;
        auto fb = new ExprBlock();
        fb->at = ls->at;
        // now finalize
        bool needUnsafe = false;
        for ( int stage=0; stage!=2; ++stage ) {
            // stage 0 is iterators, stage 1 is everything else
            for ( const auto & fl : ls->fields ) {
                if ( !fl.type->constant && !fl.capturedConstant && fl.type->needDelete() ) {
                    if ( !fl.doNotDelete && !fl.capturedRef ) {
                        if ( stage==0 && !fl.type->isIterator() ) continue;
                        if ( stage==1 && fl.type->isIterator() ) continue;
                        if ( fl.type->isPointer() && fl.type->firstType && fl.type->firstType->constant ) continue;
                        if ( ls->isLambda && !fl.type->isSafeToDelete() ) continue; // we don't do unsafe delete for lambda
                        auto fva = new ExprVar(fl.at, "__this");
                        auto fld = new ExprField(fl.at, fva, fl.name);
                        fld->ignoreCaptureConst = true;
                        auto delf = new ExprDelete(fl.at, fld);
                        delf->alwaysSafe = true;
                        fb->list.emplace_back(delf);
                        if ( fl.type->isPointer() ) {
                            needUnsafe = true;
                        }
                    }
                }
            }
        }
        auto mz = new ExprMemZero(ls->at, "memzero");
        auto lvar = new ExprVar(ls->at, "__this");
        mz->arguments.push_back(lvar);
        fb->list.push_back(mz);
        pFunc->body = fb;
        pFunc->result = new TypeDecl(Type::tVoid);
        auto cTHIS = new Variable();
        cTHIS->at = ls->at;
        cTHIS->name = "__this";
        cTHIS->type = new TypeDecl(ls);
        cTHIS->type->isExplicit = true;
        pFunc->arguments.push_back(cTHIS);
        if ( needUnsafe ) {
            wrapInUnsafe(pFunc);
        }
        verifyGenerated(pFunc->body);
        return pFunc;
    }

    FunctionPtr generateLambdaFinalizer ( const string & lambdaName, ExprBlock * block,
                                        const StructurePtr & ls, Program * thisProgram ) {
        auto lfn = lambdaName + "`finalizer";
        auto pFunc = new Function();
        pFunc->privateFunction = true;
        pFunc->generated = true;
        pFunc->at = pFunc->atDecl = block->at;
        pFunc->name = lfn;
        auto fb = new ExprBlock();
        fb->at = block->at;
        // fb->list.push_back(genComment("delete this lambda\n"));
        if ( block->finalList.size() ) {
            auto with = new ExprWith(block->at);
            auto THISVAR = new ExprVar(block->at, "__this");
            with->with = new ExprPtr2Ref(block->at, THISVAR);
            with->with->generated = true;
            auto bbl = new ExprBlock();
            bbl->at = block->at;
            with->body = bbl;
            with->body->at = block->at;
            bbl->list.reserve(block->finalList.size());     // copy finally section of the block body
            for ( auto & subexpr : block->finalList ) {
                bbl->list.push_back(subexpr->clone());
            }
            fb->list.push_back(with);
        }
        // now, lets generate all release functions (after the original finally section is generated, but before deleting the lambda itself)
        pFunc->body = fb;
        thisProgram->library.foreach([&](Module * mod){
            for ( auto & cm : mod->captureMacros ) {
                cm->releaseFunction(thisProgram, thisProgram->thisModule.get(), ls, pFunc);
            }
            return true;
        },"*");

        // delete * this
        auto THISA = new ExprVar(block->at, "__this");
        auto THISAP = new ExprPtr2Ref(block->at, THISA);
        auto delit = new ExprDelete(block->at, THISAP);
        delit->alwaysSafe = true;
        fb->list.push_back(delit);
        // delete this
        auto THISA1 = new ExprVar(block->at, "__this");
        auto delit1 = new ExprDelete(block->at, THISA1);
        delit1->native = true;
        delit1->alwaysSafe = true;
        fb->list.push_back(delit1);
        // function goo
        pFunc->result = new TypeDecl(Type::tVoid);
        auto cTHIS = new Variable();
        cTHIS->at = ls->at;
        cTHIS->name = "__this";
        cTHIS->type = new TypeDecl(Type::tPointer);
        cTHIS->type->firstType = new TypeDecl(ls);
        cTHIS->type->isExplicit = true;
        pFunc->arguments.push_back(cTHIS);
        // wrapInUnsafe(pFunc);
        verifyGenerated(pFunc->body);
        return pFunc;
    }

    FunctionPtr generateLocalFunction ( const string & lambdaName, ExprBlock * block ) {
        auto lfn = lambdaName + "`function";
        auto pFunc = new Function();
        pFunc->generated = true;
        pFunc->at = pFunc->atDecl = block->at;
        pFunc->name = lfn;
        pFunc->body = block->clone();
        pFunc->privateFunction = true;
        auto wb = static_cast<ExprBlock*>(pFunc->body);
        wb->blockFlags = 0;
        wb->arguments.clear();
        wb->returnType = nullptr;
        pFunc->result = new TypeDecl(*block->type);
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
        auto pFunc = new Function();
        pFunc->lambda = true;
        pFunc->generated = true;
        pFunc->at = pFunc->atDecl = block->at;
        pFunc->name = lfn;
        pFunc->privateFunction = true;
        pFunc->requestJit = (genFlags & generator_jit)!=0;
        pFunc->requestNoJit = (genFlags & generator_nojit)!=0;
        auto fb = new ExprBlock();
        fb->at = block->at;
        auto with = new ExprWith(block->at);
        with->with = new ExprVar(block->at, "__this");
        with->with->generated = true;
        with->body = block->clone();
        ((ExprBlock *) with->body)->isLambdaBlock = false;    // this is now a body of the function, not a lambda block
        static_cast<ExprBlock*>(with->body)->finalList.clear();
        if ( genFlags & generator_needYield ) {
            pFunc->generator = true;
            auto bbl = static_cast<ExprBlock*>(with->body);
            // goto __yeild
            auto gvar = new ExprVar(block->at, "__yield");
            auto gexpr = new ExprGoto(block->at, static_cast<Expression*>(gvar));
            bbl->list.insert(bbl->list.begin(), gexpr);
            // label "0"
            auto lzero = new ExprLabel(block->at, pFunc->totalGenLabel);
            bbl->list.insert(bbl->list.begin() + 1, lzero);
            pFunc->totalGenLabel ++;
        }
        auto wb = static_cast<ExprBlock*>(with->body);
        wb->blockFlags = 0;
        wb->arguments.clear();
        wb->returnType = nullptr;
        fb->list.push_back(with);
        pFunc->body = fb;
        pFunc->result = new TypeDecl(*block->type);
        auto cTHIS = new Variable();
        cTHIS->generated = true;
        cTHIS->at = block->at;
        cTHIS->name = "__this";
        cTHIS->type = new TypeDecl(ls);
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
                cm->captureFunction(thisProgram, thisProgram->thisModule.get(), ls, pFunc);
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
        auto pStruct = new Structure(lsn);
        pStruct->generated = true;
        pStruct->isLambda = true;
        pStruct->at = block->at;
        auto btd = block->makeBlockType();
        btd->baseType = Type::tFunction;
        btd->constant = false;
        auto thisArg = new TypeDecl(pStruct);
        btd->argTypes.insert(btd->argTypes.begin(), thisArg);
        btd->argNames.insert(btd->argNames.begin(), "__this");
        pStruct->fields.emplace_back("__lambda", btd, nullptr, AnnotationArgumentList(), false, block->at);
        pStruct->fields.back().generated = true;
        pStruct->fields.back().type->sanitize();
        auto finFunc = new TypeDecl(Type::tFunction);
        auto finArg = new TypeDecl(Type::tPointer);
        finArg->firstType = new TypeDecl(pStruct);
        finArg->constant = false;
        finArg->removeConstant = true;
        finFunc->argTypes.push_back(finArg);
        finFunc->argNames.push_back("__this");
        finFunc->firstType = new TypeDecl(Type::tVoid);
        pStruct->fields.emplace_back("__finalize", finFunc, nullptr, AnnotationArgumentList(), false, block->at);
        pStruct->fields.back().generated = true;
        pStruct->fields.back().type->sanitize();
        if ( needYield ) {
            auto yt = new TypeDecl(Type::tInt);
            pStruct->fields.emplace_back("__yield", yt, nullptr, AnnotationArgumentList(), false, block->at);
            auto & fldb = pStruct->fields.back();
            fldb.generated = true;
            fldb.type->sanitize();
        }
        for ( auto var : capt ) {
            auto td = new TypeDecl(*var->type);
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
                td->constant = var->type->constant;
                auto ptd = new TypeDecl(Type::tPointer);
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
                                            const safe_var_set & capt, const vector<CaptureEntry> & capture,
                                            const LineInfo & at, const LineInfo & captureAt, Program * thisProgram ) {
        auto asc = new ExprAscend();
        asc->at = at;
        asc->needTypeInfo = true;
        auto makeS = new ExprMakeStruct();
        // makeS->useInitializer = true;
        makeS->at = at;
        makeS->makeType = new TypeDecl(ls);
        auto ms = new MakeStruct();
        auto atTHIS = new ExprAddr(lf->at, "_::" + lf->name);
        // TODO: expand atTHIS->funcType, so that it points to correct function by type as well
        auto mTHIS = new MakeFieldDecl(lf->at, "__lambda", atTHIS, false, false);
        ms->push_back(mTHIS);
        auto atTHISF = new ExprAddr(lff->at, "_::" + lff->name);
        auto mTHISF = new MakeFieldDecl(lf->at, "__finalize", atTHISF, false, false);
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
                auto varV = new ExprVar(captureAt, cV->name);
                auto addrV = new ExprRef2Ptr(captureAt, varV);
                addrV->alwaysSafe = true;
                auto mV = new MakeFieldDecl(captureAt, cV->name, addrV, false, false);
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
                auto varV = new ExprVar(captureAt, cV->name);
                auto mV = new MakeFieldDecl(captureAt, cV->name, varV, moveS, cloneS);
                ms->push_back(mV);
            }
            auto & lexpr = ms->back();
            thisProgram->library.foreach([&](Module * mod){
                for ( auto & cm : mod->captureMacros ) {
                    auto cexpr = cm->captureExpression(thisProgram, thisProgram->thisModule.get(), lexpr->value, cV->type);
                    if ( cexpr != nullptr ) {
                        lexpr->value = cexpr;
                    }
                }
                return true;
            },"*");
        }
        makeS->structs.push_back(ms);
        asc->subexpr = makeS;
        asc->ascType = new TypeDecl(*ls->fields[0].type);
        asc->ascType->argTypes.erase(asc->ascType->argTypes.begin());
        asc->ascType->argNames.erase(asc->ascType->argNames.begin());
        asc->ascType->baseType = Type::tLambda;
        verifyGenerated(asc);
        return asc;
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
                for ( size_t i=0; i!=expr->iterators.size(); ++i ) {
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

    void giveBlockVariablesUniqueNames  ( ExpressionPtr expr ) {
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
        ExprBlock * renameBlock = nullptr;
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
                return new ExprPtr2Ref(expr->at, expr);
            }
            return Visitor::visit(expr);
        }
    protected:
        string varName;
    };

    void replaceRef2Ptr ( ExpressionPtr expr, const string & name ) {
        Ref2PtrVisitor r2ptr(name);
        expr->visit(r2ptr);
    }

    // replace break and continue with 'goto label' for the specific loop

    class BreakAndContinueVisitor : public Visitor {
    public:
        BreakAndContinueVisitor ( int32_t bg, int32_t cg, const string & bf = string(),
                                  int32_t rg = 0, const string & rf = string(), const string & rvn = string() )
            : breakGoto(bg), continueGoto(cg), breakFlag(bf),
              returnGoto(rg), returnFlag(rf), returnValueName(rvn) {
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
            if ( breakFlag.empty() ) {
                return new ExprGoto(expr->at, breakGoto);
            }
            // emit: { breakFlag = true; goto breakGoto }
            auto blk = new ExprBlock();
            blk->at = expr->at;
            blk->isCollapseable = true;
            auto flg = new ExprVar(expr->at, breakFlag);
            auto trv = new ExprConstBool(expr->at, true);
            auto cpy = new ExprCopy(expr->at, flg, trv);
            blk->list.push_back(cpy);
            blk->list.push_back(new ExprGoto(expr->at, breakGoto));
            return blk;
        }
        virtual ExpressionPtr visit(ExprContinue *expr) override {
            if ( depth ) return Visitor::visit(expr);
            return new ExprGoto(expr->at, continueGoto);
        }
        virtual ExpressionPtr visit(ExprReturn *expr) override {
            if ( depth ) return Visitor::visit(expr);
            if ( returnFlag.empty() ) return Visitor::visit(expr);
            // yield-emitted returns are internal control-flow — leave them alone
            if ( expr->fromYield ) return Visitor::visit(expr);
            // emit: { __return_value = subexpr; __returning = true; goto returnGoto }
            // so the enclosing loop's per-iteration finally runs before the real return
            auto blk = new ExprBlock();
            blk->at = expr->at;
            blk->isCollapseable = true;
            if ( expr->subexpr && !returnValueName.empty() ) {
                auto rv = new ExprVar(expr->at, returnValueName);
                auto cpy = new ExprCopy(expr->at, rv, expr->subexpr->clone());
                blk->list.push_back(cpy);
            }
            auto flg = new ExprVar(expr->at, returnFlag);
            auto trv = new ExprConstBool(expr->at, true);
            auto cpy2 = new ExprCopy(expr->at, flg, trv);
            blk->list.push_back(cpy2);
            blk->list.push_back(new ExprGoto(expr->at, returnGoto));
            return blk;
        }
    protected:
        int32_t breakGoto;
        int32_t continueGoto;
        string  breakFlag;
        int32_t returnGoto;
        string  returnFlag;
        string  returnValueName;
        int     depth = 0;
    };

    void replaceBreakAndContinue ( Expression * expr, int32_t bg, int32_t cg ) {
        BreakAndContinueVisitor rbnc(bg, cg);
        expr->visit(rbnc);
    }

    void replaceBreakAndContinueWithFlag ( Expression * expr, int32_t bg, int32_t cg, const string & breakFlag ) {
        BreakAndContinueVisitor rbnc(bg, cg, breakFlag);
        expr->visit(rbnc);
    }

    void replaceBreakContinueAndReturn ( Expression * expr, int32_t bg, int32_t cg, const string & breakFlag,
                                         int32_t rg, const string & returnFlag, const string & returnValueName ) {
        BreakAndContinueVisitor rbnc(bg, cg, breakFlag, rg, returnFlag, returnValueName);
        expr->visit(rbnc);
    }

    class HasTopLevelReturnVisitor : public Visitor {
    public:
        virtual void preVisit ( ExprWhile * ) override { depth ++; }
        virtual ExpressionPtr visit ( ExprWhile * expr ) override { depth --; return Visitor::visit(expr); }
        virtual void preVisit ( ExprFor * ) override { depth ++; }
        virtual ExpressionPtr visit ( ExprFor * expr ) override { depth --; return Visitor::visit(expr); }
        virtual void preVisit ( ExprReturn * expr ) override {
            if ( depth == 0 && !expr->fromYield ) found = true;
        }
        bool found = false;
        int  depth = 0;
    };

    bool bodyHasTopLevelReturn ( Expression * body ) {
        HasTopLevelReturnVisitor v;
        body->visit(v);
        return v.found;
    }

    ExpressionPtr generateYield( ExprYield * expr, const FunctionPtr & func ) {
        const auto & yarg = func->arguments[1];
        // TODO: verify yield type so that error is 'yield' error, not copy or move error
        auto LabelX = func->totalGenLabel ++;
        auto blk = new ExprBlock();
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
            auto mto = new ExprVar(expr->at, yarg->name);
            auto mfr = expr->subexpr->clone();
            auto mve = new ExprMove(expr->at, mto, mfr);
            blk->list.push_back(mve);
        } else {
            // result = a
            auto cto = new ExprVar(expr->at, yarg->name);
            auto cfr = expr->subexpr->clone();
            if ( makeRef ) {
                cfr = new ExprRef2Ptr(expr->at, cfr);
                cfr->alwaysSafe = true;
            }
            auto cpy = new ExprCopy(expr->at, cto, cfr);
            cpy->allowCopyTemp = true;  // this is for generators which return temp# values
            blk->list.push_back(cpy);
        }
        // yield = X
        auto yyx = new ExprVar(expr->at, "__yield");
        auto clx = new ExprConstInt(expr->at, LabelX);
        auto cpy = new ExprCopy(expr->at, yyx, clx);
        blk->list.push_back(cpy);
        // return true
        auto btr = new ExprConstBool(expr->at, true);
        auto rex = new ExprReturn(expr->at, btr);
        rex->fromYield = true;
        blk->list.push_back(rex);
        auto lbx = new ExprLabel(expr->at, LabelX,
                                          "yield at line " + to_string(expr->at.line));
        blk->list.push_back(lbx);
        verifyGenerated(blk);
        return blk;
    }

    ExpressionPtr replaceGeneratorLet ( ExprLet * expr, const FunctionPtr & func, ExprBlock * scope ) {
        auto blk = new ExprBlock();
        blk->at = expr->at;
        blk->isCollapseable = true;
        auto capture = func->arguments[0]->type->structType;
        DAS_ASSERT(capture && "generator first argument is lambda capture");
        for ( auto & var : expr->variables ) {
            auto vtd = new TypeDecl(*var->type);
            bool isRef = vtd->ref;
            if ( isRef ) {
                auto pvtd = new TypeDecl(Type::tPointer);
                pvtd->firstType = vtd;
                vtd->ref = false;
                vtd = pvtd;
                replaceRef2Ptr(scope, var->name);
            } else {
                vtd->constant = false;
            }
            // the reason we insert after __lambda and __finalize is so that when we have iterators, we delete them before we delete captured variables
            capture->fields.emplace_back(
                var->name,
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
            auto cvar = new ExprVar(var->at, func->arguments[0]->name);
            auto lvar = new ExprField(var->at, cvar, var->name);
            lvar->ignoreCaptureConst = true;
            if ( var->init ) {
                auto rini = var->init->clone();
                if ( isRef ) {
                    auto arini = new ExprRef2Ptr(expr->at, rini);
                    arini->alwaysSafe = true;
                    rini = arini;
                }
                if ( var->init_via_clone ) {
                    auto cln = new ExprClone(var->at, lvar, rini);
                    blk->list.push_back(cln);
                } else if ( var->init_via_move ) {
                    auto mve = new ExprMove(var->at, lvar, rini);
                    blk->list.push_back(mve);
                } else {
                    auto cpy = new ExprCopy(var->at, lvar, rini);
                    blk->list.push_back(cpy);
                }
            } else {
                auto mz = new ExprMemZero(var->at, "memzero");
                mz->arguments.push_back(lvar);
                blk->list.push_back(mz);
            }
        }
        verifyGenerated(blk);
        return blk;
    }

    ExpressionPtr replaceGeneratorIfThenElse ( ExprIfThenElse * expr, const FunctionPtr & func ) {
        auto blk = new ExprBlock();
        blk->at = expr->at;
        blk->isCollapseable = true;
        if ( expr->if_false ) {
            auto else_label = func->totalGenLabel ++;
            auto end_label = func->totalGenLabel ++;
            auto gtel = new ExprGoto(expr->at, else_label);
            auto btel = new ExprBlock();
            btel->at = expr->at;
            btel->list.push_back(gtel);
            auto ncnd = new ExprOp1(expr->cond->at, "!", expr->cond->clone());
            auto ifnc = new ExprIfThenElse(expr->at, ncnd, btel, nullptr);
            blk->list.push_back(ifnc);
            auto ift = expr->if_true->clone();
            if ( ift->rtti_isBlock() ){
                auto iftb = static_cast<ExprBlock*>(ift);
                iftb->isCollapseable = true;
                giveBlockVariablesUniqueNames(ift);
            }
            blk->list.push_back(ift);
            auto gten = new ExprGoto(expr->at, end_label);
            blk->list.push_back(gten);
            auto elsel = new ExprLabel(expr->at, else_label,
                                                "else if at line " + to_string(expr->at.line));
            blk->list.push_back(elsel);
            auto iff = expr->if_false->clone();
            if ( iff->rtti_isBlock() ){
                auto iffb = static_cast<ExprBlock*>(iff);
                iffb->isCollapseable = true;
                giveBlockVariablesUniqueNames(iff);
            }
            blk->list.push_back(iff);
            auto enddl = new ExprLabel(expr->at, end_label,
                                                "end if at line " + to_string(expr->at.line));
            blk->list.push_back(enddl);
        } else {
            auto end_label = func->totalGenLabel ++;
            auto gtel = new ExprGoto(expr->at, end_label);
            auto btel = new ExprBlock();
            btel->at = expr->at;
            btel->list.push_back(gtel);
            auto ncnd = new ExprOp1(expr->cond->at, "!", expr->cond->clone());
            auto ifnc = new ExprIfThenElse(expr->at, ncnd, btel, nullptr);
            blk->list.push_back(ifnc);
            auto ift = expr->if_true->clone();
            if ( ift->rtti_isBlock() ){
                auto iftb = static_cast<ExprBlock*>(ift);
                iftb->isCollapseable = true;
                giveBlockVariablesUniqueNames(ift);
            }
            blk->list.push_back(ift);
            auto enddl = new ExprLabel(expr->at, end_label,
                                                "end if at line " + to_string(expr->at.line));
            blk->list.push_back(enddl);
        }
        verifyGenerated(blk);
        return blk;
    }

    ExpressionPtr replaceGeneratorWhile ( ExprWhile * expr, const FunctionPtr & func ) {
        auto begin_loop_label = func->totalGenLabel ++;
        auto iter_end_loop_label = func->totalGenLabel ++;
        auto end_loop_label = func->totalGenLabel ++;
        ExprBlock * bodyBlock = nullptr;
        const bool hasFinally = expr->body->rtti_isBlock() &&
                                !static_cast<ExprBlock*>(expr->body)->finalList.empty();
        // only introduce __broke / __returning flags when body has a finally —
        // otherwise classic break -> end, and return just exits the enclosing function
        string breakFlag, returnFlag, returnValueName;
        bool hasReturn = false;
        if ( hasFinally ) {
            breakFlag = "__broke_while_" + to_string(expr->at.line) + "_" + to_string(expr->at.column);
            hasReturn = bodyHasTopLevelReturn(expr->body);
            if ( hasReturn ) {
                returnFlag = "__returning_while_" + to_string(expr->at.line) + "_" + to_string(expr->at.column);
                returnValueName = "__return_value_while_" + to_string(expr->at.line) + "_" + to_string(expr->at.column);
            }
        }
        if ( expr->body->rtti_isBlock() ) {
            bodyBlock = static_cast<ExprBlock*>(expr->body->clone());
            giveBlockVariablesUniqueNames(bodyBlock);
            if ( hasFinally ) {
                // break -> set flag, goto iter_end (runs finally, then exits via flag check)
                // continue -> iter_end (runs finally, flag=false, loops back)
                // return -> spill value, set flag, goto iter_end (runs finally, then real return)
                replaceBreakContinueAndReturn(bodyBlock, iter_end_loop_label, iter_end_loop_label, breakFlag,
                                              iter_end_loop_label, returnFlag, returnValueName);
            } else {
                replaceBreakAndContinue(bodyBlock, end_loop_label, begin_loop_label);
            }
        }
        auto blk = new ExprBlock();
        blk->at = expr->at;
        blk->isCollapseable = true;
        // __broke = false  (only when finally present)
        if ( hasFinally ) {
            auto leqt = new ExprLet();
            leqt->at = expr->at;
            leqt->atInit = expr->at;
            leqt->visibility = expr->at;
            auto bvar = new Variable();
            bvar->generated = true;
            bvar->at = expr->at;
            bvar->name = breakFlag;
            bvar->type = new TypeDecl(Type::tBool);
            bvar->init = new ExprConstBool(expr->at, false);
            leqt->variables.push_back(bvar);
            blk->list.push_back(leqt);
        }
        // __returning = false; __return_value : <func->result>  (only when body has return)
        if ( hasReturn ) {
            auto leqt = new ExprLet();
            leqt->at = expr->at;
            leqt->atInit = expr->at;
            leqt->visibility = expr->at;
            auto rvar = new Variable();
            rvar->generated = true;
            rvar->at = expr->at;
            rvar->name = returnFlag;
            rvar->type = new TypeDecl(Type::tBool);
            rvar->init = new ExprConstBool(expr->at, false);
            leqt->variables.push_back(rvar);
            blk->list.push_back(leqt);
            if ( func->result && !func->result->isVoid() ) {
                auto lvqt = new ExprLet();
                lvqt->at = expr->at;
                lvqt->atInit = expr->at;
                lvqt->visibility = expr->at;
                auto vvar = new Variable();
                vvar->generated = true;
                vvar->at = expr->at;
                vvar->name = returnValueName;
                vvar->type = new TypeDecl(*func->result);
                vvar->type->constant = false;
                vvar->type->explicitConst = false;
                vvar->type->ref = false;
                lvqt->variables.push_back(vvar);
                blk->list.push_back(lvqt);
            }
        }
        auto bll = new ExprLabel(expr->at, begin_loop_label,
                                          "begin while at line " + to_string(expr->at.line));
        blk->list.push_back(bll);
        // false initial / exhausted cond skips finally entirely
        auto gtel = new ExprGoto(expr->at, end_loop_label);
        auto btel = new ExprBlock();
        btel->at = expr->at;
        btel->list.push_back(gtel);
        auto ncnd = new ExprOp1(expr->cond->at, "!", expr->cond->clone());
        auto ifnc = new ExprIfThenElse(expr->at, ncnd, btel, nullptr);
        blk->list.push_back(ifnc);
        if ( bodyBlock ) {
            for ( auto & bse : bodyBlock->list ) {
                blk->list.push_back(bse->clone());
            }
        } else {
            blk->list.push_back(expr->body->clone());
        }
        if ( hasFinally ) {
            // iter_end label: per-iteration finally (normal fall-through, continue, break, return all route here)
            auto iell = new ExprLabel(expr->at, iter_end_loop_label,
                                               "iter end while at line " + to_string(expr->at.line));
            blk->list.push_back(iell);
            for ( auto & fse : bodyBlock->finalList ) {
                blk->list.push_back(fse->clone());
            }
            // if (__returning) return __return_value (runs before break check so return wins over break)
            if ( hasReturn ) {
                auto rblk = new ExprBlock();
                rblk->at = expr->at;
                rblk->isCollapseable = true;
                auto pRet = new ExprReturn();
                pRet->at = expr->at;
                if ( func->result && !func->result->isVoid() ) {
                    pRet->subexpr = new ExprVar(expr->at, returnValueName);
                }
                rblk->list.push_back(pRet);
                auto rchk = new ExprIfThenElse(expr->at, new ExprVar(expr->at, returnFlag), rblk, nullptr);
                blk->list.push_back(rchk);
            }
            // if (__broke) goto end — else fall through to goto begin
            auto gtef = new ExprGoto(expr->at, end_loop_label);
            auto btef = new ExprBlock();
            btef->at = expr->at;
            btef->list.push_back(gtef);
            auto fchk = new ExprIfThenElse(expr->at, new ExprVar(expr->at, breakFlag), btef, nullptr);
            blk->list.push_back(fchk);
        }
        auto gbeg = new ExprGoto(expr->at, begin_loop_label);
        blk->list.push_back(gbeg);
        auto ell = new ExprLabel(expr->at, end_loop_label,
                                          "end while at line " + to_string(expr->at.line));
        blk->list.push_back(ell);
        verifyGenerated(blk);
        return blk;
    }

    bool srcNeedTempVar ( Expression * src, TypeDecl * type ) {
        if ( !type->isRef() ) return false;
        return src->rtti_isCallLikeExpr() || src->rtti_isMakeLocal();
    }

    ExpressionPtr replaceGeneratorFor ( ExprFor * expr, const FunctionPtr & func ) {
        auto begin_loop_label = func->totalGenLabel ++;
        auto mid_loop_label = func->totalGenLabel ++;
        auto end_loop_label = func->totalGenLabel ++;
        ExprFor * forCopy = nullptr;
        ExprBlock * bodyBlock = nullptr;
        const bool hasFinally = expr->body->rtti_isBlock() &&
                                !static_cast<ExprBlock*>(expr->body)->finalList.empty();
        string breakFlag, returnFlag, returnValueName;
        bool hasReturn = false;
        if ( hasFinally ) {
            breakFlag = "__broke_for_" + to_string(expr->at.line) + "_" + to_string(expr->at.column);
            hasReturn = bodyHasTopLevelReturn(expr->body);
            if ( hasReturn ) {
                returnFlag = "__returning_for_" + to_string(expr->at.line) + "_" + to_string(expr->at.column);
                returnValueName = "__return_value_for_" + to_string(expr->at.line) + "_" + to_string(expr->at.column);
            }
        }
        if ( expr->body->rtti_isBlock() ) {
            forCopy = static_cast<ExprFor*>(expr->clone());
            bodyBlock = static_cast<ExprBlock*>(forCopy->body);
            giveBlockVariablesUniqueNames(forCopy);
            if ( hasFinally ) {
                // break -> set flag, goto mid (finally runs, flag check -> end, iterator_close runs)
                // continue -> mid (finally runs, advances iterator, re-checks)
                // return -> spill value, set flag, goto mid (finally runs, then real return)
                replaceBreakContinueAndReturn(bodyBlock, mid_loop_label, mid_loop_label, breakFlag,
                                              mid_loop_label, returnFlag, returnValueName);
            } else {
                replaceBreakAndContinue(bodyBlock, end_loop_label, mid_loop_label);
            }
            expr = forCopy;
        }
        auto blk = new ExprBlock();
        blk->at = expr->at;
        blk->isCollapseable = true;
        auto gtel = new ExprGoto(expr->at, end_loop_label);
        auto btel = new ExprBlock();
        btel->at = expr->at;
        btel->list.push_back(gtel);
        // names
        string loopVar = "_loop_at_" + to_string(expr->at.line) + "_" + to_string(expr->at.column);
        vector<string> srcNames, pVarNames;
        for ( size_t si=0, sis=expr->sources.size(); si!=sis; ++si ) {
            srcNames.push_back("_source_" + to_string(si) + "_at_" + to_string(expr->at.line) + "_" + to_string(expr->at.column));
            pVarNames.push_back("_pvar_" + to_string(si) + "_at_" + to_string(expr->at.line) + "_" + to_string(expr->at.column));
        }
        auto leqt = new ExprLet();
        leqt->at = expr->at;
        leqt->atInit = expr->at;
        leqt->visibility = expr->visibility;
        auto lvar = new Variable();
        lvar->generated = true;
        lvar->at = expr->at;
        lvar->name = loopVar;
        lvar->type = new TypeDecl(Type::tBool);
        lvar->init = new ExprConstBool(expr->at, true);
        leqt->variables.push_back(lvar);
        blk->list.push_back(leqt);
        // __broke = false (only when finally present)
        if ( hasFinally ) {
            auto bleqt = new ExprLet();
            bleqt->at = expr->at;
            bleqt->atInit = expr->at;
            bleqt->visibility = expr->visibility;
            auto bvar = new Variable();
            bvar->generated = true;
            bvar->at = expr->at;
            bvar->name = breakFlag;
            bvar->type = new TypeDecl(Type::tBool);
            bvar->init = new ExprConstBool(expr->at, false);
            bleqt->variables.push_back(bvar);
            blk->list.push_back(bleqt);
        }
        // __returning = false; __return_value : <func->result>  (only when body has return)
        if ( hasReturn ) {
            auto rleqt = new ExprLet();
            rleqt->at = expr->at;
            rleqt->atInit = expr->at;
            rleqt->visibility = expr->visibility;
            auto rvar = new Variable();
            rvar->generated = true;
            rvar->at = expr->at;
            rvar->name = returnFlag;
            rvar->type = new TypeDecl(Type::tBool);
            rvar->init = new ExprConstBool(expr->at, false);
            rleqt->variables.push_back(rvar);
            blk->list.push_back(rleqt);
            if ( func->result && !func->result->isVoid() ) {
                auto vleqt = new ExprLet();
                vleqt->at = expr->at;
                vleqt->atInit = expr->at;
                vleqt->visibility = expr->visibility;
                auto vvar = new Variable();
                vvar->generated = true;
                vvar->at = expr->at;
                vvar->name = returnValueName;
                vvar->type = new TypeDecl(*func->result);
                vvar->type->constant = false;
                vvar->type->explicitConst = false;
                vvar->type->ref = false;
                vleqt->variables.push_back(vvar);
                blk->list.push_back(vleqt);
            }
        }
        // sources
        for ( size_t si=0, sis=expr->sources.size(); si!=sis; ++si ) {
            const string & srcName = srcNames[si];
            const string & pVarName = pVarNames[si];
            const string & srcVarName = expr->iterators[si];
            const auto & src = expr->sources[si];
            const auto & iterv = expr->iteratorVariables[si];
            // if its a temp ref type, we need to create a temp variable
            ExprLet * tempLet = nullptr;
            if ( srcNeedTempVar(src, src->type) ) {
                tempLet = new ExprLet();
                tempLet->at = expr->at;
                tempLet->atInit = expr->at;
                tempLet->visibility = expr->visibility;
                auto svar = new Variable();
                svar->generated = true;
                svar->at = expr->at;
                svar->name = srcName + "_temp_var";
                svar->type = new TypeDecl(Type::autoinfer);
                svar->init_via_move = true;
                svar->init = src->clone();
                tempLet->variables.push_back(svar);
                blk->list.push_back(tempLet);
            }
            // let src0 = each(blah) or let src0 = blah if its iterator
            auto seqt = new ExprLet();
            seqt->at = expr->at;
            seqt->atInit = expr->at;
            seqt->visibility = expr->visibility;
            auto svar = new Variable();
            svar->generated = true;
            svar->at = expr->at;
            svar->name = srcName;
            svar->type = new TypeDecl(Type::autoinfer);
            svar->init_via_move = true;
            if ( src->type->isGoodIteratorType() ) {
                svar->init = src->clone();
            } else {
                auto ceach = new ExprCall(expr->at, "each");
                ceach->generated = true;
                ceach->alwaysSafe = true;
                if ( tempLet ) {
                    ceach->arguments.push_back(new ExprVar(expr->at, srcName + "_temp_var"));
                } else {
                    ceach->arguments.push_back(src->clone());
                }
                svar->init = ceach;
            }
            seqt->variables.push_back(svar);
            blk->list.push_back(seqt);
            // let it0 : type_of_iterable
            auto srci = new ExprLet();
            srci->at = expr->at;
            srci->atInit = expr->at;
            srci->visibility = expr->visibility;
            auto srcv = new Variable();
            srcv->at = iterv->at;
            srcv->name = srcVarName;
            if ( iterv->type->ref ) {
                srcv->do_not_delete = true;
                srcv->type = new TypeDecl(Type::tPointer);
                srcv->type->firstType = new TypeDecl(*iterv->type);
                srcv->type->firstType->constant |= src->type->constant;
                srcv->type->firstType->ref = false;
                if ( bodyBlock ) {
                    replaceRef2Ptr(bodyBlock, iterv->name);
                } else {
                    replaceRef2Ptr(expr, iterv->name);
                }
            } else {
                srcv->type = new TypeDecl(*iterv->type);
                srcv->type->constant |= src->type->constant;
            }
            srci->variables.push_back(srcv);
            blk->list.push_back(srci);
            // let pvar0 = reinterpret_cast<void?>(addr(it0))
            auto vit0 = new ExprVar(expr->at, srcVarName);
            auto adri = new ExprRef2Ptr(expr->at, vit0);
            adri->alwaysSafe = true;
            auto pvoid = new TypeDecl(Type::tPointer);
            pvoid->firstType = new TypeDecl(Type::tVoid);
            auto rein = new ExprCast(expr->at, adri, pvoid);
            rein->reinterpret = true;
            rein->alwaysSafe = true;
            auto veqt = new ExprLet();
            veqt->at = expr->at;
            veqt->atInit = expr->at;
            veqt->visibility = expr->visibility;
            auto vvar = new Variable();
            vvar->generated = true;
            vvar->at = expr->at;
            vvar->name = pVarName;
            vvar->type = new TypeDecl(*pvoid);
            vvar->init = rein;
            veqt->variables.push_back(vvar);
            blk->list.push_back(veqt);
            // loop &= _builtin_iterator_first(it0,pvar0)
            auto cbif = new ExprCall(expr->at, "_builtin_iterator_first");
            cbif->generated = true;
            cbif->arguments.push_back(new ExprVar(expr->at, srcName));
            cbif->arguments.push_back(new ExprVar(expr->at, pVarName));
            auto lande = new ExprOp2(expr->at,"&&=",
                                              new ExprVar(expr->at,loopVar),cbif);
            blk->list.push_back(lande);
        }
        auto bll = new ExprLabel(expr->at, begin_loop_label,
                                          "begin for at line " + to_string(expr->at.line));
        blk->list.push_back(bll);
        auto ncnd = new ExprOp1(expr->at, "!", new ExprVar(expr->at,loopVar));
        auto ifnc = new ExprIfThenElse(expr->at, ncnd, btel, nullptr);
        blk->list.push_back(ifnc);
        if ( bodyBlock ) {
            for ( auto & bse : bodyBlock->list ) {
                blk->list.push_back(bse->clone());
            }
        } else {
            blk->list.push_back(expr->body->clone());
        }
        auto mll = new ExprLabel(expr->at, mid_loop_label,
                                          "continue for at line " + to_string(expr->at.line));
        blk->list.push_back(mll);
        if ( hasFinally ) {
            // per-iteration finally (normal fall-through, continue, break, return all route here)
            for ( auto & fse : bodyBlock->finalList ) {
                blk->list.push_back(fse->clone());
            }
            // if (__returning) { <close iterators>; return __return_value }
            if ( hasReturn ) {
                auto rblk = new ExprBlock();
                rblk->at = expr->at;
                rblk->isCollapseable = true;
                // close iterators before returning (mirrors normal end_loop path)
                for ( size_t si=0, sis=expr->sources.size(); si!=sis; ++si ) {
                    auto cbif = new ExprCall(expr->at, "_builtin_iterator_close");
                    cbif->generated = true;
                    cbif->arguments.push_back(new ExprVar(expr->at, srcNames[si]));
                    cbif->arguments.push_back(new ExprVar(expr->at, pVarNames[si]));
                    rblk->list.push_back(cbif);
                }
                auto pRet = new ExprReturn();
                pRet->at = expr->at;
                if ( func->result && !func->result->isVoid() ) {
                    pRet->subexpr = new ExprVar(expr->at, returnValueName);
                }
                rblk->list.push_back(pRet);
                auto rchk = new ExprIfThenElse(expr->at, new ExprVar(expr->at, returnFlag), rblk, nullptr);
                blk->list.push_back(rchk);
            }
            // if (__broke) goto end — skip iterator_next
            auto gtef = new ExprGoto(expr->at, end_loop_label);
            auto btef = new ExprBlock();
            btef->at = expr->at;
            btef->list.push_back(gtef);
            auto fchk = new ExprIfThenElse(expr->at, new ExprVar(expr->at, breakFlag), btef, nullptr);
            blk->list.push_back(fchk);
        }
        // loop &= _builtin_iterator_next(it0,pvar0)
        for ( size_t si=0, sis=expr->sources.size(); si!=sis; ++si ) {
            const string & srcName = srcNames[si];
            const string & pVarName = pVarNames[si];
            auto cbif = new ExprCall(expr->at, "_builtin_iterator_next");
            cbif->generated = true;
            cbif->arguments.push_back(new ExprVar(expr->at, srcName));
            cbif->arguments.push_back(new ExprVar(expr->at, pVarName));
            auto lande = new ExprOp2(expr->at,"&&=",
                                              new ExprVar(expr->at,loopVar),cbif);
            blk->list.push_back(lande);
        }
        auto gbeg = new ExprGoto(expr->at, begin_loop_label);
        blk->list.push_back(gbeg);
        auto ell = new ExprLabel(expr->at, end_loop_label,
                                          "end for at line " + to_string(expr->at.line));
        blk->list.push_back(ell);
        // loop &= _builtin_iterator_close(it0,pvar0)
        for ( size_t si=0, sis=expr->sources.size(); si!=sis; ++si ) {
            const string & srcName = srcNames[si];
            const string & pVarName = pVarNames[si];
            auto cbif = new ExprCall(expr->at, "_builtin_iterator_close");
            cbif->generated = true;
            cbif->arguments.push_back(new ExprVar(expr->at, srcName));
            cbif->arguments.push_back(new ExprVar(expr->at, pVarName));
            blk->list.push_back(cbif);
        }
        verifyGenerated(blk);
        return blk;
    }

    FunctionPtr makeCloneTuple ( const LineInfo & at, const TypeDeclPtr & tupleType, bool fromConst ) {
        DAS_ASSERT(tupleType->isTuple() && "can only clone tuple");
        auto fn = new Function();
        fn->generated = true;
        fn->safeImplicit = true;
        fn->privateFunction = true;
        fn->name = "clone";
        fn->at = fn->atDecl = at;
        fn->result = new TypeDecl(Type::tVoid);
        auto arg0 = new Variable();
        arg0->at = at;
        arg0->name = "dest";
        arg0->type = new TypeDecl(*tupleType);
        arg0->type->constant = false;
        arg0->type->explicitConst = false;
        arg0->type->ref = false;
        fn->arguments.push_back(arg0);
        auto arg1 = new Variable();
        arg1->at = at;
        arg1->name = "src";
        arg1->type = new TypeDecl(*tupleType);
        arg1->type->constant = fromConst;
        arg1->type->explicitConst = true;
        arg1->type->ref = false;
        arg1->type->implicit = true;
        fn->arguments.push_back(arg1);
        auto block = new ExprBlock();
        block->at = at;
        for ( size_t argi=0, argis=tupleType->argTypes.size(); argi!=argis; ++argi ) {
            string argn = "_" + to_string(argi);
            auto lv = new ExprVar(at, "dest");
            auto lf = new ExprField(at, lv, argn);
            auto rv = new ExprVar(at, "src");
            auto rf = new ExprField(at, rv, argn);
            auto cl = new ExprClone(at, lf, rf);
            block->list.push_back(cl);
        }
        fn->body = block;
        verifyGenerated(fn->body);
        return fn;
    }

    FunctionPtr generateTupleFinalizer ( const LineInfo & at, const TypeDeclPtr & tupleType ) {
        DAS_ASSERT(tupleType->isTuple() && "can only finalize tuple");
        auto fn = new Function();
        fn->privateFunction = true;
        fn->generated = true;
        fn->name = "finalize";
        fn->at = fn->atDecl = at;
        fn->result = new TypeDecl(Type::tVoid);
        auto arg0 = new Variable();
        arg0->at = at;
        arg0->name = "__this";
        arg0->type = new TypeDecl(*tupleType);
        arg0->type->constant = false;
        arg0->type->ref = false;
        arg0->type->isExplicit = true;
        fn->arguments.push_back(arg0);
        auto block = new ExprBlock();
        block->at = at;
        bool needUnsafe = false;
        for ( size_t argi=0, argis=tupleType->argTypes.size(); argi!=argis; ++argi ) {
            if ( !tupleType->argTypes[argi]->constant && tupleType->argTypes[argi]->needDelete() ) {
                if ( tupleType->isPointer() && tupleType->firstType && tupleType->firstType->constant ) continue;
                string argn = "_" + to_string(argi);
                auto lv = new ExprVar(at, "__this");
                auto lf = new ExprField(at, lv, argn);
                auto cl = new ExprDelete(at, lf);
                cl->alwaysSafe = true;
                block->list.push_back(cl);
                if ( tupleType->argTypes[argi]->isPointer() ) {
                    needUnsafe = true;
                }
            }
        }
        auto mz = new ExprMemZero(at, "memzero");
        auto lvar = new ExprVar(at, "__this");
        mz->arguments.push_back(lvar);
        block->list.push_back(mz);
        fn->body = block;
        if ( needUnsafe ) {
            wrapInUnsafe(fn);
        }
        verifyGenerated(fn->body);
        return fn;
    }

    FunctionPtr makeCloneVariant ( const LineInfo & at, const TypeDeclPtr & variantType, bool fromConst ) {
        DAS_ASSERT(variantType->isVariant() && "can only clone variant");
        auto fn = new Function();
        fn->generated = true;
        fn->safeImplicit = true;
        fn->privateFunction = true;
        fn->name = "clone";
        fn->at = fn->atDecl = at;
        fn->result = new TypeDecl(Type::tVoid);
        auto arg0 = new Variable();
        arg0->at = at;
        arg0->name = "dest";
        arg0->type = new TypeDecl(*variantType);
        arg0->type->constant = false;
        arg0->type->explicitConst = false;
        arg0->type->ref = false;
        fn->arguments.push_back(arg0);
        auto arg1 = new Variable();
        arg1->at = at;
        arg1->name = "src";
        arg1->type = new TypeDecl(*variantType);
        arg1->type->constant = fromConst;
        arg1->type->explicitConst = true;
        arg1->type->ref = false;
        arg1->type->implicit = true;
        fn->arguments.push_back(arg1);
        auto block = new ExprBlock();
        block->at = at;
        ExprIfThenElse * topIf = nullptr;
        ExprIfThenElse * lastIf = nullptr;
        for ( size_t argi=0, argis=variantType->argTypes.size(); argi!=argis; ++argi ) {
            const string & argn = variantType->argNames[argi];
            auto cb = new ExprBlock();
            cb->at = at;
            auto vd = new ExprVar(at, "dest");
            auto vi = new ExprConstInt(at, int32_t(argi));
            auto svi = new ExprCall(at, "set_variant_index");
            svi->alwaysSafe = true;
            svi->arguments.push_back(vd);
            svi->arguments.push_back(vi);
            cb->list.push_back(svi);
            auto lv = new ExprVar(at, "dest");
            auto lf = new ExprField(at, lv, argn);
            lf->alwaysSafe = true;
            auto rv = new ExprVar(at, "src");
            auto rf = new ExprField(at, rv, argn);
            rf->alwaysSafe = true;
            auto cl = new ExprClone(at, lf, rf);
            cb->list.push_back(cl);
            auto av = new ExprVar(at, "src");
            auto isv = new ExprIsVariant(at, av, argn);
            auto thisIf = new ExprIfThenElse(at, isv, cb, nullptr);
            if ( lastIf ) {
                lastIf->if_false = thisIf;
                lastIf = thisIf;
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
        auto fn = new Function();
        fn->privateFunction = true;
        fn->generated = true;
        fn->name = "finalize";
        fn->at = fn->atDecl = at;
        fn->result = new TypeDecl(Type::tVoid);
        auto arg0 = new Variable();
        arg0->at = at;
        arg0->name = "__this";
        arg0->type = new TypeDecl(*variantType);
        arg0->type->constant = false;
        arg0->type->ref = false;
        arg0->type->isExplicit = true;
        fn->arguments.push_back(arg0);
        auto block = new ExprBlock();
        block->at = at;
        ExprIfThenElse * topIf = nullptr;
        ExprIfThenElse * lastIf = nullptr;
        bool needUnsafe = false;
        for ( size_t argi=0, argis=variantType->argTypes.size(); argi!=argis; ++argi ) {
            if ( !variantType->argTypes[argi]->constant && variantType->argTypes[argi]->needDelete() ) {
                if ( variantType->argTypes[argi]->isPointer() && variantType->argTypes[argi]->firstType
                    && variantType->argTypes[argi]->firstType->constant ) continue;
                const string & argn = variantType->argNames[argi];
                auto lv = new ExprVar(at, "__this");
                auto lf = new ExprField(at, lv, argn);
                lf->alwaysSafe = true;
                auto cl = new ExprDelete(at, lf);
                cl->alwaysSafe = true;
                auto cb = new ExprBlock();
                cb->at = at;
                cb->list.push_back(cl);
                auto av = new ExprVar(at, "__this");
                auto isv = new ExprIsVariant(at, av, argn);
                auto thisIf = new ExprIfThenElse(at, isv, cb, nullptr);
                if ( lastIf ) {
                    lastIf->if_false = thisIf;
                    lastIf = thisIf;
                } else {
                    topIf = lastIf = thisIf;
                }
                if ( variantType->argTypes[argi]->isPointer() ) {
                    needUnsafe = true;
                }
            }
        }
        if (topIf) block->list.push_back(topIf);
        auto mz = new ExprMemZero(at, "memzero");
        auto lvar = new ExprVar(at, "__this");
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
        auto fn = new Function();
        fn->generated = true;
        fn->safeImplicit = true;
        fn->privateFunction = true;
        fn->name = "clone";
        fn->at = fn->atDecl = at;
        fn->result = new TypeDecl(Type::tVoid);
        auto arg0 = new Variable();
        arg0->at = at;
        arg0->name = "dest";
        arg0->type = new TypeDecl(*left);
        arg0->type->constant = false;
        arg0->type->explicitConst = false;
        arg0->type->ref = true;
        fn->arguments.push_back(arg0);
        auto arg1 = new Variable();
        arg1->at = at;
        arg1->name = "src";
        arg1->type = new TypeDecl(*right);
        arg1->type->constant = true;
        arg1->type->ref = false;
        arg1->type->implicit = true;
        arg1->type->explicitConst = false;
        fn->arguments.push_back(arg1);
        auto block = new ExprBlock();
        block->at = at;
        auto lv = new ExprVar(at, "dest");
        auto rv = new ExprVar(at, "src");
        auto cl = new ExprCall(at, left->firstType->annotation->getSmartAnnotationCloneFunction());
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

    ExpressionPtr forceAt ( ExpressionPtr expr, const LineInfo & at ) {
        LocationSwapVisitor swapAt(at);
        return expr->visit(swapAt);
    }

    FunctionPtr forceAtFunction ( const FunctionPtr & func, const LineInfo & at ) {
        LocationSwapVisitor swapAt(at);
        return func->visit(swapAt);
    }

    ExpressionPtr forceGenerated ( ExpressionPtr expr, bool setGenerated ) {
        SetGeneratedVisitor setGen(setGenerated);
        return expr->visit(setGen);
    }

    FunctionPtr forceGeneratedFunction ( const FunctionPtr & expr, bool setGenerated ) {
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

    LineInfo encloseAt ( ExpressionPtr expr ) {
        EncloseVisitor enc;
        expr->visit(enc);
        return enc.enclosure;
    }

    void modifyToClassMember ( Function * func, Structure * baseClass, bool isExplicit, bool isConstant ) {
        // first argument is this
        auto argT = new TypeDecl(baseClass);
        argT->constant = isConstant;
        argT->isExplicit = isExplicit;
        auto argV = new Variable();
        argV->name = "self";
        argV->type = argT;
        argV->at = func->at;
        argV->generated = true;
        argV->capture_as_ref = true;
        func->arguments.insert(func->arguments.begin(), argV);
        // with self ...
        auto block = new ExprBlock();
        block->at = func->at;
        auto wth = new ExprWith();
        wth->at = func->at;
        auto wvar = new ExprVar(func->at,"self");
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
        auto func = new Function();
        func->isTemplate = baseClass->isTemplate;
        func->generated = true;
        func->at = method->at;
        func->atDecl = method->at;
        func->name = baseClass->name;
        func->result = new TypeDecl(baseClass);
        func->isClassMethod = true;
        func->classParent = baseClass;
        DAS_ASSERT(func->classParent);
        if ( baseClass->macroInterface ) func->macroFunction = true;
        auto block = new ExprBlock();
        block->at = func->at;
        func->body = block;
        for ( auto & arg : method->arguments ) {
            func->arguments.push_back(arg->clone());
        }
        // lef self = [[Foo()]]
        auto makeT = new ExprMakeStruct(baseClass->at);
        makeT->alwaysSafe = true;
        makeT->at = func->at;
        makeT->useInitializer = true;
        makeT->nativeClassInitializer = true;
        makeT->makeType = new TypeDecl(baseClass);
        makeT->structs.push_back(new MakeStruct());
        auto letS = new ExprLet();
        letS->at = func->at;
        letS->atInit = func->at;
        letS->visibility = func->atDecl;
        letS->alwaysSafe = true;    // this is due to local class variable
        auto argT = new TypeDecl(baseClass);
        argT->constant = false;
        auto argV = new Variable();
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
        auto cll = new ExprCall(func->at,baseClass->name+"`"+baseClass->name);
        cll->arguments.push_back(new ExprVar(func->at,"self"));
        for ( auto & arg : method->arguments ) {
            cll->arguments.push_back(new ExprVar(func->at,arg->name));
        }
        block->list.push_back(cll);
        // return self
        auto selfV = new ExprVar(baseClass->at,"self");
        selfV->at = func->at;
        auto returnDecl = new ExprReturn(baseClass->at,selfV);
        returnDecl->at = func->at;
        returnDecl->moveSemantics = true;
        block->list.push_back(returnDecl);
        // and done
        func->body = block;
        verifyGenerated(func->body);
        return func;
    }

    void makeClassRtti ( Structure * baseClass ) {
        ExpressionPtr finit = new ExprTypeInfo(baseClass->at, "rtti_classinfo", new TypeDecl(baseClass));
        if ( baseClass->parent ) {
            auto fd = (Structure::FieldDeclaration *) baseClass->findField("__rtti");
            fd->init = finit;
            fd->parentType = fd->type->isAuto();
            fd->generated = true;
        } else {
            auto pvoid = new TypeDecl(Type::tPointer);
            pvoid->firstType = new TypeDecl(Type::tVoid);
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
        ExpressionPtr finit = new ExprAddr(baseClass->at, "_::" + fname);
        if ( baseClass->parent ) {
            auto fd = (Structure::FieldDeclaration *) baseClass->findField("__finalize");
            fd->init = new ExprCast(baseClass->at, finit, new TypeDecl(Type::autoinfer));
            fd->parentType = fd->type->isAuto();
            fd->generated = true;
        } else {
            baseClass->fields.emplace_back(
                "__finalize",
                new TypeDecl(Type::autoinfer),
                finit,
                AnnotationArgumentList(),
                false,
                baseClass->at
            );
            baseClass->fields.back().generated = true;
        }
        // make a function
        auto func = new Function();
        func->generated = true;
        func->at = baseClass->at;
        func->atDecl = baseClass->at;
        func->name = fname;
        func->result = new TypeDecl(Type::tVoid);
        func->isClassMethod = true;
        func->classParent = baseClass;
        DAS_ASSERT(func->classParent);
        if ( baseClass->macroInterface ) func->macroFunction = true;
        auto block = new ExprBlock();
        block->at = func->at;
        func->body = block;
        // only one argument, and its 'self'
        auto argT = new TypeDecl(baseClass);
        argT->constant = false;
        auto argV = new Variable();
        argV->name = "self";
        argV->type = argT;
        argV->at = func->at;
        argV->generated = true;
        argV->capture_as_ref = true;
        func->arguments.push_back(argV);
        // delete
        auto vself = new ExprVar(func->at, "self");
        auto edel = new ExprDelete(func->at, vself);
        edel->alwaysSafe = true;
        block->list.push_back(edel);
        // and done
        verifyGenerated(func->body);
        return func;
    }

    ExpressionPtr convertToCloneExpr ( ExprMakeStruct * expr, int index, MakeFieldDecl * decl, bool ignoreCaptureConst ) {
        bool needIndex = expr->structs.size()>1;
        DAS_ASSERT(expr->block->rtti_isMakeBlock());
        auto mkb = static_cast<ExprMakeBlock*>(expr->block);
        DAS_ASSERT(mkb->block->rtti_isBlock());
        auto blk = static_cast<ExprBlock*>(mkb->block);
        DAS_ASSERT(blk->arguments.size()==1);
        auto selfName = blk->arguments[0]->name;
        if ( !needIndex ) {
            auto vself = new ExprVar(decl->at, selfName);
            auto fdecl = new ExprField(decl->at, vself, decl->name);
            fdecl->ignoreCaptureConst = ignoreCaptureConst;
            auto op2c = new ExprClone(decl->at, fdecl, decl->value->clone());
            return op2c;
        } else {
            auto vself = new ExprVar(decl->at, selfName);
            auto cidx = new ExprConstInt(decl->at, index);
            auto vat = new ExprAt(decl->at, vself, cidx);
            auto fdecl = new ExprField(decl->at, vat, decl->name);
            auto op2c = new ExprClone(decl->at, fdecl, decl->value->clone());
            return op2c;
        }
    }

    ExpressionPtr makeStructWhereBlock ( ExprMakeStruct * mks ) {
        // make a block
        auto block = new ExprBlock();
        block->at = mks->at;
        block->isClosure = true;
        block->returnType = new TypeDecl(Type::tVoid);
        // only one argument, and its 'self'
        auto argT = new TypeDecl(*(mks->makeType));
        argT->constant = false;
        if ( mks->structs.size() > 1 ) {
            argT->dim.push_back(int32_t(mks->structs.size()));
        }
        auto argV = new Variable();
        argV->name = "__self__" + to_string(mks->at.line) + "_" + to_string(mks->at.column);
        argV->can_shadow = mks->canShadowBlock;
        argV->type = argT;
        argV->at = mks->at;
        argV->generated = true;
        block->arguments.push_back(argV);
        // make-block
        auto mkb = new ExprMakeBlock(mks->at,block,false);
        // and done
        verifyGenerated(mkb);
        return mkb;
    }

    void assignDefaultArguments ( Function * func ) {
        for ( auto & arg : func->arguments ) {
            if ( arg->type->baseType==Type::fakeContext ) {
                arg->init = new ExprFakeContext(arg->at);
                arg->init->generated = true;
            } else if ( arg->type->baseType==Type::fakeLineInfo ) {
                arg->init = new ExprFakeLineInfo(arg->at);
                arg->init->generated = true;
            }
        }
    }
}

