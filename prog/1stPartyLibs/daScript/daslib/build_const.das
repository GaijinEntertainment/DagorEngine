options gen2
options indenting = 4
options strict_smart_pointers = true

module build_const shared private

//! Compile-time constants from the build environment — the daslang process's command
//! line and environment variables, read while the requiring module COMPILES and spliced
//! into the code as literals. Feed a global ``let`` and gate with ``static_if``; the
//! disabled branch erases before infer, so the off shape costs nothing at runtime:
//!
//! ``let PROFILING = build_flag("--profiling", "PROFILING")``
//!
//! Interpreted and ``-jit`` runs compile per launch, so the constant follows each run's
//! command line and environment. An ``-exe`` build bakes the constant at BUILD time — a
//! binary built with a feature enabled ships enabled. Everything is off/default when
//! neither carrier is present.
//!
//! ``build_flag(flag, env)`` / ``build_value(flag, env, dflt)`` / ``build_int(flag,
//! env, dflt)`` are the recommended forms — both carriers in one call, command line
//! beats environment beats default. Single-carrier forms: ``build_has_arg(flag)``
//! (presence-only), ``build_arg_value`` / ``build_arg_int`` (accept ``--name value``
//! and ``--name=value``, last occurrence wins), and ``build_env_flag`` /
//! ``build_env_value`` / ``build_env_int``. Env boolean parsing: unset, "", "0",
//! "false", "off", "no" (any case) are false; any other value is true. A set-but-EMPTY
//! environment variable counts as unset for every accessor (platforms disagree on
//! whether "" is removal or a value — daslang treats both as absent). A garbage
//! integer — argv or env — is a COMPILE ERROR, not a silent default: a typo in an
//! explicit knob should stop the build.
//!
//! Initializers compose freely — ``let P = build_has_arg("--x") || build_env_flag("X")``
//! resolves in every compile profile (``static_if`` computes a const global's init on
//! the side even where lint/IDE profiles disable in-place folding) — but the combined
//! accessors say the same thing in one call. Arguments must be literals.

require fio
require strings
require daslib/strings_boost
require daslib/strings_convert
require daslib/ast
require daslib/ast_boost
require daslib/templates_boost
require daslib/macro_boost

def private env_text_is_true(v : string) : bool {
    let s = to_lower(v)
    return !(empty(s) || s == "0" || s == "false" || s == "off" || s == "no")
}

// last occurrence wins, matching general command-line convention
def private find_arg_text(flag : string) : tuple<found : bool; value : string> {
    var res = (found = false, value = "")
    let args <- get_command_line_arguments()
    let eq_prefix = flag + "="
    for (i in range(length(args))) {
        if (args[i] == flag && i + 1 < length(args)) {
            res = (found = true, value = args[i + 1])
        } elif (args[i] |> starts_with(eq_prefix)) {
            res = (found = true, value = args[i] |> trim_prefix(eq_prefix))
        }
    }
    return res
}

[call_macro(name="build_flag")]
class BuildFlagMacro : AstCallMacro {
    //! ``build_flag(flag, env)`` — both-carrier boolean: true when ``flag`` is on the command line, else the env rule on ``env``.
    def override visit(prog : ProgramPtr; mod : Module?; var expr : ExprCallMacro?) : ExpressionPtr {
        macro_verify(expr.arguments |> length == 2, prog, expr.at, "build_flag expects (flag, env : string literals)")
        macro_verify(expr.arguments[0] is ExprConstString && expr.arguments[1] is ExprConstString, prog, expr.at,
            "build_flag: flag and env must be string literals, got {describe(expr.arguments[0])}, {describe(expr.arguments[1])}")
        let flag = string((expr.arguments[0] as ExprConstString).value)
        let name = string((expr.arguments[1] as ExprConstString).value)
        var res = false
        var on_argv = false
        let eq_prefix = flag + "="
        let args <- get_command_line_arguments()
        for (a in args) {
            if (a == flag) {
                on_argv = true
                res = true
            } elif (starts_with(a, eq_prefix)) {
                // --flag=value spelling, same truth rule as the env carrier
                on_argv = true
                res = env_text_is_true(a |> trim_prefix(eq_prefix))
            }
        }
        if (!on_argv && has_env_variable(name)) {
            res = env_text_is_true(get_env_variable(name))
        }
        return <- qmacro($v(res))
    }
}

