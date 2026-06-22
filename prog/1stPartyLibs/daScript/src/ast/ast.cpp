#include "daScript/misc/platform.h"

#include "daScript/ast/ast.h"
#include "daScript/ast/ast_visitor.h"

namespace das {

    // local or global

    bool isLocalOrGlobal ( ExpressionPtr expr ) {
        if ( expr->rtti_isVar() ) {
            auto ev = static_cast<ExprVar*>(expr);
            return ev->local || !(ev->argument || ev->block);
        } else if ( expr->rtti_isAt() ) {
            auto ea = static_cast<ExprAt*>(expr);
            if ( ea->subexpr && ea->subexpr->type && ea->subexpr->type->baseType==Type::tFixedArray ) {
                return isLocalOrGlobal(ea->subexpr);
            }
        } else if ( expr->rtti_isField() ) {
            auto ef = static_cast<ExprField*>(expr);
            if ( ef->value && ef->value->type && (ef->value->type->baseType!=Type::tHandle || ef->value->type->isLocal()) ) {
                return isLocalOrGlobal(ef->value);
            }
        } else if ( expr->rtti_isSwizzle() ) {
            auto sw = static_cast<ExprSwizzle*>(expr);
            if ( sw->value ) {
                return isLocalOrGlobal(sw->value);
            }
        } else if ( expr->rtti_isCallLikeExpr() ) {
            if ( expr->type && expr->type->ref ) {
                return true;
            }
        }
        return false;
    }

    // AOT

    AotListBase * AotListBase::head = nullptr;

    SimNode* AotFactory::operator()(Context& ctx) const
    {
        if (is_cmres) {
            return ctx.code->makeNode<SimNode_AotCMRES>(fn, wrappedFn);
        } else {
            return ctx.code->makeNode<SimNode_Aot>(fn, wrappedFn);
        }
    }

    AotListBase::AotListBase( RegisterAotFunctions prfn ) {
        tail = head;
        head = this;
        regFn = prfn;
    }

    void AotListBase::registerAot ( AotLibrary & lib ) {
        auto it = head;
        while ( it ) {
            (*it->regFn)(lib);
            it = it->tail;
        }
    }

    // aot library

    DAS_THREAD_LOCAL(unique_ptr<AotLibrary>) g_AOT_lib;

    AotLibrary & getGlobalAotLibrary() {
        if ( !*g_AOT_lib ) {
            *g_AOT_lib = make_unique<AotLibrary>();
            AotListBase::registerAot(**g_AOT_lib);
        }
        return **g_AOT_lib;
    }

    void clearGlobalAotLibrary() {
        g_AOT_lib->reset();
    }

    // annotations

    string Annotation::getMangledName() const {
        return module ? module->name + "::" + name : name;
    }

    string FunctionAnnotation::aotName ( ExprCallFunc * call ) {
        return call->func->getAotBasicName();
    }

    void Annotation::log ( TextWriter & tw, const AnnotationDeclaration & decl ) const {
        tw << decl.annotation->getMangledName();
        if ( decl.arguments.size() ) {
            tw << "(";
            for (auto & arg : decl.arguments) {
                if (&arg != &decl.arguments.front()) {
                    tw << ",";
                }
                tw << arg.name << ":" << das_to_string(arg.type) << "=";
                switch (arg.type) {
                case Type::tBool:       tw << (arg.bValue ? "true" : "false"); break;
                case Type::tInt:        tw << arg.iValue; break;
                case Type::tFloat:      tw << arg.fValue; break;
                case Type::tString:     tw << "\"" << arg.sValue << "\""; break;
                default:                tw << "error"; break;
                }
            }
            tw << ")";
        }
    }

    string AnnotationDeclaration::getMangledName() const {
        TextWriter tw;
        tw << "[";
        annotation->log(tw, *this);
        tw << "]";
        return tw.str();
    }

    // enumeration

    string Enumeration::getMangledName() const {
        return (module ? module->name+"::"+name : name); // + "#" + das_to_string(baseType);
    }

    TypeDeclPtr Enumeration::makeBaseType() const {
        return new TypeDecl(baseType);
    }

    Type Enumeration::getEnumType() const {
        switch (baseType) {
        case Type::tInt8:
        case Type::tUInt8:
            return Type::tEnumeration8;
        case Type::tInt16:
        case Type::tUInt16:
            return Type::tEnumeration16;
        case Type::tInt:
        case Type::tUInt:
            return Type::tEnumeration;
        case Type::tInt64:
        case Type::tUInt64:
            return Type::tEnumeration64;
        default:
            DAS_ASSERTF(0, "we should not be here. unsupported enumeration base type.");
            return Type::none;
        }
    }

    TypeDeclPtr Enumeration::makeEnumType() const {
        TypeDeclPtr res = nullptr;
        switch (baseType) {
        case Type::tInt8:
        case Type::tUInt8:
            res = new TypeDecl(Type::tEnumeration8); break;
        case Type::tInt16:
        case Type::tUInt16:
            res = new TypeDecl(Type::tEnumeration16); break;
        case Type::tInt:
        case Type::tUInt:
            res = new TypeDecl(Type::tEnumeration); break;
        case Type::tInt64:
        case Type::tUInt64:
            res = new TypeDecl(Type::tEnumeration64); break;
        default:
            DAS_ASSERTF(0, "we should not be here. unsupported enumeration base type.");
            return nullptr;
        }
        res->enumType = const_cast<Enumeration*>(this);
        return res;
    }


    pair<ExpressionPtr,bool> Enumeration::find ( const string & na ) const {
        auto it = find_if(list.begin(), list.end(), [&](const EnumEntry & arg){
            return arg.name == na;
        });
        return it!=list.end() ?
            pair<ExpressionPtr,bool>(it->value,true) :
            pair<ExpressionPtr,bool>(nullptr,false);
    }

    int64_t Enumeration::find ( const string & na, int64_t def ) const {
        auto it = find_if(list.begin(), list.end(), [&](const EnumEntry & arg){
            return arg.name == na;
        });
        return it!=list.end() ? getConstExprIntOrUInt(it->value) : def;
    }

    string Enumeration::find ( int64_t va, const string & def ) const {
        auto vaRef = pair(va,true);
        for ( const auto & it : list ) {
            if (    it.value
                &&  it.value->rtti_isConstant()
                &&  (!it.value->type || it.value->type->isInteger())
                && tryGetConstExprIntOrUInt(it.value)==vaRef ) {
                return it.name;
            }
        }
        return def;
    }

    bool Enumeration::add ( const string & f, const LineInfo & att ) {
        return add(f, nullptr, att);
    }

    bool Enumeration::addI ( const string & f, int64_t value, const LineInfo & att ) {
        return add(f, new ExprConstInt64(value),att);
    }

    bool Enumeration::addIEx ( const string & f, const string & fcpp, int64_t value, const LineInfo & att ) {
        return addEx(f, fcpp, new ExprConstInt64(value),att);
    }

    bool Enumeration::addEx ( const string & na, const string & naCpp, ExpressionPtr expr, const LineInfo & att ) {
        auto it = find_if(list.begin(), list.end(), [&](const EnumEntry & arg){
            return arg.name == na;
        });
        if ( it == list.end() ) {
            EnumEntry ena;
            ena.name = na;
            ena.cppName = naCpp.empty() ? na : naCpp;
            ena.value = expr;
            ena.at = att;
            list.push_back(ena);
            return true;
        } else {
            return false;
        }
    }

    bool Enumeration::add ( const string & na, ExpressionPtr expr, const LineInfo & att ) {
        return addEx(na,"",expr,att);
    }

    // structure

    TypeDeclPtr Structure::findAlias ( const string & aliasName ) const {
        return aliases.find(aliasName);
    }

    uint64_t Structure::getOwnSemanticHash(HashBuilder & hb, das_set<Structure *> & dep, das_set<Annotation *> & adep) const {
        hb.updateString(getMangledName());
        hb.update(fields.size());
        for ( auto & fld : fields ) {
            hb.updateString(fld.name);
            hb.update(fld.type->getOwnSemanticHash(hb, dep, adep));
        }
        aliases.foreach([&](const TypeDeclPtr & atype) {
            hb.update(atype->getOwnSemanticHash(hb, dep, adep));
            return true;
        });
        return hb.getHash();
    }

    StructurePtr Structure::clone() const {
        auto cs = new Structure(name);
        cs->fields.reserve(fields.size());
        for ( auto & fd : fields ) {
            cs->fields.emplace_back(fd.name, new TypeDecl(*fd.type), fd.init ? fd.init->clone() : nullptr, fd.annotation, fd.moveSemantics, fd.at);
            cs->fields.back().flags = fd.flags;
        }
        aliases.foreach([&](const TypeDeclPtr & atype) -> bool {
            cs->aliases.insert(atype->alias, new TypeDecl(*atype));
            return true;
        });
        cs->at = at;
        cs->module = module;
        cs->flags = flags;
        cs->annotations = cloneAnnotationList(annotations);
        return cs;
    }

    bool Structure::isCompatibleCast ( const Structure & castS ) const {
        if (this == &castS) {
            return true;
        }
        auto *parentS = castS.parent;
        while ( parentS ) {
            if ( parentS == this ) {
                return true;
            }
            parentS = parentS->parent;
        }
        return false;
    }

    bool Structure::hasAnyInitializers() const {
        for ( const auto & fd : fields ) {
            if ( fd.init ) return true;
        }
        return false;
    }

    // True if this structure has any non-generated ctor method (Klass`Klass).
    // Ctor methods always live in the same module as the class (parser registers
    // them together), so we look up by name hash via functionsByName / genericsByName.
    // The `classParent == this` identity check defends against same-named classes
    // in other modules bleeding in through generic instantiation.
    bool Structure::hasUserConstructor() const {
        if ( !module ) return false;
        string ctorName = name + "`" + name;
        uint64_t hName = hash64z(ctorName.c_str());
        if ( auto kv = module->functionsByName.find(hName) ) {
            for ( auto * fn : kv->second ) {
                if ( !fn->generated && fn->classParent == this ) return true;
            }
        }
        if ( auto kv = module->genericsByName.find(hName) ) {
            for ( auto * fn : kv->second ) {
                if ( !fn->generated && fn->classParent == this ) return true;
            }
        }
        return false;
    }

    // True if this structure has a non-generated 0-arg-callable ctor (no args, or
    // all args default-initialized). Used by the synth-derived-ctor gate to verify
    // the parent has a default constructor reachable as Parent`Parent(self).
    bool Structure::hasUserDefaultConstructor() const {
        if ( !module ) return false;
        string ctorName = name + "`" + name;
        uint64_t hName = hash64z(ctorName.c_str());
        auto matches = [this]( Function * fn ) -> bool {
            if ( fn->generated ) return false;
            if ( fn->classParent != this ) return false;
            // arg 0 is always 'self'; arg 1+ must be default-initialized
            for ( size_t i=1; i<fn->arguments.size(); ++i ) {
                if ( !fn->arguments[i]->init ) return false;
            }
            return true;
        };
        if ( auto kv = module->functionsByName.find(hName) ) {
            for ( auto * fn : kv->second ) {
                if ( matches(fn) ) return true;
            }
        }
        if ( auto kv = module->genericsByName.find(hName) ) {
            for ( auto * fn : kv->second ) {
                if ( matches(fn) ) return true;
            }
        }
        return false;
    }

    // True if this class has a non-generated finalizer method. Class finalizers keep
    // the plain name "finalize" (parser: ast_structVarDef), so the registry lookup is
    // by that name with the classParent identity check filtering out other classes'
    // finalizers and free-function struct finalizers.
    bool Structure::hasUserFinalizer() const {
        if ( !module ) return false;
        uint64_t hName = hash64z("finalize");
        if ( auto kv = module->functionsByName.find(hName) ) {
            for ( auto * fn : kv->second ) {
                if ( !fn->generated && fn->classParent == this ) return true;
            }
        }
        if ( auto kv = module->genericsByName.find(hName) ) {
            for ( auto * fn : kv->second ) {
                if ( !fn->generated && fn->classParent == this ) return true;
            }
        }
        return false;
    }

