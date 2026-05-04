options gen2
options indenting = 4
options no_unused_block_arguments = false
options no_unused_function_arguments = false
options strict_smart_pointers

module ast_boost shared public

//! AST macro authoring toolkit.
//!
//! Re-exports ``rtti`` and ``ast`` and provides the core annotations used by
//! macro authors: ``[macro]``, ``[reader_macro]``, ``[call_macro]``,
//! ``[function_macro]``, ``[block_macro]``, ``[variant_macro]``,
//! ``[tag_function]``, ``[tag_structure]``, ``[infer_macro]``,
//! ``[optimization_macro]``, ``[lint_macro]``, ``[global_lint_macro]``,
//! and ``[macro_function]``.

require daslib/rtti
require daslib/ast public
require strings
require daslib/strings_boost

//! Smart pointer type alias for AnnotationDeclaration.
typedef AnnotationDeclarationPtr = smart_ptr<AnnotationDeclaration>

def isVectorType(typ : Type) {
    //! Returns true if `typ` is a vector type (int/uint/float 2/3/4, range, urange).
    return (
        typ == Type.tInt2   || typ == Type.tInt3   || typ == Type.tInt4   ||
        typ == Type.tUInt2  || typ == Type.tUInt3  || typ == Type.tUInt4  ||
        typ == Type.tFloat2 || typ == Type.tFloat3 || typ == Type.tFloat4 ||
        typ == Type.tRange  || typ == Type.tURange || typ == Type.tRange64 ||
        typ == Type.tURange64
    )
}

def describe(list : AnnotationArgumentList) {
    //! Returns a comma-separated string description of an annotation argument list.
    return join([ for (arg in list); "{arg.name}={get_annotation_argument_value(arg)}" ], ",")
}

def describe(ann : AnnotationDeclaration) {
    //! Returns a string description of an annotation declaration including its arguments.
    if (ann.arguments |> length != 0) {
        return "{ann.annotation.name}({describe(ann.arguments)})"
    } else {
        return "{ann.annotation.name}"
    }
}

def describe(list : AnnotationList) {
    //! Returns a comma-separated string description of an annotation list.
    return join([ for (arg in list); describe(*arg) ], ",")
}

def describe(vvar : VariablePtr) {
    //! Returns a string description of a variable with its name and type.
    return vvar._type != null ? "{vvar.name}:{describe(vvar._type)}" : "{vvar.name}:null"
}

def describe_bitfield(bf; merger : string = "") {
    //! Returns a string listing the set bits of a bitfield value.
    static_if (typeinfo is_bitfield(bf)) {
        var inscope bftd <- typeinfo ast_typedecl(bf)
        return join([ for (name, bit in bftd.argNames, range(32)); "[{name}]";
            where(int(bf) & (1 << bit)) != 0 ], merger)
    } else {
        concept_assert(false, "requires bitfield")
    }
}

def describe_function_short(func : FunctionPtr | Function?) {
    //! Returns a short human-readable description of a function signature.
    var args <- [for (x in func.arguments); "{x.name}:{describe(x._type,false)}" ]
    let res = "{func.name} ({join(args,";")}) : {describe(func.result,false)}"
    delete args
    return res
}

def isExpression(t : TypeDeclPtr; top : bool = true) : bool {
    //! Returns true if the type declaration refers to an AST expression node type.
    if (t == null) {
        return false
    } elif (t.dim |> length != 0) {
        return false
    } elif (t.baseType == Type.tHandle) {
        if (t.annotation._module.name == "ast_core") {
            return starts_with("{t.annotation.name}", "Expr")
        } else {
            return false
        }
    } elif (t.baseType == Type.tPointer && top) {
        return isExpression(t.firstType, false)
    } else {
        return false
    }
}

def is_same_or_inherited(parent, child : Structure const?) {
    //! Returns true if `child` structure is the same as or inherits from `parent`.
    var ch = child
    while (ch != null) {
        if (parent == ch) {
            return true
        }
        ch = ch.parent
    }
    return false
}

def is_class_method(cinfo : StructurePtr; finfo : TypeDeclPtr) {
    //! Returns true if the function type declaration is a method of the given class structure.
    if (finfo.baseType != Type.tFunction) {
        return false
    }
    if (finfo.dim |> length != 0) {
        return false
    }
    if (finfo.argTypes |> length == 0) {
        return false
    }
    if (finfo.argTypes[0].baseType != Type.tStructure) {
        return false
    }
    if (finfo.argTypes[0].dim |> length != 0) {
        return false
    }
    if (!is_same_or_inherited(finfo.argTypes[0].structType, get_ptr(cinfo))) {
        return false
    }
    return true
}

def emplace_new(var vec : dasvector`smart_ptr`Expression; var ptr : smart_ptr<Expression>) {
    //! Emplaces a smart pointer to an Expression into a vector.
    emplace(vec, ptr)
}

def emplace_new(var vec : dasvector`smart_ptr`TypeDecl; var ptr : smart_ptr<TypeDecl>) {
    //! Emplaces a smart pointer to a TypeDecl into a vector.
    emplace(vec, ptr)
}

def emplace_new(var vec : dasvector`smart_ptr`Variable; var ptr : smart_ptr<Variable>) {
    //! Emplaces a smart pointer to a Variable into a vector.
    emplace(vec, ptr)
}

def emplace_new(var vec : MakeStruct; var ptr : smart_ptr<MakeFieldDecl>) {
    //! Emplaces a smart pointer to a MakeFieldDecl into a MakeStruct.
    emplace(vec, ptr)
}

def override_method(var str : StructurePtr; name, funcName : string) : bool {
    //! Overrides a structure field initializer with an address-of cast to the named function.
    for (fld in str.fields) {
        if (fld.name == name) {
            var inscope vcast <- new ExprCast(at = str.at,
                subexpr <- new ExprAddr(at = str.at, target := funcName),
                castType <- new TypeDecl(at = str.at, baseType = Type.autoinfer)
            )
            move(fld.init) <| vcast
            return true
        }
    }
    return false
}

class MacroMacro : AstFunctionAnnotation {
    //! Function annotation that guards the function body with an is_compiling_macros check.
    def override apply(var func : FunctionPtr; var group : ModuleGroup; args : AnnotationArgumentList; var errors : das_string) : bool {
        //! Wraps function body in an `is_compiling_macros` guard and marks program as needing macro module.
        compiling_program().flags |= ProgramFlags.needMacroModule
        func.flags |= FunctionFlags.macroInit
        var inscope blk <- new ExprBlock(at = func.at)
        var inscope ifm <- new ExprCall(at = func.at, name := "is_compiling_macros")
        var inscope ife <- new ExprIfThenElse(at = func.at, cond <- ifm, if_true <- func.body)
        emplace(blk.list, ife)
        move(func.body) <| blk
        return true
    }
}

class TagFunctionAnnotation : AstFunctionAnnotation {
    //! Function annotation that validates tag_function arguments are boolean tag names.
    def override apply(var func : FunctionPtr; var group : ModuleGroup; args : AnnotationArgumentList; var errors : das_string) : bool {
        //! Validates that tag_function annotation arguments are boolean tag names.
        for (arg in args) {
            let val = get_annotation_argument_value(arg)
            if (!(val is tBool)) {
                errors := "tag_function annotation expects only tag names"
                return false
            }
        }
        return true
    }
}

