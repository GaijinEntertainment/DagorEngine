#!/usr/bin/env python3

import clang.cindex
import cymbal
import re
import sys
import os
import subprocess
import ctypes

# Alt enum for versions >= 15.0 (https://github.com/llvm/llvm-project/blame/f42136d4d6e102659c6a4a870349aae6425a3ba9/clang/bindings/python/clang/cindex.py#L1320)
try:
  clang.cindex.CursorKind.TRANSLATION_UNIT_350 = clang.cindex.CursorKind(350)
except ValueError as e: # newer bindings
  if e.args[0].find("already loaded") >= 0: # <class 'clang.cindex.CursorKind'> value 350 already loaded
    clang.cindex.CursorKind.TRANSLATION_UNIT_350 = clang.cindex.CursorKind.TRANSLATION_UNIT
  else:
    raise

if sys.platform == "win32":  # disable windows message boxes within assertion handlers in libclang.dll
  SEM_FAILCRITICALERRORS = 0x0001
  SEM_NOGPFAULTERRORBOX = 0x0002
  ctypes.windll.kernel32.SetErrorMode(ctypes.c_uint(SEM_FAILCRITICALERRORS | SEM_NOGPFAULTERRORBOX))
  _OUT_TO_STDERR = 1
  ctypes.cdll.msvcrt._set_error_mode(ctypes.c_int32(_OUT_TO_STDERR))

class ClangParseFatal(Exception):
  "Clang Parser Diagnostics Fatal Level"
  def __init__(self, diagnostics):
    self.message = diagnostics.location
    super().__init__(self.message)

class ClangParseNoInput(Exception):
  "Clang Parser No Input"
  pass

class ParsedFunction:
  def __init__(self, name, full_name):
    self.call_params = []
    self.funcName = name
    self.funcFullName = full_name
    self.annotations = []
    self.functionDeclAnnotation = []
    self.result_type = 'void'
    self.cb_result_type = 'void'
    self.is_eid_query = False
    self.eidArgId = -1
    self.mgrArgId = -1
    self.mgrType = 'ecs::EntityManager'
    self.isEventType = False


def sorted_by_loc(nodes):
  return sorted(nodes, key=lambda x: x.location.offset)


def sorted_by_name(nodes):
  return sorted(nodes, key=lambda x: x.spelling)


def is_mut_ref(arg_type):
    if arg_type.kind in [clang.cindex.TypeKind.POINTER, clang.cindex.TypeKind.LVALUEREFERENCE,
                         clang.cindex.TypeKind.INCOMPLETEARRAY, clang.cindex.TypeKind.CONSTANTARRAY]:

        if arg_type.kind in [clang.cindex.TypeKind.POINTER, clang.cindex.TypeKind.LVALUEREFERENCE]:
            pointee_type = arg_type.get_pointee()
        else:
            pointee_type = arg_type.get_array_element_type()

        # print("pointee_type.kind: {}".format(pointee_type.kind))
        # print("pointee_type.is_const_qualified(): {}".format(pointee_type.is_const_qualified()))

        if not pointee_type.is_const_qualified():
            return True

    return False


def strip_end(text, suffix):
    if not text.endswith(suffix):
        return text
    return text[:len(text) - len(suffix)]


def strip_start(text, prefix):
    if not text.startswith(prefix):
        return text
    return text[len(prefix):]


def remove_quotes(text):
    return strip_end(strip_start(text, '"'), '"')


def get_param_is_optional_initializer(arg):
    if arg.type.kind == clang.cindex.TypeKind.POINTER:
        return 'NULL'
    hasDefault = False
    initializer = ''
    prevOffset = None
    for c in arg.get_tokens():
        # print c.spelling
        if (hasDefault):
            if not (prevOffset is None):
                if (prevOffset != c.extent.start.offset):
                    initializer += ' '
            prevOffset = c.extent.end.offset
            initializer += c.spelling
        elif (c.spelling == '='):
            hasDefault = True

    if (hasDefault):
        return initializer
    return


