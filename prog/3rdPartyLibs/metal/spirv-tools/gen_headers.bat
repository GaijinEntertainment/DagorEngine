rem updating genrated files for spir-v tools, only needed if external subdir has changed
python utils/generate_grammar_tables.py --spirv-core-grammar=external/spirv-headers/include/spirv/unified1/spirv.core.grammar.json --extinst-debuginfo-grammar=source/extinst.debuginfo.grammar.json --core-insts-output=source/core.insts-unified1.inc --operand-kinds-output=source/operand.kinds-unified1.inc

python utils/generate_grammar_tables.py --spirv-core-grammar=external/spirv-headers/include/spirv/unified1/spirv.core.grammar.json --extinst-debuginfo-grammar=source/extinst.debuginfo.grammar.json --extension-enum-output=source/extension_enum.inc --enum-string-mapping-output=source/enum_string_mapping.inc

python utils/generate_grammar_tables.py --extinst-opencl-grammar=external/spirv-headers/include/spirv/unified1/extinst.opencl.std.100.grammar.json --opencl-insts-output=source/opencl.std.insts.inc
python utils/generate_grammar_tables.py --extinst-glsl-grammar=external/spirv-headers/include/spirv/unified1/extinst.glsl.std.450.grammar.json --glsl-insts-output=source/glsl.std.450.insts.inc

python utils/generate_grammar_tables.py --extinst-vendor-grammar=source/extinst.spv-amd-shader-explicit-vertex-parameter.grammar.json --vendor-insts-output=source/spv-amd-shader-explicit-vertex-parameter.insts.inc
python utils/generate_grammar_tables.py --extinst-vendor-grammar=source/extinst.spv-amd-shader-trinary-minmax.grammar.json --vendor-insts-output=source/spv-amd-shader-trinary-minmax.insts.inc
python utils/generate_grammar_tables.py --extinst-vendor-grammar=source/extinst.spv-amd-gcn-shader.grammar.json --vendor-insts-output=source/spv-amd-gcn-shader.insts.inc
python utils/generate_grammar_tables.py --extinst-vendor-grammar=source/extinst.spv-amd-shader-ballot.grammar.json --vendor-insts-output=source/spv-amd-shader-ballot.insts.inc
python utils/generate_grammar_tables.py --extinst-vendor-grammar=source/extinst.debuginfo.grammar.json --vendor-insts-output=source/debuginfo.insts.inc

python utils/generate_registry_tables.py --xml=external/spirv-headers/include/spirv/spir-v.xml --generator-output=source/generators.inc

python utils/generate_language_headers.py --extinst-name=DebugInfo --extinst-grammar=source/extinst.debuginfo.grammar.json --extinst-output-base=source/DebugInfo

python utils/update_build_version.py . source/build-version.inc