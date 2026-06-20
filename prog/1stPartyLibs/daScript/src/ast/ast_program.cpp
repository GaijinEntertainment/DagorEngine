#include "daScript/misc/platform.h"

#include "daScript/ast/ast.h"
#include "daScript/ast/ast_expressions.h"
#include "daScript/ast/ast_visitor.h"

namespace das {

    // program
    vector<ReaderMacro*> Program::getReaderMacro ( const string & name ) const {
        vector<ReaderMacro*> macros;
        string moduleName, markupName;
        splitTypeName(name, moduleName, markupName);
        auto tmod = thisModule.get();
        auto modMacro = [&](Module * mod) -> bool {
            if ( thisModule->isVisibleDirectly(mod) && mod!=tmod ) {
                auto it = mod->readMacros.find(markupName);
                if ( it != mod->readMacros.end() ) {
                    macros.push_back(it->second.get());
                }
            }
            return true;
        };
        library.foreach(modMacro, moduleName);
        return macros;
    }

    int Program::getContextStackSize() const {
        return options.getIntOption("stack", policies.stack);
    }

    vector<EnumerationPtr> Program::findEnum ( const string & name ) const {
        return library.findEnum(name,thisModule.get());
    }

    vector<TypeDeclPtr> Program::findAlias ( const string & name ) const {
        return library.findAlias(name,thisModule.get());
    }

    vector<AnnotationPtr> Program::findAnnotation ( const string & name ) const {
        return library.findAnnotation(name,thisModule.get());
    }

    vector<TypeInfoMacro*> Program::findTypeInfoMacro ( const string & name ) const {
        return library.findTypeInfoMacro(name,thisModule.get());
    }

    vector<StructurePtr> Program::findStructure ( const string & name ) const {
        return library.findStructure(name,thisModule.get());
    }

    void Program::error ( const string & str, const string & extra, const string & fixme, const LineInfo & at, CompilationError cerr ) {
        errors.emplace_back(str,extra,fixme,at,cerr);
        failToCompile = true;
    }

    // Identify the "not_resolved_yet" follow-on family by numeric range.
    // All `not_resolved_yet_*` codes live in the 31300-31399 block (the
    // `not_resolved_yet` facet cluster within stage 3, semantic). See
    // include/daScript/ast/compilation_errors.h.
    static __forceinline bool isNotResolvedYet ( CompilationError cerr ) {
        int v = int(cerr);
        return v >= 31300 && v < 31400;
    }

