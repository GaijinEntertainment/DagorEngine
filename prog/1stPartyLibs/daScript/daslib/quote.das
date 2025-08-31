options gen2

options indenting = 4
options no_unused_block_arguments = false
options no_unused_function_arguments = false
options strict_smart_pointers = true

module quote shared private

require daslib/ast_boost public
require daslib/strings_boost
require daslib/contracts

def find_unique_function_ptr(mod : Module?; name : string; canfail : bool = false) {
    var inscope fn = find_unique_function(mod, name, canfail);
    return <- fn.get_ptr()
}

/*
 * We need this wrappers for initialization of Handle types
 */
struct CaptureEntryInitData {
    name : string
    mode : CaptureMode
}

def public clone(var a : dasvector`CaptureEntry; b : array<CaptureEntryInitData>) {
    a |> resize(length(b))
    for (e, i in b, count()) {
        a[i].name := e.name
        a[i].mode = e.mode
    }
}

struct LineInfoInitData {
    fileInfo : FileInfo?
    column : uint
    line : uint
    last_column : uint
    last_line : uint
}


def public clone_line_info(var b : LineInfoInitData) : LineInfo {
    return LineInfo(fileInfo = b.fileInfo,
                    column = b.column,
                    line = b.line,
                    last_column = b.last_column,
                    last_line = b.last_line)
}

def public clone(var a : dasvector`LineInfo; b : array<LineInfoInitData>) {
    a |> resize(length(b))
    for (e, i in b, count()) {
        a[i].column := e.column
        a[i].line = e.line
        a[i].last_column := e.last_column
        a[i].last_line = e.last_line
    }
}

def public clone(var args : rtti::AnnotationList; var nargs : array<smart_ptr<rtti::AnnotationDeclaration>>) {
    _::clear(args)
    for (narg in nargs) {
        _::emplace(args, narg)
    }
    delete nargs
}


struct EnumEntryInitData {
    name : string
    cppName : string
    // at : LineInfo
    // value : ExpressionPtr
}

