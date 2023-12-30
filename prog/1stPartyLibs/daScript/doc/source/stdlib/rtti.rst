
.. _stdlib_rtti:

================================
Runtime type information library
================================

.. include:: detail/rtti.rst

The RTTI module reflects runtime type information to Daslang.
It also exposes Daslang compiler infrastructure to Daslang runtime.

All functions and symbols are in "rtti" module, use require to get access to it. ::

    require rtti


++++++++++++
Type aliases
++++++++++++

.. _alias-ProgramFlags:

.. das:attribute:: ProgramFlags is a bitfield

+-----------------+---+-----+
+field            +bit+value+
+=================+===+=====+
+failToCompile    +0  +1    +
+-----------------+---+-----+
+_unsafe          +1  +2    +
+-----------------+---+-----+
+isCompiling      +2  +4    +
+-----------------+---+-----+
+isSimulating     +3  +8    +
+-----------------+---+-----+
+isCompilingMacros+4  +16   +
+-----------------+---+-----+
+needMacroModule  +5  +32   +
+-----------------+---+-----+


|typedef-rtti-ProgramFlags|

.. _alias-context_category_flags:

.. das:attribute:: context_category_flags is a bitfield

+-----------------+---+-----+
+field            +bit+value+
+=================+===+=====+
+dead             +0  +1    +
+-----------------+---+-----+
+debug_context    +1  +2    +
+-----------------+---+-----+
+thread_clone     +2  +4    +
+-----------------+---+-----+
+job_clone        +3  +8    +
+-----------------+---+-----+
+opengl           +4  +16   +
+-----------------+---+-----+
+debugger_tick    +5  +32   +
+-----------------+---+-----+
+debugger_attached+6  +64   +
+-----------------+---+-----+
+macro_context    +7  +128  +
+-----------------+---+-----+
+folding_context  +8  +256  +
+-----------------+---+-----+
+audio            +9  +512  +
+-----------------+---+-----+


|typedef-rtti-context_category_flags|

.. _alias-TypeInfoFlags:

.. das:attribute:: TypeInfoFlags is a bitfield

+----------------+---+-----+
+field           +bit+value+
+================+===+=====+
+ref             +0  +1    +
+----------------+---+-----+
+refType         +1  +2    +
+----------------+---+-----+
+canCopy         +2  +4    +
+----------------+---+-----+
+isPod           +3  +8    +
+----------------+---+-----+
+isRawPod        +4  +16   +
+----------------+---+-----+
+isConst         +5  +32   +
+----------------+---+-----+
+isTemp          +6  +64   +
+----------------+---+-----+
+isImplicit      +7  +128  +
+----------------+---+-----+
+refValue        +8  +256  +
+----------------+---+-----+
+hasInitValue    +9  +512  +
+----------------+---+-----+
+isSmartPtr      +10 +1024 +
+----------------+---+-----+
+isSmartPtrNative+11 +2048 +
+----------------+---+-----+
+isHandled       +12 +4096 +
+----------------+---+-----+
+heapGC          +13 +8192 +
+----------------+---+-----+
+stringHeapGC    +14 +16384+
+----------------+---+-----+
+lockCheck       +15 +32768+
+----------------+---+-----+


|typedef-rtti-TypeInfoFlags|

.. _alias-StructInfoFlags:

.. das:attribute:: StructInfoFlags is a bitfield

+------------+---+-----+
+field       +bit+value+
+============+===+=====+
+_class      +0  +1    +
+------------+---+-----+
+_lambda     +1  +2    +
+------------+---+-----+
+heapGC      +2  +4    +
+------------+---+-----+
+stringHeapGC+3  +8    +
+------------+---+-----+
+lockCheck   +4  +16   +
+------------+---+-----+


|typedef-rtti-StructInfoFlags|

.. _alias-ModuleFlags:

.. das:attribute:: ModuleFlags is a bitfield

+----------------+---+-----+
+field           +bit+value+
+================+===+=====+
+builtIn         +0  +1    +
+----------------+---+-----+
+promoted        +1  +2    +
+----------------+---+-----+
+isPublic        +2  +4    +
+----------------+---+-----+
+isModule        +3  +8    +
+----------------+---+-----+
+isSolidContext  +4  +16   +
+----------------+---+-----+
+doNotAllowUnsafe+5  +32   +
+----------------+---+-----+


|typedef-rtti-ModuleFlags|

.. _alias-AnnotationDeclarationFlags:

.. das:attribute:: AnnotationDeclarationFlags is a bitfield

+---------+---+-----+
+field    +bit+value+
+=========+===+=====+
+inherited+0  +1    +
+---------+---+-----+


|typedef-rtti-AnnotationDeclarationFlags|

.. _alias-RttiValue:

.. das:attribute:: RttiValue is a variant type

+-------+------+
+tBool  +bool  +
+-------+------+
+tInt   +int   +
+-------+------+
+tUInt  +uint  +
+-------+------+
+tInt64 +int64 +
+-------+------+
+tUInt64+uint64+
+-------+------+
+tFloat +float +
+-------+------+
+tDouble+double+
+-------+------+
+tString+string+
+-------+------+
+nothing+any   +
+-------+------+


|typedef-rtti-RttiValue|

.. _alias-FileAccessPtr:

.. das:attribute:: FileAccessPtr = smart_ptr<rtti::FileAccess>

|typedef-rtti-FileAccessPtr|

+++++++++
Constants
+++++++++

.. _global-rtti-FUNCINFO_INIT:

.. das:attribute:: FUNCINFO_INIT = 0x1

|variable-rtti-FUNCINFO_INIT|

.. _global-rtti-FUNCINFO_BUILTIN:

.. das:attribute:: FUNCINFO_BUILTIN = 0x2

|variable-rtti-FUNCINFO_BUILTIN|

.. _global-rtti-FUNCINFO_PRIVATE:

.. das:attribute:: FUNCINFO_PRIVATE = 0x4

|variable-rtti-FUNCINFO_PRIVATE|

.. _global-rtti-FUNCINFO_SHUTDOWN:

.. das:attribute:: FUNCINFO_SHUTDOWN = 0x8

|variable-rtti-FUNCINFO_SHUTDOWN|

.. _global-rtti-FUNCINFO_LATE_INIT:

.. das:attribute:: FUNCINFO_LATE_INIT = 0x20

|variable-rtti-FUNCINFO_LATE_INIT|

++++++++++++
Enumerations
++++++++++++

.. _enum-rtti-CompilationError:

.. das:attribute:: CompilationError

+-----------------------------------+-----+
+unspecified                        +0    +
+-----------------------------------+-----+
+mismatching_parentheses            +10001+
+-----------------------------------+-----+
+mismatching_curly_bracers          +10002+
+-----------------------------------+-----+
+string_constant_exceeds_file       +10003+
+-----------------------------------+-----+
+string_constant_exceeds_line       +10004+
+-----------------------------------+-----+
+unexpected_close_comment           +10005+
+-----------------------------------+-----+
+integer_constant_out_of_range      +10006+
+-----------------------------------+-----+
+comment_contains_eof               +10007+
+-----------------------------------+-----+
+invalid_escape_sequence            +10008+
+-----------------------------------+-----+
+invalid_line_directive             +10009+
+-----------------------------------+-----+
+syntax_error                       +20000+
+-----------------------------------+-----+
+malformed_ast                      +20001+
+-----------------------------------+-----+
+invalid_type                       +30101+
+-----------------------------------+-----+
+invalid_return_type                +30102+
+-----------------------------------+-----+
+invalid_argument_type              +30103+
+-----------------------------------+-----+
+invalid_structure_field_type       +30104+
+-----------------------------------+-----+
+invalid_array_type                 +30105+
+-----------------------------------+-----+
+invalid_table_type                 +30106+
+-----------------------------------+-----+
+invalid_argument_count             +30107+
+-----------------------------------+-----+
+invalid_variable_type              +30108+
+-----------------------------------+-----+
+invalid_new_type                   +30109+
+-----------------------------------+-----+
+invalid_index_type                 +30110+
+-----------------------------------+-----+
+invalid_annotation                 +30111+
+-----------------------------------+-----+
+invalid_swizzle_mask               +30112+
+-----------------------------------+-----+
+invalid_initialization_type        +30113+
+-----------------------------------+-----+
+invalid_with_type                  +30114+
+-----------------------------------+-----+
+invalid_override                   +30115+
+-----------------------------------+-----+
+invalid_name                       +30116+
+-----------------------------------+-----+
+invalid_array_dimension            +30117+
+-----------------------------------+-----+
+invalid_iteration_source           +30118+
+-----------------------------------+-----+
+invalid_loop                       +30119+
+-----------------------------------+-----+
+invalid_label                      +30120+
+-----------------------------------+-----+
+invalid_enumeration                +30121+
+-----------------------------------+-----+
+invalid_option                     +30122+
+-----------------------------------+-----+
+invalid_member_function            +30123+
+-----------------------------------+-----+
+function_already_declared          +30201+
+-----------------------------------+-----+
+argument_already_declared          +30202+
+-----------------------------------+-----+
+local_variable_already_declared    +30203+
+-----------------------------------+-----+
+global_variable_already_declared   +30204+
+-----------------------------------+-----+
+structure_field_already_declared   +30205+
+-----------------------------------+-----+
+structure_already_declared         +30206+
+-----------------------------------+-----+
+structure_already_has_initializer  +30207+
+-----------------------------------+-----+
+enumeration_already_declared       +30208+
+-----------------------------------+-----+
+enumeration_value_already_declared +30209+
+-----------------------------------+-----+
+type_alias_already_declared        +30210+
+-----------------------------------+-----+
+field_already_initialized          +30211+
+-----------------------------------+-----+
+type_not_found                     +30301+
+-----------------------------------+-----+
+structure_not_found                +30302+
+-----------------------------------+-----+
+operator_not_found                 +30303+
+-----------------------------------+-----+
+function_not_found                 +30304+
+-----------------------------------+-----+
+variable_not_found                 +30305+
+-----------------------------------+-----+
+handle_not_found                   +30306+
+-----------------------------------+-----+
+annotation_not_found               +30307+
+-----------------------------------+-----+
+enumeration_not_found              +30308+
+-----------------------------------+-----+
+enumeration_value_not_found        +30309+
+-----------------------------------+-----+
+type_alias_not_found               +30310+
+-----------------------------------+-----+
+bitfield_not_found                 +30311+
+-----------------------------------+-----+
+cant_initialize                    +30401+
+-----------------------------------+-----+
+cant_dereference                   +30501+
+-----------------------------------+-----+
+cant_index                         +30502+
+-----------------------------------+-----+
+cant_get_field                     +30503+
+-----------------------------------+-----+
+cant_write_to_const                +30504+
+-----------------------------------+-----+
+cant_move_to_const                 +30505+
+-----------------------------------+-----+
+cant_write_to_non_reference        +30506+
+-----------------------------------+-----+
+cant_copy                          +30507+
+-----------------------------------+-----+
+cant_move                          +30508+
+-----------------------------------+-----+
+cant_pass_temporary                +30509+
+-----------------------------------+-----+
+condition_must_be_bool             +30601+
+-----------------------------------+-----+
+condition_must_be_static           +30602+
+-----------------------------------+-----+
+cant_pipe                          +30701+
+-----------------------------------+-----+
+invalid_block                      +30801+
+-----------------------------------+-----+
+return_or_break_in_finally         +30802+
+-----------------------------------+-----+
+module_not_found                   +30901+
+-----------------------------------+-----+
+module_already_has_a_name          +30902+
+-----------------------------------+-----+
+cant_new_handle                    +31001+
+-----------------------------------+-----+
+bad_delete                         +31002+
+-----------------------------------+-----+
+cant_infer_generic                 +31100+
+-----------------------------------+-----+
+cant_infer_missing_initializer     +31101+
+-----------------------------------+-----+
+cant_infer_mismatching_restrictions+31102+
+-----------------------------------+-----+
+invalid_cast                       +31200+
+-----------------------------------+-----+
+incompatible_cast                  +31201+
+-----------------------------------+-----+
+unsafe                             +31300+
+-----------------------------------+-----+
+index_out_of_range                 +31400+
+-----------------------------------+-----+
+expecting_return_value             +32101+
+-----------------------------------+-----+
+not_expecting_return_value         +32102+
+-----------------------------------+-----+
+invalid_return_semantics           +32103+
+-----------------------------------+-----+
+invalid_yield                      +32104+
+-----------------------------------+-----+
+typeinfo_reference                 +39901+
+-----------------------------------+-----+
+typeinfo_auto                      +39902+
+-----------------------------------+-----+
+typeinfo_undefined                 +39903+
+-----------------------------------+-----+
+typeinfo_dim                       +39904+
+-----------------------------------+-----+
+typeinfo_macro_error               +39905+
+-----------------------------------+-----+
+static_assert_failed               +40100+
+-----------------------------------+-----+
+run_failed                         +40101+
+-----------------------------------+-----+
+annotation_failed                  +40102+
+-----------------------------------+-----+
+concept_failed                     +40103+
+-----------------------------------+-----+
+not_all_paths_return_value         +40200+
+-----------------------------------+-----+
+assert_with_side_effects           +40201+
+-----------------------------------+-----+
+only_fast_aot_no_cpp_name          +40202+
+-----------------------------------+-----+
+aot_side_effects                   +40203+
+-----------------------------------+-----+
+no_global_heap                     +40204+
+-----------------------------------+-----+
+no_global_variables                +40205+
+-----------------------------------+-----+
+unused_function_argument           +40206+
+-----------------------------------+-----+
+unsafe_function                    +40207+
+-----------------------------------+-----+
+too_many_infer_passes              +41000+
+-----------------------------------+-----+
+missing_node                       +50100+
+-----------------------------------+-----+


|enumeration-rtti-CompilationError|

.. _enum-rtti-Type:

.. das:attribute:: Type

+--------------+--+
+none          +0 +
+--------------+--+
+autoinfer     +1 +
+--------------+--+
+alias         +2 +
+--------------+--+
+option        +3 +
+--------------+--+
+fakeContext   +4 +
+--------------+--+
+fakeLineInfo  +5 +
+--------------+--+
+anyArgument   +6 +
+--------------+--+
+tVoid         +7 +
+--------------+--+
+tBool         +8 +
+--------------+--+
+tInt64        +13+
+--------------+--+
+tUInt64       +14+
+--------------+--+
+tInt          +15+
+--------------+--+
+tInt2         +16+
+--------------+--+
+tInt3         +17+
+--------------+--+
+tInt4         +18+
+--------------+--+
+tUInt         +19+
+--------------+--+
+tUInt2        +20+
+--------------+--+
+tUInt3        +21+
+--------------+--+
+tUInt4        +22+
+--------------+--+
+tFloat        +23+
+--------------+--+
+tFloat2       +24+
+--------------+--+
+tFloat3       +25+
+--------------+--+
+tFloat4       +26+
+--------------+--+
+tDouble       +27+
+--------------+--+
+tRange        +28+
+--------------+--+
+tURange       +29+
+--------------+--+
+tRange64      +30+
+--------------+--+
+tURange64     +31+
+--------------+--+
+tString       +32+
+--------------+--+
+tStructure    +33+
+--------------+--+
+tHandle       +34+
+--------------+--+
+tEnumeration  +35+
+--------------+--+
+tPointer      +39+
+--------------+--+
+tFunction     +40+
+--------------+--+
+tLambda       +41+
+--------------+--+
+tIterator     +42+
+--------------+--+
+tArray        +43+
+--------------+--+
+tTable        +44+
+--------------+--+
+tBlock        +45+
+--------------+--+
+tInt8         +9 +
+--------------+--+
+tUInt8        +10+
+--------------+--+
+tInt16        +11+
+--------------+--+
+tUInt16       +12+
+--------------+--+
+tTuple        +46+
+--------------+--+
+tEnumeration8 +36+
+--------------+--+
+tEnumeration16+37+
+--------------+--+
+tVariant      +47+
+--------------+--+
+tBitfield     +38+
+--------------+--+