class TagStructureAnnotation : AstStructureAnnotation {
    //! Structure annotation that validates tag_structure arguments are boolean tag names.
    def override apply(var str : StructurePtr; var group : ModuleGroup; args : AnnotationArgumentList; var errors : das_string) : bool {
        //! Validates that tag_structure annotation arguments are boolean tag names.
        for (arg in args) {
            let val = get_annotation_argument_value(arg)
            if (!(val is tBool)) {
                errors := "tag_structure annotation expects only tag names"
                return false
            }
        }
        return true
    }
}

def private for_each_tag_function(mod : Module?; tag : string; blk : block<(var func : FunctionPtr) : void>) {
    for_each_function(mod, "") $(func) {
        for (ann in func.annotations) {
            if (ann != null && ann.annotation.name == "tag_function") {
                for (t in ann.arguments) {
                    if (t.name == tag) {
                        invoke(blk, func)
                    }
                }
            }
        }
    }
    for_each_generic(mod, "") $(func) {
        for (ann in func.annotations) {
            if (ann != null && ann.annotation.name == "tag_function") {
                for (t in ann.arguments) {
                    if (t.name == tag) {
                        invoke(blk, func)
                    }
                }
            }
        }
    }
}

[deprecated]
def find_arg(argn : string; args : AnnotationArgumentList) : RttiValue {
    //! Finds an annotation argument value by name. Deprecated in favor of find_arg(args, argn).
    for (a in args) {
        if (a.name == argn) {
            return get_annotation_argument_value(a)
        }
    }
    return RttiValue_nothing()
}

def find_arg(args : AnnotationArgumentList; argn : string) : RttiValue {
    //! Finds an annotation argument value by name in the argument list.
    for (a in args) {
        if (a.name == argn) {
            return get_annotation_argument_value(a)
        }
    }
    return RttiValue_nothing()
}

def private apply_tag_annotation(tag : string; var ann : FunctionAnnotationPtr) {
    var inscope funcs : array<FunctionPtr>
    for_each_tag_function(this_module(), tag) $(func) {
        funcs |> push_clone(func)
    }
    for (func in funcs) {
        {
            var inscope tagAnn := ann
            add_function_annotation(func, tagAnn)
        }
    }
}

def setup_tag_annotation(name, tag : string; var classPtr) {
    //! Creates and registers a function annotation associated with a tag.
    var inscope ann <- make_function_annotation(name, classPtr)
    apply_tag_annotation(tag, ann)
    this_module() |> add_function_annotation(ann)
}

def find_unique_function(mod : Module?; name : string; canfail : bool = false) {
    //! Finds a unique non-generic function by name in a module; panics if more than one found.
    var inscope res : FunctionPtr
    var count = 0
    for_each_function(mod, name) $(func) {
        res := func
        count ++
    }
    if (count > 1) {
        if (!canfail) {
            panic("more than one function encountered")
        }
        return <- default<FunctionPtr>
    }
    return <- res
}

def find_unique_generic(mod : Module?; name : string; canfail : bool = false) {
    //! Finds a unique generic function by name in a module; panics if more than one found.
    var inscope res : FunctionPtr
    var count = 0
    for_each_generic(mod, name) $(func) {
        res := func
        count ++
    }
    if (count > 1) {
        if (!canfail) {
            panic("more than one generic function encountered")
        }
        return <- default<FunctionPtr>
    }
    return <- res
}

def setup_call_list(name : string; at : LineInfo; subblock : block<(var fn : FunctionPtr) : void>) : ExprBlock? {
    //! Creates or retrieves a shared function for collecting call expressions, invoking subblock on initial setup.
    var inscope fn <- compiling_module() |> find_unique_function(name)
    if (fn == null) {
        move_new(fn) <| new Function(at = at, atDecl = at, name := "{name}",
            flags = FunctionFlags.generated,
            result <- new TypeDecl(baseType = Type.autoinfer, at = at)
        )
        var inscope blk <- new ExprBlock(at = at)
        var res = get_ptr(blk)
        move(fn.body) <| blk
        invoke(subblock, fn)
        if (!(compiling_module() |> add_function(fn))) {
            panic("failed to setup macro, can't add function {fn.name}")
        }
        return res
    } elif (fn.body.__rtti != "ExprBlock") {
        panic("expecting func.ExprBlock")
    }
    return unsafe(reinterpret<ExprBlock?> fn.body)
}

def setup_call_list(name : string; at : LineInfo; isInit : bool = false; isPrivate : bool = true; isLateInit : bool = false) : ExprBlock? {
    //! Creates or retrieves a shared init/private function for collecting call expressions.
    return setup_call_list(name, at) $(fn) {
        if (isInit) {
            fn.flags |= FunctionFlags.init
            if (isLateInit) {
                fn.moreFlags |= MoreFunctionFlags.lateInit
            }
        }
        if (isPrivate) {
            fn.flags |= FunctionFlags.privateFunction
        }
    }
}

def setup_macro(name : string; at : LineInfo) : ExprBlock? {
    //! Sets up a macro initialization function guarded by `is_compiling_macros_in_module`.
    compiling_program().flags |= ProgramFlags.needMacroModule
    var inscope fn <- compiling_module() |> find_unique_function(name)
    if (fn == null) {
        move_new(fn) <| new Function(at = at, atDecl = at, name := "{name}",
            flags = FunctionFlags.generated | FunctionFlags.macroInit | FunctionFlags.privateFunction,
            result <- new TypeDecl(baseType = Type.autoinfer, at = at)
        )
        var inscope blk <- new ExprBlock(at = at)
        var inscope iblk <- new ExprBlock(at = at)
        let res = get_ptr(iblk)
        var inscope ifm <- new ExprCall(at = at, name := "is_compiling_macros_in_module")
        var inscope ifmn <- new ExprConstString(at = at, value := compiling_module().name)
        emplace(ifm.arguments, ifmn)
        var inscope ife <- new ExprIfThenElse(at = at, cond <- ifm, if_true <- iblk)
        emplace(blk.list, ife)
        move(fn.body) <| blk
        if (!(compiling_module() |> add_function(fn))) {
            panic("failed to setup macro, can't add function {fn.name}")
        }
        return unsafe(reinterpret<ExprBlock?> res)
    } else {
        if (fn.body.__rtti != "ExprBlock") {
            panic("expecting func.ExprBlock")
        }
        unsafe {
            var blk = reinterpret<ExprBlock?> fn.body
            if (blk.list |> length != 1 || blk.list[0].__rtti != "ExprIfThenElse") {
                panic("expecting is_compiling_macros")
            }
            var ife = reinterpret<ExprIfThenElse?> blk.list[0]
            if (ife.if_true.__rtti != "ExprBlock") {
                panic("expecting ife.if_true.ExprBlock")
            }
            return reinterpret<ExprBlock?> ife.if_true
        }
    }
}

