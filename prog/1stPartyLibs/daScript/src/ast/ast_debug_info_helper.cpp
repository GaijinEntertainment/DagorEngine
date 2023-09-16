#include "daScript/misc/platform.h"

#include "daScript/ast/ast.h"
#include "daScript/ast/ast_expressions.h"
#include "daScript/ast/ast_visitor.h"
#include "daScript/simulate/hash.h"

namespace das {

    class CollectLocalVariables : public Visitor {
    public:
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

    void DebugInfoHelper::appendLocalVariables ( FuncInfo * info, const ExpressionPtr & body ) {
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

    EnumInfo * DebugInfoHelper::makeEnumDebugInfo ( const Enumeration & en ) {
        auto mangledName = en.getMangledName();
        auto it = emn2e.find(mangledName);
        if ( it!=emn2e.end() ) return it->second;
        EnumInfo * eni = debugInfo->makeNode<EnumInfo>();
        eni->name = debugInfo->allocateCachedName(en.name);
        eni->module_name = debugInfo->allocateCachedName(en.module->name);
        eni->count = uint32_t(en.list.size());
        eni->fields = (EnumValueInfo **) debugInfo->allocate(sizeof(EnumValueInfo *) * eni->count);
        uint32_t i = 0;
        for ( auto & ev : en.list ) {
            eni->fields[i] = (EnumValueInfo *) debugInfo->allocate(sizeof(EnumValueInfo));
            eni->fields[i]->name = debugInfo->allocateCachedName(ev.name);
            eni->fields[i]->value = !ev.value ? -1 : getConstExprIntOrUInt(ev.value);
            i ++;
        }
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
        if ( rtti && fn.builtIn ) {
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
        if ( fn.shutdown ) fni->flags |= FuncInfo::flag_shutdown;
        if ( fn.builtIn ) fni->flags |= FuncInfo::flag_builtin;
        if ( fn.privateFunction ) fni->flags |= FuncInfo::flag_private;
        fni->result = makeTypeInfo(nullptr, fn.result);
        fni->locals = nullptr;
        fni->localCount = 0;
        fni->hash = hash_blockz64((uint8_t *)mangledName.c_str());
        fmn2f[mangledName] = fni;
        return fni;
    }

    FuncInfo * DebugInfoHelper::makeInvokeableTypeDebugInfo ( const TypeDeclPtr & blk, const LineInfo & at ) {
        Function fakeFunc;
        fakeFunc.name = "invoke " + blk->describe();
        fakeFunc.at = at;
        fakeFunc.result = blk->firstType ? blk->firstType : make_smart<TypeDecl>(Type::tVoid);
        fakeFunc.totalStackSize = sizeof(Prologue);
        for ( size_t ai=0, ais=blk->argTypes.size(); ai!=ais; ++ ai ) {
            auto argV = make_smart<Variable>();
            argV->at = at;
            argV->name = blk->argNames.empty() ? ("arg_" + to_string(ai)) : blk->argNames[ai];
            argV->type = blk->argTypes[ai];
            fakeFunc.arguments.push_back(argV);
        }
        return makeFunctionDebugInfo(fakeFunc);
    }

    StructInfo * DebugInfoHelper::makeStructureDebugInfo ( const Structure & st ) {
        string mangledName = st.getMangledName();
        auto it = smn2s.find(mangledName);
        if ( it!=smn2s.end() ) return it->second;
        StructInfo * sti = debugInfo->makeNode<StructInfo>();
        sti->name = debugInfo->allocateCachedName(st.name);
        sti->flags = 0;
        if ( st.isClass ) sti->flags |= StructInfo::flag_class;
        if ( st.isLambda ) sti->flags |= StructInfo::flag_lambda;
        auto tdecl = TypeDecl((Structure *)&st);
        auto gcf = tdecl.gcFlags();
        if ( tdecl.lockCheck() ) sti->flags |= StructInfo::flag_lockCheck;
        if ( gcf & TypeDecl::gcFlag_heap ) sti->flags |= StructInfo::flag_heapGC;
        if ( gcf & TypeDecl::gcFlag_stringHeap ) sti->flags |= StructInfo::flag_stringHeapGC;
        sti->count = (uint32_t) st.fields.size();
        sti->size = st.getSizeOf();
        sti->fields = (VarInfo **) debugInfo->allocate( sizeof(VarInfo *) * sti->count );
        for ( uint32_t i=0, is=sti->count; i!=is; ++i ) {
            auto & var = st.fields[i];
            VarInfo * vi = makeVariableDebugInfo(st, var);
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
            if ( auto fn = st.module->findFunction(st.name) ) {
                sti->init_mnh = fn->getMangledNameHash();
            }
        }
        if ( rtti ) {
            sti->annotation_list = (void *) &st.annotations;
        } else {
            sti->annotation_list = nullptr;
        }
        sti->hash = hash_blockz64((uint8_t *)mangledName.c_str());
        smn2s[mangledName] = sti;
        return sti;
    }

    TypeInfo * DebugInfoHelper::makeTypeInfo ( TypeInfo * info, const TypeDeclPtr & type ) {
        if ( info==nullptr ) {
            string mangledName = type->getMangledName();
            auto it = tmn2t.find(mangledName);
            if ( it!=tmn2t.end() ) return it->second;
            info = debugInfo->makeNode<TypeInfo>();
            tmn2t[mangledName] = info;
        }
        info->type = type->baseType;
        info->dimSize = (uint32_t) type->dim.size();
        if ( info->dimSize ) {
            info->dim = (uint32_t *) debugInfo->allocate(sizeof(uint32_t) * info->dimSize );
            for ( uint32_t i=0, is=info->dimSize; i!=is; ++i ) {
                info->dim[i] = type->dim[i];
            }
        }
        if ( type->baseType==Type::tStructure  ) {
            info->structType = makeStructureDebugInfo(*type->structType);
        } else if ( type->isEnumT() ) {
            info->enumType = type->enumType ? makeEnumDebugInfo(*type->enumType) : nullptr;
        } else if ( type->annotation ) {
#if DAS_THREAD_SAFE_ANNOTATIONS
            auto annName = debugInfo->allocateCachedName("~" + type->annotation->module->name + "::" + type->annotation->name);
            info->annotation_or_name =  ((TypeAnnotation*)(intptr_t(annName)|1));
#else
            info->annotation_or_name = type->annotation;
#endif
        } else {
            info->annotation_or_name = nullptr;
        }
        info->flags = 0;
        if (type->lockCheck()) {
            info->flags |= TypeInfo::flag_lockCheck;
        }
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
        if (type->smartPtr) {
            info->flags |= TypeInfo::flag_isSmartPtr;
            if ( type->smartPtrNative )
                info->flags |= TypeInfo::flag_isSmartPtrNative;
        }
        auto gcf = type->gcFlags();
        if ( gcf & TypeDecl::gcFlag_heap )
            info->flags |= TypeInfo::flag_heapGC;
        if ( gcf & TypeDecl::gcFlag_stringHeap )
            info->flags |= TypeInfo::flag_stringHeapGC;
        if ( type->firstType ) {
            info->firstType = makeTypeInfo(nullptr, type->firstType);
        } else {
            info->firstType = nullptr;
        }
        if ( type->secondType ) {
            info->secondType = makeTypeInfo(nullptr, type->secondType);
        } else {
            info->secondType = nullptr;
        }
        info->argTypes = nullptr;
        info->argCount = uint32_t(type->argTypes.size());
        if ( info->argCount ) {
            info->argTypes = (TypeInfo **) debugInfo->allocate(sizeof(TypeInfo *) * info->argCount );
            for ( uint32_t i=0, is=info->argCount; i!=is; ++i ) {
                info->argTypes[i] = makeTypeInfo(nullptr, type->argTypes[i]);
            }
        }
        info->argNames = nullptr;
        auto argNamesCount = uint32_t(type->argNames.size());
        if ( argNamesCount ) {
            assert(info->argCount == 0 || info->argCount == argNamesCount);
            info->argCount = argNamesCount;
            info->argNames = (const char **) debugInfo->allocate(sizeof(char *) * info->argCount );
            for ( uint32_t i=0, is=info->argCount; i!=is; ++i ) {
                info->argNames[i] = debugInfo->allocateCachedName(type->argNames[i]);
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
        if ( rtti && !var.annotation.empty() ) {
            vi->annotation_arguments = (void *) &var.annotation;
        } else {
            vi->annotation_arguments = nullptr;
        }
        if ( rtti && var.init && var.init->constexpression ) {
            if ( var.init->rtti_isStringConstant() ) {
                auto sval = static_pointer_cast<ExprConstString>(var.init);
                vi->sValue = debugInfo->allocateCachedName(sval->text);
            } else if ( var.init->rtti_isConstant() ) {
                auto cval = static_pointer_cast<ExprConst>(var.init);
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
        if ( rtti && var.init && var.init->constexpression ) {
            if ( var.init->rtti_isStringConstant() ) {
                auto sval = static_pointer_cast<ExprConstString>(var.init);
                vi->sValue = debugInfo->allocateCachedName(sval->text);
            } else if ( var.init->rtti_isConstant() ) {
                auto cval = static_pointer_cast<ExprConst>(var.init);
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
