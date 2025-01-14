#include "daScript/misc/platform.h"

#include "daScript/ast/ast.h"
#include "daScript/ast/ast_visitor.h"

namespace das {

    // local or global

    bool isLocalOrGlobal ( const ExpressionPtr & expr ) {
        if ( expr->rtti_isVar() ) {
            auto ev = static_pointer_cast<ExprVar>(expr);
            return ev->local || !(ev->argument || ev->block);
        } else if ( expr->rtti_isAt() ) {
            auto ea = static_pointer_cast<ExprAt>(expr);
            if ( ea->subexpr && ea->subexpr->type && ea->subexpr->type->dim.size() ) {
                return isLocalOrGlobal(ea->subexpr);
            }
        } else if ( expr->rtti_isField() ) {
            auto ef = static_pointer_cast<ExprField>(expr);
            if ( ef->value && ef->value->type && (ef->value->type->baseType!=Type::tHandle || ef->value->type->isLocal()) ) {
                return isLocalOrGlobal(ef->value);
            }
        } else if ( expr->rtti_isSwizzle() ) {
            auto sw = static_pointer_cast<ExprSwizzle>(expr);
            if ( sw->value ) {
                return isLocalOrGlobal(sw->value);
            }
        }
        return false;
    }

    // AOT

    AotListBase * AotListBase::head = nullptr;

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

    DAS_THREAD_LOCAL unique_ptr<AotLibrary> g_AOT_lib;

    AotLibrary & getGlobalAotLibrary() {
        if ( !g_AOT_lib ) {
            g_AOT_lib = make_unique<AotLibrary>();
            AotListBase::registerAot(*g_AOT_lib);
        }
        return *g_AOT_lib;
    }

