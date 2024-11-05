#include "daScript/misc/platform.h"

#include "daScript/simulate/debug_info.h"
#include "daScript/ast/ast.h"

#include "daScript/misc/enums.h"
#include "daScript/misc/arraytype.h"
#include "daScript/misc/vectypes.h"
#include "daScript/misc/rangetype.h"

namespace das
{
    Enum<Type> g_typeTable = {
        {   Type::autoinfer,    "auto"  },
        {   Type::alias,        "alias"  },
        {   Type::option,       "option"  },
        {   Type::typeDecl,     "typedecl" },
        {   Type::typeMacro,    "typemacro" },
        {   Type::anyArgument,  "any"  },
        {   Type::tVoid,        "void"  },
        {   Type::tBool,        "bool"  },
        {   Type::tInt8,        "int8"  },
        {   Type::tUInt8,       "uint8"  },
        {   Type::tInt16,       "int16"  },
        {   Type::tUInt16,      "uint16"  },
        {   Type::tInt64,       "int64"  },
        {   Type::tUInt64,      "uint64"  },
        {   Type::tString,      "string" },
        {   Type::tPointer,     "pointer" },
        {   Type::tEnumeration, "enumeration" },
        {   Type::tEnumeration8,"enumeration8" },
        {   Type::tEnumeration16, "enumeration16" },
        {   Type::tEnumeration64, "enumeration64" },
        {   Type::tIterator,    "iterator" },
        {   Type::tArray,       "array" },
        {   Type::tTable,       "table" },
        {   Type::tInt,         "int"   },
        {   Type::tInt2,        "int2"  },
        {   Type::tInt3,        "int3"  },
        {   Type::tInt4,        "int4"  },
        {   Type::tUInt,        "uint"  },
        {   Type::tBitfield,    "bitfield"  },
        {   Type::tUInt2,       "uint2" },
        {   Type::tUInt3,       "uint3" },
        {   Type::tUInt4,       "uint4" },
        {   Type::tFloat,       "float" },
        {   Type::tFloat2,      "float2"},
        {   Type::tFloat3,      "float3"},
        {   Type::tFloat4,      "float4"},
        {   Type::tDouble,      "double" },
        {   Type::tRange,       "range" },
        {   Type::tURange,      "urange"},
        {   Type::tRange64,     "range64" },
        {   Type::tURange64,    "urange64"},
        {   Type::tBlock,       "block"},
        {   Type::tFunction,    "function"},
        {   Type::tLambda,      "lambda"},
        {   Type::tTuple,       "tuple"},
        {   Type::tVariant,     "variant"},
        {   Type::fakeContext,  "__context"},
        {   Type::fakeLineInfo,  "__lineInfo"},
    };

    TypeAnnotation * TypeInfo::getAnnotation() const {
        if ( type != Type::tHandle ) {
            return nullptr;
        }
        return Module::resolveAnnotation(this);
    }

    StructInfo * TypeInfo::getStructType() const {
        if ( type != Type::tStructure ) {
            return nullptr;
        }
        return structType;
    }
    EnumInfo * TypeInfo::getEnumType() const {
        if ( type != Type::tEnumeration && type != Type::tEnumeration8 && type != Type::tEnumeration16 && type != Type::tEnumeration64 ) {
            return nullptr;
        }
        return enumType;
    }

    void TypeInfo::resolveAnnotation() const {
        if ( daScriptEnvironment::bound->modules ) Module::resolveAnnotation(this);
    }


    string das_to_string ( Type t ) {
        return g_typeTable.find(t);
    }

    Type nameToBasicType(const string & name) {
        return g_typeTable.find(name, Type::none);
    }

