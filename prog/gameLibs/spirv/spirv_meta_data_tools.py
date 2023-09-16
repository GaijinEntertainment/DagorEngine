"""
Tool set to work with spir-v grammar definition JSON files
TODO:
- finish documenting
- finish TODO stuff
- check naming
"""

import re

def mangled_param_with_image_extra(param):
  """
  Special version of parameter type mangling,
  parameters with the name 'Image' are treated as
  image types and 'Sampled Image' as sampled image
  type
  """
  name = '{}'
  if param.is_optional():
    name = name.format('Optional<{}>')
  if param.is_variadic():
    name = name.format('Multiple<{}>')
  if param.name == "'Image'":
    return name.format('Image')
  elif param.name == "'Sampled Image'":
    return name.format('SampledImage')
  return name.format(param.type.get_type_name())

class ParamTypeNode(object):
  """
  Special node type for operand list group search and hierarchy
  With this node type it is much simpler to group instructions
  with the same and partially similar operand lists together.
  """
  def __init__(self, parent = None):
    self.name = ''
    self.param_type = None
    self.param_quantity = None
    self.param_names = dict()
    self.instructions = []
    self.params = []
    self.children = []
    self.parent = parent
    self.level = 0

  def auto_gen_nodes(self, lang, node_parent = None, name_extra = []):
    """
    Tries to find nodes out of groups of operations
    """

    # it has instructions, so we are a candidate for a node
    if self.instructions or len(self.children) > 1:
      newNode = Node(lang)
      newNode.parent = node_parent
      newNode.name = ''.join(name_extra + [self.name])

      for c in self.children:
        newNode.childs.add(c.auto_gen_nodes(lang, newNode))

      return newNode
    elif self.children:
      return self.children[0].auto_gen_nodes(lang, node_parent, name_extra + [self.name])

    return None

  def _mangle_name(self, sep = ',', addlvl = False):
    """
    Concatenates the parent node names
    to a sort of mangled type list string
    The separator can be overridden
    """
    result = self.name
    if addlvl:
      result += '({})'.format(self.level)

    if self.parent:
      pname = self.parent._mangle_name(sep, addlvl)
      if len(pname) > 0:
        return pname + sep + result
    return result

  def add_inst(self, instruction, params = None, link_instruction = False):
    """
    Inserts a new instruction into the node tree,
    missing sub nodes are created as needed
    This can be used to build up a tree with
    repeated calls to this on the root node
    """
    # create param list on demand for convenience
    if not params:
      params = [e for e in instruction.enumerate_all_params()]

    if self.level > 0:
      paramToAdd = params[self.level - 1]
      if link_instruction:
        paramToAdd.param_type_node = self
        self.params.append(paramToAdd)

      self.param_names.setdefault(paramToAdd.name, []).append(paramToAdd)

    if len(params) <= self.level:
      self.instructions.append(instruction)
      if link_instruction:
        instruction.param_type_node = self
    else:
      name = mangled_param_with_image_extra(params[self.level])
      for c in self.children:
        if name == c.name:
          c.add_inst(instruction, params, link_instruction)
          break
      else:
        newNode = ParamTypeNode(self)
        newNode.name = name
        newNode.param_type = params[self.level].type
        newNode.param_quantity = params[self.level].quantity
        newNode.level = self.level + 1
        newNode.add_inst(instruction, params, link_instruction)
        self.children.append(newNode)

  def enumerate_all_instructions_down(self):
    """
    Generator that returns all instructions of this
    node and all its children
    """
    for i in self.instructions:
      yield i
    for c in self.children:
      for i in c.enumerate_all_instructions_down():
        yield i

  def enumerate_all_instructions_down_with_child_filter(self, filt):
    """
    Generator that returns all instructions of this
    node and all its children that pass the filt call.
    filt is called with the node and the node level/parameter index
    """
    for i in self.instructions:
      yield i
    for c in self.children:
      if filt(c, c.level):
        for i in c.enumerate_all_instructions_down_with_child_filter(filt):
          yield i

  def enumerate_all_instrctions_up(self):
    """
    Generator that returns all instruction of this node
    and all its parents.
    """
    for i in self.instructions:
      yield i

    if self.parent:
      for i in self.parent.enumerate_all_instrctions_up():
        yield i

  def print_node_paths(self, addlvl = False):
    """
    Prints the node path from root down to this node
    Node names are concatenated with ->
    It will also print the instructions of this
    node and will call down its children
    """
    print(self._mangle_name('->', addlvl))
    for i in self.instructions:
      print(i.name)
    print('')
    for c in self.children:
      c.print_node_paths(addlvl)

