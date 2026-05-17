#include "daScript/misc/platform.h"

#include "daScript/ast/ast.h"
#include "daScript/ast/ast_visitor.h"
#include "daScript/das_common.h"
#include "daScript/daScriptModule.h"
#include "daScript/misc/handle_registry.h"
#include "daScript/simulate/simulate_fusion.h"

#include <atomic>

namespace das {

    struct SubstituteModuleRefs : Visitor {
        SubstituteModuleRefs ( Module * from, Module * to ) : to(to), from(from) {}
        virtual void preVisit ( TypeDecl * td ) {
            if ( td->module == from ) td->module = to;
        }
        Module * const to;
        Module * const from;
    };

    //! Builtin modules are sometimes compiled from both native code and das code.
    //! In that case das code is parsed via parseDaScript function producing ProgramPtr
    //! It internally references itself, whereas it should actually reference the builtin module
    //! This visitor walks the program and substitutes references from parsed to builtin module.
    void SubstituteBuiltinModuleRefs ( ProgramPtr program, Module * from, Module * to ) {
        SubstituteModuleRefs subs ( from, to );
        program->visit(subs,/*visitGenerics =*/true);
    }

    bool splitTypeName ( const string & name, string & moduleName, string & funcName ) {
        // Find the first '::' that is NOT inside angle brackets — template-instance
        // struct names like "Option<_wip_mod::Thing>" embed '::' inside '<...>',
        // and we must not split there.
        size_t at = string::npos;
        int depth = 0;
        for ( size_t i = 0; i + 1 < name.size(); ++i ) {
            char c = name[i];
            if ( c=='<' ) {
                depth++;
            } else if ( c=='>' ) {
                if ( depth>0 ) depth--;
            } else if ( depth==0 && c==':' && name[i+1]==':' ) {
                at = i;
                break;
            }
        }
        if ( at!=string::npos ) {
            moduleName = name.substr(0,at);
            funcName = name.substr(at+2);
            if ( moduleName=="builtin`" ) moduleName="$";
            return true;
        } else {
            moduleName = "*";
            funcName = name;
            return false;
        }
    }

    // ANNOTATION

    const AnnotationArgument * AnnotationArgumentList::find ( const string & name, Type type ) const {
        auto it = find_if(begin(), end(), [&](const AnnotationArgument & arg){
            return (arg.name==name) && (type==Type::tVoid || type==arg.type);
        });
        return it==end() ? nullptr : &*it;
    }

    bool AnnotationArgumentList::getBoolOption(const string & name, bool def) const {
        auto arg = find(name, Type::tBool);
        return arg ? arg->bValue : def;
    }

    int32_t AnnotationArgumentList::getIntOption(const string & name, int32_t def) const {
        auto arg = find(name, Type::tInt);
        return arg ? arg->iValue : def;
    }

    uint64_t AnnotationArgumentList::getUInt64Option(const string & name, uint64_t def) const {
        auto arg = find(name, Type::tInt);
        return arg ? uint64_t(arg->iValue) : def;
    }

    uint64_t AnnotationArgumentList::getUInt64OptionEx(const string & name, const string & name2, uint64_t def) const {
        auto arg = find(name, Type::tInt);
        if (arg) return uint64_t(arg->iValue);
        arg = find(name2, Type::tInt);
        return arg ? uint64_t(arg->iValue) : def;
    }

    // MODULE

    void Module::addDependency ( Module * mod, bool pub ) {
        requireModule[mod] |= pub;
        for ( auto it : mod->requireModule ) {
            if ( it.second ) {
                addDependency(it.first, false);
            }
        }
    }

    void Module::setModuleName ( const string & n ) {
        name = n;
        nameHash = hash64z(n.c_str());
    }

    void Module::addBuiltinDependency ( ModuleLibrary & lib, Module * m, bool pub ) {
        DAS_ASSERTF(m, "Trying to add nullptr module.");
        lib.addModule(m);
        requireModule[m] = pub;
    }

    FileInfo * Module::getFileInfo() const {
        return ownFileInfo.get();
    }

    TypeAnnotation * Module::resolveAnnotation ( const TypeInfo * info ) {
        if ( info->type != Type::tHandle ) {
            return nullptr;
        }
        intptr_t ann = (intptr_t) (info->annotation_or_name);
        if ( ann & 1 ) {
            auto bound = daScriptEnvironment::getBound();
            DAS_VERIFYF(bound && bound->modules,"missing bound environment");
            // we add ~ at the beginning of the name for padding
            // if name is allocated by the compiler, it does not guarantee that it is aligned
            // we check if there is a ~ at the beginning of the name, and if it is - we skip it
            // that way we can accept both aligned and unaligned names
            auto cvtbuf = (char *) ann;
            if ( cvtbuf[0]=='~' ) cvtbuf++;
            string moduleName, annName;
            splitTypeName(cvtbuf, moduleName, annName);
            TypeAnnotation * resolve = nullptr;
            for ( auto pm = bound->modules; pm!=nullptr; pm=pm->next ) {
                if ( pm->name == moduleName ) {
                    if ( auto annT = pm->findAnnotation(annName) ) {
                        resolve = (TypeAnnotation *) annT;
                    }
                    break;
                }
            }
            if ( bound->g_resolve_annotations ) {
                info->annotation_or_name = resolve;
            }
            return resolve;
        } else {
            return info->annotation_or_name;
        }
    }

    atomic<int> g_envTotal(0);

