options gen2

module ast_aot_cpp

require fio
require strings

require daslib/match
require daslib/strings_boost
require daslib/ast_boost
require daslib/templates_boost
require daslib/functional
require daslib/algorithm

require printer_flags_visitor
require aot_constants

options strict_smart_pointers = false


def aotFunctionName(str : string) {
    return replace(str, "`", "__")
}

def aotModuleName(pm : Module?) {
    if (pm.name.empty()) {
        return "";
    } elif (pm.name == "$") {
        return "_builtin_";
    } else {
        return string(pm.name);
    }
}


// validate swizzle mask and build mask type

def isSequencialMask(fields : dasvector`uint8) {
    for (i in range(1, length(fields))) {
        if (int(fields[i - 1]) + 1 != int(fields[i])) {
            return false;
        }
    }
    return true;
}


def das_to_cppString(t : Type) {
    match (t) {
        if (Type.anyArgument) { return "vec4f"; }
        if (Type.tVoid) { return "void"; }
        if (Type.tBool) { return "bool"; }
        if (Type.tInt8) { return "int8_t"; }
        if (Type.tUInt8) { return "uint8_t"; }
        if (Type.tInt16) { return "int16_t"; }
        if (Type.tUInt16) { return "uint16_t"; }
        if (Type.tInt64) { return "int64_t"; }
        if (Type.tUInt64) { return "uint64_t"; }
        if (Type.tBitfield) { return "Bitfield"; }
        if (Type.tString) { return "char *"; }
        if (Type.tInt) { return "int32_t"; }
        if (Type.tInt2) { return "int2"; }
        if (Type.tInt3) { return "int3"; }
        if (Type.tInt4) { return "int4"; }
        if (Type.tUInt) { return "uint32_t"; }
        if (Type.tUInt2) { return "uint2"; }
        if (Type.tUInt3) { return "uint3"; }
        if (Type.tUInt4) { return "uint4"; }
        if (Type.tFloat) { return "float"; }
        if (Type.tFloat2) { return "float2"; }
        if (Type.tFloat3) { return "float3"; }
        if (Type.tFloat4) { return "float4"; }
        if (Type.tDouble) { return "double"; }
        if (Type.tRange) { return "range"; }
        if (Type.tURange) { return "urange"; }
        if (Type.tRange64) { return "range64"; }
        if (Type.tURange64) { return "urange64"; }
        if (Type.tBlock) { return "Block"; }
        if (Type.tFunction) { return "Func"; }
        if (Type.tLambda) { return "Lambda"; }
        if (Type.tTuple) { return "Tuple"; }
        if (Type.tVariant) { return "Variant"; }
        if (_) { panic("Missed type"); return ""; }
    }
}

def das_to_cppCTypeString(t : Type) {
    match (t) {
        if (Type.autoinfer) { return "autoinfer"; }
        if (Type.alias) { return "alias"; }
        if (Type.anyArgument) { return "anyArgument"; }
        if (Type.tVoid) { return "tVoid"; }
        if (Type.tStructure) { return "tStructure"; }
        if (Type.tPointer) { return "tPointer"; }
        if (Type.tBool) { return "tBool"; }
        if (Type.tInt8) { return "tInt8"; }
        if (Type.tUInt8) { return "tUInt8"; }
        if (Type.tInt16) { return "tInt16"; }
        if (Type.tUInt16) { return "tUInt16"; }
        if (Type.tInt64) { return "tInt64"; }
        if (Type.tUInt64) { return "tUInt64"; }
        if (Type.tString) { return "tString"; }
        if (Type.tPointer) { return "tPointer"; }
        if (Type.tEnumeration) { return "tEnumeration"; }
        if (Type.tEnumeration8) { return "tEnumeration8"; }
        if (Type.tEnumeration16) { return "tEnumeration16"; }
        if (Type.tEnumeration64) { return "tEnumeration64"; }
        if (Type.tBitfield) { return "tBitfield"; }
        if (Type.tIterator) { return "tIterator"; }
        if (Type.tArray) { return "tArray"; }
        if (Type.tTable) { return "tTable"; }
        if (Type.tInt) { return "tInt"; }
        if (Type.tInt2) { return "tInt2"; }
        if (Type.tInt3) { return "tInt3"; }
        if (Type.tInt4) { return "tInt4"; }
        if (Type.tUInt) { return "tUInt"; }
        if (Type.tUInt2) { return "tUInt2"; }
        if (Type.tUInt3) { return "tUInt3"; }
        if (Type.tUInt4) { return "tUInt4"; }
        if (Type.tFloat) { return "tFloat"; }
        if (Type.tFloat2) { return "tFloat2"; }
        if (Type.tFloat3) { return "tFloat3"; }
        if (Type.tFloat4) { return "tFloat4"; }
        if (Type.tDouble) { return "tDouble"; }
        if (Type.tRange) { return "tRange"; }
        if (Type.tURange) { return "tURange"; }
        if (Type.tRange64) { return "tRange64"; }
        if (Type.tURange64) { return "tURange64"; }
        if (Type.tBlock) { return "tBlock"; }
        if (Type.tFunction) { return "tFunction"; }
        if (Type.tLambda) { return "tLambda"; }
        if (Type.tTuple) { return "tTuple"; }
        if (Type.tVariant) { return "tVariant"; }
        if (Type.tHandle) { return "tHandle"; }
        if (_) { panic("Missed type"); return ""; }
    }
}



def isConstRedundantForCpp(typeDecl : TypeDeclPtr) {
    if (!empty(typeDecl.dim)) { return false; }
    if (typeDecl.isVectorType) { return true; }
    match (typeDecl.baseType) {
        if (Type.tBool) { return true; }
        if (Type.tInt8) { return true; }
        if (Type.tUInt8) { return true; }
        if (Type.tInt16) { return true; }
        if (Type.tUInt16) { return true; }
        if (Type.tInt64) { return true; }
        if (Type.tUInt64) { return true; }
        if (Type.tInt) { return true; }
        if (Type.tUInt) { return true; }
        if (Type.tFloat) { return true; }
        if (Type.tDouble) { return true; }
        if (Type.tEnumeration) { return true; }
        if (Type.tEnumeration8) { return true; }
        if (Type.tEnumeration16) { return true; }
        if (Type.tEnumeration64) { return true; }
        if (Type.tBitfield) { return true; }
        if (_) { return false; }
    }
}


enum CpptUseAlias {
    no
    yes
};

enum CpptSubstitureRef { no, yes };
enum CpptSkipRef { no, yes };
enum CpptSkipConst { no, yes };
enum CpptRedundantConst { no, yes };

struct DescribeConfig {
    substitute_ref : bool = false
    skip_ref : bool = false
    skip_const : bool = false
    redundant_const : bool = true
    cross_platform : bool = false
    use_smart_ptr : bool = false
}

def aotSuffixNameEx(funcName : das_string; suffix : string) {
    var prefix = false;

    let name = build_string() <| $(var writer) {
        for (ch in string(funcName)) {
            if (is_alnum(ch) || ch == '_') {
                writer |> write_char(ch);
            } else {
                prefix = true;
                var t_ch : string;
                match (ch) {
                    if ('=') { t_ch = "Equ"; }
                    if ('+') { t_ch = "Add"; }
                    if ('-') { t_ch = "Sub"; }
                    if ('*') { t_ch = "Mul"; }
                    if ('/') { t_ch = "Div"; }
                    if ('%') { t_ch = "Mod"; }
                    if ('&') { t_ch = "And"; }
                    if ('|') { t_ch = "Or"; }
                    if ('^') { t_ch = "Xor"; }
                    if ('?') { t_ch = "Qmark"; }
                    if ('~') { t_ch = "Tilda"; }
                    if ('!') { t_ch = "Excl"; }
                    if ('>') { t_ch = "Greater"; }
                    if ('<') { t_ch = "Less"; }
                    if ('[') { t_ch = "Sqbl"; }
                    if (']') { t_ch = "Sqbr"; }
                    if ('.') { t_ch = "Dot"; }
                    if ('`') { t_ch = "Tick"; }
                    if (_) {
                        let repr = build_string() <| $(ss) {
                            ss |> write_char('0' + (ch >> 4))
                            ss |> write_char(int(uint('0') + (uint(ch) & 0x0f)));
                        }
                        t_ch = "_0x{repr}_"
                    }
                }
                write(writer, "{t_ch}")
            }
        }

    }
    return prefix ? (suffix + name) : name;
}

def aotStructName(st : Structure?) {
    return aotSuffixNameEx(st.name, "");
}


def describeCppTypeEx(typeDecl : TypeDeclPtr;
                      in_cfg : DescribeConfig;
                      useAlias : CpptUseAlias) {
    var cfg := in_cfg
    if (isConstRedundantForCpp(typeDecl) && cfg.redundant_const) {
        if (cfg.substitute_ref && typeDecl.flags.ref) {
            // can't skip const
        } elif (typeDecl.flags.ref) {
            // can't skip const
        } else {
            cfg.skip_const = true;
        }
    }
    let baseType = typeDecl.baseType;

    return build_string() <| $(writer) {
        for (d in range(length(typeDecl.dim))) {
            write(writer, "TDim<");
        }
        if (useAlias == CpptUseAlias.yes && typeDecl.flags.aotAlias && !typeDecl.alias.empty()) {
            write(writer, "{typeDecl.alias}");
        } elif (baseType == Type.alias) {
            write(writer, "DAS_COMMENT(alias)");
        } elif (baseType == Type.autoinfer) {
            var alias = "";
            if (!typeDecl.alias.empty()) {
                alias = "({typeDecl.alias})";
            }
            write(writer, "DAS_COMMENT(auto{alias})")
        } elif (baseType == Type.tHandle) {
            let handle_name = string(typeDecl.annotation.cppName.empty() ? typeDecl.annotation.name : typeDecl.annotation.cppName)
            write(writer, "{handle_name}")
        } elif (baseType == Type.tArray) {
            if (typeDecl.firstType != null) {
                let cpp_typeDecl = describeCppTypeEx(typeDecl.firstType, DescribeConfig(cross_platform = cfg.cross_platform), useAlias);
                write(writer, "TArray<{cpp_typeDecl}>")
            } else {
                write(writer, "Array")
            }
        } elif (baseType == Type.tTable) {
            if (typeDecl.firstType != null && typeDecl.secondType != null) {
                let first_type = describeCppTypeEx(typeDecl.firstType, DescribeConfig(cross_platform = cfg.cross_platform), useAlias);
                let second_type = describeCppTypeEx(typeDecl.secondType, DescribeConfig(cross_platform = cfg.cross_platform), useAlias);
                write(writer, "TTable<{first_type},{second_type}>");
            } else {
                write(writer, "Table");
            }
        } elif (baseType == Type.tTuple) {
            let types = join(map(each(typeDecl.argTypes), @(arg) {
                let s : string = describeCppTypeEx(arg, DescribeConfig(cross_platform = cfg.cross_platform), useAlias);
                return s;
            }), ",")
            if (cfg.cross_platform) {
                write(writer, "AutoTuple<{types}>")
            } else {
                write(writer, "TTuple<{int(typeDecl.tupleSize)},{types}>")
            }
        } elif (baseType == Type.tVariant) {
            let types = join(map(each(typeDecl.argTypes), @(arg) {
                let s : string = describeCppTypeEx(arg, DescribeConfig(cross_platform = cfg.cross_platform), useAlias);
                return s
            }), ",")
            if (cfg.cross_platform) {
                write(writer, "AutoVariant<{types}>");
            } else {
                write(writer, "TVariant<{int(typeDecl.variantSize)},{int(typeDecl.variantAlign)},{types}>")
            }
        } elif (baseType == Type.tStructure) {
            if (typeDecl.structType != null) {
                if (typeDecl.structType._module.name.empty()) {
                    write(writer, "{aotStructName(typeDecl.structType)}")
                } else {
                    write(writer, "{aotModuleName(typeDecl.structType._module)}::{aotStructName(typeDecl.structType)}")
                }
            } else {
                write(writer, "DAS_COMMENT(unspecified structure) ");
            }
        } elif (baseType == Type.tPointer) {
            if (!typeDecl.flags.smartPtr) {
                if (typeDecl.firstType != null) {
                    write(writer, "{describeCppTypeEx(typeDecl.firstType,DescribeConfig(redundant_const=false,cross_platform=cfg.cross_platform),useAlias)} *");
                } else {
                    write(writer, "void *");
                }
            } else {
                let ptr_name = typeDecl.flags.smartPtrNative && cfg.use_smart_ptr ? "smart_ptr" : "smart_ptr_raw"
                var type_decl_name = "void"
                if (typeDecl.firstType != null) {
                    type_decl_name = describeCppTypeEx(typeDecl.firstType, DescribeConfig(redundant_const = false, cross_platform = cfg.cross_platform), useAlias);
                }
                write(writer, "{ptr_name}<{type_decl_name}>");
            }
        } elif (typeDecl.isEnumT) {
            if (typeDecl.enumType != null) {
                if (typeDecl.enumType.external) {
                    write(writer, "DAS_COMMENT(bound_enum) {typeDecl.enumType.cppName}");
                } elif (typeDecl.enumType._module.name.empty()) {
                    write(writer, "DAS_COMMENT(enum) {typeDecl.enumType.name}");
                } else {
                    write(writer, "DAS_COMMENT(enum) {aotModuleName(typeDecl.enumType._module)}::{typeDecl.enumType.name}")
                }
            } else {
                write(writer, "DAS_COMMENT(unspecified enumeration)")
            }
        } elif (baseType == Type.tIterator) {
            if (typeDecl.firstType != null) {
                let new_cfg = DescribeConfig(substitute_ref = cfg.substitute_ref,
                                         skip_ref = cfg.skip_ref,
                                         skip_const = cfg.skip_const,
                                         redundant_const = true,
                                         cross_platform = cfg.cross_platform)
                write(writer, "Sequence DAS_COMMENT(({describeCppTypeEx(typeDecl.firstType,new_cfg,useAlias)}))")
            } else {
                write(writer, "Sequence")
            }
        } elif (baseType == Type.tBlock || baseType == Type.tFunction || baseType == Type.tLambda) {
            var maybe_const = !typeDecl.flags.constant && typeDecl.baseType == Type.tBlock ? "const " : ""
            var type_name = "void"
            if (typeDecl.firstType != null) {
                type_name = describeCppTypeEx(typeDecl.firstType, DescribeConfig(redundant_const = true, cross_platform = cfg.cross_platform), useAlias)
            }
            let extra_comma = length(typeDecl.argTypes) != 0 ? "," : "";
            let arg_types = join(map(each(typeDecl.argTypes), @(arg) {
                let s : string = describeCppTypeEx(arg, DescribeConfig(redundant_const = true, cross_platform = cfg.cross_platform), useAlias);
                return s
            }), ",")
            write(writer, "{maybe_const}{das_to_cppString(baseType)} DAS_COMMENT(({type_name}{extra_comma}{arg_types}))");
        } else {
            write(writer, das_to_cppString(baseType));
        }
        var args <- to_array(map(each(typeDecl.dim), @(itd) { return ",{itd}>"; }))
        reverse(args);
        write(writer, "{join(args, "")}")

        if (cfg.skip_const == false && typeDecl.flags.constant) {
            write(writer, " const ");
        }
        if (typeDecl.flags.ref && cfg.skip_ref == false) {
            if (cfg.substitute_ref == false) {
                write(writer, " &");
            } else {
                write(writer, " *");
            }
        }
    }
}

def describeCppType(typeDecl : TypeDeclPtr;
                    cfg : DescribeConfig) {
    return describeCppTypeEx(typeDecl, cfg, CpptUseAlias.no);
}

class NoAotMarker : AstVisitor {
    @do_not_delete func : Function? <- null;

    // type
    def override preVisitTypeDecl(typeDecl : TypeDeclPtr) {
        if (func != null && !typeDecl.canAot) {
            func.flags |= FunctionFlags.noAot
        }
    }
    // function
    def override preVisitFunction(var f : FunctionPtr) {
        func = get_ptr(f);
    }
    def override visitFunction(var that : FunctionPtr) : FunctionPtr {
        var tmp := func
        func = null;
        return <- unsafe(reinterpret<FunctionPtr>(tmp));
    }
    // any expression
    def override preVisitExpression(expr : ExpressionPtr) {
        if (func != null && expr._type != null && !expr._type.canAot) {
            func.flags |= FunctionFlags.noAot
        }
    }
    // looks like call
    def override preVisitExprLooksLikeCall(call : smart_ptr<ExprLooksLikeCall>) {
        if (call.name == "invoke") {   // invoke of anonymous blk
            if (!empty(call.arguments) && call.arguments[0] is ExprMakeBlock) {
                let mkb = call.arguments[0] as ExprMakeBlock;
                var blk = mkb._block as ExprBlock;
                blk.blockFlags |= ExprBlockFlags.aotSkipMakeBlock;
            }
        }
    }
};

class PrologueMarker : AstVisitor {
    @do_not_delete func : Function? <- null;

    // function
    def override preVisitFunction(var f : FunctionPtr) {
        func = get_ptr(f);
    }
    def override visitFunction(var that : FunctionPtr) {
        func = null;
        return <- that;
    }
    // ExprMakeBlock
    def override preVisitExprMakeBlock(expr : smart_ptr<ExprMakeBlock>) {
        if (func != null && func.flags.hasMakeBlock) {
            let _blk = expr._block as ExprBlock;
            if (!_blk.blockFlags.aotSkipMakeBlock) {
                func.flags |= FunctionFlags.aotNeedPrologue;
            }
        }
    }
};

class UseTypeMarker : AstVisitor {
    useStructs : table<Structure?>;
    useEnums : table<Enumeration?>;

    def override preVisitTypeDecl(typeDecl : TypeDeclPtr) {
    }

    def override preVisitExpression(expr : ExpressionPtr) {
        mark(expr._type);
    }
    def override preVisitFunctionArgument(fn : FunctionPtr; variable : VariablePtr; lastArg : bool) {
        mark(variable._type);
    }
    def override preVisitExprBlockArgument(expr_blk : smart_ptr<ExprBlock>; variable : VariablePtr; lastArg : bool) {
        mark(variable._type);
    }
    def mark(decl : TypeDeclPtr) {
        if (decl == null) return ;
        if (decl.baseType == Type.tStructure) {
            assert(decl.structType != null);
            if (!(useStructs |> key_exists(decl.structType))) {
                useStructs.insert(decl.structType);
                for (fld in decl.structType.fields) {
                    mark(fld._type);
                }
            }
        } elif (decl.baseType == Type.tEnumeration ||
                 decl.baseType == Type.tEnumeration8 ||
                 decl.baseType == Type.tEnumeration16 ||
                 decl.baseType == Type.tEnumeration64) {
            assert(decl.enumType != null);
            useEnums.insert(decl.enumType);
        } else {
            if (decl.firstType != null) mark(decl.firstType);
            if (decl.secondType != null) mark(decl.secondType);
            for (arg in decl.argTypes) {
                mark(arg);
            }
        }
    }
};


def enumInfoName(info : EnumInfo?) {
    return "__enum_info__{info.hash:x}";
}
def funcInfoName(info : FuncInfo?) {
    return "__func_info__{info.hash:x}";
}
def varInfoName(info : VarInfo?) {
    return "__var_info__{info.hash:x}";
}
def structInfoName(info : StructInfo?) {
    return "__struct_info__{info.hash:x}";
}
def typeInfoName(info : TypeInfo?) {
    return "__type_info__{info.hash:x}";
}

class AotDebugInfoHelper {
    helper : smart_ptr<DebugInfoHelper> <- new DebugInfoHelper(uninitialized);
    cross_platform : bool = false

