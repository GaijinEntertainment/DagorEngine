#include "daScript/misc/platform.h"

#include "daScript/ast/ast.h"
#include "daScript/ast/ast_visitor.h"
#include "daScript/ast/ast_generate.h"
#include "daScript/ast/ast_interop.h"

#include "daScript/misc/enums.h"
#include "daScript/simulate/hash.h"

das::Context * get_context ( int stackSize = 0 );

namespace das {

    Enum<Type> g_cppCTypeTable = {
        {   Type::autoinfer,    "autoinfer"  },
        {   Type::alias,        "alias"  },
        {   Type::anyArgument,  "anyArgument"  },
        {   Type::tVoid,        "tVoid"  },
        {   Type::tStructure,   "tStructure" },
        {   Type::tPointer,     "tPointer" },
        {   Type::tBool,        "tBool"  },
        {   Type::tInt8,        "tInt8"  },
        {   Type::tUInt8,       "tUInt8"  },
        {   Type::tInt16,       "tInt16"  },
        {   Type::tUInt16,      "tUInt16"  },
        {   Type::tInt64,       "tInt64"  },
        {   Type::tUInt64,      "tUInt64"  },
        {   Type::tString,      "tString" },
        {   Type::tPointer,     "tPointer" },
        {   Type::tEnumeration,   "tEnumeration" },
        {   Type::tEnumeration8,  "tEnumeration8" },
        {   Type::tEnumeration16, "tEnumeration16" },
        {   Type::tBitfield,    "tBitfield" },
        {   Type::tIterator,    "tIterator" },
        {   Type::tArray,       "tArray" },
        {   Type::tTable,       "tTable" },
        {   Type::tInt,         "tInt"   },
        {   Type::tInt2,        "tInt2"  },
        {   Type::tInt3,        "tInt3"  },
        {   Type::tInt4,        "tInt4"  },
        {   Type::tUInt,        "tUInt"  },
        {   Type::tUInt2,       "tUInt2" },
        {   Type::tUInt3,       "tUInt3" },
        {   Type::tUInt4,       "tUInt4" },
        {   Type::tFloat,       "tFloat" },
        {   Type::tFloat2,      "tFloat2"},
        {   Type::tFloat3,      "tFloat3"},
        {   Type::tFloat4,      "tFloat4"},
        {   Type::tDouble,      "tDouble" },
        {   Type::tRange,       "tRange" },
        {   Type::tURange,      "tURange"},
        {   Type::tRange64,     "tRange64" },
        {   Type::tURange64,    "tURange64"},
        {   Type::tBlock,       "tBlock"},
        {   Type::tFunction,    "tFunction"},
        {   Type::tLambda,      "tLambda"},
        {   Type::tTuple,       "tTuple"},
        {   Type::tVariant,     "tVariant"},
        {   Type::tHandle,      "tHandle"}
    };

    Enum<Type> g_cppTypeTable = {
        {   Type::anyArgument,  "vec4f"    },
        {   Type::tVoid,        "void"     },
        {   Type::tBool,        "bool"     },
        {   Type::tInt8,        "int8_t"   },
        {   Type::tUInt8,       "uint8_t"  },
        {   Type::tInt16,       "int16_t"  },
        {   Type::tUInt16,      "uint16_t" },
        {   Type::tInt64,       "int64_t"  },
        {   Type::tUInt64,      "uint64_t" },
        {   Type::tBitfield,    "Bitfield" },
        {   Type::tString,      "char *"   },
        {   Type::tInt,         "int32_t"  },
        {   Type::tInt2,        "int2"     },
        {   Type::tInt3,        "int3"     },
        {   Type::tInt4,        "int4"     },
        {   Type::tUInt,        "uint32_t" },
        {   Type::tUInt2,       "uint2"    },
        {   Type::tUInt3,       "uint3"    },
        {   Type::tUInt4,       "uint4"    },
        {   Type::tFloat,       "float"    },
        {   Type::tFloat2,      "float2"   },
        {   Type::tFloat3,      "float3"   },
        {   Type::tFloat4,      "float4"   },
        {   Type::tDouble,      "double"   },
        {   Type::tRange,       "range"    },
        {   Type::tURange,      "urange"   },
        {   Type::tRange64,     "range64"  },
        {   Type::tURange64,    "urange64" },
        {   Type::tBlock,       "Block"    },
        {   Type::tFunction,    "Func"     },
        {   Type::tLambda,      "Lambda"   },
        {   Type::tTuple,       "Tuple"    },
        {   Type::tVariant,     "Variant"  }
    };

    string aotModuleName ( Module * pm  ) {
        if ( pm->name.empty() ) {
            return "";
        } else if ( pm->name=="$" ) {
            return "_builtin_";
        } else {
            return pm->name;
        }
    }

    string aotFunctionName ( string str ) {
        string result;
        for (char c : str) {
            if (c == '`')
                result += "__";
            else
                result += c;
        }
        return result;
    }

    string das_to_cppString ( Type t ) {
        return g_cppTypeTable.find(t);
    }

    string das_to_cppCTypeString ( Type t ) {
        return g_cppCTypeTable.find(t);
    }

    bool isConstRedundantForCpp ( const TypeDeclPtr & type ) {
        if ( type->dim.size() ) return false;
        if ( type->isVectorType() ) return true;
        switch ( type->baseType ) {
            case Type::tBool:
            case Type::tInt8:
            case Type::tUInt8:
            case Type::tInt16:
            case Type::tUInt16:
            case Type::tInt64:
            case Type::tUInt64:
            case Type::tInt:
            case Type::tUInt:
            case Type::tFloat:
            case Type::tDouble:
            case Type::tEnumeration:
            case Type::tEnumeration8:
            case Type::tEnumeration16:
            case Type::tBitfield:
                return true;
            default:
                return false;
        }
    }

    enum CpptUseAlias {
        no
    ,   yes
    };

    string describeCppTypeEx ( const TypeDeclPtr & type,
                            CpptSubstitureRef substituteRef,
                            CpptSkipRef skipRef,
                            CpptSkipConst skipConst,
                            CpptRedundantConst redundantConst,
                            CpptUseAlias useAlias ) {

        TextWriter stream;
        auto baseType = type->baseType;
        if ( isConstRedundantForCpp(type) && redundantConst==CpptRedundantConst::yes ) {
            if (substituteRef == CpptSubstitureRef::yes && type->ref) {
                // can't skip const
            } else if ( type->ref ) {
                // can't skip const
            } else {
                skipConst = CpptSkipConst::yes;
            }
        }
        if ( type->dim.size() ) {
            for ( size_t d=0, ds=type->dim.size(); d!=ds; ++d ) {
                stream << "TDim<";
            }
        }
        if ( useAlias==CpptUseAlias::yes && type->aotAlias && !type->alias.empty() ) {
            stream << type->alias;
        } else if ( baseType==Type::alias ) {
            stream << "DAS_COMMENT(alias)";
        } else if ( baseType==Type::autoinfer ) {
            stream << "DAS_COMMENT(auto";
            if ( !type->alias.empty() ) {
                stream << "(" << type->alias << ")";
            }
            stream << ")";
        } else if ( baseType==Type::tHandle ) {
            if ( type->annotation->cppName.empty() ) {
                stream << type->annotation->name;
            } else {
                stream << type->annotation->cppName;
            }
        } else if ( baseType==Type::tArray ) {
            if ( type->firstType ) {
                stream << "TArray<" << describeCppTypeEx(type->firstType,CpptSubstitureRef::no,CpptSkipRef::no,CpptSkipConst::no,CpptRedundantConst::yes,useAlias) << ">";
            } else {
                stream << "Array";
            }
        } else if ( baseType==Type::tTable ) {
            if ( type->firstType && type->secondType ) {
                stream << "TTable<" << describeCppTypeEx(type->firstType,CpptSubstitureRef::no,CpptSkipRef::no,CpptSkipConst::no,CpptRedundantConst::yes,useAlias)
                << "," << describeCppTypeEx(type->secondType,CpptSubstitureRef::no,CpptSkipRef::no,CpptSkipConst::no,CpptRedundantConst::yes,useAlias) << ">";
            } else {
                stream << "Table";
            }
        } else if ( baseType==Type::tTuple ) {
            stream << "TTuple<" << int(type->getTupleSize());
            for ( const auto & arg : type->argTypes ) {
                stream << "," << describeCppTypeEx(arg,CpptSubstitureRef::no,CpptSkipRef::no,CpptSkipConst::no,CpptRedundantConst::yes,useAlias);
            }
            stream << ">";
        } else if ( baseType==Type::tVariant ) {
            stream << "TVariant<" << int(type->getVariantSize()) << "," << int(type->getVariantAlign());
            for ( const auto & arg : type->argTypes ) {
                stream << "," << describeCppTypeEx(arg,CpptSubstitureRef::no,CpptSkipRef::no,CpptSkipConst::no,CpptRedundantConst::yes,useAlias);
            }
            stream << ">";
        } else if ( baseType==Type::tStructure ) {
            if ( type->structType ) {
                if ( type->structType->module->name.empty() ) {
                    stream << type->structType->name;
                } else {
                    stream << aotModuleName(type->structType->module) << "::" << type->structType->name;
                }
            } else {
                stream << "DAS_COMMENT(unspecified structure) ";
            }
        } else if ( baseType==Type::tPointer ) {
            if ( !type->smartPtr ) {
                if ( type->firstType ) {
                    stream << describeCppTypeEx(type->firstType,CpptSubstitureRef::no,CpptSkipRef::no,CpptSkipConst::no,CpptRedundantConst::no,useAlias) << " *";
                } else {
                    stream << "void *";
                }
            } else {
                if ( type->firstType ) {
                    stream  << (type->smartPtrNative ? "smart_ptr<" : "smart_ptr_raw<")
                            <<  describeCppTypeEx(type->firstType,CpptSubstitureRef::no,CpptSkipRef::no,CpptSkipConst::no,CpptRedundantConst::no,useAlias)
                            << ">";
                } else {
                    stream  << (type->smartPtrNative ? "smart_ptr<" : "smart_ptr_raw<") << "void>";
                }
            }
        } else if ( type->isEnumT() ) {
            if ( type->enumType ) {
                if ( type->enumType->external ) {
                    stream << "DAS_COMMENT(bound_enum) " << type->enumType->cppName;
                } else if ( type->enumType->module->name.empty() ) {
                    stream << "DAS_COMMENT(enum) " << type->enumType->name;
                } else {
                    stream << "DAS_COMMENT(enum) " << aotModuleName(type->enumType->module) << "::" << type->enumType->name;
                }
            } else {
                stream << "DAS_COMMENT(unspecified enumeration)";
            }
        } else if ( baseType==Type::tIterator ) {
            if ( type->firstType ) {
                stream << "Sequence DAS_COMMENT((" << describeCppTypeEx(type->firstType,substituteRef,skipRef,skipConst,CpptRedundantConst::yes,useAlias) << "))";
            } else {
                stream << "Sequence";
            }
        } else if ( baseType==Type::tBlock || baseType==Type::tFunction || baseType==Type::tLambda ) {
            if ( !type->constant && type->baseType==Type::tBlock ) {
                stream << "const ";
            }
            stream << das_to_cppString(baseType) << " DAS_COMMENT((";
            if ( type->firstType ) {
                stream << describeCppTypeEx(type->firstType,CpptSubstitureRef::no,CpptSkipRef::no,CpptSkipConst::no,CpptRedundantConst::yes,useAlias);
            } else {
                stream << "void";
            }
            if ( type->argTypes.size() ) {
                for ( const auto & arg : type->argTypes ) {
                    stream << "," << describeCppTypeEx(arg,CpptSubstitureRef::no,CpptSkipRef::no,CpptSkipConst::no,CpptRedundantConst::yes,useAlias);
                }
            }
            stream << "))";
        } else {
            stream << das_to_cppString(baseType);
        }
        if ( type->dim.size() ) {
            for ( auto itd=type->dim.rbegin(), itds=type->dim.rend(); itd!=itds; ++itd ) {
                stream << "," << *itd << ">";
            }
        }
        if ( skipConst==CpptSkipConst::no ) {
            if ( type->constant ) {
                stream << " const ";
            }
        }
        if ( type->ref && skipRef==CpptSkipRef::no ) {
            if ( substituteRef==CpptSubstitureRef::no ) {
                stream << " &";
            } else {
                stream << " *";
            }
        }
        return stream.str();
    }

    string describeCppType ( const TypeDeclPtr & type,
                            CpptSubstitureRef substituteRef,
                            CpptSkipRef skipRef,
                            CpptSkipConst skipConst,
                            CpptRedundantConst redundantConst ) {
        return describeCppTypeEx(type, substituteRef, skipRef, skipConst, redundantConst, CpptUseAlias::no);
    }


    class NoAotMarker : public Visitor {
    public:
        NoAotMarker() {}
    protected:
        Function * func = nullptr;
    protected:
        // type
        virtual void preVisit ( TypeDecl * type ) override {
            if ( func && !type->canAot() ) func->noAot = true;
        }
        // function
        virtual void preVisit ( Function * f ) override {
            func = f;
            Visitor::preVisit(f);
        }
        virtual FunctionPtr visit ( Function * that ) override {
            auto res = Visitor::visit(that);
            func = nullptr;
            return res;
        }
        // any expression
        virtual void preVisitExpression ( Expression * expr ) override {
            Visitor::preVisitExpression(expr);
            if ( func && expr->type && !expr->type->canAot() ) func->noAot = true;
        }
        // looks like call
        virtual void preVisit ( ExprLooksLikeCall * call ) override {
            Visitor::preVisit(call);
            if ( call->name=="invoke" ) {
                if ( call->arguments.size() && call->arguments[0]->rtti_isMakeBlock() ) {
                    auto mkb = static_pointer_cast<ExprMakeBlock>(call->arguments[0]);
                    auto blk = static_pointer_cast<ExprBlock>(mkb->block);
                    blk->aotSkipMakeBlock = true;
                }
            }
        }
    };

    class PrologueMarker : public Visitor {
    public:
        PrologueMarker() {}
    protected:
        Function * func = nullptr;
    protected:
        // function
        virtual void preVisit ( Function * f ) override {
            func = f;
            Visitor::preVisit(f);
        }
        virtual FunctionPtr visit ( Function * that ) override {
            auto res = Visitor::visit(that);
            func = nullptr;
            return res;
        }
        // ExprMakeBlock
        virtual void preVisit ( ExprMakeBlock * expr ) override {
            Visitor::preVisit(expr);
            if ( func && func->hasMakeBlock ) {
                auto block = static_pointer_cast<ExprBlock>(expr->block);
                if ( !block->aotSkipMakeBlock ) {
                    func->aotNeedPrologue = true;
                }
            }
        }
    };

    class UseTypeMarker : public Visitor {
    public:
        das_set<Structure *>    useStructs;
        das_set<Enumeration *>  useEnums;
    protected:
        virtual void preVisit ( TypeDecl * type ) override {
            Visitor::preVisit(type);
        }
        virtual void preVisitExpression ( Expression * expr ) override {
            Visitor::preVisitExpression(expr);
            mark(expr->type.get());
        }
        virtual void preVisitArgument ( Function * fn, const VariablePtr & var, bool lastArg ) override {
            Visitor::preVisitArgument(fn,var,lastArg);
            mark(var->type.get());
        }
        virtual void preVisitBlockArgument ( ExprBlock * block, const VariablePtr & var, bool lastArg ) override {
            Visitor::preVisitBlockArgument(block,var,lastArg);
            mark(var->type.get());
        }
        void mark ( TypeDecl * decl ) {
            if ( !decl ) return;
            if ( decl->baseType==Type::tStructure ) {
                DAS_ASSERT(decl->structType);
                if ( useStructs.find(decl->structType)==useStructs.end() ) {
                    useStructs.insert(decl->structType);
                    for ( auto & fld : decl->structType->fields ) {
                        mark ( fld.type.get() );
                    }
                }
            } else if ( decl->baseType==Type::tEnumeration || decl->baseType==Type::tEnumeration8 || decl->baseType==Type::tEnumeration16 ) {
                DAS_ASSERT(decl->enumType);
                useEnums.insert(decl->enumType);
            } else {
                if ( decl->firstType ) mark ( decl->firstType.get() );
                if ( decl->secondType ) mark ( decl->secondType.get() );
                for ( auto & arg : decl->argTypes ) {
                    mark(arg.get());
                }
            }
        }
    };

