options gen2
options indenting = 4
options strict_smart_pointers = true
options no_global_variables = false

module lint shared private

//! Lint pass that enables paranoid compilation checks.
//!
//! When this module is required, the ``paranoid`` lint flag is turned on
//! for the current module, enabling stricter compiler diagnostics.
//! See also ``lint_everything`` for applying lint to all modules.

require daslib/strings_boost
require daslib/ast_boost
require daslib/strings_boost
require daslib/lint_config
require math

[lint_macro]
class LintEverything : AstPassMacro {
    //! Pass macro that enables lint checking for modules.
    def override apply(prog : ProgramPtr; _mod : Module?) : bool {
        let disabled <- build_lint_macro_disabled(prog)
        paranoid(prog, true, disabled)
        return true
    }
}

struct private DeadStoreEntry {
    @do_not_delete v : Variable const?
    @do_not_delete pending_expr : Expression const?
    skip : bool
    has_pending : bool
    pending_pure : bool
}

enum private Lint021CallKind {
    plain
    narrowing_cast
    poison
}

struct private Lint021Candidate {
    @do_not_delete v : Variable const?
    safe_uses : int
    disqualified : bool
}

def private is_block_terminator(expr : ExpressionPtr) : bool {
    if (expr is ExprReturn) return true
    if (expr is ExprCall) {
        let call = expr as ExprCall
        if (call.func == null) return false
        return call.name |> eq <| "panic" && call.func._module.name |> eq <| "builtin"
    }
    return false
}

//! the memory builtins that write through their first argument
let private lint031_builtin_writers <- { "memcpy", "memmove", "memset8", "memset16", "memset32", "memset64", "memset128" }

struct private RefIndex {
    @do_not_delete functions : table<Function const?>
    names : table<string>       //!< references knowable by name only (uninferred generic bodies)
    @do_not_delete structs : table<Structure?>
    @do_not_delete enums : table<Enumeration?>
}

def private unqualified(raw_name : string) : string {
    let sep = raw_name |> rfind("::")
    return sep >= 0 ? raw_name |> slice(sep + 2) : raw_name
}

def private note_ref(var self : RefIndex; fn : Function?; raw_name : string) : void {
    if (fn != null) {
        self.functions |> insert(fn)
        return
    }
    let nm = unqualified(raw_name)
    if (!empty(nm)) {
        self.names |> insert(nm)
    }
}

def private knows(self : RefIndex; fn : Function?; nm : string) : bool {
    return key_exists(self.functions, fn) || key_exists(self.names, nm)
}

def private note_type(var self : RefIndex; t : TypeDecl?; depth : int; skip : Structure const? = null) : void {
    return if (t == null || depth > 24)
    let st : Structure const? = t.structType
    if (st != null && st != skip) {
        self.structs |> insert(t.structType)
    }
    if (t.enumType != null) {
        self.enums |> insert(t.enumType)
    }
    self |> note_type(t.firstType, depth + 1, skip)
    self |> note_type(t.secondType, depth + 1, skip)
    for (a in t.argTypes) {
        self |> note_type(a, depth + 1, skip)
    }
}

def private note_type_names(var self : RefIndex; t : TypeDecl?; depth : int) : void {
    return if (t == null || depth > 24)
    let alias = "{t.alias}"
    if (!empty(alias)) {
        self.names |> insert(alias)
    }
    if (t.structType != null) {
        self.names |> insert("{t.structType.name}")
    }
    if (t.enumType != null) {
        self.names |> insert("{t.enumType.name}")
    }
    self |> note_type_names(t.firstType, depth + 1)
    self |> note_type_names(t.secondType, depth + 1)
    for (a in t.argTypes) {
        self |> note_type_names(a, depth + 1)
    }
}

def private lint022_calls_may_be_inlined(prog : ProgramPtr) : bool {
    return (prog.getOptimize
        && !(prog._options |> find_arg("disable_auto_inline") ?as tBool ?? prog.policies.disable_auto_inline)
        && !(prog._options |> find_arg("no_aliasing") ?as tBool ?? prog.policies.no_aliasing)
        && (prog._options |> find_arg("auto_inline_functions") ?as tBool ?? prog.policies.auto_inline_functions))
}

def private lint022_block_calls_may_be_inlined(prog : ProgramPtr) : bool {
    return (prog.getOptimize
        && !(prog._options |> find_arg("disable_auto_inline") ?as tBool ?? prog.policies.disable_auto_inline))
}

def private lint022_takes_block(fn : Function?) : bool {
    for (a in fn.arguments) {
        return true if (a._type != null && a._type.baseType == Type.tBlock)
    }
    return false
}

def private lint022_enabled(prog : ProgramPtr) : bool {
    let configured = prog._options |> find_arg("_unused_private")
    if (configured is tBool) return configured as tBool
    if (lint_config_forces_on("LINT022")) return true
    return is_core_library_source(prog)
}

def private lint022_is_skipped(fn : Function?; mod : Module?) : bool {
    return (fn == null || !fn.flags.privateFunction
        || fn.flags.generated || fn.flags.isClassMethod
        || fn.fromGeneric != null || fn.isGeneric || fn.moreFlags.isTemplate
        || length(fn.annotations) > 0
        || fn.at.fileInfo == null || fn._module != mod)
}

struct private Lint023Candidate {
    @do_not_delete v : Variable const?
    stores : int
    disqualified : bool
}

//! a variable read somewhere inside argument `slot` of the call a frame tracks
struct private Lint030Mention {
    slot : int
    @do_not_delete v : Variable const?
}

//! a variable a call somewhere inside argument `slot` writes through a mutable by-reference
//! parameter; `callee` names that call for the finding
struct private Lint030Write {
    slot : int
    @do_not_delete v : Variable const?
    callee : string
}

//! one call whose argument list the walk is inside: `slot` is the argument being walked,
//! `closure_depth` the depth the call sits at, so a read inside a lambda argument - run later,
//! not at the call - is not a mention
struct private Lint030Frame {
    @do_not_delete call : ExprCall const?
    slot : int
    closure_depth : int
    mentions : array<Lint030Mention>
    writes : array<Lint030Write>
}

//! `lint030_callee_writes`' memo, `<mangled name hash>#<k>` -> whether that function writes its
//! k-th parameter - a generic's instances key apart, since `static_if` can make one write and
//! another not; a key sits at `true` while its scan runs, so a recursive chain reads as writing
var private lint030_writers : table<string; bool>

def private lint030_bare_var(e : Expression?) : Variable const? {
    return e is ExprVar ? (e as ExprVar).variable : null
}

//! the variable a chain of plain field accesses is rooted at (`dr.offs`, `s.k.v`), the bare
//! variable itself, or null for any other shape; a field reached through a pointer is a value
//! a write can move, so the chain stops there
def private lint030_field_root(e : Expression?) : Variable const? {
    return lint030_bare_var(e) if (!(e is ExprField))
    let fld = e as ExprField
    return null if (fld.value == null || fld.value._type == null || fld.value._type.baseType == Type.tPointer)
    return lint030_field_root(fld.value)
}

//! one call the write scan is inside: `slot` is the argument being walked
struct private Lint030ArgFrame {
    @do_not_delete call : ExprCall const?
    slot : int
}

//! an operator that assigns to its left operand: `=` is an ExprCopy, these are the compound forms
def private lint030_assigns(op : string) : bool {
    return op |> ends_with("=") && op != "==" && op != "!=" && op != "<=" && op != ">="
}

def private lint030_steps(op : string) : bool {
    return op == "++" || op == "--" || op == "+++" || op == "---"
}

//! scans one function body for a write to `param` during the call; which compiler write marks
//! count and which are discounted is `daslib/ARCHITECTURE_LINT.md` sec.3's LINT030 entry
class private Lint030WriteScan : AstVisitor {
    @do_not_delete param : Variable const?
    writes : bool = false
    ret_depth : int = 0
    addr_depth : int = 0
    lhs_depth : int = 0
    hop_depth : int = 0
    hops : array<bool>
    args : array<Lint030ArgFrame>
    def hop_in(through_pointer : bool) : void {
        hops |> push(through_pointer)
        if (through_pointer) {
            hop_depth++
        }
    }
    def hop_out() : void {
        if (hops |> back()) {
            hop_depth--
        }
        hops |> pop()
    }
    def override preVisitExprPtr2Ref(expr : ExprPtr2Ref?) : void {
        let bare : Variable const? = lint030_bare_var(expr.subexpr)
        hop_in(bare != param)
    }
    def override visitExprPtr2Ref(var expr : ExprPtr2Ref?) : ExpressionPtr {
        hop_out()
        return expr
    }
    def override preVisitExprField(expr : ExprField?) : void {
        let through = expr.value != null && expr.value._type != null && expr.value._type.baseType == Type.tPointer
        let pointer_read = lhs_depth == 0 && expr._type != null && expr._type.baseType == Type.tPointer
        hop_in(through || pointer_read)
    }
    def override preVisitExprCopy(expr : ExprCopy?) : void {
        lhs_depth++
    }
    def override preVisitExprCopyRight(expr : ExprCopy?; right : ExpressionPtr) : void {
        lhs_depth--
    }
    def override preVisitExprMove(expr : ExprMove?) : void {
        lhs_depth++
    }
    def override preVisitExprMoveRight(expr : ExprMove?; right : ExpressionPtr) : void {
        lhs_depth--
    }
    def override preVisitExprClone(expr : ExprClone?) : void {
        lhs_depth++
    }
    def override preVisitExprCloneRight(expr : ExprClone?; right : ExpressionPtr) : void {
        lhs_depth--
    }
    def override preVisitExprOp2(expr : ExprOp2?) : void {
        if (lint030_assigns(string(expr.op))) {
            lhs_depth++
        }
    }
    def override preVisitExprOp2Right(expr : ExprOp2?; right : ExpressionPtr) : void {
        if (lint030_assigns(string(expr.op))) {
            lhs_depth--
        }
    }
    def override preVisitExprOp1(expr : ExprOp1?) : void {
        if (lint030_steps(string(expr.op))) {
            lhs_depth++
        }
    }
    def override visitExprOp1(var expr : ExprOp1?) : ExpressionPtr {
        if (lint030_steps(string(expr.op))) {
            lhs_depth--
        }
        return expr
    }
    def override visitExprField(var expr : ExprField?) : ExpressionPtr {
        hop_out()
        return expr
    }
    def override preVisitExprSafeField(expr : ExprSafeField?) : void {
        hop_in(true)
    }
    def override visitExprSafeField(var expr : ExprSafeField?) : ExpressionPtr {
        hop_out()
        return expr
    }
    def override preVisitExprCall(expr : ExprCall?) : void {
        args |> push(Lint030ArgFrame(call = expr, slot = -1))
    }
    def override visitExprCall(var expr : ExprCall?) : ExpressionPtr {
        args |> pop()
        return expr
    }
    def override preVisitExprCallArgument(_expr : ExprCall?; _arg : ExpressionPtr; _last : bool) : void {
        if (!empty(args)) {
            args[length(args) - 1].slot++
        }
    }
    def override preVisitExprReturn(expr : ExprReturn?) : void {
        ret_depth++
    }
    def override visitExprReturn(var expr : ExprReturn?) : ExpressionPtr {
        ret_depth--
        return expr
    }
    def override preVisitExprRef2Ptr(expr : ExprRef2Ptr?) : void {
        if (ret_depth > 0) {
            addr_depth++
        }
    }
    def override visitExprRef2Ptr(var expr : ExprRef2Ptr?) : ExpressionPtr {
        if (ret_depth > 0) {
            addr_depth--
        }
        return expr
    }
    def override preVisitExprVar(expr : ExprVar?) : void {
        let v : Variable const? = expr.variable
        return if (v != param || !expr.varFlags.write || addr_depth != 0 || hop_depth != 0 || writes)
        if (!empty(args) && args[length(args) - 1].slot >= 0) {
            let top = args[length(args) - 1]
            writes = lint030_callee_writes(top.call.func, top.slot)
        } else {
            writes = true
        }
    }
}

