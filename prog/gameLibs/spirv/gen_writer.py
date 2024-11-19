#!/usr/bin/env python
"""
Generates SPIR-V Module Writer cpp file.
The writer is used to serialize a ModuleBuilder object into SPIR-V binary format.
"""

from __future__ import print_function

from spirv_meta_data_tools import to_name

import errno
import json
import os.path
import spirv_meta_data_tools
import re

class ModuleSection:
  def __init__(self, json):
    self.name = json.get('name')
    self.parent = json.get('parent', None)
    self.properties = set(json.get('properties', []))
    self.instructions = set(json.get('instructions', []))
    self.children = []

  def resolve(self, lst):
    if self.parent:
      for e in lst:
        if e.name == self.parent:
          self.parent = e
          self.parent.children.append(self)
          break
      else:
        raise Exception('unable to find parent with name {}'.format(self.parent))

def compile_module_sections(module_sections_json):
  sections = module_sections_json.get('module-sections', [])
  allNodes = [ModuleSection(e) for e in sections]

  rootNode = None
  for m in allNodes:
    m.resolve(allNodes)
    if not m.parent:
      rootNode = m

  return rootNode

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

def genarete_extended_params_value_names(tipe, allow_short_cut = False, handle_grad = False):
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

def generate_module_opcode_writer(language, instruction):
  result  = 'bool operator()({} *value)'.format(make_instruction_node_struct_name(instruction))
  result += '{\n'
  if 'spec-op' in instruction.properties:
    result += 'auto anchor = writer.beginSpecConstantOpMode();\n'
    result += 'writer.beginInstruction(Op::OpSpecConstantOp, 2);\n'
    result += 'writer.writeWord(value->resultType->resultId);\n'
    result += 'writer.writeWord(value->resultId);\n'
    result += 'writer.endWrite();\n'
    result += 'visitNode(value->specOp, *this);\n'
    result += 'writer.endSpecConstantOpMode(anchor);\n'

  elif 'switch-section' in instruction.properties:
    # TODO: hardcoded!
    # TODO: does not handle 64 bit correctly!
    result += 'writer.beginInstruction(Op::OpSwitch, 2 + 2 * value->cases.size());\n'
    result += 'writer.writeWord(value->value->resultId);\n'
    result += 'writer.writeWord(value->defaultBlock->resultId);\n'
    result += 'for(auto &&cse: value->cases) {\n'
    result += 'writer.writeWord(static_cast<unsigned>(cse.value.value));\n'
    result += 'writer.writeWord(cse.block->resultId);\n'
    result += '}\n'
    result += 'writer.endWrite();\n'

  elif 'phi' in instruction.properties:
    # TODO: hardcoded!
    result += 'writer.beginInstruction(Op::OpPhi, 2 + value->sources.size() * 2);\n'
    result += 'writer.writeWord(value->resultType->resultId);\n'
    result += 'writer.writeWord(value->resultId);\n'
    result += 'for(auto && source : value->sources) {\n'
    result += 'writer.writeWord(source.source->resultId);\n'
    result += 'writer.writeWord(source.block->resultId);\n'
    result += '}\n'
    result += 'writer.endWrite();\n'

  else:
    skipForNow = False
    skipForNowCause = 'Extended Grammar'
    staticSize = 0
    pIndex = 0
    postLenCode = ''
    if not instruction.grammar.is_core():
      # grammar id
      staticSize += 2

    # TODO: refactor this, contains duplicated code!
    for p in instruction.enumerate_all_params():
      # bad hack to fix up some bad stuff
      basicParamName = make_node_param_name(p, pIndex)
      if 'function-call' in instruction.properties and basicParamName == 'param3':
        basicParamName = 'parameters'

      if p.is_optional():
        if p.type.is_any_input_ref():
          postLenCode += 'len += value->{} ? 1 : 0;\n'.format(basicParamName)
        elif p.type.category == 'BitEnum' or p.type.category == 'ValueEnum':
          if p.type.extended_params:
            postLenCode += 'if (value->{})'.format(basicParamName)
            postLenCode += '{\n'
            postLenCode += '++len;\n'
            postLenCode += 'auto compare = *value->{};\n'.format(basicParamName)
            for v in p.type.values:
              if p.type.category == 'BitEnum':
                # never check 0
                if v.value == 0:
                  continue
                postLenCode += 'if ({0}::{1} == (compare & {0}::{1}))'.format(p.type.get_type_name(), v.name)
                postLenCode += '{\n'
                postLenCode += 'compare = compare ^ {}::{};\n'.format(p.type.get_type_name(), v.name)
              else:
                if not v.parameters:
                  # only write if they have anything that could impact the size
                  continue
                postLenCode += 'if ({0}::{1} == compare)'.format(p.type.get_type_name(), v.name)
                postLenCode += '{\n'
              for vp in v.parameters:
                if vp.type.is_any_input_ref():
                  postLenCode += '++len;\n'
                elif vp.type.name == 'LiteralInteger':
                  # should never be larger than u32
                  postLenCode += '++len;\n'
                else:
                  skipForNow = True
                  skipForNowCause = 'not handled optional for {} {} {} because ext param was not a input ref'.format(p.type.name, p.type.category, p.type.extended_params)
              postLenCode += '}\n'
            postLenCode += '}\n'
          else:
            postLenCode += 'len += value->{} ? 1 : 0;\n'.format(basicParamName)
        elif p.type.category == 'ValueEnum':
          if p.type.extended_params:
            skipForNow = True
            skipForNowCause = 'not handled optional for {} {} {}'.format(p.type.name, p.type.category, p.type.extended_params)
          else:
            postLenCode += 'len += value->{} ? 1 : 0;\n'.format(basicParamName)
        else:
          skipForNow = True
          skipForNowCause = 'not handled optional for {} {} {}'.format(p.type.name, p.type.category, p.type.extended_params)
      elif p.is_variadic():
        if p.type.is_any_input_ref():
          postLenCode += 'len += value->{}.size();\n'.format(basicParamName)
        elif p.type.name == 'LiteralInteger':
          postLenCode += 'len += value->{}.size();\n'.format(basicParamName)
        else:
          skipForNow = True
          skipForNowCause = 'not handled variadic for {} {} {}'.format(p.type.name, p.type.category, p.type.extended_params)
      elif p.type.name == 'LiteralString':
        postLenCode += 'len += (value->{}.size() + sizeof(unsigned)) / sizeof(unsigned);\n'.format(basicParamName)
      elif 'ContextDependent' in p.type.name:
        postLenCode += 'len += builder.getTypeSizeBits(value->resultType) > 32 ? 2 : 1;\n'
      elif p.type.extended_params:
        staticSize += 1;
        postLenCode += 'auto compare = value->{};\n'.format(basicParamName)
        for v in p.type.values:
          if p.type.category == 'BitEnum':
                # never check 0
            if v.value == 0:
              continue;
            postLenCode += 'if ({0}::{1} == (compare & {0}::{1}))'.format(p.type.get_type_name(), v.name)
            postLenCode += '{\n'
            postLenCode += 'compare = compare ^ {}::{};\n'.format(p.type.get_type_name(), v.name)
          else:
            if not v.parameters:
              # only write if they have anything that could impact the size
              continue
            postLenCode += 'if ({0}::{1} == compare)'.format(p.type.get_type_name(), v.name)
            postLenCode += '{\n'

          for vp in v.parameters:
            if vp.type.is_any_input_ref():
              postLenCode += '++len;\n'
            elif vp.type.name == 'LiteralInteger':
              # should never be larger than u32
              postLenCode += '++len;\n'
            else:
              skipForNow = True
              skipForNowCause = 'not handled optional for {} {} {} because ext param was not a input ref'.format(p.type.name, p.type.category, p.type.extended_params)
          postLenCode += '}\n'
      elif p.type.name == 'LiteralSpecConstantOpInteger':
        skipForNow = True
        skipForNowCause = 'not handled LiteralSpecConstantOpInteger'
      else:
        staticSize += 1
      pIndex += 1

    if skipForNow:
      result += '// skipping for now -> {}\n'.format(skipForNowCause)
    else:
      if len(postLenCode) > 0:
        result += 'size_t len = {};\n'.format(staticSize)
        result += postLenCode
        staticSize = 'len'

      if instruction.grammar.is_core():
        opcodeName = instruction.name
      else:
        opcodeName = 'OpExtInst'

      result += 'writer.beginInstruction(Op::{}, {});\n'.format(opcodeName, staticSize)
      pIndex = 0
      for p in instruction.params:
        # bad hack to fix up some bad stuff
        baseName = make_node_param_name(p, pIndex)
        if 'function-call' in instruction.properties and baseName == 'param3':
          baseName = 'parameters'

        pName = 'value->{}'.format(baseName)
        if p.is_variadic():
          result += 'for (auto && v : {})'.format(pName)
          result += '{\n'
          pName = 'v'
        elif p.is_optional():
          result += 'if ({})'.format(pName)
          result += '{\n'
          pName = '(*{})'.format(pName)


        if p.type.is_result_type():
          pName = 'value->resultType->resultId'
        elif p.type.is_result_id():
          pName = 'value->resultId'
        elif p.type.is_any_input_ref():
          pName = '{}->resultId'.format(pName)
        elif p.type.name == 'LiteralInteger':
          pName = '{}.value'.format(pName)
        elif p.type.category == 'ValueEnum' or p.type.category == 'BitEnum':
          pBaseName = pName
          pName = 'static_cast<unsigned>({})'.format(pName)

        if p.type.name == 'LiteralString':
          result += 'writer.writeString({}.c_str());\n'.format(pName)
        elif 'ContextDependent' in p.type.name:
          result += 'writer.writeWord(static_cast<unsigned>({}.value));\n'.format(pName)
          result += 'if (builder.getTypeSizeBits(value->resultType) > 32)'
          result += 'writer.writeWord(static_cast<unsigned>({}.value >> 32u));\n'.format(pName)
        else:
          result += 'writer.writeWord({});\n'.format(pName)

          if (p.type.category == 'ValueEnum' or p.type.category == 'BitEnum') and p.type.extended_params:
            extParamNames = genarete_extended_params_value_names(p.type, True, True)
            result += 'auto compareWrite = {};\n'.format(pBaseName)
            for v in p.type.values:
              if p.type.category == 'BitEnum':
                if v.value == 0:
                  continue
                result += 'if ({0}::{1} == (compareWrite & {0}::{1}))'.format(p.type.get_type_name(), v.name)
                result += '{\n'
                result += 'compareWrite = compareWrite ^ {}::{};\n'.format(p.type.get_type_name(), v.name)
              else:
                if not v.parameters:
                  continue
                result += 'if ({0}::{1} == compareWrite)'.format(p.type.get_type_name(), v.name)
                result += '{\n'

              for vp, n in zip(v.parameters, extParamNames[v.name]):
                if n == to_name(v.name):
                  vpName = '{}{}{}'.format(baseName, n[0].upper(), n[1:])
                else:
                  vpName = '{}{}{}{}'.format(baseName, v.name, n[0].upper(), n[1:])
                if vp.type.is_any_input_ref():
                  result += 'writer.writeWord(value->{}->resultId);\n'.format(vpName)
                elif vp.type.name == 'LiteralInteger':

                  result += 'writer.writeWord(static_cast<unsigned>(value->{}.value));\n'.format(vpName)
                else:
                  pass

              result += '}\n'

        if p.is_variadic() or p.is_optional():
          result += '}\n'

        if p.type.is_result_id() and not instruction.grammar.is_core():
          result += 'writer.writeWord(builder.getExtendedGrammarIdRef(value->grammarId));\n'
          result += 'writer.writeWord(static_cast<unsigned>(value->extOpCode));\n'
        pIndex += 1
      result += 'writer.endWrite();\n'

  result += 'return true;\n'
  result += '}\n'
  return result

