#include "daScript/misc/platform.h"

#include "daScript/ast/ast.h"
#include "daScript/simulate/simulate.h"
#include "daScript/simulate/data_walker.h"
#include "daScript/simulate/debug_print.h"

namespace das
{
    static TypeInfo lambda_type_info (Type::tLambda, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, 0, 0, nullptr,
        TypeInfo::flag_stringHeapGC | TypeInfo::flag_heapGC | TypeInfo::flag_lockCheck, sizeof(Lambda), 0 );

    char * presentStr ( char * buf, char * ch, int size );

    struct PtrRange {
        char * from = nullptr;
        char * to = nullptr;
        PtrRange () {}
        PtrRange ( char * F, size_t size ) : from(F), to(F+size) {}
        void clear() { from = to = nullptr; }
        __forceinline bool empty() const { return from==to; }
        __forceinline bool contains ( const PtrRange & r ) { return !empty() && !r.empty() && from<=r.from && to>=r.to; }
    };

    using loop_point = pair<void *,uint64_t>;

    struct BaseGcDataWalker : DataWalker {

        int32_t            gcFlags = TypeInfo::flag_stringHeapGC | TypeInfo::flag_heapGC;
        int32_t            gcStructFlags = StructInfo::flag_stringHeapGC | StructInfo::flag_heapGC;
        vector<loop_point> visited;
        vector<loop_point> visited_handles;

        virtual bool canVisitStructure ( char * ps, StructInfo * info ) override {
            if ( !(info->flags & gcStructFlags) ) return false;
            return find_if(visited.begin(),visited.end(),[&]( const loop_point & t ){
                    return t.first==ps && t.second==info->hash;
                }) == visited.end();
        }
        virtual bool canVisitHandle ( char * ps, TypeInfo * info ) override {
            if ( !(info->flags & gcFlags) ) return false;
            return find_if(visited_handles.begin(),visited_handles.end(),[&]( const loop_point & t ){
                    return t.first==ps && t.second==info->hash;
                }) == visited_handles.end();
        }

        virtual bool canVisitArrayData ( TypeInfo * ti, uint32_t ) override {
            return ti->flags & gcFlags;
        }
        virtual bool canVisitTableData ( TypeInfo * ti ) override {
            return (ti->firstType->flags & gcFlags) || (ti->secondType->flags & gcFlags);
        }

        virtual void walk_struct ( char * ps, StructInfo * si ) override {
            if ( ps && (si->flags & StructInfo::flag_class) ) {
                auto ti = *(TypeInfo **) ps;
                DAS_ASSERT(ti);
                si = ti->structType;
            }
            if ( canVisitStructure(ps, si) ) {
                beforeStructure(ps, si);
                for ( uint32_t i=si->firstGcField, is=si->count; i!=is; ) {
                    VarInfo * vi = si->fields[i];
                    char * pf = ps + vi->offset;
                    walk(pf, vi);
                    i = vi->nextGcField;
                }
                afterStructure(ps, si);
            }
        }

        virtual void walk_tuple ( char * ps, TypeInfo * ti ) override {
            beforeTuple(ps, ti);
            int fieldOffset = 0;
            for ( uint32_t i=0, is=ti->argCount; i!=is; ++i ) {
                TypeInfo * vi = ti->argTypes[i];
                auto fa = getTypeAlign(vi) - 1;
                fieldOffset = (fieldOffset + fa) & ~fa;
                char * pf = ps + fieldOffset;
                walk(pf, vi);
                fieldOffset += vi->size;
            }
            afterTuple(ps, ti);
        }

        virtual void walk_variant ( char * ps, TypeInfo * ti ) override {
            beforeVariant(ps, ti);
            int32_t fidx = *((int32_t *)ps);
            DAS_ASSERTF(uint32_t(fidx)<ti->argCount,"invalid variant index");
            int fieldOffset = getTypeBaseSize(Type::tInt);
            TypeInfo * vi = ti->argTypes[fidx];
            auto fa = getTypeAlign(ti) - 1;
            fieldOffset = (fieldOffset + fa) & ~fa;
            char * pf = ps + fieldOffset;
            walk(pf, vi);
            afterVariant(ps, ti);
        }

        virtual void walk_array ( char * pa, uint32_t stride, uint32_t count, TypeInfo * ti ) override {
            if ( !canVisitArrayData(ti,count) ) return;
            char * pe = pa;
            for ( uint32_t i=0; i!=count; ++i ) {
                walk(pe, ti);
                pe += stride;
            }
        }

        virtual void walk_dim ( char * pa, TypeInfo * ti ) override {
            beforeDim(pa, ti);
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
            afterDim(pa, ti);
        }

        virtual void walk_table ( Table * tab, TypeInfo * info ) override {
            if ( !canVisitTableData(info) ) return;
            int keySize = info->firstType->size;
            int valueSize = info->secondType->size;
            if (info->firstType->flags & gcFlags) {
                if (info->secondType->flags & gcFlags) {
                    for ( uint32_t i=0, is=tab->capacity; i!=is; ++i ) {
                        if ( tab->hashes[i] > HASH_KILLED64 ) {
                            // key
                            char * key = tab->keys + i*keySize;
                            walk ( key, info->firstType );
                            // value
                            char * value = tab->data + i*valueSize;
                            walk ( value, info->secondType );
                        }
                    }
                } else {
                    for ( uint32_t i=0, is=tab->capacity; i!=is; ++i ) {
                        if ( tab->hashes[i] > HASH_KILLED64 ) {
                            // key
                            char * key = tab->keys + i*keySize;
                            walk ( key, info->firstType );
                        }
                    }
                }
            } else {
                for ( uint32_t i=0, is=tab->capacity; i!=is; ++i ) {
                    if ( tab->hashes[i] > HASH_KILLED64 ) {
                        // value
                        char * value = tab->data + i*valueSize;
                        walk ( value, info->secondType );
                    }
                }
            }
        }

