../../../../tools/dagor_cdk/linux-$(uname -m)/dsc2-spirv-dev ./shaders_tools11.blk -q -shaderOn -nodisassembly -commentPP -codeDumpErr  -o ../../../../_output/shaders/testGI-tools~spirv
../../../../tools/dagor_cdk/linux-$(uname -m)/dsc2-spirv-dev ./shaders_tools_exp.blk -q -shaderOn -relinkOnly  -o ../../../../_output/shaders/testGI-tools~spirv
