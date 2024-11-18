#!/usr/bin/env python
"""
Generates SPIR-V Module Nodes header file.
Nodes are the in memory representation of a SPIR-V Module used by the ModuleBuilder.
"""
from __future__ import print_function

from spirv_meta_data_tools import to_name
from spirv_meta_data_tools import to_param_name
from spirv_meta_data_tools import generate_extended_params_value_names

import errno
import json
import os.path
import spirv_meta_data_tools
import re

def make_pretty_extened_grammar_enum_name(name):
  if name.startswith('SPV_'):
    return name[4:]
  return name.replace('.', '_')

def make_param_name(param, index):
  if param.name != param.type.name and '+' not in param.name:
    return to_param_name(param.name)
  elif is_unique_param_type(param.type):
    return to_param_name(param.name)
  else:
    return 'param_{}'.format(index)

def is_unique_param_type(tp):
  """
  Some parameters can only occur once
  Returns true for
  - IdResult
  - IdResultType
  """
  return tp.name == 'IdResult' \
      or tp.name == 'IdResultType' \
      or tp.category == 'ValueEnum' \
      or tp.category == 'BitEnum'

def make_node_param_name(param, index):
  if param.name != param.type.name and '+' not in param.name:
    return to_name(param.name)
  elif is_unique_param_type(param.type):
    return to_name(param.name)
  else:
    return 'param{}'.format(index)

def get_node_with_property(language, prop):
  for node in language.get_nodes():
    if prop in node.properties:
      return node

def translate_nodes_type(t, language, node_as_const = False, use_read_type = True):
  if node_as_const:
    const_mod = 'const '
  else:
    const_mod = ''
  if t.category == 'Composite':
    return 'Node{}'.format(t.name)
  elif t.category == 'Literal':
    if t.name == 'LiteralString':
      return 'eastl::string'
    else:
      return t.name
  elif t.category == 'Id':
    if t.name == 'IdResultType':
      return 'NodePointer<{}>'.format(get_node_with_property(language, 'typedef').full_node_name())
    elif t.name == 'IdResult':
      return 'Id'
    elif t.name == 'IdMemorySemantics':
      return 'NodePointer<{}>'.format(get_node_with_property(language, 'result-type').full_node_name())
    elif t.name == 'IdScope':
      return 'NodePointer<{}>'.format(get_node_with_property(language, 'result-type').full_node_name())
    elif t.name == 'IdRef':
      return 'NodePointer<{}>'.format(get_node_with_property(language, 'allocates-id').full_node_name())

    return 'NodePointer<{}>'.format(get_node_with_property(language, 'allocates-id').full_node_name())
  elif t.category == 'BitEnum':
    if t.extended_params and use_read_type:
      return 'TypeTraits<{}Mask>::ReadType'.format(t.name)
    else:
      return '{}Mask'.format(t.name)
  elif t.category == 'ValueEnum':
    if t.extended_params and use_read_type:
      return 'TypeTraits<{}>::ReadType'.format(t.name)
    else:
      return t.name
  return t.name

def make_combo_type_definitons(language):
  """
  Generates definitions of all composite types.
  Assumes they are pairs of types, should this change
  then this needs adjustment.
  """
  result = ''
  for t in sorted(language.enumerate_types('Composite'), key = lambda n: n.name):
    result += 'struct {}'.format(translate_nodes_type(t, language))
    result += '{\n'
    # usually only pairs, need adjustment if not
    names = ['first', 'second']
    index = 0
    for c in t.components:
      result += '{} {};\n'.format(translate_nodes_type(c, language), names[index])
      index += 1
    result += '};\n'

  return result

def flatten_node_tree(node):
  result = [node]
  for c in node.childs:
    result += flatten_node_tree(c)
  return result

def make_node_declarations_new(language):
  flat_nodes = sorted(flatten_node_tree(language.get_root_node()), key = lambda n: n.name)
  return '\n'.join([
    'enum class NodeKind {', ',\n'.join(n.name for n in flat_nodes), "};",
    # some structs are needed to be declared up front
    '\n'.join(['struct {};'.format(n.full_node_name()) for n in flat_nodes]),
    make_combo_type_definitons(language)
  ])

