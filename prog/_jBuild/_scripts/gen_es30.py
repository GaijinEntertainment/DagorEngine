import os


def strip_end(text, suffix):
    if not text.endswith(suffix):
        return text
    return text[:len(text) - len(suffix)]


def strip_start(text, prefix):
    if not text.startswith(prefix):
        return text
    return text[len(prefix):]


def strip_start_all(text, prefix):
    if not text.startswith(prefix):
        return text
    return strip_start_all(text[len(prefix):], prefix)


dot_suffix = "__"
#dot_suffix = "."
event_handler_suffix = "_event_handler"
generic_event_type = "ecs::Event"
ecs_query_suffix = "_ecs_query"
global_input_file_name = ""
reserved_components_name = {"_worker_id_" : "components.getWorkerId()", "manager" : "components.manager()"}
reserved_components_types = {"_worker_id_" : "int", "manager" : "ecs::EntityManager"}
def verify_reserved_type(name, tp):
  if name in reserved_components_types:
    if reserved_components_types[name] != tp:
      print("reserved {name} should always have type {ntype} and it has {tp}".format(name=name, ntype= reserved_components_types[name], tp = tp))
      exit(1)

class ESFunction:
  def __init__(self, name):
    self.list_of_params = set([])
    self.result_type = 'void'
    self.functionDeclAnnotation = []
    self.condition = []
    self.ro_params = []
    self.rw_params = []
    self.rq_params = []
    self.no_params = []
    self.stage_param = {}
    self.context_param = []
    self.call_params = []
    self.stagesNames = []
    self.stagesTypes = []
    self.contextsTypes = []
    self.contextsIsOptional = []
    self.stagesCalls = []
    self.hasComponents = False
    self.eventHandlers = []
    self.index = 0
    self.funcName = name
    self.baseFuncName = name
    self.combined = False
    self.fullFuncName = name
    self.annotations = []
    self.annotatedEvents = []
    self.annotatedRequirements = []
    self.annotatedRequirementNots = []
    self.annotatedTags = []
    self.annotatedBefore = []
    self.annotatedAfter = []
    self.annotatedTracked = []
    self.isOptional = False
    self.breakable = False
    self.params_array = {'ro': {}, 'rw': {}, 'rq': {}, 'no': {}}
    self.eventList = []


def remove_const_from_type(type_name):
  type_name = type_name.replace("unsigned long long", "uint64_t")
  type_name = type_name.replace("long long", "int64_t")
  if os.name == 'posix':
    type_name = type_name.replace("unsigned long", "uint64_t")
    type_name = type_name.replace("long", "int64_t")
  return strip_start(type_name, "const ")


def is_flag_type(ro_type):
  if (ro_type == 'eastl::true_type' or ro_type == 'eastl::false_type'):
    return True
  return False


def gather_annotate_list(prefix, annotations):
  filtered = []
  for annotation in annotations:
    if annotation.startswith(prefix):
      filtered.append(strip_start(annotation, prefix))
  return filtered


def checkForPresenceAndRemoveOptional(noParam, paramsList, name):
  for param in paramsList:
    if noParam['name'] == param['name']:
      if param['optional'] is False:
        print("Param <" + noParam['name'] + "> presented in REQUIRED_NOT is non-optional in " + name + " list")
        print(noParam)
        print(param)
        exit(1)
      else:
        print("Param <" + noParam['name'] + "> presented in REQUIRED_NOT is also optional in " + name + " list")
        paramsList.remove(param)


def es_function_from_parsed_function(fun):
  for fid in range(0, len(fun.call_params)):
    if ("type" in fun.call_params[fid]):
        fun.call_params[fid]["type"] = remove_const_from_type(fun.call_params[fid]["type"])
  global event_handler_suffix
  firstCallParam = 0
  if fun.funcName.endswith("_es") or fun.funcName.endswith("_es" + event_handler_suffix):
    esFun = ESFunction(fun.funcName)
    esFun.stage_param = fun.call_params[0]
    firstCallParam = 1
  elif (fun.funcName.endswith(ecs_query_suffix)):
    esFun = ESFunction(fun.funcName)
    esFun.breakable = fun.cb_result_type == "ecs::QueryCbResult"
    esFun.is_eid_query = fun.is_eid_query
    esFun.eidArgId = fun.eidArgId
    esFun.mgrArgId = fun.mgrArgId
    esFun.mgrType = fun.mgrType
    firstCallParam = 0
  esFun.fullFuncName = fun.funcFullName
  esFun.annotatedEvents = gather_annotate_list("@Event:", fun.annotations)
  for ev in esFun.annotatedEvents:
    if ev == "on_appear":
      esFun.annotatedEvents.append("ecs::EventEntityCreated")
      esFun.annotatedEvents.append("ecs::EventComponentsAppear")
    if ev == "on_disappear":
      esFun.annotatedEvents.append("ecs::EventEntityDestroyed")
      esFun.annotatedEvents.append("ecs::EventComponentsDisappear")
  seen = set(["on_appear", "on_disappear"])
  esFun.annotatedEvents = [x for x in esFun.annotatedEvents if x not in seen and not seen.add(x)]#remove duplicates, keeping order

  esFun.annotatedRequirements = gather_annotate_list("@require:", fun.annotations)
  esFun.annotatedRequirementNots = gather_annotate_list("@require_not:", fun.annotations)
  esFun.annotatedTags = gather_annotate_list("@tag:", fun.annotations)
  esFun.annotatedRequirements += gather_annotate_list("@require:", fun.functionDeclAnnotation)
  esFun.annotatedRequirementNots += gather_annotate_list("@require_not:", fun.functionDeclAnnotation)
  esFun.annotatedTags += gather_annotate_list("@tag:", fun.functionDeclAnnotation)
  esFun.annotatedBefore = gather_annotate_list("@before:", fun.annotations)
  esFun.annotatedAfter = gather_annotate_list("@after:", fun.annotations)
  if "*" in esFun.annotatedBefore or "*" in esFun.annotatedAfter:
    esFun.annotatedBefore = ["*"]
    esFun.annotatedAfter = []

  annotatedTracked = gather_annotate_list("@track:", fun.annotations)
  for i in annotatedTracked:
    dot_replaced = i.replace("_dot_", dot_suffix)
    if dot_replaced != i:
      print("Using _dot_ is deprecated, check track annotation {name} for: {fun} in {file}".format(name = i, fun = fun.funcFullName, file = global_input_file_name))
    if dot_replaced not in esFun.annotatedTracked: # intentionally don't use set() to avoid depending on it's sorting order (implementation dependant)
      esFun.annotatedTracked.append(dot_replaced)
  esFun.annotations = fun.annotations
  esFun.functionDeclAnnotation = fun.functionDeclAnnotation
  esFun.result_type = fun.result_type