    class AotDebugInfoHelper : public DebugInfoHelper {
    public:
        void writeDim ( TextWriter & ss, TypeInfo * info, const string & suffix = ""  ) const {
            if ( info->dimSize ) {
                ss << "uint32_t " << typeInfoName(info) << "_dim" << suffix << "[" << info->dimSize << "] = { ";
                for ( uint32_t i=0, is=info->dimSize; i!=is; ++i ) {
                    if ( i ) ss << ", ";
                    ss << info->dim[i];
                }
                ss << " };\n";
            }
        }
        void writeArgNames ( TextWriter & ss, TypeInfo * info, const string & suffix = "" ) const {
            if ( info->argCount && info->argNames ) {
                ss << "const char * " << typeInfoName(info) << "_arg_names" << suffix << "[" << info->argCount << "] = { ";
                for ( uint32_t i=0, is=info->argCount; i!=is; ++i ) {
                    if ( i ) ss << ", ";
                    ss << "\"" << info->argNames[i] << "\"";
                }
                ss << " };\n";
            }
        }
        void writeArgTypes ( TextWriter & ss, TypeInfo * info, const string & suffix = ""  ) const {
            if ( info->argCount && info->argTypes ) {
                ss << "TypeInfo * " << typeInfoName(info) << "_arg_types" << suffix << "[" << info->argCount << "] = { ";
                for ( uint32_t i=0, is=info->argCount; i!=is; ++i ) {
                    if ( i ) ss << ", ";
                    ss << "&" << typeInfoName(info->argTypes[i]);
                }
                ss << " };\n";
            }
        }
        string str() const {
            TextWriter ss;
            // extern declarations
            for ( auto & ti : smn2s ) {
                ss << "extern StructInfo " << structInfoName(ti.second) << ";\n";
            }
            for ( auto & ti : tmn2t ) {
                ss << "extern TypeInfo " << typeInfoName(ti.second) << ";\n";
            }
            for ( auto & ti : vmn2v ) {
                ss << "extern VarInfo " << varInfoName(ti.second) << ";\n";
            }
            for ( auto & ti : fmn2f ) {
                ss << "extern FuncInfo " << funcInfoName(ti.second) << ";\n";
            }
            for ( auto & ti : emn2e ) {
                ss << "extern EnumInfo " << enumInfoName(ti.second) << ";\n";
            }
            ss << "\n";
            for ( auto & ti : emn2e ) {
                describeCppEnumInfoValues(ss, ti.second);
                ss << "EnumInfo " << enumInfoName(ti.second) << " = { ";
                describeCppEnumInfo(ss, ti.second);
                ss << " };\n";
            }
            for ( auto & ti : smn2s ) {
                describeCppStructInfoFields(ss, ti.second);
                ss << "StructInfo " << structInfoName(ti.second) << " = {";
                describeCppStructInfo(ss, ti.second);
                ss << " };\n";
            }
            for ( auto & ti : this->fmn2f ) {
                describeCppFuncInfoFields(ss, ti.second);
                ss << "FuncInfo " << funcInfoName(ti.second) << " = {";
                describeCppFuncInfo(ss, ti.second);
                ss << " };\n";
            }
            for ( auto & ti : tmn2t ) {
                auto tinfo = ti.second;
                writeDim(ss, tinfo);
                writeArgTypes(ss, tinfo);
                writeArgNames(ss, tinfo);
                ss << "TypeInfo " << typeInfoName(tinfo) << " = { ";
                describeCppTypeInfo(ss, tinfo);
                ss << " };\n";
            }
            ss << "\n";
            ss << "static void resolveTypeInfoAnnotations()\n{\n";
            for ( auto & ti : tmn2t ) {
                auto tinfo = ti.second;
                if ( tinfo->type==Type::tHandle ) {
                    ss << "\t" << typeInfoName(ti.second) << ".resolveAnnotation();\n";
                };
            }
            ss << "}\n\n";
            return ss.str();
        }
        string enumInfoName ( EnumInfo * info ) const {
            TextWriter ss;
            ss << "__enum_info__" << HEX << info->hash << DEC;
            return ss.str();
        }
        string funcInfoName ( FuncInfo * info ) const {
            TextWriter ss;
            ss << "__func_info__" << HEX << info->hash << DEC;
            return ss.str();
        }
        string varInfoName ( VarInfo * info ) const {
            TextWriter ss;
            ss << "__var_info__" << HEX << info->hash << DEC;
            return ss.str();
        }
        string structInfoName ( StructInfo * info ) const {
            TextWriter ss;
            ss << "__struct_info__" << HEX << info->hash << DEC;
            return ss.str();
        }
        string typeInfoName ( TypeInfo * info ) const {
            TextWriter ss;
            ss << "__type_info__" << HEX << info->hash << DEC;
            return ss.str();
        }
    protected:
        void describeCppVarInfo ( TextWriter & ss, VarInfo * info, const string & suffix ) const {
            describeCppTypeInfo(ss, info, suffix);
            ss << ", \"" << info->name << "\", ";
            ss << info->offset << ", " << info->nextGcField;

        }
        void describeCppStructInfoFields ( TextWriter & ss, StructInfo * info ) const {
            if ( !info->fields ) return;
            for ( uint32_t fi=0, fis=info->count; fi!=fis; ++fi ) {
                auto suffix = "_var_" + to_string(info->hash);
                writeDim(ss, info->fields[fi], suffix);
                writeArgTypes(ss, info->fields[fi], suffix);
                writeArgNames(ss, info->fields[fi], suffix);
                ss << "VarInfo " << structInfoName(info) << "_field_" << fi << " =  { ";
                describeCppVarInfo(ss, info->fields[fi],suffix);
                ss << " };\n";
            }
            ss << "VarInfo * " << structInfoName(info) << "_fields[" << info->count << "] =  { ";
            for ( uint32_t fi=0, fis=info->count; fi!=fis; ++fi ) {
                if ( fi ) ss << ", ";
                ss << "&" << structInfoName(info) << "_field_" << fi;
            }
            ss << " };\n";
        }
        void describeCppStructInfo ( TextWriter & ss, StructInfo * info ) const {
            ss << "\"" << info->name << "\", " << "\"" << info->module_name << "\", " << info->flags << ", ";
            if ( info->fields ) {
                ss << structInfoName(info) << "_fields, ";
            } else {
                ss << "nullptr, ";
            }
            ss << info->count << ", ";
            ss << info->size << ", ";
            ss << "UINT64_C(0x" << HEX << info->init_mnh << DEC << "), ";
            ss << "nullptr, ";  // annotation list
            ss << "UINT64_C(0x" << HEX << info->hash << DEC << "), ";
            ss << info->firstGcField;
        }
        void describeCppFuncInfoFields ( TextWriter & ss, FuncInfo * info ) const {
            if ( !info->fields ) return;
            for ( uint32_t fi=0, fis=info->count; fi!=fis; ++fi ) {
                auto suffix = "_var_" + to_string(info->hash);
                writeDim(ss, info->fields[fi], suffix);
                writeArgTypes(ss, info->fields[fi], suffix);
                writeArgNames(ss, info->fields[fi], suffix);
                ss << "VarInfo " << funcInfoName(info) << "_field_" << fi << " =  { ";
                describeCppVarInfo(ss, info->fields[fi],suffix);
                ss << " };\n";
            }
            ss << "VarInfo * " << funcInfoName(info) << "_fields[" << info->count << "] =  { ";
            for ( uint32_t fi=0, fis=info->count; fi!=fis; ++fi ) {
                if ( fi ) ss << ", ";
                ss << "&" << funcInfoName(info) << "_field_" << fi;
            }
            ss << " };\n";
        }
        void describeCppFuncInfo ( TextWriter & ss, FuncInfo * info ) const {
            ss  << "\"" << info->name << "\", "
                << "\"" << info->cppName << "\", "            ;
            if ( info->fields ) {
                ss << funcInfoName(info) << "_fields, ";
            } else {
                ss << "nullptr, ";
            }
            ss  << info->count << ", "
                << info->stackSize << ", "
                << "&" << typeInfoName(info->result) << ", "
                << "nullptr,"
                << "0,"
                << "UINT64_C(0x" << HEX << info->hash << DEC << "), "
                << "0x" << HEX << info->flags << DEC;
        }
        void describeCppEnumInfoValues ( TextWriter & ss, EnumInfo * einfo ) const {
            for ( uint32_t v=0, vs=einfo->count; v!=vs; ++v ) {
                auto val = einfo->fields[v];
                ss << "EnumValueInfo " << enumInfoName(einfo) << "_value_" << v << " = { \""
                << val->name << "\", " << val->value << " };\n";
            }
            ss << "EnumValueInfo * " << enumInfoName(einfo) << "_values [] = { ";
            for ( uint32_t v=0, vs=einfo->count; v!=vs; ++v ) {
                if ( v ) ss << ", ";
                ss << "&" << enumInfoName(einfo) << "_value_" << v;
            }
            ss << " };\n";
        }
        void describeCppEnumInfo ( TextWriter & ss, EnumInfo * info ) const {
            ss  << "\"" << info->name << "\", \"" << info->module_name << "\", " << enumInfoName(info) << "_values, "
                << info->count << ", UINT64_C(0x" << HEX << info->hash << DEC << ")";
        }
        void describeCppTypeInfo ( TextWriter & ss, TypeInfo * info, const string & suffix = "" ) const {
            ss << "Type::" << das_to_cppCTypeString(info->type) << ", ";
            if ( info->type==Type::tStructure ) {
                ss << "&" << structInfoName(info->structType);
            } else {
                ss << "nullptr";
            }
            ss << ", ";
            if ( info->type==Type::tEnumeration || info->type==Type::tEnumeration8 || info->type==Type::tEnumeration16 ) {
                ss << "&" << enumInfoName(info->enumType);
            } else {
                ss << "nullptr";
            }
            if ( info->type==Type::tHandle ) {
                if ( intptr_t(info->annotation_or_name) & 1 ) {
                    auto tname = (char *)(intptr_t(info->annotation_or_name) ^ 1);  // already comes from string allocator
                    ss << ", DAS_MAKE_ANNOTATION(\"" << tname << "\")";
                }  else {
                    // we add ~ at the beginning of the name for padding
                    // if name is allocated by the compiler, it does not guarantee that it is aligned
                    // we check if there is a ~ at the beginning of the name, and if it is - we skip it
                    // that way we can accept both aligned and unaligned names
                    ss << ", DAS_MAKE_ANNOTATION(\"~" << info->annotation_or_name->module->name << "::" << info->annotation_or_name->name << "\")";
                }
            } else {
                DAS_ASSERT(info->type!=Type::tHandle);
                ss << ", nullptr";
            }
            ss << ", ";
            if ( info->firstType ) {
                ss << "&" << typeInfoName(info->firstType);
            } else {
                ss << "nullptr";
            }
            ss << ", ";
            if ( info->secondType ) {
                ss << "&" << typeInfoName(info->secondType);
            } else {
                ss << "nullptr";
            }

            if (info->argCount && info->argTypes) {
                ss << ", (TypeInfo **)" << typeInfoName(info) << "_arg_types" << suffix;
            } else {
                ss << ", nullptr";
            }
            if (info->argCount && info->argNames) {
                ss << ", " << typeInfoName(info) << "_arg_names" << suffix;
            } else {
                ss << ", nullptr";
            }
            ss << ", " << info->argCount;

            ss << ", " << info->dimSize;
            ss << ", ";
            if ( info->dimSize ) {
                ss << typeInfoName(info) << "_dim" << suffix;
            } else {
                ss << "nullptr";
            }
            ss << ", " << info->flags;
            ss << ", " << info->size;
            ss << ", UINT64_C(0x" << HEX << info->hash << DEC << ")";
        }

    };

    bool isLocalVec ( const TypeDeclPtr & vtype ) {
        return vtype->dim.size()==0 && vtype->isVectorType() && !vtype->ref;
    }

    void describeLocalCppType ( TextWriter & ss, const TypeDeclPtr & vtype, CpptSubstitureRef substituteRef = CpptSubstitureRef::yes ) {
        ss << describeCppType(vtype,substituteRef,CpptSkipRef::no);
    }

    void describeVarLocalCppType ( TextWriter & ss, const TypeDeclPtr & vtype, CpptSubstitureRef substituteRef = CpptSubstitureRef::yes ) {
        if ( vtype->isGoodBlockType() ) {
            ss << "auto";
        } else {
            ss << describeCppType(vtype,substituteRef,CpptSkipRef::no,CpptSkipConst::yes);
        }
    }

    vector<Function *> collectUsedFunctions ( vector<Module *> & modules, int totalFunctions ) {
        vector<Function *> fnn; fnn.reserve(totalFunctions);
        for (auto & pm : modules) {
            pm->functions.foreach([&](auto pfun){
                if (pfun->index < 0 || !pfun->used)
                    return;
                fnn.push_back(pfun.get());
            });
        }
        return fnn;
    }

    string aotSuffixNameEx ( const string & funcName, const char * suffix ) {
        string name;
        bool prefix = false;
        for ( char ch : funcName ) {
            if ( isalnum(ch) || ch=='_' ) {
                name += ch;
            } else {
                prefix = true;
                switch ( ch ) {
                    case '=':   name += "Equ"; break;
                    case '+':   name += "Add"; break;
                    case '-':   name += "Sub"; break;
                    case '*':   name += "Mul"; break;
                    case '/':   name += "Div"; break;
                    case '%':   name += "Mod"; break;
                    case '&':   name += "And"; break;
                    case '|':   name += "Or"; break;
                    case '^':   name += "Xor"; break;
                    case '?':   name += "Qmark"; break;
                    case '~':   name += "Tilda"; break;
                    case '!':   name += "Excl"; break;
                    case '>':   name += "Greater"; break;
                    case '<':   name += "Less"; break;
                    case '[':   name += "Sqbl"; break;
                    case ']':   name += "Sqbr"; break;
                    case '.':   name += "Dot"; break;
                    default:
                        name += "_0x";
                        name += '0' + (ch>>4);
                        name += '0' + (ch & 0x0f);
                        name += "_";
                        break;
                }
            }
        }
        return prefix ? (suffix + name) : name;
    }

    string aotFuncName ( Function * func ) {
        if ( func->hash ) {
            TextWriter tw;
            tw << aotSuffixNameEx(func->name,"_Func") << "_" << HEX << func->hash << DEC;
            return tw.str();
        } else {
            return aotSuffixNameEx(func->name,"_Func");
        }
    }

    class BlockVariableCollector : public Visitor {
    public:
        BlockVariableCollector() {}
        virtual void preVisit ( ExprBlock * block ) override {
            Visitor::preVisit(block);
            stack.push_back(block);
        }
        string getVarName ( Variable * var ) const {
            auto it = rename.find(var);
            return it==rename.end() ? var->name : it->second;
        }
        string getVarName ( const VariablePtr & var ) const {
            return getVarName(var.get());
        }
        __forceinline bool isMoved(const VariablePtr & var) const {
            return moved.find(var.get()) != moved.end();
        }
        void renameVariable ( Variable * var, const string & newName ) {
            rename[var] = newName;
        }
    protected:
        virtual ExpressionPtr visit ( ExprBlock * block ) override {
            stack.pop_back();
            return Visitor::visit(block);
        }
        bool needRenaming ( Variable * ) const {
            // TODO: check if it indeed needs renaming
            return true;
        }
        void renameVariable ( Variable * var ) {
            if ( needRenaming(var) ) {
                string newName = "__" + aotSuffixNameEx(var->name,"_Var") + "_rename_at_" + to_string(var->at.line);
                rename[var] = newName;
            }
        }
    // for loop
        virtual void preVisitFor ( ExprFor * expr, const VariablePtr & var, bool last ) override {
            Visitor::preVisitFor(expr,var,last);
            for ( auto & varr : expr->iteratorVariables ) {
                renameVariable(varr.get());
            }
        }
    // block argument
        virtual void preVisitBlockArgument ( ExprBlock * block, const VariablePtr & var, bool lastArg ) override {
            Visitor::preVisitBlockArgument(block, var, lastArg);
            renameVariable(var.get());
        }
    // functon argument
        virtual void preVisitArgument ( Function * fn, const VariablePtr & var, bool lastArg ) override {
            Visitor::preVisitArgument(fn, var, lastArg);
            renameVariable(var.get());
        }
    // let
        ExprBlock * getCurrentBlock() const {
            ExprBlock * block = nullptr;
            for (auto it=stack.rbegin(), its=stack.rend(); it!=its; ++it) {
                ExprBlock * pb = *it;
                if (pb->isClosure) {
                    block = pb;
                    break;
                }
                if (!(pb->inTheLoop && pb->finalList.size())) {
                    block = pb;
                    break;
                }
            }
            return block;
        }
        ExprBlock * getFinalBlock () const {
            for ( auto it=stack.rbegin(), its=stack.rend(); it!=its; ++it ) {
                auto blk = *it;
                if ( blk->finalList.size() ) return blk;
                if ( blk->isClosure ) return nullptr;
            }
            return nullptr;
        }
        ExprBlock * getTopBlock () const {
            for ( auto it=stack.rbegin(), its=stack.rend(); it!=its; ++it ) {
                auto blk = *it;
                if ( blk->isClosure ) return blk;
            }
            return stack.front();
        }
        virtual void preVisitLet ( ExprLet * let, const VariablePtr & var, bool last ) override {
            Visitor::preVisitLet(let, var, last);
            if ( auto bfinal = getFinalBlock() ) {
                bfinal = getTopBlock();
                variables[bfinal].push_back(var.get());
                moved.insert(var.get());
            }
            renameVariable(var.get());
        }
    // make array
        virtual void preVisit ( ExprMakeArray * expr ) override {
            auto block = getCurrentBlock();
            if ( !expr->doesNotNeedSp && expr->stackTop ) {
                localTemps[block].push_back(expr);
            }
        }
    // make tuple
        virtual void preVisit ( ExprMakeTuple * expr ) override {
            auto block = getCurrentBlock();
            if ( !expr->doesNotNeedSp && expr->stackTop ) {
                localTemps[block].push_back(expr);
            }
        }
    // make structure
        virtual void preVisit ( ExprMakeStruct * expr ) override {
            auto block = getCurrentBlock();
            if ( !expr->doesNotNeedSp && expr->stackTop ) {
                localTemps[block].push_back(expr);
            }
        }
    // make variant
        virtual void preVisit ( ExprMakeVariant * expr ) override {
            auto block = getCurrentBlock();
            if ( !expr->doesNotNeedSp && expr->stackTop ) {
                localTemps[block].push_back(expr);
            }
        }
    // call with CMRES
        virtual void preVisit ( ExprCall * expr ) override {
            auto block = getCurrentBlock();
            if ( !expr->doesNotNeedSp && expr->stackTop ) {
                localTemps[block].push_back(expr);
            }
        }
    public:
        vector<ExprBlock *>                     stack;
        das_map<ExprBlock *,vector<Variable *>>     variables;
        das_map<ExprBlock *,vector<Expression *>>   localTemps;
    protected:
        das_map<Variable *,string>              rename;
        das_set<Variable *>                     moved;
    };

    string describeCppFunc ( Function * fn, BlockVariableCollector * collector, bool needName = true, bool needInline = true ) {
        TextWriter ss;
        if ( needInline ) {
            ss << "inline ";
        }
        describeLocalCppType(ss,fn->result,CpptSubstitureRef::no);
        ss << " ";
        if ( needName ) {
            ss << aotFuncName(fn);
        } else {
            ss << "(*)";
        }
        ss << " ( Context * __context__";
        for ( auto & var : fn->arguments ) {
            ss << ", ";
            if (isLocalVec(var->type)) {
                describeLocalCppType(ss, var->type);
            } else {
                ss << describeCppType(var->type);
            }
            ss << " ";
            if ( var->type->isRefType() ) {
                ss << "& ";
            }
            if ( collector ) {
                ss << collector->getVarName(var);
            }
        }
        ss << " )";
        return ss.str();
    }

