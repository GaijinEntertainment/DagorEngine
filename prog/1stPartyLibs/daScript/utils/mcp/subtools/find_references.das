options gen2
options rtti
options indenting = 4

require ../tools/common.das public
require daslib/ast_cursor
require daslib/ast_boost

//! Subprocess form of find_references. Argv (after daslang exe + script path):
//!   <file> <line> <col> <scope> <no_opt> <project>
//! Prints the make_tool_result envelope to stdout.

// A single reference location
struct Reference {
    file : string
    line : int
    column : int
    in_function : string   // enclosing function name, or "" for global
    context : string       // short description: "call", "variable", "field", "type ref", "addr", etc.
}

// What we're looking for
enum RefTargetKind {
    none
    func
    variable
    structure
    field
    enumeration
    alias
}

struct RefTarget {
    kind : RefTargetKind
    // all pointers stored as void? for identity comparison (avoids const mismatch)
    target_ptr : void?
    field_name : string
    // for aliases: the alias name string
    alias_name : string
    // display name
    name : string
    @safe_when_uninitialized decl_at : LineInfo   //! the declaration's own position when known: a site there is its binding, not a use; for a field it is the identity - the parent's line every class that inherits it shares
}

def position_key(at : LineInfo) : string {
    let file = at.fileInfo != null ? string(at.fileInfo.name) : ""
    return "{file}:{int(at.line)}:{int(at.column)}"
}

def to_void(p) : void? {
    return unsafe(reinterpret<void?>(p))
}

def variable_target(variable : Variable const?) : RefTarget {
    return RefTarget(kind = RefTargetKind.variable, target_ptr = to_void(variable), name = source_name(string(variable.name)))
}

//! the field `fname` of `st` as a target; a class method field is its function instead
def field_target(st : Structure const?; fname : string) : RefTarget {
    let method = find_method_function(st, fname)
    if (method != null) return RefTarget(kind = RefTargetKind.func, target_ptr = to_void(method), name = string(method.name), decl_at = method.at)
    return RefTarget(kind = RefTargetKind.field, field_name = fname, name = "{st.name}.{fname}", decl_at = field_at(st, fname))
}

def field_target_of(value : Expression const?; fname : string) : RefTarget {
    let st = value_struct(value)
    return st != null ? field_target(st, fname) : RefTarget()
}

// ExprCall / ExprAddr / ExprInvoke → function, function-pointer variable, or method field
def identify_callable_target(expr : Expression const?) : RefTarget {
    if (expr is ExprCall) {
        let c = expr as ExprCall
        if (c.func != null) return RefTarget(kind = RefTargetKind.func, target_ptr = to_void(c.func), name = string(c.func.name), decl_at = c.func.at)
    }
    if (expr is ExprAddr) {
        let a = expr as ExprAddr
        if (a.func != null) return RefTarget(kind = RefTargetKind.func, target_ptr = to_void(a.func), name = string(a.func.name), decl_at = a.func.at)
    }
    // ExprInvoke → resolve the callable (first argument)
    if (expr is ExprInvoke) {
        let inv = expr as ExprInvoke
        if (!empty(inv.arguments)) {
            let callable = inv.arguments[0]
            // first arg is a variable (function pointer parameter/local)
            if (callable is ExprVar) {
                let v = callable as ExprVar
                if (v.variable != null) return variable_target(v.variable)
            }
            // first arg is a field access (struct method pointer)
            if (callable is ExprField) {
                let f = callable as ExprField
                return field_target_of(f.value, string(f.name))
            }
        }
    }
    return RefTarget()
}

// ExprVar / ExprField / ExprSafeField → variable or field
def identify_value_target(expr : Expression const?) : RefTarget {
    if (expr is ExprVar) {
        let v = expr as ExprVar
        if (v.variable != null) return variable_target(v.variable)
    }
    if (expr is ExprField) {
        let f = expr as ExprField
        return field_target_of(f.value, string(f.name))
    }
    if (expr is ExprSafeField) {
        let f = expr as ExprSafeField
        return field_target_of(f.value, string(f.name))
    }
    return RefTarget()
}

// Enum constants, type aliases (typedef, bitfield, variant, tuple), and fallback type references
def identify_type_target(program : smart_ptr<Program>; expr : Expression const?) : RefTarget {
    if (expr is ExprConstEnumeration) {
        let e = expr as ExprConstEnumeration
        if (e.enumType != null) return RefTarget(kind = RefTargetKind.enumeration, target_ptr = to_void(e.enumType), name = string(e.enumType.name))
    }
    if (expr._type != null) return declaration_target(declaration_of(program, expr._type))
    return RefTarget()
}

