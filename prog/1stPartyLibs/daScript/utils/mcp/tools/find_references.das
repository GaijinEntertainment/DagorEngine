options gen2
options no_unused_function_arguments = false
options no_unused_block_arguments = false

require common public
require daslib/ast_cursor
require daslib/ast_boost

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
        if (c.func != null) {
            return RefTarget(kind = RefTargetKind.func, target_ptr = to_void(c.func), name = string(c.func.name))
        }
    }
    // ExprAddr → function pointer
    if (expr is ExprAddr) {
        let a = expr as ExprAddr
        if (a.func != null) {
            return RefTarget(kind = RefTargetKind.func, target_ptr = to_void(a.func), name = string(a.func.name))
        }
    }
    // ExprInvoke → resolve the callable (first argument)
    if (expr is ExprInvoke) {
        let inv = expr as ExprInvoke
        if (length(inv.arguments) > 0) {
            let callable = get_ptr(inv.arguments[0])
            // first arg is a variable (function pointer parameter/local)
            if (callable is ExprVar) {
                let v = callable as ExprVar
                if (v.variable != null) {
                    let variable = get_ptr(v.variable)
                    return RefTarget(kind = RefTargetKind.variable, target_ptr = to_void(variable), name = string(variable.name))
                }
            }
            // first arg is a field access (struct method pointer)
            if (callable is ExprField) {
                let f = callable as ExprField
                if (f.value != null && f.value._type != null) {
                    var vt = get_ptr(f.value._type)
                    if (vt.structType == null && vt.firstType != null) {
                        vt = get_ptr(vt.firstType)
                    }
                    if (vt.structType != null) {
                        return RefTarget(
                            kind = RefTargetKind.field,
                            field_struct_ptr = to_void(vt.structType),
                            field_name = string(f.name),
                            name = "{vt.structType.name}.{f.name}"
                        )
                    }
                }
            }
        }
    }
    // ExprVar → variable
    if (expr is ExprVar) {
        let v = expr as ExprVar
        if (v.variable != null) {
            let variable = get_ptr(v.variable)
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
                value_type = get_ptr(f.value._type)
            }
        } else {
            let f = expr as ExprSafeField
            fname = string(f.name)
            if (f.value != null && f.value._type != null) {
                value_type = get_ptr(f.value._type)
            }
        }
        // unwrap pointer types (Foo? → Foo)
        if (value_type != null && value_type.structType == null && value_type.firstType != null) {
            value_type = get_ptr(value_type.firstType)
        }
        if (value_type != null && value_type.structType != null) {
            return RefTarget(
                kind = RefTargetKind.field,
                field_struct_ptr = to_void(value_type.structType),
                field_name = fname,
                name = "{value_type.structType.name}.{fname}"
            )
        }
    }
    // ExprConstEnumeration → enum
    if (expr is ExprConstEnumeration) {
        let e = expr as ExprConstEnumeration
        if (e.enumType != null) {
            let en = get_ptr(e.enumType)
            return RefTarget(kind = RefTargetKind.enumeration, target_ptr = to_void(en), name = string(en.name))
        }
    }
    // ExprConstBitfield → alias (bitfield type)
    if (expr is ExprConstBitfield) {
        if (expr._type != null) {
            let td = get_ptr(expr._type)
            if (!empty(td.alias)) {
                return RefTarget(kind = RefTargetKind.alias, alias_name = string(td.alias), name = string(td.alias))
            }
        }
    }
    // Type alias reference (typedef, bitfield, variant, tuple)
    if (expr._type != null) {
        let td = get_ptr(expr._type)
        if (!empty(td.alias)) {
            return RefTarget(kind = RefTargetKind.alias, alias_name = string(td.alias), name = string(td.alias))
        }
    }
    // Fallback: type reference
    if (expr._type != null) {
        let td = get_ptr(expr._type)
        if (td.structType != null) {
            return RefTarget(kind = RefTargetKind.structure, target_ptr = to_void(td.structType), name = string(td.structType.name))
        }
        if (td.enumType != null) {
            return RefTarget(kind = RefTargetKind.enumeration, target_ptr = to_void(td.enumType), name = string(td.enumType.name))
        }
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
        if (int(at.line) == 0) {
            return
        }
        // file filter
        if (!empty(file_filter) && at.fileInfo != null) {
            let fname = string(at.fileInfo.name)
            if (find(fname, file_filter) < 0) {
                return
            }
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
        currentFunc = get_ptr(fun)
    }

    def override visitFunction(var fun : FunctionPtr) : FunctionPtr {
        currentFunc = null
        return <- fun
    }

    // ExprCall → function reference
    def override preVisitExprCall(expr : smart_ptr<ExprCall>) : void {
        if (target.kind == RefTargetKind.func) {
            let c = get_ptr(expr)
            if (c.func != null && to_void(c.func) == target.target_ptr) {
                add_ref(c.at, "call")
            }
        }
    }

    // ExprAddr → function pointer reference (@@func)
    def override preVisitExprAddr(expr : smart_ptr<ExprAddr>) : void {
        if (target.kind == RefTargetKind.func) {
            let a = get_ptr(expr)
            if (a.func != null && to_void(a.func) == target.target_ptr) {
                add_ref(a.at, "addr")
            }
        }
    }

    // ExprVar → variable reference
    def override preVisitExprVar(expr : smart_ptr<ExprVar>) : void {
        if (target.kind == RefTargetKind.variable) {
            let v = get_ptr(expr)
            if (v.variable != null) {
                let vptr = get_ptr(v.variable)
                if (to_void(vptr) == target.target_ptr) {
                    add_ref(v.at, "variable")
                }
            }
        }
    }

    // ExprField → field reference
    def override preVisitExprField(expr : smart_ptr<ExprField>) : void {
        if (target.kind == RefTargetKind.field) {
            let f = get_ptr(expr)
            if (f.name == target.field_name && f.value != null && f.value._type != null) {
                var vt = get_ptr(f.value._type)
                if (vt.structType == null && vt.firstType != null) {
                    vt = get_ptr(vt.firstType)
                }
                if (vt.structType != null && to_void(vt.structType) == target.field_struct_ptr) {
                    add_ref(f.atField, "field")
                }
            }
        }
    }

    // ExprSafeField → field reference (?.field)
    def override preVisitExprSafeField(expr : smart_ptr<ExprSafeField>) : void {
        if (target.kind == RefTargetKind.field) {
            let f = get_ptr(expr)
            if (f.name == target.field_name && f.value != null && f.value._type != null) {
                var vt = get_ptr(f.value._type)
                if (vt.structType == null && vt.firstType != null) {
                    vt = get_ptr(vt.firstType)
                }
                if (vt.structType != null && to_void(vt.structType) == target.field_struct_ptr) {
                    add_ref(f.atField, "safe field")
                }
            }
        }
    }

    // ExprConstEnumeration → enum value reference
    def override preVisitExprConstEnumeration(expr : smart_ptr<ExprConstEnumeration>) : void {
        if (target.kind == RefTargetKind.enumeration) {
            let e = get_ptr(expr)
            if (e.enumType != null && to_void(get_ptr(e.enumType)) == target.target_ptr) {
                add_ref(e.at, "enum value")
            }
        }
    }

    // ExprConstBitfield → bitfield value reference
    def override preVisitExprConstBitfield(expr : smart_ptr<ExprConstBitfield>) : void {
        if (target.kind == RefTargetKind.alias) {
            let e = get_ptr(expr)
            if (e._type != null && !empty(e._type.alias) && e._type.alias == target.alias_name) {
                add_ref(e.at, "bitfield value")
            }
        }
    }

    // TypeDecl → struct/enum/alias type references (in variable types, return types, etc.)
    def override preVisitTypeDecl(typ : TypeDeclPtr) : void {
        let td = get_ptr(typ)
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
    return build_string() <| $(var w) {
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

// Try to identify a target from declarations at the cursor line
// (function defs, struct defs, enum defs — not expressions)
def identify_declaration_target(program : smart_ptr<Program>; file : string; line : int) : RefTarget {
    let thisMod = get_this_module(program)
    var result : RefTarget
    // check function declarations
    ast::for_each_function(thisMod, "") <| $(var func : FunctionPtr) {
        if (result.kind != RefTargetKind.none) {
            return
        }
        let f = get_ptr(func)
        if (int(f.at.line) == line && f.at.fileInfo != null) {
            if (find(string(f.at.fileInfo.name), file) >= 0) {
                result = RefTarget(kind = RefTargetKind.func, target_ptr = to_void(f), name = string(f.name))
            }
        }
    }
    if (result.kind != RefTargetKind.none) {
        return result
    }
    // check structure declarations
    ast::for_each_structure(thisMod) <| $(var st : StructurePtr) {
        if (result.kind != RefTargetKind.none) {
            return
        }
        let s = get_ptr(st)
        if (int(s.at.line) == line && s.at.fileInfo != null) {
            if (find(string(s.at.fileInfo.name), file) >= 0) {
                result = RefTarget(kind = RefTargetKind.structure, target_ptr = to_void(s), name = string(s.name))
            }
        }
    }
    if (result.kind != RefTargetKind.none) {
        return result
    }
    // check enumeration declarations
    ast::for_each_enumeration(thisMod) <| $(var en : EnumerationPtr) {
        if (result.kind != RefTargetKind.none) {
            return
        }
        let e = get_ptr(en)
        if (int(e.at.line) == line && e.at.fileInfo != null) {
            if (find(string(e.at.fileInfo.name), file) >= 0) {
                result = RefTarget(kind = RefTargetKind.enumeration, target_ptr = to_void(e), name = string(e.name))
            }
        }
    }
    if (result.kind != RefTargetKind.none) {
        return result
    }
    // check typedef declarations (typedef, bitfield, variant, tuple)
    ast::for_each_typedef(thisMod) <| $(name : string#; var value : TypeDeclPtr) {
        if (result.kind != RefTargetKind.none) {
            return
        }
        let td = get_ptr(value)
        if (int(td.at.line) == line && td.at.fileInfo != null) {
            if (find(string(td.at.fileInfo.name), file) >= 0) {
                result = RefTarget(kind = RefTargetKind.alias, alias_name = string(name), name = string(name))
            }
        }
    }
    if (result.kind != RefTargetKind.none) {
        return result
    }
    // check global variable declarations
    ast::for_each_global(thisMod) <| $(var value : VariablePtr) {
        if (result.kind != RefTargetKind.none) {
            return
        }
        let v = get_ptr(value)
        if (int(v.at.line) == line && v.at.fileInfo != null) {
            if (find(string(v.at.fileInfo.name), file) >= 0) {
                result = RefTarget(kind = RefTargetKind.variable, target_ptr = to_void(v), name = string(v.name))
            }
        }
    }
    return result
}

def public do_find_references(file : string; line_str, col_str, scope_str : string) : string {
    return do_find_references(file, line_str, col_str, scope_str, "")
}

def public do_find_references(file : string; line_str, col_str, scope_str, no_opt_str : string; project : string = "") : string {
    let line = to_int(line_str)
    let col = to_int(col_str)
    if (line <= 0 || col <= 0) {
        return make_tool_result("Invalid line or column (must be >= 1)", true)
    }
    let all_modules = scope_str == "all"
    let no_opt = no_opt_str == "true"
    return compile_program(file, true, no_opt, project) <| $(program; issues) {
        // Step 1: identify the target from cursor (expression-based)
        var target : RefTarget
        var hits <- find_at_cursor(program, file, line, col)
        if (!empty(hits)) {
            for (hit in hits) {
                target = identify_target(hit)
                if (target.kind != RefTargetKind.none) {
                    break
                }
            }
        }
        // Step 1b: if no expression found, try declaration-based lookup
        if (target.kind == RefTargetKind.none) {
            target = identify_declaration_target(program, file, line)
        }
        if (target.kind == RefTargetKind.none) {
            if (empty(hits)) {
                return "No expression or declaration found at {file}:{line}:{col}"
            }
            return build_string() <| $(var w) {
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
        var inscope adapter <- make_visitor(*visitor)
        if (all_modules) {
            visit_modules(program, adapter)
        } else {
            visit(program, adapter)
        }
        let result = format_references(visitor.refs, target)
        unsafe {
            delete visitor
        }
        return result
    }
}
