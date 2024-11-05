#include "daScript/misc/platform.h"

#include "parser_impl.h"
#include "parser_state.h"

#include "daScript/ast/ast_generate.h"

#undef yyextra
#define yyextra (*((das::DasParserState **)(scanner)))

void das_collect_keywords ( das::Module * mod, yyscan_t yyscanner );

namespace das {

    static __forceinline string inThisModule ( const string & name ) { return "_::" + name; }

    void das2_yyerror ( yyscan_t scanner, const string & error, const LineInfo & at, CompilationError cerr ) {
        yyextra->g_Program->error(error,"","",at,cerr);
    }

    void das_yyerror ( yyscan_t scanner, const string & error, const LineInfo & at, CompilationError cerr ) {
        yyextra->g_Program->error(error,"","",at,cerr);
    }

    void das_checkName ( yyscan_t scanner, const string & name, const LineInfo &at ) {
        if ( name.length()>=2 && name[0]=='_' && name[1]=='_' ) {
            yyextra->g_Program->error("names starting with __ are reserved, " + name,"","",at,CompilationError::invalid_name);
        }
    }

    void appendDimExpr ( TypeDecl * typeDecl, Expression * dimExpr ) {
        if ( dimExpr ) {
            int32_t dI = TypeDecl::dimConst;
            if ( dimExpr->rtti_isConstant() ) {                // note: this shortcut is here so we don`t get extra infer pass on every array
                auto cI = (ExprConst *) dimExpr;
                auto bt = cI->baseType;
                if ( bt==Type::tInt || bt==Type::tUInt ) {
                    dI = cast<int32_t>::to(cI->value);
                }
            }
            typeDecl->dim.push_back(dI);
            typeDecl->dimExpr.push_back(ExpressionPtr(dimExpr));
        } else {
            typeDecl->dim.push_back(TypeDecl::dimAuto);
            typeDecl->dimExpr.push_back(nullptr);
        }
        typeDecl->removeDim = false;
    }

    vector<ExpressionPtr> sequenceToList ( Expression * arguments ) {
        vector<ExpressionPtr> argList;
        auto arg = arguments;
        if ( arg->rtti_isSequence() ) {
            while ( arg->rtti_isSequence() ) {
                auto pSeq = static_cast<ExprSequence *>(arg);
                DAS_ASSERT(!pSeq->right->rtti_isSequence());
                argList.push_back(pSeq->right);
                arg = pSeq->left.get();
            }
            argList.push_back(arg);
            reverse(argList.begin(),argList.end());
            delete arguments;
        } else {
            argList.push_back(ExpressionPtr(arg));
        }
        return argList;
    }

    vector<ExpressionPtr> typesAndSequenceToList  ( vector<Expression *> * declL, Expression * arguments ) {
        vector<ExpressionPtr> args;
        vector<ExpressionPtr> seq;
        if ( arguments ) seq = sequenceToList(arguments);
        args.reserve(declL->size() + seq.size());
        for ( auto & decl : *declL ) args.push_back(ExpressionPtr(decl));
        for ( auto & arg : seq ) args.push_back(move(arg));
        delete declL;
        return args;
    }

    Expression * sequenceToTuple ( Expression * arguments ) {
        if ( arguments->rtti_isSequence() ) {
            auto tup = new ExprMakeTuple(arguments->at);
            tup->values = sequenceToList(arguments);
            return tup;
        } else {
            return arguments;
        }
    }

    ExprLooksLikeCall * parseFunctionArguments ( ExprLooksLikeCall * pCall, Expression * arguments ) {
        if ( arguments ) {
            pCall->arguments = sequenceToList(arguments);
        }
        return pCall;
    }

    void deleteTypeDeclarationList ( vector<Expression *> * list ) {
        if ( !list ) return;
        for ( auto pD : *list )
            delete pD;
        delete list;
    }

    void deleteVariableDeclarationList ( vector<VariableDeclaration *> * list ) {
        if ( !list ) return;
        for ( auto pD : *list )
            delete pD;
        delete list;
    }

    void varDeclToTypeDecl ( yyscan_t scanner, TypeDecl * pType, vector<VariableDeclaration*> * list, bool needNames ) {
        bool anyNames = false;
        for ( auto pDecl : *list ) {
            if ( pDecl->pTypeDecl ) {
                int count = pDecl->pNameList ? int(pDecl->pNameList->size()) : 1;
                for ( int ai=0; ai!=count; ++ai ) {
                    auto pVarType = make_smart<TypeDecl>(*pDecl->pTypeDecl);
                    if ( pDecl->pInit ) {
                        das_yyerror(scanner,"can't have default values in a type declaration",
                            (*pDecl->pNameList)[ai].at,CompilationError::cant_initialize);
                    }
                    pType->argTypes.push_back(pVarType);
                    if ( needNames && pDecl->pNameList && !(*pDecl->pNameList)[ai].name.empty() ) {
                        if ( !(*pDecl->pNameList)[ai].aka.empty() ) {
                            das_yyerror(scanner,"type declaration can't have an aka", (*pDecl->pNameList)[ai].at,
                                CompilationError::invalid_aka);
                        }
                        anyNames = true;
                    }
                }
            }
        }
        if ( anyNames ) {
            for ( auto pDecl : *list ) {
                if ( pDecl->pTypeDecl ) {
                    if ( pDecl->pNameList ) {
                        for ( const auto & name : *pDecl->pNameList ) {
                            pType->argNames.push_back(name.name);
                        }
                    } else {
                        pType->argNames.push_back(string());
                    }
                }
            }
        }
    }

