options indenting = 4
options no_unused_block_arguments = false
options no_unused_function_arguments = false

module scope_lint shared private

require daslib/strings_boost
require daslib/ast_boost
require daslib/strings_boost

[lint_macro]
class ScopeLint : AstPassMacro
    def override apply ( prog:ProgramPtr; mod:Module? ) : bool
        paranoid(prog, true)
        return true

class LintVisitor : AstVisitor
    compile_time_errors : bool
    noLint : bool = false
    def LintVisitor()
        pass
    def lint_error ( text:string; at:LineInfo ) : void
        if noLint
            return
        if compile_time_errors
            compiling_program() |> macro_error(at, text)
        else
            error("{text} at {describe(at)}\n")
    def override preVisitFunction(fun:FunctionPtr) : void
        noLint = false
        for ann in fun.annotations
            if ann.annotation.name=="no_lint"
                noLint = true
                break
    def override visitFunction(var fun:FunctionPtr) : FunctionPtr
        noLint = false
        return <- fun
    def override preVisitExprLet(expr:smart_ptr<ExprLet>) : void
        print("let {describe(expr)}\n")
        for v in expr.variables
            if !v.flags.inScope && v._type |> aggresiveInScope
                lint_error("variable {v.name} is not in scope", v.at)

[macro_function]
def paranoid ( prog:ProgramPtr; compile_time_errors : bool )
    var astVisitor = new [[LintVisitor() compile_time_errors=compile_time_errors]]
    var astVisitorAdapter <- make_visitor(*astVisitor)
    visit ( prog, astVisitorAdapter )
    astVisitorAdapter := null
    unsafe
        delete astVisitor

def aggresiveInScope ( t : TypeDeclPtr; var dep : table<Structure?> ) : bool
    if t.baseType == Type tStructure
        if dep |> key_exists(t.structType)
            return false
        dep |> insert(t.structType)
        for fld in t.structType.fields
            if fld._type |> aggresiveInScope(dep)
                return true
    elif t.baseType == Type tTuple || t.baseType == Type tVariant || t.baseType == Type option
        for arg in t.argTypes
            if arg |> aggresiveInScope(dep)
                return true
    elif t.baseType == Type tArray || t.baseType == Type tTable
        return true // any array or table needs to be in scope
    elif t.baseType == Type tHandle
        return false // TOOD: expose "needInScope" field of the type, and use that
    elif t.baseType == Type tPointer
        return t.flags.smartPtr
    return false

def aggresiveInScope ( t : TypeDeclPtr )
    var inscope dep : table<Structure?>
    return t |> aggresiveInScope(dep)
