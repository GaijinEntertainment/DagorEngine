pushd prog\tools\ShaderCompiler2
jam -sPlatformArch=arm64 -sPlatformSpec=vc17  
jam -sPlatformArch=arm64 -sPlatformSpec=vc17 -f jamfile-dx12
pause
popd
