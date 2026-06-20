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
    // for fields: struct identity + name
    field_struct_ptr : void?
    field_name : string
    // for aliases: the alias name string
    alias_name : string
    // display name
    name : string
}

def to_void(p) : void? {
    return unsafe(reinterpret<void?> p)
}

// Build a RefTarget from a cursor hit
def identify_target(hit : CursorHit) : RefTarget {
    let expr = hit.expr
    // ExprCall → function
    if (expr is ExprCall) {
        let c = expr as ExprCall
        if (c.func != null) return RefTarget(kind = RefTargetKind.func, target_ptr = to_void(c.func), name = string(c.func.name))
    }
    // ExprAddr → function pointer
    if (expr is ExprAddr) {
        let a = expr as ExprAddr
        if (a.func != null) return RefTarget(kind = RefTargetKind.func, target_ptr = to_void(a.func), name = string(a.func.name))
    }
    // ExprInvoke → resolve the callable (first argument)
    if (expr is ExprInvoke) {
        let inv = expr as ExprInvoke
        if (!empty(inv.arguments)) {
            let callable = inv.arguments[0]
            // first arg is a variable (function pointer parameter/local)
            if (callable is ExprVar) {
                let v = callable as ExprVar
                if (v.variable != null) {
                    let variable = v.variable
                    return RefTarget(kind = RefTargetKind.variable, target_ptr = to_void(variable), name = string(variable.name))
                }
            }
            // first arg is a field access (struct method pointer)
            if (callable is ExprField) {
                let f = callable as ExprField
                if (f.value != null && f.value._type != null) {
                    var vt = f.value._type
                    if (vt.structType == null && vt.firstType != null) {
                        vt = vt.firstType
                    }
                    if (vt.structType != null) return RefTarget(
                            kind = RefTargetKind.field,
                            field_struct_ptr = to_void(vt.structType),
                            field_name = string(f.name),
                            name = "{vt.structType.name}.{f.name}"
                        )
                }
            }
        }
    }
    // ExprVar → variable
    if (expr is ExprVar) {
        let v = expr as ExprVar
        if (v.variable != null) {
            let variable = v.variable
            return RefTarget(kind = RefTargetKind.variable, target_ptr = to_void(variable), name = string(variable.name))
        }
    }
    // ExprField / ExprSafeField → field
    if (expr is ExprField || expr is ExprSafeField) {
        var fname : string
        var value_type : TypeDecl const?
        if (expr is ExprField) {
            let f = expr as ExprField
            fname = string(f.name)
            if (f.value != null && f.value._type != null) {
                value_type = f.value._type
            }
        } else {
            let f = expr as ExprSafeField
            fname = string(f.name)
            if (f.value != null && f.value._type != null) {
                value_type = f.value._type
            }
        }
        // unwrap pointer types (Foo? → Foo)
        if (value_type != null && value_type.structType == null && value_type.firstType != null) {
            value_type = value_type.firstType
        }
        if (value_type != null && value_type.structType != null) return RefTarget(
                kind = RefTargetKind.field,
                field_struct_ptr = to_void(value_type.structType),
                field_name = fname,
                name = "{value_type.structType.name}.{fname}"
            )
    }
    // ExprConstEnumeration → enum
    if (expr is ExprConstEnumeration) {
        let e = expr as ExprConstEnumeration
        if (e.enumType != null) return RefTarget(kind = RefTargetKind.enumeration, target_ptr = to_void(e.enumType), name = string(e.enumType.name))
    }
    // ExprConstBitfield → alias (bitfield type)
    if (expr is ExprConstBitfield) {
        if (expr._type != null) {
            let td = expr._type
            if (!empty(td.alias)) return RefTarget(kind = RefTargetKind.alias, alias_name = string(td.alias), name = string(td.alias))
        }
    }
    // Type alias reference (typedef, bitfield, variant, tuple)
    if (expr._type != null) {
        let td = expr._type
        if (!empty(td.alias)) return RefTarget(kind = RefTargetKind.alias, alias_name = string(td.alias), name = string(td.alias))
    }
    // Fallback: type reference
    if (expr._type != null) {
        let td = expr._type
        if (td.structType != null) return RefTarget(kind = RefTargetKind.structure, target_ptr = to_void(td.structType), name = string(td.structType.name))
        if (td.enumType != null) return RefTarget(kind = RefTargetKind.enumeration, target_ptr = to_void(td.enumType), name = string(td.enumType.name))
    }
    return RefTarget()
}

// Visitor that collects all references to a given target
class RefVisitor : AstVisitor {
    target : RefTarget
    refs : array<Reference>
    currentFunc : Function const?
    file_filter : string   // if non-empty, only collect refs from this file

    def RefVisitor(t : RefTarget; file_flt : string) {
        target = t
        currentFunc = null
        file_filter = file_flt
    }

    def add_ref(at : LineInfo; context : string) : void {
        if (int(at.line) == 0) return
        // file filter
        if (!empty(file_filter) && at.fileInfo != null) {
            let fname = string(at.fileInfo.name)
            if (find(fname, file_filter) < 0) return
        }
        refs |> emplace(Reference(
            file = at.fileInfo != null ? string(at.fileInfo.name) : "",
            line = int(at.line),
            column = int(at.column),
            in_function = currentFunc != null ? string(currentFunc.name) : "",
            context = context
        ))
    }

    def override preVisitFunction(fun : FunctionPtr) : void {
        currentFunc = fun
    }