def make_encoder_for_node(language, node):
  result = ''
  if 'execution-mode' in node.properties:
    executionModelType = language.get_type_by_name('ExecutionMode')
    result += 'struct ExecutionModeWriteVisitor {\n'
    result += 'WordWriter &target;\n'
    result += 'NodePointer<NodeOpFunction> function;\n'
    for e in executionModelType.values:
      result += 'void operator()(const {}{} *value)'.format(executionModelType.get_type_name(), e.name)
      result += '{\n'
      if e.parameters:
        # for id params this is a different instruction...
        for p in e.parameters:
          if p.type.category == 'Id':
            opCode = 'OpExecutionModeId'
            break
        else:
          opCode = 'OpExecutionMode'
        # TODO: this size calc is incorrect, the correct way would be to add up each element by its size calc
        # even literal integer could be greater as 32 bit int (but currently not possible)
        result += 'target.beginInstruction(Op::{}, {});\n'.format(opCode, 2 + len(e.parameters))
        result += 'target.writeWord(function->resultId);\n'
        result += 'target.writeWord(static_cast<Id>({}::{}));\n'.format(executionModelType.get_type_name(), e.name)
        pIndex = 0
        for p in e.parameters:
          pName = make_node_param_name(p, pIndex)
          if p.type.category == 'Id':
            result += 'target.writeWord(value->{}->resultId);\n'.format(pName)
          elif p.type.category == 'ValueEnum':
            # usual word should be ok here
            result += 'target.writeWord((uint32_t)(value->{}));\n'.format(pName)
          else:
            result += 'target.writeWord(static_cast<Id>(value->{}.value));\n'.format(pName)
          pIndex += 1
      else:
        result += 'target.beginInstruction(Op::OpExecutionMode, 2);\n'
        result += 'target.writeWord(function->resultId);\n'
        # just save one memory load by just use the const
        result += 'target.writeWord(static_cast<Id>({}::{}));\n'.format(executionModelType.get_type_name(), e.name)
      result += 'target.endWrite();\n'
      result += '}\n'

    result += '};\n'

  if 'decoration' in node.properties:
    propertyType = language.get_type_by_name('Decoration')
    result += 'struct DecorationWriteVisitor {\n'
    result += 'WordWriter &target;\n'
    result += 'NodePointer<NodeId> node;\n'
    result += '// special property that is handled by the name writer\n'
    result += 'void operator()(const PropertyName *) {}\n'
    result += '// property is used to determine if a pointer type needs a forward declaration\n'
    result += 'void operator()(const PropertyForwardDeclaration *) {}\n'
    result += '// property used to place OpSelectionMerge at the right location\n'
    result += 'void operator()(const PropertySelectionMerge *){}\n'
    result += '// property used to place OpLoopMerge ar the right location\n'
    result += 'void operator()(const PorpertyLoopMerge *) {}\n'
    for v in propertyType.enumerate_unique_values():
      result += 'void operator()(const Property{} *value) {{'.format(v.name)
      # figure out which opcodes are needed
      for p in v.parameters:
        if p.type.category == 'Id':
          directOp = 'OpDecorateId'
          # has no member op
          memberOp = ''
          variableLength = False
          break
        elif p.type.name == 'LiteralString':
          # why did they add this, normal op decorate had already things that took strings
          if len(v.parameters) == 1:
            directOp = 'OpDecorateStringGOOGLE'
            memberOp = 'OpMemberDecorateStringGOOGLE'
          else:
            directOp = 'OpDecorate'
            # has no member op
            memberOp = 'OpMemberDecorate'
          variableLength = True
          break
      else:
          directOp = 'OpDecorate'
          # has no member op
          memberOp = 'OpMemberDecorate'
          variableLength = False

      if v.parameters:
        if variableLength:
          lengthVar = 'length'
          result += 'size_t length = 0;\n'

          pIndex = 0
          for p in v.parameters:
            if p.type.name == 'LiteralString':
              pName = make_node_param_name(p, pIndex)
              result += 'length += (value->{}.length() + sizeof(unsigned)) / sizeof(unsigned);'.format(pName)
            else:
              result += '++length;\n'
            pIndex += 1
        else:
          lengthVar = '{}'.format(len(v.parameters))


        pIndex = 0
        paramWriteSection = ''
        for p in v.parameters:
          pName = make_node_param_name(p, pIndex)
          if p.type.name == 'LiteralString':
            paramWriteSection += 'target.writeString(value->{}.c_str());\n'.format(pName)
          elif p.type.category == 'Id':
            paramWriteSection += 'target.writeWord(value->{}->resultId);\n'.format(pName)
          elif p.type.category == 'Literal':
            paramWriteSection += 'target.writeWord(static_cast<Id>(value->{}.value));\n'.format(pName)
          else:
            paramWriteSection += 'target.writeWord(static_cast<Id>(value->{}));\n'.format(pName)

          pIndex += 1

        if memberOp != '':
          result += 'if (value->memberIndex) {\n'
          result += 'target.beginInstruction(Op::{}, 3 + {});\n'.format(memberOp, lengthVar)
          result += 'target.writeWord(node->resultId);\n'
          result += 'target.writeWord(*value->memberIndex);\n'
          result += 'target.writeWord(static_cast<Id>({}::{}));\n'.format(propertyType.get_type_name(), v.name)
          result += paramWriteSection
          result += '} else {\n'

        result += 'target.beginInstruction(Op::{}, 2 + {});\n'.format(directOp, lengthVar)
        result += 'target.writeWord(node->resultId);\n'
        result += 'target.writeWord(static_cast<Id>({}::{}));\n'.format(propertyType.get_type_name(), v.name)
        result += paramWriteSection

        if memberOp != '':
          result += '}\n'

      else:
        if memberOp != '':
          result += 'if (value->memberIndex) {\n'
          result += 'target.beginInstruction(Op::{}, 3);\n'.format(memberOp)
          result += 'target.writeWord(node->resultId);\n'
          result += 'target.writeWord(*value->memberIndex);\n'
          result += 'target.writeWord(static_cast<Id>({}::{}));\n'.format(propertyType.get_type_name(), v.name)

          result += '} else {\n'

        result += 'target.beginInstruction(Op::{}, 2);\n'.format(directOp)
        result += 'target.writeWord(node->resultId);\n'
        result += 'target.writeWord(static_cast<Id>({}::{}));\n'.format(propertyType.get_type_name(), v.name)

        if memberOp != '':
          result += '}\n'

      result += 'target.endWrite();\n'
      result += '}\n'

    result += '};\n'

  # some types need (spec-)constants to work properly (size of arrays)
  typeDefStuff = set(['typedef', 'constant', 'sepc-constant'])
  if typeDefStuff == typeDefStuff & node.properties:
    # we need a visitor that counts the refs that are not defined yet
    pass


  result += 'void encode{}(ModuleBuilder &module, WordWriter &writer)'.format(node.name)
  result += '{\n'
  if 'header-section' in node.properties:
    result += 'writer.beginHeader();\n'
    result += 'writer.writeWord(MagicNumber);\n'
    result += 'writer.writeWord(module.getModuleVersion());\n'
    result += 'writer.writeWord(module.getToolIdentifcation());\n'
    result += 'writer.writeWord(module.getIdBounds());\n'
    result += '// one reserved slot that has to be 0\n'
    result += 'writer.writeWord(0);\n'
    result += 'writer.endWrite();\n'

  if 'capabilities' in node.properties:
    result += 'for (auto && cap : module.getEnabledCapabilities())'
    result += '{\n'
    result += 'writer.beginInstruction(Op::OpCapability, 1);\n'
    result += 'writer.writeWord(static_cast<unsigned>(cap));\n'
    result += 'writer.endWrite();\n'
    result += '}\n'

  if 'extension-enable' in node.properties:
    result += 'module.enumerateEnabledExtensions([&writer](auto , const char *name, size_t name_len) //\n'
    result += '{\n'
    result += 'writer.beginInstruction(Op::OpExtension, (name_len + sizeof(unsigned)) / sizeof(unsigned));\n'
    result += 'writer.writeString(name);\n'
    result += 'writer.endWrite();\n'
    result += '});\n'

  if 'extended-grammar-load' in node.properties:
    result += 'module.enumerateLoadedExtendedGrammars([&writer](Id id, auto , const char *name, size_t name_len) //\n'
    result += '{\n'
    result += 'writer.beginInstruction(Op::OpExtInstImport, 1 + (name_len + sizeof(unsigned)) / sizeof(unsigned));\n'
    result += 'writer.writeWord(id);\n'
    result += 'writer.writeString(name);\n'
    result += 'writer.endWrite();\n'
    result += '});\n'

  if 'set-memory-model' in node.properties:
    result += 'writer.beginInstruction(Op::OpMemoryModel, 2);\n'
    result += 'writer.writeWord(static_cast<unsigned>(module.getAddressingModel()));\n'
    result += 'writer.writeWord(static_cast<unsigned>(module.getMemoryModel()));\n'
    result += 'writer.endWrite();\n'

  if 'entry-point' in node.properties:
    result += 'module.enumerateEntryPoints([&writer](auto func, auto e_model, const auto &name, const auto &interface_list) //\n'
    result += '{\n'
    result += 'writer.beginInstruction(Op::OpEntryPoint, 2 + interface_list.size() + ((name.length() + sizeof(unsigned)) / sizeof(unsigned)));\n'
    result += 'writer.writeWord(static_cast<unsigned>(e_model));\n'
    result += 'writer.writeWord(func->resultId);\n'
    result += 'writer.writeString(name.c_str());\n'
    result += 'for (auto && i : interface_list)\n'
    result += 'writer.writeWord(i->resultId);\n'
    result += 'writer.endWrite();\n'
    result += '});\n'

  if 'execution-mode' in node.properties:
    result += 'module.enumerateExecutionModes([&writer](auto func, auto em) //\n {'
    result += 'executionModeVisitor(em, ExecutionModeWriteVisitor{writer, func});'
    result += '});\n'

  if 'string' in node.properties:
    result += 'Id emptyStringId = 0;\n'
    result += 'module.enumerateNodeType<NodeOpString>([&writer, &emptyStringId](auto sn) //\n{'
    result += 'if (sn->string.empty()) emptyStringId = sn->resultId;\n'
    result += 'writer.beginInstruction(Op::OpString, 1 + (sn->string.length() + sizeof(unsigned)) / sizeof(unsigned));\n'
    result += 'writer.writeWord(sn->resultId);\n'
    result += 'writer.writeString(sn->string.c_str());\n'
    result += 'writer.endWrite();\n'
    result += '});\n'
    result += 'if (!module.getSourceString().empty() && module.getSourceFile().value == 0 && 0 == emptyStringId){\n'
    result += '// allocate empty string if needed\n'
    result += 'emptyStringId = module.allocateId();\n'
    result += 'writer.beginInstruction(Op::OpString, 2);\n'
    result += 'writer.writeWord(emptyStringId);\n'
    result += 'writer.writeWord(0);\n'
    result += 'writer.endWrite();\n'
    result += '}\n'

  if 'source-extension' in node.properties:
    result += 'module.enumerateSourceExtensions([&writer](const eastl::string &str) //\n{'
    result += 'writer.beginInstruction(Op::OpSourceExtension, (str.length() + sizeof(unsigned)) / sizeof(unsigned));\n'
    result += 'writer.writeString(str.c_str());\n'
    result += 'writer.endWrite();\n'
    result += '});\n'

  if 'source' in node.properties:
    result += 'if (module.getSourceLanguage() != SourceLanguage::Unknown) {'
    result += 'auto sourceStringLength = module.getSourceString().length();\n'
    result += 'bool writeSourceString = 0 != sourceStringLength;\n'
    result += 'auto sourceFileNameRef = module.getSourceFile();\n'
    result += 'bool writeSourceFile = sourceFileNameRef.value != 0;\n'
    result += 'if (writeSourceString && !writeSourceFile) {\n'
    result += 'sourceFileNameRef.value = emptyStringId;\n'
    result += 'writeSourceFile = true;\n'
    result += '}\n'
    result += '// lang id + lang ver\n'
    result += 'size_t sourceInfoLength = 2;\n'
    result += 'if (writeSourceFile) ++sourceInfoLength;\n'
    result += 'if (writeSourceString) sourceInfoLength += writer.calculateStringWords(sourceStringLength);\n'
    result += 'writer.beginInstruction(Op::OpSource, sourceInfoLength);\n'
    result += 'writer.writeWord(static_cast<unsigned>(module.getSourceLanguage()));\n'
    result += 'writer.writeWord(module.getSourceLanguageVersion());\n'
    result += 'if (writeSourceFile) writer.writeWord(sourceFileNameRef.value);\n'
    result += 'const char *sourceWritePtr = nullptr;'
    result += 'if (writeSourceString) sourceWritePtr = writer.writeString(module.getSourceString().c_str());\n'
    result += 'writer.endWrite();\n'
    result += '// if source code string is too long, we need to break it up into chunks of 0xffff char blocks\n'
    result += 'while (sourceWritePtr) {\n'
    result += 'auto dist = sourceWritePtr - module.getSourceString().c_str();\n'
    result += 'writer.beginInstruction(Op::OpSourceContinued, writer.calculateStringWords(sourceStringLength - dist));\n'
    result += 'sourceWritePtr = writer.writeString(sourceWritePtr);\n'
    result += 'writer.endWrite();\n'
    result += '}\n'
    result += '}\n'


  if 'name' in node.properties:
    result += 'module.enumerateAllInstructionsWithProperty<PropertyName>([&writer](auto node, auto property)//\n'
    result += '{ property->write(node, writer);'
    result += '});\n'

  if 'module-processed' in node.properties:
    result += 'module.enumerateModuleProcessed([&writer](const char *str, size_t len)//\n'
    result += '{\n'
    result += 'writer.beginInstruction(Op::OpModuleProcessed, (len + sizeof(unsigned)) / sizeof(unsigned));\n'
    result += 'writer.writeString(str);\n'
    result += 'writer.endWrite();\n'
    result += '});\n'

  if 'decoration' in node.properties:
    result += 'module.enumerateAllInstructionsWithAnyProperty([&writer](auto node) //\n {'
    result += 'for (auto && property : node->properties)'
    result += 'visitProperty(property, DecorationWriteVisitor{writer, node});\n'
    result += '});\n'

  if typeDefStuff == typeDefStuff & node.properties:
    result += 'bool reTry = false;\n'
    result += 'FlatSet<Id> writtenTypeIds;\n'
    result += 'FlatSet<Id> writtenForwardIds;\n'
    result += 'FlatSet<Id> notWrittenTypeIds;\n'
    result += 'FlatSet<Id> notWrittenConstantIds;\n'
    result += 'FlatSet<Id> writtenConstantIds;\n'
    result += '// first get the forward declarations done and gather all type ids\n'
    result += 'module.enumerateAllTypes([&writer, &writtenForwardIds, &notWrittenTypeIds](auto node) //\n {'
    result += 'notWrittenTypeIds.insert(node->resultId);\n'
    result += 'for (auto && p : node->properties) {\n'
    result += 'if (is<PropertyForwardDeclaration>(p)) {\n'
    result += ' as<PropertyForwardDeclaration>(p)->write(node, writer);\n'
    result += ' writtenForwardIds.insert(node->resultId);\n'
    result += '}\n'
    result += '}\n'
    result += '});\n'
    result += '// second get all constants that we need to write\n'
    result += 'module.enumerateAllConstants([&notWrittenConstantIds](auto node) //\n { notWrittenConstantIds.insert(node->resultId); });\n'
    result += 'while(!notWrittenTypeIds.empty() || !notWrittenConstantIds.empty()){'
    result += 'bool keepWriting = false;\n'
    result += 'do {\n'
    result += 'keepWriting = false;\n'
    result += 'for (auto &&constId : notWrittenConstantIds) {'
    result += 'auto constant = as<NodeOperation>(module.getNode(constId));\n'
    result += 'if (writtenTypeIds.has(constant->resultType->resultId)) {'
    result += 'visitNode(constant, NodeWriteVisitor{writer, module});\n'
    result += 'writtenConstantIds.insert(constId);\n'
    result += 'notWrittenConstantIds.remove(constId);\n'
    result += '// iterators are invalid, have to go another round for the next constants\n'
    result += 'keepWriting = true;\n'
    result += 'break;\n'
    result += '}\n'
    result += '}\n'
    result += '}\n'
    result += 'while(keepWriting);'
    result += 'do {\n'
    result += 'keepWriting = false;\n'
    result += 'for (auto &&typeId : notWrittenTypeIds) {'
    result += '// try write type\n'
    result += 'auto typeDef = as<NodeTypedef>(module.getNode(typeId));\n'
    result += 'bool canWrite = true;'
    result += 'visitNode(typeDef, CanWriteTypeCheckVisitor//\n{writtenTypeIds, writtenForwardIds, writtenConstantIds, canWrite});'
    result += 'if (canWrite) {'
    result += 'visitNode(typeDef, NodeWriteVisitor{writer, module});\n'
    result += 'writtenForwardIds.remove(typeId);\n'
    result += 'writtenTypeIds.insert(typeId);\n'
    result += 'notWrittenTypeIds.remove(typeId);\n'
    result += '// iterators are invalid, have to go another round for the next type\n'
    result += 'keepWriting = true;\n'
    result += 'break;\n'
    result += '}\n'
    result += '}\n'
    result += '}\n'
    result += 'while(keepWriting);'
    result += '}\n'

  if 'undef' in node.properties:
    result += '// global undef stuff\n'
    result += 'module.enumerateAllGlobalUndefs([&writer, &module](auto node)//\n {'
    result += 'NodeWriteVisitor{writer, module}(node.get());\n'
    result += '});\n'

  if 'variable' in node.properties:
    result += '// variable stuff\n'
    result += 'module.enumerateAllGlobalVariables([&writer, &module](auto node)//\n {'
    result += 'NodeWriteVisitor{writer, module}(as<NodeOpVariable>(node).get());\n'
    result += '});\n'

  if 'function-declaration' in node.properties:
    result += 'module.enumerateAllFunctions([&module, &writer](auto node) //\n{'
    result += '// only write functions without body here\n'
    result += 'if (!node->body.empty()) return;\n'
    result += 'NodeWriteVisitor visitor{writer, module};\n'
    result += 'auto opCodeNode = as<NodeOpFunction>(node);\n'
    result += 'visitor(opCodeNode.get());\n'
    result += 'for (auto && param : opCodeNode->parameters)\n'
    result += '  visitor(as<NodeOpFunctionParameter>(param).get());\n'
    result += 'writer.beginInstruction(Op::OpFunctionEnd, 0)\n;'
    result += ' writer.endWrite();\n'
    result += '});\n'

  if 'function-definition' in node.properties:
    result += 'module.enumerateAllFunctions([&module, &writer](auto node) //\n{'
    result += '// only write functions with body here\n'
    result += 'if (node->body.empty()) return;\n'
    result += 'NodeWriteVisitor visitor{writer, module};\n'
    result += 'auto opCodeNode = as<NodeOpFunction>(node);\n'
    result += 'visitor(opCodeNode.get());\n'
    result += 'for (auto && param : opCodeNode->parameters)\n'
    result += '  visitor(as<NodeOpFunctionParameter>(param).get());\n'
    result += 'for (auto && block : opCodeNode->body)\n'
    result += '{\n'
    result += '  visitor(as<NodeOpLabel>(block).get());\n'
    result += '  for (auto && inst : as<NodeOpLabel>(block)->instructions)\n'
    result += '    visitNode(inst, visitor);\n'
    result += '  for (auto && prop : as<NodeOpLabel>(block)->properties)\n'
    result += '     visitProperty(prop.get(), BranchPropertyWriter{writer});\n'
    result += '  visitNode(block->exit, visitor);\n'
    result += '}\n'
    result += 'writer.beginInstruction(Op::OpFunctionEnd, 0)\n;'
    result += ' writer.endWrite();\n'
    result += '});\n'

  result += '}\n'

  for c in node.children:
    result += make_encoder_for_node(language, c)

  return result

