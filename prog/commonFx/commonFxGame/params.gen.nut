// disable: -file:plus-string
from "dagor.math" import Color3, Point2, Point3

let {file} = require("io")

let glob_types = {}

let codegen = {
  function writeFile(fn, text) {
    println($"writing file: {fn}")
    let f = file(fn, "wt")
    f.writestring(text)
    f.close()
  }
}


let class OutputText {
  text = null

  constructor() {
    this.text=""
  }

  function append(s) {
    local len=s.len()

    for (local i=0; i<len; ) {
      local e=s.indexof("\r\n", i)
      if (e==null) {
        this.text = "".concat(this.text, s.slice(i))
        break
      }

      this.text = "".concat(this.text, s.slice(i, e),"\n")

      i=e+2
    }
  }
}



let glob_decl_text = OutputText()
glob_decl_text.append("// clang-format off  // generated text, do not modify!\n")
glob_decl_text.append("#pragma once\n")
glob_decl_text.append("#include \"readType.h\"\n\n")
glob_decl_text.append("#include <math/dag_curveParams.h>\n")
glob_decl_text.append("#include <fx/dag_paramScript.h>\n\n\n")
glob_decl_text.append("#include <fx/dag_baseFxClasses.h>\n\n\n")
glob_decl_text.append("namespace ScriptHelpers\n")
glob_decl_text.append("{\n");
glob_decl_text.append("class TunedElement;\n");
glob_decl_text.append("};\n\n");


let glob_tools_text = OutputText()
glob_tools_text.append("// clang-format off  // generated text, do not modify!\n")
glob_tools_text.append("#include <generic/dag_tab.h>\n")
glob_tools_text.append("#include <scriptHelpers/tunedParams.h>\n\n")
glob_tools_text.append("#include <fx/effectClassTools.h>\n\n")



let class BaseParam {
  paramName = null

  function createDecl(_name, _decl) {
    error("bug: BaseParam::createDecl() called")
    return null
  }

  function generateDeclText(_text) {}

  function generateLoadText(_text) {}

  function generateTunedParamText() { return "/* FIXME: no code! */" }

  function generateTunedMemberText(text) {
    text.append("".concat("  elems.push_back(", this.generateTunedParamText(), ");\n"))
  }
}



let class InvalidParam (BaseParam) {
  typeName=null

  constructor(name, typ) {
    this.paramName=name
    this.typeName=typ.tostring()
  }

  function generateDeclText(text) {
    text.append($"  //== FIXME: {this.typeName} {this.paramName};  // <- invalid param\n")
  }

  function generateLoadText(text) {
    text.append($"    //== FIXME: {this.paramName}.load(ptr, len);  // <- invalid param\n")
  }

  function generateTunedParamText() { return $"NULL /* FIXME: invalid param {this.paramName} */" }
}



let class RefSlotParam (BaseParam) {
  slotType="Unknown"

  constructor(name, decl) {
    this.paramName = name
    this.slotType = decl.slotType
  }

  function createDecl(name, decl) {
    return this.getclass()(name, decl)
  }

  function generateDeclText(text) {
    text.append($"  void *{this.paramName};\n")
  }

  function generateLoadText(text) {
    if (this.slotType == "Effects") {
      let idName = $"{this.paramName}_id";
      text.append($"    int {idName} = readType<int>(ptr, len);\n")
      text.append($"    {this.paramName} = load_cb->getReference({idName});\n")
      text.append($"    if ({this.paramName} == nullptr && load_cb->getBrokenRefName({idName}))\n")
      text.append($"      G_ASSERTF(0, \"dafx compound: invalid sub fx reference %s for {this.paramName}\", load_cb->getBrokenRefName({idName}));\n")
    }
    else
      text.append($"    {this.paramName} = load_cb->getReference(readType<int>(ptr, len));\n")
  }

  function generateTunedParamText() {
    return $"ScriptHelpers::create_tuned_ref_slot(\"{this.paramName}\", \"{this.slotType}\")"
  }
}

glob_types.ref_slot <- RefSlotParam(null, { slotType="Unknown" });



