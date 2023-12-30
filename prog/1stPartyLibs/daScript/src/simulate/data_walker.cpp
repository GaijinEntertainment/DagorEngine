#include "daScript/misc/platform.h"

#include "daScript/simulate/data_walker.h"
#include "daScript/ast/ast.h"
#include "daScript/simulate/hash.h"

namespace das {

    void DataWalker::error ( const char * message ) {
        if ( context ) {
            context->throw_error(message);
        }
        cancel = true;
    }

    void DataWalker::walk ( vec4f x, TypeInfo * info ) {
        if ( info->flags & TypeInfo::flag_refType ) {
            walk(cast<char *>::to(x), info );
        } else {
            walk((char *)&x, info );
        }
    }

    void DataWalker::walk_struct ( char * ps, StructInfo * si ) {
        if ( ps && (si->flags & StructInfo::flag_class) ) {
            auto ti = *(TypeInfo **) ps;
            if ( ti!=nullptr ) si = ti->structType;
            else invalidData(); // we are walking uninitialized class here
        }
        if ( canVisitStructure(ps, si) ) {
            beforeStructure(ps, si);
            if ( cancel ) {
                afterStructureCancel(ps, si);
                return;
            }
            for ( uint32_t i=0, is=si->count; i!=is; ++i ) {
                bool last = i==(si->count-1);
                VarInfo * vi = si->fields[i];
                char * pf = ps + vi->offset;
                beforeStructureField(ps, si, pf, vi, last);
                if ( cancel ) {
                    afterStructureCancel(ps, si);
                    return;
                }
                walk(pf, vi);
                if ( cancel ) {
                    afterStructureCancel(ps, si);
                    return;
                }
                afterStructureField(ps, si, pf, vi, last);
                if ( cancel ) {
                    afterStructureCancel(ps, si);
                    return;
                }
            }
            afterStructure(ps, si);
        }
    }

    void DataWalker::walk_tuple ( char * ps, TypeInfo * ti ) {
        if ( canVisitTuple(ps,ti) ) {
            beforeTuple(ps, ti);
            if ( cancel ) return;
            int fieldOffset = 0;
            for ( uint32_t i=0, is=ti->argCount; i!=is; ++i ) {
                bool last = i==(ti->argCount-1);
                TypeInfo * vi = ti->argTypes[i];
                auto fa = getTypeAlign(vi) - 1;
                fieldOffset = (fieldOffset + fa) & ~fa;
                char * pf = ps + fieldOffset;
                beforeTupleEntry(ps, ti, pf, vi, last);
                if ( cancel ) return;
                walk(pf, vi);
                if ( cancel ) return;
                afterTupleEntry(ps, ti, pf, vi, last);
                if ( cancel ) return;
                fieldOffset += vi->size;
            }
            afterTuple(ps, ti);
        }
    }

    void DataWalker::walk_variant ( char * ps, TypeInfo * ti ) {
        if ( canVisitVariant(ps,ti) ) {
            beforeVariant(ps, ti);
            if ( cancel ) return;
            int32_t fidx = *((int32_t *)ps);
            DAS_ASSERTF(uint32_t(fidx)<ti->argCount,"invalid variant index");
            int fieldOffset = getTypeBaseSize(Type::tInt);
            TypeInfo * vi = ti->argTypes[fidx];
            auto fa = getTypeAlign(ti) - 1;
            fieldOffset = (fieldOffset + fa) & ~fa;
            char * pf = ps + fieldOffset;
            if ( cancel ) return;
            walk(pf, vi);
            if ( cancel ) return;
            afterVariant(ps, ti);
        }
    }

    void DataWalker::walk_array ( char * pa, uint32_t stride, uint32_t count, TypeInfo * ti ) {
        if ( !canVisitArrayData(ti,count) ) return;
        char * pe = pa;
        beforeArrayData(pa, stride, count, ti);
        if ( cancel ) return;
        for ( uint32_t i=0; i!=count; ++i ) {
            bool last = i==count-1;
            beforeArrayElement(pa, ti, pe, i, last);
            if ( cancel ) return;
            walk(pe, ti);
            if ( cancel ) return;
            afterArrayElement(pa, ti, pe, i, last);
            if ( cancel ) return;
            pe += stride;
        }
        afterArrayData(pa, stride, count, ti);
    }