class Node(object):
  """
  Describes a in memory node of a spir-v ast
  TODO:
  - node need a list of basic and extra data fields for instructions
  - this list should also contain links to the instructions and may be the parameters they come from
  """
  def __init__(self, lang):
    self.name = None
    self.parent_name = None
    self.parent = None
    self.properties = set()
    self.childs = set()
    self.instructions = set()
    self.language = lang

  def full_node_name(self):
    """
    Returns the node name with root name as prefix if it is not the root
    """
    if self.parent:
      return self.language.get_root_node().name + self.name
    else:
      return self.name

  def load_from_json(self, json_def):
    """
    Loads node definition from JSON dict
    - name : node name
    - parent : nodes parent name (optional)
    - properties : nodes properties
    """
    self.name = json_def.get('name')
    self.parent_name = json_def.get('parent', None)
    self.properties = set(json_def.get('properties', []))

  def link(self, language):
    """
    Resolves the parents name to the corresponding node object
    """
    if self.parent_name != None:
      self.parent = language.get_node_by_name(self.parent_name)
      if self.parent != None:
        self.parent.childs.add(self)

  def get_depth(self):
    """
    Calculates the distance to the farthest leaf
    """
    d = 0
    for c in self.childs:
      cd = c.get_depth()
      if cd > d:
        d = cd
    return 1 + d

  def get_rdepth(self):
    """
    Calculates the distance to the root node
    """
    if self.parent:
      return 1 + self.parent.get_rdepth()
    return 1

  def get_property_set(self):
    """
    Returns a property set with all properties of this node and its parents
    """
    if self.parent:
      return self.properties | self.parent.get_property_set()
    return self.properties

  def find_matching_nodes(self, properties):
    """
    Tries to find a set of nodes that match the given properties
    """

    # if properties are empty, stop
    if len(properties) == 0:
      return set()

    # if any cap matches or we are root then we check childs
    if len(properties & self.properties) > 0 or self.parent == None:
      lprops = properties - self.properties
      result = set()
      result.add(self)
      for c in self.childs:
        result |= c.find_matching_nodes(lprops)
      return result

    return set()

  def find_best_matching_node(self, properties):
    """
    Tries to find best matching node for the given properties
    """

    # gather all possible usable nodes
    nset = self.find_matching_nodes(properties)
    mc = -1
    bm = None
    # find he node with the most matching properties
    for n in nset:
      nmc = len(n.get_property_set() & properties)
      if nmc > mc:
        mc = nmc
        bm = n
    return bm

  def iterate_all_instruction(self):
    """
    Generator that returns all instructions of this node and all its childs
    """
    for i in self.instructions:
      yield i
    for c in self.childs:
      for i in c.iterate_all_instruction():
        yield i

  def is_leaf(self):
    """
    Checks if this node has any childs or not
    Returns true if it has no childs
    """
    return not self.childs