    void Module::Initialize() {
        daScriptEnvironment::ensure();
        g_envTotal ++;

        if (daScriptEnvironment::getBound()->modules == nullptr) {
            DAS_FATAL_ERROR("No modules founds. You should add modules before call that function.");
        }

        // InitDependencies do not add new modules.
        vector<bool> mod_state;
        bool any = true;
        bool all = false;
        while ( !all && any ) {
            all = true;
            any = false;
            size_t i = 0;
            for ( auto m = daScriptEnvironment::getBound()->modules; m ; m = m->next, i++ ) {
                auto result = m->initDependencies();
                all &= result;
                if (i >= mod_state.size()) {
                    // init
                    mod_state.emplace_back(false);
                    DAS_ASSERT(mod_state.size() == i + 1);
                }
                if (result && !mod_state.at(i)) {
                    mod_state.at(i) = true;
                    any = true;
                }
            }
        }
        if (!any) {
            // Some modules was not initialized!
            size_t i = 0;
            string error = "";
            for ( auto m = daScriptEnvironment::getBound()->modules; m ; m = m->next, i++ ) {
                DAS_ASSERT(mod_state.size() == i);
                if (!mod_state.at(i)) {
                    error += " " + m->name;
                }
            }
            DAS_FATAL_ERROR("Unable to initialize some modules:%s\n", error.c_str());
        }
        // Collect reachable TypeDecl from thread root into module roots, sweep the rest.
        auto & threadRoot = gc_root::gc_get_thread_root();
        for ( auto m = daScriptEnvironment::getBound()->modules; m ; m = m->next ) {
            m->gc_collect(&threadRoot);
        }
        threadRoot.gc_sweep();
    }

    void Module::CollectFileInfo(das::vector<FileInfoPtr> &finfos) {
        DAS_ASSERT(daScriptEnvironment::getOwned()!=nullptr);
        DAS_ASSERT(daScriptEnvironment::getBound()!=nullptr);
        auto m = daScriptEnvironment::getBound()->modules;
        while ( m ) {
            finfos.emplace_back(das::move(m->ownFileInfo));
            m = m->next;
        }
    }

    uint64_t Module::CountHandleLeaks() {
        return handleRegistry_countAll();
    }

    void Module::Shutdown( bool dumpHandleLeaks ) {
        DAS_ASSERT(daScriptEnvironment::getOwned()!=nullptr);
        DAS_ASSERT(daScriptEnvironment::getBound()!=nullptr);
        g_envTotal --;
        if ( g_envTotal==0 ) {
            shutdownDebugAgent();
        }
        auto m = daScriptEnvironment::getBound()->modules;
        while ( m ) {
            auto pM = m;
            m = m->next;
            delete pM;
        }
        // Window between module destruction (which drains job threads via
        // Module_JobQue::~Module_JobQue) and DLL unload below: function
        // pointers in dasModule*.shared_module DLLs are still valid, and any
        // live job threads that were holding handles have exited. Dump here.
        if ( dumpHandleLeaks ) handleRegistry_dumpAll();
        // Free allocated structures for dynamic modules (unloads DLLs).
        delete daScriptEnvironment::getBound()->g_dyn_modules_resolve;

        clearGlobalAotLibrary();
        DAS_ASSERTF(g_resetFusionEngineFn, "fusion library not loaded");
        g_resetFusionEngineFn();
        daScriptEnvironment::setBound(nullptr);
        if ( daScriptEnvironment::getOwned() ) {
            delete daScriptEnvironment::getOwned();
            daScriptEnvironment::setOwned(nullptr);
        }
    }

    void Module::Reset(bool debAg) {
        if ( debAg ) shutdownDebugAgent();
        auto m = daScriptEnvironment::getBound()->modules;
        while ( m ) {
            auto pM = m;
            m = m->next;
            if ( pM->promoted ) delete pM;
        }
    }

    void Module::foreach ( const callable<bool (Module * module)> & func ) {
        for (auto m = daScriptEnvironment::getBound()->modules; m != nullptr; m = m->next) {
            if (!func(m)) break;
        }
    }

    Module * Module::require ( const string & name ) {
        if ( !daScriptEnvironment::getBound() ) return nullptr;
        for ( auto m = daScriptEnvironment::getBound()->modules; m != nullptr; m = m->next ) {
            if ( m->name == name ) {
                return m;
            }
        }
        return nullptr;
    }

    Module * Module::requireEx ( const string & name, bool allowPromoted ) {
        if ( !daScriptEnvironment::getBound() ) return nullptr;
        for ( auto m = daScriptEnvironment::getBound()->modules; m != nullptr; m = m->next ) {
            if ( allowPromoted || !m->promoted ) {
                if ( m->name == name ) {
                    return m;
                }
            }
        }
        return nullptr;
    }

    Type Module::getOptionType ( const string & optName ) const {
        auto it = options.find(optName);
        if ( it != options.end() ) {
            return it->second;
        } else {
            return Type::none;
        }
    }

    Type Module::findOption ( const string & name ) {
        Type optT = Type::none;
        for ( auto m = daScriptEnvironment::getBound()->modules; m != nullptr; m = m->next ) {
            auto tt = m->getOptionType(name);
            if ( tt != Type::none ) {
                DAS_ASSERTF(optT==Type::none, "duplicate module option %s", name.c_str());
                optT = tt;
            }
        }
        return optT;
    }

    Module::Module ( const string & n ) {
        setModuleName(n);
        if ( !name.empty() ) {
            auto first = daScriptEnvironment::getBound()->modules;
            while (first != nullptr)
            {
                if (first->name == n) {
                    DAS_FATAL_ERROR("Module '%s' already created", first->name.c_str());
                }
                first = first->next;
            }
            next = daScriptEnvironment::getBound()->modules;
            daScriptEnvironment::getBound()->modules = this;
            builtIn = true;
        }
        if ( n != "$" ) {
            requireModule[require("$")] = false;
        } else if (!name.empty()) {
            requireModule[this] = false;
        }
        isModule = !n.empty();
        isPublic = true;
    }

    void Module::promoteToBuiltin(const FileAccessPtr & access) {
        DAS_ASSERTF(!builtIn, "failed to promote. already builtin");
        next = daScriptEnvironment::getBound()->modules;
        daScriptEnvironment::getBound()->modules = this;
        builtIn = true;
        promoted = true;
        promotedAccess = access;
    }