    void Program::deduplicateErrors () {
        if ( errors.size() < 2 ) return;
        // errors are already sorted by Error::operator< (at, what, extra, fixme)
        // before this is called.

        // Pass 1: scan for "primary" errors. A primary is any error whose cerr
        // is neither `unspecified` nor in the not_resolved_yet family. If any
        // primary exists program-wide, Rule 4 drops every not_resolved_yet
        // error; if any same-line coded error exists, Rule 3 drops same-line
        // unspecified entries.
        bool anyPrimaryProgramWide = false;
        das_hash_set<uint64_t> linesWithCoded;
        auto lineKey = [](const LineInfo & at) -> uint64_t {
            return (uint64_t(uintptr_t(at.fileInfo)) ^ uint64_t(at.line)) * 0x9E3779B97F4A7C15ull;
        };
        for ( const auto & e : errors ) {
            if ( e.cerr != CompilationError::unspecified && !isNotResolvedYet(e.cerr) ) {
                anyPrimaryProgramWide = true;
            }
            if ( e.cerr != CompilationError::unspecified ) {
                linesWithCoded.insert(lineKey(e.at));
            }
        }

        // Pass 2: build the surviving list applying all four rules.
        vector<Error> kept;
        kept.reserve(errors.size());
        size_t i = 0;
        const size_t N = errors.size();
        while ( i < N ) {
            const Error & e = errors[i];

            // Rule 4: drop not_resolved_yet entries program-wide if any primary exists.
            if ( anyPrimaryProgramWide && isNotResolvedYet(e.cerr) ) {
                ++i;
                continue;
            }
            // Rule 3: drop same-line unspecified if any coded error sits on the same line.
            if ( e.cerr == CompilationError::unspecified && linesWithCoded.count(lineKey(e.at)) ) {
                ++i;
                continue;
            }
            // Rule 1: collapse adjacent byte-identical entries.
            size_t j = i + 1;
            while ( j < N
                && errors[j].at == e.at
                && errors[j].what == e.what
                && errors[j].extra == e.extra
                && errors[j].fixme == e.fixme
                && errors[j].cerr == e.cerr ) {
                ++j;
            }
            int dupCount = int(j - i);
            // Rule 2: same-line, same-cerr, different-text — collapse to first
            // and append `(+N more on this line)` to the message.
            int sameLineSameCerrCount = 0;
            size_t k = j;
            while ( k < N
                && errors[k].at.fileInfo == e.at.fileInfo
                && errors[k].at.line == e.at.line
                && errors[k].cerr == e.cerr
                && (errors[k].what != e.what || errors[k].extra != e.extra || errors[k].fixme != e.fixme) ) {
                // distinct text, same line, same cerr — count it
                ++sameLineSameCerrCount;
                ++k;
            }
            // Emit the first instance with optional count suffixes.
            Error out = e;
            if ( dupCount > 1 ) {
                out.what += " (\xC3\x97" + to_string(dupCount) + ")";
            }
            if ( sameLineSameCerrCount > 0 ) {
                out.what += " (+" + to_string(sameLineSameCerrCount) + " more on this line)";
            }
            kept.push_back(out);
            i = k;  // skip past Rule 1 dups + Rule 2 same-line same-cerr siblings
        }
        errors.swap(kept);
    }

    void Program::linkError ( const string & str, const string & extra ) {
        aotErrors.emplace_back(str,extra,"",LineInfo(), CompilationError::missing_aot);
        if ( policies.fail_on_no_aot ) {
            failToCompile = true;
            errors.emplace_back("AOT link failed on " + str,extra,"",LineInfo(), CompilationError::missing_aot);
        }
    }

    Module * Program::addModule ( const string & name ) {
        if ( auto lm = library.findModule(name) ) {
            return lm;
        }
        if ( auto pm = Module::require(name) ) {
            library.addModule(pm);
            return pm;
        }
        return nullptr;
    }

    bool Program::addAlias ( const TypeDeclPtr & at ) {
        return thisModule->addAlias(at, true);
    }

    bool Program::addVariable ( const VariablePtr & var ) {
        return thisModule->addVariable(var, true);
    }

    bool Program::addStructure ( const StructurePtr & st ) {
        return thisModule->addStructure(st, true);
    }

    bool Program::addEnumeration ( const EnumerationPtr & st ) {
        return thisModule->addEnumeration(st, true);
    }

    FunctionPtr Program::findFunction(const string & mangledName) const {
        return thisModule->findFunction(mangledName);
    }

    bool Program::addFunction ( const FunctionPtr & fn ) {
        return thisModule->addFunction(fn, true);
    }

    bool Program::addGeneric ( const FunctionPtr & fn ) {
        return thisModule->addGeneric(fn, true);
    }

    bool Program::addStructureHandle ( const StructurePtr & st, const TypeAnnotationPtr & ann, const AnnotationArgumentList & arg ) {
        if ( ann->rtti_isStructureTypeAnnotation() ) {
            auto annotation = static_cast<StructureTypeAnnotation*>(ann->clone());
            annotation->name = st->name;
            string err;
            if ( annotation->create(st,arg,err) ) {
                return thisModule->addAnnotation(annotation, true);
            } else {
                error("can't create structure handle "+ann->name,err,"",st->at,CompilationError::cant_create_structure_annotation);
                return false;
            }
        } else {
            error("not a structure annotation "+ann->name,"","",st->at,CompilationError::invalid_structure_annotation);
            return false;
        }
    }

    Program::Program() {
        ref_count_magic = TRACK_PTR_PROGRAM;
        thisModule = make_unique<ModuleDas>();
        library.addBuiltInModule();
        library.addModule(thisModule.get());
    }