# assert len(fun.call_params) > firstCallParam or len(esFun.annotatedRequirements) or len(esFun.annotatedRequirementNots), "es_Function {name} hasn't parsed correctly or has zero parameters (do nothing)".format(name=fun.funcName)
  for fid in range(firstCallParam, len(fun.call_params)):
    fi = fun.call_params[fid]
    if fi["name"].find("_dot_") > 0:
      print("Using _dot_ is deprecated, check {name} in function {fun} in {file}".format(name = fi["name"], fun = fun.funcName, file = global_input_file_name))

    assert "type" in fi, "es_Function {name} hasn't parsed correctly".format(name=fun.funcName)
    assert "name" in fi, "es_Function {name} has nameles <{param}> param of type <{type}>. We won't be able to make binding".format(name=fun.funcName, param=fid, type=fi["type"])

    i = {"name": fi["name"], "type": fi["type"]}
    i["param_type"] = "rw" if fi["mutable"] else "ro"
    if i["type"].endswith("RW"):  # legacy for ecs20 compatibility
      i["param_type"] = "rw"  # legacy for ecs20 compatibility

    if (fi["optional"] is False):
      i["optional"] = False
    elif (fi["optional"] == 'NULL' or fi["optional"] == 'nullptr'):
      i["optional"] = 'ptr'
    else:
      i["optional"] = 'val'
      i["defVal"] = fi["optional"]
    if (i["type"].find('EntityId') and i["name"] == 'eid'):
      assert i["param_type"] == 'ro' or i["param_type"] == 'rq', "'eid' can't be REQUIRED_NOT or RW"

    i["type_size"] = fi["type_size"]
    if is_flag_type(i["type"]):
      i["flag_type"] = i["type"]
      esFun.condition.append(i)
    esFun.call_params.append(i)

    if (i['param_type'] == 'ro'):
      esFun.ro_params.append(i)
    elif (i['param_type'] == 'rw'):
      esFun.rw_params.append(i)
  for reqs in esFun.annotatedRequirements:
    optional = False
    if reqs.rfind('=') != -1:
      optional = 'val'
      startOptional = reqs.rfind('=')
      defVal = reqs[startOptional + 1:]
      reqs = reqs[:startOptional]
    while (reqs.endswith(' ')):
      reqs = reqs[:len(reqs) - 1]

    name_index = reqs.rfind(' ')
    if name_index < 0:
      print("es_Function {name} hasn't parsed correctly one of it's ECS_REQUIRE coponents. Correct syntax is ECS_REQUIRE(Type name, Type2 name2)".format(name=fun.funcName))
      exit(1)
    type_name = remove_const_from_type(reqs[:name_index])
    name = reqs[name_index + 1:]
    while (name.startswith('&')):
      name = name[1:]
    while (type_name.endswith('&') or type_name.endswith(' ')):
      type_name = type_name[:len(type_name) - 1]
    if is_flag_type(type_name):
      i = {"name": name, "type": "bool", "param_type": "ro", "optional": optional, "flag_type": type_name}
      if optional == 'val':
        i['defVal'] = defVal
      esFun.condition.append(i)
      esFun.ro_params.append(i)
    elif optional is False:
      i = {"name": name, "type": type_name, "param_type": "rq", "optional": False}
      esFun.rq_params.append(i)
  for reqs in esFun.annotatedRequirementNots:
    optional = False
    if reqs.rfind('=') != -1:
      optional = True
      reqs = reqs[:reqs.rfind('=')]
    while (reqs.endswith(' ')):
      reqs = reqs[:len(reqs) - 1]

    name_index = reqs.rfind(' ')
    if name_index < 0:
      print("es_Function {name} hasn't parsed correctly one of it's ECS_REQUIRE_NOT coponents. Correct syntax is ECS_REQUIRE_NOT(Type name, Type2 name2)".format(name=fun.funcName))
      exit(1)
    type_name = remove_const_from_type(reqs[:name_index])
    name = reqs[name_index + 1:]
    while (name.startswith('&')):
      name = name[1:]
    while (type_name.endswith('&') or type_name.endswith(' ')):
      type_name = type_name[:len(type_name) - 1]
    if optional is False:
      i = {"name": name, "type": type_name, "param_type": "no", "optional": False}
      esFun.no_params.append(i)

  for noParam in esFun.no_params:
    checkForPresenceAndRemoveOptional(noParam, esFun.rw_params, "RW")
    checkForPresenceAndRemoveOptional(noParam, esFun.ro_params, "RO")
    checkForPresenceAndRemoveOptional(noParam, esFun.rq_params, "RQ")
  return esFun


def getCompsName(funcName, suffix):
  return funcName + "_comps" + suffix


