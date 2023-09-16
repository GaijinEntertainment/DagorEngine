#!/usr/bin/env python
"""
Generates SPIR-V Module Decoder header file.
The Decoder is used to decode the SPIR-V Module binary format in a safe way.
"""
from __future__ import print_function

import errno
import json
import os.path
import spirv_meta_data_tools
import re


def make_pretty_extened_grammar_enum_name(name):
  if name.startswith('SPV_'):
    return name[4:]
  return name.replace('.', '_')

def make_instruction_decoder(language, instruction):
  result = 'case Op::{}:{{\n'.format(instruction.name)

  if 'call-extended-grammar' in instruction.properties:
    result += '// call extended grammar function\n'
    paramIndex = 0
    for p in instruction.params:
      result += 'auto c{} = {};\n'.format(paramIndex, make_param_reader(p, 'operands'))
      if p.type.name == 'IdResult':
        resultIndex = paramIndex
      elif p.type.name == 'IdResultType':
        typeIndex = paramIndex
      elif p.type.name == 'LiteralExtInstInteger':
        opIndex = paramIndex
        break
      else:
        extIndex = paramIndex
      paramIndex += 1

    result += 'auto extId = load_context.getExtendedGrammarIdentifierFromRefId(c{});\n'.format(extIndex)
    result += 'switch(extId) {\n'
    result += 'default: if(on_error("error while looking up extended grammar identifier")) return false; break;'
    for g in language.enumerate_extended_grammars():
      result += 'case ExtendedGrammar::{0}: if (!loadExtended{1}(c{2}, c{3}, static_cast<{1}>(c{4}.value), operands, load_context, on_error)) return false; break;'.format(make_pretty_extened_grammar_enum_name(g.name), g.cpp_op_type_name, resultIndex, typeIndex, opIndex)
    result += '};\n'

  elif 'load-extended-grammar' in instruction.properties:
    result += '// load extended grammar functions into id\n'
    paramIndex = 0
    for p in instruction.params:
      result += 'auto c{} = {};\n'.format(paramIndex, make_param_reader(p, 'operands'))
      if p.type.name == 'LiteralString':
        nameIndex = paramIndex
        result += 'auto extId = extended_grammar_name_to_id(c{0}.asString(), c{0}.length);\n'.format(paramIndex)
      elif p.type.name == 'IdResult':
        resultIndex = paramIndex
      paramIndex += 1

    result += 'if (!operands.finishOperands(on_error, "{}")) return false;\n'.format(instruction.name)
    result += 'load_context.loadExtendedGrammar(c{1}, extId, c{0}.asString(), c{0}.length);\n'.format(nameIndex, resultIndex)

  elif 'enable-extension' in instruction.properties:
    result += '// enable extension / extended grammar related features\n'
    paramIndex = 0
    for p in instruction.params:
      result += 'auto c{} = {};\n'.format(paramIndex, make_param_reader(p, 'operands'))
      if p.type.name == 'LiteralString':
        result += 'auto extId = extension_name_to_id(c{0}.asString(), c{0}.length);\n'.format(paramIndex)
        result += 'if (!operands.finishOperands(on_error, "{}")) return false;\n'.format(paramIndex)
        result += 'load_context.enableExtension(extId, c{0}.asString(), c{0}.length);\n'.format(paramIndex)
      paramIndex += 1

  elif 'spec-op' in instruction.properties:
    result += '// number of operands depend on the opcode it uses\n'
    paramIndex = 0
    for p in instruction.params:
      result += 'auto c{} = {};\n'.format(paramIndex, make_param_reader(p, 'operands'))
      if p.type.name == 'IdResult':
        resultIndex = paramIndex
      elif p.type.name == 'IdResultType':
        typeIndex = paramIndex
      elif p.type.name == 'LiteralSpecConstantOpInteger':
        opIndex = paramIndex
        break
      paramIndex += 1

    result += 'switch (static_cast<Op>(c{}.value))'.format(opIndex)
    result += '{\n'
    result += 'default: if (on_error("unexpected spec-op operation")) return false;\n'
    coreGrammar = language.get_core_grammar()
    for inst in coreGrammar.instructions:
      if inst.is_const_op:
        result += 'case Op::{}:'.format(inst.name)
        result += '{\n'
        paramIndex2 = paramIndex
        typeIndex2 = None
        idIndex2 = None
        hasContext = False
        for p in inst.params:
          if 'ContextDependent' in p.type.name and not hasContext:
            hasContext = True
            result += 'operands.setLiteralContextSize(load_context.getContextDependentSize(c{}));\n'.format(typeIndex)
          if p.type.name == 'IdResult':
            idIndex2 = paramIndex2
          elif p.type.name == 'IdResultType':
            typeIndex2 = paramIndex2
          else:
            result += 'auto c{} = {};\n'.format(paramIndex2, make_param_reader(p, 'operands'))
          paramIndex2 += 1

        result += 'if (!operands.finishOperands(on_error, "{}")) return false;\n'.format(instruction.name + '->' + inst.name)
        result += 'load_context.onSpec{}('.format(make_node_create_function_name(inst)[3:])
        paramList = ['Op::{}'.format(inst.name)]
        if resultIndex != None:
          paramList.append('c{}'.format(resultIndex))
        if typeIndex != None:
          paramList.append('c{}'.format(typeIndex))
        paramList += ['c{}'.format(pi) for pi in range(paramIndex, paramIndex2) if pi != idIndex2 and pi != typeIndex2]
        result += ','.join(paramList)
        result += ');\n'
        result += 'break;\n'
        result += '}\n'

    result += '};\n'

  else:
    paramIndex = 0
    typeIndex = None
    idIndex = None
    hasContext = False
    for p in instruction.params:
      if 'ContextDependent' in p.type.name and not hasContext:
        hasContext = True
        result += 'operands.setLiteralContextSize(load_context.getContextDependentSize(c{}));\n'.format(typeIndex)
      result += 'auto c{} = {};\n'.format(paramIndex, make_param_reader(p, 'operands'))
      if p.type.name == 'IdResult':
        idIndex = paramIndex
      elif p.type.name == 'IdResultType':
        typeIndex = paramIndex
      paramIndex += 1

    result += 'if (!operands.finishOperands(on_error, "{}")) return false;\n'.format(instruction.name)
    result += 'load_context.on{}('.format(make_node_create_function_name(instruction)[3:])
    paramList = ['Op::{}'.format(instruction.name)]
    if idIndex != None:
      paramList.append('c{}'.format(idIndex))
    if typeIndex != None:
      paramList.append('c{}'.format(typeIndex))
    paramList += ['c{}'.format(pi) for pi in range(0, paramIndex) if pi != idIndex and pi != typeIndex]
    result += ','.join(paramList)
    result += ');\n'

  result += 'break;}\n'
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