    bool Structure::canCopy(bool tempMatters) const {
        if ( circularGuard ) return true;
        circularGuard = true;
        for ( const auto & fd : fields ) {
            if ( !fd.type->canCopy(tempMatters) ) {
                circularGuard = false;
                return false;
            }
        }
        circularGuard = false;
        return true;
    }

    bool Structure::canMove() const {
        if ( circularGuard ) return true;
        circularGuard = true;
        for ( const auto & fd : fields ) {
            if ( !fd.type->canMove() ) {
                circularGuard = false;
                return false;
            }
        }
        circularGuard = false;
        return true;
    }

    bool Structure::canCloneFromConst() const {
        if ( circularGuard ) return true;
        circularGuard = true;
        for ( const auto & fd : fields ) {
            if ( !fd.type->canCloneFromConst() ) {
                circularGuard = false;
                return false;
            }
        }
        circularGuard = false;
        return true;
    }

    bool Structure::canClone() const {
        if ( circularGuard ) return true;
        circularGuard = true;
        for ( const auto & fd : fields ) {
            if ( !fd.type->canClone() ) {
                circularGuard = false;
                return false;
            }
        }
        circularGuard = false;
        return true;
    }

    bool Structure::canAot( das_set<Structure *> & recAot ) const {
        for ( const auto & fd : fields ) {
            if ( !fd.type->canAot(recAot) )
                return false;
        }
        return true;
    }

    bool Structure::canAot() const {
        das_set<Structure *> recAot;
        return canAot(recAot);
    }

    bool Structure::isNoHeapType() const {
        for ( const auto & fd : fields ) {
            if ( !fd.type->isNoHeapType() )
                return false;
        }
        return true;
    }

    bool Structure::isPod() const {
        for ( const auto & fd : fields ) {
            if ( !fd.type->isPod() )
                return false;
        }
        return true;
    }

    bool Structure::isRawPod() const {
        for ( const auto & fd : fields ) {
            if ( !fd.type->isRawPod() )
                return false;
        }
        return true;
    }

    int Structure::getSizeOf() const {
        uint64_t size = getSizeOf64();
        DAS_ASSERTF(size<=0x7fffffff,"structure %s is too big, %lu",name.c_str(),(unsigned long)size);
        return (int) (size <= 0x7fffffff ? size : 1);
    }

    uint64_t Structure::getSizeOf64() const {
        if ( circularGuard ) return 1;
        circularGuard = true;
        uint64_t size = 0;
        const Structure * cppLayoutParent = nullptr;
        for ( const auto & fd : fields ) {
            int fieldAlignemnt = fd.type->getAlignOf();
            int al = fieldAlignemnt - 1;
            if ( cppLayout ) {
                auto fp = findFieldParent(fd.name);
                if ( fp!=cppLayoutParent ) {
                    if (DAS_NON_POD_PADDING || !cppLayoutNotPod) {
                        size = cppLayoutParent ? cppLayoutParent->getSizeOf64() : 0;
                    }
                    cppLayoutParent = fp;
                }
            }
            size = (size + al) & ~al;
            size += fd.type->getSizeOf64();
        }
        circularGuard = false;
        int al = getAlignOf() - 1;
        size = (size + al) & ~al;
        return size;
    }

    uint64_t Structure::getSizeOf64(bool & failed) const {
        if ( circularGuard ) return 1;
        circularGuard = true;
        uint64_t size = 0;
        const Structure * cppLayoutParent = nullptr;
        for ( const auto & fd : fields ) {
            int fieldAlignemnt = fd.type->getAlignOfFailed(failed);
            int al = fieldAlignemnt - 1;
            if ( cppLayout ) {
                auto fp = findFieldParent(fd.name);
                if ( fp!=cppLayoutParent ) {
                    if (DAS_NON_POD_PADDING || !cppLayoutNotPod) {
                        size = cppLayoutParent ? cppLayoutParent->getSizeOf64(failed) : 0;
                    }
                    cppLayoutParent = fp;
                }
            }
            size = (size + al) & ~al;
            size += fd.type->getSizeOf64(failed);
        }
        circularGuard = false;
        int al = getAlignOfFailed(failed) - 1;
        size = (size + al) & ~al;
        return size;
    }

    int Structure::getAlignOf() const {
        if ( circularGuard ) return 1;
        circularGuard = true;
        int align = 1;
        for ( const auto & fd : fields ) {
            align = das::max ( fd.type->getAlignOf(), align );
        }
        circularGuard = false;
        return align;
    }

    int Structure::getAlignOfFailed(bool & failed) const {
        if ( circularGuard ) return 1;
        circularGuard = true;
        int align = 1;
        for ( const auto & fd : fields ) {
            align = das::max ( fd.type->getAlignOfFailed(failed), align );
        }
        circularGuard = false;
        return align;
    }

    Structure::FieldDeclarationRef Structure::findFieldRef ( const string & fieldName ) const {
        auto pField = findField(fieldName);
        if ( pField ) {
            FieldDeclarationRef ref;
            ref.owner = const_cast<Structure *>(this);
            ref.index = int32_t(pField - &fields[0]);
            return ref;
        } else {
            return FieldDeclarationRef{};
        }
    }

    const Structure::FieldDeclaration * Structure::findField ( const string & na ) const {
        if ( fieldLookup.size()==fields.size() ) {
            auto it = fieldLookup.find(na);
            if ( it == fieldLookup.end() ) return nullptr;
            return &fields[it->second];
        } else {
            for ( const auto & fd : fields ) {
                if ( fd.name==na ) {
                    return &fd;
                }
            }
        }
        return nullptr;
    }

    const Structure * Structure::findFieldParent ( const string & na ) const {
        if ( parent ) {
            if ( parent->findField(na) ) {
                return parent;
            }
        }
        if ( findField(na) ) {
            return this;
        }
        return nullptr;
    }

    string Structure::getMangledName() const {
        return module ? module->name+"::"+name : name;
    }

    bool Structure::unsafeInit ( das_set<Structure *> & dep ) const {
        if ( safeWhenUninitialized ) return false;
        if ( hasDefaultInitializer ) return true;
        for ( const auto & fd : fields ) {
            if ( fd.init ) return true;
            if ( fd.type && fd.type->unsafeInit(dep) ) return true;
        }
        return false;
    }

    bool Structure::needInScope(das_set<Structure *> & dep) const {   // &&
        for ( const auto & fd : fields ) {
            if ( fd.type && fd.type->needInScope(dep) ) {
                return true;
            }
        }
        return false;
    }

    bool Structure::canBePlacedInContainer(das_set<Structure *> & dep) const {   // &&
        for ( const auto & fd : fields ) {
            if ( fd.type && !fd.type->canBePlacedInContainer(dep) ) {
                return false;
            }
        }
        return true;
    }

    bool Structure::hasNonTrivialCtor(das_set<Structure *> & dep) const {   // &&
        for ( const auto & fd : fields ) {
            if ( fd.type && fd.type->hasNonTrivialCtor(dep) ) {
                return true;
            }
        }
        return false;
    }

    bool Structure::hasNonTrivialDtor(das_set<Structure *> & dep) const {   // &&
        for ( const auto & fd : fields ) {
            if ( fd.type && fd.type->hasNonTrivialDtor(dep) ) {
                return true;
            }
        }
        return false;
    }

    bool Structure::hasNonTrivialCopy(das_set<Structure *> & dep) const {   // &&
        for ( const auto & fd : fields ) {
            if ( fd.type && fd.type->hasNonTrivialCopy(dep) ) {
                return true;
            }
        }
        return false;
    }

    bool Structure::isExprTypeAnywhere(das_set<Structure *> & dep) const {   // &&
        for ( const auto & fd : fields ) {
            if ( fd.type && fd.type->isExprTypeAnywhere(dep) ) {
                return true;
            }
        }
        return false;
    }

    bool Structure::isSafeToDelete(das_set<Structure *> & dep) const {   // &&
        for ( const auto & fd : fields ) {
            if ( fd.type && !fd.type->isSafeToDelete(dep) && fd.annotation.find("do_not_delete",Type::tBool)==nullptr ) {
                return false;
            }
        }
        return true;
    }

    bool Structure::isLocal(das_set<Structure *> & dep) const {   // &&
        for ( const auto & fd : fields ) {
            if ( fd.type && !fd.type->isLocal(dep) ) {
                return false;
            }
        }
        return true;
    }

    bool Structure::hasStringData( das_set<void*> & dep ) const { // ||
        for ( const auto & fd : fields ) {
            if ( fd.type && fd.type->hasStringData(dep) ) {
                return true;
            }
        }
        return false;
    }

    bool Structure::hasClasses(das_set<Structure *> & dep) const {    // ||
        if ( isClass ) return true;
        for ( const auto & fd : fields ) {
            if ( fd.type && fd.type->hasClasses(dep) ) {
                return true;
            }
        }
        return false;
    }

    bool Structure::isTemp(das_set<Structure *> & dep) const {    // ||
        for ( const auto & fd : fields ) {
            if ( fd.type && fd.type->isTemp(true, true, dep) ) {
                return true;
            }
        }
        return false;
    }

    bool Structure::isShareable(das_set<Structure *> & dep) const {    // &&
        for ( const auto & fd : fields ) {
            if ( fd.type && !fd.type->isShareable(dep) ) {
                return false;
            }
        }
        return true;
    }

    // variable

    VariablePtr Variable::clone() const {
        auto pVar = new Variable();
        pVar->name = name;
        pVar->aka = aka;
        pVar->type = new TypeDecl(*type);
        if ( init )
            pVar->init = init->clone();
        if ( source )
            pVar->source = source->clone();
        pVar->at = at;
        pVar->flags = flags;
        pVar->access_flags = access_flags;
        pVar->initStackSize = initStackSize;
        pVar->annotation = annotation;
        return pVar;
    }

    string Variable::getMangledName() const {
        string mn = module ? module->name+"::"+name : name;
        return mn + " " + type->getMangledName();
    }

    uint64_t Variable::getMangledNameHash() const {
        auto mangledName = getMangledName();
        return getMNHash(mangledName);
    }

    uint64_t Variable::getMNHash(const string &mangledName) {
        return hash_blockz64((uint8_t *)mangledName.c_str());
    }

    bool Variable::isAccessUnused() const {
        return !(access_get || access_init || access_pass || access_ref);
    }

    bool Variable::isAccessDummy() const {
        return !(access_get || access_pass || access_ref);
    }

    bool Variable::isCtorInitialized() const {
        if ( !init ) {
            return false;
        }
        if ( !type->hasNonTrivialCtor() ) {
            return false;
        }
        if ( init->rtti_isCallFunc() ) {
            auto cfun = static_cast<ExprCallFunc*>(init);
            if ( cfun->func && cfun->func->isTypeConstructor ) {
                return true;
            }
        }
        return false;
    }

    // function

    AnnotationList cloneAnnotationList ( const AnnotationList & list ) {
        AnnotationList clist;
        for ( auto & ann : list ) {
            auto decl = new AnnotationDeclaration();
            decl->annotation = ann->annotation;
            decl->arguments = ann->arguments;
            decl->at = ann->at;
            clist.push_back(decl);
        }
        return clist;
    }

    FunctionPtr Function::clone() const {
        auto cfun = new Function();
        cfun->name = name;
        for ( const auto & arg : arguments ) {
            cfun->arguments.push_back(arg->clone());
        }
        cfun->annotations = cloneAnnotationList(annotations);
        cfun->result = new TypeDecl(*result);
        cfun->body = body->clone();
        cfun->index = -1;
        cfun->totalStackSize = 0;
        cfun->totalGenLabel = totalGenLabel;
        cfun->at = at;
        cfun->atDecl = atDecl;
        cfun->module = nullptr;
        cfun->flags = flags;
        cfun->moreFlags = moreFlags;
        cfun->sideEffectFlags = sideEffectFlags;
        cfun->inferStack = inferStack;
        cfun->classParent = classParent;
        // copy aliasing info
        cfun->resultAliases = resultAliases;
        cfun->argumentAliases = argumentAliases;
        cfun->resultAliasesGlobals = resultAliasesGlobals;
        return cfun;
    }

    LineInfo Function::getConceptLocation(const LineInfo & atL) const {
        return inferStack.size() ? inferStack[0].at : atL;
    }

