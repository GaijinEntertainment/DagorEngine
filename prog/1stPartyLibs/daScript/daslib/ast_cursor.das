options gen2
options indenting = 4
options strict_smart_pointers = true

//! ast_cursor provides position-based AST queries — given a file, line, and column,
//! it finds the expression nodes at that cursor position, and on request the variable
//! declarations. Results are returned innermost-first, with enclosing function context
//! and extracted names.
//!
//! Typical use is implementing IDE features like goto-definition, type-of, and
//! find-references on top of the compiled AST.

module ast_cursor shared public

require daslib/ast_boost
require daslib/utf8_utils
require strings

struct public CursorHit {
    //! A single hit at the cursor position: an expression node, or when asked for a variable declaration or a type name. Innermost hits come first.
    //! Raw pointer to the expression node (valid while the program lives); null for a variable or type hit.
    expr : Expression const?
    //! The variable whose declaration the cursor is on (`for`/`let` variable, function, lambda or block argument) when declarations were asked for; else null.
    variable : Variable const?
    //! The type the cursor's type name denotes (a struct, enum or alias spelled in the source) when declarations were asked for; else null.
    type_decl : TypeDecl const?
    //! Enclosing function, or null for expressions in global scope.
    func : Function const?
    //! Extracted name for named nodes (variable name, call name, field name, operator); the type's name for a type hit.
    name : string
    //! Expression RTTI type string: "ExprVar", "ExprCall", "ExprField", etc.; "Variable" or "TypeDecl" for the other hits.
    rtti : string
}

def public hit_at(hit : CursorHit) : LineInfo {
    //! The position of the node the hit holds: its expression, variable or type.
    if (hit.expr != null) return hit.expr.at
    if (hit.variable != null) return hit.variable.at
    return hit.type_decl.at
}

def public cursor_inside(line, col : int; at : LineInfo) : bool {
    //! Check whether a point (line, col) is inside the span described by `at`.
    //! Follows the same logic as C++ ``LineInfo::inside``.
    if (int(at.line) == 0 && int(at.last_line) == 0) return false
    let tl = int(at.line)
    let tc = int(at.column)
    let tll = int(at.last_line)
    let tlc = int(at.last_column)
    if (line < tl || line > tll) return false
    if (line == tl && line == tll) return col >= tc && col < tlc
    if (line == tl) return col >= tc
    if (line == tll) return col < tlc
    return true
}

def private expression_name(expr : Expression const?) : string {
    if (expr is ExprVar) return string((expr as ExprVar).name)
    if (expr is ExprCall) return string((expr as ExprCall).name)
    if (expr is ExprField) return string((expr as ExprField).name)
    if (expr is ExprSafeField) return string((expr as ExprSafeField).name)
    if (expr is ExprOp1) return string((expr as ExprOp1).op)
    if (expr is ExprOp2) return string((expr as ExprOp2).op)
    if (expr is ExprOp3) return "?"
    if (expr is ExprLooksLikeCall) return string((expr as ExprLooksLikeCall).name)
    if (expr is ExprNamedCall) return string((expr as ExprNamedCall).name)
    return ""
}

[arch(at="ARCHITECTURE_CURSOR.md#cursor-compiler-made-names")]
def public is_synthesized(func : Function const?) : bool {
    //! True for a member the compiler made on the `class` line (`Foo'__finalize`), never for a lambda or generator body or a generic instance.
    return func != null && func.flags.generated && !func.flags._lambda && !func.flags._generator && func.fromGeneric == null
}

[arch(at="ARCHITECTURE_CURSOR.md#cursor-compiler-made-names")]
def public source_name(name : string) : string {
    //! The name the source spells for a compiler-renamed variable (a generator's loop variable); any other name unchanged.
    return name if (!(name |> starts_with("__")))
    let at = find(name, "_rename_at_")
    return at > 2 ? slice(name, 2, at) : name
}

def private is_ident_byte(c : uint8) : bool {
    return utf32_is_word_char(uint(c))
}