    Annotation * findAnnotation ( yyscan_t scanner, const string & name, const LineInfo & at ) {
        auto ann = yyextra->g_Program->findAnnotation(name);
        if ( ann.size()==1 ) {
            return ann.back().get();
        } else if ( ann.size()==0 ) {
            das_yyerror(scanner,"annotation " + name + " not found", at, CompilationError::annotation_not_found );
            return nullptr;
        } else {
            string candidates = yyextra->g_Program->describeCandidates(ann);
            das_yyerror(scanner,"too many options for annotation " + name + "\n" + candidates, at, CompilationError::annotation_not_found );
            return nullptr;
        }
    }

    void runFunctionAnnotations ( yyscan_t scanner, DasParserState * extra, Function * func, AnnotationList * annL, const LineInfo & at ) {
        if ( annL ) {
            for ( auto itA = annL->begin(); itA!=annL->end();  ) {
                auto pA = *itA;
                if ( pA->annotation ) {
                    if ( pA->annotation->rtti_isFunctionAnnotation() ) {
                        auto ann = static_pointer_cast<FunctionAnnotation>(pA->annotation);
                        string err;
                        if ( !ann->apply(func, *yyextra->g_Program->thisModuleGroup, pA->arguments, err) ) {
                            das_yyerror(scanner,"macro [" +pA->annotation->name + "] failed to apply to a function " + func->name + "\n" + err, at,
                                CompilationError::invalid_annotation);
                        } else if ( ann->name=="type_function" && ann->module->name=="$" ) {
                            // this is awkward. we need this so that [type_function] can be used in the same module
                            auto mod = func->module ? func->module : extra->g_Program->thisModule.get();
                            string keyword = mod->name.empty() ? func->name : mod->name + "::" + func->name;
                            extra->das_keywords[func->name] = DasKeyword{false,true,keyword};
                        }
                        itA ++;
                    } else {
                        das_yyerror(scanner, pA->getMangledName() + " is not a function macro, functions are only allowed function macros",
                            at, CompilationError::invalid_annotation);
                        itA = annL->erase(itA);
                    }
                } else {
                    itA = annL->erase(itA);
                }
            }
            swap ( func->annotations, *annL );
            for ( const auto & pA : *annL ) {
                func->annotations.push_back(pA);
            }
            delete annL;
        }
    }

    Expression * ast_arrayComprehension ( yyscan_t scanner, const LineInfo & loc, vector<VariableNameAndPosition> * iters,
        Expression * srcs, Expression * subexpr, Expression * where, const LineInfo & forend, bool genSyntax, bool tableSyntax ) {
        auto pFor = make_smart<ExprFor>(loc);
        pFor->visibility = forend;
        for ( const auto & np : *iters ) {
            pFor->iterators.push_back(np.name);
            pFor->iteratorsAka.push_back(np.aka);
            if ( !np.aka.empty() ) {
                das_yyerror(scanner,"array comprehension can't have an aka",np.at,
                    CompilationError::invalid_aka);
            }
            pFor->iteratorsAt.push_back(np.at);
            pFor->iteratorsTags.push_back(np.tag);
        }
        delete iters;
        pFor->sources = sequenceToList(srcs);
        auto pAC = new ExprArrayComprehension(loc);
        pAC->generatorSyntax = genSyntax;
        pAC->tableSyntax = tableSyntax;
        pAC->exprFor = pFor;
        pAC->subexpr = ExpressionPtr(subexpr);
        if ( where ) {
            pAC->exprWhere = ExpressionPtr(where);
        }
        return pAC;
    }

    Structure * ast_structureName ( yyscan_t scanner, bool sealed, string * name, const LineInfo & atName,
        string * parent, const LineInfo & atParent ) {
        das_checkName(scanner,*name,atName);
        StructurePtr pStruct;
        if ( parent ) {
            auto structs = yyextra->g_Program->findStructure(*parent);
            if ( structs.size()==1 ) {
                pStruct = structs.back()->clone();
                pStruct->name = *name;
                pStruct->parent = structs.back().get();
                if ( pStruct->parent->sealed ) {
                    das_yyerror(scanner,"can't derive from a sealed class or structure "+*parent,atParent,
                        CompilationError::invalid_override);
                }
                pStruct->annotations.clear();
                pStruct->genCtor = false;
                pStruct->macroInterface = pStruct->parent && pStruct->parent->macroInterface;

            } else if ( structs.size()==0 ) {
                das_yyerror(scanner,"parent structure not found " + *parent,atParent,
                    CompilationError::structure_not_found);
            } else {
                string candidates = yyextra->g_Program->describeCandidates(structs);
                das_yyerror(scanner,"too many options for " + *parent + "\n" + candidates,atParent,
                    CompilationError::structure_not_found);
            }
            delete parent;
        }
        if ( !pStruct ) {
            pStruct = make_smart<Structure>(*name);
        }
        pStruct->sealed = sealed;
        if ( !yyextra->g_Program->addStructure(pStruct) ) {
            das_yyerror(scanner,"structure is already defined "+*name,atName,
                CompilationError::structure_already_declared);
            delete name;
            return nullptr;
        } else {
            yyextra->g_thisStructure = pStruct.get();
            delete name;
            return pStruct.get();
        }
    }

