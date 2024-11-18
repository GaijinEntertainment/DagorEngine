#!/usr/bin/env python
"""
Generates a header file from the supplied JSON spec of the SPIR-V
core grammar and optional extension grammars
"""

from __future__ import print_function

from spirv_meta_data_tools import to_name

import errno
import json
import os.path
import spirv_meta_data_tools
import re

def make_enumerant_name_with_scope(en):
  return en.type.name + '::' + en.cpp_enum_name

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

def make_extended_grammar_opcodes(language):
  result = ''
  for g in language.enumerate_extended_grammars():
    if not g.instructions:
      continue
    result += 'enum class {} : unsigned{{\n'.format(g.cpp_op_type_name)
    result += ',\n'.join(['{} = {}'.format(i.name, i.code) for i in g.instructions])
    result += '};\n'

  return result

def make_extended_grammar_enums(language):
  result = ''
  # first all normal enumerations
  for e in language.enumerate_types('ValueEnum'):
    if e.grammar.name == 'core':
      continue
    result += 'enum class {}:unsigned{{\n'.format(e.name)
    result += ',\n'.join(['{} = {}'.format(i.name, i.value) for i in e.values])
    result += '};\n'

  # list of bit ops our types should support
  operators = ['|', '&', '^']
  # then all bitfields
  for e in language.enumerate_types('BitEnum'):
    if e.grammar.name == 'core':
      # nothing to do
      result += ''
    else:
      result += 'enum class {}Mask:unsigned{{\n'.format(e.name)
      result += ',\n'.join(['MaskNone = 0x00000000'] + ['{} = 0x{:0>8X}'.format(i.name, i.value) for i in e.values])
      result += '};\n'
      for o in operators:
        result += 'inline {0}Mask operator{1}({0}Mask a, {0}Mask b) {{ return {0}Mask(unsigned(a) {1} unsigned(b)); }}\n'.format(e.name, o)

  return result

def make_enumerant_name_lookups(language):
  result = ''
  for tp in language.enumerate_types('ValueEnum'):
    result += 'const char *name_of({} value);\n'.format(tp.name)
  return result

def make_typetraits_simple(language):
  result = ''
  # simple types
  for t in language.enumerate_types():
    if t.category == 'Id':
      # basically aliases for Id with special meaning
      result += 'struct {}Tag;\n'.format(t.name)
      result += 'typedef detail::TaggedType<Id, {0}Tag> {0};\n'.format(t.name)
      result += 'template<> struct TypeTraits<{0}> : detail::BasicIdTypeTraits<{0}>'.format(t.name)
      result += '{\n'
      result += 'static const char *doc() {{ return "{}"; }}'.format(t.doc)
      result += 'static const char *name() {{return "{}";}}'.format(t.name)
      result += '};\n'
    elif t.category == 'Literal':
      result += 'template<> struct TypeTraits<{0}>'.format(t.name)
      result += '{\n'

      result += 'static const char *doc() {{ return "{}"; }}'.format(t.doc)
      result += 'static const char *name() {{return "{}";}}'.format(t.name)
      result += 'typedef {} ReadType;\n'.format(t.name)
      result += 'static ReadType read(const Id *&from, const Id *to, Id cds, bool &error)'
      result += '{\n'
      if t.name == 'LiteralString':
        result += 'if (to - from < 1) { error = true; return {}; }\n'
        result += 'ReadType result{static_cast<Id>(to - from), from};\n'
        result += 'from += result.words();\n'
        result += 'return result;\n'
      elif 'ContextDependent' in t.name:
        result += ' if (to - from < (cds / (8 * sizeof(Id)))) { error = true; return {}; }\n'
        result += ' unsigned long long v = *from++;\n'
        result += ' if (cds > 32) { v |= (unsigned long long)(*from) << 32; from++;}\n'
        result += ' return {v};\n'
      else:
        result += 'if (to - from < 1) { error = true; return {}; }\n'
        result += 'ReadType result{*from++};\n'
        result += 'return result;\n'
      result += '};\n'

      result += '};\n'
  return result

