#include "daScript/misc/platform.h"

#include "daScript/ast/ast.h"
#include "daScript/ast/ast_visitor.h"

namespace das {

    class SetPrinterFlags : public Visitor {
    // ExprBlock
        virtual void preVisitBlockExpression ( ExprBlock * block, Expression * expr ) override {
            Visitor::preVisitBlockExpression(block, expr);
            expr->topLevel = true;
        }
    // ExprNew
        virtual void preVisitNewArg ( ExprNew * call, Expression * expr , bool last ) override {
            Visitor::preVisitNewArg(call,expr,last);
            expr->argLevel = true;
        }
    // ExprCall
        virtual void preVisitCallArg ( ExprCall * call, Expression * expr , bool last ) override {
            Visitor::preVisitCallArg(call,expr,last);
            expr->argLevel = true;
        }
    // ExprLooksLikeCall
        virtual void preVisitLooksLikeCallArg ( ExprLooksLikeCall * call, Expression * expr , bool last ) override {
            Visitor::preVisitLooksLikeCallArg(call,expr,last);
            expr->argLevel = true;
        }
    // ExprIfThenElse
        virtual void preVisit ( ExprIfThenElse * expr ) override {
            Visitor::preVisit(expr);
            expr->cond->argLevel = true;
        }
    // ExprWhile
        virtual void preVisit ( ExprWhile * expr ) override {
            Visitor::preVisit(expr);
            expr->cond->argLevel = true;
        }
    // ExprReturn
        virtual void preVisit ( ExprReturn * expr ) override {
            Visitor::preVisit(expr);
            if ( expr->subexpr ) expr->subexpr->argLevel = true;
        }
    // ExprCopy
        virtual void preVisit ( ExprCopy * expr ) override {
            Visitor::preVisit(expr);
            if ( expr->topLevel || expr->argLevel )
                expr->right->argLevel = true;
        }
    // ExprClone
        virtual void preVisit ( ExprClone * expr ) override {
            Visitor::preVisit(expr);
            if ( expr->topLevel || expr->argLevel )
                expr->right->argLevel = true;
        }
    // ExprVar
        virtual void preVisit ( ExprVar * expr ) override {
            Visitor::preVisit(expr);
            expr->bottomLevel = true;
        }
    // ExprTypeInfo
        virtual void preVisit ( ExprTypeInfo * expr ) override {
            Visitor::preVisit(expr);
            if ( expr->subexpr ) {
                expr->subexpr->argLevel = true;
            }
        }
    // ExprArrayComprehension
        virtual void preVisit ( ExprArrayComprehension * expr ) override {
            Visitor::preVisit(expr);
            expr->subexpr->argLevel = true;
            if ( expr->exprWhere ) expr->exprWhere->argLevel = true;
        }
    };

