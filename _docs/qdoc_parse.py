import re
import json
import pprint
from log import logerror, logwarning
from qdoc_docstring_parse import parse_docstring

"""
todo:
  - ?backscope

  formatting:
    footnotes and citation
    single (probably can be easy)
      @seealso @icon @image @reference @icon @image
  ? full path in scope (module/member/member/name). Can be very easy if we handle multline comments
  ? references
  ? allow structure unreferenced/referenced later (like in luadox)? that is better for squirrel
    but makes code more complex if not support multiline comments as on block as in original
    will help with skipline issue
  ? parse includes for .Func functions
"""

doc_multilne_tags = ["code", "note","tip", "seealso", "danger", "alert", "warning", "dropdown","spoiler", "expand"]
common_fields = ["brief", "parent", "ftype","fvalue"]
function_aliases = ["function","operator","ctor"]

fields_scopes = {
  "paramsnum":function_aliases,
  "kwarged":function_aliases,
  "typemask":function_aliases,
  "vargved":function_aliases,
  "extends": ["class"],
  "writeable":["property"],
}

Leaf = True
Container = False
isRoot = True

objects_with_scopes = [
  ("table", ["table","module","page"], Container),
  ("enum", ["table","module","page"], Container),
  ("class", ["table","module","page"],Container),
  ("module", [None], Container),
  ("page", [None], Container),
  ("function", ["table","class","module","page"], Leaf),
  ("const", ["table","class","module","enum","page"], Leaf),
  ("property", ["class"], Leaf),
  ("var", ["class","enum","table","page"],Leaf),
  ("operator", ["class"], Leaf),
  ("ctor", ["class"], Leaf),
  ("value", ["table","class","module","page"],Leaf),
]
possible_objects_scopes = {}
possible_objects = []
containers_types = []
leaf_types = ["doc"]
root_types = ["module","page"]
for o in objects_with_scopes:
  typ, scopes, isLeaf = o
  possible_objects.append(typ)
  possible_objects_scopes[typ] = scopes
  if isLeaf:
    leaf_types.append(typ)
  else:
    containers_types.append(typ)

def markup_desc(linenum, typ):
  return {"type":typ, "name":linenum, "markup":True, "members":[]}

def mk_mod_desc(name, brief=None, exports=None):
  return {"type":"module", "name":name, "brief":brief, "members":[], "extras":{}, "exports":exports, "members_set":{}}

def mk_page_desc(name, brief=None):
  return {"type":"page", "name":name, "brief":brief, "members":[], "extras":{}}

def mk_class_desc(name, brief=None, parent=None):
  return {"type":"class", "name":name, "brief":brief, "doc":[], "members":[], "extras":{}, "extends":None, "parent":parent, "members_set":{}}

def mk_func_desc(name, brief=None, paramsnum=None, typemask = None, static=False, typ="function", parent=None, docstring=None, params=None, retdesc=None, is_pure=False, kwarged=False, vargved=False, members=None, extras=None):
  return {
    "type":typ,
    "static":static,
    "parent":parent,
    "name":name,
    "brief":brief,
    "params":params or [],
    "return":retdesc,
    "vargved":vargved,
    "members":members or [], #only comments expected!
    "extras":extras or {},
    "paramsnum":paramsnum,
    "typemask":typemask,
    "kwarged":kwarged,
    "is_pure":is_pure,
  }

def mk_fieldmember_desc(name, typ="value", parent=None, brief = None, writeable=None):
  return {
    "type":typ,
    "name":name,
    "parent":parent,
    "ftype":None,
    "fvalue":None,
    "brief":brief,
    "writeable":writeable,
    "members":[], #only comments expected!
    "extras":{},
  }

def mk_container(name, typ, parent=None, brief = None):
  return {
    "type":typ,
    "name":name,
    "brief":brief,
    "parent":parent,
    "members":[],
    "extras":{},
    "members_set":{},
  }

