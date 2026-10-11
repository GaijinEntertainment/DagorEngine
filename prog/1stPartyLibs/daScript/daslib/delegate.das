options gen2
options indenting = 4
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

// ── helpers ────────────────────────────────────────────────────────────

[macro_function]
def private get_arg_name(funcType : TypeDeclPtr; index : int) : string {
    if (index < length(funcType.argNames)) {
        let n = string(funcType.argNames[index])
        if (!empty(n)) return n
    }
    return "_delegate_arg_{index}"
}

[macro_function]
def private build_invoke_call(at : LineInfo; var targetExpr : ExpressionPtr; funcType : TypeDeclPtr) : ExpressionPtr {
    //! Builds  invoke(target, arg0, arg1, ...)  as an ExprInvoke (special node for lambda/function calls).
    var call = new ExprInvoke(at = at, name := "invoke")
    call.arguments |> emplace(targetExpr)
    for (i in range(length(funcType.argTypes))) {
        call.arguments |> emplace_new <| new ExprVar(at = at, name := get_arg_name(funcType, i))
    }
    return call
}

struct template public DelegateReturn {
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
                var delegate_default : $t(retType) -const
                return <- delegate_default
            }
        }
        let delegate_n = length(invocation_list)
        for (delegate_i in 0..delegate_n - 1) {
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

struct template public DelegateVoid {
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

[typemacro_function]
def public delegate(macroArgument, passArgument : TypeDeclPtr; FuncType : TypeDeclPtr) : TypeDeclPtr {  // nolint:STYLE038 - DelegateReturn/DelegateVoid splice names out of THIS scope; moving the builders into helpers breaks reification
    //! Type macro that generates a delegate struct from a function or lambda type.

    if (FuncType.baseType != Type.tFunction && FuncType.baseType != Type.tLambda) {
        macro_error(compiling_program(), macroArgument.at, "delegate expects function<...> or lambda<...> type, got {describe(FuncType)}")
        return null
    }

    var lambdaType = clone_type(FuncType)
    lambdaType.baseType = Type.tLambda
    var functionType = clone_type(FuncType)
    functionType.baseType = Type.tFunction

    let is_void = FuncType.firstType == null || FuncType.firstType.isVoid

    var template_type_ret = typeinfo ast_typedecl(type<DelegateReturn>)
    var template_type_void = typeinfo ast_typedecl(type<DelegateVoid>)

    var inscope extra_template_arguments : array<tuple<string; string>>

    if (passArgument != null) {
        if (is_typemacro_template_instance(passArgument, template_type_ret, extra_template_arguments)) {
            var args_ret <- [TypeMacroTemplateArgument(name = "DelegateLambda", argument_type = clone_type(FuncType))]
            if (infer_struct_aliases(passArgument.structType, args_ret)) return infer_template_types(passArgument, args_ret)
        }
        if (is_typemacro_template_instance(passArgument, template_type_void, extra_template_arguments)) {
            var args_void <- [TypeMacroTemplateArgument(name = "DelegateLambda", argument_type = clone_type(FuncType))]
            if (infer_struct_aliases(passArgument.structType, args_void)) return infer_template_types(passArgument, args_void)
        }
        return null
    }

    var template_arguments <- [
        TypeMacroTemplateArgument(name = "DelegateLambda", argument_type = clone_type(lambdaType))
    ]
    if (!verify_arguments(template_arguments)) return null

    var template_struct = is_void ? template_type_void.structType : template_type_ret.structType

    let struct_name = template_structure_name(template_struct, template_arguments, extra_template_arguments)

    var existing_struct = compiling_program().find_unique_structure(struct_name)
    if (existing_struct != null) return new TypeDecl(baseType = Type.tStructure, structType = existing_struct, at = macroArgument.at)

    let nArgs = length(FuncType.argTypes)

    var invokeArgs : array<VariablePtr>
    var fwdExprs : array<ExpressionPtr>
    fwdExprs |> reserve(nArgs)
    for (idx in range(nArgs)) {
        let argName = get_arg_name(FuncType, idx)
        invokeArgs |> emplace_new <| new Variable(at = macroArgument.at, name := argName,
            _type = clone_type(FuncType.argTypes[idx]))
        fwdExprs |> push(new ExprVar(at = macroArgument.at, name := argName))
    }

    var lambdaArgs : array<VariablePtr>
    var lambdaFwd  : array<ExpressionPtr>
    lambdaFwd |> reserve(nArgs)
    for (idx in range(nArgs)) {
        let argName = get_arg_name(FuncType, idx)
        lambdaArgs |> emplace_new <| new Variable(at = macroArgument.at, name := argName,
            _type = clone_type(FuncType.argTypes[idx]))
        lambdaFwd |> push(new ExprVar(at = macroArgument.at, name := argName))
    }

    var resType : TypeDeclPtr
    if (is_void) {
        var voidCall = build_invoke_call(macroArgument.at,
            new ExprVar(at = macroArgument.at, name := "cb"), FuncType)
        var wrapperLambda = qmacro(@($a(lambdaArgs)) {
            invoke(fn, $a(lambdaFwd))
        })
        resType = qmacro_template_class(struct_name, type<DelegateVoid>)
    } else {
        var discardCall = build_invoke_call(macroArgument.at,
            qmacro(invocation_list[delegate_i]), FuncType)
        var lastCall = build_invoke_call(macroArgument.at,
            qmacro(invocation_list[delegate_n - 1]), FuncType)
        var retType = clone_type(FuncType.firstType)
        var wrapperLambda = qmacro(@($a(lambdaArgs)) : $t(retType) {
            return invoke(fn, $a(lambdaFwd))
        })
        resType = qmacro_template_class(struct_name, type<DelegateReturn>)
    }

    make_typemacro_template_instance(resType.structType, template_struct, extra_template_arguments)
    add_structure_aliases(resType.structType, template_arguments)
    var funcAlias = clone_type(functionType)
    resType.structType |> add_structure_alias("DelegateFunction", funcAlias)
    if (!is_void) {
        var retAlias = clone_type(FuncType.firstType)
        resType.structType |> add_structure_alias("DelegateReturnType", retAlias)
    }

    return resType
}

