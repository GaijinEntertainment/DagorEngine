options gen2
options indenting = 4
options no_unused_block_arguments = false
options no_unused_function_arguments = false
options strict_smart_pointers = true

module delegate shared private

//! C#-like delegate (multicast callback) via type macro.
//!
//! Usage::
//!
//!     require daslib/delegate
//!     typedef OnEvent = delegate(type<function<(arg:string):bool>>)
//!     // or equivalently:
//!     typedef OnEvent = delegate(type<lambda<(arg:string):bool>>)
//!
//! The generated struct supports:
//!   - construction from lambda or function pointer
//!   - ``operator :=`` (clear + assign single handler)
//!   - ``operator +=`` (append handler)
//!   - ``invoke(args...)`` — calls all handlers; returns last result (or void)
//!   - ``empty``, ``length``, ``clear`` utilities

require daslib/macro_boost
require daslib/typemacro_boost
require strings

// ── helpers ────────────────────────────────────────────────────────────

[macro_function]
def private get_arg_name(funcType : TypeDeclPtr; index : int) : string {
    if (index < length(funcType.argNames)) {
        let n = string(funcType.argNames[index])
        if (!empty(n)) {
            return n
        }
    }
    return "_delegate_arg_{index}"
}

[macro_function]
def private build_invoke_call(at : LineInfo; var targetExpr : ExpressionPtr; funcType : TypeDeclPtr) : ExpressionPtr {
    //! Builds  invoke(target, arg0, arg1, ...)  as an ExprInvoke (special node for lambda/function calls).
    var inscope call <- new ExprInvoke(at = at, name := "invoke")
    (call as ExprInvoke).arguments |> emplace(targetExpr)
    for (i in range(length(funcType.argTypes))) {
        (call as ExprInvoke).arguments |> emplace_new <| new ExprVar(at = at, name := get_arg_name(funcType, i))
    }
    return <- call
}

// ── template: non-void return ──────────────────────────────────────────

struct template DelegateReturn {
    private invocation_list : array<DelegateLambda>

    def DelegateReturn {}

    def DelegateReturn(var lmb : DelegateLambda) {
        invocation_list |> emplace(lmb)
    }

    def DelegateReturn(var fn : DelegateFunction) {
        var lmb <- $e(wrapperLambda)
        invocation_list |> emplace(lmb)
    }

    def static operator := (var self : DelegateReturn; var lmb : DelegateLambda) {
        self.invocation_list |> clear()
        self.invocation_list |> emplace(lmb)
    }

    def static operator := (var self : DelegateReturn; var fn : DelegateFunction) {
        var lmb <- $e(wrapperLambda)
        self.invocation_list |> clear()
        self.invocation_list |> emplace(lmb)
    }

    def static operator += (var self : DelegateReturn; var lmb : DelegateLambda) {
        self.invocation_list |> emplace(lmb)
    }

    def static operator += (var self : DelegateReturn; var fn : DelegateFunction) {
        var lmb <- $e(wrapperLambda)
        self.invocation_list |> emplace(lmb)
    }

    def invoke($a(invokeArgs)) : $t(retType) {
        if (empty(invocation_list)) {
            static_if (typeinfo can_copy(type<$t(retType) -const>)) {
                return default<$t(retType) -const>
            } else {
                var _delegate_default : $t(retType) -const
                return <- _delegate_default
            }
        }
        let _delegate_n = length(invocation_list)
        for (_delegate_i in 0.._delegate_n - 1) {
            $e(discardCall)
        }
        return <- $e(lastCall)
    }

    def empty() : bool {
        return empty(invocation_list)
    }

    def length() : int {
        return length(invocation_list)
    }

    def clear() {
        invocation_list |> clear()
    }

    [unsafe_outside_of_for]
    def static each(var self : DelegateReturn == const) : auto {
        return <- unsafe(each(self.invocation_list))
    }
}

// ── template: void return ──────────────────────────────────────────────

struct template DelegateVoid {
    private invocation_list : array<DelegateLambda>

    def DelegateVoid {}

    def DelegateVoid(var lmb : DelegateLambda) {
        invocation_list |> emplace(lmb)
    }

    def DelegateVoid(var fn : DelegateFunction) {
        var lmb <- $e(wrapperLambda)
        invocation_list |> emplace(lmb)
    }

    def static operator := (var self : DelegateVoid; var lmb : DelegateLambda) {
        self.invocation_list |> clear()
        self.invocation_list |> emplace(lmb)
    }

    def static operator := (var self : DelegateVoid; var fn : DelegateFunction) {
        var lmb <- $e(wrapperLambda)
        self.invocation_list |> clear()
        self.invocation_list |> emplace(lmb)
    }

    def static operator += (var self : DelegateVoid; var lmb : DelegateLambda) {
        self.invocation_list |> emplace(lmb)
    }

    def static operator += (var self : DelegateVoid; var fn : DelegateFunction) {
        var lmb <- $e(wrapperLambda)
        self.invocation_list |> emplace(lmb)
    }

    def invoke($a(invokeArgs)) {
        for (cb in invocation_list) {
            $e(voidCall)
        }
    }

    def empty() : bool {
        return empty(invocation_list)
    }

    def length() : int {
        return length(invocation_list)
    }

    def clear() {
        invocation_list |> clear()
    }

    [unsafe_outside_of_for]
    def static each(var self : DelegateVoid == const) : auto {
        return <- unsafe(each(self.invocation_list))
    }
}

// ── typemacro ──────────────────────────────────────────────────────────

