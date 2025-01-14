#include "daScript/misc/platform.h"

#include "daScript/ast/ast.h"
#include "daScript/ast/ast_visitor.h"

namespace das {

    typedef das_hash_map<Variable *,Function *> IndirectSources;
    typedef das_hash_set<Variable *> ExpressionSources;

    void appendIndVariables ( Function * lookup, IndirectSources & indGlobalVariables, das_hash_set<Function *> & accessed ) {
        if ( accessed.find(lookup)!=accessed.end() ) return;
        accessed.insert(lookup);
        for ( auto var : lookup->useGlobalVariables ) {
            if ( indGlobalVariables.find(var)==indGlobalVariables.end() ) {
                indGlobalVariables[var] = lookup;
            }
        }
        for ( auto fun : lookup->useFunctions ) {
            appendIndVariables(fun, indGlobalVariables, accessed);
        }
    }

    bool isCompatibleFunction ( const FunctionPtr & func, const TypeDeclPtr & inv ) {
        if ( func->arguments.size()!=inv->argTypes.size() ) return false;
        if ( !inv->firstType->isSameType(*func->result, RefMatters::yes, ConstMatters::yes, TemporaryMatters::yes) ) {
            return false;
        }
        for ( size_t i=0, is=func->arguments.size(); i!=is; ++i ) {
            if ( !inv->argTypes[i]->isSameType(*func->arguments[i]->type, RefMatters::yes, ConstMatters::yes, TemporaryMatters::yes, AllowSubstitute::yes, false, false) ) {
                return false;
            }
        }
        return true;
    }

    void collectInvokeIndVariables ( const ProgramPtr & program, const TypeDeclPtr & inv, IndirectSources & sources ) {
        das_hash_set<Function *> accessed;
        program->library.foreach([&](Module * mod){
            ExpressionSources globSrc;
            mod->functions.foreach([&](const FunctionPtr & gfunc){
                // not built-in, used, address taken, can potentially alias, compatible
                if ( !gfunc->builtIn && gfunc->used && gfunc->addressTaken && !gfunc->aliasCMRES && isCompatibleFunction(gfunc, inv) ) {
                    appendIndVariables(gfunc.get(), sources, accessed);
                }
            });
            return true;
        },"*");
    }

    bool isCompatibleLambdaFunction ( const FunctionPtr & func, const TypeDeclPtr & inv ) {
        if ( func->arguments.size()!=inv->argTypes.size()+1 ) return false;
        if ( !inv->firstType->isSameType(*func->result, RefMatters::yes, ConstMatters::yes, TemporaryMatters::yes) ) {
            return false;
        }
        for ( size_t i=0, is=inv->argTypes.size(); i!=is; ++i ) {    // note, we skip 1st argument
            if ( !inv->argTypes[i]->isSameType(*func->arguments[i+1]->type, RefMatters::yes, ConstMatters::yes, TemporaryMatters::yes, AllowSubstitute::yes, false, false) ) {
                return false;
            }
        }
        return true;
    }

    void collectInvokeIndLambdaVariables ( const ProgramPtr & program, const TypeDeclPtr & inv, IndirectSources & sources ) {
        das_hash_set<Function *> accessed;
        program->library.foreach([&](Module * mod){
            ExpressionSources globSrc;
            mod->functions.foreach([&](const FunctionPtr & gfunc){
                // not built-in, used, address taken, can potentially alias, compatible
                if ( !gfunc->builtIn && gfunc->used && gfunc->addressTaken && !gfunc->aliasCMRES && gfunc->lambda && isCompatibleLambdaFunction(gfunc, inv) ) {
                    appendIndVariables(gfunc.get(), sources, accessed);
                }
            });
            return true;
        },"*");
    }