    Module::~Module() {
        if ( builtIn ) {
            Module ** p = &daScriptEnvironment::getBound()->modules;
            for ( auto m = daScriptEnvironment::getBound()->modules; m != nullptr; p = &m->next, m = m->next ) {
                if ( m == this ) {
                    *p = m->next;
                    return;
                }
            }
            DAS_ASSERTF(0, "failed to unlink. was builtIn field assigned after the fact?");
        }
    }

    void Module::gc_collect ( gc_root * from ) {
        auto target = &module_gc_root;
        // collect alias types
        aliasTypes.foreach([&](auto td) {
            if ( td ) td->gc_collect(target, from);
        });
        // collect types in handle/annotation types
        for ( auto & [key, ann] : handleTypes ) {
            if ( ann ) ann->gc_collect(target, from);
        }
        // collect types in structures (full recursive walk via virtual gc_collect)
        structures.foreach([&](auto & st) {
            st->gc_collect(target, from);
        });
        // collect types in functions (full recursive walk via virtual gc_collect)
        functions.foreach([&](auto & fn) {
            fn->gc_collect(target, from);
        });
        generics.foreach([&](auto & fn) {
            fn->gc_collect(target, from);
        });
        // collect types in globals
        globals.foreach([&](auto & var) {
            var->gc_collect(target, from);
        });
        // collect types in enumerations
        enumerations.foreach([&](auto & en) {
            en->gc_collect(target, from);
        });
    }

    void Module::CollectSharedModules() {
        Module::foreach([&](Module * mod){
            if ( mod->macroContext ) mod->macroContext->collectHeap(nullptr, true, false);   // validate?
            return true;
        });
    }

    void Module::ClearSharedModules() {
        vector<Module *> kmp;
        Module::foreach([&](Module * mod){
            if ( mod->promoted ) {
                kmp.push_back(mod);
            }
            return true;
        });
        for ( auto km : kmp ) {
            delete km;
        }
    }

    bool Module::addAlias ( const TypeDeclPtr & at, bool canFail ) {
        if ( at->alias.empty() ) return false;
        if ( aliasTypes.insert(at->alias, at) ) {
            at->module = this;
            return true;
        } else {
            if ( !canFail ) {
                DAS_FATAL_ERROR("can't add duplicate alias %s to module %s\n",at->alias.c_str(), name.c_str() );
            }
            return false;
        }
    }

    void Module::registerAnnotation ( AnnotationPtr ptr ) {
        auto key = hash64z(ptr->name.c_str());
        if ( handleTypes.find(key)==handleTypes.end() ) {
            ptr->module = this;
            handleTypes[key] = ptr;
        }
    }

    bool Module::addAnnotation ( AnnotationPtr ptr, bool canFail ) {
        auto key = hash64z(ptr->name.c_str());
        auto it = handleTypes.find(key);
        if ( it!=handleTypes.end() && it->second==ptr ) {
            ptr->seal(this);
            return true;
        }
        if ( it==handleTypes.end() ) {
            ptr->seal(this);
            handleTypes[key] = ptr;
            return true;
        } else {
            if ( !canFail ) {
                DAS_FATAL_ERROR("can't add duplicate annotation %s to module %s\n", ptr->name.c_str(), name.c_str() );
            }
            return false;
        }
    }

    bool Module::addTypeInfoMacro ( TypeInfoMacro * ptr, bool canFail ) {
        if ( typeInfoMacros.insert(make_pair(ptr->name, unique_ptr<TypeInfoMacro>(ptr))).second ) {
            ptr->seal(this);
            return true;
        } else {
            if ( !canFail ) {
                DAS_FATAL_ERROR("can't add duplicate typeinfo macro %s to module %s\n", ptr->name.c_str(), name.c_str() );
            }
            return false;
        }
    }

    bool Module::addTypeMacro ( TypeMacro * ptr, bool canFail ) {
        if ( typeMacros.insert(make_pair(ptr->name, unique_ptr<TypeMacro>(ptr))).second ) {
            return true;
        } else {
            if ( !canFail ) {
                DAS_FATAL_ERROR("can't add duplicate type macro %s to module %s\n", ptr->name.c_str(), name.c_str() );
            }
            return false;
        }
    }

    bool Module::addReaderMacro ( ReaderMacro * ptr, bool canFail ) {
        if ( readMacros.insert(make_pair(ptr->name, unique_ptr<ReaderMacro>(ptr))).second ) {
            ptr->seal(this);
            return true;
        } else {
            if ( !canFail ) {
                DAS_FATAL_ERROR("can't add duplicate reader macro %s to module %s\n", ptr->name.c_str(), name.c_str() );
            }
            return false;
        }
    }

    bool Module::addCommentReader ( CommentReader * ptr, bool canFail ) {
        if ( commentReader ) {
            if ( !canFail ) {
                DAS_FATAL_ERROR("can't add 2nd comment reader to module %s\n", name.c_str() );
            }
            return false;
        }
        commentReader.reset(ptr);
        return true;
    }


    bool Module::addVariable ( const VariablePtr & var, bool canFail ) {
        if ( globals.insert(var->name, var) ) {
            var->module = this;
            var->global = true;
            return true;
        } else {
            if ( !canFail ) {
                DAS_FATAL_ERROR("can't add duplicate variable %s to module %s\n", var->name.c_str(), name.c_str() );
            }
            return false;
        }
    }

    bool Module::addEnumeration ( const EnumerationPtr & en, bool canFail ) {
        if ( enumerations.insert(en->name, en) ) {
            en->module = this;
            return true;
        } else {
            if ( !canFail ) {
                DAS_FATAL_ERROR("can't add duplicate enumeration %s to module %s\n", en->name.c_str(), name.c_str() );
            }
            return false;
        }
    }