[typemacro_function]
def public delegate(macroArgument, passArgument : TypeDeclPtr; FuncType : TypeDeclPtr) : TypeDeclPtr {
    //! Type macro that generates a delegate struct from a function or lambda type.

    // ── validate input ─────────────────────────────────────────────────
    if (FuncType.baseType != Type.tFunction && FuncType.baseType != Type.tLambda) {
        macro_error(compiling_program(), macroArgument.at, "delegate expects function<...> or lambda<...> type, got {describe(FuncType)}")
        return <- TypeDeclPtr()
    }

    // ── normalize types ────────────────────────────────────────────────
    var inscope lambdaType <- clone_type(FuncType)
    lambdaType.baseType = Type.tLambda
    var inscope functionType <- clone_type(FuncType)
    functionType.baseType = Type.tFunction

    let is_void = FuncType.firstType == null || FuncType.firstType.isVoid

    // ── template references ────────────────────────────────────────────
    var inscope template_type_ret  <- typeinfo ast_typedecl(type<DelegateReturn>)
    var inscope template_type_void <- typeinfo ast_typedecl(type<DelegateVoid>)

    // primary template argument (used for generic matching and struct naming)
    var inscope extra_template_arguments : array<tuple<string; string>>

    // ── generic path ───────────────────────────────────────────────────
    if (passArgument != null) {
        // try DelegateReturn
        if (is_typemacro_template_instance(passArgument, template_type_ret, extra_template_arguments)) {
            var inscope args_ret <- [TypeMacroTemplateArgument(name = "DelegateLambda", argument_type <- clone_type(FuncType))]
            if (infer_struct_aliases(passArgument.structType, args_ret)) {
                return <- infer_template_types(passArgument, args_ret)
            }
        }
        // try DelegateVoid
        if (is_typemacro_template_instance(passArgument, template_type_void, extra_template_arguments)) {
            var inscope args_void <- [TypeMacroTemplateArgument(name = "DelegateLambda", argument_type <- clone_type(FuncType))]
            if (infer_struct_aliases(passArgument.structType, args_void)) {
                return <- infer_template_types(passArgument, args_void)
            }
        }
        return <- TypeDeclPtr()
    }

    // ── concrete path ──────────────────────────────────────────────────
    var inscope template_arguments <- [
        TypeMacroTemplateArgument(name = "DelegateLambda", argument_type <- clone_type(lambdaType))
    ]
    if (!verify_arguments(template_arguments)) {
        return <- TypeDeclPtr()
    }

    // pick template
    var template_struct = is_void ? template_type_void.structType : template_type_ret.structType

    let struct_name = template_structure_name(template_struct, template_arguments, extra_template_arguments)

    // deduplication
    var existing_struct = compiling_program().find_unique_structure(struct_name)
    if (existing_struct != null) {
        return <- new TypeDecl(baseType = Type.tStructure, structType = existing_struct, at = macroArgument.at)
    }

    // ── build reification variables ────────────────────────────────────
    let nArgs = length(FuncType.argTypes)

    // invoke parameter list
    var inscope invokeArgs : array<VariablePtr>
    // forward expressions (ExprVar for each arg name)
    var inscope fwdExprs : array<ExpressionPtr>
    for (idx in range(nArgs)) {
        let argName = get_arg_name(FuncType, idx)
        invokeArgs |> emplace_new <| new Variable(at = macroArgument.at, name := argName,
            _type <- clone_type(FuncType.argTypes[idx]))
        fwdExprs |> emplace_new <| new ExprVar(at = macroArgument.at, name := argName)
    }

    // lambda args for wrapper (separate copies)
    var inscope lambdaArgs : array<VariablePtr>
    var inscope lambdaFwd  : array<ExpressionPtr>
    for (idx in range(nArgs)) {
        let argName = get_arg_name(FuncType, idx)
        lambdaArgs |> emplace_new <| new Variable(at = macroArgument.at, name := argName,
            _type <- clone_type(FuncType.argTypes[idx]))
        lambdaFwd |> emplace_new <| new ExprVar(at = macroArgument.at, name := argName)
    }

    // ── instantiate template ───────────────────────────────────────────
    var inscope resType : TypeDeclPtr
    if (is_void) {
        // void-specific reification variables
        var inscope voidCall <- build_invoke_call(macroArgument.at,
            new ExprVar(at = macroArgument.at, name := "cb"), FuncType)
        var inscope wrapperLambda <- qmacro(@($a(lambdaArgs)) {
            invoke(fn, $a(lambdaFwd))
        })
        resType |> move_new <| qmacro_template_class(struct_name, type<DelegateVoid>)
    } else {
        // non-void-specific reification variables
        var inscope discardCall <- build_invoke_call(macroArgument.at,
            qmacro(invocation_list[_delegate_i]), FuncType)
        var inscope lastCall <- build_invoke_call(macroArgument.at,
            qmacro(invocation_list[_delegate_n - 1]), FuncType)
        var inscope retType <- clone_type(FuncType.firstType)
        var inscope wrapperLambda <- qmacro(@($a(lambdaArgs)) : $t(retType) {
            return invoke(fn, $a(lambdaFwd))
        })
        unsafe {
            resType |> move_new <| qmacro_template_class(struct_name, type<DelegateReturn>)
        }
    }

    // ── mark as template instance + add aliases ────────────────────────
    make_typemacro_template_instance(resType.structType, template_struct, extra_template_arguments)
    add_structure_aliases(resType.structType, template_arguments)
    // extra aliases (not used for generic matching, but needed for method type resolution)
    var inscope funcAlias <- clone_type(functionType)
    resType.structType |> add_structure_alias("DelegateFunction", funcAlias)
    if (!is_void) {
        var inscope retAlias <- clone_type(FuncType.firstType)
        resType.structType |> add_structure_alias("DelegateReturnType", retAlias)
    }

    return <- resType
}