    int getTypeBaseSize ( Type type ) {
        switch ( type ) {
            case anyArgument:   return sizeof(vec4f);
            case tPointer:      return sizeof(void *);
            case tIterator:     return sizeof(Sequence);
            case tHandle:       DAS_ASSERTF(0, "we should not be here. if this happens, iterator was somehow placed on stack. how?");
                                return sizeof(void *);
            case tString:       return sizeof(char *);
            case tBool:         return sizeof(bool);            static_assert(sizeof(bool)==1,"4 byte bool");
            case tInt8:         return sizeof(int8_t);
            case tUInt8:        return sizeof(uint8_t);
            case tInt16:        return sizeof(int16_t);
            case tUInt16:       return sizeof(uint16_t);
            case tInt64:        return sizeof(int64_t);
            case tUInt64:       return sizeof(uint64_t);
            case tEnumeration:  return sizeof(int32_t);
            case tEnumeration8: return sizeof(int8_t);
            case tEnumeration16:return sizeof(int16_t);
            case tEnumeration64:return sizeof(int64_t);
            case tInt:          return sizeof(int);
            case tInt2:         return sizeof(int2);
            case tInt3:         return sizeof(int3);
            case tInt4:         return sizeof(int4);
            case tUInt:         return sizeof(uint32_t);
            case tBitfield:     return sizeof(uint32_t);
            case tUInt2:        return sizeof(uint2);
            case tUInt3:        return sizeof(uint3);
            case tUInt4:        return sizeof(uint4);
            case tFloat:        return sizeof(float);
            case tFloat2:       return sizeof(float2);
            case tFloat3:       return sizeof(float3);
            case tFloat4:       return sizeof(float4);
            case tDouble:       return sizeof(double);
            case tRange:        return sizeof(range);
            case tURange:       return sizeof(urange);
            case tRange64:      return sizeof(range64);
            case tURange64:     return sizeof(urange64);
            case tArray:        return sizeof(Array);
            case tTable:        return sizeof(Table);
            case tVoid:         return 0;
            case tBlock:        return sizeof(Block);
            case tFunction:     return sizeof(Func);
            case tLambda:       return sizeof(Lambda);
            case tStructure:    return 0;
            case tTuple:        return 0;
            case tVariant:      return 0;
            case fakeContext:   return 0;
            case fakeLineInfo:  return 0;
            default:
                DAS_ASSERTF(0, "not implemented. likely new built-intype been added, and support has not been updated.");
                return 0;
        }
    }

    int getTypeBaseAlign ( Type type ) {
        switch ( type ) {
            case anyArgument:   return alignof(vec4f);
            case tPointer:      return alignof(void *);
            case tIterator:     return alignof(Sequence);
            case tHandle:       DAS_ASSERTF(0, "we should not be here. if this happens iterator was somehow placed on stack. how?");
                                return alignof(void *);
            case tString:       return alignof(char *);
            case tBool:         return alignof(bool);            static_assert(alignof(bool)==1,"4 byte bool");
            case tInt8:         return alignof(int8_t);
            case tUInt8:        return alignof(uint8_t);
            case tInt16:        return alignof(int16_t);
            case tUInt16:       return alignof(uint16_t);
            case tInt64:        return alignof(int64_t);
            case tUInt64:       return alignof(uint64_t);
            case tEnumeration:  return alignof(int32_t);
            case tEnumeration8: return alignof(int8_t);
            case tEnumeration16:return alignof(int16_t);
            case tEnumeration64:return alignof(int64_t);
            case tInt:          return alignof(int32_t);
            case tInt2:         return alignof(int2);
            case tInt3:         return alignof(int3);
            case tInt4:         return alignof(int4);
            case tUInt:         return alignof(uint32_t);
            case tBitfield:     return alignof(uint32_t);
            case tUInt2:        return alignof(uint2);
            case tUInt3:        return alignof(uint3);
            case tUInt4:        return alignof(uint4);
            case tFloat:        return alignof(float);
            case tFloat2:       return alignof(float2);
            case tFloat3:       return alignof(float3);
            case tFloat4:       return alignof(float4);
            case tDouble:       return alignof(double);
            case tRange:        return alignof(range);
            case tURange:       return alignof(urange);
            case tRange64:      return alignof(range64);
            case tURange64:     return alignof(urange64);
            case tArray:        return alignof(Array);
            case tTable:        return alignof(Table);
            case tVoid:         return 1;
            case tBlock:        return alignof(Block);
            case tFunction:     return alignof(Func);
            case tLambda:       return alignof(Lambda);
            case tStructure:    return 1;
            case tTuple:        return 1;
            case tVariant:      return 1;
            case fakeContext:   return 1;
            case fakeLineInfo:  return 1;
            default:
                DAS_ASSERTF(0, "not implemented. likely new built-intype been added, and support has not been updated.");
                return 1;
        }
    }