class Parameter(object):
  """
  Represents a operand of a SPIR-V instruction
  NOTE: parent can either be a instruction or enumerant
  """
  def __init__(self, parent):
    self.name = None
    self.type_name = None
    self.type = None
    self.quantity = None
    self.parent = parent
    self.param_type_node = None

  def load_from_json(self, json_def):
    """
    Loads a parameter (operand) definition from JSON dict
    kind : name of its type
    name : name (optional, fallback is type name)
    quantifier : how often can this parameter appear
                 ? optional
                 * multiple times (including zero times)
                 None exactly once
    """
    self.type_name = json_def.get('kind')
    # not all have a name, use type name instead
    self.name = json_def.get('name', self.type_name)
    self.quantity = json_def.get('quantifier', None)

  def link(self, language):
    """
    Links type name to type def
    """
    self.type = language.get_type_by_name(self.type_name)

  def is_optional(self):
    """
    Returns true if this parameter is optional
    """
    return '?' == self.quantity

  def is_variadic(self):
    """
    Returns true if this parameter can appear multiple times
    """
    return '*' == self.quantity

class Instruction(object):
  """
  Detailed representation of a SPIR-V instruction
  TODO:
  - rename params into perameters for consistency
  """
  def __init__(self, grammar):
    self.name = None
    self.code = None
    self.version = None
    self.params = []
    self.caps = set()
    self.extension_names = set()
    self.extensions = []
    self.properties = set()
    self.grammar = grammar
    self.node = None
    self.is_const_op = False
    self.param_type_node = None

  def load_from_json(self, json_def):
    self.name = json_def.get('opname')
    self.code = json_def.get('opcode')
    self.version = json_def.get('version', None)
    self.caps = set(json_def.get('capabilities', []))
    self.extension_names = set(json_def.get('extensions', []))
    self.is_const_op = False
    if self.grammar.name != 'core':
      # extended grammar ops inherit the ext calls id and type
      # to avoid handling them differently, those are added here
      # (also some spec files include these in docs for clarity)

      idParam = Parameter(self)
      idParam.type_name = u'IdResult'
      idParam.name = u'IdResult'

      typeParam = Parameter(self)
      typeParam.type_name = u'IdResultType'
      typeParam.name = u'IdResultType'

      self.params = [typeParam, idParam]

    else:
      self.params = []

    for p in json_def.get('operands', []):
      newParam = Parameter(self)
      newParam.load_from_json(p)
      self.params.append(newParam)

    # some ops have parameters with identical names (eg none)
    # later code does not expect this and will generate duplicated variable names
    # so we need to fix this
    paramNames = dict()
    for p in self.params:
      paramNames.setdefault(p.name, list())
      paramNames[p.name].append(p)

    for p in paramNames:
      v = paramNames[p]
      if len(v) > 1:
        index = 0
        for n in v:
          n.name = '{}{}'.format(n.name, index)
          index = index +1

  def link(self, language):
    for p in self.params:
      p.link(language)

    self.extensions = []
    for e in self.extension_names:
      ext = language.get_extension(e)
      ext.register_instruction(self)
      self.extensions.append(ext)

    # only resolve if we have no properties that forbid node linkage
    if len(language.no_node_properties & self.properties) == 0:
      rootNode = language.get_root_node()
      if rootNode != None:
        self.node = rootNode.find_best_matching_node(self.properties)
        if self.node != None:
          self.node.instructions.add(self)

  def extend(self, json_def):
    self.properties = set(json_def.get('properties', []))

  def enable_const_op(self):
    self.is_const_op = True

  def has_type(self):
    if len(self.params) < 2:
      return False
    return self.params[0].type.name == 'IdResultType'

  def has_id(self):
    if len(self.params) < 1:
      return False
    # if it has a result type then it has a result also
    return self.params[0].type.name in ['IdResultType', 'IdResult']

  def enumerate_all_params(self):
    if self.has_id():
      if self.has_type():
        iterateParams = self.params[2:]
        yield self.params[1]
      else:
        iterateParams = self.params[1:]
      yield self.params[0]
    else:
      iterateParams = self.params

    for p in iterateParams:
      yield p