|enumeration-rtti-Type|

.. _enum-rtti-RefMatters:

.. das:attribute:: RefMatters

+---+-+
+no +0+
+---+-+
+yes+1+
+---+-+


|enumeration-rtti-RefMatters|

.. _enum-rtti-ConstMatters:

.. das:attribute:: ConstMatters

+---+-+
+no +0+
+---+-+
+yes+1+
+---+-+


|enumeration-rtti-ConstMatters|

.. _enum-rtti-TemporaryMatters:

.. das:attribute:: TemporaryMatters

+---+-+
+no +0+
+---+-+
+yes+1+
+---+-+


|enumeration-rtti-TemporaryMatters|

++++++++++++++++++
Handled structures
++++++++++++++++++

.. _handle-rtti-FileInfo:

.. das:attribute:: FileInfo

FileInfo fields are

+-------+--------------------------------------------------------+
+name   + :ref:`builtin::das_string <handle-builtin-das_string>` +
+-------+--------------------------------------------------------+
+tabSize+int                                                     +
+-------+--------------------------------------------------------+


|structure_annotation-rtti-FileInfo|

.. _handle-rtti-LineInfo:

.. das:attribute:: LineInfo

LineInfo fields are

+-----------+-----------------------------------------------+
+last_column+uint                                           +
+-----------+-----------------------------------------------+
+line       +uint                                           +
+-----------+-----------------------------------------------+
+last_line  +uint                                           +
+-----------+-----------------------------------------------+
+column     +uint                                           +
+-----------+-----------------------------------------------+
+fileInfo   + :ref:`rtti::FileInfo <handle-rtti-FileInfo>` ?+
+-----------+-----------------------------------------------+


|structure_annotation-rtti-LineInfo|

.. _handle-rtti-Context:

.. das:attribute:: Context

Context fields are

+--------------------------+--------------------------------------------------------------+
+breakOnException          +bool                                                          +
+--------------------------+--------------------------------------------------------------+
+exception                 +string const                                                  +
+--------------------------+--------------------------------------------------------------+
+category                  + :ref:`context_category_flags <alias-context_category_flags>` +
+--------------------------+--------------------------------------------------------------+
+alwaysStackWalkOnException+bool                                                          +
+--------------------------+--------------------------------------------------------------+
+last_exception            +string const                                                  +
+--------------------------+--------------------------------------------------------------+
+contextMutex              + :ref:`rtti::recursive_mutex <handle-rtti-recursive_mutex>` ? +
+--------------------------+--------------------------------------------------------------+
+name                      + :ref:`builtin::das_string <handle-builtin-das_string>`       +
+--------------------------+--------------------------------------------------------------+
+exceptionAt               + :ref:`rtti::LineInfo <handle-rtti-LineInfo>`                 +
+--------------------------+--------------------------------------------------------------+


Context property operators are

+------------------+------+
+totalFunctions    +int   +
+------------------+------+
+totalVariables    +int   +
+------------------+------+
+getCodeAllocatorId+uint64+
+------------------+------+


|structure_annotation-rtti-Context|

.. _handle-rtti-Error:

.. das:attribute:: Error

Error fields are

+-----+------------------------------------------------------------+
+fixme+ :ref:`builtin::das_string <handle-builtin-das_string>`     +
+-----+------------------------------------------------------------+
+at   + :ref:`rtti::LineInfo <handle-rtti-LineInfo>`               +
+-----+------------------------------------------------------------+
+what + :ref:`builtin::das_string <handle-builtin-das_string>`     +
+-----+------------------------------------------------------------+
+extra+ :ref:`builtin::das_string <handle-builtin-das_string>`     +
+-----+------------------------------------------------------------+
+cerr + :ref:`rtti::CompilationError <enum-rtti-CompilationError>` +
+-----+------------------------------------------------------------+


|structure_annotation-rtti-Error|

.. _handle-rtti-FileAccess:

.. das:attribute:: FileAccess

|structure_annotation-rtti-FileAccess|

.. _handle-rtti-Module:

.. das:attribute:: Module

Module fields are

+-----------+--------------------------------------------------------+
+moduleFlags+ :ref:`ModuleFlags <alias-ModuleFlags>`                 +
+-----------+--------------------------------------------------------+
+name       + :ref:`builtin::das_string <handle-builtin-das_string>` +
+-----------+--------------------------------------------------------+


|structure_annotation-rtti-Module|

.. _handle-rtti-ModuleGroup:

.. das:attribute:: ModuleGroup

|structure_annotation-rtti-ModuleGroup|

.. _handle-rtti-AnnotationArgument:

.. das:attribute:: AnnotationArgument

AnnotationArgument fields are

+---------+--------------------------------------------------------+
+fValue   +float                                                   +
+---------+--------------------------------------------------------+
+at       + :ref:`rtti::LineInfo <handle-rtti-LineInfo>`           +
+---------+--------------------------------------------------------+
+iValue   +int                                                     +
+---------+--------------------------------------------------------+
+name     + :ref:`builtin::das_string <handle-builtin-das_string>` +
+---------+--------------------------------------------------------+
+sValue   + :ref:`builtin::das_string <handle-builtin-das_string>` +
+---------+--------------------------------------------------------+
+bValue   +bool                                                    +
+---------+--------------------------------------------------------+
+basicType+ :ref:`rtti::Type <enum-rtti-Type>`                     +
+---------+--------------------------------------------------------+


|structure_annotation-rtti-AnnotationArgument|

.. _handle-rtti-Program:

.. das:attribute:: Program

Program fields are

+--------------+--------------------------------------------------------------------------+
+thisModuleName+ :ref:`builtin::das_string <handle-builtin-das_string>`                   +
+--------------+--------------------------------------------------------------------------+
+_options      + :ref:`rtti::AnnotationArgumentList <handle-rtti-AnnotationArgumentList>` +
+--------------+--------------------------------------------------------------------------+
+errors        +vector<Error>                                                             +
+--------------+--------------------------------------------------------------------------+
+flags         + :ref:`ProgramFlags <alias-ProgramFlags>`                                 +
+--------------+--------------------------------------------------------------------------+


|structure_annotation-rtti-Program|

.. _handle-rtti-Annotation:

.. das:attribute:: Annotation

Annotation fields are

+-------+--------------------------------------------------------+
+_module+ :ref:`rtti::Module <handle-rtti-Module>` ?             +
+-------+--------------------------------------------------------+
+cppName+ :ref:`builtin::das_string <handle-builtin-das_string>` +
+-------+--------------------------------------------------------+
+name   + :ref:`builtin::das_string <handle-builtin-das_string>` +
+-------+--------------------------------------------------------+


Annotation property operators are

+--------------------------+----+
+isTypeAnnotation          +bool+
+--------------------------+----+
+isBasicStructureAnnotation+bool+
+--------------------------+----+
+isStructureAnnotation     +bool+
+--------------------------+----+
+isStructureTypeAnnotation +bool+
+--------------------------+----+
+isFunctionAnnotation      +bool+
+--------------------------+----+
+isEnumerationAnnotation   +bool+
+--------------------------+----+


|structure_annotation-rtti-Annotation|

.. _handle-rtti-AnnotationDeclaration:

.. das:attribute:: AnnotationDeclaration

AnnotationDeclaration fields are

+----------+--------------------------------------------------------------------------+
+annotation+smart_ptr< :ref:`rtti::Annotation <handle-rtti-Annotation>` >             +
+----------+--------------------------------------------------------------------------+
+arguments + :ref:`rtti::AnnotationArgumentList <handle-rtti-AnnotationArgumentList>` +
+----------+--------------------------------------------------------------------------+
+at        + :ref:`rtti::LineInfo <handle-rtti-LineInfo>`                             +
+----------+--------------------------------------------------------------------------+
+flags     + :ref:`AnnotationDeclarationFlags <alias-AnnotationDeclarationFlags>`     +
+----------+--------------------------------------------------------------------------+


|structure_annotation-rtti-AnnotationDeclaration|

.. _handle-rtti-TypeAnnotation:

.. das:attribute:: TypeAnnotation

TypeAnnotation fields are

+-------+--------------------------------------------------------+
+_module+ :ref:`rtti::Module <handle-rtti-Module>` ?             +
+-------+--------------------------------------------------------+
+cppName+ :ref:`builtin::das_string <handle-builtin-das_string>` +
+-------+--------------------------------------------------------+
+name   + :ref:`builtin::das_string <handle-builtin-das_string>` +
+-------+--------------------------------------------------------+


TypeAnnotation property operators are

+----------------------+------+
+is_any_vector         +bool  +
+----------------------+------+
+canMove               +bool  +
+----------------------+------+
+canCopy               +bool  +
+----------------------+------+
+canClone              +bool  +
+----------------------+------+
+isPod                 +bool  +
+----------------------+------+
+isRawPod              +bool  +
+----------------------+------+
+isRefType             +bool  +
+----------------------+------+
+hasNonTrivialCtor     +bool  +
+----------------------+------+
+hasNonTrivialDtor     +bool  +
+----------------------+------+
+hasNonTrivialCopy     +bool  +
+----------------------+------+
+canBePlacedInContainer+bool  +
+----------------------+------+
+isLocal               +bool  +
+----------------------+------+
+canNew                +bool  +
+----------------------+------+
+canDelete             +bool  +
+----------------------+------+
+needDelete            +bool  +
+----------------------+------+
+canDeletePtr          +bool  +
+----------------------+------+
+isIterable            +bool  +
+----------------------+------+
+isShareable           +bool  +
+----------------------+------+
+isSmart               +bool  +
+----------------------+------+
+avoidNullPtr          +bool  +
+----------------------+------+
+sizeOf                +uint64+
+----------------------+------+
+alignOf               +uint64+
+----------------------+------+


|structure_annotation-rtti-TypeAnnotation|

.. _handle-rtti-BasicStructureAnnotation:

.. das:attribute:: BasicStructureAnnotation

BasicStructureAnnotation fields are

+-------+--------------------------------------------------------+
+name   + :ref:`builtin::das_string <handle-builtin-das_string>` +
+-------+--------------------------------------------------------+
+cppName+ :ref:`builtin::das_string <handle-builtin-das_string>` +
+-------+--------------------------------------------------------+


BasicStructureAnnotation property operators are

+----------+---+
+fieldCount+int+
+----------+---+


|structure_annotation-rtti-BasicStructureAnnotation|

.. _handle-rtti-EnumValueInfo:

.. das:attribute:: EnumValueInfo

EnumValueInfo fields are

+-----+------------+
+value+int64       +
+-----+------------+
+name +string const+
+-----+------------+


|structure_annotation-rtti-EnumValueInfo|

.. _handle-rtti-EnumInfo:

.. das:attribute:: EnumInfo

EnumInfo fields are

+-----------+------------+
+count      +uint        +
+-----------+------------+
+name       +string const+
+-----------+------------+
+module_name+string const+
+-----------+------------+
+hash       +uint64      +
+-----------+------------+


|structure_annotation-rtti-EnumInfo|

.. _handle-rtti-StructInfo:

.. das:attribute:: StructInfo

StructInfo fields are

+-----------+------------------------------------------------+
+init_mnh   +uint64                                          +
+-----------+------------------------------------------------+
+size       +uint                                            +
+-----------+------------------------------------------------+
+count      +uint                                            +
+-----------+------------------------------------------------+
+name       +string const                                    +
+-----------+------------------------------------------------+
+module_name+string const                                    +
+-----------+------------------------------------------------+
+hash       +uint64                                          +
+-----------+------------------------------------------------+
+flags      + :ref:`StructInfoFlags <alias-StructInfoFlags>` +
+-----------+------------------------------------------------+


|structure_annotation-rtti-StructInfo|

.. _handle-rtti-TypeInfo:

.. das:attribute:: TypeInfo

TypeInfo fields are

+----------+------------------------------------------------+
+argTypes  + :ref:`rtti::TypeInfo <handle-rtti-TypeInfo>` ??+
+----------+------------------------------------------------+
+size      +uint                                            +
+----------+------------------------------------------------+
+secondType+ :ref:`rtti::TypeInfo <handle-rtti-TypeInfo>` ? +
+----------+------------------------------------------------+
+dimSize   +uint                                            +
+----------+------------------------------------------------+
+hash      +uint64                                          +
+----------+------------------------------------------------+
+argNames  +string const?                                   +
+----------+------------------------------------------------+
+argCount  +uint                                            +
+----------+------------------------------------------------+
+basicType + :ref:`rtti::Type <enum-rtti-Type>`             +
+----------+------------------------------------------------+
+firstType + :ref:`rtti::TypeInfo <handle-rtti-TypeInfo>` ? +
+----------+------------------------------------------------+
+flags     + :ref:`TypeInfoFlags <alias-TypeInfoFlags>`     +
+----------+------------------------------------------------+


TypeInfo property operators are

+----------+-----------------------------------------------------------+
+enumType  + :ref:`rtti::EnumInfo <handle-rtti-EnumInfo>` ?            +
+----------+-----------------------------------------------------------+
+isRef     +bool                                                       +
+----------+-----------------------------------------------------------+
+isRefType +bool                                                       +
+----------+-----------------------------------------------------------+
+isRefValue+bool                                                       +
+----------+-----------------------------------------------------------+
+canCopy   +bool                                                       +
+----------+-----------------------------------------------------------+
+isPod     +bool                                                       +
+----------+-----------------------------------------------------------+
+isRawPod  +bool                                                       +
+----------+-----------------------------------------------------------+
+isConst   +bool                                                       +
+----------+-----------------------------------------------------------+
+isTemp    +bool                                                       +
+----------+-----------------------------------------------------------+
+isImplicit+bool                                                       +
+----------+-----------------------------------------------------------+
+annotation+ :ref:`rtti::TypeAnnotation <handle-rtti-TypeAnnotation>` ?+
+----------+-----------------------------------------------------------+
+structType+ :ref:`rtti::StructInfo <handle-rtti-StructInfo>` ?        +
+----------+-----------------------------------------------------------+


|structure_annotation-rtti-TypeInfo|

.. _handle-rtti-VarInfo:

.. das:attribute:: VarInfo

VarInfo fields are

+--------------------+---------------------------------------------------------------------------------+
+argTypes            + :ref:`rtti::TypeInfo <handle-rtti-TypeInfo>` ??                                 +
+--------------------+---------------------------------------------------------------------------------+
+size                +uint                                                                             +
+--------------------+---------------------------------------------------------------------------------+
+value               +any                                                                              +
+--------------------+---------------------------------------------------------------------------------+
+secondType          + :ref:`rtti::TypeInfo <handle-rtti-TypeInfo>` ?                                  +
+--------------------+---------------------------------------------------------------------------------+
+dimSize             +uint                                                                             +
+--------------------+---------------------------------------------------------------------------------+
+name                +string const                                                                     +
+--------------------+---------------------------------------------------------------------------------+
+hash                +uint64                                                                           +
+--------------------+---------------------------------------------------------------------------------+
+argNames            +string const?                                                                    +
+--------------------+---------------------------------------------------------------------------------+
+argCount            +uint                                                                             +
+--------------------+---------------------------------------------------------------------------------+
+sValue              +string                                                                           +
+--------------------+---------------------------------------------------------------------------------+
+offset              +uint                                                                             +
+--------------------+---------------------------------------------------------------------------------+
+basicType           + :ref:`rtti::Type <enum-rtti-Type>`                                              +
+--------------------+---------------------------------------------------------------------------------+
+annotation_arguments+ :ref:`rtti::AnnotationArguments <handle-rtti-AnnotationArguments>`  const? const+
+--------------------+---------------------------------------------------------------------------------+
+firstType           + :ref:`rtti::TypeInfo <handle-rtti-TypeInfo>` ?                                  +
+--------------------+---------------------------------------------------------------------------------+
+flags               + :ref:`TypeInfoFlags <alias-TypeInfoFlags>`                                      +
+--------------------+---------------------------------------------------------------------------------+