    int getStructAlign ( StructInfo * info ) {
        int al = 0;
        for ( uint32_t i=0, is=info->count; i!=is; ++i ) {
            al = das::max ( al, getTypeAlign(info->fields[i]) );
        }
        return al;
    }

    int getTupleAlign ( TypeInfo * info ) {
        int al = 0;
        for ( uint32_t i=0, is=info->argCount; i!=is; ++i ) {
            al = das::max ( al, getTypeAlign(info->argTypes[i]) );
        }
        return al;
    }

    int getTupleSize ( TypeInfo * info ) {
        int size = 0;
        for ( uint32_t i=0, is=info->argCount; i!=is; ++i ) {
            int al = getTypeAlign(info->argTypes[i]) - 1;
            size = (size + al) & ~al;
            size += getTypeSize(info->argTypes[i]);
        }
        int al = getTupleAlign(info) - 1;
        size = (size + al) & ~al;
        return size;
    }

    int getTupleFieldOffset ( TypeInfo * info, int index ) {
        DAS_ASSERT(info->type==Type::tTuple);
        DAS_ASSERT(uint32_t(index)<info->argCount);
        int size = 0, idx = 0;
        for (int i = 0, n = (int)info->argCount; i != n; ++i) {
            TypeInfo * argT = info->argTypes[i];
            int al = getTypeAlign(argT) - 1;
            size = (size + al) & ~al;
            if ( idx==index ) {
                return size;
            }
            size += getTypeSize(argT);
            idx ++;
        }
        DAS_ASSERT(0 && "we should not even be here. field index out of range somehow???");
        return -1;
    }

    int getVariantAlign ( TypeInfo * info ) {
        int al = getTypeBaseAlign(Type::tInt);
        for ( uint32_t i=0, is=info->argCount; i!=is; ++i ) {
            al = das::max ( al, getTypeAlign(info->argTypes[i]) );
        }
        return al;
    }

    int getVariantSize ( TypeInfo * info ) {
        int maxSize = 0;
        int al = getVariantAlign(info) - 1;
        for ( uint32_t i=0, is=info->argCount; i!=is; ++i ) {
            int size = (getTypeBaseSize(Type::tInt) + al) & ~al;
            size += getTypeSize(info->argTypes[i]);
            maxSize = das::max(size, maxSize);
        }
        maxSize = (maxSize + al) & ~al;
        return maxSize;
    }

    int getVariantFieldOffset ( TypeInfo * info, int index ) {
        DAS_ASSERT(info->type==Type::tVariant);
        DAS_ASSERT(uint32_t(index)<info->argCount);
        int al = getVariantAlign(info) - 1;
        int offset = (getTypeBaseSize(Type::tInt) + al) & ~al;
        return offset;
    }

    int getTypeBaseSize ( TypeInfo * info ) {
        if ( info->type==Type::tHandle ) {
            return int(info->getAnnotation()->getSizeOf());
        } else if ( info->type==Type::tStructure ) {
            return info->structType->size;
        } else if ( info->type==Type::tTuple ) {
            return getTupleSize(info);
        } else if ( info->type==Type::tVariant ) {
            return getVariantSize(info);
        } else {
            return getTypeBaseSize(info->type);
        }
    }

    int getTypeBaseAlign ( TypeInfo * info ) {
        if ( info->type==Type::tHandle ) {
            return int(info->getAnnotation()->getAlignOf());
        } else if ( info->type==Type::tStructure ) {
            return getStructAlign(info->structType);
        } else if ( info->type==Type::tTuple ) {
            return getTupleAlign(info);
        } else if ( info->type==Type::tVariant ) {
            return getVariantAlign(info);
        } else {
            return getTypeBaseAlign(info->type);
        }
    }

