options indenting = 4
options no_unused_block_arguments = false
options no_unused_function_arguments = false
options no_aot = true
options strict_smart_pointers = true

module cpp_gen public

require fio
require daslib/defer
require daslib/json
require daslib/json_boost
require daslib/strings_boost
require daslib/regex
require daslib/regex_boost

let verbose = false

/*
    config
*/

struct ExEnum
    name : string
    removePrefix : bool = true
    isFlags : bool = false

var extra_args = ""

var local_type_names : array<string>

var blocked_type_names : array<string>

var blocked_functions : array<string> <-  [{string []
    "operator delete";
    "operator new";
    "__security_init_cookie";
    "__security_check_cookie"
}];

var blocked_functions_table : table<string;bool>

var blocked_defines : array<string> <- [{string []
    "true";
    "false"
}]

var blocked_defines_table : table<string;bool>

var blocked_enumerations : array<string>

var blocked_enumerations_table : table<string;bool>

var is_function_blocked : lambda<(name:string):bool>

var is_const_blocked : lambda<(name:string):bool>

var is_struct_blocked : lambda<(name:string):bool>

var alt_struct_name : lambda<(name:string):string>

var cmres_functions : array<string>

var allow_extern_c = false

var ex_enums : table<string; ExEnum>

var substitute_field_types : table<string; string>

var alias_types : table<string; string>

var namespace_to_prefix : table<string; string>

var generate_split_functions = false
var split_prefix = ""
var split_cpp_prefix = ""
var split_cpp_suffix = ""

var enum_suffix = ""

var const_type_table : table<string; string>

var const_uint8_type = "uint8_t"
var const_uint16_type = "uint16_t"

var ex_keywords : table<string; bool>

var all_keywords = [[string
    "float";    "double";
    "uint";     "uint8";    "uint16";   "uint64";   "uint2";    "uint3";    "uint4";
    "int";      "int8";     "int16";    "int64";    "int2";     "int3";     "int4";
    "bool"
]]

/*
    end of configuration
*/

enum ScopeKind
    Namespace
    Struct

struct Scope
    name : string
    kind : ScopeKind

enum AccessKind
    Private
    Public
    Protected

struct ClangAstDecl
    name    : string
    cppName : string

struct ClangTypedef : ClangAstDecl
    qtype   : string

struct ClangEnum : ClangAstDecl
    eclass  : bool
    etype   : string
    edecl   : array<string>

struct ClangGlobalVar : ClangAstDecl
    mangledName : string
    gtype       : string

struct ClangStructField : ClangAstDecl
    qtype       : string
    access      : AccessKind
    isBitfield  : bool

struct ClangFuncArg : ClangAstDecl
    atype   : string
    value   : string
    vtype   : string

struct ClangFunc : ClangAstDecl
    mangledName : string
    ftype       : string
    isMethod    : bool
    isStatic    : bool
    ofClass     : string
    args        : array<ClangFuncArg>

struct ClangStruct : ClangAstDecl
    tag     : string
    fields  : array<ClangStructField>
    methods : array<ClangFunc>

struct ClangAst
    typedefs : table<string;ClangTypedef>
    enums : table<string;ClangEnum>
    structs : table<string;ClangStruct>
    funcs : table<string;ClangFunc>
    globals : table<string;ClangGlobalVar>

var name_stack : array<Scope>
var access_stack : array<AccessKind>
var struct_stack : array<string>
var ast : ClangAst
var rev_enums : table<string; string>

def is_str(jso,str)
    return (jso is _string) && ((jso as _string)==str)

def with_object(var jso; field; blk:block<(var val:table<string; JsonValue?>):void> )
    assert(jso[field] is _object)
    invoke(blk, jso[field] as _object)

def with_array(var jso; field; blk:block<(var val:array<JsonValue?>):void> )
    assert(jso[field] is _array)
    invoke(blk, jso[field] as _array)

def is_valid_loc(var loc)
    if loc |> length == 0
        return false
    return true

def onInner(var root)
    root |> with_array("inner") <| $(inner)
        for idecl in inner
            assert(idecl is _object)
            var decl & = unsafe(idecl as _object)
            var skip = false
            if decl |> key_exists("loc")
                decl |> with_object("loc") <| $ ( loc )
                    skip = ! loc |> is_valid_loc
            if skip
                if verbose
                    print("// skipping {decl["name"] as _string}\n")
                continue
            let kind = decl["kind"] as _string
            if kind=="CXXRecordDecl"
                onStruct(decl)
            elif kind=="NamespaceDecl"
                onNamespace(decl)
            elif kind=="FieldDecl"
                onField(decl)
            elif kind=="FunctionDecl"
                onFunction(decl)
            elif kind=="CXXMethodDecl"
                onMethod(decl)
            elif kind=="EnumDecl"
                onEnum(decl)
            elif kind=="VarDecl"
                onVar(decl)
            elif kind=="TypedefDecl"
                onTypeDecl(decl)
            elif kind=="AccessSpecDecl"
                onAccess(decl)
            elif kind=="LinkageSpecDecl"
                onLinkageSpec(decl)
            else
                print("warning: unsupported {kind}\n")