    bool Module::removeStructure ( const StructurePtr & st ) {
        return structures.remove(st->name);
    }

    bool Module::addStructure ( const StructurePtr & st, bool canFail ) {
        if ( structures.insert(st->name, st) ) {
            st->module = this;
            return true;
        } else {
            if ( !canFail ) {
                DAS_FATAL_ERROR("can't add duplicate structure %s to module %s\n", st->name.c_str(), name.c_str() );
            }
            return false;
        }
    }

    bool Module::addKeyword (const string & kwd, bool needOxfordComma, bool canFail ) {
        auto it = find_if(keywords.begin(), keywords.end(), [&](auto value){
            return value.first == kwd;
        });
        if ( it != keywords.end() ) {
            if ( !canFail ) {
                DAS_FATAL_ERROR("can't add duplicate keyword %s to module %s\n", kwd.c_str(), name.c_str() );
            }
            return false;
        }
        keywords.emplace_back(kwd, needOxfordComma);
        return true;
    }

    bool Module::addTypeFunction (const string & kwd, bool canFail ) {
        auto it = find_if(typeFunctions.begin(), typeFunctions.end(), [&](auto value){
            return value == kwd;
        });
        if ( it != typeFunctions.end() ) {
            if ( !canFail ) {
                DAS_FATAL_ERROR("can't add duplicate type function %s to module %s\n", kwd.c_str(), name.c_str() );
            }
            return false;
        }
        typeFunctions.emplace_back(kwd);
        return true;
    }

    bool Module::addFunction ( const FunctionPtr & fn, bool canFail ) {
        fn->module = this;
        auto mangledName = fn->getMangledName();
        fn->module = nullptr;
        if ( fn->builtIn && fn->arguments.size()>DAS_MAX_FUNCTION_ARGUMENTS ) {
            DAS_FATAL_ERROR("can't add function %s to module %s, too many arguments, DAS_MAX_FUNCTION_ARGUMENTS=%i\n", mangledName.c_str(), name.c_str(), DAS_MAX_FUNCTION_ARGUMENTS );
        }
        if ( fn->builtIn && fn->sideEffectFlags==uint32_t(SideEffects::none) && fn->result->isVoid() ) {
            DAS_FATAL_ERROR("can't add function %s to module %s; it has no side effects and no return type\n", mangledName.c_str(), name.c_str() );
        }
        if ( fn->result && !fn->result->ref && fn->result->isWorkhorseType() && !fn->result->isPointer() ) {
            fn->result->constant = true;
        }
        if ( fn->builtIn ) {
            cumulativeHash = wyhash(mangledName.c_str(), mangledName.size(), cumulativeHash);
        }
        if ( fn->builtIn && fn->sideEffectFlags==uint32_t(SideEffects::modifyArgument)  ) {
            bool anyRW = false;
            for ( const auto & arg : fn->arguments ) {
                if ( arg->type->isRef() && !arg->type->isConst() ) {
                    anyRW = true;
                } else if ( arg->type->isPointer() && arg->type->firstType && !arg->type->firstType->isConst() ) {
                    anyRW = true;
                }
            }
            if ( !anyRW ) {
                DAS_FATAL_ERROR("can't add function %s to module %s; modify argument requires non-const ref argument\n", mangledName.c_str(), name.c_str() );
            }
        }
        if ( functions.insert(mangledName, fn) ) {
            functionsByName[hash64z(fn->name.c_str())].push_back(fn);
            fn->module = this;
            return true;
        } else {
            if ( !canFail ) {
                DAS_FATAL_ERROR("can't add duplicate function %s to module %s\n", mangledName.c_str(), name.c_str() );
            }
            return false;
        }
    }

    bool Module::replaceFunction ( const FunctionPtr & fn ) {
        fn->module = this;
        auto mangledName = fn->getMangledName();
        auto mangledHash = hash64z(mangledName.c_str());
        auto nameHashX = hash64z(fn->name.c_str());
        // get old function pointer before replacing (avoid touching stale types)
        auto oldFn = functions.find(mangledHash);
        if ( !oldFn ) return false;
        auto oldFnPtr = oldFn;
        // replace in safebox objects map
        functions.replace(mangledHash, fn);
        // replace in objectsInOrder (compare by pointer, not by mangled name)
        functions.foreach([&](auto & ofn){
            if ( ofn == oldFnPtr ) {
                ofn = fn;
            }
        });
        // replace in functionsByName
        auto kv = functionsByName.find(nameHashX);
        if ( kv ) {
            for ( auto & fp : kv->second ) {
                if ( fp == oldFnPtr ) {
                    fp = fn;
                    break;
                }
            }
        }
        return true;
    }

    bool Module::addGeneric ( const FunctionPtr & fn, bool canFail ) {
        fn->module = this;
        auto mangledName = fn->getMangledName();
        fn->module = nullptr;
        if ( generics.insert(mangledName, fn) ) {
            genericsByName[hash64z(fn->name.c_str())].push_back(fn);
            fn->module = this;
            return true;
        } else {
            if ( !canFail ) {
                DAS_FATAL_ERROR("can't add duplicate generic function %s to module %s\n", mangledName.c_str(), name.c_str() );
            }
            return false;
        }
    }

    TypeDeclPtr Module::findAlias ( const string & na ) const {
        return aliasTypes.find(na);
    }

    VariablePtr Module::findVariable ( const string & na ) const {
        return globals.find(na);
    }

    FunctionPtr Module::findFunctionByMangledNameHash ( uint64_t hash ) const {
        return functions.find(hash);
    }

    FunctionPtr Module::findFunction ( const string & mangledName ) const {
        return functions.find(mangledName);
    }

    FunctionPtr Module::findGeneric ( const string & mangledName ) const {
        return generics.find(mangledName);
    }