        using DataWalker::walk;

        virtual void walk ( char * pa, TypeInfo * info ) override {
            if ( pa == nullptr ) {
            } else if ( info->flags & TypeInfo::flag_ref ) {
                beforeRef(pa,info);
                TypeInfo ti = *info;
                ti.flags &= ~TypeInfo::flag_ref;
                walk(*(char **)pa, &ti);
                ti.flags |= TypeInfo::flag_ref;
                afterRef(pa,info);
            } else if ( info->dimSize ) {
                walk_dim(pa, info);
            } else {
                switch ( info->type ) {
                    case Type::tArray: {
                            auto arr = (Array *) pa;
                            beforeArray(arr, info);
                            walk_array(arr->data, info->firstType->size, arr->size, info->firstType);
                            afterArray(arr, info);
                        }
                        break;
                    case Type::tTable: {
                            auto tab = (Table *) pa;
                            beforeTable(tab, info);
                            walk_table(tab, info);
                            afterTable(tab, info);
                        }
                        break;
                    case Type::tString:     String(*((char **)pa)); break;
                    case Type::tPointer: {
                            if ( info->firstType && info->firstType->type!=Type::tVoid ) {
                                beforePtr(pa, info);
                                walk(*(char**)pa, info->firstType);
                                afterPtr(pa, info);
                            }
                        }
                        break;
                    case Type::tStructure:  walk_struct(pa, info->structType); break;
                    case Type::tTuple:      walk_tuple(pa, info); break;
                    case Type::tVariant:    walk_variant(pa, info); break;
                    case Type::tLambda: {
                            auto ll = (Lambda *) pa;
                            walk ( ll->capture, ll->getTypeInfo() );
                        }
                        break;
                    case Type::tIterator: {
                            auto ll = (Sequence *) pa;
                            if ( ll->iter ) {
                                ll->iter->walk(*this);
                            }
                        }
                        break;
                    case Type::tHandle:
                        if ( canVisitHandle(pa, info) ) {
                            beforeHandle(pa, info);
                            info->getAnnotation()->walk(*this, pa);
                            afterHandle(pa, info);
                        }
                        break;
                    default: break;
                }
            }
        }
    };