    class CppAot : public Visitor {
    public:
        CppAot ( const ProgramPtr & prog, BlockVariableCollector & cl ) : program(prog), collector(cl) {
            helper.rtti = program->options.getBoolOption("rtti",false);
            prologue = program->options.getBoolOption("aot_prologue",false) ||
                program->getDebugger();
            solidContext = program->policies.solid_context || program->options.getBoolOption("solid_context",false);
        }
        string str() const {
            return "\n" + helper.str() + sti.str()  + stg.str() + ss.str();
        };
    public:
        TextWriter                  ss, sti, stg;
    protected:
        int                         lastNewLine = -1;
        int                         tab = 0;
        int                         debugInfoGlobal = 0;
        AotDebugInfoHelper          helper;
        ProgramPtr                  program;
        BlockVariableCollector &    collector;
        das_set<string>       aotPrefix;
        vector<ExprBlock *>         scopes;
        bool                        prologue = false;
        bool                        solidContext = false;
    protected:
        void newLine () {
            auto nlPos = ss.tellp();
            if ( nlPos != lastNewLine ) {
                ss << "\n";
                lastNewLine = ss.tellp();
            }
        }
        __forceinline static bool noBracket ( Expression * expr ) {
            return expr->topLevel || expr->bottomLevel || expr->argLevel;
        }
    public:
    // enumeration
        virtual void preVisit ( Enumeration * enu ) override {
            Visitor::preVisit(enu);
            if ( enu->external ) {
                ss << "#if 0 // external enum\n";
            }
            ss << "namespace " << aotModuleName(enu->module) << " {\n";
            ss << "\nenum class " << enu->name << " : " << das_to_cppString(enu->baseType) << " {\n";
        }
        virtual void preVisitEnumerationValue ( Enumeration * enu, const string & name, Expression * value, bool last ) override {
            Visitor::preVisitEnumerationValue(enu, name, value, last);
            ss << "\t" << name << " = " << das_to_cppString(enu->baseType) << "(";
        }
        virtual ExpressionPtr visitEnumerationValue ( Enumeration * enu, const string & name, Expression * value, bool last ) override {
            ss << ")";
            if ( !last ) ss << ",";
            ss << "\n";
            return Visitor::visitEnumerationValue(enu, name, value, last);
        }
        virtual EnumerationPtr visit ( Enumeration * enu ) override {
            ss  << "};\n"   // enum
                << "}\n";   // namespace
            if ( enu->external ) {
                ss << "#endif // external enum\n";
            } else {
                string enuName;
                if ( !enu->cppName.empty() ) {
                    enuName = enu->cppName;
                } else if ( enu->module && !enu->module->name.empty() ) {
                    enuName = aotModuleName(enu->module) + "::" + enu->name;
                } else {
                    enuName = enu->name;
                }
                auto castType = das_to_cppString(enu->baseType);
                ss  << "}\n"
                    << "template <> struct cast< das::" << program->thisNamespace << "::" << enuName
                    << " > : cast_enum < das::" << program->thisNamespace << "::" << enuName << " > {};\n"
                    << "namespace " << program->thisNamespace << " {\n";
            }
            return Visitor::visit(enu);
        }
    // strcuture
        virtual bool canVisitStructureFieldInit ( Structure * ) override {
            return false;
        }
        virtual void preVisit ( Structure * that ) override {
            Visitor::preVisit(that);
            if ( that->cppLayout ) {
                ss << "\n#if 0 // skipping structure " << that->name << " declaration due to CPP layout";
            }
            ss << "namespace " << aotModuleName(that->module) << " {\n";
            for ( auto & ann : that->annotations ) {
                if ( ann->annotation->rtti_isStructureAnnotation() ) {
                    static_pointer_cast<StructureAnnotation>(ann->annotation)->aotPrefix(that, ann->arguments, ss);
                }
            }
            ss << "\nstruct " << that->name;
            if (that->cppLayout && that->parent) {
                ss << " : " << that->parent->name;
            }
            ss << " {\n";
            for ( auto & ann : that->annotations ) {
                if ( ann->annotation->rtti_isStructureAnnotation() ) {
                    static_pointer_cast<StructureAnnotation>(ann->annotation)->aotBody(that, ann->arguments, ss);
                }
            }
        }
        virtual void preVisitStructureField ( Structure * that, Structure::FieldDeclaration & decl, bool last ) override {
            Visitor::preVisitStructureField(that, decl, last);
            auto from = that->findFieldParent(decl.name);
            if ( that->cppLayout && from!=that ) {
                ss << "\t/* skipping " << decl.name << ", from " << from->name << " */";
                return;
            }
            ss << "\t" << describeCppType(decl.type) << " " << decl.name << ";";
            if ( decl.parentType ) {
                ss << " /* from " << from->name << " */";
            }
        }
        virtual void visitStructureField ( Structure * var, Structure::FieldDeclaration & decl, bool last ) override {
            ss << "\n";
            Visitor::visitStructureField(var, decl, last);
        }
        virtual StructurePtr visit ( Structure * that ) override {
            ss << "};\n";   // structure
            if ( that->fields.size() ) {
                ss << "static_assert(sizeof(" << that->name << ")==" << that->getSizeOf() << ",\"structure size mismatch with DAS\");\n";
                for ( auto & tf : that->fields ) {
                    ss << "static_assert(offsetof(" << that->name << "," << tf.name << ")=="
                        << tf.offset << ",\"structure field offset mismatch with DAS\");\n";
                }
            }
            for ( auto & ann : that->annotations ) {
                if ( ann->annotation->rtti_isStructureAnnotation() ) {
                    static_pointer_cast<StructureAnnotation>(ann->annotation)->aotSuffix(that, ann->arguments, ss);
                }
            }
            ss << "}\n";    // namespace
            if ( that->cppLayout ) {
                ss << "#endif // end of skipping structure " << that->name << " declaration due to CPP layout\n";
            }
            return Visitor::visit(that);
        }
    // program body
        virtual void preVisitProgramBody ( Program * prog, Module * that ) override {
            // functions
            ss << "\n";
            prog->thisModule->functions.foreach([&](auto fn){
                if ( !fn->builtIn && !fn->noAot ) {
                    ss << describeCppFunc(fn.get(),&collector) << ";\n";
                }
            });
            ss << "\n";
        }
    // global let body
        virtual void preVisitGlobalLetBody ( Program * prog ) override {
            Visitor::preVisitGlobalLetBody(prog);
            ss << "void __init_script ( Context * __context__, bool __init_shared )\n{\n";
            tab ++;
            // pre-declare locals
            auto & temps = collector.localTemps[nullptr];
            for ( auto & tmp : temps ) {
                ss << string(tab,'\t');
                describeVarLocalCppType(ss, tmp->type);
                ss << " " << makeLocalTempName(tmp) << ";\n";
            }
        }
        virtual void visitGlobalLetBody ( Program * prog ) override {
            tab --;
            ss << "}\n";
            Visitor::visitGlobalLetBody(prog);
        }
    // global
        virtual void preVisitGlobalLet ( const VariablePtr & var ) override {
            Visitor::preVisitGlobalLet(var);
            ss << string(tab,'\t');
            if ( !var->used ) ss << "/* ";
            if ( var->global_shared ) {
                ss << "if ( __init_shared ) ";
            }
            if ( var->global_shared ) {
                ss << (var->init ? "das_shared" : "das_shared_zero");
            } else {
                ss << (var->init ? "das_global" : "das_global_zero");
            }
            if ( solidContext ) ss << "_solid";
            ss << "<" << describeCppType(var->type,CpptSubstitureRef::no,CpptSkipRef::yes,CpptSkipConst::yes)
                << ",";
            if ( solidContext ) {
                ss << var->stackTop;
            } else {
                ss << "0x" << HEX << var->getMangledNameHash() << DEC;
            }
            ss << ">(__context__)";
        }
        virtual VariablePtr visitGlobalLet ( const VariablePtr & var ) override {
            ss << ";";
            if ( !var->used ) ss << " */";
            ss << "/*" << var->name << "*/\n";

            return Visitor::visitGlobalLet(var);
        }
        virtual void preVisitGlobalLetInit ( const VariablePtr & var, Expression * init ) override {
            Visitor::preVisitGlobalLetInit(var, init);
            ss << " = ";
        }
    // function
        virtual bool canVisitFunction ( Function * fun ) override {
            if ( fun->noAot ) return false;
            return true;
        }
        virtual void preVisit ( Function * fn) override {
            Visitor::preVisit(fn);
            ss << "\ninline ";
            describeLocalCppType(ss,fn->result,CpptSubstitureRef::no);
            ss << " " << aotFuncName(fn) << " ( Context * __context__";
        }
        virtual void preVisitFunctionBody ( Function * fn,Expression * expr ) override {
            Visitor::preVisitFunctionBody(fn,expr);
            if ( fn->aotNeedPrologue || prologue ) {
                ss << " ) { das_stack_prologue __prologue(__context__," << fn->totalStackSize
                    << ",\"" << fn->name << " \" DAS_FILE_LINE);\n";
            } else {
                ss << " )\n";
            }
        }
        virtual void preVisitArgument ( Function * fn, const VariablePtr & arg, bool last ) override {
            Visitor::preVisitArgument(fn,arg,last);
            // arg
            ss << ", ";
            if (isLocalVec(arg->type)) {
                describeLocalCppType(ss, arg->type);
            } else {
                ss << describeCppType(arg->type);
            }
            if (arg->type->isRefType()) {
                ss << " & ";
            }
            ss << " " << collector.getVarName(arg);
        }
        virtual bool canVisitArgumentInit ( Function *, const VariablePtr &, Expression * ) override {
            return false;
        }
        virtual void preVisitArgumentInit ( Function * fn, const VariablePtr & arg, Expression * expr ) override {
            Visitor::preVisitArgumentInit(fn,arg,expr);
            ss << " = ";
        }
        virtual VariablePtr visitArgument ( Function * fn, const VariablePtr & that, bool last ) override {
            return Visitor::visitArgument(fn, that, last);
        }
        virtual FunctionPtr visit ( Function * fn ) override {
            if ( fn->aotNeedPrologue || prologue ) {
                ss << "}\n";
            } else {
                ss << "\n";
            }
            return Visitor::visit(fn);
        }
    // block
        string makeLocalTempName ( Expression * expr ) const {
            uint32_t stackTop = 0;
            if ( expr->rtti_isMakeLocal() ) {
                stackTop = ((ExprMakeLocal *)expr)->stackTop;
            } else if ( expr->rtti_isCall() ) {
                stackTop = ((ExprCall *)expr)->stackTop;
            } else {
                DAS_ASSERT(0 && "we should not be here. we need stacktop for the name");
                stackTop = (expr->at.line<<16) + expr->at.column;
            }
            return "_temp_make_local_" + to_string(expr->at.line) + "_" + to_string(expr->at.column) + "_" + to_string(stackTop);
        }
        virtual void preVisit ( ExprBlock * block ) override {
            Visitor::preVisit(block);
            scopes.push_back(block);
            block->finallyBeforeBody = true;
            block->finallyDisabled = block->inTheLoop;
            ss << "{\n";
            tab ++;
            // pre-declare variables
            auto & vars = collector.variables[block];
            for ( auto & var : vars ) {
                ss << string(tab,'\t');
                describeVarLocalCppType(ss, var->type);
                auto vname = collector.getVarName(var);
                ss  << " " << vname;
                if ( var->type->constant && var->type->isRefType() && !var->type->ref ) {
                    ss << "_ConstRef";
                }
                ss << "; " << "memset(&" << vname << ",0,sizeof(" << vname << "));"
                    << "\n";
            }
            // pre-declare locals
            auto & temps = collector.localTemps[block];
            for ( auto & tmp : temps ) {
                ss << string(tab,'\t');
                describeVarLocalCppType(ss, tmp->type);
                auto tempName = makeLocalTempName(tmp);
                ss << " " << tempName << "; " << tempName << ";\n";
            }
        }
        virtual void preVisitBlockArgumentInit ( ExprBlock * block, const VariablePtr & var, Expression * init ) override {
            Visitor::preVisitBlockArgumentInit(block, var, init);
            ss << "\n#if 0\n";
        }
        virtual ExpressionPtr visitBlockArgumentInit ( ExprBlock * block, const VariablePtr & var, Expression * init ) override {
            ss << "\n#endif\n";
            return Visitor::visitBlockArgumentInit(block, var, init);
        }
        virtual void preVisitBlockExpression ( ExprBlock * block, Expression * expr ) override {
            Visitor::preVisitBlockExpression(block, expr);
            ss << string(tab,'\t');
        }
        virtual ExpressionPtr visitBlockExpression ( ExprBlock * block, Expression * that ) override {
            ss << ";"; newLine();
            return Visitor::visitBlockExpression(block, that);
        }
        virtual ExpressionPtr visit ( ExprBlock * block ) override {
            tab --;
            ss << string(tab,'\t') << "}";
            block->finallyBeforeBody = false;
            block->finallyDisabled = false;
            scopes.pop_back();
            return Visitor::visit(block);
        }
        string finallyName ( ExprBlock * block ) const {
            return "__finally_" + to_string(block->at.line);
        }
        virtual void preVisitBlockFinal ( ExprBlock * block ) override {
            Visitor::preVisitBlockFinal(block);
            ss << string(tab-1,'\t') << "/* finally */ auto " << finallyName(block) << "= das_finally([&](){\n";
        }
        virtual void preVisitBlockFinalExpression ( ExprBlock * block, Expression * expr ) override {
            Visitor::preVisitBlockFinalExpression(block, expr);
            ss << string(tab,'\t');
        }
        virtual ExpressionPtr visitBlockFinalExpression ( ExprBlock * block, Expression * that ) override {
            ss << ";"; newLine();
            return Visitor::visitBlockFinalExpression(block, that);
        }
        virtual void visitBlockFinal ( ExprBlock * block ) override {
            ss << string(tab-1,'\t') << "/* end finally */ });\n";
            Visitor::visitBlockFinal(block);
        }
    // let
        virtual void preVisitLet ( ExprLet * let, const VariablePtr & var, bool last ) override {
            Visitor::preVisitLet(let, var, last);
            auto vname = collector.getVarName(var);
            if ( var->init && var->init->rtti_isMakeBlock() ) {
                auto mkb = static_pointer_cast<ExprMakeBlock>(var->init);
                auto blk = static_pointer_cast<ExprBlock>(mkb->block);
                blk->aotSkipMakeBlock = true;
                ss << "auto " << vname << "_TempFunctor = ";
                var->init->visit(*this);
                ss << ";\n" << string(tab,'\t');
                blk->aotSkipMakeBlock = false;
                mkb->aotFunctorName = vname + "_TempFunctor";
            }
            if ( !collector.isMoved(var) ) {
                describeVarLocalCppType(ss, var->type);
                ss << " ";
            }
            auto cvname = vname;
            if ( var->type->constant && var->type->isRefType() && !var->type->ref ) {
                cvname += "_ConstRef";
            }
            ss << cvname;
            if ( !var->init && var->type->canInitWithZero() ) {
                if ( isLocalVec(var->type) ) {
                    ss << " = v_zero()";
                } else {
                    ss << " = 0";
                }
            } else if ( !var->init && !var->type->canInitWithZero() ) {
                ss << "; das_zero(" << cvname;
                ss << ")";
            }
        }
        virtual VariablePtr visitLet ( ExprLet * let, const VariablePtr & var, bool last ) override {
            if ( !last ) ss << "; ";
            if ( var->type->constant && var->type->isRefType() && !var->type->ref ) {
                auto vname = collector.getVarName(var);
                ss << ";\n\t";
                describeLocalCppType(ss, var->type);
                ss << " & " << vname << " = " << vname << "_ConstRef; ";
            }
            return Visitor::visitLet(let, var, last);
        }
        virtual void preVisitLetInit ( ExprLet * let, const VariablePtr & var, Expression * expr ) override {
            Visitor::preVisitLetInit(let,var,expr);
            if ( var->init_via_move ) {
                auto vname = collector.getVarName(var);
                auto cvname = vname;
                if ( var->type->constant && var->type->isRefType() ) {
                    cvname += "_ConstRef";
                }
                ss  << "; das_zero(" << cvname << ")"
                    << "; das_move(" << cvname << ", ";
            } else {
                ss << " = ";
            }
            if ( var->type->constant ) {
                if ( !var->type->isGoodBlockType() ) {
                    ss << "((";
                    describeVarLocalCppType(ss, var->type);
                    ss << ")";
                } else {
                    ss << "(";
                }
            }
            if ( !expr->type->isPointer() && !var->type->ref && expr->type->isAotAlias() && !var->type->isAotAlias() ) {
                if ( expr->type->alias.empty() ) {
                    ss << "das_reinterpret<"
                    << describeCppTypeEx(expr->type,CpptSubstitureRef::no,CpptSkipRef::yes,CpptSkipConst::no,CpptRedundantConst::yes,CpptUseAlias::yes) << ">::pass(";
                } else {
                    ss << "das_alias<" << expr->type->alias << ">::from(";
                }
            }
            if ( var->type->ref ) {
                ss << "&(";
            }
            if ( needPtrCast(var->type, expr->type, expr) ) {
                ss << "das_auto_cast<" << describeCppType(var->type) << ">::cast(";
            }
            if ( expr->type->isString() ) {
                ss << "(char *)(";
            }
        }
        virtual ExpressionPtr visitLetInit ( ExprLet * let , const VariablePtr & var, Expression * expr ) override {
            if ( expr->type->isString() ) {
                ss << ")";
            }
            if ( needPtrCast(var->type, expr->type, expr) ) {
                ss << ")";
            }
            if ( var->type->ref ) {
                ss << ")";
            }
            if ( !expr->type->isPointer() && !var->type->ref && expr->type->isAotAlias() && !var->type->isAotAlias() ) {
                ss << ")";
            }
            if ( var->init_via_move ) {
                ss << ")";
            }
            if ( var->type->constant ) {
                ss << ")";
            }
            return Visitor::visitLetInit(let, var, expr);
        }
    // label
        virtual void preVisit ( ExprLabel * that ) override {
            Visitor::preVisit(that);
            ss << "label_" << that->label << ":;";
        }
    // goto
        virtual void preVisit ( ExprGoto * that ) override {
            Visitor::preVisit(that);
            if ( !that->subexpr ) {
                ss << "goto label_" << that->label;
            } else {
                ss << "switch (";
            }
        }
        virtual ExpressionPtr visit(ExprGoto *that) override {
            if ( that->subexpr ) {
                ss << ") {\n";
                for ( auto it=scopes.rbegin(), its=scopes.rend(); it!=its; ++it ) {
                    auto blk = *it;
                    for ( const auto & ex : blk->list ) {
                        if ( ex->rtti_isLabel() ) {
                            auto lab = static_pointer_cast<ExprLabel>(ex);
                            ss << string(tab,'\t') << "case " << lab->label <<": goto label_" << lab->label << ";\n";
                        }
                    }
                }
                ss << string(tab,'\t') << "default: __context__->throw_error(\"invalid label\");\n";
                ss << string(tab,'\t') << "}";
            }
            return Visitor::visit(that);
        }
    // copy
        virtual void preVisit ( ExprCopy * that ) override {
            Visitor::preVisit(that);
            ss << "das_copy(";
        }
        virtual void preVisitRight ( ExprCopy * that, Expression * right ) override {
            Visitor::preVisitRight(that,right);
            ss << ",";
        }
        virtual ExpressionPtr visit ( ExprCopy * that ) override {
            ss << ")";
            return Visitor::visit(that);
        }
    // clone
        virtual void preVisit ( ExprClone * that ) override {
            Visitor::preVisit(that);
            ss << "das_clone<"
                << describeCppType(that->left->type,CpptSubstitureRef::no,CpptSkipRef::yes,CpptSkipConst::yes) << ","
                << describeCppType(that->right->type,CpptSubstitureRef::no,CpptSkipRef::yes,CpptSkipConst::yes)
                << ">::clone(";
        }
        virtual void preVisitRight ( ExprClone * that, Expression * right ) override {
            Visitor::preVisitRight(that,right);
            ss << ",";
        }
        virtual ExpressionPtr visit ( ExprClone * that ) override {
            ss << ")";
            return Visitor::visit(that);
        }
    // move
        virtual void preVisit ( ExprMove * that ) override {
            Visitor::preVisit(that);
            ss << "das_move(";
        }
        virtual void preVisitRight ( ExprMove * that, Expression * right ) override {
            Visitor::preVisitRight(that,right);
            ss << ",";
        }
        virtual ExpressionPtr visit ( ExprMove * that ) override {
            ss << ")";
            return Visitor::visit(that);
        }
    // op1
        void outPolicy ( const TypeDeclPtr & decl ) {
            if ( decl->baseType!=Type::tHandle ){
                ss << "SimPolicy<" << das_to_cppString(decl->baseType) << ">";
            } else {
                auto pname = decl->annotation->cppName.empty() ? decl->annotation->name : decl->annotation->cppName;
                ss << "SimPolicy<" << pname << ">";
            }
        }
        bool isOpPolicy ( ExprOp1 * that ) const {
            if ( isalpha(that->op[0]) ) return true;
            return that->subexpr->type->isPolicyType();
        }
        virtual void preVisit ( ExprOp1 * that ) override {
            Visitor::preVisit(that);
            if ( !that->func->builtIn || that->func->callBased ) {
                that->arguments.clear();
                that->arguments.push_back(that->subexpr);
                CallFunc_preVisit(that);
                CallFunc_preVisitCallArg(that, that->subexpr.get(), true);
            } else if ( isOpPolicy(that) ){
                outPolicy(that->subexpr->type);
                ss << "::" << opPolicyName(that) << "(";
            } else {
                if ( that->op!="+++" && that->op!="---" ) {
                    ss << that->op;
                }
                if ( !noBracket(that) && !that->subexpr->bottomLevel ) ss << "(";
            }
        }
        virtual ExpressionPtr visit ( ExprOp1 * that ) override {
            if ( !that->func->builtIn || that->func->callBased ) {
                CallFunc_visitCallArg(that, that->subexpr.get(), true);
                CallFunc_visit(that);
                that->arguments.clear();
            } else if ( isOpPolicy(that) ){
                ss << ",*__context__,nullptr)";
            } else {
                if ( that->op=="+++" || that->op=="---" ) {
                    ss << that->op[0] << that->op[1];
                }
                if ( !noBracket(that) && !that->subexpr->bottomLevel ) ss << ")";
            }
            return Visitor::visit(that);
        }
    // op2
        bool isSetBool ( ExprOp2 * that ) const {
            return (that->op=="||=" || that->op=="&&=" || that->op=="^^=") && that->right->type->isSimpleType(Type::tBool);
        }
        bool isOpPolicy ( ExprOp2 * that ) const {
            if ( isalpha(that->op[0]) ) return true;
            if ( that->op=="/" || that->op=="%" ) return true;
            if ( that->op=="<<<" || that->op==">>>" || that->op=="<<<=" || that->op==">>>=" ) return true;
            return that->type->isPolicyType() || that->left->type->isPolicyType() || that->right->type->isPolicyType();
        }
        const TypeDeclPtr & opPolicyBase ( ExprOp2 * that ) const {
            if ( that->type->isPolicyType() ) return that->type;
            else if ( that->left->type->isPolicyType() ) return that->left->type;
            else return that->right->type;
        }
        string opPolicyName ( ExprOp * that ) {
            if ( that->func->builtIn ) {
                auto bfn = static_cast<BuiltInFunction *>(that->func);
                return bfn->cppName.empty() ? bfn->name : bfn->cppName;
            } else {
                return "/* NotAPolicy */";
            }
        }
        bool isRefPolicyOp(ExprOp2 * that) {
            return
                // math
                   (that->op == "+=")
                || (that->op == "-=")
                || (that->op == "*=")
                || (that->op == "/=")
                || (that->op == "%=")
                // bin
                || (that->op == "&=")
                || (that->op == "|=")
                || (that->op == "^=")
                // bool
                || (that->op == "&&=")
                || (that->op == "||=")
                || (that->op == "^^=")
                // rotational
                || (that->op == "<<=")
                || (that->op == ">>=")
                || (that->op == "<<<=")
                || (that->op == ">>>=")
                ;
        }
        virtual void preVisit ( ExprOp2 * that ) override {
            Visitor::preVisit(that);
            if ( !noBracket(that) ) ss << "(";
            if ( !that->func->builtIn || that->func->callBased ) {
                that->arguments.clear();
                that->arguments.push_back(that->left);
                that->arguments.push_back(that->right);
                CallFunc_preVisit(that);
                CallFunc_preVisitCallArg(that, that->left.get(), false);
            } else if ( isOpPolicy(that) ) {
                auto pt = opPolicyBase(that);
                if ( policyResultNeedCast(pt, that->type) ) {
                    ss << "cast<" << describeCppType(that->type,CpptSubstitureRef::no,CpptSkipRef::yes,CpptSkipConst::yes) << ">::to(";
                }
                outPolicy(pt);
                ss << "::" << opPolicyName(that) << "(";
                if ( isRefPolicyOp(that) ) {
                    ss << "(char *)&(";
                } else if ( policyArgNeedCast(pt, that->left->type) ) {
                    if (that->left->type->isRefType()) {
                        ss << "cast<" << describeCppType(that->left->type, CpptSubstitureRef::no, CpptSkipRef::yes, CpptSkipConst::yes) << "*>::from(&(";
                    } else {
                        ss << "cast<" << describeCppType(that->left->type, CpptSubstitureRef::no, CpptSkipRef::yes, CpptSkipConst::yes) << ">::from(";
                    }
                }
            } else if ( isSetBool(that) ) {
                if ( that->op=="||=" ) ss << "DAS_SETBOOLOR((";
                else if ( that->op=="&&=" ) ss << "DAS_SETBOOLAND((";
                else if ( that->op=="^^=" ) ss << "DAS_SETBOOLXOR((";
            }
        }
        virtual void preVisitRight ( ExprOp2 * that, Expression * right ) override {
            Visitor::preVisitRight(that,right);
            if ( !that->func->builtIn || that->func->callBased ) {
                CallFunc_visitCallArg(that, that->left.get(), false);
                CallFunc_preVisitCallArg(that, that->right.get(), true);
            } else if ( isOpPolicy(that) ) {
                auto pt = opPolicyBase(that);
                if ( isRefPolicyOp(that) ) {
                    ss << ")";
                } else if ( policyArgNeedCast(pt, that->left->type) ) {
                    if (that->left->type->isRefType()) {
                        ss << "))";
                    } else {
                        ss << ")";
                    }
                }
                ss << ",";
                if ( policyArgNeedCast(pt, that->right->type) ) {
                    if (that->right->type->isRefType()) {
                        ss << "cast<" << describeCppType(that->right->type, CpptSubstitureRef::no, CpptSkipRef::yes, CpptSkipConst::yes) << " *>::from(&(";
                    } else {
                        ss << "cast<" << describeCppType(that->right->type, CpptSubstitureRef::no, CpptSkipRef::yes, CpptSkipConst::yes) << ">::from(";
                    }
                }
            } else if ( isSetBool(that) ) {
                ss << "),(";
            } else {
                if ( that->type->baseType==Type::tBool ) {
                    ss << " ";
                         if ( that->op=="&" )  ss << "&&";
                    else if ( that->op=="|" )  ss << "||";
                    else if ( that->op=="^" )  ss << "!=";
                    else if ( that->op=="^^" ) ss << "!=";
                    else ss << that->op;
                    ss << " ";
                } else {
                    ss << " " << that->op << " ";
                }
            }
        }
        virtual ExpressionPtr visit ( ExprOp2 * that ) override {
            if ( !that->func->builtIn || that->func->callBased ) {
                CallFunc_visitCallArg(that, that->right.get(), true);
                CallFunc_visit(that);
                that->arguments.clear();
            } else if ( isOpPolicy(that) ) {
                auto pt = opPolicyBase(that);
                if (policyArgNeedCast(pt, that->right->type)) {
                    if (that->right->type->isRefType()) {
                        ss << "))";
                    } else {
                        ss << ")";
                    }
                }
                ss << ",*__context__,nullptr)";
                if ( policyResultNeedCast(pt, that->type) ) {
                    ss << ")";
                }
            } else if ( isSetBool(that) ) {
                ss << "))";
            }
            if ( !noBracket(that) ) ss << ")";
            return Visitor::visit(that);
        }
    // op3
        virtual void preVisit ( ExprOp3 * that ) override {
            Visitor::preVisit(that);
            if ( !noBracket(that) ) ss << "(";
        }
        virtual void preVisitLeft  ( ExprOp3 * that, Expression * left ) override {
            Visitor::preVisitLeft(that,left);
            ss << " ? ";
            auto argT = left->type;
            if ( isLocalVec(argT) ) {
                ss << "(vec4f)";
            }
            if ( that->type->isRef() ) {
                ss << "das_auto_cast_ref<";
            } else {
                ss << "das_auto_cast<";
            }
            ss << describeCppType(that->type, CpptSubstitureRef::no, CpptSkipRef::no) << ">::cast(";
        }
        virtual void preVisitRight ( ExprOp3 * that, Expression * right ) override {
            Visitor::preVisitRight(that,right);
            ss << ") : ";
            auto argT = right->type;
            if ( isLocalVec(argT) ) {
                ss << "(vec4f)";
            }
            if ( that->type->isRef() ) {
                ss << "das_auto_cast_ref<";
            } else {
                ss << "das_auto_cast<";
            }
            ss << describeCppType(that->type, CpptSubstitureRef::no, CpptSkipRef::no) << ">::cast(";
        }
        virtual ExpressionPtr visit ( ExprOp3 * that ) override {
            ss << ")";
            if ( !noBracket(that) ) ss << ")";
            return Visitor::visit(that);
        }
    // return
        virtual void preVisit ( ExprReturn * expr ) override {
            Visitor::preVisit(expr);
            ss << "return ";
            if ( expr->moveSemantics ) ss << "/* <- */ ";
            auto retT = expr->returnFunc ? expr->returnFunc->result : expr->block->returnType;
            if ( !retT->isVoid() ) {
                if ( expr->moveSemantics ) {
                    ss << "das_auto_cast_move<";
                } else {
                    if ( retT->isRef() ) {
                        ss << "das_auto_cast_ref<";
                    } else {
                        ss << "das_auto_cast<";
                    }
                }
                ss << describeCppType(retT, CpptSubstitureRef::no, CpptSkipRef::no) << ">::cast(";
            }
        }
        virtual ExpressionPtr visit(ExprReturn* expr) override {
            auto retT = expr->returnFunc ? expr->returnFunc->result : expr->block->returnType;
            if (!retT->isVoid()) ss << ")";
            return Visitor::visit(expr);
        }
    // break
        virtual void preVisit ( ExprBreak * that ) override {
            Visitor::preVisit(that);
            ss << "break";
        }
    // continue
        virtual void preVisit ( ExprContinue * that ) override {
            Visitor::preVisit(that);
            ss << "continue";
        }
    // var
        virtual void preVisit ( ExprVar * var ) override {
            Visitor::preVisit(var);
            if ( var->type->aotAlias ) {
                ss << "das_alias<" << var->type->alias << ">::from(";
            }
        }
        virtual ExpressionPtr visit ( ExprVar * var ) override {
            if ( var->local && var->variable->type->ref ) {
                ss << "(*" << collector.getVarName(var->variable) << ")";
            } else if ( var->local || var->block || var->argument ) {
                ss << collector.getVarName(var->variable);
            } else {
                ss << (var->variable->global_shared ? "das_shared" : "das_global");
                ss << "<" << describeCppType(var->variable->type,CpptSubstitureRef::no,CpptSkipRef::yes,CpptSkipConst::yes)
                    << ",0x" << HEX << var->variable->getMangledNameHash() << DEC << ">(__context__) /*" << var->name << "*/";
            }
            if ( var->type->aotAlias ) {
                ss << ")";
            }
            return Visitor::visit(var);
        }
    // null coaelescing
        virtual void preVisit ( ExprNullCoalescing * nc ) override {
            Visitor::preVisit(nc);
            if ( nc->type->aotAlias ) {
                ss << "das_alias<" << nc->type->alias << ">::from(";
            }
            ss << "das_null_coalescing<" << describeCppType(nc->defaultValue->type,CpptSubstitureRef::no,CpptSkipRef::no,CpptSkipConst::no)
                << ">::get(";
            if ( nc->subexpr->type->isAotAlias() ) {
                ss << "(" << describeCppType(nc->defaultValue->type,CpptSubstitureRef::no,CpptSkipRef::no,CpptSkipConst::no) << " *)";
            }
        }
        virtual void preVisitNullCoaelescingDefault ( ExprNullCoalescing * nc, Expression * expr ) override {
            Visitor::preVisitNullCoaelescingDefault(nc,expr);
            ss << ",";
        }
        virtual ExpressionPtr visit ( ExprNullCoalescing * nc ) override {
            ss << ")";
            if ( nc->type->aotAlias ) {
                ss << ")";
            }
            return Visitor::visit(nc);
        }
    // is variant
        virtual void preVisit(ExprIsVariant * field) override {
            Visitor::preVisit(field);
            ss << "das_get_variant_field<"
                << describeCppType(field->value->type->argTypes[field->fieldIndex])
                << ","
                << field->value->type->getVariantFieldOffset(field->fieldIndex)
                << ","
                << field->fieldIndex
                << ">::is(";
        }
        virtual ExpressionPtr visit ( ExprIsVariant * field ) override {
            ss << ")";
            return Visitor::visit(field);
        }
    // as variant
        virtual void preVisit(ExprAsVariant * field) override {
            Visitor::preVisit(field);
            if ( field->type->aotAlias ) {
                ss << "das_alias<" << field->type->alias << ">::from(";
            }
            ss << "das_get_variant_field<"
                << describeCppType(field->value->type->argTypes[field->fieldIndex])
                << ","
                << field->value->type->getVariantFieldOffset(field->fieldIndex)
                << ","
                << field->fieldIndex
                << ">::as(";
        }
        virtual ExpressionPtr visit ( ExprAsVariant * field ) override {
            ss << ",__context__)";
            if ( field->type->aotAlias ) {
                ss << ")";
            }
            return Visitor::visit(field);
        }
    // safe as variant
        virtual void preVisit(ExprSafeAsVariant * field) override {
            Visitor::preVisit(field);
            auto fieldT = field->value->type->isPointer() ? field->value->type->firstType :  field->value->type;
            if ( fieldT->aotAlias ) {
                ss << "das_alias<" << fieldT->alias << ">::from(";
            }
            ss << "das_get_variant_field<"
                << describeCppType(fieldT->argTypes[field->fieldIndex])
                << ","
                << fieldT->getVariantFieldOffset(field->fieldIndex)
                << ","
                << field->fieldIndex
                << ">::safe_as"
                << (field->skipQQ ? "_ptr" : "")
                << "(";
        }
        virtual ExpressionPtr visit ( ExprSafeAsVariant * field ) override {
            ss << ")";
            auto fieldT = field->value->type->isPointer() ? field->value->type->firstType :  field->value->type;
            if ( fieldT->aotAlias ) {
                ss << ")";
            }
            return Visitor::visit(field);
        }
    // safe field
        virtual void preVisit ( ExprSafeField * field ) override {
            Visitor::preVisit(field);
            if ( field->type->aotAlias ) {
                ss << "das_alias<" << field->type->alias << ">::from(";
            }
            const auto & vtype = field->value->type->firstType;
            ss << (vtype->isHandle() ? "das_safe_navigation_handle" : "das_safe_navigation");
            if ( vtype->isGoodTupleType() ) ss << "_tuple";
            else if ( vtype->isGoodVariantType() ) ss << "_variant";
            else if ( field->skipQQ ) ss << "_ptr";
            ss << "<";
            if ( !vtype->isGoodTupleType() && !vtype->isGoodVariantType() ) {
                ss << describeCppType(field->value->type->firstType) << ",";
            }
            if ( field->skipQQ ) {
                ss << describeCppType(field->type);
            } else {
                ss << describeCppType(field->type->firstType);
            }
            if ( vtype->isHandle() ) {
                ss  << ">::get(";
            } else if ( vtype->isGoodTupleType() ) {
                ss  << ", " << vtype->getTupleFieldOffset(field->fieldIndex)
                    << ">::get(";
            } else if ( vtype->isGoodVariantType() ) {
                ss  << ", " << vtype->getVariantFieldOffset(field->fieldIndex)
                    << ", " << field->fieldIndex
                    <<  ">::get(";
            } else {
                ss  << ",&" << (vtype->structType->module->name.empty() ? "" : vtype->structType->module->name + "::")
                    << vtype->structType->name << "::" << field->name << ">::get(";
            }
        }
        virtual ExpressionPtr visit ( ExprSafeField * field ) override {
            const auto & vtype = field->value->type->firstType;
            if ( vtype->isHandle() ) {
                ss  << ",([&](const "
                    << describeCppType(vtype,CpptSubstitureRef::no,CpptSkipRef::no,CpptSkipConst::yes,CpptRedundantConst::yes)
                    << " * __any) -> auto & {return ";
                vtype->annotation->aotPreVisitGetFieldPtr(ss, field->name);
                ss  << "__any";
                vtype->annotation->aotVisitGetFieldPtr(ss, field->name);
                ss << " /*" << field->name << "*/";
                ss << ";})";
            }
            ss << ")";
            if ( field->type->aotAlias ) {
                ss << ")";
            }
            return Visitor::visit(field);
        }
    // field
        virtual void preVisit ( ExprField * field ) override {
            Visitor::preVisit(field);
            if ( field->type->aotAlias ) {
                ss << "das_alias<" << field->type->alias << ">::from(";
            }
            if ( field->value->type->isBitfield() ) {
                ss << "das_get_bitfield(";
            } else if ( field->value->type->isTuple() ) {
                ss << "das_get_tuple_field<"
                    << describeCppType(field->value->type->argTypes[field->fieldIndex])
                    << ","
                    << field->value->type->getTupleFieldOffset(field->fieldIndex)
                    << ">::get(";
            } else if ( field->value->type->isVariant() ) {
                ss << "das_get_variant_field<"
                    << describeCppType(field->value->type->argTypes[field->fieldIndex])
                    << ","
                    << field->value->type->getVariantFieldOffset(field->fieldIndex)
                    << ","
                    << field->fieldIndex
                    << ">::get(";
            } else if ( field->value->type->isHandle() ) {
                if (field->type->isString()) {
                    ss << "((" << describeCppType(field->type) << ")(";  // c-cast const char * etc string casts to char * or char * const
                }
                field->value->type->annotation->aotPreVisitGetField(ss, field->name);
            } else if ( field->value->type->baseType==Type::tPointer ) {
                if ( field->value->type->firstType->isHandle() ) {
                    field->value->type->firstType->annotation->aotPreVisitGetFieldPtr(ss, field->name);
                } else if ( field->value->type->firstType->isTuple() ) {
                    ss << "das_get_tuple_field_ptr<"
                        << describeCppType(field->value->type->firstType->argTypes[field->fieldIndex])
                        << ","
                        << field->value->type->firstType->getTupleFieldOffset(field->fieldIndex)
                        << ">::get(";
                } else if ( field->value->type->firstType->isVariant() ) {
                    ss << "das_get_variant_field_ptr<"
                        << describeCppType(field->value->type->firstType->argTypes[field->fieldIndex])
                        << ","
                        << field->value->type->firstType->getVariantFieldOffset(field->fieldIndex)
                        << ","
                        << field->fieldIndex
                        << ">::get(";
                }
            }
        }
        virtual ExpressionPtr visit ( ExprField * field ) override {
            if ( field->value->type->isBitfield() ) {
                ss << ",1u << " << field->fieldIndex << ")";
            } else if ( field->value->type->isTuple() ) {
                ss << ")";
            } else if ( field->value->type->isVariant() ) {
                ss << ")";
            } else if ( field->value->type->isHandle() ) {
                field->value->type->annotation->aotVisitGetField(ss, field->name);
                ss << " /*" << field->name << "*/";
                if (field->type->isString()) {
                    ss << "))";
                }
            } else if ( field->value->type->baseType==Type::tPointer ) {
                if ( field->value->type->firstType->isHandle() ) {
                    field->value->type->firstType->annotation->aotVisitGetFieldPtr(ss, field->name);
                    ss << " /*" << field->name << "*/";
                } else if ( field->value->type->firstType->isTuple() ) {
                    ss << ")";
                } else if ( field->value->type->firstType->isVariant() ) {
                    ss << ")";
                } else {
                    ss << "->" << field->name;
                }
            } else {
                ss << "." << field->name;
            }
            if ( field->type->aotAlias ) {
                ss << ")";
            }
            return Visitor::visit(field);
        }
    // at
        virtual void preVisit ( ExprAt * expr ) override {
            Visitor::preVisit(expr);
            if ( expr->type->aotAlias ) {
                ss << "das_alias<" << expr->type->alias << ">::from(";
            }
            if ( !(expr->subexpr->type->dim.size() || expr->subexpr->type->isGoodArrayType() || expr->subexpr->type->isGoodTableType()) ) {
                ss << "das_index<" << describeCppType(expr->subexpr->type,CpptSubstitureRef::no,CpptSkipRef::yes,CpptSkipConst::no)
                    << ">::at(";
            }
        }
        virtual void preVisitAtIndex ( ExprAt * expr, Expression * index ) override {
            Visitor::preVisitAtIndex(expr, index);
            if ( expr->subexpr->type->dim.size() || expr->subexpr->type->isGoodArrayType() || expr->subexpr->type->isGoodTableType() ) {
                if ( expr->subexpr->type->isNativeDim ) {
                    ss << "[";
                } else {
                    ss << "(";
                }
            } else {
                ss << ",";
            }

        }
        virtual ExpressionPtr visit ( ExprAt * expr ) override {
            if ( expr->subexpr->type->isNativeDim ) {
                ss << "]";
            } else {
                ss << ",__context__)";
            }
            if ( expr->type->aotAlias ) {
                ss << ")";
            }
            return Visitor::visit(expr);
        }
    // safe at
        virtual void preVisit ( ExprSafeAt * expr ) override {
            Visitor::preVisit(expr);
            bool isPtr = expr->subexpr->type->isPointer();
            const auto & seT = isPtr ? expr->subexpr->type->firstType : expr->subexpr->type;
            if ((seT->dim.size() || seT->isGoodArrayType() || seT->isGoodTableType())) {
                ss << describeCppType(seT,CpptSubstitureRef::no,CpptSkipRef::yes,CpptSkipConst::yes) << "::safe_index(";
            } else {
                ss << "das_index<" << describeCppType(seT,CpptSubstitureRef::no,CpptSkipRef::yes,CpptSkipConst::no)
                    << ">::safe_at(";
            }
            if (isPtr) ss << "("; else ss << "&(";
        }
        virtual void preVisitSafeAtIndex ( ExprSafeAt * expr, Expression * index ) override {
            Visitor::preVisitSafeAtIndex(expr, index);
            ss << "),";
        }
        virtual ExpressionPtr visit ( ExprSafeAt * that ) override {
            ss << ",__context__)";
            return Visitor::visit(that);
        }
    // const
        virtual ExpressionPtr visit(ExprFakeContext * c) override {
            ss << "__context__";
            return Visitor::visit(c);
        }
        virtual ExpressionPtr visit(ExprFakeLineInfo * c) override {
            ss << "((LineInfoArg *)(&LineInfo::g_LineInfoNULL))";
            return Visitor::visit(c);
        }
        virtual ExpressionPtr visit ( ExprConstPtr * c ) override {
            if ( c->getValue() ) {
                ss << "((void *) 0x" << HEX << intptr_t(c->getValue()) << DEC << ")";
            } else {
                ss << "nullptr";
            }
            return Visitor::visit(c);
        }
        virtual ExpressionPtr visit ( ExprConstEnumeration * c ) override {
            ss << describeCppType(c->type,CpptSubstitureRef::no,CpptSkipRef::yes,CpptSkipConst::yes,CpptRedundantConst::no);
            auto ctext = c->text;
            for ( auto & ee : c->enumType->list ) {
                if ( ee.name==c->text ) {
                    if ( !ee.cppName.empty() ) {
                        ctext = ee.cppName;
                    }
                    break;
                }
            }
            ss << "::" << ctext;
            return Visitor::visit(c);
        }
        virtual ExpressionPtr visit ( ExprConstInt * c ) override {
            ss << c->getValue();
            return Visitor::visit(c);
        }
        virtual ExpressionPtr visit ( ExprConstInt8 * c ) override {
            ss << int32_t(c->getValue());
            return Visitor::visit(c);
        }
        virtual ExpressionPtr visit ( ExprConstInt16 * c ) override {
            ss << int32_t(c->getValue());
            return Visitor::visit(c);
        }
        virtual ExpressionPtr visit ( ExprConstInt64 * c ) override {
            ss << "INT64_C(" << c->getValue() << ")";
            return Visitor::visit(c);
        }
        virtual ExpressionPtr visit ( ExprConstUInt8 * c ) override {
            ss << "0x" << HEX << uint32_t(c->getValue()) << DEC;
            return Visitor::visit(c);
        }
        virtual ExpressionPtr visit ( ExprConstUInt16 * c ) override {
            ss << "0x" << HEX << uint32_t(c->getValue()) << DEC;
            return Visitor::visit(c);
        }
        virtual ExpressionPtr visit ( ExprConstUInt64 * c ) override {
            ss << "UINT64_C(0x" << HEX << c->getValue() << DEC << ")";
            return Visitor::visit(c);
        }
        virtual ExpressionPtr visit ( ExprConstUInt * c ) override {
            ss << "0x" << HEX << c->getValue() << DEC << "u";
            return Visitor::visit(c);
        }
        virtual ExpressionPtr visit ( ExprConstBitfield * c ) override {
            ss << "0x" << HEX << c->getValue() << DEC << "u";
            return Visitor::visit(c);
        }
        virtual ExpressionPtr visit ( ExprConstBool * c ) override {
            ss << (c->getValue() ? "true" : "false");
            return Visitor::visit(c);
        }
        void writeOutDouble ( double val ) {
            if ( val==DBL_MIN ) ss << "DBL_MIN";
            else if ( val==-DBL_MIN ) ss << "(-DBL_MIN)";
            else if ( val==DBL_MAX ) ss << "DBL_MAX";
            else if ( val==-DBL_MAX ) ss << "(-DBL_MAX)";
            else ss << to_string_ex(val);
        }
        virtual ExpressionPtr visit ( ExprConstDouble * c ) override {
            writeOutDouble(c->getValue());
            return Visitor::visit(c);
        }
        void writeOutFloat ( float val ) {
            if ( val==FLT_MIN ) ss << "FLT_MIN";
            else if ( val==-FLT_MIN ) ss << "(-FLT_MIN)";
            else if ( val==FLT_MAX ) ss << "FLT_MAX";
            else if ( val==-FLT_MAX ) ss << "(-FLT_MAX)";
            else ss << to_string_ex(val) << "f";
        }
        virtual ExpressionPtr visit ( ExprConstFloat * c ) override {
            writeOutFloat(c->getValue());
            return Visitor::visit(c);
        }
        virtual ExpressionPtr visit ( ExprConstString * c ) override {
            if (c->text.empty())
                ss << "nullptr";
            else
                ss << "((char *) \"" << escapeString(c->text,false) << "\")";
            return Visitor::visit(c);
        }
        virtual ExpressionPtr visit ( ExprConstInt2 * c ) override {
            auto val = c->getValue();
            ss << "int2(" << val.x << "," << val.y << ")";
            return Visitor::visit(c);
        }
        virtual ExpressionPtr visit ( ExprConstRange * c ) override {
            auto val = c->getValue();
            ss << "range(" << val.from << "," << val.to << ")";
            return Visitor::visit(c);
        }
        virtual ExpressionPtr visit ( ExprConstRange64 * c ) override {
            auto val = c->getValue();
            ss << "range64(" << val.from << "," << val.to << ")";
            return Visitor::visit(c);
        }
        virtual ExpressionPtr visit ( ExprConstInt3 * c ) override {
            auto val = c->getValue();
            ss << "int3(" << val.x << "," << val.y << "," << val.z << ")";
            return Visitor::visit(c);
        }
        virtual ExpressionPtr visit ( ExprConstInt4 * c ) override {
            auto val = c->getValue();
            ss << "int4(" << val.x << "," << val.y << "," << val.z << "," << val.w << ")";
            return Visitor::visit(c);
        }
        virtual ExpressionPtr visit ( ExprConstUInt2 * c ) override {
            auto val = c->getValue();
            ss << "uint2(" << val.x << "," << val.y << ")";
            return Visitor::visit(c);
        }
        virtual ExpressionPtr visit ( ExprConstURange * c ) override {
            auto val = c->getValue();
            ss << "urange(" << val.from << "," << val.to << ")";
            return Visitor::visit(c);
        }
        virtual ExpressionPtr visit ( ExprConstURange64 * c ) override {
            auto val = c->getValue();
            ss << "urange64(" << val.from << "," << val.to << ")";
            return Visitor::visit(c);
        }
        virtual ExpressionPtr visit ( ExprConstUInt3 * c ) override {
            auto val = c->getValue();
            ss << "uint3(" << val.x << "," << val.y << "," << val.z << ")";
            return Visitor::visit(c);
        }
        virtual ExpressionPtr visit ( ExprConstUInt4 * c ) override {
            auto val = c->getValue();
            ss << "uint4(" << val.x << "," << val.y << "," << val.z << "," << val.w << ")";
            return Visitor::visit(c);
        }
        virtual ExpressionPtr visit ( ExprConstFloat2 * c ) override {
            auto val = c->getValue();
            if (val.x == 0.0f && val.y == 0.0f ) {
                ss << "v_zero()";
            } else if (val.x == val.y ) {
                ss << "v_splats(";
                writeOutFloat(val.x);
                ss << ")";
            } else {
                ss << "v_make_vec4f(";
                writeOutFloat(val.x);
                ss << ",";
                writeOutFloat(val.y);
                ss << ",0.f,0.f)";
            }
            return Visitor::visit(c);
        }
        virtual ExpressionPtr visit ( ExprConstFloat3 * c ) override {
            auto val = c->getValue();
            if (val.x == 0.0f && val.y == 0.0f && val.z == 0.0f) {
                ss << "v_zero()";
            } else if (val.x == val.y && val.x == val.z) {
                ss << "v_splats(";
                writeOutFloat(val.x);
                ss << ")";
            } else {
                ss << "v_make_vec4f(";
                writeOutFloat(val.x);
                ss << ",";
                writeOutFloat(val.y);
                ss << ",";
                writeOutFloat(val.z);
                ss << ",0.f)";
            }
            return Visitor::visit(c);
        }
        virtual ExpressionPtr visit ( ExprConstFloat4 * c ) override {
            auto val = c->getValue();
            if (val.x == 0.0f && val.y == 0.0f && val.z == 0.0f && val.w == 0.0f ) {
                ss << "v_zero()";
            } else if (val.x == val.y && val.x == val.z && val.x == val.w ) {
                ss << "v_splats(";
                writeOutFloat(val.x);
                ss << ")";
            } else {
                ss << "v_make_vec4f(";
                writeOutFloat(val.x);
                ss << ",";
                writeOutFloat(val.y);
                ss << ",";
                writeOutFloat(val.z);
                ss << ",";
                writeOutFloat(val.w);
                ss << ")";
            }
            return Visitor::visit(c);
        }
    // ExprAssume
        virtual void preVisit ( ExprAssume * expr ) override {
            Visitor::preVisit(expr);
            ss << "\n#if 0 // with, note optimizations are off\n";
        }
        virtual ExpressionPtr visit ( ExprAssume * expr ) override {
            Visitor::visit(expr);
            ss << "\n#endif\n";
            return expr;
        }
    // ExprWith
        virtual void preVisit ( ExprWith * expr ) override {
            Visitor::preVisit(expr);
            ss << "\n#if 0 // with, note optimizations are off\n";
        }
        virtual void preVisitWithBody ( ExprWith * expr, Expression * body ) override {
            Visitor::preVisitWithBody(expr, body);
            ss << "\n#endif\n";
        }
    // ExprWhile
        virtual void preVisit ( ExprWhile * wh ) override {
            Visitor::preVisit(wh);
            if ( wh->body->rtti_isBlock() ) {
                auto * block = static_cast<ExprBlock *>(wh->body.get());
                if ( !block->finalList.empty() ) {
                    ss << "{\n";
                    tab ++;
                    block->visitFinally(*this);
                    ss << string(tab,'\t');
                }
            }
            ss << "while ( ";
        }
        virtual void preVisitWhileBody ( ExprWhile * wh, Expression * body ) override {
            Visitor::preVisitWhileBody(wh,body);
            ss << " )\n";
            ss << string(tab,'\t');
        }
        virtual ExpressionPtr visit ( ExprWhile * wh ) override {
            if ( wh->body->rtti_isBlock() ) {
                auto * block = static_cast<ExprBlock *>(wh->body.get());
                if ( !block->finalList.empty() ) {
                    tab --;
                    ss << "\n" << string(tab,'\t') << "}";
                }
            }
            return Visitor::visit(wh);
        }
    // if then else
        virtual void preVisit ( ExprIfThenElse * ifte ) override {
            Visitor::preVisit(ifte);
            ss << "if ( ";
        }
        virtual void preVisitIfBlock ( ExprIfThenElse * ifte, Expression * block ) override {
            Visitor::preVisitIfBlock(ifte,block);
            ss << " )\n";
            ss << string(tab,'\t');
        }
        virtual void preVisitElseBlock ( ExprIfThenElse * ifte, Expression * block ) override {
            Visitor::preVisitElseBlock(ifte, block);
            ss << " else ";
        }
    // swizzle
        virtual void preVisit ( ExprSwizzle * expr ) override {
            Visitor::preVisit(expr);
            if ( expr->type->ref ) {
                ss << "das_swizzle_ref<"
                    << describeCppType(expr->type,CpptSubstitureRef::no,CpptSkipRef::yes) << ","
                    << describeCppType(expr->value->type,CpptSubstitureRef::no,CpptSkipRef::yes) << ","
                    << int32_t(expr->fields[0]) << ">::swizzle(";
            } else {
                if ( expr->fields.size()==1 ) {
                    const char * mask = "xyzw";
                    ss << "v_extract_" << mask[expr->fields[0]];
                    if ( expr->type->baseType!=Type::tFloat ) ss << "i";
                    ss << "(";
                    if (expr->type->baseType != Type::tFloat) ss << "v_cast_vec4i(";
                } else if ( TypeDecl::isSequencialMask(expr->fields) ) {
                    ss << "das_swizzle_seq<"
                    << describeCppType(expr->type,CpptSubstitureRef::no,CpptSkipRef::yes) << ","
                    << describeCppType(expr->value->type,CpptSubstitureRef::no,CpptSkipRef::yes) << ","
                    << int32_t(expr->fields[0]) << ">::swizzle(";
                } else {
                    ss << "das_swizzle<"
                        << describeCppType(expr->type,CpptSubstitureRef::no,CpptSkipRef::yes) << ","
                        << describeCppType(expr->value->type,CpptSubstitureRef::no,CpptSkipRef::yes);
                    for ( size_t i=0, its=expr->fields.size(); i!=its; ++i ) {
                        ss << ",";
                        ss << int32_t(expr->fields[i]);
                    }
                    ss << ">::swizzle(";
                }
            }
            if ( expr->value->type->aotAlias ) {
                ss << "das_alias<" << expr->value->type->alias << ">::from(";
            }
         }
        virtual ExpressionPtr visit ( ExprSwizzle * expr ) override {
            if ( expr->value->type->aotAlias ) {
                ss << ")";
            }
            if ( expr->type->ref ) {
                ss << ")";
            } else {
                if ( expr->fields.size()==1 ) {
                    if (expr->type->baseType != Type::tFloat) ss << ")";
                    ss << ")";
                } else if ( TypeDecl::isSequencialMask(expr->fields) ) {
                    ss << ")";
                } else {
                    ss << ")";
                }
            }
            ss << " /*";
            for ( auto f : expr->fields ) {
                switch ( f ) {
                    case 0:     ss << "x"; break;
                    case 1:     ss << "y"; break;
                    case 2:     ss << "z"; break;
                    case 3:     ss << "w"; break;
                    default:    ss << "?"; break;
                }
            }
            ss << "*/";
            return Visitor::visit(expr);
        }
    // string builder
        string outputCallTypeInfo ( uint32_t nArgs, vector<ExpressionPtr> & elements ) {
            vector<TypeInfo*> elInfo;
            elInfo.reserve(elements.size());
            for ( auto & el : elements ) {
                TypeInfo * info = helper.makeTypeInfo(nullptr, el->type);
                elInfo.push_back(info);
            }
            string debug_info_name = "__tinfo_" + to_string(debugInfoGlobal++);
            sti << "TypeInfo * " << debug_info_name << "[" << nArgs << "] = { ";
            for ( size_t i=0, is=elInfo.size(); i!=is; ++i ) {
                auto info = elInfo[i];
                if ( i ) sti << ", ";
                sti << "&" << helper.typeInfoName(info);
            }
            sti << " };\n";
            return debug_info_name;
        }
        virtual void preVisit ( ExprStringBuilder * expr ) override {
            Visitor::preVisit(expr);
            uint32_t nArgs = uint32_t(expr->elements.size());
            ss << "das_string_builder(__context__,SimNode_AotInterop<" << nArgs << ">(";
            if ( nArgs ) {
                auto debug_info_name = outputCallTypeInfo(nArgs, expr->elements);
                ss << debug_info_name << ", ";
            }
        }
        virtual void preVisitStringBuilderElement ( ExprStringBuilder * sb, Expression * expr, bool last ) override {
            Visitor::preVisitStringBuilderElement(sb, expr, last);
            ss << "cast<" << describeCppType(expr->type);
            if ( expr->type->isRefType() && !expr->type->ref ) {
                ss << " &";
            }
            ss << ">::from(";
        }
        virtual ExpressionPtr visitStringBuilderElement ( ExprStringBuilder * sb, Expression * expr, bool last ) override {
            ss << ")";
            if ( !last ) ss << ", ";
            return Visitor::visitStringBuilderElement(sb, expr, last);
        }
        virtual ExpressionPtr visit ( ExprStringBuilder * expr ) override {
            ss << "))";
            return Visitor::visit(expr);
        }
    // typedecl
        virtual void preVisit ( ExprTypeDecl * expr ) override {
            ss << "das_typedecl_value<" << describeCppType(expr->typeexpr) << ">()()";
        }
    // type-info
        virtual void preVisit ( ExprTypeInfo * expr ) override {
            Visitor::preVisit(expr);
            DAS_ASSERT(expr->macro && "internal error. we should only be here if there is a macro.");
            ss << "(";
            expr->macro->aotPrefix(ss, expr);
            if ( expr->macro->aotNeedTypeInfo(expr) ) {
                TypeInfo * info = helper.makeTypeInfo(nullptr, expr->typeexpr);
                ss << helper.typeInfoName(info);
            }
        }
        virtual bool canVisitExpr ( ExprTypeInfo * expr, Expression * ) override {
            DAS_ASSERT(expr->macro && "internal error. we should only be here if there is a macro.");
            return expr->macro->aotInfix(ss, expr);
        }
        virtual ExpressionPtr visit ( ExprTypeInfo * expr ) override {
            DAS_ASSERT(expr->macro && "internal error. we should only be here if there is a macro.");
            expr->macro->aotSuffix(ss, expr);
            ss << ")";
            return Visitor::visit(expr);
        }
    // try-catch
        virtual void preVisit ( ExprTryCatch * tc ) override {
            Visitor::preVisit(tc);
            ss << "das_try_recover(__context__, [&]()\n";
            ss << string(tab,'\t');
        }
        virtual void preVisitCatch ( ExprTryCatch * tc, Expression * block ) override {
            Visitor::preVisitCatch(tc, block);
            ss << ", [&]()\n";
            ss << string(tab,'\t');
        }
        virtual ExpressionPtr visit ( ExprTryCatch * tc ) override {
            ss << ")";
            return Visitor::visit(tc);
        }
    // ptr2ref
        virtual void preVisit ( ExprPtr2Ref * ptr2ref ) override {
            Visitor::preVisit(ptr2ref);
            if ( ptr2ref->unsafeDeref ) {
                ss << "(*(";
            } else {
                ss << "das_deref(__context__,";
            }
        }
        virtual ExpressionPtr visit ( ExprPtr2Ref * ptr2ref ) override {
            if ( ptr2ref->unsafeDeref ) {
                ss << "))";
            } else {
                ss << ")";
            }
            return Visitor::visit(ptr2ref);
        }
     // ref2ptr
        virtual void preVisit ( ExprRef2Ptr * ref2ptr ) override {
            Visitor::preVisit(ref2ptr);
            ss << "das_ref(__context__,";
        }
        virtual ExpressionPtr visit ( ExprRef2Ptr * ref2ptr ) override {
            ss << ")";
            return Visitor::visit(ref2ptr);
        }
    // addr
        virtual void preVisit ( ExprAddr * expr ) override {
            if (expr->func) {
                auto mangledName = expr->func->getMangledName();
                uint64_t hash = expr->func->getMangledNameHash();
                ss << "Func(__context__->fnByMangledName(/*" << mangledName << "*/ " << hash << "u))";
            } else {
                ss << "Func(0 /*nullptr*/)";
            }
        }
    // cast
        virtual void preVisit ( ExprCast * expr ) override {
            Visitor::preVisit(expr);
            ss << (expr->upcast ? "das_upcast" : "das_cast")
                << "<" << describeCppType(expr->castType) << ">::cast(";
        }
        virtual ExpressionPtr visit ( ExprCast * expr ) override {
            ss << ")";
            return Visitor::visit(expr);
        }
    // delete
        virtual void preVisit ( ExprDelete * edel ) override {
            Visitor::preVisit(edel);
            const auto & subt = edel->subexpr->type;
            const auto & subft = subt->firstType;
            if ( subt->isPointer() && subft->baseType==Type::tHandle ) {
                ss << "das_delete_handle<";
            } else if ( subt->isPointer() && subft->baseType==Type::tStructure &&
                       subft->structType->persistent ) {
                ss << "das_delete_persistent<";
            } else if ( subt->isPointer() && subft->baseType==Type::tStructure &&
                       subft->structType->isLambda ) {
                ss << "das_delete_lambda_struct<";
            } else {
                ss << "das_delete<";
            }
            ss <<  describeCppType(edel->subexpr->type,CpptSubstitureRef::no,CpptSkipRef::yes,CpptSkipConst::yes);
            ss << ">::clear(__context__,";
        }
        // DELETE
        virtual void preVisitDeleteSizeExpression ( ExprDelete *, Expression * ) override {
            ss << ",";
        }
        virtual ExpressionPtr visit ( ExprDelete * edel ) override {
            ss << ")";
            return Visitor::visit(edel);
        }
    // ascend
        virtual void preVisit ( ExprAscend * expr ) override {
            Visitor::preVisit(expr);
            TypeInfo * info = nullptr;
            if ( expr->needTypeInfo ) {
                info = helper.makeTypeInfo(nullptr, expr->subexpr->type);
            }
            if ( expr->type->firstType->baseType==Type::tHandle ) {
                ss  << "das_ascend_handle<"
                    << (expr->type->smartPtr ? "true" : "false") << ","
                    << describeCppType(expr->type,CpptSubstitureRef::no,CpptSkipRef::yes,CpptSkipConst::yes)
                    << ">::make(__context__,";

            } else {
                ss  << "das_ascend<"
                    << describeCppType(expr->type->firstType,CpptSubstitureRef::no,CpptSkipRef::yes,CpptSkipConst::yes) << ","
                    << describeCppType(expr->subexpr->type,CpptSubstitureRef::no,CpptSkipRef::yes,CpptSkipConst::yes)
                    << ">::make(__context__,";
            }

            if ( info ) {
                ss << "&" << helper.typeInfoName(info) << ",";
            } else {
                ss << "nullptr,";
            }
        }
        virtual ExpressionPtr visit ( ExprAscend * expr ) override {
            ss << ")";
            return Visitor::visit(expr);
        }
    // new
        virtual void preVisit ( ExprNew * enew ) override {
            Visitor::preVisit(enew);
            if ( enew->type->dim.size() ) {
                if ( enew->type->firstType->isHandle() ) {
                    ss << "das_new_dim_handle<"
                       << describeCppType(enew->type->firstType,CpptSubstitureRef::no,CpptSkipRef::yes,CpptSkipConst::yes)
                       << "," << enew->type->dim[0]
                       << "," << (enew->type->smartPtr ? "true" : "false");
                    if ( enew->initializer ) {
                        DAS_ASSERT(0 && "internal error. initializer for enew is not supported");
                    } else {
                        ss << ">::make(__context__";
                    }
                } else {
                    ss << "das_new_dim<"
                       << describeCppType(enew->type->firstType,CpptSubstitureRef::no,CpptSkipRef::yes,CpptSkipConst::yes)
                       << "," << enew->type->dim[0];
                    if ( enew->initializer ) {
                        ss  << ">::make_and_init(__context__,[&]()"
                            // << " -> " << describeCppType(enew->type->firstType,CpptSubstitureRef::no,CpptSkipRef::yes,CpptSkipConst::yes)
                            << " { return ";
                        CallFunc_preVisit(enew);
                    } else {
                        ss << ">::make(__context__";
                    }
                }
            } else {
                if ( enew->type->firstType->isHandle() ) {
                    ss  << "das_new_handle<"
                        << describeCppType(enew->type->firstType,CpptSubstitureRef::no,CpptSkipRef::yes,CpptSkipConst::yes)
                        << ","
                        << (enew->type->smartPtr ? "true" : "false");
                    if ( enew->initializer ) {
                        ss  << ">::make_and_init(__context__,[&]()"
                            // << " -> " << describeCppType(enew->type->firstType,CpptSubstitureRef::no,CpptSkipRef::yes,CpptSkipConst::yes)
                            << " { return new ";
                        CallFunc_preVisit(enew);
                    } else {
                        ss << ">::make(__context__";
                    }
                } else {
                    if ( enew->type->firstType->isStructure() && enew->type->firstType->structType->persistent ) {
                        ss << "das_new_persistent<";
                    } else {
                        ss << "das_new<";
                    }
                    ss << describeCppType(enew->type->firstType,CpptSubstitureRef::no,CpptSkipRef::yes,CpptSkipConst::yes);
                    if ( enew->initializer ) {
                        ss  << ">::make_and_init(__context__,[&]()"
                            // << " -> " << describeCppType(enew->type->firstType,CpptSubstitureRef::no,CpptSkipRef::yes,CpptSkipConst::yes)
                            << " { return ";
                        CallFunc_preVisit(enew);
                    } else {
                        ss << ">::make(__context__";
                    }
                }
            }
        }
        virtual void preVisitNewArg ( ExprNew * enew, Expression * arg, bool last ) override {
            Visitor::preVisitNewArg(enew, arg, last);
            if ( enew->initializer ) {
                CallFunc_preVisitCallArg(enew, arg, last);
            }
        }
        virtual ExpressionPtr visitNewArg ( ExprNew * enew, Expression * arg, bool last ) override {
            if ( enew->initializer ) {
                CallFunc_visitCallArg(enew, arg, last);
            } else {
                DAS_ASSERT(0 && "we should not even be here. we are visiting arguments of a new, but it has no initializer???");
                ss << ",";
            }
            return Visitor::visitNewArg(enew, arg, last);
        }
        virtual ExpressionPtr visit ( ExprNew * enew ) override {
            if ( enew->initializer ) {
                CallFunc_visit(enew);
                ss << "; })";
            } else {
                ss << ")";
            }
            return Visitor::visit(enew);
        }
    // make variant
        bool needTempSrc ( ExprMakeLocal * expr ) const {
            return !expr->doesNotNeedSp && expr->stackTop;
        }
        string mkvName ( ExprMakeVariant * expr ) const {
            if ( needTempSrc(expr) ) {
                return makeLocalTempName(expr);
            } else {
                return "__mkv_" + to_string(expr->at.line);
            }
        }
        virtual void preVisit ( ExprMakeVariant * expr ) override {
            Visitor::preVisit(expr);
            ss << "(([&]() -> " << describeCppType(expr->type,CpptSubstitureRef::no,CpptSkipRef::yes)
                << (needTempSrc(expr) ? "&" : "") << " {\n";
            tab ++;
            if ( !needTempSrc(expr) ) {
                ss << string(tab,'\t') << describeCppType(expr->type,CpptSubstitureRef::no,CpptSkipRef::yes)
                    << " " << mkvName(expr) << ";\n";
            }
            if ( expr->variants.empty() ) {
                ss << string(tab,'\t') << "das_zero(" << mkvName(expr) << ");\n";
            }
        }
        virtual void preVisitMakeVariantField ( ExprMakeVariant * expr, int index, MakeFieldDecl * decl, bool last ) override {
            Visitor::preVisitMakeVariantField(expr,index,decl,last);
            auto variantIndex = expr->type->findArgumentIndex(decl->name);
            DAS_ASSERT(variantIndex != -1 && "should not infer otherwise");
            ss  << string(tab,'\t') << "das_get_variant_field<"
                << describeCppType(expr->type->argTypes[variantIndex])
                << ","
                << expr->type->getVariantFieldOffset(variantIndex)
                << ","
                << variantIndex
                << ">::set(";
            ss << mkvName(expr);
            if ( expr->variants.size()!=1 ) ss << "(" << index << ",__context__)";
            ss <<  ") = ";
        }
        virtual MakeFieldDeclPtr visitMakeVariantField ( ExprMakeVariant * expr, int index, MakeFieldDecl * decl, bool last ) override {
            ss << ";\n";
            return Visitor::visitMakeVariantField(expr,index,decl,last);
        }
        virtual ExpressionPtr visit ( ExprMakeVariant * expr ) override {
            ss << string(tab,'\t') << "return " << mkvName(expr)<< ";\n";
            tab --;
            ss << string(tab,'\t') << "})())";
            return Visitor::visit(expr);
        }
    // make structure
        string mksName ( ExprMakeStruct * expr ) const {
            if ( needTempSrc(expr) ) {
                return makeLocalTempName(expr);
            } else {
                return "__mks_" + to_string(expr->at.line);
            }
        }
        virtual void preVisit ( ExprMakeStruct * expr ) override {
            Visitor::preVisit(expr);
            ss << "(([&](";
            if ( expr->isNewHandle ) {
                ss << describeCppType(expr->type,CpptSubstitureRef::no,CpptSkipRef::yes) << " & " << mksName(expr);
            }
            ss << ")";
            if ( !expr->isNewHandle ) {
                ss << " -> " << describeCppType(expr->type,CpptSubstitureRef::no,CpptSkipRef::yes)
                    << (needTempSrc(expr) ? "&" : "");
            }
            ss << " {\n";
            tab ++;
            if ( !expr->isNewHandle ) {
                if ( !needTempSrc(expr) ) {
                    ss << string(tab,'\t') << describeCppType(expr->type,CpptSubstitureRef::no,CpptSkipRef::yes)
                        << " " << mksName(expr) << ";\n";
                }
                if ( !expr->initAllFields || (expr->makeType->baseType==Type::tTuple && expr->structs.size()==0) ) {
                    ss << string(tab,'\t') << "das_zero(" << mksName(expr) << ");\n";
                }
            }
        }
        virtual void preVisitMakeStructureField ( ExprMakeStruct * expr, int index, MakeFieldDecl * decl, bool last ) override {
            Visitor::preVisitMakeStructureField(expr,index,decl,last);
            ss << string(tab,'\t');
            ss << (decl->moveSemantics ? "das_move((" : "das_copy((");
            if ( expr->makeType->baseType==Type::tHandle ) {
                expr->makeType->annotation->aotPreVisitGetField(ss, decl->name);
            }
            ss << mksName(expr);
            if ( expr->structs.size()!=1 ) ss << "(" << index << ",__context__)";
            if ( expr->makeType->baseType==Type::tHandle ) {
                expr->makeType->annotation->aotVisitGetField(ss, decl->name);
                ss << " /*" << decl->name << "*/";
            } else {
                ss << "." << decl->name;
            }
            ss << "),(";
        }
        virtual MakeFieldDeclPtr visitMakeStructureField ( ExprMakeStruct * expr, int index, MakeFieldDecl * decl, bool last ) override {
            ss << "));\n";
            return Visitor::visitMakeStructureField(expr,index,decl,last);
        }
        virtual bool canVisitMakeStructureBlock ( ExprMakeStruct *, Expression * ) override { return false; }
        virtual ExpressionPtr visit ( ExprMakeStruct * expr ) override {
            if ( expr->block ) {
                DAS_ASSERT(expr->block->rtti_isMakeBlock());
                auto mkb = static_pointer_cast<ExprMakeBlock>(expr->block);
                DAS_ASSERT(mkb->block->rtti_isBlock());
                auto blk = static_pointer_cast<ExprBlock>(mkb->block);
                collector.renameVariable(blk->arguments[0].get(), mksName(expr));
                ss << string(tab,'\t');
                blk->visit(*this);
            }
            if ( !expr->isNewHandle ) {
                ss << string(tab,'\t') << "return " << mksName(expr) << ";\n";
            }
            tab --;
            ss << string(tab,'\t') << "})";
            if ( !expr->isNewHandle ) ss << "()" ;
            ss << ")";
            return Visitor::visit(expr);
        }
    // make array
        string mkaName ( ExprMakeArray * expr ) const {
            if ( !needTempSrc(expr) ) {
                return "__mka_" + to_string(expr->at.line);
            } else {
                return makeLocalTempName(expr);
            }
        }
        virtual void preVisit ( ExprMakeArray * expr ) override {
            Visitor::preVisit(expr);
            ss << "(([&]() -> " << describeCppType(expr->type,CpptSubstitureRef::no,CpptSkipRef::yes)
                << (needTempSrc(expr) ? "&" : "") << " {\n";
            tab ++;
            if ( !needTempSrc(expr) ) {
                ss << string(tab,'\t') << describeCppType(expr->type,CpptSubstitureRef::no,CpptSkipRef::yes)
                    << " " << mkaName(expr) << ";\n";
            }
            if ( !expr->initAllFields ) {
                ss << string(tab,'\t') << "das_zero(" << mkaName(expr) << ");\n";
            }
        }
        virtual void preVisitMakeArrayIndex ( ExprMakeArray * expr, int index, Expression * init, bool lastField ) override {
            Visitor::preVisitMakeArrayIndex(expr, index, init, lastField);
            ss << string(tab,'\t') << mkaName(expr) << "(" << index << ",__context__) = ";
        }
        virtual ExpressionPtr visitMakeArrayIndex ( ExprMakeArray * expr, int index, Expression * init, bool lastField ) override {
            ss << ";\n";
            return Visitor::visitMakeArrayIndex(expr, index, init, lastField);
        }
        virtual ExpressionPtr visit ( ExprMakeArray * expr ) override {
            ss << string(tab,'\t') << "return " << mkaName(expr)<< ";\n";
            tab --;
            ss << string(tab,'\t') << "})())";
            return Visitor::visit(expr);
        }
   // make tuple
        string mktName ( ExprMakeTuple * expr ) const {
            if ( !needTempSrc(expr) ) {
                return "__mkt_" + to_string(expr->at.line);
            } else {
                return makeLocalTempName(expr);
            }
        }
        virtual void preVisit ( ExprMakeTuple * expr ) override {
            Visitor::preVisit(expr);
            ss << "(([&]() -> " << describeCppType(expr->type,CpptSubstitureRef::no,CpptSkipRef::yes)
                << (needTempSrc(expr) ? "&" : "") << " {\n";
            tab ++;
            if ( !needTempSrc(expr) ) {
                ss << string(tab,'\t') << describeCppType(expr->type,CpptSubstitureRef::no,CpptSkipRef::yes)
                    << " " << mktName(expr) << ";\n";
            }
            if ( !expr->initAllFields ) {
                ss << string(tab,'\t') << "das_zero(" << mktName(expr) << ");\n";
            }
        }
        virtual void preVisitMakeTupleIndex ( ExprMakeTuple * expr, int index, Expression * init, bool lastField ) override {
            Visitor::preVisitMakeTupleIndex(expr, index, init, lastField);
            ss << string(tab,'\t') << "das_get_tuple_field<"
                << describeCppType(expr->makeType->argTypes[index])
                << ","
                << expr->makeType->getTupleFieldOffset(index)
                << ">::get(" << mktName(expr) << ") = ";
        }
        virtual ExpressionPtr visitMakeTupleIndex ( ExprMakeTuple * expr, int index, Expression * init, bool lastField ) override {
            ss << ";\n";
            return Visitor::visitMakeTupleIndex(expr, index, init, lastField);
        }
        virtual ExpressionPtr visit ( ExprMakeTuple * expr ) override {
            ss << string(tab,'\t') << "return " << mktName(expr)<< ";\n";
            tab --;
            ss << string(tab,'\t') << "})())";
            return Visitor::visit(expr);
        }
    // ExprMakeBlock
        virtual bool canVisitMakeBlockBody ( ExprMakeBlock * blk ) override {
            return blk->aotFunctorName.empty();
        }
        virtual void preVisit ( ExprMakeBlock * expr ) override {
            Visitor::preVisit(expr);
            auto block = static_pointer_cast<ExprBlock>(expr->block);
            if ( !block->aotSkipMakeBlock ) {
                ss << "das_make_block";
                if ( block->returnType->isRefType() && !block->returnType->ref ) {
                    ss << "_cmres";
                }
                ss << "<" << describeCppType(block->returnType);
                for ( auto & arg : block->arguments ) {
                    ss << "," << describeCppType(arg->type);
                    if ( arg->type->isRefType() && !arg->type->ref ) {
                        ss << " &";
                    }
                }
                ss << ">(__context__," << block->stackTop << ",";
            }
            if ( !block->aotSkipMakeBlock || block->aotDoNotSkipAnnotationData ) {
                if ( block->annotationDataSid != 0 ) {
                    ss << "__context__->adBySid(" << block->annotationDataSid << "u)";
                } else {
                    ss << "0";
                }
                ss << ",";
            }
            if ( !block->aotSkipMakeBlock ) {
                auto info = helper.makeInvokeableTypeDebugInfo(block->makeBlockType(), block->at);
                ss << "&" << helper.funcInfoName(info) << ",";
            }
            if ( expr->aotFunctorName.empty() ) {
                ss << "[&](";
                int ai = 0;
                for ( auto & arg : block->arguments ) {
                    if (ai++) ss << ", ";
                    if (isLocalVec(arg->type)) {
                        describeLocalCppType(ss, arg->type);
                    } else {
                        ss << describeCppType(arg->type,CpptSubstitureRef::no,CpptSkipRef::no,CpptSkipConst::no,CpptRedundantConst::no);
                        if ( arg->type->isRefType() && !arg->type->ref ) {
                            ss << " &";
                        }
                    }
                    ss << " " << collector.getVarName(arg);
                }
                ss << ") ";
                if ( block->aotSkipMakeBlock ) {
                    ss << "DAS_AOT_INLINE_LAMBDA ";
                }
                ss << "-> " << describeCppType(block->returnType);
            } else {
                ss << expr->aotFunctorName;
            }
        }
        virtual ExpressionPtr visit ( ExprMakeBlock * expr ) override {
            auto block = static_pointer_cast<ExprBlock>(expr->block);
            if ( !block->aotSkipMakeBlock ) {
                ss << ")";
            }
            return Visitor::visit(expr);
        }