let class CubicCurveParam (BaseParam) {
  color = Color3(255, 255, 0)

  constructor(name, decl) {
    this.paramName=name
    if ("color" in decl)
      this.color=Color3(decl.color.r, decl.color.g, decl.color.b)
  }

  function createDecl(name, decl) {
    return this.getclass()(name, decl)
  }

  function generateDeclText(text) {
    text.append($"  CubicCurveSampler {this.paramName};\n")
  }

  function generateLoadText(text) {
    text.append($"    {this.paramName}.load(ptr, len);\n")
  }

  function generateTunedParamText() {
    return "ScriptHelpers::create_tuned_cubic_curve(\""+this.paramName+"\", E3DCOLOR("+
      this.color.r.tointeger()+", "+this.color.g.tointeger()+", "+this.color.b.tointeger()+"))";
  }
}

glob_types.cubic_curve <- CubicCurveParam(null, {})

let class GradientBoxParam (BaseParam) {
  constructor(name, _decl) {
    this.paramName=name;
  }

  function createDecl(name, decl) {
    return this.getclass()(name, decl)
  }

  function generateDeclText(text) {
    text.append("  GradientBoxSampler "+this.paramName+";\n")
  }

  function generateLoadText(text) {
    text.append("    "+this.paramName+".load(ptr, len);\n")
  }

  function generateTunedParamText() {
    return "ScriptHelpers::create_tuned_gradient_box(\""+this.paramName+"\")"
  }
}

glob_types.gradient_box <- GradientBoxParam(null, {})



let class SimpleTypeParam (BaseParam) {
  typeName = null

  function generateDeclText(text) {
    text.append("  "+this.typeName+" "+this.paramName+";\n");
  }

  function generateLoadText(text) {
    text.append($"    {this.paramName} = readType<{this.typeName}>(ptr, len);\n")
  }

  function getDefValText() {
    return "/* FIXME: no code! */"
  }

  function generateTunedParamText() {
    return "ScriptHelpers::create_tuned_"+this.typeName+"_param(\""+this.paramName+"\", "+this.getDefValText()+")"
  }
}



let class E3dcolorParam (SimpleTypeParam) {
  defVal = Color3(255, 255, 255)

  constructor(name, decl) {
    this.paramName = name
    this.typeName = "E3DCOLOR"
    if ("defVal" in decl)
      this.defVal=Color3(decl.defVal.r, decl.defVal.g, decl.defVal.b)
  }

  function getDefValText() {
    let {r, g, b} = this.defVal
    return "E3DCOLOR("+ r.tointeger()+", "+g.tointeger()+", "+b.tointeger()+")"
  }

  function createDecl(name, decl) {
    return this.getclass()(name, decl)
  }
}

glob_types.E3DCOLOR <- E3dcolorParam(null, {})

let class IntParam (SimpleTypeParam) {
  defVal = 0

  constructor(name, decl) {
    this.paramName=name;
    this.typeName="int";
    if ("defVal" in decl)
      this.defVal = decl.defVal.tointeger()
  }

  function getDefValText() {
    return this.defVal.tostring()
  }

  function createDecl(name, decl) {
    return this.getclass()(name, decl)
  }
}

glob_types.int <- IntParam(null, {})



let class RealParam (SimpleTypeParam) {
  defVal = 0.0

  constructor(name, decl) {
    this.paramName=name
    this.typeName="real"
    if ("defVal" in decl)
      this.defVal=decl.defVal.tofloat()
  }

  function getDefValText() {
    return this.defVal.tostring()
  }

  function createDecl(name, decl) {
    return this.getclass()(name, decl)
  }
}

glob_types.real <- RealParam(null, {})



let class BoolParam (SimpleTypeParam) {
  defVal = false;

  constructor(name, decl) {
    this.paramName = name
    this.typeName = "bool"
    if ("defVal" in decl)
      this.defVal=!!decl.defVal
  }

  function generateLoadText(text) {
    text.append($"    {this.paramName} = readType<int>(ptr, len);\n")
  }

  function getDefValText() {
    return this.defVal?"true":"false"
  }

  function createDecl(name, decl) {
    return this.getclass()(name, decl)
  }
}

glob_types.bool <- BoolParam(null, {})

let class Point2Param (SimpleTypeParam) {
  defVal = Point2(0, 0)

  constructor(name, decl) {
    this.paramName = name
    this.typeName = "Point2"
    if ("defVal" in decl)
      this.defVal = Point2(decl.defVal)
  }

  function getDefValText() {
    let {x, y} = this.defVal
    return $"Point2({x}, {y})"
  }

  function createDecl(name, decl) {
    return this.getclass()(name, decl)
  }
}

glob_types.Point2 <- Point2Param(null, {})


