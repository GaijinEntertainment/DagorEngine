#include "daScript/misc/platform.h"

#include "daScript/ast/ast.h"
#include "daScript/ast/ast_generate.h"
#include "daScript/ast/ast_infer_type.h"
#include "daScript/ast/ast_visitor.h"

namespace das {

    // in ast_handle of all places, due to reporting fields
    void reportTrait(const TypeDeclPtr &type, const string &prefix, const callable<void(const TypeDeclPtr &, const string &)> &report);

    string InferTypes::reportInferAliasErrors(const TypeDeclPtr &decl) const {
        if (!verbose)
            return "";
        TextWriter tw;
        reportInferAliasErrors(decl, tw);
        return tw.str();
    }
    void InferTypes::reportInferAliasErrors(const TypeDeclPtr &decl, TextWriter &tw, bool autoToAlias) const {
        autoToAlias |= decl->autoToAlias;
        if (decl->baseType == Type::typeMacro) {
            tw << "\tcan't infer type for 'typeMacro' ^'" << decl->typeMacroName() << "'\n";
            return;
        }
        if (decl->baseType == Type::typeDecl) {
            tw << "\tcan't infer type for 'typeDecl'\n";
            return;
        }
        if (decl->baseType == Type::autoinfer && !autoToAlias) { // until alias is fully resolved, can't infer
            tw << "\tcan't infer type for 'auto'\n";
            return;
        }
        if (decl->baseType == Type::alias || (decl->baseType == Type::autoinfer && autoToAlias)) {
            if (decl->isTag) {
                tw << "\tcan't infer type for '$t'\n";
                return;
            }
            auto aT = findAlias(decl->alias);
            if (!aT) {
                auto bT = nameToBasicType(decl->alias);
                if (bT != Type::none) {
                    aT = make_smart<TypeDecl>(bT);
                }
            }
            if (!aT) {
                tw << "\tdon't know what '" << decl->alias << "' is\n";
                return;
            }
        }
        if (decl->baseType == Type::tPointer) {
            if (decl->firstType) {
                reportInferAliasErrors(decl->firstType, tw);
            }
        } else if (decl->baseType == Type::tIterator) {
            if (decl->firstType) {
                reportInferAliasErrors(decl->firstType, tw);
            }
        } else if (decl->baseType == Type::tArray) {
            if (decl->firstType) {
                reportInferAliasErrors(decl->firstType, tw);
            }
        } else if (decl->baseType == Type::tTable) {
            if (decl->firstType) {
                reportInferAliasErrors(decl->firstType, tw);
            }
            if (decl->secondType) {
                reportInferAliasErrors(decl->secondType, tw);
            }
        } else if (decl->baseType == Type::tFunction || decl->baseType == Type::tLambda || decl->baseType == Type::tBlock ||
                   decl->baseType == Type::tVariant || decl->baseType == Type::tTuple || decl->baseType == Type::option) {
            for (size_t iA = 0, iAs = decl->argTypes.size(); iA != iAs; ++iA) {
                auto &declAT = decl->argTypes[iA];
                reportInferAliasErrors(declAT, tw);
            }
            if (decl->baseType == Type::tFunction || decl->baseType == Type::tLambda || decl->baseType == Type::tBlock) {
                if (!decl->firstType) {
                    tw << "\tcan't infer return type for '" << das_to_string(decl->baseType) << "'\n";
                    return;
                }
                reportInferAliasErrors(decl->firstType, tw);
            }
        }
    }
    string InferTypes::reportAliasError(const TypeDeclPtr &type) const {
        vector<string> aliases;
        type->collectAliasList(aliases);
        TextWriter ss;
        ss << "don't know what '" << describeType(type) << "' is";
        for (auto &aa : aliases) {
            ss << "; unknown type '" << aa << "'";
        }
        return ss.str();
    }
    string InferTypes::describeMismatchingArgument(const string &argName, const TypeDeclPtr &passType, const TypeDeclPtr &argType, int argIndex) const {
        TextWriter ss;
        ss << "\t\tinvalid argument '" << argName << "' (" << argIndex << "). expecting '"
           << describeType(argType) << "', passing '" << describeType(passType) << "'\n";
        if (passType->isAlias()) {
            ss << "\t\t" << reportAliasError(passType) << "\n";
        }
        if (argType->isRef() && !passType->isRef()) {
            ss << "\t\tcan't pass non-ref to ref\n";
        }
        if (argType->isRef() && !argType->constant && passType->constant) {
            ss << "\t\tcan't ref types can only add constness\n";
        }
        if (argType->isPointer() && !argType->constant && passType->constant) {
            ss << "\t\tpointer types can only add constness\n";
        }
        return ss.str();
    }
    int InferTypes::getMismatchingFunctionRank(const FunctionPtr &pFn, const vector<TypeDeclPtr> &nonNamedTypes, const vector<MakeFieldDeclPtr> &arguments, bool inferAuto, bool inferBlock) const {
        int rank = 0;
        size_t fnArgIndex = 0;
        for (size_t ai = 0, ais = arguments.size(); ai != ais; ++ai) {
            auto &arg = arguments[ai];
            for (;;) {
                if (fnArgIndex >= pFn->arguments.size()) {
                    return rank + 1000;
                }
                auto &fnArg = pFn->arguments[fnArgIndex];
                if (fnArg->name == arg->name) {
                    break;
                }
                if (!fnArg->init && fnArgIndex >= nonNamedTypes.size()) {
                    return rank + 1000;
                }
                fnArgIndex++;
            }
            auto &passType = arg->value->type;
            auto &argType = pFn->arguments[fnArgIndex]->type;
            if (!isMatchingArgument(pFn, argType, passType, inferAuto, inferBlock)) {
                rank += 1000;
            }
            fnArgIndex++;
        }
        while (fnArgIndex < pFn->arguments.size()) {
            if (!pFn->arguments[fnArgIndex]->init) {
                rank += 1000;
            }
            fnArgIndex++;
        }
        return rank;
    }
    int InferTypes::getMismatchingFunctionRank(const FunctionPtr &pFn, const vector<TypeDeclPtr> &types, bool inferAuto, bool inferBlock) const {
        TextWriter ss;
        size_t tot = das::min(types.size(), pFn->arguments.size());
        size_t ai;
        int rank = 0;
        for (ai = 0; ai != tot; ++ai) {
            auto &arg = pFn->arguments[ai];
            auto &passType = types[ai];
            if (!isMatchingArgument(pFn, arg->type, passType, inferAuto, inferBlock)) {
                rank += 1000;
            }
        }
        rank += int(pFn->arguments.size() - types.size()) * 1000;
        for (const auto &ann : pFn->annotations) {
            auto fnAnn = static_pointer_cast<FunctionAnnotation>(ann->annotation);
            if (fnAnn->isSpecialized()) {
                string err;
                if (!fnAnn->isCompatible(pFn, types, *ann, err)) {
                    rank += 1000;
                }
            }
        }
        return rank;
    }
    string InferTypes::describeMismatchingFunction(const FunctionPtr &pFn, const vector<TypeDeclPtr> &nonNamedTypes, const vector<MakeFieldDeclPtr> &arguments, bool inferAuto, bool inferBlock) const {
        if (pFn->arguments.size() < arguments.size()) {
            return "\t\ttoo many arguments\n";
        }
        TextWriter ss;
        size_t fnArgIndex = 0;
        for (size_t ai = 0, ais = arguments.size(); ai != ais; ++ai) {
            auto &arg = arguments[ai];
            for (;;) {
                if (fnArgIndex >= pFn->arguments.size()) {
                    auto it = find_if(pFn->arguments.begin(), pFn->arguments.end(), [&](const VariablePtr &varg) { return varg->name == arg->name; });
                    if (it != pFn->arguments.end()) {
                        ss << "\t\tcan't match argument " << arg->name << ", its submitted out of order\n";
                    } else {
                        ss << "\t\tcan't match argument " << arg->name << ", out of function arguments\n";
                    }
                    return ss.str();
                }
                auto &fnArg = pFn->arguments[fnArgIndex];
                if (fnArg->name == arg->name) {
                    break;
                }
                if (!fnArg->init && fnArgIndex >= nonNamedTypes.size()) {
                    ss << "\t\twhile looking for argument " << arg->name
                       << ", can't skip function argument " << fnArg->name << " because it has no default value\n";
                    return ss.str();
                }
                fnArgIndex++;
            }
            auto &passType = arg->value->type;
            auto &argType = pFn->arguments[fnArgIndex]->type;
            if (!isMatchingArgument(pFn, argType, passType, inferAuto, inferBlock)) {
                ss << describeMismatchingArgument(arg->name, passType, argType, (int)ai);
            }
            fnArgIndex++;
        }
        while (fnArgIndex < pFn->arguments.size()) {
            if (!pFn->arguments[fnArgIndex]->init) {
                ss << "\t\tmissing default value for function argument " << pFn->arguments[fnArgIndex]->name << "\n";
                return ss.str();
            }
            fnArgIndex++;
        }
        return ss.str();
    }
    string InferTypes::describeMismatchingFunction(const FunctionPtr &pFn, const vector<TypeDeclPtr> &types, bool inferAuto, bool inferBlock) const {
        TextWriter ss;
        size_t tot = das::min(types.size(), pFn->arguments.size());
        size_t ai;
        for (ai = 0; ai != tot; ++ai) {
            auto &arg = pFn->arguments[ai];
            auto &passType = types[ai];
            if (!isMatchingArgument(pFn, arg->type, passType, inferAuto, inferBlock)) {
                ss << describeMismatchingArgument(arg->name, passType, arg->type, (int)ai);
            }
        }
        for (size_t ais = pFn->arguments.size(); ai != ais; ++ai) {
            auto &arg = pFn->arguments[ai];
            if (!arg->init) {
                ss << "\t\tmissing argument " << arg->name << "\n";
            }
        }
        for (const auto &ann : pFn->annotations) {
            auto fnAnn = static_pointer_cast<FunctionAnnotation>(ann->annotation);
            if (fnAnn->isSpecialized()) {
                string err;
                if (!fnAnn->isCompatible(pFn, types, *ann, err)) {
                    ss << "\t\t" << err << "\n";
                }
            }
        }
        return ss.str();
    }
    string InferTypes::reportMismatchingMemberCall(Structure *st, const string &name, const vector<MakeFieldDeclPtr> &arguments, const vector<TypeDeclPtr> &nonNamedArguments, bool methodCall) const {
        auto field = st->findField(name);
        if (!field)
            return "";
        if (!field->classMethod) {
            return "member '" + name + "' is not a method in '" + st->name + "'\n";
        }
        auto addr = field->init;
        if (addr->rtti_isCast()) {
            addr = static_pointer_cast<ExprCast>(addr)->subexpr;
        }
        if (!addr->rtti_isAddr()) {
            return "function is not inferred yet\n";
        }
        auto pAddr = static_pointer_cast<ExprAddr>(addr);
        if (!pAddr->func) {
            return "expecting @@ in class member initialization\n";
        }
        vector<TypeDeclPtr> nna = nonNamedArguments;
        if (!methodCall) {
            nna.insert(nna.begin(), make_smart<TypeDecl>(st));
        }
        return describeMismatchingFunction(pAddr->func, nna, arguments, false, false);
    }
    void InferTypes::reportDualFunctionNotFound(const string &name, const string &extra,
                                                const LineInfo &at, const MatchingFunctions &candidateFunctions,
                                                const vector<TypeDeclPtr> &types, const vector<TypeDeclPtr> &types2, bool inferAuto, bool inferBlocks, bool reportDetails,
                                                CompilationError cerror, int nExtra, const string &moreError) {
        if (verbose) {
            TextWriter ss;
            ss << name << "(";
            bool first = true;
            for (auto &it : types) {
                if (!first) {
                    ss << ", ";
                }
                first = false;
                ss << it->describe();
            }
            ss << ") or (";
            first = true;
            for (auto &it : types2) {
                if (!first) {
                    ss << ", ";
                }
                first = false;
                ss << it->describe();
            }
            ss << ")\n";
            if (func) {
                ss << "while compiling: " << func->describe() << "\n";
            }
            if (!moreError.empty()) {
                ss << moreError;
            }
            if (candidateFunctions.size() == 0) {
                ss << "there are no good matching candidates out of " << nExtra << " total functions with the same name\n";
            } else if (candidateFunctions.size() > 1) {
                ss << "candidates:\n";
            } else if (candidateFunctions.size() == 1) {
                ss << (nExtra ? "\nmost likely candidate:\n" : "\ncandidate function:\n");
            }
            string moduleName, funcName;
            splitTypeName(name, moduleName, funcName);
            auto inWhichModule = getSearchModule(moduleName);
            for (auto &missFn : candidateFunctions) {
                auto visM = getFunctionVisModule(missFn);
                bool isVisible = isVisibleFunc(inWhichModule, visM);
                if (!reportInvisibleFunctions && !isVisible)
                    continue;
                bool isPrivate = missFn->privateFunction && !canCallPrivate(missFn, inWhichModule, thisModule);
                if (!reportPrivateFunctions && isPrivate)
                    continue;
                ss << "\t";
                if (missFn->module && !missFn->module->name.empty() && !(missFn->module->name == "builtin"))
                    ss << missFn->module->name << "::";
                ss << describeFunction(missFn);
                if (missFn->builtIn) {
                    ss << " // builtin";
                } else {
                    ss << " at " << missFn->at.describe();
                }
                ss << "\n";
                if (missFn->name != funcName) {
                    ss << "\t\tname is similar, typo?\n";
                }
                if (reportDetails) {
                    if (missFn->arguments.size() == types2.size()) {
                        ss << describeMismatchingFunction(missFn, types2, inferAuto, inferBlocks);
                    } else {
                        ss << describeMismatchingFunction(missFn, types, inferAuto, inferBlocks);
                    }
                }
                if (!isVisible) {
                    ss << "\t\tmodule " << visM->name << " is not visible directly from ";
                    if (inWhichModule->name.empty()) {
                        ss << "the current module\n";
                    } else {
                        ss << inWhichModule->name << "\n";
                    }
                }
                if (isPrivate) {
                    ss << "\t\tfunction is private";
                    if (missFn->module && !missFn->module->name.empty()) {
                        ss << " to module " << missFn->module->name;
                    }
                    ss << "\n";
                }
                if (missFn->isTemplate) {
                    ss << "\t\tfunction is part of a template, and can't be called without template instance\n";
                }
            }
            if (nExtra > 0 && candidateFunctions.size() != 0) {
                ss << "also " << nExtra << " more candidates\n";
            }
            error(extra, ss.str(), "", at, cerror);
        } else {
            error(extra, "", "", at, cerror);
        }
    }
    void InferTypes::reportFunctionNotFound(const string &name, const string &extra,
                                            const LineInfo &at, const MatchingFunctions &candidateFunctions,
                                            const vector<TypeDeclPtr> &types, bool inferAuto, bool inferBlocks, bool reportDetails,
                                            CompilationError cerror, int nExtra, const string &moreError) {
        if (verbose) {
            TextWriter ss;
            ss << name << "(";
            bool first = true;
            for (auto &it : types) {
                if (!first) {
                    ss << ", ";
                }
                first = false;
                ss << it->describe();
            }
            ss << ")\n";
            if (func) {
                ss << "while compiling: " << func->describe() << "\n";
            }
            if (!moreError.empty()) {
                ss << moreError;
            }
            if (candidateFunctions.size() == 0) {
                ss << "there are no good matching candidates out of " << nExtra << " total functions with the same name\n";
            } else if (candidateFunctions.size() > 1) {
                ss << "candidates:\n";
            } else if (candidateFunctions.size() == 1) {
                ss << (nExtra ? "\nmost likely candidate:\n" : "\ncandidate function:\n");
            }
            string moduleName, funcName;
            splitTypeName(name, moduleName, funcName);
            auto inWhichModule = getSearchModule(moduleName);
            for (auto &missFn : candidateFunctions) {
                auto visM = getFunctionVisModule(missFn);
                bool isVisible = isVisibleFunc(inWhichModule, visM);
                if (!reportInvisibleFunctions && !isVisible)
                    continue;
                bool isPrivate = missFn->privateFunction && !canCallPrivate(missFn, inWhichModule, thisModule);
                if (!reportPrivateFunctions && isPrivate)
                    continue;
                ss << "\t";
                if (missFn->module && !missFn->module->name.empty() && !(missFn->module->name == "builtin"))
                    ss << missFn->module->name << "::";
                ss << describeFunction(missFn);
                if (missFn->builtIn) {
                    ss << " // builtin";
                } else {
                    ss << " at " << missFn->at.describe();
                }
                ss << "\n";
                if (missFn->name != funcName) {
                    ss << "\t\tname is similar, typo?\n";
                }
                if (reportDetails) {
                    ss << describeMismatchingFunction(missFn, types, inferAuto, inferBlocks);
                }
                if (!isVisible) {
                    ss << "\t\tmodule " << visM->name << " is not visible directly from ";
                    if (inWhichModule->name.empty()) {
                        ss << "the current module\n";
                    } else {
                        ss << inWhichModule->name << "\n";
                    }
                }
                if (isPrivate) {
                    ss << "\t\tfunction is private";
                    if (missFn->module && !missFn->module->name.empty()) {
                        ss << " to module " << missFn->module->name;
                    }
                    ss << "\n";
                }
                if (missFn->isTemplate) {
                    ss << "\t\tfunction is part of a template, and can't be called without template instance\n";
                }
            }
            if (nExtra > 0 && candidateFunctions.size() != 0) {
                ss << "also " << nExtra << " more candidates\n";
            }
            error(extra, ss.str(), "", at, cerror);
        } else {
            error(extra, "", "", at, cerror);
        }
    }
    void InferTypes::reportFunctionNotFound(const string &name, const string &extra,
                                            const LineInfo &at, const MatchingFunctions &candidateFunctions,
                                            const vector<TypeDeclPtr> &nonNamedTypes, const vector<MakeFieldDeclPtr> &arguments,
                                            bool inferAuto, bool inferBlocks, bool reportDetails,
                                            CompilationError cerror, int nExtra, const string &moreError) {
        if (verbose) {
            TextWriter ss;
            if (!moreError.empty()) {
                ss << moreError;
            }
            if (candidateFunctions.size() == 0) {
                ss << "there are no good matching candidates out of " << nExtra << " total functions with the same name\n";
            } else if (candidateFunctions.size() > 1) {
                ss << "candidates:\n";
            } else if (candidateFunctions.size() == 1) {
                ss << (nExtra ? "\nmost likely candidate:\n" : "\ncandidate function:\n");
            }
            string moduleName, funcName;
            splitTypeName(name, moduleName, funcName);
            auto inWhichModule = getSearchModule(moduleName);
            for (auto &missFn : candidateFunctions) {
                auto visM = getFunctionVisModule(missFn);
                bool isVisible = isVisibleFunc(inWhichModule, visM);
                if (!reportInvisibleFunctions && !isVisible)
                    continue;
                bool isPrivate = missFn->privateFunction && !canCallPrivate(missFn, inWhichModule, thisModule);
                if (!reportPrivateFunctions && isPrivate)
                    continue;
                ss << "\t";
                if (missFn->module && !missFn->module->name.empty() && !(missFn->module->name == "builtin"))
                    ss << missFn->module->name << "::";
                ss << describeFunction(missFn);
                if (missFn->builtIn) {
                    ss << " // builtin";
                } else if (missFn->at.line) {
                    ss << " at " << missFn->at.describe();
                }
                ss << "\n";
                if (reportDetails) {
                    ss << describeMismatchingFunction(missFn, nonNamedTypes, arguments, inferAuto, inferBlocks);
                }
                if (!isVisible) {
                    ss << "\t\tmodule " << visM->name << " is not visible directly from ";
                    if (inWhichModule->name.empty()) {
                        ss << "the current module\n";
                    } else {
                        ss << inWhichModule->name << "\n";
                    }
                }
                if (isPrivate) {
                    ss << "\t\tfunction is private";
                    if (missFn->module && !missFn->module->name.empty()) {
                        ss << " to module " << missFn->module->name;
                    }
                    ss << "\n";
                }
                if (nExtra > 0 && candidateFunctions.size() != 0) {
                    ss << "also " << nExtra << " more candidates\n";
                }
            }
            error(extra, ss.str(), "", at, cerror);
        } else {
            error(extra, "", "", at, cerror);
        }
    }
    void InferTypes::reportCantClone(const string &message, const TypeDeclPtr &type, const LineInfo &at) const {
        if (verbose) {
            TextWriter ss;
            reportTrait(type, type->describe(TypeDecl::DescribeExtra::no, TypeDecl::DescribeContracts::no), [&](const TypeDeclPtr &subT, const string &trait) {
                    if ( subT != type && !subT->canClone() ) {
                        if ( !(subT->baseType==Type::tStructure || subT->baseType==Type::tVariant || subT->baseType==Type::tTuple) ) {
                            ss << "\tcan't clone '" << trait << ": " << describeType(subT) << "'\n";
                        }
                    } });
            error(message, ss.str(), "", at, CompilationError::cant_copy);
        } else {
            error(message, "", "", at, CompilationError::cant_copy);
        }
    }
    void InferTypes::reportCantCloneFromConst(const string &errorText, CompilationError errorCode, const TypeDeclPtr &type, const LineInfo &at) const {
        if (verbose) {
            TextWriter ss;
            reportTrait(type, type->describe(TypeDecl::DescribeExtra::no, TypeDecl::DescribeContracts::no), [&](const TypeDeclPtr &subT, const string &trait) {
                    if ( subT != type && !subT->canCloneFromConst() ) {
                        if ( !(subT->baseType==Type::tStructure || subT->baseType==Type::tVariant || subT->baseType==Type::tTuple) ) {
                            ss << "\tcan't assign '" << trait << ": " << describeType(subT) << " = " << describeType(subT) << " const'\n";
                        }
                    } });
            error(errorText + "; " + describeType(type), ss.str(), "", at, errorCode);
        } else {
            error(errorText, "", "", at, errorCode);
        }
    }
    void InferTypes::reportFunctionNotFound(ExprAddr *expr) {
        if (verbose && (reportInvisibleFunctions || reportPrivateFunctions)) {
            TextWriter ss;
            if (func) {
                ss << "while compiling: " << func->describe() << "\n";
            }
            string moduleName, funcName;
            splitTypeName(expr->target, moduleName, funcName);
            MatchingFunctions result;
            auto inWhichModule = getSearchModule(moduleName);
            auto hFuncName = hash64z(funcName.c_str());
            program->library.foreach ([&](Module *mod) -> bool {
                    auto itFnList = mod->functionsByName.find(hFuncName);
                    if ( itFnList ) {
                        auto & goodFunctions = itFnList->second;
                        for ( auto & missFn : goodFunctions ) {
                            auto visM = getFunctionVisModule(missFn);
                            bool isVisible = isVisibleFunc(inWhichModule,visM);
                            if ( !reportInvisibleFunctions  && !isVisible ) continue;
                            bool isPrivate = missFn->privateFunction && !canCallPrivate(missFn,inWhichModule,thisModule);
                            if ( !reportPrivateFunctions && isPrivate ) continue;
                            ss << "\t";
                            if ( missFn->module && !missFn->module->name.empty() && !(missFn->module->name=="builtin") )
                                ss << missFn->module->name << "::";
                            ss << describeFunction(missFn);
                            if ( missFn->builtIn ) {
                                ss << " // builtin";
                            } else {
                                ss << " at " << missFn->at.describe();
                            }
                            ss << "\n";
                            if ( !isVisible ) {
                                ss << "\t\tmodule " << visM->name << " is not visible directly from ";
                                if ( inWhichModule->name.empty()) {
                                    ss << "the current module\n";
                                } else {
                                    ss << inWhichModule->name << "\n";
                                }
                            }
                            if ( isPrivate ) {
                                ss << "\t\tfunction is private";
                                if ( missFn->module && !missFn->module->name.empty() ) {
                                    ss << " to module " << missFn->module->name;
                                }
                                ss << "\n";
                            }
                            if ( missFn->isTemplate ) {
                                ss << "\t\tfunction is part of a template, and can't be called without template instance\n";
                            }
                        }
                    }
                    return true; }, moduleName);
            error("function not found " + expr->target, ss.str(), "",
                  expr->at, CompilationError::function_not_found);
        } else {
            error("function not found " + expr->target, "", "",
                  expr->at, CompilationError::function_not_found);
        }
    }
    void InferTypes::reportMissingFinalizer(const string &message, const LineInfo &at, const TypeDeclPtr &ftype) {
        if (verbose) {
            auto fakeCall = make_smart<ExprCall>(at, "_::finalize");
            auto fakeVar = make_smart<ExprVar>(at, "this");
            fakeVar->type = make_smart<TypeDecl>(*ftype);
            fakeCall->arguments.push_back(fakeVar);
            vector<TypeDeclPtr> fakeTypes = {ftype};
            reportMissing(fakeCall.get(), fakeTypes, message, true, CompilationError::function_already_declared);
        } else {
            error(message, "", "", at, CompilationError::function_already_declared);
        }
    }
    int InferTypes::sortCandidates(RankedMatchingFunctions &ranked, MatchingFunctions &candidates, int nArgs) {
        if (candidates.size() == 1) {
            return 0;
        }
        sort(ranked.begin(), ranked.end(), [](const pair<Function *, int> &a, const pair<Function *, int> &b) { return a.second < b.second; });
        // if there is one or more 'one-off's' - we only leave them
        int nTotal = int(ranked.size());
        int nOnes = 0;
        for (auto &r : ranked) {
            if (r.second > 1000)
                break;
            nOnes++;
        }
        if (nOnes > 0) {
            ranked.resize(nOnes);
        } else {
            // now we remove all the ones where rank exceeds number of arguments
            int nOkay = nTotal;
            int notOkayRank = nArgs * 1000;
            while (nOkay > 0 && ranked[nOkay - 1].second >= notOkayRank) {
                nOkay--;
            }
            ranked.resize(nOkay);
        }
        candidates.clear();
        for (auto &r : ranked) {
            candidates.push_back(r.first);
        }
        return nTotal - int(ranked.size());
    }
    int InferTypes::prepareCandidates(MatchingFunctions &candidates, const vector<TypeDeclPtr> &nonNamedArguments, const vector<MakeFieldDeclPtr> &arguments, bool inferAuto, bool inferBlocks) {
        RankedMatchingFunctions ranked;
        ranked.reserve(candidates.size());
        for (auto can : candidates) {
            int rank = getMismatchingFunctionRank(can, nonNamedArguments, arguments, inferAuto, inferBlocks);
            ranked.push_back(make_pair(can, rank));
        }
        return sortCandidates(ranked, candidates, int(arguments.size() + nonNamedArguments.size()));
    }
    void InferTypes::reportMissing(ExprNamedCall *expr, const vector<TypeDeclPtr> &nonNamedArguments, const string &msg, bool reportDetails,
                                   CompilationError cerror, const string &moreError) {
        if (verbose) {
            auto can1 = findMissingCandidates(expr->name, false);
            auto can2 = findMissingGenericCandidates(expr->name, false);
            can1.reserve(can1.size() + can2.size());
            can1.insert(can1.end(), can2.begin(), can2.end());
            auto nExtra = prepareCandidates(can1, nonNamedArguments, expr->arguments, true, true);
            reportFunctionNotFound(expr->name, msg + expr->name, expr->at, can1, nonNamedArguments, expr->arguments, false, true, reportDetails, cerror, nExtra, moreError);
        } else {
            error("no matching functions or generics: " + expr->name, "", "", expr->at, cerror);
        }
    }
    void InferTypes::reportExcess(ExprNamedCall *expr, const vector<TypeDeclPtr> &nonNamedArguments, const string &msg, MatchingFunctions can1, const MatchingFunctions &can2,
                                  CompilationError cerror) {
        if (verbose) {
            can1.reserve(can1.size() + can2.size());
            can1.insert(can1.end(), can2.begin(), can2.end());
            reportFunctionNotFound(expr->name, msg + expr->name, expr->at, can1, nonNamedArguments, expr->arguments, false, true, false, cerror, 0, "");
        } else {
            error("too many matching functions or generics: " + expr->name, "", "", expr->at, cerror);
        }
    }
    int InferTypes::prepareCandidates(MatchingFunctions &candidates, const vector<TypeDeclPtr> &nonNamedArguments, bool inferAuto, bool inferBlocks) {
        if (candidates.size() < (size_t)program->policies.always_report_candidates_threshold)
            return 0;
        RankedMatchingFunctions ranked;
        ranked.reserve(candidates.size());
        for (auto can : candidates) {
            int rank = getMismatchingFunctionRank(can, nonNamedArguments, inferAuto, inferBlocks);
            ranked.push_back(make_pair(can, rank));
        }
        return sortCandidates(ranked, candidates, int(nonNamedArguments.size()));
    }
    string InferTypes::reportMethodVsCall(ExprLooksLikeCall *expr) {
        if (verbose && expr->arguments.size() >= 1) {
            if (auto tp = expr->arguments[0]->type.get()) {
                Structure *cls = nullptr;
                if (tp->isClass()) {
                    cls = tp->structType;
                } else if (tp->isPointer() && tp->firstType && tp->firstType->isClass()) {
                    cls = tp->firstType->structType;
                }
                if (cls) {
                    auto fld = cls->findField(expr->name);
                    if (fld && fld->type->isFunction()) {
                        TextWriter ss;
                        ss << cls->module->name << "::" << cls->name << " has method " << expr->name << ", did you mean ";
                        ss << *(expr->arguments[0]) << "->" << expr->name << "(";
                        for (size_t i = 1; i < expr->arguments.size(); ++i) {
                            if (i > 1)
                                ss << ", ";
                            ss << *(expr->arguments[i]);
                        }
                        ss << ")\n";
                        return ss.str();
                    }
                }
            }
        }
        return "";
    }
    void InferTypes::reportMissing(ExprLooksLikeCall *expr, const vector<TypeDeclPtr> &types,
                                   const string &msg, bool reportDetails,
                                   CompilationError cerror) {
        if (verbose) {
            auto can1 = findMissingCandidates(expr->name, false);
            auto can2 = findMissingGenericCandidates(expr->name, false);
            can1.reserve(can1.size() + can2.size());
            can1.insert(can1.end(), can2.begin(), can2.end());
            auto nExtra = prepareCandidates(can1, types, true, true);
            reportFunctionNotFound(expr->name, msg + (verbose ? expr->describe() : ""), expr->at, can1, types, true, true,
                                   reportDetails, cerror, nExtra, reportMethodVsCall(expr));
        } else {
            error("no matching functions or generics: " + expr->name, "", "", expr->at, cerror);
        }
    }
    void InferTypes::reportExcess(ExprLooksLikeCall *expr, const vector<TypeDeclPtr> &types,
                                  const string &msg, MatchingFunctions can1, const MatchingFunctions &can2,
                                  CompilationError cerror) {
        if (verbose) {
            can1.reserve(can1.size() + can2.size());
            can1.insert(can1.end(), can2.begin(), can2.end());
            reportFunctionNotFound(expr->name, msg + expr->name, expr->at, can1, types, false, true,
                                   false, cerror, 0, reportMethodVsCall(expr));
        } else {
            error("too many matching functions or generics: " + expr->name, "", "", expr->at, cerror);
        }
    }
    bool InferTypes::reportOp2Errors(ExprLooksLikeCall *expr) {
        auto expr_left = expr->arguments[0].get();
        auto expr_right = expr->arguments[1].get();
        auto expr_op = expr->name.substr(3);
        if (expr_left->type->isNumeric() && expr_right->type->isNumeric()) {
            if (isAssignmentOperator(expr_op)) {
                if (!expr_left->type->ref) {
                    error("numeric operator '" + expr_op + "' left side must be reference.", "", "",
                          expr->at, CompilationError::operator_not_found);
                    return true;
                } else if (expr_left->type->isConst()) {
                    error("numeric operator '" + expr_op + "' left side can't be constant.", "", "",
                          expr->at, CompilationError::operator_not_found);
                    return true;
                } else {
                    if (verbose) {
                        TextWriter tw;
                        tw << "\t" << *expr_left << " " << expr_op << " " << das_to_string(expr_left->type->baseType) << "(" << *expr_right << ")\n";
                        error("numeric operator '" + expr_op + "' type mismatch. both sides have to be of the same type; " +
                                  das_to_string(expr_left->type->baseType) + " " + expr_op + " " + das_to_string(expr_right->type->baseType) + " is not defined",
                              "", "try the following\n" + tw.str(),
                              expr->at, CompilationError::operator_not_found);
                        return true;
                    } else {
                        error("numeric operator '" + expr_op + "' type mismatch. both sides have to be of the same type. ", "", "",
                              expr->at, CompilationError::operator_not_found);
                        return true;
                    }
                }
            } else {
                if (verbose) {
                    if (expr_left->type->baseType != expr_right->type->baseType) {
                        TextWriter tw;
                        tw << "\t" << *expr_left << " " << expr_op << " " << das_to_string(expr_left->type->baseType) << "(" << *expr_right << ")\n";
                        tw << "\t" << das_to_string(expr_right->type->baseType) << "(" << *expr_left << ") " << expr_op << " " << *expr_right << "\n";
                        error("numeric operator '" + expr_op + "' type mismatch. both sides have to be of the same type. " +
                                  das_to_string(expr_left->type->baseType) + " " + expr_op + " " + das_to_string(expr_right->type->baseType) + " is not defined",
                              "", "try one of the following\n" + tw.str(),
                              expr->at, CompilationError::operator_not_found);
                        return true;
                    } else if (expr_left->type->isNumericStorage()) {
                        error("numeric operator '" + expr_op + "' is not defined for storage types (int8, uint8, int16, uint16)",
                              "\t" + das_to_string(expr_left->type->baseType) + " " + expr_op + " " + das_to_string(expr_right->type->baseType),
                              "", expr->at, CompilationError::operator_not_found);
                        return true;
                    } else {
                        error("numeric operator '" + expr_op + "' type mismatch",
                              "\t" + das_to_string(expr_left->type->baseType) + " " + expr_op + " " + das_to_string(expr_right->type->baseType),
                              "", expr->at, CompilationError::operator_not_found);
                        return true;
                    }
                } else {
                    error("numeric operator '" + expr_op + "' type mismatch", "", "",
                          expr->at, CompilationError::operator_not_found);
                    return true;
                }
            }
        }
        return false;
    }
    void InferTypes::collectMissingOperators(const string &opN, MatchingFunctions &mf, bool identicalName) {
        auto opName = "_::" + opN;
        auto can1 = findMissingCandidates(opName, identicalName);
        auto can2 = findMissingGenericCandidates(opName, identicalName);
        mf.reserve(mf.size() + can1.size() + can2.size());
        mf.insert(mf.end(), can1.begin(), can1.end());
        mf.insert(mf.end(), can2.begin(), can2.end());
    }
}
