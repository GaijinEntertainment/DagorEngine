#include "daScript/misc/platform.h"

#include "daScript/ast/ast_visitor.h"
#include "daScript/ast/ast.h"

#include <inttypes.h>

namespace das
{
    TypeDeclPtr makeHandleType(const ModuleLibrary & library, const char * typeName) {
        return library.makeHandleType(typeName);
    }

    Annotation * TypeDecl::isPointerToAnnotation() const {
        if ( baseType!=Type::tPointer || !firstType || firstType->baseType!=Type::tHandle ) {
            return nullptr;
        } else {
            return firstType->annotation;
        }
    }

    // auto or generic type conversion

    TypeDecl::TypeDecl(const EnumerationPtr & ep)
        : baseType(ep->getEnumType()), enumType(ep.get())
    {
    }

    bool TypeDecl::isClass() const { // CANT BE INLINED DUE TO STRUCT TYPE
        return isStructure() && structType && structType->isClass;
    }

    bool TypeDecl::isExprTypeAnywhere() const {
        das_set<Structure*> dep;
        return isExprTypeAnywhere(dep);
    }

    bool TypeDecl::isExprTypeAnywhere(das_set<Structure*> & dep) const {
        for ( auto di : dim ) {
            if ( di==TypeDecl::dimConst ) {
                return true;
            }
        }
        if ( baseType==Type::tStructure ) {
            if (structType) {
                if (dep.find(structType) != dep.end()) return false;
                dep.insert(structType);
                return structType->isExprTypeAnywhere(dep);
            }
        }
        if ( firstType && firstType->isExprTypeAnywhere(dep) ) return true;
        if ( secondType && secondType->isExprTypeAnywhere(dep) ) return true;
        for ( auto & arg : argTypes ) {
            if ( arg->isExprTypeAnywhere(dep) ) {
                return true;
            }
        }
        return false;
    }

    bool TypeDecl::isExprType() const {
        for ( auto di : dim ) {
            if ( di==TypeDecl::dimConst ) {
                return true;
            }
        }
        if ( firstType && firstType->isExprType() ) return true;
        if ( secondType && secondType->isExprType() ) return true;
        for ( auto & arg : argTypes ) {
            if ( arg->isExprType() ) {
                return true;
            }
        }
        return false;
    }

    TypeDeclPtr TypeDecl::visit ( Visitor & vis ) {
        for ( size_t i=0, is=dim.size(); i!=is; ++i ) {
            if ( dim[i]==TypeDecl::dimConst ) {
                if ( dimExpr[i] ) {
                    dimExpr[i] = dimExpr[i]->visit(vis);
                }
            }
        }
        if ( firstType ) {
            vis.preVisit(firstType.get());
            firstType = firstType->visit(vis);
        }
        if ( secondType ) {
            vis.preVisit(secondType.get());
            secondType = secondType->visit(vis);
        }
        for ( auto & argType : argTypes ) {
            vis.preVisit(argType.get());
            argType = argType->visit(vis);
        }
        return this;
    }

    void TypeDecl::applyRefToRef ( const TypeDeclPtr & TT, bool topLevel ) {
        if ( topLevel && TT->ref && TT->isRefType() ) {
            TT->ref = false;
        }
        if ( TT->isPointer() ) {
            if ( TT->firstType ) {
                applyRefToRef(TT->firstType);
            }
        } else if ( TT->baseType==Type::tIterator ) {
            applyRefToRef(TT->firstType);
        } else if ( TT->baseType==Type::tArray ) {
            applyRefToRef(TT->firstType);
        } else if ( TT->baseType==Type::tTable ) {
            applyRefToRef(TT->firstType);
            applyRefToRef(TT->secondType);
        } else if ( TT->baseType==Type::tBlock || TT->baseType==Type::tFunction || TT->baseType==Type::tLambda ) {
            if ( TT->firstType ) {
                applyRefToRef(TT->firstType);
            }
            for ( auto & arg : TT->argTypes ) {
                applyRefToRef(arg, true);
            }
        } else if ( TT->baseType==Type::tTuple || TT->baseType==Type::tVariant ) {
            for ( auto & arg : TT->argTypes ) {
                applyRefToRef(arg);
            }
        }
    }

    void TypeDecl::applyAutoContracts ( const TypeDeclPtr & TT, const TypeDeclPtr & autoT ) {
        if ( !autoT->isAuto() ) return;
        TT->ref = (TT->ref || autoT->ref) && !autoT->removeRef && !TT->removeRef;
        TT->constant = (TT->constant || autoT->constant) && !autoT->removeConstant && !TT->removeConstant;
        TT->temporary = (TT->temporary || autoT->temporary) && !autoT->removeTemporary && !TT->removeTemporary;
        if ( (autoT->removeDim || TT->removeDim) && TT->dim.size() ) TT->dim.erase(TT->dim.begin());
        TT->removeConstant = false;
        TT->removeDim = false;
        TT->removeRef = false;
        if ( autoT->isPointer() ) {
            if ( TT->firstType ) {
                applyAutoContracts(TT->firstType, autoT->firstType);
            }
        } else if ( autoT->baseType==Type::tIterator ) {
            applyAutoContracts(TT->firstType, autoT->firstType);
        } else if ( autoT->baseType==Type::tArray ) {
            applyAutoContracts(TT->firstType, autoT->firstType);
        } else if ( autoT->baseType==Type::tTable ) {
            applyAutoContracts(TT->firstType, autoT->firstType);
            applyAutoContracts(TT->secondType, autoT->secondType);
        } else if ( autoT->baseType==Type::tBlock || autoT->baseType==Type::tFunction ||
                   autoT->baseType==Type::tLambda || autoT->baseType==Type::tTuple ||
                   autoT->baseType==Type::tVariant ) {
            if ( TT->firstType ) {
                applyAutoContracts(TT->firstType, autoT->firstType);
            }
            for ( size_t i=0, is=autoT->argTypes.size(); i!=is; ++i ) {
                applyAutoContracts(TT->argTypes[i], autoT->argTypes[i]);
            }
        }
    }

    void TypeDecl::updateAliasMap ( const TypeDeclPtr & decl, const TypeDeclPtr & pass, AliasMap & aliases, OptionsMap & options ) {
        if ( !pass ) {
            return;
        } else if ( decl->baseType==Type::option ) {
            auto it = options.find(decl.get());
            DAS_VERIFYF(it!=options.end(),"internal error, option not found for the optional type");
            return updateAliasMap(decl->argTypes[it->second], pass, aliases, options);
        } else if ( decl->baseType==Type::autoinfer ) {
            if ( !decl->alias.empty() && aliases.find(decl->alias)==aliases.end() ) {
                auto TT = make_smart<TypeDecl>(*pass);
                TT->alias = decl->alias;
                TT->dim.clear();
                TT->constant = false;
                TT->temporary = false;
                TT->ref = false;
                aliases[decl->alias] = TT;
            }
        } else if ( pass->baseType==Type::autoinfer ) {
            if ( !pass->alias.empty() && aliases.find(pass->alias)==aliases.end() ) {
                auto TT = make_smart<TypeDecl>(*decl);
                TT->alias = pass->alias;
                TT->dim.clear();
                TT->constant = false;
                TT->temporary = false;
                TT->ref = false;
                aliases[pass->alias] = TT;
            }
        } else {
            if ( decl->firstType ) updateAliasMap(decl->firstType, pass->firstType, aliases, options);
            if ( decl->secondType ) updateAliasMap(decl->secondType, pass->secondType, aliases, options);
            for ( size_t iA=0, iAs = decl->argTypes.size(); iA!=iAs; ++iA ) {
                updateAliasMap(decl->argTypes[iA], pass->argTypes[iA], aliases, options);
            }
        }
    }

    TypeDeclPtr TypeDecl::inferGenericInitType ( const TypeDeclPtr & autoT, const TypeDeclPtr & initT ) {
        if ( autoT->ref ) {
            autoT->ref = false;
            auto resT = inferGenericType(autoT, initT, true, false, nullptr);
            if ( resT ) resT->ref = true;
            autoT->ref = true;
            return resT;
        } else {
            return inferGenericType(autoT, initT, true, false, nullptr);
        }
    }

    TypeDeclPtr TypeDecl::inferGenericType ( const TypeDeclPtr & autoT, const TypeDeclPtr & initT, bool topLevel, bool passType, OptionsMap * options ) {
        // for option type, we go through all the options in order. first matching is good
        if ( autoT->baseType==Type::option ) {
            for ( size_t i=0, is=autoT->argTypes.size(); i!=is; ++i ) {
                // we copy type qualifiers for each option
                auto & TT = autoT->argTypes[i];
                TT->ref = TT->ref || autoT->ref;
                TT->constant = TT->constant || autoT->constant;
                TT->temporary = TT->temporary || autoT->temporary;
                TT->removeConstant = TT->removeConstant || autoT->removeConstant;
                TT->removeDim = TT->removeDim || autoT->removeDim;
                TT->removeRef = TT->removeRef || autoT->removeRef;
                TT->explicitConst = TT->explicitConst || autoT->explicitConst;
                TT->explicitRef = TT->explicitRef || autoT->explicitRef;
                TT->implicit = TT->implicit || autoT->implicit;
                // now we infer type
                if ( auto resT = inferGenericType(TT,initT,topLevel,passType,options) ) {
                    if ( options!=nullptr ) (*options)[autoT.get()] = int(i);
                    return resT;
                }
            }
            return nullptr;
        }
        // explicit const mast match
        if ( autoT->explicitConst && (autoT->constant != initT->constant) ) {
            return nullptr;
        }
        // explicit ref match
        if ( autoT->explicitRef && (autoT->ref != initT->ref) ) {
            return nullptr;
        }
        // can't infer from the type, which is already 'auto'
        if ( initT->isAuto() ) {
            if (!autoT->isAuto()) {
                return make_smart<TypeDecl>(*autoT);
            } else if (autoT->baseType == Type::tBlock || autoT->baseType == Type::tFunction
                || autoT->baseType == Type::tLambda || autoT->baseType == Type::tTuple
                || autoT->baseType == Type::tVariant ) {
                /* two-sided infer */
            } else {
                /*
                if ( aliases && !autoT->alias.empty() ) {
                    auto it = aliases->find(autoT->alias);
                    if ( it != aliases->end() ) {
                        return make_smart<TypeDecl>(*it->second);
                    }
                }
                */
                return nullptr;
            }
        }
        // if its not an auto type, return as is
        if ( !autoT->isAuto() ) {
            auto rm = RefMatters::yes;
            auto cm = ConstMatters::yes;
            if ( topLevel ) {
                if ( (!autoT->ref && !autoT->explicitRef) || autoT->isRefType()  ) {
                    rm = RefMatters::no;
                }
                if ( autoT->constant && !autoT->explicitConst ) {
                    cm = ConstMatters::no;
                }
            }
            if ( autoT->isSameType(*initT, rm,cm, TemporaryMatters::yes, AllowSubstitute::yes, true, passType ) ) {
                return make_smart<TypeDecl>(*autoT);
            } else {
                return nullptr;
            }
        } else if ( autoT->baseType != Type::autoinfer ) {
            // if we are matching to strong type, with dim only, then it has to match
            if ( autoT->baseType != initT->baseType ) {
                return nullptr;
            }
        }
        // auto & can't be inferred from non-ref
        if ( autoT->ref && !initT->isRef() )
            return nullptr;
        // auto[][][] can't be inferred from non-array
        if ( autoT->dim.size() ) {
            if ( autoT->dim.size()!=initT->dim.size() )
                return nullptr;
            for ( size_t di=0, dis=autoT->dim.size(); di!=dis; ++di ) {
                int32_t aDI = autoT->dim[di];
                int32_t iDI = initT->dim[di];
                if ( aDI!=TypeDecl::dimAuto && aDI!=iDI ) {
                    return nullptr;
                }
            }
        }
        // non-implicit temp can't be inferred from non-temp, and non-temp from temp
        if ( autoT->baseType!=autoinfer && !autoT->implicit && !initT->implicit ) {
            if ( autoT->temporary != initT->temporary )
                return nullptr;
        }
        // auto? can't be inferred from non-pointer
        if ( autoT->isPointer() && (!initT->isPointer() || !initT->firstType) )
            return nullptr;
        // array has to match array
        if ( autoT->baseType==Type::tArray && (initT->baseType!=Type::tArray || !initT->firstType) )
            return nullptr;
        // table has to match table
        if ( autoT->baseType==Type::tTable && (initT->baseType!=Type::tTable || !initT->firstType || !initT->secondType) )
            return nullptr;
        // block has to match block, function to function, lambda to lambda
        if ( autoT->baseType==Type::tBlock || autoT->baseType==Type::tFunction
            || autoT->baseType==Type::tLambda || autoT->baseType==Type::tTuple
            || autoT->baseType==Type::tVariant ) {
            if ( initT->baseType!=autoT->baseType )
                return nullptr;
            if ( (autoT->firstType!=nullptr) != (initT->firstType!=nullptr) )   // both do or don't have return type
                return nullptr;
            if ( autoT->argTypes.size() != initT->argTypes.size() )             // both have same number of arguments
                return nullptr;
        }
        // now, lets make the type
        auto TT = make_smart<TypeDecl>(*initT);
        TT->at = autoT->at;
        TT->aotAlias = autoT->aotAlias;
        TT->alias = autoT->alias;
        TT->removeRef = false;
        TT->removeConstant = false;
        TT->removeTemporary = false;
        TT->removeDim = false;
        TT->implicit |= autoT->implicit;
        TT->explicitConst |= autoT->explicitConst;
        TT->explicitRef |= autoT->explicitRef;
        if ( autoT->isPointer() ) {
            // if it's a pointer, infer pointer-to separately
            TT->firstType = inferGenericType(autoT->firstType, initT->firstType, false, false, options);
            if ( !TT->firstType ) return nullptr;
        } else if ( autoT->baseType==Type::tIterator ) {
            // if it's a iterator, infer iterator-ofo separately
            TT->firstType = inferGenericType(autoT->firstType, initT->firstType, false, false, options);
            if ( !TT->firstType ) return nullptr;
        } else if ( autoT->baseType==Type::tArray ) {
            // if it's an array, infer array type separately
            TT->firstType = inferGenericType(autoT->firstType, initT->firstType, false, false, options);
            if ( !TT->firstType ) return nullptr;
        } else if ( autoT->baseType==Type::tTable ) {
            // if it's a table, infer table keys and values types separately
            TT->firstType = inferGenericType(autoT->firstType, initT->firstType, false, false, options);
            if ( !TT->firstType ) return nullptr;
            if ( !TT->firstType->isWorkhorseType() ) return nullptr;            // table key has to be hashable too
            TT->secondType = inferGenericType(autoT->secondType, initT->secondType, false, false, options);
            if ( !TT->secondType ) return nullptr;
        } else if ( autoT->baseType==Type::tBlock || autoT->baseType==Type::tFunction
                   || autoT->baseType==Type::tLambda || autoT->baseType==Type::tTuple
                   || autoT->baseType==Type::tVariant ) {
            // if it's a block or function, infer argument and return types
            if ( autoT->firstType ) {
                TT->firstType = inferGenericType(autoT->firstType, initT->firstType, false, false, options);
                if ( !TT->firstType ) return nullptr;
            }
            for ( size_t i=0, is=autoT->argTypes.size(); i!=is; ++i ) {
                TT->argTypes[i] = inferGenericType(autoT->argTypes[i], initT->argTypes[i], false, false, options);
                if ( !TT->argTypes[i] ) return nullptr;
            }
            if ( TT->argNames.size()==0 && !autoT->argNames.empty() ) {
                TT->argNames = autoT->argNames;
            }
        }
        return TT;
    }