|structure_annotation-rtti-VarInfo|

.. _handle-rtti-LocalVariableInfo:

.. das:attribute:: LocalVariableInfo

LocalVariableInfo fields are

+----------+--------------------------------------------------------------+
+visibility+ :ref:`rtti::LineInfo <handle-rtti-LineInfo>`                 +
+----------+--------------------------------------------------------------+
+argTypes  + :ref:`rtti::TypeInfo <handle-rtti-TypeInfo>` ??              +
+----------+--------------------------------------------------------------+
+size      +uint                                                          +
+----------+--------------------------------------------------------------+
+secondType+ :ref:`rtti::TypeInfo <handle-rtti-TypeInfo>` ?               +
+----------+--------------------------------------------------------------+
+dimSize   +uint                                                          +
+----------+--------------------------------------------------------------+
+localFlags+ :ref:`LocalVariableInfoFlags <alias-LocalVariableInfoFlags>` +
+----------+--------------------------------------------------------------+
+stackTop  +uint                                                          +
+----------+--------------------------------------------------------------+
+name      +string const                                                  +
+----------+--------------------------------------------------------------+
+hash      +uint64                                                        +
+----------+--------------------------------------------------------------+
+argNames  +string const?                                                 +
+----------+--------------------------------------------------------------+
+argCount  +uint                                                          +
+----------+--------------------------------------------------------------+
+basicType + :ref:`rtti::Type <enum-rtti-Type>`                           +
+----------+--------------------------------------------------------------+
+firstType + :ref:`rtti::TypeInfo <handle-rtti-TypeInfo>` ?               +
+----------+--------------------------------------------------------------+
+flags     + :ref:`TypeInfoFlags <alias-TypeInfoFlags>`                   +
+----------+--------------------------------------------------------------+


|structure_annotation-rtti-LocalVariableInfo|

.. _handle-rtti-FuncInfo:

.. das:attribute:: FuncInfo

FuncInfo fields are

+-----------+------------------------------------------------------------------+
+locals     + :ref:`rtti::LocalVariableInfo <handle-rtti-LocalVariableInfo>` ??+
+-----------+------------------------------------------------------------------+
+stackSize  +uint                                                              +
+-----------+------------------------------------------------------------------+
+result     + :ref:`rtti::TypeInfo <handle-rtti-TypeInfo>` ?                   +
+-----------+------------------------------------------------------------------+
+count      +uint                                                              +
+-----------+------------------------------------------------------------------+
+globals    + :ref:`rtti::VarInfo <handle-rtti-VarInfo>` ??                    +
+-----------+------------------------------------------------------------------+
+cppName    +string const                                                      +
+-----------+------------------------------------------------------------------+
+name       +string const                                                      +
+-----------+------------------------------------------------------------------+
+globalCount+uint                                                              +
+-----------+------------------------------------------------------------------+
+hash       +uint64                                                            +
+-----------+------------------------------------------------------------------+
+localCount +uint                                                              +
+-----------+------------------------------------------------------------------+
+flags      +uint                                                              +
+-----------+------------------------------------------------------------------+


|structure_annotation-rtti-FuncInfo|

.. _handle-rtti-SimFunction:

.. das:attribute:: SimFunction

SimFunction fields are

+---------------+--------------------------------------------------+
+stackSize      +uint                                              +
+---------------+--------------------------------------------------+
+mangledNameHash+uint64                                            +
+---------------+--------------------------------------------------+
+mangledName    +string                                            +
+---------------+--------------------------------------------------+
+name           +string                                            +
+---------------+--------------------------------------------------+
+debugInfo      + :ref:`rtti::FuncInfo <handle-rtti-FuncInfo>` ?   +
+---------------+--------------------------------------------------+
+flags          + :ref:`SimFunctionFlags <alias-SimFunctionFlags>` +
+---------------+--------------------------------------------------+


SimFunction property operators are

+--------+-----------------------------------------------------------+
+lineInfo+ :ref:`rtti::LineInfo <handle-rtti-LineInfo>`  const? const+
+--------+-----------------------------------------------------------+


|structure_annotation-rtti-SimFunction|

.. _handle-rtti-CodeOfPolicies:

.. das:attribute:: CodeOfPolicies

CodeOfPolicies fields are

+----------------------------------+--------------------------------------------------------+
+aot_module                        +bool                                                    +
+----------------------------------+--------------------------------------------------------+
+fail_on_no_aot                    +bool                                                    +
+----------------------------------+--------------------------------------------------------+
+jit                               +bool                                                    +
+----------------------------------+--------------------------------------------------------+
+fail_on_lack_of_aot_export        +bool                                                    +
+----------------------------------+--------------------------------------------------------+
+profiler                          +bool                                                    +
+----------------------------------+--------------------------------------------------------+
+debugger                          +bool                                                    +
+----------------------------------+--------------------------------------------------------+
+aot_order_side_effects            +bool                                                    +
+----------------------------------+--------------------------------------------------------+
+threadlock_context                +bool                                                    +
+----------------------------------+--------------------------------------------------------+
+macro_context_collect             +bool                                                    +
+----------------------------------+--------------------------------------------------------+
+rtti                              +bool                                                    +
+----------------------------------+--------------------------------------------------------+
+ignore_shared_modules             +bool                                                    +
+----------------------------------+--------------------------------------------------------+
+no_deprecated                     +bool                                                    +
+----------------------------------+--------------------------------------------------------+
+aot                               +bool                                                    +
+----------------------------------+--------------------------------------------------------+
+allow_shared_lambda               +bool                                                    +
+----------------------------------+--------------------------------------------------------+
+allow_local_variable_shadowing    +bool                                                    +
+----------------------------------+--------------------------------------------------------+
+multiple_contexts                 +bool                                                    +
+----------------------------------+--------------------------------------------------------+
+heap_size_hint                    +uint                                                    +
+----------------------------------+--------------------------------------------------------+
+profile_module                    + :ref:`builtin::das_string <handle-builtin-das_string>` +
+----------------------------------+--------------------------------------------------------+
+no_init                           +bool                                                    +
+----------------------------------+--------------------------------------------------------+
+always_report_candidates_threshold+int                                                     +
+----------------------------------+--------------------------------------------------------+
+persistent_heap                   +bool                                                    +
+----------------------------------+--------------------------------------------------------+
+no_global_heap                    +bool                                                    +
+----------------------------------+--------------------------------------------------------+
+intern_strings                    +bool                                                    +
+----------------------------------+--------------------------------------------------------+
+no_optimizations                  +bool                                                    +
+----------------------------------+--------------------------------------------------------+
+allow_block_variable_shadowing    +bool                                                    +
+----------------------------------+--------------------------------------------------------+
+no_unused_function_arguments      +bool                                                    +
+----------------------------------+--------------------------------------------------------+
+stack                             +uint                                                    +
+----------------------------------+--------------------------------------------------------+
+smart_pointer_by_value_unsafe     +bool                                                    +
+----------------------------------+--------------------------------------------------------+
+no_unused_block_arguments         +bool                                                    +
+----------------------------------+--------------------------------------------------------+
+export_all                        +bool                                                    +
+----------------------------------+--------------------------------------------------------+
+solid_context                     +bool                                                    +
+----------------------------------+--------------------------------------------------------+
+no_global_variables               +bool                                                    +
+----------------------------------+--------------------------------------------------------+
+completion                        +bool                                                    +
+----------------------------------+--------------------------------------------------------+
+string_heap_size_hint             +uint                                                    +
+----------------------------------+--------------------------------------------------------+
+macro_context_persistent_heap     +bool                                                    +
+----------------------------------+--------------------------------------------------------+
+no_unsafe                         +bool                                                    +
+----------------------------------+--------------------------------------------------------+
+local_ref_is_unsafe               +bool                                                    +
+----------------------------------+--------------------------------------------------------+
+no_aliasing                       +bool                                                    +
+----------------------------------+--------------------------------------------------------+
+no_global_variables_at_all        +bool                                                    +
+----------------------------------+--------------------------------------------------------+
+strict_smart_pointers             +bool                                                    +
+----------------------------------+--------------------------------------------------------+
+only_fast_aot                     +bool                                                    +
+----------------------------------+--------------------------------------------------------+
+debug_module                      + :ref:`builtin::das_string <handle-builtin-das_string>` +
+----------------------------------+--------------------------------------------------------+
+strict_unsafe_delete              +bool                                                    +
+----------------------------------+--------------------------------------------------------+
+default_module_public             +bool                                                    +
+----------------------------------+--------------------------------------------------------+


|structure_annotation-rtti-CodeOfPolicies|

+++++++++++++++
Typeinfo macros
+++++++++++++++

.. _call-macro-rtti-rtti_typeinfo:

.. das:attribute:: rtti_typeinfo

|typeinfo_macro-rtti-rtti_typeinfo|

+++++++++++++
Handled types
+++++++++++++

.. _handle-rtti-recursive_mutex:

.. das:attribute:: recursive_mutex

|any_annotation-rtti-recursive_mutex|

.. _handle-rtti-AnnotationArguments:

.. das:attribute:: AnnotationArguments

|any_annotation-rtti-AnnotationArguments|

.. _handle-rtti-AnnotationArgumentList:

.. das:attribute:: AnnotationArgumentList

|any_annotation-rtti-AnnotationArgumentList|

.. _handle-rtti-AnnotationList:

.. das:attribute:: AnnotationList

|any_annotation-rtti-AnnotationList|

+++++++++++++++++++++++++++++++
Initialization and finalization
+++++++++++++++++++++++++++++++

  *  :ref:`LineInfo () : rtti::LineInfo <function-_at_rtti_c__c_LineInfo>`
  *  :ref:`LineInfo (arg0:rtti::FileInfo? const implicit;arg1:int const;arg2:int const;arg3:int const;arg4:int const) : rtti::LineInfo <function-_at_rtti_c__c_LineInfo_CI1_ls_H_ls_rtti_c__c_FileInfo_gr__gr_?_Ci_Ci_Ci_Ci>`
  *  :ref:`using (arg0:block\<(var arg0:rtti::recursive_mutex explicit):void\> const implicit) : void <function-_at_rtti_c__c_using_CI0_ls_XH_ls_rtti_c__c_recursive_mutex_gr__gr_1_ls_v_gr__builtin_>`
  *  :ref:`CodeOfPolicies () : rtti::CodeOfPolicies <function-_at_rtti_c__c_CodeOfPolicies>`
  *  :ref:`using (arg0:block\<(var arg0:rtti::CodeOfPolicies explicit):void\> const implicit) : void <function-_at_rtti_c__c_using_CI0_ls_XH_ls_rtti_c__c_CodeOfPolicies_gr__gr_1_ls_v_gr__builtin_>`
  *  :ref:`using (arg0:block\<(var arg0:rtti::ModuleGroup explicit):void\> const implicit) : void <function-_at_rtti_c__c_using_CI0_ls_XH_ls_rtti_c__c_ModuleGroup_gr__gr_1_ls_v_gr__builtin_>`
  *  :ref:`RttiValue_nothing () : auto <function-_at_rtti_c__c_RttiValue_nothing>`

.. _function-_at_rtti_c__c_LineInfo:

.. das:function:: LineInfo()

LineInfo returns  :ref:`rtti::LineInfo <handle-rtti-LineInfo>`

|function-rtti-LineInfo|

.. _function-_at_rtti_c__c_LineInfo_CI1_ls_H_ls_rtti_c__c_FileInfo_gr__gr_?_Ci_Ci_Ci_Ci:

.. das:function:: LineInfo(arg0: rtti::FileInfo? const implicit; arg1: int const; arg2: int const; arg3: int const; arg4: int const)

LineInfo returns  :ref:`rtti::LineInfo <handle-rtti-LineInfo>`

+--------+--------------------------------------------------------------+
+argument+argument type                                                 +
+========+==============================================================+
+arg0    + :ref:`rtti::FileInfo <handle-rtti-FileInfo>` ? const implicit+
+--------+--------------------------------------------------------------+
+arg1    +int const                                                     +
+--------+--------------------------------------------------------------+
+arg2    +int const                                                     +
+--------+--------------------------------------------------------------+
+arg3    +int const                                                     +
+--------+--------------------------------------------------------------+
+arg4    +int const                                                     +
+--------+--------------------------------------------------------------+


|function-rtti-LineInfo|

.. _function-_at_rtti_c__c_using_CI0_ls_XH_ls_rtti_c__c_recursive_mutex_gr__gr_1_ls_v_gr__builtin_:

.. das:function:: using(arg0: block<(var arg0:rtti::recursive_mutex explicit):void> const implicit)

+--------+-----------------------------------------------------------------------------------------+
+argument+argument type                                                                            +
+========+=========================================================================================+
+arg0    +block<( :ref:`rtti::recursive_mutex <handle-rtti-recursive_mutex>` ):void> const implicit+
+--------+-----------------------------------------------------------------------------------------+


|function-rtti-using|

.. _function-_at_rtti_c__c_CodeOfPolicies:

.. das:function:: CodeOfPolicies()

CodeOfPolicies returns  :ref:`rtti::CodeOfPolicies <handle-rtti-CodeOfPolicies>`

|function-rtti-CodeOfPolicies|

.. _function-_at_rtti_c__c_using_CI0_ls_XH_ls_rtti_c__c_CodeOfPolicies_gr__gr_1_ls_v_gr__builtin_:

.. das:function:: using(arg0: block<(var arg0:rtti::CodeOfPolicies explicit):void> const implicit)

+--------+---------------------------------------------------------------------------------------+
+argument+argument type                                                                          +
+========+=======================================================================================+
+arg0    +block<( :ref:`rtti::CodeOfPolicies <handle-rtti-CodeOfPolicies>` ):void> const implicit+
+--------+---------------------------------------------------------------------------------------+


|function-rtti-using|

.. _function-_at_rtti_c__c_using_CI0_ls_XH_ls_rtti_c__c_ModuleGroup_gr__gr_1_ls_v_gr__builtin_:

.. das:function:: using(arg0: block<(var arg0:rtti::ModuleGroup explicit):void> const implicit)

+--------+---------------------------------------------------------------------------------+
+argument+argument type                                                                    +
+========+=================================================================================+
+arg0    +block<( :ref:`rtti::ModuleGroup <handle-rtti-ModuleGroup>` ):void> const implicit+
+--------+---------------------------------------------------------------------------------+


|function-rtti-using|

.. _function-_at_rtti_c__c_RttiValue_nothing:

.. das:function:: RttiValue_nothing()

RttiValue_nothing returns auto

|function-rtti-RttiValue_nothing|