    struct HeapReporter final : BaseGcDataWalker {
        bool reportStringHeap = true;
        bool reportHeap = true;
        bool heapOnly = false;
        bool errorsOnly = false;
        vector<string>      history;
        vector<string>      keys;
        vector<PtrRange>    ptrRangeStack;
        PtrRange            currentRange;
        LOG                 tp;
        int32_t             gcAlways = TypeInfo::flag_stringHeapGC | TypeInfo::flag_heapGC;
        void prepare ( const string & walk_from ) {
            DAS_ASSERT(keys.size()==0);
            DAS_ASSERT(ptrRangeStack.size()==0);
            gcFlags = TypeInfo::flag_stringHeapGC | TypeInfo::flag_heapGC;
            gcStructFlags = StructInfo::flag_stringHeapGC | StructInfo::flag_heapGC;
            history.clear();
            history.push_back(walk_from);
            currentRange.clear();
        }
        bool markRange ( const PtrRange & r ) {
            if ( currentRange.empty() ) return true;
            if ( currentRange.contains(r) ) return false;
            if ( heapOnly ) {
                int ssize = int(r.to-r.from);
                ssize = (ssize + 15) & ~15;
                return context->heap->isOwnPtr(r.from, ssize);
            }
            return true;
        }
        void pushRange ( const PtrRange & rdata ) {
            ptrRangeStack.push_back(currentRange);
            currentRange = rdata;
        }
        void popRange() {
            currentRange = ptrRangeStack.back();
            ptrRangeStack.pop_back();
        }
        bool describe_ptr ( char * pa, int tsize, bool isHandle = false ) {
            auto ssize = (tsize+15) & ~15;
            bool show = !errorsOnly;
            if ( context->stack.is_stack_ptr(pa) ) {
                if ( show ) tp << "\tSTACK";
            } else if ( context->isGlobalPtr(pa) ) {
                if ( show ) tp << "\tGLOBAL";
            } else if ( context->isSharedPtr(pa) ) {
                if ( show ) tp << "\tSHAREDGLOBAL";
            } else if ( context->heap->isOwnPtr(pa, ssize) ) {
                if ( context->heap->isValidPtr(pa, ssize) ) {
                    if ( show ) tp << "\tHEAP";
                } else {
                    tp << "\tHEAP FREE!!!";
                    show = true;
                }
            } else if ( context->stringHeap->isOwnPtr(pa, ssize) ) {
                if ( context->stringHeap->isValidPtr(pa, ssize) ) {
                    if ( show ) tp << "\tSTRINGHEAP";
                } else {
                    tp << "\tSTRINGHEAP FREE!!!";
                    show = true;
                }
            } else if ( context->constStringHeap->isOwnPtr(pa) ) {
                if ( show ) tp << "\tCONSTSTRINGHEAP";
            } else if  ( context->code->isOwnPtr(pa) ) {
                if ( show ) tp << "\tCODE";
            } else if ( context->debugInfo->isOwnPtr(pa) ) {
                if ( show ) tp << "\tDEBUGINFO";
            } else if ( isHandle ) {
                if ( show ) tp << "\tHANDLED";
            } else {
                tp << "UNCATEGORIZED!!!";
                show = true;
            }
            if ( show ) {
                tp << " " << tsize <<  " bytes, at 0x" << HEX << uint64_t(pa) << DEC << " ";
                if ( pa==nullptr ) {
                    tp << "= NULL";
                } else {
                    auto bpa = (uint8_t *) pa;
                    tp << HEX;
                    char tohex[] = "0123456789abcdef";
                    for ( int i=0; i<8; ++i ) {
                        auto t = bpa[i];
                        tp << " " << tohex[t>>4] << tohex[t&15];
                    }
                    tp << DEC;
                }
            }
            return show;
        }
        void describeInfo ( TypeInfo * ti ) {
            if ( ti->flags & (TypeInfo::flag_stringHeapGC | TypeInfo::flag_heapGC) ) tp << " ";
            if ( ti->flags & TypeInfo::flag_stringHeapGC ) tp << "<S>";
            if ( ti->flags & TypeInfo::flag_heapGC ) tp << "<H>";
        }
        void describeStructInfo ( StructInfo * ti ) {
            if ( ti->flags & (StructInfo::flag_stringHeapGC | StructInfo::flag_heapGC) ) tp << " ";
            if ( ti->flags & StructInfo::flag_stringHeapGC ) tp << "<S>";
            if ( ti->flags & StructInfo::flag_heapGC ) tp << "<H>";
        }
        virtual void beforeHandle ( char * pa, TypeInfo * ti ) override {
            visited_handles.emplace_back(make_pair(pa,ti->hash));
            auto tsize = ti->size;
            DAS_ASSERT(tsize==uint32_t(getTypeSize(ti)));
            PtrRange rdata(pa, tsize );
            if ( reportHeap && tsize && markRange(rdata) ) {
                if ( describe_ptr(pa, tsize, true) ) {
                    describeInfo(ti);
                    ReportHistory();
                    tp << " HANDLE " << getTypeInfoMangledName(ti) << "\n";
                }
            }
            pushRange(rdata);
        }
        virtual void afterHandle ( char *, TypeInfo * ) override {
            popRange();
            visited_handles.pop_back();
        }
        virtual void beforeDim ( char * pa, TypeInfo * ti ) override {
            auto tsize = ti->size;
            DAS_ASSERT(tsize==uint32_t(getTypeSize(ti)));
            PtrRange rdata(pa, tsize );
            if ( reportHeap && tsize && markRange(rdata) ) {
                if ( describe_ptr(pa, tsize, (ti->flags & TypeInfo::flag_isHandled)!=0) ) {
                    describeInfo(ti);
                    ReportHistory();
                    tp << "DIM " << getTypeInfoMangledName(ti) << "\n";
                }
            }
            pushRange(rdata);
        }
        virtual void afterDim ( char *, TypeInfo * ) override {
            popRange();
        }
        virtual bool canVisitArrayData ( TypeInfo * ti, uint32_t ) override {
            return (ti->flags | gcAlways) & gcFlags;
        }
        virtual bool canVisitTableData ( TypeInfo * ti ) override {
            return ((ti->firstType->flags | gcAlways) & gcFlags) || ((ti->secondType->flags | gcAlways) & gcFlags);
        }
        virtual void beforeArray ( Array * PA, TypeInfo * ti ) override {
            auto tsize = ti->firstType->size * PA->capacity;
            DAS_ASSERT(tsize==getTypeSize(ti->firstType)*PA->capacity);
            char * pa = PA->data;
            PtrRange rdata(pa, tsize);
            if ( reportHeap && tsize && markRange(rdata) ) {
                if ( describe_ptr(pa, tsize) ) {
                    describeInfo(ti);
                    ReportHistory();
                    tp << "ARRAY " << getTypeInfoMangledName(ti) << "\n";
                }
            }
            pushRange(rdata);
        }
        virtual void afterArray ( Array *, TypeInfo * ) override {
            popRange();
        }
        virtual void beforeTable ( Table * PT, TypeInfo * ti ) override {
            auto tsize = (ti->firstType->size + ti->secondType->size + sizeof(uint64_t)) * PT->capacity;
            DAS_ASSERT(tsize==(getTypeSize(ti->firstType)+getTypeSize(ti->secondType)+sizeof(uint64_t))*PT->capacity);
            char * pa = PT->data;
            PtrRange rdata(pa, tsize);
            if ( reportHeap && tsize && markRange(rdata) ) {
                if ( describe_ptr(pa, int(tsize)) ) {
                    describeInfo(ti);
                    ReportHistory();
                    tp << "TABLE " << getTypeInfoMangledName(ti) << "\n";
                }
            }
            pushRange(rdata);
        }
        virtual void afterTable ( Table *, TypeInfo * ) override {
            popRange();
        }
        virtual void beforeRef ( char * pa, TypeInfo * ti ) override {
            auto tsize = ti->size;
            DAS_ASSERT(tsize==uint32_t(getTypeSize(ti)));
            PtrRange rdata(pa, tsize );
            if ( reportHeap && tsize && markRange(rdata) ) {
                if ( describe_ptr(pa, tsize) ) {
                    describeInfo(ti);
                    ReportHistory();
                    tp << "& " << getTypeInfoMangledName(ti) << "\n";
                }
            }
            pushRange(rdata);
        }
        virtual void afterRef ( char *, TypeInfo * ) override {
            popRange();
        }
        virtual void beforeStructure ( char * ps, StructInfo * si ) override {
            visited.emplace_back(make_pair(ps,si->hash));
            char * pa = ps;
            auto tsize = si->size;
            if ( si->flags & StructInfo::flag_lambda ) {
                pa -= 16;
                tsize += 16;
            }
            PtrRange rdata(pa, tsize);
            if ( reportHeap && tsize && markRange(rdata) ) {
                if ( describe_ptr(pa, tsize) ) {
                    describeStructInfo(si);
                    ReportHistory();
                    tp << "STRUCTURE " << si->module_name << "::" << si->name << "\n";
                }
            }
            pushRange(rdata);
        }
        virtual void afterStructure ( char *, StructInfo * ) override {
            popRange();
            visited.pop_back();
        }
        virtual void beforeStructureField ( char *, StructInfo *, char *, VarInfo * vi, bool ) override {
            history.push_back(vi->name);
        }
        virtual void afterStructureField ( char *, StructInfo *, char *, VarInfo *, bool ) override {
            history.pop_back();
        }
        virtual void beforeVariant ( char * ps, TypeInfo * ti ) override {
            char * pa = ps;
            auto tsize = ti->size;
            DAS_ASSERT(tsize==uint32_t(getTypeSize(ti)));
            PtrRange rdata(pa, tsize);
            if ( reportHeap && tsize && markRange(rdata) ) {
                if ( describe_ptr(pa, tsize) ) {
                    describeInfo(ti);
                    ReportHistory();
                    tp << "VARIANT " << getTypeInfoMangledName(ti) << "\n";
                }
            }
            pushRange(rdata);
        }
        virtual void afterVariant ( char *, TypeInfo * ) override {
            popRange();
        }
        virtual void beforeTuple ( char * ps, TypeInfo * si ) override {
            char * pa = ps;
            auto tsize = si->size;
            DAS_ASSERT(tsize==uint32_t(getTypeSize(si)));
            PtrRange rdata(pa, tsize);
            if ( reportHeap && tsize && markRange(rdata) ) {
                if ( describe_ptr(pa, tsize) ) {
                    describeInfo(si);
                    ReportHistory();
                    tp << "TUPLE " << getTypeInfoMangledName(si) << "\n";
                }
            }
            pushRange(rdata);
        }
        virtual void afterTuple ( char *, TypeInfo * ) override {
            popRange();
        }
        virtual void beforeTupleEntry ( char *, TypeInfo * ti, char *, TypeInfo * vi, bool ) override {
            uint32_t VI = -1u;
            for ( uint32_t i=0, is=ti->argCount; i!=is; ++i ) {
                if ( ti->argTypes[i]==vi ) {
                    VI = i;
                    break;
                }
            }
            history.push_back(to_string(VI));
        }
        virtual void afterTupleEntry ( char *, TypeInfo *, char *, TypeInfo *, bool ) override {
            history.pop_back();
        }
        virtual void beforeArrayElement ( char *, TypeInfo *, char *, uint32_t index, bool ) override {
            history.push_back("["+to_string(index)+"]");
        }
        virtual void afterArrayElement ( char *, TypeInfo *, char *, uint32_t, bool ) override {
            history.pop_back();
        }
        virtual void beforeTableKey ( Table *, TypeInfo *, char * pk, TypeInfo * ki, uint32_t, bool ) override {
            string keyText = debug_value ( pk, ki, PrintFlags::none );
            keys.push_back(keyText);
            history.push_back("=>key=>");
        }
        virtual void afterTableKey ( Table *, TypeInfo *, char *, TypeInfo *, uint32_t, bool ) override {
            history.pop_back();
        }
        virtual void beforeTableValue ( Table *, TypeInfo *, char *, TypeInfo *, uint32_t, bool ) override {
            string keyText = keys.back();
            history.push_back("[\""+escapeString(keyText)+"\"]");
        }
        virtual void afterTableValue ( Table *, TypeInfo *, char *, TypeInfo *, uint32_t, bool ) override {
            keys.pop_back();
            history.pop_back();
        }
        virtual bool canVisitStructure ( char * ps, StructInfo * info ) override {
            if ( !((info->flags | gcAlways) & gcStructFlags) ) return false;
            return find_if(visited.begin(),visited.end(),[&]( const loop_point & t ){
                    return t.first==ps && t.second==info->hash;
                }) == visited.end();
        }
        virtual bool canVisitHandle ( char * ps, TypeInfo * info ) override {
            if ( !((info->flags | gcAlways) & gcFlags) ) return false;
            return find_if(visited_handles.begin(),visited_handles.end(),[&]( const loop_point & t ){
                    return t.first==ps && t.second==info->hash;
                }) == visited_handles.end();
        }
        virtual void String ( char * & st ) override {
            if ( !reportStringHeap ) return;
            if ( !st ) return;
            if ( context->constStringHeap->isOwnPtr(st) ) return;
            bool show = !errorsOnly;
            char buf[32];
            uint32_t ulen = uint32_t(strlen(st)) + 1;
            uint32_t len = (ulen + 15) & ~15;
            if ( context->stringHeap->isOwnPtr(st,len) ) {
                if ( context->stringHeap->isValidPtr(st,len) ) {
                    if ( show ) tp << "\t\tSTRING ";
                } else {
                    tp << "\t\tSTRING FREE!!! ";
                    show = true;
                }
            } else {
                if ( show ) tp << "\t\tSTRING#!!! ";
            }
            if ( show ) {
                ReportHistory();
                tp << presentStr(buf, st, 32) << ", len=" << ulen  << "(" << len << ") at 0x" << HEX << uint64_t(st) << DEC << "\n";
            }
        }
        void ReportHistory ( void ) {
            tp << "\t";
            for (size_t i=0, is=history.size(); i!=is; ++i ) {
                if ( i!=0 && (history[i].empty() || history[i][0]!='[') ) tp << ".";
                tp << history[i];
            }
            tp << " : ";
        }