    void DataWalker::walk_dim ( char * pa, TypeInfo * ti ) {
        beforeDim(pa, ti);
        if ( cancel ) return;
        TypeInfo copyInfo = *ti;
        DAS_ASSERT(copyInfo.dimSize);
        copyInfo.size = ti->dim[0] ? copyInfo.size / ti->dim[0] : copyInfo.size;
        copyInfo.dimSize --;
        vector<uint32_t> udim;
        if ( copyInfo.dimSize ) {
            for ( uint32_t i=0, is=copyInfo.dimSize; i!=is; ++i) {
                udim.push_back(ti->dim[i+1]);
            }
            copyInfo.dim = udim.data();
        } else {
            copyInfo.dim = nullptr;
        }
        uint32_t stride = copyInfo.size;
        uint32_t count = ti->dim[0];
        walk_array(pa, stride, count, &copyInfo);
        if ( cancel ) return;
        afterDim(pa, ti);
    }

    void DataWalker::walk_table ( Table * tab, TypeInfo * info ) {
        if ( !canVisitTableData(info) ) return;
        int keySize = info->firstType->size;
        int valueSize = info->secondType->size;
        uint32_t count = 0;
        for ( uint32_t i=0, is=tab->capacity; i!=is; ++i ) {
            if ( tab->hashes[i] > HASH_KILLED64 ) {
                bool last = (count == (tab->size-1));
                // key
                char * key = tab->keys + i*keySize;
                beforeTableKey(tab, info, key, info->firstType, count, last);
                if ( cancel ) return;
                walk ( key, info->firstType );
                if ( cancel ) return;
                afterTableKey(tab, info, key, info->firstType, count, last);
                if ( cancel ) return;
                // value
                char * value = tab->data + i*valueSize;
                beforeTableValue(tab, info, value, info->secondType, count, last);
                if ( cancel ) return;
                walk ( value, info->secondType );
                if ( cancel ) return;
                afterTableValue(tab, info, value, info->secondType, count, last);
                if ( cancel ) return;
                // next
                count ++;
            }
        }
    }

