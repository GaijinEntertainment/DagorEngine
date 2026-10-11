options gen2

module aot_cpp private

require strings

require daslib/match
require daslib/strings_boost
require daslib/ast_boost
require daslib/templates_boost
require daslib/linq_boost

require daslib/ast_print_flags
require daslib/aot_constants

options strict_smart_pointers = false
options unsafe_table_lookup = false
// describeCppTypeEx recurses once per type-nesting level; a deeply nested
// function/lambda/tuple type (the fuzzer's, #3065) overflows the default ~16KB stack.
// This bump only governs when aot_cpp is the ROOT program -- the direct `daslang -aot`
// C++ driver compiles it as root (utils/daslang/main.cpp::aot_compile). The CMake/CI
// driver utils/aot/main.das only REQUIRES this module, so getContextStackSize() reads
// the driver's options there; it carries a matching twin bump. Pathological depths
// still overflow and are caught fail-closed (see fail_on_aot_emit_error).
options stack = 1_048_576

def aotFieldName(name : string) : string {
    var result = replace(name, "`", "__");
    if (is_cpp_keyword(result) || result == "DELETE") {
        result = "_f_" + result;
    }
    return result;
}

var private g_aot_main_module_name = ""

def public set_aot_main_module_name(name : string) {
    //! aotModuleName returns this for the unnamed (promoted) main module; "" (the default) leaves its types in an anonymous namespace.
    //! Module-global for the process - set it around one emission and restore "".
    //! Forwards to the C++ twin so DebugInfoHelper's cached cpp type names agree.
    g_aot_main_module_name = name
    set_aot_main_module_name_cpp(name)
}

def aotModuleName(pm : Module?) {
    if (pm.name.empty()) {
        return g_aot_main_module_name;
    } elif (pm.name == "$") {
        return "_builtin_";
    } else {
        return string(pm.name);
    }
}

def aotModuleNameFromString(module_name : string) : string {
    if (empty(module_name)) {
        return g_aot_main_module_name;
    } elif (module_name == "$") {
        return "_builtin_";
    }
    return module_name;
}