    def writeDim(var writer : StringBuilderWriter?; info : TypeInfo?; suffix : string = "") {
        if (info.dimSize > 0u) {
            let dims = (range(info.dimSize) |> each()
                        |> map(@(i : int) { unsafe { return "{info.dim[i]:d}"; } })
                        |> join(", "))
            write(*writer, "uint32_t {typeInfoName(info)}_dim{suffix}[{info.dimSize:d}] = \{ {dims} \};\n");
        }
    }
    def writeArgNames(var writer : StringBuilderWriter?; info : TypeInfo?; suffix : string = "") {
        if (info.argCount > 0u && info.argNames != null) {
            let dims = (range(info.argCount) |> each()
                        |> map(@(i : int) { unsafe { return "\"{info.argNames[i]}\""; } })
                        |> join(", "))
            write(*writer, "const char * {typeInfoName(info)}_arg_names{suffix}[{info.argCount:d}] = \{ {dims} \};\n");
        }
    }
    def writeArgTypes(var writer : StringBuilderWriter?; info : TypeInfo?; suffix : string = "") {
        if (info.argCount > 0u && info.argTypes != null) {
            let dims = (range(info.argCount) |> each()
                        |> map(@(i : int) { unsafe { return "&{typeInfoName(info.argTypes[i])}"; } })
                        |> join(", "))
            write(*writer, "TypeInfo * {typeInfoName(info)}_arg_types{suffix}[{info.argCount:d}] = \{ {dims} \};\n");
        }
    }

    def str() {
        return build_string() <| $(var writer) {
            helper |> debug_helper_iter_structs($(name, ti) {
                write(writer, "extern StructInfo {structInfoName(ti)};\n");
            });
            helper |> debug_helper_iter_types($(name, ti){
                write(writer, "extern TypeInfo {typeInfoName(ti)};\n");
            })
            helper |> debug_helper_iter_vars($(name, ti){
                write(writer, "extern VarInfo {varInfoName(ti)};\n");
            })
            helper |> debug_helper_iter_funcs($(name, ti){
                write(writer, "extern FuncInfo {funcInfoName(ti)};\n");
            })
            helper |> debug_helper_iter_enums($(name, ti){
                write(writer, "extern EnumInfo {enumInfoName(ti)};\n");
            })
            write(writer, "\n");
            helper |> debug_helper_iter_enums($(name, ti){
                describeCppEnumInfoValues(unsafe(addr(writer)), ti);
                write(writer, "EnumInfo {enumInfoName(ti)} = \{ {describeCppEnumInfo(ti)} \};\n");
            })
            helper |> debug_helper_iter_structs($(name, ti){
                describeCppStructInfoFields(unsafe(addr(writer)), ti);
                write(writer, "StructInfo {structInfoName(ti)} = \{{describeCppStructInfo(ti)} \};\n");
            })
            helper |> debug_helper_iter_funcs($(name, ti){
                describeCppFuncInfoFields(unsafe(addr(writer)), ti);
                write(writer, "FuncInfo {funcInfoName(ti)} = \{{describeCppFuncInfo(ti)} \};\n");
            })
            helper |> debug_helper_iter_types($(name, ti){
                assume wr = unsafe(addr(writer))
                writeDim(wr, ti);
                writeArgTypes(wr, ti);
                writeArgNames(wr, ti);
                write(writer, "TypeInfo {typeInfoName(ti)} = \{ {describeCppTypeInfo(ti)} \};\n");
            })
            write(writer, "\n");
            write(writer, "static void resolveTypeInfoAnnotations()\n\{\n");
            write(writer, "    vector<TypeInfo> annotations = \{")
            helper |> debug_helper_iter_types($(name, ti) {
                if (ti._type == Type.tHandle) {
                    write(writer, "{typeInfoName(ti)}, ");
                }
            })
            write(writer, "\};\n");
            write(writer, "    for (auto& ann : annotations) \{\n")
            write(writer, "        ann.resolveAnnotation();\n")
            write(writer, "    \}\n")
            write(writer, "\}\n\n")
        }
    }


    def describeCppVarInfo(struct_name : string; info : VarInfo?; suffix : string) {
        return build_string() <| $(writer) {
            write(writer, "{describeCppTypeInfo(info, suffix)}, \"{info.name}\", ");
            if (cross_platform) {
                write(writer, "offsetof({struct_name},{info.name}), {info.nextGcField:d}");
            } else {
                write(writer, "{info.offset:d}, {info.nextGcField:d}")
            }
        }
    }
    def describeCppVarFuncInfo(struct_name : string; info : VarInfo?; suffix : string) {
        return build_string() <| $(writer) {
            write(writer, "{describeCppTypeInfo(info, suffix)}, \"{info.name}\", ");
            // Note: info offset is platform dependant,
            // however for functions it's not done yet and always zero.
            assert(info.offset == 0u);
            write(writer, "0, {info.nextGcField:d}");
        }
    }
    def describeCppStructInfoFields(var writer : StringBuilderWriter?; info : StructInfo?) {
        if (info.fields == null) return ;
        for (fi in range(info.count)) {
            let suffix = "_var_{info.hash:d}";
            unsafe {
                writeDim(writer, info.fields[fi], suffix);
                writeArgTypes(writer, info.fields[fi], suffix);
                writeArgNames(writer, info.fields[fi], suffix);
                let prefix = (info.module_name |> length() > 0) ? "{info.module_name}::" : "";
                write(*writer, "VarInfo {structInfoName(info)}_field_{fi} =  \{ {describeCppVarInfo(prefix + info.name, info.fields[fi],suffix)} \};\n");
            }
        }
        let fields = (info.count |> range()
                                 |> each()
                                 |> map(@(id : int) { return "&{structInfoName(info)}_field_{id}"; })
                                 |> join(", "))
        write(*writer, "VarInfo * {structInfoName(info)}_fields[{info.count:d}] =  \{ {fields} \};\n");
    }

    def describeCppStructInfo(info : StructInfo?) {
        return build_string() <| $(writer) {
            write(writer, "\"{info.name}\", \"{info.module_name}\", {int(info.flags):d}, ");
            if (info.fields != null) {
                write(writer, "{structInfoName(info)}_fields, ")
            } else {
                write(writer, "nullptr, ")
            }
            let info_size = cross_platform ? "TypeSize<{helper |> debug_helper_find_struct_cppname(info)}>::size" : "{info.size:d}";
            write(writer, "{info.count:d}, {info_size}, UINT64_C(0x{info.init_mnh:x}), nullptr, UINT64_C(0x{info.hash:x}), {info.firstGcField:d}")
        }
    }
    def describeCppFuncInfoFields(var writer : StringBuilderWriter?; info : FuncInfo?) {
        if (info.fields == null) return ;
        for (fi in range(info.count)) {
            let suffix = "_var_{info.hash:d}";
            unsafe {
                writeDim(writer, info.fields[fi], suffix);
                writeArgTypes(writer, info.fields[fi], suffix);
                writeArgNames(writer, info.fields[fi], suffix);
                write(*writer, "VarInfo {funcInfoName(info)}_field_{fi} =  \{ {describeCppVarFuncInfo(string(info.name), info.fields[fi],suffix)} \};\n");
            }
        }

        let fields = (info.count |> range() |> each()
                                |> map(@(id : int) { return "&{funcInfoName(info)}_field_{id}"; })
                                |> join(", "))
        write(*writer, "VarInfo * {funcInfoName(info)}_fields[{info.count:d}] =  \{ {fields} \};\n");
    }
    def describeCppFuncInfo(info : FuncInfo?) {
        return build_string() <| $(writer) {
            write(writer, "\"{info.name}\", \"{info.cppName}\", ");
            if (info.fields != null) {
                write(writer, "{funcInfoName(info)}_fields, ")
            } else {
                write(writer, "nullptr, ")
            }
            write(writer, "{info.count:d}, {info.stackSize:d}, &{typeInfoName(info.result)}, nullptr,0,UINT64_C(0x{info.hash:x}), 0x{info.flags:x}");
        }
    }
    def describeCppEnumInfoValues(var writer : StringBuilderWriter?; einfo : EnumInfo?) {
        for (v in range(einfo.count)) {
            unsafe {
                let val = einfo.fields[v];
                write(*writer, "EnumValueInfo {enumInfoName(einfo)}_value_{v} = \{ \"{val.name}\", {val.value} \};\n")
            }
        }
        let enum_vals = (einfo.count |> range() |> each()
                                    |> map(@(id : int) { return "&{enumInfoName(einfo)}_value_{id}"; })
                                    |> join(", "))
        write(*writer, "EnumValueInfo * {enumInfoName(einfo)}_values [] = \{ {enum_vals} \};\n");
    }
    def describeCppEnumInfo(info : EnumInfo?) {
        return "\"{info.name}\", \"{info.module_name}\", {enumInfoName(info)}_values, {info.count:d}, UINT64_C(0x{info.hash:x})";
    }
    def describeCppTypeInfo(info : TypeInfo?; suffix : string = "") {
        return build_string() <| $(writer) {
            let write_sep = $() { write(writer, ", "); };
            write(writer, "Type::{das_to_cppCTypeString(info._type)}");
            write_sep()
            if (info._type == Type.tStructure) {
                write(writer, "&{structInfoName(info.structType)}");
            } else {
                write(writer, "nullptr");
            }
            write_sep();
            if (info._type == Type.tEnumeration ||
                 info._type == Type.tEnumeration8 ||
                 info._type == Type.tEnumeration16 ||
                 info._type == Type.tEnumeration64) {
                write(writer, "&{enumInfoName(info.enumType)}");
            } else {
                write(writer, "nullptr");
            }
            write_sep();
            if (info._type == Type.tHandle) {
                let int_ptr = unsafe(reinterpret<uint64>(info.annotation_or_name));
                if ((int_ptr & uint64(1)) != uint64(0)) {
                    let tname = unsafe(reinterpret<string>(int_ptr ^ uint64(1)));  // already comes from string allocator
                    write(writer, "DAS_MAKE_ANNOTATION(\"{tname}\")");
                } else {
                    // we add ~ at the beginning of the name for padding
                    // if name is allocated by the compiler, it does not guarantee that it is aligned
                    // we check if there is a ~ at the beginning of the name, and if it is - we skip it
                    // that way we can accept both aligned and unaligned names
                    write(writer, "DAS_MAKE_ANNOTATION(\"~{info.annotation_or_name._module.name}::{info.annotation_or_name.name}\")");
                }
            } else {
                assert(info._type != Type.tHandle);
                write(writer, "nullptr");
            }
            write_sep();
            if (info.firstType != null) {
                write(writer, "&{typeInfoName(info.firstType)}");
            } else {
                write(writer, "nullptr");
            }
            write_sep();
            if (info.secondType != null) {
                write(writer, "&{typeInfoName(info.secondType)}");
            } else {
                write(writer, "nullptr");
            }

            write_sep();
            if (info.argCount > 0u && info.argTypes != null) {
                write(writer, "(TypeInfo **){typeInfoName(info)}_arg_types{suffix}");
            } else {
                write(writer, "nullptr");
            }
            write_sep();
            if (info.argCount > 0u && info.argNames != null) {
                write(writer, "{typeInfoName(info)}_arg_names{suffix}");
            } else {
                write(writer, "nullptr");
            }
            write_sep();
            write(writer, "{info.argCount:d}, {info.dimSize:d}");
            write_sep();
            if (info.dimSize > 0u) {
                write(writer, "{typeInfoName(info)}_dim{suffix}");
            } else {
                write(writer, "nullptr");
            }
            write_sep();
            write(writer, "{unsafe(reinterpret<int>(info.flags))}")
            write_sep();

            let info_size = cross_platform ? "TypeSize<{helper |> debug_helper_find_type_cppname(info)}>::size" : "{info.size:d}";
            write(writer, "{info_size}")
            write_sep();
            write(writer, "UINT64_C(0x{info.hash:x})");
        }
    }

};

def isLocalVec(vtype : TypeDeclPtr) {
    return empty(vtype.dim) && vtype.isVectorType && !vtype.flags.ref;
}

def describeLocalCppType(var writer : StringBuilderWriter?; vtype : TypeDeclPtr; cross_platform : bool; substituteRef : CpptSubstitureRef = CpptSubstitureRef.yes; skipConst : CpptSkipConst = CpptSkipConst.no) {
    let cfg = DescribeConfig(substitute_ref = substituteRef == CpptSubstitureRef.yes,
                             skip_const = skipConst == CpptSkipConst.yes,
                             cross_platform = cross_platform)
    write(*writer, "{describeCppType(vtype,cfg)}");
}

def describeVarLocalCppType(var writer : StringBuilderWriter?; vtype : TypeDeclPtr; cross_platform : bool; substituteRef : CpptSubstitureRef = CpptSubstitureRef.yes) {
    let cfg = DescribeConfig(substitute_ref = substituteRef == CpptSubstitureRef.yes,
                             skip_const = true,
                             skip_ref = false,
                             redundant_const = true,
                             cross_platform = cross_platform)
    if (vtype.isGoodBlockType) {
        write(*writer, "auto");
    } else {
        write(*writer, "{describeCppType(vtype,cfg)}");
    }
}

def aotFuncName(func : Function?) {
    if (func.hash != uint64(0)) {
        return "{aotSuffixNameEx(func.name,"_Func")}_{func.hash:x}";
    } else {
        return "{aotSuffixNameEx(func.name,"_Func")}";
    }
}

class BlockVariableCollector : AstVisitor {
    def override preVisitExprBlock(blk : smart_ptr<ExprBlock>) {
        push(stack, blk as ExprBlock);
    }
    def getVarName(variable : VariablePtr) {
        return rename?[variable.get_ptr()] ?? string(variable.name)
    }
    def isMoved(variable : VariablePtr) {
        return moved |> key_exists(variable.get_ptr())
    }
    def renameVariableTo(variable : VariablePtr; newName : string) {
        rename |> insert(variable.get_ptr(), newName);
    }

    def override visitExprBlock(var blk : smart_ptr<ExprBlock>) : ExpressionPtr {
        stack |> pop();
        return blk;
    }
    def needRenaming(variable : VariablePtr) {
        // TODO: check if it indeed needs renaming
        return true;
    }
    def renameVariable(variable : VariablePtr) {
        if (needRenaming(variable)) {
            rename |> insert(variable.get_ptr(), "__{aotSuffixNameEx(variable.name,"_Var")}_rename_at_{variable.at.line:d}_{(tempCounter++):d}");
        }
    }
// for loop
    def override preVisitExprForVariable(expr : smart_ptr<ExprFor>; variable : VariablePtr; last : bool) {
        for (varr in expr.iteratorVariables) {
            renameVariable(varr);
        }
    }
// blk argument
    def override preVisitExprBlockArgument(blk : smart_ptr<ExprBlock>; variable : VariablePtr; lastArg : bool) {
        renameVariable(variable);
    }
// functon argument
    def override preVisitFunctionArgument(fn : FunctionPtr; variable : VariablePtr; lastArg : bool) {
        renameVariable(variable);
    }
// let
    def getCurrentBlock() {
        var blk : ExprBlock? = null;
        for (i in range(length(stack))) {
            var pb = stack[length(stack) - i - 1];
            if (pb.blockFlags.isClosure) {
                blk = pb;
                break;
            }
            if (!(pb.blockFlags.inTheLoop && !pb.finalList |> empty())) {
                blk = pb;
                break;
            }
        }
        return blk;
    }
    def getFinalBlock() {
        for (i in range(length(stack))) {
            let blk = stack[length(stack) - i - 1];
            if (length(blk.finalList) != 0) return blk;
            if (blk.blockFlags.isClosure) return null;
        }
        return null;
    }
    def getTopBlock() {
        for (i in range(length(stack))) {
            let blk = stack[length(stack) - i - 1];
            if (blk.blockFlags.isClosure) return blk;
        }
        return stack[0];
    }
    def override preVisitExprLetVariable(let_var : smart_ptr<ExprLet>; var variable : VariablePtr; last : bool) {
        var bfinal = getFinalBlock()
        if (bfinal != null) {
            bfinal = getTopBlock();
            variables[bfinal] |> push(variable.get_ptr());
            moved |> insert(variable.get_ptr());
        }
        renameVariable(variable);
    }

    def handleExpr(var expr : smart_ptr<ExprMakeLocal>) {
        let need_insert = !expr.makeFlags.doesNotNeedSp && expr.stackTop != 0u;
        if (need_insert) {
            let blk = getCurrentBlock();
            localTemps[blk] |> push(expr.get_ptr());
        }
    }

// make array
    def override preVisitExprMakeArray(var expr : smart_ptr<ExprMakeArray>) {
        handleExpr(expr)
    }
// make tuple
    def override preVisitExprMakeTuple(var expr : smart_ptr<ExprMakeTuple>) {
        handleExpr(expr)
    }
// make structure
    def override preVisitExprMakeStruct(var expr : smart_ptr<ExprMakeStruct>) {
        handleExpr(expr)
    }
// make variant
    def override preVisitExprMakeVariant(var expr : smart_ptr<ExprMakeVariant>) {
        handleExpr(expr)
    }
// call with CMRES
    def override preVisitExprCall(var expr : smart_ptr<ExprCall>) {
        let need_insert = !expr.doesNotNeedSp && expr.stackTop != 0u;
        if (need_insert) {
            let blk = getCurrentBlock();
            localTemps[blk] |> push(expr.get_ptr());
        }
    }

    @do_not_delete stack :      array<ExprBlock?>
    @do_not_delete variables :  table<ExprBlock?, array<Variable?>>
    @do_not_delete localTemps : table<ExprBlock?, array<Expression?>>

    rename : table<Variable?, string>
    moved : table<Variable?>
    tempCounter : uint64 = uint64(0)
};

def collect_block_variables() {
}

def describeCppFunc(fn : FunctionPtr; var collector : BlockVariableCollector?; cross_platform : bool; needName : bool = true; needInline : bool = true) {
    return build_string() <| $(writer) {
        if (needInline) {
            write(writer, "inline ");
        }
        unsafe {describeLocalCppType(addr(writer), fn.result, cross_platform, CpptSubstitureRef.no, CpptSkipConst.yes); }
        write(writer, " {needName ? aotFuncName(fn.get_ptr()) : "(*)"}");
        unsafe {
            // let prefix = fn.arguments |> empty() ? "" : ", "
            /*
            let prefix = "";
            var args_str = "";
            for (arg1 in fn.arguments) {
                let arg : smart_ptr<Variable> := arg1;
                args_str += ", ";
                args_str += build_string() <| $(writer2) {
                                                    if (isLocalVec(arg._type)) {
                                                        unsafe {describeLocalCppType(addr(writer2), arg._type, cross_platform); }
                                                    } else {
                                                        write(writer2, "{describeCppType(arg._type, DescribeConfig(cross_platform=cross_platform))}");
                                                    }
                                                    write(writer2, " {(arg._type.isRefType) ? "& " : ""}");
                                                    if (collector != null) {
                                                        write(writer2, "{collector.getVarName(arg)}");
                                                    }
                                                }
            }
            */
            let prefix = fn.arguments |> empty() ? "" : ", "
            let args_str = (
                fn.arguments
                |> each()
                |> map(@(arg : smart_ptr<Variable>) : string {
                    return build_string() <| $(writer2) {
                        if (isLocalVec(arg._type)) {
                            unsafe {describeLocalCppType(addr(writer2), arg._type, cross_platform); }
                        } else {
                            write(writer2, "{describeCppType(arg._type, DescribeConfig(cross_platform=cross_platform))}");
                        }
                        write(writer2, " {(arg._type.isRefType) ? "& " : ""}");
                        if (collector != null) {
                            write(writer2, "{collector.getVarName(arg)}");
                        }
                    }
                })
                |> join(", ")
            )
            write(writer, " ( Context * __context__{prefix}{args_str} )");
        }
    }
}