    // TypeDecl

    string TypeDecl::describe ( DescribeExtra extra, DescribeContracts contracts, DescribeModule dmodule ) const {
        TextWriter stream;
        if ( baseType==Type::alias ) {
            if ( isTag ) {
                if ( firstType) {
                    stream << "$$(";
                    if ( firstType->dimExpr.size()==1 ) {
                        stream << *(firstType->dimExpr[0]);
                    }
                    stream << ")";
                } else {
                    stream << "$$()";
                }
            } else {
                stream << alias;
            }
        } else if ( baseType==Type::autoinfer ) {
            stream << "auto";
            if ( !alias.empty() ) {
                stream << "(" << alias << ")";
            }
        } else if ( baseType==Type::option ) {
            for ( auto & argT : argTypes ) {
                stream << argT->describe(extra);
                if ( argT != argTypes.back() ) {
                    stream << "|";
                }
            }
        } else if ( baseType==Type::tHandle ) {
            if ( annotation ) {
                if (dmodule == DescribeModule::yes && annotation->module && !annotation->module->name.empty()) {
                    stream << annotation->module->name << "::";
                }
                stream << annotation->name;
            } else {
                stream << "unspecified annotation";
            }
        } else if ( baseType==Type::tArray ) {
            if ( firstType ) {
                stream << "array<" << firstType->describe(extra) << ">";
            } else {
                stream << "array";
            }
        } else if ( baseType==Type::tTable ) {
            if ( firstType && secondType ) {
                stream << "table<" << firstType->describe(extra) << ";" << secondType->describe(extra) << ">";
            } else {
                stream << "table";
            }
        } else if ( baseType==Type::tStructure ) {
            if ( structType ) {
                if (dmodule == DescribeModule::yes && structType->module && !structType->module->name.empty()) {
                    stream << structType->module->name << "::";
                }
                stream << structType->name;
            } else {
                stream << "unspecified structure";
            }
        } else if ( baseType==Type::tPointer ) {
            if ( smartPtr ) stream << "smart_ptr<";
            if ( firstType ) {
                stream << firstType->describe(extra);
            } else {
                stream << "void";
            }
            stream << (smartPtr ? ">" : "?");
        } else if ( isEnumT() ) {
            if ( enumType ) {
                if (dmodule == DescribeModule::yes && enumType->module && !enumType->module->name.empty()) {
                    stream << enumType->module->name << "::";
                }
                stream << enumType->describe();
            } else {
                stream << "unspecified enumeration";
            }
        } else if ( baseType==Type::tIterator ) {
            if ( firstType ) {
                stream << "iterator<" << firstType->describe(extra) << ">";
            } else {
                stream << "iterator";
            }
        } else if ( baseType==Type::tBlock || baseType==Type::tFunction || baseType==Type::tLambda ) {
            stream << das_to_string(baseType) << "<";
            if ( argTypes.size() ) {
                stream << "(";
                for ( size_t ai=0, ais=argTypes.size(); ai!=ais; ++ ai ) {
                    if ( ai!=0 ) stream << ";";
                    if (!argTypes[ai]->isConst()) {
                        stream << "var ";
                    }
                    if ( argNames.size()==argTypes.size() ) {
                        stream << argNames[ai] << ":";
                    } else {
                        stream << "arg" << ai << ":";
                    }
                    stream << argTypes[ai]->describe(extra);
                }
                stream << ")";
            }
            if ( firstType ) {
                if ( argTypes.size() ) {
                    stream << ":";
                }
                stream << firstType->describe(extra);
            }
            stream << ">";
            if ( argNames.size() && argNames.size()!=argTypes.size() ) {
                stream << " DAS_COMMENT(malformed) ";
            }
        } else if ( baseType==Type::tTuple ) {
            stream << das_to_string(baseType) << "<";
            if ( argTypes.size() ) {
                int ai = 0;
                for ( const auto & arg : argTypes ) {
                    if ( argNames.size()==argTypes.size() ) {
                        const auto & argName = argNames[ai];
                        if ( !argName.empty() ) stream << argName << ":";
                    }
                    stream << arg->describe(extra);
                    if ( arg != argTypes.back() ) {
                        stream << ";";
                    }
                    ai ++;
                }
            }
            stream << ">";
        } else if ( baseType==Type::tVariant ) {
            stream << das_to_string(baseType) << "<";
            if ( argTypes.size() ) {
                int ai = 0;
                for ( const auto & arg : argTypes ) {
                    if ( argNames.size()==argTypes.size() ) {
                        const auto & argName = argNames[ai];
                        if ( !argName.empty() ) stream << argName << ":";
                    }
                    stream << arg->describe(extra);
                    if ( arg != argTypes.back() ) {
                        stream << ";";
                    }
                    ai ++;
                }
            }
            stream << ">";
        } else if ( baseType==Type::tBitfield ) {
            stream << das_to_string(baseType);
            if ( argNames.size() ) {
                stream << "<";
                int ai = 0;
                for ( const auto & arg : argNames ) {
                    if ( ai !=0 ) stream << ";";
                    stream << arg;
                    ai ++;
                }
                stream << ">";
            }
        }else {
            stream << das_to_string(baseType);
        }
        if ( extra==DescribeExtra::yes && baseType!=Type::autoinfer && baseType!=Type::alias && !alias.empty() ) {
            stream << " aka " << alias;
        }
        if ( constant ) {
            stream << " const";
        }
        for ( auto d : dim ) {
            if ( d==-1 ) {
                stream << "[]";
            } else {
                stream << "[" << d << "]";
            }
        }
        if ( ref ) {
            stream << "&";
        }
        if ( temporary ) {
            stream << "#";
        }
        if ( implicit ) {
            stream << " implicit";
        }
        if ( contracts==DescribeContracts::yes && isExplicit ) {
            stream << " explicit";
        }
        if ( explicitConst ) {
            stream << " ==const";
        }
        if ( explicitRef ) {
            stream << " ==&";
        }
        if (contracts == DescribeContracts::yes) {
            if (removeConstant || removeRef || removeDim || removeTemporary) {
                if (removeConstant) {
                    stream << " -const";
                }
                if (removeRef) {
                    stream << " -&";
                }
                if (removeDim ) {
                    stream << " -[]";
                }
                if (removeTemporary) {
                    stream << " -#";
                }
            }
        }
        return stream.str();
    }

    TextWriter& operator<< (TextWriter& stream, const TypeDecl & decl) {
        stream << decl.describe();
        return stream;
    }

    TypeDecl::TypeDecl(const TypeDecl & decl) {
        baseType = decl.baseType;
        structType = decl.structType;
        enumType = decl.enumType;
        annotation = decl.annotation;
        dim = decl.dim;
        dimExpr.reserve(decl.dimExpr.size());
        for ( auto & de : decl.dimExpr ) {
            if ( de ) {
                dimExpr.push_back(de->clone());
            } else {
                dimExpr.push_back(nullptr);
            }
        }
        flags = decl.flags;
        alias = decl.alias;
        at = decl.at;
        module = decl.module;
        if ( decl.firstType )
            firstType = make_smart<TypeDecl>(*decl.firstType);
        if ( decl.secondType )
            secondType = make_smart<TypeDecl>(*decl.secondType);
        for ( const auto & arg : decl.argTypes ) {
            argTypes.push_back(make_smart<TypeDecl>(*arg) );
        }
        argNames = decl.argNames;
    }

    TypeDecl * TypeDecl::findAlias ( const string & name, bool allowAuto ) {
        if (baseType == Type::alias) {
            return nullptr; // if it is another alias, can't find it
        } else if (baseType == Type::autoinfer && !allowAuto) {
            return nullptr; // if it has not been inferred yet, can't find it
        }
        else if (alias == name) {
            return this;
        }
        if ( baseType==Type::tPointer ) {
            return firstType ? firstType->findAlias(name,allowAuto) : nullptr;
        } else if ( baseType==Type::tIterator ) {
            return firstType ? firstType->findAlias(name,allowAuto) : nullptr;
        } else if ( baseType==Type::tArray ) {
            return firstType ? firstType->findAlias(name,allowAuto) : nullptr;
        } else if ( baseType==Type::tTable ) {
            if ( firstType ) {
                if ( auto k = firstType->findAlias(name,allowAuto) ) {
                    return k;
                }
            }
            return secondType ? secondType->findAlias(name,allowAuto) : nullptr;
        } else if ( baseType==Type::tBlock || baseType==Type::tFunction
                   || baseType==Type::tLambda || baseType==Type::tTuple
                   || baseType==Type::tVariant || baseType==Type::option ) {
            for ( auto & arg : argTypes ) {
                if ( auto att = arg->findAlias(name,allowAuto) ) {
                    return att;
                }
            }
            return firstType ? firstType->findAlias(name,allowAuto) : nullptr;
        } else {
            return nullptr;
        }
    }

    bool TypeDecl::canDelete() const {
        if ( baseType==Type::tHandle ) {
            return annotation->canDelete();
        } else if ( baseType==Type::tPointer ) {
            if ( !firstType ) {
                return false;
            } else if (firstType->baseType==Type::tHandle ) {
                return firstType->annotation->canDeletePtr();
            } else if ( firstType->baseType==Type::tStructure ) {
                return true;
            } else if ( firstType->baseType==Type::tTuple ) {
                return true;
            } else if ( firstType->baseType==Type::tVariant ) {
                return true;
            } else {
                return false;
            }
        } else if ( baseType==Type::tArray || baseType==Type::tTable ) {
            return true;
        } else if ( baseType==Type::tString ) {
            return false;
        } else if ( baseType==Type::tStructure ) {
            return true;
        } else if ( baseType==Type::tLambda ) {
            return true;
        } else if ( baseType==Type::tIterator ) {
            return true;
        } else if ( baseType==Type::tTuple || baseType==Type::tVariant ) {
            return true;
        } else {
            return false;
        }
    }

    bool TypeDecl::canDeletePtr() const {
        if ( baseType==Type::tHandle ) {
            return annotation->canDeletePtr();
        } else if ( baseType==Type::tStructure ) {
            return true;
        } else if ( baseType==Type::tTuple || baseType==Type::tVariant ) {
            return true;
        } else {
            return false;
        }
    }

    bool TypeDecl::canNew() const {
        if ( baseType==Type::tHandle ) {
            return annotation->canNew();
        } else if ( baseType==Type::tStructure ) {
            return true;
        } else if ( baseType==Type::tTuple || baseType==Type::tVariant ) {
            return true;
        } else {
            return false;
        }
    }

    bool TypeDecl::needDelete() const {
        if ( baseType==Type::tHandle ) {
            return annotation->needDelete();
        } else if ( baseType==Type::tPointer ) {
            return canDelete();
        } else if ( baseType==Type::tArray || baseType==Type::tTable ) {
            return true;
        } else if ( baseType==Type::tString ) {
            return false;
        } else if ( baseType==Type::tStructure ) {
            return true;
        } else if ( baseType==Type::tLambda ) {
            return true;
        } else if ( baseType==Type::tIterator ) {
            return true;
        } else if ( baseType==Type::tTuple || baseType==Type::tVariant ) {
            return true;
        } else {
            return false;
        }
    }

    bool TypeDecl::canAot() const {
        das_set<Structure *> recAot;
        return canAot(recAot);
    }

    bool TypeDecl::canAot( das_set<Structure *> & recAot) const {
        if ( annotation && !annotation->canAot(recAot) ) return false;
        if ( firstType && !firstType->canAot(recAot) ) return false;
        if ( secondType && !secondType->canAot(recAot) ) return false;
        for ( auto & arg : argTypes ) {
            if ( !arg->canAot(recAot) ) return false;
        }
        if ( structType ) {
            if ( recAot.find(structType)==recAot.end() ) {
                recAot.insert(structType);
                return structType->canAot(recAot);
            }
        }
        return true;
    }