    string Function::getLocationExtra() const {
        if ( !inferStack.size() ) {
            return "";
        }
        TextWriter ss;
        ss << "\nwhile compiling " << describe() << "\n";
        for ( const auto & ih : inferStack ) {
            ss << "instanced from " << ih.func->describe() << " at " << ih.at.describe() << "\n";
        }
        return ss.str();
    }

    string Function::describeName(DescribeModule moduleName) const {
        TextWriter ss;
        if ( moduleName==DescribeModule::yes && module && !module->name.empty() ) {
            ss << module->name << "::";
        }
        if ( !isalpha(name[0]) && name[0]!='_' && name[0]!='`' ) {
            ss << "operator ";
        }
        ss << name;
        return ss.str();
    }

    string Function::describe(DescribeModule moduleName, DescribeExtra extra) const {
        TextWriter ss;
        if ( moduleName==DescribeModule::yes && module && !module->name.empty() ) {
            ss << module->name << "::";
        }
        if ( !isalpha(name[0]) && name[0]!='_' && name[0]!='`' ) {
            ss << "operator ";
        }
        ss << name;
        if ( arguments.size() ) {
            ss << "(";
            for ( auto & arg : arguments ) {
                ss << arg->name << ": " << *arg->type;
                if ( extra==DescribeExtra::yes && arg->init ) {
                    ss << " = " << *arg->init;
                }
                if ( arg != arguments.back() ) ss << "; ";
            }
            ss << ")";
        }
        if ( result ) {
            ss << ": " << result->describe();
        }
        return ss.str();
    }

    string Function::getMangledName() const {
        TextWriter ss;
        getMangledName(ss);
        return ss.str();
    }

    void Function::getMangledName(TextWriter & ss) const {
        if ( module && !module->name.empty() ) {
            ss << "@" << module->name << "::";
        }
        ss << name;
        for ( auto & arg : arguments ) {
            ss << " ";
            arg->type->getMangledName(ss);
        }
        for ( auto & ann : annotations ) {
            if (ann->annotation && ann->annotation->rtti_isFunctionAnnotation() ) {
                auto fna = static_cast<FunctionAnnotation*>(ann->annotation);
                string mname;
                fna->appendToMangledName((Function *)this, *ann, mname);
                if ( !mname.empty() ) {
                    ss << " %<" << mname << ">";
                }
            }
        }
    }

    uint64_t Function::getMangledNameHash() const {
        auto mangledName = getMangledName();
        return hash_blockz64((uint8_t *)mangledName.c_str());
    }

    VariablePtr Function::findArgument(const string & na) {
        for ( auto & arg : arguments ) {
            if ( arg->name==na ) {
                return arg;
            }
        }
        return nullptr;
    }

    FunctionPtr Function::visit(Visitor & vis) {
        vis.preVisit(this);
        for ( auto & arg : arguments ) {
            vis.preVisitArgument(this, arg, arg==arguments.back() );
            if ( arg->type ) {
                vis.preVisit(arg->type);
                arg->type = arg->type->visit(vis);
                arg->type = vis.visit(arg->type);
            }
            if ( arg->init && vis.canVisitArgumentInit(this,arg,arg->init) ) {
                vis.preVisitArgumentInit(this, arg, arg->init);
                arg->init = arg->init->visit(vis);
                if ( arg->init ) {
                    arg->init = vis.visitArgumentInit(this, arg, arg->init);
                }
            }
            arg = vis.visitArgument(this, arg, arg==arguments.back() );
        }
        if ( result ) {
            vis.preVisit(result);
            result = result->visit(vis);
            result = vis.visit(result);
        }
        if ( body ) {
            vis.preVisitFunctionBody(this, body);
            if ( body ) body = body->visit(vis);
            if ( body ) body = vis.visitFunctionBody(this, body);
        }
        return vis.visit(this);
    }

    bool Function::isGeneric() const {
        for ( const auto & ann : annotations ) {
            if (ann->annotation) {
                auto fna = static_cast<FunctionAnnotation*>(ann->annotation);
                if (fna->isGeneric()) {
                    return true;
                }
            }
        }
        for ( auto & arg : arguments ) {
            if ( arg->type->isAuto() && !arg->init ) {
                return true;
            }
        }
        return false;
    }

    string Function::getAotArgumentPrefix(ExprCallFunc * call, int argIndex) const {
        for ( auto & ann : annotations ) {
            if ( ann->annotation->rtti_isFunctionAnnotation() ) {
                auto pAnn = static_cast<FunctionAnnotation*>(ann->annotation);
                return pAnn->aotArgumentPrefix(call, argIndex);
            }
        }
        return "";
    }

    string Function::getAotArgumentSuffix(ExprCallFunc * call, int argIndex) const {
        for ( auto & ann : annotations ) {
            if ( ann->annotation->rtti_isFunctionAnnotation() ) {
                auto pAnn = static_cast<FunctionAnnotation*>(ann->annotation);
                return pAnn->aotArgumentSuffix(call, argIndex);
            }
        }
        return "";
    }

    string Function::getAotName(ExprCallFunc * call) const {
        for ( auto & ann : annotations ) {
            if ( ann->annotation->rtti_isFunctionAnnotation() ) {
                auto pAnn = static_cast<FunctionAnnotation*>(ann->annotation);
                return pAnn->aotName(call);
            }
        }
        return call->func->getAotBasicName();
    }

    FunctionPtr Function::setSideEffects ( SideEffects seFlags ) {
        sideEffectFlags = uint32_t(seFlags) & ~uint32_t(SideEffects::unsafe);
        if ( uint32_t(seFlags) & uint32_t(SideEffects::unsafe) ) {
            unsafeOperation = true;
        }
        return this;
    }

    FunctionPtr Function::setCustomProperty() {
        isCustomProperty = true;
        return this;
    }

    FunctionPtr Function::setAotTemplate() {
        aotTemplate = true;
        return this;
    }

    FunctionPtr Function::setAnyTemplate() {
        anyTemplate = true;
        return this;
    }

    FunctionPtr Function::setTempResult() {
        result->temporary = true;
        return this;
    }

    FunctionPtr Function::setCaptureString() {
        captureString = true;
        return this;
    }

    FunctionPtr Function::setNoDiscard() {
        nodiscard = true;
        return this;
    }

    FunctionPtr Function::addToModule ( Module & mod, SideEffects seFlags ) {
        setSideEffects(seFlags);
        if (!mod.addFunction(this)) {
            DAS_FATAL_ERROR("addExtern(%s) failed in module %s\n", name.c_str(), mod.name.c_str());
        }
        return this;
    }

    Function * Function::getOriginPtr() const {
        if ( fromGeneric ) {
            auto origin = fromGeneric;
            while ( origin->fromGeneric ) {
                origin = origin->fromGeneric;
            }
            return origin;
        } else {
            return nullptr;
        }
    }

    FunctionPtr Function::getOrigin() const {
        if ( fromGeneric ) {
            auto origin = fromGeneric;
            while ( origin->fromGeneric ) {
                origin = origin->fromGeneric;
            }
            return origin;
        } else {
            return nullptr;
        }
    }

    // built-in function

    BuiltInFunction::BuiltInFunction ( const char * fn, const char * fnCpp ) {
        builtIn = true;
        name = fn;
        cppName = fnCpp ? fnCpp : "";
    }

    void BuiltInFunction::construct (const vector<TypeDeclPtr> & args ) {
        this->totalStackSize = sizeof(Prologue);
        for ( size_t argi=1; argi != args.size(); ++argi ) {
            auto arg = new Variable();
            arg->name = "arg" + to_string(argi-1);
            arg->type = args[argi];
            if ( arg->type->baseType==Type::fakeContext ) {
                arg->init = new ExprFakeContext(at);
                arg->init->generated = true;
            } else if ( arg->type->baseType==Type::fakeLineInfo ) {
                arg->init = new ExprFakeLineInfo(at);
                arg->init->generated = true;
            }
            if ( arg->type->isTempType() ) {
                arg->type->implicit = true;
            }
            this->arguments.push_back(arg);
        }
        result = args[0];
        if ( result->isRefType() && !result->ref ) {
            if ( result->canCopy() ) {
                copyOnReturn = true;
                moveOnReturn = false;
            } else if ( result->canMove() ) {
                copyOnReturn = false;
                moveOnReturn = true;
            } else if ( result->ref ) {
                // its ref, so its fine
            } else if ( result->hasNonTrivialCtor() ) {
                // we can initialize it locally
            } else {
                DAS_FATAL_ERROR("BuiltInFn %s can't be bound. It returns values which can't be copied or moved\n", name.c_str());
            }
        } else {
            copyOnReturn = false;
            moveOnReturn = false;
        }
    }

    void BuiltInFunction::constructExternal (const vector<TypeDeclPtr> & args ) {
        this->totalStackSize = sizeof(Prologue);
        for ( size_t argi=1; argi != args.size(); ++argi ) {
            auto arg = new Variable();
            arg->name = "arg" + to_string(argi-1);
            arg->type = args[argi];
            if ( arg->type->baseType==Type::fakeContext ) {
                arg->init = new ExprFakeContext(at);
                arg->init->generated = true;
            } else if ( arg->type->baseType==Type::fakeLineInfo ) {
                arg->init = new ExprFakeLineInfo(at);
                arg->init->generated = true;
            }
            this->arguments.push_back(arg);
        }
        result = args[0];
        if ( result->isRefType() && !result->ref ) {
            if ( result->canCopy() ) {
                copyOnReturn = true;
                moveOnReturn = false;
            } else if ( result->canMove() ) {
                copyOnReturn = false;
                moveOnReturn = true;
            } else if ( !result->ref ) {
                DAS_FATAL_ERROR("ExternalFn %s can't be bound. It returns values which can't be copied or moved\n", name.c_str());
            }
        } else {
            copyOnReturn = false;
            moveOnReturn = false;
        }
    }

    void BuiltInFunction::constructInterop (const vector<TypeDeclPtr> & args ) {
        this->totalStackSize = sizeof(Prologue);
        for ( size_t argi=1; argi!=args.size(); ++argi ) {
            auto arg = new Variable();
            arg->name = "arg" + to_string(argi-1);
            arg->type = args[argi];
            if ( arg->type->baseType==Type::fakeContext ) {
                arg->init = new ExprFakeContext(at);
                arg->init->generated = true;
            } else if ( arg->type->baseType==Type::fakeLineInfo ) {
                arg->init = new ExprFakeLineInfo(at);
                arg->init->generated = true;
            }
            this->arguments.push_back(arg);
        }
        result = args[0];
        if ( result->isRefType() && !result->ref ) {
            if ( result->canCopy() ) {
                copyOnReturn = true;
                moveOnReturn = false;
            } else if ( result->canMove() ) {
                copyOnReturn = false;
                moveOnReturn = true;
            } else if ( !result->ref ) {
                DAS_FATAL_ERROR("InteropFn %s can't be bound. It returns values which can't be copied or moved\n", name.c_str());
            }
        } else {
            copyOnReturn = false;
            moveOnReturn = false;
        }
    }

    // expression

    string Expression::describe() const {
        TextWriter ss;
        ss << *this;
        return ss.str();
    }

    ExpressionPtr Expression::clone( ExpressionPtr expr ) const {
        if ( !expr ) {
            DAS_ASSERTF(0,
                   "its not ok to clone Expression as is."
                   "if we are here, this means that ::clone function is not written correctly."
                   "incorrect clone function can be found on the stack above this.");
            return nullptr;
        }
        expr->at = at;
        expr->type = type ? new TypeDecl(*type) : nullptr;
        expr->genFlags = genFlags;
        return expr;
    }

    ExpressionPtr ExprConst::clone ( ExpressionPtr expr ) const {
        Expression::clone(expr);
        auto cexpr = static_cast<ExprConst *>(expr);
        cexpr->baseType = baseType;
        cexpr->value = value;
        cexpr->foldedNonConst = foldedNonConst;
        cexpr->promotedFromInt = promotedFromInt;
        cexpr->inexactFloatPromotion = inexactFloatPromotion;
        return expr;
    }