def declaration_target(d : DeclarationHit) : RefTarget {
    if (d.kind == DeclarationKind.func) return RefTarget(kind = RefTargetKind.func, target_ptr = to_void(d.func), name = string(d.func.name), decl_at = d.at)
    if (d.kind == DeclarationKind.structure) return RefTarget(kind = RefTargetKind.structure, target_ptr = to_void(d.structure), name = d.name, decl_at = d.at)
    if (d.kind == DeclarationKind.enumeration) return RefTarget(kind = RefTargetKind.enumeration, target_ptr = to_void(d.enumeration), name = d.name, decl_at = d.at)
    if (d.kind == DeclarationKind.alias) return RefTarget(kind = RefTargetKind.alias, alias_name = d.name, name = d.name, decl_at = d.at)
    if (d.kind == DeclarationKind.variable) return RefTarget(kind = RefTargetKind.variable, target_ptr = to_void(d.variable), name = d.name, decl_at = d.at)
    if (d.kind == DeclarationKind.field) return field_target(d.structure, d.name)
    return RefTarget()
}

def identify_target(program : smart_ptr<Program>; hit : CursorHit) : RefTarget {
    if (hit.variable != null) return variable_target(hit.variable)
    if (hit.type_decl != null) return declaration_target(declaration_of(program, hit.type_decl))
    let expr = hit.expr
    var t = identify_callable_target(expr)
    if (t.kind == RefTargetKind.none) {
        t = identify_value_target(expr)
    }
    if (t.kind == RefTargetKind.none) {
        t = identify_type_target(program, expr)
    }
    return t
}

// Visitor that collects all references to a given target
class RefVisitor : AstVisitor {
    target : RefTarget = RefTarget()
    refs : array<Reference>
    currentFunc : Function const?
    in_synthesized_member : bool
    file_filter : string   // if non-empty, only collect refs from this file
    seen : table<string>   //! one site per position: a `new T()` carries T's type on several nodes

    def RefVisitor(t : RefTarget; file_flt : string) {
        target = t
        currentFunc = null
        file_filter = file_flt
        if (int(t.decl_at.line) > 0) {
            seen |> insert(position_key(t.decl_at))
        }
    }

    def add_ref(at : LineInfo; context : string) : void {
        if (in_synthesized_member || int(at.line) == 0) return
        // file filter
        if (!empty(file_filter) && at.fileInfo != null) {
            let fname = string(at.fileInfo.name)
            if (find(fname, file_filter) < 0) return
        }
        let file = at.fileInfo != null ? string(at.fileInfo.name) : ""
        let key = position_key(at)
        return if (key_exists(seen, key))
        seen |> insert(key)
        refs |> emplace(Reference(
            file = file,
            line = int(at.line),
            column = int(at.column),
            in_function = currentFunc != null ? string(currentFunc.name) : "",
            context = context
        ))
    }

    def add_type_ref(at : LineInfo; name : string; context : string) : void {
        return if (at.fileInfo == null || !spells_name(source_line(string(at.fileInfo.name), int(at.line)), int(at.column), name))
        add_ref(at, context)
    }

    def override preVisitFunction(fun : FunctionPtr) : void {
        currentFunc = fun
        in_synthesized_member = is_synthesized(fun)
    }

    def override visitFunction(var fun : FunctionPtr) : FunctionPtr {
        currentFunc = null
        in_synthesized_member = false
        return <- fun
    }

    def override preVisitStructure(str : StructurePtr) : void {
        return if (target.kind != RefTargetKind.structure || str.parent == null || to_void(str.parent) != target.target_ptr || int(str.at.line) == 0 || str.at.fileInfo == null)
        var at = str.at
        at.column = uint(declared_name_column(at, target.name))
        add_ref(at, "parent")
    }

    // ExprCall → function reference
    def override preVisitExprCall(expr : ExprCall?) : void {
        if (target.kind == RefTargetKind.func) {
            if (expr.func != null && to_void(expr.func) == target.target_ptr) {
                add_ref(expr.at, "call")
            }
        }
    }

    // ExprAddr → function pointer reference (@@func)
    def override preVisitExprAddr(expr : ExprAddr?) : void {
        if (target.kind == RefTargetKind.func) {
            if (expr.func != null && to_void(expr.func) == target.target_ptr) {
                add_ref(expr.at, "addr")
            }
        }
    }

    // ExprVar → variable reference
    def override preVisitExprVar(expr : ExprVar?) : void {
        if (target.kind == RefTargetKind.variable) {
            if (expr.variable != null) {
                let vptr = expr.variable
                if (to_void(vptr) == target.target_ptr) {
                    add_ref(expr.at, "variable")
                }
            }
        }
    }

    // a field is one declaration across the class hierarchy: a derived class's copy keeps the parent's position; a method field's sites reference its function
    def on_target_field(value : Expression const?; name : string) : bool {
        let st = value_struct(value)
        return false if (st == null)
        if (target.kind == RefTargetKind.func) return to_void(find_method_function(st, name)) == target.target_ptr
        return target.kind == RefTargetKind.field && name == target.field_name && position_key(field_at(st, name)) == position_key(target.decl_at)
    }