    def override visitFunction(var fun : FunctionPtr) : FunctionPtr {
        currentFunc = null
        return <- fun
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

    // ExprField → field reference
    def override preVisitExprField(expr : ExprField?) : void {
        if (target.kind == RefTargetKind.field) {
            if (expr.name == target.field_name && expr.value != null && expr.value._type != null) {
                var vt = expr.value._type
                if (vt.structType == null && vt.firstType != null) {
                    vt = vt.firstType
                }
                if (vt.structType != null && to_void(vt.structType) == target.field_struct_ptr) {
                    add_ref(expr.atField, "field")
                }
            }
        }
    }

    // ExprSafeField → field reference (?.field)
    def override preVisitExprSafeField(expr : ExprSafeField?) : void {
        if (target.kind == RefTargetKind.field) {
            if (expr.name == target.field_name && expr.value != null && expr.value._type != null) {
                var vt = expr.value._type
                if (vt.structType == null && vt.firstType != null) {
                    vt = vt.firstType
                }
                if (vt.structType != null && to_void(vt.structType) == target.field_struct_ptr) {
                    add_ref(expr.atField, "safe field")
                }
            }
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
                add_ref(td.at, "type ref")
            }
        }
        if (target.kind == RefTargetKind.enumeration && td.enumType != null) {
            if (to_void(td.enumType) == target.target_ptr) {
                add_ref(td.at, "type ref")
            }
        }
        if (target.kind == RefTargetKind.alias && !empty(td.alias)) {
            if (td.alias == target.alias_name) {
                add_ref(td.at, "alias ref")
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

def private fileinfo_name_contains(name : das_string; needle : string) : bool {
    var hit = false
    peek(name) $(s) {
        hit = find(s, needle) >= 0
    }
    return hit
}

// Try to identify a target from declarations at the cursor line
// (function defs, struct defs, enum defs — not expressions)
def identify_declaration_target(program : smart_ptr<Program>; file : string; line : int) : RefTarget {
    let thisMod = get_this_module(program)
    var result : RefTarget
    // check function declarations
    ast::for_each_function(thisMod, "") $(var func : FunctionPtr) {
        if (result.kind != RefTargetKind.none) return
        let f = func
        if (int(f.at.line) == line && f.at.fileInfo != null) {
            if (fileinfo_name_contains(f.at.fileInfo.name, file)) {
                result = RefTarget(kind = RefTargetKind.func, target_ptr = to_void(f), name = string(f.name))
            }
        }
    }
    if (result.kind != RefTargetKind.none) return result
    // check structure declarations
    ast::for_each_structure(thisMod) $(var st : StructurePtr) {
        if (result.kind != RefTargetKind.none) return
        let s = st
        if (int(s.at.line) == line && s.at.fileInfo != null) {
            if (fileinfo_name_contains(s.at.fileInfo.name, file)) {
                result = RefTarget(kind = RefTargetKind.structure, target_ptr = to_void(s), name = string(s.name))
            }
        }
    }
    if (result.kind != RefTargetKind.none) return result
    // check enumeration declarations
    ast::for_each_enumeration(thisMod) $(var en : EnumerationPtr) {
        if (result.kind != RefTargetKind.none) return
        let e = en
        if (int(e.at.line) == line && e.at.fileInfo != null) {
            if (fileinfo_name_contains(e.at.fileInfo.name, file)) {
                result = RefTarget(kind = RefTargetKind.enumeration, target_ptr = to_void(e), name = string(e.name))
            }
        }
    }
    if (result.kind != RefTargetKind.none) return result
    // check typedef declarations (typedef, bitfield, variant, tuple)
    ast::for_each_typedef(thisMod) $(name : string#; var value : TypeDeclPtr) {
        if (result.kind != RefTargetKind.none) return
        let td = value
        if (int(td.at.line) == line && td.at.fileInfo != null) {
            if (fileinfo_name_contains(td.at.fileInfo.name, file)) {
                result = RefTarget(kind = RefTargetKind.alias, alias_name = string(name), name = string(name))
            }
        }
    }
    if (result.kind != RefTargetKind.none) return result
    // check global variable declarations
    ast::for_each_global(thisMod) $(var value : VariablePtr) {
        if (result.kind != RefTargetKind.none) return
        let v = value
        if (int(v.at.line) == line && v.at.fileInfo != null) {
            if (fileinfo_name_contains(v.at.fileInfo.name, file)) {
                result = RefTarget(kind = RefTargetKind.variable, target_ptr = to_void(v), name = string(v.name))
            }
        }
    }
    return result
}

def run_find_references(file : string; line_str, col_str, scope_str, no_opt_str : string; project : string = "") : string {
    let proj_err = validate_project_arg(project)
    if (!empty(proj_err)) return make_tool_result(proj_err, true)
    let line = to_int(line_str)
    let col = to_int(col_str)
    if (line <= 0 || col <= 0) return make_tool_result("Invalid line or column (must be >= 1)", true)
    let all_modules = scope_str == "all"
    let no_opt = no_opt_str == "true"
    return compile_program(file, true, no_opt, project) $(program; _issues) {
        // Step 1: identify the target from cursor (expression-based)
        var target : RefTarget
        var hits <- find_at_cursor(program, file, line, col)
        if (!empty(hits)) {
            for (hit in hits) {
                target = identify_target(hit)
                if (target.kind != RefTargetKind.none) break
            }
        }
        // Step 1b: if no expression found, try declaration-based lookup
        if (target.kind == RefTargetKind.none) {
            target = identify_declaration_target(program, file, line)
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
        // Step 2: walk AST to find all references
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