def public word_at(line_text : string; col : int) : tuple<word : string; start : int> {
    //! The identifier the 0-based byte column `col` is on in `line_text`, with its start column; a caret right after a word is on it, as in an editor, and the word is empty when `col` is on none.
    var word = ""
    var start = -1
    peek_data(line_text) $(d) {
        let n = length(d)
        return if (col < 0 || col > n)
        var b = col
        while (b > 0 && is_ident_byte(d[b - 1])) {
            b--
        }
        var e = col
        while (e < n && is_ident_byte(d[e])) {
            e++
        }
        start = b
        word = slice(d, b, e)
    }
    return (word = word, start = start)
}

[arch(at="ARCHITECTURE_CURSOR.md#cursor-variable-position")]
def public name_column(line_text : string; name : string; hint : int) : int {
    //! Where `line_text` spells `name` as a whole word (0-based byte column), else `hint`.
    var column = hint
    peek_data(line_text) $(d) {
        let n = length(d)
        let w = length(name)
        var at = find(d, name)
        while (at >= 0) {
            let before = at == 0 || !is_ident_byte(d[at - 1])
            let after = at + w >= n || !is_ident_byte(d[at + w])
            if (before && after) {
                column = at
                return
            }
            at = find(d, name, at + 1)
        }
    }
    return column
}

def public spells_name(line_text : string; col : int; name : string) : bool {
    //! True when `line_text` spells `name` as a whole word starting exactly at `col`.
    let w = word_at(line_text, col)
    return w.word == name && w.start == col
}

enum public DeclarationKind {
    //! What `find_declaration_at_cursor` or `declaration_of` found: which of `DeclarationHit`'s pointers is set.
    //! no declaration
    none
    //! a function or generic: `func`
    func
    //! a struct or class: `structure`
    structure
    //! an enumeration: `enumeration`
    enumeration
    //! a typedef: `name` and `alias_type`
    alias
    //! a global variable: `variable`
    variable
    //! a C++-bound type: `annotation`
    handled
    //! a struct or class field: `structure` owns it, `name` and `field_type` describe it
    field
}

struct public DeclarationHit {
    //! A declaration - the one whose name the cursor is on, or the one a type denotes: at most one of the pointers is set, by `kind`.
    //! which pointer below is set; `none` when there is no declaration
    kind : DeclarationKind
    //! the function or generic, for `func`
    func : Function const?
    //! the structure or class, for `structure`; the owner, for `field`
    structure : Structure const?
    //! the field's type, for `field`
    field_type : TypeDecl const?
    //! the enumeration, for `enumeration`
    enumeration : Enumeration const?
    //! the global variable, for `variable`
    variable : Variable const?
    //! the aliased type, for `alias`; null when no module of the program declares the typedef
    alias_type : TypeDecl const?
    //! the C++-bound type's annotation, for `handled`
    annotation : TypeAnnotation const?
    //! the declared name
    name : string
    //! the declaration's own position; zero for `handled` and for an alias no module declares
    @safe_when_uninitialized at : LineInfo
    //! 0-based byte column where the cursor's word starts on the cursor line; -1 when no cursor word named it
    word_column : int = -1
}

def private in_file(at : LineInfo; file : string) : bool {
    return true if (empty(file) || at.fileInfo == null)
    return find(string(at.fileInfo.name), file) >= 0
}

def private on_line(at : LineInfo; file : string; line : int) : bool {
    return int(at.line) == line && in_file(at, file)
}

def public declared_name_column(at : LineInfo; name : string) : int {
    //! Where the line a declaration starts on spells its `name`; `at.column` when it does not.
    var column = int(at.column)
    return column if (at.fileInfo == null || int(at.line) == 0)
    get_file_source_line(at.fileInfo, at.line) $(text) {
        column = name_column(string(text), name, column)
    }
    return column
}

def private typedef_in(mod : Module?; name : string) : TypeDecl const? {
    var found : TypeDecl const?
    return found if (mod == null)
    for_each_typedef(mod) $(alias_name : string#; var value : TypeDeclPtr) {
        if (alias_name == name) {
            found = value
        }
    }
    return found
}