    void ast_structureDeclaration (  yyscan_t scanner, AnnotationList * annL, const LineInfo & loc, Structure * ps,
        const LineInfo & atPs, vector<VariableDeclaration*> * list ) {
        if ( ps ) {
            auto pStruct = ps;
            pStruct->at = atPs;
            if ( pStruct->parent && pStruct->parent->isClass != pStruct->isClass ) {
                if ( pStruct->isClass ) {
                    das_yyerror(scanner,"class can only derive from a class", pStruct->at,
                        CompilationError::invalid_override);
                } else {
                    das_yyerror(scanner,"structure can only derive from a structure", pStruct->at,
                        CompilationError::invalid_override);
                }
                delete annL;
                deleteVariableDeclarationList(list);
                return;
            }
            if ( pStruct->isClass ) {
                makeClassRtti(pStruct);
                auto virtfin = makeClassFinalize(pStruct);
                if ( !yyextra->g_Program->addFunction(virtfin) ) {
                    das_yyerror(scanner,"built-in finalizer is already defined " + virtfin->getMangledName(),
                        virtfin->at, CompilationError::function_already_declared);
                }
            }
            for ( auto & ffd : pStruct->fields ) {
                ffd.implemented = false;
            }
            for ( auto pDecl : *list ) {
                for ( const auto & name_at : *pDecl->pNameList ) {
                    if ( !pStruct->isClass && pDecl->isPrivate ) {
                        das_yyerror(scanner,"only class member can be private "+name_at.name,name_at.at,
                            CompilationError::invalid_private);
                    }
                    if ( !pStruct->isClass && pDecl->isStatic ) {
                        das_yyerror(scanner,"only class member can be static "+name_at.name,name_at.at,
                            CompilationError::invalid_static);
                    }
                    if ( (pDecl->override || pDecl->sealed) && pDecl->isStatic ) {
                        das_yyerror(scanner,"static member can't be sealed or override "+name_at.name,name_at.at,
                            CompilationError::invalid_static);
                    }
                    if ( pDecl->isStatic && pDecl->annotation ) {
                        das_yyerror(scanner,"static member can't have an annotation "+name_at.name,name_at.at,
                            CompilationError::invalid_static);
                    }
                    auto oldFd = (Structure::FieldDeclaration *) pStruct->findField(name_at.name);
                    if ( !oldFd ) {
                        if ( pDecl->override ) {
                            das_yyerror(scanner,"structure field is not overriding anything "+name_at.name,name_at.at,
                                CompilationError::invalid_override);
                        } else {
                            auto td = make_smart<TypeDecl>(*pDecl->pTypeDecl);
                            auto init = pDecl->pInit ? ExpressionPtr(pDecl->pInit->clone()) : nullptr;
                            if ( pDecl->isStatic ) {
                                auto pVar = make_smart<Variable>();
                                pVar->name = pStruct->name + "`" + name_at.name;
                                pVar->type = td;
                                pVar->init = init;
                                pVar->init_via_move = pDecl->init_via_move;
                                pVar->static_class_member = true;
                                pVar->private_variable = pDecl->isPrivate || pStruct->privateStructure;
                                if ( !pStruct->module->addVariable(pVar,true) ) {
                                    das_yyerror(scanner,"static variable already exists "+name_at.name,name_at.at,
                                        CompilationError::invalid_static);
                                }
                                pStruct->hasStaticMembers = true;
                            } else {
                                pStruct->fields.emplace_back(name_at.name, td, init,
                                    pDecl->annotation ? *pDecl->annotation : AnnotationArgumentList(),
                                    pDecl->init_via_move, name_at.at);
                                auto & ffd = pStruct->fields.back();
                                ffd.privateField = pDecl->isPrivate;
                                ffd.sealed = pDecl->sealed;
                                ffd.implemented = true;
                            }
                        }
                    } else {
                        if ( pDecl->isStatic ) {
                            das_yyerror(scanner,"static structure field is already declared "+name_at.name
                                +", can't have both member at static at the same name",name_at.at,
                                    CompilationError::invalid_static);
                        } else if ( pDecl->sealed || pDecl->override ) {
                            if ( oldFd->sealed ) {
                                das_yyerror(scanner,"structure field "+name_at.name+" is sealed",
                                    name_at.at, CompilationError::invalid_override);
                            }
                            auto init = pDecl->pInit ? ExpressionPtr(pDecl->pInit->clone()) : nullptr;
                            oldFd->init = init;
                            oldFd->parentType = oldFd->type->isAuto();
                            oldFd->privateField = pDecl->isPrivate;
                            oldFd->sealed = pDecl->sealed;
                            oldFd->implemented = true;
                        } else {
                            das_yyerror(scanner,"structure field is already declared "+name_at.name
                                +", use override to replace initial value instead",name_at.at,
                                    CompilationError::invalid_override);
                        }
                    }
                }
            }
            auto pparent = pStruct->parent;
            while ( pparent ) {
                vector<AnnotationDeclarationPtr> annLextra;
                for ( auto & ad : pparent->annotations ) {
                    if ( ad->inherited ) {
                        if ( ad->annotation->rtti_isStructureAnnotation() ) {
                            annLextra.push_back(make_smart<AnnotationDeclaration>(*ad));
                        }
                    }
                }
                if ( !annLextra.empty() ) {
                    if ( annL==nullptr ) {
                        annL = new AnnotationList();
                    }
                    annL->insert(annL->begin(), annLextra.begin(), annLextra.end());
                }
                pparent = pparent->parent;
            }

            if ( annL ) {
                for ( auto pA : *annL ) {
                    if ( pA->annotation ) {
                        if ( pA->annotation->rtti_isStructureAnnotation() ) {
                            auto ann = static_pointer_cast<StructureAnnotation>(pA->annotation);
                            string err;
                            if ( !ann->touch(pStruct, *yyextra->g_Program->thisModuleGroup, pA->arguments, err) ) {
                                das_yyerror(scanner,"macro [" +pA->annotation->name + "] failed to apply to the structure " + pStruct->name + "\n" + err,
                                    loc, CompilationError::invalid_annotation);
                            }
                        } else if ( pA->annotation->rtti_isStructureTypeAnnotation() ) {
                            if ( annL->size()!=1 ) {
                                das_yyerror(scanner,"structures are only allowed one structure macro", loc,
                                    CompilationError::invalid_annotation);
                            } else {
                                if ( !yyextra->g_Program->addStructureHandle(pStruct,
                                    static_pointer_cast<StructureTypeAnnotation>(pA->annotation), pA->arguments) ) {
                                        das_yyerror(scanner,"handled structure is already defined "+pStruct->name, loc,
                                        CompilationError::structure_already_declared);
                                } else {
                                    pStruct->module->removeStructure(pStruct);
                                }
                            }
                        }
                    }
                }
                swap ( pStruct->annotations, *annL );
                for ( const auto & pA : *annL ) {
                    pStruct->annotations.push_back(pA);
                }
                delete annL;
            }
        }
        deleteVariableDeclarationList(list);
        yyextra->g_thisStructure = nullptr;
    }

