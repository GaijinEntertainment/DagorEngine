options gen2
options indenting = 4
options no_unused_block_arguments = false
options no_unused_function_arguments = false
options strict_smart_pointers = true
options unsafe_table_lookup = false

module cpp_gen public

require daslib/fio
require daslib/defer
require daslib/json
require daslib/json_boost
require daslib/strings_boost
require daslib/regex
require daslib/regex_boost

let verbose = false //! Enables verbose output during C++ binding generation.

/*
    config
*/

struct ExEnum {
    //! Configuration for an external enum to include in binding generation.
    name : string
    removePrefix : bool = true
    isFlags : bool = false
}

var extra_args = "" //! Extra command-line arguments passed to the Clang parser.

var local_type_names : array<string> //! List of type names treated as local (non-external) types.

var blocked_type_names : array<string> //! List of type names excluded from binding generation.

var blocked_functions : array<string> <-  [ //! List of function names excluded from binding generation.
    "operator delete",
    "operator new",
    "__security_init_cookie",
    "__security_check_cookie"
];

var blocked_functions_table : table<string; bool> //! Lookup table built from blocked_functions for efficient exclusion checks.

var blocked_defines : array<string> <- [ //! List of preprocessor define names excluded from binding generation.
    "true",
    "false"
];

var blocked_defines_table : table<string; bool> //! Lookup table built from blocked_defines for efficient exclusion checks.

var blocked_enumerations : array<string> //! List of enumeration names excluded from binding generation.

var blocked_enumerations_table : table<string; bool> //! Lookup table built from blocked_enumerations for efficient exclusion checks.

var is_function_blocked : lambda<(name : string) : bool> //! Optional callback to determine if a function should be excluded from bindings.

var is_const_blocked : lambda<(name : string) : bool> //! Optional callback to determine if a constant should be excluded from bindings.

var is_struct_blocked : lambda<(name : string) : bool> //! Optional callback to determine if a struct should be excluded from bindings.

var alt_struct_name : lambda<(name : string) : string> //! Optional callback to provide an alternative name for a struct binding.

var cmres_functions : array<string> //! List of function names that return by value using copy-or-move semantics.

var allow_extern_c = false //! When true, includes extern "C" functions in binding generation.

var ex_enums : table<string; ExEnum> //! Table of external enums to generate bindings for, keyed by name.

var substitute_field_types : table<string; string> //! Table mapping field types to substitute types in generated bindings.

var alias_types : table<string; string> //! Table mapping C++ type names to their daslang alias types.

var namespace_to_prefix : table<string; string> //! Table mapping C++ namespace names to binding name prefixes.

var generate_split_functions = false //! When true, splits generated function bindings across multiple files.
var split_prefix = "" //! Prefix for split output file names.
var split_cpp_prefix = "" //! C++ prefix inserted at the top of each split file.
var split_cpp_suffix = "" //! C++ suffix appended at the end of each split file.

var enum_suffix = "" //! Suffix appended to generated enum type names.

var const_type_table : table<string; string> //! Table mapping constant macro names to their C++ types.

var const_uint8_type = "uint8_t" //! C++ type name used for uint8 constants.
var const_uint16_type = "uint16_t" //! C++ type name used for uint16 constants.

var ex_keywords : table<string; bool> //! Table of extra keywords that require name escaping in generated bindings.

var all_keywords = fixed_array<string>( //! List of all daslang keywords that conflict with C++ names and need escaping.
    "float",    "double",
    "uint",     "uint8",    "uint16",   "uint64",   "uint2",    "uint3",    "uint4",
    "int",      "int8",     "int16",    "int64",    "int2",     "int3",     "int4",
    "bool"
);

/*
    end of configuration
*/

enum ScopeKind {
    //! Kind of scope encountered during Clang AST traversal.
    Namespace
    Struct
}

struct Scope {
    //! Represents a named scope (namespace or struct) during AST traversal.
    name : string
    kind : ScopeKind
}

enum AccessKind {
    //! C++ access specifier kind for struct/class members.
    Private
    Public
    Protected
}

struct ClangAstDecl {
    //! Base struct for all Clang AST declarations with name and C++ name.
    name    : string
    cppName : string
}

struct ClangTypedef : ClangAstDecl {
    //! Represents a C++ typedef parsed from the Clang AST.
    qtype   : string
}

struct ClangEnum : ClangAstDecl {
    //! Represents a C++ enum parsed from the Clang AST.
    eclass  : bool
    etype   : string
    edecl   : array<string>
}

struct ClangGlobalVar : ClangAstDecl {
    //! Represents a C++ global variable parsed from the Clang AST.
    mangledName : string
    gtype       : string
}

struct ClangStructField : ClangAstDecl {
    //! Represents a field within a C++ struct parsed from the Clang AST.
    qtype       : string
    access      : AccessKind
    isBitfield  : bool
}

struct ClangFuncArg : ClangAstDecl {
    //! Represents a C++ function argument parsed from the Clang AST.
    atype   : string
    value   : string
    vtype   : string
}

struct ClangFunc : ClangAstDecl {
    //! Represents a C++ function or method parsed from the Clang AST.
    mangledName : string
    ftype       : string
    isMethod    : bool
    isStatic    : bool
    ofClass     : string
    args        : array<ClangFuncArg>
}

struct ClangStruct : ClangAstDecl {
    //! Represents a C++ struct or class parsed from the Clang AST.
    tag     : string
    fields  : array<ClangStructField>
    methods : array<ClangFunc>
}

struct ClangAst {
    //! Top-level container for all declarations parsed from the Clang AST.
    typedefs : table<string; ClangTypedef>
    enums : table<string; ClangEnum>
    structs : table<string; ClangStruct>
    funcs : table<string; ClangFunc>
    globals : table<string; ClangGlobalVar>
}