def public typedef_named(program : smart_ptr<Program>; name : string; mod : Module? = null) : TypeDecl const? {
    //! The type declared under the typedef `name` - in `mod` when given, else in the program's own module, else in any module it requires; null when none declares it.
    var found = typedef_in(mod != null ? mod : get_this_module(program), name)
    if (found == null) {
        program_for_each_module(program) $(m) {
            if (found == null) {
                found = typedef_in(m, name)
            }
        }
    }
    return found
}

def public declaration_of(program : smart_ptr<Program>; td : TypeDecl const?) : DeclarationHit {
    //! The declaration a type denotes - its typedef, structure, enumeration or C++-bound annotation; `none` for a bare type.
    return DeclarationHit() if (td == null)
    if (!empty(td.alias)) {
        let name = string(td.alias)
        var hit = DeclarationHit(kind = DeclarationKind.alias, name = name, alias_type = typedef_named(program, name, td._module))
        if (hit.alias_type != null) {
            hit.at = hit.alias_type.at
        }
        return hit
    }
    if (td.isHandle && td.annotation != null) return DeclarationHit(kind = DeclarationKind.handled, annotation = td.annotation, name = string(td.annotation.name))
    if (td.structType != null) return DeclarationHit(kind = DeclarationKind.structure, structure = td.structType, name = string(td.structType.name), at = td.structType.at)
    if (td.enumType != null) return DeclarationHit(kind = DeclarationKind.enumeration, enumeration = td.enumType, name = string(td.enumType.name), at = td.enumType.at)
    return DeclarationHit()
}

//! Only the line's first whole-word spelling of `name` is the declaration - a self-call or a parameter named after the function is not.
def private on_name(word : string; name : string; line_text : string; here : int) : bool {
    return true if (empty(word))
    return name == word && here == name_column(line_text, name, -1)
}

def public is_own_field(fld : FieldDeclaration) : bool {
    //! True for a field the source declares in its struct's or class's own body - an override included - not one the compiler made (`__rtti`, `__finalize`) or a parent's copy.
    return fld.flags.implemented
}

def public value_struct(value : Expression const?) : Structure const? {
    //! The struct a field access reads its field from - through a pointer when `value` is one; null for any other value.
    return null if (value == null || value._type == null)
    var vt = value._type
    if (vt.structType == null && vt.firstType != null) {
        vt = vt.firstType
    }
    return vt.structType
}

def public field_name_at(expr : Expression const?) : LineInfo {
    //! Where a field access spells its field name - a method call's name sits past the `->` its node spans; a zero position for any other node.
    if (expr is ExprField) return (expr as ExprField).atField
    if (expr is ExprSafeField) return (expr as ExprSafeField).atField
    return LineInfo()
}

def public field_at(st : Structure const?; name : string) : LineInfo {
    //! Where the field `name` of `st` is declared - the parent's line for an inherited field; a zero position when there is no such field.
    let fld = find_structure_field(st, name)
    return fld != null ? fld.at : LineInfo()
}

def private function_declaration(f : Function const?; word, file : string; line : int; line_text : string; here : int) : DeclarationHit {
    return DeclarationHit() if (is_synthesized(f) || !on_line(f.at, file, line) || !on_name(word, bare_name(string(f.name)), line_text, here))
    return DeclarationHit(kind = DeclarationKind.func, func = f, name = bare_name(string(f.name)), at = f.at, word_column = here)
}