    class SourceCollector : public Visitor {
    public:
        SourceCollector ( const ProgramPtr & prog, bool anyC, bool anyG ) : program(prog), anyCallAliasing(anyC), anyGlobals(anyG) {}
    protected:
        virtual bool canVisitStructureFieldInit ( Structure * ) override { return false; }
        virtual bool canVisitArgumentInit ( Function * , const VariablePtr &, Expression * ) override { return false; }
    // this speeds up walking
        virtual bool canVisitIfSubexpr ( ExprIfThenElse * ) override {
            return !disabled;
        }
        virtual bool canVisitExpr ( ExprTypeInfo *, Expression * )  override  { return false; }
        virtual bool canVisitMakeStructureBlock ( ExprMakeStruct *, Expression * )  override  { return false; }
        virtual bool canVisitMakeStructureBody ( ExprMakeStruct * )  override {
            return !disabled;
        }
        virtual bool canVisitQuoteSubexpression ( ExprQuote * )  override  { return false; }
        virtual bool canVisitWithAliasSubexpression ( ExprAssume * )  override  { return false; }
        virtual bool canVisitMakeBlockBody ( ExprMakeBlock * )  override  { return false; }
    // we found source
        virtual void preVisit ( ExprVar * expr ) override {
            Visitor::preVisit(expr);
            if ( !disabled ) sources.insert(expr->variable.get());
        }
    // in [] only value can alias
        virtual void preVisit ( ExprAt * expr ) override {
            Visitor::preVisit(expr);
        }
        virtual void preVisitAtIndex ( ExprAt * expr, Expression * index ) override {
            Visitor::preVisitAtIndex(expr,index);
            disabled ++;
        }
        virtual ExpressionPtr visit ( ExprAt * expr ) override {
            disabled --;
            return Visitor::visit(expr);
        }
    // in ?[] only value can alias
        virtual void preVisit ( ExprSafeAt * expr ) override {
            Visitor::preVisit(expr);
        }
        virtual void preVisitSafeAtIndex ( ExprSafeAt * expr, Expression * index ) override {
            Visitor::preVisitSafeAtIndex(expr,index);
            disabled ++;
        }
        virtual ExpressionPtr visit ( ExprSafeAt * expr ) override {
            disabled --;
            return Visitor::visit(expr);
        }
    // in op3 only sources can alias
        virtual void preVisit ( ExprOp3 * expr ) override {
            Visitor::preVisit(expr);
            disabled ++;
        }
        virtual void preVisitLeft  ( ExprOp3 * expr, Expression * left ) override {
            Visitor::preVisitLeft(expr,left);
            disabled --;
        }
        virtual ExpressionPtr visit ( ExprOp3 * expr ) override {
            return Visitor::visit(expr);
        }
    // pointer deref - all bets are off
        virtual void preVisit ( ExprPtr2Ref * expr ) override {
            Visitor::preVisit(expr);
            if ( !expr->assumeNoAlias ) alwaysAliases = true;
        }
    // function call - does not alias when does not return ref
        virtual void preVisit ( ExprCall * expr ) override {
            Visitor::preVisit(expr);
            if ( anyCallAliasing ) {
                if ( !expr->func->result->isRef() ) disabled ++;
            } else {
                if ( !expr->func->result->ref ) disabled ++;
            }
        }
        virtual ExpressionPtr visit ( ExprCall * expr ) override {
            if ( anyCallAliasing ) {
                if ( !expr->func->result->isRef() ) disabled --;
            } else {
                if ( !expr->func->result->ref ) disabled --;
            }
            return Visitor::visit(expr);
        }
    // invoke
        virtual void preVisit ( ExprInvoke * expr ) override {
            Visitor::preVisit(expr);
            auto argT = expr->arguments[0]->type;
            if ( argT->isGoodBlockType() ) {
                disabled ++; // block never aliases
            } else if ( anyCallAliasing ) {
                if ( !argT->firstType->isRef() ) disabled ++;
            } else {
                if ( !argT->firstType->ref ) disabled ++;
            }
            if ( !disabled && anyGlobals ) {
                if ( argT->isGoodFunctionType() ) {
                    IndirectSources globSrc;
                    collectInvokeIndVariables(program, expr->arguments[0]->type, globSrc);
                    for ( auto & src : globSrc ) {
                        sources.insert(src.first);
                    }
                } else if ( argT->isGoodLambdaType() ) {
                    IndirectSources globSrc;
                    collectInvokeIndLambdaVariables(program, expr->arguments[0]->type, globSrc);
                    for ( auto & src : globSrc ) {
                        sources.insert(src.first);
                    }
                }
            }
        }
        virtual ExpressionPtr visit ( ExprInvoke * expr ) override {
            auto argT = expr->arguments[0]->type;
            if ( argT->isGoodBlockType() ) {
                disabled --;
            } else if ( anyCallAliasing ) {
                if ( !argT->firstType->isRef() ) disabled --;
            } else {
                if ( !argT->firstType->ref ) disabled --;
            }
            return Visitor::visit(expr);
        }
    protected:
        int disabled = 0;
        ProgramPtr program;
        set<Expression *> invokeBlocks;
    public:
        ExpressionSources sources;
        bool alwaysAliases = false;
        bool anyCallAliasing = false;
        bool anyGlobals = false;
    };

