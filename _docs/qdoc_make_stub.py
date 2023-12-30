# TODO:
# function check type args
# function - optional args
# ? function return types
#const values
# values
# ? tables
# class (at least ctor, operators and methods, later properties)

def mkFunctionStub(info):
  if isinstance(info, dict) and info.get("type") == "function":
    params = []
    for param in info["params"]:
      if "defvalue" in param:
        params.append(f'{param["name"]} = {cvtValue(param["defvalue"])}')
      else:
        params.append(param["name"])

    params_str = ", ".join(params)

    first = f'function {info["name"]}({params_str})' +" {"
    body = []  #todo make asserts of types if presented
    return (first, body, "}")
  return None

def cvtValue(v):
  if v is None:
    return "null"
  try:
    return str(int(v))
  except:
    return f'"{v}"'

def mkConstStub(info):
  if isinstance(info, dict) and info.get("type") == "const":
    return f'{info["name"]} = {cvtValue(info["fvalue"])}'
  return None

def mkStub(info):
  res = []
  parsed = False
  for parser in [mkFunctionStub, mkConstStub]:
    if r := parser(info):
      res.append(r)
      parsed = True
      break
  if not parsed:
    for i in info.get("members"):
      if not isinstance(i,str):
        res.extend(mkStub(i))
  return res

def astToString(ast, indent=0):
  res = []
  for i in ast:
    if not i:
      continue
    if isinstance(i, str):
      indent_str = ""
      for j in range(0, indent):
        indent_str += "  "
      res.append(f'{indent_str}{i}')
    elif isinstance(i, tuple):
      res.append(astToString(i, indent))
    else:
      res.append(astToString(i, indent+1))
  return "\n".join(res)


def mkModuleStub(info):
  if not info.get("type") == "module":
    return None
  return astToString(["return {", mkStub(info), "}"])
