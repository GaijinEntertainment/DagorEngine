require strings
require daslib/strings_boost

struct Flags {
    tags : array<string>
    argmap : table<string, string>
    values : array<string>
}

def get_string_aliased_arg(flags : Flags; key : string; alias : string, default_value : string = "") : string {
    return get_string_arg(flags, alias, get_string_arg(flags, key, default_value))
}

def get_string_arg(flags : Flags; key : string; default_value : string = "") : string {
    return flags.argmap?[key] ?? default_value
}

def parse_flags(args : array<string>) : Flags {
    var tags : array<string>
    var values : array<string>
    var argmap : table<string, string>
    var i = 0
    while (i < length(args)) {
        if (args[i] == "--tag") {
            i++
            if (i < length(args)) {
                let tag = args[i]
                i++
                tags |> push(tag)
            }
            continue
        }
        if (starts_with(args[i], "--")) {
            let key = args[i]
            i++
            if (i < length(args)) {
                let val = args[i]
                i++
                argmap |> insert(slice(key, 2), val)
            }
            continue
        }
        values |> push(args[i])
        i++
    }
    return Flags(
        values <- values,
        argmap <- argmap,
        tags <- tags,
    )
}