        // extended version, with entry callbacks
        virtual void walk_tuple ( char * ps, TypeInfo * ti ) override {
            if ( canVisitTuple(ps,ti) ) {
                beforeTuple(ps, ti);
                int fieldOffset = 0;
                for ( uint32_t i=0, is=ti->argCount; i!=is; ++i ) {
                    bool last = i==(ti->argCount-1);
                    TypeInfo * vi = ti->argTypes[i];
                    auto fa = getTypeAlign(vi) - 1;
                    fieldOffset = (fieldOffset + fa) & ~fa;
                    char * pf = ps + fieldOffset;
                    beforeTupleEntry(ps, ti, pf, vi, last);
                    walk(pf, vi);
                    afterTupleEntry(ps, ti, pf, vi, last);
                    fieldOffset += vi->size;
                }
                afterTuple(ps, ti);
            }
        }

        virtual void walk_struct ( char * ps, StructInfo * si ) override {
            if ( ps && (si->flags & StructInfo::flag_class) ) {
                auto ti = *(TypeInfo **) ps;
                DAS_ASSERT(ti);
                si = ti->structType;
            }
            if ( canVisitStructure(ps, si) ) {
                beforeStructure(ps, si);
                for ( uint32_t i=0, is=si->count; i!=is; ++i ) {
                    bool last = i==(si->count-1);
                    VarInfo * vi = si->fields[i];
                    char * pf = ps + vi->offset;
                    beforeStructureField(ps, si, pf, vi, last);
                    walk(pf, vi);
                    afterStructureField(ps, si, pf, vi, last);
                }
                afterStructure(ps, si);
            }
        }