def isSequencialMask(fields : dasvector`uint8) {
    for (i in range(1, length(fields))) {
        if (int(fields[i - 1]) + 1 != int(fields[i])) return false;
    }
    return true;
}


def das_to_cppString(t : Type) {   // nolint:STYLE037 - one match arm per Type enum value
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
        if (Type.tBitfield8) { return "Bitfield8"; }
        if (Type.tBitfield16) { return "Bitfield16"; }
        if (Type.tBitfield64) { return "Bitfield64"; }
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
        if (Type.tFloat16) { return "float16_t"; }
        if (Type.tHalf2) { return "half2"; }
        if (Type.tHalf3) { return "half3"; }
        if (Type.tHalf4) { return "half4"; }
        if (Type.tHalf8) { return "half8"; }
        if (Type.tShort2) { return "short2"; }
        if (Type.tShort3) { return "short3"; }
        if (Type.tShort4) { return "short4"; }
        if (Type.tShort8) { return "short8"; }
        if (Type.tUShort2) { return "ushort2"; }
        if (Type.tUShort3) { return "ushort3"; }
        if (Type.tUShort4) { return "ushort4"; }
        if (Type.tUShort8) { return "ushort8"; }
        if (Type.tByte2) { return "byte2"; }
        if (Type.tByte3) { return "byte3"; }
        if (Type.tByte4) { return "byte4"; }
        if (Type.tByte8) { return "byte8"; }
        if (Type.tByte16) { return "byte16"; }
        if (Type.tUByte2) { return "ubyte2"; }
        if (Type.tUByte3) { return "ubyte3"; }
        if (Type.tUByte4) { return "ubyte4"; }
        if (Type.tUByte8) { return "ubyte8"; }
        if (Type.tUByte16) { return "ubyte16"; }
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
    }
    panic("Missed type {t}")
    return ""
}

def das_to_cppCTypeString(t : Type) {   // nolint:STYLE037 - one match arm per Type enum value
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
        if (Type.tBitfield8) { return "tBitfield8"; }
        if (Type.tBitfield16) { return "tBitfield16"; }
        if (Type.tBitfield64) { return "tBitfield64"; }
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
        if (Type.tFloat16) { return "tFloat16"; }
        if (Type.tHalf2) { return "tHalf2"; }
        if (Type.tHalf3) { return "tHalf3"; }
        if (Type.tHalf4) { return "tHalf4"; }
        if (Type.tHalf8) { return "tHalf8"; }
        if (Type.tShort2) { return "tShort2"; }
        if (Type.tShort3) { return "tShort3"; }
        if (Type.tShort4) { return "tShort4"; }
        if (Type.tShort8) { return "tShort8"; }
        if (Type.tUShort2) { return "tUShort2"; }
        if (Type.tUShort3) { return "tUShort3"; }
        if (Type.tUShort4) { return "tUShort4"; }
        if (Type.tUShort8) { return "tUShort8"; }
        if (Type.tByte2) { return "tByte2"; }
        if (Type.tByte3) { return "tByte3"; }
        if (Type.tByte4) { return "tByte4"; }
        if (Type.tByte8) { return "tByte8"; }
        if (Type.tByte16) { return "tByte16"; }
        if (Type.tUByte2) { return "tUByte2"; }
        if (Type.tUByte3) { return "tUByte3"; }
        if (Type.tUByte4) { return "tUByte4"; }
        if (Type.tUByte8) { return "tUByte8"; }
        if (Type.tUByte16) { return "tUByte16"; }
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
    }
    panic("Missed type {t}")
    return ""
}



def isConstRedundantForCpp(typeDecl : TypeDeclPtr) {
    if (typeDecl.baseType == Type.tFixedArray) return false;
    if (typeDecl.isVectorType) return true;
    match (typeDecl.baseType) {
        if (Type.tBool || Type.tInt8 || Type.tUInt8 || Type.tInt16 || Type.tUInt16 ||
            Type.tInt64 || Type.tUInt64 || Type.tInt || Type.tUInt ||
            Type.tFloat || Type.tDouble ||
            Type.tEnumeration || Type.tEnumeration8 || Type.tEnumeration16 || Type.tEnumeration64 ||
            Type.tBitfield || Type.tBitfield8 || Type.tBitfield16 || Type.tBitfield64) {
            return true
        }
    }
    return false
}


enum CpptUseAlias {
    //! Controls whether type aliases are used in C++ codegen output.
    no
    yes
};

enum CpptSubstitureRef {
    //! Controls whether references are substituted in C++ type printing.
    no
    yes
};
enum CpptSkipConst {
    //! Controls whether const qualifiers are skipped in C++ type printing.
    no
    yes
};

struct public DescribeConfig {
    //! Configuration for C++ type description printing.
    substitute_ref : bool = false
    skip_ref : bool = false
    skip_const : bool = false
    redundant_const : bool = true
    cross_platform : bool = false
    use_smart_ptr : bool = false
}

def hex_char(var Ch : int) {
    Ch &= int(0x0f);
    if (Ch <= 9) {
        return '0' + Ch;
    } else {
        return 'a' + (Ch - 10);
    }
}

def aotSuffixNameEx(funcName : das_string; suffix : string; force : bool = false) {
    return aotSuffixNameEx(string(funcName), suffix, force);
}

def aotSuffixNameEx(funcName : string; suffix : string; force : bool = false) {
    var prefix = force || is_cpp_keyword(funcName);

    let name = build_string() $(var writer) {   // nolint:STYLE037 - one match arm per C++-illegal identifier character
        for (ch in funcName) {
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
                    if (',') { t_ch = "Comma"; }
                    if (_) {
                        let repr = build_string() $(ss) {
                            ss |> write_char(hex_char(ch >> 4))
                            ss |> write_char(hex_char(ch));
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

var private g_struct_enum_collide : table<string>

def private structEnumNsKey(moduleName : string; name : string) : string {
    return "{moduleName}::{name}";
}

def private structEnumNsKey(pm : Module?; name : string) : string {
    return structEnumNsKey(aotModuleName(pm), name);
}

def buildStructEnumCollisions(program : ProgramPtr) {
    g_struct_enum_collide |> clear();
    program.get_ptr() |> for_each_module_no_order($(pm) {
        var enumNames : table<string>;
        pm |> for_each_enumeration($(penum) {
            enumNames |> insert(string(penum.name));
        });
        if (!empty(enumNames)) {
            pm |> for_each_structure($(ps) {
                if (enumNames |> key_exists(string(ps.name))) {
                    g_struct_enum_collide |> insert(structEnumNsKey(pm, string(ps.name)));
                }
            });
        }
        delete enumNames;
    });
}

def aotStructName(st : Structure?) {
    let collide = g_struct_enum_collide |> key_exists(structEnumNsKey(st._module, string(st.name)));
    return aotSuffixNameEx(st.name, "_S", collide);
}

def aotEnumName(enu : Enumeration?) {
    let collide = g_struct_enum_collide |> key_exists(structEnumNsKey(enu._module, string(enu.name)));
    return aotSuffixNameEx(enu.name, "_E", collide);
}

def aotEnumValueName(name : das_string) {
    return aotSuffixNameEx(name, "_V");
}


def fa_element(t : TypeDeclPtr) {
    var e = clone_type(t)
    while (e.baseType == Type.tFixedArray && e.firstType != null) {
        e = clone_type(e.firstType)
    }
    return e
}

def describeCppTypeEx(typeDecl : TypeDeclPtr;   // nolint:STYLE038 - one arm per C++ type shape; the ladder IS the emitter
                      in_cfg : DescribeConfig;
                      useAlias : CpptUseAlias) {
    var cfg := in_cfg
    if (isConstRedundantForCpp(typeDecl) && cfg.redundant_const) {
        if (cfg.substitute_ref && typeDecl.flags.ref) {
        } elif (typeDecl.flags.ref) {
        } else {
            cfg.skip_const = true;
        }
    }
    let baseType = typeDecl.baseType;

    return build_string() $(writer) {   // nolint:STYLE037 - one arm per C++ type shape
        if (baseType == Type.tFixedArray && typeDecl.firstType != null) {
            write(writer, "TDim<{describeCppTypeEx(typeDecl.firstType,DescribeConfig(cross_platform=cfg.cross_platform),useAlias)},{typeDecl.fixedDim}>")
        } elif (useAlias == CpptUseAlias.yes && typeDecl.flags.aotAlias && !typeDecl.alias.empty()) {
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
        } elif (baseType == Type.tDistinct) {
            let dname = typeDecl.annotation != null ? string(typeDecl.annotation.name) : "?"
            if (typeDecl.annotation != null && !typeDecl.annotation.cppName.empty()) {
                write(writer, "DAS_COMMENT(distinct {dname}) {typeDecl.annotation.cppName}")
            } elif (typeDecl.firstType != null) {
                let under = describeCppTypeEx(typeDecl.firstType, DescribeConfig(skip_const = true, cross_platform = cfg.cross_platform), useAlias)
                write(writer, "DAS_COMMENT(distinct {dname}) {under}")
            } else {
                write(writer, "DAS_COMMENT(unspecified distinct)")
            }
        } elif (baseType == Type.tArray) {
            if (typeDecl.firstType != null) {
                let cpp_typeDecl = describeCppTypeEx(typeDecl.firstType, DescribeConfig(cross_platform = cfg.cross_platform), useAlias);
                write(writer, "TArray<{cpp_typeDecl}>")
            } else {
                write(writer, "Array")
            }
        } elif (baseType == Type.tTable) {
            if (typeDecl.firstType != null && typeDecl.secondType != null) {
                let first_type = describeCppTypeEx(typeDecl.firstType, DescribeConfig(skip_const = true, cross_platform = cfg.cross_platform), useAlias);
                let second_type = describeCppTypeEx(typeDecl.secondType, DescribeConfig(cross_platform = cfg.cross_platform), useAlias);
                write(writer, "TTable<{first_type},{second_type}>");
            } else {
                write(writer, "Table");
            }
        } elif (baseType == Type.tTuple) {
            let types = (each(typeDecl.argTypes)
                ._select(describeCppTypeEx(_, DescribeConfig(cross_platform = cfg.cross_platform), useAlias))
                .to_array()
                ._fold()) |> join(",")
            if (cfg.cross_platform) {
                write(writer, "AutoTuple<{types}>")
            } elif (types |> empty()) {
                write(writer, "TTuple<{typeDecl.tupleSize}>")
            } else {
                write(writer, "TTuple<{typeDecl.tupleSize},{types}>")
            }
        } elif (baseType == Type.tVariant) {
            let types = (each(typeDecl.argTypes)
                ._select(describeCppTypeEx(_, DescribeConfig(cross_platform = cfg.cross_platform), useAlias))
                .to_array()
                ._fold()) |> join(",")
            if (cfg.cross_platform) {
                write(writer, "AutoVariant<{types}>");
            } elif (types |> empty()) {
                write(writer, "TVariant<{typeDecl.variantSize},{typeDecl.variantAlign}>")
            } else {
                write(writer, "TVariant<{typeDecl.variantSize},{typeDecl.variantAlign},{types}>")
            }
        } elif (baseType == Type.tStructure) {
            if (typeDecl.structType != null) {
                let ns = aotModuleName(typeDecl.structType._module)
                if (ns.empty()) {
                    write(writer, "struct {aotStructName(typeDecl.structType)}")
                } else {
                    write(writer, "struct {ns}::{aotStructName(typeDecl.structType)}")
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
                let ns = aotModuleName(typeDecl.enumType._module)
                if (typeDecl.enumType.external) {
                    write(writer, "DAS_COMMENT(bound_enum) {typeDecl.enumType.cppName}");
                } elif (ns.empty()) {
                    write(writer, "DAS_COMMENT(enum) {aotEnumName(typeDecl.enumType)}");
                } else {
                    write(writer, "DAS_COMMENT(enum) {ns}::{aotEnumName(typeDecl.enumType)}")
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
            let maybe_const = !typeDecl.flags.constant && baseType == Type.tBlock ? "const " : ""
            var type_name = "void"
            if (typeDecl.firstType != null) {
                type_name = describeCppTypeEx(typeDecl.firstType, DescribeConfig(redundant_const = true, cross_platform = cfg.cross_platform), useAlias)
            }
            let extra_comma = !empty(typeDecl.argTypes) ? "," : "";
            let arg_types = (each(typeDecl.argTypes)
                ._select(describeCppTypeEx(_, DescribeConfig(redundant_const = true, cross_platform = cfg.cross_platform), useAlias))
                .to_array()
                ._fold()) |> join(",")
            write(writer, "{maybe_const}{das_to_cppString(baseType)} DAS_COMMENT(({type_name}{extra_comma}{arg_types}))");
        } else {
            write(writer, das_to_cppString(baseType));
        }
        if (!cfg.skip_const && typeDecl.flags.constant) {
            write(writer, " const ");
        }
        if (typeDecl.flags.ref && !cfg.skip_ref) {
            if (cfg.substitute_ref) {
                write(writer, " *");
            } else {
                write(writer, " &");
            }
        }
    }
}

def public describeCppType(typeDecl : TypeDeclPtr;
                           cfg : DescribeConfig) {
    //! Returns the C++ type string for a daslang TypeDecl using the given configuration.
    return describeCppTypeEx(typeDecl, cfg, CpptUseAlias.no);
}

def stackNewName(expr : ExpressionPtr) : string {
    return "_stack_new_{expr.at.line:d}_{expr.at.column:d}";
}

def stackNewPointeeType(expr : ExpressionPtr; cross_platform : bool) : string {
    if (expr is ExprNew) {
        return describeCppType((expr as ExprNew).typeexpr, DescribeConfig(skip_ref = true, skip_const = true, cross_platform = cross_platform));
    }
    return describeCppType((expr as ExprAscend).subexpr._type, DescribeConfig(skip_ref = true, skip_const = true, cross_platform = cross_platform));
}

def expectsLocalTemp(expr : ExprMakeLocal?) : bool {
    if (!expr.makeFlags.doesNotNeedSp && expr.stackTop != 0u) {
        if (expr is ExprMakeStruct) {
            let mks = expr as ExprMakeStruct
            if (mks.makeType.baseType == Type.tHandle
                    && !mks.makeType.isRefType
                    && mks.constructor == null) return false
        }
        return true
    }
    return false
}

class public NoAotMarker : AstVisitor {
    //! AST visitor that marks functions containing non-AOT-compatible types as noAot.
    //! Run it before anything reads `flags.noAot`. It also mutates the AST: `invoke` of a
    //! block literal gets `aotSkipMakeBlock`, and a surviving `quote()` logs an error.
    @do_not_delete func : Function? <- null;

    def override preVisitTypeDecl(typeDecl : TypeDeclPtr) {
        if (func != null && !typeDecl.canAot) {
            func.flags.noAot = true
        }
    }
    def override preVisitFunction(var f : FunctionPtr) {
        func = f;
    }
    def override visitFunction(var that : FunctionPtr) : FunctionPtr {
        var tmp := func
        func = null;
        return <- tmp;
    }
    def override preVisitExpression(expr : ExpressionPtr) {
        if (func != null && expr._type != null && !expr._type.canAot) {
            func.flags.noAot = true
        }
    }
    def override preVisitExprLooksLikeCall(call : ExprLooksLikeCall?) {
        if (call.name == "invoke") {
            if (!empty(call.arguments) && call.arguments[0] is ExprMakeBlock) {
                let mkb = call.arguments[0] as ExprMakeBlock;
                var blk = mkb._block as ExprBlock;
                blk.blockFlags.aotSkipMakeBlock = true;
            }
        }
    }
    def override preVisitExprQuote(expr : ExprQuote?) {
        if (func != null && !func.flags.noAot) {
            func.flags.noAot = true
            to_log(LOG_ERROR, "{describe(expr.at)}: quote( ) survived to AOT with aot_macros enabled - daslib/quote lowering did not run; require daslib/templates_boost (or daslib/quote) in the module containing the quote. Function '{func.name}' excluded from AOT.\n")
        }
    }
};

def public report_unproven_collect_carriers(program : ProgramPtr; all_modules : bool) {
    //! After NoAotMarker: reports every used function the AOT build compiles that reaches a heap collection without proof.
    //! ``all_modules`` covers the foreign functions a standalone context compiles too.
    let this_module = program.getThisModule
    get_ptr(program) |> for_each_module() $(mod) {
        if (!all_modules && mod != this_module) return
        mod |> for_each_function("") $(fn) {
            if (fn.flags.noAot || !is_used(program, fn)) return
            let why = collect_carrier_violation(fn)
            if (!empty(why)) {
                macro_error(program, fn.at, "AOT: function '{fn.name}' can reach heap_collect, and {why}; mark it [no_aot] to keep it interpreted")
            }
        }
    }
}

class public PrologueMarker : AstVisitor {
    //! AST visitor that marks functions requiring a prologue for AOT compilation.
    @do_not_delete func : Function? <- null;

    def override preVisitFunction(var f : FunctionPtr) {
        func = f;
    }
    def override visitFunction(var that : FunctionPtr) {
        func = null;
        return <- that;
    }
    def override preVisitExprMakeBlock(expr : ExprMakeBlock?) {
        if (func != null && func.flags.hasMakeBlock) {
            let blk = expr._block as ExprBlock;
            if (!blk.blockFlags.aotSkipMakeBlock) {
                func.flags.aotNeedPrologue = true;
            }
        }
    }
};

class UseTypeMarker : AstVisitor {
    //! AST visitor that collects structs, enums and handled types used in AOT-eligible functions,
    //! and the modules whose functions the used functions and globals call (externs included).
    useStructs : table<Structure?>;
    useEnums : table<Enumeration?>;
    useHandles : table<TypeAnnotation?>;
    useModules : table<Module?>;
    inUsedCode : bool = false;
    program : ProgramPtr;

    def override preVisitTypeDecl(typeDecl : TypeDeclPtr) {
    }

    def override canVisitFunction(fn : Function?) {
        return !fn.flags.noAot;
    }

    def override preVisitFunction(fun : FunctionPtr) {
        inUsedCode = is_used(program, fun);
    }
    def override visitFunction(var fun : FunctionPtr) : FunctionPtr {
        inUsedCode = false;
        return <- fun;
    }
    def override preVisitExpression(expr : ExpressionPtr) {
        mark(expr._type);
    }
    def override preVisitExprCall(expr : ExprCall?) {
        markCallee(expr.func);
    }
    def override preVisitExprOp1(expr : ExprOp1?) {
        markCallee(expr.func);
    }
    def override preVisitExprOp2(expr : ExprOp2?) {
        markCallee(expr.func);
    }
    def override preVisitExprOp3(expr : ExprOp3?) {
        markCallee(expr.func);
    }
    def override preVisitExprAddr(expr : ExprAddr?) {
        markCallee(expr.func);
    }
    def override preVisitFunctionArgument(fn : FunctionPtr; variable : VariablePtr; lastArg : bool) {
        mark(variable._type);
    }
    def override preVisitExprBlockArgument(expr_blk : ExprBlock?; variable : VariablePtr; lastArg : bool) {
        mark(variable._type);
    }
    def override preVisitGlobalLetVariable(variable : Variable?; lastArg : bool) {
        inUsedCode = is_used(program, variable);
        mark(variable._type);
    }
    def override visitGlobalLetVariable(var variable : VariablePtr; lastArg : bool) : VariablePtr {
        inUsedCode = false;
        return <- variable;
    }
    def markCallee(fn : Function?) {
        if (inUsedCode && fn != null && fn._module != null) {
            useModules.insert(fn._module);
        }
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
        } elif (decl.baseType == Type.tHandle) {
            //! a handled type decides whether a C++ module is linked, so only code the runtime
            //! program reaches counts - a dead function naming FILE? must not pull fio_core in
            if (inUsedCode && decl.annotation != null) {
                useHandles.insert(decl.annotation);
            }
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
def public funcInfoName(info : FuncInfo?) {
    //! Returns the C++ identifier name for a function's runtime info based on its hash.
    return "__func_info__{info.hash:x}";
}
def structInfoName(info : StructInfo?) {
    return "__struct_info__{info.hash:x}";
}
def typeInfoName(info : TypeInfo?) {
    return "__type_info__{info.hash:x}";
}

class public DebugVarCache {
    //! Cache for deduplicating debug variable declarations in AOT output.
    private argNamesCache : table<string; string>
    private info2Name : table<string; string>

    def emplace(name : string, value : string) : tuple<string, bool> {
        let success = !(argNamesCache |> key_exists(value))
        if (success) {
            argNamesCache |> insert(value, name)
        }
        var res_val : string;
        info2Name |> insert(name, value)
        verify(argNamesCache |> get(value) $(val) {
            info2Name |> insert(name, val)
            res_val = val
        })
        return tuple(res_val, success)
    }

    def get(k : string) : string {
        return info2Name.get_value(k);
    }

    def contains(k : string) : bool {
        return argNamesCache |> key_exists(k);
    }

    def empty() : bool {
        return argNamesCache |> empty();
    }

    def clear() {
        argNamesCache |> clear();
        info2Name |> clear();
    }
}


// `\?` is `?` to a C compiler and hides the ??-sequences it would otherwise read as trigraphs
def private c_literal(s : auto implicit) : string {
    return escape(s) |> replace("?", "\\?")
}


def private writeAnnotationArgInit(var sb : StringBuilderWriter; arg : AnnotationArgumentInfo) {
    if (arg.basicType == Type.tBool) {
        write(sb, "AnnotationArgumentInfo(\"{arg.name}\", {arg.bValue})")
    } elif (arg.basicType == Type.tString) {
        write(sb, "AnnotationArgumentInfo(\"{arg.name}\", \"{c_literal(arg.sValue)}\")")
    } elif (arg.basicType == Type.tInt) {
        write(sb, "AnnotationArgumentInfo(\"{arg.name}\", {arg.iValue})")
    } elif (arg.basicType == Type.tFloat) {
        write(sb, "AnnotationArgumentInfo(\"{arg.name}\", {to_cpp_float(arg.fValue)})")
    }
}

def private writeAnnotationList(var writer : StringBuilderWriter?; info; baseName : string) {
    if (info.annotation_count == 0u) return ;
    for (ai in range(int(info.annotation_count))) {
        unsafe {
            let ann & = get_annotation(*info, ai)
            if (ann.count > 0u) {
                let annArgs = build_string() $(var sb) {
                    for (gi in range(int(ann.count))) {
                        if (gi != 0) { write(sb, ", "); }
                        writeAnnotationArgInit(sb, get_annotation_argument(ann, gi))
                    }
                }
                write(*writer, "AnnotationArgumentInfo {baseName}_ann_{ai}_args[{int(ann.count)}] = \{ {annArgs} \};\n")
            }
        }
    }
    let anns = build_string() $(var sb) {
        for (ai in range(int(info.annotation_count))) {
            if (ai != 0) { write(sb, ", "); }
            unsafe {
                let ann & = get_annotation(*info, ai)
                let argsRef = ann.count > 0u ? "{baseName}_ann_{ai}_args" : "nullptr"
                write(sb, "AnnotationInfo(\"{ann.name}\", \"{ann.module_name}\", {argsRef}, {int(ann.count)}u)")
            }
        }
    }
    write(*writer, "AnnotationInfo {baseName}_ann[{int(info.annotation_count)}] = \{ {anns} \};\n")
}

class public AotDebugInfoHelper {
    //! Helper for generating debug type and variable information in AOT C++ output.
    private info2Name  = new DebugVarCache()
    private info2TypeName = new DebugVarCache()
    private annInfoNames : table<string; string>
    helper : DebugInfoHelper? = new DebugInfoHelper(uninitialized)
    cross_platform : bool = false
    def operator delete {
        unsafe { delete helper; }
    }

    def writeDim(var writer : StringBuilderWriter?; info : TypeInfo?; suffix : string = "") {
        if (info.dimSize > 0u) {
            let dims = (each(range(info.dimSize))
                ._select("{unsafe(info.dim[_]):d}")
                .to_array()
                ._fold()) |> join(", ")
            write(*writer, "uint32_t {typeInfoName(info)}_dim{suffix}[{info.dimSize:d}] = \{ {dims} \};\n");
        }
    }
    def writeArgNames(var writer : StringBuilderWriter?; info : TypeInfo?; suffix : string = "") {
        if (info.argCount > 0u && info.argNames != null) {
            let dims = (each(range(info.argCount))
                ._select("\"{unsafe(info.argNames[_])}\"")
                .to_array()
                ._fold()) |> join(", ")
            let name = "{typeInfoName(info)}_arg_names{suffix}"
            let (_, succ) = info2Name.emplace(name, dims)
            if (succ) {
                write(*writer, "const char * {name}[{info.argCount:d}] = \{ {dims} \};\n");
            }
        }
    }
    def writeArgTypes(var writer : StringBuilderWriter?; info : TypeInfo?; suffix : string = "") {
        if (info.argCount > 0u && info.argTypes != null) {
            let dims = (each(range(info.argCount))
                ._select("&{typeInfoName(unsafe(info.argTypes[_]))}")
                .to_array()
                ._fold()) |> join(", ")
            let name = "{typeInfoName(info)}_arg_types{suffix}"
            let (_, succ) = info2TypeName.emplace(name, dims)
            if (succ) {
                write(*writer, "TypeInfo * {name}[{info.argCount:d}] = \{ {dims} \};\n");
            }
        }
    }

    def str() {
        return build_string() $(var writer) {
            verify(info2Name.empty() && info2TypeName.empty())
            annInfoNames |> clear()
            helper |> debug_helper_iter_structs($(_name, ti) {
                write(writer, "extern StructInfo {structInfoName(ti)};\n");
            });
            helper |> debug_helper_iter_types($(_name, ti){
                write(writer, "extern TypeInfo {typeInfoName(ti)};\n");
            })
            write(writer, "\n");
            writeHandledAnnotations(unsafe(addr(writer)));
            helper |> debug_helper_iter_enums($(_name, ti){
                describeCppEnumInfoValues(unsafe(addr(writer)), ti);
                write(writer, "EnumInfo {enumInfoName(ti)} = \{ {describeCppEnumInfo(ti)} \};\n");
            })
            helper |> debug_helper_iter_structs($(_name, ti){
                describeCppStructInfoFields(unsafe(addr(writer)), ti);
                write(writer, "StructInfo {structInfoName(ti)} = \{{describeCppStructInfo(ti)} \};\n");
            })
            helper |> debug_helper_iter_funcs($(_name, ti){
                describeCppFuncInfoFields(unsafe(addr(writer)), ti);
                write(writer, "FuncInfo {funcInfoName(ti)} = \{{describeCppFuncInfo(ti)} \};\n");
            })
            helper |> debug_helper_iter_types($(_name, ti){
                assume wr = unsafe(addr(writer))
                writeDim(wr, ti);
                writeArgTypes(wr, ti);
                writeArgNames(wr, ti);
                write(writer, "TypeInfo {typeInfoName(ti)} = \{ {describeCppTypeInfo(ti)} \};\n");
            })
            write(writer, "\n");
            write(writer, "static void resolveTypeInfoAnnotations()\n\{\n");
            write(writer, "    vector<TypeInfo> annotations = \{")
            helper |> debug_helper_iter_types($(_name, ti) {
                if (ti._type == Type.tHandle) {
                    write(writer, "{typeInfoName(ti)}, ");
                }
            })
            write(writer, "\};\n");
            write(writer, "    for (auto& ann : annotations) \{\n")
            write(writer, "        ann.resolveAnnotation();\n")
            write(writer, "    \}\n")
            write(writer, "\}\n\n")
            info2Name.clear();
            info2TypeName.clear();
        }
    }


    def registerHandledAnnotation(var writer : StringBuilderWriter?; info : TypeInfo?) {
        if (info._type != Type.tHandle) return ;
        let ann = info.annotation
        let key = "{ann._module.name}::{ann.name}"
        if (annInfoNames |> key_exists(key)) return ;
        let ident = "__handled_ann_{length(annInfoNames)}"
        annInfoNames[key] = ident
        write(*writer, "AnnotationInfo {ident} = \{ \"{ann.name}\", \"{ann._module.name}\", nullptr, 0u \};\n")
    }

    def writeHandledAnnotations(var writer : StringBuilderWriter?) {
        helper |> debug_helper_iter_types($(_name, ti) {
            registerHandledAnnotation(writer, ti);
        })
        helper |> debug_helper_iter_structs($(_name, si) {
            if (si.fields == null) return ;
            for (fi in range(si.count)) {
                registerHandledAnnotation(writer, unsafe(si.fields[fi]));
            }
        })
        helper |> debug_helper_iter_funcs($(_name, fni) {
            if (fni.fields == null) return ;
            for (fi in range(fni.count)) {
                registerHandledAnnotation(writer, unsafe(fni.fields[fi]));
            }
        })
    }

    def annInfoRef(info : TypeInfo?) : string {
        let ann = info.annotation
        return "&" + annInfoNames.get_value("{ann._module.name}::{ann.name}")
    }

    def writeVarAnnotationArgs(var writer : StringBuilderWriter?; fld : VarInfo?; arrName : string) {
        if (fld.annotation_argument_count == 0u) return ;
        let annArgs = build_string() $(var sb) {
            for (ai in range(int(fld.annotation_argument_count))) {
                if (ai != 0) { write(sb, ", "); }
                writeAnnotationArgInit(sb, get_annotation_argument(*fld, ai))
            }
        }
        write(*writer, "AnnotationArgumentInfo {arrName}[{int(fld.annotation_argument_count)}] = \{ {annArgs} \};\n")
    }

    def describeCppVarInfo(struct_name : string; info : VarInfo?; suffix : string) {
        return build_string() $(writer) {
            write(writer, "{describeCppTypeInfo(info, suffix)}, \"{info.name}\", ");
            if (cross_platform) {
                write(writer, "offsetof({struct_name},{aotFieldName(info.name)}), {info.nextGcField:d}");
            } else {
                write(writer, "{info.offset:d}, {info.nextGcField:d}")
            }
        }
    }
    def describeCppVarFuncInfo(struct_name : string; info : VarInfo?; suffix : string) {
        return build_string() $(writer) {
            write(writer, "{describeCppTypeInfo(info, suffix)}, \"{info.name}\", ");
            assert(info.offset == 0u);
            write(writer, "0, {info.nextGcField:d}");
        }
    }
    def describeCppStructInfoFields(var writer : StringBuilderWriter?; info : StructInfo?) {
        writeAnnotationList(writer, info, structInfoName(info));
        if (info.fields == null) return ;
        for (fi in range(info.count)) {
            let suffix = "_var_{info.hash:d}";
            let fld & = unsafe(info.fields[fi])
            writeDim(writer, fld, suffix);
            writeArgTypes(writer, fld, suffix);
            writeArgNames(writer, fld, suffix);
            let ns = aotModuleNameFromString(info.module_name)
            let prefix = !empty(ns) ? "{ns}::" : "";
            writeVarAnnotationArgs(writer, fld, "{structInfoName(info)}_field_{fi}_ann");
            let fldAnnRef = fld.annotation_argument_count > 0u ? ", {structInfoName(info)}_field_{fi}_ann, {int(fld.annotation_argument_count)}u" : ""
            let structCppName = aotSuffixNameEx(info.name, "_S",
                g_struct_enum_collide |> key_exists(structEnumNsKey(ns, info.name)))
            write(*writer, "VarInfo {structInfoName(info)}_field_{fi} =  \{ {describeCppVarInfo(prefix + structCppName, fld,suffix)}{fldAnnRef} \};\n");
        }
        let fields = (each(range(info.count))
            ._select("&{structInfoName(info)}_field_{_}")
            .to_array()
            ._fold()) |> join(", ")
        write(*writer, "VarInfo * {structInfoName(info)}_fields[{info.count:d}] =  \{ {fields} \};\n");
    }

    def describeCppStructInfo(info : StructInfo?) {
        return build_string() $(writer) {
            write(writer, "\"{info.name}\", \"{info.module_name}\", {int(info.flags):d}, ");
            if (info.fields != null) {
                write(writer, "{structInfoName(info)}_fields, ")
            } else {
                write(writer, "nullptr, ")
            }
            let info_size = cross_platform ? "TypeSize<{helper |> debug_helper_find_struct_cppname(info)}>::size" : "{info.size:d}";
            let annRef = info.annotation_count > 0u ? "{structInfoName(info)}_ann" : "nullptr";
            write(writer, "{info.count:d}, {info_size}, UINT64_C(0x{info.init_mnh:x}), {annRef}, {int(info.annotation_count)}u, UINT64_C(0x{info.hash:x}), {info.firstGcField:d}")
        }
    }
    def describeCppFuncInfoFields(var writer : StringBuilderWriter?; info : FuncInfo?) {
        writeAnnotationList(writer, info, funcInfoName(info));
        if (info.fields == null) return ;
        for (fi in range(info.count)) {
            let suffix = "_var_{info.hash:d}";
            let fld & = unsafe(info.fields[fi])
            writeDim(writer, fld, suffix);
            writeArgTypes(writer, fld, suffix);
            writeArgNames(writer, fld, suffix);
            writeVarAnnotationArgs(writer, fld, "{funcInfoName(info)}_field_{fi}_ann");
            let fldAnnRef = fld.annotation_argument_count > 0u ? ", {funcInfoName(info)}_field_{fi}_ann, {int(fld.annotation_argument_count)}u" : ""
            write(*writer, "VarInfo {funcInfoName(info)}_field_{fi} =  \{ {describeCppVarFuncInfo(info.name, fld,suffix)}{fldAnnRef} \};\n");
        }

        let fields = (each(range(info.count))
            ._select("&{funcInfoName(info)}_field_{_}")
            .to_array()
            ._fold()) |> join(", ")
        write(*writer, "VarInfo * {funcInfoName(info)}_fields[{info.count:d}] =  \{ {fields} \};\n");
    }
    def describeCppFuncInfo(info : FuncInfo?) {
        return build_string() $(writer) {
            write(writer, "\"{info.name}\", \"{info.cppName}\", ");
            if (info.fields != null) {
                write(writer, "{funcInfoName(info)}_fields, ")
            } else {
                write(writer, "nullptr, ")
            }
            write(writer, "{info.count:d}, {info.stackSize:d}, &{typeInfoName(info.result)}, nullptr,0,UINT64_C(0x{info.hash:x}), 0x{info.flags:x}");
            if (info.annotation_count > 0u) {
                write(writer, ", {funcInfoName(info)}_ann, {int(info.annotation_count)}u");
            }
        }
    }
    def describeCppEnumInfoValues(var writer : StringBuilderWriter?; einfo : EnumInfo?) {
        for (v in range(einfo.count)) {
            let val = unsafe(einfo.fields[v]);
            let bits = uint64(val.value)
            var cppLit : string
            if (bits == 0x8000000000000000ul) {
                cppLit = "(-INT64_C(0x7fffffffffffffff) - INT64_C(1))"
            } elif (val.value < 0l) {
                let absBits = uint64(-val.value)
                cppLit = "-INT64_C(0x{absBits:x})"
            } else {
                cppLit = "INT64_C(0x{bits:x})"
            }
            write(*writer, "EnumValueInfo {enumInfoName(einfo)}_value_{v} = \{ \"{val.name}\", {cppLit} \};\n")
        }
        let enum_vals = (each(range(einfo.count))
            ._select("&{enumInfoName(einfo)}_value_{_}")
            .to_array()
            ._fold()) |> join(", ")
        write(*writer, "EnumValueInfo * {enumInfoName(einfo)}_values [] = \{ {enum_vals} \};\n");
        writeAnnotationList(writer, einfo, enumInfoName(einfo));
    }
    def describeCppEnumInfo(info : EnumInfo?) {
        let annRef = info.annotation_count > 0u ? ", {enumInfoName(info)}_ann, {int(info.annotation_count)}u" : "";
        return "\"{info.name}\", \"{info.module_name}\", {enumInfoName(info)}_values, {info.count:d}, UINT64_C(0x{info.hash:x}), {info.flags:d}{annRef}";
    }
    def describeCppTypeInfo(info : TypeInfo?; suffix : string = "") {
        return build_string() $(writer) {
            let write_sep = $ { write(writer, ", "); };
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
                write(writer, annInfoRef(info));
            } else {
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
                let name = "{typeInfoName(info)}_arg_types{suffix}"
                write(writer, "(TypeInfo **){info2TypeName.get(name)}")
            } else {
                write(writer, "nullptr");
            }
            write_sep();
            if (info.argCount > 0u && info.argNames != null) {
                write(writer, info2Name.get("{typeInfoName(info)}_arg_names{suffix}"))
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

def public isLocalVec(vtype : TypeDeclPtr) {
    //! Returns true if the type is a non-reference vector type.
    return vtype.isVectorType && !vtype.flags.ref;
}

def public describeLocalCppType(var writer : StringBuilderWriter?; vtype : TypeDeclPtr; cross_platform : bool; substituteRef : CpptSubstitureRef = CpptSubstitureRef.yes; skipConst : CpptSkipConst = CpptSkipConst.no) {
    //! Writes the C++ type representation of a local variable type to a string builder.
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

def public aotFuncName(func : Function?) {
    //! Returns the AOT C++ function name for a daslang function, including its hash suffix.
    if (func.hash != uint64(0)) {
        return "{aotSuffixNameEx(func.name,"_Func")}_{func.hash:x}";
    } else {
        return "{aotSuffixNameEx(func.name,"_Func")}";
    }
}

def private isBlockLet(variable : VariablePtr) : bool {
    if (!variable._type.isGoodBlockType) {
        return false
    }
    assert(variable.init != null && variable.init is ExprMakeBlock, "block local must be initialized with a make block expression")
    return true
}

class public BlockVariableCollector : AstVisitor {
    //! AST visitor that collects block-level variable declarations for AOT code generation.
    def override preVisitExprBlock(blk : ExprBlock?) {
        push(stack, blk as ExprBlock);
    }
    def getVarName(variable : VariablePtr) {
        return rename?[variable] ?? string(variable.name)
    }
    def isMoved(variable : VariablePtr) {
        return moved |> key_exists(variable)
    }
    def renameVariableTo(variable : VariablePtr; newName : string) {
        rename |> insert(variable, newName);
    }

    def override visitExprBlock(var blk : ExprBlock?) : ExpressionPtr {
        stack |> pop();
        return blk;
    }
    def needRenaming(variable : VariablePtr) {
        return true;
    }
    def renameVariable(variable : VariablePtr) {
        if (needRenaming(variable)) {
            rename |> insert(variable, "__{aotSuffixNameEx(variable.name,"_Var")}_rename_at_{variable.at.line:d}_{(tempCounter++):d}");
        }
    }
    def override preVisitExprForVariable(expr : ExprFor?; variable : VariablePtr; last : bool) {
        for (varr in expr.iteratorVariables) {
            renameVariable(varr);
        }
    }
    def override preVisitExprBlockArgument(blk : ExprBlock?; variable : VariablePtr; lastArg : bool) {
        renameVariable(variable);
    }
    def override preVisitFunctionArgument(fn : FunctionPtr; variable : VariablePtr; lastArg : bool) {
        renameVariable(variable);
    }
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
    def getFinalBlock() : ExprBlock? {
        for (i in range(length(stack))) {
            var blk = stack[length(stack) - i - 1];
            if (!empty(blk.finalList)) return blk;
            if (blk.blockFlags.isClosure) return null;
        }
        return null;
    }
    def getTopBlock() : ExprBlock? {
        for (i in range(length(stack))) {
            var blk = stack[length(stack) - i - 1];
            if (blk.blockFlags.isClosure) return blk;
        }
        return stack[0];
    }
    def override preVisitExprLetVariable(let_var : ExprLet?; var variable : VariablePtr; last : bool) {
        var bfinal = getFinalBlock()
        if (bfinal != null) {
            if (isBlockLet(variable)) {
                blockLets[bfinal] |> push(variable);
            }
            bfinal = getTopBlock();
            variables[bfinal] |> push(variable);
            moved |> insert(variable);
        }
        renameVariable(variable);
    }

    def handleExpr(var expr : ExprMakeLocal?) {
        let need_insert = expectsLocalTemp(expr);
        if (need_insert) {
            let blk = getCurrentBlock();
            localTemps[blk] |> push(expr);
        }
    }

    def override preVisitExprMakeArray(var expr : ExprMakeArray?) {
        handleExpr(expr)
    }
    def override preVisitExprMakeTuple(var expr : ExprMakeTuple?) {
        handleExpr(expr)
    }
    def override preVisitExprMakeStruct(var expr : ExprMakeStruct?) {
        handleExpr(expr)
    }
    def override preVisitExprMakeVariant(var expr : ExprMakeVariant?) {
        handleExpr(expr)
    }
    def override preVisitExprNew(var expr : ExprNew?) {
        if (expr.allocate_on_stack) {
            let blk = getCurrentBlock();
            stackNew[blk] |> push(expr);
        }
    }
    def override preVisitExprAscend(var expr : ExprAscend?) {
        if (expr.ascendFlags.allocate_on_stack && !expr.ascendFlags.needTypeInfo) {
            let blk = getCurrentBlock();
            stackNew[blk] |> push(expr);
        }
    }
    def override preVisitExprCall(var expr : ExprCall?) {
        let need_insert = !expr.doesNotNeedSp && expr.stackTop != 0u;
        if (need_insert) {
            let blk = getCurrentBlock();
            localTemps[blk] |> push(expr);
        }
    }
    def override preVisitExprLooksLikeCall(var expr : ExprLooksLikeCall?) {
        if (expr is ExprInvoke) {
            let inv = expr as ExprInvoke;
            let need_insert = !inv.doesNotNeedSp && inv.stackTop != 0u;
            if (need_insert) {
                let blk = getCurrentBlock();
                localTemps[blk] |> push(expr);
            }
        }
    }

    @do_not_delete stack :      array<ExprBlock?>
    @do_not_delete variables :  table<ExprBlock?, array<Variable?>>
    @do_not_delete blockLets :  table<ExprBlock?, array<Variable?>>
    @do_not_delete localTemps : table<ExprBlock?, array<Expression?>>
    @do_not_delete stackNew :   table<ExprBlock?, array<Expression?>>

    rename : table<Variable?, string>
    moved : table<Variable?>
    tempCounter : uint64 = uint64(0)
};

def public describeCppFunc(fn : FunctionPtr; var collector : BlockVariableCollector?; cross_platform : bool; needName : bool = true; needInline : bool = true) {
    //! Returns the C++ function signature string for an AOT-compiled function.
    return build_string() $(writer) {
        if (needInline) {
            write(writer, "inline ");
        }
        describeLocalCppType(unsafe(addr(writer)), fn.result,
            cross_platform, CpptSubstitureRef.no, CpptSkipConst.yes);
        write(writer, " {needName ? aotFuncName(fn) : "(*)"}");
            let prefix = fn.arguments |> empty() ? "" : ", "
            let args_str = (each(fn.arguments)
                ._select(build_string() $(writer2) {
                    if (isLocalVec(_._type)) {
                        describeLocalCppType(unsafe(addr(writer2)), _._type, cross_platform);
                    } else {
                        write(writer2, "{describeCppType(_._type, DescribeConfig(cross_platform=cross_platform))}");
                    }
                    write(writer2, " {(_._type.isRefType) ? "& " : ""}");
                    if (collector != null) {
                        write(writer2, "{collector.getVarName(_)}");
                    }
                })
                .to_array()
                ._fold()) |> join(", ")
            write(writer, " ( Context * __context__{prefix}{args_str} )");
    }
}

enum private ExitKind { None, Break, Continue, Return }

struct private TryExits {
    hasBreak : bool
    hasContinue : bool
    hasReturn : bool
}

struct private TryFlowLevel {
    isLoop : bool
    exits : TryExits
    flag : string
    slot : string
}

struct private TryFlowState {
    stack : array<TryFlowLevel>
    ret_slot : string
    ret_ptr : bool
}

class TryExitScanner : AstVisitor {
    found : TryExits;
    loopDepth : int = 0;

    def override preVisitExprForBody(ffor : ExprFor?) {
        loopDepth ++;
    }
    def override visitExprFor(var ffor : ExprFor?) : ExpressionPtr {
        loopDepth --;
        return ffor;
    }
    def override preVisitExprWhileBody(wh : ExprWhile?; body : ExpressionPtr) {
        loopDepth ++;
    }
    def override visitExprWhile(var wh : ExprWhile?) : ExpressionPtr {
        loopDepth --;
        return wh;
    }
    def override preVisitExprBreak(that : ExprBreak?) {
        found.hasBreak ||= loopDepth == 0;
    }
    def override preVisitExprContinue(that : ExprContinue?) {
        found.hasContinue ||= loopDepth == 0;
    }
    def override preVisitExprReturn(expr : ExprReturn?) {
        found.hasReturn ||= expr.returnFunc != null;
    }
};

def private anyExit(exits : TryExits) {
    return exits.hasBreak || exits.hasContinue || exits.hasReturn;
}

class public CppAot : AstVisitor {
    //! Main AST visitor that generates C++ ahead-of-time compiled code from daslang AST.

    adapter : VisitorAdapter?

    def str() {
        let maybe_nl = type_info |> empty() ? "" : "\n";
        return "\n" + declarations + helper.str() + join(type_info, "\n") + "{maybe_nl}" + join(aot_prefixes, "") + ss |> string_builder_str();
    }

    def clear() {
        ss |> string_builder_clear();
        type_info |> clear();
        aot_prefixes |> clear();
        cachedTi.clear();
    }
    ss : StringBuilderWriter ?;
    declarations : string
    type_info : array<string>;
    aot_prefixes : array<string>;
    lastNewLine : uint64 = -1ul;
    tab : int = 0;
    debugInfoGlobal : int = 0;
    helper : AotDebugInfoHelper ? = new AotDebugInfoHelper();
    program : ProgramPtr;
    @do_not_delete collector : BlockVariableCollector ? = new BlockVariableCollector();
    aotPrefix : table<string>;
    @do_not_delete local_temp_names : table<Expression?; int>;
    @do_not_delete scopes : array<ExprBlock?>;
    flow : TryFlowState;
    prologue : bool = false;
    solidContext : bool = false;
    cross_platform : bool = false;
    fastMath : bool = false;
    aot_filter_function : string;

    cachedTi = new DebugVarCache()

    def newLine() {
        write(*ss, "\n");
    }
    def tabs() {
        return build_string() $(writer) { write_chars(writer, ' ', 4 * tab); }
    }

    def noBracket(expr : ExpressionPtr) {
        return expr.printFlags.topLevel || expr.printFlags.bottomLevel || expr.printFlags.argLevel;
    }
    def override preVisitEnumeration(enu : EnumerationPtr) {
        if (enu.external) {
            write(*ss, "#if 0 // external enum\n");
        }
        write(*ss, "namespace {aotModuleName(enu._module)} \{\n\n");
        write(*ss, "enum class {aotEnumName(enu)} : {das_to_cppString(enu.baseType)} \{\n");
    }
    def override preVisitEnumerationValue(enu : EnumerationPtr; name : das_string; value : ExpressionPtr; last : bool) {
        write(*ss, "    {aotEnumValueName(name)} = {das_to_cppString(enu.baseType)}(");
    }
    def override visitEnumerationValue(enu : EnumerationPtr; name : das_string; value : ExpressionPtr; last : bool) {
        write(*ss, ")");
        if (!last) { write(*ss, ","); };
        write(*ss, "\n");
        return value;
    }
    def override visitEnumeration(enu : EnumerationPtr) {
        write(*ss, "};\n")
        write(*ss, "}\n")
        if (enu.external) {
            write(*ss, "#endif // external enum\n");
        } else {
        }
        return enu;
    }
    def override canVisitStructureFieldInit(st : StructurePtr) : bool {
        return false;
    }

    def override canVisitStructure(st : Structure?) {
        return !st.flags.isTemplate;
    }
    def override preVisitStructure(that : StructurePtr) {
        if (that.flags.cppLayout) {
            write(*ss, "\n#if 0 // skipping structure {that.name} declaration due to CPP layout");
        }
        write(*ss, "namespace {aotModuleName(that._module)} \{\n");
        for (ann in that.annotations) {
            if (ann.annotation is StructureAnnotation) {
                get_struct_aot_prefix(ann.annotation as StructureAnnotation, that, ann.arguments, ss);
            }
        }
        write(*ss, "\nstruct {aotStructName(that)}");
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
        if (that.flags.cppLayout && from != unsafe(reinterpret<Structure const?>(that))) {
            write(*ss, "    /* skipping {decl.name}, from {from.name} */");
        } else {
            write(*ss, "    {describeCppType(decl._type, DescribeConfig(cross_platform=cross_platform,skip_const = true))} {aotFieldName(string(decl.name))};");
            if (decl.flags.parentType) {
                write(*ss, " /* from {from.name} */");
            }
        }
    }
    def override visitStructureField(variable : StructurePtr; decl : FieldDeclaration; last : bool) {
        write(*ss, "\n");
    }
    def override visitStructure(that : StructurePtr) {
        write(*ss, "};\n");
        if (!cross_platform && !that.fields |> empty()) {
            let s_name = aotStructName(that)
            write(*ss, "static_assert(sizeof({s_name})=={that.sizeOf},\"structure size mismatch with DAS\");\n");
            for (tf in that.fields) {
                write(*ss, "static_assert(offsetof({s_name},{aotFieldName(string(tf.name))})=={tf.offset},\"structure field offset mismatch with DAS\");\n");
            }
        }
        for (ann in that.annotations) {
            if (ann.annotation is StructureAnnotation) {
                write_aot_suffix(ann.annotation as StructureAnnotation, that, ann.arguments, ss);
            }
        }
        write(*ss, "}\n");
        if (that.flags.cppLayout) {
            write(*ss, "#endif // end of skipping structure {that.name} declaration due to CPP layout\n");
        }
        return that;
    }
    def override preVisitProgramBody(prog : ProgramPtr; mod : Module?) {
        declarations = ss |> string_builder_str();
        ss |> string_builder_clear();
        write(*ss, "\n");
        prog.getThisModule |> for_each_module_function($(var fn : FunctionPtr) {
            if (!fn.flags.builtIn && !fn.flags.noAot && !fn.moreFlags.isTemplate) {
                write(*ss, "{describeCppFunc(fn,collector,cross_platform)};\n");
            }
        });
        write(*ss, "\n");
    }
    def write_local_temp_type(tmp : Expression?) : void {
        write(*ss, tabs())
        if (tmp._type.flags.ref) {
            write(*ss, describeCppType(tmp._type, DescribeConfig(skip_ref = true,
                skip_const = true, cross_platform = cross_platform)))
        } else {
            describeVarLocalCppType(ss, tmp._type, cross_platform)
        }
    }

    def override preVisitGlobalLet(prog : ProgramPtr) {
        write(*ss, "void __init_script ( Context * __context__, bool __init_shared )\n\{\n");
        tab ++;
        assume temps = collector.localTemps[null];
        for (tmp in temps) {
            write_local_temp_type(tmp)
            write(*ss, " {makeLocalTempName(tmp)};\n");
        }
    }
    def override visitGlobalLet(prog : ProgramPtr) {
        tab --;
        write(*ss, "}\n");
    }
    def override preVisitGlobalLetVariable(variable : Variable?; lastArg : bool) {
        write(*ss, tabs());
        if (!is_used(program, variable)) {
            write(*ss, "/* ");
        }
        if (variable.flags.global_shared) {
            write(*ss, "if ( __init_shared ) ");
        }
        write(*ss, "{variable.flags.global_shared ? "das_shared" : "das_global"}{variable.init != null ? "" : "_zero"}");
        if (solidContext) {
            write(*ss, "_solid");
        }
        let hash_val = solidContext ? "{variable.stackTop:d}" : "0x{variable.getMangledNameHash:x}";
        let cfg = DescribeConfig(skip_ref = true, skip_const = true, redundant_const = true, cross_platform = cross_platform);
        write(*ss, "<{describeCppType(variable._type,cfg)},{hash_val}>(__context__)");
    }
    def override visitGlobalLetVariable(variable : VariablePtr; last : bool) {
        write(*ss, ";");
        if (!is_used(program, variable)) {
            write(*ss, " */");
        }
        write(*ss, "/*{variable.name}*/\n");

        return variable;
    }
    def override preVisitGlobalLetVariableInit(variable : VariablePtr; init : ExpressionPtr) {
        write(*ss, " = ");
    }
    def override canVisitFunction(fun : Function?) {
        if (fun.flags.noAot) return false;
        return !fun.moreFlags.isTemplate;
    }
    def override preVisitFunction(fn : FunctionPtr) {
        flow.ret_ptr = fn.result.flags.ref;
        flow.ret_slot = fn.result.isVoid ? "" : describeCppType(fn.result, DescribeConfig(skip_const = true,
            skip_ref = true, cross_platform = cross_platform)) + (flow.ret_ptr ? " *" : "");
        if (!empty(aot_filter_function) && (fn.name == aot_filter_function || aotFuncName(fn) == aot_filter_function)) {
            write(*ss, "\n/*<<AOT_FUNC_BEGIN>>*/");
        }
        if (fastMath) {
            write(*ss, "\nDAS_FAST_MATH_PUSH");
        }
        write(*ss, "\ninline ");
        describeLocalCppType(ss, fn.result, cross_platform, CpptSubstitureRef.no, CpptSkipConst.yes);
        write(*ss, " {aotFuncName(fn)} ( Context * __context__");
    }
    def override preVisitFunctionBody(fn : FunctionPtr; expr : ExpressionPtr) {
        if (fn.flags.aotNeedPrologue || prologue) {
            write(*ss, " ) \{ das_stack_prologue __prologue(__context__,{fn.totalStackSize:d},\"{fn.name} \" DAS_FILE_LINE);\n");
        } elif (!(expr is ExprBlock)) {
            write(*ss, " ) \{\n");
            tab ++;
            write(*ss, tabs());
        } else {
            write(*ss, " )\n");
        }
    }
    def override preVisitFunctionArgument(fn : FunctionPtr; arg : VariablePtr; last : bool) {
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
        } elif (fn.body != null && !(fn.body is ExprBlock)) {
            write(*ss, ";\n");
            tab --;
            write(*ss, "}\n");
        } else {
            write(*ss, "\n");
        }
        if (fastMath) {
            write(*ss, "DAS_FAST_MATH_POP\n");
        }
        if (!empty(aot_filter_function) && (fn.name == aot_filter_function || aotFuncName(fn) == aot_filter_function)) {
            write(*ss, "/*<<AOT_FUNC_END>>*/\n");
        }
        return fn;
    }
    def makeLocalTempName(expr : ExpressionPtr) {
        if (!local_temp_names |> key_exists(expr)) {
            local_temp_names |> insert(expr, local_temp_names |> length());
        }
        if (expr is ExprMakeLocal ||
            expr is ExprMakeStruct ||
            expr is ExprMakeVariant ||
            expr is ExprMakeArray ||
            expr is ExprMakeTuple ||
            expr is ExprCall ||
            expr is ExprInvoke) {
        } else {
            panic("we should not be here. we need stacktop for the name");
        }
        return "_temp_make_local_{expr.at.line:d}_{expr.at.column:d}_{local_temp_names |> get_value(expr):d}";
    }
    def override preVisitExprBlock(var blk : ExprBlock?) {
        push(scopes, blk);
        blk.blockFlags.finallyBeforeBody = true;
        write(*ss, "\{\n");
        tab ++;
        assume vars = collector.variables[blk];
        for (variable in vars) {
            continue if (isBlockLet(variable))
            write(*ss, "{tabs()}");
            describeVarLocalCppType(ss, variable._type, cross_platform);
            let varname = collector.getVarName(variable);
            var vname = varname
            let constRef = (variable._type.flags.constant &&
                           variable._type.isRefType &&
                           !variable._type.flags.ref)
            if (constRef) {
                vname += "_ConstRef";
            }
            write(*ss, " {vname}; memset((void*)&{vname},0,sizeof({vname}));\n");
            if (constRef) {
                write(*ss, tabs());
                describeLocalCppType(ss, variable._type, cross_platform);
                write(*ss, " & {varname} = {vname};\n");
            }
        }
        collector.localTemps.get(blk) $(temps) {
            for (tmp in temps) {
                write_local_temp_type(tmp)
                let tempName = makeLocalTempName(tmp);
                write(*ss, " {tempName}; {tempName};\n");
            }
        }
        collector.stackNew.get(blk) $(news) {
            for (nw in news) {
                let nm = stackNewName(nw);
                write(*ss, "{tabs()}{stackNewPointeeType(nw, cross_platform)} {nm}; {nm};\n");
            }
        }
        collector.blockLets.get(blk) $(blets) {
            for (variable in blets) {
                emitBlockLet(variable);
            }
        }
    }
    def emitBlockLet(var variable : VariablePtr) {
        let varname = collector.getVarName(variable);
        let constRef = (variable._type.flags.constant &&
                       variable._type.isRefType &&
                       !variable._type.flags.ref)
        let cvname = constRef ? varname + "_ConstRef" : varname;
        var mkb = variable.init as ExprMakeBlock;
        var mblk = mkb._block as ExprBlock;
        mblk.blockFlags.aotSkipMakeBlock = true;
        write(*ss, "{tabs()}auto {cvname}_TempFunctor = ");
        visit_expression(variable.init, adapter);
        write(*ss, ";\n{tabs()}");
        mblk.blockFlags.aotSkipMakeBlock = false;
        mkb.aotFunctorName := cvname + "_TempFunctor";
        write(*ss, "auto {cvname} = ");
        visit_expression(variable.init, adapter);
        write(*ss, ";\n");
        if (constRef) {
            write(*ss, tabs());
            describeLocalCppType(ss, variable._type, cross_platform);
            write(*ss, " & {varname} = {cvname};\n");
        }
        mkb.aotFunctorName := cvname;
        mblk.blockFlags.aotSkipMakeBlock = true;
    }
    def override preVisitExprBlockArgumentInit(blk : ExprBlock?; variable : VariablePtr; init : ExpressionPtr) {
        write(*ss, "\n#if 0\n");
    }
    def override visitExprBlockArgumentInit(blk : ExprBlock?; variable : VariablePtr; init : ExpressionPtr) {
        write(*ss, "\n#endif\n");
        return init;
    }
    def override preVisitExprBlockExpression(blk : ExprBlock?; expr : ExpressionPtr) {
        write(*ss, tabs());
    }
    def override visitExprBlockExpression(blk : ExprBlock?; that : ExpressionPtr) {
        write(*ss, ";")
        newLine();
        return that;
    }
    def override visitExprBlock(var blk : ExprBlock?) : ExpressionPtr {
        tab --;
        write(*ss, "{tabs()}\}")
        blk.blockFlags.finallyBeforeBody = false
        blk.blockFlags.finallyDisabled = false
        scopes |> pop();
        return blk;
    }
    def finallyName(blk : ExprBlock?) {
        return "__finally_{blk.at.line:d}";
    }
    def override preVisitExprBlockFinal(blk : ExprBlock?) {
        write(*ss, "{tabs()}/* finally */ auto {finallyName(blk)}= das_finally([&]()\{\n");
    }
    def override preVisitExprBlockFinalExpression(blk : ExprBlock?; expr : ExpressionPtr) {
        write(*ss, "{tabs()}");
    }
    def override visitExprBlockFinalExpression(blk : ExprBlock?; that : ExpressionPtr) {
        write(*ss, ";");
        newLine();
        return that;
    }
    def override visitExprBlockFinal(blk : ExprBlock?) {
        write(*ss, "{tabs()}/* end finally */ \});\n");
    }
    def override preVisitExprLetVariable(let_ : ExprLet?; var variable : VariablePtr; last : bool) {
        if (isBlockLet(variable)) {
            if (!collector.isMoved(variable)) {
                emitBlockLet(variable);
                write(*ss, tabs());
            }
            return
        }
        let vname = collector.getVarName(variable);
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
            var init_val = isLocalVec(variable._type) ? "v_zero()" : "0";
            if (variable._type.baseType == Type.tFloat16) {
                init_val = "float16_t(0.f)";
            } elif (variable._type.baseType == Type.tBitfield64) {
                init_val = "das::Bitfield64(UINT64_C(0))";
            } elif (variable._type.baseType == Type.tBitfield16) {
                init_val = "das::Bitfield16(uint16_t(0))";
            } elif (variable._type.baseType == Type.tBitfield8) {
                init_val = "das::Bitfield8(uint8_t(0))";
            } elif (variable._type.baseType == Type.tBitfield) {
                init_val = "das::Bitfield(0u)";
            }
            write(*ss, "{cvname} = {init_val}");
        } else {
            if (!collector.isMoved(variable)) {
                write(*ss, "{cvname};");
            }
            write(*ss, "das_zero({cvname})");
        }
    }
    def override visitExprLetVariable(let_ : ExprLet?; variable : VariablePtr; last : bool) {
        if (!last) write(*ss, "; ");
        if (!collector.isMoved(variable) && !isBlockLet(variable)) {
            if (variable._type.flags.constant && variable._type.isRefType && !variable._type.flags.ref) {
                let vname = collector.getVarName(variable);
                write(*ss, ";\n{tabs()}");
                describeLocalCppType(ss, variable._type, cross_platform);
                write(*ss, " & {vname} = {vname}_ConstRef;\n");
            }
        }
        if (variable.init != null && variable.init is ExprMakeBlock) {
            var mkb = variable.init as ExprMakeBlock;
            mkb.aotFunctorName := ""
            (mkb._block as ExprBlock).blockFlags.aotSkipMakeBlock = false;
        }
        return variable;
    }
    def override preVisitExprLetVariableInit(let_ : ExprLet?; variable : VariablePtr; expr : ExpressionPtr) {
        return if (isBlockLet(variable))
        if (variable.flags.init_via_move && variable.init is ExprMakeBlock) {
            write(*ss, " = ");
        } elif (variable.flags.init_via_move) {
            let vname = collector.getVarName(variable);
            var cvname = vname;
            if (variable._type.flags.constant && variable._type.isRefType && !variable._type.flags.ref) {
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
        } elif (variable._type.isPointer && expr._type.isPointer
                && variable._type.firstType != null && expr._type.firstType != null
                && !is_same_type(variable._type.firstType, expr._type.firstType, false, false, false, false)) {
            write(*ss, "das_auto_cast<{describeCppType(variable._type, DescribeConfig(cross_platform=cross_platform))}>::cast(");
        }
        if (expr._type.isString) {
            let maybeRef = variable._type.flags.ref ? " &" : "";
            write(*ss, "(char *{maybeRef})(");
        }
    }
    def override visitExprLetVariableInit(let_ : ExprLet?; variable : VariablePtr; expr : ExpressionPtr) {
        return expr if (isBlockLet(variable))
        if (expr._type.isString) {
            write(*ss, ")");
        }
        if (needPtrCast(variable._type, expr._type, expr)) {
            write(*ss, ")");
        } elif (variable._type.isPointer && expr._type.isPointer
                && variable._type.firstType != null && expr._type.firstType != null
                && !is_same_type(variable._type.firstType, expr._type.firstType, false, false, false, false)) {
            write(*ss, ")");
        }
        if (variable._type.flags.ref) {
            write(*ss, ")");
        }
        if (!expr._type.isPointer && !variable._type.flags.ref && expr._type.isAotAlias && !variable._type.isAotAlias) {
            write(*ss, ")");
        }
        if (variable.flags.init_via_move && !(variable.init is ExprMakeBlock)) {
            write(*ss, ")");
        }
        if (variable._type.flags.constant) {
            write(*ss, ")");
        }
        return expr;
    }
    def override preVisitExprLabel(that : ExprLabel?) {
        write(*ss, "label_{that.labelName}:;");
    }
    def override preVisitExprGoto(that : ExprGoto?) {
        if (that.subexpr == null) {
            write(*ss, "goto label_{that.labelName}");
        } else {
            write(*ss, "switch (");
        }
    }
    def override visitExprGoto(var that : ExprGoto?) : ExpressionPtr {
        if (that.subexpr != null) {
            write(*ss, ") \{\n");
            var si = length(scopes) - 1;
            while (si >= 0) {
                let blk = scopes[si];
                for (ex in blk.list) {
                    if (ex is ExprLabel) {
                        let lab = ex as ExprLabel;
                        write(*ss, "{tabs()}case {lab.labelName}: goto label_{lab.labelName};\n");
                    }
                }
                break if (blk.blockFlags.isClosure);
                si--;
            }
            write(*ss, "{tabs()}default: __context__->throw_error(\"invalid label\");\n");
            write(*ss, "{tabs()}\}");
        }
        return that;
    }
    def override preVisitExprCopy(that : ExprCopy?) {
        write(*ss, "das_copy(");
    }
    def override preVisitExprCopyRight(that : ExprCopy?; right : ExpressionPtr) {
        write(*ss, ",");
    }
    def override visitExprCopy(var that : ExprCopy?) : ExpressionPtr {
        write(*ss, ")");
        return that;
    }
    def override preVisitExprClone(that : ExprClone?) {
        let cfg = DescribeConfig(skip_ref = true, skip_const = true, cross_platform = cross_platform)
        let cfg2 := cfg
        let left_t = describeCppType(that.left._type, cfg);
        let right_t = describeCppType(that.right._type, cfg2);
        write(*ss, "das_clone<{left_t},{right_t}>::clone(");
    }
    def override preVisitExprCloneRight(that : ExprClone?; right : ExpressionPtr) {
        write(*ss, ",");
    }
    def override visitExprClone(var that : ExprClone?) : ExpressionPtr {
        write(*ss, ")");
        return that;
    }
    def override preVisitExprMove(that : ExprMove?) {
        write(*ss, "das_move(");
    }
    def override preVisitExprMoveRight(that : ExprMove?; right : ExpressionPtr) {
        write(*ss, ",");
    }
    def override visitExprMove(var that : ExprMove?) : ExpressionPtr {
        write(*ss, ")");
        return that;
    }
    def override visitExprQuote(var that : ExprQuote?) : ExpressionPtr {
        write(*ss, "\n#error quote( ) was not lowered (daslib/quote) at {describe(that.at)}\n")
        return that;
    }
    def outPolicy(decl : TypeDeclPtr) {
        if (decl.baseType != Type.tHandle){
            write(*ss, "SimPolicy<{das_to_cppString(decl.baseType)}>");
        } else {
            write(*ss, "SimPolicy<{decl.annotation.cppName.empty() ? decl.annotation.name : decl.annotation.cppName}>");
        }
    }
    def isOpPolicy1(that : ExprOp1?) {
        if (is_alpha(first_character(that.op))) return true;
        return that.subexpr._type.isPolicyType;
    }
    def isF16Op1(that : ExprOp1?) {
        return that._type.baseType == Type.tFloat16 || (that._type.isVectorType && that._type.vectorBaseType == Type.tFloat16);
    }
    def override preVisitExprOp1(var that : ExprOp1?) {
        if (!that.func.flags.builtIn || that.func.flags.callBased) {
            that.arguments |> clear();
            var subexpr := that.subexpr
            that.arguments |> emplace(subexpr);
            CallFunc_preVisit(that);
            CallFunc_preVisitCallArg(that, that.subexpr, true);
        } elif (isOpPolicy1(that)) {
            if (isF16Op1(that)) {
                let cfg = DescribeConfig(skip_ref = true, skip_const = true, cross_platform = cross_platform)
                write(*ss, "cast<{describeCppType(that._type, cfg)}>::to(");
            }
            outPolicy(that.subexpr._type);
            write(*ss, "::{opPolicyName(that)}(");
            if (that.subexpr._type.baseType == Type.tFloat16) {
                write(*ss, "cast<float16_t>::from(");
            }
        } else {
            if (that.op != "+++" && that.op != "---") {
                write(*ss, that.op);
            }
            if (!noBracket(that) && !that.subexpr.printFlags.bottomLevel) {
                write(*ss, "(");
            }
        }
    }
    def override visitExprOp1(var that : ExprOp1?) : ExpressionPtr {
        if (!that.func.flags.builtIn || that.func.flags.callBased) {
            CallFunc_visitCallArg(that, that.subexpr, true);
            CallFunc_visit(that);
            that.arguments.clear();
        } elif (isOpPolicy1(that)) {
            if (that.subexpr._type.baseType == Type.tFloat16) {
                write(*ss, ")");
            }
            write(*ss, ",*__context__,nullptr)");
            if (isF16Op1(that)) {
                write(*ss, ")");
            }
        } else {
            if (that.op == "+++" || that.op == "---") {
                peek(that.op) $(op) {
                    write(*ss, slice(op, 0, 2));
                }
            }
            if (!noBracket(that) && !that.subexpr.printFlags.bottomLevel) {
                write(*ss, ")");
            }
        }
        return that;
    }
    def isSetBool(that : ExprOp2?) {
        return ((that.op == "||=" || that.op == "&&=" || that.op == "^^=") &&
                that.right._type.baseType == Type.tBool && that.right._type.isSimpleType);
    }
    def isOpPolicy2(that : ExprOp2?) {
        if (is_alpha(first_character(that.op))
                || that.op == "/" || that.op == "%"
                || that.op == "<<<" || that.op == ">>>" || that.op == "<<<=" || that.op == ">>>="
                || that.op == "<<" || that.op == ">>" || that.op == "<<=" || that.op == ">>=") return true;
        return that._type.isPolicyType || that.left._type.isPolicyType || that.right._type.isPolicyType;
    }
    def opPolicyBase(that : ExprOp2?) {
        if (that._type.isPolicyType) {
            return that._type;
        } elif (that.left._type.isPolicyType) {
            return that.left._type;
        } else {
            return that.right._type;
        }
    }
    def opPolicyName(that : ExprOp?) {
        assume fn = (unsafe(reinterpret<ExprCallFunc?>(that))).func;
        if (fn.flags.builtIn) {
            let bfn = fn as BuiltInFunction;
            return string(bfn.cppName.empty() ? bfn.name : bfn.cppName);
        } else {
            return "/* NotAPolicy */";
        }
    }
    def isRefPolicyOp(th : ExprOp2?) {
        let op = string(th.op);
        return ["+=", "-=", "*=", "/=", "%=",
                "&=", "|=", "^=",
                "&&=", "||=", "^^=",
                "<<=", ">>=", "<<<=", ">>>="
                ]._any(_ == op)
    }

    def override preVisitExprOp2(var that : ExprOp2?) {
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
            if (isRefPolicyOp(that) && (pt.isVectorType || pt.isString || pt.baseType == Type.tFloat16)) {
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
    def override preVisitExprOp2Right(that : ExprOp2?; right : ExpressionPtr) {
        if (!that.func.flags.builtIn || that.func.flags.callBased) {
            CallFunc_visitCallArg(that, that.left, false);
            CallFunc_preVisitCallArg(that, that.right, true);
        } elif (isOpPolicy2(that)) {
            assume pt = opPolicyBase(that);
            if (isRefPolicyOp(that) && (pt.isVectorType || pt.isString || pt.baseType == Type.tFloat16)) {
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
                let that_op = string(that.op)
                match (that_op) {
                    if ("&") { op = "&&"; }
                    if ("|") { op = "||"; }
                    if ("^" || "^^") { op = "!="; }
                    if (_) { op = that_op; }
                }
                write(*ss, " {op} ");
            } else {
                write(*ss, " {that.op} ");
            }
        }
    }
    def override visitExprOp2(var that : ExprOp2?) : ExpressionPtr {
        if (!that.func.flags.builtIn || that.func.flags.callBased) {
            CallFunc_visitCallArg(that, that.right, true);
            CallFunc_visit(that);
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
    def override preVisitExprOp3(that : ExprOp3?) {
        if (!noBracket(that)) {
            write(*ss, "(");
        }
    }
    def override preVisitExprOp3Left(that : ExprOp3?; left : ExpressionPtr) {
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
    def override preVisitExprOp3Right(that : ExprOp3?; right : ExpressionPtr) {
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
    def override visitExprOp3(var that : ExprOp3?) : ExpressionPtr {
        write(*ss, ")");
        if (!noBracket(that)) {
            write(*ss, ")");
        }
        return that;
    }
    def returnLevel(expr : ExprReturn?) : int {
        return expr.returnFunc == null ? -1 : flowLevel(ExitKind.Return);
    }
    def override preVisitExprReturn(expr : ExprReturn?) {
        let at = returnLevel(expr);
        write(*ss, at < 0 ? "return " : "\{ ");
        if (at >= 0 && flow.stack[at].slot != "") {
            write(*ss, "{flow.stack[at].slot} = {flow.ret_ptr ? "&(" : ""}");
        }
        if (expr.returnFlags.moveSemantics) {
            write(*ss, "/* <- */ ");
        }
        if (expr.returnFunc == null && expr._block == null) {
            write(*ss, "/* no function/block for return */ ");
            return ;
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
    def override visitExprReturn(var expr : ExprReturn?) : ExpressionPtr {
        if (expr.returnFunc == null && expr._block == null) {
            write(*ss, "/* no function/block for return */ ");
            return expr;
        }
        let at = returnLevel(expr);
        assume retT = expr.returnFunc != null ? expr.returnFunc.result : expr._block.returnType;
        if (!retT.isVoid) {
            write(*ss, ")");
            if (at >= 0 && flow.ret_ptr) {
                write(*ss, ")");
            }
        }
        if (at >= 0) {
            write(*ss, "{retT.isVoid ? "" : "; "}{flow.stack[at].flag} = {int(ExitKind.Return)}; return; \}");
        }
        return expr;
    }
    def override preVisitExprBreak(that : ExprBreak?) {
        writeExit(ExitKind.Break, "");
    }
    def override preVisitExprContinue(that : ExprContinue?) {
        writeExit(ExitKind.Continue, "");
    }
    def override preVisitExprVar(variable : ExprVar?) {
        if (variable._type.flags.aotAlias) {
            write(*ss, "das_alias<{variable._type.alias}>::from(");
        }
    }
    def override visitExprVar(var variable : ExprVar?) : ExpressionPtr {
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
    def override preVisitExprNullCoalescing(nc : ExprNullCoalescing?) {
        if (nc._type.flags.aotAlias) {
            write(*ss, "das_alias<{nc._type.alias}>::from(");
        }
        let type_str1 = describeCppType(nc._type, DescribeConfig(cross_platform = cross_platform));
        write(*ss, "das_null_coalescing<{type_str1}>::get(");
        if (nc.subexpr._type.isAotAlias) {
            let type_str2 = describeCppType(nc._type, DescribeConfig(cross_platform = cross_platform));
            write(*ss, "({type_str2} *)");
        }
    }
    def override preVisitExprNullCoalescingDefault(nc : ExprNullCoalescing?; expr : ExpressionPtr) {
        write(*ss, ",");
    }
    def override visitExprNullCoalescing(var nc : ExprNullCoalescing?) : ExpressionPtr {
        write(*ss, ")");
        if (nc._type.flags.aotAlias) {
            write(*ss, ")");
        }
        return nc;
    }
    def stringify_variadic_types(types : dasvector`ptr`TypeDecl) {
        return (each(types)
            ._select(describeCppTypeEx(_, DescribeConfig(redundant_const = true, cross_platform = cross_platform), CpptUseAlias.no))
            .to_array()
            ._fold()) |> join(",")
    }

    def get_variant_field(in_field_type : TypeDeclPtr; index : int; is_pointer : bool = false) {
        let field_type = fa_element(in_field_type)
        let type_str = describeCppType(field_type.argTypes[index], DescribeConfig(cross_platform = cross_platform));
        let maybe_ptr = is_pointer ? "_ptr" : "";
        if (cross_platform) {
            return "das_get_auto_variant_field{maybe_ptr}<{type_str},{index},{stringify_variadic_types(field_type.argTypes)}>";
        } else {
            let offset = field_type.get_variant_field_offset(index);
            return "das_get_variant_field{maybe_ptr}<{type_str},{offset},{index}>";
        }
    }

    def override preVisitExprIsVariant(field : ExprIsVariant?) {
        assume var_type = field.value._type;
        write(*ss, "{get_variant_field(var_type, field.fieldIndex)}::is(")
    }
    def override visitExprIsVariant(var field : ExprIsVariant?) : ExpressionPtr {
        write(*ss, ")");
        return field;
    }
    def override preVisitExprAsVariant(field : ExprAsVariant?) {
        assume var_type = field.value._type;
        if (field._type.flags.aotAlias) {
            write(*ss, "das_alias<{field._type.alias}>::from(");
        }
        write(*ss, "{get_variant_field(var_type, field.fieldIndex)}::as(");
    }
    def override visitExprAsVariant(var field : ExprAsVariant?) : ExpressionPtr {
        write(*ss, ",__context__)");
        if (field._type.flags.aotAlias) {
            write(*ss, ")");
        }
        return field;
    }
    def override preVisitExprSafeAsVariant(field : ExprSafeAsVariant?) {
        assume fieldT = field.value._type.isPointer ? field.value._type.firstType :  field.value._type;
        if (fieldT.flags.aotAlias) {
            write(*ss, "das_alias<{fieldT.alias}>::from(");
        }
        write(*ss, "{get_variant_field(fieldT, field.fieldIndex)}::safe_as{(field.skipQQ ? "_ptr" : "")}(")
    }
    def override visitExprSafeAsVariant(var field : ExprSafeAsVariant?) : ExpressionPtr {
        write(*ss, ")");
        assume fieldT = field.value._type.isPointer ? field.value._type.firstType :  field.value._type;
        if (fieldT.flags.aotAlias) {
            write(*ss, ")");
        }
        return field;
    }
    def override preVisitExprSafeField(field : ExprSafeField?) {
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
            let ns = aotModuleName(vtype.structType._module);
            let mod_name = (ns |> empty() ? "" : "{ns}::");
            write(*ss, ",&{mod_name}{aotStructName(vtype.structType)}::{aotFieldName(string(field.name))}>::get(");
        }
    }
    def override visitExprSafeField(var field : ExprSafeField?) : ExpressionPtr {
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
    def get_tuple_field(in_field_type : TypeDeclPtr; index : int; is_pointer : bool = false) {
        let field_type = fa_element(in_field_type)
        let field_tp = describeCppType(field_type.argTypes[index], DescribeConfig(cross_platform = cross_platform));
        let maybe_ptr = is_pointer ? "_ptr" : "";
        if (cross_platform) {
            return "das_get_auto_tuple_field{maybe_ptr}<{field_tp},{index},{stringify_variadic_types(field_type.argTypes)}>";
        } else {
            let field_offset = field_type.get_tuple_field_offset(index);
            return "das_get_tuple_field{maybe_ptr}<{field_tp},{field_offset}>";
        }
    }

    def override preVisitExprField(field : ExprField?) {
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
                if (field.value._type.flags.constant) {
                    write(*ss, "((char *)(const char *)(");
                } else {
                    write(*ss, "(({describeCppType(field._type, DescribeConfig(cross_platform=cross_platform))})(");
                }
            }
            if (field._type.flags.isNativeDim) {
                write(*ss, "das_reinterpret<{describeCppType(field._type, DescribeConfig(skip_ref=true,cross_platform=cross_platform))}>::pass(");
            }
            aot_previsit_get_field(field_type.annotation, ss, string(field.name));
        } elif (field_type.baseType == Type.tPointer) {
            if (field_type.firstType.isHandle) {
                if (field._type.isString) {
                    if (field.value._type.flags.constant) {
                        write(*ss, "((char *)(const char *)(");
                    } else {
                        write(*ss, "(({describeCppType(field._type, DescribeConfig(cross_platform=cross_platform))})(");
                    }
                }
                if (field._type.flags.isNativeDim) {
                    write(*ss, "das_reinterpret<{describeCppType(field._type, DescribeConfig(skip_ref=true,cross_platform=cross_platform))}>::pass(");
                }
                aot_previsit_get_field_ptr(field_type.firstType.annotation, ss, string(field.name));
            } elif (field.value._type.firstType.isTuple) {
                write(*ss, "{get_tuple_field(field_type.firstType, field.fieldIndex, true)}::get(")
            } elif (field.value._type.firstType.isVariant) {
                write(*ss, "{get_variant_field(field_type.firstType, field.fieldIndex, true)}::get(")
            }
        }
    }
    def write_field_access_suffix(var sb : StringBuilderWriter?; field : ExprField?) : void {
        write(*sb, " /*{field.name}*/")
        if (field._type.isString) {
            write(*sb, "))")
        }
        if (field._type.flags.isNativeDim) {
            write(*sb, ")")
        }
    }

    def override visitExprField(var field : ExprField?) : ExpressionPtr {
        if (field.value._type.isBitfield) {
            write(*ss, ",1u << {field.fieldIndex})");
        } elif (field.value._type.isTuple) {
            write(*ss, ")");
        } elif (field.value._type.isVariant) {
            write(*ss, ")");
        } elif (field.value._type.isHandle) {
            aot_visit_get_field(field.value._type.annotation, ss, string(field.name));
            write_field_access_suffix(ss, field);
        } elif (field.value._type.baseType == Type.tPointer) {
            if (field.value._type.firstType.isHandle) {
                aot_type_ann_get_field_ptr(field.value._type.firstType.annotation, ss, string(field.name));
                write_field_access_suffix(ss, field);
            } elif (field.value._type.firstType.isTuple) {
                write(*ss, ")");
            } elif (field.value._type.firstType.isVariant) {
                write(*ss, ")");
            } else {
                write(*ss, "->{aotFieldName(string(field.name))}");
            }
        } else {
            write(*ss, ".{aotFieldName(string(field.name))}");
        }
        if (field._type.flags.aotAlias) {
            write(*ss, ")");
        }
        return field;
    }
    def override preVisitExprAt(expr : ExprAt?) {
        if (expr._type.flags.aotAlias) {
            write(*ss, "das_alias<{expr._type.alias}>::from(");
        }
        if (!(expr.subexpr._type.baseType == Type.tFixedArray || expr.subexpr._type.isGoodArrayType || expr.subexpr._type.isGoodTableType)) {
            let type_str = describeCppType(expr.subexpr._type, DescribeConfig(skip_ref = true, cross_platform = cross_platform));
            write(*ss, "das_index<{type_str}>::at(");
        }
    }
    def override preVisitExprAtIndex(var expr : ExprAt?; index : ExpressionPtr) {
        if (expr.subexpr._type.baseType == Type.tFixedArray || expr.subexpr._type.isGoodArrayType || expr.subexpr._type.isGoodTableType) {
            if (expr.subexpr._type.flags.isNativeDim) {
                write(*ss, "[");
            } else {
                write(*ss, "(");
            }
        } else {
            write(*ss, ",");
        }

    }
    def override visitExprAt(var expr : ExprAt?) : ExpressionPtr {
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
    def override preVisitExprSafeAt(expr : ExprSafeAt?) {
        let isPtr : bool = expr.subexpr._type.isPointer;
        assume seT = isPtr ? expr.subexpr._type.firstType : expr.subexpr._type;
        if ((seT.baseType == Type.tFixedArray || seT.isGoodArrayType || seT.isGoodTableType)) {
            let type_str = describeCppType(seT, DescribeConfig(skip_ref = true, skip_const = true, cross_platform = cross_platform))
            write(*ss, "{type_str}::safe_index(");
        } elif (isPtr && !seT.isVectorType) {
            let type_str = describeCppType(expr.subexpr._type, DescribeConfig(skip_ref = true, cross_platform = cross_platform))
            write(*ss, "das_index<{type_str}>::safe_at(");
        } else {
            let type_str = describeCppType(seT, DescribeConfig(skip_ref = true, cross_platform = cross_platform))
            write(*ss, "das_index<{type_str}>::safe_at(");
        }
        write(*ss, isPtr ? "(" : "&(");
    }
    def override preVisitExprSafeAtIndex(expr : ExprSafeAt?; index : ExpressionPtr) {
        write(*ss, "),");
    }
    def override visitExprSafeAt(var that : ExprSafeAt?) : ExpressionPtr {
        write(*ss, ",__context__)");
        return that;
    }
    def override visitExprFakeContext(var c : ExprFakeContext?) : ExpressionPtr {
        write(*ss, "__context__");
        return c;
    }
    def override visitExprFakeLineInfo(var c : ExprFakeLineInfo?) : ExpressionPtr {
        write(*ss, "((LineInfoArg *)(&LineInfo::g_LineInfoNULL))");
        return c;
    }
    def override visitExprConstPtr(var c : ExprConstPtr?) : ExpressionPtr {
        let pvalue = c.getValue
        if (pvalue != null) {
            write(*ss, "((void *) 0x{unsafe(reinterpret<int64>(pvalue)):x})");
        } else {
            write(*ss, "nullptr");
        }
        return c;
    }
    def override visitExprConstEnumeration(var c : ExprConstEnumeration?) : ExpressionPtr {
        let cfg = DescribeConfig(skip_ref = true, skip_const = true, redundant_const = false, cross_platform = cross_platform);
        let type_str = describeCppType(c._type, cfg);
        write(*ss, type_str);
        var ctext = aotEnumValueName(c.value);
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
    def override visitExprConstInt(var c : ExprConstInt?) : ExpressionPtr {
        write(*ss, "{c.getValue}");
        return c;
    }
    def override visitExprConstInt8(var c : ExprConstInt8?) : ExpressionPtr {
        write(*ss, "{c.getValue}");
        return c;
    }
    def override visitExprConstInt16(var c : ExprConstInt16?) : ExpressionPtr {
        write(*ss, "{c.getValue}");
        return c;
    }
    def override visitExprConstInt64(var c : ExprConstInt64?) : ExpressionPtr{
        if (c.getValue == LONG_MIN) {
            write(*ss, "INT64_MIN");
        } else {
            write(*ss, "INT64_C({c.getValue})");
        }
        return c;
    }
    def override visitExprConstUInt8(var c : ExprConstUInt8?) : ExpressionPtr {
        write(*ss, "0x{uint(c.getValue):x}");
        return c;
    }
    def override visitExprConstUInt16(var c : ExprConstUInt16?) : ExpressionPtr {
        write(*ss, "0x{uint(c.getValue):x}");
        return c;
    }
    def override visitExprConstUInt64(var c : ExprConstUInt64?) : ExpressionPtr {
        write(*ss, "UINT64_C(0x{c.getValue:x})");
        return c;
    }
    def override visitExprConstUInt(var c : ExprConstUInt?) : ExpressionPtr {
        write(*ss, "0x{c.getValue:x}u");
        return c;
    }
    def override visitExprConstBitfield(var c : ExprConstBitfield?) : ExpressionPtr {
        if (c._type.baseType == Type.tBitfield64) {
            write(*ss, "das::Bitfield64(UINT64_C(0x{c.getValue:x}))");
        } elif (c._type.baseType == Type.tBitfield16) {
            write(*ss, "das::Bitfield16(uint16_t(0x{uint(c.getValue):x}))");
        } elif (c._type.baseType == Type.tBitfield8) {
            write(*ss, "das::Bitfield8(uint8_t(0x{uint(c.getValue):x}))");
        } else {
            write(*ss, "das::Bitfield(0x{c.getValue:x}u)");
        }
        return c;
    }
    def override visitExprConstBool(var c : ExprConstBool?) : ExpressionPtr {
        write(*ss, (c.getValue ? "true" : "false"));
        return c;
    }

    def to_cpp_double(val : double) : string {
        if (val == DBL_MIN) return "DBL_MIN"
        if (val == -DBL_MIN) return "(-DBL_MIN)"
        if (val == DBL_MAX) return "DBL_MAX"
        if (val == -DBL_MAX) return "(-DBL_MAX)"
        if (val != val) return "((double)NAN)"        // nolint:LINT007 — canonical IEEE-754 NaN test
        if (val > DBL_MAX) return "((double)INFINITY)"
        if (val < -DBL_MAX) return "((double)(-INFINITY))"
        return "{val:.17e}"
    }

    def writeOutDouble(val : double) {
        write(*ss, to_cpp_double(val));
    }
    def override visitExprConstDouble(var c : ExprConstDouble?) : ExpressionPtr {
        writeOutDouble(c.getValue);
        return c;
    }
    def writeOutFloat(value : float) {
        write(*ss, to_cpp_float(value));
    }
    def override visitExprConstFloat(var c : ExprConstFloat?) : ExpressionPtr {
        writeOutFloat(c.getValue);
        return c;
    }
    def override visitExprConstFloat16(var c : ExprConstFloat16?) : ExpressionPtr {
        write(*ss, "float16_t(");
        writeOutFloat(float(c.value));
        write(*ss, ")");
        return c;
    }
    def override visitExprConstString(var c : ExprConstString?) : ExpressionPtr {
        var blob = false
        peek(c.value) $(text) {
            blob = length(text) != length(c.value)
        }
        if (c.value.empty()) {
            write(*ss, "nullptr");
        } elif (blob) {
            write(*ss, "([]() -> char * \{ static const unsigned char blob[] = \{");
            for (b in to_bytes(c.value)) {
                write(*ss, "{int(b)},");
            }
            write(*ss, "0 \}; return (char *) blob; \}())");
        } else {
            write(*ss, "((char *) \"{c_literal(c.value)}\")");
        }
        return c;
    }
    def override visitExprConstInt2(var c : ExprConstInt2?) : ExpressionPtr {
        let val = c.getValue;
        write(*ss, "int2({val.x},{val.y})");
        return c;
    }
    def override visitExprConstRange(var c : ExprConstRange?) : ExpressionPtr {
        let val = c.getValue;
        write(*ss, "range({val.x:d},{val.y:d})");
        return c;
    }
    def override visitExprConstRange64(var c : ExprConstRange64?) : ExpressionPtr {
        let val = c.getValue;
        write(*ss, "range64({val.x:d}ll,{val.y:d}ll)");
        return c;
    }
    def override visitExprConstInt3(var c : ExprConstInt3?) : ExpressionPtr {
        let val = c.getValue;
        write(*ss, "int3({val.x:d},{val.y:d},{val.z:d})");
        return c;
    }
    def override visitExprConstInt4(var c : ExprConstInt4?) : ExpressionPtr {
        let val = c.getValue;
        write(*ss, "int4({val.x:d},{val.y:d},{val.z:d},{val.w:d})");
        return c;
    }
    def override visitExprConstUInt2(var c : ExprConstUInt2?) : ExpressionPtr {
        let val = c.getValue;
        write(*ss, "uint2({val.x:d},{val.y:d})");
        return c;
    }
    def override visitExprConstURange(var c : ExprConstURange?) : ExpressionPtr {
        let val = c.getValue;
        write(*ss, "urange({val.x:d},{val.y:d})");
        return c;
    }
    def override visitExprConstURange64(var c : ExprConstURange64?) : ExpressionPtr {
        let val = c.getValue;
        write(*ss, "urange64({val.x:d}ull,{val.y:d}ull)");
        return c;
    }
    def override visitExprConstUInt3(var c : ExprConstUInt3?) : ExpressionPtr {
        let val = c.getValue;
        write(*ss, "uint3({val.x:d},{val.y:d},{val.z:d})");
        return c;
    }
    def override visitExprConstUInt4(var c : ExprConstUInt4?) : ExpressionPtr {
        let val = c.getValue;
        write(*ss, "uint4({val.x:d},{val.y:d},{val.z:d},{val.w:d})");
        return c;
    }
    def override visitExprConstFloat2(var c : ExprConstFloat2?) : ExpressionPtr {
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
    def override visitExprConstFloat3(var c : ExprConstFloat3?) : ExpressionPtr {
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
    def override visitExprConstFloat4(var c : ExprConstFloat4?) : ExpressionPtr {
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
    def override canVisitWithAliasSubexpression(_expr : ExprAssume?) : bool {
        return false;
    }
    def override preVisitExprAssume(expr : ExprAssume?) {
        write(*ss, "\n#if 0 // assume {expr.alias} = {describe(expr.subexpr) |> replace("\n", " ")}\n");
    }
    def override visitExprAssume(var expr : ExprAssume?) : ExpressionPtr {
        write(*ss, "\n#endif // assume {expr.alias}\n");
        return expr;
    }
    def override preVisitExprWith(expr : ExprWith?) {
        write(*ss, "\n#if 0 // with, note optimizations are off\n");
    }
    def override preVisitExprWithBody(expr : ExprWith?; body : ExpressionPtr) {
        write(*ss, "\n#endif\n");
    }
    def override preVisitExprWhile(wh : ExprWhile?) {
        write(*ss, "while ( ");
    }
    def override preVisitExprWhileBody(wh : ExprWhile?; body : ExpressionPtr) {
        write(*ss, " )\n{tabs()}");
        flow.stack |> push(TryFlowLevel(isLoop = true));
    }
    def override visitExprWhile(var wh : ExprWhile?) : ExpressionPtr {
        flow.stack |> pop();
        return wh;
    }
    def override preVisitExprIfThenElse(ifte : ExprIfThenElse?) {
        write(*ss, "if ( ");
    }
    def override preVisitExprIfThenElseIfBlock(ifte : ExprIfThenElse?; blk : ExpressionPtr) {
        if (!(blk is ExprBlock)) {
            write(*ss, " ) \{\n");
            tab ++;
            write(*ss, tabs());
        } else {
            write(*ss, " )\n");
            write(*ss, tabs());
        }
    }
    def override preVisitExprIfThenElseElseBlock(ifte : ExprIfThenElse?; blk : ExpressionPtr) {
        if (ifte.if_true != null && !(ifte.if_true is ExprBlock)) {
            write(*ss, ";\n");
            tab --;
            write(*ss, tabs());
            write(*ss, "} else ");
        } else {
            write(*ss, " else ");
        }
        if (!(blk is ExprBlock)) {
            write(*ss, "\{\n");
            tab ++;
            write(*ss, tabs());
        }
    }
    def override visitExprIfThenElse(var ifte : ExprIfThenElse?) : ExpressionPtr {
        var isNotBlock : bool
        if (ifte.if_false != null) {
            isNotBlock = !(ifte.if_false is ExprBlock)
        } else {
            isNotBlock = !(ifte.if_true is ExprBlock)
        }
        if (isNotBlock) {
            write(*ss, ";\n");
            tab --;
            write(*ss, tabs());
            write(*ss, "\}");
        }
        return ifte;
    }
    def is_lattice_elem(t : Type) : bool {
        return t == Type.tFloat16 || t == Type.tInt16 || t == Type.tUInt16 || t == Type.tInt8 || t == Type.tUInt8
    }
    def override preVisitExprSwizzle(expr : ExprSwizzle?) {
        assume srcT = expr.value._type
        if (srcT.isVectorType && is_lattice_elem(srcT.vectorBaseType)) {
            let et = das_to_cppString(srcT.vectorBaseType)
            let type_str = describeCppType(expr._type, DescribeConfig(skip_ref = true, cross_platform = cross_platform))
            if (expr._type.flags.ref) {
                write(*ss, "das_swizzle_ref_small<{type_str},{et},{int(expr.fields[0])}>::swizzle(")
            } else {
                let field_str = (each(expr.fields)
                    ._select("{_:d}")
                    .to_array()
                    ._fold()) |> join(",")
                write(*ss, "das_swizzle_small<{type_str},{et},{field_str}>::swizzle(")
            }
            return
        }
        if (expr._type.flags.ref) {
            let type_str = describeCppType(expr._type, DescribeConfig(skip_ref = true, cross_platform = cross_platform));
            let value_str = describeCppType(expr.value._type, DescribeConfig(skip_ref = true, cross_platform = cross_platform));
            write(*ss, "das_swizzle_ref<{type_str},{value_str},{int(expr.fields[0])}>::swizzle(");
        } else {
            if (length(expr.fields) == 1) {
                let mask = fixed_array('x', 'y', 'z', 'w');
                let is64bit = expr._type.baseType == Type.tInt64 || expr._type.baseType == Type.tUInt64;
                write(*ss, "v_extract_")
                *ss |> write_char(mask[int(expr.fields[0])]);
                if (expr._type.baseType != Type.tFloat) {
                    write(*ss, is64bit ? "i64" : "i");
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
                    let field_str = (each(expr.fields)
                        ._select("{_:d}")
                        .to_array()
                        ._fold()) |> join(",")
                    write(*ss, "das_swizzle<{type_str},{value_str},{field_str}>::swizzle(");
                }
            }
        }
        if (expr.value._type.flags.aotAlias) {
            write(*ss, "das_alias<{expr.value._type.alias}>::from(");
        }
     }
    def override visitExprSwizzle(var expr : ExprSwizzle?) : ExpressionPtr {
        if (expr.value._type.flags.aotAlias) {
            write(*ss, ")");
        }
        assume srcT = expr.value._type
        if (srcT.isVectorType && is_lattice_elem(srcT.vectorBaseType)) {
            write(*ss, ")");
            return expr;
        }
        if (expr._type.flags.ref) {
            write(*ss, ")");
        } else {
            if (length(expr.fields) == 1) {
                if (expr._type.baseType != Type.tFloat) write(*ss, ")");
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
    def outputCallTypeInfo(nArgs : uint; elements : dasvector`ptr`Expression) {
        let debug_info_name = "__tinfo_{debugInfoGlobal++:d}";
        let tinfo_str = (each(elements)
            ._select("&{typeInfoName(helper.helper |> make_type_info(null, _._type))}")
            .to_array()
            ._fold()) |> join(", ")
        let (val, succ) = cachedTi.emplace(debug_info_name, tinfo_str)
        if (!succ) {
            return val
        } else {
            type_info.push(build_string() $(writer) {
                write(writer, "TypeInfo * {debug_info_name}[{nArgs:d}] = \{ {tinfo_str} \};");
            })
            return debug_info_name;
        }
    }
    def override preVisitExprStringBuilder(expr : ExprStringBuilder?) {
        let nArgs = uint(length(expr.elements));
        write(*ss, "das_string_builder");
        if (expr.stringBuilderFlags.isTempString) {write(*ss, "_temp"); }
        write(*ss, "(__context__,SimNode_AotInterop<{nArgs:d}>(");
        if (nArgs != 0u) {
            let debug_info_name = outputCallTypeInfo(nArgs, expr.elements);
            write(*ss, "{debug_info_name}, ");
        }
    }
    def override preVisitExprStringBuilderElement(sb : ExprStringBuilder?; expr : ExpressionPtr; last : bool) {
        write(*ss, "cast<{describeCppType(expr._type,DescribeConfig(cross_platform=cross_platform))}");
        if (expr._type.isRefType && !expr._type.flags.ref) {
            write(*ss, " &");
        }
        write(*ss, ">::from(");
    }
    def override visitExprStringBuilderElement(sb : ExprStringBuilder?; expr : ExpressionPtr; last : bool) {
        write(*ss, ")");
        if (!last) {
            write(*ss, ", ");
        }
        return expr;
    }
    def override visitExprStringBuilder(var expr : ExprStringBuilder?) : ExpressionPtr {
        write(*ss, "))");
        return expr;
    }
    def override preVisitExprTypeDecl(expr : ExprTypeDecl?) {
        write(*ss, "das_typedecl_value<{describeCppType(expr.typeexpr,DescribeConfig(cross_platform=cross_platform))}>()()");
    }
    def override preVisitExprTypeInfo(expr : ExprTypeInfo?) {
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

    def override canVisitExprTypeInfo(expr : ExprTypeInfo?; expr_ : ExpressionPtr) {
        if (expr.macro == null) {
            panic("internal error. we should only be here if there is a macro.");
        }
        return expr.macro |> macro_aot_infix(ss, expr);
    }

    def override visitExprTypeInfo(var expr : ExprTypeInfo?) : ExpressionPtr {
        if (expr.macro == null) {
            panic("internal error. we should only be here if there is a macro.");
        }
        expr.macro |> write_aot_macro_suffix(ss, expr);
        write(*ss, ")");
        return expr;
    }
    def flowLevel(code : ExitKind) : int {
        for (i in range(length(flow.stack))) {
            let at = length(flow.stack) - 1 - i;
            assume lvl = flow.stack[at];
            if (!lvl.isLoop) return lvl.flag != "" ? at : -1;
            if (code != ExitKind.Return) return -1;
        }
        return -1;
    }
    def writeExit(code : ExitKind; slot : string) {
        let at = flowLevel(code);
        if (at >= 0) {
            write(*ss, "\{ {flow.stack[at].flag} = {int(code)}; return; \}");
        } elif (code == ExitKind.Break) {
            write(*ss, "break");
        } elif (code == ExitKind.Continue) {
            write(*ss, "continue");
        } else {
            write(*ss, "return {slot == "" ? "" : (flow.ret_ptr ? "*" : "") + slot}");
        }
    }
    def scanExits(tc : ExprTryCatch?) : TryExits {
        var scanner = new TryExitScanner();
        make_visitor(*scanner) $(adp) {
            tc.try_block |> visit(adp);
            tc.catch_block |> visit(adp);
        }
        return scanner.found;
    }
    def override preVisitExprTryCatch(tc : ExprTryCatch?) {
        var lvl = TryFlowLevel(exits = scanExits(tc));
        if (anyExit(lvl.exits)) {
            lvl.flag = "__try_flow_{int(tc.at.line)}_{int(tc.at.column)}";
            write(*ss, "int32_t {lvl.flag} = {int(ExitKind.None)};\n{tabs()}");
            let at = flowLevel(ExitKind.Return);
            if (at >= 0) {
                lvl.slot = flow.stack[at].slot;
            } elif (lvl.exits.hasReturn && flow.ret_slot != "") {
                lvl.slot = "__try_ret_{int(tc.at.line)}_{int(tc.at.column)}";
                write(*ss, "{flow.ret_slot} {lvl.slot};\n{tabs()}");
            }
        }
        flow.stack |> push(lvl);
        write(*ss, "das_try_recover(__context__, [&]()\n");
        write(*ss, "{tabs()}");
    }
    def override preVisitExprTryCatchCatch(tc : ExprTryCatch?; blk : ExpressionPtr) {
        write(*ss, ", [&]()\n");
        write(*ss, "{tabs()}");
    }
    def override visitExprTryCatch(var tc : ExprTryCatch?) : ExpressionPtr {
        write(*ss, ")");
        let lvl = flow.stack[length(flow.stack) - 1];
        flow.stack |> pop();
        if (!anyExit(lvl.exits)) return tc;
        write(*ss, ";");
        let taken = fixed_array(lvl.exits.hasBreak, lvl.exits.hasContinue, lvl.exits.hasReturn);
        let codes = fixed_array(ExitKind.Break, ExitKind.Continue, ExitKind.Return);
        for (t, code in taken, codes) {
            if (!t) continue;
            write(*ss, "\n{tabs()}if ( {lvl.flag} == {int(code)} ) \{ ");
            writeExit(code, lvl.slot);
            write(*ss, "; \}");
        }
        return tc;
    }
    def isDistinctDeref(ptr2ref : ExprPtr2Ref?) {
        return ptr2ref.subexpr._type != null && ptr2ref.subexpr._type.baseType == Type.tDistinct
    }
    def override preVisitExprPtr2Ref(ptr2ref : ExprPtr2Ref?) {
        if (isDistinctDeref(ptr2ref)) {
            write(*ss, "(");
        } elif (ptr2ref.unsafeDeref) {
            write(*ss, "(*(");
        } else {
            write(*ss, "das_deref(__context__,");
        }
    }
    def override visitExprPtr2Ref(var ptr2ref : ExprPtr2Ref?) : ExpressionPtr {
        if (isDistinctDeref(ptr2ref)) {
            write(*ss, ")");
        } elif (ptr2ref.unsafeDeref) {
            write(*ss, "))");
        } else {
            write(*ss, ",__FILE__,__LINE__)");
        }
        return ptr2ref;
    }
    def override preVisitExprRef2Ptr(ref2ptr : ExprRef2Ptr?) {
        write(*ss, "das_ref(__context__,");
    }
    def override visitExprRef2Ptr(var ref2ptr : ExprRef2Ptr?) : ExpressionPtr {
        write(*ss, ")");
        return ref2ptr;
    }
    def queryByMNH(name : string; hash : uint64) {
        return "Func(__context__->fnByMangledName(/*{name}*/ 0x{hash:x}))";
    }

    def override preVisitExprAddr(expr : ExprAddr?) {
        if (expr.func != null) {
            let mangledName =  expr.func |> get_mangled_name();
            let hash = expr.func.getMangledNameHash;
            write(*ss, queryByMNH(mangledName, hash));
        } else {
            write(*ss, "Func(0 /*nullptr*/)");
        }
    }
    def override preVisitExprCast(expr : ExprCast?) {
        write(*ss, "{(expr.castFlags.upcastCast ? "das_upcast" : "das_cast")}<{describeCppType(expr.castType,DescribeConfig(cross_platform=cross_platform))}>::cast(");
    }
    def override visitExprCast(var expr : ExprCast?) : ExpressionPtr {
        write(*ss, ")");
        return expr;
    }
    def override preVisitExprDelete(edel : ExprDelete?) {
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
    def override preVisitExprDeleteSizeExpression(del : ExprDelete?; expr : ExpressionPtr) {
        write(*ss, ",");
    }

    def override visitExprDelete(var edel : ExprDelete?) : ExpressionPtr {
        write(*ss, ")");
        return edel;
    }
    def override preVisitExprAscend(expr : ExprAscend?) {
        if (expr.ascendFlags.allocate_on_stack && !expr.ascendFlags.needTypeInfo
                && expr._type.firstType.baseType != Type.tHandle) {
            let type_str = describeCppType(expr._type.firstType, DescribeConfig(skip_ref = true, skip_const = true, cross_platform = cross_platform));
            let subexpr_str = describeCppType(expr.subexpr._type, DescribeConfig(skip_ref = true, skip_const = true, cross_platform = cross_platform));
            write(*ss, "das_ascend_stack<{type_str},{subexpr_str}>::make({stackNewName(expr)},");
            return
        }
        let info : TypeInfo?  = (expr.ascendFlags.needTypeInfo
                                ? helper.helper |> make_type_info(null, expr.subexpr._type)
                                : null);
        if (expr._type.baseType == Type.tPointer && expr._type.firstType.baseType == Type.tHandle) {
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
    def override visitExprAscend(var expr : ExprAscend?) : ExpressionPtr {
        write(*ss, ")");
        return expr;
    }
    def override preVisitExprNew(enew : ExprNew?) {
        if (enew.allocate_on_stack) {
            let nm = stackNewName(enew);
            write(*ss, "(memset((void*)&{nm},0,sizeof({nm})), &{nm}");
            return
        }
        if (enew._type.baseType == Type.tFixedArray) {
            let ptrType = fa_element(enew._type)
            if (ptrType.firstType.isHandle) {
                let type_str = describeCppType(ptrType.firstType, DescribeConfig(skip_ref = true, skip_const = true, cross_platform = cross_platform));
                write(*ss, "das_new_dim_handle<{type_str},{enew._type.fixedDim},{ptrType.flags.smartPtr}");
                if (enew.initializer) {
                    panic("internal error. initializer for enew is not supported");
                } else {
                    write(*ss, ">::make(__context__");
                }
            } else {
                let type_str = describeCppType(ptrType.firstType, DescribeConfig(skip_ref = true, skip_const = true, cross_platform = cross_platform));
                write(*ss, "das_new_dim<{type_str},{enew._type.fixedDim}");
                if (enew.initializer) {
                    write(*ss, ">::make_and_init(__context__,[&]() \{ return ");
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
                    CallFunc_preVisit(enew);
                } else {
                    write(*ss, ">::make(__context__");
                }
            }
        }
    }
    def override preVisitExprNewArgument(enew : ExprNew?; arg : ExpressionPtr; last : bool) {
        if (enew.initializer) {
            CallFunc_preVisitCallArg(enew, arg, last);
        }
    }
    def override visitExprNewArgument(enew : ExprNew?; arg : ExpressionPtr; last : bool) {
        if (enew.initializer) {
            CallFunc_visitCallArg(enew, arg, last);
        } else {
            panic("we should not even be here. we are visiting arguments of a new, but it has no initializer???");
        }
        return arg;
    }
    def override visitExprNew(var enew : ExprNew?) : ExpressionPtr {
        if (enew.initializer) {
            CallFunc_visit(enew);
            write(*ss, "; \})");
        } else {
            write(*ss, ")");
        }
        return enew;
    }
    def needTempSrc(expr : ExprMakeLocal?) {
        return expectsLocalTemp(expr);
    }
    def mkvName(expr : ExprMakeVariant?) {
        if (needTempSrc(expr)) {
            return makeLocalTempName(expr);
        } else {
            return "__mkv_{expr.at.line:d}";
        }
    }
    def override preVisitExprMakeVariant(expr : ExprMakeVariant?) {
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
    def override preVisitExprMakeVariantField(expr : ExprMakeVariant?; index : int; decl : MakeFieldDeclPtr; last : bool) {
        let variantType = fa_element(expr._type)
        let variantIndex = find_argument_index(variantType, string(decl.name));
        if (variantIndex == -1) {
            panic("should not infer otherwise");
        }
        let type_str = describeCppType(variantType.argTypes[variantIndex], DescribeConfig(cross_platform = cross_platform));
        write(*ss, "{tabs()}{get_variant_field(variantType, variantIndex)}::set(");
        write(*ss, "{mkvName(expr)}");
        if (length(expr.variants) != 1) write(*ss, "({index},__context__)");
        write(*ss, ") = ");
        if (variantType.argTypes[variantIndex].isPointer && decl.value._type.isPointer && variantType.argTypes[variantIndex].firstType != null && decl.value._type.firstType != null && !is_same_type(variantType.argTypes[variantIndex].firstType, decl.value._type.firstType, false, false, false, false)) {
            write(*ss, "das_auto_cast<{type_str}>::cast(");
        }
    }
    def override visitExprMakeVariantField(expr : ExprMakeVariant?; index : int; decl : MakeFieldDeclPtr; last : bool) {
        let variantType = fa_element(expr._type)
        let variantIndex = find_argument_index(variantType, string(decl.name))
        if (variantType.argTypes[variantIndex].isPointer && decl.value._type.isPointer && variantType.argTypes[variantIndex].firstType != null && decl.value._type.firstType != null && !is_same_type(variantType.argTypes[variantIndex].firstType, decl.value._type.firstType, false, false, false, false)) {
            write(*ss, ")");
        }
        write(*ss, ";\n");
        return decl;
    }
    def override visitExprMakeVariant(var expr : ExprMakeVariant?) : ExpressionPtr {
        write(*ss, "{tabs()}return {mkvName(expr)};\n");
        tab --;
        write(*ss, "{tabs()}\})())");
        return expr;
    }
    def mksName(expr : ExprMakeStruct?) {
        if (needTempSrc(expr)) {
            return makeLocalTempName(expr);
        } else {
            return "__mks_{expr.at.line:d}";
        }
    }
    def override preVisitExprMakeStruct(expr : ExprMakeStruct?) {
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
                let expr_type = describeCppType(expr._type, DescribeConfig(skip_ref = true, skip_const = true, cross_platform = cross_platform));
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
    def override preVisitExprMakeStructField(expr : ExprMakeStruct?; index : int; decl : MakeFieldDeclPtr; last : bool) {
        let mkType = fa_element(expr.makeType)
        write(*ss, "{tabs()}");
        write(*ss, "{decl.flags.moveSemantics ? "das_move((" : "das_copy(("}");
        if (mkType.baseType == Type.tHandle) {
            aot_previsit_get_field(mkType.annotation, ss, string(decl.name));
        }
        write(*ss, "{mksName(expr)}");
        if (length(expr.structs) != 1) write(*ss, "({index},__context__)");
        if (mkType.baseType == Type.tHandle) {
            aot_visit_get_field(mkType.annotation, ss, string(decl.name));
            write(*ss, " /*{decl.name}*/");
        } else {
            write(*ss, ".{aotFieldName(string(decl.name))}");
        }
        write(*ss, "),(");
    }
    def override visitExprMakeStructField(expr : ExprMakeStruct?; index : int; var decl : MakeFieldDeclPtr; last : bool) {
        write(*ss, "));\n");
        return decl;
    }
    def override canVisitExprMakeStructBlock(str : ExprMakeStruct?; expr : ExpressionPtr) { return false; }

    def override visitExprMakeStruct(var expr : ExprMakeStruct?) : ExpressionPtr {
        if (expr._block != null) {
            assert(expr._block is ExprMakeBlock);
            var mkb = expr._block as ExprMakeBlock;
            assert(mkb._block is ExprBlock);
            var blk = unsafe(reinterpret<ExprBlock?>(mkb._block));
            collector.renameVariableTo(blk.arguments[0], mksName(expr));
            write(*ss, "{tabs()}");
            visit_expression(mkb._block, adapter);
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
    def mkaName(expr : ExprMakeArray?) {
        if (!needTempSrc(expr)) {
            return "__mka_{expr.at.line:d}";
        } else {
            return makeLocalTempName(expr);
        }
    }
    def override preVisitExprMakeArray(expr : ExprMakeArray?) {
        let type_str = describeCppType(expr._type, DescribeConfig(skip_ref = true, skip_const = true, cross_platform = cross_platform));
        write(*ss, "(([&]() -> {type_str}{(needTempSrc(expr) ? "&" : "")} \{\n");
        tab ++;
        if (!needTempSrc(expr)) {
            write(*ss, "{tabs()}{describeCppType(expr._type,DescribeConfig(skip_ref=true,cross_platform=cross_platform))} {mkaName(expr)};\n");
        }
        if (!expr.makeFlags.initAllFields || expr.makeArrayOnHeap) {
            write(*ss, "{tabs()}das_zero({mkaName(expr)});\n");
        }
        if (expr.makeArrayOnHeap) {
            let elem_str = describeCppType(expr.recordType, DescribeConfig(skip_ref = true, skip_const = true, cross_platform = cross_platform));
            write(*ss, "{tabs()}builtin_array_resize({mkaName(expr)},{length(expr.values)},sizeof({elem_str}),__context__,nullptr);\n");
        }
    }
    def override preVisitExprMakeArrayIndex(expr : ExprMakeArray?; index : int; init : ExpressionPtr; last : bool) {
        if (init._type.canCopy) {
            write(*ss, "{tabs()}das_copy(({mkaName(expr)}({index},__context__)),(");
        } else {
            write(*ss, "{tabs()}das_move({mkaName(expr)}({index},__context__),");
        }
    }
    def override visitExprMakeArrayIndex(expr : ExprMakeArray?; index : int; init : ExpressionPtr; last : bool) {
        if (init._type.canCopy) {
            write(*ss, "));\n");
        } else {
            write(*ss, ");\n");
        }
        return init;
    }
    def override visitExprMakeArray(var expr : ExprMakeArray?) : ExpressionPtr {
        write(*ss, "{tabs()}return {mkaName(expr)};\n");
        tab --;
        write(*ss, "{tabs()}\})())");
        return expr;
    }
    def mktName(expr : ExprMakeTuple?) {
        if (!needTempSrc(expr)) {
            return "__mkt_{expr.at.line:d}";
        } else {
            return makeLocalTempName(expr);
        }
    }
    def override preVisitExprMakeTuple(expr : ExprMakeTuple?) {
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
    def override preVisitExprMakeTupleIndex(expr : ExprMakeTuple?; index : int; init : ExpressionPtr; last : bool) {
        if (init._type.canCopy) {
            write(*ss, "{tabs()}{get_tuple_field(expr.makeType, index)}::get({mktName(expr)}) = ");
        } else {
            write(*ss, "{tabs()}das_move({get_tuple_field(expr.makeType, index)}::get({mktName(expr)}),");
        }
    }
    def override visitExprMakeTupleIndex(expr : ExprMakeTuple?; index : int; var init : ExpressionPtr; last : bool) {
        if (init._type.canCopy) {
            write(*ss, ";\n");
        } else {
            write(*ss, ");\n");
        }
        return init;
    }
    def override visitExprMakeTuple(var expr : ExprMakeTuple?) : ExpressionPtr {
        write(*ss, "{tabs()}return {mktName(expr)};\n");
        tab --;
        write(*ss, "{tabs()}\})())");
        return expr;
    }
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
            let args = (each(blk.arguments)
                ._select("{describeCppType(_._type,DescribeConfig(cross_platform=cross_platform))}{_._type.isRefType && !_._type.flags.ref ? " &" : ""}")
                .to_array()
                ._fold()) |> join(",")
            let maybe_comma = args |> empty() ? "" : ",";
            write(*ss, "<{type_str}{maybe_comma}{args}>(__context__,{blk.stackTop:d},");
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
            var blk_t = blk |> make_block_type()
            let info = helper.helper |> make_invokable_type_debug_info(blk_t, blk.at);
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

    def override preVisitExprLooksLikeCall(var call : ExprLooksLikeCall?) {   // nolint:STYLE037,STYLE038 - one arm per intrinsic call name
        if (isInvokeWithTemp(call)) {
            write(*ss, "({makeLocalTempName(call)} = (");
        }
        if (call.name == "debug") {
            assume argType = call.arguments[0]._type;
            let info = helper.helper |> make_type_info(null, argType);
            write(*ss, "das_debug(__context__,&{typeInfoName(info)},__FILE__,__LINE__,");
            let maybe_ref = ((argType.isRefType && !argType.flags.ref) ? "&" : "")
            write(*ss, "cast<{describeCppType(argType,DescribeConfig(cross_platform=cross_platform))}{maybe_ref}>::from(");
        } elif (call.name == "assert" || call.name == "verify") {
            let ea = call as ExprAssert;
            if (length(call.arguments) == 1) {
                write(*ss, "{ea.isVerify ? "das_verify" : "das_assert"}(");
            } else {
                write(*ss, "{ea.isVerify ? "das_verifyf" : "das_assertf"}(");
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
                    if (call.arguments[0] is ExprRef2Value && (call.arguments[0] as ExprRef2Value).subexpr is ExprField) {
                        let field = (call.arguments[0] as ExprRef2Value).subexpr as ExprField;
                        methodOffset = field.field.offset;
                        methodName = string(field.field.name);
                    } elif (call.arguments[0] is ExprField) {
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
                    write(*ss, ",offsetof({describeCppType(argType.argTypes[0], cfg)},{aotFieldName(methodName)})");
                } else {
                    write(*ss, ",{methodOffset}/*{methodName}*/");
                }
            }
            write(*ss, ">::invoke");
            if (einv.isCopyOrMove) {
                write(*ss, "_cmres");
            }
            if (length(call.arguments) > 1) {
                let args = (each(range(1, length(call.arguments)))
                    ._select("{describeCppType(call.arguments[_]._type, DescribeConfig(cross_platform=cross_platform))}{(call.arguments[_]._type.isRefType && !call.arguments[_]._type.flags.ref) ? " &" : ""}")
                    .to_array()
                    ._fold()) |> join(",")
                write(*ss, "<{args}>");
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
    def override canVisitExprLooksLikeCallArgument(call : ExprLooksLikeCall?; arg : ExpressionPtr; last : bool) {
        if (!empty(call.arguments) && call.arguments[0] == arg &&  call is ExprInvoke) {
            let inv = call as ExprInvoke;
            if (inv.isInvokeMethod) return false;
        }
        return true;
    }
    def override preVisitExprLooksLikeCallArgument(call : ExprLooksLikeCall?; arg : ExpressionPtr; last : bool) {
        if (call.name == "invoke") {
            assume argType = arg._type;
            if (arg._type.isRefType) {
                if (needsArgPass(arg)) {
                    write(*ss, "das_arg<{describeCppType(argType,DescribeConfig(skip_ref=true,cross_platform=cross_platform))}>::pass(");
                }
            }
        }
    }
    def override visitExprLooksLikeCallArgument(call : ExprLooksLikeCall?; arg : ExpressionPtr; last : bool) {
        if (call.name == "invoke") {
            assume argType = arg._type;
            if (arg._type.isRefType) {
                if (needsArgPass(arg)) {
                    write(*ss, ")");
                }
            }
        }
        if (!last) {
            if (call.name == "debug") {
                write(*ss, "),(");
            } else {
                write(*ss, ",");
            }
        }
        return arg;
    }
    def override visitExprLooksLikeCall(var call : ExprLooksLikeCall?) : ExpressionPtr {
        if (call.name == "debug") {
            write(*ss, "))");
        } elif (call.name == "assert" || call.name == "verify") {
            write(*ss, ",__context__)");
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
        if (isInvokeWithTemp(call)) {
            write(*ss, "))");
        }
        return call;
    }
    def policyArgNeedCast(polType : TypeDeclPtr; argType : TypeDeclPtr) {
        if (argType.isVectorType) return false;
        if (!polType.isHandle) {
            if (polType.isVecPolicyType && argType.isVecPolicyType && argType.baseType != Type.tFloat16) return false;
        }
        return polType.isPolicyType;
    }
    def policyResultNeedCast(polType : TypeDeclPtr; resType : TypeDeclPtr) {
        if (resType.isVoid || !resType.isPolicyType) return false;
        if (resType.isVectorType && is_lattice_elem(resType.vectorBaseType)) return true;
        return policyArgNeedCast(polType, resType);
    }
    def isPolicyBasedCall(call : ExprCall?) {
        let bif = call.func as BuiltInFunction;
        if (call.arguments |> empty() && call.func.result.baseType == Type.tHandle) return false;
        return bif.flags.policyBased;
    }
    def isPolicyBasedCallFunc(call : ExprCallFunc?) {
        if (call.func.flags.builtIn) {
            let bif = call.func as BuiltInFunction;
            if (bif.flags.policyBased) return true;
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
            return empty(bif.cppName);
        }
        if (func.flags.noAot || func.flags.aotHybrid || func.moreFlags.requestJit) return true;
        return func._module != program.getThisModule && !emitsForeignBody(func);
    }
    //! true when this unit emits the C++ body of the foreign function, so a call to it is a plain
    //! C++ call; the regular -aot and -lib units emit no foreign body and answer false
    def emitsForeignBody(func : Function?) : bool {
        return false;
    }
    def needsArgPassType(argType : TypeDeclPtr) {
        return !argType.flags.constant && !argType.isGoodBlockType;
    }
    def needsArgPass(expr : ExpressionPtr) {
        if (expr is ExprMakeBlock) {
            let mkblk = expr as ExprMakeBlock;
            let blk = mkblk._block as ExprBlock;
            if (blk.blockFlags.aotSkipMakeBlock) return false;
        }
        return needsArgPassType(expr._type);
    }
    def isCallWithTemp(call : ExprCallFunc?) {
        if (call is ExprCall) {
            let expr = call as ExprCall;
            return !expr.doesNotNeedSp && expr.stackTop != 0u;
        }
        return false;
    }
    def isInvokeWithTemp(call : ExprLooksLikeCall?) {
        if (call is ExprInvoke) {
            let inv = call as ExprInvoke;
            return !inv.doesNotNeedSp && inv.stackTop != 0u;
        }
        return false;
    }
    def CallFunc_preVisit(call : ExprCallFunc?) {   // nolint:STYLE037,STYLE038 - one arm per call-emission form
        if (call.func.moreFlags.propertyFunction) {
            if (call.func.result.flags.aotAlias) {
                write(*ss, "das_alias<{call.func.result.alias}>::from(");
            }
            if (call.func.result.isString) {
                write(*ss, "(({describeCppType(call.func.result,DescribeConfig(cross_platform=cross_platform))})(");
            }
            write(*ss, "((");
            return ;
        }
        let aotName = get_aot_name(call.func, call);
        for (ann in call.func.annotations) {
            if (ann.annotation is FunctionAnnotation) {
                let pAnn = ann.annotation as FunctionAnnotation;
                if (!(aotPrefix |> key_exists(aotName))) {
                    aot_prefixes.push(build_string() $(writer) {
                        get_func_aot_prefix(pAnn, unsafe(addr(writer)), call);
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
                write(*ss, "(({describeCppType(call.func.result,DescribeConfig(cross_platform=cross_platform))})(");
            }
            let bif = call.func as BuiltInFunction;
            if (call.arguments |> empty() && call.func.result.baseType == Type.tHandle) {
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
                if (!empty(call.arguments)) {
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
        return arg.isString && !arg.flags.ref;
    }
    def CallFunc_preVisitCallArg(call : ExprCallFunc?; arg : ExpressionPtr; is_last : bool) {
        if (call.func.moreFlags.propertyFunction) return ;
        var argIndex = 0;
        for (it in range(length(call.arguments))) {
            if (call.arguments[it] == arg) break;
            argIndex++;
        }
        assert(argIndex != length(call.arguments));
        write(*ss, "{get_aot_arg_prefix(call.func, call, argIndex)}");
        assume argType = call.arguments[argIndex]._type;
        assume funArgType = call.func.arguments[argIndex]._type;
        if (call.func.flags.builtIn && funArgType.isAotAlias) {
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
        if (needStringCast(call.func, funArgType)) {
            write(*ss, "(das_string_cast(");
        }
    }
    def CallFunc_visitCallArg(call : ExprCallFunc?; arg : ExpressionPtr; last : bool) {
        if (call.func.moreFlags.propertyFunction) return ;
        var argIndex = 0;
        for (it in range(length(call.arguments))) {
            if (call.arguments[it] == arg) break;
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
        if (call.func.flags.builtIn && funArgType.isAotAlias) write(*ss, ")");
        write(*ss, "{get_aot_arg_suffix(call.func, call, argIndex)}");
        if (!last) {
            write(*ss, ",");
        }
    }
    def CallFunc_visit(call : ExprCallFunc?) {
        if (call.func.moreFlags.propertyFunction) {
            if (call.func.flags.builtIn) {
                let efn = call.func as ExternalFnBase;
                write(*ss, ").{efn.cppName}())");
            } else {
                peek(call.func.name) $(fn) {
                    assert(starts_with(fn, ".`"));
                    write(*ss, ").{aotFieldName(fn.chop(2, length(fn)))}())");
                }
            }
            if (call.func.result.isString) {
                write(*ss, "))");
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
    def override preVisitExprCall(call : ExprCall?) {
        CallFunc_preVisit(call);
    }
    def override preVisitExprCallArgument(call : ExprCall?; arg : ExpressionPtr; last : bool) {
        CallFunc_preVisitCallArg(call, arg, last);
    }
    def override visitExprCallArgument(call : ExprCall?; arg : ExpressionPtr; last : bool) {
        CallFunc_visitCallArg(call, arg, last);
        return arg;
    }
    def override visitExprCall(var call : ExprCall?) : ExpressionPtr {
        CallFunc_visit(call);
        return call;
    }
    def forSrcName(varName : das_string) {
        return "__{aotSuffixNameEx(varName, "")}_iterator";
    }
    def needLoopName(ffor : ExprFor?) {
        return "__need_loop_{ffor.at.line:d}";
    }
    def override preVisitExprFor(ffor : ExprFor?) {
        write(*ss, "\{\n");
        tab ++;
        let nl = needLoopName(ffor);
        write(*ss, "{tabs()}bool {nl} = true;\n");
    }
    def override preVisitExprForBody(ffor : ExprFor?) {
        flow.stack |> push(TryFlowLevel(isLoop = true));
        let nl = needLoopName(ffor);
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
        if (!(isExprCallFunc(expr))) return false;
        assume call = unsafe(reinterpret<ExprCallFunc?>(expr));
        return call.func != null && call.func.flags.builtIn && call.func._module.name == "$" && (call.name == "count" || call.name == "ucount");
    }
    def override preVisitExprForSource(ffor : ExprFor?; that : ExpressionPtr; last : bool) {
        var idx = 0;
        let idxs = length(ffor.sources);
        for (id in range(idxs)) {
            if (ffor.sources[id] == that) break;
            idx++
        }
        assume src = ffor.sources[idx];
        assume variable = ffor.iteratorVariables[idx];
        write(*ss, "{tabs()}// {variable.name}: {variable._type.describe()}\n");
        if (isCountOrUCount(src)) {
            write(*ss, "{tabs()}das_iterator_{(unsafe(reinterpret<ExprCallFunc?>(src))).func.name} DAS_COMMENT(");
        } else {
            let type_str = describeCppType(src._type, DescribeConfig(substitute_ref = true, skip_ref = true, cross_platform = cross_platform));
            write(*ss, "{tabs()}das_iterator<{type_str}> {forSrcName(variable.name)}((");
        }
    }
    def override visitExprForSource(ffor : ExprFor?; that : ExpressionPtr; last : bool) {
        var idx = 0;
        let idxs = length(ffor.sources);
        for (id in range(idxs)) {
            if (ffor.sources[id] == that) break;
            idx++
        }
        assume src = ffor.sources[idx];
        assume variable = ffor.iteratorVariables[idx];
        if (isCountOrUCount(src)) {
            assume pCall = unsafe(reinterpret<ExprCallFunc?>(src));
            write(*ss, ") {forSrcName(variable.name)}(");
            visit_expression(pCall.arguments[0], adapter);
            write(*ss, ",");
            visit_expression(pCall.arguments[1], adapter);
            write(*ss, ");\n");
        } else {
            write(*ss, "));\n");
        }

        let skipTC : bool = !variable._type.flags.ref
        let cfg = DescribeConfig(substitute_ref = true, skip_const = skipTC, cross_platform = cross_platform)
        let type_str = describeCppType(variable._type, cfg);
        write(*ss, "{tabs()}{type_str} {collector.getVarName(variable)};\n");
        let nl = needLoopName(ffor);
        write(*ss, "{tabs()}{nl} = {forSrcName(variable.name)}.first(__context__,");
        write(*ss, "({collector.getVarName(variable)})");
        write(*ss, ") && {nl};\n");
        return that;
    }
    def override visitExprFor(var ffor : ExprFor?) : ExpressionPtr {
        flow.stack |> pop();
        write(*ss, "\n");
        for (x in range(ffor.iteratorVariables |> length)) {
            assume variable = ffor.iteratorVariables[ffor.iteratorVariables |> length - 1 - x];
            write(*ss, "{tabs()}{forSrcName(variable.name)}.close(__context__,");
            write(*ss, "({collector.getVarName(variable)}));\n");
        }
        tab --;
        write(*ss, "{tabs()}\}");
        return ffor;
    }
};