//! whether `fn` writes its `k`-th parameter during the call (`Lint030WriteScan`): a by-value
//! parameter's writes never reach the caller, and a function with no body to read - a builtin,
//! an extern - is taken as writing
def private lint030_callee_writes(fn : Function const?; k : int) : bool {
    return true if (fn == null || k >= length(fn.arguments))
    let p = fn.arguments[k]
    return false if (p._type == null || p._type.flags.constant || !(p._type.flags.ref || p._type.isRefType))
    return true if (fn.body == null)
    let key = "{fn.getMangledNameHash}#{k}"
    if (key_exists(lint030_writers, key)) {
        return lint030_writers?[key] ?? true
    }
    lint030_writers[key] = true
    var scan = new Lint030WriteScan(param = p)
    make_visitor(*scan) $(adapter) {
        visit(fn.body, adapter)
    }
    let writes = scan.writes
    unsafe {
        delete scan
    }
    lint030_writers[key] = writes
    return writes
}

var private lint031_writers : table<string; bool>

//! The operand a node carries a value of unchanged: a cast, a reference-to-value or
//! value-to-pointer step, an intptr round trip, pointer arithmetic (a call whose result aliases
//! its first argument's pointee); null for any other node.
def private lint031_carrier(expr : Expression const?) : Expression const? {
    if (expr is ExprCast) return (expr as ExprCast).subexpr
    if (expr is ExprRef2Ptr) return (expr as ExprRef2Ptr).subexpr
    if (expr is ExprRef2Value) return (expr as ExprRef2Value).subexpr
    if (expr is ExprCall) {
        let call = expr as ExprCall
        if (lint031_is_intptr(call) || (call.func != null && call.func.flags.firstArgReturnType && !empty(call.arguments))) {
            return call.arguments[0]
        }
    }
    return null
}

//! the name of the generic an instance came from, walked to the root; the function's own otherwise
def private lint031_root_name(fn : Function const?) : string {
    var root = fn
    while (root.fromGeneric != null) {
        root = root.fromGeneric
    }
    return string(root.name)
}

def private lint031_is_intptr(call : ExprCall const?) : bool {
    if (call.func == null || length(call.arguments) != 1) return false
    return lint031_root_name(call.func) == "intptr"
}

def private lint031_is_integer(t : TypeDecl const?) : bool {
    let bt = t.baseType
    return (bt == Type.tInt || bt == Type.tUInt || bt == Type.tInt64 || bt == Type.tUInt64
        || bt == Type.tInt8 || bt == Type.tUInt8 || bt == Type.tInt16 || bt == Type.tUInt16)
}

//! The operand a node reaches INTO - a deref, a field, an index, a swizzle - so a write to the
//! node is a write through the operand; null otherwise. Below the target, a node that yields a
//! pointer, or a field or index that yields an integer, is a load of an address: not the operand's memory.
def private lint031_pointee_of(expr : Expression const?; is_target : bool) : Expression const? {
    let loaded_pointer = !is_target && expr._type != null && expr._type.isPointer
    if (expr is ExprPtr2Ref) {
        if (loaded_pointer) return null
        return (expr as ExprPtr2Ref).subexpr
    }
    if (loaded_pointer || (!is_target && expr._type != null && lint031_is_integer(expr._type))) return null
    if (expr is ExprField) return (expr as ExprField).value
    if (expr is ExprAt) return (expr as ExprAt).subexpr
    if (expr is ExprSwizzle) return (expr as ExprSwizzle).value
    return null
}

//! The parameter in `params` a write target is reached from; null when the target roots elsewhere.
//! `through`: the write lands in what the target holds, so a local pointer copy is followed to
//! its initializer - reassigning the copy itself writes nothing the parameter owns.
def private lint031_root(params : array<Variable const?>; target : Expression const?; through : bool) : Variable const? {
    var expr = target
    var into = through
    var is_target = true
    while (expr != null) {
        let first = is_target
        is_target = false
        if (expr is ExprVar) {
            let v = (expr as ExprVar).variable
            if (v == null) return null
            if (params |> find_index(v) >= 0) return into ? v : null
            if (v.init == null || v._type == null) return null
            let via_intptr = v.init is ExprCall && lint031_is_intptr(v.init as ExprCall)
            if (!(v._type.flags.ref || via_intptr || v._type.isPointer)) return null
            expr = v.init
            continue
        }
        let carried = lint031_carrier(expr)
        if (carried != null) {
            expr = carried
            continue
        }
        expr = lint031_pointee_of(expr, first)
        into = true
    }
    return null
}

//! scans one function body for a write through `params` the LINT031 way - the write sites and
//! the chase the rule applies to a const parameter, so a callee is judged as its caller would be
class private Lint031BodyScan : AstVisitor {
    @do_not_delete params : array<Variable const?>
    writes : bool = false
    def note(target : Expression const?; through : bool) : void {
        if (!writes && lint031_root(params, target, through) != null) {
            writes = true
        }
    }
    def override preVisitExprCopy(expr : ExprCopy?) : void {
        note(expr.left, false)
    }
    def override preVisitExprClone(expr : ExprClone?) : void {
        note(expr.left, false)
    }
    def override preVisitExprMove(expr : ExprMove?) : void {
        note(expr.left, false)
    }
    def override preVisitExprOp2(expr : ExprOp2?) : void {
        if (lint030_assigns(string(expr.op))) {
            note(expr.left, false)
        }
    }
    def override preVisitExprOp1(expr : ExprOp1?) : void {
        if (lint030_steps(string(expr.op))) {
            note(expr.subexpr, false)
        }
    }
    def override preVisitExprCallArgument(expr : ExprCall?; arg : ExpressionPtr; _last : bool) : void {
        if (lint031_slot_written(expr, arg)) {
            note(arg, true)
        }
    }
}

//! whether `fn` writes through its `k`-th parameter: a builtin answers by name (both pointers of a
//! memory builtin register as `void *`, so the type says nothing), a das function by its body, read
//! once per (function, slot); a slot still being scanned - recursion - reads as not writing
def private lint031_callee_writes(fn : Function const?; k : int) : bool {
    return false if (fn == null || k >= length(fn.arguments))
    if (fn.flags.builtIn || fn.body == null) {
        return k == 0 && lint031_builtin_writers |> key_exists(lint031_root_name(fn))
    }
    let pt = fn.arguments[k]._type
    return false if (pt == null || pt.flags.constant || !(pt.isRef || pt.isPointer)
        || (pt.isPointer && pt.firstType != null && pt.firstType.flags.constant))
    let key = "{fn.getMangledNameHash}#{k}"
    if (key_exists(lint031_writers, key)) {
        return lint031_writers?[key] ?? false
    }
    lint031_writers[key] = false
    var scan = new Lint031BodyScan()
    scan.params |> push(fn.arguments[k])
    make_visitor(*scan) $(adapter) {
        visit(fn.body, adapter)
    }
    let writes = scan.writes
    unsafe {
        delete scan
    }
    lint031_writers[key] = writes
    return writes
}

//! `call` writes through what `arg` addresses - `arg` fills a slot whose callee writes through it
def private lint031_slot_written(call : ExprCall const?; arg : ExpressionPtr) : bool {
    return false if (call.func == null)
    for (a, i in call.arguments, count()) {
        if (a == arg) return lint031_callee_writes(call.func, i)
    }
    return false
}

