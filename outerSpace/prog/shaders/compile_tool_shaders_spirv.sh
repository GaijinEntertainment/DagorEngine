../../../tools/dagor_cdk/linux-$(uname -m)/dsc2-spirv-dev ./shaders_tools11.blk -q -shaderOn -nodisassembly -commentPP -codeDumpErr -maxVSF 4096  -o ../../../_output/shaders/outerSpace-tools~spirv -out ../../tools/toolsSpirV
../../../tools/dagor_cdk/linux-$(uname -m)/dsc2-spirv-dev ./shaders_tools_exp.blk -q -shaderOn -relinkOnly  -o ../../../_output/shaders/outerSpace-tools~spirv
