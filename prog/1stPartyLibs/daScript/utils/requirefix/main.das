options gen2

require requirefix
require fio

// This is just a simple CLI demo.
// The real use case will probably involve integrating the requirefix
// module into a das plugin or some other runner.

[export]
def main() {
    let cli_args <- get_command_line_arguments()
    var target_files : array<string>
    var project_file : string
    var arg_index = find_index(cli_args, "--") + 1
    while (arg_index < length(cli_args)) {
        let arg = cli_args[arg_index]
        arg_index++
        if (arg == "--project") {
            project_file = cli_args[arg_index]
            arg_index++
            continue
        }
        target_files |> push(arg)
    }
    if (length(target_files) == 0) {
        return
    }

    for (target in target_files) {
        var config = RequirefixConfig(
            target = target,
            require_roots = {"./requirefix" => ""},
            project_file = project_file,
            abs_filename_fn <- @(filename : string) : string {
                return get_full_file_name(filename)
            },
            read_file_fn <- @(filename : string) : string {
                return read_file(filename)
            },
        )
        var result = analyze_module_usages(config)
        if (result.err != "") {
            print("{target}: error: {result.err}\n")
        }
        for (mod in result.modules) {
            if (mod.is_public) {
                print("{target}:{mod.required_at}: public\n")
                continue
            }
            if (length(mod.refs) > 0) {
                var proxy_requires : array<string>
                for (ref in mod.refs) {
                    if (ref.kind != ModuleRefKind.pub_require) {
                        break
                    }
                    proxy_requires |> push(ref.key)
                }
                if (length(proxy_requires) == length(mod.refs)) {
                    print("{target}:{mod.required_at}: proxied {proxy_requires}\n")
                    continue
                }
                print("{target}:{mod.required_at}: {mod.refs}\n")
                continue
            }
            print("{target}:{mod.required_at}: required module {mod.req_name} is unused\n")
        }
    }
}

def read_file(path : string) : string {
    let f = fopen(path, "r")
    if (f == null) {
        return ""
    }
    let s = fread(f)
    fclose(f)
    return s
}