def make_typetrait_enum(t):
  result = ''
  nameSet = ['first', 'second', 'third']

  if t.category == 'ValueEnum':
    result += 'template<> struct TypeTraits<{0}>'.format(t.name)
    result += '{\n'
    result += 'static const char *doc() {{ return "{}"; }}'.format(t.doc)
    result += 'static const char *name() {{return "{}";}}'.format(t.name)

    # TODO: extend param name search to try if it is only one, then use the enum name instead
    if t.extended_params:
      result += 'struct ReadType {'
      result += '{} value;\n'.format(t.name)
      result += 'union DataTag {\n'
      for v in t.values:
        if len(v.parameters) < 1:
          continue
        result += 'struct {}Tag'.format(v.name)
        result += '{\n'
        for pi in range(0, len(v.parameters)):
          if v.parameters[pi].type.name == v.parameters[pi].name:
            result += '{} {};\n'.format(v.parameters[pi].type.get_type_name(), nameSet[pi])
          else:
            result += '{} {};\n'.format(v.parameters[pi].type.get_type_name(), to_name(v.parameters[pi].name))
        result += '}'
        result += v.name
        result += ';\n'
      result += '} data;'
      result += '};'
    else:
      result += 'typedef {} ReadType;'.format(t.name)
    result += 'static ReadType read(const Id *&from, const Id *to, Id cds, bool &error)'
    result += '{\n'
    result += 'if (to - from < 1) { error = true; return {}; }\n'
    if t.extended_params:
      result += 'ReadType result;\n'
      result += 'result.value = static_cast<{}>(*from++);\n'.format(t.name)
      result += 'switch(result.value) {'
      result += 'default: error = true; return {};\n'
      hasAnyEmpty = False
      for v in t.enumerate_unique_values():
        if len(v.parameters) < 1:
          result += 'case {}::{}:'.format(t.name, v.name)
          hasAnyEmpty = True
      if hasAnyEmpty:
        result += 'break;\n'
      for v in t.enumerate_unique_values():
        if len(v.parameters) < 1:
          continue
        result += 'case {}::{}:'.format(t.name, v.name)
        result += '{\n'
        for pi in range(0, len(v.parameters)):
          if v.parameters[pi].type.name == v.parameters[pi].name:
            result += 'result.data.{}.{} = TypeTraits<{}>::read(from, to, cds, error);\n'.format(v.name, nameSet[pi], v.parameters[pi].type.get_type_name())
          else:
            result += 'result.data.{}.{} = TypeTraits<{}>::read(from, to, cds, error);\n'.format(v.name, to_name(v.parameters[pi].name), v.parameters[pi].type.get_type_name())
          result += 'if (error) return {};\n'
        result += 'break;\n'
        result += '};\n'
      result += '};\n'
    else:
      result += 'ReadType result{static_cast<ReadType>(*from++)};\n'
    result += 'return result;\n'
    result += '}\n'
    result += '};\n'
  elif t.category == 'BitEnum':
    result += 'template<> struct TypeTraits<{0}Mask>'.format(t.name)
    result += '{\n'
    result += 'static const char *doc() {{ return "{}"; }}'.format(t.doc)
    result += 'static const char *name() {{return "{}";}}'.format(t.name)
    # TODO: extend param name search to try if it is only one, then use the enum name instead
    if t.extended_params:
      result += 'struct ReadType {'
      result += '{}Mask value;\n'.format(t.name)
      result += 'struct DataTag {\n'
      for v in t.values:
        if len(v.parameters) < 1:
          continue
        result += 'struct {}Tag'.format(v.name)
        result += '{\n'
        for pi in range(0, len(v.parameters)):
          if v.parameters[pi].type.name == v.parameters[pi].name:
            result += '{} {};\n'.format(v.parameters[pi].type.get_type_name(), nameSet[pi])
          else:
            result += '{} {};\n'.format(v.parameters[pi].type.get_type_name(), to_name(v.parameters[pi].name))
        result += '}'
        result += v.name
        result += ';\n'
      result += '} data;'
      result += '};'
    else:
      result += 'typedef {}Mask ReadType;'.format(t.name)
    result += 'static ReadType read(const Id *&from, const Id *to, Id cds, bool &error)'
    result += '{\n'
    if t.extended_params:
      result += 'if (to - from < 1) { error = true; return {}; }\n'
      result += 'ReadType result;\n'
      result += 'result.value = static_cast<{}Mask>(*from++);\n'.format(t.name)
      doneSet = set()
      doneSet.add(0)
      if t.overlapping_values:
        result += 'auto compareValue = result.value;\n'
        for v in sorted(t.values, key = lambda p: p.value):
          if v.value in doneSet:
            continue
          doneSet.add(v.value)
          result += 'if ({0}Mask::{1} == (compareValue & {0}Mask::{1}))'.format(t.name, v.name)
          result += '{\n'
          for pi in range(0, len(v.parameters)):
            if v.parameters[pi].type.name == v.parameters[pi].name:
              result += 'result.data.{}.{} = TypeTraits<{}>::read(from, to, cds, error);\n'.format(v.name, nameSet[pi], v.parameters[pi].type.get_type_name())
            else:
              result += 'result.data.{}.{} = TypeTraits<{}>::read(from, to, cds, error);\n'.format(v.name, to_name(v.parameters[pi].name), v.parameters[pi].type.get_type_name())
            result += 'if (error) return {};\n'
          # modify compare value to avoid loads when they are already handled
          result += 'compareValue = compareValue ^ {0}Mask::{1};\n'.format(t.name, v.name)
          result += '}\n'
      else:
        for v in t.values:
          if len(v.parameters) < 1:
            continue
          if v.value in doneSet:
            continue
          doneSet.add(v.value)
          result += 'if ({0}Mask::{1} == (result.value & {0}Mask::{1}))'.format(t.name, v.name)
          result += '{\n'
          for pi in range(0, len(v.parameters)):
            if v.parameters[pi].type.name == v.parameters[pi].name:
              result += 'result.data.{}.{} = TypeTraits<{}>::read(from, to, cds, error);\n'.format(v.name, nameSet[pi], v.parameters[pi].type.get_type_name())
            else:
              result += 'result.data.{}.{} = TypeTraits<{}>::read(from, to, cds, error);\n'.format(v.name, to_name(v.parameters[pi].name), v.parameters[pi].type.get_type_name())
            result += 'if (error) return {};\n'
          result += '}\n'
    else:
      result += 'if (to - from < 1) { error = true; return {}; }\n'
      result += 'ReadType result{static_cast<ReadType>(*from++)};\n'
    result += 'return result;\n'
    result += '};\n'
    result += '};\n'
  return result

