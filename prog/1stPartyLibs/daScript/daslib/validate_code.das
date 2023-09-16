options indenting = 4
options no_unused_block_arguments = false
options no_unused_function_arguments = false
options strict_smart_pointers = true

module validate_code shared private

require daslib/ast_boost
require daslib/rtti
require daslib/strings
require daslib/contracts

[macro_function]
def check_recursive ( at:LineInfo; fun:Function?; var call_graph:array<Function?>; var visited:table<Function?> )
    if fun.flags.builtIn
        return
    if call_graph |> find_index(fun)>=0
        var sb = build_string <| $ ( writer )
            writer |> write("recursive calls are not allowed")
            for v in call_graph
                writer |> write(" -> ")
                writer |> write(v.name)
            writer |> write(" -> ")
            writer |> write(fun.name)
        macro_error(compiling_program(), at, sb)
        return
    visited |> insert(fun)
    call_graph |> push(fun)
    unsafe(reinterpret<FunctionPtr> fun) |> get_use_functions <| $ ( f )
        check_recursive(at, get_ptr(f), call_graph, visited)
    call_graph |> pop()

[macro_function]
def check_recursive ( at:LineInfo; fun:Function? )
    var call_graph : array<Function?>
    var visited : table<Function?>
    check_recursive(at, fun, call_graph, visited)

[macro_function]
def collect_call_tree ( fun:Function?; var visited:table<Function?> )
    if fun.flags.builtIn
        return
    if visited |> key_exists(fun)
        return
    visited |> insert(fun)
    unsafe(reinterpret<FunctionPtr> fun) |> get_use_functions <| $ ( f )
        collect_call_tree(get_ptr(f), visited)

[macro]
class ValidateCompletionVisitor : AstVisitor
    [[do_not_delete]] thisFunc:Function?
    def override preVisitFunction(fun:FunctionPtr) : void
        thisFunc = get_ptr(fun)
    def override visitFunction(var fun:FunctionPtr) : FunctionPtr
        thisFunc = null
        return <- fun
    def override preVisitExprFor(expr:smart_ptr<ExprFor>) : void
        var srcOk = false
        for src in expr.sources
            if src._type.isRange || src._type.isArray || src._type.isGoodArrayType || isYetAnotherVectorTemplate(src._type)
                srcOk = true
                break
            elif src is ExprCall
                let srcc = src as ExprCall
                if srcc.name=="__::builtin`keys" || srcc.name=="__::builtin`values"
                    srcOk = true
                    break
                // TODO: add support for more known finite sources
        if !srcOk
            macro_error(compiling_program(), expr.at, "we don't know if this `for` loop ever finishes. add explicit range")
    def override preVisitExprWhile(expr:smart_ptr<ExprWhile>) : void
        macro_error(compiling_program(), expr.at, "we can't tell if this `while` loop ever finishes")
    def override preVisitExprCall(expr:smart_ptr<ExprCall>) : void
        if expr.func.flags.builtIn
            return
        if expr.func == thisFunc
            macro_error(compiling_program(), expr.at, "recursive calls are not allowed")
        else
            check_recursive(expr.at, expr.func)

[function_macro(name="verify_completion")]
class VerifyCompletion : AstFunctionAnnotation
    def override lint ( var func:FunctionPtr; var group:ModuleGroup; args,progArgs:AnnotationArgumentList; var errors : das_string ) : bool
        var visited : table<Function?>
        var astVisitor = new ValidateCompletionVisitor()
        var inscope adapter <- make_visitor(*astVisitor)
        collect_call_tree(get_ptr(func), visited)
        for fun in visited |> keys()
            visit(unsafe(reinterpret<FunctionPtr> fun), adapter)
        unsafe
            delete astVisitor
        return true

[macro]
class ValidateShaderVisitor : AstVisitor
    def override preVisitTypeDecl(typ:TypeDeclPtr) : void
        if !typ.isNoHeapType
            macro_error(compiling_program(), typ.at, "not shader-like, type requires heap {describe(typ)}")
    def override preVisitExprAscend(expr:smart_ptr<ExprAscend>) : void
        macro_error(compiling_program(), expr.at, "can't ascend in shader-like code (new [[...]])")
    def override preVisitExprNew(expr:smart_ptr<ExprNew>): void
        macro_error(compiling_program(), expr.at, "can't new in shader-like code (new ...)")

[simulate_macro(name="shader_like")]
class JIT_LLVM : AstSimulateMacro
    def lint_module ( prog:Program?; var ctx:Context? )
        var astVisitor = new ValidateShaderVisitor()
        var inscope adapter <- make_visitor(*astVisitor)
        prog |> for_each_module <| $ ( mod )
            mod |> for_each_function("") <| $ ( f )
                if f.flags.used
                    visit(f, adapter)
            mod |> for_each_global() <| $ ( g )
                if g.flags.used
                    if !g._type.isNoHeapType
                        macro_error(compiling_program(), g.at, "global variable {g.name} requires heap")
                    elif g.init!=null
                        visit(g.init, adapter)
        unsafe
            delete astVisitor
    def override simulate ( prog:Program?; var ctx:Context? ) : bool
        return true if is_in_completion() || is_compiling_macros()
        return true if ctx.category.debug_context || ctx.category.macro_context || ctx.category.folding_context || ctx.category.debugger_tick || ctx.category.debugger_attached
        if prog._options |> find_arg("shader_like") ?as tBool ?? false
            lint_module(prog,ctx)
            return true
        return true

[_macro,macro_function]
def init_shader_like
    if is_compiling_macros_in_module("validate_code")
        this_module() |> add_module_option("shader_like",Type tBool)
