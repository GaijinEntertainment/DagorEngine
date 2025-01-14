#include "daScript/misc/platform.h"

#include "daScript/ast/ast.h"
#include "daScript/ast/ast_generate.h"
#include "daScript/ast/ast_expressions.h"
#include "daScript/ast/ast_visitor.h"

namespace das {

    struct hash_vec4f {
        uint64_t operator () ( vec4f v ) const {
            uint64_t a[2];
            memcpy(a,&v,sizeof(vec4f));
            return a[0] ^ a[1];
        }
    };

    struct equalto_vec4f {
        bool operator () ( vec4f a, vec4f b ) const {
            return memcmp(&a,&b,sizeof(vec4f))==0;
        }
    };

    struct hash_ccs {
        uint64_t operator () ( const char * str ) const {
            return hash64z(str);
        }
    };

    struct equalto_ccs {
        bool operator () ( const char * a, const char * b ) const {
            return strcmp(a,b)==0;
        }
    };


    das::unordered_set<string> g_cpp_keywords = {
        /* reserved C++ names*/
        "alignas","alignof","and","and_eq","asm","atomic_cancel","atomic_commit","atomic_noexcept","auto"
        ,"bitand","bitor","bool","break","case","catch","char","char8_t","char16_t","char32_t","class"
        ,"compl","concept","const","consteval","constexpr","constinit","const_cast","continue","co_await"
        ,"co_return","co_yield","decltype","default","delete","do","double","dynamic_cast","else","enum"
        ,"explicit","export","extern","false","float","for","friend","goto","if","inline","int","long"
        ,"mutable","namespace","new","noexcept","not","not_eq","nullptr","operator","or","or_eq","private"
        ,"protected","public","reflexpr","register","reinterpret_cast","requires","return","short","signed"
        ,"sizeof","static","static_assert","static_cast","struct","switch","synchronized","template","this"
        ,"thread_local","throw","true","try","typedef","typeid","typename","union","unsigned","using"
        ,"virtual","void","volatile","wchar_t","while","xor","xor_eq"
        /* extra */
        ,"override","final","import","module","transaction_safe","transaction_safe_dynamic"
    };

    bool isCppKeyword(const string & str) {
        return g_cpp_keywords.find(str) != g_cpp_keywords.end();
    }

    bool hasLabels ( const smart_ptr<ExprBlock> & block ) {
        for ( auto & be : block->list ) {
            if ( be->rtti_isLabel() ) {
                return true;
            }
        }
        return false;
    }

    bool exprReturns ( const ExpressionPtr & expr ) {
        if ( expr->rtti_isReturn() ) {
            return true;
        } else if ( expr->rtti_isBlock() ) {
            auto block = static_pointer_cast<ExprBlock>(expr);
            if ( hasLabels(block) ) {
                // NOTE: block with labels assumed to always return for now. real world analysis is hard
                return true;
            } else {
                for ( auto & be : block->list ) {
                    if ( be->rtti_isBreak() || be->rtti_isContinue() || be->rtti_isGoto() ) {
                        break;
                    }
                    if ( exprReturns(be) ) {
                        return true;
                    }
                }
            }
        } else if ( expr->rtti_isIfThenElse() ) {
            auto ite = static_pointer_cast<ExprIfThenElse>(expr);
            if ( ite->if_false ) {
                return exprReturns(ite->if_true) && exprReturns(ite->if_false);
            }
        } else if ( expr->rtti_isWith() ) {
            auto wth = static_pointer_cast<ExprWith>(expr);
            return exprReturns(wth->body);
        } else if ( expr->rtti_isWhile() ) {
            auto wh = static_pointer_cast<ExprWhile>(expr);
            return exprReturns(wh->body);
        } else if ( expr->rtti_isFor() ) {
            auto fr = static_pointer_cast<ExprFor>(expr);
            return exprReturns(fr->body);
        } else if ( expr->rtti_isUnsafe() ) {
            auto us = static_pointer_cast<ExprUnsafe>(expr);
            return exprReturns(us->body);
        }
        return false;
    }

   bool exprReturnsOrBreaks ( const ExpressionPtr & expr ) {
        if ( expr->rtti_isReturn() ) {
            return true;
        } else if ( expr->rtti_isBlock() ) {
            auto block = static_pointer_cast<ExprBlock>(expr);
            if ( hasLabels(block) ) {
                // NOTE: block with labels assumed to always return for now. real world analysis is hard
                return true;
            } else {
                for ( auto & be : block->list ) {
                    if ( be->rtti_isBreak() || be->rtti_isContinue() || be->rtti_isGoto() ) {
                        return true;
                    }
                    if ( exprReturnsOrBreaks(be) ) {
                        return true;
                    }
                }
            }
        } else if ( expr->rtti_isIfThenElse() ) {
            auto ite = static_pointer_cast<ExprIfThenElse>(expr);
            if ( ite->if_false ) {
                return exprReturnsOrBreaks(ite->if_true) && exprReturnsOrBreaks(ite->if_false);
            }
        } else if ( expr->rtti_isWith() ) {
            auto wth = static_pointer_cast<ExprWith>(expr);
            return exprReturnsOrBreaks(wth->body);
        } else if ( expr->rtti_isWhile() ) {
            auto wh = static_pointer_cast<ExprWhile>(expr);
            return exprReturns(wh->body);   // note has its own break
        } else if ( expr->rtti_isFor() ) {
            auto fr = static_pointer_cast<ExprFor>(expr);
            return exprReturns(fr->body);   // note has its own break
        } else if ( expr->rtti_isUnsafe() ) {
            auto us = static_pointer_cast<ExprUnsafe>(expr);
            return exprReturnsOrBreaks(us->body);
        }
        return false;
    }