def make_final_encoder_entry_for_node(node):
  result = 'encode{}(module, writer);\n'.format(node.name)
  for c in node.children:
    result += make_final_encoder_entry_for_node(c)

  return result;

def generate_module_opcodes_writer(language):
  result  = 'struct NodeWriteVisitor {\n'
  result += 'WordWriter &writer;\n'
  result += 'ModuleBuilder &builder;\n'
  for i in language.enumerate_all_instructions():
    if i.node:
      result += generate_module_opcode_writer(language, i)

  result += '// generic nodes do nothing, should never be called anyways\n'
  for n in language.get_nodes():
    result += 'bool operator()({}* value) {{ return false; }}\n'.format(n.full_node_name())

  result += '};\n'
  return result

def generate_module_writer(language, cfg, module_sections_json):
  result  = '// Copyright (C) Gaijin Games KFT.  All rights reserved.\n\n'
  result += '// auto generated, do not modify!\n'
  result += '#include "{}"\n'.format(cfg.get('module-node-file-name'))
  result += '#include <spirv/module_builder.h>\n'
  result += 'using namespace spirv;\n'

  sectionRoot = compile_module_sections(module_sections_json)

  # FIXME: hardcoded!
  result += 'struct BranchPropertyWriter {\n'
  result += 'WordWriter &writer;\n'
  result += 'void operator()(PorpertyLoopMerge *m) {\n'
  result += 'writer.beginInstruction(Op::OpLoopMerge, 3 + (((m->controlMask & LoopControlMask::DependencyLength) == LoopControlMask::DependencyLength) ? 1 : 0));\n'
  result += 'writer.writeWord(m->mergeBlock->resultId);\n'
  result += 'writer.writeWord(m->continueBlock->resultId);\n'
  result += 'writer.writeWord(static_cast<unsigned>(m->controlMask));\n'
  result += 'if ((m->controlMask & LoopControlMask::DependencyLength) == LoopControlMask::DependencyLength)\n'
  result += 'writer.writeWord(static_cast<unsigned>(m->dependencyLength.value));\n'
  result += 'writer.endWrite();\n'
  result += '}\n'
  result += 'void operator()(PropertySelectionMerge *m) {\n'
  result += 'writer.beginInstruction(Op::OpSelectionMerge, 2);\n'
  result += 'writer.writeWord(m->mergeBlock->resultId);\n'
  result += 'writer.writeWord(static_cast<unsigned>(m->controlMask));\n'
  result += 'writer.endWrite();\n'
  result += '}\n'
  result += 'template<typename T> void operator()(T *) {}\n'
  result += '};\n'

  result += generate_module_opcodes_writer(language)

  result += 'struct CanWriteTypeCheckVisitor {\n'
  result += 'FlatSet<Id> &writtenTypes;\n'
  result += 'FlatSet<Id> &forwardTypes;\n'
  result += 'FlatSet<Id> &writtenConstants;\n'
  result += 'bool &canWrite;\n'
  # TODO: types have to check if all their base types and constants they depend on are written
  result += 'template<typename T> bool operator()(T *value) { canWrite = true; value->visitRefs([&](auto node)//\n {if (writtenTypes.has(node->resultId)) return;if(forwardTypes.has(node->resultId)) return; if(writtenConstants.has(node->resultId)) return; canWrite = false;}); return true; }\n'
  result += '};\n'

  result += make_encoder_for_node(language, sectionRoot)

  result += 'eastl::vector<unsigned> spirv::write(ModuleBuilder &module, const WriteConfig &cfg, ErrorHandler &e_handler) {'
  result += 'WordWriter writer;\n'
  result += 'writer.errorHandler = &e_handler;\n'
  result += make_final_encoder_entry_for_node(sectionRoot)

  result += 'return eastl::move(writer.words);\n'
  result += '}\n'

  return result

def main():
  import argparse
  parser = argparse.ArgumentParser(description = 'SPIR-V Module Writer Generator')

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
  print(generate_module_writer(spirv, build_json, section_json), file = open(build_json.get('module-writer-file-name'), 'w'))

if __name__ == '__main__':
  main()
