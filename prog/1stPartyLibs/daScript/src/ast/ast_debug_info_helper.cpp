#include "daScript/misc/platform.h"

#include "daScript/ast/ast.h"
#include "daScript/ast/ast_expressions.h"
#include "daScript/ast/ast_visitor.h"
#include "daScript/simulate/hash.h"

namespace das {

    class CollectLocalVariables : public Visitor {
    public:
        virtual bool canVisitStructureFieldInit ( Structure * ) override { return false; }
        virtual bool canVisitArgumentInit ( Function * , const VariablePtr &, Expression * ) override { return false; }
        virtual bool canVisitQuoteSubexpression ( ExprQuote * ) override { return false; }
        virtual void preVisit ( ExprLet * expr ) override {
            for ( const auto & var : expr->variables) {
                locals.push_back(make_tuple(var,expr->visibility,bool(var->type->ref)));
            }
        }
        virtual void preVisitFor ( ExprFor * expr, const VariablePtr & var, bool ) override {
            locals.push_back(make_tuple(var,expr->visibility,bool(var->type->ref)));
        }
    public:
        vector<tuple<VariablePtr,LineInfo,bool>> locals;
    };

    void DebugInfoHelper::logMemInfo ( TextWriter & tw ) {
        tw << "DEBUG INFO IS:\n";
        tw << "\tStructInfo " << (int(smn2s.size() * sizeof(StructInfo))) << " = " << int(smn2s.size()) << " x " << int(sizeof(StructInfo)) << "\n";
        tw << "\tTypeInfo " << (int(tmn2t.size()*sizeof(TypeInfo))) <<  " = " << int(tmn2t.size()) << " x " << int(sizeof(TypeInfo)) << "\n";
        tw << "\tVarInfo " << (int(vmn2v.size()*sizeof(VarInfo))) << " = " << int(vmn2v.size()) << " x " << int(sizeof(VarInfo)) << "\n";
        tw << "\tFuncInfo " << (int(fmn2f.size()*sizeof(FuncInfo))) << " = " << int(fmn2f.size()) << " x " << int(sizeof(FuncInfo)) << "\n";
        tw << "\tEnumInfo " << (int(emn2e.size()*sizeof(EnumInfo))) << " = " << int(emn2e.size()) << " x " << int(sizeof(EnumInfo)) << "\n";
        tw << "\tSTRINGS " << debugInfo->stringBytes << "\n";
        tw << "TOTAL " << debugInfo->bytesAllocated() << "\n";
    }

    void DebugInfoHelper::appendGlobalVariables ( FuncInfo * info, const FunctionPtr & body ) {
        info->globalCount = uint32_t(body->useGlobalVariables.size());
        info->globals = (VarInfo **) debugInfo->allocate(sizeof(VarInfo *) * info->globalCount);
        uint32_t i = 0;
        for ( auto var : body->useGlobalVariables ) {
            info->globals[i] = makeVariableDebugInfo(*var);
            i ++;
        }
    }

    void DebugInfoHelper::appendLocalVariables ( FuncInfo * info, ExpressionPtr body ) {
        CollectLocalVariables lv;
        body->visit(lv);
        info->localCount = uint32_t(lv.locals.size());
        info->locals = (LocalVariableInfo **) debugInfo->allocate(sizeof(VarInfo *) * info->localCount);
        uint32_t i = 0;
        for ( auto & var_vis : lv.locals ) {
            auto var = get<0>(var_vis);
            LocalVariableInfo * lvar = (LocalVariableInfo *) debugInfo->allocate(sizeof(LocalVariableInfo));
            info->locals[i] = lvar;
            makeTypeInfo(lvar, var->type);
            if ( get<2>(var_vis) ) lvar->flags |= TypeInfo::flag_ref;
            lvar->name = debugInfo->allocateCachedName(var->name);
            lvar->stackTop = var->stackTop;
            lvar->visibility = get<1>(var_vis);
            lvar->localFlags = 0;
            lvar->cmres = var->aliasCMRES;
            i ++ ;
        }
    }

