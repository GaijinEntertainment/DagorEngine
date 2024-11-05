options stack = 102400
require ecs
require remote.jsonrpc_boost


[rpc_call(name="das.compile_errors_count")]
def get_das_compile_errors_count(id : uint64)
  send_response(JV(das_compile_errors_count()), id)
