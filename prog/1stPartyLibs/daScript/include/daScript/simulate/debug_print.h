#pragma once

#include "daScript/simulate/data_walker.h"
#include "daScript/simulate/heap.h"
#include "daScript/simulate/runtime_string.h"
#include "daScript/simulate/simulate.h"

namespace das {

    class StringBuilderWriter : public StringWriter<VectorAllocationPolicy> {
    public:
        StringBuilderWriter() { }
        __forceinline char * c_str() const {
            return (char *) data.data();
        }
    };

    template <typename Writer>
    struct DebugDataWalker : DataWalker {
        using loop_point = pair<void *,uint64_t>;
        Writer & ss;
        PrintFlags flags;
        vector<loop_point> visited;
        vector<loop_point> visited_handles;
        DebugDataWalker() = delete;
        DebugDataWalker ( Writer & sss, PrintFlags f ) : ss(sss), flags(f) {}
    // data structures
        __forceinline void br() {
            if ( int(flags) & int(PrintFlags::namesAndDimensions) ) {
                if ( !(int(flags) & int(PrintFlags::singleLine)) ) {
                    ss << "\n";
                }
            }
        }
        virtual bool canVisitStructure ( char * ps, StructInfo * info ) override {
            auto it = find_if(visited.begin(),visited.end(),[&]( const loop_point & t ){
                return t.first==ps && t.second==info->hash;
            });
            if ( it==visited.end() ) {
                return true;
            } else {
                ss << "~loop at 0x" << HEX << intptr_t(ps) << DEC << " " << info->name << "~";
                return false;
            }
        }
        virtual bool canVisitHandle ( char * ps, TypeInfo * info ) override {
            auto it = find_if(visited.begin(),visited.end(),[&]( const loop_point & t ){
                return t.first==ps && t.second==info->hash;
            });
            if ( it==visited.end() ) {
                return true;
            } else {
                ss  << "~handle loop at 0x" << HEX << intptr_t(ps) << DEC << "~";
                return false;
            }
        }
        virtual void beforeStructure ( char * ps, StructInfo * info ) override {
            visited.emplace_back(make_pair(ps,info->hash));
            ss << "[[";
            if ( int(flags) & int(PrintFlags::namesAndDimensions) ) {
                ss << info->name;
            }
            if ( int(flags) & int(PrintFlags::refAddresses) ) {
                ss << " at 0x" << HEX << intptr_t(ps) << DEC;
            }
            br();
        }
        virtual void afterStructure ( char *, StructInfo * ) override {
            ss << "]]";
            br();
            visited.pop_back();
        }
        virtual void afterStructureCancel ( char *, StructInfo * ) override {
            visited.pop_back();
        }
        virtual void beforeStructureField ( char *, StructInfo *, char *, VarInfo * vi, bool ) override {
            ss << " ";
            if ( int(flags) & int(PrintFlags::namesAndDimensions) ) {
                ss << vi->name << " = ";
            }
        }
        virtual void afterStructureField ( char *, StructInfo *, char *, VarInfo *, bool last ) override {
            if ( !last ) {
                ss << ";";
            }
            br();
        }

        virtual void beforeTuple ( char * ps, TypeInfo * ) override {
            ss << "[[";
            if ( int(flags) & int(PrintFlags::namesAndDimensions) ) {
                ss << "tuple";
            }
            if ( int(flags) & int(PrintFlags::refAddresses) ) {
                ss << " at 0x" << HEX << intptr_t(ps) << DEC;
            }
            br();
        }
        virtual void afterTuple ( char *, TypeInfo * ) override {
            ss << "]]";
            br();
        }
        virtual void beforeTupleEntry ( char *, TypeInfo *, char *, TypeInfo *, bool ) override {
            ss << " ";
        }
        virtual void afterTupleEntry ( char *, TypeInfo *, char *, TypeInfo *, bool last ) override {
            if ( !last ) {
                ss << ";";
            }
            br();
        }

