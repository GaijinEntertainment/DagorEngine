import json
from collections.abc import Iterable
from functools import reduce

"""

https://github.com/ralsina/rst-cheatsheet/blob/master/rst-cheatsheet.rst
https://sphinx-rtd-tutorial.readthedocs.io/en/latest/docstrings.html

"""
def pprint(v):
  print(json.dumps(v, indent=2))

def mkTitle(tag, underOnly=False):
  def build(content):
    decor = "".join([tag for i in range(len(content))])
    if underOnly:
      return content+"\n"+decor
    return decor+"\n"+content+"\n"+decor
  return build

partTitle = mkTitle("#")
chapterTitle = mkTitle("*")
sectionTitle = mkTitle("=")
sectionTitle2 = mkTitle("=", True)
subsectionTitle = mkTitle("-")
subsubsectionTitle = mkTitle("^")
paragraphTitle = mkTitle('"')
titlesMap = {
  0: partTitle,
  1: chapterTitle,
  2: sectionTitle,
  3: sectionTitle2,
  4: subsectionTitle,
  5: subsubsectionTitle,
  6: paragraphTitle
}
separator = "\n\n\n"

def intag(content, tag):
  return f"{tag}{content}{tag}"

def linesToRes(lines):
  if isinstance(lines, str):
    return lines+"\n"
  if isinstance(lines, Iterable):
    return "".join([linesToRes(m) for m in lines if m is not None])
  return lines

def code_block(lines, lang = "sq"):
  if isinstance(lines,str):
    lines = [lines]
  res = [f'.. code-block:: {lang}\n']
  for f in lines:
    res.append(f"    {f}")
  res.append("")
  return res

def preblk(text):
  return [f'  ``{text}``', ""]


typemaskMap = {
  '.': "any_type",
  'o': "null",
  'i': "integer",
  'f': "float",
  'n': "integer_or_float",
  's': "string",
  't': "table",
  'a': "array",
  'u': "userdata",
  'c': "closure_and_nativeclosure",
  'g': "generator",
  'p': "userpointer",
  'v': "thread",
  'x': "class_instance",
  'y': "class",
  'b': "bool",
}

def convertTypeStr(s):
  if s is None:
    return 'any_type'
  res = [typemaskMap.get(i, i) for i in s.split("|")]
  return "|".join(res)

args_names = [f"arg{i}" for i in range(100)]

def convertTypemask(typemask):
  types = []
  appendNext = False
  for s in typemask:
    if s == " ":
      continue
    if s not in typemaskMap and s !="|":
      types = None
      break
    elif s in typemaskMap:
      if appendNext and len(types)>0:
        types[-1].append(typemaskMap.get(s))
        appendNext = False
      else:
        types.append([typemaskMap.get(s)])
    elif s == "|":
      appendNext = True
      continue
  if types:
    return [" or ".join(t) for t in types]
  return None

def convertTypemaskAndParamsToParams(desc):
  paramsnum = desc["paramsnum"]
  typemask = desc["typemask"]
  parent = desc["parent"]
  isMethod = parent is not None and desc["type"]!="ctor"
  vargved = False
  if not isinstance(paramsnum, int):
    return None, None
  if paramsnum < 0:
    paramsnum = -paramsnum
    vargved = True
  types = []
  if typemask == "nullptr" or typemask is None or (len(typemask)==0 and isMethod) or (len(typemask)==1 and not isMethod):
    types = None
  else:
    types_new = convertTypemask(typemask)
    if types_new:
      types = types_new
  if not isMethod:  #'this' removed
    paramsnum -= 1
    if types:
      types = types[1:]
  args = []
  for i in range(max(paramsnum, len(types) if types else 0)):
    paramtype = "."
    paramtype = types[i] if types and len(types)>i else paramtype
    args.append({"name":args_names[i], "paramtype":paramtype, "description":"autodoc from typemask/paramscheck"})
  if vargved:
    args.append({"name":"...", "paramtype":"any_type", "description": "this function accepts unlimited arguments"})
  return args, vargved

def convertTypemaskStr(s):
  if s == "nullptr":
    return "undefined"

  if s is None:
    return 'anytype'
  s = s.split("|")
  res = []
  for r in s:
    res.append(" or ".join([m.get(i,i) for i in r]))
  return " , ".join(res)

def nameAndDef(p):
  if "defvalue" in p:
    return p["name"] +" = " + json.dumps(p["defvalue"])
  else:
    return p["name"]

def get_short_func_args(desc):
  defArgs, vargved = convertTypemaskAndParamsToParams(desc)
  params = desc["params"]
  if len(params)==0 and defArgs:
    pstr = ", ".join([nameAndDef(p) for p in defArgs])
  else:
    pstr = ", ".join([nameAndDef(p) for p in params])
    if  desc.get("vargved"):
      pstr=pstr+", ..."
  return pstr