    ExpressionPtr Expression::autoDereference ( ExpressionPtr expr ) {
        if ( expr->type && !expr->type->isAutoOrAlias() && expr->type->isRef() && !expr->type->isRefType() ) {
            auto ar2l = new ExprRef2Value();
            ar2l->subexpr = expr;
            ar2l->at = expr->at;
            ar2l->type = new TypeDecl(*expr->type);
            ar2l->type->ref = false;
            return ar2l;
        } else {
            return expr;
        }
    }

   // Reader

    ExpressionPtr ExprReader::visit(Visitor & vis) {
        if ( !vis.canVisitReader(this) ) return this;
        vis.preVisit(this);
        return vis.visit(this);
    }

    ExpressionPtr ExprReader::clone( ExpressionPtr expr ) const {
        auto cexpr = clonePtr<ExprReader>(expr);
        Expression::clone(cexpr);
        cexpr->macro = macro;
        cexpr->sequence = sequence;
        return cexpr;
    }

    // Label

    ExpressionPtr ExprLabel::visit(Visitor & vis) {
        if ( !vis.canVisitLabel(this) ) return this;
        vis.preVisit(this);
        return vis.visit(this);
    }

    ExpressionPtr ExprLabel::clone( ExpressionPtr expr ) const {
        auto cexpr = clonePtr<ExprLabel>(expr);
        Expression::clone(cexpr);
        cexpr->label = label;
        cexpr->comment = comment;
        return cexpr;
    }

    // Goto

    ExpressionPtr ExprGoto::visit(Visitor & vis) {
        vis.preVisit(this);
        if ( subexpr ) {
            subexpr = subexpr->visit(vis);
        }
        return vis.visit(this);
    }

    ExpressionPtr ExprGoto::clone( ExpressionPtr expr ) const {
        auto cexpr = clonePtr<ExprGoto>(expr);
        Expression::clone(cexpr);
        cexpr->label = label;
        if ( subexpr ) {
            cexpr->subexpr = subexpr->clone();
        }
        return cexpr;
    }

    // ExprRef2Value

    ExpressionPtr ExprRef2Value::visit(Visitor & vis) {
        vis.preVisit(this);
        subexpr = subexpr->visit(vis);
        return vis.visit(this);
    }

    ExpressionPtr ExprRef2Value::clone( ExpressionPtr expr ) const {
        auto cexpr = clonePtr<ExprRef2Value>(expr);
        Expression::clone(cexpr);
        cexpr->subexpr = subexpr->clone();
        return cexpr;
    }

    void ExprRef2Value::markNoDiscard() {
        subexpr->markNoDiscard();
    }

    // ExprRef2Ptr

    ExpressionPtr ExprRef2Ptr::visit(Visitor & vis) {
        vis.preVisit(this);
        subexpr = subexpr->visit(vis);
        return vis.visit(this);
    }

    ExpressionPtr ExprRef2Ptr::clone( ExpressionPtr expr ) const {
        auto cexpr = clonePtr<ExprRef2Ptr>(expr);
        Expression::clone(cexpr);
        cexpr->subexpr = subexpr->clone();
        return cexpr;
    }

    void ExprRef2Ptr::markNoDiscard() {
        subexpr->markNoDiscard();
    }

    // ExprPtr2Ref

    ExpressionPtr ExprPtr2Ref::visit(Visitor & vis) {
        vis.preVisit(this);
        subexpr = subexpr->visit(vis);
        return vis.visit(this);
    }

    ExpressionPtr ExprPtr2Ref::clone( ExpressionPtr expr ) const {
        auto cexpr = clonePtr<ExprPtr2Ref>(expr);
        Expression::clone(cexpr);
        cexpr->subexpr = subexpr->clone();
        cexpr->assumeNoAlias = assumeNoAlias;
        return cexpr;
    }

    // ExprAddr

    ExpressionPtr ExprAddr::visit(Visitor & vis) {
        vis.preVisit(this);
        if ( funcType ) {
            vis.preVisit(funcType);
            funcType = funcType->visit(vis);
            funcType = vis.visit(funcType);
        }
        return vis.visit(this);
    }

    ExpressionPtr ExprAddr::clone( ExpressionPtr expr ) const {
        auto cexpr = clonePtr<ExprAddr>(expr);
        Expression::clone(cexpr);
        cexpr->target = target;
        cexpr->func = func;
        if (funcType) cexpr->funcType = new TypeDecl(*funcType);
        return cexpr;
    }

    // ExprNullCoalescing

    ExpressionPtr ExprNullCoalescing::visit(Visitor & vis) {
        vis.preVisit(this);
        subexpr = subexpr->visit(vis);
        vis.preVisitNullCoaelescingDefault(this, defaultValue);
        defaultValue = defaultValue->visit(vis);
        return vis.visit(this);
    }

    ExpressionPtr ExprNullCoalescing::clone( ExpressionPtr expr ) const {
        auto cexpr = clonePtr<ExprNullCoalescing>(expr);
        ExprPtr2Ref::clone(cexpr);
        cexpr->defaultValue = defaultValue->clone();
        return cexpr;
    }

    void ExprNullCoalescing::markNoDiscard() {
        subexpr->markNoDiscard();
        defaultValue->markNoDiscard();
    }

    // ConstBitfield

    ExpressionPtr ExprConstBitfield::clone( ExpressionPtr expr ) const {
        auto cexpr = clonePtr<ExprConstBitfield>(expr);
        ExprConstT<uint64_t,ExprConstBitfield>::clone(cexpr);
        if ( bitfieldType ) {
            cexpr->bitfieldType = new TypeDecl(*bitfieldType);
        }
        return cexpr;
    }

    // ConstPtr

    ExpressionPtr ExprConstPtr::clone( ExpressionPtr expr ) const {
        auto cexpr = clonePtr<ExprConstPtr>(expr);
        ExprConst::clone(cexpr);
        cexpr->isSmartPtr = isSmartPtr;
        if ( ptrType ) {
            cexpr->ptrType = new TypeDecl(*ptrType);
        }
        return cexpr;
    }

    // ConstEnumeration

    ExpressionPtr ExprConstEnumeration::visit(Visitor & vis) {
        vis.preVisit((ExprConst *)this);
        vis.preVisit(this);
        auto res = vis.visit(this);
        if ( res != this )
            return res;
        return vis.visit((ExprConst *)this);
    }

    ExpressionPtr ExprConstEnumeration::clone( ExpressionPtr expr ) const {
        auto cexpr = clonePtr<ExprConstEnumeration>(expr);
        ExprConst::clone(cexpr);
        cexpr->enumType = enumType;
        cexpr->text = text;
        return cexpr;
    }

    // ConstString

    ExpressionPtr ExprConstString::visit(Visitor & vis) {
        vis.preVisit((ExprConst *)this);
        vis.preVisit(this);
        auto res = vis.visit(this);
        if ( res != this )
            return res;
        return vis.visit((ExprConst *)this);
    }

    ExpressionPtr ExprConstString::clone( ExpressionPtr expr ) const {
        auto cexpr = clonePtr<ExprConstString>(expr);
        ExprConst::clone(cexpr);
        cexpr->text = text;
        return cexpr;
    }

    string TypeDecl::typeMacroName() const {
        if ( typeMacroExpr.size()<1 ) return "";
        if ( typeMacroExpr[0]->rtti_isStringConstant() ) {
            return ((ExprConstString *)typeMacroExpr[0])->text;
        } else {
            return "";
        }
    }

    // ExprStaticAssert

    ExpressionPtr ExprStaticAssert::clone( ExpressionPtr expr ) const {
        auto cexpr = clonePtr<ExprStaticAssert>(expr);
        ExprLooksLikeCall::clone(cexpr);
        return cexpr;
    }

    // ExprAssert

    ExpressionPtr ExprAssert::clone( ExpressionPtr expr ) const {
        auto cexpr = clonePtr<ExprAssert>(expr);
        ExprLooksLikeCall::clone(cexpr);
        cexpr->isVerify = isVerify;
        return cexpr;
    }

    // ExprQuote

    ExpressionPtr ExprQuote::visit(Visitor & vis) {
        vis.preVisit(this);
        if ( vis.canVisitQuoteSubexpression(this) ) {
            for ( auto & arg : arguments ) {
                vis.preVisitLooksLikeCallArg(this, arg, arg==arguments.back());
                arg = arg->visit(vis);
                arg = vis.visitLooksLikeCallArg(this, arg, arg==arguments.back());
            }
        }
        return vis.visit(this);
    }

    ExpressionPtr ExprQuote::clone( ExpressionPtr expr ) const {
        auto cexpr = clonePtr<ExprQuote>(expr);
        ExprLooksLikeCall::clone(cexpr);
        return cexpr;
    }

    // ExprDebug

    ExpressionPtr ExprDebug::clone( ExpressionPtr expr ) const {
        auto cexpr = clonePtr<ExprDebug>(expr);
        ExprLooksLikeCall::clone(cexpr);
        return cexpr;
    }

    // ExprMemZero

    ExpressionPtr ExprMemZero::clone( ExpressionPtr expr ) const {
        auto cexpr = clonePtr<ExprMemZero>(expr);
        ExprLooksLikeCall::clone(cexpr);
        return cexpr;
    }

    // ExprMakeGenerator

    ExprMakeGenerator::ExprMakeGenerator ( const LineInfo & a, ExpressionPtr b )
    : ExprLooksLikeCall(a, "generator") {
        __rtti = "ExprMakeGenerator";
        if ( b ) {
            arguments.push_back(b);
        }
    }

    ExpressionPtr ExprMakeGenerator::visit(Visitor & vis) {
        vis.preVisit(this);
        if ( iterType ) {
            vis.preVisit(iterType);
            iterType = iterType->visit(vis);
            iterType = vis.visit(iterType);
        }
        ExprLooksLikeCall::visit(vis);
        return vis.visit(this);
    }

    ExpressionPtr ExprMakeGenerator::clone( ExpressionPtr expr ) const {
        auto cexpr = clonePtr<ExprMakeGenerator>(expr);
        ExprLooksLikeCall::clone(cexpr);
        if ( iterType ) {
            cexpr->iterType = new TypeDecl(*iterType);
        }
        cexpr->capture = capture;
        cexpr->captureAt = captureAt;
        return cexpr;
    }

    // ExprYield

    ExprYield::ExprYield ( const LineInfo & a, ExpressionPtr b ) : Expression(a) {
        __rtti = "ExprYield";
        subexpr = b;
    }

    ExpressionPtr ExprYield::visit(Visitor & vis) {
        vis.preVisit(this);
        subexpr = subexpr->visit(vis);
        return vis.visit(this);
    }

    ExpressionPtr ExprYield::clone( ExpressionPtr expr ) const {
        auto cexpr = clonePtr<ExprYield>(expr);
        Expression::clone(cexpr);
        cexpr->subexpr = subexpr->clone();
        cexpr->returnFlags = returnFlags;
        return cexpr;
    }

    // ExprMakeBlock

    ExpressionPtr ExprMakeBlock::visit(Visitor & vis) {
        vis.preVisit(this);
        if ( vis.canVisitMakeBlockBody(this) ) block = block->visit(vis);
        return vis.visit(this);
    }

    ExpressionPtr ExprMakeBlock::clone( ExpressionPtr expr ) const {
        auto cexpr = clonePtr<ExprMakeBlock>(expr);
        Expression::clone(cexpr);
        cexpr->block = block->clone();
        cexpr->isLambda = isLambda;
        cexpr->isLocalFunction = isLocalFunction;
        cexpr->capture = capture;
        cexpr->captureAt = captureAt;
        cexpr->aotFunctorName = aotFunctorName;
        return cexpr;
    }

    // ExprInvoke

    ExpressionPtr ExprInvoke::clone( ExpressionPtr expr ) const {
        auto cexpr = clonePtr<ExprInvoke>(expr);
        ExprLooksLikeCall::clone(cexpr);
        cexpr->isInvokeMethod = isInvokeMethod;
        return cexpr;
    }

    // ExprErase

    ExpressionPtr ExprErase::clone( ExpressionPtr expr ) const {
        auto cexpr = clonePtr<ExprErase>(expr);
        ExprLooksLikeCall::clone(cexpr);
        return cexpr;
    }

    // ExprSetInsert