    Enumeration * ast_addEmptyEnum ( yyscan_t scanner, string * name, const LineInfo & atName ) {
        das_checkName(scanner,*name,atName);
        auto pEnum = make_smart<Enumeration>(*name);
        delete name;
        pEnum->at = atName;
        if ( !yyextra->g_Program->addEnumeration(pEnum) ) {
            das_yyerror(scanner,"enumeration is already defined "+pEnum->name, atName,
                CompilationError::enumeration_already_declared);
            return pEnum.orphan();
        } else {
            return pEnum.get();
        }
    }

    void ast_enumDeclaration (  yyscan_t scanner, AnnotationList * annL, const LineInfo & atannL, bool pubE, Enumeration * pEnum, Enumeration * pE, Type ebt ) {
        pEnum->baseType = ebt;
        pEnum->isPrivate = !pubE;
        pEnum->list = das::move(pE->list);
        if ( annL ) {
            for ( auto pA : *annL ) {
                if ( pA->annotation ) {
                    if ( pA->annotation->rtti_isEnumerationAnnotation() ) {
                        auto ann = static_pointer_cast<EnumerationAnnotation>(pA->annotation);
                        string err;
                        if ( !ann->touch(pEnum, *yyextra->g_Program->thisModuleGroup, pA->arguments, err) ) {
                            das_yyerror(scanner,"macro [" +pA->annotation->name + "] failed to finalize the enumeration " + pEnum->name + "\n" + err, atannL,
                                CompilationError::invalid_annotation);
                        }
                    }
                }
            }
            swap ( pEnum->annotations, *annL );
            delete annL;
        }
        delete pE;
    }

    void ast_globalLetList (  yyscan_t scanner, bool kwd_let, bool glob_shar, bool pub_var, vector<VariableDeclaration*> * list ) {
        for ( auto pDecl : *list ) {
            if ( pDecl->pTypeDecl ) {
                for ( const auto & name_at : *pDecl->pNameList ) {
                    VariablePtr pVar = make_smart<Variable>();
                    pVar->name = name_at.name;
                    pVar->aka = name_at.aka;
                    if ( !name_at.aka.empty() ) {
                        das_yyerror(scanner,"global variable " + name_at.name + " can't have an aka",name_at.at,
                            CompilationError::invalid_aka);
                    }
                    pVar->at = name_at.at;
                    pVar->type = make_smart<TypeDecl>(*pDecl->pTypeDecl);
                    if ( pDecl->pInit ) {
                        pVar->init = pDecl->pInit->clone();
                        pVar->init_via_move = pDecl->init_via_move;
                        pVar->init_via_clone = pDecl->init_via_clone;
                    }
                    if ( kwd_let ) {
                        pVar->type->constant = true;
                    } else {
                        pVar->type->removeConstant = true;
                    }
                    pVar->global_shared = glob_shar;
                    pVar->private_variable = !pub_var;
                    if ( !yyextra->g_Program->addVariable(pVar) )
                        das_yyerror(scanner,"global variable is already declared " + name_at.name,name_at.at,
                            CompilationError::global_variable_already_declared);
                }
            }
        }
        deleteVariableDeclarationList(list);
    }

    void ast_globalLet ( yyscan_t scanner, bool kwd_let, bool glob_shar, bool pub_var,
        AnnotationArgumentList * ann, VariableDeclaration * decl ) {
        auto pDecl = decl;
        if ( pDecl->pTypeDecl ) {
            for ( const auto & name_at : *pDecl->pNameList ) {
                VariablePtr pVar = make_smart<Variable>();
                pVar->name = name_at.name;
                pVar->aka = name_at.aka;
                if ( !name_at.aka.empty() ) {
                    das_yyerror(scanner,"global variable " + name_at.name + " can't have an aka",name_at.at,
                        CompilationError::invalid_aka);
                }
                pVar->at = name_at.at;
                pVar->type = make_smart<TypeDecl>(*pDecl->pTypeDecl);
                if ( pDecl->pInit ) {
                    pVar->init = pDecl->pInit->clone();
                    pVar->init_via_move = pDecl->init_via_move;
                    pVar->init_via_clone = pDecl->init_via_clone;
                }
                if ( kwd_let ) {
                    pVar->type->constant = true;
                } else {
                    pVar->type->removeConstant = true;
                }
                pVar->global_shared = glob_shar;
                pVar->private_variable = !pub_var;
                if ( ann ) {
                    // note: we can do this because the annotation syntax is for single variable only
                    pVar->annotation = das::move(*ann);
                    delete ann;
                    ann = nullptr;
                }
                if ( !yyextra->g_Program->addVariable(pVar) )
                    das_yyerror(scanner,"global variable is already declared " + name_at.name,name_at.at,
                        CompilationError::global_variable_already_declared);
            }
        }
        delete pDecl;
    }

    bool isOpName ( const string & name ) {
        return !(isalpha(name[0]) || name[0]=='_' || name[0]=='`');
    }

