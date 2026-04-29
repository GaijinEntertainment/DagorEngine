#include "daScript/misc/platform.h"

#include "daScript/ast/ast.h"
#include "daScript/ast/ast_generate.h"
#include "daScript/ast/ast_infer_type.h"
#include "daScript/ast/ast_visitor.h"

namespace das {

    Module *InferTypes::getSearchModule(string &moduleName) const {
        if (moduleName == "_") {
            moduleName = "*";
            return thisModule;
        } else if (moduleName == "__") {
            moduleName = thisModule->name;
            return thisModule;
        } else if (func) {
            if (func->fromGeneric) {
                auto origin = func->getOrigin();
                if (moduleName.empty()) { // ::foo in generic means generic::goo, not this::foo
                    moduleName = origin->module->name;
                }
                return origin->module;
            } else {
                return func->module;
            }
        } else {
            return thisModule;
        }
    }
    Module *InferTypes::getFunctionVisModule(Function *fn) const {
        return fn->fromGeneric ? fn->getOrigin()->module : fn->module;
    }
    bool InferTypes::canCallPrivate(Function *pFn, Module *mod, Module *thisMod) const {
        if (!pFn->privateFunction) {
            return true;
        } else if (pFn->module == mod || pFn->module == thisMod) {
            return true;
        }
        // builtin module can access private functions of $
        if (pFn->module->name == "$" && mod->name == "builtin") {
            return true;
        }
        if (pFn->fromGeneric) {
            auto origin = pFn->getOrigin();
            return (origin->module == mod) || (origin->module == thisMod);
        } else {
            return false;
        }
    }
    bool InferTypes::canAccessGlobal(const VariablePtr &pVar, Module *mod, Module *thisMod) const {
        if (!pVar->private_variable) {
            return true;
        } else if (pVar->module == mod || pVar->module == thisMod) {
            return true;
        } else {
            return false;
        }
    }
    bool InferTypes::isVisibleFunc(Module *inWhichModule, Module *objModule) const {
        if (inWhichModule->isVisibleDirectly(objModule))
            return true;
        // can always call within same module from instanced generic
        if (func && func->fromGeneric) {
            auto inWhichOtherModule = func->getOrigin()->module;
            if (inWhichOtherModule->isVisibleDirectly(objModule))
                return true;
        }
        return false;
    }
    MatchingFunctions InferTypes::findFuncAddr(const string &name) const {
        string moduleName, funcName;
        splitTypeName(name, moduleName, funcName);
        MatchingFunctions result;
        auto inWhichModule = getSearchModule(moduleName);
        auto hFuncName = hash64z(funcName.c_str());
        program->library.foreach ([&](Module *mod) -> bool {
                auto itFnList = mod->functionsByName.find(hFuncName);
                if ( itFnList ) {
                    auto & goodFunctions = itFnList->second;
                    for ( auto & pFn : goodFunctions ) {
                        // if ( pFn->isTemplate ) continue;
                        if ( isVisibleFunc(inWhichModule,getFunctionVisModule(pFn)) ) {
                            if ( canCallPrivate(pFn,inWhichModule,thisModule) ) {
                                result.push_back(pFn);
                            }
                        }
                    }
                }
                return true; }, moduleName);
        return result;
    }
    MatchingFunctions InferTypes::findTypedFuncAddr(string &name, const vector<TypeDeclPtr> &arguments) {
        MatchingFunctions result = findMatchingFunctions(name, arguments, false, false);
        if (result.size() == 0) {
            auto fakeCall = make_smart<ExprCall>(LineInfo(), name);
            for (auto &arg : arguments) {
                // TODO: support blocks?
                auto fakeArg = make_smart<ExprTypeDecl>(LineInfo(), arg);
                fakeArg->type = make_smart<TypeDecl>(*arg);
                fakeCall->arguments.push_back(fakeArg);
            }
            auto fn = inferFunctionCall(fakeCall.get());
            if (fakeCall->name != name) {
                name = fakeCall->name;
                result = findMatchingFunctions(fakeCall->name, arguments, false, false);
            } else if (fn) {
                result.push_back(fn.get());
            }
        }
        return result;
    }
    bool InferTypes::isOperator(const string &s) const {
        for (auto ch : s) {
            if (ch >= '0' && ch <= '9')
                return false;
            else if (ch >= 'a' && ch <= 'z')
                return false;
            else if (ch >= 'A' && ch <= 'Z')
                return false;
            else if (ch == '_')
                return false;
        }
        return true;
    }
    bool InferTypes::isCloseEnoughName(const string &s, const string &t, bool identical) const {
        if (s == t)
            return true;
        if (identical)
            return false;
        auto ls = s.size();
        auto lt = t.size();
        if (ls - lt > 3 || lt - ls > 3)
            return false; // too much difference in length, no way its typo
        if (isOperator(s) || isOperator(t))
            return false;
        string upper_s, upper_t;
        upper_s.reserve(s.size());
        for (auto ch : s)
            upper_s.push_back((char)toupper(ch));
        upper_t.reserve(t.size());
        for (auto ch : t)
            upper_t.push_back((char)toupper(ch));
        if (upper_s == upper_t)
            return true;
        /*
        Length ≤ 5: Distance ≤ 1 is likely a typo.
        Length 6–10: Distance ≤ 2 is likely a typo.
        Length > 10: Distance ≤ 3 might still be a typo.
        */
        int longer = int(ls > lt ? ls : lt);
        int maxDist = 1;
        if (longer > 10)
            maxDist = 3;
        else if (longer > 5)
            maxDist = 2;
        auto dist = levenshtein_distance(upper_s.c_str(), upper_t.c_str());
        if (dist <= maxDist) {
            return true;
        } else {
            return false;
        }
    }
    MatchingFunctions InferTypes::findMissingCandidates(const string &name, bool identicalName) const {
        string moduleName, funcName;
        splitTypeName(name, moduleName, funcName);
        MatchingFunctions result;
        getSearchModule(moduleName);
        program->library.foreach ([&](Module *mod) -> bool {
                mod->functions.foreach([&](const FunctionPtr & fn) -> bool {
                    if ( fn->isTemplate ) return true;
                    if ( isCloseEnoughName(fn->name,funcName,identicalName) ) {
                        isCloseEnoughName(fn->name,funcName,identicalName);
                        result.push_back(fn.get());
                    }
                    return true;
                });
                return true; }, moduleName);
        return result;
    }
    MatchingFunctions InferTypes::findMissingGenericCandidates(const string &name, bool identicalName) const {
        // TODO: better error reporting
        string moduleName, funcName;
        splitTypeName(name, moduleName, funcName);
        MatchingFunctions result;
        getSearchModule(moduleName);
        program->library.foreach ([&](Module *mod) -> bool {
                mod->generics.foreach([&](const FunctionPtr & fn) -> bool {
                    if ( isCloseEnoughName(fn->name,funcName,identicalName) ) {
                        result.push_back(fn.get());
                    }
                    return true;
                });
                return true; }, moduleName);
        return result;
    }
    bool InferTypes::isMatchingArgument(const FunctionPtr &pFn, TypeDeclPtr argType, TypeDeclPtr passType, bool inferAuto, bool inferBlock, AliasMap *aliases, OptionsMap *options) const {
        if (!passType) {
            return false;
        }
        if (argType->explicitConst && (argType->constant != passType->constant)) { // explicit const mast match
            return false;
        }
        if (argType->explicitRef && (argType->ref != passType->ref)) { // explicit ref match
            return false;
        }
        if (argType->baseType == Type::anyArgument) {
            return true;
        }
        if (inferAuto) {
            // if it's an alias, we de'alias it, and see if it matches at all
            if (argType->isAlias()) {
                argType = inferAlias(argType, pFn, aliases);
                if (!argType) {
                    return false;
                }
            }
            // match auto argument
            if (argType->isAuto()) {
                return TypeDecl::inferGenericType(argType, passType, true, true, options) != nullptr;
            }
        }
        // match inferable block
        if (inferBlock && passType->isAuto() &&
            (passType->isGoodBlockType() || passType->isGoodLambdaType() || passType->isGoodFunctionType() || passType->isGoodArrayType() || passType->isGoodTableType())) {
            return TypeDecl::inferGenericType(passType, argType, true, true, options) != nullptr;
        }
        // compare types which don't need inference
        auto tempMatters = argType->implicit ? TemporaryMatters::no : TemporaryMatters::yes;
        if (!argType->isSameType(*passType, RefMatters::no, ConstMatters::no, tempMatters, AllowSubstitute::yes, true, true)) {
            return false;
        }
        // can't pass non-ref to ref
        if (argType->isRef() && !passType->isRef()) {
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
    bool InferTypes::isFunctionCompatible(Function *pFn, const vector<TypeDeclPtr> &types, bool inferAuto, bool inferBlock, bool checkLastArgumentsInit) const {
        if (pFn->arguments.size() < types.size()) {
            return false;
        }
        if (inferAuto && inferBlock) {
            AliasMap aliases;
            program->updateAliasMapCallback = [&](const TypeDeclPtr &argType, const TypeDeclPtr &passType) {
                OptionsMap options;
                TypeDecl::updateAliasMap(argType, passType, aliases, options);
            };
            for (;;) {
                bool anyFailed = false;
                auto totalAliases = aliases.size();
                for (size_t ai = 0, ais = types.size(); ai != ais; ++ai) {
                    auto argType = pFn->arguments[ai]->type;
                    auto passType = types[ai];
                    if (argType->isAlias()) {
                        argType = inferPartialAliases(argType, passType, pFn, &aliases);
                    }
                    OptionsMap options;
                    if (!isMatchingArgument(pFn, argType, passType, inferAuto, inferBlock, &aliases, &options)) {
                        anyFailed = true;
                        continue;
                    }
                    TypeDecl::updateAliasMap(argType, passType, aliases, options);
                }
                if (!anyFailed) {
                    break;
                }
                if (totalAliases == aliases.size()) {
                    return false;
                }
            }
            // clear callback
            program->updateAliasMapCallback = nullptr;
        } else {
            for (size_t ai = 0, ais = types.size(); ai != ais; ++ai) {
                if (!isMatchingArgument(pFn, pFn->arguments[ai]->type, types[ai], inferAuto, inferBlock)) {
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
        for (const auto &ann : pFn->annotations) {
            auto fnAnn = static_pointer_cast<FunctionAnnotation>(ann->annotation);
            if (fnAnn->isSpecialized()) {
                string err;
                if (!fnAnn->isCompatible(pFn, types, *ann, err)) {
                    return false;
                }
            }
        }
        return true;
    }
    bool InferTypes::isFunctionCompatible(Function *pFn, const vector<TypeDeclPtr> &nonNamedTypes, const vector<MakeFieldDeclPtr> &arguments, bool inferAuto, bool inferBlock) const {
        if ((pFn->arguments.size() < arguments.size()) || (pFn->arguments.size() < nonNamedTypes.size())) {
            return false;
        }

        if (!isFunctionCompatible(pFn, nonNamedTypes, inferAuto, inferBlock, false)) {
            return false;
        }

        size_t fnArgIndex = 0;
        for (size_t ai = 0, ais = arguments.size(); ai != ais; ++ai) {
            auto &arg = arguments[ai];
            for (;;) {
                if (fnArgIndex >= pFn->arguments.size()) { // out of source arguments. done
                    return false;
                }
                auto &fnArg = pFn->arguments[fnArgIndex];
                if (fnArg->name == arg->name) { // found it, name matches
                    break;
                }

                if (!fnArg->init && fnArgIndex >= nonNamedTypes.size()) { // can't skip - no default value
                    return false;
                }
                fnArgIndex++;
            }
            if (!isMatchingArgument(pFn, pFn->arguments[fnArgIndex]->type, arg->value->type, inferAuto, inferBlock)) {
                return false;
            }
            fnArgIndex++;
        }
        while (fnArgIndex < pFn->arguments.size()) {
            if (!pFn->arguments[fnArgIndex]->init) {
                return false; // tail must be defaults
            }
            fnArgIndex++;
        }
        return true;
    }
    Function *InferTypes::findMethodFunction(Structure *st, const string &name) const {
        if (name.find("::") != string::npos) {
            return nullptr;
        }
        auto field = st->findField(name);
        if (!field) {
            return nullptr;
        }
        if (!field->classMethod) {
            return nullptr;
        }
        auto addr = field->init;
        if (addr->rtti_isCast()) {
            addr = static_pointer_cast<ExprCast>(addr)->subexpr;
        }
        if (!addr->rtti_isAddr()) {
            return nullptr;
        }
        auto pAddr = static_pointer_cast<ExprAddr>(addr);
        if (!pAddr->func) {
            return nullptr;
        }
        return pAddr->func;
    }
    bool InferTypes::hasMatchingMemberCall(Structure *st, const string &name, const vector<MakeFieldDeclPtr> &arguments, const vector<TypeDeclPtr> &nonNamedArguments, bool methodCall) const {
        auto methodFunc = findMethodFunction(st, name);
        if (!methodFunc) {
            return false;
        }
        vector<TypeDeclPtr> nna = nonNamedArguments;
        if (!methodCall) {
            nna.insert(nna.begin(), make_smart<TypeDecl>(st));
        }
        return isFunctionCompatible(methodFunc, nna, arguments, false, false);
    }
    MatchingFunctions InferTypes::findMatchingFunctions(const string &name, const vector<TypeDeclPtr> &types, const vector<MakeFieldDeclPtr> &arguments, bool inferBlock) const {
        string moduleName, funcName;
        splitTypeName(name, moduleName, funcName);
        auto inWhichModule = getSearchModule(moduleName);
        return findMatchingFunctions(moduleName, inWhichModule, funcName, types, arguments, inferBlock);
    }
    MatchingFunctions InferTypes::findMatchingFunctions(const string &moduleName, Module *inWhichModule, const string &funcName, const vector<TypeDeclPtr> &types, const vector<MakeFieldDeclPtr> &arguments, bool inferBlock) const {
        MatchingFunctions result;
        auto hFuncName = hash64z(funcName.c_str());
        program->library.foreach ([&](Module *mod) -> bool {
                auto itFnList = mod->functionsByName.find(hFuncName);
                if ( itFnList ) {
                    auto & goodFunctions = itFnList->second;
                    for ( auto & pFn : goodFunctions ) {
                        if ( pFn->isTemplate ) continue;
                        if ( isVisibleFunc(inWhichModule,getFunctionVisModule(pFn)) ) {
                            if ( !pFn->fromGeneric || thisModule->isVisibleDirectly(mod) ) {
                                if ( canCallPrivate(pFn,inWhichModule,thisModule) ) {
                                    if ( isFunctionCompatible(pFn, types, arguments, false, inferBlock) ) {
                                        result.push_back(pFn);
                                    }
                                }
                            }
                        }
                    }
                }
                return true; }, moduleName);
        return result;
    }
    uint64_t InferTypes::getLookupHash(const vector<TypeDeclPtr> &types) const {
        uint64_t argHash = UINT64_C(14695981039346656037);
        for (auto &arg : types) {
            arg->getLookupHash(argHash);
        }
        return argHash ? argHash : UINT64_C(14695981039346656037);
    }
    MatchingFunctions InferTypes::findMatchingFunctions(const string &name, const vector<TypeDeclPtr> &types, bool inferBlock, bool visCheck) const {
        string moduleName, funcName;
        splitTypeName(name, moduleName, funcName);
        auto inWhichModule = getSearchModule(moduleName);
        return findMatchingFunctions(moduleName, inWhichModule, funcName, types, inferBlock, visCheck);
    }
    MatchingFunctions InferTypes::findMatchingFunctions(const string &moduleName, Module *inWhichModule, const string &funcName, const vector<TypeDeclPtr> &types, bool inferBlock, bool visCheck) const {
        MatchingFunctions result;
        auto hFuncName = hash64z(funcName.c_str());
        uint64_t argHash = 0;
        program->library.foreach ([&](Module *mod) -> bool {
                auto itFnList = mod->functionsByName.find(hFuncName);
                if ( itFnList ) {
                    auto & goodFunctions = itFnList->second;
                    for ( auto & pFn : goodFunctions ) {
                        if ( pFn->jitOnly && !jitEnabled() ) continue;
                        if ( pFn->isTemplate ) continue;
                        if ( !visCheck || isVisibleFunc(inWhichModule,getFunctionVisModule(pFn) ) ) {
                            if ( !pFn->fromGeneric || thisModule->isVisibleDirectly(mod) ) {
                                if ( canCallPrivate(pFn,inWhichModule,thisModule) ) {
                                    if ( !argHash ) {
                                        argHash = fragile_bit_set::key(getLookupHash(types));
                                    }
                                    auto itLook = pFn->lookup.find_and_reserve(argHash);    // if found in lookup
                                    if ( *itLook ) {
                                        if ( fragile_bit_set::is_true(*itLook) ) {
                                            result.push_back(pFn);
                                        }
                                        continue;
                                    }
                                    if ( isFunctionCompatible(pFn, types, false, inferBlock) ) {
                                        result.push_back(pFn);
                                        *itLook = fragile_bit_set::set_true(argHash);
                                    } else {
                                        *itLook = fragile_bit_set::set_false(argHash);
                                    }
                                }
                            }
                        }
                    }
                }
                return true; }, moduleName);
        return result;
    }
    void InferTypes::findMatchingFunctionsAndGenerics(MatchingFunctions &resultFunctions, MatchingFunctions &resultGenerics, const string &name, const vector<TypeDeclPtr> &types, const vector<MakeFieldDeclPtr> &arguments, bool inferBlock) const {
        string moduleName, funcName;
        splitTypeName(name, moduleName, funcName);
        auto inWhichModule = getSearchModule(moduleName);
        auto hFuncName = hash64z(funcName.c_str());
        program->library.foreach ([&](Module *mod) -> bool {
                {   // functions
                    auto itFnList = mod->functionsByName.find(hFuncName);
                    if ( itFnList ) {
                        auto & goodFunctions = itFnList->second;
                        for ( auto & pFn : goodFunctions ) {
                            if ( pFn->isTemplate ) continue;
                            if ( isVisibleFunc(inWhichModule,getFunctionVisModule(pFn)) ) {
                                if ( !pFn->fromGeneric || thisModule->isVisibleDirectly(mod) ) {
                                    if ( canCallPrivate(pFn,inWhichModule,thisModule) ) {
                                        if ( isFunctionCompatible(pFn, types, arguments, false, inferBlock) ) {
                                            resultFunctions.push_back(pFn);
                                        }
                                    }
                                }
                            }
                        }
                    }
                }
                {   // generics
                    auto itFnList = mod->genericsByName.find(hFuncName);
                    if ( itFnList ) {
                        auto & goodFunctions = itFnList->second;
                        for ( auto & pFn : goodFunctions ) {
                            if ( pFn->jitOnly && !jitEnabled() ) continue;
                            if ( pFn->isTemplate ) continue;
                            if ( isVisibleFunc(inWhichModule,getFunctionVisModule(pFn)) ) {
                                if ( canCallPrivate(pFn,inWhichModule,thisModule) ) {
                                    if ( isFunctionCompatible(pFn, types, arguments, true, true) ) {   // infer block here?
                                        resultGenerics.push_back(pFn);
                                    }
                                }
                            }
                        }
                    }
                }
                return true; }, moduleName);
    }
    void InferTypes::findMatchingFunctionsAndGenerics(MatchingFunctions &resultFunctions, MatchingFunctions &resultGenerics, const string &name, const vector<TypeDeclPtr> &types, bool inferBlock, bool visCheck) const {
        string moduleName, funcName;
        splitTypeName(name, moduleName, funcName);
        auto inWhichModule = getSearchModule(moduleName);
        auto hFuncName = hash64z(funcName.c_str());
        uint64_t argHash = fragile_bit_set::key(getLookupHash(types));
        program->library.foreach ([&](Module *mod) -> bool {
                { // functions
                    auto itFnList = mod->functionsByName.find(hFuncName);
                    if ( itFnList ) {
                        auto & goodFunctions = itFnList->second;
                        for ( auto & pFn : goodFunctions ) {
                            if ( pFn->jitOnly && !jitEnabled() ) continue;
                            if ( pFn->isTemplate ) continue;
                            if ( !visCheck || isVisibleFunc(inWhichModule,getFunctionVisModule(pFn) ) ) {
                                if ( !pFn->fromGeneric || thisModule->isVisibleDirectly(mod) ) {
                                    if ( !visCheck || canCallPrivate(pFn,inWhichModule,thisModule) ) {
                                        auto itLook = pFn->lookup.find_and_reserve(argHash);    // if found in lookup
                                        if ( *itLook ) {
                                            if ( fragile_bit_set::is_true(*itLook) ) {
                                                resultFunctions.push_back(pFn);
                                            }
                                            continue;
                                        }
                                        if ( isFunctionCompatible(pFn, types, false, inferBlock) ) {
                                            resultFunctions.push_back(pFn);
                                            *itLook = fragile_bit_set::set_true(argHash);
                                        } else {
                                            *itLook = fragile_bit_set::set_false(argHash);
                                        }
                                    }
                                }
                            }
                        }
                    }
                }
                { // generics
                    auto itFnList = mod->genericsByName.find(hFuncName);
                    if ( itFnList ) {
                        auto & goodFunctions = itFnList->second;
                        for ( auto & pFn : goodFunctions ) {
                            if ( pFn->isTemplate ) continue;
                            if ( !visCheck || isVisibleFunc(inWhichModule,getFunctionVisModule(pFn)) ) {
                                if ( !visCheck || canCallPrivate(pFn,inWhichModule,thisModule) ) {
                                    auto itLook = pFn->lookup.find_and_reserve(argHash);    // if found in lookup
                                    if ( *itLook ) {
                                        if ( fragile_bit_set::is_true(*itLook) ) {
                                            resultGenerics.push_back(pFn);
                                        }
                                        continue;
                                    }
                                    if ( isFunctionCompatible(pFn, types, true, true) ) {   // infer block here?
                                        resultGenerics.push_back(pFn);
                                        *itLook = fragile_bit_set::set_true(argHash);
                                    } else {
                                        *itLook = fragile_bit_set::set_false(argHash);
                                    }
                                }
                            }
                        }
                    }
                }
                return true; }, moduleName);
    }
    MatchingFunctions InferTypes::findDefaultConstructor(const string &sna) const {
        vector<TypeDeclPtr> argDummy;
        return findMatchingFunctions(thisModule->name, thisModule, sna, argDummy); // "__::sna"
    }
    bool InferTypes::hasDefaultUserConstructor(const string &sna) const {
        vector<TypeDeclPtr> argDummy;
        auto fnlist = findDefaultConstructor(sna);
        for (auto &fn : fnlist) {
            if (fn->arguments.size() == 0) {
                return true;
            } else {
                bool allDefault = true;
                for (auto &arg : fn->arguments) {
                    if (!arg->init) {
                        allDefault = false;
                        break;
                    }
                }
                if (allDefault) {
                    return true;
                }
            }
        }
        return false;
    }
    bool InferTypes::verifyAnyFunc(const MatchingFunctions &fnList, const LineInfo &at) const {
        int genCount = 0;
        int customCount = 0;
        for (auto &fn : fnList) {
            if (!fn->generated) {
                customCount++;
            } else {
                genCount++;
            }
        }
        if (customCount && genCount) {
            if (verbose) {
                string candidates = program->describeCandidates(fnList);
                error("both generated and custom '" + fnList.front()->name + "' functions exist for " + describeFunction(fnList.front()), candidates, "",
                      at, CompilationError::function_not_found);
            } else {
                error("both generated and custom '" + fnList.front()->name + "' functions exist", "", "",
                      at, CompilationError::function_not_found);
            }
            return false;
        } else if (customCount > 1) {
            if (verbose) {
                string candidates = program->describeCandidates(fnList);
                error("too many custom '" + fnList.front()->name + "' functions exist for " + describeFunction(fnList.front()), candidates, "",
                      at, CompilationError::function_not_found);
            } else {
                error("too many custom '" + fnList.front()->name + "' functions exist", "", "",
                      at, CompilationError::function_not_found);
            }
            return false;
        } else {
            return true;
        }
    }
    MatchingFunctions InferTypes::getCloneFunc(const TypeDeclPtr &left, const TypeDeclPtr &right) const {
        vector<TypeDeclPtr> argDummy = {left, right};
        auto clones = findMatchingFunctions("*", thisModule, "clone", argDummy); // "_::clone"
        applyLSP(argDummy, clones);
        return clones;
    }
    bool InferTypes::verifyCloneFunc(const MatchingFunctions &fnList, const LineInfo &at) const {
        return verifyAnyFunc(fnList, at);
    }
    MatchingFunctions InferTypes::getFinalizeFunc(const TypeDeclPtr &subexpr) const {
        vector<TypeDeclPtr> argDummy = {subexpr};
        auto fins = findMatchingFunctions("*", thisModule, "finalize", argDummy); // "_::finalize"
        applyLSP(argDummy, fins);
        return fins;
    }
    bool InferTypes::verifyFinalizeFunc(const MatchingFunctions &fnList, const LineInfo &at) const {
        return verifyAnyFunc(fnList, at);
    }
    vector<ExpressionPtr> InferTypes::demoteCallArguments(ExprNamedCall *expr, const FunctionPtr &pFn) {
        vector<ExpressionPtr> newCallArguments;
        size_t fnArgIndex = 0;
        for (size_t ai = 0, ais = expr->arguments.size(); ai != ais; ++ai) {
            auto &arg = expr->arguments[ai];
            for (;;) {
                DAS_ASSERTF(fnArgIndex < pFn->arguments.size(), "somehow matched function which does not match. not enough args");
                auto &fnArg = pFn->arguments[fnArgIndex];
                if (fnArg->name == arg->name) {
                    break;
                }

                if (fnArgIndex < expr->nonNamedArguments.size()) {
                    newCallArguments.push_back(expr->nonNamedArguments[fnArgIndex]->clone());
                } else {
                    DAS_ASSERTF(fnArg->init, "somehow matched function, which does not match. can only skip defaults");
                    newCallArguments.push_back(fnArg->init->clone());
                }
                fnArgIndex++;
            }
            newCallArguments.push_back(arg->value->clone());
            fnArgIndex++;
        }
        while (fnArgIndex < pFn->arguments.size()) {
            auto &fnArg = pFn->arguments[fnArgIndex];
            DAS_ASSERTF(fnArg->init, "somehow matched function, which does not match. tail has to be defaults");
            newCallArguments.push_back(fnArg->init->clone());
            fnArgIndex++;
        }
        return newCallArguments;
    }
    ExpressionPtr InferTypes::demoteCall(ExprNamedCall *expr, const FunctionPtr &pFn) {
        auto newCall = make_smart<ExprCall>(expr->at, pFn->name);
        newCall->arguments = demoteCallArguments(expr, pFn);
        return newCall;
    }
    bool InferTypes::isConsumeArgumentFunc(Function *fn) {
        if (fn->fromGeneric && fn->fromGeneric->module->name == "builtin" && fn->fromGeneric->name == "consume_argument") {
            return true;
        }
        return false;
    }
    bool InferTypes::isConsumeArgumentCall(Expression *arg) {
        if (arg->rtti_isCall()) {
            auto argCall = (ExprCall *)arg;
            if (argCall->func && isConsumeArgumentFunc(argCall->func))
                return true;
        }
        return false;
    }
    string InferTypes::getGenericInstanceName(const Function *fn) const {
        string name;
        if (fn->module) {
            if (fn->module->name == "$") {
                name += "builtin";
            } else {
                name += fn->module->name;
            }
        }
        name += "`";
        auto fCh = fn->name[0];
        if (!(isalnum(fCh) || fCh == '_' || fCh == '`')) {
            name += "`operator";
        }
        name += fn->name;
        string newName;
        newName.reserve(fn->name.length());
        for (auto &ch : name) {
            if (isalnum(ch) || ch == '_' || ch == '`') {
                newName.append(1, ch);
            } else {
                switch (ch) {
                case '=':
                    newName += "`eq";
                    break;
                case '+':
                    newName += "`add";
                    break;
                case '-':
                    newName += "`sub";
                    break;
                case '*':
                    newName += "`mul";
                    break;
                case '/':
                    newName += "`div";
                    break;
                case '%':
                    newName += "`mod";
                    break;
                case '<':
                    newName += "`less";
                    break;
                case '>':
                    newName += "`gt";
                    break;
                case '!':
                    newName += "`not";
                    break;
                case '~':
                    newName += "`bnot";
                    break;
                case '&':
                    newName += "`and";
                    break;
                case '|':
                    newName += "`or";
                    break;
                case '^':
                    newName += "`xor";
                    break;
                case '?':
                    newName += "`qmark";
                    break;
                case '@':
                    newName += "`at";
                    break;
                case ':':
                    newName += "`col";
                    break;
                case '.':
                    newName += "`dot";
                    break;
                case '[':
                    newName += "`lsq";
                    break;
                case ']':
                    newName += "`rsq";
                    break;
                default:
                    newName += "_`_";
                    break;
                }
            }
        }
        newName += "`";
        newName += to_string(fn->getMangledNameHash());
        return newName;
    }
    string InferTypes::callCloneName(const string &name) {
        return "__::" + name;
    }
    int InferTypes::computeSubstituteDistance(const vector<TypeDeclPtr> &arguments, const FunctionPtr &fn) {
        int distance = 0;
        for (size_t i = 0, is = arguments.size(); i != is; ++i) {
            const auto &argType = arguments[i];
            const auto &funType = fn->arguments[i]->type;
            if (!argType->isSameType(*funType, RefMatters::no, ConstMatters::no,
                                     TemporaryMatters::no, AllowSubstitute::no)) {
                distance++;
            }
        }
        return distance;
    }
    int InferTypes::moreSpecialized(const TypeDeclPtr &t1, const TypeDeclPtr &t2, TypeDeclPtr &passType) {
        // 0. deal with options
        bool opt1 = t1->baseType == Type::option;
        bool opt2 = t2->baseType == Type::option;
        if (opt1 || opt2) {
            int numS[3] = {0, 0, 0};
            if (opt1 && opt2) {
                for (auto &arg1 : t1->argTypes) {
                    for (auto &arg2 : t2->argTypes) {
                        auto res = moreSpecialized(arg1, arg2, passType);
                        numS[res + 1]++;
                    }
                }
            } else if (opt1) {
                for (auto &arg1 : t1->argTypes) {
                    auto res = moreSpecialized(arg1, t2, passType);
                    numS[res + 1]++;
                }
            } else if (opt2) {
                for (auto &arg2 : t2->argTypes) {
                    auto res = moreSpecialized(t1, arg2, passType);
                    numS[res + 1]++;
                }
            } else {
                DAS_VERIFY(0);
            }
            int total = (numS[0] ? 1 : 0) + (numS[1] ? 1 : 0) + (numS[2] ? 1 : 0);
            if (total == 1) {
                if (numS[0])
                    return -1;
                else if (numS[1])
                    return 0;
                else
                    return 1;
            } else {
                return 0;
            }
        }
        // 1. non auto is more specialized than auto
        bool a1 = t1->isAutoOrAlias();
        bool a2 = t2->isAutoOrAlias();
        if (a1 != a2) { // if one is auto
            return a1 ? -1 : 1;
        }
        // 2. if both non-auto, the one without cast is more specialized
        if (!a1 && !a2) { // if both solid types
            bool s1 = passType->isSameType(*t1, RefMatters::no, ConstMatters::no, TemporaryMatters::no, AllowSubstitute::no);
            bool s2 = passType->isSameType(*t2, RefMatters::no, ConstMatters::no, TemporaryMatters::no, AllowSubstitute::no);
            if (s1 != s2) {
                return s1 ? 1 : -1;
            } else {
                return 0;
            }
        }
        // at this point we are dealing with 2 auto types
        // 3. one with dim is more specialized, than one without
        //      if both have dim, one with actual value is more specialized, than the other one
        {
            int d1 = t1->dim.size() ? t1->dim[0] : 0;
            int d2 = t2->dim.size() ? t2->dim[0] : 0;
            if (d1 != d2) {
                if (d1 && d2) {
                    return d1 == -1 ? -1 : 1;
                } else {
                    return d1 ? 1 : -1;
                }
            }
        }
        // 4. the one with base type of auto\alias is less specialized
        //      if both are auto\alias - we assume its the same level of specialization
        bool ba1 = t1->baseType == Type::autoinfer || t1->baseType == Type::alias;
        bool ba2 = t2->baseType == Type::autoinfer || t2->baseType == Type::alias;
        if (ba1 != ba2) {
            return ba1 ? -1 : 1;
        } else if (ba1 && ba2) {
            return 0;
        }
        // 5. if both are typemacros, we need to pick the more specialized one
        if (t1->baseType == Type::typeMacro && t2->baseType == Type::typeMacro) {
            // the one with more arguments wins
            size_t d1 = t1->dimExpr.size();
            size_t d2 = t2->dimExpr.size();
            if (d1 != d2) {
                return d1 < d2 ? -1 : 1;
            }
            // we go through argument, and for the type<...> compare specialization
            bool less = false;
            bool more = false;
            for (size_t d = 0; d != d1; ++d) {
                TypeDeclPtr t1Arg, t2Arg;
                if (t1->dimExpr[d]->rtti_isTypeDecl()) {
                    t1Arg = static_pointer_cast<ExprTypeDecl>(t1->dimExpr[d])->typeexpr;
                }
                if (t2->dimExpr[d]->rtti_isTypeDecl()) {
                    t2Arg = static_pointer_cast<ExprTypeDecl>(t2->dimExpr[d])->typeexpr;
                }
                if (t1Arg && t2Arg) {
                    // only if both are types, we can compare
                    int cmpr = moreSpecialized(t1Arg, t2Arg, passType);
                    if (cmpr < 0)
                        less = true;
                    else if (cmpr > 0)
                        more = true;
                }
            }
            if (less && more)
                return 0;
            else if (less)
                return -1;
            else if (more)
                return 1;
            else
                return 0;
        }
        // at this point base type is not auto for both, so lets compare the subtypes
        // if either does not match the base type, we arrive here through wrong option
        if (t1->baseType != passType->baseType || t2->baseType != passType->baseType) {
            return 0;
        }
        //    DAS_ASSERT(t1->baseType==passType->baseType && "how did it match otherwise?");
        //    DAS_ASSERT(t2->baseType==passType->baseType && "how did it match otherwise?");

        // if its an array or a pointer, we compare specialization of subtype
        if (t1->baseType == Type::tPointer || t1->baseType == Type::tArray || t1->baseType == Type::tIterator) {
            return moreSpecialized(t1->firstType, t2->firstType, passType->firstType);
            // if its a table, we compare both subtypes
        } else if (t1->baseType == Type::tTable) {
            int i1 = moreSpecialized(t1->firstType, t2->firstType, passType->firstType);
            int i2 = moreSpecialized(t1->secondType, t2->secondType, passType->secondType);
            bool less = (i1 < 0) || (i2 < 0);
            bool more = (i1 > 0) || (i2 > 0);
            if (less && more)
                return 0;
            else if (less)
                return -1;
            else if (more)
                return 1;
            else
                return 0;
            // if its a tuple or a variant, we compare all subtypes
        } else if (t1->baseType == Type::tVariant || t1->baseType == Type::tTuple) {
            bool less = false;
            bool more = false;
            DAS_ASSERT(t1->argTypes.size() && passType->argTypes.size() && "how did it match otherwise?");
            DAS_ASSERT(t2->argTypes.size() && passType->argTypes.size() && "how did it match otherwise?");
            for (size_t aI = 0, aIs = t1->argTypes.size(); aI != aIs; ++aI) {
                int cmpr = moreSpecialized(
                    t1->argTypes[aI],
                    t2->argTypes[aI],
                    passType->argTypes[aI]);
                if (cmpr < 0)
                    less = true;
                else if (cmpr > 0)
                    more = true;
            }
            if (less && more)
                return 0;
            else if (less)
                return -1;
            else if (more)
                return 1;
            else
                return 0;
            // if its a block, a function, or a lambda, we compare all subtypes and firstType (result)
        } else if (t1->baseType == Type::tBlock || t1->baseType == Type::tLambda || t1->baseType == Type::tFunction) {
            bool less = false;
            bool more = false;
            int cmpr = moreSpecialized(t1->firstType, t2->firstType, passType->firstType);
            if (cmpr < 0)
                less = true;
            else if (cmpr > 0)
                more = true;
            DAS_ASSERT(t1->argTypes.size() && passType->argTypes.size() && "how did it match otherwise?");
            DAS_ASSERT(t2->argTypes.size() && passType->argTypes.size() && "how did it match otherwise?");
            for (size_t aI = 0, aIs = t1->argTypes.size(); aI != aIs; ++aI) {
                cmpr = moreSpecialized(
                    t1->argTypes[aI],
                    t2->argTypes[aI],
                    passType->argTypes[aI]);
                if (cmpr < 0)
                    less = true;
                else if (cmpr > 0)
                    more = true;
            }
            if (less && more)
                return 0;
            else if (less)
                return -1;
            else if (more)
                return 1;
            else
                return 0;
        }
        return 0;
    }
    bool InferTypes::copmareFunctionSpecialization(const FunctionPtr &f1, const FunctionPtr &f2, ExprLooksLikeCall *expr) {
        size_t nArgs = expr->arguments.size();
        bool less = false;
        bool more = false;
        for (size_t aI = 0; aI != nArgs; ++aI) {
            const auto &f1A = f1->arguments[aI]->type;
            const auto &f2A = f2->arguments[aI]->type;
            int cmpr = moreSpecialized(f1A, f2A, expr->arguments[aI]->type);
            if (cmpr < 0)
                less = true;
            else if (cmpr > 0)
                more = true;
        }
        if (!more && !less) {
            // if functions are identical, the one with more specialization annotations win
            int spF1 = 0;
            for (auto &ann : f1->annotations) {
                auto fnA = static_pointer_cast<FunctionAnnotation>(ann->annotation);
                if (fnA->isSpecialized())
                    spF1++;
            }
            int spF2 = 0;
            for (auto &ann : f2->annotations) {
                auto fnA = static_pointer_cast<FunctionAnnotation>(ann->annotation);
                if (fnA->isSpecialized())
                    spF2++;
            }
            if (spF1 > spF2)
                more = true;
        }
        if (more && !less) {
            return true;
        } else {
            return false;
        }
    }
    void InferTypes::applyLSP(const vector<TypeDeclPtr> &arguments, MatchingFunctions &functions) {
        if (functions.size() <= 1)
            return;
        vector<pair<int, Function *>> fnm;
        for (auto &fn : functions) {
            auto dist = computeSubstituteDistance(arguments, fn);
            fnm.push_back(make_pair(dist, fn));
        }
        sort(fnm.begin(), fnm.end(), [&](auto a, auto b) { return a.first < b.first; });
        int count = 1;
        int depth = fnm[0].first;
        while (count < int(fnm.size())) {
            if (fnm[count].first != depth)
                break;
            count++;
        }
        if (count == 1) {
            functions.resize(1);
            functions.front() = fnm[0].second;
        }
    }
    bool InferTypes::inferArguments(vector<TypeDeclPtr> &types, const vector<ExpressionPtr> &arguments) {
        types.reserve(arguments.size());
        for (auto &ar : arguments) {
            if (!ar->type) {
                return false;
            }
            if(ar->type->baseType==Type::tVoid) {
                error("void type is not allowed as argument", "", "", ar->at);
                return false;
            }
            DAS_ASSERT(!ar->type->isExprType() && "if this happens, we are calling infer function call without checking for '[expr]'. do that from where we call up the stack.");
            // if its an auto or an alias
            // we only allow it, if its a block or lambda
            if (ar->type->baseType != Type::tBlock && ar->type->baseType != Type::tLambda && ar->type->baseType != Type::tFunction && ar->type->baseType != Type::tArray && ar->type->baseType != Type::tTable) {
                if (ar->type->isAutoOrAlias()) {
                    return false;
                }
            }
            types.push_back(ar->type);
        }
        return true;
    }

    struct InferDepthGuard {
        InferDepthGuard(int32_t *depth) : depth(depth) { (*depth)++; }
        ~InferDepthGuard() { (*depth)--; }
        int32_t *depth;
    };

    FunctionPtr InferTypes::inferFunctionCall(ExprLooksLikeCall *expr, InferCallError cerr, Function *lookupFunction, bool failOnMissingCtor, bool visCheck) {
        if ( inferDepth > 1 ) {
            error("infer expression depth exceeded maximum allowed", "", "",
                expr->at, CompilationError::too_many_infer_passes);
            return nullptr;
        }
        InferDepthGuard guard(&inferDepth);
        vector<TypeDeclPtr> types;
        if (!inferArguments(types, expr->arguments)) {
            if (func)
                func->notInferred();
            return nullptr;
        }
        MatchingFunctions functions, generics;
        if (!lookupFunction) {
            findMatchingFunctionsAndGenerics(functions, generics, expr->name, types, true, visCheck);
            applyLSP(types, functions);
        } else {
            functions.push_back(lookupFunction);
        }
        if (functions.size() == 1) {
            auto funcC = functions.back();
            if ( inArgumentInit && funcC==func ) {
                error("recursive call in argument initializer is not allowed", "", "", expr->at);
                return nullptr;
            }
            if (funcC->result->baseType == Type::autoinfer) {
                if ( cerr != InferCallError::tryOperator ) {
                    error("cannot infer type for function call '" + expr->name + "' with 'auto' return type", "", "", expr->at, CompilationError::invalid_type);
                    return nullptr;
                }
            }
            if ( find(inInfer.begin(), inInfer.end(), funcC) != inInfer.end() ) {
                error("recursive call in function is not allowed", "", "", expr->at);
                return nullptr;
            }
            if (funcC->firstArgReturnType) {
                TypeDecl::clone(expr->type, expr->arguments[0]->type);
                expr->type->ref = false;
            } else {
                TypeDecl::clone(expr->type, funcC->result);
            }
            // infer FORWARD types
            for (size_t iF = 0, iFs = expr->arguments.size(); iF != iFs; ++iF) {
                auto &arg = expr->arguments[iF];
                if (arg->type->isAuto()) {
                    if (arg->type->isGoodBlockType() || arg->type->isGoodLambdaType() || arg->type->isGoodFunctionType()) {
                        if (arg->rtti_isMakeBlock()) { // "it's always MakeBlock. unless its function and @@funcName
                            auto mkBlock = static_pointer_cast<ExprMakeBlock>(arg);
                            auto block = static_pointer_cast<ExprBlock>(mkBlock->block);
                            auto retT = TypeDecl::inferGenericType(mkBlock->type, funcC->arguments[iF]->type, true, true, nullptr);
                            DAS_ASSERTF(retT, "how? it matched during findMatchingFunctions the same way");
                            TypeDecl::applyAutoContracts(mkBlock->type, funcC->arguments[iF]->type);
                            TypeDecl::clone(block->returnType, retT->firstType);
                            for (size_t ba = 0, bas = retT->argTypes.size(); ba != bas; ++ba) {
                                TypeDecl::clone(block->arguments[ba]->type, retT->argTypes[ba]);
                            }
                            setBlockCopyMoveFlags(block.get());
                            reportAstChanged();
                        }
                    } else if (arg->type->isGoodArrayType() || arg->type->isGoodTableType()) {
                        if (arg->rtti_isMakeStruct()) { // its always MakeStruct
                            auto mkStruct = static_pointer_cast<ExprMakeStruct>(arg);
                            if (mkStruct->structs.size()) {
                                error("internal compiler error: array<auto> type not under default<array<auto>> or default<table<auto;auto>>", "", "", expr->at);
                                return nullptr;
                            }
                            auto retT = TypeDecl::inferGenericType(mkStruct->type, funcC->arguments[iF]->type, true, true, nullptr);
                            DAS_ASSERTF(retT, "how? it matched during findMatchingFunctions the same way");
                            TypeDecl::applyAutoContracts(mkStruct->type, funcC->arguments[iF]->type);
                            mkStruct->makeType = retT;
                            reportAstChanged();
                        } else {
                            error("internal compiler error: unknown array<auto> type not under make strcut", "", "", expr->at);
                            return nullptr;
                        }
                    }
                }
            }
            // append default arguments
            for (size_t iT = expr->arguments.size(), iTs = funcC->arguments.size(); iT != iTs; ++iT) {
                auto newArg = funcC->arguments[iT]->init->clone();
                if (!newArg->type) {
                    // recursive resolve???
                    inInfer.push_back(funcC);
                    newArg = newArg->visit(*this);
                    inInfer.pop_back();
                }
                if (newArg->type && newArg->type->baseType == Type::fakeLineInfo) {
                    newArg->at = expr->at;
                }
                expr->arguments.push_back(newArg);
            }
            // dereference what needs dereferences
            for (size_t iA = 0, iAs = expr->arguments.size(); iA != iAs; ++iA)
                if (!funcC->arguments[iA]->type->isRef())
                    expr->arguments[iA] = Expression::autoDereference(expr->arguments[iA]);
            // and all good
            return funcC;
        } else if (functions.size() > 1) {
            if (cerr != InferCallError::tryOperator) {
                reportExcess(expr, types, "too many matching functions or generics ", functions, generics);
            }
        } else if (functions.size() == 0) {
            // if there is more than one, we pick more specialized
            if (generics.size() > 1) {
                stable_sort(generics.begin(), generics.end(), [&](const FunctionPtr &f1, const FunctionPtr &f2) { return copmareFunctionSpecialization(f1, f2, expr); });
                // if one is most specialized, we pick it, otherwise we report all of them
                if (copmareFunctionSpecialization(generics.front(), generics[1], expr)) {
                    generics.resize(1);
                }
            }
            if (generics.size() == 1) {
                auto oneGeneric = generics.back();
                auto genName = getGenericInstanceName(oneGeneric);
                auto instancedFunctions = findMatchingFunctions(program->thisModule->name, thisModule, genName, types, true); // "__::genName"
                if (instancedFunctions.size() > 1) {
                    TextWriter ss;
                    for (auto &instFn : instancedFunctions) {
                        ss << "\t" << describeFunction(instFn) << " in "
                           << (instFn->module->name.empty() ? "this module" : ("'" + instFn->module->name + "'"))
                           << "\n";
                    }
                    error("internal compiler error: multiple instances of '" + genName + "'", ss.str(), "", expr->at);
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
                    bool aliasMapUpdated = false;
                    program->updateAliasMapCallback = [&](const TypeDeclPtr &argType, const TypeDeclPtr &passType) {
                        OptionsMap options;
                        TypeDecl::updateAliasMap(argType, passType, aliases, options);
                        aliasMapUpdated = true;
                    };
                    vector<bool> defaultRef(types.size());
                    for (;;) {
                        bool anyFailed = false;
                        auto totalAliases = aliases.size();
                        for (size_t ai = 0, ais = types.size(); ai != ais; ++ai) {
                            auto argType = clone->arguments[ai]->type;
                            auto passType = types[ai];
                            if (argType->isAlias()) {
                                argType = inferPartialAliases(argType, passType, clone, &aliases);
                            }
                            OptionsMap options;
                            if (!isMatchingArgument(clone, argType, passType, true, true, &aliases, &options)) {
                                anyFailed = true;
                                continue;
                            }
                            TypeDecl::updateAliasMap(argType, passType, aliases, options);
                            if (argType->baseType == Type::option) {
                                auto it = options.find(argType.get());
                                if (it != options.end()) {
                                    auto optionType = argType->argTypes[it->second].get();
                                    defaultRef[ai] = optionType->ref;
                                }
                            } else {
                                defaultRef[ai] = argType->ref;
                            }
                        }
                        if (!anyFailed)
                            break;
                        if (totalAliases == aliases.size()) {
                            DAS_ASSERTF(0, "we should not be here. function matched arguments!");
                            break;
                        }
                    }
                    // now, we resolve types given inferred aliases
                    for (size_t sz = 0, szs = types.size(); sz != szs; ++sz) {
                        auto &argT = clone->arguments[sz]->type;
                        auto &passT = types[sz];
                        if (argT->isAlias()) {
                            argT = inferPartialAliases(argT, passT, clone, &aliases);
                        }
                        bool appendHasOptions = false;
                        bool isAutoWto = argT->isAutoWithoutOptions(appendHasOptions);
                        if (isAutoWto || appendHasOptions) {
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
                    for (size_t ai = types.size(), ais = clone->arguments.size(); ai != ais; ++ai) {
                        auto &arg = clone->arguments[ai];
                        if (arg->type->isAutoOrAlias()) {
                            if (arg->init) {
                                arg->init = arg->init->visit(*this);
                                if (arg->init->type && !arg->init->type->isAutoOrAlias()) {
                                    arg->type = make_smart<TypeDecl>(*arg->init->type);
                                    continue;
                                }
                            }
                            auto argT = inferPartialAliases(arg->type, arg->type, clone, &aliases);
                            if ( !argT->isAutoOrAlias() ) {
                                arg->type = argT;
                                continue;
                            }
                            error("unknown type of argument " + clone->arguments[ai]->name + "; can't instance " + describeFunction(oneGeneric), "",
                                  "provide argument type explicitly",
                                  expr->at, CompilationError::invalid_type);
                            return nullptr;
                        }
                    }
                    // clear callback
                    program->updateAliasMapCallback = nullptr;
                    // if we updated alias map (via typemacro), we need to reapply it to the result
                    if (aliasMapUpdated) {
                        for (auto &arg : clone->arguments) {
                            if (arg->type->isAlias()) {
                                arg->type = inferPartialAliases(arg->type, arg->type, clone, &aliases);
                            }
                        }
                        if (clone->result && clone->result->isAlias()) {
                            clone->result = inferPartialAliases(clone->result, clone->result, clone, &aliases);
                        }
                        // now, we go through alias map - and see, which ones are 'lost' ie no longer part of argument or results
                        for (auto it : aliases) {
                            bool found = false;
                            for (auto &arg : clone->arguments) {
                                if (arg->type->findAlias(it.first)) {
                                    found = true;
                                    break;
                                }
                            }
                            if (!found && clone->result) {
                                if (clone->result->findAlias(it.first)) {
                                    found = true;
                                }
                            }
                            if (!found) {
                                auto localAlias = make_smart<ExprAssume>(expr->at, it.first, it.second);
                                DAS_ASSERT(clone->body->rtti_isBlock());
                                auto blk = (ExprBlock *)clone->body.get();
                                blk->list.insert(blk->list.begin(), localAlias);
                            }
                        }
                    }
                    // now we verify if tail end can indeed be fully inferred
                    if (!program->addFunction(clone)) {
                        clone->module = thisModule;
                        auto exf = thisModule->functions.find(clone->getMangledName());
                        clone->module = nullptr;
                        DAS_ASSERTF(exf, "if we can't add, this means there is function with exactly this mangled name");
                        if (exf->fromGeneric != clone->fromGeneric) { // TODO: getOrigin??
                            error("can't instance generic " + describeFunction(clone),
                                  +"\ttrying to instance from module " + clone->fromGeneric->module->name + "\n" + "\texisting instance from module " + exf->fromGeneric->module->name, "",
                                  expr->at, CompilationError::function_already_declared);
                            return nullptr;
                        }
                    } else {
                        // perform generic_apply
                        for (auto &pA : clone->annotations) {
                            if (pA->annotation->rtti_isFunctionAnnotation()) {
                                auto ann = static_pointer_cast<FunctionAnnotation>(pA->annotation);
                                string err;
                                if (!ann->generic_apply(clone, *(program->thisModuleGroup), pA->arguments, err)) {
                                    error("Macro [" + pA->annotation->name + "] failed to generic_apply to a function " + clone->name + "\n",
                                          err, "", clone->at, CompilationError::invalid_annotation);
                                    return nullptr;
                                }
                            }
                        }
                    }
                    expr->name = callCloneName(clone->name);
                    reportAstChanged();
                }
            } else if (generics.size() > 1) {
                if (cerr != InferCallError::tryOperator) {
                    copmareFunctionSpecialization(generics.front(), generics[1], expr);
                    reportExcess(expr, types, "too many matching functions or generics ", functions, generics);
                }
            } else {
                if (auto aliasT = findAlias(expr->name)) {
                    if (aliasT->isCtorType()) {
                        expr->name = das_to_string(aliasT->baseType);
                        if (aliasT->baseType == Type::tBitfield || aliasT->baseType == Type::tBitfield8 ||
                            aliasT->baseType == Type::tBitfield16 || aliasT->baseType == Type::tBitfield64) {
                            expr->aliasSubstitution = aliasT;
                        }
                        reportAstChanged();
                    } else if (aliasT->isStructure()) {
                        if (expr->arguments.size() == 0) {
                            expr->name = aliasT->structType->name;
                            bool isPrivate = aliasT->structType->privateStructure;
                            if (isPrivate && aliasT->structType->module != thisModule) {
                                error("can't access private structure " + aliasT->structType->name, "", "",
                                      expr->at, CompilationError::function_not_found);
                            } else { // if ( !tryMakeStructureCtor (aliasT->structType, true, true) ) {
                                if (failOnMissingCtor) {
                                    error("default constructor " + aliasT->structType->name + " is not visible directly",
                                          "try default<" + expr->name + "> instead", "", expr->at, CompilationError::function_not_found);
                                }
                            }
                        } else {
                            error("can only generate default structure constructor without arguments",
                                  "", "", expr->at, CompilationError::invalid_argument_count);
                        }
                    } else {
                        if (cerr == InferCallError::operatorOp2) {
                            if (!reportOp2Errors(expr)) {
                                reportMissing(expr, types, "no matching functions or generics: ", true);
                            }
                        } else if (cerr != InferCallError::tryOperator) {
                            reportMissing(expr, types, "no matching functions or generics: ", true);
                        }
                    }
                } else {
                    if (cerr == InferCallError::operatorOp2) {
                        if (!reportOp2Errors(expr)) {
                            reportMissing(expr, types, "no matching functions or generics: ", true);
                        }
                    } else if (cerr != InferCallError::tryOperator) {
                        reportMissing(expr, types, "no matching functions or generics: ", true);
                    }
                }
            }
        }
        return nullptr;
    }
    ExpressionPtr InferTypes::inferGenericOperator3(const string &opN, const LineInfo &expr_at, const ExpressionPtr &arg0, const ExpressionPtr &arg1, const ExpressionPtr &arg2, InferCallError err) {
        auto opName = "_::" + opN;
        auto tempCall = make_smart<ExprLooksLikeCall>(expr_at, opName);
        tempCall->arguments.push_back(arg0);
        if (arg1)
            tempCall->arguments.push_back(arg1);
        if (arg2)
            tempCall->arguments.push_back(arg2);
        auto ffunc = inferFunctionCall(tempCall.get(), err).get();
        if (opName != tempCall->name) { // this happens when the operator gets instanced
            reportAstChanged();
            auto opCall = make_smart<ExprCall>(expr_at, tempCall->name);
            opCall->arguments = das::move(tempCall->arguments);
            return opCall;
        } else if (ffunc) { // function found
            reportAstChanged();
            auto opCall = make_smart<ExprCall>(expr_at, opN);
            opCall->arguments = das::move(tempCall->arguments);
            return opCall;
        } else {
            return nullptr;
        }
    }
    ExpressionPtr InferTypes::inferGenericOperator(const string &opN, const LineInfo &expr_at, const ExpressionPtr &arg0, const ExpressionPtr &arg1, InferCallError err) {
        if ( arg0->type && arg0->type->isExprType() ) return nullptr;
        if ( arg1 && arg1->type && arg1->type->isExprType() ) return nullptr;
        auto opName = "_::" + opN;
        auto tempCall = make_smart<ExprLooksLikeCall>(expr_at, opName);
        tempCall->arguments.push_back(arg0);
        if (arg1)
            tempCall->arguments.push_back(arg1);
        auto ffunc = inferFunctionCall(tempCall.get(), err).get();
        if (opName != tempCall->name) { // this happens when the operator gets instanced
            reportAstChanged();
            auto opCall = make_smart<ExprCall>(expr_at, tempCall->name);
            opCall->arguments = das::move(tempCall->arguments);
            return opCall;
        } else if (ffunc) { // function found
            reportAstChanged();
            auto opCall = make_smart<ExprCall>(expr_at, opN);
            opCall->arguments = das::move(tempCall->arguments);
            return opCall;
        } else {
            return nullptr;
        }
    }
    ExpressionPtr InferTypes::inferGenericOperatorWithName(const string &opN, const LineInfo &expr_at, const ExpressionPtr &arg0, const string &arg1, InferCallError err) {
        auto conststring = make_smart<TypeDecl>(Type::tString);
        conststring->constant = true;
        auto fieldName = make_smart<ExprConstString>(expr_at, arg1);
        fieldName->type = conststring;
        return inferGenericOperator(opN, expr_at, arg0, fieldName, err);
    }
    Variable *InferTypes::findMatchingBlockOrLambdaVariable(const string &name) {
        // local (that on the stack)
        for (auto it = local.rbegin(); it != local.rend(); ++it) {
            auto var = (*it).get();
            if (var->name == name || var->aka == name) {
                return var;
            }
        }
        // block arguments
        for (auto it = blocks.rbegin(); it != blocks.rend(); ++it) {
            ExprBlock *block = *it;
            for (auto &arg : block->arguments) {
                if (arg->name == name || arg->aka == name) {
                    return arg.get();
                }
            }
        }
        // function argument
        if (func) {
            for (auto &arg : func->arguments) {
                if (arg->name == name || arg->aka == name) {
                    return arg.get();
                }
            }
        }
        // static class method accessing static variables
        if (func && func->isStaticClassMethod && func->classParent->hasStaticMembers) {
            auto staticVarName = func->classParent->name + "`" + name;
            if (auto var = func->classParent->module->findVariable(staticVarName)) {
                return var.get();
            }
        }
        // global
        auto vars = findMatchingVar(name, false);
        if (vars.size() == 1) {
            auto var = vars.back();
            return var.get();
        }
        // and nada
        return nullptr;
    }
}