    FunctionPtr Module::findUniqueFunction ( const string & mangledName ) const {
        auto it = functionsByName.find(hash64z(mangledName.c_str()));
        if ( !it ) return nullptr;
        if ( it->second.size()!=1 ) return nullptr;
        return it->second[0];
    }

    StructurePtr Module::findStructureByMangledNameHash ( uint64_t hash ) const {
        return structures.find(hash);
    }

    StructurePtr Module::findStructure ( const string & na ) const {
        return structures.find(na);
    }

    AnnotationPtr Module::findAnnotation ( const string & na ) const {
        auto it = handleTypes.find(hash64z(na.c_str()));
        return it != handleTypes.end() ? it->second : nullptr;
    }

    ReaderMacro * Module::findReaderMacro ( const string & na ) const {
        auto it = readMacros.find(na);
        return it != readMacros.end() ? it->second.get() : nullptr;
    }

    TypeInfoMacro * Module::findTypeInfoMacro ( const string & na ) const {
        auto it = typeInfoMacros.find(na);
        return it != typeInfoMacros.end() ? it->second.get() : nullptr;
    }

    EnumerationPtr Module::findEnumByMangledNameHash ( uint64_t hash ) const {
        return enumerations.find(hash);
    }

    EnumerationPtr Module::findEnum ( const string & na ) const {
        return enumerations.find(na);
    }

    ExprCallFactory * Module::findCall ( const string & na ) const {
        auto it = callThis.find(na);
        return it != callThis.end() ? &it->second : nullptr;
    }

    static bool appendBuiltinModuleContent ( Module * target, ProgramPtr program, const string & modName ) {
        if ( !program ) {
            DAS_FATAL_ERROR("builtin module did not parse %s\n", modName.c_str());
            return false;
        }
        if (program->failed()) {
            TextWriter issues;
            for (auto & err : program->errors) {
                issues << reportError(err.at, err.what, err.extra, err.fixme, err.cerr);
            }
            DAS_FATAL_ERROR("%s\nbuiltin module did not compile %s\n", issues.str().c_str(), modName.c_str());
            return false;
        }
        // append content into target module
        program->thisModule->aliasTypes.foreach([&](auto aliasTypePtr){
            target->addAlias(aliasTypePtr);
        });
        program->thisModule->enumerations.foreach([&](auto penum){
            target->addEnumeration(penum);
        });
        program->thisModule->structures.foreach([&](auto pst){
            target->addStructure(pst);
        });
        program->thisModule->generics.foreach([&](auto fn){
            target->addGeneric(fn);
        });
        program->thisModule->globals.foreach([&](auto gvar){
            target->addVariable(gvar);
        });
        program->thisModule->functions.foreach([&](auto fn){
            target->addFunction(fn);
        });
        for (auto & rqm : program->thisModule->requireModule) {
            if ( rqm.first != target ) {
                target->requireModule[rqm.first] |= rqm.second;
            }
        }
        // macros
        auto ptm = program->thisModule.get();
        if ( ptm->macroContext ) {
            swap ( target->macroContext, ptm->macroContext );
            for ( auto & [key, fna] : ptm->handleTypes ) {
                target->addAnnotation(fna);
            }
        }
        for (auto & m : ptm->simulateMacros) target->simulateMacros.push_back(std::move(m));
        for (auto & m : ptm->captureMacros) target->captureMacros.push_back(std::move(m));
        for (auto & m : ptm->forLoopMacros) target->forLoopMacros.push_back(std::move(m));
        for (auto & m : ptm->variantMacros) target->variantMacros.push_back(std::move(m));
        for (auto & m : ptm->macros) target->macros.push_back(std::move(m));
        for (auto & m : ptm->inferMacros) target->inferMacros.push_back(std::move(m));
        for (auto & m : ptm->optimizationMacros) target->optimizationMacros.push_back(std::move(m));
        for (auto & m : ptm->lintMacros) target->lintMacros.push_back(std::move(m));
        for (auto & m : ptm->globalLintMacros) target->globalLintMacros.push_back(std::move(m));
        for ( auto & rm : ptm->readMacros ) {
            rm.second->seal(target);
            target->readMacros[rm.first] = std::move(rm.second);
        }
        for ( auto & tm : ptm->typeMacros ) {
            target->typeMacros[tm.first] = std::move(tm.second);
        }
        target->commentReader = std::move(ptm->commentReader);
        for ( auto & op : ptm->options) {
            DAS_ASSERTF(target->options.find(op.first)==target->options.end(),"duplicate option %s", op.first.c_str());
            target->options[op.first] = op.second;
        }
        SubstituteBuiltinModuleRefs( program, program->thisModule.get(), target );
        return true;
    }

    bool Module::compileBuiltinModule ( const string & modName, const unsigned char * const str, unsigned int str_len ) {
        TextWriter issues;
        auto access = make_smart<FileAccess>();
        auto fileInfo = make_unique<TextFileInfo>((char *) str, uint32_t(str_len), false);
        access->setFileInfo(modName, das::move(fileInfo));
        ModuleGroup dummyLibGroup;
        auto program = parseDaScript(modName, "", access, issues, dummyLibGroup, true);
        ownFileInfo = access->letGoOfFileInfo(modName);
        DAS_ASSERTF(ownFileInfo,"something went wrong and FileInfo for builtin module can not be obtained");
        auto result = appendBuiltinModuleContent(this, program, modName);
        program->thisModule->module_gc_root.gc_dump_to_thread_root();
        return result;
    }

    bool isValidBuiltinName ( const string & name, bool canPunkt ) {
        if ( name.size()>=2 && name[0]=='.' && name[1]=='`') {  // any name starting with .` is ok
            return true;
        }
        bool hasPunkt = false;
        bool hasAlNum = false;
        for ( auto ch : name ) {
            if ( isalpha(ch) ) {
                hasAlNum = true;
                if ( isupper(ch) ) {
                    return false;
                }
            } else if ( isdigit(ch) || ch=='_' || ch=='`' ) {
                hasAlNum = true;
            } else if ( ispunct(ch) ) {
                if ( canPunkt ) {
                    hasPunkt = true;
                } else {
                    return false;
                }
            } else {
                return false;
            }
        }
        return hasPunkt ^ hasAlNum; // has punkt, or alnum, but never both
    }