    ExpressionPtr ExprSetInsert::clone( ExpressionPtr expr ) const {
        auto cexpr = clonePtr<ExprSetInsert>(expr);
        ExprLooksLikeCall::clone(cexpr);
        return cexpr;
    }

    // ExprFind

    ExpressionPtr ExprFind::clone( ExpressionPtr expr ) const {
        auto cexpr = clonePtr<ExprFind>(expr);
        ExprLooksLikeCall::clone(cexpr);
        return cexpr;
    }

    // ExprKeyExists

    ExpressionPtr ExprKeyExists::clone( ExpressionPtr expr ) const {
        auto cexpr = clonePtr<ExprKeyExists>(expr);
        ExprLooksLikeCall::clone(cexpr);
        return cexpr;
    }

    // ExprIs

    ExpressionPtr ExprIs::visit(Visitor & vis) {
        vis.preVisit(this);
        subexpr = subexpr->visit(vis);
        vis.preVisitType(this, typeexpr);
        vis.preVisit(typeexpr);
        typeexpr = typeexpr->visit(vis);
        typeexpr = vis.visit(typeexpr);
        return vis.visit(this);
    }

    ExpressionPtr ExprIs::clone( ExpressionPtr expr ) const {
        auto cexpr = clonePtr<ExprIs>(expr);
        Expression::clone(cexpr);
        cexpr->subexpr = subexpr->clone();
        cexpr->typeexpr = new TypeDecl(*typeexpr);
        return cexpr;
    }

    // ExprTypeDecl

    ExpressionPtr ExprTypeDecl::visit(Visitor & vis) {
        vis.preVisit(this);
        if ( typeexpr ) {
            vis.preVisit(typeexpr);
            typeexpr = typeexpr->visit(vis);
            typeexpr = vis.visit(typeexpr);
        }
        return vis.visit(this);
    }

    ExpressionPtr ExprTypeDecl::clone( ExpressionPtr expr ) const {
        auto cexpr = clonePtr<ExprTypeDecl>(expr);
        Expression::clone(cexpr);
        cexpr->typeexpr = new TypeDecl(*typeexpr);
        return cexpr;
    }

    // ExprTypeInfo

    ExpressionPtr ExprTypeInfo::visit(Visitor & vis) {
        vis.preVisit(this);
        if ( typeexpr ) {
            vis.preVisit(typeexpr);
            typeexpr = typeexpr->visit(vis);
            typeexpr = vis.visit(typeexpr);
        }
        if ( vis.canVisitExpr(this,subexpr) ) {
            if ( subexpr ) {
                subexpr = subexpr->visit(vis);
            }
        }
        return vis.visit(this);
    }

    ExpressionPtr ExprTypeInfo::clone( ExpressionPtr expr ) const {
        auto cexpr = clonePtr<ExprTypeInfo>(expr);
        Expression::clone(cexpr);
        cexpr->trait = trait;
        cexpr->subtrait = subtrait;
        cexpr->extratrait = extratrait;
        if ( subexpr )
            cexpr->subexpr = subexpr->clone();
        if ( typeexpr )
            cexpr->typeexpr = new TypeDecl(*typeexpr);
        return cexpr;
    }

    // ExprDelete

    ExpressionPtr ExprDelete::visit(Visitor & vis) {
        vis.preVisit(this);
        if ( subexpr ) {
            subexpr = subexpr->visit(vis);
        }
        if ( sizeexpr ) {
            vis.preVisitDeleteSizeExpression(this, sizeexpr);
            sizeexpr = sizeexpr->visit(vis);
        }
        return vis.visit(this);
    }

    ExpressionPtr ExprDelete::clone( ExpressionPtr expr ) const {
        auto cexpr = clonePtr<ExprDelete>(expr);
        Expression::clone(cexpr);
        cexpr->subexpr = subexpr->clone();
        cexpr->sizeexpr = sizeexpr ? sizeexpr->clone() : nullptr;
        cexpr->native = native;
        return cexpr;
    }

    // ExprCast

    ExpressionPtr ExprCast::clone( ExpressionPtr expr  ) const {
        auto cexpr = clonePtr<ExprCast>(expr);
        Expression::clone(cexpr);
        cexpr->subexpr = subexpr->clone();
        cexpr->castType = new TypeDecl(*castType);
        cexpr->castFlags = castFlags;
        return cexpr;
    }

    ExpressionPtr ExprCast::visit(Visitor & vis) {
        vis.preVisit(this);
        if ( castType ) {
            vis.preVisit(castType);
            castType = castType->visit(vis);
            castType = vis.visit(castType);
        }
        subexpr = subexpr->visit(vis);
        return vis.visit(this);
    }

    // ExprAscend

    ExpressionPtr ExprAscend::clone( ExpressionPtr expr  ) const {
        auto cexpr = clonePtr<ExprAscend>(expr);
        Expression::clone(cexpr);
        cexpr->subexpr = subexpr->clone();
        if ( ascType ) cexpr->ascType = new TypeDecl(*ascType);
        cexpr->ascendFlags = ascendFlags;
        return cexpr;
    }

    ExpressionPtr ExprAscend::visit(Visitor & vis) {
        vis.preVisit(this);
        subexpr = subexpr->visit(vis);
        return vis.visit(this);
    }

    // ExprNew

    ExpressionPtr ExprNew::visit(Visitor & vis) {
        vis.preVisit(this);
        if ( typeexpr ) {
            vis.preVisit(typeexpr);
            typeexpr = typeexpr->visit(vis);
            typeexpr = vis.visit(typeexpr);
        }
        for ( auto & arg : arguments ) {
            vis.preVisitNewArg(this, arg, arg==arguments.back());
            arg = arg->visit(vis);
            arg = vis.visitNewArg(this, arg, arg==arguments.back());
        }
        return vis.visit(this);
    }

    ExpressionPtr ExprNew::clone( ExpressionPtr expr ) const {
        auto cexpr = clonePtr<ExprNew>(expr);
        ExprLooksLikeCall::clone(cexpr);
        cexpr->typeexpr = new TypeDecl(*typeexpr);
        cexpr->initializer = initializer;
        cexpr->allocate_on_stack = allocate_on_stack;
        return cexpr;
    }

    // ExprAt

    ExpressionPtr ExprAt::visit(Visitor & vis) {
        vis.preVisit(this);
        subexpr = subexpr->visit(vis);
        vis.preVisitAtIndex(this, index);
        index = index->visit(vis);
        return vis.visit(this);
    }

    ExpressionPtr ExprAt::clone( ExpressionPtr expr ) const {
        auto cexpr = clonePtr<ExprAt>(expr);
        Expression::clone(cexpr);
        cexpr->subexpr = subexpr->clone();
        cexpr->index = index->clone();
        cexpr->no_promotion = no_promotion;
        return cexpr;
    }

    void ExprAt::markNoDiscard() {
        subexpr->markNoDiscard();
    }

    // ExprSafeAt

    ExpressionPtr ExprSafeAt::visit(Visitor & vis) {
        vis.preVisit(this);
        subexpr = subexpr->visit(vis);
        vis.preVisitSafeAtIndex(this, index);
        index = index->visit(vis);
        return vis.visit(this);
    }

    ExpressionPtr ExprSafeAt::clone( ExpressionPtr expr ) const {
        auto cexpr = clonePtr<ExprSafeAt>(expr);
        Expression::clone(cexpr);
        cexpr->subexpr = subexpr->clone();
        cexpr->index = index->clone();
        cexpr->no_promotion = no_promotion;
        return cexpr;
    }

    // ExprBlock

    bool ExprBlock::collapse() {
        bool any = false;
        if ( !list.empty() ) {
            vector<ExpressionPtr> lst;
            collapse(lst, list);
            if ( lst != list ) {
                swap ( list, lst );
                any = true;
            }
        }
        if ( !finalList.empty() ) {
            vector<ExpressionPtr> flst;
            collapse(flst, finalList);
            if ( flst != finalList ) {
                swap ( finalList, flst );
                any = true;
            }
        }
        return any;
    }

    void ExprBlock::collapse ( vector<ExpressionPtr> & res, const vector<ExpressionPtr> & lst ) {
        for ( const auto & ex :lst ) {
            if ( ex->rtti_isBlock() ) {
                auto blk = static_cast<ExprBlock*>(ex);
                // note: no need to check for `isCollapseable'. typically needCollapse is sufficient
                if ( /* blk->isCollapseable && */ blk->finalList.empty() ) {
                    collapse(res, blk->list);
                } else {
                    res.push_back(ex);
                }
            } else {
                res.push_back(ex);
            }
        }
    }

    string ExprBlock::getMangledName(bool includeName, bool includeResult) const {
        TextWriter ss;
        if ( includeResult ) {
            ss << returnType->getMangledName();
        }
        for ( auto & arg : arguments ) {
            if ( includeName ) {
                ss << " " << arg->name << ":" << arg->type->getMangledName();
            } else {
                ss << " " << arg->type->getMangledName();
            }
        }
        return ss.str();
    }

    TypeDeclPtr ExprBlock::makeBlockType () const {
        auto eT = new TypeDecl(Type::tBlock);
        eT->constant = true;
        if ( type ) {
            eT->firstType = new TypeDecl(*type);
        }
        for ( auto & arg : arguments ) {
            if ( arg->type ) {
                eT->argTypes.push_back(new TypeDecl(*arg->type));
                eT->argNames.push_back(arg->name);
            }
        }
        return eT;
    }

    void ExprBlock::visitFinally(Visitor & vis) {
        if ( !finalList.empty() && !finallyDisabled ) {
            vis.preVisitBlockFinal(this);
            for ( auto it = finalList.begin(); it!=finalList.end(); ) {
                auto & subexpr = *it;
                vis.preVisitBlockFinalExpression(this, subexpr);
                if ( subexpr ) subexpr = subexpr->visit(vis);
                if ( subexpr ) subexpr = vis.visitBlockFinalExpression(this, subexpr);
                if ( subexpr ) ++it; else it = finalList.erase(it);
            }
            vis.visitBlockFinal(this);
        }
    }

    ExpressionPtr ExprBlock::visit(Visitor & vis) {
        vis.preVisit(this);
        if ( isClosure ) {
          for ( auto it = arguments.begin(); it != arguments.end(); ) {
              auto & arg = *it;
              vis.preVisitBlockArgument(this, arg, arg==arguments.back());
              if ( arg->type ) {
                  vis.preVisit(arg->type);
                  arg->type = arg->type->visit(vis);
                  arg->type = vis.visit(arg->type);
              }
              if ( arg->init ) {
                  vis.preVisitBlockArgumentInit(this, arg, arg->init);
                  arg->init = arg->init->visit(vis);
                  if ( arg->init ) {
                      arg->init = vis.visitBlockArgumentInit(this, arg, arg->init);
                  }
              }
              arg = vis.visitBlockArgument(this, arg, arg==arguments.back());
              if ( arg ) ++it; else it = arguments.erase(it);
          }
          if ( returnType ) {
              vis.preVisit(returnType);
              returnType = returnType->visit(vis);
              returnType = vis.visit(returnType);
          }
        }
        if ( finallyBeforeBody ) {
            visitFinally(vis);
        }
        for ( auto it = list.begin(); it!=list.end(); ) {
            auto & subexpr = *it;
            vis.preVisitBlockExpression(this, subexpr);
            if ( subexpr ) subexpr = subexpr->visit(vis);
            if ( subexpr ) subexpr = vis.visitBlockExpression(this, subexpr);
            if ( subexpr ) ++it; else it = list.erase(it);
        }
        if ( !finallyBeforeBody ) {
            visitFinally(vis);
        }
        return vis.visit(this);
    }

    ExpressionPtr ExprBlock::clone( ExpressionPtr expr ) const {
        auto cexpr = clonePtr<ExprBlock>(expr);
        Expression::clone(cexpr);
        cexpr->list.reserve(list.size());
        for ( auto & subexpr : list ) {
            if ( subexpr ) cexpr->list.push_back(subexpr->clone());
        }
        cexpr->finalList.reserve(finalList.size());
        for ( auto & subexpr : finalList ) {
            if ( subexpr ) cexpr->finalList.push_back(subexpr->clone());
        }
        cexpr->blockFlags = blockFlags;
        if ( returnType )
            cexpr->returnType = new TypeDecl(*returnType);
        for ( auto & arg : arguments ) {
            cexpr->arguments.push_back(arg->clone());
        }
        cexpr->annotations = cloneAnnotationList(annotations);
        cexpr->maxLabelIndex = maxLabelIndex;
        cexpr->inFunction = inFunction;
        return cexpr;
    }