class CppAot : AstVisitor {
/*
    CppAot ( const ProgramPtr & prog, BlockVariableCollector & cl ) : program(prog), collector(cl) {
        helper.rtti = program.options.getBoolOption("rtti",false);
        prologue = program.options.getBoolOption("aot_prologue",false) ||
            program.getDebugger();
        solidContext = program.policies.solid_context || program.options.getBoolOption("solid_context",false);
    }
*/

    adapter : smart_ptr<VisitorAdapter>

    def str() {
        let maybe_nl = type_info |> empty() ? "" : "\n";
        return "\n" + declarations + helper.str() + join(type_info, "\n") + "{maybe_nl}" + join(aot_prefixes, "") + ss |> string_builder_str();
    }

    def clear() {
        ss |> string_builder_clear();
        type_info |> clear();
        aot_prefixes |> clear();
    }
    ss : StringBuilderWriter ?;
    declarations : string
    type_info : array<string>;
    aot_prefixes : array<string>;// looks like it always empty. Can we remove it?
    lastNewLine : uint64 = -1ul;
    tab : int = 0;
    debugInfoGlobal : int = 0;
    helper : AotDebugInfoHelper ? = new AotDebugInfoHelper();
    program : ProgramPtr;
    collector : BlockVariableCollector ? = new BlockVariableCollector();
    aotPrefix : table<string>;
    @do_not_delete local_temp_names : table<Expression?; int>;
    @do_not_delete scopes : array<ExprBlock?>;
    prologue : bool = false;
    solidContext : bool = false;
    cross_platform : bool = false;
    def newLine() {
        // auto nlPos = ss.tellp();
        write(*ss, "\n");
        /*if ( nlPos != lastNewLine ) {
            lastNewLine = ss.tellp();
        }*/
    }
    def tabs() {
        return build_string() <| $(writer) { write_chars(writer, ' ', 4 * tab); }
    }

    def noBracket(expr : ExpressionPtr) {
        return expr.printFlags.topLevel || expr.printFlags.bottomLevel || expr.printFlags.argLevel;
    }
// enumeration
    def override preVisitEnumeration(enu : EnumerationPtr) {
        if (enu.external) {
            write(*ss, "#if 0 // external enum\n");
        }
        write(*ss, "namespace {aotModuleName(enu._module)} \{\n\n");
        write(*ss, "enum class {enu.name} : {das_to_cppString(enu.baseType)} \{\n");
    }
    def override preVisitEnumerationValue(enu : EnumerationPtr; name : das_string; value : ExpressionPtr; last : bool) {
        write(*ss, "    {name} = {das_to_cppString(enu.baseType)}(");
    }
    def override visitEnumerationValue(enu : EnumerationPtr; name : das_string; value : ExpressionPtr; last : bool) {
        write(*ss, ")");
        if (!last) { write(*ss, ","); };
        write(*ss, "\n");
        return value;
    }
    def override visitEnumeration(enu : EnumerationPtr) {
        write(*ss, "};\n")   // enum
        write(*ss, "}\n")   // namespace
        if (enu.external) {
            write(*ss, "#endif // external enum\n");
        } else {
        }
        return enu;
    }
// strcuture
    def override canVisitStructureFieldInit(st : StructurePtr) : bool {
        return false;
    }

    def override canVisitStructure(st : Structure?) {
        return !st.flags.isTemplate;     // not a thing with templates
    }
    def override preVisitStructure(that : StructurePtr) {
        if (that.flags.cppLayout) {
            write(*ss, "\n#if 0 // skipping structure {that.name} declaration due to CPP layout");
        }
        write(*ss, "namespace {aotModuleName(that._module)} \{\n");
        for (ann in that.annotations) {
            if (ann.annotation is StructureAnnotation) {
                get_struct_aot_prefix(ann.annotation as StructureAnnotation, that.get_ptr(), ann.arguments, ss);
            }
        }
        write(*ss, "\nstruct {aotStructName(that.get_ptr())}");
        if (that.flags.cppLayout && that.parent != null) {
            write(*ss, " : {aotStructName(that.parent)}");
        }
        write(*ss, " \{\n");
        for (ann in that.annotations) {
            if (ann.annotation is StructureAnnotation) {
                write_aot_body(ann.annotation as StructureAnnotation, that, ann.arguments, ss);
            }
        }
    }
    def override preVisitStructureField(that : StructurePtr; decl : FieldDeclaration; last : bool) {
        let from = find_struct_field_parent(that, string(decl.name));
        unsafe {
        if (that.flags.cppLayout && from != reinterpret<Structure const?>(that.get_ptr())) {
            write(*ss, "    /* skipping {decl.name}, from {from.name} */");
        } else {
            write(*ss, "    {describeCppType(decl._type, DescribeConfig(cross_platform=cross_platform,skip_const = true))} {decl.name};");
            if (decl.flags.parentType) {
                write(*ss, " /* from {from.name} */");
            }
        }
        }
    }
    def override visitStructureField(variable : StructurePtr; decl : FieldDeclaration; last : bool) {
        write(*ss, "\n");
    }
    def override visitStructure(that : StructurePtr) {
        write(*ss, "};\n");   // structure
        if (!cross_platform && !that.fields |> empty()) {
            let s_name = aotStructName(that.get_ptr())
            write(*ss, "static_assert(sizeof({s_name})=={that.sizeOf},\"structure size mismatch with DAS\");\n");
            for (tf in that.fields) {
                write(*ss, "static_assert(offsetof({s_name},{tf.name})=={tf.offset},\"structure field offset mismatch with DAS\");\n");
            }
        }
        for (ann in that.annotations) {
            if (ann.annotation is StructureAnnotation) {
                write_aot_suffix(ann.annotation as StructureAnnotation, that, ann.arguments, ss);
            }
        }
        write(*ss, "}\n");    // namespace
        if (that.flags.cppLayout) {
            write(*ss, "#endif // end of skipping structure {that.name} declaration due to CPP layout\n");
        }
        return that;
    }
// program body
    def override preVisitProgramBody(prog : ProgramPtr; mod : Module?) {
        // functions
        declarations = ss |> string_builder_str();
        ss |> string_builder_clear();
        write(*ss, "\n");
        prog.getThisModule |> for_each_module_function($(var fn : FunctionPtr) {
            if (!fn.flags.builtIn && !fn.flags.noAot) {
                write(*ss, "{describeCppFunc(fn,collector,cross_platform)};\n");
            }
        });
        write(*ss, "\n");
    }
// global let body
    def override preVisitGlobalLet(prog : ProgramPtr) {
        write(*ss, "void __init_script ( Context * __context__, bool __init_shared )\n\{\n");
        tab ++;
        // pre-declare locals
        assume temps = collector.localTemps[null];
        for (tmp in temps) {
            write(*ss, tabs());
            describeVarLocalCppType(ss, tmp._type, cross_platform);
            write(*ss, " {makeLocalTempName(unsafe(reinterpret<ExpressionPtr>(tmp)))};\n");
        }
    }
    def override visitGlobalLet(prog : ProgramPtr) {
        tab --;
        write(*ss, "}\n");
    }
// global
    def override preVisitGlobalLetVariable(variable : smart_ptr<Variable>; lastArg : bool) {
        write(*ss, tabs());
        if (!variable.flags.used) {
            write(*ss, "/* ");
        }
        if (variable.flags.global_shared) {
            write(*ss, "if ( __init_shared ) ");
        }
        write(*ss, "{variable.flags.global_shared ? "das_shared" : "das_global"}{variable.init != null ? "" : "_zero"}");
        if (solidContext) {
            write(*ss, "_solid");
        }
        let hash_val = solidContext ? "0x{variable.stackTop:x}" : "0x{variable.getMangledNameHash:x}";
        let cfg = DescribeConfig(skip_ref = true, skip_const = true, redundant_const = true, cross_platform = cross_platform);
        write(*ss, "<{describeCppType(variable._type,cfg)},{hash_val}>(__context__)");
    }
    def override visitGlobalLetVariable(variable : VariablePtr; last : bool) {
        write(*ss, ";");
        if (!variable.flags.used) {
            write(*ss, " */");
        }
        write(*ss, "/*{variable.name}*/\n");

        return variable;
    }
    def override preVisitGlobalLetVariableInit(variable : VariablePtr; init : ExpressionPtr) {
        write(*ss, " = ");
    }
// function
    def override canVisitFunction(fun : Function?) {
        if (fun.flags.noAot) return false;
        if (fun.moreFlags.isTemplate) return false;
        return true;
    }
    def override preVisitFunction(fn : FunctionPtr) {
        write(*ss, "\ninline ");
        describeLocalCppType(ss, fn.result, cross_platform, CpptSubstitureRef.no, CpptSkipConst.yes);
        write(*ss, " {aotFuncName(fn.get_ptr())} ( Context * __context__");
    }
    def override preVisitFunctionBody(fn : FunctionPtr; expr : ExpressionPtr) {
        if (fn.flags.aotNeedPrologue || prologue) {
            write(*ss, " ) \{ das_stack_prologue __prologue(__context__,{fn.totalStackSize:d},\"{fn.name} \" DAS_FILE_LINE);\n");
        } else {
            write(*ss, " )\n");
        }
    }
    def override preVisitFunctionArgument(fn : FunctionPtr; arg : VariablePtr; last : bool) {
        // arg
        write(*ss, ", ");
        if (isLocalVec(arg._type)) {
            describeLocalCppType(ss, arg._type, cross_platform);
        } else {
            write(*ss, describeCppType(arg._type, DescribeConfig(cross_platform = cross_platform)))
        }
        if (arg._type.isRefType) {
            write(*ss, " & ");
        }
        write(*ss, " {collector.getVarName(arg)}");
    }
    def override canVisitFunctionArgumentInit(fn : Function?; variable : VariablePtr; expr : ExpressionPtr) {
        return false;
    }
    def override preVisitFunctionArgumentInit(fn : FunctionPtr; arg : VariablePtr; expr : ExpressionPtr) {
        write(*ss, " = ");
    }
    def override visitFunctionArgument(fn : FunctionPtr; that : VariablePtr; last : bool) {
        return that;
    }
    def override visitFunction(fn : FunctionPtr) {
        if (fn.flags.aotNeedPrologue || prologue) {
            write(*ss, "}\n");
        } else {
            write(*ss, "\n");
        }
        return fn;
    }
// blk
    def makeLocalTempName(expr : ExpressionPtr) {
        // todo: fix casts to parents A -> B -> C (from A to B)
        if (!local_temp_names |> key_exists(expr.get_ptr())) {
            local_temp_names |> insert(expr.get_ptr(), local_temp_names |> length());
        }
        /*if (isMakeLocal(expr)) {
            stackTop = (expr as ExprMakeLocal).stackTop;
        }*/
        if (expr is ExprMakeLocal ||
            expr is ExprMakeStruct ||
            expr is ExprMakeVariant ||
            expr is ExprMakeArray ||
            expr is ExprMakeTuple ||
            expr is ExprCall) {
        } else {
            panic("we should not be here. we need stacktop for the name");
        }
        return "_temp_make_local_{expr.at.line:d}_{expr.at.column:d}_{local_temp_names |> get_value(expr.get_ptr()):d}";
    }
    def override preVisitExprBlock(var blk : smart_ptr<ExprBlock>) {
        push(scopes, blk as ExprBlock);
        blk.blockFlags |= ExprBlockFlags.finallyBeforeBody;
        if (blk.blockFlags.inTheLoop) {
            blk.blockFlags |= ExprBlockFlags.finallyDisabled;
        }
        write(*ss, "\{\n");
        tab ++;
        // pre-declare variables
        assume vars = collector.variables[blk.get_ptr()];
        for (variable in vars) {
            write(*ss, "{tabs()}");
            describeVarLocalCppType(ss, variable._type, cross_platform);
            var vname = collector.getVarName(unsafe(reinterpret<VariablePtr>(variable)));
            if (variable._type.flags.constant && variable._type.isRefType && !variable._type.flags.ref) {
                vname += "_ConstRef";
            }
            write(*ss, " {vname}");
            write(*ss, "; memset((void*)&{vname},0,sizeof({vname}));\n");
        }
        // pre-declare locals
        collector.localTemps.get(blk.get_ptr()) <| $(temps) {
            for (tmp in temps) {
                write(*ss, tabs());
                describeVarLocalCppType(ss, tmp._type, cross_platform);
                let tempName = makeLocalTempName(unsafe(reinterpret<ExpressionPtr>(tmp)));
                write(*ss, " {tempName}; {tempName};\n");
            }
        }
    }
    def override preVisitExprBlockArgumentInit(blk : smart_ptr<ExprBlock>; variable : VariablePtr; init : ExpressionPtr) {
        write(*ss, "\n#if 0\n");
    }
    def override visitExprBlockArgumentInit(blk : smart_ptr<ExprBlock>; variable : VariablePtr; init : ExpressionPtr) {
        write(*ss, "\n#endif\n");
        return init;
    }
    def override preVisitExprBlockExpression(blk : smart_ptr<ExprBlock>; expr : ExpressionPtr) {
        write(*ss, tabs());
    }
    def override visitExprBlockExpression(blk : smart_ptr<ExprBlock>; that : ExpressionPtr) {
        write(*ss, ";")
        newLine();
        return that;
    }
    def override visitExprBlock(var blk : smart_ptr<ExprBlock>) : ExpressionPtr {
        tab --;
        write(*ss, "{tabs()}\}")
        blk.blockFlags &= ~ExprBlockFlags.finallyBeforeBody
        blk.blockFlags &= ~ExprBlockFlags.finallyDisabled
        scopes |> pop();
        return blk;
    }
    def finallyName(blk : smart_ptr<ExprBlock>) {
        return "__finally_{blk.at.line:d}";
    }
    def override preVisitExprBlockFinal(blk : smart_ptr<ExprBlock>) {
        write(*ss, "{tabs()}/* finally */ auto {finallyName(blk)}= das_finally([&]()\{\n");
    }
    def override preVisitExprBlockFinalExpression(blk : smart_ptr<ExprBlock>; expr : ExpressionPtr) {
        write(*ss, "{tabs()}");
    }
    def override visitExprBlockFinalExpression(blk : smart_ptr<ExprBlock>; that : ExpressionPtr) {
        write(*ss, ";");
        newLine();
        return that;
    }
    def override visitExprBlockFinal(blk : smart_ptr<ExprBlock>) {
        write(*ss, "{tabs()}/* end finally */ \});\n");
    }
// let
    def override preVisitExprLetVariable(let_ : smart_ptr<ExprLet>; var variable : VariablePtr; last : bool) {
        let vname = collector.getVarName(variable);
        if (variable.init != null && variable.init is ExprMakeBlock) {
            var mkb = variable.init as ExprMakeBlock;
            var blk = mkb._block as ExprBlock;
            blk.blockFlags |= ExprBlockFlags.aotSkipMakeBlock;
            write(*ss, "auto {vname}_TempFunctor = ");
            visit_expression(variable.init, adapter);
            write(*ss, ";\n{tabs()}");
            blk.blockFlags &= ~ExprBlockFlags.aotSkipMakeBlock;
            mkb.aotFunctorName := vname + "_TempFunctor";
        }
        if (!collector.isMoved(variable)) {
            describeVarLocalCppType(ss, variable._type, cross_platform);
            write(*ss, " ");
        }
        var cvname = vname;
        if (variable._type.flags.constant && variable._type.isRefType && !variable._type.flags.ref) {
            cvname += "_ConstRef";
        }
        if (variable.init != null) {
            write(*ss, cvname);
        } elif (variable._type.canInitWithZero) {
            let init_val = isLocalVec(variable._type) ? "v_zero()" : "0";
            write(*ss, "{cvname} = {init_val}");
        } else {
            if (!collector.isMoved(variable)) {
                write(*ss, "{cvname};");
            }
            write(*ss, "das_zero({cvname})");
        }
    }
    def override visitExprLetVariable(let_ : smart_ptr<ExprLet>; variable : VariablePtr; last : bool) {
        if (!last) write(*ss, "; ");
        if (variable._type.flags.constant && variable._type.isRefType && !variable._type.flags.ref) {
            let vname = collector.getVarName(variable);
            write(*ss, ";\n    ");
            describeLocalCppType(ss, variable._type, cross_platform);
            write(*ss, " & {vname} = {vname}_ConstRef; ");
        }
        return variable;
    }
    def override preVisitExprLetVariableInit(let_ : smart_ptr<ExprLet>; variable : VariablePtr; expr : ExpressionPtr) {
        if (variable.flags.init_via_move && variable.init is ExprMakeBlock) {
            write(*ss, " = ");
        } elif (variable.flags.init_via_move) {
            let vname = collector.getVarName(variable);
            var cvname = vname;
            if (variable._type.flags.constant && variable._type.isRefType) {
                cvname += "_ConstRef";
            }
            write(*ss, "; das_zero({cvname}); das_move({cvname}, ");
        } else {
            write(*ss, " = ");
        }
        if (variable._type.flags.constant) {
            if (!variable._type.isGoodBlockType) {
                write(*ss, "((");
                describeVarLocalCppType(ss, variable._type, cross_platform);
                write(*ss, ")");
            } else {
                write(*ss, "(");
            }
        }
        if (!expr._type.isPointer && !variable._type.flags.ref && expr._type.isAotAlias && !variable._type.isAotAlias) {
            if (expr._type.alias.empty()) {
                let cfg = DescribeConfig(skip_ref = true, redundant_const = true, cross_platform = cross_platform)
                let type_str = describeCppTypeEx(expr._type, cfg, CpptUseAlias.yes);
                write(*ss, "das_reinterpret<{type_str}>::pass(")
            } else {
                write(*ss, "das_alias<{expr._type.alias}>::from(");
            }
        }
        if (variable._type.flags.ref) {
            write(*ss, "&(");
        }
        if (needPtrCast(variable._type, expr._type, expr)) {
            write(*ss, "das_auto_cast<{describeCppType(variable._type, DescribeConfig(cross_platform=cross_platform))}>::cast(");
        }
        if (expr._type.isString) {
            let maybeRef = variable._type.flags.ref ? " &" : "";
            write(*ss, "(char *{maybeRef})(");
        }
    }
    def override visitExprLetVariableInit(let_ : smart_ptr<ExprLet>; variable : VariablePtr; expr : ExpressionPtr) {
        if (expr._type.isString) {
            write(*ss, ")");
        }
        if (needPtrCast(variable._type, expr._type, expr)) {
            write(*ss, ")");
        }
        if (variable._type.flags.ref) {
            write(*ss, ")");
        }
        if (!expr._type.isPointer && !variable._type.flags.ref && expr._type.isAotAlias && !variable._type.isAotAlias) {
            write(*ss, ")");
        }
        if (variable.flags.init_via_move && variable.init is ExprMakeBlock) {
            /* nothing. this is let a <- $ { ... } */
        } elif (variable.flags.init_via_move) {
            write(*ss, ")");
        }
        if (variable._type.flags.constant) {
            write(*ss, ")");
        }
        return expr;
    }
// label
    def override preVisitExprLabel(that : smart_ptr<ExprLabel>) {
        write(*ss, "label_{that.labelName}:;");
    }
// goto
    def override preVisitExprGoto(that : smart_ptr<ExprGoto>) {
        if (that.subexpr == null) {
            write(*ss, "goto label_{that.labelName}");
        } else {
            write(*ss, "switch (");
        }
    }
    def override visitExprGoto(var that : smart_ptr<ExprGoto>) : ExpressionPtr {
        if (that.subexpr != null) {
            write(*ss, ") \{\n");
            scopes |> reverse();
            for (blk in scopes) {
                for (ex in blk.list) {
                    if (ex is ExprLabel) {
                        let lab = ex as ExprLabel;
                        write(*ss, "{tabs()}case {lab.labelName}: goto label_{lab.labelName};\n");
                    }
                }
            }
            scopes |> reverse();
            write(*ss, "{tabs()}default: __context__->throw_error(\"invalid label\");\n");
            write(*ss, "{tabs()}\}");
        }
        return that;
    }
// copy
    def override preVisitExprCopy(that : smart_ptr<ExprCopy>) {
        write(*ss, "das_copy(");
    }
    def override preVisitExprCopyRight(that : smart_ptr<ExprCopy>; right : ExpressionPtr) {
        write(*ss, ",");
    }
    def override visitExprCopy(var that : smart_ptr<ExprCopy>) : ExpressionPtr {
        write(*ss, ")");
        return that;
    }
// clone
    def override preVisitExprClone(that : smart_ptr<ExprClone>) {
        let cfg = DescribeConfig(skip_ref = true, skip_const = true, cross_platform = cross_platform)
        let cfg2 := cfg
        let left_t = describeCppType(that.left._type, cfg);
        let right_t = describeCppType(that.right._type, cfg2);
        write(*ss, "das_clone<{left_t},{right_t}>::clone(");
    }
    def override preVisitExprCloneRight(that : smart_ptr<ExprClone>; right : ExpressionPtr) {
        write(*ss, ",");
    }
    def override visitExprClone(var that : smart_ptr<ExprClone>) : ExpressionPtr {
        write(*ss, ")");
        return that;
    }
// move
    def override preVisitExprMove(that : smart_ptr<ExprMove>) {
        write(*ss, "das_move(");
    }
    def override preVisitExprMoveRight(that : smart_ptr<ExprMove>; right : ExpressionPtr) {
        write(*ss, ",");
    }
    def override visitExprMove(var that : smart_ptr<ExprMove>) : ExpressionPtr {
        write(*ss, ")");
        return that;
    }
// op1
    def outPolicy(decl : TypeDeclPtr) {
        if (decl.baseType != Type.tHandle){
            write(*ss, "SimPolicy<{das_to_cppString(decl.baseType)}>");
        } else {
            write(*ss, "SimPolicy<{decl.annotation.cppName.empty() ? decl.annotation.name : decl.annotation.cppName}>");
        }
    }
    def isOpPolicy1(that : smart_ptr<ExprOp1>) {
        if (is_alpha(character_at(string(that.op), 0))) {
            return true;
        }
        return that.subexpr._type.isPolicyType;
    }
    def override preVisitExprOp1(var that : smart_ptr<ExprOp1>) {
        if (!that.func.flags.builtIn || that.func.flags.callBased) {
            that.arguments |> clear();
            var subexpr := that.subexpr
            that.arguments |> emplace(subexpr);
            CallFunc_preVisit(that);
            CallFunc_preVisitCallArg(that, that.subexpr, true);
        } elif (isOpPolicy1(that)) {
            outPolicy(that.subexpr._type);
            write(*ss, "::{opPolicyName(that)}(");
        } else {
            if (that.op != "+++" && that.op != "---") {
                write(*ss, that.op);
            }
            if (!noBracket(that) && !that.subexpr.printFlags.bottomLevel) {
                write(*ss, "(");
            }
        }
    }
    def override visitExprOp1(var that : smart_ptr<ExprOp1>) : ExpressionPtr {
        if (!that.func.flags.builtIn || that.func.flags.callBased) {
            CallFunc_visitCallArg(that, that.subexpr, true);
            CallFunc_visit(that);
            that.arguments.clear();
        } elif (isOpPolicy1(that)){
            write(*ss, ",*__context__,nullptr)");
        } else {
            if (that.op == "+++" || that.op == "---") {
                *ss |> write_char(character_at(string(that.op), 0))
                *ss |> write_char(character_at(string(that.op), 1));
            }
            if (!noBracket(that) && !that.subexpr.printFlags.bottomLevel) {
                write(*ss, ")");
            }
        }
        return that;
    }
// op2
    def isSetBool(that : smart_ptr<ExprOp2>) {
        return ((that.op == "||=" || that.op == "&&=" || that.op == "^^=") &&
                that.right._type.baseType == Type.tBool && that.right._type.isSimpleType);
    }
    def isOpPolicy2(that : smart_ptr<ExprOp2>) {
        if (is_alpha(character_at(string(that.op), 0))) return true;
        if (that.op == "/" || that.op == "%") return true;
        if (that.op == "<<<" || that.op == ">>>" || that.op == "<<<=" || that.op == ">>>=") return true;
        return that._type.isPolicyType || that.left._type.isPolicyType || that.right._type.isPolicyType;
    }
    def opPolicyBase(that : smart_ptr<ExprOp2>) {
        if (that._type.isPolicyType) {
            return that._type;
        } elif (that.left._type.isPolicyType) {
            return that.left._type;
        } else {
            return that.right._type;
        }
    }
    def opPolicyName(that : smart_ptr<ExprOp>) {
        unsafe {
            assume fn = (reinterpret<ExprCallFunc?>(that.get_ptr())).func;
            if (fn.flags.builtIn) {
                let bfn = fn as BuiltInFunction;
                return string(bfn.cppName.empty() ? bfn.name : bfn.cppName);
            } else {
                return "/* NotAPolicy */";
            }
        }
    }
    def isRefPolicyOp(th : smart_ptr<ExprOp2>) {
        let op = string(th.op);
        unsafe {
            return (["+=", "-=", "*=", "/=", "%=",// math
                    "&=", "|=", "^=",         // bin
                    "&&=", "||=", "^^=",      // bool
                    "<<=", ">>=", "<<<=", ">>>="// rotational
                   ] |> each()
                     |> map(@(s : string) { return s == op; })
                     |> any())
        }
    }