class Grammar(object):
  """
  Represents a grammar of SPIR-V besides the core grammar
  multiple extended grammars exist
  """
  def __init__(self, name, lang):
    self.name = name
    self.major = None
    self.minor = None
    self.rev = None
    self.instructions = []
    self.language = lang
    self.index = None
    if name != 'core':
      parts = name.replace('.', '_').split('_')
      if parts[0] == 'SPV':
        parts = parts[1:]

      if parts[0] == 'GLSL':
        self.cpp_op_type_name = ''.join(parts)
      else:
        self.cpp_op_type_name = parts[0] + ''.join([n.capitalize() for n in parts[1:]])
    else:
      self.cpp_op_type_name = 'Op'

  def load_from_json(self, json_def):
    combo_v = int(str(json_def.get('version', 0)), 0)
    self.major = int(str(json_def.get('major_version', combo_v / 100)), 0)
    self.minor = int(str(json_def.get('minor_version', combo_v % 100)), 0)
    self.rev = int(str(json_def.get('revision', 0)), 0)
    self.instructions = []

    for i in json_def.get('instructions', []):
      newInstruction = Instruction(self)
      newInstruction.load_from_json(i)
      self.instructions.append(newInstruction)

  def link(self, language):
    for i in self.instructions:
      i.link(language)

  def extend_instruction(self, iname, json_def):
    for i in self.instructions:
      if iname == i.name:
        i.extend(json_def)
        return True
    return False

  def is_core(self):
    return 'core' == self.name


class Enumerant(object):
  """
  Represents a value enumeration of a BitEnum or ValueEnum type
  Some of them can have additional parameters
  """
  def __init__(self, tp):
    self.name = None
    self.value = None
    self.version = None
    self.caps = set()
    self.extension_names = set()
    self.extensions = []
    self.parameters = []
    self.type = tp
    self.cpp_enum_name = None

  def load_bit_enum_from_json(self, json_def):
    self.name = json_def.get('enumerant')
    self.version = json_def.get('version', None)
    self.value = int(str(json_def.get('value')), 0)
    self.caps = set(json_def.get('capabilities', []))
    self.extension_names = set(json_def.get('extensions', []))
    self.parameters = []
    for p in json_def.get('parameters', []):
      newParam = Parameter(self)
      newParam.load_from_json(p)
      self.parameters.append(newParam)

    if self.name[0].isdigit():
      self.cpp_enum_name = self.type.name + self.name
    else:
      self.cpp_enum_name = self.name

  def load_value_enum_from_json(self, json_def):
    self.load_bit_enum_from_json(json_def)

  def link(self, language):
    for p in self.parameters:
      p.link(language)
    self.extensions = []
    for e in self.extension_names:
      ext = language.get_extension(e)
      ext.register_enumerant(self)
      self.extensions.append(ext)