def get_param_base_type(arg_type):
    if arg_type.kind in [clang.cindex.TypeKind.POINTER, clang.cindex.TypeKind.LVALUEREFERENCE,
                         clang.cindex.TypeKind.INCOMPLETEARRAY, clang.cindex.TypeKind.CONSTANTARRAY]:

        if arg_type.kind in [clang.cindex.TypeKind.POINTER, clang.cindex.TypeKind.LVALUEREFERENCE]:
            pointee_type = arg_type.get_pointee()
        else:
            pointee_type = arg_type.get_array_element_type()

        return pointee_type
    return arg_type


def is_template(node):
    return hasattr(node, 'type') and node.type.get_num_template_arguments() != -1


def add_attribute_component_type(node, attrTypes):
    if node.kind != clang.cindex.CursorKind.STRUCT_DECL or not is_template(node) or node.spelling != 'ComponentTypeInfo':  # node.kind != clang.cindex.CursorKind.CLASS_TEMPLATE or
        return None

    # print node.kind
    # print node.type.get_template_argument_type(0).spelling
    # return node.type.get_template_argument_type(0).spelling
    for c in node.get_children():
        if (c.kind == clang.cindex.CursorKind.VAR_DECL and c.displayname == 'isAttributeType'):
           attrTypes[node.type.get_template_argument_type(0).spelling] = node.type.get_template_argument_type(0).get_size()
           return


def fully_qualified(c):
    if c is None:
        return ''
    elif c.kind in (clang.cindex.CursorKind.TRANSLATION_UNIT_350, clang.cindex.CursorKind.TRANSLATION_UNIT):
        return ''
    else:
        res = fully_qualified(c.semantic_parent)
        if res != '':
            return res + '::' + c.spelling
    return c.spelling

def is_event_type(node):
    generic_event_type = "ecs::Event"
    if node.type.kind == clang.cindex.TypeKind.LVALUEREFERENCE:
        pointee_type = node.type.get_pointee()
        decl = pointee_type.get_declaration()
        if decl.type.spelling == generic_event_type:
            return True
        if any(arg.kind == clang.cindex.CursorKind.CXX_BASE_SPECIFIER and arg.spelling == generic_event_type for arg in decl.get_children()):
            return True
    return False

def get_params_info(node, call_params):
    assert node.kind == clang.cindex.CursorKind.PARM_DECL
    # for c in node.get_children():
    #    get_params_info(c)
    # type
    # if ((node.kind == clang.cindex.CursorKind.TYPE_REF)):
    #    print (node.type.kind + ' ')
    #    print ('type: <' + node.spelling+'> ')
    if ((node.kind == clang.cindex.CursorKind.PARM_DECL)):
        call_param = {'name': node.displayname}
        # if (node.type.get_canonical().is_const_qualified()):
        #    print 'CONST'
        # if (node.kind.is_reference()):
        #    print 'ref'
        # print (node.type.get_ref_qualifier())#full type name
        call_param['mutable'] = True if (is_mut_ref(node.type)) else False
        # print (node.type.get_canonical().spelling)#full type name
        # print (get_param_base_type(node.type).get_canonical().spelling)#full type name
        # print (get_param_base_type(node.type).spelling)#full type name
        # print (node.type.spelling)#full type name
        #if node.type.get_declaration().kind == clang.cindex.CursorKind.CLASS_DECL or node.type.get_declaration().kind == clang.cindex.CursorKind.STRUCT_DECL:
              #get_attribute_info(c, attributeTypes)
        #if node.type.get_declaration().kind == clang.cindex.TypeKind.LVALUEREFERENCE:
        #    pointee_type = node.type.get_pointee()
        #    print(node.displayname)
        #    print(pointee_type.get_declaration().spelling)
        paramType = get_param_base_type(node.type)
        call_param['type'] = paramType.spelling
        call_param['type_size'] = paramType.get_size()
        # print("type " +paramType.spelling+ " templates"+str(paramType.get_num_template_arguments()))
        if (paramType.get_num_template_arguments() != -1 and re.match(r".*Attribute.*(RW|RO|T).*", paramType.spelling) is not None):
            call_param['AttributeValType'] = paramType.get_template_argument_type(0).spelling
            call_param['AttributeValTypeSize'] = paramType.get_template_argument_type(0).get_size()
        else:
            call_param['AttributeValType'] = None
            call_param['AttributeValTypeSize'] = -10
            # print "template of " + node.type.get_template_argument_type(0).spelling

        # print ('name:' + node.displayname)#node.displayname ?
        val = get_param_is_optional_initializer(node)
        if (val is None):
            call_param['optional'] = False
        else:
            call_param['optional'] = val
        call_params.append(call_param)
        # print node.is_copy_constructor()
        # print node.access_specifier