    vector<VariableDeclaration*> * ast_structVarDefAbstract ( yyscan_t scanner, vector<VariableDeclaration*> * list,
        AnnotationList * annL, bool isPrivate, bool cnst, Function * func ) {
        if ( yyextra->g_Program->policies.no_members_functions_in_struct && !yyextra->g_thisStructure->isClass ) {
            das_yyerror(scanner,"structure can't have a member function",
                func->at, CompilationError::invalid_member_function);
        } else if ( func->isGeneric() ) {
            das_yyerror(scanner,"generic function can't be a member of a class " + func->getMangledName(),
                func->at, CompilationError::invalid_member_function);
        } else if ( func->name==yyextra->g_thisStructure->name || func->name=="finalize" ) {
            das_yyerror(scanner,"initializers and finalizers can't be abstract " + func->getMangledName(),
                func->at, CompilationError::invalid_member_function);
        } else if ( annL!=nullptr ) {
            das_yyerror(scanner,"abstract functions can't have annotations " + func->getMangledName(),
                func->at, CompilationError::invalid_member_function);
            delete annL;
        } else if ( func->result->baseType==Type::autoinfer ) {
            das_yyerror(scanner,"abstract functions must specify return type explicitly " + func->getMangledName(),
                func->at, CompilationError::invalid_member_function);
        } else if ( isOpName(func->name) ) {
            das_yyerror(scanner,"abstract functions can't be operators " + func->getMangledName(),
                func->at, CompilationError::invalid_member_function);
        } else {
            auto varName = func->name;
            func->name = yyextra->g_thisStructure->name + "`" + func->name;
            auto vars = new vector<VariableNameAndPosition>();
            vars->emplace_back(VariableNameAndPosition{varName,"",func->at});
            TypeDecl * funcType = new TypeDecl(Type::tFunction);
            funcType->at = func->at;
            swap ( funcType->firstType, func->result );
            funcType->argTypes.reserve ( func->arguments.size() );
            if ( yyextra->g_thisStructure->isClass ) {
                auto selfType = make_smart<TypeDecl>(yyextra->g_thisStructure);
                selfType->constant = cnst;
                funcType->argTypes.push_back(selfType);
                funcType->argNames.push_back("self");
            }
            for ( auto & arg : func->arguments ) {
                funcType->argTypes.push_back(arg->type);
                funcType->argNames.push_back(arg->name);
            }
            VariableDeclaration * decl = new VariableDeclaration(
                vars,
                funcType,
                nullptr
            );
            decl->isPrivate = isPrivate;
            list->push_back(decl);
        }
        func->delRef();
        return list;
    }

    void implAddGenericFunction ( yyscan_t scanner, Function * func ) {
        for ( auto & arg : func->arguments ) {
            vector<MatchingOptionError> optionErrors;
            findMatchingOptions(arg->type, optionErrors);
            if ( !optionErrors.empty() ) {
                for ( auto & opt : optionErrors ) {
                    // opt.type has matching options opt.option1 and opt.option2
                    das_yyerror(scanner,"generic function argument " + arg->name + " of type " + opt.optionType->describe()
                    + " has matching options " + opt.option1->describe() + " and " + opt.option2->describe(),
                        opt.optionType->at, CompilationError::invalid_type);
                }
            }
        }
        if ( !yyextra->g_Program->addGeneric(func) ) {
            das_yyerror(scanner,"generic function is already defined " + func->getMangledName(),
                func->at, CompilationError::function_already_declared);
        }
    }

    vector<VariableDeclaration*> * ast_structVarDef ( yyscan_t scanner, vector<VariableDeclaration*> * list,
        AnnotationList * annL, bool isStatic, bool isPrivate, int ovr, bool cnst, Function * func, Expression * block,
            const LineInfo & fromBlock, const LineInfo & annLAt ) {
        func->atDecl = fromBlock;
        func->body = block;
        auto isGeneric = func->isGeneric();
        if ( !yyextra->g_thisStructure ) {
            das_yyerror(scanner,"internal error or invalid macro. member function is declared outside of a class",
                func->at, CompilationError::invalid_member_function);
        } else if ( yyextra->g_Program->policies.no_members_functions_in_struct && !yyextra->g_thisStructure->isClass ) {
            das_yyerror(scanner,"structure can't have a member function",
                func->at, CompilationError::invalid_member_function);
        } else if ( isGeneric && !isStatic ) {
            das_yyerror(scanner,"generic function can't be a member of a class " + func->getMangledName(),
                func->at, CompilationError::invalid_member_function);
        } else if ( isOpName(func->name) ) {
            if ( isStatic ) {
                das_yyerror(scanner,"operator can't be static " + func->getMangledName(),
                    func->at, CompilationError::invalid_member_function);
            }
            if ( ovr ) {
                das_yyerror(scanner,"can't override an operator " + func->getMangledName(),
                    func->at, CompilationError::invalid_member_function);
            }
            modifyToClassMember(func, yyextra->g_thisStructure, false, cnst);
            assignDefaultArguments(func);
            runFunctionAnnotations(scanner, nullptr, func, annL, annLAt);
            if ( !yyextra->g_Program->addFunction(func) ) {
                das_yyerror(scanner,"function is already defined " + func->getMangledName(),
                    func->at, CompilationError::function_already_declared);
            }
            func->delRef();
        } else {
            func->privateFunction = yyextra->g_thisStructure->privateStructure;
            if ( func->name != yyextra->g_thisStructure->name && func->name != "finalize") {
                if ( isStatic ) {
                    func->name = yyextra->g_thisStructure->name + "`" + func->name;
                    func->isClassMethod = true;
                    func->isStaticClassMethod = true;
                    func->classParent = yyextra->g_thisStructure;
                    func->privateFunction = isPrivate || yyextra->g_thisStructure->privateStructure;
                } else {
                    auto varName = func->name;
                    func->name = yyextra->g_thisStructure->name + "`" + func->name;
                    auto vars = new vector<VariableNameAndPosition>();
                    vars->emplace_back(VariableNameAndPosition{varName,"",func->at});
                    Expression * finit = new ExprAddr(func->at, inThisModule(func->name));
                    if ( ovr == OVERRIDE_OVERRIDE ) {
                        finit = new ExprCast(func->at, finit, make_smart<TypeDecl>(Type::autoinfer));
                    }
                    VariableDeclaration * decl = new VariableDeclaration(
                        vars,
                        new TypeDecl(Type::autoinfer),
                        finit
                    );
                    decl->override = ovr == OVERRIDE_OVERRIDE;
                    decl->sealed = ovr == OVERRIDE_SEALED;
                    decl->isPrivate = isPrivate;
                    list->push_back(decl);
                    modifyToClassMember(func, yyextra->g_thisStructure, false, cnst);
                }
            } else {
                if ( isStatic ) {
                    das_yyerror(scanner,"initializer or a finalizer can't be static " + func->getMangledName(),
                        func->at, CompilationError::invalid_member_function);
                }
                if ( ovr ) {
                    das_yyerror(scanner,"can't override an initializer or a finalizer " + func->getMangledName(),
                        func->at, CompilationError::invalid_member_function);
                }
                if ( cnst ) {
                    das_yyerror(scanner,"can't have a constant initializer or a finalizer " + func->getMangledName(),
                        func->at, CompilationError::invalid_member_function);
                }
                if ( func->name!="finalize" ) {
                    auto ctr = makeClassConstructor(yyextra->g_thisStructure, func);
                    if ( !yyextra->g_Program->addFunction(ctr) ) {
                        das_yyerror(scanner,"intializer is already defined " + ctr->getMangledName(),
                            ctr->at, CompilationError::function_already_declared);
                    }
                    func->name = yyextra->g_thisStructure->name + "`" + yyextra->g_thisStructure->name;
                    modifyToClassMember(func, yyextra->g_thisStructure, false, false);
                    func->arguments[0]->no_capture = true;  // we can't capture self in the class c-tor
                } else {
                    modifyToClassMember(func, yyextra->g_thisStructure, true, false);
                }
            }
            assignDefaultArguments(func);
            if ( isGeneric ) {
                implAddGenericFunction(scanner, func);
            } else {
                runFunctionAnnotations(scanner, nullptr, func, annL, annLAt);
                if ( !yyextra->g_Program->addFunction(func) ) {
                    das_yyerror(scanner,"function is already defined " + func->getMangledName(),
                        func->at, CompilationError::function_already_declared);
                }
            }
            func->delRef();
        }
        return list;
    }

