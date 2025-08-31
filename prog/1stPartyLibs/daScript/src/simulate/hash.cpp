#include "daScript/misc/platform.h"


#include "daScript/ast/ast.h"
#include "daScript/simulate/hash.h"
#include "daScript/simulate/runtime_string.h"
#include "daScript/simulate/data_walker.h"

namespace das
{
    struct HashDataWalker : DataWalker, HashBuilder {
    public:
    // walker
        HashDataWalker ( Context & ctx ) {
            context = &ctx;
        }
    // data types
        virtual void Null ( TypeInfo * ti ) override    { update(ti->hash); }
        virtual void Bool ( bool & t ) override         { update(t); }
        virtual void Int8 ( int8_t & t ) override       { update(t); }
        virtual void UInt8 ( uint8_t & t ) override     { update(t); }
        virtual void Int16 ( int16_t & t ) override     { update(t); }
        virtual void UInt16 ( uint16_t & t ) override   { update(t); }
        virtual void Int64 ( int64_t & t ) override     { update(t); }
        virtual void UInt64 ( uint64_t & t ) override   { update(t); }
        virtual void String ( char * & t ) override     { updateString(t); }
        virtual void Double ( double & t ) override     { update(t); }
        virtual void Float ( float & t ) override       { update(t); }
        virtual void Int ( int32_t & t ) override       { update(t); }
        virtual void UInt ( uint32_t & t ) override     { update(t); }
        virtual void Bitfield ( uint32_t & t, TypeInfo * ti ) override { update(t); update(ti->hash); }
        virtual void Int2 ( int2 & t ) override         { update(t); }
        virtual void Int3 ( int3 & t ) override         { update(t); }
        virtual void Int4 ( int4 & t ) override         { update(t); }
        virtual void UInt2 ( uint2 & t ) override       { update(t); }
        virtual void UInt3 ( uint3 & t ) override       { update(t); }
        virtual void UInt4 ( uint4 & t ) override       { update(t); }
        virtual void Float2 ( float2 & t ) override     { update(t); }
        virtual void Float3 ( float3 & t ) override     { update(t); }
        virtual void Float4 ( float4 & t ) override     { update(t); }
        virtual void Range ( range & t ) override       { update(t); }
        virtual void URange ( urange & t ) override     { update(t); }
        virtual void Range64 ( range64 & t ) override   { update(t); }
        virtual void URange64 ( urange64 & t ) override { update(t); }
        virtual void VoidPtr ( void * & p ) override    { update(uint64_t(intptr_t(p))); }
        // WalkBlock
        virtual void WalkFunction ( Func * fn ) override { if ( fn && fn->PTR ) update(fn->PTR->mangledNameHash); }
        virtual void WalkEnumeration ( int32_t & t, EnumInfo * en ) override   { update(t); update(en->hash); }
        virtual void WalkEnumeration8  ( int8_t & t, EnumInfo * en ) override  { update(t); update(en->hash); }
        virtual void WalkEnumeration16 ( int16_t & t, EnumInfo * en ) override { update(t); update(en->hash); }
        virtual void WalkEnumeration64 ( int64_t & t, EnumInfo * en ) override { update(t); update(en->hash); }
    // unsupported
        virtual void beforeIterator ( Sequence *, TypeInfo * ) override { error("HASH, not expecting iterator"); }
        virtual void WalkBlock ( Block * ) override                     { error("HASH, not expecting block"); }
        virtual void FakeContext ( Context * ) override                 { error("HASH, not expecting context"); }
    };

    uint64_t hash_value ( Context & ctx, void * pX, TypeInfo * info ) {
        HashDataWalker walker(ctx);
        walker.walk((char*)pX,info);
        return walker.getHash();
    }

    uint64_t hash_value ( Context & ctx, vec4f value, TypeInfo * info ) {
        HashDataWalker walker(ctx);
        walker.walk(value,info);
        return walker.getHash();
    }
}