def make_property_definition(language):
  propertyType = language.get_type_by_name('Decoration')

  result = 'struct Property {\n'
  result += '{} type;\n'.format(propertyType.get_type_name())
  result += 'template<typename T> static constexpr bool is(T *) { return true; }\n'
  result += '};\n'

  # special name property
  # structs can have multiple of those, each for its members
  result += '// Debug names are stored as a property\n'
  result += 'constexpr {0} {0}Name = static_cast<{0}>(0x7FFFFFFF);\n'.format(propertyType.get_type_name())

  result += 'struct PropertyName {\n'
  result += 'const {0} type = {0}Name;\n'.format(propertyType.get_type_name())
  result += 'eastl::optional<Id> memberIndex;\n'
  result += 'eastl::string name;\n'
  result += 'template<typename T> static constexpr bool is(const T *value) { return value->type =='
  result += '{}Name;'.format(propertyType.get_type_name())
  result += '}\n'
  result += 'template<typename N, typename T> void write(N node, T &target) const;\n'
  result += 'CastableUniquePointer<PropertyName> clone() const {\n'
  result += 'CastableUniquePointer<PropertyName> result { new PropertyName };\n'
  result += 'result->memberIndex = memberIndex;\n'
  result += 'result->name = name;\n'
  result += 'return result;\n'
  result += '}\n'
  result += 'auto cloneWithMemberIndexOverride(LiteralInteger i) const { auto v = clone(); v->memberIndex = i.value; return v; }\n'
  result += 'template<typename T> void visitRefs(T) {}\n'
  result += '};\n'

  # special forward pointer property
  # if a pointer is marked with this it needs a forward pointer def to break cycles
  # This property does not hold any extra data and can not have a member index
  result += '// This property marks a pointer type that requires a forward declaration to break type declaration cycles\n'
  result += 'constexpr {0} {0}ForwardDeclaration = static_cast<{0}>(0x7FFFFFFE);\n'.format(propertyType.get_type_name())

  result += 'struct PropertyForwardDeclaration {\n'
  result += 'const {0} type = {0}ForwardDeclaration;\n'.format(propertyType.get_type_name())
  result += 'template<typename T> static constexpr bool is(const T *value) { return value->type =='
  result += '{}ForwardDeclaration;'.format(propertyType.get_type_name())
  result += '}\n'
  result += 'template<typename N, typename T> void write(N node, T &target) const;\n'
  result += 'CastableUniquePointer<PropertyForwardDeclaration> clone() const {\n'
  result += 'CastableUniquePointer<PropertyForwardDeclaration> result { new PropertyForwardDeclaration };\n'
  result += 'return result;\n'
  result += '}\n'
  result += 'auto cloneWithMemberIndexOverride(LiteralInteger) const { return clone(); }\n'
  result += 'template<typename T> void visitRefs(T) {}\n'
  result += '};\n'

  # special selection merge property
  # it matches OpSelectionMerge, it is attached to the block which is ended by the branch instruction
  result += '// This property marks conditional branches and switches with their merge block\n'
  result += 'constexpr {0} {0}SelectionMerge = static_cast<{0}>(0x7FFFFFFD);\n'.format(propertyType.get_type_name())

  result += 'struct PropertySelectionMerge {\n'
  result += 'const {0} type = {0}SelectionMerge;\n'.format(propertyType.get_type_name())
  # TODO: don't use hard coded names!
  result += 'NodePointer<NodeBlock> mergeBlock;\n'
  result += 'SelectionControlMask controlMask;\n'
  result += 'template<typename T> static constexpr bool is(const T *value) { return value->type =='
  result += '{}SelectionMerge;'.format(propertyType.get_type_name())
  result += '}\n'
  result += 'template<typename N, typename T> void write(N, T &target) const;\n'
  result += 'CastableUniquePointer<PropertySelectionMerge> clone() const {\n'
  result += 'CastableUniquePointer<PropertySelectionMerge> result { new PropertySelectionMerge };\n'
  result += 'result->mergeBlock = mergeBlock;\n'
  result += 'result->controlMask = controlMask;\n'
  result += 'return result;\n'
  result += '}\n'
  result += 'auto cloneWithMemberIndexOverride(LiteralInteger) const { return clone(); }\n'
  result += 'template<typename T> void visitRefs(T visitor) { visitor(mergeBlock); }\n'
  result += '};\n'

  # special loop merge property
  # TODO: everything is hardcoded and breaks as soon the spec for this changes
  result += '// This property marks the initial branch of a loop with its merge and continue block\n'
  result += 'constexpr {0} {0}LoopMerge = static_cast<{0}>(0x7FFFFFFC);\n'.format(propertyType.get_type_name())

  result += 'struct PorpertyLoopMerge {\n'
  result += 'const {0} type = {0}LoopMerge;\n'.format(propertyType.get_type_name())
  # TODO: don't use hard coded names!
  result += 'NodePointer<NodeBlock> mergeBlock;\n'
  result += 'NodePointer<NodeBlock> continueBlock;\n'
  result += 'LoopControlMask controlMask;\n'
  result += 'LiteralInteger dependencyLength;\n'
  result += 'template<typename T> static constexpr bool is(const T *value) { return value->type =='
  result += '{}LoopMerge;'.format(propertyType.get_type_name())
  result += '}\n'
  result += 'template<typename N, typename T> void write(N, T &target) const;\n'
  result += 'CastableUniquePointer<PorpertyLoopMerge> clone() const {\n'
  result += 'CastableUniquePointer<PorpertyLoopMerge> result { new PorpertyLoopMerge };\n'
  result += 'result->mergeBlock = mergeBlock;\n'
  result += 'result->continueBlock = continueBlock;\n'
  result += 'result->controlMask = controlMask;\n'
  result += 'result->dependencyLength = dependencyLength;\n'
  result += 'return result;\n'
  result += '}\n'
  result += 'auto cloneWithMemberIndexOverride(LiteralInteger) const { return clone(); }\n'
  result += 'template<typename T> void visitRefs(T visitor) { visitor(continueBlock); visitor(mergeBlock); }\n'
  result += '};\n'

  valuesSeen = dict()
  for v in propertyType.values:
    if v.value in valuesSeen:
      result += "typedef Property{} Property{};\n".format(valuesSeen[v.value], v.name)
    else:
      valuesSeen[v.value] = v.name
      result += 'struct Property{}'.format(v.name)
      result += '{\n'
      result += 'const {0} type = {0}::{1};\n'.format(propertyType.get_type_name(), v.name)
      # properties can be per member, if so, this is then set
      result += 'eastl::optional<Id> memberIndex;\n'
      pIndex = 0
      refParams = []
      for p in v.parameters:
        pName = make_node_param_name(p, pIndex)
        pType = translate_nodes_type(p.type, language, False, False)
        result += '{} {};'.format(pType, pName);
        if p.type.is_any_input_ref() or p.type.is_result_type():
          refParams.append([p, pName])
        pIndex += 1

      result += 'template <typename T> static constexpr bool is(const T *value) {'
      result += 'return value->type == {}::{};'.format(propertyType.get_type_name(), v.name)
      result += '}\n'

      result += 'CastableUniquePointer<Property{}> clone() const {{\n'.format(v.name)
      result += 'CastableUniquePointer<Property{0}> result//\n {{ new Property{0} }};\n'.format(v.name)
      result += 'result->memberIndex = memberIndex;\n'
      pIndex = 0
      for p in v.parameters:
        pName = make_node_param_name(p, pIndex)
        result += 'result->{0} = {0};\n'.format(pName);
        pIndex += 1
      result += 'return result;\n'
      result += '}\n'

      result += 'CastableUniquePointer<Property{}> cloneWithMemberIndexOverride(LiteralInteger i) const {{\n'.format(v.name)
      result += 'CastableUniquePointer<Property{0}> result//\n {{ new Property{0} }};\n'.format(v.name)
      result += 'result->memberIndex = i.value;\n'
      pIndex = 0
      for p in v.parameters:
        pName = make_node_param_name(p, pIndex)
        result += 'result->{0} = {0};\n'.format(pName);
        pIndex += 1
      result += 'return result;\n'
      result += '}\n'

      result += 'template<typename T> void visitRefs(T'
      if refParams:
        result += ' visitor) {'
        for pn in refParams:
          p = pn[0]
          n = pn[1]
          result += 'visitor({});'.format(n)
        result += '}'
      else:
        result += '){}'

      result += '};\n'

  return result

