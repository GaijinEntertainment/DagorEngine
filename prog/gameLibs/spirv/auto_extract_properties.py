#!/usr/bin/env python
# see main for info

from __future__ import print_function

import json
import spirv_meta_data_tools

def basic_properties(inst):
  # this extended instruction set has only meta data for debugging
  if inst.grammar.name == 'DebugInfo':
    inst.properties.add('debug')

  # typedef
  if inst.name.startswith('OpType'):
    if 'Forward' in inst.name:
      inst.properties.add('forward-typedef')
    else:
      inst.properties.add('typedef')
    # detect if the type has with param which indicates it has a bit with field
    for p in inst.params:
      if "'Width'" == p.name:
        inst.properties.add('bit-size')
        break
  # constants
  elif 'Constant' in inst.name:
    if 'Spec' in inst.name:
      inst.properties.add('spec-constant')
      if 'ConstantOp' in inst.name:
        inst.properties.add('spec-op')
    else:
      inst.properties.add('constant')
    if 'Composite' in inst.name:
      inst.properties.add('composite-constant')
    if 'Sampler' in inst.name:
      inst.properties.add('sampler-constant')
  # mark atomic ops as special
  elif 'Atomic' in inst.name:
    inst.properties.add('atomic-op')
  # phi is a unique op
  elif 'Phi' in inst.name:
    inst.properties.add('phi')

def id_result_matcher(level, name):
  if level == 0:
    return 'IdResult' == name
  else:
    return True

def type_result_matcher(level, name):
  if level == 1:
    return 'IdResultType' == name
  else:
    return id_result_matcher(level, name)

def unary_op_matcher(level, name):
  if level == 2:
    return 'IdRef' == name
  elif level > 2:
    return 'IdRef' != name
  else:
    return type_result_matcher(level, name)

def binary_op_matcher(level, name):
  if level == 3:
    return 'IdRef' == name
  else:
    return unary_op_matcher(level, name)

def trinary_op_matcher(level, name):
  if level == 4:
    return 'IdRef' == name
  else:
    return binary_op_matcher(level, name)

def multinary_op_matcher(level, name):
  if level == 5:
    return 'IdRef' == name
  elif level > 5:
    return True
  else:
    return trinary_op_matcher(level, name)

def unary_action_matcher(level, name):
  if level == 0:
    return 'IdRef' == name
  else:
    return False

def match_root_path(root, path, idex = 0, max_idex = None):
  if not root:
    return None

  if max_idex:
    if idex >= max_idex:
      return root

  if idex >= len(path):
    return root
  if path[idex] == '*':
    return root

  for c in root.children:
    if c.name == path[idex]:
      return match_root_path(c, path, idex + 1)

  return None