    uint32_t ExprBlock::getEvalFlags() const {
        uint32_t flg = getFinallyEvalFlags();
        for ( const auto & ex : list ) {
            flg |= ex->getEvalFlags();
        }
        return flg;
    }

    uint32_t ExprBlock::getFinallyEvalFlags() const {
        uint32_t flg = 0;
        for ( const auto & ex : finalList ) {
            flg |= ex->getEvalFlags();
        }
        return flg;
    }

    VariablePtr ExprBlock::findArgument(const string & name) {
        for ( auto & arg : arguments ) {
            if ( arg->name==name ) {
                return arg;
            }
        }
        return nullptr;
    }

    // ExprSwizzle

    ExpressionPtr ExprSwizzle::visit(Visitor & vis) {
        vis.preVisit(this);
        value = value->visit(vis);
        return vis.visit(this);
    }

    ExpressionPtr ExprSwizzle::clone( ExpressionPtr expr ) const {
        auto cexpr = clonePtr<ExprSwizzle>(expr);
        Expression::clone(cexpr);
        cexpr->mask = mask;
        cexpr->value = value->clone();
        return cexpr;
    }

    // ExprField

    ExpressionPtr ExprField::visit(Visitor & vis) {
        vis.preVisit(this);
        value = value->visit(vis);
        return vis.visit(this);
    }

    Structure::FieldDeclaration * ExprField::field() const {
        return fieldRef.get();
    }

    ExpressionPtr ExprField::clone( ExpressionPtr expr ) const {
        auto cexpr = clonePtr<ExprField>(expr);
        Expression::clone(cexpr);
        cexpr->name = name;
        if ( value) {
            cexpr->value = value->clone();
        }
        cexpr->fieldRef = fieldRef;
        cexpr->fieldIndex = fieldIndex;
        cexpr->unsafeDeref = unsafeDeref;
        cexpr->ignoreCaptureConst = ignoreCaptureConst;
        cexpr->no_promotion = no_promotion;
        cexpr->atField = atField;
        return cexpr;
    }

    void ExprField::markNoDiscard() {
        value->markNoDiscard();
    }

    // ExprIs

    ExpressionPtr ExprIsVariant::visit(Visitor & vis) {
        vis.preVisit(this);
        value = value->visit(vis);
        return vis.visit(this);
    }

    ExpressionPtr ExprIsVariant::clone( ExpressionPtr expr ) const {
        auto cexpr = clonePtr<ExprIsVariant>(expr);
        ExprField::clone(cexpr);
        return cexpr;
    }

    // ExprAsVariant

    ExpressionPtr ExprAsVariant::visit(Visitor & vis) {
        vis.preVisit(this);
        value = value->visit(vis);
        return vis.visit(this);
    }

    ExpressionPtr ExprAsVariant::clone( ExpressionPtr expr ) const {
        auto cexpr = clonePtr<ExprAsVariant>(expr);
        ExprField::clone(cexpr);
        return cexpr;
    }

    // ExprSafeAsVariant

    ExpressionPtr ExprSafeAsVariant::visit(Visitor & vis) {
        vis.preVisit(this);
        value = value->visit(vis);
        return vis.visit(this);
    }

    ExpressionPtr ExprSafeAsVariant::clone( ExpressionPtr expr ) const {
        auto cexpr = clonePtr<ExprSafeAsVariant>(expr);
        ExprField::clone(cexpr);
        return cexpr;
    }

    // ExprSafeField

    ExpressionPtr ExprSafeField::visit(Visitor & vis) {
        vis.preVisit(this);
        value = value->visit(vis);
        return vis.visit(this);
    }

    ExpressionPtr ExprSafeField::clone( ExpressionPtr expr ) const {
        auto cexpr = clonePtr<ExprSafeField>(expr);
        ExprField::clone(cexpr);
        return cexpr;
    }

    // ExprStringBuilder

    ExpressionPtr ExprStringBuilder::visit(Visitor & vis) {
        vis.preVisit(this);
        for ( auto it = elements.begin(); it!=elements.end(); ) {
            auto & elem = *it;
            vis.preVisitStringBuilderElement(this, elem, elem==elements.back());
            elem = elem->visit(vis);
            if ( elem ) elem = vis.visitStringBuilderElement(this, elem, elem==elements.back());
            if ( elem ) ++ it; else it = elements.erase(it);
        }
        return vis.visit(this);
    }

    ExpressionPtr ExprStringBuilder::clone( ExpressionPtr expr  ) const {
        auto cexpr = clonePtr<ExprStringBuilder>(expr);
        Expression::clone(cexpr);
        cexpr->elements.reserve(elements.size());
        for ( auto & elem : elements ) {
            cexpr->elements.push_back(elem->clone());
        }
        return cexpr;
    }

    // ExprVar

    ExpressionPtr ExprVar::visit(Visitor & vis) {
        vis.preVisit(this);
        return vis.visit(this);
    }

    ExpressionPtr ExprVar::clone( ExpressionPtr expr ) const {
        auto cexpr = clonePtr<ExprVar>(expr);
        Expression::clone(cexpr);
        cexpr->name = name;
        cexpr->variable = variable;
        cexpr->local = local;
        cexpr->block = block;
        cexpr->pBlock = pBlock;
        cexpr->argument = argument;
        cexpr->argumentIndex = argumentIndex;
        return cexpr;
    }

    // ExprTag

    ExpressionPtr ExprTag::visit(Visitor & vis) {
        vis.preVisit(this);
        if ( subexpr ) {
            subexpr = subexpr->visit(vis);
        }
        if ( value ) {
            vis.preVisitTagValue(this, value);
            value = value->visit(vis);
        }
        return vis.visit(this);
    }

    ExpressionPtr ExprTag::clone ( ExpressionPtr expr ) const {
        auto cexpr = clonePtr<ExprTag>(expr);
        Expression::clone(cexpr);
        if ( subexpr )  cexpr->subexpr = subexpr->clone();
        if ( value ) cexpr->value = value->clone();
        cexpr->name = name;
        return cexpr;
    }

    // ExprOp

    ExpressionPtr ExprOp::clone( ExpressionPtr expr ) const {
        if ( !expr ) {
            DAS_ASSERTF(0,"can't clone ExprOp");
            return nullptr;
        }
        auto cexpr = static_cast<ExprOp*>(expr);
        ExprCallFunc::clone(cexpr);
        cexpr->op = op;
        cexpr->func = func;
        cexpr->at = at;
        return cexpr;
    }

    // ExprOp1

    bool ExprOp1::swap_tail ( Expression * expr, Expression * swapExpr ) {
        if ( subexpr==expr ) {
            subexpr = swapExpr;
            return true;
        } else {
            return subexpr->swap_tail(expr,swapExpr);
        }
    }

    ExpressionPtr ExprOp1::visit(Visitor & vis) {
        vis.preVisit(this);
        subexpr = subexpr->visit(vis);
        return vis.visit(this);
    }

    ExpressionPtr ExprOp1::clone( ExpressionPtr expr ) const {
        auto cexpr = clonePtr<ExprOp1>(expr);
        ExprOp::clone(cexpr);
        cexpr->subexpr = subexpr->clone();
        return cexpr;
    }

    string ExprOp1::describe() const {
        TextWriter stream;
        stream << name << " ( ";
        if ( subexpr && subexpr->type ) {
            stream << *subexpr->type;
        } else {
            stream << "???";
        }
        stream << " )";
        return stream.str();
    }

    void ExprOp1::markNoDiscard() {
        subexpr->markNoDiscard();
    }

    // ExprOp2

    bool ExprOp2::swap_tail ( Expression * expr, Expression * swapExpr ) {
        if ( right==expr ) {
            right = swapExpr;
            return true;
        } else {
            return right->swap_tail(expr,swapExpr);
        }
    }

    ExpressionPtr ExprOp2::visit(Visitor & vis) {
        vis.preVisit(this);
        left = left->visit(vis);
        vis.preVisitRight(this, right);
        right = right->visit(vis);
        return vis.visit(this);
    }

    ExpressionPtr ExprOp2::clone( ExpressionPtr expr ) const {
        auto cexpr = clonePtr<ExprOp2>(expr);
        ExprOp::clone(cexpr);
        cexpr->left = left->clone();
        cexpr->right = right->clone();
        return cexpr;
    }

    string ExprOp2::describe() const {
        TextWriter stream;
        stream << name << " ( ";
        if ( left && left->type ) {
            stream << *left->type;
        } else {
            stream << "???";
        }
        stream << ", ";
        if ( right && right->type ) {
            stream << *right->type;
        } else {
            stream << "???";
        }
        stream << " )";
        return stream.str();
    }

    void ExprOp2::markNoDiscard() {
        left->markNoDiscard();
        right->markNoDiscard();
    }

    // ExprOp3

    bool ExprOp3::swap_tail ( Expression * expr, Expression * swapExpr ) {
        if ( right==expr ) {
            right = swapExpr;
            return true;
        } else {
            return right->swap_tail(expr,swapExpr);
        }
    }

    ExpressionPtr ExprOp3::visit(Visitor & vis) {
        vis.preVisit(this);
        subexpr = subexpr->visit(vis);
        vis.preVisitLeft(this, left);
        left = left->visit(vis);
        vis.preVisitRight(this, right);
        right = right->visit(vis);
        return vis.visit(this);
    }

    ExpressionPtr ExprOp3::clone( ExpressionPtr expr ) const {
        auto cexpr = clonePtr<ExprOp3>(expr);
        ExprOp::clone(cexpr);
        cexpr->subexpr = subexpr->clone();
        cexpr->left = left->clone();
        cexpr->right = right->clone();
        return cexpr;
    }

    string ExprOp3::describe() const {
        TextWriter stream;
        stream << name << " ( ";
        if ( subexpr && subexpr->type ) {
            stream << *subexpr->type;
        } else {
            stream << "???";
        }
        stream << ", ";
        if ( left && left->type ) {
            stream << *left->type;
        } else {
            stream << "???";
        }
        stream << ", ";
        if ( right && right->type ) {
            stream << *right->type;
        } else {
            stream << "???";
        }
        stream << " )";
        return stream.str();
    }

    void ExprOp3::markNoDiscard() {
        subexpr->markNoDiscard();
        left->markNoDiscard();
        right->markNoDiscard();
    }

    // ExprMove

    ExpressionPtr ExprMove::visit(Visitor & vis) {
        vis.preVisit(this);
        if ( vis.isRightFirst(this) ) {
            vis.preVisitRight(this, right);
            right = right->visit(vis);
            left = left->visit(vis);
        } else {
            left = left->visit(vis);
            vis.preVisitRight(this, right);
            right = right->visit(vis);
        }
        return vis.visit(this);
    }

    ExpressionPtr ExprMove::clone( ExpressionPtr expr ) const {
        auto cexpr = clonePtr<ExprMove>(expr);
        ExprOp2::clone(cexpr);
        cexpr->moveFlags = moveFlags;
        return cexpr;
    }

    // ExprClone

    ExpressionPtr ExprClone::visit(Visitor & vis) {
        vis.preVisit(this);
        if ( vis.isRightFirst(this) ) {
            vis.preVisitRight(this, right);
            right = right->visit(vis);
            left = left->visit(vis);
        } else {
            left = left->visit(vis);
            vis.preVisitRight(this, right);
            right = right->visit(vis);
        }
        return vis.visit(this);
    }

    ExpressionPtr ExprClone::clone( ExpressionPtr expr ) const {
        auto cexpr = clonePtr<ExprClone>(expr);
        ExprOp2::clone(cexpr);
        return cexpr;
    }

    // ExprCopy

    ExpressionPtr ExprCopy::visit(Visitor & vis) {
        vis.preVisit(this);
        if ( vis.isRightFirst(this) ) {
            vis.preVisitRight(this, right);
            right = right->visit(vis);
            left = left->visit(vis);
        } else {
            left = left->visit(vis);
            vis.preVisitRight(this, right);
            right = right->visit(vis);
        }
        return vis.visit(this);
    }