        virtual void beforeVariant ( char * ps, TypeInfo * ti ) override {
            if ( int(flags) & int(PrintFlags::namesAndDimensions) ) {
                ss << "[[variant ";
            }
            if ( int(flags) & int(PrintFlags::refAddresses) ) {
                ss << "at 0x" << HEX << intptr_t(ps) << DEC << " ";
            }
            if ( (int(flags) & int(PrintFlags::namesAndDimensions)) && ti->argNames) {
                auto vindex = *(uint32_t *)ps;
                if ( vindex < ti->argCount ) {
                    ss << ti->argNames[vindex] << "=";
                } else {
                    ss << "unknown=";
                }
            }
            br();
        }
        virtual void afterVariant ( char *, TypeInfo * ) override {
            if (int(flags) & int(PrintFlags::namesAndDimensions)) {
                ss << "]]";
            }
            br();
        }

        virtual void beforeArrayElement ( char *, TypeInfo *, char *, uint32_t, bool ) override {
            ss << " ";
        }
        virtual void afterArrayElement ( char *, TypeInfo *, char *, uint32_t, bool last ) override {
            if ( !last ) {
                ss << ";";
            }
            br();
        }
        virtual void beforeTableKey ( Table *, TypeInfo *, char *, TypeInfo *, uint32_t, bool ) override {
            ss << " ";
        }
        virtual void beforeTableValue ( Table *, TypeInfo *, char *, TypeInfo *, uint32_t, bool ) override {
            ss << " : ";
        }
        virtual void afterTableValue ( Table *, TypeInfo *, char *, TypeInfo *, uint32_t, bool last ) override {
            if ( !last ) {
                ss << ";";
            }
            br();
        }
        virtual void beforeDim ( char *, TypeInfo * ti ) override {
            ss << "[[";
            if ( int(flags) & int(PrintFlags::namesAndDimensions) ) {
                ss << debug_type(ti);
            }
            br();
        }
        virtual void afterDim ( char *, TypeInfo * ) override {
            ss << "]]";
            br();
        }
        virtual void beforeArray ( Array * arr, TypeInfo * ti ) override {
            ss << "[[";
            if ( int(flags) & int(PrintFlags::namesAndDimensions) ) {
                ss << debug_type(ti);
                if ( arr->shared ) ss << " /*shared*/ ";
            }
            if ( int(flags) & int(PrintFlags::refAddresses) ) {
                ss << " data at 0x" << HEX << intptr_t(arr->data) << DEC;
            }
            br();
        }
        virtual void afterArray ( Array *, TypeInfo * ) override {
            ss << "]]";
            br();
        }
        virtual void beforeTable ( Table * tab, TypeInfo * ti ) override {
            ss << "[[";
            if ( int(flags) & int(PrintFlags::namesAndDimensions) ) {
                ss << debug_type(ti);
                if ( tab->shared ) ss << " /*shared*/ ";
            }
            if ( int(flags) & int(PrintFlags::refAddresses) ) {
                ss << " data at 0x" << HEX << intptr_t(tab->data) << DEC;
            }
            br();
        }
        virtual void afterTable ( Table *, TypeInfo * ) override {
            ss << "]]";
            br();
        }
        virtual void beforeRef ( char * pa, TypeInfo * ti ) override {
            if ( int(flags) & int(PrintFlags::namesAndDimensions) ) {
                ss << "(" << debug_type(ti) << " 0x" << HEX << intptr_t(pa) << DEC << " ref = ";
            }
        }
        virtual void afterRef ( char *, TypeInfo * ) override {
            if ( int(flags) & int(PrintFlags::namesAndDimensions) ) {
                ss << ")";
            }
        }
        virtual void beforePtr ( char * pa, TypeInfo * ti ) override {
            if ( int(flags) & int(PrintFlags::namesAndDimensions) ) {
                ss << "(" << debug_type(ti) << " 0x" << HEX << intptr_t(*(char**)pa) << DEC;
                if ( ti->flags & TypeInfo::flag_isSmartPtr ) {
                    if ( ptr_ref_count * ps = *(ptr_ref_count**)pa) {
#if DAS_SMART_PTR_ID
                        ss << " smart_ptr(" << int32_t(ps->use_count())
                            << ",id=" << HEX << ps->ref_count_id << DEC << ") = ";
#else
                        ss << " smart_ptr(" << int32_t(ps->use_count()) << ") = ";
#endif
                    } else {
                        ss << " smart_ptr = ";
                    }
                }
                else {
                    ss << " ptr = ";
                }
            }
        }
        virtual void afterPtr ( char *, TypeInfo * ) override {
            if ( int(flags) & int(PrintFlags::namesAndDimensions) ) {
                ss << ")";
            }
        }
        virtual void beforeHandle ( char * ps, TypeInfo * ti ) override {
            visited_handles.emplace_back(make_pair(ps,ti->hash));
            if ( int(flags) & int(PrintFlags::namesAndDimensions) ) {
                ss << "[[" << debug_type(ti) << " ";
            }
            br();
        }
        virtual void afterHandle ( char *, TypeInfo * ) override {
            if ( int(flags) & int(PrintFlags::namesAndDimensions) ) {
                ss << "]]";
            }
            br();
            visited_handles.pop_back();
        }
        virtual void beforeLambda ( Lambda *, TypeInfo * ti ) override {
            if ( int(flags) & int(PrintFlags::namesAndDimensions) ) {
                ss << "(" << debug_type(ti) << " ";
            }
        }
        virtual void afterLambda ( Lambda *, TypeInfo * ) override {
            if ( int(flags) & int(PrintFlags::namesAndDimensions) ) {
                ss << ")";
            }
        }
    // types
        virtual void Null ( TypeInfo * ) override {
            ss << "null";
        }
        virtual void Bool ( bool & b ) override {
            ss << (b ? "true" : "false");
        }
        virtual void Int8 ( int8_t & i ) override {
            ss << int32_t(i);
        }
        virtual void UInt8 ( uint8_t & ui ) override {
            ss << "0x" << HEX << uint32_t(ui) << DEC;
        }
        virtual void Int16 ( int16_t & i ) override {
            ss << int32_t(i);
        }
        virtual void UInt16 ( uint16_t & ui ) override {
            ss << "0x" << HEX << uint32_t(ui) << DEC;
        }
        virtual void Int64 ( int64_t & i ) override {
            ss << i;
            if ( int(flags) & int(PrintFlags::typeQualifiers) ) {
                ss << "l";
            }
        }
        virtual void UInt64 ( uint64_t & ui ) override {
            ss << "0x" << HEX << ui << DEC;
            if ( int(flags) & int(PrintFlags::typeQualifiers) ) {
                ss << "ul";
            }
        }
        virtual void VoidPtr ( void * & p ) override {
            uint64_t ui = uint64_t(intptr_t(p));
            ss << "0x" << HEX << ui << DEC;
            if ( int(flags) & int(PrintFlags::typeQualifiers) ) {
                ss << "p";
            }
        }
        virtual void String ( char * & str ) override {
            string text = str ? str : "";
            if ( int(flags) & int(PrintFlags::escapeString) ) {
                ss << "\"" << escapeString(text) << "\"";
            } else {
                ss << text;
            }
            if ( int(flags) & int(PrintFlags::refAddresses) ) {
                ss << " /*0x" << HEX << intptr_t(str) << DEC << "*/";
            }
        }
        virtual void Float ( float & f ) override {
            if ( int(flags) & int(PrintFlags::fixedFloatingPoint) )
                ss << FIXEDFP << f << SCIENTIFIC;
            else
                ss << f;
            if ( int(flags) & int(PrintFlags::typeQualifiers) ) {
                ss << "f";
            }
        }
        virtual void Double ( double & f ) override {
            if ( int(flags) & int(PrintFlags::fixedFloatingPoint) )
                ss << FIXEDFP << f << SCIENTIFIC;
            else
                ss << f;
            if ( int(flags) & int(PrintFlags::typeQualifiers) ) {
                ss << "lf";
            }
        }
        virtual void Int ( int32_t & i ) override {
            ss << i;
        }
        virtual void UInt ( uint32_t & ui ) override {
            ss << "0x" << HEX << ui << DEC;
            if ( int(flags) & int(PrintFlags::typeQualifiers) ) {
                ss << "u";
            }
        }
        virtual void Bitfield ( uint32_t & ui, TypeInfo * info ) override {
            if ( info->argNames ) {
                if ( ui ) {
                    ss << "(";
                    bool any = false;
                    for ( uint32_t bit=0, bits=info->argCount; bit!=bits; ++bit ) {
                        if ( ui & (1<<bit) ) {
                            if ( any ) ss << "|"; else any = true;
                            ss << info->argNames[bit];
                            if ( int(flags) & int(PrintFlags::namesAndDimensions) ) {
                                ss << "(" << (1u<<bit) << ")";
                            }
                        }
                    }
                    ss << ")";
                } else {
                    ss << "(0)";
                }
            } else {
                ss << "0x" << HEX << ui << DEC;
                if ( int(flags) & int(PrintFlags::typeQualifiers) ) {
                    ss << "u";
                }
            }
        }
        virtual void Int2 ( int2 & i ) override {
            ss << i;
        }
        virtual void Int3 ( int3 & i ) override {
            ss << i;
        }
        virtual void Int4 ( int4 & i ) override {
            ss << i;
        }
        virtual void UInt2 ( uint2 & ui ) override {
            ss << ui;
        }
        virtual void UInt3 ( uint3 & ui ) override {
            ss << ui;
        }
        virtual void UInt4 ( uint4 & ui ) override {
            ss << ui;
        }
        virtual void Float2 ( float2 & fv ) override {
            ss << fv;
        }
        virtual void Float3 ( float3 & fv ) override {
            ss << fv;
        }
        virtual void Float4 ( float4 & fv ) override {
            ss << fv;
        }
        virtual void Range ( range & ra ) override {
            ss << ra;
        }
        virtual void URange ( urange & ra ) override {
            ss << ra;
        }
        virtual void Range64 ( range64 & ra ) override {
            ss << ra;
        }
        virtual void URange64 ( urange64 & ra ) override {
            ss << ra;
        }
        virtual void FakeContext ( Context * ctx ) override {
            ss << "context 0x" << HEX << intptr_t(ctx) << DEC;
        }
        virtual void FakeLineInfo ( LineInfo * at ) override {
            ss << (at ? at->describe() : "lineinfo null)");
        }
        virtual void beforeIterator ( Sequence *, TypeInfo * ) override {
            ss << "iterator [[";
        }
        virtual void afterIterator ( Sequence *, TypeInfo * ) override {
            ss << "]]";
        }
        virtual void WalkBlock ( struct Block * pa ) override {
            ss << "block 0x" << HEX << intptr_t(pa->body) << DEC;
        }
        virtual void WalkEnumeration ( int32_t & value, EnumInfo * info ) override {
            for ( uint32_t t=0, ts=info->count; t!=ts; ++t ) {
                if ( value == info->fields[t]->value ) {
                    ss << info->fields[t]->name;
                    return;
                }
            }
            ss << "enum " << value;
        }
        virtual void WalkEnumeration8 ( int8_t & value, EnumInfo * info ) override {
            for ( uint32_t t=0, ts=info->count; t!=ts; ++t ) {
                if ( value == info->fields[t]->value ) {
                    ss << info->fields[t]->name;
                    return;
                }
            }
            ss << "enum " << value;
        }
        virtual void WalkEnumeration16 ( int16_t & value, EnumInfo * info ) override {
            for ( uint32_t t=0, ts=info->count; t!=ts; ++t ) {
                if ( value == info->fields[t]->value ) {
                    ss << info->fields[t]->name;
                    return;
                }
            }
            ss << "enum " << value;
        }
        virtual void WalkFunction ( Func * fn ) override {
            if ( fn->PTR ) {
                ss << fn->PTR->name << "/*" << fn->PTR->mangledName << "*/";
            } else {
                ss << "null";
            }
        }
        virtual void invalidData () override {
            ss << "invalid data";
        }
    };

    string debug_value ( void * pX, TypeInfo * info, PrintFlags flags );
    string debug_value ( vec4f value, TypeInfo * info, PrintFlags flags );
    string debug_json_value ( void * pX, TypeInfo * info, bool humanReadable );
    string debug_json_value ( vec4f value, TypeInfo * info, bool humanReadable );
}
