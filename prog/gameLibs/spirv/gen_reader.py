#!/usr/bin/env python
"""
Generates SPIR-V Module Reader cpp file.
The reader is a translator from SPIR-V Module Decoder to ModuleBuilder.
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

def make_reader_result_type(param):
  if param.is_optional():
    fmt = 'Optional<{}>'
  elif param.is_variadic():
    fmt = 'Multiple<{}>'
  else:
    fmt = '{}'

  if param.type.extended_params:
    fmt = fmt.format('TypeTraits<{}>::ReadType')

  return fmt.format(param.type.get_type_name())

def make_param_name(param, index):
  if param.name != param.type.name and '+' not in param.name:
    return to_param_name(param.name)
  elif is_unique_param_type(param.type):
    return to_param_name(param.name)
  else:
    return 'param_{}'.format(index)

def make_instruction_node_struct_name(instruction):
  return '{}Op{}'.format(instruction.grammar.language.get_root_node().name, make_node_create_function_name(instruction)[3:])

def make_reader_result_type(param):
  if param.is_optional():
    fmt = 'Optional<{}>'
  elif param.is_variadic():
    fmt = 'Multiple<{}>'
  else:
    fmt = '{}'

  if param.type.extended_params:
    fmt = fmt.format('TypeTraits<{}>::ReadType')

  return fmt.format(param.type.get_type_name())

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

def generate_instruction_reader(language, instruction):
  result = ''

  if 'call-extended-grammar' in instruction.properties:
    result += 'ExtendedGrammar getExtendedGrammarIdentifierFromRefId(IdRef id) {'
    result += 'return moduleBuilder.getExtendedGrammarFromIdRef(id.value);\n'
    result += '}\n'

  elif 'load-extended-grammar' in instruction.properties:
    result += 'void loadExtendedGrammar(IdResult id, ExtendedGrammar egram, const char *name, size_t length) {'
    result += 'moduleBuilder.loadExtendedGrammar(id, egram, name, length);\n'
    result += '}\n'

  elif 'enable-extension' in instruction.properties:
    result += 'void enableExtension(Extension ext, const char *name, size_t length) {'
    result += 'moduleBuilder.enableExtension(ext, name, length);\n'
    result += '}\n'

  elif 'spec-op' in instruction.properties:
    coreGrammar = language.get_core_grammar()
    for inst in coreGrammar.instructions:
      if inst.is_const_op:
        result += 'void onSpec{}('.format(make_node_create_function_name(inst)[3:])
        paramNameList = ['op']
        paramList = ['Op op']
        if instruction.has_id():
          paramList.append('IdResult id')
          paramNameList.append('id')
        if instruction.has_type():
          paramList.append('IdResultType type')
          paramNameList.append('type')

        pIndex = 0
        for p in inst.enumerate_all_params():
          if not p.type.is_result_type() and not p.type.is_result_id():
            pName = make_param_name(p, pIndex)
            pType = make_reader_result_type(p)
            paramList.append('{} {}'.format(pType, pName))
            paramNameList.append(pName)
          pIndex += 1;
        result += ','.join(paramList)
        result += ') {\n'
        # TODO: hack to obtain current node
        result += '// do not add node to current block, use static block to grab the node\n'
        result += 'auto blockTemp = currentBlock;\n'
        result += 'currentBlock = &specConstGrabBlock;\n'
        result += 'on{}({});\n'.format(make_node_create_function_name(inst)[3:], ', '.join(paramNameList))
        # TODO: node type hardcoded
        result += 'auto specOp = as<NodeOperation>(specConstGrabBlock.instructions.back());\n'
        result += 'specConstGrabBlock.instructions.pop_back();\n'
        result += 'currentBlock = blockTemp;\n'
        result += 'auto node = moduleBuilder.newNode<{}>(specOp->resultId, specOp->resultType, specOp);'.format(make_instruction_node_struct_name(instruction))
        result += 'if (currentBlock) currentBlock->instructions.push_back(node);\n'
        result += '}\n'

  else:
    result += 'void on{}('.format(make_node_create_function_name(instruction)[3:])
    paramList = [e for e in instruction.enumerate_all_params()]

    paramNameList = ['']

    if instruction.grammar.is_core():
      paramTypeNameList = ['Op']
    else:
      paramTypeNameList = [instruction.grammar.cpp_op_type_name]

    pIndex = 0
    for p in paramList:
      paramNameList.append(make_param_name(p, pIndex))
      paramTypeNameList.append(make_reader_result_type(p))
      pIndex += 1;

    result += ', '.join(['{} {}'.format(t, n) for t, n in zip(paramTypeNameList, paramNameList)])
    result += '){\n'
    # specialized stuff first
    if 'selection-merge' in instruction.properties:
      # use current block as target
      result += 'SelectionMergeDef newSMerge;\n'
      result += 'newSMerge.target = currentBlock;\n'
      for t, n in zip(paramList, paramNameList[1:]):
        if t.type.is_any_input_ref():
          result += 'newSMerge.mergeBlock = {};\n'.format(n)
        elif t.type.name == 'SelectionControl':
          result += 'newSMerge.controlMask = {};\n'.format(n)
      result += 'selectionMerges.push_back(newSMerge);\n'

    elif 'loop-merge' in instruction.properties:
      # use current block as target
      result += 'LoopMergeDef newLMerge;\n'
      result += 'newLMerge.target = currentBlock;\n'
      for t, n in zip(paramList, paramNameList[1:]):
        if t.type.is_any_input_ref():
          if t.name == "'Merge Block'":
            result += 'newLMerge.mergeBlock = {};\n'.format(n)
          elif t.name == "'Continue Target'":
            result += 'newLMerge.continueBlock = {};\n'.format(n)
        elif t.type.name == 'LoopControl':
          # TODO: this is hardcoded, not so great!
          result += 'newLMerge.controlMask = {}.value;\n'.format(n)
          result += 'newLMerge.dependencyLength = {}.data.DependencyLength.first;\n'.format(n)
      result += 'loopMerges.push_back(newLMerge);\n'

    elif 'forward-typedef' in instruction.properties:
      result += '// forward def is translated into a property of the target op, so we can later write the correct instructions\n'
      result += '// no need to store the storage class, has to be the same as the target pointer\n'
      for t, n in zip(paramList, paramNameList[1:]):
        if t.type.is_any_input_ref():
          result += 'forwardProperties.push_back({});\n'.format(n)

    elif 'enable-capability' in instruction.properties:
      for t, n in zip(paramList, paramNameList[1:]):
        if t.type.name == 'Capability':
          result += 'moduleBuilder.enableCapability({});\n'.format(n)

    elif 'set-memory-model' in instruction.properties:
      for t, n in zip(paramList, paramNameList[1:]):
        if t.type.name == 'MemoryModel':
          result += 'moduleBuilder.setMemoryModel({});\n'.format(n)
        elif t.type.name == 'AddressingModel':
          result += 'moduleBuilder.setAddressingModel({});\n'.format(n)

    elif 'export-entry-point' in instruction.properties:
      result += 'EntryPointDef entryPoint;\n'
      for t, n in zip(paramList, paramNameList[1:]):
        if t.type.name == 'ExecutionModel':
          result += 'entryPoint.executionModel = {};\n'.format(n)
        elif t.type.name == 'LiteralString':
          result += 'entryPoint.name = {};\n'.format(n)
        elif t.type.category == 'Id':
          if t.is_variadic():
            result += 'entryPoint.interface = {};\n'.format(n)
          else:
            result += 'entryPoint.function = {};\n'.format(n)
      result += 'entryPoints.push_back(entryPoint);\n'

    elif 'function-begin' in instruction.properties:
      result += 'currentFunction = moduleBuilder.newNode<NodeOpFunction>(id_result.value, moduleBuilder.getType(id_result_type), function_control, moduleBuilder.getNode(function_type));\n'

    elif 'function-parameter' in instruction.properties:
      result += 'currentFunction->parameters.push_back(moduleBuilder.newNode<NodeOpFunctionParameter>(id_result.value, moduleBuilder.getType(id_result_type)));\n'

    elif 'function-end' in instruction.properties:
      result += 'currentFunction.reset();\n'

    elif 'block-begin' in instruction.properties:
      result += 'currentBlock = moduleBuilder.newNode<NodeOpLabel>(id_result.value);\n'
      result += 'currentFunction->body.push_back(currentBlock);\n'

    elif 'phi' in instruction.properties:
      callList = []
      for p, n in zip(paramList, paramNameList[1:]):
        if p.type.category == 'Composite':
          countName = '{}.count'.format(n)
          sourceRange = n
          callList.append('nullptr')
          callList.append('0')
        elif p.type.is_any_input_ref() and not p.is_variadic() and not p.is_optional():
          callList.append('moduleBuilder.getNode({})'.format(n))
        elif p.type.is_result_type():
          callList.append('moduleBuilder.getType({})'.format(n))
        elif p.type.is_result_id():
          callList.append('{}.value'.format(n))

      result += 'auto node = moduleBuilder.newNode<{}>({});\n'.format(make_instruction_node_struct_name(instruction), ', '.join(callList))
      result += 'node->sources.resize({});\n'.format(countName)
      result += 'for (int i = 0; {}; ++i)'.format(countName)
      result += '{\n'
      result += 'auto sourceInfo = {}.consume();\n'.format(sourceRange)
      result += 'auto &ref = node->sources[i];\n'
      result += 'PropertyReferenceResolveInfo info;\n'
      # hardcoded node name!
      result += 'info.target = reinterpret_cast<NodePointer<NodeId>*>(&ref.source);\n'
      result += 'info.ref = sourceInfo.first;\n'
      result += 'propertyReferenceResolves.push_back(info);\n'
      # hardcoded node name!
      result += 'info.target = reinterpret_cast<NodePointer<NodeId>*>(&ref.block);\n'
      result += 'info.ref = sourceInfo.second;\n'
      result += 'propertyReferenceResolves.push_back(info);\n'
      result += '}\n'
      result += 'if (currentBlock) currentBlock->instructions.push_back(node);\n'

    elif 'switch-section' in instruction.properties:
      callList = []
      for p, n in zip(paramList, paramNameList[1:]):
        if p.type.category == 'Composite':
          countName = '{}.count'.format(n)
          sourceRange = n
          callList.append('nullptr')
          callList.append('0')
        elif p.type.is_any_input_ref() and not p.is_variadic() and not p.is_optional():
          if n == 'selector':
            callList.append('moduleBuilder.getNode({})'.format(n))
          else:
            callList.append('NodePointer<NodeId>{}')
          defaultRef = n
        elif p.type.is_result_type():
          callList.append('moduleBuilder.getType({})'.format(n))
        elif p.type.is_result_id():
          callList.append('{}.value'.format(n))

      result += 'auto node = moduleBuilder.newNode<{}>({});\n'.format(make_instruction_node_struct_name(instruction), ', '.join(callList))
      result += 'PropertyReferenceResolveInfo info;\n'
      result += 'info.target = reinterpret_cast<NodePointer<NodeId>*>(&node->defaultBlock);\n'
      result += 'info.ref = {};\n'.format(defaultRef)
      result += 'propertyReferenceResolves.push_back(info);\n'
      result += 'node->cases.resize({});\n'.format(countName)
      result += 'for (int i = 0; {}; ++i)'.format(countName)
      result += '{\n'
      result += 'auto sourceInfo = {}.consume();\n'.format(sourceRange)
      result += 'auto &ref = node->cases[i];\n'
      result += 'ref.value = sourceInfo.first;\n'
      # hardcoded node name!
      result += 'info.target = reinterpret_cast<NodePointer<NodeId>*>(&ref.block);\n'
      result += 'info.ref = sourceInfo.second;\n'
      result += 'propertyReferenceResolves.push_back(info);\n'
      result += '}\n'
      result += 'currentBlock->exit = node;\n'
      result += 'currentBlock.reset();\n'

    elif 'branch' in instruction.properties:
      callList = []
      for p, n in zip(paramList, paramNameList[1:]):
        if p.type.is_any_input_ref():
          callList.append('NodePointer<NodeId>{}')
          targetRef = n

      result += 'auto node = moduleBuilder.newNode<{}>({});\n'.format(make_instruction_node_struct_name(instruction), ', '.join(callList))
      result += 'PropertyReferenceResolveInfo info;\n'
      result += 'info.target = reinterpret_cast<NodePointer<NodeId>*>(&node->targetLabel);\n'
      result += 'info.ref = {};\n'.format(targetRef)
      result += 'propertyReferenceResolves.push_back(info);\n'
      result += 'currentBlock->exit = node;\n'
      result += 'currentBlock.reset();\n'

    elif 'branch-conditional' in instruction.properties:
      callList = []
      refCount = 0
      for p, n in zip(paramList, paramNameList[1:]):
        if p.type.is_any_input_ref():
          if refCount == 0:
            callList.append('moduleBuilder.getNode({})'.format(n))
          else:
            if refCount == 1:
              trueRef = n
            else:
              falseRef = n
            callList.append('NodePointer<NodeId>{}')
          refCount += 1
        else:
          result += 'eastl::vector<{}> {}Var;\n'.format(translate_nodes_type(p.type, language, False), n)
          result += 'while (!{}.empty()) {{'.format(n)
          result += '{0}Var.push_back({0}.consume());\n'.format(n)
          result += '}\n'
          callList.append('{0}Var.data(), {0}Var.size()'.format(n))

      result += 'auto node = moduleBuilder.newNode<{}>({});\n'.format(make_instruction_node_struct_name(instruction), ', '.join(callList))
      result += 'PropertyReferenceResolveInfo info;\n'
      result += 'info.target = reinterpret_cast<NodePointer<NodeId>*>(&node->trueLabel);\n'
      result += 'info.ref = {};\n'.format(trueRef)
      result += 'propertyReferenceResolves.push_back(info);\n'
      result += 'info.target = reinterpret_cast<NodePointer<NodeId>*>(&node->falseLabel);\n'
      result += 'info.ref = {};\n'.format(falseRef)
      result += 'propertyReferenceResolves.push_back(info);\n'
      result += 'currentBlock->exit = node;\n'
      result += 'currentBlock.reset();\n'

    elif instruction.node:
      callList = []
      for t, n in zip(paramList, paramNameList[1:]):
        extendedExtra = []
        if t.type.extended_params:
          extParamNames = generate_extended_params_value_names(t.type)

          if t.is_optional():
            result += 'eastl::optional<{}> {}Val;\n'.format(translate_nodes_type(t.type, language, False, False), n)
          else:
            result += '{} {}Val;\n'.format(translate_nodes_type(t.type, language, False, False), n)

          for e in t.type.values:
            for pi in range(0, len(e.parameters)):
              p = e.parameters[pi]
              if p.type.is_any_input_ref():
                result += '{} {}_{}_{};\n'.format(translate_nodes_type(p.type, language, False), n, e.name, extParamNames[e.name][pi])
              else:
                result += 'eastl::optional<{}> {}_{}_{};\n'.format(translate_nodes_type(p.type, language, False), n, e.name, extParamNames[e.name][pi])
              extendedExtra.append('{}_{}_{}'.format(n, e.name, extParamNames[e.name][pi]))

          if t.is_optional():
            result += 'if ({}.valid) {{'.format(n)
            result += '{0}Val = {0}.value.value;\n'.format(n)
            for e in t.type.values:
              if e.parameters:
                if t.type.category == 'BitEnum':
                  result += 'if (({0}.value.value & {2}::{1}) != {2}::MaskNone) {{'.format(n, e.name, translate_nodes_type(t.type, language, False, False))
                else:
                  result += 'if ({0}.value.value == {2}::{1}) {{'.format(n, e.name, translate_nodes_type(t.type, language, False, False))
                for pi in range(0, len(e.parameters)):
                  p = e.parameters[pi]
                  if p.type.is_any_input_ref():
                    result += '{0}_{1}_{3} = {2}(moduleBuilder.getNode({0}.value.data.{1}.{3}));\n'.format(n, e.name, translate_nodes_type(p.type, language, False), extParamNames[e.name][pi])
                  else:
                    result += '{0}_{1}_{2} = {0}.value.data.{1}.{2};\n'.format(n, e.name, extParamNames[e.name][pi])
                result += '}\n'
          else:
            result += '{0}Val = {0}.value;\n'.format(n)
            for e in t.type.values:
              if e.parameters:
                if t.type.category == 'BitEnum':
                  result += 'if (({0}.value & {2}::{1}) != {2}::MaskNone) {{'.format(n, e.name, translate_nodes_type(t.type, language, False, False))
                else:
                  result += 'if ({0}.value == {2}::{1}) {{'.format(n, e.name, translate_nodes_type(t.type, language, False, False))
                for pi in range(0, len(e.parameters)):
                  p = e.parameters[pi]
                  if p.type.is_any_input_ref():
                    result += '{0}_{1}_{3} = {2}(moduleBuilder.getNode({0}.data.{1}.{3}));\n'.format(n, e.name, translate_nodes_type(p.type, language, False), extParamNames[e.name][pi])
                  else:
                    result += '{0}_{1}_{2} = {0}.value.{1}.{2};\n'.format(n, e.name, extParamNames[e.name][pi])
                result += '}\n'

          if t.is_optional():
            result += '}\n'

        elif t.is_optional():
          result += 'eastl::optional<{}> {}Opt;'.format(translate_nodes_type(t.type, language, False), n)
          result += 'if ({}.valid) {{'.format(n)
          if t.type.is_any_input_ref():
            result += '{0}Opt = moduleBuilder.getNode({0}.value);\n'.format(n)
          elif t.type.is_result_type():
            result += '{0}Opt = moduleBuilder.getType({0}.value);\n'.format(n)
          else:
            result += '{0}Opt = {0}.value;\n'.format(n)

          result += '}\n'

          n = '{}Opt'.format(n)
        elif t.is_variadic():
          result += '// FIXME: use vector directly in constructor\n'
          result += 'eastl::vector<{}> {}Var;\n'.format(translate_nodes_type(t.type, language, False), n)
          result += 'while (!{}.empty()) {{'.format(n)
          if t.type.is_any_input_ref():
            result += '{0}Var.push_back(moduleBuilder.getNode({0}.consume()));\n'.format(n)
          elif t.type.is_result_type():
            result += '{0}Var.push_back(moduleBuilder.getType({0}.consume()));\n'.format(n)
          else:
            result += '{0}Var.push_back({0}.consume());\n'.format(n)
          result += '}\n'

          # right now it has to be this way, as soon the constructors can accept vectors for variadic params we can use it directly
          n = '{0}Var.data(), {0}Var.size()'.format(n)

        if t.type.is_any_input_ref() and not t.is_variadic() and not t.is_optional():
          callList.append('moduleBuilder.getNode({})'.format(n))
        elif t.type.is_result_type():
          callList.append('moduleBuilder.getType({})'.format(n))
        elif t.type.is_result_id():
          callList.append('{}.value'.format(n))
        elif t.type.name == 'LiteralString':
          callList.append('{}.asStringObj()'.format(n))
        elif t.type.extended_params:
          callList.append('{}Val'.format(n))
          callList += extendedExtra
        else:
          callList.append(n)

      if 'typedef' in instruction.properties:
        result += 'moduleBuilder.newNode<{}>({});\n'.format(make_instruction_node_struct_name(instruction), ', '.join(callList))

      else:
        result += 'auto node = moduleBuilder.newNode<{}>({});\n'.format(make_instruction_node_struct_name(instruction), ', '.join(callList))
        if 'ends-block' in instruction.properties:
          result += 'currentBlock->exit = node;\n'
          result += 'currentBlock.reset();\n'
        else:
          result += 'if (currentBlock) currentBlock->instructions.push_back(node);\n'
          if 'global-variable' in instruction.properties:
            result += 'else moduleBuilder.addGlobalVariable(node);\n'
          elif 'undef' in instruction.properties:
            result += 'else moduleBuilder.addGlobalUndef(node);\n'

    else:
      if 'adds-property' in instruction.properties:
        if 'use-property-group' in instruction.properties:
          # if its a composite source then its member
          compositeSource = False
          for t, n in zip(paramList, paramNameList[1:]):
            if not t.is_variadic() and not t.is_optional() and t.type.is_any_input_ref():
              targetName = n
            elif t.type.is_any_input_ref() and t.is_variadic():
              sourceName = n
            elif t.type.category == 'Composite':
              sourceName = n
              compositeSource = True

          result += '// need to store new infos into separate vectors and append them later to not invalidate iterators\n'
          result += 'eastl::vector<PropertyTargetResolveInfo> targetsToAdd;\n'
          result += 'eastl::vector<PropertyReferenceResolveInfo> refsToAdd;\n'
          result += 'for (auto && prop : propertyTargetResolves) {'
          result += 'if (prop.target == {}) {{'.format(targetName)
          result += 'while({}.count) {{'.format(sourceName)
          result += 'auto newTargetValue = {}.consume();\n'.format(sourceName);
          if compositeSource:
            result += 'auto newProp = visitProperty(prop.property.get(), PorpertyCloneWithMemberIndexVisitor{ newTargetValue.second });\n'
            result += 'PropertyTargetResolveInfo targetResolve { newTargetValue.first, eastl::move(newProp) };\n'
          else:
            result += 'auto newProp = visitProperty(prop.property.get(), PorpertyCloneVisitor{});\n'
            result += 'PropertyTargetResolveInfo targetResolve { newTargetValue, eastl::move(newProp) };\n'

          propertyType = language.get_type_by_name('Decoration')
          extParamNames = generate_extended_params_value_names(propertyType, True)
          typeChecks = []
          for v in propertyType.values:
            for pi in range(0, len(v.parameters)):
              p = v.parameters[pi]
              if p.type.is_any_input_ref():
                typeCheck  = 'if (is<Property{}>(newProp)) {{'.format(v.name)
                typeCheck += 'PropertyReferenceResolveInfo refResolve//\n {{ reinterpret_cast<NodePointer<NodeId>*>(&as<Property{}>(newProp)->{}) }};'.format(v.name, extParamNames[v.name][pi])
                typeCheck += 'auto cmp = reinterpret_cast<NodePointer<NodeId>*>(&as<Property{}>(prop.property)->{});'.format(v.name, extParamNames[v.name][pi])
                typeCheck += 'for (auto && ref : propertyReferenceResolves) {\n'
                typeCheck += 'if (ref.target == cmp) {'
                typeCheck += 'refResolve.ref =  ref.ref;\n'
                typeCheck += 'refsToAdd.push_back(refResolve);\n'
                typeCheck += 'break;\n'
                typeCheck += '}\n'
                typeCheck += '}\n'
                typeCheck += '}\n'
                typeChecks.append(typeCheck)
                break
          result += 'else '.join(typeChecks)
          result += 'targetsToAdd.push_back(eastl::move(targetResolve));\n'
          result += '}\n'
          result += '}\n'
          result += '}'
          result += 'propertyReferenceResolves.insert(propertyReferenceResolves.end(), refsToAdd.begin(), refsToAdd.end());\n'
          result += 'for (auto && ta : targetsToAdd)'
          result += 'propertyTargetResolves.push_back(eastl::move(ta));\n'

        else:
          result += 'CastableUniquePointer<Property> propPtr;\n'
          for t, n in zip(paramList, paramNameList[1:]):
            if t.name == 'Decoration':
              result += 'switch ({}.value) {{'.format(n)
              decoName = n
              decoType = t.type
              break

          memberAdd = ''
          if 'member-property' in instruction.properties:
            for t, n in zip(paramList, paramNameList[1:]):
              if t.type.name == 'LiteralInteger':
                memberAdd = 'newProp->memberIndex = {}.value;\n'.format(n)
                break

          # currently uses two naming schemes, should be fixed though!
          # TODO: unify naming of extended params
          extParamNames = generate_extended_params_value_names(decoType, True)
          extParamNames2 = generate_extended_params_value_names(decoType, False)
          seenValues = set()
          for e in decoType.values:
            if e.value in seenValues:
              continue
            seenValues.add(e.value)
            result += 'case {}::{}: {{\n'.format(decoType.get_type_name(), e.name)
            result += 'auto newProp = new Property{};\n'.format(e.name)
            result += 'propPtr.reset(newProp);\n'
            result += memberAdd
            resolveCount = 0
            for pi in range(0, len(e.parameters)):
              p = e.parameters[pi]
              if extParamNames[e.name][pi] != e.name:
                sourceName = '{}.data.{}.{}'.format(decoName, e.name, extParamNames2[e.name][pi])
              else:
                sourceName = '{}.data.{}{}'.format(decoName, e.name, extParamNames2[e.name][pi])

              if p.type.is_any_input_ref():
                resolveCount += 1
                if resolveCount > 1:
                  raise 'Error: more than one id to resolve for properties, needs a rewrite to work!'
                # record that it needs a id resolve
                result += 'PropertyReferenceResolveInfo refResolve//\n {{ reinterpret_cast<NodePointer<NodeId>*>(&newProp->{}), IdRef{{{}.value}}}};'.format(extParamNames[e.name][pi], sourceName)
                result += 'propertyReferenceResolves.push_back(refResolve);\n'
                continue

              if p.type.name == 'LiteralString':
                sourceName += '.asStringObj()'

              result += 'newProp->{} = {};\n'.format(extParamNames[e.name][pi], sourceName)


            result += 'break;}\n'

          result += '}\n'

          for t, n in zip(paramList, paramNameList[1:]):
            if t.type.is_any_input_ref():
              result += 'PropertyTargetResolveInfo targetResolve//\n {{ {}, eastl::move(propPtr) }};\n'.format(n)
              result += 'propertyTargetResolves.push_back(eastl::move(targetResolve));\n'
              break

      if 'source-language' in instruction.properties:
        for p, n in zip(paramList, paramNameList[1:]):
          if p.type.name == 'SourceLanguage':
            result += 'moduleBuilder.setSourceLanguage({});\n'.format(n)
          elif n == 'version':
            result += 'moduleBuilder.setSourceLanguageVersion({}.value);\n'.format(n)

      if 'source-code' in instruction.properties:
        for p, n in zip(paramList, paramNameList[1:]):
          if p.type.name == 'LiteralString':
            result += 'if ({}.valid)'.format(n)
            result += 'moduleBuilder.setSourceString({}.value.asStringObj());\n'.format(n)
          elif p.type.is_any_input_ref():
            result += 'if ({}.valid)'.format(n)
            result += 'moduleBuilder.setSourceFile({}.value);\n'.format(n)

      if 'continue-source-code' in instruction.properties:
        for p, n in zip(paramList, paramNameList[1:]):
          if p.type.name == 'LiteralString':
            result += 'moduleBuilder.continueSourceString({}.asStringObj());\n'.format(n)
            break

      if 'source-code-extension' in instruction.properties:
        anythingWritten = True
        for p, n in zip(paramList, paramNameList[1:]):
          if p.type.name == 'LiteralString':
            result += 'moduleBuilder.addSourceCodeExtension({}.asStringObj());\n'.format(n)
            break

      if 'object-name' in instruction.properties or 'object-member-name' in instruction.properties:
        result += 'CastableUniquePointer<PropertyName> nameProperty { new PropertyName };\n'
        memberIndexName = None
        for p, n in zip(paramList, paramNameList[1:]):
          if p.type.name == 'LiteralString':
            nameString = n
          elif p.type.is_any_input_ref():
            targetName = n
          elif p.type.name == 'LiteralInteger':
            memberIndexName = n

        if memberIndexName:
          result += 'nameProperty->memberIndex = {}.value;\n'.format(memberIndexName)
        result += 'nameProperty->name = {}.asStringObj();\n'.format(nameString)
        result += 'PropertyTargetResolveInfo info {{ {}, eastl::move(nameProperty) }};\n'.format(targetName)
        result += 'propertyTargetResolves.push_back(eastl::move(info));\n'

      if 'set-execution-mode' in instruction.properties or 'set-execution-mode-id' in instruction.properties:
        for p, n in zip(paramList, paramNameList[1:]):
          if p.type.is_any_input_ref():
            targetName = n
          elif p.type.name == 'ExecutionMode':
            modeName = n
            modeType = p.type

        result += 'ExectionModeDef info;\n'
        result += 'info.target = {};\n'.format(targetName)

        result += 'switch ({}.value)'.format(modeName)
        result += '{\n'
        extParamNames = generate_extended_params_value_names(modeType, False)
        for e in modeType.values:
          result += 'case {}::{}:\n'.format(modeType.get_type_name(), e.name)
          paramList = []
          forwardRefs = []
          if e.parameters:
            for pi in range(0, len(e.parameters)):
              p = e.parameters[pi]
              if p.type.is_any_input_ref():
                forwardRefs.append(extParamNames[e.name][pi])
              else:
                if extParamNames[e.name][pi] != e.name:
                  sourceName = '{}.data.{}.{}'.format(modeName, e.name, extParamNames[e.name][pi])
                else:
                  sourceName = '{}.data.{}{}'.format(modeName, e.name, extParamNames[e.name][pi])
                paramList.append(sourceName)

          if forwardRefs:
            result += '{\n'
            result += 'auto emode = new ExecutionMode{};\n'.format(e.name)
            result += 'PropertyReferenceResolveInfo refInfo;\n'
            for p in forwardRefs:
              result += 'refInfo.target = &emode->{};\n'.format(p)
              if p != e.name:
                result += 'refInfo.ref = {}.data.{}.{};\n'.format(modeName, e.name, p)
              else:
                result += 'refInfo.ref = {}.data.{}{};\n'.format(modeName, e.name, p)
              result += 'propertyReferenceResolves.push_back(refInfo);\n'
            result += 'info.mode = emode;\n'
            result += '}\n'
          else:
            result += 'info.mode = new ExecutionMode{}'.format(e.name)

            if paramList:
              result += '({})\n'.format(', '.join(paramList));
          result += ';\n'
          result += 'break;\n'
        result += '}\n'

        result += 'executionModes.push_back(eastl::move(info));\n'

      if 'module-processing-step' in instruction.properties:
        for p, n in zip(paramList, paramNameList[1:]):
          if p.type.name == 'LiteralString':
            result += 'moduleBuilder.addModuleProcess({0}.asString(), {0}.length);\n'.format(n)
            break

    result += '}\n'
  return result

def generate_module_reader(language, build_cfg):
  result = '// auto generated, do not modify!\n'
  result += '#include <spirv/module_builder.h>\n'
  result += '#include "{}"\n'.format(build_cfg.get('module-decoder-file-name'))
  result += '#include "{}"\n'.format(build_cfg.get('module-node-file-name'))
  result += 'using namespace spirv;\n'

  result += 'struct PorpertyCloneVisitor {\n'
  result += 'template<typename T> CastableUniquePointer<Property> operator()(const T *p) { return p->clone(); }\n'
  result += '};\n'

  result += 'struct PorpertyCloneWithMemberIndexVisitor {\n'
  result += 'LiteralInteger newMemberIndex;\n'
  result += 'template<typename T> CastableUniquePointer<Property> operator()(const T *p) { return p->cloneWithMemberIndexOverride(newMemberIndex); }\n'
  result += '};\n'

  result += 'struct ExectionModeDef {\n'
  result += 'IdRef target;\n'
  result += 'CastableUniquePointer<ExecutionModeBase> mode;\n'
  result += '};\n'

  result += '// entry points forward reference functions and its interface, so need to store for later resolve. Reader result types are ok, as the source lives longer that this struct\n'
  result += 'struct EntryPointDef {\n'
  result += 'ExecutionModel executionModel;\n'
  result += 'IdRef function;\n'
  result += 'LiteralString name;\n'
  result += 'Multiple<IdRef> interface;\n'
  result += '};\n'

  result += '// selection merge property needed to be resolved later as it has forward refs\n'
  result += 'struct SelectionMergeDef {\n'
  result += 'NodePointer<NodeBlock> target;\n'
  result += 'IdRef mergeBlock;\n'
  result += 'SelectionControlMask controlMask;\n'
  result += '};\n'

  result += '// loop merge property needed to be resolved later as it has forward refs\n'
  result += 'struct LoopMergeDef {\n'
  result += 'NodePointer<NodeBlock> target;\n'
  result += 'IdRef mergeBlock;\n'
  result += 'IdRef continueBlock;\n'
  result += 'LoopControlMask controlMask;\n'
  result += 'LiteralInteger dependencyLength;\n'
  result += '};\n'

  result += '// generic property definitions, properties (decoartions) are all using forward refs, so need to store it for later resolve\n'
  result += 'struct PropertyTargetResolveInfo {\n'
  result += 'IdRef target;\n'
  result += 'CastableUniquePointer<Property> property;\n'
  result += '};\n'

  result += '// some properties them self forward reference some objects, need to record them for later resolve\n'
  result += 'struct PropertyReferenceResolveInfo {\n'
  result += 'NodePointer<NodeId> *target;\n'
  result += 'IdRef ref;\n'
  result += '};\n'

  result += 'struct ReaderContext {\n'
  result += 'ModuleBuilder &moduleBuilder;\n'
  result += 'eastl::vector<EntryPointDef> &entryPoints;\n'
  result += 'NodePointer<NodeOpFunction> &currentFunction;\n'
  result += 'eastl::vector<IdRef> &forwardProperties;\n'
  result += 'eastl::vector<SelectionMergeDef> &selectionMerges;\n'
  result += 'eastl::vector<LoopMergeDef> &loopMerges;\n'
  result += 'NodePointer<NodeBlock> &currentBlock;\n'
  result += 'eastl::vector<PropertyTargetResolveInfo> &propertyTargetResolves;\n'
  result += 'eastl::vector<PropertyReferenceResolveInfo> &propertyReferenceResolves;\n'
  result += 'eastl::vector<ExectionModeDef> &executionModes;\n'
  result += 'NodeBlock &specConstGrabBlock;\n'
  result += 'unsigned &sourcePosition;\n'

  result += 'template<typename T> void finalize(T on_error) {\n'
  result += 'for (auto && entry : entryPoints) {\n'
  result += 'moduleBuilder.addEntryPoint(entry.executionModel, entry.function, entry.name, entry.interface);\n'
  result += '}\n'
  result += 'for (auto && prop : forwardProperties) {\n'
  result += 'moduleBuilder.getNode(prop)->properties.emplace_back(new PropertyForwardDeclaration);\n'
  result += '}\n'
  result += 'for (auto && smerge : selectionMerges) {\n'
  # TODO: generate constructors and use them here!
  result += 'auto prop = new PropertySelectionMerge;\n'
  result += 'prop->mergeBlock = moduleBuilder.getNode(smerge.mergeBlock);\n'
  result += 'prop->controlMask = smerge.controlMask;\n'
  result += 'smerge.target->properties.emplace_back(prop);\n'
  result += '}\n'
  result += 'for (auto && lmerge : loopMerges) {\n'
  # TODO: generate constructors and use them here!
  result += 'auto prop = new PorpertyLoopMerge;\n'
  result += 'prop->mergeBlock = moduleBuilder.getNode(lmerge.mergeBlock);\n'
  result += 'prop->continueBlock = moduleBuilder.getNode(lmerge.continueBlock);\n'
  result += 'prop->controlMask = lmerge.controlMask;\n'
  result += 'prop->dependencyLength = lmerge.dependencyLength;\n'
  result += 'lmerge.target->properties.emplace_back(prop);\n'
  result += '}\n'

  result += 'for (auto && prop : propertyReferenceResolves) {\n'
  result += '*prop.target = moduleBuilder.getNode(prop.ref);\n'
  result += '}\n'

  result += 'for (auto && prop : propertyTargetResolves) {\n'
  result += 'moduleBuilder.getNode(prop.target)->properties.push_back(eastl::move(prop.property));\n'
  result += '}\n'

  result += 'for (auto && em : executionModes) {\n'
  result += 'moduleBuilder.addExecutionMode(em.target, eastl::move(em.mode));\n'
  result += '}\n'

  result += '}\n'

  result += 'bool operator()(const FileHeader &header) {\n'
  result += '// handle own endian only\n'
  result += 'if (header.magic != MagicNumber) return false;\n'
  result += 'moduleBuilder.setModuleVersion(header.version);\n'
  result += 'moduleBuilder.setToolIdentification(header.toolId);\n'
  result += 'moduleBuilder.setIdBounds(header.idBounds);\n'
  result += ' return true; }\n'
  result += 'void setAt(unsigned pos) { sourcePosition = pos; }\n'
  result += 'unsigned getContextDependentSize(IdResultType type_id) {\n'
  result += 'return moduleBuilder.getTypeSizeBits(moduleBuilder.getType(type_id));\n'
  result += '}\n'
  iCount = 0;
  for i in language.enumerate_all_instructions():
    result += generate_instruction_reader(language, i)
    iCount += 1

  result += '// {} instructions handled\n'.format(iCount)

  result += '};\n'

  result += 'struct ErrorRecorder {\n'
  result += 'ReaderResult &target;\n'
  result += 'bool operator()(const char *msg) { target.errorLog.emplace_back(msg); return true; }\n'
  result += 'bool operator()(const char *op_name, const char *msg) { target.errorLog.emplace_back(op_name); target.errorLog.back().append(" : "); target.errorLog.back().append(msg); return true; }\n'
  result += '};\n'

  result += 'ReaderResult spirv::read(ModuleBuilder &builder, const unsigned *words, unsigned word_count) {'
  result += 'eastl::vector<EntryPointDef> entryPoints;\n'
  result += 'NodePointer<NodeOpFunction> currentFunction;\n'
  result += 'eastl::vector<IdRef> forwardProperties;\n'
  result += 'eastl::vector<SelectionMergeDef> selectionMerges;\n'
  result += 'eastl::vector<LoopMergeDef> loopMerges;\n'
  result += 'NodePointer<NodeBlock> currentBlock;\n'
  result += 'eastl::vector<PropertyTargetResolveInfo> propertyTargetResolves;\n'
  result += 'eastl::vector<PropertyReferenceResolveInfo> propertyReferenceResolves;\n'
  result += 'eastl::vector<ExectionModeDef> executionModes;\n'
  result += 'NodeBlock specConstGrabBlock;\n'
  result += 'unsigned sourcePosition = 0;'
  result += 'ReaderResult resultInfo;\n'
  result += 'ReaderContext ctx//\n{builder, entryPoints, currentFunction, forwardProperties, selectionMerges, loopMerges, currentBlock, propertyTargetResolves, propertyReferenceResolves, executionModes, specConstGrabBlock, sourcePosition};\n'
  result += 'ErrorRecorder onError{resultInfo};\n'
  result += 'if(load(words, word_count, ctx, ctx, onError))\n'
  result += ' ctx.finalize(onError);\n'
  result += 'return resultInfo;\n'
  result += '}\n'

  return result

def main():
  import argparse
  parser = argparse.ArgumentParser(description = 'SPIR-V Module Reader Generator')

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
  print(generate_module_reader(spirv, build_json), file = open(build_json.get('module-reader-file-name'), 'w'))

if __name__ == '__main__':
  main()