    void Module::verifyBuiltinNames(uint32_t flags) {
        bool failed = false;
        if ( flags & VerifyBuiltinFlags::verifyAliasTypes ) {
            aliasTypes.foreach([&](auto aliasTypePtr){
                if ( !isValidBuiltinName(aliasTypePtr->alias) ) {
                    DAS_FATAL_ERROR("%s - alias type has incorrect name. expecting snake_case\n", aliasTypePtr->alias.c_str());
                    failed = true;
                }
            });
        }
        if ( flags & VerifyBuiltinFlags::verifyHandleTypes ) {
            for ( auto & [key, annPtr] : handleTypes ) {
                if ( !isValidBuiltinName(annPtr->name) ) {
                    DAS_FATAL_LOG("%s - annotation has incorrect name. expecting snake_case\n", annPtr->name.c_str());
                    failed = true;
                }
            }
        }
        if ( flags & VerifyBuiltinFlags::verifyGlobals ) {
            globals.foreach([&](auto var){
                if ( !isValidBuiltinName(var->name) ) {
                    DAS_FATAL_LOG("%s - global variable has incorrect name. expecting snake_case\n", var->name.c_str());
                    failed = true;
                }
            });
        }
        if ( flags & VerifyBuiltinFlags::verifyFunctions ) {
            functions.foreach([&](auto fun){
                if ( !isValidBuiltinName(fun->name, true) ) {
                    DAS_FATAL_LOG("%s - function has incorrect name. expecting snake_case\n", fun->name.c_str());
                    failed = true;
                }
            });
        }
        if ( flags & VerifyBuiltinFlags::verifyGenerics ) {
            generics.foreach([&](auto fun){
                if ( !isValidBuiltinName(fun->name) ) {
                    DAS_FATAL_LOG("%s - generic function has incorrect name. expecting snake_case\n", fun->name.c_str());
                    failed = true;
                }
            });
        }
        if ( flags & VerifyBuiltinFlags::verifyStructures ) {
            structures.foreach([&](auto st){
                if ( !isValidBuiltinName(st->name) ) {
                    DAS_FATAL_LOG("%s - structure has incorrect name. expecting snake_case\n", st->name.c_str());
                    failed = true;
                }
                if ( flags & VerifyBuiltinFlags::verifyStructureFields ) {
                    for ( auto & fd : st->fields ) {
                        if ( !isValidBuiltinName(fd.name) ) {
                            DAS_FATAL_LOG("%s.%s - structure field has incorrect name. expecting snake_case\n", st->name.c_str(), fd.name.c_str());
                            failed = true;
                        }
                    }
                }
            });
        }
        if ( flags & VerifyBuiltinFlags::verifyEnums ) {
            enumerations.foreach([&](auto en){
                if ( !isValidBuiltinName(en->name) ) {
                    DAS_FATAL_LOG("%s - enumeration has incorrect name. expecting snake_case\n", en->name.c_str());
                    failed = true;
                }
                if ( flags & VerifyBuiltinFlags::verifyEnumFields ) {
                    for ( auto & fd : en->list ) {
                        if ( !isValidBuiltinName(fd.name) ) {
                            DAS_FATAL_LOG("%s.%s - enumeration field has incorrect name. expecting snake_case\n", en->name.c_str(), fd.name.c_str());
                            failed = true;
                        }
                    }
                }
            });
        }
        if ( failed ) {
            DAS_FATAL_ERROR("verifyBuiltinNames failed");
        }
    }

    void Module::verifyAotReady() {
        bool failed = false;
        functions.foreach([&](auto fun){
            if ( fun->builtIn ) {
                auto bif = (BuiltInFunction *) fun;
                if ( !bif->policyBased && bif->cppName.empty() ) {
                    DAS_FATAL_LOG("builtin function %s is missing cppName\n", fun->describe().c_str());
                    failed = true;
                }
            }
        });
        if ( failed ) {
            DAS_FATAL_ERROR("verifyAotReady failed");
        }
    }

    // MODULE LIBRARY

    ModuleLibrary::ModuleLibrary( Module * this_module ) {
        addModule(this_module);
    }

    void ModuleLibrary::addBuiltInModule () {
        Module * module = Module::require("$");
        DAS_ASSERTF(module, "builtin module not found? or you have forgotten to NEED_MODULE(Module_BuiltIn) be called first");
        if (module) addModule(module);
    }

    void ModuleLibrary::renameModule ( Module * module, const string & newName ) {
        auto oldHashLookup = moduleLookupByHash.find(module->nameHash);
        DAS_VERIFYF(oldHashLookup!=moduleLookupByHash.end(), "module %s not found", module->name.c_str());
        moduleLookupByHash.erase(oldHashLookup);
        module->setModuleName(newName);
        DAS_VERIFYF(moduleLookupByHash.find(module->nameHash)==moduleLookupByHash.end(), "duplicate module hash %s", module->name.c_str());
        moduleLookupByHash[module->nameHash] = module;
    }

    bool ModuleLibrary::addModule ( Module * module ) {
        DAS_ASSERTF(module, "module not found? or you have forgotten to NEED_MODULE(Module_<name>) be called first before addModule(require(<name>))");
        if ( module ) {
            thisModule = thisModule ? thisModule : module;
            if ( find(modules.begin(),modules.end(),module)==modules.end() ) {
                for ( auto dep : ordered(module->requireModule, [](auto m1, auto m2){ return m1->name < m2->name; }) ) {
                    if ( dep.first != module ) {
                        addModule ( dep.first );
                    }
                }
                modules.push_back(module);
                if ( moduleLookupByHash.find(module->nameHash)!=moduleLookupByHash.end() ) {
                    modules.pop_back();
                    return false;
                }
                moduleLookupByHash[module->nameHash] = module;
                module->addPrerequisits(*this);
                return true;
            }
        }
        return false;
    }