var name_stack : array<Scope> //! Stack of nested namespace/struct scopes during AST traversal.
var access_stack : array<AccessKind> //! Stack of access specifiers (public/private/protected) during traversal.
var struct_stack : array<string> //! Stack of struct names during nested struct traversal.
var ast : ClangAst //! Parsed Clang AST containing all extracted declarations.
var rev_enums : table<string; string> //! Reverse mapping from enum value to enum type name.

def is_str(jso, str) {
    //! Checks if a JSON value is a string equal to the given value.
    return (jso is _string) && ((jso as _string) == str)
}

def with_object(var jso; field; blk : block<(var val : table<string; JsonValue?>) : void>) {
    //! Invokes a block with the specified JSON object field.
    assert(jso[field] is _object)
    invoke(blk, jso[field] as _object)
}

def with_array(var jso; field; blk : block<(var val : array<JsonValue?>) : void>) {
    //! Invokes a block with the specified JSON array field.
    assert(jso[field] is _array)
    invoke(blk, jso[field] as _array)
}

def is_valid_loc(var loc) {
    //! Checks if a source location is valid (non-empty).
    if (loc |> length == 0) {
        return false
    }
    return true
}

def onInner(var root) {
    //! Recursively processes all inner declarations of a Clang AST node.
    root |> with_array("inner") $(inner) {
        for (idecl in inner) {
            assert(idecl is _object)
            var decl & = unsafe(idecl as _object)
            var skip = false
            if (decl |> key_exists("loc")) {
                decl |> with_object("loc") $(loc) {
                    skip = !loc |> is_valid_loc
                }
            }
            if (skip) {
                if (verbose) {
                    print("// skipping {decl["name"] as _string}\n")
                }
                continue
            }
            let kind = decl["kind"] as _string
            if (kind == "CXXRecordDecl") {
                onStruct(decl)
            } elif (kind == "NamespaceDecl") {
                onNamespace(decl)
            } elif (kind == "FieldDecl") {
                onField(decl)
            } elif (kind == "FunctionDecl") {
                onFunction(decl)
            } elif (kind == "CXXMethodDecl") {
                onMethod(decl)
            } elif (kind == "EnumDecl") {
                onEnum(decl)
            } elif (kind == "VarDecl") {
                onVar(decl)
            } elif (kind == "TypedefDecl") {
                onTypeDecl(decl)
            } elif (kind == "AccessSpecDecl") {
                onAccess(decl)
            } elif (kind == "LinkageSpecDecl") {
                onLinkageSpec(decl)
            } else {
                print("warning: unsupported {kind}\n")
            }
        }
    }
}


[sideeffects] // TODO: fixme. find why we need to specify [sideeffects] here
def onLinkageSpec(var root) {
    //! Processes an extern linkage specification node.
    if (!allow_extern_c) {
        return
    }
    if (!root |> key_exists("inner")) {
        return
    }
    if (verbose) {
        if (root |> key_exists("language")) {
            print("extern \"{root["language"] as _string}\"\n")
        }
    }
    onInner(root)
}

def onNamespace(var root) {
    //! Processes a C++ namespace declaration and its inner declarations.
    if (!root |> key_exists("inner")) {
        return
    }
    let name = root["name"] as _string
    let fullName = join([iterator for(n in name_stack); n.name], "::") + "::" + name
    if (verbose) {
        print("namespace {name} //{fullName}\n")
    }
    name_stack |> push(Scope(name = name, kind = ScopeKind.Namespace))
    onInner(root)
    name_stack |> pop()
}

def onEnum(var root) {
    //! Processes a C++ enum declaration and its constants.
    if (!root |> key_exists("name")) {
        print("enum 'name' missing in {root}\n")
        return
    }
    let name = root["name"] as _string
    let fullName = getCppName(name)
    if (root |> key_exists("scopedEnumTag")) {
        let tag = root["scopedEnumTag"] as _string
        let qtype = getQualType(root, "fixedUnderlyingType")
        if (verbose) {
            print("enum {tag} {name} : {qtype} // {fullName}\n")
        }
        ast.enums[fullName] <- ClangEnum(
            name = name,
            cppName = "{fullName}{enum_suffix}",
            eclass = true,
            etype = qtype
        )
    } else {
        if (verbose) {
            print("enum {name} // {fullName}\n")
        }
        ast.enums[fullName] <- ClangEnum(
            name = name,
            cppName = "{fullName}{enum_suffix}",
            eclass = false
        )
    }
    root |> with_array("inner") $(inner) {
        for (edo in inner) {
            assert(edo is _object)
            var ed & = unsafe(edo as _object)
            if (ed["kind"] as _string == "EnumConstantDecl") {
                var cname = ed["name"] as _string
                ast.enums[fullName].edecl |> push(cname)
            }
        }
    }
}

def onVar(var root) {
    //! Processes a global variable declaration.
    let name = root["name"] as _string
    let fullName = getCppName(name)
    let mangledName = root["mangledName"] as _string // todo: undecorate mangled name
    var qualType = getQualType(root)
    if (verbose) {
        print("global variable {name} // {fullName}\n// {mangledName}\n// {qualType}\n")
    }
    ast.globals[fullName] <- ClangGlobalVar(
        name = name,
        cppName = fullName,
        mangledName = mangledName,
        gtype = qualType
    )
}

def getQualType(var root; field : string = "type") {
    //! Extracts the qualified type string from a Clang AST node.
    var qto & = unsafe(root[field] as _object)
    return qto["qualType"] as _string
}