    ExpressionPtr ExprCopy::clone( ExpressionPtr expr ) const {
        auto cexpr = clonePtr<ExprCopy>(expr);
        ExprOp2::clone(cexpr);
        cexpr->copyFlags = copyFlags;
        return cexpr;
    }

    // ExprTryCatch

    ExpressionPtr ExprTryCatch::visit(Visitor & vis) {
        vis.preVisit(this);
        try_block = try_block->visit(vis);
        vis.preVisitCatch(this,catch_block);
        catch_block = catch_block->visit(vis);
        return vis.visit(this);
    }

    uint32_t ExprTryCatch::getEvalFlags() const {
        return try_block->getEvalFlags() | catch_block->getEvalFlags();
    }

    ExpressionPtr ExprTryCatch::clone( ExpressionPtr expr ) const {
        auto cexpr = clonePtr<ExprTryCatch>(expr);
        Expression::clone(cexpr);
        cexpr->try_block = try_block->clone();
        cexpr->catch_block = catch_block->clone();
        return cexpr;
    }

    // ExprReturn

    ExpressionPtr ExprReturn::visit(Visitor & vis) {
        vis.preVisit(this);
        if ( subexpr ) {
            subexpr = subexpr->visit(vis);
        }
        return vis.visit(this);
    }

    ExpressionPtr ExprReturn::clone( ExpressionPtr expr ) const {
        auto cexpr = clonePtr<ExprReturn>(expr);
        Expression::clone(cexpr);
        if ( subexpr )
            cexpr->subexpr = subexpr->clone();
        cexpr->moveSemantics = moveSemantics;
        cexpr->fromYield = fromYield;
        cexpr->fromComprehension = fromComprehension;
        return cexpr;
    }

    // ExprBreak

    ExpressionPtr ExprBreak::visit(Visitor & vis) {
        vis.preVisit(this);
        return vis.visit(this);
    }

    ExpressionPtr ExprBreak::clone( ExpressionPtr expr ) const {
        auto cexpr = clonePtr<ExprBreak>(expr);
        Expression::clone(cexpr);
        return cexpr;
    }

    // ExprContinue

    ExpressionPtr ExprContinue::visit(Visitor & vis) {
        vis.preVisit(this);
        return vis.visit(this);
    }

    ExpressionPtr ExprContinue::clone( ExpressionPtr expr ) const {
        auto cexpr = clonePtr<ExprContinue>(expr);
        Expression::clone(cexpr);
        return cexpr;
    }

    // ExprIfThenElse

    ExpressionPtr ExprIfThenElse::visit(Visitor & vis) {
        vis.preVisit(this);
        cond = cond->visit(vis);
        if ( vis.canVisitIfSubexpr(this) ) {
            vis.preVisitIfBlock(this, if_true);
            if_true = if_true->visit(vis);
            if ( if_false ) {
                vis.preVisitElseBlock(this, if_false);
                if_false = if_false->visit(vis);
            }
        }
        return vis.visit(this);
    }

    ExpressionPtr ExprIfThenElse::clone( ExpressionPtr expr ) const {
        auto cexpr = clonePtr<ExprIfThenElse>(expr);
        Expression::clone(cexpr);
        cexpr->cond = cond->clone();
        cexpr->if_true = if_true->clone();
        if ( if_false )
            cexpr->if_false = if_false->clone();
        cexpr->ifFlags = ifFlags;
        return cexpr;
    }

    uint32_t ExprIfThenElse::getEvalFlags() const {
        auto flagsE = cond->getEvalFlags() | if_true->getEvalFlags();
        if (if_false) flagsE |= if_false->getEvalFlags();
        return flagsE;
    }

    // ExprWith

    ExpressionPtr ExprWith::visit(Visitor & vis) {
        vis.preVisit(this);
        with = with->visit(vis);
        vis.preVisitWithBody(this, body);
        body = body->visit(vis);
        return vis.visit(this);
    }

    ExpressionPtr ExprWith::clone( ExpressionPtr expr ) const {
        auto cexpr = clonePtr<ExprWith>(expr);
        Expression::clone(cexpr);
        cexpr->with = with->clone();
        cexpr->body = body->clone();
        return cexpr;
    }

    // ExprAssume

    ExpressionPtr ExprAssume::visit(Visitor & vis) {
        vis.preVisit(this);
        if ( vis.canVisitWithAliasSubexpression(this) ) {
            if ( subexpr ) subexpr = subexpr->visit(vis);
            if ( assumeType ) assumeType = assumeType->visit(vis);

        }
        return vis.visit(this);
    }

    ExpressionPtr ExprAssume::clone( ExpressionPtr expr ) const {
        auto cexpr = clonePtr<ExprAssume>(expr);
        Expression::clone(cexpr);
        cexpr->alias = alias;
        if ( subexpr ) cexpr->subexpr = subexpr->clone();
        if ( assumeType ) cexpr->assumeType = new TypeDecl(*assumeType);
        return cexpr;
    }

    // ExprUnsafe

    ExpressionPtr ExprUnsafe::visit(Visitor & vis) {
        vis.preVisit(this);
        body = body->visit(vis);
        return vis.visit(this);
    }

    ExpressionPtr ExprUnsafe::clone( ExpressionPtr expr ) const {
        auto cexpr = clonePtr<ExprUnsafe>(expr);
        Expression::clone(cexpr);
        cexpr->body = body->clone();
        return cexpr;
    }

    uint32_t ExprUnsafe::getEvalFlags() const {
        return body->getEvalFlags();
    }

    // ExprWhile

    ExpressionPtr ExprWhile::visit(Visitor & vis) {
        vis.preVisit(this);
        cond = cond->visit(vis);
        vis.preVisitWhileBody(this, body);
        body = body->visit(vis);
        return vis.visit(this);
    }

    ExpressionPtr ExprWhile::clone( ExpressionPtr expr ) const {
        auto cexpr = clonePtr<ExprWhile>(expr);
        Expression::clone(cexpr);
        cexpr->cond = cond->clone();
        cexpr->body = body->clone();
        cexpr->annotations = annotations;
        return cexpr;
    }

    uint32_t ExprWhile::getEvalFlags() const {
        return body->getEvalFlags() & ~(EvalFlags::stopForBreak | EvalFlags::stopForContinue);
    }

    // ExprFor

    ExpressionPtr ExprFor::visit(Visitor & vis) {
        vis.preVisit(this);
        for ( auto & var : iteratorVariables ) {
            vis.preVisitFor(this, var, var==iteratorVariables.back());
            var = vis.visitFor(this, var, var==iteratorVariables.back());
        }
        for ( auto & src : sources ) {
            vis.preVisitForSource(this, src, src==sources.back());
            src = src->visit(vis);
            src = vis.visitForSource(this, src, src==sources.back());
        }
        vis.preVisitForStack(this);
        if ( body ) {
            vis.preVisitForBody(this, body);
            body = body->visit(vis);
        }
        return vis.visit(this);
    }

    ExpressionPtr ExprFor::clone( ExpressionPtr expr ) const {
        auto cexpr = clonePtr<ExprFor>(expr);
        Expression::clone(cexpr);
        cexpr->iterators = iterators;
        cexpr->iteratorsAka = iteratorsAka;
        cexpr->iteratorsAt = iteratorsAt;
        cexpr->iteratorsTupleExpansion = iteratorsTupleExpansion;
        for ( auto tag : iteratorsTags )
            cexpr->iteratorsTags.push_back(tag ? tag->clone() : nullptr);
        cexpr->visibility = visibility;
        for ( auto & src : sources )
            cexpr->sources.push_back(src->clone());
        for ( auto & var : iteratorVariables )
            cexpr->iteratorVariables.push_back(var->clone());
        if ( body ) {
            cexpr->body = body->clone();
        }
        cexpr->allowIteratorOptimization = allowIteratorOptimization;
        cexpr->canShadow = canShadow;
        cexpr->annotations = annotations;
        return cexpr;
    }

    Variable * ExprFor::findIterator(const string & name) const {
        for ( auto & v : iteratorVariables ) {
            if ( v->name==name ) {
                return v;
            }
        }
        return nullptr;
    }

    uint32_t ExprFor::getEvalFlags() const {
        return body->getEvalFlags() & ~(EvalFlags::stopForBreak | EvalFlags::stopForContinue);
    }

    // ExprLet

    ExpressionPtr ExprLet::visit(Visitor & vis) {
        vis.preVisit(this);
        for ( auto it = variables.begin(); it!=variables.end(); ) {
            auto & var = *it;
            vis.preVisitLet(this, var, var==variables.back());
            if ( var->type ) {
                vis.preVisit(var->type);
                var->type = var->type->visit(vis);
                var->type = vis.visit(var->type);
            }
            if ( var->init ) {
                vis.preVisitLetInit(this, var, var->init);
                var->init = var->init->visit(vis);
                var->init = vis.visitLetInit(this, var, var->init);
            }
            var = vis.visitLet(this, var, var==variables.back());
            if ( var ) ++it; else it = variables.erase(it);
        }
        return vis.visit(this);
    }

    ExpressionPtr ExprLet::clone( ExpressionPtr expr ) const {
        auto cexpr = clonePtr<ExprLet>(expr);
        Expression::clone(cexpr);
        for ( auto & var : variables )
            cexpr->variables.push_back(var->clone());
        cexpr->visibility = visibility;
        cexpr->atInit = atInit;
        cexpr->letFlags = letFlags;
        return cexpr;
    }

    Variable * ExprLet::find(const string & name) const {
        for ( auto & v : variables ) {
            if ( v->name==name ) {
                return v;
            }
        }
        return nullptr;
    }

    // ExprCallMacro

    ExpressionPtr ExprCallMacro::visit(Visitor & vis) {
        vis.preVisit(this);
        int index = 0;
        for ( auto & arg : arguments ) {
            if ( !macro || macro->canVisitArguments(this,index) ) {
                vis.preVisitLooksLikeCallArg(this, arg, arg==arguments.back());
                arg = arg->visit(vis);
                arg = vis.visitLooksLikeCallArg(this, arg, arg==arguments.back());
            }
            index ++;
        }
        return vis.visit(this);
    }

    ExpressionPtr ExprCallMacro::clone( ExpressionPtr expr ) const {
        auto cexpr = clonePtr<ExprCallMacro>(expr);
        ExprLooksLikeCall::clone(cexpr);
        cexpr->macro = macro;
        cexpr->inFunction = inFunction;
        return cexpr;
    }

    // ExprLooksLikeCall

    ExpressionPtr ExprLooksLikeCall::visit(Visitor & vis) {
        if ( vis.canVisitLooksLikeCall(this) ) {
            vis.preVisit(this);
            for ( auto & arg : arguments ) {
                if ( vis.canVisitLooksLikeCallArg(this, arg, arg==arguments.back()) ) {
                    vis.preVisitLooksLikeCallArg(this, arg, arg==arguments.back());
                    arg = arg->visit(vis);
                    arg = vis.visitLooksLikeCallArg(this, arg, arg==arguments.back());
                }
            }
            return vis.visit(this);
        } else {
            return this;
        }
    }

    ExpressionPtr ExprLooksLikeCall::clone( ExpressionPtr expr ) const {
        auto cexpr = clonePtr<ExprLooksLikeCall>(expr);
        Expression::clone(cexpr);
        cexpr->atEnclosure = atEnclosure;
        cexpr->name = name;
        cexpr->pipedCallArgument = pipedCallArgument;
        for ( auto & arg : arguments ) {
            cexpr->arguments.push_back(arg->clone());
        }
        return cexpr;
    }

    string ExprLooksLikeCall::describe() const {
        TextWriter stream;
        stream << name << "(";
        for ( auto & arg : arguments ) {
            if ( arg->type )
                stream << *arg->type;
            else
                stream << "???";
            if (arg != arguments.back()) {
                stream << ", ";
            }
        }
        stream << ")";
        return stream.str();
    }

    void ExprLooksLikeCall::autoDereference() {
        for ( size_t iA = 0; iA != arguments.size(); ++iA ) {
            arguments[iA] = Expression::autoDereference(arguments[iA]);
        }
    }