    bool needAvoidNullPtr ( const TypeDeclPtr & type, bool allowDim ) {
        if ( !type ) {
            return false;
        }
        if ( !allowDim && type->dim.size() ) {
            return false;
        }
        if ( auto * ann = (TypeAnnotation *) type->isPointerToAnnotation() ) {
            if ( ann->avoidNullPtr() ) {
                return true;
            }
        }
        return false;
    }

    class LintVisitor : public Visitor {
        bool checkOnlyFastAot;
        bool checkAotSideEffects;
        bool checkNoGlobalHeap;
        bool checkNoGlobalVariables;
        bool checkNoGlobalVariablesAtAll;
        bool checkUnusedArgument;
        bool checkUnusedBlockArgument;
        bool checkUnsafe;
        bool checkDeprecated;
        bool disableInit;
        bool noLocalClassMembers;
    public:
        LintVisitor ( const ProgramPtr & prog ) : program(prog) {
            checkOnlyFastAot = program->options.getBoolOption("only_fast_aot", program->policies.only_fast_aot);
            checkAotSideEffects = program->options.getBoolOption("aot_order_side_effects", program->policies.aot_order_side_effects);
            checkNoGlobalHeap = program->options.getBoolOption("no_global_heap", program->policies.no_global_heap);
            checkNoGlobalVariables = program->options.getBoolOption("no_global_variables", program->policies.no_global_variables);
            checkNoGlobalVariablesAtAll = program->options.getBoolOption("no_global_variables_at_all", program->policies.no_global_variables_at_all);
            checkUnusedArgument = program->options.getBoolOption("no_unused_function_arguments", program->policies.no_unused_function_arguments);
            checkUnusedBlockArgument = program->options.getBoolOption("no_unused_block_arguments", program->policies.no_unused_block_arguments);
            checkUnsafe = program->policies.no_unsafe || program->thisModule->doNotAllowUnsafe;
            checkDeprecated = program->options.getBoolOption("no_deprecated", program->policies.no_deprecated);
            disableInit = prog->options.getBoolOption("no_init", prog->policies.no_init);
            noLocalClassMembers = prog->options.getBoolOption("no_local_class_members", prog->policies.no_local_class_members);
        }
    protected:
        void verifyOnlyFastAot ( Function * _func, const LineInfo & at ) {
            if ( !checkOnlyFastAot ) return;
            if ( _func && _func->builtIn ) {
                auto bif = (BuiltInFunction *) _func;
                if ( bif->cppName.empty() ) {
                    program->error(_func->describe() + " has no cppName while onlyFastAot option is set", "", "", at,
                                   CompilationError::only_fast_aot_no_cpp_name );
                }
            }
        }
        bool isValidModuleName(const string & str) const {
            return !isCppKeyword(str);
        }
        virtual void preVisitModule ( Module * mod ) override {
            Visitor::preVisitModule(mod);
            if ( !mod->name.empty() && !isValidModuleName(mod->name) ) {
                program->error("invalid module name '" + mod->name + "'", "", "",
                    LineInfo(), CompilationError::invalid_name );
            }
        }
        bool isValidEnumName(const string & str) const {
            return !isCppKeyword(str);
        }
        bool isValidEnumValueName(const string & str) const {
            return !isCppKeyword(str);
        }
        void lintType ( TypeDecl * td ) {
            for ( auto & name : td->argNames ) {
                if (!isValidVarName(name)) {
                    program->error("invalid type argument name '" + name + "'", "", "",
                        td->at, CompilationError::invalid_name );
                }
            }
            if ( td->firstType ) lintType(td->firstType.get());
            if ( td->secondType ) lintType(td->secondType.get());
            for ( auto & arg : td->argTypes ) lintType(arg.get());
        }
        virtual void preVisit ( TypeDecl * td ) override {
            Visitor::preVisit(td);
            lintType(td);
        }
        virtual void preVisitAlias ( TypeDecl * td, const string & name ) override {
            Visitor::preVisitAlias(td,name);
            if ( !td->isAuto() ) {
                if ( td->getSizeOf64()>0x7fffffff ) {
                    program->error("alias '" + name + "' is too big", "", "",
                        td->at, CompilationError::invalid_type );
                }
            }
        }
        virtual void preVisit ( Enumeration * enu ) override {
            Visitor::preVisit(enu);
            if (!isValidEnumName(enu->name)) {
                program->error("invalid enumeration name '" + enu->name + "'", "", "",
                    enu->at, CompilationError::invalid_name );
            }
        }
        virtual void preVisitEnumerationValue ( Enumeration * enu, const string & name, Expression * value, bool last ) override {
            Visitor::preVisitEnumerationValue(enu,name,value,last);
            if (!isValidEnumValueName(name)) {
                program->error("invalid enumeration value name '" + name + "'", "", "",
                    enu->at, CompilationError::invalid_name );
            }
        }
        bool isValidStructureName(const string & str) const {
            return !isCppKeyword(str);
        }
        virtual void preVisit ( Structure * var ) override {
            Visitor::preVisit(var);
            if (!isValidStructureName(var->name)) {
                program->error("invalid structure name '" + var->name + "'", "", "",
                    var->at, CompilationError::invalid_name );
            }
            if ( var->getSizeOf64()>0x7fffffff ) {
                program->error("structure '" + var->name + "' is too big", "", "",
                    var->at, CompilationError::invalid_type );
            }
        }
        virtual void preVisitExpression ( Expression * expr ) override {
            if ( expr->alwaysSafe && expr->userSaidItsSafe && !expr->generated ) {
                auto origin = func->getOrigin();
                if ( !(origin && origin->generated) && !func->generated )
                {
                    auto fnMod = origin ? origin->module : func->module;
                    if ( fnMod == program->thisModule.get() ) {
                        anyUnsafe = true;
                        if ( checkUnsafe ) {
                            program->error("unsafe function '" + func->getMangledName() + "'", "unsafe functions are prohibited by CodeOfPolicies", "",
                                expr->at, CompilationError::unsafe_function);
                        }
                    }
                }
            }
        }
        bool isValidVarName(const string & str) const {
            return !isCppKeyword(str);
        }
        virtual void preVisitStructureField ( Structure * var, Structure::FieldDeclaration & decl, bool last ) override {
            Visitor::preVisitStructureField(var, decl, last);
            if (!isValidVarName(decl.name)) {
                program->error("invalid structure field name " + decl.name, "", "",
                    decl.at, CompilationError::invalid_name );
            }
            if ( noLocalClassMembers ) {
                if ( !decl.type->ref && decl.type->hasClasses() ) {
                    program->error("class can't contain local class declarations", decl.name + ": " + decl.type->describe(), "",
                        decl.at, CompilationError::invalid_structure_field_type);
                }
            }
        }
        virtual void preVisitGlobalLet ( const VariablePtr & var ) override {
            Visitor::preVisitGlobalLet(var);
            if (!isValidVarName(var->name)) {
                program->error("invalid variable name '" + var->name + "'", "", "",
                    var->at, CompilationError::invalid_name );
            }
            if ( checkNoGlobalVariables && !var->generated ) {
                if ( checkNoGlobalVariablesAtAll ) {
                    program->error("variable '" + var->name + "' is disabled via option no_global_variables_at_all", "", "",
                        var->at, CompilationError::no_global_variables );
                } else if ( !var->type->isConst() ) {
                    program->error("variable '" + var->name + "' is not a constant, which is disabled via option no_global_variables", "", "",
                        var->at, CompilationError::no_global_variables );
                }
            }
            if ( checkNoGlobalHeap ) {
                if ( !var->type->isNoHeapType() ) { // note: this is too dangerous to allow even with generated
                    program->error("variable '" + var->name + "' uses heap, which is disabled via option no_global_heap", "", "",
                        var->at, CompilationError::no_global_heap );
                }
            }
            if ( !var->init ) {
                if ( needAvoidNullPtr(var->type,true) ) {
                    program->error("global variable of type '" + var->type->describe() + "' needs to be initialized to avoid null pointer", "", "",
                        var->at, CompilationError::cant_be_null);
                }
            } else {
                if ( needAvoidNullPtr(var->type,false) && var->init->rtti_isNullPtr() ) {
                    program->error("global variable of type '" + var->type->describe() + "' can't be initialized with null", "", "",
                        var->init->at, CompilationError::cant_be_null);
                }
            }
            if ( var->type->getSizeOf64()>0x7fffffff ) {
                program->error("global variable '" + var->name + "' is too big", "", "",
                    var->at,CompilationError::invalid_variable_type);
            }
        }
        virtual void preVisitGlobalLetInit ( const VariablePtr & var, Expression * that ) override {
            Visitor::preVisitGlobalLetInit(var,that);
            globalVar = var.get();
        }
        virtual ExpressionPtr visitGlobalLetInit ( const VariablePtr & var, Expression * that ) override {
            if ( disableInit && !var->init->rtti_isConstant() ) {   // we double check here, if it made it past infer
                program->error("[init] is disabled in the options or CodeOfPolicies", "", "",
                        var->at, CompilationError::no_init);
            }
            globalVar->index = -3; // initialized. -1 by default
            globalVar = nullptr;
            return Visitor::visitGlobalLetInit(var,that);;
        }
        virtual void preVisit(ExprVar * expr) override {
            Visitor::preVisit(expr);
            if ( globalVar && expr->isGlobalVariable() ) {
                if ( expr->variable->index!=-3 ) {
                    if ( expr->variable->module==globalVar->module ) {
                        program->error("global variable " + expr->name + " is initialized after " + globalVar->name + " (" + to_string(expr->variable->index) + ")",
                            "", "", expr->at, CompilationError::variable_not_found);
                    }
                }
            }
        }