class SetupAnyAnnotation : AstStructureAnnotation {
    //! Base structure annotation for auto-registering macro classes via generated setup calls.
    annotation_function_call : string = ""
    name : string
    def setup_call(var st : StructurePtr; var cll : smart_ptr<ExprCall>) {
        //! Populates the annotation setup call with the structure name argument.
        emplace_new(cll.arguments, new ExprConstString(at = st.at, value := name))
    }
    def override apply(var st : StructurePtr; var group : ModuleGroup; args : AnnotationArgumentList; var errors : das_string) : bool {
        //! Registers the annotated structure as a macro by generating the setup call.
        let argName = args |> find_arg("name")
        if (argName is nothing) {
            name = "{st.name}"
        } elif (!(argName is tString)) {
            errors := "name must be string"
            return false
        } else {
            name = argName as tString
        }
        var blk <- setup_macro("__setup_macros", st.at)
        var inscope cll <- new ExprCall(at = st.at, name := annotation_function_call)
        self->setup_call(st, cll)
        var inscope nee <- new ExprNew(at = st.at, initializer = true,
            typeexpr <- new TypeDecl(baseType = Type.tStructure, structType = get_ptr(st), at = st.at)
        )
        emplace(cll.arguments, nee)
        emplace(blk.list, cll)
        return true
    }
}

class SetupFunctionAnnotation : SetupAnyAnnotation {
    //! Registers annotated structure as a function annotation macro.
    override annotation_function_call : string = "add_new_function_annotation"
}

class SetupBlockAnnotation : SetupAnyAnnotation {
    //! Registers annotated structure as a block annotation macro.
    override annotation_function_call : string = "add_new_block_annotation"
}

class SetupStructureAnnotation : SetupAnyAnnotation {
    //! Registers annotated structure as a structure annotation macro.
    override annotation_function_call : string = "add_new_structure_annotation"
}

class SetupEnumerationAnnotation : SetupAnyAnnotation {
    //! Registers annotated structure as an enumeration annotation macro.
    override annotation_function_call : string = "add_new_enumeration_annotation"
}

class SetupContractAnnotation : SetupAnyAnnotation {
    //! Registers annotated structure as a contract annotation macro.
    override annotation_function_call : string = "add_new_contract_annotation"
}

class SetupReaderMacro : SetupAnyAnnotation {
    //! Registers annotated structure as a reader macro.
    override annotation_function_call : string = "add_new_reader_macro"
}

class SetupCommentReader : SetupAnyAnnotation {
    //! Registers annotated structure as a comment reader macro.
    override annotation_function_call : string = "add_new_comment_reader"
}

class SetupVariantMacro : SetupAnyAnnotation {
    //! Registers annotated structure as a variant macro.
    override annotation_function_call : string = "add_new_variant_macro"
}

class SetupForLoopMacro : SetupAnyAnnotation {
    //! Registers annotated structure as a for-loop macro.
    override annotation_function_call : string = "add_new_for_loop_macro"
}

class SetupCaptureMacro : SetupAnyAnnotation {
    //! Registers annotated structure as a capture macro.
    override annotation_function_call : string = "add_new_capture_macro"
}

class SetupTypeMacro : SetupAnyAnnotation {
    //! Registers annotated structure as a type macro.
    override annotation_function_call : string = "add_new_type_macro"
}

class SetupSimulateMacro : SetupAnyAnnotation {
    //! Registers annotated structure as a simulate macro.
    override annotation_function_call : string = "add_new_simulate_macro"
}

class SetupCallMacro : SetupAnyAnnotation {
    //! Registers annotated structure as a call macro.
    override annotation_function_call : string = "add_new_call_macro"
}

class SetupTypeInfoMacro : SetupAnyAnnotation {
    //! Registers annotated structure as a typeinfo macro.
    override annotation_function_call : string = "add_new_typeinfo_macro"
}

class SetupInferMacro : SetupAnyAnnotation {
    //! Registers annotated structure as an infer macro.
    override annotation_function_call : string = "add_new_infer_macro"
}

class SetupDirtyInferMacro : SetupAnyAnnotation {
    //! Registers annotated structure as a dirty infer macro.
    override annotation_function_call : string = "add_new_dirty_infer_macro"
}

class SetupLintMacro : SetupAnyAnnotation {
    //! Registers annotated structure as a lint macro.
    override annotation_function_call : string = "add_new_lint_macro"
}

class SetupGlobalLintMacro : SetupAnyAnnotation {
    //! Registers annotated structure as a global lint macro.
    override annotation_function_call : string = "add_new_global_lint_macro"
}

class SetupOptimizationMacro : SetupAnyAnnotation {
    //! Registers annotated structure as an optimization macro.
    override annotation_function_call : string = "add_new_optimization_macro"
}