class Type(object):
  """
  Represents a SPIR-V type
  """
  def __init__(self, grammar):
    self.category = None
    self.name = None
    self.values = []
    self.components = []
    self.component_names = []
    self.doc = None
    self.overlapping_values = False
    self.grammar = grammar
    self.extended_params = False

  def load_composite_from_json(self, json_def):
    self.category = json_def.get('category')
    self.name = json_def.get('kind')
    self.doc = json_def.get('doc', '')
    self.component_names = []
    for c in json_def.get('bases'):
      self.component_names.append(c)

  def load_literal_from_json(self, json_def):
    self.category = json_def.get('category')
    self.name = json_def.get('kind')
    self.doc = json_def.get('doc', '')

  def load_id_from_json(self, json_def):
    self.category = json_def.get('category')
    self.name = json_def.get('kind')
    self.doc = json_def.get('doc', '')

  def load_bit_enum_from_json(self, json_def):
    self.category = json_def.get('category')
    self.name = json_def.get('kind')
    self.doc = json_def.get('doc', '')
    self.values = []
    for v in json_def.get('enumerants', []):
      newEnum = Enumerant(self)
      newEnum.load_bit_enum_from_json(v)
      self.values.append(newEnum)

    # sort them from largest to smallest value so bit testing becomes simpler
    self.value = sorted(self.values, key = lambda d: d.value, reverse = True)

    # check if any bit values overlap, if yes then some algorithms need to behave differently
    for v in self.value:
      for v2 in self.value:
        if v2 != v and v2.value & v.value != 0:
          self.overlapping_values = True
          break
      else:
        # no break hit, keep searching
        continue
      # hit if inner loop hit a break, so we are done searching
      break
    else:
      # never hit any break
      self.overlapping_values = False

  def load_value_enum_from_json(self, json_def):
    self.category = json_def.get('category')
    self.name = json_def.get('kind')
    self.doc = json_def.get('doc', '')
    self.values = []
    for v in json_def.get('enumerants', []):
      newEnum = Enumerant(self)
      newEnum.load_value_enum_from_json(v)
      self.values.append(newEnum)

    # ensure they are sorted from smallest to largest
    self.value = sorted(self.values, key = lambda d: d.value)

  def link(self, language):
    self.components = []
    for cn in self.component_names:
      self.components.append(language.get_type_by_name(cn))

    self.extended_params = False
    for v in self.values:
      v.link(language)
      if len(v.parameters) > 0:
        self.extended_params = True

  def enumerate_unique_values(self):
    visited = set()
    for v in self.values:
      if v.value not in visited:
        visited.add(v.value)
        yield v

  def get_type_name(self):
    if self.category == 'BitEnum':
      return self.name + 'Mask'
    if self.name == 'FPRoundingMode':
      return self.name + 'Value'
    else:
      return self.name

  def is_result_type(self):
    return 'IdResultType' == self.name

  def is_result_id(self):
    return 'IdResult' == self.name

  def is_ref_id(self):
    return 'IdRef' == self.name

  def is_any_input_ref(self):
    return 'Id' == self.category and self.name not in ['IdResultType', 'IdResult']

class Extension(object):
  """
  Represents a extension of the core grammar
  Extensions do not have a grammar of its own
  they are part of the core grammar but they
  are optional
  Enumerants or Instructions can require a extension
  to be usable
  """
  def __init__(self, name):
    self.name = name
    self.instructions = set()
    self.enumerants = set()
    self.index = None

  def register_instruction(self, inst):
    self.instructions.add(inst)

  def register_enumerant(self, enu):
    self.enumerants.add(enu)