def public find_declaration_at_cursor(program : smart_ptr<Program>; file : string; line, col : int; line_text : string) : DeclarationHit {
    //! The declaration whose name is at the cursor, the parent a `class` line names included; `line_text` "" makes it a line-only lookup, `file` is a substring of ``FileInfo.name``.
    var hit = DeclarationHit()
    let w = word_at(line_text, col)
    let word = w.word
    let here = w.start
    let thisMod = get_this_module(program)
    ast::for_each_function(thisMod, "") $(var func : FunctionPtr) {
        return if (hit.kind != DeclarationKind.none)
        hit = function_declaration(func, word, file, line, line_text, here)
    }
    if (hit.kind == DeclarationKind.none) {
        ast::for_each_generic(thisMod) $(var func : FunctionPtr) {
            return if (hit.kind != DeclarationKind.none)
            hit = function_declaration(func, word, file, line, line_text, here)
        }
    }
    if (hit.kind == DeclarationKind.none) {
        ast::for_each_structure(thisMod) $(var st : StructurePtr) {
            return if (hit.kind != DeclarationKind.none)
            let s = st
            return if (!on_line(s.at, file, line))
            if (on_name(word, string(s.name), line_text, here)) {
                hit = DeclarationHit(kind = DeclarationKind.structure, structure = s, name = string(s.name), at = s.at, word_column = here)
            } elif (!empty(word) && s.parent != null && s.parent.name == word && here == name_column(line_text, word, -1)) {
                let p = s.parent
                hit = DeclarationHit(kind = DeclarationKind.structure, structure = p, name = word, at = p.at, word_column = here)
            }
        }
    }
    if (hit.kind == DeclarationKind.none) {
        ast::for_each_structure(thisMod) $(var st : StructurePtr) {
            return if (hit.kind != DeclarationKind.none)
            let s = st
            return if (s.flags.generated)   // a lambda's capture struct holds each captured variable as a field at the variable's own position
            for (fld in s.fields) {
                continue if (!is_own_field(fld) || fld.flags.classMethod || !on_line(fld.at, file, line) || !on_name(word, string(fld.name), line_text, here))
                hit = DeclarationHit(kind = DeclarationKind.field, structure = s, field_type = fld._type, name = string(fld.name), at = fld.at, word_column = here)
                break
            }
        }
    }
    if (hit.kind == DeclarationKind.none) {
        ast::for_each_enumeration(thisMod) $(var en : EnumerationPtr) {
            return if (hit.kind != DeclarationKind.none)
            let e = en
            return if (!on_line(e.at, file, line) || !on_name(word, string(e.name), line_text, here))
            hit = DeclarationHit(kind = DeclarationKind.enumeration, enumeration = e, name = string(e.name), at = e.at, word_column = here)
        }
    }
    if (hit.kind == DeclarationKind.none) {
        ast::for_each_typedef(thisMod) $(name : string#; var value : TypeDeclPtr) {
            return if (hit.kind != DeclarationKind.none)
            let td = value
            return if (!on_line(td.at, file, line) || !on_name(word, string(name), line_text, here))
            hit = DeclarationHit(kind = DeclarationKind.alias, alias_type = td, name = string(name), at = td.at, word_column = here)
        }
    }
    if (hit.kind == DeclarationKind.none) {
        ast::for_each_global(thisMod) $(var value : VariablePtr) {
            return if (hit.kind != DeclarationKind.none)
            let v = value
            return if (!on_line(v.at, file, line) || !on_name(word, string(v.name), line_text, here))
            hit = DeclarationHit(kind = DeclarationKind.variable, variable = v, name = string(v.name), at = v.at, word_column = here)
        }
    }
    return hit
}

def public bare_name(name : string) : string {
    //! "Animal`speak" -> "speak": the token the source spells at a method's declaration and call sites.
    let ix = rfind(name, "`")
    return ix < 0 ? name : slice(name, ix + 1)
}

class CursorVisitor : AstVisitor {
    targetLine : int
    targetCol : int
    targetFile : string
    hits : array<CursorHit>
    currentFunc : Function const?
    withDeclarations : bool
    inSynthesized : bool
    targetLineText : string
    typeHits : table<string>    // one type hit per name and position: a `new T()` carries T on several nodes

    def CursorVisitor(file : string; line, col : int; declarations : bool; line_text : string) {
        targetFile = file
        targetLine = line
        targetCol = col
        withDeclarations = declarations
        targetLineText = line_text
    }

    def in_target_file(at : LineInfo) : bool {
        return in_file(at, targetFile)
    }

