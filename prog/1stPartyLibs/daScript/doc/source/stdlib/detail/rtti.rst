.. |typedef-rtti-FileAccessPtr| replace:: smart_ptr<FileAccess>, i.e pointer to the `FileAccess` object.

.. |typedef-rtti-ModuleFlags| replace:: Flags which represent the module's state.

.. |typedef-rtti-ProgramFlags| replace:: Flags which represent state of the `Program` object, both during and after compilation.

.. |typedef-rtti-RttiValue| replace:: Variant type which represents value of any annotation arguments and variable annotations.

.. |enumeration-rtti-CompilationError| replace:: Enumeration which represents error type for each of the errors which compiler returns and various stages.

.. |enumeration-rtti-ConstMatters| replace:: Yes or no flag which indicates if constant flag of the type matters (during comparison).

.. |enumeration-rtti-RefMatters| replace:: Yes or no flag which indicates if reference flag of the type matters (during comparison).

.. |enumeration-rtti-TemporaryMatters| replace:: Yes or no flag which indicates if temporary flag of the type matters (during comparison).

.. |enumeration-rtti-Type| replace:: One of the fundamental (base) types of any type object.

.. |function-rtti-RttiValue_nothing| replace:: Constructs new RttiValue of type 'nothing'.

.. |function-rtti-arg_names| replace:: Iterates through argument names of the rtti type object.

.. |function-rtti-arg_types| replace:: Iterates through argument types of the rtti type object.

.. |function-rtti-basic_struct_for_each_field| replace:: Iterates through each field of the structure object.

.. |function-rtti-builtin_is_same_type| replace:: Returns true if two `TypeInfo` objects are the same given comparison criteria.

.. |function-rtti-compile| replace:: Compile Daslang program given as string.

.. |function-rtti-compile_file| replace:: Compile Daslang program given as file in the `FileAccess` object.

.. |function-rtti-context_for_each_function| replace:: Iterates through all functions in the `Context`.

.. |function-rtti-context_for_each_variable| replace:: Iterates through all variables in the `Context`.

.. |function-rtti-each_dim| replace:: Iterates through all dim values of the rtti type object, i.e. through all size properties of the array.

.. |function-rtti-get_annotation_argument_value| replace:: Returns RttiValue which represents argument value for the specific annotation argument.

.. |function-rtti-get_das_type_name| replace:: Returns name of the `Type` object.

.. |function-rtti-get_dim| replace:: Get dim property of the type, i.e. size of the static array.

.. |function-rtti-get_function_info| replace:: Get function declaration info by index.

.. |function-rtti-get_module| replace:: Get `Module` object by name.

.. |function-rtti-get_this_module| replace:: Get current `Program` object currently compiled module.

.. |function-rtti-get_total_functions| replace:: Get total number of functions in the context.

.. |function-rtti-get_total_variables| replace:: Get total number of global variables in the context.

.. |function-rtti-get_variable_info| replace:: Get global variable type information by variable index.

.. |function-rtti-get_variable_value| replace:: Return RttiValue which represents value of the global variable.

.. |function-rtti-is_compatible_cast| replace:: Returns true if `from` type can be casted to `to` type.

.. |function-rtti-is_same_type| replace:: Returns true if two `TypeInfo` objects are the same given comparison criteria.

.. |function-rtti-make_file_access| replace:: Creates new `FileAccess` object.

.. |function-rtti-module_for_each_annotation| replace:: Iterates though each handled type in the module.

.. |function-rtti-module_for_each_enumeration| replace:: Iterates through each enumeration in the module.

.. |function-rtti-module_for_each_function| replace:: Iterates through each function in the module.

.. |function-rtti-module_for_each_generic| replace:: Iterates through each generic function in the module.

.. |function-rtti-module_for_each_global| replace:: Iterates through each global variable in the module.

.. |function-rtti-module_for_each_structure| replace:: Iterates through all structure declarations in the `Module` object.

.. |function-rtti-program_for_each_module| replace:: Iterates through all modules of the `Program` object.

.. |function-rtti-program_for_each_registered_module| replace:: Iterates through all registered modules of the Daslang runtime.

.. |function-rtti-module_for_each_dependency| replace:: Iterates through each dependency of the module.

.. |function-rtti-rtti_builtin_structure_for_each_annotation| replace:: Iterates through each annotation for the `Structure` object.

.. |function-rtti-set_file_source| replace:: Sets source for the specified file in the `FileAccess` object.

.. |function-rtti-add_file_access_root| replace:: Add extra root directory (search path) to the `FileAccess` object.

.. |function-rtti-structure_for_each_annotation| replace:: Iterates through each annotation for the `Structure` object.

