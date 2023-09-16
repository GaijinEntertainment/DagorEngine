#include "daScript/misc/platform.h"

#include "daScript/ast/ast.h"
#include "daScript/ast/ast_handle.h"

namespace das {

    void setParents ( Module * mod, const char * child, const std::initializer_list<const char *> & parents ) {
        auto chA = mod->findAnnotation(child);
        DAS_VERIFYF(chA,"missing child annotation");
        DAS_VERIFYF(chA->rtti_isBasicStructureAnnotation(),"expecting basic structure annotation");
        auto bsaCh = (BasicStructureAnnotation *) chA.get();
        for ( auto parent : parents ) {
            auto chP = mod->findAnnotation(parent);
            DAS_VERIFYF(chP,"missing parent annotation");
            bsaCh->parents.push_back((TypeAnnotation *)chP.get());
        }
    }


    bool BasicStructureAnnotation::canSubstitute(TypeAnnotation * ann) const {
        if ( this==ann ) return true;
        if ( this->module != ann->module ) return false;
        if ( ann->rtti_isBasicStructureAnnotation() ) {
            auto* AEA = static_cast<BasicStructureAnnotation*>(ann);
            for ( auto p : AEA->parents ) {
                if ( p == this ) return true;
            }
        }
        return false;
    }

    void BasicStructureAnnotation::seal( Module * m ) {
        TypeAnnotation::seal(m);
        mlib = nullptr;
    }

    int32_t BasicStructureAnnotation::getGcFlags(das_set<Structure *> & dep, das_set<Annotation *> & depA) const {
        int32_t gcf = 0;
        for ( auto & it : fields ) {
            auto & sfield = it.second;
            if ( sfield.constDecl ) gcf |= sfield.constDecl->gcFlags(dep,depA);
            if ( sfield.decl )      gcf |= sfield.decl->gcFlags(dep,depA);
        }
        return gcf;
    }

    TypeInfo * BasicStructureAnnotation::getFieldType ( const string & na ) const {
        updateTypeInfo();
        for ( uint32_t n=0; n!=sti->count; ++n ) {
            auto & fi = sti->fields[n];
            if ( fi->name == na ) {
                return fi;
            }
        }
        return nullptr;
    }

    uint32_t BasicStructureAnnotation::getFieldOffset ( const string & na ) const {
        auto it = fields.find(na);
        if ( it!=fields.end() ) {
            return it->second.offset;
        } else {
            return -1U;
        }
    }

    TypeDeclPtr BasicStructureAnnotation::makeFieldType ( const string & na, bool isConst ) const {
        auto it = fields.find(na);
        if ( it!=fields.end() ) {
            auto & sfield = it->second;
            auto t = isConst && sfield.constDecl ? make_smart<TypeDecl>(*sfield.constDecl) :  make_smart<TypeDecl>(*sfield.decl);
            if ( sfield.offset != -1U ) {
                t->ref = true;
            }
            return t;
        } else {
            return nullptr;
        }
    }

    TypeDeclPtr BasicStructureAnnotation::makeSafeFieldType ( const string & na, bool isConst ) const {
        auto it = fields.find(na);
        if ( it!=fields.end() ) {
            auto & sfield = it->second;
            if ( sfield.offset!=-1U ) {
                return isConst && sfield.constDecl ? make_smart<TypeDecl>(*sfield.constDecl) : make_smart<TypeDecl>(*sfield.decl);
            } else {
                return nullptr;
            }
        } else {
            return nullptr;
        }
    }

    void BasicStructureAnnotation::aotPreVisitGetField ( TextWriter & ss, const string & fieldName ) {
        auto it = fields.find(fieldName);
        if (it != fields.end()) {
            ss << it->second.aotPrefix;
        }
    }

    void BasicStructureAnnotation::aotPreVisitGetFieldPtr ( TextWriter & ss, const string & fieldName ) {
        auto it = fields.find(fieldName);
        if (it != fields.end()) {
            ss << it->second.aotPrefix;
        }
    }

    void BasicStructureAnnotation::aotVisitGetField ( TextWriter & ss, const string & fieldName ) {
        auto it = fields.find(fieldName);
        if (it != fields.end()) {
            ss << "." << it->second.cppName << it->second.aotPostfix;
        } else {
            ss << "." << fieldName << " /*undefined */";
        }
    }

