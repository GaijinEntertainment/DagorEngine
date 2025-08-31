options gen2
options indenting = 4
options no_unused_block_arguments = false
options no_unused_function_arguments = false
options strict_smart_pointers = true

module das_source_formatter_fio shared private

require fio
require strings
require daslib/defer
require daslib/das_source_formatter


def public format_file(file_name : string const) {
    var unformatted : string = ""
    var formatted : string = ""
    fopen(file_name, "rb") <| $(fr) {
        if (fr == null) {
            to_log(LOG_ERROR, "cannot open '{file_name}'\n")
        } else {
            fmap(fr) <| $(data) {
                unformatted = string(data)
                formatted = format_source(data)
            }
        }
    }

    if (!empty(formatted) && formatted != unformatted) {
        fopen(file_name, "wb") <| $(fw) {
            if (fw == null) {
                to_log(LOG_ERROR, "cannot open '{file_name} for write'\n")
            } else {
                fw |> fprint(formatted)
            }
        }
    }
}



def public format_files(file_names : array<string>&) {
    for (fname in file_names) {
        format_file(fname)
    }
}


/*

[export]
def main() {
    var fileNames: array<string>
    var filesListFn: string = ""
    var ignore = true
    for (arg in get_command_line_arguments()) {
        if (arg == "--debug") {
            debugMode = true
        }
        if (arg == "-args") {
            ignore = false
            continue
        }
        if (!ignore) {
            if (arg |> character_at(0) != '-') {
                fileNames |> push(arg)
            }
            if (arg |> starts_with("--files:")) {
                filesListFn = arg |> slice(8)
            }
        }
    }

    if (!empty(filesListFn)) {
        fopen(filesListFn, "rb") <| $(f) {
            if (f == null) {
                to_log(LOG_ERROR, "cannot open '{filesListFn}'\n")
            } else {
                while (!feof(f)) {
                    var file_name = fgets(f)
                    if (!empty(file_name)) {
                        file_name = replace(file_name, "\n", "")
                        file_name = replace(file_name, "\r", "")
                        fileNames |> push(file_name)
                    }
                }
            }
        }
    }

    format_files(fileNames)
}
*/