    // ExprCall

    ExpressionPtr ExprCall::visit(Visitor & vis) {
        if ( vis.canVisitCall(this) ) {
            vis.preVisit(this);
            for ( auto & arg : arguments ) {
                vis.preVisitCallArg(this, arg, arg==arguments.back());
                arg = arg->visit(vis);
                arg = vis.visitCallArg(this, arg, arg==arguments.back());
            }
            return vis.visit(this);
        } else {
            return this;
        }
    }

    void ExprCall::markNoDiscard() {
        notDiscarded = true;
    }

    ExpressionPtr ExprCall::clone( ExpressionPtr expr ) const {
        auto cexpr = clonePtr<ExprCall>(expr);
        ExprLooksLikeCall::clone(cexpr);
        cexpr->func = func;
        return cexpr;
    }

    // named call

    ExpressionPtr ExprNamedCall::visit(Visitor & vis) {
        if ( vis.canVisitNamedCall(this) ) {
            vis.preVisit(this);

            if (nonNamedArguments.size() > 0) {
                ExprCall dummy;
                dummy.gc_unlink();
                for (auto& arg : nonNamedArguments) {
                    vis.preVisitCallArg(&dummy, arg, arg == nonNamedArguments.back());
                    arg = arg->visit(vis);
                    arg = vis.visitCallArg(&dummy, arg, arg == nonNamedArguments.back());
                }
                this->argumentsFailedToInfer = dummy.argumentsFailedToInfer;
            }
            if ( arguments ) for (auto& arg : *arguments) {
                vis.preVisitNamedCallArg(this, arg, arg == arguments->back());
                arg->value = arg->value->visit(vis);
                arg = vis.visitNamedCallArg(this, arg, arg==arguments->back());
            }
            return vis.visit(this);
        } else {
            return this;
        }
    }

    ExpressionPtr ExprNamedCall::clone( ExpressionPtr expr ) const {
        auto cexpr = clonePtr<ExprNamedCall>(expr);
        Expression::clone(cexpr);
        cexpr->name = name;
        if ( arguments ) {
            cexpr->arguments = new MakeStruct();
            cexpr->arguments->reserve(arguments->size());
            for ( auto & arg : *arguments ) {
                cexpr->arguments->push_back(arg->clone());
            }
        }
        cexpr->methodCall = methodCall;
        return cexpr;
    }

    // make structure

    MakeFieldDeclPtr MakeFieldDecl::clone() const {
        auto md = new MakeFieldDecl();
        md->at = at;
        md->flags = flags;
        md->name = name;
        md->value = value ? value->clone() : nullptr;
        md->tag = tag ? tag->clone() : nullptr;
        return md;
    }

    ExpressionPtr ExprMakeStruct::clone( ExpressionPtr expr ) const {
        auto cexpr = clonePtr<ExprMakeStruct>(expr);
        ExprMakeLocal::clone(cexpr);
        cexpr->structs.reserve ( structs.size() );
        for ( auto & fields : structs ) {
            auto mfd = new MakeStruct();
            mfd->reserve( fields->size() );
            for ( auto & fd : *fields ) {
                mfd->push_back(fd->clone());
            }
            cexpr->structs.push_back(mfd);
        }
        if ( makeType ) {
            cexpr->makeType = new TypeDecl(*makeType);
        }
        cexpr->makeStructFlags = makeStructFlags;
        if ( block ) {
            cexpr->block = block->clone();
        }
        return cexpr;
    }

    ExpressionPtr ExprMakeStruct::visit(Visitor & vis) {
        if ( vis.canVisitMakeStructure(this) ) {
            vis.preVisit(this);
            if ( makeType ) {
                vis.preVisit(makeType);
                makeType = makeType->visit(vis);
                makeType = vis.visit(makeType);
            }
            if ( vis.canVisitMakeStructureBody(this) ) {
                for ( int index=0; index != int(structs.size()); ++index ) {
                    vis.preVisitMakeStructureIndex(this, index, index==int(structs.size()-1));
                    auto & fields = structs[index];
                    for ( auto it = fields->begin(); it != fields->end(); ) {
                        auto & field = *it;
                        vis.preVisitMakeStructureField(this, index, field, field==fields->back());
                        field->value = field->value->visit(vis);
                        if ( field ) {
                            field = vis.visitMakeStructureField(this, index, field, field==fields->back());
                        }
                        if ( field ) ++it; else it = fields->erase(it);
                    }
                    vis.visitMakeStructureIndex(this, index, index==int(structs.size()-1));
                }
            }
            if ( block && vis.canVisitMakeStructureBlock(this, block) ) {
                vis.preVisitMakeStructureBlock(this, block);
                block = block->visit(vis);
                if ( block ) block = vis.visitMakeStructureBlock(this, block);
            }
            return vis.visit(this);
        } else {
            return this;
        }
    }

    void ExprMakeStruct::markNoDiscard() {
        for ( auto & fields : structs ) {
            for ( auto & field : *fields ) {
                field->value->markNoDiscard();
            }
        }
    }

    // make variant

    ExpressionPtr ExprMakeVariant::clone( ExpressionPtr expr ) const {
        auto cexpr = clonePtr<ExprMakeVariant>(expr);
        ExprMakeLocal::clone(cexpr);
        cexpr->variants.reserve ( variants.size() );
        for ( auto & fd : variants ) {
            cexpr->variants.push_back(fd->clone());
        }
        cexpr->makeType = new TypeDecl(*makeType);
        return cexpr;
    }

    ExpressionPtr ExprMakeVariant::visit(Visitor & vis) {
        vis.preVisit(this);
        if ( makeType ) {
            vis.preVisit(makeType);
            makeType = makeType->visit(vis);
            makeType = vis.visit(makeType);
        }
        int index = 0;
        for ( auto it = variants.begin(); it != variants.end(); index ++ ) {
            auto & field = *it;
            vis.preVisitMakeVariantField(this, index, field, field==variants.back());
            field->value = field->value->visit(vis);
            if ( field ) {
                field = vis.visitMakeVariantField(this, index, field, field==variants.back());
            }
            if ( field ) ++it; else it = variants.erase(it);
        }
        return vis.visit(this);
    }

    void ExprMakeVariant::markNoDiscard() {
        for ( auto & field : variants ) {
            field->value->markNoDiscard();
        }
    }

    // make array

    ExpressionPtr ExprMakeArray::clone( ExpressionPtr expr ) const {
        auto cexpr = clonePtr<ExprMakeArray>(expr);
        ExprMakeLocal::clone(cexpr);
        cexpr->values.reserve ( values.size() );
        for ( auto & val : values ) {
            cexpr->values.push_back(val->clone());
        }
        cexpr->makeType = new TypeDecl(*makeType);
        if (recordType) {
            cexpr->recordType = new TypeDecl(*recordType);
        }
        cexpr->gen2 = gen2;
        return cexpr;
    }

    ExpressionPtr ExprMakeArray::visit(Visitor & vis) {
        vis.preVisit(this);
        if ( makeType ) {
            vis.preVisit(makeType);
            makeType = makeType->visit(vis);
            makeType = vis.visit(makeType);
        }
        int index = 0;
        for ( auto it = values.begin(); it != values.end(); ) {
            auto & value = *it;
            vis.preVisitMakeArrayIndex(this, index, value, index==int(values.size()-1));
            value = value->visit(vis);
            if ( value ) {
                value = vis.visitMakeArrayIndex(this, index, value, index==int(values.size()-1));
            }
            if ( value ) ++it; else it = values.erase(it);
            index ++;
        }
        return vis.visit(this);
    }

    void ExprMakeArray::markNoDiscard() {
        for ( auto & val : values ) {
            val->markNoDiscard();
        }
    }

    // make tuple

    ExpressionPtr ExprMakeTuple::clone( ExpressionPtr expr ) const {
        auto cexpr = clonePtr<ExprMakeTuple>(expr);
        ExprMakeLocal::clone(cexpr);
        cexpr->values.reserve ( values.size() );
        for ( auto & val : values ) {
            cexpr->values.push_back(val->clone());
        }
        cexpr->recordNames = recordNames;
        cexpr->shorthandRecordNames = shorthandRecordNames;
        if ( makeType ) {
            cexpr->makeType = new TypeDecl(*makeType);
        }
        if (recordType) {
            cexpr->recordType = new TypeDecl(*recordType);
        }
        cexpr->isKeyValue = isKeyValue;
        return cexpr;
    }

    ExpressionPtr ExprMakeTuple::visit(Visitor & vis) {
        vis.preVisit(this);
        if ( makeType ) {
            vis.preVisit(makeType);
            makeType = makeType->visit(vis);
            makeType = vis.visit(makeType);
        }
        int index = 0;
        for ( auto it = values.begin(); it != values.end(); ) {
            auto & value = *it;
            vis.preVisitMakeTupleIndex(this, index, value, index==int(values.size()-1));
            value = value->visit(vis);
            if ( value ) {
                value = vis.visitMakeTupleIndex(this, index, value, index==int(values.size()-1));
            }
            if ( value ) ++it; else it = values.erase(it);
            index ++;
        }
        return vis.visit(this);
    }


    // array comprehension

    ExpressionPtr ExprArrayComprehension::clone( ExpressionPtr expr ) const {
        auto cexpr = clonePtr<ExprArrayComprehension>(expr);
        Expression::clone(cexpr);
        cexpr->exprFor = exprFor->clone();
        cexpr->subexpr = subexpr->clone();
        cexpr->generatorSyntax = generatorSyntax;
        cexpr->tableSyntax = tableSyntax;
        if ( exprWhere ) {
            cexpr->exprWhere = exprWhere->clone();
        }
        return cexpr;
    }

    ExpressionPtr ExprArrayComprehension::visit(Visitor & vis) {
        vis.preVisit(this);
        exprFor = exprFor->visit(vis);
        vis.preVisitArrayComprehensionSubexpr(this, subexpr);
        subexpr = subexpr->visit(vis);
        if ( exprWhere ) {
            vis.preVisitArrayComprehensionWhere(this, exprWhere);
            exprWhere = exprWhere->visit(vis);
        }
        return vis.visit(this);
    }

    pair<int64_t,bool> tryGetConstExprIntOrUInt ( ExpressionPtr expr ) {
        DAS_ASSERTF ( expr && expr->rtti_isConstant(),
                     "expecting constant. something in enumeration (or otherwise) did not fold.");
        auto econst = static_cast<ExprConst*>(expr);
        switch (econst->baseType) {
        case Type::tInt8:       return make_pair(static_cast<ExprConstInt8*>(expr)->getValue(), true);
        case Type::tUInt8:      return make_pair(static_cast<ExprConstUInt8*>(expr)->getValue(), true);
        case Type::tInt16:      return make_pair(static_cast<ExprConstInt16*>(expr)->getValue(), true);
        case Type::tUInt16:     return make_pair(static_cast<ExprConstUInt16*>(expr)->getValue(), true);
        case Type::tInt:        return make_pair(static_cast<ExprConstInt*>(expr)->getValue(), true);
        case Type::tUInt:       return make_pair(static_cast<ExprConstUInt*>(expr)->getValue(), true);
        case Type::tBitfield:   return make_pair(static_cast<ExprConstBitfield*>(expr)->getValue(), true);
        case Type::tBitfield8:  return make_pair(static_cast<ExprConstBitfield*>(expr)->getValue(), true);
        case Type::tBitfield16: return make_pair(static_cast<ExprConstBitfield*>(expr)->getValue(), true);
        case Type::tBitfield64: return make_pair(static_cast<ExprConstBitfield*>(expr)->getValue(), true);
        case Type::tInt64:      return make_pair(static_cast<ExprConstInt64*>(expr)->getValue(), true);
        case Type::tUInt64:     return make_pair(static_cast<ExprConstUInt64*>(expr)->getValue(), true);
        default:                return make_pair(0, false);
        }
    }

    int64_t getConstExprIntOrUInt ( ExpressionPtr expr ) {
        auto result = tryGetConstExprIntOrUInt(expr);
        DAS_ASSERTF ( result.second,
            "we should not even be here. there is an enumeration of unsupported type."
            "something in enumeration (or otherwise) did not fold.");
        return result.second ? result.first : 0;
    }

}