    def override preVisitExprOp2(var that : smart_ptr<ExprOp2>) {
        if (!noBracket(that)) write(*ss, "(");
        if (!that.func.flags.builtIn || that.func.flags.callBased) {
            that.arguments |> clear();
            var left := that.left;
            var right := that.right;
            that.arguments |> emplace(left);
            that.arguments |> emplace(right);
            CallFunc_preVisit(that);
            CallFunc_preVisitCallArg(that, that.left, false);
        } elif (isOpPolicy2(that)) {
            assume pt = opPolicyBase(that);
            if (policyResultNeedCast(pt, that._type)) {
                let cfg = DescribeConfig(skip_ref = true, skip_const = true, cross_platform = cross_platform)
                let type_str = describeCppType(that._type, cfg);
                write(*ss, "cast<{type_str}>::to(");
            }
            outPolicy(pt);
            write(*ss, "::{opPolicyName(that)}(");
            if (isRefPolicyOp(that) && (pt.isVectorType || pt.isString)) {
                write(*ss, "(char *)&(");
            } elif (policyArgNeedCast(pt, that.left._type)) {
                let cfg = DescribeConfig(skip_ref = true, skip_const = true, cross_platform = cross_platform)
                let type_str = describeCppType(that.left._type, cfg);
                if (that.left._type.isRefType) {
                    write(*ss, "cast<{type_str}*>::from(&(");
                } else {
                    write(*ss, "cast<{type_str}>::from(");
                }
            }
        } elif (isSetBool(that)) {
            if (that.op == "||=") {
                write(*ss, "DAS_SETBOOLOR((");
            } elif (that.op == "&&=") {
                write(*ss, "DAS_SETBOOLAND((");
            } elif (that.op == "^^=") {
                write(*ss, "DAS_SETBOOLXOR((");
            }
        }
    }
    def override preVisitExprOp2Right(that : smart_ptr<ExprOp2>; right : ExpressionPtr) {
        if (!that.func.flags.builtIn || that.func.flags.callBased) {
            CallFunc_visitCallArg(that, that.left, false);
            CallFunc_preVisitCallArg(that, that.right, true);
        } elif (isOpPolicy2(that)) {
            assume pt = opPolicyBase(that);
            if (isRefPolicyOp(that) && (pt.isVectorType || pt.isString)) {
                write(*ss, ")");
            } elif (policyArgNeedCast(pt, that.left._type)) {
                if (that.left._type.isRefType) {
                    write(*ss, "))");
                } else {
                    write(*ss, ")");
                }
            }
            write(*ss, ",");
            if (policyArgNeedCast(pt, that.right._type)) {
                let cfg = DescribeConfig(skip_ref = true, skip_const = true, cross_platform = cross_platform)
                let right_type = describeCppType(that.right._type, cfg);
                if (that.right._type.isRefType) {
                    write(*ss, "cast<{right_type} *>::from(&(");
                } else {
                    write(*ss, "cast<{right_type}>::from(");
                }
            }
        } elif (isSetBool(that)) {
            write(*ss, "),(");
        } else {
            if (that._type.baseType == Type.tBool) {
                var op = "";
                match (string(that.op)) {
                    if ("&") { op = "&&"; }
                    if ("|") { op = "||"; }
                    if ("^" || "^^") { op = "!="; }
                    if (_) { op = string(that.op); }
                }
                write(*ss, " {op} ");
            } else {
                write(*ss, " {that.op} ");
            }
        }
    }
    def override visitExprOp2(var that : smart_ptr<ExprOp2>) : ExpressionPtr {
        if (!that.func.flags.builtIn || that.func.flags.callBased) {
            CallFunc_visitCallArg(that, that.right, true);
            CallFunc_visit(that);
            // todo: fails in soa.das that.arguments |> clear();
        } elif (isOpPolicy2(that)) {
            assume pt = opPolicyBase(that);
            if (policyArgNeedCast(pt, that.right._type)) {
                if (that.right._type.isRefType) {
                    write(*ss, "))");
                } else {
                    write(*ss, ")");
                }
            }
            write(*ss, ",*__context__,nullptr)");
            if (policyResultNeedCast(pt, that._type)) {
                write(*ss, ")");
            }
        } elif (isSetBool(that)) {
            write(*ss, "))");
        }
        if (!noBracket(that)) {
            write(*ss, ")");
        }
        return that;
    }
// op3
    def override preVisitExprOp3(that : smart_ptr<ExprOp3>) {
        if (!noBracket(that)) {
            write(*ss, "(");
        }
    }
    def override preVisitExprOp3Left(that : smart_ptr<ExprOp3>; left : ExpressionPtr) {
        write(*ss, " ? ");
        assume argT = left._type;
        if (isLocalVec(argT)) {
            write(*ss, "(vec4f)");
        }
        if (that._type.isRef) {
            write(*ss, "das_auto_cast_ref");
        } else {
            write(*ss, "das_auto_cast");
        }
        write(*ss, "<{describeCppType(that._type, DescribeConfig(cross_platform=cross_platform))}>::cast(");
    }
    def override preVisitExprOp3Right(that : smart_ptr<ExprOp3>; right : ExpressionPtr) {
        write(*ss, ") : ");
        if (isLocalVec(right._type)) {
            write(*ss, "(vec4f)");
        }
        if (that._type.isRef) {
            write(*ss, "das_auto_cast_ref");
        } else {
            write(*ss, "das_auto_cast");
        }
        write(*ss, "<{describeCppType(that._type, DescribeConfig(cross_platform=cross_platform))}>::cast(");
    }
    def override visitExprOp3(var that : smart_ptr<ExprOp3>) : ExpressionPtr {
        write(*ss, ")");
        if (!noBracket(that)) {
            write(*ss, ")");
        }
        return that;
    }
// return
    def override preVisitExprReturn(expr : smart_ptr<ExprReturn>) {
        write(*ss, "return ");
        if (expr.returnFlags.moveSemantics) {
            write(*ss, "/* <- */ ");
        }
        assume retT = expr.returnFunc != null ? expr.returnFunc.result : expr._block.returnType;
        if (!retT.isVoid) {
            if (expr.returnFlags.moveSemantics) {
                write(*ss, "das_auto_cast_move");
            } else {
                if (retT.isRef) {
                    write(*ss, "das_auto_cast_ref");
                } else {
                    write(*ss, "das_auto_cast");
                }
            }
            write(*ss, "<{describeCppType(retT, DescribeConfig(skip_const=true, cross_platform=cross_platform))}>::cast(");
        }
    }
    def override visitExprReturn(var expr : smart_ptr<ExprReturn>) : ExpressionPtr {
        assume retT = expr.returnFunc != null ? expr.returnFunc.result : expr._block.returnType;
        if (!retT.isVoid) {
            write(*ss, ")");
        }
        return expr;
    }
// break
    def override preVisitExprBreak(that : smart_ptr<ExprBreak>) {
        write(*ss, "break");
    }
// continue
    def override preVisitExprContinue(that : smart_ptr<ExprContinue>) {
        write(*ss, "continue");
    }
// var
    def override preVisitExprVar(variable : smart_ptr<ExprVar>) {
        if (variable._type.flags.aotAlias) {
            write(*ss, "das_alias<{variable._type.alias}>::from(");
        }
    }
    def override visitExprVar(var variable : smart_ptr<ExprVar>) : ExpressionPtr {
        if (variable.varFlags.local && variable.variable._type.flags.ref) {
            write(*ss, "(*{collector.getVarName(variable.variable)})");
        } elif (variable.varFlags.local || variable.pBlock != null || variable.varFlags.argument) {
            write(*ss, "{collector.getVarName(variable.variable)}");
        } else {
            write(*ss, (variable.variable.flags.global_shared ? "das_shared" : "das_global"));
            let type_str = describeCppType(variable.variable._type, DescribeConfig(skip_ref = true, skip_const = true, cross_platform = cross_platform));
            write(*ss, "<{type_str},0x{variable.variable.getMangledNameHash:x}>(__context__) /*{variable.name}*/");
        }
        if (variable._type.flags.aotAlias) {
            write(*ss, ")");
        }
        return variable;
    }
// null coaelescing
    def override preVisitExprNullCoalescing(nc : smart_ptr<ExprNullCoalescing>) {
        if (nc._type.flags.aotAlias) {
            write(*ss, "das_alias<{nc._type.alias}>::from(");
        }
        let type_str1 = describeCppType(nc.defaultValue._type, DescribeConfig(cross_platform = cross_platform));
        write(*ss, "das_null_coalescing<{type_str1}>::get(");
        if (nc.subexpr._type.isAotAlias) {
            let type_str2 = describeCppType(nc.defaultValue._type, DescribeConfig(cross_platform = cross_platform));
            write(*ss, "({type_str2} *)");
        }
    }
    def override preVisitExprNullCoalescingDefault(nc : smart_ptr<ExprNullCoalescing>; expr : ExpressionPtr) {
        write(*ss, ",");
    }
    def override visitExprNullCoalescing(var nc : smart_ptr<ExprNullCoalescing>) : ExpressionPtr {
        write(*ss, ")");
        if (nc._type.flags.aotAlias) {
            write(*ss, ")");
        }
        return nc;
    }
// is variant
    def stringify_variadic_types(types : dasvector`smart_ptr`TypeDecl) {
        unsafe {
            return (types |> each()
                                   |> map(@(t : TypeDeclPtr) : string {
                                       return describeCppTypeEx(t,
                                                                DescribeConfig(redundant_const = true,
                                                                               cross_platform = cross_platform),
                                                                CpptUseAlias.no);
                                   })
                                   |> join(","))
       }
    }

    def get_variant_field(field_type : TypeDeclPtr; index : int; is_pointer : bool = false) {
        let type_str = describeCppType(field_type.argTypes[index], DescribeConfig(cross_platform = cross_platform));
        let maybe_ptr = is_pointer ? "_ptr" : "";
        if (cross_platform) {
            return "das_get_auto_variant_field{maybe_ptr}<{type_str},{index},{stringify_variadic_types(field_type.argTypes)}>";
        } else {
            let offset = field_type.get_variant_field_offset(index);
            return "das_get_variant_field{maybe_ptr}<{type_str},{offset},{index}>";
        }
    }

    def override preVisitExprIsVariant(field : smart_ptr<ExprIsVariant>) {
        assume var_type = field.value._type;
        write(*ss, "{get_variant_field(var_type, field.fieldIndex)}::is(")
    }
    def override visitExprIsVariant(var field : smart_ptr<ExprIsVariant>) : ExpressionPtr {
        write(*ss, ")");
        return field;
    }
// as variant
    def override preVisitExprAsVariant(field : smart_ptr<ExprAsVariant>) {
        assume var_type = field.value._type;
        if (field._type.flags.aotAlias) {
            write(*ss, "das_alias<{field._type.alias}>::from(");
        }
        write(*ss, "{get_variant_field(var_type, field.fieldIndex)}::as(");
    }
    def override visitExprAsVariant(var field : smart_ptr<ExprAsVariant>) : ExpressionPtr {
        write(*ss, ",__context__)");
        if (field._type.flags.aotAlias) {
            write(*ss, ")");
        }
        return field;
    }
// safe as variant
    def override preVisitExprSafeAsVariant(field : smart_ptr<ExprSafeAsVariant>) {
        assume fieldT = field.value._type.isPointer ? field.value._type.firstType :  field.value._type;
        if (fieldT.flags.aotAlias) {
            write(*ss, "das_alias<{fieldT.alias}>::from(");
        }
        write(*ss, "{get_variant_field(fieldT, field.fieldIndex)}::safe_as{(field.skipQQ ? "_ptr" : "")}(")
    }
    def override visitExprSafeAsVariant(var field : smart_ptr<ExprSafeAsVariant>) : ExpressionPtr {
        write(*ss, ")");
        assume fieldT = field.value._type.isPointer ? field.value._type.firstType :  field.value._type;
        if (fieldT.flags.aotAlias) {
            write(*ss, ")");
        }
        return field;
    }
// safe field
    def override preVisitExprSafeField(field : smart_ptr<ExprSafeField>) {
        if (field._type.flags.aotAlias) {
            write(*ss, "das_alias<{field._type.alias}>::from(");
        }
        assume vtype = field.value._type.firstType;
        write(*ss, (vtype.isHandle ? "das_safe_navigation_handle" : "das_safe_navigation"));
        if (vtype.isGoodTupleType) { write(*ss, "_tuple"); }
        elif (vtype.isGoodVariantType) { write(*ss, "_variant"); }
        elif (field.skipQQ) { write(*ss, "_ptr"); }
        write(*ss, "<");
        if (!vtype.isGoodTupleType && !vtype.isGoodVariantType) {
            write(*ss, "{describeCppType(field.value._type.firstType, DescribeConfig(cross_platform=cross_platform))},");
        }
        if (field.skipQQ) {
            write(*ss, "{describeCppType(field._type, DescribeConfig(cross_platform=cross_platform))}");
        } else {
            write(*ss, "{describeCppType(field._type.firstType, DescribeConfig(cross_platform=cross_platform))}");
        }
        if (vtype.isHandle) {
            write(*ss, ">::get(");
        } elif (vtype.isGoodTupleType) {
            if (cross_platform) {
                panic("Platform independent code enabled. But field {field.name} is tuple")
            }
            write(*ss, ", {vtype.get_tuple_field_offset(field.fieldIndex)}>::get(");
        } elif (vtype.isGoodVariantType) {
            if (cross_platform) {
                panic("Platform independent code enabled. But field {field.name} is tuple")
            }
            write(*ss, ", {vtype.get_variant_field_offset(field.fieldIndex)}, {field.fieldIndex}>::get(");
        } else {
            let mod_name = (vtype.structType._module.name.empty() ? "" : string(vtype.structType._module.name) + "::");
            write(*ss, ",&{mod_name}{vtype.structType.name}::{field.name}>::get(");
        }
    }
    def override visitExprSafeField(var field : smart_ptr<ExprSafeField>) : ExpressionPtr {
        assume vtype = field.value._type.firstType;
        if (vtype.isHandle) {
            let type_str = describeCppType(vtype, DescribeConfig(skip_const = true, redundant_const = true, cross_platform = cross_platform));
            write(*ss, ",([&](const {type_str} * __any) -> auto & \{return ");
            aot_previsit_get_field_ptr(vtype.annotation, ss, string(field.name));
            write(*ss, "__any");
            aot_type_ann_get_field_ptr(vtype.annotation, ss, string(field.name));
            write(*ss, " /*{field.name}*/");
            write(*ss, ";\})");
        }
        write(*ss, ")");
        if (field._type.flags.aotAlias) {
            write(*ss, ")");
        }
        return field;
    }
// field
    def get_tuple_field(field_type : TypeDeclPtr; index : int; is_pointer : bool = false) {
        let field_tp = describeCppType(field_type.argTypes[index], DescribeConfig(cross_platform = cross_platform));
        let maybe_ptr = is_pointer ? "_ptr" : "";
        if (cross_platform) {
            return "das_get_auto_tuple_field{maybe_ptr}<{field_tp},{index},{stringify_variadic_types(field_type.argTypes)}>";
        } else {
            let field_offset = field_type.get_tuple_field_offset(index);
            return "das_get_tuple_field{maybe_ptr}<{field_tp},{field_offset}>";
        }
    }