    void DataWalker::walk ( char * pa, TypeInfo * info ) {
        if ( pa == nullptr ) {
            Null(info);
        } else if ( info->flags & TypeInfo::flag_ref ) {
            beforeRef(pa,info);
            if ( cancel ) return;
            TypeInfo ti = *info;
            ti.flags &= ~TypeInfo::flag_ref;
            walk(*(char **)pa, &ti);
            ti.flags |= TypeInfo::flag_ref;
            if ( cancel ) return;
            afterRef(pa,info);
        } else if ( info->dimSize ) {
            walk_dim(pa, info);
        } else if ( info->type==Type::tArray ) {
            auto arr = (Array *) pa;
            if ( canVisitArray(arr,info) ) {
                beforeArray(arr, info);
                if ( cancel ) return;
                walk_array(arr->data, info->firstType->size, arr->size, info->firstType);
                if ( cancel ) return;
                afterArray(arr, info);
            }
        } else if ( info->type==Type::tTable ) {
            auto tab = (Table *) pa;
            if ( canVisitTable(pa,info) ) {
                beforeTable(tab, info);
                if ( cancel ) return;
                walk_table(tab, info);
                if ( cancel ) return;
                afterTable(tab, info);
            }
        } else {
            switch ( info->type ) {
                case Type::tBool:       Bool(*((bool *)pa)); break;
                case Type::tInt8:       Int8(*((int8_t *)pa)); break;
                case Type::tUInt8:      UInt8(*((uint8_t *)pa)); break;
                case Type::tInt16:      Int16(*((int16_t *)pa)); break;
                case Type::tUInt16:     UInt16(*((uint16_t *)pa)); break;
                case Type::tInt64:      Int64(*((int64_t *)pa)); break;
                case Type::tUInt64:     UInt64(*((uint64_t *)pa)); break;
                case Type::tString:     String(*((char **)pa)); break;
                case Type::tInt:        Int(*((int32_t *)pa)); break;
                case Type::tInt2:       Int2(*((int2 *)pa)); break;
                case Type::tInt3:       Int3(*((int3 *)pa)); break;
                case Type::tInt4:       Int4(*((int4 *)pa)); break;
                case Type::tUInt:       UInt(*((uint32_t *)pa)); break;
                case Type::tBitfield:   Bitfield(*((uint32_t *)pa),info); break;
                case Type::tUInt2:      UInt2(*((uint2 *)pa)); break;
                case Type::tUInt3:      UInt3(*((uint3 *)pa)); break;
                case Type::tUInt4:      UInt4(*((uint4 *)pa)); break;
                case Type::tFloat:      Float(*((float *)pa)); break;
                case Type::tFloat2:     Float2(*((float2 *)pa)); break;
                case Type::tFloat3:     Float3(*((float3 *)pa)); break;
                case Type::tFloat4:     Float4(*((float4 *)pa)); break;
                case Type::tDouble:     Double(*((double *)pa)); break;
                case Type::tRange:      Range(*((range *)pa)); break;
                case Type::tURange:     URange(*((urange *)pa)); break;
                case Type::tRange64:    Range64(*((range64 *)pa)); break;
                case Type::tURange64:   URange64(*((urange64 *)pa)); break;
                case Type::tEnumeration:    WalkEnumeration(*((int32_t *)pa), info->enumType); break;
                case Type::tEnumeration8:   WalkEnumeration8(*((int8_t *)pa),  info->enumType); break;
                case Type::tEnumeration16:  WalkEnumeration16(*((int16_t *)pa), info->enumType); break;
                case Type::fakeContext:     FakeContext(*(Context**)pa); break;
                case Type::fakeLineInfo:    FakeLineInfo(*(LineInfo**)pa); break;
                case Type::tPointer:
                    if ( canVisitPointer(info) ) {
                        beforePtr(pa, info);
                        if ( cancel ) return;
                        if ( info->firstType && info->firstType->type!=Type::tVoid ) {
                            walk(*(char**)pa, info->firstType);
                            if ( cancel ) return;
                        } else {
                            VoidPtr(*(void**)pa);
                            if ( cancel ) return;
                        }
                        afterPtr(pa, info);
                    }
                    break;
                case Type::tStructure:  walk_struct(pa, info->structType); break;
                case Type::tTuple:      walk_tuple(pa, info); break;
                case Type::tVariant:    walk_variant(pa, info); break;
                case Type::tBlock:      WalkBlock((Block *)pa); break;
                case Type::tFunction:   WalkFunction((Func *)pa); break;
                case Type::tLambda: {
                        if ( canVisitLambda(info) ) {
                            auto ll = (Lambda *) pa;
                            beforeLambda(ll, info);
                            if ( cancel ) return;
                            walk ( ll->capture, ll->getTypeInfo() );
                            if ( cancel ) return;
                            afterLambda(ll, info);
                        }
                    }
                    break;
                case Type::tIterator: {
                        if ( canVisitIterator(info) ) {
                            auto ll = (Sequence *) pa;
                            beforeIterator(ll, info);
                            if ( cancel ) return;
                            if ( ll->iter ) {
                                ll->iter->walk(*this);
                            } else {
                                Null(info);
                            }
                            if ( cancel ) return;
                            afterIterator(ll, info);
                        }
                    }
                    break;
                case Type::tHandle:
                    if ( canVisitHandle(pa, info) ) {
                        beforeHandle(pa, info);
                        if ( cancel ) return;
                        info->getAnnotation()->walk(*this, pa);
                        if ( cancel ) return;
                        afterHandle(pa, info);
                    }
                    break;
                case Type::tVoid:       break;  // skip void
                case Type::anyArgument: break;  // skip any argument
                default:                DAS_ASSERTF(0, "unsupported print type"); break;
            }
        }
    }
}