def make_typetraits_enum(language):
  result = ''

  #need to reorder defenitions to make it compile

  #filter enums
  enumTypes = []
  for t in language.enumerate_types():
    if (t.category == 'ValueEnum') or (t.category == 'BitEnum'):
      enumTypes.append(t)

  #find dependencies
  orderedTypeIds = []
  for i in range(0, len(enumTypes)):
    t = enumTypes[i]
    if t.category == 'ValueEnum':
      if t.extended_params:
        for v in t.enumerate_unique_values():
          if len(v.parameters) < 1:
            continue

          for u in v.parameters:
            fieldTypeName = u.type.name

            found = False
            for q in orderedTypeIds:
              if fieldTypeName == enumTypes[q].name:
                found = True
                break
            if found:
              continue

            for j in range(0, len(enumTypes)):
              if j == i:
                continue
              q = enumTypes[j]
              if fieldTypeName == q.name:
                orderedTypeIds.append(j)
                break

  #comment about what was reordered
  result += '//reordered type traits defenitions\n'
  for t in orderedTypeIds:
    result += '// {0} \n'.format(enumTypes[t].name)

  #generate output
  for t in orderedTypeIds:
    result += make_typetrait_enum(enumTypes[t])

  for i in range(0, len(enumTypes)):
    t = enumTypes[i]
    if i in orderedTypeIds:
      continue
    result += make_typetrait_enum(t)

  return result

def make_typetraits_composite(language):
  result = ''
  elementNames = ['first', 'second']
  for t in language.enumerate_types():
    if t.category == 'Composite':
      result += 'struct {}'.format(t.name)
      result += '{\n'
      for i in range(0, len(t.components)):
        result += '{} {};\n'.format(t.components[i].name, elementNames[i])
      result += '};\n'
      result += 'template<>'
      result += 'struct TypeTraits<{}>'.format(t.name)
      result += '{\n'
      result += 'static const char *name() {{ return "{0}";}}\n'.format(t.name)
      result += 'static {} read(const Id *&from, const Id *to, Id cds, bool &error)'.format(t.name)
      result += '{\n'
      result += 'auto localFrom = from;\n'
      for i in range(0, len(t.components)):
        result += 'auto c{} = TypeTraits<{}>::read(localFrom, to, cds, error);\n'.format(i, t.components[i].name)
        result += 'if (error) return {};\n'
      result += '{} result;\n'.format(t.name)
      for i in range(0, len(t.components)):
        result += 'result.{} = c{};\n'.format(elementNames[i],i)
      result += 'from = localFrom;\n'
      result += 'return result;\n'
      result += '}\n'
      result += '};\n'
  return result

def make_typetraits(language):
  result = ''
  result += make_typetraits_simple(language)
  result += make_typetraits_enum(language)
  result += make_typetraits_composite(language)
  return result