def find_non_optional_params_list(name, lst):
  for i in lst:
    if (name == i['name'] and i['optional'] is False):
      return True
  return False


def filter_params_list(esFunction):
  for i in esFunction.rq_params:
    if find_non_optional_params_list(i['name'], esFunction.ro_params) or find_non_optional_params_list(i['name'], esFunction.rw_params):
      esFunction.rq_params.remove(i)
      continue


def gen_params_list(params, suffix, genList):
  for i in params:
    if (i['param_type'] != suffix):
      params.remove(i)
      continue
  addList = []
  if (len(params) > 0):
    for i in params:
      if i['name'] in reserved_components_name:
        verify_reserved_type(i['name'], i['type'])
        continue
      typeName = i['type']
      name = i['name'].replace("_dot_", dot_suffix)
      line = '  {ECS_HASH("%s"), ecs::ComponentTypeInfo<%s>()' % (name, typeName)
      flags = []
      if (i['optional'] is not False):
        flags.append('ecs::CDF_OPTIONAL')
      if (len(flags)):
        line += ', ' + ('|'.join(flags) if len(flags) > 1 else flags[0])
      line += '}'
      addList.append(line)
  if len(addList) > 0:
    addList[0] = "//start of {num} {suffix} components at [{start}]\n".format(suffix=suffix, num=len(addList), start=len(genList)) + addList[0]
  start = len(genList)
  genList += addList
  return start, len(addList)


def getSimdFuncName(funcName):
  return funcName + '_all'


def getEventHandlerFuncName(funcName):
  return funcName + '_all_events'


def getStageMask(stageName):
  return '(1<<' + stageName + ')'


def getStageName(esFunction):
  return esFunction.stage_param['type'] + '::STAGE'


def non_optional_param_names(params):
  ret = set([])
  for x in params:
    if not x["optional"]:
      ret.add(x["name"])
  return frozenset(ret)


def add_tag_string(tags, already_printed):
  list_str = ",".join(tags)
  already_printed = True if len(list_str) > 0 or already_printed else False
  if already_printed:
    list_str = ',nullptr' if len(list_str) == 0 else (',"' + list_str + '"')
  else:
    list_str = ''
  return list_str, already_printed

#this function match that there are user_info = nullptr before that.
def add_quant(quant_number, already_printed):

  already_printed = True if len(quant_number) > 0 or already_printed else False
  if already_printed:
    quant_number = ',nullptr' if len(quant_number) == 0 else (', nullptr, ' + quant_number + '')
  else:
    quant_number = ''
  return quant_number, already_printed


def gen_es_desc(esFunction, moduleName, stages, rw_params_str, ro_params_str, rq_params_str, no_params_str):
  funcName = esFunction.funcName
  global event_handler_suffix
  semanticFuncName = strip_end(funcName, event_handler_suffix)
  simdFuncName = getSimdFuncName(funcName) if len(esFunction.stagesNames) > 0 else "nullptr"
  eventHandlers_str = ", {eventHandlers}".format(eventHandlers=getEventHandlerFuncName(funcName)) if len(esFunction.eventHandlers) > 0 else ""
# eventmask_str = 'ecs::EventSetBuilder<%s>::build()' % (('%s_EVENT_SET' % esFunction.funcName.upper()) if esFunction.eventHandlers else '')
  eventListStr = ''
  if (len(esFunction.eventList) > 0):
    eventListStr = ',\n                       '.join(esFunction.eventList)
  eventmask_str = 'ecs::EventSetBuilder<%s>::build()' % eventListStr

  non_optional_ro_params = non_optional_param_names(esFunction.ro_params)
  non_optional_rw_params = non_optional_param_names(esFunction.rw_params)
  already_printed = False

  def_quant_code = get_annotated_quant(esFunction.annotations)
  def_quant_code, already_printed = add_quant(def_quant_code, already_printed)
  after_list_str, already_printed = add_tag_string(esFunction.annotatedAfter, already_printed)
  before_list_str, already_printed = add_tag_string(esFunction.annotatedBefore, already_printed)
  track_list_str, already_printed = add_tag_string(esFunction.annotatedTracked, already_printed)
  tags_list_str, already_printed = add_tag_string(esFunction.annotatedTags, already_printed)

  genCode = '''static ecs::EntitySystemDesc {funcName}_es_desc
(
  "{semanticFuncName}",
  "{moduleName}",
  ecs::EntitySystemOps({simdFuncName}{eventHandlers_str}),
  {rw_params_str},
  {ro_params_str},
  {rq_params_str},
  {no_params_str},
  {eventmask_str},
  {stages}
{tags_list_str}{track_list_str}{before_list_str}{after_list_str}{def_quant_code});
'''.format(**locals())
  return genCode

def gen_getter(esFunction, i):
  compsName = getCompsName(esFunction.funcName, '')
  if i['name'] in reserved_components_name:
    return reserved_components_name[i['name']]

  suffix = 'RW' if i['param_type'] == 'rw' else 'RO'
  if (i['name'] not in esFunction.params_array['ro']):
    suffix = 'RW'
  ecs_getter = 'ECS_' + suffix + '_COMP'
  if (i['optional'] == 'ptr') or (i['type'] == 'const_string'):
    ecs_getter += '_PTR'
  elif (i['optional'] == 'val'):
    ecs_getter += '_OR'
  name = i['name'].replace("_dot_", dot_suffix)
  ecs_getter += '(' + compsName + ', "' + name + '", '
  if (i['type'] == 'const_string'):
    if ('defVal' in i):
      print("const_string doesn't support default value")
      exit(1)
    ecs_getter += 'const char'
  elif ('defVal' in i):
    ecs_getter += i['type'] + '(' + i['defVal'] + ')'
  else:
    ecs_getter += i['type']
  ecs_getter += ')'
  return ecs_getter