def parse_lambda_query_function(node, call_params):
  for arg in sorted_by_loc(node.get_children()):
    if (arg.kind == clang.cindex.CursorKind.PARM_DECL):
      get_params_info(arg, call_params)
#   print arg.kind
#   print arg.spelling
#   parse_lambda_query_function(arg)
#   for arg in node.get_arguments():
#     get_params_info(arg)

lambda_call_expressions = [
    clang.cindex.CursorKind.LAMBDA_EXPR,
    clang.cindex.CursorKind.OBJ_BOOL_LITERAL_EXPR # For some reason clang on python3 returns this kind for lambda expression. It is a dirty workaround, but it works
]


def get_annotation(node):
  annotations = []
  for arg in sorted_by_name(node.get_children()):
      if (arg.kind == clang.cindex.CursorKind.ANNOTATE_ATTR):
          annotations.append(arg.spelling)
      elif (arg.kind not in lambda_call_expressions):
          annotations += get_annotation(arg)
  return annotations


def get_lambda_return_type(node):
    decl = node.type.get_declaration()
    for child in decl.get_children():
        if child.kind == clang.cindex.CursorKind.CXX_METHOD:
            return child.result_type
    return None


def parse_query_function(node, parsed_function):
  for arg in node.get_children():
      if (arg.kind in lambda_call_expressions):
          parse_lambda_query_function(arg, parsed_function.call_params)
          parsed_function.annotations += get_annotation(arg)
          parsed_function.cb_result_type = get_lambda_return_type(arg).spelling
      else:
          parse_query_function(arg, parsed_function)


def get_attribute_info(node, attributeTypes):
    add_attribute_component_type(node, attributeTypes)
    for c in node.get_children():
        get_attribute_info(c, attributeTypes)


def find_func_decl_annotation(node):
    ret = []
    if (node.kind == clang.cindex.CursorKind.ANNOTATE_ATTR):
        ret.append(node.displayname)
    for c in sorted_by_name(node.get_children()):
        ret += find_func_decl_annotation(c)
    return ret