    bool TypeDecl::canMove() const {
        if (baseType == Type::tHandle) {
            return annotation->canMove();
        } else if (baseType == Type::tBlock) {
            return false;
        } else if (baseType == Type::tStructure && structType) {
            return !structType->circular ? structType->canMove() : false;
        } else if (baseType == Type::tTuple || baseType == Type::tVariant || baseType == Type::option ) {
            for (const auto & arg : argTypes) {
                if (!arg->canMove()) return false;
            }
            return true;
        } else {
            return true;
        }
    }

    bool TypeDecl::canClone() const {
        if (baseType == Type::tHandle) {
            return annotation->canClone();
        } else if (baseType == Type::tStructure && structType) {
            return !structType->circular ? structType->canClone() : false;
        } else if (baseType == Type::tTuple || baseType == Type::tVariant  || baseType == Type::option) {
            for (const auto & arg : argTypes) {
                if (!arg->canClone()) return false;
            }
            return true;
        } else if (baseType == Type::tBlock) {
            return false;
        } else if (baseType == Type::tIterator) {
            return false;
        } else if (baseType == Type::tPointer) {
            return true;
        } else {
            return true;
        }
    }

    bool TypeDecl::canCopy(bool tempMatters) const {
        if ( baseType == Type::tHandle ) {
            return annotation->canCopy();
        } else if (baseType == Type::tArray || baseType == Type::tTable || baseType == Type::tBlock || baseType == Type::tIterator) {
            return false;
        } else if (baseType == Type::tStructure && structType) {
            return !structType->circular ? structType->canCopy(tempMatters) : false;
        } else if (baseType == Type::tTuple || baseType == Type::tVariant || baseType == Type::option) {
            for (const auto & arg : argTypes) {
                if (!arg->canCopy(tempMatters)) return false;
            }
            return true;
        } else if (baseType == Type::tPointer) {
            return !smartPtr;
        } else if ( baseType == Type::tLambda ) {
            return false;
        } else if ( baseType == Type::tString ) {
            return tempMatters ? false : true;
        } else {
            return true;
        }
    }

        bool TypeDecl::isNoHeapType() const {
            if ( baseType==Type::tArray || baseType==Type::tTable
                    || baseType==Type::tBlock || baseType==Type::tLambda )
                return false;
            if ( baseType==Type::tStructure && structType )
                return !structType->circular ? structType->isNoHeapType() : false;
            if ( baseType==Type::tHandle )
                return annotation->isPod();
            if ( baseType==Type::tTuple || baseType==Type::tVariant || baseType == Type::option ) {
                for ( auto & arg : argTypes ) {
                    if ( !arg->isNoHeapType() ) {
                        return false;
                    }
                }
            }
            if ( baseType==Type::tPointer ) return false;   // pointer can point to heap
            return true;
        }

    bool TypeDecl::isPod() const {
        if ( baseType==Type::tArray || baseType==Type::tTable || baseType==Type::tString
                || baseType==Type::tBlock || baseType==Type::tLambda )
            return false;
        if ( baseType==Type::tStructure && structType )
            return !structType->circular ? structType->isPod() : false;
        if ( baseType==Type::tHandle )
            return annotation->isPod();
        if ( baseType==Type::tPointer )
            return !smartPtr;
        if ( baseType==Type::tTuple || baseType==Type::tVariant || baseType == Type::option ) {
            for ( auto & arg : argTypes ) {
                if ( !arg->isPod() ) {
                    return false;
                }
            }
        }
        return true;
    }

    bool TypeDecl::isRawPod() const {
        if ( baseType==Type::tArray || baseType==Type::tTable || baseType==Type::tString
            || baseType==Type::tBlock || baseType==Type::tLambda || baseType==Type::tFunction )
            return false;
        if ( baseType==Type::tStructure && structType )
            return !structType->circular ? structType->isRawPod() : false;
        if ( baseType==Type::tHandle )
            return annotation->isRawPod();
        if ( baseType==Type::tPointer )
            return false;
        if ( baseType==Type::tTuple || baseType==Type::tVariant || baseType == Type::option ) {
            for ( auto & arg : argTypes ) {
                if ( !arg->isRawPod() ) {
                    return false;
                }
            }
        }
        return true;
    }

    bool TypeDecl::needInScope() const {
        das_set<Structure *> dep;
        return needInScope(dep);
    }

    bool TypeDecl::needInScope( das_set<Structure*> & dep ) const {
        if ( baseType==Type::tHandle ) {
            return annotation->needInScope();
        } else if ( baseType==Type::tStructure ) {
            if (structType) {
                if (dep.find(structType) != dep.end()) return false;
                dep.insert(structType);
                return structType->needInScope(dep);
            }
        } else if ( baseType==Type::tTuple || baseType==Type::tVariant || baseType == Type::option ) {
            for ( const auto & arg : argTypes ) {
                if ( arg->needInScope(dep) ) {
                    return true;
                }
            }
            return false;
        } else if ( baseType==Type::tArray || baseType==Type::tTable ) {
            if ( firstType && firstType->needInScope(dep) ) return true;
            if ( secondType && secondType->needInScope(dep) ) return true;
        } else if ( baseType==Type::tPointer) {
            return smartPtr;
        }
        return false;
    }

    bool TypeDecl::canBePlacedInContainer() const {
        das_set<Structure *> dep;
        return canBePlacedInContainer(dep);
    }

    bool TypeDecl::canBePlacedInContainer(das_set<Structure *> & dep) const {
        if ( baseType==Type::tHandle ) {
            return annotation->canBePlacedInContainer();
        } else if ( baseType==Type::tStructure ) {
            if (structType) {
                if (dep.find(structType) != dep.end()) return true;
                dep.insert(structType);
                return structType->canBePlacedInContainer(dep);
            }
        } else if ( baseType==Type::tTuple || baseType==Type::tVariant || baseType == Type::option ) {
            for ( const auto & arg : argTypes ) {
                if ( !arg->canBePlacedInContainer(dep) ) {
                    return false;
                }
            }
            return true;
        } else if ( baseType==Type::tBlock ) {
            return false;
        } else if ( baseType==Type::tArray || baseType==Type::tTable ) {
            if ( firstType && !firstType->canBePlacedInContainer(dep) ) return false;
            if ( secondType && !secondType->canBePlacedInContainer(dep) ) return false;
        }
        return true;
    }

    bool TypeDecl::hasNonTrivialCtor() const {
        das_set<Structure *> dep;
        return hasNonTrivialCtor(dep);
    }

    bool TypeDecl::hasNonTrivialCtor(das_set<Structure *> & dep) const {
        if ( baseType==Type::tHandle ) {
            return annotation->hasNonTrivialCtor();
        } else if ( baseType==Type::tStructure ) {
            if (structType) {
                if (dep.find(structType) != dep.end()) return false;
                dep.insert(structType);
                return structType->hasNonTrivialCtor(dep);
            }
        } else if ( baseType==Type::tTuple || baseType==Type::tVariant || baseType == Type::option ) {
            for ( const auto & arg : argTypes ) {
                if ( arg->hasNonTrivialCtor(dep) ) {
                    return true;
                }
            }
        } else if ( baseType==Type::tArray || baseType==Type::tTable ) {
            if ( firstType && firstType->hasNonTrivialCtor(dep) ) return true;
            if ( secondType && secondType->hasNonTrivialCtor(dep) ) return true;
        }
        return false;
    }

    bool TypeDecl::hasNonTrivialDtor() const {
        das_set<Structure *> dep;
        return hasNonTrivialDtor(dep);
    }

    bool TypeDecl::hasNonTrivialDtor(das_set<Structure *> & dep) const {
        if ( baseType==Type::tHandle ) {
            return annotation->hasNonTrivialDtor();
        } else if ( baseType==Type::tStructure ) {
            if (structType) {
                if (dep.find(structType) != dep.end()) return false;
                dep.insert(structType);
                return structType->hasNonTrivialDtor(dep);
            }
        } else if ( baseType==Type::tTuple || baseType==Type::tVariant || baseType == Type::option ) {
            for ( const auto & arg : argTypes ) {
                if ( arg->hasNonTrivialDtor(dep) ) {
                    return true;
                }
            }
        } else if ( baseType==Type::tArray || baseType==Type::tTable ) {
            if ( firstType && firstType->hasNonTrivialDtor(dep) ) return true;
            if ( secondType && secondType->hasNonTrivialDtor(dep) ) return true;
        }
        return false;
    }

    bool TypeDecl::hasNonTrivialCopy() const {
        das_set<Structure *> dep;
        return hasNonTrivialCopy(dep);
    }

    bool TypeDecl::hasNonTrivialCopy(das_set<Structure *> & dep) const {
        if ( baseType==Type::tHandle ) {
            return annotation->hasNonTrivialCopy();
        } else if ( baseType==Type::tStructure ) {
            if (structType) {
                if (dep.find(structType) != dep.end()) return false;
                dep.insert(structType);
                return structType->hasNonTrivialCopy(dep);
            }
        } else if ( baseType==Type::tTuple || baseType==Type::tVariant || baseType == Type::option ) {
            for ( const auto & arg : argTypes ) {
                if ( arg->hasNonTrivialCopy(dep) ) {
                    return true;
                }
            }
        } else if ( baseType==Type::tArray || baseType==Type::tTable ) {
            if ( firstType && firstType->hasNonTrivialCopy(dep) ) return true;
            if ( secondType && secondType->hasNonTrivialCopy(dep) ) return true;
        }
        return false;
    }

    bool TypeDecl::hasClasses() const {
        das_set<Structure *> dep;
        return hasClasses(dep);
    }

    bool TypeDecl::hasClasses(das_set<Structure *> & dep) const {
        if ( baseType==Type::tStructure ) {
            if (structType) {
                if (dep.find(structType) != dep.end()) return false;
                dep.insert(structType);
                return structType->hasClasses(dep);
            }
        } else if ( baseType==Type::tTuple || baseType==Type::tVariant || baseType == Type::option ) {
            for ( const auto & arg : argTypes ) {
                if ( arg->hasClasses(dep) ) {
                    return true;
                }
            }
        } else if ( baseType==Type::tArray || baseType==Type::tTable ) {
            if ( firstType && firstType->hasClasses(dep) ) return true;
            if ( secondType && secondType->hasClasses(dep) ) return true;
        }
        return false;
    }

    bool TypeDecl::lockCheck() const {
        das_set<Structure *> dep;
        return lockCheck(dep);
    }

    bool TypeDecl::lockCheck(das_set<Structure *> & dep) const {
        // logic is 'OR'
        if ( baseType==Type::tStructure ) {
            if ( structType ) {
                if (dep.find(structType) != dep.end()) return false;
                if ( structType->skipLockCheck ) return false;
                dep.insert(structType);
                for ( auto fld : structType->fields ) {
                    bool checkLocks = true;
                    for ( auto & ann : fld.annotation ) {
                        if ( ann.name=="skip_field_lock_check" ) {
                            checkLocks = false;
                            break;
                        }
                    }
                    if ( checkLocks && fld.type->lockCheck(dep) ) {
                        return true;
                    }
                }
            }
        } else if ( baseType==Type::tTuple || baseType==Type::tVariant || baseType == Type::option ) {
            for ( const auto & arg : argTypes ) {
                if ( arg->lockCheck(dep) ) {
                    return true;
                }
            }
        } else if ( baseType==Type::tArray || baseType==Type::tTable ) {
            return true;
        }
        return false;
    }

    int32_t TypeDecl::gcFlags() const {
        das_set<Structure *> dep;
        das_set<Annotation *> depA;
        return gcFlags(dep,depA);
    }

    int32_t TypeDecl::gcFlags(das_set<Structure *> & dep, das_set<Annotation *> & depA) const {
        int32_t gcf = 0;
        if ( baseType==Type::tStructure ) {
            if ( structType ) {
                if (dep.find(structType) != dep.end()) return 0;
                dep.insert(structType);
                if ( structType->isLambda || structType->isClass ) gcf |= gcFlag_heap | gcFlag_stringHeap;
                for ( auto fld : structType->fields ) {
                    gcf |= fld.type->gcFlags(dep,depA);
                }
            }
        } else if ( baseType==Type::tTuple || baseType==Type::tVariant || baseType == Type::option ) {
            for ( const auto & arg : argTypes ) {
                gcf |= arg->gcFlags(dep,depA);
            }
        } else if ( baseType==Type::tArray || baseType==Type::tTable ) {
            gcf |= gcFlag_heap;
            if ( firstType ) gcf |= firstType->gcFlags(dep,depA);
            if ( secondType ) gcf |= secondType->gcFlags(dep,depA);
        } else if ( baseType==Type::tHandle ) {
            if (depA.find(annotation) != depA.end()) return 0;
            depA.insert(annotation);
            auto ann = static_cast<TypeAnnotation *>(annotation);
            gcf |= ann->getGcFlags(dep,depA);
        } else if ( baseType==Type::tString ) {
            gcf |= gcFlag_stringHeap;
        } else if ( baseType==Type::tPointer ) {
            gcf |= gcFlag_heap;
            if ( firstType ) gcf |= firstType->gcFlags(dep,depA);
        } else if ( baseType==Type::tLambda || baseType==Type::tIterator ) {
            gcf |= gcFlag_heap;
            gcf |= gcFlag_stringHeap;
        }
        return gcf;
    }

    bool TypeDecl::isSafeToDelete() const {
        das_set<Structure *> dep;
        return isSafeToDelete(dep);
    }

