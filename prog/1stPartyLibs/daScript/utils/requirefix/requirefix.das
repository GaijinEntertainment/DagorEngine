module requirefix

options gen2

require daslib/ast
require daslib/strings_boost

struct RequiredModule {
    mod : Module?
    mod_name : string
    req_name : string
    is_public : bool

    line : int
}

struct ModuleUsageResult {
    mod : Module const?
    name : string     // Short module name (e.g. "safe")
    req_name : string // Full require path (e.g. "ecs.safe")
    is_public : bool
    tag : string
    refs : array<ModuleRef>
    required_at : int
    provides : table<string>
}

enum ModuleRefKind {
    variable
    function_call
    annotation
    overloaded_operator
    type_enum
    type_struct
    type_handle
    type_parent
    pub_require

    // A dummy tag to mark requires that can't be removed,
    // but we are not sure why.
    unknown

    // Require provides [init] side effects.
    side_effect_init
}

struct ModuleRef {
    kind : ModuleRefKind
    key : string // A func/var/type name
    line : int
}

def ref_function_call(name : string; line : uint) : ModuleRef {
    return ModuleRef(kind = ModuleRefKind.function_call, key = name, line = int(line))
}

def ref_variable(name : string; line : uint) : ModuleRef {
    return ModuleRef(kind = ModuleRefKind.variable, key = name, line = int(line))
}

def ref_annotation(name : string; line : uint) : ModuleRef {
    return ModuleRef(kind = ModuleRefKind.annotation, key = name, line = int(line))
}

def ref_overloaded_operator(name : string; line : uint) : ModuleRef {
    return ModuleRef(kind = ModuleRefKind.overloaded_operator, key = name, line = int(line))
}

def ref_pub_require(name : string; line : uint) : ModuleRef {
    return ModuleRef(kind = ModuleRefKind.pub_require, key = name, line = int(line))
}

struct RequirefixConfig {
    target : string

    require_roots : table<string, string>

    project_file : string = ""

    // Read a file and return its contents as a string.
    // Return empty string if the file cannot be read.
    read_file_fn : lambda<(filename : string) : string>
    abs_filename_fn : lambda<(filename : string) : string>
}

struct ModuleAnalysisResult {
    modules : array<ModuleUsageResult>
    err : string
}

struct RequirefixContext {
    filename : string
    mod_name : string
    last_error : string

    project_file : string = ""
    read_file_fn : lambda<(filename : string) : string>
    abs_filename_fn : lambda<(filename : string) : string>

    require_roots : table<string, string>

    // Maps a transitive module name to all direct requires that provide it.
    // For example, if both A and B do "require math public",
    // then providers["math"] = ["A", "B"].
    // See collect_public_deps for more info.
    providers : table<string, array<string>>
}

def public analyze_module_usages(var config : RequirefixConfig) : ModuleAnalysisResult {
    var result : ModuleAnalysisResult
    var ctx = new RequirefixContext()
    ctx.project_file = config.project_file
    ctx.read_file_fn <- config.read_file_fn
    ctx.abs_filename_fn <- config.abs_filename_fn
    ctx.filename = config.target
    for (filename, replacement in keys(config.require_roots), values(config.require_roots)) {
        ctx.require_roots |> insert(ctx.abs_filename_fn(filename), replacement)
    }

    using() $(var mg : ModuleGroup) {
        using() $(var cop : CodeOfPolicies) {
            cop.ignore_shared_modules = true
            cop.no_optimizations = true
            cop.no_infer_time_folding = true
            cop.lint_check = true
            cop.export_all = true

            result.modules <- analyze_file(ctx, mg, cop)
        }
    }

    result.err = ctx.last_error
    return <- result
}

def is_builtin_like(name : string) : bool {
    return name == "$" || name == "builtin"
}

def is_builtin_like(name : das_string) : bool {
    return name == "$" || name == "builtin"
}

def collect_public_deps(mod : Module?; root_mod : string; var exported : table<string>) {
    module_for_each_dependency(mod) $(dep, is_pub) {
        if (!is_pub) {
            return
        }
        if (is_builtin_like(dep.name) || dep.name == "") {
            return
        }
        let dep_name = "{dep.name}"
        if (!key_exists(exported, dep_name)) {
            exported |> insert(dep_name)
            collect_public_deps(dep, root_mod, exported)
        }
    }
}