def get_func_signature(desc):
  name = desc["name"]
  kwarged = desc["kwarged"]
  br = ("({","})") if kwarged else ("(",")")
  pstr = get_short_func_args(desc)
  if desc.get("parent"):
    typ = desc["type"]
    if typ=="ctor":
      name = desc.get("parent") or "constructor"
      return f'{name}({pstr})'
    else:
      name = desc.get("parent")+"."+name
      if typ=="operator":
        return f'{typ} {name}{br[0]}{pstr}{br[1]}'
      else:
        return f'method {name}{br[0]}{pstr}{br[1]}'
  func_name = "pure function" if desc.get("is_pure") else "function"
  return f'{func_name} {name}{br[0]}{pstr}{br[1]}'

def params_rst(params):
  res = []
  for param in params:
    desc = param.get('description') or " "
    defvalue = param.get("defvalue")
    defvalue = f", default = {defvalue}" if defvalue else ''
    optional = f", optional" if param.get('optional') else ''
    res.extend([
      f"  :param {param['name']}: {desc}{defvalue}",
      f"  :type {param['name']}: {convertTypeStr(param['paramtype'])}{optional}",
      ""
    ])
  return res

def mk_function_arguments(desc):
#:param [ParamName]: [ParamDescription], default = [DefaultParamVal]
#:type [ParamName]: [ParamType](, optional)
  documented_params = desc["params"]
  res = []
  autoArgs, vargved = convertTypemaskAndParamsToParams(desc)
  if len(documented_params)==0 and autoArgs:
    res.extend(params_rst(autoArgs))
  elif documented_params and not autoArgs:
    res.extend(params_rst(documented_params))
  elif documented_params and autoArgs:
    res.extend(params_rst(documented_params))
  if desc["paramsnum"] or desc["typemask"]:
    if desc["paramsnum"]:
      res.extend([preblk(f"nparamscheck:{desc['paramsnum']}")])
    if desc["typemask"]:
      typmask = convertTypemask(desc["typemask"])
      if typmask:
        res.extend([preblk(f"typecheck mask: {', '.join(typmask)}")])
  if desc["kwarged"]:
    res.extend([preblk("Function is kwarged - arguments passed in a table")])
  return res

def render_ctor(desc):
  funcDesc = f'{get_func_signature(desc)}'
  res = [f".. sq:method:: {funcDesc}", ""]
  if desc["brief"]:
    res.extend([f"  {desc['brief']}",""])
  res.extend(mk_function_arguments(desc))
  if r := desc.get("members"):
    res.extend(r)
  return res

def render_func(desc):
  funcDesc = f'{get_func_signature(desc)}'
  res = [f".. sq:function:: {funcDesc}", ""]

  if desc["brief"]:
    res.extend(["  "+desc["brief"],""])
  res.extend(mk_function_arguments(desc))

#:return: [ReturnDescription]
#:rtype: [ReturnType]
  if ret := desc.get("return"):
    descr = ret.get("description")
    if descr:
      res.extend([
        f"  :return: {descr}",
        f"  :rtype: {convertTypeStr(ret['rtype'])}",
        ""
      ])
    else:
      res.extend([f"  :return: {convertTypeStr(ret['rtype'])}",""])
  if r:=desc.get("members"):
    res.extend(membersToRes(r))
    res.append("")
  return res

def render_value(desc):
  vDesc = f'{desc["name"]}'
  if desc.get("parent"):
    vDesc = desc.get("parent")+"."+vDesc
  typ = desc.get("type")
  if typ != "value":
    vDesc = desc.get("type") + " " + vDesc
  writeable = desc.get("writeable")
  ftype = desc.get("ftype")
  fvalue = desc.get("fvalue")
  readonly = writeable is not None and not writeable
  if fvalue:
#    if isinstance(fvalue, str) and not simple_literal.match(fvalue):
#      fvalue = f'"{fvalue}"'
    vDesc = vDesc + f" = {fvalue}"
  res = [f".. sq:attribute:: {vDesc}", ""]
  if ftype:
    res.append(preblk(f"type: {convertTypeStr(ftype)}"))
  if readonly:
    res.extend([preblk("readonly")])
    #vDesc = "readonly "+vDesc
  if desc["brief"]:
    res.extend(["  "+desc["brief"],""])
  if mr := desc.get("members"):
    res.extend(membersToRes(desc["members"]))
    res.append("")
  return res


def render_def(desc, level=None):
  titleFunc = titlesMap.get(level, subsectionTitle)
  res = [separator, titleFunc(desc["type"] + " " + desc["name"])]
  if "brief" in desc:
    res.append(desc["brief"])
  if mr := desc.get("members"):
    res.append(membersToRes(mr))
    res.append("")
  res.append(separator)
  return res

def render_code(desc, level=None):
  indent = "    "
  lang = "" if len(desc.get("extras",[]))==0 else desc["extras"][0]
  res = ["", f".. code-block:: {lang}"]
  if n := desc.get("name"):
    res.extend([f"{indent}:caption: {n}",indent])
  res.append("")
  if mr := desc.get("members"):
    res.extend([f"{indent}{m}" for m in mr])
  res.append("")
  return res

def mk_render_tip(tag):
  def render_tip(desc, level=None):
    res = ["", f".. {tag}::", ""]
    res.extend([f"  {m}" for m in desc.get("members",[])])
    res.append("")
    return res
  return render_tip