    // looks like call
        virtual void preVisit ( ExprLooksLikeCall * call ) override {
            Visitor::preVisit(call);
            if (call->name == "debug" ) {
                auto argType = call->arguments[0]->type;
                TypeInfo * info = helper.makeTypeInfo(nullptr, argType);
                ss << "das_debug(__context__,&" << helper.typeInfoName(info) << ",__FILE__,__LINE__,";
                ss << "cast<"
                   << describeCppType(argType)
                   << ((argType->isRefType() && !argType->ref) ? "&" : "")
                   << ">::from(";
            } else if (call->name == "assert" || call->name=="verify") {
                auto ea = static_cast<ExprAssert *>(call);
                if ( call->arguments.size()==1 ) {
                    ss << (ea->isVerify ? "DAS_VERIFY" : "DAS_ASSERT") << "((";
                } else {
                    ss << (ea->isVerify ? "DAS_VERIFYF" : "DAS_ASSERTF") << "((";
                }
            } else if (call->name == "erase") {
                ss << "__builtin_table_erase(__context__,";
            } else if (call->name == "insert") {
                ss << "__builtin_table_set_insert(__context__,";
            } else if (call->name == "find") {
                ss << "__builtin_table_find(__context__,";
            } else if (call->name == "key_exists") {
                ss << "__builtin_table_key_exists(__context__,";
            } else if (call->name == "keys") {
                ss << "__builtin_table_keys(__context__,";
            } else if (call->name == "values") {
                ss << "__builtin_table_values(__context__,";
            } else if ( call->name=="invoke" ) {
                auto bt = call->arguments[0]->type->baseType;
                if (bt == Type::tBlock) ss << "das_invoke";
                else if (bt == Type::tLambda) ss << "das_invoke_lambda";
                else if (bt == Type::tFunction) ss << "das_invoke_function";
                else if (bt == Type::tString) ss << "das_invoke_function_by_name";
                else ss << "das_invoke /*unknown*/";
                ExprInvoke * einv = static_cast<ExprInvoke *>(call);
                ss << "<" << describeCppType(call->type) << ">::invoke";
                if ( einv->isCopyOrMove() ) ss << "_cmres";
                if ( call->arguments.size()>1 ) {
                    ss << "<";
                    for ( const auto & arg : call->arguments ) {
                        if ( arg!=call->arguments.front() ) {
                            ss << describeCppType(arg->type);
                            if ( arg->type->isRefType() && !arg->type->ref ) {
                                ss << " &";
                            }
                            if ( arg!=call->arguments.back() ) {
                                ss << ",";
                            }
                        }
                    }
                    ss << ">";
                }
                ss << "(__context__,nullptr,";
            } else if ( call->name=="memzero" ) {
                ss << "memset(&(";
            } else if ( call->name=="static_assert" ) {
                ss << "das_static_assert(";
            } else {
                ss << call->name << "(";
            }
        }
        virtual void preVisitLooksLikeCallArg ( ExprLooksLikeCall * call, Expression * arg, bool last ) override {
            Visitor::preVisitLooksLikeCallArg(call, arg, last);
            if ( call->name=="invoke" ) {
                auto argType = arg->type;
                if ( arg->type->isRefType() ) {
                    if ( needsArgPass(arg) ) {
                        ss << "das_arg<" << describeCppType(argType,CpptSubstitureRef::no,CpptSkipRef::yes) << ">::pass(";
                    }
                }
            }
        }
        virtual ExpressionPtr visitLooksLikeCallArg ( ExprLooksLikeCall * call, Expression * arg, bool last ) override {
            if ( call->name=="invoke" ) {
                auto argType = arg->type;
                if ( arg->type->isRefType() ) {
                    if ( needsArgPass(arg) ) {
                        ss << ")";
                    }
                }
            }
            if ( !last ) {
                if (call->name == "assert" || call->name=="verify" || call->name=="debug") {
                    ss << "),(";
                } else {
                    ss << ",";
                }
            }
            return Visitor::visitLooksLikeCallArg(call, arg, last);
        }
        virtual ExpressionPtr visit ( ExprLooksLikeCall * call ) override {
            if ( call->name=="assert" || call->name=="verify" || call->name=="debug" ) {
                ss << "))";
            } else if ( call->name=="memzero" ) {
                ss << "), 0, " << call->arguments[0]->type->getSizeOf() << ")";
            } else {
                ss << ")";
            }
            return Visitor::visit(call);
        }
    // call
        bool isPolicyBasedCall ( ExprCall * call ) const {
            auto bif = static_cast<BuiltInFunction *>(call->func);
            if ( !call->arguments.size() && call->func->result->baseType==Type::tHandle ) {
                // c-tor?
                return false;
            } else if ( bif->policyBased ) {
                return true;
            } else {
                return false;
            }
        }
        bool policyArgNeedCast ( const TypeDeclPtr & polType, const TypeDeclPtr & argType ) {
            if ( argType->isVectorType() ) {
                return false;
            }
            if ( !polType->isHandle() ) {
                if ( polType->isVecPolicyType() && argType->isVecPolicyType() ) {
                    return false;
                }
            }
            if ( !polType->isPolicyType() ) {
                return false;
            }
            return true;
        }
        bool policyResultNeedCast ( const TypeDeclPtr & polType, const TypeDeclPtr & resType ) {
            if ( resType->isVoid() ) {
                return false;
            }
            if ( !resType->isPolicyType() ) {
                return false;
            }
            return policyArgNeedCast(polType, resType);
        }
        bool isPolicyBasedCall ( ExprCallFunc * call ) {
            if ( call->func->builtIn ) {
                auto bif = static_cast<BuiltInFunction *>(call->func);
                if ( bif->policyBased ) {
                    return true;
                }
            }
            return false;
        }
        bool isHybridCall ( Function * func ) {
            if ( func->builtIn ) {
                auto bif = (BuiltInFunction *) func;
                DAS_ASSERTF(!func->policyBased, "we should not be here. policy based calls are handled elsewhere");
                DAS_ASSERTF(!func->callBased, "we should not be here. call-based calls handled elsewhere");
                if ( bif->cppName.empty() ) {
                    return true;
                }
                return false;
            }
            if ( func->noAot ) return true;
            if ( func->aotHybrid ) return true;
            if ( func->module == program->thisModule.get() ) return false;
            return true;
        }
        bool needsArgPass ( const TypeDeclPtr & argType ) const {
            return !argType->constant && !argType->isGoodBlockType();
        }
        bool needsArgPass ( Expression * expr ) const {
            if ( expr->rtti_isMakeBlock() ) {
                auto mkblock = static_cast<ExprMakeBlock *>(expr);
                auto block = static_pointer_cast<ExprBlock>(mkblock->block);
                if ( block->aotSkipMakeBlock ) {
                    return false;
                }
            }
            return needsArgPass(expr->type);
        }
        bool isCallWithTemp ( ExprCallFunc * call ) {
            if ( call->rtti_isCall() ) {
                auto expr = (ExprCall *) call;
                return !expr->doesNotNeedSp && expr->stackTop;
            }
            return false;
        }
        void CallFunc_preVisit ( ExprCallFunc * call ) {
            Visitor::preVisit(call);
            if ( call->func->propertyFunction ) {   // property function goes ((arg0).name()). we do `((` here
                if ( call->func->result->aotAlias ) {
                    ss << "das_alias<" << call->func->result->alias << ">::from(";
                }
                if ( call->func->result->isString() ) {
                    ss << "((" << describeCppType(call->func->result) << ")(";  // c-cast const char * etc string casts to char * or char * const
                }
                ss << "((";
                return;
            }
            string aotName = call->func->getAotName(call);
            for ( auto & ann : call->func->annotations ) {
                if ( ann->annotation->rtti_isFunctionAnnotation() ) {
                    auto pAnn = static_pointer_cast<FunctionAnnotation>(ann->annotation);
                    if ( aotPrefix.find(aotName)==aotPrefix.end() ) {
                        pAnn->aotPrefix(stg, call);
                        aotPrefix.insert(aotName);
                    }
                }
            }
            if ( isCallWithTemp(call) ) {
                ss << "(" << makeLocalTempName(call) << " = (";
            }
            if ( call->func->result->aotAlias ) {
                ss << "das_alias<" << call->func->result->alias << ">::from(";
            }
            if ( call->func->builtIn ) {
                if ( call->func->result->isString() ) {
                    ss << "((" << describeCppType(call->func->result) << ")(";  // c-cast const char * etc string casts to char * or char * const
                }
                auto bif = static_cast<BuiltInFunction *>(call->func);
                if ( !call->arguments.size() && call->func->result->baseType==Type::tHandle ) {
                    // c-tor?
                    ss << "/*c-tor*/ ";
                } else if ( bif->policyBased ) {
                    outPolicy(call->arguments[0]->type);
                    ss << "::";
                }
                if ( bif->interopFn ) {
                    ss << "das_call_interop<";
                    ss << describeCppType(call->func->result);
                    ss << ">::call(&";
                }
                ss << aotName;
                if ( bif->interopFn ) {
                    uint32_t nArgs = (uint32_t) call->arguments.size();
                    ss << ",__context__,SimNode_AotInterop<" << nArgs << ">(";
                    if ( nArgs ) {
                        auto debug_info_name = outputCallTypeInfo(nArgs, call->arguments);
                        ss << debug_info_name << ",";
                    }
                } else {
                    ss << "(";
                }
            } else {
                if ( isHybridCall(call->func) ) {
                    ss << "das_invoke_function<" << describeCppType(call->func->result) << ">::invoke";
                    if ( call->func->result->isRefType() && !call->func->result->ref ) {
                        ss << "_cmres";
                    }
                    auto mangledName = call->func->getMangledName();
                    uint64_t hash = call->func->getMangledNameHash();
                    if ( call->arguments.size()>=1 ) {
                        ss << "<";
                        for ( const auto & arg : call->func->arguments ) {
                            ss << describeCppType(arg->type);
                            if ( arg->type->isRefType() && !arg->type->ref ) {
                                ss << " &";
                            }
                            if ( arg!=call->func->arguments.back() ) {
                                ss << ",";
                            }
                        }
                        ss << ">(__context__,nullptr,";
                        ss << "Func(__context__->fnByMangledName(/*" << mangledName << "*/ " << hash << "u)),";
                    } else {
                        ss << "(__context__,nullptr,";
                        ss << "Func(__context__->fnByMangledName(/*" << mangledName << "*/ " << hash << "u))";
                    }
                } else {
                    ss << aotFuncName(call->func) << "(__context__";
                    if  ( call->arguments.size() ) ss << ",";
                }
            }
        }
        bool needSubstitute ( const TypeDeclPtr & argType, const TypeDeclPtr & passType ) const {
            if (argType->baseType == Type::anyArgument) return false;
            return !argType->isSameType(*passType,RefMatters::no,ConstMatters::no,TemporaryMatters::no,AllowSubstitute::no);
        }
        bool needPtrCast ( const TypeDeclPtr & argType, const TypeDeclPtr & passType, Expression * passExpr ) const {
            if ( passExpr->rtti_isNullPtr() ) return true;
            return argType->isVoidPointer() ^ passType->isVoidPointer();
        }
        bool needStringCast ( const FunctionPtr & func, const TypeDeclPtr & arg ) {
            return func->needStringCast && arg->isString() && !arg->ref;
        }
        void CallFunc_preVisitCallArg ( ExprCallFunc * call, Expression * arg, bool ) {
            if ( call->func->propertyFunction ) return; // property function goes ((arg0).name()). we do nothing here
            int argIndex = 0;
            auto it = call->arguments.begin();
            auto its = call->arguments.end();
            for ( ; it!=its; ++it, ++argIndex ) {
                if ( it->get()==arg ) {
                    break;
                }
            }
            DAS_ASSERT(it != call->arguments.end());
            ss << call->func->getAotArgumentPrefix(call, argIndex);
            auto argType = (*it)->type;
            auto funArgType = call->func->arguments[it-call->arguments.begin()]->type;
            if ( funArgType->isAotAlias() ) {
                if ( funArgType->alias.empty() ) {
                    ss << "das_reinterpret<"
                    << describeCppTypeEx(funArgType,CpptSubstitureRef::no,CpptSkipRef::yes,CpptSkipConst::no,CpptRedundantConst::yes,CpptUseAlias::yes) << ">::pass(";
                } else {
                    ss << "das_alias<" << funArgType->alias << ">::to(";
                }
            }
            if ( !call->func->noPointerCast && needPtrCast(funArgType,arg->type,arg) ) {
                ss << "das_auto_cast<" << describeCppType(funArgType,CpptSubstitureRef::no,CpptSkipRef::no) << ">::cast(";
            }
            if ( !call->func->anyTemplate && (call->func->interopFn || funArgType->baseType==Type::anyArgument) ) {
                ss << "cast<" << describeCppType(argType);
                if ( argType->isRefType() && !argType->ref ) {
                    ss << " &";
                }
                ss << ">::from(";
            }
            if ( needSubstitute(funArgType,arg->type) ) {
                ss << "das_reinterpret<" << describeCppType(funArgType,CpptSubstitureRef::no,CpptSkipRef::yes) << ">::pass(";
            }
            if ( !call->func->interopFn && arg->type->isRefType() ) {
                if ( needsArgPass(arg) ) {
                    ss << "das_arg<" << describeCppType(argType,CpptSubstitureRef::no,CpptSkipRef::yes) << ">::pass(";
                }
            }
            if ( isPolicyBasedCall(call) && policyArgNeedCast(call->func->result, argType) ) {
                ss << "cast<" << describeCppType(argType,CpptSubstitureRef::no,CpptSkipRef::yes,CpptSkipConst::yes) << ">::from(";
            }
            if ( needStringCast(call->func,argType) ) {
                ss << "(das_string_cast(";
            }
        }
        void CallFunc_visitCallArg ( ExprCallFunc * call, Expression * arg, bool last ) {
            if ( call->func->propertyFunction ) return; // property function goes ((arg0).name()). we do nothing here
            int argIndex = 0;
            auto it = call->arguments.begin();
            auto its = call->arguments.end();
            for ( ; it!=its; ++it, ++argIndex ) {
                if ( it->get()==arg ) {
                    break;
                }
            }
            DAS_ASSERT(it != call->arguments.end());
            auto argType = (*it)->type;
            if ( needStringCast(call->func,argType) ) {
                ss << "))";
            }
            if ( isPolicyBasedCall(call) && policyArgNeedCast(call->func->result, argType) ) {
                ss << ")";
            }
            auto funArgType = call->func->arguments[it-call->arguments.begin()]->type;
            if ( !call->func->anyTemplate && (call->func->interopFn || funArgType->baseType==Type::anyArgument) ) {
                ss << ")";
            }
            if ( needSubstitute(funArgType,arg->type) ) ss << ")";
            if ( !call->func->interopFn && arg->type->isRefType() ) {
                if ( needsArgPass(arg) ) {
                    ss << ")";
                }
            }
            if ( !call->func->noPointerCast && needPtrCast(funArgType,arg->type,arg) ) ss << ")";
            if ( funArgType->isAotAlias() ) ss << ")";
            ss << call->func->getAotArgumentSuffix(call, argIndex);
            if ( !last ) ss << ",";
        }
        void CallFunc_visit ( ExprCallFunc * call ) {
            if ( call->func->propertyFunction ) {   // property function goes ((arg0).name()). we do `).name())` here
                if ( call->func->builtIn ) {
                    auto efn = static_cast<ExternalFnBase *>(call->func);
                    ss << ")." << efn->cppName << "())";    // we skip .` part of the deal
                } else {
                    DAS_ASSERT(call->func->name[0]=='.' && call->func->name[1]=='`');
                    ss << ")." << (call->func->name.c_str()+2) << "())";    // we skip .` part of the deal
                }
                if ( call->func->result->isString() ) {
                    ss << "))";  // c-cast const char * etc string casts to char * or char * const
                }
                if ( call->func->result->aotAlias ) {
                    ss << ")";
                }
                return;
            }
            if ( call->func->interopFn ) {
                ss << ")";
            }
            if ( !call->arguments.size() && call->func->result->baseType==Type::tHandle ) {
                // c-tor?
                ss << "/*end-c-tor*/";
            } else if ( isPolicyBasedCall(call) ) {
                ss << ",*__context__,nullptr";
            }
            ss << ")";
            if ( call->func->builtIn && call->func->result->isString() ) {
                ss << "))";
            }
            if ( call->func->result->aotAlias ) {
                ss << ")";
            }
            if ( isCallWithTemp(call) ) {
                ss << "))";
            }
        }
        virtual void preVisit ( ExprCall * call ) override {
            Visitor::preVisit(call);
            CallFunc_preVisit(call);
        }
        virtual void preVisitCallArg ( ExprCall * call, Expression * arg, bool last ) override {
            Visitor::preVisitCallArg(call, arg, last);
            CallFunc_preVisitCallArg(call, arg, last);
        }
        virtual ExpressionPtr visitCallArg ( ExprCall * call, Expression * arg, bool last ) override {
            CallFunc_visitCallArg(call, arg, last);
            return Visitor::visitCallArg(call, arg, last);
        }
        virtual ExpressionPtr visit ( ExprCall * call ) override {
            CallFunc_visit(call);
            return Visitor::visit(call);
        }
    // for
        string forSrcName ( const string & varName ) const {
            return "__" + varName + "_iterator";
        }
        string needLoopName ( ExprFor * ffor ) const {
            return "__need_loop_" + to_string(ffor->at.line);
        }
        virtual void preVisit ( ExprFor * ffor ) override {
            Visitor::preVisit(ffor);
            ss << "{\n";
            tab ++;
            auto nl = needLoopName(ffor);
            ss << string(tab,'\t') << "bool " << nl << " = true;\n";
        }
        virtual void preVisitForBody ( ExprFor * ffor, Expression * body ) override {
            Visitor::preVisitForBody(ffor, body);
            auto nl = needLoopName(ffor);
            if ( body->rtti_isBlock() ) {
                auto * block = static_cast<ExprBlock *>(body);
                if ( !block->finalList.empty() ) {
                    block->visitFinally(*this);
                }
            }
            ss << string(tab,'\t') << "for ( ; " << nl << " ; " << nl << " = ";
            for ( auto & var : ffor->iteratorVariables ) {
                if (var != ffor->iteratorVariables.front()) {
                    ss << " && ";
                }
                ss << forSrcName(var->name) << ".next(__context__,";
                ss << "(" << collector.getVarName(var) << "))";
            }
            ss << " )\n";
            ss << string(tab,'\t');
        }
        bool isCountOrUCount ( Expression * expr ) const {
            if ( !expr->rtti_isCallFunc() ) return false;
            auto call = static_cast<ExprCallFunc *>(expr);
            return  call->func && call->func->builtIn && call->func->module->name=="$" && (call->name=="count" || call->name=="ucount");
        }
        virtual void preVisitForSource ( ExprFor * ffor, Expression * that, bool last ) override {
            Visitor::preVisitForSource(ffor, that, last);
            size_t idx;
            auto idxs = ffor->sources.size();
            for ( idx=0; idx!=idxs; ++idx ) {
                if ( ffor->sources[idx].get()==that ) {
                    break;
                }
            }
            auto & src = ffor->sources[idx];
            auto & var = ffor->iteratorVariables[idx];
            ss << string(tab,'\t') << "// " << var->name << " : " << var->type->describe() << "\n";
            if ( isCountOrUCount(src.get()) ) {
                ss << string(tab,'\t') << "das_iterator_" << ((ExprCallFunc *) src.get())->func->name << " DAS_COMMENT(";
            } else {
                ss << string(tab,'\t') << "das_iterator<"
                    << describeCppType(src->type,CpptSubstitureRef::yes,CpptSkipRef::yes,CpptSkipConst::no)
                        << "> " << forSrcName(var->name) << "(";
            }
        }
        virtual ExpressionPtr visitForSource ( ExprFor * ffor, Expression * that , bool last ) override {
            size_t idx;
            auto idxs = ffor->sources.size();
            for ( idx=0; idx!=idxs; ++idx ) {
                if ( ffor->sources[idx].get()==that ) {
                    break;
                }
            }
            auto & src = ffor->sources[idx];
            auto & var = ffor->iteratorVariables[idx];
            if ( isCountOrUCount(src.get()) ) {
                auto pCall = ((ExprCall *) src.get());
                ss << ") " << forSrcName(var->name) << "(";
                pCall->arguments[0]->visit(*this);
                ss << ",";
                pCall->arguments[1]->visit(*this);
                ss << ");\n";
            } else {
                ss << ");\n";
            }
            // source
            bool skipTC = var->type->isString() && !var->type->ref;
            ss << string(tab,'\t') << describeCppType(var->type,CpptSubstitureRef::yes,CpptSkipRef::no,
                                                      skipTC ? CpptSkipConst::yes : CpptSkipConst::no)
                << " " << collector.getVarName(var) << ";\n";
            // loop
            auto nl = needLoopName(ffor);
            ss << string(tab, '\t') << nl << " = " << forSrcName(var->name)
                << ".first(__context__,";
            ss << "(" << collector.getVarName(var) << ")";
            ss << ") && " << nl << ";\n";
            return Visitor::visitForSource(ffor, that, last);
        }
        virtual ExpressionPtr visit ( ExprFor * ffor ) override {
            ss << "\n";
            for ( auto & var : ffor->iteratorVariables ) {
                ss << string(tab, '\t') << forSrcName(var->name) << ".close(__context__,";
                ss << "(" << collector.getVarName(var) << "));\n";
            }
            tab --;
            ss << string(tab,'\t') << "}";
            return Visitor::visit(ffor);
        }
    };