    bool TypeDecl::isSafeToDelete ( das_set<Structure*> & dep) const {
        if ( baseType==Type::tPointer ) {
            if ( smartPtr ) return true;
            return !firstType || firstType->baseType==Type::tVoid;
        } else if ( baseType==Type::tHandle ) {
            return !annotation->hasNonTrivialDtor() || annotation->canDelete();
        } else  if ( baseType==Type::tStructure ) {
            if (structType) {
                if (dep.find(structType) != dep.end()) return true;
                dep.insert(structType);
                return structType->isSafeToDelete(dep);
            } else {
                return true;
            }
        } else if ( baseType==Type::tTuple || baseType==Type::tVariant || baseType == Type::option ) {
            for ( const auto & arg : argTypes ) {
                if ( !arg->isSafeToDelete(dep) ) {
                    return false;
                }
            }
            return true;
        } else if ( baseType==Type::tBlock ) {
            return false;
        } else if ( baseType==Type::tTable ) {
            if ( secondType && !secondType->isSafeToDelete(dep) ) return false;
        } else if ( baseType==Type::tArray ) {
            if ( firstType && !firstType->isSafeToDelete(dep) ) return false;
        }
        return true;
    }


    bool TypeDecl::isLocal() const {
        das_set<Structure *> dep;
        return isLocal(dep);
    }

    bool TypeDecl::isLocal(das_set<Structure *> & dep) const {
        if ( baseType==Type::tHandle ) {
            return annotation->isLocal();
        } else  if ( baseType==Type::tStructure ) {
            if (structType) {
                if (dep.find(structType) != dep.end()) return true;
                dep.insert(structType);
                return structType->isLocal(dep);
            } else {
                return true;
            }
        } else if ( baseType==Type::tTuple || baseType==Type::tVariant || baseType == Type::option ) {
            for ( const auto & arg : argTypes ) {
                if ( !arg->isLocal(dep) ) {
                    return false;
                }
            }
            return true;
        } else if ( baseType==Type::tBlock ) {
            return false;
        } else if ( baseType==Type::tArray || baseType==Type::tTable ) {
            if ( firstType && !firstType->isLocal(dep) ) return false;
            if ( secondType && !secondType->isLocal(dep) ) return false;
        }
        return true;
    }

    void TypeDecl::sanitize ( ) {
        isExplicit = false;
        if ( firstType ) firstType->sanitize();
        if ( secondType ) secondType->sanitize();
        for ( auto & argT : argTypes ) argT->sanitize();
    }

    bool isSameExactNullType ( const TypeDeclPtr & a, const TypeDeclPtr & b ) {
        if ( a && b ) {
            return a->isSameExactType(*b);
        } else if ( a && !b ) {
            return false;
        } else if ( !a && b ) {
            return false;
        } else {
            return true;
        }
    }

    bool TypeDecl::isSameExactType ( const TypeDecl & decl ) const {
        if (    baseType!=decl.baseType || structType!=decl.structType || enumType!=decl.enumType
            ||  annotation!=decl.annotation || flags!=decl.flags || alias!=decl.alias ) {
                return false;
            }
        if ( dim.size() != decl.dim.size() ) return false;
        for ( size_t i=0, is=dim.size(); i!=is; ++i ) {
            if ( dim[i] != decl.dim[i] ) {
                return false;
            }
        }
        if ( !isSameExactNullType(firstType,decl.firstType) ) return false;
        if ( !isSameExactNullType(secondType,decl.secondType) ) return false;
        if ( argTypes.size() != decl.argTypes.size() ) return false;
        for ( size_t i=0, is=argTypes.size(); i!=is; ++i ) {
            if ( !argTypes[i]->isSameExactType(*(decl.argTypes[i])) ) {
                return false;
            }
        }
        if ( argNames.size() != decl.argNames.size() ) return false;
        for ( size_t i=0, is=argNames.size(); i!=is; ++i ) {
            if ( argNames[i]!=decl.argNames[i] ) {
                return false;
            }
        }
        return true;
    }

    bool TypeDecl::isSameType ( const TypeDecl & decl,
             RefMatters refMatters,
             ConstMatters constMatters,
             TemporaryMatters temporaryMatters,
             AllowSubstitute allowSubstitute,
             bool topLevel,
             bool isPassType ) const {
        if ( baseType!=decl.baseType ) {
            return false;
        }
        if ( topLevel && !isPointer() && !isRef()  ) {
            constMatters = ConstMatters::no;
        }
        if ( topLevel && !isTempType() ) {
            temporaryMatters = TemporaryMatters::no;
        }
        if ( refMatters == RefMatters::yes ) {
            if ( ref!=decl.ref ) {
                return false;
            }
        }
        if ( constMatters == ConstMatters::yes ) {
            if ( constant!=decl.constant ) {
                return false;
            }
        }
        if ( temporaryMatters == TemporaryMatters::yes ) {
            if ( temporary != decl.temporary ) {
                return false;
            }
        }
        if ( dim!=decl.dim ) {
            return false;
        }
        if ( baseType==Type::tHandle && annotation!=decl.annotation ) {
            if ( !isExplicit && (allowSubstitute == AllowSubstitute::yes) ) {
                if ( annotation->canSubstitute(decl.annotation) ) {
                    return true;
                } else if ( decl.annotation->canBeSubstituted(annotation) ) {
                    return true;
                }
            }
            return false;
        }
        if ( baseType==Type::tStructure && structType!=decl.structType ) {
            if ( !isExplicit && (allowSubstitute == AllowSubstitute::yes) ) {
                if ( structType && decl.structType && structType->isCompatibleCast(*(decl.structType)) ){
                    return true;
                }
            }
            return false;
        }
        if ( baseType==Type::tPointer || baseType==Type::tIterator ) {
            if ( smartPtr != decl.smartPtr ) {
                return false;
            }
            bool iAmVoid = !firstType || firstType->isVoid();
            bool heIsVoid = !decl.firstType || decl.firstType->isVoid();
            TemporaryMatters tpm = implicit ? TemporaryMatters::no : TemporaryMatters::yes;
            if ( topLevel ) {
                ConstMatters pcm = ConstMatters::yes;
                if ( isPassType && firstType && firstType->constant ) {
                    pcm = ConstMatters::no;
                }
                if ( !iAmVoid && !heIsVoid &&
                        !firstType->isSameType(*decl.firstType,RefMatters::yes,pcm,
                            tpm, isExplicit ? AllowSubstitute::no : allowSubstitute,false) ) {
                    return false;
                }
            } else {
                if ( !iAmVoid && !heIsVoid ) {
                    if ( !firstType->isSameType(*decl.firstType,RefMatters::yes,ConstMatters::yes,
                                tpm, isExplicit ? AllowSubstitute::no : allowSubstitute,false) ) {
                        return false;
                    }
                } else {
                    if ( iAmVoid != heIsVoid ) {
                        return false;
                    }
                }
            }
        }
        if ( isEnumT() ) {
            if ( baseType != decl.baseType ) {
                return false;
            }
            if ( enumType && decl.enumType && enumType!=decl.enumType ) {
                return false;
            }
        }
        if ( baseType==Type::tArray ) {
            if ( firstType && decl.firstType && !firstType->isSameType(*decl.firstType,RefMatters::yes,ConstMatters::yes,
                    TemporaryMatters::yes,AllowSubstitute::no,false) ) {
                return false;
            }
        }
        if ( baseType==Type::tTable ) {
            if ( firstType && decl.firstType && !firstType->isSameType(*decl.firstType,RefMatters::yes,ConstMatters::yes,
                    TemporaryMatters::yes,AllowSubstitute::no,false) ) {
                return false;
            }
            if ( secondType && decl.secondType && !secondType->isSameType(*decl.secondType,RefMatters::yes,ConstMatters::yes,
                    TemporaryMatters::yes,AllowSubstitute::no,false) ) {
                return false;
            }
        }
        if ( baseType==Type::tTuple || baseType==Type::tVariant || baseType==Type::option ) {
            if ( firstType && decl.firstType && !firstType->isSameType(*decl.firstType,RefMatters::yes,ConstMatters::yes,
                    TemporaryMatters::yes,AllowSubstitute::no,true) ) {
                return false;
            }
            if ( firstType || argTypes.size() ) {    // if not any block or any function
                if ( argTypes.size() != decl.argTypes.size() ) {
                    return false;
                }
                if (baseType == Type::tVariant) {
                    if (argNames.size() != decl.argNames.size()) {
                        return false;
                    }
                    for ( size_t i=0, is=argNames.size(); i!=is; ++i ) {
                        const auto & arg = argNames[i];
                        const auto & declArg = decl.argNames[i];
                        if ( arg != declArg ) {
                            return false;
                        }
                    }
                }
                for ( size_t i=0, is=argTypes.size(); i!=is; ++i ) {
                    const auto & arg = argTypes[i];
                    const auto & declArg = decl.argTypes[i];
                    if ( !arg->isSameType(*declArg, RefMatters::yes, ConstMatters::yes,
                            TemporaryMatters::yes,AllowSubstitute::no,true ) ) {
                        return false;
                    }
                }
            }
        }
        if ( baseType==Type::tBlock || baseType==Type::tFunction ||baseType==Type::tLambda  ) {
            if ( firstType && decl.firstType && !firstType->isSameType(*decl.firstType,RefMatters::yes,ConstMatters::yes,
                    TemporaryMatters::yes,AllowSubstitute::no,true) ) {
                return false;
            }
            if ( firstType || argTypes.size() ) {    // if not any block or any function
                if ( argTypes.size() != decl.argTypes.size() ) {
                    return false;
                }
                for ( size_t i=0, is=argTypes.size(); i!=is; ++i ) {
                    const auto & argType = argTypes[i];
                    const auto & passType = decl.argTypes[i];
                    auto rMat = argType->isRefType() ? RefMatters::no : RefMatters::yes;
                    if ( !argType->isSameType(*passType, rMat, ConstMatters::yes,
                            TemporaryMatters::yes,AllowSubstitute::no,true ) ) {
                        return false;
                    }
                }
            }
        }
        if ( baseType==Type::tBitfield ) {
            bool iAmAnyBitfield = argNames.size()==0;
            bool heIsAnyBitfield = decl.argNames.size()==0;
            bool compareArgs = false;
            if ( topLevel ) {
                if ( !iAmAnyBitfield && !heIsAnyBitfield ) {
                    compareArgs = true;
                }
            } else {
                if ( !iAmAnyBitfield && !heIsAnyBitfield ) {
                    compareArgs = true;
                }
                if ( iAmAnyBitfield != heIsAnyBitfield ) {
                    return false;
                }
            }
            if ( compareArgs ) {
                if (argNames.size() != decl.argNames.size()) {
                    return false;
                }
                for ( size_t i=0, is=argNames.size(); i!=is; ++i ) {
                    const auto & arg = argNames[i];
                    const auto & declArg = decl.argNames[i];
                    if ( arg != declArg ) {
                        return false;
                    }
                }
            }
        }
        return true;
    }

    bool TypeDecl::canWrite() const {
        bool cw = baseType==Type::tPointer || baseType==Type::anyArgument || isRef();
        if ( baseType!=Type::tPointer ) {
            cw &= !constant;
        } else if ( firstType ) {
            cw &= !firstType->constant;
        }
        return cw;
    }

    // validate swizzle mask and build mask type

    bool TypeDecl::isSequencialMask ( const vector<uint8_t> & fields ) {
        for ( size_t i=1, is=fields.size(); i<is; ++i ) {
            if ( (fields[i-1]+1)!=fields[i] ) {
                return false;
            }
        }
        return true;
    }

    int TypeDecl::getMaskFieldIndex ( char ch ) {
        switch ( ch ) {
            case 'x':   case 'X':   case 'r':   case 'R':   return 0;
            case 'y':   case 'Y':   case 'g':   case 'G':   return 1;
            case 'z':   case 'Z':   case 'b':   case 'B':   return 2;
            case 'w':   case 'W':   case 'a':   case 'A':   return 3;
            default:    return -1;
        }
    }

    bool TypeDecl::buildSwizzleMask ( const string & mask, int dim, vector<uint8_t> & fields ) {
        fields.clear();
        for ( auto ch : mask ) {
            int field = getMaskFieldIndex(ch);
            if ( field==-1 || field>=dim ) {
                return false;
            }
            fields.push_back(uint8_t(field));
        }
        return fields.size()>=1 && fields.size()<=4;
    }

    bool TypeDecl::isBaseVectorType() const {
        switch (baseType) {
            case tInt2:
            case tInt3:
            case tInt4:
            case tUInt2:
            case tUInt3:
            case tUInt4:
            case tFloat2:
            case tFloat3:
            case tFloat4:
            case tRange:
            case tURange:
            case tRange64:
            case tURange64:
                return true;
            default:
                return false;
        }
    }

    int TypeDecl::getVectorDim() const {
        switch (baseType) {
            case tInt2:
            case tUInt2:
            case tFloat2:
            case tRange:
            case tURange:
            case tRange64:
            case tURange64:
                return 2;
            case tInt3:
            case tUInt3:
            case tFloat3:
                return 3;
            case tInt4:
            case tUInt4:
            case tFloat4:
                return 4;
            default:
                DAS_ASSERTF(0,
                        "we should not even be here. we are calling getVectorDim on an unsupported baseType."
                        "likely new vector type been added.");
                return 0;
        }
    }

    Type TypeDecl::getVectorBaseType() const {
        switch (baseType) {
            case tInt2:
            case tInt3:
            case tInt4:     return Type::tInt;
            case tUInt2:
            case tUInt3:
            case tUInt4:    return Type::tUInt;
            case tFloat2:
            case tFloat3:
            case tFloat4:   return Type::tFloat;
            case tRange:    return Type::tInt;
            case tURange:   return Type::tUInt;
            case tRange64:  return Type::tInt64;
            case tURange64: return Type::tUInt64;
            default:
                DAS_ASSERTF(0,
                       "we should not even be here. we are calling getVectorBaseType on an unsuppored baseType."
                       "likely new vector type been added.");
                return Type::none;
        }
    }

