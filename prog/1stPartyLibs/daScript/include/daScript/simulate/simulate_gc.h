#pragma once

#include "daScript/simulate/data_walker.h"

namespace das
{
    struct PtrRange {
        char * from = nullptr;
        char * to = nullptr;
        PtrRange () {}
        PtrRange ( char * F, size_t size ) : from(F), to(F+size) {}
        void clear() { from = to = nullptr; }
        __forceinline bool empty() const { return from==to; }
        __forceinline bool contains ( const PtrRange & r ) { return !empty() && !r.empty() && from<=r.from && to>=r.to; }
    };

    struct DAS_API BaseGcDataWalker : DataWalker {

        int32_t            gcFlags = TypeInfo::flag_stringHeapGC | TypeInfo::flag_heapGC;
        int32_t            gcStructFlags = StructInfo::flag_stringHeapGC | StructInfo::flag_heapGC;

        BaseGcDataWalker() {
            collecting = true;
            reading = false;
        }
        virtual bool canVisitStructure ( char * /*ps*/, StructInfo * info ) override {
            if ( !(info->flags & gcStructFlags) ) return false;
            return true;
        }
        virtual bool canVisitHandle ( char * /*ps*/, TypeInfo * info ) override {
            if ( !(info->flags & gcFlags) ) return false;
            return true;
        }
        virtual bool canVisitPointer ( TypeInfo * ti ) override {
            return ti->flags & gcFlags;
        }
        virtual bool canVisitArrayData ( TypeInfo * ti, uint64_t ) override {
            return ti->flags & gcFlags;
        }
        virtual bool canVisitTableData ( TypeInfo * ti ) override {
            return (ti->firstType->flags & gcFlags) || (ti->secondType->flags & gcFlags);
        }

        virtual void walk_struct ( char * ps, StructInfo * si ) override;
        virtual void walk_tuple ( char * ps, TypeInfo * ti ) override;
        virtual void walk_variant ( char * ps, TypeInfo * ti ) override;
        virtual void walk_array ( char * pa, uint32_t stride, uint64_t count, TypeInfo * ti ) override;
        virtual void walk_dim ( char * pa, TypeInfo * ti ) override;
        virtual void walk_table ( Table * tab, TypeInfo * info ) override;
        virtual void beforeIterator ( Iterator * ) {}
        using DataWalker::beforeIterator;
        using DataWalker::walk;
        virtual void walk ( char * pa, TypeInfo * info ) override;
    };
}
