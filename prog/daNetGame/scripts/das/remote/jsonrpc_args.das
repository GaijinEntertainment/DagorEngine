options indenting = 2
module jsonrpc_args shared

require strings

require daslib/json_boost public


def get_arg(params : JsonValue?; idx : int = 0) : JsonValue?
  if params == null
    return null
  if params is _array
    assume arr = params as _array
    return idx < length(arr) ? arr[idx] : null
  return idx == 0 && !(params is _object) ? params : null


def get_arg_name(params : JsonValue?; name : string) : JsonValue?
  if params == null
    return null
  if params is _object
    return (params as _object)?[name] ?? [[JsonValue? ]]
  return null


def get_args_count(params : JsonValue?) : int
  if params == null
    return 1 // one null value
  if params is _array
    return length(params as _array)
  if params is _object
    return length(params as _object)
  return 1


def length(js : JsonValue?)
  if js is _array
    return length(js as _array)
  if js is _object
    return length(js as _object)
  return 0


def is_null(js : JsonValue?) : bool
  return js == null || js is _null

def is_array(js : JsonValue?) : bool
  return js != null && js is _array

def is_object(js : JsonValue?) : bool
  return js != null && js is _object

def is_bool(js : JsonValue?) : bool
  return js != null && js is _bool

def as_bool(js : JsonValue?) : bool
  if js == null
    return false
  if js is _bool
    return js as _bool
  if js is _number
    return (js as _number) != 0lf
  if js is _string
    return length(js as _string) > 0
  return js is _object || js is _array

def is_string(js : JsonValue?) : bool
  return js != null && js is _string

def as_string(js : JsonValue?) : string
  if js == null
    return ""
  if js is _string
    return js as _string
  if js is _bool
    return js as _bool ? "true" : "false"
  if js is _number
    return "{js as _number}"
  return js is _object ? "[Object object]" : js is _array ? "[Array array]" : ""

def is_number(js : JsonValue?) : bool
  return js != null && js is _number

def as_number(js : JsonValue?) : double
  return as_double(js)

def as_double(js : JsonValue?) : double
  if js == null
    return 0lf
  if js is _number
    return js as _number
  if js is _bool
    return js as _bool ? 1lf : 0lf
  if js is _string
    return double(js as _string)
  return 0lf

def as_float(js : JsonValue?) : float
  if js == null
    return 0f
  if js is _number
    return float(js as _number)
  if js is _bool
    return js as _bool ? 1f : 0f
  if js is _string
    return to_float(js as _string)
  return 0f

def as_int(js : JsonValue?) : int
  if js == null
    return 0
  if js is _number
    return int(js as _number)
  if js is _bool
    return js as _bool ? 1 : 0
  if js is _string
    return to_int(js as _string)
  return 0

def as_uint(js : JsonValue?) : uint
  if js == null
    return 0u
  if js is _number
    return uint(js as _number)
  if js is _bool
    return js as _bool ? 1u : 0u
  if js is _string
    return uint(to_float(js as _string))
  return 0u