    [arch(at="ARCHITECTURE_CURSOR.md#cursor-variable-position")]
    def cursor_on_variable(v : Variable const?) : bool {
        return cursor_on_token(v.at, string(v.name))
    }

    def add_variable_at_cursor(v : Variable const?) : void {
        return if (!withDeclarations || inSynthesized || v == null || v.flags.generated || !in_target_file(v.at) || !cursor_on_variable(v))
        hits |> emplace(CursorHit(
            variable = v,
            func = currentFunc,
            name = string(v.name),
            rtti = "Variable"
        ))
    }

    def override preVisitFunction(fun : FunctionPtr) : void {
        currentFunc = fun
        inSynthesized = is_synthesized(fun)
    }

    def override preVisitFunctionArgument(fun : FunctionPtr; arg : VariablePtr; lastArg : bool) : void {
        add_variable_at_cursor(arg)
    }

    def override preVisitExprBlockArgument(blk : ExprBlock?; arg : VariablePtr; lastArg : bool) : void {
        add_variable_at_cursor(arg)
    }

    def override preVisitExprLetVariable(expr : ExprLet?; arg : VariablePtr; lastArg : bool) : void {
        add_variable_at_cursor(arg)
    }

    def override preVisitExprForVariable(expr : ExprFor?; svar : VariablePtr; last : bool) : void {
        add_variable_at_cursor(svar)
    }

    // type names are TypeDecls, not expressions; a method's implicit self carries its class where the source spells no name, so spells_name gates the hit
    def override preVisitTypeDecl(typ : TypeDeclPtr) : void {
        return if (!withDeclarations || inSynthesized || typ == null || !in_target_file(typ.at))
        let td = typ
        let name = string(td.structType != null ? td.structType.name : td.enumType != null ? td.enumType.name : td.alias)
        return if (!cursor_on_token(td.at, name) || !spells_name(targetLineText, int(td.at.column), name))
        let key = "{int(td.at.column)}:{name}"
        return if (key_exists(typeHits, key))
        typeHits |> insert(key)
        hits |> emplace(CursorHit(
            type_decl = td,
            func = currentFunc,
            name = name,
            rtti = "TypeDecl"
        ))
    }

    def override visitFunction(var fun : FunctionPtr) : FunctionPtr {
        currentFunc = null
        inSynthesized = false
        return fun
    }

    // a compiler-made variable - the __this a lambda reads its captures through - is not source the cursor can be on
    def generated_variable(e : Expression const?) : bool {
        return false if (!(e is ExprVar))
        let v = e as ExprVar
        return v.variable != null && v.variable.flags.generated
    }

    // a field access spans its value and the dot; the field name has its own position
    def on_field_name(e : Expression const?) : bool {
        let at = field_name_at(e)
        return int(at.line) > 0 && cursor_on_token(at, expression_name(e))
    }

    def cursor_on_token(at : LineInfo; name : string) : bool {
        return false if (int(at.line) != targetLine)
        if (int(at.last_line) != int(at.line) || int(at.last_column) > int(at.column)) {
            return cursor_inside(targetLine, targetCol, at)
        }
        let c = int(at.column)
        return targetCol >= c && targetCol < c + length(name)
    }

    // a generated field's initializer - the finalizer and vtable entries - sits on the class line with no source of its own
    def override preVisitStructureField(str : StructurePtr; decl : FieldDeclaration; last : bool) : void {
        inSynthesized = decl.flags.generated
    }

    def override visitStructureField(str : StructurePtr; decl : FieldDeclaration; last : bool) : void {
        inSynthesized = false
    }

    [arch(at="ARCHITECTURE_CURSOR.md#cursor-class-fields")]
    def override preVisitExpression(expr : ExpressionPtr) : void {
        let e = expr
        return if (inSynthesized || !in_target_file(e.at) || generated_variable(e) || e is ExprRef2Value)
        if (cursor_inside(targetLine, targetCol, e.at) || on_field_name(e)) {
            hits |> emplace(CursorHit(
                expr = e,
                func = currentFunc,
                name = expression_name(expr),
                rtti = e.__rtti
            ))
        }
    }
}