def private aotEmitStruct(ps : Structure?;
                          emitSet : table<Structure?>;
                          var visited : table<Structure?>;
                          program : ProgramPtr;
                          adapter : VisitorAdapter?) {
    if (visited |> key_exists(ps)) return ;
    visited |> insert(ps);
    for (fld in ps.fields) {
        let ft = fld._type
        if (ft.baseType == Type.tStructure && ft.structType != null && emitSet |> key_exists(ft.structType)) {
            aotEmitStruct(ft.structType, emitSet, visited, program, adapter);
        }
    }
    program |> visit_structure(ps, adapter);
}

def public dumpDependencies(program : ProgramPtr; var aotVisitor : CppAot?; prune_to_used : bool = false) {
    //! Writes forward declarations and dependent type/function definitions for AOT output.
    buildStructEnumCollisions(program);
    var utm = new UseTypeMarker(program := program);
    make_visitor(*utm) $(adapter) {
        visit(program, adapter);
        if (prune_to_used) {                       // a standalone context emits the foreign functions too
            var foreign <- foreignUsedFunctions(program, false)
            for (fn in foreign) {
                visit(fn, adapter)
            }
        }
    }
    let remUS = program._options  |> find_arg("remove_unused_symbols") ?as tBool ?? true;
    let kept <- prune_to_used ? collectUsedModules(program) : table<Module?>()
    program.get_ptr() |> for_each_module_no_order($(pm) {
        if (pm.moduleFlags.fromExtraDependency ||
            (prune_to_used && pm.name != "" && !(kept |> key_exists(pm)))) return ;
        pm |> for_each_structure($(ps) {
            write(*aotVisitor.ss, "namespace {aotModuleName(ps._module)} \{ struct {aotStructName(ps)}; \};\n");
        });
    });
    program.get_ptr() |> for_each_module_no_order($(pm) {
        if (pm == program.getThisModule || pm.moduleFlags.fromExtraDependency) return ;
        pm |> for_each_enumeration($(penum) {
            if (!remUS || utm.useEnums |> key_exists(penum)) {
                program |> visit_enumeration(penum, aotVisitor.adapter)
            } else {
                write(*aotVisitor.ss, "// unused enumeration {penum.name}\n");
            }
        });
        var emitSet : table<Structure?>;
        pm |> for_each_structure($(ps) {
            if (!remUS || utm.useStructs |> key_exists(ps)) {
                emitSet |> insert(ps);
            } else {
                write(*aotVisitor.ss, "// unused structure {ps.name}\n");
            }
        });
        var visited : table<Structure?>;
        pm |> for_each_structure($(ps) {
            if (emitSet |> key_exists(ps)) {
                aotEmitStruct(ps, emitSet, visited, program, aotVisitor.adapter);
            }
        });
    })
    unsafe {
        delete utm
    }
}