        virtual void preVisit(ExprFor * expr) override {
            Visitor::preVisit(expr);
            // macro generated invisible variable
            // DAS_ASSERT(expr->visibility.line);
        }
        virtual void preVisit(ExprDelete * expr) override {
            Visitor::preVisit(expr);
            if ( needAvoidNullPtr(expr->subexpr->type,true) ) {
                program->error("can't delete " + expr->subexpr->type->describe() + ", it will create null pointer", "", "",
                    expr->subexpr->at, CompilationError::cant_be_null);
            }

        }
        virtual void preVisit(ExprLet * expr) override {
            Visitor::preVisit(expr);
            // macro genearted invisible variable
            // DAS_ASSERT(expr->visibility.line);
            for (const auto & var : expr->variables) {
                if (!isValidVarName(var->name)) {
                    program->error("invalid variable name " + var->name, "", "",
                        var->at, CompilationError::invalid_name );
                }
                if ( !var->init ) {
                    if ( needAvoidNullPtr(var->type,true) ) {
                        program->error("local variable of type " + var->type->describe() + " needs to be initialized to avoid null pointer", "", "",
                            var->at, CompilationError::cant_be_null);
                    }
                } else {
                    if ( needAvoidNullPtr(var->type,false) && var->init->rtti_isNullPtr() ) {
                        program->error("local variable of type " + var->type->describe() + " can't be initialized with null", "", "",
                            var->init->at, CompilationError::cant_be_null);
                    }
                }
                if ( var->type->getSizeOf64()>0x7fffffff ) {
                    program->error("local variable " + var->name + " is too big", "", "",
                        var->at,CompilationError::invalid_variable_type);
                }
            }
        }
        virtual void preVisit ( ExprReturn * expr ) override {
            Visitor::preVisit(expr);
            if ( expr->returnType && needAvoidNullPtr(expr->returnType,false) && expr->subexpr->rtti_isNullPtr() ) {
                program->error("can't return null", "", "",
                    expr->subexpr->at, CompilationError::cant_be_null);
            }
        }
        void verifyToTableMove ( ExprCall * expr ) {
            if ( expr->arguments[0]->rtti_isMakeArray() ) {
                auto ma = static_cast<ExprMakeArray *>(expr->arguments[0].get());
                if ( ma->values.size()==0 ) return;
                if ( ma->recordType->isTuple() ) {
                    if ( ma->recordType->argTypes[0]->isString() ) {
                        das_set<const char *,hash_ccs,equalto_ccs> seen;
                        for ( const auto & arg : ma->values ) {
                            if ( arg->rtti_isMakeTuple() ) {
                                auto mt = static_cast<ExprMakeTuple *>(arg.get());
                                if ( mt->values.size()==2 ) {   // its redundant
                                    if ( mt->values[0]->rtti_isStringConstant() ) {
                                        auto mc = static_cast<ExprConstString *>(mt->values[0].get());
                                        if ( seen.find(mc->text.c_str())==seen.end() ) {
                                            seen.insert(mc->text.c_str());
                                        } else {
                                            program->error("duplicate key in string=> table initialization", "", "",
                                                mt->values[0]->at, CompilationError::duplicate_key);
                                        }
                                    }
                                }
                            }
                        }
                    } else {
                        das_set<vec4f,hash_vec4f,equalto_vec4f> seen;
                        for ( const auto & arg : ma->values ) {
                            if ( arg->rtti_isMakeTuple() ) {
                                auto mt = static_cast<ExprMakeTuple *>(arg.get());
                                if ( mt->values.size()==2 ) {   // its redundant
                                    if ( mt->values[0]->rtti_isConstant() ) {
                                        auto mc = static_cast<ExprConst *>(mt->values[0].get());
                                        if ( seen.find(mc->value)==seen.end() ) {
                                            seen.insert(mc->value);
                                        } else {
                                            program->error("duplicate key in table initialization", "", "",
                                                mt->values[0]->at, CompilationError::duplicate_key);
                                        }
                                    }
                                }
                            }
                        }
                    }
                } else {
                    if ( ma->recordType->isString() ) {
                        das_set<const char *,hash_ccs,equalto_ccs> seen;
                        for ( const auto & arg : ma->values ) {
                            if ( arg->rtti_isStringConstant() ) {
                                auto mc = static_cast<ExprConstString *>(arg.get());
                                if ( seen.find(mc->text.c_str())==seen.end() ) {
                                    seen.insert(mc->text.c_str());
                                } else {
                                    program->error("duplicate key in string=> set initialization", "", "",
                                        arg->at, CompilationError::duplicate_key);
                                }
                            }
                        }
                    } else {
                        das_set<vec4f,hash_vec4f,equalto_vec4f> seen;
                        for ( const auto & arg : ma->values ) {
                            if ( arg->rtti_isConstant() ) {
                                auto mc = static_cast<ExprConst *>(arg.get());
                                if ( seen.find(mc->value)==seen.end() ) {
                                    seen.insert(mc->value);
                                } else {
                                    program->error("duplicate key in set initialization", "", "",
                                        arg->at, CompilationError::duplicate_key);
                                }
                            }
                        }
                    }
                }
            }
        }
        virtual void preVisit ( ExprCall * expr ) override {
            Visitor::preVisit(expr);
            verifyOnlyFastAot(expr->func, expr->at);
            if ( checkDeprecated && expr->func->deprecated ) {
                string message = "";
                for ( auto & ann : expr->func->annotations ) {
                    if ( ann->annotation->rtti_isFunctionAnnotation() ) {
                        auto fnAnn = static_pointer_cast<FunctionAnnotation>(ann->annotation);
                        if ( fnAnn->name=="deprecated" ) {
                            for ( auto & arg : ann->arguments ) {
                                if ( arg.name=="message" && arg.type==Type::tString ) {
                                    message = arg.sValue;
                                }
                            }
                            break;
                        }
                    }
                }
                program->error("function " + expr->func->getMangledName() + " is deprecated.","deprecated functions are prohibited by CodeOfPolicies", message,
                    expr->at, CompilationError::deprecated_function);
            }
            for ( const auto & annDecl : expr->func->annotations ) {
                auto ann = annDecl->annotation;
                if ( ann->rtti_isFunctionAnnotation() ) {
                    auto fnAnn = static_pointer_cast<FunctionAnnotation>(ann);
                    string err;
                    if ( !fnAnn->verifyCall(expr, annDecl->arguments, program->options, err) ) {
                        program->error("call annotated by " + fnAnn->name + " failed", err, "",
                                       expr->at, CompilationError::annotation_failed);
                    }
                }
            }
            if ( checkAotSideEffects ) {
                if ( expr->arguments.size()>1 ) {
                    for ( auto & arg : expr->arguments ) {
                        if ( !arg->noNativeSideEffects ) {
                            program->error("side effects may affect function " + expr->func->name + " evaluation order", "", "",
                                expr->at, CompilationError::aot_side_effects );
                            break;
                        }
                    }
                }
            }
            for ( size_t i=0, is=expr->arguments.size(); i!=is; ++i ) {
                const auto & arg = expr->arguments[i];
                const auto & funArg = expr->func->arguments[i];
                const auto & argType = funArg->type;
                if ( needAvoidNullPtr(argType,false) && arg->rtti_isNullPtr() ) {
                    program->error("can't pass null to function " + expr->func->describeName() + " argument " + funArg->name , "", "",
                        arg->at, CompilationError::cant_be_null);
                }
            }
            if ( expr->func->name=="builtin`to_table_move" && expr->func->fromGeneric && expr->func->fromGeneric->module->name=="$" ) {
                verifyToTableMove(expr);
            }
        }
        virtual void preVisit ( ExprOp1 * expr ) override {
            Visitor::preVisit(expr);
            verifyOnlyFastAot(expr->func, expr->at);
        }
        virtual void preVisit ( ExprOp2 * expr ) override {
            Visitor::preVisit(expr);
            verifyOnlyFastAot(expr->func, expr->at);
            if ( checkAotSideEffects ) {
                if ( !expr->left->noNativeSideEffects || !expr->right->noNativeSideEffects ) {
                    program->error("side effects may affect evaluation order", "", "", expr->at,
                                   CompilationError::aot_side_effects );
                }
            }
        }
        virtual void preVisit ( ExprOp3 * expr ) override {
            Visitor::preVisit(expr);
            verifyOnlyFastAot(expr->func, expr->at);
            if ( checkAotSideEffects ) {
                if ( !expr->subexpr->noNativeSideEffects || !expr->left->noNativeSideEffects || !expr->right->noNativeSideEffects ) {
                    program->error("side effects may affect evaluation order", "", "", expr->at,
                                   CompilationError::aot_side_effects );
                }
            }
        }
        virtual void preVisit ( ExprCopy * expr ) override {
            Visitor::preVisit(expr);
            verifyOnlyFastAot(expr->func, expr->at);
            // @E1 = E2, E2 side effects are before E1 side effects
            /*
            if ( checkAotSideEffects ) {
                if ( !expr->left->noNativeSideEffects || !expr->right->noNativeSideEffects ) {
                    program->error("side effects may affect copy evaluation order", expr->at,
                                   CompilationError::aot_side_effects );
                }
            }
            */
            if ( needAvoidNullPtr(expr->left->type,false) && expr->right->rtti_isNullPtr() ) {
                program->error("can't assign null pointer to " + expr->left->type->describe(), "", "",
                    expr->right->at, CompilationError::cant_be_null);
            }
        }
        virtual void preVisit ( ExprMove * expr ) override {
            Visitor::preVisit(expr);
            verifyOnlyFastAot(expr->func, expr->at);
            if ( checkAotSideEffects ) {
                if ( !expr->left->noNativeSideEffects || !expr->right->noNativeSideEffects ) {
                    program->error("side effects may affect move evaluation order", "", "",
                        expr->at, CompilationError::aot_side_effects );
                }
            }
            if ( needAvoidNullPtr(expr->left->type,false) && expr->right->rtti_isNullPtr() ) {
                program->error("can't assign null pointer to " + expr->left->type->describe(), "", "",
                    expr->right->at, CompilationError::cant_be_null);
            }
        }
        virtual void preVisit ( ExprClone * expr ) override {
            Visitor::preVisit(expr);
            verifyOnlyFastAot(expr->func, expr->at);
            if ( checkAotSideEffects ) {
                if ( !expr->left->noNativeSideEffects || !expr->right->noNativeSideEffects ) {
                    program->error("side effects may affect clone evaluation order", "", "",
                        expr->at, CompilationError::aot_side_effects );
                }
            }
        }
        virtual void preVisit ( ExprAscend * expr ) override {
            Visitor::preVisit(expr);
            if ( expr->subexpr->type->getSizeOf64()>0x7fffffff ) {
                program->error("can't ascend type which is too big", "", "",
                    expr->at, CompilationError::invalid_new_type);
            }
            if ( !expr->subexpr->type->getSizeOf64() ) {
                program->error("can't ascend (to heap) type of size 0",  "", "",
                    expr->at, CompilationError::invalid_new_type);
            }
        }
        virtual void preVisit ( ExprNew * expr ) override {
            Visitor::preVisit(expr);
            if ( expr->typeexpr->getSizeOf64()>0x7fffffff ) {
                program->error("can't new to a type that is too big", "", "",
                    expr->at, CompilationError::invalid_new_type);
            }
            if ( !expr->typeexpr->getSizeOf64() ) {
                program->error("can't new (to heap) type of size 0",  "", "",
                    expr->at, CompilationError::invalid_new_type);
            }
        }
        virtual void preVisit ( ExprAssert * expr ) override {
            Visitor::preVisit(expr);
            if ( !expr->isVerify && !expr->arguments[0]->noSideEffects ) {
                program->error("assert expressions can't have side-effects (use verify instead)", "", "",
                    expr->at, CompilationError::assert_with_side_effects);
            }
        }
        virtual void preVisit ( ExprUnsafe * expr ) override {
            if ( !expr->generated ) {
                auto origin = func->getOrigin();
                if ( !(origin && origin->generated) && !func->generated )
                {
                    auto fnMod = origin ? origin->module : func->module;
                    if ( fnMod == program->thisModule.get() ) {
                        anyUnsafe = true;
                        if ( checkUnsafe ) {
                            program->error("unsafe function " + func->getMangledName(), "unsafe functions are prohibited by CodeOfPolicies", "",
                                expr->at, CompilationError::unsafe_function);
                        }
                    }
                }
            }
        }
        bool isValidFunctionName(const string & str) const {
            return !isCppKeyword(str);
        }
        virtual void preVisit ( Function * fn ) override {
            Visitor::preVisit(fn);
            func = fn;
            if (!isValidFunctionName(fn->name)) {
                program->error("invalid function name " + fn->name, "", "",
                    fn->at, CompilationError::invalid_name );
            }
            if ( !fn->result->isVoid() && !fn->result->isAuto() ) {
                if ( !exprReturns(fn->body) ) {
                    program->error("not all control paths return value",  "", "",
                        fn->at, CompilationError::not_all_paths_return_value);
                }
            }
            if ( !fn->safeImplicit ) {
                for ( auto arg : fn->arguments ) {
                    if ( hasImplicit(arg->type) ) {
                        auto origin = fn->getOrigin();
                        auto fnMod = origin ? origin->module : fn->module;
                        if ( fnMod == program->thisModule.get() ) {
                            anyUnsafe = true;
                            if ( checkUnsafe ) {
                                program->error("implicit argument " + arg->name,  "implicit is unsafe and is prohibited by the CodeOfPolicies", "",
                                    fn->at, CompilationError::unsafe_function);
                            }
                        }
                    }
                }
            }
            for ( auto & ann : fn->annotations ) {
                if ( ann->annotation->rtti_isFunctionAnnotation() ) {
                    auto fann = static_pointer_cast<FunctionAnnotation>(ann->annotation);
                    string err;
                    if ( !fann->lint(fn, *program->thisModuleGroup, ann->arguments, program->options, err) ) {
                        program->error("function annotation lint failed\n", err, "", fn->at, CompilationError::annotation_failed );
                    }
                }
            }
            if ( (fn->init | fn->shutdown) && disableInit ) { // we double-check here. we check in the infer first, but this here is for the case where macro does it later
                program->error("[init] is disabled in the options or CodeOfPolicies",  "", "",
                    fn->at, CompilationError::no_init);
            }
        }
        virtual FunctionPtr visit ( Function * fn ) override {
            func = nullptr;
            return Visitor::visit(fn);
        }
        virtual void preVisitArgument ( Function * fn, const VariablePtr & var, bool lastArg ) override {
            Visitor::preVisitArgument(fn, var, lastArg);
            if (!isValidVarName(var->name)) {
                program->error("invalid argument variable name " + var->name, "", "",
                    var->at, CompilationError::invalid_name );
            }
            if ( checkUnusedArgument ) {
                if ( !var->marked_used && var->isAccessUnused() ) {
                    program->error("unused function argument " + var->name, "",
                          "use [unused_argument(" + var->name + ")] if intentional",
                        var->at, CompilationError::unused_function_argument);
                }
            }
            if ( var->type->getSizeOf64()>0x7fffffff ) {
                program->error("argument variable " + var->name + " is too big", "", "",
                    var->at,CompilationError::invalid_variable_type);
            }
        }
        virtual void preVisit ( ExprBlock * block ) override {
            Visitor::preVisit(block);
            if ( block->isClosure ) {
                if (  !block->returnType->isVoid() && !block->returnType->isAuto() ) {
                    if ( !exprReturns(block) ) {
                        program->error("not all control paths of the block return value",  "", "",
                            block->at, CompilationError::not_all_paths_return_value);
                    }
                }
            }
        }
        virtual void preVisitBlockArgument ( ExprBlock * block, const VariablePtr & var, bool lastArg ) override {
            Visitor::preVisitBlockArgument(block, var, lastArg);
            if (!isValidVarName(var->name)) {
                program->error("invalid block argument variable name " + var->name, "", "",
                    var->at, CompilationError::invalid_name );
            }
            if ( checkUnusedBlockArgument ) {
                if ( !var->marked_used && var->isAccessUnused() ) {
                    program->error("unused block argument " + var->name, "",
                          "use [unused_argument(" + var->name + ")] if intentional",
                        var->at, CompilationError::unused_block_argument);
                }
            }
            if ( var->type->getSizeOf64()>0x7fffffff ) {
                program->error("block argument variable " + var->name + " is too big", "", "",
                    var->at,CompilationError::invalid_variable_type);
            }
        }
        virtual void preVisitBlockExpression ( ExprBlock * block, Expression * expr ) override {
            Visitor::preVisitBlockExpression(block, expr);
            if ( expr->rtti_isOp2() ) {
                auto op2 = static_cast<ExprOp2 *>(expr);
                if ( op2->func && op2->func->builtIn && op2->func->sideEffectFlags==0 ) {
                    program->error("top level no side effect operation " + op2->op, "", "",
                        expr->at, CompilationError::top_level_no_sideeffect_operation);
                }
            }
        }
    public:
        ProgramPtr program;
        Function * func = nullptr;
        Variable * globalVar = nullptr;
        bool anyUnsafe = false;
    };