def gen_call_params(esFunction, indent, call_params, firstComma=', '):
  genCode = ''
  for i in call_params:
    genCode += indent + firstComma + gen_getter(esFunction, i) + '\n'
    firstComma = ', '
  genCode += indent + ');\n'
  return genCode


def infoCast(stageType):
  if (stageType != 'ecs::UpdateStageInfo'):
    return '*info.cast<' + stageType + '>()'
  else:
    return 'info'


def gen_condition(esFunction, conditions):
  secondCondition = False
  expression = ''
  for i in conditions:
    expression += (' && ' if secondCondition else '') + ('!' if i['flag_type'] == 'eastl::false_type' else '') + gen_getter(esFunction, i)
    secondCondition = True
  return expression


def gen_es_simd(esFunction):
  if (len(esFunction.stagesNames) < 1):
    return ''
  simdFuncName = getSimdFuncName(esFunction.funcName)
  genCode = 'static void ' + simdFuncName + '(const ecs::UpdateStageInfo &__restrict info, const ecs::QueryView & __restrict components)\n{\n'
  indent = '  '
  if not esFunction.hasComponents:
    genCode += '  G_UNUSED(components);\n'
  for index in range(0, len(esFunction.stagesTypes)):
    if (len(esFunction.stagesTypes) > 1):
      genCode += indent + ('else ' if index != 0 else '') + 'if (info.stage == ' + esFunction.stagesNames[index] + ')\n'
      genCode += indent + '{\n'
      indent2 = indent + '  '
    else:
      indent2 = indent
    infoCasted = infoCast(esFunction.stagesTypes[index])
    if (esFunction.contextsIsOptional[index]):
      genCode += '''{indent2}if (!{funcName}_should_process({infoCasted}))
{indent2}  return;\n'''.format(indent2=indent2, funcName=esFunction.funcName, infoCasted=infoCasted)
    if (esFunction.contextsTypes[index] != ''):
#     genCode += indent2 + 'if (!ecs::optional_should_process<{context_type}>({infoCasted}))\n'.format(context_type = esFunction.contextsTypes[index], infoCasted = infoCasted)
#     genCode += indent2+'  return;\n'
      genCode += indent2
      genCode += esFunction.contextsTypes[index] + ' ctx({info});\n'.format(info=infoCasted)

    preBodyIndent = indent2
    preForIndent = indent2
    if esFunction.hasComponents:
      preForIndent = indent2
      genCode += indent2 + 'auto comp = components.begin(), compE = components.end(); G_ASSERT(comp!=compE);\n' + indent2 + 'do\n'
    if len(esFunction.condition):
      genCode += indent2 + '{\n'

    indent2 += '  '
    if len(esFunction.condition):
      genCode += indent2 + 'if ( !(' + gen_condition(esFunction, esFunction.condition) + ') )\n' + indent2 + '  continue;\n'

    genCode += indent2
    genCode += esFunction.funcName + '('
    genCode += infoCasted
    if (esFunction.contextsTypes[index] != ''):
      genCode += ", ctx"
    genCode += '\n' + gen_call_params(esFunction, indent2, esFunction.stagesCalls[index]) if len(esFunction.stagesCalls[index]) else ');\n'

    if len(esFunction.condition):
      genCode += preForIndent + '}\n'

    if esFunction.hasComponents:
      genCode += preForIndent + 'while (++comp != compE);\n'

    if (len(esFunction.stagesTypes) > 1):
      genCode += indent + '}\n'
  genCode += '}\n'
  return genCode


def gen_es_event_handler(esFunction):
  global generic_event_type
  if (len(esFunction.eventHandlers) < 1):
    return ''
#  if (len(esFunction.eventHandlers) > 1):
#  print ("Warning: currently ECS codegen supports only one event handler, because Events are untyped!")
  funcName = esFunction.eventHandlers[0].funcName
  mbConst = '' if any([eh.stage_param['mutable'] for eh in esFunction.eventHandlers]) else 'const '
  simdFuncName = getEventHandlerFuncName(esFunction.funcName)

  genCode = 'static void {eventHandlerFuncName}({mbConst}{generic_event_type} &__restrict evt, const ecs::QueryView &__restrict components)\n{{\n'.\
            format(eventHandlerFuncName=simdFuncName, mbConst=mbConst, generic_event_type=generic_event_type)
  hasGenericEvent = False
  if len([x for x in esFunction.eventHandlers if len(x.call_params) != 0]) == 0:
    genCode += '  G_UNUSED(components);\n'

  indent = '  ' if len(esFunction.eventHandlers) > 1 else ''
  for index in range(0, len(esFunction.eventHandlers)):
    eventType = esFunction.eventHandlers[index].stage_param['type']
    curFuncName = esFunction.eventHandlers[index].fullFuncName
    elseStatement = '} else ' if index != 0 else ''
    event = '*event' if eventType != generic_event_type else 'evt'
    if eventType != generic_event_type:
      event = 'static_cast<{mbConst}{eventType}&>(evt)'.format(**locals())
    call_indent = indent + '    ' + ('  ' if len(esFunction.condition) else '')
    call_ = gen_call_params(esFunction, call_indent, esFunction.eventHandlers[index].call_params)
    if (eventType != 'ecs::Event'):
      if len(esFunction.eventHandlers) != 1:
        indent = '  '
        genCode += '{elseStatement}if (evt.is<{eventType}>()) {{\n'.format(**locals())
      else:
        genCode += '  G_FAST_ASSERT(evt.is<{eventType}>());\n'.format(**locals())
    else:
      assert hasGenericEvent is False, '{funcName} already has generic (ecs::Event &) overload '.format(funcName=funcName)
      if (hasGenericEvent):
        continue
      hasGenericEvent = True
      genCode += '} else {\n' if index != 0 else ''
    if len(esFunction.eventHandlers[index].condition):
      condition = gen_condition(esFunction, esFunction.eventHandlers[index].condition)
      genCode += '''  {indent}auto comp = components.begin(), compE = components.end(); G_ASSERT(comp!=compE); do
  {indent}{{
    {indent}if ( !({condition}) )
      {indent}continue;
    {indent}{curFuncName}({event}
    {indent}{call_}  {indent}}}{indent} while (++comp != compE);\n{indent}'''.format(**locals())
    elif esFunction.eventHandlers[index].hasComponents: #esFunction.hasComponents:
      genCode += '''  {indent}auto comp = components.begin(), compE = components.end(); G_ASSERT(comp!=compE); do
    {indent}{curFuncName}({event}
    {indent}{call_}{indent}  while (++comp != compE);\n{indent}'''.format(**locals())
    else:
      genCode += '''  {indent}{curFuncName}({event}
    {indent}{call_}'''.format(**locals())

  if len(esFunction.eventHandlers) != 1:
    genCode += '  }} else {{G_ASSERTF(0, "Unexpected event type <%s> in {funcName}", evt.getName());}}\n'.format(funcName=funcName) if (not hasGenericEvent) else '}\n'
  genCode += '}\n'
  return genCode


