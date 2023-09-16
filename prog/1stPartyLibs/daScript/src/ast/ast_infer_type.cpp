#include "daScript/misc/platform.h"

#include "daScript/ast/ast.h"
#include "daScript/ast/ast_visitor.h"
#include "daScript/ast/ast_generate.h"

#define DAS_XSTR(s) #s
#define DAS_STR(s) DAS_XSTR(s)

namespace das {

    // todo: check for eastl and look for better container
    typedef vector<Function *>  MatchingFunctions;
    class CaptureLambda : public Visitor {
    public:
        virtual void preVisit ( ExprVar * expr ) override {
            if ( !expr->type ) {    // trying to capture non-inferred section
                fail = true;
                return;
            }
            auto var = expr->variable;
            if ( expr->local || expr->argument || expr->block ) {
                if ( expr->argument || (scope.find(var) != scope.end()) ) {
                    auto varT = var->type;
                    if ( !varT || varT->isAutoOrAlias() ) {
                        fail = true;
                        return;
                    }
                    capt.insert(var);
                }
            }
        }
        das_hash_set<VariablePtr, smart_ptr_hash> scope;
        safe_var_set                              capt;
        bool                                      fail = false;
    };

// type inference

    class InferTypes : public FoldingVisitor {
    public:
        InferTypes( const ProgramPtr & prog ) : FoldingVisitor(prog ) {
            enableInferTimeFolding = prog->options.getBoolOption("infer_time_folding",true);
            disableAot = prog->options.getBoolOption("no_aot",false);
            multiContext = prog->options.getBoolOption("multiple_contexts", prog->policies.multiple_contexts);
            checkNoGlobalVariablesAtAll = prog->options.getBoolOption("no_global_variables_at_all", prog->policies.no_global_variables_at_all);
            strictSmartPointers = prog->options.getBoolOption("strict_smart_pointers", prog->policies.strict_smart_pointers);
            disableInit = prog->options.getBoolOption("no_init", prog->policies.no_init);
            skipModuleLockChecks = prog->options.getBoolOption("skip_module_lock_checks", false);
            strictUnsafeDelete = prog->options.getBoolOption("strict_unsafe_delete", prog->policies.strict_unsafe_delete);
        }
        bool finished() const { return !needRestart; }
        bool verbose = true;
    protected:
        FunctionPtr             func;
        VariablePtr             globalVar;
        vector<VariablePtr>     local;
        vector<ExpressionPtr>   loop;
        vector<ExprBlock *>     blocks;
        vector<ExprBlock *>     scopes;
        vector<ExprWith *>      with;
        vector<ExprAssume *>    assume;
        vector<size_t>          varStack;
        vector<size_t>          assumeStack;
        vector<bool>            inFinally;
        bool                    canFoldResult = true;
        das_hash_set<int32_t>   labels;
        size_t                  fieldOffset = 0;
        int32_t                 fieldIndex = 0;
        bool                    cppLayout = false;
        bool                    cppLayoutPod = false;
        const Structure *       cppLayoutParent = nullptr;
        bool                    needRestart = false;
        bool                    enableInferTimeFolding = true;
        bool                    disableAot = false;
        bool                    multiContext = false;
        Expression *            lastEnuValue = nullptr;
        int32_t                 unsafeDepth = 0;
        bool                    checkNoGlobalVariablesAtAll = false;
        bool                    strictSmartPointers = false;
        bool                    disableInit = false;
        bool                    skipModuleLockChecks = false;
        bool                    strictUnsafeDelete = false;
    public:
        vector<FunctionPtr>     extraFunctions;
    protected:
        string generateNewLambdaName(const LineInfo & at) {
            string mod = ctx.thisProgram->thisModule->name;
            if ( mod.empty() ) mod = "thismodule";
            return "_lambda_" + mod + "_" + to_string(at.line) + "_" + to_string(at.column)
                + "_" + to_string(program->newLambdaIndex++);
        }
        string generateNewLocalFunctionName(const LineInfo & at) {
            string mod = ctx.thisProgram->thisModule->name;
            if ( mod.empty() ) mod = "thismodule";
            return "_localfunction_" + mod + "_" + to_string(at.line) + "_" + to_string(at.column)
                + "_" + to_string(program->newLambdaIndex++);
        }
        void pushVarStack() {
            varStack.push_back(local.size());
            assumeStack.push_back(assume.size());
        }
        void popVarStack()  {
            assume.resize(assumeStack.back());
            assumeStack.pop_back();
            local.resize(varStack.back());
            varStack.pop_back();
        }
        void error ( const string & err, const string & extra, const string & fixme, const LineInfo & at, CompilationError cerr = CompilationError::unspecified ) const {
            if ( verbose && func ) {
                string ex = extra;
                if (!extra.empty()) ex += "\n";
                ex += func->getLocationExtra();
                program->error(err,ex,fixme,at,cerr);
            } else {
                program->error(err,extra,fixme,at,cerr);
            }
        }
        void reportAstChanged() {
            needRestart = true;
        }
        virtual void reportFolding() override {
            FoldingVisitor::reportFolding();
            needRestart = true;
        }
        string describeType ( const TypeDeclPtr & decl ) const {
            return verbose ? decl->describe() : "";
        }
        string describeType ( const TypeDecl * decl ) const {
            return verbose ? decl->describe() : "";
        }
        string describeFunction ( const FunctionPtr & fun ) const {
            return verbose ? fun->describe() : "";
        }
        string describeFunction ( const Function * fun ) const {
            return verbose ? fun->describe() : "";
        }
    protected:
        void verifyType ( const TypeDeclPtr & decl, bool allowExplicit = false ) const {
            // TODO: enable and cleanup
            if ( decl->isExplicit && !allowExplicit ) {
                /*
                error("expression can't be explicit here " + describeType(decl), "", "",
                      decl->at,CompilationError::invalid_type);
                */
            }
            /*
            if ( decl->dim.size() && decl->ref ) {
                error("can't declare an array of references, " + describeType(decl), "", "",
                      decl->at,CompilationError::invalid_type);
            }
            */
            uint64_t size = 1;
            for ( auto di : decl->dim ) {
                if ( di<=0 ) {
                    error("array dimension can't be 0 or less, " + describeType(decl), "", "",
                          decl->at,CompilationError::invalid_array_dimension);
                }
                size *= di;
                if ( size>0x7fffffff ) {
                    error("array is too big, " + describeType(decl), "", "",
                          decl->at,CompilationError::invalid_array_dimension);
                    break;
                }
            }
            if ( decl->baseType==Type::tFunction || decl->baseType==Type::tLambda
                || decl->baseType==Type::tBlock || decl->baseType==Type::tVariant ||
                    decl->baseType==Type::tTuple ) {
                if ( decl->argNames.size() && decl->argNames.size()!=decl->argTypes.size() ) {
                    string allNames = "\t";
                    for ( const auto & na : decl->argNames ) {
                        allNames += na + " ";
                    }
                    error("malformed type, " + describeType(decl), allNames, "",
                        decl->at,CompilationError::invalid_type);
                }
            }
            if ( decl->baseType==Type::tVoid ) {
                if ( decl->dim.size() ) {
                    error("can't declare an array of void, " + describeType(decl), "", "",
                          decl->at,CompilationError::invalid_type);
                }
                if ( decl->ref ) {
                    error("can't declare a void reference, " + describeType(decl), "", "",
                          decl->at,CompilationError::invalid_type);
                }
            } else if ( decl->baseType==Type::tPointer ) {
                if ( auto ptrType = decl->firstType ) {
                    if ( ptrType->ref ) {
                        error("can't declare a pointer to a reference, " + describeType(decl), "", "",
                              ptrType->at,CompilationError::invalid_type);
                    }
                    if ( decl->smartPtr ) {
                        if ( ptrType->baseType != Type::tHandle ) {
                            error("can't declare a smart pointer to anything other than annotation, " + describeType(decl), "", "",
                                ptrType->at,CompilationError::invalid_type);
                        } else if ( !ptrType->annotation->isSmart() ) {
                            error("this annotation does not support smart pointers, " + describeType(decl), "", "",
                                ptrType->at,CompilationError::invalid_type);
                        }
                    }
                    verifyType(ptrType);
                } else {
                    if ( decl->smartPtr ) {
                        error("can't declare a void smart pointer", "", "",
                              decl->at,CompilationError::invalid_type);
                    }
                }
            } else if ( decl->baseType==Type::tIterator ) {
                if ( auto ptrType = decl->firstType ) {
                    verifyType(ptrType);
                }
            } else if ( decl->baseType==Type::tArray ) {
                if ( auto arrayType = decl->firstType ) {
                    if ( arrayType->isAutoOrAlias() ) {
                        error("array type is not fully resolved, " + describeType(arrayType), "", "",
                              arrayType->at,CompilationError::invalid_array_type);
                    }
                    if ( arrayType->ref ) {
                        error("can't declare an array of references, " + describeType(arrayType), "", "",
                              arrayType->at,CompilationError::invalid_array_type);
                    }
                    if ( arrayType->baseType==Type::tVoid) {
                        error("can't declare a void array, " + describeType(arrayType), "", "",
                              arrayType->at,CompilationError::invalid_array_type);
                    }
                    if ( !arrayType->canBePlacedInContainer() ) {
                        error("can't have array of non-trivial type, " + describeType(arrayType), "", "",
                              arrayType->at,CompilationError::invalid_type);
                    }
                    verifyType(arrayType);
                }
            } else if ( decl->baseType==Type::tTable ) {
                if ( auto keyType = decl->firstType ) {
                    if ( keyType->isAutoOrAlias() ) {
                        error("table key is not fully resolved, " + describeType(keyType), "", "",
                              keyType->at,CompilationError::invalid_array_type);
                    }
                    if ( keyType->ref ) {
                        error("table key can't be declared as a reference, " + describeType(keyType), "", "",
                              keyType->at,CompilationError::invalid_table_type);
                    }
                    if ( !keyType->isWorkhorseType() ) {
                        error("table key has to be declare as a basic 'hashable' type, " + describeType(keyType), "", "",
                              keyType->at,CompilationError::invalid_table_type);
                    }
                    verifyType(keyType);
                }
                if ( auto valueType = decl->secondType ) {
                    if ( valueType->isAutoOrAlias() ) {
                        error("table value is not fully resolved, " + describeType(valueType), "", "",
                              valueType->at,CompilationError::invalid_array_type);
                    }
                    if ( valueType->ref ) {
                        error("table value can't be declared as a reference, " + describeType(valueType), "", "",
                              valueType->at,CompilationError::invalid_table_type);
                    }
                    if ( !valueType->canBePlacedInContainer() ) {
                        error("can't have table value of non-trivial type, " + describeType(valueType), "", "",
                              valueType->at,CompilationError::invalid_type);
                    }
                    verifyType(valueType);
                }
            } else if ( decl->baseType==Type::tBlock || decl->baseType==Type::tFunction || decl->baseType==Type::tLambda ) {
                if ( auto resultType = decl->firstType ) {
                    if ( !resultType->isReturnType() ) {
                        error("not a valid return type, " + describeType(resultType), "", "",
                              resultType->at,CompilationError::invalid_return_type);
                    }
                    verifyType(resultType);
                }
                for ( auto & argType : decl->argTypes ) {
                    if ( argType->ref && argType->isRefType() ) {
                        error("can't pass a boxed type by a reference, " + describeType(argType), "", "",
                              argType->at,CompilationError::invalid_argument_type);
                    }
                    verifyType(argType,true);
                }
            } else if ( decl->baseType==Type::tTuple ) {
                for ( auto & argType : decl->argTypes ) {
                    if ( argType->ref ) {
                        error("tuple element can't be ref, " + describeType(argType), "", "",
                              argType->at,CompilationError::invalid_type);
                    }
                    verifyType(argType);
                }
            } else if ( decl->baseType==Type::tVariant ) {
                for ( auto & argType : decl->argTypes ) {
                    if ( argType->ref ) {
                        error("variant element can't be ref, " + describeType(argType), "", "",
                              argType->at,CompilationError::invalid_type);
                    }
                    verifyType(argType);
                }
            }
        }

        void propagateTempType ( const TypeDeclPtr & parentType, TypeDeclPtr & subexprType ) {
            if ( subexprType->isTempType() ) {
                if ( parentType->temporary ) subexprType->temporary = true;   // array<int?># -> int?#
            } else {
                subexprType->temporary = false; // array<int#> -> int
            }
        }

        // find type alias name, and resolve it to type
        // without one generic function
        TypeDeclPtr findFuncAlias ( const FunctionPtr & fptr, const string & name ) const {
            for ( auto & arg : fptr->arguments ) {
                if ( auto aT = arg->type->findAlias(name,true) ) {
                    return aT;
                }
            }
            if ( auto rT = fptr->result->findAlias(name,true) ) {
                return rT;
            }
            TypeDeclPtr rT;
            program->thisModule->globals.find_first([&](auto gvar){
                if ( auto vT = gvar->type->findAlias(name,false) ) {
                    rT = vT;
                    return true;
                }
                return false;
            });
            if ( rT ) return rT;
            TypeDeclPtr mtd = program->makeTypeDeclaration(LineInfo(),name);
            return mtd->isAlias() ? nullptr : mtd;
        }

        // find type alias name, and resolve it to type
        // within current context
        TypeDeclPtr findAlias ( const string & name ) const {
            if ( func ) {
                for ( auto it = local.rbegin(), its=local.rend(); it!=its; ++it ) {
                    auto & var = *it;
                    if ( auto vT = var->type->findAlias(name) ) {
                        return vT;
                    }
                }
                for ( auto & arg : func->arguments ) {
                    if ( auto aT = arg->type->findAlias(name) ) {
                        return aT;
                    }
                }
                if ( auto rT = func->result->findAlias(name,true) ) {
                    return rT;
                }
            }
            TypeDeclPtr rT;
            program->thisModule->globals.find_first([&](auto gvar){
                if ( auto vT = gvar->type->findAlias(name) ) {
                    rT = vT;
                    return true;
                }
                return false;
            });
            if ( rT ) return rT;
            TypeDeclPtr mtd = program->makeTypeDeclaration(LineInfo(),name);
            return (!mtd || mtd->isAlias()) ? nullptr : mtd;
        }

        // WARNING: this is really really slow, use faster tests when u can isAutoOrAlias for one
        // type chain is fully resolved, and not aliased \ auto
        bool isFullySealedType(const TypeDeclPtr & ptr, das_set<const TypeDecl *> & all ) {
            if (!ptr) return false;
            if ( ptr->baseType==Type::tStructure || ptr->baseType==Type::tTuple || ptr->baseType==Type::tVariant ) {
                auto thisType = ptr.get();
                if ( all.find(thisType)!=all.end() ) return true;
                all.insert(thisType);
            }
            if (ptr->baseType==Type::autoinfer || ptr->baseType==Type::alias) return false;
            if (ptr->firstType && !isFullySealedType(ptr->firstType,all)) return false;
            if (ptr->secondType && !isFullySealedType(ptr->secondType,all)) return false;
            for (auto & argT : ptr->argTypes) {
                if (argT && !isFullySealedType(argT,all)) return false;
            }
            if ( ptr->structType ) {
                for ( auto & fd : ptr->structType->fields )
                    if ( !isFullySealedType(fd.type,all) ) return false;
            }
            return true;
        }
        bool isFullySealedType(const TypeDeclPtr & ptr) {
            das_set<const TypeDecl *> all;
            return isFullySealedType(ptr, all);
        }
        // infer alias type
        TypeDeclPtr inferAlias ( const TypeDeclPtr & decl, const FunctionPtr & fptr = nullptr, AliasMap * aliases = nullptr, OptionsMap * options = nullptr ) const {
            if ( decl->baseType==Type::autoinfer ) {    // until alias is fully resolved, can't infer
                return nullptr;
            }
            if ( decl->baseType==Type::alias ) {
                if ( decl->isTag ) return nullptr;  // we can never infer a tag type
                auto aT = fptr ? findFuncAlias(fptr, decl->alias) : findAlias(decl->alias);
                if ( aliases ) {
                    if ( aT && aT->baseType==Type::autoinfer ) {
                        auto it = aliases->find(decl->alias);
                        if ( it != aliases->end() ) {
                            aT = it->second.get();
                        }
                    }
                }
                if ( !aT ) {
                    auto bT = nameToBasicType(decl->alias);
                    if ( bT != Type::none ) {
                        aT = make_smart<TypeDecl>(bT);
                    }
                }
                if ( aT ) {
                    auto resT = make_smart<TypeDecl>(*aT);
                    resT->at = decl->at;
                    resT->ref = (resT->ref || decl->ref) && !decl->removeRef;
                    resT->constant = (resT->constant || decl->constant) && !decl->removeConstant;
                    resT->temporary = (resT->temporary || decl->temporary) && !decl->removeTemporary;
                    resT->dim = decl->dim;
                    resT->alias.clear();
                    return resT;
                } else {
                    return nullptr;
                }
            }
            auto resT = make_smart<TypeDecl>(*decl);
            if ( decl->baseType==Type::tPointer ) {
                if ( decl->firstType ) {
                    resT->firstType = inferAlias(decl->firstType,fptr,aliases,options);
                    if ( !resT->firstType ) return nullptr;
                }
            } else if ( decl->baseType==Type::tIterator ) {
                if ( decl->firstType ) {
                    resT->firstType = inferAlias(decl->firstType,fptr,aliases,options);
                    if ( !resT->firstType ) return nullptr;
                }
            } else if ( decl->baseType==Type::tArray ) {
                if ( decl->firstType ) {
                    resT->firstType = inferAlias(decl->firstType,fptr,aliases,options);
                    if ( !resT->firstType ) return nullptr;
                }
            } else if ( decl->baseType==Type::tTable ) {
                if ( decl->firstType ) {
                    resT->firstType = inferAlias(decl->firstType,fptr,aliases,options);
                    if ( !resT->firstType ) return nullptr;
                }
                if ( decl->secondType ) {
                    resT->secondType = inferAlias(decl->secondType,fptr,aliases,options);
                    if ( !resT->secondType ) return nullptr;
                }
            } else if ( decl->baseType==Type::tFunction || decl->baseType==Type::tLambda || decl->baseType==Type::tBlock  ) {
                for ( size_t iA=0, iAs=decl->argTypes.size(); iA!=iAs; ++iA ) {
                    auto & declAT = decl->argTypes[iA];
                    if ( auto infAT = inferAlias(declAT,fptr,aliases,options) ) {
                        resT->argTypes[iA] = infAT;
                    } else {
                        return nullptr;
                    }
                }
                if ( !resT->firstType ) return nullptr;
                resT->firstType = inferAlias(decl->firstType,fptr,aliases,options);
                if ( !resT->firstType ) return nullptr;
            } else if ( decl->baseType==Type::tVariant || decl->baseType==Type::tTuple || decl->baseType==Type::option ) {
                for ( size_t iA=0, iAs=decl->argTypes.size(); iA!=iAs; ++iA ) {
                    auto & declAT = decl->argTypes[iA];
                    if ( auto infAT = inferAlias(declAT,fptr,aliases,options) ) {
                        resT->argTypes[iA] = infAT;
                    } else {
                        return nullptr;
                    }
                }
            }
            return resT;
        }

        string reportInferAliasErrors ( const TypeDeclPtr & decl ) const {
            if ( !verbose ) return "";
            TextWriter tw;
            reportInferAliasErrors(decl, tw);
            return tw.str();
        }

        void reportInferAliasErrors ( const TypeDeclPtr & decl, TextWriter & tw ) const {
            if ( decl->baseType==Type::autoinfer ) {    // until alias is fully resolved, can't infer
                tw << "\tcan't infer type for auto\n";
                return;
            }
            if ( decl->baseType==Type::alias ) {
                if ( decl->isTag ) {
                    tw << "\tcan't infer type for $t\n";
                    return;
                }
                auto aT = findAlias(decl->alias);
                if ( !aT ) {
                    auto bT = nameToBasicType(decl->alias);
                    if ( bT != Type::none ) {
                        aT = make_smart<TypeDecl>(bT);
                    }
                }
                if ( !aT ) {
                    tw << "\tdon't know what " << decl->alias << " is\n";
                    return;
                }
            }
            if ( decl->baseType==Type::tPointer ) {
                if ( decl->firstType ) {
                    reportInferAliasErrors(decl->firstType,tw);
                }
            } else if ( decl->baseType==Type::tIterator ) {
                if ( decl->firstType ) {
                    reportInferAliasErrors(decl->firstType,tw);
                }
            } else if ( decl->baseType==Type::tArray ) {
                if ( decl->firstType ) {
                    reportInferAliasErrors(decl->firstType,tw);
                }
            } else if ( decl->baseType==Type::tTable ) {
                if ( decl->firstType ) {
                    reportInferAliasErrors(decl->firstType,tw);
                }
                if ( decl->secondType ) {
                    reportInferAliasErrors(decl->secondType,tw);
                }
            } else if ( decl->baseType==Type::tFunction || decl->baseType==Type::tLambda || decl->baseType==Type::tBlock ||
                            decl->baseType==Type::tVariant || decl->baseType==Type::tTuple || decl->baseType==Type::option ) {
                for ( size_t iA=0, iAs=decl->argTypes.size(); iA!=iAs; ++iA ) {
                    auto & declAT = decl->argTypes[iA];
                    reportInferAliasErrors(declAT,tw);
                }
                if ( decl->baseType==Type::tFunction || decl->baseType==Type::tLambda || decl->baseType==Type::tBlock  ) {
                    if ( !decl->firstType ) {
                        tw << "\tcan't infer return type for " << das_to_string(decl->baseType) << "\n";
                        return;
                    }
                    reportInferAliasErrors(decl->firstType,tw);
                }
            }
        }

        // infer alias type
        TypeDeclPtr inferPartialAliases ( const TypeDeclPtr & decl, const FunctionPtr & fptr = nullptr, AliasMap * aliases = nullptr ) const {
            if ( decl->baseType==Type::autoinfer ) {
                return decl;
            }
            if ( decl->baseType==Type::alias ) {
                auto aT = fptr ? findFuncAlias(fptr, decl->alias) : findAlias(decl->alias);
                if ( aliases ) {
                    if ( aT && aT->baseType==Type::autoinfer ) {
                        auto it = aliases->find(decl->alias);
                        if ( it != aliases->end() ) {
                            aT = it->second.get();
                        }
                    }
                }
                if ( aT ) {
                    auto resT = make_smart<TypeDecl>(*aT);
                    resT->at = decl->at;
                    resT->ref = (resT->ref || decl->ref) && !decl->removeRef;
                    resT->constant = (resT->constant || decl->constant) && !decl->removeConstant;
                    resT->temporary = (resT->temporary || decl->temporary) && !decl->removeTemporary;
                    resT->implicit = (resT->implicit || decl->implicit);
                    resT->dim = decl->dim;
                    resT->alias.clear();
                    return resT;
                } else {
                    return decl;
                }
            }
            auto resT = make_smart<TypeDecl>(*decl);
            if ( decl->baseType==Type::tPointer ) {
                if ( decl->firstType ) {
                    resT->firstType = inferPartialAliases(decl->firstType,fptr,aliases);
                }
            } else if ( decl->baseType==Type::tIterator ) {
                if ( decl->firstType ) {
                    resT->firstType = inferPartialAliases(decl->firstType,fptr,aliases);
                }
            } else if ( decl->baseType==Type::tArray ) {
                if ( decl->firstType ) {
                    resT->firstType = inferPartialAliases(decl->firstType,fptr,aliases);
                }
            } else if ( decl->baseType==Type::tTable ) {
                if ( decl->firstType ) {
                    resT->firstType = inferPartialAliases(decl->firstType,fptr,aliases);
                }
                if ( decl->secondType ) {
                    resT->secondType = inferPartialAliases(decl->secondType,fptr,aliases);
                }
            } else if ( decl->baseType==Type::tFunction || decl->baseType==Type::tLambda
                || decl->baseType==Type::tBlock || decl->baseType==Type::tVariant ||
                    decl->baseType==Type::tTuple|| decl->baseType==Type::option ) {
                for ( size_t iA=0, iAs=decl->argTypes.size(); iA!=iAs; ++iA ) {
                    auto & declAT = decl->argTypes[iA];
                    resT->argTypes[iA] = inferPartialAliases(declAT,fptr,aliases);
                }
                if ( decl->firstType) {
                    resT->firstType = inferPartialAliases(decl->firstType,fptr,aliases);
                }
            }
            return resT;
        }


        Module * getSearchModule(string & moduleName) const {
            if ( moduleName=="_" ) {
                moduleName = "*";
                return program->thisModule.get();
            } else if ( moduleName=="__" ) {
                moduleName = program->thisModule->name;
                return program->thisModule.get();
            } else if ( func ) {
                if ( func->fromGeneric ) {
                    auto origin = func->getOrigin();
                    if ( moduleName.empty() ) {     // ::foo in generic means generic::goo, not this::foo
                        moduleName = origin->module->name;
                    }
                    return origin->module;
                } else {
                    return func->module;
                }
            } else {
                return program->thisModule.get();
            }
        }

        Module * getFunctionVisModule( Function * fn ) const {
            return fn->fromGeneric ? fn->getOrigin()->module : fn->module;
        }

        bool canCallPrivate ( const FunctionPtr & pFn, Module * mod, Module * thisMod ) const {
            if ( !pFn->privateFunction ) {
                return true;
            } else if ( pFn->module==mod || pFn->module==thisMod ) {
                return true;
            }if ( pFn->fromGeneric ) {
                auto origin = pFn->getOrigin();
                return (origin->module==mod) || (origin->module==thisMod);
            } else {
                return false;
            }
        }

        bool canAccessGlobal ( const VariablePtr & pVar, Module * mod, Module * thisMod ) const {
            if ( !pVar->private_variable ) {
                return true;
            } else if ( pVar->module==mod || pVar->module==thisMod ) {
                return true;
            } else {
                return false;
            }
        }

        // b <- a <- this
        //  a(x)    b
        //  this`a(x)   __::b
        //      inWhichModule = this
        //      objModule = _b
        bool isVisibleFunc ( Module * inWhichModule, Module * objModule ) const {
            if ( inWhichModule->isVisibleDirectly(objModule) ) return true;
            // can always call within same module from instanced generic
            if ( func && func->fromGeneric ) {
                auto inWhichOtherModule = func->getOrigin()->module;
                if ( inWhichOtherModule->isVisibleDirectly(objModule) ) return true;
            }
            return false;
        }

        MatchingFunctions findFuncAddr ( const string & name ) const {
            string moduleName, funcName;
            splitTypeName(name, moduleName, funcName);
            MatchingFunctions result;
            auto inWhichModule = getSearchModule(moduleName);
            auto hFuncName = hash64z(funcName.c_str());
            program->library.foreach([&](Module * mod) -> bool {
                auto itFnList = mod->functionsByName.find(hFuncName);
                if ( itFnList != mod->functionsByName.end() ) {
                    auto & goodFunctions = itFnList->second;
                    for ( auto & pFn : goodFunctions ) {
                        if ( isVisibleFunc(inWhichModule,getFunctionVisModule(pFn.get())) ) {
                            if ( canCallPrivate(pFn,inWhichModule,program->thisModule.get()) ) {
                                result.push_back(pFn.get());
                            }
                        }
                    }
                }
                return true;
            },moduleName);
            return result;
        }

        MatchingFunctions findTypedFuncAddr ( const string & name, const vector<TypeDeclPtr> & arguments ) const {
            return findMatchingFunctions(name, arguments, false, false);
        }

        MatchingFunctions findCandidates ( const string & name, const vector<TypeDeclPtr> & ) const {
            string moduleName, funcName;
            splitTypeName(name, moduleName, funcName);
            MatchingFunctions result;
            getSearchModule(moduleName);
            auto hFuncName = hash64z(funcName.c_str());
            program->library.foreach([&](Module * mod) -> bool {
                auto itFnList = mod->functionsByName.find(hFuncName);
                if ( itFnList != mod->functionsByName.end() ) {
                    auto & goodFunctions = itFnList->second;
                    result.reserve(result.size()+goodFunctions.size());
                    for ( auto & it : goodFunctions ) {
                        result.push_back(it.get());
                    }
                }
                return true;
            },moduleName);
            return result;
        }

        MatchingFunctions findCandidates ( const string & name, const vector<MakeFieldDeclPtr> & ) const {
            string moduleName, funcName;
            splitTypeName(name, moduleName, funcName);
            MatchingFunctions result;
            getSearchModule(moduleName);
            auto hFuncName = hash64z(funcName.c_str());
            program->library.foreach([&](Module * mod) -> bool {
                auto itFnList = mod->functionsByName.find(hFuncName);
                if ( itFnList != mod->functionsByName.end() ) {
                    auto & goodFunctions = itFnList->second;
                    result.reserve(result.size()+goodFunctions.size());
                    for ( auto & it : goodFunctions ) {
                        result.push_back(it.get());
                    }
                }
                return true;
            },moduleName);
            return result;
        }

        MatchingFunctions findGenericCandidates ( const string & name, const vector<MakeFieldDeclPtr> & ) const {
            // TODO: better error reporting
            string moduleName, funcName;
            splitTypeName(name, moduleName, funcName);
            MatchingFunctions result;
            getSearchModule(moduleName);
            auto hFuncName = hash64z(funcName.c_str());
            program->library.foreach([&](Module * mod) -> bool {
                auto itFnList = mod->genericsByName.find(hFuncName);
                if ( itFnList != mod->genericsByName.end() ) {
                    auto & goodFunctions = itFnList->second;
                    result.reserve(result.size()+goodFunctions.size());
                    for ( auto & it : goodFunctions ) {
                        result.push_back(it.get());
                    }
                }
                return true;
            },moduleName);
            return result;
        }

        MatchingFunctions findGenericCandidates ( const string & name, const vector<TypeDeclPtr> & ) const {
            string moduleName, funcName;
            splitTypeName(name, moduleName, funcName);
            MatchingFunctions result;
            getSearchModule(moduleName);
            auto hFuncName = hash64z(funcName.c_str());
            program->library.foreach([&](Module * mod) -> bool {
                auto itFnList = mod->genericsByName.find(hFuncName);
                if ( itFnList != mod->genericsByName.end() ) {
                    auto & goodFunctions = itFnList->second;
                    result.reserve(result.size()+goodFunctions.size());
                    for ( auto & it : goodFunctions ) {
                        result.push_back(it.get());
                    }
                }
                return true;
            },moduleName);
            return result;
        }

        bool isMatchingArgument(const FunctionPtr & pFn, TypeDeclPtr argType, TypeDeclPtr passType, bool inferAuto, bool inferBlock, AliasMap * aliases = nullptr, OptionsMap * options = nullptr ) const {
            if (!passType) {
                return false;
            }
            if ( argType->explicitConst && (argType->constant != passType->constant) ) {    // explicit const mast match
                return false;
            }
            if ( argType->explicitRef && (argType->ref != passType->ref) ) {                // explicit ref match
                return false;
            }
            if ( argType->baseType==Type::anyArgument ) {
                return true;
            }
            if (inferAuto) {
                // if it's an alias, we de'alias it, and see if it matches at all
                if (argType->isAlias()) {
                    argType = inferAlias(argType, pFn, aliases);
                    if ( !argType ) {
                        return false;
                    }
                }
                // match auto argument
                if (argType->isAuto()) {
                    return TypeDecl::inferGenericType(argType, passType, true, true, options) != nullptr;
                }
            }
            // match inferable block
            if (inferBlock && passType->isAuto() && (passType->isGoodBlockType() || passType->isGoodLambdaType() || passType->isGoodFunctionType())) {
                return TypeDecl::inferGenericType(passType, argType, true, true, options) != nullptr;
            }
            // compare types which don't need inference
            auto tempMatters = argType->implicit ? TemporaryMatters::no : TemporaryMatters::yes;
            if ( !argType->isSameType(*passType, RefMatters::no, ConstMatters::no, tempMatters, AllowSubstitute::yes, true, true) ) {
                return false;
            }
            // can't pass non-ref to ref
            if ( argType->isRef() && !passType->isRef() ) {
                return false;
            }
            // ref types can only add constness
            if (argType->isRef() && !argType->constant && passType->constant) {
                return false;
            }
            // pointer types can only add constant
            if (argType->isPointer() && !argType->constant && passType->constant) {
                return false;
            }
            // all good
            return true;
        }

        bool isFunctionCompatible ( const FunctionPtr & pFn, const vector<TypeDeclPtr> & types, bool inferAuto, bool inferBlock, bool checkLastArgumentsInit = true ) const {
            if ( pFn->arguments.size() < types.size() ) {
                return false;
            }
            if ( inferAuto && inferBlock ) {
                AliasMap aliases;
                for ( ;; ) {
                    bool anyFailed = false;
                    auto totalAliases = aliases.size();
                    for ( size_t ai=0, ais=types.size(); ai!=ais; ++ai ) {
                        auto argType = pFn->arguments[ai]->type;
                        if ( argType->isAlias() ) {
                            argType = inferPartialAliases(argType, pFn, &aliases);
                        }
                        auto passType = types[ai];
                        OptionsMap options;
                        if (!isMatchingArgument(pFn,argType,passType,inferAuto,inferBlock,&aliases,&options)) {
                            anyFailed = true;
                            continue;
                        }
                        TypeDecl::updateAliasMap(argType, passType, aliases, options);
                    }
                    if ( !anyFailed ) {
                        break;
                    }
                    if ( totalAliases == aliases.size() ) {
                        return false;
                    }
                }
            } else {
                for ( size_t ai=0, ais=types.size(); ai!=ais; ++ai ) {
                    if (!isMatchingArgument(pFn, pFn->arguments[ai]->type, types[ai],inferAuto,inferBlock)) {
                        return false;
                    }
                }
            }
            if (checkLastArgumentsInit) {
                for (auto ti = types.size(), tis = pFn->arguments.size(); ti != tis; ++ti) {
                    if (!pFn->arguments[ti]->init) {
                        return false;
                    }
                }
            }
            for ( const auto & ann : pFn->annotations ) {
                auto fnAnn = static_pointer_cast<FunctionAnnotation>(ann->annotation);
                if ( fnAnn->isSpecialized() ) {
                    string err;
                    if ( !fnAnn->isCompatible(pFn, types, *ann, err) ) {
                        return false;
                    }
                }
            }
            return true;
        }

        bool isFunctionCompatible ( const FunctionPtr & pFn, const vector<TypeDeclPtr>& nonNamedTypes, const vector<MakeFieldDeclPtr> & arguments, bool inferAuto, bool inferBlock ) const {
            if ( ( pFn->arguments.size() < arguments.size() ) || ( pFn->arguments.size() < nonNamedTypes.size() ) ) {
                return false;
            }

            if ( !isFunctionCompatible(pFn, nonNamedTypes, inferAuto, inferBlock, false) ) {
                return false;
            }


            size_t fnArgIndex = 0;
            for ( size_t ai=0, ais=arguments.size(); ai!=ais; ++ai ) {
                auto & arg = arguments[ai];
                for ( ;; ) {
                    if ( fnArgIndex >= pFn->arguments.size() ) {    // out of source arguments. done
                        return false;
                    }
                    auto & fnArg = pFn->arguments[fnArgIndex];
                    if ( fnArg->name == arg->name ) {               // found it, name matches
                        break;
                    }

                    if ( !fnArg->init && fnArgIndex >= nonNamedTypes.size() ) {    // can't skip - no default value
                        return false;
                    }
                    fnArgIndex ++;
                }
                if (!isMatchingArgument(pFn, pFn->arguments[fnArgIndex]->type, arg->value->type,inferAuto,inferBlock)) {
                    return false;
                }
                fnArgIndex ++;
            }
            while ( fnArgIndex < pFn->arguments.size() ) {
                if ( !pFn->arguments[fnArgIndex]->init ) {
                    return false;                                   // tail must be defaults
                }
                fnArgIndex ++;
            }
            return true;
        }

        string reportAliasError( const TypeDeclPtr & type ) const {
            vector<string> aliases;
            type->collectAliasList(aliases);
            TextWriter ss;
            ss << "don't know what " << describeType(type) << " is";
            for (auto & aa : aliases) {
                ss << "; unknown type " << aa;
            }
            return ss.str();
        }

        string describeMismatchingArgument( const string & argName, const TypeDeclPtr & passType, const TypeDeclPtr & argType, int argIndex ) const {
            TextWriter ss;
            ss << "\t\tinvalid argument " << argName << " (" << argIndex << "). expecting "
                << describeType(argType) << ", passing " << describeType(passType) << "\n";
            if (passType->isAlias()) {
                ss << "\t\t" << reportAliasError(passType) << "\n";
            }
            if ( argType->isRef() && !passType->isRef() ) {
                ss << "\t\tcan't pass non-ref to ref\n";
            }
            if ( argType->isRef() && !argType->constant && passType->constant ) {
                ss << "\t\tcan't ref types can only add constness\n";
            }
            if ( argType->isPointer() && !argType->constant && passType->constant) {
                ss << "\t\tpointer types can only add constness\n";
            }
            return ss.str();
        }

        int getMismatchingFunctionRank(const FunctionPtr & pFn, const vector<TypeDeclPtr>& nonNamedTypes, const vector<MakeFieldDeclPtr> & arguments, bool inferAuto, bool inferBlock) const {
            int rank = 0;
            size_t fnArgIndex = 0;
            for ( size_t ai=0, ais=arguments.size(); ai!=ais; ++ai ) {
                auto & arg = arguments[ai];
                for ( ;; ) {
                    if ( fnArgIndex >= pFn->arguments.size() ) {
                        return rank + 1000;
                    }
                    auto & fnArg = pFn->arguments[fnArgIndex];
                    if ( fnArg->name == arg->name ) {
                        break;
                    }
                    if ( !fnArg->init && fnArgIndex >= nonNamedTypes.size()) {
                        return rank + 1000;
                    }
                    fnArgIndex ++;
                }
                auto & passType = arg->value->type;
                auto & argType = pFn->arguments[fnArgIndex]->type;
                if (!isMatchingArgument(pFn, argType, passType,inferAuto,inferBlock)) {
                    rank += 1000;
                }
                fnArgIndex ++;
            }
            while ( fnArgIndex < pFn->arguments.size() ) {
                if ( !pFn->arguments[fnArgIndex]->init ) {
                    rank += 1000;
                }
                fnArgIndex ++;
            }
            return rank;
        }

        int getMismatchingFunctionRank(const FunctionPtr & pFn, const vector<TypeDeclPtr> & types, bool inferAuto, bool inferBlock) const {
            TextWriter ss;
            size_t tot = das::min ( types.size(), pFn->arguments.size() );
            size_t ai;
            int rank = 0;
            for (ai = 0; ai != tot; ++ai) {
                auto & arg = pFn->arguments[ai];
                auto & passType = types[ai];
                if (!isMatchingArgument(pFn, arg->type, passType, inferAuto, inferBlock)) {
                    rank += 1000;
                }
            }
            rank += int(pFn->arguments.size() - types.size()) * 1000;
            for ( const auto & ann : pFn->annotations ) {
                auto fnAnn = static_pointer_cast<FunctionAnnotation>(ann->annotation);
                if ( fnAnn->isSpecialized() ) {
                    string err;
                    if ( !fnAnn->isCompatible(pFn, types, *ann, err) ) {
                        rank += 1000;
                    }
                }
            }
            return rank;
        }

        string describeMismatchingFunction(const FunctionPtr & pFn, const vector<TypeDeclPtr>& nonNamedTypes, const vector<MakeFieldDeclPtr> & arguments, bool inferAuto, bool inferBlock) const {
            if ( pFn->arguments.size() < arguments.size() ) {
                return "\t\ttoo many arguments\n";
            }
            TextWriter ss;
            size_t fnArgIndex = 0;
            for ( size_t ai=0, ais=arguments.size(); ai!=ais; ++ai ) {
                auto & arg = arguments[ai];
                for ( ;; ) {
                    if ( fnArgIndex >= pFn->arguments.size() ) {
                        auto it = find_if ( pFn->arguments.begin(), pFn->arguments.end(), [&]( const VariablePtr & varg){
                            return varg->name == arg->name;
                        });
                        if ( it != pFn->arguments.end() ) {
                            ss << "\t\tcan't match argument " << arg->name << ", its submitted out of order\n";
                        } else {
                            ss << "\t\tcan't match argument " << arg->name << ", out of function arguments\n";
                        }
                        return ss.str();
                    }
                    auto & fnArg = pFn->arguments[fnArgIndex];
                    if ( fnArg->name == arg->name ) {
                        break;
                    }
                    if ( !fnArg->init && fnArgIndex >= nonNamedTypes.size()) {
                        ss << "\t\twhile looking for argument " << arg->name
                            << ", can't skip function argument " << fnArg->name << " because it has no default value\n";
                        return ss.str();
                    }
                    fnArgIndex ++;
                }
                auto & passType = arg->value->type;
                auto & argType = pFn->arguments[fnArgIndex]->type;
                if (!isMatchingArgument(pFn, argType, passType,inferAuto,inferBlock)) {
                    ss << describeMismatchingArgument(arg->name, passType, argType, (int)ai);
                }
                fnArgIndex ++;
            }
            while ( fnArgIndex < pFn->arguments.size() ) {
                if ( !pFn->arguments[fnArgIndex]->init ) {
                    ss << "\t\tmissing default value for function argument " << pFn->arguments[fnArgIndex]->name << "\n";
                    return ss.str();
                }
                fnArgIndex ++;
            }
            return ss.str();
        }

        string describeMismatchingFunction(const FunctionPtr & pFn, const vector<TypeDeclPtr> & types, bool inferAuto, bool inferBlock) const {
            TextWriter ss;
            size_t tot = das::min ( types.size(), pFn->arguments.size() );
            size_t ai;
            for (ai = 0; ai != tot; ++ai) {
                auto & arg = pFn->arguments[ai];
                auto & passType = types[ai];
                if (!isMatchingArgument(pFn, arg->type, passType, inferAuto, inferBlock)) {
                    ss << describeMismatchingArgument(arg->name, passType, arg->type, (int)ai);
                }
            }
            for ( size_t ais=pFn->arguments.size(); ai!=ais; ++ai ) {
                auto & arg = pFn->arguments[ai];
                if ( !arg->init ) {
                    ss << "\t\tmissing argument " << arg->name << "\n";
                }
            }
            for ( const auto & ann : pFn->annotations ) {
                auto fnAnn = static_pointer_cast<FunctionAnnotation>(ann->annotation);
                if ( fnAnn->isSpecialized() ) {
                    string err;
                    if ( !fnAnn->isCompatible(pFn, types, *ann, err) ) {
                        ss << "\t\t" << err << "\n";
                    }
                }
            }
            return ss.str();
        }

        MatchingFunctions findMatchingFunctions ( const string & name, const vector<TypeDeclPtr>& types, const vector<MakeFieldDeclPtr> & arguments, bool inferBlock = false ) const {
            string moduleName, funcName;
            splitTypeName(name, moduleName, funcName);
            MatchingFunctions result;
            auto inWhichModule = getSearchModule(moduleName);
            auto thisModule = program->thisModule.get();
            auto hFuncName = hash64z(funcName.c_str());
            program->library.foreach([&](Module * mod) -> bool {
                auto itFnList = mod->functionsByName.find(hFuncName);
                if ( itFnList != mod->functionsByName.end() ) {
                    auto & goodFunctions = itFnList->second;
                    for ( auto & pFn : goodFunctions ) {
                        if ( isVisibleFunc(inWhichModule,getFunctionVisModule(pFn.get())) ) {
                            if ( !pFn->fromGeneric || thisModule->isVisibleDirectly(mod) ) {
                                if ( canCallPrivate(pFn,inWhichModule,thisModule) ) {
                                    if ( isFunctionCompatible(pFn, types, arguments, false, inferBlock) ) {
                                        result.push_back(pFn.get());
                                    }
                                }
                            }
                        }
                    }
                }
                return true;
            },moduleName);
            return result;
        }

        MatchingFunctions findMatchingFunctions ( const string & name, const vector<TypeDeclPtr> & types, bool inferBlock = false, bool visCheck = true ) const {
            string moduleName, funcName;
            splitTypeName(name, moduleName, funcName);
            MatchingFunctions result;
            auto inWhichModule = getSearchModule(moduleName);
            auto thisModule = program->thisModule.get();
            auto hFuncName = hash64z(funcName.c_str());
            program->library.foreach([&](Module * mod) -> bool {
                auto itFnList = mod->functionsByName.find(hFuncName);
                if ( itFnList != mod->functionsByName.end() ) {
                    auto & goodFunctions = itFnList->second;
                    for ( auto & pFn : goodFunctions ) {
                        if ( pFn->jitOnly && !program->policies.jit ) continue;
                        if ( !visCheck || isVisibleFunc(inWhichModule,getFunctionVisModule(pFn.get()) ) ) {
                            if ( !pFn->fromGeneric || thisModule->isVisibleDirectly(mod) ) {
                                if ( canCallPrivate(pFn,inWhichModule,thisModule) ) {
                                    if ( isFunctionCompatible(pFn, types, false, inferBlock) ) {
                                        result.push_back(pFn.get());
                                    }
                                }
                            }
                        }
                    }
                }
                return true;
            },moduleName);
            return result;
        }

        MatchingFunctions findMatchingGenerics ( const string & name, const vector<TypeDeclPtr>& types, const vector<MakeFieldDeclPtr> & arguments ) const {
            string moduleName, funcName;
            splitTypeName(name, moduleName, funcName);
            MatchingFunctions result;
            auto inWhichModule = getSearchModule(moduleName);
            auto thisModule = program->thisModule.get();
            auto hFuncName = hash64z(funcName.c_str());
            program->library.foreach([&](Module * mod) -> bool {
                auto itFnList = mod->genericsByName.find(hFuncName);
                if ( itFnList != mod->genericsByName.end() ) {
                    auto & goodFunctions = itFnList->second;
                    for ( auto & pFn : goodFunctions ) {
                        if ( pFn->jitOnly && !program->policies.jit ) continue;
                        if ( isVisibleFunc(inWhichModule,getFunctionVisModule(pFn.get())) ) {
                            if ( canCallPrivate(pFn,inWhichModule,thisModule) ) {
                                if ( isFunctionCompatible(pFn, types, arguments, true, true) ) {   // infer block here?
                                    result.push_back(pFn.get());
                                }
                            }
                        }
                    }
                }
                return true;
            },moduleName);
            return result;
        }

        MatchingFunctions findMatchingGenerics ( const string & name, const vector<TypeDeclPtr> & types ) const {
            string moduleName, funcName;
            splitTypeName(name, moduleName, funcName);
            MatchingFunctions result;
            auto inWhichModule = getSearchModule(moduleName);
            auto thisModule = program->thisModule.get();
            auto hFuncName = hash64z(funcName.c_str());
            program->library.foreach([&](Module * mod) -> bool {
                auto itFnList = mod->genericsByName.find(hFuncName);
                if ( itFnList != mod->genericsByName.end() ) {
                    auto & goodFunctions = itFnList->second;
                    for ( auto & pFn : goodFunctions ) {
                        if ( isVisibleFunc(inWhichModule,getFunctionVisModule(pFn.get())) ) {
                            if ( canCallPrivate(pFn,inWhichModule,thisModule) ) {
                                if ( isFunctionCompatible(pFn, types, true, true) ) {   // infer block here?
                                    result.push_back(pFn.get());
                                }
                            }
                        }
                    }
                }
                return true;
            },moduleName);
            return result;
        }

        void reportFunctionNotFound( const string & name, const string & extra,
                                    const LineInfo & at, const MatchingFunctions & candidateFunctions,
            const vector<TypeDeclPtr> & types, bool inferAuto, bool inferBlocks, bool reportDetails,
                                    CompilationError cerror, int nExtra ) {
            if ( verbose ) {
                TextWriter ss;
                if ( candidateFunctions.size()==0 ) {
                    ss << "there are no good matching candidates out of " << nExtra << " total functions with the same name\n";
                } else if ( candidateFunctions.size() > 1 ) {
                    ss << "candidates:\n";
                } else if ( candidateFunctions.size()==1 ) {
                    ss << (nExtra ? "\nmost likely candidate:\n" : "\ncandidate function:\n");
                }
                string moduleName, funcName;
                splitTypeName(name, moduleName, funcName);
                auto inWhichModule = getSearchModule(moduleName);
                for ( auto & missFn : candidateFunctions ) {
                    ss << "\t";
                    if ( missFn->module && !missFn->module->name.empty() && !(missFn->module->name=="$") )
                        ss << missFn->module->name << "::";
                    ss << describeFunction(missFn);
                    if ( missFn->builtIn ) {
                        ss << " // builtin";
                    } else {
                        ss << " at " << missFn->at.describe();
                    }
                    ss << "\n";
                    if ( reportDetails ) {
                        ss << describeMismatchingFunction(missFn, types, inferAuto, inferBlocks);
                    }
                    auto visM = getFunctionVisModule(missFn);
                    if ( !isVisibleFunc(inWhichModule,visM) ) {
                        ss << "\t\tmodule " << visM->name << " is not visible directly from ";
                        if ( inWhichModule->name.empty()) {
                            ss << "the current module\n";
                        } else {
                            ss << inWhichModule->name << "\n";
                        }
                    }
                    if ( missFn->privateFunction && !canCallPrivate(missFn,inWhichModule,program->thisModule.get()) ) {
                        ss << "\t\tfunction is private";
                        if ( missFn->module && !missFn->module->name.empty() ) {
                            ss << " to module " << missFn->module->name;
                        }
                        ss << "\n";
                    }
                }
                if ( nExtra>0 && candidateFunctions.size()!=0 ) {
                    ss << "also " << nExtra << " more candidates\n";
                }
                error(extra, ss.str(), "", at, cerror);
            } else {
                error(extra, "", "", at, cerror);
            }
        }

        void reportFunctionNotFound( const string & name, const string & extra,
                                    const LineInfo & at, const MatchingFunctions & candidateFunctions,
                                    const vector<TypeDeclPtr>& nonNamedTypes, const vector<MakeFieldDeclPtr> & arguments,
                                    bool inferAuto, bool inferBlocks, bool reportDetails ,
                                    CompilationError cerror, int nExtra) {
            if ( verbose ) {
                TextWriter ss;
                if ( candidateFunctions.size()==0 ) {
                    ss << "there are no good matching candidates out of " << nExtra << " total functions with the same name\n";
                } else if ( candidateFunctions.size() > 1 ) {
                    ss << "candidates:\n";
                } else if ( candidateFunctions.size()==1 ) {
                    ss << (nExtra ? "\nmost likely candidate:\n" : "\ncandidate function:\n");
                }
                string moduleName, funcName;
                splitTypeName(name, moduleName, funcName);
                auto inWhichModule = getSearchModule(moduleName);
                for ( auto & missFn : candidateFunctions ) {
                    ss << "\t";
                    if ( missFn->module && !missFn->module->name.empty() && !(missFn->module->name=="$") )
                        ss << missFn->module->name << "::";
                    ss << describeFunction(missFn);
                    if ( missFn->builtIn ) {
                        ss << " // builtin";
                    } else if ( missFn->at.line ) {
                        ss << " at " << missFn->at.describe();
                    }
                    ss << "\n";
                    if ( reportDetails ) {
                        ss << describeMismatchingFunction(missFn, nonNamedTypes, arguments, inferAuto, inferBlocks);
                    }
                    auto visM = getFunctionVisModule(missFn);
                    if ( !isVisibleFunc(inWhichModule,visM) ) {
                        ss << "\t\tmodule " << visM->name << " is not visible directly from ";
                        if ( inWhichModule->name.empty()) {
                            ss << "the current module\n";
                        } else {
                            ss << inWhichModule->name << "\n";
                        }
                    }
                    if ( missFn->privateFunction && !canCallPrivate(missFn,inWhichModule,program->thisModule.get()) ) {
                        ss << "\t\tfunction is private";
                        if ( missFn->module && !missFn->module->name.empty() ) {
                            ss << " to module " << missFn->module->name;
                        }
                        ss << "\n";
                    }
                    if ( nExtra>0 && candidateFunctions.size()!=0 ) {
                        ss << "also " << nExtra << " more candidates\n";
                    }
                }
                error(extra, ss.str(), "", at, cerror);
            } else {
                error(extra, "", "", at, cerror);
            }
        }

        bool hasUserConstructor ( const string & sna ) const {
            vector<TypeDeclPtr> argDummy;
            auto fnlist = findMatchingFunctions(sna, argDummy);
            return fnlist.size() != 0;  // at least one. if more its an error, but it has one for sure
        }

        bool verifyAnyFunc ( const MatchingFunctions & fnList, const LineInfo & at ) const {
            int genCount = 0;
            int customCount = 0;
            for ( auto & fn : fnList ) {
                if ( !fn->generated ) {
                    customCount ++;
                } else {
                    genCount ++;
                }
            }
            if ( customCount && genCount ) {
                if ( verbose ) {
                    string candidates = program->describeCandidates(fnList);
                    error("both generated and custom " + fnList.front()->name + " functions exist for " + describeFunction(fnList.front()), candidates, "",
                        at, CompilationError::function_not_found);
                } else {
                    error("both generated and custom " + fnList.front()->name + " functions exist", "", "",
                        at, CompilationError::function_not_found);

                }
                return false;
            } else if ( customCount>1 ) {
                if ( verbose ) {
                    string candidates = program->describeCandidates(fnList);
                    error("too many custom  " + fnList.front()->name + " functions exist for " + describeFunction(fnList.front()), candidates, "",
                        at,CompilationError::function_not_found);
                } else {
                    error("too many custom  " + fnList.front()->name + " functions exist", "", "",
                        at,CompilationError::function_not_found);
                }
                return false;
            } else {
                return true;
            }
        }

        MatchingFunctions getCloneFunc ( const TypeDeclPtr & left, const TypeDeclPtr & right ) const {
            vector<TypeDeclPtr> argDummy = { left, right };
            auto clones = findMatchingFunctions("_::clone", argDummy);
            applyLSP(argDummy, clones);
            return clones;
        }

        bool verifyCloneFunc ( const MatchingFunctions & fnList, const LineInfo & at ) const {
            return verifyAnyFunc(fnList, at);
        }

        MatchingFunctions getFinalizeFunc ( const TypeDeclPtr & subexpr ) const {
            vector<TypeDeclPtr> argDummy = { subexpr };
            auto fins = findMatchingFunctions("_::finalize", argDummy);
            applyLSP(argDummy, fins);
            return fins;
        }

        bool verifyFinalizeFunc ( const MatchingFunctions & fnList, const LineInfo & at ) const {
            return verifyAnyFunc(fnList, at);
        }

        ExprWith * hasMatchingWith ( const string & fieldName ) const {
            for ( auto it=with.rbegin(), its=with.rend(); it!=its; ++it ) {
                auto eW = *it;
                if ( auto eWT = eW->with->type ) {
                    StructurePtr pSt;
                    if ( eWT->isStructure() ) {
                        pSt = eWT->structType;
                    } else if ( eWT->isPointer() && eWT->firstType && eWT->firstType->isStructure() ) {
                        pSt = eWT->firstType->structType;
                    }
                    if ( pSt ) {
                        if ( pSt->fieldLookup.find(fieldName) != pSt->fieldLookup.end() ) {
                            return eW;
                        }
                    }
                }
            }
            return nullptr;
        }

        bool inferTypeExpr ( TypeDecl * type ) {
            bool any = false;
            for ( size_t i=0, is=type->dim.size(); i!=is; ++i ) {
                if ( type->dim[i]==TypeDecl::dimConst ) {
                    if ( type->dimExpr[i] ) {
                        if ( auto constExpr = getConstExpr(type->dimExpr[i].get()) ) {
                            if ( constExpr->type->isIndex() ) {
                                auto cI = static_pointer_cast<ExprConstInt>(constExpr);
                                auto dI = cI->getValue();
                                if ( dI>0) {
                                    type->dim[i] = dI;
                                    any = true;
                                } else {
                                    error("array dimension can't be 0 or less",  "", "",
                                        type->at, CompilationError::invalid_array_dimension);
                                }
                            } else {
                                error("array dimension must be int32 or uint32",  "", "",
                                    type->at, CompilationError::invalid_array_dimension);
                            }
                        } else {
                            error("array dimension must be constant",  "", "",
                                type->at, CompilationError::invalid_array_dimension);
                        }
                    } else {
                        error("can't deduce array dimension",  "", "",
                            type->at, CompilationError::invalid_array_dimension);
                    }
                }
            }
            if ( type->firstType ) {
                any |= inferTypeExpr(type->firstType.get());
            }
            if ( type->secondType ) {
                any |= inferTypeExpr(type->secondType.get());
            }
            for ( auto & argType : type->argTypes ) {
                any |= inferTypeExpr(argType.get());
            }
            return any;
        }

    protected:

    // type
        virtual void preVisit ( TypeDecl * type ) override {
            if ( inferTypeExpr(type) ) {
                reportAstChanged();
            }
#if 0
            // note: this code here is for debugging mangled names facilities
            //  do not delete, until those are stabilized
            auto mname = type->getMangledName(true);
            if ( mname=="CN<;f>0<i;f>U" ) {
                mname = type->getMangledName(true);
            }
            MangledNameParser parser;
            const char * mnamec = mname.c_str();
            auto mtype = parser.parseTypeFromMangledName ( mnamec, program->library, program->thisModule.get() );
            if ( !type->isSameExactType(*mtype) ) {
                LOG(LogLevel::error) tp;
                auto mtname = mtype->getMangledName(true);
                tp << "Not exactly the same type '" << mname << "` vs `" << mtname << "`\n";
                if ( mname==mtname ) tp << "however strings match\n";
                DAS_ASSERT(type->isSameExactType(*mtype) && "type validation failed");
            }
#endif
        }

        virtual  TypeDeclPtr visitAlias ( TypeDecl * td, const string & ) override {
            if ( td->isAlias() ) {
                if ( auto ta = inferAlias(td) ) {
                    if ( ta->isAutoOrAlias() ) {
                        error("internal error. can't be inferred " + describeType(td),  "", "",
                            td->at, CompilationError::invalid_type);
                        return td;
                    }
                    return ta;
                } else {
                    ta = inferAlias(td);
                    error("can't be inferred " + describeType(td),  reportInferAliasErrors(td), "",
                        td->at, CompilationError::invalid_type);
                }
            }
            return td;
        }

    // enumeration

        ExpressionPtr makeEnumConstValue(Enumeration * enu, int64_t nextInt) const {
            vec4f nextV = v_zero();
            switch (enu->baseType) {
            case Type::tInt8:       nextV = cast<int8_t>  ::from(int8_t  (nextInt)); break;
            case Type::tUInt8:      nextV = cast<uint8_t> ::from(uint8_t (nextInt)); break;
            case Type::tInt16:      nextV = cast<int16_t> ::from(int16_t (nextInt)); break;
            case Type::tUInt16:     nextV = cast<uint16_t>::from(uint16_t(nextInt)); break;
            case Type::tInt:        nextV = cast<int32_t> ::from(int32_t (nextInt)); break;
            case Type::tUInt:       nextV = cast<uint32_t>::from(uint32_t(nextInt)); break;
            case Type::tBitfield:   nextV = cast<uint32_t>::from(uint32_t(nextInt)); break;
            case Type::tInt64:      nextV = cast<int64_t> ::from(int64_t (nextInt)); break;
            case Type::tUInt64:     nextV = cast<uint64_t>::from(uint64_t(nextInt)); break;
            default: DAS_ASSERTF(0,"we should not be here. unsupported enum type");
            }
            auto nextValue = Program::makeConst(enu->at, enu->makeBaseType(), nextV);
            nextValue->type = enu->makeBaseType();
            return nextValue;
        }

        virtual ExpressionPtr visitEnumerationValue ( Enumeration * enu, const string & name, Expression * value, bool last ) override {
            if ( !value ) {
                if ( lastEnuValue ) {
                    if ( lastEnuValue->rtti_isConstant() && lastEnuValue->type && lastEnuValue->type->isInteger() ) {
                        reportAstChanged();
                        int64_t nextInt = getConstExprIntOrUInt(lastEnuValue) + 1;
                        auto nextValue = makeEnumConstValue(enu, nextInt);
                        lastEnuValue = nextValue.get();
                        return nextValue;
                    } else {
                        error("enumeration value " + name + " can't be inferred yet",  "", "",
                            enu->at);
                    }
                } else {
                    reportAstChanged();
                    auto zeroValue = Program::makeConst(enu->at, enu->makeBaseType(), v_zero());
                    zeroValue->type = enu->makeBaseType();
                    lastEnuValue = zeroValue.get();
                    return zeroValue;
                }
            } else {
                if ( !value->type ) {
                    error("enumeration value " + name + " can't be inferred yet",  "", "",
                        value->at);
                } else if ( !value->type || !value->type->isInteger() ) {
                    error("enumeration value " + name + " has to be signed or unsigned integer of any size, and not " + value->type->describe(), "", "",
                          value->at, CompilationError::invalid_enumeration);
                } else if ( !value->rtti_isConstant() ) {
                    if ( auto fenum = getConstExpr(value) ) {
                        reportAstChanged();
                        return fenum;
                    } else {
                        error("enumeration value " + name + " must be integer constant, and not an expression", "", "",
                            value->at, CompilationError::invalid_enumeration);
                    }
                } else if (value->type->baseType != enu->baseType) {
                    reportAstChanged();
                    int64_t thisInt = getConstExprIntOrUInt(value);
                    auto thisValue = makeEnumConstValue(enu, thisInt);
                    lastEnuValue = thisValue.get();
                    return thisValue;
                }
            }
            lastEnuValue = value;
            return Visitor::visitEnumerationValue(enu, name, value, last);
        }

        virtual void preVisit ( Enumeration * enu ) override {
            Visitor::preVisit(enu);
            lastEnuValue = nullptr;
        }

        virtual EnumerationPtr visit ( Enumeration * enu ) override {
            lastEnuValue = nullptr;
            return Visitor::visit(enu);
        }

    // strcuture
        virtual void preVisit ( Structure * that ) override {
            Visitor::preVisit(that);
            fieldOffset = 0;
            cppLayout = that->cppLayout;
            cppLayoutPod = !that->cppLayoutNotPod;
            cppLayoutParent = nullptr;
            that->fieldLookup.clear();
            fieldIndex = 0;
        }
        virtual void preVisitStructureField ( Structure * that, Structure::FieldDeclaration & decl, bool last ) override {
            Visitor::preVisitStructureField(that, decl, last);
            that->fieldLookup[decl.name] = fieldIndex++;
            if ( decl.type->isAuto() && !decl.init) {
                error("structure field type can't be inferred, it needs an initializer", "", "",
                      decl.at, CompilationError::cant_infer_missing_initializer );
            }
        }
        virtual void visitStructureField ( Structure * st, Structure::FieldDeclaration & decl, bool ) override {
            if ( decl.type && decl.type->isExprType() ) {
                return;
            }
            if ( decl.parentType ) {
                auto pf = st->parent->findField(decl.name);
                if ( !pf->type->isAutoOrAlias() ) {
                    decl.type = make_smart<TypeDecl>(*pf->type);
                    decl.parentType = false;
                    decl.type->sanitize();
                    reportAstChanged();
                } else {
                    error("not fully resolved yet",  "", "", decl.at);
                    return;
                }
            }
            if ( decl.type->isAlias() ) {
                if ( auto aT = inferAlias(decl.type) ) {
                    decl.type = aT;
                    decl.type->sanitize();
                    reportAstChanged();
                } else {
                    error("undefined structure field type " + describeType(decl.type),
                        reportInferAliasErrors(decl.type), "", decl.at, CompilationError::invalid_structure_field_type );
                }
            }
            if ( decl.type->isAuto() && decl.init && decl.init->type ) {
                auto varT = TypeDecl::inferGenericType(decl.type, decl.init->type, false, false, nullptr);
                if ( !varT ) {
                    error("structure field initialization type can't be inferred, "
                          + describeType(decl.type) + " = " + describeType(decl.init->type), "", "",
                          decl.at, CompilationError::invalid_structure_field_type );
                } else {
                    TypeDecl::applyAutoContracts(varT, decl.type);
                    decl.type = varT;
                    decl.type->ref = false;
                    decl.type->sanitize();
                    reportAstChanged();
                }
            }
            if ( isCircularType(decl.type) ) {
                return;
            }
            if ( decl.type->isVoid() ) {
                error("structure field type can't be declared void", "", "",
                    decl.at,CompilationError::invalid_structure_field_type);
            } else if ( decl.type->ref ) {
                error("structure field type can't be declared a reference", "", "",
                    decl.at,CompilationError::invalid_structure_field_type);
            }
            if ( decl.init ) {
                if ( decl.init->type ) {
                    if ( !canCopyOrMoveType(decl.type,decl.init->type,TemporaryMatters::yes, decl.init.get()) ) {
                        error("structure field initialization type mismatch, "
                              + describeType(decl.type) + " = " + describeType(decl.init->type),  "", "",
                            decl.at,CompilationError::invalid_initialization_type);
                    } else if ( !decl.type->canCopy() && !decl.moveSemantics ) {
                        error("field " + decl.name + " can't be copied, use <- instead; " + describeType(decl.type), "", "",
                              decl.init->at, CompilationError::invalid_initialization_type );
                    } else if ( !decl.init->type->canCopy() && !decl.init->type->canMove() ) {
                        error("field " + decl.name + "can't be initialized at all; " + describeType(decl.init->type),  "", "",
                            decl.at,CompilationError::invalid_initialization_type);
                    } else if (decl.moveSemantics && decl.init->type->isConst()) {
                        error("can't move from a constant value " + describeType(decl.init->type), "", "",
                            decl.init->at, CompilationError::cant_move);
                    }
                } else if ( !decl.type->isAuto() ){
                    if ( decl.init->rtti_isCast() ) {
                        auto castExpr = static_pointer_cast<ExprCast>(decl.init);
                        if ( castExpr->castType->isAuto() ) {
                            reportAstChanged();
                            castExpr->castType = make_smart<TypeDecl>(*decl.type);
                        }
                    }
                }
            }
            // TODO: verify. correct test is in fact the one bellow
            //  if ( isFullySealedType(decl.type) ) {
            // isFullyInferred may be sufficient
            if ( decl.type->isFullyInferred() ) {
                int fieldAlignemnt = decl.type->getAlignOf();
                int fa = fieldAlignemnt - 1;
                if ( cppLayout ) {
                    auto fp = st->findFieldParent(decl.name);
                    if ( fp!=cppLayoutParent ) {
                        if (DAS_NON_POD_PADDING || cppLayoutPod) {
                            fieldOffset = cppLayoutParent ? cppLayoutParent->getSizeOf64() : 0;
                        }
                        cppLayoutParent = fp;
                    }
                }
                fieldOffset = (fieldOffset + fa) & ~fa;
                decl.offset = int(fieldOffset);
                fieldOffset += decl.type->getSizeOf64();
            }
            verifyType(decl.type);
        }
        void tryMakeStructureCtor ( Structure * var ) {
            if ( !var->genCtor  ) {
                if ( !hasUserConstructor(var->name) ) {
                    auto ctor = makeConstructor(var);
                    ctor->exports = program->options.getBoolOption("always_export_initializer", false);
                    extraFunctions.push_back(ctor);
                    var->genCtor = true;
                    reportAstChanged();
                } else if ( !var->isClass ) {
                    error("structure already has user defined initializer", "", "",
                          var->at, CompilationError::structure_already_has_initializer);
                }
            }
        }
        virtual StructurePtr visit ( Structure * var ) override {
            if ( !var->genCtor && var->hasAnyInitializers() ) {
                tryMakeStructureCtor(var);
            }
            auto tt = make_smart<TypeDecl>(Type::tStructure);
            tt->structType = var;
            if ( isCircularType(tt) ) {
                var->circular = true;
                error("type creates circular dependency",  "", "",
                    var->at,CompilationError::invalid_type);
            }
            return Visitor::visit(var);
        }
    // globals
        virtual void preVisitGlobalLet ( const VariablePtr & var ) override {
            Visitor::preVisitGlobalLet(var);
            if ( checkNoGlobalVariablesAtAll && !var->generated ) {
                error("global variables are disabled by option no_global_variables_at_all", "", "",
                      var->at, CompilationError::no_global_variables );
            }
            if ( var->type->isAuto() && !var->init) {
                error("global variable type can't be inferred, it needs an initializer", "", "",
                      var->at, CompilationError::cant_infer_missing_initializer );
            }
            if ( var->type->isAlias() ) {
                if ( auto aT = inferAlias(var->type) ) {
                    var->type = aT;
                    reportAstChanged();
                } else {
                    error("undefined global variable type " + describeType(var->type),
                        reportInferAliasErrors(var->type), "", var->at, CompilationError::invalid_type );
                }
            }
        }
        virtual void preVisitGlobalLetInit ( const VariablePtr & var, Expression * init ) override {
            Visitor::preVisitGlobalLetInit(var, init);
            globalVar = var;
        }
        virtual ExpressionPtr visitGlobalLetInit ( const VariablePtr & var, Expression * init ) override {
            globalVar = nullptr;
            if ( !var->init->type ) return Visitor::visitGlobalLetInit(var, init);
            if ( var->type->isAuto() ) {
                auto varT = TypeDecl::inferGenericInitType(var->type, var->init->type);
                if ( !varT || varT->isAlias() ) {
                    error("global variable " + var->name + " initialization type can't be inferred, "
                          + describeType(var->type) + " = " + describeType(var->init->type), "", "",
                          var->at, CompilationError::cant_infer_mismatching_restrictions );
                } else {
                    varT->ref = false;
                    TypeDecl::applyAutoContracts(varT, var->type);
                    var->type = varT;
                    reportAstChanged();
                }
            } else if ( !canCopyOrMoveType(var->type,var->init->type,TemporaryMatters::no,var->init.get()) ) {
                error("global variable " + var->name + " initialization type mismatch, "
                      + describeType(var->type) + " = " + describeType(var->init->type),  "", "",
                    var->at, CompilationError::invalid_initialization_type);
            } else if ( var->type->ref && !var->type->isConst() && var->init->type->isConst() ) {
                error("global variable " + var->name + " initialization type mismatch, const matters "
                      + describeType(var->type) + " = " + describeType(var->init->type),  "", "",
                    var->at, CompilationError::invalid_initialization_type);
            } else if ( !var->init_via_clone && (!var->init->type->canCopy() && !var->init->type->canMove()) ) {
                error("global variable " + var->name + " can't be initialized at all. " + describeType(var->type), "", "",
                      var->at, CompilationError::invalid_initialization_type);
            } else if ( var->init_via_move && var->init->type->isConst() ) {
                error("global variable " + var->name + " can't init (move) from a constant value",  "", "",
                    var->at, CompilationError::cant_move);
            } else if ( !(var->init_via_move || var->init_via_clone) && !var->init->type->canCopy() ) {
                error("global variable " + var->name + " can't be copied",  "", "",
                    var->at, CompilationError::cant_copy);
            } else if ( var->init_via_move && !var->init->type->canMove() ) {
                error("global variable " + var->name + " can't be moved",  "", "",
                    var->at, CompilationError::cant_move);
            } else if ( var->init_via_clone && !var->init->type->canClone() ) {
                auto varType = make_smart<TypeDecl>(*var->type);
                varType->ref = true;
                auto fnList = getCloneFunc(varType, var->init->type);
                if ( fnList.size() && verifyCloneFunc(fnList, var->at) ) {
                    return promoteToCloneToMove(var);
                } else {
                    error("global variable " + var->name + " can't be cloned",  "", "",
                        var->at, CompilationError::cant_copy);
                }
            } else {
                if ( var->init_via_clone ) {
                    return promoteToCloneToMove(var);
                }
            }
            if ( var->init->rtti_isVar() ) {    // this folds specifically global a = b, where b is const
                auto ivar = static_pointer_cast<ExprVar>(var->init);
                if ( ivar->isGlobalVariable() && ivar->variable->init && ivar->variable->init->rtti_isConstant() ) {
                    reportAstChanged();
                    return ivar->variable->init;
                }
            }
            if ( disableInit && !var->init->rtti_isConstant() ) {
                program->error("[init] is disabled in the options or CodeOfPolicies", "", "",
                        var->at, CompilationError::no_init);
            }
            return Visitor::visitGlobalLetInit(var, init);
        }
        virtual VariablePtr visitGlobalLet ( const VariablePtr & var ) override {
            if ( var->type && var->type->isExprType() ) {
                return Visitor::visitGlobalLet(var);
            }
            if ( isCircularType(var->type) ) {
                return Visitor::visitGlobalLet(var);
            }
            if ( var->type->ref )
                error("global variable can't be declared a reference",  "", "",
                    var->at, CompilationError::invalid_variable_type);
            if ( var->type->isVoid() )
                error("global variable can't be declared void",  "", "",
                    var->at, CompilationError::invalid_variable_type);
            if ( var->type->isHandle() && !var->type->annotation->isLocal() )
                error("can't have a global variable of handled type " + var->type->annotation->name, "", "",
                      var->at, CompilationError::invalid_variable_type);
            if ( !var->type->constant && var->global_shared )
                error("shared global variable must be constant", "", "",
                      var->at, CompilationError::invalid_variable_type);
            if ( var->global_shared && !var->init )
                error("shared global variable must be initialized", "", "",
                      var->at, CompilationError::invalid_variable_type);
            if ( var->global_shared && !var->type->isShareable() ) {
                if ( !(var->type->isSimpleType(Type::tLambda) && program->policies.allow_shared_lambda) ) {
                    error("this variable type can't be shared, " + describeType(var->type), "", "",
                        var->at, CompilationError::invalid_variable_type);
                }
            }
            if ( !var->type->ref && var->type->hasClasses() ) {
                error("can only have global pointers to classes " + describeType(var->type), "", "",
                      var->at, CompilationError::unsafe);
            }
            if ( !var->init && var->type->hasNonTrivialCtor() ) {
                error("global variable of type " + describeType(var->type) + " needs to be initialized", "", "",
                    var->at, CompilationError::invalid_variable_type);
            }
            verifyType(var->type);
            return Visitor::visitGlobalLet(var);
        }
    // function
        bool safeExpression ( Expression * expr ) const {
            if ( unsafeDepth ) return true;
            if ( expr->alwaysSafe ) return true;
            return false;
        }
        virtual void preVisit ( Function * f ) override {
            Visitor::preVisit(f);
            canFoldResult = true;
            unsafeDepth = 0;
            func = f;
            func->hasReturn = false;
            func->noAot |= disableAot;
            if ( f->arguments.size() > DAS_MAX_FUNCTION_ARGUMENTS ) {
                error("function has too many arguments, max allowed is DAS_MAX_FUNCTION_ARGUMENTS=" DAS_STR(DAS_MAX_FUNCTION_ARGUMENTS),  "", "",
                    f->at, CompilationError::too_many_arguments);
            }
            if ( (f->init | f->shutdown) && disableInit ) {
                error("[init] is disabled in the options or CodeOfPolicies",  "", "",
                    f->at, CompilationError::no_init);
            }
        }
        virtual void preVisitArgument ( Function * fn, const VariablePtr & var, bool lastArg ) override {
            Visitor::preVisitArgument(fn, var, lastArg);
            if ( var->type->isAlias() ) {
                if ( auto aT = inferAlias(var->type) ) {
                    var->type = aT;
                    reportAstChanged();
                } else {
                    error("undefined function argument type " + describeType(var->type),
                        reportInferAliasErrors(var->type), "", var->at, CompilationError::type_not_found );
                }
            }
            if ( var->type->ref && var->type->isRefType() ) {   // silently fix a : Foo& into a : Foo
                var->type->ref = false;
                auto mname = fn->getMangledName();
                if ( fn->module->functions.find(mname) ) {
                    error("function already exists in non-ref form", "\t" + fn->describe(Function::DescribeModule::yes), "",
                        var->at, CompilationError::cant_infer_generic );
                    var->type->ref = true;
                }
            }
        }
        virtual ExpressionPtr visitArgumentInit ( Function * f, const VariablePtr & arg, Expression * that ) override {
            if (arg->type->isAuto() && arg->init->type) {
                auto varT = TypeDecl::inferGenericType(arg->type, arg->init->type, false, false, nullptr);
                if ( !varT ) {
                    error("generic argument type can't be inferred, "
                        + describeType(arg->type) + " = " + describeType(arg->init->type), "", "",
                        arg->at, CompilationError::cant_infer_generic );
                } else {
                    TypeDecl::applyAutoContracts(varT, arg->type);
                    arg->type = varT;
                    arg->type->ref = false; // so that def ( a = VAR ) infers as def ( a : var_type ), not as def ( a : var_type & )
                    reportAstChanged();
                    return Visitor::visitArgumentInit(f, arg, that);
                }
            }
            if ( !arg->init->type || !arg->type->isSameType(*arg->init->type, RefMatters::no, ConstMatters::no, TemporaryMatters::no) ) {
                error("function argument default value type mismatch " + describeType(arg->type)
                    + " vs " + (arg->init->type ? describeType(arg->init->type) : "???"),  "", "",
                    arg->init->at, CompilationError::invalid_type);
            }
            if (arg->init->type && arg->type->ref && !arg->init->type->ref ) {
                error("function argument default value type mismatch " + describeType(arg->type)
                    + " vs " + describeType(arg->init->type) + ", reference matters", "", "",
                    arg->init->at, CompilationError::invalid_type);
            }
            return Visitor::visitArgumentInit(f, arg, that);
        }
        virtual VariablePtr visitArgument ( Function * fn, const VariablePtr & var, bool lastArg ) override {
            if ( var->type->isAuto() ) {
                error("unresolved generics are not supported",  "", "",
                    var->at, CompilationError::cant_infer_generic );
            }
            verifyType(var->type,true);
            return Visitor::visitArgument(fn, var, lastArg);
        }
        virtual FunctionPtr visit ( Function * that ) override {
            // if function got no 'result', function is a void function
            if ( !func->hasReturn && canFoldResult ) {
                if ( func->result->isAuto() ) {
                    func->result = make_smart<TypeDecl>(Type::tVoid);
                    reportAstChanged();
                } else if ( !func->result->isVoid() ){
                    error("function does not return a value", "", "",
                        func->at, CompilationError::expecting_return_value);
                }
            }
            if  ( func->result->isAlias() ) {
                if ( auto aT = inferAlias(func->result) ) {
                    func->result = aT;
                    func->result->sanitize();
                    reportAstChanged();
                } else {
                    error("undefined function result type " + describeType(func->result),
                        reportInferAliasErrors(func->result), "", func->at, CompilationError::type_not_found );
                }
            }
            verifyType(func->result);
            if ( !func->result->isReturnType() ) {
                error("not a valid function return type " + describeType(func->result), "", "",
                      func->result->at,CompilationError::invalid_return_type);
            }
            if ( func->result->isRefType() && !func->result->ref ) {
                if ( func->result->canCopy() ) {
                    func->copyOnReturn = true;
                    func->moveOnReturn = false;
                } else if ( func->result->canMove() ) {
                    func->copyOnReturn = false;
                    func->moveOnReturn = true;
                } else {
                    // the error will be reported in the inferReturnType
                    /*
                    error("this type can't be copied or moved. not a valid function return type "
                          + describeType(func->result),func->result->at,CompilationError::invalid_return_type);
                    */
                }
            } else {
                func->copyOnReturn = false;
                func->moveOnReturn = false;
            }
            // if any of this asserts failed, there is logic error in how we pop
            DAS_ASSERT(loop.size()==0);
            DAS_ASSERT(scopes.size()==0);
            DAS_ASSERT(blocks.size()==0);
            DAS_ASSERT(local.size()==0);
            DAS_ASSERT(with.size()==0);
            labels.clear();
            func.reset();
            return Visitor::visit(that);
        }
    // any expression
        virtual void preVisitExpression ( Expression * expr ) override {
            Visitor::preVisitExpression(expr);
            expr->type.reset();
        }
    // const
        vec4f getEnumerationValue( ExprConstEnumeration * expr, bool & inferred ) const {
            inferred = false;
            auto cfa = expr->enumType->find(expr->text);
            if ( !cfa.second ) {
                return v_zero();
            }
            if ( !cfa.first || !cfa.first->rtti_isConstant() ) {
                return v_zero();
            }
            vec4f envalue = v_zero();
            int64_t iou = getConstExprIntOrUInt(cfa.first);
            switch ( expr->enumType->baseType) {
            case Type::tInt8:
            case Type::tUInt8:      { int8_t tv = int8_t(iou); memcpy(&envalue, &tv, sizeof(int8_t)); break; }
            case Type::tInt16:
            case Type::tUInt16:     { int16_t tv = int16_t(iou); memcpy(&envalue, &tv, sizeof(int16_t)); break; }
            case Type::tInt:
            case Type::tUInt:
            case Type::tBitfield:   { int32_t tv = int32_t(iou); memcpy(&envalue, &tv, sizeof(int32_t)); break; }
            case Type::tInt64:
            case Type::tUInt64:     { memcpy(&envalue, &iou, sizeof(int64_t)); break; }
            default:
                DAS_ASSERTF( 0, "we should not even be here. unsupported enumeration type." );
            }
            inferred = true;
            return envalue;
        }
        virtual ExpressionPtr visit ( ExprConst * c ) override {
            if ( c->baseType==Type::tEnumeration || c->baseType==Type::tEnumeration8 ||
                c->baseType==Type::tEnumeration16 ) {
                auto cE = static_cast<ExprConstEnumeration *>(c);
                bool infE = false;
                c->value = getEnumerationValue(cE, infE);
                if ( infE ) {
                    c->type = cE->enumType->makeEnumType();
                    c->type->constant = true;
                } else {
                    error("enumeration value not inferred yet " + cE->text,  "", "",
                        c->at, CompilationError::invalid_enumeration);
                    c->type.reset();
                }
            } else if ( c->baseType==Type::tBitfield ) {
                auto cB = static_cast<ExprConstBitfield *>(c);
                if ( cB->bitfieldType ) {
                    c->type = make_smart<TypeDecl>(*cB->bitfieldType);
                    c->type->ref = false;
                } else {
                    c->type = make_smart<TypeDecl>(Type::tBitfield);
                }
                c->type->constant = true;
            } else if ( c->baseType==Type::tPointer ) {
                c->type = make_smart<TypeDecl>(c->baseType);
                auto cptr = static_cast<ExprConstPtr *>(c);
                c->type->smartPtr = cptr->isSmartPtr;
                if ( cptr->ptrType ) {
                    c->type->firstType = make_smart<TypeDecl>(*cptr->ptrType);
                    c->type->constant = c->type->firstType->constant;
                } else {
                    c->type->constant = true;
                }
            } else {
                c->type = make_smart<TypeDecl>(c->baseType);
                c->type->constant = true;
            }
            return Visitor::visit(c);
        }
    // ExprUnsafe
        virtual void preVisit ( ExprUnsafe * expr ) override {
            Visitor::preVisit(expr);
            unsafeDepth ++;
        }
        virtual ExpressionPtr visit ( ExprUnsafe * expr ) override {
            unsafeDepth --;
            return Visitor::visit(expr);
        }
    // ExprGoto
        Expression * findLabel ( int32_t label ) const {
            for ( auto it = scopes.rbegin(), its=scopes.rend(); it!=its; ++it ) {
                auto blk = *it;
                for ( const auto & ex : blk->list ) {
                    if ( ex->rtti_isLabel() ) {
                        auto lab = static_pointer_cast<ExprLabel>(ex);
                        if ( lab->label == label ) {
                            return lab.get();
                        }
                    }
                }
            }
            return nullptr;
        }
        virtual ExpressionPtr visit ( ExprGoto * expr ) override {
            if ( expr->subexpr ) {
                if ( !expr->subexpr->type ) return Visitor::visit(expr);
                expr->subexpr = Expression::autoDereference(expr->subexpr);
                if ( !expr->subexpr->type->isSimpleType(Type::tInt) ) {
                    error("label type has to be int, not " + describeType(expr->subexpr->type),  "", "",
                        expr->at, CompilationError::invalid_label);
                } else {
                    if ( enableInferTimeFolding ) {
                        if ( auto se = getConstExpr(expr->subexpr.get()) ) {
                            auto le = static_pointer_cast<ExprConstInt>(se);
                            expr->label = le->getValue();
                            expr->subexpr = nullptr;
                        }
                    }
                }
            }
            if ( !expr->subexpr && !findLabel(expr->label) ) {
                error("can't find label " + to_string(expr->label),  "", "",
                    expr->at, CompilationError::invalid_label);
            }
            return Visitor::visit(expr);
        }
    // ExprReader
        virtual ExpressionPtr visit ( ExprReader * expr ) override {
            // implement reader macros
            auto errc = ctx.thisProgram->errors.size();
            auto thisModule = ctx.thisProgram->thisModule.get();
            auto substitute = expr->macro->visit(ctx.thisProgram, thisModule, expr);
            if ( substitute ) {
                reportAstChanged();
                return substitute;
            }
            if ( errc==ctx.thisProgram->errors.size() ) {
                error("unsupported read macro " + expr->macro->name,  "", "",
                    expr->at, CompilationError::unsupported_read_macro);
            }
            return Visitor::visit(expr);
        }
    // ExprLabel
        virtual void preVisit ( ExprLabel * expr ) override {
            Visitor::preVisit(expr);
            if ( !labels.insert(expr->label).second ) {
                error("duplicate label " + to_string(expr->label),  "", "",
                    expr->at, CompilationError::invalid_label);
            }
        }
    // ExprRef2Value
        virtual ExpressionPtr visit ( ExprRef2Value * expr ) override {
            if ( !expr->subexpr->type ) return Visitor::visit(expr);
            // infer
            if ( !expr->subexpr->type->isRef() ) {
                if ( expr->subexpr->rtti_isConstant() ) {
                    reportAstChanged();
                    return expr->subexpr;
                } else {
                    TextWriter tw;
                    tw << *expr->subexpr;
                    error("can only dereference a reference", tw.str(), "",
                        expr->at, CompilationError::invalid_type);
                }
            } else if ( !expr->subexpr->type->isSimpleType() ) {
                error("can only dereference value types, not a " + describeType(expr->subexpr->type),  "", "",
                    expr->at, CompilationError::invalid_type);
            } else {
                expr->type = make_smart<TypeDecl>(*expr->subexpr->type);
                expr->type->ref = false;
            }
            return Visitor::visit(expr);
        }
    // ExprAddr
        virtual ExpressionPtr visit ( ExprAddr * expr ) override {
            if (expr->funcType) {
                // when we infer function type, we really don't care for the result.
                // however having auto or alias in the result may cause problems, so we swap it to void
                // and then swap it back to whatever it was
                auto retT = expr->funcType->firstType;
                expr->funcType->firstType = make_smart<TypeDecl>(Type::tVoid);
                if (expr->funcType->isAlias()) {
                    auto aT = inferAlias(expr->funcType);
                    if (aT) {
                        expr->funcType = aT;
                        reportAstChanged();
                    } else {
                        error("undefined address expression type " + describeType(expr->funcType),
                            reportInferAliasErrors(expr->funcType), "", expr->at, CompilationError::type_not_found);
                        return Visitor::visit(expr);
                    }
                }
                if (expr->funcType->isAuto()) {
                    error("function of undefined type " + describeType(expr->funcType),  "", "",
                        expr->at, CompilationError::type_not_found);
                    return Visitor::visit(expr);
                }
                expr->funcType->firstType = retT;
            }
            expr->func = nullptr;
            MatchingFunctions fns;
            if (expr->funcType) {
                if ( !expr->funcType->isFunction() ) {
                    error("function of non-function type " + describeType(expr->funcType),  "", "",
                        expr->at, CompilationError::type_not_found);
                }
                fns = findTypedFuncAddr(expr->target, expr->funcType->argTypes);
                if ( fns.size()==0 ) {
                    string moduleName, funcName;
                    splitTypeName(expr->target, moduleName, funcName);
                    fns = findTypedFuncAddr(moduleName + "::`" + funcName, expr->funcType->argTypes);
                    if ( fns.size()==1 && !fns.back()->fromGeneric ) {
                        error("function not found " + expr->target,  "", "",
                            expr->at, CompilationError::function_not_found);
                        return Visitor::visit(expr);
                    }
                }
            } else {
                fns = findFuncAddr(expr->target);
            }
            if ( fns.size()==1 ) {
                expr->func = fns.back();
                expr->func->addr = true;
                expr->type = make_smart<TypeDecl>(Type::tFunction);
                expr->type->firstType = make_smart<TypeDecl>(*expr->func->result);
                expr->type->argTypes.reserve ( expr->func->arguments.size() );
                for ( auto & arg : expr->func->arguments ) {
                    auto at = make_smart<TypeDecl>(*arg->type);
                    expr->type->argTypes.push_back(at);
                    expr->type->argNames.push_back(arg->name);
                }
                verifyType(expr->type);
            } else if ( fns.size()==0 ) {
                error("function not found " + expr->target,  "", "",
                    expr->at, CompilationError::function_not_found);
            } else {
                string candidates = verbose ? program->describeCandidates(fns) : "";
                error("function not found " + expr->target, candidates, "",
                    expr->at, CompilationError::function_not_found);
            }
            if (expr->func && expr->func->builtIn) {
                error("can't get address of builtin function " + describeFunction(expr->func),  "", "",
                    expr->at, CompilationError::type_not_found);
                return Visitor::visit(expr);
            }
            return Visitor::visit(expr);
        }
    // ExprPtr2Ref
        virtual ExpressionPtr visit ( ExprPtr2Ref * expr ) override {
            if ( !expr->subexpr->type ) return Visitor::visit(expr);
            // safe deref of non-pointer is it
            if ( expr->alwaysSafe && !expr->subexpr->type->isPointer() ) {
                reportAstChanged();
                return expr->subexpr;
            }
            expr->unsafeDeref = func ? func->unsafeDeref : false;
            // infer
            expr->subexpr = Expression::autoDereference(expr->subexpr);
            if ( !expr->subexpr->type->isPointer() ) {
                error("can only dereference a pointer",  "", "",
                    expr->at, CompilationError::cant_dereference);
            } else if ( !expr->subexpr->type->firstType || expr->subexpr->type->firstType->isVoid() ) {
                error("can only dereference a pointer to something",  "", "",
                    expr->at, CompilationError::cant_dereference);
            } else {
                expr->type = make_smart<TypeDecl>(*expr->subexpr->type->firstType);
                expr->type->ref = true;
                expr->type->constant |= expr->subexpr->type->constant;
                propagateTempType(expr->subexpr->type, expr->type); // deref(Foo#?) is Foo#
            }
            return Visitor::visit(expr);
        }
    // ExprRef2Ptr
        virtual ExpressionPtr visit ( ExprRef2Ptr * expr ) override {
            if ( !expr->subexpr->type ) return Visitor::visit(expr);
            // infer
            if ( !expr->subexpr->type->isRef() ) {
                error("can only make a pointer of of a reference",  "", "",
                    expr->at, CompilationError::cant_dereference);
            } else {
                if ( !safeExpression(expr) ) {
                    if ( !expr->subexpr->type->temporary ) { // address of temp type is temp, but its safe
                        error("address of reference requires unsafe",  "", "",
                            expr->at, CompilationError::unsafe);
                    }
                }
                expr->type = make_smart<TypeDecl>(Type::tPointer);
                expr->type->firstType = make_smart<TypeDecl>(*expr->subexpr->type);
                expr->type->firstType->ref = false;
                expr->type->constant |= expr->subexpr->type->constant;
                propagateTempType(expr->subexpr->type, expr->type); // addr(Foo#) is Foo#?#
            }
            return Visitor::visit(expr);
        }
    // ExprNullCoalescing
        void propagateAlwaysSafe ( const ExpressionPtr & expr ) {
            if ( expr->alwaysSafe ) return;
            // make a ?as b ?? c always safe
            if (expr->rtti_isSafeAsVariant()) {
                reportAstChanged();
                auto sav = static_pointer_cast<ExprSafeAsVariant>(expr);
                sav->alwaysSafe = true;
                if ( sav->value->type->isPointer() ) {
                    propagateAlwaysSafe(sav->value);
                }
            }
            // make a ?[b] ?? c always safe
            else if ( expr->rtti_isSafeAt() ) {
                reportAstChanged();
                auto sat = static_pointer_cast<ExprSafeAt>(expr);
                sat->alwaysSafe = true;
                if ( sat->subexpr->type->isPointer() ) {
                    propagateAlwaysSafe(sat->subexpr);
                }
            }
            // make a ? b ?? c always safe (we need flag only)
            else if ( expr->rtti_isSafeField() ) {
                reportAstChanged();
                auto saf = static_pointer_cast<ExprSafeField>(expr);
                saf->alwaysSafe = true;
                if ( saf->value->type->isPointer() ) {
                    propagateAlwaysSafe(saf->value);
                }
            }
        }
        virtual ExpressionPtr visit ( ExprNullCoalescing * expr ) override {
            if ( !expr->subexpr->type      || expr->subexpr->type->isAliasOrExpr()      ) return Visitor::visit(expr);  // failed to infer
            if ( !expr->defaultValue->type || expr->defaultValue->type->isAliasOrExpr() ) return Visitor::visit(expr);  // failed to infer
            if ( auto opE = inferGenericOperator("??",expr->at,expr->subexpr,expr->defaultValue) ) return opE;
            // infer
            expr->subexpr = Expression::autoDereference(expr->subexpr);
            auto seT = expr->subexpr->type;
            auto dvT = expr->defaultValue->type;
            if ( !seT->isPointer() ) {
                error("can only dereference a pointer",  "", "",
                    expr->at, CompilationError::cant_dereference);
            } else if ( !seT->firstType || seT->firstType->isVoid() ) {
                error("can only dereference a pointer to something",  "", "",
                    expr->at, CompilationError::cant_dereference);
            } else if ( !seT->firstType->isSameType(*dvT,RefMatters::no, ConstMatters::no, TemporaryMatters::yes) ) {
                error("default value type mismatch in " + describeType(seT->firstType) + " vs "
                      + describeType(dvT), "", "",
                    expr->at, CompilationError::cant_dereference);
            } else if ( seT->isRef() && !seT->isConst() && dvT->isConst() ) {
                error("default value type mismatch, constant matters in " + describeType(seT) + " vs "
                      + describeType(dvT),  "", "",
                    expr->at, CompilationError::cant_dereference);
            } else {
                expr->type = make_smart<TypeDecl>(*dvT);
                expr->type->constant |= expr->subexpr->type->constant;
                propagateTempType(expr->subexpr->type, expr->type); // t?# ?? def = #t
                propagateAlwaysSafe(expr->subexpr);
            }
            return Visitor::visit(expr);
        }
    // ExprStaticAssert
        virtual ExpressionPtr visit ( ExprStaticAssert * expr ) override {
            if ( expr->argumentsFailedToInfer ) return Visitor::visit(expr);
            if ( expr->arguments.size()<1 || expr->arguments.size()>2  ) {
                error("static_assert(expr) or static_assert(expr,string)",  "", "",
                    expr->at, CompilationError::invalid_argument_count);
                return Visitor::visit(expr);
            }
            // infer
            expr->autoDereference();
            if ( !expr->arguments[0]->type->isSimpleType(Type::tBool) )
                error("static assert condition must be boolean",  "", "",
                    expr->at, CompilationError::invalid_argument_type);
            if ( expr->arguments.size()==2 && !expr->arguments[1]->rtti_isStringConstant() ) {
                error("static assert comment must be string constant",  "", "",
                    expr->at, CompilationError::invalid_argument_type);
            }
            expr->type = make_smart<TypeDecl>(Type::tVoid);
            return Visitor::visit(expr);
        }
    // ExprAssert
        virtual ExpressionPtr visit ( ExprAssert * expr ) override {
            if ( expr->argumentsFailedToInfer ) return Visitor::visit(expr);
            if ( expr->arguments.size()<1 || expr->arguments.size()>2  ) {
                error("assert(expr) or assert(expr,string)",  "", "",
                    expr->at, CompilationError::invalid_argument_count);
                return Visitor::visit(expr);
            }
            // infer
            expr->autoDereference();
            if ( !expr->arguments[0]->type->isSimpleType(Type::tBool) )
                error("assert condition must be boolean",  "", "",
                    expr->at, CompilationError::invalid_argument_type);
            if ( expr->arguments.size()==2 && !expr->arguments[1]->rtti_isStringConstant() ) {
                error("assert comment must be string constant",  "", "",
                    expr->at, CompilationError::invalid_argument_type);
            }
            expr->type = make_smart<TypeDecl>(Type::tVoid);
            return Visitor::visit(expr);
        }
    // ExprQuote
        virtual ExpressionPtr visit ( ExprQuote * expr ) override {
            if ( expr->arguments.size()!=1  ) {
                error("quote(expr) only. can only return one expression tree",  "", "",
                    expr->at, CompilationError::invalid_argument_count);
                return Visitor::visit(expr);
            }
            // infer
            expr->type = make_smart<TypeDecl>(Type::tPointer);
            expr->type->smartPtr = true;
            expr->type->smartPtrNative = true;
            expr->type->firstType = make_smart<TypeDecl>(Type::tHandle);
            expr->type->firstType->annotation = (TypeAnnotation *) Module::require("ast")->findAnnotation("Expression").get();
            // mark quote as noAot
            if ( func  ) func->noAot = true;
            return Visitor::visit(expr);
        }
    // ExprDebug
        virtual ExpressionPtr visit ( ExprDebug * expr ) override {
            if ( expr->argumentsFailedToInfer ) return Visitor::visit(expr);
            if ( expr->arguments.size()<1 || expr->arguments.size()>2 ) {
                error("debug(expr) or debug(expr,string)",  "", "",
                    expr->at, CompilationError::invalid_argument_count);
                return Visitor::visit(expr);
            }
            // infer
            if ( expr->arguments.size()==2 && !expr->arguments[1]->rtti_isStringConstant() ) {
                error("debug comment must be string constant",  "", "",
                    expr->at, CompilationError::invalid_argument_type);
            }
            expr->type = make_smart<TypeDecl>(*expr->arguments[0]->type);
            return Visitor::visit(expr);
        }
    // ExprMemZero
        virtual ExpressionPtr visit ( ExprMemZero * expr ) override {
            if ( expr->argumentsFailedToInfer ) return Visitor::visit(expr);
            if ( expr->arguments.size()!=1 ) {
                error("memzero(ref expr)",  "", "",
                    expr->at, CompilationError::invalid_argument_count);
                return Visitor::visit(expr);
            }
            // infer
            const auto & arg = expr->arguments[0];
            if ( !arg->type->isRef() ) {
                error("memzero argument must be reference",  "", "",
                    expr->at, CompilationError::invalid_argument_type);
            } else if ( arg->type->isConst() ) {
                error("memzero argument can't be constant",  "", "",
                    expr->at, CompilationError::invalid_argument_type);
            }
            expr->type = make_smart<TypeDecl>();
            return Visitor::visit(expr);
        }
    // ExprMakeGenerator
        virtual ExpressionPtr visit ( ExprMakeGenerator * expr ) override {
            if ( expr->iterType && expr->iterType->isExprType() ) {
                return Visitor::visit(expr);
            }
            if ( expr->iterType->isAlias() ) {
                auto aT = inferAlias(expr->iterType);
                if ( aT ) {
                    expr->iterType = aT;
                    reportAstChanged();
                } else {
                    error("undefined generator type " + describeType(expr->iterType),
                        reportInferAliasErrors(expr->iterType), "", expr->at, CompilationError::type_not_found);
                    return Visitor::visit(expr);
                }
            }
            if ( expr->iterType->isAuto() ) {
                error("generator of undefined type " + describeType(expr->iterType),  "", "",
                    expr->at, CompilationError::type_not_found);
                return Visitor::visit(expr);
            } else if ( expr->iterType->isVoid() ) {
                error("generator can't be void (yet)",  "", "",
                    expr->at, CompilationError::type_not_found);
                return Visitor::visit(expr);
            }
            if ( expr->arguments.size()!=1 ) {
                error("expecting generator(closure)",  "", "",
                    expr->at, CompilationError::invalid_argument_count);
            } else if ( !expr->arguments[0]->rtti_isMakeBlock() ) {
                error("expecting generator(closure)",  "", "",
                    expr->at, CompilationError::invalid_argument_type);
            } else {
                auto mkBlock = static_pointer_cast<ExprMakeBlock>(expr->arguments[0]);
                auto block = static_pointer_cast<ExprBlock>(mkBlock->block);
                if ( auto bT = block->makeBlockType() ) {
                    // TODO: verify
                    // if ( !isFullySealedType(bT) ) {
                    if ( bT->isAutoOrAlias() ) {
                        error("can't infer generator block type",  "", "",
                            expr->at, CompilationError::invalid_block);
                    } else if ( !bT->firstType->isSimpleType(Type::tBool) ) {
                        error("generator must return boolean",  "", "",
                            expr->at, CompilationError::invalid_argument_type);
                    } else if ( !bT->argTypes.empty() ) {
                        error("generator must have no arguments",  "", "",
                            expr->at, CompilationError::invalid_argument_type);
                    } else {
                        // TODO: check validity of the generator type
                        CaptureLambda cl;
                        // we can only capture in-scope variables
                        // i.e stuff BEFORE the scope
                        for ( auto & lv : local )
                            cl.scope.insert(lv);
                        for ( auto & bls : blocks ) {
                            for ( auto & blv : bls->arguments ) {
                                cl.scope.insert(blv);
                            }
                        }
                        block->visit(cl);
                        if ( !cl.fail ) {
                            for ( auto ba : block->arguments ) {
                                cl.capt.erase(ba);
                            }
                            // add "yield" argument
                            bool makeRef = false;
                            if ( !expr->iterType->isVoid() ) {
                                auto yva = make_smart<Variable>();
                                if ( expr->iterType->ref ) {
                                    yva->type = make_smart<TypeDecl>(Type::tPointer);
                                    yva->type->firstType = make_smart<TypeDecl>(*expr->iterType);
                                    yva->type->firstType->ref = false;
                                    yva->type->constant = false;
                                    yva->type->ref = true;
                                    makeRef = true;
                                } else {
                                    yva->type = make_smart<TypeDecl>(*expr->iterType);
                                    yva->type->constant = false;
                                    yva->type->ref = !expr->iterType->isRefType();
                                }
                                yva->name = (makeRef ? "_ryield" : "_yield_") + to_string(block->at.line);
                                yva->at = block->at;
                                yva->generated = true;
                                block->arguments.push_back(yva);
                            }
                            // collapse it from the very top, so that all the macro goo which adds empty block goo collapses
                            block->collapse();
                            // make it all
                            bool isUnsafe = !safeExpression(expr);
                            if ( verifyCapture(expr->capture, cl, isUnsafe, expr->at) ) {
                                string lname = generateNewLambdaName(block->at);
                                auto ls = generateLambdaStruct(lname, block.get(), cl.capt, expr->capture, true);
                                ls->generator = true;
                                if ( program->addStructure(ls) ) {
                                    auto jitFlags = (func && func->requestJit) ? generator_jit : 0;
                                    auto pFn = generateLambdaFunction(lname, block.get(), ls, cl.capt, expr->capture, generator_needYield | jitFlags, program);
                                    if ( program->addFunction(pFn) ) {
                                        auto pFnFin = generateLambdaFinalizer(lname, block.get(), ls);
                                        if ( program->addFunction(pFnFin) ) {
                                            if ( func && func->isClassMethod ) {
                                                // lambda, captured in the class is a method of that class - for the purposes of 'private'
                                                pFn->isClassMethod = true;
                                                pFn->classParent = func->classParent;
                                                DAS_ASSERT(pFn->classParent);
                                                pFnFin->isClassMethod = true;
                                                pFnFin->classParent = func->classParent;
                                                DAS_ASSERT(pFnFin->classParent);
                                            }
                                            reportAstChanged();
                                            auto ms = generateLambdaMakeStruct ( ls, pFn, pFnFin, cl.capt, expr->capture, expr->at, program );
                                            // each ( [[ ]]] )
                                            auto cEach = make_smart<ExprCall>(block->at, makeRef ? "each_ref" : "each");
                                            cEach->generated = true;
                                            cEach->arguments.push_back(ms);
                                            return cEach;
                                        } else {
                                            error("generator finalizer name mismatch",  "", "",
                                                expr->at, CompilationError::invalid_block);
                                        }
                                    } else {
                                        error("generator function name mismatch",  "", "",
                                            expr->at, CompilationError::invalid_block);
                                    }
                                } else {
                                    error("generator struct name mismatch " + ls->name,  "", "",
                                        expr->at, CompilationError::invalid_block);
                                }
                            }
                            // in case of error
                            if ( !expr->iterType->isVoid() ) {
                                block->arguments.pop_back();
                            }
                        }
                    }
                }
            }
            return Visitor::visit(expr);
        }
    // ExprMakeBlock
        bool verifyCapture ( const vector<CaptureEntry> & capture, const CaptureLambda & cl, bool isUnsafe, const LineInfo & at ) {
            for ( auto & cV : cl.capt ) {
                CaptureMode mode = CaptureMode::capture_any;
                auto it = find_if ( capture.begin(), capture.end(), [&] ( const auto & entry ){
                    return entry.name == cV->name;
                });
                if ( it != capture.end() ) {
                    mode = it->mode;
                }
                if ( mode == CaptureMode::capture_any ) {
                    if ( cV->capture_as_ref ) {
                        // this is ok by default
                    } else if ( !cV->type->canCopy() && !cV->type->canMove() ) {
                        error("can't captured variable " + cV->name,  "it can't be copied or moved", "",
                            at, CompilationError::invalid_capture);
                        return false;
                    } else if ( !cV->type->canCopy() && isUnsafe ) {
                        error("implicit capture by move requires unsafe, while capturing " + cV->name,  "", "",
                            at, CompilationError::invalid_capture);
                        return false;
                    }
                } else if ( mode == CaptureMode::capture_by_reference ) {
                    if ( !cV->capture_as_ref && isUnsafe ) {
                        error("capture by reference requires unsafe, while capturing " + cV->name,  "", "",
                            at, CompilationError::invalid_capture);
                        return false;
                    }
                } else if ( mode == CaptureMode::capture_by_move ) {
                    if ( !cV->type->canMove() ) {
                        error("can't move captured variable " + cV->name,  "", "",
                            at, CompilationError::invalid_capture);
                        return false;
                    }
                } else if ( mode == CaptureMode::capture_by_copy ) {
                    if ( !cV->type->canCopy() ) {
                        error("can't copy captured variable " + cV->name,  "", "",
                            at, CompilationError::invalid_capture);
                        return false;
                    }
                }
                if ( cV->no_capture ) {
                    error("can't capture variable " + cV->name,
                        cV->name=="self" ? "can't capture `self` in the class initializer" :  "it is marked as no_capture",
                            "", at, CompilationError::invalid_capture);
                    return false;
                }
            }
            return true;
        }
        ExpressionPtr convertBlockToLambda ( ExprMakeBlock * expr ) {
            auto block = static_pointer_cast<ExprBlock>(expr->block);
            if ( auto bT = block->makeBlockType() ) {
                // TODO: verify
                // if ( !isFullySealedType(bT) ) {
                if ( bT->isAutoOrAlias() ) {
                    error("can't infer lambda block type",  "", "",
                        expr->at, CompilationError::invalid_block);
                } else {
                    CaptureLambda cl;
                    // we can only capture in-scope variables
                    // i.e stuff BEFORE the scope
                    for ( auto & lv : local )
                        cl.scope.insert(lv);
                    for ( auto & bls : blocks ) {
                        for ( auto & blv : bls->arguments ) {
                            cl.scope.insert(blv);
                        }
                    }
                    block->visit(cl);
                    if ( !cl.fail ) {
                        for ( auto ba : block->arguments ) {
                            cl.capt.erase(ba);
                        }
                        bool isUnsafe = !safeExpression(expr);
                        if ( verifyCapture(expr->capture, cl, isUnsafe, expr->at) ) {
                            string lname = generateNewLambdaName(block->at);
                            auto ls = generateLambdaStruct(lname, block.get(), cl.capt, expr->capture, false);
                            if ( program->addStructure(ls) ) {
                                auto jitFlags = (func && func->requestJit) ? generator_jit : 0;
                                auto pFn = generateLambdaFunction(lname, block.get(), ls, cl.capt, expr->capture, jitFlags, program);
                                if ( program->addFunction(pFn) ) {
                                    auto pFnFin = generateLambdaFinalizer(lname, block.get(), ls);
                                    if ( program->addFunction(pFnFin) ) {
                                        // lambda, captured in the class is a method of that class - for the purposes of 'private'
                                        if ( func && func->isClassMethod ) {
                                            pFn->isClassMethod = true;
                                            pFn->classParent = func->classParent;
                                            DAS_ASSERT(pFn->classParent);
                                            pFnFin->isClassMethod = true;
                                            pFnFin->classParent = func->classParent;
                                            DAS_ASSERT(pFnFin->classParent);
                                        }
                                        reportAstChanged();
                                        auto ms = generateLambdaMakeStruct ( ls, pFn, pFnFin, cl.capt, expr->capture, expr->at, program );
                                        return ms;
                                    } else {
                                        error("lambda finalizer name mismatch",  "", "",
                                            expr->at, CompilationError::invalid_block);
                                    }
                                } else {
                                    error("lambda function name mismatch",  "", "",
                                        expr->at, CompilationError::invalid_block);
                                }
                            } else {
                                error("lambda struct name mismatch",  "", "",
                                    expr->at, CompilationError::invalid_block);
                            }
                        }
                    }
                }
            }
            return nullptr;
        }
        ExpressionPtr convertBlockToLocalFunction ( ExprMakeBlock * expr ) {
            auto block = static_pointer_cast<ExprBlock>(expr->block);
            if ( auto bT = block->makeBlockType() ) {
                // TODO: verify
                // if ( !isFullySealedType(bT) ) {
                if ( bT->isAutoOrAlias() ) {
                    error("can't infer local function block type",  "", "",
                        expr->at, CompilationError::invalid_block);
                } else {
                    string lname = generateNewLocalFunctionName(block->at);
                    auto pFn = generateLocalFunction(lname, block.get());
                    if ( program->addFunction(pFn) ) {
                        reportAstChanged();
                        return make_smart<ExprAddr>(expr->at, "_::" + lname + "`function");
                    } else {
                        error("local function name mismatch",  "", "",
                            expr->at, CompilationError::invalid_block);
                    }
                }
            }
            return nullptr;
        }
        virtual ExpressionPtr visit ( ExprMakeBlock * expr ) override {
            auto block = static_pointer_cast<ExprBlock>(expr->block);
            // can only infer block type, if all argument types are inferred
            for ( auto & arg : block->arguments ) {
                if ( arg->type->isAlias() ) {
                    error(reportAliasError(arg->type),  "", "",
                        arg->at, CompilationError::invalid_argument_type);
                    return Visitor::visit(expr);
                }
            }
            expr->type = block->makeBlockType();
            if ( expr->isLambda ) {
                expr->type->baseType = Type::tLambda;
                // TODO: verify
                // if ( isFullySealedType(expr->type) ) {
                if ( !expr->type->isAutoOrAlias() ) {
                    if ( isFullyInferredBlock(block.get()) ) {
                        if ( auto btl = convertBlockToLambda(expr) ) {
                            return btl;
                        }
                    } else {
                        error("block is not fully inferred yet",  "", "",
                            expr->at, CompilationError::invalid_block);
                    }
                }
            } else if ( expr->isLocalFunction ) {
                expr->type->baseType = Type::tFunction;
                if ( !expr->type->isAutoOrAlias() ) {
                    if ( isFullyInferredBlock(block.get()) ) {
                        if ( auto btl = convertBlockToLocalFunction(expr) ) {
                            return btl;
                        }
                    } else {
                        error("block is not fully inferred yet",  "", "",
                            expr->at, CompilationError::invalid_block);
                    }
                }
            }
            return Visitor::visit(expr);
        }
    // ExprInvoke
        virtual ExpressionPtr visit ( ExprInvoke * expr ) override {
            if ( expr->argumentsFailedToInfer ) {
                auto blockT = expr->arguments[0]->type;
                if ( !blockT ) {
                    // no go, no block
                } else if ( !blockT->isGoodBlockType() && !blockT->isGoodFunctionType() && !blockT->isGoodLambdaType() ) {
                    // no go, not a good block
                } else if ( expr->arguments.size()-1 != blockT->argTypes.size() ) {
                    // default arguments
                    // invoke(foo.GetValue,cast<auto> foo)
                    if ( expr->isInvokeMethod ) {
                        auto classDotMethod = expr->arguments[0];
                        if ( classDotMethod->rtti_isR2V() ) {
                            classDotMethod = static_pointer_cast<ExprRef2Value>(classDotMethod)->subexpr;
                        }
                        if ( classDotMethod->rtti_isField() ) {
                            auto eField = static_pointer_cast<ExprField>(classDotMethod);
                            if ( eField->value->type && !eField->value->type->isAutoOrAlias() ) {
                                if ( eField->value->type->baseType==Type::tStructure ) {
                                    auto stt = eField->value->type->structType;
                                    auto sttf = stt->findField(eField->name);
                                    if ( sttf ) {
                                        if ( sttf->init && sttf->init->rtti_isAddr() ) {
                                            auto fnAddr = static_pointer_cast<ExprAddr>(sttf->init);
                                            if ( fnAddr->func ) {
                                                int fnArgSize = int(fnAddr->func->arguments.size());
                                                int fromFnArgSize = int(expr->arguments.size()-1);
                                                bool allHaveInit = true;
                                                for ( int ai=fromFnArgSize; ai<fnArgSize; ++ai ) {
                                                    if ( !fnAddr->func->arguments[ai]->init ) {
                                                        allHaveInit = false;
                                                        break;
                                                    }
                                                }
                                                if ( allHaveInit ) {
                                                    for ( int ai=fromFnArgSize; ai<fnArgSize; ++ai ) {
                                                        expr->arguments.emplace_back(fnAddr->func->arguments[ai]->init->clone());
                                                    }
                                                    reportAstChanged();
                                                    return Visitor::visit(expr);
                                                }
                                            } else {
                                                error(fnAddr->target + " is not fully resolved yet", "", "",
                                                    expr->at, CompilationError::invalid_argument_count);
                                            }
                                        } else {
                                            error(stt->name + "->" + eField->name + " expecting function", "", "",
                                                expr->at, CompilationError::invalid_argument_count);
                                        }
                                    } else {
                                        error(stt->name + "->" + eField->name + " not found", "", "",
                                            expr->at, CompilationError::invalid_argument_count);
                                    }
                                } else {
                                    error("expecting class->method", "", "",
                                        expr->at, CompilationError::invalid_argument_count);
                                }
                            } else {
                                error("class type is not inferred yet", "", "",
                                    expr->at, CompilationError::invalid_argument_count);
                            }
                        } else {
                            error("expecting class.method in the method invoke", "", "",
                                expr->at, CompilationError::invalid_argument_count);
                        }
                    }
                    error("expecting " + to_string(blockT->argTypes.size()) + " arguments, got " + to_string(expr->arguments.size()-1) ,  "", "",
                        expr->at, CompilationError::invalid_argument_count);
                } else {
                    for ( size_t i=0, is=blockT->argTypes.size(); i!=is; ++i ) {
                        auto & arg = expr->arguments[i+1];
                        auto & passType = arg->type;
                        auto & argType = blockT->argTypes[i];
                        if ( arg->rtti_isCast() && !passType ) {
                            auto argCast = static_pointer_cast<ExprCast>(arg);
                            if ( argCast->castType->isAuto() ) {
                                reportAstChanged();
                                argCast->castType = make_smart<TypeDecl>(*argType);
                            }
                        }
                    }
                }
                return Visitor::visit(expr);
            }
            if ( expr->arguments.size()<1 ) {
                error("expecting invoke(block_or_function_or_lambda) or invoke(block_or_function_or_lambda,...)",  "", "",
                    expr->at, CompilationError::invalid_argument_count);
                return Visitor::visit(expr);
            }
            // infer
            expr->arguments[0] = Expression::autoDereference(expr->arguments[0]);
            auto blockT = expr->arguments[0]->type;
            if ( blockT->isAutoOrAlias() ) {
                error("invoke argument not fully inferred " + describeType(blockT),  "", "",
                    expr->at, CompilationError::invalid_argument_type);
                return Visitor::visit(expr);
            }
            // promote invoke(string_name,...) into string_name(...)
            if ( expr->arguments[0]->rtti_isStringConstant() ) {
                auto cname = static_pointer_cast<ExprConstString>(expr->arguments[0])->text;
                auto call = make_smart<ExprCall>(expr->at, cname);
                for ( size_t i=1, is=expr->arguments.size(); i<is; ++i ) {
                    call->arguments.push_back(expr->arguments[i]->clone());
                }
                reportAstChanged();
                return call;
            }
            if ( !blockT->isGoodBlockType() && !blockT->isGoodFunctionType() && !blockT->isGoodLambdaType() && !blockT->isString() ) {
                error("expecting block, or function, or lambda, or string, not a " + describeType(blockT),  "", "",
                    expr->at, CompilationError::invalid_argument_type);
                 return Visitor::visit(expr);
            }
            if ( !blockT->isString() && expr->arguments.size()-1 != blockT->argTypes.size() ) {
                error("invalid number of arguments, expecting " + describeType(blockT), "", "",
                      expr->at, CompilationError::invalid_argument_count);
                return Visitor::visit(expr);
            }
            if ( blockT->isGoodLambdaType() ) {
                expr->arguments[0] = Expression::autoDereference(expr->arguments[0]);
            }
            for ( size_t i=0, is=blockT->argTypes.size(); i!=is; ++i ) {
                auto & passType = expr->arguments[i+1]->type;
                auto & argType = blockT->argTypes[i];
                if ( !isMatchingArgument(nullptr, argType, passType, false,false) ) {
                    auto extras = verbose ? ("\n" + describeMismatchingArgument(to_string(i+1), passType, argType, (int)i)) : "";
                    error("incompatible argument " + to_string(i+1),
                        "\t" + describeType(passType) + " vs " + describeType(argType) + extras, "",
                        expr->at, CompilationError::invalid_argument_type);
                    return Visitor::visit(expr);
                }
                // TODO: invoke with TEMP types
                if ( !argType->isRef() )
                    expr->arguments[i+1] = Expression::autoDereference(expr->arguments[i+1]);
            }
            if ( blockT->firstType ) {
                expr->type = make_smart<TypeDecl>(*blockT->firstType);
            } else {
                expr->type = make_smart<TypeDecl>();
            }
            return Visitor::visit(expr);
        }

    // ExprSetInsert
        virtual ExpressionPtr visit ( ExprSetInsert * expr ) override {
            if ( expr->argumentsFailedToInfer ) return Visitor::visit(expr);
            if ( expr->arguments.size()!=2 ) {
                error("insert(table,key)",  "", "",
                    expr->at, CompilationError::invalid_argument_count);
                return Visitor::visit(expr);
            }
            // infer
            expr->arguments[1] = Expression::autoDereference(expr->arguments[1]);
            auto containerType = expr->arguments[0]->type;
            auto valueType = expr->arguments[1]->type;
            if ( containerType->isGoodTableType() ) {
                if ( !containerType->firstType->isSameType(*valueType,RefMatters::no, ConstMatters::no, TemporaryMatters::no) )
                    error("key must be of the same type as table<key,...>",  "", "",
                        expr->at, CompilationError::invalid_argument_type);
                expr->type = make_smart<TypeDecl>(Type::tBool);
            } else {
                error("first argument must be fully qualified table",  "", "",
                    expr->at, CompilationError::invalid_argument_type);
            }
            valueType->constant = true;
            return Visitor::visit(expr);
        }
    // ExprErase
        virtual ExpressionPtr visit ( ExprErase * expr ) override {
            if ( expr->argumentsFailedToInfer ) return Visitor::visit(expr);
            if ( expr->arguments.size()!=2 ) {
                error("eraseKey(table,key)",  "", "",
                    expr->at, CompilationError::invalid_argument_count);
                return Visitor::visit(expr);
            }
            // infer
            expr->arguments[1] = Expression::autoDereference(expr->arguments[1]);
            auto containerType = expr->arguments[0]->type;
            auto valueType = expr->arguments[1]->type;
            if ( containerType->isGoodTableType() ) {
                if ( !containerType->firstType->isSameType(*valueType,RefMatters::no, ConstMatters::no, TemporaryMatters::no) )
                    error("key must be of the same type as table<key,...>",  "", "",
                        expr->at, CompilationError::invalid_argument_type);
                expr->type = make_smart<TypeDecl>(Type::tBool);
            } else {
                error("first argument must be fully qualified table",  "", "",
                    expr->at, CompilationError::invalid_argument_type);
            }
            valueType->constant = true;
            return Visitor::visit(expr);
        }
    // ExprFind
        virtual ExpressionPtr visit ( ExprFind * expr ) override {
            if ( expr->argumentsFailedToInfer ) return Visitor::visit(expr);
            if ( expr->arguments.size()!=2 ) {
                error("findKey(table,key)",  "", "",
                    expr->at, CompilationError::invalid_argument_count);
                return Visitor::visit(expr);
            }
            // infer
            expr->arguments[1] = Expression::autoDereference(expr->arguments[1]);
            auto containerType = expr->arguments[0]->type;
            auto valueType = expr->arguments[1]->type;
            if ( containerType->isGoodTableType() ) {
                if ( !containerType->firstType->isSameType(*valueType,RefMatters::no, ConstMatters::no, TemporaryMatters::no) )
                    error("key must be of the same type as table<key,...>",  "", "",
                        expr->at, CompilationError::invalid_argument_type);
                expr->type = make_smart<TypeDecl>(Type::tPointer);
                expr->type->firstType = make_smart<TypeDecl>(*containerType->secondType);
            } else {
                error("first argument must be fully qualified table",  "", "",
                    expr->at, CompilationError::invalid_argument_type);
            }
            containerType->constant = true;
            valueType->constant = true;
            return Visitor::visit(expr);
        }
    // ExprKeyExists
        virtual ExpressionPtr visit ( ExprKeyExists * expr ) override {
            if ( expr->argumentsFailedToInfer ) return Visitor::visit(expr);
            if ( expr->arguments.size()!=2 ) {
                error("keyExists(table,key)",  "", "",
                    expr->at, CompilationError::invalid_argument_count);
                return Visitor::visit(expr);
            }
            // infer
            expr->arguments[1] = Expression::autoDereference(expr->arguments[1]);
            auto containerType = expr->arguments[0]->type;
            auto valueType = expr->arguments[1]->type;
            if ( containerType->isGoodTableType() ) {
                if ( !containerType->firstType->isSameType(*valueType,RefMatters::no, ConstMatters::no, TemporaryMatters::no) )
                    error("key must be of the same type as table<key,...>",  "", "",
                        expr->at, CompilationError::invalid_argument_type);
                expr->type = make_smart<TypeDecl>(Type::tBool);
            } else {
                error("first argument must be fully qualified table",  "", "",
                    expr->at, CompilationError::invalid_argument_type);
            }
            containerType->constant = true;
            valueType->constant = true;
            return Visitor::visit(expr);
        }
    // ExprIs
        virtual ExpressionPtr visit ( ExprIs * expr ) override {
            if ( expr->typeexpr->isExprType() ) {
                return Visitor::visit(expr);
            }
            if ( !expr->subexpr->type ) {
                return Visitor::visit(expr);
            }
            // TODO: verify
            if ( expr->typeexpr->isAutoOrAlias() ) {
                error("is " + (expr->typeexpr ? describeType(expr->typeexpr) : "...") + " can't be inferred", "", "",
                      expr->at, CompilationError::type_not_found);
                return Visitor::visit(expr);
            }
            auto nErrors = program->errors.size();
            if ( expr->typeexpr->isAlias() ) {
                if ( auto eT = inferAlias(expr->typeexpr) ) {
                    expr->typeexpr = eT;
                    reportAstChanged();
                    return Visitor::visit(expr);
                } else {
                    error("undefined is expression type " + describeType(expr->typeexpr),
                        reportInferAliasErrors(expr->typeexpr), "", expr->at, CompilationError::type_not_found);
                    return Visitor::visit(expr);
                }
            }
            if ( expr->typeexpr->isAuto() ) {
                error("is ... auto is undefined, " + describeType(expr->typeexpr), "", "",
                      expr->at, CompilationError::typeinfo_auto);
                return Visitor::visit(expr);
            }
            // TODO: verify, is this even necessary now that we have tests above?
            // if ( !isFullySealedType(expr->typeexpr) ) {
            if ( expr->typeexpr->isAutoOrAlias() ) {
                error("is " + (expr->typeexpr ? describeType(expr->typeexpr) : "...") + " can't be fully inferred", "", "",
                      expr->at, CompilationError::type_not_found);
                return Visitor::visit(expr);
            }
            verifyType(expr->typeexpr);
            if ( nErrors==program->errors.size() ) {
                reportAstChanged();
                bool isSame = expr->subexpr->type->isSameType(*expr->typeexpr, RefMatters::no, ConstMatters::no, TemporaryMatters::no);
                return make_smart<ExprConstBool>(expr->at, isSame);
            }
            // infer
            return Visitor::visit(expr);
        }


    // ExprTypeDecl
    virtual ExpressionPtr visit ( ExprTypeDecl * expr ) override {
        if ( expr->typeexpr->isExprType() ) {
            return Visitor::visit(expr);
        }
        if ( expr->typeexpr->isAlias() ) {
            if ( auto eT = inferAlias(expr->typeexpr) ) {
                eT->sanitize();
                expr->typeexpr = eT;
                reportAstChanged();
                return Visitor::visit(expr);
            } else {
                error("undefined type<" + describeType(expr->typeexpr)+">",
                    reportInferAliasErrors(expr->typeexpr), "", expr->at, CompilationError::type_not_found);
                return Visitor::visit(expr);
            }
        }
        if ( expr->typeexpr->isAutoOrAlias() ) {
            error("type<" +  describeType(expr->typeexpr) + "> can't be fully inferred",  "", "",
                expr->at, CompilationError::type_not_found);
            return Visitor::visit(expr);
        }
        verifyType(expr->typeexpr,true);
        expr->type = make_smart<TypeDecl>(*expr->typeexpr);
        return Visitor::visit(expr);
    }

    // ExprTypeInfo
        virtual ExpressionPtr visit ( ExprTypeInfo * expr ) override {
            expr->macro = nullptr;
            if ( expr->typeexpr && expr->typeexpr->isExprType() ) {
                return Visitor::visit(expr);
            }
            if ( expr->subexpr && expr->subexpr->type ) {
                expr->typeexpr = make_smart<TypeDecl>(*expr->subexpr->type);
                expr->typeexpr->ref = false;
            }
            // verify
            bool allowMissingTypeExpr = false;
            if ( expr->trait=="builtin_function_exists" || expr->trait=="builtin_module_exists" ) {
                allowMissingTypeExpr = true;
            }
            bool allowMissingType = false;
            if ( expr->trait=="builtin_annotation_exists" ) {
                allowMissingType = true;
            }
            //
            if ( !expr->typeexpr && !allowMissingTypeExpr ) {
                error("typeinfo(" + (expr->typeexpr ? describeType(expr->typeexpr) : "...") + ") can't be inferred",  "", "",
                    expr->at, CompilationError::type_not_found);
                return Visitor::visit(expr);
            }
            auto nErrors = program->errors.size();
            if ( expr->typeexpr ) {
                if ( expr->typeexpr->isAlias() ) {
                    if ( auto eT = inferAlias(expr->typeexpr) ) {
                        expr->typeexpr = eT;
                        eT->sanitize();
                        reportAstChanged();
                        return Visitor::visit(expr);
                    } else if ( !allowMissingType ) {
                        error("undefined typeinfo type expression type " + describeType(expr->typeexpr),
                            reportInferAliasErrors(expr->typeexpr), "", expr->at, CompilationError::type_not_found);
                        return Visitor::visit(expr);
                    }
                }
                if ( expr->typeexpr->isAuto() ) {
                    error("typeinfo(... auto) is undefined, " + describeType(expr->typeexpr), "", "",
                        expr->at, CompilationError::typeinfo_auto);
                    return Visitor::visit(expr);
                }
                if ( allowMissingType ? expr->typeexpr->isAuto() : !expr->typeexpr->isFullyInferred() ) {
                    error("typeinfo(" + (expr->typeexpr ? describeType(expr->typeexpr) : "...") + ") can't be fully inferred",  "", "",
                        expr->at, CompilationError::type_not_found);
                    return Visitor::visit(expr);
                }
                verifyType(expr->typeexpr,true);
            }
            if ( nErrors==program->errors.size() ) {
                if ( expr->trait=="sizeof" ) {
                    if ( expr->typeexpr->isExprTypeAnywhere() ) {
                        error("typeinfo(sizeof " + describeType(expr->typeexpr) + ") is not fully inferred, expecting resolved dim",  "", "",
                            expr->at, CompilationError::type_not_found);
                        return Visitor::visit(expr);
                    }
                    reportAstChanged();
                    uint64_t size = expr->typeexpr->getSizeOf64();
                    if ( size>0x7fffffff ) {
                        error("typeinfo(sizeof " + describeType(expr->typeexpr) + ") is too big",  "", "",
                            expr->at, CompilationError::invalid_type);
                        return Visitor::visit(expr);
                    }
                    return make_smart<ExprConstInt>(expr->at, int(size));
                } else if ( expr->trait=="alignof" ) {
                    if ( expr->typeexpr->isExprTypeAnywhere() ) {
                        error("typeinfo(alignof " + describeType(expr->typeexpr) + ") is not fully inferred, expecting resolved dim",  "", "",
                            expr->at, CompilationError::type_not_found);
                        return Visitor::visit(expr);
                    }
                    reportAstChanged();
                    return make_smart<ExprConstInt>(expr->at, expr->typeexpr->getAlignOf());
                } else if ( expr->trait=="is_dim" ) {
                    reportAstChanged();
                    return make_smart<ExprConstBool>(expr->at, expr->typeexpr->dim.size()!=0);
                } else if ( expr->trait=="dim" ) {
                    if ( expr->typeexpr->isExprTypeAnywhere() ) {
                        error("typeinfo(dim " + describeType(expr->typeexpr) + ") is not fully inferred, expecting resolved dim",  "", "",
                            expr->at, CompilationError::type_not_found);
                        return Visitor::visit(expr);
                    }
                    if ( expr->typeexpr->dim.size() ) {
                        reportAstChanged();
                        return make_smart<ExprConstInt>(expr->at, expr->typeexpr->dim[0]);
                    } else {
                        error("typeinfo(dim non_array) is prohibited, " + describeType(expr->typeexpr), "", "",
                              expr->at,CompilationError::typeinfo_dim);
                    }
                } else if ( expr->trait=="is_any_vector" ) {
                    reportAstChanged();
                    return make_smart<ExprConstBool>(expr->at, expr->typeexpr->isHandle() &&
                        expr->typeexpr->annotation && expr->typeexpr->annotation->isYetAnotherVectorTemplate());
                } else if ( expr->trait=="variant_index" || expr->trait=="safe_variant_index" ) {
                    if ( !expr->typeexpr->isGoodVariantType() ) {
                        if (expr->trait == "variant_index") {
                            error("variant_index only valid for variant, not for " + describeType(expr->typeexpr), "", "",
                                expr->at, CompilationError::invalid_type);
                        } else {
                            reportAstChanged();
                            return make_smart<ExprConstInt>(expr->at, -1);
                        }
                    } else {
                        int32_t index = expr->typeexpr->findArgumentIndex(expr->subtrait);
                        if ( index!=-1 ||  expr->trait=="safe_variant_index" ) {
                            reportAstChanged();
                            return make_smart<ExprConstInt>(expr->at, index);
                        } else {
                            error("variant_index variant " + expr->subtrait + " not found in " + describeType(expr->typeexpr), "", "",
                                expr->at,CompilationError::typeinfo_undefined);
                        }
                    }
                } else if ( expr->trait=="mangled_name" ) {
                    if ( !expr->subexpr ) {
                        error("mangled name requires subexpression", "", "",
                            expr->at,CompilationError::typeinfo_undefined);
                    } else {
                        if ( expr->subexpr->rtti_isAddr() ) {
                            auto eaddr = static_pointer_cast<ExprAddr>(expr->subexpr);
                            if ( !eaddr->func ) {
                                error("mangled name of unknown @@function", "", "",
                                    expr->at,CompilationError::typeinfo_undefined);
                            } else {
                                reportAstChanged();
                                return make_smart<ExprConstString>(expr->at, eaddr->func->getMangledName());
                            }
                        } else {
                            error("unsupported mangled name subexpression ", expr->subexpr->__rtti, "",
                                expr->at,CompilationError::typeinfo_undefined);
                        }
                    }
                } else if ( expr->trait=="is_argument" ) {
                    if ( !expr->subexpr ) {
                        error("is argument requires subexpression", "", "",
                            expr->at,CompilationError::typeinfo_undefined);
                    } else {
                        if ( expr->subexpr->rtti_isVar() ) {
                            auto evar = static_pointer_cast<ExprVar>(expr->subexpr);
                            reportAstChanged();
                            return make_smart<ExprConstBool>(expr->at, func->findArgument(evar->name) != nullptr);
                        } else {
                            reportAstChanged();
                            return make_smart<ExprConstBool>(expr->at, false);
                        }
                    }
                } else if ( expr->trait=="typename" ) {
                    reportAstChanged();
                    return make_smart<ExprConstString>(expr->at, expr->typeexpr->describe(TypeDecl::DescribeExtra::no, TypeDecl::DescribeContracts::no));
                } else if ( expr->trait=="undecorated_typename" ) {
                    reportAstChanged();
                    return make_smart<ExprConstString>(expr->at, expr->typeexpr->describe(TypeDecl::DescribeExtra::no, TypeDecl::DescribeContracts::no, TypeDecl::DescribeModule::no));
                } else if ( expr->trait=="stripped_typename" ) {
                    reportAstChanged();
                    auto ctype = make_smart<TypeDecl>(*expr->typeexpr);
                    ctype->constant = false; ctype->temporary = false; ctype->ref = false;
                    return make_smart<ExprConstString>(expr->at, ctype->describe(TypeDecl::DescribeExtra::no, TypeDecl::DescribeContracts::no, TypeDecl::DescribeModule::yes));
                } else if ( expr->trait=="fulltypename" ) {
                    reportAstChanged();
                    return make_smart<ExprConstString>(expr->at, expr->typeexpr->describe(TypeDecl::DescribeExtra::no, TypeDecl::DescribeContracts::yes));
                } else if ( expr->trait=="modulename" ) {
                    reportAstChanged();
                    auto modd = expr->typeexpr->module;
                    return make_smart<ExprConstString>(expr->at, modd ? modd->name : "");
                } else if ( expr->trait=="struct_modulename" ) {
                    if ( expr->typeexpr->isStructure() ) {
                        reportAstChanged();
                        auto modd = expr->typeexpr->structType->module;
                        return make_smart<ExprConstString>(expr->at, modd ? modd->name : "");
                    } else {
                        error("can't get struct_modulename of " + expr->typeexpr->describe(), "", "",
                            expr->at,CompilationError::typeinfo_undefined);
                    }
                } else if ( expr->trait=="is_pod" ) {
                    reportAstChanged();
                    return make_smart<ExprConstBool>(expr->at, expr->typeexpr->isPod());
                } else if ( expr->trait=="is_raw" ) {
                    reportAstChanged();
                    return make_smart<ExprConstBool>(expr->at, expr->typeexpr->isRawPod());
                } else if ( expr->trait=="is_struct" ) {
                    reportAstChanged();
                    return make_smart<ExprConstBool>(expr->at, expr->typeexpr->isStructure());
                } else if ( expr->trait=="is_tuple" ) {
                    reportAstChanged();
                    return make_smart<ExprConstBool>(expr->at, expr->typeexpr->isTuple());
                } else if ( expr->trait=="is_variant" ) {
                    reportAstChanged();
                    return make_smart<ExprConstBool>(expr->at, expr->typeexpr->isVariant());
                } else if ( expr->trait=="is_class" ) {
                    reportAstChanged();
                    return make_smart<ExprConstBool>(expr->at, expr->typeexpr->isClass());
                } else if ( expr->trait=="is_lambda" ) {
                    reportAstChanged();
                    return make_smart<ExprConstBool>(expr->at, expr->typeexpr->isLambda());
                } else if ( expr->trait=="is_enum" ) {
                    reportAstChanged();
                    return make_smart<ExprConstBool>(expr->at, expr->typeexpr->isEnum());
                } else if ( expr->trait=="is_bitfield" ) {
                    reportAstChanged();
                    return make_smart<ExprConstBool>(expr->at, expr->typeexpr->isBitfield());
                } else if (expr->trait == "is_string") {
                    reportAstChanged();
                    return make_smart<ExprConstBool>(expr->at, expr->typeexpr->isString());
                } else if ( expr->trait=="is_handle" ) {
                    reportAstChanged();
                    return make_smart<ExprConstBool>(expr->at, expr->typeexpr->isHandle());
                } else if ( expr->trait=="is_ref" ) {
                    reportAstChanged();
                    return make_smart<ExprConstBool>(expr->at, expr->typeexpr->isRef());
                } else if ( expr->trait=="is_ref_type" ) {
                    reportAstChanged();
                    return make_smart<ExprConstBool>(expr->at, expr->typeexpr->isRefType());
                } else if ( expr->trait=="is_ref_value" ) {
                    reportAstChanged();
                    return make_smart<ExprConstBool>(expr->at, bool(expr->typeexpr->ref));
                } else if ( expr->trait=="is_const" ) {
                    reportAstChanged();
                    return make_smart<ExprConstBool>(expr->at, expr->typeexpr->isConst());
                } else if ( expr->trait=="is_temp" ) {
                    reportAstChanged();
                    return make_smart<ExprConstBool>(expr->at, expr->typeexpr->isTemp());
                } else if ( expr->trait=="is_temp_type" ) {
                    reportAstChanged();
                    return make_smart<ExprConstBool>(expr->at, expr->typeexpr->isTempType());
                } else if ( expr->trait=="is_pointer" ) {
                    reportAstChanged();
                    return make_smart<ExprConstBool>(expr->at, expr->typeexpr->isPointer());
                } else if ( expr->trait=="is_smart_ptr" ) {
                    reportAstChanged();
                    return make_smart<ExprConstBool>(expr->at, expr->typeexpr->smartPtr && expr->typeexpr->isPointer());
                } else if ( expr->trait=="is_iterator" ) {
                    reportAstChanged();
                    return make_smart<ExprConstBool>(expr->at, expr->typeexpr->isGoodIteratorType());
                } else if ( expr->trait=="is_iterable" ) {
                    reportAstChanged();
                    bool iterable = false;
                    if ( expr->typeexpr->dim.size() ) {
                        iterable = true;
                    } else if ( expr->typeexpr->isGoodIteratorType() ) {
                        iterable = true;
                    } else if ( expr->typeexpr->isGoodArrayType() ) {
                        iterable = true;
                    } else if ( expr->typeexpr->isRange() ) {
                        iterable = true;
                    } else if ( expr->typeexpr->isString() ) {
                        iterable = true;
                    } else if ( expr->typeexpr->isHandle() && expr->typeexpr->annotation->isIterable() ) {
                        iterable = true;
                    }
                    return make_smart<ExprConstBool>(expr->at, iterable);
                } else if ( expr->trait=="is_vector" ) {
                    reportAstChanged();
                    return make_smart<ExprConstBool>(expr->at, expr->typeexpr->isVectorType());
                } else if ( expr->trait=="vector_dim" ) {
                    reportAstChanged();
                    return make_smart<ExprConstInt>(expr->at, expr->typeexpr->getVectorDim());
                } else if ( expr->trait=="is_array" ) {
                    reportAstChanged();
                    return make_smart<ExprConstBool>(expr->at, expr->typeexpr->isGoodArrayType());
                } else if ( expr->trait=="is_table" ) {
                    reportAstChanged();
                    return make_smart<ExprConstBool>(expr->at, expr->typeexpr->isGoodTableType());
                } else if ( expr->trait=="is_numeric" ) {
                    reportAstChanged();
                    return make_smart<ExprConstBool>(expr->at, expr->typeexpr->isNumeric());
                } else if ( expr->trait=="is_numeric_comparable" ) {
                    reportAstChanged();
                    return make_smart<ExprConstBool>(expr->at, expr->typeexpr->isNumericComparable());
                } else if ( expr->trait=="is_local" ) {
                    reportAstChanged();
                    return make_smart<ExprConstBool>(expr->at, expr->typeexpr->isLocal());
                } else if (expr->trait == "is_function") {
                     reportAstChanged();
                     return make_smart<ExprConstBool>(expr->at, expr->typeexpr->isFunction());
                } else if (expr->trait == "is_void") {
                     reportAstChanged();
                     return make_smart<ExprConstBool>(expr->at, expr->typeexpr->isVoid());
                } else if (expr->trait == "is_void_pointer") {
                     reportAstChanged();
                     return make_smart<ExprConstBool>(expr->at, expr->typeexpr->isVoidPointer());
                } else if ( expr->trait=="need_inscope" ) {
                    reportAstChanged();
                    return make_smart<ExprConstBool>(expr->at, expr->typeexpr->needInScope());
                } else if ( expr->trait=="can_be_placed_in_container" ) {
                    reportAstChanged();
                    return make_smart<ExprConstBool>(expr->at, expr->typeexpr->canBePlacedInContainer());
                } else if ( expr->trait=="can_copy" ) {
                    reportAstChanged();
                    return make_smart<ExprConstBool>(expr->at, expr->typeexpr->canCopy());
                } else if ( expr->trait=="can_move" ) {
                    reportAstChanged();
                    return make_smart<ExprConstBool>(expr->at, expr->typeexpr->canMove());
                } else if ( expr->trait=="can_clone" ) {
                    reportAstChanged();
                    return make_smart<ExprConstBool>(expr->at, expr->typeexpr->canClone());
                } else if ( expr->trait=="can_new" ) {
                    reportAstChanged();
                    return make_smart<ExprConstBool>(expr->at, expr->typeexpr->canNew());
                } else if ( expr->trait=="can_delete" ) {
                    reportAstChanged();
                    return make_smart<ExprConstBool>(expr->at, expr->typeexpr->canDelete());
                } else if ( expr->trait=="can_delete_ptr" ) {
                    reportAstChanged();
                    return make_smart<ExprConstBool>(expr->at, expr->typeexpr->canDeletePtr());
                } else if ( expr->trait=="need_delete" ) {
                    reportAstChanged();
                    return make_smart<ExprConstBool>(expr->at, expr->typeexpr->needDelete());
                } else if ( expr->trait=="has_nontrivial_ctor" ) {
                    reportAstChanged();
                    return make_smart<ExprConstBool>(expr->at, expr->typeexpr->hasNonTrivialCtor());
                } else if ( expr->trait=="has_nontrivial_dtor" ) {
                    reportAstChanged();
                    return make_smart<ExprConstBool>(expr->at, expr->typeexpr->hasNonTrivialDtor());
                } else if ( expr->trait=="has_nontrivial_copy" ) {
                    reportAstChanged();
                    return make_smart<ExprConstBool>(expr->at, expr->typeexpr->hasNonTrivialCopy());
                } else if ( expr->trait=="need_lock_check" ) {
                    reportAstChanged();
                    return make_smart<ExprConstBool>(expr->at,((func && func->skipLockCheck) || skipModuleLockChecks) ? false : expr->typeexpr->lockCheck());
                } else if ( expr->trait=="has_field" || expr->trait=="safe_has_field" ) {
                    auto etype = expr->typeexpr;
                    if ( etype->isPointer() && etype->firstType ) etype = etype->firstType;
                    if ( etype->isStructure() ) {
                        reportAstChanged();
                        return make_smart<ExprConstBool>(expr->at, etype->structType->findField(expr->subtrait));
                    } else if ( etype->isHandle() ) {
                        reportAstChanged();
                        auto ft =etype->annotation->makeFieldType(expr->subtrait, false);
                        return make_smart<ExprConstBool>(expr->at, ft!=nullptr);
                    } else {
                        if ( expr->trait=="safe_has_field" ) {
                            return make_smart<ExprConstBool>(expr->at, false);
                        } else {
                            error("typeinfo(has_field<" + expr->subtrait
                                  + "> ...) is only defined for structures and handled types, "
                                    + describeType(expr->typeexpr),  "", "",
                                expr->at, CompilationError::typeinfo_undefined);
                        }
                    }
                } else if ( expr->trait=="struct_has_annotation" || expr->trait=="struct_safe_has_annotation" ) {
                    if ( expr->typeexpr->isStructure() ) {
                        reportAstChanged();
                        const auto & ann = expr->typeexpr->structType->annotations;
                        auto it = find_if ( ann.begin(), ann.end(), [&](const AnnotationDeclarationPtr & pa){
                            return pa->annotation->name == expr->subtrait;
                        });
                        return make_smart<ExprConstBool>(expr->at, it!=ann.end());
                    } else {
                        if ( expr->trait=="struct_safe_has_annotation" ) {
                            return make_smart<ExprConstBool>(expr->at, false);
                        } else {
                            error("typeinfo(struct_has_annotation<" + expr->subtrait
                                  + "> ...) is only defined for structures, "
                                    + describeType(expr->typeexpr), "", "",
                                expr->at, CompilationError::typeinfo_undefined);
                        }
                    }
                } else if ( expr->trait=="struct_has_annotation_argument" || expr->trait=="struct_safe_has_annotation_argument" ) {
                    if ( expr->typeexpr->isStructure() ) {
                        const auto & ann = expr->typeexpr->structType->annotations;
                        auto it = find_if ( ann.begin(), ann.end(), [&](const AnnotationDeclarationPtr & pa){
                            return pa->annotation->name == expr->subtrait;
                        });
                        if ( it == ann.end() ) {
                            if ( expr->trait=="struct_safe_has_annotation_argument" ) {
                                return make_smart<ExprConstBool>(expr->at, false);
                            } else {
                                error("typeinfo(struct_has_annotation_argument<" + expr->subtrait
                                      + ";"  + expr->extratrait + "> ...) annotation not found ", "", "",
                                      expr->at, CompilationError::typeinfo_undefined);
                            }
                        } else {
                            reportAstChanged();
                            const auto & args = (*it)->arguments;
                            auto ita = find_if ( args.begin(), args.end(), [&](const AnnotationArgument & arg){
                                return arg.name == expr->extratrait;
                            });
                            return make_smart<ExprConstBool>(expr->at, ita!=args.end());
                        }
                    } else {
                        if ( expr->trait=="struct_safe_has_annotation_argument" ) {
                            return make_smart<ExprConstBool>(expr->at, false);
                        } else {
                            error("typeinfo(struct_has_annotation_argument<" + expr->subtrait
                                  + "> ...) is only defined for structures, "
                                    + describeType(expr->typeexpr),  "", "",
                                expr->at, CompilationError::typeinfo_undefined);
                        }
                    }
                } else if ( expr->trait=="struct_get_annotation_argument" ) {
                    if ( expr->typeexpr->isStructure() ) {
                        const auto & ann = expr->typeexpr->structType->annotations;
                        auto it = find_if ( ann.begin(), ann.end(), [&](const AnnotationDeclarationPtr & pa){
                            return pa->annotation->name == expr->subtrait;
                        });
                        if ( it == ann.end() ) {
                                error("typeinfo(struct_get_annotation_argument<" + expr->subtrait
                                      + ";"  + expr->extratrait + "> ...) annotation not found ", "", "",
                                  expr->at, CompilationError::typeinfo_undefined);
                        } else {
                            const auto & args = (*it)->arguments;
                            auto ita = find_if ( args.begin(), args.end(), [&](const AnnotationArgument & arg){
                                return arg.name == expr->extratrait;
                            });
                            if ( ita == args.end() ) {
                                error("typeinfo(struct_get_annotation_argument<" + expr->subtrait
                                      + ";"  + expr->extratrait + "> ...) annotation argument not found ", "", "",
                                  expr->at, CompilationError::typeinfo_undefined);
                            } else {
                                switch ( ita->type ) {
                                    case Type::tBool:
                                        reportAstChanged();
                                        return make_smart<ExprConstBool>(expr->at, ita->bValue);
                                    case Type::tInt:
                                        reportAstChanged();
                                        return make_smart<ExprConstInt>(expr->at, ita->iValue);
                                    case Type::tFloat:
                                        reportAstChanged();
                                        return make_smart<ExprConstFloat>(expr->at, ita->fValue);
                                    case Type::tString:
                                        reportAstChanged();
                                        return make_smart<ExprConstString>(expr->at, ita->sValue);
                                    default:
                                        error("typeinfo(struct_get_annotation_argument<" + expr->subtrait
                                              + ";"  + expr->extratrait + "> ...) unsupported annotation argument type ", "", "",
                                          expr->at, CompilationError::typeinfo_undefined);
                                }
                            }
                        }
                    } else {
                        error("typeinfo(struct_get_annotation_argument<" + expr->subtrait
                              + "> ...) is only defined for structures, "
                                + describeType(expr->typeexpr), "", "",
                            expr->at, CompilationError::typeinfo_undefined);
                    }
                } else if ( expr->trait=="offsetof" ) {
                    if ( expr->typeexpr->isStructure() ) {
                        auto decl = expr->typeexpr->structType->findField(expr->subtrait);
                        // NOTE: we do need to check if its fully sealed here
                        if ( isFullySealedType(expr->typeexpr) ) {
                            reportAstChanged();
                            return make_smart<ExprConstInt>(expr->at, decl->offset);
                        } else {
                            error("typeinfo(offsetof<" + expr->subtrait
                                  + "> ...) of undefined type, " + describeType(expr->typeexpr), "", "",
                                expr->at, CompilationError::typeinfo_undefined);
                        }
                    } else {
                        error("typeinfo(offsetof<" + expr->subtrait
                              + "> ...) is only defined for structures, " + describeType(expr->typeexpr), "", "",
                            expr->at, CompilationError::typeinfo_undefined);
                    }
                } else if ( expr->trait=="builtin_function_exists" ) {
                    if ( !expr->subexpr ) {
                        error("builtin_function_exists requires subexpression", "", "",
                            expr->at,CompilationError::typeinfo_undefined);
                    } else {
                        if ( expr->subexpr->rtti_isAddr() ) {
                            auto eaddr = static_pointer_cast<ExprAddr>(expr->subexpr);
                            if ( !eaddr->func ) {
                                reportAstChanged();
                                return make_smart<ExprConstBool>(false);
                            } else if ( !eaddr->func->builtIn ) {
                                error("builtin_function_exists of non-builtin function @@" + describeFunction(eaddr->func), "", "",
                                    expr->at,CompilationError::typeinfo_undefined);
                            } else {
                                reportAstChanged();
                                return make_smart<ExprConstBool>(true);
                            }
                        } else {
                            error("unsupported mangled name subexpression ", expr->subexpr->__rtti, "",
                                expr->at,CompilationError::typeinfo_undefined);
                        }
                    }
                } else if ( expr->trait=="builtin_module_exists" ) {
                    if ( !expr->subexpr ) {
                        error("builtin_module_exists requires subexpression", "", "",
                            expr->at,CompilationError::typeinfo_undefined);
                    } else {
                        if ( expr->subexpr->rtti_isVar() ) {
                            auto evar = static_pointer_cast<ExprVar>(expr->subexpr);
                            auto mod = Module::requireEx(evar->name, false);
                            reportAstChanged();
                            return make_smart<ExprConstBool>(mod!=nullptr);
                        } else {
                            error("unsupported module name subexpression ", expr->subexpr->__rtti, "",
                                expr->at,CompilationError::typeinfo_undefined);
                        }
                    }
                } else if ( expr->trait=="builtin_annotation_exists" ) {
                    if ( expr->typeexpr->isAlias() ) {
                        reportAstChanged();
                        return make_smart<ExprConstBool>(false);
                    } else if ( !expr->typeexpr->isHandle() ) {
                        error("builtin_function_exists requires annotation type", "", "",
                            expr->at,CompilationError::typeinfo_undefined);
                    } else {
                        reportAstChanged();
                        return make_smart<ExprConstBool>(true);
                    }
                } else {
                    auto mtis = program->findTypeInfoMacro(expr->trait);
                    if ( mtis.size()>1 ) {
                        error("typeinfo(" + expr->trait + " ...) too many macros match the trait", "", "",
                            expr->at, CompilationError::typeinfo_undefined);
                    } else if ( mtis.empty() ) {
                        error("typeinfo(" + expr->trait + " ...) is undefined, " + describeType(expr->typeexpr), "", "",
                            expr->at, CompilationError::typeinfo_undefined);
                    } else {
                        expr->macro = mtis.back().get();
                        string errors;
                        auto cexpr = expr->macro->getAstChange(expr, errors);
                        if ( cexpr ) {
                            reportAstChanged();
                            return cexpr;
                        } else if ( !errors.empty() ) {
                            error("typeinfo(" + expr->trait + " ...) macro reported error; " + errors, "", "",
                                expr->at, CompilationError::typeinfo_macro_error);
                        } else {
                            auto ctype = expr->macro->getAstType(program->library, expr, errors);
                            if ( ctype ) {
                                expr->type = ctype;
                                if ( func && expr->macro->noAot(expr) ) {
                                    func->noAot = true;
                                }
                                return Visitor::visit(expr);
                            } else if ( !errors.empty() ) {
                                error("typeinfo(" + expr->trait + " ...) macro reported error; " + errors, "", "",
                                    expr->at, CompilationError::typeinfo_macro_error);
                            } else {
                                error("typeinfo(" + expr->trait + " ...) is macro integration error; no ast change and no type", "", "",
                                    expr->at, CompilationError::typeinfo_macro_error);
                            }
                        }
                    }
                }
            }
            // infer
            return Visitor::visit(expr);
        }
    // ExprDelete
        void reportMissingFinalizer ( const string & message, const LineInfo & at, const TypeDeclPtr & ftype ) {
            if ( verbose ) {
                auto fakeCall = make_smart<ExprCall>(at, "_::finalize");
                auto fakeVar = make_smart<ExprVar>(at, "this");
                fakeVar->type = make_smart<TypeDecl>(*ftype);
                fakeCall->arguments.push_back(fakeVar);
                vector<TypeDeclPtr> fakeTypes = { ftype };
                reportMissing(fakeCall.get(), fakeTypes, message, true, CompilationError::function_already_declared);
            } else {
                error(message, "", "", at, CompilationError::function_already_declared);
            }
        }
        virtual ExpressionPtr visit ( ExprDelete * expr ) override {
            if ( !expr->subexpr->type ) return Visitor::visit(expr);
            if ( expr->sizeexpr && !expr->sizeexpr->type ) return Visitor::visit(expr);
            // lets see if there is clone operator already (a user operator can ignore all the rules bellow)
            if ( !expr->native ) {
                auto fnList = getFinalizeFunc(expr->subexpr->type);
                if ( fnList.size() ) {
                    if ( verifyFinalizeFunc(fnList, expr->at) ) {
                        reportAstChanged();
                        // auto fn = fnList[0];
                        // string finalizeName = (fn->module->name.empty() ? "_" : fn->module->name) + "::finalize";
                        string finalizeName = "_::finalize";
                        auto finalizeFn = make_smart<ExprCall>(expr->at, finalizeName);
                        finalizeFn->arguments.push_back(expr->subexpr->clone());
                        return ExpressionPtr(finalizeFn);
                    } else {
                        return Visitor::visit(expr);
                    }
                }
            }
            // size
            if ( expr->sizeexpr ) {
                expr->sizeexpr = Expression::autoDereference(expr->sizeexpr);
                if ( !expr->sizeexpr->type->isSimpleType(Type::tInt) ) {
                    error("can't delete, expecting size to be int and not " + describeType(expr->sizeexpr->type), "", "",
                      expr->at, CompilationError::bad_delete);
                }
            }
            // strict unsafe delete
            if ( strictUnsafeDelete && !safeExpression(expr) && !expr->subexpr->type->isSafeToDelete() ) {
                error("delete of " + expr->subexpr->type->describe() + " requires unsafe",  "", "",
                    expr->at, CompilationError::unsafe);
                return Visitor::visit(expr);
            }
            // infer
            if ( !expr->subexpr->type->canDelete() ) {
                expr->subexpr->type->canDelete();
                error("can't delete " + describeType(expr->subexpr->type), "", "",
                      expr->at, CompilationError::bad_delete);
            } else if ( !expr->subexpr->type->isRef() ) {
                error("can only delete reference " + describeType(expr->subexpr->type), "", "",
                      expr->at, CompilationError::bad_delete);
            } else if ( expr->subexpr->type->isConst() ) {
                error("can't delete constant expression " + describeType(expr->subexpr->type), "", "",
                      expr->at, CompilationError::bad_delete);
            } else if ( expr->subexpr->type->isPointer() ) {
                if ( !safeExpression(expr) && !(expr->subexpr->type->smartPtr || expr->subexpr->type->smartPtrNative) ) {
                    error("delete of pointer requires unsafe",  "", "",
                        expr->at, CompilationError::unsafe);
                } else if ( expr->subexpr->type->firstType && expr->subexpr->type->firstType->isHandle() &&
                    expr->subexpr->type->firstType->annotation->isSmart() && !expr->subexpr->type->smartPtr ) {
                        error("can't delete smart pointer type via regular pointer delete",  "", "",
                            expr->at, CompilationError::bad_delete);
                } else if ( !expr->native ) {
                    if ( expr->subexpr->type->firstType && expr->subexpr->type->firstType->needDelete() ) {
                        auto ptrf = getFinalizeFunc(expr->subexpr->type);
                        if ( ptrf.size()==0 ) {
                            auto fnDel = generatePointerFinalizer(expr->subexpr->type, expr->at);
                            if ( !strictUnsafeDelete && !expr->alwaysSafe ) fnDel->unsafeOperation = true;
                            if ( !program->addFunction(fnDel) ) {
                                reportMissingFinalizer("finalizer mismatch ", expr->at, expr->subexpr->type);
                                return Visitor::visit(expr);
                            }
                        } else if ( ptrf.size() > 1 ) {
                            string candidates = verbose ? program->describeCandidates(ptrf) : "";
                            error("too many finalizers", candidates, "",
                                expr->at, CompilationError::function_already_declared);
                            return Visitor::visit(expr);
                        }
                        reportAstChanged();
                        expr->native = true;
                        auto cloneFn = make_smart<ExprCall>(expr->at, "_::finalize");
                        cloneFn->arguments.push_back(expr->subexpr->clone());
                        return ExpressionPtr(cloneFn);
                    } else {
                        reportAstChanged();
                        expr->native = true;
                    }
                }
            } else {
                auto finalizeType = expr->subexpr->type;
                if ( finalizeType->isGoodIteratorType() ) {
                    reportAstChanged();
                    auto cloneFn = make_smart<ExprCall>(expr->at, "_builtin_iterator_delete");
                    cloneFn->arguments.push_back(expr->subexpr->clone());
                    return ExpressionPtr(cloneFn);
                } else if ( finalizeType->isGoodArrayType() || finalizeType->isGoodTableType() ) {
                    reportAstChanged();
                    auto cloneFn = make_smart<ExprCall>(expr->at, "_::finalize");
                    cloneFn->arguments.push_back(expr->subexpr->clone());
                    return ExpressionPtr(cloneFn);
                } else if ( finalizeType->isStructure() ) {
                    auto fnDel = generateStructureFinalizer(finalizeType->structType);
                    if ( program->addFunction(fnDel) ) {
                        reportAstChanged();
                        auto cloneFn = make_smart<ExprCall>(expr->at, "_::finalize");
                        cloneFn->arguments.push_back(expr->subexpr->clone());
                        return ExpressionPtr(cloneFn);
                    } else {
                        reportMissingFinalizer("finalizer mismatch ", expr->at, expr->subexpr->type);
                        return Visitor::visit(expr);
                    }
                } else if ( finalizeType->isTuple() ) {
                    auto fnDel = generateTupleFinalizer(expr->at, finalizeType);
                    if ( program->addFunction(fnDel) ) {
                        reportAstChanged();
                        auto cloneFn = make_smart<ExprCall>(expr->at, "_::finalize");
                        cloneFn->arguments.push_back(expr->subexpr->clone());
                        return ExpressionPtr(cloneFn);
                    } else {
                        reportMissingFinalizer("finalizer mismatch ", expr->at, expr->subexpr->type);
                        return Visitor::visit(expr);
                    }
                } else if ( finalizeType->isVariant() ) {
                    auto fnDel = generateVariantFinalizer(expr->at, finalizeType);
                    if ( program->addFunction(fnDel) ) {
                        reportAstChanged();
                        auto cloneFn = make_smart<ExprCall>(expr->at, "_::finalize");
                        cloneFn->arguments.push_back(expr->subexpr->clone());
                        return ExpressionPtr(cloneFn);
                    } else {
                        reportMissingFinalizer("finalizer mismatch ", expr->at, expr->subexpr->type);
                        return Visitor::visit(expr);
                    }
                } else if ( finalizeType->dim.size() ) {
                    reportAstChanged();
                    auto cloneFn = make_smart<ExprCall>(expr->at, "finalize_dim");
                    cloneFn->arguments.push_back(expr->subexpr->clone());
                    return ExpressionPtr(cloneFn);
                } else {
                    expr->type = make_smart<TypeDecl>();
                    return Visitor::visit(expr);
                }
            }
            return Visitor::visit(expr);
        }
    // ExprCast
        TypeDeclPtr castStruct ( const LineInfo & at, const TypeDeclPtr & subexprType, const TypeDeclPtr & castType, bool upcast ) const {
            auto seT = subexprType;
            auto cT = castType;
            if ( seT->isStructure() ) {
                if ( cT->isStructure() ) {
                    bool compatibleCast;
                    if ( upcast ) {
                        compatibleCast = seT->structType->isCompatibleCast(*cT->structType);
                    } else {
                        compatibleCast = cT->structType->isCompatibleCast(*seT->structType);
                    }
                    if ( compatibleCast ) {
                        auto exprType = make_smart<TypeDecl>(*cT);
                        exprType->ref = seT->ref;
                        exprType->constant = seT->constant;
                        return exprType;
                    } else {
                        error("incompatible cast, can't cast " + seT->structType->name
                              + " to " + cT->structType->name,  "", "",
                            at, CompilationError::invalid_cast);
                    }
                } else {
                    error("invalid cast, expecting structure",  "", "",
                        at, CompilationError::invalid_cast);
                }
            } else if ( seT->isPointer() && seT->firstType && seT->firstType->isStructure() ) {
                if ( cT->isPointer() && cT->firstType->isStructure() ) {
                    bool compatibleCast;
                    if ( upcast ) {
                        compatibleCast = seT->firstType->structType->isCompatibleCast(*cT->firstType->structType);
                    } else {
                        compatibleCast = cT->firstType->structType->isCompatibleCast(*seT->firstType->structType);
                    }
                    if ( compatibleCast ) {
                        auto exprType = make_smart<TypeDecl>(*cT);
                        exprType->ref = seT->ref;
                        exprType->constant = seT->constant;
                        return exprType;
                    } else {
                        error("incompatible cast, can't cast " + seT->firstType->structType->name
                              + "? to " + cT->firstType->structType->name + "?",  "", "",
                            at, CompilationError::invalid_cast);
                    }
                } else {
                    error("invalid cast, expecting structure pointer",  "", "",
                        at, CompilationError::invalid_cast);
                }
            }
            return nullptr;
        }
        TypeDeclPtr castFunc ( const LineInfo & at, const TypeDeclPtr & subexprType, const TypeDeclPtr & castType, bool upcast ) const {
            auto seTF = subexprType;
            auto cTF = castType;
            if ( seTF->argTypes.size()!=cTF->argTypes.size() ) {
                error("invalid cast, number of arguments does not match",  "", "",
                    at, CompilationError::invalid_cast);
                return nullptr;
            }
            // result
            auto funT = make_smart<TypeDecl>(*seTF);
            auto cresT = cTF->firstType;
            auto resT = funT->firstType;
            if ( !cresT->isSameType(*resT,RefMatters::yes, ConstMatters::no, TemporaryMatters::no) ) {
                if ( resT->isStructure() || (resT->isPointer() && resT->firstType && resT->firstType->isStructure()) ) {
                    auto tryRes = castStruct(at, resT, cresT, upcast);
                    if ( tryRes ) {
                        funT->firstType = tryRes;
                    }
                }
            }
            funT->firstType->constant = cresT->constant;
            // arguments
            for ( size_t i=0, is=seTF->argTypes.size(); i!=is; ++i ) {
                auto seT = seTF->argTypes[i];
                auto cT = cTF->argTypes[i];
                if ( !cT->isSameType(*seT,RefMatters::yes, ConstMatters::no, TemporaryMatters::no) ) {
                    if ( seT->isStructure() || (seT->isPointer() && seT->firstType->isStructure()) ) {
                        auto tryArg = castStruct(at, seT, cT, upcast);
                        if ( tryArg ) {
                            funT->argTypes[i] = tryArg;
                        }
                    }
                }
                funT->argTypes[i]->constant = cT->constant;
            }
            if ( castType->isSameType(*funT, RefMatters::yes, ConstMatters::yes, TemporaryMatters::yes) ) {
                return funT;
            } else {
                error("incompatible cast, can't cast " + describeType(funT) + " to " + describeType(castType),  "", "",
                    at, CompilationError::invalid_cast);
                return nullptr;
            }
        }

        virtual ExpressionPtr visit ( ExprCast * expr ) override {
            if ( !expr->subexpr->type ) return Visitor::visit(expr);
            if ( expr->castType && expr->castType->isExprType() ) {
                return Visitor::visit(expr);
            }
            if ( expr->castType->isAlias() ) {
                auto aT = inferAlias(expr->castType);
                if ( aT ) {
                    expr->castType = aT;
                    expr->castType->sanitize();
                    reportAstChanged();
                } else {
                    error("undefined cast type " + describeType(expr->castType),
                        reportInferAliasErrors(expr->castType), "", expr->at, CompilationError::type_not_found);
                    return Visitor::visit(expr);
                }
            }
            if ( expr->castType->isAuto() ) {
                error("casting to undefined type " + describeType(expr->castType),  "", "",
                    expr->at, CompilationError::type_not_found);
                return Visitor::visit(expr);
            }
            if ( expr->subexpr->type->isSameExactType(*expr->castType) ) {
                reportAstChanged();
                return expr->subexpr;
            }
            if ( expr->reinterpret ) {
                expr->type = make_smart<TypeDecl>(*expr->castType);
                expr->type->ref = expr->subexpr->type->ref;
            } else if ( expr->castType->isGoodBlockType() ||  expr->castType->isGoodFunctionType() || expr->castType->isGoodLambdaType() ) {
                expr->type = castFunc(expr->at, expr->subexpr->type, expr->castType, expr->upcast);
            } else if ( expr->castType->isHandle() ) {
                if ( expr->castType->isSameType(*expr->subexpr->type, RefMatters::yes, ConstMatters::yes, TemporaryMatters::yes, AllowSubstitute::yes) ) {
                    expr->type = make_smart<TypeDecl>(*expr->castType);
                    expr->type->ref = expr->subexpr->type->ref;
                } else {
                    expr->type = nullptr;
                }
            } else {
                expr->type = castStruct(expr->at, expr->subexpr->type, expr->castType, expr->upcast);
            }
            if ( expr->upcast || expr->reinterpret ) {
                if ( !safeExpression(expr) ) {
                    error("cast requires unsafe",  "", "",
                        expr->at, CompilationError::unsafe);
                }
            }
            if ( expr->type ) {
                verifyType(expr->type);
            } else {
                error("can't cast " + describeType(expr->subexpr->type) + " to " + describeType(expr->castType), "", "",
                      expr->at, CompilationError::type_not_found);
                return Visitor::visit(expr);
            }
            return Visitor::visit(expr);
        }
    // ExprAscend
        virtual ExpressionPtr visit ( ExprAscend * expr ) override {
            if ( !expr->subexpr->type ) return Visitor::visit(expr);
            if ( expr->subexpr->type->baseType==Type::tHandle && expr->subexpr->rtti_isMakeStruct() ) {
                auto mks = static_pointer_cast<ExprMakeStruct>(expr->subexpr);
                if ( !mks->isNewHandle ) {
                    reportAstChanged();
                    mks->isNewHandle = true;
                }
            }
            if ( !expr->subexpr->type->isRef() ) {
                error("can't ascend (to heap) non-reference value",  "", "",
                    expr->at, CompilationError::invalid_new_type);
            } else if ( expr->subexpr->type->baseType==Type::tHandle ) {
                const auto & subt = expr->subexpr->type;
                if ( !subt->dim.empty() ) {
                    error("can't new [] of handled type, " + describeType(subt),  "", "",
                        expr->at, CompilationError::invalid_new_type);
                }
                if ( !subt->annotation->canNew() ) {
                    error("can't new this handled type at all, " + describeType(subt),  "", "",
                        expr->at, CompilationError::invalid_new_type);
                }
            }
            if ( expr->ascType ) {
                expr->type = make_smart<TypeDecl>(*expr->ascType);
            } else {
                expr->type = make_smart<TypeDecl>(Type::tPointer);
                expr->type->firstType = make_smart<TypeDecl>(*expr->subexpr->type);
                expr->type->firstType->ref = false;
                if ( expr->type->firstType->baseType==Type::tHandle ) {
                    expr->type->smartPtr = expr->type->firstType->annotation->isSmart();
                }
            }
            return Visitor::visit(expr);
        }
    // ExprNew
        virtual void preVisit ( ExprNew * call ) override {
            Visitor::preVisit(call);
            call->argumentsFailedToInfer = false;
            if ( call->func && call->func->arguments.size() > DAS_MAX_FUNCTION_ARGUMENTS ) {
                error("too many arguments in new, max allowed is DAS_MAX_FUNCTION_ARGUMENTS=" DAS_STR(DAS_MAX_FUNCTION_ARGUMENTS),  "", "",
                    call->at, CompilationError::too_many_arguments);
            }
        }
        virtual void preVisitNewArg ( ExprNew * call, Expression * arg, bool last ) override {
            Visitor::preVisitNewArg(call, arg, last);
            arg->isCallArgument = true;
        }
        virtual ExpressionPtr visitNewArg ( ExprNew * call, Expression * arg , bool last ) override {
            if ( !arg->type ) call->argumentsFailedToInfer = true;
            if ( arg->type && arg->type->isAliasOrExpr() ) call->argumentsFailedToInfer = true;
            return Visitor::visitNewArg(call, arg, last);
        }
        virtual ExpressionPtr visit ( ExprNew * expr ) override {
            if ( expr->argumentsFailedToInfer ) return Visitor::visit(expr);
            if ( !expr->typeexpr ) {
                error("new type did not infer", "", "",
                    expr->at, CompilationError::type_not_found);
                return Visitor::visit(expr);
            }
            if ( expr->typeexpr && expr->typeexpr->isExprType() ) {
                return Visitor::visit(expr);
            }
            // infer
            if ( expr->typeexpr->isAlias() ) {
                if ( auto aT = findAlias(expr->typeexpr->alias) ) {
                    expr->typeexpr = make_smart<TypeDecl>(*aT);
                    expr->typeexpr->ref = false;        // drop a ref
                    expr->typeexpr->constant = false;   // drop a const
                    expr->typeexpr->sanitize();
                    reportAstChanged();
                } else {
                    error("undefined new expression type " + describeType(expr->typeexpr), "", "",
                          expr->at, CompilationError::type_not_found);
                    return Visitor::visit(expr);
                }
            }
            expr->name.clear();
            expr->func = nullptr;
            if ( expr->typeexpr->ref ) {
                error("can't new a reference",  "", "",
                    expr->at, CompilationError::invalid_new_type);
            } else if ( expr->typeexpr->baseType==Type::tStructure ) {
                if ( !expr->initializer && expr->typeexpr->structType->isClass ) {
                    error("new of class requires () syntax; " + describeType(expr->typeexpr), "", "",
                        expr->at, CompilationError::invalid_new_type);
                }
                expr->type = make_smart<TypeDecl>(Type::tPointer);
                expr->type->firstType = make_smart<TypeDecl>(*expr->typeexpr);
                expr->type->firstType->dim.clear();
                expr->type->dim = expr->typeexpr->dim;
                expr->name = expr->typeexpr->structType->getMangledName();
            } else if ( expr->typeexpr->baseType==Type::tHandle ) {
                if ( expr->typeexpr->annotation->canNew() ) {
                    expr->type = make_smart<TypeDecl>(Type::tPointer);
                    expr->type->firstType = make_smart<TypeDecl>(*expr->typeexpr);
                    expr->type->firstType->dim.clear();
                    expr->type->dim = expr->typeexpr->dim;
                    expr->type->smartPtr = expr->typeexpr->annotation->isSmart();
                    expr->name =  expr->typeexpr->annotation->module->name + "::" + expr->typeexpr->annotation->name;
                } else {
                    error("can't new this type " + describeType(expr->typeexpr), "", "",
                          expr->at, CompilationError::invalid_new_type);
                }
            } else if ( expr->typeexpr->baseType==Type::tTuple ) {
                expr->type = make_smart<TypeDecl>(Type::tPointer);
                expr->type->firstType = make_smart<TypeDecl>(*expr->typeexpr);
                expr->type->firstType->dim.clear();
                expr->type->dim = expr->typeexpr->dim;
                expr->name = expr->typeexpr->getMangledName();
            } else if ( expr->typeexpr->baseType==Type::tVariant ) {
                expr->type = make_smart<TypeDecl>(Type::tPointer);
                expr->type->firstType = make_smart<TypeDecl>(*expr->typeexpr);
                expr->type->firstType->dim.clear();
                expr->type->dim = expr->typeexpr->dim;
                expr->name = expr->typeexpr->getMangledName();
            } else {
                error("can only new tuples, variants, structures or handled types, not " + describeType(expr->typeexpr), "", "",
                      expr->at, CompilationError::invalid_new_type);
            }
            if ( expr->initializer && expr->name.empty() ) {
                error("only native structures can have initializers, not " + describeType(expr->typeexpr), "", "",
                      expr->at, CompilationError::invalid_new_type);
            }
            if ( expr->type && expr->initializer && !expr->name.empty() ) {
                auto resultType = expr->type;
                expr->func = inferFunctionCall(expr).get();
                swap ( resultType, expr->type );
                if ( expr->func ) {
                    if ( !expr->type->firstType->isSameType(*resultType, RefMatters::yes, ConstMatters::yes, TemporaryMatters::yes) ) {
                        error("initializer returns " + describeType(resultType) + " vs "
                            +  describeType(expr->type->firstType), "", "",
                              expr->at, CompilationError::invalid_new_type);
                    }
                }
                else {
                    error(describeType(expr->type->firstType) + " does not have default initializer", "", "",
                        expr->at, CompilationError::invalid_new_type);
                }
            }
            verifyType(expr->typeexpr);
            return Visitor::visit(expr);
        }
    // ExprAt
        virtual ExpressionPtr visit ( ExprAt * expr ) override {
            if ( !expr->subexpr->type || expr->subexpr->type->isAliasOrExpr() ) return Visitor::visit(expr);    // failed to infer
            if ( !expr->index->type   || expr->index->type->isAliasOrExpr()   ) return Visitor::visit(expr);    // failed to infer
            if ( !expr->no_promotion ) {
                if ( auto opE = inferGenericOperator("[]",expr->at,expr->subexpr,expr->index) ) {
                    opE->alwaysSafe = expr->alwaysSafe;
                    return opE;
                }
            }
            expr->index = Expression::autoDereference(expr->index);
            auto seT = expr->subexpr->type;
            auto ixT = expr->index->type;
            if ( seT->isGoodTableType() ) {
                if ( !seT->firstType->isSameType(*ixT,RefMatters::no, ConstMatters::no, TemporaryMatters::no) ) {
                    error("table index type mismatch, "
                        + describeType(seT->firstType) + " vs " + describeType(ixT),  "", "",
                        expr->index->at, CompilationError::invalid_index_type);
                    return Visitor::visit(expr);
                }
                if ( seT->isConst() ) {
                    error("can't index in the constant table, use find instead", "", "",
                        expr->index->at, CompilationError::invalid_table_type);
                    return Visitor::visit(expr);
                }
                if ( ixT->temporary && seT->firstType->isTempType() ) {
                    error("can't index with the temporary key", "", "",
                        expr->index->at, CompilationError::invalid_index_type);
                    return Visitor::visit(expr);
                }
                if ( seT->secondType->isVoid() ) {
                    error("can't index into table<...;void>", "", "",
                        expr->index->at, CompilationError::invalid_index_type);
                    return Visitor::visit(expr);
                }
                if ( seT->secondType && seT->secondType->lockCheck() ) {
                    if ( !(expr->at.fileInfo && expr->at.fileInfo->name=="builtin.das") && !(func && func->skipLockCheck) && !skipModuleLockChecks ) {
                        reportAstChanged(); // we promote tab[index] into _at_with_lockcheck(tab,index)
                        auto pCall = make_smart<ExprCall>(expr->at, "_at_with_lockcheck");
                        pCall->arguments.push_back(expr->subexpr->clone());
                        pCall->arguments.push_back(expr->index->clone());
                        return pCall;
                    }
                }
                expr->type = make_smart<TypeDecl>(*seT->secondType);
                expr->type->ref = true;
                expr->type->constant |= seT->constant;
            } else if ( seT->isHandle() ) {
                if ( !seT->annotation->isIndexable(ixT) ) {
                    error("handle " + seT->annotation->name + " does not support index type " + describeType(ixT),  "", "",
                        expr->index->at, CompilationError::invalid_index_type);
                    return Visitor::visit(expr);
                }
                expr->type = seT->annotation->makeIndexType(expr->subexpr, expr->index);
                expr->type->constant |= seT->constant;
            } else if ( seT->isPointer() ) {
                if ( !safeExpression(expr) ) {
                    error("index of the pointer requires unsafe",  "", "",
                        expr->at, CompilationError::unsafe);
                }
                if ( !ixT->isIndex() ) {
                    error("index must be int or uint, not " + describeType(ixT),  "", "",
                        expr->index->at, CompilationError::invalid_index_type);
                    return Visitor::visit(expr);
                } else if ( !seT->firstType || seT->firstType->getSizeOf()==0 ){
                    error("can't index void pointer",  "", "",
                        expr->index->at, CompilationError::invalid_index_type);
                    return Visitor::visit(expr);
                } else {
                    expr->subexpr = Expression::autoDereference(expr->subexpr);
                    expr->type = make_smart<TypeDecl>(*seT->firstType);
                    expr->type->ref = true;
                    expr->type->constant |= seT->constant;
                }
            } else {
                if ( ixT->isRange() && (seT->isGoodArrayType() || seT->dim.size()) ) {  // a[range(b)] into subset(a,range(b))
                    auto subset = make_smart<ExprCall>(expr->at, "subarray");
                    subset->arguments.push_back(expr->subexpr->clone());
                    subset->arguments.push_back(expr->index->clone());
                    reportAstChanged();
                    return subset;
                }
                if ( !ixT->isIndex() ) {
                    expr->type.reset();
                    error("index must be int or uint, not " + describeType(ixT),  "", "",
                        expr->index->at, CompilationError::invalid_index_type);
                    return Visitor::visit(expr);
                } else if ( seT->isVectorType() ) {
                    expr->type = make_smart<TypeDecl>(seT->getVectorBaseType());
                    expr->type->ref = seT->ref;
                    expr->type->constant = seT->constant;
                } else if ( seT->isGoodArrayType() ) {
                    expr->type = make_smart<TypeDecl>(*seT->firstType);
                    expr->type->ref = true;
                    expr->type->constant |= seT->constant;
                } else if ( !seT->dim.size() ) {
                    error("type can't be indexed " + describeType(seT),  "", "",
                        expr->subexpr->at, CompilationError::cant_index);
                    return Visitor::visit(expr);
                } else if ( !seT->isAutoArrayResolved() ) {
                    error("type dimensions are not resolved yet " + describeType(seT),  "", "",
                        expr->subexpr->at, CompilationError::cant_index);
                    return Visitor::visit(expr);
                } else {
                    expr->type = make_smart<TypeDecl>(*seT);
                    expr->type->ref = true;
                    expr->type->dim.erase(expr->type->dim.begin());
                    if ( !expr->type->dimExpr.empty() ) {
                        expr->type->dimExpr.erase(expr->type->dimExpr.begin());
                    }
                    expr->type->constant |= seT->constant;
                }
            }
            propagateTempType(expr->subexpr->type, expr->type); // foo#[a] = a#
            return Visitor::visit(expr);
        }
    // ExprSafeAt
        virtual ExpressionPtr visit ( ExprSafeAt * expr ) override {
            if ( !expr->subexpr->type || expr->subexpr->type->isAliasOrExpr() ) return Visitor::visit(expr);    // failed to infer
            if ( !expr->index->type   || expr->index->type->isAliasOrExpr()   ) return Visitor::visit(expr);    // failed to infer
            if ( !expr->no_promotion ) {
                if ( auto opE = inferGenericOperator("?[]",expr->at,expr->subexpr,expr->index) ) return opE;
            }
            if ( !expr->subexpr->type->isVectorType() ) {
                expr->subexpr = Expression::autoDereference(expr->subexpr);
            }
            expr->index = Expression::autoDereference(expr->index);
            auto ixT = expr->index->type;
            if ( expr->subexpr->type->isPointer() ) {
                if (!expr->subexpr->type->firstType) {
                    error("can't index void pointer", "", "",
                        expr->index->at, CompilationError::cant_index);
                    return Visitor::visit(expr);
                }
                auto seT = expr->subexpr->type->firstType;
                if (seT->isGoodTableType()) {
                    if ( !safeExpression(expr) ) {
                        error("safe-index of table<> requires unsafe",  "", "",
                            expr->at, CompilationError::unsafe);
                    }
                    if ( !seT->firstType->isSameType(*ixT,RefMatters::no, ConstMatters::no, TemporaryMatters::no) ) {
                        error("table safe-index type mismatch, "
                            + describeType(seT->firstType) + " vs " + describeType(ixT),  "", "",
                            expr->index->at, CompilationError::invalid_index_type);
                        return Visitor::visit(expr);
                    }
                    expr->type = make_smart<TypeDecl>(Type::tPointer);
                    expr->type->firstType = make_smart<TypeDecl>(*seT->secondType);
                    expr->type->constant |= seT->constant;
                } else if (seT->isHandle()) {
                    // TODO: support handle safe index
                    // if (!seT->annotation->isIndexable(ixT)) {
                        error("handle " + seT->annotation->name + " does not support safe index type " + describeType(ixT), "", "",
                            expr->index->at, CompilationError::invalid_index_type);
                        return Visitor::visit(expr);
                    // }
                    // expr->type = seT->annotation->makeIndexType(expr->subexpr, expr->index);
                    // expr->type->constant |= seT->constant;
                }
                else if (seT->isPointer()) {
                    error("safe-index of pointer is not supported", "", "",
                        expr->index->at, CompilationError::cant_index);
                    return Visitor::visit(expr);
                } else {
                    if (!ixT->isIndex()) {
                        expr->type.reset();
                        error("index must be int or uint, not " + describeType(ixT), "", "",
                            expr->index->at, CompilationError::invalid_index_type);
                        return Visitor::visit(expr);
                    } else if (seT->isVectorType()) {
                        expr->type = make_smart<TypeDecl>(Type::tPointer);
                        expr->type->firstType = make_smart<TypeDecl>(seT->getVectorBaseType());
                        expr->type->firstType->constant = seT->constant;
                    } else if (seT->isGoodArrayType()) {
                        if ( !safeExpression(expr) ) {
                            error("safe-index of array<> requires unsafe",  "", "",
                                expr->at, CompilationError::unsafe);
                        }
                        expr->type = make_smart<TypeDecl>(Type::tPointer);
                        expr->type->firstType = make_smart<TypeDecl>(*seT->firstType);
                        expr->type->firstType->constant |= seT->constant;
                    } else if ( seT->dim.size() ) {
                        if ( !seT->isAutoArrayResolved() ) {
                            error("type dimensions are not resolved yet " + describeType(seT), "", "",
                                expr->subexpr->at, CompilationError::cant_index);
                            return Visitor::visit(expr);
                        } else {
                            expr->type = make_smart<TypeDecl>(Type::tPointer);
                            expr->type->firstType = make_smart<TypeDecl>(*seT);
                            expr->type->firstType->dim.erase(expr->type->firstType->dim.begin());
                            if ( !expr->type->firstType->dimExpr.empty() ) {
                                expr->type->firstType->dimExpr.erase(expr->type->firstType->dimExpr.begin());
                            }
                            expr->type->firstType->constant |= seT->constant;
                        }
                    } else if ( seT->isVectorType() ) {
                        expr->type = make_smart<TypeDecl>(Type::tPointer);
                        expr->type->firstType = make_smart<TypeDecl>(seT->getVectorBaseType());
                        expr->type->firstType->constant = seT->constant;
                    } else {
                        error("type can't be safe-indexed " + describeType(seT), "", "",
                            expr->subexpr->at, CompilationError::cant_index);
                        return Visitor::visit(expr);
                    }
                }
            } else if ( expr->subexpr->type->isGoodArrayType() ) {
                if ( !safeExpression(expr) ) {
                    error("safe-index of array<> requires unsafe",  "", "",
                        expr->at, CompilationError::unsafe);
                }
                auto seT = expr->subexpr->type;
                expr->type = make_smart<TypeDecl>(Type::tPointer);
                expr->type->firstType = make_smart<TypeDecl>(*seT->firstType);
                expr->type->firstType->constant |= seT->constant;
            } else if ( expr->subexpr->type->isGoodTableType() ) {
                if ( !safeExpression(expr) ) {
                    error("safe-index of table<> requires unsafe",  "", "",
                        expr->at, CompilationError::unsafe);
                }
                const auto & seT = expr->subexpr->type;
                if ( !seT->firstType->isSameType(*ixT,RefMatters::no, ConstMatters::no, TemporaryMatters::no) ) {
                    error("table safe-index type mismatch, "
                        + describeType(seT->firstType) + " vs " + describeType(ixT),  "", "",
                        expr->index->at, CompilationError::invalid_index_type);
                    return Visitor::visit(expr);
                }
                if ( seT->secondType->isVoid() ) {
                    error("can't safe-index into table<...;void>", "", "",
                        expr->index->at, CompilationError::invalid_index_type);
                    return Visitor::visit(expr);
                }
                expr->type = make_smart<TypeDecl>(Type::tPointer);
                expr->type->firstType = make_smart<TypeDecl>(*seT->secondType);
                expr->type->constant |= seT->constant;
            } else if ( expr->subexpr->type->dim.size() ) {
                if ( !safeExpression(expr) ) {
                    error("safe-index of [] requires unsafe",  "", "",
                        expr->at, CompilationError::unsafe);
                }
                const auto & seT = expr->subexpr->type;
                if ( !seT->isAutoArrayResolved() ) {
                    error("type dimensions are not resolved yet " + describeType(seT), "", "",
                        expr->subexpr->at, CompilationError::cant_index);
                }
                expr->type = make_smart<TypeDecl>(Type::tPointer);
                expr->type->firstType = make_smart<TypeDecl>(*seT);
                expr->type->firstType->dim.erase(expr->type->firstType->dim.begin());
                if ( !expr->type->firstType->dimExpr.empty() ) {
                    expr->type->firstType->dimExpr.erase(expr->type->firstType->dimExpr.begin());
                }
                expr->type->firstType->constant |= seT->constant;
            } else if ( expr->subexpr->type->isVectorType() && expr->subexpr->type->isRef() ) {
                const auto & seT = expr->subexpr->type;
                expr->type = make_smart<TypeDecl>(Type::tPointer);
                expr->type->firstType = make_smart<TypeDecl>(seT->getVectorBaseType());
                expr->type->firstType->constant = seT->constant;
            } else {
                error("type can't be safe-indexed " + describeType(expr->subexpr->type), "", "",
                    expr->subexpr->at, CompilationError::cant_index);
                return Visitor::visit(expr);
            }
            propagateTempType(expr->subexpr->type, expr->type); // foo#?[a] = a?#
            return Visitor::visit(expr);
        }
    // ExprBlock
        void setBlockCopyMoveFlags ( ExprBlock * block ) {
            if ( block->returnType->isRefType() && !block->returnType->ref ) {
                if ( block->returnType->canCopy() ) {
                    block->copyOnReturn = true;
                    block->moveOnReturn = false;
                } else if ( block->returnType->canMove() ) {
                    block->copyOnReturn = false;
                    block->moveOnReturn = true;
                } else {
                    error("this type can't be returned at all " + describeType(block->returnType), "", "",
                          block->at, CompilationError::invalid_return_type);
                }
            } else {
                block->copyOnReturn = false;
                block->moveOnReturn = false;
            }
        }
        virtual void preVisit ( ExprBlock * block ) override {
            Visitor::preVisit(block);
            block->hasEarlyOut = false;
            block->hasReturn = false;
            if ( block->isClosure ) {
                if ( block->returnType ) {
                    blocks.push_back(block);
                    block->type = make_smart<TypeDecl>(*block->returnType);
                } else {
                    error("malformed ast, closure is missing return type",  "", "",
                        block->at, CompilationError::malformed_ast );
                }
            }
            scopes.push_back(block);
            inFinally.push_back(false);
            pushVarStack();
            block->inFunction = func.get();
        }
        virtual void preVisitBlockFinal ( ExprBlock * block ) override {
            Visitor::preVisitBlockFinal(block);
            inFinally.back() = true;
            if ( block->getFinallyEvalFlags() ) {
                error("finally section can't have break, return, or goto",  "", "",
                    block->at, CompilationError::return_or_break_in_finally );
            }
        }
        virtual void preVisitBlockArgument ( ExprBlock * block, const VariablePtr & var, bool lastArg ) override {
            Visitor::preVisitBlockArgument(block, var, lastArg);
            if ( !var->can_shadow && !program->policies.allow_block_variable_shadowing ) {
                if ( func ) {
                    for ( auto & fna : func->arguments ) {
                        if ( fna->name==var->name ) {
                            error("block argument " + var->name +" is shadowed by function argument "
                                + fna->name + " : " + describeType(fna->type) + " at line " + to_string(fna->at.line), "", "",
                                    var->at, CompilationError::variable_not_found);
                        }
                    }
                }
                for ( auto & blk : blocks ) {
                    if ( blk == block ) continue;
                    for ( auto & bna : blk->arguments ) {
                        if ( bna->name==var->name ) {
                            error("block argument " + var->name +" is shadowed by another block argument "
                                + bna->name + " : " + describeType(bna->type) + " at line " + to_string(bna->at.line), "", "",
                                    var->at, CompilationError::variable_not_found);
                        }
                    }
                }
                for ( auto & lv : local ) {
                    if ( lv->name==var->name ) {
                        error("block argument " + var->name +" is shadowed by local variable "
                            + lv->name + " : " + describeType(lv->type) + " at line " + to_string(lv->at.line), "", "",
                                var->at, CompilationError::variable_not_found);
                        break;
                    }
                }
                if ( auto eW = hasMatchingWith(var->name) ) {
                    error("block argument " + var->name + " is shadowed by with expression at line " + to_string(eW->at.line), "", "",
                        var->at, CompilationError::variable_not_found);
                }
            }
            if ( var->type->isAlias() ) {
                auto aT = inferAlias(var->type);
                if ( aT ) {
                    var->type = aT;
                    reportAstChanged();
                } else {
                    error("undefined block argument type " + describeType(var->type),
                        reportInferAliasErrors(var->type), "", var->at, CompilationError::type_not_found);
                }
            }
            if ( var->type->isAuto() && !var->init) {
                error("block argument type can't be inferred, it needs an "
                    "initializer or to be passed to the function with the explicit block definition", "", "",
                      var->at, CompilationError::cant_infer_missing_initializer );
            }
            if ( var->type->ref && var->type->isRefType() ) { // silently fix a : Foo& into a : Foo
                var->type->ref = false;
            }
            verifyType(var->type,true);
        }
        virtual ExpressionPtr visitBlockArgumentInit (ExprBlock * block, const VariablePtr & arg, Expression * that ) override {
            if ( !arg->init->type ) {
                error("block argument initialization with undefined expression", "", "",
                        arg->at, CompilationError::invalid_type );
                return Visitor::visitBlockArgumentInit(block, arg, that);
            }
            if ( arg->type->isAuto() ) {
                auto argT = TypeDecl::inferGenericType(arg->type, arg->init->type, false, false, nullptr);
                if ( !argT ) {
                    error("block argument initialization type can't be inferred, "
                          + describeType(arg->type) + " = " + describeType(arg->init->type), "", "",
                          arg->at, CompilationError::cant_infer_mismatching_restrictions );
                } else {
                    TypeDecl::applyAutoContracts(argT, arg->type);
                    arg->type = argT;
                    arg->type->ref = false; // so that def ( a = VAR ) infers as def ( a : var_type ), not as def ( a : var_type & )
                    reportAstChanged();
                    return Visitor::visitBlockArgumentInit(block, arg, that);
                }
            }
            if (!arg->type->isAuto()) {
                if (!arg->init->type || !arg->type->isSameType(*arg->init->type, RefMatters::no, ConstMatters::no, TemporaryMatters::no)) {
                    error("block argument default value type mismatch "
                        + describeType(arg->type) + " vs " + (arg->init->type ? describeType(arg->init->type) : "???"),  "", "",
                        arg->init->at, CompilationError::invalid_argument_type);
                }
                if (arg->init->type && arg->type->ref && !arg->init->type->ref) {
                    error("block argument default value type mismatch " + describeType(arg->type)
                        + " vs " + describeType(arg->init->type) + ", reference matters",   "", "",
                        arg->init->at, CompilationError::invalid_argument_type);
                }
            }
            return Visitor::visitBlockArgumentInit(block, arg, that);
        }

        virtual ExpressionPtr visitBlockExpression ( ExprBlock * block, Expression * that ) override {
            // lets do strenth reduction on ++ and -- at the top-level expressions
            if ( that->type && that->rtti_isOp1() && that->type->isWorkhorseType() ) {
                auto op1 = static_cast<ExprOp1 *>(that);
                if ( op1->op=="+++" ) {
                    op1->op = "++";
                    reportAstChanged();
                } else if ( op1->op=="---" ) {
                    op1->op = "--";
                    reportAstChanged();
                }
            }
            // lets collapse the following every time
            //  return quote() <|
            //      pass
            if ( that->rtti_isMakeBlock() ) {
                auto mblk = static_cast<ExprMakeBlock *>(that);
                auto iblk = static_cast<ExprBlock *>(mblk->block.get());
                if ( iblk->list.empty() && iblk->finalList.empty() ) {
                    reportAstChanged();
                    return nullptr;
                }
            }
            return Visitor::visitBlockExpression(block, that);
        }

        virtual ExpressionPtr visit ( ExprBlock * block ) override {
            // to the rest of it
            popVarStack();
            scopes.pop_back();
            inFinally.pop_back();
            if ( block->isClosure && block->returnType ) {
                blocks.pop_back();
                if ( block->list.size() ) {
                    uint32_t flags = block->getEvalFlags();
                    if ( flags & EvalFlags::stopForBreak ) {
                        error("captured block can't 'break' outside of the block",  "", "",
                              block->at, CompilationError::invalid_block);
                    }
                    if ( flags & EvalFlags::stopForContinue ) {
                        error("captured block can't 'continue' outside of the block",  "", "",
                              block->at, CompilationError::invalid_block);
                    }
                }
                if ( !block->hasReturn && canFoldResult && block->type->isAuto() ) {
                    block->returnType = make_smart<TypeDecl>(Type::tVoid);
                    block->type = make_smart<TypeDecl>(Type::tVoid);
                    setBlockCopyMoveFlags(block);
                    reportAstChanged();
                }
                if ( block->returnType ) {
                    setBlockCopyMoveFlags(block);
                    verifyType(block->returnType);
                }
            }
            if ( block->needCollapse ) {
                block->needCollapse = false;
                if ( block->collapse() ) reportAstChanged();
            }
            return Visitor::visit(block);
        }
    // Swizzle
        virtual ExpressionPtr visit ( ExprSwizzle * expr ) override {
            if ( !expr->value->type ) return Visitor::visit(expr);
            auto valT = expr->value->type;
            if ( !valT->isVectorType() ) {
                error("unsupported swizzle type " + valT->describe(),   "", "",
                    expr->at, CompilationError::invalid_swizzle_mask);
                return Visitor::visit(expr);
            }
            int dim = valT->getVectorDim();
            if ( !TypeDecl::buildSwizzleMask(expr->mask, dim, expr->fields) ) {
                error("invalid swizzle mask",   "", "",
                    expr->at, CompilationError::invalid_swizzle_mask);
            } else {
                auto bt = valT->getVectorBaseType();
                auto rt = valT->isRange() ?
                    TypeDecl::getRangeType(bt, int(expr->fields.size())) :
                    TypeDecl::getVectorType(bt, int(expr->fields.size()));
                expr->type = make_smart<TypeDecl>(rt);
                expr->type->constant = valT->constant;
                expr->type->ref = valT->ref;
                if ( expr->type->ref ) {
                    expr->type->ref &= TypeDecl::isSequencialMask(expr->fields);
                }
                if ( !expr->type->ref ) {
                    expr->value = Expression::autoDereference(expr->value);
                }
            }
            return Visitor::visit(expr);
        }
    // ExprAsVariant
        virtual ExpressionPtr visit(ExprAsVariant * expr) override {
            if (!expr->value->type || expr->value->type->isAliasOrExpr()) return Visitor::visit(expr);
            // implement variant macros
            ExpressionPtr substitute;
            auto thisModule = ctx.thisProgram->thisModule.get();
            auto modMacro = [&](Module * mod) -> bool {
                if ( thisModule->isVisibleDirectly(mod) && mod!=thisModule ) {
                    for ( const auto & pm : mod->variantMacros ) {
                        if ( (substitute = pm->visitAs(ctx.thisProgram, thisModule, expr)) ) {
                            return false;
                        }
                    }
                }
                return true;
            };
            Module::foreach(modMacro);
            if ( !substitute ) ctx.thisProgram->library.foreach(modMacro, "*");
            if ( substitute ) {
                reportAstChanged();
                return substitute;
            }
            // generic operator
            if ( auto opE = inferGenericOperator("`as`"+expr->name,expr->at,expr->value,nullptr) ) return opE;
            if ( auto opE = inferGenericOperatorWithName("`as",expr->at,expr->value,expr->name) ) return opE;
            // regular infer
            auto valT = expr->value->type;
            if ( !valT->isGoodVariantType() ) {
                error(" as " + expr->name + " only allowed for variants", "", "",
                    expr->at, CompilationError::invalid_type);
                return Visitor::visit(expr);

            }
            int index = valT->findArgumentIndex(expr->name);
            if ( index==-1 || index>=int(valT->argTypes.size()) ) {
                error("can't get variant field " + expr->name, "", "",
                    expr->at, CompilationError::cant_get_field);
                return Visitor::visit(expr);
            }
            expr->fieldIndex = index;
            expr->type = make_smart<TypeDecl>(*valT->argTypes[expr->fieldIndex]);
            expr->type->ref = true;
            expr->type->constant |= valT->constant;
            propagateTempType(expr->value->type, expr->type); // a# as foo = foo#
            return Visitor::visit(expr);
        }
    // ExprSafeAsVariant
        virtual ExpressionPtr visit(ExprSafeAsVariant * expr) override {
            if (!expr->value->type || expr->value->type->isAliasOrExpr()) return Visitor::visit(expr);
            // implement variant macros
            ExpressionPtr substitute;
            auto thisModule = ctx.thisProgram->thisModule.get();
            auto modMacro = [&](Module * mod) -> bool {
                if ( thisModule->isVisibleDirectly(mod) && mod!=thisModule ) {
                    for ( const auto & pm : mod->variantMacros ) {
                        if ( (substitute = pm->visitSafeAs(ctx.thisProgram, thisModule, expr)) ) {
                            return false;
                        }
                    }
                }
                return true;
            };
            Module::foreach(modMacro);
            if ( !substitute ) ctx.thisProgram->library.foreach(modMacro, "*");
            if ( substitute ) {
                reportAstChanged();
                return substitute;
            }
            // generic operator
            if ( auto opE = inferGenericOperator("?as`"+expr->name,expr->at,expr->value,nullptr) ) return opE;
            if ( auto opE = inferGenericOperatorWithName("?as",expr->at,expr->value,expr->name) ) return opE;
            // regular infer
            if ( !expr->value->type->isPointer() && !safeExpression(expr) ) {
                error("variant ?as on non-pointer requires unsafe", "", "",
                    expr->at,CompilationError::unsafe);
            }
            auto valT = expr->value->type->isPointer() ? expr->value->type->firstType : expr->value->type;
            if ( !valT || !valT->isGoodVariantType() ) {
                error(" ?as " + expr->name + " only allowed for variants or pointers to variants and not " + expr->value->type->describe(), "", "",
                    expr->at, CompilationError::invalid_type);
                return Visitor::visit(expr);

            }
            if ( expr->value->type->isPointer() ) {
                expr->value = Expression::autoDereference(expr->value);
            }
            int index = valT->findArgumentIndex(expr->name);
            if ( index==-1 || index>=int(valT->argTypes.size()) ) {
                error("can't get variant field " + expr->name, "", "",
                    expr->at, CompilationError::cant_get_field);
                return Visitor::visit(expr);
            }
            expr->fieldIndex = index;
            expr->type = make_smart<TypeDecl>(*valT->argTypes[expr->fieldIndex]);
            expr->skipQQ = expr->type->isPointer();
            if ( !expr->skipQQ ) {
                auto fieldType = expr->type;
                expr->type = make_smart<TypeDecl>(Type::tPointer);
                expr->type->firstType = fieldType;
            }
            expr->type->constant |= valT->constant || expr->value->type->constant;
            propagateTempType(expr->value->type, expr->type); // a# ?as foo = foo?#
            return Visitor::visit(expr);
        }
    // ExprIsVariant
        virtual ExpressionPtr visit(ExprIsVariant * expr) override {
            if (!expr->value->type || expr->value->type->isAliasOrExpr()) return Visitor::visit(expr);
            // implement variant macros
            ExpressionPtr substitute;
            auto thisModule = ctx.thisProgram->thisModule.get();
            auto modMacro = [&](Module * mod) -> bool {
                if ( thisModule->isVisibleDirectly(mod) && mod!=thisModule ) {
                    for ( const auto & pm : mod->variantMacros ) {
                        if ( (substitute = pm->visitIs(ctx.thisProgram, thisModule, expr)) ) {
                            return false;
                        }
                    }
                }
                return true;
            };
            Module::foreach(modMacro);
            if ( !substitute ) ctx.thisProgram->library.foreach(modMacro, "*");
            if ( substitute ) {
                reportAstChanged();
                return substitute;
            }
            // generic operator
            if ( auto opE = inferGenericOperator("`is`"+expr->name,expr->at,expr->value,nullptr) ) return opE;
            if ( auto opE = inferGenericOperatorWithName("`is",expr->at,expr->value,expr->name) ) return opE;
            // regular infer
            auto valT = expr->value->type;
            if ( !valT->isGoodVariantType() ) {
                error(" is " + expr->name + " only allowed for variants", "", "",
                    expr->at, CompilationError::invalid_type);
                return Visitor::visit(expr);

            }
            int index = valT->findArgumentIndex(expr->name);
            if ( index==-1 || index>=int(valT->argTypes.size()) ) {
                error("can't get variant field " + expr->name, "", "",
                    expr->at, CompilationError::cant_get_field);
                return Visitor::visit(expr);
            }
            expr->fieldIndex = index;
            expr->type = make_smart<TypeDecl>(Type::tBool);
            return Visitor::visit(expr);
        }
    // ExprField
        bool verifyPrivateFieldLookup ( ExprField * expr ) {
            // lets verify private field lookup
            if ( expr->field && expr->field->privateField ) {
                bool canLookup = false;
                if ( func && func->isClassMethod ) {
                    TypeDecl selfT(func->classParent);
                    if ( selfT.isSameType(*expr->value->type,
                            RefMatters::no,ConstMatters::no,TemporaryMatters::no,AllowSubstitute::yes) ) {
                        canLookup = true;
                    }
                }
                if ( !canLookup ) {
                    error("can't access private field " + expr->name + " of " + describeType(expr->value->type)
                        + " outside of member functions.", "", "",
                            expr->at, CompilationError::cant_get_field);
                    return false;
                }
            }
            return true;
        }
        virtual ExpressionPtr visit ( ExprField * expr ) override {
            if ( !expr->value->type || expr->value->type->isAliasOrExpr() ) return Visitor::visit(expr);    // failed to infer
            if ( expr->underClone ) { // we wait for the 'right' type to be infered
                if ( !expr->underClone->right->type || expr->underClone->right->type->isAliasOrExpr() ) {
                    error("under clone field type not infered yet", "", "",
                            expr->at, CompilationError::cant_get_field);
                    return Visitor::visit(expr);
                }
            }
            if ( expr->name.empty() ) {
                error("syntax error, expecting field after .", "", "",
                        expr->at, CompilationError::cant_get_field);
                return Visitor::visit(expr);
            }
            auto valT = expr->value->type;
            if ( valT->isPointer() ) {
                if ( !expr->no_promotion && valT->firstType ) {
                    auto derefV = make_smart<ExprPtr2Ref>(expr->at, expr->value);
                    derefV->type = valT->firstType;
                    TypeDecl::applyAutoContracts(derefV->type,valT->firstType);
                    derefV->type->ref = true;
                    derefV->type->constant |= valT->constant;
                    if ( expr->underClone ) {
                        expr->underClone->cloneSet = inferGenericOperator(".`"+expr->name+"`clone",expr->at,derefV,expr->underClone->right);
                        if ( expr->underClone->cloneSet ) {
                            return Visitor::visit(expr);
                        }
                    }
                    if ( auto opE = inferGenericOperator(".`"+expr->name,expr->at,derefV,nullptr) ) return opE;
                    if ( auto opE = inferGenericOperatorWithName(".",expr->at,derefV,expr->name) ) return opE;
                }
                expr->value = Expression::autoDereference(expr->value);
                expr->unsafeDeref = func ? func->unsafeDeref : false;
                if ( !valT->firstType ) {
                    error("can't get field " + expr->name + " of void pointer", "", "",
                        expr->at, CompilationError::cant_get_field);
                    return Visitor::visit(expr);
                } else if ( valT->firstType->isStructure() ) {
                    expr->field = valT->firstType->structType->findField(expr->name);
                } else if ( valT->firstType->isHandle() ) {
                    expr->annotation = valT->firstType->annotation;
                    expr->type = expr->annotation->makeFieldType(expr->name, valT->constant || valT->firstType->constant);
                    if ( expr->type )
                        expr->type->constant |= valT->constant || valT->firstType->constant;
                } else if ( valT->firstType->isGoodTupleType() ) {
                    int index = valT->tupleFieldIndex(expr->name);
                    if ( index==-1 || index>=int(valT->firstType->argTypes.size()) ) {
                        error("can't get tuple field " + expr->name, "", "",
                            expr->at, CompilationError::cant_get_field);
                        return Visitor::visit(expr);
                    }
                    expr->fieldIndex = index;
                } else if ( valT->firstType->isGoodVariantType() ) {
                    if ( !safeExpression(expr) ) {
                        error("variant.field requires unsafe", "", "",
                            expr->at, CompilationError::unsafe);
                        return Visitor::visit(expr);
                    }
                    int index = valT->variantFieldIndex(expr->name);
                    if ( index==-1 || index>=int(valT->firstType->argTypes.size()) ) {
                        error("can't get variant field " + expr->name, "", "",
                            expr->at, CompilationError::cant_get_field);
                        return Visitor::visit(expr);
                    }
                    expr->fieldIndex = index;
                }
            } else {
                if ( !expr->no_promotion ) {
                    if ( expr->underClone ) {
                        expr->underClone->cloneSet = inferGenericOperator(".`"+expr->name+"`clone",expr->at,expr->value,expr->underClone->right);
                        if ( expr->underClone->cloneSet ) {
                            return Visitor::visit(expr);
                        }
                    }
                    if ( auto opE = inferGenericOperator(".`"+expr->name,expr->at,expr->value,nullptr) ) return opE;
                    if ( auto opE = inferGenericOperatorWithName(".",expr->at,expr->value,expr->name) ) return opE;
                }
                if ( valT->isVectorType() ) {
                    reportAstChanged();
                    return make_smart<ExprSwizzle>(expr->at,expr->value,expr->name);
                } else if ( valT->isBitfield() ) {
                    expr->value = Expression::autoDereference(expr->value);
                    valT = expr->value->type;
                    int index = valT->bitFieldIndex(expr->name);
                    if ( index==-1 || index>=int(valT->argNames.size()) ) {
                        error("can't get bit field " + expr->name + " in " + describeType(valT), "", "",
                            expr->at, CompilationError::cant_get_field);
                        return Visitor::visit(expr);
                    }
                    expr->fieldIndex = index;
                } else if ( valT->isHandle() ) {
                    expr->annotation = valT->annotation;
                    expr->type = expr->annotation->makeFieldType(expr->name, valT->constant);
                } else if ( valT->isStructure() ) {
                    expr->field = valT->structType->findField(expr->name);
                } else if ( valT->isGoodTupleType() ) {
                    int index = valT->tupleFieldIndex(expr->name);
                    if ( index==-1 || index>=int(valT->argTypes.size()) ) {
                        error("can't get tuple field " + expr->name, "", "",
                            expr->at, CompilationError::cant_get_field);
                        return Visitor::visit(expr);
                    }
                    expr->fieldIndex = index;
                } else if ( valT->isGoodVariantType() ) {
                    if ( !safeExpression(expr) ) {
                        error("variant.field requires unsafe", "", "",
                            expr->at, CompilationError::unsafe);
                        return Visitor::visit(expr);
                    }
                    int index = valT->variantFieldIndex(expr->name);
                    if ( index==-1 || index>=int(valT->argTypes.size()) ) {
                        error("can't get variant field " + expr->name, "", "",
                            expr->at, CompilationError::cant_get_field);
                        return Visitor::visit(expr);
                    }
                    expr->fieldIndex = index;
                } else {
                    error("can't get field " + expr->name + " of " + describeType(expr->value->type), "", "",
                        expr->at, CompilationError::cant_get_field);
                    return Visitor::visit(expr);
                }
            }
            // lets verify private field lookup
            if ( !verifyPrivateFieldLookup(expr) ) {
                return Visitor::visit(expr);
            }
            // handle
            if ( expr->field ) {
                expr->type = make_smart<TypeDecl>(*expr->field->type);
                expr->type->ref = true;
                expr->type->constant |= valT->constant;
                if ( valT->isPointer() && valT->firstType ) {
                    expr->type->constant |= valT->firstType->constant;
                }
                if ( !expr->ignoreCaptureConst ) {
                    expr->type->constant |= expr->field->capturedConstant;
                }
            } else if ( expr->fieldIndex!=-1 ) {
                if ( valT->isBitfield() ) {
                    expr->type = make_smart<TypeDecl>(Type::tBool);
                } else {
                    auto tupleT = valT->isPointer() ? valT->firstType : valT;
                    expr->type = make_smart<TypeDecl>(*tupleT->argTypes[expr->fieldIndex]);
                    expr->type->ref = true;
                    expr->type->constant |= tupleT->constant;
                }
            } else if ( !expr->type ) {
                error("field " + expr->name + " not found in " + describeType(expr->value->type), "", "",
                      expr->at, CompilationError::cant_get_field);
                return Visitor::visit(expr);
            } else {
                expr->type->constant |= valT->constant;
            }
            propagateTempType(expr->value->type, expr->type);   // a#.foo = foo#
            return Visitor::visit(expr);
        }
    // ExprSafeField
        virtual ExpressionPtr visit ( ExprSafeField * expr ) override {
            if ( !expr->value->type || expr->value->type->isAliasOrExpr() ) return Visitor::visit(expr);    // failed to infer
            if ( !expr->no_promotion ) {
                if ( auto opE = inferGenericOperator("?.`"+expr->name,expr->at,expr->value,nullptr) ) return opE;
                if ( auto opE = inferGenericOperatorWithName("?.",expr->at,expr->value,expr->name) ) return opE;
            }
            auto valT = expr->value->type;
            if ( !valT->isPointer() || !valT->firstType ) {
                error("can only safe dereference a pointer to a tupe, a structure or a handle " + describeType(valT), "", "",
                      expr->at, CompilationError::cant_get_field);
                return Visitor::visit(expr);
            }
            expr->value = Expression::autoDereference(expr->value);
            if ( valT->firstType->structType ) {
                expr->field = valT->firstType->structType->findField(expr->name);
                if ( !expr->field ) {
                    error("can't safe get field " + expr->name, "", "",
                        expr->at, CompilationError::cant_get_field);
                    return Visitor::visit(expr);
                }
                expr->type = make_smart<TypeDecl>(*expr->field->type);
            } else if ( valT->firstType->isHandle() ) {
                expr->annotation = valT->firstType->annotation;
                expr->type = expr->annotation->makeSafeFieldType(expr->name, valT->constant);
                if ( !expr->type ) {
                    error("can't safe get field " + expr->name, "", "",
                        expr->at, CompilationError::cant_get_field);
                    return Visitor::visit(expr);
                }
            } else if ( valT->firstType->isGoodTupleType() ) {
                int index = valT->tupleFieldIndex(expr->name);
                if ( index==-1 || index>=int(valT->firstType->argTypes.size()) ) {
                    error("can't get tuple field " + expr->name, "", "",
                        expr->at, CompilationError::cant_get_field);
                    return Visitor::visit(expr);
                }
                expr->fieldIndex = index;
                expr->type = make_smart<TypeDecl>(*valT->firstType->argTypes[expr->fieldIndex]);
            } else if ( valT->firstType->isGoodVariantType() ) {
                if ( !safeExpression(expr) ) {
                    error("variant?.field requires unsafe", "", "",
                        expr->at, CompilationError::unsafe);
                    return Visitor::visit(expr);
                }
                int index = valT->variantFieldIndex(expr->name);
                if ( index==-1 || index>=int(valT->firstType->argTypes.size()) ) {
                    error("can't get variant field " + expr->name, "", "",
                        expr->at, CompilationError::cant_get_field);
                    return Visitor::visit(expr);
                }
                expr->fieldIndex = index;
                expr->type = make_smart<TypeDecl>(*valT->firstType->argTypes[expr->fieldIndex]);
            } else {
                error("can only safe dereference a pointer to a tuple, a structure or a handle " + describeType(valT), "", "",
                      expr->at, CompilationError::cant_get_field);
                return Visitor::visit(expr);
            }
            expr->skipQQ = expr->type->isPointer();
            if ( !expr->skipQQ ) {
                auto fieldType = expr->type;
                expr->type = make_smart<TypeDecl>(Type::tPointer);
                expr->type->firstType = fieldType;
            }
            expr->type->constant |= valT->constant;
            propagateTempType(expr->value->type, expr->type); // a#?.foo = foo?#
            // lets verify private field lookup
            verifyPrivateFieldLookup(expr);
            return Visitor::visit(expr);
        }
    // tag
        virtual void preVisit ( ExprTag * expr ) override {
            Visitor::preVisit(expr);
            error("macro tags can only appear in macro blocks", "", "", expr->at, CompilationError::unbound_macro_tag);
        }
    // ExprVar
        vector<VariablePtr> findMatchingVar ( const string & name, bool seePrivate ) const {
            string moduleName, varName;
            splitTypeName(name, moduleName, varName);
            vector<VariablePtr> result;
            auto inWhichModule = getSearchModule(moduleName);
            auto thisModule = program->thisModule.get();
            program->library.foreach([&](Module * mod) -> bool {
                if ( auto var = mod->findVariable(varName) ) {
                    if ( inWhichModule->isVisibleDirectly(var->module) ) {
                        if ( seePrivate || canAccessGlobal(var,inWhichModule,thisModule) ) {
                            result.push_back(var);
                        }
                    }
                }
                return true;
            },moduleName);
            return result;
        }
        virtual void preVisit ( ExprVar * expr ) override {
            Visitor::preVisit(expr);
            expr->variable = nullptr;
            expr->local = false;
            expr->block = false;
            expr->pBlock = nullptr;
            expr->argument = false;
            expr->argumentIndex = -1;
        }
        virtual ExpressionPtr visit ( ExprVar * expr ) override {
            // assume (that on the stack)
            for ( auto it = assume.rbegin(), its=assume.rend(); it!=its; ++it ) {
                auto ita = *it;
                if ( ita->alias==expr->name ) {
                    reportAstChanged();
                    auto csub = ita->subexpr->clone();
                    // forceAt(csub, ita->at);
                    return csub;
                }
            }
            // local (that on the stack)
            for ( auto it = local.rbegin(); it!=local.rend(); ++it ) {
                auto var = *it;
                if ( var->name==expr->name || var->aka==expr->name ) {
                    expr->variable = var;
                    expr->local = true;
                    expr->type = make_smart<TypeDecl>(*var->type);
                    expr->type->ref = true;
                    var->used_in_finally = inFinally.empty() ? false : inFinally.back();
                    return Visitor::visit(expr);
                }
            }
            // with
            if ( auto eW = hasMatchingWith(expr->name) ) {
                reportAstChanged();
                return make_smart<ExprField>(expr->at, forceAt(eW->with->clone(),expr->at), expr->name);
            }
            // block arguments
            for ( auto it = blocks.rbegin(); it!=blocks.rend(); ++it ) {
                ExprBlock * block = *it;
                int argumentIndex = 0;
                for ( auto & arg : block->arguments ) {
                    if ( arg->name==expr->name || arg->aka==expr->name ) {
                        expr->variable = arg;
                        expr->argumentIndex = argumentIndex;
                        expr->block = true;
                        if ( blocks.rbegin() == it ) {
                            expr->thisBlock = true;
                        }
                        expr->type = make_smart<TypeDecl>(*arg->type);
                        if (!expr->type->isRefType())
                            expr->type->ref = true;
                        expr->type->sanitize();
                        expr->pBlock = static_cast<ExprBlock*>(block);
                        return Visitor::visit(expr);
                    }
                    argumentIndex ++;
                }
            }
            // function argument
            if ( func ) {
                int argumentIndex = 0;
                for ( auto & arg : func->arguments ) {
                    if ( arg->name==expr->name || arg->aka==expr->name ) {
                        expr->variable = arg;
                        expr->argumentIndex = argumentIndex;
                        expr->argument = true;
                        expr->type = make_smart<TypeDecl>(*arg->type);
                        if (!expr->type->isRefType())
                            expr->type->ref = true;
                        expr->type->sanitize();
                        return Visitor::visit(expr);
                    }
                    argumentIndex ++;
                }
            }
            // global
            auto vars = findMatchingVar(expr->name, false);
            if ( vars.size()==1 ) {
                auto var = vars.back();
                if ( var==globalVar ) {
                    error("global variable " + expr->name + " cant't be initialized with itself",
                        "", "", expr->at, CompilationError::variable_not_found);
                    return Visitor::visit(expr);
                }
                expr->variable = var;
                expr->type = make_smart<TypeDecl>(*var->type);
                expr->type->ref = true;
                return Visitor::visit(expr);

            } else if ( vars.size()==0 ) {
                if ( verbose ) {
                    vars = findMatchingVar(expr->name, true);
                    if ( vars.size() ) {
                        TextWriter errs;
                        for ( auto & var : vars ) {
                            errs << "\t" << var->module->name << "::" << var->name << " : " << describeType(var->type);
                            if ( var->at.line ) {
                                errs << " at " << var->at.describe();
                            }
                            errs << "\n";
                        }
                        error("can't access private variable " + expr->name, "not visible due to privacy:\n" + errs.str(), "",
                            expr->at, CompilationError::variable_not_found);
                    } else {
                        error("can't locate variable " + expr->name, "", "",
                            expr->at, CompilationError::variable_not_found);
                    }
                } else {
                    error("can't locate variable " + expr->name, "", "",
                        expr->at, CompilationError::variable_not_found);
                }
            } else {
                if ( verbose ) {
                    TextWriter errs;
                    for ( auto & var : vars ) {
                        errs << "\t" << var->module->name << "::" << var->name << " : " << describeType(var->type);
                        if ( var->at.line ) {
                            errs << " at " << var->at.describe();
                        }
                        errs << "\n";
                    }
                    error("too many matching variables " + expr->name, "candidates are:\n" + errs.str(), "",
                        expr->at, CompilationError::variable_not_found);
                } else {
                    error("too many matching variables " + expr->name, "", "",
                        expr->at, CompilationError::variable_not_found);
                }
            }
            return Visitor::visit(expr);
        }
    // ExprOp1
        virtual ExpressionPtr visit ( ExprOp1 * expr ) override {
            if ( !expr->subexpr->type || expr->subexpr->type->isAliasOrExpr() ) return Visitor::visit(expr);    // failed to infer
            // pointer arithmetics
            if ( expr->subexpr->type->isPointer() ) {
                if ( !expr->subexpr->type->firstType ) {
                    error("operations on void pointers are prohibited; " +
                        describeType(expr->subexpr->type), "", "",
                        expr->at, CompilationError::invalid_type);
                } else {
                    string pop;
                            if ( expr->op=="++" || expr->op=="+++" )   { pop = "i_das_ptr_inc"; }
                    else    if ( expr->op=="--" || expr->op=="---" )   { pop = "i_das_ptr_dec"; }
                    if ( !pop.empty() ) {
                        reportAstChanged();
                        auto popc = make_smart<ExprCall>(expr->at, pop);
                        auto stride = expr->subexpr->type->firstType->getSizeOf();
                        popc->arguments.push_back(expr->subexpr->clone());
                        popc->arguments.push_back(make_smart<ExprConstInt>(expr->at,stride));
                        return popc;
                    } else {
                        error("pointer arithmetics only allows +, -, +=, -=, ++, --; not" + expr->op, "", "",
                            expr->at, CompilationError::operator_not_found);
                    }
                }
            }
            if ( expr->op=="++" || expr->op=="+++" || expr->op=="--" || expr->op=="---" ) {
                if ( expr->subexpr->type->isWorkhorseType() ) {
                    if ( !expr->subexpr->type->ref ) {
                        error(expr->op + " can't be applied to non reference " + describeType(expr->subexpr->type), "", "",
                            expr->at, CompilationError::operator_not_found);
                        return Visitor::visit(expr);
                    } else if ( expr->subexpr->type->constant ) {
                        error(expr->op + " can't be applied to constant " + describeType(expr->subexpr->type), "", "",
                            expr->at, CompilationError::operator_not_found);
                        return Visitor::visit(expr);
                    }
                }
            }
            auto opName = "_::" + expr->op;
            auto tempCall = make_smart<ExprLooksLikeCall>(expr->at,opName);
            tempCall->arguments.push_back(expr->subexpr);
            expr->func = inferFunctionCall(tempCall.get()).get();
            if ( opName != tempCall->name ) {   // this happens when the operator gets instanced
                reportAstChanged();
                auto opCall = make_smart<ExprCall>(expr->at, tempCall->name);
                opCall->arguments = das::move(tempCall->arguments);
                return opCall;
            }
            if ( expr->func ) {
                if ( expr->func->firstArgReturnType ) {
                    expr->type = make_smart<TypeDecl>(*expr->arguments[0]->type);
                    expr->type->ref = false;
                } else {
                    expr->type = make_smart<TypeDecl>(*expr->func->result);
                }
                if ( !expr->func->arguments[0]->type->isRef() )
                    expr->subexpr = Expression::autoDereference(expr->subexpr);
                // lets try to fold it
                if ( expr->func && expr->func->unsafeOperation && safeExpression(expr) ) {
                    error("unsafe operator " + expr->name + " requires unsafe", "", "",
                        expr->at, CompilationError::unsafe);
                } else if ( enableInferTimeFolding && isConstExprFunc(expr->func) ) {
                    if ( auto se = getConstExpr(expr->subexpr.get()) ) {
                        expr->subexpr = se;
                        return evalAndFold(expr);
                    }
                }
            }
            return Visitor::visit(expr);
        }
    // ExprOp2
        bool isAssignmentOperator ( const string & op ) {
            return (op=="+=") || (op=="-=") || (op=="*=") || (op=="/=")
                || (op=="%=") || (op=="&=") || (op=="|=") || (op=="^=")
                || (op=="<<=") || (op==">>=") || (op=="<<<=") || (op==">>>=");
        }

        bool isLogicOperator ( const string & op ) {
            return (op=="==") || (op=="!=") || (op==">=") || (op=="<=") || (op==">") || (op=="<");
        }

        bool isCommutativeOperator ( const string & op ) {
            return (op=="*") || (op=="+") || isLogicOperator(op);
        }

        string flipCommutativeOperatorSide ( const string & op ) {
                 if ( op==">"  ) return "<";
            else if ( op=="<"  ) return ">";
            else if ( op==">=" ) return "<=";
            else if ( op=="<=" ) return ">=";
            else return op;
        }

        bool canOperateOnPointers ( const TypeDeclPtr & leftType, const TypeDeclPtr & rightType, TemporaryMatters tmatter ) const {
            if ( leftType->baseType==Type::tPointer ) {
                return leftType->isSameType(*rightType, RefMatters::no, ConstMatters::no, tmatter, AllowSubstitute::yes);
            } else {
                return leftType->isSameType(*rightType, RefMatters::no, ConstMatters::no, tmatter, AllowSubstitute::no);
            }
        }

        bool isSameSmartPtrType ( const TypeDeclPtr & lt, const TypeDeclPtr & rt, bool leftOnly = false ) {
            auto lt_smart = lt->smartPtr;
            auto rt_smart = rt->smartPtr;
            if ( leftOnly ) {
                // smart smart  - ok
                // smart ptr    - ok
                // ptr   smart  - not OK
                // ptr   ptr    - ok
                if ( !lt_smart && rt_smart ) return false;
            }
            lt->smartPtr = false;
            rt->smartPtr = false;
            bool res =  canOperateOnPointers(lt,rt,TemporaryMatters::no);
            lt->smartPtr = lt_smart;
            rt->smartPtr = rt_smart;
            return res;
        }

        virtual ExpressionPtr visit ( ExprOp2 * expr ) override {
            if ( !expr->left->type  || expr->left->type->isAliasOrExpr()  ) return Visitor::visit(expr);    // failed to infer
            if ( !expr->right->type || expr->right->type->isAliasOrExpr() ) return Visitor::visit(expr);    // failed to infer
            // flippling commutative operations, if the constant is on the left
            if ( expr->left->rtti_isConstant() && !expr->right->rtti_isConstant() ) {                   // if left is const, but right is not
                if ( expr->left->type->isNumericComparable() && isCommutativeOperator(expr->op) ) {     // if its compareable, and its a logic operator
                    auto flip = flipCommutativeOperatorSide(expr->op);                              // we swap left and right, and change the op
                    auto nexpr = make_smart<ExprOp2>(expr->at, flip, expr->right, expr->left);
                    reportAstChanged();
                    return nexpr;
                }
            }
            // pointer arithmetics
            if ( expr->left->type->isPointer() && expr->right->type->isSimpleType(Type::tInt) ) {
                if ( !expr->left->type->firstType ) {
                    error("operations on void pointers are prohibited; " +
                        describeType(expr->left->type), "", "",
                        expr->at, CompilationError::invalid_type);
                } else {
                    string pop;
                            if ( expr->op=="+" )    { pop = "i_das_ptr_add"; }
                    else    if ( expr->op=="-" )    { pop = "i_das_ptr_sub"; }
                    else    if ( expr->op=="+=" )   { pop = "i_das_ptr_set_add"; }
                    else    if ( expr->op=="-=" )   { pop = "i_das_ptr_set_sub"; }
                    if ( !pop.empty() ) {
                        reportAstChanged();
                        auto popc = make_smart<ExprCall>(expr->at, pop);
                        auto stride = expr->left->type->firstType->getSizeOf();
                        popc->arguments.push_back(expr->left->clone());
                        popc->arguments.push_back(expr->right->clone());
                        popc->arguments.push_back(make_smart<ExprConstInt>(expr->at,stride));
                        return popc;
                    } else {
                        error("pointer arithmetics only allows +, -, +=, -=, ++, --; not" + expr->op, "", "",
                            expr->at, CompilationError::operator_not_found);
                    }
                }
            }
            // infer
            if ( expr->left->type->isPointer() && expr->right->type->isPointer() ) {
                if ( !isSameSmartPtrType(expr->left->type,expr->right->type) ) {
                    error("operations on incompatible pointers are prohibited; " +
                        describeType(expr->left->type) + " vs " + describeType(expr->right->type), "", "",
                        expr->at, CompilationError::invalid_type);
                }
                if ( expr->op=="-" ) {
                    if ( !expr->left->type->firstType ) {
                        error("operations on void pointers are prohibited; " +
                            describeType(expr->left->type), "", "",
                            expr->at, CompilationError::invalid_type);
                    } else {
                        reportAstChanged();
                        auto popc = make_smart<ExprCall>(expr->at, "i_das_ptr_diff");
                        auto stride = expr->left->type->firstType->getSizeOf();
                        popc->arguments.push_back(expr->left->clone());
                        popc->arguments.push_back(expr->right->clone());
                        popc->arguments.push_back(make_smart<ExprConstInt>(expr->at,stride));
                        return popc;
                    }
                }
            }
            if ( expr->left->type->isEnum() && expr->right->type->isEnum() )
                if ( !expr->left->type->isSameType(*expr->right->type,RefMatters::no, ConstMatters::no, TemporaryMatters::no) )
                    error("operations on different enumerations are prohibited", "", "",
                        expr->at, CompilationError::invalid_type);
            auto opName = "_::" + expr->op;
            auto tempCall = make_smart<ExprLooksLikeCall>(expr->at,opName);
            tempCall->arguments.push_back(expr->left);
            tempCall->arguments.push_back(expr->right);
            expr->func = inferFunctionCall(tempCall.get(),InferCallError::operatorOp2).get();
            if ( opName != tempCall->name ) {   // this happens when the operator gets instanced
                reportAstChanged();
                auto opCall = make_smart<ExprCall>(expr->at, tempCall->name);
                opCall->arguments = das::move(tempCall->arguments);
                return opCall;
            }
            if ( expr->func ) {
                if ( expr->func->firstArgReturnType ) {
                    expr->type = make_smart<TypeDecl>(*expr->arguments[0]->type);
                    expr->type->ref = false;
                } else {
                    expr->type = make_smart<TypeDecl>(*expr->func->result);
                }
                if ( !expr->func->arguments[0]->type->isRef() )
                    expr->left = Expression::autoDereference(expr->left);
                if ( !expr->func->arguments[1]->type->isRef() )
                    expr->right = Expression::autoDereference(expr->right);
                // lets try to fold it
                if ( expr->func && expr->func->unsafeOperation && !safeExpression(expr) ) {
                    error("unsafe operator " + expr->name + " requires unsafe", "", "",
                        expr->at, CompilationError::unsafe);
                } else if ( enableInferTimeFolding && isConstExprFunc(expr->func) ) {
                    auto lcc = getConstExpr(expr->left.get());
                    auto rcc = getConstExpr(expr->right.get());
                    if ( lcc && rcc ) {
                        expr->left = lcc;
                        expr->right = rcc;
                        return evalAndFold(expr);
                    }
                }
            }
            return Visitor::visit(expr);
        }
    // ExprOp3
        virtual ExpressionPtr visit ( ExprOp3 * expr ) override {
            if ( !expr->subexpr->type || !expr->left->type || !expr->right->type ) return Visitor::visit(expr);
            // infer
            if ( expr->op!="?" ) {
                error("Op3 currently only supports 'is'", "", "",
                    expr->at, CompilationError::operator_not_found);
                return Visitor::visit(expr);
            }
            expr->subexpr = Expression::autoDereference(expr->subexpr);
            if ( !expr->subexpr->type->isSimpleType(Type::tBool) ) {
                error("cond operator condition must be boolean", "", "",
                    expr->at, CompilationError::condition_must_be_bool);
            } else if ( !expr->left->type->isSameType(*expr->right->type,RefMatters::no, ConstMatters::no, TemporaryMatters::no) ) {
                error("cond operator must return the same types on both sides",
                    "\t" + (verbose ? (expr->left->type->describe() + " vs " + expr->right->type->describe()) :""), "",
                      expr->at, CompilationError::operator_not_found);
            } else {
                if ( expr->left->type->ref ^ expr->right->type->ref ) { // if either one is not ref
                    expr->left = Expression::autoDereference(expr->left);
                    expr->right = Expression::autoDereference(expr->right);
                }
                expr->type = make_smart<TypeDecl>(*expr->left->type);
                expr->type->constant |= expr->right->type->constant;
                // lets try to fold it
                if ( enableInferTimeFolding ) {
                    auto ccc = getConstExpr(expr->subexpr.get());
                    auto lcc = getConstExpr(expr->left.get());
                    auto rcc = getConstExpr(expr->right.get());
                    if ( ccc && lcc && rcc ) {
                        expr->subexpr = ccc;
                        expr->left = lcc;
                        expr->right = rcc;
                        return evalAndFold(expr);
                    }
                }
            }
            return Visitor::visit(expr);
        }
    // ExprMove
        bool canCopyOrMoveType ( const TypeDeclPtr & leftType, const TypeDeclPtr & rightType, TemporaryMatters tmatter, Expression * leftExpr ) const {
            if ( leftType->baseType==Type::tPointer ) {
                if ( !leftType->isVoidPointer() && rightType->isVoidPointer() ) return leftExpr->rtti_isNullPtr();  // anyptr = null ok, otherwise no void copy
                return leftType->isSameType(*rightType, RefMatters::no, ConstMatters::no, tmatter, AllowSubstitute::yes, true, true);
            } else {
                return leftType->isSameType(*rightType, RefMatters::no, ConstMatters::no, tmatter, AllowSubstitute::no, true, true);
            }
        }
        string moveErrorInfo(ExprMove * expr) const {
            if ( verbose ) {
                return ", " + describeType(expr->left->type) + " <- " + describeType(expr->right->type);
            } else {
                return "";
            }
        }
        virtual ExpressionPtr visit ( ExprMove * expr ) override {
            if ( !expr->left->type || !expr->right->type ) return Visitor::visit(expr);
            // infer
            if ( !canCopyOrMoveType(expr->left->type,expr->right->type,TemporaryMatters::no,expr->right.get()) ) {
                error("can only move compatible type"+moveErrorInfo(expr), "", "",
                    expr->at, CompilationError::operator_not_found);
            } else if ( !expr->left->type->isRef() ) {
                error("can only move to a reference"+moveErrorInfo(expr), "", "",
                    expr->at, CompilationError::cant_write_to_non_reference);
            } else if ( !expr->right->type->isRef() && !expr->right->type->isMoveableValue() ) {
                error("can only move to from a reference"+moveErrorInfo(expr), "", "",
                    expr->at, CompilationError::cant_write_to_non_reference);
            } else if ( expr->left->type->constant ) {
                error("can't move to a constant value"+moveErrorInfo(expr), "", "",
                    expr->at, CompilationError::cant_move_to_const);
            } else if ( !expr->left->type->canMove() ) {
                error("this type can't be moved, use clone (:=) instead"+moveErrorInfo(expr), "", "",
                    expr->at, CompilationError::cant_move);
            } else if ( expr->right->type->constant ) {
                error("can't move from a constant value"+moveErrorInfo(expr), "", "",
                    expr->at, CompilationError::cant_move);
            } else if ( expr->right->type->isTemp(true,false) ) {
                error("can't move temporary value"+moveErrorInfo(expr), "", "",
                    expr->at, CompilationError::cant_pass_temporary);
            } else if ( strictSmartPointers && !safeExpression(expr) && expr->right->type->needInScope() ) {
                error("moving values which contain smart pointers is unsafe",
                    "try `move(smart_ptr&) <| smart_ptr&` or `move_new(smart_ptr&) <| new [[YouTypeHere ...]]` instead", "",
                        expr->at, CompilationError::unsafe);
            } else if ( expr->right->type->isPointer() && expr->right->type->smartPtr ) {
                if ( !expr->right->type->ref && !safeExpression(expr) && !expr->right->rtti_isAscend() ) {
                    error("moving from the smart pointer value requires unsafe",  "",
                        "try moving from reference instead",
                        expr->at, CompilationError::unsafe);
                    return Visitor::visit(expr);
                }
            } else if ( expr->left->type->hasClasses() && !safeExpression(expr) ) {
                error("moving classes requires unsafe"+moveErrorInfo(expr), "", "",
                    expr->at, CompilationError::unsafe);
            } else if ( expr->left->type->lockCheck() || expr->right->type->lockCheck()) {
                if ( !expr->skipLockCheck && !(expr->at.fileInfo && expr->at.fileInfo->name=="builtin.das") && !(func && func->skipLockCheck) && !skipModuleLockChecks ) {
                    reportAstChanged();
                    auto pCall = make_smart<ExprCall>(expr->at,"_move_with_lockcheck");
                    pCall->arguments.push_back(expr->left->clone());
                    pCall->arguments.push_back(expr->right->clone());
                    return pCall;
                }
            }
            expr->type = make_smart<TypeDecl>();  // we return nothing
            return Visitor::visit(expr);
        }
    // ExprCopy
        string copyErrorInfo(ExprCopy * expr) const {
            if ( verbose ) {
                return ", " + describeType(expr->left->type) + " = " + describeType(expr->right->type);
            } else {
                return "";
            }
        }
        virtual ExpressionPtr visit ( ExprCopy * expr ) override {
            if ( !expr->left->type || !expr->right->type ) return Visitor::visit(expr);
            // infer
            if ( !canCopyOrMoveType(expr->left->type,expr->right->type,TemporaryMatters::no,expr->right.get()) ) {
                error("can only copy compatible type"+copyErrorInfo(expr), "", "",
                    expr->at, CompilationError::operator_not_found);
            } else if ( !expr->left->type->isRef() ) {
                error("can only copy to a reference"+copyErrorInfo(expr), "", "",
                    expr->at, CompilationError::cant_write_to_non_reference);
            } else if ( expr->left->type->constant ) {
                error("can't write to a constant value"+copyErrorInfo(expr), "", "",
                    expr->at, CompilationError::cant_write_to_const);
            } else if ( !expr->allowCopyTemp && expr->right->type->isTemp(true,false) ) {
                error("can't copy temporary value"+copyErrorInfo(expr), "", "",
                    expr->at, CompilationError::cant_pass_temporary);
            } else if ( expr->left->type->hasClasses() && !safeExpression(expr) ) {
                error("copying classes requires unsafe"+copyErrorInfo(expr), "", "",
                    expr->at, CompilationError::unsafe);
            }
            if ( !expr->left->type->canCopy() ) {
                error("this type can't be copied"+copyErrorInfo(expr),
                    "", "use move (<-) or clone (:=) instead", expr->at, CompilationError::cant_copy);
            }
            expr->type = make_smart<TypeDecl>();  // we return nothing
            return Visitor::visit(expr);
        }
    // ExprClone
        virtual void preVisit ( ExprClone * expr ) override {
            Visitor::preVisit(expr);
            if ( expr->left->rtti_isField() ) {
                auto field = static_pointer_cast<ExprField>(expr->left);
                field->underClone = expr;
            }
            expr->cloneSet.reset();
        }
        virtual ExpressionPtr visit ( ExprClone * expr ) override {
            if ( expr->cloneSet ) return expr->cloneSet;   // we've been promoted from underneath
            if ( !expr->left->type || !expr->right->type ) return Visitor::visit(expr);
            // lets see if there is clone operator already (a user operator can ignore all the rules bellow)
            auto fnList = getCloneFunc(expr->left->type, expr->right->type);
            if ( fnList.size() ) {
                if ( verifyCloneFunc(fnList, expr->at) ) {
                    reportAstChanged();
                    string cloneName = "_::clone";
                    auto cloneFn = make_smart<ExprCall>(expr->at, cloneName);
                    cloneFn->arguments.push_back(expr->left->clone());
                    cloneFn->arguments.push_back(expr->right->clone());
                    return ExpressionPtr(cloneFn);
                } else {
                    return Visitor::visit(expr);
                }
            }
            // infer
            if ( !isSameSmartPtrType(expr->left->type,expr->right->type,true) ) {
                error("can only clone the same type " + describeType(expr->left->type) + " vs " + describeType(expr->right->type), "", "",
                      expr->at, CompilationError::operator_not_found);
            } else if ( !expr->left->type->isRef() ) {
                error("can only clone to a reference", "", "",
                    expr->at, CompilationError::cant_write_to_non_reference);
            } else if ( expr->left->type->constant ) {
                error("can't write to a constant value", "", "",
                    expr->at, CompilationError::cant_write_to_const);
            } else if ( !expr->left->type->canClone() ) {
                error("type " + describeType(expr->left->type) + " can't be cloned from " + describeType(expr->right->type), "", "",
                    expr->at, CompilationError::cant_copy);
            } else {
                auto cloneType = expr->left->type;
                if ( cloneType->isHandle() ) {
                    expr->type = make_smart<TypeDecl>();  // we return nothing
                    return Visitor::visit(expr);
                } else if ( cloneType->isString() && (expr->right->type->isTemp() || multiContext) ) {
                    reportAstChanged();
                    auto cloneFn = make_smart<ExprCall>(expr->at, "clone_string");
                    cloneFn->arguments.push_back(expr->right->clone());
                    return make_smart<ExprCopy>(expr->at, expr->left->clone(), cloneFn);
                } else if ( cloneType->isPointer() && cloneType->smartPtr ) {
                    auto fnClone = makeCloneSmartPtr(expr->at, cloneType, expr->right->type);
                    if ( program->addFunction(fnClone) ) {
                        reportAstChanged();
                        auto cloneFn = make_smart<ExprCall>(expr->at, "_::clone");
                        cloneFn->arguments.push_back(expr->left->clone());
                        cloneFn->arguments.push_back(expr->right->clone());
                        return ExpressionPtr(cloneFn);
                    } else {
                        reportMissingFinalizer("smart pointer clone mismatch ", expr->at, cloneType);
                        return Visitor::visit(expr);
                    }
                } else if ( cloneType->canCopy(expr->right->type->isTemp() || multiContext) ) {
                    reportAstChanged();
                    auto eCopy = make_smart<ExprCopy>(expr->at, expr->left->clone(), expr->right->clone());
                    eCopy->allowCopyTemp = true;
                    return eCopy;
                } else if ( cloneType->isGoodArrayType() || cloneType->isGoodTableType() ) {
                    reportAstChanged();
                    auto cloneFn = make_smart<ExprCall>(expr->at, "_::clone");
                    cloneFn->arguments.push_back(expr->left->clone());
                    cloneFn->arguments.push_back(expr->right->clone());
                    return ExpressionPtr(cloneFn);
                } else if ( cloneType->isStructure() ) {
                    reportAstChanged();
                    auto stt = cloneType->structType;
                    fnList = getCloneFunc(cloneType,cloneType);
                    if ( verifyCloneFunc(fnList, expr->at) ) {
                        if ( fnList.size()==0 ) {
                            auto clf = makeClone(stt);
                            clf->privateFunction = true;
                            extraFunctions.push_back(clf);
                        }
                        auto cloneFn = make_smart<ExprCall>(expr->at, "_::clone");
                        cloneFn->arguments.push_back(expr->left->clone());
                        cloneFn->arguments.push_back(expr->right->clone());
                        return ExpressionPtr(cloneFn);
                    } else {
                        return Visitor::visit(expr);
                    }
                } else if ( cloneType->isTuple() ) {
                    reportAstChanged();
                    fnList = getCloneFunc(cloneType,cloneType);
                    if ( verifyCloneFunc(fnList, expr->at) ) {
                        if ( fnList.size()==0 ) {
                            auto clf = makeCloneTuple(expr->at, cloneType);
                            clf->privateFunction = true;
                            extraFunctions.push_back(clf);
                        }
                        auto cloneFn = make_smart<ExprCall>(expr->at, "_::clone");
                        cloneFn->arguments.push_back(expr->left->clone());
                        cloneFn->arguments.push_back(expr->right->clone());
                        return ExpressionPtr(cloneFn);
                    } else {
                        return Visitor::visit(expr);
                    }
                } else if ( cloneType->isVariant() ) {
                    reportAstChanged();
                    fnList = getCloneFunc(cloneType,cloneType);
                    if ( verifyCloneFunc(fnList, expr->at) ) {
                        if ( fnList.size()==0 ) {
                            auto clf = makeCloneVariant(expr->at, cloneType);
                            clf->privateFunction = true;
                            extraFunctions.push_back(clf);
                        }
                        auto cloneFn = make_smart<ExprCall>(expr->at, "_::clone");
                        cloneFn->arguments.push_back(expr->left->clone());
                        cloneFn->arguments.push_back(expr->right->clone());
                        return ExpressionPtr(cloneFn);
                    } else {
                        return Visitor::visit(expr);
                    }
                } else if ( cloneType->dim.size() ) {
                    reportAstChanged();
                    auto cloneFn = make_smart<ExprCall>(expr->at, "clone_dim");
                    cloneFn->arguments.push_back(expr->left->clone());
                    cloneFn->arguments.push_back(expr->right->clone());
                    return ExpressionPtr(cloneFn);
                } else {
                    error("this type can't be cloned " + describeType(cloneType), "", "",
                        expr->at, CompilationError::cant_copy);
                }
            }
            return Visitor::visit(expr);
        }
    // ExprTryCatch
        ExpressionPtr visit ( ExprTryCatch * expr ) override {
            if ( program->policies.jit ) {
                auto tryBlock = make_smart<ExprMakeBlock>(expr->try_block->at,expr->try_block);
                ((ExprBlock *)tryBlock->block.get())->returnType = make_smart<TypeDecl>(Type::autoinfer);
                auto catchBlock = make_smart<ExprMakeBlock>(expr->catch_block->at,expr->catch_block);
                ((ExprBlock *)catchBlock->block.get())->returnType = make_smart<TypeDecl>(Type::autoinfer);
                auto ccall = make_smart<ExprCall>(expr->at, "builtin_try_recover");
                ccall->arguments.push_back(tryBlock);
                ccall->arguments.push_back(catchBlock);
                return ccall;
            }
            return Visitor::visit(expr);
        }
    // ExprReturn
        bool inferReturnType ( TypeDeclPtr & resType, ExprReturn * expr ) {
            if ( resType->isAuto() ) {
                if ( expr->subexpr ) {
                    if (!expr->subexpr->type) {
                        error("subexpresion type is not resolved yet", "", "", expr->at);
                        return false;
                    } else if (expr->subexpr->type->isAutoOrAlias()) {
                        error("subexpresion type is not fully resolved yet", "", "", expr->at);
                        return true;
                    }
                    auto resT = TypeDecl::inferGenericType(resType, expr->subexpr->type, false, false, nullptr);
                    if ( !resT ) {
                        error("type can't be inferred, "
                              + describeType(resType) + ", returns " + describeType(expr->subexpr->type),"", "",
                              expr->at, CompilationError::cant_infer_mismatching_restrictions );
                    } else {
                        resT->ref = false;
                        TypeDecl::applyAutoContracts(resT, resType);
                        resType = resT;
                        reportAstChanged();
                        return true;
                    }
                } else {
                    resType = make_smart<TypeDecl>(Type::tVoid);
                    reportAstChanged();
                    return true;
                }
            }
            if ( resType->isVoid() ) {
                if ( expr->subexpr ) {
                    error("not expecting a return value", "", "",
                        expr->at, CompilationError::not_expecting_return_value);
                }
            } else {
                if ( !expr->subexpr ) {
                    error("expecting a return value", "", "",
                        expr->at, CompilationError::expecting_return_value);
                } else {
                    if ( !canCopyOrMoveType(resType,expr->subexpr->type,TemporaryMatters::yes,expr->subexpr.get()) ) {
                        error("incompatible return type, expecting "
                              + describeType(resType) + ", passing " + describeType(expr->subexpr->type), "", "",
                              expr->at, CompilationError::invalid_return_type);
                    }
                    if ( resType->ref && !expr->subexpr->type->isRef() ) {
                        error("incompatible return type, reference matters. expecting "
                              + describeType(resType) + ", passing " + describeType(expr->subexpr->type), "", "",
                              expr->at, CompilationError::invalid_return_type);
                    }
                    if ( resType->isRef() && !resType->isConst() && expr->subexpr->type->isConst() ) {
                        error("incompatible return type, constant matters. expecting "
                              + describeType(resType) + ", passing " + describeType(expr->subexpr->type), "", "",
                              expr->at, CompilationError::invalid_return_type);
                    }
                }
            }
            if ( resType->isRefType() ) {
                if ( !resType->canCopy() && !resType->canMove() ) {
                    error("this type can't be returned at all " + describeType(resType), "", "",
                          expr->at, CompilationError::invalid_return_type);
                }
            }
            if ( expr->moveSemantics && expr->subexpr->type->isConst() ) {
                error("can't return via move from a constant value", "", "",
                    expr->at, CompilationError::cant_move);
            }
            return false;
        }
        virtual void preVisit ( ExprReturn * expr ) override {
            Visitor::preVisit(expr);
            expr->block = nullptr;
            expr->returnType.reset();
            // ok, now lets mark early outs for the block chain
            auto i = scopes.size();
            while ( i>0 ) {
                i --;
                scopes[i]->hasEarlyOut = true;
                if ( scopes[i]->isClosure ) break;
            }
        }
        virtual ExpressionPtr visit ( ExprReturn * expr ) override {
            if ( blocks.size() ) {
                ExprBlock * block = blocks.back();
                expr->block = block;
                block->hasReturn = true;
                if ( expr->subexpr ) {
                    if ( !expr->subexpr->type ) return Visitor::visit(expr);
                    if ( !block->returnType->ref ) {
                        expr->subexpr = Expression::autoDereference(expr->subexpr);
                    } else {
                        expr->returnReference = true;
                    }
                    expr->returnInBlock = true;
                }
                if ( inferReturnType(block->type, expr) ) {
                    block->returnType = make_smart<TypeDecl>(*block->type);
                    setBlockCopyMoveFlags(block);
                }
                if ( block->moveOnReturn && !expr->moveSemantics ) {
                    error("this type can't be copied; " + describeType(block->type),"","use return <- instead",
                          expr->at, CompilationError::invalid_return_semantics );
                }
                if ( block->returnType && block->returnType->ref && !safeExpression(expr) ) {
                    error("returning reference requires unsafe", "", "",
                        func->result->at,CompilationError::invalid_return_type);
                }
                if ( block->returnType && block->returnType->isTemp() && !safeExpression(expr) ) {
                    error("returning temporary value from block requires unsafe", "", "",
                        func->result->at,CompilationError::invalid_return_type);
                }
                if ( strictSmartPointers && block->returnType && !expr->moveSemantics && !safeExpression(expr) && block->returnType->needInScope() ) {
                    error("returning smart pointers without move semantics is unsafe", "use return <- instead", "",
                        expr->at,CompilationError::unsafe);
                }
                if ( block->returnType ) expr->returnType = make_smart<TypeDecl>(*block->returnType);
            } else {
                // infer
                func->hasReturn = true;
                if ( expr->subexpr ) {
                    if ( !expr->subexpr->type ) return Visitor::visit(expr);
                    if ( !func->result->ref ) {
                        if ( !expr->moveSemantics ) {
                            expr->subexpr = Expression::autoDereference(expr->subexpr);
                        }
                    } else {
                        expr->returnReference = true;
                    }
                }
                inferReturnType(func->result, expr);
                if ( func->moveOnReturn && !expr->moveSemantics ) {
                    error("this type can't be copied; " + describeType(func->result),"","use return <- instead",
                          expr->at, CompilationError::invalid_return_semantics );
                }
                if ( func->result->ref && !safeExpression(expr) ) {
                    error("returning reference requires unsafe", "", "",
                        func->result->at,CompilationError::invalid_return_type);
                }
                if ( func->result->isTemp() && !safeExpression(expr) ) {
                    error("returning temporary value from function requires unsafe", "", "",
                        func->result->at,CompilationError::invalid_return_type);
                }
                if ( strictSmartPointers && !expr->moveSemantics && !safeExpression(expr) && func->result->needInScope() ) {
                    error("returning smart pointers without move semantics is unsafe", "use return <- instead", "",
                        expr->at,CompilationError::unsafe);
                }
               if ( func->result ) expr->returnType = make_smart<TypeDecl>(*func->result);
            }
            if ( expr->moveSemantics && expr->subexpr && expr->subexpr->type && expr->subexpr->type->lockCheck() ) {
                if ( !(expr->at.fileInfo && expr->at.fileInfo->name=="builtin.das") ) {
                    bool checkIt = true;
                    if ( expr->subexpr->rtti_isCall() ) {
                        auto ccall = static_pointer_cast<ExprCall>(expr->subexpr);
                        if ( ccall->name=="_return_with_lockcheck" || ccall->name=="__::builtin`_return_with_lockcheck" ) checkIt = false;
                    }
                    if ( checkIt && !expr->skipLockCheck && !(func && func->skipLockCheck) && !skipModuleLockChecks ) {
                        reportAstChanged();
                        auto pCall = make_smart<ExprCall>(expr->at,"_return_with_lockcheck");
                        pCall->arguments.push_back(expr->subexpr->clone());
                        auto pRet = expr->clone();
                        static_pointer_cast<ExprReturn>(pRet)->subexpr = pCall;
                        return pRet;
                    }
                }
            }
            expr->type = make_smart<TypeDecl>();
            return Visitor::visit(expr);
        }
    // ExprYield
        virtual ExpressionPtr visit ( ExprYield * expr ) override {
            if ( blocks.size() ) {
                error("can't yield from inside the block", "", "",
                      expr->at, CompilationError::invalid_yield );
            } else {
                if ( !func->generator ) {
                    error("can't yield from non-generator", "", "",
                          expr->at, CompilationError::invalid_yield );
                    return Visitor::visit(expr);
                }
                if ( !expr->subexpr->type ) return Visitor::visit(expr);
                // const auto & yarg = func->arguments[1];
                // TODO: verify yield type so that error is 'yield' error, not copy or move error
                auto blk = generateYield(expr, func);
                scopes.back()->needCollapse = true;
                reportAstChanged();
                return blk;
            }
            expr->type = make_smart<TypeDecl>();
            return Visitor::visit(expr);
        }

    // ExprBreak
        virtual ExpressionPtr visit ( ExprBreak * expr ) override {
            if ( !loop.size() )
                error("'break' without a loop",  "", "",
                    expr->at, CompilationError::invalid_loop);
            return Visitor::visit(expr);
        }
    // ExprContinue
        virtual ExpressionPtr visit ( ExprContinue * expr ) override {
            if ( !loop.size() )
                error("'continue' without a loop",  "", "",
                    expr->at, CompilationError::invalid_loop);
            return Visitor::visit(expr);
        }
    // ExprIfThenElse
        bool isConstExprFunc(Function * fun) const {
            return (fun->sideEffectFlags == 0) && (fun->builtIn) && (fun->result->isFoldable());
        }
        ExpressionPtr getConstExpr ( Expression * expr ) {
            if ( expr->rtti_isConstant() && expr->type && expr->type->isFoldable() ) {
                if ( expr->type->isEnum() ) {
                    auto enumc = static_cast<ExprConstEnumeration *>(expr);
                    auto enumv = enumc->enumType->find(enumc->text);
                    if ( !enumv.second ) return nullptr;                        // not found???
                    if ( !enumv.first || !enumv.first->type ) return nullptr;   // not resolved
                    if ( !enumv.first->rtti_isConstant() ) return nullptr;      // not a constant
                    // TODO: do we need to check if const is of the same size?
                    // if ( enumc->baseType != enumv.first->type->baseType ) return nullptr;   // not a constant of the same type
                }
                return expr;
            }
            if ( expr->rtti_isR2V() ) {
                auto r2v = static_cast<ExprRef2Value *>(expr);
                return getConstExpr(r2v->subexpr.get());
            }
            if ( expr->rtti_isVar() ) { // global variable which happens to be constant
                auto var = static_cast<ExprVar *>(expr);
                auto variable = var->variable;
                if ( variable && variable->init && variable->type->isConst() && variable->type->isFoldable() ) {
                    if ( !var->local && !var->argument && !var->block ) {
                        if ( variable->init->rtti_isConstant() ) {
                            return variable->init;
                        }
                    }
                }
            }
            return nullptr;
        }
        virtual bool canVisitIfSubexpr ( ExprIfThenElse * expr ) override {
            if ( expr->isStatic ) {
                // static_if prevents normal resolve flow
                //  so we say 'hasReturn' for current block or function
                //  to prevent it from becoming void
                //  until static_if resolves that is
                if ( blocks.size() ) {
                    ExprBlock * block = blocks.back();
                    block->hasReturn = true;
                } else {
                    func->hasReturn = true;
                }
            }
            return !expr->isStatic;
        }
        virtual ExpressionPtr visit ( ExprIfThenElse * expr ) override {
            if ( !expr->cond->type ) {
                return Visitor::visit(expr);
            }
            // infer
            if ( !expr->cond->type->isSimpleType(Type::tBool) ) {
                error("if-then-else condition must be boolean, and not " + expr->cond->type->describe(),  "", "",
                    expr->at, CompilationError::condition_must_be_bool);
                return Visitor::visit(expr);
            }
            if ( expr->cond->type->isRef() ) {
                expr->cond = Expression::autoDereference(expr->cond);
            }
            // now, for the static if
            if ( (enableInferTimeFolding && !expr->doNotFold) || expr->isStatic ) {
                if ( auto constCond = getConstExpr(expr->cond.get()) ) {
                    reportAstChanged();
                    auto condR = static_pointer_cast<ExprConstBool>(constCond)->getValue();
                    if ( condR ) {
                        return expr->if_true;
                    } else {
                        return expr->if_false;
                    }
                } else if ( expr->isStatic ) {
                    error("static_if must resolve to constant", "", "",
                        expr->at, CompilationError::condition_must_be_static);
                    return Visitor::visit(expr);
                }
            }
            // now, to unwrap the generator
            if ( func && func->generator ) {
                // only topmost
                //  which in case of generator is 2, due to
                //  def GENERATOR { with LAMBDA { ...collapse here... } }
                if ( !blocks.empty() /* || scopes.size()!=2 */ ) {
                    return Visitor::visit(expr);
                }
                uint32_t tf = expr->if_true->getEvalFlags();
                uint32_t ff = expr->if_false ? expr->if_false->getEvalFlags() : 0;
                if ( (tf|ff) & EvalFlags::yield ) { // only unwrap if it has "yield"
                    // verify if it can be cloned at all
                    if ( expr->if_true->rtti_isBlock() ){
                        auto iftb = static_pointer_cast<ExprBlock>(expr->if_true);
                        if ( !iftb->finalList.empty() ) {
                            error("can't have final section in the if-then-else inside generator yet", "", "",
                                  expr->at, CompilationError::invalid_yield);
                            return Visitor::visit(expr);
                        }
                    }
                    if ( expr->if_false && expr->if_false->rtti_isBlock() ){
                        auto iffb = static_pointer_cast<ExprBlock>(expr->if_false);
                        if ( !iffb->finalList.empty() ) {
                            error("can't have final section in the if-then-else inside generator yet", "", "",
                                  expr->at, CompilationError::invalid_yield);
                            return Visitor::visit(expr);
                        }
                    }
                    auto blk = replaceGeneratorIfThenElse(expr, func);
                    scopes.back()->needCollapse = true;
                    reportAstChanged();
                    return blk;
                }
            }
            return Visitor::visit(expr);
        }
    // ExprAssume
        virtual void preVisit ( ExprAssume * expr ) override {
            Visitor::preVisit(expr);
            assume.push_back(expr);
        }
    // ExprWith
        virtual void preVisit ( ExprWith * expr ) override {
            Visitor::preVisit(expr);
            with.push_back(expr);
        }
        virtual ExpressionPtr visit ( ExprWith * expr ) override {
            if ( auto wT = expr->with->type ) {
                StructurePtr pSt;
                if ( wT->dim.size() ) {
                    error("with array in undefined, " + describeType(wT), "", "",
                        expr->at, CompilationError::invalid_with_type );
                } else if ( wT->isStructure() ) {
                    pSt = wT->structType;
                } else if ( wT->isPointer() && wT->firstType && wT->firstType->isStructure() ) {
                    pSt = wT->firstType->structType;
                } else {
                    error("unexpected with type " + describeType(wT), "", "",
                        expr->at, CompilationError::invalid_with_type );
                }
                if ( pSt ) {
                    for ( auto fi : pSt->fields ) {
                        for ( auto & lv : local ) {
                            if ( lv->name==fi.name ) {
                                error("with expression shadows local variable " +
                                      lv->name + " at line " + to_string(lv->at.line), "", "",
                                        expr->at, CompilationError::variable_not_found);
                            }
                        }
                    }
                }
            }
            with.pop_back();
            return Visitor::visit(expr);
        }
    // ExprWhile
        virtual void preVisit ( ExprWhile * expr ) override {
            Visitor::preVisit(expr);
            loop.push_back(expr);
        }
        virtual ExpressionPtr visit ( ExprWhile * expr ) override {
            loop.pop_back();
            if ( !expr->cond->type ) return Visitor::visit(expr);
            // infer
            if ( !expr->cond->type->isSimpleType(Type::tBool) ) {
                error("while loop condition must be boolean", "", "",
                    expr->at, CompilationError::invalid_loop);
            }
            if ( expr->cond->type->isRef() ) {
                expr->cond = Expression::autoDereference(expr->cond);
            }
            // now, to unwrap the generator
            if ( func && func->generator ) {
                // only topmost
                //  which in case of generator is 2, due to
                //  def GENERATOR { with LAMBDA { ...collapse here... } }
                if ( !blocks.empty() /* || scopes.size()!=2 */ ) {
                    return Visitor::visit(expr);
                }
                uint32_t tf = expr->body->getEvalFlags();
                if ( tf & EvalFlags::yield ) { // only unwrap if it has "yield"
                    auto blk = replaceGeneratorWhile(expr, func);
                    scopes.back()->needCollapse = true;
                    reportAstChanged();
                    return blk;
                }
            }
            return Visitor::visit(expr);
        }
    // ExprFor
        virtual void preVisit ( ExprFor * expr ) override {
            Visitor::preVisit(expr);
            // macro generated invisible variables
            // DAS_ASSERT(expr->visibility.line);
            loop.push_back(expr);
            pushVarStack();
        }
        virtual void preVisitForStack ( ExprFor * expr ) override {
            Visitor::preVisitForStack(expr);
            if ( !expr->iterators.size() ) {
                error("for loop needs at least one iterator", "", "",
                    expr->at, CompilationError::invalid_loop);
                return;
            } else if ( expr->iterators.size() != expr->sources.size() ) {
                error("for loop needs as many iterators as there are sources", "", "",
                    expr->at, CompilationError::invalid_loop);
                return;
            }
            // iterator variables
            int idx = 0;
            expr->iteratorVariables.clear();
            for ( auto & src : expr->sources ) {
                if (!src->type) {
                    idx++;
                    continue;
                }
                auto pVar = make_smart<Variable>();
                pVar->name = expr->iterators[idx];
                pVar->aka = expr->iteratorsAka[idx];
                pVar->at = expr->iteratorsAt[idx];
                if ( src->type->dim.size() ) {
                    pVar->type = make_smart<TypeDecl>(*src->type);
                    pVar->type->ref = true;
                    pVar->type->dim.erase(pVar->type->dim.begin());
                    if ( !pVar->type->dimExpr.empty() ) {
                        pVar->type->dimExpr.erase(pVar->type->dimExpr.begin());
                    }
                } else if ( src->type->isGoodIteratorType() ) {
                    pVar->type = make_smart<TypeDecl>(*src->type->firstType);
                } else if ( src->type->isGoodArrayType() ) {
                    pVar->type = make_smart<TypeDecl>(*src->type->firstType);
                    pVar->type->ref = true;
                } else if ( src->type->isRange() ) {
                    pVar->type = make_smart<TypeDecl>(src->type->getRangeBaseType());
                    pVar->type->ref = false;
                    pVar->type->constant = true;
                } else if ( src->type->isString() ) {
                    pVar->type = make_smart<TypeDecl>(Type::tInt);
                    pVar->type->ref = false;
                    pVar->type->constant = true;
                } else if ( src->type->isHandle() && src->type->annotation->isIterable() ) {
                    pVar->type = make_smart<TypeDecl>(*src->type->annotation->makeIteratorType(src));
                } else {
                    error("unsupported iteration type " + src->type->describe() + " for the loop variable " + pVar->name + ", iterating over " + describeType(src->type), "", "",
                        expr->at, CompilationError::invalid_iteration_source);
                    return;
                }
                pVar->type->constant |= src->type->isConst();
                pVar->type->temporary |= src->type->isTemp();
                pVar->source = src;
                pVar->can_shadow = expr->canShadow;
                if ( !pVar->can_shadow && !program->policies.allow_local_variable_shadowing ) {
                    if ( func ) {
                        for ( auto & fna : func->arguments ) {
                            if ( fna->name==pVar->name ) {
                                error("for loop iterator variable " + pVar->name +" is shadowed by function argument "
                                    + fna->name + " : " + describeType(fna->type) + " at line " + to_string(fna->at.line), "", "",
                                        pVar->at, CompilationError::variable_not_found);
                            }
                        }
                    }
                    for ( auto & blk : blocks ) {
                        for ( auto & bna : blk->arguments ) {
                            if ( bna->name==pVar->name ) {
                                error("for loop iterator variable " + pVar->name +" is shadowed by block argument "
                                    + bna->name + " : " + describeType(bna->type) + " at line " + to_string(bna->at.line), "", "",
                                        pVar->at, CompilationError::variable_not_found);
                            }
                        }
                    }
                    for ( auto & lv : local ) {
                        if ( lv->name==pVar->name ) {
                            error("for loop iterator variable " + pVar->name +" is shadowed by another local variable "
                                + lv->name + " : " + describeType(lv->type) + " at line " + to_string(lv->at.line), "", "",
                                pVar->at, CompilationError::variable_not_found);
                            break;
                        }
                    }
                    if ( auto eW = hasMatchingWith(pVar->name) ) {
                        error("for loop iterator variable " + pVar->name + " is shadowed by with expression at line " + to_string(eW->at.line), "", "",
                            pVar->at, CompilationError::variable_not_found);
                    }
                }
                local.push_back(pVar);
                expr->iteratorVariables.push_back(pVar);
                ++ idx;
            }
        }
        virtual void preVisitForSource ( ExprFor * expr, Expression * that, bool last ) override {
            Visitor::preVisitForSource(expr,that,last);
            that->isForLoopSource = true;
        }
        virtual ExpressionPtr visitForSource ( ExprFor * expr, Expression * that , bool last ) override {
            if ( program->policies.jit & that->type && that->type->isHandle() && that->type->annotation->isIterable() ) {
                reportAstChanged();
                auto eachFn = make_smart<ExprCall>(expr->at, "each");
                eachFn->arguments.push_back(that->clone());
                return eachFn;
            }
            if ( that->type && that->type->isRef() ) {
                return Expression::autoDereference(that);
            }
            return Visitor::visitForSource(expr, that, last);
        }
        virtual ExpressionPtr visit ( ExprFor * expr ) override {
            popVarStack();
            loop.pop_back();
            // now, to unwrap the generator
            if ( func && func->generator ) {
                // only fully resolved loop
                if ( expr->sources.empty() ) {
                    return Visitor::visit(expr);
                } else if ( expr->iteratorVariables.size() != expr->sources.size() ) {
                    return Visitor::visit(expr);
                }
                // only topmost
                //  which in case of generator is 2, due to
                //  def GENERATOR { with LAMBDA { ...collapse here... } }
                if ( !blocks.empty() /* || scopes.size()!=2 */ ) {
                    return Visitor::visit(expr);
                }
                uint32_t tf = expr->body->getEvalFlags();
                if ( tf & EvalFlags::yield ) { // only unwrap if it has "yield"
                    auto blk = replaceGeneratorFor(expr, func);
                    scopes.back()->needCollapse = true;
                    reportAstChanged();
                    return blk;
                }
            }
            // implement for loop macro
            ExpressionPtr substitute;
            auto thisModule = ctx.thisProgram->thisModule.get();
            auto modMacro = [&](Module * mod) -> bool {
                if ( thisModule->isVisibleDirectly(mod) && mod!=thisModule ) {
                    for ( const auto & pm : mod->forLoopMacros ) {
                        if ( (substitute = pm->visit(ctx.thisProgram, thisModule, expr)) ) {
                            return false;
                        }
                    }
                }
                return true;
            };
            Module::foreach(modMacro);
            if ( !substitute ) ctx.thisProgram->library.foreach(modMacro, "*");
            if ( substitute ) {
                reportAstChanged();
                return substitute;
            }
            return Visitor::visit(expr);
        }
    // ExprLet
        virtual void preVisit ( ExprLet * expr ) override {
            Visitor::preVisit(expr);
            DAS_ASSERT(!scopes.empty());
            auto scope = scopes.back();
            expr->visibility.fileInfo = expr->at.fileInfo;
            expr->visibility.column = expr->atInit.last_column;
            expr->visibility.line = expr->atInit.last_line;
            expr->visibility.last_column = scope->at.last_column;
            expr->visibility.last_line = scope->at.last_line;
            // macro generated invisible variable
            // DAS_ASSERTF(expr->visibility.line,"visiblity failed at %s",expr->at.describe().c_str());
            if ( expr->inScope && scopes.back()->inTheLoop ) {
                error("in scope let is not allowed in the loop",
                    "you can always create scope with 'if true'", "", expr->at, CompilationError::in_scope_in_the_loop);
            }
        }
        virtual void preVisitLet ( ExprLet * expr, const VariablePtr & var, bool last ) override {
            Visitor::preVisitLet(expr, var, last);
            if ( var->type && var->type->isExprType() ) {
                return;
            }
            if ( var->type->isAlias() ) {
                auto aT = inferAlias(var->type);
                if ( aT ) {
                    var->type = aT;
                    var->type->sanitize();
                    reportAstChanged();
                } else {
                    error("undefined let type " + describeType(var->type),
                        reportInferAliasErrors(var->type), "", var->at, CompilationError::type_not_found);
                }
            }
            if ( var->type->isAuto() && !var->init) {
                error("local variable type can't be inferred, it needs an initializer", "", "",
                      var->at, CompilationError::cant_infer_missing_initializer );
            }
            if ( !var->can_shadow && !program->policies.allow_local_variable_shadowing ) {
                if ( func ) {
                    for ( auto & fna : func->arguments ) {
                        if ( fna->name==var->name ) {
                            error("local variable " + var->name +" is shadowed by function argument "
                                + fna->name + " : " + describeType(fna->type) + " at line " + to_string(fna->at.line), "", "",
                                    var->at, CompilationError::variable_not_found);
                        }
                    }
                }
                for ( auto & blk : blocks ) {
                    for ( auto & bna : blk->arguments ) {
                        if ( bna->name==var->name ) {
                            error("local variable " + var->name +" is shadowed by block argument "
                                + bna->name + " : " + describeType(bna->type) + " at line " + to_string(bna->at.line), "", "",
                                    var->at, CompilationError::variable_not_found);
                        }
                    }
                }
                for ( auto & lv : local ) {
                    if ( lv->name==var->name ) {
                        error("local variable " + var->name +" is shadowed by another local variable "
                            + lv->name + " : " + describeType(lv->type) + " at line " + to_string(lv->at.line), "", "",
                            var->at, CompilationError::variable_not_found);
                        break;
                    }
                }
                if ( auto eW = hasMatchingWith(var->name) ) {
                    error("local variable " + var->name + " is shadowed by with expression at line " + to_string(eW->at.line), "", "",
                        var->at, CompilationError::variable_not_found);
                }
            }
            if ( !var->init ) {
                local.push_back(var);
            }
            if ( expr->inScope && var->type && var->type->isConst() ) {
                error("in scope let can't be const", "", "", var->at, CompilationError::invalid_variable_type);
            }
        }
        virtual VariablePtr visitLet ( ExprLet * expr, const VariablePtr & var, bool last ) override {
            if ( var->type && var->type->isExprType() ) {
                return Visitor::visitLet(expr,var,last);
            }
            if ( isCircularType(var->type) ) {
                return Visitor::visitLet(expr,var,last);
            }
            if ( var->type->ref && !var->init )
                error("local reference has to be initialized", "", "",
                      var->at, CompilationError::invalid_variable_type);
            if ( var->type->ref && var->init && !(var->init->alwaysSafe || isLocalOrGlobal(var->init)) && !safeExpression(expr) && !(var->init->type && var->init->type->temporary) ) {
                if ( program->policies.local_ref_is_unsafe ) {
                    error("local reference to non-local expression is unsafe", "", "",
                        var->at, CompilationError::unsafe);
                }
            }
            if ( var->type->isVoid() )
                error("local variable can't be declared void", "", "",
                      var->at, CompilationError::invalid_variable_type);
            if ( !var->type->isLocal() && !var->type->ref && !safeExpression(expr) ) {
                if ( var->type->isGoodBlockType() ) {
                    if ( !var->init ) {
                        error("local block variable needs to be initialized", "", "",
                            var->at, CompilationError::invalid_variable_type);
                    }
                } else {
                    error("local variable of type " + describeType(var->type) + " requires unsafe", "", "",
                        var->at, CompilationError::invalid_variable_type);
                }
            }
            if ( !var->type->ref && var->type->hasClasses() && !safeExpression(expr) ) {
                error("local class requires unsafe " + describeType(var->type), "", "",
                      var->at, CompilationError::unsafe);
            }
            if ( !var->type->isAutoOrAlias() ){
                if ( !var->init && var->type->isLocal() ) { // we already report error for non-local
                    if ( var->type->hasNonTrivialCtor() ) {
                        error("local variable of type " + describeType(var->type) + " needs to be initialized", "", "",
                            var->at, CompilationError::invalid_variable_type);
                    }
                } else if ( var->init && var->init->rtti_isCast() ) {
                    auto castExpr = static_pointer_cast<ExprCast>(var->init);
                    if ( castExpr->castType->isAuto() ) {
                        reportAstChanged();
                        castExpr->castType = make_smart<TypeDecl>(*var->type);
                    }
                }
                if ( expr->inScope ) {
                    if ( !var->inScope ) {
                        if ( var->type->canDelete() ) {
                            var->inScope = true;
                            auto eVar = make_smart<ExprVar>(var->at, var->name);
                            auto exprDel = make_smart<ExprDelete>(var->at, eVar);
                            scopes.back()->finalList.insert(scopes.back()->finalList.begin(), exprDel);
                            reportAstChanged();
                        } else {
                            error("can't delete " + describeType(var->type), "", "",
                                var->at, CompilationError::bad_delete);
                        }
                    }
                } else {
                    if ( strictSmartPointers && !var->generated && !safeExpression(expr) && var->type->needInScope() ) {
                        error("variable " + var->name + " of type " + describeType(var->type) + " requires var inscope to be safe", "", "",
                            var->at, CompilationError::invalid_variable_type);
                    }
                }
            }
            verifyType(var->type);
            return Visitor::visitLet(expr,var,last);
        }
        ExpressionPtr promoteToCloneToMove ( const VariablePtr & var ) {
            reportAstChanged();
            var->init_via_clone = false;
            var->init_via_move = true;
            auto c2m = make_smart<ExprCall>(var->at,"clone_to_move");
            c2m->arguments.push_back(var->init);
            return c2m;
        }
        virtual ExpressionPtr visitLetInit ( ExprLet * expr, const VariablePtr & var, Expression * init ) override {
            local.push_back(var);
            if ( !var->init->type ) {
                return Visitor::visitLetInit(expr, var, init);
            }
            if ( var->type->isAuto() ) {
                auto varT = TypeDecl::inferGenericInitType(var->type, var->init->type);
                if ( !varT || varT->isAlias() ) {
                    error("local variable " + var->name + " initialization type can't be inferred, "
                          + describeType(var->type) + " = " + describeType(var->init->type), "", "",
                          var->at, CompilationError::cant_infer_mismatching_restrictions );
                } else if ( varT->isVoid() ) {
                    error("local variable " + var->name + " initialization can't be void, "
                          + describeType(var->type), "", "",
                          var->at, CompilationError::cant_infer_mismatching_restrictions );
                } else {
                    varT->ref = false;
                    TypeDecl::applyAutoContracts(varT, var->type);
                    var->type = varT;
                    var->type->sanitize();
                    reportAstChanged();
                }
            } else if ( !canCopyOrMoveType(var->type,var->init->type,TemporaryMatters::no,var->init.get()) ) {
                error("local variable " + var->name + " initialization type mismatch, "
                      + describeType(var->type) + " = " + describeType(var->init->type), "", "",
                    var->at, CompilationError::invalid_initialization_type);
            } else if ( var->type->ref && !var->init->type->isRef()) {
                error("local variable " + var->name + " initialization type mismatch. reference can't be initialized via value, "
                      + describeType(var->type) + " = " + describeType(var->init->type), "", "",
                    var->at, CompilationError::invalid_initialization_type);
            } else if ( var->type->ref &&  !var->type->isConst() && var->init->type->isConst() ) {
                error("local variable " + var->name + " initialization type mismatch. const matters, "
                      + describeType(var->type) + " = " + describeType(var->init->type), "", "",
                    var->at, CompilationError::invalid_initialization_type);

            } else if ( !var->type->ref && var->type->isGoodBlockType() ) {
                if ( !var->init->rtti_isMakeBlock() ) {
                    error("local variable " + var->name + " can only be initialized with make block expression", "", "",
                        var->at, CompilationError::invalid_initialization_type);
                }
            } else if ( !var->type->ref && !var->init->type->canCopy() && !var->init->type->canMove()
                        && var->type->hasNonTrivialCtor() && !var->isCtorInitialized() ) {
                error("local variable " + var->name + " can only be initialized with type constructor", "", "",
                    var->at, CompilationError::invalid_initialization_type);
            } else if ( !var->type->ref && !var->init->type->canCopy() && !var->init->type->canMove()
                        && var->type->hasNonTrivialCtor() && var->isCtorInitialized() && var->init_via_move ) {
                error("local variable " + var->name + " can only be initialized with copy", "", "",
                    var->at, CompilationError::invalid_initialization_type);
            } else if ( !var->type->ref && !var->init->type->canCopy() && !var->init->type->canMove()
                        && !var->type->hasNonTrivialCtor() ) {
                error("local variable " + var->name + " can't be initialized at all", "", "",
                    var->at, CompilationError::invalid_initialization_type);
            } else if ( !var->type->ref && !var->init->type->canCopy()
                       && var->init->type->canMove() && !(var->init_via_move || var->init_via_clone) ) {
                error("local variable " + var->name + " can only be move-initialized","","use <- for that",
                    var->at, CompilationError::invalid_initialization_type);
            } else if ( var->init_via_move && var->init->type->isConst() ) {
                error("local variable " + var->name + " can't init (move) from a constant value", "", "",
                    var->at, CompilationError::cant_move);
            } else if ( var->init_via_clone && !var->init->type->canClone() ) {
                auto varType = make_smart<TypeDecl>(*var->type);
                varType->ref = true;
                auto fnList = getCloneFunc(varType, var->init->type);
                if ( fnList.size() && verifyCloneFunc(fnList, expr->at) ) {
                    return promoteToCloneToMove(var);
                } else {
                    error("local variable " + var->name + " of type " + describeType(var->type)
                    + " can't be cloned from " + describeType(var->init->type),"", "",
                      var->at, CompilationError::cant_copy);
                }
            } else {
                if ( var->init_via_clone ) {
                    return promoteToCloneToMove(var);
                }
            }
            return Visitor::visitLetInit(expr, var, init);
        }
        virtual ExpressionPtr visit ( ExprLet * expr ) override {
            if ( func && func->generator ) {
                // only topmost
                //  which in case of generator is 2, due to
                //  def GENERATOR { with LAMBDA { ...collapse here... } }
                if ( !blocks.empty() || scopes.size()!=2 ) {
                    return Visitor::visit(expr);
                }
                for ( auto & var : expr->variables ) {
                    // TODO: verify
                    // if ( !isFullySealedType(var->type) ) {
                    if ( var->type->isAutoOrAlias() ) {
                        error("type not ready yet", "", "", var->at);
                        return Visitor::visit(expr);
                    }
                }
                auto blk = replaceGeneratorLet(expr, func, scopes.back());
                scopes.back()->needCollapse = true;
                // need to update finalizer
                auto finFuncs = getFinalizeFunc ( func->arguments[0]->type );
                if ( finFuncs.size()==1 ) {
                    auto finFunc = finFuncs.back();
                    auto stype = func->arguments[0]->type->structType;
                    auto lname = stype->name;
                    auto newFinalizer = generateStructureFinalizer(stype);
                    finFunc->body = newFinalizer->body;
                }
                // ---
                reportAstChanged();
                return blk;
            }
            if ( scopes.size() ) {
                auto hasEarlyOut = scopes.back()->hasEarlyOut;
                expr->hasEarlyOut = hasEarlyOut;
                for ( auto & var : expr->variables ) {
                    var->early_out = hasEarlyOut;
                }
            }
            return Visitor::visit(expr);
        }
    // ExprCallMacro
        virtual void preVisit ( ExprCallMacro * expr ) override {
            auto errc = ctx.thisProgram->errors.size();
            auto thisModule = ctx.thisProgram->thisModule.get();
            canFoldResult = expr->macro->canFoldReturnResult(expr) && canFoldResult;
            expr->macro->preVisit(ctx.thisProgram, thisModule, expr);
            if ( errc==ctx.thisProgram->errors.size() ) {
                error("unsupported call macro " + expr->macro->name + "", "potentially missing require", "",
                    expr->at, CompilationError::unsupported_call_macro);
            }
            return Visitor::preVisit(expr);
        }
        virtual ExpressionPtr visit ( ExprCallMacro * expr ) override {
            auto errc = ctx.thisProgram->errors.size();
            auto thisModule = ctx.thisProgram->thisModule.get();
            auto substitute = expr->macro->visit(ctx.thisProgram, thisModule, expr);
            if ( substitute ) {
                reportAstChanged();
                return substitute;
            }
            if ( errc==ctx.thisProgram->errors.size() ) {
                error("unsupported call macro " + expr->macro->name,  "", "",
                    expr->at, CompilationError::unsupported_call_macro);
            }
            return Visitor::visit(expr);
        }
    // ExprLooksLikeCall
        virtual void preVisit ( ExprLooksLikeCall * call ) override {
            Visitor::preVisit(call);
            call->argumentsFailedToInfer = false;
            if ( call->arguments.size() > DAS_MAX_FUNCTION_ARGUMENTS ) {
                error("too many arguments in " + call->name + ", max allowed is DAS_MAX_FUNCTION_ARGUMENTS=" DAS_STR(DAS_MAX_FUNCTION_ARGUMENTS),  "", "",
                    call->at, CompilationError::too_many_arguments);
            }
        }
        virtual ExpressionPtr visitLooksLikeCallArg ( ExprLooksLikeCall * call, Expression * arg , bool last ) override {
            if ( !arg->type ) call->argumentsFailedToInfer = true;
            if ( arg->type && arg->type->isAliasOrExpr() ) call->argumentsFailedToInfer = true;
            return Visitor::visitLooksLikeCallArg(call, arg, last);
        }
    // ExprNamedCall
        ExpressionPtr demoteCall ( ExprNamedCall * expr, const FunctionPtr & pFn ) {
            auto newCall = make_smart<ExprCall>(expr->at,pFn->name);
            size_t fnArgIndex = 0;
            for ( size_t ai=0, ais=expr->arguments.size(); ai!=ais; ++ai ) {
                auto & arg = expr->arguments[ai];
                for ( ;; ) {
                    DAS_ASSERTF ( fnArgIndex < pFn->arguments.size(), "somehow matched function which does not match. not enough args" );
                    auto & fnArg = pFn->arguments[fnArgIndex];
                    if ( fnArg->name == arg->name ) {
                        break;
                    }

                    if (fnArgIndex < expr->nonNamedArguments.size()) {
                        newCall->arguments.push_back(expr->nonNamedArguments[fnArgIndex]->clone());
                    }
                    else
                    {
                        DAS_ASSERTF(fnArg->init, "somehow matched function, which does not match. can only skip defaults");
                        newCall->arguments.push_back(fnArg->init->clone());
                    }
                    fnArgIndex ++;
                }
                newCall->arguments.push_back(arg->value->clone());
                fnArgIndex ++;
            }
            while ( fnArgIndex < pFn->arguments.size() ) {
                auto & fnArg = pFn->arguments[fnArgIndex];
                DAS_ASSERTF( fnArg->init, "somehow matched function, which does not match. tail has to be defaults");
                newCall->arguments.push_back(fnArg->init->clone());
                fnArgIndex ++;
            }
            return newCall;
        }
        virtual void preVisit ( ExprNamedCall * call ) override {
            Visitor::preVisit(call);
            call->argumentsFailedToInfer = false;
            if ( (call->arguments.size() > DAS_MAX_FUNCTION_ARGUMENTS) || ( call->nonNamedArguments.size() > DAS_MAX_FUNCTION_ARGUMENTS) ) {
                error("too many arguments in " + call->name + ", max allowed is DAS_MAX_FUNCTION_ARGUMENTS=" DAS_STR(DAS_MAX_FUNCTION_ARGUMENTS),  "", "",
                    call->at, CompilationError::too_many_arguments);
            }
        }
        virtual MakeFieldDeclPtr visitNamedCallArg ( ExprNamedCall * call, MakeFieldDecl * arg , bool last ) override {
            if (!arg->value->type) {
                call->argumentsFailedToInfer = true;
            } else if (arg->value->type && arg->value->type->isAliasOrExpr()) {
                call->argumentsFailedToInfer = true;
            }
            return Visitor::visitNamedCallArg(call, arg, last);
        }

        typedef vector<pair<Function *,int>> RankedMatchingFunctions;

        int sortCandidates ( RankedMatchingFunctions & ranked, MatchingFunctions & candidates, int nArgs ) {
            if ( candidates.size()==1 ) {
                return 0;
            }
            sort(ranked.begin(), ranked.end(), [](const pair<Function *,int> & a, const pair<Function *,int> & b) {
                return a.second < b.second;
            });
            // if there is one or more 'one-off's' - we only leave them
            int nTotal = int(ranked.size());
            int nOnes = 0;
            for ( auto & r : ranked ) {
                if ( r.second>1000 ) break;
                nOnes ++;
            }
            if ( nOnes>0 ) {
                ranked.resize(nOnes);
            } else {
                // now we remove all the ones where rank exceeds number of arguments
                int nOkay = nTotal;
                int notOkayRank = nArgs * 1000;
                while ( nOkay>0 && ranked[nOkay-1].second>=notOkayRank ) {
                    nOkay --;
                }
                ranked.resize(nOkay);
            }
            candidates.clear();
            for ( auto & r : ranked ) {
                candidates.push_back(r.first);
            }
            return nTotal - int(ranked.size());
        }

        int prepareCandidates( MatchingFunctions & candidates, const vector<TypeDeclPtr>& nonNamedArguments,  const vector<MakeFieldDeclPtr> & arguments, bool inferAuto, bool inferBlocks ) {
            RankedMatchingFunctions ranked;
            ranked.reserve(candidates.size());
            for ( auto can : candidates ) {
                int rank = getMismatchingFunctionRank(can, nonNamedArguments, arguments, inferAuto, inferBlocks);
                ranked.push_back(make_pair(can,rank));
            }
            return sortCandidates(ranked, candidates, int(arguments.size()+nonNamedArguments.size()));
        }

        void reportMissing ( ExprNamedCall * expr, const vector<TypeDeclPtr>& nonNamedArguments, const string & msg, bool reportDetails,
                                    CompilationError cerror = CompilationError::function_not_found) {
            if ( verbose ) {
                auto can1 = findCandidates(expr->name, expr->arguments);
                auto can2 = findGenericCandidates(expr->name, expr->arguments);
                can1.reserve(can1.size()+can2.size());
                can1.insert(can1.end(), can2.begin(), can2.end());
                auto nExtra = prepareCandidates(can1, nonNamedArguments, expr->arguments, true, true);
                reportFunctionNotFound(expr->name, msg + expr->name, expr->at, can1, nonNamedArguments, expr->arguments, false, true, reportDetails, cerror, nExtra);
            } else {
                error("no matching functions or generics " + expr->name, "", "", expr->at, cerror);
            }
        }
        void reportExcess ( ExprNamedCall * expr, const vector<TypeDeclPtr>& nonNamedArguments, const string & msg, MatchingFunctions can1, const MatchingFunctions & can2,
                                    CompilationError cerror = CompilationError::function_not_found) {
            if ( verbose ) {
                can1.reserve(can1.size()+can2.size());
                can1.insert(can1.end(), can2.begin(), can2.end());
                reportFunctionNotFound(expr->name, msg + expr->name, expr->at, can1, nonNamedArguments,expr->arguments, false, true, false, cerror, 0);
            } else {
                error("too many matching functions or generics " + expr->name, "", "", expr->at, cerror);
            }
        }

        int prepareCandidates ( MatchingFunctions & candidates, const vector<TypeDeclPtr>& nonNamedArguments,  bool inferAuto, bool inferBlocks ) {
            if ( candidates.size()<(size_t)program->policies.always_report_candidates_threshold ) return 0;
            RankedMatchingFunctions ranked;
            ranked.reserve(candidates.size());
            for ( auto can : candidates ) {
                int rank = getMismatchingFunctionRank(can, nonNamedArguments, inferAuto, inferBlocks);
                ranked.push_back(make_pair(can,rank));
            }
            return sortCandidates(ranked, candidates, int(nonNamedArguments.size()));
        }

        void reportMissing ( ExprLooksLikeCall * expr, const vector<TypeDeclPtr> & types,
                                    const string & msg, bool reportDetails,
                                    CompilationError cerror = CompilationError::function_not_found) {
            if ( verbose ) {
                auto can1 = findCandidates(expr->name, types);
                auto can2 = findGenericCandidates(expr->name, types);
                can1.reserve(can1.size()+can2.size());
                can1.insert(can1.end(), can2.begin(), can2.end());
                auto nExtra = prepareCandidates(can1, types, true, true);
                reportFunctionNotFound(expr->name, msg + (verbose ? expr->describe() : ""), expr->at, can1, types, true, true, reportDetails, cerror, nExtra);
            } else {
                error("no matching functions or generics " + expr->name, "", "", expr->at, cerror);
            }
        }
        void reportExcess ( ExprLooksLikeCall * expr, const vector<TypeDeclPtr> & types,
                                   const string & msg, MatchingFunctions can1, const MatchingFunctions & can2,
                                    CompilationError cerror = CompilationError::function_not_found) {
            if ( verbose ) {
                can1.reserve(can1.size()+can2.size());
                can1.insert(can1.end(), can2.begin(), can2.end());
                reportFunctionNotFound(expr->name, msg + expr->name, expr->at, can1, types, false, true, false, cerror, 0);
            } else {
                error("too many matching functions or generics " + expr->name, "", "", expr->at, cerror);
            }
        }
        virtual ExpressionPtr visit ( ExprNamedCall * expr ) override {
            if ( expr->argumentsFailedToInfer ) return Visitor::visit(expr);

            vector<TypeDeclPtr> nonNamedTypes;
            if (!inferArguments(nonNamedTypes, expr->nonNamedArguments)) {
                //TODO: report error
                return Visitor::visit(expr);
            }
            auto functions = findMatchingFunctions(expr->name, nonNamedTypes, expr->arguments, true);
            auto generics = findMatchingGenerics(expr->name, nonNamedTypes, expr->arguments);
            if ( functions.size()> 1 ) {
                vector<TypeDeclPtr> types;
                types.reserve(expr->arguments.size());
                for ( const auto & arg : expr->arguments ) {
                    types.push_back(arg->value->type);
                }
                applyLSP(types, functions);
            }
            if ( generics.size()>1 || functions.size()>1 ) {
                reportExcess(expr, nonNamedTypes, "too many matching functions or generics ", functions, generics);
            } else if ( functions.size()==0 ) {
                if ( generics.size()==1 ) {
                    reportAstChanged();
                    return demoteCall(expr,generics.back());
                } else {
                    reportMissing(expr, nonNamedTypes, "no matching functions or generics ", true);
                }
            } else {
                auto fun = functions.back();
                if ( generics.size()==1 ) {
                    auto gen = generics.back();
                    if ( fun->fromGeneric != gen ) { // TODO: getOrigin??
                        reportExcess(expr, nonNamedTypes, "too many matching functions or generics ", functions, generics);
                        return Visitor::visit(expr);
                    }
                }
                reportAstChanged();
                return demoteCall(expr,fun);
            }
            return Visitor::visit(expr);
        }
    // ExprCall
        virtual void preVisit ( ExprCall * call ) override {
            Visitor::preVisit(call);
            call->argumentsFailedToInfer = false;
            if ( call->arguments.size() > DAS_MAX_FUNCTION_ARGUMENTS ) {
                error("too many arguments in " + call->name + ", max allowed is DAS_MAX_FUNCTION_ARGUMENTS=" DAS_STR(DAS_MAX_FUNCTION_ARGUMENTS),  "", "",
                    call->at, CompilationError::too_many_arguments);
            }
        }
        virtual void preVisitCallArg ( ExprCall * call, Expression * arg, bool last ) override {
            Visitor::preVisitCallArg(call, arg, last);
            arg->isCallArgument = true;
        }
        virtual ExpressionPtr visitCallArg ( ExprCall * call, Expression * arg , bool last ) override {
            if (!arg->type) {
                call->argumentsFailedToInfer = true;
            } else if (arg->type && arg->type->isAliasOrExpr()) {
                call->argumentsFailedToInfer = true;
            }
            return Visitor::visitCallArg(call, arg, last);
        }
        string getGenericInstanceName(const Function * fn) const {
            string name;
            if ( fn->module ) {
                if ( fn->module->name=="$" ) {
                    name += "builtin";
                } else {
                    name += fn->module->name;
                }
            }
            name += "`";
            auto fCh = fn->name[0];
            if ( !(isalnum(fCh) || fCh=='_' || fCh=='`') ) {
                name += "`operator";
            }
            name += fn->name;
            string newName;
            newName.reserve(fn->name.length());
            for ( auto & ch : name ) {
                if ( isalnum(ch) || ch=='_' || ch=='`' ) {
                    newName.append(1,ch);
                } else {
                    switch ( ch ) {
                        case '=':   newName += "`eq"; break;
                        case '+':   newName += "`add"; break;
                        case '-':   newName += "`sub"; break;
                        case '*':   newName += "`mul"; break;
                        case '/':   newName += "`div"; break;
                        case '%':   newName += "`mod"; break;
                        case '<':   newName += "`less"; break;
                        case '>':   newName += "`gt"; break;
                        case '!':   newName += "`not"; break;
                        case '~':   newName += "`bnot"; break;
                        case '&':   newName += "`and"; break;
                        case '|':   newName += "`or"; break;
                        case '^':   newName += "`xor"; break;
                        case '?':   newName += "`qmark"; break;
                        case '@':   newName += "`at"; break;
                        case ':':   newName += "`col"; break;
                        case '.':   newName += "`dot"; break;
                        case '[':   newName += "`lsq"; break;
                        case ']':   newName += "`rsq"; break;
                        default:    newName += "_`_"; break;
                    }
                }

            }
            return newName;
        }
        string callCloneName ( const string & name ) {
            return "__::" + name;
        }

        // however many casts is where its at
        static int computeSubstituteDistance ( const vector<TypeDeclPtr> & arguments, const FunctionPtr & fn ) {
            int distance = 0;
            for ( size_t i=0, is=arguments.size(); i!=is; ++i ) {
                const auto & argType = arguments[i];
                const auto & funType = fn->arguments[i]->type;
                if ( !argType->isSameType ( *funType, RefMatters::no, ConstMatters::no,
                    TemporaryMatters::no, AllowSubstitute::no) ) {
                    distance ++;
                }
            }
            return distance;
        }

        // -1 - less specialized, +1 - more specialized, 0 - we don't know
        int moreSpecialized ( const TypeDeclPtr & t1, const TypeDeclPtr & t2, TypeDeclPtr & passType  ) {
        // 0. deal with options
            bool opt1 = t1->baseType==Type::option;
            bool opt2 = t2->baseType==Type::option;
            if ( opt1 || opt2 ) {
                int numS[3] = {0, 0, 0};
                if ( opt1 && opt2 ) {
                    for ( auto & arg1 : t1->argTypes ) {
                        for ( auto & arg2 : t2->argTypes ) {
                            auto res = moreSpecialized(arg1,arg2,passType);
                            numS[res+1] ++;
                        }
                    }
                } else if ( opt1 ) {
                    for ( auto & arg1 : t1->argTypes ) {
                        auto res = moreSpecialized(arg1,t2,passType);
                        numS[res+1] ++;
                    }
                } else if ( opt2 ) {
                    for ( auto & arg2 : t2->argTypes ) {
                        auto res = moreSpecialized(t1,arg2,passType);
                        numS[res+1] ++;
                    }
                } else {
                    DAS_VERIFY(0);
                }
                int total = (numS[0]?1:0) + (numS[1]?1:0) + (numS[2]?1:0);
                if ( total==1 ) {
                    if ( numS[0] ) return -1;
                    else if ( numS[1] ) return 0;
                    else return 1;
                } else {
                    return 0;
                }
            }
        // 1. non auto is more specialized than auto
            bool a1 = t1->isAutoOrAlias();
            bool a2 = t2->isAutoOrAlias();
            if ( a1!=a2 ) {             // if one is auto
                return a1 ? -1 : 1;
            }
        // 2. if both non-auto, the one without cast is more specialized
            if ( !a1 && !a2 ) {        // if both solid types
                bool s1 = passType->isSameType(*t1,RefMatters::no,ConstMatters::no,TemporaryMatters::no,AllowSubstitute::no);
                bool s2 = passType->isSameType(*t2,RefMatters::no,ConstMatters::no,TemporaryMatters::no,AllowSubstitute::no);
                if ( s1!=s2 ) {
                    return s1 ? 1 : -1;
                } else {
                    return 0;
                }
            }
    // at this point we are dealing with 2 auto types
        // 3. one with dim is more specialized, than one without
        //      if both have dim, one with actual value is more specialized, than the other one
            int d1 = t1->dim.size() ? t1->dim[0] : 0;
            int d2 = t2->dim.size() ? t2->dim[0] : 0;
            if ( d1!=d2 ) {
                if ( d1 && d2 ) {
                    return d1==-1 ? -1 : 1;
                } else {
                    return d1 ? 1 : -1;
                }
            }
        // 4. the one with base type of auto\alias is less specialized
        //      if both are auto\alias - we assume its the same level of specialization
            bool ba1 = t1->baseType==Type::autoinfer || t1->baseType==Type::alias;
            bool ba2 = t2->baseType==Type::autoinfer || t2->baseType==Type::alias;
            if ( ba1!=ba2 ) {
                return ba1 ? -1 : 1;
            } else if ( ba1 && ba2 ) {
                return 0;
            }
    // at this point base type is not auto for both, so lets compare the subtypes
        // if either does not match the base type, we arrive here through wrong option
        if ( t1->baseType!=passType->baseType || t2->baseType!=passType->baseType) {
            return 0;
        }
        //    DAS_ASSERT(t1->baseType==passType->baseType && "how did it match otherwise?");
        //    DAS_ASSERT(t2->baseType==passType->baseType && "how did it match otherwise?");


        // if its an array or a pointer, we compare specialization of subtype
            if ( t1->baseType==Type::tPointer || t1->baseType==Type::tArray || t1->baseType==Type::tIterator ) {
                return moreSpecialized(t1->firstType, t2->firstType, passType->firstType);
        // if its a table, we compare both subtypes
            } else if ( t1->baseType==Type::tTable ) {
                int i1 = moreSpecialized(t1->firstType, t2->firstType, passType->firstType);
                int i2 = moreSpecialized(t1->secondType, t2->secondType, passType->secondType);
                bool less = (i1<0) || (i2<0);
                bool more = (i1>0) || (i2>0);
                if ( less && more ) return 0;
                else if ( less ) return -1;
                else if ( more ) return 1;
                else return 0;
        // if its a tuple or a variant, we compare all subtypes
            } else if ( t1->baseType==Type::tVariant || t1->baseType==Type::tTuple ) {
                bool less = false;
                bool more = false;
                DAS_ASSERT(t1->argTypes.size() && passType->argTypes.size() && "how did it match otherwise?");
                DAS_ASSERT(t2->argTypes.size() && passType->argTypes.size() && "how did it match otherwise?");
                for ( size_t aI=0, aIs=t1->argTypes.size(); aI!=aIs; ++aI ) {
                    int cmpr = moreSpecialized(
                        t1->argTypes[aI],
                        t2->argTypes[aI],
                        passType->argTypes[aI]);
                    if ( cmpr<0 ) less = true;
                    else if ( cmpr>0 ) more = true;
                }
                if ( less && more ) return 0;
                else if ( less ) return -1;
                else if ( more ) return 1;
                else return 0;
        // if its a block, a function, or a lambda, we compare all subtypes and firstType (result)
            } else if ( t1->baseType==Type::tBlock || t1->baseType==Type::tLambda || t1->baseType==Type::tFunction ) {
                bool less = false;
                bool more = false;
                int cmpr = moreSpecialized(t1->firstType, t2->firstType, passType->firstType);
                if ( cmpr<0 ) less = true;
                else if ( cmpr>0 ) more = true;
                DAS_ASSERT(t1->argTypes.size() && passType->argTypes.size() && "how did it match otherwise?");
                DAS_ASSERT(t2->argTypes.size() && passType->argTypes.size() && "how did it match otherwise?");
                for ( size_t aI=0, aIs=t1->argTypes.size(); aI!=aIs; ++aI ) {
                    cmpr = moreSpecialized(
                        t1->argTypes[aI],
                        t2->argTypes[aI],
                        passType->argTypes[aI]);
                    if ( cmpr<0 ) less = true;
                    else if ( cmpr>0 ) more = true;
                }
                if ( less && more ) return 0;
                else if ( less ) return -1;
                else if ( more ) return 1;
                else return 0;
            }
            return 0;
        }

        // compares two generics
        //  one generic is more specialized than the other, if ALL arguments are more specialized or the
        bool copmareFunctionSpecialization ( const FunctionPtr & f1, const FunctionPtr & f2, ExprLooksLikeCall * expr ) {
            size_t nArgs = expr->arguments.size();
            bool less = false;
            bool more = false;
            for ( size_t aI = 0; aI!=nArgs; ++aI ) {
                const auto & f1A = f1->arguments[aI]->type;
                const auto & f2A = f2->arguments[aI]->type;
                int cmpr = moreSpecialized(f1A,f2A,expr->arguments[aI]->type);
                if ( cmpr<0 ) less = true;
                else if ( cmpr>0 ) more = true;
            }
            if ( !more && !less ) {
                // if functions are identical, the one with more specialization annotations win
                int spF1 = 0;
                for ( auto & ann : f1->annotations ) {
                    auto fnA = static_pointer_cast<FunctionAnnotation>(ann->annotation);
                    if ( fnA->isSpecialized() ) spF1 ++;
                }
                int spF2 = 0;
                for ( auto & ann : f2->annotations ) {
                    auto fnA = static_pointer_cast<FunctionAnnotation>(ann->annotation);
                    if ( fnA->isSpecialized() ) spF2 ++;
                }
                if ( spF1 > spF2 ) more = true;
            }
            if ( more && !less ) {
                return true;
            } else {
                return false;
            }
        }

        static void applyLSP ( const vector<TypeDeclPtr> & arguments, MatchingFunctions & functions ) {
            if ( functions.size()<=1 ) return;
            vector<pair<int,Function *>> fnm;
            for ( auto & fn : functions ) {
                auto dist = computeSubstituteDistance(arguments, fn);
                fnm.push_back(make_pair(dist,fn));
            }
            sort ( fnm.begin(), fnm.end(), [&] ( auto a, auto b ) {
                return a.first < b.first;
            });
            int count = 1;
            int depth = fnm[0].first;
            while ( count < int(fnm.size()) ) {
                if ( fnm[count].first != depth ) break;
                count ++;
            }
            if ( count == 1 ) {
                functions.resize(1);
                functions.front() = fnm[0].second;
            }
        }

        bool reportOp2Errors ( ExprLooksLikeCall * expr ) {
            auto expr_left = expr->arguments[0].get();
            auto expr_right = expr->arguments[1].get();
            auto expr_op = expr->name.substr(3);
            if (expr_left->type->isNumeric() && expr_right->type->isNumeric()) {
                if ( isAssignmentOperator(expr_op) ) {
                    if ( !expr_left->type->ref ) {
                        error("numeric operator " + expr_op + " left side must be reference.", "", "",
                            expr->at, CompilationError::operator_not_found);
                        return true;
                    } else if ( expr_left->type->isConst() ) {
                        error("numeric operator " + expr_op + " left side can't be constant.", "", "",
                            expr->at, CompilationError::operator_not_found);
                        return true;
                    } else  {
                        if ( verbose ) {
                            TextWriter tw;
                            tw << "\t" << *expr_left << " " << expr_op << " " << das_to_string(expr_left->type->baseType) << " (" << *expr_right << ")\n";
                            error("numeric operator " + expr_op + " type mismatch. both sides have to be of the same type. " +
                                das_to_string(expr_left->type->baseType) + " " + expr_op + " " + das_to_string(expr_right->type->baseType)
                                + " is not defined", "", "try the following\n" + tw.str(),
                                expr->at, CompilationError::operator_not_found);
                            return true;
                        } else {
                            error("numeric operator " + expr_op + " type mismatch. both sides have to be of the same type. ",  "", "",
                                expr->at, CompilationError::operator_not_found);
                            return true;
                        }
                    }
                } else {
                    if ( verbose ) {
                        if ( expr_left->type->baseType != expr_right->type->baseType ) {
                            TextWriter tw;
                            tw << "\t" << *expr_left << " " << expr_op << " " << das_to_string(expr_left->type->baseType) << " (" << *expr_right << ")\n";
                            tw << "\t" << das_to_string(expr_right->type->baseType) << "(" << *expr_left << ") " << expr_op << " " << *expr_right << "\n";
                            error("numeric operator " + expr_op + " type mismatch. both sides have to be of the same type. " +
                                das_to_string(expr_left->type->baseType) + " " + expr_op + " " + das_to_string(expr_right->type->baseType)
                                    + " is not defined", "", "try one of the following\n" + tw.str(),
                                    expr->at, CompilationError::operator_not_found);
                            return true;
                        } else if ( expr_left->type->isNumericStorage()  ) {
                            error("numeric operator " + expr_op + " is not defined for storage types (int8,uint8,int16,uint16).",
                                "\t" + das_to_string(expr_left->type->baseType) + " " + expr_op + " " + das_to_string(expr_right->type->baseType),
                                    "", expr->at, CompilationError::operator_not_found);
                            return true;
                        } else {
                            error("numeric operator " + expr_op + " type mismatch.",
                                "\t" + das_to_string(expr_left->type->baseType) + " " + expr_op + " " + das_to_string(expr_right->type->baseType),
                                    "", expr->at, CompilationError::operator_not_found);
                            return true;
                        }
                    } else {
                        error("numeric operator " + expr_op + " type mismatch.", "" , "",
                            expr->at, CompilationError::operator_not_found);
                        return true;
                    }
                }
            }
            return false;
        }
        enum class InferCallError {
            functionOrGeneric,
            operatorOp2,
            tryOperator
        };

        bool inferArguments(vector<TypeDeclPtr>& types, const vector<ExpressionPtr>& arguments) {
            types.reserve(arguments.size());
            for (auto& ar : arguments) {
                if (!ar->type) {
                    return false;
                }
                DAS_ASSERT(!ar->type->isExprType()
                    && "if this happens, we are calling infer function call without checking for '[expr]'. do that from where we call up the stack.");
                // if its an auto or an alias
                // we only allow it, if its a block or lambda
                if ( ar->type->baseType!=Type::tBlock && ar->type->baseType!=Type::tLambda && ar->type->baseType!=Type::tFunction ) {
                    if ( ar->type->isAutoOrAlias() ) {
                        return false;
                    }
                }
                types.push_back(ar->type);
            }
            return true;
        }

        FunctionPtr inferFunctionCall ( ExprLooksLikeCall * expr, InferCallError cerr=InferCallError::functionOrGeneric ) {
            vector<TypeDeclPtr> types;
            if (!inferArguments(types, expr->arguments)) {
                return nullptr;
            }
            auto functions = findMatchingFunctions(expr->name, types, true);
            auto generics = findMatchingGenerics(expr->name, types);
            applyLSP(types,functions);
            if ( functions.size()==1 ) {
                auto funcC = functions.back();
                if ( funcC->firstArgReturnType ) {
                    expr->type = make_smart<TypeDecl>(*expr->arguments[0]->type);
                    expr->type->ref = false;
                } else {
                    expr->type = make_smart<TypeDecl>(*funcC->result);
                }
                // infer FORWARD types
                for ( size_t iF=0, iFs=expr->arguments.size(); iF!=iFs; ++iF ) {
                    auto & arg = expr->arguments[iF];
                    if ( arg->type->isAuto() && (arg->type->isGoodBlockType() || arg->type->isGoodLambdaType() || arg->type->isGoodFunctionType()) ) {
                        if ( arg->rtti_isMakeBlock() ) { // "it's always MakeBlock. unless its function and @@funcName
                            auto mkBlock = static_pointer_cast<ExprMakeBlock>(arg);
                            auto block = static_pointer_cast<ExprBlock>(mkBlock->block);
                            auto retT = TypeDecl::inferGenericType(mkBlock->type, funcC->arguments[iF]->type, true, true, nullptr);
                            DAS_ASSERTF ( retT, "how? it matched during findMatchingFunctions the same way");
                            TypeDecl::applyAutoContracts(mkBlock->type, funcC->arguments[iF]->type);
                            block->returnType = make_smart<TypeDecl>(*retT->firstType);
                            for ( size_t ba=0, bas=retT->argTypes.size(); ba!=bas; ++ba ) {
                                block->arguments[ba]->type = make_smart<TypeDecl>(*retT->argTypes[ba]);
                            }
                            setBlockCopyMoveFlags(block.get());
                            reportAstChanged();
                        }
                    }
                }
                // append default arguments
                for ( size_t iT=expr->arguments.size(), iTs=funcC->arguments.size(); iT!=iTs; ++iT ) {
                    auto newArg = funcC->arguments[iT]->init->clone();
                    if ( !newArg->type ) {
                        // recursive resolve???
                        newArg = newArg->visit(*this);
                    }
                    if ( newArg->type && newArg->type->baseType==Type::fakeLineInfo ) {
                        newArg->at = expr->at;
                    }
                    expr->arguments.push_back(newArg);
                }
                // dereference what needs dereferences
                for ( size_t iA=0, iAs=expr->arguments.size(); iA!=iAs; ++iA )
                    if ( !funcC->arguments[iA]->type->isRef() )
                        expr->arguments[iA] = Expression::autoDereference(expr->arguments[iA]);
                // and all good
                return funcC;
            } else if ( functions.size()>1 ) {
                reportExcess(expr, types, "too many matching functions or generics ", functions, generics);
            } else if ( functions.size()==0 ) {
                // if there is more than one, we pick more specialized
                if ( generics.size()>1 ) {
                    stable_sort(generics.begin(), generics.end(), [&](const FunctionPtr & f1, const FunctionPtr & f2 ){
                        return copmareFunctionSpecialization(f1,f2,expr);
                    });
                    // if one is most specialized, we pick it, otherwise we report all of them
                    if ( copmareFunctionSpecialization(generics.front(),generics[1],expr) ) {
                        generics.resize(1);
                    }
                }
                if ( generics.size()==1 ) {
                    auto oneGeneric = generics.back();
                    auto genName = getGenericInstanceName(oneGeneric);
                    auto instancedFunctions = findMatchingFunctions("__::" + genName, types, true);
                    if ( instancedFunctions.size() > 1 ) {
                        TextWriter ss;
                        for ( auto & instFn : instancedFunctions ) {
                            ss  << "\t" << describeFunction(instFn) << " in "
                                << (instFn->module->name.empty() ? "this module" : instFn->module->name)
                                << "\n";
                        }
                        error("internal compiler error. multiple instances of " + genName, ss.str(), "", expr->at);
                    } else if (instancedFunctions.size() == 1) {
                        expr->name = callCloneName(genName);
                        reportAstChanged();
                    } else if (instancedFunctions.size() == 0) {
                        auto clone = oneGeneric->clone();
                        clone->name = genName;
                        clone->fromGeneric = oneGeneric;
                        clone->privateFunction = true;
                        if (func) {
                            clone->inferStack.emplace_back(expr->at, func);
                            clone->inferStack.insert(clone->inferStack.end(), func->inferStack.begin(), func->inferStack.end());
                        }
                        // we build alias map for the generic
                        AliasMap aliases;
                        vector<bool> defaultRef(types.size());
                        for (;; ) {
                            bool anyFailed = false;
                            auto totalAliases = aliases.size();
                            for (size_t ai=0, ais=types.size(); ai!=ais; ++ai) {
                                auto argType = clone->arguments[ai]->type;
                                if (argType->isAlias()) {
                                    argType = inferPartialAliases(argType, clone, &aliases);
                                }
                                auto passType = types[ai];
                                OptionsMap options;
                                if (!isMatchingArgument(clone, argType, passType, true, true, &aliases, &options)) {
                                    anyFailed = true;
                                    continue;
                                }
                                TypeDecl::updateAliasMap(argType, passType, aliases, options);
                                if ( argType->baseType==Type::option ) {
                                    auto it = options.find(argType.get());
                                    if ( it!=options.end() ) {
                                        auto optionType = argType->argTypes[it->second].get();
                                        defaultRef[ai] = optionType->ref;
                                    }
                                } else {
                                    defaultRef[ai] = argType->ref;
                                }
                            }
                            if (!anyFailed) break;
                            if (totalAliases == aliases.size()) {
                                DAS_ASSERTF(0, "we should not be here. function matched arguments!");
                                break;
                            }
                        }
                        // now, we resolve types given inferred aliases
                        for (size_t sz=0, szs=types.size(); sz!=szs; ++sz) {
                            auto & argT = clone->arguments[sz]->type;
                            if (argT->isAlias()) {
                                argT = inferPartialAliases(argT, clone, &aliases);
                            }
                            bool appendHasOptions = false;
                            bool isAutoWto = argT->isAutoWithoutOptions(appendHasOptions);
                            if ( isAutoWto || appendHasOptions) {
                                auto & passT = types[sz];
                                auto resT = TypeDecl::inferGenericType(argT, passT, true, true, nullptr);
                                DAS_ASSERTF(resT, "how? we had this working at findMatchingGenerics");
                                resT->ref = defaultRef[sz];
                                TypeDecl::applyAutoContracts(resT, argT);
                                TypeDecl::applyRefToRef(resT, true);
                                resT->isExplicit = isAutoWto; // this is generic for this type, and this type only
                                argT = resT;
                            } else {
                                TypeDecl::applyRefToRef(argT, true);
                            }
                        }
                        // resolve tail-end types
                        for ( size_t ai=types.size(), ais=clone->arguments.size(); ai!=ais; ++ai ) {
                            auto & arg = clone->arguments[ai];
                            if ( arg->type->isAuto() ) {
                                if ( arg->init ) {
                                    arg->init = arg->init->visit(*this);
                                    if (arg->init->type && !arg->init->type->isAutoOrAlias()) {
                                        arg->type = make_smart<TypeDecl>(*arg->init->type);
                                        continue;
                                    }
                                }
                                error("unknown type of argument " + clone->arguments[ai]->name
                                    + "; can't instance " + describeFunction(oneGeneric), "",
                                    "provide argument type explicitly",
                                    expr->at, CompilationError::invalid_type);
                                return nullptr;
                            }
                        }
                        // now we verify if tail end can indeed be fully inferred
                        if (!program->addFunction(clone)) {
                            clone->module = program->thisModule.get();
                            auto exf = program->thisModule->functions.find(clone->getMangledName());
                            clone->module = nullptr;
                            DAS_ASSERTF(exf, "if we can't add, this means there is function with exactly this mangled name");
                            if (exf->fromGeneric != clone->fromGeneric) { // TODO: getOrigin??
                                error("can't instance generic " + describeFunction(clone),
                                    + "\ttrying to instance from module " + clone->fromGeneric->module->name + "\n"
                                    + "\texisting instance from module " + exf->fromGeneric->module->name, "",
                                    expr->at, CompilationError::function_already_declared);
                                return nullptr;
                            }
                        } else {
                            // perform generic_apply
                            for ( auto & pA : clone->annotations ) {
                                if ( pA->annotation->rtti_isFunctionAnnotation() ) {
                                    auto ann = static_pointer_cast<FunctionAnnotation>(pA->annotation);
                                    string err;
                                    if ( !ann->generic_apply(clone, *(program->thisModuleGroup), pA->arguments, err) ) {
                                        error("Macro [" +pA->annotation->name + "] failed to generic_apply to a function " + clone->name + "\n",
                                            err, "", clone->at, CompilationError::invalid_annotation);
                                        return nullptr;
                                    }
                                }
                            }
                        }
                        expr->name = callCloneName(clone->name);
                        reportAstChanged();
                    }
                } else if ( generics.size()>1 ) {
                    copmareFunctionSpecialization(generics.front(),generics[1],expr);
                    reportExcess(expr, types, "too many matching functions or generics ", functions, generics);
                } else {
                    if ( auto aliasT = findAlias(expr->name) ) {
                        if ( aliasT->isCtorType() ) {
                            expr->name = das_to_string(aliasT->baseType);
                            if ( aliasT->baseType==Type::tBitfield ) {
                                expr->aliasSubstitution = aliasT;
                            }
                            reportAstChanged();
                        } else if ( aliasT->isStructure() ) {
                            if ( expr->arguments.size()==0 ) {
                                expr->name = aliasT->structType->name;
                                tryMakeStructureCtor (aliasT->structType);
                                reportAstChanged();
                            } else {
                                error("can only generate default structure constructor without arguments",
                                    "", "", expr->at, CompilationError::invalid_argument_count);
                            }
                        } else {
                            if ( cerr==InferCallError::operatorOp2 ) {
                                if ( !reportOp2Errors(expr) ) {
                                    reportMissing(expr, types, "no matching functions or generics ", true);
                                }
                            } else if ( cerr!=InferCallError::tryOperator ) {
                                reportMissing(expr, types, "no matching functions or generics ", true);
                            }
                        }
                    } else {
                        if ( cerr==InferCallError::operatorOp2 ) {
                            if ( !reportOp2Errors(expr) ) {
                                reportMissing(expr, types, "no matching functions or generics ", true);
                            }
                        } else if ( cerr!=InferCallError::tryOperator ) {
                            reportMissing(expr, types, "no matching functions or generics ", true);
                        }
                    }
                }
            }
            return nullptr;
        }
        ExpressionPtr inferGenericOperator ( const string & opN, const LineInfo & expr_at, const ExpressionPtr & arg0, const ExpressionPtr & arg1, InferCallError err = InferCallError::tryOperator ) {
            auto opName = "_::" + opN;
            auto tempCall = make_smart<ExprLooksLikeCall>(expr_at,opName);
            tempCall->arguments.push_back(arg0);
            if ( arg1 ) tempCall->arguments.push_back(arg1);
            auto ffunc = inferFunctionCall(tempCall.get(),err).get();
            if ( opName != tempCall->name ) {   // this happens when the operator gets instanced
                reportAstChanged();
                auto opCall = make_smart<ExprCall>(expr_at, tempCall->name);
                opCall->arguments = das::move(tempCall->arguments);
                return opCall;
            } else if ( ffunc ) { // function found
                reportAstChanged();
                auto opCall = make_smart<ExprCall>(expr_at, opN);
                opCall->arguments = das::move(tempCall->arguments);
                return opCall;
            } else {
                return nullptr;
            }
        }
        ExpressionPtr inferGenericOperatorWithName ( const string & opN, const LineInfo & expr_at, const ExpressionPtr & arg0, const string & arg1, InferCallError err = InferCallError::tryOperator ) {
            auto conststring = make_smart<TypeDecl>(Type::tString);
            conststring->constant = true;
            auto fieldName = make_smart<ExprConstString>(expr_at,arg1);
            fieldName->type = conststring;
            return inferGenericOperator(opN, expr_at, arg0, fieldName, err);
        }
        virtual ExpressionPtr visit ( ExprCall * expr ) override {
            if (expr->argumentsFailedToInfer) return Visitor::visit(expr);
            expr->func = inferFunctionCall(expr).get();
            if ( expr->aliasSubstitution  ) {
                if ( expr->arguments.size()!=1 ) {
                    error("casting to bitfield requires one argument", "", "",
                        expr->at, CompilationError::invalid_cast);
                    return Visitor::visit(expr);
                }
                auto ecast = make_smart<ExprCast>(expr->at, expr->arguments[0]->clone(), expr->aliasSubstitution );
                ecast->reinterpret = true;
                ecast->alwaysSafe = true;
                expr->aliasSubstitution.reset();
                return ecast;
            }
            if ( func && !expr->func && func->isClassMethod && func->arguments.size()>=1 ) {
                auto bt = func->arguments[0]->type;
                if ( bt && bt->isClass() ) {
                    if ( expr->name.find("::") == string::npos ) {  // we only promote to self->call() if its not blah::call, _::call, or __::call
                        auto memFn = bt->structType->findField(expr->name);
                        if ( memFn && memFn->type && memFn->type->isFunction() ) {
                            auto self = new ExprVar(expr->at, "self");
                            auto pInvoke = makeInvokeMethod(expr->at, self, expr->name);
                            for ( auto & arg : expr->arguments ) {
                                pInvoke->arguments.push_back(arg->clone());
                            }
                            pInvoke->alwaysSafe = expr->alwaysSafe;
                            reportAstChanged();
                            return pInvoke;
                        }
                    }
                }
            }
            /*
            // NOTE: this should not be necessary, since infer function call suppose to report every time
            if ( !expr->func ) {
                error("unknwon function " + expr->name, "", "",
                    expr->at, CompilationError::function_not_found);
            }
            */
            if ( expr->func && expr->func->unsafeOperation && !safeExpression(expr) ) {
                error("unsafe call " + expr->name + " requires unsafe", "", "",
                    expr->at, CompilationError::unsafe);
            } else if ( expr->func && expr->func->unsafeOutsideOfFor && !(expr->isForLoopSource || safeExpression(expr)) ) {
                error(expr->name + " is unsafe, when not source of the for loop. requires unsafe", "", "",
                    expr->at, CompilationError::unsafe);
            } else if (enableInferTimeFolding && expr->func && isConstExprFunc(expr->func)) {
                vector<ExpressionPtr> cargs; cargs.reserve(expr->arguments.size());
                bool failed = false;
                for (auto & arg : expr->arguments) {
                    if ( auto carg = getConstExpr(arg.get()) ) {
                        cargs.push_back(carg);
                    } else if ( arg->type->baseType==Type::fakeContext || arg->type->baseType==Type::fakeLineInfo ) {
                        cargs.push_back(cloneWithType(arg));
                    } else {
                        failed = true;
                        break;
                    }
                }
                if ( !failed ) {
                    swap(cargs, expr->arguments);
                    return evalAndFold(expr);
                }
            }
            if ( expr->func ) {
                for ( const auto & ann : expr->func->annotations ) {
                    if ( ann->annotation->rtti_isFunctionAnnotation() ) {
                        auto fnAnn = static_pointer_cast<FunctionAnnotation>(ann->annotation);
                        string err;
                        auto fexpr = fnAnn->transformCall(expr, err);
                        if ( !err.empty() ) {
                            error("call annotated by " + fnAnn->name + " failed to transform, " + err, "", "",
                                           expr->at, CompilationError::annotation_failed);
                        } else if ( fexpr ) {
                            reportAstChanged();
                            return fexpr;
                        }
                    }
                }
            }
            return Visitor::visit(expr);
        }
    // StringBuilder
        virtual ExpressionPtr visitStringBuilderElement ( ExprStringBuilder *, Expression * expr, bool ) override {
            auto res = Expression::autoDereference(expr);
            if ( expr->constexpression ) {
                return evalAndFoldString(res.get());
            } else {
                return res;
            }
        }
        virtual ExpressionPtr visit ( ExprStringBuilder * expr ) override {
            expr->type = make_smart<TypeDecl>(Type::tString);
            return evalAndFoldStringBuilder(expr);
        }
    // make variant
        virtual void preVisit ( ExprMakeVariant * expr ) override {
            Visitor::preVisit(expr);
            if ( expr->makeType && expr->makeType->isExprType() ) {
                return;
            }
            verifyType(expr->makeType);
            if ( expr->makeType->baseType != Type::tVariant ) {
                error("[[variant" + describeType(expr->makeType) + "]] with non-variant type","", "",
                    expr->at, CompilationError::invalid_type);
            }
            if ( expr->makeType->dim.size()>1 ) {
                error("[[" + describeType(expr->makeType) + "]] variant can only initialize single dimension arrays", "", "",
                    expr->at, CompilationError::invalid_type);
            } else if ( expr->makeType->dim.size()==1 && expr->makeType->dim[0]!=int32_t(expr->variants.size()) ) {
                error("[[" + describeType(expr->makeType) + "]] variant dimension mismatch, provided " +
                    to_string(expr->variants.size()) + " elements, expecting " + to_string(expr->makeType->dim[0]), "", "",
                    expr->at, CompilationError::invalid_type);
            } else if ( expr->makeType->ref ) {
                error("[[" + describeType(expr->makeType) + "]] variant can't be reference", "", "",
                    expr->at, CompilationError::invalid_type);
            }
        }
        virtual MakeFieldDeclPtr visitMakeVariantField ( ExprMakeVariant * expr, int index, MakeFieldDecl * decl, bool last ) override {
            if ( !decl->value->type ) {
                return Visitor::visitMakeVariantField(expr,index,decl,last);
            }
            auto fieldVariant = expr->makeType->findArgumentIndex(decl->name);
            if (fieldVariant != -1) {
                auto fieldType = expr->makeType->argTypes[fieldVariant];
                if ( !canCopyOrMoveType(fieldType,decl->value->type,TemporaryMatters::yes,decl->value.get()) ) {
                    error("can't initialize field " + decl->name + "; expecting "
                        + describeType(fieldType) + ", passing " + describeType(decl->value->type),"", "",
                        decl->value->at, CompilationError::invalid_type);
                } else if ( decl->value->type->isTemp(true,false) ) {
                    error("can't initialize variant field " + decl->name + " with temporary value", "", "",
                        decl->value->at, CompilationError::cant_pass_temporary);
                }
                if (!fieldType->canCopy() && !decl->moveSemantics) {
                    error("field " + decl->name + " can't be copied; " + describeType(fieldType),"","use <- instead",
                        decl->at, CompilationError::invalid_type);
                } else if (decl->moveSemantics && decl->value->type->isConst()) {
                    error("can't move from a constant value " + describeType(decl->value->type), "", "",
                        decl->value->at, CompilationError::cant_move);
                }
            } else {
                error("field not found, " + decl->name, "", "",
                    decl->at, CompilationError::cant_get_field);
            }
            return Visitor::visitMakeVariantField(expr,index,decl,last);
        }
        virtual ExpressionPtr visit ( ExprMakeVariant * expr ) override {
            if ( expr->makeType && expr->makeType->isExprType() ) {
                return Visitor::visit(expr);
            }
            // result type
            auto resT = make_smart<TypeDecl>(*expr->makeType);
            uint32_t resDim = uint32_t(expr->variants.size());
            if ( resDim==0 ) {
                resT->dim.clear();
            } else if ( resDim!=1 ) {
                resT->dim.resize(1);
                resT->dim[0] = resDim;
            } else {
                resT->dim.clear();
            }
            expr->type = resT;
            verifyType(expr->type);
            return Visitor::visit(expr);
        }
    // make structure
        virtual void preVisit ( ExprMakeStruct * expr ) override {
            Visitor::preVisit(expr);
            if ( expr->makeType && expr->makeType->isExprType() ) {
                return;
            }
            verifyType(expr->makeType);
            if ( expr->makeType->baseType!=Type::tStructure && expr->makeType->baseType!=Type::tHandle ) {
                if ( expr->structs.size() ) {
                    error("[[" + describeType(expr->makeType) + "]] with non-structure type", "", "",
                          expr->at, CompilationError::invalid_type);
                }
            }
            if ( expr->makeType->dim.size()>1 ) {
                error("[[" + describeType(expr->makeType) + "]] struct can only initialize single dimension arrays", "", "",
                    expr->at, CompilationError::invalid_type);
            } else if ( expr->makeType->dim.size()==1 && expr->makeType->dim[0]!=int32_t(expr->structs.size()) ) {
                error("[[" + describeType(expr->makeType) + "]] struct dimension mismatch, provided " +
                      to_string(expr->structs.size()) + " elements, expecting " + to_string(expr->makeType->dim[0]), "", "",
                    expr->at, CompilationError::invalid_type);
            } else if ( expr->makeType->ref ) {
                error("[[" + describeType(expr->makeType) + "]] struct can't be reference", "", "",
                    expr->at, CompilationError::invalid_type);
            } else if ( !expr->makeType->isLocal() && !expr->isNewHandle ) {
                error("[[" + describeType(expr->makeType) + "]] struct can`t be allocated locally (on the stack), since Handled type isLocal() returned false. Allocate it on the heap (new [[...]]) or modify your C++ bindings.", "", "",
                    expr->at, CompilationError::invalid_type);
            } else if ( expr->makeType->baseType==Type::tHandle && expr->isNewHandle && !expr->useInitializer ) {
                error("new [[" + describeType(expr->makeType) + "]] struct requires initializer syntax", "",
                        "use new [[" + describeType(expr->makeType) + "()]] instead",
                    expr->at, CompilationError::invalid_type);
            }
        }
        virtual MakeFieldDeclPtr visitMakeStructureField ( ExprMakeStruct * expr, int index, MakeFieldDecl * decl, bool last ) override {
            if ( !decl->value->type ) {
                return Visitor::visitMakeStructureField(expr,index,decl,last);
            }
            if ( decl->cloneSemantics ) {
                if ( !expr->block ) expr->block = makeStructWhereBlock(expr);
                DAS_ASSERT(expr->block->rtti_isMakeBlock());
                auto mkb = static_pointer_cast<ExprMakeBlock>(expr->block);
                DAS_ASSERT(mkb->block->rtti_isBlock());
                auto blk = static_pointer_cast<ExprBlock>(mkb->block);
                auto cle = convertToCloneExpr(expr,index,decl);
                blk->list.insert(blk->list.begin(), cle); // TODO: fix order. we are making them backwards now
                reportAstChanged();
                return nullptr;
            }
            if ( expr->makeType->baseType == Type::tStructure ) {
                if ( auto field = expr->makeType->structType->findField(decl->name) ) {
                    if ( !canCopyOrMoveType(field->type,decl->value->type,TemporaryMatters::yes,decl->value.get()) ) {
                        error("can't initialize field " + decl->name + "; expecting "
                              + describeType(field->type) + ", passing " + describeType(decl->value->type), "", "",
                                decl->value->at, CompilationError::invalid_type );
                    } else if ( decl->value->type->isTemp(true,false) ) {
                        if ( expr->makeType->structType->isLambda ) {
                            error("can't capture temporary lambda variable " + decl->name, "", "",
                                decl->value->at, CompilationError::cant_pass_temporary);
                        } else {
                            error("can't initialize structure field " + decl->name + " with temporary value", "", "",
                                decl->value->at, CompilationError::cant_pass_temporary);
                        }
                    }
                    if( !field->type->canCopy() && !decl->moveSemantics ) {
                        error("field " + decl->name + " can't be copied; " + describeType(field->type),"","use <- instead",
                              decl->at, CompilationError::invalid_type );
                    } else if (decl->moveSemantics && decl->value->type->isConst()) {
                        error("can't move from a constant value " + describeType(decl->value->type), "", "",
                            decl->value->at, CompilationError::cant_move);
                    }
                    if ( !decl->moveSemantics && !field->type->ref ) {
                        decl->value = Expression::autoDereference(decl->value);
                    }
                } else {
                    error("field not found, " + decl->name, "", "",
                        decl->at, CompilationError::cant_get_field);
                }
            } else if ( expr->makeType->baseType == Type::tHandle ) {
                if ( auto fldt = expr->makeType->annotation->makeFieldType(decl->name, false) ) {
                    if ( !fldt->isRef() ) {
                        error("field is a property, not a value; " + decl->name, "", "",
                            decl->at, CompilationError::cant_get_field);
                    }
                    if ( !canCopyOrMoveType(fldt,decl->value->type,TemporaryMatters::no,decl->value.get()) ) {
                        error("can't initialize field " + decl->name + "; expecting "
                              + describeType(fldt)+", passing " + describeType(decl->value->type), "", "",
                                decl->value->at, CompilationError::invalid_type );
                    }
                    if( !fldt->canCopy() && !decl->moveSemantics ) {
                        error("field " + decl->name + " can't be copied; " + describeType(fldt),"","use <- instead",
                              decl->at, CompilationError::invalid_type );
                    } else if (decl->moveSemantics && decl->value->type->isConst()) {
                        error("can't move from a constant value " + describeType(decl->value->type), "", "",
                            decl->value->at, CompilationError::cant_move);
                    }
                    if ( !decl->moveSemantics && !fldt->ref ) {
                        decl->value = Expression::autoDereference(decl->value);
                    }
                } else {
                    error("annotation field not found, " + decl->name, "", "",
                        decl->at, CompilationError::cant_get_field);
                }
            }
            return Visitor::visitMakeStructureField(expr,index,decl,last);
        }
        virtual ExpressionPtr visit ( ExprMakeStruct * expr ) override {
            if ( expr->makeType && expr->makeType->isExprType() ) {
                return Visitor::visit(expr);
            }
            if ( expr->makeType && expr->makeType->isAlias() ) {
                if ( auto aT = inferAlias(expr->makeType) ) {
                    expr->makeType = aT;
                    reportAstChanged();
                } else {
                    error("undefined [[ ]] expression type " + describeType(expr->makeType),
                        reportInferAliasErrors(expr->makeType), "", expr->makeType->at, CompilationError::type_not_found );
                }
            }
            if ( expr->block ) {
                DAS_ASSERT(expr->block->rtti_isMakeBlock());
                auto mkb = static_pointer_cast<ExprMakeBlock>(expr->block);
                DAS_ASSERT(mkb->block->rtti_isBlock());
                auto blk = static_pointer_cast<ExprBlock>(mkb->block);
                if ( blk->arguments.size()!=1 ) {
                    error("where closure should only have one argument",  "", "",
                        expr->block->at, CompilationError::invalid_block );
                } else {
                    auto arg = blk->arguments[0];
                    if ( arg->type ) {
                        int32_t rec = 0;
                        if ( expr->structs.size()>1 ) rec = int32_t(expr->structs.size());
                        auto passT = make_smart<TypeDecl>(*expr->makeType);
                        passT->dim.clear();
                        if ( rec ) passT->dim.push_back(rec);
                        if ( arg->type->isAuto() ) {
                            auto nargT = TypeDecl::inferGenericType(passT, arg->type, false, false, nullptr);
                            if ( nargT ) {
                                TypeDecl::applyAutoContracts(nargT, arg->type);
                                arg->type = nargT;
                            } else {
                                error("can't infer where closure block argument",  "", "",
                                    arg->at, CompilationError::invalid_block );
                            }
                        }
                        if ( !arg->type->isSameType(*passT,RefMatters::no,ConstMatters::no,TemporaryMatters::no) ) {
                            error("where closure block argument type mismatch, " +
                                describeType(arg->type) + " vs " + describeType(expr->makeType),  "", "",
                                    arg->at, CompilationError::invalid_block );
                        } else if ( arg->type->constant ) {
                            error("where closure block argument can't be constant, " +
                                describeType(arg->type) + " vs " + describeType(expr->makeType),  "", "",
                                    arg->at, CompilationError::invalid_block );
                        }
                    }
                }
            }
            // promote to make variant
            if ( expr->makeType->baseType == Type::tVariant ) {
                if ( expr->block ) {
                    error("[[variant]] can't have where closure",  "", "",
                        expr->block->at, CompilationError::invalid_block );
                    return Visitor::visit(expr);
                }
                auto mkv = make_smart<ExprMakeVariant>(expr->at);
                mkv->makeType = make_smart<TypeDecl>(*expr->makeType);
                auto allGood = true;
                for (auto & st : expr->structs) {
                    if (st->size() != 1) {
                        error("variant only supports one initializer", "", "",
                            st->front()->at, CompilationError::field_already_initialized);
                        allGood = false;
                    } else {
                        mkv->variants.push_back(st->front()->clone());
                    }
                }
                if ( allGood ) {
                    reportAstChanged();
                    return mkv;
                }
            }
            // see if there are any duplicate fields
            if ( expr->makeType->baseType == Type::tStructure ) {
                bool anyDuplicates = false;
                for ( auto & st : expr->structs ) {
                    das_set<string> fld;
                    for ( auto & fi : *st ) {
                        if ( fld.find(fi->name) != fld.end() ) {
                            error("field " + fi->name + " is already initialized", "", "",
                                fi->at, CompilationError::field_already_initialized);
                            anyDuplicates = true;
                        } else {
                            fld.insert(fi->name);
                        }
                    }
                }
                if ( anyDuplicates ) return Visitor::visit(expr);
                // see if we need to fill in missing fields
                if ( expr->useInitializer && expr->makeType->structType ) {
                    for ( auto & stf : expr->makeType->structType->fields  ) {
                        if ( stf.init  ) {
                            if ( !stf.init->type || stf.init->type->isAuto() ) {
                                error("not fully resolved yet", "", "", expr->at);
                                return Visitor::visit(expr);
                            }
                        }
                    }
                    if ( !expr->structs.size() ) expr->structs.emplace_back(make_smart<MakeStruct>());
                    for ( auto & st : expr->structs ) {
                        for ( auto & fi : expr->makeType->structType->fields ) {
                            if ( fi.init ) {
                                auto it = find_if(st->begin(), st->end(), [&](const MakeFieldDeclPtr & fd){
                                    return fd->name == fi.name;
                                });
                                if ( it==st->end() ) {
                                    auto msf = make_smart<MakeFieldDecl>(fi.at, fi.name, fi.init->clone(), !fi.init->type->canCopy(), false);
                                    st->push_back(msf);
                                    reportAstChanged();
                                }
                            }
                        }
                    }
                    expr->useInitializer = false;
                }
                // see if we need to init fields
                if ( expr->makeType->structType ) {
                    expr->initAllFields = !expr->structs.empty();
                    for ( auto & st : expr->structs ) {
                        if ( st->size() == expr->makeType->structType->fields.size() ) {
                            for ( auto & va : *st ) {
                                if ( va->value->rtti_isMakeLocal() ) {
                                    auto mkl = static_pointer_cast<ExprMakeLocal>(va->value);
                                    expr->initAllFields &= mkl->initAllFields;
                                }
                            }
                        } else {
                            expr->initAllFields = false;
                            break;
                        }
                    }
                } else {
                    expr->initAllFields = false;
                }
            } else {
                if ( expr->makeType->baseType==Type::tTuple && expr->structs.size()==0 ) {
                    expr->initAllFields = true;
                }
            }
            // result type
            auto resT = make_smart<TypeDecl>(*expr->makeType);
            uint32_t resDim = uint32_t(expr->structs.size());
            if ( resDim==0 ) {
                // resT->dim.clear();
            } else if ( resDim!=1 ) {
                resT->dim.resize(1);
                resT->dim[0] = resDim;
            } else {
                if ( expr->makeType->dim.size()==1 && expr->makeType->dim[0]==1 ) {
                    // do nothing
                } else if ( expr->makeType->dim.size()==1 && expr->makeType->dim[0]==TypeDecl::dimAuto ) {
                    resT->dim.resize(1);
                    resT->dim[0] = 1;
                    expr->makeType->dim[0] = 1;
                    reportAstChanged();
                } else {
                    resT->dim.clear();
                }
            }
            expr->type = resT;
            if ( expr->type->isString() ) {
                reportAstChanged();
                auto ecs = make_smart<ExprConstString>(expr->at);
                ecs->type = make_smart<TypeDecl>(Type::tString);
                return ecs;
            } else if ( expr->type->isEnumT() ) {
                auto f0 = expr->type->enumType->find(0,"");
                if ( !f0.empty() ) {
                    reportAstChanged();
                    auto et = make_smart<TypeDecl>(*expr->type);
                    et->ref = false;
                    auto ens = make_smart<ExprConstEnumeration>(expr->at, f0, et);
                    ens->type = make_smart<TypeDecl>(*et);
                    return ens;
                } else {
                    error("[[" + describeType(expr->makeType) + "() ]] enumeration is missing 0 value", "", "",
                          expr->at, CompilationError::invalid_type);
                }
            } else if ( expr->type->isPointer() ) {
                expr->type->ref = false;
                reportAstChanged();
                auto ews = make_smart<ExprConstPtr>(expr->at);
                ews->type = make_smart<TypeDecl>(*expr->type);
                ews->isSmartPtr = expr->type->smartPtr;
                if ( expr->type->firstType ) {
                    ews->ptrType = make_smart<TypeDecl>(*expr->type->firstType);
                }
                return ews;
            } else if ( expr->type->isWorkhorseType() ) {
                expr->type->ref = false;
                reportAstChanged();
                auto ews = Program::makeConst(expr->at, expr->type, v_zero());
                ews->type = make_smart<TypeDecl>(*expr->type);
                return ews;
            } else if ( !expr->type->isRefType() ) {
                expr->type->ref = true;
            }
            if ( expr->type->isAuto() ) {
                error("[[auto ]] needs to be fully inferred", "", "",
                    expr->at, CompilationError::invalid_type);
                return Visitor::visit(expr);
            }
            verifyType(expr->type);
            return Visitor::visit(expr);
        }
    // make tuple
        virtual void preVisit ( ExprMakeTuple * expr ) override {
            Visitor::preVisit(expr);
            expr->makeType.reset();
            expr->initAllFields = true;
        }
        virtual ExpressionPtr visitMakeTupleIndex ( ExprMakeTuple * expr, int index, Expression * init, bool lastField ) override {
            if (!init->type) {
                return Visitor::visitMakeArrayIndex(expr, index, init, lastField);
            }
            if (!init->type->canCopy() && init->type->canMove() && init->type->isConst()) {
                error("can't move from a constant value " + describeType(init->type), "", "",
                    init->at, CompilationError::cant_move);
            }
            if ( init->rtti_isMakeLocal() ) {
                auto initl = static_cast<ExprMakeLocal *>(init);
                expr->initAllFields &= initl->initAllFields;
            }

            return Expression::autoDereference(Visitor::visitMakeTupleIndex(expr, index, init, lastField));
        }
        virtual ExpressionPtr visit ( ExprMakeTuple * expr ) override {
            for ( auto & val : expr->values ) {
                if ( !val->type || val->type->isAutoOrAlias() ) {
                    error("not fully defined tuple element type", "", "",
                        expr->at, CompilationError::invalid_type);
                    return Visitor::visit(expr);
                }
            }
            if ( expr->recordType ) {
                if ( !expr->recordType->isTuple() ) {
                    error("internal error. ExprMakeTuple with non-tuple record type", "", "",
                        expr->at, CompilationError::invalid_type);
                    return Visitor::visit(expr);
                }
                size_t argCount = expr->values.size();
                if ( expr->recordType->argTypes.size() != argCount ) {
                    error("expecting " + to_string(argCount) + " arguments in " + describeType(expr->recordType), "", "",
                        expr->at, CompilationError::invalid_type);
                    return Visitor::visit(expr);
                }
                auto mkt = make_smart<TypeDecl>(Type::tTuple);
                for ( size_t ai=0; ai!=argCount; ++ai ) {
                    const auto & val = expr->values[ai];
                    const auto & argT = expr->recordType->argTypes[ai];
                    if ( !argT->isSameType(*val->type,RefMatters::no, ConstMatters::no, TemporaryMatters::no) ) {
                        error("invalid argument _" + to_string(ai) + ", expecting " +
                                describeType(argT) + ", passing " + describeType(val->type), "", "",
                              expr->at, CompilationError::invalid_type);
                    }
                    auto valT = make_smart<TypeDecl>(*argT);
                    valT->ref = false;
                    valT->constant = false;
                    mkt->argTypes.push_back(valT);
                }
                expr->makeType = mkt;
            } else {
                expr->makeType = make_smart<TypeDecl>(Type::tTuple);
                for ( auto & val : expr->values ) {
                    auto valT = make_smart<TypeDecl>(*val->type);
                    valT->ref = false;
                    valT->constant = false;
                    expr->makeType->argTypes.push_back(valT);
                }
            }
            expr->type = make_smart<TypeDecl>(*expr->makeType);
            verifyType(expr->type);
            if ( expr->isKeyValue ) {
                auto keyType = expr->makeType->argTypes[0];
                if ( keyType->ref ) {
                    error("a => b tuple key can't be declared as a reference", "", "",
                          keyType->at,CompilationError::invalid_table_type);
                }
                if ( !keyType->isWorkhorseType() ) {
                    error("a => b tuple key has to be declare as a basic 'hashable' type", "", "",
                          keyType->at,CompilationError::invalid_table_type);
                }
            }
            for (auto & argType : expr->makeType->argTypes) {
                if ( !argType->canCopy() && !argType->canMove() ) {
                    error("tuple element has to be copyable or moveable", "", "",
                        expr->at, CompilationError::invalid_type);
                }
            }
            return Visitor::visit(expr);
        }
    // make array
        virtual void preVisit ( ExprMakeArray * expr ) override {
            Visitor::preVisit(expr);
            if ( expr->makeType && expr->makeType->isExprType() ) {
                return;
            }
            if ( expr->makeType && expr->makeType->isAuto() ) {
                return;
            }
            verifyType(expr->makeType);
            if ( expr->makeType->dim.size()>1 ) {
                error("[[" + describeType(expr->makeType) + "]] array can only initialize single dimension arrays", "", "",
                      expr->at, CompilationError::invalid_type);
            } else if ( expr->makeType->dim.size()==1 && expr->makeType->dim[0]!=int32_t(expr->values.size()) ) {
                error("[[" + describeType(expr->makeType) + "]] array dimension mismatch, provided " +
                      to_string(expr->values.size()) + " elements, expecting " + to_string(expr->makeType->dim[0]), "", "",
                    expr->at, CompilationError::invalid_type);
            } else if ( expr->makeType->ref ) {
                error("[[" + describeType(expr->makeType) + "]] array can't be reference", "", "",
                    expr->at, CompilationError::invalid_type);
            }
            expr->recordType = make_smart<TypeDecl>(*expr->makeType);
            expr->recordType->dim.clear();
            expr->initAllFields = true;
        }
        virtual ExpressionPtr visitMakeArrayIndex ( ExprMakeArray * expr, int index, Expression * init, bool last ) override {
            if ( expr->makeType && expr->makeType->isExprType() ) {
                return Visitor::visitMakeArrayIndex(expr,index,init,last);
            }
            if ( !expr->recordType ) {
                if ( index==0 ) {
                    if ( init->type && !init->type->isAutoOrAlias() ) {
                        // blah[] vs blah
                        TypeDeclPtr mkt;
                        if ( expr->makeType->dim.size() && !init->type->dim.size() ) {
                            if (expr->makeType->dim.size() == 1 && expr->makeType->dim[0] == TypeDecl::dimAuto) {
                                auto infT = make_smart<TypeDecl>(*expr->makeType);
                                infT->dim.clear();
                                mkt = TypeDecl::inferGenericType(infT, init->type, false, false, nullptr);
                                if (mkt) {
                                    mkt->dim.resize(1);
                                    mkt->dim[0] = int32_t(expr->values.size());
                                }
                            }
                        } else {
                            mkt = TypeDecl::inferGenericType(expr->makeType, init->type, false, false, nullptr);
                        }
                        if ( !mkt ) {
                            error("array type can't be inferred, "
                                  + describeType(expr->makeType) + " = " + describeType(init->type), "", "",
                                  init->at, CompilationError::invalid_array_type );
                        } else {
                            mkt->ref = false;
                            mkt->constant = false;
                            TypeDecl::applyAutoContracts(mkt, init->type);
                            expr->makeType = mkt;
                            reportAstChanged();
                            return Visitor::visitMakeArrayIndex(expr,index,init,last);
                        }
                    } else {
                        error("can't infer array auto type, first element type is undefined", "", "",
                            init->at, CompilationError::invalid_array_type );
                    }
                }
            }
            if ( !init->type|| !expr->recordType ) {
                return Visitor::visitMakeArrayIndex(expr,index,init,last);
            }
            if ( !canCopyOrMoveType(expr->recordType,init->type,TemporaryMatters::no,init) ) {
                if ( expr->recordType->isVariant() ) {
                    int uidx = expr->recordType->getVariantUniqueFieldIndex(init->type);
                    if ( uidx==-1 ) {
                        vector<pair<string,TypeDeclPtr>> options;
                        for ( size_t vi=0, vis=expr->recordType->argTypes.size(); vi!=vis; ++vi ) {
                            const auto & argT = expr->recordType->argTypes[vi];
                            if ( argT->isSameType(*init->type,RefMatters::no,ConstMatters::no,TemporaryMatters::no,AllowSubstitute::no) ) {
                                options.emplace_back(expr->recordType->argNames[vi], argT);
                            }
                        }
                        if ( options.size()==0 ) {
                            error("can't recognize unique variant " + describeType(init->type) + " in " + describeType(expr->recordType), "", "",
                                init->at, CompilationError::invalid_type);
                        } else {
                            if ( verbose ) {
                                TextWriter tw;
                                for ( auto & opt : options ) {
                                    tw << "\t\t" << opt.first << ":" << describeType(opt.second);
                                    if ( opt!=options.back() )
                                        tw << "\n";
                                }
                                error("can't recognize unique variant " + describeType(init->type) + " in " + describeType(expr->recordType),
                                    "\tcandidates are:\n" + tw.str(), "",
                                    init->at, CompilationError::invalid_type);
                            } else {
                                error("can't recognize unique variant", "", "",
                                    init->at, CompilationError::invalid_type);
                            }
                        }
                    } else {
                        auto mkv = make_smart<ExprMakeVariant>(expr->at);
                        mkv->variants.push_back(make_smart<MakeFieldDecl>(
                            init->at,
                            expr->recordType->argNames[uidx],
                            init->clone(),
                            false,          // move
                            false           // clone
                        ));
                        mkv->makeType = make_smart<TypeDecl>(*expr->recordType);
                        reportAstChanged();
                        return mkv;
                    }
                } else {
                    error("can't initialize array element " + to_string(index) + "; expecting "
                        + describeType(expr->recordType)+", passing " + describeType(init->type), "", "",
                            init->at, CompilationError::invalid_type );
                }
            } else if ( !expr->recordType->canCopy() && expr->recordType->canMove() && init->type->isConst() ) {
                error("can't move from a constant value\n\t" + describeType(init->type), "", "",
                    init->at, CompilationError::cant_move);
            } else if ( init->type->isTemp(true,false) ) {
                error("can't initialize array element with temporary value", "", "",
                    init->at, CompilationError::cant_pass_temporary);
            }
            if ( init->rtti_isMakeLocal() ) {
                auto initl = static_cast<ExprMakeLocal *>(init);
                expr->initAllFields &= initl->initAllFields;
            }
            return Visitor::visitMakeArrayIndex(expr,index,init,last);
        }
        virtual ExpressionPtr visit ( ExprMakeArray * expr ) override {
            if ( expr->makeType && expr->makeType->isExprType() ) {
                return Visitor::visit(expr);
            }
            if ( !expr->recordType ) {
                return Visitor::visit(expr);
            }
            if ( expr->recordType->isVariant() ) {
                bool canPromoteToMakeVariant = true;
                for ( auto & eval : expr->values ) {
                    if ( !eval->rtti_isMakeVariant() ) {
                        canPromoteToMakeVariant = false;
                        break;
                    } else {
                        auto emkv = static_pointer_cast<ExprMakeVariant>(eval);
                        if ( emkv->variants.size()!=1 ) {
                            canPromoteToMakeVariant = false;
                            break;
                        }
                    }
                }
                if ( canPromoteToMakeVariant ) {
                        auto mkv = make_smart<ExprMakeVariant>(expr->at);
                        for ( const auto & eval : expr->values ) {
                            auto emkv = static_pointer_cast<ExprMakeVariant>(eval);
                            DAS_ASSERT(emkv->variants.size()==1);
                            const auto & fmkv = emkv->variants[0];
                            mkv->variants.push_back(make_smart<MakeFieldDecl>(
                                fmkv->at,
                                fmkv->name,
                                fmkv->value->clone(),
                                bool(fmkv->moveSemantics),
                                bool(fmkv->cloneSemantics)
                            ));
                        }
                        mkv->makeType = make_smart<TypeDecl>(*expr->makeType);
                        reportAstChanged();
                        return mkv;
                }
            }
            if ( !expr->recordType->canCopy() && !expr->recordType->canMove() ) {
                error("array element has to be copyable or moveable", "", "",
                    expr->at, CompilationError::invalid_type);
            }
            auto resT = make_smart<TypeDecl>(*expr->makeType);
            uint32_t resDim = uint32_t(expr->values.size());
            if ( resDim!=1 || expr->makeType->dim.size() ) {
                resT->dim.resize(1);
                resT->dim[0] = resDim;
            } else {
                DAS_ASSERT(expr->values.size()==1);
                auto eval = expr->values[0];
                if ( !eval->type ) {
                    error("unknown value type", "", "",
                        expr->at, CompilationError::invalid_type);
                    return Visitor::visit(expr);
                } else if ( !expr->recordType->isSameType(*(eval->type),RefMatters::no,ConstMatters::no,TemporaryMatters::no,AllowSubstitute::no) ) {
                    error("incompatible value type. expecting " + describeType(expr->recordType) + " vs " + describeType(eval->type), "", "",
                        eval->at, CompilationError::invalid_type);
                    return Visitor::visit(expr);
                } else {
                    reportAstChanged();
                    auto resExpr = expr->values[0];
                    if ( resExpr->rtti_isMakeTuple() ) {
                        auto mkt = static_pointer_cast<ExprMakeTuple>(resExpr);
                        mkt->recordType = make_smart<TypeDecl>(*expr->recordType);
                        mkt->makeType.reset();
                    }
                    return resExpr;
                }
            }
            expr->type = resT;
            verifyType(expr->type);
            return Visitor::visit(expr);
        }
    // array comprehension
        virtual void preVisitArrayComprehensionSubexpr ( ExprArrayComprehension * expr, Expression * subexpr ) override {
            Visitor::preVisitArrayComprehensionSubexpr(expr, subexpr);
            pushVarStack();
            auto eFor = static_cast<ExprFor *>(expr->exprFor.get());
            preVisitForStack(eFor);
        }
        virtual void preVisitArrayComprehensionWhere ( ExprArrayComprehension * expr, Expression * where ) override {
            Visitor::preVisitArrayComprehensionWhere(expr, where);
        }
        virtual ExpressionPtr visit ( ExprArrayComprehension * expr ) override {
            popVarStack();
            if ( expr->subexpr->type ) {
                if ( !expr->subexpr->type->canCopy() && !expr->subexpr->type->canMove() ) {
                    error("comprehension element has to be copyable or moveable", "", "",
                        expr->at, CompilationError::invalid_type);
                } else if ( expr->subexpr->type->isAutoOrAlias() ) {
                    error("comprehension element type is not resolved", "", "",
                        expr->at, CompilationError::invalid_type);
                } else {
                    auto pAC = expr->generatorSyntax ?
                        generateComprehensionIterator(expr) : generateComprehension(expr);
                    reportAstChanged();
                    return pAC;
                }
            }
            return Visitor::visit(expr);
        }
    };

    // try infer, if failed - no macros
    // run macros til any of them does work, then reinfer and restart (i.e. infer after each macro)
    void Program::inferTypes(TextWriter &logs, ModuleGroup & libGroup) {
        newLambdaIndex = 1;
        inferTypesDirty(logs, false);
        bool anyMacrosDidWork = false;
        bool anyMacrosFailedToInfer = false;
        if ( failed() ) goto failed_to_infer;
        do {
            anyMacrosDidWork = false;
            anyMacrosFailedToInfer = false;
            auto modMacro = [&](Module * mod) -> bool {    // we run all macros for each module
                if ( thisModule->isVisibleDirectly(mod) && mod!=thisModule.get() ) {
                    for ( const auto & pm : mod->macros ) {
                        bool anyWork = pm->apply(this, thisModule.get());
                        if ( failed() ) {                       // if macro failed, we report it, and we are done
                            error("macro " + mod->name + "::" + pm->name + " failed", "", "", LineInfo());
                            return false;
                        }
                        if ( anyWork ) {                        // if macro did anything, we done
                            reportingInferErrors = true;
                            inferTypesDirty(logs, true);
                            reportingInferErrors = false;
                            if ( failed() ) {                   // if it failed to infer types after, we report it
                                error("macro " + mod->name + "::" + pm->name + " failed to infer", "", "", LineInfo());
                                anyMacrosFailedToInfer = true;
                                return false;
                            }
                            anyMacrosDidWork = true;            // if any work been done, we start over
                            return false;
                        }
                    }
                }
                return true;
            };
            Module::foreach(modMacro);
            if (failed()) break;
            if (anyMacrosDidWork) continue;
            if (relocatePotentiallyUninitialized(logs)) {
                anyMacrosDidWork = true;
                reportingInferErrors = true;
                inferTypesDirty(logs, true);
                reportingInferErrors = false;
                if ( failed() ) {
                    error("interlan compiler error. variable relocation infer to fail", "", "", LineInfo());
                }
                continue;
            }
            libGroup.foreach(modMacro, "*");
        } while ( !failed() && anyMacrosDidWork );
    failed_to_infer:;
        if ( failed() && !anyMacrosFailedToInfer && !macroException ) {
            reportingInferErrors = true;
            inferTypesDirty(logs, true);
            reportingInferErrors = false;
        }
    }

    void Program::inferTypesDirty(TextWriter & logs, bool verbose) {
        const bool log = options.getBoolOption("log_infer_passes",false);
        int pass = 0, maxPasses = 50;
        if (auto maxP = options.find("max_infer_passes", Type::tInt)) {
            maxPasses = maxP->iValue;
        }
        if ( log ) {
            logs << "INITIAL CODE:\n" << *this;
        }
        for ( pass = 0; pass < maxPasses; ++pass ) {
            if ( macroException ) break;
            failToCompile = false;
            errors.clear();
            InferTypes context(this);
            context.verbose = verbose || log;
            visit(context);
            for ( auto efn : context.extraFunctions ) {
                addFunction(efn);
            }
            bool anyMacrosDidWork = false;
            auto modMacro = [&](Module * mod) -> bool {
                if ( thisModule->isVisibleDirectly(mod) && mod!=thisModule.get() ) {
                    for ( const auto & pm : mod->inferMacros ) {
                        anyMacrosDidWork |= pm->apply(this, thisModule.get());
                    }
                }
                return true;
            };
            Module::foreach(modMacro);
            library.foreach(modMacro, "*");
            if ( log ) {
                logs << "PASS " << pass << ":\n" << *this;
                sort(errors.begin(), errors.end());
                for (auto & err : errors) {
                    logs << reportError(err.at, err.what, err.extra, err.fixme, err.cerr);
                }
            }
            if ( anyMacrosDidWork ) continue;
            if ( context.finished() ) break;
        }
        if (pass == maxPasses) {
            error("type inference exceeded maximum allowed number of passes ("+to_string(maxPasses)+")\n"
                    "this is likely due to a loop in the type system", "", "",
                LineInfo(), CompilationError::too_many_infer_passes);
        }
    }
}