    int getDimSize ( TypeInfo * info ) {
        int size = 1;
        if ( info->dimSize ) {
            for ( uint32_t i=0, is=info->dimSize; i!=is; ++i ) {
                size *= info->dim[i];
            }
        }
        return size;
    }

    int getTypeSize ( TypeInfo * info ) {
        return getDimSize(info) * getTypeBaseSize(info);
    }

    int getTypeAlign ( TypeInfo * info ) {
        return getTypeBaseAlign(info);
    }

    bool isVoid ( const TypeInfo * THIS ) {
        return (THIS->type==Type::tVoid) && (THIS->dimSize==0);
    }

    bool isPointer ( const TypeInfo * THIS ) {
        return (THIS->type==Type::tPointer) && (THIS->dimSize==0);
    }

    bool isValidArgumentType ( TypeInfo * argType, TypeInfo * passType ) {
        // passing non-ref to ref, or passing not the same type
        if ( (argType->isRef() && !passType->isRef()) || !isSameType(argType,passType,RefMatters::no, ConstMatters::no, TemporaryMatters::no,false) ) {
            return false;
        }
        // ref or pointer can only add const
        if ( (argType->isRef() || argType->type==Type::tPointer) && !argType->isConst() && passType->isConst() ) {
            return false;
        }
        // all good
        return true;
    }

    bool isMatchingArgumentType ( TypeInfo * argType, TypeInfo * passType) {
        if (!passType) {
            return false;
        }
        if ( argType->type==Type::anyArgument ) {
            return true;
        }
        // compare types which don't need inference
        auto tempMatters = argType->isImplicit() ? TemporaryMatters::no : TemporaryMatters::yes;
        if ( !isSameType(argType, passType, RefMatters::no, ConstMatters::no, tempMatters, true) ) {
            return false;
        }
        // can't pass non-ref to ref
        if ( argType->isRef() && !passType->isRef() ) {
            return false;
        }
        // ref types can only add constness
        if (argType->isRef() && !argType->isConst() && passType->isConst()) {
            return false;
        }
        // pointer types can only add constant
        if (isPointer(argType) && !argType->isConst() && passType->isConst()) {
            return false;
        }
        // all good
        return true;
    }