    AnnotationArgumentInfo * DebugInfoHelper::makeAnnotationArguments ( const AnnotationArgumentList & list, uint32_t & count ) {
        count = 0;
        if ( list.empty() ) return nullptr;
        auto args = (AnnotationArgumentInfo *) debugInfo->allocate(uint32_t(sizeof(AnnotationArgumentInfo) * list.size()));
        for ( const auto & arg : list ) {
            switch ( arg.type ) {
            case Type::tBool: case Type::tInt: case Type::tFloat: case Type::tString:
                break;
            default:
                continue;   // nested aList args only exist during parsing
            }
            auto & ai = args[count++];
            ai.type = arg.type;
            ai.name = debugInfo->allocateCachedName(arg.name);
            ai.sValue = arg.type==Type::tString ? debugInfo->allocateCachedName(arg.sValue) : nullptr;
            ai.iValue = arg.iValue; // raw union copy covers bool/int/float
        }
        return count ? args : nullptr;
    }

    AnnotationInfo * DebugInfoHelper::makeAnnotationList ( const AnnotationList & list, uint32_t & count ) {
        count = 0;
        if ( list.empty() ) return nullptr;
        auto anns = (AnnotationInfo *) debugInfo->allocate(uint32_t(sizeof(AnnotationInfo) * list.size()));
        for ( const auto & decl : list ) {
            if ( !decl->annotation ) continue;
            auto & ai = anns[count++];
            ai.name = debugInfo->allocateCachedName(decl->annotation->name);
            ai.module_name = debugInfo->allocateCachedName(decl->annotation->module ? decl->annotation->module->name : "");
            ai.arguments = makeAnnotationArguments(decl->arguments, ai.count);
            ai.resolved = nullptr;
        }
        return count ? anns : nullptr;
    }

    EnumInfo * DebugInfoHelper::makeEnumDebugInfo ( const Enumeration & en ) {
        auto mangledName = en.getMangledName();
        auto it = emn2e.find(mangledName);
        if ( it!=emn2e.end() ) return it->second;
        EnumInfo * eni = debugInfo->makeNode<EnumInfo>();
        eni->name = debugInfo->allocateCachedName(en.name);
        eni->module_name = debugInfo->allocateCachedName(en.module->name);
        eni->count = uint32_t(en.list.size());
        eni->flags = 0;
        if ( en.baseType==Type::tUInt8 || en.baseType==Type::tUInt16
                || en.baseType==Type::tUInt || en.baseType==Type::tUInt64 ) {
            eni->flags |= EnumInfo::flag_unsigned;
        }
        eni->fields = (EnumValueInfo **) debugInfo->allocate(sizeof(EnumValueInfo *) * eni->count);
        uint32_t i = 0;
        for ( auto & ev : en.list ) {
            eni->fields[i] = (EnumValueInfo *) debugInfo->allocate(sizeof(EnumValueInfo));
            eni->fields[i]->name = debugInfo->allocateCachedName(ev.name);
            eni->fields[i]->value = !ev.value ? -1 : getConstExprIntOrUInt(ev.value);
            i ++;
        }
        eni->annotations = makeAnnotationList(en.annotations, eni->annotation_count);
        eni->hash = hash_blockz64((uint8_t *)mangledName.c_str());
        emn2e[mangledName] = eni;
        return eni;
    }

    FuncInfo * DebugInfoHelper::makeFunctionDebugInfo ( const Function & fn ) {
        string mangledName = fn.getMangledName();
        auto it = fmn2f.find(mangledName);
        if ( it!=fmn2f.end() ) return it->second;
        FuncInfo * fni = debugInfo->makeNode<FuncInfo>();
        fni->name = debugInfo->allocateCachedName(fn.name);
        if ( fn.builtIn ) {
            auto bfn = (BuiltInFunction *) &fn;
            fni->cppName = debugInfo->allocateCachedName(bfn->cppName);
        } else {
            fni->cppName = nullptr;
        }
        fni->stackSize = fn.totalStackSize;
        fni->count = (uint32_t) fn.arguments.size();
        fni->fields = (VarInfo **) debugInfo->allocate(sizeof(VarInfo *) * fni->count);
        for ( uint32_t i=0, is=fni->count; i!=is; ++i ) {
            fni->fields[i] = makeVariableDebugInfo(*fn.arguments[i]);
        }
        fni->flags = 0;
        if ( fn.init || fn.macroInit ) {
            fni->flags |= FuncInfo::flag_init;
            if ( fn.lateInit ) fni->flags |= FuncInfo::flag_late_init;
        }
        if ( fn.shutdown ) {
            fni->flags |= FuncInfo::flag_shutdown;
            if ( fn.lateShutdown ) fni->flags |= FuncInfo::flag_late_shutdown;
        }
        if ( fn.builtIn ) fni->flags |= FuncInfo::flag_builtin;
        if ( fn.privateFunction ) fni->flags |= FuncInfo::flag_private;
        fni->result = makeTypeInfo(nullptr, fn.result);
        fni->locals = nullptr;
        fni->localCount = 0;
        fni->annotations = makeAnnotationList(fn.annotations, fni->annotation_count);
        fni->hash = hash_blockz64((uint8_t *)mangledName.c_str());
        fmn2f[mangledName] = fni;
        return fni;
    }