//! (lines spanned, start column, end column): a nested node spans no more lines, starts no
//! earlier and ends no later than the node holding it
def private span_key(hit : CursorHit) : int3 {
    if (hit.expr == null) {
        let c = int(hit_at(hit).column)
        return int3(0, c, c + length(hit.name))
    }
    let at = hit.expr.at
    return int3(int(at.last_line) - int(at.line), int(at.column), int(at.last_column))
}

//! sorted innermost first: fewer lines, then the later start, then the earlier end, then the
//! named node (a call, a field, a variable) over a nameless one on the same span, then the
//! later-visited (deeper) node
def private innermost_first(var src : array<CursorHit>) : array<CursorHit> {
    var order : array<int>
    order |> reserve(length(src))
    for (i in range(length(src))) {
        order |> push(i)
    }
    order |> sort() $(a, b) {
        let sa = span_key(src[a])
        let sb = span_key(src[b])
        if (sa.x != sb.x) return sa.x < sb.x
        if (sa.y != sb.y) return sa.y > sb.y
        if (sa.z != sb.z) return sa.z < sb.z
        let na = !empty(src[a].name)
        let nb = !empty(src[b].name)
        if (na != nb) return na
        return a > b
    }
    var result : array<CursorHit>
    result |> reserve(length(src))
    for (i in order) {
        result |> emplace <| src[i]
    }
    return <- result
}

def public find_at_cursor(program : smart_ptr<Program>; file : string; line, col : int; declarations : bool = false; line_text : string = "") : array<CursorHit> {
    //! Find all expression nodes at the given cursor position, innermost first; `file` is a substring of ``FileInfo.name``
    //! ("" matches any); nodes inside a synthesized member and compiler-made variables are never hits. With `declarations`, a cursor
    //! on a `for`/`let` variable's or an argument's name is a `variable` hit and, given the line's source as `line_text`, on a type name a `type_decl` hit.
    return <- cursor_hits(file, line, col, declarations, line_text) $(var visitor) {
        make_visitor(*visitor) $(adapter) {
            visit(program, adapter)
        }
    }
}

def public find_at_cursor_in_function(func : FunctionPtr; file : string; line, col : int; declarations : bool = false; line_text : string = "") : array<CursorHit> {
    //! Find all expression nodes at the given cursor position within a single function body.
    //! `file`, `declarations` and `line_text` mean what they mean for `find_at_cursor`.
    return <- cursor_hits(file, line, col, declarations, line_text) $(var visitor) {
        visitor.currentFunc = func
        make_visitor(*visitor) $(adapter) {
            visit(func, adapter)
        }
    }
}

//! one visitor's run: `walk` drives it, the hits come back innermost first
def private cursor_hits(file : string; line, col : int; declarations : bool; line_text : string; walk : block<(var visitor : CursorVisitor?) : void>) : array<CursorHit> {
    var visitor = new CursorVisitor(file, line, col, declarations, line_text)
    invoke(walk, visitor)
    var result <- innermost_first(visitor.hits)
    unsafe {
        delete visitor
    }
    return <- result
}

def public describe(hit : CursorHit) : string {
    //! Pretty-print a CursorHit for debugging. Includes RTTI type, name, resolved type, source location, and enclosing function.
    return build_string() $(var w) {
        write(w, hit.rtti)
        if (!empty(hit.name)) {
            write(w, "({hit.name})")
        }
        if (hit.variable != null && hit.variable._type != null) {
            write(w, " : {describe(hit.variable._type)}")
        } elif (hit.type_decl != null) {
            write(w, " : {describe(unsafe(reinterpret<TypeDecl?>(hit.type_decl)))}")
        } elif (hit.expr != null && hit.expr._type != null) {
            write(w, " : {describe(hit.expr._type)}")
        }
        let at = hit_at(hit)
        write(w, " at {int(at.line)}:{int(at.column)}-{int(at.last_line)}:{int(at.last_column)}")
        if (hit.func != null) {
            write(w, " in {hit.func.name}")
        }
    }
}