    TypeDecl * Program::makeTypeDeclaration(const LineInfo &at, const string &name) {
        das::vector<das::StructurePtr> structs;
        das::vector<das::AnnotationPtr> handles;
        das::vector<das::EnumerationPtr> enums;
        das::vector<das::TypeDeclPtr> aliases;
        library.findWithCallback(name, thisModule.get(), [&](Module * pm, const string &name, Module * inWhichModule) {
            library.findStructure(structs, pm, name, inWhichModule);
            library.findAnnotation(handles, pm, name, inWhichModule);
            library.findEnum(enums, pm, name, inWhichModule);
            library.findAlias(aliases, pm, name, inWhichModule);
        });

        if ( ((structs.size()!=0)+(handles.size()!=0)+(enums.size()!=0)+(aliases.size()!=0)) > 1 ) {
            string candidates = describeCandidates(structs);
            candidates += describeCandidates(handles, false);
            candidates += describeCandidates(enums, false);
            candidates += describeCandidates(aliases, false);
            error("undefined make type declaration type "+name,candidates,"",
                at,CompilationError::ambiguous_type);
            return nullptr;
        } else if ( structs.size() ) {
            if ( structs.size()==1 ) {
                auto pTD = new TypeDecl(structs.back());
                pTD->at = at;
                return pTD;
            } else {
                string candidates = describeCandidates(structs);
                error("too many options for "+name,candidates,"",
                    at,CompilationError::ambiguous_structure);
                return nullptr;
            }
        } else if ( handles.size() ) {
            if ( handles.size()==1 ) {
                if ( handles.back()->rtti_isHandledTypeAnnotation() ) {
                    auto pTD = new TypeDecl(Type::tHandle);
                    pTD->annotation = static_cast<TypeAnnotation *>(handles.back());
                    pTD->at = at;
                    return pTD;
                } else {
                    error("not a handled type annotation "+name,"","",
                        at,CompilationError::invalid_annotation);
                    return nullptr;
                }
            } else {
                string candidates = describeCandidates(handles);
                error("too many options for "+name, candidates, "",
                    at,CompilationError::ambiguous_annotation);
                return nullptr;
            }
        } else if ( enums.size() ) {
            if ( enums.size()==1 ) {
                auto pTD = new TypeDecl(enums.back());
                pTD->enumType = enums.back();
                pTD->at = at;
                return pTD;
            } else {
                string candidates = describeCandidates(enums);
                error("too many options for "+name,candidates,"",
                    at,CompilationError::ambiguous_enumeration);
                return nullptr;
            }
        } else if ( aliases.size() ) {
            if ( aliases.size()==1 ) {
                auto pTD = new TypeDecl(*aliases.back());
                pTD->at = at;
                return pTD;
            } else {
                string candidates = describeCandidates(aliases);
                error("too many options for "+name,candidates,"",
                    at,CompilationError::ambiguous_type_alias);
                return nullptr;
            }
        } else {
            auto tt = new TypeDecl(Type::alias);
            tt->alias = name;
            tt->at = at;
            return tt;
        }
    }

    ExprLooksLikeCall * Program::makeCall ( const LineInfo & at, const string & name ) {
        vector<ExprCallFactory *> ptr;
        string moduleName, funcName;
        splitTypeName(name, moduleName, funcName);
        if ( moduleName != "_" && moduleName != "__" ) {    // those are never found. in reality we may want to support this one day with "*" and "thisModuleName" accordingly
            library.foreach([&](Module * pm) -> bool {
                if ( auto pp = pm->findCall(funcName) ) {
                    ptr.push_back(pp);
                }
                return true;
            }, moduleName);
        }
        if ( ptr.size()==1 ) {
            return (*ptr.back())(at);
        } else if ( ptr.size()==0 ) {
            return new ExprCall(at,name);
        } else {
            error("too many options for " + name,"","", at, CompilationError::ambiguous_function);
            return new ExprCall(at,name);
        }
    }