class LintVisitor : AstVisitor {
    lint030_frames : array<Lint030Frame>
    @do_not_delete lint014_candidates : array<Variable const?>
    lint023_candidates : array<Lint023Candidate>
    lint023_store_nodes : table<uint64>
    @do_not_delete lint029_candidates : array<Variable const?>
    @do_not_delete lint029_mutated : array<Variable const?>
    lint029_exempt : bool = false
    @do_not_delete lint031_params : array<Variable const?>
    @do_not_delete lint031_reported : array<Variable const?>
    //! AST visitor that performs lint checks.
    exprForTerminator : array<uint64>
    compile_time_errors : bool
    noLint : bool = false
    genericDepth : int = 0
    multiple_contexts : bool = false
    collect_errors : bool = false
    errors : array<string>
    issues : array<LintIssue>
    disabled_codes : table<string>
    enabled_codes : table<string>
    branch_depth : int = 0
    ds_entries : array<DeadStoreEntry>
    funcFile : string
    lint021_candidates : array<Lint021Candidate>
    lint021_call_kind : array<Lint021CallKind>
    lint021_narrow_depth : int = 0
    lint021_poison_depth : int = 0
    lint021_closure_depth : int = 0
    lint023_deferred_depth : int = 0
    flow_jumps : bool = false
    lint022_refs : RefIndex
    @do_not_delete lint022_self_struct : Structure const? = null
    lint022_generic_sweep : bool = false
    def LintVisitor() {
        pass
    }
    def is_suppressed(text : string; at : LineInfo) : bool {
        let code = extract_lint_code(text)
        if (!empty(code)) {
            return true if (key_exists(disabled_codes, code)
                            || (!empty(enabled_codes) && !key_exists(enabled_codes, code)))
        }
        return is_lint_suppressed(at, code)
    }
    def lint_error(text : string; at : LineInfo) : void {
        if (noLint || is_suppressed(text, at)
                || (at.fileInfo != null && !empty(funcFile) && at.fileInfo.name != funcFile)) return
        if (compile_time_errors) {
            compiling_program() |> macro_error(at, text)
        } elif (collect_errors) {
            errors |> push("{text} at {describe(at)}")
            issues |> push(make_lint_issue(text, at))
        } else {
            error("{text} at {describe(at)}\n")
        }
    }
    def private source_operator_is_clone(at : LineInfo) : bool {
        //! Inference lowers a string ExprClone to ExprCopy in ordinary mode, but preserves the clone operator's LineInfo. Check that source token so a regular '=' copy stays silent.
        if (at.fileInfo == null || at.line == 0u || at.line != at.last_line) return false
        var found = false
        get_file_source_line(at.fileInfo, at.line) $(line) {
            var start = int(at.column) - 1
            if (start < 0) {
                start = 0
            }
            var finish = int(at.last_column) + 1
            if (finish < start + 3) {
                finish = start + 3
            }
            if (finish > length(line)) {
                finish = length(line)
            }
            if (finish > start) {
                found = find(slice(line, start, finish), ":=") >= 0
            }
        }
        return found
    }
    def private root_generic(fn : Function const?) : Function const? {
        var root = fn
        while (root != null && root.fromGeneric != null) {
            root = root.fromGeneric
        }
        return root
    }
    def private is_temporary_string_source(expr : Expression?) : bool {
        if (expr == null) return false
        if (expr._type != null && expr._type.flags.temporary) return true
        var inner = expr
        if (inner is ExprRef2Value) {
            inner = (inner as ExprRef2Value).subexpr
        }
        if (inner is ExprVar) {
            let variable = (inner as ExprVar).variable
            return variable != null && variable._type != null && variable._type.flags.temporary
        }
        return false
    }
    def private is_inferred_temp_string_clone(expr : Expression?; clone_at : LineInfo) : bool {
        //! InferTypes rewrites `dst := string#_expr` to `dst = clone_string(string#_expr)` and gives the injected call the original := token location. A source-written clone_string call keeps its own location.
        if (expr == null || !(expr is ExprCall)) return false
        let call = expr as ExprCall
        return (call.func != null && call.func.name == "clone_string"
            && call.at.line == clone_at.line && call.at.column == clone_at.column)
    }
    def private is_inferred_temp_string_init(expr : Expression?; init_at : LineInfo) : bool {
        //! InferTypes rewrites `var dst := string#_expr` through clone_string. The wrapper result is a regular string, so inspect its source argument to preserve the temporary-string exception.
        if (expr == null || !(expr is ExprCall)) return false
        let call = expr as ExprCall
        let root = root_generic(call.func)
        return (root != null && root.name == "clone_string"
            && root._module != null && root._module.name == "builtin"
            && call.at.line == init_at.line && call.at.column == init_at.column
            && !empty(call.arguments) && is_temporary_string_source(call.arguments[0]))
    }
    def private source_variable_init_is_clone(v : VariablePtr) : bool {
        //! String clone initialization is lowered before lint runs. Search only between the source variable name and its initializer, so another declaration or := inside the RHS cannot produce a false positive.
        if (v == null || v.init == null || v.at.fileInfo == null || v.at.line == 0u) return false
        var found = false
        get_file_source_line(v.at.fileInfo, v.at.line) $(line) {
            var start = int(v.at.column) - 1
            if (start < 0) {
                start = 0
            }
            var finish = length(line)
            if (v.init.at.line == v.at.line && int(v.init.at.column) > start) {
                finish = int(v.init.at.column) + 1
                if (finish > length(line)) {
                    finish = length(line)
                }
            }
            if (finish > start) {
                found = find(slice(line, start, finish), ":=") >= 0
            }
        }
        return found
    }
    def private check_string_clone_init(v : VariablePtr) : void {
        if (v == null || multiple_contexts || noLint || genericDepth > 0
                || v.flags.generated || v._type == null || v.init == null
                || v._type.baseType != Type.tString
                || is_temporary_string_source(v.init)
                || is_inferred_temp_string_init(v.init, v.at)
                || !source_variable_init_is_clone(v)) return
        lint_error("LINT016: ':=' does not clone a string variable without 'options multiple_contexts'; use '=' for same-context storage, or did you mean '= clone_string(value)' for a cross-context copy", v.at)
    }
    def private check_string_clone_assign(expr : ExprCopy?) : void {
        if (expr == null || multiple_contexts || noLint || genericDepth > 0 || expr.genFlags.generated
                || expr.left == null || expr.right == null
                || expr.left._type == null || expr.right._type == null
                || expr.left._type.baseType != Type.tString
                || expr.right._type.baseType != Type.tString
                || is_temporary_string_source(expr.right)
                || is_inferred_temp_string_clone(expr.right, expr.at)
                || !source_operator_is_clone(expr.at)) return
        lint_error("LINT016: ':=' does not clone a string without 'options multiple_contexts'; use '=' for same-context storage, or did you mean 'clone_string(value)' for a cross-context copy", expr.at)
    }
    def private check_string_push_clone(expr : ExprCall?) : void {
        if (expr == null || multiple_contexts || noLint || genericDepth > 0 || expr.genFlags.generated
                || expr.func == null || length(expr.arguments) < 2) return
        let root = root_generic(expr.func)
        if (root == null || root.name != "push_clone"
                || root._module == null || root._module.name != "builtin") return
        let dst = expr.arguments[0]
        let value = expr.arguments[1]
        if (dst == null || value == null || dst._type == null || value._type == null
                || dst._type.baseType != Type.tArray || dst._type.firstType == null
                || dst._type.firstType.baseType != Type.tString
                || value._type.baseType != Type.tString
                || is_temporary_string_source(value)) return
        lint_error("LINT016: 'push_clone' does not clone a string without 'options multiple_contexts'; use 'push(value)' for same-context storage, or did you mean 'push(clone_string(value))' for a cross-context copy", expr.at)
    }
    //! the callee-body memos outlive a program: the runner, the MCP server and the LSP keep one
    //! process across programs, and an edited body keeps its mangled name
    def override preVisitProgram(_prog : ProgramPtr) : void {
        lint030_writers |> clear()
        lint031_writers |> clear()
    }

    def override preVisitFunction(fun : FunctionPtr) : void {
        noLint = lint022_generic_sweep
        funcFile = fun.at.fileInfo != null ? "{fun.at.fileInfo.name}" : ""
        lint022_self_struct = null
        if (fun.flags.isClassMethod || fun.flags.generated) {
            if (fun.result != null && fun.result.structType != null) {
                lint022_self_struct = fun.result.structType
            } elif (!empty(fun.arguments) && fun.arguments[0]._type != null) {
                lint022_self_struct = fun.arguments[0]._type.structType
            }
        }
        if (fun.fromGeneric != null || fun.flags.generated) {
            genericDepth++
        }
        branch_depth = 0
        ds_entries |> clear()
        lint031_params |> clear()
        lint031_reported |> clear()
        lint021_candidates |> clear()
        lint021_call_kind |> clear()
        lint021_narrow_depth = 0
        lint021_poison_depth = 0
        lint021_closure_depth = 0
        lint023_deferred_depth = 0
        lint030_frames |> clear()
        flow_jumps = false
        if (fun.moreFlags.isTemplate) {
            noLint = true
            return
        }
        for (ann in fun.annotations) {
            if (ann.annotation.name == "no_lint") {
                noLint = true
                break
            }
        }
    }
    def override preVisitExprReturn(expr : ExprReturn?) : void {
        if (expr.subexpr != null && expr.subexpr is ExprVar) {
            let ev = expr.subexpr as ExprVar
            let idx = lint014_candidates |> find_index(ev.variable)
            if (idx >= 0) {
                lint014_candidates |> erase(idx)
            }
            let idx29 = lint029_candidates |> find_index(ev.variable)
            if (idx29 >= 0) {
                lint029_candidates |> erase(idx29)
            }
        }
    }