    FuncInfo * DebugInfoHelper::makeInvokeableTypeDebugInfo ( const TypeDeclPtr & blk, const LineInfo & at ) {
        gc_local<Function> fakeFuncPtr(new Function());
        Function & fakeFunc = *fakeFuncPtr;
        fakeFunc.name = "invoke " + blk->describe();
        fakeFunc.at = at;
        gc_local<TypeDecl> voidType(new TypeDecl(Type::tVoid));
        fakeFunc.result = blk->firstType ? blk->firstType : (TypeDecl*)voidType;
        fakeFunc.totalStackSize = sizeof(Prologue);
        for ( size_t ai=0, ais=blk->argTypes.size(); ai!=ais; ++ ai ) {
            auto argV = new Variable();
            argV->at = at;
            argV->name = blk->argNames.empty() ? ("arg_" + to_string(ai)) : blk->argNames[ai];
            argV->type = blk->argTypes[ai];
            fakeFunc.arguments.push_back(argV);
        }
        return makeFunctionDebugInfo(fakeFunc);
    }

    StructInfo * DebugInfoHelper::makeStructureDebugInfo ( const Structure & st ) {
        DAS_VERIFYF(!st.isTemplate,"cannot make debug info for template structure %s", st.name.c_str());
        string mangledName = st.getMangledName();
        auto it = smn2s.find(mangledName);
        if ( it!=smn2s.end() ) return it->second;
        StructInfo * sti = debugInfo->makeNode<StructInfo>();
        smn2s[mangledName] = sti;
        sti->name = debugInfo->allocateCachedName(st.name);
        sti->flags = 0;
        if ( st.isClass ) sti->flags |= StructInfo::flag_class;
        if ( st.isLambda ) sti->flags |= StructInfo::flag_lambda;
        gc_local<TypeDecl> tdecl(new TypeDecl((Structure *)&st));
        auto gcf = tdecl->gcFlags();
        if ( gcf & TypeDecl::gcFlag_heap ) sti->flags |= StructInfo::flag_heapGC;
        if ( gcf & TypeDecl::gcFlag_stringHeap ) sti->flags |= StructInfo::flag_stringHeapGC;
        sti->count = (uint32_t) st.fields.size();
        sti->size = st.getSizeOf();
        s2cppTypeName[sti] = describeCppType(tdecl, CpptSubstitureRef::no,CpptSkipRef::yes, CpptSkipConst::yes);
        sti->fields = (VarInfo **) debugInfo->allocate( sizeof(VarInfo *) * sti->count );
        for ( uint32_t i=0, is=sti->count; i!=is; ++i ) {
            auto & var = st.fields[i];
            VarInfo * vi = makeVariableDebugInfo(st, var);
            if (var.privateField) {
                vi->flags |= TypeInfo::flag_private;
            }
            if (var.classMethod) {
                vi->flags |= TypeInfo::flag_classMethod;
            }
            sti->fields[i] = vi;
        }
        sti->firstGcField = sti->count;
        if ( sti->count && (sti->flags & (StructInfo::flag_heapGC | StructInfo::flag_stringHeapGC))) {
            int32_t prev = -1;
            for ( uint32_t i=0, is=sti->count; i!=is; ++i ) {
                das::VarInfo * var = sti->fields[i];
                if (var->flags & (TypeInfo::flag_heapGC | TypeInfo::flag_stringHeapGC)) {
                    if ( prev == -1 ) {
                        sti->firstGcField = i;
                    } else {
                        sti->fields[prev]->nextGcField = i;
                    }
                    prev = i;
                }
            }
            if (prev != -1)
                sti->fields[prev]->nextGcField = sti->count;
        }
        sti->init_mnh = 0;
        if ( st.module ) {
            sti->module_name = debugInfo->allocateCachedName(st.module->name);
            if ( auto it = st.module->functionsByName.find(hash64z(st.name.c_str())) ) {
                for ( auto * fn : it->second ) {
                    if ( fn->arguments.empty() && fn->result && fn->result->structType == &st ) {
                        sti->init_mnh = fn->getMangledNameHash();
                        break;
                    }
                }
            }
        }
        sti->annotations = makeAnnotationList(st.annotations, sti->annotation_count);
        sti->hash = hash_blockz64((uint8_t *)mangledName.c_str());
        return sti;
    }