    ExprLooksLikeCall * Program::makeCall ( const LineInfo & at, const LineInfo & atEnd, const string & name ) {
        auto res = makeCall(at, name);
        if ( res ) {
            res->atEnclosure = at;
            res->atEnclosure.last_column = atEnd.last_column;
            res->atEnclosure.last_line = atEnd.last_line;
        }
        return res;
    }

    ExpressionPtr Program::makeConst ( const LineInfo & at, const TypeDeclPtr & type, vec4f value ) {
        if ( type->ref || type->baseType==Type::tFixedArray ) return nullptr;
        switch ( type->baseType ) {
            case Type::tBool:           return new ExprConstBool(at, cast<bool>::to(value));
            case Type::tInt8:           return new ExprConstInt8(at, cast<int8_t>::to(value));
            case Type::tInt16:          return new ExprConstInt16(at, cast<int16_t>::to(value));
            case Type::tInt64:          return new ExprConstInt64(at, cast<int64_t>::to(value));
            case Type::tInt:            return new ExprConstInt(at, cast<int32_t>::to(value));
            case Type::tInt2:           return new ExprConstInt2(at, cast<int2>::to(value));
            case Type::tInt3:           return new ExprConstInt3(at, cast<int3>::to(value));
            case Type::tInt4:           return new ExprConstInt4(at, cast<int4>::to(value));
            case Type::tUInt8:          return new ExprConstUInt8(at, cast<uint8_t>::to(value));
            case Type::tUInt16:         return new ExprConstUInt16(at, cast<uint16_t>::to(value));
            case Type::tUInt64:         return new ExprConstUInt64(at, cast<uint64_t>::to(value));
            case Type::tUInt:           return new ExprConstUInt(at, cast<uint32_t>::to(value));
            case Type::tBitfield:       return new ExprConstBitfield(at, cast<uint32_t>::to(value));
            case Type::tBitfield8:      {
                auto res = new ExprConstBitfield(at, cast<uint8_t>::to(value));
                res->baseType = Type::tBitfield8;
                return res;
            }
            case Type::tBitfield16:     {
                auto res = new ExprConstBitfield(at, cast<uint16_t>::to(value));
                res->baseType = Type::tBitfield16;
                return res;
            }
            case Type::tBitfield64:     {
                auto res = new ExprConstBitfield(at, cast<uint64_t>::to(value));
                res->baseType = Type::tBitfield64;
                return res;
            }
            case Type::tUInt2:          return new ExprConstUInt2(at, cast<uint2>::to(value));
            case Type::tUInt3:          return new ExprConstUInt3(at, cast<uint3>::to(value));
            case Type::tUInt4:          return new ExprConstUInt4(at, cast<uint4>::to(value));
            case Type::tFloat:          return new ExprConstFloat(at, cast<float>::to(value));
            case Type::tFloat2:         return new ExprConstFloat2(at, cast<float2>::to(value));
            case Type::tFloat3:         return new ExprConstFloat3(at, cast<float3>::to(value));
            case Type::tFloat4:         return new ExprConstFloat4(at, cast<float4>::to(value));
            case Type::tDouble:         return new ExprConstDouble(at, cast<double>::to(value));
            case Type::tRange:          return new ExprConstRange(at, cast<range>::to(value));
            case Type::tURange:         return new ExprConstURange(at, cast<urange>::to(value));
            case Type::tRange64:        return new ExprConstRange64(at, cast<range64>::to(value));
            case Type::tURange64:       return new ExprConstURange64(at, cast<urange64>::to(value));
            default:                    DAS_ASSERTF(0, "we should not even be here"); return nullptr;
        }
    }