    def override preVisitExprField(field : smart_ptr<ExprField>) {
        if (field._type.flags.aotAlias) {
            write(*ss, "das_alias<{field._type.alias}>::from(");
        }
        assume field_type = field.value._type;
        if (field_type.isBitfield) {
            write(*ss, "das_get_bitfield(");
        } elif (field_type.isTuple) {
            write(*ss, "{get_tuple_field(field_type, field.fieldIndex)}::get(")
        } elif (field_type.isVariant) {
            write(*ss, "{get_variant_field(field_type, field.fieldIndex)}::get(")
        } elif (field_type.isHandle) {
            if (field._type.isString) {
                write(*ss, "(({describeCppType(field._type, DescribeConfig(cross_platform=cross_platform))})(");  // c-cast const char * etc string casts to char * or char * const
            }
            aot_previsit_get_field(field_type.annotation, ss, string(field.name));
        } elif (field_type.baseType == Type.tPointer) {
            if (field_type.firstType.isHandle) {
                aot_previsit_get_field_ptr(field_type.firstType.annotation, ss, string(field.name));
            } elif (field.value._type.firstType.isTuple) {
                write(*ss, "{get_tuple_field(field_type.firstType, field.fieldIndex, true)}::get(")
            } elif (field.value._type.firstType.isVariant) {
                write(*ss, "{get_variant_field(field_type.firstType, field.fieldIndex, true)}::get(")
            }
        }
    }
    def override visitExprField(var field : smart_ptr<ExprField>) : ExpressionPtr {
        if (field.value._type.isBitfield) {
            write(*ss, ",1u << {field.fieldIndex})");
        } elif (field.value._type.isTuple) {
            write(*ss, ")");
        } elif (field.value._type.isVariant) {
            write(*ss, ")");
        } elif (field.value._type.isHandle) {
            aot_visit_get_field(field.value._type.annotation, ss, string(field.name));
            write(*ss, " /*{field.name}*/");
            if (field._type.isString) {
                write(*ss, "))");
            }
        } elif (field.value._type.baseType == Type.tPointer) {
            if (field.value._type.firstType.isHandle) {
                aot_type_ann_get_field_ptr(field.value._type.firstType.annotation, ss, string(field.name));
                write(*ss, " /*{field.name}*/");
            } elif (field.value._type.firstType.isTuple) {
                write(*ss, ")");
            } elif (field.value._type.firstType.isVariant) {
                write(*ss, ")");
            } else {
                write(*ss, "->{field.name}");
            }
        } else {
            write(*ss, ".{field.name}");
        }
        if (field._type.flags.aotAlias) {
            write(*ss, ")");
        }
        return field;
    }
// at
    def override preVisitExprAt(expr : smart_ptr<ExprAt>) {
        if (expr._type.flags.aotAlias) {
            write(*ss, "das_alias<{expr._type.alias}>::from(");
        }
        if (!(!expr.subexpr._type.dim |> empty() || expr.subexpr._type.isGoodArrayType || expr.subexpr._type.isGoodTableType)) {
            let type_str = describeCppType(expr.subexpr._type, DescribeConfig(skip_ref = true, cross_platform = cross_platform));
            write(*ss, "das_index<{type_str}>::at(");
        }
    }
    def override preVisitExprAtIndex(var expr : smart_ptr<ExprAt>; index : ExpressionPtr) {
        if (!expr.subexpr._type.dim |> empty() || expr.subexpr._type.isGoodArrayType || expr.subexpr._type.isGoodTableType) {
            if (expr.subexpr._type.flags.isNativeDim) {
                write(*ss, "[");
            } else {
                write(*ss, "(");
            }
        } else {
            write(*ss, ",");
        }

    }
    def override visitExprAt(var expr : smart_ptr<ExprAt>) : ExpressionPtr {
        if (expr.subexpr._type.flags.isNativeDim) {
            write(*ss, "]");
        } else {
            write(*ss, ",__context__)");
        }
        if (expr._type.flags.aotAlias) {
            write(*ss, ")");
        }
        return expr;
    }
// safe at
    def override preVisitExprSafeAt(expr : smart_ptr<ExprSafeAt>) {
        let isPtr : bool = expr.subexpr._type.isPointer;
        assume seT = isPtr ? expr.subexpr._type.firstType : expr.subexpr._type;
        if ((!seT.dim |> empty() || seT.isGoodArrayType || seT.isGoodTableType)) {
            let type_str = describeCppType(seT, DescribeConfig(skip_ref = true, skip_const = true, cross_platform = cross_platform))
            write(*ss, "{type_str}::safe_index(");
        } else {
            let type_str = describeCppType(seT, DescribeConfig(skip_ref = true, cross_platform = cross_platform))
            write(*ss, "das_index<{type_str}>::safe_at(");
        }
        write(*ss, isPtr ? "(" : "&(");
    }
    def override preVisitExprSafeAtIndex(expr : smart_ptr<ExprSafeAt>; index : ExpressionPtr) {
        write(*ss, "),");
    }
    def override visitExprSafeAt(var that : smart_ptr<ExprSafeAt>) : ExpressionPtr {
        write(*ss, ",__context__)");
        return that;
    }
// const
    def override visitExprFakeContext(var c : smart_ptr<ExprFakeContext>) : ExpressionPtr {
        write(*ss, "__context__");
        return c;
    }
    def override visitExprFakeLineInfo(var c : smart_ptr<ExprFakeLineInfo>) : ExpressionPtr {
        write(*ss, "((LineInfoArg *)(&LineInfo::g_LineInfoNULL))");
        return c;
    }
    def override visitExprConstPtr(var c : smart_ptr<ExprConstPtr>) : ExpressionPtr {
        if (c.getValue != null) {
            unsafe {write(*ss, "((void *) 0x{(reinterpret<int64>(c.getValue)):x})"); }
        } else {
            write(*ss, "nullptr");
        }
        return c;
    }
    def override visitExprConstEnumeration(var c : smart_ptr<ExprConstEnumeration>) : ExpressionPtr {
        let cfg = DescribeConfig(skip_ref = true, skip_const = true, redundant_const = false, cross_platform = cross_platform);
        let type_str = describeCppType(c._type, cfg);
        write(*ss, type_str);
        var ctext = string(c.value);
        for (ee in c.enumType.list) {
            if (ee.name == c.value) {
                if (!ee.cppName.empty()) {
                    ctext = string(ee.cppName);
                }
                break;
            }
        }
        write(*ss, "::{ctext}");
        return c;
    }
    def override visitExprConstInt(var c : smart_ptr<ExprConstInt>) : ExpressionPtr {
        write(*ss, "{c.getValue}");
        return c;
    }
    def override visitExprConstInt8(var c : smart_ptr<ExprConstInt8>) : ExpressionPtr {
        write(*ss, "{c.getValue}");
        return c;
    }
    def override visitExprConstInt16(var c : smart_ptr<ExprConstInt16>) : ExpressionPtr {
        write(*ss, "{c.getValue}");
        return c;
    }
    def override visitExprConstInt64(var c : smart_ptr<ExprConstInt64>) : ExpressionPtr{
        if (c.getValue == LONG_MIN) {
            write(*ss, "INT64_MIN"); // silence overflow warning
        } else {
            write(*ss, "INT64_C({c.getValue})");
        }
        return c;
    }
    def override visitExprConstUInt8(var c : smart_ptr<ExprConstUInt8>) : ExpressionPtr {
        write(*ss, "0x{uint(c.getValue):x}");
        return c;
    }
    def override visitExprConstUInt16(var c : smart_ptr<ExprConstUInt16>) : ExpressionPtr {
        write(*ss, "0x{uint(c.getValue):x}");
        return c;
    }
    def override visitExprConstUInt64(var c : smart_ptr<ExprConstUInt64>) : ExpressionPtr {
        write(*ss, "UINT64_C(0x{c.getValue:x})");
        return c;
    }
    def override visitExprConstUInt(var c : smart_ptr<ExprConstUInt>) : ExpressionPtr {
        write(*ss, "0x{c.getValue:x}u");
        return c;
    }
    def override visitExprConstBitfield(var c : smart_ptr<ExprConstBitfield>) : ExpressionPtr {
        write(*ss, "0x{c.getValue:x}u");
        return c;
    }
    def override visitExprConstBool(var c : smart_ptr<ExprConstBool>) : ExpressionPtr {
        write(*ss, (c.getValue ? "true" : "false"));
        return c;
    }

    def to_cpp_double(val : double) {
        return "{val:e}";
    }

    def writeOutDouble(val : double) {
        write(*ss, to_cpp_double(val));
    }
    def override visitExprConstDouble(var c : smart_ptr<ExprConstDouble>) : ExpressionPtr {
        writeOutDouble(c.getValue);
        return c;
    }
    def writeOutFloat(value : float) {
        write(*ss, to_cpp_float(value));
    }
    def override visitExprConstFloat(var c : smart_ptr<ExprConstFloat>) : ExpressionPtr {
        writeOutFloat(c.getValue);
        return c;
    }
    def override visitExprConstString(var c : smart_ptr<ExprConstString>) : ExpressionPtr {
        if (c.value.empty()) {
            write(*ss, "nullptr");
        } else {
            write(*ss, "((char *) \"{escape(string(c.value))}\")");
        }
        return c;
    }
    def override visitExprConstInt2(var c : smart_ptr<ExprConstInt2>) : ExpressionPtr {
        let val = c.getValue;
        write(*ss, "int2({val.x},{val.y})");
        return c;
    }
    def override visitExprConstRange(var c : smart_ptr<ExprConstRange>) : ExpressionPtr {
        let val = c.getValue;
        write(*ss, "range({val.x},{val.y})");
        return c;
    }
    def override visitExprConstRange64(var c : smart_ptr<ExprConstRange64>) : ExpressionPtr {
        let val = c.getValue;
        write(*ss, "range64({val.x},{val.y})");
        return c;
    }
    def override visitExprConstInt3(var c : smart_ptr<ExprConstInt3>) : ExpressionPtr {
        let val = c.getValue;
        write(*ss, "int3({val.x},{val.y},{val.z})");
        return c;
    }
    def override visitExprConstInt4(var c : smart_ptr<ExprConstInt4>) : ExpressionPtr {
        let val = c.getValue;
        write(*ss, "int4({val.x},{val.y},{val.z},{val.w})");
        return c;
    }
    def override visitExprConstUInt2(var c : smart_ptr<ExprConstUInt2>) : ExpressionPtr {
        let val = c.getValue;
        write(*ss, "uint2({val.x},{val.y})");
        return c;
    }
    def override visitExprConstURange(var c : smart_ptr<ExprConstURange>) : ExpressionPtr {
        let val = c.getValue;
        write(*ss, "urange({val.x},{val.y})");
        return c;
    }
    def override visitExprConstURange64(var c : smart_ptr<ExprConstURange64>) : ExpressionPtr {
        let val = c.getValue;
        write(*ss, "urange64({val.x},{val.y})");
        return c;
    }
    def override visitExprConstUInt3(var c : smart_ptr<ExprConstUInt3>) : ExpressionPtr {
        let val = c.getValue;
        write(*ss, "uint3({val.x},{val.y},{val.z})");
        return c;
    }
    def override visitExprConstUInt4(var c : smart_ptr<ExprConstUInt4>) : ExpressionPtr {
        let val = c.getValue;
        write(*ss, "uint4({val.x},{val.y},{val.z},{val.w})");
        return c;
    }
    def override visitExprConstFloat2(var c : smart_ptr<ExprConstFloat2>) : ExpressionPtr {
        let val = c.getValue;
        if (val.x == 0.0f && val.y == 0.0f) {
            write(*ss, "v_zero()");
        } elif (val.x == val.y) {
            write(*ss, "v_splats(");
            writeOutFloat(val.x);
            write(*ss, ")");
        } else {
            write(*ss, "v_make_vec4f(");
            writeOutFloat(val.x);
            write(*ss, ",");
            writeOutFloat(val.y);
            write(*ss, ",0.f,0.f)");
        }
        return c;
    }
    def override visitExprConstFloat3(var c : smart_ptr<ExprConstFloat3>) : ExpressionPtr {
        let val = c.getValue;
        if (val.x == 0.0f && val.y == 0.0f && val.z == 0.0f) {
            write(*ss, "v_zero()");
        } elif (val.x == val.y && val.x == val.z) {
            write(*ss, "v_splats(");
            writeOutFloat(val.x);
            write(*ss, ")");
        } else {
            write(*ss, "v_make_vec4f(");
            writeOutFloat(val.x);
            write(*ss, ",");
            writeOutFloat(val.y);
            write(*ss, ",");
            writeOutFloat(val.z);
            write(*ss, ",0.f)");
        }
        return c;
    }
    def override visitExprConstFloat4(var c : smart_ptr<ExprConstFloat4>) : ExpressionPtr {
        let val = c.getValue;
        if (val.x == 0.0f && val.y == 0.0f && val.z == 0.0f && val.w == 0.0f) {
            write(*ss, "v_zero()");
        } elif (val.x == val.y && val.x == val.z && val.x == val.w) {
            write(*ss, "v_splats(");
            writeOutFloat(val.x);
            write(*ss, ")");
        } else {
            write(*ss, "v_make_vec4f(");
            writeOutFloat(val.x);
            write(*ss, ",");
            writeOutFloat(val.y);
            write(*ss, ",");
            writeOutFloat(val.z);
            write(*ss, ",");
            writeOutFloat(val.w);
            write(*ss, ")");
        }
        return c;
    }
// ExprAssume
    def override preVisitExprAssume(expr : smart_ptr<ExprAssume>) {
        write(*ss, "\n#if 0 // with, note optimizations are off\n");
    }
    def override visitExprAssume(var expr : smart_ptr<ExprAssume>) : ExpressionPtr {
        write(*ss, "\n#endif\n");
        return expr;
    }
// ExprWith
    def override preVisitExprWith(expr : smart_ptr<ExprWith>) {
        write(*ss, "\n#if 0 // with, note optimizations are off\n");
    }
    def override preVisitExprWithBody(expr : smart_ptr<ExprWith>; body : ExpressionPtr) {
        write(*ss, "\n#endif\n");
    }
// ExprWhile
    def override preVisitExprWhile(wh : smart_ptr<ExprWhile>) {
        if (wh.body is ExprBlock) {
            let blk = wh.body as ExprBlock;
            if (!blk.finalList.empty()) {
                write(*ss, "\{\n");
                tab ++;
                visit_finally(blk, adapter);
                write(*ss, tabs());
            }
        }
        write(*ss, "while ( ");
    }
    def override preVisitExprWhileBody(wh : smart_ptr<ExprWhile>; body : ExpressionPtr) {
        write(*ss, " )\n{tabs()}");
    }
    def override visitExprWhile(var wh : smart_ptr<ExprWhile>) : ExpressionPtr {
        if (wh.body is ExprBlock) {
            let blk = wh.body as ExprBlock;
            if (!blk.finalList.empty()) {
                tab --;
                write(*ss, "\n{tabs()}\}");
            }
        }
        return wh;
    }
// if then else
    def override preVisitExprIfThenElse(ifte : smart_ptr<ExprIfThenElse>) {
        write(*ss, "if ( ");
    }
    def override preVisitExprIfThenElseIfBlock(ifte : smart_ptr<ExprIfThenElse>; blk : ExpressionPtr) {
        write(*ss, " )\n");
        write(*ss, tabs());
    }
    def override preVisitExprIfThenElseElseBlock(ifte : smart_ptr<ExprIfThenElse>; blk : ExpressionPtr) {
        write(*ss, " else ");
    }
// swizzle
    def override preVisitExprSwizzle(expr : smart_ptr<ExprSwizzle>) {
        if (expr._type.flags.ref) {
            let type_str = describeCppType(expr._type, DescribeConfig(skip_ref = true, cross_platform = cross_platform));
            let value_str = describeCppType(expr.value._type, DescribeConfig(skip_ref = true, cross_platform = cross_platform));
            write(*ss, "das_swizzle_ref<{type_str},{value_str},{int(expr.fields[0])}>::swizzle(");
        } else {
            if (length(expr.fields) == 1) {
                let mask = "xyzw";
                write(*ss, "v_extract_")
                *ss |> write_char(character_at(mask, int(expr.fields[0])));
                if (expr._type.baseType != Type.tFloat) {
                    write(*ss, "i");
                }
                write(*ss, "(");
                if (expr._type.baseType != Type.tFloat) {
                    write(*ss, "v_cast_vec4i(");
                }
            } else {
                let type_str = describeCppType(expr._type, DescribeConfig(skip_ref = true, cross_platform = cross_platform));
                let value_str = describeCppType(expr.value._type, DescribeConfig(skip_ref = true, cross_platform = cross_platform));
                if (isSequencialMask(expr.fields)) {
                    write(*ss, "das_swizzle_seq<{type_str},{value_str},{expr.fields[0]:d}>::swizzle(");
                } else {
                    let field_str = (expr.fields |> each()
                                                 |> map(@(f) { return "{f:d}"; })
                                                 |> join(","));
                    write(*ss, "das_swizzle<{type_str},{value_str},{field_str}>::swizzle(");
                }
            }
        }
        if (expr.value._type.flags.aotAlias) {
            write(*ss, "das_alias<{expr.value._type.alias}>::from(");
        }
     }
    def override visitExprSwizzle(var expr : smart_ptr<ExprSwizzle>) : ExpressionPtr {
        if (expr.value._type.flags.aotAlias) {
            write(*ss, ")");
        }
        if (expr._type.flags.ref) {
            write(*ss, ")");
        } else {
            if (length(expr.fields) == 1) {
                if (expr._type.baseType != Type.tFloat) write(*ss, ")");
                write(*ss, ")");
            } elif (isSequencialMask(expr.fields)) {
                write(*ss, ")");
            } else {
                write(*ss, ")");
            }
        }
        write(*ss, " /*");
        for (f in expr.fields) {
            match (int(f)) {
                if (0) { write(*ss, "x"); }
                if (1) { write(*ss, "y"); }
                if (2) { write(*ss, "z"); }
                if (3) { write(*ss, "w"); }
                if (_) { write(*ss, "?"); }
            }
        }
        write(*ss, "*/");
        return expr;
    }
// string builder
    def outputCallTypeInfo(nArgs : uint; elements : dasvector`smart_ptr`Expression) {
        let debug_info_name = "__tinfo_{debugInfoGlobal++:d}";
        type_info.push(build_string() <| $(writer) {
            let tinfo_str = (
                elements |> each()
                         |> map(@(arg) {
                            return "&{typeInfoName(helper.helper |> make_type_info(null, arg._type))}";
                         })
                         |> join(", ")
            )
            write(writer, "TypeInfo * {debug_info_name}[{nArgs:d}] = \{ {tinfo_str} \};");
        })
        return debug_info_name;
    }
    def override preVisitExprStringBuilder(expr : smart_ptr<ExprStringBuilder>) {
        let nArgs = uint(length(expr.elements));
        write(*ss, "das_string_builder");
        if (expr.stringBuilderFlags.isTempString) {write(*ss, "_temp"); }
        write(*ss, "(__context__,SimNode_AotInterop<{nArgs:d}>(");
        if (nArgs != 0u) {
            let debug_info_name = outputCallTypeInfo(nArgs, expr.elements);
            write(*ss, "{debug_info_name}, ");
        }
    }
    def override preVisitExprStringBuilderElement(sb : smart_ptr<ExprStringBuilder>; expr : ExpressionPtr; last : bool) {
        write(*ss, "cast<{describeCppType(expr._type,DescribeConfig(cross_platform=cross_platform))}");
        if (expr._type.isRefType && !expr._type.flags.ref) {
            write(*ss, " &");
        }
        write(*ss, ">::from(");
    }
    def override visitExprStringBuilderElement(sb : smart_ptr<ExprStringBuilder>; expr : ExpressionPtr; last : bool) {
        write(*ss, ")");
        if (!last) {
            write(*ss, ", ");
        }
        return expr;
    }
    def override visitExprStringBuilder(var expr : smart_ptr<ExprStringBuilder>) : ExpressionPtr {
        write(*ss, "))");
        return expr;
    }
// typedecl
    def override preVisitExprTypeDecl(expr : smart_ptr<ExprTypeDecl>) {
        write(*ss, "das_typedecl_value<{describeCppType(expr.typeexpr,DescribeConfig(cross_platform=cross_platform))}>()()");
    }
// type-info
    def override preVisitExprTypeInfo(expr : smart_ptr<ExprTypeInfo>) {
        if (expr.macro == null) {
            panic("internal error. we should only be here if there is a macro.");
        }
        write(*ss, "(");
        expr.macro |> write_aot_macro_prefix(ss, expr);
        if (expr.macro |> aot_need_type_info(expr)) {
            let info = helper.helper |> make_type_info(null, expr.typeexpr);
            write(*ss, typeInfoName(info));
        }
    }