def make_property_writes(language):
  result = 'template<typename N, typename T> void PropertyName::write(N node, T &target) const {\n'
  result += 'auto nameLen = (name.length() + sizeof(unsigned))/ sizeof(unsigned);\n'
  result += 'if (memberIndex)\n'
  result += '{\n'
  result += '  target.beginInstruction(Op::OpMemberName, 2 + nameLen);\n'
  result += '  target.writeWord(node->resultId);\n'
  result += '  target.writeWord(*memberIndex);\n'
  result += '} else {\n'
  result += '  target.beginInstruction(Op::OpName, 1 + nameLen);\n'
  result += '  target.writeWord(node->resultId);\n'
  result += '}\n'
  result += 'target.writeString(name.c_str());\n'
  result += 'target.endWrite();\n'
  result += '}\n'

  result += 'template<typename N, typename T> void PropertyForwardDeclaration::write(N node, T &target) const {\n'
  result += 'target.beginInstruction(Op::OpTypeForwardPointer, 2);\n'
  result += 'target.writeWord(node->resultId);\n'
  result += 'target.writeWord(static_cast<unsigned>(as<NodeOpTypePointer>(node)->storageClass));\n'
  result += 'target.endWrite();\n'
  result += '}\n'

  result += 'template<typename N, typename T> void PropertySelectionMerge::write(N, T &target) const {\n'
  result += 'target.beginInstruction(Op::OpSelectionMerge, 2);\n'
  result += 'target.writeWord(mergeBlock->resultId);\n'
  result += 'target.writeWord(static_cast<unsigned>(controlMask));\n'
  result += 'target.endWrite();\n'
  result += '}\n'

  result += 'template<typename N, typename T> void PorpertyLoopMerge::write(N, T &target) const {\n'
  result += 'target.beginInstruction(Op::OpLoopMerge, 3  + (bool(controlMask & LoopControlMask::DependencyLength) ? 1 : 0));\n'
  result += 'target.writeWord(mergeBlock->resultId);\n'
  result += 'target.writeWord(continueBlock->resultId);\n'
  result += 'target.writeWord(static_cast<unsigned>(controlMask));\n'
  result += 'if (bool(controlMask & LoopControlMask::DependencyLength)) target.writeWord(static_cast<unsigned>(dependencyLength.value)); \n'
  result += 'target.endWrite();\n'
  result += '}\n'

  return result

def make_property_visitor(language):
  propertyType = language.get_type_by_name('Decoration')

  result  = 'template<typename T, typename U>'
  result += 'inline auto visitProperty(T *p, U u) {\n'
  result += 'switch (p->type) {\n'
  for v in propertyType.enumerate_unique_values():
    result += 'case {0}::{1}: return u(reinterpret_cast<Property{1} *>(p));\n'.format(propertyType.get_type_name(), v.name)
  result += '}\n'

  # TODO: hardcoded value names!
  result += 'if (p->type == DecorationName) { return u(reinterpret_cast<PropertyName *>(p)); }\n'
  result += 'if (p->type == DecorationForwardDeclaration) { return u(reinterpret_cast<PropertyForwardDeclaration *>(p)); }\n'
  result += 'if (p->type == DecorationSelectionMerge) { return u(reinterpret_cast<PropertySelectionMerge *>(p)); }\n'
  result += 'if (p->type == DecorationLoopMerge) { return u(reinterpret_cast<PorpertyLoopMerge *>(p)); }\n'
  result += 'return detail::make_default<decltype(u(reinterpret_cast<PorpertyLoopMerge *>(p)))>();\n'

  result += '}\n'

  result += 'template<typename T, typename U>'
  result += 'inline auto visitProperty(const CastableUniquePointer<T> &p, U u) { return visitProperty(p.get(), u); }\n'
  result += 'template<typename T, typename U>'
  result += 'inline auto visitProperty(CastableUniquePointer<T> &p, U u) { return visitProperty(p.get(), u); }\n'

  return result

def make_execution_mode_infos(language):
  executionModelType = language.get_type_by_name('ExecutionMode')
  result  = 'struct {}Base'.format(executionModelType.get_type_name())
  result += '{\n'
  result += 'const {} mode;\n'.format(executionModelType.get_type_name())
  result += 'template<typename T>static constexpr bool is(const T *) { return true; }\n'
  result += 'protected:\n'
  result += '{0}Base({0} m)'.format(executionModelType.get_type_name())
  result += ':mode{m}{}'
  result += '};\n'
  for e in executionModelType.values:
    result += 'struct {0}{1} : {0}Base'.format(executionModelType.get_type_name(), e.name)
    result += '{\n'
    pIndex = 0
    refParams = []
    for p in e.parameters:
      pName = make_node_param_name(p, pIndex)
      pType = translate_nodes_type(p.type, language, False, False)
      result += '{} {};\n'.format(pType, pName)
      if p.type.is_any_input_ref() or p.type.is_result_type():
        refParams.append([p, pName])
      pIndex += 1

    # constructor
    result += '{0}{1}() : {0}Base({0}::{1}) {{}}\n'.format(executionModelType.get_type_name(), e.name)
    result += '~{}{}() = default;\n'.format(executionModelType.get_type_name(), e.name)
    if e.parameters:
      result += '{}{}('.format(executionModelType.get_type_name(), e.name)

      paramList = []
      pIndex = 0
      for p in e.parameters:
        pName = make_param_name(p, pIndex)
        pType = translate_nodes_type(p.type, language, False, False)
        paramList.append('{} {}\n'.format(pType, pName))
        pIndex += 1

      result += ', '.join(paramList)
      result += ') : {0}Base({0}::{1}) {{'.format(executionModelType.get_type_name(), e.name)

      pIndex = 0
      for p in e.parameters:
        nName = make_node_param_name(p, pIndex)
        pName = make_param_name(p, pIndex)
        result += '{} = {};\n'.format(nName, pName)
        pIndex += 1

      result += '}\n'

    # type check
    result += 'template<typename T> static constexpr bool is(const T *v)'
    result += '{'
    result += 'return v->mode == {}::{};'.format(executionModelType.get_type_name(), e.name)
    result += '}\n'

    # ref visitor
    result += 'template<typename T> void visitRefs(T'
    if refParams:
      result += ' visitor) {'
      for pn in refParams:
        p = pn[0]
        n = pn[1]
        result += 'visitor({});'.format(n)
      result += '}'
    else:
      result += '){}'

    result += '};\n'

  result += 'template<typename T, typename U> void executionModeVisitor(T *t, U u) {\n'
  result += 'switch (t->mode) {'

  uniqueSwitchOps = []

  duplicatedEntry = False
  for e in executionModelType.values:
    for j in uniqueSwitchOps:
      if e.value == j:
        duplicatedEntry = True
        result += '//duplicated {} = {} \n'.format(e.name, e.value)
        break
    if duplicatedEntry:
      continue
    uniqueSwitchOps.append(e.value)

    result += 'case {}::{}:\n'.format(executionModelType.get_type_name(), e.name)
    result += 'u(reinterpret_cast<{}{} *>(t)); break;\n'.format(executionModelType.get_type_name(), e.name)
  result += '}\n'
  result += '};\n'
  return result