+++++++++++
Type access
+++++++++++

  *  :ref:`get_dim (typeinfo:rtti::TypeInfo const implicit;index:int const;context:__context const;at:__lineInfo const) : int <function-_at_rtti_c__c_get_dim_CIH_ls_rtti_c__c_TypeInfo_gr__Ci_C_c_C_l>`
  *  :ref:`get_dim (typeinfo:rtti::VarInfo const implicit;index:int const;context:__context const;at:__lineInfo const) : int <function-_at_rtti_c__c_get_dim_CIH_ls_rtti_c__c_VarInfo_gr__Ci_C_c_C_l>`
  *  :ref:`builtin_is_same_type (a:rtti::TypeInfo const? const implicit;b:rtti::TypeInfo const? const implicit;refMatters:rtti::RefMatters const;cosntMatters:rtti::ConstMatters const;tempMatters:rtti::TemporaryMatters const;topLevel:bool const) : bool <function-_at_rtti_c__c_builtin_is_same_type_CI1_ls_CH_ls_rtti_c__c_TypeInfo_gr__gr_?_CI1_ls_CH_ls_rtti_c__c_TypeInfo_gr__gr_?_CE_ls_rtti_c__c_RefMatters_gr__CE_ls_rtti_c__c_ConstMatters_gr__CE_ls_rtti_c__c_TemporaryMatters_gr__Cb>`
  *  :ref:`get_type_size (type:rtti::TypeInfo? const implicit) : int <function-_at_rtti_c__c_get_type_size_CI1_ls_H_ls_rtti_c__c_TypeInfo_gr__gr_?>`
  *  :ref:`get_type_align (type:rtti::TypeInfo? const implicit) : int <function-_at_rtti_c__c_get_type_align_CI1_ls_H_ls_rtti_c__c_TypeInfo_gr__gr_?>`
  *  :ref:`is_compatible_cast (from:rtti::StructInfo const? const implicit;to:rtti::StructInfo const? const implicit) : bool <function-_at_rtti_c__c_is_compatible_cast_CI1_ls_CH_ls_rtti_c__c_StructInfo_gr__gr_?_CI1_ls_CH_ls_rtti_c__c_StructInfo_gr__gr_?>`
  *  :ref:`get_das_type_name (type:rtti::Type const;context:__context const) : string <function-_at_rtti_c__c_get_das_type_name_CE_ls_rtti_c__c_Type_gr__C_c>`
  *  :ref:`is_same_type (a:rtti::TypeInfo const;b:rtti::TypeInfo const;refMatters:rtti::RefMatters const;constMatters:rtti::ConstMatters const;temporaryMatters:rtti::TemporaryMatters const;topLevel:bool const) : auto <function-_at_rtti_c__c_is_same_type_CH_ls_rtti_c__c_TypeInfo_gr__CH_ls_rtti_c__c_TypeInfo_gr__CE_ls_rtti_c__c_RefMatters_gr__CE_ls_rtti_c__c_ConstMatters_gr__CE_ls_rtti_c__c_TemporaryMatters_gr__Cb>`
  *  :ref:`is_compatible_cast (a:rtti::StructInfo const;b:rtti::StructInfo const) : auto <function-_at_rtti_c__c_is_compatible_cast_CH_ls_rtti_c__c_StructInfo_gr__CH_ls_rtti_c__c_StructInfo_gr_>`
  *  :ref:`each_dim (info:rtti::TypeInfo const) : auto <function-_at_rtti_c__c_each_dim_CH_ls_rtti_c__c_TypeInfo_gr_>`
  *  :ref:`each_dim (info:rtti::VarInfo const) : auto <function-_at_rtti_c__c_each_dim_CH_ls_rtti_c__c_VarInfo_gr_>`
  *  :ref:`arg_types (info:rtti::TypeInfo const) : auto <function-_at_rtti_c__c_arg_types_CH_ls_rtti_c__c_TypeInfo_gr_>`
  *  :ref:`arg_types (info:rtti::VarInfo const) : auto <function-_at_rtti_c__c_arg_types_CH_ls_rtti_c__c_VarInfo_gr_>`
  *  :ref:`arg_names (info:rtti::TypeInfo const) : auto <function-_at_rtti_c__c_arg_names_CH_ls_rtti_c__c_TypeInfo_gr_>`
  *  :ref:`arg_names (info:rtti::VarInfo const) : auto <function-_at_rtti_c__c_arg_names_CH_ls_rtti_c__c_VarInfo_gr_>`

.. _function-_at_rtti_c__c_get_dim_CIH_ls_rtti_c__c_TypeInfo_gr__Ci_C_c_C_l:

.. das:function:: get_dim(typeinfo: TypeInfo const implicit; index: int const)

get_dim returns int

+--------+-------------------------------------------------------------+
+argument+argument type                                                +
+========+=============================================================+
+typeinfo+ :ref:`rtti::TypeInfo <handle-rtti-TypeInfo>`  const implicit+
+--------+-------------------------------------------------------------+
+index   +int const                                                    +
+--------+-------------------------------------------------------------+


|function-rtti-get_dim|

.. _function-_at_rtti_c__c_get_dim_CIH_ls_rtti_c__c_VarInfo_gr__Ci_C_c_C_l:

.. das:function:: get_dim(typeinfo: VarInfo const implicit; index: int const)

get_dim returns int

+--------+-----------------------------------------------------------+
+argument+argument type                                              +
+========+===========================================================+
+typeinfo+ :ref:`rtti::VarInfo <handle-rtti-VarInfo>`  const implicit+
+--------+-----------------------------------------------------------+
+index   +int const                                                  +
+--------+-----------------------------------------------------------+


|function-rtti-get_dim|

.. _function-_at_rtti_c__c_builtin_is_same_type_CI1_ls_CH_ls_rtti_c__c_TypeInfo_gr__gr_?_CI1_ls_CH_ls_rtti_c__c_TypeInfo_gr__gr_?_CE_ls_rtti_c__c_RefMatters_gr__CE_ls_rtti_c__c_ConstMatters_gr__CE_ls_rtti_c__c_TemporaryMatters_gr__Cb:

.. das:function:: builtin_is_same_type(a: rtti::TypeInfo const? const implicit; b: rtti::TypeInfo const? const implicit; refMatters: RefMatters const; cosntMatters: ConstMatters const; tempMatters: TemporaryMatters const; topLevel: bool const)

builtin_is_same_type returns bool

+------------+--------------------------------------------------------------------+
+argument    +argument type                                                       +
+============+====================================================================+
+a           + :ref:`rtti::TypeInfo <handle-rtti-TypeInfo>`  const? const implicit+
+------------+--------------------------------------------------------------------+
+b           + :ref:`rtti::TypeInfo <handle-rtti-TypeInfo>`  const? const implicit+
+------------+--------------------------------------------------------------------+
+refMatters  + :ref:`rtti::RefMatters <enum-rtti-RefMatters>`  const              +
+------------+--------------------------------------------------------------------+
+cosntMatters+ :ref:`rtti::ConstMatters <enum-rtti-ConstMatters>`  const          +
+------------+--------------------------------------------------------------------+
+tempMatters + :ref:`rtti::TemporaryMatters <enum-rtti-TemporaryMatters>`  const  +
+------------+--------------------------------------------------------------------+
+topLevel    +bool const                                                          +
+------------+--------------------------------------------------------------------+


|function-rtti-builtin_is_same_type|

.. _function-_at_rtti_c__c_get_type_size_CI1_ls_H_ls_rtti_c__c_TypeInfo_gr__gr_?:

.. das:function:: get_type_size(type: rtti::TypeInfo? const implicit)

get_type_size returns int

+--------+--------------------------------------------------------------+
+argument+argument type                                                 +
+========+==============================================================+
+type    + :ref:`rtti::TypeInfo <handle-rtti-TypeInfo>` ? const implicit+
+--------+--------------------------------------------------------------+


|function-rtti-get_type_size|

.. _function-_at_rtti_c__c_get_type_align_CI1_ls_H_ls_rtti_c__c_TypeInfo_gr__gr_?:

.. das:function:: get_type_align(type: rtti::TypeInfo? const implicit)

get_type_align returns int

+--------+--------------------------------------------------------------+
+argument+argument type                                                 +
+========+==============================================================+
+type    + :ref:`rtti::TypeInfo <handle-rtti-TypeInfo>` ? const implicit+
+--------+--------------------------------------------------------------+


|function-rtti-get_type_align|

.. _function-_at_rtti_c__c_is_compatible_cast_CI1_ls_CH_ls_rtti_c__c_StructInfo_gr__gr_?_CI1_ls_CH_ls_rtti_c__c_StructInfo_gr__gr_?:

.. das:function:: is_compatible_cast(from: rtti::StructInfo const? const implicit; to: rtti::StructInfo const? const implicit)

is_compatible_cast returns bool

+--------+------------------------------------------------------------------------+
+argument+argument type                                                           +
+========+========================================================================+
+from    + :ref:`rtti::StructInfo <handle-rtti-StructInfo>`  const? const implicit+
+--------+------------------------------------------------------------------------+
+to      + :ref:`rtti::StructInfo <handle-rtti-StructInfo>`  const? const implicit+
+--------+------------------------------------------------------------------------+


|function-rtti-is_compatible_cast|

.. _function-_at_rtti_c__c_get_das_type_name_CE_ls_rtti_c__c_Type_gr__C_c:

.. das:function:: get_das_type_name(type: Type const)

get_das_type_name returns string

+--------+------------------------------------------+
+argument+argument type                             +
+========+==========================================+
+type    + :ref:`rtti::Type <enum-rtti-Type>`  const+
+--------+------------------------------------------+


|function-rtti-get_das_type_name|

.. _function-_at_rtti_c__c_is_same_type_CH_ls_rtti_c__c_TypeInfo_gr__CH_ls_rtti_c__c_TypeInfo_gr__CE_ls_rtti_c__c_RefMatters_gr__CE_ls_rtti_c__c_ConstMatters_gr__CE_ls_rtti_c__c_TemporaryMatters_gr__Cb:

.. das:function:: is_same_type(a: TypeInfo const; b: TypeInfo const; refMatters: RefMatters const; constMatters: ConstMatters const; temporaryMatters: TemporaryMatters const; topLevel: bool const)

is_same_type returns auto

+----------------+------------------------------------------------------------------+
+argument        +argument type                                                     +
+================+==================================================================+
+a               + :ref:`rtti::TypeInfo <handle-rtti-TypeInfo>`  const              +
+----------------+------------------------------------------------------------------+
+b               + :ref:`rtti::TypeInfo <handle-rtti-TypeInfo>`  const              +
+----------------+------------------------------------------------------------------+
+refMatters      + :ref:`rtti::RefMatters <enum-rtti-RefMatters>`  const            +
+----------------+------------------------------------------------------------------+
+constMatters    + :ref:`rtti::ConstMatters <enum-rtti-ConstMatters>`  const        +
+----------------+------------------------------------------------------------------+
+temporaryMatters+ :ref:`rtti::TemporaryMatters <enum-rtti-TemporaryMatters>`  const+
+----------------+------------------------------------------------------------------+
+topLevel        +bool const                                                        +
+----------------+------------------------------------------------------------------+


|function-rtti-is_same_type|

.. _function-_at_rtti_c__c_is_compatible_cast_CH_ls_rtti_c__c_StructInfo_gr__CH_ls_rtti_c__c_StructInfo_gr_:

.. das:function:: is_compatible_cast(a: StructInfo const; b: StructInfo const)

is_compatible_cast returns auto

+--------+--------------------------------------------------------+
+argument+argument type                                           +
+========+========================================================+
+a       + :ref:`rtti::StructInfo <handle-rtti-StructInfo>`  const+
+--------+--------------------------------------------------------+
+b       + :ref:`rtti::StructInfo <handle-rtti-StructInfo>`  const+
+--------+--------------------------------------------------------+


|function-rtti-is_compatible_cast|

.. _function-_at_rtti_c__c_each_dim_CH_ls_rtti_c__c_TypeInfo_gr_:

.. das:function:: each_dim(info: TypeInfo const)

each_dim returns auto

+--------+----------------------------------------------------+
+argument+argument type                                       +
+========+====================================================+
+info    + :ref:`rtti::TypeInfo <handle-rtti-TypeInfo>`  const+
+--------+----------------------------------------------------+


|function-rtti-each_dim|

.. _function-_at_rtti_c__c_each_dim_CH_ls_rtti_c__c_VarInfo_gr_:

.. das:function:: each_dim(info: VarInfo const)

each_dim returns auto

+--------+--------------------------------------------------+
+argument+argument type                                     +
+========+==================================================+
+info    + :ref:`rtti::VarInfo <handle-rtti-VarInfo>`  const+
+--------+--------------------------------------------------+


|function-rtti-each_dim|

.. _function-_at_rtti_c__c_arg_types_CH_ls_rtti_c__c_TypeInfo_gr_:

.. das:function:: arg_types(info: TypeInfo const)

arg_types returns auto

+--------+----------------------------------------------------+
+argument+argument type                                       +
+========+====================================================+
+info    + :ref:`rtti::TypeInfo <handle-rtti-TypeInfo>`  const+
+--------+----------------------------------------------------+


|function-rtti-arg_types|

.. _function-_at_rtti_c__c_arg_types_CH_ls_rtti_c__c_VarInfo_gr_:

.. das:function:: arg_types(info: VarInfo const)

arg_types returns auto

+--------+--------------------------------------------------+
+argument+argument type                                     +
+========+==================================================+
+info    + :ref:`rtti::VarInfo <handle-rtti-VarInfo>`  const+
+--------+--------------------------------------------------+


|function-rtti-arg_types|

.. _function-_at_rtti_c__c_arg_names_CH_ls_rtti_c__c_TypeInfo_gr_:

.. das:function:: arg_names(info: TypeInfo const)

arg_names returns auto

+--------+----------------------------------------------------+
+argument+argument type                                       +
+========+====================================================+
+info    + :ref:`rtti::TypeInfo <handle-rtti-TypeInfo>`  const+
+--------+----------------------------------------------------+


|function-rtti-arg_names|

.. _function-_at_rtti_c__c_arg_names_CH_ls_rtti_c__c_VarInfo_gr_:

.. das:function:: arg_names(info: VarInfo const)

arg_names returns auto

+--------+--------------------------------------------------+
+argument+argument type                                     +
+========+==================================================+
+info    + :ref:`rtti::VarInfo <handle-rtti-VarInfo>`  const+
+--------+--------------------------------------------------+


|function-rtti-arg_names|