    def override canVisitExprTypeInfo(expr : smart_ptr<ExprTypeInfo>; expr_ : ExpressionPtr) {
        if (expr.macro == null) {
            panic("internal error. we should only be here if there is a macro.");
        }
        return expr.macro |> macro_aot_infix(ss, expr);
    }

    def override visitExprTypeInfo(var expr : smart_ptr<ExprTypeInfo>) : ExpressionPtr {
        if (expr.macro == null) {
            panic("internal error. we should only be here if there is a macro.");
        }
        expr.macro |> write_aot_macro_suffix(ss, expr);
        write(*ss, ")");
        return expr;
    }
// try-catch
    def override preVisitExprTryCatch(tc : smart_ptr<ExprTryCatch>) {
        write(*ss, "das_try_recover(__context__, [&]()\n");
        write(*ss, "{tabs()}");
    }
    def override preVisitExprTryCatchCatch(tc : smart_ptr<ExprTryCatch>; blk : ExpressionPtr) {
        write(*ss, ", [&]()\n");
        write(*ss, "{tabs()}");
    }
    def override visitExprTryCatch(var tc : smart_ptr<ExprTryCatch>) : ExpressionPtr {
        write(*ss, ")");
        return tc;
    }
// ptr2ref
    def override preVisitExprPtr2Ref(ptr2ref : smart_ptr<ExprPtr2Ref>) {
        if (ptr2ref.unsafeDeref) {
            write(*ss, "(*(");
        } else {
            write(*ss, "das_deref(__context__,");
        }
    }
    def override visitExprPtr2Ref(var ptr2ref : smart_ptr<ExprPtr2Ref>) : ExpressionPtr {
        if (ptr2ref.unsafeDeref) {
            write(*ss, "))");
        } else {
            write(*ss, ")");
        }
        return ptr2ref;
    }
 // ref2ptr
    def override preVisitExprRef2Ptr(ref2ptr : smart_ptr<ExprRef2Ptr>) {
        write(*ss, "das_ref(__context__,");
    }
    def override visitExprRef2Ptr(var ref2ptr : smart_ptr<ExprRef2Ptr>) : ExpressionPtr {
        write(*ss, ")");
        return ref2ptr;
    }
// addr
    def queryByMNH(name : string; hash : uint64) {
        return "Func(__context__->fnByMangledName(/*{name}*/ 0x{hash:x}))";
    }

    def override preVisitExprAddr(expr : smart_ptr<ExprAddr>) {
        if (expr.func != null) {
            let mangledName =  expr.func |> get_mangled_name();
            let hash = expr.func.getMangledNameHash;
            write(*ss, queryByMNH(mangledName, hash));
        } else {
            write(*ss, "Func(0 /*nullptr*/)");
        }
    }
// cast
    def override preVisitExprCast(expr : smart_ptr<ExprCast>) {
        write(*ss, "{(expr.castFlags.upcastCast ? "das_upcast" : "das_cast")}<{describeCppType(expr.castType,DescribeConfig(cross_platform=cross_platform))}>::cast(");
    }
    def override visitExprCast(var expr : smart_ptr<ExprCast>) : ExpressionPtr {
        write(*ss, ")");
        return expr;
    }
// delete
    def override preVisitExprDelete(edel : smart_ptr<ExprDelete>) {
        assume subt = edel.subexpr._type;
        assume subft = subt.firstType;
        if (subt.isPointer && subft.baseType == Type.tHandle) {
            write(*ss, "das_delete_handle<");
        } elif (subt.isPointer && subft.baseType == Type.tStructure &&
                   subft.structType.flags.persistent) {
            write(*ss, "das_delete_persistent<");
        } elif (subt.isPointer && subft.baseType == Type.tStructure &&
                   subft.structType.flags.isLambda) {
            write(*ss, "das_delete_lambda_struct<");
        } else {
            write(*ss, "das_delete<");
        }
        write(*ss, "{describeCppType(edel.subexpr._type,DescribeConfig(skip_ref=true,skip_const=true,cross_platform=cross_platform))}");
        write(*ss, ">::clear(__context__,");
    }
    // DELETE
    def override preVisitExprDeleteSizeExpression(del : smart_ptr<ExprDelete>; expr : ExpressionPtr) {
        write(*ss, ",");
    }