    struct Option {
        const char *    name;
        Type            type;
    } g_allOptions [] = {
    // lint
        "lint",                         Type::tBool,
        "only_fast_aot",                Type::tBool,
        "aot_order_side_effects",       Type::tBool,
        "no_global_heap",               Type::tBool,
        "no_global_variables",          Type::tBool,
        "no_global_variables_at_all",   Type::tBool,
        "no_unused_function_arguments", Type::tBool,
        "no_unused_block_arguments",    Type::tBool,
        "no_deprecated",                Type::tBool,
        "no_aliasing",                  Type::tBool,
        "strict_smart_pointers",        Type::tBool,
        "no_init",                      Type::tBool,
        "no_local_class_members",       Type::tBool,
        "report_invisible_functions",   Type::tBool,
        "report_private_functions",     Type::tBool,
        "strict_properties",            Type::tBool,
        "very_safe_context",            Type::tBool,
    // memory
        "stack",                        Type::tInt,
        "intern_strings",               Type::tBool,
        "multiple_contexts",            Type::tBool,
        "persistent_heap",              Type::tBool,
        "heap_size_hint",               Type::tInt,
        "heap_size_limit",              Type::tInt,
        "string_heap_size_hint",        Type::tInt,
        "string_heap_size_limit",       Type::tInt,
        "gc",                           Type::tBool,
        "solid_context",                Type::tBool,    // we will not have AOT or patches
    // aot
        "no_aot",                       Type::tBool,
        "aot_prologue",                 Type::tBool,
    // logging
        "log",                          Type::tBool,
        "log_optimization_passes",      Type::tBool,
        "log_optimization",             Type::tBool,
        "log_stack",                    Type::tBool,
        "log_init",                     Type::tBool,
        "log_symbol_use",               Type::tBool,
        "log_var_scope",                Type::tBool,
        "log_nodes",                    Type::tBool,
        "log_nodes_aot_hash",           Type::tBool,
        "log_mem",                      Type::tBool,
        "log_debug_mem",                Type::tBool,
        "log_cpp",                      Type::tBool,
        "log_aot",                      Type::tBool,
        "log_infer_passes",             Type::tBool,
        "log_require",                  Type::tBool,
        "log_compile_time",             Type::tBool,
        "log_total_compile_time",       Type::tBool,
        "log_generics",                 Type::tBool,
        "log_mn_hash",                  Type::tBool,
        "log_gmn_hash",                 Type::tBool,
        "log_ad_hash",                  Type::tBool,
        "log_aliasing",                 Type::tBool,
        "print_ref",                    Type::tBool,
        "print_var_access",             Type::tBool,
        "print_c_style",                Type::tBool,
        "print_func_use",               Type::tBool,
        "gen2_make_syntax",             Type::tBool,
        "relaxed_assign",               Type::tBool,
    // rtti
        "rtti",                         Type::tBool,
    // optimization
        "optimize",                     Type::tBool,
        "fusion",                       Type::tBool,
        "remove_unused_symbols",        Type::tBool,
        "no_fast_call",                 Type::tBool,
    // language
        "always_export_initializer",    Type::tBool,
        "infer_time_folding",           Type::tBool,
        "disable_run",                  Type::tBool,
        "max_infer_passes",             Type::tInt,
        "indenting",                    Type::tInt,
        "no_unsafe_uninitialized_structures", Type::tBool,
        "relaxed_pointer_const",        Type::tBool,
        "unsafe_table_lookup",          Type::tBool,
    // debugger
        "debugger",                     Type::tBool,
    // profiler
        "profiler",                     Type::tBool,
    // runtime checks
        "skip_lock_checks",             Type::tBool,
        "skip_module_lock_checks",      Type::tBool,
    // pinvoke
        "threadlock_context",           Type::tBool,
    // version_2_syntax
        "gen2",                         Type::tBool,
    };