def onNumericLiteral(var inner_obj) {
    //! Extracts the value and type from a numeric literal AST node.
    var value = ""
    var valueType = ""
    let valueCategory = inner_obj["valueCategory"] as _string
    if (valueCategory == "rvalue") {
        value = inner_obj["value"] as _string
    }
    valueType = getQualType(inner_obj)
    return (value, valueType)
}

def onInitArg(var root) {
    //! Parses the default argument initializer from a function parameter node.
    var value = ""
    var valueType = ""
    if (root |> key_exists("inner")) {
        root |> with_array("inner") $(inner) {
            var inner_obj & = unsafe(inner[0] as _object)
            let kind = inner_obj["kind"] as _string
            if (kind == "ImplicitCastExpr") {
                let castKind = inner_obj["castKind"] as _string
                if (castKind == "NullToPointer") {
                    value = "nullptr"
                } elif (castKind == "ArrayToPointerDecay") {
                    let valueCategory = inner_obj["valueCategory"] as _string
                    valueType = getQualType(inner_obj)
                    if (valueCategory == "rvalue") {
                        if (valueType == "const char *") {
                            inner_obj |> with_array("inner") $(iarr) {
                                var iobj & = unsafe(iarr[0] as _object)
                                value = iobj["value"] as _string
                            }
                        } else {
                            print("warning: unsupported decay type {valueType}")
                        }
                    }
                } else {
                    print("warning: unsupproted cast kind {castKind}\n")
                }
            } elif (kind == "IntegerLiteral" || kind == "FloatingLiteral") {
                let literal = onNumericLiteral(inner_obj)
                value = literal._0
                valueType = literal._1
            } elif (kind == "UnaryOperator") {
                let opCode = inner_obj["opcode"] as _string
                inner_obj |> with_array("inner") $(iinner) {
                    var iinner_obj & = unsafe(iinner[0] as _object)
                    let ikind = iinner_obj["kind"] as _string
                    if (ikind == "IntegerLiteral" || ikind == "FloatingLiteral") {
                        let literal = onNumericLiteral(iinner_obj)
                        value = opCode + literal._0
                        valueType = literal._1
                    }
                }
            } elif (kind == "CXXBoolLiteralExpr") {
                let valueCategory = inner_obj["valueCategory"] as _string
                if (valueCategory == "rvalue") {
                    value = (inner_obj["value"] as _bool) ? "true" : "false"
                }
                valueType = getQualType(inner_obj)
            } else {
                print("warning: unsupproted init kind {kind}\n")
            }
        }
    }
    return (value, valueType)
}

def onFunctionBody(var root; isMethod : bool; ofClass : string = "") : ClangFunc {
    //! Parses a function or method declaration into a ClangFunc descriptor.
    let name = root["name"] as _string
    let fullName = getCppName(name)
    let mangledName = root["mangledName"] as _string // todo: undecorate mangled name
    var qualType = getQualType(root)
    if (verbose) {
        print("function {name} // {fullName}\n// {mangledName}\n// {qualType}\n")
    }
    var isStatic = false
    if (isMethod && root |> key_exists("storageClass")) {
        if (root["storageClass"] as _string == "static") {
            isStatic = true
        }
    }
    var ffunc <- ClangFunc(
        name = name,
        cppName = fullName,
        mangledName = mangledName,
        ftype = qualType,
        isMethod = isMethod,
        isStatic = isStatic,
        ofClass = ofClass
    )
    if (root |> key_exists("inner")) {
        root |> with_array("inner") $(inner) {
            for (fad in inner) {
                assert(fad is _object)
                var ad & = unsafe(fad as _object)
                if (ad["kind"] as _string == "ParmVarDecl") {
                    let fieldName = key_exists(ad, "name") ? (ad["name"] as _string) : ""
                    var fqualType = getQualType(ad)
                    var fieldInit = ""
                    var fieldInitType = ""
                    if (key_exists(ad, "init")) {
                        let arg = onInitArg(ad)
                        fieldInit  = arg._0
                        fieldInitType = arg._1
                    }
                    ffunc.args |> emplace(ClangFuncArg(
                        name = fieldName,
                        atype = fqualType,
                        value = fieldInit,
                        vtype = fieldInitType
                    ))
                    if (verbose) {
                        print("\t{fqualType} {fieldName} = {fieldInit}\n")
                    }
                }
            }
        }
    }
    return <- ffunc
}

def onMethod(var root) {
    //! Processes a C++ method declaration within a struct or class.
    if (length(struct_stack) > 0) {
        var in_class = struct_stack[length(struct_stack) - 1]
        ast.structs[in_class].methods |> emplace <| onFunctionBody(root, true, join(struct_stack, "::"))
    } else {
        print("WARNING: method not in class {root["name"]}")
    }
}

def onFunction(var root) {
    //! Processes a standalone C++ function declaration.
    var fdecl <- onFunctionBody(root, false)
    ast.funcs[fdecl.mangledName] <- fdecl
}

def onStruct(var root) {
    //! Processes a C++ struct or class declaration and its members.
    let name = key_exists(root, "name") ? (root["name"] as _string) : ""
    let tagUsed = root["tagUsed"] as _string
    let fullName = getCppName(name)
    if (verbose) {
        print("{tagUsed} {name} // {fullName}\n")
    }
    if (!root |> key_exists("inner")) {
        return
    }
    if (tagUsed == "struct") {
        let pub = AccessKind.Public
        access_stack |> push(pub)
    } else {
        access_stack |> push(AccessKind.Private)
    }
    ast.structs[fullName] <- ClangStruct(
        name = name,
        cppName = fullName,
        tag = tagUsed
    )
    name_stack |> push(Scope(name = name, kind = ScopeKind.Struct))
    struct_stack |> push(fullName)
    onInner(root)
    struct_stack |> pop()
    name_stack |> pop()
    access_stack |> pop()
}