    Type TypeDecl::getVectorType ( Type bt, int dim ) {
        if ( dim==1 ) return bt;
        if ( bt==Type::tFloat ) {
            switch ( dim ) {
                case 2:     return Type::tFloat2;
                case 3:     return Type::tFloat3;
                case 4:     return Type::tFloat4;
                default:    DAS_ASSERTF(0, "we should not be herewe are calling getVectorType on an unsuppored baseType."
                                   "likely new vector type been added.");
                            return Type::none;
            }
        } else if ( bt==Type::tInt ) {
            switch ( dim ) {
                case 2:     return Type::tInt2;
                case 3:     return Type::tInt3;
                case 4:     return Type::tInt4;
                default:    DAS_ASSERTF(0, "we should not be here. we are calling getVectorType on an unsuppored baseType."
                                   "likely new vector type been added.");
                    return Type::none;
            }
        } else if ( bt==Type::tUInt ) {
            switch ( dim ) {
                case 2:     return Type::tUInt2;
                case 3:     return Type::tUInt3;
                case 4:     return Type::tUInt4;
                default:    DAS_ASSERTF(0, "we should not be here. we are calling getVectorType on an unsuppored baseType."
                                   "likely new vector type been added.");
                    return Type::none;
            }
        } else {
            DAS_ASSERTF(0, "we should not be here. we are calling getVectorType on an unsuppored baseType."
                   "likely new vector type been added.");
            return Type::none;
        }
    }

    Type TypeDecl::getRangeType ( Type bt, int dim ) {
        if ( dim==1 ) return bt;
        switch ( bt ) {
            case Type::tInt:    return Type::tRange;
            case Type::tUInt:   return Type::tURange;
            case Type::tInt64:  return Type::tRange64;
            case Type::tUInt64: return Type::tURange64;
            default: DAS_ASSERTF(0, "we should not be here. we are calling getRangeType on an unsuppored baseType."
                "likely new range type been added.");
                return Type::none;
        }
    }

    void TypeDecl::collectAliasList(vector<string> & aliases) const {
        // auto is auto.... or auto....?
        if ( baseType==Type::alias ) {
            aliases.push_back(alias);
        } else  if ( baseType==Type::tPointer ) {
            if ( firstType )
                firstType->collectAliasList(aliases);
        } else  if ( baseType==Type::tIterator ) {
            if ( firstType )
                firstType->collectAliasList(aliases);
        } else if ( baseType==Type::tArray ) {
            if ( firstType )
                firstType->collectAliasList(aliases);
        } else if ( baseType==Type::tTable ) {
            if ( firstType )
                firstType->collectAliasList(aliases);
            if ( secondType )
                secondType->collectAliasList(aliases);
        } else if ( baseType==Type::tBlock || baseType==Type::tFunction ||
                   baseType==Type::tLambda || baseType==Type::tTuple ||
                   baseType==Type::tVariant  || baseType == Type::option ) {
            if ( firstType )
                firstType->collectAliasList(aliases);
            for ( auto & arg : argTypes )
                arg->collectAliasList(aliases);
        }
    }

    bool TypeDecl::isAotAlias () const {
        if ( aotAlias ) {
            return true;
        } else  if ( baseType==Type::tPointer ) {
            if ( firstType )
                return firstType->isAotAlias();
        } else  if ( baseType==Type::tIterator ) {
            if ( firstType )
                return firstType->isAotAlias();
        } else if ( baseType==Type::tArray ) {
            if ( firstType )
                return firstType->isAotAlias();
        } else if ( baseType==Type::tTable ) {
            bool any = false;
            if ( firstType )
                any |= firstType->isAotAlias();
            if ( secondType )
                any |= secondType->isAotAlias();
            return any;
        } else if ( baseType==Type::tBlock || baseType==Type::tFunction ||
                   baseType==Type::tLambda || baseType==Type::tTuple ||
                   baseType==Type::tVariant || baseType == Type::option ) {
            bool any = false;
            if ( firstType )
                any |= firstType->isAotAlias();
            for ( auto & arg : argTypes )
                any |= arg->isAotAlias();
            return any;
        }
        return false;
    }

    bool TypeDecl::isAlias() const {
        // auto is auto.... or auto....?
        if ( baseType==Type::alias ) {
            return true;
        } else  if ( baseType==Type::tPointer ) {
            if ( firstType )
                return firstType->isAlias();
        } else  if ( baseType==Type::tIterator ) {
            if ( firstType )
                return firstType->isAlias();
        } else if ( baseType==Type::tArray ) {
            if ( firstType )
                return firstType->isAlias();
        } else if ( baseType==Type::tTable ) {
            bool any = false;
            if ( firstType )
                any |= firstType->isAlias();
            if ( secondType )
                any |= secondType->isAlias();
            return any;
        } else if ( baseType==Type::tBlock || baseType==Type::tFunction ||
                   baseType==Type::tLambda || baseType==Type::tTuple ||
                   baseType==Type::tVariant || baseType==Type::option ) {
            bool any = false;
            if ( firstType )
                any |= firstType->isAlias();
            for ( auto & arg : argTypes )
                any |= arg->isAlias();
            return any;
        }
        return false;
    }

    bool TypeDecl::isAliasOrExpr() const {
        // if its dim[expr]
        for ( auto di : dim ) {
            if ( di==TypeDecl::dimConst ) {
                return true;
            }
        }
        // auto is auto.... or auto....?
        if ( baseType==Type::alias ) {
            return true;
        } else  if ( baseType==Type::tPointer ) {
            if ( firstType )
                return firstType->isAliasOrExpr();
        } else  if ( baseType==Type::tIterator ) {
            if ( firstType )
                return firstType->isAliasOrExpr();
        } else if ( baseType==Type::tArray ) {
            if ( firstType )
                return firstType->isAliasOrExpr();
        } else if ( baseType==Type::tTable ) {
            bool any = false;
            if ( firstType )
                any |= firstType->isAliasOrExpr();
            if ( secondType )
                any |= secondType->isAliasOrExpr();
            return any;
        } else if ( baseType==Type::tBlock || baseType==Type::tFunction ||
                   baseType==Type::tLambda || baseType==Type::tTuple ||
                   baseType==Type::tVariant|| baseType==Type::option ) {
            bool any = false;
            if ( firstType )
                any |= firstType->isAliasOrExpr();
            for ( auto & arg : argTypes )
                any |= arg->isAliasOrExpr();
            return any;
        }
        return false;
    }

    bool TypeDecl::isAutoArrayResolved() const {
        for ( auto di : dim ) {
            if ( di==TypeDecl::dimAuto ) {
                return false;
            }
        }
        return true;
    }

    bool TypeDecl::isAuto() const {
        // auto is auto.... or auto....?
        // also dim[] is aito
        for ( auto di : dim ) {
            if ( di==TypeDecl::dimAuto ) {
                return true;
            }
        }
        if ( baseType==Type::autoinfer ) {
            return true;
        } else if ( baseType==Type::option ) {
            return true;
        } else if ( baseType==Type::tPointer ) {
            if ( firstType )
                return firstType->isAuto();
        } else if ( baseType==Type::tIterator ) {
            if ( firstType )
                return firstType->isAuto();
        } else if ( baseType==Type::tArray ) {
            if ( firstType )
                return firstType->isAuto();
        } else if ( baseType==Type::tTable ) {
            bool any = false;
            if ( firstType )
                any |= firstType->isAuto();
            if ( secondType )
                any |= secondType->isAuto();
            return any;
        } else if ( baseType==Type::tBlock || baseType==Type::tFunction ||
                   baseType==Type::tLambda || baseType==Type::tTuple ||
                   baseType==Type::tVariant ) {
            bool any = false;
            if ( firstType )
                any |= firstType->isAuto();
            for ( auto & arg : argTypes )
                any |= arg->isAuto();
            return any;
        }
        return false;
    }

    bool TypeDecl::isAutoWithoutOptions(bool & hasOptions) const {
        // auto is auto.... or auto....?
        // also dim[] is aito
        for ( auto di : dim ) {
            if ( di==TypeDecl::dimAuto ) {
                return true;
            }
        }
        if ( baseType==Type::autoinfer ) {
            return true;
        } else if ( baseType==Type::tPointer ) {
            if ( firstType )
                return firstType->isAutoWithoutOptions(hasOptions);
        } else if ( baseType==Type::tIterator ) {
            if ( firstType )
                return firstType->isAutoWithoutOptions(hasOptions);
        } else if ( baseType==Type::tArray ) {
            if ( firstType )
                return firstType->isAutoWithoutOptions(hasOptions);
        } else if ( baseType==Type::tTable ) {
            bool any = false;
            if ( firstType )
                any |= firstType->isAutoWithoutOptions(hasOptions);
            if ( secondType )
                any |= secondType->isAutoWithoutOptions(hasOptions);
            return any;
        } else if ( baseType==Type::tBlock || baseType==Type::tFunction ||
                   baseType==Type::tLambda || baseType==Type::tTuple ||
                   baseType==Type::tVariant || baseType==Type::option ) {
            if ( baseType==Type::option ) hasOptions = true;
            bool any = false;
            if ( firstType )
                any |= firstType->isAutoWithoutOptions(hasOptions);
            for ( auto & arg : argTypes )
                any |= arg->isAutoWithoutOptions(hasOptions);
            return any;
        }
        return false;
    }


    bool TypeDecl::isAutoOrAlias() const {
        // auto is auto.... or auto....?
        // also dim[] is aito
        for (auto di : dim) {
            if (di == TypeDecl::dimAuto) {
                return true;
            }
        }
        if (baseType == Type::autoinfer) {
            return true;
        } else if ( baseType==Type::option ) {
            return true;
        } if (baseType == Type::alias) {
            return true;
        } else if (baseType == Type::tPointer) {
            if (firstType)
                return firstType->isAutoOrAlias();
        } else if (baseType == Type::tIterator) {
            if (firstType)
                return firstType->isAutoOrAlias();
        } else if (baseType == Type::tArray) {
            if (firstType)
                return firstType->isAutoOrAlias();
        } else if (baseType == Type::tTable) {
            bool any = false;
            if (firstType)
                any |= firstType->isAutoOrAlias();
            if (secondType)
                any |= secondType->isAutoOrAlias();
            return any;
        } else if (baseType == Type::tBlock || baseType == Type::tFunction ||
            baseType == Type::tLambda || baseType == Type::tTuple ||
            baseType == Type::tVariant ) {
            bool any = false;
            if (firstType)
                any |= firstType->isAutoOrAlias();
            for (auto& arg : argTypes)
                any |= arg->isAutoOrAlias();
            return any;
        }
        return false;
    }

    bool TypeDecl::isFoldable() const {
        if ( dim.size() || ref )
            return false;
        switch ( baseType ) {
            case Type::tBool:
            case Type::tInt:
            case Type::tInt2:
            case Type::tInt3:
            case Type::tInt4:
            case Type::tInt8:
            case Type::tInt16:
            case Type::tInt64:
            case Type::tUInt:
            case Type::tBitfield:
            case Type::tUInt2:
            case Type::tUInt3:
            case Type::tUInt4:
            case Type::tUInt8:
            case Type::tUInt16:
            case Type::tUInt64:
            case Type::tFloat:
            case Type::tFloat2:
            case Type::tFloat3:
            case Type::tFloat4:
            case Type::tString:
            case Type::tDouble:
            case Type::tEnumeration:
            case Type::tEnumeration8:
            case Type::tEnumeration16:
            case Type::tRange:
            case Type::tURange:
            case Type::tRange64:
            case Type::tURange64:
                return true;
            default:
                return false;
        }
    }

    bool TypeDecl::isCtorType() const {
        if ( dim.size() )
            return false;
        switch ( baseType ) {
            // case Type::tBool:
            case Type::tInt8:
            case Type::tUInt8:
            case Type::tInt16:
            case Type::tUInt16:
            case Type::tInt64:
            case Type::tUInt64:
            case Type::tInt:
            case Type::tInt2:
            case Type::tInt3:
            case Type::tInt4:
            case Type::tUInt:
            case Type::tBitfield:
            case Type::tUInt2:
            case Type::tUInt3:
            case Type::tUInt4:
            case Type::tFloat:
            case Type::tFloat2:
            case Type::tFloat3:
            case Type::tFloat4:
            case Type::tRange:
            case Type::tURange:
            case Type::tRange64:
            case Type::tURange64:
            case Type::tString:
            case Type::tDouble:
            // case Type::tPointer:
                return true;
            default:
                return false;
        }
    }

    bool TypeDecl::isWorkhorseType() const {
        if ( dim.size() )
            return false;
        switch ( baseType ) {
            case Type::tBool:
            case Type::tInt8:
            case Type::tUInt8:
            case Type::tInt16:
            case Type::tUInt16:
            case Type::tInt64:
            case Type::tUInt64:
            case Type::tEnumeration:
            case Type::tEnumeration8:
            case Type::tEnumeration16:
            case Type::tInt:
            case Type::tInt2:
            case Type::tInt3:
            case Type::tInt4:
            case Type::tUInt:
            case Type::tBitfield:
            case Type::tUInt2:
            case Type::tUInt3:
            case Type::tUInt4:
            case Type::tFloat:
            case Type::tFloat2:
            case Type::tFloat3:
            case Type::tFloat4:
            case Type::tRange:
            case Type::tURange:
            case Type::tRange64:
            case Type::tURange64:
            case Type::tString:
            case Type::tDouble:
                return true;
            case Type::tPointer:
                return !smartPtr;
            default:
                return false;
        }
    }