    StructurePtr Program::visitStructure(Visitor & vis, Structure * pst) {
        vis.preVisit(pst);
        pst->aliases.foreach([&](auto & alsv){
            vis.preVisitStructureAlias(pst, alsv->alias, alsv);
            vis.preVisit(alsv);
            auto alssv = alsv->visit(vis);
            if ( alssv ) alssv = vis.visit(alssv);
            if ( alssv ) alssv = vis.visitStructureAlias(pst, alssv->alias, alssv);
            if ( alssv!=alsv ) {
                pst->aliases.replace(alsv->alias, alssv);
                alsv = alssv;
            }
        });
        for ( auto & fi : pst->fields ) {
            vis.preVisitStructureField(pst, fi, &fi==&pst->fields.back());
            if ( fi.type ) {
                vis.preVisit(fi.type);
                fi.type = fi.type->visit(vis);
                fi.type = vis.visit(fi.type);
            }
            if ( fi.init && vis.canVisitStructureFieldInit(pst) ) {
                fi.init = fi.init->visit(vis);
            }
            vis.visitStructureField(pst, fi, &fi==&pst->fields.back());
        }
        return vis.visit(pst);
    }

    EnumerationPtr Program::visitEnumeration(Visitor & vis, Enumeration * penum) {
        vis.preVisit(penum);
        size_t count = 0;
        size_t total = penum->list.size();
        for ( auto & itf : penum->list ) {
            vis.preVisitEnumerationValue(penum, itf.name, itf.value, count==total);
            if ( itf.value ) itf.value = itf.value->visit(vis);
            itf.value = vis.visitEnumerationValue(penum, itf.name, itf.value, count==total);
            count ++;
        }
        return vis.visit(penum);
    }

    void Program::visitModules(Visitor & vis, bool visitGenerics) {
        vis.preVisitProgram(this);
        library.foreach([&](Module * pm) -> bool {
            visitModule(vis, pm, visitGenerics);
            return true;
        }, "*");
        vis.visitProgram(this);
    }

    void Program::visitModulesInOrder(Visitor & vis, bool visitGenerics) {
        vis.preVisitProgram(this);
        library.foreach_in_order([&](Module * pm) -> bool {
            visitModule(vis, pm, visitGenerics);
            return true;
        }, thisModule.get());
        vis.visitProgram(this);
    }

    void Program::visit(Visitor & vis, bool visitGenerics, bool sortStructures ) {
        vis.preVisitProgram(this);
        visitModule(vis, thisModule.get(), visitGenerics, sortStructures);
        vis.visitProgram(this);
    }

    static void collectStructDeps ( const TypeDeclPtr & type, Structure * owner, das_hash_set<Structure *> & deps ) {
        if ( !type ) return;
        if ( type->baseType == Type::tStructure && type->structType && type->structType != owner ) {
            if ( type->isPointer() ) return;   // pointers don't need full definition
            if ( !deps.insert(type->structType).second ) return;   // already visited
            // recurse into the struct's own fields
            for ( auto & field : type->structType->fields ) {
                collectStructDeps(field.type, owner, deps);
            }
        }
        // recurse into firstType / secondType for containers
        if ( type->firstType ) collectStructDeps(type->firstType, owner, deps);
        if ( type->secondType ) collectStructDeps(type->secondType, owner, deps);
        // recurse into argTypes (e.g. tuple, variant element types)
        for ( auto & argType : type->argTypes ) {
            collectStructDeps(argType, owner, deps);
        }
    }

    static void topoSortStructures ( vector<StructurePtr> & structs ) {
        if ( structs.size() <= 1 ) return;
        // build adjacency: struct -> set of structs it depends on (by value)
        das_hash_map<Structure *, das_hash_set<Structure *>> deps;
        das_hash_set<Structure *> allSet;
        for ( auto & sp : structs ) {
            allSet.insert(sp);
        }
        for ( auto & sp : structs ) {
            auto & d = deps[sp];
            for ( auto & field : sp->fields ) {
                collectStructDeps(field.type, sp, d);
            }
            // only keep deps that are in our set
            das_hash_set<Structure *> filtered;
            for ( auto dep : d ) {
                if ( allSet.count(dep) ) filtered.insert(dep);
            }
            d = das::move(filtered);
        }
        // Kahn's algorithm using vector as queue
        das_hash_map<Structure *, int> inDegree;
        for ( auto & [s, dd] : deps ) {
            inDegree[s] = (int)dd.size();
        }
        vector<Structure *> sorted;
        sorted.reserve(structs.size());
        // seed with zero-dependency structs
        for ( auto & sp : structs ) {
            if ( inDegree[sp] == 0 ) sorted.push_back(sp);
        }
        // process in FIFO order
        for ( size_t qi = 0; qi < sorted.size(); qi++ ) {
            auto s = sorted[qi];
            for ( auto & [other, dd] : deps ) {
                if ( dd.erase(s) ) {
                    inDegree[other]--;
                    if ( inDegree[other] == 0 ) sorted.push_back(other);
                }
            }
        }
        if ( sorted.size() != structs.size() ) return; // cycle - keep original order
        // reorder structs to match sorted order
        das_hash_map<Structure *, StructurePtr> byPtr;
        for ( auto & sp : structs ) byPtr[sp] = sp;
        for ( size_t i = 0; i < sorted.size(); i++ ) {
            structs[i] = byPtr[sorted[i]];
        }
    }