[sideeffects] // TODO: fixme. find why we need to specify [sideeffects] here
def onLinkageSpec(var root)
    if !allow_extern_c
        return
    if ! root |> key_exists("inner")
        return
    if verbose
        if root |> key_exists("language")
            print("extern \"{root["language"] as _string}\"\n")
    onInner(root)

def onNamespace(var root)
    if ! root |> key_exists("inner")
        return
    let name = root["name"] as _string
    let fullName = join([[for n in name_stack; n.name]],"::") + "::" + name
    if verbose
        print("namespace {name} //{fullName}\n")
    name_stack |> push([[Scope name=name, kind=ScopeKind Namespace]])
    onInner(root)
    name_stack |> pop()

def onEnum(var root)
    if !root |> key_exists("name")
        print("enum 'name' missing in {root}\n")
        return
    let name = root["name"] as _string
    let fullName = getCppName(name)
    if root |> key_exists("scopedEnumTag")
        let tag = root["scopedEnumTag"] as _string
        let qtype = getQualType(root,"fixedUnderlyingType")
        if verbose
            print("enum {tag} {name} : {qtype} // {fullName}\n")
        ast.enums[fullName] <- [[ClangEnum
            name=name,
            cppName="{fullName}{enum_suffix}",
            eclass=true,
            etype=qtype
        ]]
    else
        if verbose
            print("enum {name} // {fullName}\n")
        ast.enums[fullName] <- [[ClangEnum
            name=name,
            cppName="{fullName}{enum_suffix}",
            eclass=false
        ]]
    root |> with_array("inner") <| $ ( inner )
        for edo in inner
            assert(edo is _object)
            var ed & = unsafe(edo as _object)
            if ed["kind"] as _string == "EnumConstantDecl"
                var cname = ed["name"] as _string
                ast.enums[fullName].edecl |> push(cname)

def onVar(var root)
    let name = root["name"] as _string
    let fullName = getCppName(name)
    let mangledName = root["mangledName"] as _string // todo: undecorate mangled name
    var qualType = getQualType(root)
    if verbose
        print("global variable {name} // {fullName}\n// {mangledName}\n// {qualType}\n")
    ast.globals[fullName] <- [[ClangGlobalVar
        name = name,
        cppName = fullName,
        mangledName = mangledName,
        gtype = qualType
    ]]

def getQualType(var root; field:string="type")
    var qto & = unsafe(root[field] as _object)
    return qto["qualType"] as _string

def onNumericLiteral(var inner_obj)
    var value = ""
    var valueType =""
    let valueCategory = inner_obj["valueCategory"] as _string
    if valueCategory=="rvalue"
        value = inner_obj["value"] as _string
    valueType = getQualType(inner_obj)
    return [[auto value, valueType]]

def onInitArg(var root)
    var value = ""
    var valueType = ""
    if root |> key_exists("inner")
        root |> with_array("inner") <| $ ( inner )
            var inner_obj & = unsafe(inner[0] as _object)
            let kind = inner_obj["kind"] as _string
            if kind=="ImplicitCastExpr"
                let castKind = inner_obj["castKind"] as _string
                if castKind=="NullToPointer"
                    value = "nullptr"
                elif castKind=="ArrayToPointerDecay"
                    let valueCategory = inner_obj["valueCategory"] as _string
                    valueType = getQualType(inner_obj)
                    if valueCategory=="rvalue"
                        if valueType=="const char *"
                            inner_obj |> with_array("inner") <| $ ( iarr )
                                var iobj & = unsafe(iarr[0] as _object)
                                value = iobj["value"] as _string
                        else
                            print("warning: unsupported decay type {valueType}")
                else
                    print("warning: unsupproted cast kind {castKind}\n")
            elif kind=="IntegerLiteral" || kind=="FloatingLiteral"
                let literal = onNumericLiteral(inner_obj)
                value = literal._0
                valueType = literal._1
            elif kind=="UnaryOperator"
                let opCode = inner_obj["opcode"] as _string
                inner_obj |> with_array("inner") <| $ (iinner)
                    var iinner_obj & = unsafe(iinner[0] as _object)
                    let ikind = iinner_obj["kind"] as _string
                    if ikind=="IntegerLiteral" || ikind=="FloatingLiteral"
                        let literal = onNumericLiteral(iinner_obj)
                        value = opCode + literal._0
                        valueType = literal._1
            elif kind=="CXXBoolLiteralExpr"
                let valueCategory = inner_obj["valueCategory"] as _string
                if valueCategory=="rvalue"
                    value = (inner_obj["value"] as _bool) ? "true" : "false"
                valueType = getQualType(inner_obj)
            else
                print("warning: unsupproted init kind {kind}\n")
    return [[auto value, valueType]]

