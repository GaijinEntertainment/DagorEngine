#include "daScript/misc/platform.h"

#include "daScript/simulate/debug_print.h"
#include "daScript/misc/fpe.h"
#include "daScript/misc/debug_break.h"

#include "daScript/ast/ast.h"


namespace das {

    struct JsonWriter : DataWalker {
        JsonWriter () { ss << FIXEDFP; }
        TextWriter ss;
        bool enumAsInt = false;
        bool unescape = false;
        bool embed = false;
        bool optional = false; // if true, we do not write zero values, only non-zero ones
        bool inTableKey = false; // vec/range emit as array form here to fit inside the outer "..." quote
        vector<bool> ignoreNextFields;
        vector<bool> anyStructFields;
    // data structures
        virtual void beforeStructure ( char *, StructInfo * ) override {
            ss << "{";
            anyStructFields.push_back(false);
        }
        virtual void afterStructure ( char *, StructInfo * ) override {
            ss << "}";
            if (!anyStructFields.empty()) {
                anyStructFields.pop_back();
            }
        }
        virtual void beforeStructureField ( char * /*ps*/, StructInfo *si, char * pf, VarInfo * vi, bool ) override {
            enumAsInt = false;
            unescape = false;
            embed = false;
            optional = false;
            string name = vi->name ? vi->name : "";
            for ( uint32_t ai=0, ais=vi->annotation_argument_count; ai!=ais; ++ai ) {
                const auto & arg = vi->annotation_arguments[ai];
                if ( strcmp(arg.name,"enum_as_int")==0 && arg.type==Type::tBool ) {
                    enumAsInt = arg.bValue;
                } else if ( strcmp(arg.name,"unescape")==0 && arg.type==Type::tBool ) {
                    unescape = arg.bValue;
                } else if ( strcmp(arg.name,"embed")==0 && arg.type==Type::tBool ) {
                    embed = arg.bValue;
                } else if ( strcmp(arg.name,"optional")==0 && arg.type==Type::tBool ) {
                    optional = arg.bValue;
                } else if ( strcmp(arg.name,"rename")==0 ) {
                    if ( arg.type==Type::tString ) {
                        name = arg.sValue;
                    } else if ( arg.type==Type::tBool && !name.empty() && name[0]=='_' ) {
                        name = name.substr(1);
                    }
                }
            }
            bool ignoreNextField = false;
            if ( si->flags & StructInfo::flag_class ) {
                // A class carries its methods as function-pointer fields. They aren't data —
                // skip them entirely (rather than emit "kek":null) along with the class plumbing.
                if (name == "__rtti" || name == "__finalize" || (vi->flags & TypeInfo::flag_classMethod)) {
                    ignoreNextField = true;
                }
            }
            if ( optional && !ignoreNextField ) {
                if ( vi->type==Type::tInt || vi->type==Type::tUInt ) {
                    auto val = *((uint32_t *)pf);
                    ignoreNextField = val == 0;
                } else if ( vi->type==Type::tInt8 || vi->type==Type::tUInt8 ) {
                    auto val = *((uint8_t *)pf);
                    ignoreNextField = val == 0;
                } else if ( vi->type==Type::tInt16 || vi->type==Type::tUInt16 ) {
                    auto val = *((uint16_t *)pf);
                    ignoreNextField = val == 0;
                } else if ( vi->type==Type::tInt64 || vi->type==Type::tUInt64 ) {
                    auto val = *((uint64_t *)pf);
                    ignoreNextField = val == 0;
                } else if ( vi->type==Type::tFloat ) {
                    auto val = *((float *)pf);
                    ignoreNextField = val == 0.f;
                } else if ( vi->type==Type::tDouble ) {
                    auto val = *((double *)pf);
                    ignoreNextField = val == 0.0;
                } else if ( vi->type==Type::tBool ) {
                    auto val = *((bool *)pf);
                    ignoreNextField = !val;
                } else if ( vi->type==Type::tString && vi->dimSize==0 ) {
                    auto st = *((char **) pf);
                    ignoreNextField = st==nullptr || strlen(st)==0;
                } else if ( vi->type==Type::tPointer ) {
                    auto ptr = *((void **) pf);
                    ignoreNextField = ptr==nullptr;
                } else if ( vi->type==Type::tArray && vi->dimSize==0 ) {
                    auto arr = (Array *) pf;
                    ignoreNextField = arr->size==0;
                } else if ( vi->type==Type::tTable && vi->dimSize==0 ) {
                    auto tab = (Table *) pf;
                    ignoreNextField = tab->size==0;
                }
            }
            if ( !ignoreNextField ) {
                if ( anyStructFields.back() ) ss << ",";
                ss << "\"" << name << "\":";
                anyStructFields.back() = true;
            }
            ignoreNextFields.push_back(ignoreNextField);
        }
        virtual bool canVisitArray ( Array * /*ar*/, TypeInfo * ) override {
            return ignoreNextFields.empty() || !ignoreNextFields.back();
        }
        virtual bool canVisitTable ( char * /*ps*/, TypeInfo * ) override {
            return ignoreNextFields.empty() || !ignoreNextFields.back();
        }
        virtual void afterStructureField ( char *, StructInfo *, char *, VarInfo *, bool ) override {
            enumAsInt = false;
            unescape = false;
            embed = false;
            optional = false;
            if ( !ignoreNextFields.empty() ) {
                ignoreNextFields.pop_back();
            }
        }
        virtual void beforeTuple ( char *, TypeInfo * ) override {
            ss << "{";
        }
        virtual void afterTuple ( char *, TypeInfo * ) override {
            ss << "}";
        }
        virtual void beforeTupleEntry ( char *, TypeInfo * /*ti*/, char *, int idx, bool ) override {
            ss << "\"_" << idx << "\":";
        }
        virtual void afterTupleEntry ( char *, TypeInfo *, char *, int, bool last ) override {
            if ( !last ) ss << ",";
        }
        virtual void beforeVariant ( char * ps, TypeInfo * ti ) override {
            int32_t fidx = *((int32_t *)ps);
            ss << "{\"" << ti->argNames[fidx] << "\":";
        }
        virtual void afterVariant ( char *, TypeInfo * ) override {
            ss << "}";
        }
        virtual void beforeArrayData ( char *, uint32_t, uint64_t, TypeInfo * ) override {
            ss << "[";
        }
        virtual void afterArrayData ( char *, uint32_t, uint64_t, TypeInfo * ) override {
            ss << "]";
        }
        virtual void afterArrayElement ( char *, TypeInfo *, char *, uint64_t, bool last ) override {
            if ( !last ) ss << ",";
        }
        virtual void beforeTable ( Table *, TypeInfo * ) override {
            ss << "{";
        }
        virtual void beforeTableKey ( Table *, TypeInfo *, char *, TypeInfo * ki, uint64_t, bool ) override {
            if ( ki->type!=Type::tString ) {
                ss << "\"";
                inTableKey = true;
            }
        }
        virtual void afterTableKey ( Table *, TypeInfo *, char *, TypeInfo * ki, uint64_t, bool ) override {
            if ( ki->type!=Type::tString ) {
                ss << "\":";
                inTableKey = false;
            } else {
                ss << ":";
            }
        }
        virtual void afterTableValue ( Table *, TypeInfo *, char *, TypeInfo *, uint64_t, bool last ) override {
            if ( !last ) ss << ",";
        }
        virtual void afterTable ( Table *, TypeInfo * ) override {
            ss << "}";
        }
    // types
        virtual void Null ( TypeInfo * ) override {
            if ( !ignoreNextFields.empty() && ignoreNextFields.back() ) return;
            ss << "null";
        }
        virtual void Bool ( bool & value ) override {
            if ( !ignoreNextFields.empty() && ignoreNextFields.back() ) return;
            ss << (value ? "true" : "false");
        }
        virtual void Int8 ( int8_t & value ) override {
            if ( !ignoreNextFields.empty() && ignoreNextFields.back() ) return;
            ss << int32_t(value);
        }
        virtual void UInt8 ( uint8_t & value ) override {
            if ( !ignoreNextFields.empty() && ignoreNextFields.back() ) return;
            ss << int32_t(value);
        }
        virtual void Int16 ( int16_t & value ) override {
            if ( !ignoreNextFields.empty() && ignoreNextFields.back() ) return;
            ss << int32_t(value);
        }
        virtual void UInt16 ( uint16_t & value ) override {
            if ( !ignoreNextFields.empty() && ignoreNextFields.back() ) return;
            ss << int32_t(value);
        }
        virtual void Int64 ( int64_t & value ) override {
            if ( !ignoreNextFields.empty() && ignoreNextFields.back() ) return;
            ss << value;
        }
        virtual void UInt64 ( uint64_t & value ) override {
            if ( !ignoreNextFields.empty() && ignoreNextFields.back() ) return;
            ss << int64_t(value);
        }
        virtual void String ( char * & value ) override {
            if ( !ignoreNextFields.empty() && ignoreNextFields.back() ) return;
            if ( unescape ) {
                ss << "\"" << value << "\"";
            } else if ( embed ) {
                ss << value;
            } else {
                ss << "\"" << escapeString(value ? value : "",false) << "\"";
            }
        }
        virtual void Double ( double & value ) override {
            if ( !ignoreNextFields.empty() && ignoreNextFields.back() ) return;
            ss << value;
        }
        virtual void Float ( float & value ) override {
            if ( !ignoreNextFields.empty() && ignoreNextFields.back() ) return;
            ss << value;
        }
        virtual void Int ( int32_t & value ) override {
            if ( !ignoreNextFields.empty() && ignoreNextFields.back() ) return;
            ss << value;
        }
        virtual void UInt ( uint32_t & value ) override {
            if ( !ignoreNextFields.empty() && ignoreNextFields.back() ) return;
            ss << int64_t(value);
        }
        virtual void Bitfield ( uint32_t & value, TypeInfo * ) override {
            ss << value;
        }
        virtual void Bitfield8 ( uint8_t & value, TypeInfo * ) override {
            ss << int32_t(value);
        }
        virtual void Bitfield16 ( uint16_t & value, TypeInfo * ) override {
            ss << int32_t(value);
        }
        virtual void Bitfield64 ( uint64_t & value, TypeInfo * ) override {
            ss << int64_t(value);
        }
        virtual void Int2 ( int2 & value ) override {
            if ( inTableKey ) ss << "[" << value.x << "," << value.y << "]";
            else ss << "{\"x\":" << value.x << ",\"y\":" << value.y << "}";
        }
        virtual void Int3 ( int3 & value ) override {
            if ( inTableKey ) ss << "[" << value.x << "," << value.y << "," << value.z << "]";
            else ss << "{\"x\":" << value.x << ",\"y\":" << value.y << ",\"z\":" << value.z << "}";
        }
        virtual void Int4 ( int4 & value ) override {
            if ( inTableKey ) ss << "[" << value.x << "," << value.y << "," << value.z << "," << value.w << "]";
            else ss << "{\"x\":" << value.x << ",\"y\":" << value.y << ",\"z\":" << value.z << ",\"w\":" << value.w << "}";
        }
        virtual void UInt2 ( uint2 & value ) override {
            if ( inTableKey ) ss << "[" << int64_t(value.x) << "," << int64_t(value.y) << "]";
            else ss << "{\"x\":" << int64_t(value.x) << ",\"y\":" << int64_t(value.y) << "}";
        }
        virtual void UInt3 ( uint3 & value ) override {
            if ( inTableKey ) ss << "[" << int64_t(value.x) << "," << int64_t(value.y) << "," << int64_t(value.z) << "]";
            else ss << "{\"x\":" << int64_t(value.x) << ",\"y\":" << int64_t(value.y) << ",\"z\":" << int64_t(value.z) << "}";
        }
        virtual void UInt4 ( uint4 & value ) override {
            if ( inTableKey ) ss << "[" << int64_t(value.x) << "," << int64_t(value.y) << "," << int64_t(value.z) << "," << int64_t(value.w) << "]";
            else ss << "{\"x\":" << int64_t(value.x) << ",\"y\":" << int64_t(value.y) << ",\"z\":" << int64_t(value.z) << ",\"w\":" << int64_t(value.w) << "}";
        }
        virtual void Float2 ( float2 & value ) override {
            if ( inTableKey ) ss << "[" << value.x << "," << value.y << "]";
            else ss << "{\"x\":" << value.x << ",\"y\":" << value.y << "}";
        }
        virtual void Float3 ( float3 & value ) override {
            if ( inTableKey ) ss << "[" << value.x << "," << value.y << "," << value.z << "]";
            else ss << "{\"x\":" << value.x << ",\"y\":" << value.y << ",\"z\":" << value.z << "}";
        }
        virtual void Float4 ( float4 & value ) override {
            if ( inTableKey ) ss << "[" << value.x << "," << value.y << "," << value.z << "," << value.w << "]";
            else ss << "{\"x\":" << value.x << ",\"y\":" << value.y << ",\"z\":" << value.z << ",\"w\":" << value.w << "}";
        }
        // 16/8-bit lattice: arity <=4 mirrors the xyzw object/array form of Float2..4;
        // 8/16-lane forms are always JSON arrays (no field names past w)
        template <typename TT, typename PT>
        void printSVec2 ( const TT & value ) {
            if ( inTableKey ) ss << "[" << PT(value.x) << "," << PT(value.y) << "]";
            else ss << "{\"x\":" << PT(value.x) << ",\"y\":" << PT(value.y) << "}";
        }
        template <typename TT, typename PT>
        void printSVec3 ( const TT & value ) {
            if ( inTableKey ) ss << "[" << PT(value.x) << "," << PT(value.y) << "," << PT(value.z) << "]";
            else ss << "{\"x\":" << PT(value.x) << ",\"y\":" << PT(value.y) << ",\"z\":" << PT(value.z) << "}";
        }
        template <typename TT, typename PT>
        void printSVec4 ( const TT & value ) {
            if ( inTableKey ) ss << "[" << PT(value.x) << "," << PT(value.y) << "," << PT(value.z) << "," << PT(value.w) << "]";
            else ss << "{\"x\":" << PT(value.x) << ",\"y\":" << PT(value.y) << ",\"z\":" << PT(value.z) << ",\"w\":" << PT(value.w) << "}";
        }
        template <typename TT, typename PT, int N>
        void printSVecN ( const TT & value ) {
            ss << "[";
            for ( int i = 0; i != N; ++i ) {
                if ( i ) ss << ",";
                ss << PT(value.s[i]);
            }
            ss << "]";
        }
        static __forceinline float f16v ( const float16_t & h ) { return h.toFloat(); }
        virtual void Float16 ( float16_t & value ) override {
            ss << value.toFloat();
        }
        virtual void Half2 ( half2 & value ) override {
            if ( inTableKey ) ss << "[" << f16v(value.x) << "," << f16v(value.y) << "]";
            else ss << "{\"x\":" << f16v(value.x) << ",\"y\":" << f16v(value.y) << "}";
        }
        virtual void Half3 ( half3 & value ) override {
            if ( inTableKey ) ss << "[" << f16v(value.x) << "," << f16v(value.y) << "," << f16v(value.z) << "]";
            else ss << "{\"x\":" << f16v(value.x) << ",\"y\":" << f16v(value.y) << ",\"z\":" << f16v(value.z) << "}";
        }
        virtual void Half4 ( half4 & value ) override {
            if ( inTableKey ) ss << "[" << f16v(value.x) << "," << f16v(value.y) << "," << f16v(value.z) << "," << f16v(value.w) << "]";
            else ss << "{\"x\":" << f16v(value.x) << ",\"y\":" << f16v(value.y) << ",\"z\":" << f16v(value.z) << ",\"w\":" << f16v(value.w) << "}";
        }
        virtual void Half8 ( half8 & value ) override {
            ss << "[";
            for ( int i = 0; i != 8; ++i ) {
                if ( i ) ss << ",";
                ss << value.s[i].toFloat();
            }
            ss << "]";
        }
        virtual void Short2 ( short2 & value ) override { printSVec2<short2, int32_t>(value); }
        virtual void Short3 ( short3 & value ) override { printSVec3<short3, int32_t>(value); }
        virtual void Short4 ( short4 & value ) override { printSVec4<short4, int32_t>(value); }
        virtual void Short8 ( short8 & value ) override { printSVecN<short8, int32_t, 8>(value); }
        virtual void UShort2 ( ushort2 & value ) override { printSVec2<ushort2, uint32_t>(value); }
        virtual void UShort3 ( ushort3 & value ) override { printSVec3<ushort3, uint32_t>(value); }
        virtual void UShort4 ( ushort4 & value ) override { printSVec4<ushort4, uint32_t>(value); }
        virtual void UShort8 ( ushort8 & value ) override { printSVecN<ushort8, uint32_t, 8>(value); }
        virtual void Byte2 ( byte2 & value ) override { printSVec2<byte2, int32_t>(value); }
        virtual void Byte3 ( byte3 & value ) override { printSVec3<byte3, int32_t>(value); }
        virtual void Byte4 ( byte4 & value ) override { printSVec4<byte4, int32_t>(value); }
        virtual void Byte8 ( byte8 & value ) override { printSVecN<byte8, int32_t, 8>(value); }
        virtual void Byte16 ( byte16 & value ) override { printSVecN<byte16, int32_t, 16>(value); }
        virtual void UByte2 ( ubyte2 & value ) override { printSVec2<ubyte2, uint32_t>(value); }
        virtual void UByte3 ( ubyte3 & value ) override { printSVec3<ubyte3, uint32_t>(value); }
        virtual void UByte4 ( ubyte4 & value ) override { printSVec4<ubyte4, uint32_t>(value); }
        virtual void UByte8 ( ubyte8 & value ) override { printSVecN<ubyte8, uint32_t, 8>(value); }
        virtual void UByte16 ( ubyte16 & value ) override { printSVecN<ubyte16, uint32_t, 16>(value); }
        virtual void Range ( range & value ) override {
            ss << "[" << value.x << "," << value.y << "]";
        }
        virtual void URange ( urange & value ) override {
            ss << "[" << int64_t(value.x) << "," << int64_t(value.y) << "]";
        }
        virtual void Range64 ( range64 & value ) override {
            ss << "[" << value.x << "," << value.y << "]";
        }
        virtual void URange64 ( urange64 & value ) override {
            ss << "[" << int64_t(value.x) << "," << int64_t(value.y) << "]";
        }
        virtual void VoidPtr ( void * & ) override {
            if ( !ignoreNextFields.empty() && ignoreNextFields.back() ) return;
            ss << "null";
        }
        void Enum ( int64_t value, EnumInfo * info ) {
            if ( enumAsInt ) {
                ss << value;
            } else {
                for ( uint32_t t=0; t!=info->count; ++t ) {
                    if ( info->fields[t]->value==value ) {
                        if ( auto name = info->fields[t]->name ) {
                            ss << "\"" << name << "\"";
                        } else {
                            ss << value;
                        }
                        return;
                    }
                }
                // No enumerator matches this value (e.g. an OR-combined flag enum like
                // `OpenOnArrow | DefaultOpen`). Emit the number rather than nothing — emitting
                // nothing produced invalid JSON ({"flags": ,...}) and dropped the whole object.
                // The numeric form round-trips: scanEnum already accepts a bare integer.
                ss << value;
            }
        }
        virtual void WalkEnumeration ( int32_t & value, EnumInfo * info ) override {
            // Same sign-extension trap as the 8/16-bit walkers: uint32-backed values > INT32_MAX
            // come in as negative int32_t, promote to negative int64_t, and miss the lookup against
            // the positive int64_t-stored field value.
            int64_t v = (info && (info->flags & EnumInfo::flag_unsigned))
                ? int64_t(uint32_t(value)) : int64_t(value);
            Enum(v,info);
        }
        virtual void WalkEnumeration8  ( int8_t & value, EnumInfo * info ) override {
            // For uint8-backed enums the byte represents an unsigned value; promoting via int8_t
            // sign-extends so the lookup against the int64_t-stored field value would miss.
            int64_t v = (info && (info->flags & EnumInfo::flag_unsigned))
                ? int64_t(uint8_t(value)) : int64_t(value);
            Enum(v,info);
        }
        virtual void WalkEnumeration16 ( int16_t & value, EnumInfo * info ) override {
            int64_t v = (info && (info->flags & EnumInfo::flag_unsigned))
                ? int64_t(uint16_t(value)) : int64_t(value);
            Enum(v,info);
        }
        virtual void WalkEnumeration64 ( int64_t & value, EnumInfo * info ) override {
            // No promotion happens here — int64_t reads at its native width, so signedness can't
            // wrap during the implicit conversion to Enum()'s int64_t parameter. The lookup against
            // uint64_t-backed values with the top bit set will compare bit-for-bit equal because the
            // stored value already round-tripped through int64_t at EnumValueInfo build time.
            Enum(value,info);
        }
        // A function pointer / block isn't JSON data. Emit `null` rather than nothing so the output
        // stays valid JSON. Class methods are dropped earlier (flag_classMethod); this still guards
        // the __lambda/__finalize fields inside a lambda's capture struct and any function-typed
        // field, which otherwise left a dangling `:` and produced invalid JSON.
        virtual void WalkFunction ( Func * ) override {
            if ( !ignoreNextFields.empty() && ignoreNextFields.back() ) return;
            ss << "null";
        }
        virtual void WalkBlock ( Block * ) override {
            if ( !ignoreNextFields.empty() && ignoreNextFields.back() ) return;
            ss << "null";
        }