def make_instruction_node_struct_name(instruction):
  return '{}Op{}'.format(instruction.grammar.language.get_root_node().name, make_node_create_function_name(instruction)[3:])

def make_node_decl_new(node):
  result = 'struct {};\n'.format(node.full_node_name())
  for c in node.childs:
    result += make_node_decl_new(c)
  return result

def make_extension_enum(language):
  result  = 'enum class Extension {\n'
  result += ',\n'.join(['{} = {}'.format(e.name[4:], e.index) for e in sorted([e for e in language.enumerate_extensions()], key = lambda e: e.index)] + ['Count'])
  result += '};\n'
  return result

def make_pretty_extened_grammar_enum_name(name):
  if name.startswith('SPV_'):
    return name[4:]
  return name.replace('.', '_')

def make_extended_grammars_enum(language):
  result  = 'enum class ExtendedGrammar {\n'
  result += ',\n'.join(['{} = {}'.format(make_pretty_extened_grammar_enum_name(g.name), g.index) for g in sorted([g for g in language.enumerate_extended_grammars()], key = lambda e: e.index)] + ['Count'])
  result += '};\n'
  return result

def make_node_visitor_for_node(node):
  """
  Generates visitor calls for non operation nodes
  """
  result = ''
  for c in node.childs:
    result += make_node_visitor_for_node(c)

  result += 'if ({}::is(node))'.format(node.full_node_name())
  result += '  if (h(reinterpret_cast<{} *>(node)))\n'.format(node.full_node_name())
  result += '    return;\n'

  return result

def make_node_dispacher(language):
  result = '// a dispatcher that tries to dispatch the exact instruction type first, if the callback returns false it tries to go up the cast chain until the callback returns true\n'
  result += 'template<typename N, typename H>\n'
  result += 'inline void visitNode(N* node, H h) {\n'
  result += 'switch(node->opCode) {'
  extBlock = dict()
  for i in language.enumerate_all_instructions():
    if not i.node:
      continue

    if i.grammar.is_core():
      result += 'case Op::{}:'.format(i.name)
      result += 'if (h(reinterpret_cast<{} *>(node))) return;\n'.format(make_instruction_node_struct_name(i))
      result += 'break;\n'
    else:
      # extended ops are grouped by their grammar and put in a later block
      extBlock.setdefault(i.grammar, []).append(i)

  if extBlock:
    result += 'case Op::OpExtInst:\n'
    result += 'switch(node->grammarId) {\n'
    for g in extBlock:
      result += 'case ExtendedGrammar::{}:'.format(make_pretty_extened_grammar_enum_name(g.name))
      result += 'switch(static_cast<{}>(node->extOpCode))'.format(g.cpp_op_type_name)
      result += '{\n'
      for i in extBlock[g]:
        result += 'case {}::{}:'.format(g.cpp_op_type_name, i.name)
        result += 'if (h(reinterpret_cast<{} *>(node))) return;\n'.format(make_instruction_node_struct_name(i))
        result += 'break;\n'
      result += '}\n'
      result += 'break;\n'

    result += '}\n'
    result += 'break;\n'
  result += '}\n'
  # and and now generic nodes by hierarchy bottom up
  result += make_node_visitor_for_node(language.get_root_node())
  result += '}\n'

  # overload for NodePointer version
  result += 'template<typename N, typename H>\n'
  result += 'inline void visitNode(const NodePointer<N> &node, H h) { visitNode(node.get(), h); }\n'
  result += 'template<typename N, typename H>\n'
  result += 'inline void visitNode(NodePointer<N> &node, H h) { visitNode(node.get(), h); }\n'

  return result

def generate_header(language, cfg, module_sections_json):
  result = make_traits_header_preamble();
  result += '#include "{}"\n'.format(cfg.get('base-cpp-header-file-name'))
  result += '//temporary hack for value named as type i.e. struct a#Tag { a b; } a;\n'
  result += 'typedef spirv::FPRoundingMode FPRoundingModeValue;\n'
  result += '\n'
  result += 'namespace spirv {\n'
  # simple prototypes for extension lookups
  result += make_extension_enum(language)
  result += make_extended_grammars_enum(language)
  result += '// extension and extended grammar names to internal id translation\n'
  result += 'Extension extension_name_to_id(const char *name, Id name_len);\n'
  result += 'dag::ConstSpan<char> extension_id_to_name(Extension ident);\n'
  result += 'ExtendedGrammar extended_grammar_name_to_id(const char *name, Id name_len);\n'
  result += 'dag::ConstSpan<char> extended_grammar_id_to_name(ExtendedGrammar ident);\n'
  result += '// extended grammar opcodes\n'
  result += make_extended_grammar_opcodes(language)
  result += '// extended grammar enum types\n'
  result += make_extended_grammar_enums(language)
  result += '// type traits\n'
  result += make_typetraits(language)
  result += '// enumerant name lookups\n'
  result += make_enumerant_name_lookups(language)

  result += '}\n'

  result += make_traits_header_epilogue();

  return result