    bool TypeDecl::canInitWithZero() const {
        if ( dim.size() )
            return false;
        if ( isVectorType() )
            return true;
        switch ( baseType ) {
            case Type::tBool:
            case Type::tInt8:
            case Type::tUInt8:
            case Type::tInt16:
            case Type::tUInt16:
            case Type::tInt64:
            case Type::tUInt64:
            case Type::tInt:
            case Type::tUInt:
            case Type::tBitfield:
            case Type::tFloat:
            case Type::tDouble:
            case Type::tString:
                return true;
            case Type::tPointer:
                return !smartPtr;
            default:
                return false;
        }
    }

    bool TypeDecl::isPolicyType() const {
        if ( dim.size() )
            return true;
        switch ( baseType ) {
            case Type::tVoid:
            case Type::tEnumeration:
            case Type::tEnumeration8:
            case Type::tEnumeration16:
            case Type::tBool:
                /*
            case Type::tInt8:
            case Type::tUInt8:
            case Type::tInt16:
            case Type::tUInt16:
                 */
            case Type::tInt64:
            case Type::tUInt64:
            case Type::tInt:
            case Type::tUInt:
            case Type::tBitfield:
            case Type::tFloat:
            case Type::tDouble:
            case Type::tPointer:
            case Type::tFunction:
            case Type::tLambda:
                return false;
            default:
                return true;
        }
    }

    bool TypeDecl::isVecPolicyType() const {
        if ( dim.size() )
            return false;
        if ( baseType==Type::tString ) {
            return false;
        }
        return isPolicyType();
    }

    bool TypeDecl::isReturnType() const {
        if ( isGoodBlockType() ) {
            return false;
        }
        return true;
    }

    Type TypeDecl::getRangeBaseType() const {
        switch ( baseType ) {
            case Type::tRange:  return Type::tInt;
            case Type::tURange: return Type::tUInt;
            case Type::tRange64:  return Type::tInt64;
            case Type::tURange64: return Type::tUInt64;
            default:
                DAS_ASSERTF(0, "we should not even be here. we are calling getRangeBaseType on an unsuppored baseType."
                       "likely new range type been added.");
                return Type::none;
        }
    }

    bool TypeDecl::isRefType() const {
        if ( dim.size() ) return true;
        if ( baseType==Type::tHandle ) return annotation->isRefType();
        return baseType==Type::tTuple || baseType==Type::tVariant ||
                baseType==Type::tStructure || baseType==Type::tArray ||
                baseType==Type::tTable || baseType==Type::tBlock ||
                baseType==Type::tIterator;
    }

    bool TypeDecl::isTemp( bool topLevel, bool refMatters ) const {
        das_set<Structure *> dep;
        return isTemp(topLevel, refMatters, dep);
    }

    bool TypeDecl::isTemp( bool topLevel, bool refMatters, das_set<Structure *> & dep ) const {
        if ( topLevel && !isTempType(refMatters) ) {
            return false;
        } else if ( temporary ) {
            return true;
        } else if ( baseType==Type::tStructure ) {
            if (structType) {
                if (dep.find(structType) != dep.end()) return false;
                dep.insert(structType);
                return structType->isTemp(dep);
            } else {
                return false;
            }
        } else if ( baseType==Type::tPointer || baseType==Type::tIterator ) {
            return firstType ? firstType->isTemp(false, true, dep) : false;
        } else if ( baseType==Type::tArray ) {
            return firstType ? firstType->isTemp(false, true, dep) : false;
        } else if ( baseType==Type::tTable ) {
            if ( firstType && firstType->isTemp(false, true, dep) ) {
                return true;
            } else if ( secondType && secondType->isTemp(false, true, dep) ) {
                return true;
            }
        } else if ( /* baseType==Type::tBlock || baseType==Type::tFunction || baseType==Type::tLambda || */
                   baseType==Type::tTuple || baseType==Type::tVariant || baseType == Type::option ) {
            if ( firstType && firstType->isTemp(true, true, dep) ) {
                return true;
            }
            for ( const auto & argT : argTypes ) {
                if ( argT->isTemp(true, true, dep) ) {
                    return true;
                }
            }
        }
        return false;
    }

    bool TypeDecl::isFullyInferred() const {
        das_set<Structure *> dep;
        return isFullyInferred(dep);
    }

    bool TypeDecl::isFullyInferred( das_set<Structure *> & dep ) const {
        if ( isAutoOrAlias() ) {
            return false;
        } else if ( baseType==Type::tStructure ) {
            if ( structType ) {
                if (dep.find(structType) != dep.end()) return true;
                dep.insert(structType);
                for ( auto & fd : structType->fields ) {
                    if ( !fd.type ) return false;
                    if ( !fd.type->isFullyInferred(dep) ) return false;
                }
            }
        }
        return true;
    }

    bool TypeDecl::isShareable() const {
        das_set<Structure *> dep;
        return isShareable(dep);
    }

    bool TypeDecl::isShareable( das_set<Structure *> & dep ) const {
        if ( baseType==Type::tIterator || baseType==Type::tBlock || baseType==Type::tLambda ) {
            return false;
        } else if ( baseType==Type::tHandle ) {
            return annotation ? annotation->isShareable() : true;
        } else if ( baseType==Type::tStructure ) {
            if (structType) {
                if (dep.find(structType) != dep.end()) return true;
                dep.insert(structType);
                return structType->isShareable(dep);
            } else {
                return true;
            }
        } else if ( baseType==Type::tArray ) {
            return firstType ? firstType->isShareable(dep) : true;
        } else if ( baseType==Type::tTable ) {
            if ( firstType && !firstType->isShareable(dep) ) {
                return false;
            } else if ( secondType && !secondType->isShareable(dep) ) {
                return false;
            } else {
                return true;
            }
        } else if ( baseType==Type::tTuple || baseType==Type::tVariant || baseType == Type::option ) {
            for ( const auto & argT : argTypes ) {
                if ( !argT->isShareable(dep) ) {
                    return false;
                }
            }
            return true;
        } else {
            return true;
        }
    }

    bool TypeDecl::isFloatOrDouble() const {
        if (dim.size() != 0) return false;
        switch (baseType) {
        case Type::tFloat:
        case Type::tDouble:
            return true;
        default:;
        }
        return false;
    }

    bool TypeDecl::isBool() const {
        if (dim.size() != 0) return false;
        return baseType==Type::tBool;
    }

    bool TypeDecl::isInteger() const {
        if (dim.size() != 0) return false;
        switch (baseType) {
        case Type::tInt:
        case Type::tUInt:
        case Type::tBitfield:
        case Type::tInt8:
        case Type::tUInt8:
        case Type::tInt16:
        case Type::tUInt16:
        case Type::tInt64:
        case Type::tUInt64:
            return true;
        default:;
        }
        return false;
    }

    bool TypeDecl::isSignedInteger() const {
        if (dim.size() != 0) return false;
        switch (baseType) {
        case Type::tInt:
        case Type::tInt8:
        case Type::tInt16:
        case Type::tInt64:
            return true;
        default:;
        }
        return false;
    }

    bool TypeDecl::isUnsignedInteger() const {
        if (dim.size() != 0) return false;
        switch (baseType) {
        case Type::tUInt:
        case Type::tBitfield:
        case Type::tUInt8:
        case Type::tUInt16:
        case Type::tUInt64:
            return true;
        default:;
        }
        return false;
    }

    bool TypeDecl::isSignedIntegerOrIntVec() const {
        if (dim.size() != 0) return false;
        switch (baseType) {
        case Type::tInt:
        case Type::tInt8:
        case Type::tInt16:
        case Type::tInt64:
        case Type::tInt2:
        case Type::tInt3:
        case Type::tInt4:
            return true;
        default:;
        }
        return false;
    }

    bool TypeDecl::isUnsignedIntegerOrIntVec() const {
        if (dim.size() != 0) return false;
        switch (baseType) {
        case Type::tUInt:
        case Type::tBitfield:
        case Type::tUInt8:
        case Type::tUInt16:
        case Type::tUInt64:
        case Type::tUInt2:
        case Type::tUInt3:
        case Type::tUInt4:
            return true;
        default:;
        }
        return false;
    }


    bool TypeDecl::isNumeric() const {
        if (dim.size() != 0) return false;
        switch (baseType) {
        case Type::tInt:
        case Type::tUInt:
        case Type::tBitfield:
        case Type::tInt8:
        case Type::tUInt8:
        case Type::tInt16:
        case Type::tUInt16:
        case Type::tInt64:
        case Type::tUInt64:
        case Type::tFloat:
        case Type::tDouble:
            return true;
        default:;
        }
        return false;
    }

    bool TypeDecl::isNumericStorage() const {
        if (dim.size() != 0) return false;
        switch (baseType) {
        case Type::tInt8:
        case Type::tUInt8:
        case Type::tInt16:
        case Type::tUInt16:
            return true;
        default:;
        }
        return false;
    }

    bool TypeDecl::isNumericComparable() const {
        if (dim.size() != 0) return false;
        switch (baseType) {
        case Type::tInt:
        case Type::tUInt:
        case Type::tBitfield:
        case Type::tInt64:
        case Type::tUInt64:
        case Type::tFloat:
        case Type::tDouble:
            return true;
        default:;
        }
        return false;
    }

    bool TypeDecl::isIndex() const {
        return (baseType==Type::tInt || baseType==Type::tUInt) && dim.size()==0;
    }

    int TypeDecl::getTupleFieldOffset ( int index ) const {
        DAS_ASSERT(baseType==Type::tTuple);
        DAS_ASSERT(index>=0 && index<int(argTypes.size()));
        int size = 0, idx = 0;
        for ( const auto & argT : argTypes ) {
            int al = argT->getAlignOf() - 1;
            size = (size + al) & ~al;
            if ( idx==index ) {
                return size;
            }
            size += argT->getSizeOf();
            idx ++;
        }
        DAS_ASSERT(0 && "we should not even be here. field index out of range somehow???");
        return -1;
    }

    int TypeDecl::getTupleSize() const {
        uint64_t size = getTupleSize64();
        DAS_ASSERTF(size<=0x7fffffff,"tuple size is too big %lu",(unsigned long)size);
        return (int) (size<=0x7fffffff ? size : 1);
    }

    uint64_t TypeDecl::getTupleSize64() const {
        DAS_ASSERT(baseType==Type::tTuple);
        uint64_t size = 0;
        for ( const auto & argT : argTypes ) {
            int al = argT->getAlignOf() - 1;
            size = (size + al) & ~al;
            size += argT->getSizeOf64();
        }
        int al = getTupleAlign() - 1;
        size = (size + al) & ~al;
        return size;
    }

    int TypeDecl::getTupleAlign() const {
        DAS_ASSERT(baseType==Type::tTuple);
        int align = 1;
        for ( const auto & argT : argTypes ) {
            align = das::max ( argT->getAlignOf(), align );
        }
        return align;
    }

    int TypeDecl::getVariantFieldOffset ( int index ) const {
        index;
        DAS_ASSERT(baseType==Type::tVariant);
        DAS_ASSERT(index>=0 && index<int(argTypes.size()));
        int al = getVariantAlign() - 1;
        int offset = (getTypeBaseSize(Type::tInt) + al) & ~al;
        return offset;
    }

    int TypeDecl::getVariantUniqueFieldIndex ( const TypeDeclPtr & uniqueType ) const {
        int index = -1;
        int idx = 0;
        for ( const auto & argT : argTypes ) {
            if ( argT->isSameType(*uniqueType, RefMatters::no, ConstMatters::no, TemporaryMatters::no, AllowSubstitute::yes) ) {
                if ( index != -1 ) {
                    return -1;
                } else {
                    index = idx;
                }
            }
            idx ++;
        }
        return index;
    }

    int TypeDecl::getVariantSize() const {
        uint64_t size = getVariantSize64();
        DAS_ASSERTF(size<=0x7fffffff,"variant size is too big %lu",(unsigned long)size);
        return (int) (size<=0x7fffffff ? size : 1);
    }

    uint64_t TypeDecl::getVariantSize64() const {
        DAS_ASSERT(baseType==Type::tVariant);
        uint64_t maxSize = 0;
        int al = getVariantAlign() - 1;
        for ( const auto & argT : argTypes ) {
            uint64_t size = (getTypeBaseSize(Type::tInt) + al) & ~al;
            size += argT->getSizeOf64();
            maxSize = das::max(size, maxSize);
        }
        maxSize = (maxSize + al) & ~al;
        return maxSize;
    }

    int TypeDecl::getVariantAlign() const {
        DAS_ASSERT(baseType==Type::tVariant);
        int align = getTypeBaseAlign(Type::tInt);
        for ( const auto & argT : argTypes ) {
            align = das::max ( argT->getAlignOf(), align );
        }
        return align;
    }

    int TypeDecl::getVectorFieldOffset ( int index ) const {
        DAS_ASSERT(index>=0 && index<=3);
        switch ( baseType ) {
            case Type::tRange64:
            case Type::tURange64:
                return index * 8;
            case Type::tInt2:
            case Type::tUInt2:
            case Type::tInt3:
            case Type::tUInt3:
            case Type::tInt4:
            case Type::tUInt4:
            case Type::tFloat2:
            case Type::tFloat3:
            case Type::tFloat4:
            case Type::tRange:
            case Type::tURange:
                return index * 4;
            default:
                return -1;
        }
    }