def make_node_create_function_name(instruction):
  if instruction.grammar.name == 'core':
    return 'new' + instruction.name[2:]
  else:
    if instruction.grammar.cpp_op_type_name.startswith('Debug'):
      if instruction.name.startswith(instruction.grammar.name):
        # if instruction has the full grammar name as prefix drop that
        return 'new' + instruction.grammar.cpp_op_type_name + instruction.name[slice(len(instruction.grammar.name), None)]
      else:
        # drop 'Debug' prefix of each opcode, two times debug in the same name makes no sense
        return 'new' + instruction.grammar.cpp_op_type_name + instruction.name[5:]
    elif not instruction.grammar.cpp_op_type_name.startswith('GLSL'):
      nameSet = instruction.grammar.name.replace('.', '_').split('_')
      if nameSet[0] == 'SPV':
        nameSet = nameSet[1:]
      # chop off the prefix, one time is enough
      iname = instruction.name[slice(0, -len(nameSet[0]))]
      return 'new' + instruction.grammar.cpp_op_type_name + iname
    else:
      return 'new' + instruction.grammar.cpp_op_type_name + instruction.name

def make_instruction_node_struct_name(instruction):
  return '{}Op{}'.format(instruction.grammar.language.get_root_node().name, make_node_create_function_name(instruction)[3:])

def make_node_create_function_param_list_2(language, instruction, defaults):
  # TODO: generate actual variants of parameter lists
  paramList = []
  paramIndex = 0
  needsDefault = False
  for p in instruction.enumerate_all_params():
    if p.type.extended_params:
      paramDef = p.type.get_type_name()
      if p.is_optional():
        paramDef = 'eastl::optional<{}>'.format(paramDef)

      pName = make_param_name(p, paramIndex)
      paramDef = '{} {}'.format(paramDef, pName)
      if needsDefault:
        paramDef += '= {}'
      paramList.append(paramDef)

      # if a parameter has a extend param enum/bit type then we need to provide different overloads
      # for bitfields we only provide one with all as optional
      if p.type.category == 'ValueEnum':
        # mutual exclusive per enum value
        # not needed right now, no value enum is used with that right now, only for properties and such
        raise Exception('unimplemented make_node_create_function_param_list_2 path for ValueEnum with extended values')
      else:
        for v in p.type.values:
          peIndex = 0
          for e in v.parameters:
            if e.name == e.type.name:
              # if it has no dedicated name, then generate one if we have to store all possible extra values
              epName = to_name(v.name)
              # hack for gradient X/Y - JSON def has no names for value dependent parameters
              if v.name == 'Grad':
                epName += ['_x', '_y'][peIndex]
            else:
              # if it has a dedicated name or we don't need all
              # then generate a generic name if needed
              # TODO: this breaks if generic names clash but the type do not match
              epName = make_param_name(e, peIndex)
            epName = '{}_{}'.format(pName, epName)

            # cant be optional or variadic
            epType = translate_nodes_type(e.type, language, False, False)
            if e.type.is_any_input_ref():
              eDef = '{} {}'.format(epType, epName)
            else:
              eDef = 'eastl::optional<{}> {}'.format(epType, epName)
            if defaults:
              eDef += ' = {}'
              needsDefault = True
            paramList.append(eDef)
            peIndex += 1
    else:
      paramDef = translate_nodes_type(p.type, language, True)

      if p.is_optional():
        if '*' in paramDef:
          paramDef = '{} *{}'.format(paramDef, make_param_name(p, paramIndex))
          if defaults:
            paramDef += ' = nullptr'
            needsDefault = True
        else:
          paramDef = 'eastl::optional<{}> {}'.format(paramDef, make_param_name(p, paramIndex))
          if defaults:
            paramDef += ' = {}'
            needsDefault = True

        paramList.append(paramDef)
      elif p.is_variadic():
        paramDef = '{} *{}'.format(paramDef, make_param_name(p, paramIndex))
        if defaults:
          paramDef += ' = nullptr'
          needsDefault = True

        paramList.append(paramDef)

        paramDef = 'size_t {}_count'.format(make_param_name(p, paramIndex))
        if defaults:
          paramDef += ' = 0'
          needsDefault = True
        paramList.append(paramDef)
      else:
        paramList.append('{} {}'.format(paramDef, make_param_name(p, paramIndex)))

    paramIndex += 1

  return paramList

def make_node_is_implementation(node):
  result = ''
  result += 'template<typename T> inline constexpr bool {}::is(const T *val)\n'.format(node.full_node_name())
  result += '{'
  if not node.parent:
    result += 'return true;'
  else:
    result += 'return '
    result += '||'.join(['val->nodeKind == NodeKind::{}'.format(node.name)] + ['{}::is(val)'.format(c.full_node_name()) for c in sorted(node.childs, key = lambda n: n.name)])
    result += ';'
  result += '}\n'
  for i in sorted(node.instructions, key = lambda n: n.name):
    result += 'template<typename T> inline constexpr bool {}::is(const T *val)'.format(make_instruction_node_struct_name(i))
    result += '{'
    if i.grammar.is_core():
      result += 'return val->opCode == Op::{};'.format(i.name)
    else:
      result += 'return val->opCode == Op::OpExtInst &&'
      result +=' val->grammarId == ExtendedGrammar::{0} && static_cast<{1}>(val->extOpCode) == {1}::{2};'.format(make_pretty_extened_grammar_enum_name(i.grammar.name), i.grammar.cpp_op_type_name, i.name)
    result +=' }\n'
  for c in sorted(node.childs, key = lambda n: n.name):
    result += make_node_is_implementation(c)

  return result