    Expression * ast_NameName ( yyscan_t scanner, string * ena, string * eni, const LineInfo & enaAt, const LineInfo & eniAt ) {
        Enumeration * pEnum = nullptr;
        Expression * resConst = nullptr;
        auto enums = yyextra->g_Program->findEnum(*ena);
        auto aliases = yyextra->g_Program->findAlias(*ena);
        if ( enums.size()+aliases.size() > 1 ) {
            string candidates;
            if ( enums.size() ) candidates += yyextra->g_Program->describeCandidates(enums);
            if ( aliases.size() ) candidates += yyextra->g_Program->describeCandidates(aliases);
            das_yyerror(scanner,"too many options for the " + *ena + "\n" + candidates, enaAt,
                CompilationError::type_not_found);
        } else if ( enums.size()==0 && aliases.size()==0 ) {
            das_yyerror(scanner,"enumeration or bitfield not found " + *ena, enaAt,
                CompilationError::type_not_found);
        } else if ( enums.size()==1 ) {
            pEnum = enums.back().get();
        } else if ( aliases.size()==1 ) {
            auto alias = aliases.back();
            if ( alias->isEnum() ) {
                pEnum = alias->enumType;
            } else if ( alias->isBitfield() ) {
                int bit = alias->findArgumentIndex(*eni);
                if ( bit!=-1 ) {
                    auto td = make_smart<TypeDecl>(*alias);
                    td->ref = false;
                    auto bitConst = new ExprConstBitfield(eniAt, 1u << bit);
                    bitConst->bitfieldType = make_smart<TypeDecl>(*alias);
                    resConst = bitConst;
                } else {
                    das_yyerror(scanner,"enumeration or bitfield not found " + *ena, enaAt,
                        CompilationError::bitfield_not_found);
                }
            } else {
                das_yyerror(scanner,"expecting enumeration or bitfield " + *ena, enaAt,
                    CompilationError::syntax_error);
            }
        }
        if ( pEnum ) {
            auto td = make_smart<TypeDecl>(pEnum);
            resConst = new ExprConstEnumeration(eniAt, *eni, td);
        }
        delete ena;
        delete eni;
        if ( resConst ) {
            return resConst;
        } else {
            return new ExprConstInt(0);   // dummy
        }
    }