class ArgsConverter : AstVisitor {
    //! AST visitor that converts function arguments to C++ AOT calling conventions.
    private arg_id : uint = 0u
    private ss : StringBuilderWriter?
    private cross_platform : bool

    def ArgsConverter(var ss_ : StringBuilderWriter?; cp : bool) { ss = ss_; cross_platform = cp; }

    def override canVisitFunction(fun : Function?) {
        if (fun.flags.noAot) return false;
        return !fun.moreFlags.isTemplate;
    }

    def override preVisitFunction(fn : FunctionPtr) {
        write(*ss, "static vec4f __wrap_{aotFuncName(fn)} ( Context * __context__ ) \{\n")
        assert(arg_id == 0u)
    }

    def override preVisitFunctionArgument(fn : FunctionPtr, arg : VariablePtr, last : bool) {
        let type_name = build_string() $(writer) {
            if (isLocalVec(arg._type)) {
                describeLocalCppType(unsafe(addr(writer)), arg._type, cross_platform)
            } else {
                write(writer, describeCppType(arg._type, DescribeConfig(cross_platform = cross_platform)))
            }
            if (arg._type.isRefType) {
                write(writer, " & ")
            }
        }
        write(*ss, "    {type_name} arg_{aotSuffixNameEx(arg.name, "")} = cast_aot_arg<{type_name}")
        write(*ss, ">::to(*__context__,__context__->abiArguments()[{arg_id |> int}]);\n")
        arg_id++
    }

