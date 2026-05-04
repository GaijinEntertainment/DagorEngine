#pragma once

#include "daScript/simulate/data_walker.h"
#include "daScript/simulate/runtime_string.h"
#include "daScript/simulate/simulate.h"

namespace das {

    DAS_API uint64_t getCancelLimit();

    class StringBuilderWriter : public TextWriter {
    public:
        StringBuilderWriter() { }
    };

    template <typename Writer>
    struct DebugDataWalker : DataWalker {
        using loop_point = pair<void *,uint64_t>;
        Writer & ss;
        PrintFlags flags;
        DebugDataWalker() = delete;
        DebugDataWalker ( Writer & sss, PrintFlags f ) : ss(sss), flags(f), limit(getCancelLimit()) {}
        uint64_t limit = 0;
    // we cancel
        __forceinline bool cancel() override {
            if ( _cancel ) return true;
            if ( limit==0 ) return false;
            if ( ss.tellp()>limit ) {
                _cancel = true;
                return true;
            }
            return false;
        }
    // helpers
        __forceinline bool hasFlag(PrintFlags f) const { return (int(flags) & int(f)) != 0; }
        __forceinline string debug_type_no_dim(const TypeInfo * ti) {
            TypeInfo tmp = *ti;
            tmp.dimSize = 0;
            tmp.dim = nullptr;
            tmp.flags &= ~(TypeInfo::flag_isConst | TypeInfo::flag_ref | TypeInfo::flag_isTemp | TypeInfo::flag_isImplicit);
            return debug_type(&tmp);
        }
    // data structures
        __forceinline void br() {
            if ( hasFlag(PrintFlags::namesAndDimensions) ) {
                if ( !hasFlag(PrintFlags::singleLine) ) {
                    ss << "\n";
                }
            }
        }

        virtual bool revisitStructure ( char * ps, StructInfo * info ) override {
            ss << "~loop at 0x" << HEX << intptr_t(ps) << DEC << " " << info->name << "~";
            return false;
         }
        virtual bool revisitHandle ( char * ps, TypeInfo * ) override {
            ss  << "~handle loop at 0x" << HEX << intptr_t(ps) << DEC << "~";
            return false;
         }
        virtual void beforeStructure ( char * ps, StructInfo * info ) override {
            if ( hasFlag(PrintFlags::namesAndDimensions) ) {
                ss << info->name;
            }
            ss << "(";
            if ( hasFlag(PrintFlags::refAddresses) ) {
                ss << " at 0x" << HEX << intptr_t(ps) << DEC;
            }
            br();
        }
        virtual void afterStructure ( char *, StructInfo * ) override {
            ss << ")";
            br();
        }
        virtual void beforeStructureField ( char *, StructInfo *, char *, VarInfo * vi, bool ) override {
            ss << " ";
            if ( hasFlag(PrintFlags::namesAndDimensions) ) {
                ss << vi->name << " = ";
            }
        }
        virtual void afterStructureField ( char *, StructInfo *, char *, VarInfo *, bool last ) override {
            if ( !last ) {
                ss << ",";
            }
            br();
        }

        virtual void beforeTuple ( char * ps, TypeInfo * ti ) override {
            if ( hasFlag(PrintFlags::fullTypeInfo) ) {
                ss << debug_type(ti);
            }
            ss << "(";
            if ( hasFlag(PrintFlags::refAddresses) ) {
                ss << " at 0x" << HEX << intptr_t(ps) << DEC;
            }
            if ( !ti->isTupleOfSimpleTypes() ) br();
        }
        virtual void afterTuple ( char *, TypeInfo * ti ) override {
            ss << ")";
            if ( !ti->isTupleOfSimpleTypes() ) br();
        }
        virtual void beforeTupleEntry ( char *, TypeInfo *, char *, int, bool ) override {
            ss << " ";
        }
        virtual void afterTupleEntry ( char *, TypeInfo * ti, char *, int, bool last ) override {
            if ( !last ) {
                ss << ",";
            }
            if ( !ti->isTupleOfSimpleTypes() ) br();
        }