+++++++++++++++++++
Rtti context access
+++++++++++++++++++

  *  :ref:`get_total_functions (context:rtti::Context implicit) : int <function-_at_rtti_c__c_get_total_functions_IH_ls_rtti_c__c_Context_gr_>`
  *  :ref:`get_total_variables (context:rtti::Context implicit) : int <function-_at_rtti_c__c_get_total_variables_IH_ls_rtti_c__c_Context_gr_>`
  *  :ref:`get_function_info (context:any;index:int const) : rtti::FuncInfo const& <function-_at_rtti_c__c_get_function_info_*_Ci>`
  *  :ref:`get_variable_info (context:any;index:int const) : rtti::VarInfo const& <function-_at_rtti_c__c_get_variable_info_*_Ci>`
  *  :ref:`get_variable_value (varInfo:rtti::VarInfo const implicit) : variant\<tBool:bool;tInt:int;tUInt:uint;tInt64:int64;tUInt64:uint64;tFloat:float;tDouble:double;tString:string;nothing:any\> <function-_at_rtti_c__c_get_variable_value_CIH_ls_rtti_c__c_VarInfo_gr_>`
  *  :ref:`get_function_info (context:rtti::Context implicit;function:function\<\> const) : rtti::FuncInfo const? const <function-_at_rtti_c__c_get_function_info_IH_ls_rtti_c__c_Context_gr__C_at__at_>`
  *  :ref:`get_function_by_mnh (context:rtti::Context implicit;MNH:uint64 const) : function\<\> <function-_at_rtti_c__c_get_function_by_mnh_IH_ls_rtti_c__c_Context_gr__Cu64>`
  *  :ref:`get_line_info (line:__lineInfo const) : rtti::LineInfo <function-_at_rtti_c__c_get_line_info_C_l>`
  *  :ref:`get_line_info (depth:int const;context:__context const;line:__lineInfo const) : rtti::LineInfo <function-_at_rtti_c__c_get_line_info_Ci_C_c_C_l>`
  *  :ref:`this_context (context:__context const) : rtti::Context& <function-_at_rtti_c__c_this_context_C_c>`
  *  :ref:`context_for_each_function (blk:block\<(info:rtti::FuncInfo const):void\> const) : auto <function-_at_rtti_c__c_context_for_each_function_CN_ls_info_gr_0_ls_CH_ls_rtti_c__c_FuncInfo_gr__gr_1_ls_v_gr__builtin_>`
  *  :ref:`context_for_each_variable (blk:block\<(info:rtti::VarInfo const):void\> const) : auto <function-_at_rtti_c__c_context_for_each_variable_CN_ls_info_gr_0_ls_CH_ls_rtti_c__c_VarInfo_gr__gr_1_ls_v_gr__builtin_>`
  *  :ref:`class_info (cl:auto const) : rtti::StructInfo const? <function-_at_rtti_c__c_class_info_C.>`
  *  :ref:`type_info (vinfo:rtti::LocalVariableInfo const) : rtti::TypeInfo const? <function-_at_rtti_c__c_type_info_CH_ls_rtti_c__c_LocalVariableInfo_gr_>`
  *  :ref:`type_info (vinfo:rtti::VarInfo const) : rtti::TypeInfo const? <function-_at_rtti_c__c_type_info_CH_ls_rtti_c__c_VarInfo_gr_>`

.. _function-_at_rtti_c__c_get_total_functions_IH_ls_rtti_c__c_Context_gr_:

.. das:function:: get_total_functions(context: Context implicit)

get_total_functions returns int

+--------+-----------------------------------------------------+
+argument+argument type                                        +
+========+=====================================================+
+context + :ref:`rtti::Context <handle-rtti-Context>`  implicit+
+--------+-----------------------------------------------------+


|function-rtti-get_total_functions|

.. _function-_at_rtti_c__c_get_total_variables_IH_ls_rtti_c__c_Context_gr_:

.. das:function:: get_total_variables(context: Context implicit)

get_total_variables returns int

+--------+-----------------------------------------------------+
+argument+argument type                                        +
+========+=====================================================+
+context + :ref:`rtti::Context <handle-rtti-Context>`  implicit+
+--------+-----------------------------------------------------+


|function-rtti-get_total_variables|

.. _function-_at_rtti_c__c_get_function_info_*_Ci:

.. das:function:: get_function_info(context: any; index: int const)

get_function_info returns  :ref:`rtti::FuncInfo <handle-rtti-FuncInfo>`  const&

+--------+-------------+
+argument+argument type+
+========+=============+
+context +any          +
+--------+-------------+
+index   +int const    +
+--------+-------------+


|function-rtti-get_function_info|

.. _function-_at_rtti_c__c_get_variable_info_*_Ci:

.. das:function:: get_variable_info(context: any; index: int const)

get_variable_info returns  :ref:`rtti::VarInfo <handle-rtti-VarInfo>`  const&

+--------+-------------+
+argument+argument type+
+========+=============+
+context +any          +
+--------+-------------+
+index   +int const    +
+--------+-------------+


|function-rtti-get_variable_info|

.. _function-_at_rtti_c__c_get_variable_value_CIH_ls_rtti_c__c_VarInfo_gr_:

.. das:function:: get_variable_value(varInfo: VarInfo const implicit)

get_variable_value returns  :ref:`RttiValue <alias-RttiValue>`

+--------+-----------------------------------------------------------+
+argument+argument type                                              +
+========+===========================================================+
+varInfo + :ref:`rtti::VarInfo <handle-rtti-VarInfo>`  const implicit+
+--------+-----------------------------------------------------------+


|function-rtti-get_variable_value|

.. _function-_at_rtti_c__c_get_function_info_IH_ls_rtti_c__c_Context_gr__C_at__at_:

.. das:function:: get_function_info(context: Context implicit; function: function<> const)

get_function_info returns  :ref:`rtti::FuncInfo <handle-rtti-FuncInfo>`  const? const

+--------+-----------------------------------------------------+
+argument+argument type                                        +
+========+=====================================================+
+context + :ref:`rtti::Context <handle-rtti-Context>`  implicit+
+--------+-----------------------------------------------------+
+function+function<> const                                     +
+--------+-----------------------------------------------------+


|function-rtti-get_function_info|

.. _function-_at_rtti_c__c_get_function_by_mnh_IH_ls_rtti_c__c_Context_gr__Cu64:

.. das:function:: get_function_by_mnh(context: Context implicit; MNH: uint64 const)

get_function_by_mnh returns function<>

+--------+-----------------------------------------------------+
+argument+argument type                                        +
+========+=====================================================+
+context + :ref:`rtti::Context <handle-rtti-Context>`  implicit+
+--------+-----------------------------------------------------+
+MNH     +uint64 const                                         +
+--------+-----------------------------------------------------+


|function-rtti-get_function_by_mnh|

.. _function-_at_rtti_c__c_get_line_info_C_l:

.. das:function:: get_line_info()

get_line_info returns  :ref:`rtti::LineInfo <handle-rtti-LineInfo>`

|function-rtti-get_line_info|

.. _function-_at_rtti_c__c_get_line_info_Ci_C_c_C_l:

.. das:function:: get_line_info(depth: int const)

get_line_info returns  :ref:`rtti::LineInfo <handle-rtti-LineInfo>`

+--------+-------------+
+argument+argument type+
+========+=============+
+depth   +int const    +
+--------+-------------+


|function-rtti-get_line_info|

.. _function-_at_rtti_c__c_this_context_C_c:

.. das:function:: this_context()

this_context returns  :ref:`rtti::Context <handle-rtti-Context>` &

|function-rtti-this_context|

.. _function-_at_rtti_c__c_context_for_each_function_CN_ls_info_gr_0_ls_CH_ls_rtti_c__c_FuncInfo_gr__gr_1_ls_v_gr__builtin_:

.. das:function:: context_for_each_function(blk: block<(info:rtti::FuncInfo const):void> const)

context_for_each_function returns auto

+--------+-----------------------------------------------------------------------------+
+argument+argument type                                                                +
+========+=============================================================================+
+blk     +block<(info: :ref:`rtti::FuncInfo <handle-rtti-FuncInfo>`  const):void> const+
+--------+-----------------------------------------------------------------------------+


|function-rtti-context_for_each_function|

.. _function-_at_rtti_c__c_context_for_each_variable_CN_ls_info_gr_0_ls_CH_ls_rtti_c__c_VarInfo_gr__gr_1_ls_v_gr__builtin_:

.. das:function:: context_for_each_variable(blk: block<(info:rtti::VarInfo const):void> const)

context_for_each_variable returns auto

+--------+---------------------------------------------------------------------------+
+argument+argument type                                                              +
+========+===========================================================================+
+blk     +block<(info: :ref:`rtti::VarInfo <handle-rtti-VarInfo>`  const):void> const+
+--------+---------------------------------------------------------------------------+


|function-rtti-context_for_each_variable|

.. _function-_at_rtti_c__c_class_info_C.:

.. das:function:: class_info(cl: auto const)

class_info returns  :ref:`rtti::StructInfo <handle-rtti-StructInfo>`  const?

+--------+-------------+
+argument+argument type+
+========+=============+
+cl      +auto const   +
+--------+-------------+


|function-rtti-class_info|

.. _function-_at_rtti_c__c_type_info_CH_ls_rtti_c__c_LocalVariableInfo_gr_:

.. das:function:: type_info(vinfo: LocalVariableInfo const)

type_info returns  :ref:`rtti::TypeInfo <handle-rtti-TypeInfo>`  const?

+--------+----------------------------------------------------------------------+
+argument+argument type                                                         +
+========+======================================================================+
+vinfo   + :ref:`rtti::LocalVariableInfo <handle-rtti-LocalVariableInfo>`  const+
+--------+----------------------------------------------------------------------+


|function-rtti-type_info|

.. _function-_at_rtti_c__c_type_info_CH_ls_rtti_c__c_VarInfo_gr_:

.. das:function:: type_info(vinfo: VarInfo const)

type_info returns  :ref:`rtti::TypeInfo <handle-rtti-TypeInfo>`  const?

+--------+--------------------------------------------------+
+argument+argument type                                     +
+========+==================================================+
+vinfo   + :ref:`rtti::VarInfo <handle-rtti-VarInfo>`  const+
+--------+--------------------------------------------------+


|function-rtti-type_info|

++++++++++++++
Program access
++++++++++++++

  *  :ref:`get_this_module (program:smart_ptr\<rtti::Program\> const implicit) : rtti::Module? <function-_at_rtti_c__c_get_this_module_CI1_ls_H_ls_rtti_c__c_Program_gr__gr_?M>`
  *  :ref:`get_module (name:string const implicit) : rtti::Module? <function-_at_rtti_c__c_get_module_CIs>`
  *  :ref:`program_for_each_module (program:smart_ptr\<rtti::Program\> const implicit;block:block\<(var arg0:rtti::Module?):void\> const implicit;context:__context const;line:__lineInfo const) : void <function-_at_rtti_c__c_program_for_each_module_CI1_ls_H_ls_rtti_c__c_Program_gr__gr_?M_CI0_ls_1_ls_H_ls_rtti_c__c_Module_gr__gr_?_gr_1_ls_v_gr__builtin__C_c_C_l>`
  *  :ref:`program_for_each_registered_module (block:block\<(var arg0:rtti::Module?):void\> const implicit;context:__context const;line:__lineInfo const) : void <function-_at_rtti_c__c_program_for_each_registered_module_CI0_ls_1_ls_H_ls_rtti_c__c_Module_gr__gr_?_gr_1_ls_v_gr__builtin__C_c_C_l>`

.. _function-_at_rtti_c__c_get_this_module_CI1_ls_H_ls_rtti_c__c_Program_gr__gr_?M:

.. das:function:: get_this_module(program: smart_ptr<rtti::Program> const implicit)

get_this_module returns  :ref:`rtti::Module <handle-rtti-Module>` ?

+--------+----------------------------------------------------------------------+
+argument+argument type                                                         +
+========+======================================================================+
+program +smart_ptr< :ref:`rtti::Program <handle-rtti-Program>` > const implicit+
+--------+----------------------------------------------------------------------+


|function-rtti-get_this_module|

.. _function-_at_rtti_c__c_get_module_CIs:

.. das:function:: get_module(name: string const implicit)

get_module returns  :ref:`rtti::Module <handle-rtti-Module>` ?

+--------+---------------------+
+argument+argument type        +
+========+=====================+
+name    +string const implicit+
+--------+---------------------+


|function-rtti-get_module|

.. _function-_at_rtti_c__c_program_for_each_module_CI1_ls_H_ls_rtti_c__c_Program_gr__gr_?M_CI0_ls_1_ls_H_ls_rtti_c__c_Module_gr__gr_?_gr_1_ls_v_gr__builtin__C_c_C_l:

.. das:function:: program_for_each_module(program: smart_ptr<rtti::Program> const implicit; block: block<(var arg0:rtti::Module?):void> const implicit)

+--------+------------------------------------------------------------------------+
+argument+argument type                                                           +
+========+========================================================================+
+program +smart_ptr< :ref:`rtti::Program <handle-rtti-Program>` > const implicit  +
+--------+------------------------------------------------------------------------+
+block   +block<( :ref:`rtti::Module <handle-rtti-Module>` ?):void> const implicit+
+--------+------------------------------------------------------------------------+


|function-rtti-program_for_each_module|

.. _function-_at_rtti_c__c_program_for_each_registered_module_CI0_ls_1_ls_H_ls_rtti_c__c_Module_gr__gr_?_gr_1_ls_v_gr__builtin__C_c_C_l:

.. das:function:: program_for_each_registered_module(block: block<(var arg0:rtti::Module?):void> const implicit)

+--------+------------------------------------------------------------------------+
+argument+argument type                                                           +
+========+========================================================================+
+block   +block<( :ref:`rtti::Module <handle-rtti-Module>` ?):void> const implicit+
+--------+------------------------------------------------------------------------+


|function-rtti-program_for_each_registered_module|

+++++++++++++
Module access
+++++++++++++

  *  :ref:`module_for_each_structure (module:rtti::Module? const implicit;block:block\<(arg0:rtti::StructInfo const):void\> const implicit;context:__context const;line:__lineInfo const) : void <function-_at_rtti_c__c_module_for_each_structure_CI1_ls_H_ls_rtti_c__c_Module_gr__gr_?_CI0_ls_CH_ls_rtti_c__c_StructInfo_gr__gr_1_ls_v_gr__builtin__C_c_C_l>`
  *  :ref:`module_for_each_enumeration (module:rtti::Module? const implicit;block:block\<(arg0:rtti::EnumInfo const):void\> const implicit;context:__context const;line:__lineInfo const) : void <function-_at_rtti_c__c_module_for_each_enumeration_CI1_ls_H_ls_rtti_c__c_Module_gr__gr_?_CI0_ls_CH_ls_rtti_c__c_EnumInfo_gr__gr_1_ls_v_gr__builtin__C_c_C_l>`
  *  :ref:`module_for_each_function (module:rtti::Module? const implicit;block:block\<(arg0:rtti::FuncInfo const):void\> const implicit;context:__context const;line:__lineInfo const) : void <function-_at_rtti_c__c_module_for_each_function_CI1_ls_H_ls_rtti_c__c_Module_gr__gr_?_CI0_ls_CH_ls_rtti_c__c_FuncInfo_gr__gr_1_ls_v_gr__builtin__C_c_C_l>`
  *  :ref:`module_for_each_generic (module:rtti::Module? const implicit;block:block\<(arg0:rtti::FuncInfo const):void\> const implicit;context:__context const;line:__lineInfo const) : void <function-_at_rtti_c__c_module_for_each_generic_CI1_ls_H_ls_rtti_c__c_Module_gr__gr_?_CI0_ls_CH_ls_rtti_c__c_FuncInfo_gr__gr_1_ls_v_gr__builtin__C_c_C_l>`
  *  :ref:`module_for_each_global (module:rtti::Module? const implicit;block:block\<(arg0:rtti::VarInfo const):void\> const implicit;context:__context const;line:__lineInfo const) : void <function-_at_rtti_c__c_module_for_each_global_CI1_ls_H_ls_rtti_c__c_Module_gr__gr_?_CI0_ls_CH_ls_rtti_c__c_VarInfo_gr__gr_1_ls_v_gr__builtin__C_c_C_l>`
  *  :ref:`module_for_each_annotation (module:rtti::Module? const implicit;block:block\<(arg0:rtti::Annotation const):void\> const implicit;context:__context const;line:__lineInfo const) : void <function-_at_rtti_c__c_module_for_each_annotation_CI1_ls_H_ls_rtti_c__c_Module_gr__gr_?_CI0_ls_CH_ls_rtti_c__c_Annotation_gr__gr_1_ls_v_gr__builtin__C_c_C_l>`