def get_function_info(node, file, is_es_name, all_functions, all_gets, queries_types, current_function):
    if (node.location.file is not None and node.location.file.name.find(file) != -1):
        if ((node.kind == clang.cindex.CursorKind.FUNCTION_DECL) and is_es_name(node.spelling)):
            # print ('function: <' + node.spelling+'>' + node.displayname)
            # get_params_info(node)
            sortedArgs = sorted_by_loc(node.get_arguments())
            if not sortedArgs:
                raise RuntimeError("ECS systems with no arguments are not supported!"
                                   " Please, add at least a single argument to {} "
                                   "(could be `ecs::Event&`, `ecs::EntityMgr&` or `ecs::EntityId)".format(node.spelling))
            current_function = node.spelling
            parsedFunction = ParsedFunction(node.spelling, fully_qualified(node))
            parsedFunction.isEvent = is_event_type(sortedArgs[0])
            for arg in sortedArgs:
                get_params_info(arg, parsedFunction.call_params)
            parsedFunction.annotations = get_annotation(node)
            all_functions.append(parsedFunction)

        if ((node.kind == clang.cindex.CursorKind.FUNCTION_TEMPLATE) and node.spelling.endswith('_ecs_query')):
            queries_types[node.spelling] = node.result_type.get_canonical().spelling

        if node.kind == clang.cindex.CursorKind.CALL_EXPR and node.spelling.endswith('_ecs_codegen_get_call'):
            args = list(node.get_arguments())
            typeName = list(list(node.get_children())[0].get_children())[0].result_type.get_pointee().spelling
            for c in args[1].walk_preorder():
                if (c.kind == clang.cindex.CursorKind.STRING_LITERAL):
                    typeName = strip_start(strip_end(c.spelling, '"'), '"')
                    break
            getterName = remove_quotes(list(args[0].get_children())[0].spelling)
            all_gets.append((getterName, {'type': typeName, 'file': node.location.file.name, 'line': node.location.line, 'fun': current_function}))
        if node.kind == clang.cindex.CursorKind.CALL_EXPR and node.spelling.endswith('_ecs_codegen_set_call'):
            args = list(node.get_arguments())
            typeName = args[1].type.get_pointee().spelling
            typeName = args[1].type.get_canonical().spelling if typeName == '' else typeName

            getterName = remove_quotes(list(args[0].get_children())[0].spelling)
            all_gets.append((getterName, {'type': typeName, 'file': node.location.file.name, 'line': node.location.line, 'fun': current_function}))
        if ((node.kind == clang.cindex.CursorKind.CALL_EXPR) and node.spelling.endswith('_ecs_query')):
            current_function = node.spelling
            parsedFunction = ParsedFunction(node.spelling, fully_qualified(node.referenced) if (node.referenced is not None) else fully_qualified(node))
            argsList = list(node.get_arguments())
            parsedFunction.eidArgId = next((i for i,v in enumerate(argsList) if v.type.get_canonical().spelling == 'ecs::EntityId'), -1)
            parsedFunction.mgrArgId = next((i for i,v in enumerate(argsList) if v.type.get_canonical().spelling.find('ecs::EntityManager') != -1), -1)
            if parsedFunction.mgrArgId != -1:
              parsedFunction.mgrType = argsList[parsedFunction.mgrArgId].type.get_canonical().spelling
            if parsedFunction.eidArgId != -1:
                parsedFunction.is_eid_query = True
            if node.referenced is not None:
                parsedFunction.functionDeclAnnotation = find_func_decl_annotation(node.referenced)
            if (node.spelling in queries_types):
                parsedFunction.result_type = queries_types[node.spelling]
            # print ('query function: <' + node.spelling+'>' + node.displayname)
            parse_query_function(node, parsedFunction)
            all_functions.append(parsedFunction)
            # print (parsedFunction.call_params)

    if (node.location.file is None or node.location.file.name.find(file) != -1):
        for c in node.get_children():
            get_function_info(c, file, is_es_name, all_functions, all_gets, queries_types, current_function)


def dump_obj(obj, level=0):
    for key, value in obj.__dict__.items():
        if not isinstance(value, object):
            print(" " * level + "%s -> %s" % (key, value))
        else:
            dump_obj(value, level + 2)


def get_diag_info(diag):
    return {'severity': diag.severity,
            'location': diag.location,
            'spelling': diag.spelling}


def get_clang_info():
    p = subprocess.Popen(['clang -x c++ -fsyntax-only -v -'], shell=True,
                         stdout=subprocess.PIPE,
                         stderr=subprocess.PIPE,
                         stdin=subprocess.PIPE)
    clangInfo = p.communicate()[1]
    if type(clangInfo) != str:
        clangInfo = clangInfo.decode("utf-8")
    clangInfo = clangInfo.split("\n")
    result = {"ResourceDir": "", "LibDir": ""}
    for infoLine in clangInfo:
        rd = infoLine.find("-resource-dir")
        if rd != -1:
            s = infoLine.find(" ", rd) + 1
            e = infoLine.find(" ", s)
            result["ResourceDir"] = infoLine[s:e]
        rd = infoLine.find("InstalledDir:")
        if rd != -1:
            s = infoLine.find(" ")
            if s != -1:
                installedDir = infoLine[s + 1:]
                libDir = os.path.join(os.path.dirname(installedDir), "lib64")
                if not os.path.exists(libDir):
                    libDir = os.path.join(os.path.dirname(installedDir), "lib")
                result["LibDir"] = libDir
    return result