    def override visitFunction(var fun : FunctionPtr) : FunctionPtr {
        if (!noLint && genericDepth == 0) {
            for (v in lint014_candidates) {
                lint_error("LINT014: mutable argument {v.name}: {describe(v._type)} is never written; declare it without 'var'", v.at)
            }
        }
        lint014_candidates |> clear()
        if (!noLint && genericDepth == 0) {
            for (c in lint023_candidates) {
                if (!c.disqualified && c.stores > 0 && !flow_jumps) {
                    lint_error("LINT023: mutable by-value argument {c.v.name}: {describe(c.v._type, false, false, false)} is written but never read - the caller never sees a write to a by-value copy; declare it `{describe(c.v._type, false, false, false)}&` if it is an out-parameter, otherwise the write is dead", c.v.at)
                }
            }
        }
        lint023_candidates |> clear()
        lint023_store_nodes |> clear()
        if (!noLint && genericDepth == 0) {
            for (v in lint029_candidates) {
                let structish = (v._type != null
                    && (v._type.baseType == Type.tStructure || v._type.baseType == Type.tHandle))
                continue if (structish && lint029_mutated |> find_index(v) < 0)
                lint_error("LINT029: by-ref argument {v.name}: {describe(v._type, false, false, false)} is mutated - every call site sees the write; declare the parameter const and return the result instead if this function should be pure", v.at)
            }
        }
        lint029_candidates |> clear()
        lint029_mutated |> clear()
        if (!noLint && genericDepth == 0) {
            for (e in ds_entries) {
                if (!e.skip && e.has_pending && e.pending_pure && e.pending_expr != null) {
                    let vname = "{e.v.name}"
                    lint_error("LINT010: dead store of '{vname}': value written but never read before scope exit", e.pending_expr.at)
                }
            }
        }
        if (!noLint && genericDepth == 0) {
            for (c in lint021_candidates) {
                if (!c.disqualified && c.safe_uses > 0) {
                    lint_error("LINT021: 64-bit local '{c.v.name}' is never consumed as 64-bit - every use narrows through int(...)/uint(...); narrow once at the declaration (or use length() int math), or lift the sinks to 64-bit (range64, int64 resize/reserve/erase); nolint if the 64-bit math under a cast is load-bearing", c.v.at)
                }
            }
        }
        lint021_candidates |> clear()
        noLint = false
        if (fun.fromGeneric != null || fun.flags.generated) {
            genericDepth--
        }
        return <- fun
    }
    def override preVisitExprBlock(blk : ExprBlock?) : void {
        exprForTerminator |> push(0ul)
        let bf = blk.blockFlags
        if (!noLint && genericDepth == 0 && (bf.isClosure || bf.isLambdaBlock || bf.isGeneratorBlock)) {
            branch_depth++
            lint021_closure_depth++
            lint023_deferred_depth++
        }
    }
    def override visitExprBlock(var blk : ExprBlock?) : ExpressionPtr {
        if (!empty(exprForTerminator)) {
            exprForTerminator |> pop()
        }
        let bf = blk.blockFlags
        if (!noLint && genericDepth == 0 && (bf.isClosure || bf.isLambdaBlock || bf.isGeneratorBlock)) {
            branch_depth--
            lint021_closure_depth--
            lint023_deferred_depth--
        }
        return <- blk
    }
    def override preVisitExprBlockExpression(_blk : ExprBlock?; expr : ExpressionPtr) : void {
        if (noLint || genericDepth > 0 || !(expr is ExprOp1)) return
        let op1 = expr as ExprOp1
        if (op1.op == "++" || op1.op == "--" || op1.op == "+++" || op1.op == "---") {
            if (op1.subexpr != null && op1.subexpr is ExprVar) {
                lint023_store_nodes |> insert(intptr(op1.subexpr))
            }
        }
    }
    def override visitExprBlockExpression(_blk : ExprBlock?; var expr : ExpressionPtr) : ExpressionPtr {
        let lb = exprForTerminator |> back()
        if (lb != 0ul) {
            lint_error("LINT001: unreachable code", expr.at)
        }
        if (!noLint && genericDepth == 0 && expr is ExprOp1 && !expr.genFlags.generated) {
            let op1 = expr as ExprOp1
            if (op1.op == "+" || op1.op == "-") {
                lint_error("LINT015: free-floating unary '{op1.op}' expression as a statement - its value is silently discarded; this is almost always a continuation line missing its parens, wrap the multi-line RHS in (...)", expr.at)
            }
        }
        if (is_block_terminator(expr)) {
            exprForTerminator |> pop()
            exprForTerminator |> push(intptr(expr))
        }
        return <- expr
    }
    def override preVisitExprLabel(_expr : ExprLabel?) : void {
        if (!empty(exprForTerminator)) {
            exprForTerminator |> pop()
        }
        exprForTerminator |> push(0ul)
        note_flow_jump()
    }

    def override preVisitExprCast(expr : ExprCast?) : void {
        if (lint022_generic_sweep) {
            lint022_refs |> note_type_names(expr.castType, 0)
        }
        if (noLint || genericDepth > 0
            || !expr.castFlags.reinterpretCast || expr.genFlags.alwaysSafe
            || expr.subexpr == null || expr.subexpr._type == null || expr.castType == null
            || expr.castType.isVoidPointer != expr.subexpr._type.isVoidPointer) return
        if (is_same_type(expr.castType, expr.subexpr._type, false, true, true, false)) {
            lint_error("LINT005: redundant reinterpret cast; target type is the same as source type", expr.at)
        }
    }

    def override preVisitExprForVariable(expr : ExprFor?; v : VariablePtr; _last : bool) : void {
        if (expr.canShadow) return
        validate_var(v, false)
    }

    def private is_inline_temp_name(name : string) : bool {
        if (!(name |> starts_with("_inl")) || length(name) <= 4) return false
        var digit = false
        peek_data(name) $(bytes) {
            digit = is_number(int(bytes[4]))
        }
        return digit
    }

    def validate_var(v : VariablePtr; can_make_const : bool) {
        if (v.flags.generated || v.flags.marked_used) return
        let name = string(v.name)
        if (name |> starts_with("__") || find(name, "`") >= 0 || is_inline_temp_name(name)) return
        if (name |> starts_with("_")) {
            if (name |> ends_with("_")) return
            if (v.access_flags.access_ref || v.access_flags.access_pass || v.access_flags.access_get || v.access_flags.access_fold) {
                lint_error("LINT004: variable '{v.name}' is used and should be named without underscore prefix", v.at)
                return
            }
            return
        }
        if (v.isAccessUnused) {
            lint_error("LINT002: unused variable {v.name}: {describe(v._type)} (add an underscore prefix if you really need it)", v.at)
            return
        }
        if (!v.access_flags.access_ref && !v.access_flags.access_pass && v.init != null && v.init |> is_expr_const() && !v.access_flags.access_fold) {
            lint_error("LINT002: unused variable {v.name}: {describe(v._type)} (add an underscore prefix if you really need it)", v.at)
            return
        }
        if (!v.access_flags.access_ref && !v.access_flags.access_pass && !v.access_flags.access_get && (v.init == null || v.init.flags.noSideEffects) && !v.access_flags.access_fold) {
            lint_error("LINT002: unused variable {v.name}: {describe(v._type)} (add an underscore prefix if you really need it)", v.at)
            return
        }
        if (!v.access_flags.access_get && !v.access_flags.access_ref && (v.init == null)) {
            lint_error("LINT002: variable {v.name}: {describe(v._type)} is never used", v.at)
            return
        }
        if (can_make_const && (v.flags.inScope || v.access_info.access_info_pass_mutable)) return
        if (can_make_const && v._type.baseType != Type.tPointer && !v._type.flags.constant && !v.access_flags.access_ref && v._type.canCloneFromConst) {
            lint_error("LINT003: variable {v.name}: {describe(v._type)} can be made const (declare with 'let')", v.at)
            return
        }
        if (can_make_const && v._type.baseType == Type.tPointer && !v._type.flags.constant && !(v.access_flags.access_ref || v.access_flags.access_pass) && v._type.canCloneFromConst) {
            lint_error("LINT003: variable {v.name}: {describe(v._type)} can be made const (declare with 'let')", v.at)
            return
        }
    }

    def validate_argument(v : VariablePtr; code, kind : string) {
        if (noLint || genericDepth > 0 || v.flags.generated || v.flags.marked_used) return
        let name = string(v.name)
        if (name |> starts_with("_") || find(name, "`") >= 0) return
        if (v.isAccessUnused) {
            lint_error("{code}: unused {kind} {v.name}: {describe(v._type)} (add an underscore prefix or [unused_argument({v.name})] if intentional)", v.at)
        }
    }

    def validate_unwritten_var_argument(fun : FunctionPtr; v : VariablePtr) {
        if (noLint || genericDepth > 0 || v.flags.generated || v.flags.marked_used) return
        let name = string(v.name)
        if ((name |> starts_with("_")) || find(name, "`") >= 0
                || fun.name == "finalize" || fun.moreFlags.addressTaken
                || v._type == null || v._type.flags.constant
                || !(v._type.flags.ref || v._type.isRefType)
                || v.access_flags.access_ref || v.access_info.access_info_pass_mutable
                || v.isAccessUnused) {
            return
        }
        lint014_candidates |> push(v)
    }

    def lint029_collect_candidate(fun : FunctionPtr; v : VariablePtr) {
        if (lint029_exempt || noLint || genericDepth > 0 || v.flags.generated || v.flags.marked_used || v.isAccessUnused) return
        let name = string(v.name)
        if ((name |> starts_with("_")) || find(name, "`") >= 0
                || fun.name == "finalize" || fun.moreFlags.addressTaken
                || v._type == null || v._type.flags.constant
                || !(v._type.flags.ref || v._type.isRefType)
                || !(v.access_flags.access_ref || v.access_info.access_info_pass_mutable)) {
            return
        }
        lint029_candidates |> push(v)
    }

    def lint023_collect_candidate(v : VariablePtr) {
        if (noLint || genericDepth > 0 || v.flags.generated || v.flags.marked_used || v.isAccessUnused) return
        let name = string(v.name)
        if ((name |> starts_with("_")) || find(name, "`") >= 0
                || v._type == null || v._type.flags.constant
                || v._type.flags.ref || v._type.isRefType) {
            return
        }
        lint023_candidates |> push(Lint023Candidate(v = v, stores = 0, disqualified = false))
    }

    def lint023_find(v : Variable const?) : int {
        for (i in range(length(lint023_candidates))) {
            if (lint023_candidates[i].v == v) return i
        }
        return -1
    }

    def lint023_mark_store_target(lhs : ExpressionPtr) : void {
        if (lhs == null) return
        if (lhs is ExprVar) {
            lint023_store_nodes |> insert(intptr(lhs))
        } elif (lhs is ExprSwizzle) {
            let sw = lhs as ExprSwizzle
            if (sw.value != null && sw.value is ExprVar) {
                lint023_store_nodes |> insert(intptr(sw.value))
            }
        }
    }

