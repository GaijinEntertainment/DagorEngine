options indenting = 2
require DagorConsole
require DagorMath public

[generic]
def visual_log(msg : string | string#; color : E3DCOLOR = E3DCOLOR(0xffffffff); level : int = 0)
  _builtin_visual_log(msg, color, level)


[generic]
def add_hint(var hints : ConsoleProcessorHints;
             name : string;
             min_args : int;
             max_args : int;
             args_description : string = "")
  add_hint(hints, name, min_args, max_args, args_description, "")