    TypeInfo * DebugInfoHelper::makeTypeInfo ( TypeInfo * info, const TypeDeclPtr & type ) {
        // runtime TypeInfo stays flattened forever (FIXED_ARRAY_REWORK.md): chain dims emit
        // straight into info->dim; element fields read through elemType, whole-type semantics
        // (size, quals, gc, names) stay on the chain head
        vector<int32_t> dims;
        const TypeDecl * elemType = type;
        // tDistinct erases to its underlying type here, same boundary as the fixed-array flatten -
        // runtime TypeInfo (interpreter, JIT, fusion, RTTI) never observes a distinct type
        while ( elemType->baseType==Type::tFixedArray || elemType->baseType==Type::tDistinct ) {
            if ( elemType->baseType==Type::tFixedArray ) dims.push_back(elemType->fixedDim);
            DAS_ASSERTF(elemType->firstType, "tFixedArray/tDistinct chain without an element");
            elemType = elemType->firstType;
        }
        if ( info==nullptr ) {
            string mangledName = type->getMangledName();
            auto it = tmn2t.find(mangledName);
            if ( it!=tmn2t.end() ) return it->second;
            info = debugInfo->makeNode<TypeInfo>();
            tmn2t[mangledName] = info;
        }
        info->type = elemType->baseType;
        info->dimSize = (uint32_t) dims.size();
        if ( info->dimSize ) {
            info->dim = (uint32_t *) debugInfo->allocate(sizeof(uint32_t) * info->dimSize );
            for ( uint32_t i=0, is=info->dimSize; i!=is; ++i ) {
                info->dim[i] = dims[i];
            }
        }
        if ( elemType->baseType==Type::tStructure  ) {
            info->structType = makeStructureDebugInfo(*elemType->structType);
        } else if ( elemType->isEnumT() ) {
            info->enumType = elemType->enumType ? makeEnumDebugInfo(*elemType->enumType) : nullptr;
        } else if ( elemType->annotation ) {
            auto ann = (AnnotationInfo *) debugInfo->allocate(sizeof(AnnotationInfo));
            ann->name = debugInfo->allocateCachedName(elemType->annotation->name);
            ann->module_name = debugInfo->allocateCachedName(elemType->annotation->module ? elemType->annotation->module->name : "");
            ann->arguments = nullptr;
            ann->count = 0;
            ann->resolved = nullptr;
            info->annotation_info = ann;
        } else {
            info->annotation_info = nullptr;
        }
        info->flags = 0;
        if (type->ref) {
            info->flags |= TypeInfo::flag_ref;
            info->flags |= TypeInfo::flag_refValue;
        }
        if (type->isRefType()) {
            info->flags |= TypeInfo::flag_refType;
            info->flags &= ~TypeInfo::flag_ref;
        }
        if (type->canCopy())
            info->flags |= TypeInfo::flag_canCopy;
        if (type->isPod())
            info->flags |= TypeInfo::flag_isPod;
        if (type->isConst())
            info->flags |= TypeInfo::flag_isConst;
        if (type->temporary)
            info->flags |= TypeInfo::flag_isTemp;
        if (type->implicit)
            info->flags |= TypeInfo::flag_isImplicit;
        if (type->isRawPod())
            info->flags |= TypeInfo::flag_isRawPod;
        if (elemType->smartPtr) {
            info->flags |= TypeInfo::flag_isSmartPtr;
            if ( elemType->smartPtrNative )
                info->flags |= TypeInfo::flag_isSmartPtrNative;
        }
        auto gcf = type->gcFlags();
        if ( gcf & TypeDecl::gcFlag_heap )
            info->flags |= TypeInfo::flag_heapGC;
        if ( gcf & TypeDecl::gcFlag_stringHeap )
            info->flags |= TypeInfo::flag_stringHeapGC;
        if ( elemType->firstType ) {
            info->firstType = makeTypeInfo(nullptr, elemType->firstType);
        } else if ( elemType->baseType==Type::tStructure && elemType->structType->parent!=nullptr ) {
            gc_local<TypeDecl> tdecl(new TypeDecl(elemType->structType->parent));
            info->firstType = makeTypeInfo(nullptr, tdecl);
        } else {
            info->firstType = nullptr;
        }
        if ( elemType->secondType ) {
            info->secondType = makeTypeInfo(nullptr, elemType->secondType);
        } else {
            info->secondType = nullptr;
        }
        info->argTypes = nullptr;
        info->argCount = uint32_t(elemType->argTypes.size());
        if ( info->argCount ) {
            info->argTypes = (TypeInfo **) debugInfo->allocate(sizeof(TypeInfo *) * info->argCount );
            for ( uint32_t i=0, is=info->argCount; i!=is; ++i ) {
                info->argTypes[i] = makeTypeInfo(nullptr, elemType->argTypes[i]);
            }
        }
        info->argNames = nullptr;
        auto argNamesCount = uint32_t(elemType->argNames.size());
        t2cppTypeName[info] = describeCppType(type, CpptSubstitureRef::no,CpptSkipRef::yes, CpptSkipConst::yes);
        if ( argNamesCount ) {
            DAS_ASSERT(info->argCount == 0 || info->argCount == argNamesCount);
            info->argCount = argNamesCount;
            info->argNames = (const char **) debugInfo->allocate(sizeof(char *) * info->argCount );
            for ( uint32_t i=0, is=info->argCount; i!=is; ++i ) {
                info->argNames[i] = debugInfo->allocateCachedName(elemType->argNames[i]);
            }
        }
        auto mangledName = type->getMangledName();
        info->size = type->isAutoOrAlias() ? 0 : type->getSizeOf();
        info->hash = hash_blockz64((uint8_t *)mangledName.c_str());
        debugInfo->lookup[info->hash] = info;
        return info;
    }