class Language(object):
  """
  Language object holds a complete SPIR-V language
  specification.
  It consists of at least one grammar, the core grammar,
  and optional extended grammars with additional instructions.
  """
  def __init__(self):
    self.grammars = dict()
    self.types = dict()
    self.nodes = []
    self.root_node = None
    self.magic = None
    self.no_node_properties = set()
    self.no_param_node_properties = set()
    self.extensions = dict()
    self.param_list_node_root = None
    self.param_list_all_node_root = None

  def load_grammar(self, gname, gdef):
    """
    Loads a grammar from the given JSON def.
    The name of the grammar 'gname' should be 'core'
    for the core grammar. For other grammars this
    should be the string that is used with OpExtInstImport.
    """
    newGrammar = Grammar(gname, self)
    # types are special, they are global
    for t in gdef.get('operand_kinds', []):
      newType = Type(newGrammar)
      cat = t.get('category')
      if cat == 'Composite':
        newType.load_composite_from_json(t)
      elif cat == 'Literal':
        newType.load_literal_from_json(t)
      elif cat == 'Id':
        newType.load_id_from_json(t)
      elif cat == 'BitEnum':
        newType.load_bit_enum_from_json(t)
      elif cat == 'ValueEnum':
        newType.load_value_enum_from_json(t)
      else:
        print('unexpected type category {} for {}'.format(cat, t.get('kind')))
      self.types[newType.name] = newType

    # magic is also global
    self.magic = int(str(gdef.get('magic_number', self.magic)), 0)

    newGrammar.load_from_json(gdef)
    self.grammars[newGrammar.name] = newGrammar

  def load_extened_grammar(self, gedef, warn_handler):
    """
    Load extended grammar specification that holds more information
    for advanced processing of the language.
    It consists of a list of instructions. Each instruction info
    has its grammar name that it belongs to and a optional list
    of properties.
    """
    for i in gedef.get('instructions'):
      iname = i.get('opname')
      # if not specified we assume its core
      igram = i.get('grammar', 'core')

      tgram = self.get_grammar_by_name(igram)
      if tgram == None:
        warn_handler('instruction {} references grammar {} which is not specified, ignoring'.format(iname, igram))
        continue

      if False == tgram.extend_instruction(iname, i):
        warn_handler('instruction {} is not part of grammar {}'.format(iname, igram))

  def load_node_def(self, ndef):
    """
    Loads in memory AST node specification and creates the needed nodes
    """
    for n in ndef.get('nodes', []):
      newNode = Node(self)
      newNode.load_from_json(n)
      self.nodes.append(newNode)
    self.no_node_properties = set(ndef.get('no-node-properties', []))
    self.no_param_node_properties = set(ndef.get('no-param-node-properties', []))

  def load_spec_const_op_white_list(self, wlist_def):
    """
    Loads white list for OpSpecConstantOp of the core grammar
    Instructions that are usable have their property 'is_const_op'
    set to True
    """
    coreGrammar = self.get_core_grammar()
    whiteList = wlist_def.get('instructions', [])
    for i in coreGrammar.instructions:
      if i.name in whiteList:
        i.enable_const_op()

  def finish_loading(self, err_handler):
    """
    After all loads are finished, this has to be called
    to resolve names to object references
    """
    # link types together
    for t in self.types:
      self.types[t].link(self)
    # link nodes to types
    for n in self.nodes:
      n.link(self)

    # find root
    for n in self.nodes:
      if n.parent == None:
        self.root_node = n
        break
    else:
      if len(self.nodes) > 0:
        err_handler('unable to determine root node')

    # link grammar stuff
    for g in self.grammars:
      self.grammars[g].link(self)

    gnames = sorted([g for g in self.grammars if not self.grammars[g].is_core()], key = lambda n: len(n))
    for gi in range(0, len(gnames)):
      self.grammars[gnames[gi]].index = gi

    enames = sorted([e for e in self.extensions], key = lambda n: len(n))
    for ei in range(0, len(enames)):
      self.extensions[enames[ei]].index = ei

    # check if there are other nodes that could be root
    for n in self.nodes:
      if n.parent == None and n != self.root_node:
        err_handler('multiple possible root nodes, found {} but {} was already found as root'.format(n.name, self.root_node.name))

    # build this for instructions that do not have any no node property
    # this tree can be used to build up param info for the ast nodes
    self.param_list_node_root = ParamTypeNode()
    for i in self.enumerate_all_instructions():
      if not (i.properties & self.no_param_node_properties):
        self.param_list_node_root.add_inst(i, None, True)

    # second tree with all instructions in it
    self.param_list_all_node_root = ParamTypeNode()
    for i in self.enumerate_all_instructions():
      self.param_list_all_node_root.add_inst(i)

  def get_grammars(self):
    return self.grammars

  def get_grammar_by_name(self, name):
    if name in self.grammars:
      return self.grammars[name]
    return None

  def get_type_by_name(self, name):
    if name in self.types:
      return self.types[name]
    return None

  def enumerate_all_types(self):
    for t in self.types:
      yield self.types[t]

  def get_node_by_name(self, name):
    for n in self.nodes:
      if n.name == name:
        return n
    return None

  def get_nodes(self):
    return self.nodes

  def get_root_node(self):
    return self.root_node

  def get_extension(self, name):
    if name not in self.extensions:
      self.extensions[name] = Extension(name)
    return self.extensions[name]

  def enumerate_all_instructions(self):
    for g in self.grammars:
      for i in self.grammars[g].instructions:
        yield i

  def enumerate_types(self, type_filter = None):
    """
    Enumerates all types with an optional filter
    The optional type_filter can be a callable
    or a string
    If type_filter is a callable then it is
    called with the type category as parameter.
    The callable type_filter should return false
    if the type should be omitted.
    If type_filter is a string then types with
    a different category name are omitted.
    """
    if callable(type_filter):
      for t in self.types:
        if type_filter(self.types[t].category):
          yield self.types[t]
    elif isinstance(type_filter, basestring):
      for t in self.types:
        if self.types[t].category == type_filter:
          yield self.types[t]
    else:
      for t in self.types:
        yield self.types[t]

  def enumerate_extensions(self):
    for e in self.extensions:
      yield self.extensions[e]

  def enumerate_extended_grammars(self):
    for g in self.grammars:
      if g != 'core':
        yield self.grammars[g]

  def get_core_grammar(self):
    for g in self.grammars:
      if g == 'core':
        return self.grammars[g]
    return None

  def get_param_nodes_root(self):
    """
    Returns the param node tree that only contains
    instructions that are allowed in the ast tree
    too
    """
    return self.param_list_node_root

  def get_param_all_node_root(self):
    """
    Returns the param node tree that contains all instructions
    including those that are not used for ast build
    """
    return self.param_list_all_node_root