    Expression * ast_makeBlock ( yyscan_t scanner, int bal, AnnotationList * annL, vector<CaptureEntry> * clist,
        vector<VariableDeclaration*> * list, TypeDecl * result, Expression * block, const LineInfo & blockAt, const LineInfo & annLAt ) {
        auto mkb = new ExprMakeBlock(blockAt,ExpressionPtr(block), bal==1, bal==2);
        ExprBlock * closure = (ExprBlock *) block;
        closure->returnType = TypeDeclPtr(result);
        if ( list ) {
            for ( auto pDecl : *list ) {
                if ( pDecl->pTypeDecl ) {
                    for ( const auto & name_at : *pDecl->pNameList ) {
                        if ( !closure->findArgument(name_at.name) ) {
                            VariablePtr pVar = make_smart<Variable>();
                            pVar->name = name_at.name;
                            pVar->aka = name_at.aka;
                            pVar->at = name_at.at;
                            pVar->type = make_smart<TypeDecl>(*pDecl->pTypeDecl);
                            if ( pDecl->pInit ) {
                                pVar->init = ExpressionPtr(pDecl->pInit->clone());
                                pVar->init_via_move = pDecl->init_via_move;
                                pVar->init_via_clone = pDecl->init_via_clone;
                            }
                            if ( pDecl->annotation ) {
                                pVar->annotation = *pDecl->annotation;
                            }
                            if ( auto pTagExpr = name_at.tag ) {
                                pVar->tag = true;
                                pVar->source = pTagExpr;
                            }
                            closure->arguments.push_back(pVar);
                        } else {
                            das_yyerror(scanner,"block argument is already declared " + name_at.name,
                                name_at.at,CompilationError::argument_already_declared);
                        }
                    }
                }
            }
            deleteVariableDeclarationList(list);
        }
        if ( clist ) {
            swap ( mkb->capture, *clist );
            delete clist;
            if ( bal != 1 ) {   // if its not lambda, can't capture
                das_yyerror(scanner,"can only have capture section for the lambda",
                    mkb->at,CompilationError::invalid_capture);
            }
        }
        if ( annL ) {
            for ( auto pA : *annL ) {
                if ( pA->annotation ) {
                    if ( pA->annotation->rtti_isFunctionAnnotation() ) {
                        auto ann = static_pointer_cast<FunctionAnnotation>(pA->annotation);
                        string err;
                        if ( !ann->apply(closure, *yyextra->g_Program->thisModuleGroup, pA->arguments, err) ) {
                            das_yyerror(scanner,"macro [" +pA->annotation->name + "] failed to apply to the block\n" + err, annLAt,
                                CompilationError::invalid_annotation);
                        }
                    } else {
                        das_yyerror(scanner,"blocks are only allowed function macros", annLAt,
                            CompilationError::invalid_annotation);
                    }
                }
            }
            swap ( closure->annotations, *annL );
            for ( const auto & pA : *annL ) {
                closure->annotations.push_back(pA);
            }
            delete annL;
        }
        return mkb;
    }

    Expression * ast_Let ( yyscan_t scanner, bool kwd_let, bool inScope, VariableDeclaration * decl, const LineInfo & kwd_letAt, const LineInfo & declAt ) {
        auto pLet = new ExprLet();
        pLet->at = kwd_letAt;
        pLet->atInit = declAt;
        pLet->inScope = inScope;
        pLet->isTupleExpansion = decl->isTupleExpansion;
        if ( decl->pTypeDecl ) {
            for ( const auto & name_at : *decl->pNameList ) {
                if ( !pLet->find(name_at.name) ) {
                    VariablePtr pVar = make_smart<Variable>();
                    pVar->name = name_at.name;
                    pVar->aka = name_at.aka;
                    pVar->at = name_at.at;
                    pVar->type = make_smart<TypeDecl>(*decl->pTypeDecl);
                    if ( decl->pInit ) {
                        pVar->init = decl->pInit->clone();
                        pVar->init_via_move = decl->init_via_move;
                        pVar->init_via_clone = decl->init_via_clone;
                    }
                    if ( kwd_let ) {
                        pVar->type->constant = true;
                    } else {
                        pVar->type->removeConstant = true;
                    }
                    pLet->variables.push_back(pVar);
                } else {
                    das_yyerror(scanner,"local variable is already declared " + name_at.name,name_at.at,
                        CompilationError::local_variable_already_declared);
                }
            }
        }
        if ( auto pTagExpr = decl->pNameList->front().tag ) {
            auto pTag = new ExprTag(declAt, pTagExpr, ExpressionPtr(pLet), "i");
            delete decl;
            return pTag;
        } else {
            delete decl;
            return pLet;
        }
    }

    Function * ast_functionDeclarationHeader ( yyscan_t scanner, string * name, vector<VariableDeclaration*> * list,
        TypeDecl * result, const LineInfo & nameAt ) {
        auto pFunction = make_smart<Function>();
        pFunction->at = nameAt;
        pFunction->name = *name;
        pFunction->result = TypeDeclPtr(result);
        if ( list ) {
            for ( auto pDecl : *list ) {
                if ( pDecl->pTypeDecl ) {
                    for ( const auto & name_at : *pDecl->pNameList ) {
                        if ( !pFunction->findArgument(name_at.name) ) {
                            VariablePtr pVar = make_smart<Variable>();
                            pVar->name = name_at.name;
                            pVar->aka = name_at.aka;
                            pVar->at = name_at.at;
                            pVar->type = make_smart<TypeDecl>(*pDecl->pTypeDecl);
                            if ( pDecl->pInit ) {
                                pVar->init = ExpressionPtr(pDecl->pInit->clone());
                                pVar->init_via_move = pDecl->init_via_move;
                                pVar->init_via_clone = pDecl->init_via_clone;
                            }
                            if ( pDecl->annotation ) {
                                pVar->annotation = *pDecl->annotation;
                            }
                            pFunction->arguments.push_back(pVar);
                        } else {
                            das_yyerror(scanner,"function argument is already declared " + name_at.name,name_at.at,
                                CompilationError::argument_already_declared);
                        }
                    }
                }
            }
            deleteVariableDeclarationList(list);
        }
        delete name;
        return pFunction.orphan();
    }

    void das_collect_all_keywords ( Module * mod, yyscan_t scanner ) {
        das_collect_keywords(mod,scanner);
        for ( auto it : mod->requireModule ) {
            if ( it.second ) {
                das_collect_keywords(it.first,scanner);
            }
        }
    }

