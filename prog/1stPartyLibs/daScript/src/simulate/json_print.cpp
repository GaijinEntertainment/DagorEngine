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
        bool optional = false;
        bool skipName = false;
    // data structures
        virtual void beforeStructure ( char *, StructInfo * ) override {
            ss << "{";
        }
        virtual void afterStructure ( char *, StructInfo * ) override {
            ss << "}";
        }
        virtual void beforeStructureField ( char * ps, StructInfo *, char * pf, VarInfo * vi, bool ) override {
            enumAsInt = false;
            unescape = false;
            embed = false;
            optional = false;
            skipName = false;
            string name = vi->name ? vi->name : "";
            if ( vi->annotation_arguments ) {
                auto aa = (AnnotationArguments *) vi->annotation_arguments;
                for ( auto arg : *aa ) {
                    if ( arg.name=="enum_as_int" && arg.type==Type::tBool ) {
                        enumAsInt = arg.bValue;
                    } else if ( arg.name=="unescape" && arg.type==Type::tBool ) {
                        unescape = arg.bValue;
                    } else if ( arg.name=="embed" && arg.type==Type::tBool ) {
                        embed = arg.bValue;
                    } else if ( arg.name=="optional" && arg.type==Type::tBool ) {
                        optional = arg.bValue;
                    } else if ( arg.name=="rename" && arg.type==Type::tString ) {
                        name = arg.sValue;
                    }
                }
            }
            if ( optional ) {
                if ( vi->type==Type::tString && vi->dimSize==0 ) {
                    auto st = *((char **) pf);
                    skipName =st==nullptr || strlen(st)==0;
                } else if ( vi->type==Type::tArray && vi->dimSize==0 ) {
                    auto arr = (Array *) pf;
                    skipName = arr->size==0;
                } else if ( vi->type==Type::tTable && vi->dimSize==0 ) {
                    auto tab = (Table *) pf;
                    skipName = tab->size==0;
                }
            }
            if ( !skipName ) {
                if ( ps!=pf ) ss << ",";
                ss << "\"" << name << "\":";
            }
        }
        virtual bool canVisitArray ( Array * ar, TypeInfo * ) override {
            if ( optional && ar->size==0 ) return false;
            return true;
        }
        virtual bool canVisitTable ( char * ps, TypeInfo * ) override {
            if ( optional ) {
                Table * tab = (Table *)ps;
                if ( tab->size==0 ) return false;
            }
            return true;
        }
        virtual void afterStructureField ( char *, StructInfo *, char *, VarInfo *, bool ) override {
            enumAsInt = false;
            unescape = false;
            embed = false;
            optional = false;
            skipName = true;
        }
        virtual void beforeTuple ( char *, TypeInfo * ) override {
            ss << "{";
        }
        virtual void afterTuple ( char *, TypeInfo * ) override {
            ss << "}";
        }
        virtual void beforeTupleEntry ( char *, TypeInfo * ti, char *, TypeInfo * vi, bool ) override {
            // TODO: we can actuallyss this, right?
            int32_t idx = -1u;
            for ( int32_t i=0, is=ti->argCount; i!=is; ++i ) {
                if ( ti->argTypes[i]==vi ) {
                    idx = i;
                    break;
                }
            }
            ss << "\"_" << idx << "\":";
        }
        virtual void afterTupleEntry ( char *, TypeInfo *, char *, TypeInfo *, bool last ) override {
            if ( !last ) ss << ",";
        }
        virtual void beforeVariant ( char * ps, TypeInfo * ti ) override {
            int32_t fidx = *((int32_t *)ps);
            ss << "{\"" << ti->argNames[fidx] << "\":";
        }
        virtual void afterVariant ( char *, TypeInfo * ) override {
            ss << "}";
        }
        virtual void beforeArrayData ( char *, uint32_t, uint32_t, TypeInfo * ) override {
            ss << "[";
        }
        virtual void afterArrayData ( char *, uint32_t, uint32_t, TypeInfo * ) override {
            ss << "]";
        }
        virtual void afterArrayElement ( char *, TypeInfo *, char *, uint32_t, bool last ) override {
            if ( !last ) ss << ",";
        }
        virtual void beforeTable ( Table *, TypeInfo * ) override {
            ss << "{";
        }
        virtual void beforeTableKey ( Table *, TypeInfo *, char *, TypeInfo * ki, uint32_t, bool ) override {
            if ( ki->type!=Type::tString ) ss << "\"";
        }
        virtual void afterTableKey ( Table *, TypeInfo *, char *, TypeInfo * ki, uint32_t, bool ) override {
            if ( ki->type!=Type::tString ) ss << "\":"; else ss << ":";
        }
        virtual void afterTableValue ( Table *, TypeInfo *, char *, TypeInfo *, uint32_t, bool last ) override {
            if ( !last ) ss << ",";
        }
        virtual void afterTable ( Table *, TypeInfo * ) override {
            ss << "}";
        }
    // types
        virtual void Null ( TypeInfo * ) override {
            ss << "null";
        }
        virtual void Bool ( bool & value ) override {
            ss << (value ? "true" : "false");
        }
        virtual void Int8 ( int8_t & value ) override {
            ss << value;
        }
        virtual void UInt8 ( uint8_t & value ) override {
            ss << int32_t(value);
        }
        virtual void Int16 ( int16_t & value ) override {
            ss << value;
        }
        virtual void UInt16 ( uint16_t & value ) override {
            ss << int32_t(value);
        }
        virtual void Int64 ( int64_t & value ) override {
            ss << value;
        }
        virtual void UInt64 ( uint64_t & value ) override {
            ss << int64_t(value);
        }
        virtual void String ( char * & value ) override {
            if ( optional && (value==nullptr || strlen(value)==0) ) return;
            if ( unescape ) {
                ss << "\"" << value << "\"";
            } else if ( embed ) {
                ss << value;
            } else {
                ss << "\"" << escapeString(value ? value : "",false) << "\"";
            }
        }
        virtual void Double ( double & value ) override {
            ss << value;
        }
        virtual void Float ( float & value ) override {
            ss << value;
        }
        virtual void Int ( int32_t & value ) override {
            ss << value;
        }
        virtual void UInt ( uint32_t & value ) override {
            ss << int64_t(value);
        }
        virtual void Bitfield ( uint32_t & value, TypeInfo * ) override {
            ss << value;
        }
        virtual void Int2 ( int2 & value ) override {
            ss << "[" << value.x << "," << value.y << "]";
        }
        virtual void Int3 ( int3 & value ) override {
            ss << "[" << value.x << "," << value.y << "," << value.z << "]";
        }
        virtual void Int4 ( int4 & value ) override {
            ss << "[" << value.x << "," << value.y << "," << value.z << "," << value.w << "]";
        }
        virtual void UInt2 ( uint2 & value ) override {
            ss << "[" << int64_t(value.x) << "," << int64_t(value.y) << "]";
        }
        virtual void UInt3 ( uint3 & value ) override {
            ss << "[" << int64_t(value.x) << "," << int64_t(value.y) << "," << int64_t(value.z) << "]";
        }
        virtual void UInt4 ( uint4 & value ) override {
            ss << "[" << int64_t(value.x) << "," << int64_t(value.y) << "," << int64_t(value.z) << "," << int64_t(value.w) << "]";
        }
        virtual void Float2 ( float2 & value ) override {
            ss << "[" << value.x << "," << value.y << "]";
        }
        virtual void Float3 ( float3 & value ) override {
            ss << "[" << value.x << "," << value.y << "," << value.z << "]";
        }
        virtual void Float4 ( float4 & value ) override {
            ss << "[" << value.x << "," << value.y << "," << value.z << "," << value.w << "]";
        }
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
            }
        }
        virtual void WalkEnumeration ( int32_t & value, EnumInfo * info ) override {
            Enum(value,info);
        }
        virtual void WalkEnumeration8  ( int8_t & value, EnumInfo * info ) override {
            Enum(value,info);
        }
        virtual void WalkEnumeration16 ( int16_t & value, EnumInfo * info ) override {
            Enum(value,info);
        }
        virtual void WalkEnumeration64 ( int64_t & value, EnumInfo * info ) override {
            Enum(value,info);
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