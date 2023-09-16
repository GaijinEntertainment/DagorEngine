SET PYTHON_BIN=%GDEVTOOL%\python\python
SET CLANGFMT=%GDEVTOOL%\LLVM-15.0.7\bin\clang-format

call %PYTHON_BIN% auto_extract_properties.py %1 %2
call %PYTHON_BIN% gen_traits.py %1 %2
call %PYTHON_BIN% gen_writer.py %1 %2
call %PYTHON_BIN% gen_decoder.py %1 %2
call %PYTHON_BIN% gen_nodes.py %1 %2
call %PYTHON_BIN% gen_reader.py %1 %2

call %CLANGFMT% -i -style=file ..\publicInclude\spirv\traits_table.h
call %CLANGFMT% -i -style=file module_decoder.h
call %CLANGFMT% -i -style=file module_nodes.h
call %CLANGFMT% -i -style=file traits_table.cpp
call %CLANGFMT% -i -style=file module_writer.cpp
call %CLANGFMT% -i -style=file module_reader.cpp