def onFunctionBody(var root; isMethod:bool; ofClass:string="") : ClangFunc
    let name = root["name"] as _string
    let fullName = getCppName(name)
    let mangledName = root["mangledName"] as _string // todo: undecorate mangled name
    var qualType = getQualType(root)
    if verbose
        print("function {name} // {fullName}\n// {mangledName}\n// {qualType}\n")
    var isStatic = false
    if isMethod && root |> key_exists("storageClass")
        if root["storageClass"] as _string == "static"
            isStatic = true
    var ffunc <- [[ClangFunc
        name=name,
        cppName=fullName,
        mangledName=mangledName,
        ftype=qualType,
        isMethod=isMethod,
        isStatic=isStatic,
        ofClass=ofClass
    ]]
    if root |> key_exists("inner")
        root |> with_array("inner") <| $ ( inner )
            for fad in inner
                assert(fad is _object)
                var ad & = unsafe(fad as _object)
                if ad["kind"] as _string == "ParmVarDecl"
                    let fieldName = key_exists(ad,"name") ? (ad["name"] as _string) : ""
                    var fqualType = getQualType(ad)
                    var fieldInit = ""
                    var fieldInitType = ""
                    if key_exists(ad,"init")
                        let arg = onInitArg(ad)
                        fieldInit  = arg._0
                        fieldInitType = arg._1
                    ffunc.args |> emplace([[ClangFuncArg
                        name=fieldName,
                        atype=fqualType,
                        value=fieldInit,
                        vtype=fieldInitType
                    ]])
                    if verbose
                        print("\t{fqualType} {fieldName} = {fieldInit}\n")
    return <- ffunc

def onMethod(var root)
    if length(struct_stack)>0
        var in_class = struct_stack[length(struct_stack)-1]
        ast.structs[in_class].methods |> emplace <| onFunctionBody(root,true,join(struct_stack,"::"))
    else
        print("WARNING: method not in class {root["name"]}")

def onFunction(var root)
    var fdecl <- onFunctionBody(root,false)
    ast.funcs[fdecl.mangledName] <- fdecl

def onStruct(var root)
    let name = key_exists(root,"name") ? (root["name"] as _string) : ""
    let tagUsed = root["tagUsed"] as _string
    let fullName = getCppName(name)
    if verbose
        print("{tagUsed} {name} // {fullName}\n")
    if ! root |> key_exists("inner")
        return
    if tagUsed=="struct"
        access_stack |> push(AccessKind Public)
    else
        access_stack |> push(AccessKind Private)
    ast.structs[fullName] <- [[ClangStruct
        name=name,
        cppName=fullName,
        tag=tagUsed
    ]]
    name_stack |> push([[Scope name=name, kind=ScopeKind Struct]])
    struct_stack |> push(fullName)
    onInner(root)
    struct_stack |> pop()
    name_stack |> pop()
    access_stack |> pop()

def onAccess(var root)
    let access = root["access"] as _string
    var kind = AccessKind Public
    if access=="public"
        kind = AccessKind Public
    elif access=="private"
        kind = AccessKind Private
    elif access=="protected"
        kind = AccessKind Protected
    else
        panic("unsupported access kind {access}\n")
    access_stack[length(access_stack)-1] = kind

def isBitfield(var root)
    if root |> key_exists("isBitfield")
        if root["isBitfield"] as _bool
            return true
    return false

def onField(var root)
    let name = key_exists(root,"name") ? (root["name"] as _string) : ""
    var qualType = getQualType(root)
    var access = access_stack[length(access_stack)-1]
    if verbose
        print("\t{qualType} {name} // {access}\n")
    let sfn = struct_stack[length(struct_stack)-1]
    ast.structs[sfn].fields |> push([[ClangStructField
        name = name,
        cppName = name,
        qtype = qualType,
        access = access,
        isBitfield = root |> isBitfield
    ]])