def collect_required_modules(prog : smart_ptr<Program>) : array<RequiredModule> {
    var result : array<RequiredModule>
    prog |> for_each_require_declaration() $ [unused_argument(file)] (mod, req_name, file, pub, at) {
        var rd : RequiredModule
        rd.mod = mod
        rd.mod_name = "{mod.name}"
        rd.req_name = "{req_name}"
        rd.is_public = pub
        rd.line = int(at.line)
        result |> push(rd)
    }
    return <- result
}

def infer_require_path(ctx : RequirefixContext?; m : Module const?) : string {
    // TODO: maybe we can use getPrerequisits to avoid this kludge impl?
    let filename = ctx.abs_filename_fn("{m.fileName}")
    for (prefix, replacement in keys(ctx.require_roots), values(ctx.require_roots)) {
        if (prefix == "") {
            continue
        }
        if (filename |> starts_with(prefix)) {
            var pkg = trim_prefix(filename, prefix)
            pkg = trim_suffix(pkg, ".das")
            pkg = replace(pkg, "/", ".")
            pkg = trim_prefix(pkg, ".")
            if (replacement != "") {
                pkg = replacement + "." + pkg
            }
            return pkg
        }
    }

    return "{m.name}"
}

def dedup_providers(ctx : RequirefixContext?; var usage_tab : table<string, ModuleUsageResult>; var needed : table<string, Module const?>) {
    var candidates : array<string>
    var extra_usages : table<string, array<string>>
    for (mod_name, mod_usage in keys(usage_tab), values(usage_tab)) {
        if (length(mod_usage.refs) > 0) {
            for (dep in keys(mod_usage.provides)) {
                if (!key_exists(needed, dep)) {
                    continue
                }
                extra_usages[mod_name] |> push(infer_require_path(ctx, mod_usage.mod))
                needed |> erase(dep)
            }
        } else {
            candidates |> push(mod_name)
        }
    }

    // Pick the candidate that covers the most
    // needed deps. Break ties by fewest total provides (less redundant imports).
    var kept : table<string>
    while (length(needed) > 0) {
        var best_name = ""
        var best_covered = 0
        var best_depcount = 0
        for (c in candidates) {
            if (key_exists(kept, c)) {
                continue
            }
            // A candidate score is higher if it covers more deps
            // we need, but is lower if it has more dependencies of its own.
            var covered = 0
            let depcount = length(usage_tab[c].provides)
            for (dep in keys(usage_tab[c].provides)) {
                if (key_exists(needed, dep)) {
                    covered++
                }
            }
            if (covered > best_covered || (covered == best_covered && covered > 0 && depcount < best_depcount)) {
                best_covered = covered
                best_depcount = depcount
                best_name = c
            }
        }
        if (best_covered == 0) {
            break
        }
        kept |> insert(best_name)
        for (dep in keys(usage_tab[best_name].provides)) {
            if (!key_exists(needed, dep)) {
                continue
            }
            let mod = needed[dep]
            extra_usages[best_name] |> push(infer_require_path(ctx, mod))
            needed |> erase(dep)
        }
    }

    for (mod_name, list in keys(extra_usages), values(extra_usages)) {
        for (dep_name in list) {
            let ref = ref_pub_require(dep_name, uint(usage_tab[mod_name].required_at))
            usage_tab[mod_name].refs |> push(ref)
        }
    }
}

def func_is_provided_by(mod : Module?; name : string) : bool {
    var found = false
    for_each_function(mod, name) $ [unused_argument(fun)] (fun) {
        found = true
    }
    if (found) {
        return true
    }
    for_each_generic(mod, name) $ [unused_argument(fun)] (fun) {
        found = true
    }
    return found
}