.. _function-_at_rtti_c__c_module_for_each_structure_CI1_ls_H_ls_rtti_c__c_Module_gr__gr_?_CI0_ls_CH_ls_rtti_c__c_StructInfo_gr__gr_1_ls_v_gr__builtin__C_c_C_l:

.. das:function:: module_for_each_structure(module: rtti::Module? const implicit; block: block<(arg0:rtti::StructInfo const):void> const implicit)

+--------+-------------------------------------------------------------------------------------+
+argument+argument type                                                                        +
+========+=====================================================================================+
+module  + :ref:`rtti::Module <handle-rtti-Module>` ? const implicit                           +
+--------+-------------------------------------------------------------------------------------+
+block   +block<( :ref:`rtti::StructInfo <handle-rtti-StructInfo>`  const):void> const implicit+
+--------+-------------------------------------------------------------------------------------+


|function-rtti-module_for_each_structure|

.. _function-_at_rtti_c__c_module_for_each_enumeration_CI1_ls_H_ls_rtti_c__c_Module_gr__gr_?_CI0_ls_CH_ls_rtti_c__c_EnumInfo_gr__gr_1_ls_v_gr__builtin__C_c_C_l:

.. das:function:: module_for_each_enumeration(module: rtti::Module? const implicit; block: block<(arg0:rtti::EnumInfo const):void> const implicit)

+--------+---------------------------------------------------------------------------------+
+argument+argument type                                                                    +
+========+=================================================================================+
+module  + :ref:`rtti::Module <handle-rtti-Module>` ? const implicit                       +
+--------+---------------------------------------------------------------------------------+
+block   +block<( :ref:`rtti::EnumInfo <handle-rtti-EnumInfo>`  const):void> const implicit+
+--------+---------------------------------------------------------------------------------+


|function-rtti-module_for_each_enumeration|

.. _function-_at_rtti_c__c_module_for_each_function_CI1_ls_H_ls_rtti_c__c_Module_gr__gr_?_CI0_ls_CH_ls_rtti_c__c_FuncInfo_gr__gr_1_ls_v_gr__builtin__C_c_C_l:

.. das:function:: module_for_each_function(module: rtti::Module? const implicit; block: block<(arg0:rtti::FuncInfo const):void> const implicit)

+--------+---------------------------------------------------------------------------------+
+argument+argument type                                                                    +
+========+=================================================================================+
+module  + :ref:`rtti::Module <handle-rtti-Module>` ? const implicit                       +
+--------+---------------------------------------------------------------------------------+
+block   +block<( :ref:`rtti::FuncInfo <handle-rtti-FuncInfo>`  const):void> const implicit+
+--------+---------------------------------------------------------------------------------+


|function-rtti-module_for_each_function|

.. _function-_at_rtti_c__c_module_for_each_generic_CI1_ls_H_ls_rtti_c__c_Module_gr__gr_?_CI0_ls_CH_ls_rtti_c__c_FuncInfo_gr__gr_1_ls_v_gr__builtin__C_c_C_l:

.. das:function:: module_for_each_generic(module: rtti::Module? const implicit; block: block<(arg0:rtti::FuncInfo const):void> const implicit)

+--------+---------------------------------------------------------------------------------+
+argument+argument type                                                                    +
+========+=================================================================================+
+module  + :ref:`rtti::Module <handle-rtti-Module>` ? const implicit                       +
+--------+---------------------------------------------------------------------------------+
+block   +block<( :ref:`rtti::FuncInfo <handle-rtti-FuncInfo>`  const):void> const implicit+
+--------+---------------------------------------------------------------------------------+


|function-rtti-module_for_each_generic|

.. _function-_at_rtti_c__c_module_for_each_global_CI1_ls_H_ls_rtti_c__c_Module_gr__gr_?_CI0_ls_CH_ls_rtti_c__c_VarInfo_gr__gr_1_ls_v_gr__builtin__C_c_C_l:

.. das:function:: module_for_each_global(module: rtti::Module? const implicit; block: block<(arg0:rtti::VarInfo const):void> const implicit)

+--------+-------------------------------------------------------------------------------+
+argument+argument type                                                                  +
+========+===============================================================================+
+module  + :ref:`rtti::Module <handle-rtti-Module>` ? const implicit                     +
+--------+-------------------------------------------------------------------------------+
+block   +block<( :ref:`rtti::VarInfo <handle-rtti-VarInfo>`  const):void> const implicit+
+--------+-------------------------------------------------------------------------------+


|function-rtti-module_for_each_global|

.. _function-_at_rtti_c__c_module_for_each_annotation_CI1_ls_H_ls_rtti_c__c_Module_gr__gr_?_CI0_ls_CH_ls_rtti_c__c_Annotation_gr__gr_1_ls_v_gr__builtin__C_c_C_l:

.. das:function:: module_for_each_annotation(module: rtti::Module? const implicit; block: block<(arg0:rtti::Annotation const):void> const implicit)

+--------+-------------------------------------------------------------------------------------+
+argument+argument type                                                                        +
+========+=====================================================================================+
+module  + :ref:`rtti::Module <handle-rtti-Module>` ? const implicit                           +
+--------+-------------------------------------------------------------------------------------+
+block   +block<( :ref:`rtti::Annotation <handle-rtti-Annotation>`  const):void> const implicit+
+--------+-------------------------------------------------------------------------------------+


|function-rtti-module_for_each_annotation|

+++++++++++++++++
Annotation access
+++++++++++++++++

  *  :ref:`get_annotation_argument_value (info:rtti::AnnotationArgument const implicit;context:__context const) : variant\<tBool:bool;tInt:int;tUInt:uint;tInt64:int64;tUInt64:uint64;tFloat:float;tDouble:double;tString:string;nothing:any\> <function-_at_rtti_c__c_get_annotation_argument_value_CIH_ls_rtti_c__c_AnnotationArgument_gr__C_c>`
  *  :ref:`add_annotation_argument (annotation:rtti::AnnotationArgumentList implicit;name:string const implicit) : int <function-_at_rtti_c__c_add_annotation_argument_IH_ls_rtti_c__c_AnnotationArgumentList_gr__CIs>`

.. _function-_at_rtti_c__c_get_annotation_argument_value_CIH_ls_rtti_c__c_AnnotationArgument_gr__C_c:

.. das:function:: get_annotation_argument_value(info: AnnotationArgument const implicit)

get_annotation_argument_value returns  :ref:`RttiValue <alias-RttiValue>`

+--------+---------------------------------------------------------------------------------+
+argument+argument type                                                                    +
+========+=================================================================================+
+info    + :ref:`rtti::AnnotationArgument <handle-rtti-AnnotationArgument>`  const implicit+
+--------+---------------------------------------------------------------------------------+


|function-rtti-get_annotation_argument_value|

.. _function-_at_rtti_c__c_add_annotation_argument_IH_ls_rtti_c__c_AnnotationArgumentList_gr__CIs:

.. das:function:: add_annotation_argument(annotation: AnnotationArgumentList implicit; name: string const implicit)

add_annotation_argument returns int

+----------+-----------------------------------------------------------------------------------+
+argument  +argument type                                                                      +
+==========+===================================================================================+
+annotation+ :ref:`rtti::AnnotationArgumentList <handle-rtti-AnnotationArgumentList>`  implicit+
+----------+-----------------------------------------------------------------------------------+
+name      +string const implicit                                                              +
+----------+-----------------------------------------------------------------------------------+


|function-rtti-add_annotation_argument|

++++++++++++++++++++++++++
Compilation and simulation
++++++++++++++++++++++++++

  *  :ref:`compile (module_name:string const implicit;codeText:string const implicit;codeOfPolicies:rtti::CodeOfPolicies const implicit;block:block\<(var arg0:bool;var arg1:smart_ptr\<rtti::Program\>;arg2:$::das_string const):void\> const implicit;context:__context const;line:__lineInfo const) : void <function-_at_rtti_c__c_compile_CIs_CIs_CIH_ls_rtti_c__c_CodeOfPolicies_gr__CI0_ls_b;1_ls_H_ls_rtti_c__c_Program_gr__gr_?W;CH_ls__builtin__c__c_das_string_gr__gr_1_ls_v_gr__builtin__C_c_C_l>`
  *  :ref:`compile (module_name:string const implicit;codeText:string const implicit;codeOfPolicies:rtti::CodeOfPolicies const implicit;exportAll:bool const;block:block\<(var arg0:bool;var arg1:smart_ptr\<rtti::Program\>;arg2:$::das_string const):void\> const implicit;context:__context const;line:__lineInfo const) : void <function-_at_rtti_c__c_compile_CIs_CIs_CIH_ls_rtti_c__c_CodeOfPolicies_gr__Cb_CI0_ls_b;1_ls_H_ls_rtti_c__c_Program_gr__gr_?W;CH_ls__builtin__c__c_das_string_gr__gr_1_ls_v_gr__builtin__C_c_C_l>`
  *  :ref:`compile_file (module_name:string const implicit;fileAccess:smart_ptr\<rtti::FileAccess\> const implicit;moduleGroup:rtti::ModuleGroup? const implicit;codeOfPolicies:rtti::CodeOfPolicies const implicit;block:block\<(var arg0:bool;var arg1:smart_ptr\<rtti::Program\>;arg2:$::das_string const):void\> const implicit;context:__context const;line:__lineInfo const) : void <function-_at_rtti_c__c_compile_file_CIs_CI1_ls_H_ls_rtti_c__c_FileAccess_gr__gr_?W_CI1_ls_H_ls_rtti_c__c_ModuleGroup_gr__gr_?_CIH_ls_rtti_c__c_CodeOfPolicies_gr__CI0_ls_b;1_ls_H_ls_rtti_c__c_Program_gr__gr_?W;CH_ls__builtin__c__c_das_string_gr__gr_1_ls_v_gr__builtin__C_c_C_l>`
  *  :ref:`for_each_expected_error (program:smart_ptr\<rtti::Program\> const implicit;block:block\<(var arg0:rtti::CompilationError;var arg1:int):void\> const implicit;context:__context const;line:__lineInfo const) : void <function-_at_rtti_c__c_for_each_expected_error_CI1_ls_H_ls_rtti_c__c_Program_gr__gr_?W_CI0_ls_E_ls_rtti_c__c_CompilationError_gr_;i_gr_1_ls_v_gr__builtin__C_c_C_l>`
  *  :ref:`for_each_require_declaration (program:smart_ptr\<rtti::Program\> const implicit;block:block\<(var arg0:rtti::Module?;arg1:string const#;arg2:string const#;var arg3:bool;arg4:rtti::LineInfo const&):void\> const implicit;context:__context const;line:__lineInfo const) : void <function-_at_rtti_c__c_for_each_require_declaration_CI1_ls_H_ls_rtti_c__c_Program_gr__gr_?W_CI0_ls_1_ls_H_ls_rtti_c__c_Module_gr__gr_?;C_hh_s;C_hh_s;b;C&H_ls_rtti_c__c_LineInfo_gr__gr_1_ls_v_gr__builtin__C_c_C_l>`
  *  :ref:`simulate (program:smart_ptr\<rtti::Program\> const& implicit;block:block\<(var arg0:bool;var arg1:smart_ptr\<rtti::Context\>;var arg2:$::das_string):void\> const implicit;context:__context const;line:__lineInfo const) : void <function-_at_rtti_c__c_simulate_C&I1_ls_H_ls_rtti_c__c_Program_gr__gr_?W_CI0_ls_b;1_ls_H_ls_rtti_c__c_Context_gr__gr_?W;H_ls__builtin__c__c_das_string_gr__gr_1_ls_v_gr__builtin__C_c_C_l>`

.. _function-_at_rtti_c__c_compile_CIs_CIs_CIH_ls_rtti_c__c_CodeOfPolicies_gr__CI0_ls_b;1_ls_H_ls_rtti_c__c_Program_gr__gr_?W;CH_ls__builtin__c__c_das_string_gr__gr_1_ls_v_gr__builtin__C_c_C_l:

.. das:function:: compile(module_name: string const implicit; codeText: string const implicit; codeOfPolicies: CodeOfPolicies const implicit; block: block<(var arg0:bool;var arg1:smart_ptr<rtti::Program>;arg2:das_string const):void> const implicit)

+--------------+--------------------------------------------------------------------------------------------------------------------------------------------------------+
+argument      +argument type                                                                                                                                           +
+==============+========================================================================================================================================================+
+module_name   +string const implicit                                                                                                                                   +
+--------------+--------------------------------------------------------------------------------------------------------------------------------------------------------+
+codeText      +string const implicit                                                                                                                                   +
+--------------+--------------------------------------------------------------------------------------------------------------------------------------------------------+
+codeOfPolicies+ :ref:`rtti::CodeOfPolicies <handle-rtti-CodeOfPolicies>`  const implicit                                                                               +
+--------------+--------------------------------------------------------------------------------------------------------------------------------------------------------+
+block         +block<(bool;smart_ptr< :ref:`rtti::Program <handle-rtti-Program>` >; :ref:`builtin::das_string <handle-builtin-das_string>`  const):void> const implicit+
+--------------+--------------------------------------------------------------------------------------------------------------------------------------------------------+


|function-rtti-compile|

.. _function-_at_rtti_c__c_compile_CIs_CIs_CIH_ls_rtti_c__c_CodeOfPolicies_gr__Cb_CI0_ls_b;1_ls_H_ls_rtti_c__c_Program_gr__gr_?W;CH_ls__builtin__c__c_das_string_gr__gr_1_ls_v_gr__builtin__C_c_C_l:

.. das:function:: compile(module_name: string const implicit; codeText: string const implicit; codeOfPolicies: CodeOfPolicies const implicit; exportAll: bool const; block: block<(var arg0:bool;var arg1:smart_ptr<rtti::Program>;arg2:das_string const):void> const implicit)

+--------------+--------------------------------------------------------------------------------------------------------------------------------------------------------+
+argument      +argument type                                                                                                                                           +
+==============+========================================================================================================================================================+
+module_name   +string const implicit                                                                                                                                   +
+--------------+--------------------------------------------------------------------------------------------------------------------------------------------------------+
+codeText      +string const implicit                                                                                                                                   +
+--------------+--------------------------------------------------------------------------------------------------------------------------------------------------------+
+codeOfPolicies+ :ref:`rtti::CodeOfPolicies <handle-rtti-CodeOfPolicies>`  const implicit                                                                               +
+--------------+--------------------------------------------------------------------------------------------------------------------------------------------------------+
+exportAll     +bool const                                                                                                                                              +
+--------------+--------------------------------------------------------------------------------------------------------------------------------------------------------+
+block         +block<(bool;smart_ptr< :ref:`rtti::Program <handle-rtti-Program>` >; :ref:`builtin::das_string <handle-builtin-das_string>`  const):void> const implicit+
+--------------+--------------------------------------------------------------------------------------------------------------------------------------------------------+


|function-rtti-compile|

.. _function-_at_rtti_c__c_compile_file_CIs_CI1_ls_H_ls_rtti_c__c_FileAccess_gr__gr_?W_CI1_ls_H_ls_rtti_c__c_ModuleGroup_gr__gr_?_CIH_ls_rtti_c__c_CodeOfPolicies_gr__CI0_ls_b;1_ls_H_ls_rtti_c__c_Program_gr__gr_?W;CH_ls__builtin__c__c_das_string_gr__gr_1_ls_v_gr__builtin__C_c_C_l:

.. das:function:: compile_file(module_name: string const implicit; fileAccess: smart_ptr<rtti::FileAccess> const implicit; moduleGroup: rtti::ModuleGroup? const implicit; codeOfPolicies: CodeOfPolicies const implicit; block: block<(var arg0:bool;var arg1:smart_ptr<rtti::Program>;arg2:das_string const):void> const implicit)

+--------------+--------------------------------------------------------------------------------------------------------------------------------------------------------+
+argument      +argument type                                                                                                                                           +
+==============+========================================================================================================================================================+
+module_name   +string const implicit                                                                                                                                   +
+--------------+--------------------------------------------------------------------------------------------------------------------------------------------------------+
+fileAccess    +smart_ptr< :ref:`rtti::FileAccess <handle-rtti-FileAccess>` > const implicit                                                                            +
+--------------+--------------------------------------------------------------------------------------------------------------------------------------------------------+
+moduleGroup   + :ref:`rtti::ModuleGroup <handle-rtti-ModuleGroup>` ? const implicit                                                                                    +
+--------------+--------------------------------------------------------------------------------------------------------------------------------------------------------+
+codeOfPolicies+ :ref:`rtti::CodeOfPolicies <handle-rtti-CodeOfPolicies>`  const implicit                                                                               +
+--------------+--------------------------------------------------------------------------------------------------------------------------------------------------------+
+block         +block<(bool;smart_ptr< :ref:`rtti::Program <handle-rtti-Program>` >; :ref:`builtin::das_string <handle-builtin-das_string>`  const):void> const implicit+
+--------------+--------------------------------------------------------------------------------------------------------------------------------------------------------+


|function-rtti-compile_file|

.. _function-_at_rtti_c__c_for_each_expected_error_CI1_ls_H_ls_rtti_c__c_Program_gr__gr_?W_CI0_ls_E_ls_rtti_c__c_CompilationError_gr_;i_gr_1_ls_v_gr__builtin__C_c_C_l:

.. das:function:: for_each_expected_error(program: smart_ptr<rtti::Program> const implicit; block: block<(var arg0:rtti::CompilationError;var arg1:int):void> const implicit)

+--------+---------------------------------------------------------------------------------------------+
+argument+argument type                                                                                +
+========+=============================================================================================+
+program +smart_ptr< :ref:`rtti::Program <handle-rtti-Program>` > const implicit                       +
+--------+---------------------------------------------------------------------------------------------+
+block   +block<( :ref:`rtti::CompilationError <enum-rtti-CompilationError>` ;int):void> const implicit+
+--------+---------------------------------------------------------------------------------------------+


|function-rtti-for_each_expected_error|

.. _function-_at_rtti_c__c_for_each_require_declaration_CI1_ls_H_ls_rtti_c__c_Program_gr__gr_?W_CI0_ls_1_ls_H_ls_rtti_c__c_Module_gr__gr_?;C_hh_s;C_hh_s;b;C&H_ls_rtti_c__c_LineInfo_gr__gr_1_ls_v_gr__builtin__C_c_C_l:

.. das:function:: for_each_require_declaration(program: smart_ptr<rtti::Program> const implicit; block: block<(var arg0:rtti::Module?;arg1:string const#;arg2:string const#;var arg3:bool;arg4:rtti::LineInfo const&):void> const implicit)

+--------+---------------------------------------------------------------------------------------------------------------------------------------------------------------+
+argument+argument type                                                                                                                                                  +
+========+===============================================================================================================================================================+
+program +smart_ptr< :ref:`rtti::Program <handle-rtti-Program>` > const implicit                                                                                         +
+--------+---------------------------------------------------------------------------------------------------------------------------------------------------------------+
+block   +block<( :ref:`rtti::Module <handle-rtti-Module>` ?;string const#;string const#;bool; :ref:`rtti::LineInfo <handle-rtti-LineInfo>`  const&):void> const implicit+
+--------+---------------------------------------------------------------------------------------------------------------------------------------------------------------+


|function-rtti-for_each_require_declaration|

.. _function-_at_rtti_c__c_simulate_C&I1_ls_H_ls_rtti_c__c_Program_gr__gr_?W_CI0_ls_b;1_ls_H_ls_rtti_c__c_Context_gr__gr_?W;H_ls__builtin__c__c_das_string_gr__gr_1_ls_v_gr__builtin__C_c_C_l:

.. das:function:: simulate(program: smart_ptr<rtti::Program> const& implicit; block: block<(var arg0:bool;var arg1:smart_ptr<rtti::Context>;var arg2:das_string):void> const implicit)

+--------+--------------------------------------------------------------------------------------------------------------------------------------------------+
+argument+argument type                                                                                                                                     +
+========+==================================================================================================================================================+
+program +smart_ptr< :ref:`rtti::Program <handle-rtti-Program>` > const& implicit                                                                           +
+--------+--------------------------------------------------------------------------------------------------------------------------------------------------+
+block   +block<(bool;smart_ptr< :ref:`rtti::Context <handle-rtti-Context>` >; :ref:`builtin::das_string <handle-builtin-das_string>` ):void> const implicit+
+--------+--------------------------------------------------------------------------------------------------------------------------------------------------+


|function-rtti-simulate|

+++++++++++
File access
+++++++++++

  *  :ref:`make_file_access (project:string const implicit;context:__context const;at:__lineInfo const) : smart_ptr\<rtti::FileAccess\> <function-_at_rtti_c__c_make_file_access_CIs_C_c_C_l>`
  *  :ref:`set_file_source (access:smart_ptr\<rtti::FileAccess\> const implicit;fileName:string const implicit;text:string const implicit;context:__context const;line:__lineInfo const) : bool <function-_at_rtti_c__c_set_file_source_CI1_ls_H_ls_rtti_c__c_FileAccess_gr__gr_?M_CIs_CIs_C_c_C_l>`
  *  :ref:`add_file_access_root (access:smart_ptr\<rtti::FileAccess\> const implicit;mod:string const implicit;path:string const implicit) : bool <function-_at_rtti_c__c_add_file_access_root_CI1_ls_H_ls_rtti_c__c_FileAccess_gr__gr_?W_CIs_CIs>`

.. _function-_at_rtti_c__c_make_file_access_CIs_C_c_C_l:

.. das:function:: make_file_access(project: string const implicit)

make_file_access returns smart_ptr< :ref:`rtti::FileAccess <handle-rtti-FileAccess>` >

+--------+---------------------+
+argument+argument type        +
+========+=====================+
+project +string const implicit+
+--------+---------------------+


|function-rtti-make_file_access|

.. _function-_at_rtti_c__c_set_file_source_CI1_ls_H_ls_rtti_c__c_FileAccess_gr__gr_?M_CIs_CIs_C_c_C_l:

.. das:function:: set_file_source(access: smart_ptr<rtti::FileAccess> const implicit; fileName: string const implicit; text: string const implicit)

set_file_source returns bool

+--------+----------------------------------------------------------------------------+
+argument+argument type                                                               +
+========+============================================================================+
+access  +smart_ptr< :ref:`rtti::FileAccess <handle-rtti-FileAccess>` > const implicit+
+--------+----------------------------------------------------------------------------+
+fileName+string const implicit                                                       +
+--------+----------------------------------------------------------------------------+
+text    +string const implicit                                                       +
+--------+----------------------------------------------------------------------------+


|function-rtti-set_file_source|

.. _function-_at_rtti_c__c_add_file_access_root_CI1_ls_H_ls_rtti_c__c_FileAccess_gr__gr_?W_CIs_CIs:

.. das:function:: add_file_access_root(access: smart_ptr<rtti::FileAccess> const implicit; mod: string const implicit; path: string const implicit)

add_file_access_root returns bool

+--------+----------------------------------------------------------------------------+
+argument+argument type                                                               +
+========+============================================================================+
+access  +smart_ptr< :ref:`rtti::FileAccess <handle-rtti-FileAccess>` > const implicit+
+--------+----------------------------------------------------------------------------+
+mod     +string const implicit                                                       +
+--------+----------------------------------------------------------------------------+
+path    +string const implicit                                                       +
+--------+----------------------------------------------------------------------------+


|function-rtti-add_file_access_root|

++++++++++++++++
Structure access
++++++++++++++++

  *  :ref:`rtti_builtin_structure_for_each_annotation (struct:rtti::StructInfo const implicit;block:block\<\> const implicit;context:__context const;line:__lineInfo const) : void <function-_at_rtti_c__c_rtti_builtin_structure_for_each_annotation_CIH_ls_rtti_c__c_StructInfo_gr__CI_builtin__C_c_C_l>`
  *  :ref:`basic_struct_for_each_field (annotation:rtti::BasicStructureAnnotation const implicit;block:block\<(var arg0:string;var arg1:string;arg2:rtti::TypeInfo const;var arg3:uint):void\> const implicit;context:__context const;line:__lineInfo const) : void <function-_at_rtti_c__c_basic_struct_for_each_field_CIH_ls_rtti_c__c_BasicStructureAnnotation_gr__CI0_ls_s;s;CH_ls_rtti_c__c_TypeInfo_gr_;u_gr_1_ls_v_gr__builtin__C_c_C_l>`
  *  :ref:`basic_struct_for_each_parent (annotation:rtti::BasicStructureAnnotation const implicit;block:block\<(var arg0:rtti::Annotation?):void\> const implicit;context:__context const;line:__lineInfo const) : void <function-_at_rtti_c__c_basic_struct_for_each_parent_CIH_ls_rtti_c__c_BasicStructureAnnotation_gr__CI0_ls_1_ls_H_ls_rtti_c__c_Annotation_gr__gr_?_gr_1_ls_v_gr__builtin__C_c_C_l>`
  *  :ref:`structure_for_each_annotation (st:rtti::StructInfo const;subexpr:block\<(ann:rtti::Annotation const;args:rtti::AnnotationArguments const):void\> const) : auto <function-_at_rtti_c__c_structure_for_each_annotation_CH_ls_rtti_c__c_StructInfo_gr__CN_ls_ann;args_gr_0_ls_CH_ls_rtti_c__c_Annotation_gr_;CH_ls_rtti_c__c_AnnotationArguments_gr__gr_1_ls_v_gr__builtin_>`

.. _function-_at_rtti_c__c_rtti_builtin_structure_for_each_annotation_CIH_ls_rtti_c__c_StructInfo_gr__CI_builtin__C_c_C_l:

.. das:function:: rtti_builtin_structure_for_each_annotation(struct: StructInfo const implicit; block: block<> const implicit)

+--------+-----------------------------------------------------------------+
+argument+argument type                                                    +
+========+=================================================================+
+struct  + :ref:`rtti::StructInfo <handle-rtti-StructInfo>`  const implicit+
+--------+-----------------------------------------------------------------+
+block   +block<> const implicit                                           +
+--------+-----------------------------------------------------------------+


|function-rtti-rtti_builtin_structure_for_each_annotation|

.. _function-_at_rtti_c__c_basic_struct_for_each_field_CIH_ls_rtti_c__c_BasicStructureAnnotation_gr__CI0_ls_s;s;CH_ls_rtti_c__c_TypeInfo_gr_;u_gr_1_ls_v_gr__builtin__C_c_C_l:

.. das:function:: basic_struct_for_each_field(annotation: BasicStructureAnnotation const implicit; block: block<(var arg0:string;var arg1:string;arg2:rtti::TypeInfo const;var arg3:uint):void> const implicit)

+----------+----------------------------------------------------------------------------------------------------+
+argument  +argument type                                                                                       +
+==========+====================================================================================================+
+annotation+ :ref:`rtti::BasicStructureAnnotation <handle-rtti-BasicStructureAnnotation>`  const implicit       +
+----------+----------------------------------------------------------------------------------------------------+
+block     +block<(string;string; :ref:`rtti::TypeInfo <handle-rtti-TypeInfo>`  const;uint):void> const implicit+
+----------+----------------------------------------------------------------------------------------------------+


|function-rtti-basic_struct_for_each_field|

.. _function-_at_rtti_c__c_basic_struct_for_each_parent_CIH_ls_rtti_c__c_BasicStructureAnnotation_gr__CI0_ls_1_ls_H_ls_rtti_c__c_Annotation_gr__gr_?_gr_1_ls_v_gr__builtin__C_c_C_l:

.. das:function:: basic_struct_for_each_parent(annotation: BasicStructureAnnotation const implicit; block: block<(var arg0:rtti::Annotation?):void> const implicit)

+----------+---------------------------------------------------------------------------------------------+
+argument  +argument type                                                                                +
+==========+=============================================================================================+
+annotation+ :ref:`rtti::BasicStructureAnnotation <handle-rtti-BasicStructureAnnotation>`  const implicit+
+----------+---------------------------------------------------------------------------------------------+
+block     +block<( :ref:`rtti::Annotation <handle-rtti-Annotation>` ?):void> const implicit             +
+----------+---------------------------------------------------------------------------------------------+


|function-rtti-basic_struct_for_each_parent|

.. _function-_at_rtti_c__c_structure_for_each_annotation_CH_ls_rtti_c__c_StructInfo_gr__CN_ls_ann;args_gr_0_ls_CH_ls_rtti_c__c_Annotation_gr_;CH_ls_rtti_c__c_AnnotationArguments_gr__gr_1_ls_v_gr__builtin_:

.. das:function:: structure_for_each_annotation(st: StructInfo const; subexpr: block<(ann:rtti::Annotation const;args:rtti::AnnotationArguments const):void> const)

structure_for_each_annotation returns auto

+--------+----------------------------------------------------------------------------------------------------------------------------------------------------------------+
+argument+argument type                                                                                                                                                   +
+========+================================================================================================================================================================+
+st      + :ref:`rtti::StructInfo <handle-rtti-StructInfo>`  const                                                                                                        +
+--------+----------------------------------------------------------------------------------------------------------------------------------------------------------------+
+subexpr +block<(ann: :ref:`rtti::Annotation <handle-rtti-Annotation>`  const;args: :ref:`rtti::AnnotationArguments <handle-rtti-AnnotationArguments>`  const):void> const+
+--------+----------------------------------------------------------------------------------------------------------------------------------------------------------------+


|function-rtti-structure_for_each_annotation|

+++++++++++++++++++++++++
Data walking and printing
+++++++++++++++++++++++++

  *  :ref:`sprint_data (data:void? const implicit;type:rtti::TypeInfo const? const implicit;flags:bitfield const;context:__context const) : string <function-_at_rtti_c__c_sprint_data_CI?_CI1_ls_CH_ls_rtti_c__c_TypeInfo_gr__gr_?_Ct_C_c>`
  *  :ref:`sprint_data (data:float4 const;type:rtti::TypeInfo const? const implicit;flags:bitfield const;context:__context const) : string <function-_at_rtti_c__c_sprint_data_Cf4_CI1_ls_CH_ls_rtti_c__c_TypeInfo_gr__gr_?_Ct_C_c>`
  *  :ref:`describe (type:rtti::TypeInfo const? const implicit;context:__context const) : string <function-_at_rtti_c__c_describe_CI1_ls_CH_ls_rtti_c__c_TypeInfo_gr__gr_?_C_c>`
  *  :ref:`describe (lineinfo:rtti::LineInfo const implicit;fully:bool const;context:__context const) : string <function-_at_rtti_c__c_describe_CIH_ls_rtti_c__c_LineInfo_gr__Cb_C_c>`
  *  :ref:`get_mangled_name (type:rtti::TypeInfo const? const implicit;context:__context const) : string <function-_at_rtti_c__c_get_mangled_name_CI1_ls_CH_ls_rtti_c__c_TypeInfo_gr__gr_?_C_c>`

.. _function-_at_rtti_c__c_sprint_data_CI?_CI1_ls_CH_ls_rtti_c__c_TypeInfo_gr__gr_?_Ct_C_c:

.. das:function:: sprint_data(data: void? const implicit; type: rtti::TypeInfo const? const implicit; flags: bitfield const)

sprint_data returns string

+--------+--------------------------------------------------------------------+
+argument+argument type                                                       +
+========+====================================================================+
+data    +void? const implicit                                                +
+--------+--------------------------------------------------------------------+
+type    + :ref:`rtti::TypeInfo <handle-rtti-TypeInfo>`  const? const implicit+
+--------+--------------------------------------------------------------------+
+flags   +bitfield<> const                                                    +
+--------+--------------------------------------------------------------------+


|function-rtti-sprint_data|

.. _function-_at_rtti_c__c_sprint_data_Cf4_CI1_ls_CH_ls_rtti_c__c_TypeInfo_gr__gr_?_Ct_C_c:

.. das:function:: sprint_data(data: float4 const; type: rtti::TypeInfo const? const implicit; flags: bitfield const)

sprint_data returns string

+--------+--------------------------------------------------------------------+
+argument+argument type                                                       +
+========+====================================================================+
+data    +float4 const                                                        +
+--------+--------------------------------------------------------------------+
+type    + :ref:`rtti::TypeInfo <handle-rtti-TypeInfo>`  const? const implicit+
+--------+--------------------------------------------------------------------+
+flags   +bitfield<> const                                                    +
+--------+--------------------------------------------------------------------+


|function-rtti-sprint_data|

.. _function-_at_rtti_c__c_describe_CI1_ls_CH_ls_rtti_c__c_TypeInfo_gr__gr_?_C_c:

.. das:function:: describe(type: rtti::TypeInfo const? const implicit)

describe returns string

+--------+--------------------------------------------------------------------+
+argument+argument type                                                       +
+========+====================================================================+
+type    + :ref:`rtti::TypeInfo <handle-rtti-TypeInfo>`  const? const implicit+
+--------+--------------------------------------------------------------------+


|function-rtti-describe|

.. _function-_at_rtti_c__c_describe_CIH_ls_rtti_c__c_LineInfo_gr__Cb_C_c:

.. das:function:: describe(lineinfo: LineInfo const implicit; fully: bool const)

describe returns string

+--------+-------------------------------------------------------------+
+argument+argument type                                                +
+========+=============================================================+
+lineinfo+ :ref:`rtti::LineInfo <handle-rtti-LineInfo>`  const implicit+
+--------+-------------------------------------------------------------+
+fully   +bool const                                                   +
+--------+-------------------------------------------------------------+


|function-rtti-describe|

.. _function-_at_rtti_c__c_get_mangled_name_CI1_ls_CH_ls_rtti_c__c_TypeInfo_gr__gr_?_C_c:

.. das:function:: get_mangled_name(type: rtti::TypeInfo const? const implicit)

get_mangled_name returns string

+--------+--------------------------------------------------------------------+
+argument+argument type                                                       +
+========+====================================================================+
+type    + :ref:`rtti::TypeInfo <handle-rtti-TypeInfo>`  const? const implicit+
+--------+--------------------------------------------------------------------+


|function-rtti-get_mangled_name|

++++++++++++++++++++++++++++++
Function and mangled name hash
++++++++++++++++++++++++++++++

  *  :ref:`get_function_by_mangled_name_hash (src:uint64 const;context:__context const) : function\<\> <function-_at_rtti_c__c_get_function_by_mangled_name_hash_Cu64_C_c>`
  *  :ref:`get_function_by_mangled_name_hash (src:uint64 const;context:rtti::Context implicit) : function\<\> <function-_at_rtti_c__c_get_function_by_mangled_name_hash_Cu64_IH_ls_rtti_c__c_Context_gr_>`
  *  :ref:`get_function_mangled_name_hash (src:function\<\> const;context:__context const) : uint64 <function-_at_rtti_c__c_get_function_mangled_name_hash_C_at__at__C_c>`
  *  :ref:`get_function_address (MNH:uint64 const;at:rtti::Context implicit) : uint64 <function-_at_rtti_c__c_get_function_address_Cu64_IH_ls_rtti_c__c_Context_gr_>`

.. _function-_at_rtti_c__c_get_function_by_mangled_name_hash_Cu64_C_c:

.. das:function:: get_function_by_mangled_name_hash(src: uint64 const)

get_function_by_mangled_name_hash returns function<>

+--------+-------------+
+argument+argument type+
+========+=============+
+src     +uint64 const +
+--------+-------------+


|function-rtti-get_function_by_mangled_name_hash|

.. _function-_at_rtti_c__c_get_function_by_mangled_name_hash_Cu64_IH_ls_rtti_c__c_Context_gr_:

.. das:function:: get_function_by_mangled_name_hash(src: uint64 const; context: Context implicit)

get_function_by_mangled_name_hash returns function<>

+--------+-----------------------------------------------------+
+argument+argument type                                        +
+========+=====================================================+
+src     +uint64 const                                         +
+--------+-----------------------------------------------------+
+context + :ref:`rtti::Context <handle-rtti-Context>`  implicit+
+--------+-----------------------------------------------------+


|function-rtti-get_function_by_mangled_name_hash|

.. _function-_at_rtti_c__c_get_function_mangled_name_hash_C_at__at__C_c:

.. das:function:: get_function_mangled_name_hash(src: function<> const)

get_function_mangled_name_hash returns uint64

+--------+----------------+
+argument+argument type   +
+========+================+
+src     +function<> const+
+--------+----------------+


|function-rtti-get_function_mangled_name_hash|

.. _function-_at_rtti_c__c_get_function_address_Cu64_IH_ls_rtti_c__c_Context_gr_:

.. das:function:: get_function_address(MNH: uint64 const; at: Context implicit)

get_function_address returns uint64

+--------+-----------------------------------------------------+
+argument+argument type                                        +
+========+=====================================================+
+MNH     +uint64 const                                         +
+--------+-----------------------------------------------------+
+at      + :ref:`rtti::Context <handle-rtti-Context>`  implicit+
+--------+-----------------------------------------------------+


|function-rtti-get_function_address|

+++++++++++++++++++++++++
Context and mutex locking
+++++++++++++++++++++++++

  *  :ref:`lock_this_context (block:block\<void\> const implicit;context:__context const;line:__lineInfo const) : void <function-_at_rtti_c__c_lock_this_context_CI1_ls_v_gr__builtin__C_c_C_l>`
  *  :ref:`lock_context (lock_context:rtti::Context implicit;block:block\<void\> const implicit;context:__context const;line:__lineInfo const) : void <function-_at_rtti_c__c_lock_context_IH_ls_rtti_c__c_Context_gr__CI1_ls_v_gr__builtin__C_c_C_l>`
  *  :ref:`lock_mutex (mutex:rtti::recursive_mutex implicit;block:block\<void\> const implicit;context:__context const;line:__lineInfo const) : void <function-_at_rtti_c__c_lock_mutex_IH_ls_rtti_c__c_recursive_mutex_gr__CI1_ls_v_gr__builtin__C_c_C_l>`

.. _function-_at_rtti_c__c_lock_this_context_CI1_ls_v_gr__builtin__C_c_C_l:

.. das:function:: lock_this_context(block: block<void> const implicit)

+--------+----------------------+
+argument+argument type         +
+========+======================+
+block   +block<> const implicit+
+--------+----------------------+


|function-rtti-lock_this_context|

.. _function-_at_rtti_c__c_lock_context_IH_ls_rtti_c__c_Context_gr__CI1_ls_v_gr__builtin__C_c_C_l:

.. das:function:: lock_context(lock_context: Context implicit; block: block<void> const implicit)

+------------+-----------------------------------------------------+
+argument    +argument type                                        +
+============+=====================================================+
+lock_context+ :ref:`rtti::Context <handle-rtti-Context>`  implicit+
+------------+-----------------------------------------------------+
+block       +block<> const implicit                               +
+------------+-----------------------------------------------------+


|function-rtti-lock_context|

.. _function-_at_rtti_c__c_lock_mutex_IH_ls_rtti_c__c_recursive_mutex_gr__CI1_ls_v_gr__builtin__C_c_C_l:

.. das:function:: lock_mutex(mutex: recursive_mutex implicit; block: block<void> const implicit)

+--------+---------------------------------------------------------------------+
+argument+argument type                                                        +
+========+=====================================================================+
+mutex   + :ref:`rtti::recursive_mutex <handle-rtti-recursive_mutex>`  implicit+
+--------+---------------------------------------------------------------------+
+block   +block<> const implicit                                               +
+--------+---------------------------------------------------------------------+


|function-rtti-lock_mutex|

+++++++++++++++++++
Runtime data access
+++++++++++++++++++

  *  :ref:`get_table_key_index (table:void? const implicit;key:any;baseType:rtti::Type const;valueTypeSize:int const;context:__context const;at:__lineInfo const) : int <function-_at_rtti_c__c_get_table_key_index_CI?_*_CE_ls_rtti_c__c_Type_gr__Ci_C_c_C_l>`

.. _function-_at_rtti_c__c_get_table_key_index_CI?_*_CE_ls_rtti_c__c_Type_gr__Ci_C_c_C_l:

.. das:function:: get_table_key_index(table: void? const implicit; key: any; baseType: Type const; valueTypeSize: int const)

get_table_key_index returns int

+-------------+------------------------------------------+
+argument     +argument type                             +
+=============+==========================================+
+table        +void? const implicit                      +
+-------------+------------------------------------------+
+key          +any                                       +
+-------------+------------------------------------------+
+baseType     + :ref:`rtti::Type <enum-rtti-Type>`  const+
+-------------+------------------------------------------+
+valueTypeSize+int const                                 +
+-------------+------------------------------------------+


|function-rtti-get_table_key_index|

+++++++++++++
Uncategorized
+++++++++++++

.. _function-_at_rtti_c__c_with_program_serialized_CI0_ls_1_ls_H_ls_rtti_c__c_Program_gr__gr_?W_gr_1_ls_v_gr__builtin__CI1_ls_H_ls_rtti_c__c_Program_gr__gr_?W_C_c_C_l:

.. das:function:: with_program_serialized(program: block<(var arg0:smart_ptr<rtti::Program>):void> const implicit; block: smart_ptr<rtti::Program> const implicit)

+--------+------------------------------------------------------------------------------------+
+argument+argument type                                                                       +
+========+====================================================================================+
+program +block<(smart_ptr< :ref:`rtti::Program <handle-rtti-Program>` >):void> const implicit+
+--------+------------------------------------------------------------------------------------+
+block   +smart_ptr< :ref:`rtti::Program <handle-rtti-Program>` > const implicit              +
+--------+------------------------------------------------------------------------------------+


|function-rtti-with_program_serialized|

.. _function-_at_rtti_c__c_get_tuple_field_offset_CI1_ls_H_ls_rtti_c__c_TypeInfo_gr__gr_?_Ci:

.. das:function:: get_tuple_field_offset(type: rtti::TypeInfo? const implicit; index: int const)

get_tuple_field_offset returns int

+--------+--------------------------------------------------------------+
+argument+argument type                                                 +
+========+==============================================================+
+type    + :ref:`rtti::TypeInfo <handle-rtti-TypeInfo>` ? const implicit+
+--------+--------------------------------------------------------------+
+index   +int const                                                     +
+--------+--------------------------------------------------------------+


|function-rtti-get_tuple_field_offset|

.. _function-_at_rtti_c__c_get_variant_field_offset_CI1_ls_H_ls_rtti_c__c_TypeInfo_gr__gr_?_Ci:

.. das:function:: get_variant_field_offset(type: rtti::TypeInfo? const implicit; index: int const)

get_variant_field_offset returns int

+--------+--------------------------------------------------------------+
+argument+argument type                                                 +
+========+==============================================================+
+type    + :ref:`rtti::TypeInfo <handle-rtti-TypeInfo>` ? const implicit+
+--------+--------------------------------------------------------------+
+index   +int const                                                     +
+--------+--------------------------------------------------------------+


|function-rtti-get_variant_field_offset|

.. _function-_at_rtti_c__c_each_I_eq_H_ls_rtti_c__c_FuncInfo_gr__C_c:

.. das:function:: each(info: FuncInfo implicit ==const)

each returns iterator< :ref:`rtti::VarInfo <handle-rtti-VarInfo>` >

+--------+--------------------------------------------------------+
+argument+argument type                                           +
+========+========================================================+
+info    + :ref:`rtti::FuncInfo <handle-rtti-FuncInfo>`  implicit!+
+--------+--------------------------------------------------------+


|function-rtti-each|

.. _function-_at_rtti_c__c_each_CI_eq_H_ls_rtti_c__c_FuncInfo_gr__C_c:

.. das:function:: each(info: FuncInfo const implicit ==const)

each returns iterator< :ref:`rtti::VarInfo <handle-rtti-VarInfo>`  const>

+--------+--------------------------------------------------------------+
+argument+argument type                                                 +
+========+==============================================================+
+info    + :ref:`rtti::FuncInfo <handle-rtti-FuncInfo>`  const implicit!+
+--------+--------------------------------------------------------------+


|function-rtti-each|

.. _function-_at_rtti_c__c_each_I_eq_H_ls_rtti_c__c_StructInfo_gr__C_c:

.. das:function:: each(info: StructInfo implicit ==const)

each returns iterator< :ref:`rtti::VarInfo <handle-rtti-VarInfo>` >

+--------+------------------------------------------------------------+
+argument+argument type                                               +
+========+============================================================+
+info    + :ref:`rtti::StructInfo <handle-rtti-StructInfo>`  implicit!+
+--------+------------------------------------------------------------+


|function-rtti-each|

.. _function-_at_rtti_c__c_each_CI_eq_H_ls_rtti_c__c_StructInfo_gr__C_c:

.. das:function:: each(info: StructInfo const implicit ==const)

each returns iterator< :ref:`rtti::VarInfo <handle-rtti-VarInfo>`  const>

+--------+------------------------------------------------------------------+
+argument+argument type                                                     +
+========+==================================================================+
+info    + :ref:`rtti::StructInfo <handle-rtti-StructInfo>`  const implicit!+
+--------+------------------------------------------------------------------+


|function-rtti-each|

.. _function-_at_rtti_c__c_each_I_eq_H_ls_rtti_c__c_EnumInfo_gr__C_c:

.. das:function:: each(info: EnumInfo implicit ==const)

each returns iterator< :ref:`rtti::EnumValueInfo <handle-rtti-EnumValueInfo>` >

+--------+--------------------------------------------------------+
+argument+argument type                                           +
+========+========================================================+
+info    + :ref:`rtti::EnumInfo <handle-rtti-EnumInfo>`  implicit!+
+--------+--------------------------------------------------------+


|function-rtti-each|

.. _function-_at_rtti_c__c_each_CI_eq_H_ls_rtti_c__c_EnumInfo_gr__C_c:

.. das:function:: each(info: EnumInfo const implicit ==const)

each returns iterator< :ref:`rtti::EnumValueInfo <handle-rtti-EnumValueInfo>`  const>

+--------+--------------------------------------------------------------+
+argument+argument type                                                 +
+========+==============================================================+
+info    + :ref:`rtti::EnumInfo <handle-rtti-EnumInfo>`  const implicit!+
+--------+--------------------------------------------------------------+


|function-rtti-each|