def get_annotated_quant(functionDeclAnnotation):
  token = '@can_parallel_for'
  return next(iter([anot[anot.find(token) + len(token) + 1:] for anot in functionDeclAnnotation if token in anot]), '')


def gen_ecs_query_desc(esFunction, rw_params_str, ro_params_str, rq_params_str, no_params_str):
  funcName = esFunction.funcName
  def_quant_code = get_annotated_quant(esFunction.functionDeclAnnotation)
  def_quant_code = '\n  , ' + def_quant_code if def_quant_code != '' else ''
  genCode = '''static ecs::CompileTimeQueryDesc {funcName}_desc
(
  "{esFunction.fullFuncName}",
  {rw_params_str},
  {ro_params_str},
  {rq_params_str},
  {no_params_str}{def_quant_code});
'''.format(**locals())
  return genCode


def gen_ecs_query_simd(esFunction):
  funcName = esFunction.funcName
  fullFuncName = esFunction.fullFuncName
  def_quant_code = get_annotated_quant(esFunction.functionDeclAnnotation)

  call_ = gen_call_params(esFunction, '            ', esFunction.call_params, '  ')
  eid_arg = 'ecs::EntityId eid, ' if not esFunction.eidArgId == -1 else ''
  mgr_arg = (esFunction.mgrType + ' &manager, ') if not esFunction.mgrArgId == -1 else ''
  first_arg = mgr_arg if esFunction.mgrArgId < esFunction.eidArgId else eid_arg
  second_arg = eid_arg if esFunction.mgrArgId < esFunction.eidArgId else mgr_arg
  manager_name = '&manager' if not esFunction.mgrArgId == -1 else 'g_entity_mgr'
  continue_expr = 'return' if esFunction.is_eid_query else 'continue'
  eid_pass = ' eid,' if esFunction.is_eid_query else ''
  loop_expr = 'auto comp = components.begin(), compE = components.end(); G_ASSERT(comp != compE); do' if not esFunction.is_eid_query else 'constexpr size_t comp = 0;'
  end_loop_expr = 'while (++comp != compE);' if not esFunction.is_eid_query else ''
  parallelForCode = '  , nullptr, {funcName}_desc.getQuant()'.format(**locals()) if def_quant_code != '' and not esFunction.is_eid_query else ''
  if len(esFunction.condition):
    condition = gen_condition(esFunction, esFunction.condition)
    func_call = 'function(\n{call_}'.format(**locals())
    if esFunction.breakable:
      callInCond = call_.replace(");\n", "")
      func_call = '''if (function(\n{callInCond}) == ecs::QueryCbResult::Stop)
            return ecs::QueryCbResult::Stop;'''.format(**locals())
    callCode = '''
        {loop_expr}
        {{
          if ( !({condition}) )
            {continue_expr};
          {func_call}
        }}{end_loop_expr}
      '''.format(**locals())
    if esFunction.breakable:
      callCode = callCode + "  return ecs::QueryCbResult::Continue;\n    "
  elif esFunction.breakable:
    callInCond = call_.replace(");\n", "")
    callCode = '''
        {loop_expr}
        {{
          if (function(\n{callInCond}) == ecs::QueryCbResult::Stop)
            return ecs::QueryCbResult::Stop;
        }}{end_loop_expr}
      '''.format(**locals())
    callCode = callCode + "    return ecs::QueryCbResult::Continue;\n    "
  else:
    callCode = '''
        {loop_expr}
        {{
          function(\n{call_}
        }}{end_loop_expr}
    '''.format(**locals())

  genCode = '''template<typename Callable>
inline {result_type} {fullFuncName}({first_arg}{second_arg}Callable function)
{{
  {return_statement}perform_query({manager_name},{eid_pass} {funcName}_desc.getHandle(),
    [&function](const ecs::QueryView& __restrict components)
    {{{callCode}}}
  {parallelForCode});
}}
'''.format(result_type=esFunction.result_type, return_statement='' if esFunction.result_type == 'void' else 'return ', **locals())
  return genCode


def param_equal(a, b):
  if (a["name"] != b["name"]) or (a["type"] != b["type"]) or (a["optional"] != b["optional"]):
    return False
  return True


# todo: remake to union
def combine_params(final_params, from_params):
  for i in from_params:
    found = False
    for j in final_params:
      if param_equal(i, j):
        found = True
        break
    if (found is False):
      final_params.append(i)


def list_of_names(srcParams, combineNames):
  for ai in srcParams:
    combineNames.add(ai["name"])