        virtual void walk_table ( Table * tab, TypeInfo * info ) override {
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
                    walk ( key, info->firstType );
                    afterTableKey(tab, info, key, info->firstType, count, last);
                    // value
                    char * value = tab->data + i*valueSize;
                    beforeTableValue(tab, info, value, info->secondType, count, last);
                    walk ( value, info->secondType );
                    afterTableValue(tab, info, value, info->secondType, count, last);
                    // next
                    count ++;
                }
            }
        }
    };

    void Context::reportAnyHeap(LineInfo * at, bool sth, bool rgh, bool rghOnly, bool errorsOnly) {
        LOG tp(LogLevel::debug);
        // now
        HeapReporter walker;
        walker.context = this;
        walker.reportStringHeap = sth;
        walker.reportHeap = rgh;
        walker.heapOnly = rghOnly;
        walker.errorsOnly = errorsOnly;
        // mark GC roots
        tp << "GC ROOTS:\n";
        foreach_gc_root([&](void * _pa, TypeInfo * ti) {
            char * pa = (char *) _pa;
            walker.prepare("ext_gc_root");
            if ( ti ) {
                walker.walk(pa, ti);
            } else {
                Lambda lmb(pa);
                walker.walk((char *)&lmb, &lambda_type_info);
            }
        });
        // mark globals
        if ( sharedOwner ) {
            tp << "SHARED GLOBALS:\n";
            for ( int i=0, is=totalVariables; i!=is; ++i ) {
                auto & pv = globalVariables[i];
                if ( !pv.shared ) continue;
                if ( (pv.debugInfo->flags | walker.gcAlways) & walker.gcFlags ) {
                    walker.prepare(pv.name);
                    walker.walk(shared + pv.offset, pv.debugInfo);
                }
            }
        }
        tp << "GLOBALS:\n";
        for ( int i=0, is=totalVariables; i!=is; ++i ) {
            auto & pv = globalVariables[i];
            if ( pv.shared ) continue;
            if ( (pv.debugInfo->flags | walker.gcAlways) & walker.gcFlags ) {
                walker.prepare(pv.name);
                walker.walk(globals + pv.offset, pv.debugInfo);
            }
        }
        // mark stack
        char * sp = stack.ap();
        const LineInfo * lineAt = at;
        while (  sp < stack.top() ) {
            Prologue * pp = (Prologue *) sp;
            Block * block = nullptr;
            FuncInfo * info = nullptr;
            char * SP = sp;
            if ( pp->info ) {
                intptr_t iblock = intptr_t(pp->block);
                if ( iblock & 1 ) {
                    block = (Block *) (iblock & ~1);
                    info = block->info;
                    SP = stack.bottom() + block->stackOffset;
                } else {
                    info = pp->info;
                }
                tp << "FUNCTION " << info->name << "\n";
            }
            if ( info ) {
                if ( info->count ) {
                    tp << "ARGUMENTS:\n";
                    for ( uint32_t i=0, is=info->count; i!=is; ++i ) {
                        if ( (info->fields[i]->flags | walker.gcAlways) & walker.gcFlags ) {
                            walker.prepare(info->fields[i]->name);
                            walker.walk(pp->arguments[i], info->fields[i]);
                        }
                    }
                }
                if ( info->locals ) {
                    tp << "LOCALS:\n";
                    for ( uint32_t i=0, is=info->localCount; i!=is; ++i ) {
                        auto lv = info->locals[i];
                        bool inScope = lineAt ? lineAt->inside(lv->visibility) : false;
                        if ( !inScope ) continue;
                        char * addr = nullptr;
                        if ( lv->cmres ) {
                            addr = (char *)pp->cmres;
                        } else {
                            addr = SP + lv->stackTop;
                        }
                        if ( addr ) {
                            if ( (lv->flags | walker.gcAlways) & walker.gcFlags ) {
                                walker.prepare(lv->name);
                                walker.walk(addr, lv);
                            }
                        }
                    }
                }
            }
            lineAt = info ? pp->line : nullptr;
            sp += info ? info->stackSize : pp->stackSize;
        }
    }

    struct GcMarkStringHeap final : BaseGcDataWalker {
        bool validate = false;
        GcMarkStringHeap () {
            gcFlags = TypeInfo::flag_stringHeapGC;
            gcStructFlags = StructInfo::flag_stringHeapGC;
        }
        using loop_point = pair<void *,uint64_t>;
        das_set<char *> failed;

        virtual void beforeStructure ( char * ps, StructInfo * info ) override {
            visited.emplace_back(make_pair(ps,info->hash));
        }
        virtual void afterStructure ( char *, StructInfo * ) override {
            visited.pop_back();
        }
        virtual void beforeHandle ( char * ps, TypeInfo * ti ) override {
            visited_handles.emplace_back(make_pair(ps,ti->hash));
        }
        virtual void afterHandle ( char *, TypeInfo * ) override {
            visited_handles.pop_back();
        }
        virtual void String ( char * & st ) override {
            DataWalker::String(st);
            if ( !st ) return;
            if ( context->constStringHeap->isOwnPtr(st) ) return;
            uint32_t len = uint32_t(strlen(st)) + 1;
            len = (len + 15) & ~15;
            if ( validate ) {
                if ( context->stringHeap->isOwnPtr(st, len) ) {
                    if ( context->stringHeap->isValidPtr(st, len) ) {
                        context->stringHeap->mark(st, len);
                    } else {
                        failed.insert(st);
                    }
                }
            } else {
                context->stringHeap->mark(st, len);
            }
        }

        // fastest versions, without any before/after

        virtual void walk_tuple ( char * ps, TypeInfo * ti ) override {
            int fieldOffset = 0;
            for ( uint32_t i=0, is=ti->argCount; i!=is; ++i ) {
                TypeInfo * vi = ti->argTypes[i];
                auto fa = getTypeAlign(vi) - 1;
                fieldOffset = (fieldOffset + fa) & ~fa;
                char * pf = ps + fieldOffset;
                walk(pf, vi);
                fieldOffset += vi->size;
            }
        }

        virtual void walk_variant ( char * ps, TypeInfo * ti ) override {
            int32_t fidx = *((int32_t *)ps);
            DAS_ASSERTF(uint32_t(fidx)<ti->argCount,"invalid variant index");
            int fieldOffset = getTypeBaseSize(Type::tInt);
            TypeInfo * vi = ti->argTypes[fidx];
            auto fa = getTypeAlign(ti) - 1;
            fieldOffset = (fieldOffset + fa) & ~fa;
            char * pf = ps + fieldOffset;
            walk(pf, vi);
        }

        virtual void walk_dim ( char * pa, TypeInfo * ti ) override {
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
        }

        using DataWalker::walk;

        virtual void walk ( char * pa, TypeInfo * info ) override {
            if ( pa == nullptr ) {
            } else if ( info->flags & TypeInfo::flag_ref ) {
                beforeRef(pa,info);
                TypeInfo ti = *info;
                ti.flags &= ~TypeInfo::flag_ref;
                walk(*(char **)pa, &ti);
                ti.flags |= TypeInfo::flag_ref;
                afterRef(pa,info);
            } else if ( info->dimSize ) {
                walk_dim(pa, info);
            } else {
                switch ( info->type ) {
                    case Type::tArray: {
                            auto arr = (Array *) pa;
                            walk_array(arr->data, info->firstType->size, arr->size, info->firstType);
                        }
                        break;
                    case Type::tTable: {
                            auto tab = (Table *) pa;
                            walk_table(tab, info);
                        }
                        break;
                    case Type::tString:     String(*((char **)pa)); break;
                    case Type::tPointer: {
                            if ( info->firstType && info->firstType->type!=Type::tVoid ) {
                                walk(*(char**)pa, info->firstType);
                            }
                        }
                        break;
                    case Type::tStructure:  walk_struct(pa, info->structType); break;
                    case Type::tTuple:      walk_tuple(pa, info); break;
                    case Type::tVariant:    walk_variant(pa, info); break;
                    case Type::tLambda: {
                            auto ll = (Lambda *) pa;
                            walk ( ll->capture, ll->getTypeInfo() );
                        }
                        break;
                    case Type::tIterator: {
                            auto ll = (Sequence *) pa;
                            if ( ll->iter ) {
                                ll->iter->walk(*this);
                            }
                        }
                        break;
                    case Type::tHandle:
                        if ( canVisitHandle(pa, info) ) {
                            beforeHandle(pa, info);
                            info->getAnnotation()->walk(*this, pa);
                            afterHandle(pa, info);
                        }
                        break;
                    default: break;
                }
            }
        }
    };

    void Context::collectStringHeap ( LineInfo * at, bool validate ) {
        // clean up, so that all small allocations are marked as 'free'
        if ( !stringHeap->mark() ) return;
        // now
        GcMarkStringHeap walker;
        walker.context = this;
        walker.validate = validate;
        // mark GC roots
        foreach_gc_root([&](void * _pa, TypeInfo * ti) {
            char * pa = (char *) _pa;
            if ( ti ) {
                walker.walk(pa, ti);
            } else {
                Lambda lmb(pa);
                walker.walk((char *)&lmb, &lambda_type_info);
            }
        });
        // mark globals
        if ( sharedOwner ) {
            for ( int i=0, is=totalVariables; i!=is; ++i ) {
                auto & pv = globalVariables[i];
                if ( !pv.shared ) continue;
                walker.walk(shared + pv.offset, pv.debugInfo);
            }
        }
        for ( int i=0, is=totalVariables; i!=is; ++i ) {
            auto & pv = globalVariables[i];
            if ( pv.shared ) continue;
            walker.walk(globals + pv.offset, pv.debugInfo);
        }
        // mark stack
        char * sp = stack.ap();
        const LineInfo * lineAt = at;
        while (  sp < stack.top() ) {
            Prologue * pp = (Prologue *) sp;
            Block * block = nullptr;
            FuncInfo * info = nullptr;
            char * SP = sp;
            if ( pp->info ) {
                intptr_t iblock = intptr_t(pp->block);
                if ( iblock & 1 ) {
                    block = (Block *) (iblock & ~1);
                    info = block->info;
                    SP = stack.bottom() + block->stackOffset;
                } else {
                    info = pp->info;
                }
            }
            if ( info ) {
                for ( uint32_t i=0, is=info->count; i!=is; ++i ) {
                    walker.walk(pp->arguments[i], info->fields[i]);
                }
                if ( info->locals ) {
                    for ( uint32_t i=0, is=info->localCount; i!=is; ++i ) {
                        auto lv = info->locals[i];
                        bool inScope = lineAt ? lineAt->inside(lv->visibility) : false;
                        if ( !inScope ) continue;
                        char * addr = nullptr;
                        if ( lv->cmres ) {
                            addr = (char *)pp->cmres;
                        } else {
                            addr = SP + lv->stackTop;
                        }
                        if ( addr ) {
                            walker.walk(addr, lv);
                        }
                    }
                }
            }
            lineAt = info ? pp->line : nullptr;
            sp += info ? info->stackSize : pp->stackSize;
        }
        // sweep
        stringHeap->sweep();
        // report errors
        if ( !walker.failed.empty() ) {
            reportAnyHeap(at, true, false, false, true);
            TextWriter tw;
            tw << "string GC failed on the following dangling pointers:" << HEX;
            for ( auto f : walker.failed ) {
                tw << " " << uint64_t(f);
            }
            auto etext = stringHeap->allocateString(tw.str());
            throw_error_at(at, "%s", etext);
        }
    }

    struct GcMarkAnyHeap final : BaseGcDataWalker {
        vector<PtrRange>    ptrRangeStack;
        PtrRange            currentRange;
        das_set<char *>     failed;
        bool                markStringHeap = true;
        bool                validate = false;
        void prepare() {
            currentRange.clear();
            gcFlags = TypeInfo::flag_heapGC;
            gcStructFlags = StructInfo::flag_heapGC;
            if ( markStringHeap ) {
                gcFlags |= TypeInfo::flag_stringHeapGC;
                gcStructFlags |= StructInfo::flag_stringHeapGC;
            }
        }
        bool markAndPushRange ( const PtrRange & r ) {
            bool result = true;
            ptrRangeStack.push_back(currentRange);
            if ( !r.empty() && !currentRange.contains(r) ) {
                int ssize = int(r.to-r.from);
                ssize = (ssize + 15) & ~15;
                if ( validate ) {
                    if ( context->heap->isOwnPtr(r.from, ssize) ) {
                        if ( context->heap->isValidPtr(r.from, ssize) ) {
                            result = context->heap->mark(r.from, ssize);
                        } else {
                            failed.insert(r.from);
                        }
                    }
                } else {
                    result = context->heap->mark(r.from, ssize);
                }
                currentRange = r;
            }
            return result;
        }
        void popRange() {
            currentRange = ptrRangeStack.back();
            ptrRangeStack.pop_back();
        }
        virtual void beforeHandle ( char * pa, TypeInfo * ti ) override {
            visited_handles.emplace_back(make_pair(pa,ti->hash));
            PtrRange rdata(pa, ti->size);
            markAndPushRange(rdata);
        }
        virtual void afterHandle ( char *, TypeInfo * ) override {
            popRange();
            visited_handles.pop_back();
        }
        virtual void beforeDim ( char * pa, TypeInfo * ti ) override {
            PtrRange rdata(pa, ti->size);
            markAndPushRange(rdata);
        }
        virtual void afterDim ( char *, TypeInfo * ) override {
            popRange();
        }
        virtual void beforeArray ( Array * PA, TypeInfo * ti ) override {
            PtrRange rdata(PA->data, size_t(ti->firstType->size) * size_t(PA->capacity));
            markAndPushRange(rdata);
        }
        virtual void afterArray ( Array *, TypeInfo * ) override {
            popRange();
        }
        virtual void beforeTable ( Table * PT, TypeInfo * ti ) override {
            PtrRange rdata(PT->data, (ti->firstType->size+ti->secondType->size+sizeof(uint64_t))*size_t(PT->capacity));
            markAndPushRange(rdata);
        }
        virtual void afterTable ( Table *, TypeInfo * ) override {
            popRange();
        }
        virtual void beforeRef ( char * pa, TypeInfo * ti ) override {
            PtrRange rdata(pa, ti->size);
            markAndPushRange(rdata);
        }
        virtual void afterRef ( char *, TypeInfo * ) override {
            popRange();
        }
        virtual void beforePtr ( char * pa, TypeInfo * ti ) override {
            if ( auto ptr = *(char**)pa ) {
                PtrRange rdata(ptr, ti->firstType->size);
                markAndPushRange(rdata);
            }
        }
        virtual void afterPtr ( char * pa, TypeInfo * ) override {
            if ( *(char**)pa ) popRange();
        }
        virtual void beforeStructure ( char * pa, StructInfo * ti ) override {
            visited.emplace_back(make_pair(pa,ti->hash));
            auto tsize = ti->size;
            if ( ti->flags & StructInfo::flag_lambda ) {
                pa -= 16;
                tsize += 16;
            }
            PtrRange rdata(pa, tsize);
            markAndPushRange(rdata);
        }
        virtual void afterStructure ( char *, StructInfo * ) override {
            popRange();
            visited.pop_back();
        }
        virtual void beforeVariant ( char * ps, TypeInfo * ti ) override {
            char * pa = ps;
            PtrRange rdata(pa, ti->size);
            markAndPushRange(rdata);
        }
        virtual void afterVariant ( char *, TypeInfo * ) override {
            popRange();
        }
        virtual void beforeTuple ( char * pa, TypeInfo * ti ) override {
            PtrRange rdata(pa, ti->size);
            markAndPushRange(rdata);
        }
        virtual void afterTuple ( char *, TypeInfo * ) override {
            popRange();
        }
        virtual void String ( char * & st ) override {
            if ( !markStringHeap ) return;
            if ( !st ) return;
            if ( context->constStringHeap->isOwnPtr(st) ) return;
            uint32_t len = uint32_t(strlen(st)) + 1;
            len = (len + 15) & ~15;
            if ( validate ) {
                if ( context->stringHeap->isOwnPtr(st, len) ) {
                    if ( context->stringHeap->isValidPtr(st, len) ) {
                        context->stringHeap->mark(st, len);
                    } else {
                        failed.insert(st);
                    }
                }
            } else {
                context->stringHeap->mark(st, len);
            }
        }

        using DataWalker::walk;

        virtual void walk ( char * pa, TypeInfo * info ) override {
            if ( pa == nullptr ) {
            } else if ( info->flags & TypeInfo::flag_ref ) {
                beforeRef(pa,info);
                TypeInfo ti = *info;
                ti.flags &= ~TypeInfo::flag_ref;
                walk(*(char **)pa, &ti);
                ti.flags |= TypeInfo::flag_ref;
                afterRef(pa,info);
            } else if ( info->dimSize ) {
                walk_dim(pa, info);
            } else {
                switch ( info->type ) {
                    case Type::tArray: {
                            auto arr = (Array *) pa;
                            beforeArray(arr, info);
                            walk_array(arr->data, info->firstType->size, arr->size, info->firstType);
                            afterArray(arr, info);
                        }
                        break;
                    case Type::tTable: {
                            auto tab = (Table *) pa;
                            beforeTable(tab, info);
                            walk_table(tab, info);
                            afterTable(tab, info);
                        }
                        break;
                    case Type::tString:     String(*((char **)pa)); break;
                    case Type::tPointer: if ( *(char**)pa && info->firstType ) {
                            if ( info->firstType->type==Type::tStructure ) {
                                auto *si = info->firstType->structType;
                                auto tsize = info->firstType->size;
                                auto *ps = *(char**)pa;
                                if ( si->flags & StructInfo::flag_lambda ) {
                                    ps -= 16;
                                    tsize += 16;
                                }
                                else if ( si->flags & StructInfo::flag_class ) {
                                    si = (*(TypeInfo **) ps)->structType;
                                    tsize = si->size;
                                }
                                if (markAndPushRange(PtrRange(ps, tsize))) {
                                    // walk_struct(*(char**)pa, info->firstType->structType);
                                    ps = *(char**)pa;
                                    if ( canVisitStructure(ps, si) ) {
                                        visited.emplace_back(make_pair(pa,si->hash));
                                        for ( uint32_t i=si->firstGcField, is=si->count; i!=is; ) {
                                            VarInfo * vi = si->fields[i];
                                            char * pf = ps + vi->offset;
                                            walk(pf, vi);
                                            i = vi->nextGcField;
                                        }
                                        visited.pop_back();
                                    }
                                }
                                popRange();
                            } else {
                                beforePtr(pa, info);
                                walk(*(char**)pa, info->firstType);
                                afterPtr(pa, info);
                            }
                        }
                        break;
                    case Type::tStructure:  walk_struct(pa, info->structType); break;
                    case Type::tTuple:      walk_tuple(pa, info); break;
                    case Type::tVariant:    walk_variant(pa, info); break;
                    case Type::tLambda: {
                            auto ll = (Lambda *) pa;
                            walk ( ll->capture, ll->getTypeInfo() );
                        }
                        break;
                    case Type::tIterator: {
                            auto ll = (Sequence *) pa;
                            if ( ll->iter ) {
                                ll->iter->walk(*this);
                            }
                        }
                        break;
                    case Type::tHandle:
                        if ( canVisitHandle(pa, info) ) {
                            beforeHandle(pa, info);
                            info->getAnnotation()->walk(*this, pa);
                            afterHandle(pa, info);
                        }
                        break;
                    default: break;
                }
            }
        }
    };



    void Context::collectHeap ( LineInfo * at, bool sheap, bool validate ) {
        // clean up, so that all small allocations are marked as 'free'
        if ( sheap && !stringHeap->mark() ) return;
        if ( !heap->mark() ) return;
        // now
        GcMarkAnyHeap walker;
        walker.markStringHeap = sheap;
        walker.context = this;
        walker.validate = validate;
        // mark GC roots
        foreach_gc_root([&](void * _pa, TypeInfo * ti) {
            char * pa = (char *) _pa;
            walker.prepare();
            if ( ti ) {
                walker.walk(pa, ti);
            } else {
                Lambda lmb(pa);
                walker.walk((char *)&lmb, &lambda_type_info);
            }
        });
        // mark globals
        if ( sharedOwner ) {
            for ( int i=0, is=totalVariables; i!=is; ++i ) {
                auto & pv = globalVariables[i];
                if ( !pv.shared ) continue;
                walker.prepare();
                walker.walk(shared + pv.offset, pv.debugInfo);
            }
        }
        for ( int i=0, is=totalVariables; i!=is; ++i ) {
            auto & pv = globalVariables[i];
            if ( pv.shared ) continue;
            walker.prepare();
            walker.walk(globals + pv.offset, pv.debugInfo);
        }
        // mark stack
        char * sp = stack.ap();
        const LineInfo * lineAt = at;
        while (  sp < stack.top() ) {
            Prologue * pp = (Prologue *) sp;
            Block * block = nullptr;
            FuncInfo * info = nullptr;
            char * SP = sp;
            if ( pp->info ) {
                intptr_t iblock = intptr_t(pp->block);
                if ( iblock & 1 ) {
                    block = (Block *) (iblock & ~1);
                    info = block->info;
                    SP = stack.bottom() + block->stackOffset;
                } else {
                    info = pp->info;
                }
            }
            if ( info ) {
                for ( uint32_t i=0, is=info->count; i!=is; ++i ) {
                    walker.prepare();
                    walker.walk(pp->arguments[i], info->fields[i]);
                }
                if ( info->locals && lineAt ) {
                    for ( uint32_t i=0, is=info->localCount; i!=is; ++i ) {
                        auto lv = info->locals[i];
                        bool inScope = lineAt->inside(lv->visibility);
                        if ( !inScope ) continue;
                        char * addr = nullptr;
                        if ( lv->cmres ) {
                            addr = (char *)pp->cmres;
                        } else {
                            addr = SP + lv->stackTop;
                        }
                        if ( addr ) {
                            walker.prepare();
                            walker.walk(addr, lv);
                        }
                    }
                }
            }
            lineAt = info ? pp->line : nullptr;
            sp += info ? info->stackSize : pp->stackSize;
        }
        // sweep
        if ( sheap ) stringHeap->sweep();
        // report errors
        heap->sweep();
        if ( !walker.failed.empty() ) {
            reportAnyHeap(at, sheap, true, true, true);
            TextWriter tw;
            tw << "GC failed on the following dangling pointers:" << HEX;
            for ( auto f : walker.failed ) {
                tw << " " << uint64_t(f);
            }
            auto etext = stringHeap->allocateString(tw.str());
            throw_error_at(at, "%s", etext);
        }
    }
}