def make_node_definitions_new_for_node(node):
  """
  Generates all node definitions of the given node, its children
  and instructions.
  A node definition stores, opcode, extended grammar identification,
  extended grammar op code and any extra data depending on the
  instruction and node type.
  """

  # build the node it self first
  result = 'struct {}'.format(node.full_node_name())
  if node.parent:
    result += ' : {}'.format(node.parent.full_node_name())
  result += '{'

  if not node.parent:
    result += 'NodeKind nodeKind;\n'
    result += 'Op opCode;\n'
    result += 'Id extOpCode;\n'
    result += 'ExtendedGrammar grammarId;\n'

  elif 'allocates-id' in node.properties:
    result += 'Id resultId;\n'
    result += 'eastl::vector<CastableUniquePointer<Property>> properties;\n'

  elif 'result-type' in node.properties:
    result += 'NodePointer<{}> resultType;\n'.format(get_node_with_property(node.language, 'typedef').full_node_name())

  elif 'unary' in node.properties:
    result += 'NodePointer<{}> first;\n'.format(get_node_with_property(node.language, 'allocates-id').full_node_name())

  elif 'binary' in node.properties:
    result += 'NodePointer<{}> first;\n'.format(get_node_with_property(node.language, 'allocates-id').full_node_name())
    result += 'NodePointer<{}> second;\n'.format(get_node_with_property(node.language, 'allocates-id').full_node_name())

  elif 'trinary' in node.properties:
    result += 'NodePointer<{}> first;\n'.format(get_node_with_property(node.language, 'allocates-id').full_node_name())
    result += 'NodePointer<{}> second;\n'.format(get_node_with_property(node.language, 'allocates-id').full_node_name())
    result += 'NodePointer<{}> third;\n'.format(get_node_with_property(node.language, 'allocates-id').full_node_name())

  elif 'multinary' in node.properties:
    # for multinary ops (ops with more than 3 inputs) we store the number of inputs and an array which then can be
    # accessed with the range from 0 to pCount safely (memory backing comes from the underlying op structure)
    result += 'Id pCount;\n'
    result += '// operands can be more, this depends on the opcode\n'
    result += 'NodePointer<{}> operands[4];\n'.format(get_node_with_property(node.language, 'allocates-id').full_node_name())

  elif 'function-begin' in node.properties:
    result += 'FunctionControlMask functionControl;\n'
    result += 'NodePointer<{}> functionType;\n'.format(get_node_with_property(node.language, 'typedef').full_node_name())
    result += 'eastl::vector<NodePointer<{}>> body;\n'.format(get_node_with_property(node.language, 'block-begin').full_node_name())
    result += 'eastl::vector<NodePointer<{}>> parameters;\n'.format(get_node_with_property(node.language, 'variable').full_node_name())

  elif 'function-call' in node.properties:
    result += 'NodePointer<{}> function;\n'.format(get_node_with_property(node.language, 'result-type').full_node_name())
    result += 'eastl::vector<NodePointer<{}>> parameters;\n'.format(get_node_with_property(node.language, 'result-type').full_node_name())

  elif 'block-begin' in node.properties:
    result += 'eastl::vector<NodePointer<{}>> instructions;\n'.format(node.language.get_root_node().name)
    result += 'NodePointer<{}> exit;\n'.format(get_node_with_property(node.language, 'ends-block').full_node_name())

  elif 'switch-section' in node.properties:
    result += 'NodePointer<{}> value;\n'.format(get_node_with_property(node.language, 'result-type').full_node_name())
    result += 'NodePointer<{}> defaultBlock;\n'.format(get_node_with_property(node.language, 'block-begin').full_node_name())
    result += 'struct Case {{ LiteralInteger value; NodePointer<{}> block;}};\n'.format(get_node_with_property(node.language, 'block-begin').full_node_name())
    result += 'eastl::vector<Case> cases;\n'

  elif 'scoped' in node.properties:
    result += 'NodePointer<{}> scope;\n'.format(get_node_with_property(node.language, 'result-type').full_node_name())

  elif 'grouped' in node.properties:
    result += 'GroupOperation groupOperation;\n'
    result += 'NodePointer<{}> value;\n'.format(get_node_with_property(node.language, 'result-type').full_node_name())

  elif 'spec-op' in node.properties:
    result += 'NodePointer<NodeOperation> specOp;\n'

  elif 'phi' in node.properties:
    result += 'struct SourceInfo {\n'
    result += 'NodePointer<{}> block;\n'.format(get_node_with_property(node.language, 'block-begin').full_node_name())
    result += 'NodePointer<{}> source;\n'.format(get_node_with_property(node.language, 'result-type').full_node_name())
    result += '};\n'
    result += 'eastl::vector<SourceInfo> sources;\n'

  elif 'switch-section' in node.properties:
    result += 'NodePointer<{}> value;\n'.format(get_node_with_property(node.language, 'result-type').full_node_name())
    result += 'struct CaseInfo {\n'
    result += 'LiteralInteger value;\n'
    result += 'NodePointer<{}> block;\n'.format(get_node_with_property(node.language, 'block-begin').full_node_name())
    result += '};\n'
    result += 'NodePointer<{}> defaultBlock;\n'
    result += 'eastl::vector<CaseInfo> cases;\n'

  result += 'template<typename T> static constexpr bool is(const T *value);\n'

  result += 'template<typename T> static bool visit({0} *node, T visitor);\n'.format(node.full_node_name())
  result += 'template<typename T> void visitRefs(T) {}\n'

  result += '};\n'

  for i in sorted(node.instructions, key = lambda n: n.name):
    # instructions do not have any parent type in their cpp code
    # they are described with their cast info table.
    # This gives each instruction node the freedom to name any
    # parameter as it wants to.
    result += 'struct {}'.format(make_instruction_node_struct_name(i))
    result += '{'
    result += 'const NodeKind nodeKind = NodeKind::{};\n'.format(node.name)

    # on construction assign their corresponding values
    # make all opcode related params const to make it impossible to
    # fiddle with them (non instruction nodes are intentionally not const)
    if i.grammar.is_core():
      result += 'const Op opCode = Op::{};\n'.format(i.name)
      result += 'const Id extOpCode = {};\n'
      result += 'const ExtendedGrammar grammarId = ExtendedGrammar::Count;\n'
    else:
      result += 'const Op opCode = Op::OpExtInst;\n'
      # we can use the enum type instead, this is ok underlying type is the same
      result += 'const {0} extOpCode = {0}::{1};\n'.format(i.grammar.cpp_op_type_name, i.name)
      result += 'const ExtendedGrammar grammarId = ExtendedGrammar::{};\n'.format(make_pretty_extened_grammar_enum_name(i.grammar.name))

    if 'function-begin' in i.properties:
      result += 'Id resultId = 0;\n'
      result += 'eastl::vector<CastableUniquePointer<Property>> properties;\n'
      result += 'NodePointer<{}> resultType;\n'.format(get_node_with_property(node.language, 'typedef').full_node_name())
      result += 'FunctionControlMask functionControl;\n'
      result += 'NodePointer<{}> functionType;\n'.format(get_node_with_property(node.language, 'typedef').full_node_name())
      result += 'eastl::vector<NodePointer<{}>> body;\n'.format(get_node_with_property(node.language, 'block-begin').full_node_name())
      result += 'eastl::vector<NodePointer<{}>> parameters;\n'.format(get_node_with_property(node.language, 'variable').full_node_name())

      result += 'template<typename T> void visitRefs(T visitor) {visitor(resultType);visitor(functionType);for(auto &&p : parameters)visitor(p); for(auto &&b : body)visitor(b);}\n'

    elif 'function-call' in i.properties:
      result += 'Id resultId = 0;\n'
      result += 'eastl::vector<CastableUniquePointer<Property>> properties;\n'
      result += 'NodePointer<{}> resultType;\n'.format(get_node_with_property(node.language, 'typedef').full_node_name())
      result += 'NodePointer<{}> function;\n'.format(get_node_with_property(node.language, 'result-type').full_node_name())
      result += 'eastl::vector<NodePointer<{}>> parameters;\n'.format(get_node_with_property(node.language, 'result-type').full_node_name())

      result += 'template<typename T> void visitRefs(T visitor) {visitor(resultType);visitor(function);for(auto &&p : parameters)visitor(p);}\n'

    elif 'block-begin' in i.properties:
      result += 'Id resultId = 0;\n'
      result += 'eastl::vector<CastableUniquePointer<Property>> properties;\n'
      result += 'eastl::vector<NodePointer<{}>> instructions;\n'.format(node.language.get_root_node().name)
      result += 'NodePointer<{}> exit;\n'.format(get_node_with_property(node.language, 'ends-block').full_node_name())

      result += 'template<typename T> void visitRefs(T visitor) {for(auto &&i : instructions)visitor(i);visitor(exit);}\n'

    elif 'switch-section' in i.properties:
      result += 'NodePointer<{}> value;\n'.format(get_node_with_property(node.language, 'result-type').full_node_name())
      result += 'NodePointer<{}> defaultBlock;\n'.format(get_node_with_property(node.language, 'block-begin').full_node_name())
      result += 'struct Case {{ LiteralInteger value; NodePointer<{}> block;}};\n'.format(get_node_with_property(node.language, 'block-begin').full_node_name())
      result += 'eastl::vector<Case> cases;\n'

      result += 'template<typename T> void visitRefs(T visitor) {visitor(value);visitor(defaultBlock);for(auto &&c : cases)visitor(c.block);}\n'

    elif 'spec-op' in i.properties:
      result += 'Id resultId = 0;\n'
      result += 'eastl::vector<CastableUniquePointer<Property>> properties;\n'
      result += 'NodePointer<NodeTypedef> resultType;\n'
      result += 'NodePointer<NodeOperation> specOp;\n'

      result += 'template<typename T> void visitRefs(T visitor) {specOp->visitRefs(visitor);}\n'

    elif 'phi' in i.properties:
      result += 'Id resultId = 0;\n'
      result += 'eastl::vector<CastableUniquePointer<Property>> properties;\n'
      result += 'NodePointer<NodeTypedef> resultType;\n'
      result += 'struct SourceInfo {\n'
      result += 'NodePointer<{}> block;\n'.format(get_node_with_property(node.language, 'block-begin').full_node_name())
      result += 'NodePointer<{}> source;\n'.format(get_node_with_property(node.language, 'result-type').full_node_name())
      result += '};\n'
      result += 'eastl::vector<SourceInfo> sources;\n'

      result += 'template<typename T> void visitRefs(T visitor) {visitor(resultType);for(auto &&s : sources){visitor(s.block); visitor(s.source);}}\n'

    else:
      pIndex = 0
      hasCounter = False
      visitorParams = []
      for p in i.enumerate_all_params():

        pName = make_node_param_name(p, pIndex)
        if pName == 'idResult':
          pName = 'resultId = 0'
        elif pName == 'idResultType':
          pName = 'resultType'

        if p.type.is_any_input_ref() or p.type.is_result_type():
          visitorParams.append([p, pName])
        elif p.type.extended_params:
          foundOne = False
          for v in p.type.values:
            if not foundOne:
              for vp in v.parameters:
                if vp.type.is_any_input_ref():
                  visitorParams.append([p, pName])
                  foundOne = True
                  break

        pType = translate_nodes_type(p.type, node.language, False, False)
        if p.is_optional():
          # also use optional for refs to make it clear that it is optional
          pType = 'eastl::optional<{}>'.format(pType)
        elif p.is_variadic():
          pType = 'eastl::vector<{}>'.format(pType)

        if 'multinary' in i.properties:
          # have to insert the counter with constant value after id result and id result type
          if not (hasCounter or p.type.is_result_type() or p.type.is_result_id()):
            tnode = i.param_type_node
            while tnode:
              if tnode.param_type.is_ref_id():
                break
              tnode = tnode.parent

            paramCount = 0;
            while tnode.param_type.is_ref_id():
              paramCount += 1
              tnode = tnode.parent
              if not tnode or not tnode.param_type:
                break

            result += 'const Id pCount = {};\n'.format(paramCount)
            hasCounter = True

        result += '{} {};\n'.format(pType, pName)
        # if it has a id, it can have properties and a name
        if p.type.is_result_id():
          result += 'eastl::vector<CastableUniquePointer<Property>> properties;\n'

        if p.type.extended_params:
          result += '// extra values for {}\n'.format(pName)
          # if a type has extended params we append them appropriately
          # TODO: some instructions only accept a certain subset of the values
          #       currently are all allowed (without any extra meta data it
          #       is not possible to narrow this down)

          # bit mask types potentially need to store extra data for all bits
          needsAll = p.type.category == 'BitEnum'
          # if they type is a enum value and no bit mask, then only extra data for one value at a time needs to be store
          # so try to reduce them to the bare minimum
          uniqueExtras = set()

          extParamNames = generate_extended_params_value_names(p.type, True, True)
          for v in p.type.values:
            peIndex = 0
            for e, vpn in zip(v.parameters, extParamNames[v.name]):
              if vpn == to_name(v.name):
                epName = '{}{}{}'.format(pName, vpn[0].upper(), vpn[1:])
              else:
                epName = '{}{}{}{}'.format(pName, v.name, vpn[0].upper(), vpn[1:])

              # cant be optional or variadic
              epType = translate_nodes_type(e.type, node.language, False, False)
              eDef = '{} {}'.format(epType, epName)
              if needsAll:
                result += '{};\n'.format(eDef)
              else:
                uniqueExtras.add(eDef)
              peIndex += 1

          for v in uniqueExtras:
            result += '{};\n'.format(v)

        pIndex += 1

      # generate ref visitor
      result += 'template<typename T> void visitRefs(T'
      if visitorParams:
        result += ' visitor) {'
        for pn in visitorParams:
          p = pn[0]
          n = pn[1]

          if p.type.extended_params:
            extParamNames = generate_extended_params_value_names(p.type, True, True)
            for v in p.type.values:
              for vp, vpn in zip(v.parameters, extParamNames[v.name]):
                if vpn == to_name(v.name):
                  vpName = '{}{}{}'.format(n, vpn[0].upper(), vpn[1:])
                else:
                  vpName = '{}{}{}{}'.format(n, v.name, vpn[0].upper(), vpn[1:])
                if vp.type.is_any_input_ref():
                  result += 'if ({0}) visitor({0});\n'.format(vpName)
          elif p.is_optional():
            result += 'if ({0}) visitor(*{0});'.format(n)
          elif p.is_variadic():
            result += 'for(auto &&ref : {0})visitor(ref);'.format(n)
          else:
            result += 'visitor({});'.format(n)
        result += '}'
      else:
        result += '){}'

    # generate constructors
    result += '{}()=default;\n'.format(make_instruction_node_struct_name(i))
    result += '~{}()=default;\n'.format(make_instruction_node_struct_name(i))
    result += '{0}(const {0} &)=delete;\n'.format(make_instruction_node_struct_name(i))
    result += '{0}& operator=(const {0} &)=delete;\n'.format(make_instruction_node_struct_name(i))
    result += '{0}({0} &&)=delete;\n'.format(make_instruction_node_struct_name(i))
    result += '{0}& operator=({0} &&)=delete;\n'.format(make_instruction_node_struct_name(i))

    # todo make variants with and without id_result
    paramList = make_node_create_function_param_list_2(node.language, i, True)
    if paramList:
      result += '{}('.format(make_instruction_node_struct_name(i))

      if 'spec-op' in i.properties:
        paramList.pop()
        paramList.append('NodePointer<NodeOperation> spec_op')

      result += ', '.join(paramList)
      result += ') {\n'

      if 'function-begin' in i.properties:
        result += 'resultType = id_result_type;\n'
        result += 'resultId = id_result;\n'
        result += 'functionControl = function_control;\n'
        result += 'functionType = function_type;\n'

      elif 'function-call' in i.properties:
        result += 'resultType = id_result_type;\n'
        result += 'resultId = id_result;\n'
        result += 'function = function;\n'
        result += 'parameters.assign(param_3, param_3 + param_3_count);\n'

      elif 'switch-section' in i.properties:
        result += 'value = selector;\n'
        result += 'defaultBlock = _default;\n'
        result += 'cases.reserve(target_count);\n'
        result += 'for (int i = 0; i < target_count; ++i) {\n'
        result += 'Case cblock;\n'
        result += 'cblock.value = target[i].first;\n'
        result += 'cblock.block = target[i].second;\n'
        result += 'cases.push_back(cblock);\n'
        result += '}\n'

      elif 'phi' in i.properties:
        result += 'resultId = id_result;'
        result += 'resultType = id_result_type;'
        result += 'sources.reserve(variable_parent_count);\n'
        result += 'for (int i = 0; i < variable_parent_count; ++i) {\n'
        result += 'SourceInfo info;\n'
        result += 'info.source = variable_parent[i].first;\n'
        result += 'info.block = variable_parent[i].second;\n'
        result += 'sources.push_back(info);\n'
        result += '}\n'

      elif 'spec-op' in i.properties:
        result += 'resultId = id_result;\n'
        result += 'resultType = id_result_type;\n'
        result += 'specOp = spec_op;\n'

      else:
        pIndex = 0
        for p in i.enumerate_all_params():
          pDstName = make_node_param_name(p, pIndex)
          if pDstName == 'idResult':
            pDstName = 'resultId'
          elif pDstName == 'idResultType':
            pDstName = 'resultType'

          pSrcName = make_param_name(p, pIndex)

          if p.is_variadic():
            result += 'this->{0}.assign({1}, {1} + {1}_count);\n'.format(pDstName, pSrcName)
          else:
            result += 'this->{} = {};\n'.format(pDstName, pSrcName)

          if p.type.extended_params:
            extParamNames = generate_extended_params_value_names(p.type, True, True)
            for v in p.type.values:
              for vp, vpn in zip(v.parameters, extParamNames[v.name]):
                if vpn == to_name(v.name):
                  vpName = '{}{}{}'.format(pDstName, vpn[0].upper(), vpn[1:])
                  vppName = '{}_{}'.format(pSrcName, vpn)
                else:
                  vppName = '{}_{}_{}'.format(pSrcName, v.name.lower(), vpn)
                  vpName = '{}{}{}{}'.format(pDstName, v.name, vpn[0].upper(), vpn[1:])
                if vp.type.is_any_input_ref():
                  result += 'this->{} = {};\n'.format(vpName, vppName)
                else:
                  result += 'if ({})'.format(vppName)
                  result += 'this->{} = *{};\n'.format(vpName, vppName)

          pIndex += 1

      result += '}\n'

    result += 'template<typename T> static constexpr bool is(const T *value);\n'

    result += '};\n'

  for c in sorted(node.childs, key = lambda n: n.name):
    result += make_node_definitions_new_for_node(c)

  # visitor handler implementation
  result += 'template<typename T> inline bool {0}::visit({0} *node, T visitor)\n'.format(node.full_node_name())
  result += '{\n'
  if not node.childs and len(node.instructions) == 1:
    # special case where the node has no children and only one instruction
    # can safely castes to that op
    result += '// simplified case where this node has no children and only one instruction\n'

    for i in sorted(node.instructions, key = lambda n: n.name):
      result += 'if (visitor(reinterpret_cast<{} *>(node)))'.format(make_instruction_node_struct_name(i))
      result += 'return true;\n'
      break

  else:
    if node.instructions:
      # if the node has instructions of its own, try it first
      # if the kind matches, then it is one of its instructions
      result += 'if (node->nodeKind == NodeKind::{})'.format(node.name)
      result += '{\n'
      # first gather them in sets that are needed to be handled differently
      extendedOps = dict()
      coreOps = []
      for i in sorted(node.instructions, key = lambda n: n.name):
        if i.grammar.is_core():
          coreOps.append(i)
        else:
          extendedOps.setdefault(i.grammar, []).append(i)

      # if we have only one instruction then we don't need a switch
      useOpCodeSwitch = False
      if len(coreOps) > 1:
        useOpCodeSwitch = True
      elif coreOps:
        if extendedOps:
          useOpCodeSwitch = True

      if useOpCodeSwitch:
        result += 'switch (node->opCode) {'
        # silence the whining about not all handled cases for some compilers
        result += 'default: break;\n'

      # make handlers for core grammar instructions
      uniqueSwitchOps = []
      for i in coreOps:
        if useOpCodeSwitch:

          duplicatedEntry = False
          for j in uniqueSwitchOps:
            if i.code == j:
              duplicatedEntry = True
              result += '//duplicated {} = {} \n'.format(i.name, i.code)
              break

          if duplicatedEntry:
            continue

          uniqueSwitchOps.append(i.code)

          result += 'case Op::{}:'.format(i.name)
        else:
          result += '// only one operation\n'
          result += 'if (node->opCode == Op::{}) {{'.format(i.name)

        result += 'if(visitor(reinterpret_cast<{} *>(node))) return true;\n'.format(make_instruction_node_struct_name(i))
        if useOpCodeSwitch:
          result += 'break;\n'

      if extendedOps:
        if useOpCodeSwitch:
          result += 'case Op::OpExtInst:'


        useGrammaSwitch = len(extendedOps) > 1
        if useGrammaSwitch:
          result += 'switch (node->grammarId) {'
          # silence the whining about not all handled cases for some compilers
          result += 'default: break;\n'

        for g in extendedOps:
          if useGrammaSwitch:
            result += 'case ExtendedGrammar::{}:'.format(make_pretty_extened_grammar_enum_name(g.name))
          else:
            result += '// only one extended grammar is used here\n'
            result += 'if (node->grammarId == ExtendedGrammar::{}) {{'.format(make_pretty_extened_grammar_enum_name(g.name))

          extOpCodeSwitch = len(extendedOps[g]) > 1
          if extOpCodeSwitch:
            result += 'switch (static_cast<{}>(node->extOpCode)) {{'.format(g.cpp_op_type_name)
            # silence the whining about not all handled cases for some compilers
            result += 'default: break;\n'

          for i in extendedOps[g]:
            if extOpCodeSwitch:
              result += 'case {}::{}:'.format(g.cpp_op_type_name, i.name)
            else:
              result += '// only one instruction for this grammar\n'
              result += 'if (static_cast<{0}>(node->extOpCode) == {0}::{1}) {{'.format(g.cpp_op_type_name, i.name)

            result += 'if (visitor(reinterpret_cast<{} *>(node))) return true;\n'.format(make_instruction_node_struct_name(i))

            if extOpCodeSwitch:
              result += 'break;\n'

          result += '}'

          if useGrammaSwitch:
            result += 'break;\n'

        result += '}'

        if useOpCodeSwitch:
          result += 'break;'

      result += '}\n'
      result += '}\n'

    if node.instructions:
      result += ' else {\n'

    needElse = False
    for c in sorted(node.childs, key = lambda n: n.name):
      if needElse:
        result += 'else '
      needElse = True
      result += 'if ({}::is(node))'.format(c.full_node_name())
      result += '{'
      result += 'if ({0}::visit(reinterpret_cast<{0} *>(node), visitor)) return true;\n'.format(c.full_node_name())
      result += '}\n'

    if node.childs:
      result += 'else  { return false; }'
    elif node.instructions:
      # if no instruction then we have to skip this and the visitor ends up just calling the visitor target
      result += 'return false;'

    if node.instructions:
      result += '}\n'

  # if we end up here no other visitor accepted the visit call
  result += 'return visitor(node);\n'
  result += '}\n'

  return result