def build_list_of_param(fun):
  all_params = {"has": set(), "not": set()}
  list_of_names(fun.ro_params, all_params["has"])
  list_of_names(fun.rw_params, all_params["has"])
  list_of_names(fun.rq_params, all_params["has"])
  list_of_names(fun.no_params, all_params["not"])
  return all_params


def compare_params(srcParams, addParams, srcName, addName, name):
  for ai in srcParams:
    for bi in addParams:
      if ai["name"] == bi["name"]:
        if (not param_equal(ai, bi)):
          print("Two functions : ('" + srcName + "', '" + addName + "') with conflicting set of " + name + " params, at param<" + ai["name"] + ">")
          print(ai)
          print(bi)
          exit(1)
  return False


def check_tags(srcFunction, addFunction, src, add, name):
  if src != add:
    srcTags = ",".join(src)
    addTags = ",".join(add)
    print("Two functions : ('{srcFunction.funcName}' and '{addFunction.funcName}') with conflicting set of {name} '{srcTags}' and '{addTags}'".format(**locals()))
    exit(1)


def combine_two_func(srcFunction, addFunction):
  check_tags(srcFunction, addFunction, srcFunction.annotatedTags, addFunction.annotatedTags, "tags")
  check_tags(srcFunction, addFunction, srcFunction.annotatedAfter, addFunction.annotatedAfter, "after")
  check_tags(srcFunction, addFunction, srcFunction.annotatedBefore, addFunction.annotatedBefore, "before")
  compare_params(srcFunction.ro_params, addFunction.ro_params, srcFunction.funcName, addFunction.funcName, "RO")
  for t in addFunction.annotatedTracked:
    if t not in srcFunction.annotatedTracked:
      srcFunction.annotatedTracked.append(t)
  compare_params(srcFunction.rw_params, addFunction.ro_params, srcFunction.funcName, addFunction.funcName, "RW")
  compare_params(srcFunction.rq_params, addFunction.rq_params, srcFunction.funcName, addFunction.funcName, "RQ")
  compare_params(srcFunction.no_params, addFunction.no_params, srcFunction.funcName, addFunction.funcName, "NO")
  srcFunction.list_of_params.add((non_optional_param_names(addFunction.ro_params), non_optional_param_names(addFunction.rw_params),))

  combine_params(srcFunction.ro_params, addFunction.ro_params)
  combine_params(srcFunction.rw_params, addFunction.rw_params)
  combine_params(srcFunction.rq_params, addFunction.rq_params)
  combine_params(srcFunction.no_params, addFunction.no_params)
  addFunction.baseFuncName = srcFunction.funcName
  addFunction.combined = True


def combine_es(srcFunction, addFunction):
  combine_two_func(srcFunction, addFunction)
#   if (first_different_element_in_list(srcFunction.call_params, addFunction.call_params) != -1):
#     print ("You have declared two _es functions : ('" +srcFunction.funcName + "', '" + addFunction.funcName + "') with different params call order")
#     print(srcFunction.call_params)
#     print(addFunction.call_params)
#     exit(1)
  srcFunction.stagesCalls.append(addFunction.call_params)
  srcFunction.stagesNames.append(getStageName(addFunction))
  srcFunction.stagesTypes.append(addFunction.stage_param['type'])
  if len(addFunction.context_param) > 1:
    srcFunction.contextsTypes.append(addFunction.context_param['type'])
  else:
    srcFunction.contextsTypes.append('')
  srcFunction.contextsIsOptional.append(addFunction.isOptional)


def combine_functions(allESFunctions):
  for i in range(0, len(allESFunctions)):
    if (allESFunctions[i].funcName == ''):
      continue

    allESFunctions[i].list_of_params.add((non_optional_param_names(allESFunctions[i].ro_params), non_optional_param_names(allESFunctions[i].rw_params),))
    allESFunctions[i].stagesNames = [getStageName(allESFunctions[i])]
    allESFunctions[i].stagesTypes = [allESFunctions[i].stage_param['type']]
    if len(allESFunctions[i].context_param) > 1:
      allESFunctions[i].contextsTypes = [allESFunctions[i].context_param['type']]
    else:
      allESFunctions[i].contextsTypes = ['']
    allESFunctions[i].contextsIsOptional = [allESFunctions[i].isOptional]
    allESFunctions[i].stagesCalls = [allESFunctions[i].call_params]
    for j in range(i + 1, len(allESFunctions)):
      if (allESFunctions[i].funcName == allESFunctions[j].funcName):
        combine_es(allESFunctions[i], allESFunctions[j])
        allESFunctions[j].funcName = ''

    if(len(allESFunctions[i].stagesTypes) > 1):
      if ('ecs::UpdateStageInfo' in allESFunctions[i].stagesTypes):
        print('on of functions "' + allESFunctions[i].funcName + '" uses base ecs::UpdateStageInfo staging type')
        print('You can have only one such function')
        exit(1)

    if (len(allESFunctions[i].stagesNames) != len(allESFunctions[i].stagesTypes)):
      print("stages have differnet number of stage Types and stage Names")
      print(allESFunctions[i].stagesTypes)
      print(allESFunctions[i].stagesNames)
      exit(1)


def combine_es_and_event_functions(esFunction, allEventFunctions):
  global event_handler_suffix
  global generic_event_type
  typedEventList = []
  genericEventList = []
  for j in range(0, len(allEventFunctions)):
    if not allEventFunctions[j].combined and esFunction.funcName == strip_end(allEventFunctions[j].funcName, event_handler_suffix):
      combine_two_func(esFunction, allEventFunctions[j])
      if allEventFunctions[j].stage_param['type'] == generic_event_type:
        esFunction.eventHandlers.append(allEventFunctions[j])
        if len(allEventFunctions[j].annotatedEvents) > 0:
          genericEventList = genericEventList + allEventFunctions[j].annotatedEvents
      else:
        typedEventList.append(allEventFunctions[j].stage_param['type'])
        esFunction.eventHandlers.insert(0, allEventFunctions[j])

  esFunction.eventList = typedEventList + genericEventList