def build_advanced_properties(root):
  idResultRoot = match_root_path(root, ['IdResult'])
  resultTypeRoot = match_root_path(idResultRoot, ['IdResultType'])

  unaryOpRoot = match_root_path(resultTypeRoot, ['IdRef'])
  binrayOpRoot = match_root_path(unaryOpRoot, ['IdRef'])
  trinaryOpRoot = match_root_path(binrayOpRoot, ['IdRef'])
  multinaryOpRoot = match_root_path(trinaryOpRoot, ['IdRef'])

  unaryActionRoot = match_root_path(root, ['IdRef'])
  binaryActionRoot = match_root_path(unaryActionRoot, ['IdRef'])
  trinaryActionRoot = match_root_path(binaryActionRoot, ['IdRef'])
  multinaryActionRoot = match_root_path(trinaryActionRoot, ['IdRef'])

  imageActionRoot = match_root_path(root, ['Image'])
  imageOpsRoot = match_root_path(resultTypeRoot, ['Image'])
  sampledImageOpsRoot = match_root_path(resultTypeRoot, ['SampledImage'])

  # have scoped ops node?
  groupOpsRoot = match_root_path(resultTypeRoot, ['IdScope'])
  groupOpsUnaryOpRoot = match_root_path(groupOpsRoot, ['IdRef'])
  groupOpsBinrayOpRoot = match_root_path(groupOpsUnaryOpRoot, ['IdRef'])
  groupOpsTrinaryOpRoot = match_root_path(groupOpsBinrayOpRoot, ['IdRef'])
  groupOpsMultinaryOpRoot = match_root_path(groupOpsTrinaryOpRoot, ['IdRef'])

  # have groups ops node?
  group2OpsRoot = match_root_path(groupOpsRoot, ['GroupOperation'])
  group2OpsUnaryOpRoot = match_root_path(group2OpsRoot, ['IdRef'])
  #group2OpsBinrayOpRoot = match_root_path(group2OpsUnaryOpRoot, ['IdRef'])
  #group2OpsTrinaryOpRoot = match_root_path(group2OpsBinrayOpRoot, ['IdRef'])
  #group2OpsMultinaryOpRoot = match_root_path(group2OpsTrinaryOpRoot, ['IdRef'])

  groupActionRoot = match_root_path(root, ['IdScope'])
  groupActionUnaryOpRoot = match_root_path(groupActionRoot, ['IdRef'])
  groupActionBinrayOpRoot = match_root_path(groupActionUnaryOpRoot, ['IdRef'])
  groupActionTrinaryOpRoot = match_root_path(groupActionBinrayOpRoot, ['IdRef'])
  groupActionMultinaryOpRoot = match_root_path(groupActionTrinaryOpRoot, ['IdRef'])

  for i in root.instructions:
    i.properties.add('action')

  for i in groupActionRoot.enumerate_all_instructions_down():
    i.properties.add('action')

  for i in idResultRoot.enumerate_all_instructions_down():
    i.properties.add('allocates-id')

  for i in resultTypeRoot.enumerate_all_instructions_down():
    i.properties.add('result-type')

  for i in groupOpsRoot.enumerate_all_instructions_down():
    i.properties.add('scoped')

  for i in groupActionRoot.enumerate_all_instructions_down():
    i.properties.add('scoped')

  for i in group2OpsRoot.enumerate_all_instructions_down():
    i.properties.add('grouped')

  for i in unaryOpRoot.enumerate_all_instructions_down_with_child_filter(lambda a, b: (b == 4 and a.name != 'IdRef') or b > 4):
    i.properties.add('unary')

  for i in binrayOpRoot.enumerate_all_instructions_down_with_child_filter(lambda a, b: (b == 5 and a.name != 'IdRef') or b > 5):
    i.properties.add('binary')

  for i in trinaryOpRoot.enumerate_all_instructions_down_with_child_filter(lambda a, b: (b == 6 and a.name != 'IdRef') or b > 6):
    i.properties.add('trinary')

  for i in multinaryOpRoot.enumerate_all_instructions_down():
    i.properties.add('multinary')

  for i in unaryActionRoot.enumerate_all_instructions_down():
    i.properties.add('action')

  for i in unaryActionRoot.enumerate_all_instructions_down_with_child_filter(lambda a, b: (b == 2 and a.name != 'IdRef') or b > 2):
    i.properties.add('unary')

  for i in binaryActionRoot.enumerate_all_instructions_down_with_child_filter(lambda a, b: (b == 3 and a.name != 'IdRef') or b > 3):
    i.properties.add('binary')

  for i in trinaryActionRoot.enumerate_all_instructions_down_with_child_filter(lambda a, b: (b == 4 and a.name != 'IdRef') or b > 4):
    i.properties.add('trinary')

  for i in multinaryActionRoot.enumerate_all_instructions_down():
    i.properties.add('multinary')

  for i in imageActionRoot.enumerate_all_instructions_down():
    i.properties.add('action')
    i.properties.add('image-op')

  for i in imageOpsRoot.enumerate_all_instructions_down():
    i.properties.add('image-op')

  for i in sampledImageOpsRoot.enumerate_all_instructions_down():
    i.properties.add('sampled-image-op')

  #for i in groupOpsUnaryOpRoot.enumerate_all_instructions_down_with_child_filter(lambda a, b: (b == 5 and a.name != 'IdRef') or b > 5):
  #  i.properties.add('unary')

  #for i in groupOpsBinrayOpRoot.enumerate_all_instructions_down_with_child_filter(lambda a, b: (b == 6 and a.name != 'IdRef') or b > 6):
  #  i.properties.add('binary')

  #for i in groupOpsTrinaryOpRoot.enumerate_all_instructions_down_with_child_filter(lambda a, b: (b == 7 and a.name != 'IdRef') or b > 7):
  #  i.properties.add('trinary')

  #for i in groupOpsMultinaryOpRoot.enumerate_all_instructions_down():
  #  i.properties.add('multinary')

  #for i in group2OpsUnaryOpRoot.enumerate_all_instructions_down_with_child_filter(lambda a, b: (b == 6 and a.name != 'IdRef') or b > 6):
  #  i.properties.add('unary')

  #for i in group2OpsBinrayOpRoot.enumerate_all_instructions_down_with_child_filter(lambda a, b: (b == 7 and a.name != 'IdRef') or b > 7):
  #  i.properties.add('binary')

  #for i in group2OpsTrinaryOpRoot.enumerate_all_instructions_down_with_child_filter(lambda a, b: (b == 8 and a.name != 'IdRef') or b > 8):
  #  i.properties.add('trinary')

  #for i in group2OpsMultinaryOpRoot.enumerate_all_instructions_down():
  #  i.properties.add('multinary')

  #for i in groupActionUnaryOpRoot.enumerate_all_instructions_down_with_child_filter(lambda a, b: (b == 3 and a.name != 'IdRef') or b > 3):
  #  i.properties.add('unary')

  #for i in groupActionBinrayOpRoot.enumerate_all_instructions_down_with_child_filter(lambda a, b: (b == 4 and a.name != 'IdRef') or b > 4):
  #  i.properties.add('binary')

  #for i in groupActionTrinaryOpRoot.enumerate_all_instructions_down_with_child_filter(lambda a, b: (b == 5 and a.name != 'IdRef') or b > 5):
  #  i.properties.add('trinary')

  #for i in groupActionMultinaryOpRoot.enumerate_all_instructions_down():
  #  i.properties.add('multinary')

  #root.print_node_paths()