    bool isSameType ( const TypeInfo * THIS,
                     const TypeInfo * decl,
                     RefMatters refMatters,
                     ConstMatters constMatters,
                     TemporaryMatters temporaryMatters,
                     bool topLevel ) {
        if ( topLevel && !isPointer(THIS) && !THIS->isRef() ) {
            constMatters = ConstMatters::no;
        }
        if ( THIS->type != decl->type ) {
            return false;
        }
        if ( THIS->type==Type::tHandle && THIS->getAnnotation()!=decl->getAnnotation() ) {
            return false;
        }
        if ( THIS->type==Type::tStructure && THIS->structType!=decl->structType ) {
            return false;
        }
        if ( THIS->type==Type::tPointer || THIS->type==Type::tIterator ) {
            if ( (THIS->firstType && !isVoid(THIS->firstType))
                && (decl->firstType && !isVoid(decl->firstType))
                && !isSameType(THIS->firstType, decl->firstType, RefMatters::yes, ConstMatters::yes, TemporaryMatters::yes, false) ) {
                return false;
            }

        }
        if ( THIS->type==Type::tEnumeration || THIS->type==Type::tEnumeration8 ||
            THIS->type==Type::tEnumeration16 || THIS->type==Type::tEnumeration64 ) {
            if ( THIS->type != decl->type ) {
                return false;
            }
            if ( THIS->enumType && decl->enumType && THIS->enumType!=decl->enumType ) {
                return false;
            }
        }
        if ( THIS->type==Type::tArray ) {
            if ( THIS->firstType && decl->firstType && !isSameType(THIS->firstType, decl->firstType,
                                                                   RefMatters::yes, ConstMatters::yes, TemporaryMatters::yes, false) ) {
                return false;
            }
        }
        if ( THIS->type==Type::tTable ) {
            if ( THIS->firstType && decl->firstType && !isSameType(THIS->firstType, decl->firstType,
                                                                   RefMatters::yes, ConstMatters::yes, TemporaryMatters::yes, false) ) {
                return false;
            }
            if ( THIS->secondType && decl->secondType && !isSameType(THIS->secondType, decl->secondType,
                                                                     RefMatters::yes, ConstMatters::yes, TemporaryMatters::yes, false) ) {
                return false;
            }
        }
        if ( THIS->type==Type::tBlock || THIS->type==Type::tFunction
            || THIS->type==Type::tLambda || THIS->type==Type::tTuple
            || THIS->type==Type::tVariant ) {
            if ( THIS->firstType && decl->firstType && !isSameType(THIS->firstType, decl->firstType,
                                                                   RefMatters::yes, ConstMatters::yes, TemporaryMatters::yes, false) ) {
                return false;
            }
            if ( THIS->firstType || THIS->argCount) {    // if not any block or any function
                if ( THIS->argCount != decl->argCount ) {
                    return false;
                }
                for ( uint32_t i=0, is=THIS->argCount; i!=is; ++i ) {
                    auto arg = THIS->argTypes[i];
                    auto declArg = decl->argTypes[i];
                    if ( !isSameType(arg,declArg,RefMatters::yes, ConstMatters::yes, TemporaryMatters::yes,true) ) {
                        return false;
                    }
                }
            }
        }
        if ( THIS->dimSize != decl->dimSize ) {
            return false;
        } else if ( THIS->dim ) {
            for ( uint32_t i=0,is=THIS->dimSize; i!=is; ++i ) {
                if ( THIS->dim[i] != decl->dim[i] ) {
                    return false;
                }
            }
        }
        if ( refMatters == RefMatters::yes ) {
            if ( THIS->isRef() != decl->isRef() ) {
                return false;
            }
        }
        if ( constMatters == ConstMatters::yes ) {
            if ( THIS->isConst() != decl->isConst() ) {
                return false;
            }
        }
        if ( temporaryMatters == TemporaryMatters::yes ) {
            if ( THIS->isTemp() != decl->isTemp() ) {
                return false;
            }
        }
        return true;
    }

    bool isCompatibleCast ( const StructInfo * THIS, const StructInfo * castS ) {
        if ( castS->count < THIS->count ) {
            return false;
        }
        for ( uint32_t i=0, is=THIS->count; i!=is; ++i ) {
            VarInfo * fd = THIS->fields[i];
            VarInfo * cfd = castS->fields[i];
            if ( strcmp(fd->name,cfd->name)!=0 ) {
                return false;
            }
            if ( !isSameType(fd,cfd,RefMatters::yes, ConstMatters::yes, TemporaryMatters::yes,true) ) {
                return false;
            }
        }
        return true;
    }