    def lint023_note_use(expr : ExprVar?) : void {
        let ci = lint023_find(expr.variable)
        if (ci < 0 || lint023_candidates[ci].disqualified) return
        if (expr.varFlags.under_clone || key_exists(lint023_store_nodes, intptr(expr))) {
            lint023_candidates[ci].stores++
        } elif (lint023_candidates[ci].stores > 0 || lint023_deferred_depth > 0) {
            lint023_candidates[ci].disqualified = true
        }
    }

    def override preVisitFunctionArgument(fun : FunctionPtr; arg : VariablePtr; _lastArg : bool) : void {
        if (lint022_generic_sweep) {
            lint022_refs |> note_type_names(arg._type, 0)
        }
        if (arg._type != null && arg._type.flags.constant && !arg._type.flags._implicit
                && (arg._type.isRef || arg._type.isPointer)) {
            lint031_params |> push(arg)
        }
        if (fun.flags.isClassMethod) return
        for (ann in fun.annotations) {
            if (ann.annotation.name == "extern") return
        }
        validate_argument(arg, "LINT012", "function argument")
        validate_unwritten_var_argument(fun, arg)
        lint023_collect_candidate(arg)
        lint029_collect_candidate(fun, arg)
    }

    def lint031_note_write(target : Expression const?; at : LineInfo) : void {
        lint031_note_write_root(target, false, at)
    }

    def lint031_note_write_root(target : Expression const?; through : bool; at : LineInfo) : void {
        if (noLint || genericDepth > 0 || empty(lint031_params)) return
        let root = lint031_root(lint031_params, target, through)
        if (root == null || lint031_reported |> find_index(root) >= 0) return
        lint031_reported |> push(root)
        let kind = root._type.isPointer ? "pointer" : "reference"
        lint_error("LINT031: `{root.name}` is a const {kind} parameter the body writes through at line {int(at.line)} - the const was stripped on the way (a reinterpret, an intptr round trip, or a writable slot of a call), so the declared type denies a write the function performs; declare the parameter `var`", root.at)
    }

    //! the chase runs first: a callee body is scanned only for an argument rooted at a const parameter
    def lint031_note_call_argument(call : ExprCall?; arg : ExpressionPtr) : void {
        if (noLint || genericDepth > 0 || empty(lint031_params)) return
        if (lint031_root(lint031_params, arg, true) != null && lint031_slot_written(call, arg)) {
            lint031_note_write_root(arg, true, call.at)
        }
    }

    def private lint029_note_mutation(v : Variable const?) : void {
        if (v != null && lint029_candidates |> find_index(v) >= 0
                && lint029_mutated |> find_index(v) < 0) {
            lint029_mutated |> push(v)
        }
    }

    def override preVisitExprField(expr : ExprField?) : void {
        if (noLint || genericDepth > 0 || expr.value == null) return
        var obj = expr.value
        if (obj is ExprRef2Value) {
            obj = (obj as ExprRef2Value).subexpr
        }
        return if (!(obj is ExprVar) || (expr._type != null
            && (expr._type.baseType == Type.tPointer || expr._type.baseType == Type.tHandle)))
        lint029_note_mutation((obj as ExprVar).variable)
    }

    def override preVisitExprBlockArgument(_blk : ExprBlock?; arg : VariablePtr; _lastArg : bool) : void {
        validate_argument(arg, "LINT013", "block argument")
    }

    def lint017_long_twin(fname : string; icall : ExprCall?) : string {
        let nargs = length(icall.arguments)
        if (nargs == 0 || icall.arguments[0]._type == null) return ""
        let bt = icall.arguments[0]._type.baseType
        if (fname == "length" && (bt == Type.tArray || bt == Type.tTable || bt == Type.tString)) return "long_length"
        if (fname == "capacity" && (bt == Type.tArray || bt == Type.tTable)) return "long_capacity"
        if (fname == "count" && (bt == Type.tArray || bt == Type.tIterator)) return "long_count"
        if (fname == "find_index" && bt == Type.tArray && nargs == 2) return "long_find_index"
        if ((fname == "fread" || fname == "fwrite") && bt == Type.tPointer && nargs == 2) return "long_{fname}"
        return ""
    }

    def lint017_is_32bit_parse(fname : string; icall : ExprCall?) : bool {
        if (fname != "to_int" && fname != "to_uint") return false
        let nargs = length(icall.arguments)
        if (nargs == 0 || icall.arguments[0]._type == null) return false
        return icall.arguments[0]._type.baseType == Type.tString
    }

    // The remedy repeats the site's own string and hex arguments, and keeps a mixed-sign
    // site's outer cast, so only the parse width changes.
    def lint017_parse_remedy(cast_name : string; iname : string; icall : ExprCall?) : string {
        let str_arg = describe(icall.arguments[0])
        let hex = length(icall.arguments) >= 2 ? describe(icall.arguments[1]) : "false"
        let widened = "{iname}64({str_arg}, {hex})"
        let same_sign = cast_name == (iname == "to_int" ? "int64" : "uint64")
        return same_sign ? widened : "{cast_name}({widened})"
    }

    def check_lint017_32bit_cast(expr : ExprCall?) : void {
        if (noLint || genericDepth > 0
            || (expr.name != "int64" && expr.name != "uint64")
            || length(expr.arguments) != 1) return
        let inner = expr.arguments[0]
        if (!(inner is ExprCall)) return
        let icall = inner as ExprCall
        let iname = string(icall.func != null && icall.func.fromGeneric != null
            ? icall.func.fromGeneric.name
            : icall.name)
        let twin = lint017_long_twin(iname, icall)
        if (!empty(twin)) {
            lint_error("LINT017: {expr.name}({iname}(...)) widens an already-32-bit result, so the 2^31 limit is still hit inside {iname} (as a wrap, or as the panic guard on array/table/string length) before the cast runs; call {twin}(...) instead", expr.at)
            return
        }
        if (lint017_is_32bit_parse(iname, icall)) {
            lint_error("LINT017: {expr.name}({iname}(...)) widens a 32-bit string parse - {iname} answers 0 for any value that overflows 32 bits, so the cast asks for a range the parse cannot give; call {lint017_parse_remedy(string(expr.name), iname, icall)} instead", expr.at)
        }
    }

    def lint024_scan(expr : ExpressionPtr; var has_product : bool&; var has_call : bool&) : void {
        if (expr is ExprOp2) {
            let op2 = expr as ExprOp2
            let op = string(op2.op)
            if (op == "*") {
                has_product = true
            }
            if (op == "*" || op == "+" || op == "-") {
                lint024_scan(op2.left, has_product, has_call)
                lint024_scan(op2.right, has_product, has_call)
                return
            }
        }
        if (expr is ExprCall) {
            has_call = true
        }
    }

    def check_lint024_widened_product(expr : ExprCall?) : void {
        if (noLint || genericDepth > 0
            || (expr.name != "int64" && expr.name != "uint64")
            || length(expr.arguments) != 1) return
        let inner = expr.arguments[0]
        if (inner._type == null || !(inner is ExprOp2)) return
        let bt = inner._type.baseType
        if (bt != Type.tInt && bt != Type.tUInt) return
        var has_product = false
        var has_call = false
        lint024_scan(inner, has_product, has_call)
        if (!has_product || !has_call) return
        lint_error("LINT024: {expr.name}(...) widens a product that already wrapped - every factor is 32-bit, so the multiply overflows at 2^31 before the cast runs; widen a factor instead: {expr.name}(a) * b", expr.at)
    }

    def lint018_check_arg(expr : ExprCall?; fname : string; idx : int; isMem : bool) : bool {
        let a = expr.arguments[idx]
        if (!(a is ExprCall)) return false
        let scall = a as ExprCall
        if (scall.name != "int" || length(scall.arguments) != 1) return false
        let src = scall.arguments[0]._type
        if (src == null) return false
        let bt = src.baseType
        if (isMem) {
            if (bt != Type.tInt64 && bt != Type.tUInt64 && bt != Type.tUInt) return false
            lint_error("LINT018: int(...) truncates the {fname} size above 2^31; {fname} has uint/int64/uint64 overloads, so drop the cast", expr.at)
        } elif (bt == Type.tInt64) {
            lint_error("LINT018: int(...) truncates the {fname} argument above 2^31; {fname} has an int64 overload, so drop the cast", expr.at)
        } elif (bt == Type.tUInt64) {
            lint_error("LINT018: int(...) truncates the {fname} argument above 2^31; {fname} has an int64 overload - widen with int64(...) instead of narrowing", expr.at)
        } else {
            return false
        }
        return true
    }

    def check_lint018_mem_size_narrowing(expr : ExprCall?) : void {
        if (noLint || genericDepth > 0) return
        let fname = string(expr.func != null && expr.func.fromGeneric != null
            ? expr.func.fromGeneric.name
            : expr.name)
        let nargs = length(expr.arguments)
        if ((fname == "memcpy" || fname == "memmove" || fname == "memcmp") && nargs == 3) {
            lint018_check_arg(expr, fname, 2, true)
            return
        }
        if (nargs < 2 || expr.arguments[0]._type == null) return
        let bt = expr.arguments[0]._type.baseType
        if (bt == Type.tArray) {
            if (nargs == 2 && (fname == "resize" || fname == "resize_no_init"
                    || fname == "resize_and_init" || fname == "reserve" || fname == "erase")) {
                lint018_check_arg(expr, fname, 1, false)
            } elif (nargs == 3 && fname == "resize_and_init") {
                lint018_check_arg(expr, fname, 1, false)
            } elif (nargs == 3 && fname == "erase") {
                if (!lint018_check_arg(expr, fname, 1, false)) {
                    lint018_check_arg(expr, fname, 2, false)
                }
            }
        } elif (bt == Type.tTable) {
            if (nargs == 2 && fname == "reserve") {
                lint018_check_arg(expr, fname, 1, false)
            }
        }
    }