    uint64_t Context::getInitSemanticHash() {
        const uint64_t fnv_prime = 1099511628211ul;
        uint64_t hash = globalsSize ^ sharedSize;
        for ( int i=0; i!=totalVariables; ++i ) {
            hash = (hash ^ (globalVariables[i].shared ? 13 : 17)) * fnv_prime;
            hash = (hash ^ globalVariables[i].mangledNameHash) * fnv_prime;
            hash = (hash ^ globalVariables[i].size) * fnv_prime;
            if ( globalVariables[i].init ) {
                hash = (hash ^ getSemanticHash(globalVariables[i].init,this)) * fnv_prime;
            }
        }
        return hash;
    }

    void Program::writeStandaloneContextMethods ( TextWriter & logs ) {
        vector<Function *> fnn = collectUsedFunctions(library.modules, totalFunctions);
        BlockVariableCollector collector;

        for ( auto fn : fnn ) {
            if ( !fn->exports ) continue;
            if ( fn->module != thisModule.get() ) continue;
            logs << "    auto " << aotFunctionName(fn->getOrigin() ? fn->getOrigin()->name : fn->name) << " ( ";
        // describe arguments
            for ( auto & var : fn->arguments ) {
                if (isLocalVec(var->type)) {
                    describeLocalCppType(logs, var->type);
                } else {
                    logs << describeCppType(var->type);
                }
                logs << " ";
                if ( var->type->isRefType() ) {
                    logs << "& ";
                }
                logs << collector.getVarName(var);
                bool last = &var == &fn->arguments.back();
                if ( ! last ) logs << ", ";
            }
            logs << " ) -> ";
            describeLocalCppType(logs,fn->result,CpptSubstitureRef::no);
            logs << " {\n";
            logs << "        return " << aotFuncName(fn) << "(this";
            for ( auto & var : fn->arguments ) {
                logs << ", " << collector.getVarName(var);
            }
            logs << "); \n";
            logs << "    }\n\n";
        }
    }