    def override visitFunction(fn : FunctionPtr) {
        let is_cmres = fn.flags.copyOnReturn || fn.flags.moveOnReturn

        arg_id = 0u
        write(*ss, "    ")
        if (!fn.result.isVoid && !is_cmres) {
            write(*ss, "return cast<")
            describeLocalCppType(ss, fn.result, cross_platform, CpptSubstitureRef.no, CpptSkipConst.yes)
            write(*ss, ">::from(")
        }
        if (is_cmres) {
            write(*ss, "*((")
            describeLocalCppType(ss, fn.result, cross_platform, CpptSubstitureRef.no, CpptSkipConst.yes)
            write(*ss, " *) __context__->abiCMRES) = ")
        }
        write(*ss, "{aotFuncName(fn)}(__context__")
        for (arg in fn.arguments) {
            write(*ss, ", arg_{aotSuffixNameEx(arg.name, "")}")
        }
        write(*ss, ")")
        if (!fn.result.isVoid && !is_cmres) {
            write(*ss, ");\n")
        } else {
            write(*ss, ";\n")
            write(*ss, "    return v_zero();\n")
        }
        write(*ss, "}\n")
        return fn;
    }

}



def collectUsedFunctions(program : ProgramPtr; modules : array<Module?>; totalFunctions : int; this_module : Module?; all_modules : bool; is_all : bool = false) : array<Function?> {
    var fnn : array<Function?>;
    fnn.reserve(totalFunctions);
    for (pm in modules) {
        pm |> for_each_module_function($(pfun) {
            if ((!all_modules && pfun._module != this_module)
                    || function_index(program, pfun) < 0 || !is_used(program, pfun)) return ;
            if (!is_all) {
                if (pfun.flags.builtIn || pfun.flags.noAot) return ;
            }
            fnn |> push(pfun);
        });
    }
    return <- fnn;
}