def main():
  import argparse
  parser = argparse.ArgumentParser(description = 'SPIR-V properties generator')

  parser.add_argument('--b',
                      metavar = '<path>',
                      type = str,
                      default = "build.json",
                      required = False,
                      help = 'Path to build info JSON file')

  args = parser.parse_args()


  with open(args.b) as build_file:
    build_json = json.load(build_file)

  # generates properties for instructions
  # override file defines instructions with use defined properties
  # those overridden instructions are ignored by the generator

  with open(build_json.get('instruction-property-overrides')) as override_file:
    override_json = json.load(override_file)


  spirv = spirv_meta_data_tools.Language()
  # loop over every grammar and build the properties for its instructions
  grammars = build_json.get('grammars', dict())
  for gname in grammars:
    with open(grammars[gname]) as input_file:
      spirv.load_grammar(gname, json.load(input_file))

  # override has the same format as normal extended grammar
  spirv.load_extened_grammar(override_json, lambda m: print(m))

  # load nodes def, but only apply no node properties
  with open(build_json.get('node-declarations')) as node_spec_file:
    node_spec_json = json.load(node_spec_file)
    spirv.no_param_node_properties = set(node_spec_json.get('no-param-node-properties', []))

  spirv.finish_loading(lambda m: print(m))

  # need to build a different param node three of instructions without any properties
  rootNode = spirv_meta_data_tools.ParamTypeNode()

  for i in spirv.enumerate_all_instructions():
    # if override has already set the properties, then skip this
    if len(i.properties) > 0:
      continue
    basic_properties(i)
    rootNode.add_inst(i)

  build_advanced_properties(rootNode)

  for i in spirv.enumerate_all_instructions():
    if not i.properties:
      print('{} has no properties'.format(i.name))

  result = spirv_meta_data_tools.make_extended_grammar_json_ready_def(spirv)


  with open(str(build_json.get('instruction-properties')), 'w') as out_file:
    json.dump(result, out_file, indent = 2, separators=(',', ': '))

if __name__ == '__main__':
    main()