def analyze_file(var ctx : RequirefixContext?; var mg : ModuleGroup; var cop : CodeOfPolicies) : array<ModuleUsageResult> {
    var result : array<ModuleUsageResult>
    var inscope access <- make_file_access(ctx.project_file)
    if (access == null) {
        ctx.last_error = "make_file_access failed"
        return <- result
    }

    var usage_tab : table<string, ModuleUsageResult>
    compile_file(ctx.filename, access, unsafe(addr(mg)), cop) $(ok, prog, issues) {
        if (!ok || prog == null) {
            ctx.last_error = "compile_file: {ctx.filename}: {issues}"
            return
        }

        let required = collect_required_modules(prog)
        for (req in required) {
            usage_tab[req.mod_name] = ModuleUsageResult(
                mod = req.mod,
                name = req.mod_name,
                req_name = req.req_name,
                required_at = req.line,
                is_public = req.is_public,
            )
        }

        // If the script requires A, and A requires B publically,
        // then we can't treat A as unused if script needs definitions from B.
        // To properly map the symbols from B reachable from A to A,
        // we would need an extra mapping of symbol providers.
        for (req in required) {
            var exported : table<string>
            collect_public_deps(req.mod, req.mod_name, exported)
            for (name in keys(exported)) {
                ctx.providers[name] |> push(req.mod_name)
            }
            usage_tab[req.mod_name].provides <- exported
        }

        let this_mod = prog |> get_this_module()
        ctx.mod_name = "{this_mod.name}"

        var visitor = new ModuleRefsVisitor()
        visitor.ctx = ctx

        make_visitor(*visitor) $(adapter) {
            visit(prog, adapter)

            // The visitor above won't handle generics, so we need this one.
            visitor.generics = true
            for_each_generic(this_mod) $(fun) {
                visit(fun, adapter)
            }
        }

        // Mark modules that have [init] functions as needed for side effects.
        // These modules may have no visible references in the current file, but
        // their [init] functions run when the module is loaded.
        for (req in required) {
            if (req.mod == null) {
                continue
            }
            var init_func = ""
            for_each_function(req.mod, "") $(fun) {
                if (init_func != "") {
                    return
                }
                for (a in fun.annotations) {
                    if (a.annotation != null && a.annotation.name == "init") {
                        init_func = "{fun.name}"
                        break
                    }
                }
            }
            if (init_func != "") {
                let ref = ModuleRef(kind = ModuleRefKind.side_effect_init, key = init_func, line = req.line)
                visitor.mod_usages[req.mod_name] |> push(ref)
            }
        }

        // For every function that can't be traced back to its module,
        // do a conservative name-based lookup.
        // For a symbol named "foo" that was used, mark every module
        // that happens to export "foo" as used (all of them!)
        for (func_name, call_lines in keys(visitor.unresolved_call_names), values(visitor.unresolved_call_names)) {
            for (req in required) {
                if (req.mod == null) {
                    continue
                }
                if (!func_is_provided_by(req.mod, func_name)) {
                    continue
                }
                for (call_line in call_lines) {
                    let ref = ref_function_call(func_name, call_line)
                    visitor.mod_usages[req.mod_name] |> push(ref)
                }
            }
        }

        var needed_transitive : table<string, Module const?>
        for (mod_name, mod_refs in keys(visitor.mod_usages), values(visitor.mod_usages)) {
            if (key_exists(usage_tab, mod_name)) {
                for (r in mod_refs) {
                    usage_tab[mod_name].refs |> push(r)
                }
            } else {
                needed_transitive |> insert(mod_name, visitor.mod_mapping.get_value(mod_name))
            }
        }

        dedup_providers(ctx, usage_tab, needed_transitive)
    }
    if (ctx.last_error != "") {
        return <- result
    }

    verify_unused_by_recompilation(ctx, usage_tab, cop)

    result |> reserve(length(usage_tab))
    for (mod_usage in values(usage_tab)) {
        mod_usage.tag = "unused"
        for (ref in mod_usage.refs) {
            if (ref.kind == ModuleRefKind.unknown) {
                mod_usage.tag = "used:unknown"
                break
            }
            if (mod_usage.tag == "unused") {
                if (ref.kind == ModuleRefKind.side_effect_init) {
                    mod_usage.tag = "used:side_effects"
                    break
                }
                mod_usage.tag = "used:referenced"
            }
        }
        if (mod_usage.tag == "unused" && mod_usage.is_public) {
            mod_usage.tag = "public"
        }

        result |> emplace(mod_usage)
    }

    return <- result
}