    def check_lint020_range_narrowing(expr : ExprCall?) : void {
        if (noLint || genericDepth > 0) return
        let isRange = expr.name == "range"
        let isURange = expr.name == "urange"
        let isInterval = expr.name == "interval"
        if (!isRange && !isURange && !isInterval) return
        let nargs = length(expr.arguments)
        if (nargs < 1 || nargs > 2) return
        for (a in expr.arguments) {
            if (!(a is ExprCall)) { continue; }
            let scall = a as ExprCall
            if ((scall.name != "int" && scall.name != "uint") || length(scall.arguments) != 1) { continue; }
            let src = scall.arguments[0]._type
            if (src == null || (src.baseType != Type.tInt64 && src.baseType != Type.tUInt64)) { continue; }
            if (isInterval) {
                lint_error("LINT020: {scall.name}(...) truncates a 64-bit bound above 2^31; interval has an int64 form, so drop the cast", expr.at)
            } else {
                let twin = isRange ? "range64" : "urange64"
                lint_error("LINT020: {expr.name}({scall.name}(...)) truncates a 64-bit bound above 2^31; use {twin}(...) on the 64-bit values directly - arrays and pointers index with int64/uint64 as-is", expr.at)
            }
            return
        }
    }

    def lint021_is_narrow_cast(expr : ExprCall?) : bool {
        if ((expr.name != "int" && expr.name != "uint") || length(expr.arguments) != 1) return false
        let t = expr.arguments[0]._type
        return t != null && (t.baseType == Type.tInt64 || t.baseType == Type.tUInt64)
    }

    def lint021_is_transparent(expr : ExprCall?) : bool {
        let fname = string(expr.func != null && expr.func.fromGeneric != null
            ? expr.func.fromGeneric.name
            : expr.name)
        return fname == "max" || fname == "abs"
    }

    //! the variables `call` writes through a mutable by-reference parameter it writes during the
    //! call - a bare variable in a `var` slot of `&` or `isRefType` type - into `slot` of the frame
    //! that holds `call` as an argument
    def private lint030_note_writes(call : ExprCall const?; slot : int; var into : array<Lint030Write>) : void {
        return if (call.func == null)
        for (k in range(min(length(call.arguments), length(call.func.arguments)))) {
            let p = call.func.arguments[k]
            continue if (p._type == null || p._type.flags.constant || !(p._type.flags.ref || p._type.isRefType)
                || !lint030_callee_writes(call.func, k))
            let v = lint030_bare_var(call.arguments[k])
            if (v != null) {
                into |> push(Lint030Write(slot = slot, v = v, callee = "{call.name}"))
            }
        }
    }

    //! whether argument `slot` of `call` is `v` itself, or a plain field of it, handed over by
    //! reference: the callee then sees its final state whatever the order, so that read is not
    //! a dependency
    def private lint030_passed_by_ref(call : ExprCall const?; slot : int; v : Variable const?) : bool {
        return false if (call.func == null || slot >= length(call.func.arguments))
        let p = call.func.arguments[slot]
        return p._type != null && (p._type.flags.ref || p._type.isRefType) && lint030_field_root(call.arguments[slot]) == v
    }

    def private lint030_check(frame : Lint030Frame) : void {
        return if (noLint || genericDepth > 0 || frame.call == null)
        var reported : table<int>
        for (w in frame.writes) {
            continue if (key_exists(reported, w.slot))
            for (m in frame.mentions) {
                continue if (m.slot == w.slot || m.v != w.v || lint030_passed_by_ref(frame.call, m.slot, m.v))
                lint_error("LINT030: argument {m.slot + 1} reads `{m.v.name}` while the call `{w.callee}(...)` in argument {w.slot + 1} writes it by reference - the order arguments are evaluated in is not defined, so the read sees the value before or after the write by the compiler's choice; run the writing call as a statement of its own first", frame.call.at)
                reported |> insert(w.slot)
                break
            }
        }
    }

    def override preVisitExprCallArgument(expr : ExprCall?; arg : ExpressionPtr; _last : bool) : void {
        if (!empty(lint030_frames)) {
            lint030_frames[length(lint030_frames) - 1].slot++
        }
        lint031_note_call_argument(expr, arg)
    }

    def override preVisitExprCall(expr : ExprCall?) : void {
        lint030_frames |> emplace(Lint030Frame(call = expr, slot = -1, closure_depth = lint021_closure_depth))
        var kind = Lint021CallKind.plain
        if (!noLint && genericDepth == 0) {
            if (lint021_is_narrow_cast(expr)) {
                kind = Lint021CallKind.narrowing_cast
                lint021_narrow_depth++
            } elif (lint021_narrow_depth > 0 && !lint021_is_transparent(expr)) {
                kind = Lint021CallKind.poison
                lint021_poison_depth++
            }
        }
        lint021_call_kind |> push(kind)
        lint022_note(expr.func, "{expr.name}")
        check_string_push_clone(expr)
        check_lint017_32bit_cast(expr)
        check_lint024_widened_product(expr)
        check_lint018_mem_size_narrowing(expr)
        check_lint020_range_narrowing(expr)
    }

    def override visitExprCall(var expr : ExprCall?) : ExpressionPtr {
        if (!empty(lint030_frames)) {
            var frame <- lint030_frames[length(lint030_frames) - 1]
            lint030_frames |> pop()
            lint030_check(frame)
            if (!empty(lint030_frames)) {
                let parent = length(lint030_frames) - 1
                let slot = lint030_frames[parent].slot
                if (slot >= 0 && frame.closure_depth == lint030_frames[parent].closure_depth) {
                    for (m in frame.mentions) {
                        lint030_frames[parent].mentions |> push(Lint030Mention(slot = slot, v = m.v))
                    }
                    for (w in frame.writes) {
                        lint030_frames[parent].writes |> push(Lint030Write(slot = slot, v = w.v, callee = w.callee))
                    }
                    lint030_note_writes(expr, slot, lint030_frames[parent].writes)
                }
            }
        }
        if (!empty(lint021_call_kind)) {
            let kind = lint021_call_kind |> back()
            lint021_call_kind |> pop()
            if (kind == Lint021CallKind.narrowing_cast) {
                lint021_narrow_depth--
            } elif (kind == Lint021CallKind.poison) {
                lint021_poison_depth--
            }
        }
        return <- expr
    }

    def lint022_note(fn : Function?; raw_name : string) : void {
        lint022_refs |> note_ref(fn, raw_name)
    }

    def override preVisitStructure(st : StructurePtr) : void {
        lint022_self_struct = st
    }

    def override visitStructure(var st : StructurePtr) : StructurePtr {
        lint022_self_struct = null
        return <- st
    }

    def override preVisitExpression(expr : ExpressionPtr) : void {
        lint022_refs |> note_type(expr._type, 0, lint022_self_struct)
        if (lint022_generic_sweep) {
            lint022_refs |> note_type_names(expr._type, 0)
        }
        if (expr is ExprConstString && !expr.genFlags.generated) {
            let nm = string((expr as ExprConstString).value)
            if (!empty(nm)) {
                lint022_refs.names |> insert(nm)
            }
        }
    }

    def override preVisitExprAddr(expr : ExprAddr?) : void {
        lint022_note(expr.func, "{expr.target}")
    }

    def override preVisitExprNew(expr : ExprNew?) : void {
        if (lint022_generic_sweep) {
            lint022_refs |> note_type_names(expr.typeexpr, 0)
        }
        lint022_note(expr.func, "{expr.name}")
    }

    def override preVisitExprOp1(expr : ExprOp1?) : void {
        lint022_note(expr.func, "{expr.name}")
        if (lint030_steps(string(expr.op))) {
            lint031_note_write(expr.subexpr, expr.at)
        }
    }

    def lint022_is_unreferenced(fn : Function?; mod : Module?) : bool {
        return (!lint022_is_skipped(fn, mod)
            && !(lint022_refs |> knows(fn, "{fn.name}")))
    }

    def lint022_collect_generic_refs(mod : Module?) : void {
        let saved_noLint = noLint
        lint022_generic_sweep = true
        make_visitor(self) $(adapter) {
            for_each_generic(mod, "") $(gfn) {
                visit(gfn, adapter)
            }
        }
        lint022_generic_sweep = false
        noLint = saved_noLint
    }

    def lint022_note_decl_types(mod : Module?) : void {
        for_each_function(mod, "") $(fn) {
            return if (fn.flags.generated)
            let self_st = (fn.flags.isClassMethod && !empty(fn.arguments)
                && fn.arguments[0]._type != null) ? fn.arguments[0]._type.structType : null
            for (a in fn.arguments) {
                lint022_refs |> note_type(a._type, 0, self_st)
            }
            lint022_refs |> note_type(fn.result, 0, self_st)
        }
        for_each_generic(mod, "") $(gfn) {
            for (a in gfn.arguments) {
                lint022_refs |> note_type(a._type, 0)
            }
            lint022_refs |> note_type(gfn.result, 0)
        }
        for_each_structure(mod) $(st) {
            if (st.parent != null) {
                lint022_refs.structs |> insert(st.parent)
            }
            for (f in st.fields) {
                lint022_refs |> note_type(f._type, 0, st)
            }
        }
        for_each_global(mod) $(v) {
            lint022_refs |> note_type(v._type, 0)
        }
        for_each_typedef(mod) $(_name, td) {
            lint022_refs |> note_type(td, 0)
        }
    }

    def lint022_struct_unreferenced(st : Structure?; mod : Module?) : bool {
        return (st != null && st.flags.privateStructure && !st.flags.generated
            && length(st.annotations) == 0
            && st.at.fileInfo != null && st._module == mod
            && !key_exists(lint022_refs.structs, st)
            && !key_exists(lint022_refs.names, "{st.name}"))
    }

    def lint022_enum_unreferenced(en : Enumeration?; mod : Module?) : bool {
        return (en != null && en.isPrivate
            && length(en.annotations) == 0
            && en.at.fileInfo != null && en._module == mod
            && !key_exists(lint022_refs.enums, en)
            && !key_exists(lint022_refs.names, "{en.name}"))
    }

    def lint022_has_candidate(prog : ProgramPtr; mod : Module?; check_functions : bool) : bool {
        var any = false
        if (check_functions) {
            for_each_function(mod, "") $(fn) {
                if (lint022_is_unreferenced(fn, mod)) {
                    any = true
                }
            }
        }
        for_each_structure(mod) $(st) {
            if (lint022_struct_unreferenced(st, mod)) {
                any = true
            }
        }
        for_each_enumeration(mod) $(en) {
            if (lint022_enum_unreferenced(en, mod)) {
                any = true
            }
        }
        return any
    }