let class Point3Param (SimpleTypeParam) {
  defVal = Point3(0, 0, 0)

  constructor(name, decl) {
    this.paramName = name
    this.typeName = "Point3"
    if ("defVal" in decl)
      this.defVal = Point3(decl.defVal)
  }

  function getDefValText() {
    let {x, y, z} = this.defVal
    return $"Point3({x}, {y}, {z})"
  }

  function createDecl(name, decl) {
    return this.getclass()(name, decl)
  }
}

glob_types.Point3 <- Point3Param(null, {})


let class TypeRefParam (BaseParam) {
  typeRef = null

  constructor(name, typ) {
    this.paramName=name
    this.typeRef=typ
  }

  function generateDeclText(text) {
    text.append("  "+this.typeRef.paramName+" "+this.paramName+";\n")
  }

  function generateLoadText(text) {
    text.append("    "+this.paramName+".load(ptr, len, load_cb);\n")
  }

  function generateTunedParamText() {
    return this.typeRef.paramName+"::createTunedElement(\""+this.paramName+"\")"
  }
}


let class DynArrayParam (BaseParam) {
  typeRef = null
  memberToShowInCaption = null

  constructor(name, decl) {
    this.paramName=name

    let etype=decl.elemType
    if (etype!=null) {
      if (!(etype in glob_types)) {
        this.typeRef=InvalidParam(etype, etype)
        codegen.error("Unknown type "+etype)
      }
      else
        this.typeRef=glob_types[etype]
    }

    if ("memberToShowInCaption" in decl)
      this.memberToShowInCaption = decl.memberToShowInCaption
  }

  function createDecl(name, decl) {
    return this.getclass()(name, decl)
  }

  function generateDeclText(text) {
    text.append("".concat("  SmallTab<", this.typeRef.paramName, "> "+this.paramName, ";\n"))
  }

  function generateLoadText(text) {
    text.append($"    {this.paramName}.resize(readType<int>(ptr, len));\n")
    text.append($"    for (auto &param : {this.paramName})\n")
    text.append($"      param.load(ptr, len, load_cb);\n")
  }

  function generateTunedParamText() {
    return ""
  }

  function generateTunedMemberText(text) {
    text.append("".concat("  elems.push_back(create_tuned_array(\"", this.paramName, "\", ", this.typeRef.paramName, "::createTunedElement(\"", this.typeRef.paramName, "\")));\n"))
    if (this.memberToShowInCaption != null)
      text.append("".concat("  set_tuned_array_member_to_show_in_caption(*elems.back(), \"", this.memberToShowInCaption, "\");\n\n"))
  }
}

glob_types.dyn_array <- DynArrayParam(null, {elemType=null})



let class EnumParam (BaseParam) {
  entries=null

  constructor(name, decl) {
    this.paramName=name
    this.entries=[]

    local index=0
    foreach (val in decl.list) {
      if (typeof(val) == "string")
        this.entries.append({name=val, value=index++})
      else {
        local i=val.tointeger()

        if (this.entries.len())
          this.entries[this.entries.len()-1].value = i
        index = i+1
      }
    }
  }

  function generateDeclText(text) {
    text.append("  int "+this.paramName+";\n")
  }

  function generateLoadText(text) {
    text.append($"    {this.paramName} = readType<int>(ptr, len);\n")
  }

  function createDecl(name, decl) {
    return this.getclass()(name, decl)
  }

  function generateTunedMemberText(text) {
    text.append("  {\n");
    text.append("    Tab<ScriptHelpers::EnumEntry> enumEntries(tmpmem);\n")
    text.append("    enumEntries.resize("+this.entries.len()+");\n")
    text.append("\n")

    foreach(i,e in this.entries) {
      text.append("    enumEntries["+i+"].name = \""+e.name+"\";\n")
      text.append("    enumEntries["+i+"].value = "+e.value+";\n")
    }

    text.append("\n")
    text.append("    elems.push_back(ScriptHelpers::create_tuned_enum_param(\""+this.paramName+"\", enumEntries));\n")
    text.append("  }\n")
  }
}

glob_types.list <- EnumParam(null, {list=[]})



let class ExternStruct (BaseParam) {
  constructor(name) {
    this.paramName=name
  }

  function createDecl(name, _decl) {
    return TypeRefParam(name, this)
  }
}