def onAccess(var root) {
    //! Updates the current access specifier (public, private, or protected).
    let access = root["access"] as _string
    var kind = AccessKind.Public
    if (access == "public") {
        kind = AccessKind.Public
    } elif (access == "private") {
        kind = AccessKind.Private
    } elif (access == "protected") {
        kind = AccessKind.Protected
    } else {
        panic("unsupported access kind {access}\n")
    }
    access_stack[length(access_stack) - 1] = kind
}

def isBitfield(var root) {
    //! Checks if a field declaration is a bitfield.
    if (root |> key_exists("isBitfield")) {
        if (root["isBitfield"] as _bool) {
            return true
        }
    }
    return false
}

def onField(var root) {
    //! Processes a struct or class field declaration.
    let name = key_exists(root, "name") ? (root["name"] as _string) : ""
    var qualType = getQualType(root)
    var access = access_stack[length(access_stack) - 1]
    if (verbose) {
        print("\t{qualType} {name} // {access}\n")
    }
    let sfn = struct_stack[length(struct_stack) - 1]
    ast.structs[sfn].fields |> push(ClangStructField(
        name = name,
        cppName = name,
        qtype = qualType,
        access = access,
        isBitfield = root |> isBitfield
    ))
}

def getCppName(baseName) {
    //! Builds a fully qualified C++ name using the current namespace stack.
    var name = join([iterator for(n in name_stack); n.name], "::")
    if (length(name) != 0) {
        name = "{name}::{baseName}"
    } else {
        name = baseName
    }
    return name
}

def onTypeDecl(var root) {
    //! Processes a C++ typedef declaration.
    let name = root["name"] as _string
    let fullName = getCppName(name)
    var qualType = getQualType(root)
    if (verbose) {
        print("typedecl {name} = {qualType} // {fullName}\n")
    }
    ast.typedefs[fullName] = ClangTypedef(
        name = name,
        cppName = fullName,
        qtype = qualType
    )
}

def isBlockedType(qtype : string) {
    //! Checks if a type name matches any blocked type pattern.
    for (btn in blocked_type_names) {
        if (qtype |> find(btn) != -1) {
            return true
        }
    }
    return false
}

def isLocalType(qtype : string) {
    //! Checks if a type name matches any local type pattern.
    for (btn in local_type_names) {
        if (qtype |> find(btn) != -1) {
            return true
        }
    }
    return false
}