    void clearGlobalAotLibrary() {
        g_AOT_lib.reset();
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
        return make_smart<TypeDecl>(baseType);
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
        TypeDeclPtr res;
        switch (baseType) {
        case Type::tInt8:
        case Type::tUInt8:
            res = make_smart<TypeDecl>(Type::tEnumeration8); break;
        case Type::tInt16:
        case Type::tUInt16:
            res = make_smart<TypeDecl>(Type::tEnumeration16); break;
        case Type::tInt:
        case Type::tUInt:
            res = make_smart<TypeDecl>(Type::tEnumeration); break;
        case Type::tInt64:
        case Type::tUInt64:
            res = make_smart<TypeDecl>(Type::tEnumeration64); break;
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
        for ( const auto & it : list ) {
            if ( getConstExprIntOrUInt(it.value)==va ) {
                return it.name;
            }
        }
        return def;
    }

    bool Enumeration::add ( const string & f, const LineInfo & att ) {
        return add(f, nullptr, att);
    }

    bool Enumeration::addI ( const string & f, int64_t value, const LineInfo & att ) {
        return add(f, make_smart<ExprConstInt64>(value),att);
    }

    bool Enumeration::addIEx ( const string & f, const string & fcpp, int64_t value, const LineInfo & att ) {
        return addEx(f, fcpp, make_smart<ExprConstInt64>(value),att);
    }

    bool Enumeration::addEx ( const string & na, const string & naCpp, const ExpressionPtr & expr, const LineInfo & att ) {
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

    bool Enumeration::add ( const string & na, const ExpressionPtr & expr, const LineInfo & att ) {
        return addEx(na,"",expr,att);
    }

    // structure

    uint64_t Structure::getOwnSemanticHash(HashBuilder & hb, das_set<Structure *> & dep, das_set<Annotation *> & adep) const {
        hb.updateString(getMangledName());
        hb.update(fields.size());
        for ( auto & fld : fields ) {
            hb.updateString(fld.name);
            hb.update(fld.type->getOwnSemanticHash(hb, dep, adep));
        }
        return hb.getHash();
    }

    StructurePtr Structure::clone() const {
        auto cs = make_smart<Structure>(name);
        cs->fields.reserve(fields.size());
        for ( auto & fd : fields ) {
            cs->fields.emplace_back(fd.name, fd.type, fd.init, fd.annotation, fd.moveSemantics, fd.at);
            cs->fields.back().flags = fd.flags;
        }
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

    bool Structure::canCopy(bool tempMatters) const {
        for ( const auto & fd : fields ) {
            if ( !fd.type->canCopy(tempMatters) )
                return false;
        }
        return true;
    }

    bool Structure::canMove() const {
        for ( const auto & fd : fields ) {
            if ( !fd.type->canMove() )
                return false;
        }
        return true;
    }

    bool Structure::canCloneFromConst() const {
        for ( const auto & fd : fields ) {
            if ( !fd.type->canCloneFromConst() )
                return false;
        }
        return true;
    }

    bool Structure::canClone() const {
        for ( const auto & fd : fields ) {
            if ( !fd.type->canClone() )
                return false;
        }
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
        int al = getAlignOf() - 1;
        size = (size + al) & ~al;
        return size;
    }

    int Structure::getAlignOf() const {
        int align = 1;
        for ( const auto & fd : fields ) {
            align = das::max ( fd.type->getAlignOf(), align );
        }
        return align;
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
        auto pVar = make_smart<Variable>();
        pVar->name = name;
        pVar->aka = aka;
        pVar->type = make_smart<TypeDecl>(*type);
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
        return hash_blockz64((uint8_t *)mangledName.c_str());
    }

    bool Variable::isAccessUnused() const {
        return !(access_get || access_init || access_pass || access_ref);
    }

    bool Variable::isCtorInitialized() const {
        if ( !init ) {
            return false;
        }
        if ( !type->hasNonTrivialCtor() ) {
            return false;
        }
        if ( init->rtti_isCallFunc() ) {
            auto cfun = static_pointer_cast<ExprCallFunc>(init);
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
            auto decl = make_smart<AnnotationDeclaration>();
            decl->annotation = ann->annotation;
            decl->arguments = ann->arguments;
            decl->at = ann->at;
            clist.push_back(decl);
        }
        return clist;
    }

    FunctionPtr Function::clone() const {
        auto cfun = make_smart<Function>();
        cfun->name = name;
        for ( const auto & arg : arguments ) {
            cfun->arguments.push_back(arg->clone());
        }
        cfun->annotations = cloneAnnotationList(annotations);
        cfun->result = make_smart<TypeDecl>(*result);
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
        FixedBufferTextWriter ss;
        getMangledName(ss);
        return ss.str();
    }

    void Function::getMangledName(FixedBufferTextWriter & ss) const {
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
                auto fna = static_pointer_cast<FunctionAnnotation>(ann->annotation);
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
                vis.preVisit(arg->type.get());
                arg->type = arg->type->visit(vis);
                arg->type = vis.visit(arg->type.get());
            }
            if ( arg->init && vis.canVisitArgumentInit(this,arg,arg->init.get()) ) {
                vis.preVisitArgumentInit(this, arg, arg->init.get());
                arg->init = arg->init->visit(vis);
                if ( arg->init ) {
                    arg->init = vis.visitArgumentInit(this, arg, arg->init.get());
                }
            }
            arg = vis.visitArgument(this, arg, arg==arguments.back() );
        }
        if ( result ) {
            vis.preVisit(result.get());
            result = result->visit(vis);
            result = vis.visit(result.get());
        }
        if ( body ) {
            vis.preVisitFunctionBody(this, body.get());
            if ( body ) body = body->visit(vis);
            if ( body ) body = vis.visitFunctionBody(this, body.get());
        }
        return vis.visit(this);
    }

    bool Function::isGeneric() const {
        for ( const auto & ann : annotations ) {
            if (ann->annotation) {
                auto fna = static_pointer_cast<FunctionAnnotation>(ann->annotation);
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
                auto pAnn = static_pointer_cast<FunctionAnnotation>(ann->annotation);
                return pAnn->aotArgumentPrefix(call, argIndex);
            }
        }
        return "";
    }

    string Function::getAotArgumentSuffix(ExprCallFunc * call, int argIndex) const {
        for ( auto & ann : annotations ) {
            if ( ann->annotation->rtti_isFunctionAnnotation() ) {
                auto pAnn = static_pointer_cast<FunctionAnnotation>(ann->annotation);
                return pAnn->aotArgumentSuffix(call, argIndex);
            }
        }
        return "";
    }

    string Function::getAotName(ExprCallFunc * call) const {
        for ( auto & ann : annotations ) {
            if ( ann->annotation->rtti_isFunctionAnnotation() ) {
                auto pAnn = static_pointer_cast<FunctionAnnotation>(ann->annotation);
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
            auto origin = fromGeneric.get();
            while ( origin->fromGeneric ) {
                origin = origin->fromGeneric.get();
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
            auto arg = make_smart<Variable>();
            arg->name = "arg" + to_string(argi-1);
            arg->type = args[argi];
            if ( arg->type->baseType==Type::fakeContext ) {
                arg->init = make_smart<ExprFakeContext>(at);
                arg->init->generated = true;
            } else if ( arg->type->baseType==Type::fakeLineInfo ) {
                arg->init = make_smart<ExprFakeLineInfo>(at);
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
            auto arg = make_smart<Variable>();
            arg->name = "arg" + to_string(argi-1);
            arg->type = args[argi];
            if ( arg->type->baseType==Type::fakeContext ) {
                arg->init = make_smart<ExprFakeContext>(at);
                arg->init->generated = true;
            } else if ( arg->type->baseType==Type::fakeLineInfo ) {
                arg->init = make_smart<ExprFakeLineInfo>(at);
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
            auto arg = make_smart<Variable>();
            arg->name = "arg" + to_string(argi-1);
            arg->type = args[argi];
            if ( arg->type->baseType==Type::fakeContext ) {
                arg->init = make_smart<ExprFakeContext>(at);
                arg->init->generated = true;
            } else if ( arg->type->baseType==Type::fakeLineInfo ) {
                arg->init = make_smart<ExprFakeLineInfo>(at);
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

    // add extern func

    void addExternFunc(Module& mod, const FunctionPtr & fnX, bool isCmres, SideEffects seFlags) {
        if (!isCmres) {
            if (fnX->result->isRefType() && !fnX->result->ref) {
                DAS_FATAL_ERROR(
                    "addExtern(%s)::failed in module %s\n"
                    "  this function should be bound with addExtern<DAS_BIND_FUNC(%s), SimNode_ExtFuncCallAndCopyOrMove>\n"
                    "  likely cast<> is implemented for the return type, and it should not\n",
                    fnX->name.c_str(), mod.name.c_str(), fnX->name.c_str());
            }
        }
        fnX->setSideEffects(seFlags);
        if (!mod.addFunction(fnX)) {
            DAS_FATAL_ERROR("addExtern(%s) failed in module %s\n", fnX->name.c_str(), mod.name.c_str());
        }
    }

    // expression

    ExpressionPtr Expression::clone( const ExpressionPtr & expr ) const {
        if ( !expr ) {
            DAS_ASSERTF(0,
                   "its not ok to clone Expression as is."
                   "if we are here, this means that ::clone function is not written correctly."
                   "incorrect clone function can be found on the stack above this.");
            return nullptr;
        }
        expr->at = at;
        expr->type = type ? make_smart<TypeDecl>(*type) : nullptr;
        expr->genFlags = genFlags;
        return expr;
    }

    ExpressionPtr Expression::autoDereference ( const ExpressionPtr & expr ) {
        if ( expr->type && !expr->type->isAutoOrAlias() && expr->type->isRef() && !expr->type->isRefType() ) {
            auto ar2l = make_smart<ExprRef2Value>();
            ar2l->subexpr = expr;
            ar2l->at = expr->at;
            ar2l->type = make_smart<TypeDecl>(*expr->type);
            ar2l->type->ref = false;
            return ar2l;
        } else {
            return expr;
        }
    }

   // Reader

    ExpressionPtr ExprReader::visit(Visitor & vis) {
        vis.preVisit(this);
        return vis.visit(this);
    }

    ExpressionPtr ExprReader::clone( const ExpressionPtr & expr ) const {
        auto cexpr = clonePtr<ExprReader>(expr);
        Expression::clone(cexpr);
        cexpr->macro = macro;
        cexpr->sequence = sequence;
        return cexpr;
    }

    // Label

    ExpressionPtr ExprLabel::visit(Visitor & vis) {
        vis.preVisit(this);
        return vis.visit(this);
    }

    ExpressionPtr ExprLabel::clone( const ExpressionPtr & expr ) const {
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

    ExpressionPtr ExprGoto::clone( const ExpressionPtr & expr ) const {
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

    ExpressionPtr ExprRef2Value::clone( const ExpressionPtr & expr ) const {
        auto cexpr = clonePtr<ExprRef2Value>(expr);
        Expression::clone(cexpr);
        cexpr->subexpr = subexpr->clone();
        return cexpr;
    }

    // ExprRef2Ptr

    ExpressionPtr ExprRef2Ptr::visit(Visitor & vis) {
        vis.preVisit(this);
        subexpr = subexpr->visit(vis);
        return vis.visit(this);
    }

    ExpressionPtr ExprRef2Ptr::clone( const ExpressionPtr & expr ) const {
        auto cexpr = clonePtr<ExprRef2Ptr>(expr);
        Expression::clone(cexpr);
        cexpr->subexpr = subexpr->clone();
        return cexpr;
    }

    // ExprPtr2Ref

    ExpressionPtr ExprPtr2Ref::visit(Visitor & vis) {
        vis.preVisit(this);
        subexpr = subexpr->visit(vis);
        return vis.visit(this);
    }

    ExpressionPtr ExprPtr2Ref::clone( const ExpressionPtr & expr ) const {
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
            vis.preVisit(funcType.get());
            funcType = funcType->visit(vis);
            funcType = vis.visit(funcType.get());
        }
        return vis.visit(this);
    }

    ExpressionPtr ExprAddr::clone( const ExpressionPtr & expr ) const {
        auto cexpr = clonePtr<ExprAddr>(expr);
        Expression::clone(cexpr);
        cexpr->target = target;
        cexpr->func = func;
        if (funcType) cexpr->funcType = make_smart<TypeDecl>(*funcType);
        return cexpr;
    }

    // ExprNullCoalescing

    ExpressionPtr ExprNullCoalescing::visit(Visitor & vis) {
        vis.preVisit(this);
        subexpr = subexpr->visit(vis);
        vis.preVisitNullCoaelescingDefault(this, defaultValue.get());
        defaultValue = defaultValue->visit(vis);
        return vis.visit(this);
    }

    ExpressionPtr ExprNullCoalescing::clone( const ExpressionPtr & expr ) const {
        auto cexpr = clonePtr<ExprNullCoalescing>(expr);
        ExprPtr2Ref::clone(cexpr);
        cexpr->defaultValue = defaultValue->clone();
        return cexpr;
    }

    // ConstBitfield

    ExpressionPtr ExprConstBitfield::clone( const ExpressionPtr & expr ) const {
        auto cexpr = clonePtr<ExprConstBitfield>(expr);
        ExprConstT<uint32_t,ExprConstBitfield>::clone(cexpr);
        if ( bitfieldType ) {
            cexpr->bitfieldType = make_smart<TypeDecl>(*bitfieldType);
        }
        return cexpr;
    }

    // ConstPtr

    ExpressionPtr ExprConstPtr::clone( const ExpressionPtr & expr ) const {
        auto cexpr = clonePtr<ExprConstPtr>(expr);
        ExprConst::clone(cexpr);
        cexpr->isSmartPtr = isSmartPtr;
        if ( ptrType ) {
            cexpr->ptrType = make_smart<TypeDecl>(*ptrType);
        }
        return cexpr;
    }

    // ConstEnumeration

    ExpressionPtr ExprConstEnumeration::visit(Visitor & vis) {
        vis.preVisit((ExprConst *)this);
        vis.preVisit(this);
        auto res = vis.visit(this);
        if ( res.get() != this )
            return res;
        return vis.visit((ExprConst *)this);
    }

    ExpressionPtr ExprConstEnumeration::clone( const ExpressionPtr & expr ) const {
        auto cexpr = clonePtr<ExprConstEnumeration>(expr);
        ExprConst::clone(cexpr);
        cexpr->enumType = enumType;
        cexpr->text = text;
        cexpr->value = value;
        return cexpr;
    }

    // ConstString

    ExpressionPtr ExprConstString::visit(Visitor & vis) {
        vis.preVisit((ExprConst *)this);
        vis.preVisit(this);
        auto res = vis.visit(this);
        if ( res.get() != this )
            return res;
        return vis.visit((ExprConst *)this);
    }

    ExpressionPtr ExprConstString::clone( const ExpressionPtr & expr ) const {
        auto cexpr = clonePtr<ExprConstString>(expr);
        Expression::clone(cexpr);
        cexpr->value = value;
        cexpr->text = text;
        return cexpr;
    }

    string TypeDecl::typeMacroName() const {
        if ( dimExpr.size()<1 ) return "";
        if ( dimExpr[0]->rtti_isStringConstant() ) {
            return ((ExprConstString *)dimExpr[0].get())->text;
        } else {
            return "";
        }
    }

    // ExprStaticAssert

    ExpressionPtr ExprStaticAssert::clone( const ExpressionPtr & expr ) const {
        auto cexpr = clonePtr<ExprStaticAssert>(expr);
        ExprLooksLikeCall::clone(cexpr);
        return cexpr;
    }

    // ExprAssert

    ExpressionPtr ExprAssert::clone( const ExpressionPtr & expr ) const {
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
                vis.preVisitLooksLikeCallArg(this, arg.get(), arg==arguments.back());
                arg = arg->visit(vis);
                arg = vis.visitLooksLikeCallArg(this, arg.get(), arg==arguments.back());
            }
        }
        return vis.visit(this);
    }

    ExpressionPtr ExprQuote::clone( const ExpressionPtr & expr ) const {
        auto cexpr = clonePtr<ExprQuote>(expr);
        ExprLooksLikeCall::clone(cexpr);
        return cexpr;
    }

    // ExprDebug

    ExpressionPtr ExprDebug::clone( const ExpressionPtr & expr ) const {
        auto cexpr = clonePtr<ExprDebug>(expr);
        ExprLooksLikeCall::clone(cexpr);
        return cexpr;
    }

    // ExprMemZero

    ExpressionPtr ExprMemZero::clone( const ExpressionPtr & expr ) const {
        auto cexpr = clonePtr<ExprMemZero>(expr);
        ExprLooksLikeCall::clone(cexpr);
        return cexpr;
    }

    // ExprMakeGenerator

    ExprMakeGenerator::ExprMakeGenerator ( const LineInfo & a, const ExpressionPtr & b )
    : ExprLooksLikeCall(a, "generator") {
        __rtti = "ExprMakeGenerator";
        if ( b ) {
            arguments.push_back(b);
        }
    }

    ExpressionPtr ExprMakeGenerator::visit(Visitor & vis) {
        vis.preVisit(this);
        if ( iterType ) {
            vis.preVisit(iterType.get());
            iterType = iterType->visit(vis);
            iterType = vis.visit(iterType.get());
        }
        ExprLooksLikeCall::visit(vis);
        return vis.visit(this);
    }

    ExpressionPtr ExprMakeGenerator::clone( const ExpressionPtr & expr ) const {
        auto cexpr = clonePtr<ExprMakeGenerator>(expr);
        ExprLooksLikeCall::clone(cexpr);
        if ( iterType ) {
            cexpr->iterType = make_smart<TypeDecl>(*iterType);
        }
        cexpr->capture = capture;
        return cexpr;
    }

    // ExprYield

    ExprYield::ExprYield ( const LineInfo & a, const ExpressionPtr & b ) : Expression(a) {
        __rtti = "ExprYield";
        subexpr = b;
    }

    ExpressionPtr ExprYield::visit(Visitor & vis) {
        vis.preVisit(this);
        subexpr = subexpr->visit(vis);
        return vis.visit(this);
    }

    ExpressionPtr ExprYield::clone( const ExpressionPtr & expr ) const {
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

    ExpressionPtr ExprMakeBlock::clone( const ExpressionPtr & expr ) const {
        auto cexpr = clonePtr<ExprMakeBlock>(expr);
        Expression::clone(cexpr);
        cexpr->block = block->clone();
        cexpr->isLambda = isLambda;
        cexpr->isLocalFunction = isLocalFunction;
        cexpr->capture = capture;
        cexpr->aotFunctorName = aotFunctorName;
        return cexpr;
    }

    // ExprInvoke

    ExpressionPtr ExprInvoke::clone( const ExpressionPtr & expr ) const {
        auto cexpr = clonePtr<ExprInvoke>(expr);
        ExprLooksLikeCall::clone(cexpr);
        cexpr->isInvokeMethod = isInvokeMethod;
        return cexpr;
    }

    // ExprErase

    ExpressionPtr ExprErase::clone( const ExpressionPtr & expr ) const {
        auto cexpr = clonePtr<ExprErase>(expr);
        ExprLooksLikeCall::clone(cexpr);
        return cexpr;
    }

    // ExprSetInsert

    ExpressionPtr ExprSetInsert::clone( const ExpressionPtr & expr ) const {
        auto cexpr = clonePtr<ExprSetInsert>(expr);
        ExprLooksLikeCall::clone(cexpr);
        return cexpr;
    }

    // ExprFind

    ExpressionPtr ExprFind::clone( const ExpressionPtr & expr ) const {
        auto cexpr = clonePtr<ExprFind>(expr);
        ExprLooksLikeCall::clone(cexpr);
        return cexpr;
    }

    // ExprKeyExists

    ExpressionPtr ExprKeyExists::clone( const ExpressionPtr & expr ) const {
        auto cexpr = clonePtr<ExprKeyExists>(expr);
        ExprLooksLikeCall::clone(cexpr);
        return cexpr;
    }

    // ExprIs

    ExpressionPtr ExprIs::visit(Visitor & vis) {
        vis.preVisit(this);
        subexpr = subexpr->visit(vis);
        vis.preVisitType(this, typeexpr.get());
        vis.preVisit(typeexpr.get());
        typeexpr = typeexpr->visit(vis);
        typeexpr = vis.visit(typeexpr.get());
        return vis.visit(this);
    }

    ExpressionPtr ExprIs::clone( const ExpressionPtr & expr ) const {
        auto cexpr = clonePtr<ExprIs>(expr);
        Expression::clone(cexpr);
        cexpr->subexpr = subexpr->clone();
        cexpr->typeexpr = make_smart<TypeDecl>(*typeexpr);
        return cexpr;
    }

    // ExprTypeDecl

    ExpressionPtr ExprTypeDecl::visit(Visitor & vis) {
        vis.preVisit(this);
        if ( typeexpr ) {
            vis.preVisit(typeexpr.get());
            typeexpr = typeexpr->visit(vis);
            typeexpr = vis.visit(typeexpr.get());
        }
        return vis.visit(this);
    }

    ExpressionPtr ExprTypeDecl::clone( const ExpressionPtr & expr ) const {
        auto cexpr = clonePtr<ExprTypeDecl>(expr);
        Expression::clone(cexpr);
        cexpr->typeexpr = typeexpr;
        return cexpr;
    }

    // ExprTypeInfo

    ExpressionPtr ExprTypeInfo::visit(Visitor & vis) {
        vis.preVisit(this);
        if ( typeexpr ) {
            vis.preVisit(typeexpr.get());
            typeexpr = typeexpr->visit(vis);
            typeexpr = vis.visit(typeexpr.get());
        }
        if ( vis.canVisitExpr(this,subexpr.get()) ) {
            if ( subexpr ) {
                subexpr = subexpr->visit(vis);
            }
        }
        return vis.visit(this);
    }

    ExpressionPtr ExprTypeInfo::clone( const ExpressionPtr & expr ) const {
        auto cexpr = clonePtr<ExprTypeInfo>(expr);
        Expression::clone(cexpr);
        cexpr->trait = trait;
        cexpr->subtrait = subtrait;
        cexpr->extratrait = extratrait;
        if ( subexpr )
            cexpr->subexpr = subexpr->clone();
        if ( typeexpr )
            cexpr->typeexpr = typeexpr;
        return cexpr;
    }

    // ExprDelete

    ExpressionPtr ExprDelete::visit(Visitor & vis) {
        vis.preVisit(this);
        if ( subexpr ) {
            subexpr = subexpr->visit(vis);
        }
        if ( sizeexpr ) {
            vis.preVisitDeleteSizeExpression(this, sizeexpr.get());
            sizeexpr = sizeexpr->visit(vis);
        }
        return vis.visit(this);
    }

    ExpressionPtr ExprDelete::clone( const ExpressionPtr & expr ) const {
        auto cexpr = clonePtr<ExprDelete>(expr);
        Expression::clone(cexpr);
        cexpr->subexpr = subexpr->clone();
        cexpr->sizeexpr = sizeexpr ? sizeexpr->clone() : nullptr;
        cexpr->native = native;
        return cexpr;
    }

    // ExprCast

    ExpressionPtr ExprCast::clone( const ExpressionPtr & expr  ) const {
        auto cexpr = clonePtr<ExprCast>(expr);
        Expression::clone(cexpr);
        cexpr->subexpr = subexpr->clone();
        cexpr->castType = make_smart<TypeDecl>(*castType);
        cexpr->castFlags = castFlags;
        return cexpr;
    }

    ExpressionPtr ExprCast::visit(Visitor & vis) {
        vis.preVisit(this);
        if ( castType ) {
            vis.preVisit(castType.get());
            castType = castType->visit(vis);
            castType = vis.visit(castType.get());
        }
        subexpr = subexpr->visit(vis);
        return vis.visit(this);
    }

    // ExprAscend

    ExpressionPtr ExprAscend::clone( const ExpressionPtr & expr  ) const {
        auto cexpr = clonePtr<ExprAscend>(expr);
        Expression::clone(cexpr);
        cexpr->subexpr = subexpr->clone();
        if ( ascType ) cexpr->ascType = make_smart<TypeDecl>(*ascType);
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
            vis.preVisit(typeexpr.get());
            typeexpr = typeexpr->visit(vis);
            typeexpr = vis.visit(typeexpr.get());
        }
        for ( auto & arg : arguments ) {
            vis.preVisitNewArg(this, arg.get(), arg==arguments.back());
            arg = arg->visit(vis);
            arg = vis.visitNewArg(this, arg.get(), arg==arguments.back());
        }
        return vis.visit(this);
    }

    ExpressionPtr ExprNew::clone( const ExpressionPtr & expr ) const {
        auto cexpr = clonePtr<ExprNew>(expr);
        ExprLooksLikeCall::clone(cexpr);
        cexpr->typeexpr = typeexpr;
        cexpr->initializer = initializer;
        return cexpr;
    }

    // ExprAt

    ExpressionPtr ExprAt::visit(Visitor & vis) {
        vis.preVisit(this);
        subexpr = subexpr->visit(vis);
        vis.preVisitAtIndex(this, index.get());
        index = index->visit(vis);
        return vis.visit(this);
    }

    ExpressionPtr ExprAt::clone( const ExpressionPtr & expr ) const {
        auto cexpr = clonePtr<ExprAt>(expr);
        Expression::clone(cexpr);
        cexpr->subexpr = subexpr->clone();
        cexpr->index = index->clone();
        cexpr->no_promotion = no_promotion;
        return cexpr;
    }

    // ExprSafeAt

    ExpressionPtr ExprSafeAt::visit(Visitor & vis) {
        vis.preVisit(this);
        subexpr = subexpr->visit(vis);
        vis.preVisitSafeAtIndex(this, index.get());
        index = index->visit(vis);
        return vis.visit(this);
    }

    ExpressionPtr ExprSafeAt::clone( const ExpressionPtr & expr ) const {
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
                auto blk = static_pointer_cast<ExprBlock>(ex);
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
        auto eT = make_smart<TypeDecl>(Type::tBlock);
        eT->constant = true;
        if ( type ) {
            eT->firstType = make_smart<TypeDecl>(*type);
        }
        for ( auto & arg : arguments ) {
            if ( arg->type ) {
                eT->argTypes.push_back(make_smart<TypeDecl>(*arg->type));
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
                vis.preVisitBlockFinalExpression(this, subexpr.get());
                if ( subexpr ) subexpr = subexpr->visit(vis);
                if ( subexpr ) subexpr = vis.visitBlockFinalExpression(this, subexpr.get());
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
                  vis.preVisit(arg->type.get());
                  arg->type = arg->type->visit(vis);
                  arg->type = vis.visit(arg->type.get());
              }
              if ( arg->init ) {
                  vis.preVisitBlockArgumentInit(this, arg, arg->init.get());
                  arg->init = arg->init->visit(vis);
                  if ( arg->init ) {
                      arg->init = vis.visitBlockArgumentInit(this, arg, arg->init.get());
                  }
              }
              arg = vis.visitBlockArgument(this, arg, arg==arguments.back());
              if ( arg ) ++it; else it = arguments.erase(it);
          }
          if ( returnType ) {
              vis.preVisit(returnType.get());
              returnType = returnType->visit(vis);
              returnType = vis.visit(returnType.get());
          }
        }
        if ( finallyBeforeBody ) {
            visitFinally(vis);
        }
        for ( auto it = list.begin(); it!=list.end(); ) {
            auto & subexpr = *it;
            vis.preVisitBlockExpression(this, subexpr.get());
            if ( subexpr ) subexpr = subexpr->visit(vis);
            if ( subexpr ) subexpr = vis.visitBlockExpression(this, subexpr.get());
            if ( subexpr ) ++it; else it = list.erase(it);
        }
        if ( !finallyBeforeBody ) {
            visitFinally(vis);
        }
        return vis.visit(this);
    }

    ExpressionPtr ExprBlock::clone( const ExpressionPtr & expr ) const {
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
            cexpr->returnType = make_smart<TypeDecl>(*returnType);
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

    ExpressionPtr ExprSwizzle::clone( const ExpressionPtr & expr ) const {
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

    ExpressionPtr ExprField::clone( const ExpressionPtr & expr ) const {
        auto cexpr = clonePtr<ExprField>(expr);
        Expression::clone(cexpr);
        cexpr->name = name;
        if ( value) {
            cexpr->value = value->clone();
        }
        cexpr->field = field;
        cexpr->fieldIndex = fieldIndex;
        cexpr->unsafeDeref = unsafeDeref;
        cexpr->no_promotion = no_promotion;
        cexpr->atField = atField;
        return cexpr;
    }

    // ExprIs

    ExpressionPtr ExprIsVariant::visit(Visitor & vis) {
        vis.preVisit(this);
        value = value->visit(vis);
        return vis.visit(this);
    }

    ExpressionPtr ExprIsVariant::clone( const ExpressionPtr & expr ) const {
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

    ExpressionPtr ExprAsVariant::clone( const ExpressionPtr & expr ) const {
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

    ExpressionPtr ExprSafeAsVariant::clone( const ExpressionPtr & expr ) const {
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

    ExpressionPtr ExprSafeField::clone( const ExpressionPtr & expr ) const {
        auto cexpr = clonePtr<ExprSafeField>(expr);
        ExprField::clone(cexpr);
        return cexpr;
    }

    // ExprStringBuilder

    ExpressionPtr ExprStringBuilder::visit(Visitor & vis) {
        vis.preVisit(this);
        for ( auto it = elements.begin(); it!=elements.end(); ) {
            auto & elem = *it;
            vis.preVisitStringBuilderElement(this, elem.get(), elem==elements.back());
            elem = elem->visit(vis);
            if ( elem ) elem = vis.visitStringBuilderElement(this, elem.get(), elem==elements.back());
            if ( elem ) ++ it; else it = elements.erase(it);
        }
        return vis.visit(this);
    }

    ExpressionPtr ExprStringBuilder::clone( const ExpressionPtr & expr  ) const {
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

    ExpressionPtr ExprVar::clone( const ExpressionPtr & expr ) const {
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
            vis.preVisitTagValue(this, value.get());
            value = value->visit(vis);
        }
        return vis.visit(this);
    }

    ExpressionPtr ExprTag::clone ( const ExpressionPtr & expr ) const {
        auto cexpr = clonePtr<ExprTag>(expr);
        Expression::clone(cexpr);
        if ( subexpr )  cexpr->subexpr = subexpr->clone();
        if ( value ) cexpr->value = value->clone();
        cexpr->name = name;
        return cexpr;
    }

    // ExprOp

    ExpressionPtr ExprOp::clone( const ExpressionPtr & expr ) const {
        if ( !expr ) {
            DAS_ASSERTF(0,"can't clone ExprOp");
            return nullptr;
        }
        auto cexpr = static_pointer_cast<ExprOp>(expr);
        ExprCallFunc::clone(cexpr);
        cexpr->op = op;
        cexpr->func = func;
        cexpr->at = at;
        return cexpr;
    }

    // ExprOp1

    bool ExprOp1::swap_tail ( Expression * expr, Expression * swapExpr ) {
        if ( subexpr.get()==expr ) {
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

    ExpressionPtr ExprOp1::clone( const ExpressionPtr & expr ) const {
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

    // ExprOp2

    bool ExprOp2::swap_tail ( Expression * expr, Expression * swapExpr ) {
        if ( right.get()==expr ) {
            right = swapExpr;
            return true;
        } else {
            return right->swap_tail(expr,swapExpr);
        }
    }

    ExpressionPtr ExprOp2::visit(Visitor & vis) {
        vis.preVisit(this);
        left = left->visit(vis);
        vis.preVisitRight(this, right.get());
        right = right->visit(vis);
        return vis.visit(this);
    }

    ExpressionPtr ExprOp2::clone( const ExpressionPtr & expr ) const {
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

    // ExprOp3

    bool ExprOp3::swap_tail ( Expression * expr, Expression * swapExpr ) {
        if ( right.get()==expr ) {
            right = swapExpr;
            return true;
        } else {
            return right->swap_tail(expr,swapExpr);
        }
    }

    ExpressionPtr ExprOp3::visit(Visitor & vis) {
        vis.preVisit(this);
        subexpr = subexpr->visit(vis);
        vis.preVisitLeft(this, left.get());
        left = left->visit(vis);
        vis.preVisitRight(this, right.get());
        right = right->visit(vis);
        return vis.visit(this);
    }

    ExpressionPtr ExprOp3::clone( const ExpressionPtr & expr ) const {
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


    // ExprMove

    ExpressionPtr ExprMove::visit(Visitor & vis) {
        vis.preVisit(this);
        if ( vis.isRightFirst(this) ) {
            vis.preVisitRight(this, right.get());
            right = right->visit(vis);
            left = left->visit(vis);
        } else {
            left = left->visit(vis);
            vis.preVisitRight(this, right.get());
            right = right->visit(vis);
        }
        return vis.visit(this);
    }

    ExpressionPtr ExprMove::clone( const ExpressionPtr & expr ) const {
        auto cexpr = clonePtr<ExprMove>(expr);
        ExprOp2::clone(cexpr);
        cexpr->moveFlags = moveFlags;
        return cexpr;
    }

    // ExprClone

    ExpressionPtr ExprClone::visit(Visitor & vis) {
        vis.preVisit(this);
        if ( vis.isRightFirst(this) ) {
            vis.preVisitRight(this, right.get());
            right = right->visit(vis);
            left = left->visit(vis);
        } else {
            left = left->visit(vis);
            vis.preVisitRight(this, right.get());
            right = right->visit(vis);
        }
        return vis.visit(this);
    }

    ExpressionPtr ExprClone::clone( const ExpressionPtr & expr ) const {
        auto cexpr = clonePtr<ExprClone>(expr);
        ExprOp2::clone(cexpr);
        return cexpr;
    }

    // ExprCopy

    ExpressionPtr ExprCopy::visit(Visitor & vis) {
        vis.preVisit(this);
        if ( vis.isRightFirst(this) ) {
            vis.preVisitRight(this, right.get());
            right = right->visit(vis);
            left = left->visit(vis);
        } else {
            left = left->visit(vis);
            vis.preVisitRight(this, right.get());
            right = right->visit(vis);
        }
        return vis.visit(this);
    }

    ExpressionPtr ExprCopy::clone( const ExpressionPtr & expr ) const {
        auto cexpr = clonePtr<ExprCopy>(expr);
        ExprOp2::clone(cexpr);
        cexpr->copyFlags = copyFlags;
        return cexpr;
    }

    // ExprTryCatch

    ExpressionPtr ExprTryCatch::visit(Visitor & vis) {
        vis.preVisit(this);
        try_block = try_block->visit(vis);
        vis.preVisitCatch(this,catch_block.get());
        catch_block = catch_block->visit(vis);
        return vis.visit(this);
    }

    uint32_t ExprTryCatch::getEvalFlags() const {
        return try_block->getEvalFlags() | catch_block->getEvalFlags();
    }

    ExpressionPtr ExprTryCatch::clone( const ExpressionPtr & expr ) const {
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

    ExpressionPtr ExprReturn::clone( const ExpressionPtr & expr ) const {
        auto cexpr = clonePtr<ExprReturn>(expr);
        Expression::clone(cexpr);
        if ( subexpr )
            cexpr->subexpr = subexpr->clone();
        cexpr->moveSemantics = moveSemantics;
        cexpr->fromYield = fromYield;
        return cexpr;
    }

    // ExprBreak

    ExpressionPtr ExprBreak::visit(Visitor & vis) {
        vis.preVisit(this);
        return vis.visit(this);
    }

    ExpressionPtr ExprBreak::clone( const ExpressionPtr & expr ) const {
        auto cexpr = clonePtr<ExprBreak>(expr);
        Expression::clone(cexpr);
        return cexpr;
    }

    // ExprContinue

    ExpressionPtr ExprContinue::visit(Visitor & vis) {
        vis.preVisit(this);
        return vis.visit(this);
    }

    ExpressionPtr ExprContinue::clone( const ExpressionPtr & expr ) const {
        auto cexpr = clonePtr<ExprContinue>(expr);
        Expression::clone(cexpr);
        return cexpr;
    }

    // ExprIfThenElse

    ExpressionPtr ExprIfThenElse::visit(Visitor & vis) {
        vis.preVisit(this);
        cond = cond->visit(vis);
        if ( vis.canVisitIfSubexpr(this) ) {
            vis.preVisitIfBlock(this, if_true.get());
            if_true = if_true->visit(vis);
            if ( if_false ) {
                vis.preVisitElseBlock(this, if_false.get());
                if_false = if_false->visit(vis);
            }
        }
        return vis.visit(this);
    }

    ExpressionPtr ExprIfThenElse::clone( const ExpressionPtr & expr ) const {
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
        vis.preVisitWithBody(this, body.get());
        body = body->visit(vis);
        return vis.visit(this);
    }

    ExpressionPtr ExprWith::clone( const ExpressionPtr & expr ) const {
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
            subexpr = subexpr->visit(vis);
        }
        return vis.visit(this);
    }

    ExpressionPtr ExprAssume::clone( const ExpressionPtr & expr ) const {
        auto cexpr = clonePtr<ExprAssume>(expr);
        Expression::clone(cexpr);
        cexpr->alias = alias;
        cexpr->subexpr = subexpr->clone();
        return cexpr;
    }

    // ExprUnsafe

    ExpressionPtr ExprUnsafe::visit(Visitor & vis) {
        vis.preVisit(this);
        body = body->visit(vis);
        return vis.visit(this);
    }

    ExpressionPtr ExprUnsafe::clone( const ExpressionPtr & expr ) const {
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
        vis.preVisitWhileBody(this, body.get());
        body = body->visit(vis);
        return vis.visit(this);
    }

    ExpressionPtr ExprWhile::clone( const ExpressionPtr & expr ) const {
        auto cexpr = clonePtr<ExprWhile>(expr);
        Expression::clone(cexpr);
        cexpr->cond = cond->clone();
        cexpr->body = body->clone();
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
            vis.preVisitForSource(this, src.get(), src==sources.back());
            src = src->visit(vis);
            src = vis.visitForSource(this, src.get(), src==sources.back());
        }
        vis.preVisitForStack(this);
        if ( body ) {
            vis.preVisitForBody(this, body.get());
            body = body->visit(vis);
        }
        return vis.visit(this);
    }

    ExpressionPtr ExprFor::clone( const ExpressionPtr & expr ) const {
        auto cexpr = clonePtr<ExprFor>(expr);
        Expression::clone(cexpr);
        cexpr->iterators = iterators;
        cexpr->iteratorsAka = iteratorsAka;
        cexpr->iteratorsAt = iteratorsAt;
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
        return cexpr;
    }

    Variable * ExprFor::findIterator(const string & name) const {
        for ( auto & v : iteratorVariables ) {
            if ( v->name==name ) {
                return v.get();
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
                vis.preVisit(var->type.get());
                var->type = var->type->visit(vis);
                var->type = vis.visit(var->type.get());
            }
            if ( var->init ) {
                vis.preVisitLetInit(this, var, var->init.get());
                var->init = var->init->visit(vis);
                var->init = vis.visitLetInit(this, var, var->init.get());
            }
            var = vis.visitLet(this, var, var==variables.back());
            if ( var ) ++it; else it = variables.erase(it);
        }
        return vis.visit(this);
    }

    ExpressionPtr ExprLet::clone( const ExpressionPtr & expr ) const {
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
                return v.get();
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
                vis.preVisitLooksLikeCallArg(this, arg.get(), arg==arguments.back());
                arg = arg->visit(vis);
                arg = vis.visitLooksLikeCallArg(this, arg.get(), arg==arguments.back());
            }
            index ++;
        }
        return vis.visit(this);
    }

    ExpressionPtr ExprCallMacro::clone( const ExpressionPtr & expr ) const {
        auto cexpr = clonePtr<ExprCallMacro>(expr);
        ExprLooksLikeCall::clone(cexpr);
        cexpr->macro = macro;
        cexpr->inFunction = inFunction;
        return cexpr;
    }

    // ExprLooksLikeCall

    ExpressionPtr ExprLooksLikeCall::visit(Visitor & vis) {
        vis.preVisit(this);
        for ( auto & arg : arguments ) {
            if ( vis.canVisitLooksLikeCallArg(this, arg.get(), arg==arguments.back()) ) {
                vis.preVisitLooksLikeCallArg(this, arg.get(), arg==arguments.back());
                arg = arg->visit(vis);
                arg = vis.visitLooksLikeCallArg(this, arg.get(), arg==arguments.back());
            }
        }
        return vis.visit(this);
    }

    ExpressionPtr ExprLooksLikeCall::clone( const ExpressionPtr & expr ) const {
        auto cexpr = clonePtr<ExprLooksLikeCall>(expr);
        Expression::clone(cexpr);
        cexpr->atEnclosure = atEnclosure;
        cexpr->name = name;
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
                vis.preVisitCallArg(this, arg.get(), arg==arguments.back());
                arg = arg->visit(vis);
                arg = vis.visitCallArg(this, arg.get(), arg==arguments.back());
            }
            return vis.visit(this);
        } else {
            return this;
        }
    }

    ExpressionPtr ExprCall::clone( const ExpressionPtr & expr ) const {
        auto cexpr = clonePtr<ExprCall>(expr);
        ExprLooksLikeCall::clone(cexpr);
        cexpr->func = func;
        return cexpr;
    }

    // named call

    ExpressionPtr ExprNamedCall::visit(Visitor & vis) {
        vis.preVisit(this);

        if (nonNamedArguments.size() > 0) {
            ExprCall dummy;
            for (auto& arg : nonNamedArguments) {
                vis.preVisitCallArg(&dummy, arg.get(), arg == nonNamedArguments.back());
                arg = arg->visit(vis);
                arg = vis.visitCallArg(&dummy, arg.get(), arg == nonNamedArguments.back());
            }
            this->argumentsFailedToInfer = dummy.argumentsFailedToInfer;
        }
        for (auto& arg : arguments) {
            vis.preVisitNamedCallArg(this, arg.get(), arg == arguments.back());
            arg->value = arg->value->visit(vis);
            arg = vis.visitNamedCallArg(this, arg.get(), arg==arguments.back());
        }
        return vis.visit(this);
    }

    ExpressionPtr ExprNamedCall::clone( const ExpressionPtr & expr ) const {
        auto cexpr = clonePtr<ExprNamedCall>(expr);
        Expression::clone(cexpr);
        cexpr->name = name;
        cexpr->arguments.reserve(arguments.size());
        for ( auto & arg : arguments ) {
            cexpr->arguments.push_back(arg->clone());
        }
        return cexpr;
    }

    // make structure

    MakeFieldDeclPtr MakeFieldDecl::clone() const {
        auto md = make_smart<MakeFieldDecl>();
        md->at = at;
        md->flags = flags;
        md->name = name;
        md->value = value ? value->clone() : nullptr;
        md->tag = tag ? tag->clone() : nullptr;
        return md;
    }

    ExpressionPtr ExprMakeStruct::clone( const ExpressionPtr & expr ) const {
        auto cexpr = clonePtr<ExprMakeStruct>(expr);
        ExprMakeLocal::clone(cexpr);
        cexpr->structs.reserve ( structs.size() );
        for ( auto & fields : structs ) {
            auto mfd = make_smart<MakeStruct>();
            mfd->reserve( fields->size() );
            for ( auto & fd : *fields ) {
                mfd->push_back(fd->clone());
            }
            cexpr->structs.push_back(mfd);
        }
        cexpr->makeType = make_smart<TypeDecl>(*makeType);
        cexpr->makeStructFlags = makeStructFlags;
        if ( block ) {
            cexpr->block = block->clone();
        }
        return cexpr;
    }

    ExpressionPtr ExprMakeStruct::visit(Visitor & vis) {
        vis.preVisit(this);
        if ( makeType ) {
            vis.preVisit(makeType.get());
            makeType = makeType->visit(vis);
            makeType = vis.visit(makeType.get());
        }
        if ( vis.canVisitMakeStructureBody(this) ) {
            for ( int index=0; index != int(structs.size()); ++index ) {
                vis.preVisitMakeStructureIndex(this, index, index==int(structs.size()-1));
                auto & fields = structs[index];
                for ( auto it = fields->begin(); it != fields->end(); ) {
                    auto & field = *it;
                    vis.preVisitMakeStructureField(this, index, field.get(), field==fields->back());
                    field->value = field->value->visit(vis);
                    if ( field ) {
                        field = vis.visitMakeStructureField(this, index, field.get(), field==fields->back());
                    }
                    if ( field ) ++it; else it = fields->erase(it);
                }
                vis.visitMakeStructureIndex(this, index, index==int(structs.size()-1));
            }
        }
        if ( block && vis.canVisitMakeStructureBlock(this, block.get()) ) {
            vis.preVisitMakeStructureBlock(this, block.get());
            block = block->visit(vis);
            if ( block ) block = vis.visitMakeStructureBlock(this, block.get());
        }
        return vis.visit(this);
    }

    // make variant

    ExpressionPtr ExprMakeVariant::clone( const ExpressionPtr & expr ) const {
        auto cexpr = clonePtr<ExprMakeVariant>(expr);
        ExprMakeLocal::clone(cexpr);
        cexpr->variants.reserve ( variants.size() );
        for ( auto & fd : variants ) {
            cexpr->variants.push_back(fd->clone());
        }
        cexpr->makeType = make_smart<TypeDecl>(*makeType);
        return cexpr;
    }

    ExpressionPtr ExprMakeVariant::visit(Visitor & vis) {
        vis.preVisit(this);
        if ( makeType ) {
            vis.preVisit(makeType.get());
            makeType = makeType->visit(vis);
            makeType = vis.visit(makeType.get());
        }
        int index = 0;
        for ( auto it = variants.begin(); it != variants.end(); index ++ ) {
            auto & field = *it;
            vis.preVisitMakeVariantField(this, index, field.get(), field==variants.back());
            field->value = field->value->visit(vis);
            if ( field ) {
                field = vis.visitMakeVariantField(this, index, field.get(), field==variants.back());
            }
            if ( field ) ++it; else it = variants.erase(it);
        }
        return vis.visit(this);
    }

    // make array

    ExpressionPtr ExprMakeArray::clone( const ExpressionPtr & expr ) const {
        auto cexpr = clonePtr<ExprMakeArray>(expr);
        ExprMakeLocal::clone(cexpr);
        cexpr->values.reserve ( values.size() );
        for ( auto & val : values ) {
            cexpr->values.push_back(val->clone());
        }
        cexpr->makeType = make_smart<TypeDecl>(*makeType);
        cexpr->gen2 = gen2;
        return cexpr;
    }

    ExpressionPtr ExprMakeArray::visit(Visitor & vis) {
        vis.preVisit(this);
        if ( makeType ) {
            vis.preVisit(makeType.get());
            makeType = makeType->visit(vis);
            makeType = vis.visit(makeType.get());
        }
        int index = 0;
        for ( auto it = values.begin(); it != values.end(); ) {
            auto & value = *it;
            vis.preVisitMakeArrayIndex(this, index, value.get(), index==int(values.size()-1));
            value = value->visit(vis);
            if ( value ) {
                value = vis.visitMakeArrayIndex(this, index, value.get(), index==int(values.size()-1));
            }
            if ( value ) ++it; else it = values.erase(it);
            index ++;
        }
        return vis.visit(this);
    }

    // make tuple

    ExpressionPtr ExprMakeTuple::clone( const ExpressionPtr & expr ) const {
        auto cexpr = clonePtr<ExprMakeTuple>(expr);
        ExprMakeLocal::clone(cexpr);
        cexpr->values.reserve ( values.size() );
        for ( auto & val : values ) {
            cexpr->values.push_back(val->clone());
        }
        if ( makeType ) {
            cexpr->makeType = make_smart<TypeDecl>(*makeType);
        }
        cexpr->isKeyValue = isKeyValue;
        return cexpr;
    }

    ExpressionPtr ExprMakeTuple::visit(Visitor & vis) {
        vis.preVisit(this);
        if ( makeType ) {
            vis.preVisit(makeType.get());
            makeType = makeType->visit(vis);
            makeType = vis.visit(makeType.get());
        }
        int index = 0;
        for ( auto it = values.begin(); it != values.end(); ) {
            auto & value = *it;
            vis.preVisitMakeTupleIndex(this, index, value.get(), index==int(values.size()-1));
            value = value->visit(vis);
            if ( value ) {
                value = vis.visitMakeTupleIndex(this, index, value.get(), index==int(values.size()-1));
            }
            if ( value ) ++it; else it = values.erase(it);
            index ++;
        }
        return vis.visit(this);
    }


    // array comprehension

    ExpressionPtr ExprArrayComprehension::clone( const ExpressionPtr & expr ) const {
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
        vis.preVisitArrayComprehensionSubexpr(this, subexpr.get());
        subexpr = subexpr->visit(vis);
        if ( exprWhere ) {
            vis.preVisitArrayComprehensionWhere(this, exprWhere.get());
            exprWhere = exprWhere->visit(vis);
        }
        return vis.visit(this);
    }

    // program

    vector<ReaderMacroPtr> Program::getReaderMacro ( const string & name ) const {
        vector<ReaderMacroPtr> macros;
        string moduleName, markupName;
        splitTypeName(name, moduleName, markupName);
        auto tmod = thisModule.get();
        auto modMacro = [&](Module * mod) -> bool {
            if ( thisModule->isVisibleDirectly(mod) && mod!=tmod ) {
                auto it = mod->readMacros.find(markupName);
                if ( it != mod->readMacros.end() ) {
                    macros.push_back(it->second);
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

    vector<TypeInfoMacroPtr> Program::findTypeInfoMacro ( const string & name ) const {
        return library.findTypeInfoMacro(name,thisModule.get());
    }

    vector<StructurePtr> Program::findStructure ( const string & name ) const {
        return library.findStructure(name,thisModule.get());
    }

    void Program::error ( const string & str, const string & extra, const string & fixme, const LineInfo & at, CompilationError cerr ) {
        errors.emplace_back(str,extra,fixme,at,cerr);
        failToCompile = true;
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
            auto annotation = static_pointer_cast<StructureTypeAnnotation>(ann->clone());
            annotation->name = st->name;
            string err;
            if ( annotation->create(st,arg,err) ) {
                return thisModule->addAnnotation(annotation, true);
            } else {
                error("can't create structure handle "+ann->name,err,"",st->at,CompilationError::invalid_annotation);
                return false;
            }
        } else {
            error("not a structure annotation "+ann->name,"","",st->at,CompilationError::invalid_annotation);
            return false;
        }
    }

    Program::Program() {
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
                at,CompilationError::type_not_found);
            return nullptr;
        } else if ( structs.size() ) {
            if ( structs.size()==1 ) {
                auto pTD = new TypeDecl(structs.back());
                pTD->at = at;
                return pTD;
            } else {
                string candidates = describeCandidates(structs);
                error("too many options for "+name,candidates,"",
                    at,CompilationError::structure_not_found);
                return nullptr;
            }
        } else if ( handles.size() ) {
            if ( handles.size()==1 ) {
                if ( handles.back()->rtti_isHandledTypeAnnotation() ) {
                    auto pTD = new TypeDecl(Type::tHandle);
                    pTD->annotation = static_cast<TypeAnnotation *>(handles.back().get());
                    pTD->at = at;
                    return pTD;
                } else {
                    error("not a handled type annotation "+name,"","",
                        at,CompilationError::handle_not_found);
                    return nullptr;
                }
            } else {
                string candidates = describeCandidates(handles);
                error("too many options for "+name, candidates, "",
                    at,CompilationError::handle_not_found);
                return nullptr;
            }
        } else if ( enums.size() ) {
            if ( enums.size()==1 ) {
                auto pTD = new TypeDecl(enums.back());
                pTD->enumType = enums.back().get();
                pTD->at = at;
                return pTD;
            } else {
                string candidates = describeCandidates(enums);
                error("too many options for "+name,candidates,"",
                    at,CompilationError::enumeration_not_found);
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
                    at,CompilationError::type_alias_not_found);
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
        library.foreach([&](Module * pm) -> bool {
            if ( auto pp = pm->findCall(funcName) ) {
                ptr.push_back(pp);
            }
            return true;
        }, moduleName);
        if ( ptr.size()==1 ) {
            return (*ptr.back())(at);
        } else if ( ptr.size()==0 ) {
            return new ExprCall(at,name);
        } else {
            error("too many options for " + name,"","", at, CompilationError::function_not_found);
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

    int64_t getConstExprIntOrUInt ( const ExpressionPtr & expr ) {
        DAS_ASSERTF ( expr && expr->rtti_isConstant(),
                     "expecting constant. something in enumeration (or otherwise) did not fold.");
        auto econst = static_pointer_cast<ExprConst>(expr);
        switch (econst->baseType) {
        case Type::tInt8:       return static_pointer_cast<ExprConstInt8>(expr)->getValue();
        case Type::tUInt8:      return static_pointer_cast<ExprConstUInt8>(expr)->getValue();
        case Type::tInt16:      return static_pointer_cast<ExprConstInt16>(expr)->getValue();
        case Type::tUInt16:     return static_pointer_cast<ExprConstUInt16>(expr)->getValue();
        case Type::tInt:        return static_pointer_cast<ExprConstInt>(expr)->getValue();
        case Type::tUInt:       return static_pointer_cast<ExprConstUInt>(expr)->getValue();
        case Type::tBitfield:   return static_pointer_cast<ExprConstBitfield>(expr)->getValue();
        case Type::tInt64:      return static_pointer_cast<ExprConstInt64>(expr)->getValue();
        case Type::tUInt64:     return static_pointer_cast<ExprConstUInt64>(expr)->getValue();
        default:
            DAS_ASSERTF ( 0,
                "we should not even be here. there is an enumeration of unsupported type."
                "something in enumeration (or otherwise) did not fold.");
            return 0;
        }
    }

    ExpressionPtr Program::makeConst ( const LineInfo & at, const TypeDeclPtr & type, vec4f value ) {
        if ( type->dim.size() || type->ref ) return nullptr;
        switch ( type->baseType ) {
            case Type::tBool:           return make_smart<ExprConstBool>(at, cast<bool>::to(value));
            case Type::tInt8:           return make_smart<ExprConstInt8>(at, cast<int8_t>::to(value));
            case Type::tInt16:          return make_smart<ExprConstInt16>(at, cast<int16_t>::to(value));
            case Type::tInt64:          return make_smart<ExprConstInt64>(at, cast<int64_t>::to(value));
            case Type::tInt:            return make_smart<ExprConstInt>(at, cast<int32_t>::to(value));
            case Type::tInt2:           return make_smart<ExprConstInt2>(at, cast<int2>::to(value));
            case Type::tInt3:           return make_smart<ExprConstInt3>(at, cast<int3>::to(value));
            case Type::tInt4:           return make_smart<ExprConstInt4>(at, cast<int4>::to(value));
            case Type::tUInt8:          return make_smart<ExprConstUInt8>(at, cast<uint8_t>::to(value));
            case Type::tUInt16:         return make_smart<ExprConstUInt16>(at, cast<uint16_t>::to(value));
            case Type::tUInt64:         return make_smart<ExprConstUInt64>(at, cast<uint64_t>::to(value));
            case Type::tUInt:           return make_smart<ExprConstUInt>(at, cast<uint32_t>::to(value));
            case Type::tBitfield:       return make_smart<ExprConstBitfield>(at, cast<uint32_t>::to(value));
            case Type::tUInt2:          return make_smart<ExprConstUInt2>(at, cast<uint2>::to(value));
            case Type::tUInt3:          return make_smart<ExprConstUInt3>(at, cast<uint3>::to(value));
            case Type::tUInt4:          return make_smart<ExprConstUInt4>(at, cast<uint4>::to(value));
            case Type::tFloat:          return make_smart<ExprConstFloat>(at, cast<float>::to(value));
            case Type::tFloat2:         return make_smart<ExprConstFloat2>(at, cast<float2>::to(value));
            case Type::tFloat3:         return make_smart<ExprConstFloat3>(at, cast<float3>::to(value));
            case Type::tFloat4:         return make_smart<ExprConstFloat4>(at, cast<float4>::to(value));
            case Type::tDouble:         return make_smart<ExprConstDouble>(at, cast<double>::to(value));
            case Type::tRange:          return make_smart<ExprConstRange>(at, cast<range>::to(value));
            case Type::tURange:         return make_smart<ExprConstURange>(at, cast<urange>::to(value));
            case Type::tRange64:        return make_smart<ExprConstRange64>(at, cast<range64>::to(value));
            case Type::tURange64:       return make_smart<ExprConstURange64>(at, cast<urange64>::to(value));
            default:                    DAS_ASSERTF(0, "we should not even be here"); return nullptr;
        }
    }

    StructurePtr Program::visitStructure(Visitor & vis, Structure * pst) {
        vis.preVisit(pst);
        for ( auto & fi : pst->fields ) {
            vis.preVisitStructureField(pst, fi, &fi==&pst->fields.back());
            if ( fi.type ) {
                vis.preVisit(fi.type.get());
                fi.type = fi.type->visit(vis);
                fi.type = vis.visit(fi.type.get());
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
            vis.preVisitEnumerationValue(penum, itf.name, itf.value.get(), count==total);
            if ( itf.value ) itf.value = itf.value->visit(vis);
            itf.value = vis.visitEnumerationValue(penum, itf.name, itf.value.get(), count==total);
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

    void Program::visit(Visitor & vis, bool visitGenerics ) {
        vis.preVisitProgram(this);
        visitModule(vis, thisModule.get(), visitGenerics);
        vis.visitProgram(this);
    }

    void Program::visitModule(Visitor & vis, Module * thatModule, bool visitGenerics) {
        vis.preVisitModule(thatModule);
        // enumerations
        thatModule->enumerations.foreach([&](auto & penum){
            if ( vis.canVisitEnumeration(penum.get()) ) {
                auto penumn = visitEnumeration(vis, penum.get());
                if ( penumn != penum ) {
                    thatModule->enumerations.replace(penum->name, penumn);
                    penum = penumn;
                }
            }
        });
        // structures
        thatModule->structures.foreach([&](auto & spst){
            Structure * pst = spst.get();
            if ( vis.canVisitStructure(pst) ) {
                StructurePtr pstn = visitStructure(vis, pst);
                if ( pstn.get() != pst ) {
                    thatModule->structures.replace(pst->name, pstn);
                    spst = pstn;
                }
            }
        });
        // aliases
        thatModule->aliasTypes.foreach([&](auto & alsv){
            vis.preVisitAlias(alsv.get(), alsv->alias);
            vis.preVisit(alsv.get());
            auto alssv = alsv->visit(vis);
            if ( alssv ) alssv = vis.visit(alssv.get());
            if ( alssv ) alsv = vis.visitAlias(alssv.get(), alssv->alias);
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
            if ( vis.canVisitGlobalVariable(var.get()) ) {
                vis.preVisitGlobalLet(var);
                if ( var->type ) {
                    vis.preVisit(var->type.get());
                    var->type = var->type->visit(vis);
                    var->type = vis.visit(var->type.get());
                }
                if ( var->init ) {
                    vis.preVisitGlobalLetInit(var, var->init.get());
                    var->init = var->init->visit(vis);
                    var->init = vis.visitGlobalLetInit(var, var->init.get());
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
                if ( !fn->builtIn ) {
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
            if ( !fn->builtIn ) {
                if ( vis.canVisitFunction(fn.get()) ) {
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
        return !policies.no_optimizations && options.getBoolOption("optimize",true);
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
        if (log) {
            logs << *this << "\n";
        }
        do {
            if ( log ) logs << "OPTIMIZE:\n"; if ( logPass ) logs << *this;
            any = false;
            last = optimizationRefFolding();    if ( failed() ) break;  any |= last;
            if ( log ) logs << "REF FOLDING: " << (last ? "optimized" : "nothing") << "\n"; if ( logPass ) logs << *this;
            last = optimizationUnused(logs);    if ( failed() ) break;  any |= last;
            if ( log ) logs << "REMOVE UNUSED:" << (last ? "optimized" : "nothing") << "\n"; if ( logPass ) logs << *this;
            last = optimizationConstFolding();  if ( failed() ) break;  any |= last;
            if ( log ) logs << "CONST FOLDING:" << (last ? "optimized" : "nothing") << "\n"; if ( logPass ) logs << *this;
            last = optimizationCondFolding();  if ( failed() ) break;  any |= last;
            if ( log ) logs << "COND FOLDING:" << (last ? "optimized" : "nothing") << "\n"; if ( logPass ) logs << *this;
            last = optimizationBlockFolding();  if ( failed() ) break;  any |= last;
            if ( log ) logs << "BLOCK FOLDING:" << (last ? "optimized" : "nothing") << "\n"; if ( logPass ) logs << *this;
            // this is here again for a reason
            last = optimizationUnused(logs);    if ( failed() ) break;  any |= last;
            if ( log ) logs << "REMOVE UNUSED:" << (last ? "optimized" : "nothing") << "\n"; if ( logPass ) logs << *this;
            // now, user macros
            last = false;
            auto modMacro = [&](Module * mod) -> bool {    // we run all macros for each module
                if ( thisModule->isVisibleDirectly(mod) && mod!=thisModule.get() ) {
                    for ( const auto & pm : mod->optimizationMacros ) {
                        last |= pm->apply(this, thisModule.get());
                        if ( failed() ) {                       // if macro failed, we report it, and we are done
                            error("optimization macro " + mod->name + "::" + pm->name + " failed", "","",LineInfo());
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
        } while ( any );
    }
}