[call_macro(name="build_value")]
class BuildValueMacro : AstCallMacro {
    //! ``build_value(flag, env, dflt)`` — both-carrier string: command line beats environment beats ``dflt``.
    def override visit(prog : ProgramPtr; mod : Module?; var expr : ExprCallMacro?) : ExpressionPtr {
        macro_verify(expr.arguments |> length == 3, prog, expr.at, "build_value expects (flag, env, dflt : string literals)")
        macro_verify(expr.arguments[0] is ExprConstString && expr.arguments[1] is ExprConstString && expr.arguments[2] is ExprConstString, prog, expr.at,
            "build_value: flag, env and dflt must be string literals, got {describe(expr.arguments[0])}, {describe(expr.arguments[1])}, {describe(expr.arguments[2])}")
        let flag = string((expr.arguments[0] as ExprConstString).value)
        let name = string((expr.arguments[1] as ExprConstString).value)
        var res = string((expr.arguments[2] as ExprConstString).value)
        let hit = find_arg_text(flag)
        if (hit.found) {
            res = hit.value
        } elif (has_env_variable(name)) {
            let v = get_env_variable(name)
            if (!empty(v)) {
                res = v
            }
        }
        return <- qmacro($v(res))
    }
}

[call_macro(name="build_int")]
class BuildIntMacro : AstCallMacro {
    //! ``build_int(flag, env, dflt)`` — both-carrier integer; a garbage value on either carrier is a compile error.
    def override visit(prog : ProgramPtr; mod : Module?; var expr : ExprCallMacro?) : ExpressionPtr {
        macro_verify(expr.arguments |> length == 3, prog, expr.at, "build_int expects (flag, env : string literals; dflt : int literal)")
        macro_verify(expr.arguments[0] is ExprConstString && expr.arguments[1] is ExprConstString && expr.arguments[2] is ExprConstInt, prog, expr.at,
            "build_int: flag and env must be string literals and dflt an int literal, got {describe(expr.arguments[0])}, {describe(expr.arguments[1])}, {describe(expr.arguments[2])}")
        let flag = string((expr.arguments[0] as ExprConstString).value)
        let name = string((expr.arguments[1] as ExprConstString).value)
        var res = (expr.arguments[2] as ExprConstInt).value
        let hit = find_arg_text(flag)
        let env_text = has_env_variable(name) ? get_env_variable(name) : ""
        if (hit.found) {
            let parsed = try_to_int(hit.value)
            macro_verify(parsed |> is_ok, prog, expr.at, "build_int: {flag} value \"{hit.value}\" is not an integer")
            res = parsed ?? 0
        } elif (!empty(env_text)) {
            let parsed = try_to_int(env_text)
            macro_verify(parsed |> is_ok, prog, expr.at, "build_int: {name} value \"{env_text}\" is not an integer")
            res = parsed ?? 0
        }
        return <- qmacro($v(res))
    }
}

[call_macro(name="build_has_arg")]
class BuildHasArgMacro : AstCallMacro {
    //! ``build_has_arg(flag)`` — true when ``flag`` appears verbatim on the command line.
    def override visit(prog : ProgramPtr; mod : Module?; var expr : ExprCallMacro?) : ExpressionPtr {
        macro_verify(expr.arguments |> length == 1, prog, expr.at, "build_has_arg expects (flag : string literal)")
        macro_verify(expr.arguments[0] is ExprConstString, prog, expr.at, "build_has_arg: flag must be a string literal, got {describe(expr.arguments[0])}")
        let flag = string((expr.arguments[0] as ExprConstString).value)
        var found = false
        let eq_prefix = flag + "="
        let args <- get_command_line_arguments()
        for (a in args) {
            if (a == flag || starts_with(a, eq_prefix)) {
                found = true
            }
        }
        return <- qmacro($v(found))
    }
}

[call_macro(name="build_arg_value")]
class BuildArgValueMacro : AstCallMacro {
    //! ``build_arg_value(flag, dflt)`` — the ``--name value`` / ``--name=value`` argument, or ``dflt``.
    def override visit(prog : ProgramPtr; mod : Module?; var expr : ExprCallMacro?) : ExpressionPtr {
        macro_verify(expr.arguments |> length == 2, prog, expr.at, "build_arg_value expects (flag, dflt : string literals)")
        macro_verify(expr.arguments[0] is ExprConstString && expr.arguments[1] is ExprConstString, prog, expr.at,
            "build_arg_value: flag and dflt must be string literals, got {describe(expr.arguments[0])}, {describe(expr.arguments[1])}")
        let flag = string((expr.arguments[0] as ExprConstString).value)
        let hit = find_arg_text(flag)
        let res = hit.found ? hit.value : string((expr.arguments[1] as ExprConstString).value)
        return <- qmacro($v(res))
    }
}