    class Printer : public Visitor {
    public:
        Printer ( const Program * program = nullptr ) {
            if ( program ) {
                printRef = program->options.getBoolOption("print_ref");
                printVarAccess = program->options.getBoolOption("print_var_access");
                printAliases= program->options.getBoolOption("log_aliasing");
                printFuncUse= program->options.getBoolOption("print_func_use");
                gen2 = program->policies.version_2_syntax;
                printCStyle = program->options.getBoolOption("print_c_style") || gen2;
            }
        }
        string str() const { return ss.str(); };
        bool printRef = false;
        bool printVarAccess = false;
        bool printCStyle = false;
        bool printAliases = false;
        bool printFuncUse = false;
        bool gen2 = false;
    protected:
        void newLine () {
            auto nlPos = ss.tellp();
            if ( nlPos != lastNewLine ) {
                ss << "\n";
                lastNewLine = ss.tellp();
            }
        }
        __forceinline static bool noBracket ( Expression * expr ) {
            return expr->topLevel || expr->bottomLevel || expr->argLevel;
        }
    // can we see inside quote
        virtual bool canVisitQuoteSubexpression ( ExprQuote * ) override { return true; }
    // TYPE
        bool ET = false;
        virtual void preVisit ( TypeDecl * td ) override {
            Visitor::preVisit(td);
            ET = td->isExprType();
            if ( ET ) ss << "/*[";
        }
        virtual TypeDeclPtr visit ( TypeDecl * td ) override {
            if ( ET ) ss << "]*/";
            return Visitor::visit(td);

        }
    // enumeration
        virtual void preVisit ( Enumeration * enu ) override {
            Visitor::preVisit(enu);
            ss << "enum ";
            ss << (enu->isPrivate ? "private " : "public ") << enu->name << " : " << das_to_string(enu->baseType) << "\n";
            if ( gen2 ) {
                ss << "{\n";
            }
        }
        virtual void preVisitEnumerationValue ( Enumeration * enu, const string & name, Expression * value, bool last ) override {
            Visitor::preVisitEnumerationValue(enu, name, value, last);
            ss << "\t" << name << " = ";
        }
        virtual ExpressionPtr visitEnumerationValue ( Enumeration * enu, const string & name, Expression * value, bool last ) override {
            if ( gen2 ) {
                if ( !last ) ss << ",";
                ss << "\n";
            } else {
                ss << "\n";
            }
            return Visitor::visitEnumerationValue(enu, name, value, last);
        }
        virtual EnumerationPtr visit ( Enumeration * enu ) override {
            if ( gen2 ) ss << "}\n";
            ss << "\n";
            return Visitor::visit(enu);
        }
    // strcuture
        void logAnnotationArguments(const AnnotationArgumentList & arguments) {
            bool first = true;
            for (const auto & aarg : arguments) {
                if (first) first = false; else ss << ",";
                ss << aarg.name << "=";
                switch (aarg.type) {
                case Type::tInt:    ss << aarg.iValue; break;
                case Type::tFloat:  ss << aarg.fValue; break;
                case Type::tBool:   ss << aarg.bValue; break;
                case Type::tString: ss << "\"" << aarg.sValue << "\""; break;
                default:
                    DAS_ASSERT(0 && "we should not even be here. "
                        "somehow annotation has unsupported argument type");
                    break;
                }
            }
        }
        void logAnnotations(const AnnotationList & annList) {
            if (!annList.empty()) {
                ss << "[";
                bool first = true;
                for (const auto & ann : annList) {
                    if ( first ) first = false; else ss << ",";
                    ann->annotation->log(ss, *ann);
                }
                ss << "]\n";
            }
        }
        virtual void preVisit ( Structure * that ) override {
            Visitor::preVisit(that);
            logAnnotations(that->annotations);
            if ( that->macroInterface ) ss << "[macro_interface]\n";
            ss << (that->isClass ? "class " : "struct ");
            ss << (that->privateStructure ? "private " : "public ") << that->name;
            if ( that->parent ) {
                ss << " : " << that->parent->name;
            }
            ss << "\n";
            if ( gen2 ) ss << "{\n";
        }
        void outputVariableAnnotation ( const AnnotationArgumentList & annotation ) {
            if ( annotation.size() ) {
                ss << "[";
                int ai = 0;
                for ( auto & arg : annotation ) {
                    if ( ai++ ) ss << ",";
                    ss << arg.name << "=";
                    switch ( arg.type ) {
                        case Type::tInt:        ss << arg.iValue; break;
                        case Type::tFloat:      ss << arg.fValue; break;
                        case Type::tBool:       ss << (arg.bValue ? "true" : "false"); break;
                        case Type::tString:     ss << "\"" << arg.sValue << "\""; break;
                        default:    DAS_ASSERTF(false,"this type is not allowed in annotation");
                    }
                }
                ss << "] ";
            }
        }
        virtual void preVisitStructureField ( Structure * that, Structure::FieldDeclaration & decl, bool last ) override {
            Visitor::preVisitStructureField(that, decl, last);
            ss << "\t";
            outputVariableAnnotation(decl.annotation);
            if ( decl.privateField ) ss << "private ";
            ss << decl.name << " : " << decl.type->describe();
            if ( decl.parentType ) {
                ss << " /* from " << that->parent->name << " */";
            }
            if ( decl.init ) {
                ss << (decl.moveSemantics ? " <- " : " = ");
            }
        }
        virtual void visitStructureField ( Structure * var, Structure::FieldDeclaration & decl, bool last ) override {
            if ( decl.capturedConstant ) ss << " // captured constant";
            if ( gen2 ) ss << ";";
            ss << "\n";
            Visitor::visitStructureField(var, decl, last);
        }
        virtual StructurePtr visit ( Structure * that ) override {
            if ( gen2 ) ss << "}\n";
            ss << "\n";
            return Visitor::visit(that);
        }
    // alias
        virtual void preVisitAlias ( TypeDecl * td, const string & name ) override {
            Visitor::preVisitAlias(td,name);
            ss  << "typedef " << name << " = " << td->describe();
            if ( gen2 ) ss << ";";
            ss << "\n\n";
        }
    // global
        virtual void preVisitGlobalLet ( const VariablePtr & var ) override {
            Visitor::preVisitGlobalLet(var);
            if ( printFuncUse ) {
                if ( var->useFunctions.size() ) {
                    ss << "// use functions";
                    for ( auto & ufn : var->useFunctions ) {
                        ss << " " << ufn->getMangledName();
                    }
                    ss << "\n";
                }
                if ( var->useGlobalVariables.size() ) {
                    ss << "// use global variables";
                    for ( auto & uvar : var->useGlobalVariables ) {
                        ss << " " << uvar->getMangledName();
                    }
                    ss << "\n";
                }
            }
            ss  << (var->type->constant ? "let" : "var")
                << (var->global_shared ? " shared" : "")
                << (var->private_variable ? " private" : "")
                << "\n\t";
            outputVariableAnnotation(var->annotation);
            if ( var->isAccessUnused() ) ss << " /*unused*/ ";
            if ( printVarAccess && !var->access_ref ) ss << "$";
            if ( printVarAccess && !var->access_pass ) ss << "%";
            ss << var->name << " : " << var->type->describe();
        }
        virtual VariablePtr visitGlobalLet ( const VariablePtr & var ) override {
            ss << "\n\n";
            return Visitor::visitGlobalLet(var);
        }
        virtual void preVisitGlobalLetInit ( const VariablePtr & var, Expression * init ) override {
            Visitor::preVisitGlobalLetInit(var, init);
            if ( var->init_via_move ) ss << " <- ";
            else if ( var->init_via_clone ) ss << " := ";
            else ss << " = ";
        }
    // function
        virtual void preVisit ( Function * fn) override {
            Visitor::preVisit(fn);
            if ( fn->unsafeFunction ) {
                ss << "/*unsafe*/ ";
            }
            if ( fn->knownSideEffects ) {
                if ( !fn->sideEffectFlags ) {
                ss << "[nosideeffects]\n";
                } else {
                    ss << "// ";
                    if ( fn->userScenario ) { ss << "[user_scenario]"; }
                    if ( fn->modifyExternal ) { ss << "[modify_external]"; }
                    if ( fn->modifyArgument ) { ss << "[modify_argument]"; }
                    if ( fn->accessGlobal ) { ss << "[access_global]"; }
                    if ( fn->invoke ) { ss << "[invoke]"; }
                    if ( fn->captureString ) { ss << "[capture_string]"; }
                    if ( fn->callCaptureString ) { ss << "[call_capture_string]"; }
                    ss << "\n";
                }
            }
            if ( fn->moreFlags ) {
                ss << "// ";
                if ( fn->macroFunction ) { ss << "[macro_function]"; }
                if ( fn->needStringCast ) { ss << "[need_string_cast]"; }
                if ( fn->aotHashDeppendsOnArguments ) { ss << "[aot_hash_deppends_on_arguments]"; }
                if ( fn->requestJit ) { ss << "[jit]"; }
                if ( fn->requestNoJit ) { ss << "[no_jit]"; }
                if ( fn->nodiscard ) { ss << "[nodiscard]"; }
                ss << "\n";
            }
            if ( fn->fastCall ) { ss << "[fastcall]\n"; }
            if ( fn->addr ) { ss << "[addr]\n"; }
            if ( fn->exports ) { ss << "[export]\n"; }
            if ( fn->init ) { ss << "[init" << (fn->lateInit ? "(late)" : "") << "]\n"; }
            if ( fn->macroInit ) { ss << "[macro_init]\n"; }
            if ( fn->macroFunction ) { ss << "[macro_function]\n"; }
            if ( fn->shutdown ) { ss << "[finalize]\n"; }
            if ( fn->unsafeDeref ) { ss << "[unsafe_deref]\n"; }
            if ( fn->unsafeOperation ) { ss << "[unsafe_operation]\n"; }
            if ( fn->isClassMethod ) { ss << "[class_method(" << fn->classParent->getMangledName() << ")]\n"; }
            if ( fn->generator ) { ss << "[GENERATOR]\n"; }
            logAnnotations(fn->annotations);
            if ( printAliases ) {
                if ( fn->resultAliases.size() ) {
                    ss << "// cmres aliases arguments";
                    for ( auto ai : fn->resultAliases ) {
                        ss << " " << fn->arguments[ai]->name;
                    }
                    ss << "\n";
                }
                if ( fn->resultAliasesGlobals.size() ) {
                    for ( auto & vinfo : fn->resultAliasesGlobals ) {
                        ss << "// cmres " << (vinfo.viaPointer ? "always " : "") << "aliases " << vinfo.var->getMangledName();
                        if ( fn != vinfo.func ) {
                            ss << " via function " << vinfo.func->getMangledName();
                        }
                        ss << "\n";
                    }
                }
            }
            if ( printFuncUse ) {
                if ( fn->useFunctions.size() ) {
                    ss << "// use functions";
                    for ( auto & ufn : fn->useFunctions ) {
                        ss << " " << ufn->getMangledName();
                    }
                    ss << "\n";
                }
                if ( fn->useGlobalVariables.size() ) {
                    ss << "// use global variables";
                    for ( auto & uvar : fn->useGlobalVariables ) {
                        ss << " " << uvar->getMangledName();
                    }
                    ss << "\n";
                }
            }
            if ( fn->fromGeneric ) {
                ss << "// from generic " << fn->fromGeneric->describe() << "\n";
            }
            ss << "def " << (fn->privateFunction ? "private " : "public ") << fn->name;
            if ( fn->arguments.size() ) ss << "(";
        }
        virtual void preVisitFunctionBody ( Function * fn,Expression * expr ) override {
            Visitor::preVisitFunctionBody(fn,expr);
            if ( fn->arguments.size() ) ss << ")";
            if ( fn->result && !fn->result->isVoid() ) ss << " : " << fn->result->describe();
            ss << "\n";
        }
        virtual void preVisitArgument ( Function * fn, const VariablePtr & arg, bool last ) override {
            Visitor::preVisitArgument(fn,arg,last);
            if (!arg->annotation.empty()) {
                ss << "[";
                logAnnotationArguments(arg->annotation);
                ss << "] ";
            }
            if ( !arg->type->isConst() ) ss << "var ";
            if ( arg->isAccessUnused() ) ss << " /*unused*/ ";
            if ( arg->no_capture ) ss << " /*no_capture*/ ";
            if ( printVarAccess && !arg->access_ref ) ss << "$";
            if ( printVarAccess && !arg->access_pass ) ss << "%";
            ss << arg->name;
            if ( !arg->aka.empty() ) ss << " aka " << arg->aka;
            ss << ":" << arg->type->describe();
        }
        virtual void preVisitArgumentInit ( Function * fn, const VariablePtr & arg, Expression * expr ) override {
            Visitor::preVisitArgumentInit(fn,arg,expr);
            ss << " = ";
        }
        virtual VariablePtr visitArgument ( Function * fn, const VariablePtr & that, bool last ) override {
            if ( !last ) ss << "; ";
            return Visitor::visitArgument(fn, that, last);
        }
        virtual FunctionPtr visit ( Function * fn ) override {
            ss << "\n\n";
            return Visitor::visit(fn);
        }
    // Ref2Value
        virtual void preVisit ( ExprRef2Value * expr ) override {
            Visitor::preVisit(expr);
            if ( printRef ) ss << "r2v(";
        }
        virtual ExpressionPtr visit ( ExprRef2Value * expr ) override {
            if ( printRef ) ss << ")";
            return Visitor::visit(expr);
        }
    // make block
        void describeCapture ( const vector<CaptureEntry> & capture ) {
            if ( !capture.empty() ) {
                ss << " [[";
                bool first = true;
                for ( const auto & ce : capture ) {
                    if ( first ) first = false; else ss << ",";
                    switch ( ce.mode ) {
                        case CaptureMode::capture_by_clone:     ss << ":="; break;
                        case CaptureMode::capture_by_copy:      ss << "="; break;
                        case CaptureMode::capture_by_move:      ss << "<-"; break;
                        case CaptureMode::capture_by_reference: ss << "&"; break;
                        default:;
                    }
                    ss << ce.name;
                }
                ss << "]] ";
            }
        }
        virtual void preVisit ( ExprMakeBlock * expr ) override {
            Visitor::preVisit(expr);
            if ( expr->isLambda ) ss << "@";
            else if ( expr->isLocalFunction ) ss << "@@";
            else ss << "$";
            describeCapture(expr->capture);
        }
    // block
        virtual void preVisitBlockExpression ( ExprBlock * block, Expression * expr ) override {
            Visitor::preVisitBlockExpression(block, expr);
            ss << string(tab,'\t');
        }
        bool needsTrailingSemicolumn ( Expression * that ) {
            if ( that->rtti_isIfThenElse() ) return false;
            if ( that->rtti_isFor() ) return false;
            if ( that->rtti_isWhile() ) return false;
            if ( that->rtti_isWith() ) return false;
            return true;
        }
        virtual ExpressionPtr visitBlockExpression ( ExprBlock * block, Expression * that ) override {
            if ( printCStyle || block->isClosure ) {
                if ( needsTrailingSemicolumn(that) ) {
                    ss << ";";
                }
            }
            newLine();
            return Visitor::visitBlockExpression(block, that);
        }
        virtual void preVisit ( ExprBlock * block ) override {
            Visitor::preVisit(block);
            if ( block->isClosure ) {
                logAnnotations(block->annotations);
                if ( block->returnType || block->arguments.size() ) {
                    ss << "(";
                    for ( auto & arg : block->arguments ) {
                        if (!arg->annotation.empty()) {
                            ss << "[";
                            logAnnotationArguments(arg->annotation);
                            ss << "] ";
                        }
                        if ( !arg->type->isConst() ) ss << "var ";
                        ss << arg->name;
                        if ( !arg->aka.empty() ) ss << " aka " << arg->aka;
                        ss << ":" << arg->type->describe();
                        if ( arg != block->arguments.back() )
                            ss << "; ";
                    }
                    ss << ")";
                    if ( block->returnType ) {
                        ss << ":" << block->returnType->describe();
                    }
                    ss << "\n";
                }
            }
            if ( printCStyle || block->isClosure ) ss << string(tab,'\t') << "{\n";
            tab ++;
        }
        virtual ExpressionPtr visit ( ExprBlock * block ) override {
            tab --;
            if ( printCStyle || block->isClosure ) ss << string(tab,'\t') << "}";
            return Visitor::visit(block);
        }
        virtual void preVisitBlockFinal ( ExprBlock * block ) override {
            Visitor::preVisitBlockFinal(block);
            if ( printCStyle || block->isClosure ) {
                ss << string(tab-1,'\t') << "} finally {\n";
            } else {
                ss << string(tab-1,'\t') << "finally\n";
            }
        }
        virtual void preVisitBlockFinalExpression ( ExprBlock * block, Expression * expr ) override {
            Visitor::preVisitBlockFinalExpression(block, expr);
            ss << string(tab,'\t');
        }
        virtual ExpressionPtr visitBlockFinalExpression ( ExprBlock * block, Expression * that ) override {
            if ( printCStyle || block->isClosure ) {
                if ( needsTrailingSemicolumn(that) ) {
                    ss << ";";
                }
            }
            newLine();
            return Visitor::visitBlockFinalExpression(block, that);
        }
    // string builder
        virtual void preVisit ( ExprStringBuilder * expr ) override {
            Visitor::preVisit(expr);
            ss << "string_builder";
            if ( expr->isTempString )
                ss << "_temp";
            ss << "(";
        }
        virtual ExpressionPtr visitStringBuilderElement ( ExprStringBuilder * sb, Expression * expr, bool last ) override {
            if ( !last ) ss << ", ";
            return Visitor::visitStringBuilderElement(sb, expr, last);
        }
        virtual ExpressionPtr visit ( ExprStringBuilder * expr ) override {
            ss << ")";
            return Visitor::visit(expr);
        }
    // label
        virtual void preVisit ( ExprReader * expr ) override {
            Visitor::preVisit(expr);
            ss << "#" << expr->macro->name << " " << expr->sequence << " ";
        }
    // label
        virtual void preVisit ( ExprLabel * expr ) override {
            Visitor::preVisit(expr);
            ss << "label " << expr->label << ":";
            if ( !expr->comment.empty() ) {
                ss << " /*" << expr->comment << "*/";
            }
        }
    // goto
        virtual void preVisit ( ExprGoto * expr ) override {
            Visitor::preVisit(expr);
            ss << "goto ";
            if ( !expr->subexpr )
                ss << "label " << expr->label;
        }
    // let
        virtual void preVisit ( ExprLet * let ) override {
            Visitor::preVisit(let);
            bool isLet = true;
            if ( let->variables.size() ) {
                auto pVar = let->variables.back();
                if ( pVar->type && !pVar->type->isConst() ) {
                    isLet = false;
                }
            }
            ss << (isLet ? "let " : "var ") << (let->inScope ? "inscope " : "");
        }
        virtual void preVisitLet ( ExprLet * let, const VariablePtr & var, bool last ) override {
            Visitor::preVisitLet(let, var, last);
            if ( var->isAccessUnused() ) ss << " /*unused*/ ";
            if ( printVarAccess && !var->access_ref ) ss << "$";
            if ( printVarAccess && !var->access_pass ) ss << "%";
            ss << var->name;
            if ( !var->aka.empty() ) ss << " aka " << var->aka;
            if ( printAliases && var->aliasCMRES ) ss << "/*cmres*/";
            if ( var->early_out ) ss << "/*early_out*/";
            if ( var->used_in_finally ) ss << "/*used_in_finally*/";
            ss << ":" << var->type->describe();
        }
        virtual VariablePtr visitLet ( ExprLet * let, const VariablePtr & var, bool last ) override {
            if ( !last ) ss << "; ";
            return Visitor::visitLet(let, var, last);
        }
        virtual void preVisitLetInit ( ExprLet * let, const VariablePtr & var, Expression * expr ) override {
            Visitor::preVisitLetInit(let,var,expr);
            if ( var->init_via_move ) ss << " <- ";
            else if ( var->init_via_clone ) ss << " := ";
            else ss << " = ";
        }
    // if then else
        virtual void preVisit ( ExprIfThenElse * ifte ) override {
            Visitor::preVisit(ifte);
            ss << (ifte->isStatic ? "static_if " : "if ");
            if ( gen2 ) ss << "( ";
        }
        virtual void preVisitIfBlock ( ExprIfThenElse * ifte, Expression * block ) override {
            Visitor::preVisitIfBlock(ifte,block);
            if ( gen2 ) ss <<  " )";
            ss << "\n";
        }
        virtual void preVisitElseBlock ( ExprIfThenElse * ifte, Expression * block ) override {
            Visitor::preVisitElseBlock(ifte, block);
            ss << "\n" << string(tab,'\t');
            if (block && block->rtti_isIfThenElse()) {
                ss << (ifte->isStatic ? "static_el" : "el");
            } else {
                ss << (ifte->isStatic ? "static_else\n" : "else\n");
            }
        }
    // try-catch
        virtual void preVisit ( ExprTryCatch * tc ) override {
            Visitor::preVisit(tc);
            ss << "try\n";
        }
        virtual void preVisitCatch ( ExprTryCatch * tc, Expression * block ) override {
            Visitor::preVisitCatch(tc, block);
            ss << string(tab,'\t') << "recover\n";
        }
    // for
        virtual void preVisit ( ExprFor * ffor ) override {
            Visitor::preVisit(ffor);
            ss << "for ";
            if ( gen2 ) ss << "( ";
            bool first = true;
            for ( auto i : ffor->iterators ) {
                if ( first) first = false; else ss << ", ";
                ss << i;
            }
            ss << " in ";
        }
        virtual void preVisitForBody ( ExprFor * ffor, Expression * body ) override {
            Visitor::preVisitForBody(ffor, body);
            if ( gen2 ) ss << " )";
            ss << "\n";
        }
        virtual ExpressionPtr visitForSource ( ExprFor * ffor, Expression * that , bool last ) override {
            if ( !last ) ss << ",";
            return Visitor::visitForSource(ffor, that, last);
        }
    // with
        virtual void preVisit ( ExprWith * wh ) override {
            Visitor::preVisit(wh);
            ss << "with ";
            if ( gen2 ) ss << "( ";
        }
        virtual void preVisitWithBody ( ExprWith * wh, Expression * body ) override {
            Visitor::preVisitWithBody(wh,body);
            if ( gen2 ) ss << " )";
            ss << "\n";
        }
    // with alias
        virtual bool canVisitWithAliasSubexpression ( ExprAssume * ) override {
            return true;
        }
        virtual void preVisit ( ExprAssume * wh ) override {
            Visitor::preVisit(wh);
            ss << "assume " << wh->alias << " = ";
        }
    // tag
        virtual void preVisit ( ExprTag * expr ) override {
            Visitor::preVisit(expr);
            if ( expr->name=="...") {
                ss << "...";
            } else {
                ss << "$" << expr->name << "(";
            }
        }
        virtual void preVisitTagValue ( ExprTag * expr, Expression * value ) override {
            Visitor::preVisitTagValue(expr,value);
            if ( expr->name!="...") {
                ss << ")(";
            }
        }
        virtual ExpressionPtr visit ( ExprTag * expr ) override {
            if ( expr->name!="...") {
                ss << ")";
            }
            return Visitor::visit(expr);
        }
    // while
        virtual void preVisit ( ExprWhile * wh ) override {
            Visitor::preVisit(wh);
            ss << "while ";
            if ( gen2 ) ss << "( ";
        }
        virtual void preVisitWhileBody ( ExprWhile * wh, Expression * body ) override {
            Visitor::preVisitWhileBody(wh,body);
            if ( gen2 ) ss << " )";
            ss << "\n";
        }
    // while
        virtual void preVisit ( ExprUnsafe * wh ) override {
            Visitor::preVisit(wh);
            ss << "unsafe\n";
        }
    // call
        virtual void preVisit ( ExprCall * call ) override {
            Visitor::preVisit(call);
            ss << call->name;
            if ( printAliases && call->doesNotNeedSp ) ss << " /*no_sp*/ ";
            ss << "(";
        }
        virtual ExpressionPtr visitCallArg ( ExprCall * call, Expression * arg, bool last ) override {
            if ( !last ) ss << ",";
            return Visitor::visitCallArg(call, arg, last);
        }
        virtual ExpressionPtr visit ( ExprCall * c ) override {
            ss << ")";
            return Visitor::visit(c);
        }
    // named call
        virtual void preVisit ( ExprNamedCall * call ) override {
            Visitor::preVisit(call);
            ss << call->name << "(";
        }
        virtual void preVisitNamedCallArg ( ExprNamedCall * call, MakeFieldDecl * arg, bool last ) override {
            if (call->arguments[0] == arg) {
                if (call->nonNamedArguments.size() > 0) ss << ",";
                ss << "[";
            }
            Visitor::preVisitNamedCallArg(call, arg, last);
            ss << arg->name << (arg->moveSemantics ? "<-" : "=" );
        }
        virtual MakeFieldDeclPtr visitNamedCallArg ( ExprNamedCall * call, MakeFieldDecl * arg, bool last ) override {
            if ( !last ) ss << ",";
            return Visitor::visitNamedCallArg(call, arg, last);
        }
        virtual ExpressionPtr visit ( ExprNamedCall * c ) override {
            ss << "])";
            return Visitor::visit(c);
        }
    // looks like call
        virtual void preVisit ( ExprCallMacro * call ) override {
            Visitor::preVisit(call);
            ss << call->name << "(";
        }
        virtual ExpressionPtr visit ( ExprCallMacro * c ) override {
            ss << ")";
            return Visitor::visit(c);
        }
    // looks like call
        virtual void preVisit ( ExprLooksLikeCall * call ) override {
            Visitor::preVisit(call);
            ss << call->name;
            if ( call->rtti_isInvoke() ) {
                auto * inv = (ExprInvoke *) call;
                if ( printAliases && inv->doesNotNeedSp ) ss << " /*no_sp*/ ";
                if ( inv->isInvokeMethod ) ss << "/*method*/";
            }
            ss << "(";
        }
        virtual ExpressionPtr visitLooksLikeCallArg ( ExprLooksLikeCall * call, Expression * arg, bool last ) override {
            if ( !last ) ss << ",";
            return Visitor::visitLooksLikeCallArg(call, arg, last);
        }
        virtual ExpressionPtr visit ( ExprLooksLikeCall * c ) override {
            ss << ")";
            return Visitor::visit(c);
        }
    // const
        virtual ExpressionPtr visit(ExprFakeContext * c) override {
            ss << "__context__";
            return Visitor::visit(c);
        }
        virtual ExpressionPtr visit(ExprFakeLineInfo * c) override {
            ss << "__lineinfo__";
            return Visitor::visit(c);
        }
        virtual ExpressionPtr visit ( ExprConstPtr * c ) override {
            if ( c->ptrType ) {
                ss << "const_pointer<" << c->ptrType->describe() << ">(";
            }
            if ( c->getValue() ) {
                ss << "*0x" << HEX << intptr_t(c->getValue()) << DEC;
            } else {
                ss << "null";
            }
            if ( c->ptrType ) {
                ss << ")";
            }
            return Visitor::visit(c);
        }
        virtual ExpressionPtr visit ( ExprConstEnumeration * c ) override {
            if ( c->enumType->module && !c->enumType->module->name.empty() ) {
                ss << c->enumType->module->name << "::";
            }
            ss << c->enumType->name << " " << c->text;
            return Visitor::visit(c);
        }
        virtual ExpressionPtr visit ( ExprConstInt * c ) override {
            ss << c->getValue();
            return Visitor::visit(c);
        }
        virtual ExpressionPtr visit ( ExprConstInt8 * c ) override {
            ss << int32_t(c->getValue()) << "/*i8*/";
            return Visitor::visit(c);
        }
        virtual ExpressionPtr visit ( ExprConstUInt8 * c ) override {
            ss << "0x" << HEX << uint32_t(c->getValue()) << DEC << "u8";
            return Visitor::visit(c);
        }
        virtual ExpressionPtr visit ( ExprConstInt16 * c ) override {
            ss << int32_t(c->getValue()) << "/*i16*/";
            return Visitor::visit(c);
        }
        virtual ExpressionPtr visit ( ExprConstUInt16 * c ) override {
            ss << "0x" << HEX << uint32_t(c->getValue()) << DEC <<  "/*u16*/";
            return Visitor::visit(c);
        }
        virtual ExpressionPtr visit ( ExprConstInt64 * c ) override {
            ss << c->getValue();
            return Visitor::visit(c);
        }
        virtual ExpressionPtr visit ( ExprConstUInt64 * c ) override {
            ss << "0x" << HEX << c->getValue() << DEC;
            return Visitor::visit(c);
        }
        virtual ExpressionPtr visit ( ExprConstUInt * c ) override {
            ss << "0x" << HEX << c->getValue() << DEC;
            return Visitor::visit(c);
        }
        virtual ExpressionPtr visit ( ExprConstBitfield * c ) override {
            string name;
            if ( c->bitfieldType && !c->bitfieldType->alias.empty() ) {
                name = c->bitfieldType->findBitfieldName(c->getValue());
            }
            if ( !name.empty() ) {
                ss << c->bitfieldType->alias << " " << name;
            } else {
                ss << "bitfield(0x" << HEX << c->getValue() << DEC << ")";
            }
            return Visitor::visit(c);
        }
        virtual ExpressionPtr visit ( ExprConstBool * c ) override {
            ss << (c->getValue() ? "true" : "false");
            return Visitor::visit(c);
        }
        virtual ExpressionPtr visit ( ExprConstDouble * c ) override {
            ss << c->getValue() << "lf";
            return Visitor::visit(c);
        }
        virtual ExpressionPtr visit ( ExprConstFloat * c ) override {
            ss << c->getValue() << "f";
            return Visitor::visit(c);
        }
        virtual ExpressionPtr visit ( ExprConstString * c ) override {
            ss << "\"" << escapeString(c->text) << "\"";
            return Visitor::visit(c);
        }
        virtual ExpressionPtr visit ( ExprConstInt2 * c ) override {
            auto val = c->getValue();
            ss << "int2(" << val.x << "," << val.y << ")";
            return Visitor::visit(c);
        }
        virtual ExpressionPtr visit ( ExprConstRange * c ) override {
            auto val = c->getValue();
            ss << "range(" << val.from << "," << val.to << ")";
            return Visitor::visit(c);
        }
        virtual ExpressionPtr visit ( ExprConstRange64 * c ) override {
            auto val = c->getValue();
            ss << "range64(" << val.from << "," << val.to << ")";
            return Visitor::visit(c);
        }
        virtual ExpressionPtr visit ( ExprConstInt3 * c ) override {
            auto val = c->getValue();
            ss << "int3(" << val.x << "," << val.y << "," << val.z << ")";
            return Visitor::visit(c);
        }
        virtual ExpressionPtr visit ( ExprConstInt4 * c ) override {
            auto val = c->getValue();
            ss << "int4(" << val.x << "," << val.y << "," << val.z << "," << val.w << ")";
            return Visitor::visit(c);
        }
        virtual ExpressionPtr visit ( ExprConstUInt2 * c ) override {
            auto val = c->getValue();
            ss << "uint2(" << val.x << "," << val.y << ")";
            return Visitor::visit(c);
        }
        virtual ExpressionPtr visit ( ExprConstURange * c ) override {
            auto val = c->getValue();
            ss << "urange(" << val.from << "," << val.to << ")";
            return Visitor::visit(c);
        }
        virtual ExpressionPtr visit ( ExprConstURange64 * c ) override {
            auto val = c->getValue();
            ss << "urange64(" << val.from << "," << val.to << ")";
            return Visitor::visit(c);
        }
        virtual ExpressionPtr visit ( ExprConstUInt3 * c ) override {
            auto val = c->getValue();
            ss << "uint3(" << val.x << "," << val.y << "," << val.z << ")";
            return Visitor::visit(c);
        }
        virtual ExpressionPtr visit ( ExprConstUInt4 * c ) override {
            auto val = c->getValue();
            ss << "uint4(" << val.x << "," << val.y << "," << val.z << "," << val.w << ")";
            return Visitor::visit(c);
        }
        virtual ExpressionPtr visit ( ExprConstFloat2 * c ) override {
            auto val = c->getValue();
            ss << "float2(" << val.x << "f," << val.y << "f)";
            return Visitor::visit(c);
        }
        virtual ExpressionPtr visit ( ExprConstFloat3 * c ) override {
            auto val = c->getValue();
            ss << "float3(" << val.x << "f," << val.y << "f," << val.z << "f)";
            return Visitor::visit(c);
        }
        virtual ExpressionPtr visit ( ExprConstFloat4 * c ) override {
            auto val = c->getValue();
            ss << "float4(" << val.x << "f," << val.y << "f," << val.z << "f," << val.w << "f)";
            return Visitor::visit(c);
        }
    // var
        virtual ExpressionPtr visit ( ExprVar * var ) override {
            if ( printRef && var->r2v ) ss << "@";
            if ( printRef && var->r2cr ) ss << "$";
            if ( printRef && var->write ) ss << "#";
            ss << var->name;
            return Visitor::visit(var);
        }
    // swizzle
        virtual ExpressionPtr visit ( ExprSwizzle * expr ) override {
            ss << ".";
            if ( printRef && expr->r2v ) ss << "@";
            if ( printRef && expr->r2cr ) ss << "$";
            if ( printRef && expr->write ) ss << "#";
            if ( expr->fields.size() ) {
                for ( auto f : expr->fields ) {
                    switch ( f ) {
                        case 0:     ss << "x"; break;
                        case 1:     ss << "y"; break;
                        case 2:     ss << "z"; break;
                        case 3:     ss << "w"; break;
                        default:    ss << "?"; break;
                    }
                }
            } else {
                ss << expr->mask;
                if ( expr->value->type && expr->value->type->isVectorType() ) {
                    auto dim = expr->value->type->getVectorDim();
                    vector<uint8_t> dummy;
                    if ( !TypeDecl::buildSwizzleMask(expr->mask, dim, dummy) ) {
                        ss << "/*invalid swizzle*/";
                        return Visitor::visit(expr);
                    }
                }
            }
            return Visitor::visit(expr);
        }
    // field
        virtual ExpressionPtr visit ( ExprField * field ) override {
            if ( printRef && field->r2v ) ss << "@";
            if ( printRef && field->r2cr ) ss << "$";
            if ( printRef && field->write ) ss << "#";
            ss << "." << field->name;
            return Visitor::visit(field);
        }
        virtual ExpressionPtr visit ( ExprSafeField * field ) override {
            if ( printRef && field->r2v ) ss << "@";
            if ( printRef && field->r2cr ) ss << "$";
            if ( printRef && field->write ) ss << "#";
            ss << ".?" << field->name;
            return Visitor::visit(field);
        }
        virtual ExpressionPtr visit ( ExprIsVariant * field ) override {
            if ( printRef && field->r2v ) ss << "@";
            if ( printRef && field->r2cr ) ss << "$";
            if ( printRef && field->write ) ss << "#";
            ss << " is " << field->name;
            return Visitor::visit(field);
        }
        virtual ExpressionPtr visit ( ExprAsVariant * field ) override {
            if ( printRef && field->r2v ) ss << "@";
            if ( printRef && field->r2cr ) ss << "$";
            if ( printRef && field->write ) ss << "#";
            ss << " as " << field->name;
            return Visitor::visit(field);
        }
        virtual ExpressionPtr visit ( ExprSafeAsVariant * field ) override {
            if ( printRef && field->r2v ) ss << "@";
            if ( printRef && field->r2cr ) ss << "$";
            if ( printRef && field->write ) ss << "#";
            ss << " ?as " << field->name;
            return Visitor::visit(field);
        }
    // addr
        virtual void preVisit ( ExprAddr * expr ) override {
            ss << "@@";
            if ( expr->funcType ) {
                ss << "<" << expr->funcType->describe() << ">";
            }
            ss << expr->target;
        }
    // ptr2ref
        virtual void preVisit ( ExprPtr2Ref * ptr2ref ) override {
            Visitor::preVisit(ptr2ref);
            ss << "deref(";
        }
        virtual ExpressionPtr visit ( ExprPtr2Ref * ptr2ref ) override {
            ss << ")";
            return Visitor::visit(ptr2ref);
        }
     // ref2ptr
        virtual void preVisit ( ExprRef2Ptr * ptr2ref ) override {
            Visitor::preVisit(ptr2ref);
            ss << "addr(";
        }
        virtual ExpressionPtr visit ( ExprRef2Ptr * ptr2ref ) override {
            ss << ")";
            return Visitor::visit(ptr2ref);
        }
    // delete
        virtual void preVisit ( ExprDelete * edel ) override {
            Visitor::preVisit(edel);
            ss << "delete ";
            if ( edel->native ) ss << "/*native*/ ";
        }
        virtual void preVisitDeleteSizeExpression ( ExprDelete *, Expression * ) override {
            ss << ", rtti.size = ";
        }
    // cast
        virtual void preVisit ( ExprCast * expr ) override {
            Visitor::preVisit(expr);
            if ( expr->reinterpret ) {
                ss << "reinterpret";
            } else if ( expr->upcast ) {
                ss << "upcast";
            } else {
                ss << "cast";
            }
            ss << "<" << expr->castType->describe() << "> ";
        }
    // ascend
        virtual void preVisit ( ExprAscend * expr ) override {
            Visitor::preVisit(expr);
            if ( expr->ascType ) {
                ss << "new<" << expr->ascType->describe() << "> ";
            } else {
                ss << "new ";
            }
        }
    // new
        virtual void preVisit ( ExprNew * enew ) override {
            Visitor::preVisit(enew);
            if ( enew->typeexpr ) {
                ss << "new " << enew->typeexpr->describe();
            } else {
                ss << "new ???";
            }
            if ( enew->initializer ) ss << "(";
        }
        virtual ExpressionPtr visitNewArg ( ExprNew * call, Expression * arg, bool last ) override {
            if ( !last ) ss << ",";
            return Visitor::visitNewArg(call, arg, last);
        }
        virtual ExpressionPtr visit ( ExprNew * enew ) override {
            if ( enew->initializer ) ss << ")";
            return Visitor::visit(enew);
        }
    // null coaelescing
        virtual void preVisitNullCoaelescingDefault ( ExprNullCoalescing * nc, Expression * expr ) override {
            Visitor::preVisitNullCoaelescingDefault(nc,expr);
            ss << " ?? ";
        }
    // at
        virtual void preVisitAtIndex ( ExprAt * expr, Expression * index ) override {
            Visitor::preVisitAtIndex(expr, index);
            if ( printRef && expr->r2v ) ss << "@";
            if ( printRef && expr->r2cr ) ss << "$";
            if ( printRef && expr->write ) ss << "#";
            ss << "[";

        }
        virtual ExpressionPtr visit ( ExprAt * that ) override {
            ss << "]";
            return Visitor::visit(that);
        }
    // safe at
        virtual void preVisitSafeAtIndex ( ExprSafeAt * expr, Expression * index ) override {
            Visitor::preVisitAtIndex(expr, index);
            if ( printRef && expr->r2v ) ss << "@";
            if ( printRef && expr->r2cr ) ss << "$";
            if ( printRef && expr->write ) ss << "#";
            ss << "?[";

        }
        virtual ExpressionPtr visit ( ExprSafeAt * that ) override {
            ss << "]";
            return Visitor::visit(that);
        }
    // op1
        virtual void preVisit ( ExprOp1 * that ) override {
            Visitor::preVisit(that);
            if ( that->op!="+++" && that->op!="---" ) {
                ss << that->op;
            }
            if ( !noBracket(that) && !that->subexpr->bottomLevel ) ss << "(";
        }
        virtual ExpressionPtr visit ( ExprOp1 * that ) override {
            if ( that->op=="+++" || that->op=="---" ) {
                ss << that->op[0] << that->op[1];
            }
            if ( !noBracket(that) && !that->subexpr->bottomLevel ) ss << ")";
            return Visitor::visit(that);
        }
    // op2
        virtual void preVisit ( ExprOp2 * that ) override {
            Visitor::preVisit(that);
            if ( !noBracket(that) ) ss << "(";
        }
        virtual void preVisitRight ( ExprOp2 * op2, Expression * right ) override {
            Visitor::preVisitRight(op2,right);
            ss << " " << op2->op << " ";
        }
        virtual ExpressionPtr visit ( ExprOp2 * that ) override {
            if ( !noBracket(that) ) ss << ")";
            return Visitor::visit(that);
        }
    // op3
        virtual void preVisit ( ExprOp3 * that ) override {
            Visitor::preVisit(that);
            if ( !noBracket(that) ) ss << "(";
        }
        virtual void preVisitLeft  ( ExprOp3 * that, Expression * left ) override {
            Visitor::preVisitLeft(that,left);
            ss << " ? ";
        }
        virtual void preVisitRight ( ExprOp3 * that, Expression * right ) override {
            Visitor::preVisitRight(that,right);
            ss << " : ";
        }
        virtual ExpressionPtr visit ( ExprOp3 * that ) override {
            if ( !noBracket(that) ) ss << ")";
            return Visitor::visit(that);
        }
    // copy & move, clone
        virtual void preVisitRight ( ExprCopy * that, Expression * right ) override {
            Visitor::preVisitRight(that,right);
            ss << " = ";
        }
        virtual void preVisitRight ( ExprMove * that, Expression * right ) override {
            Visitor::preVisitRight(that,right);
            ss << " <- ";
        }
        virtual void preVisitRight ( ExprClone * that, Expression * right ) override {
            Visitor::preVisitRight(that,right);
            ss << " := ";
        }
    // return
        virtual void preVisit ( ExprReturn * expr ) override {
            Visitor::preVisit(expr);
            ss << "return ";
            if ( expr->fromYield ) ss << "/*yield*/ ";
            if ( expr->moveSemantics ) ss << "<- ";
        }
    // yield
        virtual void preVisit ( ExprYield * expr ) override {
            Visitor::preVisit(expr);
            ss << "yield ";
            if ( expr->moveSemantics ) ss << "<- ";
        }
    // break
        virtual void preVisit ( ExprBreak * that ) override {
            Visitor::preVisit(that);
            ss << "break";
        }
    // continue
        virtual void preVisit ( ExprContinue * that ) override {
            Visitor::preVisit(that);
            ss << "continue";
        }
    // typedecl
        virtual void preVisit ( ExprTypeDecl * expr ) override {
            Visitor::preVisit(expr);
            ss << "type<" << *expr->typeexpr << ">";
        }
    // typeinfo
        virtual void preVisit ( ExprTypeInfo * expr ) override {
            Visitor::preVisit(expr);
            ss << "typeinfo(" << expr->trait << " ";
            if ( !expr->subexpr ) {
                ss << "type<" << *expr->typeexpr << ">";
            }
        }
        virtual ExpressionPtr visit ( ExprTypeInfo * expr ) override {
            ss << ")";
            return Visitor::visit(expr);
        }
    // is
        virtual void preVisitType ( ExprIs * expr, TypeDecl * decl ) override {
            Visitor::preVisit(expr);
            ss << " is type<" << decl->describe(TypeDecl::DescribeExtra::no, TypeDecl::DescribeContracts::yes) << ">";
        }
    // make variant
        virtual void preVisit ( ExprMakeVariant * expr ) override {
            Visitor::preVisit(expr);
            ss << "[[";
            if ( expr->type ) {
                ss << expr->type->describe() << " ";
            }
        }
        virtual void preVisitMakeVariantField ( ExprMakeVariant * expr, int index, MakeFieldDecl * decl, bool last ) override {
            Visitor::preVisitMakeVariantField(expr,index,decl,last);
            ss << decl->name << (decl->moveSemantics ? " <- " : " = ");
        }
        virtual MakeFieldDeclPtr visitMakeVariantField ( ExprMakeVariant * expr, int index, MakeFieldDecl * decl, bool last ) override {
            if (!last) ss << "; ";
            return Visitor::visitMakeVariantField(expr,index,decl,last);
        }
        virtual ExpressionPtr visit ( ExprMakeVariant * expr ) override {
            ss << "]]";
            return Visitor::visit(expr);
        }
    // make structure
        virtual void preVisit ( ExprMakeStruct * expr ) override {
            Visitor::preVisit(expr);
            if ( gen2 ) {
                if ( expr->structs.empty() ) {
                    ss << "default<";
                    if ( expr->type ) {
                        ss << expr->type->describe();
                    } else {
                        ss << "/* undefined */";
                    }
                    ss << ">";
                    if ( !expr->useInitializer && !expr->constructor ) {
                        ss << " uninitialized";
                    }
                } else {
                    ss << "struct<";
                    if ( expr->type ) {
                        ss << expr->type->describe();
                    } else {
                        ss << "/* undefined */";
                    }
                    ss << ">(";
                    if ( !expr->useInitializer && !expr->constructor ) {
                        ss << "uninitialized ";
                    }
                }
            } else {
                ss << "[[";
                if ( expr->type ) {
                    ss << expr->type->describe();
                    if ( expr->useInitializer || expr->constructor ) {
                        ss << "()";
                    }
                    ss << " ";
                }
            }
        }
        virtual void preVisitMakeStructureField ( ExprMakeStruct * expr, int index, MakeFieldDecl * decl, bool last ) override {
            Visitor::preVisitMakeStructureField(expr,index,decl,last);
            ss << decl->name << (decl->moveSemantics ? " <- " : " = ");
        }
        virtual MakeFieldDeclPtr visitMakeStructureField ( ExprMakeStruct * expr, int index, MakeFieldDecl * decl, bool last ) override {
            if ( !last ) ss << ", ";
            return Visitor::visitMakeStructureField(expr,index,decl,last);
        }
        virtual void visitMakeStructureIndex ( ExprMakeStruct * expr, int index, bool lastField ) override {
            if ( !lastField ) ss << "; ";
            Visitor::visitMakeStructureIndex(expr, index, lastField);
        }
        virtual void preVisitMakeStructureBlock ( ExprMakeStruct *, Expression * ) override {
            ss << " where ";
        }
        virtual ExpressionPtr visit ( ExprMakeStruct * expr ) override {
            if ( gen2 ) {
                if ( !expr->structs.empty() ) {
                    ss << ")";
                }
            } else {
                ss << "]]";
            }
            return Visitor::visit(expr);
        }
    // make tuple
        virtual void preVisit ( ExprMakeTuple * expr ) override {
            Visitor::preVisit(expr);
            if ( gen2 ) {
                ss << "tuple";
                if ( expr->type ) {
                    ss << "<" << expr->type->describe() << ">";
                } else {
                    ss << "</* undefined */>";
                }
                ss << "(";
            } else {
                ss << "[[";
                if ( expr->type ) {
                    ss << expr->type->describe();
                } else {
                    ss << "/* undefined */";
                }
                ss << " ";
            }
        }
        virtual ExpressionPtr visitMakeTupleIndex ( ExprMakeTuple * expr, int index, Expression * init, bool lastField ) override {
            if ( !lastField ) ss << ",";
            return Visitor::visitMakeTupleIndex(expr, index, init, lastField);
        }
        virtual ExpressionPtr visit ( ExprMakeTuple * expr ) override {
            if ( gen2 ) {
                ss << ")";
            } else {
                ss << "]]";
            }
            return Visitor::visit(expr);
        }
    // make array
        virtual void preVisit ( ExprMakeArray * expr ) override {
            Visitor::preVisit(expr);
            if ( gen2 ) {
                // if ( expr->type ) { ss << "/*" << expr->type->describe() << "*/ "; }
                ss << "fixed_array";
                if ( expr->makeType ) {
                    ss << "<" << expr->makeType->describe() << ">";
                } else {
                    ss << "</* undefined */>";
                }
                ss << "(";
            } else {
                ss << "[[";
                if ( expr->type ) {
                    ss << expr->type->describe();
                } else {
                    ss << "/* undefined */";
                }
                ss << " ";
            }
        }
        virtual ExpressionPtr visitMakeArrayIndex ( ExprMakeArray * expr, int index, Expression * init, bool lastField ) override {
            if ( !lastField ) {
                ss << (gen2 ? ", " : "; ");
            }
            return Visitor::visitMakeArrayIndex(expr, index, init, lastField);
        }
        virtual ExpressionPtr visit ( ExprMakeArray * expr ) override {
            ss << (gen2 ? ")" : "]]");
            return Visitor::visit(expr);
        }
    // array comprehension
        virtual void preVisit ( ExprArrayComprehension * expr ) override {
            Visitor::preVisit(expr);
            if ( expr->generatorSyntax ) {
                ss << "[[";
            } else if ( expr->tableSyntax ) {
                ss << "{";
            } else {
                ss << "[{";
            }
        }
        virtual void preVisitArrayComprehensionSubexpr ( ExprArrayComprehension * expr, Expression * subexpr ) override {
            Visitor::preVisitArrayComprehensionSubexpr(expr, subexpr);
            ss << "; ";
        }
        virtual void preVisitArrayComprehensionWhere ( ExprArrayComprehension * expr, Expression * where ) override {
            Visitor::preVisitArrayComprehensionWhere(expr, where);
            ss << "; where ";
        }
        virtual ExpressionPtr visit ( ExprArrayComprehension * expr ) override {
            if ( expr->generatorSyntax ) {
                ss << "]]";
            } else if ( expr->tableSyntax ) {
                ss << "}";
            } else {
                ss << "}]";
            }
            return Visitor::visit(expr);
        }
    // quote
        virtual void preVisit ( ExprQuote * expr ) override {
            Visitor::preVisit(expr);
            ss << "quote(";
        }
        virtual ExpressionPtr visit ( ExprQuote * expr ) override {
            ss << ")";
            return Visitor::visit(expr);
        }
    protected:
        TextWriter          ss;
        uint64_t            lastNewLine = -1ul;
        int                 tab = 0;
    };

    void Program::setPrintFlags() {
        SetPrinterFlags pflags;
        visit(pflags);
    }

    template <typename TT>
    __forceinline StringWriter&  print ( StringWriter& stream, const TT & value ) {
        SetPrinterFlags flags;
        const_cast<TT&>(value).visit(flags);
        Printer log(nullptr);
        const_cast<TT&>(value).visit(log);
        stream << log.str();
        return stream;
    }

    StringWriter& operator<< (StringWriter& stream, const Program & program) {
        bool any = false;
        program.library.foreach([&](Module * pm) {
            if ( !pm->name.empty() && pm->name!="$" ) {
                stream << "require " << pm->name << "\n";
                any = true;
            }
            return true;
        }, "*");
        if (any) stream << "\n";
        bool logGenerics = program.options.getBoolOption("log_generics");
        SetPrinterFlags flags;
        const_cast<Program&>(program).visit(flags, logGenerics);
        Printer log(&program);
        const_cast<Program&>(program).visit(log, logGenerics);
        stream << log.str();
        return stream;
    }

    StringWriter& operator<< (StringWriter& stream, const Expression & expr) {
        return print(stream,expr);
    }

    StringWriter& operator<< (StringWriter& stream, const Function & func) {
        return print(stream,func);
    }
}