let class ParamStruct (BaseParam) {
  members = null
  version = 0

  constructor(name, ver) {
    this.paramName = name
    this.version = ver
    this.members = []
  }


  function createDecl(name, _decl) {
    return TypeRefParam(name, this)
  }


  function generateDeclText(text) {
    text.append("\nclass "+this.paramName+"\n{\npublic:\n")

    foreach (mem in this.members)
      mem.generateDeclText(text)

    text.append("\n\n")
    text.append("  static ScriptHelpers::TunedElement *createTunedElement(const char *name);\n")
    text.append("\n")
    text.append("  void load(const char *&ptr, int &len, BaseParamScriptLoadCB *load_cb)\n")
    text.append("  {\n")
    text.append("    G_UNREFERENCED(load_cb);\n")

    text.append("    CHECK_FX_VERSION(ptr, len, "+this.version+");\n\n")

    foreach (mem in this.members)
      mem.generateLoadText(text)

    text.append("  }\n")
    text.append("};\n")
  }


  function generateToolsText(text) {
    text.append("\n\nScriptHelpers::TunedElement *"+
      this.paramName+"::createTunedElement(const char *name)\n")
    text.append("{\n")
    text.append("  Tab<ScriptHelpers::TunedElement *> elems(tmpmem);\n")
    text.append("  elems.reserve("+this.members.len()+");\n\n")

    foreach (mem in this.members)
      mem.generateTunedMemberText(text)

    text.append("\n  return ScriptHelpers::create_tuned_struct(name, "+this.version+", elems);\n");
    text.append("}\n");
  }


  function addMember(obj) {
    this.members.append(obj)
  }
}



let function declare_extern_struct(name) {
  if (name in glob_types) {
    codegen.error(name+" already declared")
    return
  }

  let s = ExternStruct(name)
  glob_types[name] <- s
}



let function declare_struct(name, version, list) {
  if (name in glob_types) {
    codegen.error(name+" already declared")
    return
  }

  local s = ParamStruct(name, version)
  glob_types[name] <- s

  foreach (index, decl in list) {
    if (!("name" in decl)) {
      s.addMember(InvalidParam("_"+index))
      codegen.error("Unnamed member #"+index+" in "+name)
      continue
    }

    local etype=decl.type
    if (!(etype in glob_types)) {
      s.addMember(InvalidParam(decl.name, etype))
      codegen.error("Unknown type "+etype)
      continue
    }

    s.addMember(glob_types[etype].createDecl(decl.name, decl))
  }

  s.generateDeclText(glob_decl_text)
  s.generateToolsText(glob_tools_text)
}



let function write_declarations(fn) {
  codegen.writeFile(fn, glob_decl_text.text)
}


let function write_tools_code(fn) {
  codegen.writeFile(fn, glob_tools_text.text)
}



local module_name = null


let function include_decl_h(name) {
  local fname = name.slice(0,1).tolower()+name.slice(1)
  if (name!=module_name)
    glob_decl_text.append($"#include <{fname}_decl.h>\n")
  glob_tools_text.append($"#include <{fname}_decl.h>\n")
}


let function begin_declare_params(name) {
  module_name = name
  include_decl_h(name)
}


let function end_module() {
  let fname = "".concat(module_name.slice(0,1).tolower(), module_name.slice(1))
  write_declarations($"{fname}_decl.h")
  write_tools_code($"../commonFxTools/{fname}_tools.cpp")

  module_name=null
}


let function end_declare_params(und_name, version, struct_list) {
glob_tools_text.append("\n")
glob_tools_text.append(@"
class "+module_name+@"EffectTools : public IEffectClassTools
{
public:
  virtual const char *getClassName() { return """+module_name+@"""; }

  virtual ScriptHelpers::TunedElement *createTunedElement()
  {
    Tab<ScriptHelpers::TunedElement *> elems(tmpmem);
    elems.reserve("+struct_list.len()+@");
")
glob_tools_text.append("\n")

foreach(s in struct_list) {
  if (!("name" in s))
    s.name <- s.struct+"_data"
  glob_tools_text.append("    elems.push_back("+s.struct+"::createTunedElement(\""+s.name+"\"));\n")
}

glob_tools_text.append(@"
    return ScriptHelpers::create_tuned_group(""params"", "+version+@", elems);
  }
};

static "+module_name+@"EffectTools tools;


void register_"+und_name+@"_fx_tools() { ::register_effect_class_tools(&tools); }")
glob_tools_text.append("\n")

  end_module()
}

return {
  glob_decl_text
  glob_tools_text
  declare_extern_struct
  declare_struct
  include_decl_h
  begin_declare_params
  end_declare_params
  end_module
}