    void Program::registerAotCpp ( TextWriter & logs, Context & context, bool headers, bool allModules ) {
        vector<Function *> fnn = collectUsedFunctions(library.modules, totalFunctions);
        if ( headers ) {
            logs << "\nvoid registerAot ( AotLibrary & aotLib )\n{\n";
        }
        bool funInit = false;
        for ( int i=0, is=context.totalFunctions; i!=is; ++i ) {
            if ( !allModules && fnn[i]->module != thisModule.get() )
                continue;
            if ( fnn[i]->init ) {
                funInit = true;
            }
            if ( fnn[i]->noAot )
                continue;
            // SimFunction * fn = context.getFunction(i);
            uint64_t semH = fnn[i]->aotHash;
            logs << "\t// " << aotFuncName(fnn[i]) << "\n";
            logs << "\taotLib[0x" << HEX << semH << DEC << "] = [&](Context & ctx){\n\t\treturn ";
            logs << "ctx.code->makeNode<SimNode_Aot";
            if ( fnn[i]->copyOnReturn || fnn[i]->moveOnReturn ) {
                logs << "CMRES";
            }
            logs << "<" << describeCppFunc(fnn[i],nullptr,false,false) << ",";
            logs << "&" << aotFuncName(fnn[i]) << ">>();\n\t};\n";
        }
        if ( context.totalVariables || funInit ) {
            uint64_t semH = context.getInitSemanticHash();
            semH = getInitSemanticHashWithDep(semH);
            logs << "\t// [[ init script ]]\n";
            logs << "\taotLib[0x" << HEX << semH << DEC << "] = [&](Context & ctx){\n";
            logs << "\t\tctx.aotInitScript = ctx.code->makeNode<SimNode_Aot<void (*)(Context *, bool),&__init_script>>();\n";
            logs << "\t\treturn ctx.aotInitScript;\n";
            logs << "\t};\n";
        }
        if ( headers ) {
            logs << "}\n";
        }
    }

