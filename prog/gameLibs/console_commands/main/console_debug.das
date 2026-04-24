options no_aot
options no_global_variables = false
options persistent_heap
options gc

require DagorConsole
require %daslib/static_let


def push_non_empty(var arr : array<string>; val : string)
  if val != ""
    arr |> push(val)

[console_cmd(name="console.toggle_command", hint="Call command with the next argument on each call", is_hook=true)]
def toggle_console_command(cmd : string;
                           arg1 : string = "";
                           arg2 : string = "";
                           arg3 : string = "";
                           arg4 : string = "";
                           arg5 : string = "";
                           arg6 : string = "";
                           arg7 : string = "";
                           arg8 : string = "";
                           arg9 : string = "";
                           arg10 : string = "";
                           arg11 : string = "")

  if (arg1 == "")
    console_print("Usage: 'command' 'arg1' 'arg2' ... (up to 11 different args)")
    return

  var args : array<string>
  args |> push_non_empty(arg1)
  args |> push_non_empty(arg2)
  args |> push_non_empty(arg3)
  args |> push_non_empty(arg4)
  args |> push_non_empty(arg5)
  args |> push_non_empty(arg6)
  args |> push_non_empty(arg7)
  args |> push_non_empty(arg8)
  args |> push_non_empty(arg9)
  args |> push_non_empty(arg10)
  args |> push_non_empty(arg11)

  static_let <|
    var currentArgIndexByCommandLine : table</*command*/string; /*current arg index*/int>

  var argIndex = 0

  let found = currentArgIndexByCommandLine |> get(cmd) <| $(var currentIdx)
    currentIdx = (currentIdx + 1) % length(args)
    argIndex = currentIdx

  if !found
    currentArgIndexByCommandLine |> insert(cmd, 0)

  console_command("{cmd} {args[argIndex]}")
