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
        virtual void beforeStructureField ( char * ps, StructInfo *si, char * pf, VarInfo * vi, bool ) override {
            enumAsInt = false;
            unescape = false;
            embed = false;
            optional = false;
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
            bool ignoreNextField = false;
            if ( si->flags & StructInfo::flag_class ) {
                if (name == "__rtti" || name == "__finalize") {
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
        virtual bool canVisitArray ( Array * ar, TypeInfo * ) override {
            return ignoreNextFields.empty() || !ignoreNextFields.back();
        }
        virtual bool canVisitTable ( char * ps, TypeInfo * ) override {
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
        virtual void beforeTupleEntry ( char *, TypeInfo * ti, char *, int idx, bool ) override {
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
            if ( !ignoreNextFields.empty() && ignoreNextFields.back() ) return;
            ss << "null";
        }
        virtual void Bool ( bool & value ) override {
            if ( !ignoreNextFields.empty() && ignoreNextFields.back() ) return;
            ss << (value ? "true" : "false");
        }
        virtual void Int8 ( int8_t & value ) override {
            if ( !ignoreNextFields.empty() && ignoreNextFields.back() ) return;
            ss << value;
        }
        virtual void UInt8 ( uint8_t & value ) override {
            if ( !ignoreNextFields.empty() && ignoreNextFields.back() ) return;
            ss << int32_t(value);
        }
        virtual void Int16 ( int16_t & value ) override {
            if ( !ignoreNextFields.empty() && ignoreNextFields.back() ) return;
            ss << value;
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

        virtual bool revisitStructure ( char * ps, StructInfo * si ) override {
            ss << "null";
            return false;
        }
        virtual bool revisitHandle ( char * ps, TypeInfo * ti ) override {
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