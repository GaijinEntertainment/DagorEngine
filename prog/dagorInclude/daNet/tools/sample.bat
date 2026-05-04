@echo off

set PATH=%PATH%;%GDEVTOOL%\python
set CODEGEN_MPI=python codegenMpi.py
set CODEGEN_ID_BASE=mpiCodegenIdBase.txt

%CODEGEN_MPI% sample.mpi unitMpi.h %CODEGEN_ID_BASE%

python codegenMpiNames.py %CODEGEN_ID_BASE% mpiIdNames.h mpiDebugFile.h