    bool collectSources ( const ProgramPtr & prog, Expression * expr, ExpressionSources & src, bool anyC, bool anyG ) {
        SourceCollector srr(prog, anyC, anyG);
        expr->visit(srr);
        src = das::move(srr.sources);
        return srr.alwaysAliases;
    }

    bool appendSources ( const ProgramPtr & prog, Expression * expr, ExpressionSources & src, bool anyC, bool anyG ) {
        SourceCollector srr(prog, anyC, anyG);
        expr->visit(srr);
        for ( auto s : srr.sources ) {
            src.insert(s);
        }
        return srr.alwaysAliases;
    }

    Variable * intersectSources ( ExpressionSources & a, ExpressionSources & b ) {
        if ( a.size() < b.size() ) {
            for ( auto va : a ) {
                if ( b.find(va)!=b.end() ) return va;
            }
            return nullptr;
        } else {
            for ( auto vb : b ) {
                if ( a.find(vb)!=a.end() ) return vb;
            }
            return nullptr;
        }
    }

    void deriveAliasing ( const FunctionPtr & func, TextWriter & logs, bool logAliasing ) {
        if ( func->arguments.size()==0 && func->useGlobalVariables.size()==0 && func->useFunctions.size()==0 ) return;
    // collect indirect global variables
        IndirectSources ind;
        das_hash_set<Function *> depInd;
        appendIndVariables(func.get(),ind,depInd);
        if ( !(func->copyOnReturn || func->moveOnReturn) ) return;  // not a cmres function, we don't need cmres aliasing
        if ( func->arguments.size()==0 && ind.size()==0 ) return;   // no arguments, no globals, no cmres aliasing
    // collect type aliasing
        if ( logAliasing ) logs << "function " << func->getMangledName() << " returns by reference\n";
        das_set<Structure *> rdep;
        TypeAliasMap resTypeAliases;
        func->result->collectAliasing(resTypeAliases, rdep, false);
    // do arguments
        func->resultAliases.clear();
        for ( size_t i=0, is=func->arguments.size(); i!=is; ++i ) {
            if ( !(func->arguments[i]->type->isRef() || func->arguments[i]->type->baseType==Type::tPointer) ) {
                continue;
            }
            das_set<Structure *> dep;
            TypeAliasMap typeAliases;
            func->arguments[i]->type->collectAliasing(typeAliases, dep, false);
            for ( auto resT : resTypeAliases ) {
                for ( auto alias : typeAliases ) {
                    if ( alias.second.first->isSameType(*(resT.second.first),RefMatters::no,ConstMatters::no,TemporaryMatters::no,AllowSubstitute::yes,false,false) ) {
                        func->resultAliases.push_back(int(i));
                        if ( logAliasing ) logs << "\targument " << i << " aliasing result with type "
                            << func->arguments[i]->type->describe() << "\n";
                        goto nada;
                    }
                }
            }
            nada:;
        }
    // do globals
        func->resultAliasesGlobals.clear();
        for ( auto & it : ind ) {
            if ( !(it.first->type->isRef() || it.first->type->baseType==Type::tPointer) || it.first->type->temporary) {
                continue;
            }
            das_set<Structure *> dep;
            TypeAliasMap typeAliases;
            it.first->type->collectAliasing(typeAliases, dep, false);
            for ( auto resT : resTypeAliases ) {
                for ( auto alias : typeAliases ) {
                    if ( alias.second.first->isSameType(*(resT.second.first),RefMatters::no,ConstMatters::no,TemporaryMatters::no,AllowSubstitute::yes,false,false) ) {
                        Function::AliasInfo info = {it.first, it.second, resT.second.second || alias.second.second};
                        func->resultAliasesGlobals.emplace_back(info);
                        if ( logAliasing ) logs << "\tglobal variable " << it.first->getMangledName() << " aliasing result with type "
                            << it.first->type->describe() << (info.viaPointer ? " through pointer aliasing" : "") <<  "\n";
                        goto nadaHere;
                    }
                }
            }
            nadaHere:;
        }
    }