.. |function-rtti-class_info| replace:: Returns `StructInfo?`` for the class.

.. |structure_annotation-rtti-Annotation| replace:: Handled type or macro.

.. |structure_annotation-rtti-AnnotationArgument| replace:: Single argument of the annotation, typically part of the `AnnotationArgumentList`.

.. |any_annotation-rtti-AnnotationArgumentList| replace:: List of annotation arguments and properties.

.. |any_annotation-rtti-AnnotationArguments| replace:: List of annotation arguments.

.. |structure_annotation-rtti-AnnotationDeclaration| replace:: Annotation declaration, its location, and arguments.

.. |any_annotation-rtti-AnnotationList| replace:: List of all annotations attached to the object (function or structure).

.. |structure_annotation-rtti-BasicStructureAnnotation| replace:: Handled type which represents structure-like object.

.. |structure_annotation-rtti-EnumInfo| replace:: Type object which represents enumeration.

.. |structure_annotation-rtti-EnumValueInfo| replace:: Single element of enumeration, its name and value.

.. |structure_annotation-rtti-Error| replace:: Object which holds information about compilation error or exception.

.. |structure_annotation-rtti-FileAccess| replace:: Object which holds collection of files as well as means to access them (Project).

.. |structure_annotation-rtti-FileInfo| replace:: Information about a single file stored in the `FileAccess` object.

.. |structure_annotation-rtti-FuncInfo| replace:: Object which represents function declaration.

.. |structure_annotation-rtti-SimFunction| replace:: Object which represents simulated function in the `Context`.

.. |structure_annotation-rtti-LineInfo| replace:: Information about a section of the file stored in the `FileAccess` object.

.. |structure_annotation-rtti-Module| replace:: Collection of types, aliases, functions, classes, macros etc under a single namespace.

.. |structure_annotation-rtti-ModuleGroup| replace:: Collection of modules.

.. |structure_annotation-rtti-Program| replace:: Object representing full information about Daslang program during and after compilation (but not the simulated result of the program).

.. |structure_annotation-rtti-StructInfo| replace:: Type object which represents structure or class.

.. |structure_annotation-rtti-TypeAnnotation| replace:: Handled type.

.. |structure_annotation-rtti-TypeInfo| replace:: Object which represents any Daslang type.

.. |structure_annotation-rtti-VarInfo| replace:: Object which represents variable declaration.

.. |structure_annotation-rtti-LocalVariableInfo| replace:: Object which represents local variable declaration.

.. |any_annotation-rtti-dasvector`Error| replace:: to be documented

.. |variable-rtti-FUNCINFO_BUILTIN| replace:: Function flag which indicates that function is a built-in function.

.. |variable-rtti-FUNCINFO_INIT| replace:: Function flag which indicates that function is called during the `Context` initialization.

.. |variable-rtti-FUNCINFO_PRIVATE| replace:: Function flag which indicates that function is private.

.. |variable-rtti-FUNCINFO_SHUTDOWN| replace:: Function flag which indicates that function is called during the `Context` shutdown.

.. |variable-rtti-FUNCINFO_LATE_INIT| replace:: Function flag which indicates that function initialization is ordered via custom init order.

.. |typedef-rtti-context_category_flags| replace:: Flags which specify type of the `Context`.

.. |typedef-rtti-TypeInfoFlags| replace:: Flags which specify properties of the `TypeInfo` object (any rtti type).

.. |typedef-rtti-StructInfoFlags| replace:: Flags which represent properties of the `StructInfo` object (rtti object which represents structure type).

.. |structure_annotation-rtti-Context| replace:: Object which holds single Daslang Context. Context is the result of the simulation of the Daslang program.

.. |structure_annotation-rtti-CodeOfPolicies| replace:: Object which holds compilation and simulation settings and restrictions.

.. |function-rtti-LineInfo| replace:: LineInfo initializer.

.. |function-rtti-CodeOfPolicies| replace:: CodeOfPolicies initializer.

.. |function-rtti-using| replace:: Creates object which can be used inside of the block scope.

.. |function-rtti-for_each_expected_error| replace:: Iterates through each compilation error of the `Program` object.

.. |function-rtti-for_each_require_declaration| replace:: Iterates though each `require` declaration of the compiled program.

.. |function-rtti-simulate| replace:: Simulates Daslang program and creates 'Context' object.

.. |function-rtti-add_annotation_argument| replace:: Adds annotation argument to the `AnnotationArgumentList` object.

.. |function-rtti-sprint_data| replace:: Prints data given `TypeInfo` and returns result as a string, similar to `print` function.

.. |function-rtti-describe| replace:: Describe rtti object and return data as string.

.. |function-rtti-get_mangled_name| replace:: Returns mangled name of the function.

.. |function-rtti-get_function_by_mnh| replace:: Returns `SimFunction` by mangled name hash.

.. |function-rtti-get_function_by_mangled_name_hash| replace:: Returns `function<>` given mangled name hash.

.. |function-rtti-get_line_info| replace:: Returns `LineInfo` object for the current line (line where get_line_info is called from).

.. |function-rtti-this_context| replace:: Returns current `Context` object.

.. |function-rtti-get_function_mangled_name_hash| replace:: Returns mangled name hash of the `function<>` object.

.. |function-rtti-type_info| replace:: Returns `TypeInfo` object for the local variable.

.. |typeinfo_macro-rtti-rtti_typeinfo| replace:: Generates `TypeInfo` for the given expression or type.

.. |any_annotation-rtti-recursive_mutex| replace:: Holds system-specific recursive mutex object (typically std::recursive_mutex).

.. |function-rtti-lock_this_context| replace:: Makes recursive critical section of the current `Context` object.

.. |function-rtti-lock_context| replace:: Makes recursive critical section of the given `Context` object.

.. |function-rtti-lock_mutex| replace:: Makes recursive critical section of the given recursive_mutex object.

.. |function-rtti-get_function_address| replace:: Return function pointer `SimFunction *` given mangled name hash.

.. |function-rtti-basic_struct_for_each_parent| replace:: Iterates through each parent type of the `BasicStructureAnnotation` object.

.. |typedef-rtti-AnnotationDeclarationFlags| replace:: Flags which represent properties of the `AnnotationDeclaration` object.

.. |function-rtti-get_type_size| replace:: Returns size of the type in bytes.

.. |function-rtti-get_type_align| replace:: Returns alignment of the type in bytes.

.. |function-rtti-get_table_key_index| replace:: Returns index of the key in the table.

.. |function-rtti-with_program_serialized| replace:: Serializes program and then deserializes it. This is to test serialization.

.. |function-rtti-get_tuple_field_offset| replace:: Returns offset of the tuple field.

.. |function-rtti-get_variant_field_offset| replace:: Returns offset of the variant field.

.. |function-rtti-each| replace:: Iterates through each element of the object.

