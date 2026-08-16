options gen2

options indenting = 4

module llvm_dsl shared private

require daslib/ast_boost
require daslib/macro_boost
require daslib/templates_boost

// jit_fn_type(signature) — derive a JIT extern helper's LLVM
// function type from a das signature.
// `signature` is EITHER an inline block `$(args) : ret {}` OR
// `@@stub`, a reference to a signature-carrier function.
[call_macro(name="jit_fn_type")]
class JitFnTypeMacro : AstCallMacro {
    def override visit(prog : ProgramPtr; mod : Module?; var call : ExprCallMacro?) : ExpressionPtr {
        macro_verify(call.arguments |> length == 1, prog, call.at,
            "jit_fn_type expects one argument: an inline signature block, or @@signature_stub")
        var argTypes : array<Type>
        var retType = Type.tVoid
        let arg = call.arguments[0]
        if (arg is ExprMakeBlock && (arg as ExprMakeBlock)._block is ExprBlock) {
            let blk = (arg as ExprMakeBlock)._block as ExprBlock
            for (a in blk.arguments) {
                argTypes |> push(a._type.baseType)
            }
            if (blk.returnType != null) {
                retType = blk.returnType.baseType
            }
        } elif (arg is ExprAddr) {
            let fn = (arg as ExprAddr).func
            macro_verify(fn != null, prog, call.at,
                "jit_fn_type: @@ does not resolve to a function, got {describe(arg)}")
            for (a in fn.arguments) {
                argTypes |> push(a._type.baseType)
            }
            retType = fn.result.baseType
        } else {
            macro_verify(false, prog, call.at,
                "jit_fn_type expects an inline signature block or @@signature_stub, got {describe(arg)}")
            return default<ExpressionPtr>
        }
        var argExprs <- [for (bt in argTypes); qmacro(jit_extern_type($v(bt)))]
        // no <LLVMTypeRef> on fixed_array: the alias would not unify across the macro/splice boundary
        if (empty(argExprs)) {
            return qmacro(LLVMFunctionType(jit_extern_type($v(retType))))
        }
        return qmacro(LLVMFunctionType(jit_extern_type($v(retType)), fixed_array($a(argExprs))))
    }
}

// jit_struct_type(type<tuple<...>>) — build an LLVM struct type from a tuple's element types, mirroring
// a C++ struct layout. Field names in the tuple are documentation only. Opaque pointers collapse to `ptr`,
// so the element types determine the struct. Only for natural-layout mirrors (no manual padding).
[call_macro(name="jit_struct_type")]
class JitStructTypeMacro : AstCallMacro {
    def override visit(prog : ProgramPtr; mod : Module?; var call : ExprCallMacro?) : ExpressionPtr {
        macro_verify(call.arguments |> length == 1, prog, call.at,
            "jit_struct_type expects one argument: type<tuple<...>>")
        macro_verify(call.arguments[0] is ExprTypeDecl, prog, call.at,
            "jit_struct_type expects type<tuple<...>>, got {describe(call.arguments[0])}")
        let td = (call.arguments[0] as ExprTypeDecl).typeexpr
        macro_verify(td != null && td.baseType == Type.tTuple, prog, call.at,
            "jit_struct_type expects a tuple type, got {describe(td)}")
        var fieldExprs <- [for (ft in td.argTypes); qmacro(jit_extern_type($v(ft.baseType)))]
        return qmacro(g_prim_t |> StructType(fixed_array($a(fieldExprs))))
    }
}