//! The used functions living outside the entry module. A standalone context is one translation
//! unit, so each is emitted there or nowhere - builtins are C++ already, templates never emit.
//! `before_no_aot_marking` keeps the ones NoAotMarker has yet to judge; after it, `[no_aot]` is out.
def public foreignUsedFunctions(program : ProgramPtr; before_no_aot_marking : bool) : array<Function?> {
    let thisModule = program.getThisModule
    var foreign : array<Function?>
    program.get_ptr() |> for_each_module_no_order($(pm) {
        if (pm == thisModule) return
        pm |> for_each_module_function($(pfun) {
            if (function_index(program, pfun) < 0 || !is_used(program, pfun) || pfun.flags.builtIn || pfun.moreFlags.isTemplate
                    || (!before_no_aot_marking && pfun.flags.noAot)) return
            foreign |> push(pfun)
        })
    })
    return <- foreign
}

def public collectProgramUsedFunctions(program : ProgramPtr; all_modules : bool; is_all : bool) : array<Function?> {
    //! Collects all used functions from a program's modules for AOT code generation.
    var modules : array<Module?>;
    program.get_ptr() |> for_each_module_no_order($(mod) {
        modules.push(mod);
    })
    return collectUsedFunctions(program, modules, program.totalFunctions, program.getThisModule, all_modules, is_all)
}