def genEnums(hf, mf, df : FILE const?) {
    //! Generates C++ binding code for all parsed enumerations.
    fprint(hf, "// ------------\n// enumerations\n// ------------\n\n")
    fprint(mf, "// ------------\n// enumerations\n// ------------\n\n")
    fprint(df, "// ------------\n// enumerations\n// ------------\n\n")
    for (en in values(ast.enums)) {
        var enName = en.name
        var removePrefix = false
        var isFlags = false
        get(ex_enums, en.name) $(val) {
            enName = val.name
            removePrefix = val.removePrefix
            isFlags = val.isFlags
        }
        if (blocked_enumerations_table |> key_exists(en.cppName)) {
            continue
        }
        fprint(df, "#ifdef {getDefinePrefix(en.cppName)}\n")
        fprint(df, "addEnumeration(make_smart<Enumeration{enName}>());\n")
        if (isFlags) {
            fprint(df, "addEnumFlagOps<{en.cppName}>(*this,lib,\"{en.cppName}\");\n")
        }
        fprint(df, "#endif\n")
        if (en.eclass) {
            fprint(hf, "BIND_ENUM_CAST({en.cppName})\n")
            fprint(hf, "DAS_BASE_BIND_ENUM_GEN({en.cppName},{enName})\n")
        } else {
            fprint(hf, "DAS_BIND_ENUM_CAST({en.cppName})\n")
            fprint(hf, "DAS_BASE_BIND_ENUM_GEN({en.cppName},{enName})\n")
        }
        fprint(mf, "#ifdef {getDefinePrefix(en.cppName)}\n")
        fprint(mf, "
class Enumeration{enName} : public das::Enumeration \{
public:
    Enumeration{enName}() : das::Enumeration(\"{enName}\") \{
        external = true
        cppName = \"{en.cppName}\"
        baseType = (das::Type) das::ToBasicType< das::underlying_type< {en.cppName} >::type >::type
");
        let uename = to_upper(en.name);
        for (ene in en.edecl) {
            var entryName = ene;
            var entryCppName = ene;
            if (removePrefix) {
                if (ene |> starts_with("{en.name}_")) {
                    entryName = slice(ene, length(en.name) + 1);
                } elif (ene |> starts_with(en.name)) {
                    entryName = slice(ene, length(en.name));
                } elif (ene |> starts_with("{uename}_")) {
                    entryName = slice(ene, length(en.name) + 1);
                } elif (ene |> starts_with(uename)) {
                    entryName = slice(ene, length(en.name));
                } else {
                    print("warning: {ene} does not start with {en.name}\n");
                }
            }
            if (ex_keywords |> key_exists(entryName)) {
                entryName = "_{entryName}";
            }
            fprint(mf, "\t\taddIEx(\"{entryName}\", \"{entryCppName}\", int64_t({en.cppName}::{ene}), das::LineInfo());\n");
        }
        fprint(mf, "\t\}\n\};\n");
        fprint(mf, "#endif\n");
    }
    fprint(hf, "\n");
    fprint(df, "\n");
}

def needToGenStruct(st) {
    //! Determines if a struct should be included in binding generation.
    if (empty(st.name)) {
        return false
    }
    if (is_struct_blocked != null) {
        if (invoke(is_struct_blocked, st.name)) {
            return false
        }
    }
    return true
}

def needToGenField(fld) {
    //! Determines if a struct field should be included in binding generation.
    if (empty(fld.name)) {
        return false
    }
    if (fld.access != AccessKind.Public) {
        return false
    }
    if (fld.qtype |> isBlockedType) {
        return false
    }
    return true
}

def getBindFldName(name : string) {
    //! Returns a safe binding field name, prefixing reserved words with underscore.
    if (name == "type") {
        return "_{name}"
    }
    return name
}

def genStructs(hf, mf, df : FILE const?) {
    //! Generates C++ binding code for all parsed structures and their fields.
    fprint(hf, "// ------------\n// structures\n// ------------\n\n")
    fprint(mf, "// ------------\n// structures\n// ------------\n\n")
    fprint(df, "// ------------\n// structures\n// ------------\n\n")
    for (st in values(ast.structs)) {
        if (!needToGenStruct(st)) {
            continue
        }
        if (!(alias_types |> key_exists(st.cppName))) {
            fprint(mf, "#ifdef {getDefinePrefix(st.cppName)}\n")
            fprint(mf, "\tIMPLEMENT_EXTERNAL_TYPE_FACTORY({st.name},{st.cppName})\n")
            fprint(mf, "#endif\n")
        }
    }
    fprint(mf, "\n")
    for (st in values(ast.structs)) {
        if (!needToGenStruct(st)) {
            continue
        }
        if (alias_types |> key_exists(st.cppName)) {
            /*
            let alias_value = alias_types[st.cppName]
            fprint(mf,"template <> struct typeFactory<{st.cppName}> \{\n")
            fprint(mf,"\tstatic TypeDeclPtr make(const ModuleLibrary &) \{\n")
            fprint(mf,"\t\tauto t = make_smart<TypeDecl>({alias_value});\n")
            fprint(mf,"\t\tt->alias = \"{st.name}\";\n")
            fprint(mf,"\t\tt->aotAlias = true;\n")
            fprint(mf,"\t\treturn t;\n")
            fprint(mf,"\t\}\n")
            fprint(mf,"\};\n")
            fprint(mf,"template <> struct typeName<{st.cppName}> \{ constexpr static const char * name() \{ return \"{st.name}\"; \} \};\n")
            fprint(df,"addAlias(typeFactory<{st.cppName}>::make(lib));\n")
            */
            continue
        }
        fprint(df, "#ifdef {getDefinePrefix(st.cppName)}\n")
        fprint(df, "auto ann_{st.name} = make_smart<{st.name}_GeneratedAnnotation>(lib);\n")
        fprint(df, "addAnnotation(ann_{st.name});\n")
        fprint(df, "#endif\n")
        fprint(hf, "MAKE_EXTERNAL_TYPE_FACTORY({st.name},{st.cppName})\n")
        fprint(mf, "#ifdef {getDefinePrefix(st.cppName)}\n")
        fprint(mf, "struct {st.name}_GeneratedAnnotation : ManagedStructureAnnotation<{st.cppName}> \{\n")
        if (isLocalType(st.name)) {
            fprint(mf, "\tvirtual bool isLocal() const override \{ return true; \}\n")
            fprint(mf, "\tvirtual bool canBePlacedInContainer() const override \{ return true; \}\n")
        }
        fprint(mf, "\t{st.name}_GeneratedAnnotation(ModuleLibrary & ml) : ManagedStructureAnnotation (\"{st.name}\", ml) \{\n")
        fprint(mf, "\t\}\n")
        fprint(mf, "\tvoid init () \{\n")
        for (fld in st.fields) {
            if (!needToGenField(fld)) {
                continue
            }
            if (fld.isBitfield) {// TODO: support bitfields
                continue
            }
            var substType = ""
            substitute_field_types |> get("{st.cppName}::{fld.cppName}") $(val) {
                substType = val
            }
            if (substType |> empty()) {
                fprint(mf, "\t\taddField<DAS_BIND_MANAGED_FIELD({fld.cppName})>(\"{getBindFldName(fld.name)}\",\"{fld.cppName}\");\n")
            } else {
                fprint(mf, "\t\taddField<{substType},offsetof(ManagedType,{fld.cppName})>(\"{getBindFldName(fld.name)}\",\"{fld.cppName}\")\n")
                fprint(mf, "\t\t\t.adjustAot(\"das_reinterpret<{substType}>::pass(\",\")\");\n")
            }
        }
        fprint(mf, "\t\}\n\};\n")
        fprint(mf, "#endif\n")
    }
    for (st in values(ast.structs)) {
        if (!needToGenStruct(st)) {
            continue
        }
        if (alias_types |> key_exists(st.cppName)) {
            continue
        }
        fprint(df, "#ifdef {getDefinePrefix(st.cppName)}\n")
        fprint(df, "initRecAnnotation(ann_{st.name},lib);\n")
        fprint(df, "#endif\n")
    }
    fprint(hf, "\n")
    fprint(df, "\n")
}

var op_regex <- %regex~operator[^a-zA-Z_]%% //! Regex pattern matching C++ operator overload names.

def isBlockedFun(fn) {
    //! Checks if a function should be excluded from binding generation.
    if (fn.isMethod && fn.isStatic) {
        print("warning: skipping static method {fn.name} in {fn.cppName}\n")
        return true
    }
    if (regex_match(op_regex, fn.name) != -1) {
        print("warning: skipping {fn.name} in {fn.cppName}\n")
        return true
    }
    if (fn.ftype |> find("...") != -1) {
        print("warning: variadic function {fn.name} aka {fn.cppName}\n")
        return true
    }
    for (fa in fn.args) {
        if (fa.atype |> isBlockedType) {
            return true
        }
    }
    if (blocked_functions_table |> key_exists(fn.cppName)) {
        return true
    }
    if (is_function_blocked != null) {
        if (invoke(is_function_blocked, fn.cppName)) {
            return true
        }
    }
    return false
}

def qType2FnType(st) {
    //! Converts a qualified type string to a C function pointer type.
    let ob = find(st, "(")
    assert(ob != -1)
    return slice(st, 0, ob) + "(*)" + slice(st, ob)
}

def qType2FnTypeClass(st, className) {
    //! Converts a qualified type string to a class member function pointer type.
    let ob = find(st, "(")
    assert(ob != -1)
    return slice(st, 0, ob) + "({className}::*)" + slice(st, ob)
}

def getFunRetType(fn) {
    //! Extracts the return type from a function's qualified type string.
    let ob = find(fn.ftype, "(")
    assert(ob != -1)
    var cppResType = slice(fn.ftype, 0, ob)
    while (cppResType |> ends_with(" ")) {
        cppResType = slice(cppResType, 0, -1)
    }
    return cppResType
}

def isRefFun(fn) {
    //! Checks if a function returns a reference type.
    let ob = find(fn.ftype, "(")
    assert(ob != -1)
    var cppResType = slice(fn.ftype, 0, ob)
    if (cppResType |> starts_with("const ")) {
        cppResType = slice(cppResType, 6)
    }
    if (cppResType |> ends_with(" ")) {
        cppResType = slice(cppResType, 0, -1)
    }
    if (cppResType |> ends_with("&")) {
        return true
    }
    return false
}

def isCmresFun(fn) {
    //! Checks if a function returns by value a type requiring copy-or-move semantics.
    // note: hacking cpp type to autodetect CMRES
    let ob = find(fn.ftype, "(")
    assert(ob != -1)
    var cppResType = slice(fn.ftype, 0, ob)
    if (cppResType |> starts_with("const ")) {
        cppResType = slice(cppResType, 6)
    }
    if (cppResType |> ends_with(" ")) {
        cppResType = slice(cppResType, 0, -1)
    }
    for (atype in keys(alias_types)) {
        if (cppResType == atype) {
            return false
        }
    }
    if (ast.structs |> key_exists(cppResType)) {
        return true
    }
    if (alt_struct_name != null) {
        let aname = invoke(alt_struct_name, cppResType)
        if (aname != "") {
            if (ast.structs |> key_exists(aname)) {
                return true
            }
        }
    }
    for (bf in cmres_functions) {
        if (bf == fn.cppName) {
            return true
        }
    }
    return false
}

var g_method_name = 0 //! Counter for generating unique method binding names.

def methodName(name : string) {
    //! Generates a unique internal name for a method binding.
    g_method_name ++
    return "_{name}_method_{g_method_name}"
}

var g_split_count = 0 //! Counter for the number of functions generated in the current split file.
var g_split_factor = 20 //! Number of functions per split output file.
var g_split_file : FILE const? = null //! File handle for the current split output file.

def openSplitFile {
    //! Opens a new split output file when the split threshold is reached.
    if ((g_split_count % g_split_factor) == 0) {
        closeSplitFile()
        let split_index = g_split_count / g_split_factor
        let fname = "{split_prefix}_{split_index}.cpp"
        print("SPLIT {split_index} at {fname}\n")
        g_split_file = fopen(fname, "wb")
        var prefix = replace(split_cpp_prefix, "$$$", "{split_index}")
        fprint(g_split_file, prefix)
    }
    g_split_count ++
}

def closeSplitFile {
    //! Closes the current split output file if one is open.
    if (g_split_file != null) {
        fprint(g_split_file, split_cpp_suffix)
        fclose(g_split_file)
        g_split_file = null
    }
}

def getFunctions(df, df_method : FILE const?) {
    //! Generates C++ binding code for all parsed functions and methods.
    fprint(df, "// ------------\n// functions\n// ------------\n\n")
    var all_fn : table<string; int>
    for (fn in values(ast.funcs)) {
        all_fn[fn.cppName] ++
    }
    for (fn in values(ast.funcs)) {
        if (fn |> isBlockedFun) {
            continue
        }
        let castbind = all_fn[fn.cppName] > 1
        genFunction(fn, castbind, df)
        if (generate_split_functions) {
            openSplitFile()
            genFunction(fn, castbind, g_split_file)
        }
    }

    fprint(df_method, "// ------------\n// methods \n// ------------\n\n")
    for (st in values(ast.structs)) {
        var all_met : table<string; int>
        for (fn in st.methods) {
            all_met[fn.name] ++
        }
        for (fn in st.methods) {
            if (fn |> isBlockedFun) {
                continue
            }
            let castbind = all_met[fn.name] > 1
            genFunction(fn, castbind, df_method)
            if (generate_split_functions) {
                openSplitFile()
                genFunction(fn, castbind, g_split_file)
            }
        }
    }
}

var {
    reg_args <- %regex~(.+)\s*\((.+)\)%%
}

def parseQualArgs(qname : string; var rtype : string&) {
    //! Parses argument types and return type from a qualified function type string.
    if (regex_match(reg_args, qname) != -1) {
        let qargs = regex_group(reg_args, 2, qname)
        rtype = regex_group(reg_args, 1, qname)
        return <-  split(qargs, ", ")
    }
    return <- array<string>()
}

def getDefinePrefix(xname : string) {
    //! Returns the preprocessor define prefix for a given C++ qualified name.
    for (nn, pn in keys(namespace_to_prefix), values(namespace_to_prefix)) {
        if (xname |> starts_with(nn)) {
            return pn
        }
    }
    return "GLOBAL_NAMESPACE"
}

var gen_fn_callback : lambda<(fn : ClangFunc) : void> //! Optional callback invoked for each generated function binding.

def genFunction(fn : ClangFunc; castbind : bool; df : FILE const?) {
    //! Generates C++ binding code for a single function or method.
    var bind_enchantation = ""
    var cpp_enchantation = ""
    var using_mname = ""
    var using_mname_expr = ""
    if (gen_fn_callback != null) {
        invoke(gen_fn_callback, fn)
    }
    if (fn.isMethod) {
        if (castbind) {
            let fType = qType2FnTypeClass(fn.ftype, fn.ofClass)
            using_mname = methodName(fn.name)
            using_mname_expr = "das::das_call_member<{fType},&{fn.cppName}>"
            bind_enchantation = "DAS_CALL_METHOD({using_mname})"
            cpp_enchantation = "\"das_call_member<{fType},&{fn.cppName}>::invoke\""
            print("warning: {fn.cppName} using clang-ast binding of {fType}\n")
        } else {
            using_mname = methodName(fn.name)
            bind_enchantation = "DAS_CALL_METHOD({using_mname})"
            cpp_enchantation = "DAS_CALL_MEMBER_CPP({fn.cppName})"
        }
    } else {
        if (castbind) {
            let fType = qType2FnType(fn.ftype)
            bind_enchantation = "{fType},{fn.cppName}"
            cpp_enchantation = "\"{fn.cppName}\""
            print("warning: {fn.cppName} using clang-ast binding of {fType}\n")
        } else {
            bind_enchantation = "DAS_BIND_FUN({fn.cppName})"
            cpp_enchantation = "\"{fn.cppName}\""
        }
    }
    var extra_enchantation = ""
    if (fn |> isRefFun) {
        extra_enchantation = ", SimNode_ExtFuncCallRef"
    } elif (fn |> isCmresFun) {
        extra_enchantation = ", SimNode_ExtFuncCallAndCopyOrMove"
    }
    fprint(df, "#ifdef {getDefinePrefix(fn.cppName)}\n")
    if (!empty(using_mname)) {
        if (!empty(using_mname_expr)) {
            fprint(df, "using {using_mname} = {using_mname_expr};\n")
        } else {
            fprint(df, "using {using_mname} = DAS_CALL_MEMBER({fn.cppName});\n")
        }
    }
    fprint(df, "addExtern<{bind_enchantation}{extra_enchantation}>(*this, lib, \"{fn.name}\",SideEffects::worstDefault, {cpp_enchantation})\n")
    fprint(df, "\t->args(\{")
    if (fn.isMethod) {
        fprint(df, "\"self\",")
    }
    for (arg, argi in fn.args, count()) {
        if (argi != 0) {
            fprint(df, ",")
        }
        if (empty(arg.name)) {
            fprint(df, "\"arg{fn.isMethod ? argi+1 : argi}\"")
        } else {
            fprint(df, "\"{arg.name}\"")
        }
    }
    if (!extra_args |> empty) {
        if (length(fn.args) > 0) {
            fprint(df, ",")
        }
        fprint(df, extra_args)
    }
    fprint(df, "\})\n")
    var rtype : string
    var qargs <- parseQualArgs(fn.ftype, rtype)
    var revArgs = true
    if (length(qargs) != length(fn.args)) {
        print("warning: args did not parse '{fn.ftype}' to {qargs}\n")
        delete qargs
        revArgs = false
    }
    if (revArgs) {
        get(rev_enums, rtype) $(rt) {
            fprint(df, "\t\t->res_type(makeType<{rt}>(lib))\n")
        }
    }
    for (arg, _argi in fn.args, count()) {
        let argi = fn.isMethod ? _argi + 1 : _argi
        var thisArgEnum = ""
        if (revArgs) {
            get(rev_enums, qargs[_argi]) $(en) {
                thisArgEnum = en
                fprint(df, "\t\t->arg_type({argi},makeType<{thisArgEnum}>(lib))\n")
            }
        }
        if (!empty(arg.value)) {
            if (arg.value == "nullptr") {
                if (arg.atype == "const char *") {
                    fprint(df, "\t\t->arg_init({argi},make_smart<ExprConstString>(\"\"))\n")
                } else {
                    fprint(df, "\t\t->arg_init({argi},make_smart<ExprConstPtr>())\n")
                }
            } elif (arg.vtype == "int") {
                if (empty(thisArgEnum)) {
                    fprint(df, "\t\t->arg_init({argi},make_smart<ExprConstInt>({arg.value}))\n")
                } else {
                    fprint(df, "\t\t->arg_init({argi},make_smart<ExprConstEnumeration>({arg.value},makeType<{thisArgEnum}>(lib)))\n")
                }
            } elif (arg.vtype == "float") {
                fprint(df, "\t\t->arg_init({argi},make_smart<ExprConstFloat>({arg.value}))\n")
            } elif (arg.vtype == "bool") {
                fprint(df, "\t\t->arg_init({argi},make_smart<ExprConstBool>({arg.value}))\n")
            } elif (arg.vtype == "const char *") {
                fprint(df, "\t\t->arg_init({argi},make_smart<ExprConstString>({arg.value}))\n")
            } else {
                fprint(df, "\t\t// {arg.name} : {arg.atype} = {arg.value}\n")
                print("warning: unsupported init {arg.atype} of {arg.vtype}\n")
            }
        }
    }
    fprint(df, ";\n")
    fprint(df, "#endif\n")
}

def genBindings(hf, df, mf_enum, df_enum, mf_ann, df_ann, df_method : FILE const?) {
    //! Generates all C++ bindings (enums, structs, functions) to the output files.
    genEnums(hf, mf_enum, df_enum)
    genStructs(hf, mf_ann, df_ann)
    getFunctions(df, df_method)
    closeSplitFile()
}

def bindHeaders(fname, outprefix : string) {
    //! Parses a Clang AST JSON file and generates all binding output files.
    delete blocked_functions_table
    for (fn in blocked_functions) {
        blocked_functions_table[fn] = true
    }
    for (enk, env in keys(ex_enums), values(ex_enums)) {
        rev_enums[env.name] = enk
    }
    for (kwd in all_keywords) {
        ex_keywords[kwd] = true
    }
    for (be in blocked_enumerations) {
        blocked_enumerations_table[be] = true
    }
    fopen(fname, "rb") $(f) {
        if (f == null) {
            print("can't open {fname}")
        } else {
            fread(f) $(data) {
                print("\n{fname}\n")
                var error = ""
                var json = read_json(data, error)
                if (json == null) {
                    panic("failed to parse, {error}\n")
                } else {
                    assert(json is _object)
                    var root & = unsafe(json as _object)
                    verify(root["kind"] |> is_str <| "TranslationUnitDecl")
                    onInner(root)
                    // debug(ast,"AST =")
                    fopen("{outprefix}.h", "wb") $(hf) {
                        fwrite(hf, "#pragma once\n\n")
                        fopen("{outprefix}.inc", "wb") $(df) {
                            fopen("{outprefix}.enum.cpp_inc", "wb") $(mf_enum) {
                                fopen("{outprefix}.enum.inc", "wb") $(df_enum) {
                                    fopen("{outprefix}.ann.cpp_inc", "wb") $(mf_ann) {
                                        fopen("{outprefix}.ann.inc", "wb") $(df_ann) {
                                            fopen("{outprefix}.method.inc", "wb") $(df_method) {
                                                genBindings(hf, df, mf_enum, df_enum, mf_ann, df_ann, df_method)
                                            }
                                        }
                                    }
                                }
                            }
                        }
                    }
                }
            }
        }
    }
}


def genDefineConstants(fname, prefix : string) {
    //! Generates constant bindings from a single header file.
    var fnames <-  [ fname ]
    defer() { delete fnames; }
    fnames |> genDefineConstants <| prefix
}

def getGenConstType(name, default_name : string) {
    //! Returns the C++ type for a constant, falling back to the default type.
    return const_type_table?[name] ?? default_name
}

def searchAndGenConst(var regex_def : Regex; defTName, suffix : string; var ofs : table<int; bool>; data : string; var dup : table<string; bool>; hf : FILE const?) {
    //! Searches for #define constants matching a regex and generates binding code.
    regex_foreach(regex_def, data) $(r) {
        if (!ofs |> key_exists(r.x)) {
            let DEF = regex_group(regex_def, 1, data)
            let VAL = regex_group(regex_def, 2, data)
            if (!blocked_defines_table |> key_exists(DEF)) {
                if (is_const_blocked == null || !invoke(is_const_blocked, DEF)) {
                    let ctt = getGenConstType(DEF, defTName)
                    if (!dup[DEF]) {
                        fprint(hf, "addConstant<{ctt}>(*this,\"{DEF}\",{ctt}({VAL}{suffix}));\n")
                        dup[DEF] = true
                    } else {
                        print("const: duplicate {DEF}\n")
                    }
                }
            }
        }
        return true
    }
}

def genDefineConstants(fnames : array<string>; prefix : string) {
    //! Generates constant bindings from multiple header files.
    delete blocked_defines_table
    for (bd in blocked_defines) {
        blocked_defines_table[bd] = true
    }
    var reg_def_hex <- %regex~#define\s+(\w+)\s+(0x[0-9A-Za-z]+)%%
    var reg_def_dec <- %regex~#define\s+(\w+)\s+(\d+)%%
    var reg_def_UINT8 <- %regex~#define\s+(\w+)\s+UINT8_C\s*\((0x[0-9A-Za-z]+)\)%%
    var reg_def_UINT16 <- %regex~#define\s+(\w+)\s+UINT16_C\s*\((0x[0-9A-Za-z]+)\)%%
    var reg_def_UINT32 <- %regex~#define\s+(\w+)\s+UINT32_C\s*\((0x[0-9A-Za-z]+)\)%%
    var reg_def_UINT64 <- %regex~#define\s+(\w+)\s+UINT64_C\s*\((0x[0-9A-Za-z]+)\)%%
    fopen("{prefix}.const_inc", "wb") $(hf) {
        if (hf == null) {
            panic("can't write {prefix}.const_inc")
        }
        for (fname in fnames) {
            fopen(fname, "rb") $(f) {
                if (f == null) {
                    panic("can't open {fname}")
                }
                var ofs : table<int; bool>
                let data = fread(f)
                var dup : table<string; bool>
                searchAndGenConst(reg_def_hex, "uint32_t", "u", ofs, data, dup, hf)
                searchAndGenConst(reg_def_dec, "int32_t", "", ofs, data, dup, hf)
                searchAndGenConst(reg_def_UINT8, const_uint8_type, "u", ofs, data, dup, hf)
                searchAndGenConst(reg_def_UINT16, const_uint16_type, "u", ofs, data, dup, hf)
                searchAndGenConst(reg_def_UINT32, "uint32_t", "u", ofs, data, dup, hf)
                searchAndGenConst(reg_def_UINT64, "uint64_t", "ul", ofs, data, dup, hf)
            }
        }
    }
}
