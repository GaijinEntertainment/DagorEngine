options gen2
module review_nttp private

require daslib/ast
require daslib/ast_boost
require daslib/macro_boost
require math // nolint:STYLE030 — coverage require: pulls the module into the scanned registry
require strings
require jit // nolint:STYLE030 — coverage require
require dasbind // coverage require: not a policy module, so the scan asserts its binds stay non-NTTP
require daslib/fio // nolint:STYLE030 — coverage require
require daslib/rtti // coverage require
require daslib/jobque_boost // coverage require
require daslib/network // nolint:STYLE030 — coverage require
require daslib/uriparser_boost // nolint:STYLE030 — coverage require
require daslib/debug // coverage require

// Bind flavor exists only in the compiled module registry, so this check runs at
// compile time; the policy and its exemptions are src/builtin/REVIEW.md's rule.

def private is_plain_value_bind(func : FunctionPtr) : bool {
    if (!func.flags.builtIn || !func.flags.callBased || func.flags.interopFn) {
        return false
    }
    return !(func.result == null || func.result.isRefType || func.result.flags.ref)
}

def private is_shared_generic_helper(cppName : string) : bool {
    return (
        (cppName |> starts_with("das_vector_"))
        || (cppName |> starts_with("das_equ"))
        || (cppName |> starts_with("das_nequ"))
        || (cppName |> starts_with("das_handle_equ"))
        || (cppName |> starts_with("das_handle_nequ")))
}

[_macro]
def check_nttp_policy {
    // the list is the single source of truth: membership, the finding text, and the
    // coverage check below all read it, so adding a module cannot leave one of them stale
    let inlineOnlyModules <- ["$", "math", "strings", "jit"]
    let policyNames = build_string() $(writer) {
        for (name, idx in inlineOnlyModules, count()) {
            writer |> write(idx > 0 ? ", " : "")
            writer |> write(name)
        }
    }
    var matched : table<string>
    program_for_each_module(compiling_program()) $(mod) {
        let inInlineOnly = find_index(inlineOnlyModules, "{mod.name}") >= 0
        if (inInlineOnly) {
            matched |> insert("{mod.name}")
        }
        for_each_function(mod, "") $(func) {
            return if (!is_plain_value_bind(func) || is_shared_generic_helper("{(func as BuiltInFunction).cppName}"))
            if (inInlineOnly && !func.moreFlags2.nttp) {
                macro_error(compiling_program(), func.at, "REVIEW policy: {mod.name}::{func.name} is not bound with addExternInline; every plain-value bind in {policyNames} is NTTP")
            } elif (!inInlineOnly && func.moreFlags2.nttp) {
                macro_error(compiling_program(), func.at, "REVIEW policy: {mod.name}::{func.name} is bound with addExternInline; only {policyNames} binds are NTTP")
            }
        }
    }
    // fail closed: a policy module name that matched nothing means the scan
    // silently skipped it (typo or module rename) — the gate must not stay green
    for (name in inlineOnlyModules) {
        if (!(matched |> key_exists(name))) {
            panic("review_nttp: policy module '{name}' not found in the scanned registry — gate coverage is broken")
        }
    }
}
