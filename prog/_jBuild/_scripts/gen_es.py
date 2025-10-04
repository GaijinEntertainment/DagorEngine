import sys
import io
import os
import difflib
import platform

from clang_ecs_parser import parse_ecs_functions
from gen_es30 import gen_es, remove_const_from_type, dot_suffix

version = "1.0"
es_suffix = "_es"
event_handler_suffix = "_event_handler"


# replace with regexp
def is_es_name(name):
   return name.endswith(es_suffix) \
       or name.endswith(es_suffix + event_handler_suffix)


if len(sys.argv) < 4:
  print("ERROR: expected 3 arguments.\nUsage: gen_es.py input output rel_path_for_include [CHECK]")
  sys.exit(1)

input_file_name = sys.argv[1]
output_file_name = sys.argv[2]
rel_path_for_include = sys.argv[3]
compiler_errors = []

# print sys.argv
clang_args = []
is64bit = False
isPs4 = False
isPs5 = False
clang_defines = ['-Dstricmp=strcasecmp',
                 '-D__forceinline=inline',
                 '-DSEH_DISABLED', '-D_WINSOCK_DEPRECATED_NO_WARNINGS',
                 '-D_HAS_CHAR16_T_LANGUAGE_SUPPORT', '-D_CRT_SECURE_NO_WARNINGS', '-D_CRTRESTRICT', '-D_CRTNOALIAS',
                 '-DNOMINMAX', '-D_MSC_EXTENSIONS']
# '-D__int64="long long"', '-D"_int64=long long"',
# '-DWINAPI_FAMILY=100',
# '-D__forceinline=inline',
# '-D__clang__', '-D__GNUC__', '-D__GXX_EXPERIMENTAL_CXX0X__',

for argI in range(4, len(sys.argv)):
    arg = sys.argv[argI]
    if arg.startswith("-D") or arg.startswith("/D"):
        define = "-D" + arg[len("-D"):]
        if (define not in clang_defines):
            clang_args = clang_args + [define]
        if define[len("-D"):] == "_TARGET_64BIT":
            is64bit = True
        if define[len("/D"):] == "_TARGET_PS4":
            isPs4 = True
        if define[len("/D"):] == "_TARGET_PS5":
            isPs5 = True
    elif arg.startswith("-FI") or arg.startswith("/FI"):
        clang_args = clang_args + ["-include", arg[len("-FI"):]]
    elif arg.startswith("-I") or arg.startswith("/I"):
        if (len(arg[len("-I"):]) > 1):
            clang_args = clang_args + ["-I" + arg[len("-I"):]]
        else:
            clang_args = clang_args + ["-I", sys.argv[argI + 1]]
            argI = argI + 1
    elif arg in ['-isystem', '-include', '-imsvc']:
        clang_args = clang_args + [{'-imsvc': '-isystem'}.get(arg, arg), sys.argv[argI + 1]]
        argI = argI + 1
clang_args = clang_args + ['-w', '-std=c++20']
clang_args = clang_args + clang_defines
if (isPs4):
    clang_args = clang_args + ['-D__ORBIS__']
if (isPs5):
    clang_args = clang_args + ['-D__PROSPERO__']
clang_options = ['-ferror-limit=0', '-msse4', ('-m64' if is64bit else '-m32')]
if platform.system() == 'Windows':
    clang_options += ['-fms-compatibility', '-fms-extensions', '-fmsc-version=1900']

clang_args = clang_args + clang_options
clang_args = clang_args + ['-D_ECS_CODEGEN']
# print clang_args
all_gets = []
allParsedFunctions = parse_ecs_functions(input_file_name, os.path.basename(input_file_name), clang_args, is_es_name, False, compiler_errors, all_gets)

resultCode = gen_es(allParsedFunctions, event_handler_suffix, input_file_name)
gets_code = ""
gets_type_code = ""
# Note: line info significantly increase commits diffs & creates conflictcs on branch merges for frequently changed files (so it disabled by default)
veboselineinfo = os.environ.get('ECS_CODEGEN_VERBOSE_LINEINFO', 'no') == 'yes'
for some_get, some_get_type in all_gets:
  get_type = remove_const_from_type(some_get_type['type'])
  some_get_name = some_get.replace("_dot_", dot_suffix)
  if some_get_name != some_get:
    print("Using _dot_ is deprecated, check {name} in {file} Line {line}".format(name = some_get, file = some_get_type['file'], line = some_get_type['line']))
  file = some_get_type['file']
  line = some_get_type['line'] if veboselineinfo else 0
  fun = some_get_type['fun']
  while file.startswith("../"):
    file = file[len("../"):]
  get_component_type_fun = '''static constexpr ecs::component_t {some_get}_get_type()'''.format(**locals())
  gets_code += get_component_type_fun + ";\n"
  gets_type_code += get_component_type_fun + '''{{return ecs::ComponentTypeInfo<{get_type}>::type; }}\n'''.format(**locals())
  gets_code += '''static ecs::LTComponentList {some_get}_component(ECS_HASH("{some_get_name}"), {some_get}_get_type(), "{file}", "{fun}", {line});\n'''.format(**locals())

include_preamble = """// Built with ECS codegen version %s
#include <daECS/core/entitySystem.h>
#include <daECS/core/componentTypes.h>
""" % version
include_preamble += '#include "' + rel_path_for_include + '"\n'
if rel_path_for_include.endswith('ES.cpp.inl'):
  include_preamble += 'ECS_DEF_PULL_VAR(' + rel_path_for_include[0:len(rel_path_for_include)-10].rsplit('/',1)[0] + ');\n'
elif rel_path_for_include.endswith('.cpp.inl'):
  include_preamble += 'ECS_DEF_PULL_VAR(' + rel_path_for_include[0:len(rel_path_for_include)-8].rsplit('/',1)[0] + ');\n'

resultCode = include_preamble + resultCode
if len(gets_code) > 0:
  resultCode = "#include <daECS/core/internal/ltComponentList.h>\n" + gets_code + resultCode + gets_type_code

is_file_changed = True
if len(sys.argv) > 4 and sys.argv[4] == 'CHECK':
  is_file_changed = False

existing_lines = ''

if os.path.isfile(output_file_name):
  with io.open(output_file_name, 'rt', encoding='utf-8') as f:
    existing_lines = f.read()

  if resultCode != existing_lines:
    is_file_changed = True

else:
  is_file_changed = True

if len(sys.argv) > 4 and (sys.argv[4] == 'CHECK'):
  if is_file_changed:
    print("ERROR: " + sys.argv[2] + " is missing or altered")
    if (len(compiler_errors) > 0):
      print(compiler_errors)
    if os.path.isfile(output_file_name):
      for line in difflib.unified_diff(existing_lines.splitlines(), resultCode.splitlines()):
        sys.stdout.write(line)
    else:
      print("file was not commited: '" + output_file_name + "'")
    sys.exit(13)
  sys.exit(0)

if is_file_changed:
  # Use windows line ending if WSL linux detected
  nl = '\r\n' if ('linux' in sys.platform and 'Microsoft' in open('/proc/sys/kernel/osrelease').read()) else None
  with io.open(output_file_name, 'wt', encoding='utf-8', newline=nl) as f:
    f.write(unicode(resultCode, 'utf-8') if sys.version_info[0] == 2 else resultCode)