def mk_prop_desc(name, brief=None, parent=None, writeable=None):
  return mk_fieldmember_desc(name, brief=brief, typ="property", parent=parent, writeable=writeable)

def mk_const_desc(name, brief = None, parent=None):
  return mk_fieldmember_desc(name, brief=brief, typ="const", parent=parent)

def mk_var_desc(name, brief=None, parent=None):
  return mk_fieldmember_desc(name, brief=brief, typ="var", parent=parent)

def mk_enum_desc(name, brief=None, parent=None):
  return  mk_container(name, "enum", brief=brief, parent=parent)

def mk_table_desc(name, brief=None, parent=None):
  return mk_container(name, "table", brief=brief, parent=parent)

descs_by_tag = {
  "table": mk_table_desc,
  "function":mk_func_desc,
  "operator":mk_func_desc,
  "ctor":mk_func_desc,
  "value":mk_fieldmember_desc,
  "enum": mk_enum_desc,
  "const": mk_const_desc,
  "var": mk_var_desc,
  "property":mk_prop_desc,
  "class":mk_class_desc,
  "module":mk_mod_desc,
  "page":mk_page_desc,
}

pp = pprint.PrettyPrinter(indent=2)
def pprint(v):
  print(json.dumps(v, indent=2))

def stripWSLine(line, indent=0):
  res = ""
  if len(line) <= indent:
    return line
  for i in range(indent):
    if line[i] != " ": #fixme: whitespace check?
      return line[i:]
  return line[indent:]

class Context:
    def __init__(self):
        self.file_path = None
        self.scopes = []
        self.scope = None
        self.line = None
        self.incomment = False
        self.skip_next_line = False

    def backScope(self):
        self.scopes[:] = self.scopes[:-1]
        if len(self.scopes)>0:
          self.scope = self.scopes[-1]
        else:
          self.scope = None

    def addScope(self, scope):
        if self.scope and self.scope.get("type") not in root_types and self.scope.get("type") in containers_types:
          scope["parent"] = self.scope["name"]
        #scope["sources"]:[self.file_path]
        if scope.get("type") in root_types:
          if scope.get("sources") is None:
            scope["sources"] = []
          if self.file_path not in scope["sources"]:
            scope["sources"].append(self.file_path)
        self.scope = scope
        self.scopes.append(scope)

    def setScope(self, scope):
        if scope.get("type") in root_types:
          if scope.get("sources") is None:
            scope["sources"] = []
          if self.file_path not in scope["sources"]:
            scope["sources"].append(self.file_path)
        self.scope = scope
        self.scopes = [scope]

    def setScopes(self, scopes):
      scope = scopes[-1]
      if scope.get("type") in root_types:
        if scope.get("sources") is None:
          scope["sources"] = []
        if self.file_path not in scope["sources"]:
          scope["sources"].append(self.file_path)
      self.scope = scopes[-1]
      self.scopes = scopes

    def resetScope(self):
        self.scope = None
        self.scopes = []

    def up(self, **kwargs):
        for k, v in kwargs.items():
            setattr(self, k, v)

def pctx(ctx):
  print([attr for attr in dir(ctx) if not attr.startswith('__')])

USE_DOXYGEN_COMMENTS = False

