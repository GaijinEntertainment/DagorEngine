pushd prog\tools\ShaderCompiler2
jam -sPlatformArch=arm64 -sPlatformSpec=vc17  
jam -sPlatformArch=arm64 -sPlatformSpec=vc17 -f jamfile-dx12
jam -sPlatformArch=arm64 -sPlatformSpec=vc17 -f jamfile-hlsl2spirv
pause
popd