    def override preVisitExprField(expr : ExprField?) : void {
        if (on_target_field(expr.value, string(expr.name))) {
            add_ref(expr.atField, target.kind == RefTargetKind.func ? "method call" : "field")
        }
    }

    def override preVisitExprSafeField(expr : ExprSafeField?) : void {
        if (on_target_field(expr.value, string(expr.name))) {
            add_ref(expr.atField, target.kind == RefTargetKind.func ? "method call" : "safe field")
        }
    }

    // ExprConstEnumeration → enum value reference
    def override preVisitExprConstEnumeration(expr : ExprConstEnumeration?) : void {
        if (target.kind == RefTargetKind.enumeration) {
            if (expr.enumType != null && to_void(expr.enumType) == target.target_ptr) {
                add_ref(expr.at, "enum value")
            }
        }
    }

    // ExprConstBitfield → bitfield value reference
    def override preVisitExprConstBitfield(expr : ExprConstBitfield?) : void {
        if (target.kind == RefTargetKind.alias) {
            if (expr._type != null && !empty(expr._type.alias) && expr._type.alias == target.alias_name) {
                add_ref(expr.at, "bitfield value")
            }
        }
    }

    // TypeDecl → struct/enum/alias type references (in variable types, return types, etc.)
    def override preVisitTypeDecl(typ : TypeDeclPtr) : void {
        let td = typ
        if (target.kind == RefTargetKind.structure && td.structType != null) {
            if (to_void(td.structType) == target.target_ptr) {
                add_type_ref(td.at, target.name, "type ref")
            }
        }
        if (target.kind == RefTargetKind.enumeration && td.enumType != null) {
            if (to_void(td.enumType) == target.target_ptr) {
                add_type_ref(td.at, target.name, "type ref")
            }
        }
        if (target.kind == RefTargetKind.alias && !empty(td.alias)) {
            if (td.alias == target.alias_name) {
                add_type_ref(td.at, target.alias_name, "alias ref")
            }
        }
    }
}

def format_references(refs : array<Reference>; target : RefTarget) : string {
    return build_string() $(var w) {
        write(w, "Found {length(refs)} reference(s) to {target.kind} '{target.name}':\n")
        for (r in refs) {
            write(w, "  {r.file}:{r.line}:{r.column}")
            if (!empty(r.in_function)) {
                write(w, " in {r.in_function}")
            }
            write(w, " [{r.context}]\n")
        }
    }
}

def run_find_references(file : string; line_str, col_str, scope_str, no_opt_str : string; project : string = "") : string {
    let proj_err = validate_project_arg(project)
    if (!empty(proj_err)) return make_tool_result(proj_err, true)
    let line = to_int(line_str)
    let col = to_int(col_str)
    let col0 = col - 1
    if (line <= 0 || col0 < 0) return make_tool_result("Invalid line or column (must be >= 1)", true)
    let all_modules = scope_str == "all"
    let no_opt = no_opt_str == "true"
    return compile_program(file, true, no_opt, project) $(program; _issues) {
        var target = declaration_target(find_declaration_at_cursor(program, file, line, col0, source_line(file, line)))
        var hits <- find_at_cursor(program, file, line, col0, true, source_line(file, line))
        if (target.kind == RefTargetKind.none) {
            for (hit in hits) {
                target = identify_target(program, hit)
                if (target.kind != RefTargetKind.none) break
            }
        }
        if (target.kind == RefTargetKind.none) {
            target = declaration_target(find_declaration_at_cursor(program, file, line, col0, ""))
        }
        if (target.kind == RefTargetKind.none) {
            if (empty(hits)) return "No expression or declaration found at {file}:{line}:{col}"
            return build_string() $(var w) {
                write(w, "Could not identify a referenceable symbol at {file}:{line}:{col}\n")
                write(w, "Found expressions:\n")
                for (hit in hits) {
                    write(w, "  {describe(hit)}\n")
                }
            }
        }
        let file_filter = all_modules ? "" : file
        var visitor = new RefVisitor(target, file_filter)
        make_visitor(*visitor) $(adapter) {
            if (all_modules) {
                visit_modules(program, adapter)
            } else {
                visit(program, adapter)
            }
        }
        let result = format_references(visitor.refs, target)
        unsafe {
            delete visitor
        }
        return result
    }
}

[export]
def main {
    let raw <- get_command_line_arguments()
    let args <- subtool_user_args(raw)
    if (length(args) < 6) {
        print(make_tool_result("find_references subtool: expected 6 args (file, line, col, scope, no_opt, project), got {length(args)}", true))
        return
    }
    let file = args[0]
    let line_str = args[1]
    let col_str = args[2]
    let scope_str = args[3]
    let no_opt_str = args[4]
    let project = args[5]
    print(run_find_references(file, line_str, col_str, scope_str, no_opt_str, project))
}