def generate_source(language, cfg):
  result  = '// Copyright (C) Gaijin Games KFT.  All rights reserved.\n'
  result += '// auto generated, do not modify!\n'
  result += '#include "{}"\n'.format(cfg.get('generated-cpp-header-file-name'))
  result += 'using namespace spirv;\n'

  idFindTemplate = 'for (Id i = 0; i < static_cast<Id>({1}); ++i) if (string_equals(name, {0}[i], name_len)) return static_cast<{2}>(i); return {1};'
  nameFindTemplate = 'if (ident < {1}) return make_span({0}[static_cast<unsigned>(ident)], {0}_len[static_cast<unsigned>(ident)]); return make_span("<invalid {2} id>", {3} + 13);'

  # extension name <-> id mapping
  sortedExtensionList = ['"{}"'.format(e.name) for e in sorted([e for e in language.enumerate_extensions()], key = lambda e: e.index)]

  result += 'static const char *extension_name_table[] = //\n'
  result += '{\n'
  result += ',\n'.join(sortedExtensionList)
  result += '};\n'
  result += 'static const size_t extension_name_table_len[] = //\n'
  result += '{\n'
  result += ',\n'.join(['{}'.format(len(e) - 2) for e in sortedExtensionList])
  result += '};\n'
  result += 'Extension spirv::extension_name_to_id(const char *name, Id name_len){\n'
  result += idFindTemplate.format('extension_name_table', 'Extension::Count', 'Extension')
  result += '};\n\n'
  result += 'dag::ConstSpan<char> spirv::extension_id_to_name(Extension ident){\n'
  result += nameFindTemplate.format('extension_name_table', 'Extension::Count', 'extension', 9)
  result += '};\n\n'

  # extended grammar name <-> id mapping
  sortedExtendedGrammarList = ['"{}"'.format(g.name) for g in sorted([g for g in language.enumerate_extended_grammars()], key = lambda g: g.index)]

  result += 'static const char *extended_grammar_name_table[] = //\n'
  result += '{\n'
  result += ',\n'.join(sortedExtendedGrammarList)
  result += '};\n'
  result += 'static const size_t extended_grammar_name_table_len[] = //\n'
  result += '{\n'
  result += ',\n'.join(['{}'.format(len(e) - 2) for e in sortedExtendedGrammarList])
  result += '};\n'

  result += 'ExtendedGrammar spirv::extended_grammar_name_to_id(const char *name, Id name_len){\n'
  result += idFindTemplate.format('extended_grammar_name_table', 'ExtendedGrammar::Count', 'ExtendedGrammar')
  result += '};\n\n'
  result += 'dag::ConstSpan<char> spirv::extended_grammar_id_to_name(ExtendedGrammar ident){\n'
  result += nameFindTemplate.format('extended_grammar_name_table', 'ExtendedGrammar::Count', 'extended grammar', 16)
  result += '};\n\n'

  # generate enum value lookup implementations
  for tp in language.enumerate_types('ValueEnum'):
    result += 'const char *spirv::name_of({} value){{\n'.format(tp.name)
    result += 'switch(value){\n'
    for e in tp.enumerate_unique_values():
        result += 'case {}: return "{}";\n'.format(make_enumerant_name_with_scope(e), e.name)
    result += '}\n'
    result += 'return "<unrecognized enum value of {}>";\n'.format(tp.name)
    result += '}\n\n'

  return result

def make_traits_header_preamble():
  return '//\n' \
         '// Dagor Engine 6.5 - Game Libraries\n' \
         '// Copyright (C) Gaijin Games KFT.  All rights reserved.\n' \
         '//\n' \
         '#pragma once\n\n' \
         '// Auto Generated File, Generated By spirv/gen_traits.py\n\n'

def make_traits_header_epilogue():
  return '\n'

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
  print(generate_header(spirv, build_json, section_json), file = open(build_json.get('generated-cpp-header-file-name'), 'w'))
  print(generate_source(spirv, build_json), file = open(build_json.get('generated-cpp-source-file-name'), 'w'))

if __name__ == '__main__':
  main()
