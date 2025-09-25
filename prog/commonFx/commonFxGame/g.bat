call ..\..\..\prog\_jBuild\make_dagor_tools_path.cmd
set CSQEXE=%DAGOR_CDK_DIR%\csq-dev.exe
%CSQEXE% lightfxShadow.gen.nut
%CSQEXE% lightFx.gen.nut
%CSQEXE% dafxEmitter.gen.nut
%CSQEXE% dafxSparks.gen.nut
%CSQEXE% dafxModfx.gen.nut
%CSQEXE% dafxCompound.gen.nut