    class AliasMarker : public Visitor {
    public:
        AliasMarker ( const ProgramPtr & prog, bool permanent, bool everything, TextWriter & ls ) : program(prog), logs(ls) {
            checkAliasing = program->options.getBoolOption("no_aliasing", program->policies.no_aliasing);
            logAliasing = program->options.getBoolOption("log_aliasing", false);
            isPermanent = permanent;
            isEverything = everything;
        }
    protected:
        ProgramPtr  program;
        das_hash_map<Expression *,Expression *> cmresDest;
        bool checkAliasing = false;
        bool logAliasing = false;
        TextWriter & logs;
        bool isPermanent = false;
        bool isEverything = false;
    protected:
        virtual bool canVisitStructureFieldInit ( Structure * ) override { return false; }
        virtual bool canVisitArgumentInit ( Function * , const VariablePtr &, Expression * ) override { return false; }
        virtual bool canVisitQuoteSubexpression ( ExprQuote * ) override { return false; }
        virtual bool canVisitGlobalVariable ( Variable * var ) override {
            if ( var->aliasesResolved ) return false;
            if ( !var->used && !isEverything ) return false;
            var->aliasesResolved = isPermanent;
            return true;
        }
        virtual bool canVisitFunction ( Function * fun ) override {
            if ( fun->aliasesResolved ) return false;
            if ( !fun->used && !isEverything ) return false;
            fun->aliasesResolved = isPermanent;
            return true;
        }
    protected:
        virtual void preVisit ( ExprCopy * expr ) override {
            Visitor::preVisit(expr);
            cmresDest[expr->right.get()] = expr->left.get();
        }
        virtual void preVisit ( ExprMove * expr ) override {
            Visitor::preVisit(expr);
            cmresDest[expr->right.get()] = expr->left.get();
        }
        virtual FunctionPtr visit ( Function * fn ) override {
            cmresDest.clear();
            return Visitor::visit(fn);
        }
    // make local
        template <typename TT>
        void preVisitMakeLocal ( ExprMakeLocal * expr, TT && collectAliasSources ) {
            auto it = cmresDest.find(expr);
            if ( it!=cmresDest.end() ) {
                // left
                auto resOut = it->second;
                ExpressionSources resSrc;
                if ( collectSources(program, resOut, resSrc, false, true) ) {
                    if ( checkAliasing ) {
                        program->error("[[" + string(expr->__rtti) + " ]] always aliases",
                            "some form of ... *ptr ... = [[" + string(expr->__rtti) + " ...]] where we don't know where pointer came from", "",
                                expr->at, CompilationError::make_local_aliasing);
                    } else {
                        expr->alwaysAlias = true;
                    }
                    return;
                }
                // check if there are any globals
                bool anyGlobals = false;
                for ( auto v : resSrc ) {
                    if ( v->global ) {
                        anyGlobals = true;
                        break;
                    }
                }
                // right
                ExpressionSources argSrc;
                if ( collectAliasSources(argSrc,anyGlobals) ) {
                    if ( checkAliasing ) {
                        program->error("[[" + string(expr->__rtti) + " ]] always aliases",
                            "some form of ... = [[" + string(expr->__rtti) + " ... *ptr ... ]] where we don't know where pointer came from", "",
                                expr->at, CompilationError::make_local_aliasing);
                    } else {
                        expr->alwaysAlias = true;
                    }
                } else if ( auto aliasVar = intersectSources ( resSrc, argSrc ) ) {
                    if ( checkAliasing ) {
                        program->error("[[" + string(expr->__rtti) + " ]] aliases",
                            "some form of ... " + aliasVar->name + " ... = [[" + string(expr->__rtti) + " ... " + aliasVar->name + " ... ]]", "",
                                expr->at, CompilationError::make_local_aliasing);
                    } else {
                        expr->alwaysAlias = true;
                    }
                }
            }
        }
        virtual void preVisit ( ExprMakeArray * expr ) override {
            Visitor::preVisit(expr);
            preVisitMakeLocal(expr,[&](ExpressionSources & argSrc, bool anyGlobals) -> bool {
                for ( auto & arg : expr->values ) {
                    if ( appendSources(program, arg.get(), argSrc, true, anyGlobals) ) return true;
                }
                return false;
            });
        }
        virtual void preVisit ( ExprMakeStruct * expr ) override {
            Visitor::preVisit(expr);
            preVisitMakeLocal(expr,[&](ExpressionSources & argSrc, bool anyGlobals) -> bool {
                for ( auto & arg : expr->structs ) {
                    for ( auto & subarg : *arg ) {
                        if ( appendSources(program, subarg->value.get(), argSrc, true, anyGlobals) ) return true;
                    }
                }
                return false;
            });
        }
        virtual void preVisit ( ExprMakeTuple * expr ) override {
            Visitor::preVisit(expr);
            preVisitMakeLocal(expr,[&](ExpressionSources & argSrc, bool anyGlobals) -> bool {
                for ( auto & arg : expr->values ) {
                    if ( appendSources(program, arg.get(), argSrc, true, anyGlobals) ) return true;
                }
                return false;
            });
        }
        virtual void preVisit ( ExprMakeVariant * expr ) override {
            Visitor::preVisit(expr);
            preVisitMakeLocal(expr,[&](ExpressionSources & argSrc, bool anyGlobals) -> bool {
                for ( auto & arg : expr->variants ) {
                    if ( appendSources(program, arg->value.get(), argSrc, true, anyGlobals) ) return true;
                }
                return false;
            });
        }
    // invoke
        virtual void preVisit ( ExprInvoke * expr ) override {
            Visitor::preVisit(expr);
            auto it = cmresDest.find(expr);
            if ( it!=cmresDest.end() ) {
                auto argT = expr->arguments[0]->type;
                if ( argT->isGoodBlockType() ) {
                    return; // block never aliases
                }
                if ( !(argT->firstType->isRefType() && !argT->firstType->ref) ) {
                    return; // not a CMRES
                }
                // everything can alias on the right
                auto resOut = it->second;
                ExpressionSources resSrc;
                if ( collectSources(program, resOut, resSrc, false, true) ) {
                    if ( checkAliasing ) {
                        program->error("invoke result always aliases",
                            "some form of ... *ptr ... = invoke( ... ) where we don't know where pointer came from", "",
                                expr->at, CompilationError::argument_aliasing);
                    } else {
                        expr->cmresAlias = true;
                        return;
                    }
                }
                // check if there are any globals
                bool anyGlobals = false;
                for ( auto v : resSrc ) {
                    if ( v->global ) {
                        anyGlobals = true;
                        break;
                    }
                }
                // invoke arguments can only alias invoke of function or lambda
                for ( size_t ai=0, ais=expr->arguments.size(); ai!=ais; ++ai ) {
                    if ( !(expr->arguments[ai]->type->isRef() || expr->arguments[ai]->type->baseType==Type::tPointer) ) {
                        continue;
                    }
                    ExpressionSources argSrc;
                    if ( collectSources(program, expr->arguments[ai].get(), argSrc, false, anyGlobals) ) {
                        if ( checkAliasing ) {
                            program->error("invoke result aliases argument " + to_string(ai),
                                "some form of ... = invoke( ... *ptr ... ) where we don't know where pointer came from", "",
                                    expr->at, CompilationError::argument_aliasing);
                        } else {
                            expr->cmresAlias = true;
                            return;
                        }
                    } else if ( auto aliasVar = intersectSources ( resSrc, argSrc ) ) {
                        if ( checkAliasing ) {
                            program->error("invoke result aliases argument " + to_string(ai),
                                "some form of ... " + aliasVar->name + " ... = invoke( ... " + aliasVar->name + " ... )", "",
                                    expr->at, CompilationError::argument_aliasing);
                        } else {
                            expr->cmresAlias = true;
                            return;
                        }
                    }
                }
                // we never get invoke(@@....,....) because its converted into a call
                // we can analyze functions by signature, and collect all global variables referenced by matching functions
                if ( anyGlobals) {
                    IndirectSources globSrc;
                    if ( argT->isGoodFunctionType() ) {
                        collectInvokeIndVariables(program, expr->arguments[0]->type, globSrc);
                    } else {
                        collectInvokeIndLambdaVariables(program, expr->arguments[0]->type, globSrc);
                    }
                    for ( auto gIt : globSrc ) {
                        if ( resSrc.find(gIt.first)!=resSrc.end() ) {
                            if ( checkAliasing ) {
                                if ( argT->isGoodFunctionType() ) {
                                    program->error("invoke result potentially aliases global " + gIt.first->name + " through function " + gIt.second->getMangledName(),
                                        "some form of ... " + gIt.first->name + " ... = invoke( ... " + gIt.first->name + " ... )", "",
                                            expr->at, CompilationError::argument_aliasing);
                                } else {
                                    program->error("invoke result potentially aliases global " + gIt.first->name + " through lambda at " + gIt.second->at.describe(),
                                        "some form of ... " + gIt.first->name + " ... = invoke( ... " + gIt.first->name + " ... )", "",
                                            expr->at, CompilationError::argument_aliasing);
                                }
                            } else {
                                expr->cmresAlias = true;
                                goto bailout;
                            }
                        }
                    }
                }
                bailout:;
            }
        }
    // call
        virtual void preVisit ( ExprCall * expr ) override {
            Visitor::preVisit(expr);
            if ( expr->func->aliasCMRES ) {
                expr->cmresAlias = true;
                goto bailout;
            }
            if ( !expr->func->neverAliasCMRES && (expr->func->resultAliases.size() || expr->func->resultAliasesGlobals.size()) ) {
                auto it = cmresDest.find(expr);
                if ( it!=cmresDest.end() ) {
                    auto resOut = it->second;
                    ExpressionSources resSrc;
                    if ( collectSources(program, resOut, resSrc, false, true) ) {
                        if ( checkAliasing ) {
                            program->error("function " + expr->func->describeName() + " result always aliases",
                                "some form of ... *ptr ... = " + expr->func->name + "( ... ) where we don't know where pointer came from", "",
                                    expr->at, CompilationError::argument_aliasing);
                        } else {
                            expr->cmresAlias = true;
                            goto bailout;
                        }
                    }
                    // check if there are any globals
                    bool anyGlobals = false;
                    for ( auto v : resSrc ) {
                        if ( v->global ) {
                            anyGlobals = true;
                            break;
                        }
                    }
                    // now go thorough all arguments, which are potential aliases
                    for ( auto ai : expr->func->resultAliases ) {
                        ExpressionSources argSrc;
                        if ( collectSources(program, expr->arguments[ai].get(), argSrc, false, anyGlobals) ) {
                            if ( checkAliasing ) {
                                program->error("function " + expr->func->describeName() + " result aliases argument " + expr->func->arguments[ai]->name,
                                    "some form of ... = " + expr->func->name + "( ... *ptr ... ) where we don't know where pointer came from", "",
                                        expr->at, CompilationError::argument_aliasing);
                            } else {
                                expr->cmresAlias = true;
                                goto bailout;
                            }
                        } else if ( auto aliasVar = intersectSources ( resSrc, argSrc ) ) {
                            if ( checkAliasing ) {
                                program->error("function " + expr->func->describeName() + " result aliases argument " + expr->func->arguments[ai]->name,
                                    "some form of ... " + aliasVar->name + " ... = " + expr->func->name + "( ... " + aliasVar->name + " ... )", "",
                                        expr->at, CompilationError::argument_aliasing);
                            } else {
                                expr->cmresAlias = true;
                                goto bailout;
                            }
                        }
                    }
                    for ( auto & vit : expr->func->resultAliasesGlobals) {
                        ExpressionSources argSrc;
                        argSrc.insert(vit.var);
                        if ( vit.viaPointer ) {
                            if ( checkAliasing ) {
                                program->error("function " + expr->func->describeName() + " result aliases global variable " + vit.var->getMangledName() + " through function " + vit.func->getMangledName(),
                                    "some form of ... = " + expr->func->name + "(...) where we don't know where the pointer in " + vit.var->name + " came from", "",
                                        expr->at, CompilationError::argument_aliasing);
                            } else {
                                expr->cmresAlias = true;
                                goto bailout;
                            }
                        } else if ( auto aliasVar = intersectSources ( resSrc, argSrc ) ) {
                            if ( checkAliasing ) {
                                string whereMessage;
                                if ( vit.func == expr->func ) {
                                    whereMessage = expr->func->name + "() uses " + vit.var->name;
                                } else {
                                    whereMessage = expr->func->name + "() indirectly calls " + vit.func->name + "() which uses " + vit.var->name;
                                }
                                program->error("function " + expr->func->describeName() + " result aliases global variable " + vit.var->getMangledName() + " through function " + vit.func->getMangledName(),
                                    "some form of ... " + aliasVar->name + " ... = " + expr->func->name + "(...) where " + whereMessage, "",
                                        expr->at, CompilationError::argument_aliasing);
                            } else {
                                expr->cmresAlias = true;
                                goto bailout;
                            }
                        }
                    }
                }
            }
        bailout:;
            if ( logAliasing && expr->cmresAlias ) {
                logs << expr->at.describe() << ": " << expr->func->describeName() << " aliases with CMRES, stack optimization disabled\n";
            }
        }
    };

    void Program::deriveAliases(TextWriter & logs, bool permanent, bool everything) {
        bool logAliasing = options.getBoolOption("log_aliasing", false);
        if ( logAliasing ) logs << "ALIASING:\n";

        library.foreach([&](Module * mod){
            if ( logAliasing ) logs << "module " << mod->name << ":\n";
            mod->functions.foreach([&](const FunctionPtr & func){
                if ( func->builtIn || !func->used ) return;
                deriveAliasing(func, logs, logAliasing);
            });
            return true;
        },"*");
        /*
        thisModule->functions.foreach([&](const FunctionPtr & func) {
            if ( func->builtIn || !func->used ) return;
            deriveAliasing(func, logs, logAliasing);
        });
        */
        AliasMarker marker(this, permanent, everything, logs);
        visit(marker);
        if ( logAliasing ) logs << "\n";
    }
}