def make_extended_grammar_json_ready_def(language):
  """
  Builds a dict that contains the extended grammar specification of the given language
  """
  instructionList = []
  for i in language.enumerate_all_instructions():
    if len(i.properties) > 0:
      instruction = dict()
      instruction['opname'] = i.name
      instruction['grammar'] = i.grammar.name
      instruction['properties'] = list(i.properties)
      instructionList.append(instruction)
  result = dict()
  result['instructions'] = instructionList
  result['info'] = [
    "Auto generated extended grammar by spirv_meta_data_tools.make_extended_grammar_json_ready_def"
  ]
  return result

def mangle_name(anything):
  if 'default' == anything:
    return '_default'
  if 'register' == anything:
    return '_register'
  if 'asm' == anything:
    return 'not_reserved_asm'
  return anything

def multi_replace(s, lst, rpl):
  for l in lst:
    s = s.replace(l, rpl)
  return s

def split_cap_or_space(s):
  secondSet = []
  for s in re.findall('[a-zA-Z][^A-Z]*', s):
    secondSet += [p for p in s.split(' ') if p != '']
  return secondSet

def to_name(anything):
  parts = anything.replace('-', ' ').replace("'", '').replace('<', ' ').replace('>', ' ').replace(',', ' ').replace('.', ' ').replace('~', ' ').split(' ')
  if len(parts) > 1:
    result = ''.join([p.capitalize() for p in parts])
  else:
    result = parts[0]
  result = result[0].lower() + result[1:]
  result = mangle_name(result)
  return result

def to_param_name(anything):
  parts = split_cap_or_space(multi_replace(anything, '-<>,.~_', ' ').replace("'", ''))
  name = '_'.join([p.lower() for p in parts])
  name = mangle_name(name)
  return name

def generate_extended_params_value_names(tipe, allow_short_cut = False, handle_grad = False):
  result = dict()

  for v in tipe.values:
    result[v.name] = []
    if v.name == 'Grad' and handle_grad:
      valueNames = ['x', 'y']
    else:
      valueNames = ['first', 'second', 'third']

    for ei in range(0, len(v.parameters)):
      e = v.parameters[ei]
      if e.type.name == e.name:
        if len(v.parameters) > 1 or not allow_short_cut:
          result[v.name].append(valueNames[ei])
        else:
          result[v.name].append(to_name(v.name))
      else:
        result[v.name].append(to_name(e.name))

  return result