    void BasicStructureAnnotation::aotVisitGetFieldPtr ( TextWriter & ss, const string & fieldName ) {
        auto it = fields.find(fieldName);
        if (it != fields.end()) {
            ss << "->" << it->second.cppName << it->second.aotPostfix;
        } else {
            ss << "->" << fieldName << " /*undefined */";
        }
    }

    BasicStructureAnnotation::StructureField & BasicStructureAnnotation::addFieldEx ( const string & na, const string & cppNa, off_t offset, TypeDeclPtr pT ) {
        auto & field = fields[na];
        if ( field.decl ) {
            DAS_FATAL_ERROR("structure field %s already exist in structure %s\n", na.c_str(), name.c_str() );
        }
        fieldsInOrder.push_back(na);
        field.cppName = cppNa;
        field.decl = pT;
        field.offset = offset;
        return field;
    }

    void BasicStructureAnnotation::updateTypeInfo() const {
        lock_guard<recursive_mutex> guard(walkMutex);
        if ( !sti ) {
            auto debugInfo = helpA.debugInfo;
            sti = debugInfo->template makeNode<StructInfo>();
            sti->name = debugInfo->allocateName(name);
            // flags
            sti->flags = 0;
            // count fields
            sti->count = 0;
            for ( auto & fi : fields ) {
                auto & var = fi.second;
                if ( var.offset != -1U ) {
                    sti->count ++;
                }
            }
            // and allocate
            sti->size = (uint32_t) getSizeOf();
            sti->fields = (VarInfo **) debugInfo->allocate( sizeof(VarInfo *) * sti->count );
            int i = 0;
            for ( const auto & fn : fieldsInOrder ) {
                auto itvar = fields.find(fn);
                if ( itvar != fields.end() ) {
                    auto & var = itvar->second;
                    if ( var.offset != -1U ) {
                        VarInfo * vi = debugInfo->template makeNode<VarInfo>();
                        helpA.makeTypeInfo(vi, var.decl);
                        vi->name = debugInfo->allocateName(fn);
                        vi->offset = var.offset;
                        sti->fields[i++] = vi;
                    }
                }
            }
        }

    }

    void BasicStructureAnnotation::walk ( DataWalker & walker, void * data ) {
        updateTypeInfo();
        walker.walk_struct((char *)data, sti);
    }

    void BasicStructureAnnotation::from ( BasicStructureAnnotation * ann ) {
        parents.reserve(ann->parents.size() + 1);
        parents.push_back(ann);
        for (auto pp : ann->parents) {
            parents.push_back(pp);
        }
    }

    void BasicStructureAnnotation::from ( const char * parentName ) {
        from((BasicStructureAnnotation*)(this->module->findAnnotation(parentName).get()));
    }

    void Program::validateAotCpp ( TextWriter & logs, Context & ) {
        library.foreach([&](Module * mod) -> bool {
            if ( mod->builtIn ) {
                logs << "// validating " << mod->name << "\n";
                mod->handleTypes.foreach([&](auto tp){
                    if ( tp->rtti_isBasicStructureAnnotation() ) {
                        auto bs = static_pointer_cast<BasicStructureAnnotation>(tp);
                        if ( !bs->validationNeverFails ) {
                            auto cppt = make_smart<TypeDecl>(Type::tHandle);
                            cppt->annotation = bs.get();
                            auto cppn = describeCppType(cppt);
                            logs << "//\t" << cppn << " aka " << tp->name << "\n";
                            for ( const auto & flp : bs->fields ) {
                                const auto & fld = flp.second;
                                if ( fld.offset != -1u ) {
                                    if ( fld.cppName.find('(')==string::npos ) {   // sometimes we bind ref member function as if field
                                        logs << "\t\tstatic_assert(offsetof(" << cppn << ","
                                            << fld.cppName << ")==" << fld.offset << ",\"mismatching offset\");\n";
                                    }
                                }
                            }
                        }
                    }
                });
                mod->enumerations.foreach([&](auto tp){
                    auto cppt = make_smart<TypeDecl>(tp);
                    auto cppn = describeCppType(cppt);
                    auto baset = tp->makeBaseType();
                    logs << "//\t" << cppn << " aka " << tp->name << "\n";
                    logs << "\t\tstatic_assert( is_same < underlying_type< " << cppn << " >::type, "
                        << describeCppType(baset) << ">::value,\"mismatching underlying type, expecting "
                            << das_to_string(tp->baseType) << "\");\n";
                });
            }
            return true;
        },"*");
    }
}