def render_spoiler(desc, level=None):
  name = desc.get("name", "expand")
  res = ["", f".. dropdown:: {name}", ""]
  res.extend([f"  {m}" for m in desc.get("members", [])])
  res.append("")
  return res

def render_class(desc, level=None):
  res = [separator]
  titleFunc = titlesMap.get(level, subsectionTitle)
  title = desc["type"] + " " + desc["name"]
  if desc["extends"]:
    title += (" extends " +desc["extends"])
  res = [titleFunc(title)]
  res.append(desc["brief"])
  res.append("")
  res.append(membersToRes(desc.get("members")))
  res.append(separator)
  return res

renderersMap = {
  "class":render_class,
  "value":render_value,
  "var":render_value,
  "property":render_value,
  "const":render_value,
  "function":render_func,
  "operator":render_func,
  "ctor":render_ctor,
  "code":render_code,
  "spolier":render_spoiler,
  "dropdown":render_spoiler,
  "expand": render_spoiler,
  "tip": mk_render_tip("tip"),
  "warning": mk_render_tip("warning"),
  "alert": mk_render_tip("danger"),
  "danger": mk_render_tip("danger"),
  "seealso": mk_render_tip("seealso"),
  "note": mk_render_tip("note"),
}

def memberToRes(member):
  return member if isinstance(member, str) else renderersMap.get(member["type"], render_def)(member)


def make_table(grid):
    cell_width = 2 + max(reduce(lambda x,y: x+y, [[len(item) for item in row] for row in grid], []))
    num_cols = len(grid[0])
    rst = table_div(num_cols, cell_width, 0)
    header_flag = 0
    for row in grid:
        rst = rst + '| ' + '| '.join([normalize_cell(x, cell_width-1) for x in row]) + '|\n'
        rst = rst + table_div(num_cols, cell_width, header_flag)
        header_flag = 0
    return rst

def table_div(num_cols, col_width, header_flag=0):
    if header_flag == 1:
        return num_cols*('+' + (col_width)*'=') + '+\n'
    else:
        return num_cols*('+' + (col_width)*'-') + '+\n'

def normalize_cell(string, length):
    return string + ((length - len(string)) * ' ')

def membersToRes(members):
  if not members:
    return None
  res = []
  inComments = None
  for m in members:
    if isinstance(m, str):
      if not inComments:
        inComments = True
      res.append(m)
    elif inComments:
      res.append("")
      res.append(memberToRes(m))
      inComments = False
    else:
      inComments = False
      res.append(memberToRes(m))

  return res

def renderModuleToRst(object):
  pageName = object["name"]
  isModule = object["type"] == "module"
  res = [chapterTitle(pageName)]
  brief = "  " + object["brief"] if object["brief"] else None
  if isModule:
    res.extend(["",f"module '{pageName}'",""])
  res.extend(["",brief,""])
  res.extend(["*Source files:*\n\n*" + "\n".join(object["sources"])+"*","",""])
  isTable = object.get("exports") in ["table", None] and isModule
  members = []
  exports = []
  if isModule:
    members = [f"//'{pageName}' exports:"]
    indent = ""
    if isTable:
      members.append("{")
      indent = "  "
    for m in object["members"]:
      if isinstance(m, str):
        continue
      else:
        typ = m["type"]
        name = m["name"]
        exports.append(name)
        if typ in ["function"]:
          name = get_func_signature(m)
        else:
          name = f"{typ} {name}"
        if typ in ["function","operator"]:
          rstr = ""
          argstr = ""
          if ret := m.get('return'):
            rtype = ret.get('rtype','o')
            rstr = convertTypeStr(rtype)
          if (ps := m.get('params')) or m.get("paramsnum") or m.get("typecheck"):
            args, _ = convertTypemaskAndParamsToParams(m)
            kwarged = m.get('kwarged')
            if kwarged:
              argstr = "table"
            elif args or ps:
              argstr = ", ".join([convertTypeStr(p.get('paramtype','.')) for p in (ps or args)])
          signStr = ""
          if rstr or argstr:
            if not rstr:
              rstr = "_undocumented_"
            signStr = ": " + argstr + " -> " + rstr
          members.append(indent+name + signStr)
        else:
          members.append(indent+name)
    if isTable:
      members.append("}")
  members.append("")
#  grid = [["**Exports**"]]
#  for m in module["members"]:
#    typ = m["type"]
#    grid.append([intag(typ, "*")+" "+intag(m["name"]+("()" if typ=="function" else ""),"**")])
#  members.append(make_table(grid))
#  members.append('')
  if isModule:
    if not isTable:
      res.extend([f"Usage::","",f'  let {pageName} = require("{pageName}")'])
    else:
      res.extend([f"Usage::","",'  let { ' + ", ".join(exports) + ' }' + f' = require("{pageName}")'])
    res.extend(["", "or::","",f'  from "{pageName}" import *'])
    res.append("")
    res.extend(code_block(members))
  res.extend([membersToRes(object["members"]),""])
  return linesToRes(res)

