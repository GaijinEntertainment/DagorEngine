REM updating genrated files for spir-v tools, only needed if external subdir has changed

SET CORE_GRAMMAR_PRM="--spirv-core-grammar=external/spirv-headers/include/spirv/unified1/spirv.core.grammar.json"
SET DEBUGINFO_GRAMMAR_PRM="--extinst-debuginfo-grammar=external/spirv-headers/include/spirv/unified1/extinst.debuginfo.grammar.json"
SET CLDEBUGINFO100_GRAMMAR_PRM="--extinst-cldebuginfo100-grammar=external/spirv-headers/include/spirv/unified1/extinst.opencl.debuginfo.100.grammar.json"

python utils/generate_grammar_tables.py %CORE_GRAMMAR_PRM% %DEBUGINFO_GRAMMAR_PRM% %CLDEBUGINFO100_GRAMMAR_PRM% --extension-enum-output=source/extension_enum.inc --enum-string-mapping-output=source/enum_string_mapping.inc --output-language=c++
python utils/generate_grammar_tables.py %CORE_GRAMMAR_PRM% %DEBUGINFO_GRAMMAR_PRM% %CLDEBUGINFO100_GRAMMAR_PRM% --core-insts-output=source/core.insts-unified1.inc --operand-kinds-output=source/operand.kinds-unified1.inc --output-language=c++

python utils/generate_grammar_tables.py --extinst-vendor-grammar=external/spirv-headers/include/spirv/unified1/extinst.debuginfo.grammar.json --vendor-insts-output=source/debuginfo.insts.inc --vendor-operand-kind-prefix="" --output-language=c++
python utils/generate_grammar_tables.py --extinst-glsl-grammar=external/spirv-headers/include/spirv/unified1/extinst.glsl.std.450.grammar.json --glsl-insts-output=source/glsl.std.450.insts.inc --output-language=c++

python utils/generate_grammar_tables.py --extinst-vendor-grammar=external/spirv-headers/include/spirv/unified1/extinst.nonsemantic.clspvreflection.grammar.json --vendor-insts-output=source/nonsemantic.clspvreflection.insts.inc --vendor-operand-kind-prefix="" --output-language=c++
python utils/generate_grammar_tables.py --extinst-vendor-grammar=external/spirv-headers/include/spirv/unified1/extinst.nonsemantic.shader.debuginfo.100.grammar.json --vendor-insts-output=source/nonsemantic.shader.debuginfo.100.insts.inc --vendor-operand-kind-prefix="SHDEBUG100_" --output-language=c++
python utils/generate_grammar_tables.py --extinst-vendor-grammar=external/spirv-headers/include/spirv/unified1/extinst.opencl.debuginfo.100.grammar.json --vendor-insts-output=source/opencl.debuginfo.100.insts.inc --vendor-operand-kind-prefix="CLDEBUG100_" --output-language=c++

python utils/generate_grammar_tables.py --extinst-vendor-grammar=external/spirv-headers/include/spirv/unified1/extinst.spv-amd-gcn-shader.grammar.json --vendor-insts-output=source/spv-amd-gcn-shader.insts.inc --vendor-operand-kind-prefix="" --output-language=c++
python utils/generate_grammar_tables.py --extinst-vendor-grammar=external/spirv-headers/include/spirv/unified1/extinst.spv-amd-shader-ballot.grammar.json --vendor-insts-output=source/spv-amd-shader-ballot.insts.inc --vendor-operand-kind-prefix="" --output-language=c++
python utils/generate_grammar_tables.py --extinst-vendor-grammar=external/spirv-headers/include/spirv/unified1/extinst.spv-amd-shader-explicit-vertex-parameter.grammar.json --vendor-insts-output=source/spv-amd-shader-explicit-vertex-parameter.insts.inc --vendor-operand-kind-prefix="" --output-language=c++
python utils/generate_grammar_tables.py --extinst-vendor-grammar=external/spirv-headers/include/spirv/unified1/extinst.spv-amd-shader-trinary-minmax.grammar.json --vendor-insts-output=source/spv-amd-shader-trinary-minmax.insts.inc --vendor-operand-kind-prefix="" --output-language=c++

python utils/generate_grammar_tables.py --extinst-opencl-grammar=external/spirv-headers/include/spirv/unified1/extinst.opencl.std.100.grammar.json --opencl-insts-output=source/opencl.std.insts.inc --output-language=c++

python utils/generate_registry_tables.py --xml=external/spirv-headers/include/spirv/spir-v.xml --generator-output=source/generators.inc
python utils/update_build_version.py . source/build-version.inc