    def lint022_check_unused_private(prog : ProgramPtr) : void {
        return if (key_exists(disabled_codes, "LINT022")
                    || (!empty(enabled_codes) && !key_exists(enabled_codes, "LINT022"))
                    || !lint022_enabled(prog))
        let mod = prog.getThisModule
        return if (mod == null)
        let check_functions = !lint022_calls_may_be_inlined(prog)
        let skip_block_functions = lint022_block_calls_may_be_inlined(prog)
        lint022_note_decl_types(mod)
        return if (!lint022_has_candidate(prog, mod, check_functions))
        lint022_collect_generic_refs(mod)
        if (check_functions) {
            for_each_function(mod, "") $(fn) {
                return if (!lint022_is_unreferenced(fn, mod) || (skip_block_functions && lint022_takes_block(fn)))
                lint022_report_at_decl("LINT022: private function '{fn.name}' is never used; remove it, or make it public if another module needs it", fn.at)
            }
        }
        for_each_structure(mod) $(st) {
            return if (!lint022_struct_unreferenced(st, mod))
            let kind = st.flags.isClass ? "class" : "structure"
            lint022_report_at_decl("LINT022: private {kind} '{st.name}' is never used; remove it, or make it public if another module needs it", st.at)
        }
        for_each_enumeration(mod) $(en) {
            return if (!lint022_enum_unreferenced(en, mod))
            lint022_report_at_decl("LINT022: private enumeration '{en.name}' is never used; remove it, or make it public if another module needs it", en.at)
        }
    }

    def lint022_report_at_decl(msg : string; at : LineInfo) : void {
        //! Reports at a declaration no function walk visited: retargets ``funcFile`` so ``lint_error``'s own-file gate admits a symbol from any file of the module.
        funcFile = at.fileInfo != null ? "{at.fileInfo.name}" : ""
        lint_error(msg, at)
    }

    def override canVisitWithAliasSubexpression(_expr : ExprAssume?) : bool {
        return false
    }

    def override preVisitExprLet(expr : ExprLet?) : void {
        for (v in expr.variables) {
            validate_var(v, true)
        }
    }

    def is_const_zero(expr : ExpressionPtr) : bool {
        if (expr == null) return false
        if (expr is ExprConstInt) return (expr as ExprConstInt).value == 0
        if (expr is ExprConstUInt) return (expr as ExprConstUInt).value == 0u
        if (expr is ExprConstInt64) return (expr as ExprConstInt64).value == 0l
        if (expr is ExprConstUInt64) return (expr as ExprConstUInt64).value == 0ul
        if (expr is ExprConstFloat) return (expr as ExprConstFloat).value == 0.0f
        if (expr is ExprConstDouble) return (expr as ExprConstDouble).value == 0.0lf
        return false
    }

    def expr_equal(a : ExpressionPtr; b : ExpressionPtr; require_pure : bool = true) : bool {
        if (a == null || b == null
            || (require_pure && (!a.flags.noSideEffects || !b.flags.noSideEffects))) return false
        return describe(a) == describe(b)
    }

    def override preVisitExprOp2(expr : ExprOp2?) : void {
        lint022_note(expr.func, "{expr.name}")
        if (noLint || genericDepth > 0) return
        let op = string(expr.op)
        if (length(op) >= 2 && op |> ends_with("=") && op != "==" && op != "!=" && op != "<=" && op != ">=") {
            lint023_mark_store_target(expr.left)
            lint031_note_write(expr.left, expr.at)
        }
        if ((op == "/" || op == "%" || op == "/=" || op == "%=") && is_const_zero(expr.right)) {
            lint_error("LINT006: division by zero ({op})", expr.at)
            return
        }
        let op_same_suspicious = (
            op == "==" || op == "!=" ||
            op == "<"  || op == ">"  ||
            op == "<=" || op == ">=" ||
            op == "-"  || op == "/"  || op == "%" ||
            op == "&&" || op == "||" ||
            op == "&"  || op == "|"  || op == "^" ||
            op == "-=" || op == "/=" || op == "%="
        )
        if (op_same_suspicious && expr_equal(expr.left, expr.right, false)
                && !(expr.left.__rtti |> starts_with("ExprConst"))) {
            lint_error("LINT007: left and right operands of '{op}' are the same", expr.at)
        }
    }

    def override preVisitExprOp3(expr : ExprOp3?) : void {
        lint022_note(expr.func, "{expr.name}")
        if (noLint || genericDepth > 0) return
        if (expr_equal(expr.left, expr.right, false)) {
            lint_error("LINT008: both branches of ternary '?:' are equivalent", expr.at)
        }
    }

    def override preVisitExprIfThenElse(expr : ExprIfThenElse?) : void {
        if (noLint || genericDepth > 0) return
        if (expr.if_false != null && expr_equal(expr.if_true, expr.if_false, false)) {
            lint_error("LINT009: 'then' branch is equivalent to 'else' branch", expr.at)
        }
        branch_depth++
    }
    def override visitExprIfThenElse(var expr : ExprIfThenElse?) : ExpressionPtr {
        if (!noLint && genericDepth == 0) {
            branch_depth--
        }
        return <- expr
    }

    //! stands LINT010 and LINT023 down for the rest of this function (ARCHITECTURE_LINT.md, LINT023)
    def private note_flow_jump() : void {
        if (noLint || genericDepth > 0) return
        flow_jumps = true
        for (e in ds_entries) {
            e.skip = true
        }
    }

    def private ds_find(v : VariablePtr) : int {
        let vc : Variable const? = v
        for (i in range(length(ds_entries))) {
            if (ds_entries[i].v == vc) return i
        }
        return -1
    }

    def private lint021_find(v : VariablePtr) : int {
        let vc : Variable const? = v
        for (i in range(length(lint021_candidates))) {
            if (lint021_candidates[i].v == vc) return i
        }
        return -1
    }

    def override preVisitExprLetVariable(_let : ExprLet?; v : VariablePtr; _last : bool) : void {
        if (lint022_generic_sweep) {
            lint022_refs |> note_type_names(v._type, 0)
        }
        check_string_clone_init(v)
        if (noLint || genericDepth > 0) return
        if (v._type != null && !v._type.flags.ref && v.init != null && !v.flags.generated
                && (v._type.baseType == Type.tInt64 || v._type.baseType == Type.tUInt64)) {
            lint021_candidates |> emplace(Lint021Candidate(v = v, safe_uses = 0, disqualified = false))
        }
        if ((v._type != null && v._type.flags.ref)
            || v.flags.inScope
            || v.access_flags.access_fold
            || !v.access_flags.access_get) return
        let has_init = v.init != null
        let init_pure = has_init && v.init.flags.noSideEffects
        ds_entries |> emplace(DeadStoreEntry(
            v = v,
            pending_expr = v.init,
            skip = flow_jumps,
            has_pending = has_init,
            pending_pure = init_pure
        ))
    }

    def override preVisitGlobalLetVariable(v : VariablePtr; _last : bool) : void {
        check_string_clone_init(v)
    }

    def override preVisitExprVar(expr : ExprVar?) : void {
        if (lint022_generic_sweep) {
            lint022_refs.names |> insert("{expr.name}")
        }
        if (!empty(lint030_frames)) {
            let top = length(lint030_frames) - 1
            if (lint030_frames[top].slot >= 0 && lint030_frames[top].closure_depth == lint021_closure_depth) {
                lint030_frames[top].mentions |> push(Lint030Mention(slot = lint030_frames[top].slot, v = expr.variable))
            }
        }
        if (noLint || genericDepth > 0) return
        if (key_exists(lint023_store_nodes, intptr(expr))) {
            lint029_note_mutation(expr.variable)
        }
        lint023_note_use(expr)
        let ci = lint021_find(expr.variable)
        if (ci >= 0 && !lint021_candidates[ci].disqualified) {
            if (expr.varFlags.write || lint021_closure_depth > 0
                    || lint021_narrow_depth == 0 || lint021_poison_depth > 0) {
                lint021_candidates[ci].disqualified = true
            } else {
                lint021_candidates[ci].safe_uses++
            }
        }
        let i = ds_find(expr.variable)
        if (i < 0 || ds_entries[i].skip) return
        let vf = expr.varFlags
        if (branch_depth > 0 || (vf.write && !vf.under_clone)) {
            ds_entries[i].skip = true
            return
        }
        if (vf.write) return
        ds_entries[i].has_pending = false
    }

    def private ds_handle_store(lhs : ExpressionPtr; rhs : ExpressionPtr; var store_expr : ExpressionPtr) : void {
        if (lhs == null || !(lhs is ExprVar)) return
        let ev = lhs as ExprVar
        let i = ds_find(ev.variable)
        if (i < 0 || ds_entries[i].skip || branch_depth > 0) return
        if (ds_entries[i].has_pending && ds_entries[i].pending_pure && ds_entries[i].pending_expr != null) {
            let vname = "{ev.variable.name}"
            lint_error("LINT010: dead store of '{vname}': overwritten without intervening read", ds_entries[i].pending_expr.at)
        }
        ds_entries[i].has_pending = true
        ds_entries[i].pending_expr = store_expr
        ds_entries[i].pending_pure = rhs != null && rhs.flags.noSideEffects
    }

    def override preVisitExprCopy(expr : ExprCopy?) : void {
        lint023_mark_store_target(expr.left)
        lint031_note_write(expr.left, expr.at)
    }
    def override preVisitExprClone(expr : ExprClone?) : void {
        lint023_mark_store_target(expr.left)
        lint031_note_write(expr.left, expr.at)
    }
    def override preVisitExprMove(expr : ExprMove?) : void {
        lint023_mark_store_target(expr.left)
        lint031_note_write(expr.left, expr.at)
    }
    def override visitExprCopy(var expr : ExprCopy?) : ExpressionPtr {
        check_string_clone_assign(expr)
        ds_handle_store(expr.left, expr.right, expr)
        return <- expr
    }
    def override visitExprClone(var expr : ExprClone?) : ExpressionPtr {
        ds_handle_store(expr.left, expr.right, expr)
        return <- expr
    }
    def override visitExprMove(var expr : ExprMove?) : ExpressionPtr {
        ds_handle_store(expr.left, expr.right, expr)
        return <- expr
    }