def parse(file_path, data, parsed_result = None, print_res=False):
  file_path = file_path.replace("\\","/")
  result = parsed_result or {}
  ctx = Context()
  ctx.up(file_path = file_path)

  def printErr(error):
    logerror(f'{ctx.file_path}:{ctx.linenum}: {error}. Line:\n {ctx.line}')

  def printWarn(error):
    logwarning(f'{ctx.file_path}:{ctx.linenum}: {error}. Line:\n {ctx.line}')

  def printErrScope(error):
    logerror(f'{ctx.file_path}:{ctx.linenum}: {error}. Line:\n {ctx.line}')
    if ctx.scope:
      logerror("Scope: None")
    else:
      logerror("Scope:")
      logerror(json.dumps(ctx.scope, indent=2))

  def try_to_find_member(desc, doSearchInParents=True):
    if doSearchInParents and len(ctx.scopes) < 1:
      return None
    parent_scope = ctx.scopes[-1] if doSearchInParents else ctx.scope
    if parent_scope.get("type") not in possible_objects_scopes[desc["type"]]:
      return None
    members_set = parent_scope.get("members_set") #perf optimization
    if members_set and parent_scope.get("type") in containers_types:
      found_member_to_update = None
      for memberName in members_set:
        if memberName == desc["name"]:
          found_member_to_update = memberName
          break
      if found_member_to_update:
        for member in parent_scope.get("members"):
          if isinstance(member, str):
            continue
          if member.get("name") == found_member_to_update and member.get("type") in leaf_types:
            member.update(desc)
            newscopes = ctx.scopes[0:-1] + [member] if doSearchInParents else ctx.scopes+[member]
            ctx.setScopes(newscopes)
            return ctx.scope
    return None

  def add_parsed_member(desc):
    if ctx.scope and ctx.scope.get("type") in leaf_types:
      if ctx.scope["name"] == desc["name"]:
        ctx.scope.update(desc)
        return ctx.scope
      if t := try_to_find_member(desc):
        return t
      ctx.backScope()
      return add_parsed_member(desc)
    if ctx.scope and ctx.scope.get("type") not in possible_objects_scopes[desc["type"]]:
      ctx.backScope()
      return add_parsed_member(desc)

    if ctx.scope is not None and ctx.scope["type"] in containers_types:
      scopeMembers = ctx.scope.get("members")
      descIsStr = isinstance(desc, str)
      if scopeMembers is not None:
        if not descIsStr:
          if t := try_to_find_member(desc, False):
            return t
        scopeMembers.append(desc)
        membersSet = ctx.scope.get("members_set")
        if not descIsStr and membersSet is not None:
          membersSet.update({desc["name"]:True})
        ctx.addScope(desc)
        return desc
    else:
      printErrScope("Unreferenced member - not in page, module, table or class scope")
      return True
    return desc

  native_value_definition_re = re.compile(r'''\.(ConstVar|Bind|Var|SetValue|Const|Prop)\s*\(\"([\w\d\_]+)\".*''')
  const_definition_re = re.compile(r'''\.(Const|ConstVar)\s*\(\"[\w\d\_]+\"\s*\,\s*(\S*)\)''')
  mconst_definition_re = re.compile(r'''(CONST)\s*\(\s*([\w\d\_]+)''')
  prop_full_definition_re = re.compile(r'''\.Prop\s*\(\"([\w\d\_]+)\".*\,.*\,.*''')
  writeable_re = re.compile(r'''\.Prop\s*\(\"([\w\d\_]+)\".*\,.*\,.*''')
  cpp_functions = {}

  cpp_function_re = re.compile(r"([\w\*]+)\s+([\w\d\_]+)\s*\((.*)\)")

  def parse_cpp_function(line):
    #bool can_use_embedded_browser()
    #example: static void add_cycle_action(const char* tag, Sqrat::Function func)
    if cpp_function_re.search(line):
      arg_str_idx_start = line.index("(")
      args_str = line[arg_str_idx_start+1 : line.index(")")]
      type_and_name = line[0:arg_str_idx_start].split(" ")
      name = type_and_name[-1]
      rtype = " ".join(type_and_name[0:-1])
      cpp_functions.update({name:(rtype, args_str, line)})
      return True

  def parse_native_fieldmember(line):
    native_definition = native_value_definition_re.search(line)
    const_definition = const_definition_re.search(line)
    mconst_definition = mconst_definition_re.search(line)
    prop_full_definition = prop_full_definition_re.search(line)

    if not (native_definition or mconst_definition):
      return None
    if native_definition:
      typ, name = native_definition.groups()
    elif const_definition:
      typ, name = const_definition.groups()
      typ = "const"
    elif mconst_definition:
      typ, name = mconst_definition.groups()
    typ = typ.lower()
    parent = None
    if ctx.scope and ctx.scope.get("parent") is not None:
      parent = ctx.scope.get("parent")
    desc = mk_fieldmember_desc(name, parent=parent)
    fvalue = None
    if typ=="const" :
      desc["type"] = "const"
      if const_definition:
        _, fvalue = const_definition.groups()
      if fvalue:
        desc["fvalue"] = parse_fieldvalue(fvalue)
    if typ=="var" :
      desc["type"] = "var"
    if typ=="prop":
      writeable = not (not writeable_re.search(line))
      desc = mk_prop_desc(name, writeable=writeable, parent = parent)
    return add_parsed_member(desc)

  native_ctors_re = re.compile(r'''\.(SquirrelCtor|Ctor).*''')
  native_ctors_full_re = re.compile(r'''.*\.SquirrelCtor\(\s*(.*)\s*\,\s*([\-\d]+)\s*\,\s*\"([\w\.\|]+)\"''')
  native_func_re = re.compile(r'''\.\s*(SquirrelFunc|Func|StaticFunc|GlobalFunc|SquirrelCtor|Ctor)\s*\(\"([\w\d\_]+)\"(.*)''')
  simple_func_re = re.compile(r"\.Func\s*\(\"([\w\d\_]+)\"\,\s*([\w\d\_]+)\)")
  native_func_full_re = re.compile(r'''\.\s*(SquirrelFunc|StaticFunc|GlobalFunc)\s*\(\"([\w\d\_]+)\"\s*\,(.*)\s*\,\s*([\-\d]+)\s*\,\s*\"?([\w\.\|\s]+)\"?''')
  native_func_fulldecl_re = re.compile(r'''\.(SquirrelFuncDeclString)\s*\(\s*([^,\"\s][^,\"]*)\s*,\s*\"((?:[^\"\\]|\\.)*)\"\s*(\,\s*\"(.*)\")?\)''')
  cpp_types_parsers = [
    lambda x: "s" if "char" in x else None,
    lambda x: "c" if "Function" in x else None,
    lambda x: "sqObject" if "Sqrat::Object" in x else None,
    lambda x: "o" if "void" in x else None,
    lambda x: "i" if "int" in x else None,
    lambda x: "b" if "bool" in x else None,
    lambda x: "f" if "float" in x else None,
  ]

  def parse_cpp_type(line):
    for i in cpp_types_parsers:
      if s:=i(line):
        return s
    return line

  def parse_native_function(line):
    parent = None
    if ctx.scope:
      if ctx.scope.get("type")=="class":
        parent = ctx.scope.get("name")
      elif ctx.scope.get("parent") is not None:
        parent = ctx.scope.get("parent")
    ctors = native_ctors_re.search(line)
    m = native_func_re.search(line)
    docstringfunc = native_func_fulldecl_re.search(line)
    if not m and not ctors and not docstringfunc:
      return None
    ctorsfull = native_ctors_full_re.search(line)
    mfull = native_func_full_re.search(line)
    typ = "function"
    paramsnum = None
    typemask = None
    cpp_name = None
    docstring = None
    params = None
    retdesc = None
    is_pure = False
    t="function"
    members = None
    if docstringfunc:
      t, cpp_name, docstring, _,  docstring_comments = docstringfunc.groups()
      parsed = parse_docstring(docstring, docstring_comments)
      name = parsed["name"]
      typemask = parsed["typemask"]
      paramsnum = parsed["paramsnum"]
      members = parsed["members"]
      params = parsed["args"]
      retdesc = {"rtype": parsed["rtype"], "description": None}
      is_pure = parsed.get("is_pure", False)

    elif mfull:
      t, name, cpp_name, paramsnum, typemask = mfull.groups()
      try:
        paramsnum = int(paramsnum)
      except ValueError:
        paramsum = None
    elif ctors:
      typ = "ctor"
      name = ""
      t, = ctors.groups()
      if ctorsfull:
        t, paramsnum, typemask = ctorsfull.groups()
        try:
          paramsnum = int(paramsnum)
        except ValueError:
          paramsum = None
    else:
      t, name, _ = m.groups()

    if parent:
      if name.startswith("_") and name[1:] in ["get","set", "add","newslot","delslot","div","mul","sub","modulo","unm","tostring","nexti","call","cmp","typeof","cloned"]:
        typ = "operator"

    desc = mk_func_desc(name, brief=None, paramsnum=paramsnum, typemask=typemask, static = t=="StaticFunc", typ = typ, parent=parent, docstring=docstring, params=params, retdesc=retdesc, is_pure=is_pure, members=members)
    if s:= simple_func_re.search(line):
      _, cpp_func_name = s.groups()
      if cpp_func_name in cpp_functions:
        rtype, args_str, line = cpp_functions[cpp_func_name]
        rtype = rtype.replace("const ", "").replace("static ","")
        if "::" in rtype:
          rtype = rtype.split("::")[-1]
        rtype = rtype.replace("*","")
        args = []
        if args_str:
          for a in args_str.split(","):
            a = a.replace("const ", "").replace("static ","").strip()
            defValueExists = False
            defvalue = None
            if "=" in a:
              a_ = a.split("=")
              a = a_[0].strip()
              defvalue =a_[1].strip()
              defValueExists = True
            a_s = a.split(" ")
            pname = a_s[-1]
            ptype = parse_cpp_type(" ".join(a_s[:-1])) or "."
            ptype = ptype.replace("*","")
            pname = pname.replace("*","")
            if pname == "":
              pname = "arg"
            pdesc = {"name":pname, "paramtype":ptype, "description":None}
            if defValueExists:
              pdesc["defvalue"] = defvalue
            args.append(pdesc)

        desc["params"] = args
        desc["return"] = {"rtype":parse_cpp_type(rtype) or rtype, "description":None}
    return add_parsed_member(desc)

  def parse_param(line):
    """One of
      @param name
      @param name .|i|x|t|Type|List<type>
      @param name .|i|x|t|Type|List<type> def value
      @param name : any description
      @param name .|i|x|t|Type|List<type> : any description
      @param name .|i|x|t|Type|List<type> def value : any description
    """
    least = re.search(r"([\w\d\_]+).*", line)
    if not least:
      return None
    if not ctx.scope or ctx.scope.get("type") not in ["function","ctor","operator","method"]:
      printErrScope("Incorrect @param - not in function scope")
      return None
    name, = least.groups()
    lines = line.split(":")
    defvalue = None
    description = None
    typemask = None
    if len(lines) > 1:
      line = lines[0].strip()
      description = ":".join(lines[1:]).strip()
    typed = re.search(r"[\w\d\_]+\s+([\w\d\<\>\|\.]+).*", line)
    paramtype = "."
    if typed:
      paramtype, = typed.groups()
    defvalued = re.search(r"[\w\d\_]+\s+[\w\d\<\>\|\.]+\s+([\S]+)\s*", line)
    if defvalued:
      defvalue, = defvalued.groups()
      defvalue = parse_fieldvalue(defvalue)
      ctx.scope["params"].append({"name":name, "paramtype":paramtype, "description":description, "defvalue": defvalue})
      return True
    else:
      ctx.scope["params"].append({"name":name, "paramtype":paramtype, "description":description})
      return True

  def parse_fieldvalue(value):
    try:
      value = json.loads(value)
    except ValueError:
      pass
    return value

  def parse_fields(tag, line):
    if tag not in common_fields and tag not in fields_scopes:
      return False
    fieldname = tag
    scopes = fields_scopes.get(tag)
    if isinstance(scopes, str) and scopes is not None:
      scopes = [scopes]
    value = parse_fieldvalue(line)
    if value == '':
      value = True
    if ctx.scope and fieldname in ctx.scope:
      ctx.scope[fieldname] = value
      return True
    if scopes is not None and (not ctx.scope or ctx.scope.get("type") not in scopes):
      printErrScope(f"Incorrect {fieldname} - not in '{scopes}' scope")
      return False
    elif ctx.scope and "extras" in ctx.scope:
      ctx.scope["extras"].update({fieldname:value})
      return True
    return False

  def parse_return(line):
    """ One of
      @return .|i|x|t|Type|List<type>
      @return .|i|x|t|Type|List<type> : any description
      @return : any description
    """
    if not ctx.scope or ctx.scope.get("type") != "function":
      printErr("Incorrect @return - not in function scope")
      return None
    rtype_re = re.search(r"([\w\d\|\.\<\>]+).*", line)
    if rtype_re:
      rtype, = rtype_re.groups()
    lines = line.split(":")
    description = None
    if len(lines) > 1:
      line = lines[0].strip()
      description = ":".join(lines[1:]).strip()
    ctx.scope["return"] = {"rtype":rtype, "description":description}
    return True

  def parse_module_from_name(typ, name):
    moduleName = None
    isNameIsModule = "=" in name
    if "/" in name or "=" in name:
      check = True
      for i in [("/", False), ("=", True)]:
        if i[0] in name:
          namepath = name.split(i[0])
          if len(namepath) == 2:
            moduleName, name = namepath
            check=False
            break
      if check and not moduleName:
        printErr(f"Incorrect @{typ}. Should be @{typ} <name> in scope module|page, or @{typ} <module|page>/<name> or <module>=<name>")
    return moduleName, name, isNameIsModule

  def parse_skip_next_line(line):
    ctx.up(skip_next_line=True)
    return True

  def parse_doc_obj(typ, name, brief, allowedScopes=None):
    if typ in root_types:
      obj = result.get(name)
      if obj is None:
        obj = descs_by_tag[typ](name, brief=brief)
        result[name] = obj
      ctx.setScope(obj)
      return obj
    moduleName, name, isSelfIsModule = parse_module_from_name(typ, name)
    if moduleName:
      mod = result.get(moduleName)
      if mod is None:
        mod = mk_mod_desc(moduleName, exports=name if isSelfIsModule else None)
        result[moduleName] = mod
      ctx.setScope(mod)
    if ctx.scope and ctx.scope.get("name")==name and ctx.scope.get("type") == typ:
      if not ctx.scope["brief"]:
        ctx.scope["brief"] = brief
      return True
    if allowedScopes and ctx.scope.get("type") not in leaf_types and (not ctx.scope or ctx.scope.get("type") not in allowedScopes):
      printErrScope(f"Incorrect {typ} definition - not in {' or '.join(allowedScopes)} scope")
      return True
    parent = None
    if ctx.scope and ctx.scope.get("type") not in root_types and ctx.scope.get("type") not in leaf_types:
      parent = ctx.scope["name"]
    desc = descs_by_tag.get(typ, mk_fieldmember_desc)(name, brief=brief, parent=parent)
    return add_parsed_member(desc)

  objects_re = re.compile(r"([\/\w\d\.\_\=]+)\s*(.*)")

  def parse_objects(tag, line):
    if tag not in possible_objects:
      return False
    possible_objects_str = "|".join(possible_objects)
    typ = tag
    m = objects_re.search(line)
    if not m:
      return None
    name, brief = m.groups()
    if brief == "":
      brief = None
    return parse_doc_obj(typ, name, brief, possible_objects_scopes.get(typ))

  def parse_exit_scope(line):
    ctx.resetScope()
    return True

  def parse_doc_line(line):
    if len(ctx.scopes)==0:
      printWarn("Unexpected qdoc comment. Should be in some scope (module, class, etc)")
      return None
    if ctx.scope and ctx.scope["type"] in leaf_types:
      ctx.scope["members"].append(line)
    else:
      try:
        ctx.scopes[-1]["members"].append(line)
      except:
        printErrScope("couldn't apply qdoc line")
    return True

  parser_by_tag = {
    "skip": parse_skip_next_line,
    "skipline": parse_skip_next_line,
    "resetscope": parse_exit_scope,
    "return": parse_return,
    "param" :parse_param,
  }
  qdoc_single_re = re.compile(r".*///(?!/)\s?(.*)")
  nonqdoc_single_re = re.compile(r"/s*////(.*)")
  qdoc_m_start_re  = re.compile(r"\s*/\*\*(?!\**/)(.*)")
  qdoc_m_start_re2 = re.compile(r"\s*\/\*\s?qdox(.*)")
  qdoc_m_end_re = re.compile(r"\s*(.*)\*\/.*")
  tag_comment_re = re.compile(r"\s*\/\/\/\\s*@(\S+)\s*(.*)")
  tag_re = re.compile(r"\s*@(\S+)\s*(.*)")
  starts_indent_re = re.compile(r"\s*(///)?(?!/)\s*(\S+)")
  mlnTextBlock = None
  mlnIndent = None

  for linenum, line in enumerate(data):
    ctx.up(line = line)
    ctx.up(linenum = linenum)
    parsed = False
    nonqdox = nonqdoc_single_re.search(line)
    if nonqdox:
      continue
    m = qdoc_single_re.search(line)
    ms = qdoc_m_start_re.search(line) if USE_DOXYGEN_COMMENTS else None
    ms2 = qdoc_m_start_re2.search(line)
    me = qdoc_m_end_re.search(line)
    if mlnTextBlock :
      if me:
        mlnTextBlock = None
        mlnIndent = None
      elif ctx.incomment or m:
        if m:
          line, = m.groups()
        if mlnTextBlock["type"]+"@" in line:
          mlnTextBlock = None
          mlnIndent = None
        else:
          mlnTextBlock["members"].append(stripWSLine(line,mlnIndent))
        continue
      else:
        mlnTextBlock = None
    if not ctx.incomment:
      if ctx.skip_next_line:
        ctx.up(skip_next_line = False)
        continue
      if ctx.scope:
        for f in [parse_native_fieldmember, parse_native_function]:
          res = f(line)
          if res:
            parsed = True
            break
      for f in [parse_cpp_function]:
        res = f(line)
        if res:
          parsed = True
          break

    if parsed:
      continue
    if not ctx.incomment:
      if m:
        restline, = m.groups()
        if not restline.startswith("/"):
          line = restline
        else:
          print(restline)
      elif ms2:
        line, = ms2.groups()
        if not line.endswith("*/"):
          ctx.up(incomment=True)
      elif USE_DOXYGEN_COMMENTS and ms:
        line, = ms.groups()
        if not line.endswith("*/"):
          ctx.up(incomment=True)
      else:
        continue
    elif me:
      line, = me.groups()
      ctx.up(incomment = False)
    tag_parsed = tag_re.search(line)
    if tag_parsed:
      tag, rest = tag_parsed.groups()
      pars = parser_by_tag.get(tag)
      if pars:
        pars(rest.strip())
        parsed = True
      elif tag in doc_multilne_tags:
        if "members" in ctx.scope:
          splitted = rest.strip().split()
          name = splitted[0] if len(splitted) else ""
          extras = splitted[0:-1]
          if tag in ["spoiler", "expand", "dropdown"]:
            name = rest.strip()
            extras = []
          desc = {"type":tag, "name": name, "extras":extras, "members":[]}
          ctx.scope["members"].append(desc)
          mlnTextBlock = desc
          mlnIndent = ctx.line.index(tag)-1+2
        else:
          logwarning(f"incorrect scope for multiline tag:{tag}")
        continue
      else:
        for f in [parse_objects, parse_fields]:
          res = f(tag, rest.strip())
          if res:
            parsed = True
            break
    if not parsed:
      parse_doc_line(line)
  if print_res:
    pprint(result)
  return result