    void Program::writeStandaloneContext ( TextWriter & logs ) {

        logs << "\n\n";
        logs << "class StandaloneContext : public Context {\n";
        logs << "public: \n";
        writeStandaloneContextMethods(logs);
        logs << "    StandaloneContext() {\n";

        auto disableInit = options.getBoolOption("no_init", policies.no_init);
        logs << "    auto & context = *this;\n";
        logs << "    context.breakOnException |= " << policies.debugger << " /*policies.debugger*/;\n";
        logs << "    context.persistent = " << options.getBoolOption("persistent_heap", policies.persistent_heap) << " /*options.getBoolOption(\"persistent_heap\", policies.persistent_heap)*/;\n";
        logs << "    if ( context.persistent ) {\n";
        logs << "        context.heap = make_smart<PersistentHeapAllocator>();\n";
        logs << "        context.stringHeap = make_smart<PersistentStringAllocator>();\n";
        logs << "    } else {\n";
        logs << "        context.heap = make_smart<LinearHeapAllocator>();\n";
        logs << "        context.stringHeap = make_smart<LinearStringAllocator>();\n";
        logs << "    }\n";
        logs << "    context.heap->setInitialSize ( " << options.getIntOption("heap_size_hint", policies.heap_size_hint) << " /*options.getIntOption(\"heap_size_hint\", policies.heap_size_hint)*/);\n";
        logs << "    context.stringHeap->setInitialSize ( " << options.getIntOption("string_heap_size_hint", policies.string_heap_size_hint) << " /*options.getIntOption(\"string_heap_size_hint\", policies.string_heap_size_hint)*/);\n";
        logs << "    context.constStringHeap = make_shared<ConstStringAllocator>();\n";
        logs << "    if ( " << globalStringHeapSize << " /*globalStringHeapSize*/) {\n";
        logs << "        context.constStringHeap->setInitialSize(" << globalStringHeapSize << "/*globalStringHeapSize*/);\n";
        logs << "    }\n";

        // logs << "DebugInfoHelper helper(context.debugInfo);\n";
        // logs << "helper.rtti = " << options.getBoolOption("rtti",policies.rtti) << ";\n";
        // logs << "context.thisHelper = &helper;\n";
        logs << "    context.globalVariables = (GlobalVariable *) context.code->allocate( " << totalVariables << "/*totalVariables*/*sizeof(GlobalVariable) );\n";
        logs << "    context.globalsSize = 0;\n";
        logs << "    context.sharedSize = 0;\n";

        if ( totalVariables ) {
            for (auto & pm : library.modules ) {
                pm->globals.foreach([&](auto pvar){
                    if (!pvar->used)
                        return;
                    if ( pvar->index<0 ) {
                        error("Internal compiler errors. Simulating variable which is not used" + pvar->name,
                            "", "", LineInfo());
                        return;
                    }
                    logs << "     // totalVariables  "  << "\n";
                    logs << "    {\n";
                    logs << "        auto & gvar = context.globalVariables[" << pvar->index << "/*pvar->index*/];\n";
                    logs << "        gvar.name = context.code->allocateName(\"" << pvar->name << "\"/*pvar->name*/);\n";
                    logs << "        gvar.size = " << pvar->type->getSizeOf() << "/*pvar->type->getSizeOf()*/;\n";
                    // logs << "        gvar.debugInfo = helper.makeVariableDebugInfo(*pvar);\n";
                    logs << "        gvar.flags = 0;\n";
                    logs << "        if ( " << pvar->global_shared << " /*pvar->global_shared*/) {\n";
                    logs << "            gvar.offset = context.sharedSize;\n";
                    logs << "            gvar.shared = true;\n";
                    logs << "            context.sharedSize = (context.sharedSize + gvar.size + 0xf) & ~0xf;\n";
                    logs << "        } else {\n";
                    logs << "            gvar.offset = context.globalsSize;\n";
                    logs << "            context.globalsSize = (context.globalsSize + gvar.size + 0xf) & ~0xf;\n";
                    logs << "        }\n";
                    logs << "        gvar.mangledNameHash = 0x" << HEX << pvar->getMangledNameHash() << DEC  << "/*pvar->getMangledNameHash()*/;\n";
                    logs << "        gvar.init = nullptr;\n";
                    logs << "    }\n";
                });
            }
        }
        logs << "    context.globals = (char *) das_aligned_alloc16(context.globalsSize);\n";
        logs << "    context.shared = (char *) das_aligned_alloc16(context.sharedSize);\n";
        logs << "    context.sharedOwner = true;\n";
        logs << "    context.totalVariables = " << totalVariables << "/*totalVariables*/;\n";
        logs << "    context.functions = (SimFunction *) context.code->allocate( " << totalFunctions << "/*totalFunctions*/*sizeof(SimFunction) );\n";
        logs << "    context.totalFunctions = " << totalFunctions << "/*totalFunctions*/;\n";
        logs << "    auto debuggerOrGC = "  << getDebugger()                      << "/*getDebugger()*/ || "
                                            << options.getBoolOption("gc", false) << "/*options.getBoolOption(\"gc\", false)*/;\n";
        vector<FunctionPtr> lookupFunctionTable;
        logs << "    bool anyPInvoke = false;\n";
        if ( totalFunctions ) {
            for (auto & pm : library.modules) {
                pm->functions.foreach([&](auto pfun){
                    if (pfun->index < 0 || !pfun->used)
                        return;
                    if ( (pfun->init || pfun->shutdown) && disableInit ) {
                        error("[init] is disabled in the options or CodeOfPolicies",
                            "internal compiler error. [init] function made it all the way to simulate somehow", "",
                                pfun->at, CompilationError::no_init);
                    }
                    logs << "     // totalFunctions  "  << "\n";
                    logs << "    {\n";
                    logs << "        string mangledName = \"" << pfun->getMangledName() << "\"/*pfun->getMangledName()*/;\n";
                    logs << "        auto MNH = hash_blockz64((uint8_t *)mangledName.c_str());\n";
                    logs << "        auto & gfun = context.functions[" << pfun->index << "/*pfun->index*/];\n";
                    logs << "        gfun.name = context.code->allocateName(\"" << pfun->name << "\"/*pfun->name*/);\n";
                    logs << "        gfun.mangledName = context.code->allocateName(mangledName);\n";
                    logs << "        gfun.stackSize = " << pfun->totalStackSize << "/*pfun->totalStackSize*/;\n";
                    logs << "        gfun.mangledNameHash = MNH;\n";
                    logs << "        gfun.aotFunction = nullptr;\n";
                    logs << "        gfun.flags = 0;\n";
                    logs << "        gfun.fastcall = " << pfun->fastCall << "/*pfun->fastCall*/;\n";
                    logs << "        gfun.unsafe = " << pfun->unsafeOperation << "/*pfun->unsafeOperation*/;\n";
                    logs << "        if ( " << (pfun->result->isRefType() && !pfun->result->ref)
                                        << "/*(pfun->result->isRefType() && !pfun->result->ref)*/ ) {\n";
                    logs << "            gfun.cmres = true;\n";
                    logs << "        }\n";
                    logs << "        if ( " << (pfun->module->builtIn && !pfun->module->promoted)
                                        << "/*(pfun->module->builtIn && !pfun->module->promoted)*/ ) {\n";
                    logs << "            gfun.builtin = true;\n";
                    logs << "        }\n";

                    logs << "        if ( " << pfun->pinvoke << "/*pfun->pinvoke*/ ) {\n";
                    logs << "            anyPInvoke = true;\n";
                    logs << "            gfun.pinvoke = true;\n";
                    logs << "        }\n";
                    logs << "    }\n";
                    lookupFunctionTable.push_back(pfun);
                });
            }
        }

        logs << "    if ( anyPInvoke || " << (policies.threadlock_context || policies.debugger)
                                        << "/*(policies.threadlock_context || policies.debugger)*/ ) {\n";
        logs << "        context.contextMutex = new recursive_mutex;\n";
        logs << "    }\n";

        logs << "    context.tabMnLookup = make_shared<das_hash_map<uint64_t,SimFunction *>>();\n";
        logs << "    context.tabMnLookup->clear();\n";

        for ( const auto & fn : lookupFunctionTable ) {
            auto mnh = fn->getMangledNameHash();

            logs << "    // " << fn->getMangledName() << "\n";
            logs << "    (*context.tabMnLookup)["<< mnh <<"/*mnh*/] = context.functions + " << fn->index << "/*fn->index*/;\n";
        }

        logs << "    context.tabGMnLookup = make_shared<das_hash_map<uint64_t,uint32_t>>();\n";
        logs << "    context.tabGMnLookup->clear();\n";
        logs << "    for ( int i=0, is=context.totalVariables; i!=is; ++i ) {\n";
        logs << "        auto mnh = context.globalVariables[i].mangledNameHash;\n";
        logs << "        (*context.tabGMnLookup)[mnh] = context.globalVariables[i].offset;\n";
        logs << "    }\n";

        logs << "    for ( int i=0, is=context.totalVariables; i!=is; ++i ) {\n";
        logs << "        auto & gvar = context.globalVariables[i];\n";
        logs << "        uint32_t voffset = context.globalOffsetByMangledName(gvar.mangledNameHash);\n";
        logs << "    }\n";

        logs << "    context.tabAdLookup = make_shared<das_hash_map<uint64_t,uint64_t>>();\n";
        for (auto & pm : library.modules ) {
            for(auto s2d : pm->annotationData ) {
                logs << "    (*context.tabAdLookup)["<< s2d.first <<"] = "<< s2d.second <<";\n";
            }
        }

        vector<pair<string, uint64_t>> fnn; fnn.reserve(totalFunctions);
        for (auto & pm : library.modules) {
            pm->functions.foreach([&](auto pfun){
                if (pfun->index < 0 || !pfun->used)
                    return;
                fnn.emplace_back(pfun->name, getFunctionAotHash(pfun.get()));
            });
        }

        logs << "    auto & aotLib = getGlobalAotLibrary();\n";
        logs << "    SimFunction * fn = nullptr;\n";

        for ( int fni=0, fnis=totalFunctions; fni!=fnis; ++fni ) {
            const auto & [name, aotHash] = fnn[fni];
            logs << " // fnis = " << fni << "\n";
            logs << "    fn = &context.functions[" << fni << "/*fni*/];\n";
            logs << "    {\n";
            logs << "        // " << name << "\n";
            logs << "        uint64_t semHash = 0x" << HEX << aotHash << DEC << "/*fnn[fni]*/;\n";
            logs << "        auto it = aotLib.find(semHash);\n";
            logs << "        if ( it != aotLib.end() ) {\n";
            logs << "            fn->code = (it->second)(context);\n";
            logs << "            fn->aot = true;\n";
            logs << "            auto fcb = (SimNode_CallBase *) fn->code;\n";
            logs << "            fn->aotFunction = fcb->aotFunction;\n";
            logs << "        }\n";
            logs << "    }\n";
        }
    // aot init
        if ( initSemanticHashWithDep ) {
            logs << "    {\n";
            logs << "        uint64_t semHash = 0x" << HEX << initSemanticHashWithDep << DEC <<"/*initSemanticHashWithDep*/;\n";
            logs << "        auto it = aotLib.find(semHash);\n";
            logs << "        if ( it != aotLib.end() ) {\n";
            logs << "            (it->second)(context);\n";
            logs << "        }\n";
            logs << "    }\n";
        }

        logs << "    context.runInitScript();\n";

        logs << "    }\n";
        logs << "};\n";

        logs << "#ifdef STANDALONE_CONTEXT_TESTS\n";
        logs << "static Context * registerStandaloneTest ( ) {\n";
        logs << "    auto ctx = new StandaloneContext();\n";
        logs << "    return ctx;\n";
        logs << "}\n";
        logs << "StandaloneContextNode node(registerStandaloneTest);\n";
        logs << "#endif\n";

    }