// Build source text from lines, commenting out the specified line (lines are 1-based).
def comment_out_src_line(source : string; skip_line : int) : string {
    return build_string() <| $(str) {
        var line_num = 1
        var i = 0
        let src_len = length(source)
        while (i < src_len) {
            var eol = i
            while (eol < src_len && character_at(source, eol) != '\n') {
                eol++
            }
            if (eol < src_len) {
                eol++ // Include the newline
            }
            if (line_num == skip_line) {
                str |> write("// REMOVED: ")
            }
            str |> write(slice(source, i, eol))
            line_num++
            i = eol
        }
    }
}

// For each module in usage_tab with 0 refs, try recompiling the file
// without that require line. If compilation fails, the require is
// actually needed.
def verify_unused_by_recompilation(var ctx : RequirefixContext?; var usage_tab : table<string, ModuleUsageResult>; var cop : CodeOfPolicies) {
    let source = ctx.read_file_fn(ctx.filename)
    if (length(source) == 0) {
        ctx.last_error = "read_file_fn: can't read {ctx.filename}"
        return
    }

    var candidates : array<string>
    for (mod_name, mod_usage in keys(usage_tab), values(usage_tab)) {
        if (length(mod_usage.refs) == 0) {
            candidates |> push(mod_name)
        }
    }
    for (mod_name in candidates) {
        let skip_line = usage_tab[mod_name].required_at
        if (skip_line <= 0) {
            continue
        }
        let modified_source = comment_out_src_line(source, skip_line)
        {
            var inscope access <- make_file_access(ctx.project_file)
            if (access == null) {
                continue
            }
            set_file_source(access, ctx.filename, modified_source)
            var compiles_ok = true
            using() $(var mg : ModuleGroup) {
                compile_file(ctx.filename, access, unsafe(addr(mg)), cop) $ [unused_argument(issues)] (ok, prog, issues) {
                    if (!ok || prog == null) {
                        compiles_ok = false
                    }
                }
            }
            if (!compiles_ok) {
                let ref = ModuleRef(kind = ModuleRefKind.unknown, key = "compilation_check", line = skip_line)
                usage_tab[mod_name].refs |> push(ref)
            }
        }
    }
}

class ModuleRefsVisitor : AstVisitor {
    mod_usages : table<string, array<ModuleRef>>
    mod_mapping : table<string, Module const?>

    generics : bool = false

    ctx : RequirefixContext?

    // Unresolved call names from uninstantiated generic bodies.
    // Maps function name to the line numbers where it was called.
    unresolved_call_names : table<string, array<uint>>

    def add_module_ref(mod : Module const?; ref : ModuleRef) {
        if (mod == null) {
            return
        }
        if (is_builtin_like(mod.name)) {
            return
        }
        if (mod.name == ctx.mod_name || mod.name == "") {
            return
        }
        let mod_name = "{mod.name}"
        mod_mapping |> insert(mod_name, mod)
        mod_usages[mod_name] |> push(ref)
    }

    // Generic function instances have _module pointing to the
    // instantiating module, not the defining one. Follow fromGeneric
    // to find the original module.
    def func_module(fun : Function?) : Module const? {
        if (fun == null) {
            return null
        }
        if (fun.fromGeneric != null) {
            return fun.fromGeneric._module
        }
        return fun._module
    }

    def add_func_ref(fun : Function?; ref : ModuleRef) {
        add_module_ref(func_module(fun), ref)
        if (fun != null && fun.fromGeneric != null && fun._module != fun.fromGeneric._module) {
            add_module_ref(fun._module, ref)
        }
    }

    def visit_operator(fun : Function?; line : uint; name : string = "") {
        if (fun == null) {
            return
        }
        var operator_name = name
        if (name == "") {
            operator_name = "{fun.name}"
        }
        let ref = ref_overloaded_operator(operator_name, line)
        add_func_ref(fun, ref)
    }

    def visit_annotation(a : AnnotationDeclaration?) {
        if (a.annotation == null) {
            return
        }
        let ref = ref_annotation("{a.annotation.name}", a.at.line)
        add_module_ref(a.annotation._module, ref)
    }