    void ast_requireModule ( yyscan_t scanner, string * name, string * modalias, bool pub, const LineInfo & atName ) {
        auto info = yyextra->g_Access->getModuleInfo(*name, yyextra->g_FileAccessStack.back()->name);
        if ( auto mod = yyextra->g_Program->addModule(info.moduleName) ) {
            yyextra->g_Program->allRequireDecl.push_back(make_tuple(mod,*name,info.fileName,pub,atName));
            yyextra->g_Program->thisModule->addDependency(mod, pub);
            das_collect_all_keywords(mod,scanner);
            if ( !info.importName.empty() ) {
                auto malias = modalias ? *modalias : info.importName;
                auto ita = yyextra->das_module_alias.find(malias);
                if ( ita !=yyextra->das_module_alias.end() ) {
                    if ( ita->second != info.moduleName ) {
                        das_yyerror(scanner,"module alias already used " + malias + " as " + ita->second,atName,
                            CompilationError::module_not_found);
                    }
                } else {
                    yyextra->das_module_alias[malias] = info.moduleName;
                }
            }
        } else {
            yyextra->g_Program->allRequireDecl.push_back(make_tuple((Module *)nullptr,*name,info.fileName,pub,atName));
            das_yyerror(scanner,"required module not found " + *name,atName,
                CompilationError::module_not_found);
        }
        delete name;
        if ( modalias) delete modalias;
    }

    Expression * ast_forLoop ( yyscan_t,  vector<VariableNameAndPosition> * iters, Expression * srcs,
        Expression * block, const LineInfo & locAt, const LineInfo & blockAt ) {
        auto pFor = new ExprFor(locAt);
        pFor->visibility = blockAt;
        for ( const auto & np : *iters ) {
            pFor->iterators.push_back(np.name);
            pFor->iteratorsAka.push_back(np.aka);
            pFor->iteratorsAt.push_back(np.at);
            pFor->iteratorsTags.push_back(np.tag);
        }
        delete iters;
        pFor->sources = sequenceToList(srcs);
        pFor->body = ExpressionPtr(block);
        ((ExprBlock *)block)->inTheLoop = true;
        return pFor;
    }

    AnnotationArgumentList * ast_annotationArgumentListEntry ( yyscan_t, AnnotationArgumentList * argL, AnnotationArgument * arg ) {
        if ( arg->type==Type::none ) {
            for ( auto & sarg : *(arg->aList) ) {
                sarg.name = arg->name;
                sarg.at = arg->at;
                argL->push_back(sarg);
            }
            delete arg->aList;
        } else {
            argL->push_back(*arg);
        }
        delete arg;
        return argL;
    }

    Expression * ast_lpipe ( yyscan_t scanner, Expression * fncall, Expression * arg, const LineInfo & locAt ) {
        if ( fncall->rtti_isCallLikeExpr() ) {
            auto pCall = (ExprLooksLikeCall *) fncall;
            pCall->arguments.push_back(ExpressionPtr(arg));
            return fncall;
        } else if ( fncall->rtti_isVar() ) {
            auto pVar = (ExprVar *) fncall;
            auto pCall = yyextra->g_Program->makeCall(pVar->at,pVar->name);
            delete pVar;
            pCall->arguments.push_back(ExpressionPtr(arg));
            return pCall;
        } else {
            das_yyerror(scanner,"can only lpipe into a function call",locAt,CompilationError::cant_pipe);
            return fncall;
        }
    }

    Expression * ast_rpipe ( yyscan_t scanner, Expression * arg, Expression * fncall, const LineInfo & locAt ) {
        if ( fncall->rtti_isCallLikeExpr() ) {
            auto pCall = (ExprLooksLikeCall *) fncall;
            pCall->arguments.insert(pCall->arguments.begin(),ExpressionPtr(arg));
            return fncall;
        } else if ( fncall->rtti_isVar() ) {
            auto pVar = (ExprVar *) fncall;
            auto pCall = yyextra->g_Program->makeCall(pVar->at,pVar->name);
            delete pVar;
            pCall->arguments.insert(pCall->arguments.begin(),ExpressionPtr(arg));
            return pCall;
        } else if (fncall->rtti_isNamedCall()) {
            auto pCall = (ExprNamedCall*)fncall;
            pCall->nonNamedArguments.insert(pCall->nonNamedArguments.begin(), ExpressionPtr(arg));
            return fncall;
        } else if (fncall->rtti_isField() ) {
            auto pField = (ExprField*)fncall;
            if ( auto pipeto = ast_rpipe(scanner, arg, pField->value.get(), locAt) ) {
                return pField;
            } else {
                return nullptr;
            }
        } else if (fncall->rtti_isSafeField() ) {
            auto pField = (ExprSafeField*)fncall;
            if ( auto pipeto = ast_rpipe(scanner, arg, pField->value.get(), locAt) ) {
                return pField;
            } else {
                return nullptr;
            }
        } else {
            das_yyerror(scanner,"can only rpipe into a function call",locAt,CompilationError::cant_pipe);
            return fncall;
        }
    }

    Expression * ast_makeGenerator ( yyscan_t, TypeDecl * typeDecl, vector<CaptureEntry> * clist, Expression * subexpr, const LineInfo & locAt ) {
        auto gen = new ExprMakeGenerator(locAt, subexpr);
        gen->iterType = TypeDeclPtr(typeDecl);
        if ( clist ) {
            swap ( gen->capture, *clist );
            delete clist;
        }
        return gen;
    }

    ExprBlock * ast_wrapInBlock ( Expression * expr ) {
        auto block = new ExprBlock();
        block->at = expr->at;
        block->list.push_back(ExpressionPtr(expr));
        return block;
    }

    int skip_underscode ( char * tok, char * buf, char * bufend ) {
        char * out = buf;
        for ( ;; ) {
            char ch = *tok ++;
            if ( ch==0 ) break;
            if ( ch=='_' ) continue;
            *out++ = ch;
            if ( out==bufend ) { out--; break; }
        }
        *out = 0;
        return int(out - buf);
    }

    Expression * ast_makeStructToMakeVariant ( MakeStruct * decl, const LineInfo & locAt ) {
        auto mks = new ExprMakeStruct(locAt);
        for ( auto & f : *decl ) {
            auto fld = new MakeStruct();
            fld->emplace_back(f);
            mks->structs.push_back(fld);
        }
        delete decl;
        return mks;
    }

 }