def registerAotCpp(var logs : StringBuilderWriter?; program : ProgramPtr; var context : Context; cross_platform; headers, all_modules : bool = false) {
    let fnn = collectProgramUsedFunctions(program, all_modules, false);
    var visitor = new ArgsConverter(logs, cross_platform);
    make_visitor(*visitor) $(adapter_marker) {
        visit(program, adapter_marker);
        if (all_modules) {
            // a whole-program table (a standalone context) needs a thunk per foreign row as well;
            // a generic instance several modules share has one name and gets one thunk
            var thunked : table<string>
            for (pfun in collectProgramUsedFunctions(program, false, false)) {
                thunked |> insert(aotFuncName(pfun))
            }
            var foreign <- foreignUsedFunctions(program, false)
            for (fn in foreign) {
                let name = aotFuncName(fn)
                if (thunked |> key_exists(name)) continue
                thunked |> insert(name)
                visit(fn, adapter_marker)
            }
        }
    }
    unsafe {
        delete visitor
    }

    if (!fnn.empty()) {
        write(*logs, "\n#ifdef _MSC_VER\n#pragma optimize(\"\", off)\n#endif\n");
        write(*logs, "struct AotFunction \{ uint64_t hash; bool is_cmres; void * fn; vec4f (*wrappedFn)(Context*); \};\n");
        write(*logs, "static AotFunction functions[] = \{\n");
        for (fn in fnn) {
            let is_cmres = fn.flags.copyOnReturn || fn.flags.moveOnReturn ? "true" : "false";
            let fn_name = aotFuncName(fn);
            write(*logs, "    // {get_aot_hash_comment(fn)}\n");
            write(*logs, "    \{ {fn.aotHash}, {is_cmres}, (void*)&{fn_name}, &__wrap_{fn_name} \},\n");
        }
        write(*logs, "};\n");
        write(*logs, "#ifdef _MSC_VER\n#pragma optimize(\"\", on)\n#endif\n");
    }
    if (headers) {
        write(*logs, "\nvoid registerAot ( AotLibrary & aotLib )\n\{\n")
    } else {
        write(*logs, "\nstatic void registerAotFunctions ( AotLibrary & aotLib ) \{\n")
    }
    if (!fnn.empty()) {
        write(*logs, "    for (const auto &[hash, cmres, fn1, fn2] : functions) \{\n")
        write(*logs, "        aotLibraryEmplace(aotLib, hash, AotFactory(cmres, fn1, fn2));\n")
        write(*logs, "    \}\n")
    }

    if (headers) {
        write(*logs, "}\n");
    } else {
        write(*logs, "    resolveTypeInfoAnnotations();\n");
        write(*logs, "}\n");
    }
}