    void Program::visitModule(Visitor & vis, Module * thatModule, bool visitGenerics, bool sortStructures) {
        vis.preVisitModule(thatModule);
        // enumerations
        thatModule->enumerations.foreach([&](auto & penum){
            if ( vis.canVisitEnumeration(penum) ) {
                auto penumn = visitEnumeration(vis, penum);
                if ( penumn != penum ) {
                    thatModule->enumerations.replace(penum->name, penumn);
                    penum = penumn;
                }
            }
        });
        // structures
        if ( sortStructures ) {
            // collect, topologically sort, then visit
            vector<StructurePtr> allStructs;
            thatModule->structures.foreach([&](auto & spst){
                if ( vis.canVisitStructure(spst) ) {
                    allStructs.push_back(spst);
                }
            });
            topoSortStructures(allStructs);
            for ( auto & spst : allStructs ) {
                Structure * pst = spst;
                StructurePtr pstn = visitStructure(vis, pst);
                if ( pstn != pst ) {
                    thatModule->structures.replace(pst->name, pstn);
                }
            }
        } else {
            thatModule->structures.foreach([&](auto & spst){
                Structure * pst = spst;
                if ( vis.canVisitStructure(pst) ) {
                    StructurePtr pstn = visitStructure(vis, pst);
                    if ( pstn != pst ) {
                        thatModule->structures.replace(pst->name, pstn);
                        spst = pstn;
                    }
                }
            });
        }
        // aliases
        thatModule->aliasTypes.foreach([&](auto & alsv){
            vis.preVisitAlias(alsv, alsv->alias);
            vis.preVisit(alsv);
            auto alssv = alsv->visit(vis);
            if ( alssv ) alssv = vis.visit(alssv);
            if ( alssv ) alssv = vis.visitAlias(alssv, alssv->alias);
            if ( alssv!=alsv ) {
                thatModule->aliasTypes.replace(alssv->alias, alssv);
                alsv = alssv;
            }
        });
        // real things
        vis.preVisitProgramBody(this,thatModule);
        // globals
        vis.preVisitGlobalLetBody(this);
        thatModule->globals.foreach([&](auto & var){
            if ( vis.canVisitGlobalVariable(var) ) {
                vis.preVisitGlobalLet(var);
                if ( var->type ) {
                    vis.preVisit(var->type);
                    var->type = var->type->visit(vis);
                    var->type = vis.visit(var->type);
                }
                if ( var->init ) {
                    vis.preVisitGlobalLetInit(var, var->init);
                    var->init = var->init->visit(vis);
                    var->init = vis.visitGlobalLetInit(var, var->init);
                }
                auto varn = vis.visitGlobalLet(var);
                if ( varn!=var ) {
                    thatModule->globals.replace(var->name, varn);
                    var = varn;
                }
            }
        });
        vis.visitGlobalLetBody(this);
        // generics
        if ( visitGenerics ) {
            thatModule->generics.foreach([&](auto & fn){
                if ( !fn->builtIn || visitBuiltinFunctions ) {
                    auto nfn = fn->visit(vis);
                    if ( fn != nfn ) {
                        thatModule->generics.replace(fn->getMangledName(), nfn);
                        fn = nfn;
                        DAS_ASSERTF(false,"todo: take care of genericsByName?");
                    }
                }
            });
        }
        // functions
        thatModule->functions.foreach([&](auto & fn){
            if ( !fn->builtIn || visitBuiltinFunctions ) {
                if ( vis.canVisitFunction(fn) ) {
                    auto nfn = fn->visit(vis);
                    if ( fn != nfn ) {
                        thatModule->functions.replace(fn->getMangledName(), nfn);
                        fn = nfn;
                        DAS_ASSERTF(false,"todo: take care of functionsByName?");
                    }
                }
            }
        });
        vis.visitModule(thatModule);
    }