# todo just append one list to another?


def validate_function_params(name, params):
  for param in params:
    assert param['name'] != '', "es_Function {name} has parameter of type {type} without name".format(name=name, type=param['type'])

# fixme: we'd better parse is_boxed_type in ecs parser, since we now that in componenttypeinfo
# todo:


def is_upgradable_type(ro_type, rw_type):
  return ro_type['type'] == rw_type['type']


# for faster search
def create_cached_params(esFunction, params):
  for param in params:
    esFunction.params_array[param['param_type']][param['name']] = param


def get_es_desc(esFunction, ret):
  for index_ro_param in range(0, len(esFunction.ro_params)):
    ro_param = esFunction.ro_params[index_ro_param]
    for rw_param in esFunction.rw_params:
      if (ro_param['name'] == rw_param['name'] and is_upgradable_type(ro_param, rw_param)):
        for ev in esFunction.eventList:
          if (ev.endswith("EventComponentChanged")):
            print("upgrading RO component <" + ro_param['name'] + "> in : " + esFunction.funcName + "is required, but that could cause code malfunction")
            print("as subscription only work on RO components.")
            print("Rename es <" + esFunction.funcName + "> event_handler for EventComponentChanged to something else")
            exit(1)
            break
        esFunction.ro_params[index_ro_param]['param_type'] = 'rw'
        break
  create_cached_params(esFunction, esFunction.rw_params)
  create_cached_params(esFunction, esFunction.ro_params)
  create_cached_params(esFunction, esFunction.rq_params)
  create_cached_params(esFunction, esFunction.no_params)
  validate_function_params(esFunction.funcName, esFunction.rw_params)
  validate_function_params(esFunction.funcName, esFunction.ro_params)
  validate_function_params(esFunction.funcName, esFunction.rq_params)
  validate_function_params(esFunction.funcName, esFunction.no_params)
  filter_params_list(esFunction)
  ret['comps_desc_name'] = getCompsName(esFunction.funcName, '')
  componentsList = []
  rwStart, rwCount = gen_params_list(esFunction.rw_params, "rw", componentsList)
  roStart, roCount = gen_params_list(esFunction.ro_params, "ro", componentsList)
  rqStart, rqCount = gen_params_list(esFunction.rq_params, "rq", componentsList)
  noStart, noCount = gen_params_list(esFunction.no_params, "no", componentsList)
  if len(componentsList) > 0:
    genCode = "static constexpr ecs::ComponentDesc " + ret['comps_desc_name'] + "[] ="
    genCode += "\n{\n"
    genCode += ",\n".join(componentsList)
    genCode += '\n};\n'
  else:
    genCode = "//static constexpr ecs::ComponentDesc " + ret['comps_desc_name'] + "[] ={};\n"
  ret['comps_desc_code'] = genCode

  ret['rw_params_str'] = ('make_span({name}+{rwStart}, {rwCount})/*rw*/' if rwCount > 0 else 'empty_span()').format(name=ret['comps_desc_name'], **locals())
  ret['ro_params_str'] = ('make_span({name}+{roStart}, {roCount})/*ro*/' if roCount > 0 else 'empty_span()').format(name=ret['comps_desc_name'], **locals())
  ret['rq_params_str'] = ('make_span({name}+{rqStart}, {rqCount})/*rq*/' if rqCount > 0 else 'empty_span()').format(name=ret['comps_desc_name'], **locals())
  ret['no_params_str'] = ('make_span({name}+{noStart}, {noCount})/*no*/' if noCount > 0 else 'empty_span()').format(name=ret['comps_desc_name'], **locals())


allESFunctions = []
allEventFunctions = []
allQueryFunctions = []


def gen_es(allParsedFunctions, event_suffix, input_file_name):
  global event_handler_suffix
  global allESFunctions, allEventFunctions, allQueryFunctions
  global global_input_file_name
  global_input_file_name = input_file_name
  event_handler_suffix = event_suffix
  for i in range(0, len(allParsedFunctions)):
    if allParsedFunctions[i].funcName.endswith("_es") or allParsedFunctions[i].funcName.endswith("_es" + event_handler_suffix):
      esFun = es_function_from_parsed_function(allParsedFunctions[i])
      if allParsedFunctions[i].isEvent or\
         len(esFun.annotatedEvents) > 0 or\
         len(esFun.annotatedTracked) > 0: #todo: or type inherits from ecs::Event
        allEventFunctions.append(esFun)
      else:
        allESFunctions.append(esFun)
    elif (allParsedFunctions[i].funcName.endswith(ecs_query_suffix)):
      found = False
      for j in range(0, len(allQueryFunctions)):
        if (allQueryFunctions[j].funcName == allParsedFunctions[i].funcName):
          esFun = es_function_from_parsed_function(allParsedFunctions[i])
          paramGroups = ["ro", "rw", "rq", "no"]
          for group in paramGroups:
            fullGroupName = group+"_params"
            oldParams = getattr(allQueryFunctions[j], fullGroupName)
            newParams = getattr(esFun, fullGroupName)
            if oldParams != newParams:
              print("query {} has inconsistent parameters for group \"{}\"".format(allQueryFunctions[j].funcName, group))
              for paramIndex, (p1, p2) in enumerate(zip(oldParams, newParams)):
                if p1 != p2:
                  print("parameter <{}>:\n{} {} (optional: {}) != {} {} (optional: {})".format(paramIndex, p1["type"], p1["name"], p1["optional"], p2["type"], p2["name"], p2["optional"]))
              exit(1)
          found = True
