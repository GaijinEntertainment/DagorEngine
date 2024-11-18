python3 auto_extract_properties.py
python3 gen_traits.py
python3 gen_writer.py
python3 gen_decoder.py
python3 gen_nodes.py
python3 gen_reader.py

clang-format -i -style=file ../publicInclude/spirv/traits_table.h
clang-format -i -style=file module_decoder.h
clang-format -i -style=file module_nodes.h
clang-format -i -style=file traits_table.cpp
clang-format -i -style=file module_writer.cpp
clang-format -i -style=file module_reader.cpp