    int TypeDecl::getBaseSizeOf() const {
        uint64_t size = getBaseSizeOf64();
        DAS_ASSERTF(size<=0x7fffffff,"base size %" PRIu64 " is too big", size);
        return int(size<=0x7fffffff ? size : 1);
    }

    uint64_t TypeDecl::getBaseSizeOf64() const {
        if ( baseType==Type::tHandle ) {
            return annotation->getSizeOf();
        } else if ( baseType==Type::tStructure ) {
            return !structType->circular ? structType->getSizeOf64() : 0;
        } else if ( baseType==Type::tTuple ) {
            return getTupleSize64();
        } else if ( baseType==Type::tVariant ) {
            return getVariantSize64();
        } else if ( isEnumT() ) {
            return enumType ? getTypeBaseSize(enumType->baseType) : getTypeBaseSize(Type::tInt);
        } else {
           return getTypeBaseSize(baseType);
        }
    }

    int TypeDecl::getAlignOf() const {
        if ( baseType==Type::tHandle ) {
            return int(annotation->getAlignOf());
        } else if ( baseType==Type::tStructure ) {
            return !structType->circular ? structType->getAlignOf() : 0;
        } else if ( baseType==Type::tTuple ) {
            return getTupleAlign();
        } else if ( baseType==Type::tVariant ) {
            return getVariantAlign();
        } else if ( isEnumT() ) {
            return enumType ? getTypeBaseAlign(enumType->baseType) : getTypeBaseAlign(Type::tInt);
        } else {
            return getTypeBaseAlign(baseType);
        }
    }

    int TypeDecl::getCountOf() const {
        uint64_t count = getCountOf64();
        DAS_ASSERTF(count <= 0x7fffffff, "count too big %lu", (unsigned long)count);
        return int(count <= 0x7fffffff ? count : 1);
    }

    uint64_t TypeDecl::getCountOf64() const {
        uint64_t size = 1;
        for ( auto i : dim ) {
            size *= i;
        }
        return size;
    }

    int TypeDecl::getSizeOf() const {
        uint64_t size = getSizeOf64();
        DAS_ASSERTF(size <= 0x7fffffff, "size too big %lu", (unsigned long)size);
        return int(size <= 0x7fffffff ? size : 1);
    }

    uint64_t TypeDecl::getSizeOf64() const {
        return getBaseSizeOf64() * getCountOf64();
    }

    int TypeDecl::getStride() const {
        uint64_t stride = getStride64();
        DAS_ASSERTF(stride <= 0x7fffffff, "stride too big %lu", (unsigned long)stride);
        return int(stride <= 0x7fffffff ? stride : 1);
    }

    uint64_t TypeDecl::getStride64() const {
        uint64_t size = 1;
        if ( dim.size() > 1 ) {
            for ( size_t i=1, is=dim.size(); i!=is; ++i ) {
                size *= dim[i];
            }
        }
        return getBaseSizeOf64() * size;
    }

    int TypeDecl::findArgumentIndex( const string & name ) const {
        for (int index=0, indexs=int(argNames.size()); index!=indexs; ++index) {
            if (argNames[index] == name) return index;
        }
        return -1;
    }

    string TypeDecl::findBitfieldName ( uint32_t val ) const {
       if ( argNames.size() ) {
            if ( val && (val & (val-1))==0 ) {  // if bit is set, and only one bit
                int index = 31 - das_clz(val);
                if ( index < int(argNames.size()) ) {
                    return argNames[index];
                }
            }
        }
        return "";
    }

    bool isCircularType ( const TypeDeclPtr & type, vector<const TypeDecl *> & all ) {
        if ( type->baseType==Type::tPointer ) {
            return false;
        }
        if ( type->firstType && isCircularType(type->firstType, all) ) return true;
        if ( type->secondType && isCircularType(type->secondType, all) ) return true;
        auto pt = type.get();
        for (auto it : all) {
            if ( it == pt ) return true;
        }
        all.push_back(pt);
        if ( type->baseType==Type::tStructure ) {
            if ( type->structType ) {
                for ( auto & fd : type->structType->fields ) {
                    if ( isCircularType(fd.type, all) ) return true;
                }
            }
        }  else if ( type->baseType==Type::tTuple || type->baseType==Type::tVariant || type->baseType == Type::option ) {
            for ( auto & arg : type->argTypes ) {
                if ( isCircularType(arg, all) ) return true;
            }
        }
        all.pop_back();
        return false;
    }

    bool isCircularType ( const TypeDeclPtr & type ) {
        vector<const TypeDecl *> all;
        return isCircularType(type, all);
    }

    void TypeDecl::addVariant(const string & name, const TypeDeclPtr & tt) {
        DAS_ASSERT(find(argNames.begin(), argNames.end(), name) == argNames.end() && "duplicate variant");
        argNames.push_back(name);
        argTypes.push_back(tt);
    }

    string TypeDecl::getMangledName ( bool fullName ) const {
        FixedBufferTextWriter ss;
        getMangledName(ss, fullName);
        return ss.str();
    }

    void TypeDecl::getMangledName ( FixedBufferTextWriter & ss, bool fullName ) const {
        if ( constant )     ss << "C";
        if ( ref )          ss << "&";
        if ( temporary )    ss << "#";
        if ( implicit )     ss << "I";
        if ( explicitConst )ss << "=";
        if ( explicitRef )  ss << "R";
        if ( isExplicit )   ss << "X";
        if ( aotAlias )     ss << "F";
        if ( dim.size() ) {
            for ( auto d : dim ) {
                ss << "[" << d << "]";
            }
        }
        if ( fullName ) {
            if ( removeDim )        ss << "-[]";
            if ( removeConstant )   ss << "-C";
            if ( removeRef )        ss << "-&";
            if ( removeTemporary )  ss << "-#";
        }
        if ( !alias.empty() ) {
            ss << "Y<" << alias << ">";
        }
        if ( argNames.size() ) {
            ss << "N<";
            bool first = true;
            for ( auto & arg : argNames ) {
                if ( first ) first = false; else ss << ";";
                ss << arg;
            }
            ss << ">";
        }
        if ( argTypes.size() ) {
            ss << "0<";
            bool first = true;
            for ( auto & arg : argTypes ) {
                if ( first ) first = false; else ss << ";";
                arg->getMangledName(ss, fullName);
            }
            ss << ">";
        }
        if ( firstType ) {
            ss << "1<";
            firstType->getMangledName(ss, fullName);
            ss << ">";
        }
        if ( secondType ) {
            ss << "2<";
            secondType->getMangledName(ss, fullName);
            ss << ">";
        }
        if ( baseType==Type::tHandle ) {
            ss << "H<";
            if ( !annotation->module->name.empty() ) {
                ss << annotation->module->name << "::";
             }
             ss << annotation->name << ">";
        } else if ( baseType==Type::tStructure ) {
            ss << "S<";
            if ( structType->module && !structType->module->name.empty() ) {
                ss << structType->module->name << "::";
            }
            ss << structType->name << ">";
        } else if ( baseType==Type::tEnumeration || baseType==Type::tEnumeration8 || baseType==Type::tEnumeration16 ) {
            ss << "E";
            if ( baseType==Type::tEnumeration8 ) ss << "8";
            else if ( baseType==Type::tEnumeration16 ) ss << "16";
            if ( enumType ) {
                ss << "<" << enumType->getMangledName() << ">";
            }
        } else if ( baseType==Type::tPointer ) {
            ss << "?";
            if ( smartPtr ) {
                ss << (smartPtrNative ? "W" : "M");
            }
        } else {
            switch ( baseType ) {
                case Type::anyArgument:     ss << "*"; break;
                case Type::fakeContext:     ss << "_c"; break;
                case Type::fakeLineInfo:    ss << "_l"; break;
                case Type::autoinfer:       ss << "."; break;
                case Type::option:          ss << "|"; break;
                case Type::alias:           ss << "L"; break;
                case Type::tIterator:       ss << "G"; break;
                case Type::tArray:          ss << "A"; break;
                case Type::tTable:          ss << "T"; break;
                case Type::tBlock:          ss << "$"; break;
                case Type::tFunction:       ss << "@@"; break;
                case Type::tLambda:         ss << "@"; break;
                case Type::tTuple:          ss << "U"; break;
                case Type::tVariant:        ss << "V"; break;
                case Type::tBitfield:       ss << "t"; break;
                case Type::tInt:            ss << "i"; break;
                case Type::tInt2:           ss << "i2"; break;
                case Type::tInt3:           ss << "i3"; break;
                case Type::tInt4:           ss << "i4"; break;
                case Type::tInt8:           ss << "i8"; break;
                case Type::tInt16:          ss << "i16"; break;
                case Type::tInt64:          ss << "i64"; break;
                case Type::tUInt:           ss << "u"; break;
                case Type::tUInt2:          ss << "u2"; break;
                case Type::tUInt3:          ss << "u3"; break;
                case Type::tUInt4:          ss << "u4"; break;
                case Type::tUInt8:          ss << "u8"; break;
                case Type::tUInt16:         ss << "u16"; break;
                case Type::tUInt64:         ss << "u64"; break;
                case Type::tFloat:          ss << "f"; break;
                case Type::tFloat2:         ss << "f2"; break;
                case Type::tFloat3:         ss << "f3"; break;
                case Type::tFloat4:         ss << "f4"; break;
                case Type::tRange:          ss << "r"; break;
                case Type::tURange:         ss << "z"; break;
                case Type::tRange64:        ss << "r64"; break;
                case Type::tURange64:       ss << "z64"; break;
                case Type::tDouble:         ss << "d"; break;
                case Type::tString:         ss << "s"; break;
                case Type::tVoid:           ss << "v"; break;
                case Type::tBool:           ss << "b"; break;
                default:
                    LOG(LogLevel::error) << das_to_string(baseType) << "\n";
                    DAS_ASSERT(0 && "we should not be here");
                    break;
            }
        }
    }

    void append ( TypeAliasMap & aliases, const TypeDeclPtr & td, bool viaPointer ) {
        auto mname = td->getMangledName();
        auto & aMain = aliases[mname];
        aMain.first = td;
        aMain.second |= viaPointer;
        if ( td->isBaseVectorType() ) {
            auto bt = make_smart<TypeDecl>(td->getVectorBaseType());
            auto & aSub = aliases[bt->getMangledName()];
            aSub.first = bt;
            aSub.second |= viaPointer;
        }
    }

    void TypeDecl::collectAliasing ( TypeAliasMap & aliases, das_set<Structure *> & dep, bool viaPointer ) const {
        append(aliases, (TypeDecl *) this, viaPointer);
        if ( temporary ) return;    // temporary types never alias
        if ( baseType==Type::tArray ) {
            if ( firstType  ) {
                firstType->collectAliasing(aliases, dep, viaPointer);
            }
        } else if ( baseType==Type::tTable ) {
            if ( secondType ) {
                secondType->collectAliasing(aliases, dep, viaPointer);
            }
        } else if ( baseType==Type::tStructure ) {
            if ( structType ) {
                if (dep.find(structType) != dep.end()) return;
                dep.insert(structType);
                for ( auto & fld : structType->fields ) {
                    fld.type->collectAliasing(aliases, dep, viaPointer);
                }
            }
        } else if ( baseType==Type::tTuple || baseType==Type::tVariant ) {
            for ( auto & argT : argTypes ) {
                argT->collectAliasing(aliases, dep, viaPointer);
            }
        } else if ( baseType==Type::tPointer ) {
            if ( firstType ) {
                firstType->collectAliasing(aliases, dep, true);
            }
        }
    }

    void TypeDecl::collectContainerAliasing ( TypeAliasMap & aliases, das_set<Structure *> & dep, bool viaPointer ) const {
        if ( constant ) return;
        if ( baseType==Type::tArray ) {
            if ( firstType && !firstType->constant ) {
                append(aliases, firstType, viaPointer);
                firstType->collectContainerAliasing(aliases, dep, viaPointer);
            }
        } else if ( baseType==Type::tTable ) {
            if ( secondType && !secondType->constant ) {
                append(aliases, secondType, viaPointer);
                secondType->collectContainerAliasing(aliases, dep, viaPointer);
            }
        } else if ( baseType==Type::tStructure ) {
            if ( structType ) {
                if (dep.find(structType) != dep.end()) return;
                dep.insert(structType);
                for ( auto & fld : structType->fields ) {
                    if ( !fld.type->constant ) {
                        fld.type->collectContainerAliasing(aliases, dep, viaPointer);
                    }
                }
            }
        } else if ( baseType==Type::tTuple || baseType==Type::tVariant ) {
            for ( auto & argT : argTypes ) {
                argT->collectContainerAliasing(aliases, dep, viaPointer);
            }
        } else if ( baseType==Type::tPointer ) {
            if ( firstType ) {
                firstType->collectContainerAliasing(aliases, dep, viaPointer);
            }
        }
    }

    int TypeDecl::tupleFieldIndex( const string & name ) const {
        int index = 0;
        if ( sscanf(name.c_str(),"_%i",&index)==1 ) {
            return index;
        } else {
            auto vT = isPointer() ? firstType.get() : this;
            if (!vT) return -1;
            if ( name=="_first" ) {
                return 0;
            } else if ( name=="_last" ) {
                return int(vT->argTypes.size())-1;
            } else {
                return vT->findArgumentIndex(name);
            }
            return -1;
        }
    }

    int TypeDecl::variantFieldIndex ( const string & name ) const {
        auto vT = isPointer() ? firstType.get() : this;
        if (!vT) return -1;
        return vT->findArgumentIndex(name);
    }

    // Mangled name parser

    void MangledNameParser::error ( const string &, const char * ) {
        DAS_VERIFY(0 && "invalid mangled name");
    }