        virtual void beforeVariant ( char * ps, TypeInfo * ti ) override {
            if ( hasFlag(PrintFlags::namesAndDimensions) ) {
                ss << "variant(";
            }
            if ( hasFlag(PrintFlags::refAddresses) ) {
                ss << "at 0x" << HEX << intptr_t(ps) << DEC << " ";
            }
            if ( hasFlag(PrintFlags::namesAndDimensions) && ti->argNames) {
                auto vindex = *(uint32_t *)ps;
                if ( vindex < ti->argCount ) {
                    ss << ti->argNames[vindex] << " = ";
                } else {
                    ss << "unknown = ";
                }
            }
            if ( !ti->isVariantOfSimpleTypes() ) br();
        }
        virtual void afterVariant ( char *, TypeInfo * ti ) override {
            if ( hasFlag(PrintFlags::namesAndDimensions) ) {
                ss << ")";
            }
            if ( !ti->isVariantOfSimpleTypes() ) br();
        }

        virtual void beforeArrayElement ( char *, TypeInfo *, char *, uint32_t, bool ) override {
            ss << " ";
        }
        virtual void afterArrayElement ( char *, TypeInfo * ti, char *, uint32_t, bool last ) override {
            if ( !last ) {
                ss << ",";
            }
            if ( !ti->isSimpleType() ) br();
        }
        virtual void beforeTableKey ( Table *, TypeInfo *, char *, TypeInfo *, uint32_t, bool ) override {
            ss << " ";
        }
        virtual void beforeTableValue ( Table *, TypeInfo *, char *, TypeInfo *, uint32_t, bool ) override {
            ss << " => ";
        }
        virtual void afterTableValue ( Table *, TypeInfo * ti, char *, TypeInfo *, uint32_t, bool last ) override {
            if ( !last ) {
                ss << ",";
            }
            if ( !ti->isTableOfSimpleTypes() ) br();
        }
        virtual void beforeDim ( char *, TypeInfo * ti ) override {
            if ( hasFlag(PrintFlags::fullTypeInfo) ) {
                ss << "fixed_array<" << debug_type_no_dim(ti) << ">(";
            } else {
                ss << "[";
            }
            if ( !ti->isDimOfSimpleType() ) br();
        }
        virtual void afterDim ( char *, TypeInfo * ti ) override {
            if ( hasFlag(PrintFlags::fullTypeInfo) ) {
                ss << ")";
            } else {
                ss << "]";
            }
            if ( !ti->isDimOfSimpleType() ) br();
        }
        virtual void beforeArray ( Array * arr, TypeInfo * ti ) override {
            if ( hasFlag(PrintFlags::fullTypeInfo) ) {
                ss << debug_type(ti) << "(";
            } else {
                ss << "[";
            }
            if ( hasFlag(PrintFlags::namesAndDimensions) ) {
                if ( arr->shared ) ss << " /*shared*/ ";
            }
            if ( hasFlag(PrintFlags::refAddresses) ) {
                ss << " data at 0x" << HEX << intptr_t(arr->data) << DEC;
            }
            if ( !ti->isArrayOfSimpleType() ) br();
        }
        virtual void afterArray ( Array *, TypeInfo * ti ) override {
            if ( hasFlag(PrintFlags::fullTypeInfo) ) {
                ss << ")";
            } else {
                ss << "]";
            }
            if ( !ti->isArrayOfSimpleType() ) br();
        }
        virtual void beforeTable ( Table * tab, TypeInfo * ti ) override {
            if ( hasFlag(PrintFlags::fullTypeInfo) ) {
                ss << debug_type(ti) << "(";
            } else {
                ss << "{";
            }
            if ( hasFlag(PrintFlags::namesAndDimensions) ) {
                if ( tab->shared ) ss << " /*shared*/ ";
            }
            if ( hasFlag(PrintFlags::refAddresses) ) {
                ss << " data at 0x" << HEX << intptr_t(tab->data) << DEC;
            }
            if ( !ti->isTableOfSimpleTypes() ) br();
        }
        virtual void afterTable ( Table *, TypeInfo * ti ) override {
            if ( hasFlag(PrintFlags::fullTypeInfo) ) {
                ss << ")";
            } else {
                ss << "}";
            }
            if ( !ti->isTableOfSimpleTypes() ) br();
        }
        virtual void beforeRef ( char * pa, TypeInfo * ti ) override {
            if ( hasFlag(PrintFlags::namesAndDimensions) ) {
                ss << "(" << debug_type(ti) << " 0x" << HEX << intptr_t(pa) << DEC << " ref = ";
            }
        }
        virtual void afterRef ( char *, TypeInfo * ) override {
            if ( hasFlag(PrintFlags::namesAndDimensions) ) {
                ss << ")";
            }
        }
        virtual void beforePtr ( char * pa, TypeInfo * ti ) override {
            if ( hasFlag(PrintFlags::namesAndDimensions) ) {
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
            if ( hasFlag(PrintFlags::namesAndDimensions) ) {
                ss << ")";
            }
        }
        virtual void beforeHandle ( char *, TypeInfo * ti ) override {
            if ( hasFlag(PrintFlags::namesAndDimensions) ) {
                ss << debug_type(ti) << "(";
            }
            br();
        }
        virtual void afterHandle ( char *, TypeInfo * ) override {
            if ( hasFlag(PrintFlags::namesAndDimensions) ) {
                ss << ")";
            }
            br();
        }
        virtual void afterHandleCancel ( char *, TypeInfo * ) override {
            if ( hasFlag(PrintFlags::namesAndDimensions) ) {
                ss << ")";
            }
            br();
        }
        virtual void beforeLambda ( Lambda *, TypeInfo * ti ) override {
            if ( hasFlag(PrintFlags::namesAndDimensions) ) {
                ss << "(" << debug_type(ti) << " ";
            }
        }
        virtual void afterLambda ( Lambda *, TypeInfo * ) override {
            if ( hasFlag(PrintFlags::namesAndDimensions) ) {
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
            if ( hasFlag(PrintFlags::typeQualifiers) ) {
                ss << "l";
            }
        }
        virtual void UInt64 ( uint64_t & ui ) override {
            ss << "0x" << HEX << ui << DEC;
            if ( hasFlag(PrintFlags::typeQualifiers) ) {
                ss << "ul";
            }
        }
        virtual void VoidPtr ( void * & p ) override {
            uint64_t ui = uint64_t(intptr_t(p));
            ss << "0x" << HEX << ui << DEC;
            if ( hasFlag(PrintFlags::typeQualifiers) ) {
                ss << "p";
            }
        }
        virtual void String ( char * & str ) override {
            string text = str ? str : "";
            if ( hasFlag(PrintFlags::escapeString) ) {
                ss << "\"" << escapeString(text) << "\"";
            } else {
                ss << text;
            }
            if ( hasFlag(PrintFlags::refAddresses) ) {
                ss << " /*0x" << HEX << intptr_t(str) << DEC << "*/";
            }
        }
        virtual void Float ( float & f ) override {
            bool typeQualifiers = hasFlag(PrintFlags::typeQualifiers);
            if ( typeQualifiers && isnan(f) ) {
                ss << "nan";
            } else if ( typeQualifiers && isinf(f) ) {
                ss << (f>0 ? "inf" : "-inf");
            } else {
                if ( hasFlag(PrintFlags::fixedFloatingPoint) )
                    ss << FIXEDFP << f << SCIENTIFIC;
                else
                    ss << f;
                if ( typeQualifiers ) {
                    ss << "f";
                }
            }
        }
        virtual void Double ( double & f ) override {
            bool typeQualifiers = hasFlag(PrintFlags::typeQualifiers);
            if ( typeQualifiers && isnan(f) ) {
                ss << "nan";
            } else if ( typeQualifiers && isinf(f) ) {
                ss << (f>0 ? "inf" : "-inf");
            } else {
                if ( hasFlag(PrintFlags::fixedFloatingPoint) )
                    ss << FIXEDFP << f << SCIENTIFIC;
                else
                    ss << f;
                if ( typeQualifiers ) {
                    ss << "lf";
                }
            }
        }
        virtual void Int ( int32_t & i ) override {
            ss << i;
        }
        virtual void UInt ( uint32_t & ui ) override {
            ss << "0x" << HEX << ui << DEC;
            if ( hasFlag(PrintFlags::typeQualifiers) ) {
                ss << "u";
            }
        }
        void anyBitfield ( uint64_t ui, TypeInfo * info ) {
            if ( info->argNames ) {
                if ( ui ) {
                    ss << "(";
                    bool any = false;
                    for ( uint64_t bit=0, bits=info->argCount; bit!=bits; ++bit ) {
                        if ( ui & (1ull<<uint64_t(bit)) ) {
                            if ( any ) ss << "|"; else any = true;
                            ss << info->argNames[bit];
                            if ( hasFlag(PrintFlags::namesAndDimensions) ) {
                                ss << "(" << (1ull<<bit) << ")";
                            }
                        }
                    }
                    ss << ")";
                } else {
                    ss << "(0)";
                }
            } else {
                ss << "0x" << HEX << ui << DEC;
                if ( hasFlag(PrintFlags::typeQualifiers) ) {
                    ss << "u";
                }
            }
        }
        virtual void Bitfield ( uint32_t & ui, TypeInfo * info ) override {
            anyBitfield( uint64_t(ui), info );
        }
        virtual void Bitfield8 ( uint8_t & ui, TypeInfo * info ) override {
            anyBitfield( uint64_t(ui), info );
        }
        virtual void Bitfield16 ( uint16_t & ui, TypeInfo * info ) override {
            anyBitfield( uint64_t(ui), info );
        }
        virtual void Bitfield64 ( uint64_t & ui, TypeInfo * info ) override {
            anyBitfield( ui, info );
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
        virtual void beforeIterator ( Sequence *, TypeInfo * ti ) override {
            ss << "iterator<" << (ti && ti->firstType ? debug_type(ti->firstType) : "") << ">";
        }
        virtual void afterIterator ( Sequence *, TypeInfo * ) override {
        }
        virtual void WalkBlock ( struct Block * pa ) override {
            ss << "block";
            if ( pa->body ) ss << HEX << getSemanticHash(pa->body, context) << DEC;
        }
        __forceinline void emitEnumName ( EnumInfo * info ) {
            if ( hasFlag(PrintFlags::fullTypeInfo) && info->name ) {
                ss << info->name << ".";
            }
        }
        virtual void WalkEnumeration ( int32_t & value, EnumInfo * info ) override {
            int64_t uvalue = uint64_t(value);
            for ( uint32_t t=0, ts=info->count; t!=ts; ++t ) {
                if ( value == info->fields[t]->value || uvalue == info->fields[t]->value ) {
                    emitEnumName(info);
                    ss << info->fields[t]->name;
                    return;
                }
            }
            ss << "enum " << value;
        }
        virtual void WalkEnumeration8 ( int8_t & value, EnumInfo * info ) override {
            int64_t uvalue = uint8_t(value);
            for ( uint32_t t=0, ts=info->count; t!=ts; ++t ) {
                if ( value == info->fields[t]->value || uvalue == info->fields[t]->value ) {
                    emitEnumName(info);
                    ss << info->fields[t]->name;
                    return;
                }
            }
            ss << "enum " << value;
        }
        virtual void WalkEnumeration16 ( int16_t & value, EnumInfo * info ) override {
            int64_t uvalue = uint16_t(value);
            for ( uint32_t t=0, ts=info->count; t!=ts; ++t ) {
                if ( value == info->fields[t]->value || uvalue == info->fields[t]->value ) {
                    emitEnumName(info);
                    ss << info->fields[t]->name;
                    return;
                }
            }
            ss << "enum " << value;
        }
        virtual void WalkEnumeration64 ( int64_t & value, EnumInfo * info ) override {
            for ( uint32_t t=0, ts=info->count; t!=ts; ++t ) {
                if ( value == info->fields[t]->value ) {
                    emitEnumName(info);
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

    DAS_API string debug_value ( const void * pX, TypeInfo * info, PrintFlags flags );
    DAS_API string debug_value ( vec4f value, TypeInfo * info, PrintFlags flags );
    DAS_API string debug_json_value ( void * pX, TypeInfo * info, bool humanReadable );
    DAS_API string debug_json_value ( vec4f value, TypeInfo * info, bool humanReadable );
    DAS_API bool debug_json_scan ( Context & ctx, char * dst, TypeInfo * info, const char * json, uint32_t jsonLen, LineInfo * at );
}