def make_node_definitions_new(language):
  result  = ''
  # some structs are needed to be declared up front
  result += make_node_definitions_new_for_node(language.get_root_node())
  result += 'template<typename T, typename U> inline bool visitNode(T *node, U visitor) { return T::visit(node, visitor); }\n'
  result += 'template<typename T, typename U> inline bool visitNode(NodePointer<T> node, U visitor) { return T::visit(node.get(), visitor); }\n'
  result += make_node_is_implementation(language.get_root_node())
  return result

def generate_module_nodes(language, build_cfg):
  result  = '// Copyright (C) Gaijin Games KFT.  All rights reserved.\n\n'
  result += '// auto generated, do not modify!\n'
  result += '#pragma once\n'
  result += '#include "{}"\n'.format(build_cfg.get('generated-cpp-header-file-name'))

  result += 'namespace spirv {'
  # make node types
  result += '// some in memory node declarations\n'
  result += make_node_declarations_new(language)
  result += '// node properties\n'
  result += make_property_definition(language)
  result += '// in memory nodes\n'
  result += make_node_definitions_new(language)
  result += '// property visitor\n'
  result += make_property_visitor(language)
  result += '// property writes\n'
  result += make_property_writes(language)
  result += '// execution mode infos\n'
  result += make_execution_mode_infos(language)
  result += '}'

  return result

