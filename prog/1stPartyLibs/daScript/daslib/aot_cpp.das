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

def aotFieldName(name : string) : string {
    var result = replace(name, "`", "__");
    // Escape names that clash with C++ keywords (e.g. `char`, `register`, `extern`)
    // or with Windows SDK macros (e.g. `DELETE` from winnt.h).
    if (is_cpp_keyword(result) || result == "DELETE") {
        result = "_f_" + result;
    }
    return result;
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
        if (int(fields[i - 1]) + 1 != int(fields[i])) return false;
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
enum CpptSkipRef {
    //! Controls whether references are skipped in C++ type printing.
    no
    yes
};
enum CpptSkipConst {
    //! Controls whether const qualifiers are skipped in C++ type printing.
    no
    yes
};
enum CpptRedundantConst {
    //! Controls whether redundant const qualifiers are emitted in C++ type printing.
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

def aotSuffixNameEx(funcName : das_string; suffix : string) {
    return aotSuffixNameEx(string(funcName), suffix);
}

def aotSuffixNameEx(funcName : string; suffix : string) {
    var prefix = is_cpp_keyword(funcName);

    let name = build_string() $(var writer) {
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

def aotStructName(st : Structure?) {
    // Suffix "_S" is prepended only when `st.name` is a C++ keyword (or contains
    // a non-alnum char, which the parser disallows for struct names — so in
    // practice: keyword names only). Non-keyword struct names emit unchanged.
    return aotSuffixNameEx(st.name, "_S");
}

def aotEnumName(enu : Enumeration?) {
    // Same shape as aotStructName but for enums.
    return aotSuffixNameEx(enu.name, "_E");
}

def aotEnumValueName(name : das_string) {
    // Used for enum value identifiers inside `enum class { ... }` body and at access sites
    // (`EnumName::ValueName`). Prepended `_V` only for C++ keyword names.
    return aotSuffixNameEx(name, "_V");
}


def fa_element(t : TypeDeclPtr) {
    // element of a tFixedArray chain (a clone of t itself when not dim'd) - structural
    // reads (argTypes/annotation/variant offsets) live on the element, not the chain
    // head. Clone-walks so the result is non-const whatever the caller's view is.
    var e = clone_type(t)
    while (e.baseType == Type.tFixedArray && e.firstType != null) {
        e = clone_type(e.firstType)
    }
    return e
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

    return build_string() $(writer) {
        if (baseType == Type.tFixedArray && typeDecl.firstType != null) {
            // natural recursion; inner chain nodes are canonically bare (qualifiers live on the head)
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
            } else {
                write(writer, "TVariant<{typeDecl.variantSize},{typeDecl.variantAlign},{types}>")
            }
        } elif (baseType == Type.tStructure) {
            if (typeDecl.structType != null) {
                // Elaborated-type-specifier (`struct X`) keeps GCC's -Wchanges-meaning
                // happy when a field name shadows the struct name in the same scope —
                // e.g. `struct User { TTuple<...,struct Profile> Profile; }`. Valid in
                // every C++ context where a bare struct name is, including template args
                // and qualified names.
                if (typeDecl.structType._module.name.empty()) {
                    write(writer, "struct {aotStructName(typeDecl.structType)}")
                } else {
                    write(writer, "struct {aotModuleName(typeDecl.structType._module)}::{aotStructName(typeDecl.structType)}")
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
                    write(writer, "DAS_COMMENT(enum) {aotEnumName(typeDecl.enumType)}");
                } else {
                    write(writer, "DAS_COMMENT(enum) {aotModuleName(typeDecl.enumType._module)}::{aotEnumName(typeDecl.enumType)}")
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
        // Reference handling: emit `*` when substitute_ref is requested (iterator
        // and stack-temp variable emission paths use `T*` storage), otherwise emit
        // `&` so writable reference parameters (including value-annotation handles
        // like Handle<T>, EntityId, das::Time used as `var x : T&`) propagate
        // mutations back to the caller in AOT.
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

// escape analysis: a `new`/ascend whose pointee lives in the stack frame gets a block-scope
// storage declared up front; this names it uniquely per allocation site
def stackNewName(expr : ExpressionPtr) : string {
    return "_stack_new_{expr.at.line:d}_{expr.at.column:d}";
}

// the pointee (C++) type for a stack-allocated `new` (ExprNew) or ascend (ExprAscend)
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

class NoAotMarker : AstVisitor {
    //! AST visitor that marks functions containing non-AOT-compatible types as noAot.
    @do_not_delete func : Function? <- null;

    // type
    def override preVisitTypeDecl(typeDecl : TypeDeclPtr) {
        if (func != null && !typeDecl.canAot) {
            func.flags.noAot = true
        }
    }
    // function
    def override preVisitFunction(var f : FunctionPtr) {
        func = f;
    }
    def override visitFunction(var that : FunctionPtr) : FunctionPtr {
        var tmp := func
        func = null;
        return <- tmp;
    }
    // any expression
    def override preVisitExpression(expr : ExpressionPtr) {
        if (func != null && expr._type != null && !expr._type.canAot) {
            func.flags.noAot = true
        }
    }
    // looks like call
    def override preVisitExprLooksLikeCall(call : ExprLooksLikeCall?) {
        if (call.name == "invoke") {   // invoke of anonymous blk
            if (!empty(call.arguments) && call.arguments[0] is ExprMakeBlock) {
                let mkb = call.arguments[0] as ExprMakeBlock;
                var blk = mkb._block as ExprBlock;
                blk.blockFlags.aotSkipMakeBlock = true;
            }
        }
    }
};

class public PrologueMarker : AstVisitor {
    //! AST visitor that marks functions requiring a prologue for AOT compilation.
    @do_not_delete func : Function? <- null;

    // function
    def override preVisitFunction(var f : FunctionPtr) {
        func = f;
    }
    def override visitFunction(var that : FunctionPtr) {
        func = null;
        return <- that;
    }
    // ExprMakeBlock
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
    //! AST visitor that collects structs and enums used in AOT-eligible functions.
    useStructs : table<Structure?>;
    useEnums : table<Enumeration?>;

    def override preVisitTypeDecl(typeDecl : TypeDeclPtr) {
    }

    def override canVisitFunction(fn : Function?) {
        return !fn.flags.noAot;
    }

    def override preVisitExpression(expr : ExpressionPtr) {
        mark(expr._type);
    }
    def override preVisitFunctionArgument(fn : FunctionPtr; variable : VariablePtr; lastArg : bool) {
        mark(variable._type);
    }
    def override preVisitExprBlockArgument(expr_blk : ExprBlock?; variable : VariablePtr; lastArg : bool) {
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
def public funcInfoName(info : FuncInfo?) {
    //! Returns the C++ identifier name for a function's runtime info based on its hash.
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


class public AotDebugInfoHelper {
    //! Helper for generating debug type and variable information in AOT C++ output.
    private info2Name  = new DebugVarCache()
    private info2TypeName = new DebugVarCache()
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
            helper |> debug_helper_iter_structs($(_name, ti) {
                write(writer, "extern StructInfo {structInfoName(ti)};\n");
            });
            helper |> debug_helper_iter_types($(_name, ti){
                write(writer, "extern TypeInfo {typeInfoName(ti)};\n");
            })
            write(writer, "\n");
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
            helper |> debug_helper_iter_structs($(_name, si) {
                if (si.fields == null) return ;
                for (fi in range(si.count)) {
                    let fld & = unsafe(si.fields[fi])
                    if (fld.annotation_arguments == null || empty(*fld.annotation_arguments)) continue;
                    write(writer, "    {structInfoName(si)}_field_{fi}.annotation_arguments = &{structInfoName(si)}_field_{fi}_ann;\n")
                }
            })
            write(writer, "\}\n\n")
            info2Name.clear();
            info2TypeName.clear();
        }
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
            let fld & = unsafe(info.fields[fi])
            writeDim(writer, fld, suffix);
            writeArgTypes(writer, fld, suffix);
            writeArgNames(writer, fld, suffix);
            let prefix = (info.module_name |> !empty(info.module_name)) ? "{info.module_name}::" : "";
            // info.name is the struct's daslang name; mangle if C++ keyword so offsetof()
            // in describeCppVarInfo sees the same identifier the struct decl emitted.
            write(*writer, "VarInfo {structInfoName(info)}_field_{fi} =  \{ {describeCppVarInfo(prefix + aotSuffixNameEx(info.name, "_S"), fld,suffix)} \};\n");
            if (fld.annotation_arguments != null) {
                if (length(*fld.annotation_arguments) > 0) {
                    let annArgs = build_string() $(var sb) {
                        var first = true
                        for (arg in *fld.annotation_arguments) {
                            if (!first) { write(sb, ", "); }
                            first = false
                            if (arg.basicType == Type.tBool) {
                                write(sb, "AnnotationArgument(\"{arg.name}\", {arg.bValue})")
                            } elif (arg.basicType == Type.tString) {
                                write(sb, "AnnotationArgument(\"{arg.name}\", string(\"{arg.sValue}\"))")
                            } elif (arg.basicType == Type.tInt) {
                                write(sb, "AnnotationArgument(\"{arg.name}\", {arg.iValue})")
                            } elif (arg.basicType == Type.tFloat) {
                                write(sb, "AnnotationArgument(\"{arg.name}\", {arg.fValue}f)")
                            }
                        }
                    }
                    write(*writer, "static AnnotationArguments {structInfoName(info)}_field_{fi}_ann = \{ {annArgs} \};\n")
                }
            }
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
            write(writer, "{info.count:d}, {info_size}, UINT64_C(0x{info.init_mnh:x}), nullptr, UINT64_C(0x{info.hash:x}), {info.firstGcField:d}")
        }
    }
    def describeCppFuncInfoFields(var writer : StringBuilderWriter?; info : FuncInfo?) {
        if (info.fields == null) return ;
        for (fi in range(info.count)) {
            let suffix = "_var_{info.hash:d}";
            let fld & = unsafe(info.fields[fi])
            writeDim(writer, fld, suffix);
            writeArgTypes(writer, fld, suffix);
            writeArgNames(writer, fld, suffix);
            write(*writer, "VarInfo {funcInfoName(info)}_field_{fi} =  \{ {describeCppVarFuncInfo(info.name, fld,suffix)} \};\n");
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
        }
    }
    def describeCppEnumInfoValues(var writer : StringBuilderWriter?; einfo : EnumInfo?) {
        for (v in range(einfo.count)) {
            let val = unsafe(einfo.fields[v]);
            // Portable C++ int64 literal: the previous `int64_t(UINT64_C(0x...))` form relied on
            // the uint64→int64 narrowing being well-defined, which is only true post-C++20.
            // Negative values are emitted via unary minus on a positive INT64_C literal;
            // INT64_MIN is special-cased because |INT64_MIN| overflows int64 before the
            // unary minus binds, so we construct it from valid signed parts instead.
            let bits = uint64(val.value)
            var cppLit : string
            if (bits == 0x8000000000000000ul) {
                cppLit := "(-INT64_C(0x7fffffffffffffff) - INT64_C(1))"
            } elif (val.value < 0l) {
                let absBits = uint64(-val.value)
                cppLit := "-INT64_C(0x{absBits:x})"
            } else {
                cppLit := "INT64_C(0x{bits:x})"
            }
            write(*writer, "EnumValueInfo {enumInfoName(einfo)}_value_{v} = \{ \"{val.name}\", {cppLit} \};\n")
        }
        let enum_vals = (each(range(einfo.count))
            ._select("&{enumInfoName(einfo)}_value_{_}")
            .to_array()
            ._fold()) |> join(", ")
        write(*writer, "EnumValueInfo * {enumInfoName(einfo)}_values [] = \{ {enum_vals} \};\n");
    }
    def describeCppEnumInfo(info : EnumInfo?) {
        return "\"{info.name}\", \"{info.module_name}\", {enumInfoName(info)}_values, {info.count:d}, UINT64_C(0x{info.hash:x}), {info.flags:d}";
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
                let ann_ptr : void? = info.annotation_or_name
                let int_ptr = unsafe(reinterpret<uint64>(ann_ptr));
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
        // TODO: check if it indeed needs renaming
        return true;
    }
    def renameVariable(variable : VariablePtr) {
        if (needRenaming(variable)) {
            rename |> insert(variable, "__{aotSuffixNameEx(variable.name,"_Var")}_rename_at_{variable.at.line:d}_{(tempCounter++):d}");
        }
    }
// for loop
    def override preVisitExprForVariable(expr : ExprFor?; variable : VariablePtr; last : bool) {
        for (varr in expr.iteratorVariables) {
            renameVariable(varr);
        }
    }
// blk argument
    def override preVisitExprBlockArgument(blk : ExprBlock?; variable : VariablePtr; lastArg : bool) {
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
            if (!empty(blk.finalList)) return blk;
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
    def override preVisitExprLetVariable(let_var : ExprLet?; var variable : VariablePtr; last : bool) {
        var bfinal = getFinalBlock()
        if (bfinal != null) {
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

// make array
    def override preVisitExprMakeArray(var expr : ExprMakeArray?) {
        handleExpr(expr)
    }
// make tuple
    def override preVisitExprMakeTuple(var expr : ExprMakeTuple?) {
        handleExpr(expr)
    }
// make structure
    def override preVisitExprMakeStruct(var expr : ExprMakeStruct?) {
        handleExpr(expr)
    }
// make variant
    def override preVisitExprMakeVariant(var expr : ExprMakeVariant?) {
        handleExpr(expr)
    }
// new -> stack frame
    def override preVisitExprNew(var expr : ExprNew?) {
        if (expr.allocate_on_stack) {
            let blk = getCurrentBlock();
            stackNew[blk] |> push(expr);
        }
    }
// ascend -> stack frame
    def override preVisitExprAscend(var expr : ExprAscend?) {
        if (expr.ascendFlags.allocate_on_stack && !expr.ascendFlags.needTypeInfo) {
            let blk = getCurrentBlock();
            stackNew[blk] |> push(expr);
        }
    }
// call with CMRES
    def override preVisitExprCall(var expr : ExprCall?) {
        let need_insert = !expr.doesNotNeedSp && expr.stackTop != 0u;
        if (need_insert) {
            let blk = getCurrentBlock();
            localTemps[blk] |> push(expr);
        }
    }
// invoke with CMRES
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
    @do_not_delete localTemps : table<ExprBlock?, array<Expression?>>
    @do_not_delete stackNew :   table<ExprBlock?, array<Expression?>>

    rename : table<Variable?, string>
    moved : table<Variable?>
    tempCounter : uint64 = uint64(0)
};

def collect_block_variables() {
}

def public describeCppFunc(fn : FunctionPtr; var collector : BlockVariableCollector?; cross_platform : bool; needName : bool = true; needInline : bool = true) {
    //! Returns the C++ function signature string for an AOT-compiled function.
    return build_string() $(writer) {
        if (needInline) {
            write(writer, "inline ");
        }
        describeLocalCppType(unsafe(addr(writer)), fn.result,
            cross_platform, CpptSubstitureRef.no, CpptSkipConst.yes);
        write(writer, " {needName ? aotFuncName(fn) : "(*)"}");
            // let prefix = fn.arguments |> empty() ? "" : ", "
            /*
            let prefix = "";
            var args_str = "";
            for (arg1 in fn.arguments) {
                let arg : Variable? := arg1;
                args_str += ", ";
                args_str += build_string() $(writer2) {
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

class public CppAot : AstVisitor {
    //! Main AST visitor that generates C++ ahead-of-time compiled code from daslang AST.
/*
    CppAot ( const ProgramPtr & prog, BlockVariableCollector & cl ) : program(prog), collector(cl) {
        helper.rtti = program.options.getBoolOption("rtti",false);
        prologue = program.options.getBoolOption("aot_prologue",false) ||
            program.getDebugger();
        solidContext = program.policies.solid_context || program.options.getBoolOption("solid_context",false);
    }
*/

    adapter : VisitorAdapter ?

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
    aot_prefixes : array<string>;// looks like it always empty. Can we remove it?
    lastNewLine : uint64 = -1ul;
    tab : int = 0;
    debugInfoGlobal : int = 0;
    helper : AotDebugInfoHelper ? = new AotDebugInfoHelper();
    program : ProgramPtr;
    @do_not_delete collector : BlockVariableCollector ? = new BlockVariableCollector();
    aotPrefix : table<string>;
    @do_not_delete local_temp_names : table<Expression?; int>;
    @do_not_delete scopes : array<ExprBlock?>;
    prologue : bool = false;
    solidContext : bool = false;
    cross_platform : bool = false;
    aot_filter_function : string;  // when set, emit markers around matching function

    cachedTi = new DebugVarCache()

    def newLine() {
        // auto nlPos = ss.tellp();
        write(*ss, "\n");
        /*if ( nlPos != lastNewLine ) {
            lastNewLine = ss.tellp();
        }*/
    }
    def tabs() {
        return build_string() $(writer) { write_chars(writer, ' ', 4 * tab); }
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
        write(*ss, "};\n")   // enum
        write(*ss, "}\n")   // namespace
        if (enu.external) {
            write(*ss, "#endif // external enum\n");
        } else {
        }
        return enu;
    }
// structure
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
        write(*ss, "};\n");   // structure
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
            write(*ss, " {makeLocalTempName(tmp)};\n");
        }
    }
    def override visitGlobalLet(prog : ProgramPtr) {
        tab --;
        write(*ss, "}\n");
    }
// global
    def override preVisitGlobalLetVariable(variable : Variable?; lastArg : bool) {
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
        let hash_val = solidContext ? "{variable.stackTop:d}" : "0x{variable.getMangledNameHash:x}";
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
        return !fun.moreFlags.isTemplate;
    }
    def override preVisitFunction(fn : FunctionPtr) {
        if (!empty(aot_filter_function) && (fn.name == aot_filter_function || aotFuncName(fn) == aot_filter_function)) {
            write(*ss, "\n/*<<AOT_FUNC_BEGIN>>*/");
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
        } elif (fn.body != null && !(fn.body is ExprBlock)) {
            write(*ss, ";\n");
            tab --;
            write(*ss, "}\n");
        } else {
            write(*ss, "\n");
        }
        if (!empty(aot_filter_function) && (fn.name == aot_filter_function || aotFuncName(fn) == aot_filter_function)) {
            write(*ss, "/*<<AOT_FUNC_END>>*/\n");
        }
        return fn;
    }
// blk
    def makeLocalTempName(expr : ExpressionPtr) {
        // todo: fix casts to parents A -> B -> C (from A to B)
        if (!local_temp_names |> key_exists(expr)) {
            local_temp_names |> insert(expr, local_temp_names |> length());
        }
        /*if (isMakeLocal(expr)) {
            stackTop = (expr as ExprMakeLocal).stackTop;
        }*/
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
        // pre-declare variables
        assume vars = collector.variables[blk];
        for (variable in vars) {
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
        // pre-declare locals
        collector.localTemps.get(blk) $(temps) {
            for (tmp in temps) {
                write(*ss, tabs());
                describeVarLocalCppType(ss, tmp._type, cross_platform);
                let tempName = makeLocalTempName(tmp);
                write(*ss, " {tempName}; {tempName};\n");
            }
        }
        // pre-declare stack-frame storage for non-escaping `new` / ascend. The per-evaluation
        // (re)initialization happens at the use site - plain `new` memsets each time (matching the
        // interpreter), ascend overwrites the whole value via das_ascend_stack
        collector.stackNew.get(blk) $(news) {
            for (nw in news) {
                let nm = stackNewName(nw);
                write(*ss, "{tabs()}{stackNewPointeeType(nw, cross_platform)} {nm}; {nm};\n");
            }
        }
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
// let
    def override preVisitExprLetVariable(let_ : ExprLet?; var variable : VariablePtr; last : bool) {
        let vname = collector.getVarName(variable);
        if (variable.init != null && variable.init is ExprMakeBlock) {
            var mkb = variable.init as ExprMakeBlock;
            var blk = mkb._block as ExprBlock;
            blk.blockFlags.aotSkipMakeBlock = true;
            write(*ss, "auto {vname}_TempFunctor = ");
            visit_expression(variable.init, adapter);
            write(*ss, ";\n{tabs()}");
            blk.blockFlags.aotSkipMakeBlock = false;
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
    def override visitExprLetVariable(let_ : ExprLet?; variable : VariablePtr; last : bool) {
        if (!last) write(*ss, "; ");
        if (!collector.isMoved(variable)) {
            if (variable._type.flags.constant && variable._type.isRefType && !variable._type.flags.ref) {
                let vname = collector.getVarName(variable);
                write(*ss, ";\n{tabs()}");
                describeLocalCppType(ss, variable._type, cross_platform);
                write(*ss, " & {vname} = {vname}_ConstRef;\n");
            }
        }
        if (variable.init != null && variable.init is ExprMakeBlock) {
            var mkb = variable.init as ExprMakeBlock;
            // Restore ast as it was before, just for consistency.
            mkb.aotFunctorName := ""
        }
        return variable;
    }
    def override preVisitExprLetVariableInit(let_ : ExprLet?; variable : VariablePtr; expr : ExpressionPtr) {
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
    def override preVisitExprLabel(that : ExprLabel?) {
        write(*ss, "label_{that.labelName}:;");
    }
// goto
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
// clone
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
// move
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
// op1
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
    def override preVisitExprOp1(var that : ExprOp1?) {
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
    def override visitExprOp1(var that : ExprOp1?) : ExpressionPtr {
        if (!that.func.flags.builtIn || that.func.flags.callBased) {
            CallFunc_visitCallArg(that, that.subexpr, true);
            CallFunc_visit(that);
            that.arguments.clear();
        } elif (isOpPolicy1(that)){
            write(*ss, ",*__context__,nullptr)");
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
// op2
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
        return ["+=", "-=", "*=", "/=", "%=",// math
                "&=", "|=", "^=",         // bin
                "&&=", "||=", "^^=",      // bool
                "<<=", ">>=", "<<<=", ">>>="// rotational
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
    def override preVisitExprOp2Right(that : ExprOp2?; right : ExpressionPtr) {
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
// return
    def override preVisitExprReturn(expr : ExprReturn?) {
        write(*ss, "return ");
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
        assume retT = expr.returnFunc != null ? expr.returnFunc.result : expr._block.returnType;
        if (!retT.isVoid) {
            write(*ss, ")");
        }
        return expr;
    }
// break
    def override preVisitExprBreak(that : ExprBreak?) {
        write(*ss, "break");
    }
// continue
    def override preVisitExprContinue(that : ExprContinue?) {
        write(*ss, "continue");
    }
// var
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
// null coaelescing
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
// is variant
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
// as variant
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
// safe as variant
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
// safe field
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
            let mod_name = (vtype.structType._module.name.empty() ? "" : string(vtype.structType._module.name) + "::");
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
// field
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
                // c-cast const char * etc string casts to char * or char * const
                if (field.value._type.flags.constant) {
                    write(*ss, "((const char * const)(");
                } else {
                    write(*ss, "(({describeCppType(field._type, DescribeConfig(cross_platform=cross_platform))})(");
                }
            }
            if (field._type.flags.isNativeDim) {
                // native C-array member: pun to the TDim view so iterate/pass/copy/safe-at consumers compile (issue #3077)
                write(*ss, "das_reinterpret<{describeCppType(field._type, DescribeConfig(skip_ref=true,cross_platform=cross_platform))}>::pass(");
            }
            aot_previsit_get_field(field_type.annotation, ss, string(field.name));
        } elif (field_type.baseType == Type.tPointer) {
            if (field_type.firstType.isHandle) {
                if (field._type.isString) {
                    // c-cast const char * etc string casts to char * or char * const
                    if (field.value._type.flags.constant) {
                        write(*ss, "((const char * const)(");
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
    def override visitExprField(var field : ExprField?) : ExpressionPtr {
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
            if (field._type.flags.isNativeDim) {
                write(*ss, ")");
            }
        } elif (field.value._type.baseType == Type.tPointer) {
            if (field.value._type.firstType.isHandle) {
                aot_type_ann_get_field_ptr(field.value._type.firstType.annotation, ss, string(field.name));
                write(*ss, " /*{field.name}*/");
                if (field._type.isString) {
                    write(*ss, "))")
                }
                if (field._type.flags.isNativeDim) {
                    write(*ss, ")");
                }
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
// at
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
// safe at
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
// const
    def override visitExprFakeContext(var c : ExprFakeContext?) : ExpressionPtr {
        write(*ss, "__context__");
        return c;
    }
    def override visitExprFakeLineInfo(var c : ExprFakeLineInfo?) : ExpressionPtr {
        write(*ss, "((LineInfoArg *)(&LineInfo::g_LineInfoNULL))");
        return c;
    }
    def override visitExprConstPtr(var c : ExprConstPtr?) : ExpressionPtr {
        if (c.getValue != null) {
            write(*ss, "((void *) 0x{unsafe(reinterpret<int64>(c.getValue)):x}))");
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
            write(*ss, "INT64_MIN"); // silence overflow warning
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
            write(*ss, "UINT64_C(0x{c.getValue:x})");
        } elif (c._type.baseType == Type.tBitfield16) {
            write(*ss, "uint16_t(0x{uint(c.getValue):x})");
        } elif (c._type.baseType == Type.tBitfield8) {
            write(*ss, "uint8_t(0x{uint(c.getValue):x})");
        } else {
            write(*ss, "0x{c.getValue:x}u");
        }
        return c;
    }
    def override visitExprConstBool(var c : ExprConstBool?) : ExpressionPtr {
        write(*ss, (c.getValue ? "true" : "false"));
        return c;
    }

    def to_cpp_double(val : double) : string {
        // Mirror the C++-side to_cpp_double (runtime_string.cpp:526): emit
        // portable C++ tokens for non-finite and boundary doubles. Without
        // this, daslang's '{val:.17e}' formatter prints bare 'inf' / 'nan',
        // which either don't exist in C++ (inf) or resolve to '<cmath>'s
        // 'double nan(const char*)' overload (nan) — both bake into the
        // AOT-emitted code and break the C++ build. Bit-pattern tests
        // (val != val for NaN, magnitude vs DBL_MAX for ±inf) avoid pulling
        // in `require math`, which interferes with this file's `match`-based
        // type-table helpers (control-flow inference cascade).
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
    def override visitExprConstString(var c : ExprConstString?) : ExpressionPtr {
        if (c.value.empty()) {
            write(*ss, "nullptr");
        } else {
            peek(c.value) $(sv) {
                write(*ss, "((char *) \"{escape(sv)}\")");
            }
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
// ExprAssume
    def override preVisitExprAssume(expr : ExprAssume?) {
        write(*ss, "\n#if 0 // with, note optimizations are off\n");
    }
    def override visitExprAssume(var expr : ExprAssume?) : ExpressionPtr {
        write(*ss, "\n#endif\n");
        return expr;
    }
// ExprWith
    def override preVisitExprWith(expr : ExprWith?) {
        write(*ss, "\n#if 0 // with, note optimizations are off\n");
    }
    def override preVisitExprWithBody(expr : ExprWith?; body : ExpressionPtr) {
        write(*ss, "\n#endif\n");
    }
// ExprWhile
    def override preVisitExprWhile(wh : ExprWhile?) {
        write(*ss, "while ( ");
    }
    def override preVisitExprWhileBody(wh : ExprWhile?; body : ExpressionPtr) {
        write(*ss, " )\n{tabs()}");
    }
    def override visitExprWhile(var wh : ExprWhile?) : ExpressionPtr {
        return wh;
    }
// if then else
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
// swizzle
    def override preVisitExprSwizzle(expr : ExprSwizzle?) {
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
// string builder
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
// typedecl
    def override preVisitExprTypeDecl(expr : ExprTypeDecl?) {
        write(*ss, "das_typedecl_value<{describeCppType(expr.typeexpr,DescribeConfig(cross_platform=cross_platform))}>()()");
    }
// type-info
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
// try-catch
    def override preVisitExprTryCatch(tc : ExprTryCatch?) {
        write(*ss, "das_try_recover(__context__, [&]()\n");
        write(*ss, "{tabs()}");
    }
    def override preVisitExprTryCatchCatch(tc : ExprTryCatch?; blk : ExpressionPtr) {
        write(*ss, ", [&]()\n");
        write(*ss, "{tabs()}");
    }
    def override visitExprTryCatch(var tc : ExprTryCatch?) : ExpressionPtr {
        write(*ss, ")");
        return tc;
    }
// ptr2ref
    def override preVisitExprPtr2Ref(ptr2ref : ExprPtr2Ref?) {
        if (ptr2ref.unsafeDeref) {
            write(*ss, "(*(");
        } else {
            write(*ss, "das_deref(__context__,");
        }
    }
    def override visitExprPtr2Ref(var ptr2ref : ExprPtr2Ref?) : ExpressionPtr {
        if (ptr2ref.unsafeDeref) {
            write(*ss, "))");
        } else {
            write(*ss, ",__FILE__,__LINE__)");
        }
        return ptr2ref;
    }
 // ref2ptr
    def override preVisitExprRef2Ptr(ref2ptr : ExprRef2Ptr?) {
        write(*ss, "das_ref(__context__,");
    }
    def override visitExprRef2Ptr(var ref2ptr : ExprRef2Ptr?) : ExpressionPtr {
        write(*ss, ")");
        return ref2ptr;
    }
// addr
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
// cast
    def override preVisitExprCast(expr : ExprCast?) {
        write(*ss, "{(expr.castFlags.upcastCast ? "das_upcast" : "das_cast")}<{describeCppType(expr.castType,DescribeConfig(cross_platform=cross_platform))}>::cast(");
    }
    def override visitExprCast(var expr : ExprCast?) : ExpressionPtr {
        write(*ss, ")");
        return expr;
    }
// delete
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
    // DELETE
    def override preVisitExprDeleteSizeExpression(del : ExprDelete?; expr : ExpressionPtr) {
        write(*ss, ",");
    }

    def override visitExprDelete(var edel : ExprDelete?) : ExpressionPtr {
        write(*ss, ")");
        return edel;
    }
// ascend
    def override preVisitExprAscend(expr : ExprAscend?) {
        if (expr.ascendFlags.allocate_on_stack && !expr.ascendFlags.needTypeInfo
                && expr._type.firstType.baseType != Type.tHandle) {
            // build the value, then copy it into the pre-declared stack-frame storage
            let type_str = describeCppType(expr._type.firstType, DescribeConfig(skip_ref = true, skip_const = true, cross_platform = cross_platform));
            let subexpr_str = describeCppType(expr.subexpr._type, DescribeConfig(skip_ref = true, skip_const = true, cross_platform = cross_platform));
            write(*ss, "das_ascend_stack<{type_str},{subexpr_str}>::make({stackNewName(expr)},");
            return
        }
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
    def override visitExprAscend(var expr : ExprAscend?) : ExpressionPtr {
        write(*ss, ")");
        return expr;
    }
// new
    def override preVisitExprNew(enew : ExprNew?) {
        if (enew.allocate_on_stack) {
            // pointee lives in the pre-declared stack-frame storage; zero it per evaluation
            // (matching the interpreter's SimNode_NewStack) and take its address
            let nm = stackNewName(enew);
            write(*ss, "(memset((void*)&{nm},0,sizeof({nm})), &{nm}");
            return
        }
        if (enew._type.baseType == Type.tFixedArray) {
            let ptrType = fa_element(enew._type)   // the fixed-array chain wraps the pointer node; pointee is its firstType
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
            // write(*ss, ",");
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
// make variant
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
        let variantType = fa_element(expr._type)   // dim'd make-variant carries the variant on the chain element
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
// make structure
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
        let mkType = fa_element(expr.makeType)   // dim'd make-struct carries the record on the chain element
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
// make array
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
        if (!expr.makeFlags.initAllFields) {
            write(*ss, "{tabs()}das_zero({mkaName(expr)});\n");
        }
    }
    def override preVisitExprMakeArrayIndex(expr : ExprMakeArray?; index : int; init : ExpressionPtr; last : bool) {
        if (init._type.canCopy) {
            write(*ss, "{tabs()}{mkaName(expr)}({index},__context__) = ");
        } else {
            write(*ss, "{tabs()}das_move({mkaName(expr)}({index},__context__),");
        }
    }
    def override visitExprMakeArrayIndex(expr : ExprMakeArray?; index : int; init : ExpressionPtr; last : bool) {
        if (init._type.canCopy) {
            write(*ss, ";\n");
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
// make tuple
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

// looks like call
    def override preVisitExprLooksLikeCall(var call : ExprLooksLikeCall?) {
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
                unsafe {
                let args = (each(range(1, length(call.arguments)))
                    ._select("{describeCppType(call.arguments[_]._type, DescribeConfig(cross_platform=cross_platform))}{(call.arguments[_]._type.isRefType && !call.arguments[_]._type.flags.ref) ? " &" : ""}")
                    .to_array()
                    ._fold()) |> join(",")
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
// call
    def policyArgNeedCast(polType : TypeDeclPtr; argType : TypeDeclPtr) {
        if (argType.isVectorType) return false;
        if (!polType.isHandle) {
            if (polType.isVecPolicyType && argType.isVecPolicyType) return false;
        }
        return polType.isPolicyType;
    }
    def policyResultNeedCast(polType : TypeDeclPtr; resType : TypeDeclPtr) {
        if (resType.isVoid || !resType.isPolicyType) return false;
        return policyArgNeedCast(polType, resType);
    }
    def isPolicyBasedCall(call : ExprCall?) {
        let bif = call.func as BuiltInFunction;
        // empty arg ctor returning handle is NOT policy-based
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
        if (func.flags.noAot || func.flags.aotHybrid) return true;
        return func._module != program.getThisModule;
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
    def CallFunc_preVisit(call : ExprCallFunc?) {
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
        return func.moreFlags.needStringCast && arg.isString && !arg.flags.ref;
    }
    def CallFunc_preVisitCallArg(call : ExprCallFunc?; arg : ExpressionPtr; is_last : bool) {
        if (call.func.moreFlags.propertyFunction) return ;  // property function goes ((arg0).name()). we do nothing here
        var argIndex = 0;
        for (it in range(length(call.arguments))) {
            if (call.arguments[it] == arg) break;
            argIndex++;
        }
        assert(argIndex != length(call.arguments));
        write(*ss, "{get_aot_arg_prefix(call.func, call, argIndex)}");
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
    def CallFunc_visitCallArg(call : ExprCallFunc?; arg : ExpressionPtr; last : bool) {
        if (call.func.moreFlags.propertyFunction) return ;  // property function goes ((arg0).name()). we do nothing here
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
        if (funArgType.isAotAlias) write(*ss, ")");
        write(*ss, "{get_aot_arg_suffix(call.func, call, argIndex)}");
        if (!last) {
            write(*ss, ",");
        }
    }
    def CallFunc_visit(call : ExprCallFunc?) {
        if (call.func.moreFlags.propertyFunction) {     // property function goes ((arg0).name()). we do `).name())` here
            if (call.func.flags.builtIn) {
                let efn = call.func as ExternalFnBase;
                write(*ss, ").{efn.cppName}())");
            } else {
                peek(call.func.name) $(fn) {
                    assert(starts_with(fn, ".`"));
                    write(*ss, ").{aotFieldName(fn.chop(2, length(fn)))}())");    // we skip .` part of the deal
                }
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
// for
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
            write(*ss, "{tabs()}das_iterator<{type_str}> {forSrcName(variable.name)}(");
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
        }
        write(*ss, ");\n");

        // source
        // A value (non-ref) loop var is a local copy that first()/next() write into, so it must not be
        // C++-const — a const POD value local is ill-formed without a user default ctor (e.g. struct rows
        // from a const-element iterator such as from_xml_node). Ref vars are pointers (const on pointee is ok).
        let skipTC : bool = !variable._type.flags.ref
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
    def override visitExprFor(var ffor : ExprFor?) : ExpressionPtr {
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


def public dumpDependencies(program : ProgramPtr; var aotVisitor : CppAot?) {
    //! Writes forward declarations and dependent type/function definitions for AOT output.
    let utm = new UseTypeMarker();
    make_visitor(*utm) $(adapter) {
        visit(program, adapter);
    }
    let remUS = program._options  |> find_arg("remove_unused_symbols") ?as tBool ?? true;
    program.get_ptr() |> for_each_module_no_order($(pm) {
        if (pm.moduleFlags.fromExtraDependency) return ;
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
        pm |> for_each_structure($(ps) {
            if (!remUS || utm.useStructs |> key_exists(ps)) {
                program |> visit_structure(ps, aotVisitor.adapter);
            } else {
                write(*aotVisitor.ss, "// unused structure {ps.name}\n");
            }
        });
    })

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
        // arg
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



def collectUsedFunctions(modules : array<Module?>; totalFunctions : int; this_module : Module?; all_modules : bool; is_all : bool = false) : array<Function?> {
    var fnn : array<Function?>;
    fnn.reserve(totalFunctions);
    for (pm in modules) {
        pm |> for_each_module_function($(pfun) {
            if ((!all_modules && pfun._module != this_module)
                    || pfun.index < 0 || !pfun.flags.used) return ;
            if (!is_all) {
                if (pfun.flags.builtIn || pfun.flags.noAot) return ;
            }
            fnn |> push(pfun);
        });
    }
    return <- fnn;
}

def public collectProgramUsedFunctions(program : ProgramPtr; all_modules : bool; is_all : bool) : array<Function?> {
    //! Collects all used functions from a program's modules for AOT code generation.
    var modules : array<Module?>;
    program.get_ptr() |> for_each_module_no_order($(mod) {
        modules.push(mod);
    })
    return collectUsedFunctions(modules, program.totalFunctions, program.getThisModule, all_modules, is_all)
}

def registerAotCpp(var logs : StringBuilderWriter?; program : ProgramPtr; var context : Context; cross_platform; headers, all_modules : bool = false) {
    let fnn = collectProgramUsedFunctions(program, all_modules, false);
    let visitor = new ArgsConverter(logs, cross_platform);
    make_visitor(*visitor) $(adapter_marker) {
        visit(program, adapter_marker);
    }

    var funInit = false;
    for (fn in fnn) {
        if (fn.flags.init) {
            funInit = true;
        }
    }

    if (!fnn.empty()) {
        write(*logs, "\n#ifdef _MSC_VER\n#pragma optimize(\"\", off)\n#endif\n"); // Let's disable any optimizations. It helps on MSVC to compile faster
        // We should duplicate fields of AotFactory to reduce comptime.
        write(*logs, "struct AotFunction \{ uint64_t hash; bool is_cmres; void * fn; vec4f (*wrappedFn)(Context*); \};\n");
        write(*logs, "static AotFunction functions[] = \{\n");
        for (fn in fnn) {
            let is_cmres = fn.flags.copyOnReturn || fn.flags.moveOnReturn ? "true" : "false";
            let fn_name = aotFuncName(fn);
            write(*logs, "    // {get_aot_hash_comment(fn)}\n");
            write(*logs, "    \{ {fn.aotHash}, {is_cmres}, (void*)&{fn_name}, &__wrap_{fn_name} \},\n");
        }
        write(*logs, "};\n");
        write(*logs, "#ifdef _MSC_VER\n#pragma optimize(\"\", on)\n#endif\n"); // Enable optimizations back
    }
    if (headers) {
        write(*logs, "\nvoid registerAot ( AotLibrary & aotLib )\n\{\n")
    } else {
        write(*logs, "\nstatic void registerAotFunctions ( AotLibrary & aotLib ) \{\n")
    }
    if (!fnn.empty()) {
        write(*logs, "    for (const auto &[hash, cmres, fn1, fn2] : functions) \{\n")
        write(*logs, "        aotLib.emplace(hash, AotFactory(cmres, fn1, fn2));\n")
        write(*logs, "    \}\n")
    }

    if (headers) {
        write(*logs, "}\n");
    } else {
        write(*logs, "    resolveTypeInfoAnnotations();\n");
        write(*logs, "}\n");
    }
}

def public setAotHashes(program : ProgramPtr; var ctx : Context) {
    //! Computes semantic and AOT hashes for all used functions in the program.
    // compute semantic hash for each used function
    var fni = 0;
    program.get_ptr() |> for_each_module_no_order($(mod) {
        mod |> for_each_module_function($(pfun) {
            if (pfun.index < 0 || !pfun.flags.used) return ;
            pfun.hash = get_function_hash_by_id(pfun, pfun.index, unsafe(reinterpret<void?>(ctx)));
            fni++;
        })
    });
    // compute AOT hash for each used function
    // its the same as semantic hash, only takes dependencies into account
    program.get_ptr() |> for_each_module_no_order($(mod) {
        mod |> for_each_module_function($(pfun) {
            if (pfun.index < 0 || !pfun.flags.used) return ;
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

def public getRequiredModulesFor(program : ProgramPtr) {
    //! Collects required module declarations and checks for no-AOT modules in the program.
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
        } elif (mod.moduleFlags.fromExtraDependency && mod.name != "builtin") {
            // nothing, it was added by aot-module.
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
}


[export]
def public run_aot(prog : Program?; var pctx : Context?; cop : CodeOfPolicies) : string {
    //! Generates the complete AOT C++ source code string for a compiled and simulated program.
    let program <- unsafe(reinterpret<ProgramPtr>(prog));
    return build_string() $(writer) {
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
            write(writer, AOT_INCLUDES);
            write(writer, join(modules_str, ""));
            write(writer, "\n#endif\n");
        } else {
            // header
            write(writer, AOT_INCLUDES);
            write(writer, "{join(modules_str, "")}\n");
            write(writer, AOT_HEADERS);
            write(writer, "namespace das \{\n");
            write(writer, "namespace {program.thisNamespace} \{\n");
            build_string() $(tmp_writer) {
                let marker_vis = new NoAotMarker();
                make_visitor(*marker_vis) $(adapter_marker) {
                    visit(program, adapter_marker);
                }

                var coll = new BlockVariableCollector();
                make_visitor(*coll) $(adapter_coll) {
                    visit(program, adapter_coll)
                }

                // mark prologue
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
                                            cross_platform = cop.cross_platform)
                cpp_aot.helper.helper.rtti = program._options  |> find_arg("rtti") ?as tBool ?? false;
                cpp_aot.helper.cross_platform = cop.cross_platform;
                make_visitor(*cpp_aot) $(adapter) {
                    cpp_aot.adapter := adapter
                    dumpDependencies(program, cpp_aot);
                    visit(program, cpp_aot.adapter, true)
                }
                write(writer, "{cpp_aot.str()}");
                unsafe {
                    delete cpp_aot.helper.helper
                    delete cpp_aot.collector
                    delete cpp_aot
                }
            }
            registerAotCpp(unsafe(addr(writer)), program, *pctx, cop.cross_platform, false);
            write(writer, "\n");
            if (!cop.aot_lib) write(writer, "static AotListBase impl(registerAotFunctions);\n");
            // validation stuff
            if (cop.paranoid_validation) {
                /* todo: program->validateAotCpp(tw,*pctx);
                tw << "\n";*/
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
            let marker_vis = new NoAotMarker();
            make_visitor(*marker_vis) $(adapter_marker) {
                visit(program, adapter_marker);
            }

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
                                        aot_filter_function = func_name)
            cpp_aot.helper.helper.rtti = program._options  |> find_arg("rtti") ?as tBool ?? false;
            cpp_aot.helper.cross_platform = cop.cross_platform;
            make_visitor(*cpp_aot) $(adapter) {
                cpp_aot.adapter := adapter
                dumpDependencies(program, cpp_aot);
                visit(program, cpp_aot.adapter, true)
            }
            let full_output = cpp_aot.str()
            unsafe {
                delete cpp_aot.helper.helper
                delete cpp_aot.collector
                delete cpp_aot
            }
            // extract between markers
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
