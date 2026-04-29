#include "daScript/misc/platform.h"
#include "daScript/ast/ast.h"
#include "daScript/ast/ast_generate.h"
#include "daScript/ast/ast_infer_type.h"
#include "daScript/ast/ast_visitor.h"

namespace das {

    bool InferTypes::finished() const { return !needRestart; }
    bool InferTypes::canVisitGlobalVariable ( Variable * ) { return !fatalAliasLoop; }
    bool InferTypes::canVisitEnumeration ( Enumeration * ) { return !fatalAliasLoop; }

    string InferTypes::generateNewLambdaName(const LineInfo &at) {
        string mod = thisModule->name;
        if (mod.empty())
            mod = "thismodule";
        return "_lambda_" + mod + "_" + to_string(at.line) + "_" + to_string(program->newLambdaIndex++);
    }
    string InferTypes::generateNewLocalFunctionName(const LineInfo &at) {
        string mod = thisModule->name;
        if (mod.empty())
            mod = "thismodule";
        return "_localfunction_" + mod + "_" + to_string(at.line) + "_" + to_string(program->newLambdaIndex++);
    }
    void InferTypes::pushVarStack() {
        varStack.push_back(local.size());
        assumeStack.push_back(assume.size());
        assumeTypeStack.push_back(assumeType.size());
    }
    void InferTypes::popVarStack() {
        assume.resize(assumeStack.back());
        assumeStack.pop_back();
        assumeType.resize(assumeTypeStack.back());
        assumeTypeStack.pop_back();
        local.resize(varStack.back());
        varStack.pop_back();
    }
    void InferTypes::error(const string &err, const string &extra, const string &fixme, const LineInfo &at, CompilationError cerr) const {
        if (verbose && func) {
            string ex = extra;
            if (!extra.empty())
                ex += "\n";
            ex += func->getLocationExtra();
            program->error(err, ex, fixme, at, cerr);
        } else {
            program->error(err, extra, fixme, at, cerr);
        }
    }
    void InferTypes::reportAstChanged() {
        needRestart = true;
        if (func)
            func->notInferred();
    }
    void InferTypes::reportFolding() {
        FoldingVisitor::reportFolding();
        needRestart = true;
        if (func)
            func->notInferred();
    }
    string InferTypes::describeType(const TypeDeclPtr &decl) const {
        return verbose ? decl->describe() : "";
    }
    string InferTypes::describeType(const TypeDecl *decl) const {
        return verbose ? decl->describe() : "";
    }
    string InferTypes::describeFunction(const FunctionPtr &fun) const {
        return verbose ? fun->describe() : "";
    }
    string InferTypes::describeFunction(const Function *fun) const {
        return verbose ? fun->describe() : "";
    }
    void InferTypes::verifyType(const TypeDeclPtr &decl, bool allowExplicit, bool classMethod) const {
        // TODO: enable and cleanup
        if (decl->isExplicit && !allowExplicit) {
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
        for (auto di : decl->dim) {
            if (di <= 0) {
                error("array dimension can't be 0 or less: '" + describeType(decl) + "'", "", "",
                      decl->at, CompilationError::invalid_array_dimension);
            }
            size *= di;
            if (size > 0x7fffffff) {
                error("array is too big: '" + describeType(decl) + "'", "", "",
                      decl->at, CompilationError::invalid_array_dimension);
                break;
            }
        }
        if (decl->baseType == Type::tFunction || decl->baseType == Type::tLambda || decl->baseType == Type::tBlock || decl->baseType == Type::tVariant ||
            decl->baseType == Type::tTuple) {
            if (decl->argNames.size() && decl->argNames.size() != decl->argTypes.size()) {
                string allNames = "\t";
                for (const auto &na : decl->argNames) {
                    allNames += na + " ";
                }
                error("malformed type: '" + describeType(decl) + "'", allNames, "",
                      decl->at, CompilationError::invalid_type);
            }
        }
        if (decl->baseType == Type::tVoid) {
            if (decl->dim.size()) {
                error("can't declare an array of void: '" + describeType(decl) + "'", "", "",
                      decl->at, CompilationError::invalid_type);
            }
            if (decl->ref) {
                error("can't declare a void reference: '" + describeType(decl) + "'", "", "",
                      decl->at, CompilationError::invalid_type);
            }
        } else if (decl->baseType == Type::tPointer) {
            if (auto ptrType = decl->firstType) {
                if (ptrType->ref) {
                    error("can't declare a pointer to a reference: '" + describeType(decl) + "'", "", "",
                          ptrType->at, CompilationError::invalid_type);
                }
                if (decl->smartPtr) {
                    if (ptrType->baseType != Type::tHandle) {
                        error("can't declare a smart pointer to anything other than annotation: '" + describeType(decl) + "'", "", "",
                              ptrType->at, CompilationError::invalid_type);
                    } else if (!ptrType->annotation->isSmart()) {
                        error("this annotation does not support smart pointers: '" + describeType(decl) + "'", "", "",
                              ptrType->at, CompilationError::invalid_type);
                    }
                }
                verifyType(ptrType);
            } else {
                if (decl->smartPtr) {
                    error("can't declare a void smart pointer", "", "",
                          decl->at, CompilationError::invalid_type);
                }
            }
        } else if (decl->baseType == Type::tIterator) {
            if (auto ptrType = decl->firstType) {
                verifyType(ptrType);
            }
        } else if (decl->baseType == Type::tArray) {
            if (auto arrayType = decl->firstType) {
                if (arrayType->isAutoOrAlias()) {
                    error("array type is not fully resolved: '" + describeType(arrayType) + "'", "", "",
                          arrayType->at, CompilationError::invalid_array_type);
                }
                if (arrayType->ref) {
                    error("can't declare an array of references: '" + describeType(arrayType) + "'", "", "",
                          arrayType->at, CompilationError::invalid_array_type);
                }
                if (arrayType->baseType == Type::tVoid) {
                    error("can't declare a void array: '" + describeType(arrayType) + "'", "", "",
                          arrayType->at, CompilationError::invalid_array_type);
                }
                if (!arrayType->canBePlacedInContainer()) {
                    error("can't have array of non-trivial type: '" + describeType(arrayType) + "'", "", "",
                          arrayType->at, CompilationError::invalid_type);
                }
                verifyType(arrayType);
            }
        } else if (decl->baseType == Type::tTable) {
            if (auto keyType = decl->firstType) {
                if (keyType->isAutoOrAlias()) {
                    error("table key is not fully resolved: '" + describeType(keyType) + "'", "", "",
                          keyType->at, CompilationError::invalid_array_type);
                }
                if (keyType->ref) {
                    error("table key can't be declared as a reference: '" + describeType(keyType) + "'", "", "",
                          keyType->at, CompilationError::invalid_table_type);
                }
                if (!(keyType->isWorkhorseType() || (keyType->baseType == Type::tHandle && !keyType->annotation->isRefType()))) {
                    error("table key has to be declare as a basic 'hashable' type: '" + describeType(keyType) + "'", "", "",
                          keyType->at, CompilationError::invalid_table_type);
                }
                verifyType(keyType);
            }
            if (auto valueType = decl->secondType) {
                if (valueType->isAutoOrAlias()) {
                    error("table value is not fully resolved: '" + describeType(valueType) + "'", "", "",
                          valueType->at, CompilationError::invalid_array_type);
                }
                if (valueType->ref) {
                    error("table value can't be declared as a reference: '" + describeType(valueType) + "'", "", "",
                          valueType->at, CompilationError::invalid_table_type);
                }
                if (!valueType->canBePlacedInContainer()) {
                    error("can't have table value of non-trivial type: '" + describeType(valueType) + "'", "", "",
                          valueType->at, CompilationError::invalid_type);
                }
                verifyType(valueType);
            }
        } else if (decl->baseType == Type::tBlock || decl->baseType == Type::tFunction || decl->baseType == Type::tLambda) {
            if (auto resultType = decl->firstType) {
                if (!resultType->isReturnType()) {
                    error("not a valid return type: '" + describeType(resultType) + "'", "", "",
                          resultType->at, CompilationError::invalid_return_type);
                }
                verifyType(resultType);
            }
            for (auto &argType : decl->argTypes) {
                if (!classMethod && (argType->ref && argType->isRefType())) {
                    error("can't pass a boxed type by a reference: '" + describeType(argType) + "'", "", "",
                          argType->at, CompilationError::invalid_argument_type);
                }
                verifyType(argType, true);
            }
        } else if (decl->baseType == Type::tTuple) {
            for (auto &argType : decl->argTypes) {
                if (argType->ref) {
                    error("tuple element can't be a reference: '" + describeType(argType) + "'", "", "",
                          argType->at, CompilationError::invalid_type);
                }
                if (argType->isVoid()) {
                    error("tuple element can't be void", "", "",
                          argType->at, CompilationError::invalid_type);
                }
                if (!argType->canBePlacedInContainer()) {
                    error("invalid tuple element type: '" + describeType(argType) + "'", "", "",
                          argType->at, CompilationError::invalid_type);
                }
                verifyType(argType);
            }
        } else if (decl->baseType == Type::tVariant) {
            for (auto &argType : decl->argTypes) {
                if (argType->ref) {
                    error("variant element can't be a reference: '" + describeType(argType) + "'", "", "",
                          argType->at, CompilationError::invalid_type);
                }
                if (argType->isVoid()) {
                    error("variant element can't be void", "", "",
                          argType->at, CompilationError::invalid_type);
                }
                if (!argType->canBePlacedInContainer()) {
                    error("invalid variant element type: '" + describeType(argType) + "'", "", "",
                          argType->at, CompilationError::invalid_type);
                }
                verifyType(argType);
            }
        }
    }
    bool InferTypes::jitEnabled() const {
        return program->policies.jit_enabled && (!func || !func->requestNoJit);
    }
    void InferTypes::propagateTempType(const TypeDeclPtr &parentType, TypeDeclPtr &subexprType) {
        if (subexprType->isTempType()) {
            if (parentType->temporary)
                subexprType->temporary = true; // array<int?># -> int?#
        } else {
            subexprType->temporary = false; // array<int#> -> int
        }
    }
    TypeDeclPtr InferTypes::findFuncAlias(const FunctionPtr &fptr, const string &name) const {
        for (auto &arg : fptr->arguments) {
            if (auto aT = arg->type->findAlias(name, true)) {
                return aT;
            }
        }
        if (auto rT = fptr->result->findAlias(name, true)) {
            return rT;
        }
        TypeDeclPtr rT;
        thisModule->globals.find_first([&](auto gvar) {
                if ( auto vT = gvar->type->findAlias(name,false) ) {
                    rT = vT;
                    return true;
                }
                return false; });
        if (rT)
            return rT;
        TypeDeclPtr mtd = program->makeTypeDeclaration(LineInfo(), name);
        return (!mtd || mtd->isAlias()) ? nullptr : mtd;
    }
    TypeDeclPtr InferTypes::findAlias(const string &name) const {
        if (func) {
            for (auto &ast : assumeType) {
                if (ast->alias == name) {
                    return ast->assumeType;
                }
            }
            for (auto it = local.rbegin(), its = local.rend(); it != its; ++it) {
                auto &var = *it;
                if (auto vT = var->type->findAlias(name)) {
                    return vT;
                }
            }
            for (auto &arg : func->arguments) {
                if (auto aT = arg->type->findAlias(name)) {
                    return aT;
                }
            }
            if (auto rT = func->result->findAlias(name, true)) {
                return rT;
            }
        }
        Structure *aliasStruct = currentStructure ? currentStructure : (func ? func->classParent : nullptr);
        if (aliasStruct) {
            if (auto cT = aliasStruct->findAlias(name)) {
                return cT;
            }
        }
        TypeDeclPtr rT;
        thisModule->globals.find_first([&](auto gvar) {
                if ( auto vT = gvar->type->findAlias(name) ) {
                    rT = vT;
                    return true;
                }
                return false; });
        if (rT)
            return rT;
        TypeDeclPtr mtd = program->makeTypeDeclaration(LineInfo(), name);
        return (!mtd || mtd->isAlias()) ? nullptr : mtd;
    }

    bool InferTypes::isLoop(vector<string> & visited, const TypeDeclPtr &decl) const {
        if ( decl->baseType == Type::alias ) {
            if ( find(visited.begin(), visited.end(), decl->alias) != visited.end() ) {
                return true;
            }
            visited.push_back(decl->alias);
        }
        if ( decl->firstType ) {
            if ( isLoop(visited, decl->firstType) ) {
                return true;
            }
        }
        if ( decl->secondType ) {
            if ( isLoop(visited, decl->secondType) ) {
                return true;
            }
        }
        for ( auto & argType : decl->argTypes ) {
            if ( isLoop(visited, argType) ) {
                return true;
            }
        }
        if ( decl->baseType == Type::alias ) {
            visited.pop_back();
        }
        return false;
    }

    TypeDeclPtr InferTypes::inferAlias(const TypeDeclPtr &decl, const FunctionPtr &fptr, AliasMap *aliases, OptionsMap *options, bool autoToAlias) const {
        autoToAlias |= decl->autoToAlias;
        if (decl->baseType == Type::typeDecl || decl->baseType == Type::typeMacro) {
            return nullptr;
        }
        if (decl->baseType == Type::autoinfer && !autoToAlias) { // until alias is fully resolved, can't infer
            return nullptr;
        }
        if (decl->baseType == Type::alias || (decl->baseType == Type::autoinfer && autoToAlias)) {
            if (decl->isTag)
                return nullptr; // we can never infer a tag type
            TypeDeclPtr aT;
            if (aliases) {
                auto it = aliases->find(decl->alias);
                if (it != aliases->end()) {
                    aT = it->second.get();
                }
            }
            if (!aT) {
                aT = fptr ? findFuncAlias(fptr, decl->alias) : findAlias(decl->alias);
            }
            if (!aT) {
                auto bT = nameToBasicType(decl->alias);
                if (bT != Type::none) {
                    aT = make_smart<TypeDecl>(bT);
                }
            }
            if (aT) {
                auto resT = make_smart<TypeDecl>(*aT);
                resT->at = decl->at;
                resT->ref = (resT->ref || decl->ref) && !decl->removeRef;
                resT->constant = (resT->constant || decl->constant) && !decl->removeConstant;
                resT->temporary = (resT->temporary || decl->temporary) && !decl->removeTemporary;
                resT->dim = decl->dim;
                resT->aotAlias = false;
                resT->alias.clear();
                return resT;
            } else {
                return nullptr;
            }
        }
        auto resT = make_smart<TypeDecl>(*decl);
        if (decl->baseType == Type::tPointer) {
            if (decl->firstType) {
                resT->firstType = inferAlias(decl->firstType, fptr, aliases, options, autoToAlias);
                if (!resT->firstType)
                    return nullptr;
            }
        } else if (decl->baseType == Type::tIterator) {
            if (decl->firstType) {
                resT->firstType = inferAlias(decl->firstType, fptr, aliases, options, autoToAlias);
                if (!resT->firstType)
                    return nullptr;
            }
        } else if (decl->baseType == Type::tArray) {
            if (decl->firstType) {
                resT->firstType = inferAlias(decl->firstType, fptr, aliases, options, autoToAlias);
                if (!resT->firstType)
                    return nullptr;
            }
        } else if (decl->baseType == Type::tTable) {
            if (decl->firstType) {
                resT->firstType = inferAlias(decl->firstType, fptr, aliases, options, autoToAlias);
                if (!resT->firstType)
                    return nullptr;
            }
            if (decl->secondType) {
                resT->secondType = inferAlias(decl->secondType, fptr, aliases, options, autoToAlias);
                if (!resT->secondType)
                    return nullptr;
            }
        } else if (decl->baseType == Type::tFunction || decl->baseType == Type::tLambda || decl->baseType == Type::tBlock) {
            for (size_t iA = 0, iAs = decl->argTypes.size(); iA != iAs; ++iA) {
                auto &declAT = decl->argTypes[iA];
                if (auto infAT = inferAlias(declAT, fptr, aliases, options, autoToAlias)) {
                    resT->argTypes[iA] = infAT;
                } else {
                    return nullptr;
                }
            }
            if (!resT->firstType)
                return nullptr;
            resT->firstType = inferAlias(decl->firstType, fptr, aliases, options, autoToAlias);
            if (!resT->firstType)
                return nullptr;
        } else if (decl->baseType == Type::tVariant || decl->baseType == Type::tTuple || decl->baseType == Type::option) {
            for (size_t iA = 0, iAs = decl->argTypes.size(); iA != iAs; ++iA) {
                auto &declAT = decl->argTypes[iA];
                if (auto infAT = inferAlias(declAT, fptr, aliases, options, autoToAlias)) {
                    resT->argTypes[iA] = infAT;
                } else {
                    return nullptr;
                }
            }
        }
        return resT;
    }
    TypeDeclPtr InferTypes::inferPartialAliases(const TypeDeclPtr &decl, const TypeDeclPtr &passType, const FunctionPtr &fptr, AliasMap *aliases) const {
        if (decl->baseType == Type::typeDecl || decl->baseType == Type::typeMacro) {
            for (auto &de : decl->dimExpr) {
                if (de && de->rtti_isTypeDecl()) {
                    auto td = static_pointer_cast<ExprTypeDecl>(de);
                    // since we don't have passType in typeexpr(3), we pass what we have
                    td->typeexpr = inferPartialAliases(td->typeexpr, td->typeexpr, fptr, aliases);
                }
            }
            if (decl->baseType == Type::typeMacro) {
                auto tmn = decl->typeMacroName();
                auto tms = findTypeMacro(tmn);
                if (tms.size() == 0) {
                    return decl;
                } else if (tms.size() > 1) {
                    return decl;
                } else {
                    auto resType = tms[0]->visit(program, thisModule, decl, passType);
                    if (!resType) {
                        return decl;
                    }
                    TypeDecl::applyAutoContracts(resType, decl);
                    return resType;
                }
            }
            return decl;
        }
        if (decl->baseType == Type::autoinfer) {
            return decl;
        }
        if (decl->baseType == Type::alias) {
            TypeDeclPtr aT;
            if (aliases) {
                auto it = aliases->find(decl->alias);
                if (it != aliases->end()) {
                    aT = it->second.get();
                }
            }
            if (!aT) {
                aT = fptr ? findFuncAlias(fptr, decl->alias) : findAlias(decl->alias);
            }
            if (aT) {
                auto resT = make_smart<TypeDecl>(*aT);
                resT->at = decl->at;
                resT->ref = (resT->ref || decl->ref) && !decl->removeRef;
                resT->constant = (resT->constant || decl->constant) && !decl->removeConstant;
                resT->temporary = (resT->temporary || decl->temporary) && !decl->removeTemporary;
                resT->implicit = (resT->implicit || decl->implicit);
                resT->explicitConst = (resT->explicitConst || decl->explicitConst);
                resT->dim = decl->dim;
                resT->aotAlias = false;
                // resT->alias.clear(); // this may speed things up, but it breaks typemacro-based aliases
                return resT;
            } else {
                return decl;
            }
        }
        // if its an option, we go through each
        if (decl->baseType == Type::option) {
            auto resT = make_smart<TypeDecl>(*decl);
            for (size_t iA = 0, iAs = decl->argTypes.size(); iA != iAs; ++iA) {
                auto &declAT = decl->argTypes[iA];
                resT->argTypes[iA] = inferPartialAliases(declAT, passType, fptr, aliases);
            }
            return resT;
        }
        // now, if pass type don't match at all, we use decl as passType
        auto passT = decl->baseType == passType->baseType ? passType : decl;
        // if they don't match, it will not infer no matter what, so we early out
        auto resT = make_smart<TypeDecl>(*decl);
        if (decl->baseType == Type::tPointer) {
            if (decl->firstType && passT->firstType) {
                resT->firstType = inferPartialAliases(decl->firstType, passT->firstType, fptr, aliases);
            }
        } else if (decl->baseType == Type::tIterator) {
            if (decl->firstType) {
                resT->firstType = inferPartialAliases(decl->firstType, passT->firstType, fptr, aliases);
            }
        } else if (decl->baseType == Type::tArray) {
            if (decl->firstType) {
                resT->firstType = inferPartialAliases(decl->firstType, passT->firstType, fptr, aliases);
            }
        } else if (decl->baseType == Type::tTable) {
            resT->firstType = inferPartialAliases(decl->firstType, passT->firstType, fptr, aliases);
            resT->secondType = inferPartialAliases(decl->secondType, passT->secondType, fptr, aliases);
        } else if (decl->baseType == Type::tFunction || decl->baseType == Type::tLambda || decl->baseType == Type::tBlock) {
            if (decl->argTypes.size() == passT->argTypes.size()) {
                for (size_t iA = 0, iAs = decl->argTypes.size(); iA != iAs; ++iA) {
                    auto &declAT = decl->argTypes[iA];
                    auto &passAT = passT->argTypes[iA];
                    resT->argTypes[iA] = inferPartialAliases(declAT, passAT, fptr, aliases);
                }
                if (decl->firstType) {
                    resT->firstType = inferPartialAliases(decl->firstType, passT->firstType, fptr, aliases);
                }
            }
        } else if (decl->baseType == Type::tVariant || decl->baseType == Type::tTuple) {
            if (decl->argTypes.size() == passT->argTypes.size()) {
                for (size_t iA = 0, iAs = decl->argTypes.size(); iA != iAs; ++iA) {
                    auto &declAT = decl->argTypes[iA];
                    auto &passAT = passT->argTypes[iA];
                    resT->argTypes[iA] = inferPartialAliases(declAT, passAT, fptr, aliases);
                }
            }
        }
        return resT;
    }
    ExprWith *InferTypes::hasMatchingWith(const string &fieldName) const {
        for (auto it = with.rbegin(), its = with.rend(); it != its; ++it) {
            auto eW = *it;
            if (auto eWT = eW->with->type) {
                StructurePtr pSt;
                if (eWT->isStructure()) {
                    pSt = eWT->structType;
                } else if (eWT->isPointer() && eWT->firstType && eWT->firstType->isStructure()) {
                    pSt = eWT->firstType->structType;
                }
                if (pSt) {
                    if (pSt->fieldLookup.find(fieldName) != pSt->fieldLookup.end()) {
                        return eW;
                    }
                    if (pSt->hasStaticMembers) {
                        auto fname = pSt->name + "`" + fieldName;
                        if (auto pVar = pSt->module->findVariable(fname)) {
                            if (pVar->static_class_member) {
                                return eW;
                            }
                        }
                    }
                }
            }
        }
        return nullptr;
    }
    ExpressionPtr InferTypes::promoteToProperty(ExprVar *expr, const ExpressionPtr &right) {
        for (auto it = with.rbegin(), its = with.rend(); it != its; ++it) {
            auto eW = *it;
            if (auto eWT = eW->with->type) {
                StructurePtr pSt;
                if (eWT->isStructure()) {
                    pSt = eWT->structType;
                } else if (eWT->isPointer() && eWT->firstType && eWT->firstType->isStructure()) {
                    pSt = eWT->firstType->structType;
                }
                if (pSt) {
                    if (eWT->isPointer()) {
                        auto derefV = make_smart<ExprPtr2Ref>(expr->at, eW->with);
                        derefV->type = make_smart<TypeDecl>(*eWT->firstType);
                        TypeDecl::applyAutoContracts(derefV->type, eWT->firstType);
                        derefV->type->ref = true;
                        derefV->type->constant |= eWT->constant;
                        if (right) {
                            if (auto cloneSet = inferGenericOperator(".`" + expr->name + "`clone", expr->at, derefV, right))
                                return cloneSet;
                        } else {
                            if (auto opE = inferGenericOperator(".`" + expr->name, expr->at, derefV, nullptr))
                                return opE;
                            if (auto opE = inferGenericOperatorWithName(".", expr->at, derefV, expr->name))
                                return opE;
                        }
                    } else {
                        if (right) {
                            if (auto cloneSet = inferGenericOperator(".`" + expr->name + "`clone", expr->at, eW->with, right))
                                return cloneSet;
                        } else {
                            if (auto opE = inferGenericOperator(".`" + expr->name, expr->at, eW->with, nullptr))
                                return opE;
                            if (auto opE = inferGenericOperatorWithName(".", expr->at, eW->with, expr->name))
                                return opE;
                        }
                    }
                }
            }
        }
        return nullptr;
    }
    vector<TypeMacro *> InferTypes::findTypeMacro(const string &name) const {
        string moduleName, funcName;
        splitTypeName(name, moduleName, funcName);
        vector<TypeMacro *> result;
        auto inWhichModule = getSearchModule(moduleName);
        program->library.foreach ([&](Module *mod) -> bool {
                if ( inWhichModule->isVisibleDirectly(mod) ) {
                    auto it = mod->typeMacros.find(funcName);
                    if ( it != mod->typeMacros.end() ) {
                        result.push_back(it->second.get());
                    }
                }
                return true; }, moduleName);
        return result;
    }
    bool InferTypes::inferTypeExpr(TypeDeclPtr &type) {
        bool any = false;
        if (type->baseType != Type::typeDecl && type->baseType != Type::typeMacro) {
            for (size_t i = 0, is = type->dim.size(); i != is; ++i) {
                if (type->dim[i] == TypeDecl::dimConst) {
                    if (type->dimExpr[i]) {
                        if (auto constExpr = getConstExpr(type->dimExpr[i].get())) {
                            if (constExpr->type->isIndex()) {
                                auto cI = static_pointer_cast<ExprConstInt>(constExpr);
                                auto dI = cI->getValue();
                                if (dI > 0) {
                                    type->dim[i] = dI;
                                    any = true;
                                } else {
                                    error("array dimension can't be 0 or less", "", "",
                                          type->at, CompilationError::invalid_array_dimension);
                                }
                            } else {
                                error("array dimension must be int32 or uint32", "", "",
                                      type->at, CompilationError::invalid_array_dimension);
                            }
                        } else {
                            error("array dimension must be constant", "", "",
                                  type->at, CompilationError::invalid_array_dimension);
                        }
                    } else {
                        error("can't deduce array dimension", "", "",
                              type->at, CompilationError::invalid_array_dimension);
                    }
                }
            }
        } else if (type->baseType == Type::typeDecl) {
            if (type->dimExpr.size() != 1) {
                error("typeDecl must have exactly one dimension", "", "",
                      type->at, CompilationError::invalid_type);
            } else if (type->dimExpr[0]->type) {
                if (!type->dimExpr[0]->type->isAutoOrAlias()) {
                    auto resType = make_smart<TypeDecl>(*type->dimExpr[0]->type);
                    resType->ref = false;
                    TypeDecl::applyAutoContracts(resType, type);
                    type = resType;
                    return true;
                } else {
                    error("can't deduce typeDecl type", "", "",
                          type->at, CompilationError::invalid_type);
                }
            } else {
                error("can't deduce type", "", "",
                      type->at, CompilationError::invalid_type);
            }
        } else if (type->baseType == Type::typeMacro) {
            auto tmn = type->typeMacroName();
            auto tms = findTypeMacro(tmn);
            if (tms.size() == 0) {
                error("can't find typeMacro " + tmn, "", "",
                      type->at, CompilationError::invalid_type);
            } else if (tms.size() > 1) {
                error("too many typeMacro " + tmn + " found", "", "",
                      type->at, CompilationError::invalid_type);
            } else {
                auto resType = tms[0]->visit(program, thisModule, type, nullptr);
                if (!resType) {
                    error("can't deduce typeMacro " + tmn, "", "",
                          type->at, CompilationError::invalid_type);
                } else {
                    TypeDecl::applyAutoContracts(resType, type);
                    type = resType;
                    return true;
                }
            }
        }
        if (type->firstType) {
            any |= inferTypeExpr(type->firstType);
        }
        if (type->secondType) {
            any |= inferTypeExpr(type->secondType);
        }
        for (auto &argType : type->argTypes) {
            any |= inferTypeExpr(argType);
        }
        return any;
    }
    FunctionPtr InferTypes::getOrCreateDummy(Module *mod) {
        auto dummy = make_smart<Function>();
        dummy->name = "```dummy```";
        dummy->module = mod;
        dummy->generated = true;
        auto dummyName = dummy->getMangledName();
        if (auto gen = mod->findGeneric(dummyName)) {
            return gen;
        }
        auto result = dummy;
        mod->addGeneric(dummy);
        reportAstChanged();
        return result;
    }
    bool InferTypes::tryMakeStructureCtor(Structure *var, bool isPrivate, bool fromGeneric) {
        if (!hasDefaultUserConstructor(var->name)) {
            auto ctor = makeConstructor(var, isPrivate);
            if (fromGeneric) {
                ctor->fromGeneric = getOrCreateDummy(var->module);
            }
            bool export_for_aot = !var->cppLayout && (daScriptEnvironment::getBound() && daScriptEnvironment::getBound()->g_isInAot);
            ctor->exports = alwaysExportInitializer || export_for_aot;
            extraFunctions.push_back(ctor);
            reportAstChanged();
            return true;
        } else {
            return false;
        }
    }
    bool InferTypes::safeExpression(Expression *expr) const {
        if (unsafeDepth)
            return true;
        if (expr->alwaysSafe)
            return true;
        return false;
    }
    TypeDeclPtr InferTypes::castStruct(const LineInfo &at, const TypeDeclPtr &subexprType, const TypeDeclPtr &castType, bool upcast) const {
        auto seT = subexprType;
        auto cT = castType;
        if (seT->isStructure()) {
            if (cT->isStructure()) {
                bool compatibleCast;
                if (upcast) {
                    compatibleCast = seT->structType->isCompatibleCast(*cT->structType);
                } else {
                    compatibleCast = cT->structType->isCompatibleCast(*seT->structType);
                }
                if (compatibleCast) {
                    auto exprType = make_smart<TypeDecl>(*cT);
                    exprType->ref = seT->ref;
                    exprType->constant = seT->constant;
                    return exprType;
                } else {
                    error("incompatible cast, can't cast " + seT->structType->name + " to " + cT->structType->name, "", "",
                          at, CompilationError::invalid_cast);
                }
            } else {
                error("invalid cast, expecting structure", "", "",
                      at, CompilationError::invalid_cast);
            }
        } else if (seT->isPointer() && seT->firstType && seT->firstType->isStructure()) {
            if (cT->isPointer() && cT->firstType->isStructure()) {
                bool compatibleCast;
                if (upcast) {
                    compatibleCast = seT->firstType->structType->isCompatibleCast(*cT->firstType->structType);
                } else {
                    compatibleCast = cT->firstType->structType->isCompatibleCast(*seT->firstType->structType);
                }
                if (compatibleCast) {
                    auto exprType = make_smart<TypeDecl>(*cT);
                    exprType->ref = seT->ref;
                    exprType->constant = seT->constant;
                    return exprType;
                } else {
                    error("incompatible cast, can't cast '" + seT->firstType->structType->name + "?' to '" + cT->firstType->structType->name + "?'", "", "",
                          at, CompilationError::invalid_cast);
                }
            } else {
                error("invalid cast, expecting structure pointer", "", "",
                      at, CompilationError::invalid_cast);
            }
        }
        return nullptr;
    }
    TypeDeclPtr InferTypes::castFunc(const LineInfo &at, const TypeDeclPtr &subexprType, const TypeDeclPtr &castType, bool upcast) const {
        auto seTF = subexprType;
        auto cTF = castType;
        if (seTF->argTypes.size() != cTF->argTypes.size()) {
            error("invalid cast, number of arguments does not match", "", "",
                  at, CompilationError::invalid_cast);
            return nullptr;
        }
        // result
        auto funT = make_smart<TypeDecl>(*seTF);
        auto cresT = cTF->firstType;
        auto resT = funT->firstType;
        if (resT == nullptr) {
            return nullptr;
        }
        if (!cresT->isSameType(*resT, RefMatters::yes, ConstMatters::no, TemporaryMatters::no)) {
            if (resT->isStructure() || (resT->isPointer() && resT->firstType && resT->firstType->isStructure())) {
                auto tryRes = castStruct(at, resT, cresT, upcast);
                if (tryRes) {
                    funT->firstType = tryRes;
                }
            }
        }
        funT->firstType->constant = cresT->constant;
        // arguments
        for (size_t i = 0, is = seTF->argTypes.size(); i != is; ++i) {
            auto seT = seTF->argTypes[i];
            auto cT = cTF->argTypes[i];
            if (!cT->isSameType(*seT, RefMatters::yes, ConstMatters::no, TemporaryMatters::no)) {
                if (seT->isStructure() || (seT->isPointer() && seT->firstType->isStructure())) {
                    auto tryArg = castStruct(at, seT, cT, upcast);
                    if (tryArg) {
                        funT->argTypes[i] = tryArg;
                    }
                }
            }
            funT->argTypes[i]->constant = cT->constant;
        }
        if (castType->isSameType(*funT, RefMatters::yes, ConstMatters::yes, TemporaryMatters::yes)) {
            return funT;
        } else {
            error("incompatible cast, can't cast " + describeType(funT) + " to " + describeType(castType), "", "",
                  at, CompilationError::invalid_cast);
            return nullptr;
        }
    }
    void InferTypes::updateNewFlags(ExprAscend *expr) {
        if (expr->subexpr->rtti_isMakeStruct()) {
            auto mks = static_pointer_cast<ExprMakeStruct>(expr->subexpr);
            if (expr->subexpr->type->baseType == Type::tHandle) {
                if (!mks->isNewHandle) {
                    reportAstChanged();
                    mks->isNewHandle = true;
                }
            } else {
                if (!mks->isNewClass) {
                    reportAstChanged();
                    mks->isNewClass = true;
                }
            }
        }
    }
    void InferTypes::setBlockCopyMoveFlags(ExprBlock *block) {
        if (block->returnType->isRefType() && !block->returnType->ref) {
            if (block->returnType->canCopy()) {
                block->copyOnReturn = true;
                block->moveOnReturn = false;
            } else if (block->returnType->canMove()) {
                block->copyOnReturn = false;
                block->moveOnReturn = true;
            } else {
                error("this type can't be returned at all: '" + describeType(block->returnType) + "'", "", "",
                      block->at, CompilationError::invalid_return_type);
            }
        } else {
            block->copyOnReturn = false;
            block->moveOnReturn = false;
        }
    }
    bool InferTypes::verifyPrivateFieldLookup(ExprField *expr) {
        // lets verify private field lookup
        if (expr->field() && expr->field()->privateField) {
            bool canLookup = false;
            if (func && func->isClassMethod) {
                TypeDecl selfT(func->classParent);
                if (selfT.isSameType(*expr->value->type,
                                     RefMatters::no, ConstMatters::no, TemporaryMatters::no, AllowSubstitute::yes)) {
                    canLookup = true;
                }
            }
            if (!canLookup) {
                error("can't access private field '" + expr->name + "' of " + describeType(expr->value->type) + " outside of member functions.", "", "",
                      expr->at, CompilationError::cant_get_field);
                return false;
            }
        }
        return true;
    }
    ExpressionPtr InferTypes::promoteToProperty(ExprField *expr, const ExpressionPtr &right, const string &opName) {
        if (!expr->no_promotion && expr->value->type) {
            if (right) {
                if (auto cloneSet = inferGenericOperator(".`" + expr->name + "`" + opName, expr->at, expr->value, right))
                    return cloneSet;
                auto valT = expr->value->type;
                if (valT->isPointer() && valT->firstType) {
                    auto derefV = make_smart<ExprPtr2Ref>(expr->at, expr->value);
                    derefV->type = make_smart<TypeDecl>(*valT->firstType);
                    TypeDecl::applyAutoContracts(derefV->type, valT->firstType);
                    derefV->type->ref = true;
                    derefV->type->constant |= valT->constant;
                    if (auto cloneSet = inferGenericOperator(".`" + expr->name + "`" + opName, expr->at, derefV, right))
                        return cloneSet;
                }
            } else {
                if (auto opE = inferGenericOperator(".`" + expr->name, expr->at, expr->value, nullptr))
                    return opE;
                if (auto opE = inferGenericOperatorWithName(".", expr->at, expr->value, expr->name))
                    return opE;
                auto valT = expr->value->type;
                if (valT->isPointer() && valT->firstType) {
                    auto derefV = make_smart<ExprPtr2Ref>(expr->at, expr->value);
                    derefV->type = make_smart<TypeDecl>(*valT->firstType);
                    TypeDecl::applyAutoContracts(derefV->type, valT->firstType);
                    derefV->type->ref = true;
                    derefV->type->constant |= valT->constant;
                    if (auto opE = inferGenericOperator(".`" + expr->name, expr->at, derefV, nullptr))
                        return opE;
                    if (auto opE = inferGenericOperatorWithName(".", expr->at, derefV, expr->name))
                        return opE;
                }
            }
        }
        return nullptr;
    }
    LineInfo InferTypes::makeConstAt(ExprField *expr) const {
        LineInfo constAt = expr->value->at;
        constAt.last_column = expr->atField.last_column;
        constAt.last_line = expr->atField.last_line;
        return constAt;
    }
    vector<VariablePtr> InferTypes::findMatchingVar(const string &name, bool seePrivate) const {
        string moduleName, varName;
        splitTypeName(name, moduleName, varName);
        vector<VariablePtr> result;
        auto inWhichModule = getSearchModule(moduleName);
        program->library.foreach ([&](Module *mod) -> bool {
                if ( auto var = mod->findVariable(varName) ) {
                    if ( inWhichModule->isVisibleDirectly(var->module) ) {
                        if ( seePrivate || canAccessGlobal(var,inWhichModule,thisModule) ) {
                            result.push_back(var);
                        }
                    }
                }
                return true; }, moduleName);
        return result;
    }
    bool InferTypes::inferReturnType(TypeDeclPtr &resType, ExprReturn *expr) {
        if (expr->subexpr && expr->subexpr->type && expr->subexpr->type->isVoid()) {
            error("returning void value", "", "",
                  expr->at, CompilationError::invalid_return_type);
            return false;
        }
        if (resType->isAuto()) {
            if (expr->subexpr) {
                if (!expr->subexpr->type) {
                    error("subexpresion type is not resolved yet", "", "", expr->at);
                    return false;
                } else if (expr->subexpr->type->isAutoOrAlias()) {
                    error("subexpresion type is not fully resolved yet", "", "", expr->at);
                    return true;
                }
                auto resT = TypeDecl::inferGenericType(resType, expr->subexpr->type, false, false, nullptr);
                if (!resT) {
                    error("type can't be inferred, " + describeType(resType) + ", returns " + describeType(expr->subexpr->type), "", "",
                          expr->at, CompilationError::cant_infer_mismatching_restrictions);
                } else {
                    resT->ref = false;
                    TypeDecl::applyAutoContracts(resT, resType);
                    resType = resT;
                    if (!resType->ref && resType->isWorkhorseType() && !resType->isPointer()) {
                        resType->constant = true;
                    }
                    resType->sanitize();
                    reportAstChanged();
                    return true;
                }
            } else {
                resType = make_smart<TypeDecl>(Type::tVoid);
                reportAstChanged();
                return true;
            }
        }
        if (resType->isVoid()) {
            if (expr->subexpr) {
                error("not expecting a return value", "", "",
                      expr->at, CompilationError::not_expecting_return_value);
            }
        } else {
            if (!expr->subexpr) {
                error("expecting a return value", "", "",
                      expr->at, CompilationError::expecting_return_value);
            } else {
                if (!canCopyOrMoveType(resType, expr->subexpr->type, TemporaryMatters::yes, expr->subexpr.get(),
                                       "incompatible return type", CompilationError::invalid_return_type, expr->at)) {
                }
                if (resType->ref && !expr->subexpr->type->isRef()) {
                    error("incompatible return type, reference matters. expecting " + describeType(resType) + ", passing " + describeType(expr->subexpr->type), "", "",
                          expr->at, CompilationError::invalid_return_type);
                }
                if (resType->isRef() && !resType->isConst() && expr->subexpr->type->isConst() && expr->moveSemantics) {
                    error("incompatible return type, constant matters. expecting " + describeType(resType) + ", passing " + describeType(expr->subexpr->type), "", "",
                          expr->at, CompilationError::invalid_return_type);
                }
            }
        }
        if (resType->isRefType()) {
            if (!resType->canCopy() && !resType->canMove()) {
                error("this type can't be returned at all " + describeType(resType), "", "",
                      expr->at, CompilationError::invalid_return_type);
            }
        }
        if (expr->moveSemantics && expr->subexpr->type->isConst()) {
            error("can't return via move from a constant value", "", "",
                  expr->at, CompilationError::cant_move);
        }
        return false;
    }
    void InferTypes::getDetailsAndSuggests(ExprReturn *expr, string &details, string &suggestions) const {
        if (verbose) {
            bool canMove = expr->subexpr->type->canMove();
            bool canClone = expr->subexpr->type->canClone();
            bool isConstant = expr->subexpr->type->isConst();
            if (canMove) {
                if (isConstant) {
                    details += "this type can't be moved because it's constant ";
                } else {
                    details += "this type can be moved ";
                    suggestions += "use return <- instead";
                }
            }
            if (canClone) {
                details += (details.size() ? "also, " : "");
                details += "this type can be cloned ";
                suggestions += (suggestions.size() ? " or " : "");
                suggestions += "use return <- clone(...) instead";
            }
            if (!canMove && !canClone) {
                details += "this type can't be copied or moved, so it can't be returned at all";
            }
        }
    }
    bool InferTypes::isConstExprFunc(Function *fun) const {
        return (fun->sideEffectFlags == 0) && (fun->builtIn) && (fun->result->isFoldable());
    }
    ExpressionPtr InferTypes::getConstExpr(Expression *expr) {
        if (expr->rtti_isConstant() && expr->type && expr->type->isFoldable()) {
            if (expr->type->isEnum()) {
                auto enumc = static_cast<ExprConstEnumeration *>(expr);
                auto enumv = enumc->enumType->find(enumc->text);
                if (!enumv.second)
                    return nullptr; // not found???
                if (!enumv.first || !enumv.first->type)
                    return nullptr; // not resolved
                if (!enumv.first->rtti_isConstant())
                    return nullptr; // not a constant
                // TODO: do we need to check if const is of the same size?
                // if ( enumc->baseType != enumv.first->type->baseType ) return nullptr;   // not a constant of the same type
            }
            return expr;
        }
        if (expr->rtti_isR2V()) {
            auto r2v = static_cast<ExprRef2Value *>(expr);
            return getConstExpr(r2v->subexpr.get());
        }
        if (expr->rtti_isVar()) { // global variable which happens to be constant
            auto var = static_cast<ExprVar *>(expr);
            auto variable = var->variable;
            if (variable && variable->init && variable->type->isConst() && variable->type->isFoldable()) {
                if (/*!var->local &&*/ // this is an interesting question. should we allow local const to be folded?
                    !var->argument &&
                    !var->block) {
                    if (variable->init->rtti_isConstant()) {
                        variable->access_fold = true;
                        return variable->init;
                    }
                }
            }
        }
        if (expr->rtti_isSwizzle()) {
            auto swz = static_cast<ExprSwizzle *>(expr);
            if (swz->value->type) {
                if (auto cswz = getConstExpr(swz->value.get())) {
                    int dim = swz->value->type->getVectorDim();
                    vector<uint8_t> fields;
                    if (TypeDecl::buildSwizzleMask(swz->mask, dim, fields)) {
                        auto baseType = swz->value->type->getVectorBaseType();
                        vec4f data = static_cast<ExprConst *>(cswz.get())->value;
                        vec4f resData = v_zero();
                        if (baseType != Type::tInt64 && baseType != Type::tUInt64) {
                            int32_t *res = (int32_t *)&resData;
                            int32_t *src = (int32_t *)&data;
                            int outI = 0;
                            for (auto f : fields) {
                                res[outI++] = src[f];
                            }
                        } else {
                            int64_t *res = (int64_t *)&resData;
                            int64_t *src = (int64_t *)&data;
                            int outI = 0;
                            for (auto f : fields) {
                                res[outI++] = src[f];
                            }
                        }
                        auto vecType = swz->type->getVectorType(baseType, int(fields.size()));
                        auto constValue = program->makeConst(expr->at, make_smart<TypeDecl>(vecType), resData);
                        constValue->type = make_smart<TypeDecl>(vecType);
                        constValue->type->at = expr->at;
                        return constValue;
                    }
                }
            }
        }
        return nullptr;
    }
    bool InferTypes::isPodDelete(TypeDecl *typ) {
        if (typ->temporary)
            return false;
        das_set<Structure *> dep;
        bool hasHeap = false;
        if (!isPodDelete(typ, dep, hasHeap))
            return false;
        return hasHeap;
    }
    bool InferTypes::isPodDelete(TypeDecl *typ, das_set<Structure *> &dep, bool &hasHeap) {
        if (typ->temporary)
            return false;
        if (typ->baseType == Type::tHandle) {
            return typ->annotation->isPod();
        } else if (typ->baseType == Type::tStructure) {
            if (typ->structType) {
                if (dep.find(typ->structType) != dep.end())
                    return true;
                dep.insert(typ->structType);
                if (typ->structType->isClass)
                    return false;
                auto finFn = getFinalizeFunc(typ);
                if (finFn.size() == 0) {
                    // ok, no finalize - there will be one generated
                } else if (finFn.size() == 1) {
                    auto finFunc = finFn.back();
                    if (!finFunc->generated)
                        return false;
                } else {
                    // seriously? more than one
                    return false;
                }
                for (const auto &fd : typ->structType->fields) {
                    if (fd.type && !fd.doNotDelete && (fd.type->constant || !isPodDelete(fd.type.get(), dep, hasHeap))) {
                        return false;
                    }
                }
            }
            return true;
        } else if (typ->baseType == Type::tTuple || typ->baseType == Type::tVariant || typ->baseType == Type::option) {
            for (const auto &arg : typ->argTypes) {
                if (arg->constant || !isPodDelete(arg.get(), dep, hasHeap)) {
                    return false;
                }
            }
            return true;
        } else if (typ->baseType == Type::tArray || typ->baseType == Type::tTable) {
            if (typ->firstType && (typ->firstType->constant || !isPodDelete(typ->firstType.get(), dep, hasHeap)))
                return false;
            if (typ->secondType && (typ->secondType->constant || !isPodDelete(typ->secondType.get(), dep, hasHeap)))
                return false;
            hasHeap = true;
        } else if (typ->baseType == Type::tPointer) {
            return !typ->needDelete(); // pointer is good if we don't need to delete
        } else if (typ->baseType == Type::tIterator || typ->baseType == Type::tBlock || typ->baseType == Type::tLambda || typ->baseType == Type::tFunction) {
            return false;
        }
        return true;
    }
    bool InferTypes::isEmptyInit(const VariablePtr &var) const {
        if (var->type && var->init) {
            if (var->init->rtti_isMakeStruct()) {
                auto ma = (ExprMakeStruct *)(var->init.get());
                if (ma->structs.empty() && ma->makeType) {
                    if (var->type->isGoodArrayType() && ma->makeType->isGoodArrayType() && ma->makeType->firstType->baseType == Type::autoinfer) {
                        return true;
                    } else if (var->type->isGoodTableType() && ma->makeType->isGoodTableType() && ma->makeType->firstType->baseType == Type::autoinfer && ma->makeType->secondType->baseType == Type::autoinfer) {
                        return true;
                    }
                }
            }
        }
        return false;
    }
    ExpressionPtr InferTypes::promoteToCloneToMove(const VariablePtr &var) {
        reportAstChanged();
        var->init_via_clone = false;
        var->init_via_move = true;
        auto c2m = make_smart<ExprCall>(var->at, "clone_to_move");
        c2m->arguments.push_back(var->init);
        return c2m;
    }
    bool InferTypes::canRelaxAssign(Expression *init) const {
        if (!relaxedAssign)
            return false;
        if (!init->type || !init->type->canMove())
            return false; // only if it can be moved
        if (init->rtti_isMakeLocal())
            return true; // a = [[...]] is always ok to transform to a <- [[...]]
        else if (init->rtti_isMakeBlock())
            return true; // a = @... is always ok to transform to a <- @
        else if (init->rtti_isAscend())
            return true; // a = new [[Foo()]] is always ok to transform to a <- new [[Foo()]]
        else if (init->rtti_isCallFunc()) {
            auto call = static_cast<ExprCallFunc *>(init);
            if (call->func && call->func->result && !call->func->result->ref) {
                return true; // a = f() is ok to transform to a <- f(), if its not a function which returns reference
            }
        } else if (init->rtti_isInvoke()) {
            auto inv = static_cast<ExprInvoke *>(init);
            if (inv->isCopyOrMove()) {
                return true; // a = invoke(f,...) is ok to transform to a <- invoke(f,...), if it does not return reference
            }
        }
        return false;
    }
    void InferTypes::expandTupleName(const string &name, const LineInfo &varAt) {
        // split name which consits of multiple names separated by ` into parts
        vector<string> parts;
        size_t pos = 0;
        while (pos < name.size()) {
            auto npos = name.find("`", pos);
            if (npos == string::npos) {
                parts.push_back(name.substr(pos));
                break;
            } else {
                parts.push_back(name.substr(pos, npos - pos));
                pos = npos + 1;
            }
        }
        int partIndex = 0;
        for (auto &part : parts) {
            // we build var_name._partIndex
            auto varName = make_smart<ExprVar>(varAt, name);
            auto partExpr = make_smart<ExprField>(varAt, varName, "_" + to_string(partIndex), true);
            assume.push_back(AssumeEntry{make_smart<ExprAssume>(varAt, part, ExpressionPtr(partExpr)), {}});
            partIndex++;
        }
    }
    void InferTypes::markNoDiscard(Expression *expr) { // this one marks that expression tree is not discarded (stops at call)
        expr->markNoDiscard();
    }
    bool InferTypes::isCloneArrayExpression(ExprCall *expr) const {
        if (!expr->func)
            return false;
        if (expr->arguments.size() != 2)
            return false;
        if (!(expr->func->name == "clone" || (expr->func->fromGeneric && expr->func->fromGeneric->name == "clone")))
            return false;
        if (!expr->arguments[1]->rtti_isCall()) {
            if (expr->arguments[1]->rtti_isMakeStruct()) {
                auto mks = static_cast<ExprMakeStruct *>(expr->arguments[1].get());
                if (mks->structs.size() == 0) {
                    return true; // its default<array<T>>
                } else {
                    return false;
                }
            } else {
                return false;
            }
        }
        auto call = (ExprCall *)(expr->arguments[1].get());
        if (!call->func)
            return false;
        if (!call->func->fromGeneric)
            return false;
        if (!(call->func->fromGeneric->name == "to_array_move" && call->func->fromGeneric->module->name == "builtin"))
            return false;
        return true;
    }

}