[call_macro(name="build_arg_int")]
class BuildArgIntMacro : AstCallMacro {
    //! ``build_arg_int(flag, dflt)`` — integer form of ``build_arg_value``; garbage is a compile error.
    def override visit(prog : ProgramPtr; mod : Module?; var expr : ExprCallMacro?) : ExpressionPtr {
        macro_verify(expr.arguments |> length == 2, prog, expr.at, "build_arg_int expects (flag : string literal; dflt : int literal)")
        macro_verify(expr.arguments[0] is ExprConstString && expr.arguments[1] is ExprConstInt, prog, expr.at,
            "build_arg_int: flag must be a string literal and dflt an int literal, got {describe(expr.arguments[0])}, {describe(expr.arguments[1])}")
        let flag = string((expr.arguments[0] as ExprConstString).value)
        let hit = find_arg_text(flag)
        var res = (expr.arguments[1] as ExprConstInt).value
        if (hit.found) {
            let parsed = try_to_int(hit.value)
            macro_verify(parsed |> is_ok, prog, expr.at, "build_arg_int: {flag} value \"{hit.value}\" is not an integer")
            res = parsed ?? 0
        }
        return <- qmacro($v(res))
    }
}

[call_macro(name="build_env_flag")]
class BuildEnvFlagMacro : AstCallMacro {
    //! ``build_env_flag(name)`` — boolean env rule: unset/""/0/false/off/no are false, anything else true.
    def override visit(prog : ProgramPtr; mod : Module?; var expr : ExprCallMacro?) : ExpressionPtr {
        macro_verify(expr.arguments |> length == 1, prog, expr.at, "build_env_flag expects (name : string literal)")
        macro_verify(expr.arguments[0] is ExprConstString, prog, expr.at, "build_env_flag: name must be a string literal, got {describe(expr.arguments[0])}")
        let name = string((expr.arguments[0] as ExprConstString).value)
        let res = has_env_variable(name) ? env_text_is_true(get_env_variable(name)) : false
        return <- qmacro($v(res))
    }
}

[call_macro(name="build_env_value")]
class BuildEnvValueMacro : AstCallMacro {
    //! ``build_env_value(name, dflt)`` — the environment value, or ``dflt`` when unset or empty.
    def override visit(prog : ProgramPtr; mod : Module?; var expr : ExprCallMacro?) : ExpressionPtr {
        macro_verify(expr.arguments |> length == 2, prog, expr.at, "build_env_value expects (name, dflt : string literals)")
        macro_verify(expr.arguments[0] is ExprConstString && expr.arguments[1] is ExprConstString, prog, expr.at,
            "build_env_value: name and dflt must be string literals, got {describe(expr.arguments[0])}, {describe(expr.arguments[1])}")
        let name = string((expr.arguments[0] as ExprConstString).value)
        var res = string((expr.arguments[1] as ExprConstString).value)
        if (has_env_variable(name)) {
            let v = get_env_variable(name)
            if (!empty(v)) {
                res = v
            }
        }
        return <- qmacro($v(res))
    }
}

[call_macro(name="build_env_int")]
class BuildEnvIntMacro : AstCallMacro {
    //! ``build_env_int(name, dflt)`` — integer env value; set-but-empty is ``dflt``, garbage is a compile error.
    def override visit(prog : ProgramPtr; mod : Module?; var expr : ExprCallMacro?) : ExpressionPtr {
        macro_verify(expr.arguments |> length == 2, prog, expr.at, "build_env_int expects (name : string literal; dflt : int literal)")
        macro_verify(expr.arguments[0] is ExprConstString && expr.arguments[1] is ExprConstInt, prog, expr.at,
            "build_env_int: name must be a string literal and dflt an int literal, got {describe(expr.arguments[0])}, {describe(expr.arguments[1])}")
        let name = string((expr.arguments[0] as ExprConstString).value)
        var res = (expr.arguments[1] as ExprConstInt).value
        let env_text = has_env_variable(name) ? get_env_variable(name) : ""
        if (!empty(env_text)) {
            let parsed = try_to_int(env_text)
            macro_verify(parsed |> is_ok, prog, expr.at, "build_env_int: {name} value \"{env_text}\" is not an integer")
            res = parsed ?? 0
        }
        return <- qmacro($v(res))
    }
}