def main():
  import argparse
  parser = argparse.ArgumentParser(description = 'Generate SPIR-V meta data')

  parser.add_argument('--b',
                      metavar = '<path>',
                      type = str,
                      default = "build.json",
                      required = False,
                      help = 'Path to build info JSON file')

  args = parser.parse_args()

  with open(args.b) as build_file:
    build_json = json.load(build_file)

  spirv = spirv_meta_data_tools.Language()

  # load basic grammars
  grammarSet = build_json.get('grammars')
  for g in grammarSet:
    with open(grammarSet[g]) as grammar_file:
      spirv.load_grammar(g, json.load(grammar_file))

  # load instruction properties
  with open(build_json.get('instruction-properties')) as eg_file:
    eg_json = json.load(eg_file)
    spirv.load_extened_grammar(eg_json, lambda m: print(m))

  # load node declarations
  with open(build_json.get('node-declarations')) as n_file:
    n_json = json.load(n_file)
    spirv.load_node_def(n_json)

  # load spec op white list
  with open(build_json.get('spec-op-white-list')) as wlist_file:
    wlist_json = json.load(wlist_file)
    spirv.load_spec_const_op_white_list(wlist_json)

  # load module section info
  with open(build_json.get('module-sections-info')) as section_file:
    section_json = json.load(section_file)

  # link everything together
  spirv.finish_loading(lambda m: print(m))
  # build generated files
  print(generate_module_nodes(spirv, build_json), file = open(build_json.get('module-node-file-name'), 'w'))

if __name__ == '__main__':
  main()