    string MangledNameParser::parseAnyName ( const char * & ch, bool allowModule ) {
        string name;
        while ( isalnum(*ch) || *ch=='_' || *ch=='`' || (*ch==':' && allowModule) || (*ch=='$' && allowModule) ) {
            char che[2] = { *ch, 0 };
            name.append(che);
            ch ++;
        }
        return name;
    }

    string MangledNameParser::parseAnyNameInBrackets ( const char * & ch, bool allowModule ) {
        if ( *ch!='<' ) error("expecting '<'", ch);
        ch ++;
        auto name = parseAnyName(ch, allowModule);
        if ( *ch!='>' ) error("expecting '>'", ch);
        ch ++;
        return name;
    }

    TypeDeclPtr MangledNameParser::parseTypeFromMangledName ( const char * & ch, const ModuleLibrary & library, Module * thisModule ) {
        switch ( *ch ) {
            case '[': {
                ch ++;
                string numT = "";
                bool neg = false;
                if ( *ch=='-' ) {
                    ch ++;
                    neg = true;
                }
                while ( isdigit(*ch) ) {
                    numT += *ch;
                    ch ++;
                }
                if ( *ch!=']' ) error("expecting ']", ch);
                ch ++;
                int di = atoi(numT.c_str());
                if ( neg ) di = -di;
                auto pt = parseTypeFromMangledName(ch,library,thisModule);
                pt->dim.insert(pt->dim.begin(), di);
                return pt;
            }
            case '1': {
                ch ++;
                if ( *ch!='<' ) error("expecting '<'", ch);
                ch ++;
                auto ft = parseTypeFromMangledName(ch,library,thisModule);
                if ( *ch!='>' ) error("expecting '>'", ch);
                ch ++;
                auto pt = parseTypeFromMangledName(ch,library,thisModule);
                pt->firstType = das::move(ft);
                return pt;
            };
            case '2': {
                ch ++;
                if ( *ch!='<' ) error("expecting '<'", ch);
                ch ++;
                auto ft = parseTypeFromMangledName(ch,library,thisModule);
                if ( *ch!='>' ) error("expecting '>'", ch);
                ch ++;
                auto pt = parseTypeFromMangledName(ch,library,thisModule);
                pt->secondType = das::move(ft);
                return pt;
            };
            case 'H': {
                ch ++;
                auto pt = make_smart<TypeDecl>(Type::tHandle);
                auto annName = parseAnyNameInBrackets(ch,true);
                auto ann = library.findAnnotation(annName,thisModule);
                if ( thisModule && ann.size()==0 ) {
                    if ( auto tann = thisModule->findAnnotation(annName) ) {
                        ann.push_back(tann);
                    }
                }
                if ( ann.size()!=1 ) error("unresolved annotation '" + annName + "'", ch);
                pt->annotation = (TypeAnnotation *) ann.back().get();
                if ( !pt->annotation->rtti_isHandledTypeAnnotation() ) error("'" + annName + "' is not a handled type", ch);
                return pt;
            };
            case 'S': {
                ch ++;
                auto sname = parseAnyNameInBrackets(ch,true);
                auto pt = make_smart<TypeDecl>(Type::tStructure);
                auto stt = library.findStructure(sname, thisModule);
                if ( thisModule && stt.size()==0 ) {
                    if ( auto tstt = thisModule->findStructure(sname) ) {
                        stt.push_back(tstt);
                    }
                }
                if ( stt.size()==1 ) error("unknown structure '" + sname + "'", ch);
                pt->structType = stt.back().get();
                return pt;
            }
            case '0': {
                ch ++;
                vector<TypeDeclPtr> types;
                while ( *ch!=0 && *ch!='>' ) {
                    ch ++;
                    auto tt = parseTypeFromMangledName(ch,library,thisModule);
                    types.push_back(tt);
                    if (*ch!=';' && *ch!='>') error("expecting ';' or '>'", ch);
                }
                if ( *ch!='>' ) error("expecting '>'", ch);
                ch++;
                auto pt = parseTypeFromMangledName(ch,library,thisModule);
                pt->argTypes = das::move(types);
                return pt;
            };
            case 'N': {
                ch ++;
                vector<string> names;
                while ( *ch!=0 && *ch!='>' ) {
                    ch ++;
                    auto name = parseAnyName(ch, false);
                    names.push_back(name);
                }
                if ( *ch!='>' ) error("expecting '>'", ch);
                ch++;
                auto pt = parseTypeFromMangledName(ch,library,thisModule);
                pt->argNames = das::move(names);
                return pt;
            };
            case 'E': {
                ch ++;
                TypeDeclPtr pt;
                if ( *ch=='8' ) {
                    ch ++;
                    pt = make_smart<TypeDecl>(Type::tEnumeration8);
                } else if ( ch[0]=='1' && ch[1]=='6' ) {
                    ch += 2;
                    pt = make_smart<TypeDecl>(Type::tEnumeration16);
                } else {
                    pt = make_smart<TypeDecl>(Type::tEnumeration);
                }
                if ( *ch=='<' ) {
                    auto sname = parseAnyNameInBrackets(ch,true);
                    auto stt = library.findEnum(sname, thisModule);
                    if ( thisModule && stt.size()==0 ) {
                        if ( auto tstt = thisModule->findEnum(sname) ) {
                            stt.push_back(tstt);
                        }
                    }
                    if ( stt.size()!=1 ) error("unresolved enumeration '" + sname + "'", ch);
                    pt->enumType = stt.back().get();
                }
                return pt;
            }
            case 'Y': {
                ch ++;
                auto aname = parseAnyNameInBrackets(ch,false);
                auto pt = parseTypeFromMangledName(ch,library,thisModule);
                pt->alias = aname;
                return pt;
            };
            case '_': {
                        if ( ch[1]=='c' )                   { ch+=2; return make_smart<TypeDecl>(Type::fakeContext); }
                else    if ( ch[1]=='l' )                   { ch+=2; return make_smart<TypeDecl>(Type::fakeLineInfo); }
                else                                        { error("unsupported mangled name format - expecting fake...", ch); return make_smart<TypeDecl>(); }
            }
            case 'i': {
                        if ( ch[1]=='2' )                   { ch+=2; return make_smart<TypeDecl>(Type::tInt2); }
                else    if ( ch[1]=='3' )                   { ch+=2; return make_smart<TypeDecl>(Type::tInt3); }
                else    if ( ch[1]=='4' )                   { ch+=2; return make_smart<TypeDecl>(Type::tInt4); }
                else    if ( ch[1]=='8' )                   { ch+=2; return make_smart<TypeDecl>(Type::tInt8); }
                else    if ( ch[1]=='1' && ch[2]=='6' )     { ch+=3; return make_smart<TypeDecl>(Type::tInt16); }
                else    if ( ch[1]=='6' && ch[2]=='4' )     { ch+=3; return make_smart<TypeDecl>(Type::tInt64); }
                else                                        { ch+=1; return make_smart<TypeDecl>(Type::tInt); }
            }
            case 'u': {
                        if ( ch[1]=='2' )                   { ch+=2; return make_smart<TypeDecl>(Type::tUInt2); }
                else    if ( ch[1]=='3' )                   { ch+=2; return make_smart<TypeDecl>(Type::tUInt3); }
                else    if ( ch[1]=='4' )                   { ch+=2; return make_smart<TypeDecl>(Type::tUInt4); }
                else    if ( ch[1]=='8' )                   { ch+=2; return make_smart<TypeDecl>(Type::tUInt8); }
                else    if ( ch[1]=='1' && ch[2]=='6' )     { ch+=3; return make_smart<TypeDecl>(Type::tUInt16); }
                else    if ( ch[1]=='6' && ch[2]=='4' )     { ch+=3; return make_smart<TypeDecl>(Type::tUInt64); }
                else                                        { ch+=1; return make_smart<TypeDecl>(Type::tUInt); }
            }
            case 'f': {
                        if ( ch[1]=='2' )                   { ch+=2; return make_smart<TypeDecl>(Type::tFloat2); }
                else    if ( ch[1]=='3' )                   { ch+=2; return make_smart<TypeDecl>(Type::tFloat3); }
                else    if ( ch[1]=='4' )                   { ch+=2; return make_smart<TypeDecl>(Type::tFloat4); }
                else                                        { ch+=1; return make_smart<TypeDecl>(Type::tFloat); }
            case '.':   ch++; return make_smart<TypeDecl>(Type::autoinfer);
            case '|':   ch++; return make_smart<TypeDecl>(Type::option);
            case '*':   ch++; return make_smart<TypeDecl>(Type::anyArgument);
            case 'L':   ch++; return make_smart<TypeDecl>(Type::alias);
            case 'A':   ch++; return make_smart<TypeDecl>(Type::tArray);
            case 'T':   ch++; return make_smart<TypeDecl>(Type::tTable);
            case 'G':   ch++; return make_smart<TypeDecl>(Type::tIterator);
            case '$':   ch++; return make_smart<TypeDecl>(Type::tBlock);
            case 'U':   ch++; return make_smart<TypeDecl>(Type::tTuple);
            case 'V':   ch++; return make_smart<TypeDecl>(Type::tVariant);
            case 't':   ch++; return make_smart<TypeDecl>(Type::tBitfield);
            case 'r':   {
                        if ( ch[1]=='6' && ch[2]=='4' )     { ch+=3; return make_smart<TypeDecl>(Type::tRange64); }
                else                                        { ch+=1; return make_smart<TypeDecl>(Type::tRange); }
            }
            case 'z':   {
                        if ( ch[1]=='6' && ch[2]=='4' )     { ch+=3; return make_smart<TypeDecl>(Type::tURange64); }
                else                                        { ch+=1; return make_smart<TypeDecl>(Type::tURange); }
            }
            case 'd':   ch++; return make_smart<TypeDecl>(Type::tDouble);
            case 's':   ch++; return make_smart<TypeDecl>(Type::tString);
            case 'v':   ch++; return make_smart<TypeDecl>(Type::tVoid);
            case 'b':   ch++; return make_smart<TypeDecl>(Type::tBool);
            case '?':   {
                ch++;
                auto pt = make_smart<TypeDecl>(Type::tPointer);
                if ( *ch=='M' ) {
                    pt->smartPtr = true;
                    pt->smartPtrNative = false;
                    ch ++;
                } else if ( *ch=='W' ) {
                    pt->smartPtr = true;
                    pt->smartPtrNative = true;
                    ch ++;
                }
                return pt;
            };
            case '@':   {
                if ( ch[1]=='@' ) {
                    ch+=2;
                    return make_smart<TypeDecl>(Type::tFunction);
                } else {
                    ch++;
                    return make_smart<TypeDecl>(Type::tLambda);
                }
            };
            case '&': {
                ch ++;
                auto pt = parseTypeFromMangledName(ch,library,thisModule);
                pt->ref = true;
                return pt;
            };
            case '#': {
                ch ++;
                auto pt = parseTypeFromMangledName(ch,library,thisModule);
                pt->temporary = true;
                return pt;
            };
            case 'C': {
                ch ++;
                auto pt = parseTypeFromMangledName(ch,library,thisModule);
                pt->constant = true;
                return pt;
            };
            case '-': {
                switch ( ch[1] ) {
                case '&': {
                    ch += 2;
                    auto pt = parseTypeFromMangledName(ch,library,thisModule);
                    pt->ref = false;
                    pt->removeRef = true;
                    return pt;
                }
                case '#': {
                    ch += 2;
                    auto pt = parseTypeFromMangledName(ch,library,thisModule);
                    pt->temporary = false;
                    pt->removeTemporary = true;
                    return pt;
                }
                case 'C': {
                    ch += 2;
                    auto pt = parseTypeFromMangledName(ch,library,thisModule);
                    pt->constant = false;
                    pt->removeConstant = true;
                    return pt;
                }
                case '[': {
                    if ( ch[2]!=']' ) error("expecting '-[]'", ch);
                    ch += 3;
                    auto pt = parseTypeFromMangledName(ch,library,thisModule);
                    pt->removeDim = true;
                    return pt;
                }
                }
                error("unsupported mangled name format - expecting remove trait", ch);
                return make_smart<TypeDecl>();
            };
            case 'I': {
                ch ++;
                auto pt = parseTypeFromMangledName(ch,library,thisModule);
                pt->implicit = true;
                return pt;
            };
            case 'X': {
                ch ++;
                auto pt = parseTypeFromMangledName(ch,library,thisModule);
                pt->isExplicit = true;
                return pt;
            };
            case '=': {
                ch ++;
                auto pt = parseTypeFromMangledName(ch,library,thisModule);
                pt->explicitConst = true;
                return pt;
            };
            case 'R': {
                ch ++;
                auto pt = parseTypeFromMangledName(ch,library,thisModule);
                pt->explicitRef = true;
                return pt;
            };
            case 'F': {
                ch ++;
                auto pt = parseTypeFromMangledName(ch,library,thisModule);
                pt->aotAlias = true;
                return pt;
            };
            }
        }
        error("unsupported mangled name format symbol", ch);
        return make_smart<TypeDecl>();
    }

    TypeDeclPtr parseTypeFromMangledName ( const char * & ch, const ModuleLibrary & library, Module * thisModule ) {
        MangledNameParser parser;
        return parser.parseTypeFromMangledName(ch, library, thisModule);
    }

    bool hasImplicit ( const TypeDeclPtr & type ) {
        if ( type->implicit ) return true;
        if ( type->firstType && hasImplicit(type->firstType) ) return true;
        if ( type->secondType && hasImplicit(type->secondType) ) return true;
        for ( auto & argT : type->argTypes ) {
            if ( hasImplicit(argT) ) return true;
        }
        return false;
    }

    bool isMatchingArgumentType (TypeDeclPtr argType, TypeDeclPtr passType) {
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
}