    def override preVisitExprGoto(_expr : ExprGoto?) : void {
        note_flow_jump()
    }
    def override preVisitExprFor(_expr : ExprFor?) : void {
        if (!noLint && genericDepth == 0) {
            branch_depth++
            lint023_deferred_depth++
        }
    }
    def override visitExprFor(var expr : ExprFor?) : ExpressionPtr {
        if (!noLint && genericDepth == 0) {
            branch_depth--
            lint023_deferred_depth--
        }
        return <- expr
    }
    def override preVisitExprWhile(_expr : ExprWhile?) : void {
        if (!noLint && genericDepth == 0) {
            branch_depth++
            lint023_deferred_depth++
        }
    }
    def override visitExprWhile(var expr : ExprWhile?) : ExpressionPtr {
        if (!noLint && genericDepth == 0) {
            branch_depth--
            lint023_deferred_depth--
        }
        return <- expr
    }
    def override preVisitExprTryCatch(_expr : ExprTryCatch?) : void {
        if (!noLint && genericDepth == 0) branch_depth++
    }
    def override visitExprTryCatch(var expr : ExprTryCatch?) : ExpressionPtr {
        if (!noLint && genericDepth == 0) branch_depth--
        return <- expr
    }
    def override visitExprConstFloat(var expr : ExprConstFloat?) : ExpressionPtr {
        if (!noLint && genericDepth == 0 && expr.inexactFloatPromotion) {
            lint_error("LINT011: int literal does not exactly represent as float", expr.at)
        }
        return <- expr
    }
    def override visitExprConstDouble(var expr : ExprConstDouble?) : ExpressionPtr {
        if (!noLint && genericDepth == 0 && expr.inexactFloatPromotion) {
            lint_error("LINT011: int literal does not exactly represent as double", expr.at)
        }
        return <- expr
    }
}

def public paranoid(prog : ProgramPtr; compile_time_errors : bool) {
    //! Runs the paranoid lint visitor on the program to check for common coding issues.
    let empty_set : table<string>
    paranoid(prog, compile_time_errors, empty_set)
}

def public paranoid(prog : ProgramPtr; compile_time_errors : bool; disabled_codes : table<string>) {
    //! Filter-aware compile-time variant. ``disabled_codes`` suppresses
    //! the listed rule IDs (e.g. "LINT003") alongside the usual
    //! ``// nolint:CODE`` directives.
    var astVisitor = new LintVisitor(
        compile_time_errors = compile_time_errors,
        lint029_exempt = lint029_source_exempt(prog),
        multiple_contexts = prog._options |> find_arg("multiple_contexts") ?as tBool ?? prog.policies.multiple_contexts)
    astVisitor.disabled_codes := disabled_codes
    make_visitor(*astVisitor) $(adapter) {
        visit(prog, adapter)
    }
    astVisitor->lint022_check_unused_private(prog)
    unsafe {
        delete astVisitor
    }
}

def public paranoid_collect(prog : ProgramPtr; var errors : array<string>) : int {
    //! Runs the paranoid lint visitor and collects errors as strings.
    //! Returns the number of lint issues found.
    let empty_set : table<string>
    return paranoid_collect(prog, errors, empty_set, empty_set)
}

def public paranoid_collect(prog : ProgramPtr; var errors : array<string>;
                            disabled_codes, enabled_codes : table<string>) : int {
    //! Filter-aware overload. `disabled_codes` is a denylist; `enabled_codes`
    //! is a whitelist (empty == all). Codes use the bare form (e.g. "LINT002").
    var astVisitor = new LintVisitor(
        compile_time_errors = false,
        collect_errors = true,
        lint029_exempt = lint029_source_exempt(prog),
        multiple_contexts = prog._options |> find_arg("multiple_contexts") ?as tBool ?? prog.policies.multiple_contexts)
    astVisitor.disabled_codes := disabled_codes
    astVisitor.enabled_codes := enabled_codes
    make_visitor(*astVisitor) $(adapter) {
        visit(prog, adapter)
    }
    astVisitor->lint022_check_unused_private(prog)
    let count = length(astVisitor.errors)
    errors |> push_from(astVisitor.errors)
    unsafe {
        delete astVisitor
    }
    return count
}

def public paranoid_collect_issues(prog : ProgramPtr; var issues : array<LintIssue>;
                                   disabled_codes, enabled_codes : table<string>) : int {
    //! Structured twin of ``paranoid_collect`` — same filters, but emits
    //! ``LintIssue`` records (rule code + position) instead of display strings.
    var astVisitor = new LintVisitor(
        compile_time_errors = false,
        collect_errors = true,
        lint029_exempt = lint029_source_exempt(prog),
        multiple_contexts = prog._options |> find_arg("multiple_contexts") ?as tBool ?? prog.policies.multiple_contexts)
    astVisitor.disabled_codes := disabled_codes
    astVisitor.enabled_codes := enabled_codes
    make_visitor(*astVisitor) $(adapter) {
        visit(prog, adapter)
    }
    astVisitor->lint022_check_unused_private(prog)
    let count = length(astVisitor.issues)
    issues |> push_from(astVisitor.issues)
    unsafe {
        delete astVisitor
    }
    return count
}

def private is_nolint_space(b : uint8) : bool {
    return int(b) == ' ' || int(b) == '\t'
}

def private comment_start_outside_literals(d : array<uint8> const# implicit) : int {
    let n = length(d)
    var p = 0
    while (p < n) {
        let c = int(d[p])
        if (c == '"' || c == '\'') {
            p++
            while (p < n && int(d[p]) != c) {
                p += (int(d[p]) == '\\' && p + 1 < n) ? 2 : 1
            }
            p++
        } elif (c == '/' && p + 1 < n && int(d[p + 1]) == '/') {
            return p
        } else {
            p++
        }
    }
    return -1
}

def private stale_scan_line(bytes : array<uint8> const# implicit; fname : string; line_no : int;
                            disabled_codes, enabled_codes : table<string>;
                            var stale_codes : array<string>; var self_suppressed : bool&) : void {
    let n = length(bytes)
    var p = comment_start_outside_literals(bytes)
    return if (p < 0)
    p += 2
    while (p < n && (int(bytes[p]) == '/' || int(bytes[p]) == '!')) {
        p++
    }
    while (p < n && is_nolint_space(bytes[p])) {
        p++
    }
    return if (find(bytes, "nolint:", p) != p)
    while (p < n) {
        while (p < n && is_nolint_space(bytes[p])) {
            p++
        }
        break if (p >= n)
        if (find(bytes, "nolint:", p) == p) {
            p += length("nolint:")
            while (p < n) {
                while (p < n && is_nolint_space(bytes[p])) {
                    p++
                }
                let tok_start = p
                while (p < n && int(bytes[p]) != ',' && !is_nolint_space(bytes[p])) {
                    p++
                }
                break if (p == tok_start)
                let code = slice(bytes, tok_start, p)
                if (code == "LINT019") {
                    self_suppressed = true
                } elif (is_rule_code(code)
                        && !key_exists(disabled_codes, code)
                        && (empty(enabled_codes) || key_exists(enabled_codes, code))
                        && !was_nolint_consumed(fname, line_no, code)) {
                    stale_codes |> push(code)
                }
                while (p < n && is_nolint_space(bytes[p])) {
                    p++
                }
                break if (p >= n || int(bytes[p]) != ',')
                p++
            }
        } else {
            while (p < n && !is_nolint_space(bytes[p])) {
                p++
            }
        }
    }
}

//! LINT019 - stale nolint detection: reports every leading ``// nolint:CODE`` directive in
//! `source` (recognition rule: stale_scan_line) whose code suppressed nothing this process.
//! Call AFTER all lint passes ran; a partial run under-consumes and would lie.
def public stale_nolint_collect(fname, source : string; disabled_codes, enabled_codes : table<string>; var issues : array<LintIssue>) : int {
    return 0 if (key_exists(disabled_codes, "LINT019")
                    || (!empty(enabled_codes) && !key_exists(enabled_codes, "LINT019")))
    var count = 0
    var line_no = 0
    var stale_codes : array<string>
    stale_codes |> reserve(4)
    for (raw_line in split(source, "\n")) {
        line_no++
        let line = raw_line |> ends_with("\r") ? slice(raw_line, 0, length(raw_line) - 1) : raw_line
        continue if (find(line, "nolint:") < 0)
        stale_codes |> clear()
        var self_suppressed = false
        peek_data(line) $(bytes) {
            stale_scan_line(bytes, fname, line_no, disabled_codes, enabled_codes,
                stale_codes, self_suppressed)
        }
        if (!self_suppressed) {
            issues |> reserve(length(issues) + length(stale_codes))
            for (code in stale_codes) {
                let msg = "LINT019: stale nolint - {code} suppresses nothing on this line; remove it, or add LINT019 to the list if the rule fires only in downstream compiles (generic bodies - instantiations elsewhere fire at the generic's line - macro-template lines, option-gated rules)"
                issues |> push(LintIssue(code = "LINT019", message = msg, file = fname,
                    line = line_no, column = 0, last_line = line_no, last_column = 0))
                count++
            }
        }
    }
    return count
}

def private is_rule_code(code : string) : bool {
    var prefix_len = 0
    if (code |> starts_with("LINT") || code |> starts_with("PERF")) {
        prefix_len = 4
    } elif (code |> starts_with("STYLE")) {
        prefix_len = 5
    } else {
        return false
    }
    var digits = 0
    var ok = true
    peek_data(code) $(bytes) {
        let n = length(bytes)
        for (i in range(prefix_len, n)) {
            if (!is_number(int(bytes[i]))) {
                ok = false
            }
            digits++
        }
    }
    return ok && digits > 0
}