def public setAotHashes(program : ProgramPtr; ctx : Context) {
    //! Computes semantic and AOT hashes for all used functions in the program.
    // compute semantic hash for each used function
    var fni = 0;
    program.get_ptr() |> for_each_module_no_order($(mod) {
        mod |> for_each_module_function($(pfun) {
            let pindex = function_index(program, pfun)
            if (pindex < 0 || !is_used(program, pfun)) return ;
            pfun.hash = get_function_hash_by_id(pfun, pindex, unsafe(reinterpret<void?>(ctx)));
            fni++;
        })
    });
    program.get_ptr() |> for_each_module_no_order($(mod) {
        mod |> for_each_module_function($(pfun) {
            if (function_index(program, pfun) < 0 || !is_used(program, pfun)) return ;
            pfun.aotHash = get_function_aot_hash(pfun);
        });
    });
}

def public dumpRegisterAot(var tw : StringBuilderWriter?; program : ProgramPtr; var context : Context; allModules : bool; cross_platform : bool) {
    //! Writes the AOT registration code and function list to the string builder.
    registerAotCpp(tw, program, context, cross_platform, false, allModules);
    write(*tw, "\n");
    write(*tw, "static AotListBase impl(registerAotFunctions);\n");
}

//! The builtin C++ modules in the order `register_builtin_modules_impl` (`src/builtin/modules.cpp`)
//! registers them. A module constructor requires `strings` / `rtti_core` by name, so this order is
//! the one every host already relies on; it goes first in a standalone context's registration.
let public DEFAULT_MODULE_ORDER = [
    "Module_BuiltIn", "Module_Math", "Module_Strings", "Module_Rtti", "Module_Ast", "Module_Jit",
    "Module_Debugger", "Module_Network", "Module_UriParser", "Module_JobQue", "Module_FIO", "Module_DASBIND"
]

def private isCppModule(mod : Module?) : bool {
    return !empty(mod.cppClassName)
}

def private canAotModule(mod : Module?) : bool {
    var ok = false
    build_string() $(wr) {
        ok = mod |> aot_require(unsafe(addr(wr)))
    }
    return ok
}

def private markTypeModules(var used : table<Module?>; var seen : table<Structure?>; t : TypeDeclPtr) {
    if (t == null) {
        return
    }
    if (t.annotation != null && t.annotation._module != null) {
        used |> insert(t.annotation._module)
    }
    if (t.enumType != null && t.enumType._module != null) {
        used |> insert(t.enumType._module)
    }
    markStructModules(used, seen, t.structType)
    markTypeModules(used, seen, t.firstType)
    markTypeModules(used, seen, t.secondType)
    for (a in t.argTypes) {
        markTypeModules(used, seen, a)
    }
}


//! A used structure keeps its field types' modules alive: a field of a handled type is the only
//! thing that reaches the module owning it, and dropping that module drops its aot_require include.
def private markStructModules(var used : table<Module?>; var seen : table<Structure?>; st : Structure?) {
    if (st == null || (seen |> key_exists(st))) {
        return
    }
    seen |> insert(st)
    if (st._module != null) {
        used |> insert(st._module)
    }
    for (fld in st.fields) {
        markTypeModules(used, seen, fld._type)
    }
}


def private collectUsedModules(program : ProgramPtr) : table<Module?> {
    //! Modules owning a function (externs included), struct, enum or handled type the runtime
    //! program still reaches.
    var used : table<Module?>
    program.get_ptr() |> for_each_module_no_order($(pm) {
        pm |> for_each_module_function($(pfun) {
            if (function_index(program, pfun) >= 0 && is_used(program, pfun) && pfun._module != null) {
                used |> insert(pfun._module)
            }
        })
    })
    var utm = new UseTypeMarker(program := program)
    make_visitor(*utm) $(adapter) {
        visit(program, adapter)
        var foreign <- foreignUsedFunctions(program, false)
        for (fn in foreign) {
            visit(fn, adapter)
        }
    }
    var seenStructs : table<Structure?>
    for (st in keys(utm.useStructs)) {
        markStructModules(used, seenStructs, st)
    }
    // a standalone context emits its own module's structures even where nothing reaches them
    program.getThisModule |> for_each_structure($(ps) {
        markStructModules(used, seenStructs, ps)
    })
    for (en in keys(utm.useEnums)) {
        if (en._module != null) {
            used |> insert(en._module)
        }
    }
    for (ann in keys(utm.useHandles)) {
        if (ann._module != null) {
            used |> insert(ann._module)
        }
    }
    for (mod in keys(utm.useModules)) {
        used |> insert(mod)
    }
    unsafe {
        delete utm
    }
    return <- used
}

//! a module of a standalone context's registration list, with its cross-TU registration rank
struct public StandaloneModuleEntry {
    mod : Module?
    rank : int
}

def private insertWithDependencies(var linked, visited : table<Module?>; var mod : Module?) {
    //! `visited` tracks the walk, not membership: a module already linked as used still has
    //! its dependencies to bring in
    if (visited |> key_exists(mod)) return
    visited |> insert(mod)
    linked |> insert(mod)
    module_for_each_dependency(mod) $(dep; _pub) {
        if (isCppModule(dep) && canAotModule(dep)) {
            insertWithDependencies(linked, visited, dep)
        }
    }
}

def private cppDependencyDepth(linked : table<Module?>; var visiting : table<Module?>; var mod : Module?) : int {
    //! 1 + the deepest linked C++ dependency - a rank that puts dependencies first across TUs;
    //! `visiting` cuts dependency cycles (modules require each other), which count as depth 0
    if (visiting |> key_exists(mod)) return 0
    visiting |> insert(mod)
    var depth = 0
    module_for_each_dependency(mod) $(dep; _pub) {
        if (isCppModule(dep) && (linked |> key_exists(dep))) {
            let depDepth = cppDependencyDepth(linked, visiting, dep)
            if (depDepth > depth) {
                depth = depDepth
            }
        }
    }
    visiting |> erase(mod)
    return depth + 1
}

def public collectLinkedModules(program : ProgramPtr) : table<Module?> {
    //! Modules a standalone context links: what the runtime program uses, and - once that reaches a
    //! C++ module beyond the builtin one - every linked C++ module's dependencies, because module
    //! constructors require those by name; a default module the program never reaches stays out.
    var linked <- collectUsedModules(program)
    var reachesCpp = false
    for (mod in keys(linked)) {
        if (isCppModule(mod) && mod.cppClassName != "Module_BuiltIn") {
            reachesCpp = true
        }
    }
    if (!reachesCpp) {
        return <- linked
    }
    var cppModules : array<Module?>
    program.get_ptr() |> for_each_module($(mod) {
        if (mod.name != "" && isCppModule(mod)) {
            cppModules |> push(mod)
        }
    })
    var visited : table<Module?>
    for (mod in cppModules) {
        if ((linked |> key_exists(mod)) && canAotModule(mod)) {
            insertWithDependencies(linked, visited, mod)
        }
    }
    return <- linked
}

def public standaloneModuleRegistration(program : ProgramPtr) : array<StandaloneModuleEntry> {
    //! The C++ modules a standalone context registers: the builtin set first in
    //! `DEFAULT_MODULE_ORDER`, then the rest dependencies-first; the rank orders the same way
    //! across TUs. Empty when the program reaches no C++ module beyond the builtin one.
    let linked <- collectLinkedModules(program)
    var ordered : array<StandaloneModuleEntry>
    var visited : table<Module?>
    var cppModules : array<Module?>
    program.get_ptr() |> for_each_module($(mod) {
        if (mod.name != "" && isCppModule(mod) && (linked |> key_exists(mod))) {
            cppModules |> push(mod)
        }
    })
    if (length(cppModules) <= 1) {
        return <- ordered
    }
    ordered |> reserve(length(cppModules))
    var rank = 0
    for (className in DEFAULT_MODULE_ORDER) {
        for (mod in cppModules) {
            if (mod.cppClassName == className && !(visited |> key_exists(mod))) {
                visited |> insert(mod)
                ordered |> push(StandaloneModuleEntry(mod = mod, rank = rank))
            }
        }
        rank++
    }
    var rest : array<Module?>
    for (mod in cppModules) {
        appendDependenciesFirst(rest, visited, linked, mod)
    }
    for (mod in rest) {
        var visiting : table<Module?>
        ordered |> push(StandaloneModuleEntry(mod = mod, rank = length(DEFAULT_MODULE_ORDER) + cppDependencyDepth(linked, visiting, mod)))
    }
    return <- ordered
}

def private appendDependenciesFirst(var ordered : array<Module?>; var visited : table<Module?>; linked : table<Module?>; var mod : Module?) {
    if (visited |> key_exists(mod)) return
    visited |> insert(mod)
    module_for_each_dependency(mod) $(dep; _pub) {
        if (linked |> key_exists(dep) && isCppModule(dep)) {
            appendDependenciesFirst(ordered, visited, linked, dep)
        }
    }
    ordered |> push(mod)
}

def public getRequiredModulesFor(program : ProgramPtr; prune_to_used : bool = false) {
    //! Collects required module declarations and checks for no-AOT modules in the program.
    //! `prune_to_used` drops modules the runtime program never reaches - standalone only - and then
    //! the linked set alone decides; `fromExtraDependency` is a host-process fact (ARCHITECTURE_EMIT sec. 5).
    var modules_str : array<string>
    var noAotModule = false
    let linked <- prune_to_used ? collectLinkedModules(program) : table<Module?>()
    program.get_ptr() |> for_each_module($(mod) {
        if (mod.name == "") {
        } elif (prune_to_used && mod.name != "$" && mod.name != "builtin" && !(linked |> key_exists(mod))) {
            modules_str |> push(" // require {mod.name} - compile time only, not linked\n")
        } elif (!prune_to_used && mod.moduleFlags.fromExtraDependency && mod.name != "builtin") {
        } else {
            modules_str |> push(build_string() $(wr) {
                write(wr, " // require {mod.name}\n")
                if (!(mod |> aot_require(unsafe(addr(wr))))) {
                    write(wr, "  // AOT disabled due to this module\n");
                    noAotModule = true;
                }
            });
        }
    });
    return (modules_str, noAotModule)
}

def public compile_and_simulate(input : string, var access; var mg : ModuleGroup?, cop, blk) {
    //! Compiles a daslang file, simulates it, and invokes a callback with the program and context.
    let wasInAot = is_in_aot()
    set_aot();
    compile_file(input, access, mg, cop) $(ok; var program : smart_ptr<Program>; issues) {
        if (!ok) {
            to_log(LOG_ERROR, "failed to compile {input}\n{issues}\n")
            return
        }
        simulate(program) $(sok; var pctx : smart_ptr<Context>; serrors) {
            if (!sok) {
                to_log(LOG_ERROR, "failed to simulate {input}\n{serrors}\n")
                panic("Failed to simulate {serrors}")
            }
            blk(program, pctx)
        }
    }
    if (!wasInAot) reset_aot()
}


def public log_aot_emit_errors(program : ProgramPtr) : bool {
    //! Logs collected emit errors; returns true when the program has any.
    if (!program.flags.macroException && !program.flags.failToCompile) {
        return false
    }
    for (err in program.errors) {
        var msg = "{describe(err.at)}: error[{int(err.cerr)}]: {err.what}"
        if (!empty(err.extra)) {
            msg += " -- {err.extra}"
        }
        if (!empty(err.fixme)) {
            msg += " -- {err.fixme}"
        }
        to_log(LOG_ERROR, "{msg}\n")
    }
    return true
}

def public fail_on_aot_emit_error(program : ProgramPtr) {
    if (log_aot_emit_errors(program)) {
        panic("AOT codegen failed: {length(program.errors)} codegen error(s) during emission (see log)")
    }
}

[export]
def public run_aot(prog : Program?; var pctx : Context?; cop : CodeOfPolicies) : string {   // nolint:STYLE038 - linear prologue emission, one write per AOT section
    //! Generates the complete AOT C++ source code string for a compiled and simulated program.
    let program <- unsafe(reinterpret<ProgramPtr>(prog));
    return build_string() $(writer) {
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
            write(writer, AOT_INCLUDES);
            write(writer, join(modules_str, ""));
            write(writer, "\n#endif\n");
        } else {
            write(writer, AOT_INCLUDES);
            write(writer, "{join(modules_str, "")}\n");
            write(writer, AOT_HEADERS);
            write(writer, "namespace das \{\n");
            write(writer, "namespace {program.thisNamespace} \{\n");
            build_string() $(tmp_writer) {
                var marker_vis = new NoAotMarker();
                make_visitor(*marker_vis) $(adapter_marker) {
                    visit(program, adapter_marker);
                }
                report_unproven_collect_carriers(program, false)

                var coll = new BlockVariableCollector();
                make_visitor(*coll) $(adapter_coll) {
                    visit(program, adapter_coll)
                }

                var pmarker = new PrologueMarker();
                make_visitor(*pmarker) $(adapter_p) {
                    visit(program, adapter_p)
                }

                var flags = new SetPrinterFlags();
                make_visitor(*flags) $(flags_adapter) {
                    visit(program, flags_adapter)
                }


                setAotHashes(program, *pctx)

                var cpp_aot = new CppAot(ss = unsafe(addr(tmp_writer)),
                                            program := program,
                                            collector = coll,
                                            prologue = program._options |> find_arg("aot_prologue") ?as tBool ?? false ||
                                                    program.getDebugger,
                                            solidContext = program.policies.solid_context ||
                                                    (program._options |> find_arg("solid_context") ?as tBool ?? false),
                                            cross_platform = cop.cross_platform,
                                            fastMath = program._options |> find_arg("fast_math") ?as tBool ?? program.policies.fast_math)
                cpp_aot.helper.cross_platform = cop.cross_platform;
                make_visitor(*cpp_aot) $(adapter) {
                    cpp_aot.adapter := adapter
                    dumpDependencies(program, cpp_aot);
                    visit(program, cpp_aot.adapter, true)
                }
                if (!program.flags.macroException && !program.flags.failToCompile) {
                    write(writer, "{cpp_aot.str()}");
                }
                unsafe {
                    delete cpp_aot.helper.helper
                    delete cpp_aot.collector
                    delete cpp_aot
                    delete flags
                    delete pmarker
                    delete marker_vis
                }
                fail_on_aot_emit_error(program)
            }
            registerAotCpp(unsafe(addr(writer)), program, *pctx, cop.cross_platform, false);
            write(writer, "\n");
            if (!cop.aot_lib) write(writer, "static AotListBase impl(registerAotFunctions);\n");
            if (cop.paranoid_validation) {
            }
            write(writer, "\} // namespace {program.thisNamespace}\n");
            if (cop.aot_lib) {
                write(writer, "AotListBase impl_aot_{program.getThisModule.name}({program.thisNamespace}::registerAotFunctions);\n");
            }
            write(writer, "\} // namespace das\n");
            write(writer, AOT_FOOTER);
        }
    }
}


let AOT_FUNC_BEGIN = "/*<<AOT_FUNC_BEGIN>>*/"
let AOT_FUNC_END = "/*<<AOT_FUNC_END>>*/"

[export]
def public run_aot_function(prog : Program?; var pctx : Context?; cop : CodeOfPolicies; func_name : string) : string {
    //! Generates AOT C++ for a single function, identified by name.
    //! Returns the extracted function body, or empty string if not found.
    let program <- unsafe(reinterpret<ProgramPtr>(prog));
    return build_string() $(writer) {
        let (modules_str, noAotModule) = getRequiredModulesFor(program)
        if ((program._options |> find_arg("no_aot") ?as tBool ?? false) || noAotModule) return
        build_string() $(tmp_writer) {
            var marker_vis = new NoAotMarker();
            make_visitor(*marker_vis) $(adapter_marker) {
                visit(program, adapter_marker);
            }
            report_unproven_collect_carriers(program, false)

            var coll = new BlockVariableCollector();
            make_visitor(*coll) $(adapter_coll) {
                visit(program, adapter_coll)
            }

            var pmarker = new PrologueMarker();
            make_visitor(*pmarker) $(adapter_p) {
                visit(program, adapter_p)
            }

            var flags = new SetPrinterFlags();
            make_visitor(*flags) $(flags_adapter) {
                visit(program, flags_adapter)
            }

            setAotHashes(program, *pctx)

            var cpp_aot = new CppAot(ss = unsafe(addr(tmp_writer)),
                                        program := program,
                                        collector = coll,
                                        prologue = program._options |> find_arg("aot_prologue") ?as tBool ?? false ||
                                                program.getDebugger,
                                        solidContext = program.policies.solid_context ||
                                                (program._options |> find_arg("solid_context") ?as tBool ?? false),
                                        cross_platform = cop.cross_platform,
                                        fastMath = program._options |> find_arg("fast_math") ?as tBool ?? program.policies.fast_math,
                                        aot_filter_function = func_name)
            cpp_aot.helper.cross_platform = cop.cross_platform;
            make_visitor(*cpp_aot) $(adapter) {
                cpp_aot.adapter := adapter
                dumpDependencies(program, cpp_aot);
                visit(program, cpp_aot.adapter, true)
            }
            var full_output = ""
            if (!program.flags.macroException && !program.flags.failToCompile) {
                full_output = cpp_aot.str()
            }
            unsafe {
                delete cpp_aot.helper.helper
                delete cpp_aot.collector
                delete cpp_aot
                delete flags
                delete pmarker
                delete marker_vis
            }
            fail_on_aot_emit_error(program)
            let begin_pos = find(full_output, AOT_FUNC_BEGIN)
            if (begin_pos >= 0) {
                let content_start = begin_pos + length(AOT_FUNC_BEGIN)
                let end_pos = find(full_output, AOT_FUNC_END, content_start)
                if (end_pos >= 0) {
                    write(writer, slice(full_output, content_start, end_pos))
                }
            }
        }
    }
}

[export, unused_argument(paranoid_validation), unused_argument(cross_platform)]
def public aot(input : string; paranoid_validation : bool, cross_platform : bool; cop : CodeOfPolicies) : string {
    //! Compiles, simulates, and generates AOT C++ code for a daslang input file.
    var result = "";
    using() $(var mg : ModuleGroup) {
        var inscope access <- make_file_access("")
        compile_and_simulate(input, access, unsafe(addr(mg)), cop) $(program : ProgramPtr; var pctx : smart_ptr<Context>) {
            result = run_aot(program.get_ptr(), pctx.get_ptr(), cop)
        }
    }
    return result
}
