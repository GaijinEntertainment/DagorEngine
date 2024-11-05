options no_aot

require DagorConsole
require DagorSystem
require quirrel
require strings

[console_cmd(name=quirrel_hello_world)]
def quirrel_hello_world()
  let script = "println(\"Hello,squirrel\")"
  var v = sq_open(1024l)
  sqstd_seterrorhandlers(v)
  sqdas_register(v)
  if sq_compile(v, script, int64(length(script)), "das_demo_script.nut", 1ul, null) != 0l
    logerr("compile error")
  sq_pushroottable(v)
  if sq_call(v, 1l, SQFalse, SQTrue) != 0l
    logerr("call error")
  sq_close(v)