    class StandaloneContextGen : public CppAot {
    public:
      StandaloneContextGen(ProgramPtr prog, BlockVariableCollector &coll,
                           string cppOutD, string standaloneContextName)
          : CppAot(prog, coll), contextNameSuffix(standaloneContextName) {
            cppOutputDir = cppOutD;
      }
    private:
        void writeAotHeaderIncludes () {
            ss << "#include \"daScript/misc/platform.h\"\n\n";

            ss << "#include \"daScript/simulate/simulate.h\"\n";
            ss << "#include \"daScript/simulate/aot.h\"\n";
            ss << "#include \"daScript/simulate/aot_library.h\"\n";
            ss << "\n";
        }

        void writeAotHeader () {
            ss << "\n";
            ss << "#if defined(_MSC_VER)\n";
            ss << "#pragma warning(push)\n";
            ss << "#pragma warning(disable:4100)   // unreferenced formal parameter\n";
            ss << "#pragma warning(disable:4189)   // local variable is initialized but not referenced\n";
            ss << "#pragma warning(disable:4244)   // conversion from 'int32_t' to 'float', possible loss of data\n";
            ss << "#pragma warning(disable:4114)   // same qualifier more than once\n";
            ss << "#pragma warning(disable:4623)   // default constructor was implicitly defined as deleted\n";
            ss << "#pragma warning(disable:4946)   // reinterpret_cast used besseen related classes\n";
            ss << "#pragma warning(disable:4269)   // 'const' automatic data initialized with compiler generated default constructor produces unreliable results\n";
            ss << "#pragma warning(disable:4555)   // result of expression not used\n";
            ss << "#endif\n";
            ss << "#if defined(__GNUC__) && !defined(__clang__)\n";
            ss << "#pragma GCC diagnostic push\n";
            ss << "#pragma GCC diagnostic ignored \"-Wunused-parameter\"\n";
            ss << "#pragma GCC diagnostic ignored \"-Wunused-variable\"\n";
            ss << "#pragma GCC diagnostic ignored \"-Wunused-function\"\n";
            ss << "#pragma GCC diagnostic ignored \"-Wwrite-strings\"\n";
            ss << "#pragma GCC diagnostic ignored \"-Wreturn-local-addr\"\n";
            ss << "#pragma GCC diagnostic ignored \"-Wignored-qualifiers\"\n";
            ss << "#pragma GCC diagnostic ignored \"-Wsign-compare\"\n";
            ss << "#pragma GCC diagnostic ignored \"-Wsubobject-linkage\"\n";
            ss << "#endif\n";
            ss << "#if defined(__clang__)\n";
            ss << "#pragma clang diagnostic push\n";
            ss << "#pragma clang diagnostic ignored \"-Wunused-parameter\"\n";
            ss << "#pragma clang diagnostic ignored \"-Wwritable-strings\"\n";
            ss << "#pragma clang diagnostic ignored \"-Wunused-variable\"\n";
            ss << "#if defined(__APPLE__)\n";
            ss << "#pragma clang diagnostic ignored \"-Wunused-but-set-variable\"\n";
            ss << "#endif\n";
            ss << "#pragma clang diagnostic ignored \"-Wunsequenced\"\n";
            ss << "#pragma clang diagnostic ignored \"-Wunused-function\"\n";
            ss << "#endif\n";
            ss << "\n";
        }

        void writeAotFooter () {
            ss << "#if defined(_MSC_VER)\n";
            ss << "#pragma warning(pop)\n";
            ss << "#endif\n";
            ss << "#if defined(__GNUC__) && !defined(__clang__)\n";
            ss << "#pragma GCC diagnostic pop\n";
            ss << "#endif\n";
            ss << "#if defined(__clang__)\n";
            ss << "#pragma clang diagnostic pop\n";
            ss << "#endif\n";
        }

        void writeRegistration ( Context & context ) {
            ss << "namespace "      << program->thisNamespace << " {\n";
            ss << "\nstatic void registerAotFunctions ( AotLibrary & aotLib ) {\n";
            program->registerAotCpp(ss, context, false, true);
            ss << "\tresolveTypeInfoAnnotations();\n";
            ss << "};\n";
            ss << "\n";
            ss << "AotListBase impl(registerAotFunctions);\n";
            ss << "} // namespace " << program->thisNamespace << "\n";

            ss << "namespace "      << contextNameSuffix << " {\n";
            program->writeStandaloneContext(ss);
            ss << "} // namespace " << contextNameSuffix << "\n";
        }

        void writeRequiredModulesFor ( Module * mod ) {
            // lets comment on required modules
            for ( auto [req, pub] : mod->requireModule ) {
                if ( req->name=="" ) {
                    // nothing, its main program module. i.e ::
                } else {
                    if ( req->name=="$" ) {
                        ss << " // require builtin\n";
                    } else {
                        ss << " // require " << req->name << "\n";
                    }
                    if ( req->aotRequire(ss)==ModuleAotType::no_aot ) {
                        ss << "  // no_aot ignored in standalone context\n";
                    }
                }
            }
        }

        void setAotHashes ( Context & context ) {
            // compute semantic hash for each used function
            int fni = 0;
            for ( auto & pm : program->library.getModules() ) {
                pm->functions.foreach([&](auto pfun){
                    if (pfun->index < 0 || !pfun->used)
                        return;
                    SimFunction * fn = context.getFunction(fni);
                    pfun->hash = getFunctionHash(pfun.get(), fn->code, &context);
                    fni++;
                });
            }
            // compute AOT hash for each used function
            // its the same as semantic hash, only takes dependencies into account
            for (auto & pm : program->library.getModules() ) {
                pm->functions.foreach([&](auto pfun){
                    if (pfun->index < 0 || !pfun->used)
                        return;
                    pfun->aotHash = getFunctionAotHash(pfun.get());
                    fni++;
                });
            }
        }

        bool saveToFile ( const string & fname, const string & str ) {
            FILE * f = fopen (fname.c_str(), "w");
            if ( !f ) return false;
            size_t bytes_written = fwrite(str.c_str(), str.length(), 1, f);
            fclose(f);
            return bytes_written == str.length();
        }
    public:
        virtual void visitGlobalLetBody ( Program * prog ) override {
            vector<Variable*> globals;
            prog->library.foreach([&]( Module * pm ) {
                pm->globals.foreach([&]( VariablePtr pvar ){
                    if ( pvar->index < 0 || !pvar->used ) return;
                    if ( pvar->module == prog->thisModule.get() ) return;
                    globals.push_back(pvar.get());
                });
                return true;
            }, "*");

            for ( auto var : globals ) {
                preVisitGlobalLet(var);
                if ( var->init ) {
                    preVisitGlobalLetInit(var, var->init.get());
                    var->init = var->init->visit(*this);
                    var->init = visitGlobalLetInit(var, var->init.get());
                }
                auto varn = visitGlobalLet(var);
            }
            tab --;
            ss << "}\n";
            Visitor::visitGlobalLetBody(prog);
        }
        virtual void preVisitProgramBody ( Program * prog, Module * that ) override {
            // functions
            ss << "\n";
            // print forward declarations
            vector<Function *> fnn; fnn.reserve(prog->totalFunctions);
            prog->library.foreach([&](Module * pm) {
                pm->functions.foreach([&](auto pfun) {
                    if ( pfun->index < 0 || !pfun->used ) return true;
                    if ( pfun->builtIn || pfun->noAot) return true;
                    auto needInline = that == pm;
                    ss << describeCppFunc(pfun.get(), &collector, true, needInline) << ";\n";
                    return true;
                });
                return true;
            }, "*");
            ss << "\n";
        }
        bool run() {
            shared_ptr<Context> pctx ( get_context(program->getContextStackSize()) );
            if ( !program->simulate(*pctx, tw) ) {
                tw << "failed to simulate\n";
                for ( auto & err : program->errors ) {
                    tw << reportError(err.at, err.what, err.extra, err.fixme, err.cerr);
                }
                return false;
            }
            Context & context = *pctx;
            // header

            daScriptEnvironment::bound->g_Program = program;    // setting it for the AOT macros


            // mark prologue
            PrologueMarker pmarker;
            program->visit(pmarker);

            setAotHashes(context);

            // now, for that AOT
            program->setPrintFlags();
            program->visit(collector);


            program->library.foreach([&] (Module * mod) {
                // if ( mod->isProperBuiltin() ) return true;
                moduleNamespace = mod->promoted ? "" : mod->name;

                ss << "// Module " << mod->name << "\n";

                writeAotHeaderIncludes();
                writeRequiredModulesFor(mod);
                writeAotHeader();

                ss << "namespace das {\n";
                program->visitModule(*this, mod);
                if ( mod->name.empty() ) writeRegistration(context);
                ss << "} // namespace das\n";

                writeAotFooter();

                nameToOutput[mod->name] = ss.str();
                ss = std::move(TextWriter{}); // clear the stream
                return true;
            }, "*");

            // get the name of the current file from program?

            for ( auto & [nm, out] : nameToOutput ) {
                if ( nm.empty() ) nm = contextNameSuffix;
                const auto outputFile = cppOutputDir + '/' + nm + ".das.cpp";
                saveToFile(outputFile, out);
            }

            daScriptEnvironment::bound->g_Program.reset();

            return true;
        }
    private:
        TextWriter                  tw;
        das_map<string, string>     nameToOutput;
        string                      cppOutputDir;
        string                      moduleNamespace;
        const string                contextNameSuffix;
    };

    void runStandaloneVisitor ( ProgramPtr prog, string cppOutputDir, string standaloneContextName ) {
        BlockVariableCollector coll;
        StandaloneContextGen gen(prog, coll, cppOutputDir, standaloneContextName);
        gen.run();
    }

    void Program::aotCpp ( Context & context, TextWriter & logs ) {
        // run no-aot marker
        NoAotMarker marker;
        visit(marker);
        // mark prologue
        PrologueMarker pmarker;
        visit(pmarker);
        // compute semantic hash for each used function
        int fni = 0;
        for (auto & pm : library.modules) {
            pm->functions.foreach([&](auto pfun){
                if (pfun->index < 0 || !pfun->used)
                    return;
                SimFunction * fn = context.getFunction(fni);
                pfun->hash = getFunctionHash(pfun.get(), fn->code, &context);
                fni++;
            });
        }
        // compute AOT hash for each used function
        // its the same as semantic hash, only takes dependencies into account
        for (auto & pm : library.modules) {
            pm->functions.foreach([&](auto pfun){
                if (pfun->index < 0 || !pfun->used)
                    return;
                pfun->aotHash = getFunctionAotHash(pfun.get());
                fni++;
            });
        }
        // now, for that AOT
        setPrintFlags();
        BlockVariableCollector collector;
        visit(collector);
        CppAot aotVisitor(this,collector);
        // pre visit all enumerations and structures for each dependency
        bool remUS = options.getBoolOption("remove_unused_symbols",true);
        UseTypeMarker utm;
        visit(utm);
        for ( auto & pm : library.modules ) {
            pm->structures.foreach([&](auto ps){
                aotVisitor.ss << "namespace " << aotModuleName(ps->module) << " { struct " << ps->name << "; };\n";
            });
        }
        for ( auto & pm : library.modules ) {
            if ( pm == thisModule.get() ) {
                continue;
            }
            pm->enumerations.foreach([&](auto penum){
                auto pe = penum.get();
                if ( !remUS || utm.useEnums.find(pe)!=utm.useEnums.end() ) {
                    visitEnumeration(aotVisitor, pe);
                } else {
                    aotVisitor.ss << "// unused enumeration " << pe->name << "\n";
                }
            });
            // aotVisitor.ss << "namespace " << aotModuleName(pm) << " {\n";
            pm->structures.foreach([&](auto ps){
                if ( !remUS || utm.useStructs.find(ps.get())!=utm.useStructs.end() ) {
                    visitStructure(aotVisitor, ps.get());
                } else {
                    aotVisitor.ss << "// unused structure " << ps->name << "\n";
                }
            });
            // aotVisitor.ss << "\n}; // " << pm->name << "\n";
        }
        // now to the main body
        visit(aotVisitor);
        logs << aotVisitor.str();
    }
}