    void ModuleLibrary::foreach ( const callable<bool (Module * module)> & func, const string & moduleName ) const {
        DAS_ASSERT(moduleName!="_" && moduleName!="__");
        if (  moduleName=="*" ) {
            for ( auto pm : modules ) {
                if ( !func(pm) ) break;
            }
        } else {
            if ( auto pm = findModule(moduleName) ) {
                if ( !func(pm) ) return;
                for ( auto & dep : pm->requireModule ) {
                    if ( dep.second ) {  // public dependency
                        if ( !func(dep.first) ) return;
                    }
                }
            }
        }
    }

    void ModuleLibrary::foreach_in_order ( const callable<bool (Module * module)> & func, Module * thisM ) const {
        DAS_ASSERT(modules.size());
        // {builtin} {THIS_MODULE} {require1} {require2} ...
        for (auto module : modules) {
            if ( module==thisM ) continue;
            if ( !func(module) ) return;
        }
        if (thisM) {
            func(thisM);
        }
    }

    Module * ModuleLibrary::findModuleByMangledNameHash ( uint64_t hash ) const {
        auto it = moduleLookupByHash.find(hash);
        return it != moduleLookupByHash.end() ? it->second : nullptr;
    }

    Module * ModuleLibrary::findModule ( const string & mn ) const {
        auto hash = hash64z(mn.c_str());
        auto it = moduleLookupByHash.find(hash);
        return it != moduleLookupByHash.end() ? it->second : nullptr;
    }

    void ModuleLibrary::findWithCallback ( const string & name, Module * inWhichModule, const callable<void (Module * pm, const string &name, Module * inWhichModule)> & func ) const {
        string moduleName, funcName;
        splitTypeName(name, moduleName, funcName);
        if ( moduleName!="_" && moduleName!="__") { // those are never found. in reality we may want to support this one day with "*" and "thisModuleName" accordingly
            foreach([&](Module * pm) -> bool {
                if ( inWhichModule->isVisibleDirectly(pm) ) {
                    func(pm, funcName, inWhichModule);
                }
                return true;
            }, moduleName);
        }
    }

    void ModuleLibrary::findAnnotation ( vector<AnnotationPtr> & ptr, Module * pm, const string & annotationName, Module * ) const {
        if ( auto pp = pm->findAnnotation(annotationName) )
            ptr.push_back(das::move(pp));
    }

    vector<AnnotationPtr> ModuleLibrary::findAnnotation ( const string & name, Module * inWhichModule ) const {
        vector<AnnotationPtr> ptr;
        string moduleName, annName;
        splitTypeName(name, moduleName, annName);
        if ( moduleName!="_" && moduleName!="__") {
            foreach([&](Module * pm) -> bool {
                if ( !inWhichModule || inWhichModule->isVisibleDirectly(pm) )
                    findAnnotation(ptr, pm, annName, inWhichModule);
                return true;
            }, moduleName);
        }
        return ptr;
    }

    vector<TypeInfoMacro*> ModuleLibrary::findTypeInfoMacro ( const string & name, Module * inWhichModule ) const {
        vector<TypeInfoMacro*> ptr;
        string moduleName, annName;
        splitTypeName(name, moduleName, annName);
        if ( moduleName!="_" && moduleName!="__") {
            foreach([&](Module * pm) -> bool {
                if ( !inWhichModule || inWhichModule->isVisibleDirectly(pm) )
                    if ( auto pp = pm->findTypeInfoMacro(annName) )
                        ptr.push_back(pp);
                return true;
            }, moduleName);
        }
        return ptr;
    }

    void ModuleLibrary::findStructure ( vector<StructurePtr> & ptr, Module * pm, const string & structName, Module * inWhichModule ) const {
        if ( auto pp = pm->findStructure(structName) ) {
            if ( !pp->privateStructure || pp->module==inWhichModule ) {
                ptr.push_back(das::move(pp));
            }
        }
    }

    vector<StructurePtr> ModuleLibrary::findStructure ( const string & name, Module * inWhichModule ) const {
        vector<StructurePtr> ptr;
        string moduleName, funcName;
        splitTypeName(name, moduleName, funcName);
        foreach([&](Module * pm) -> bool {
            if ( !inWhichModule || inWhichModule->isVisibleDirectly(pm) ) {
                findStructure(ptr, pm, funcName, inWhichModule);
            }
            return true;
        }, moduleName);
        return ptr;
    }

    void ModuleLibrary::findAlias ( vector<TypeDeclPtr> & ptr, Module * pm, const string & aliasName, Module * inWhichModule ) const {
        if ( auto pp = pm->findAlias(aliasName) ) {
            if ( !pp->isPrivateAlias || pp->module==inWhichModule ) {
                if ( pp->isEnumT() ) {
                    if ( !pp->enumType->isPrivate || pp->enumType->module==inWhichModule ) {
                        ptr.push_back(das::move(pp));
                    }
                } else if ( pp->baseType==Type::tStructure ) {
                    if ( !pp->structType->privateStructure || pp->structType->module==inWhichModule ) {
                        ptr.push_back(das::move(pp));
                    }
                } else {
                    ptr.push_back(das::move(pp));
                }
            }
        }
    }

    vector<TypeDeclPtr> ModuleLibrary::findAlias ( const string & name, Module * inWhichModule ) const {
        vector<TypeDeclPtr> ptr;
        string moduleName, aliasName;
        splitTypeName(name, moduleName, aliasName);
        foreach([&](Module * pm) -> bool {
            if ( !inWhichModule || inWhichModule->isVisibleDirectly(pm) )
                findAlias(ptr, pm, aliasName, inWhichModule);
            return true;
        }, moduleName);
        return ptr;
    }


