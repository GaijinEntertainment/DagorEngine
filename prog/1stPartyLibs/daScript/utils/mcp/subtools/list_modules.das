options gen2
options rtti
options indenting = 4

require ../tools/list_modules.das public

//! Subprocess form of list_modules, spawned with the caller's -project_root / -load_module flags
//! and -ignore-manifest. Argv (after daslang exe + script path): <json>, the literal "true" or
//! "false". Prints the make_tool_result envelope to stdout.

[export]
def main {
    let raw <- get_command_line_arguments()
    let args <- subtool_user_args(raw)
    let json = !empty(args) && args[0] == "true"
    print(list_modules_here(json))
}