    VarInfo * DebugInfoHelper::makeVariableDebugInfo ( const Structure & st, const Structure::FieldDeclaration & var ) {
        string mangledName = st.getMangledName() + " field " + var.name;
        auto it = vmn2v.find(mangledName);
        if ( it!=vmn2v.end() ) return it->second;
        VarInfo * vi = debugInfo->makeNode<VarInfo>();
        makeTypeInfo(vi, var.type);
        vi->name = debugInfo->allocateCachedName(var.name);
        vi->offset = var.offset;
        vi->annotation_arguments = makeAnnotationArguments(var.annotation, vi->annotation_argument_count);
        if ( var.init && var.init->constexpression ) {
            if ( var.init->rtti_isStringConstant() ) {
                auto sval = static_cast<ExprConstString*>(var.init);
                vi->sValue = debugInfo->allocateCachedName(sval->text);
            } else if ( var.init->rtti_isConstant() ) {
                auto cval = static_cast<ExprConst*>(var.init);
                vi->value = cval->value;
            } else {
                vi->value = v_zero();
            }
            vi->flags |= TypeInfo::flag_hasInitValue;
        } else {
            vi->value = v_zero();
        }
        vi->hash = hash_blockz64((uint8_t *)mangledName.c_str());
        vmn2v[mangledName] = vi;
        return vi;
    }

    VarInfo * DebugInfoHelper::makeVariableDebugInfo ( const Variable & var ) {
        string mangledName = var.getMangledName();
        auto it = vmn2v.find(mangledName);
        if ( it!=vmn2v.end() ) return it->second;
        VarInfo * vi = debugInfo->makeNode<VarInfo>();
        makeTypeInfo(vi, var.type);
        vi->name = debugInfo->allocateCachedName(var.name);
        vi->offset = 0;
        vi->annotation_arguments = makeAnnotationArguments(var.annotation, vi->annotation_argument_count);
        if ( var.init && var.init->constexpression ) {
            if ( var.init->rtti_isStringConstant() ) {
                auto sval = static_cast<ExprConstString*>(var.init);
                vi->sValue = debugInfo->allocateCachedName(sval->text);
            } else if ( var.init->rtti_isConstant() ) {
                auto cval = static_cast<ExprConst*>(var.init);
                vi->value = cval->value;
            } else {
                vi->value = v_zero();
            }
            vi->flags |= TypeInfo::flag_hasInitValue;
        } else {
            vi->value = v_zero();
        }
        vi->hash = hash_blockz64((uint8_t *)mangledName.c_str());
        vmn2v[mangledName] = vi;
        return vi;
    }

}