def public clone(var a : dasvector`EnumEntry; var b : array<EnumEntryInitData>) {
    a |> resize(length(b))
    for (e, i in b, count()) {
        a[i].name := e.name
        a[i].cppName := e.cppName
        // move_new(a[i].value) <| e.value
    }
}

struct AnnotationArgumentInitData {
    basicType : Type
    name : string
    sValue : string
    bValue : bool
    iValue : int
    fValue : float
    // @safe_when_uninitialized at: LineInfo // todo: add if needed
}

def public clone(var a : AnnotationArgumentList; b : array<AnnotationArgumentInitData>) {
    a |> resize(length(b))
    for (e, i in b, count()) {
        a[i].name := e.name
        if (e.basicType == Type.tString) {
            a[i].sValue := e.sValue
        } elif (e.basicType == Type.tBool) {
            a[i].bValue := e.bValue
        } elif (e.basicType == Type.tInt) {
            a[i].iValue := e.iValue
        } elif (e.basicType == Type.tFloat) {
            a[i].fValue := e.fValue
        }
        // a[i].at := e.at
        a[i].basicType = e.basicType
    }
}

def public cvt_to_mks(var args) : smart_ptr<MakeStruct> {
    var inscope res <- new MakeStruct(uninitialized);
    *res |> resize(length(args))
    for (arg, i in args, count()) {
        if (true) {
            move_new((*res)[i]) <| arg;
        }
    }
    return <- res;
}

struct FileInfoInitData {
    name : string
    tabSize : int
}

def public clone_file_info(b : FileInfoInitData) : FileInfo? {
    return clone_file_info(b.name, b.tabSize)
}

let blacklist = [
    ("ast::Function", "inferStack"),
    ("ast::Function", "annotations"),
    ("ast::Function", "classParent"), // loop
    ("ast::ExprReturn", "_block"), // loop
    ("ast::ExprVar", "pBlock"), // loop
    ("ast::EnumEntry", "value"), // loop
    ("ast::EnumEntry", "at"), // ignore it for now
    ("rtti::AnnotationArgument", "at"),
    ("ast::CaptureEntry", "at"), // ignore it for now
    ("ast::MakeFieldDecl", "at"), // ignore it for now
    ("ast::Enumeration", "list"), // too huge in TypeDecl
]

let managed_types = [
    "rtti::Module",
    "rtti::Annotation",
    "rtti::TypeAnnotation",
    "ast::Structure",
    "ast::CallMacro",
    "ast::Function",
    "rtti::FileInfo",
    "ast::MakeStruct",
    "rtti::LineInfo",
];

def private get_dst_type(var _src_tp : TypeDeclPtr) {
    var inscope src_tp <- _src_tp;
    let tstr = describe(src_tp);
    if (tstr |> starts_with("$::das_string")) {
        return <- new TypeDecl(baseType = Type.tString);
    } elif (tstr |> starts_with("ast::CaptureEntry")) {
        return <- typeinfo ast_typedecl(type<CaptureEntryInitData>);
    } elif (tstr |> starts_with("ast::EnumEntry")) {
        return <- typeinfo ast_typedecl(type<EnumEntryInitData>);
    } elif (tstr |> starts_with("rtti::LineInfo")) {
        return <- typeinfo ast_typedecl(type<LineInfoInitData>);
    } elif (tstr |> starts_with("rtti::AnnotationArgument")) {
        return <- typeinfo ast_typedecl(type<AnnotationArgumentInitData>);
    } elif (tstr |> starts_with("rtti::FileInfo")) {
        return <- typeinfo ast_typedecl(type<FileInfoInitData>);
    } else {
        return <- src_tp;
    }
}


def private create_string_expr(name) {
    return <- new ExprConstString(value := name);
}


def private to_array_move_expr(var mkArr : smart_ptr<ExprMakeArray>, at : LineInfo) {
    var inscope mkToArrayMove <- new ExprCall(at = at, name := "to_array_move")
    emplace(mkToArrayMove.arguments, mkArr)
    return <- mkToArrayMove
}

def private to_arr_move_args(var args : array<ExpressionPtr>; var dst_tp : TypeDeclPtr; at : LineInfo) {
    assert(args |> length > 0);
    unsafe {
        return <- to_array_move_expr(new ExprMakeArray(values := args,
                                                       gen2 = true,
                                                       makeType <- clone_type(dst_tp),
                                                       at = at), at)
    }
}

def private convert_vec_expr(mod : Module?; field_values : uint8 const?; xtype : TypeDeclPtr, at : LineInfo) : ExpressionPtr {
    var inscope src_tp <- get_handled_type_index_type_declaration(xtype.annotation,
                             default<Expression?>, default<Expression?>);
    src_tp.flags &= ~TypeDeclFlags.ref; // vec type has ref, we don't need it
    var inscope dst_tp <- get_dst_type(clone(src_tp));
    var inscope args : array<ExpressionPtr>;

    if (get_vector_length(field_values, src_tp) == 0) {
        return <- new ExprMakeStruct(at = at, makeType <- new TypeDecl(baseType = Type.tArray, firstType <- clone_type(dst_tp)),
                                     makeStructFlags = ExprMakeStructFlags.canShadowBlock);
    }
    for (idx in range(get_vector_length(field_values, src_tp))) {
        if (true) {
            var inscope arg_expr <- convert_quote_expr(mod, unsafe(reinterpret<uint8?>(get_vector_ptr_at_index(field_values, src_tp.get_ptr(), idx))), src_tp, at);
            if (dst_tp.baseType != Type.tString && dst_tp != src_tp) {
                var inscope make_str <- unsafe(reinterpret<smart_ptr<ExprMakeStruct>>(arg_expr));
                move_new(make_str.makeType) <| clone_type(dst_tp);
                args |> emplace(make_str);
            } else {
                args |> emplace(arg_expr);
            }
        }
    }
    if (describe(xtype) == "ast::MakeStruct") {
        unsafe {
            return <- new ExprCall(at = at, name := "cvt_to_mks",
                                arguments := array<ExpressionPtr>(
                                   new ExprMakeArray(values := args,
                                      gen2 = true,
                                      makeType <- clone_type(dst_tp),
                                      at = at)
                                )
                            )
        }
    }
    return <- to_arr_move_args(args, dst_tp, at);
}

def private find_module_expr(name, at) {
     return <- new ExprCall(at = at, name := "get_module",
                            arguments := array<ExpressionPtr>(create_string_expr(name)))
}

def private get_find_in_module_expr(fn_name; module_name; obj_name; at) {
     return <- new ExprCall(at = at, name := fn_name,
         arguments := array<ExpressionPtr>(
            find_module_expr(module_name, at),
            create_string_expr(obj_name)
         )
     )
}

def private is_quote_field_allowed(f_name, class_name, data) {
    for ((cl, fld) in blacklist) {
        if (fld == f_name && (cl == "*" || class_name == cl ||
                             (class_name == "ast::Expression" &&
                              unsafe(reinterpret<Expression>(data)).__rtti == cl))) {
            return false;
        }
    }
    return true;

}

def private create_field_expr(name; var res_field; loc; flags) {
    return <- new MakeFieldDecl(name := name, value <- res_field, at := loc, flags = flags);
}

def private convert_quote_struct_field(mod : Module?, data : uint8 const?,
                                       tstr : string, name : string,
                                       offset : uint; xtype : TypeDeclPtr; parent_loc : LineInfo) : smart_ptr<MakeFieldDecl> {
    if (!is_quote_field_allowed(name, tstr, data)) {
        return <- null;
    }

    var fdata = unsafe(data + int(offset));
    var inscope res_field <- convert_quote_expr(mod, fdata, xtype, parent_loc);
    if (res_field == null) { return <- null; }

    var flags : MakeFieldDeclFlags;
    if (!xtype.canCopy) {
        flags = ((describe(xtype) == "$::das_string" || isYetAnotherVectorTemplate(xtype)) ?
                        MakeFieldDeclFlags.cloneSemantics : MakeFieldDeclFlags.moveSemantics);
    }
    if (describe(xtype) == "rtti::LineInfo") {
        var inscope prev <- res_field;
        move_new(res_field) <| new ExprCall(at = parent_loc, name := "clone_line_info",
                               arguments := array<ExpressionPtr>(prev))
    }

    return <- create_field_expr(name, clone(res_field), parent_loc, flags);
}

def private convert_quote_structure(mod : Module?; data : uint8 const?;
                                    tstr : string; info : TypeDeclPtr; at : LineInfo) : ExpressionPtr {
    assume ann = info.annotation;
    var inscope mkS1 <- new default<MakeStruct>;
    for_each_field(*unsafe(reinterpret<BasicStructureAnnotation?>(ann))) <| $(name, cppName, xtype, offset) {
        var inscope mkFld = convert_quote_struct_field(mod, data, tstr, name, offset, xtype, at);
        if (mkFld != null) {
            emplace(*mkS1, mkFld);
        }
    }
    var inscope mkStr <- new ExprMakeStruct(makeType <- get_dst_type(clone_type(info)), at = at)
    emplace(mkStr.structs, mkS1)
    mkStr.makeStructFlags |= ExprMakeStructFlags.useInitializer | ExprMakeStructFlags.canShadowBlock;
    if (describe(info) == "rtti::FileInfo") {
        return <- new ExprCall(at = at, name := "clone_file_info",
                               arguments := array<ExpressionPtr>(mkStr))
    }
    return <- mkStr;
}

def private convert_pointer_expr(mod; data; info; at) : ExpressionPtr {
    assume pdata = *unsafe(reinterpret<uint8 const ??> data);
    if (pdata == null) {
        return <- new ExprConstPtr(at = at);
    }
    assume first_tstr = describe(info.firstType);
    var inscope final_type <- clone(info.firstType);
    if (first_tstr == "ast::Expression") {
        assume annotation = mod |> module_find_type_annotation(unsafe(reinterpret<ExpressionPtr>(pdata)).__rtti);
        move_new(final_type) <| new TypeDecl(baseType = Type.tHandle, annotation = annotation)
    }
    var inscope subexpr <- convert_quote_expr(mod, pdata, final_type, at);

    // This types are created on cpp side by managed functions.
    // We already got pointer, no need to ascend.
    for (t in managed_types) {
        if (t == first_tstr) {
            return <- subexpr
        }
    }
    return <- new ExprAscend(subexpr <- subexpr, at = subexpr != null ? subexpr.at : at);
}

def private convert_quote_expr(mod : Module?; var data : uint8 const?; info : TypeDeclPtr; at : LineInfo) : ExpressionPtr {
    if (data == null) {
        return <- null;
    }
    if (info.baseType != Type.tHandle && info.baseType != Type.tPointer) {
        return <- walk_and_convert(data, info, at);
    }
    if (isYetAnotherVectorTemplate(info)) {
        return <- convert_vec_expr(mod, data, info, at);
    }
    let tstr = describe(info);

    // First check managed types
    if (tstr == "rtti::Annotation") {
        assume pdata = unsafe(reinterpret<rtti::Annotation>(data));
        return <- get_find_in_module_expr("module_find_annotation", pdata._module.name, pdata.name, at);
    } elif (tstr == "rtti::TypeAnnotation") {
         assume pdata = unsafe(reinterpret<rtti::TypeAnnotation>(data));
         return <- get_find_in_module_expr("module_find_type_annotation", pdata._module.name, pdata.name, at)
    } elif (tstr == "ast::CallMacro") {
         assume pdata = unsafe(reinterpret<CallMacro?>(data));
         return <- get_find_in_module_expr("find_call_macro", pdata._module.name, pdata.name, at)
    } elif (tstr == "ast::Structure") {
         assume pdata = unsafe(reinterpret<Structure?>(data));
         // disable support for Structure::Module. If we compile without shared_modules it's impossible
         if (pdata._module.name == "ast_boost") { // How to check COP.shared_modules = false?
            return <- null;
         }
         // It's unnecessary, however there is bug in aot with ternary operator
         var inscope tmp = get_find_in_module_expr("module_find_structure", pdata._module.name, pdata.name, at);
         return <- tmp;
    } elif (tstr == "ast::Function") {
         assume pdata = unsafe(reinterpret<Function?>(data));
         return <- get_find_in_module_expr("find_unique_function_ptr", pdata._module.name, pdata.name, at);
    } elif (tstr == "rtti::Module") {
         assume cur_mod = unsafe(reinterpret<Module?>(data));
         return <- find_module_expr(cur_mod.name, at);
    } elif (tstr == "$::das_string") {
        return <- create_string_expr(*unsafe(reinterpret<das_string?>(data)));
    }
    // Convert pointers
    if (info.baseType == Type.tPointer) {
        return <- convert_pointer_expr(mod, data, info, at);
    }
    // Convert structures
    if (info.annotation != null && info.annotation.isBasicStructureAnnotation) {
        var field_loc = at;
        // if possible use Expression.at
        if (isExpression(info)) {
            assume fval = unsafe(reinterpret<ExpressionPtr>(data));
            field_loc = fval.at;
        }
        return <- convert_quote_structure(mod, data, tstr, info, field_loc);
    }
    panic("Unexpected type {tstr}!");
    return <- null;
}

def convert_quote_to_expression(var arg_expr : ExpressionPtr; at : LineInfo) : ExpressionPtr {
    let mod = get_module("ast");
    assume annotation = mod |> module_find_type_annotation(arg_expr.__rtti);
    var inscope tp <- new TypeDecl(baseType = Type.tHandle, annotation = annotation);
    tp.flags |= TypeDeclFlags.smartPtr;
    return <- new ExprAscend(subexpr <- convert_quote_expr(mod, unsafe(reinterpret<uint8 const?>(arg_expr.get_ptr())), tp, at));
}

class QuoteConverter : AstVisitor {
    astChanged : bool = false
    def override visitExprQuote(var expr : smart_ptr<ExprQuote>) : ExpressionPtr {
        astChanged = true
        return <- convert_quote_to_expression(expr.arguments[0], expr.at)
    }
}

[infer_macro]
class QuotePass : AstPassMacro {
    def override apply(prog : ProgramPtr; mod : Module?) : bool {
        // Unwrapping ExprQuote is slow and inefficient, do it only if necessary.
        if (!compiling_program().policies.aot_macros) {
            return false // nothing to do
        }
        var astVisitor = new QuoteConverter()
        var inscope astVisitorAdapter <- make_visitor(*astVisitor)
        visit(prog, astVisitorAdapter)
        var result = astVisitor.astChanged
        unsafe {
            delete astVisitor
        }
        return result
    }
}