# todo: check for compatibility
          break
      if not found:
        allQueryFunctions.append(es_function_from_parsed_function(allParsedFunctions[i]))

  resultCode = ''

  if (len(allESFunctions) == 0 and len(allEventFunctions) == 0 and len(allQueryFunctions) == 0):
    print("WARNING! ES codegen hadn't found ANY entitySystem (*_es or *_ecs_query function) in file <{file}>".format(file=input_file_name))

  combine_functions(allESFunctions)
#   combine_functions(allEventFunctions)

#   for i in range(0, len(allESFunctions)):
#     if (allESFunctions[i].funcName == ''):
#       continue
#     combine_es_and_event_functions(allESFunctions[i], allEventFunctions)

  for i in range(0, len(allESFunctions)):
    allESFunctions[i].hasComponents = len([x for x in allESFunctions[i].stagesCalls if len([y for y in x if y['name'] not in reserved_components_name]) != 0]) != 0

  for i in range(0, len(allEventFunctions)):
    allEventFunctions[i].hasComponents = len([x for x in [allEventFunctions[i].call_params] if len([y for y in x if (y['name'] not in reserved_components_name)]) != 0]) != 0

  for i in range(0, len(allEventFunctions)):
    if (allEventFunctions[i].combined):
      continue
    emptyEsFunction = ESFunction(strip_end(allEventFunctions[i].funcName, event_handler_suffix))
    emptyEsFunction.stagesNames = []
    emptyEsFunction.stagesTypes = []
    emptyEsFunction.stagesCalls = [allEventFunctions[i].call_params]
    emptyEsFunction.condition = allEventFunctions[i].condition
    emptyEsFunction.annotatedTags = allEventFunctions[i].annotatedTags
    emptyEsFunction.annotatedBefore = allEventFunctions[i].annotatedBefore
    emptyEsFunction.annotatedAfter = allEventFunctions[i].annotatedAfter
    emptyEsFunction.annotatedTracked = allEventFunctions[i].annotatedTracked
    combine_es_and_event_functions(emptyEsFunction, allEventFunctions)
    emptyEsFunction.funcName = allEventFunctions[i].funcName
    allESFunctions.append(emptyEsFunction)

#  this is reporting of what was changed since we stop combining event handlers and update es (they even had different names!)
#
#  for i in range(0, len(allESFunctions)):
#    iParams = build_list_of_param(allESFunctions[i])
#    iFuncName = allESFunctions[i].funcName
#    for j in range(i+1, len(allESFunctions)):
#      jFuncName = allESFunctions[j].funcName
#      if strip_end(iFuncName, event_suffix) != strip_end(jFuncName, event_suffix):
#        continue
#      jParams = build_list_of_param(allESFunctions[j])
#      #if (len(allESFunctions[i].eventHandlers) != 0) != (len(allESFunctions[i].stagesTypes) != 0):
#      #  continue
#      if iParams['has'] != jParams['has']:
#        jAdditions = ",".join(jParams['has']-iParams['has'])
#        iAdditions = ",".join(iParams['has']-jParams['has'])
#        common = ",".join(iParams['has'].intersection(jParams['has']))
#        print('''{input_file_name}: list of required params for ES {iFuncName} and {jFuncName} differs:
#  {iFuncName} requires unique: <{iAdditions}>,
#  {jFuncName} requires unique: <{jAdditions}>
#  common: <{common}>'''.format(**locals()))
#      if iParams['not'] != jParams['not']:
#        jAdditions = ",".join(jParams['not']-iParams['not'])
#        iAdditions = ",".join(iParams['not']-jParams['not'])
#        common = ",".join(iParams['not'].intersection(jParams['not']))
#        print('''{input_file_name}: list of required_not params for ES {iFuncName} and {jFuncName} differs:
#  {iFuncName} requires NOT unique: <{iAdditions}>,
#  {jFuncName} requires NOT unique: <{jAdditions}>
#  common: <{common}>'''.format(**locals()))
#
#

  resultCode = '#include <daECS/core/internal/performQuery.h>\n'

  for i in range(0, len(allESFunctions)):
    if (allESFunctions[i].funcName == ''):
      continue
    ro_rw_desc = {}
    get_es_desc(allESFunctions[i], ro_rw_desc)

    if (len(allESFunctions[i].stagesNames) > 1):
      stagesCode = "\n  | ".join([getStageMask(x) for x in allESFunctions[i].stagesNames])
    elif (len(allESFunctions[i].stagesNames) > 0):
      stagesCode = getStageMask(allESFunctions[i].stagesNames[0])
    else:
      stagesCode = '0'

    if len(ro_rw_desc['comps_desc_code']) > 0:
      resultCode += ro_rw_desc['comps_desc_code']

    resultCode += gen_es_simd(allESFunctions[i])
    resultCode += gen_es_event_handler(allESFunctions[i])
    resultCode += gen_es_desc(allESFunctions[i], strip_start_all(input_file_name, "../"), stagesCode, ro_rw_desc['rw_params_str'], ro_rw_desc['ro_params_str'], ro_rw_desc['rq_params_str'], ro_rw_desc['no_params_str'])

  for i in range(0, len(allQueryFunctions)):
    ro_rw_desc = {}
    get_es_desc(allQueryFunctions[i], ro_rw_desc)
    resultCode += ro_rw_desc['comps_desc_code']
    # resultCode += gen_query_simd(allQueryFunctions[i])
    resultCode += gen_ecs_query_desc(allQueryFunctions[i], ro_rw_desc['rw_params_str'], ro_rw_desc['ro_params_str'], ro_rw_desc['rq_params_str'], ro_rw_desc['no_params_str'])
    resultCode += gen_ecs_query_simd(allQueryFunctions[i])

  return resultCode