def make_param_type(param):
  if param.is_optional():
    fmt = 'Optional<{}>'
  elif param.is_variadic():
    fmt = 'Multiple<{}>'
  else:
    fmt = '{}'
  return fmt.format(param.type.get_type_name())

def make_param_reader(param, input_name):
  return '{}.read<{}>()'.format(input_name, make_param_type(param))

def make_extended_grammar_decoder(language, grammar):
  result  = 'template<typename LC, typename ECB>'
  result += 'inline bool loadExtended{0}(IdResult result_id, IdResultType result_type, {0} opcode, detail::Operands &operands, LC load_context, ECB on_error)\n'.format(grammar.cpp_op_type_name)
  result += '{\n'
  result += 'switch(opcode) {\n'
  result += 'default: if (on_error("unknown opcode for extended grammar \'{}\'")) return false; break;\n'.format(grammar.name)
  for i in grammar.instructions:
    result += 'case {}::{}:'.format(grammar.cpp_op_type_name, i.name)
    result += '{\n'
    inputCount = 0
    idIndex = None
    for p in i.params:
      if p.type.name == 'IdResult' or p.type.name == 'IdResultType':
        continue
      if 'ContextDependent' in p.type.name and not hasContext:
        hasContext = True
        result += 'operands.setLiteralContextSize(load_context.getContextDependentSize(result_type));\n'
      result += 'auto c{} = {};\n'.format(inputCount, make_param_reader(p, 'operands'))
      inputCount += 1
    result += 'if (!operands.finishOperands(on_error, "{}")) return false;\n'.format(grammar.name + ':' + i.name)
    result += 'load_context.on{}('.format(make_node_create_function_name(i)[3:])
    result += ','.join(['opcode', 'result_id', 'result_type'] + ['c{}'.format(index) for index in range(0, inputCount)])
    result += ');\n'
    result += 'break;}\n'
  result += '};\n'
  result += 'return true;\n'
  result += '}\n'
  return result

def make_decoder(language):
  result  = ''
  for g in language.enumerate_extended_grammars():
    result += make_extended_grammar_decoder(language, g)

  result += 'template<typename HCB, typename LC, typename ECB>'
  result += 'inline bool load(const Id *words, Id word_count, HCB on_header, LC load_context, ECB on_error){\n'
  result += 'if (word_count * sizeof(Id) < sizeof(FileHeader)) { on_error("Input is not a spir-v file, no enough words to process"); return false;}\n'
  result += 'if (!on_header(*reinterpret_cast<const FileHeader *>(words))) return true;\n'
  result += 'auto at = reinterpret_cast<const Id *>(reinterpret_cast<const FileHeader *>(words) + 1);\n'
  result += 'auto ed = words + word_count;\n'
  result += 'while (at < ed){\n'
  result += 'auto op = static_cast<Op>(*at & OpCodeMask);\n'
  result += 'auto len = *at >> WordCountShift;\n'
  result += 'load_context.setAt(at - words);\n'
  result += 'detail::Operands operands{at + 1, len - 1};\n'
  result += 'switch(op) {\n'
  result += 'default:\n'
  result += '// if on_error returns true we stop\n'
  result +=' if(on_error("unknown op code encountered")) return false; break;\n'
  seenValues = dict()
  for i in language.get_core_grammar().instructions:
    if i.code in seenValues:
      result += "// {} alias of {}\n".format(i.name, seenValues[i.code])
    else:
      result += make_instruction_decoder(language, i)
      seenValues[i.code] = i.name
  result += '}\n'
  result += 'at += len;\n'
  result += '}\n'
  result += 'return true;}\n'

  return result

def generate_module_decoder(language, build_cfg):
  result  = '// this file is auto generated, do not modify\n//-V::1020\n'
  result += '#pragma once\n'
  result += '#include "{}"\n'.format(build_cfg.get('generated-cpp-header-file-name'))

  result += 'namespace spirv {'
  # make instruction decoder
  result += '// binary decoders\n'
  result += make_decoder(language)
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
  print(generate_module_decoder(spirv, build_json), file = open(build_json.get('module-decoder-file-name'), 'w'))

if __name__ == '__main__':
  main()