    void ModuleLibrary::findEnum ( vector<EnumerationPtr> & ptr, Module * pm, const string & enumName, Module * inWhichModule ) const {
        if ( auto pp = pm->findEnum(enumName) ) {
            if ( !pp->isPrivate || pp->module==inWhichModule ) {
                ptr.push_back(das::move(pp));
            }
        }
    }

    vector<EnumerationPtr> ModuleLibrary::findEnum ( const string & name, Module * inWhichModule ) const {
        vector<EnumerationPtr> ptr;
        string moduleName, enumName;
        splitTypeName(name, moduleName, enumName);
        foreach([&](Module * pm) -> bool {
            if ( !inWhichModule || inWhichModule->isVisibleDirectly(pm) ) {
                findEnum(ptr, pm, enumName, inWhichModule);
            }
            return true;
        }, moduleName);
        return ptr;
    }

    TypeDeclPtr ModuleLibrary::makeStructureType ( const string & name ) const {
        auto t = new TypeDecl(Type::tStructure);
        auto structs = findStructure(name,nullptr);
        if ( structs.size()==1 ) {
            t->structType = structs.back();
        } else {
            DAS_FATAL_ERROR("makeStructureType(%s) failed\n", name.c_str());
            return nullptr;
        }
        return t;
    }

    vector<AnnotationPtr> ModuleLibrary::findStaticAnnotation ( const string & name ) const {
        vector<AnnotationPtr> ptr;
        string moduleName, annName;
        splitTypeName(name, moduleName, annName);
        Module::foreach([&](Module * pm) -> bool {
            if ( auto pp = pm->findAnnotation(annName) )
                ptr.push_back(pp);
            return true;
        });
        return ptr;
    }

    TypeDeclPtr ModuleLibrary::makeHandleType ( const string & name ) const {
        auto t = new TypeDecl(Type::tHandle);
        auto handles = findAnnotation(name,nullptr);
#if DAS_ALLOW_ANNOTATION_LOOKUP
        bool need_require = false;
        if ( handles.size()==0 ) {
            handles = findStaticAnnotation(name);
            need_require = true;
        }
#endif
        if ( handles.size()==1 ) {
#if DAS_ALLOW_ANNOTATION_LOOKUP
            if ( need_require ) {
                ModuleLibrary * THAT = (ModuleLibrary *) this;
                THAT->addModule(handles.back()->module);
            }
#endif
            if ( handles.back()->rtti_isHandledTypeAnnotation() ) {
                t->annotation = static_cast<TypeAnnotation*>(handles.back());
            } else {
                DAS_FATAL_ERROR("makeHandleType(%s) failed, not a handle type\n", name.c_str());
                return nullptr;
            }
        } else if ( handles.size()==0 ) {

#if DAS_ALLOW_ANNOTATION_LOOKUP
            DAS_FATAL_LOG("NEED_MODULE(Module_<name>) with %s needs to be before this module binding.\n", name.c_str());
            DAS_FATAL_LOG("Explicit lib.addModule(require(\"<name>\")); would be preferable.\n");
#else
            DAS_FATAL_LOG("In the bound module missing is lib.addModule(require(\"module-name\")) which contains module-name::%s\n", name.c_str());
            Module::foreach([&](Module * mod){
                if ( mod->findAnnotation(name) ) {
                    DAS_FATAL_LOG("\tLikely candidate is lib.addModule(require(\"%s\"));\n", mod->name.c_str());
                }
                return true;
            });
#endif
            DAS_FATAL_ERROR("makeHandleType(%s) failed, missing annotation\n", name.c_str());
            return nullptr;
        } else {

            for ( auto & h : handles ) {
                DAS_FATAL_LOG("\tduplicate annotation%s::%s\n", h->name.c_str(), name.c_str());
            }
            DAS_FATAL_ERROR("makeHandleType(%s) failed, duplicate annotation\n", name.c_str());
            return nullptr;
        }
        return t;
    }

    TypeDeclPtr ModuleLibrary::makeEnumType ( const string & name ) const {
        auto enums = findEnum(name,nullptr);
        if ( enums.size()==1 ) {
            return enums.back()->makeEnumType();
        } else {
            DAS_FATAL_ERROR("makeEnumType(%s) failed\n", name.c_str());
            return nullptr;
        }
    }

    void ModuleLibrary::reset() {
        for ( auto & mod : modules ) {
            if ( !mod->builtIn ) {
                delete mod;
            }
        }
        modules.clear();
        moduleLookupByHash.clear();
    }

    // Module group

    ModuleGroup::~ModuleGroup() {
        reset();
    }

    void ModuleGroup::collectMacroContexts() {
        foreach([&](Module * pm) -> bool {
            if ( pm->macroContext ) pm->macroContext->collectHeap(nullptr, true, false);   // validate?
            return true;
        }, "*");
    }

    ModuleGroupUserData * ModuleGroup::getUserData ( const string & dataName ) const {
        auto it = userData.find(dataName);
        return it != userData.end() ? it->second.get() : nullptr;
    }

    bool ModuleGroup::setUserData ( ModuleGroupUserData * data ) {
        auto it = userData.find(data->name);
        if ( it != userData.end() ) {
            return false;
        }
        userData[data->name] = ModuleGroupUserDataPtr(data);
        return true;
    }

    using ModulesPullers = das::vector<module_pull_t>;

    static ModulesPullers & get_pullers() {
        static ModulesPullers pullers;
        return pullers;
    }

    ModulePullHelper::ModulePullHelper(module_pull_t pull) {
        get_pullers().push_back(pull);
    }

    void pull_all_auto_registered_modules() {
        for (module_pull_t pull : get_pullers()) {
            *das::ModuleKarma += unsigned(intptr_t(pull()));
        }
    }
}