    bool Program::getOptimize() const {
        if ( policies.no_optimizations ) return false;
        auto arg = options.find("optimize",Type::tBool);
        if ( arg ) return arg->bValue;
        arg = options.find("no_optimization",Type::tBool);
        if ( arg ) return !arg->bValue;
        return true;
    }

    bool Program::getDebugger() const {
        return policies.debugger || options.getBoolOption("debugger",false);
    }

    bool Program::getProfiler() const {
        return policies.profiler || options.getBoolOption("profiler",false);
    }

    void Program::optimize(TextWriter & logs, ModuleGroup & libGroup) {
        bool logOpt = options.getBoolOption("log_optimization",false);
        bool logPass = options.getBoolOption("log_optimization_passes",false);
        bool log = logOpt || logPass;
        bool any, last;
        int optimizationRound = 1;
        if (log) {
            logs << *this << "\n";
        }
        do {
            if ( log ) logs << "OPTIMIZE " << optimizationRound << ":\n"; if ( logPass ) logs << *this;
            any = false;
            last = optimizationRefFolding(optimizationRound);    if ( failed() ) break;  any |= last;
            if ( log ) logs << "REF FOLDING: " << (last ? "optimized" : "nothing") << "\n"; if ( logPass ) logs << *this;
            last = optimizationUnused(logs, optimizationRound);    if ( failed() ) break;  any |= last;
            if ( log ) logs << "REMOVE UNUSED:" << (last ? "optimized" : "nothing") << "\n"; if ( logPass ) logs << *this;
            last = optimizationConstFolding(optimizationRound);  if ( failed() ) break;  any |= last;
            if ( log ) logs << "CONST FOLDING:" << (last ? "optimized" : "nothing") << "\n"; if ( logPass ) logs << *this;
            last = optimizationCondFolding(optimizationRound);  if ( failed() ) break;  any |= last;
            if ( log ) logs << "COND FOLDING:" << (last ? "optimized" : "nothing") << "\n"; if ( logPass ) logs << *this;
            last = optimizationBlockFolding(optimizationRound);  if ( failed() ) break;  any |= last;
            if ( log ) logs << "BLOCK FOLDING:" << (last ? "optimized" : "nothing") << "\n"; if ( logPass ) logs << *this;
            // this is here again for a reason
            last = optimizationUnused(logs, optimizationRound);    if ( failed() ) break;  any |= last;
            if ( log ) logs << "REMOVE UNUSED:" << (last ? "optimized" : "nothing") << "\n"; if ( logPass ) logs << *this;
            // now, user macros
            last = false;
            auto modMacro = [&](Module * mod) -> bool {    // we run all macros for each module
                if ( thisModule->isVisibleDirectly(mod) && mod!=thisModule.get() ) {
                    for ( const auto & pm : mod->optimizationMacros ) {
                        last |= pm->apply(this, thisModule.get());
                        if ( failed() ) {                       // if macro failed, we report it, and we are done
                            error("optimization macro " + mod->name + "::" + pm->name + " failed", "","",LineInfo(), CompilationError::runtime_macro);
                            return false;
                        }
                    }
                }
                return true;
            };
            Module::foreach(modMacro);
            if ( failed() ) break;
            any |= last;
            libGroup.foreach(modMacro,"*");
            if ( failed() ) break;
            any |= last;
            if ( log ) logs << "MACROS:" << (last ? "optimized" : "nothing") << "\n"; if ( logPass ) logs << *this;
            optimizationRound++;
        } while ( any );
    }

}
