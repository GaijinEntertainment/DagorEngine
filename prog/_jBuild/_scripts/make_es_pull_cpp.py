import sys
import os

if len(sys.argv) < 4:
  print("ERROR: too few args, usage: make_es_pull_cpp.py <dest_cpp> <es_include> <final_pull_var> [es_pull_name]...")
  sys.exit(13)

f = open(sys.argv[1], 'wt')
f.write('#include <{0}>\n'.format(sys.argv[2]))
f.write('#define REG_SYS\\\n')
for argI in range(4, len(sys.argv)):
  f.write('  RS({0})\\\n'.format(sys.argv[argI]))
f.write('\n')
f.write('#define RS(x) ECS_DECL_PULL_VAR(x);\n')
f.write('REG_SYS\n')
f.write('#undef RS\n')
f.write('#define RS(x) + ECS_PULL_VAR(x)\n')
f.write('size_t {0} = 0 REG_SYS;\n'.format(sys.argv[3]))
f.close()