def parse_ecs_functions(input_filename, search_filename, clang_args, should_parse_name, skipFunctionBodies, compiler_errors, all_gets):
    from clang.cindex import Index
    if os.environ.get('CLANG_LIBRARY_PATH', None):
      clang.cindex.conf.set_library_path(os.environ.get('CLANG_LIBRARY_PATH'))
    elif sys.platform.startswith('win'):
        assert sys.maxsize > 2**32, '64-bit python3 is required!'
        clanglibpath = os.path.join(os.environ['DAGOR_CLANG_DIR'], "bin")
        clang.cindex.conf.set_library_path(clanglibpath)
    else:
        clangInfo = get_clang_info()  # TODO: additional clang invocation is SLOW. Do something about it (use envvars?)
        libclang = 'libclang.%s' % ({'darwin':'dylib'}.get(sys.platform, 'so'))
        libclangpath = os.path.join(clangInfo["LibDir"], libclang)
        if not os.path.isfile(libclangpath): # on Ubuntu/Debian InstalledDir and libclang.so dir could be unrelated
            libclangpath = subprocess.check_output('clang --print-file-name %s' % libclang, shell=True)[:-1] # :-1 to cut trailing \n
        clang.cindex.conf.set_library_file(libclangpath)
        clang_args += ["-resource-dir", clangInfo["ResourceDir"]]

    try:
      cymbal.monkeypatch_type('get_template_argument_type',
                              'clang_Type_getTemplateArgumentAsType',
                              [clang.cindex.Type, ctypes.c_uint],
                              clang.cindex.Type)

      cymbal.monkeypatch_type('get_num_template_arguments',
                              'clang_Type_getNumTemplateArguments',
                              [clang.cindex.Type],
                              ctypes.c_int)
    except cymbal.clangext.CymbalException:
        pass

    from pprint import pprint
    index = Index.create()
    clang_args = ['-x', 'c++', input_filename] + clang_args
    # print clang_args
    options = clang.cindex.TranslationUnit.PARSE_SKIP_FUNCTION_BODIES if skipFunctionBodies else 0
    tu = index.parse(None, clang_args, None, options)
    if not tu:
        print("Fatal error: unable to load input")
        raise ClangParseNoInput
        return []
    for diag in tu.diagnostics:
        if (diag.severity >= clang.cindex.Diagnostic.Error):
            compiler_errors = compiler_errors + [{'severity': diag.severity, 'location': diag.location, 'spelling': diag.spelling}]
        if (diag.severity >= clang.cindex.Diagnostic.Fatal):
            print("Fatal error: can't parse file <{file}>".format(file=input_filename))
            print(diag.location)
            print(diag.spelling)
            raise ClangParseFatal(diag)
    all_functions = []
    # attributeTypes = {}
    queries_types = {}
    get_function_info(tu.cursor, search_filename, should_parse_name, all_functions, all_gets, queries_types, "")
    # if (len(all_functions)):
    #    get_attribute_info(tu.cursor, attributeTypes)
    #    for f in all_functions:
    #        #pprint(f.call_params)
    #        for param in f.call_params:
    #            param['isAttributeValType'] = (True if param['type'] in attributeTypes else False)

    try:
        getter_keys = set(list(zip(*all_gets))[0])

        def filter_pop(key):
            if key in getter_keys:
                getter_keys.remove(key)
                return True
            return False
        all_gets_unique = [x for x in reversed(all_gets) if filter_pop(x[0])]
        del all_gets[:]
        for v in all_gets_unique:
            all_gets.append(v)
        all_gets.sort(key=lambda x: x[0])
    except IndexError:
        pass
    return all_functions


'''
es_suffix = "_es"
context_suffix = "_with_context"
event_handler_suffix = "_event_handler"

def is_es_name(name):
   return True if (name.endswith(es_suffix) or name.endswith(es_suffix+context_suffix) or name.endswith(es_suffix+event_handler_suffix)) else False

def main():
    from optparse import OptionParser, OptionGroup
    global opts

    parser = OptionParser("usage: %prog [options] {filename} [clang-args*]")
    parser.disable_interspersed_args()
    (opts, args) = parser.parse_args()

    if len(args) == 0:
        parser.error('invalid number arguments')

    all_functions = parse_ecs_functions(args[0], args[1:], is_es_name)
    #allESFunctions = []
    for fi in all_functions:
        dump_obj(fi)

if __name__ == '__main__':
    main()
'''
