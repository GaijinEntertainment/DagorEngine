#pragma once

#include "daScript/simulate/debug_info.h"

#include "daScript/misc/arraytype.h"
#include "daScript/misc/vectypes.h"
#include "daScript/misc/rangetype.h"

namespace das {

    // NOTE: parameters here are unreferenced for a reason
    //            the idea is you copy the function defintion, and paste to your code
#if defined(_MSC_VER)
#pragma warning(push)
#pragma warning(disable:4100)    // unreferenced formal parameter
#elif defined(__EDG__)
#pragma diag_suppress 826
#elif defined(__GNUC__)
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wunused-parameter"
#elif defined(__clang__)
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wunused-parameter"
#endif

    struct DataWalker : ptr_ref_count {
    // we doing what?
        class Context * context = nullptr;
        bool reading = false;
        bool _cancel = false;
    // helpers
        void error ( const char * message );
        virtual bool cancel () { return _cancel; }
    // data structures
        virtual bool canVisitArray ( Array * ar, TypeInfo * ti ) { return true; }
        virtual bool canVisitArrayData ( TypeInfo * ti, uint32_t count ) { return true; }
        virtual bool canVisitHandle ( char * ps, TypeInfo * ti ) { return true; }
        virtual bool canVisitStructure ( char * ps, StructInfo * si ) { return true; }
        virtual bool canVisitTuple ( char * ps, TypeInfo * ti ) { return true; }
        virtual bool canVisitVariant ( char * ps, TypeInfo * ti ) { return true; }
        virtual bool canVisitTable ( char * ps, TypeInfo * ti ) { return true; }
        virtual bool canVisitTableData ( TypeInfo * ti ) { return true; }
        virtual bool canVisitPointer ( TypeInfo * ti ) { return true; }
        virtual bool canVisitLambda ( TypeInfo * ti ) { return true; }
        virtual bool canVisitIterator ( TypeInfo * ti ) { return true; }
        virtual void beforeStructure ( char * ps, StructInfo * si ) {}
        virtual void afterStructure ( char * ps, StructInfo * si ) {}
        virtual void afterStructureCancel ( char * ps, StructInfo * si ) {}
        virtual void beforeStructureField ( char * ps, StructInfo * si, char * pv, VarInfo * vi, bool last ) {}
        virtual void afterStructureField ( char * ps, StructInfo * si, char * pv, VarInfo * vi, bool last ) {}
        virtual void beforeTuple ( char * ps, TypeInfo * ti ) {}
        virtual void afterTuple ( char * ps, TypeInfo * ti ) {}
        virtual void beforeTupleEntry ( char * ps, TypeInfo * ti, char * pv, TypeInfo * vi, bool last ) {}
        virtual void afterTupleEntry ( char * ps, TypeInfo * ti, char * pv, TypeInfo * vi, bool last ) {}
        virtual void beforeVariant ( char * ps, TypeInfo * ti ) {}
        virtual void afterVariant ( char * ps, TypeInfo * ti ) {}
        virtual void beforeArrayData ( char * pa, uint32_t stride, uint32_t count, TypeInfo * ti ) {}
        virtual void afterArrayData ( char * pa, uint32_t stride, uint32_t count, TypeInfo * ti ) {}
        virtual void beforeArrayElement ( char * pa, TypeInfo * ti, char * pe, uint32_t index, bool last ) {}
        virtual void afterArrayElement ( char * pa, TypeInfo * ti, char * pe, uint32_t index, bool last ) {}
        virtual void beforeDim ( char * pa, TypeInfo * ti ) {}
        virtual void afterDim ( char * pa, TypeInfo * ti ) {}
        virtual void beforeArray ( Array * pa, TypeInfo * ti ) {}
        virtual void afterArray ( Array * pa, TypeInfo * ti ) {}
        virtual void beforeTable ( Table * pa, TypeInfo * ti ) {}
        virtual void beforeTableKey ( Table * pa, TypeInfo * ti, char * pk, TypeInfo * ki, uint32_t index, bool last ) {}
        virtual void afterTableKey ( Table * pa, TypeInfo * ti, char * pk, TypeInfo * ki, uint32_t index, bool last ) {}
        virtual void beforeTableValue ( Table * pa, TypeInfo * ti, char * pv, TypeInfo * kv, uint32_t index, bool last ) {}
        virtual void afterTableValue ( Table * pa, TypeInfo * ti, char * pv, TypeInfo * kv, uint32_t index, bool last ) {}
        virtual void afterTable ( Table * pa, TypeInfo * ti ) {}
        virtual void beforeRef ( char * pa, TypeInfo * ti ) {}
        virtual void afterRef ( char * pa, TypeInfo * ti ) {}
        virtual void beforePtr ( char * pa, TypeInfo * ti ) {}
        virtual void afterPtr ( char * pa, TypeInfo * ti ) {}
        virtual void beforeHandle ( char * pa, TypeInfo * ti ) {}
        virtual void afterHandle ( char * pa, TypeInfo * ti ) {}
        virtual void beforeLambda ( Lambda *, TypeInfo * ti ) {}
        virtual void afterLambda ( Lambda *, TypeInfo * ti ) {}
        virtual void beforeIterator ( Sequence *, TypeInfo * ti ) {}
        virtual void afterIterator ( Sequence *, TypeInfo * ti ) {}
    // types
        virtual void Null ( TypeInfo * ti ) {}
        virtual void Bool ( bool & ) {}
        virtual void Int8 ( int8_t & ) {}
        virtual void UInt8 ( uint8_t & ) {}
        virtual void Int16 ( int16_t & ) {}
        virtual void UInt16 ( uint16_t & ) {}
        virtual void Int64 ( int64_t & ) {}
        virtual void UInt64 ( uint64_t & ) {}
        virtual void String ( char * & ) {}
        virtual void Double ( double & ) {}
        virtual void Float ( float & ) {}
        virtual void Int ( int32_t & ) {}
        virtual void UInt ( uint32_t & ) {}
        virtual void Bitfield ( uint32_t &, TypeInfo * ti ) {}
        virtual void Int2 ( int2 & ) {}
        virtual void Int3 ( int3 & ) {}
        virtual void Int4 ( int4 & ) {}
        virtual void UInt2 ( uint2 & ) {}
        virtual void UInt3 ( uint3 & ) {}
        virtual void UInt4 ( uint4 & ) {}
        virtual void Float2 ( float2 & ) {}
        virtual void Float3 ( float3 & ) {}
        virtual void Float4 ( float4 & ) {}
        virtual void Range ( range & ) {}
        virtual void URange ( urange & ) {}
        virtual void Range64 ( range64 & ) {}
        virtual void URange64 ( urange64 & ) {}
        virtual void VoidPtr ( void * & ) {}
        virtual void WalkBlock ( Block * ) {}
        virtual void WalkFunction ( Func * ) {}
        virtual void WalkEnumeration ( int32_t &, EnumInfo * ) {}
        virtual void WalkEnumeration8  ( int8_t &, EnumInfo * ) {}
        virtual void WalkEnumeration16 ( int16_t &, EnumInfo * ) {}
        virtual void WalkEnumeration64 ( int64_t &, EnumInfo * ) {}
        virtual void FakeContext ( Context * ) {}
        virtual void FakeLineInfo ( LineInfo * ) {}
    // walk
        virtual void walk ( char * pf, TypeInfo * ti );
        virtual void walk ( vec4f f, TypeInfo * ti );
        virtual void walk_struct ( char * ps, StructInfo * si );
        virtual void walk_tuple ( char * ps, TypeInfo * info );
        virtual void walk_variant ( char * ps, TypeInfo * info );
        virtual void walk_array ( char * pa, uint32_t stride, uint32_t count, TypeInfo * ti );
        virtual void walk_dim ( char * pa, TypeInfo * ti );
        virtual void walk_table ( Table * tab, TypeInfo * info );
    // invalid data
        virtual void invalidData () {}
    };

    typedef smart_ptr<DataWalker> DataWalkerPtr;

#if defined(_MSC_VER)
#pragma warning(pop)
#elif defined(__EDG__)
#pragma diag_default 826
#elif defined(__GNUC__)
#pragma GCC diagnostic pop
#elif defined(__clang__)
#pragma clang diagnostic pop
#endif
}