    string debug_type ( const TypeInfo * info ) {
        if ( !info ) return "";
        TextWriter stream;
        // its never auto or alias
        if ( info->type==Type::tHandle ) {
            stream << info->getAnnotation()->name;
        } else if ( info->type==Type::tStructure ) {
            stream << info->structType->name;
        } else if ( info->type==Type::tPointer ) {
            stream << debug_type(info->firstType) << " ?";
        } else if ( info->type==Type::tEnumeration || info->type==Type::tEnumeration8 || info->type==Type::tEnumeration16 || info->type==Type::tEnumeration64 ) {
            stream << ((info->enumType && info->enumType->name) ? info->enumType->name : "enum");
        } else if ( info->type==Type::tArray ) {
            stream << "array<" << debug_type(info->firstType) << ">";
        } else if ( info->type==Type::tTable ) {
            stream << "table<" << debug_type(info->firstType) << "," << debug_type(info->secondType) << ">";
        } else if ( info->type==Type::tIterator ) {
            stream << "iterator<" << (info->firstType ? debug_type(info->firstType) : "") << ">";
        } else if ( info->type==Type::tTuple || info->type==Type::tVariant ) {
            stream << das_to_string(info->type) << "<";
            for ( uint32_t a=0, as=info->argCount; a!=as; ++a ) {
                if ( a!=0 ) stream << "; ";
                if ( info->argNames ) {
                    stream << info->argNames[a] << ":";
                }
                stream << debug_type(info->argTypes[a]);
            }
            stream << ">";
        } else if ( info->type==Type::tFunction || info->type==Type::tBlock || info->type==Type::tLambda ) {
            stream << das_to_string(info->type) << "<";
            if ( info->argCount ) {
                stream << "(";
                for ( uint32_t ai=0, ais=info->argCount; ai!=ais; ++ ai ) {
                    if ( ai!=0 ) stream << "; ";
                    if (!info->argTypes[ai]->isConst()) {
                        stream << "var ";
                    }
                    if ( info->argNames ) {
                        stream << info->argNames[ai] << ":";
                    } else {
                        stream << "arg" << ai << ":";
                    }
                    stream << debug_type(info->argTypes[ai]);
                }
                stream << ")";
            }
            if ( info->firstType ) {
                if ( info->argCount ) {
                    stream << ":";
                }
                stream << debug_type(info->firstType);
            }
            stream << ">";
        } else if ( info->type==Type::tBitfield ) {
            stream << "bitfield";
            if ( info->argNames ) {
                stream << "<";
                for ( uint32_t ai=0, ais=info->argCount; ai!=ais; ++ai ) {
                    if ( ai !=0 ) stream << "; ";
                    stream << info->argNames[ai];
                }
                stream << ">";
            }
        } else {
            stream << das_to_string(info->type);
        }
        for ( uint32_t i=0, is=info->dimSize; i!=is; ++i ) {
            stream << "[" << info->dim[i] << "]";
        }
        if ( info->flags & TypeInfo::flag_isConst ) {
            stream << " const";
        }
        if ( info->flags & TypeInfo::flag_ref )
            stream << " &";
        if ( info->flags & TypeInfo::flag_isTemp ) {
            stream << " #";
        }
        if ( info->flags & TypeInfo::flag_isImplicit ) {
            stream << " !";
        }
        return stream.str();
    }