    void verifyOptions() {
        bool failed = false;
        for ( const auto & opt : g_allOptions ) {
            if ( !isValidBuiltinName(opt.name) ) {
                DAS_FATAL_ERROR("%s - invalid option. expecting snake_case\n", opt.name);
                failed = true;
            }
        }
        DAS_VERIFYF(!failed, "verifyOptions failed");
    }

    void Program::lint ( TextWriter & /*logs*/, ModuleGroup & libGroup ) {
        if (!options.getBoolOption("lint", true)) {
            return;
        }
        // note: build access flags is now called before patchAnnotations, and is no longer needed in lint
        // TextWriter logs; buildAccessFlags(logs);
        checkSideEffects();
        // lint it
        LintVisitor lintV(this);
        visit(lintV);
        unsafe = lintV.anyUnsafe;
        // check for invalid options
        das_map<string,Type> ao;
        for ( const auto & opt : g_allOptions ) {
            ao[opt.name] = opt.type;
        }
        for ( const auto & opt : options ) {
            Type optT = Type::none;
            auto it = ao.find(opt.name);
            if ( it != ao.end() ) {
                optT = it->second;
            } else {
                for ( auto & mod : libGroup.modules ) {
                    auto ot = mod->getOptionType(opt.name);
                    if ( ot!=Type::none ) {
                        optT = ot;
                        break;
                    }
                }
            }
            if ( optT!=Type::none && optT!=opt.type ) {
                error("invalid option type for '" + opt.name
                      + "', unexpected '" + das_to_string(opt.type)
                      + "', expecting '" + das_to_string(optT) + "'", "", "",
                        LineInfo(), CompilationError::invalid_option);
            } else if ( optT==Type::none ){
                error("invalid option '" + opt.name + "'",  "", "",
                    LineInfo(), CompilationError::invalid_option);
            }
        }
        set<Module *> lints;
        Module::foreach([&](Module * mod) -> bool {
            DAS_ASSERT ( mod!=thisModule.get() );
            for ( const auto & pm : mod->globalLintMacros ) {
                pm->apply(this, thisModule.get());
            }
            return true;
        });
        libGroup.foreach([&](Module * mod) -> bool {
            if ( thisModule->isVisibleDirectly(mod) && mod!=thisModule.get() ) {
                for ( const auto & pm : mod->lintMacros ) {
                    pm->apply(this, thisModule.get());
                }
            }
            return true;
        },"*");
    }
}