class TagFunctionMacro : SetupAnyAnnotation {
    //! Setup annotation that registers a tag-based function annotation with name and tag arguments.
    override annotation_function_call : string = "setup_tag_annotation"
    tag : string
    def override setup_call(var st : StructurePtr; var cll : smart_ptr<ExprCall>) {
        //! Populates the annotation setup call with both name and tag arguments.
        emplace_new(cll.arguments, new ExprConstString(at = st.at, value := name))
        emplace_new(cll.arguments, new ExprConstString(at = st.at, value := tag))
    }
    def override apply(var st : StructurePtr; var group : ModuleGroup; args : AnnotationArgumentList; var errors : das_string) : bool {
        //! Validates the tag argument and registers the structure as a tag function macro.
        let argTag = args |> find_arg("tag")
        if (argTag is nothing) {
            errors := "missing tag argument"
            return false
        } elif (!(argTag is tString)) {
            errors := "tag must be string"
            return false
        }
        tag = argTag as tString
        return SetupAnyAnnotation`apply(cast<SetupAnyAnnotation> self, st, group, args, errors)
    }
}

def public panic_expr_as : void ? {
    //! Panics with an error when an `as` variant expression fails or encounters a null pointer.
    panic("invalid 'as' expression or null pointer dereference")
    return null
}

def private makeAsOrSafeAs(expr; var enull : ExpressionPtr) {
    var inscope vdr <- new ExprField(at = expr.at, name := "__rtti", value <- clone_expression(expr.value))
    var inscope cna <- new ExprConstString(at = expr.at, value := expr.name)
    var inscope cond <- new ExprOp2(at = expr.at, op := "==", left <- vdr, right <- cna)
    var inscope ctype <- new TypeDecl(at = expr.at,
        baseType = Type.tPointer,
        firstType <- new TypeDecl(at = expr.at, baseType = Type.alias, alias := expr.name)
    )
    var inscope esub <- clone_expression(expr.value)
    if (expr.value._type.baseType == Type.tPointer) {
        var inscope neqz <- new ExprOp2(at = expr.at, op := "!=",
            left <- clone_expression(expr.value),
            right <- new ExprConstPtr(at = expr.at)
        )
        var inscope nand <- new ExprOp2(at = expr.at, op := "&&",
            left <- neqz,
            right <- cond
        )
        move(cond) <| nand
    } else {
        var inscope nsub <- new ExprRef2Ptr(at = expr.at,
            subexpr <- esub,
            genFlags = ExprGenFlags.alwaysSafe
        )
        move(esub) <| nsub
    }
    var inscope vcast <- new ExprCast(at = expr.at,
        subexpr <- esub,
        castType <- ctype,
        castFlags = ExprCastFlags.reinterpretCast,
        genFlags = ExprGenFlags.alwaysSafe
    )
    var inscope tri <- new ExprOp3(at = expr.at, op := "?",
        subexpr <- cond,
        left <- vcast,
        right <- enull
    )
    tri.genFlags = expr.genFlags
    return <- tri
}

// replacing ExprIsVariant(value,name) => ExprOp2('==",value.__rtti,"name")
// if value is ast_core::Expr*
class BetterRttiVisitor : AstVariantMacro {
    //! Variant macro that optimizes `is` checks on AST expression types to __rtti string comparison.
    def override visitExprIsVariant(prog : ProgramPtr; mod : Module?; expr : smart_ptr<ExprIsVariant>) : ExpressionPtr {
        //! Replaces `is` variant checks on AST expression types with __rtti string comparison.
        if (isExpression(expr.value._type)) {
            var inscope vdr <- new ExprField(at = expr.at, name := "__rtti", value <- clone_expression(expr.value))
            var inscope cna <- new ExprConstString(at = expr.at, value := expr.name)
            var inscope veq <- new ExprOp2(at = expr.at, op := "==", left <- vdr, right <- cna)
            return <- veq
        }
        return <- default<ExpressionPtr>
    }
    def override visitExprAsVariant(prog : ProgramPtr; mod : Module?; expr : smart_ptr<ExprAsVariant>) : ExpressionPtr {
        //! Replaces `as` variant casts on AST expression types with __rtti-guarded reinterpret casts.
        if (isExpression(expr.value._type)) {
            return <- makeAsOrSafeAs(expr, new ExprCall(at = expr.at, name := "panic_expr_as"))
        }
        return <- default<ExpressionPtr>
    }
    def override visitExprSafeAsVariant(prog : ProgramPtr; mod : Module?; expr : smart_ptr<ExprSafeAsVariant>) : ExpressionPtr {
        //! Replaces safe `as` variant casts on AST expression types with __rtti-guarded casts returning null on mismatch.
        if (isExpression(expr.value._type)) {
            return <- makeAsOrSafeAs(expr, new ExprConstPtr(at = expr.at))
        }
        return <- default<ExpressionPtr>
    }
}

def operator is BuiltInFunction(anything) {
    //! Fallback overload; returns false for any non-Function type.
    return false
}

def operator is BuiltInFunction(foo : Function?) {
    //! Returns true if the function is a built-in function.
    return foo.flags.builtIn
}

def operator as BuiltInFunction(foo : Function?) {
    //! Casts a Function pointer to BuiltInFunction pointer after verifying it is built-in.
    verify(foo is BuiltInFunction)
    return unsafe(reinterpret<BuiltInFunction?>(foo));
}

def operator is ExternalFnBase(anything) {
    //! Fallback overload; returns false for any non-Function type.
    return false
}

def operator is ExternalFnBase(foo : Function?) {
    //! Returns true if the function is a built-in property function.
    return foo.moreFlags.propertyFunction && foo.flags.builtIn
}

def operator as ExternalFnBase(foo : Function?) {
    //! Casts a Function pointer to ExternalFnBase pointer after verification.
    verify(foo is ExternalFnBase)
    return unsafe(reinterpret<ExternalFnBase?>(foo));
}

def operator is FunctionAnnotation(anything) {
    //! Fallback overload; returns false for any non-Annotation type.
    return false
}

def operator is FunctionAnnotation(foo : Annotation?) {
    //! Returns true if the annotation is a function annotation.
    return foo.isFunctionAnnotation
}

def operator is FunctionAnnotation(foo : smart_ptr<Annotation>) {
    //! Returns true if the smart_ptr annotation is a function annotation.
    return foo.isFunctionAnnotation
}

def operator as FunctionAnnotation(foo : Annotation?) {
    //! Casts an Annotation pointer to FunctionAnnotation pointer after verification.
    verify(foo is FunctionAnnotation)
    return unsafe(reinterpret<FunctionAnnotation?>(foo));
}

def operator as FunctionAnnotation(foo : smart_ptr<Annotation>) {
    //! Casts a smart_ptr Annotation to FunctionAnnotation pointer after verification.
    verify(foo is FunctionAnnotation)
    return unsafe(reinterpret<FunctionAnnotation?>(foo.get_ptr()));
}


def operator is StructureAnnotation(anything) {
    //! Fallback overload; returns false for any non-Annotation type.
    return false
}

def operator is StructureAnnotation(foo : Annotation?) {
    //! Returns true if the annotation is a structure annotation.
    return foo.isStructureAnnotation
}

def operator is StructureAnnotation(foo : smart_ptr<Annotation>) {
    //! Returns true if the smart_ptr annotation is a structure annotation.
    return foo.isStructureAnnotation
}

def operator as StructureAnnotation(foo : Annotation?) {
    //! Casts an Annotation pointer to StructureAnnotation pointer after verification.
    verify(foo is StructureAnnotation)
    return unsafe(reinterpret<StructureAnnotation?>(foo));
}

def operator as StructureAnnotation(foo : smart_ptr<Annotation>) {
    //! Casts a smart_ptr Annotation to StructureAnnotation pointer after verification.
    verify(foo is StructureAnnotation)
    return unsafe(reinterpret<StructureAnnotation?>(foo.get_ptr()));
}

def private walk_and_convert_array(data : uint8 const?; info : TypeDeclPtr; at : LineInfo) : ExpressionPtr {
    let total = any_array_size(data)
    if (total != 0) {
        let stride = info.firstType.sizeOf
        var inscope mkArr <- new ExprMakeArray(at = at, makeType <- clone_type(info.firstType))
        any_array_foreach(data, stride) $(value) {
            emplace_new(mkArr.values) <| walk_and_convert(value, info.firstType, at)
        }
        push(mkArr.makeType.dim, total)
        var inscope mkToArrayMove <- new ExprCall(at = at, name := "to_array_move")
        emplace(mkToArrayMove.arguments, mkArr)
        return <- mkToArrayMove
    } else {
        return <- new ExprMakeStruct(at = at, makeType <- clone_type(info))
    }
}

def private walk_and_convert_dim(data : uint8 const?; info : TypeDeclPtr; at : LineInfo) : ExpressionPtr {
    let stride = info.baseSizeOf
    let total = info.countOf
    var inscope einfo <- clone_type(info)
    clear(einfo.dim)
    var inscope mkArr <- new ExprMakeArray(at = at, makeType := info)
    for (x in range(total)) {
        unsafe {
            var elem <- walk_and_convert(data + stride * x, einfo, at)
            emplace(mkArr.values, elem)
        }
    }
    return <- mkArr
}

def private walk_and_convert_pointer(data : uint8 const?; info : TypeDeclPtr; at : LineInfo) : ExpressionPtr {
    unsafe {
        let pdata = *(reinterpret<uint8 const ??> data)
        var elem <- walk_and_convert(pdata, info.firstType, at)
        var mkAsc <- new ExprAscend(at = at, subexpr <- elem)
        return mkAsc
    }
}

def private walk_and_convert_tuple(data : uint8 const?; info : TypeDeclPtr; at : LineInfo) : ExpressionPtr {
    var inscope mkArr <- new ExprMakeTuple(at = at, recordType <- clone_type(info))
    for (idx in range(info.argTypes |> length)) {
        let offset = get_tuple_field_offset(info, idx)
        unsafe {
            var elem <- walk_and_convert(data + offset, info.argTypes[idx], at)
            emplace(mkArr.values, elem)
        }
    }
    return <- mkArr
}

def private walk_and_convert_structure(data : uint8 const?; info : TypeDeclPtr; at : LineInfo) : ExpressionPtr {
    var inscope mkStruct <- new ExprMakeStruct(at = at, makeType <- clone_type(info))
    let stype = info.structType
    var inscope mkS <- new MakeStruct(uninitialized)
    for (x in stype.fields) {
        if (x.annotation |> find_arg("do_not_convert") is nothing) {
            unsafe {
                let fdata = data + x.offset
                var elem <- walk_and_convert(fdata, x._type, at)
                var mkF <- new MakeFieldDecl(at = at, name := x.name, value <- elem)
                if (!x._type.canCopy) {
                    mkF.flags = MakeFieldDeclFlags.moveSemantics
                }
                emplace_new(*mkS, mkF)
            }
        }
    }
    emplace(mkStruct.structs, mkS)
    return <- mkStruct
}

def private walk_and_convert_variant(data : uint8 const?; info : TypeDeclPtr; at : LineInfo) : ExpressionPtr {
    unsafe {
        let vindex = *reinterpret<int?> data
        let offset = get_variant_field_offset(info, vindex)
        var mkVariant <- new ExprMakeVariant(at = at, makeType <- clone_type(info))
        var elem <- walk_and_convert(data + offset, info.argTypes[vindex], at)
        var mkS <- new MakeFieldDecl(at = at, name := info.argNames[vindex], value <- elem)
        if (!info.argTypes[vindex].canCopy) {
            mkS.flags = MakeFieldDeclFlags.moveSemantics
        }
        emplace(mkVariant.variants, mkS)
        return mkVariant
    }
}

def private walk_and_convert_table(data : uint8 const?; info : TypeDeclPtr; at : LineInfo) : ExpressionPtr {
    let total = any_table_size(data)
    if (total != 0) {
        var inscope tupT <- new TypeDecl(baseType = Type.tTuple, at = at)
        emplace_new(tupT.argTypes, clone_type(info.firstType))
        emplace_new(tupT.argTypes, clone_type(info.secondType))
        var inscope mkArr <- new ExprMakeArray(at = at, makeType <- clone_type(tupT))
        let key_stride = info.firstType.sizeOf
        let value_stride = info.secondType.sizeOf
        any_table_foreach(data, key_stride, value_stride) $(pkey, pvalue) {
            var inscope key <- walk_and_convert(pkey, info.firstType, at)
            var inscope value <- walk_and_convert(pvalue, info.secondType, at)
            var inscope mkTup <- new ExprMakeTuple(at = at, recordType <- clone_type(tupT), isKeyValue = true)
            emplace(mkTup.values, key)
            emplace(mkTup.values, value)
            emplace(mkArr.values, mkTup)
        }
        push(mkArr.makeType.dim, total)
        var inscope mkToTableMove <- new ExprCall(at = at, name := "to_table_move")
        emplace(mkToTableMove.arguments, mkArr)
        return <- mkToTableMove
    } else {
        return <- new ExprMakeStruct(at = at, makeType <- clone_type(info))
    }
}

def private walk_and_convert_basic(data : uint8 const?; info : TypeDeclPtr; at : LineInfo) : ExpressionPtr {
    unsafe {
        if (info.baseType == Type.tInt8) {
            return new ExprConstInt8(at = at, value = *reinterpret<int8?> data)
        } elif (info.baseType == Type.tInt16) {
            return new ExprConstInt16(at = at, value = *reinterpret<int16?> data)
        } elif (info.baseType == Type.tInt) {
            return new ExprConstInt(at = at, value = *reinterpret<int?> data)
        } elif (info.baseType == Type.tInt64) {
            return new ExprConstInt64(at = at, value = *reinterpret<int64?> data)
        } elif (info.baseType == Type.tUInt8) {
            return new ExprConstUInt8(at = at, value = *reinterpret<uint8?> data)
        } elif (info.baseType == Type.tUInt16) {
            return new ExprConstUInt16(at = at, value = *reinterpret<uint16?> data)
        } elif (info.baseType == Type.tUInt) {
            return new ExprConstUInt(at = at, value = *reinterpret<uint?> data)
        }  elif (info.baseType == Type.tUInt64) {
            return new ExprConstUInt64(at = at, value = *reinterpret<uint64?> data)
        } elif (info.baseType == Type.tFloat) {
            return new ExprConstFloat(at = at, value = *reinterpret<float?> data)
        } elif (info.baseType == Type.tDouble) {
            return new ExprConstDouble(at = at, value = *reinterpret<double?> data)
        } elif (info.baseType == Type.tBool) {
            return new ExprConstBool(at = at, value = *reinterpret<bool?> data)
        } elif (info.baseType == Type.tRange) {
            return new ExprConstRange(at = at, value = *reinterpret<range?> data)
        } elif (info.baseType == Type.tURange) {
            return new ExprConstURange(at = at, value = *reinterpret<urange?> data)
        } elif (info.baseType == Type.tRange64) {
            return new ExprConstRange64(at = at, value = *reinterpret<range64?> data)
        } elif (info.baseType == Type.tURange64) {
            return new ExprConstURange64(at = at, value = *reinterpret<urange64?> data)
        } elif (info.baseType == Type.tString) {
            return new ExprConstString(at = at, value := *reinterpret<string?> data)
        } elif (info.baseType == Type.tBitfield8
                || info.baseType == Type.tBitfield16
                || info.baseType == Type.tBitfield
                || info.baseType == Type.tBitfield64) {
            return new ExprConstBitfield(at = at, value = *reinterpret<bitfield?> data, bitfieldType := info)
        } else {
            panic("unsupported {info.baseType}")
            return default<ExpressionPtr>
        }
    }
}

def private walk_and_convert_enumeration(data : uint8 const?; info : TypeDeclPtr; at : LineInfo) : ExpressionPtr {
    var eval = 0l
    if (info.baseType == Type.tEnumeration) {
        eval = int64(*unsafe(reinterpret<int?> data))
    } elif (info.baseType == Type.tEnumeration8) {
        eval = int64(*data)
    } elif (info.baseType == Type.tEnumeration16) {
        eval = int64(*unsafe(reinterpret<int16?> data))
    } elif (info.baseType == Type.tEnumeration64) {
        eval = *unsafe(reinterpret<int64?> data)
    } else {
        panic("unsupported enumeration")
    }
    let name = info.enumType |> find_enum_name(eval)
    panic("can't find name of enum value {eval} in {describe(info)}") if (name == "")
    return <- new ExprConstEnumeration(
        enumType := unsafe(reinterpret<Enumeration?> info.enumType),
        value := name)
}

def walk_and_convert(data : uint8 const?; info : TypeDeclPtr; at : LineInfo) : ExpressionPtr {
    //! Recursively converts raw data to an AST expression based on type information.
    // print("0x{intptr(data)} {describe(info)}\n")
    if (info.dim |> length != 0) {
        return <- walk_and_convert_dim(data, info, at)
    } elif (info.baseType == Type.tArray) {
        return <- walk_and_convert_array(data, info, at)
    } elif (info.baseType == Type.tPointer) {
        unsafe {
            if (*reinterpret<uint8 const ??> data == null) {
                return new ExprConstPtr(at = at)
            } elif (info.firstType == null || info.firstType.isVoid) {
                return new ExprConstPtr(at = at)
            } else {
                return walk_and_convert_pointer(data, info, at)
            }
        }
    } elif (info.baseType == Type.tStructure) {
        return <- walk_and_convert_structure(data, info, at)
    } elif (info.baseType == Type.tVariant) {
        return <- walk_and_convert_variant(data, info, at)
    } elif (info.baseType == Type.tTuple) {
        return <- walk_and_convert_tuple(data, info, at)
    } elif (info.baseType == Type.tTable) {
        return <- walk_and_convert_table(data, info, at)
    } elif (info.baseType == Type.tEnumeration8 || info.baseType == Type.tEnumeration16 || info.baseType == Type.tEnumeration64 || info.baseType == Type.tEnumeration) {
        return <- walk_and_convert_enumeration(data, info, at)
    } else {
        return <- walk_and_convert_basic(data, info, at)
    }
}

def convert_to_expression(var value : auto& ==const; at : LineInfo) {
    //! Converts a mutable reference value to an AST expression at the given source location.
    var inscope info <- typeinfo ast_typedecl(value)
    unsafe {
        let pval = addr(value)
        return walk_and_convert(reinterpret<uint8 const?> pval, info, at)
    }
}

def convert_to_expression(value : auto ==const; at : LineInfo) {
    //! Converts a constant value to an AST expression at the given source location.
    var inscope info <- typeinfo ast_typedecl(value)
    unsafe {
        let pval = addr(value)
        return walk_and_convert(reinterpret<uint8 const?> pval, info, at)
    }
}

def convert_to_expression(var value : auto ==const) {
    //! Converts a mutable value to an AST expression with default source location.
    return <- convert_to_expression(value, LineInfo())
}

def convert_to_expression(value : auto ==const) {
    //! Converts a constant value to an AST expression with default source location.
    return <- convert_to_expression(value, LineInfo())
}

def find_annotation(mod_name, ann_name : string) : Annotation const? {
    //! Finds an annotation by module name and annotation name in the compiling program.
    var mod = find_compiling_module(mod_name)
    if (mod == null) {
        return null
    }
    var ann : Annotation const?
    module_for_each_annotation(mod) $(value) {
        if (value.name == ann_name) {
            unsafe {
                ann = addr(value)
            }
        }
    }
    return ann
}

def append_annotation(mod_name, ann_name : string; args : array< tuple<argname : string; argvalue : RttiValue> >) {
    //! Creates an AnnotationDeclaration with the specified arguments from the given annotation.
    var ann = find_annotation(mod_name, ann_name)
    assert(ann != null)
    var inscope decl <- new AnnotationDeclaration(uninitialized)
    decl.annotation := unsafe(reinterpret<smart_ptr<Annotation>> ann)
    for (argP in args) {
        let argName & = argP.argname
        let arg & = argP.argvalue
        if (arg is tBool) {
            decl.arguments |> add_annotation_argument(argName, arg as tBool)
        } elif (arg is tInt) {
            decl.arguments |> add_annotation_argument(argName, arg as tInt)
        } elif (arg is tFloat) {
            decl.arguments |> add_annotation_argument(argName, arg as tFloat)
        } elif (arg is tString) {
            decl.arguments |> add_annotation_argument(argName, arg as tString)
        } else {
            panic("Invalid rtti value {arg} for annotation argument {argName}")
        }
    }
    return <- decl
}

def append_annotation(mod_name, ann_name : string) {
    //! Creates an AnnotationDeclaration from the given module and annotation name.
    var ann = find_annotation(mod_name, ann_name)
    assert(ann != null)
    var inscope decl <- new AnnotationDeclaration(uninitialized)
    decl.annotation := unsafe(reinterpret<smart_ptr<Annotation>> ann)
    return <- decl
}

def append_annotation(var func : FunctionPtr; mod_name, ann_name : string) {
    //! Appends an annotation to a function by module and annotation name.
    var ann = find_annotation(mod_name, ann_name)
    assert(ann != null)
    var inscope decl <- append_annotation(mod_name, ann_name)
    add_function_annotation(func, decl)
}

def append_annotation(var blk : smart_ptr<ExprBlock>; mod_name, ann_name : string) {
    //! Appends an annotation to an expression block by module and annotation name.
    var ann = find_annotation(mod_name, ann_name)
    assert(ann != null)
    var inscope decl <- append_annotation(mod_name, ann_name)
    add_block_annotation(blk, decl)
}

def append_annotation(var st : smart_ptr<Structure>; mod_name, ann_name : string) {
    //! Appends an annotation to a structure by module and annotation name.
    var ann = find_annotation(mod_name, ann_name)
    assert(ann != null)
    var inscope decl <- append_annotation(mod_name, ann_name)
    add_structure_annotation(st, decl)
}

def append_annotation(var func : FunctionPtr; mod_name, ann_name : string; args : array< tuple<argname : string; argvalue : RttiValue> >) {
    //! Appends an annotation with arguments to a function.
    var ann = find_annotation(mod_name, ann_name)
    assert(ann != null)
    var inscope decl <- append_annotation(mod_name, ann_name, args)
    add_function_annotation(func, decl)
}

def append_annotation(var blk : smart_ptr<ExprBlock>; mod_name, ann_name : string; args : array< tuple<argname : string; argvalue : RttiValue> >) {
    //! Appends an annotation with arguments to an expression block.
    var ann = find_annotation(mod_name, ann_name)
    assert(ann != null)
    var inscope decl <- append_annotation(mod_name, ann_name, args)
    add_block_annotation(blk, decl)
}

def append_annotation(var st : smart_ptr<Structure>; mod_name, ann_name : string; args : array< tuple<argname : string; argvalue : RttiValue> >) {
    //! Appends an annotation with arguments to a structure.
    var ann = find_annotation(mod_name, ann_name)
    assert(ann != null)
    var inscope decl <- append_annotation(mod_name, ann_name, args)
    add_structure_annotation(st, decl)
}

def add_annotation_argument(var arguments : AnnotationArgumentList; argName : string; val : bool) {
    //! Adds a boolean annotation argument to an argument list.
    let argIdx = arguments |> add_annotation_argument(argName)
    arguments[argIdx].basicType = Type.tBool
    arguments[argIdx].bValue = val
    return argIdx
}

def add_annotation_argument(var arguments : AnnotationArgumentList; argName : string; val : int) {
    //! Adds an integer annotation argument to an argument list.
    let argIdx = arguments |> add_annotation_argument(argName)
    arguments[argIdx].basicType = Type.tInt
    arguments[argIdx].iValue = val
    return argIdx
}

def add_annotation_argument(var arguments : AnnotationArgumentList; argName : string; val : float) {
    //! Adds a float annotation argument to an argument list.
    let argIdx = arguments |> add_annotation_argument(argName)
    arguments[argIdx].basicType = Type.tFloat
    arguments[argIdx].fValue = val
    return argIdx
}

def add_annotation_argument(var arguments : AnnotationArgumentList; argName : string; val : string) {
    //! Adds a string annotation argument to an argument list.
    let argIdx = arguments |> add_annotation_argument(argName)
    arguments[argIdx].basicType = Type.tString
    arguments[argIdx].sValue := val
    return argIdx
}

def add_annotation_argument(var arguments : AnnotationArgumentList; ann : rtti::AnnotationArgument) {
    //! Adds an annotation argument by copying from an existing AnnotationArgument.
    let argIdx = arguments |> add_annotation_argument(string(ann.name))
    arguments[argIdx].basicType = ann.basicType
    if (ann.basicType == Type.tBool) {
        arguments[argIdx].bValue = ann.bValue
    } elif (ann.basicType == Type.tInt) {
        arguments[argIdx].iValue = ann.iValue
    } elif (ann.basicType == Type.tFloat) {
        arguments[argIdx].fValue = ann.fValue
    } elif (ann.basicType == Type.tString) {
        arguments[argIdx].sValue := ann.sValue
    } else {
        assert(false, "unsupported annotation type, add more types")
    }
    return argIdx
}

def get_for_source_index(expr : smart_ptr<ExprFor>; svar : VariablePtr) {
    //! Returns the index of an iterator variable in a for-loop expression, or -1 if not found.
    for (v, t in expr.iteratorVariables, count()) {
        if (v == svar) {
            return t
        }
    }
    return -1
}

def get_for_source_index(expr : smart_ptr<ExprFor>; source : ExpressionPtr) {
    //! Returns the index of a source expression in a for-loop expression, or -1 if not found.
    for (s, t in expr.sources, count()) {
        if (s == source) {
            return t
        }
    }
    return -1
}

[macro_function]
def make_static_assert_false(text : string; at : LineInfo) {
    //! Creates a static_assert(false, text) expression for compile-time error reporting.
    var inscope esa <- new ExprStaticAssert(at = at)
    esa.arguments |> emplace_new <| new ExprConstBool(at = at, value = false)
    esa.arguments |> emplace_new <| new ExprConstString(at = at, value := text)
    return <- esa
}

[_macro]
def private setup {
    if (is_compiling_macros_in_module("ast_boost")) {
        add_new_function_annotation("macro", new MacroMacro())
        add_new_function_annotation("tag_function", new TagFunctionAnnotation())
        add_new_structure_annotation("function_macro", new SetupFunctionAnnotation())
        add_new_structure_annotation("block_macro", new SetupBlockAnnotation())
        add_new_structure_annotation("structure_macro", new SetupStructureAnnotation())
        add_new_structure_annotation("enumeration_macro", new SetupEnumerationAnnotation())
        add_new_structure_annotation("contract", new SetupContractAnnotation())
        add_new_structure_annotation("reader_macro", new SetupReaderMacro())
        add_new_structure_annotation("comment_reader", new SetupCommentReader())
        add_new_structure_annotation("call_macro", new SetupCallMacro())
        add_new_structure_annotation("typeinfo_macro", new SetupTypeInfoMacro())
        add_new_structure_annotation("variant_macro", new SetupVariantMacro())
        add_new_structure_annotation("for_loop_macro", new SetupForLoopMacro())
        add_new_structure_annotation("capture_macro", new SetupCaptureMacro())
        add_new_structure_annotation("type_macro", new SetupTypeMacro())
        add_new_structure_annotation("simulate_macro", new SetupSimulateMacro())
        add_new_structure_annotation("tag_structure", new TagStructureAnnotation())
        add_new_structure_annotation("tag_function_macro", new TagFunctionMacro())
        add_new_structure_annotation("infer_macro", new SetupInferMacro())
        add_new_structure_annotation("dirty_infer_macro", new SetupDirtyInferMacro())
        add_new_structure_annotation("optimization_macro", new SetupOptimizationMacro())
        add_new_structure_annotation("lint_macro", new SetupLintMacro())
        add_new_structure_annotation("global_lint_macro", new SetupGlobalLintMacro())
        add_new_variant_macro("better_rtti_in_expr", new BetterRttiVisitor())
    }
}


def public function_to_type(fn : FunctionPtr) : TypeDeclPtr {
    //! Converts a function declaration to a function-type TypeDecl.
    var inscope td <- new TypeDecl(baseType = Type.tFunction,
        firstType <- fn.result != null ? clone_type(fn.result) : new TypeDecl(baseType = Type.autoinfer)
    )
    td.argNames |> resize(length(fn.arguments))
    for (arg, argi in fn.arguments, count()) {
        td.argTypes |> emplace_new <| clone_type(arg._type)
        td.argNames[argi] := arg.name
    }
    return <- td
}

def public visit_finally(blk : ExprBlock?; adapter : smart_ptr<VisitorAdapter>) {
    //! Visits the finally section of an ExprBlock using the given visitor adapter.
    unsafe {
        visit_finally(reinterpret<smart_ptr<ExprBlock>> blk, adapter)
    }
}

def public isCMRES(fun : FunctionPtr) : bool {
    //! Returns true if the function returns by copy or move (CMRES).
    return fun.flags.copyOnReturn || fun.flags.moveOnReturn
}

def public isCMRES(fun : Function?) : bool {
    //! Returns true if the function returns by copy or move (CMRES).
    return fun.flags.copyOnReturn || fun.flags.moveOnReturn
}

def public isMakeLocal(expr : ExpressionPtr) {
    //! Returns true if the expression is a make-local expression (struct, variant, array, or tuple).
    return ((expr.__rtti == "ExprMakeLocal")
        ||  (expr.__rtti == "ExprMakeStruct")
        ||  (expr.__rtti == "ExprMakeVariant")
        ||  (expr.__rtti == "ExprMakeArray")
        ||  (expr.__rtti == "ExprMakeTuple"))
}

def public isExprCallFunc(expr : ExpressionPtr) {
    //! Returns true if the expression is a function call expression (call, op, or new).
    return ((expr.__rtti == "ExprCallFunc")
        ||  (expr.__rtti == "ExprOp")
        ||  (expr.__rtti == "ExprNew")
        ||  (expr.__rtti == "ExprCall"))
}

def public get_workhorse_types {
    //! Returns a fixed array of all commonly used base types.
    return fixed_array(
        Type.tBool,
        Type.tString,           Type.tPointer,
        Type.tFloat,            Type.tDouble,
        Type.tEnumeration,      Type.tEnumeration8,     Type.tEnumeration16,    Type.tEnumeration64,
        Type.tBitfield,         Type.tBitfield8,        Type.tBitfield16,       Type.tBitfield64,
        Type.tInt,              Type.tInt8,             Type.tInt16,            Type.tInt64,
        Type.tUInt,             Type.tUInt8,            Type.tUInt16,           Type.tUInt64,
        Type.tInt2,             Type.tInt3,             Type.tInt4,
        Type.tUInt2,            Type.tUInt3,            Type.tUInt4,
        Type.tFloat2,           Type.tFloat3,           Type.tFloat4,
        Type.tRange,            Type.tRange64,
        Type.tURange,           Type.tURange64,
    )
}

def public find_argument_index(typ : TypeDeclPtr; name : string) {
    //! Returns the index of a named argument in a type declaration, or -1 if not found.
    for (a, ai in typ.argNames, count()) {
        if (a == name) {
            return ai
        }
    }
    return -1
}

def public isCMRESType(blockT : TypeDeclPtr) {
    //! Returns true if the type is a reference type not passed by ref (CMRES pattern).
    return blockT != null && blockT.isRefType && !blockT.flags.ref
}

bitfield DebugExpressionFlags {
    //! Flags controlling debug expression output formatting.
    refCount,
    lineInfo
}

def private debug_expression_impl(var writer : StringBuilderWriter; expr : ExpressionPtr; deFlags : DebugExpressionFlags; tabs : int) {
    if (expr == null) {
        writer |> write("null")
        return
    }
    if (empty(expr.__rtti)) {
        writer |> write("[NO RTTI]")
        return
    }
    writer |> write("({expr.__rtti}")
    if (deFlags.refCount) {
        writer |> write("<{smart_ptr_use_count(expr)}>")
    }
    if (deFlags.lineInfo) {
        let e = get_ptr(expr)
        writer |> write(" at={int(e.at.line)}:{int(e.at.column)}-{int(e.at.last_line)}:{int(e.at.last_column)}")
        if (e.at.fileInfo != null) {
            writer |> write(" file=\"{e.at.fileInfo.name}\"")
        }
        if (string(e.__rtti) == "ExprCall" || string(e.__rtti) == "ExprInvoke") {
            let enc = unsafe(reinterpret<ExprLooksLikeCall?> e)
            if (int(enc.atEnclosure.line) != 0 || int(enc.atEnclosure.last_line) != 0) {
                writer |> write(" atEnclosure={int(enc.atEnclosure.line)}:{int(enc.atEnclosure.column)}-{int(enc.atEnclosure.last_line)}:{int(enc.atEnclosure.last_column)}")
            }
        }
    }
    if (string(expr.__rtti) |> starts_with("ExprConst")) {
        writer |> write(" {describe(expr)})")
        return
    }
    var ann = unsafe(reinterpret<BasicStructureAnnotation?> get_expression_annotation(get_ptr(expr)))
    if (ann == null) {
        writer |> write(")")
        return
    }
    for_each_field(*ann) $(name, cppName, xtype, offset) {
        if (offset != -1u) {
            let tstr = describe(xtype)
            var p8 : int8?
            unsafe {
                p8 = (reinterpret<int8?> expr) + int(offset)
            }
            if (tstr == "smart_ptr<ast_core::Expression>") {
                let pv = unsafe(reinterpret<ExpressionPtr?> p8)
                let ts = repeat("  ", tabs + 2)
                writer |> write("\n{ts}({name} ")
                debug_expression_impl(writer, *pv, deFlags, tabs + 2)
                writer |> write(")")
            } elif (tstr == "$::das_string") {
                let pv = unsafe(reinterpret<das_string?> p8)
                writer |> write(" {name}=\"{*pv}\"")
            } elif (tstr == "int") {
                let pv = unsafe(reinterpret<int?> p8)
                writer |> write(" {name}={*pv}")
            } elif (tstr == "uint") {
                let pv = unsafe(reinterpret<uint?> p8)
                writer |> write(" {name}={*pv}")
            } elif (tstr == "int64") {
                let pv = unsafe(reinterpret<int64?> p8)
                writer |> write(" {name}={*pv}")
            } elif (tstr == "uint64") {
                let pv = unsafe(reinterpret<uint64?> p8)
                writer |> write(" {name}={*pv}")
            } elif (tstr == "bool") {
                let pv = unsafe(reinterpret<bool?> p8)
                writer |> write(" {name}={*pv}")
            } elif (tstr == "$::dasvector`smart_ptr`Expression") {
                pass
            } else {
                pass
            }
        }
    }
                // writer |> write(" {name}={describe(xtype)}")
    for_each_field(*ann) $(name, cppName, xtype, offset) {
        if (offset != -1u) {
            let tstr = describe(xtype)
            var p8 : int8?
            unsafe {
                p8 = (reinterpret<int8?> expr) + int(offset)
            }
            if (tstr == "$::dasvector`smart_ptr`Expression") {
                let pv = unsafe(reinterpret<dasvector`smart_ptr`Expression?> p8)
                if (length(*pv) != 0) {
                    let ts = repeat("  ", tabs + 2)
                    writer |> write("\n{ts}[{name}\n")
                    for (l, i in *pv, count()) {
                        writer |> write("{ts}  ")
                        debug_expression_impl(writer, l, deFlags, tabs + 2)
                        if (i != length(*pv) - 1) {
                            writer |> write("\n")
                        }
                    }
                    writer |> write("]")
                }
            } elif (tstr == "$::dasvector`smart_ptr`Variable") {
                let pv = unsafe(reinterpret<dasvector`smart_ptr`Variable?> p8)
                if (length(*pv) != 0) {
                    let ts = repeat("  ", tabs + 2)
                    writer |> write("\n{ts}[{name}\n")
                    for (l, i in *pv, count()) {
                        writer |> write("{ts}  ")
                        writer |> write("{l.name}:{describe(l._type)}")
                        if (l.init != null) {
                            writer |> write(" = ")
                            debug_expression_impl(writer, l.init, deFlags, tabs + 2)
                        }
                        if (i != length(*pv) - 1) {
                            writer |> write("\n")
                        }
                    }
                    writer |> write("]")
                }
            }
        }
    }
    writer |> write(")")
}

def public debug_expression(expr : ExpressionPtr; deFlags : DebugExpressionFlags = bitfield(0)) {
    //! Returns a debug S-expression string representation of an AST expression.
    return build_string() $(writer) {
        debug_expression_impl(writer, expr, deFlags, 0)
    }
}

def public debug_expression(expr : Expression?) {
    //! Returns a debug S-expression string representation of an AST expression from a raw pointer.
    unsafe {
        return debug_expression(reinterpret<ExpressionPtr> expr)
    }
}

def public describe(expr : Expression?) {
    //! Returns a string description of an AST expression from a raw pointer.
    unsafe {
        return describe(reinterpret<ExpressionPtr> expr)
    }
}

def getVectorElementCount(bt : Type) {
    //! Returns the number of elements in a vector base type (2, 3, or 4), or 0 if not a vector.
    return 2 if (bt == Type.tFloat2 || bt == Type.tInt2 || bt == Type.tUInt2 || bt == Type.tRange || bt == Type.tURange || bt == Type.tRange64 || bt == Type.tURange64)
    return 3 if (bt == Type.tFloat3 || bt == Type.tInt3 || bt == Type.tUInt3)
    return 4 if (bt == Type.tFloat4 || bt == Type.tInt4 || bt == Type.tUInt4)
    return 0
}

def getVectorElementSize(bt : Type) {
    //! Returns the byte size of a single element in a vector base type.
    return 8 if (bt == Type.tRange64 || bt == Type.tURange64)
    return 4
}

def getVectorElementType(bt : Type) {
    //! Returns the scalar element type of a vector base type.
    return Type.tFloat if (bt == Type.tFloat2 || bt == Type.tFloat3 || bt == Type.tFloat4)
    return Type.tInt if (bt == Type.tInt2 || bt == Type.tInt3 || bt == Type.tInt4 || bt == Type.tRange)
    return Type.tUInt if (bt == Type.tUInt2 || bt == Type.tUInt3 || bt == Type.tUInt4 || bt == Type.tURange)
    return Type.tInt64 if (bt == Type.tRange64)
    return Type.tUInt64 if (bt == Type.tURange64)
    return Type.tVoid
}

def getVectorOffset(bt : Type; ident : string) {
    //! Returns the element index for a vector swizzle identifier (x/y/z/w or r/g/b/a), or -1 if invalid.
    var ofs = -1
    if (ident == "x" || ident == "X" || ident == "r" || ident == "R") {
        ofs = 0
    } elif (ident == "y" || ident == "Y" || ident == "g" || ident == "G") {
        ofs = 1
    } elif (ident == "z" || ident == "Z" || ident == "b" || ident == "B") {
        ofs = 2
    } elif (ident == "w" || ident == "W" || ident == "a" || ident == "A") {
        ofs = 3
    }
    let count = getVectorElementCount(bt)
    return ofs >= count ? -1 : ofs
}