    string getTypeInfoMangledName ( TypeInfo * info ) {
        TextWriter ss;
        if ( info->flags & TypeInfo::flag_isConst )     ss << "C";
        if ( info->flags & TypeInfo::flag_ref )         ss << "&";
        if ( info->flags & TypeInfo::flag_isTemp )      ss << "#";
        if ( info->flags & TypeInfo::flag_isImplicit )  ss << "I";
        //  if ( explicitConst )ss << "=";
        //  if ( explicitRef )  ss << "R";
        //  if ( isExplicit )   ss << "X";
        //  if ( aotAlias )     ss << "F";
        if ( info->dimSize ) {
            for ( uint32_t d=0, ds=info->dimSize; d!=ds; ++d ) {
                ss << "[" << info->dim[d] << "]";
            }
        }
        if ( info->argNames ) {
            ss << "N<";
            for ( uint32_t a=0, as=info->argCount; a!=as; ++a ) {
                if ( a ) ss << ";";
                ss << info->argNames[a];
            }
            ss << ">";
        }
        if ( info->argTypes ) {
            ss << "0<";
            for ( uint32_t a=0, as=info->argCount; a!=as; ++a ) {
                if ( a ) ss << ";";
                ss << getTypeInfoMangledName(info->argTypes[a]);
            }
            ss << ">";
        }
        if ( info->firstType ) {
            ss << "1<" << getTypeInfoMangledName(info->firstType) << ">";
        }
        if ( info->secondType ) {
            ss << "2<" << getTypeInfoMangledName(info->secondType) << ">";
        }
        if ( info->type==Type::tHandle ) {
            ss << "H<";
            auto annotation = info->getAnnotation();
            if ( !annotation->module->name.empty() ) {
                ss << annotation->module->name << "::";
             }
             ss << annotation->name << ">";
        } else if ( info->type==Type::tStructure ) {
            ss << "S<";
            // TODO: add module name to struct info
            /*
            if ( info->structType->module && !info->structType->module->name.empty() ) {
                ss << info->structType->module->name << "::";
            }
            */
            ss << info->structType->name << ">";
        } else if ( info->type==Type::tEnumeration || info->type==Type::tEnumeration8 || info->type==Type::tEnumeration16 || info->type==Type::tEnumeration64 ) {
            ss << "E";
            if ( info->type==Type::tEnumeration8 ) ss << "8";
            else if ( info->type==Type::tEnumeration16 ) ss << "16";
            else if ( info->type==Type::tEnumeration64 ) ss << "64";
            if ( info->enumType ) {
                // TODO: add module name to enum info
                ss << "<" << info->enumType->name << ">";
            }
        } else if ( info->type==Type::tPointer ) {
            ss << "?";
            if ( info->flags & TypeInfo::flag_isSmartPtr ) {
                ss << (info->flags & TypeInfo::flag_isSmartPtrNative ? "W" : "M");
            }
        } else {
            switch ( info->type ) {
                case Type::anyArgument:     ss << "*"; break;
                case Type::fakeContext:     ss << "_c"; break;
                case Type::fakeLineInfo:    ss << "_l"; break;
                case Type::autoinfer:       ss << "."; break;
                case Type::alias:           ss << "L"; break;
                case Type::tIterator:       ss << "G"; break;
                case Type::tArray:          ss << "A"; break;
                case Type::tTable:          ss << "T"; break;
                case Type::tBlock:          ss << "$"; break;
                case Type::tFunction:       ss << "@@"; break;
                case Type::tLambda:         ss << "@"; break;
                case Type::tTuple:          ss << "U"; break;
                case Type::tVariant:        ss << "V"; break;
                case Type::tBitfield:       ss << "t"; break;
                case Type::tInt:            ss << "i"; break;
                case Type::tInt2:           ss << "i2"; break;
                case Type::tInt3:           ss << "i3"; break;
                case Type::tInt4:           ss << "i4"; break;
                case Type::tInt8:           ss << "i8"; break;
                case Type::tInt16:          ss << "i16"; break;
                case Type::tInt64:          ss << "i64"; break;
                case Type::tUInt:           ss << "u"; break;
                case Type::tUInt2:          ss << "u2"; break;
                case Type::tUInt3:          ss << "u3"; break;
                case Type::tUInt4:          ss << "u4"; break;
                case Type::tUInt8:          ss << "u8"; break;
                case Type::tUInt16:         ss << "u16"; break;
                case Type::tUInt64:         ss << "u64"; break;
                case Type::tFloat:          ss << "f"; break;
                case Type::tFloat2:         ss << "f2"; break;
                case Type::tFloat3:         ss << "f3"; break;
                case Type::tFloat4:         ss << "f4"; break;
                case Type::tRange:          ss << "r"; break;
                case Type::tURange:         ss << "z"; break;
                case Type::tRange64:        ss << "r64"; break;
                case Type::tURange64:       ss << "z64"; break;
                case Type::tDouble:         ss << "d"; break;
                case Type::tString:         ss << "s"; break;
                case Type::tVoid:           ss << "v"; break;
                case Type::tBool:           ss << "b"; break;
                default:
                    LOG(LogLevel::error) << "ERROR " << das_to_string(info->type) << "\n";
                    DAS_ASSERT(0 && "we should not be here");
                    break;
            }
        }
        return ss.str();
    }


    LineInfo LineInfo::g_LineInfoNULL;

    string LineInfo::describe(bool fully) const {
        if ( fileInfo ) {
            TextWriter ss;
            ss << fileInfo->name << ":" << line << ":" << column;
            if ( fully ) ss << "-" << last_line << ":" << last_column;
            return ss.str();
        } else {
            return string();
        }
    }