    def override preVisitExprCall(expr : ExprCall?) {
        if (expr.func != null) {
            let ref = ref_function_call("{expr.func.name}", expr.at.line)
            add_func_ref(expr.func, ref)
            return
        }
        if (generics && expr.name != "") {
            // Record the name for later name-based resolution.
            unresolved_call_names["{expr.name}"] |> push(expr.at.line)
        }
    }

    def override preVisitExprAddr(expr : ExprAddr?) {
        if (expr.func == null) {
            return
        }
        let ref = ref_function_call("{expr.func.name}", expr.at.line)
        add_func_ref(expr.func, ref)
    }

    def override preVisitExprVar(expr : ExprVar?) {
        if (expr.variable == null) {
            return
        }
        let ref = ref_variable("{expr.variable.name}", expr.at.line)
        add_module_ref(expr.variable._module, ref)
    }

    def override preVisitExprOp1(expr : ExprOp1?) {
        visit_operator(expr.func, expr.at.line)
    }

    def override preVisitExprOp2(expr : ExprOp2?) {
        visit_operator(expr.func, expr.at.line)
    }

    def override preVisitExprOp3(expr : ExprOp3?) {
        visit_operator(expr.func, expr.at.line)
    }

    def override preVisitExprCopy(expr : ExprCopy?) {
        visit_operator(expr.func, expr.at.line, "copy")
    }

    def override preVisitExprMove(expr : ExprMove?) {
        visit_operator(expr.func, expr.at.line, "move")
    }

    def override preVisitExprClone(expr : ExprClone?) {
        visit_operator(expr.func, expr.at.line, "clone")
    }

    def override preVisitExprNew(expr : ExprNew?) {
        if (expr.func == null) {
            return
        }
        let ref = ref_function_call("new {expr.func.name}", expr.at.line)
        add_func_ref(expr.func, ref)
    }

    def override preVisitExprMakeStruct(expr : ExprMakeStruct?) {
        if (expr.constructor == null) {
            return
        }
        let ref = ref_function_call("make {expr.constructor.name}", expr.at.line)
        add_func_ref(expr.constructor, ref)
    }

    def override preVisitExprConstEnumeration(expr : ExprConstEnumeration?) {
        if (expr.enumType == null) {
            return
        }
        let ref = ModuleRef(kind = ModuleRefKind.type_enum, key = "{expr.enumType.name}", line = int(expr.at.line))
        add_module_ref(expr.enumType._module, ref)
    }

    def override preVisitExprConstBitfield(expr : ExprConstBitfield?) {
        if (expr.bitfieldType == null || expr.bitfieldType.annotation == null) {
            return
        }
        let ref = ModuleRef(kind = ModuleRefKind.type_handle, key = "{expr.bitfieldType.annotation.name}", line = int(expr.at.line))
        add_module_ref(expr.bitfieldType.annotation._module, ref)
    }

    def override preVisitTypeDecl(typ : TypeDeclPtr) {
        var mod : Module const?
        var kind : ModuleRefKind
        var name : string
        if (typ.baseType == Type.tStructure && typ.structType != null) {
            mod = typ.structType._module
            name = "{typ.structType.name}"
            kind = ModuleRefKind.type_struct
        }
        if (typ.baseType == Type.tEnumeration && typ.enumType != null) {
            mod = typ.enumType._module
            name = "{typ.enumType.name}"
            kind = ModuleRefKind.type_enum
        }
        if (typ.baseType == Type.tHandle && typ.annotation != null) {
            mod = typ.annotation._module
            name = "{typ.annotation.name}"
            kind = ModuleRefKind.type_handle
        }

        if (mod != null) {
            add_module_ref(mod, ModuleRef(kind = kind, key = name, line = int(typ.at.line)))
        }
    }

    def override preVisitEnumeration(decl : EnumerationPtr) {
        for (a in decl.annotations) {
            visit_annotation(a)
        }
    }

    def override preVisitFunction(decl : FunctionPtr) {
        for (a in decl.annotations) {
            visit_annotation(a)
        }
    }

    def override preVisitStructure(decl : StructurePtr) {
        if (decl.parent != null) {
            let ref = ModuleRef(kind = ModuleRefKind.type_parent, key = "{decl.parent.name}", line = int(decl.at.line))
            add_module_ref(decl.parent._module, ref)
        }

        for (a in decl.annotations) {
            visit_annotation(a)
        }
    }
}