        virtual bool revisitStructure ( char * /*ps*/, StructInfo * /*si*/ ) override {
            ss << "null";
            return false;
        }
        virtual bool revisitHandle ( char * /*ps*/, TypeInfo * /*ti*/ ) override {
            ss << "null";
            return false;
        }
    };

    string human_readable_json ( const string & str ) {
        string result;
        string tab;
        auto it = str.begin();
        auto tail = str.end();
        bool inString = false;
        bool nextIsEscape = false;
        while ( it != tail ) {
            while ( it!=tail && *it==' ') {
                it ++;
            }
            while ( it!=tail && *it!='\n' ) {
                auto Ch = *it;
                if ( !inString && (Ch=='[' || Ch=='{') ) {
                    result += Ch;
                    tab += "  ";
                    result += '\n';
                    result += tab;
                } else if ( !inString && (Ch==']' || Ch=='}') ) {
                    result += '\n';
                    if ( tab.size()>=2 ) tab.resize(tab.size()-2);
                    result += tab;
                    result += Ch;
                } else if ( !inString && Ch==',' ) {
                    result += Ch;
                    result += '\n';
                    result += tab;
                } else if ( inString && Ch=='\\' && !nextIsEscape ) {
                    result += Ch;
                    nextIsEscape = true;
                } else if ( Ch=='"' ) {
                    result += Ch;
                    if ( nextIsEscape ) {
                        nextIsEscape = false;
                    } else {
                        inString = !inString;
                    }
                } else {
                    result += Ch;
                    nextIsEscape = false;
                }
                it ++;
            }
            if ( it == tail ) break;
            while ( it!=tail && *it=='\n' ) {
                it ++;
            }
            result += "\n";
            result += tab;
        }
        return result;
    }
    string debug_json_value ( void * pX, TypeInfo * info, bool humanReadable ) {
        JsonWriter walker;
        walker.walk((char*)pX,info);
        if ( humanReadable ) {
            return human_readable_json(walker.ss.str());
        } else {
            return walker.ss.str();
        }
    }

    string debug_json_value ( vec4f value, TypeInfo * info, bool humanReadable ) {
        JsonWriter walker;
         walker.walk(value,info);
        if ( humanReadable ) {
            return human_readable_json(walker.ss.str());
        } else {
            return walker.ss.str();
        }
    }
}