    bool LineInfo::operator < ( const LineInfo & info ) const {
        if ( fileInfo && info.fileInfo && fileInfo->name != info.fileInfo->name)
            return fileInfo->name<info.fileInfo->name;
        return (line==info.line) ? column<info.column : line<info.line;
    }
    bool LineInfo::operator == ( const LineInfo & info ) const {
        if ( ((bool)fileInfo) != ((bool)info.fileInfo))
          return false;
        if ( fileInfo && fileInfo->name!=info.fileInfo->name ) return false;
        return line==info.line && column==info.column;
    }
    bool LineInfo::operator != ( const LineInfo & info ) const {
        return !(*this == info);
    }

    bool LineInfo::inside ( const LineInfo & info ) const {
        if ( fileInfo && fileInfo != info.fileInfo ) return false;
        if ( line < info.line || line >= info.last_line ) return false;
        uint32_t from = line==info.line ? info.column : 0;
        uint32_t to = line==info.last_line ? info.last_column : 100500;
        return (column>=from) && (column<to);
    }

    bool LineInfo::empty() const {
        return (line==0) && (column==0) && (last_line==0) && (last_column==0);
    }

    void FileInfo::reserveProfileData() {
#if DAS_ENABLE_PROFILER
        if ( source ) {
            uint32_t nl = 0;
            auto se = source + sourceLength;
            for ( auto si=source; si!=se; ++si ) {
                if ( *si=='\n' ) {
                    nl ++;
                }
            }
            profileData.reserve(nl + 2);
        }
#endif
    }

    void TextFileInfo::getSourceAndLength ( const char * & src, uint32_t & len ) {
        src = source;
        len = sourceLength;
    }

    void TextFileInfo::freeSourceData() {
        if ( source ) {
            das_aligned_free16((void*)source);
            source = nullptr;
            sourceLength = 0;
        }
    }

    bool FileAccess::isSameFileName ( const string & a, const string & b ) const {
        if ( a.size() != b.size() ) return false;
        auto it_a = a.begin();
        auto it_b = b.begin();
        while ( it_a != a.end() ) {
            bool isSlahA = *it_a=='\\' || *it_a=='/';
            bool isSlahB = *it_b=='\\' || *it_b=='/';
            if ( isSlahA != isSlahB ) {
                return false;
            } else if ( !isSlahA && (tolower(*it_a) != tolower(*it_b)) ) {
                return false;
            }
            ++it_a;
            ++it_b;
        }
        return true;
    }

    FileInfoPtr FileAccess::letGoOfFileInfo ( const string & fileName ) {
        auto it = files.find(fileName);
        if ( it == files.end() ) return nullptr;
        return das::move(it->second);
    }

    FileInfo * FileAccess::setFileInfo ( const string & fileName, FileInfoPtr && info ) {
        // TODO: test. for now we need to allow replace
        // if ( files.find(fileName)!=files.end() ) return nullptr;
        files[fileName] = das::move(info);
        auto ins = files.find(fileName);
        ins->second->name = (char *) ins->first.c_str();
        return ins->second.get();
    }

    bool FileAccess::invalidateFileInfo ( const string & fileName ) {
        auto it = files.find(fileName);
        if ( it != files.end() ) {
            files.erase(it);
            return true;
        } else {
            return false;
        }
    }

    FileInfo * FileAccess::getFileInfo ( const string & fileName ) {
        auto it = files.find(fileName);
        if ( it != files.end() ) {
            return it->second.get();
        }
        auto ni = getNewFileInfo(fileName);
        if ( ni ) {
            ni->reserveProfileData();
        }
        return ni;
    }

    string FileAccess::getIncludeFileName ( const string & fileName, const string & incFileName ) const {
        auto np = fileName.find_last_of("\\/");
        if ( np != string::npos ) {
            return fileName.substr(0,np+1) + incFileName;
        } else {
            return incFileName;
        }
    }

    ModuleInfo FileAccess::getModuleInfo ( const string & req, const string & from ) const {
        auto mod = getModuleName(req);
        string modFName = getModuleFileName(req);
        string modFn = getIncludeFileName(from, modFName) + ".das";
        ModuleInfo info;
        info.moduleName = mod;
        info.fileName = modFn;
        return info;
    }

    void FileAccess::freeSourceData() {
        for ( auto & fp : files ) {
            fp.second->freeSourceData();
        }
    }
}
