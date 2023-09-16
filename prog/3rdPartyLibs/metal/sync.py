#!/usr/bin/python

import os
import os.path
import sys


os.system("python SPIRV-Tools/utils/generate_grammar_tables.py --spirv-core-grammar=SPIRV-Headers/include/spirv/unified1/spirv.core.grammar.json --extinst-debuginfo-grammar=SPIRV-Headers/include/spirv/unified1/extinst.debuginfo.grammar.json --extinst-cldebuginfo100-grammar=SPIRV-Headers/include/spirv/unified1/extinst.opencl.debuginfo.100.grammar.json --extension-enum-output=SPIRV-Tools/include/extension_enum.inc --enum-string-mapping-output=SPIRV-Tools/include/enum_string_mapping.inc")
os.system("python SPIRV-Tools/utils/generate_grammar_tables.py --extinst-opencl-grammar=SPIRV-Headers/include/spirv/unified1/extinst.opencl.std.100.grammar.json --opencl-insts-output=SPIRV-Tools/include/opencl.std.insts.inc")
os.system("python SPIRV-Tools/utils/generate_grammar_tables.py --extinst-glsl-grammar=SPIRV-Headers/include/spirv/unified1/extinst.glsl.std.450.grammar.json --glsl-insts-output=SPIRV-Tools/include/glsl.std.450.insts.inc")
os.system("python SPIRV-Tools/utils/generate_grammar_tables.py --extinst-vendor-grammar=SPIRV-Headers/include/spirv/unified1/extinst.spv-amd-shader-explicit-vertex-parameter.grammar.json --vendor-insts-output=SPIRV-Tools/include/spv-amd-shader-explicit-vertex-parameter.insts.inc")
os.system("python SPIRV-Tools/utils/generate_grammar_tables.py --extinst-vendor-grammar=SPIRV-Headers/include/spirv/unified1/extinst.spv-amd-shader-trinary-minmax.grammar.json --vendor-insts-output=SPIRV-Tools/include/spv-amd-shader-trinary-minmax.insts.inc")
os.system("python SPIRV-Tools/utils/generate_grammar_tables.py --extinst-vendor-grammar=SPIRV-Headers/include/spirv/unified1/extinst.spv-amd-gcn-shader.grammar.json --vendor-insts-output=SPIRV-Tools/include/spv-amd-gcn-shader.insts.inc")
os.system("python SPIRV-Tools/utils/generate_grammar_tables.py --extinst-vendor-grammar=SPIRV-Headers/include/spirv/unified1/extinst.spv-amd-shader-ballot.grammar.json --vendor-insts-output=SPIRV-Tools/include/spv-amd-shader-ballot.insts.inc")
os.system("python SPIRV-Tools/utils/generate_grammar_tables.py --extinst-vendor-grammar=SPIRV-Headers/include/spirv/unified1/extinst.debuginfo.grammar.json --vendor-insts-output=SPIRV-Tools/include/debuginfo.insts.inc")
os.system("python SPIRV-Tools/utils/generate_grammar_tables.py --extinst-vendor-grammar=SPIRV-Headers/include/spirv/unified1/extinst.nonsemantic.clspvreflection.grammar.json --vendor-insts-output=SPIRV-Tools/include/nonsemantic.clspvreflection.insts.inc")
os.system("python SPIRV-Tools/utils/generate_grammar_tables.py --extinst-vendor-grammar=SPIRV-Headers/include/spirv/unified1/extinst.opencl.debuginfo.100.grammar.json --vendor-insts-output=SPIRV-Tools/include/opencl.debuginfo.100.insts.inc --vendor-operand-kind-prefix=CLDEBUG100_")
os.system("python SPIRV-Tools/utils/generate_grammar_tables.py --extinst-vendor-grammar=SPIRV-Headers/include/spirv/unified1/extinst.nonsemantic.shader.debuginfo.100.grammar.json --vendor-insts-output=SPIRV-Tools/include/nonsemantic.shader.debuginfo.100.insts.inc --vendor-operand-kind-prefix=CLDEBUG100_")
os.system("python SPIRV-Tools/utils/generate_language_headers.py --extinst-grammar=SPIRV-Headers/include/spirv/unified1/extinst.debuginfo.grammar.json --extinst-output-path=SPIRV-Tools/include/DebugInfo.h")
os.system("python SPIRV-Tools/utils/generate_language_headers.py --extinst-grammar=SPIRV-Headers/include/spirv/unified1/extinst.opencl.debuginfo.100.grammar.json --extinst-output-path=SPIRV-Tools/include/OpenCLDebugInfo100.h")
os.system("python SPIRV-Tools/utils/generate_language_headers.py --extinst-grammar=SPIRV-Headers/include/spirv/unified1/extinst.nonsemantic.shader.debuginfo.100.grammar.json --extinst-output-path=SPIRV-Tools/include/NonSemanticShaderDebugInfo100.h")
os.system("python SPIRV-Tools/utils/generate_grammar_tables.py --spirv-core-grammar=SPIRV-Headers/include/spirv/1.0/spirv.core.grammar.json --extinst-debuginfo-grammar=SPIRV-Headers/include/spirv/unified1/extinst.debuginfo.grammar.json --extinst-cldebuginfo100-grammar=SPIRV-Headers/include/spirv/unified1/extinst.opencl.debuginfo.100.grammar.json --core-insts-output=SPIRV-Tools/include/core.insts-1.0.inc --operand-kinds-output=SPIRV-Tools/include/operand.kinds-1.0.inc")
os.system("python SPIRV-Tools/utils/generate_grammar_tables.py --spirv-core-grammar=SPIRV-Headers/include/spirv/1.1/spirv.core.grammar.json --extinst-debuginfo-grammar=SPIRV-Headers/include/spirv/unified1/extinst.debuginfo.grammar.json --extinst-cldebuginfo100-grammar=SPIRV-Headers/include/spirv/unified1/extinst.opencl.debuginfo.100.grammar.json --core-insts-output=SPIRV-Tools/include/core.insts-1.1.inc --operand-kinds-output=SPIRV-Tools/include/operand.kinds-1.1.inc")
os.system("python SPIRV-Tools/utils/generate_grammar_tables.py --spirv-core-grammar=SPIRV-Headers/include/spirv/1.2/spirv.core.grammar.json --extinst-debuginfo-grammar=SPIRV-Headers/include/spirv/unified1/extinst.debuginfo.grammar.json --extinst-cldebuginfo100-grammar=SPIRV-Headers/include/spirv/unified1/extinst.opencl.debuginfo.100.grammar.json --core-insts-output=SPIRV-Tools/include/core.insts-1.2.inc --operand-kinds-output=SPIRV-Tools/include/operand.kinds-1.2.inc")
os.system("python SPIRV-Tools/utils/generate_grammar_tables.py --spirv-core-grammar=SPIRV-Headers/include/spirv/unified1/spirv.core.grammar.json --extinst-debuginfo-grammar=SPIRV-Headers/include/spirv/unified1/extinst.debuginfo.grammar.json --extinst-cldebuginfo100-grammar=SPIRV-Headers/include/spirv/unified1/extinst.opencl.debuginfo.100.grammar.json --core-insts-output=SPIRV-Tools/include/core.insts-unified1.inc --operand-kinds-output=SPIRV-Tools/include/operand.kinds-unified1.inc")
os.system("python SPIRV-Tools/utils/generate_registry_tables.py --xml=SPIRV-Headers/include/spirv/spir-v.xml --generator-output=SPIRV-Tools/include/generators.inc")


# generate build_info.h for glslang:
if os.path.isfile("glslang/build_info.h.tmpl"):
	os.system("cd glslang && python build_info.py . -i build_info.h.tmpl -o glslang/build_info.h")

os.system("cd glslang && git apply --ignore-space-change --ignore-whitespace ../glslang_half_support.patch")
os.system("cd glslang && git apply --ignore-space-change --ignore-whitespace ../glslang_sort_global_constants.diff")