def getCppName(baseName)
    var name = join([[for n in name_stack; n.name]],"::")
    if length(name) != 0
        name = "{name}::{baseName}"
    else
        name = baseName
    return name

def onTypeDecl(var root)
    let name = root["name"] as _string
    let fullName = getCppName(name)
    var qualType = getQualType(root)
    if verbose
        print("typedecl {name} = {qualType} // {fullName}\n")
    ast.typedefs[fullName] = [[ClangTypedef
        name=name,
        cppName=fullName,
        qtype=qualType
    ]]

def isBlockedType(qtype:string)
    for btn in blocked_type_names
        if qtype |> find(btn) != -1
            return true
    return false

def isLocalType(qtype:string)
    for btn in local_type_names
        if qtype |> find(btn) != -1
            return true
    return false


def genEnums(hf,mf,df:FILE const?)
    fprint(hf,"// ------------\n// enumerations\n// ------------\n\n")
    fprint(mf,"// ------------\n// enumerations\n// ------------\n\n")
    fprint(df,"// ------------\n// enumerations\n// ------------\n\n")
    for en in values(ast.enums)
        var enName = en.name
        var removePrefix = false
        var isFlags = false
        get(ex_enums, en.name) <| $ ( val )
            enName = val.name
            removePrefix = val.removePrefix
            isFlags = val.isFlags
        if blocked_enumerations_table |> key_exists(en.cppName)
            continue
        fprint(df,"#ifdef {getDefinePrefix(en.cppName)}\n")
        fprint(df,"addEnumeration(make_smart<Enumeration{enName}>());\n");
        if isFlags
            fprint(df,"addEnumFlagOps<{en.cppName}>(*this,lib,\"{en.cppName}\");\n");
        fprint(df,"#endif\n")
        if en.eclass
            fprint(hf,"BIND_ENUM_CAST({en.cppName})\n")
            fprint(hf,"DAS_BASE_BIND_ENUM_GEN({en.cppName},{enName})\n")
        else
            fprint(hf,"DAS_BIND_ENUM_CAST({en.cppName})\n")
            fprint(hf,"DAS_BASE_BIND_ENUM_GEN({en.cppName},{enName})\n")
        fprint(mf,"#ifdef {getDefinePrefix(en.cppName)}\n")
        fprint(mf,"
class Enumeration{enName} : public das::Enumeration \{
public:
    Enumeration{enName}() : das::Enumeration(\"{enName}\") \{
        external = true;
        cppName = \"{en.cppName}\";
        baseType = (das::Type) das::ToBasicType< das::underlying_type< {en.cppName} >::type >::type;
")
        let uename = to_upper(en.name)
        for ene in en.edecl
            var entryName = ene
            var entryCppName = ene
            if removePrefix
                if ene |> starts_with("{en.name}_")
                    entryName = slice(ene,length(en.name)+1)
                elif ene |> starts_with(en.name)
                    entryName = slice(ene,length(en.name))
                elif ene |> starts_with("{uename}_")
                    entryName = slice(ene,length(en.name)+1)
                elif ene |> starts_with(uename)
                    entryName = slice(ene,length(en.name))
                else
                    print("warning: {ene} does not start with {en.name}\n")
            if ex_keywords |> key_exists(entryName)
                entryName = "_{entryName}"
            fprint(mf,"\t\taddIEx(\"{entryName}\", \"{entryCppName}\", int64_t({en.cppName}::{ene}), das::LineInfo());\n")
        fprint(mf,"\t\}\n\};\n")
        fprint(mf,"#endif\n")
    fprint(hf,"\n")
    fprint(df,"\n")

def needToGenStruct(st)
    if empty(st.name)
        return false
    if is_struct_blocked!=null
        if invoke(is_struct_blocked, st.name)
            return false
    return true

def needToGenField(fld)
    if empty(fld.name)
        return false
    if fld.access != AccessKind Public
        return false
    if fld.qtype |> isBlockedType
        return false
    return true

def getBindFldName(name:string)
    if name=="type"
        return "_{name}"
    return name

def genStructs(hf,mf,df:FILE const?)
    fprint(hf,"// ------------\n// structures\n// ------------\n\n")
    fprint(mf,"// ------------\n// structures\n// ------------\n\n")
    fprint(df,"// ------------\n// structures\n// ------------\n\n")
    for st in values(ast.structs)
        if !needToGenStruct(st)
            continue
        if !(alias_types |> key_exists(st.cppName))
            fprint(mf,"#ifdef {getDefinePrefix(st.cppName)}\n")
            fprint(mf,"\tIMPLEMENT_EXTERNAL_TYPE_FACTORY({st.name},{st.cppName})\n")
            fprint(mf,"#endif\n")
    fprint(mf,"\n")
    for st in values(ast.structs)
        if !needToGenStruct(st)
            continue
        if alias_types |> key_exists(st.cppName)
            /*
            let alias_value = alias_types[st.cppName]
            fprint(mf,"template <> struct typeFactory<{st.cppName}> \{\n")
            fprint(mf,"\tstatic TypeDeclPtr make(const ModuleLibrary &) \{\n")
            fprint(mf,"\t\tauto t = make_smart<TypeDecl>({alias_value});\n")
            fprint(mf,"\t\tt->alias = \"{st.name}\";\n")
            fprint(mf,"\t\tt->aotAlias = true;\n")
            fprint(mf,"\t\treturn t;\n")
            fprint(mf,"\t\}\n");
            fprint(mf,"\};\n");
            fprint(mf,"template <> struct typeName<{st.cppName}> \{ constexpr static const char * name() \{ return \"{st.name}\"; \} \};\n")
            fprint(df,"addAlias(typeFactory<{st.cppName}>::make(lib));\n")
            */
            continue
        fprint(df,"#ifdef {getDefinePrefix(st.cppName)}\n")
        fprint(df,"auto ann_{st.name} = make_smart<{st.name}_GeneratedAnnotation>(lib);\n")
        fprint(df,"addAnnotation(ann_{st.name});\n")
        fprint(df,"#endif\n")
        fprint(hf,"MAKE_EXTERNAL_TYPE_FACTORY({st.name},{st.cppName})\n")
        fprint(mf,"#ifdef {getDefinePrefix(st.cppName)}\n")
        fprint(mf,"struct {st.name}_GeneratedAnnotation : ManagedStructureAnnotation<{st.cppName}> \{\n")
        if isLocalType(st.name)
            fprint(mf,"\tvirtual bool isLocal() const override \{ return true; \}\n")
            fprint(mf,"\tvirtual bool canBePlacedInContainer() const override \{ return true; \}\n")
        fprint(mf,"\t{st.name}_GeneratedAnnotation(ModuleLibrary & ml) : ManagedStructureAnnotation (\"{st.name}\", ml) \{\n")
        fprint(mf,"\t\}\n")
        fprint(mf,"\tvoid init () \{\n")
        for fld in st.fields
            if !needToGenField(fld)
                continue
            if fld.isBitfield           // TODO: support bitfields
                continue
            var substType = ""
            substitute_field_types |> get("{st.cppName}::{fld.cppName}") <| $ ( val )
                substType = val
            if substType |> empty()
                fprint(mf,"\t\taddField<DAS_BIND_MANAGED_FIELD({fld.cppName})>(\"{getBindFldName(fld.name)}\",\"{fld.cppName}\");\n")
            else
                fprint(mf,"\t\taddField<{substType},offsetof(ManagedType,{fld.cppName})>(\"{getBindFldName(fld.name)}\",\"{fld.cppName}\")\n")
                fprint(mf,"\t\t\t.adjustAot(\"das_reinterpret<{substType}>::pass(\",\")\");\n")
        fprint(mf,"\t\}\n\};\n")
        fprint(mf,"#endif\n")
    for st in values(ast.structs)
        if !needToGenStruct(st)
            continue
        if alias_types |> key_exists(st.cppName)
            continue
        fprint(df,"#ifdef {getDefinePrefix(st.cppName)}\n")
        fprint(df,"initRecAnnotation(ann_{st.name},lib);\n")
        fprint(df,"#endif\n")
    fprint(hf,"\n")
    fprint(df,"\n")

var op_regex <- %regex~operator[^a-zA-Z_]%%

def isBlockedFun(fn)
    if fn.isMethod && fn.isStatic
        print("warning: skipping static method {fn.name} in {fn.cppName}\n")
        return true
    if regex_match(op_regex, fn.name) != -1
        print("warning: skipping {fn.name} in {fn.cppName}\n")
        return true
    if fn.ftype |> find("...") != -1
        print("warning: variadic function {fn.name} aka {fn.cppName}\n")
        return true
    for fa in fn.args
        if fa.atype |> isBlockedType
            return true
    if blocked_functions_table |> key_exists(fn.cppName)
        return true
    if is_function_blocked!=null
        if invoke(is_function_blocked, fn.cppName)
            return true
    return false

def qType2FnType(st)
    let ob = find(st,"(")
    assert(ob!=-1)
    return slice(st,0,ob) + "(*)" + slice(st,ob)

def qType2FnTypeClass(st,className)
    let ob = find(st,"(")
    assert(ob!=-1)
    return slice(st,0,ob) + "({className}::*)" + slice(st,ob)

def getFunRetType(fn)
    let ob = find(fn.ftype,"(")
    assert(ob!=-1)
    var cppResType = slice(fn.ftype,0,ob)
    while cppResType |> ends_with(" ")
        cppResType = slice(cppResType, 0, -1)
    return cppResType

def isRefFun(fn)
    let ob = find(fn.ftype,"(")
    assert(ob!=-1)
    var cppResType = slice(fn.ftype,0,ob)
    if cppResType |> starts_with("const ")
        cppResType = slice(cppResType, 6)
    if cppResType |> ends_with(" ")
        cppResType = slice(cppResType, 0, -1)
    if cppResType |> ends_with("&")
        return true
    return false

def isCmresFun(fn)
    // note: hacking cpp type to autodetect CMRES
    let ob = find(fn.ftype,"(")
    assert(ob!=-1)
    var cppResType = slice(fn.ftype,0,ob)
    if cppResType |> starts_with("const ")
        cppResType = slice(cppResType, 6)
    if cppResType |> ends_with(" ")
        cppResType = slice(cppResType, 0, -1)
    for atype in keys(alias_types)
        if cppResType == atype
            return false
    if ast.structs |> key_exists(cppResType)
        return true
    if alt_struct_name!=null
        let aname = invoke(alt_struct_name, cppResType)
        if aname!=""
            if ast.structs |> key_exists(aname)
                return true
    for bf in cmres_functions
        if bf == fn.cppName
            return true
    return false

var g_method_name = 0

def methodName(name:string)
    g_method_name ++
    return "_{name}_method_{g_method_name}"

var g_split_count = 0
var g_split_factor = 20
var g_split_file : FILE const? = null

def openSplitFile
    if (g_split_count % g_split_factor) == 0
        closeSplitFile()
        let split_index = g_split_count / g_split_factor
        let fname = "{split_prefix}_{split_index}.cpp"
        print("SPLIT {split_index} at {fname}\n")
        g_split_file = fopen(fname, "wb")
        var prefix = replace(split_cpp_prefix,"$$$","{split_index}")
        fprint(g_split_file,prefix)
    g_split_count ++

def closeSplitFile
    if g_split_file != null
        fprint(g_split_file,split_cpp_suffix)
        fclose(g_split_file)
        g_split_file = null

def getFunctions(df,df_method:FILE const?)
    fprint(df,"// ------------\n// functions\n// ------------\n\n")
    var all_fn : table<string; int>
    for fn in values(ast.funcs)
        all_fn[fn.cppName] ++
    for fn in values(ast.funcs)
        if fn |> isBlockedFun
            continue
        let castbind = all_fn[fn.cppName] > 1
        genFunction(fn, castbind, df)
        if generate_split_functions
            openSplitFile()
            genFunction(fn, castbind, g_split_file)

    fprint(df_method,"// ------------\n// methods \n// ------------\n\n")
    for st in values(ast.structs)
        var all_met : table<string; int>
        for fn in st.methods
            all_met[fn.name] ++
        for fn in st.methods
            if fn |> isBlockedFun
                continue
            let castbind = all_met[fn.name] > 1
            genFunction(fn, castbind, df_method)
            if generate_split_functions
                openSplitFile()
                genFunction(fn, castbind, g_split_file)

var
    reg_args <- %regex~(.+)\s*\((.+)\)%%

def parseQualArgs ( qname:string; var rtype:string& )
    if regex_match(reg_args, qname)!=-1
        let qargs = regex_group(reg_args, 2, qname)
        rtype = regex_group(reg_args, 1, qname)
        return <-  split(qargs, ", ")
    return <- [[array<string>]]

def getDefinePrefix(xname:string)
    for nn,pn in keys(namespace_to_prefix),values(namespace_to_prefix)
        if xname |> starts_with(nn)
            return pn
    return "GLOBAL_NAMESPACE"

var gen_fn_callback : lambda<(fn:ClangFunc):void>

def genFunction(fn:ClangFunc; castbind:bool; df:FILE const?)
    var bind_enchantation = ""
    var cpp_enchantation = ""
    var using_mname = ""
    var using_mname_expr = ""
    if gen_fn_callback != null
        invoke(gen_fn_callback, fn)
    if fn.isMethod
        if castbind
            let fType = qType2FnTypeClass(fn.ftype,fn.ofClass)
            using_mname = methodName(fn.name)
            using_mname_expr = "das::das_call_member<{fType},&{fn.cppName}>"
            bind_enchantation = "DAS_CALL_METHOD({using_mname})"
            cpp_enchantation = "\"das_call_member<{fType},&{fn.cppName}>::invoke\""
            print("warning: {fn.cppName} using clang-ast binding of {fType}\n")
        else
            using_mname = methodName(fn.name)
            bind_enchantation = "DAS_CALL_METHOD({using_mname})"
            cpp_enchantation = "DAS_CALL_MEMBER_CPP({fn.cppName})"
    else
        if castbind
            let fType = qType2FnType(fn.ftype)
            bind_enchantation = "{fType},{fn.cppName}"
            cpp_enchantation = "\"{fn.cppName}\""
            print("warning: {fn.cppName} using clang-ast binding of {fType}\n")
        else
            bind_enchantation = "DAS_BIND_FUN({fn.cppName})"
            cpp_enchantation = "\"{fn.cppName}\""
    var extra_enchantation = ""
    if fn |> isRefFun
        extra_enchantation = ", SimNode_ExtFuncCallRef"
    elif fn |> isCmresFun
        extra_enchantation = ", SimNode_ExtFuncCallAndCopyOrMove"
    fprint(df,"#ifdef {getDefinePrefix(fn.cppName)}\n")
    if !empty(using_mname)
        if !empty(using_mname_expr)
            fprint(df,"using {using_mname} = {using_mname_expr};\n");
        else
            fprint(df,"using {using_mname} = DAS_CALL_MEMBER({fn.cppName});\n");
    fprint(df,"addExtern<{bind_enchantation}{extra_enchantation}>(*this, lib, \"{fn.name}\",SideEffects::worstDefault, {cpp_enchantation})\n")
    fprint(df,"\t->args(\{")
    if fn.isMethod
        fprint(df,"\"self\",")
    for arg,argi in fn.args,count()
        if argi!=0
            fprint(df,",")
        if empty(arg.name)
            fprint(df,"\"arg{fn.isMethod ? argi+1 : argi}\"")
        else
            fprint(df,"\"{arg.name}\"")
    if !extra_args |> empty
        if length(fn.args) > 0
            fprint(df,",")
        fprint(df,extra_args)
    fprint(df,"\})\n")
    var rtype : string
    var qargs <- parseQualArgs(fn.ftype, rtype)
    var revArgs = true
    if length(qargs) != length(fn.args)
        print("warning: args did not parse `{fn.ftype}` to {qargs}\n")
        delete qargs
        revArgs = false
    if revArgs
        get(rev_enums, rtype) <| $ ( rt )
            fprint(df,"\t\t->res_type(makeType<{rt}>(lib))\n")
    for arg,_argi in fn.args,count()
        let argi = fn.isMethod ? _argi+1 : _argi
        var thisArgEnum = ""
        if revArgs
            get(rev_enums, qargs[_argi]) <| $ ( en )
                thisArgEnum = en
                fprint(df,"\t\t->arg_type({argi},makeType<{thisArgEnum}>(lib))\n")
        if !empty(arg.value)
            if arg.value=="nullptr"
                if arg.atype=="const char *"
                    fprint(df,"\t\t->arg_init({argi},make_smart<ExprConstString>(\"\"))\n")
                else
                    fprint(df,"\t\t->arg_init({argi},make_smart<ExprConstPtr>())\n")
            elif arg.vtype=="int"
                if empty(thisArgEnum)
                    fprint(df,"\t\t->arg_init({argi},make_smart<ExprConstInt>({arg.value}))\n")
                else
                    fprint(df,"\t\t->arg_init({argi},make_smart<ExprConstEnumeration>({arg.value},makeType<{thisArgEnum}>(lib)))\n")
            elif arg.vtype=="float"
                fprint(df,"\t\t->arg_init({argi},make_smart<ExprConstFloat>({arg.value}))\n")
            elif arg.vtype=="bool"
                fprint(df,"\t\t->arg_init({argi},make_smart<ExprConstBool>({arg.value}))\n")
            elif arg.vtype=="const char *"
                fprint(df,"\t\t->arg_init({argi},make_smart<ExprConstString>({arg.value}))\n")
            else
                fprint(df,"\t\t// {arg.name} : {arg.atype} = {arg.value}\n")
                print("warning: unsupported init {arg.atype} of {arg.vtype}\n")
    fprint(df,";\n")
    fprint(df,"#endif\n")

def genBindings(hf,df,mf_enum,df_enum,mf_ann,df_ann,df_method:FILE const?)
    genEnums(hf,mf_enum,df_enum)
    genStructs(hf,mf_ann,df_ann)
    getFunctions(df,df_method)
    closeSplitFile()

def bindHeaders ( fname, outprefix:string )
    delete blocked_functions_table
    for fn in blocked_functions
        blocked_functions_table[fn] = true
    for enk,env in keys(ex_enums),values(ex_enums)
        rev_enums[env.name] = enk
    for kwd in all_keywords
        ex_keywords[kwd] = true
    for be in blocked_enumerations
        blocked_enumerations_table[be] = true
    fopen(fname, "rb") <| $ ( f )
        if f==null
            print("can't open {fname}")
        else
            fread(f) <| $ ( data )
                print ( "\n{fname}\n" )
                var error = ""
                var json = read_json(data, error)
                if json == null
                    panic("failed to parse, {error}\n")
                else
                    assert(json is _object)
                    var root & = unsafe(json as _object)
                    verify( root["kind"] |> is_str <| "TranslationUnitDecl" )
                    onInner(root)
                    // debug(ast,"AST =")
                    fopen("{outprefix}.h","wb") <| $(hf)
                        fwrite(hf,"#pragma once\n\n")
                        fopen("{outprefix}.inc","wb") <| $(df)
                            fopen("{outprefix}.enum.cpp_inc","wb") <| $(mf_enum)
                                fopen("{outprefix}.enum.inc","wb") <| $(df_enum)
                                    fopen("{outprefix}.ann.cpp_inc","wb") <| $(mf_ann)
                                        fopen("{outprefix}.ann.inc","wb") <| $(df_ann)
                                            fopen("{outprefix}.method.inc","wb") <| $(df_method)
                                                genBindings(hf,df,mf_enum,df_enum,mf_ann,df_ann,df_method)


def genDefineConstants(fname,prefix:string)
    var fnames <- [{ auto[] fname }]
    defer <| { delete fnames; }
    fnames |> genDefineConstants <| prefix

def getGenConstType ( name, default_name:string )
    return const_type_table?[name] ?? default_name

def searchAndGenConst ( var regex_def:Regex; defTName,suffix:string; var ofs:table<int; bool>; data:string; var dup:table<string;bool>; hf:FILE const? )
    regex_foreach(regex_def,data) <| $ ( r )
        if ! ofs |> key_exists(r.x)
            let DEF = regex_group(regex_def,1,data)
            let VAL = regex_group(regex_def,2,data)
            if !blocked_defines_table |> key_exists(DEF)
                if is_const_blocked==null || !invoke(is_const_blocked,DEF)
                    let ctt = getGenConstType(DEF,defTName)
                    if !dup[DEF]
                        fprint(hf,"addConstant<{ctt}>(*this,\"{DEF}\",{ctt}({VAL}{suffix}));\n")
                        dup[DEF] = true
                    else
                        print("const: duplicate {DEF}\n")
        return true

def genDefineConstants(fnames:array<string>; prefix:string)
    delete blocked_defines_table
    for bd in blocked_defines
        blocked_defines_table[bd] = true
    var reg_def_hex <- %regex~#define\s+(\w+)\s+(0x[0-9A-Za-z]+)%%
    var reg_def_dec <- %regex~#define\s+(\w+)\s+(\d+)%%
    var reg_def_UINT8 <- %regex~#define\s+(\w+)\s+UINT8_C\s*\((0x[0-9A-Za-z]+)\)%%
    var reg_def_UINT16 <- %regex~#define\s+(\w+)\s+UINT16_C\s*\((0x[0-9A-Za-z]+)\)%%
    var reg_def_UINT32 <- %regex~#define\s+(\w+)\s+UINT32_C\s*\((0x[0-9A-Za-z]+)\)%%
    var reg_def_UINT64 <- %regex~#define\s+(\w+)\s+UINT64_C\s*\((0x[0-9A-Za-z]+)\)%%
    fopen("{prefix}.const_inc","wb") <| $(hf)
        if hf==null
            panic("can't write {prefix}.const_inc")
        for fname in fnames
            fopen(fname,"rb") <| $( f )
                if f == null
                    panic("can't open {fname}")
                var ofs : table<int; bool>
                let data = fread(f)
                var dup : table<string; bool>
                searchAndGenConst(reg_def_hex, "uint32_t", "u", ofs, data, dup, hf)
                searchAndGenConst(reg_def_dec, "int32_t", "", ofs, data, dup, hf)
                searchAndGenConst(reg_def_UINT8, const_uint8_type, "u", ofs, data, dup, hf)
                searchAndGenConst(reg_def_UINT16, const_uint16_type, "u", ofs, data, dup, hf)
                searchAndGenConst(reg_def_UINT32, "uint32_t", "u", ofs, data, dup, hf)
                searchAndGenConst(reg_def_UINT64, "uint64_t", "ul", ofs, data, dup, hf)