    def override visitExprDelete(var edel : smart_ptr<ExprDelete>) : ExpressionPtr {
        write(*ss, ")");
        return edel;
    }
// ascend
    def override preVisitExprAscend(expr : smart_ptr<ExprAscend>) {
        let info : TypeInfo?  = (expr.ascendFlags.needTypeInfo
                                ? helper.helper |> make_type_info(null, expr.subexpr._type)
                                : null);
        if (expr._type.firstType.baseType == Type.tHandle) {
            write(*ss, "das_ascend_handle<{expr._type.flags.smartPtr},{describeCppType(expr._type,DescribeConfig(skip_ref=true,skip_const=true,cross_platform=cross_platform))}>::make(__context__,");
        } else {
            let type_str = describeCppType(expr._type.firstType, DescribeConfig(skip_ref = true, skip_const = true, cross_platform = cross_platform));
            let subexpr_str = describeCppType(expr.subexpr._type, DescribeConfig(skip_ref = true, skip_const = true, cross_platform = cross_platform));
            write(*ss, "das_ascend<{type_str},{subexpr_str}>::make(__context__,");
        }

        if (info != null) {
            write(*ss, "&{typeInfoName(info)},");
        } else {
            write(*ss, "nullptr,");
        }
    }
    def override visitExprAscend(var expr : smart_ptr<ExprAscend>) : ExpressionPtr {
        write(*ss, ")");
        return expr;
    }
// new
    def override preVisitExprNew(enew : smart_ptr<ExprNew>) {
        if (!enew._type.dim |> empty()) {
            if (enew._type.firstType.isHandle) {
                let type_str = describeCppType(enew._type.firstType, DescribeConfig(skip_ref = true, skip_const = true, cross_platform = cross_platform));
                write(*ss, "das_new_dim_handle<{type_str},{enew._type.dim[0]},{enew._type.flags.smartPtr}");
                if (enew.initializer) {
                    panic("internal error. initializer for enew is not supported");
                } else {
                    write(*ss, ">::make(__context__");
                }
            } else {
                let type_str = describeCppType(enew._type.firstType, DescribeConfig(skip_ref = true, skip_const = true, cross_platform = cross_platform));
                write(*ss, "das_new_dim<{type_str},{enew._type.dim[0]}");
                if (enew.initializer) {
                    write(*ss, ">::make_and_init(__context__,[&]() \{ return ");
                        // << " . " << describeCppType(enew._type.firstType,DescribeConfig(skip_ref=true,skip_const=true,cross_platform=cross_platform))
                    CallFunc_preVisit(enew);
                } else {
                    write(*ss, ">::make(__context__");
                }
            }
        } else {
            if (enew._type.firstType.isHandle) {
                let type_str = describeCppType(enew._type.firstType, DescribeConfig(skip_ref = true, skip_const = true, cross_platform = cross_platform));
                write(*ss, "das_new_handle<{type_str},{enew._type.flags.smartPtr}");
                if (enew.initializer) {
                    write(*ss, ">::make_and_init(__context__,[&]() \{ return new ")
                        // << " . " << describeCppType(enew._type.firstType,DescribeConfig(skip_ref=true,skip_const=true,cross_platform=cross_platform))
                    CallFunc_preVisit(enew);
                } else {
                    write(*ss, ">::make(__context__");
                }
            } else {
                if (enew._type.firstType.isStructure && enew._type.firstType.structType.flags.persistent) {
                    write(*ss, "das_new_persistent<");
                } else {
                    write(*ss, "das_new<");
                }
                write(*ss, describeCppType(enew._type.firstType, DescribeConfig(skip_ref = true, skip_const = true, cross_platform = cross_platform)));
                if (enew.initializer) {
                    write(*ss, ">::make_and_init(__context__,[&]() \{ return ")
                        // << " . " << describeCppType(enew._type.firstType,DescribeConfig(skip_ref=true,skip_const=true,cross_platform=cross_platform))
                    CallFunc_preVisit(enew);
                } else {
                    write(*ss, ">::make(__context__");
                }
            }
        }
    }
    def override preVisitExprNewArgument(enew : smart_ptr<ExprNew>; arg : ExpressionPtr; last : bool) {
        if (enew.initializer) {
            CallFunc_preVisitCallArg(enew, arg, last);
        }
    }
    def override visitExprNewArgument(enew : smart_ptr<ExprNew>; arg : ExpressionPtr; last : bool) {
        if (enew.initializer) {
            CallFunc_visitCallArg(enew, arg, last);
        } else {
            panic("we should not even be here. we are visiting arguments of a new, but it has no initializer???");
            write(*ss, ",");
        }
        return arg;
    }
    def override visitExprNew(var enew : smart_ptr<ExprNew>) : ExpressionPtr {
        if (enew.initializer) {
            CallFunc_visit(enew);
            write(*ss, "; \})");
        } else {
            write(*ss, ")");
        }
        return enew;
    }
// make variant
    def needTempSrc(expr : smart_ptr<ExprMakeLocal>) {
        return !expr.makeFlags.doesNotNeedSp && expr.stackTop != 0u;
    }
    def mkvName(expr : smart_ptr<ExprMakeVariant>) {
        if (needTempSrc(expr)) {
            return makeLocalTempName(expr);
        } else {
            return "__mkv_{expr.at.line:d}";
        }
    }
    def override preVisitExprMakeVariant(expr : smart_ptr<ExprMakeVariant>) {
        let type_str = describeCppType(expr._type, DescribeConfig(skip_ref = true, skip_const = true, cross_platform = cross_platform));
        write(*ss, "(([&]() -> {type_str}{needTempSrc(expr) ? "&" : ""} \{\n");
        tab ++;
        if (!needTempSrc(expr)) {
            write(*ss, "{tabs()}{describeCppType(expr._type,DescribeConfig(skip_ref=true,cross_platform=cross_platform))} {mkvName(expr)};\n");
        }
        if (expr.variants.empty()) {
            write(*ss, "{tabs()}das_zero({mkvName(expr)});\n");
        }
    }
    def override preVisitExprMakeVariantField(expr : smart_ptr<ExprMakeVariant>; index : int; decl : MakeFieldDeclPtr; last : bool) {
        let variantIndex = find_argument_index(expr._type, string(decl.name));
        if (variantIndex == -1) {
            panic("should not infer otherwise");
        }
        let type_str = describeCppType(expr._type.argTypes[variantIndex], DescribeConfig(cross_platform = cross_platform));
        let offset_str = expr._type.get_variant_field_offset(variantIndex);
        write(*ss, "{tabs()}{get_variant_field(expr._type, variantIndex)}::set(");
        write(*ss, "{mkvName(expr)}");
        if (length(expr.variants) != 1) write(*ss, "({index},__context__)");
        write(*ss, ") = ");
    }
    def override visitExprMakeVariantField(expr : smart_ptr<ExprMakeVariant>; index : int; decl : MakeFieldDeclPtr; last : bool) {
        write(*ss, ";\n");
        return decl;
    }
    def override visitExprMakeVariant(var expr : smart_ptr<ExprMakeVariant>) : ExpressionPtr {
        write(*ss, "{tabs()}return {mkvName(expr)};\n");
        tab --;
        write(*ss, "{tabs()}\})())");
        return expr;
    }
// make structure
    def mksName(expr : smart_ptr<ExprMakeStruct>) {
        if (needTempSrc(expr)) {
            return makeLocalTempName(expr);
        } else {
            return "__mks_{expr.at.line:d}";
        }
    }
    def override preVisitExprMakeStruct(expr : smart_ptr<ExprMakeStruct>) {
        write(*ss, "(([&](");
        if (expr.makeStructFlags.isNewHandle) {
            let type_str = describeCppType(expr._type, DescribeConfig(skip_ref = true, cross_platform = cross_platform));
            write(*ss, "{type_str} & {mksName(expr)}");
        }
        write(*ss, ")");
        if (!expr.makeStructFlags.isNewHandle) {
            let expr_type = describeCppType(expr._type, DescribeConfig(skip_ref = true, skip_const = true, cross_platform = cross_platform));
            write(*ss, " -> {expr_type}{needTempSrc(expr) ? "&" : ""}");
        }
        write(*ss, " \{\n");
        tab ++;
        if (!expr.makeStructFlags.isNewHandle) {
            if (!needTempSrc(expr)) {
                let expr_type = describeCppType(expr._type, DescribeConfig(skip_ref = true, cross_platform = cross_platform));
                write(*ss, "{tabs()}{expr_type} {mksName(expr)}");
                if (expr.constructor != null) {
                    write(*ss, " = ");
                    let call_func = expr.constructor;
                    if (isHybridCall(call_func)) {
                        write(*ss, "das_invoke_function<{describeCppType(call_func.result,DescribeConfig(skip_const=true,cross_platform=cross_platform))}>::invoke_cmres");
                        assume mangledName = call_func |> get_mangled_name();
                        let hash = call_func.getMangledNameHash;
                        write(*ss, "(__context__,nullptr,");
                        write(*ss, queryByMNH(mangledName, hash));
                        write(*ss, ")");
                    } else {
                        write(*ss, "{aotFuncName(call_func)}(__context__)");
                    }
                }
                write(*ss, ";\n");
            } else {
                if (expr.constructor != null) {
                    write(*ss, "{tabs()}{mksName(expr)} = ");
                    let call_func = expr.constructor;
                    if (isHybridCall(call_func)) {
                        write(*ss, "das_invoke_function<{describeCppType(call_func.result, DescribeConfig(skip_const=true,cross_platform=cross_platform))}>::invoke_cmres");
                        assume mangledName = call_func |> get_mangled_name();
                        let hash = call_func.getMangledNameHash;
                        write(*ss, "(__context__,nullptr,");
                        write(*ss, queryByMNH(mangledName, hash));
                        write(*ss, ")");
                    } else {
                        write(*ss, "{aotFuncName(call_func)}(__context__)");
                    }
                    write(*ss, ";\n");
                }
            }
            if ((expr.constructor == null && !expr.makeFlags.initAllFields) || (expr.makeType.baseType == Type.tTuple && expr.structs |> empty())) {
                write(*ss, "{tabs()}das_zero({mksName(expr)});\n");
            }
        }
    }
    def override preVisitExprMakeStructField(expr : smart_ptr<ExprMakeStruct>; index : int; decl : MakeFieldDeclPtr; last : bool) {
        write(*ss, "{tabs()}");
        write(*ss, "{decl.flags.moveSemantics ? "das_move((" : "das_copy(("}");
        if (expr.makeType.baseType == Type.tHandle) {
            aot_previsit_get_field(expr.makeType.annotation, ss, string(decl.name));
        }
        write(*ss, "{mksName(expr)}");
        if (length(expr.structs) != 1) write(*ss, "({index},__context__)");
        if (expr.makeType.baseType == Type.tHandle) {
            aot_visit_get_field(expr.makeType.annotation, ss, string(decl.name));
            write(*ss, " /*{decl.name}*/");
        } else {
            write(*ss, ".{decl.name}");
        }
        write(*ss, "),(");
    }
    def override visitExprMakeStructField(expr : smart_ptr<ExprMakeStruct>; index : int; var decl : MakeFieldDeclPtr; last : bool) {
        write(*ss, "));\n");
        return decl;
    }
    def override canVisitExprMakeStructBlock(str : smart_ptr<ExprMakeStruct>; expr : ExpressionPtr) { return false; }

    def override visitExprMakeStruct(var expr : smart_ptr<ExprMakeStruct>) : ExpressionPtr {
        if (expr._block != null) {
            assert(expr._block is ExprMakeBlock);
            var mkb = expr._block as ExprMakeBlock;
            assert(mkb._block is ExprBlock);
            unsafe {
                var blk <- reinterpret<smart_ptr<ExprBlock>>(mkb._block);
                collector.renameVariableTo(blk.arguments[0], mksName(expr));
                write(*ss, "{tabs()}");
                visit_expression(blk, adapter);
            }
        }
        if (!expr.makeStructFlags.isNewHandle) {
            write(*ss, "{tabs()}return {mksName(expr)};\n");
        }
        tab --;
        write(*ss, "{tabs()}\})");
        if (!expr.makeStructFlags.isNewHandle) { write(*ss, "()"); }
        write(*ss, ")");
        return expr;
    }
// make array
    def mkaName(expr : smart_ptr<ExprMakeArray>) {
        if (!needTempSrc(expr)) {
            return "__mka_{expr.at.line:d}";
        } else {
            return makeLocalTempName(expr);
        }
    }
    def override preVisitExprMakeArray(expr : smart_ptr<ExprMakeArray>) {
        let type_str = describeCppType(expr._type, DescribeConfig(skip_ref = true, skip_const = true, cross_platform = cross_platform));
        write(*ss, "(([&]() -> {type_str}{(needTempSrc(expr) ? "&" : "")} \{\n");
        tab ++;
        if (!needTempSrc(expr)) {
            write(*ss, "{tabs()}{describeCppType(expr._type,DescribeConfig(skip_ref=true,cross_platform=cross_platform))} {mkaName(expr)};\n");
        }
        if (!expr.makeFlags.initAllFields) {
            write(*ss, "{tabs()}das_zero({mkaName(expr)});\n");
        }
    }
    def override preVisitExprMakeArrayIndex(expr : smart_ptr<ExprMakeArray>; index : int; init : ExpressionPtr; last : bool) {
        write(*ss, "{tabs()}{mkaName(expr)}({index},__context__) = ");
    }
    def override visitExprMakeArrayIndex(expr : smart_ptr<ExprMakeArray>; index : int; init : ExpressionPtr; last : bool) {
        write(*ss, ";\n");
        return init;
    }
    def override visitExprMakeArray(var expr : smart_ptr<ExprMakeArray>) : ExpressionPtr {
        write(*ss, "{tabs()}return {mkaName(expr)};\n");
        tab --;
        write(*ss, "{tabs()}\})())");
        return expr;
    }
// make tuple
    def mktName(expr : smart_ptr<ExprMakeTuple>) {
        if (!needTempSrc(expr)) {
            return "__mkt_{expr.at.line:d}";
        } else {
            return makeLocalTempName(expr);
        }
    }
    def override preVisitExprMakeTuple(expr : smart_ptr<ExprMakeTuple>) {
        let type_str1 = describeCppType(expr._type, DescribeConfig(skip_ref = true, skip_const = true, cross_platform = cross_platform))
        write(*ss, "(([&]() -> {type_str1}{(needTempSrc(expr) ? "&" : "")} \{\n");
        tab ++;
        if (!needTempSrc(expr)) {
            let type_str2 = describeCppType(expr._type, DescribeConfig(skip_ref = true, cross_platform = cross_platform));
            write(*ss, "{tabs()}{type_str2} {mktName(expr)};\n");
        }
        if (!expr.makeFlags.initAllFields) {
            write(*ss, "{tabs()}das_zero({mktName(expr)});\n");
        }
    }
    def override preVisitExprMakeTupleIndex(expr : smart_ptr<ExprMakeTuple>; index : int; init : ExpressionPtr; last : bool) {
        write(*ss, "{tabs()}{get_tuple_field(expr.makeType, index)}::get({mktName(expr)}) = ");
    }
    def override visitExprMakeTupleIndex(expr : smart_ptr<ExprMakeTuple>; index : int; var init : ExpressionPtr; last : bool) {
        write(*ss, ";\n");
        return init;
    }
    def override visitExprMakeTuple(var expr : smart_ptr<ExprMakeTuple>) : ExpressionPtr {
        write(*ss, "{tabs()}return {mktName(expr)};\n");
        tab --;
        write(*ss, "{tabs()}\})())");
        return expr;
    }
// ExprMakeBlock
    def override canVisitMakeBlockBody(blk : ExprMakeBlockPtr) {
        return blk.aotFunctorName.empty();
    }
    def override preVisitExprMakeBlock(expr : ExprMakeBlockPtr) {
        let blk = expr._block as ExprBlock;
        if (!blk.blockFlags.aotSkipMakeBlock) {
            write(*ss, "das_make_block");
            if (blk.returnType.isRefType && !blk.returnType.flags.ref) {
                write(*ss, "_cmres");
            }
            let type_str = describeCppType(blk.returnType, DescribeConfig(skip_const = true, cross_platform = cross_platform));
            unsafe {
                let args = (blk.arguments
                    |> each()
                    |> map(@(arg) { return "{describeCppType(arg._type,DescribeConfig(cross_platform=cross_platform))}{arg._type.isRefType && !arg._type.flags.ref ? " &" : ""}"; })
                    |> join(",")
                )
                let maybe_comma = args |> empty() ? "" : ",";
                write(*ss, "<{type_str}{maybe_comma}{args}>(__context__,{blk.stackTop:d},");
            }
        }
        if (!blk.blockFlags.aotSkipMakeBlock || blk.blockFlags.aotDoNotSkipAnnotationData) {
            if (blk.annotationDataSid != uint64(0)) {
                write(*ss, "__context__->adBySid({blk.annotationDataSid:d}u)");
            } else {
                write(*ss, "0");
            }
            write(*ss, ",");
        }
        if (!blk.blockFlags.aotSkipMakeBlock) {
            let info = helper.helper |> make_invokable_type_debug_info(blk |> make_block_type(), blk.at);
            write(*ss, "&{funcInfoName(info)},");
        }
        if (expr.aotFunctorName.empty()) {
            write(*ss, "[&](");
            var ai = 0;
            for (arg in blk.arguments) {
                if (ai++ != 0) {
                    write(*ss, ", ");
                }
                if (isLocalVec(arg._type)) {
                    describeLocalCppType(ss, arg._type, cross_platform);
                } else {
                    write(*ss, "{describeCppType(arg._type,DescribeConfig(redundant_const=false,cross_platform=cross_platform))}");
                    if (arg._type.isRefType && !arg._type.flags.ref) {
                        write(*ss, " &");
                    }
                }
                write(*ss, " {collector.getVarName(arg)}");
            }
            write(*ss, ") ");
            if (blk.blockFlags.aotSkipMakeBlock) {
                write(*ss, "DAS_AOT_INLINE_LAMBDA ");
            }
            write(*ss, "-> {describeCppType(blk.returnType, DescribeConfig(skip_const=true,cross_platform=cross_platform))}");
        } else {
            write(*ss, "{expr.aotFunctorName}");
        }
    }
    def override visitExprMakeBlock(var expr : ExprMakeBlockPtr) : ExpressionPtr {
        let blk = expr._block as ExprBlock;
        if (!blk.blockFlags.aotSkipMakeBlock) {
            write(*ss, ")");
        }
        return expr;
    }

// looks like call
    def override preVisitExprLooksLikeCall(var call : smart_ptr<ExprLooksLikeCall>) {
        if (call.name == "debug") {
            assume argType = call.arguments[0]._type;
            unsafe {
                let info = helper.helper |> make_type_info(reinterpret<TypeInfo?>(null), argType);
                write(*ss, "das_debug(__context__,&{typeInfoName(info)},__FILE__,__LINE__,");
            }
            let maybe_ref = ((argType.isRefType && !argType.flags.ref) ? "&" : "")
            write(*ss, "cast<{describeCppType(argType,DescribeConfig(cross_platform=cross_platform))}{maybe_ref}>::from(");
        } elif (call.name == "assert" || call.name == "verify") {
            let ea = call as ExprAssert;
            if (length(call.arguments) == 1) {
                write(*ss, "{ea.isVerify ? "DAS_VERIFY" : "DAS_ASSERT"}((");
            } else {
                write(*ss, "{ea.isVerify ? "DAS_VERIFYF" : "DAS_ASSERTF"}((");
            }
        } elif (call.name == "erase") {
            write(*ss, "__builtin_table_erase(__context__,");
        } elif (call.name == "insert") {
            write(*ss, "__builtin_table_set_insert(__context__,");
        } elif (call.name == "find") {
            write(*ss, "__builtin_table_find(__context__,");
        } elif (call.name == "key_exists") {
            write(*ss, "__builtin_table_key_exists(__context__,");
        } elif (call.name == "keys") {
            write(*ss, "__builtin_table_keys(__context__,");
        } elif (call.name == "values") {
            write(*ss, "__builtin_table_values(__context__,");
        } elif (call.name == "invoke" || call is ExprInvoke) {
            let bt = call.arguments[0]._type.baseType;
            var methodOffset = -1;
            var methodName = "";
            if (bt == Type.tFunction) {
                let einv = call as ExprInvoke;
                if (einv.isInvokeMethod) {
                    if (call.arguments[0] is ExprField) {
                        let field = call.arguments[0] as ExprField;
                        methodOffset = field.field.offset;
                        methodName = string(field.field.name);
                    } else {
                        panic("internal error. expected field");
                    }
                }
            }
            if (bt == Type.tBlock) {
                write(*ss, "das_invoke");
            } elif (bt == Type.tLambda) {
                write(*ss, "das_invoke_lambda");
            } elif (bt == Type.tFunction && methodOffset != -1) {
                write(*ss, "das_invoke_method");
            } elif (bt == Type.tFunction) {
                write(*ss, "das_invoke_function");
            } elif (bt == Type.tString) {
                write(*ss, "das_invoke_function_by_name");
            } else {
                write(*ss, "das_invoke /*unknown*/");
            }
            let einv = call as ExprInvoke;
            write(*ss, "<{describeCppType(call._type,DescribeConfig(skip_const=true,cross_platform=cross_platform))}");
            if (methodOffset != -1) {
                assume argType = call.arguments[0]._type;
                if (cross_platform) {
                    let cfg = DescribeConfig(skip_ref = true, skip_const = true, cross_platform = cross_platform)
                    write(*ss, ",offsetof({describeCppType(argType.argTypes[0], cfg)},{methodName})");
                } else {
                    write(*ss, ",{methodOffset}/*{methodName}*/");
                }
            }
            write(*ss, ">::invoke");
            if (einv.isCopyOrMove) {
                write(*ss, "_cmres");
            }
            if (length(call.arguments) > 1) {
                unsafe {
                let args = (range(1, length(call.arguments))
                        |> each()
                        |> map(@capture(& call) (id : int) {
                            assume arg = call.arguments[id];
                            let is_ref = (arg._type.isRefType && !arg._type.flags.ref);
                            return "{describeCppType(arg._type,DescribeConfig(cross_platform=cross_platform))}{is_ref ? " &" : ""}";
                        })
                        |> join(","))
                write(*ss, "<{args}>");
                }
            }
            write(*ss, "(__context__,nullptr,");
        } elif (call.name == "memzero") {
            write(*ss, "memset((void*)&(");
        } elif (call.name == "static_assert") {
            write(*ss, "das_static_assert(");
        } else {
            write(*ss, "{call.name}(");
        }
    }
    def override canVisitExprLooksLikeCallArgument(call : smart_ptr<ExprLooksLikeCall>; arg : ExpressionPtr; last : bool) {
        if (length(call.arguments) >= 1 && call.arguments[0] == arg &&  call is ExprInvoke) {
            let inv = call as ExprInvoke;
            if (inv.isInvokeMethod) return false;
        }
        return true;
    }
    def override preVisitExprLooksLikeCallArgument(call : smart_ptr<ExprLooksLikeCall>; arg : ExpressionPtr; last : bool) {
        if (call.name == "invoke") {
            assume argType = arg._type;
            if (arg._type.isRefType) {
                if (needsArgPass(arg)) {
                    write(*ss, "das_arg<{describeCppType(argType,DescribeConfig(skip_ref=true,cross_platform=cross_platform))}>::pass(");
                }
            }
        }
    }
    def override visitExprLooksLikeCallArgument(call : smart_ptr<ExprLooksLikeCall>; arg : ExpressionPtr; last : bool) {
        if (call.name == "invoke") {
            assume argType = arg._type;
            if (arg._type.isRefType) {
                if (needsArgPass(arg)) {
                    write(*ss, ")");
                }
            }
        }
        if (!last) {
            if (call.name == "assert" || call.name == "verify" || call.name == "debug") {
                write(*ss, "),(");
            } else {
                write(*ss, ",");
            }
        }
        return arg;
    }
    def override visitExprLooksLikeCall(var call : smart_ptr<ExprLooksLikeCall>) : ExpressionPtr {
        if (call.name == "assert" || call.name == "verify" || call.name == "debug") {
            write(*ss, "))");
        } elif (call.name == "memzero") {
            assume cur_type = call.arguments[0]._type;
            if (cross_platform) {
                write(*ss, "), 0, TypeSize<{describeCppType(cur_type,DescribeConfig(skip_ref=true,cross_platform=cross_platform))}>::size)");
            } else {
                write(*ss, "), 0, {cur_type.sizeOf})");
            }
        } else {
            write(*ss, ")");
        }
        return call;
    }
// call
    def policyArgNeedCast(polType : TypeDeclPtr; argType : TypeDeclPtr) {
        if (argType.isVectorType) {
            return false;
        }
        if (!polType.isHandle) {
            if (polType.isVecPolicyType && argType.isVecPolicyType) {
                return false;
            }
        }
        if (!polType.isPolicyType) {
            return false;
        }
        return true;
    }
    def policyResultNeedCast(polType : TypeDeclPtr; resType : TypeDeclPtr) {
        if (resType.isVoid) {
            return false;
        }
        if (!resType.isPolicyType) {
            return false;
        }
        return policyArgNeedCast(polType, resType);
    }
    def isPolicyBasedCall(call : smart_ptr<ExprCall>) {
        let bif = call.func as BuiltInFunction;
        if (call.arguments |> empty() && call.func.result.baseType == Type.tHandle) {
            // c-tor?
            return false;
        } elif (bif.flags.policyBased) {
            return true;
        } else {
            return false;
        }
    }
    def isPolicyBasedCallFunc(call : smart_ptr<ExprCallFunc>) {
        if (call.func.flags.builtIn) {
            let bif = call.func as BuiltInFunction;
            if (bif.flags.policyBased) {
                return true;
            }
        }
        return false;
    }
    def isHybridCall(func : Function?) {
        if (func.flags.builtIn) {
            let bif = func as BuiltInFunction;
            if (func.flags.policyBased) {
                panic("we should not be here. policy based calls are handled elsewhere");
            }
            if (func.flags.callBased) {
                panic("we should not be here. call-based calls handled elsewhere");
            }
            if (length(bif.cppName) == 0) {
                return true;
            }
            return false;
        }
        if (func.flags.noAot) return true;
        if (func.flags.aotHybrid) return true;
        if (func._module == program.getThisModule) return false;
        return true;
    }
    def needsArgPassType(argType : TypeDeclPtr) {
        return !argType.flags.constant && !argType.isGoodBlockType;
    }
    def needsArgPass(expr : ExpressionPtr) {
        if (expr is ExprMakeBlock) {
            let mkblk = expr as ExprMakeBlock;
            let blk = mkblk._block as ExprBlock;
            if (blk.blockFlags.aotSkipMakeBlock) {
                return false;
            }
        }
        return needsArgPassType(expr._type);
    }
    def isCallWithTemp(call : smart_ptr<ExprCallFunc>) {
        if (call is ExprCall) {
            let expr = call as ExprCall;
            return !expr.doesNotNeedSp && expr.stackTop != 0u;
        }
        return false;
    }
    def CallFunc_preVisit(call : smart_ptr<ExprCallFunc>) {
        if (call.func.moreFlags.propertyFunction) {     // property function goes ((arg0).name()). we do `((` here
            if (call.func.result.flags.aotAlias) {
                write(*ss, "das_alias<{call.func.result.alias}>::from(");
            }
            if (call.func.result.isString) {
                write(*ss, "(({describeCppType(call.func.result,DescribeConfig(cross_platform=cross_platform))})(");  // c-cast const char * etc string casts to char * or char * const
            }
            write(*ss, "((");
            return ;
        }
        let aotName = get_aot_name(call.func, call.get_ptr());
        for (ann in call.func.annotations) {
            if (ann.annotation is FunctionAnnotation) {
                let pAnn = ann.annotation as FunctionAnnotation;
                if (!(aotPrefix |> key_exists(aotName))) {
                    aot_prefixes.push(build_string() <| $(writer) {
                        get_func_aot_prefix(pAnn, unsafe(addr(writer)), call.get_ptr());
                    })
                    aotPrefix |> insert(aotName);
                }
            }
        }
        if (isCallWithTemp(call)) {
            write(*ss, "({makeLocalTempName(call)} = (");
        }
        if (call.func.result.flags.aotAlias) {
            write(*ss, "das_alias<{call.func.result.alias}>::from(");
        }
        if (call.func.flags.builtIn) {
            if (call.func.result.isString) {
                write(*ss, "(({describeCppType(call.func.result,DescribeConfig(cross_platform=cross_platform))})(");  // c-cast const char * etc string casts to char * or char * const
            }
            let bif = call.func as BuiltInFunction;
            if (call.arguments |> empty() && call.func.result.baseType == Type.tHandle) {
                // c-tor?
                write(*ss, "/*c-tor*/ ");
            } elif (bif.flags.policyBased) {
                outPolicy(call.arguments[0]._type);
                write(*ss, "::");
            }
            if (bif.flags.interopFn) {
                write(*ss, "das_call_interop<{describeCppType(call.func.result,DescribeConfig(cross_platform=cross_platform))}>::call(&");
            }
            write(*ss, "{aotName}");
            if (bif.flags.interopFn) {
                let nArgs : uint = uint(length(call.arguments));
                write(*ss, ",__context__,SimNode_AotInterop<{nArgs:d}>(");
                if (nArgs != 0u) {
                    write(*ss, "{outputCallTypeInfo(nArgs, call.arguments)},");
                }
            } else {
                write(*ss, "(");
            }
        } else {
            if (isHybridCall(call.func)) {
                write(*ss, "das_invoke_function<{describeCppType(call.func.result,DescribeConfig(skip_const=true,cross_platform=cross_platform))}>::invoke");
                if (call.func.result.isRefType && !call.func.result.flags.ref) {
                    write(*ss, "_cmres");
                }
                assume mangledName = call.func |> get_mangled_name();
                let hash = call.func.getMangledNameHash;
                if (length(call.arguments) >= 1) {
                    write(*ss, "<");
                    for (arg in call.func.arguments) {
                        write(*ss, "{describeCppType(arg._type,DescribeConfig(cross_platform=cross_platform))}");
                        if (arg._type.isRefType && !arg._type.flags.ref) {
                            write(*ss, " &");
                        }
                        if (arg != call.func.arguments.back()) {
                            write(*ss, ",");
                        }
                    }
                    write(*ss, ">(__context__,nullptr,");
                    write(*ss, "{queryByMNH(mangledName, hash)},");
                } else {
                    write(*ss, "(__context__,nullptr,");
                    write(*ss, queryByMNH(mangledName, hash));
                }
            } else {
                write(*ss, "{aotFuncName(call.func)}(__context__");
                if (!call.arguments |> empty()) write(*ss, ",");
            }
        }
    }
    def needSubstitute(argType : TypeDeclPtr; passType : TypeDeclPtr) {
        if (argType.baseType == Type.anyArgument) return false;
        let ref_matters = false;
        let const_matters = false;
        let temporary_matters = false;
        let allow_substitute = false;
        return !is_same_type(argType, passType, ref_matters, const_matters, temporary_matters, allow_substitute);
    }
    def needPtrCast(argType : TypeDeclPtr; passType : TypeDeclPtr; passExpr : ExpressionPtr) {
        if (passExpr is ExprConstPtr) return true;
        return argType.isVoidPointer ^^ passType.isVoidPointer;
    }
    def needStringCast(func : Function?; arg : TypeDeclPtr) {
        return func.moreFlags.needStringCast && arg.isString && !arg.flags.ref;
    }
    def CallFunc_preVisitCallArg(call : smart_ptr<ExprCallFunc>; arg : ExpressionPtr; is_last : bool) {
        if (call.func.moreFlags.propertyFunction) return ;  // property function goes ((arg0).name()). we do nothing here
        var argIndex = 0;
        for (it in range(length(call.arguments))) {
            if (call.arguments[it].get_ptr() == arg.get_ptr()) {
                break;
            }
            argIndex++;
        }
        assert(argIndex != length(call.arguments));
        write(*ss, "{get_aot_arg_prefix(call.func, call.get_ptr(), argIndex)}");
        assume argType = call.arguments[argIndex]._type;
        assume funArgType = call.func.arguments[argIndex]._type;
        if (funArgType.isAotAlias) {
            if (funArgType.alias |> empty()) {
                let fun_t_str = describeCppTypeEx(funArgType, DescribeConfig(skip_ref = true, redundant_const = true, cross_platform = cross_platform), CpptUseAlias.yes);
                write(*ss, "das_reinterpret<{fun_t_str}>::pass(");
            } else {
                write(*ss, "das_alias<{funArgType.alias}>::to(");
            }
        }
        if (!call.func.flags.noPointerCast && needPtrCast(funArgType, arg._type, arg)) {
            write(*ss, "das_auto_cast<{describeCppType(funArgType,DescribeConfig(cross_platform=cross_platform))}>::cast(");
        }
        if (!call.func.flags.anyTemplate && (call.func.flags.interopFn || funArgType.baseType == Type.anyArgument)) {
            let maybe_ref = argType.isRefType && !argType.flags.ref ? " &" : "";
            write(*ss, "cast<{describeCppType(argType,DescribeConfig(cross_platform=cross_platform))}{maybe_ref}>::from(");
        }
        if (needSubstitute(funArgType, arg._type)) {
            write(*ss, "das_reinterpret<{describeCppType(funArgType,DescribeConfig(skip_ref=true,cross_platform=cross_platform))}>::pass(");
        }
        if (!call.func.flags.interopFn && arg._type.isRefType) {
            if (needsArgPass(arg)) {
                write(*ss, "das_arg<{describeCppType(argType,DescribeConfig(skip_ref=true,cross_platform=cross_platform))}>::pass(");
            }
        }
        if (isPolicyBasedCallFunc(call) && policyArgNeedCast(call.func.result, argType)) {
            write(*ss, "cast<{describeCppType(argType,DescribeConfig(skip_ref=true,skip_const=true,cross_platform=cross_platform))}>::from(");
        }
        if (needStringCast(call.func, argType)) {
            write(*ss, "(das_string_cast(");
        }
    }
    def CallFunc_visitCallArg(call : smart_ptr<ExprCallFunc>; arg : ExpressionPtr; last : bool) {
        if (call.func.moreFlags.propertyFunction) return ;  // property function goes ((arg0).name()). we do nothing here
        var argIndex = 0;
        for (it in range(length(call.arguments))) {
            if (call.arguments[it] == arg) {
                break;
            }
            argIndex++;
        }

        assert(argIndex != length(call.arguments));
        assume argType = call.func.arguments[argIndex]._type;
        if (needStringCast(call.func, argType)) {
            write(*ss, "))");
        }
        if (isPolicyBasedCallFunc(call) && policyArgNeedCast(call.func.result, argType)) {
            write(*ss, ")");
        }
        assume funArgType = call.func.arguments[argIndex]._type;
        if (!call.func.flags.anyTemplate && (call.func.flags.interopFn || funArgType.baseType == Type.anyArgument)) {
            write(*ss, ")");
        }
        if (needSubstitute(funArgType, arg._type)) {
            write(*ss, ")");
        }
        if (!call.func.flags.interopFn && arg._type.isRefType) {
            if (needsArgPass(arg)) {
                write(*ss, ")");
            }
        }
        if (!call.func.flags.noPointerCast && needPtrCast(funArgType, arg._type, arg)) {
            write(*ss, ")");
        }
        if (funArgType.isAotAlias) write(*ss, ")");
        write(*ss, "{get_aot_arg_suffix(call.func, call.get_ptr(), argIndex)}");
        if (!last) {
            write(*ss, ",");
        }
    }
    def CallFunc_visit(call : smart_ptr<ExprCallFunc>) {
        if (call.func.moreFlags.propertyFunction) {     // property function goes ((arg0).name()). we do `).name())` here
            if (call.func.flags.builtIn) {
                let efn = call.func as ExternalFnBase;
                write(*ss, ").{efn.cppName}())");
            } else {
                assert(starts_with(string(call.func.name), ".`"));
                write(*ss, ").{string(call.func.name).chop(2, length(call.func.name))}())");    // we skip .` part of the deal
            }
            if (call.func.result.isString) {
                write(*ss, "))");  // c-cast const char * etc string casts to char * or char * const
            }
            if (call.func.result.flags.aotAlias) {
                write(*ss, ")");
            }
            return ;
        }
        if (call.func.flags.interopFn) {
            write(*ss, ")");
        }
        if (call.arguments |> empty() && call.func.result.baseType == Type.tHandle) {
            // c-tor?
            write(*ss, "/*end-c-tor*/");
        } elif (isPolicyBasedCallFunc(call)) {
            write(*ss, ",*__context__,nullptr");
        }
        write(*ss, ")");
        if (call.func.flags.builtIn && call.func.result.isString) {
            write(*ss, "))");
        }
        if (call.func.result.flags.aotAlias) {
            write(*ss, ")");
        }
        if (isCallWithTemp(call)) {
            write(*ss, "))");
        }
    }
    def override preVisitExprCall(call : smart_ptr<ExprCall>) {
        CallFunc_preVisit(call);
    }
    def override preVisitExprCallArgument(call : smart_ptr<ExprCall>; arg : ExpressionPtr; last : bool) {
        CallFunc_preVisitCallArg(call, arg, last);
    }
    def override visitExprCallArgument(call : smart_ptr<ExprCall>; arg : ExpressionPtr; last : bool) {
        CallFunc_visitCallArg(call, arg, last);
        return arg;
    }
    def override visitExprCall(var call : smart_ptr<ExprCall>) : ExpressionPtr {
        CallFunc_visit(call);
        return call;
    }
// for
    def forSrcName(varName : das_string) {
        return "__{aotSuffixNameEx(varName, "")}_iterator";
    }
    def needLoopName(ffor : smart_ptr<ExprFor>) {
        return "__need_loop_{ffor.at.line:d}";
    }
    def override preVisitExprFor(ffor : smart_ptr<ExprFor>) {
        write(*ss, "\{\n");
        tab ++;
        let nl = needLoopName(ffor);
        write(*ss, "{tabs()}bool {nl} = true;\n");
    }
    def override preVisitExprForBody(ffor : smart_ptr<ExprFor>) {
        let nl = needLoopName(ffor);
        if (ffor.body is ExprBlock) {
            let blk = ffor.body as ExprBlock;
            if (!blk.finalList.empty()) {
                visit_finally(blk, adapter)
            }
        }
        write(*ss, "{tabs()}for ( ; {nl} ; {nl} = ");
        for (variable in ffor.iteratorVariables) {
            if (variable != ffor.iteratorVariables[0]) {
                write(*ss, " && ");
            }
            write(*ss, "{forSrcName(variable.name)}.next(__context__,");
            write(*ss, "({collector.getVarName(variable)}))");
        }
        write(*ss, " )\n");
        write(*ss, "{tabs()}");
    }
    def isCountOrUCount(expr : ExpressionPtr) {
        if (!(isExprCallFunc(expr))) {
            return false;
        }
        assume call = unsafe(reinterpret<smart_ptr<ExprCallFunc>>(expr));
        return call.func != null && call.func.flags.builtIn && call.func._module.name == "$" && (call.name == "count" || call.name == "ucount");
    }
    def override preVisitExprForSource(ffor : smart_ptr<ExprFor>; that : ExpressionPtr; last : bool) {
        var idx = 0;
        let idxs = length(ffor.sources);
        for (id in range(idxs)) {
            if (ffor.sources[id] == that) {
                break;
            }
            idx++
        }
        assume src = ffor.sources[idx];
        assume variable = ffor.iteratorVariables[idx];
        write(*ss, "{tabs()}// {variable.name}: {variable._type.describe()}\n");
        if (isCountOrUCount(src)) {
            write(*ss, "{tabs()}das_iterator_{(unsafe(reinterpret<smart_ptr<ExprCallFunc>>(src))).func.name} DAS_COMMENT(");
        } else {
            let type_str = describeCppType(src._type, DescribeConfig(substitute_ref = true, skip_ref = true, cross_platform = cross_platform));
            write(*ss, "{tabs()}das_iterator<{type_str}> {forSrcName(variable.name)}(");
        }
    }
    def override visitExprForSource(ffor : smart_ptr<ExprFor>; that : ExpressionPtr; last : bool) {
        var idx = 0;
        let idxs = length(ffor.sources);
        for (id in range(idxs)) {
            if (ffor.sources[id] == that) {
                break;
            }
            idx++
        }
        assume src = ffor.sources[idx];
        assume variable = ffor.iteratorVariables[idx];
        if (isCountOrUCount(src)) {
            assume pCall = unsafe(reinterpret<smart_ptr<ExprCallFunc>>(src));
            write(*ss, ") {forSrcName(variable.name)}(");
            visit_expression(pCall.arguments[0], adapter);
            write(*ss, ",");
            visit_expression(pCall.arguments[1], adapter);
        }
        write(*ss, ");\n");

        // source
        let skipTC : bool = variable._type.isString && !variable._type.flags.ref;
        let cfg = DescribeConfig(substitute_ref = true, skip_const = skipTC, cross_platform = cross_platform)
        let type_str = describeCppType(variable._type, cfg);
        write(*ss, "{tabs()}{type_str} {collector.getVarName(variable)};\n");
        // loop
        let nl = needLoopName(ffor);
        write(*ss, "{tabs()}{nl} = {forSrcName(variable.name)}.first(__context__,");
        write(*ss, "({collector.getVarName(variable)})");
        write(*ss, ") && {nl};\n");
        return that;
    }
    def override visitExprFor(var ffor : smart_ptr<ExprFor>) : ExpressionPtr {
        write(*ss, "\n");
        for (variable in ffor.iteratorVariables) {
            write(*ss, "{tabs()}{forSrcName(variable.name)}.close(__context__,");
            write(*ss, "({collector.getVarName(variable)}));\n");
        }
        tab --;
        write(*ss, "{tabs()}\}");
        return ffor;
    }
};


def dumpDependencies(program : ProgramPtr; var aotVisitor : CppAot?) {
    let utm = new UseTypeMarker();
    var inscope adapter <- make_visitor(*utm)
    visit(program, adapter);
    let remUS = program._options  |> find_arg("remove_unused_symbols") ?as tBool ?? true;
    program.get_ptr() |> for_each_module_no_order($(pm) {
        pm |> for_each_structure($(ps) {
            write(*aotVisitor.ss, "namespace {aotModuleName(ps._module)} \{ struct {aotStructName(ps.get_ptr())}; \};\n");
        });
        pm |> for_each_module_function($(fn) {
            // If some functions from another module used we need to declare this type.
            // Because we have to add debug info about this function signature in our module
            if (fn.index < 0 || !fn.flags.used)
                return ;
            visit(fn, adapter);
        });
    });
    program.get_ptr() |> for_each_module_no_order($(pm) {
        if (pm == program.getThisModule) {
            return ;
        }
        pm |> for_each_enumeration($(penum) {
            if (!remUS || utm.useEnums |> key_exists(penum.get_ptr())) {
                program |> visit_enumeration(penum, aotVisitor.adapter)
            } else {
                write(*aotVisitor.ss, "// unused enumeration {penum.name}\n");
            }
        });
        pm |> for_each_structure($(ps) {
            if (!remUS || utm.useStructs |> key_exists(ps.get_ptr())) {
                program |> visit_structure(ps, aotVisitor.adapter);
            } else {
                write(*aotVisitor.ss, "// unused structure {ps.name}\n");
            }
        });
    })

}

def new_cpp_aot(var ss : StringBuilderWriter?; program : ProgramPtr) {
    var cpp_aot = new CppAot(ss = ss, program := program)
}



def collectUsedFunctions(modules : array<Module?>; totalFunctions : int; this_module : Module?; all_modules : bool; is_all : bool = false) : array<Function?> {
    var fnn : array<Function?>;
    fnn.reserve(totalFunctions);
    for (pm in modules) {
        pm |> for_each_module_function($(pfun) {
            if (!all_modules && pfun._module != this_module) {
                return ;
            }
            if (pfun.index < 0 || !pfun.flags.used)
                return ;
            if (!is_all) {
                if (pfun.flags.builtIn || pfun.flags.noAot) return ;
            }
            fnn |> push(pfun.get_ptr());
        });
    }
    return <- fnn;
}

def collectProgramUsedFunctions(program : ProgramPtr; all_modules : bool; is_all : bool) : array<Function?> {
    var modules : array<Module?>;
    program.get_ptr() |> for_each_module_no_order($(mod) {
        modules.push(mod);
    })
    return collectUsedFunctions(modules, program.totalFunctions, program.getThisModule, all_modules, is_all)
}

def registerAotCpp(var logs : StringBuilderWriter?; program : ProgramPtr; var context : Context; cross_platform; headers, all_modules : bool = false) {
    let fnn = collectProgramUsedFunctions(program, all_modules, false);
    if (headers) {
        write(*logs, "\nvoid registerAot ( AotLibrary & aotLib )\n\{\n");
    }
    var funInit = false;
    for (fn in fnn) {
        if (!all_modules && fn._module != program.getThisModule)
            continue;
        if (fn.flags.init) {
            funInit = true;
        }
        let semH = fn.aotHash;
        write(*logs, "    aotLib[0x{semH:x}] = +[](Context & ctx) -> SimNode* \{\n");
        write(*logs, "        return ctx.code->makeNode<AutoSimNode_Aot");
        if (fn.flags.copyOnReturn || fn.flags.moveOnReturn) {
            write(*logs, "CMRES");
        }
        write(*logs, "<&{aotFuncName(fn)}>>()");
        write(*logs, ";\n    \};\n");
    }

    if (context.totalVariables != 0 || funInit) {
        let semH = program |> getInitSemanticHashWithDep(context.getInitSemanticHash);
        write(*logs, "    // [[ init script ]]\n");
        write(*logs, "    aotLib[0x{semH:x}] = +[](Context & ctx) -> SimNode* \{\n");
        write(*logs, "        ctx.aotInitScript = ctx.code->makeNode<AutoSimNode_Aot<&__init_script>>();\n");
        write(*logs, "        return ctx.aotInitScript;\n");
        write(*logs, "    \};\n");
    }

    if (headers) {
        write(*logs, "}\n");
    }
}

def setAotHashes(program : ProgramPtr; var ctx : Context) {
    // compute semantic hash for each used function
    var fni = 0;
    program.get_ptr() |> for_each_module_no_order($(mod) {
        mod |> for_each_module_function($(pfun) {
            if (pfun.index < 0 || !pfun.flags.used)
                return ;
            pfun.hash = get_function_hash_by_id(pfun.get_ptr(), pfun.index, unsafe(reinterpret<void?>(ctx)));
            fni++;
        })
    });
    // compute AOT hash for each used function
    // its the same as semantic hash, only takes dependencies into account
    program.get_ptr() |> for_each_module_no_order($(mod) {
        mod |> for_each_module_function($(pfun) {
            if (pfun.index < 0 || !pfun.flags.used)
                return ;
            pfun.aotHash = get_function_aot_hash(pfun.get_ptr());
        });
    });
}

def dumpRegisterAot(var tw : StringBuilderWriter?; program : ProgramPtr; var context : Context; allModules : bool; cross_platform : bool) {
    write(*tw, "\nstatic void registerAotFunctions ( AotLibrary & aotLib ) \{\n");
    registerAotCpp(tw, program, context, cross_platform, false, allModules);
    write(*tw, "    resolveTypeInfoAnnotations();\n");
    write(*tw, "\};\n");
    write(*tw, "\n");
    write(*tw, "static AotListBase impl(registerAotFunctions);\n");
}

def getRequiredModulesFor(program : ProgramPtr) {
    var modules_str : array<string>
    // lets comment on required modules
    /* todo: we don't need it?
    for (cur_mod in ordered(mod.requireModule)) {
        let (req, pub) = cur_mod
        if (req.name.empty()) {
            // nothing, its main program module. i.e ::
        } else {
            if (req.name == "$") {
                write(ss, " // require builtin\n");
            } else {
                write(ss, " // require {req.name}\n");
            }
            if (req.aotRequire(ss) == ModuleAotType::no_aot) {
                write(ss, "  // no_aot ignored in standalone context\n");
            }
        }
    }
    */
    var noAotModule = false
    program.get_ptr() |> for_each_module($(mod) {
        if (mod.name == "") {
            // nothing, its main program module. i.e ::
        } else {
            modules_str |> push(build_string() <| $(wr) {
                if (mod.name == "$") {
                    write(wr, " // require builtin\n");
                } else {
                    write(wr, " // require {mod.name}\n");
                }
                if (!(mod |> aot_require(unsafe(addr(wr))))) {
                    write(wr, "  // AOT disabled due to this module");
                    noAotModule = true;
                }
            });
        }
    });
    return (modules_str, noAotModule)
}

def compile_and_simulate(input : string, var access; var mg : ModuleGroup?, cop, blk) {
    set_aot();
    compile_file(input, access, mg, cop) <| $(ok; var program : smart_ptr<Program>; issues) {
        if (!ok) {
            print("failed to compile {input}\n{issues}\n")
            return
        }
        simulate(program) <| $(sok; var pctx : smart_ptr<Context>; serrors) {
            if (!sok) {
                panic("Failed to simulate {serrors}")
            }
            blk(program, pctx)
        }
    }
}



[export]
def aot(input : string; isAotLib, paranoid_validation : bool, cross_platform : bool; cop : CodeOfPolicies) : string {
    var result = "";
    using <| $(var mg : ModuleGroup) {
        var inscope access <- make_file_access("")
        compile_and_simulate(input, access, unsafe(addr(mg)), cop) <| $(program : ProgramPtr; var pctx : smart_ptr<Context>) {
            result = build_string() <| $(writer) {
                // lets comment on required modules
                let (modules_str, noAotModule) = getRequiredModulesFor(program)
                if (program._options |> find_arg("no_aot") ?as tBool ?? false) {
                    if (!noAotModule) {
                        write(writer, "// AOT disabled due to options no_aot=true. There are no modules which require no_aot\n\n");
                    } else {
                        write(writer, "// AOT disabled due to options no_aot=true. There are also some modules which require no_aot\n\n");
                    }
                } elif (noAotModule) {
                    write(writer, "// AOT disabled due to module requirements\n");
                    write(writer, "#if 0\n\n");
                    write(writer, join(modules_str, ""));
                    write(writer, "\n#endif\n");
                } else {
                    // header
                    write(writer, AOT_INCLUDES);
                    write(writer, "{join(modules_str, "")}\n");
                    write(writer, AOT_HEADERS);
                    write(writer, "namespace das \{\n");
                    write(writer, "namespace {program.thisNamespace} \{\n");
                    build_string() <| $(tmp_writer) {
                        let marker_vis = new NoAotMarker();
                        var inscope adapter_marker <- make_visitor(*marker_vis)
                        visit(program, adapter_marker);

                        var coll = new BlockVariableCollector();
                        var inscope adapter_coll <- make_visitor(*coll)
                        visit(program, adapter_coll)

                        // mark prologue
                        var pmarker = new PrologueMarker();
                        var inscope adapter_p <- make_visitor(*pmarker)
                        visit(program, adapter_p)

                        var flags = new SetPrinterFlags();
                        var inscope flags_adapter <- make_visitor(*flags)
                        visit(program, flags_adapter)


                        setAotHashes(program, *pctx)

                        var cpp_aot = new CppAot(ss = unsafe(addr(tmp_writer)),
                                                 program := program,
                                                 collector = coll,
                                                 prologue = program._options |> find_arg("aot_prologue") ?as tBool ?? false ||
                                                            program.getDebugger,
                                                 solidContext = program.policies.solid_context ||
                                                            (program._options |> find_arg("solid_context") ?as tBool ?? false),
                                                 cross_platform = cross_platform)
                        cpp_aot.helper.helper.rtti = program._options  |> find_arg("rtti") ?as tBool ?? false;
                        cpp_aot.helper.cross_platform = cross_platform;
                        var inscope adapter <- make_visitor(*cpp_aot)
                        cpp_aot.adapter := adapter
                        dumpDependencies(program, cpp_aot);
                        visit(program, cpp_aot.adapter)
                        write(writer, "{cpp_aot.str()}");
                    }
                    write(writer, "\nstatic void registerAotFunctions ( AotLibrary & aotLib ) \{\n");
                    registerAotCpp(unsafe(addr(writer)), program, *pctx, cross_platform, false);
                    write(writer, "    resolveTypeInfoAnnotations();\n");
                    write(writer, "}\n");
                    write(writer, "\n");
                    if (!isAotLib) write(writer, "static AotListBase impl(registerAotFunctions);\n");
                    // validation stuff
                    if (paranoid_validation) {
                        /* todo: program->validateAotCpp(tw,*pctx);
                        tw << "\n";*/
                    }
                    write(writer, "\} // namespace {program.thisNamespace}\n");
                    if (isAotLib) {
                        write(writer, "AotListBase impl_aot_{program.getThisModule.name}({program.thisNamespace}::registerAotFunctions);\n");
                    }
                    write(writer, "\} // namespace das\n");
                    write(writer, AOT_FOOTER);
                }
            }
        }
    }
    return result
}
