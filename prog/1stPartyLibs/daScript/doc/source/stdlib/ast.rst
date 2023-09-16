
.. _stdlib_ast:

========================
AST manipulation library
========================

.. include:: detail/ast.rst

The AST module implements compilation time reflection for the daScript syntax tree.

All functions and symbols are in "ast" module, use require to get access to it. ::

    require ast


++++++++++++
Type aliases
++++++++++++

.. _alias-TypeDeclFlags:

.. das:attribute:: TypeDeclFlags is a bitfield

+---------------+---+-----+
+field          +bit+value+
+===============+===+=====+
+ref            +0  +1    +
+---------------+---+-----+
+constant       +1  +2    +
+---------------+---+-----+
+temporary      +2  +4    +
+---------------+---+-----+
+_implicit      +3  +8    +
+---------------+---+-----+
+removeRef      +4  +16   +
+---------------+---+-----+
+removeConstant +5  +32   +
+---------------+---+-----+
+removeDim      +6  +64   +
+---------------+---+-----+
+removeTemporary+7  +128  +
+---------------+---+-----+
+explicitConst  +8  +256  +
+---------------+---+-----+
+aotAlias       +9  +512  +
+---------------+---+-----+
+smartPtr       +10 +1024 +
+---------------+---+-----+
+smartPtrNative +11 +2048 +
+---------------+---+-----+
+isExplicit     +12 +4096 +
+---------------+---+-----+
+isNativeDim    +13 +8192 +
+---------------+---+-----+
+isTag          +14 +16384+
+---------------+---+-----+
+explicitRef    +15 +32768+
+---------------+---+-----+


|typedef-ast-TypeDeclFlags|

.. _alias-FieldDeclarationFlags:

.. das:attribute:: FieldDeclarationFlags is a bitfield

+----------------+---+-----+
+field           +bit+value+
+================+===+=====+
+moveSemantics   +0  +1    +
+----------------+---+-----+
+parentType      +1  +2    +
+----------------+---+-----+
+capturedConstant+2  +4    +
+----------------+---+-----+
+generated       +3  +8    +
+----------------+---+-----+
+capturedRef     +4  +16   +
+----------------+---+-----+
+doNotDelete     +5  +32   +
+----------------+---+-----+
+privateField    +6  +64   +
+----------------+---+-----+
+_sealed         +7  +128  +
+----------------+---+-----+
+implemented     +8  +256  +
+----------------+---+-----+


|typedef-ast-FieldDeclarationFlags|

.. _alias-StructureFlags:

.. das:attribute:: StructureFlags is a bitfield

+----------------+---+-----+
+field           +bit+value+
+================+===+=====+
+isClass         +0  +1    +
+----------------+---+-----+
+genCtor         +1  +2    +
+----------------+---+-----+
+cppLayout       +2  +4    +
+----------------+---+-----+
+cppLayoutNotPod +3  +8    +
+----------------+---+-----+
+generated       +4  +16   +
+----------------+---+-----+
+persistent      +5  +32   +
+----------------+---+-----+
+isLambda        +6  +64   +
+----------------+---+-----+
+privateStructure+7  +128  +
+----------------+---+-----+
+macroInterface  +8  +256  +
+----------------+---+-----+
+_sealed         +9  +512  +
+----------------+---+-----+
+skipLockCheck   +10 +1024 +
+----------------+---+-----+
+circular        +11 +2048 +
+----------------+---+-----+
+_generator      +12 +4096 +
+----------------+---+-----+


|typedef-ast-StructureFlags|

.. _alias-ExprGenFlags:

.. das:attribute:: ExprGenFlags is a bitfield

+---------------+---+-----+
+field          +bit+value+
+===============+===+=====+
+alwaysSafe     +0  +1    +
+---------------+---+-----+
+generated      +1  +2    +
+---------------+---+-----+
+userSaidItsSafe+2  +4    +
+---------------+---+-----+


|typedef-ast-ExprGenFlags|

.. _alias-ExprLetFlags:

.. das:attribute:: ExprLetFlags is a bitfield

+-----------+---+-----+
+field      +bit+value+
+===========+===+=====+
+inScope    +0  +1    +
+-----------+---+-----+
+hasEarlyOut+1  +2    +
+-----------+---+-----+


|typedef-ast-ExprLetFlags|

.. _alias-ExprFlags:

.. das:attribute:: ExprFlags is a bitfield

+-------------------+---+-----+
+field              +bit+value+
+===================+===+=====+
+constexpression    +0  +1    +
+-------------------+---+-----+
+noSideEffects      +1  +2    +
+-------------------+---+-----+
+noNativeSideEffects+2  +4    +
+-------------------+---+-----+
+isForLoopSource    +3  +8    +
+-------------------+---+-----+
+isCallArgument     +4  +16   +
+-------------------+---+-----+


|typedef-ast-ExprFlags|

.. _alias-ExprPrintFlags:

.. das:attribute:: ExprPrintFlags is a bitfield

+-----------+---+-----+
+field      +bit+value+
+===========+===+=====+
+topLevel   +0  +1    +
+-----------+---+-----+
+argLevel   +1  +2    +
+-----------+---+-----+
+bottomLevel+2  +4    +
+-----------+---+-----+


|typedef-ast-ExprPrintFlags|

.. _alias-FunctionFlags:

.. das:attribute:: FunctionFlags is a bitfield

+---------------------+---+-----------+
+field                +bit+value      +
+=====================+===+===========+
+builtIn              +0  +1          +
+---------------------+---+-----------+
+policyBased          +1  +2          +
+---------------------+---+-----------+
+callBased            +2  +4          +
+---------------------+---+-----------+
+interopFn            +3  +8          +
+---------------------+---+-----------+
+hasReturn            +4  +16         +
+---------------------+---+-----------+
+copyOnReturn         +5  +32         +
+---------------------+---+-----------+
+moveOnReturn         +6  +64         +
+---------------------+---+-----------+
+exports              +7  +128        +
+---------------------+---+-----------+
+init                 +8  +256        +
+---------------------+---+-----------+
+addr                 +9  +512        +
+---------------------+---+-----------+
+used                 +10 +1024       +
+---------------------+---+-----------+
+fastCall             +11 +2048       +
+---------------------+---+-----------+
+knownSideEffects     +12 +4096       +
+---------------------+---+-----------+
+hasToRunAtCompileTime+13 +8192       +
+---------------------+---+-----------+
+unsafeOperation      +14 +16384      +
+---------------------+---+-----------+
+unsafeDeref          +15 +32768      +
+---------------------+---+-----------+
+hasMakeBlock         +16 +65536      +
+---------------------+---+-----------+
+aotNeedPrologue      +17 +131072     +
+---------------------+---+-----------+
+noAot                +18 +262144     +
+---------------------+---+-----------+
+aotHybrid            +19 +524288     +
+---------------------+---+-----------+
+aotTemplate          +20 +1048576    +
+---------------------+---+-----------+
+generated            +21 +2097152    +
+---------------------+---+-----------+
+privateFunction      +22 +4194304    +
+---------------------+---+-----------+
+_generator           +23 +8388608    +
+---------------------+---+-----------+
+_lambda              +24 +16777216   +
+---------------------+---+-----------+
+firstArgReturnType   +25 +33554432   +
+---------------------+---+-----------+
+noPointerCast        +26 +67108864   +
+---------------------+---+-----------+
+isClassMethod        +27 +134217728  +
+---------------------+---+-----------+
+isTypeConstructor    +28 +268435456  +
+---------------------+---+-----------+
+shutdown             +29 +536870912  +
+---------------------+---+-----------+
+anyTemplate          +30 +1073741824 +
+---------------------+---+-----------+
+macroInit            +31 +-2147483648+
+---------------------+---+-----------+


|typedef-ast-FunctionFlags|

.. _alias-MoreFunctionFlags:

.. das:attribute:: MoreFunctionFlags is a bitfield

+--------------------------+---+-----+
+field                     +bit+value+
+==========================+===+=====+
+macroFunction             +0  +1    +
+--------------------------+---+-----+
+needStringCast            +1  +2    +
+--------------------------+---+-----+
+aotHashDeppendsOnArguments+2  +4    +
+--------------------------+---+-----+
+lateInit                  +3  +8    +
+--------------------------+---+-----+
+requestJit                +4  +16   +
+--------------------------+---+-----+
+unsafeOutsideOfFor        +5  +32   +
+--------------------------+---+-----+
+skipLockCheck             +6  +64   +
+--------------------------+---+-----+
+safeImplicit              +7  +128  +
+--------------------------+---+-----+
+deprecated                +8  +256  +
+--------------------------+---+-----+
+aliasCMRES                +9  +512  +
+--------------------------+---+-----+
+neverAliasCMRES           +10 +1024 +
+--------------------------+---+-----+
+addressTaken              +11 +2048 +
+--------------------------+---+-----+
+propertyFunction          +12 +4096 +
+--------------------------+---+-----+
+pinvoke                   +13 +8192 +
+--------------------------+---+-----+
+jitOnly                   +14 +16384+
+--------------------------+---+-----+


|typedef-ast-MoreFunctionFlags|

.. _alias-FunctionSideEffectFlags:

.. das:attribute:: FunctionSideEffectFlags is a bitfield

+--------------+---+-----+
+field         +bit+value+
+==============+===+=====+
+_unsafe       +0  +1    +
+--------------+---+-----+
+userScenario  +1  +2    +
+--------------+---+-----+
+modifyExternal+2  +4    +
+--------------+---+-----+
+modifyArgument+3  +8    +
+--------------+---+-----+
+accessGlobal  +4  +16   +
+--------------+---+-----+
+invoke        +5  +32   +
+--------------+---+-----+


|typedef-ast-FunctionSideEffectFlags|

.. _alias-VariableFlags:

.. das:attribute:: VariableFlags is a bitfield

+----------------+---+-----+
+field           +bit+value+
+================+===+=====+
+init_via_move   +0  +1    +
+----------------+---+-----+
+init_via_clone  +1  +2    +
+----------------+---+-----+
+used            +2  +4    +
+----------------+---+-----+
+aliasCMRES      +3  +8    +
+----------------+---+-----+
+marked_used     +4  +16   +
+----------------+---+-----+
+global_shared   +5  +32   +
+----------------+---+-----+
+do_not_delete   +6  +64   +
+----------------+---+-----+
+generated       +7  +128  +
+----------------+---+-----+
+capture_as_ref  +8  +256  +
+----------------+---+-----+
+can_shadow      +9  +512  +
+----------------+---+-----+
+private_variable+10 +1024 +
+----------------+---+-----+
+tag             +11 +2048 +
+----------------+---+-----+
+global          +12 +4096 +
+----------------+---+-----+
+inScope         +13 +8192 +
+----------------+---+-----+
+no_capture      +14 +16384+
+----------------+---+-----+
+early_out       +15 +32768+
+----------------+---+-----+
+used_in_finally +16 +65536+
+----------------+---+-----+


|typedef-ast-VariableFlags|

.. _alias-VariableAccessFlags:

.. das:attribute:: VariableAccessFlags is a bitfield

+-------------+---+-----+
+field        +bit+value+
+=============+===+=====+
+access_extern+0  +1    +
+-------------+---+-----+
+access_get   +1  +2    +
+-------------+---+-----+
+access_ref   +2  +4    +
+-------------+---+-----+
+access_init  +3  +8    +
+-------------+---+-----+
+access_pass  +4  +16   +
+-------------+---+-----+


|typedef-ast-VariableAccessFlags|

.. _alias-ExprBlockFlags:

.. das:attribute:: ExprBlockFlags is a bitfield

+--------------------------+---+-----+
+field                     +bit+value+
+==========================+===+=====+
+isClosure                 +0  +1    +
+--------------------------+---+-----+
+hasReturn                 +1  +2    +
+--------------------------+---+-----+
+copyOnReturn              +2  +4    +
+--------------------------+---+-----+
+moveOnReturn              +3  +8    +
+--------------------------+---+-----+
+inTheLoop                 +4  +16   +
+--------------------------+---+-----+
+finallyBeforeBody         +5  +32   +
+--------------------------+---+-----+
+finallyDisabled           +6  +64   +
+--------------------------+---+-----+
+aotSkipMakeBlock          +7  +128  +
+--------------------------+---+-----+
+aotDoNotSkipAnnotationData+8  +256  +
+--------------------------+---+-----+
+isCollapseable            +9  +512  +
+--------------------------+---+-----+
+needCollapse              +10 +1024 +
+--------------------------+---+-----+
+hasMakeBlock              +11 +2048 +
+--------------------------+---+-----+
+hasEarlyOut               +12 +4096 +
+--------------------------+---+-----+


|typedef-ast-ExprBlockFlags|

.. _alias-ExprAtFlags:

.. das:attribute:: ExprAtFlags is a bitfield

+------------+---+-----+
+field       +bit+value+
+============+===+=====+
+r2v         +0  +1    +
+------------+---+-----+
+r2cr        +1  +2    +
+------------+---+-----+
+write       +2  +4    +
+------------+---+-----+
+no_promotion+3  +8    +
+------------+---+-----+


|typedef-ast-ExprAtFlags|

.. _alias-ExprMakeLocalFlags:

.. das:attribute:: ExprMakeLocalFlags is a bitfield

+---------------+---+-----+
+field          +bit+value+
+===============+===+=====+
+useStackRef    +0  +1    +
+---------------+---+-----+
+useCMRES       +1  +2    +
+---------------+---+-----+
+doesNotNeedSp  +2  +4    +
+---------------+---+-----+
+doesNotNeedInit+3  +8    +
+---------------+---+-----+
+initAllFields  +4  +16   +
+---------------+---+-----+
+alwaysAlias    +5  +32   +
+---------------+---+-----+


|typedef-ast-ExprMakeLocalFlags|

.. _alias-ExprAscendFlags:

.. das:attribute:: ExprAscendFlags is a bitfield

+------------+---+-----+
+field       +bit+value+
+============+===+=====+
+useStackRef +0  +1    +
+------------+---+-----+
+needTypeInfo+1  +2    +
+------------+---+-----+
+isMakeLambda+2  +4    +
+------------+---+-----+


|typedef-ast-ExprAscendFlags|

.. _alias-ExprCastFlags:

.. das:attribute:: ExprCastFlags is a bitfield

+---------------+---+-----+
+field          +bit+value+
+===============+===+=====+
+upcastCast     +0  +1    +
+---------------+---+-----+
+reinterpretCast+1  +2    +
+---------------+---+-----+


|typedef-ast-ExprCastFlags|

.. _alias-ExprVarFlags:

.. das:attribute:: ExprVarFlags is a bitfield

+---------+---+-----+
+field    +bit+value+
+=========+===+=====+
+local    +0  +1    +
+---------+---+-----+
+argument +1  +2    +
+---------+---+-----+
+_block   +2  +4    +
+---------+---+-----+
+thisBlock+3  +8    +
+---------+---+-----+
+r2v      +4  +16   +
+---------+---+-----+
+r2cr     +5  +32   +
+---------+---+-----+
+write    +6  +64   +
+---------+---+-----+


|typedef-ast-ExprVarFlags|

.. _alias-ExprMakeStructFlags:

.. das:attribute:: ExprMakeStructFlags is a bitfield

+--------------+---+-----+
+field         +bit+value+
+==============+===+=====+
+useInitializer+0  +1    +
+--------------+---+-----+
+isNewHandle   +1  +2    +
+--------------+---+-----+


|typedef-ast-ExprMakeStructFlags|

.. _alias-MakeFieldDeclFlags:

.. das:attribute:: MakeFieldDeclFlags is a bitfield

+--------------+---+-----+
+field         +bit+value+
+==============+===+=====+
+moveSemantics +0  +1    +
+--------------+---+-----+
+cloneSemantics+1  +2    +
+--------------+---+-----+


|typedef-ast-MakeFieldDeclFlags|

.. _alias-ExprFieldDerefFlags:

.. das:attribute:: ExprFieldDerefFlags is a bitfield

+------------------+---+-----+
+field             +bit+value+
+==================+===+=====+
+unsafeDeref       +0  +1    +
+------------------+---+-----+
+ignoreCaptureConst+1  +2    +
+------------------+---+-----+


|typedef-ast-ExprFieldDerefFlags|

.. _alias-ExprFieldFieldFlags:

.. das:attribute:: ExprFieldFieldFlags is a bitfield

+------------+---+-----+
+field       +bit+value+
+============+===+=====+
+r2v         +0  +1    +
+------------+---+-----+
+r2cr        +1  +2    +
+------------+---+-----+
+write       +2  +4    +
+------------+---+-----+
+no_promotion+3  +8    +
+------------+---+-----+


|typedef-ast-ExprFieldFieldFlags|

.. _alias-ExprSwizzleFieldFlags:

.. das:attribute:: ExprSwizzleFieldFlags is a bitfield

+-----+---+-----+
+field+bit+value+
+=====+===+=====+
+r2v  +0  +1    +
+-----+---+-----+
+r2cr +1  +2    +
+-----+---+-----+
+write+2  +4    +
+-----+---+-----+


|typedef-ast-ExprSwizzleFieldFlags|

.. _alias-ExprYieldFlags:

.. das:attribute:: ExprYieldFlags is a bitfield

+-------------+---+-----+
+field        +bit+value+
+=============+===+=====+
+moveSemantics+0  +1    +
+-------------+---+-----+
+skipLockCheck+1  +2    +
+-------------+---+-----+


|typedef-ast-ExprYieldFlags|

.. _alias-ExprReturnFlags:

.. das:attribute:: ExprReturnFlags is a bitfield

+------------------+---+-----+
+field             +bit+value+
+==================+===+=====+
+moveSemantics     +0  +1    +
+------------------+---+-----+
+returnReference   +1  +2    +
+------------------+---+-----+
+returnInBlock     +2  +4    +
+------------------+---+-----+
+takeOverRightStack+3  +8    +
+------------------+---+-----+
+returnCallCMRES   +4  +16   +
+------------------+---+-----+
+returnCMRES       +5  +32   +
+------------------+---+-----+
+fromYield         +6  +64   +
+------------------+---+-----+
+fromComprehension +7  +128  +
+------------------+---+-----+
+skipLockCheck     +8  +256  +
+------------------+---+-----+


|typedef-ast-ExprReturnFlags|

.. _alias-ExprMakeBlockFlags:

.. das:attribute:: ExprMakeBlockFlags is a bitfield

+---------------+---+-----+
+field          +bit+value+
+===============+===+=====+
+isLambda       +0  +1    +
+---------------+---+-----+
+isLocalFunction+1  +2    +
+---------------+---+-----+


|typedef-ast-ExprMakeBlockFlags|

.. _alias-CopyFlags:

.. das:attribute:: CopyFlags is a bitfield

+------------------+---+-----+
+field             +bit+value+
+==================+===+=====+
+allowCopyTemp     +0  +1    +
+------------------+---+-----+
+takeOverRightStack+1  +2    +
+------------------+---+-----+


|typedef-ast-CopyFlags|

.. _alias-MoveFlags:

.. das:attribute:: MoveFlags is a bitfield

+------------------+---+-----+
+field             +bit+value+
+==================+===+=====+
+skipLockCheck     +0  +1    +
+------------------+---+-----+
+takeOverRightStack+1  +2    +
+------------------+---+-----+


|typedef-ast-MoveFlags|

.. _alias-IfFlags:

.. das:attribute:: IfFlags is a bitfield

+---------+---+-----+
+field    +bit+value+
+=========+===+=====+
+isStatic +0  +1    +
+---------+---+-----+
+doNotFold+1  +2    +
+---------+---+-----+


|typedef-ast-IfFlags|

.. _alias-ExpressionPtr:

.. das:attribute:: ExpressionPtr = smart_ptr<ast::Expression>

|typedef-ast-ExpressionPtr|

.. _alias-ProgramPtr:

.. das:attribute:: ProgramPtr = smart_ptr<rtti::Program>

|typedef-ast-ProgramPtr|

.. _alias-TypeDeclPtr:

.. das:attribute:: TypeDeclPtr = smart_ptr<ast::TypeDecl>

|typedef-ast-TypeDeclPtr|

.. _alias-VectorTypeDeclPtr:

.. das:attribute:: VectorTypeDeclPtr = dasvector`smart_ptr`TypeDecl

|typedef-ast-VectorTypeDeclPtr|

.. _alias-EnumerationPtr:

.. das:attribute:: EnumerationPtr = smart_ptr<ast::Enumeration>

|typedef-ast-EnumerationPtr|

.. _alias-StructurePtr:

.. das:attribute:: StructurePtr = smart_ptr<ast::Structure>

|typedef-ast-StructurePtr|

.. _alias-FunctionPtr:

.. das:attribute:: FunctionPtr = smart_ptr<ast::Function>

|typedef-ast-FunctionPtr|

.. _alias-VariablePtr:

.. das:attribute:: VariablePtr = smart_ptr<ast::Variable>

|typedef-ast-VariablePtr|

.. _alias-MakeFieldDeclPtr:

.. das:attribute:: MakeFieldDeclPtr = smart_ptr<ast::MakeFieldDecl>

|typedef-ast-MakeFieldDeclPtr|

.. _alias-FunctionAnnotationPtr:

.. das:attribute:: FunctionAnnotationPtr = smart_ptr<ast::FunctionAnnotation>

|typedef-ast-FunctionAnnotationPtr|

.. _alias-StructureAnnotationPtr:

.. das:attribute:: StructureAnnotationPtr = smart_ptr<ast::StructureAnnotation>

|typedef-ast-StructureAnnotationPtr|

.. _alias-EnumerationAnnotationPtr:

.. das:attribute:: EnumerationAnnotationPtr = smart_ptr<ast::EnumerationAnnotation>

|typedef-ast-EnumerationAnnotationPtr|

.. _alias-PassMacroPtr:

.. das:attribute:: PassMacroPtr = smart_ptr<ast::PassMacro>

|typedef-ast-PassMacroPtr|

.. _alias-VariantMacroPtr:

.. das:attribute:: VariantMacroPtr = smart_ptr<ast::VariantMacro>

|typedef-ast-VariantMacroPtr|

.. _alias-ReaderMacroPtr:

.. das:attribute:: ReaderMacroPtr = smart_ptr<ast::ReaderMacro>

|typedef-ast-ReaderMacroPtr|

.. _alias-CommentReaderPtr:

.. das:attribute:: CommentReaderPtr = smart_ptr<ast::CommentReader>

|typedef-ast-CommentReaderPtr|

.. _alias-CallMacroPtr:

.. das:attribute:: CallMacroPtr = smart_ptr<ast::CallMacro>

|typedef-ast-CallMacroPtr|

.. _alias-TypeInfoMacroPtr:

.. das:attribute:: TypeInfoMacroPtr = smart_ptr<ast::TypeInfoMacro>

|typedef-ast-TypeInfoMacroPtr|

.. _alias-ForLoopMacroPtr:

.. das:attribute:: ForLoopMacroPtr = smart_ptr<ast::ForLoopMacro>

|typedef-ast-ForLoopMacroPtr|

.. _alias-CaptureMacroPtr:

.. das:attribute:: CaptureMacroPtr = smart_ptr<ast::CaptureMacro>

|typedef-ast-CaptureMacroPtr|

.. _alias-SimulateMacroPtr:

.. das:attribute:: SimulateMacroPtr = smart_ptr<ast::SimulateMacro>

|typedef-ast-SimulateMacroPtr|

++++++++++++
Enumerations
++++++++++++

.. _enum-ast-SideEffects:

.. das:attribute:: SideEffects

+-------------------------+--+
+none                     +0 +
+-------------------------+--+
+unsafe                   +1 +
+-------------------------+--+
+userScenario             +2 +
+-------------------------+--+
+modifyExternal           +4 +
+-------------------------+--+
+accessExternal           +4 +
+-------------------------+--+
+modifyArgument           +8 +
+-------------------------+--+
+modifyArgumentAndExternal+12+
+-------------------------+--+
+worstDefault             +12+
+-------------------------+--+
+accessGlobal             +16+
+-------------------------+--+
+invoke                   +32+
+-------------------------+--+
+inferredSideEffects      +56+
+-------------------------+--+


|enumeration-ast-SideEffects|

.. _enum-ast-CaptureMode:

.. das:attribute:: CaptureMode

+--------------------+-+
+capture_any         +0+
+--------------------+-+
+capture_by_copy     +1+
+--------------------+-+
+capture_by_reference+2+
+--------------------+-+
+capture_by_clone    +3+
+--------------------+-+
+capture_by_move     +4+
+--------------------+-+


|enumeration-ast-CaptureMode|

++++++++++++++++++
Handled structures
++++++++++++++++++

.. _handle-ast-ModuleLibrary:

.. das:attribute:: ModuleLibrary

|structure_annotation-ast-ModuleLibrary|

.. _handle-ast-Expression:

.. das:attribute:: Expression

Expression fields are

+----------+-------------------------------------------------------+
+at        + :ref:`rtti::LineInfo <handle-rtti-LineInfo>`          +
+----------+-------------------------------------------------------+
+_type     +smart_ptr< :ref:`ast::TypeDecl <handle-ast-TypeDecl>` >+
+----------+-------------------------------------------------------+
+genFlags  + :ref:`ExprGenFlags <alias-ExprGenFlags>`              +
+----------+-------------------------------------------------------+
+flags     + :ref:`ExprFlags <alias-ExprFlags>`                    +
+----------+-------------------------------------------------------+
+__rtti    +string const                                           +
+----------+-------------------------------------------------------+
+printFlags+ :ref:`ExprPrintFlags <alias-ExprPrintFlags>`          +
+----------+-------------------------------------------------------+


|structure_annotation-ast-Expression|

.. _handle-ast-TypeDecl:

.. das:attribute:: TypeDecl

TypeDecl fields are

+----------+-----------------------------------------------------------+
+enumType  + :ref:`ast::Enumeration <handle-ast-Enumeration>` ?        +
+----------+-----------------------------------------------------------+
+firstType +smart_ptr< :ref:`ast::TypeDecl <handle-ast-TypeDecl>` >    +
+----------+-----------------------------------------------------------+
+at        + :ref:`rtti::LineInfo <handle-rtti-LineInfo>`              +
+----------+-----------------------------------------------------------+
+annotation+ :ref:`rtti::TypeAnnotation <handle-rtti-TypeAnnotation>` ?+
+----------+-----------------------------------------------------------+
+dimExpr   +vector<smart_ptr<Expression>>                              +
+----------+-----------------------------------------------------------+
+secondType+smart_ptr< :ref:`ast::TypeDecl <handle-ast-TypeDecl>` >    +
+----------+-----------------------------------------------------------+
+argNames  +vector<das_string>                                         +
+----------+-----------------------------------------------------------+
+flags     + :ref:`TypeDeclFlags <alias-TypeDeclFlags>`                +
+----------+-----------------------------------------------------------+
+dim       +vector<int>                                                +
+----------+-----------------------------------------------------------+
+structType+ :ref:`ast::Structure <handle-ast-Structure>` ?            +
+----------+-----------------------------------------------------------+
+argTypes  +vector<smart_ptr<TypeDecl>>                                +
+----------+-----------------------------------------------------------+
+alias     + :ref:`builtin::das_string <handle-builtin-das_string>`    +
+----------+-----------------------------------------------------------+
+baseType  + :ref:`rtti::Type <enum-rtti-Type>`                        +
+----------+-----------------------------------------------------------+
+_module   + :ref:`rtti::Module <handle-rtti-Module>` ?                +
+----------+-----------------------------------------------------------+


TypeDecl property operators are

+-------------------------+------------------------------------+
+canAot                   +bool                                +
+-------------------------+------------------------------------+
+isExprType               +bool                                +
+-------------------------+------------------------------------+
+isSimpleType             +bool                                +
+-------------------------+------------------------------------+
+isArray                  +bool                                +
+-------------------------+------------------------------------+
+isGoodIteratorType       +bool                                +
+-------------------------+------------------------------------+
+isGoodArrayType          +bool                                +
+-------------------------+------------------------------------+
+isGoodTableType          +bool                                +
+-------------------------+------------------------------------+
+isGoodBlockType          +bool                                +
+-------------------------+------------------------------------+
+isGoodFunctionType       +bool                                +
+-------------------------+------------------------------------+
+isGoodLambdaType         +bool                                +
+-------------------------+------------------------------------+
+isGoodTupleType          +bool                                +
+-------------------------+------------------------------------+
+isGoodVariantType        +bool                                +
+-------------------------+------------------------------------+
+isVoid                   +bool                                +
+-------------------------+------------------------------------+
+isRef                    +bool                                +
+-------------------------+------------------------------------+
+isRefType                +bool                                +
+-------------------------+------------------------------------+
+canWrite                 +bool                                +
+-------------------------+------------------------------------+
+isAotAlias               +bool                                +
+-------------------------+------------------------------------+
+isShareable              +bool                                +
+-------------------------+------------------------------------+
+isIndex                  +bool                                +
+-------------------------+------------------------------------+
+isBool                   +bool                                +
+-------------------------+------------------------------------+
+isInteger                +bool                                +
+-------------------------+------------------------------------+
+isSignedInteger          +bool                                +
+-------------------------+------------------------------------+
+isUnsignedInteger        +bool                                +
+-------------------------+------------------------------------+
+isSignedIntegerOrIntVec  +bool                                +
+-------------------------+------------------------------------+
+isUnsignedIntegerOrIntVec+bool                                +
+-------------------------+------------------------------------+
+isFloatOrDouble          +bool                                +
+-------------------------+------------------------------------+
+isNumeric                +bool                                +
+-------------------------+------------------------------------+
+isNumericComparable      +bool                                +
+-------------------------+------------------------------------+
+isPointer                +bool                                +
+-------------------------+------------------------------------+
+isVoidPointer            +bool                                +
+-------------------------+------------------------------------+
+isIterator               +bool                                +
+-------------------------+------------------------------------+
+isEnum                   +bool                                +
+-------------------------+------------------------------------+
+isEnumT                  +bool                                +
+-------------------------+------------------------------------+
+isHandle                 +bool                                +
+-------------------------+------------------------------------+
+isStructure              +bool                                +
+-------------------------+------------------------------------+
+isClass                  +bool                                +
+-------------------------+------------------------------------+
+isFunction               +bool                                +
+-------------------------+------------------------------------+
+isTuple                  +bool                                +
+-------------------------+------------------------------------+
+isVariant                +bool                                +
+-------------------------+------------------------------------+
+sizeOf                   +int                                 +
+-------------------------+------------------------------------+
+countOf                  +int                                 +
+-------------------------+------------------------------------+
+alignOf                  +int                                 +
+-------------------------+------------------------------------+
+baseSizeOf               +int                                 +
+-------------------------+------------------------------------+
+stride                   +int                                 +
+-------------------------+------------------------------------+
+tupleSize                +int                                 +
+-------------------------+------------------------------------+
+tupleAlign               +int                                 +
+-------------------------+------------------------------------+
+variantSize              +int                                 +
+-------------------------+------------------------------------+
+variantAlign             +int                                 +
+-------------------------+------------------------------------+
+canCopy                  +bool                                +
+-------------------------+------------------------------------+
+canMove                  +bool                                +
+-------------------------+------------------------------------+
+canClone                 +bool                                +
+-------------------------+------------------------------------+
+canNew                   +bool                                +
+-------------------------+------------------------------------+
+canDeletePtr             +bool                                +
+-------------------------+------------------------------------+
+canDelete                +bool                                +
+-------------------------+------------------------------------+
+needDelete               +bool                                +
+-------------------------+------------------------------------+
+isPod                    +bool                                +
+-------------------------+------------------------------------+
+isRawPod                 +bool                                +
+-------------------------+------------------------------------+
+isNoHeapType             +bool                                +
+-------------------------+------------------------------------+
+isWorkhorseType          +bool                                +
+-------------------------+------------------------------------+
+isPolicyType             +bool                                +
+-------------------------+------------------------------------+
+isVecPolicyType          +bool                                +
+-------------------------+------------------------------------+
+isReturnType             +bool                                +
+-------------------------+------------------------------------+
+isCtorType               +bool                                +
+-------------------------+------------------------------------+
+isRange                  +bool                                +
+-------------------------+------------------------------------+
+isString                 +bool                                +
+-------------------------+------------------------------------+
+isConst                  +bool                                +
+-------------------------+------------------------------------+
+isFoldable               +bool                                +
+-------------------------+------------------------------------+
+isAlias                  +bool                                +
+-------------------------+------------------------------------+
+isAutoArrayResolved      +bool                                +
+-------------------------+------------------------------------+
+isAuto                   +bool                                +
+-------------------------+------------------------------------+
+isAutoOrAlias            +bool                                +
+-------------------------+------------------------------------+
+isVectorType             +bool                                +
+-------------------------+------------------------------------+
+isBitfield               +bool                                +
+-------------------------+------------------------------------+
+isLocal                  +bool                                +
+-------------------------+------------------------------------+
+hasClasses               +bool                                +
+-------------------------+------------------------------------+
+hasNonTrivialCtor        +bool                                +
+-------------------------+------------------------------------+
+hasNonTrivialDtor        +bool                                +
+-------------------------+------------------------------------+
+hasNonTrivialCopy        +bool                                +
+-------------------------+------------------------------------+
+canBePlacedInContainer   +bool                                +
+-------------------------+------------------------------------+
+vectorBaseType           + :ref:`rtti::Type <enum-rtti-Type>` +
+-------------------------+------------------------------------+
+vectorDim                +int                                 +
+-------------------------+------------------------------------+
+canInitWithZero          +bool                                +
+-------------------------+------------------------------------+
+rangeBaseType            + :ref:`rtti::Type <enum-rtti-Type>` +
+-------------------------+------------------------------------+


|structure_annotation-ast-TypeDecl|

.. _handle-ast-Structure:

.. das:attribute:: Structure

Structure fields are

+-----------+----------------------------------------------------------+
+at         + :ref:`rtti::LineInfo <handle-rtti-LineInfo>`             +
+-----------+----------------------------------------------------------+
+annotations+ :ref:`rtti::AnnotationList <handle-rtti-AnnotationList>` +
+-----------+----------------------------------------------------------+
+flags      + :ref:`StructureFlags <alias-StructureFlags>`             +
+-----------+----------------------------------------------------------+
+name       + :ref:`builtin::das_string <handle-builtin-das_string>`   +
+-----------+----------------------------------------------------------+
+parent     + :ref:`ast::Structure <handle-ast-Structure>` ?           +
+-----------+----------------------------------------------------------+
+fields     +vector<FieldDeclaration>                                  +
+-----------+----------------------------------------------------------+
+_module    + :ref:`rtti::Module <handle-rtti-Module>` ?               +
+-----------+----------------------------------------------------------+


|structure_annotation-ast-Structure|

.. _handle-ast-FieldDeclaration:

.. das:attribute:: FieldDeclaration

FieldDeclaration fields are

+----------+--------------------------------------------------------------------------+
+at        + :ref:`rtti::LineInfo <handle-rtti-LineInfo>`                             +
+----------+--------------------------------------------------------------------------+
+annotation+ :ref:`rtti::AnnotationArgumentList <handle-rtti-AnnotationArgumentList>` +
+----------+--------------------------------------------------------------------------+
+_type     +smart_ptr< :ref:`ast::TypeDecl <handle-ast-TypeDecl>` >                   +
+----------+--------------------------------------------------------------------------+
+flags     + :ref:`FieldDeclarationFlags <alias-FieldDeclarationFlags>`               +
+----------+--------------------------------------------------------------------------+
+name      + :ref:`builtin::das_string <handle-builtin-das_string>`                   +
+----------+--------------------------------------------------------------------------+
+init      +smart_ptr< :ref:`ast::Expression <handle-ast-Expression>` >               +
+----------+--------------------------------------------------------------------------+
+offset    +int                                                                       +
+----------+--------------------------------------------------------------------------+


|structure_annotation-ast-FieldDeclaration|

.. _handle-ast-EnumEntry:

.. das:attribute:: EnumEntry

EnumEntry fields are

+-----+-----------------------------------------------------------+
+at   + :ref:`rtti::LineInfo <handle-rtti-LineInfo>`              +
+-----+-----------------------------------------------------------+
+value+smart_ptr< :ref:`ast::Expression <handle-ast-Expression>` >+
+-----+-----------------------------------------------------------+
+name + :ref:`builtin::das_string <handle-builtin-das_string>`    +
+-----+-----------------------------------------------------------+


|structure_annotation-ast-EnumEntry|

.. _handle-ast-Enumeration:

.. das:attribute:: Enumeration

Enumeration fields are

+-----------+----------------------------------------------------------+
+at         + :ref:`rtti::LineInfo <handle-rtti-LineInfo>`             +
+-----------+----------------------------------------------------------+
+annotations+ :ref:`rtti::AnnotationList <handle-rtti-AnnotationList>` +
+-----------+----------------------------------------------------------+
+isPrivate  +bool                                                      +
+-----------+----------------------------------------------------------+
+name       + :ref:`builtin::das_string <handle-builtin-das_string>`   +
+-----------+----------------------------------------------------------+
+list       +vector<EnumEntry>                                         +
+-----------+----------------------------------------------------------+
+cppName    + :ref:`builtin::das_string <handle-builtin-das_string>`   +
+-----------+----------------------------------------------------------+
+external   +bool                                                      +
+-----------+----------------------------------------------------------+
+baseType   + :ref:`rtti::Type <enum-rtti-Type>`                       +
+-----------+----------------------------------------------------------+
+_module    + :ref:`rtti::Module <handle-rtti-Module>` ?               +
+-----------+----------------------------------------------------------+


|structure_annotation-ast-Enumeration|

.. _handle-ast-Function:

.. das:attribute:: Function

Function fields are

+---------------+----------------------------------------------------------------+
+index          +int                                                             +
+---------------+----------------------------------------------------------------+
+at             + :ref:`rtti::LineInfo <handle-rtti-LineInfo>`                   +
+---------------+----------------------------------------------------------------+
+annotations    + :ref:`rtti::AnnotationList <handle-rtti-AnnotationList>`       +
+---------------+----------------------------------------------------------------+
+sideEffectFlags+ :ref:`FunctionSideEffectFlags <alias-FunctionSideEffectFlags>` +
+---------------+----------------------------------------------------------------+
+inferStack     +vector<InferHistory>                                            +
+---------------+----------------------------------------------------------------+
+atDecl         + :ref:`rtti::LineInfo <handle-rtti-LineInfo>`                   +
+---------------+----------------------------------------------------------------+
+hash           +uint64                                                          +
+---------------+----------------------------------------------------------------+
+totalGenLabel  +int                                                             +
+---------------+----------------------------------------------------------------+
+flags          + :ref:`FunctionFlags <alias-FunctionFlags>`                     +
+---------------+----------------------------------------------------------------+
+name           + :ref:`builtin::das_string <handle-builtin-das_string>`         +
+---------------+----------------------------------------------------------------+
+aotHash        +uint64                                                          +
+---------------+----------------------------------------------------------------+
+body           +smart_ptr< :ref:`ast::Expression <handle-ast-Expression>` >     +
+---------------+----------------------------------------------------------------+
+_module        + :ref:`rtti::Module <handle-rtti-Module>` ?                     +
+---------------+----------------------------------------------------------------+
+moreFlags      + :ref:`MoreFunctionFlags <alias-MoreFunctionFlags>`             +
+---------------+----------------------------------------------------------------+
+fromGeneric    +smart_ptr< :ref:`ast::Function <handle-ast-Function>` >         +
+---------------+----------------------------------------------------------------+
+arguments      +vector<smart_ptr<Variable>>                                     +
+---------------+----------------------------------------------------------------+
+result         +smart_ptr< :ref:`ast::TypeDecl <handle-ast-TypeDecl>` >         +
+---------------+----------------------------------------------------------------+
+classParent    + :ref:`ast::Structure <handle-ast-Structure>` ?                 +
+---------------+----------------------------------------------------------------+
+totalStackSize +uint                                                            +
+---------------+----------------------------------------------------------------+


Function property operators are

+---------+-------------------------------------------------------+
+origin   +smart_ptr< :ref:`ast::Function <handle-ast-Function>` >+
+---------+-------------------------------------------------------+
+isGeneric+bool                                                   +
+---------+-------------------------------------------------------+


|structure_annotation-ast-Function|

.. _handle-ast-InferHistory:

.. das:attribute:: InferHistory

InferHistory fields are

+----+----------------------------------------------+
+at  + :ref:`rtti::LineInfo <handle-rtti-LineInfo>` +
+----+----------------------------------------------+
+func+ :ref:`ast::Function <handle-ast-Function>` ? +
+----+----------------------------------------------+


|structure_annotation-ast-InferHistory|

.. _handle-ast-Variable:

.. das:attribute:: Variable

Variable fields are

+-------------+--------------------------------------------------------------------------+
+index        +int                                                                       +
+-------------+--------------------------------------------------------------------------+
+at           + :ref:`rtti::LineInfo <handle-rtti-LineInfo>`                             +
+-------------+--------------------------------------------------------------------------+
+initStackSize+uint                                                                      +
+-------------+--------------------------------------------------------------------------+
+annotation   + :ref:`rtti::AnnotationArgumentList <handle-rtti-AnnotationArgumentList>` +
+-------------+--------------------------------------------------------------------------+
+_type        +smart_ptr< :ref:`ast::TypeDecl <handle-ast-TypeDecl>` >                   +
+-------------+--------------------------------------------------------------------------+
+source       +smart_ptr< :ref:`ast::Expression <handle-ast-Expression>` >               +
+-------------+--------------------------------------------------------------------------+
+stackTop     +uint                                                                      +
+-------------+--------------------------------------------------------------------------+
+flags        + :ref:`VariableFlags <alias-VariableFlags>`                               +
+-------------+--------------------------------------------------------------------------+
+name         + :ref:`builtin::das_string <handle-builtin-das_string>`                   +
+-------------+--------------------------------------------------------------------------+
+_aka         + :ref:`builtin::das_string <handle-builtin-das_string>`                   +
+-------------+--------------------------------------------------------------------------+
+init         +smart_ptr< :ref:`ast::Expression <handle-ast-Expression>` >               +
+-------------+--------------------------------------------------------------------------+
+_module      + :ref:`rtti::Module <handle-rtti-Module>` ?                               +
+-------------+--------------------------------------------------------------------------+
+access_flags + :ref:`VariableAccessFlags <alias-VariableAccessFlags>`                   +
+-------------+--------------------------------------------------------------------------+


Variable property operators are

+--------------+----+
+isAccessUnused+bool+
+--------------+----+


|structure_annotation-ast-Variable|

.. _handle-ast-AstContext:

.. das:attribute:: AstContext

AstContext fields are

+------+-------------------------------------------------------+
+_loop +vector<smart_ptr<Expression>>                          +
+------+-------------------------------------------------------+
+scopes+vector<smart_ptr<Expression>>                          +
+------+-------------------------------------------------------+
+func  +smart_ptr< :ref:`ast::Function <handle-ast-Function>` >+
+------+-------------------------------------------------------+
+blocks+vector<smart_ptr<Expression>>                          +
+------+-------------------------------------------------------+
+_with +vector<smart_ptr<Expression>>                          +
+------+-------------------------------------------------------+


|structure_annotation-ast-AstContext|

.. _handle-ast-ExprBlock:

.. das:attribute:: ExprBlock

ExprBlock fields are

+-----------------+----------------------------------------------------------+
+at               + :ref:`rtti::LineInfo <handle-rtti-LineInfo>`             +
+-----------------+----------------------------------------------------------+
+annotations      + :ref:`rtti::AnnotationList <handle-rtti-AnnotationList>` +
+-----------------+----------------------------------------------------------+
+annotationDataSid+uint64                                                    +
+-----------------+----------------------------------------------------------+
+returnType       +smart_ptr< :ref:`ast::TypeDecl <handle-ast-TypeDecl>` >   +
+-----------------+----------------------------------------------------------+
+finalList        +vector<smart_ptr<Expression>>                             +
+-----------------+----------------------------------------------------------+
+_type            +smart_ptr< :ref:`ast::TypeDecl <handle-ast-TypeDecl>` >   +
+-----------------+----------------------------------------------------------+
+maxLabelIndex    +int                                                       +
+-----------------+----------------------------------------------------------+
+genFlags         + :ref:`ExprGenFlags <alias-ExprGenFlags>`                 +
+-----------------+----------------------------------------------------------+
+stackCleanVars   +vector<pair`uint`uint>                                    +
+-----------------+----------------------------------------------------------+
+stackTop         +uint                                                      +
+-----------------+----------------------------------------------------------+
+flags            + :ref:`ExprFlags <alias-ExprFlags>`                       +
+-----------------+----------------------------------------------------------+
+stackVarTop      +uint                                                      +
+-----------------+----------------------------------------------------------+
+list             +vector<smart_ptr<Expression>>                             +
+-----------------+----------------------------------------------------------+
+annotationData   +uint64                                                    +
+-----------------+----------------------------------------------------------+
+__rtti           +string const                                              +
+-----------------+----------------------------------------------------------+
+arguments        +vector<smart_ptr<Variable>>                               +
+-----------------+----------------------------------------------------------+
+blockFlags       + :ref:`ExprBlockFlags <alias-ExprBlockFlags>`             +
+-----------------+----------------------------------------------------------+
+stackVarBottom   +uint                                                      +
+-----------------+----------------------------------------------------------+
+printFlags       + :ref:`ExprPrintFlags <alias-ExprPrintFlags>`             +
+-----------------+----------------------------------------------------------+


|structure_annotation-ast-ExprBlock|

.. _handle-ast-ExprLet:

.. das:attribute:: ExprLet

ExprLet fields are

+----------+-------------------------------------------------------+
+at        + :ref:`rtti::LineInfo <handle-rtti-LineInfo>`          +
+----------+-------------------------------------------------------+
+letFlags  + :ref:`ExprLetFlags <alias-ExprLetFlags>`              +
+----------+-------------------------------------------------------+
+_type     +smart_ptr< :ref:`ast::TypeDecl <handle-ast-TypeDecl>` >+
+----------+-------------------------------------------------------+
+genFlags  + :ref:`ExprGenFlags <alias-ExprGenFlags>`              +
+----------+-------------------------------------------------------+
+flags     + :ref:`ExprFlags <alias-ExprFlags>`                    +
+----------+-------------------------------------------------------+
+__rtti    +string const                                           +
+----------+-------------------------------------------------------+
+variables +vector<smart_ptr<Variable>>                            +
+----------+-------------------------------------------------------+
+atInit    + :ref:`rtti::LineInfo <handle-rtti-LineInfo>`          +
+----------+-------------------------------------------------------+
+printFlags+ :ref:`ExprPrintFlags <alias-ExprPrintFlags>`          +
+----------+-------------------------------------------------------+


|structure_annotation-ast-ExprLet|

.. _handle-ast-ExprStringBuilder:

.. das:attribute:: ExprStringBuilder

ExprStringBuilder fields are

+----------+-------------------------------------------------------+
+at        + :ref:`rtti::LineInfo <handle-rtti-LineInfo>`          +
+----------+-------------------------------------------------------+
+_type     +smart_ptr< :ref:`ast::TypeDecl <handle-ast-TypeDecl>` >+
+----------+-------------------------------------------------------+
+genFlags  + :ref:`ExprGenFlags <alias-ExprGenFlags>`              +
+----------+-------------------------------------------------------+
+flags     + :ref:`ExprFlags <alias-ExprFlags>`                    +
+----------+-------------------------------------------------------+
+elements  +vector<smart_ptr<Expression>>                          +
+----------+-------------------------------------------------------+
+__rtti    +string const                                           +
+----------+-------------------------------------------------------+
+printFlags+ :ref:`ExprPrintFlags <alias-ExprPrintFlags>`          +
+----------+-------------------------------------------------------+


|structure_annotation-ast-ExprStringBuilder|

.. _handle-ast-MakeFieldDecl:

.. das:attribute:: MakeFieldDecl

MakeFieldDecl fields are

+-----+-----------------------------------------------------------+
+at   + :ref:`rtti::LineInfo <handle-rtti-LineInfo>`              +
+-----+-----------------------------------------------------------+
+tag  +smart_ptr< :ref:`ast::Expression <handle-ast-Expression>` >+
+-----+-----------------------------------------------------------+
+value+smart_ptr< :ref:`ast::Expression <handle-ast-Expression>` >+
+-----+-----------------------------------------------------------+
+flags+ :ref:`MakeFieldDeclFlags <alias-MakeFieldDeclFlags>`      +
+-----+-----------------------------------------------------------+
+name + :ref:`builtin::das_string <handle-builtin-das_string>`    +
+-----+-----------------------------------------------------------+


|structure_annotation-ast-MakeFieldDecl|

.. _handle-ast-ExprNamedCall:

.. das:attribute:: ExprNamedCall

ExprNamedCall fields are

+----------------------+--------------------------------------------------------+
+at                    + :ref:`rtti::LineInfo <handle-rtti-LineInfo>`           +
+----------------------+--------------------------------------------------------+
+_type                 +smart_ptr< :ref:`ast::TypeDecl <handle-ast-TypeDecl>` > +
+----------------------+--------------------------------------------------------+
+genFlags              + :ref:`ExprGenFlags <alias-ExprGenFlags>`               +
+----------------------+--------------------------------------------------------+
+flags                 + :ref:`ExprFlags <alias-ExprFlags>`                     +
+----------------------+--------------------------------------------------------+
+name                  + :ref:`builtin::das_string <handle-builtin-das_string>` +
+----------------------+--------------------------------------------------------+
+argumentsFailedToInfer+bool                                                    +
+----------------------+--------------------------------------------------------+
+__rtti                +string const                                            +
+----------------------+--------------------------------------------------------+
+nonNamedArguments     +vector<smart_ptr<Expression>>                           +
+----------------------+--------------------------------------------------------+
+arguments             + :ref:`ast::MakeStruct <handle-ast-MakeStruct>`         +
+----------------------+--------------------------------------------------------+
+printFlags            + :ref:`ExprPrintFlags <alias-ExprPrintFlags>`           +
+----------------------+--------------------------------------------------------+


|structure_annotation-ast-ExprNamedCall|

.. _handle-ast-ExprLooksLikeCall:

.. das:attribute:: ExprLooksLikeCall

ExprLooksLikeCall fields are

+----------------------+--------------------------------------------------------+
+at                    + :ref:`rtti::LineInfo <handle-rtti-LineInfo>`           +
+----------------------+--------------------------------------------------------+
+_type                 +smart_ptr< :ref:`ast::TypeDecl <handle-ast-TypeDecl>` > +
+----------------------+--------------------------------------------------------+
+genFlags              + :ref:`ExprGenFlags <alias-ExprGenFlags>`               +
+----------------------+--------------------------------------------------------+
+flags                 + :ref:`ExprFlags <alias-ExprFlags>`                     +
+----------------------+--------------------------------------------------------+
+name                  + :ref:`builtin::das_string <handle-builtin-das_string>` +
+----------------------+--------------------------------------------------------+
+atEnclosure           + :ref:`rtti::LineInfo <handle-rtti-LineInfo>`           +
+----------------------+--------------------------------------------------------+
+argumentsFailedToInfer+bool                                                    +
+----------------------+--------------------------------------------------------+
+__rtti                +string const                                            +
+----------------------+--------------------------------------------------------+
+arguments             +vector<smart_ptr<Expression>>                           +
+----------------------+--------------------------------------------------------+
+printFlags            + :ref:`ExprPrintFlags <alias-ExprPrintFlags>`           +
+----------------------+--------------------------------------------------------+


|structure_annotation-ast-ExprLooksLikeCall|

.. _handle-ast-ExprCallFunc:

.. das:attribute:: ExprCallFunc

ExprCallFunc fields are

+----------------------+--------------------------------------------------------+
+at                    + :ref:`rtti::LineInfo <handle-rtti-LineInfo>`           +
+----------------------+--------------------------------------------------------+
+_type                 +smart_ptr< :ref:`ast::TypeDecl <handle-ast-TypeDecl>` > +
+----------------------+--------------------------------------------------------+
+genFlags              + :ref:`ExprGenFlags <alias-ExprGenFlags>`               +
+----------------------+--------------------------------------------------------+
+flags                 + :ref:`ExprFlags <alias-ExprFlags>`                     +
+----------------------+--------------------------------------------------------+
+stackTop              +uint                                                    +
+----------------------+--------------------------------------------------------+
+name                  + :ref:`builtin::das_string <handle-builtin-das_string>` +
+----------------------+--------------------------------------------------------+
+atEnclosure           + :ref:`rtti::LineInfo <handle-rtti-LineInfo>`           +
+----------------------+--------------------------------------------------------+
+argumentsFailedToInfer+bool                                                    +
+----------------------+--------------------------------------------------------+
+__rtti                +string const                                            +
+----------------------+--------------------------------------------------------+
+arguments             +vector<smart_ptr<Expression>>                           +
+----------------------+--------------------------------------------------------+
+func                  + :ref:`ast::Function <handle-ast-Function>` ?           +
+----------------------+--------------------------------------------------------+
+printFlags            + :ref:`ExprPrintFlags <alias-ExprPrintFlags>`           +
+----------------------+--------------------------------------------------------+


|structure_annotation-ast-ExprCallFunc|

.. _handle-ast-ExprNew:

.. das:attribute:: ExprNew

ExprNew fields are

+----------------------+--------------------------------------------------------+
+initializer           +bool                                                    +
+----------------------+--------------------------------------------------------+
+at                    + :ref:`rtti::LineInfo <handle-rtti-LineInfo>`           +
+----------------------+--------------------------------------------------------+
+typeexpr              +smart_ptr< :ref:`ast::TypeDecl <handle-ast-TypeDecl>` > +
+----------------------+--------------------------------------------------------+
+_type                 +smart_ptr< :ref:`ast::TypeDecl <handle-ast-TypeDecl>` > +
+----------------------+--------------------------------------------------------+
+genFlags              + :ref:`ExprGenFlags <alias-ExprGenFlags>`               +
+----------------------+--------------------------------------------------------+
+flags                 + :ref:`ExprFlags <alias-ExprFlags>`                     +
+----------------------+--------------------------------------------------------+
+stackTop              +uint                                                    +
+----------------------+--------------------------------------------------------+
+name                  + :ref:`builtin::das_string <handle-builtin-das_string>` +
+----------------------+--------------------------------------------------------+
+atEnclosure           + :ref:`rtti::LineInfo <handle-rtti-LineInfo>`           +
+----------------------+--------------------------------------------------------+
+argumentsFailedToInfer+bool                                                    +
+----------------------+--------------------------------------------------------+
+__rtti                +string const                                            +
+----------------------+--------------------------------------------------------+
+arguments             +vector<smart_ptr<Expression>>                           +
+----------------------+--------------------------------------------------------+
+func                  + :ref:`ast::Function <handle-ast-Function>` ?           +
+----------------------+--------------------------------------------------------+
+printFlags            + :ref:`ExprPrintFlags <alias-ExprPrintFlags>`           +
+----------------------+--------------------------------------------------------+


|structure_annotation-ast-ExprNew|

.. _handle-ast-ExprCall:

.. das:attribute:: ExprCall

ExprCall fields are

+----------------------+--------------------------------------------------------+
+at                    + :ref:`rtti::LineInfo <handle-rtti-LineInfo>`           +
+----------------------+--------------------------------------------------------+
+_type                 +smart_ptr< :ref:`ast::TypeDecl <handle-ast-TypeDecl>` > +
+----------------------+--------------------------------------------------------+
+doesNotNeedSp         +bool                                                    +
+----------------------+--------------------------------------------------------+
+cmresAlias            +bool                                                    +
+----------------------+--------------------------------------------------------+
+genFlags              + :ref:`ExprGenFlags <alias-ExprGenFlags>`               +
+----------------------+--------------------------------------------------------+
+flags                 + :ref:`ExprFlags <alias-ExprFlags>`                     +
+----------------------+--------------------------------------------------------+
+stackTop              +uint                                                    +
+----------------------+--------------------------------------------------------+
+name                  + :ref:`builtin::das_string <handle-builtin-das_string>` +
+----------------------+--------------------------------------------------------+
+atEnclosure           + :ref:`rtti::LineInfo <handle-rtti-LineInfo>`           +
+----------------------+--------------------------------------------------------+
+argumentsFailedToInfer+bool                                                    +
+----------------------+--------------------------------------------------------+
+__rtti                +string const                                            +
+----------------------+--------------------------------------------------------+
+arguments             +vector<smart_ptr<Expression>>                           +
+----------------------+--------------------------------------------------------+
+func                  + :ref:`ast::Function <handle-ast-Function>` ?           +
+----------------------+--------------------------------------------------------+
+printFlags            + :ref:`ExprPrintFlags <alias-ExprPrintFlags>`           +
+----------------------+--------------------------------------------------------+


|structure_annotation-ast-ExprCall|

.. _handle-ast-ExprPtr2Ref:

.. das:attribute:: ExprPtr2Ref

ExprPtr2Ref fields are

+----------+-----------------------------------------------------------+
+at        + :ref:`rtti::LineInfo <handle-rtti-LineInfo>`              +
+----------+-----------------------------------------------------------+
+_type     +smart_ptr< :ref:`ast::TypeDecl <handle-ast-TypeDecl>` >    +
+----------+-----------------------------------------------------------+
+genFlags  + :ref:`ExprGenFlags <alias-ExprGenFlags>`                  +
+----------+-----------------------------------------------------------+
+flags     + :ref:`ExprFlags <alias-ExprFlags>`                        +
+----------+-----------------------------------------------------------+
+subexpr   +smart_ptr< :ref:`ast::Expression <handle-ast-Expression>` >+
+----------+-----------------------------------------------------------+
+__rtti    +string const                                               +
+----------+-----------------------------------------------------------+
+printFlags+ :ref:`ExprPrintFlags <alias-ExprPrintFlags>`              +
+----------+-----------------------------------------------------------+


|structure_annotation-ast-ExprPtr2Ref|

.. _handle-ast-ExprNullCoalescing:

.. das:attribute:: ExprNullCoalescing

ExprNullCoalescing fields are

+------------+-----------------------------------------------------------+
+at          + :ref:`rtti::LineInfo <handle-rtti-LineInfo>`              +
+------------+-----------------------------------------------------------+
+_type       +smart_ptr< :ref:`ast::TypeDecl <handle-ast-TypeDecl>` >    +
+------------+-----------------------------------------------------------+
+defaultValue+smart_ptr< :ref:`ast::Expression <handle-ast-Expression>` >+
+------------+-----------------------------------------------------------+
+genFlags    + :ref:`ExprGenFlags <alias-ExprGenFlags>`                  +
+------------+-----------------------------------------------------------+
+flags       + :ref:`ExprFlags <alias-ExprFlags>`                        +
+------------+-----------------------------------------------------------+
+subexpr     +smart_ptr< :ref:`ast::Expression <handle-ast-Expression>` >+
+------------+-----------------------------------------------------------+
+__rtti      +string const                                               +
+------------+-----------------------------------------------------------+
+printFlags  + :ref:`ExprPrintFlags <alias-ExprPrintFlags>`              +
+------------+-----------------------------------------------------------+


|structure_annotation-ast-ExprNullCoalescing|

.. _handle-ast-ExprAt:

.. das:attribute:: ExprAt

ExprAt fields are

+----------+-----------------------------------------------------------+
+index     +smart_ptr< :ref:`ast::Expression <handle-ast-Expression>` >+
+----------+-----------------------------------------------------------+
+at        + :ref:`rtti::LineInfo <handle-rtti-LineInfo>`              +
+----------+-----------------------------------------------------------+
+_type     +smart_ptr< :ref:`ast::TypeDecl <handle-ast-TypeDecl>` >    +
+----------+-----------------------------------------------------------+
+genFlags  + :ref:`ExprGenFlags <alias-ExprGenFlags>`                  +
+----------+-----------------------------------------------------------+
+flags     + :ref:`ExprFlags <alias-ExprFlags>`                        +
+----------+-----------------------------------------------------------+
+atFlags   + :ref:`ExprAtFlags <alias-ExprAtFlags>`                    +
+----------+-----------------------------------------------------------+
+subexpr   +smart_ptr< :ref:`ast::Expression <handle-ast-Expression>` >+
+----------+-----------------------------------------------------------+
+__rtti    +string const                                               +
+----------+-----------------------------------------------------------+
+printFlags+ :ref:`ExprPrintFlags <alias-ExprPrintFlags>`              +
+----------+-----------------------------------------------------------+


|structure_annotation-ast-ExprAt|

.. _handle-ast-ExprSafeAt:

.. das:attribute:: ExprSafeAt

ExprSafeAt fields are

+----------+-----------------------------------------------------------+
+index     +smart_ptr< :ref:`ast::Expression <handle-ast-Expression>` >+
+----------+-----------------------------------------------------------+
+at        + :ref:`rtti::LineInfo <handle-rtti-LineInfo>`              +
+----------+-----------------------------------------------------------+
+_type     +smart_ptr< :ref:`ast::TypeDecl <handle-ast-TypeDecl>` >    +
+----------+-----------------------------------------------------------+
+genFlags  + :ref:`ExprGenFlags <alias-ExprGenFlags>`                  +
+----------+-----------------------------------------------------------+
+flags     + :ref:`ExprFlags <alias-ExprFlags>`                        +
+----------+-----------------------------------------------------------+
+atFlags   + :ref:`ExprAtFlags <alias-ExprAtFlags>`                    +
+----------+-----------------------------------------------------------+
+subexpr   +smart_ptr< :ref:`ast::Expression <handle-ast-Expression>` >+
+----------+-----------------------------------------------------------+
+__rtti    +string const                                               +
+----------+-----------------------------------------------------------+
+printFlags+ :ref:`ExprPrintFlags <alias-ExprPrintFlags>`              +
+----------+-----------------------------------------------------------+


|structure_annotation-ast-ExprSafeAt|

.. _handle-ast-ExprIs:

.. das:attribute:: ExprIs

ExprIs fields are

+----------+-----------------------------------------------------------+
+at        + :ref:`rtti::LineInfo <handle-rtti-LineInfo>`              +
+----------+-----------------------------------------------------------+
+typeexpr  +smart_ptr< :ref:`ast::TypeDecl <handle-ast-TypeDecl>` >    +
+----------+-----------------------------------------------------------+
+_type     +smart_ptr< :ref:`ast::TypeDecl <handle-ast-TypeDecl>` >    +
+----------+-----------------------------------------------------------+
+genFlags  + :ref:`ExprGenFlags <alias-ExprGenFlags>`                  +
+----------+-----------------------------------------------------------+
+flags     + :ref:`ExprFlags <alias-ExprFlags>`                        +
+----------+-----------------------------------------------------------+
+subexpr   +smart_ptr< :ref:`ast::Expression <handle-ast-Expression>` >+
+----------+-----------------------------------------------------------+
+__rtti    +string const                                               +
+----------+-----------------------------------------------------------+
+printFlags+ :ref:`ExprPrintFlags <alias-ExprPrintFlags>`              +
+----------+-----------------------------------------------------------+


|structure_annotation-ast-ExprIs|

.. _handle-ast-ExprOp:

.. das:attribute:: ExprOp

|structure_annotation-ast-ExprOp|

.. _handle-ast-ExprOp2:

.. das:attribute:: ExprOp2

ExprOp2 fields are

+----------------------+-----------------------------------------------------------+
+at                    + :ref:`rtti::LineInfo <handle-rtti-LineInfo>`              +
+----------------------+-----------------------------------------------------------+
+_type                 +smart_ptr< :ref:`ast::TypeDecl <handle-ast-TypeDecl>` >    +
+----------------------+-----------------------------------------------------------+
+genFlags              + :ref:`ExprGenFlags <alias-ExprGenFlags>`                  +
+----------------------+-----------------------------------------------------------+
+flags                 + :ref:`ExprFlags <alias-ExprFlags>`                        +
+----------------------+-----------------------------------------------------------+
+stackTop              +uint                                                       +
+----------------------+-----------------------------------------------------------+
+name                  + :ref:`builtin::das_string <handle-builtin-das_string>`    +
+----------------------+-----------------------------------------------------------+
+atEnclosure           + :ref:`rtti::LineInfo <handle-rtti-LineInfo>`              +
+----------------------+-----------------------------------------------------------+
+left                  +smart_ptr< :ref:`ast::Expression <handle-ast-Expression>` >+
+----------------------+-----------------------------------------------------------+
+right                 +smart_ptr< :ref:`ast::Expression <handle-ast-Expression>` >+
+----------------------+-----------------------------------------------------------+
+op                    + :ref:`builtin::das_string <handle-builtin-das_string>`    +
+----------------------+-----------------------------------------------------------+
+argumentsFailedToInfer+bool                                                       +
+----------------------+-----------------------------------------------------------+
+__rtti                +string const                                               +
+----------------------+-----------------------------------------------------------+
+arguments             +vector<smart_ptr<Expression>>                              +
+----------------------+-----------------------------------------------------------+
+func                  + :ref:`ast::Function <handle-ast-Function>` ?              +
+----------------------+-----------------------------------------------------------+
+printFlags            + :ref:`ExprPrintFlags <alias-ExprPrintFlags>`              +
+----------------------+-----------------------------------------------------------+


|structure_annotation-ast-ExprOp2|

.. _handle-ast-ExprOp3:

.. das:attribute:: ExprOp3

ExprOp3 fields are

+----------------------+-----------------------------------------------------------+
+at                    + :ref:`rtti::LineInfo <handle-rtti-LineInfo>`              +
+----------------------+-----------------------------------------------------------+
+_type                 +smart_ptr< :ref:`ast::TypeDecl <handle-ast-TypeDecl>` >    +
+----------------------+-----------------------------------------------------------+
+genFlags              + :ref:`ExprGenFlags <alias-ExprGenFlags>`                  +
+----------------------+-----------------------------------------------------------+
+flags                 + :ref:`ExprFlags <alias-ExprFlags>`                        +
+----------------------+-----------------------------------------------------------+
+stackTop              +uint                                                       +
+----------------------+-----------------------------------------------------------+
+name                  + :ref:`builtin::das_string <handle-builtin-das_string>`    +
+----------------------+-----------------------------------------------------------+
+atEnclosure           + :ref:`rtti::LineInfo <handle-rtti-LineInfo>`              +
+----------------------+-----------------------------------------------------------+
+left                  +smart_ptr< :ref:`ast::Expression <handle-ast-Expression>` >+
+----------------------+-----------------------------------------------------------+
+right                 +smart_ptr< :ref:`ast::Expression <handle-ast-Expression>` >+
+----------------------+-----------------------------------------------------------+
+op                    + :ref:`builtin::das_string <handle-builtin-das_string>`    +
+----------------------+-----------------------------------------------------------+
+argumentsFailedToInfer+bool                                                       +
+----------------------+-----------------------------------------------------------+
+subexpr               +smart_ptr< :ref:`ast::Expression <handle-ast-Expression>` >+
+----------------------+-----------------------------------------------------------+
+__rtti                +string const                                               +
+----------------------+-----------------------------------------------------------+
+arguments             +vector<smart_ptr<Expression>>                              +
+----------------------+-----------------------------------------------------------+
+func                  + :ref:`ast::Function <handle-ast-Function>` ?              +
+----------------------+-----------------------------------------------------------+
+printFlags            + :ref:`ExprPrintFlags <alias-ExprPrintFlags>`              +
+----------------------+-----------------------------------------------------------+


|structure_annotation-ast-ExprOp3|

.. _handle-ast-ExprCopy:

.. das:attribute:: ExprCopy

ExprCopy fields are

+----------------------+-----------------------------------------------------------+
+at                    + :ref:`rtti::LineInfo <handle-rtti-LineInfo>`              +
+----------------------+-----------------------------------------------------------+
+_type                 +smart_ptr< :ref:`ast::TypeDecl <handle-ast-TypeDecl>` >    +
+----------------------+-----------------------------------------------------------+
+copy_flags            + :ref:`CopyFlags <alias-CopyFlags>`                        +
+----------------------+-----------------------------------------------------------+
+genFlags              + :ref:`ExprGenFlags <alias-ExprGenFlags>`                  +
+----------------------+-----------------------------------------------------------+
+flags                 + :ref:`ExprFlags <alias-ExprFlags>`                        +
+----------------------+-----------------------------------------------------------+
+stackTop              +uint                                                       +
+----------------------+-----------------------------------------------------------+
+name                  + :ref:`builtin::das_string <handle-builtin-das_string>`    +
+----------------------+-----------------------------------------------------------+
+atEnclosure           + :ref:`rtti::LineInfo <handle-rtti-LineInfo>`              +
+----------------------+-----------------------------------------------------------+
+left                  +smart_ptr< :ref:`ast::Expression <handle-ast-Expression>` >+
+----------------------+-----------------------------------------------------------+
+right                 +smart_ptr< :ref:`ast::Expression <handle-ast-Expression>` >+
+----------------------+-----------------------------------------------------------+
+op                    + :ref:`builtin::das_string <handle-builtin-das_string>`    +
+----------------------+-----------------------------------------------------------+
+argumentsFailedToInfer+bool                                                       +
+----------------------+-----------------------------------------------------------+
+__rtti                +string const                                               +
+----------------------+-----------------------------------------------------------+
+arguments             +vector<smart_ptr<Expression>>                              +
+----------------------+-----------------------------------------------------------+
+func                  + :ref:`ast::Function <handle-ast-Function>` ?              +
+----------------------+-----------------------------------------------------------+
+printFlags            + :ref:`ExprPrintFlags <alias-ExprPrintFlags>`              +
+----------------------+-----------------------------------------------------------+


|structure_annotation-ast-ExprCopy|

.. _handle-ast-ExprMove:

.. das:attribute:: ExprMove

ExprMove fields are

+----------------------+-----------------------------------------------------------+
+at                    + :ref:`rtti::LineInfo <handle-rtti-LineInfo>`              +
+----------------------+-----------------------------------------------------------+
+_type                 +smart_ptr< :ref:`ast::TypeDecl <handle-ast-TypeDecl>` >    +
+----------------------+-----------------------------------------------------------+
+move_flags            + :ref:`MoveFlags <alias-MoveFlags>`                        +
+----------------------+-----------------------------------------------------------+
+genFlags              + :ref:`ExprGenFlags <alias-ExprGenFlags>`                  +
+----------------------+-----------------------------------------------------------+
+flags                 + :ref:`ExprFlags <alias-ExprFlags>`                        +
+----------------------+-----------------------------------------------------------+
+stackTop              +uint                                                       +
+----------------------+-----------------------------------------------------------+
+name                  + :ref:`builtin::das_string <handle-builtin-das_string>`    +
+----------------------+-----------------------------------------------------------+
+atEnclosure           + :ref:`rtti::LineInfo <handle-rtti-LineInfo>`              +
+----------------------+-----------------------------------------------------------+
+left                  +smart_ptr< :ref:`ast::Expression <handle-ast-Expression>` >+
+----------------------+-----------------------------------------------------------+
+right                 +smart_ptr< :ref:`ast::Expression <handle-ast-Expression>` >+
+----------------------+-----------------------------------------------------------+
+op                    + :ref:`builtin::das_string <handle-builtin-das_string>`    +
+----------------------+-----------------------------------------------------------+
+argumentsFailedToInfer+bool                                                       +
+----------------------+-----------------------------------------------------------+
+__rtti                +string const                                               +
+----------------------+-----------------------------------------------------------+
+arguments             +vector<smart_ptr<Expression>>                              +
+----------------------+-----------------------------------------------------------+
+func                  + :ref:`ast::Function <handle-ast-Function>` ?              +
+----------------------+-----------------------------------------------------------+
+printFlags            + :ref:`ExprPrintFlags <alias-ExprPrintFlags>`              +
+----------------------+-----------------------------------------------------------+


|structure_annotation-ast-ExprMove|

.. _handle-ast-ExprClone:

.. das:attribute:: ExprClone

ExprClone fields are

+----------------------+-----------------------------------------------------------+
+at                    + :ref:`rtti::LineInfo <handle-rtti-LineInfo>`              +
+----------------------+-----------------------------------------------------------+
+_type                 +smart_ptr< :ref:`ast::TypeDecl <handle-ast-TypeDecl>` >    +
+----------------------+-----------------------------------------------------------+
+genFlags              + :ref:`ExprGenFlags <alias-ExprGenFlags>`                  +
+----------------------+-----------------------------------------------------------+
+flags                 + :ref:`ExprFlags <alias-ExprFlags>`                        +
+----------------------+-----------------------------------------------------------+
+stackTop              +uint                                                       +
+----------------------+-----------------------------------------------------------+
+name                  + :ref:`builtin::das_string <handle-builtin-das_string>`    +
+----------------------+-----------------------------------------------------------+
+atEnclosure           + :ref:`rtti::LineInfo <handle-rtti-LineInfo>`              +
+----------------------+-----------------------------------------------------------+
+left                  +smart_ptr< :ref:`ast::Expression <handle-ast-Expression>` >+
+----------------------+-----------------------------------------------------------+
+right                 +smart_ptr< :ref:`ast::Expression <handle-ast-Expression>` >+
+----------------------+-----------------------------------------------------------+
+op                    + :ref:`builtin::das_string <handle-builtin-das_string>`    +
+----------------------+-----------------------------------------------------------+
+argumentsFailedToInfer+bool                                                       +
+----------------------+-----------------------------------------------------------+
+__rtti                +string const                                               +
+----------------------+-----------------------------------------------------------+
+arguments             +vector<smart_ptr<Expression>>                              +
+----------------------+-----------------------------------------------------------+
+func                  + :ref:`ast::Function <handle-ast-Function>` ?              +
+----------------------+-----------------------------------------------------------+
+printFlags            + :ref:`ExprPrintFlags <alias-ExprPrintFlags>`              +
+----------------------+-----------------------------------------------------------+


|structure_annotation-ast-ExprClone|

.. _handle-ast-ExprWith:

.. das:attribute:: ExprWith

ExprWith fields are

+----------+-----------------------------------------------------------+
+at        + :ref:`rtti::LineInfo <handle-rtti-LineInfo>`              +
+----------+-----------------------------------------------------------+
+_type     +smart_ptr< :ref:`ast::TypeDecl <handle-ast-TypeDecl>` >    +
+----------+-----------------------------------------------------------+
+genFlags  + :ref:`ExprGenFlags <alias-ExprGenFlags>`                  +
+----------+-----------------------------------------------------------+
+flags     + :ref:`ExprFlags <alias-ExprFlags>`                        +
+----------+-----------------------------------------------------------+
+body      +smart_ptr< :ref:`ast::Expression <handle-ast-Expression>` >+
+----------+-----------------------------------------------------------+
+__rtti    +string const                                               +
+----------+-----------------------------------------------------------+
+printFlags+ :ref:`ExprPrintFlags <alias-ExprPrintFlags>`              +
+----------+-----------------------------------------------------------+
+_with     +smart_ptr< :ref:`ast::Expression <handle-ast-Expression>` >+
+----------+-----------------------------------------------------------+


|structure_annotation-ast-ExprWith|

.. _handle-ast-ExprAssume:

.. das:attribute:: ExprAssume

ExprAssume fields are

+----------+-----------------------------------------------------------+
+at        + :ref:`rtti::LineInfo <handle-rtti-LineInfo>`              +
+----------+-----------------------------------------------------------+
+_type     +smart_ptr< :ref:`ast::TypeDecl <handle-ast-TypeDecl>` >    +
+----------+-----------------------------------------------------------+
+genFlags  + :ref:`ExprGenFlags <alias-ExprGenFlags>`                  +
+----------+-----------------------------------------------------------+
+flags     + :ref:`ExprFlags <alias-ExprFlags>`                        +
+----------+-----------------------------------------------------------+
+alias     + :ref:`builtin::das_string <handle-builtin-das_string>`    +
+----------+-----------------------------------------------------------+
+subexpr   +smart_ptr< :ref:`ast::Expression <handle-ast-Expression>` >+
+----------+-----------------------------------------------------------+
+__rtti    +string const                                               +
+----------+-----------------------------------------------------------+
+printFlags+ :ref:`ExprPrintFlags <alias-ExprPrintFlags>`              +
+----------+-----------------------------------------------------------+


|structure_annotation-ast-ExprAssume|

.. _handle-ast-ExprWhile:

.. das:attribute:: ExprWhile

ExprWhile fields are

+----------+-----------------------------------------------------------+
+at        + :ref:`rtti::LineInfo <handle-rtti-LineInfo>`              +
+----------+-----------------------------------------------------------+
+_type     +smart_ptr< :ref:`ast::TypeDecl <handle-ast-TypeDecl>` >    +
+----------+-----------------------------------------------------------+
+genFlags  + :ref:`ExprGenFlags <alias-ExprGenFlags>`                  +
+----------+-----------------------------------------------------------+
+flags     + :ref:`ExprFlags <alias-ExprFlags>`                        +
+----------+-----------------------------------------------------------+
+body      +smart_ptr< :ref:`ast::Expression <handle-ast-Expression>` >+
+----------+-----------------------------------------------------------+
+cond      +smart_ptr< :ref:`ast::Expression <handle-ast-Expression>` >+
+----------+-----------------------------------------------------------+
+__rtti    +string const                                               +
+----------+-----------------------------------------------------------+
+printFlags+ :ref:`ExprPrintFlags <alias-ExprPrintFlags>`              +
+----------+-----------------------------------------------------------+


|structure_annotation-ast-ExprWhile|

.. _handle-ast-ExprTryCatch:

.. das:attribute:: ExprTryCatch

ExprTryCatch fields are

+-----------+-----------------------------------------------------------+
+at         + :ref:`rtti::LineInfo <handle-rtti-LineInfo>`              +
+-----------+-----------------------------------------------------------+
+try_block  +smart_ptr< :ref:`ast::Expression <handle-ast-Expression>` >+
+-----------+-----------------------------------------------------------+
+_type      +smart_ptr< :ref:`ast::TypeDecl <handle-ast-TypeDecl>` >    +
+-----------+-----------------------------------------------------------+
+genFlags   + :ref:`ExprGenFlags <alias-ExprGenFlags>`                  +
+-----------+-----------------------------------------------------------+
+flags      + :ref:`ExprFlags <alias-ExprFlags>`                        +
+-----------+-----------------------------------------------------------+
+catch_block+smart_ptr< :ref:`ast::Expression <handle-ast-Expression>` >+
+-----------+-----------------------------------------------------------+
+__rtti     +string const                                               +
+-----------+-----------------------------------------------------------+
+printFlags + :ref:`ExprPrintFlags <alias-ExprPrintFlags>`              +
+-----------+-----------------------------------------------------------+


|structure_annotation-ast-ExprTryCatch|

.. _handle-ast-ExprIfThenElse:

.. das:attribute:: ExprIfThenElse

ExprIfThenElse fields are

+----------+-----------------------------------------------------------+
+at        + :ref:`rtti::LineInfo <handle-rtti-LineInfo>`              +
+----------+-----------------------------------------------------------+
+_type     +smart_ptr< :ref:`ast::TypeDecl <handle-ast-TypeDecl>` >    +
+----------+-----------------------------------------------------------+
+if_true   +smart_ptr< :ref:`ast::Expression <handle-ast-Expression>` >+
+----------+-----------------------------------------------------------+
+genFlags  + :ref:`ExprGenFlags <alias-ExprGenFlags>`                  +
+----------+-----------------------------------------------------------+
+flags     + :ref:`ExprFlags <alias-ExprFlags>`                        +
+----------+-----------------------------------------------------------+
+if_false  +smart_ptr< :ref:`ast::Expression <handle-ast-Expression>` >+
+----------+-----------------------------------------------------------+
+cond      +smart_ptr< :ref:`ast::Expression <handle-ast-Expression>` >+
+----------+-----------------------------------------------------------+
+__rtti    +string const                                               +
+----------+-----------------------------------------------------------+
+printFlags+ :ref:`ExprPrintFlags <alias-ExprPrintFlags>`              +
+----------+-----------------------------------------------------------+
+if_flags  + :ref:`IfFlags <alias-IfFlags>`                            +
+----------+-----------------------------------------------------------+


|structure_annotation-ast-ExprIfThenElse|

.. _handle-ast-ExprFor:

.. das:attribute:: ExprFor

ExprFor fields are

+-------------------------+-----------------------------------------------------------+
+at                       + :ref:`rtti::LineInfo <handle-rtti-LineInfo>`              +
+-------------------------+-----------------------------------------------------------+
+iteratorsTags            +vector<smart_ptr<Expression>>                              +
+-------------------------+-----------------------------------------------------------+
+iteratorsAka             +vector<das_string>                                         +
+-------------------------+-----------------------------------------------------------+
+_type                    +smart_ptr< :ref:`ast::TypeDecl <handle-ast-TypeDecl>` >    +
+-------------------------+-----------------------------------------------------------+
+visibility               + :ref:`rtti::LineInfo <handle-rtti-LineInfo>`              +
+-------------------------+-----------------------------------------------------------+
+allowIteratorOptimization+bool                                                       +
+-------------------------+-----------------------------------------------------------+
+genFlags                 + :ref:`ExprGenFlags <alias-ExprGenFlags>`                  +
+-------------------------+-----------------------------------------------------------+
+flags                    + :ref:`ExprFlags <alias-ExprFlags>`                        +
+-------------------------+-----------------------------------------------------------+
+iterators                +vector<das_string>                                         +
+-------------------------+-----------------------------------------------------------+
+body                     +smart_ptr< :ref:`ast::Expression <handle-ast-Expression>` >+
+-------------------------+-----------------------------------------------------------+
+sources                  +vector<smart_ptr<Expression>>                              +
+-------------------------+-----------------------------------------------------------+
+iteratorVariables        +vector<smart_ptr<Variable>>                                +
+-------------------------+-----------------------------------------------------------+
+iteratorsAt              +vector<LineInfo>                                           +
+-------------------------+-----------------------------------------------------------+
+canShadow                +bool                                                       +
+-------------------------+-----------------------------------------------------------+
+__rtti                   +string const                                               +
+-------------------------+-----------------------------------------------------------+
+printFlags               + :ref:`ExprPrintFlags <alias-ExprPrintFlags>`              +
+-------------------------+-----------------------------------------------------------+


|structure_annotation-ast-ExprFor|

.. _handle-ast-ExprMakeLocal:

.. das:attribute:: ExprMakeLocal

ExprMakeLocal fields are

+-----------+-------------------------------------------------------+
+at         + :ref:`rtti::LineInfo <handle-rtti-LineInfo>`          +
+-----------+-------------------------------------------------------+
+makeFlags  + :ref:`ExprMakeLocalFlags <alias-ExprMakeLocalFlags>`  +
+-----------+-------------------------------------------------------+
+makeType   +smart_ptr< :ref:`ast::TypeDecl <handle-ast-TypeDecl>` >+
+-----------+-------------------------------------------------------+
+_type      +smart_ptr< :ref:`ast::TypeDecl <handle-ast-TypeDecl>` >+
+-----------+-------------------------------------------------------+
+genFlags   + :ref:`ExprGenFlags <alias-ExprGenFlags>`              +
+-----------+-------------------------------------------------------+
+flags      + :ref:`ExprFlags <alias-ExprFlags>`                    +
+-----------+-------------------------------------------------------+
+stackTop   +uint                                                   +
+-----------+-------------------------------------------------------+
+extraOffset+uint                                                   +
+-----------+-------------------------------------------------------+
+__rtti     +string const                                           +
+-----------+-------------------------------------------------------+
+printFlags + :ref:`ExprPrintFlags <alias-ExprPrintFlags>`          +
+-----------+-------------------------------------------------------+


|structure_annotation-ast-ExprMakeLocal|

.. _handle-ast-ExprMakeStruct:

.. das:attribute:: ExprMakeStruct

ExprMakeStruct fields are

+---------------+-----------------------------------------------------------+
+at             + :ref:`rtti::LineInfo <handle-rtti-LineInfo>`              +
+---------------+-----------------------------------------------------------+
+makeFlags      + :ref:`ExprMakeLocalFlags <alias-ExprMakeLocalFlags>`      +
+---------------+-----------------------------------------------------------+
+makeType       +smart_ptr< :ref:`ast::TypeDecl <handle-ast-TypeDecl>` >    +
+---------------+-----------------------------------------------------------+
+_type          +smart_ptr< :ref:`ast::TypeDecl <handle-ast-TypeDecl>` >    +
+---------------+-----------------------------------------------------------+
+genFlags       + :ref:`ExprGenFlags <alias-ExprGenFlags>`                  +
+---------------+-----------------------------------------------------------+
+flags          + :ref:`ExprFlags <alias-ExprFlags>`                        +
+---------------+-----------------------------------------------------------+
+stackTop       +uint                                                       +
+---------------+-----------------------------------------------------------+
+structs        +vector<smart_ptr<MakeStruct>>                              +
+---------------+-----------------------------------------------------------+
+extraOffset    +uint                                                       +
+---------------+-----------------------------------------------------------+
+_block         +smart_ptr< :ref:`ast::Expression <handle-ast-Expression>` >+
+---------------+-----------------------------------------------------------+
+makeStructFlags+ :ref:`ExprMakeStructFlags <alias-ExprMakeStructFlags>`    +
+---------------+-----------------------------------------------------------+
+__rtti         +string const                                               +
+---------------+-----------------------------------------------------------+
+printFlags     + :ref:`ExprPrintFlags <alias-ExprPrintFlags>`              +
+---------------+-----------------------------------------------------------+


|structure_annotation-ast-ExprMakeStruct|

.. _handle-ast-ExprMakeVariant:

.. das:attribute:: ExprMakeVariant

ExprMakeVariant fields are

+-----------+-------------------------------------------------------+
+at         + :ref:`rtti::LineInfo <handle-rtti-LineInfo>`          +
+-----------+-------------------------------------------------------+
+makeFlags  + :ref:`ExprMakeLocalFlags <alias-ExprMakeLocalFlags>`  +
+-----------+-------------------------------------------------------+
+variants   +vector<smart_ptr<MakeFieldDecl>>                       +
+-----------+-------------------------------------------------------+
+makeType   +smart_ptr< :ref:`ast::TypeDecl <handle-ast-TypeDecl>` >+
+-----------+-------------------------------------------------------+
+_type      +smart_ptr< :ref:`ast::TypeDecl <handle-ast-TypeDecl>` >+
+-----------+-------------------------------------------------------+
+genFlags   + :ref:`ExprGenFlags <alias-ExprGenFlags>`              +
+-----------+-------------------------------------------------------+
+flags      + :ref:`ExprFlags <alias-ExprFlags>`                    +
+-----------+-------------------------------------------------------+
+stackTop   +uint                                                   +
+-----------+-------------------------------------------------------+
+extraOffset+uint                                                   +
+-----------+-------------------------------------------------------+
+__rtti     +string const                                           +
+-----------+-------------------------------------------------------+
+printFlags + :ref:`ExprPrintFlags <alias-ExprPrintFlags>`          +
+-----------+-------------------------------------------------------+


|structure_annotation-ast-ExprMakeVariant|

.. _handle-ast-ExprMakeArray:

.. das:attribute:: ExprMakeArray

ExprMakeArray fields are

+-----------+-------------------------------------------------------+
+at         + :ref:`rtti::LineInfo <handle-rtti-LineInfo>`          +
+-----------+-------------------------------------------------------+
+makeFlags  + :ref:`ExprMakeLocalFlags <alias-ExprMakeLocalFlags>`  +
+-----------+-------------------------------------------------------+
+makeType   +smart_ptr< :ref:`ast::TypeDecl <handle-ast-TypeDecl>` >+
+-----------+-------------------------------------------------------+
+_type      +smart_ptr< :ref:`ast::TypeDecl <handle-ast-TypeDecl>` >+
+-----------+-------------------------------------------------------+
+recordType +smart_ptr< :ref:`ast::TypeDecl <handle-ast-TypeDecl>` >+
+-----------+-------------------------------------------------------+
+genFlags   + :ref:`ExprGenFlags <alias-ExprGenFlags>`              +
+-----------+-------------------------------------------------------+
+flags      + :ref:`ExprFlags <alias-ExprFlags>`                    +
+-----------+-------------------------------------------------------+
+stackTop   +uint                                                   +
+-----------+-------------------------------------------------------+
+extraOffset+uint                                                   +
+-----------+-------------------------------------------------------+
+values     +vector<smart_ptr<Expression>>                          +
+-----------+-------------------------------------------------------+
+__rtti     +string const                                           +
+-----------+-------------------------------------------------------+
+printFlags + :ref:`ExprPrintFlags <alias-ExprPrintFlags>`          +
+-----------+-------------------------------------------------------+


|structure_annotation-ast-ExprMakeArray|

.. _handle-ast-ExprMakeTuple:

.. das:attribute:: ExprMakeTuple

ExprMakeTuple fields are

+-----------+-------------------------------------------------------+
+at         + :ref:`rtti::LineInfo <handle-rtti-LineInfo>`          +
+-----------+-------------------------------------------------------+
+isKeyValue +bool                                                   +
+-----------+-------------------------------------------------------+
+makeFlags  + :ref:`ExprMakeLocalFlags <alias-ExprMakeLocalFlags>`  +
+-----------+-------------------------------------------------------+
+makeType   +smart_ptr< :ref:`ast::TypeDecl <handle-ast-TypeDecl>` >+
+-----------+-------------------------------------------------------+
+_type      +smart_ptr< :ref:`ast::TypeDecl <handle-ast-TypeDecl>` >+
+-----------+-------------------------------------------------------+
+recordType +smart_ptr< :ref:`ast::TypeDecl <handle-ast-TypeDecl>` >+
+-----------+-------------------------------------------------------+
+genFlags   + :ref:`ExprGenFlags <alias-ExprGenFlags>`              +
+-----------+-------------------------------------------------------+
+flags      + :ref:`ExprFlags <alias-ExprFlags>`                    +
+-----------+-------------------------------------------------------+
+stackTop   +uint                                                   +
+-----------+-------------------------------------------------------+
+extraOffset+uint                                                   +
+-----------+-------------------------------------------------------+
+values     +vector<smart_ptr<Expression>>                          +
+-----------+-------------------------------------------------------+
+__rtti     +string const                                           +
+-----------+-------------------------------------------------------+
+printFlags + :ref:`ExprPrintFlags <alias-ExprPrintFlags>`          +
+-----------+-------------------------------------------------------+


|structure_annotation-ast-ExprMakeTuple|

.. _handle-ast-ExprArrayComprehension:

.. das:attribute:: ExprArrayComprehension

ExprArrayComprehension fields are

+---------------+-----------------------------------------------------------+
+at             + :ref:`rtti::LineInfo <handle-rtti-LineInfo>`              +
+---------------+-----------------------------------------------------------+
+generatorSyntax+bool                                                       +
+---------------+-----------------------------------------------------------+
+_type          +smart_ptr< :ref:`ast::TypeDecl <handle-ast-TypeDecl>` >    +
+---------------+-----------------------------------------------------------+
+genFlags       + :ref:`ExprGenFlags <alias-ExprGenFlags>`                  +
+---------------+-----------------------------------------------------------+
+flags          + :ref:`ExprFlags <alias-ExprFlags>`                        +
+---------------+-----------------------------------------------------------+
+subexpr        +smart_ptr< :ref:`ast::Expression <handle-ast-Expression>` >+
+---------------+-----------------------------------------------------------+
+__rtti         +string const                                               +
+---------------+-----------------------------------------------------------+
+exprFor        +smart_ptr< :ref:`ast::Expression <handle-ast-Expression>` >+
+---------------+-----------------------------------------------------------+
+printFlags     + :ref:`ExprPrintFlags <alias-ExprPrintFlags>`              +
+---------------+-----------------------------------------------------------+
+exprWhere      +smart_ptr< :ref:`ast::Expression <handle-ast-Expression>` >+
+---------------+-----------------------------------------------------------+


|structure_annotation-ast-ExprArrayComprehension|

.. _handle-ast-TypeInfoMacro:

.. das:attribute:: TypeInfoMacro

TypeInfoMacro fields are

+-------+--------------------------------------------------------+
+name   + :ref:`builtin::das_string <handle-builtin-das_string>` +
+-------+--------------------------------------------------------+
+_module+ :ref:`rtti::Module <handle-rtti-Module>` ?             +
+-------+--------------------------------------------------------+


|structure_annotation-ast-TypeInfoMacro|

.. _handle-ast-ExprTypeInfo:

.. das:attribute:: ExprTypeInfo

ExprTypeInfo fields are

+----------+-----------------------------------------------------------+
+at        + :ref:`rtti::LineInfo <handle-rtti-LineInfo>`              +
+----------+-----------------------------------------------------------+
+macro     + :ref:`ast::TypeInfoMacro <handle-ast-TypeInfoMacro>` ?    +
+----------+-----------------------------------------------------------+
+typeexpr  +smart_ptr< :ref:`ast::TypeDecl <handle-ast-TypeDecl>` >    +
+----------+-----------------------------------------------------------+
+_type     +smart_ptr< :ref:`ast::TypeDecl <handle-ast-TypeDecl>` >    +
+----------+-----------------------------------------------------------+
+trait     + :ref:`builtin::das_string <handle-builtin-das_string>`    +
+----------+-----------------------------------------------------------+
+genFlags  + :ref:`ExprGenFlags <alias-ExprGenFlags>`                  +
+----------+-----------------------------------------------------------+
+flags     + :ref:`ExprFlags <alias-ExprFlags>`                        +
+----------+-----------------------------------------------------------+
+subtrait  + :ref:`builtin::das_string <handle-builtin-das_string>`    +
+----------+-----------------------------------------------------------+
+extratrait+ :ref:`builtin::das_string <handle-builtin-das_string>`    +
+----------+-----------------------------------------------------------+
+subexpr   +smart_ptr< :ref:`ast::Expression <handle-ast-Expression>` >+
+----------+-----------------------------------------------------------+
+__rtti    +string const                                               +
+----------+-----------------------------------------------------------+
+printFlags+ :ref:`ExprPrintFlags <alias-ExprPrintFlags>`              +
+----------+-----------------------------------------------------------+


|structure_annotation-ast-ExprTypeInfo|

.. _handle-ast-ExprTypeDecl:

.. das:attribute:: ExprTypeDecl

ExprTypeDecl fields are

+----------+-------------------------------------------------------+
+at        + :ref:`rtti::LineInfo <handle-rtti-LineInfo>`          +
+----------+-------------------------------------------------------+
+typeexpr  +smart_ptr< :ref:`ast::TypeDecl <handle-ast-TypeDecl>` >+
+----------+-------------------------------------------------------+
+_type     +smart_ptr< :ref:`ast::TypeDecl <handle-ast-TypeDecl>` >+
+----------+-------------------------------------------------------+
+genFlags  + :ref:`ExprGenFlags <alias-ExprGenFlags>`              +
+----------+-------------------------------------------------------+
+flags     + :ref:`ExprFlags <alias-ExprFlags>`                    +
+----------+-------------------------------------------------------+
+__rtti    +string const                                           +
+----------+-------------------------------------------------------+
+printFlags+ :ref:`ExprPrintFlags <alias-ExprPrintFlags>`          +
+----------+-------------------------------------------------------+


|structure_annotation-ast-ExprTypeDecl|

.. _handle-ast-ExprLabel:

.. das:attribute:: ExprLabel

ExprLabel fields are

+----------+--------------------------------------------------------+
+at        + :ref:`rtti::LineInfo <handle-rtti-LineInfo>`           +
+----------+--------------------------------------------------------+
+comment   + :ref:`builtin::das_string <handle-builtin-das_string>` +
+----------+--------------------------------------------------------+
+_type     +smart_ptr< :ref:`ast::TypeDecl <handle-ast-TypeDecl>` > +
+----------+--------------------------------------------------------+
+genFlags  + :ref:`ExprGenFlags <alias-ExprGenFlags>`               +
+----------+--------------------------------------------------------+
+flags     + :ref:`ExprFlags <alias-ExprFlags>`                     +
+----------+--------------------------------------------------------+
+labelName +int                                                     +
+----------+--------------------------------------------------------+
+__rtti    +string const                                            +
+----------+--------------------------------------------------------+
+printFlags+ :ref:`ExprPrintFlags <alias-ExprPrintFlags>`           +
+----------+--------------------------------------------------------+


|structure_annotation-ast-ExprLabel|

.. _handle-ast-ExprGoto:

.. das:attribute:: ExprGoto

ExprGoto fields are

+----------+-----------------------------------------------------------+
+at        + :ref:`rtti::LineInfo <handle-rtti-LineInfo>`              +
+----------+-----------------------------------------------------------+
+_type     +smart_ptr< :ref:`ast::TypeDecl <handle-ast-TypeDecl>` >    +
+----------+-----------------------------------------------------------+
+genFlags  + :ref:`ExprGenFlags <alias-ExprGenFlags>`                  +
+----------+-----------------------------------------------------------+
+flags     + :ref:`ExprFlags <alias-ExprFlags>`                        +
+----------+-----------------------------------------------------------+
+labelName +int                                                        +
+----------+-----------------------------------------------------------+
+subexpr   +smart_ptr< :ref:`ast::Expression <handle-ast-Expression>` >+
+----------+-----------------------------------------------------------+
+__rtti    +string const                                               +
+----------+-----------------------------------------------------------+
+printFlags+ :ref:`ExprPrintFlags <alias-ExprPrintFlags>`              +
+----------+-----------------------------------------------------------+


|structure_annotation-ast-ExprGoto|

.. _handle-ast-ExprRef2Value:

.. das:attribute:: ExprRef2Value

ExprRef2Value fields are

+----------+-----------------------------------------------------------+
+at        + :ref:`rtti::LineInfo <handle-rtti-LineInfo>`              +
+----------+-----------------------------------------------------------+
+_type     +smart_ptr< :ref:`ast::TypeDecl <handle-ast-TypeDecl>` >    +
+----------+-----------------------------------------------------------+
+genFlags  + :ref:`ExprGenFlags <alias-ExprGenFlags>`                  +
+----------+-----------------------------------------------------------+
+flags     + :ref:`ExprFlags <alias-ExprFlags>`                        +
+----------+-----------------------------------------------------------+
+subexpr   +smart_ptr< :ref:`ast::Expression <handle-ast-Expression>` >+
+----------+-----------------------------------------------------------+
+__rtti    +string const                                               +
+----------+-----------------------------------------------------------+
+printFlags+ :ref:`ExprPrintFlags <alias-ExprPrintFlags>`              +
+----------+-----------------------------------------------------------+


|structure_annotation-ast-ExprRef2Value|

.. _handle-ast-ExprRef2Ptr:

.. das:attribute:: ExprRef2Ptr

ExprRef2Ptr fields are

+----------+-----------------------------------------------------------+
+at        + :ref:`rtti::LineInfo <handle-rtti-LineInfo>`              +
+----------+-----------------------------------------------------------+
+_type     +smart_ptr< :ref:`ast::TypeDecl <handle-ast-TypeDecl>` >    +
+----------+-----------------------------------------------------------+
+genFlags  + :ref:`ExprGenFlags <alias-ExprGenFlags>`                  +
+----------+-----------------------------------------------------------+
+flags     + :ref:`ExprFlags <alias-ExprFlags>`                        +
+----------+-----------------------------------------------------------+
+subexpr   +smart_ptr< :ref:`ast::Expression <handle-ast-Expression>` >+
+----------+-----------------------------------------------------------+
+__rtti    +string const                                               +
+----------+-----------------------------------------------------------+
+printFlags+ :ref:`ExprPrintFlags <alias-ExprPrintFlags>`              +
+----------+-----------------------------------------------------------+


|structure_annotation-ast-ExprRef2Ptr|

.. _handle-ast-ExprAddr:

.. das:attribute:: ExprAddr

ExprAddr fields are

+----------+--------------------------------------------------------+
+at        + :ref:`rtti::LineInfo <handle-rtti-LineInfo>`           +
+----------+--------------------------------------------------------+
+_type     +smart_ptr< :ref:`ast::TypeDecl <handle-ast-TypeDecl>` > +
+----------+--------------------------------------------------------+
+genFlags  + :ref:`ExprGenFlags <alias-ExprGenFlags>`               +
+----------+--------------------------------------------------------+
+flags     + :ref:`ExprFlags <alias-ExprFlags>`                     +
+----------+--------------------------------------------------------+
+funcType  +smart_ptr< :ref:`ast::TypeDecl <handle-ast-TypeDecl>` > +
+----------+--------------------------------------------------------+
+target    + :ref:`builtin::das_string <handle-builtin-das_string>` +
+----------+--------------------------------------------------------+
+__rtti    +string const                                            +
+----------+--------------------------------------------------------+
+func      + :ref:`ast::Function <handle-ast-Function>` ?           +
+----------+--------------------------------------------------------+
+printFlags+ :ref:`ExprPrintFlags <alias-ExprPrintFlags>`           +
+----------+--------------------------------------------------------+


|structure_annotation-ast-ExprAddr|

.. _handle-ast-ExprAssert:

.. das:attribute:: ExprAssert

ExprAssert fields are

+----------------------+--------------------------------------------------------+
+at                    + :ref:`rtti::LineInfo <handle-rtti-LineInfo>`           +
+----------------------+--------------------------------------------------------+
+_type                 +smart_ptr< :ref:`ast::TypeDecl <handle-ast-TypeDecl>` > +
+----------------------+--------------------------------------------------------+
+genFlags              + :ref:`ExprGenFlags <alias-ExprGenFlags>`               +
+----------------------+--------------------------------------------------------+
+flags                 + :ref:`ExprFlags <alias-ExprFlags>`                     +
+----------------------+--------------------------------------------------------+
+name                  + :ref:`builtin::das_string <handle-builtin-das_string>` +
+----------------------+--------------------------------------------------------+
+isVerify              +bool                                                    +
+----------------------+--------------------------------------------------------+
+atEnclosure           + :ref:`rtti::LineInfo <handle-rtti-LineInfo>`           +
+----------------------+--------------------------------------------------------+
+argumentsFailedToInfer+bool                                                    +
+----------------------+--------------------------------------------------------+
+__rtti                +string const                                            +
+----------------------+--------------------------------------------------------+
+arguments             +vector<smart_ptr<Expression>>                           +
+----------------------+--------------------------------------------------------+
+printFlags            + :ref:`ExprPrintFlags <alias-ExprPrintFlags>`           +
+----------------------+--------------------------------------------------------+


|structure_annotation-ast-ExprAssert|

.. _handle-ast-ExprQuote:

.. das:attribute:: ExprQuote

ExprQuote fields are

+----------------------+--------------------------------------------------------+
+at                    + :ref:`rtti::LineInfo <handle-rtti-LineInfo>`           +
+----------------------+--------------------------------------------------------+
+_type                 +smart_ptr< :ref:`ast::TypeDecl <handle-ast-TypeDecl>` > +
+----------------------+--------------------------------------------------------+
+genFlags              + :ref:`ExprGenFlags <alias-ExprGenFlags>`               +
+----------------------+--------------------------------------------------------+
+flags                 + :ref:`ExprFlags <alias-ExprFlags>`                     +
+----------------------+--------------------------------------------------------+
+name                  + :ref:`builtin::das_string <handle-builtin-das_string>` +
+----------------------+--------------------------------------------------------+
+atEnclosure           + :ref:`rtti::LineInfo <handle-rtti-LineInfo>`           +
+----------------------+--------------------------------------------------------+
+argumentsFailedToInfer+bool                                                    +
+----------------------+--------------------------------------------------------+
+__rtti                +string const                                            +
+----------------------+--------------------------------------------------------+
+arguments             +vector<smart_ptr<Expression>>                           +
+----------------------+--------------------------------------------------------+
+printFlags            + :ref:`ExprPrintFlags <alias-ExprPrintFlags>`           +
+----------------------+--------------------------------------------------------+


|structure_annotation-ast-ExprQuote|

.. _handle-ast-ExprStaticAssert:

.. das:attribute:: ExprStaticAssert

ExprStaticAssert fields are

+----------------------+--------------------------------------------------------+
+at                    + :ref:`rtti::LineInfo <handle-rtti-LineInfo>`           +
+----------------------+--------------------------------------------------------+
+_type                 +smart_ptr< :ref:`ast::TypeDecl <handle-ast-TypeDecl>` > +
+----------------------+--------------------------------------------------------+
+genFlags              + :ref:`ExprGenFlags <alias-ExprGenFlags>`               +
+----------------------+--------------------------------------------------------+
+flags                 + :ref:`ExprFlags <alias-ExprFlags>`                     +
+----------------------+--------------------------------------------------------+
+name                  + :ref:`builtin::das_string <handle-builtin-das_string>` +
+----------------------+--------------------------------------------------------+
+atEnclosure           + :ref:`rtti::LineInfo <handle-rtti-LineInfo>`           +
+----------------------+--------------------------------------------------------+
+argumentsFailedToInfer+bool                                                    +
+----------------------+--------------------------------------------------------+
+__rtti                +string const                                            +
+----------------------+--------------------------------------------------------+
+arguments             +vector<smart_ptr<Expression>>                           +
+----------------------+--------------------------------------------------------+
+printFlags            + :ref:`ExprPrintFlags <alias-ExprPrintFlags>`           +
+----------------------+--------------------------------------------------------+


|structure_annotation-ast-ExprStaticAssert|

.. _handle-ast-ExprDebug:

.. das:attribute:: ExprDebug

ExprDebug fields are

+----------------------+--------------------------------------------------------+
+at                    + :ref:`rtti::LineInfo <handle-rtti-LineInfo>`           +
+----------------------+--------------------------------------------------------+
+_type                 +smart_ptr< :ref:`ast::TypeDecl <handle-ast-TypeDecl>` > +
+----------------------+--------------------------------------------------------+
+genFlags              + :ref:`ExprGenFlags <alias-ExprGenFlags>`               +
+----------------------+--------------------------------------------------------+
+flags                 + :ref:`ExprFlags <alias-ExprFlags>`                     +
+----------------------+--------------------------------------------------------+
+name                  + :ref:`builtin::das_string <handle-builtin-das_string>` +
+----------------------+--------------------------------------------------------+
+atEnclosure           + :ref:`rtti::LineInfo <handle-rtti-LineInfo>`           +
+----------------------+--------------------------------------------------------+
+argumentsFailedToInfer+bool                                                    +
+----------------------+--------------------------------------------------------+
+__rtti                +string const                                            +
+----------------------+--------------------------------------------------------+
+arguments             +vector<smart_ptr<Expression>>                           +
+----------------------+--------------------------------------------------------+
+printFlags            + :ref:`ExprPrintFlags <alias-ExprPrintFlags>`           +
+----------------------+--------------------------------------------------------+


|structure_annotation-ast-ExprDebug|

.. _handle-ast-ExprInvoke:

.. das:attribute:: ExprInvoke

ExprInvoke fields are

+----------------------+--------------------------------------------------------+
+at                    + :ref:`rtti::LineInfo <handle-rtti-LineInfo>`           +
+----------------------+--------------------------------------------------------+
+isInvokeMethod        +bool                                                    +
+----------------------+--------------------------------------------------------+
+_type                 +smart_ptr< :ref:`ast::TypeDecl <handle-ast-TypeDecl>` > +
+----------------------+--------------------------------------------------------+
+doesNotNeedSp         +bool                                                    +
+----------------------+--------------------------------------------------------+
+genFlags              + :ref:`ExprGenFlags <alias-ExprGenFlags>`               +
+----------------------+--------------------------------------------------------+
+flags                 + :ref:`ExprFlags <alias-ExprFlags>`                     +
+----------------------+--------------------------------------------------------+
+stackTop              +uint                                                    +
+----------------------+--------------------------------------------------------+
+name                  + :ref:`builtin::das_string <handle-builtin-das_string>` +
+----------------------+--------------------------------------------------------+
+atEnclosure           + :ref:`rtti::LineInfo <handle-rtti-LineInfo>`           +
+----------------------+--------------------------------------------------------+
+argumentsFailedToInfer+bool                                                    +
+----------------------+--------------------------------------------------------+
+__rtti                +string const                                            +
+----------------------+--------------------------------------------------------+
+arguments             +vector<smart_ptr<Expression>>                           +
+----------------------+--------------------------------------------------------+
+printFlags            + :ref:`ExprPrintFlags <alias-ExprPrintFlags>`           +
+----------------------+--------------------------------------------------------+


|structure_annotation-ast-ExprInvoke|

.. _handle-ast-ExprErase:

.. das:attribute:: ExprErase

ExprErase fields are

+----------------------+--------------------------------------------------------+
+at                    + :ref:`rtti::LineInfo <handle-rtti-LineInfo>`           +
+----------------------+--------------------------------------------------------+
+_type                 +smart_ptr< :ref:`ast::TypeDecl <handle-ast-TypeDecl>` > +
+----------------------+--------------------------------------------------------+
+genFlags              + :ref:`ExprGenFlags <alias-ExprGenFlags>`               +
+----------------------+--------------------------------------------------------+
+flags                 + :ref:`ExprFlags <alias-ExprFlags>`                     +
+----------------------+--------------------------------------------------------+
+name                  + :ref:`builtin::das_string <handle-builtin-das_string>` +
+----------------------+--------------------------------------------------------+
+atEnclosure           + :ref:`rtti::LineInfo <handle-rtti-LineInfo>`           +
+----------------------+--------------------------------------------------------+
+argumentsFailedToInfer+bool                                                    +
+----------------------+--------------------------------------------------------+
+__rtti                +string const                                            +
+----------------------+--------------------------------------------------------+
+arguments             +vector<smart_ptr<Expression>>                           +
+----------------------+--------------------------------------------------------+
+printFlags            + :ref:`ExprPrintFlags <alias-ExprPrintFlags>`           +
+----------------------+--------------------------------------------------------+


|structure_annotation-ast-ExprErase|

.. _handle-ast-ExprSetInsert:

.. das:attribute:: ExprSetInsert

ExprSetInsert fields are

+----------------------+--------------------------------------------------------+
+at                    + :ref:`rtti::LineInfo <handle-rtti-LineInfo>`           +
+----------------------+--------------------------------------------------------+
+_type                 +smart_ptr< :ref:`ast::TypeDecl <handle-ast-TypeDecl>` > +
+----------------------+--------------------------------------------------------+
+genFlags              + :ref:`ExprGenFlags <alias-ExprGenFlags>`               +
+----------------------+--------------------------------------------------------+
+flags                 + :ref:`ExprFlags <alias-ExprFlags>`                     +
+----------------------+--------------------------------------------------------+
+name                  + :ref:`builtin::das_string <handle-builtin-das_string>` +
+----------------------+--------------------------------------------------------+
+atEnclosure           + :ref:`rtti::LineInfo <handle-rtti-LineInfo>`           +
+----------------------+--------------------------------------------------------+
+argumentsFailedToInfer+bool                                                    +
+----------------------+--------------------------------------------------------+
+__rtti                +string const                                            +
+----------------------+--------------------------------------------------------+
+arguments             +vector<smart_ptr<Expression>>                           +
+----------------------+--------------------------------------------------------+
+printFlags            + :ref:`ExprPrintFlags <alias-ExprPrintFlags>`           +
+----------------------+--------------------------------------------------------+


|structure_annotation-ast-ExprSetInsert|

.. _handle-ast-ExprFind:

.. das:attribute:: ExprFind

ExprFind fields are

+----------------------+--------------------------------------------------------+
+at                    + :ref:`rtti::LineInfo <handle-rtti-LineInfo>`           +
+----------------------+--------------------------------------------------------+
+_type                 +smart_ptr< :ref:`ast::TypeDecl <handle-ast-TypeDecl>` > +
+----------------------+--------------------------------------------------------+
+genFlags              + :ref:`ExprGenFlags <alias-ExprGenFlags>`               +
+----------------------+--------------------------------------------------------+
+flags                 + :ref:`ExprFlags <alias-ExprFlags>`                     +
+----------------------+--------------------------------------------------------+
+name                  + :ref:`builtin::das_string <handle-builtin-das_string>` +
+----------------------+--------------------------------------------------------+
+atEnclosure           + :ref:`rtti::LineInfo <handle-rtti-LineInfo>`           +
+----------------------+--------------------------------------------------------+
+argumentsFailedToInfer+bool                                                    +
+----------------------+--------------------------------------------------------+
+__rtti                +string const                                            +
+----------------------+--------------------------------------------------------+
+arguments             +vector<smart_ptr<Expression>>                           +
+----------------------+--------------------------------------------------------+
+printFlags            + :ref:`ExprPrintFlags <alias-ExprPrintFlags>`           +
+----------------------+--------------------------------------------------------+


|structure_annotation-ast-ExprFind|

.. _handle-ast-ExprKeyExists:

.. das:attribute:: ExprKeyExists

ExprKeyExists fields are

+----------------------+--------------------------------------------------------+
+at                    + :ref:`rtti::LineInfo <handle-rtti-LineInfo>`           +
+----------------------+--------------------------------------------------------+
+_type                 +smart_ptr< :ref:`ast::TypeDecl <handle-ast-TypeDecl>` > +
+----------------------+--------------------------------------------------------+
+genFlags              + :ref:`ExprGenFlags <alias-ExprGenFlags>`               +
+----------------------+--------------------------------------------------------+
+flags                 + :ref:`ExprFlags <alias-ExprFlags>`                     +
+----------------------+--------------------------------------------------------+
+name                  + :ref:`builtin::das_string <handle-builtin-das_string>` +
+----------------------+--------------------------------------------------------+
+atEnclosure           + :ref:`rtti::LineInfo <handle-rtti-LineInfo>`           +
+----------------------+--------------------------------------------------------+
+argumentsFailedToInfer+bool                                                    +
+----------------------+--------------------------------------------------------+
+__rtti                +string const                                            +
+----------------------+--------------------------------------------------------+
+arguments             +vector<smart_ptr<Expression>>                           +
+----------------------+--------------------------------------------------------+
+printFlags            + :ref:`ExprPrintFlags <alias-ExprPrintFlags>`           +
+----------------------+--------------------------------------------------------+


|structure_annotation-ast-ExprKeyExists|

.. _handle-ast-ExprAscend:

.. das:attribute:: ExprAscend

ExprAscend fields are

+-----------+-----------------------------------------------------------+
+at         + :ref:`rtti::LineInfo <handle-rtti-LineInfo>`              +
+-----------+-----------------------------------------------------------+
+_type      +smart_ptr< :ref:`ast::TypeDecl <handle-ast-TypeDecl>` >    +
+-----------+-----------------------------------------------------------+
+ascendFlags+ :ref:`ExprAscendFlags <alias-ExprAscendFlags>`            +
+-----------+-----------------------------------------------------------+
+genFlags   + :ref:`ExprGenFlags <alias-ExprGenFlags>`                  +
+-----------+-----------------------------------------------------------+
+flags      + :ref:`ExprFlags <alias-ExprFlags>`                        +
+-----------+-----------------------------------------------------------+
+ascType    +smart_ptr< :ref:`ast::TypeDecl <handle-ast-TypeDecl>` >    +
+-----------+-----------------------------------------------------------+
+stackTop   +uint                                                       +
+-----------+-----------------------------------------------------------+
+subexpr    +smart_ptr< :ref:`ast::Expression <handle-ast-Expression>` >+
+-----------+-----------------------------------------------------------+
+__rtti     +string const                                               +
+-----------+-----------------------------------------------------------+
+printFlags + :ref:`ExprPrintFlags <alias-ExprPrintFlags>`              +
+-----------+-----------------------------------------------------------+


|structure_annotation-ast-ExprAscend|

.. _handle-ast-ExprCast:

.. das:attribute:: ExprCast

ExprCast fields are

+----------+-----------------------------------------------------------+
+castType  +smart_ptr< :ref:`ast::TypeDecl <handle-ast-TypeDecl>` >    +
+----------+-----------------------------------------------------------+
+at        + :ref:`rtti::LineInfo <handle-rtti-LineInfo>`              +
+----------+-----------------------------------------------------------+
+_type     +smart_ptr< :ref:`ast::TypeDecl <handle-ast-TypeDecl>` >    +
+----------+-----------------------------------------------------------+
+genFlags  + :ref:`ExprGenFlags <alias-ExprGenFlags>`                  +
+----------+-----------------------------------------------------------+
+flags     + :ref:`ExprFlags <alias-ExprFlags>`                        +
+----------+-----------------------------------------------------------+
+subexpr   +smart_ptr< :ref:`ast::Expression <handle-ast-Expression>` >+
+----------+-----------------------------------------------------------+
+__rtti    +string const                                               +
+----------+-----------------------------------------------------------+
+castFlags + :ref:`ExprCastFlags <alias-ExprCastFlags>`                +
+----------+-----------------------------------------------------------+
+printFlags+ :ref:`ExprPrintFlags <alias-ExprPrintFlags>`              +
+----------+-----------------------------------------------------------+


|structure_annotation-ast-ExprCast|

.. _handle-ast-ExprDelete:

.. das:attribute:: ExprDelete

ExprDelete fields are

+----------+-----------------------------------------------------------+
+at        + :ref:`rtti::LineInfo <handle-rtti-LineInfo>`              +
+----------+-----------------------------------------------------------+
+_type     +smart_ptr< :ref:`ast::TypeDecl <handle-ast-TypeDecl>` >    +
+----------+-----------------------------------------------------------+
+genFlags  + :ref:`ExprGenFlags <alias-ExprGenFlags>`                  +
+----------+-----------------------------------------------------------+
+flags     + :ref:`ExprFlags <alias-ExprFlags>`                        +
+----------+-----------------------------------------------------------+
+subexpr   +smart_ptr< :ref:`ast::Expression <handle-ast-Expression>` >+
+----------+-----------------------------------------------------------+
+__rtti    +string const                                               +
+----------+-----------------------------------------------------------+
+native    +bool                                                       +
+----------+-----------------------------------------------------------+
+printFlags+ :ref:`ExprPrintFlags <alias-ExprPrintFlags>`              +
+----------+-----------------------------------------------------------+


|structure_annotation-ast-ExprDelete|

.. _handle-ast-ExprVar:

.. das:attribute:: ExprVar

ExprVar fields are

+-------------+--------------------------------------------------------+
+at           + :ref:`rtti::LineInfo <handle-rtti-LineInfo>`           +
+-------------+--------------------------------------------------------+
+_type        +smart_ptr< :ref:`ast::TypeDecl <handle-ast-TypeDecl>` > +
+-------------+--------------------------------------------------------+
+genFlags     + :ref:`ExprGenFlags <alias-ExprGenFlags>`               +
+-------------+--------------------------------------------------------+
+flags        + :ref:`ExprFlags <alias-ExprFlags>`                     +
+-------------+--------------------------------------------------------+
+variable     +smart_ptr< :ref:`ast::Variable <handle-ast-Variable>` > +
+-------------+--------------------------------------------------------+
+name         + :ref:`builtin::das_string <handle-builtin-das_string>` +
+-------------+--------------------------------------------------------+
+argumentIndex+int                                                     +
+-------------+--------------------------------------------------------+
+__rtti       +string const                                            +
+-------------+--------------------------------------------------------+
+pBlock       + :ref:`ast::ExprBlock <handle-ast-ExprBlock>` ?         +
+-------------+--------------------------------------------------------+
+varFlags     + :ref:`ExprVarFlags <alias-ExprVarFlags>`               +
+-------------+--------------------------------------------------------+
+printFlags   + :ref:`ExprPrintFlags <alias-ExprPrintFlags>`           +
+-------------+--------------------------------------------------------+


|structure_annotation-ast-ExprVar|

.. _handle-ast-ExprTag:

.. das:attribute:: ExprTag

ExprTag fields are

+----------+-----------------------------------------------------------+
+at        + :ref:`rtti::LineInfo <handle-rtti-LineInfo>`              +
+----------+-----------------------------------------------------------+
+_type     +smart_ptr< :ref:`ast::TypeDecl <handle-ast-TypeDecl>` >    +
+----------+-----------------------------------------------------------+
+value     +smart_ptr< :ref:`ast::Expression <handle-ast-Expression>` >+
+----------+-----------------------------------------------------------+
+genFlags  + :ref:`ExprGenFlags <alias-ExprGenFlags>`                  +
+----------+-----------------------------------------------------------+
+flags     + :ref:`ExprFlags <alias-ExprFlags>`                        +
+----------+-----------------------------------------------------------+
+name      + :ref:`builtin::das_string <handle-builtin-das_string>`    +
+----------+-----------------------------------------------------------+
+subexpr   +smart_ptr< :ref:`ast::Expression <handle-ast-Expression>` >+
+----------+-----------------------------------------------------------+
+__rtti    +string const                                               +
+----------+-----------------------------------------------------------+
+printFlags+ :ref:`ExprPrintFlags <alias-ExprPrintFlags>`              +
+----------+-----------------------------------------------------------+


|structure_annotation-ast-ExprTag|

.. _handle-ast-ExprSwizzle:

.. das:attribute:: ExprSwizzle

ExprSwizzle fields are

+----------+------------------------------------------------------------+
+at        + :ref:`rtti::LineInfo <handle-rtti-LineInfo>`               +
+----------+------------------------------------------------------------+
+_type     +smart_ptr< :ref:`ast::TypeDecl <handle-ast-TypeDecl>` >     +
+----------+------------------------------------------------------------+
+value     +smart_ptr< :ref:`ast::Expression <handle-ast-Expression>` > +
+----------+------------------------------------------------------------+
+genFlags  + :ref:`ExprGenFlags <alias-ExprGenFlags>`                   +
+----------+------------------------------------------------------------+
+flags     + :ref:`ExprFlags <alias-ExprFlags>`                         +
+----------+------------------------------------------------------------+
+fieldFlags+ :ref:`ExprSwizzleFieldFlags <alias-ExprSwizzleFieldFlags>` +
+----------+------------------------------------------------------------+
+fields    +vector<uint8>                                               +
+----------+------------------------------------------------------------+
+__rtti    +string const                                                +
+----------+------------------------------------------------------------+
+printFlags+ :ref:`ExprPrintFlags <alias-ExprPrintFlags>`               +
+----------+------------------------------------------------------------+
+mask      + :ref:`builtin::das_string <handle-builtin-das_string>`     +
+----------+------------------------------------------------------------+


|structure_annotation-ast-ExprSwizzle|

.. _handle-ast-ExprField:

.. das:attribute:: ExprField

ExprField fields are

+----------+-------------------------------------------------------------------------+
+at        + :ref:`rtti::LineInfo <handle-rtti-LineInfo>`                            +
+----------+-------------------------------------------------------------------------+
+field     + :ref:`ast::FieldDeclaration <handle-ast-FieldDeclaration>`  const? const+
+----------+-------------------------------------------------------------------------+
+annotation+smart_ptr< :ref:`rtti::TypeAnnotation <handle-rtti-TypeAnnotation>` >    +
+----------+-------------------------------------------------------------------------+
+_type     +smart_ptr< :ref:`ast::TypeDecl <handle-ast-TypeDecl>` >                  +
+----------+-------------------------------------------------------------------------+
+derefFlags+ :ref:`ExprFieldDerefFlags <alias-ExprFieldDerefFlags>`                  +
+----------+-------------------------------------------------------------------------+
+value     +smart_ptr< :ref:`ast::Expression <handle-ast-Expression>` >              +
+----------+-------------------------------------------------------------------------+
+genFlags  + :ref:`ExprGenFlags <alias-ExprGenFlags>`                                +
+----------+-------------------------------------------------------------------------+
+flags     + :ref:`ExprFlags <alias-ExprFlags>`                                      +
+----------+-------------------------------------------------------------------------+
+name      + :ref:`builtin::das_string <handle-builtin-das_string>`                  +
+----------+-------------------------------------------------------------------------+
+atField   + :ref:`rtti::LineInfo <handle-rtti-LineInfo>`                            +
+----------+-------------------------------------------------------------------------+
+fieldFlags+ :ref:`ExprFieldFieldFlags <alias-ExprFieldFieldFlags>`                  +
+----------+-------------------------------------------------------------------------+
+__rtti    +string const                                                             +
+----------+-------------------------------------------------------------------------+
+fieldIndex+int                                                                      +
+----------+-------------------------------------------------------------------------+
+printFlags+ :ref:`ExprPrintFlags <alias-ExprPrintFlags>`                            +
+----------+-------------------------------------------------------------------------+


|structure_annotation-ast-ExprField|

.. _handle-ast-ExprSafeField:

.. das:attribute:: ExprSafeField

ExprSafeField fields are

+----------+-------------------------------------------------------------------------+
+at        + :ref:`rtti::LineInfo <handle-rtti-LineInfo>`                            +
+----------+-------------------------------------------------------------------------+
+field     + :ref:`ast::FieldDeclaration <handle-ast-FieldDeclaration>`  const? const+
+----------+-------------------------------------------------------------------------+
+annotation+smart_ptr< :ref:`rtti::TypeAnnotation <handle-rtti-TypeAnnotation>` >    +
+----------+-------------------------------------------------------------------------+
+_type     +smart_ptr< :ref:`ast::TypeDecl <handle-ast-TypeDecl>` >                  +
+----------+-------------------------------------------------------------------------+
+derefFlags+ :ref:`ExprFieldDerefFlags <alias-ExprFieldDerefFlags>`                  +
+----------+-------------------------------------------------------------------------+
+value     +smart_ptr< :ref:`ast::Expression <handle-ast-Expression>` >              +
+----------+-------------------------------------------------------------------------+
+genFlags  + :ref:`ExprGenFlags <alias-ExprGenFlags>`                                +
+----------+-------------------------------------------------------------------------+
+flags     + :ref:`ExprFlags <alias-ExprFlags>`                                      +
+----------+-------------------------------------------------------------------------+
+name      + :ref:`builtin::das_string <handle-builtin-das_string>`                  +
+----------+-------------------------------------------------------------------------+
+atField   + :ref:`rtti::LineInfo <handle-rtti-LineInfo>`                            +
+----------+-------------------------------------------------------------------------+
+fieldFlags+ :ref:`ExprFieldFieldFlags <alias-ExprFieldFieldFlags>`                  +
+----------+-------------------------------------------------------------------------+
+__rtti    +string const                                                             +
+----------+-------------------------------------------------------------------------+
+fieldIndex+int                                                                      +
+----------+-------------------------------------------------------------------------+
+printFlags+ :ref:`ExprPrintFlags <alias-ExprPrintFlags>`                            +
+----------+-------------------------------------------------------------------------+
+skipQQ    +bool                                                                     +
+----------+-------------------------------------------------------------------------+


|structure_annotation-ast-ExprSafeField|

.. _handle-ast-ExprIsVariant:

.. das:attribute:: ExprIsVariant

ExprIsVariant fields are

+----------+-------------------------------------------------------------------------+
+at        + :ref:`rtti::LineInfo <handle-rtti-LineInfo>`                            +
+----------+-------------------------------------------------------------------------+
+field     + :ref:`ast::FieldDeclaration <handle-ast-FieldDeclaration>`  const? const+
+----------+-------------------------------------------------------------------------+
+annotation+smart_ptr< :ref:`rtti::TypeAnnotation <handle-rtti-TypeAnnotation>` >    +
+----------+-------------------------------------------------------------------------+
+_type     +smart_ptr< :ref:`ast::TypeDecl <handle-ast-TypeDecl>` >                  +
+----------+-------------------------------------------------------------------------+
+derefFlags+ :ref:`ExprFieldDerefFlags <alias-ExprFieldDerefFlags>`                  +
+----------+-------------------------------------------------------------------------+
+value     +smart_ptr< :ref:`ast::Expression <handle-ast-Expression>` >              +
+----------+-------------------------------------------------------------------------+
+genFlags  + :ref:`ExprGenFlags <alias-ExprGenFlags>`                                +
+----------+-------------------------------------------------------------------------+
+flags     + :ref:`ExprFlags <alias-ExprFlags>`                                      +
+----------+-------------------------------------------------------------------------+
+name      + :ref:`builtin::das_string <handle-builtin-das_string>`                  +
+----------+-------------------------------------------------------------------------+
+atField   + :ref:`rtti::LineInfo <handle-rtti-LineInfo>`                            +
+----------+-------------------------------------------------------------------------+
+fieldFlags+ :ref:`ExprFieldFieldFlags <alias-ExprFieldFieldFlags>`                  +
+----------+-------------------------------------------------------------------------+
+__rtti    +string const                                                             +
+----------+-------------------------------------------------------------------------+
+fieldIndex+int                                                                      +
+----------+-------------------------------------------------------------------------+
+printFlags+ :ref:`ExprPrintFlags <alias-ExprPrintFlags>`                            +
+----------+-------------------------------------------------------------------------+


|structure_annotation-ast-ExprIsVariant|

.. _handle-ast-ExprAsVariant:

.. das:attribute:: ExprAsVariant

ExprAsVariant fields are

+----------+-------------------------------------------------------------------------+
+at        + :ref:`rtti::LineInfo <handle-rtti-LineInfo>`                            +
+----------+-------------------------------------------------------------------------+
+field     + :ref:`ast::FieldDeclaration <handle-ast-FieldDeclaration>`  const? const+
+----------+-------------------------------------------------------------------------+
+annotation+smart_ptr< :ref:`rtti::TypeAnnotation <handle-rtti-TypeAnnotation>` >    +
+----------+-------------------------------------------------------------------------+
+_type     +smart_ptr< :ref:`ast::TypeDecl <handle-ast-TypeDecl>` >                  +
+----------+-------------------------------------------------------------------------+
+derefFlags+ :ref:`ExprFieldDerefFlags <alias-ExprFieldDerefFlags>`                  +
+----------+-------------------------------------------------------------------------+
+value     +smart_ptr< :ref:`ast::Expression <handle-ast-Expression>` >              +
+----------+-------------------------------------------------------------------------+
+genFlags  + :ref:`ExprGenFlags <alias-ExprGenFlags>`                                +
+----------+-------------------------------------------------------------------------+
+flags     + :ref:`ExprFlags <alias-ExprFlags>`                                      +
+----------+-------------------------------------------------------------------------+
+name      + :ref:`builtin::das_string <handle-builtin-das_string>`                  +
+----------+-------------------------------------------------------------------------+
+atField   + :ref:`rtti::LineInfo <handle-rtti-LineInfo>`                            +
+----------+-------------------------------------------------------------------------+
+fieldFlags+ :ref:`ExprFieldFieldFlags <alias-ExprFieldFieldFlags>`                  +
+----------+-------------------------------------------------------------------------+
+__rtti    +string const                                                             +
+----------+-------------------------------------------------------------------------+
+fieldIndex+int                                                                      +
+----------+-------------------------------------------------------------------------+
+printFlags+ :ref:`ExprPrintFlags <alias-ExprPrintFlags>`                            +
+----------+-------------------------------------------------------------------------+


|structure_annotation-ast-ExprAsVariant|

.. _handle-ast-ExprSafeAsVariant:

.. das:attribute:: ExprSafeAsVariant

ExprSafeAsVariant fields are

+----------+-------------------------------------------------------------------------+
+at        + :ref:`rtti::LineInfo <handle-rtti-LineInfo>`                            +
+----------+-------------------------------------------------------------------------+
+field     + :ref:`ast::FieldDeclaration <handle-ast-FieldDeclaration>`  const? const+
+----------+-------------------------------------------------------------------------+
+annotation+smart_ptr< :ref:`rtti::TypeAnnotation <handle-rtti-TypeAnnotation>` >    +
+----------+-------------------------------------------------------------------------+
+_type     +smart_ptr< :ref:`ast::TypeDecl <handle-ast-TypeDecl>` >                  +
+----------+-------------------------------------------------------------------------+
+derefFlags+ :ref:`ExprFieldDerefFlags <alias-ExprFieldDerefFlags>`                  +
+----------+-------------------------------------------------------------------------+
+value     +smart_ptr< :ref:`ast::Expression <handle-ast-Expression>` >              +
+----------+-------------------------------------------------------------------------+
+genFlags  + :ref:`ExprGenFlags <alias-ExprGenFlags>`                                +
+----------+-------------------------------------------------------------------------+
+flags     + :ref:`ExprFlags <alias-ExprFlags>`                                      +
+----------+-------------------------------------------------------------------------+
+name      + :ref:`builtin::das_string <handle-builtin-das_string>`                  +
+----------+-------------------------------------------------------------------------+
+atField   + :ref:`rtti::LineInfo <handle-rtti-LineInfo>`                            +
+----------+-------------------------------------------------------------------------+
+fieldFlags+ :ref:`ExprFieldFieldFlags <alias-ExprFieldFieldFlags>`                  +
+----------+-------------------------------------------------------------------------+
+__rtti    +string const                                                             +
+----------+-------------------------------------------------------------------------+
+fieldIndex+int                                                                      +
+----------+-------------------------------------------------------------------------+
+printFlags+ :ref:`ExprPrintFlags <alias-ExprPrintFlags>`                            +
+----------+-------------------------------------------------------------------------+
+skipQQ    +bool                                                                     +
+----------+-------------------------------------------------------------------------+


|structure_annotation-ast-ExprSafeAsVariant|

.. _handle-ast-ExprOp1:

.. das:attribute:: ExprOp1

ExprOp1 fields are

+----------------------+-----------------------------------------------------------+
+at                    + :ref:`rtti::LineInfo <handle-rtti-LineInfo>`              +
+----------------------+-----------------------------------------------------------+
+_type                 +smart_ptr< :ref:`ast::TypeDecl <handle-ast-TypeDecl>` >    +
+----------------------+-----------------------------------------------------------+
+genFlags              + :ref:`ExprGenFlags <alias-ExprGenFlags>`                  +
+----------------------+-----------------------------------------------------------+
+flags                 + :ref:`ExprFlags <alias-ExprFlags>`                        +
+----------------------+-----------------------------------------------------------+
+stackTop              +uint                                                       +
+----------------------+-----------------------------------------------------------+
+name                  + :ref:`builtin::das_string <handle-builtin-das_string>`    +
+----------------------+-----------------------------------------------------------+
+atEnclosure           + :ref:`rtti::LineInfo <handle-rtti-LineInfo>`              +
+----------------------+-----------------------------------------------------------+
+op                    + :ref:`builtin::das_string <handle-builtin-das_string>`    +
+----------------------+-----------------------------------------------------------+
+argumentsFailedToInfer+bool                                                       +
+----------------------+-----------------------------------------------------------+
+subexpr               +smart_ptr< :ref:`ast::Expression <handle-ast-Expression>` >+
+----------------------+-----------------------------------------------------------+
+__rtti                +string const                                               +
+----------------------+-----------------------------------------------------------+
+arguments             +vector<smart_ptr<Expression>>                              +
+----------------------+-----------------------------------------------------------+
+func                  + :ref:`ast::Function <handle-ast-Function>` ?              +
+----------------------+-----------------------------------------------------------+
+printFlags            + :ref:`ExprPrintFlags <alias-ExprPrintFlags>`              +
+----------------------+-----------------------------------------------------------+


|structure_annotation-ast-ExprOp1|

.. _handle-ast-ExprReturn:

.. das:attribute:: ExprReturn

ExprReturn fields are

+-----------+-----------------------------------------------------------+
+at         + :ref:`rtti::LineInfo <handle-rtti-LineInfo>`              +
+-----------+-----------------------------------------------------------+
+_type      +smart_ptr< :ref:`ast::TypeDecl <handle-ast-TypeDecl>` >    +
+-----------+-----------------------------------------------------------+
+genFlags   + :ref:`ExprGenFlags <alias-ExprGenFlags>`                  +
+-----------+-----------------------------------------------------------+
+flags      + :ref:`ExprFlags <alias-ExprFlags>`                        +
+-----------+-----------------------------------------------------------+
+stackTop   +uint                                                       +
+-----------+-----------------------------------------------------------+
+block      + :ref:`ast::ExprBlock <handle-ast-ExprBlock>` ?            +
+-----------+-----------------------------------------------------------+
+subexpr    +smart_ptr< :ref:`ast::Expression <handle-ast-Expression>` >+
+-----------+-----------------------------------------------------------+
+refStackTop+uint                                                       +
+-----------+-----------------------------------------------------------+
+__rtti     +string const                                               +
+-----------+-----------------------------------------------------------+
+printFlags + :ref:`ExprPrintFlags <alias-ExprPrintFlags>`              +
+-----------+-----------------------------------------------------------+
+returnFlags+ :ref:`ExprReturnFlags <alias-ExprReturnFlags>`            +
+-----------+-----------------------------------------------------------+


|structure_annotation-ast-ExprReturn|

.. _handle-ast-ExprYield:

.. das:attribute:: ExprYield

ExprYield fields are

+-----------+-----------------------------------------------------------+
+at         + :ref:`rtti::LineInfo <handle-rtti-LineInfo>`              +
+-----------+-----------------------------------------------------------+
+_type      +smart_ptr< :ref:`ast::TypeDecl <handle-ast-TypeDecl>` >    +
+-----------+-----------------------------------------------------------+
+genFlags   + :ref:`ExprGenFlags <alias-ExprGenFlags>`                  +
+-----------+-----------------------------------------------------------+
+flags      + :ref:`ExprFlags <alias-ExprFlags>`                        +
+-----------+-----------------------------------------------------------+
+subexpr    +smart_ptr< :ref:`ast::Expression <handle-ast-Expression>` >+
+-----------+-----------------------------------------------------------+
+__rtti     +string const                                               +
+-----------+-----------------------------------------------------------+
+printFlags + :ref:`ExprPrintFlags <alias-ExprPrintFlags>`              +
+-----------+-----------------------------------------------------------+
+returnFlags+ :ref:`ExprYieldFlags <alias-ExprYieldFlags>`              +
+-----------+-----------------------------------------------------------+


|structure_annotation-ast-ExprYield|

.. _handle-ast-ExprBreak:

.. das:attribute:: ExprBreak

ExprBreak fields are

+----------+-------------------------------------------------------+
+at        + :ref:`rtti::LineInfo <handle-rtti-LineInfo>`          +
+----------+-------------------------------------------------------+
+_type     +smart_ptr< :ref:`ast::TypeDecl <handle-ast-TypeDecl>` >+
+----------+-------------------------------------------------------+
+genFlags  + :ref:`ExprGenFlags <alias-ExprGenFlags>`              +
+----------+-------------------------------------------------------+
+flags     + :ref:`ExprFlags <alias-ExprFlags>`                    +
+----------+-------------------------------------------------------+
+__rtti    +string const                                           +
+----------+-------------------------------------------------------+
+printFlags+ :ref:`ExprPrintFlags <alias-ExprPrintFlags>`          +
+----------+-------------------------------------------------------+


|structure_annotation-ast-ExprBreak|

.. _handle-ast-ExprContinue:

.. das:attribute:: ExprContinue

ExprContinue fields are

+----------+-------------------------------------------------------+
+at        + :ref:`rtti::LineInfo <handle-rtti-LineInfo>`          +
+----------+-------------------------------------------------------+
+_type     +smart_ptr< :ref:`ast::TypeDecl <handle-ast-TypeDecl>` >+
+----------+-------------------------------------------------------+
+genFlags  + :ref:`ExprGenFlags <alias-ExprGenFlags>`              +
+----------+-------------------------------------------------------+
+flags     + :ref:`ExprFlags <alias-ExprFlags>`                    +
+----------+-------------------------------------------------------+
+__rtti    +string const                                           +
+----------+-------------------------------------------------------+
+printFlags+ :ref:`ExprPrintFlags <alias-ExprPrintFlags>`          +
+----------+-------------------------------------------------------+


|structure_annotation-ast-ExprContinue|

.. _handle-ast-ExprConst:

.. das:attribute:: ExprConst

ExprConst fields are

+----------+-------------------------------------------------------+
+at        + :ref:`rtti::LineInfo <handle-rtti-LineInfo>`          +
+----------+-------------------------------------------------------+
+_type     +smart_ptr< :ref:`ast::TypeDecl <handle-ast-TypeDecl>` >+
+----------+-------------------------------------------------------+
+genFlags  + :ref:`ExprGenFlags <alias-ExprGenFlags>`              +
+----------+-------------------------------------------------------+
+flags     + :ref:`ExprFlags <alias-ExprFlags>`                    +
+----------+-------------------------------------------------------+
+baseType  + :ref:`rtti::Type <enum-rtti-Type>`                    +
+----------+-------------------------------------------------------+
+__rtti    +string const                                           +
+----------+-------------------------------------------------------+
+printFlags+ :ref:`ExprPrintFlags <alias-ExprPrintFlags>`          +
+----------+-------------------------------------------------------+


|structure_annotation-ast-ExprConst|

.. _handle-ast-ExprFakeContext:

.. das:attribute:: ExprFakeContext

ExprFakeContext fields are

+----------+-------------------------------------------------------+
+at        + :ref:`rtti::LineInfo <handle-rtti-LineInfo>`          +
+----------+-------------------------------------------------------+
+_type     +smart_ptr< :ref:`ast::TypeDecl <handle-ast-TypeDecl>` >+
+----------+-------------------------------------------------------+
+genFlags  + :ref:`ExprGenFlags <alias-ExprGenFlags>`              +
+----------+-------------------------------------------------------+
+flags     + :ref:`ExprFlags <alias-ExprFlags>`                    +
+----------+-------------------------------------------------------+
+baseType  + :ref:`rtti::Type <enum-rtti-Type>`                    +
+----------+-------------------------------------------------------+
+__rtti    +string const                                           +
+----------+-------------------------------------------------------+
+printFlags+ :ref:`ExprPrintFlags <alias-ExprPrintFlags>`          +
+----------+-------------------------------------------------------+


|structure_annotation-ast-ExprFakeContext|

.. _handle-ast-ExprFakeLineInfo:

.. das:attribute:: ExprFakeLineInfo

ExprFakeLineInfo fields are

+----------+-------------------------------------------------------+
+at        + :ref:`rtti::LineInfo <handle-rtti-LineInfo>`          +
+----------+-------------------------------------------------------+
+_type     +smart_ptr< :ref:`ast::TypeDecl <handle-ast-TypeDecl>` >+
+----------+-------------------------------------------------------+
+genFlags  + :ref:`ExprGenFlags <alias-ExprGenFlags>`              +
+----------+-------------------------------------------------------+
+flags     + :ref:`ExprFlags <alias-ExprFlags>`                    +
+----------+-------------------------------------------------------+
+baseType  + :ref:`rtti::Type <enum-rtti-Type>`                    +
+----------+-------------------------------------------------------+
+__rtti    +string const                                           +
+----------+-------------------------------------------------------+
+printFlags+ :ref:`ExprPrintFlags <alias-ExprPrintFlags>`          +
+----------+-------------------------------------------------------+


|structure_annotation-ast-ExprFakeLineInfo|

.. _handle-ast-ExprConstPtr:

.. das:attribute:: ExprConstPtr

ExprConstPtr fields are

+----------+-------------------------------------------------------+
+at        + :ref:`rtti::LineInfo <handle-rtti-LineInfo>`          +
+----------+-------------------------------------------------------+
+_type     +smart_ptr< :ref:`ast::TypeDecl <handle-ast-TypeDecl>` >+
+----------+-------------------------------------------------------+
+value     +void?                                                  +
+----------+-------------------------------------------------------+
+genFlags  + :ref:`ExprGenFlags <alias-ExprGenFlags>`              +
+----------+-------------------------------------------------------+
+flags     + :ref:`ExprFlags <alias-ExprFlags>`                    +
+----------+-------------------------------------------------------+
+baseType  + :ref:`rtti::Type <enum-rtti-Type>`                    +
+----------+-------------------------------------------------------+
+__rtti    +string const                                           +
+----------+-------------------------------------------------------+
+printFlags+ :ref:`ExprPrintFlags <alias-ExprPrintFlags>`          +
+----------+-------------------------------------------------------+


|structure_annotation-ast-ExprConstPtr|

.. _handle-ast-ExprConstInt8:

.. das:attribute:: ExprConstInt8

ExprConstInt8 fields are

+----------+-------------------------------------------------------+
+at        + :ref:`rtti::LineInfo <handle-rtti-LineInfo>`          +
+----------+-------------------------------------------------------+
+_type     +smart_ptr< :ref:`ast::TypeDecl <handle-ast-TypeDecl>` >+
+----------+-------------------------------------------------------+
+value     +int8                                                   +
+----------+-------------------------------------------------------+
+genFlags  + :ref:`ExprGenFlags <alias-ExprGenFlags>`              +
+----------+-------------------------------------------------------+
+flags     + :ref:`ExprFlags <alias-ExprFlags>`                    +
+----------+-------------------------------------------------------+
+baseType  + :ref:`rtti::Type <enum-rtti-Type>`                    +
+----------+-------------------------------------------------------+
+__rtti    +string const                                           +
+----------+-------------------------------------------------------+
+printFlags+ :ref:`ExprPrintFlags <alias-ExprPrintFlags>`          +
+----------+-------------------------------------------------------+


|structure_annotation-ast-ExprConstInt8|

.. _handle-ast-ExprConstInt16:

.. das:attribute:: ExprConstInt16

ExprConstInt16 fields are

+----------+-------------------------------------------------------+
+at        + :ref:`rtti::LineInfo <handle-rtti-LineInfo>`          +
+----------+-------------------------------------------------------+
+_type     +smart_ptr< :ref:`ast::TypeDecl <handle-ast-TypeDecl>` >+
+----------+-------------------------------------------------------+
+value     +int16                                                  +
+----------+-------------------------------------------------------+
+genFlags  + :ref:`ExprGenFlags <alias-ExprGenFlags>`              +
+----------+-------------------------------------------------------+
+flags     + :ref:`ExprFlags <alias-ExprFlags>`                    +
+----------+-------------------------------------------------------+
+baseType  + :ref:`rtti::Type <enum-rtti-Type>`                    +
+----------+-------------------------------------------------------+
+__rtti    +string const                                           +
+----------+-------------------------------------------------------+
+printFlags+ :ref:`ExprPrintFlags <alias-ExprPrintFlags>`          +
+----------+-------------------------------------------------------+


|structure_annotation-ast-ExprConstInt16|

.. _handle-ast-ExprConstInt64:

.. das:attribute:: ExprConstInt64

ExprConstInt64 fields are

+----------+-------------------------------------------------------+
+at        + :ref:`rtti::LineInfo <handle-rtti-LineInfo>`          +
+----------+-------------------------------------------------------+
+_type     +smart_ptr< :ref:`ast::TypeDecl <handle-ast-TypeDecl>` >+
+----------+-------------------------------------------------------+
+value     +int64                                                  +
+----------+-------------------------------------------------------+
+genFlags  + :ref:`ExprGenFlags <alias-ExprGenFlags>`              +
+----------+-------------------------------------------------------+
+flags     + :ref:`ExprFlags <alias-ExprFlags>`                    +
+----------+-------------------------------------------------------+
+baseType  + :ref:`rtti::Type <enum-rtti-Type>`                    +
+----------+-------------------------------------------------------+
+__rtti    +string const                                           +
+----------+-------------------------------------------------------+
+printFlags+ :ref:`ExprPrintFlags <alias-ExprPrintFlags>`          +
+----------+-------------------------------------------------------+


|structure_annotation-ast-ExprConstInt64|

.. _handle-ast-ExprConstInt:

.. das:attribute:: ExprConstInt

ExprConstInt fields are

+----------+-------------------------------------------------------+
+at        + :ref:`rtti::LineInfo <handle-rtti-LineInfo>`          +
+----------+-------------------------------------------------------+
+_type     +smart_ptr< :ref:`ast::TypeDecl <handle-ast-TypeDecl>` >+
+----------+-------------------------------------------------------+
+value     +int                                                    +
+----------+-------------------------------------------------------+
+genFlags  + :ref:`ExprGenFlags <alias-ExprGenFlags>`              +
+----------+-------------------------------------------------------+
+flags     + :ref:`ExprFlags <alias-ExprFlags>`                    +
+----------+-------------------------------------------------------+
+baseType  + :ref:`rtti::Type <enum-rtti-Type>`                    +
+----------+-------------------------------------------------------+
+__rtti    +string const                                           +
+----------+-------------------------------------------------------+
+printFlags+ :ref:`ExprPrintFlags <alias-ExprPrintFlags>`          +
+----------+-------------------------------------------------------+


|structure_annotation-ast-ExprConstInt|

.. _handle-ast-ExprConstInt2:

.. das:attribute:: ExprConstInt2

ExprConstInt2 fields are

+----------+-------------------------------------------------------+
+at        + :ref:`rtti::LineInfo <handle-rtti-LineInfo>`          +
+----------+-------------------------------------------------------+
+_type     +smart_ptr< :ref:`ast::TypeDecl <handle-ast-TypeDecl>` >+
+----------+-------------------------------------------------------+
+value     +int2                                                   +
+----------+-------------------------------------------------------+
+genFlags  + :ref:`ExprGenFlags <alias-ExprGenFlags>`              +
+----------+-------------------------------------------------------+
+flags     + :ref:`ExprFlags <alias-ExprFlags>`                    +
+----------+-------------------------------------------------------+
+baseType  + :ref:`rtti::Type <enum-rtti-Type>`                    +
+----------+-------------------------------------------------------+
+__rtti    +string const                                           +
+----------+-------------------------------------------------------+
+printFlags+ :ref:`ExprPrintFlags <alias-ExprPrintFlags>`          +
+----------+-------------------------------------------------------+


|structure_annotation-ast-ExprConstInt2|

.. _handle-ast-ExprConstInt3:

.. das:attribute:: ExprConstInt3

ExprConstInt3 fields are

+----------+-------------------------------------------------------+
+at        + :ref:`rtti::LineInfo <handle-rtti-LineInfo>`          +
+----------+-------------------------------------------------------+
+_type     +smart_ptr< :ref:`ast::TypeDecl <handle-ast-TypeDecl>` >+
+----------+-------------------------------------------------------+
+value     +int3                                                   +
+----------+-------------------------------------------------------+
+genFlags  + :ref:`ExprGenFlags <alias-ExprGenFlags>`              +
+----------+-------------------------------------------------------+
+flags     + :ref:`ExprFlags <alias-ExprFlags>`                    +
+----------+-------------------------------------------------------+
+baseType  + :ref:`rtti::Type <enum-rtti-Type>`                    +
+----------+-------------------------------------------------------+
+__rtti    +string const                                           +
+----------+-------------------------------------------------------+
+printFlags+ :ref:`ExprPrintFlags <alias-ExprPrintFlags>`          +
+----------+-------------------------------------------------------+


|structure_annotation-ast-ExprConstInt3|

.. _handle-ast-ExprConstInt4:

.. das:attribute:: ExprConstInt4

ExprConstInt4 fields are

+----------+-------------------------------------------------------+
+at        + :ref:`rtti::LineInfo <handle-rtti-LineInfo>`          +
+----------+-------------------------------------------------------+
+_type     +smart_ptr< :ref:`ast::TypeDecl <handle-ast-TypeDecl>` >+
+----------+-------------------------------------------------------+
+value     +int4                                                   +
+----------+-------------------------------------------------------+
+genFlags  + :ref:`ExprGenFlags <alias-ExprGenFlags>`              +
+----------+-------------------------------------------------------+
+flags     + :ref:`ExprFlags <alias-ExprFlags>`                    +
+----------+-------------------------------------------------------+
+baseType  + :ref:`rtti::Type <enum-rtti-Type>`                    +
+----------+-------------------------------------------------------+
+__rtti    +string const                                           +
+----------+-------------------------------------------------------+
+printFlags+ :ref:`ExprPrintFlags <alias-ExprPrintFlags>`          +
+----------+-------------------------------------------------------+


|structure_annotation-ast-ExprConstInt4|

.. _handle-ast-ExprConstUInt8:

.. das:attribute:: ExprConstUInt8

ExprConstUInt8 fields are

+----------+-------------------------------------------------------+
+at        + :ref:`rtti::LineInfo <handle-rtti-LineInfo>`          +
+----------+-------------------------------------------------------+
+_type     +smart_ptr< :ref:`ast::TypeDecl <handle-ast-TypeDecl>` >+
+----------+-------------------------------------------------------+
+value     +uint8                                                  +
+----------+-------------------------------------------------------+
+genFlags  + :ref:`ExprGenFlags <alias-ExprGenFlags>`              +
+----------+-------------------------------------------------------+
+flags     + :ref:`ExprFlags <alias-ExprFlags>`                    +
+----------+-------------------------------------------------------+
+baseType  + :ref:`rtti::Type <enum-rtti-Type>`                    +
+----------+-------------------------------------------------------+
+__rtti    +string const                                           +
+----------+-------------------------------------------------------+
+printFlags+ :ref:`ExprPrintFlags <alias-ExprPrintFlags>`          +
+----------+-------------------------------------------------------+


|structure_annotation-ast-ExprConstUInt8|

.. _handle-ast-ExprConstUInt16:

.. das:attribute:: ExprConstUInt16

ExprConstUInt16 fields are

+----------+-------------------------------------------------------+
+at        + :ref:`rtti::LineInfo <handle-rtti-LineInfo>`          +
+----------+-------------------------------------------------------+
+_type     +smart_ptr< :ref:`ast::TypeDecl <handle-ast-TypeDecl>` >+
+----------+-------------------------------------------------------+
+value     +uint16                                                 +
+----------+-------------------------------------------------------+
+genFlags  + :ref:`ExprGenFlags <alias-ExprGenFlags>`              +
+----------+-------------------------------------------------------+
+flags     + :ref:`ExprFlags <alias-ExprFlags>`                    +
+----------+-------------------------------------------------------+
+baseType  + :ref:`rtti::Type <enum-rtti-Type>`                    +
+----------+-------------------------------------------------------+
+__rtti    +string const                                           +
+----------+-------------------------------------------------------+
+printFlags+ :ref:`ExprPrintFlags <alias-ExprPrintFlags>`          +
+----------+-------------------------------------------------------+


|structure_annotation-ast-ExprConstUInt16|

.. _handle-ast-ExprConstUInt64:

.. das:attribute:: ExprConstUInt64

ExprConstUInt64 fields are

+----------+-------------------------------------------------------+
+at        + :ref:`rtti::LineInfo <handle-rtti-LineInfo>`          +
+----------+-------------------------------------------------------+
+_type     +smart_ptr< :ref:`ast::TypeDecl <handle-ast-TypeDecl>` >+
+----------+-------------------------------------------------------+
+value     +uint64                                                 +
+----------+-------------------------------------------------------+
+genFlags  + :ref:`ExprGenFlags <alias-ExprGenFlags>`              +
+----------+-------------------------------------------------------+
+flags     + :ref:`ExprFlags <alias-ExprFlags>`                    +
+----------+-------------------------------------------------------+
+baseType  + :ref:`rtti::Type <enum-rtti-Type>`                    +
+----------+-------------------------------------------------------+
+__rtti    +string const                                           +
+----------+-------------------------------------------------------+
+printFlags+ :ref:`ExprPrintFlags <alias-ExprPrintFlags>`          +
+----------+-------------------------------------------------------+


|structure_annotation-ast-ExprConstUInt64|

.. _handle-ast-ExprConstUInt:

.. das:attribute:: ExprConstUInt

ExprConstUInt fields are

+----------+-------------------------------------------------------+
+at        + :ref:`rtti::LineInfo <handle-rtti-LineInfo>`          +
+----------+-------------------------------------------------------+
+_type     +smart_ptr< :ref:`ast::TypeDecl <handle-ast-TypeDecl>` >+
+----------+-------------------------------------------------------+
+value     +uint                                                   +
+----------+-------------------------------------------------------+
+genFlags  + :ref:`ExprGenFlags <alias-ExprGenFlags>`              +
+----------+-------------------------------------------------------+
+flags     + :ref:`ExprFlags <alias-ExprFlags>`                    +
+----------+-------------------------------------------------------+
+baseType  + :ref:`rtti::Type <enum-rtti-Type>`                    +
+----------+-------------------------------------------------------+
+__rtti    +string const                                           +
+----------+-------------------------------------------------------+
+printFlags+ :ref:`ExprPrintFlags <alias-ExprPrintFlags>`          +
+----------+-------------------------------------------------------+


|structure_annotation-ast-ExprConstUInt|

.. _handle-ast-ExprConstUInt2:

.. das:attribute:: ExprConstUInt2

ExprConstUInt2 fields are

+----------+-------------------------------------------------------+
+at        + :ref:`rtti::LineInfo <handle-rtti-LineInfo>`          +
+----------+-------------------------------------------------------+
+_type     +smart_ptr< :ref:`ast::TypeDecl <handle-ast-TypeDecl>` >+
+----------+-------------------------------------------------------+
+value     +uint2                                                  +
+----------+-------------------------------------------------------+
+genFlags  + :ref:`ExprGenFlags <alias-ExprGenFlags>`              +
+----------+-------------------------------------------------------+
+flags     + :ref:`ExprFlags <alias-ExprFlags>`                    +
+----------+-------------------------------------------------------+
+baseType  + :ref:`rtti::Type <enum-rtti-Type>`                    +
+----------+-------------------------------------------------------+
+__rtti    +string const                                           +
+----------+-------------------------------------------------------+
+printFlags+ :ref:`ExprPrintFlags <alias-ExprPrintFlags>`          +
+----------+-------------------------------------------------------+


|structure_annotation-ast-ExprConstUInt2|

.. _handle-ast-ExprConstUInt3:

.. das:attribute:: ExprConstUInt3

ExprConstUInt3 fields are

+----------+-------------------------------------------------------+
+at        + :ref:`rtti::LineInfo <handle-rtti-LineInfo>`          +
+----------+-------------------------------------------------------+
+_type     +smart_ptr< :ref:`ast::TypeDecl <handle-ast-TypeDecl>` >+
+----------+-------------------------------------------------------+
+value     +uint3                                                  +
+----------+-------------------------------------------------------+
+genFlags  + :ref:`ExprGenFlags <alias-ExprGenFlags>`              +
+----------+-------------------------------------------------------+
+flags     + :ref:`ExprFlags <alias-ExprFlags>`                    +
+----------+-------------------------------------------------------+
+baseType  + :ref:`rtti::Type <enum-rtti-Type>`                    +
+----------+-------------------------------------------------------+
+__rtti    +string const                                           +
+----------+-------------------------------------------------------+
+printFlags+ :ref:`ExprPrintFlags <alias-ExprPrintFlags>`          +
+----------+-------------------------------------------------------+


|structure_annotation-ast-ExprConstUInt3|

.. _handle-ast-ExprConstUInt4:

.. das:attribute:: ExprConstUInt4

ExprConstUInt4 fields are

+----------+-------------------------------------------------------+
+at        + :ref:`rtti::LineInfo <handle-rtti-LineInfo>`          +
+----------+-------------------------------------------------------+
+_type     +smart_ptr< :ref:`ast::TypeDecl <handle-ast-TypeDecl>` >+
+----------+-------------------------------------------------------+
+value     +uint4                                                  +
+----------+-------------------------------------------------------+
+genFlags  + :ref:`ExprGenFlags <alias-ExprGenFlags>`              +
+----------+-------------------------------------------------------+
+flags     + :ref:`ExprFlags <alias-ExprFlags>`                    +
+----------+-------------------------------------------------------+
+baseType  + :ref:`rtti::Type <enum-rtti-Type>`                    +
+----------+-------------------------------------------------------+
+__rtti    +string const                                           +
+----------+-------------------------------------------------------+
+printFlags+ :ref:`ExprPrintFlags <alias-ExprPrintFlags>`          +
+----------+-------------------------------------------------------+


|structure_annotation-ast-ExprConstUInt4|

.. _handle-ast-ExprConstRange:

.. das:attribute:: ExprConstRange

ExprConstRange fields are

+----------+-------------------------------------------------------+
+at        + :ref:`rtti::LineInfo <handle-rtti-LineInfo>`          +
+----------+-------------------------------------------------------+
+_type     +smart_ptr< :ref:`ast::TypeDecl <handle-ast-TypeDecl>` >+
+----------+-------------------------------------------------------+
+value     +range                                                  +
+----------+-------------------------------------------------------+
+genFlags  + :ref:`ExprGenFlags <alias-ExprGenFlags>`              +
+----------+-------------------------------------------------------+
+flags     + :ref:`ExprFlags <alias-ExprFlags>`                    +
+----------+-------------------------------------------------------+
+baseType  + :ref:`rtti::Type <enum-rtti-Type>`                    +
+----------+-------------------------------------------------------+
+__rtti    +string const                                           +
+----------+-------------------------------------------------------+
+printFlags+ :ref:`ExprPrintFlags <alias-ExprPrintFlags>`          +
+----------+-------------------------------------------------------+


|structure_annotation-ast-ExprConstRange|

.. _handle-ast-ExprConstURange:

.. das:attribute:: ExprConstURange

ExprConstURange fields are

+----------+-------------------------------------------------------+
+at        + :ref:`rtti::LineInfo <handle-rtti-LineInfo>`          +
+----------+-------------------------------------------------------+
+_type     +smart_ptr< :ref:`ast::TypeDecl <handle-ast-TypeDecl>` >+
+----------+-------------------------------------------------------+
+value     +urange                                                 +
+----------+-------------------------------------------------------+
+genFlags  + :ref:`ExprGenFlags <alias-ExprGenFlags>`              +
+----------+-------------------------------------------------------+
+flags     + :ref:`ExprFlags <alias-ExprFlags>`                    +
+----------+-------------------------------------------------------+
+baseType  + :ref:`rtti::Type <enum-rtti-Type>`                    +
+----------+-------------------------------------------------------+
+__rtti    +string const                                           +
+----------+-------------------------------------------------------+
+printFlags+ :ref:`ExprPrintFlags <alias-ExprPrintFlags>`          +
+----------+-------------------------------------------------------+


|structure_annotation-ast-ExprConstURange|

.. _handle-ast-ExprConstRange64:

.. das:attribute:: ExprConstRange64

ExprConstRange64 fields are

+----------+-------------------------------------------------------+
+at        + :ref:`rtti::LineInfo <handle-rtti-LineInfo>`          +
+----------+-------------------------------------------------------+
+_type     +smart_ptr< :ref:`ast::TypeDecl <handle-ast-TypeDecl>` >+
+----------+-------------------------------------------------------+
+value     +range64                                                +
+----------+-------------------------------------------------------+
+genFlags  + :ref:`ExprGenFlags <alias-ExprGenFlags>`              +
+----------+-------------------------------------------------------+
+flags     + :ref:`ExprFlags <alias-ExprFlags>`                    +
+----------+-------------------------------------------------------+
+baseType  + :ref:`rtti::Type <enum-rtti-Type>`                    +
+----------+-------------------------------------------------------+
+__rtti    +string const                                           +
+----------+-------------------------------------------------------+
+printFlags+ :ref:`ExprPrintFlags <alias-ExprPrintFlags>`          +
+----------+-------------------------------------------------------+


|structure_annotation-ast-ExprConstRange64|

.. _handle-ast-ExprConstURange64:

.. das:attribute:: ExprConstURange64

ExprConstURange64 fields are

+----------+-------------------------------------------------------+
+at        + :ref:`rtti::LineInfo <handle-rtti-LineInfo>`          +
+----------+-------------------------------------------------------+
+_type     +smart_ptr< :ref:`ast::TypeDecl <handle-ast-TypeDecl>` >+
+----------+-------------------------------------------------------+
+value     +urange64                                               +
+----------+-------------------------------------------------------+
+genFlags  + :ref:`ExprGenFlags <alias-ExprGenFlags>`              +
+----------+-------------------------------------------------------+
+flags     + :ref:`ExprFlags <alias-ExprFlags>`                    +
+----------+-------------------------------------------------------+
+baseType  + :ref:`rtti::Type <enum-rtti-Type>`                    +
+----------+-------------------------------------------------------+
+__rtti    +string const                                           +
+----------+-------------------------------------------------------+
+printFlags+ :ref:`ExprPrintFlags <alias-ExprPrintFlags>`          +
+----------+-------------------------------------------------------+


|structure_annotation-ast-ExprConstURange64|

.. _handle-ast-ExprConstFloat:

.. das:attribute:: ExprConstFloat

ExprConstFloat fields are

+----------+-------------------------------------------------------+
+at        + :ref:`rtti::LineInfo <handle-rtti-LineInfo>`          +
+----------+-------------------------------------------------------+
+_type     +smart_ptr< :ref:`ast::TypeDecl <handle-ast-TypeDecl>` >+
+----------+-------------------------------------------------------+
+value     +float                                                  +
+----------+-------------------------------------------------------+
+genFlags  + :ref:`ExprGenFlags <alias-ExprGenFlags>`              +
+----------+-------------------------------------------------------+
+flags     + :ref:`ExprFlags <alias-ExprFlags>`                    +
+----------+-------------------------------------------------------+
+baseType  + :ref:`rtti::Type <enum-rtti-Type>`                    +
+----------+-------------------------------------------------------+
+__rtti    +string const                                           +
+----------+-------------------------------------------------------+
+printFlags+ :ref:`ExprPrintFlags <alias-ExprPrintFlags>`          +
+----------+-------------------------------------------------------+


|structure_annotation-ast-ExprConstFloat|

.. _handle-ast-ExprConstFloat2:

.. das:attribute:: ExprConstFloat2

ExprConstFloat2 fields are

+----------+-------------------------------------------------------+
+at        + :ref:`rtti::LineInfo <handle-rtti-LineInfo>`          +
+----------+-------------------------------------------------------+
+_type     +smart_ptr< :ref:`ast::TypeDecl <handle-ast-TypeDecl>` >+
+----------+-------------------------------------------------------+
+value     +float2                                                 +
+----------+-------------------------------------------------------+
+genFlags  + :ref:`ExprGenFlags <alias-ExprGenFlags>`              +
+----------+-------------------------------------------------------+
+flags     + :ref:`ExprFlags <alias-ExprFlags>`                    +
+----------+-------------------------------------------------------+
+baseType  + :ref:`rtti::Type <enum-rtti-Type>`                    +
+----------+-------------------------------------------------------+
+__rtti    +string const                                           +
+----------+-------------------------------------------------------+
+printFlags+ :ref:`ExprPrintFlags <alias-ExprPrintFlags>`          +
+----------+-------------------------------------------------------+


|structure_annotation-ast-ExprConstFloat2|

.. _handle-ast-ExprConstFloat3:

.. das:attribute:: ExprConstFloat3

ExprConstFloat3 fields are

+----------+-------------------------------------------------------+
+at        + :ref:`rtti::LineInfo <handle-rtti-LineInfo>`          +
+----------+-------------------------------------------------------+
+_type     +smart_ptr< :ref:`ast::TypeDecl <handle-ast-TypeDecl>` >+
+----------+-------------------------------------------------------+
+value     +float3                                                 +
+----------+-------------------------------------------------------+
+genFlags  + :ref:`ExprGenFlags <alias-ExprGenFlags>`              +
+----------+-------------------------------------------------------+
+flags     + :ref:`ExprFlags <alias-ExprFlags>`                    +
+----------+-------------------------------------------------------+
+baseType  + :ref:`rtti::Type <enum-rtti-Type>`                    +
+----------+-------------------------------------------------------+
+__rtti    +string const                                           +
+----------+-------------------------------------------------------+
+printFlags+ :ref:`ExprPrintFlags <alias-ExprPrintFlags>`          +
+----------+-------------------------------------------------------+


|structure_annotation-ast-ExprConstFloat3|

.. _handle-ast-ExprConstFloat4:

.. das:attribute:: ExprConstFloat4

ExprConstFloat4 fields are

+----------+-------------------------------------------------------+
+at        + :ref:`rtti::LineInfo <handle-rtti-LineInfo>`          +
+----------+-------------------------------------------------------+
+_type     +smart_ptr< :ref:`ast::TypeDecl <handle-ast-TypeDecl>` >+
+----------+-------------------------------------------------------+
+value     +float4                                                 +
+----------+-------------------------------------------------------+
+genFlags  + :ref:`ExprGenFlags <alias-ExprGenFlags>`              +
+----------+-------------------------------------------------------+
+flags     + :ref:`ExprFlags <alias-ExprFlags>`                    +
+----------+-------------------------------------------------------+
+baseType  + :ref:`rtti::Type <enum-rtti-Type>`                    +
+----------+-------------------------------------------------------+
+__rtti    +string const                                           +
+----------+-------------------------------------------------------+
+printFlags+ :ref:`ExprPrintFlags <alias-ExprPrintFlags>`          +
+----------+-------------------------------------------------------+


|structure_annotation-ast-ExprConstFloat4|

.. _handle-ast-ExprConstDouble:

.. das:attribute:: ExprConstDouble

ExprConstDouble fields are

+----------+-------------------------------------------------------+
+at        + :ref:`rtti::LineInfo <handle-rtti-LineInfo>`          +
+----------+-------------------------------------------------------+
+_type     +smart_ptr< :ref:`ast::TypeDecl <handle-ast-TypeDecl>` >+
+----------+-------------------------------------------------------+
+value     +double                                                 +
+----------+-------------------------------------------------------+
+genFlags  + :ref:`ExprGenFlags <alias-ExprGenFlags>`              +
+----------+-------------------------------------------------------+
+flags     + :ref:`ExprFlags <alias-ExprFlags>`                    +
+----------+-------------------------------------------------------+
+baseType  + :ref:`rtti::Type <enum-rtti-Type>`                    +
+----------+-------------------------------------------------------+
+__rtti    +string const                                           +
+----------+-------------------------------------------------------+
+printFlags+ :ref:`ExprPrintFlags <alias-ExprPrintFlags>`          +
+----------+-------------------------------------------------------+


|structure_annotation-ast-ExprConstDouble|

.. _handle-ast-ExprConstBool:

.. das:attribute:: ExprConstBool

ExprConstBool fields are

+----------+-------------------------------------------------------+
+at        + :ref:`rtti::LineInfo <handle-rtti-LineInfo>`          +
+----------+-------------------------------------------------------+
+_type     +smart_ptr< :ref:`ast::TypeDecl <handle-ast-TypeDecl>` >+
+----------+-------------------------------------------------------+
+value     +bool                                                   +
+----------+-------------------------------------------------------+
+genFlags  + :ref:`ExprGenFlags <alias-ExprGenFlags>`              +
+----------+-------------------------------------------------------+
+flags     + :ref:`ExprFlags <alias-ExprFlags>`                    +
+----------+-------------------------------------------------------+
+baseType  + :ref:`rtti::Type <enum-rtti-Type>`                    +
+----------+-------------------------------------------------------+
+__rtti    +string const                                           +
+----------+-------------------------------------------------------+
+printFlags+ :ref:`ExprPrintFlags <alias-ExprPrintFlags>`          +
+----------+-------------------------------------------------------+


|structure_annotation-ast-ExprConstBool|

.. _handle-ast-CaptureEntry:

.. das:attribute:: CaptureEntry

CaptureEntry fields are

+----+--------------------------------------------------------+
+mode+ :ref:`ast::CaptureMode <enum-ast-CaptureMode>`         +
+----+--------------------------------------------------------+
+name+ :ref:`builtin::das_string <handle-builtin-das_string>` +
+----+--------------------------------------------------------+


|structure_annotation-ast-CaptureEntry|

.. _handle-ast-ExprMakeBlock:

.. das:attribute:: ExprMakeBlock

ExprMakeBlock fields are

+----------+-----------------------------------------------------------+
+at        + :ref:`rtti::LineInfo <handle-rtti-LineInfo>`              +
+----------+-----------------------------------------------------------+
+capture   +vector<CaptureEntry>                                       +
+----------+-----------------------------------------------------------+
+_type     +smart_ptr< :ref:`ast::TypeDecl <handle-ast-TypeDecl>` >    +
+----------+-----------------------------------------------------------+
+mmFlags   + :ref:`ExprMakeBlockFlags <alias-ExprMakeBlockFlags>`      +
+----------+-----------------------------------------------------------+
+genFlags  + :ref:`ExprGenFlags <alias-ExprGenFlags>`                  +
+----------+-----------------------------------------------------------+
+flags     + :ref:`ExprFlags <alias-ExprFlags>`                        +
+----------+-----------------------------------------------------------+
+stackTop  +uint                                                       +
+----------+-----------------------------------------------------------+
+_block    +smart_ptr< :ref:`ast::Expression <handle-ast-Expression>` >+
+----------+-----------------------------------------------------------+
+__rtti    +string const                                               +
+----------+-----------------------------------------------------------+
+printFlags+ :ref:`ExprPrintFlags <alias-ExprPrintFlags>`              +
+----------+-----------------------------------------------------------+


|structure_annotation-ast-ExprMakeBlock|

.. _handle-ast-ExprMakeGenerator:

.. das:attribute:: ExprMakeGenerator

ExprMakeGenerator fields are

+----------------------+--------------------------------------------------------+
+at                    + :ref:`rtti::LineInfo <handle-rtti-LineInfo>`           +
+----------------------+--------------------------------------------------------+
+capture               +vector<CaptureEntry>                                    +
+----------------------+--------------------------------------------------------+
+_type                 +smart_ptr< :ref:`ast::TypeDecl <handle-ast-TypeDecl>` > +
+----------------------+--------------------------------------------------------+
+genFlags              + :ref:`ExprGenFlags <alias-ExprGenFlags>`               +
+----------------------+--------------------------------------------------------+
+flags                 + :ref:`ExprFlags <alias-ExprFlags>`                     +
+----------------------+--------------------------------------------------------+
+name                  + :ref:`builtin::das_string <handle-builtin-das_string>` +
+----------------------+--------------------------------------------------------+
+atEnclosure           + :ref:`rtti::LineInfo <handle-rtti-LineInfo>`           +
+----------------------+--------------------------------------------------------+
+iterType              +smart_ptr< :ref:`ast::TypeDecl <handle-ast-TypeDecl>` > +
+----------------------+--------------------------------------------------------+
+argumentsFailedToInfer+bool                                                    +
+----------------------+--------------------------------------------------------+
+__rtti                +string const                                            +
+----------------------+--------------------------------------------------------+
+arguments             +vector<smart_ptr<Expression>>                           +
+----------------------+--------------------------------------------------------+
+printFlags            + :ref:`ExprPrintFlags <alias-ExprPrintFlags>`           +
+----------------------+--------------------------------------------------------+


|structure_annotation-ast-ExprMakeGenerator|

.. _handle-ast-ExprMemZero:

.. das:attribute:: ExprMemZero

ExprMemZero fields are

+----------------------+--------------------------------------------------------+
+at                    + :ref:`rtti::LineInfo <handle-rtti-LineInfo>`           +
+----------------------+--------------------------------------------------------+
+_type                 +smart_ptr< :ref:`ast::TypeDecl <handle-ast-TypeDecl>` > +
+----------------------+--------------------------------------------------------+
+genFlags              + :ref:`ExprGenFlags <alias-ExprGenFlags>`               +
+----------------------+--------------------------------------------------------+
+flags                 + :ref:`ExprFlags <alias-ExprFlags>`                     +
+----------------------+--------------------------------------------------------+
+name                  + :ref:`builtin::das_string <handle-builtin-das_string>` +
+----------------------+--------------------------------------------------------+
+atEnclosure           + :ref:`rtti::LineInfo <handle-rtti-LineInfo>`           +
+----------------------+--------------------------------------------------------+
+argumentsFailedToInfer+bool                                                    +
+----------------------+--------------------------------------------------------+
+__rtti                +string const                                            +
+----------------------+--------------------------------------------------------+
+arguments             +vector<smart_ptr<Expression>>                           +
+----------------------+--------------------------------------------------------+
+printFlags            + :ref:`ExprPrintFlags <alias-ExprPrintFlags>`           +
+----------------------+--------------------------------------------------------+


|structure_annotation-ast-ExprMemZero|

.. _handle-ast-ExprConstEnumeration:

.. das:attribute:: ExprConstEnumeration

ExprConstEnumeration fields are

+----------+-------------------------------------------------------------+
+enumType  +smart_ptr< :ref:`ast::Enumeration <handle-ast-Enumeration>` >+
+----------+-------------------------------------------------------------+
+at        + :ref:`rtti::LineInfo <handle-rtti-LineInfo>`                +
+----------+-------------------------------------------------------------+
+_type     +smart_ptr< :ref:`ast::TypeDecl <handle-ast-TypeDecl>` >      +
+----------+-------------------------------------------------------------+
+value     + :ref:`builtin::das_string <handle-builtin-das_string>`      +
+----------+-------------------------------------------------------------+
+genFlags  + :ref:`ExprGenFlags <alias-ExprGenFlags>`                    +
+----------+-------------------------------------------------------------+
+flags     + :ref:`ExprFlags <alias-ExprFlags>`                          +
+----------+-------------------------------------------------------------+
+baseType  + :ref:`rtti::Type <enum-rtti-Type>`                          +
+----------+-------------------------------------------------------------+
+__rtti    +string const                                                 +
+----------+-------------------------------------------------------------+
+printFlags+ :ref:`ExprPrintFlags <alias-ExprPrintFlags>`                +
+----------+-------------------------------------------------------------+


|structure_annotation-ast-ExprConstEnumeration|

.. _handle-ast-ExprConstBitfield:

.. das:attribute:: ExprConstBitfield

ExprConstBitfield fields are

+------------+-------------------------------------------------------+
+at          + :ref:`rtti::LineInfo <handle-rtti-LineInfo>`          +
+------------+-------------------------------------------------------+
+_type       +smart_ptr< :ref:`ast::TypeDecl <handle-ast-TypeDecl>` >+
+------------+-------------------------------------------------------+
+value       +bitfield<>                                             +
+------------+-------------------------------------------------------+
+genFlags    + :ref:`ExprGenFlags <alias-ExprGenFlags>`              +
+------------+-------------------------------------------------------+
+flags       + :ref:`ExprFlags <alias-ExprFlags>`                    +
+------------+-------------------------------------------------------+
+baseType    + :ref:`rtti::Type <enum-rtti-Type>`                    +
+------------+-------------------------------------------------------+
+__rtti      +string const                                           +
+------------+-------------------------------------------------------+
+printFlags  + :ref:`ExprPrintFlags <alias-ExprPrintFlags>`          +
+------------+-------------------------------------------------------+
+bitfieldType+smart_ptr< :ref:`ast::TypeDecl <handle-ast-TypeDecl>` >+
+------------+-------------------------------------------------------+


|structure_annotation-ast-ExprConstBitfield|

.. _handle-ast-ExprConstString:

.. das:attribute:: ExprConstString

ExprConstString fields are

+----------+--------------------------------------------------------+
+at        + :ref:`rtti::LineInfo <handle-rtti-LineInfo>`           +
+----------+--------------------------------------------------------+
+_type     +smart_ptr< :ref:`ast::TypeDecl <handle-ast-TypeDecl>` > +
+----------+--------------------------------------------------------+
+value     + :ref:`builtin::das_string <handle-builtin-das_string>` +
+----------+--------------------------------------------------------+
+genFlags  + :ref:`ExprGenFlags <alias-ExprGenFlags>`               +
+----------+--------------------------------------------------------+
+flags     + :ref:`ExprFlags <alias-ExprFlags>`                     +
+----------+--------------------------------------------------------+
+baseType  + :ref:`rtti::Type <enum-rtti-Type>`                     +
+----------+--------------------------------------------------------+
+__rtti    +string const                                            +
+----------+--------------------------------------------------------+
+printFlags+ :ref:`ExprPrintFlags <alias-ExprPrintFlags>`           +
+----------+--------------------------------------------------------+


|structure_annotation-ast-ExprConstString|

.. _handle-ast-ExprUnsafe:

.. das:attribute:: ExprUnsafe

ExprUnsafe fields are

+----------+-----------------------------------------------------------+
+at        + :ref:`rtti::LineInfo <handle-rtti-LineInfo>`              +
+----------+-----------------------------------------------------------+
+_type     +smart_ptr< :ref:`ast::TypeDecl <handle-ast-TypeDecl>` >    +
+----------+-----------------------------------------------------------+
+genFlags  + :ref:`ExprGenFlags <alias-ExprGenFlags>`                  +
+----------+-----------------------------------------------------------+
+flags     + :ref:`ExprFlags <alias-ExprFlags>`                        +
+----------+-----------------------------------------------------------+
+body      +smart_ptr< :ref:`ast::Expression <handle-ast-Expression>` >+
+----------+-----------------------------------------------------------+
+__rtti    +string const                                               +
+----------+-----------------------------------------------------------+
+printFlags+ :ref:`ExprPrintFlags <alias-ExprPrintFlags>`              +
+----------+-----------------------------------------------------------+


|structure_annotation-ast-ExprUnsafe|

.. _handle-ast-VisitorAdapter:

.. das:attribute:: VisitorAdapter

|structure_annotation-ast-VisitorAdapter|

.. _handle-ast-FunctionAnnotation:

.. das:attribute:: FunctionAnnotation

|structure_annotation-ast-FunctionAnnotation|

.. _handle-ast-StructureAnnotation:

.. das:attribute:: StructureAnnotation

|structure_annotation-ast-StructureAnnotation|

.. _handle-ast-EnumerationAnnotation:

.. das:attribute:: EnumerationAnnotation

|structure_annotation-ast-EnumerationAnnotation|

.. _handle-ast-PassMacro:

.. das:attribute:: PassMacro

PassMacro fields are

+----+--------------------------------------------------------+
+name+ :ref:`builtin::das_string <handle-builtin-das_string>` +
+----+--------------------------------------------------------+


|structure_annotation-ast-PassMacro|

.. _handle-ast-ReaderMacro:

.. das:attribute:: ReaderMacro

ReaderMacro fields are

+-------+--------------------------------------------------------+
+name   + :ref:`builtin::das_string <handle-builtin-das_string>` +
+-------+--------------------------------------------------------+
+_module+ :ref:`rtti::Module <handle-rtti-Module>` ?             +
+-------+--------------------------------------------------------+


|structure_annotation-ast-ReaderMacro|

.. _handle-ast-CommentReader:

.. das:attribute:: CommentReader

|structure_annotation-ast-CommentReader|

.. _handle-ast-CallMacro:

.. das:attribute:: CallMacro

CallMacro fields are

+-------+--------------------------------------------------------+
+name   + :ref:`builtin::das_string <handle-builtin-das_string>` +
+-------+--------------------------------------------------------+
+_module+ :ref:`rtti::Module <handle-rtti-Module>` ?             +
+-------+--------------------------------------------------------+


|structure_annotation-ast-CallMacro|

.. _handle-ast-VariantMacro:

.. das:attribute:: VariantMacro

VariantMacro fields are

+----+--------------------------------------------------------+
+name+ :ref:`builtin::das_string <handle-builtin-das_string>` +
+----+--------------------------------------------------------+


|structure_annotation-ast-VariantMacro|

.. _handle-ast-ForLoopMacro:

.. das:attribute:: ForLoopMacro

ForLoopMacro fields are

+----+--------------------------------------------------------+
+name+ :ref:`builtin::das_string <handle-builtin-das_string>` +
+----+--------------------------------------------------------+


|structure_annotation-ast-ForLoopMacro|

.. _handle-ast-CaptureMacro:

.. das:attribute:: CaptureMacro

CaptureMacro fields are

+----+--------------------------------------------------------+
+name+ :ref:`builtin::das_string <handle-builtin-das_string>` +
+----+--------------------------------------------------------+


|structure_annotation-ast-CaptureMacro|

.. _handle-ast-SimulateMacro:

.. das:attribute:: SimulateMacro

SimulateMacro fields are

+----+--------------------------------------------------------+
+name+ :ref:`builtin::das_string <handle-builtin-das_string>` +
+----+--------------------------------------------------------+


|structure_annotation-ast-SimulateMacro|

.. _handle-ast-ExprReader:

.. das:attribute:: ExprReader

ExprReader fields are

+----------+-------------------------------------------------------------+
+at        + :ref:`rtti::LineInfo <handle-rtti-LineInfo>`                +
+----------+-------------------------------------------------------------+
+macro     +smart_ptr< :ref:`ast::ReaderMacro <handle-ast-ReaderMacro>` >+
+----------+-------------------------------------------------------------+
+_type     +smart_ptr< :ref:`ast::TypeDecl <handle-ast-TypeDecl>` >      +
+----------+-------------------------------------------------------------+
+genFlags  + :ref:`ExprGenFlags <alias-ExprGenFlags>`                    +
+----------+-------------------------------------------------------------+
+flags     + :ref:`ExprFlags <alias-ExprFlags>`                          +
+----------+-------------------------------------------------------------+
+__rtti    +string const                                                 +
+----------+-------------------------------------------------------------+
+printFlags+ :ref:`ExprPrintFlags <alias-ExprPrintFlags>`                +
+----------+-------------------------------------------------------------+
+sequence  + :ref:`builtin::das_string <handle-builtin-das_string>`      +
+----------+-------------------------------------------------------------+


|structure_annotation-ast-ExprReader|

.. _handle-ast-ExprCallMacro:

.. das:attribute:: ExprCallMacro

ExprCallMacro fields are

+----------------------+--------------------------------------------------------+
+at                    + :ref:`rtti::LineInfo <handle-rtti-LineInfo>`           +
+----------------------+--------------------------------------------------------+
+macro                 + :ref:`ast::CallMacro <handle-ast-CallMacro>` ?         +
+----------------------+--------------------------------------------------------+
+_type                 +smart_ptr< :ref:`ast::TypeDecl <handle-ast-TypeDecl>` > +
+----------------------+--------------------------------------------------------+
+genFlags              + :ref:`ExprGenFlags <alias-ExprGenFlags>`               +
+----------------------+--------------------------------------------------------+
+flags                 + :ref:`ExprFlags <alias-ExprFlags>`                     +
+----------------------+--------------------------------------------------------+
+name                  + :ref:`builtin::das_string <handle-builtin-das_string>` +
+----------------------+--------------------------------------------------------+
+atEnclosure           + :ref:`rtti::LineInfo <handle-rtti-LineInfo>`           +
+----------------------+--------------------------------------------------------+
+argumentsFailedToInfer+bool                                                    +
+----------------------+--------------------------------------------------------+
+__rtti                +string const                                            +
+----------------------+--------------------------------------------------------+
+arguments             +vector<smart_ptr<Expression>>                           +
+----------------------+--------------------------------------------------------+
+printFlags            + :ref:`ExprPrintFlags <alias-ExprPrintFlags>`           +
+----------------------+--------------------------------------------------------+


|structure_annotation-ast-ExprCallMacro|

+++++++++++
Call macros
+++++++++++

.. _call-macro-ast-quote:

.. das:attribute:: quote

|function_annotation-ast-quote|

+++++++++++++++
Typeinfo macros
+++++++++++++++

.. _call-macro-ast-ast_typedecl:

.. das:attribute:: ast_typedecl

|typeinfo_macro-ast-ast_typedecl|

.. _call-macro-ast-ast_function:

.. das:attribute:: ast_function

|typeinfo_macro-ast-ast_function|

+++++++++++++
Handled types
+++++++++++++

.. _handle-ast-MakeStruct:

.. das:attribute:: MakeStruct

|any_annotation-ast-MakeStruct|

+++++++
Classes
+++++++

.. _struct-ast-AstFunctionAnnotation:

.. das:attribute:: AstFunctionAnnotation

|class-ast-AstFunctionAnnotation|

it defines as follows


.. das:function:: AstFunctionAnnotation.transform(self: AstFunctionAnnotation; call: smart_ptr<ast::ExprCallFunc>; errors: das_string)

transform returns  :ref:`ExpressionPtr <alias-ExpressionPtr>` 

+--------+----------------------------------------------------------------------+
+argument+argument type                                                         +
+========+======================================================================+
+self    + :ref:`ast::AstFunctionAnnotation <struct-ast-AstFunctionAnnotation>` +
+--------+----------------------------------------------------------------------+
+call    +smart_ptr< :ref:`ast::ExprCallFunc <handle-ast-ExprCallFunc>` >       +
+--------+----------------------------------------------------------------------+
+errors  + :ref:`builtin::das_string <handle-builtin-das_string>`               +
+--------+----------------------------------------------------------------------+


|method-ast-AstFunctionAnnotation.transform|

.. das:function:: AstFunctionAnnotation.verifyCall(self: AstFunctionAnnotation; call: smart_ptr<ast::ExprCallFunc>; args: AnnotationArgumentList const; progArgs: AnnotationArgumentList const; errors: das_string)

verifyCall returns bool

+--------+--------------------------------------------------------------------------------+
+argument+argument type                                                                   +
+========+================================================================================+
+self    + :ref:`ast::AstFunctionAnnotation <struct-ast-AstFunctionAnnotation>`           +
+--------+--------------------------------------------------------------------------------+
+call    +smart_ptr< :ref:`ast::ExprCallFunc <handle-ast-ExprCallFunc>` >                 +
+--------+--------------------------------------------------------------------------------+
+args    + :ref:`rtti::AnnotationArgumentList <handle-rtti-AnnotationArgumentList>`  const+
+--------+--------------------------------------------------------------------------------+
+progArgs+ :ref:`rtti::AnnotationArgumentList <handle-rtti-AnnotationArgumentList>`  const+
+--------+--------------------------------------------------------------------------------+
+errors  + :ref:`builtin::das_string <handle-builtin-das_string>`                         +
+--------+--------------------------------------------------------------------------------+


|method-ast-AstFunctionAnnotation.verifyCall|

.. das:function:: AstFunctionAnnotation.apply(self: AstFunctionAnnotation; func: FunctionPtr; group: ModuleGroup; args: AnnotationArgumentList const; errors: das_string)

apply returns bool

+--------+--------------------------------------------------------------------------------+
+argument+argument type                                                                   +
+========+================================================================================+
+self    + :ref:`ast::AstFunctionAnnotation <struct-ast-AstFunctionAnnotation>`           +
+--------+--------------------------------------------------------------------------------+
+func    + :ref:`FunctionPtr <alias-FunctionPtr>`                                         +
+--------+--------------------------------------------------------------------------------+
+group   + :ref:`rtti::ModuleGroup <handle-rtti-ModuleGroup>`                             +
+--------+--------------------------------------------------------------------------------+
+args    + :ref:`rtti::AnnotationArgumentList <handle-rtti-AnnotationArgumentList>`  const+
+--------+--------------------------------------------------------------------------------+
+errors  + :ref:`builtin::das_string <handle-builtin-das_string>`                         +
+--------+--------------------------------------------------------------------------------+


|method-ast-AstFunctionAnnotation.apply|

.. das:function:: AstFunctionAnnotation.generic_apply(self: AstFunctionAnnotation; func: FunctionPtr; group: ModuleGroup; args: AnnotationArgumentList const; errors: das_string)

generic_apply returns bool

+--------+--------------------------------------------------------------------------------+
+argument+argument type                                                                   +
+========+================================================================================+
+self    + :ref:`ast::AstFunctionAnnotation <struct-ast-AstFunctionAnnotation>`           +
+--------+--------------------------------------------------------------------------------+
+func    + :ref:`FunctionPtr <alias-FunctionPtr>`                                         +
+--------+--------------------------------------------------------------------------------+
+group   + :ref:`rtti::ModuleGroup <handle-rtti-ModuleGroup>`                             +
+--------+--------------------------------------------------------------------------------+
+args    + :ref:`rtti::AnnotationArgumentList <handle-rtti-AnnotationArgumentList>`  const+
+--------+--------------------------------------------------------------------------------+
+errors  + :ref:`builtin::das_string <handle-builtin-das_string>`                         +
+--------+--------------------------------------------------------------------------------+


|method-ast-AstFunctionAnnotation.generic_apply|

.. das:function:: AstFunctionAnnotation.finish(self: AstFunctionAnnotation; func: FunctionPtr; group: ModuleGroup; args: AnnotationArgumentList const; progArgs: AnnotationArgumentList const; errors: das_string)

finish returns bool

+--------+--------------------------------------------------------------------------------+
+argument+argument type                                                                   +
+========+================================================================================+
+self    + :ref:`ast::AstFunctionAnnotation <struct-ast-AstFunctionAnnotation>`           +
+--------+--------------------------------------------------------------------------------+
+func    + :ref:`FunctionPtr <alias-FunctionPtr>`                                         +
+--------+--------------------------------------------------------------------------------+
+group   + :ref:`rtti::ModuleGroup <handle-rtti-ModuleGroup>`                             +
+--------+--------------------------------------------------------------------------------+
+args    + :ref:`rtti::AnnotationArgumentList <handle-rtti-AnnotationArgumentList>`  const+
+--------+--------------------------------------------------------------------------------+
+progArgs+ :ref:`rtti::AnnotationArgumentList <handle-rtti-AnnotationArgumentList>`  const+
+--------+--------------------------------------------------------------------------------+
+errors  + :ref:`builtin::das_string <handle-builtin-das_string>`                         +
+--------+--------------------------------------------------------------------------------+


|method-ast-AstFunctionAnnotation.finish|

.. das:function:: AstFunctionAnnotation.patch(self: AstFunctionAnnotation; func: FunctionPtr; group: ModuleGroup; args: AnnotationArgumentList const; progArgs: AnnotationArgumentList const; errors: das_string; astChanged: bool&)

patch returns bool

+----------+--------------------------------------------------------------------------------+
+argument  +argument type                                                                   +
+==========+================================================================================+
+self      + :ref:`ast::AstFunctionAnnotation <struct-ast-AstFunctionAnnotation>`           +
+----------+--------------------------------------------------------------------------------+
+func      + :ref:`FunctionPtr <alias-FunctionPtr>`                                         +
+----------+--------------------------------------------------------------------------------+
+group     + :ref:`rtti::ModuleGroup <handle-rtti-ModuleGroup>`                             +
+----------+--------------------------------------------------------------------------------+
+args      + :ref:`rtti::AnnotationArgumentList <handle-rtti-AnnotationArgumentList>`  const+
+----------+--------------------------------------------------------------------------------+
+progArgs  + :ref:`rtti::AnnotationArgumentList <handle-rtti-AnnotationArgumentList>`  const+
+----------+--------------------------------------------------------------------------------+
+errors    + :ref:`builtin::das_string <handle-builtin-das_string>`                         +
+----------+--------------------------------------------------------------------------------+
+astChanged+bool&                                                                           +
+----------+--------------------------------------------------------------------------------+


|method-ast-AstFunctionAnnotation.patch|

.. das:function:: AstFunctionAnnotation.fixup(self: AstFunctionAnnotation; func: FunctionPtr; group: ModuleGroup; args: AnnotationArgumentList const; progArgs: AnnotationArgumentList const; errors: das_string)

fixup returns bool

+--------+--------------------------------------------------------------------------------+
+argument+argument type                                                                   +
+========+================================================================================+
+self    + :ref:`ast::AstFunctionAnnotation <struct-ast-AstFunctionAnnotation>`           +
+--------+--------------------------------------------------------------------------------+
+func    + :ref:`FunctionPtr <alias-FunctionPtr>`                                         +
+--------+--------------------------------------------------------------------------------+
+group   + :ref:`rtti::ModuleGroup <handle-rtti-ModuleGroup>`                             +
+--------+--------------------------------------------------------------------------------+
+args    + :ref:`rtti::AnnotationArgumentList <handle-rtti-AnnotationArgumentList>`  const+
+--------+--------------------------------------------------------------------------------+
+progArgs+ :ref:`rtti::AnnotationArgumentList <handle-rtti-AnnotationArgumentList>`  const+
+--------+--------------------------------------------------------------------------------+
+errors  + :ref:`builtin::das_string <handle-builtin-das_string>`                         +
+--------+--------------------------------------------------------------------------------+


|method-ast-AstFunctionAnnotation.fixup|

.. das:function:: AstFunctionAnnotation.lint(self: AstFunctionAnnotation; func: FunctionPtr; group: ModuleGroup; args: AnnotationArgumentList const; progArgs: AnnotationArgumentList const; errors: das_string)

lint returns bool

+--------+--------------------------------------------------------------------------------+
+argument+argument type                                                                   +
+========+================================================================================+
+self    + :ref:`ast::AstFunctionAnnotation <struct-ast-AstFunctionAnnotation>`           +
+--------+--------------------------------------------------------------------------------+
+func    + :ref:`FunctionPtr <alias-FunctionPtr>`                                         +
+--------+--------------------------------------------------------------------------------+
+group   + :ref:`rtti::ModuleGroup <handle-rtti-ModuleGroup>`                             +
+--------+--------------------------------------------------------------------------------+
+args    + :ref:`rtti::AnnotationArgumentList <handle-rtti-AnnotationArgumentList>`  const+
+--------+--------------------------------------------------------------------------------+
+progArgs+ :ref:`rtti::AnnotationArgumentList <handle-rtti-AnnotationArgumentList>`  const+
+--------+--------------------------------------------------------------------------------+
+errors  + :ref:`builtin::das_string <handle-builtin-das_string>`                         +
+--------+--------------------------------------------------------------------------------+


|method-ast-AstFunctionAnnotation.lint|

.. das:function:: AstFunctionAnnotation.complete(self: AstFunctionAnnotation; func: FunctionPtr; ctx: smart_ptr<rtti::Context>)

+--------+----------------------------------------------------------------------+
+argument+argument type                                                         +
+========+======================================================================+
+self    + :ref:`ast::AstFunctionAnnotation <struct-ast-AstFunctionAnnotation>` +
+--------+----------------------------------------------------------------------+
+func    + :ref:`FunctionPtr <alias-FunctionPtr>`                               +
+--------+----------------------------------------------------------------------+
+ctx     +smart_ptr< :ref:`rtti::Context <handle-rtti-Context>` >               +
+--------+----------------------------------------------------------------------+


|method-ast-AstFunctionAnnotation.complete|

.. das:function:: AstFunctionAnnotation.isCompatible(self: AstFunctionAnnotation; func: FunctionPtr; types: VectorTypeDeclPtr; decl: AnnotationDeclaration const; errors: das_string)

isCompatible returns bool

+--------+------------------------------------------------------------------------------+
+argument+argument type                                                                 +
+========+==============================================================================+
+self    + :ref:`ast::AstFunctionAnnotation <struct-ast-AstFunctionAnnotation>`         +
+--------+------------------------------------------------------------------------------+
+func    + :ref:`FunctionPtr <alias-FunctionPtr>`                                       +
+--------+------------------------------------------------------------------------------+
+types   + :ref:`VectorTypeDeclPtr <alias-VectorTypeDeclPtr>`                           +
+--------+------------------------------------------------------------------------------+
+decl    + :ref:`rtti::AnnotationDeclaration <handle-rtti-AnnotationDeclaration>`  const+
+--------+------------------------------------------------------------------------------+
+errors  + :ref:`builtin::das_string <handle-builtin-das_string>`                       +
+--------+------------------------------------------------------------------------------+


|method-ast-AstFunctionAnnotation.isCompatible|

.. das:function:: AstFunctionAnnotation.isSpecialized(self: AstFunctionAnnotation)

isSpecialized returns bool

|method-ast-AstFunctionAnnotation.isSpecialized|

.. das:function:: AstFunctionAnnotation.appendToMangledName(self: AstFunctionAnnotation; func: FunctionPtr; decl: AnnotationDeclaration const; mangledName: das_string)

+-----------+------------------------------------------------------------------------------+
+argument   +argument type                                                                 +
+===========+==============================================================================+
+self       + :ref:`ast::AstFunctionAnnotation <struct-ast-AstFunctionAnnotation>`         +
+-----------+------------------------------------------------------------------------------+
+func       + :ref:`FunctionPtr <alias-FunctionPtr>`                                       +
+-----------+------------------------------------------------------------------------------+
+decl       + :ref:`rtti::AnnotationDeclaration <handle-rtti-AnnotationDeclaration>`  const+
+-----------+------------------------------------------------------------------------------+
+mangledName+ :ref:`builtin::das_string <handle-builtin-das_string>`                       +
+-----------+------------------------------------------------------------------------------+


|method-ast-AstFunctionAnnotation.appendToMangledName|

.. _struct-ast-AstBlockAnnotation:

.. das:attribute:: AstBlockAnnotation

|class-ast-AstBlockAnnotation|

it defines as follows


.. das:function:: AstBlockAnnotation.apply(self: AstBlockAnnotation; blk: smart_ptr<ast::ExprBlock>; group: ModuleGroup; args: AnnotationArgumentList const; errors: das_string)

apply returns bool

+--------+--------------------------------------------------------------------------------+
+argument+argument type                                                                   +
+========+================================================================================+
+self    + :ref:`ast::AstBlockAnnotation <struct-ast-AstBlockAnnotation>`                 +
+--------+--------------------------------------------------------------------------------+
+blk     +smart_ptr< :ref:`ast::ExprBlock <handle-ast-ExprBlock>` >                       +
+--------+--------------------------------------------------------------------------------+
+group   + :ref:`rtti::ModuleGroup <handle-rtti-ModuleGroup>`                             +
+--------+--------------------------------------------------------------------------------+
+args    + :ref:`rtti::AnnotationArgumentList <handle-rtti-AnnotationArgumentList>`  const+
+--------+--------------------------------------------------------------------------------+
+errors  + :ref:`builtin::das_string <handle-builtin-das_string>`                         +
+--------+--------------------------------------------------------------------------------+


|method-ast-AstBlockAnnotation.apply|

.. das:function:: AstBlockAnnotation.finish(self: AstBlockAnnotation; blk: smart_ptr<ast::ExprBlock>; group: ModuleGroup; args: AnnotationArgumentList const; progArgs: AnnotationArgumentList const; errors: das_string)

finish returns bool

+--------+--------------------------------------------------------------------------------+
+argument+argument type                                                                   +
+========+================================================================================+
+self    + :ref:`ast::AstBlockAnnotation <struct-ast-AstBlockAnnotation>`                 +
+--------+--------------------------------------------------------------------------------+
+blk     +smart_ptr< :ref:`ast::ExprBlock <handle-ast-ExprBlock>` >                       +
+--------+--------------------------------------------------------------------------------+
+group   + :ref:`rtti::ModuleGroup <handle-rtti-ModuleGroup>`                             +
+--------+--------------------------------------------------------------------------------+
+args    + :ref:`rtti::AnnotationArgumentList <handle-rtti-AnnotationArgumentList>`  const+
+--------+--------------------------------------------------------------------------------+
+progArgs+ :ref:`rtti::AnnotationArgumentList <handle-rtti-AnnotationArgumentList>`  const+
+--------+--------------------------------------------------------------------------------+
+errors  + :ref:`builtin::das_string <handle-builtin-das_string>`                         +
+--------+--------------------------------------------------------------------------------+


|method-ast-AstBlockAnnotation.finish|

.. _struct-ast-AstStructureAnnotation:

.. das:attribute:: AstStructureAnnotation

|class-ast-AstStructureAnnotation|

it defines as follows


.. das:function:: AstStructureAnnotation.apply(self: AstStructureAnnotation; st: StructurePtr; group: ModuleGroup; args: AnnotationArgumentList const; errors: das_string)

apply returns bool

+--------+--------------------------------------------------------------------------------+
+argument+argument type                                                                   +
+========+================================================================================+
+self    + :ref:`ast::AstStructureAnnotation <struct-ast-AstStructureAnnotation>`         +
+--------+--------------------------------------------------------------------------------+
+st      + :ref:`StructurePtr <alias-StructurePtr>`                                       +
+--------+--------------------------------------------------------------------------------+
+group   + :ref:`rtti::ModuleGroup <handle-rtti-ModuleGroup>`                             +
+--------+--------------------------------------------------------------------------------+
+args    + :ref:`rtti::AnnotationArgumentList <handle-rtti-AnnotationArgumentList>`  const+
+--------+--------------------------------------------------------------------------------+
+errors  + :ref:`builtin::das_string <handle-builtin-das_string>`                         +
+--------+--------------------------------------------------------------------------------+


|method-ast-AstStructureAnnotation.apply|

.. das:function:: AstStructureAnnotation.finish(self: AstStructureAnnotation; st: StructurePtr; group: ModuleGroup; args: AnnotationArgumentList const; errors: das_string)

finish returns bool

+--------+--------------------------------------------------------------------------------+
+argument+argument type                                                                   +
+========+================================================================================+
+self    + :ref:`ast::AstStructureAnnotation <struct-ast-AstStructureAnnotation>`         +
+--------+--------------------------------------------------------------------------------+
+st      + :ref:`StructurePtr <alias-StructurePtr>`                                       +
+--------+--------------------------------------------------------------------------------+
+group   + :ref:`rtti::ModuleGroup <handle-rtti-ModuleGroup>`                             +
+--------+--------------------------------------------------------------------------------+
+args    + :ref:`rtti::AnnotationArgumentList <handle-rtti-AnnotationArgumentList>`  const+
+--------+--------------------------------------------------------------------------------+
+errors  + :ref:`builtin::das_string <handle-builtin-das_string>`                         +
+--------+--------------------------------------------------------------------------------+


|method-ast-AstStructureAnnotation.finish|

.. das:function:: AstStructureAnnotation.patch(self: AstStructureAnnotation; st: StructurePtr; group: ModuleGroup; args: AnnotationArgumentList const; errors: das_string; astChanged: bool&)

patch returns bool

+----------+--------------------------------------------------------------------------------+
+argument  +argument type                                                                   +
+==========+================================================================================+
+self      + :ref:`ast::AstStructureAnnotation <struct-ast-AstStructureAnnotation>`         +
+----------+--------------------------------------------------------------------------------+
+st        + :ref:`StructurePtr <alias-StructurePtr>`                                       +
+----------+--------------------------------------------------------------------------------+
+group     + :ref:`rtti::ModuleGroup <handle-rtti-ModuleGroup>`                             +
+----------+--------------------------------------------------------------------------------+
+args      + :ref:`rtti::AnnotationArgumentList <handle-rtti-AnnotationArgumentList>`  const+
+----------+--------------------------------------------------------------------------------+
+errors    + :ref:`builtin::das_string <handle-builtin-das_string>`                         +
+----------+--------------------------------------------------------------------------------+
+astChanged+bool&                                                                           +
+----------+--------------------------------------------------------------------------------+


|method-ast-AstStructureAnnotation.patch|

.. das:function:: AstStructureAnnotation.complete(self: AstStructureAnnotation; st: StructurePtr; ctx: smart_ptr<rtti::Context>)

+--------+------------------------------------------------------------------------+
+argument+argument type                                                           +
+========+========================================================================+
+self    + :ref:`ast::AstStructureAnnotation <struct-ast-AstStructureAnnotation>` +
+--------+------------------------------------------------------------------------+
+st      + :ref:`StructurePtr <alias-StructurePtr>`                               +
+--------+------------------------------------------------------------------------+
+ctx     +smart_ptr< :ref:`rtti::Context <handle-rtti-Context>` >                 +
+--------+------------------------------------------------------------------------+


|method-ast-AstStructureAnnotation.complete|

.. das:function:: AstStructureAnnotation.aotPrefix(self: AstStructureAnnotation; st: StructurePtr; args: AnnotationArgumentList const; writer: StringBuilderWriter)

+--------+--------------------------------------------------------------------------------+
+argument+argument type                                                                   +
+========+================================================================================+
+self    + :ref:`ast::AstStructureAnnotation <struct-ast-AstStructureAnnotation>`         +
+--------+--------------------------------------------------------------------------------+
+st      + :ref:`StructurePtr <alias-StructurePtr>`                                       +
+--------+--------------------------------------------------------------------------------+
+args    + :ref:`rtti::AnnotationArgumentList <handle-rtti-AnnotationArgumentList>`  const+
+--------+--------------------------------------------------------------------------------+
+writer  + :ref:`strings::StringBuilderWriter <handle-strings-StringBuilderWriter>`       +
+--------+--------------------------------------------------------------------------------+


|method-ast-AstStructureAnnotation.aotPrefix|

.. das:function:: AstStructureAnnotation.aotBody(self: AstStructureAnnotation; st: StructurePtr; args: AnnotationArgumentList const; writer: StringBuilderWriter)

+--------+--------------------------------------------------------------------------------+
+argument+argument type                                                                   +
+========+================================================================================+
+self    + :ref:`ast::AstStructureAnnotation <struct-ast-AstStructureAnnotation>`         +
+--------+--------------------------------------------------------------------------------+
+st      + :ref:`StructurePtr <alias-StructurePtr>`                                       +
+--------+--------------------------------------------------------------------------------+
+args    + :ref:`rtti::AnnotationArgumentList <handle-rtti-AnnotationArgumentList>`  const+
+--------+--------------------------------------------------------------------------------+
+writer  + :ref:`strings::StringBuilderWriter <handle-strings-StringBuilderWriter>`       +
+--------+--------------------------------------------------------------------------------+


|method-ast-AstStructureAnnotation.aotBody|

.. das:function:: AstStructureAnnotation.aotSuffix(self: AstStructureAnnotation; st: StructurePtr; args: AnnotationArgumentList const; writer: StringBuilderWriter)

+--------+--------------------------------------------------------------------------------+
+argument+argument type                                                                   +
+========+================================================================================+
+self    + :ref:`ast::AstStructureAnnotation <struct-ast-AstStructureAnnotation>`         +
+--------+--------------------------------------------------------------------------------+
+st      + :ref:`StructurePtr <alias-StructurePtr>`                                       +
+--------+--------------------------------------------------------------------------------+
+args    + :ref:`rtti::AnnotationArgumentList <handle-rtti-AnnotationArgumentList>`  const+
+--------+--------------------------------------------------------------------------------+
+writer  + :ref:`strings::StringBuilderWriter <handle-strings-StringBuilderWriter>`       +
+--------+--------------------------------------------------------------------------------+


|method-ast-AstStructureAnnotation.aotSuffix|

.. _struct-ast-AstPassMacro:

.. das:attribute:: AstPassMacro

|class-ast-AstPassMacro|

it defines as follows


.. das:function:: AstPassMacro.apply(self: AstPassMacro; prog: ProgramPtr; mod: rtti::Module? const)

apply returns bool

+--------+----------------------------------------------------+
+argument+argument type                                       +
+========+====================================================+
+self    + :ref:`ast::AstPassMacro <struct-ast-AstPassMacro>` +
+--------+----------------------------------------------------+
+prog    + :ref:`ProgramPtr <alias-ProgramPtr>`               +
+--------+----------------------------------------------------+
+mod     + :ref:`rtti::Module <handle-rtti-Module>` ? const   +
+--------+----------------------------------------------------+


|method-ast-AstPassMacro.apply|

.. _struct-ast-AstVariantMacro:

.. das:attribute:: AstVariantMacro

|class-ast-AstVariantMacro|

it defines as follows


.. das:function:: AstVariantMacro.visitExprIsVariant(self: AstVariantMacro; prog: ProgramPtr; mod: rtti::Module? const; expr: smart_ptr<ast::ExprIsVariant> const)

visitExprIsVariant returns  :ref:`ExpressionPtr <alias-ExpressionPtr>` 

+--------+-----------------------------------------------------------------------+
+argument+argument type                                                          +
+========+=======================================================================+
+self    + :ref:`ast::AstVariantMacro <struct-ast-AstVariantMacro>`              +
+--------+-----------------------------------------------------------------------+
+prog    + :ref:`ProgramPtr <alias-ProgramPtr>`                                  +
+--------+-----------------------------------------------------------------------+
+mod     + :ref:`rtti::Module <handle-rtti-Module>` ? const                      +
+--------+-----------------------------------------------------------------------+
+expr    +smart_ptr< :ref:`ast::ExprIsVariant <handle-ast-ExprIsVariant>` > const+
+--------+-----------------------------------------------------------------------+


|method-ast-AstVariantMacro.visitExprIsVariant|

.. das:function:: AstVariantMacro.visitExprAsVariant(self: AstVariantMacro; prog: ProgramPtr; mod: rtti::Module? const; expr: smart_ptr<ast::ExprAsVariant> const)

visitExprAsVariant returns  :ref:`ExpressionPtr <alias-ExpressionPtr>` 

+--------+-----------------------------------------------------------------------+
+argument+argument type                                                          +
+========+=======================================================================+
+self    + :ref:`ast::AstVariantMacro <struct-ast-AstVariantMacro>`              +
+--------+-----------------------------------------------------------------------+
+prog    + :ref:`ProgramPtr <alias-ProgramPtr>`                                  +
+--------+-----------------------------------------------------------------------+
+mod     + :ref:`rtti::Module <handle-rtti-Module>` ? const                      +
+--------+-----------------------------------------------------------------------+
+expr    +smart_ptr< :ref:`ast::ExprAsVariant <handle-ast-ExprAsVariant>` > const+
+--------+-----------------------------------------------------------------------+


|method-ast-AstVariantMacro.visitExprAsVariant|

.. das:function:: AstVariantMacro.visitExprSafeAsVariant(self: AstVariantMacro; prog: ProgramPtr; mod: rtti::Module? const; expr: smart_ptr<ast::ExprSafeAsVariant> const)

visitExprSafeAsVariant returns  :ref:`ExpressionPtr <alias-ExpressionPtr>` 

+--------+-------------------------------------------------------------------------------+
+argument+argument type                                                                  +
+========+===============================================================================+
+self    + :ref:`ast::AstVariantMacro <struct-ast-AstVariantMacro>`                      +
+--------+-------------------------------------------------------------------------------+
+prog    + :ref:`ProgramPtr <alias-ProgramPtr>`                                          +
+--------+-------------------------------------------------------------------------------+
+mod     + :ref:`rtti::Module <handle-rtti-Module>` ? const                              +
+--------+-------------------------------------------------------------------------------+
+expr    +smart_ptr< :ref:`ast::ExprSafeAsVariant <handle-ast-ExprSafeAsVariant>` > const+
+--------+-------------------------------------------------------------------------------+


|method-ast-AstVariantMacro.visitExprSafeAsVariant|

.. _struct-ast-AstForLoopMacro:

.. das:attribute:: AstForLoopMacro

|class-ast-AstForLoopMacro|

it defines as follows


.. das:function:: AstForLoopMacro.visitExprFor(self: AstForLoopMacro; prog: ProgramPtr; mod: rtti::Module? const; expr: smart_ptr<ast::ExprFor> const)

visitExprFor returns  :ref:`ExpressionPtr <alias-ExpressionPtr>` 

+--------+-----------------------------------------------------------+
+argument+argument type                                              +
+========+===========================================================+
+self    + :ref:`ast::AstForLoopMacro <struct-ast-AstForLoopMacro>`  +
+--------+-----------------------------------------------------------+
+prog    + :ref:`ProgramPtr <alias-ProgramPtr>`                      +
+--------+-----------------------------------------------------------+
+mod     + :ref:`rtti::Module <handle-rtti-Module>` ? const          +
+--------+-----------------------------------------------------------+
+expr    +smart_ptr< :ref:`ast::ExprFor <handle-ast-ExprFor>` > const+
+--------+-----------------------------------------------------------+


|method-ast-AstForLoopMacro.visitExprFor|

.. _struct-ast-AstCaptureMacro:

.. das:attribute:: AstCaptureMacro

|class-ast-AstCaptureMacro|

it defines as follows


.. das:function:: AstCaptureMacro.captureExpression(self: AstCaptureMacro; prog: rtti::Program? const; mod: rtti::Module? const; expr: ExpressionPtr; etype: TypeDeclPtr)

captureExpression returns  :ref:`ExpressionPtr <alias-ExpressionPtr>` 

+--------+----------------------------------------------------------+
+argument+argument type                                             +
+========+==========================================================+
+self    + :ref:`ast::AstCaptureMacro <struct-ast-AstCaptureMacro>` +
+--------+----------------------------------------------------------+
+prog    + :ref:`rtti::Program <handle-rtti-Program>` ? const       +
+--------+----------------------------------------------------------+
+mod     + :ref:`rtti::Module <handle-rtti-Module>` ? const         +
+--------+----------------------------------------------------------+
+expr    + :ref:`ExpressionPtr <alias-ExpressionPtr>`               +
+--------+----------------------------------------------------------+
+etype   + :ref:`TypeDeclPtr <alias-TypeDeclPtr>`                   +
+--------+----------------------------------------------------------+


|method-ast-AstCaptureMacro.captureExpression|

.. das:function:: AstCaptureMacro.captureFunction(self: AstCaptureMacro; prog: rtti::Program? const; mod: rtti::Module? const; lcs: ast::Structure?; fun: FunctionPtr)

+--------+----------------------------------------------------------+
+argument+argument type                                             +
+========+==========================================================+
+self    + :ref:`ast::AstCaptureMacro <struct-ast-AstCaptureMacro>` +
+--------+----------------------------------------------------------+
+prog    + :ref:`rtti::Program <handle-rtti-Program>` ? const       +
+--------+----------------------------------------------------------+
+mod     + :ref:`rtti::Module <handle-rtti-Module>` ? const         +
+--------+----------------------------------------------------------+
+lcs     + :ref:`ast::Structure <handle-ast-Structure>` ?           +
+--------+----------------------------------------------------------+
+fun     + :ref:`FunctionPtr <alias-FunctionPtr>`                   +
+--------+----------------------------------------------------------+


|method-ast-AstCaptureMacro.captureFunction|

.. _struct-ast-AstSimulateMacro:

.. das:attribute:: AstSimulateMacro

|class-ast-AstSimulateMacro|

it defines as follows


.. das:function:: AstSimulateMacro.preSimulate(self: AstSimulateMacro; prog: rtti::Program? const; ctx: rtti::Context? const)

preSimulate returns bool

+--------+------------------------------------------------------------+
+argument+argument type                                               +
+========+============================================================+
+self    + :ref:`ast::AstSimulateMacro <struct-ast-AstSimulateMacro>` +
+--------+------------------------------------------------------------+
+prog    + :ref:`rtti::Program <handle-rtti-Program>` ? const         +
+--------+------------------------------------------------------------+
+ctx     + :ref:`rtti::Context <handle-rtti-Context>` ? const         +
+--------+------------------------------------------------------------+


|method-ast-AstSimulateMacro.preSimulate|

.. das:function:: AstSimulateMacro.simulate(self: AstSimulateMacro; prog: rtti::Program? const; ctx: rtti::Context? const)

simulate returns bool

+--------+------------------------------------------------------------+
+argument+argument type                                               +
+========+============================================================+
+self    + :ref:`ast::AstSimulateMacro <struct-ast-AstSimulateMacro>` +
+--------+------------------------------------------------------------+
+prog    + :ref:`rtti::Program <handle-rtti-Program>` ? const         +
+--------+------------------------------------------------------------+
+ctx     + :ref:`rtti::Context <handle-rtti-Context>` ? const         +
+--------+------------------------------------------------------------+


|method-ast-AstSimulateMacro.simulate|

.. _struct-ast-AstReaderMacro:

.. das:attribute:: AstReaderMacro

|class-ast-AstReaderMacro|

it defines as follows


.. das:function:: AstReaderMacro.accept(self: AstReaderMacro; prog: ProgramPtr; mod: rtti::Module? const; expr: ast::ExprReader? const; ch: int const; info: LineInfo const)

accept returns bool

+--------+--------------------------------------------------------+
+argument+argument type                                           +
+========+========================================================+
+self    + :ref:`ast::AstReaderMacro <struct-ast-AstReaderMacro>` +
+--------+--------------------------------------------------------+
+prog    + :ref:`ProgramPtr <alias-ProgramPtr>`                   +
+--------+--------------------------------------------------------+
+mod     + :ref:`rtti::Module <handle-rtti-Module>` ? const       +
+--------+--------------------------------------------------------+
+expr    + :ref:`ast::ExprReader <handle-ast-ExprReader>` ? const +
+--------+--------------------------------------------------------+
+ch      +int const                                               +
+--------+--------------------------------------------------------+
+info    + :ref:`rtti::LineInfo <handle-rtti-LineInfo>`  const    +
+--------+--------------------------------------------------------+


|method-ast-AstReaderMacro.accept|

.. das:function:: AstReaderMacro.visit(self: AstReaderMacro; prog: ProgramPtr; mod: rtti::Module? const; expr: smart_ptr<ast::ExprReader> const)

visit returns  :ref:`ExpressionPtr <alias-ExpressionPtr>` 

+--------+-----------------------------------------------------------------+
+argument+argument type                                                    +
+========+=================================================================+
+self    + :ref:`ast::AstReaderMacro <struct-ast-AstReaderMacro>`          +
+--------+-----------------------------------------------------------------+
+prog    + :ref:`ProgramPtr <alias-ProgramPtr>`                            +
+--------+-----------------------------------------------------------------+
+mod     + :ref:`rtti::Module <handle-rtti-Module>` ? const                +
+--------+-----------------------------------------------------------------+
+expr    +smart_ptr< :ref:`ast::ExprReader <handle-ast-ExprReader>` > const+
+--------+-----------------------------------------------------------------+


|method-ast-AstReaderMacro.visit|

.. _struct-ast-AstCommentReader:

.. das:attribute:: AstCommentReader

|class-ast-AstCommentReader|

it defines as follows


.. das:function:: AstCommentReader.open(self: AstCommentReader; prog: ProgramPtr; mod: rtti::Module? const; cpp: bool const; info: LineInfo const)

+--------+------------------------------------------------------------+
+argument+argument type                                               +
+========+============================================================+
+self    + :ref:`ast::AstCommentReader <struct-ast-AstCommentReader>` +
+--------+------------------------------------------------------------+
+prog    + :ref:`ProgramPtr <alias-ProgramPtr>`                       +
+--------+------------------------------------------------------------+
+mod     + :ref:`rtti::Module <handle-rtti-Module>` ? const           +
+--------+------------------------------------------------------------+
+cpp     +bool const                                                  +
+--------+------------------------------------------------------------+
+info    + :ref:`rtti::LineInfo <handle-rtti-LineInfo>`  const        +
+--------+------------------------------------------------------------+


|method-ast-AstCommentReader.open|

.. das:function:: AstCommentReader.accept(self: AstCommentReader; prog: ProgramPtr; mod: rtti::Module? const; ch: int const; info: LineInfo const)

+--------+------------------------------------------------------------+
+argument+argument type                                               +
+========+============================================================+
+self    + :ref:`ast::AstCommentReader <struct-ast-AstCommentReader>` +
+--------+------------------------------------------------------------+
+prog    + :ref:`ProgramPtr <alias-ProgramPtr>`                       +
+--------+------------------------------------------------------------+
+mod     + :ref:`rtti::Module <handle-rtti-Module>` ? const           +
+--------+------------------------------------------------------------+
+ch      +int const                                                   +
+--------+------------------------------------------------------------+
+info    + :ref:`rtti::LineInfo <handle-rtti-LineInfo>`  const        +
+--------+------------------------------------------------------------+


|method-ast-AstCommentReader.accept|

.. das:function:: AstCommentReader.close(self: AstCommentReader; prog: ProgramPtr; mod: rtti::Module? const; info: LineInfo const)

+--------+------------------------------------------------------------+
+argument+argument type                                               +
+========+============================================================+
+self    + :ref:`ast::AstCommentReader <struct-ast-AstCommentReader>` +
+--------+------------------------------------------------------------+
+prog    + :ref:`ProgramPtr <alias-ProgramPtr>`                       +
+--------+------------------------------------------------------------+
+mod     + :ref:`rtti::Module <handle-rtti-Module>` ? const           +
+--------+------------------------------------------------------------+
+info    + :ref:`rtti::LineInfo <handle-rtti-LineInfo>`  const        +
+--------+------------------------------------------------------------+


|method-ast-AstCommentReader.close|

.. das:function:: AstCommentReader.beforeStructure(self: AstCommentReader; prog: ProgramPtr; mod: rtti::Module? const; info: LineInfo const)

+--------+------------------------------------------------------------+
+argument+argument type                                               +
+========+============================================================+
+self    + :ref:`ast::AstCommentReader <struct-ast-AstCommentReader>` +
+--------+------------------------------------------------------------+
+prog    + :ref:`ProgramPtr <alias-ProgramPtr>`                       +
+--------+------------------------------------------------------------+
+mod     + :ref:`rtti::Module <handle-rtti-Module>` ? const           +
+--------+------------------------------------------------------------+
+info    + :ref:`rtti::LineInfo <handle-rtti-LineInfo>`  const        +
+--------+------------------------------------------------------------+


|method-ast-AstCommentReader.beforeStructure|

.. das:function:: AstCommentReader.afterStructure(self: AstCommentReader; st: StructurePtr; prog: ProgramPtr; mod: rtti::Module? const; info: LineInfo const)

+--------+------------------------------------------------------------+
+argument+argument type                                               +
+========+============================================================+
+self    + :ref:`ast::AstCommentReader <struct-ast-AstCommentReader>` +
+--------+------------------------------------------------------------+
+st      + :ref:`StructurePtr <alias-StructurePtr>`                   +
+--------+------------------------------------------------------------+
+prog    + :ref:`ProgramPtr <alias-ProgramPtr>`                       +
+--------+------------------------------------------------------------+
+mod     + :ref:`rtti::Module <handle-rtti-Module>` ? const           +
+--------+------------------------------------------------------------+
+info    + :ref:`rtti::LineInfo <handle-rtti-LineInfo>`  const        +
+--------+------------------------------------------------------------+


|method-ast-AstCommentReader.afterStructure|

.. das:function:: AstCommentReader.beforeStructureFields(self: AstCommentReader; prog: ProgramPtr; mod: rtti::Module? const; info: LineInfo const)

+--------+------------------------------------------------------------+
+argument+argument type                                               +
+========+============================================================+
+self    + :ref:`ast::AstCommentReader <struct-ast-AstCommentReader>` +
+--------+------------------------------------------------------------+
+prog    + :ref:`ProgramPtr <alias-ProgramPtr>`                       +
+--------+------------------------------------------------------------+
+mod     + :ref:`rtti::Module <handle-rtti-Module>` ? const           +
+--------+------------------------------------------------------------+
+info    + :ref:`rtti::LineInfo <handle-rtti-LineInfo>`  const        +
+--------+------------------------------------------------------------+


|method-ast-AstCommentReader.beforeStructureFields|

.. das:function:: AstCommentReader.afterStructureField(self: AstCommentReader; name: string const; prog: ProgramPtr; mod: rtti::Module? const; info: LineInfo const)

+--------+------------------------------------------------------------+
+argument+argument type                                               +
+========+============================================================+
+self    + :ref:`ast::AstCommentReader <struct-ast-AstCommentReader>` +
+--------+------------------------------------------------------------+
+name    +string const                                                +
+--------+------------------------------------------------------------+
+prog    + :ref:`ProgramPtr <alias-ProgramPtr>`                       +
+--------+------------------------------------------------------------+
+mod     + :ref:`rtti::Module <handle-rtti-Module>` ? const           +
+--------+------------------------------------------------------------+
+info    + :ref:`rtti::LineInfo <handle-rtti-LineInfo>`  const        +
+--------+------------------------------------------------------------+


|method-ast-AstCommentReader.afterStructureField|

.. das:function:: AstCommentReader.afterStructureFields(self: AstCommentReader; prog: ProgramPtr; mod: rtti::Module? const; info: LineInfo const)

+--------+------------------------------------------------------------+
+argument+argument type                                               +
+========+============================================================+
+self    + :ref:`ast::AstCommentReader <struct-ast-AstCommentReader>` +
+--------+------------------------------------------------------------+
+prog    + :ref:`ProgramPtr <alias-ProgramPtr>`                       +
+--------+------------------------------------------------------------+
+mod     + :ref:`rtti::Module <handle-rtti-Module>` ? const           +
+--------+------------------------------------------------------------+
+info    + :ref:`rtti::LineInfo <handle-rtti-LineInfo>`  const        +
+--------+------------------------------------------------------------+


|method-ast-AstCommentReader.afterStructureFields|

.. das:function:: AstCommentReader.beforeFunction(self: AstCommentReader; prog: ProgramPtr; mod: rtti::Module? const; info: LineInfo const)

+--------+------------------------------------------------------------+
+argument+argument type                                               +
+========+============================================================+
+self    + :ref:`ast::AstCommentReader <struct-ast-AstCommentReader>` +
+--------+------------------------------------------------------------+
+prog    + :ref:`ProgramPtr <alias-ProgramPtr>`                       +
+--------+------------------------------------------------------------+
+mod     + :ref:`rtti::Module <handle-rtti-Module>` ? const           +
+--------+------------------------------------------------------------+
+info    + :ref:`rtti::LineInfo <handle-rtti-LineInfo>`  const        +
+--------+------------------------------------------------------------+


|method-ast-AstCommentReader.beforeFunction|

.. das:function:: AstCommentReader.afterFunction(self: AstCommentReader; fn: FunctionPtr; prog: ProgramPtr; mod: rtti::Module? const; info: LineInfo const)

+--------+------------------------------------------------------------+
+argument+argument type                                               +
+========+============================================================+
+self    + :ref:`ast::AstCommentReader <struct-ast-AstCommentReader>` +
+--------+------------------------------------------------------------+
+fn      + :ref:`FunctionPtr <alias-FunctionPtr>`                     +
+--------+------------------------------------------------------------+
+prog    + :ref:`ProgramPtr <alias-ProgramPtr>`                       +
+--------+------------------------------------------------------------+
+mod     + :ref:`rtti::Module <handle-rtti-Module>` ? const           +
+--------+------------------------------------------------------------+
+info    + :ref:`rtti::LineInfo <handle-rtti-LineInfo>`  const        +
+--------+------------------------------------------------------------+


|method-ast-AstCommentReader.afterFunction|

.. das:function:: AstCommentReader.beforeGlobalVariables(self: AstCommentReader; prog: ProgramPtr; mod: rtti::Module? const; info: LineInfo const)

+--------+------------------------------------------------------------+
+argument+argument type                                               +
+========+============================================================+
+self    + :ref:`ast::AstCommentReader <struct-ast-AstCommentReader>` +
+--------+------------------------------------------------------------+
+prog    + :ref:`ProgramPtr <alias-ProgramPtr>`                       +
+--------+------------------------------------------------------------+
+mod     + :ref:`rtti::Module <handle-rtti-Module>` ? const           +
+--------+------------------------------------------------------------+
+info    + :ref:`rtti::LineInfo <handle-rtti-LineInfo>`  const        +
+--------+------------------------------------------------------------+


|method-ast-AstCommentReader.beforeGlobalVariables|

.. das:function:: AstCommentReader.afterGlobalVariable(self: AstCommentReader; name: string const; prog: ProgramPtr; mod: rtti::Module? const; info: LineInfo const)

+--------+------------------------------------------------------------+
+argument+argument type                                               +
+========+============================================================+
+self    + :ref:`ast::AstCommentReader <struct-ast-AstCommentReader>` +
+--------+------------------------------------------------------------+
+name    +string const                                                +
+--------+------------------------------------------------------------+
+prog    + :ref:`ProgramPtr <alias-ProgramPtr>`                       +
+--------+------------------------------------------------------------+
+mod     + :ref:`rtti::Module <handle-rtti-Module>` ? const           +
+--------+------------------------------------------------------------+
+info    + :ref:`rtti::LineInfo <handle-rtti-LineInfo>`  const        +
+--------+------------------------------------------------------------+


|method-ast-AstCommentReader.afterGlobalVariable|

.. das:function:: AstCommentReader.afterGlobalVariables(self: AstCommentReader; prog: ProgramPtr; mod: rtti::Module? const; info: LineInfo const)

+--------+------------------------------------------------------------+
+argument+argument type                                               +
+========+============================================================+
+self    + :ref:`ast::AstCommentReader <struct-ast-AstCommentReader>` +
+--------+------------------------------------------------------------+
+prog    + :ref:`ProgramPtr <alias-ProgramPtr>`                       +
+--------+------------------------------------------------------------+
+mod     + :ref:`rtti::Module <handle-rtti-Module>` ? const           +
+--------+------------------------------------------------------------+
+info    + :ref:`rtti::LineInfo <handle-rtti-LineInfo>`  const        +
+--------+------------------------------------------------------------+


|method-ast-AstCommentReader.afterGlobalVariables|

.. das:function:: AstCommentReader.beforeVariant(self: AstCommentReader; prog: ProgramPtr; mod: rtti::Module? const; info: LineInfo const)

+--------+------------------------------------------------------------+
+argument+argument type                                               +
+========+============================================================+
+self    + :ref:`ast::AstCommentReader <struct-ast-AstCommentReader>` +
+--------+------------------------------------------------------------+
+prog    + :ref:`ProgramPtr <alias-ProgramPtr>`                       +
+--------+------------------------------------------------------------+
+mod     + :ref:`rtti::Module <handle-rtti-Module>` ? const           +
+--------+------------------------------------------------------------+
+info    + :ref:`rtti::LineInfo <handle-rtti-LineInfo>`  const        +
+--------+------------------------------------------------------------+


|method-ast-AstCommentReader.beforeVariant|

.. das:function:: AstCommentReader.afterVariant(self: AstCommentReader; name: string const; prog: ProgramPtr; mod: rtti::Module? const; info: LineInfo const)

+--------+------------------------------------------------------------+
+argument+argument type                                               +
+========+============================================================+
+self    + :ref:`ast::AstCommentReader <struct-ast-AstCommentReader>` +
+--------+------------------------------------------------------------+
+name    +string const                                                +
+--------+------------------------------------------------------------+
+prog    + :ref:`ProgramPtr <alias-ProgramPtr>`                       +
+--------+------------------------------------------------------------+
+mod     + :ref:`rtti::Module <handle-rtti-Module>` ? const           +
+--------+------------------------------------------------------------+
+info    + :ref:`rtti::LineInfo <handle-rtti-LineInfo>`  const        +
+--------+------------------------------------------------------------+


|method-ast-AstCommentReader.afterVariant|

.. das:function:: AstCommentReader.beforeEnumeration(self: AstCommentReader; prog: ProgramPtr; mod: rtti::Module? const; info: LineInfo const)

+--------+------------------------------------------------------------+
+argument+argument type                                               +
+========+============================================================+
+self    + :ref:`ast::AstCommentReader <struct-ast-AstCommentReader>` +
+--------+------------------------------------------------------------+
+prog    + :ref:`ProgramPtr <alias-ProgramPtr>`                       +
+--------+------------------------------------------------------------+
+mod     + :ref:`rtti::Module <handle-rtti-Module>` ? const           +
+--------+------------------------------------------------------------+
+info    + :ref:`rtti::LineInfo <handle-rtti-LineInfo>`  const        +
+--------+------------------------------------------------------------+


|method-ast-AstCommentReader.beforeEnumeration|

.. das:function:: AstCommentReader.afterEnumeration(self: AstCommentReader; name: string const; prog: ProgramPtr; mod: rtti::Module? const; info: LineInfo const)

+--------+------------------------------------------------------------+
+argument+argument type                                               +
+========+============================================================+
+self    + :ref:`ast::AstCommentReader <struct-ast-AstCommentReader>` +
+--------+------------------------------------------------------------+
+name    +string const                                                +
+--------+------------------------------------------------------------+
+prog    + :ref:`ProgramPtr <alias-ProgramPtr>`                       +
+--------+------------------------------------------------------------+
+mod     + :ref:`rtti::Module <handle-rtti-Module>` ? const           +
+--------+------------------------------------------------------------+
+info    + :ref:`rtti::LineInfo <handle-rtti-LineInfo>`  const        +
+--------+------------------------------------------------------------+


|method-ast-AstCommentReader.afterEnumeration|

.. das:function:: AstCommentReader.beforeAlias(self: AstCommentReader; prog: ProgramPtr; mod: rtti::Module? const; info: LineInfo const)

+--------+------------------------------------------------------------+
+argument+argument type                                               +
+========+============================================================+
+self    + :ref:`ast::AstCommentReader <struct-ast-AstCommentReader>` +
+--------+------------------------------------------------------------+
+prog    + :ref:`ProgramPtr <alias-ProgramPtr>`                       +
+--------+------------------------------------------------------------+
+mod     + :ref:`rtti::Module <handle-rtti-Module>` ? const           +
+--------+------------------------------------------------------------+
+info    + :ref:`rtti::LineInfo <handle-rtti-LineInfo>`  const        +
+--------+------------------------------------------------------------+


|method-ast-AstCommentReader.beforeAlias|

.. das:function:: AstCommentReader.afterAlias(self: AstCommentReader; name: string const; prog: ProgramPtr; mod: rtti::Module? const; info: LineInfo const)

+--------+------------------------------------------------------------+
+argument+argument type                                               +
+========+============================================================+
+self    + :ref:`ast::AstCommentReader <struct-ast-AstCommentReader>` +
+--------+------------------------------------------------------------+
+name    +string const                                                +
+--------+------------------------------------------------------------+
+prog    + :ref:`ProgramPtr <alias-ProgramPtr>`                       +
+--------+------------------------------------------------------------+
+mod     + :ref:`rtti::Module <handle-rtti-Module>` ? const           +
+--------+------------------------------------------------------------+
+info    + :ref:`rtti::LineInfo <handle-rtti-LineInfo>`  const        +
+--------+------------------------------------------------------------+


|method-ast-AstCommentReader.afterAlias|

.. _struct-ast-AstCallMacro:

.. das:attribute:: AstCallMacro

|class-ast-AstCallMacro|

it defines as follows


.. das:function:: AstCallMacro.preVisit(self: AstCallMacro; prog: ProgramPtr; mod: rtti::Module? const; expr: smart_ptr<ast::ExprCallMacro> const)

+--------+-----------------------------------------------------------------------+
+argument+argument type                                                          +
+========+=======================================================================+
+self    + :ref:`ast::AstCallMacro <struct-ast-AstCallMacro>`                    +
+--------+-----------------------------------------------------------------------+
+prog    + :ref:`ProgramPtr <alias-ProgramPtr>`                                  +
+--------+-----------------------------------------------------------------------+
+mod     + :ref:`rtti::Module <handle-rtti-Module>` ? const                      +
+--------+-----------------------------------------------------------------------+
+expr    +smart_ptr< :ref:`ast::ExprCallMacro <handle-ast-ExprCallMacro>` > const+
+--------+-----------------------------------------------------------------------+


|method-ast-AstCallMacro.preVisit|

.. das:function:: AstCallMacro.visit(self: AstCallMacro; prog: ProgramPtr; mod: rtti::Module? const; expr: smart_ptr<ast::ExprCallMacro> const)

visit returns  :ref:`ExpressionPtr <alias-ExpressionPtr>` 

+--------+-----------------------------------------------------------------------+
+argument+argument type                                                          +
+========+=======================================================================+
+self    + :ref:`ast::AstCallMacro <struct-ast-AstCallMacro>`                    +
+--------+-----------------------------------------------------------------------+
+prog    + :ref:`ProgramPtr <alias-ProgramPtr>`                                  +
+--------+-----------------------------------------------------------------------+
+mod     + :ref:`rtti::Module <handle-rtti-Module>` ? const                      +
+--------+-----------------------------------------------------------------------+
+expr    +smart_ptr< :ref:`ast::ExprCallMacro <handle-ast-ExprCallMacro>` > const+
+--------+-----------------------------------------------------------------------+


|method-ast-AstCallMacro.visit|

.. das:function:: AstCallMacro.canVisitArgument(self: AstCallMacro; expr: smart_ptr<ast::ExprCallMacro> const; argIndex: int const)

canVisitArgument returns bool

+--------+-----------------------------------------------------------------------+
+argument+argument type                                                          +
+========+=======================================================================+
+self    + :ref:`ast::AstCallMacro <struct-ast-AstCallMacro>`                    +
+--------+-----------------------------------------------------------------------+
+expr    +smart_ptr< :ref:`ast::ExprCallMacro <handle-ast-ExprCallMacro>` > const+
+--------+-----------------------------------------------------------------------+
+argIndex+int const                                                              +
+--------+-----------------------------------------------------------------------+


|method-ast-AstCallMacro.canVisitArgument|

.. das:function:: AstCallMacro.canFoldReturnResult(self: AstCallMacro; expr: smart_ptr<ast::ExprCallMacro> const)

canFoldReturnResult returns bool

+--------+-----------------------------------------------------------------------+
+argument+argument type                                                          +
+========+=======================================================================+
+self    + :ref:`ast::AstCallMacro <struct-ast-AstCallMacro>`                    +
+--------+-----------------------------------------------------------------------+
+expr    +smart_ptr< :ref:`ast::ExprCallMacro <handle-ast-ExprCallMacro>` > const+
+--------+-----------------------------------------------------------------------+


|method-ast-AstCallMacro.canFoldReturnResult|

.. _struct-ast-AstTypeInfoMacro:

.. das:attribute:: AstTypeInfoMacro

|class-ast-AstTypeInfoMacro|

it defines as follows


.. das:function:: AstTypeInfoMacro.getAstChange(self: AstTypeInfoMacro; expr: smart_ptr<ast::ExprTypeInfo> const; errors: das_string)

getAstChange returns  :ref:`ExpressionPtr <alias-ExpressionPtr>` 

+--------+---------------------------------------------------------------------+
+argument+argument type                                                        +
+========+=====================================================================+
+self    + :ref:`ast::AstTypeInfoMacro <struct-ast-AstTypeInfoMacro>`          +
+--------+---------------------------------------------------------------------+
+expr    +smart_ptr< :ref:`ast::ExprTypeInfo <handle-ast-ExprTypeInfo>` > const+
+--------+---------------------------------------------------------------------+
+errors  + :ref:`builtin::das_string <handle-builtin-das_string>`              +
+--------+---------------------------------------------------------------------+


|method-ast-AstTypeInfoMacro.getAstChange|

.. das:function:: AstTypeInfoMacro.getAstType(self: AstTypeInfoMacro; lib: ModuleLibrary; expr: smart_ptr<ast::ExprTypeInfo> const; errors: das_string)

getAstType returns  :ref:`TypeDeclPtr <alias-TypeDeclPtr>` 

+--------+---------------------------------------------------------------------+
+argument+argument type                                                        +
+========+=====================================================================+
+self    + :ref:`ast::AstTypeInfoMacro <struct-ast-AstTypeInfoMacro>`          +
+--------+---------------------------------------------------------------------+
+lib     + :ref:`ast::ModuleLibrary <handle-ast-ModuleLibrary>`                +
+--------+---------------------------------------------------------------------+
+expr    +smart_ptr< :ref:`ast::ExprTypeInfo <handle-ast-ExprTypeInfo>` > const+
+--------+---------------------------------------------------------------------+
+errors  + :ref:`builtin::das_string <handle-builtin-das_string>`              +
+--------+---------------------------------------------------------------------+


|method-ast-AstTypeInfoMacro.getAstType|

.. _struct-ast-AstEnumerationAnnotation:

.. das:attribute:: AstEnumerationAnnotation

|class-ast-AstEnumerationAnnotation|

it defines as follows


.. das:function:: AstEnumerationAnnotation.apply(self: AstEnumerationAnnotation; st: EnumerationPtr; group: ModuleGroup; args: AnnotationArgumentList const; errors: das_string)

apply returns bool

+--------+--------------------------------------------------------------------------------+
+argument+argument type                                                                   +
+========+================================================================================+
+self    + :ref:`ast::AstEnumerationAnnotation <struct-ast-AstEnumerationAnnotation>`     +
+--------+--------------------------------------------------------------------------------+
+st      + :ref:`EnumerationPtr <alias-EnumerationPtr>`                                   +
+--------+--------------------------------------------------------------------------------+
+group   + :ref:`rtti::ModuleGroup <handle-rtti-ModuleGroup>`                             +
+--------+--------------------------------------------------------------------------------+
+args    + :ref:`rtti::AnnotationArgumentList <handle-rtti-AnnotationArgumentList>`  const+
+--------+--------------------------------------------------------------------------------+
+errors  + :ref:`builtin::das_string <handle-builtin-das_string>`                         +
+--------+--------------------------------------------------------------------------------+


|method-ast-AstEnumerationAnnotation.apply|

.. _struct-ast-AstVisitor:

.. das:attribute:: AstVisitor

|class-ast-AstVisitor|

it defines as follows


.. das:function:: AstVisitor.preVisitProgram(self: AstVisitor; prog: ProgramPtr)

+--------+------------------------------------------------+
+argument+argument type                                   +
+========+================================================+
+self    + :ref:`ast::AstVisitor <struct-ast-AstVisitor>` +
+--------+------------------------------------------------+
+prog    + :ref:`ProgramPtr <alias-ProgramPtr>`           +
+--------+------------------------------------------------+


|method-ast-AstVisitor.preVisitProgram|

.. das:function:: AstVisitor.visitProgram(self: AstVisitor; porg: ProgramPtr)

+--------+------------------------------------------------+
+argument+argument type                                   +
+========+================================================+
+self    + :ref:`ast::AstVisitor <struct-ast-AstVisitor>` +
+--------+------------------------------------------------+
+porg    + :ref:`ProgramPtr <alias-ProgramPtr>`           +
+--------+------------------------------------------------+


|method-ast-AstVisitor.visitProgram|

.. das:function:: AstVisitor.preVisitProgramBody(self: AstVisitor; prog: ProgramPtr; mod: rtti::Module? const)

+--------+-------------------------------------------------+
+argument+argument type                                    +
+========+=================================================+
+self    + :ref:`ast::AstVisitor <struct-ast-AstVisitor>`  +
+--------+-------------------------------------------------+
+prog    + :ref:`ProgramPtr <alias-ProgramPtr>`            +
+--------+-------------------------------------------------+
+mod     + :ref:`rtti::Module <handle-rtti-Module>` ? const+
+--------+-------------------------------------------------+


|method-ast-AstVisitor.preVisitProgramBody|

.. das:function:: AstVisitor.preVisitModule(self: AstVisitor; mod: rtti::Module? const)

+--------+-------------------------------------------------+
+argument+argument type                                    +
+========+=================================================+
+self    + :ref:`ast::AstVisitor <struct-ast-AstVisitor>`  +
+--------+-------------------------------------------------+
+mod     + :ref:`rtti::Module <handle-rtti-Module>` ? const+
+--------+-------------------------------------------------+


|method-ast-AstVisitor.preVisitModule|

.. das:function:: AstVisitor.visitModule(self: AstVisitor; mod: rtti::Module? const)

+--------+-------------------------------------------------+
+argument+argument type                                    +
+========+=================================================+
+self    + :ref:`ast::AstVisitor <struct-ast-AstVisitor>`  +
+--------+-------------------------------------------------+
+mod     + :ref:`rtti::Module <handle-rtti-Module>` ? const+
+--------+-------------------------------------------------+


|method-ast-AstVisitor.visitModule|

.. das:function:: AstVisitor.preVisitTypeDecl(self: AstVisitor; typ: TypeDeclPtr)

+--------+------------------------------------------------+
+argument+argument type                                   +
+========+================================================+
+self    + :ref:`ast::AstVisitor <struct-ast-AstVisitor>` +
+--------+------------------------------------------------+
+typ     + :ref:`TypeDeclPtr <alias-TypeDeclPtr>`         +
+--------+------------------------------------------------+


|method-ast-AstVisitor.preVisitTypeDecl|

.. das:function:: AstVisitor.visitTypeDecl(self: AstVisitor; typ: TypeDeclPtr)

visitTypeDecl returns  :ref:`TypeDeclPtr <alias-TypeDeclPtr>` 

+--------+------------------------------------------------+
+argument+argument type                                   +
+========+================================================+
+self    + :ref:`ast::AstVisitor <struct-ast-AstVisitor>` +
+--------+------------------------------------------------+
+typ     + :ref:`TypeDeclPtr <alias-TypeDeclPtr>`         +
+--------+------------------------------------------------+


|method-ast-AstVisitor.visitTypeDecl|

.. das:function:: AstVisitor.preVisitAlias(self: AstVisitor; typ: TypeDeclPtr; name: das_string const)

+--------+--------------------------------------------------------------+
+argument+argument type                                                 +
+========+==============================================================+
+self    + :ref:`ast::AstVisitor <struct-ast-AstVisitor>`               +
+--------+--------------------------------------------------------------+
+typ     + :ref:`TypeDeclPtr <alias-TypeDeclPtr>`                       +
+--------+--------------------------------------------------------------+
+name    + :ref:`builtin::das_string <handle-builtin-das_string>`  const+
+--------+--------------------------------------------------------------+


|method-ast-AstVisitor.preVisitAlias|

.. das:function:: AstVisitor.visitAlias(self: AstVisitor; typ: TypeDeclPtr; name: das_string const)

visitAlias returns  :ref:`TypeDeclPtr <alias-TypeDeclPtr>` 

+--------+--------------------------------------------------------------+
+argument+argument type                                                 +
+========+==============================================================+
+self    + :ref:`ast::AstVisitor <struct-ast-AstVisitor>`               +
+--------+--------------------------------------------------------------+
+typ     + :ref:`TypeDeclPtr <alias-TypeDeclPtr>`                       +
+--------+--------------------------------------------------------------+
+name    + :ref:`builtin::das_string <handle-builtin-das_string>`  const+
+--------+--------------------------------------------------------------+


|method-ast-AstVisitor.visitAlias|

.. das:function:: AstVisitor.canVisitEnumeration(self: AstVisitor; arg: ast::Enumeration? const)

canVisitEnumeration returns bool

+--------+---------------------------------------------------------+
+argument+argument type                                            +
+========+=========================================================+
+self    + :ref:`ast::AstVisitor <struct-ast-AstVisitor>`          +
+--------+---------------------------------------------------------+
+arg     + :ref:`ast::Enumeration <handle-ast-Enumeration>` ? const+
+--------+---------------------------------------------------------+


|method-ast-AstVisitor.canVisitEnumeration|

.. das:function:: AstVisitor.preVisitEnumeration(self: AstVisitor; enu: EnumerationPtr)

+--------+------------------------------------------------+
+argument+argument type                                   +
+========+================================================+
+self    + :ref:`ast::AstVisitor <struct-ast-AstVisitor>` +
+--------+------------------------------------------------+
+enu     + :ref:`EnumerationPtr <alias-EnumerationPtr>`   +
+--------+------------------------------------------------+


|method-ast-AstVisitor.preVisitEnumeration|

.. das:function:: AstVisitor.preVisitEnumerationValue(self: AstVisitor; enu: EnumerationPtr; name: das_string const; value: ExpressionPtr; last: bool const)

+--------+--------------------------------------------------------------+
+argument+argument type                                                 +
+========+==============================================================+
+self    + :ref:`ast::AstVisitor <struct-ast-AstVisitor>`               +
+--------+--------------------------------------------------------------+
+enu     + :ref:`EnumerationPtr <alias-EnumerationPtr>`                 +
+--------+--------------------------------------------------------------+
+name    + :ref:`builtin::das_string <handle-builtin-das_string>`  const+
+--------+--------------------------------------------------------------+
+value   + :ref:`ExpressionPtr <alias-ExpressionPtr>`                   +
+--------+--------------------------------------------------------------+
+last    +bool const                                                    +
+--------+--------------------------------------------------------------+


|method-ast-AstVisitor.preVisitEnumerationValue|

.. das:function:: AstVisitor.visitEnumerationValue(self: AstVisitor; enu: EnumerationPtr; name: das_string const; value: ExpressionPtr; last: bool const)

visitEnumerationValue returns  :ref:`ExpressionPtr <alias-ExpressionPtr>` 

+--------+--------------------------------------------------------------+
+argument+argument type                                                 +
+========+==============================================================+
+self    + :ref:`ast::AstVisitor <struct-ast-AstVisitor>`               +
+--------+--------------------------------------------------------------+
+enu     + :ref:`EnumerationPtr <alias-EnumerationPtr>`                 +
+--------+--------------------------------------------------------------+
+name    + :ref:`builtin::das_string <handle-builtin-das_string>`  const+
+--------+--------------------------------------------------------------+
+value   + :ref:`ExpressionPtr <alias-ExpressionPtr>`                   +
+--------+--------------------------------------------------------------+
+last    +bool const                                                    +
+--------+--------------------------------------------------------------+


|method-ast-AstVisitor.visitEnumerationValue|

.. das:function:: AstVisitor.visitEnumeration(self: AstVisitor; enu: EnumerationPtr)

visitEnumeration returns  :ref:`EnumerationPtr <alias-EnumerationPtr>` 

+--------+------------------------------------------------+
+argument+argument type                                   +
+========+================================================+
+self    + :ref:`ast::AstVisitor <struct-ast-AstVisitor>` +
+--------+------------------------------------------------+
+enu     + :ref:`EnumerationPtr <alias-EnumerationPtr>`   +
+--------+------------------------------------------------+


|method-ast-AstVisitor.visitEnumeration|

.. das:function:: AstVisitor.canVisitStructure(self: AstVisitor; arg: ast::Structure? const)

canVisitStructure returns bool

+--------+-----------------------------------------------------+
+argument+argument type                                        +
+========+=====================================================+
+self    + :ref:`ast::AstVisitor <struct-ast-AstVisitor>`      +
+--------+-----------------------------------------------------+
+arg     + :ref:`ast::Structure <handle-ast-Structure>` ? const+
+--------+-----------------------------------------------------+


|method-ast-AstVisitor.canVisitStructure|

.. das:function:: AstVisitor.preVisitStructure(self: AstVisitor; str: StructurePtr)

+--------+------------------------------------------------+
+argument+argument type                                   +
+========+================================================+
+self    + :ref:`ast::AstVisitor <struct-ast-AstVisitor>` +
+--------+------------------------------------------------+
+str     + :ref:`StructurePtr <alias-StructurePtr>`       +
+--------+------------------------------------------------+


|method-ast-AstVisitor.preVisitStructure|

.. das:function:: AstVisitor.preVisitStructureField(self: AstVisitor; str: StructurePtr; decl: FieldDeclaration const; last: bool const)

+--------+------------------------------------------------------------------+
+argument+argument type                                                     +
+========+==================================================================+
+self    + :ref:`ast::AstVisitor <struct-ast-AstVisitor>`                   +
+--------+------------------------------------------------------------------+
+str     + :ref:`StructurePtr <alias-StructurePtr>`                         +
+--------+------------------------------------------------------------------+
+decl    + :ref:`ast::FieldDeclaration <handle-ast-FieldDeclaration>`  const+
+--------+------------------------------------------------------------------+
+last    +bool const                                                        +
+--------+------------------------------------------------------------------+


|method-ast-AstVisitor.preVisitStructureField|

.. das:function:: AstVisitor.visitStructureField(self: AstVisitor; str: StructurePtr; decl: FieldDeclaration const; last: bool const)

+--------+------------------------------------------------------------------+
+argument+argument type                                                     +
+========+==================================================================+
+self    + :ref:`ast::AstVisitor <struct-ast-AstVisitor>`                   +
+--------+------------------------------------------------------------------+
+str     + :ref:`StructurePtr <alias-StructurePtr>`                         +
+--------+------------------------------------------------------------------+
+decl    + :ref:`ast::FieldDeclaration <handle-ast-FieldDeclaration>`  const+
+--------+------------------------------------------------------------------+
+last    +bool const                                                        +
+--------+------------------------------------------------------------------+


|method-ast-AstVisitor.visitStructureField|

.. das:function:: AstVisitor.visitStructure(self: AstVisitor; str: StructurePtr)

visitStructure returns  :ref:`StructurePtr <alias-StructurePtr>` 

+--------+------------------------------------------------+
+argument+argument type                                   +
+========+================================================+
+self    + :ref:`ast::AstVisitor <struct-ast-AstVisitor>` +
+--------+------------------------------------------------+
+str     + :ref:`StructurePtr <alias-StructurePtr>`       +
+--------+------------------------------------------------+


|method-ast-AstVisitor.visitStructure|

.. das:function:: AstVisitor.canVisitFunction(self: AstVisitor; fun: ast::Function? const)

canVisitFunction returns bool

+--------+---------------------------------------------------+
+argument+argument type                                      +
+========+===================================================+
+self    + :ref:`ast::AstVisitor <struct-ast-AstVisitor>`    +
+--------+---------------------------------------------------+
+fun     + :ref:`ast::Function <handle-ast-Function>` ? const+
+--------+---------------------------------------------------+


|method-ast-AstVisitor.canVisitFunction|

.. das:function:: AstVisitor.canVisitFunctionArgumentInit(self: AstVisitor; fun: ast::Function? const; arg: VariablePtr; value: ExpressionPtr)

canVisitFunctionArgumentInit returns bool

+--------+---------------------------------------------------+
+argument+argument type                                      +
+========+===================================================+
+self    + :ref:`ast::AstVisitor <struct-ast-AstVisitor>`    +
+--------+---------------------------------------------------+
+fun     + :ref:`ast::Function <handle-ast-Function>` ? const+
+--------+---------------------------------------------------+
+arg     + :ref:`VariablePtr <alias-VariablePtr>`            +
+--------+---------------------------------------------------+
+value   + :ref:`ExpressionPtr <alias-ExpressionPtr>`        +
+--------+---------------------------------------------------+


|method-ast-AstVisitor.canVisitFunctionArgumentInit|

.. das:function:: AstVisitor.preVisitFunction(self: AstVisitor; fun: FunctionPtr)

+--------+------------------------------------------------+
+argument+argument type                                   +
+========+================================================+
+self    + :ref:`ast::AstVisitor <struct-ast-AstVisitor>` +
+--------+------------------------------------------------+
+fun     + :ref:`FunctionPtr <alias-FunctionPtr>`         +
+--------+------------------------------------------------+


|method-ast-AstVisitor.preVisitFunction|

.. das:function:: AstVisitor.visitFunction(self: AstVisitor; fun: FunctionPtr)

visitFunction returns  :ref:`FunctionPtr <alias-FunctionPtr>` 

+--------+------------------------------------------------+
+argument+argument type                                   +
+========+================================================+
+self    + :ref:`ast::AstVisitor <struct-ast-AstVisitor>` +
+--------+------------------------------------------------+
+fun     + :ref:`FunctionPtr <alias-FunctionPtr>`         +
+--------+------------------------------------------------+


|method-ast-AstVisitor.visitFunction|

.. das:function:: AstVisitor.preVisitFunctionArgument(self: AstVisitor; fun: FunctionPtr; arg: VariablePtr; lastArg: bool const)

+--------+------------------------------------------------+
+argument+argument type                                   +
+========+================================================+
+self    + :ref:`ast::AstVisitor <struct-ast-AstVisitor>` +
+--------+------------------------------------------------+
+fun     + :ref:`FunctionPtr <alias-FunctionPtr>`         +
+--------+------------------------------------------------+
+arg     + :ref:`VariablePtr <alias-VariablePtr>`         +
+--------+------------------------------------------------+
+lastArg +bool const                                      +
+--------+------------------------------------------------+


|method-ast-AstVisitor.preVisitFunctionArgument|

.. das:function:: AstVisitor.visitFunctionArgument(self: AstVisitor; fun: FunctionPtr; arg: VariablePtr; lastArg: bool const)

visitFunctionArgument returns  :ref:`VariablePtr <alias-VariablePtr>` 

+--------+------------------------------------------------+
+argument+argument type                                   +
+========+================================================+
+self    + :ref:`ast::AstVisitor <struct-ast-AstVisitor>` +
+--------+------------------------------------------------+
+fun     + :ref:`FunctionPtr <alias-FunctionPtr>`         +
+--------+------------------------------------------------+
+arg     + :ref:`VariablePtr <alias-VariablePtr>`         +
+--------+------------------------------------------------+
+lastArg +bool const                                      +
+--------+------------------------------------------------+


|method-ast-AstVisitor.visitFunctionArgument|

.. das:function:: AstVisitor.preVisitFunctionArgumentInit(self: AstVisitor; fun: FunctionPtr; arg: VariablePtr; value: ExpressionPtr)

+--------+------------------------------------------------+
+argument+argument type                                   +
+========+================================================+
+self    + :ref:`ast::AstVisitor <struct-ast-AstVisitor>` +
+--------+------------------------------------------------+
+fun     + :ref:`FunctionPtr <alias-FunctionPtr>`         +
+--------+------------------------------------------------+
+arg     + :ref:`VariablePtr <alias-VariablePtr>`         +
+--------+------------------------------------------------+
+value   + :ref:`ExpressionPtr <alias-ExpressionPtr>`     +
+--------+------------------------------------------------+


|method-ast-AstVisitor.preVisitFunctionArgumentInit|

.. das:function:: AstVisitor.visitFunctionArgumentInit(self: AstVisitor; fun: FunctionPtr; arg: VariablePtr; value: ExpressionPtr)

visitFunctionArgumentInit returns  :ref:`ExpressionPtr <alias-ExpressionPtr>` 

+--------+------------------------------------------------+
+argument+argument type                                   +
+========+================================================+
+self    + :ref:`ast::AstVisitor <struct-ast-AstVisitor>` +
+--------+------------------------------------------------+
+fun     + :ref:`FunctionPtr <alias-FunctionPtr>`         +
+--------+------------------------------------------------+
+arg     + :ref:`VariablePtr <alias-VariablePtr>`         +
+--------+------------------------------------------------+
+value   + :ref:`ExpressionPtr <alias-ExpressionPtr>`     +
+--------+------------------------------------------------+


|method-ast-AstVisitor.visitFunctionArgumentInit|

.. das:function:: AstVisitor.preVisitFunctionBody(self: AstVisitor; fun: FunctionPtr; expr: ExpressionPtr)

+--------+------------------------------------------------+
+argument+argument type                                   +
+========+================================================+
+self    + :ref:`ast::AstVisitor <struct-ast-AstVisitor>` +
+--------+------------------------------------------------+
+fun     + :ref:`FunctionPtr <alias-FunctionPtr>`         +
+--------+------------------------------------------------+
+expr    + :ref:`ExpressionPtr <alias-ExpressionPtr>`     +
+--------+------------------------------------------------+


|method-ast-AstVisitor.preVisitFunctionBody|

.. das:function:: AstVisitor.visitFunctionBody(self: AstVisitor; fun: FunctionPtr; expr: ExpressionPtr)

visitFunctionBody returns  :ref:`ExpressionPtr <alias-ExpressionPtr>` 

+--------+------------------------------------------------+
+argument+argument type                                   +
+========+================================================+
+self    + :ref:`ast::AstVisitor <struct-ast-AstVisitor>` +
+--------+------------------------------------------------+
+fun     + :ref:`FunctionPtr <alias-FunctionPtr>`         +
+--------+------------------------------------------------+
+expr    + :ref:`ExpressionPtr <alias-ExpressionPtr>`     +
+--------+------------------------------------------------+


|method-ast-AstVisitor.visitFunctionBody|

.. das:function:: AstVisitor.preVisitExpression(self: AstVisitor; expr: ExpressionPtr)

+--------+------------------------------------------------+
+argument+argument type                                   +
+========+================================================+
+self    + :ref:`ast::AstVisitor <struct-ast-AstVisitor>` +
+--------+------------------------------------------------+
+expr    + :ref:`ExpressionPtr <alias-ExpressionPtr>`     +
+--------+------------------------------------------------+


|method-ast-AstVisitor.preVisitExpression|

.. das:function:: AstVisitor.visitExpression(self: AstVisitor; expr: ExpressionPtr)

visitExpression returns  :ref:`ExpressionPtr <alias-ExpressionPtr>` 

+--------+------------------------------------------------+
+argument+argument type                                   +
+========+================================================+
+self    + :ref:`ast::AstVisitor <struct-ast-AstVisitor>` +
+--------+------------------------------------------------+
+expr    + :ref:`ExpressionPtr <alias-ExpressionPtr>`     +
+--------+------------------------------------------------+


|method-ast-AstVisitor.visitExpression|

.. das:function:: AstVisitor.preVisitExprBlock(self: AstVisitor; blk: smart_ptr<ast::ExprBlock> const)

+--------+---------------------------------------------------------------+
+argument+argument type                                                  +
+========+===============================================================+
+self    + :ref:`ast::AstVisitor <struct-ast-AstVisitor>`                +
+--------+---------------------------------------------------------------+
+blk     +smart_ptr< :ref:`ast::ExprBlock <handle-ast-ExprBlock>` > const+
+--------+---------------------------------------------------------------+


|method-ast-AstVisitor.preVisitExprBlock|

.. das:function:: AstVisitor.visitExprBlock(self: AstVisitor; blk: smart_ptr<ast::ExprBlock> const)

visitExprBlock returns  :ref:`ExpressionPtr <alias-ExpressionPtr>` 

+--------+---------------------------------------------------------------+
+argument+argument type                                                  +
+========+===============================================================+
+self    + :ref:`ast::AstVisitor <struct-ast-AstVisitor>`                +
+--------+---------------------------------------------------------------+
+blk     +smart_ptr< :ref:`ast::ExprBlock <handle-ast-ExprBlock>` > const+
+--------+---------------------------------------------------------------+


|method-ast-AstVisitor.visitExprBlock|

.. das:function:: AstVisitor.preVisitExprBlockArgument(self: AstVisitor; blk: smart_ptr<ast::ExprBlock> const; arg: VariablePtr; lastArg: bool const)

+--------+---------------------------------------------------------------+
+argument+argument type                                                  +
+========+===============================================================+
+self    + :ref:`ast::AstVisitor <struct-ast-AstVisitor>`                +
+--------+---------------------------------------------------------------+
+blk     +smart_ptr< :ref:`ast::ExprBlock <handle-ast-ExprBlock>` > const+
+--------+---------------------------------------------------------------+
+arg     + :ref:`VariablePtr <alias-VariablePtr>`                        +
+--------+---------------------------------------------------------------+
+lastArg +bool const                                                     +
+--------+---------------------------------------------------------------+


|method-ast-AstVisitor.preVisitExprBlockArgument|

.. das:function:: AstVisitor.visitExprBlockArgument(self: AstVisitor; blk: smart_ptr<ast::ExprBlock> const; arg: VariablePtr; lastArg: bool const)

visitExprBlockArgument returns  :ref:`VariablePtr <alias-VariablePtr>` 

+--------+---------------------------------------------------------------+
+argument+argument type                                                  +
+========+===============================================================+
+self    + :ref:`ast::AstVisitor <struct-ast-AstVisitor>`                +
+--------+---------------------------------------------------------------+
+blk     +smart_ptr< :ref:`ast::ExprBlock <handle-ast-ExprBlock>` > const+
+--------+---------------------------------------------------------------+
+arg     + :ref:`VariablePtr <alias-VariablePtr>`                        +
+--------+---------------------------------------------------------------+
+lastArg +bool const                                                     +
+--------+---------------------------------------------------------------+


|method-ast-AstVisitor.visitExprBlockArgument|

.. das:function:: AstVisitor.preVisitExprBlockArgumentInit(self: AstVisitor; blk: smart_ptr<ast::ExprBlock> const; arg: VariablePtr; expr: ExpressionPtr)

+--------+---------------------------------------------------------------+
+argument+argument type                                                  +
+========+===============================================================+
+self    + :ref:`ast::AstVisitor <struct-ast-AstVisitor>`                +
+--------+---------------------------------------------------------------+
+blk     +smart_ptr< :ref:`ast::ExprBlock <handle-ast-ExprBlock>` > const+
+--------+---------------------------------------------------------------+
+arg     + :ref:`VariablePtr <alias-VariablePtr>`                        +
+--------+---------------------------------------------------------------+
+expr    + :ref:`ExpressionPtr <alias-ExpressionPtr>`                    +
+--------+---------------------------------------------------------------+


|method-ast-AstVisitor.preVisitExprBlockArgumentInit|

.. das:function:: AstVisitor.visitExprBlockArgumentInit(self: AstVisitor; blk: smart_ptr<ast::ExprBlock> const; arg: VariablePtr; expr: ExpressionPtr)

visitExprBlockArgumentInit returns  :ref:`ExpressionPtr <alias-ExpressionPtr>` 

+--------+---------------------------------------------------------------+
+argument+argument type                                                  +
+========+===============================================================+
+self    + :ref:`ast::AstVisitor <struct-ast-AstVisitor>`                +
+--------+---------------------------------------------------------------+
+blk     +smart_ptr< :ref:`ast::ExprBlock <handle-ast-ExprBlock>` > const+
+--------+---------------------------------------------------------------+
+arg     + :ref:`VariablePtr <alias-VariablePtr>`                        +
+--------+---------------------------------------------------------------+
+expr    + :ref:`ExpressionPtr <alias-ExpressionPtr>`                    +
+--------+---------------------------------------------------------------+


|method-ast-AstVisitor.visitExprBlockArgumentInit|

.. das:function:: AstVisitor.preVisitExprBlockExpression(self: AstVisitor; blk: smart_ptr<ast::ExprBlock> const; expr: ExpressionPtr)

+--------+---------------------------------------------------------------+
+argument+argument type                                                  +
+========+===============================================================+
+self    + :ref:`ast::AstVisitor <struct-ast-AstVisitor>`                +
+--------+---------------------------------------------------------------+
+blk     +smart_ptr< :ref:`ast::ExprBlock <handle-ast-ExprBlock>` > const+
+--------+---------------------------------------------------------------+
+expr    + :ref:`ExpressionPtr <alias-ExpressionPtr>`                    +
+--------+---------------------------------------------------------------+


|method-ast-AstVisitor.preVisitExprBlockExpression|

.. das:function:: AstVisitor.visitExprBlockExpression(self: AstVisitor; blk: smart_ptr<ast::ExprBlock> const; expr: ExpressionPtr)

visitExprBlockExpression returns  :ref:`ExpressionPtr <alias-ExpressionPtr>` 

+--------+---------------------------------------------------------------+
+argument+argument type                                                  +
+========+===============================================================+
+self    + :ref:`ast::AstVisitor <struct-ast-AstVisitor>`                +
+--------+---------------------------------------------------------------+
+blk     +smart_ptr< :ref:`ast::ExprBlock <handle-ast-ExprBlock>` > const+
+--------+---------------------------------------------------------------+
+expr    + :ref:`ExpressionPtr <alias-ExpressionPtr>`                    +
+--------+---------------------------------------------------------------+


|method-ast-AstVisitor.visitExprBlockExpression|

.. das:function:: AstVisitor.preVisitExprBlockFinal(self: AstVisitor; blk: smart_ptr<ast::ExprBlock> const)

+--------+---------------------------------------------------------------+
+argument+argument type                                                  +
+========+===============================================================+
+self    + :ref:`ast::AstVisitor <struct-ast-AstVisitor>`                +
+--------+---------------------------------------------------------------+
+blk     +smart_ptr< :ref:`ast::ExprBlock <handle-ast-ExprBlock>` > const+
+--------+---------------------------------------------------------------+


|method-ast-AstVisitor.preVisitExprBlockFinal|

.. das:function:: AstVisitor.visitExprBlockFinal(self: AstVisitor; blk: smart_ptr<ast::ExprBlock> const)

+--------+---------------------------------------------------------------+
+argument+argument type                                                  +
+========+===============================================================+
+self    + :ref:`ast::AstVisitor <struct-ast-AstVisitor>`                +
+--------+---------------------------------------------------------------+
+blk     +smart_ptr< :ref:`ast::ExprBlock <handle-ast-ExprBlock>` > const+
+--------+---------------------------------------------------------------+


|method-ast-AstVisitor.visitExprBlockFinal|

.. das:function:: AstVisitor.preVisitExprBlockFinalExpression(self: AstVisitor; blk: smart_ptr<ast::ExprBlock> const; expr: ExpressionPtr)

+--------+---------------------------------------------------------------+
+argument+argument type                                                  +
+========+===============================================================+
+self    + :ref:`ast::AstVisitor <struct-ast-AstVisitor>`                +
+--------+---------------------------------------------------------------+
+blk     +smart_ptr< :ref:`ast::ExprBlock <handle-ast-ExprBlock>` > const+
+--------+---------------------------------------------------------------+
+expr    + :ref:`ExpressionPtr <alias-ExpressionPtr>`                    +
+--------+---------------------------------------------------------------+


|method-ast-AstVisitor.preVisitExprBlockFinalExpression|

.. das:function:: AstVisitor.visitExprBlockFinalExpression(self: AstVisitor; blk: smart_ptr<ast::ExprBlock> const; expr: ExpressionPtr)

visitExprBlockFinalExpression returns  :ref:`ExpressionPtr <alias-ExpressionPtr>` 

+--------+---------------------------------------------------------------+
+argument+argument type                                                  +
+========+===============================================================+
+self    + :ref:`ast::AstVisitor <struct-ast-AstVisitor>`                +
+--------+---------------------------------------------------------------+
+blk     +smart_ptr< :ref:`ast::ExprBlock <handle-ast-ExprBlock>` > const+
+--------+---------------------------------------------------------------+
+expr    + :ref:`ExpressionPtr <alias-ExpressionPtr>`                    +
+--------+---------------------------------------------------------------+


|method-ast-AstVisitor.visitExprBlockFinalExpression|

.. das:function:: AstVisitor.preVisitExprLet(self: AstVisitor; expr: smart_ptr<ast::ExprLet> const)

+--------+-----------------------------------------------------------+
+argument+argument type                                              +
+========+===========================================================+
+self    + :ref:`ast::AstVisitor <struct-ast-AstVisitor>`            +
+--------+-----------------------------------------------------------+
+expr    +smart_ptr< :ref:`ast::ExprLet <handle-ast-ExprLet>` > const+
+--------+-----------------------------------------------------------+


|method-ast-AstVisitor.preVisitExprLet|

.. das:function:: AstVisitor.visitExprLet(self: AstVisitor; expr: smart_ptr<ast::ExprLet> const)

visitExprLet returns  :ref:`ExpressionPtr <alias-ExpressionPtr>` 

+--------+-----------------------------------------------------------+
+argument+argument type                                              +
+========+===========================================================+
+self    + :ref:`ast::AstVisitor <struct-ast-AstVisitor>`            +
+--------+-----------------------------------------------------------+
+expr    +smart_ptr< :ref:`ast::ExprLet <handle-ast-ExprLet>` > const+
+--------+-----------------------------------------------------------+


|method-ast-AstVisitor.visitExprLet|

.. das:function:: AstVisitor.preVisitExprLetVariable(self: AstVisitor; expr: smart_ptr<ast::ExprLet> const; arg: VariablePtr; lastArg: bool const)

+--------+-----------------------------------------------------------+
+argument+argument type                                              +
+========+===========================================================+
+self    + :ref:`ast::AstVisitor <struct-ast-AstVisitor>`            +
+--------+-----------------------------------------------------------+
+expr    +smart_ptr< :ref:`ast::ExprLet <handle-ast-ExprLet>` > const+
+--------+-----------------------------------------------------------+
+arg     + :ref:`VariablePtr <alias-VariablePtr>`                    +
+--------+-----------------------------------------------------------+
+lastArg +bool const                                                 +
+--------+-----------------------------------------------------------+


|method-ast-AstVisitor.preVisitExprLetVariable|

.. das:function:: AstVisitor.visitExprLetVariable(self: AstVisitor; expr: smart_ptr<ast::ExprLet> const; arg: VariablePtr; lastArg: bool const)

visitExprLetVariable returns  :ref:`VariablePtr <alias-VariablePtr>` 

+--------+-----------------------------------------------------------+
+argument+argument type                                              +
+========+===========================================================+
+self    + :ref:`ast::AstVisitor <struct-ast-AstVisitor>`            +
+--------+-----------------------------------------------------------+
+expr    +smart_ptr< :ref:`ast::ExprLet <handle-ast-ExprLet>` > const+
+--------+-----------------------------------------------------------+
+arg     + :ref:`VariablePtr <alias-VariablePtr>`                    +
+--------+-----------------------------------------------------------+
+lastArg +bool const                                                 +
+--------+-----------------------------------------------------------+


|method-ast-AstVisitor.visitExprLetVariable|

.. das:function:: AstVisitor.preVisitExprLetVariableInit(self: AstVisitor; blk: smart_ptr<ast::ExprLet> const; arg: VariablePtr; expr: ExpressionPtr)

+--------+-----------------------------------------------------------+
+argument+argument type                                              +
+========+===========================================================+
+self    + :ref:`ast::AstVisitor <struct-ast-AstVisitor>`            +
+--------+-----------------------------------------------------------+
+blk     +smart_ptr< :ref:`ast::ExprLet <handle-ast-ExprLet>` > const+
+--------+-----------------------------------------------------------+
+arg     + :ref:`VariablePtr <alias-VariablePtr>`                    +
+--------+-----------------------------------------------------------+
+expr    + :ref:`ExpressionPtr <alias-ExpressionPtr>`                +
+--------+-----------------------------------------------------------+


|method-ast-AstVisitor.preVisitExprLetVariableInit|

.. das:function:: AstVisitor.visitExprLetVariableInit(self: AstVisitor; blk: smart_ptr<ast::ExprLet> const; arg: VariablePtr; expr: ExpressionPtr)

visitExprLetVariableInit returns  :ref:`ExpressionPtr <alias-ExpressionPtr>` 

+--------+-----------------------------------------------------------+
+argument+argument type                                              +
+========+===========================================================+
+self    + :ref:`ast::AstVisitor <struct-ast-AstVisitor>`            +
+--------+-----------------------------------------------------------+
+blk     +smart_ptr< :ref:`ast::ExprLet <handle-ast-ExprLet>` > const+
+--------+-----------------------------------------------------------+
+arg     + :ref:`VariablePtr <alias-VariablePtr>`                    +
+--------+-----------------------------------------------------------+
+expr    + :ref:`ExpressionPtr <alias-ExpressionPtr>`                +
+--------+-----------------------------------------------------------+


|method-ast-AstVisitor.visitExprLetVariableInit|

.. das:function:: AstVisitor.canVisitGlobalVariable(self: AstVisitor; arg: ast::Variable? const)

canVisitGlobalVariable returns bool

+--------+---------------------------------------------------+
+argument+argument type                                      +
+========+===================================================+
+self    + :ref:`ast::AstVisitor <struct-ast-AstVisitor>`    +
+--------+---------------------------------------------------+
+arg     + :ref:`ast::Variable <handle-ast-Variable>` ? const+
+--------+---------------------------------------------------+


|method-ast-AstVisitor.canVisitGlobalVariable|

.. das:function:: AstVisitor.preVisitGlobalLet(self: AstVisitor; prog: ProgramPtr)

+--------+------------------------------------------------+
+argument+argument type                                   +
+========+================================================+
+self    + :ref:`ast::AstVisitor <struct-ast-AstVisitor>` +
+--------+------------------------------------------------+
+prog    + :ref:`ProgramPtr <alias-ProgramPtr>`           +
+--------+------------------------------------------------+


|method-ast-AstVisitor.preVisitGlobalLet|

.. das:function:: AstVisitor.visitGlobalLet(self: AstVisitor; prog: ProgramPtr)

+--------+------------------------------------------------+
+argument+argument type                                   +
+========+================================================+
+self    + :ref:`ast::AstVisitor <struct-ast-AstVisitor>` +
+--------+------------------------------------------------+
+prog    + :ref:`ProgramPtr <alias-ProgramPtr>`           +
+--------+------------------------------------------------+


|method-ast-AstVisitor.visitGlobalLet|

.. das:function:: AstVisitor.preVisitGlobalLetVariable(self: AstVisitor; arg: VariablePtr; lastArg: bool const)

+--------+------------------------------------------------+
+argument+argument type                                   +
+========+================================================+
+self    + :ref:`ast::AstVisitor <struct-ast-AstVisitor>` +
+--------+------------------------------------------------+
+arg     + :ref:`VariablePtr <alias-VariablePtr>`         +
+--------+------------------------------------------------+
+lastArg +bool const                                      +
+--------+------------------------------------------------+


|method-ast-AstVisitor.preVisitGlobalLetVariable|

.. das:function:: AstVisitor.visitGlobalLetVariable(self: AstVisitor; arg: VariablePtr; lastArg: bool const)

visitGlobalLetVariable returns  :ref:`VariablePtr <alias-VariablePtr>` 

+--------+------------------------------------------------+
+argument+argument type                                   +
+========+================================================+
+self    + :ref:`ast::AstVisitor <struct-ast-AstVisitor>` +
+--------+------------------------------------------------+
+arg     + :ref:`VariablePtr <alias-VariablePtr>`         +
+--------+------------------------------------------------+
+lastArg +bool const                                      +
+--------+------------------------------------------------+


|method-ast-AstVisitor.visitGlobalLetVariable|

.. das:function:: AstVisitor.preVisitGlobalLetVariableInit(self: AstVisitor; arg: VariablePtr; expr: ExpressionPtr)

+--------+------------------------------------------------+
+argument+argument type                                   +
+========+================================================+
+self    + :ref:`ast::AstVisitor <struct-ast-AstVisitor>` +
+--------+------------------------------------------------+
+arg     + :ref:`VariablePtr <alias-VariablePtr>`         +
+--------+------------------------------------------------+
+expr    + :ref:`ExpressionPtr <alias-ExpressionPtr>`     +
+--------+------------------------------------------------+


|method-ast-AstVisitor.preVisitGlobalLetVariableInit|

.. das:function:: AstVisitor.visitGlobalLetVariableInit(self: AstVisitor; arg: VariablePtr; expr: ExpressionPtr)

visitGlobalLetVariableInit returns  :ref:`ExpressionPtr <alias-ExpressionPtr>` 

+--------+------------------------------------------------+
+argument+argument type                                   +
+========+================================================+
+self    + :ref:`ast::AstVisitor <struct-ast-AstVisitor>` +
+--------+------------------------------------------------+
+arg     + :ref:`VariablePtr <alias-VariablePtr>`         +
+--------+------------------------------------------------+
+expr    + :ref:`ExpressionPtr <alias-ExpressionPtr>`     +
+--------+------------------------------------------------+


|method-ast-AstVisitor.visitGlobalLetVariableInit|

.. das:function:: AstVisitor.preVisitExprStringBuilder(self: AstVisitor; expr: smart_ptr<ast::ExprStringBuilder> const)

+--------+-------------------------------------------------------------------------------+
+argument+argument type                                                                  +
+========+===============================================================================+
+self    + :ref:`ast::AstVisitor <struct-ast-AstVisitor>`                                +
+--------+-------------------------------------------------------------------------------+
+expr    +smart_ptr< :ref:`ast::ExprStringBuilder <handle-ast-ExprStringBuilder>` > const+
+--------+-------------------------------------------------------------------------------+


|method-ast-AstVisitor.preVisitExprStringBuilder|

.. das:function:: AstVisitor.visitExprStringBuilder(self: AstVisitor; expr: smart_ptr<ast::ExprStringBuilder> const)

visitExprStringBuilder returns  :ref:`ExpressionPtr <alias-ExpressionPtr>` 

+--------+-------------------------------------------------------------------------------+
+argument+argument type                                                                  +
+========+===============================================================================+
+self    + :ref:`ast::AstVisitor <struct-ast-AstVisitor>`                                +
+--------+-------------------------------------------------------------------------------+
+expr    +smart_ptr< :ref:`ast::ExprStringBuilder <handle-ast-ExprStringBuilder>` > const+
+--------+-------------------------------------------------------------------------------+


|method-ast-AstVisitor.visitExprStringBuilder|

.. das:function:: AstVisitor.preVisitExprStringBuilderElement(self: AstVisitor; expr: smart_ptr<ast::ExprStringBuilder> const; elem: ExpressionPtr; last: bool const)

+--------+-------------------------------------------------------------------------------+
+argument+argument type                                                                  +
+========+===============================================================================+
+self    + :ref:`ast::AstVisitor <struct-ast-AstVisitor>`                                +
+--------+-------------------------------------------------------------------------------+
+expr    +smart_ptr< :ref:`ast::ExprStringBuilder <handle-ast-ExprStringBuilder>` > const+
+--------+-------------------------------------------------------------------------------+
+elem    + :ref:`ExpressionPtr <alias-ExpressionPtr>`                                    +
+--------+-------------------------------------------------------------------------------+
+last    +bool const                                                                     +
+--------+-------------------------------------------------------------------------------+


|method-ast-AstVisitor.preVisitExprStringBuilderElement|

.. das:function:: AstVisitor.visitExprStringBuilderElement(self: AstVisitor; expr: smart_ptr<ast::ExprStringBuilder> const; elem: ExpressionPtr; last: bool const)

visitExprStringBuilderElement returns  :ref:`ExpressionPtr <alias-ExpressionPtr>` 

+--------+-------------------------------------------------------------------------------+
+argument+argument type                                                                  +
+========+===============================================================================+
+self    + :ref:`ast::AstVisitor <struct-ast-AstVisitor>`                                +
+--------+-------------------------------------------------------------------------------+
+expr    +smart_ptr< :ref:`ast::ExprStringBuilder <handle-ast-ExprStringBuilder>` > const+
+--------+-------------------------------------------------------------------------------+
+elem    + :ref:`ExpressionPtr <alias-ExpressionPtr>`                                    +
+--------+-------------------------------------------------------------------------------+
+last    +bool const                                                                     +
+--------+-------------------------------------------------------------------------------+


|method-ast-AstVisitor.visitExprStringBuilderElement|

.. das:function:: AstVisitor.preVisitExprNew(self: AstVisitor; expr: smart_ptr<ast::ExprNew> const)

+--------+-----------------------------------------------------------+
+argument+argument type                                              +
+========+===========================================================+
+self    + :ref:`ast::AstVisitor <struct-ast-AstVisitor>`            +
+--------+-----------------------------------------------------------+
+expr    +smart_ptr< :ref:`ast::ExprNew <handle-ast-ExprNew>` > const+
+--------+-----------------------------------------------------------+


|method-ast-AstVisitor.preVisitExprNew|

.. das:function:: AstVisitor.visitExprNew(self: AstVisitor; expr: smart_ptr<ast::ExprNew> const)

visitExprNew returns  :ref:`ExpressionPtr <alias-ExpressionPtr>` 

+--------+-----------------------------------------------------------+
+argument+argument type                                              +
+========+===========================================================+
+self    + :ref:`ast::AstVisitor <struct-ast-AstVisitor>`            +
+--------+-----------------------------------------------------------+
+expr    +smart_ptr< :ref:`ast::ExprNew <handle-ast-ExprNew>` > const+
+--------+-----------------------------------------------------------+


|method-ast-AstVisitor.visitExprNew|

.. das:function:: AstVisitor.preVisitExprNewArgument(self: AstVisitor; expr: smart_ptr<ast::ExprNew> const; arg: ExpressionPtr; last: bool const)

+--------+-----------------------------------------------------------+
+argument+argument type                                              +
+========+===========================================================+
+self    + :ref:`ast::AstVisitor <struct-ast-AstVisitor>`            +
+--------+-----------------------------------------------------------+
+expr    +smart_ptr< :ref:`ast::ExprNew <handle-ast-ExprNew>` > const+
+--------+-----------------------------------------------------------+
+arg     + :ref:`ExpressionPtr <alias-ExpressionPtr>`                +
+--------+-----------------------------------------------------------+
+last    +bool const                                                 +
+--------+-----------------------------------------------------------+


|method-ast-AstVisitor.preVisitExprNewArgument|

.. das:function:: AstVisitor.visitExprNewArgument(self: AstVisitor; expr: smart_ptr<ast::ExprNew> const; arg: ExpressionPtr; last: bool const)

visitExprNewArgument returns  :ref:`ExpressionPtr <alias-ExpressionPtr>` 

+--------+-----------------------------------------------------------+
+argument+argument type                                              +
+========+===========================================================+
+self    + :ref:`ast::AstVisitor <struct-ast-AstVisitor>`            +
+--------+-----------------------------------------------------------+
+expr    +smart_ptr< :ref:`ast::ExprNew <handle-ast-ExprNew>` > const+
+--------+-----------------------------------------------------------+
+arg     + :ref:`ExpressionPtr <alias-ExpressionPtr>`                +
+--------+-----------------------------------------------------------+
+last    +bool const                                                 +
+--------+-----------------------------------------------------------+


|method-ast-AstVisitor.visitExprNewArgument|

.. das:function:: AstVisitor.preVisitExprNamedCall(self: AstVisitor; expr: smart_ptr<ast::ExprNamedCall> const)

+--------+-----------------------------------------------------------------------+
+argument+argument type                                                          +
+========+=======================================================================+
+self    + :ref:`ast::AstVisitor <struct-ast-AstVisitor>`                        +
+--------+-----------------------------------------------------------------------+
+expr    +smart_ptr< :ref:`ast::ExprNamedCall <handle-ast-ExprNamedCall>` > const+
+--------+-----------------------------------------------------------------------+


|method-ast-AstVisitor.preVisitExprNamedCall|

.. das:function:: AstVisitor.visitExprNamedCall(self: AstVisitor; expr: smart_ptr<ast::ExprNamedCall> const)

visitExprNamedCall returns  :ref:`ExpressionPtr <alias-ExpressionPtr>` 

+--------+-----------------------------------------------------------------------+
+argument+argument type                                                          +
+========+=======================================================================+
+self    + :ref:`ast::AstVisitor <struct-ast-AstVisitor>`                        +
+--------+-----------------------------------------------------------------------+
+expr    +smart_ptr< :ref:`ast::ExprNamedCall <handle-ast-ExprNamedCall>` > const+
+--------+-----------------------------------------------------------------------+


|method-ast-AstVisitor.visitExprNamedCall|

.. das:function:: AstVisitor.preVisitExprNamedCallArgument(self: AstVisitor; expr: smart_ptr<ast::ExprNamedCall> const; arg: MakeFieldDeclPtr; last: bool const)

+--------+-----------------------------------------------------------------------+
+argument+argument type                                                          +
+========+=======================================================================+
+self    + :ref:`ast::AstVisitor <struct-ast-AstVisitor>`                        +
+--------+-----------------------------------------------------------------------+
+expr    +smart_ptr< :ref:`ast::ExprNamedCall <handle-ast-ExprNamedCall>` > const+
+--------+-----------------------------------------------------------------------+
+arg     + :ref:`MakeFieldDeclPtr <alias-MakeFieldDeclPtr>`                      +
+--------+-----------------------------------------------------------------------+
+last    +bool const                                                             +
+--------+-----------------------------------------------------------------------+


|method-ast-AstVisitor.preVisitExprNamedCallArgument|

.. das:function:: AstVisitor.visitExprNamedCallArgument(self: AstVisitor; expr: smart_ptr<ast::ExprNamedCall> const; arg: MakeFieldDeclPtr; last: bool const)

visitExprNamedCallArgument returns  :ref:`MakeFieldDeclPtr <alias-MakeFieldDeclPtr>` 

+--------+-----------------------------------------------------------------------+
+argument+argument type                                                          +
+========+=======================================================================+
+self    + :ref:`ast::AstVisitor <struct-ast-AstVisitor>`                        +
+--------+-----------------------------------------------------------------------+
+expr    +smart_ptr< :ref:`ast::ExprNamedCall <handle-ast-ExprNamedCall>` > const+
+--------+-----------------------------------------------------------------------+
+arg     + :ref:`MakeFieldDeclPtr <alias-MakeFieldDeclPtr>`                      +
+--------+-----------------------------------------------------------------------+
+last    +bool const                                                             +
+--------+-----------------------------------------------------------------------+


|method-ast-AstVisitor.visitExprNamedCallArgument|

.. das:function:: AstVisitor.preVisitExprLooksLikeCall(self: AstVisitor; expr: smart_ptr<ast::ExprLooksLikeCall> const)

+--------+-------------------------------------------------------------------------------+
+argument+argument type                                                                  +
+========+===============================================================================+
+self    + :ref:`ast::AstVisitor <struct-ast-AstVisitor>`                                +
+--------+-------------------------------------------------------------------------------+
+expr    +smart_ptr< :ref:`ast::ExprLooksLikeCall <handle-ast-ExprLooksLikeCall>` > const+
+--------+-------------------------------------------------------------------------------+


|method-ast-AstVisitor.preVisitExprLooksLikeCall|

.. das:function:: AstVisitor.visitExprLooksLikeCall(self: AstVisitor; expr: smart_ptr<ast::ExprLooksLikeCall> const)

visitExprLooksLikeCall returns  :ref:`ExpressionPtr <alias-ExpressionPtr>` 

+--------+-------------------------------------------------------------------------------+
+argument+argument type                                                                  +
+========+===============================================================================+
+self    + :ref:`ast::AstVisitor <struct-ast-AstVisitor>`                                +
+--------+-------------------------------------------------------------------------------+
+expr    +smart_ptr< :ref:`ast::ExprLooksLikeCall <handle-ast-ExprLooksLikeCall>` > const+
+--------+-------------------------------------------------------------------------------+


|method-ast-AstVisitor.visitExprLooksLikeCall|

.. das:function:: AstVisitor.preVisitExprLooksLikeCallArgument(self: AstVisitor; expr: smart_ptr<ast::ExprLooksLikeCall> const; arg: ExpressionPtr; last: bool const)

+--------+-------------------------------------------------------------------------------+
+argument+argument type                                                                  +
+========+===============================================================================+
+self    + :ref:`ast::AstVisitor <struct-ast-AstVisitor>`                                +
+--------+-------------------------------------------------------------------------------+
+expr    +smart_ptr< :ref:`ast::ExprLooksLikeCall <handle-ast-ExprLooksLikeCall>` > const+
+--------+-------------------------------------------------------------------------------+
+arg     + :ref:`ExpressionPtr <alias-ExpressionPtr>`                                    +
+--------+-------------------------------------------------------------------------------+
+last    +bool const                                                                     +
+--------+-------------------------------------------------------------------------------+


|method-ast-AstVisitor.preVisitExprLooksLikeCallArgument|

.. das:function:: AstVisitor.visitExprLooksLikeCallArgument(self: AstVisitor; expr: smart_ptr<ast::ExprLooksLikeCall> const; arg: ExpressionPtr; last: bool const)

visitExprLooksLikeCallArgument returns  :ref:`ExpressionPtr <alias-ExpressionPtr>` 

+--------+-------------------------------------------------------------------------------+
+argument+argument type                                                                  +
+========+===============================================================================+
+self    + :ref:`ast::AstVisitor <struct-ast-AstVisitor>`                                +
+--------+-------------------------------------------------------------------------------+
+expr    +smart_ptr< :ref:`ast::ExprLooksLikeCall <handle-ast-ExprLooksLikeCall>` > const+
+--------+-------------------------------------------------------------------------------+
+arg     + :ref:`ExpressionPtr <alias-ExpressionPtr>`                                    +
+--------+-------------------------------------------------------------------------------+
+last    +bool const                                                                     +
+--------+-------------------------------------------------------------------------------+


|method-ast-AstVisitor.visitExprLooksLikeCallArgument|

.. das:function:: AstVisitor.canVisitCall(self: AstVisitor; expr: ast::ExprCall? const)

canVisitCall returns bool

+--------+---------------------------------------------------+
+argument+argument type                                      +
+========+===================================================+
+self    + :ref:`ast::AstVisitor <struct-ast-AstVisitor>`    +
+--------+---------------------------------------------------+
+expr    + :ref:`ast::ExprCall <handle-ast-ExprCall>` ? const+
+--------+---------------------------------------------------+


|method-ast-AstVisitor.canVisitCall|

.. das:function:: AstVisitor.preVisitExprCall(self: AstVisitor; expr: smart_ptr<ast::ExprCall> const)

+--------+-------------------------------------------------------------+
+argument+argument type                                                +
+========+=============================================================+
+self    + :ref:`ast::AstVisitor <struct-ast-AstVisitor>`              +
+--------+-------------------------------------------------------------+
+expr    +smart_ptr< :ref:`ast::ExprCall <handle-ast-ExprCall>` > const+
+--------+-------------------------------------------------------------+


|method-ast-AstVisitor.preVisitExprCall|

.. das:function:: AstVisitor.visitExprCall(self: AstVisitor; expr: smart_ptr<ast::ExprCall> const)

visitExprCall returns  :ref:`ExpressionPtr <alias-ExpressionPtr>` 

+--------+-------------------------------------------------------------+
+argument+argument type                                                +
+========+=============================================================+
+self    + :ref:`ast::AstVisitor <struct-ast-AstVisitor>`              +
+--------+-------------------------------------------------------------+
+expr    +smart_ptr< :ref:`ast::ExprCall <handle-ast-ExprCall>` > const+
+--------+-------------------------------------------------------------+


|method-ast-AstVisitor.visitExprCall|

.. das:function:: AstVisitor.preVisitExprCallArgument(self: AstVisitor; expr: smart_ptr<ast::ExprCall> const; arg: ExpressionPtr; last: bool const)

+--------+-------------------------------------------------------------+
+argument+argument type                                                +
+========+=============================================================+
+self    + :ref:`ast::AstVisitor <struct-ast-AstVisitor>`              +
+--------+-------------------------------------------------------------+
+expr    +smart_ptr< :ref:`ast::ExprCall <handle-ast-ExprCall>` > const+
+--------+-------------------------------------------------------------+
+arg     + :ref:`ExpressionPtr <alias-ExpressionPtr>`                  +
+--------+-------------------------------------------------------------+
+last    +bool const                                                   +
+--------+-------------------------------------------------------------+


|method-ast-AstVisitor.preVisitExprCallArgument|

.. das:function:: AstVisitor.visitExprCallArgument(self: AstVisitor; expr: smart_ptr<ast::ExprCall> const; arg: ExpressionPtr; last: bool const)

visitExprCallArgument returns  :ref:`ExpressionPtr <alias-ExpressionPtr>` 

+--------+-------------------------------------------------------------+
+argument+argument type                                                +
+========+=============================================================+
+self    + :ref:`ast::AstVisitor <struct-ast-AstVisitor>`              +
+--------+-------------------------------------------------------------+
+expr    +smart_ptr< :ref:`ast::ExprCall <handle-ast-ExprCall>` > const+
+--------+-------------------------------------------------------------+
+arg     + :ref:`ExpressionPtr <alias-ExpressionPtr>`                  +
+--------+-------------------------------------------------------------+
+last    +bool const                                                   +
+--------+-------------------------------------------------------------+


|method-ast-AstVisitor.visitExprCallArgument|

.. das:function:: AstVisitor.preVisitExprNullCoalescing(self: AstVisitor; expr: smart_ptr<ast::ExprNullCoalescing> const)

+--------+---------------------------------------------------------------------------------+
+argument+argument type                                                                    +
+========+=================================================================================+
+self    + :ref:`ast::AstVisitor <struct-ast-AstVisitor>`                                  +
+--------+---------------------------------------------------------------------------------+
+expr    +smart_ptr< :ref:`ast::ExprNullCoalescing <handle-ast-ExprNullCoalescing>` > const+
+--------+---------------------------------------------------------------------------------+


|method-ast-AstVisitor.preVisitExprNullCoalescing|

.. das:function:: AstVisitor.visitExprNullCoalescing(self: AstVisitor; expr: smart_ptr<ast::ExprNullCoalescing> const)

visitExprNullCoalescing returns  :ref:`ExpressionPtr <alias-ExpressionPtr>` 

+--------+---------------------------------------------------------------------------------+
+argument+argument type                                                                    +
+========+=================================================================================+
+self    + :ref:`ast::AstVisitor <struct-ast-AstVisitor>`                                  +
+--------+---------------------------------------------------------------------------------+
+expr    +smart_ptr< :ref:`ast::ExprNullCoalescing <handle-ast-ExprNullCoalescing>` > const+
+--------+---------------------------------------------------------------------------------+


|method-ast-AstVisitor.visitExprNullCoalescing|

.. das:function:: AstVisitor.preVisitExprNullCoalescingDefault(self: AstVisitor; expr: smart_ptr<ast::ExprNullCoalescing> const; defval: ExpressionPtr)

+--------+---------------------------------------------------------------------------------+
+argument+argument type                                                                    +
+========+=================================================================================+
+self    + :ref:`ast::AstVisitor <struct-ast-AstVisitor>`                                  +
+--------+---------------------------------------------------------------------------------+
+expr    +smart_ptr< :ref:`ast::ExprNullCoalescing <handle-ast-ExprNullCoalescing>` > const+
+--------+---------------------------------------------------------------------------------+
+defval  + :ref:`ExpressionPtr <alias-ExpressionPtr>`                                      +
+--------+---------------------------------------------------------------------------------+


|method-ast-AstVisitor.preVisitExprNullCoalescingDefault|

.. das:function:: AstVisitor.preVisitExprAt(self: AstVisitor; expr: smart_ptr<ast::ExprAt> const)

+--------+---------------------------------------------------------+
+argument+argument type                                            +
+========+=========================================================+
+self    + :ref:`ast::AstVisitor <struct-ast-AstVisitor>`          +
+--------+---------------------------------------------------------+
+expr    +smart_ptr< :ref:`ast::ExprAt <handle-ast-ExprAt>` > const+
+--------+---------------------------------------------------------+


|method-ast-AstVisitor.preVisitExprAt|

.. das:function:: AstVisitor.visitExprAt(self: AstVisitor; expr: smart_ptr<ast::ExprAt> const)

visitExprAt returns  :ref:`ExpressionPtr <alias-ExpressionPtr>` 

+--------+---------------------------------------------------------+
+argument+argument type                                            +
+========+=========================================================+
+self    + :ref:`ast::AstVisitor <struct-ast-AstVisitor>`          +
+--------+---------------------------------------------------------+
+expr    +smart_ptr< :ref:`ast::ExprAt <handle-ast-ExprAt>` > const+
+--------+---------------------------------------------------------+


|method-ast-AstVisitor.visitExprAt|

.. das:function:: AstVisitor.preVisitExprAtIndex(self: AstVisitor; expr: smart_ptr<ast::ExprAt> const; index: ExpressionPtr)

+--------+---------------------------------------------------------+
+argument+argument type                                            +
+========+=========================================================+
+self    + :ref:`ast::AstVisitor <struct-ast-AstVisitor>`          +
+--------+---------------------------------------------------------+
+expr    +smart_ptr< :ref:`ast::ExprAt <handle-ast-ExprAt>` > const+
+--------+---------------------------------------------------------+
+index   + :ref:`ExpressionPtr <alias-ExpressionPtr>`              +
+--------+---------------------------------------------------------+


|method-ast-AstVisitor.preVisitExprAtIndex|

.. das:function:: AstVisitor.preVisitExprSafeAt(self: AstVisitor; expr: smart_ptr<ast::ExprSafeAt> const)

+--------+-----------------------------------------------------------------+
+argument+argument type                                                    +
+========+=================================================================+
+self    + :ref:`ast::AstVisitor <struct-ast-AstVisitor>`                  +
+--------+-----------------------------------------------------------------+
+expr    +smart_ptr< :ref:`ast::ExprSafeAt <handle-ast-ExprSafeAt>` > const+
+--------+-----------------------------------------------------------------+


|method-ast-AstVisitor.preVisitExprSafeAt|

.. das:function:: AstVisitor.visitExprSafeAt(self: AstVisitor; expr: smart_ptr<ast::ExprSafeAt> const)

visitExprSafeAt returns  :ref:`ExpressionPtr <alias-ExpressionPtr>` 

+--------+-----------------------------------------------------------------+
+argument+argument type                                                    +
+========+=================================================================+
+self    + :ref:`ast::AstVisitor <struct-ast-AstVisitor>`                  +
+--------+-----------------------------------------------------------------+
+expr    +smart_ptr< :ref:`ast::ExprSafeAt <handle-ast-ExprSafeAt>` > const+
+--------+-----------------------------------------------------------------+


|method-ast-AstVisitor.visitExprSafeAt|

.. das:function:: AstVisitor.preVisitExprSafeAtIndex(self: AstVisitor; expr: smart_ptr<ast::ExprAt> const; index: ExpressionPtr)

+--------+---------------------------------------------------------+
+argument+argument type                                            +
+========+=========================================================+
+self    + :ref:`ast::AstVisitor <struct-ast-AstVisitor>`          +
+--------+---------------------------------------------------------+
+expr    +smart_ptr< :ref:`ast::ExprAt <handle-ast-ExprAt>` > const+
+--------+---------------------------------------------------------+
+index   + :ref:`ExpressionPtr <alias-ExpressionPtr>`              +
+--------+---------------------------------------------------------+


|method-ast-AstVisitor.preVisitExprSafeAtIndex|

.. das:function:: AstVisitor.preVisitExprIs(self: AstVisitor; expr: smart_ptr<ast::ExprIs> const)

+--------+---------------------------------------------------------+
+argument+argument type                                            +
+========+=========================================================+
+self    + :ref:`ast::AstVisitor <struct-ast-AstVisitor>`          +
+--------+---------------------------------------------------------+
+expr    +smart_ptr< :ref:`ast::ExprIs <handle-ast-ExprIs>` > const+
+--------+---------------------------------------------------------+


|method-ast-AstVisitor.preVisitExprIs|

.. das:function:: AstVisitor.visitExprIs(self: AstVisitor; expr: smart_ptr<ast::ExprIs> const)

visitExprIs returns  :ref:`ExpressionPtr <alias-ExpressionPtr>` 

+--------+---------------------------------------------------------+
+argument+argument type                                            +
+========+=========================================================+
+self    + :ref:`ast::AstVisitor <struct-ast-AstVisitor>`          +
+--------+---------------------------------------------------------+
+expr    +smart_ptr< :ref:`ast::ExprIs <handle-ast-ExprIs>` > const+
+--------+---------------------------------------------------------+


|method-ast-AstVisitor.visitExprIs|

.. das:function:: AstVisitor.preVisitExprIsType(self: AstVisitor; expr: smart_ptr<ast::ExprAt> const; typeDecl: TypeDeclPtr)

+--------+---------------------------------------------------------+
+argument+argument type                                            +
+========+=========================================================+
+self    + :ref:`ast::AstVisitor <struct-ast-AstVisitor>`          +
+--------+---------------------------------------------------------+
+expr    +smart_ptr< :ref:`ast::ExprAt <handle-ast-ExprAt>` > const+
+--------+---------------------------------------------------------+
+typeDecl+ :ref:`TypeDeclPtr <alias-TypeDeclPtr>`                  +
+--------+---------------------------------------------------------+


|method-ast-AstVisitor.preVisitExprIsType|

.. das:function:: AstVisitor.preVisitExprOp2(self: AstVisitor; expr: smart_ptr<ast::ExprOp2> const)

+--------+-----------------------------------------------------------+
+argument+argument type                                              +
+========+===========================================================+
+self    + :ref:`ast::AstVisitor <struct-ast-AstVisitor>`            +
+--------+-----------------------------------------------------------+
+expr    +smart_ptr< :ref:`ast::ExprOp2 <handle-ast-ExprOp2>` > const+
+--------+-----------------------------------------------------------+


|method-ast-AstVisitor.preVisitExprOp2|

.. das:function:: AstVisitor.visitExprOp2(self: AstVisitor; expr: smart_ptr<ast::ExprOp2> const)

visitExprOp2 returns  :ref:`ExpressionPtr <alias-ExpressionPtr>` 

+--------+-----------------------------------------------------------+
+argument+argument type                                              +
+========+===========================================================+
+self    + :ref:`ast::AstVisitor <struct-ast-AstVisitor>`            +
+--------+-----------------------------------------------------------+
+expr    +smart_ptr< :ref:`ast::ExprOp2 <handle-ast-ExprOp2>` > const+
+--------+-----------------------------------------------------------+


|method-ast-AstVisitor.visitExprOp2|

.. das:function:: AstVisitor.preVisitExprOp2Right(self: AstVisitor; expr: smart_ptr<ast::ExprOp2> const; right: ExpressionPtr)

+--------+-----------------------------------------------------------+
+argument+argument type                                              +
+========+===========================================================+
+self    + :ref:`ast::AstVisitor <struct-ast-AstVisitor>`            +
+--------+-----------------------------------------------------------+
+expr    +smart_ptr< :ref:`ast::ExprOp2 <handle-ast-ExprOp2>` > const+
+--------+-----------------------------------------------------------+
+right   + :ref:`ExpressionPtr <alias-ExpressionPtr>`                +
+--------+-----------------------------------------------------------+


|method-ast-AstVisitor.preVisitExprOp2Right|

.. das:function:: AstVisitor.preVisitExprOp3(self: AstVisitor; expr: smart_ptr<ast::ExprOp3> const)

+--------+-----------------------------------------------------------+
+argument+argument type                                              +
+========+===========================================================+
+self    + :ref:`ast::AstVisitor <struct-ast-AstVisitor>`            +
+--------+-----------------------------------------------------------+
+expr    +smart_ptr< :ref:`ast::ExprOp3 <handle-ast-ExprOp3>` > const+
+--------+-----------------------------------------------------------+


|method-ast-AstVisitor.preVisitExprOp3|

.. das:function:: AstVisitor.visitExprOp3(self: AstVisitor; expr: smart_ptr<ast::ExprOp3> const)

visitExprOp3 returns  :ref:`ExpressionPtr <alias-ExpressionPtr>` 

+--------+-----------------------------------------------------------+
+argument+argument type                                              +
+========+===========================================================+
+self    + :ref:`ast::AstVisitor <struct-ast-AstVisitor>`            +
+--------+-----------------------------------------------------------+
+expr    +smart_ptr< :ref:`ast::ExprOp3 <handle-ast-ExprOp3>` > const+
+--------+-----------------------------------------------------------+


|method-ast-AstVisitor.visitExprOp3|

.. das:function:: AstVisitor.preVisitExprOp3Left(self: AstVisitor; expr: smart_ptr<ast::ExprOp3> const; left: ExpressionPtr)

+--------+-----------------------------------------------------------+
+argument+argument type                                              +
+========+===========================================================+
+self    + :ref:`ast::AstVisitor <struct-ast-AstVisitor>`            +
+--------+-----------------------------------------------------------+
+expr    +smart_ptr< :ref:`ast::ExprOp3 <handle-ast-ExprOp3>` > const+
+--------+-----------------------------------------------------------+
+left    + :ref:`ExpressionPtr <alias-ExpressionPtr>`                +
+--------+-----------------------------------------------------------+


|method-ast-AstVisitor.preVisitExprOp3Left|

.. das:function:: AstVisitor.preVisitExprOp3Right(self: AstVisitor; expr: smart_ptr<ast::ExprOp3> const; right: ExpressionPtr)

+--------+-----------------------------------------------------------+
+argument+argument type                                              +
+========+===========================================================+
+self    + :ref:`ast::AstVisitor <struct-ast-AstVisitor>`            +
+--------+-----------------------------------------------------------+
+expr    +smart_ptr< :ref:`ast::ExprOp3 <handle-ast-ExprOp3>` > const+
+--------+-----------------------------------------------------------+
+right   + :ref:`ExpressionPtr <alias-ExpressionPtr>`                +
+--------+-----------------------------------------------------------+


|method-ast-AstVisitor.preVisitExprOp3Right|

.. das:function:: AstVisitor.preVisitExprCopy(self: AstVisitor; expr: smart_ptr<ast::ExprCopy> const)

+--------+-------------------------------------------------------------+
+argument+argument type                                                +
+========+=============================================================+
+self    + :ref:`ast::AstVisitor <struct-ast-AstVisitor>`              +
+--------+-------------------------------------------------------------+
+expr    +smart_ptr< :ref:`ast::ExprCopy <handle-ast-ExprCopy>` > const+
+--------+-------------------------------------------------------------+


|method-ast-AstVisitor.preVisitExprCopy|

.. das:function:: AstVisitor.visitExprCopy(self: AstVisitor; expr: smart_ptr<ast::ExprCopy> const)

visitExprCopy returns  :ref:`ExpressionPtr <alias-ExpressionPtr>` 

+--------+-------------------------------------------------------------+
+argument+argument type                                                +
+========+=============================================================+
+self    + :ref:`ast::AstVisitor <struct-ast-AstVisitor>`              +
+--------+-------------------------------------------------------------+
+expr    +smart_ptr< :ref:`ast::ExprCopy <handle-ast-ExprCopy>` > const+
+--------+-------------------------------------------------------------+


|method-ast-AstVisitor.visitExprCopy|

.. das:function:: AstVisitor.preVisitExprCopyRight(self: AstVisitor; expr: smart_ptr<ast::ExprCopy> const; right: ExpressionPtr)

+--------+-------------------------------------------------------------+
+argument+argument type                                                +
+========+=============================================================+
+self    + :ref:`ast::AstVisitor <struct-ast-AstVisitor>`              +
+--------+-------------------------------------------------------------+
+expr    +smart_ptr< :ref:`ast::ExprCopy <handle-ast-ExprCopy>` > const+
+--------+-------------------------------------------------------------+
+right   + :ref:`ExpressionPtr <alias-ExpressionPtr>`                  +
+--------+-------------------------------------------------------------+


|method-ast-AstVisitor.preVisitExprCopyRight|

.. das:function:: AstVisitor.preVisitExprMove(self: AstVisitor; expr: smart_ptr<ast::ExprMove> const)

+--------+-------------------------------------------------------------+
+argument+argument type                                                +
+========+=============================================================+
+self    + :ref:`ast::AstVisitor <struct-ast-AstVisitor>`              +
+--------+-------------------------------------------------------------+
+expr    +smart_ptr< :ref:`ast::ExprMove <handle-ast-ExprMove>` > const+
+--------+-------------------------------------------------------------+


|method-ast-AstVisitor.preVisitExprMove|

.. das:function:: AstVisitor.visitExprMove(self: AstVisitor; expr: smart_ptr<ast::ExprMove> const)

visitExprMove returns  :ref:`ExpressionPtr <alias-ExpressionPtr>` 

+--------+-------------------------------------------------------------+
+argument+argument type                                                +
+========+=============================================================+
+self    + :ref:`ast::AstVisitor <struct-ast-AstVisitor>`              +
+--------+-------------------------------------------------------------+
+expr    +smart_ptr< :ref:`ast::ExprMove <handle-ast-ExprMove>` > const+
+--------+-------------------------------------------------------------+


|method-ast-AstVisitor.visitExprMove|

.. das:function:: AstVisitor.preVisitExprMoveRight(self: AstVisitor; expr: smart_ptr<ast::ExprMove> const; right: ExpressionPtr)

+--------+-------------------------------------------------------------+
+argument+argument type                                                +
+========+=============================================================+
+self    + :ref:`ast::AstVisitor <struct-ast-AstVisitor>`              +
+--------+-------------------------------------------------------------+
+expr    +smart_ptr< :ref:`ast::ExprMove <handle-ast-ExprMove>` > const+
+--------+-------------------------------------------------------------+
+right   + :ref:`ExpressionPtr <alias-ExpressionPtr>`                  +
+--------+-------------------------------------------------------------+


|method-ast-AstVisitor.preVisitExprMoveRight|

.. das:function:: AstVisitor.preVisitExprClone(self: AstVisitor; expr: smart_ptr<ast::ExprClone> const)

+--------+---------------------------------------------------------------+
+argument+argument type                                                  +
+========+===============================================================+
+self    + :ref:`ast::AstVisitor <struct-ast-AstVisitor>`                +
+--------+---------------------------------------------------------------+
+expr    +smart_ptr< :ref:`ast::ExprClone <handle-ast-ExprClone>` > const+
+--------+---------------------------------------------------------------+


|method-ast-AstVisitor.preVisitExprClone|

.. das:function:: AstVisitor.visitExprClone(self: AstVisitor; expr: smart_ptr<ast::ExprClone> const)

visitExprClone returns  :ref:`ExpressionPtr <alias-ExpressionPtr>` 

+--------+---------------------------------------------------------------+
+argument+argument type                                                  +
+========+===============================================================+
+self    + :ref:`ast::AstVisitor <struct-ast-AstVisitor>`                +
+--------+---------------------------------------------------------------+
+expr    +smart_ptr< :ref:`ast::ExprClone <handle-ast-ExprClone>` > const+
+--------+---------------------------------------------------------------+


|method-ast-AstVisitor.visitExprClone|

.. das:function:: AstVisitor.preVisitExprCloneRight(self: AstVisitor; expr: smart_ptr<ast::ExprClone> const; right: ExpressionPtr)

+--------+---------------------------------------------------------------+
+argument+argument type                                                  +
+========+===============================================================+
+self    + :ref:`ast::AstVisitor <struct-ast-AstVisitor>`                +
+--------+---------------------------------------------------------------+
+expr    +smart_ptr< :ref:`ast::ExprClone <handle-ast-ExprClone>` > const+
+--------+---------------------------------------------------------------+
+right   + :ref:`ExpressionPtr <alias-ExpressionPtr>`                    +
+--------+---------------------------------------------------------------+


|method-ast-AstVisitor.preVisitExprCloneRight|

.. das:function:: AstVisitor.canVisitWithAliasSubexpression(self: AstVisitor; expr: smart_ptr<ast::ExprAssume> const)

canVisitWithAliasSubexpression returns bool

+--------+-----------------------------------------------------------------+
+argument+argument type                                                    +
+========+=================================================================+
+self    + :ref:`ast::AstVisitor <struct-ast-AstVisitor>`                  +
+--------+-----------------------------------------------------------------+
+expr    +smart_ptr< :ref:`ast::ExprAssume <handle-ast-ExprAssume>` > const+
+--------+-----------------------------------------------------------------+


|method-ast-AstVisitor.canVisitWithAliasSubexpression|

.. das:function:: AstVisitor.preVisitExprAssume(self: AstVisitor; expr: smart_ptr<ast::ExprAssume> const)

+--------+-----------------------------------------------------------------+
+argument+argument type                                                    +
+========+=================================================================+
+self    + :ref:`ast::AstVisitor <struct-ast-AstVisitor>`                  +
+--------+-----------------------------------------------------------------+
+expr    +smart_ptr< :ref:`ast::ExprAssume <handle-ast-ExprAssume>` > const+
+--------+-----------------------------------------------------------------+


|method-ast-AstVisitor.preVisitExprAssume|

.. das:function:: AstVisitor.visitExprAssume(self: AstVisitor; expr: smart_ptr<ast::ExprAssume> const)

visitExprAssume returns  :ref:`ExpressionPtr <alias-ExpressionPtr>` 

+--------+-----------------------------------------------------------------+
+argument+argument type                                                    +
+========+=================================================================+
+self    + :ref:`ast::AstVisitor <struct-ast-AstVisitor>`                  +
+--------+-----------------------------------------------------------------+
+expr    +smart_ptr< :ref:`ast::ExprAssume <handle-ast-ExprAssume>` > const+
+--------+-----------------------------------------------------------------+


|method-ast-AstVisitor.visitExprAssume|

.. das:function:: AstVisitor.preVisitExprWith(self: AstVisitor; expr: smart_ptr<ast::ExprWith> const)

+--------+-------------------------------------------------------------+
+argument+argument type                                                +
+========+=============================================================+
+self    + :ref:`ast::AstVisitor <struct-ast-AstVisitor>`              +
+--------+-------------------------------------------------------------+
+expr    +smart_ptr< :ref:`ast::ExprWith <handle-ast-ExprWith>` > const+
+--------+-------------------------------------------------------------+


|method-ast-AstVisitor.preVisitExprWith|

.. das:function:: AstVisitor.visitExprWith(self: AstVisitor; expr: smart_ptr<ast::ExprWith> const)

visitExprWith returns  :ref:`ExpressionPtr <alias-ExpressionPtr>` 

+--------+-------------------------------------------------------------+
+argument+argument type                                                +
+========+=============================================================+
+self    + :ref:`ast::AstVisitor <struct-ast-AstVisitor>`              +
+--------+-------------------------------------------------------------+
+expr    +smart_ptr< :ref:`ast::ExprWith <handle-ast-ExprWith>` > const+
+--------+-------------------------------------------------------------+


|method-ast-AstVisitor.visitExprWith|

.. das:function:: AstVisitor.preVisitExprWithBody(self: AstVisitor; expr: smart_ptr<ast::ExprWith> const; right: ExpressionPtr)

+--------+-------------------------------------------------------------+
+argument+argument type                                                +
+========+=============================================================+
+self    + :ref:`ast::AstVisitor <struct-ast-AstVisitor>`              +
+--------+-------------------------------------------------------------+
+expr    +smart_ptr< :ref:`ast::ExprWith <handle-ast-ExprWith>` > const+
+--------+-------------------------------------------------------------+
+right   + :ref:`ExpressionPtr <alias-ExpressionPtr>`                  +
+--------+-------------------------------------------------------------+


|method-ast-AstVisitor.preVisitExprWithBody|

.. das:function:: AstVisitor.preVisitExprWhile(self: AstVisitor; expr: smart_ptr<ast::ExprWhile> const)

+--------+---------------------------------------------------------------+
+argument+argument type                                                  +
+========+===============================================================+
+self    + :ref:`ast::AstVisitor <struct-ast-AstVisitor>`                +
+--------+---------------------------------------------------------------+
+expr    +smart_ptr< :ref:`ast::ExprWhile <handle-ast-ExprWhile>` > const+
+--------+---------------------------------------------------------------+


|method-ast-AstVisitor.preVisitExprWhile|

.. das:function:: AstVisitor.visitExprWhile(self: AstVisitor; expr: smart_ptr<ast::ExprWhile> const)

visitExprWhile returns  :ref:`ExpressionPtr <alias-ExpressionPtr>` 

+--------+---------------------------------------------------------------+
+argument+argument type                                                  +
+========+===============================================================+
+self    + :ref:`ast::AstVisitor <struct-ast-AstVisitor>`                +
+--------+---------------------------------------------------------------+
+expr    +smart_ptr< :ref:`ast::ExprWhile <handle-ast-ExprWhile>` > const+
+--------+---------------------------------------------------------------+


|method-ast-AstVisitor.visitExprWhile|

.. das:function:: AstVisitor.preVisitExprWhileBody(self: AstVisitor; expr: smart_ptr<ast::ExprWhile> const; right: ExpressionPtr)

+--------+---------------------------------------------------------------+
+argument+argument type                                                  +
+========+===============================================================+
+self    + :ref:`ast::AstVisitor <struct-ast-AstVisitor>`                +
+--------+---------------------------------------------------------------+
+expr    +smart_ptr< :ref:`ast::ExprWhile <handle-ast-ExprWhile>` > const+
+--------+---------------------------------------------------------------+
+right   + :ref:`ExpressionPtr <alias-ExpressionPtr>`                    +
+--------+---------------------------------------------------------------+


|method-ast-AstVisitor.preVisitExprWhileBody|

.. das:function:: AstVisitor.preVisitExprTryCatch(self: AstVisitor; expr: smart_ptr<ast::ExprTryCatch> const)

+--------+---------------------------------------------------------------------+
+argument+argument type                                                        +
+========+=====================================================================+
+self    + :ref:`ast::AstVisitor <struct-ast-AstVisitor>`                      +
+--------+---------------------------------------------------------------------+
+expr    +smart_ptr< :ref:`ast::ExprTryCatch <handle-ast-ExprTryCatch>` > const+
+--------+---------------------------------------------------------------------+


|method-ast-AstVisitor.preVisitExprTryCatch|

.. das:function:: AstVisitor.visitExprTryCatch(self: AstVisitor; expr: smart_ptr<ast::ExprTryCatch> const)

visitExprTryCatch returns  :ref:`ExpressionPtr <alias-ExpressionPtr>` 

+--------+---------------------------------------------------------------------+
+argument+argument type                                                        +
+========+=====================================================================+
+self    + :ref:`ast::AstVisitor <struct-ast-AstVisitor>`                      +
+--------+---------------------------------------------------------------------+
+expr    +smart_ptr< :ref:`ast::ExprTryCatch <handle-ast-ExprTryCatch>` > const+
+--------+---------------------------------------------------------------------+


|method-ast-AstVisitor.visitExprTryCatch|

.. das:function:: AstVisitor.preVisitExprTryCatchCatch(self: AstVisitor; expr: smart_ptr<ast::ExprTryCatch> const; right: ExpressionPtr)

+--------+---------------------------------------------------------------------+
+argument+argument type                                                        +
+========+=====================================================================+
+self    + :ref:`ast::AstVisitor <struct-ast-AstVisitor>`                      +
+--------+---------------------------------------------------------------------+
+expr    +smart_ptr< :ref:`ast::ExprTryCatch <handle-ast-ExprTryCatch>` > const+
+--------+---------------------------------------------------------------------+
+right   + :ref:`ExpressionPtr <alias-ExpressionPtr>`                          +
+--------+---------------------------------------------------------------------+


|method-ast-AstVisitor.preVisitExprTryCatchCatch|

.. das:function:: AstVisitor.preVisitExprIfThenElse(self: AstVisitor; expr: smart_ptr<ast::ExprIfThenElse> const)

+--------+-------------------------------------------------------------------------+
+argument+argument type                                                            +
+========+=========================================================================+
+self    + :ref:`ast::AstVisitor <struct-ast-AstVisitor>`                          +
+--------+-------------------------------------------------------------------------+
+expr    +smart_ptr< :ref:`ast::ExprIfThenElse <handle-ast-ExprIfThenElse>` > const+
+--------+-------------------------------------------------------------------------+


|method-ast-AstVisitor.preVisitExprIfThenElse|

.. das:function:: AstVisitor.visitExprIfThenElse(self: AstVisitor; expr: smart_ptr<ast::ExprIfThenElse> const)

visitExprIfThenElse returns  :ref:`ExpressionPtr <alias-ExpressionPtr>` 

+--------+-------------------------------------------------------------------------+
+argument+argument type                                                            +
+========+=========================================================================+
+self    + :ref:`ast::AstVisitor <struct-ast-AstVisitor>`                          +
+--------+-------------------------------------------------------------------------+
+expr    +smart_ptr< :ref:`ast::ExprIfThenElse <handle-ast-ExprIfThenElse>` > const+
+--------+-------------------------------------------------------------------------+


|method-ast-AstVisitor.visitExprIfThenElse|

.. das:function:: AstVisitor.preVisitExprIfThenElseIfBlock(self: AstVisitor; expr: smart_ptr<ast::ExprIfThenElse> const; ifBlock: ExpressionPtr)

+--------+-------------------------------------------------------------------------+
+argument+argument type                                                            +
+========+=========================================================================+
+self    + :ref:`ast::AstVisitor <struct-ast-AstVisitor>`                          +
+--------+-------------------------------------------------------------------------+
+expr    +smart_ptr< :ref:`ast::ExprIfThenElse <handle-ast-ExprIfThenElse>` > const+
+--------+-------------------------------------------------------------------------+
+ifBlock + :ref:`ExpressionPtr <alias-ExpressionPtr>`                              +
+--------+-------------------------------------------------------------------------+


|method-ast-AstVisitor.preVisitExprIfThenElseIfBlock|

.. das:function:: AstVisitor.preVisitExprIfThenElseElseBlock(self: AstVisitor; expr: smart_ptr<ast::ExprIfThenElse> const; elseBlock: ExpressionPtr)

+---------+-------------------------------------------------------------------------+
+argument +argument type                                                            +
+=========+=========================================================================+
+self     + :ref:`ast::AstVisitor <struct-ast-AstVisitor>`                          +
+---------+-------------------------------------------------------------------------+
+expr     +smart_ptr< :ref:`ast::ExprIfThenElse <handle-ast-ExprIfThenElse>` > const+
+---------+-------------------------------------------------------------------------+
+elseBlock+ :ref:`ExpressionPtr <alias-ExpressionPtr>`                              +
+---------+-------------------------------------------------------------------------+


|method-ast-AstVisitor.preVisitExprIfThenElseElseBlock|

.. das:function:: AstVisitor.preVisitExprFor(self: AstVisitor; expr: smart_ptr<ast::ExprFor> const)

+--------+-----------------------------------------------------------+
+argument+argument type                                              +
+========+===========================================================+
+self    + :ref:`ast::AstVisitor <struct-ast-AstVisitor>`            +
+--------+-----------------------------------------------------------+
+expr    +smart_ptr< :ref:`ast::ExprFor <handle-ast-ExprFor>` > const+
+--------+-----------------------------------------------------------+


|method-ast-AstVisitor.preVisitExprFor|

.. das:function:: AstVisitor.visitExprFor(self: AstVisitor; expr: smart_ptr<ast::ExprFor> const)

visitExprFor returns  :ref:`ExpressionPtr <alias-ExpressionPtr>` 

+--------+-----------------------------------------------------------+
+argument+argument type                                              +
+========+===========================================================+
+self    + :ref:`ast::AstVisitor <struct-ast-AstVisitor>`            +
+--------+-----------------------------------------------------------+
+expr    +smart_ptr< :ref:`ast::ExprFor <handle-ast-ExprFor>` > const+
+--------+-----------------------------------------------------------+


|method-ast-AstVisitor.visitExprFor|

.. das:function:: AstVisitor.preVisitExprForVariable(self: AstVisitor; expr: smart_ptr<ast::ExprFor> const; svar: VariablePtr; last: bool const)

+--------+-----------------------------------------------------------+
+argument+argument type                                              +
+========+===========================================================+
+self    + :ref:`ast::AstVisitor <struct-ast-AstVisitor>`            +
+--------+-----------------------------------------------------------+
+expr    +smart_ptr< :ref:`ast::ExprFor <handle-ast-ExprFor>` > const+
+--------+-----------------------------------------------------------+
+svar    + :ref:`VariablePtr <alias-VariablePtr>`                    +
+--------+-----------------------------------------------------------+
+last    +bool const                                                 +
+--------+-----------------------------------------------------------+


|method-ast-AstVisitor.preVisitExprForVariable|

.. das:function:: AstVisitor.visitExprForVariable(self: AstVisitor; expr: smart_ptr<ast::ExprFor> const; svar: VariablePtr; last: bool const)

visitExprForVariable returns  :ref:`VariablePtr <alias-VariablePtr>` 

+--------+-----------------------------------------------------------+
+argument+argument type                                              +
+========+===========================================================+
+self    + :ref:`ast::AstVisitor <struct-ast-AstVisitor>`            +
+--------+-----------------------------------------------------------+
+expr    +smart_ptr< :ref:`ast::ExprFor <handle-ast-ExprFor>` > const+
+--------+-----------------------------------------------------------+
+svar    + :ref:`VariablePtr <alias-VariablePtr>`                    +
+--------+-----------------------------------------------------------+
+last    +bool const                                                 +
+--------+-----------------------------------------------------------+


|method-ast-AstVisitor.visitExprForVariable|

.. das:function:: AstVisitor.preVisitExprForSource(self: AstVisitor; expr: smart_ptr<ast::ExprFor> const; source: ExpressionPtr; last: bool const)

+--------+-----------------------------------------------------------+
+argument+argument type                                              +
+========+===========================================================+
+self    + :ref:`ast::AstVisitor <struct-ast-AstVisitor>`            +
+--------+-----------------------------------------------------------+
+expr    +smart_ptr< :ref:`ast::ExprFor <handle-ast-ExprFor>` > const+
+--------+-----------------------------------------------------------+
+source  + :ref:`ExpressionPtr <alias-ExpressionPtr>`                +
+--------+-----------------------------------------------------------+
+last    +bool const                                                 +
+--------+-----------------------------------------------------------+


|method-ast-AstVisitor.preVisitExprForSource|

.. das:function:: AstVisitor.visitExprForSource(self: AstVisitor; expr: smart_ptr<ast::ExprFor> const; source: ExpressionPtr; last: bool const)

visitExprForSource returns  :ref:`ExpressionPtr <alias-ExpressionPtr>` 

+--------+-----------------------------------------------------------+
+argument+argument type                                              +
+========+===========================================================+
+self    + :ref:`ast::AstVisitor <struct-ast-AstVisitor>`            +
+--------+-----------------------------------------------------------+
+expr    +smart_ptr< :ref:`ast::ExprFor <handle-ast-ExprFor>` > const+
+--------+-----------------------------------------------------------+
+source  + :ref:`ExpressionPtr <alias-ExpressionPtr>`                +
+--------+-----------------------------------------------------------+
+last    +bool const                                                 +
+--------+-----------------------------------------------------------+


|method-ast-AstVisitor.visitExprForSource|

.. das:function:: AstVisitor.preVisitExprForStack(self: AstVisitor; expr: smart_ptr<ast::ExprFor> const)

+--------+-----------------------------------------------------------+
+argument+argument type                                              +
+========+===========================================================+
+self    + :ref:`ast::AstVisitor <struct-ast-AstVisitor>`            +
+--------+-----------------------------------------------------------+
+expr    +smart_ptr< :ref:`ast::ExprFor <handle-ast-ExprFor>` > const+
+--------+-----------------------------------------------------------+


|method-ast-AstVisitor.preVisitExprForStack|

.. das:function:: AstVisitor.preVisitExprForBody(self: AstVisitor; expr: smart_ptr<ast::ExprFor> const)

+--------+-----------------------------------------------------------+
+argument+argument type                                              +
+========+===========================================================+
+self    + :ref:`ast::AstVisitor <struct-ast-AstVisitor>`            +
+--------+-----------------------------------------------------------+
+expr    +smart_ptr< :ref:`ast::ExprFor <handle-ast-ExprFor>` > const+
+--------+-----------------------------------------------------------+


|method-ast-AstVisitor.preVisitExprForBody|

.. das:function:: AstVisitor.preVisitExprMakeVariant(self: AstVisitor; expr: smart_ptr<ast::ExprMakeVariant> const)

+--------+---------------------------------------------------------------------------+
+argument+argument type                                                              +
+========+===========================================================================+
+self    + :ref:`ast::AstVisitor <struct-ast-AstVisitor>`                            +
+--------+---------------------------------------------------------------------------+
+expr    +smart_ptr< :ref:`ast::ExprMakeVariant <handle-ast-ExprMakeVariant>` > const+
+--------+---------------------------------------------------------------------------+


|method-ast-AstVisitor.preVisitExprMakeVariant|

.. das:function:: AstVisitor.visitExprMakeVariant(self: AstVisitor; expr: smart_ptr<ast::ExprMakeVariant> const)

visitExprMakeVariant returns  :ref:`ExpressionPtr <alias-ExpressionPtr>` 

+--------+---------------------------------------------------------------------------+
+argument+argument type                                                              +
+========+===========================================================================+
+self    + :ref:`ast::AstVisitor <struct-ast-AstVisitor>`                            +
+--------+---------------------------------------------------------------------------+
+expr    +smart_ptr< :ref:`ast::ExprMakeVariant <handle-ast-ExprMakeVariant>` > const+
+--------+---------------------------------------------------------------------------+


|method-ast-AstVisitor.visitExprMakeVariant|

.. das:function:: AstVisitor.preVisitExprMakeVariantField(self: AstVisitor; expr: smart_ptr<ast::ExprMakeVariant> const; index: int const; decl: MakeFieldDeclPtr; last: bool const)

+--------+---------------------------------------------------------------------------+
+argument+argument type                                                              +
+========+===========================================================================+
+self    + :ref:`ast::AstVisitor <struct-ast-AstVisitor>`                            +
+--------+---------------------------------------------------------------------------+
+expr    +smart_ptr< :ref:`ast::ExprMakeVariant <handle-ast-ExprMakeVariant>` > const+
+--------+---------------------------------------------------------------------------+
+index   +int const                                                                  +
+--------+---------------------------------------------------------------------------+
+decl    + :ref:`MakeFieldDeclPtr <alias-MakeFieldDeclPtr>`                          +
+--------+---------------------------------------------------------------------------+
+last    +bool const                                                                 +
+--------+---------------------------------------------------------------------------+


|method-ast-AstVisitor.preVisitExprMakeVariantField|

.. das:function:: AstVisitor.visitExprMakeVariantField(self: AstVisitor; expr: smart_ptr<ast::ExprMakeVariant> const; index: int const; decl: MakeFieldDeclPtr; last: bool const)

visitExprMakeVariantField returns  :ref:`MakeFieldDeclPtr <alias-MakeFieldDeclPtr>` 

+--------+---------------------------------------------------------------------------+
+argument+argument type                                                              +
+========+===========================================================================+
+self    + :ref:`ast::AstVisitor <struct-ast-AstVisitor>`                            +
+--------+---------------------------------------------------------------------------+
+expr    +smart_ptr< :ref:`ast::ExprMakeVariant <handle-ast-ExprMakeVariant>` > const+
+--------+---------------------------------------------------------------------------+
+index   +int const                                                                  +
+--------+---------------------------------------------------------------------------+
+decl    + :ref:`MakeFieldDeclPtr <alias-MakeFieldDeclPtr>`                          +
+--------+---------------------------------------------------------------------------+
+last    +bool const                                                                 +
+--------+---------------------------------------------------------------------------+


|method-ast-AstVisitor.visitExprMakeVariantField|

.. das:function:: AstVisitor.canVisitMakeStructBody(self: AstVisitor; expr: smart_ptr<ast::ExprMakeStruct> const)

canVisitMakeStructBody returns bool

+--------+-------------------------------------------------------------------------+
+argument+argument type                                                            +
+========+=========================================================================+
+self    + :ref:`ast::AstVisitor <struct-ast-AstVisitor>`                          +
+--------+-------------------------------------------------------------------------+
+expr    +smart_ptr< :ref:`ast::ExprMakeStruct <handle-ast-ExprMakeStruct>` > const+
+--------+-------------------------------------------------------------------------+


|method-ast-AstVisitor.canVisitMakeStructBody|

.. das:function:: AstVisitor.canVisitMakeStructBlock(self: AstVisitor; expr: smart_ptr<ast::ExprMakeStruct> const; blk: ExpressionPtr)

canVisitMakeStructBlock returns bool

+--------+-------------------------------------------------------------------------+
+argument+argument type                                                            +
+========+=========================================================================+
+self    + :ref:`ast::AstVisitor <struct-ast-AstVisitor>`                          +
+--------+-------------------------------------------------------------------------+
+expr    +smart_ptr< :ref:`ast::ExprMakeStruct <handle-ast-ExprMakeStruct>` > const+
+--------+-------------------------------------------------------------------------+
+blk     + :ref:`ExpressionPtr <alias-ExpressionPtr>`                              +
+--------+-------------------------------------------------------------------------+


|method-ast-AstVisitor.canVisitMakeStructBlock|

.. das:function:: AstVisitor.preVisitExprMakeStruct(self: AstVisitor; expr: smart_ptr<ast::ExprMakeStruct> const)

+--------+-------------------------------------------------------------------------+
+argument+argument type                                                            +
+========+=========================================================================+
+self    + :ref:`ast::AstVisitor <struct-ast-AstVisitor>`                          +
+--------+-------------------------------------------------------------------------+
+expr    +smart_ptr< :ref:`ast::ExprMakeStruct <handle-ast-ExprMakeStruct>` > const+
+--------+-------------------------------------------------------------------------+


|method-ast-AstVisitor.preVisitExprMakeStruct|

.. das:function:: AstVisitor.visitExprMakeStruct(self: AstVisitor; expr: smart_ptr<ast::ExprMakeStruct> const)

visitExprMakeStruct returns  :ref:`ExpressionPtr <alias-ExpressionPtr>` 

+--------+-------------------------------------------------------------------------+
+argument+argument type                                                            +
+========+=========================================================================+
+self    + :ref:`ast::AstVisitor <struct-ast-AstVisitor>`                          +
+--------+-------------------------------------------------------------------------+
+expr    +smart_ptr< :ref:`ast::ExprMakeStruct <handle-ast-ExprMakeStruct>` > const+
+--------+-------------------------------------------------------------------------+


|method-ast-AstVisitor.visitExprMakeStruct|

.. das:function:: AstVisitor.preVisitExprMakeStructIndex(self: AstVisitor; expr: smart_ptr<ast::ExprMakeStruct> const; index: int const; last: bool const)

+--------+-------------------------------------------------------------------------+
+argument+argument type                                                            +
+========+=========================================================================+
+self    + :ref:`ast::AstVisitor <struct-ast-AstVisitor>`                          +
+--------+-------------------------------------------------------------------------+
+expr    +smart_ptr< :ref:`ast::ExprMakeStruct <handle-ast-ExprMakeStruct>` > const+
+--------+-------------------------------------------------------------------------+
+index   +int const                                                                +
+--------+-------------------------------------------------------------------------+
+last    +bool const                                                               +
+--------+-------------------------------------------------------------------------+


|method-ast-AstVisitor.preVisitExprMakeStructIndex|

.. das:function:: AstVisitor.visitExprMakeStructIndex(self: AstVisitor; expr: smart_ptr<ast::ExprMakeStruct> const; index: int const; last: bool const)

+--------+-------------------------------------------------------------------------+
+argument+argument type                                                            +
+========+=========================================================================+
+self    + :ref:`ast::AstVisitor <struct-ast-AstVisitor>`                          +
+--------+-------------------------------------------------------------------------+
+expr    +smart_ptr< :ref:`ast::ExprMakeStruct <handle-ast-ExprMakeStruct>` > const+
+--------+-------------------------------------------------------------------------+
+index   +int const                                                                +
+--------+-------------------------------------------------------------------------+
+last    +bool const                                                               +
+--------+-------------------------------------------------------------------------+


|method-ast-AstVisitor.visitExprMakeStructIndex|

.. das:function:: AstVisitor.preVisitExprMakeStructField(self: AstVisitor; expr: smart_ptr<ast::ExprMakeStruct> const; index: int const; decl: MakeFieldDeclPtr; last: bool const)

+--------+-------------------------------------------------------------------------+
+argument+argument type                                                            +
+========+=========================================================================+
+self    + :ref:`ast::AstVisitor <struct-ast-AstVisitor>`                          +
+--------+-------------------------------------------------------------------------+
+expr    +smart_ptr< :ref:`ast::ExprMakeStruct <handle-ast-ExprMakeStruct>` > const+
+--------+-------------------------------------------------------------------------+
+index   +int const                                                                +
+--------+-------------------------------------------------------------------------+
+decl    + :ref:`MakeFieldDeclPtr <alias-MakeFieldDeclPtr>`                        +
+--------+-------------------------------------------------------------------------+
+last    +bool const                                                               +
+--------+-------------------------------------------------------------------------+


|method-ast-AstVisitor.preVisitExprMakeStructField|

.. das:function:: AstVisitor.visitExprMakeStructField(self: AstVisitor; expr: smart_ptr<ast::ExprMakeStruct> const; index: int const; decl: MakeFieldDeclPtr; last: bool const)

visitExprMakeStructField returns  :ref:`MakeFieldDeclPtr <alias-MakeFieldDeclPtr>` 

+--------+-------------------------------------------------------------------------+
+argument+argument type                                                            +
+========+=========================================================================+
+self    + :ref:`ast::AstVisitor <struct-ast-AstVisitor>`                          +
+--------+-------------------------------------------------------------------------+
+expr    +smart_ptr< :ref:`ast::ExprMakeStruct <handle-ast-ExprMakeStruct>` > const+
+--------+-------------------------------------------------------------------------+
+index   +int const                                                                +
+--------+-------------------------------------------------------------------------+
+decl    + :ref:`MakeFieldDeclPtr <alias-MakeFieldDeclPtr>`                        +
+--------+-------------------------------------------------------------------------+
+last    +bool const                                                               +
+--------+-------------------------------------------------------------------------+


|method-ast-AstVisitor.visitExprMakeStructField|

.. das:function:: AstVisitor.preVisitExprMakeArray(self: AstVisitor; expr: smart_ptr<ast::ExprMakeArray> const)

+--------+-----------------------------------------------------------------------+
+argument+argument type                                                          +
+========+=======================================================================+
+self    + :ref:`ast::AstVisitor <struct-ast-AstVisitor>`                        +
+--------+-----------------------------------------------------------------------+
+expr    +smart_ptr< :ref:`ast::ExprMakeArray <handle-ast-ExprMakeArray>` > const+
+--------+-----------------------------------------------------------------------+


|method-ast-AstVisitor.preVisitExprMakeArray|

.. das:function:: AstVisitor.visitExprMakeArray(self: AstVisitor; expr: smart_ptr<ast::ExprMakeArray> const)

visitExprMakeArray returns  :ref:`ExpressionPtr <alias-ExpressionPtr>` 

+--------+-----------------------------------------------------------------------+
+argument+argument type                                                          +
+========+=======================================================================+
+self    + :ref:`ast::AstVisitor <struct-ast-AstVisitor>`                        +
+--------+-----------------------------------------------------------------------+
+expr    +smart_ptr< :ref:`ast::ExprMakeArray <handle-ast-ExprMakeArray>` > const+
+--------+-----------------------------------------------------------------------+


|method-ast-AstVisitor.visitExprMakeArray|

.. das:function:: AstVisitor.preVisitExprMakeArrayIndex(self: AstVisitor; expr: smart_ptr<ast::ExprMakeArray> const; index: int const; init: ExpressionPtr; last: bool const)

+--------+-----------------------------------------------------------------------+
+argument+argument type                                                          +
+========+=======================================================================+
+self    + :ref:`ast::AstVisitor <struct-ast-AstVisitor>`                        +
+--------+-----------------------------------------------------------------------+
+expr    +smart_ptr< :ref:`ast::ExprMakeArray <handle-ast-ExprMakeArray>` > const+
+--------+-----------------------------------------------------------------------+
+index   +int const                                                              +
+--------+-----------------------------------------------------------------------+
+init    + :ref:`ExpressionPtr <alias-ExpressionPtr>`                            +
+--------+-----------------------------------------------------------------------+
+last    +bool const                                                             +
+--------+-----------------------------------------------------------------------+


|method-ast-AstVisitor.preVisitExprMakeArrayIndex|

.. das:function:: AstVisitor.visitExprMakeArrayIndex(self: AstVisitor; expr: smart_ptr<ast::ExprMakeArray> const; index: int const; init: ExpressionPtr; last: bool const)

visitExprMakeArrayIndex returns  :ref:`ExpressionPtr <alias-ExpressionPtr>` 

+--------+-----------------------------------------------------------------------+
+argument+argument type                                                          +
+========+=======================================================================+
+self    + :ref:`ast::AstVisitor <struct-ast-AstVisitor>`                        +
+--------+-----------------------------------------------------------------------+
+expr    +smart_ptr< :ref:`ast::ExprMakeArray <handle-ast-ExprMakeArray>` > const+
+--------+-----------------------------------------------------------------------+
+index   +int const                                                              +
+--------+-----------------------------------------------------------------------+
+init    + :ref:`ExpressionPtr <alias-ExpressionPtr>`                            +
+--------+-----------------------------------------------------------------------+
+last    +bool const                                                             +
+--------+-----------------------------------------------------------------------+


|method-ast-AstVisitor.visitExprMakeArrayIndex|

.. das:function:: AstVisitor.preVisitExprMakeTuple(self: AstVisitor; expr: smart_ptr<ast::ExprMakeTuple> const)

+--------+-----------------------------------------------------------------------+
+argument+argument type                                                          +
+========+=======================================================================+
+self    + :ref:`ast::AstVisitor <struct-ast-AstVisitor>`                        +
+--------+-----------------------------------------------------------------------+
+expr    +smart_ptr< :ref:`ast::ExprMakeTuple <handle-ast-ExprMakeTuple>` > const+
+--------+-----------------------------------------------------------------------+


|method-ast-AstVisitor.preVisitExprMakeTuple|

.. das:function:: AstVisitor.visitExprMakeTuple(self: AstVisitor; expr: smart_ptr<ast::ExprMakeTuple> const)

visitExprMakeTuple returns  :ref:`ExpressionPtr <alias-ExpressionPtr>` 

+--------+-----------------------------------------------------------------------+
+argument+argument type                                                          +
+========+=======================================================================+
+self    + :ref:`ast::AstVisitor <struct-ast-AstVisitor>`                        +
+--------+-----------------------------------------------------------------------+
+expr    +smart_ptr< :ref:`ast::ExprMakeTuple <handle-ast-ExprMakeTuple>` > const+
+--------+-----------------------------------------------------------------------+


|method-ast-AstVisitor.visitExprMakeTuple|

.. das:function:: AstVisitor.preVisitExprMakeTupleIndex(self: AstVisitor; expr: smart_ptr<ast::ExprMakeTuple> const; index: int const; init: ExpressionPtr; last: bool const)

+--------+-----------------------------------------------------------------------+
+argument+argument type                                                          +
+========+=======================================================================+
+self    + :ref:`ast::AstVisitor <struct-ast-AstVisitor>`                        +
+--------+-----------------------------------------------------------------------+
+expr    +smart_ptr< :ref:`ast::ExprMakeTuple <handle-ast-ExprMakeTuple>` > const+
+--------+-----------------------------------------------------------------------+
+index   +int const                                                              +
+--------+-----------------------------------------------------------------------+
+init    + :ref:`ExpressionPtr <alias-ExpressionPtr>`                            +
+--------+-----------------------------------------------------------------------+
+last    +bool const                                                             +
+--------+-----------------------------------------------------------------------+


|method-ast-AstVisitor.preVisitExprMakeTupleIndex|

.. das:function:: AstVisitor.visitExprMakeTupleIndex(self: AstVisitor; expr: smart_ptr<ast::ExprMakeTuple> const; index: int const; init: ExpressionPtr; last: bool const)

visitExprMakeTupleIndex returns  :ref:`ExpressionPtr <alias-ExpressionPtr>` 

+--------+-----------------------------------------------------------------------+
+argument+argument type                                                          +
+========+=======================================================================+
+self    + :ref:`ast::AstVisitor <struct-ast-AstVisitor>`                        +
+--------+-----------------------------------------------------------------------+
+expr    +smart_ptr< :ref:`ast::ExprMakeTuple <handle-ast-ExprMakeTuple>` > const+
+--------+-----------------------------------------------------------------------+
+index   +int const                                                              +
+--------+-----------------------------------------------------------------------+
+init    + :ref:`ExpressionPtr <alias-ExpressionPtr>`                            +
+--------+-----------------------------------------------------------------------+
+last    +bool const                                                             +
+--------+-----------------------------------------------------------------------+


|method-ast-AstVisitor.visitExprMakeTupleIndex|

.. das:function:: AstVisitor.preVisitExprArrayComprehension(self: AstVisitor; expr: smart_ptr<ast::ExprArrayComprehension> const)

+--------+-----------------------------------------------------------------------------------------+
+argument+argument type                                                                            +
+========+=========================================================================================+
+self    + :ref:`ast::AstVisitor <struct-ast-AstVisitor>`                                          +
+--------+-----------------------------------------------------------------------------------------+
+expr    +smart_ptr< :ref:`ast::ExprArrayComprehension <handle-ast-ExprArrayComprehension>` > const+
+--------+-----------------------------------------------------------------------------------------+


|method-ast-AstVisitor.preVisitExprArrayComprehension|

.. das:function:: AstVisitor.visitExprArrayComprehension(self: AstVisitor; expr: smart_ptr<ast::ExprArrayComprehension> const)

visitExprArrayComprehension returns  :ref:`ExpressionPtr <alias-ExpressionPtr>` 

+--------+-----------------------------------------------------------------------------------------+
+argument+argument type                                                                            +
+========+=========================================================================================+
+self    + :ref:`ast::AstVisitor <struct-ast-AstVisitor>`                                          +
+--------+-----------------------------------------------------------------------------------------+
+expr    +smart_ptr< :ref:`ast::ExprArrayComprehension <handle-ast-ExprArrayComprehension>` > const+
+--------+-----------------------------------------------------------------------------------------+


|method-ast-AstVisitor.visitExprArrayComprehension|

.. das:function:: AstVisitor.preVisitExprArrayComprehensionSubexpr(self: AstVisitor; expr: smart_ptr<ast::ExprArrayComprehension> const; subexrp: ExpressionPtr)

+--------+-----------------------------------------------------------------------------------------+
+argument+argument type                                                                            +
+========+=========================================================================================+
+self    + :ref:`ast::AstVisitor <struct-ast-AstVisitor>`                                          +
+--------+-----------------------------------------------------------------------------------------+
+expr    +smart_ptr< :ref:`ast::ExprArrayComprehension <handle-ast-ExprArrayComprehension>` > const+
+--------+-----------------------------------------------------------------------------------------+
+subexrp + :ref:`ExpressionPtr <alias-ExpressionPtr>`                                              +
+--------+-----------------------------------------------------------------------------------------+


|method-ast-AstVisitor.preVisitExprArrayComprehensionSubexpr|

.. das:function:: AstVisitor.preVisitExprArrayComprehensionWhere(self: AstVisitor; expr: smart_ptr<ast::ExprArrayComprehension> const; filter: ExpressionPtr)

+--------+-----------------------------------------------------------------------------------------+
+argument+argument type                                                                            +
+========+=========================================================================================+
+self    + :ref:`ast::AstVisitor <struct-ast-AstVisitor>`                                          +
+--------+-----------------------------------------------------------------------------------------+
+expr    +smart_ptr< :ref:`ast::ExprArrayComprehension <handle-ast-ExprArrayComprehension>` > const+
+--------+-----------------------------------------------------------------------------------------+
+filter  + :ref:`ExpressionPtr <alias-ExpressionPtr>`                                              +
+--------+-----------------------------------------------------------------------------------------+


|method-ast-AstVisitor.preVisitExprArrayComprehensionWhere|

.. das:function:: AstVisitor.preVisitExprTypeInfo(self: AstVisitor; expr: smart_ptr<ast::ExprTypeInfo> const)

+--------+---------------------------------------------------------------------+
+argument+argument type                                                        +
+========+=====================================================================+
+self    + :ref:`ast::AstVisitor <struct-ast-AstVisitor>`                      +
+--------+---------------------------------------------------------------------+
+expr    +smart_ptr< :ref:`ast::ExprTypeInfo <handle-ast-ExprTypeInfo>` > const+
+--------+---------------------------------------------------------------------+


|method-ast-AstVisitor.preVisitExprTypeInfo|

.. das:function:: AstVisitor.visitExprTypeInfo(self: AstVisitor; expr: smart_ptr<ast::ExprTypeInfo> const)

visitExprTypeInfo returns  :ref:`ExpressionPtr <alias-ExpressionPtr>` 

+--------+---------------------------------------------------------------------+
+argument+argument type                                                        +
+========+=====================================================================+
+self    + :ref:`ast::AstVisitor <struct-ast-AstVisitor>`                      +
+--------+---------------------------------------------------------------------+
+expr    +smart_ptr< :ref:`ast::ExprTypeInfo <handle-ast-ExprTypeInfo>` > const+
+--------+---------------------------------------------------------------------+


|method-ast-AstVisitor.visitExprTypeInfo|

.. das:function:: AstVisitor.preVisitExprPtr2Ref(self: AstVisitor; expr: smart_ptr<ast::ExprPtr2Ref> const)

+--------+-------------------------------------------------------------------+
+argument+argument type                                                      +
+========+===================================================================+
+self    + :ref:`ast::AstVisitor <struct-ast-AstVisitor>`                    +
+--------+-------------------------------------------------------------------+
+expr    +smart_ptr< :ref:`ast::ExprPtr2Ref <handle-ast-ExprPtr2Ref>` > const+
+--------+-------------------------------------------------------------------+


|method-ast-AstVisitor.preVisitExprPtr2Ref|

.. das:function:: AstVisitor.visitExprPtr2Ref(self: AstVisitor; expr: smart_ptr<ast::ExprPtr2Ref> const)

visitExprPtr2Ref returns  :ref:`ExpressionPtr <alias-ExpressionPtr>` 

+--------+-------------------------------------------------------------------+
+argument+argument type                                                      +
+========+===================================================================+
+self    + :ref:`ast::AstVisitor <struct-ast-AstVisitor>`                    +
+--------+-------------------------------------------------------------------+
+expr    +smart_ptr< :ref:`ast::ExprPtr2Ref <handle-ast-ExprPtr2Ref>` > const+
+--------+-------------------------------------------------------------------+


|method-ast-AstVisitor.visitExprPtr2Ref|

.. das:function:: AstVisitor.preVisitExprLabel(self: AstVisitor; expr: smart_ptr<ast::ExprLabel> const)

+--------+---------------------------------------------------------------+
+argument+argument type                                                  +
+========+===============================================================+
+self    + :ref:`ast::AstVisitor <struct-ast-AstVisitor>`                +
+--------+---------------------------------------------------------------+
+expr    +smart_ptr< :ref:`ast::ExprLabel <handle-ast-ExprLabel>` > const+
+--------+---------------------------------------------------------------+


|method-ast-AstVisitor.preVisitExprLabel|

.. das:function:: AstVisitor.visitExprLabel(self: AstVisitor; expr: smart_ptr<ast::ExprLabel> const)

visitExprLabel returns  :ref:`ExpressionPtr <alias-ExpressionPtr>` 

+--------+---------------------------------------------------------------+
+argument+argument type                                                  +
+========+===============================================================+
+self    + :ref:`ast::AstVisitor <struct-ast-AstVisitor>`                +
+--------+---------------------------------------------------------------+
+expr    +smart_ptr< :ref:`ast::ExprLabel <handle-ast-ExprLabel>` > const+
+--------+---------------------------------------------------------------+


|method-ast-AstVisitor.visitExprLabel|

.. das:function:: AstVisitor.preVisitExprGoto(self: AstVisitor; expr: smart_ptr<ast::ExprGoto> const)

+--------+-------------------------------------------------------------+
+argument+argument type                                                +
+========+=============================================================+
+self    + :ref:`ast::AstVisitor <struct-ast-AstVisitor>`              +
+--------+-------------------------------------------------------------+
+expr    +smart_ptr< :ref:`ast::ExprGoto <handle-ast-ExprGoto>` > const+
+--------+-------------------------------------------------------------+


|method-ast-AstVisitor.preVisitExprGoto|

.. das:function:: AstVisitor.visitExprGoto(self: AstVisitor; expr: smart_ptr<ast::ExprGoto> const)

visitExprGoto returns  :ref:`ExpressionPtr <alias-ExpressionPtr>` 

+--------+-------------------------------------------------------------+
+argument+argument type                                                +
+========+=============================================================+
+self    + :ref:`ast::AstVisitor <struct-ast-AstVisitor>`              +
+--------+-------------------------------------------------------------+
+expr    +smart_ptr< :ref:`ast::ExprGoto <handle-ast-ExprGoto>` > const+
+--------+-------------------------------------------------------------+


|method-ast-AstVisitor.visitExprGoto|

.. das:function:: AstVisitor.preVisitExprRef2Value(self: AstVisitor; expr: smart_ptr<ast::ExprRef2Value> const)

+--------+-----------------------------------------------------------------------+
+argument+argument type                                                          +
+========+=======================================================================+
+self    + :ref:`ast::AstVisitor <struct-ast-AstVisitor>`                        +
+--------+-----------------------------------------------------------------------+
+expr    +smart_ptr< :ref:`ast::ExprRef2Value <handle-ast-ExprRef2Value>` > const+
+--------+-----------------------------------------------------------------------+


|method-ast-AstVisitor.preVisitExprRef2Value|

.. das:function:: AstVisitor.visitExprRef2Value(self: AstVisitor; expr: smart_ptr<ast::ExprRef2Value> const)

visitExprRef2Value returns  :ref:`ExpressionPtr <alias-ExpressionPtr>` 

+--------+-----------------------------------------------------------------------+
+argument+argument type                                                          +
+========+=======================================================================+
+self    + :ref:`ast::AstVisitor <struct-ast-AstVisitor>`                        +
+--------+-----------------------------------------------------------------------+
+expr    +smart_ptr< :ref:`ast::ExprRef2Value <handle-ast-ExprRef2Value>` > const+
+--------+-----------------------------------------------------------------------+


|method-ast-AstVisitor.visitExprRef2Value|

.. das:function:: AstVisitor.preVisitExprRef2Ptr(self: AstVisitor; expr: smart_ptr<ast::ExprRef2Ptr> const)

+--------+-------------------------------------------------------------------+
+argument+argument type                                                      +
+========+===================================================================+
+self    + :ref:`ast::AstVisitor <struct-ast-AstVisitor>`                    +
+--------+-------------------------------------------------------------------+
+expr    +smart_ptr< :ref:`ast::ExprRef2Ptr <handle-ast-ExprRef2Ptr>` > const+
+--------+-------------------------------------------------------------------+


|method-ast-AstVisitor.preVisitExprRef2Ptr|

.. das:function:: AstVisitor.visitExprRef2Ptr(self: AstVisitor; expr: smart_ptr<ast::ExprRef2Ptr> const)

visitExprRef2Ptr returns  :ref:`ExpressionPtr <alias-ExpressionPtr>` 

+--------+-------------------------------------------------------------------+
+argument+argument type                                                      +
+========+===================================================================+
+self    + :ref:`ast::AstVisitor <struct-ast-AstVisitor>`                    +
+--------+-------------------------------------------------------------------+
+expr    +smart_ptr< :ref:`ast::ExprRef2Ptr <handle-ast-ExprRef2Ptr>` > const+
+--------+-------------------------------------------------------------------+


|method-ast-AstVisitor.visitExprRef2Ptr|

.. das:function:: AstVisitor.preVisitExprAddr(self: AstVisitor; expr: smart_ptr<ast::ExprAddr> const)

+--------+-------------------------------------------------------------+
+argument+argument type                                                +
+========+=============================================================+
+self    + :ref:`ast::AstVisitor <struct-ast-AstVisitor>`              +
+--------+-------------------------------------------------------------+
+expr    +smart_ptr< :ref:`ast::ExprAddr <handle-ast-ExprAddr>` > const+
+--------+-------------------------------------------------------------+


|method-ast-AstVisitor.preVisitExprAddr|

.. das:function:: AstVisitor.visitExprAddr(self: AstVisitor; expr: smart_ptr<ast::ExprAddr> const)

visitExprAddr returns  :ref:`ExpressionPtr <alias-ExpressionPtr>` 

+--------+-------------------------------------------------------------+
+argument+argument type                                                +
+========+=============================================================+
+self    + :ref:`ast::AstVisitor <struct-ast-AstVisitor>`              +
+--------+-------------------------------------------------------------+
+expr    +smart_ptr< :ref:`ast::ExprAddr <handle-ast-ExprAddr>` > const+
+--------+-------------------------------------------------------------+


|method-ast-AstVisitor.visitExprAddr|

.. das:function:: AstVisitor.preVisitExprAssert(self: AstVisitor; expr: smart_ptr<ast::ExprAssert> const)

+--------+-----------------------------------------------------------------+
+argument+argument type                                                    +
+========+=================================================================+
+self    + :ref:`ast::AstVisitor <struct-ast-AstVisitor>`                  +
+--------+-----------------------------------------------------------------+
+expr    +smart_ptr< :ref:`ast::ExprAssert <handle-ast-ExprAssert>` > const+
+--------+-----------------------------------------------------------------+


|method-ast-AstVisitor.preVisitExprAssert|

.. das:function:: AstVisitor.visitExprAssert(self: AstVisitor; expr: smart_ptr<ast::ExprAssert> const)

visitExprAssert returns  :ref:`ExpressionPtr <alias-ExpressionPtr>` 

+--------+-----------------------------------------------------------------+
+argument+argument type                                                    +
+========+=================================================================+
+self    + :ref:`ast::AstVisitor <struct-ast-AstVisitor>`                  +
+--------+-----------------------------------------------------------------+
+expr    +smart_ptr< :ref:`ast::ExprAssert <handle-ast-ExprAssert>` > const+
+--------+-----------------------------------------------------------------+


|method-ast-AstVisitor.visitExprAssert|

.. das:function:: AstVisitor.preVisitExprStaticAssert(self: AstVisitor; expr: smart_ptr<ast::ExprStaticAssert> const)

+--------+-----------------------------------------------------------------------------+
+argument+argument type                                                                +
+========+=============================================================================+
+self    + :ref:`ast::AstVisitor <struct-ast-AstVisitor>`                              +
+--------+-----------------------------------------------------------------------------+
+expr    +smart_ptr< :ref:`ast::ExprStaticAssert <handle-ast-ExprStaticAssert>` > const+
+--------+-----------------------------------------------------------------------------+


|method-ast-AstVisitor.preVisitExprStaticAssert|

.. das:function:: AstVisitor.visitExprStaticAssert(self: AstVisitor; expr: smart_ptr<ast::ExprStaticAssert> const)

visitExprStaticAssert returns  :ref:`ExpressionPtr <alias-ExpressionPtr>` 

+--------+-----------------------------------------------------------------------------+
+argument+argument type                                                                +
+========+=============================================================================+
+self    + :ref:`ast::AstVisitor <struct-ast-AstVisitor>`                              +
+--------+-----------------------------------------------------------------------------+
+expr    +smart_ptr< :ref:`ast::ExprStaticAssert <handle-ast-ExprStaticAssert>` > const+
+--------+-----------------------------------------------------------------------------+


|method-ast-AstVisitor.visitExprStaticAssert|

.. das:function:: AstVisitor.preVisitExprQuote(self: AstVisitor; expr: smart_ptr<ast::ExprQuote> const)

+--------+---------------------------------------------------------------+
+argument+argument type                                                  +
+========+===============================================================+
+self    + :ref:`ast::AstVisitor <struct-ast-AstVisitor>`                +
+--------+---------------------------------------------------------------+
+expr    +smart_ptr< :ref:`ast::ExprQuote <handle-ast-ExprQuote>` > const+
+--------+---------------------------------------------------------------+


|method-ast-AstVisitor.preVisitExprQuote|

.. das:function:: AstVisitor.visitExprQuote(self: AstVisitor; expr: smart_ptr<ast::ExprQuote> const)

visitExprQuote returns  :ref:`ExpressionPtr <alias-ExpressionPtr>` 

+--------+---------------------------------------------------------------+
+argument+argument type                                                  +
+========+===============================================================+
+self    + :ref:`ast::AstVisitor <struct-ast-AstVisitor>`                +
+--------+---------------------------------------------------------------+
+expr    +smart_ptr< :ref:`ast::ExprQuote <handle-ast-ExprQuote>` > const+
+--------+---------------------------------------------------------------+


|method-ast-AstVisitor.visitExprQuote|

.. das:function:: AstVisitor.preVisitExprDebug(self: AstVisitor; expr: smart_ptr<ast::ExprDebug> const)

+--------+---------------------------------------------------------------+
+argument+argument type                                                  +
+========+===============================================================+
+self    + :ref:`ast::AstVisitor <struct-ast-AstVisitor>`                +
+--------+---------------------------------------------------------------+
+expr    +smart_ptr< :ref:`ast::ExprDebug <handle-ast-ExprDebug>` > const+
+--------+---------------------------------------------------------------+


|method-ast-AstVisitor.preVisitExprDebug|

.. das:function:: AstVisitor.visitExprDebug(self: AstVisitor; expr: smart_ptr<ast::ExprDebug> const)

visitExprDebug returns  :ref:`ExpressionPtr <alias-ExpressionPtr>` 

+--------+---------------------------------------------------------------+
+argument+argument type                                                  +
+========+===============================================================+
+self    + :ref:`ast::AstVisitor <struct-ast-AstVisitor>`                +
+--------+---------------------------------------------------------------+
+expr    +smart_ptr< :ref:`ast::ExprDebug <handle-ast-ExprDebug>` > const+
+--------+---------------------------------------------------------------+


|method-ast-AstVisitor.visitExprDebug|

.. das:function:: AstVisitor.preVisitExprInvoke(self: AstVisitor; expr: smart_ptr<ast::ExprInvoke> const)

+--------+-----------------------------------------------------------------+
+argument+argument type                                                    +
+========+=================================================================+
+self    + :ref:`ast::AstVisitor <struct-ast-AstVisitor>`                  +
+--------+-----------------------------------------------------------------+
+expr    +smart_ptr< :ref:`ast::ExprInvoke <handle-ast-ExprInvoke>` > const+
+--------+-----------------------------------------------------------------+


|method-ast-AstVisitor.preVisitExprInvoke|

.. das:function:: AstVisitor.visitExprInvoke(self: AstVisitor; expr: smart_ptr<ast::ExprInvoke> const)

visitExprInvoke returns  :ref:`ExpressionPtr <alias-ExpressionPtr>` 

+--------+-----------------------------------------------------------------+
+argument+argument type                                                    +
+========+=================================================================+
+self    + :ref:`ast::AstVisitor <struct-ast-AstVisitor>`                  +
+--------+-----------------------------------------------------------------+
+expr    +smart_ptr< :ref:`ast::ExprInvoke <handle-ast-ExprInvoke>` > const+
+--------+-----------------------------------------------------------------+


|method-ast-AstVisitor.visitExprInvoke|

.. das:function:: AstVisitor.preVisitExprErase(self: AstVisitor; expr: smart_ptr<ast::ExprErase> const)

+--------+---------------------------------------------------------------+
+argument+argument type                                                  +
+========+===============================================================+
+self    + :ref:`ast::AstVisitor <struct-ast-AstVisitor>`                +
+--------+---------------------------------------------------------------+
+expr    +smart_ptr< :ref:`ast::ExprErase <handle-ast-ExprErase>` > const+
+--------+---------------------------------------------------------------+


|method-ast-AstVisitor.preVisitExprErase|

.. das:function:: AstVisitor.visitExprErase(self: AstVisitor; expr: smart_ptr<ast::ExprErase> const)

visitExprErase returns  :ref:`ExpressionPtr <alias-ExpressionPtr>` 

+--------+---------------------------------------------------------------+
+argument+argument type                                                  +
+========+===============================================================+
+self    + :ref:`ast::AstVisitor <struct-ast-AstVisitor>`                +
+--------+---------------------------------------------------------------+
+expr    +smart_ptr< :ref:`ast::ExprErase <handle-ast-ExprErase>` > const+
+--------+---------------------------------------------------------------+


|method-ast-AstVisitor.visitExprErase|

.. das:function:: AstVisitor.preVisitExprSetInsert(self: AstVisitor; expr: smart_ptr<ast::ExprSetInsert> const)

+--------+-----------------------------------------------------------------------+
+argument+argument type                                                          +
+========+=======================================================================+
+self    + :ref:`ast::AstVisitor <struct-ast-AstVisitor>`                        +
+--------+-----------------------------------------------------------------------+
+expr    +smart_ptr< :ref:`ast::ExprSetInsert <handle-ast-ExprSetInsert>` > const+
+--------+-----------------------------------------------------------------------+


|method-ast-AstVisitor.preVisitExprSetInsert|

.. das:function:: AstVisitor.visitExprSetInsert(self: AstVisitor; expr: smart_ptr<ast::ExprSetInsert> const)

visitExprSetInsert returns  :ref:`ExpressionPtr <alias-ExpressionPtr>` 

+--------+-----------------------------------------------------------------------+
+argument+argument type                                                          +
+========+=======================================================================+
+self    + :ref:`ast::AstVisitor <struct-ast-AstVisitor>`                        +
+--------+-----------------------------------------------------------------------+
+expr    +smart_ptr< :ref:`ast::ExprSetInsert <handle-ast-ExprSetInsert>` > const+
+--------+-----------------------------------------------------------------------+


|method-ast-AstVisitor.visitExprSetInsert|

.. das:function:: AstVisitor.preVisitExprFind(self: AstVisitor; expr: smart_ptr<ast::ExprFind> const)

+--------+-------------------------------------------------------------+
+argument+argument type                                                +
+========+=============================================================+
+self    + :ref:`ast::AstVisitor <struct-ast-AstVisitor>`              +
+--------+-------------------------------------------------------------+
+expr    +smart_ptr< :ref:`ast::ExprFind <handle-ast-ExprFind>` > const+
+--------+-------------------------------------------------------------+


|method-ast-AstVisitor.preVisitExprFind|

.. das:function:: AstVisitor.visitExprFind(self: AstVisitor; expr: smart_ptr<ast::ExprFind> const)

visitExprFind returns  :ref:`ExpressionPtr <alias-ExpressionPtr>` 

+--------+-------------------------------------------------------------+
+argument+argument type                                                +
+========+=============================================================+
+self    + :ref:`ast::AstVisitor <struct-ast-AstVisitor>`              +
+--------+-------------------------------------------------------------+
+expr    +smart_ptr< :ref:`ast::ExprFind <handle-ast-ExprFind>` > const+
+--------+-------------------------------------------------------------+


|method-ast-AstVisitor.visitExprFind|

.. das:function:: AstVisitor.preVisitExprKeyExists(self: AstVisitor; expr: smart_ptr<ast::ExprKeyExists> const)

+--------+-----------------------------------------------------------------------+
+argument+argument type                                                          +
+========+=======================================================================+
+self    + :ref:`ast::AstVisitor <struct-ast-AstVisitor>`                        +
+--------+-----------------------------------------------------------------------+
+expr    +smart_ptr< :ref:`ast::ExprKeyExists <handle-ast-ExprKeyExists>` > const+
+--------+-----------------------------------------------------------------------+


|method-ast-AstVisitor.preVisitExprKeyExists|

.. das:function:: AstVisitor.visitExprKeyExists(self: AstVisitor; expr: smart_ptr<ast::ExprKeyExists> const)

visitExprKeyExists returns  :ref:`ExpressionPtr <alias-ExpressionPtr>` 

+--------+-----------------------------------------------------------------------+
+argument+argument type                                                          +
+========+=======================================================================+
+self    + :ref:`ast::AstVisitor <struct-ast-AstVisitor>`                        +
+--------+-----------------------------------------------------------------------+
+expr    +smart_ptr< :ref:`ast::ExprKeyExists <handle-ast-ExprKeyExists>` > const+
+--------+-----------------------------------------------------------------------+


|method-ast-AstVisitor.visitExprKeyExists|

.. das:function:: AstVisitor.preVisitExprAscend(self: AstVisitor; expr: smart_ptr<ast::ExprAscend> const)

+--------+-----------------------------------------------------------------+
+argument+argument type                                                    +
+========+=================================================================+
+self    + :ref:`ast::AstVisitor <struct-ast-AstVisitor>`                  +
+--------+-----------------------------------------------------------------+
+expr    +smart_ptr< :ref:`ast::ExprAscend <handle-ast-ExprAscend>` > const+
+--------+-----------------------------------------------------------------+


|method-ast-AstVisitor.preVisitExprAscend|

.. das:function:: AstVisitor.visitExprAscend(self: AstVisitor; expr: smart_ptr<ast::ExprAscend> const)

visitExprAscend returns  :ref:`ExpressionPtr <alias-ExpressionPtr>` 

+--------+-----------------------------------------------------------------+
+argument+argument type                                                    +
+========+=================================================================+
+self    + :ref:`ast::AstVisitor <struct-ast-AstVisitor>`                  +
+--------+-----------------------------------------------------------------+
+expr    +smart_ptr< :ref:`ast::ExprAscend <handle-ast-ExprAscend>` > const+
+--------+-----------------------------------------------------------------+


|method-ast-AstVisitor.visitExprAscend|

.. das:function:: AstVisitor.preVisitExprCast(self: AstVisitor; expr: smart_ptr<ast::ExprCast> const)

+--------+-------------------------------------------------------------+
+argument+argument type                                                +
+========+=============================================================+
+self    + :ref:`ast::AstVisitor <struct-ast-AstVisitor>`              +
+--------+-------------------------------------------------------------+
+expr    +smart_ptr< :ref:`ast::ExprCast <handle-ast-ExprCast>` > const+
+--------+-------------------------------------------------------------+


|method-ast-AstVisitor.preVisitExprCast|

.. das:function:: AstVisitor.visitExprCast(self: AstVisitor; expr: smart_ptr<ast::ExprCast> const)

visitExprCast returns  :ref:`ExpressionPtr <alias-ExpressionPtr>` 

+--------+-------------------------------------------------------------+
+argument+argument type                                                +
+========+=============================================================+
+self    + :ref:`ast::AstVisitor <struct-ast-AstVisitor>`              +
+--------+-------------------------------------------------------------+
+expr    +smart_ptr< :ref:`ast::ExprCast <handle-ast-ExprCast>` > const+
+--------+-------------------------------------------------------------+


|method-ast-AstVisitor.visitExprCast|

.. das:function:: AstVisitor.preVisitExprDelete(self: AstVisitor; expr: smart_ptr<ast::ExprDelete> const)

+--------+-----------------------------------------------------------------+
+argument+argument type                                                    +
+========+=================================================================+
+self    + :ref:`ast::AstVisitor <struct-ast-AstVisitor>`                  +
+--------+-----------------------------------------------------------------+
+expr    +smart_ptr< :ref:`ast::ExprDelete <handle-ast-ExprDelete>` > const+
+--------+-----------------------------------------------------------------+


|method-ast-AstVisitor.preVisitExprDelete|

.. das:function:: AstVisitor.visitExprDelete(self: AstVisitor; expr: smart_ptr<ast::ExprDelete> const)

visitExprDelete returns  :ref:`ExpressionPtr <alias-ExpressionPtr>` 

+--------+-----------------------------------------------------------------+
+argument+argument type                                                    +
+========+=================================================================+
+self    + :ref:`ast::AstVisitor <struct-ast-AstVisitor>`                  +
+--------+-----------------------------------------------------------------+
+expr    +smart_ptr< :ref:`ast::ExprDelete <handle-ast-ExprDelete>` > const+
+--------+-----------------------------------------------------------------+


|method-ast-AstVisitor.visitExprDelete|

.. das:function:: AstVisitor.preVisitExprVar(self: AstVisitor; expr: smart_ptr<ast::ExprVar> const)

+--------+-----------------------------------------------------------+
+argument+argument type                                              +
+========+===========================================================+
+self    + :ref:`ast::AstVisitor <struct-ast-AstVisitor>`            +
+--------+-----------------------------------------------------------+
+expr    +smart_ptr< :ref:`ast::ExprVar <handle-ast-ExprVar>` > const+
+--------+-----------------------------------------------------------+


|method-ast-AstVisitor.preVisitExprVar|

.. das:function:: AstVisitor.visitExprVar(self: AstVisitor; expr: smart_ptr<ast::ExprVar> const)

visitExprVar returns  :ref:`ExpressionPtr <alias-ExpressionPtr>` 

+--------+-----------------------------------------------------------+
+argument+argument type                                              +
+========+===========================================================+
+self    + :ref:`ast::AstVisitor <struct-ast-AstVisitor>`            +
+--------+-----------------------------------------------------------+
+expr    +smart_ptr< :ref:`ast::ExprVar <handle-ast-ExprVar>` > const+
+--------+-----------------------------------------------------------+


|method-ast-AstVisitor.visitExprVar|

.. das:function:: AstVisitor.preVisitExprTag(self: AstVisitor; expr: smart_ptr<ast::ExprTag> const)

+--------+-----------------------------------------------------------+
+argument+argument type                                              +
+========+===========================================================+
+self    + :ref:`ast::AstVisitor <struct-ast-AstVisitor>`            +
+--------+-----------------------------------------------------------+
+expr    +smart_ptr< :ref:`ast::ExprTag <handle-ast-ExprTag>` > const+
+--------+-----------------------------------------------------------+


|method-ast-AstVisitor.preVisitExprTag|

.. das:function:: AstVisitor.preVisitExprTagValue(self: AstVisitor; expr: smart_ptr<ast::ExprTag> const; value: ExpressionPtr)

+--------+-----------------------------------------------------------+
+argument+argument type                                              +
+========+===========================================================+
+self    + :ref:`ast::AstVisitor <struct-ast-AstVisitor>`            +
+--------+-----------------------------------------------------------+
+expr    +smart_ptr< :ref:`ast::ExprTag <handle-ast-ExprTag>` > const+
+--------+-----------------------------------------------------------+
+value   + :ref:`ExpressionPtr <alias-ExpressionPtr>`                +
+--------+-----------------------------------------------------------+


|method-ast-AstVisitor.preVisitExprTagValue|

.. das:function:: AstVisitor.visitExprTag(self: AstVisitor; expr: smart_ptr<ast::ExprTag> const)

visitExprTag returns  :ref:`ExpressionPtr <alias-ExpressionPtr>` 

+--------+-----------------------------------------------------------+
+argument+argument type                                              +
+========+===========================================================+
+self    + :ref:`ast::AstVisitor <struct-ast-AstVisitor>`            +
+--------+-----------------------------------------------------------+
+expr    +smart_ptr< :ref:`ast::ExprTag <handle-ast-ExprTag>` > const+
+--------+-----------------------------------------------------------+


|method-ast-AstVisitor.visitExprTag|

.. das:function:: AstVisitor.preVisitExprField(self: AstVisitor; expr: smart_ptr<ast::ExprField> const)

+--------+---------------------------------------------------------------+
+argument+argument type                                                  +
+========+===============================================================+
+self    + :ref:`ast::AstVisitor <struct-ast-AstVisitor>`                +
+--------+---------------------------------------------------------------+
+expr    +smart_ptr< :ref:`ast::ExprField <handle-ast-ExprField>` > const+
+--------+---------------------------------------------------------------+


|method-ast-AstVisitor.preVisitExprField|

.. das:function:: AstVisitor.visitExprField(self: AstVisitor; expr: smart_ptr<ast::ExprField> const)

visitExprField returns  :ref:`ExpressionPtr <alias-ExpressionPtr>` 

+--------+---------------------------------------------------------------+
+argument+argument type                                                  +
+========+===============================================================+
+self    + :ref:`ast::AstVisitor <struct-ast-AstVisitor>`                +
+--------+---------------------------------------------------------------+
+expr    +smart_ptr< :ref:`ast::ExprField <handle-ast-ExprField>` > const+
+--------+---------------------------------------------------------------+


|method-ast-AstVisitor.visitExprField|

.. das:function:: AstVisitor.preVisitExprSafeField(self: AstVisitor; expr: smart_ptr<ast::ExprSafeField> const)

+--------+-----------------------------------------------------------------------+
+argument+argument type                                                          +
+========+=======================================================================+
+self    + :ref:`ast::AstVisitor <struct-ast-AstVisitor>`                        +
+--------+-----------------------------------------------------------------------+
+expr    +smart_ptr< :ref:`ast::ExprSafeField <handle-ast-ExprSafeField>` > const+
+--------+-----------------------------------------------------------------------+


|method-ast-AstVisitor.preVisitExprSafeField|

.. das:function:: AstVisitor.visitExprSafeField(self: AstVisitor; expr: smart_ptr<ast::ExprSafeField> const)

visitExprSafeField returns  :ref:`ExpressionPtr <alias-ExpressionPtr>` 

+--------+-----------------------------------------------------------------------+
+argument+argument type                                                          +
+========+=======================================================================+
+self    + :ref:`ast::AstVisitor <struct-ast-AstVisitor>`                        +
+--------+-----------------------------------------------------------------------+
+expr    +smart_ptr< :ref:`ast::ExprSafeField <handle-ast-ExprSafeField>` > const+
+--------+-----------------------------------------------------------------------+


|method-ast-AstVisitor.visitExprSafeField|

.. das:function:: AstVisitor.preVisitExprSwizzle(self: AstVisitor; expr: smart_ptr<ast::ExprSwizzle> const)

+--------+-------------------------------------------------------------------+
+argument+argument type                                                      +
+========+===================================================================+
+self    + :ref:`ast::AstVisitor <struct-ast-AstVisitor>`                    +
+--------+-------------------------------------------------------------------+
+expr    +smart_ptr< :ref:`ast::ExprSwizzle <handle-ast-ExprSwizzle>` > const+
+--------+-------------------------------------------------------------------+


|method-ast-AstVisitor.preVisitExprSwizzle|

.. das:function:: AstVisitor.visitExprSwizzle(self: AstVisitor; expr: smart_ptr<ast::ExprSwizzle> const)

visitExprSwizzle returns  :ref:`ExpressionPtr <alias-ExpressionPtr>` 

+--------+-------------------------------------------------------------------+
+argument+argument type                                                      +
+========+===================================================================+
+self    + :ref:`ast::AstVisitor <struct-ast-AstVisitor>`                    +
+--------+-------------------------------------------------------------------+
+expr    +smart_ptr< :ref:`ast::ExprSwizzle <handle-ast-ExprSwizzle>` > const+
+--------+-------------------------------------------------------------------+


|method-ast-AstVisitor.visitExprSwizzle|

.. das:function:: AstVisitor.preVisitExprIsVariant(self: AstVisitor; expr: smart_ptr<ast::ExprIsVariant> const)

+--------+-----------------------------------------------------------------------+
+argument+argument type                                                          +
+========+=======================================================================+
+self    + :ref:`ast::AstVisitor <struct-ast-AstVisitor>`                        +
+--------+-----------------------------------------------------------------------+
+expr    +smart_ptr< :ref:`ast::ExprIsVariant <handle-ast-ExprIsVariant>` > const+
+--------+-----------------------------------------------------------------------+


|method-ast-AstVisitor.preVisitExprIsVariant|

.. das:function:: AstVisitor.visitExprIsVariant(self: AstVisitor; expr: smart_ptr<ast::ExprIsVariant> const)

visitExprIsVariant returns  :ref:`ExpressionPtr <alias-ExpressionPtr>` 

+--------+-----------------------------------------------------------------------+
+argument+argument type                                                          +
+========+=======================================================================+
+self    + :ref:`ast::AstVisitor <struct-ast-AstVisitor>`                        +
+--------+-----------------------------------------------------------------------+
+expr    +smart_ptr< :ref:`ast::ExprIsVariant <handle-ast-ExprIsVariant>` > const+
+--------+-----------------------------------------------------------------------+


|method-ast-AstVisitor.visitExprIsVariant|

.. das:function:: AstVisitor.preVisitExprAsVariant(self: AstVisitor; expr: smart_ptr<ast::ExprAsVariant> const)

+--------+-----------------------------------------------------------------------+
+argument+argument type                                                          +
+========+=======================================================================+
+self    + :ref:`ast::AstVisitor <struct-ast-AstVisitor>`                        +
+--------+-----------------------------------------------------------------------+
+expr    +smart_ptr< :ref:`ast::ExprAsVariant <handle-ast-ExprAsVariant>` > const+
+--------+-----------------------------------------------------------------------+


|method-ast-AstVisitor.preVisitExprAsVariant|

.. das:function:: AstVisitor.visitExprAsVariant(self: AstVisitor; expr: smart_ptr<ast::ExprAsVariant> const)

visitExprAsVariant returns  :ref:`ExpressionPtr <alias-ExpressionPtr>` 

+--------+-----------------------------------------------------------------------+
+argument+argument type                                                          +
+========+=======================================================================+
+self    + :ref:`ast::AstVisitor <struct-ast-AstVisitor>`                        +
+--------+-----------------------------------------------------------------------+
+expr    +smart_ptr< :ref:`ast::ExprAsVariant <handle-ast-ExprAsVariant>` > const+
+--------+-----------------------------------------------------------------------+


|method-ast-AstVisitor.visitExprAsVariant|

.. das:function:: AstVisitor.preVisitExprSafeAsVariant(self: AstVisitor; expr: smart_ptr<ast::ExprSafeAsVariant> const)

+--------+-------------------------------------------------------------------------------+
+argument+argument type                                                                  +
+========+===============================================================================+
+self    + :ref:`ast::AstVisitor <struct-ast-AstVisitor>`                                +
+--------+-------------------------------------------------------------------------------+
+expr    +smart_ptr< :ref:`ast::ExprSafeAsVariant <handle-ast-ExprSafeAsVariant>` > const+
+--------+-------------------------------------------------------------------------------+


|method-ast-AstVisitor.preVisitExprSafeAsVariant|

.. das:function:: AstVisitor.visitExprSafeAsVariant(self: AstVisitor; expr: smart_ptr<ast::ExprSafeAsVariant> const)

visitExprSafeAsVariant returns  :ref:`ExpressionPtr <alias-ExpressionPtr>` 

+--------+-------------------------------------------------------------------------------+
+argument+argument type                                                                  +
+========+===============================================================================+
+self    + :ref:`ast::AstVisitor <struct-ast-AstVisitor>`                                +
+--------+-------------------------------------------------------------------------------+
+expr    +smart_ptr< :ref:`ast::ExprSafeAsVariant <handle-ast-ExprSafeAsVariant>` > const+
+--------+-------------------------------------------------------------------------------+


|method-ast-AstVisitor.visitExprSafeAsVariant|

.. das:function:: AstVisitor.preVisitExprOp1(self: AstVisitor; expr: smart_ptr<ast::ExprOp1> const)

+--------+-----------------------------------------------------------+
+argument+argument type                                              +
+========+===========================================================+
+self    + :ref:`ast::AstVisitor <struct-ast-AstVisitor>`            +
+--------+-----------------------------------------------------------+
+expr    +smart_ptr< :ref:`ast::ExprOp1 <handle-ast-ExprOp1>` > const+
+--------+-----------------------------------------------------------+


|method-ast-AstVisitor.preVisitExprOp1|

.. das:function:: AstVisitor.visitExprOp1(self: AstVisitor; expr: smart_ptr<ast::ExprOp1> const)

visitExprOp1 returns  :ref:`ExpressionPtr <alias-ExpressionPtr>` 

+--------+-----------------------------------------------------------+
+argument+argument type                                              +
+========+===========================================================+
+self    + :ref:`ast::AstVisitor <struct-ast-AstVisitor>`            +
+--------+-----------------------------------------------------------+
+expr    +smart_ptr< :ref:`ast::ExprOp1 <handle-ast-ExprOp1>` > const+
+--------+-----------------------------------------------------------+


|method-ast-AstVisitor.visitExprOp1|

.. das:function:: AstVisitor.preVisitExprReturn(self: AstVisitor; expr: smart_ptr<ast::ExprReturn> const)

+--------+-----------------------------------------------------------------+
+argument+argument type                                                    +
+========+=================================================================+
+self    + :ref:`ast::AstVisitor <struct-ast-AstVisitor>`                  +
+--------+-----------------------------------------------------------------+
+expr    +smart_ptr< :ref:`ast::ExprReturn <handle-ast-ExprReturn>` > const+
+--------+-----------------------------------------------------------------+


|method-ast-AstVisitor.preVisitExprReturn|

.. das:function:: AstVisitor.visitExprReturn(self: AstVisitor; expr: smart_ptr<ast::ExprReturn> const)

visitExprReturn returns  :ref:`ExpressionPtr <alias-ExpressionPtr>` 

+--------+-----------------------------------------------------------------+
+argument+argument type                                                    +
+========+=================================================================+
+self    + :ref:`ast::AstVisitor <struct-ast-AstVisitor>`                  +
+--------+-----------------------------------------------------------------+
+expr    +smart_ptr< :ref:`ast::ExprReturn <handle-ast-ExprReturn>` > const+
+--------+-----------------------------------------------------------------+


|method-ast-AstVisitor.visitExprReturn|

.. das:function:: AstVisitor.preVisitExprYield(self: AstVisitor; expr: smart_ptr<ast::ExprYield> const)

+--------+---------------------------------------------------------------+
+argument+argument type                                                  +
+========+===============================================================+
+self    + :ref:`ast::AstVisitor <struct-ast-AstVisitor>`                +
+--------+---------------------------------------------------------------+
+expr    +smart_ptr< :ref:`ast::ExprYield <handle-ast-ExprYield>` > const+
+--------+---------------------------------------------------------------+


|method-ast-AstVisitor.preVisitExprYield|

.. das:function:: AstVisitor.visitExprYield(self: AstVisitor; expr: smart_ptr<ast::ExprYield> const)

visitExprYield returns  :ref:`ExpressionPtr <alias-ExpressionPtr>` 

+--------+---------------------------------------------------------------+
+argument+argument type                                                  +
+========+===============================================================+
+self    + :ref:`ast::AstVisitor <struct-ast-AstVisitor>`                +
+--------+---------------------------------------------------------------+
+expr    +smart_ptr< :ref:`ast::ExprYield <handle-ast-ExprYield>` > const+
+--------+---------------------------------------------------------------+


|method-ast-AstVisitor.visitExprYield|

.. das:function:: AstVisitor.preVisitExprBreak(self: AstVisitor; expr: smart_ptr<ast::ExprBreak> const)

+--------+---------------------------------------------------------------+
+argument+argument type                                                  +
+========+===============================================================+
+self    + :ref:`ast::AstVisitor <struct-ast-AstVisitor>`                +
+--------+---------------------------------------------------------------+
+expr    +smart_ptr< :ref:`ast::ExprBreak <handle-ast-ExprBreak>` > const+
+--------+---------------------------------------------------------------+


|method-ast-AstVisitor.preVisitExprBreak|

.. das:function:: AstVisitor.visitExprBreak(self: AstVisitor; expr: smart_ptr<ast::ExprBreak> const)

visitExprBreak returns  :ref:`ExpressionPtr <alias-ExpressionPtr>` 

+--------+---------------------------------------------------------------+
+argument+argument type                                                  +
+========+===============================================================+
+self    + :ref:`ast::AstVisitor <struct-ast-AstVisitor>`                +
+--------+---------------------------------------------------------------+
+expr    +smart_ptr< :ref:`ast::ExprBreak <handle-ast-ExprBreak>` > const+
+--------+---------------------------------------------------------------+


|method-ast-AstVisitor.visitExprBreak|

.. das:function:: AstVisitor.preVisitExprContinue(self: AstVisitor; expr: smart_ptr<ast::ExprContinue> const)

+--------+---------------------------------------------------------------------+
+argument+argument type                                                        +
+========+=====================================================================+
+self    + :ref:`ast::AstVisitor <struct-ast-AstVisitor>`                      +
+--------+---------------------------------------------------------------------+
+expr    +smart_ptr< :ref:`ast::ExprContinue <handle-ast-ExprContinue>` > const+
+--------+---------------------------------------------------------------------+


|method-ast-AstVisitor.preVisitExprContinue|

.. das:function:: AstVisitor.visitExprContinue(self: AstVisitor; expr: smart_ptr<ast::ExprContinue> const)

visitExprContinue returns  :ref:`ExpressionPtr <alias-ExpressionPtr>` 

+--------+---------------------------------------------------------------------+
+argument+argument type                                                        +
+========+=====================================================================+
+self    + :ref:`ast::AstVisitor <struct-ast-AstVisitor>`                      +
+--------+---------------------------------------------------------------------+
+expr    +smart_ptr< :ref:`ast::ExprContinue <handle-ast-ExprContinue>` > const+
+--------+---------------------------------------------------------------------+


|method-ast-AstVisitor.visitExprContinue|

.. das:function:: AstVisitor.canVisitMakeBlockBody(self: AstVisitor; expr: smart_ptr<ast::ExprMakeBlock> const)

canVisitMakeBlockBody returns bool

+--------+-----------------------------------------------------------------------+
+argument+argument type                                                          +
+========+=======================================================================+
+self    + :ref:`ast::AstVisitor <struct-ast-AstVisitor>`                        +
+--------+-----------------------------------------------------------------------+
+expr    +smart_ptr< :ref:`ast::ExprMakeBlock <handle-ast-ExprMakeBlock>` > const+
+--------+-----------------------------------------------------------------------+


|method-ast-AstVisitor.canVisitMakeBlockBody|

.. das:function:: AstVisitor.preVisitExprMakeBlock(self: AstVisitor; expr: smart_ptr<ast::ExprMakeBlock> const)

+--------+-----------------------------------------------------------------------+
+argument+argument type                                                          +
+========+=======================================================================+
+self    + :ref:`ast::AstVisitor <struct-ast-AstVisitor>`                        +
+--------+-----------------------------------------------------------------------+
+expr    +smart_ptr< :ref:`ast::ExprMakeBlock <handle-ast-ExprMakeBlock>` > const+
+--------+-----------------------------------------------------------------------+


|method-ast-AstVisitor.preVisitExprMakeBlock|

.. das:function:: AstVisitor.visitExprMakeBlock(self: AstVisitor; expr: smart_ptr<ast::ExprMakeBlock> const)

visitExprMakeBlock returns  :ref:`ExpressionPtr <alias-ExpressionPtr>` 

+--------+-----------------------------------------------------------------------+
+argument+argument type                                                          +
+========+=======================================================================+
+self    + :ref:`ast::AstVisitor <struct-ast-AstVisitor>`                        +
+--------+-----------------------------------------------------------------------+
+expr    +smart_ptr< :ref:`ast::ExprMakeBlock <handle-ast-ExprMakeBlock>` > const+
+--------+-----------------------------------------------------------------------+


|method-ast-AstVisitor.visitExprMakeBlock|

.. das:function:: AstVisitor.preVisitExprMakeGenerator(self: AstVisitor; expr: smart_ptr<ast::ExprMakeGenerator> const)

+--------+-------------------------------------------------------------------------------+
+argument+argument type                                                                  +
+========+===============================================================================+
+self    + :ref:`ast::AstVisitor <struct-ast-AstVisitor>`                                +
+--------+-------------------------------------------------------------------------------+
+expr    +smart_ptr< :ref:`ast::ExprMakeGenerator <handle-ast-ExprMakeGenerator>` > const+
+--------+-------------------------------------------------------------------------------+


|method-ast-AstVisitor.preVisitExprMakeGenerator|

.. das:function:: AstVisitor.visitExprMakeGenerator(self: AstVisitor; expr: smart_ptr<ast::ExprMakeGenerator> const)

visitExprMakeGenerator returns  :ref:`ExpressionPtr <alias-ExpressionPtr>` 

+--------+-------------------------------------------------------------------------------+
+argument+argument type                                                                  +
+========+===============================================================================+
+self    + :ref:`ast::AstVisitor <struct-ast-AstVisitor>`                                +
+--------+-------------------------------------------------------------------------------+
+expr    +smart_ptr< :ref:`ast::ExprMakeGenerator <handle-ast-ExprMakeGenerator>` > const+
+--------+-------------------------------------------------------------------------------+


|method-ast-AstVisitor.visitExprMakeGenerator|

.. das:function:: AstVisitor.preVisitExprMemZero(self: AstVisitor; expr: smart_ptr<ast::ExprMemZero> const)

+--------+-------------------------------------------------------------------+
+argument+argument type                                                      +
+========+===================================================================+
+self    + :ref:`ast::AstVisitor <struct-ast-AstVisitor>`                    +
+--------+-------------------------------------------------------------------+
+expr    +smart_ptr< :ref:`ast::ExprMemZero <handle-ast-ExprMemZero>` > const+
+--------+-------------------------------------------------------------------+


|method-ast-AstVisitor.preVisitExprMemZero|

.. das:function:: AstVisitor.visitExprMemZero(self: AstVisitor; expr: smart_ptr<ast::ExprMemZero> const)

visitExprMemZero returns  :ref:`ExpressionPtr <alias-ExpressionPtr>` 

+--------+-------------------------------------------------------------------+
+argument+argument type                                                      +
+========+===================================================================+
+self    + :ref:`ast::AstVisitor <struct-ast-AstVisitor>`                    +
+--------+-------------------------------------------------------------------+
+expr    +smart_ptr< :ref:`ast::ExprMemZero <handle-ast-ExprMemZero>` > const+
+--------+-------------------------------------------------------------------+


|method-ast-AstVisitor.visitExprMemZero|

.. das:function:: AstVisitor.preVisitExprConst(self: AstVisitor; expr: smart_ptr<ast::ExprConst> const)

+--------+---------------------------------------------------------------+
+argument+argument type                                                  +
+========+===============================================================+
+self    + :ref:`ast::AstVisitor <struct-ast-AstVisitor>`                +
+--------+---------------------------------------------------------------+
+expr    +smart_ptr< :ref:`ast::ExprConst <handle-ast-ExprConst>` > const+
+--------+---------------------------------------------------------------+


|method-ast-AstVisitor.preVisitExprConst|

.. das:function:: AstVisitor.visitExprConst(self: AstVisitor; expr: smart_ptr<ast::ExprConst> const)

visitExprConst returns  :ref:`ExpressionPtr <alias-ExpressionPtr>` 

+--------+---------------------------------------------------------------+
+argument+argument type                                                  +
+========+===============================================================+
+self    + :ref:`ast::AstVisitor <struct-ast-AstVisitor>`                +
+--------+---------------------------------------------------------------+
+expr    +smart_ptr< :ref:`ast::ExprConst <handle-ast-ExprConst>` > const+
+--------+---------------------------------------------------------------+


|method-ast-AstVisitor.visitExprConst|

.. das:function:: AstVisitor.preVisitExprConstPtr(self: AstVisitor; expr: smart_ptr<ast::ExprConstPtr> const)

+--------+---------------------------------------------------------------------+
+argument+argument type                                                        +
+========+=====================================================================+
+self    + :ref:`ast::AstVisitor <struct-ast-AstVisitor>`                      +
+--------+---------------------------------------------------------------------+
+expr    +smart_ptr< :ref:`ast::ExprConstPtr <handle-ast-ExprConstPtr>` > const+
+--------+---------------------------------------------------------------------+


|method-ast-AstVisitor.preVisitExprConstPtr|

.. das:function:: AstVisitor.visitExprConstPtr(self: AstVisitor; expr: smart_ptr<ast::ExprConstPtr> const)

visitExprConstPtr returns  :ref:`ExpressionPtr <alias-ExpressionPtr>` 

+--------+---------------------------------------------------------------------+
+argument+argument type                                                        +
+========+=====================================================================+
+self    + :ref:`ast::AstVisitor <struct-ast-AstVisitor>`                      +
+--------+---------------------------------------------------------------------+
+expr    +smart_ptr< :ref:`ast::ExprConstPtr <handle-ast-ExprConstPtr>` > const+
+--------+---------------------------------------------------------------------+


|method-ast-AstVisitor.visitExprConstPtr|

.. das:function:: AstVisitor.preVisitExprConstEnumeration(self: AstVisitor; expr: smart_ptr<ast::ExprConstEnumeration> const)

+--------+-------------------------------------------------------------------------------------+
+argument+argument type                                                                        +
+========+=====================================================================================+
+self    + :ref:`ast::AstVisitor <struct-ast-AstVisitor>`                                      +
+--------+-------------------------------------------------------------------------------------+
+expr    +smart_ptr< :ref:`ast::ExprConstEnumeration <handle-ast-ExprConstEnumeration>` > const+
+--------+-------------------------------------------------------------------------------------+


|method-ast-AstVisitor.preVisitExprConstEnumeration|

.. das:function:: AstVisitor.visitExprConstEnumeration(self: AstVisitor; expr: smart_ptr<ast::ExprConstEnumeration> const)

visitExprConstEnumeration returns  :ref:`ExpressionPtr <alias-ExpressionPtr>` 

+--------+-------------------------------------------------------------------------------------+
+argument+argument type                                                                        +
+========+=====================================================================================+
+self    + :ref:`ast::AstVisitor <struct-ast-AstVisitor>`                                      +
+--------+-------------------------------------------------------------------------------------+
+expr    +smart_ptr< :ref:`ast::ExprConstEnumeration <handle-ast-ExprConstEnumeration>` > const+
+--------+-------------------------------------------------------------------------------------+


|method-ast-AstVisitor.visitExprConstEnumeration|

.. das:function:: AstVisitor.preVisitExprConstBitfield(self: AstVisitor; expr: smart_ptr<ast::ExprConstBitfield> const)

+--------+-------------------------------------------------------------------------------+
+argument+argument type                                                                  +
+========+===============================================================================+
+self    + :ref:`ast::AstVisitor <struct-ast-AstVisitor>`                                +
+--------+-------------------------------------------------------------------------------+
+expr    +smart_ptr< :ref:`ast::ExprConstBitfield <handle-ast-ExprConstBitfield>` > const+
+--------+-------------------------------------------------------------------------------+


|method-ast-AstVisitor.preVisitExprConstBitfield|

.. das:function:: AstVisitor.visitExprConstBitfield(self: AstVisitor; expr: smart_ptr<ast::ExprConstBitfield> const)

visitExprConstBitfield returns  :ref:`ExpressionPtr <alias-ExpressionPtr>` 

+--------+-------------------------------------------------------------------------------+
+argument+argument type                                                                  +
+========+===============================================================================+
+self    + :ref:`ast::AstVisitor <struct-ast-AstVisitor>`                                +
+--------+-------------------------------------------------------------------------------+
+expr    +smart_ptr< :ref:`ast::ExprConstBitfield <handle-ast-ExprConstBitfield>` > const+
+--------+-------------------------------------------------------------------------------+


|method-ast-AstVisitor.visitExprConstBitfield|

.. das:function:: AstVisitor.preVisitExprConstInt8(self: AstVisitor; expr: smart_ptr<ast::ExprConstInt8> const)

+--------+-----------------------------------------------------------------------+
+argument+argument type                                                          +
+========+=======================================================================+
+self    + :ref:`ast::AstVisitor <struct-ast-AstVisitor>`                        +
+--------+-----------------------------------------------------------------------+
+expr    +smart_ptr< :ref:`ast::ExprConstInt8 <handle-ast-ExprConstInt8>` > const+
+--------+-----------------------------------------------------------------------+


|method-ast-AstVisitor.preVisitExprConstInt8|

.. das:function:: AstVisitor.visitExprConstInt8(self: AstVisitor; expr: smart_ptr<ast::ExprConstInt8> const)

visitExprConstInt8 returns  :ref:`ExpressionPtr <alias-ExpressionPtr>` 

+--------+-----------------------------------------------------------------------+
+argument+argument type                                                          +
+========+=======================================================================+
+self    + :ref:`ast::AstVisitor <struct-ast-AstVisitor>`                        +
+--------+-----------------------------------------------------------------------+
+expr    +smart_ptr< :ref:`ast::ExprConstInt8 <handle-ast-ExprConstInt8>` > const+
+--------+-----------------------------------------------------------------------+


|method-ast-AstVisitor.visitExprConstInt8|

.. das:function:: AstVisitor.preVisitExprConstInt16(self: AstVisitor; expr: smart_ptr<ast::ExprConstInt16> const)

+--------+-------------------------------------------------------------------------+
+argument+argument type                                                            +
+========+=========================================================================+
+self    + :ref:`ast::AstVisitor <struct-ast-AstVisitor>`                          +
+--------+-------------------------------------------------------------------------+
+expr    +smart_ptr< :ref:`ast::ExprConstInt16 <handle-ast-ExprConstInt16>` > const+
+--------+-------------------------------------------------------------------------+


|method-ast-AstVisitor.preVisitExprConstInt16|

.. das:function:: AstVisitor.visitExprConstInt16(self: AstVisitor; expr: smart_ptr<ast::ExprConstInt16> const)

visitExprConstInt16 returns  :ref:`ExpressionPtr <alias-ExpressionPtr>` 

+--------+-------------------------------------------------------------------------+
+argument+argument type                                                            +
+========+=========================================================================+
+self    + :ref:`ast::AstVisitor <struct-ast-AstVisitor>`                          +
+--------+-------------------------------------------------------------------------+
+expr    +smart_ptr< :ref:`ast::ExprConstInt16 <handle-ast-ExprConstInt16>` > const+
+--------+-------------------------------------------------------------------------+


|method-ast-AstVisitor.visitExprConstInt16|

.. das:function:: AstVisitor.preVisitExprConstInt64(self: AstVisitor; expr: smart_ptr<ast::ExprConstInt64> const)

+--------+-------------------------------------------------------------------------+
+argument+argument type                                                            +
+========+=========================================================================+
+self    + :ref:`ast::AstVisitor <struct-ast-AstVisitor>`                          +
+--------+-------------------------------------------------------------------------+
+expr    +smart_ptr< :ref:`ast::ExprConstInt64 <handle-ast-ExprConstInt64>` > const+
+--------+-------------------------------------------------------------------------+


|method-ast-AstVisitor.preVisitExprConstInt64|

.. das:function:: AstVisitor.visitExprConstInt64(self: AstVisitor; expr: smart_ptr<ast::ExprConstInt64> const)

visitExprConstInt64 returns  :ref:`ExpressionPtr <alias-ExpressionPtr>` 

+--------+-------------------------------------------------------------------------+
+argument+argument type                                                            +
+========+=========================================================================+
+self    + :ref:`ast::AstVisitor <struct-ast-AstVisitor>`                          +
+--------+-------------------------------------------------------------------------+
+expr    +smart_ptr< :ref:`ast::ExprConstInt64 <handle-ast-ExprConstInt64>` > const+
+--------+-------------------------------------------------------------------------+


|method-ast-AstVisitor.visitExprConstInt64|

.. das:function:: AstVisitor.preVisitExprConstInt(self: AstVisitor; expr: smart_ptr<ast::ExprConstInt> const)

+--------+---------------------------------------------------------------------+
+argument+argument type                                                        +
+========+=====================================================================+
+self    + :ref:`ast::AstVisitor <struct-ast-AstVisitor>`                      +
+--------+---------------------------------------------------------------------+
+expr    +smart_ptr< :ref:`ast::ExprConstInt <handle-ast-ExprConstInt>` > const+
+--------+---------------------------------------------------------------------+


|method-ast-AstVisitor.preVisitExprConstInt|

.. das:function:: AstVisitor.visitExprConstInt(self: AstVisitor; expr: smart_ptr<ast::ExprConstInt> const)

visitExprConstInt returns  :ref:`ExpressionPtr <alias-ExpressionPtr>` 

+--------+---------------------------------------------------------------------+
+argument+argument type                                                        +
+========+=====================================================================+
+self    + :ref:`ast::AstVisitor <struct-ast-AstVisitor>`                      +
+--------+---------------------------------------------------------------------+
+expr    +smart_ptr< :ref:`ast::ExprConstInt <handle-ast-ExprConstInt>` > const+
+--------+---------------------------------------------------------------------+


|method-ast-AstVisitor.visitExprConstInt|

.. das:function:: AstVisitor.preVisitExprConstInt2(self: AstVisitor; expr: smart_ptr<ast::ExprConstInt2> const)

+--------+-----------------------------------------------------------------------+
+argument+argument type                                                          +
+========+=======================================================================+
+self    + :ref:`ast::AstVisitor <struct-ast-AstVisitor>`                        +
+--------+-----------------------------------------------------------------------+
+expr    +smart_ptr< :ref:`ast::ExprConstInt2 <handle-ast-ExprConstInt2>` > const+
+--------+-----------------------------------------------------------------------+


|method-ast-AstVisitor.preVisitExprConstInt2|

.. das:function:: AstVisitor.visitExprConstInt2(self: AstVisitor; expr: smart_ptr<ast::ExprConstInt2> const)

visitExprConstInt2 returns  :ref:`ExpressionPtr <alias-ExpressionPtr>` 

+--------+-----------------------------------------------------------------------+
+argument+argument type                                                          +
+========+=======================================================================+
+self    + :ref:`ast::AstVisitor <struct-ast-AstVisitor>`                        +
+--------+-----------------------------------------------------------------------+
+expr    +smart_ptr< :ref:`ast::ExprConstInt2 <handle-ast-ExprConstInt2>` > const+
+--------+-----------------------------------------------------------------------+


|method-ast-AstVisitor.visitExprConstInt2|

.. das:function:: AstVisitor.preVisitExprConstInt3(self: AstVisitor; expr: smart_ptr<ast::ExprConstInt3> const)

+--------+-----------------------------------------------------------------------+
+argument+argument type                                                          +
+========+=======================================================================+
+self    + :ref:`ast::AstVisitor <struct-ast-AstVisitor>`                        +
+--------+-----------------------------------------------------------------------+
+expr    +smart_ptr< :ref:`ast::ExprConstInt3 <handle-ast-ExprConstInt3>` > const+
+--------+-----------------------------------------------------------------------+


|method-ast-AstVisitor.preVisitExprConstInt3|

.. das:function:: AstVisitor.visitExprConstInt3(self: AstVisitor; expr: smart_ptr<ast::ExprConstInt3> const)

visitExprConstInt3 returns  :ref:`ExpressionPtr <alias-ExpressionPtr>` 

+--------+-----------------------------------------------------------------------+
+argument+argument type                                                          +
+========+=======================================================================+
+self    + :ref:`ast::AstVisitor <struct-ast-AstVisitor>`                        +
+--------+-----------------------------------------------------------------------+
+expr    +smart_ptr< :ref:`ast::ExprConstInt3 <handle-ast-ExprConstInt3>` > const+
+--------+-----------------------------------------------------------------------+


|method-ast-AstVisitor.visitExprConstInt3|

.. das:function:: AstVisitor.preVisitExprConstInt4(self: AstVisitor; expr: smart_ptr<ast::ExprConstInt4> const)

+--------+-----------------------------------------------------------------------+
+argument+argument type                                                          +
+========+=======================================================================+
+self    + :ref:`ast::AstVisitor <struct-ast-AstVisitor>`                        +
+--------+-----------------------------------------------------------------------+
+expr    +smart_ptr< :ref:`ast::ExprConstInt4 <handle-ast-ExprConstInt4>` > const+
+--------+-----------------------------------------------------------------------+


|method-ast-AstVisitor.preVisitExprConstInt4|

.. das:function:: AstVisitor.visitExprConstInt4(self: AstVisitor; expr: smart_ptr<ast::ExprConstInt4> const)

visitExprConstInt4 returns  :ref:`ExpressionPtr <alias-ExpressionPtr>` 

+--------+-----------------------------------------------------------------------+
+argument+argument type                                                          +
+========+=======================================================================+
+self    + :ref:`ast::AstVisitor <struct-ast-AstVisitor>`                        +
+--------+-----------------------------------------------------------------------+
+expr    +smart_ptr< :ref:`ast::ExprConstInt4 <handle-ast-ExprConstInt4>` > const+
+--------+-----------------------------------------------------------------------+


|method-ast-AstVisitor.visitExprConstInt4|

.. das:function:: AstVisitor.preVisitExprConstUInt8(self: AstVisitor; expr: smart_ptr<ast::ExprConstUInt8> const)

+--------+-------------------------------------------------------------------------+
+argument+argument type                                                            +
+========+=========================================================================+
+self    + :ref:`ast::AstVisitor <struct-ast-AstVisitor>`                          +
+--------+-------------------------------------------------------------------------+
+expr    +smart_ptr< :ref:`ast::ExprConstUInt8 <handle-ast-ExprConstUInt8>` > const+
+--------+-------------------------------------------------------------------------+


|method-ast-AstVisitor.preVisitExprConstUInt8|

.. das:function:: AstVisitor.visitExprConstUInt8(self: AstVisitor; expr: smart_ptr<ast::ExprConstUInt8> const)

visitExprConstUInt8 returns  :ref:`ExpressionPtr <alias-ExpressionPtr>` 

+--------+-------------------------------------------------------------------------+
+argument+argument type                                                            +
+========+=========================================================================+
+self    + :ref:`ast::AstVisitor <struct-ast-AstVisitor>`                          +
+--------+-------------------------------------------------------------------------+
+expr    +smart_ptr< :ref:`ast::ExprConstUInt8 <handle-ast-ExprConstUInt8>` > const+
+--------+-------------------------------------------------------------------------+


|method-ast-AstVisitor.visitExprConstUInt8|

.. das:function:: AstVisitor.preVisitExprConstUInt16(self: AstVisitor; expr: smart_ptr<ast::ExprConstUInt16> const)

+--------+---------------------------------------------------------------------------+
+argument+argument type                                                              +
+========+===========================================================================+
+self    + :ref:`ast::AstVisitor <struct-ast-AstVisitor>`                            +
+--------+---------------------------------------------------------------------------+
+expr    +smart_ptr< :ref:`ast::ExprConstUInt16 <handle-ast-ExprConstUInt16>` > const+
+--------+---------------------------------------------------------------------------+


|method-ast-AstVisitor.preVisitExprConstUInt16|

.. das:function:: AstVisitor.visitExprConstUInt16(self: AstVisitor; expr: smart_ptr<ast::ExprConstUInt16> const)

visitExprConstUInt16 returns  :ref:`ExpressionPtr <alias-ExpressionPtr>` 

+--------+---------------------------------------------------------------------------+
+argument+argument type                                                              +
+========+===========================================================================+
+self    + :ref:`ast::AstVisitor <struct-ast-AstVisitor>`                            +
+--------+---------------------------------------------------------------------------+
+expr    +smart_ptr< :ref:`ast::ExprConstUInt16 <handle-ast-ExprConstUInt16>` > const+
+--------+---------------------------------------------------------------------------+


|method-ast-AstVisitor.visitExprConstUInt16|

.. das:function:: AstVisitor.preVisitExprConstUInt64(self: AstVisitor; expr: smart_ptr<ast::ExprConstUInt64> const)

+--------+---------------------------------------------------------------------------+
+argument+argument type                                                              +
+========+===========================================================================+
+self    + :ref:`ast::AstVisitor <struct-ast-AstVisitor>`                            +
+--------+---------------------------------------------------------------------------+
+expr    +smart_ptr< :ref:`ast::ExprConstUInt64 <handle-ast-ExprConstUInt64>` > const+
+--------+---------------------------------------------------------------------------+


|method-ast-AstVisitor.preVisitExprConstUInt64|

.. das:function:: AstVisitor.visitExprConstUInt64(self: AstVisitor; expr: smart_ptr<ast::ExprConstUInt64> const)

visitExprConstUInt64 returns  :ref:`ExpressionPtr <alias-ExpressionPtr>` 

+--------+---------------------------------------------------------------------------+
+argument+argument type                                                              +
+========+===========================================================================+
+self    + :ref:`ast::AstVisitor <struct-ast-AstVisitor>`                            +
+--------+---------------------------------------------------------------------------+
+expr    +smart_ptr< :ref:`ast::ExprConstUInt64 <handle-ast-ExprConstUInt64>` > const+
+--------+---------------------------------------------------------------------------+


|method-ast-AstVisitor.visitExprConstUInt64|

.. das:function:: AstVisitor.preVisitExprConstUInt(self: AstVisitor; expr: smart_ptr<ast::ExprConstUInt> const)

+--------+-----------------------------------------------------------------------+
+argument+argument type                                                          +
+========+=======================================================================+
+self    + :ref:`ast::AstVisitor <struct-ast-AstVisitor>`                        +
+--------+-----------------------------------------------------------------------+
+expr    +smart_ptr< :ref:`ast::ExprConstUInt <handle-ast-ExprConstUInt>` > const+
+--------+-----------------------------------------------------------------------+


|method-ast-AstVisitor.preVisitExprConstUInt|

.. das:function:: AstVisitor.visitExprConstUInt(self: AstVisitor; expr: smart_ptr<ast::ExprConstUInt> const)

visitExprConstUInt returns  :ref:`ExpressionPtr <alias-ExpressionPtr>` 

+--------+-----------------------------------------------------------------------+
+argument+argument type                                                          +
+========+=======================================================================+
+self    + :ref:`ast::AstVisitor <struct-ast-AstVisitor>`                        +
+--------+-----------------------------------------------------------------------+
+expr    +smart_ptr< :ref:`ast::ExprConstUInt <handle-ast-ExprConstUInt>` > const+
+--------+-----------------------------------------------------------------------+


|method-ast-AstVisitor.visitExprConstUInt|

.. das:function:: AstVisitor.preVisitExprConstUInt2(self: AstVisitor; expr: smart_ptr<ast::ExprConstUInt2> const)

+--------+-------------------------------------------------------------------------+
+argument+argument type                                                            +
+========+=========================================================================+
+self    + :ref:`ast::AstVisitor <struct-ast-AstVisitor>`                          +
+--------+-------------------------------------------------------------------------+
+expr    +smart_ptr< :ref:`ast::ExprConstUInt2 <handle-ast-ExprConstUInt2>` > const+
+--------+-------------------------------------------------------------------------+


|method-ast-AstVisitor.preVisitExprConstUInt2|

.. das:function:: AstVisitor.visitExprConstUInt2(self: AstVisitor; expr: smart_ptr<ast::ExprConstUInt2> const)

visitExprConstUInt2 returns  :ref:`ExpressionPtr <alias-ExpressionPtr>` 

+--------+-------------------------------------------------------------------------+
+argument+argument type                                                            +
+========+=========================================================================+
+self    + :ref:`ast::AstVisitor <struct-ast-AstVisitor>`                          +
+--------+-------------------------------------------------------------------------+
+expr    +smart_ptr< :ref:`ast::ExprConstUInt2 <handle-ast-ExprConstUInt2>` > const+
+--------+-------------------------------------------------------------------------+


|method-ast-AstVisitor.visitExprConstUInt2|

.. das:function:: AstVisitor.preVisitExprConstUInt3(self: AstVisitor; expr: smart_ptr<ast::ExprConstUInt3> const)

+--------+-------------------------------------------------------------------------+
+argument+argument type                                                            +
+========+=========================================================================+
+self    + :ref:`ast::AstVisitor <struct-ast-AstVisitor>`                          +
+--------+-------------------------------------------------------------------------+
+expr    +smart_ptr< :ref:`ast::ExprConstUInt3 <handle-ast-ExprConstUInt3>` > const+
+--------+-------------------------------------------------------------------------+


|method-ast-AstVisitor.preVisitExprConstUInt3|

.. das:function:: AstVisitor.visitExprConstUInt3(self: AstVisitor; expr: smart_ptr<ast::ExprConstUInt3> const)

visitExprConstUInt3 returns  :ref:`ExpressionPtr <alias-ExpressionPtr>` 

+--------+-------------------------------------------------------------------------+
+argument+argument type                                                            +
+========+=========================================================================+
+self    + :ref:`ast::AstVisitor <struct-ast-AstVisitor>`                          +
+--------+-------------------------------------------------------------------------+
+expr    +smart_ptr< :ref:`ast::ExprConstUInt3 <handle-ast-ExprConstUInt3>` > const+
+--------+-------------------------------------------------------------------------+


|method-ast-AstVisitor.visitExprConstUInt3|

.. das:function:: AstVisitor.preVisitExprConstUInt4(self: AstVisitor; expr: smart_ptr<ast::ExprConstUInt4> const)

+--------+-------------------------------------------------------------------------+
+argument+argument type                                                            +
+========+=========================================================================+
+self    + :ref:`ast::AstVisitor <struct-ast-AstVisitor>`                          +
+--------+-------------------------------------------------------------------------+
+expr    +smart_ptr< :ref:`ast::ExprConstUInt4 <handle-ast-ExprConstUInt4>` > const+
+--------+-------------------------------------------------------------------------+


|method-ast-AstVisitor.preVisitExprConstUInt4|

.. das:function:: AstVisitor.visitExprConstUInt4(self: AstVisitor; expr: smart_ptr<ast::ExprConstUInt4> const)

visitExprConstUInt4 returns  :ref:`ExpressionPtr <alias-ExpressionPtr>` 

+--------+-------------------------------------------------------------------------+
+argument+argument type                                                            +
+========+=========================================================================+
+self    + :ref:`ast::AstVisitor <struct-ast-AstVisitor>`                          +
+--------+-------------------------------------------------------------------------+
+expr    +smart_ptr< :ref:`ast::ExprConstUInt4 <handle-ast-ExprConstUInt4>` > const+
+--------+-------------------------------------------------------------------------+


|method-ast-AstVisitor.visitExprConstUInt4|

.. das:function:: AstVisitor.preVisitExprConstRange(self: AstVisitor; expr: smart_ptr<ast::ExprConstRange> const)

+--------+-------------------------------------------------------------------------+
+argument+argument type                                                            +
+========+=========================================================================+
+self    + :ref:`ast::AstVisitor <struct-ast-AstVisitor>`                          +
+--------+-------------------------------------------------------------------------+
+expr    +smart_ptr< :ref:`ast::ExprConstRange <handle-ast-ExprConstRange>` > const+
+--------+-------------------------------------------------------------------------+


|method-ast-AstVisitor.preVisitExprConstRange|

.. das:function:: AstVisitor.visitExprConstRange(self: AstVisitor; expr: smart_ptr<ast::ExprConstRange> const)

visitExprConstRange returns  :ref:`ExpressionPtr <alias-ExpressionPtr>` 

+--------+-------------------------------------------------------------------------+
+argument+argument type                                                            +
+========+=========================================================================+
+self    + :ref:`ast::AstVisitor <struct-ast-AstVisitor>`                          +
+--------+-------------------------------------------------------------------------+
+expr    +smart_ptr< :ref:`ast::ExprConstRange <handle-ast-ExprConstRange>` > const+
+--------+-------------------------------------------------------------------------+


|method-ast-AstVisitor.visitExprConstRange|

.. das:function:: AstVisitor.preVisitExprConstURange(self: AstVisitor; expr: smart_ptr<ast::ExprConstURange> const)

+--------+---------------------------------------------------------------------------+
+argument+argument type                                                              +
+========+===========================================================================+
+self    + :ref:`ast::AstVisitor <struct-ast-AstVisitor>`                            +
+--------+---------------------------------------------------------------------------+
+expr    +smart_ptr< :ref:`ast::ExprConstURange <handle-ast-ExprConstURange>` > const+
+--------+---------------------------------------------------------------------------+


|method-ast-AstVisitor.preVisitExprConstURange|

.. das:function:: AstVisitor.visitExprConstURange(self: AstVisitor; expr: smart_ptr<ast::ExprConstURange> const)

visitExprConstURange returns  :ref:`ExpressionPtr <alias-ExpressionPtr>` 

+--------+---------------------------------------------------------------------------+
+argument+argument type                                                              +
+========+===========================================================================+
+self    + :ref:`ast::AstVisitor <struct-ast-AstVisitor>`                            +
+--------+---------------------------------------------------------------------------+
+expr    +smart_ptr< :ref:`ast::ExprConstURange <handle-ast-ExprConstURange>` > const+
+--------+---------------------------------------------------------------------------+


|method-ast-AstVisitor.visitExprConstURange|

.. das:function:: AstVisitor.preVisitExprConstRange64(self: AstVisitor; expr: smart_ptr<ast::ExprConstRange64> const)

+--------+-----------------------------------------------------------------------------+
+argument+argument type                                                                +
+========+=============================================================================+
+self    + :ref:`ast::AstVisitor <struct-ast-AstVisitor>`                              +
+--------+-----------------------------------------------------------------------------+
+expr    +smart_ptr< :ref:`ast::ExprConstRange64 <handle-ast-ExprConstRange64>` > const+
+--------+-----------------------------------------------------------------------------+


|method-ast-AstVisitor.preVisitExprConstRange64|

.. das:function:: AstVisitor.visitExprConstRange64(self: AstVisitor; expr: smart_ptr<ast::ExprConstRange64> const)

visitExprConstRange64 returns  :ref:`ExpressionPtr <alias-ExpressionPtr>` 

+--------+-----------------------------------------------------------------------------+
+argument+argument type                                                                +
+========+=============================================================================+
+self    + :ref:`ast::AstVisitor <struct-ast-AstVisitor>`                              +
+--------+-----------------------------------------------------------------------------+
+expr    +smart_ptr< :ref:`ast::ExprConstRange64 <handle-ast-ExprConstRange64>` > const+
+--------+-----------------------------------------------------------------------------+


|method-ast-AstVisitor.visitExprConstRange64|

.. das:function:: AstVisitor.preVisitExprConstURange64(self: AstVisitor; expr: smart_ptr<ast::ExprConstURange64> const)

+--------+-------------------------------------------------------------------------------+
+argument+argument type                                                                  +
+========+===============================================================================+
+self    + :ref:`ast::AstVisitor <struct-ast-AstVisitor>`                                +
+--------+-------------------------------------------------------------------------------+
+expr    +smart_ptr< :ref:`ast::ExprConstURange64 <handle-ast-ExprConstURange64>` > const+
+--------+-------------------------------------------------------------------------------+


|method-ast-AstVisitor.preVisitExprConstURange64|

.. das:function:: AstVisitor.visitExprConstURange64(self: AstVisitor; expr: smart_ptr<ast::ExprConstURange64> const)

visitExprConstURange64 returns  :ref:`ExpressionPtr <alias-ExpressionPtr>` 

+--------+-------------------------------------------------------------------------------+
+argument+argument type                                                                  +
+========+===============================================================================+
+self    + :ref:`ast::AstVisitor <struct-ast-AstVisitor>`                                +
+--------+-------------------------------------------------------------------------------+
+expr    +smart_ptr< :ref:`ast::ExprConstURange64 <handle-ast-ExprConstURange64>` > const+
+--------+-------------------------------------------------------------------------------+


|method-ast-AstVisitor.visitExprConstURange64|

.. das:function:: AstVisitor.preVisitExprConstBool(self: AstVisitor; expr: smart_ptr<ast::ExprConstBool> const)

+--------+-----------------------------------------------------------------------+
+argument+argument type                                                          +
+========+=======================================================================+
+self    + :ref:`ast::AstVisitor <struct-ast-AstVisitor>`                        +
+--------+-----------------------------------------------------------------------+
+expr    +smart_ptr< :ref:`ast::ExprConstBool <handle-ast-ExprConstBool>` > const+
+--------+-----------------------------------------------------------------------+


|method-ast-AstVisitor.preVisitExprConstBool|

.. das:function:: AstVisitor.visitExprConstBool(self: AstVisitor; expr: smart_ptr<ast::ExprConstBool> const)

visitExprConstBool returns  :ref:`ExpressionPtr <alias-ExpressionPtr>` 

+--------+-----------------------------------------------------------------------+
+argument+argument type                                                          +
+========+=======================================================================+
+self    + :ref:`ast::AstVisitor <struct-ast-AstVisitor>`                        +
+--------+-----------------------------------------------------------------------+
+expr    +smart_ptr< :ref:`ast::ExprConstBool <handle-ast-ExprConstBool>` > const+
+--------+-----------------------------------------------------------------------+


|method-ast-AstVisitor.visitExprConstBool|

.. das:function:: AstVisitor.preVisitExprConstFloat(self: AstVisitor; expr: smart_ptr<ast::ExprConstFloat> const)

+--------+-------------------------------------------------------------------------+
+argument+argument type                                                            +
+========+=========================================================================+
+self    + :ref:`ast::AstVisitor <struct-ast-AstVisitor>`                          +
+--------+-------------------------------------------------------------------------+
+expr    +smart_ptr< :ref:`ast::ExprConstFloat <handle-ast-ExprConstFloat>` > const+
+--------+-------------------------------------------------------------------------+


|method-ast-AstVisitor.preVisitExprConstFloat|

.. das:function:: AstVisitor.visitExprConstFloat(self: AstVisitor; expr: smart_ptr<ast::ExprConstFloat> const)

visitExprConstFloat returns  :ref:`ExpressionPtr <alias-ExpressionPtr>` 

+--------+-------------------------------------------------------------------------+
+argument+argument type                                                            +
+========+=========================================================================+
+self    + :ref:`ast::AstVisitor <struct-ast-AstVisitor>`                          +
+--------+-------------------------------------------------------------------------+
+expr    +smart_ptr< :ref:`ast::ExprConstFloat <handle-ast-ExprConstFloat>` > const+
+--------+-------------------------------------------------------------------------+


|method-ast-AstVisitor.visitExprConstFloat|

.. das:function:: AstVisitor.preVisitExprConstFloat2(self: AstVisitor; expr: smart_ptr<ast::ExprConstFloat2> const)

+--------+---------------------------------------------------------------------------+
+argument+argument type                                                              +
+========+===========================================================================+
+self    + :ref:`ast::AstVisitor <struct-ast-AstVisitor>`                            +
+--------+---------------------------------------------------------------------------+
+expr    +smart_ptr< :ref:`ast::ExprConstFloat2 <handle-ast-ExprConstFloat2>` > const+
+--------+---------------------------------------------------------------------------+


|method-ast-AstVisitor.preVisitExprConstFloat2|

.. das:function:: AstVisitor.visitExprConstFloat2(self: AstVisitor; expr: smart_ptr<ast::ExprConstFloat2> const)

visitExprConstFloat2 returns  :ref:`ExpressionPtr <alias-ExpressionPtr>` 

+--------+---------------------------------------------------------------------------+
+argument+argument type                                                              +
+========+===========================================================================+
+self    + :ref:`ast::AstVisitor <struct-ast-AstVisitor>`                            +
+--------+---------------------------------------------------------------------------+
+expr    +smart_ptr< :ref:`ast::ExprConstFloat2 <handle-ast-ExprConstFloat2>` > const+
+--------+---------------------------------------------------------------------------+


|method-ast-AstVisitor.visitExprConstFloat2|

.. das:function:: AstVisitor.preVisitExprConstFloat3(self: AstVisitor; expr: smart_ptr<ast::ExprConstFloat3> const)

+--------+---------------------------------------------------------------------------+
+argument+argument type                                                              +
+========+===========================================================================+
+self    + :ref:`ast::AstVisitor <struct-ast-AstVisitor>`                            +
+--------+---------------------------------------------------------------------------+
+expr    +smart_ptr< :ref:`ast::ExprConstFloat3 <handle-ast-ExprConstFloat3>` > const+
+--------+---------------------------------------------------------------------------+


|method-ast-AstVisitor.preVisitExprConstFloat3|

.. das:function:: AstVisitor.visitExprConstFloat3(self: AstVisitor; expr: smart_ptr<ast::ExprConstFloat3> const)

visitExprConstFloat3 returns  :ref:`ExpressionPtr <alias-ExpressionPtr>` 

+--------+---------------------------------------------------------------------------+
+argument+argument type                                                              +
+========+===========================================================================+
+self    + :ref:`ast::AstVisitor <struct-ast-AstVisitor>`                            +
+--------+---------------------------------------------------------------------------+
+expr    +smart_ptr< :ref:`ast::ExprConstFloat3 <handle-ast-ExprConstFloat3>` > const+
+--------+---------------------------------------------------------------------------+


|method-ast-AstVisitor.visitExprConstFloat3|

.. das:function:: AstVisitor.preVisitExprConstFloat4(self: AstVisitor; expr: smart_ptr<ast::ExprConstFloat4> const)

+--------+---------------------------------------------------------------------------+
+argument+argument type                                                              +
+========+===========================================================================+
+self    + :ref:`ast::AstVisitor <struct-ast-AstVisitor>`                            +
+--------+---------------------------------------------------------------------------+
+expr    +smart_ptr< :ref:`ast::ExprConstFloat4 <handle-ast-ExprConstFloat4>` > const+
+--------+---------------------------------------------------------------------------+


|method-ast-AstVisitor.preVisitExprConstFloat4|

.. das:function:: AstVisitor.visitExprConstFloat4(self: AstVisitor; expr: smart_ptr<ast::ExprConstFloat4> const)

visitExprConstFloat4 returns  :ref:`ExpressionPtr <alias-ExpressionPtr>` 

+--------+---------------------------------------------------------------------------+
+argument+argument type                                                              +
+========+===========================================================================+
+self    + :ref:`ast::AstVisitor <struct-ast-AstVisitor>`                            +
+--------+---------------------------------------------------------------------------+
+expr    +smart_ptr< :ref:`ast::ExprConstFloat4 <handle-ast-ExprConstFloat4>` > const+
+--------+---------------------------------------------------------------------------+


|method-ast-AstVisitor.visitExprConstFloat4|

.. das:function:: AstVisitor.preVisitExprConstString(self: AstVisitor; expr: smart_ptr<ast::ExprConstString> const)

+--------+---------------------------------------------------------------------------+
+argument+argument type                                                              +
+========+===========================================================================+
+self    + :ref:`ast::AstVisitor <struct-ast-AstVisitor>`                            +
+--------+---------------------------------------------------------------------------+
+expr    +smart_ptr< :ref:`ast::ExprConstString <handle-ast-ExprConstString>` > const+
+--------+---------------------------------------------------------------------------+


|method-ast-AstVisitor.preVisitExprConstString|

.. das:function:: AstVisitor.visitExprConstString(self: AstVisitor; expr: smart_ptr<ast::ExprConstString> const)

visitExprConstString returns  :ref:`ExpressionPtr <alias-ExpressionPtr>` 

+--------+---------------------------------------------------------------------------+
+argument+argument type                                                              +
+========+===========================================================================+
+self    + :ref:`ast::AstVisitor <struct-ast-AstVisitor>`                            +
+--------+---------------------------------------------------------------------------+
+expr    +smart_ptr< :ref:`ast::ExprConstString <handle-ast-ExprConstString>` > const+
+--------+---------------------------------------------------------------------------+


|method-ast-AstVisitor.visitExprConstString|

.. das:function:: AstVisitor.preVisitExprConstDouble(self: AstVisitor; expr: smart_ptr<ast::ExprConstDouble> const)

+--------+---------------------------------------------------------------------------+
+argument+argument type                                                              +
+========+===========================================================================+
+self    + :ref:`ast::AstVisitor <struct-ast-AstVisitor>`                            +
+--------+---------------------------------------------------------------------------+
+expr    +smart_ptr< :ref:`ast::ExprConstDouble <handle-ast-ExprConstDouble>` > const+
+--------+---------------------------------------------------------------------------+


|method-ast-AstVisitor.preVisitExprConstDouble|

.. das:function:: AstVisitor.visitExprConstDouble(self: AstVisitor; expr: smart_ptr<ast::ExprConstDouble> const)

visitExprConstDouble returns  :ref:`ExpressionPtr <alias-ExpressionPtr>` 

+--------+---------------------------------------------------------------------------+
+argument+argument type                                                              +
+========+===========================================================================+
+self    + :ref:`ast::AstVisitor <struct-ast-AstVisitor>`                            +
+--------+---------------------------------------------------------------------------+
+expr    +smart_ptr< :ref:`ast::ExprConstDouble <handle-ast-ExprConstDouble>` > const+
+--------+---------------------------------------------------------------------------+


|method-ast-AstVisitor.visitExprConstDouble|

.. das:function:: AstVisitor.preVisitExprFakeContext(self: AstVisitor; expr: smart_ptr<ast::ExprFakeContext> const)

+--------+---------------------------------------------------------------------------+
+argument+argument type                                                              +
+========+===========================================================================+
+self    + :ref:`ast::AstVisitor <struct-ast-AstVisitor>`                            +
+--------+---------------------------------------------------------------------------+
+expr    +smart_ptr< :ref:`ast::ExprFakeContext <handle-ast-ExprFakeContext>` > const+
+--------+---------------------------------------------------------------------------+


|method-ast-AstVisitor.preVisitExprFakeContext|

.. das:function:: AstVisitor.visitExprFakeContext(self: AstVisitor; expr: smart_ptr<ast::ExprFakeContext> const)

visitExprFakeContext returns  :ref:`ExpressionPtr <alias-ExpressionPtr>` 

+--------+---------------------------------------------------------------------------+
+argument+argument type                                                              +
+========+===========================================================================+
+self    + :ref:`ast::AstVisitor <struct-ast-AstVisitor>`                            +
+--------+---------------------------------------------------------------------------+
+expr    +smart_ptr< :ref:`ast::ExprFakeContext <handle-ast-ExprFakeContext>` > const+
+--------+---------------------------------------------------------------------------+


|method-ast-AstVisitor.visitExprFakeContext|

.. das:function:: AstVisitor.preVisitExprFakeLineInfo(self: AstVisitor; expr: smart_ptr<ast::ExprFakeLineInfo> const)

+--------+-----------------------------------------------------------------------------+
+argument+argument type                                                                +
+========+=============================================================================+
+self    + :ref:`ast::AstVisitor <struct-ast-AstVisitor>`                              +
+--------+-----------------------------------------------------------------------------+
+expr    +smart_ptr< :ref:`ast::ExprFakeLineInfo <handle-ast-ExprFakeLineInfo>` > const+
+--------+-----------------------------------------------------------------------------+


|method-ast-AstVisitor.preVisitExprFakeLineInfo|

.. das:function:: AstVisitor.visitExprFakeLineInfo(self: AstVisitor; expr: smart_ptr<ast::ExprFakeLineInfo> const)

visitExprFakeLineInfo returns  :ref:`ExpressionPtr <alias-ExpressionPtr>` 

+--------+-----------------------------------------------------------------------------+
+argument+argument type                                                                +
+========+=============================================================================+
+self    + :ref:`ast::AstVisitor <struct-ast-AstVisitor>`                              +
+--------+-----------------------------------------------------------------------------+
+expr    +smart_ptr< :ref:`ast::ExprFakeLineInfo <handle-ast-ExprFakeLineInfo>` > const+
+--------+-----------------------------------------------------------------------------+


|method-ast-AstVisitor.visitExprFakeLineInfo|

.. das:function:: AstVisitor.preVisitExprReader(self: AstVisitor; expr: smart_ptr<ast::ExprReader> const)

+--------+-----------------------------------------------------------------+
+argument+argument type                                                    +
+========+=================================================================+
+self    + :ref:`ast::AstVisitor <struct-ast-AstVisitor>`                  +
+--------+-----------------------------------------------------------------+
+expr    +smart_ptr< :ref:`ast::ExprReader <handle-ast-ExprReader>` > const+
+--------+-----------------------------------------------------------------+


|method-ast-AstVisitor.preVisitExprReader|

.. das:function:: AstVisitor.visitExprReader(self: AstVisitor; expr: smart_ptr<ast::ExprReader> const)

visitExprReader returns  :ref:`ExpressionPtr <alias-ExpressionPtr>` 

+--------+-----------------------------------------------------------------+
+argument+argument type                                                    +
+========+=================================================================+
+self    + :ref:`ast::AstVisitor <struct-ast-AstVisitor>`                  +
+--------+-----------------------------------------------------------------+
+expr    +smart_ptr< :ref:`ast::ExprReader <handle-ast-ExprReader>` > const+
+--------+-----------------------------------------------------------------+


|method-ast-AstVisitor.visitExprReader|

.. das:function:: AstVisitor.preVisitExprUnsafe(self: AstVisitor; expr: smart_ptr<ast::ExprUnsafe> const)

+--------+-----------------------------------------------------------------+
+argument+argument type                                                    +
+========+=================================================================+
+self    + :ref:`ast::AstVisitor <struct-ast-AstVisitor>`                  +
+--------+-----------------------------------------------------------------+
+expr    +smart_ptr< :ref:`ast::ExprUnsafe <handle-ast-ExprUnsafe>` > const+
+--------+-----------------------------------------------------------------+


|method-ast-AstVisitor.preVisitExprUnsafe|

.. das:function:: AstVisitor.visitExprUnsafe(self: AstVisitor; expr: smart_ptr<ast::ExprUnsafe> const)

visitExprUnsafe returns  :ref:`ExpressionPtr <alias-ExpressionPtr>` 

+--------+-----------------------------------------------------------------+
+argument+argument type                                                    +
+========+=================================================================+
+self    + :ref:`ast::AstVisitor <struct-ast-AstVisitor>`                  +
+--------+-----------------------------------------------------------------+
+expr    +smart_ptr< :ref:`ast::ExprUnsafe <handle-ast-ExprUnsafe>` > const+
+--------+-----------------------------------------------------------------+


|method-ast-AstVisitor.visitExprUnsafe|

.. das:function:: AstVisitor.preVisitExprCallMacro(self: AstVisitor; expr: smart_ptr<ast::ExprCallMacro> const)

+--------+-----------------------------------------------------------------------+
+argument+argument type                                                          +
+========+=======================================================================+
+self    + :ref:`ast::AstVisitor <struct-ast-AstVisitor>`                        +
+--------+-----------------------------------------------------------------------+
+expr    +smart_ptr< :ref:`ast::ExprCallMacro <handle-ast-ExprCallMacro>` > const+
+--------+-----------------------------------------------------------------------+


|method-ast-AstVisitor.preVisitExprCallMacro|

.. das:function:: AstVisitor.visitExprCallMacro(self: AstVisitor; expr: smart_ptr<ast::ExprCallMacro> const)

visitExprCallMacro returns  :ref:`ExpressionPtr <alias-ExpressionPtr>` 

+--------+-----------------------------------------------------------------------+
+argument+argument type                                                          +
+========+=======================================================================+
+self    + :ref:`ast::AstVisitor <struct-ast-AstVisitor>`                        +
+--------+-----------------------------------------------------------------------+
+expr    +smart_ptr< :ref:`ast::ExprCallMacro <handle-ast-ExprCallMacro>` > const+
+--------+-----------------------------------------------------------------------+


|method-ast-AstVisitor.visitExprCallMacro|

+++++++++++++++
Call generation
+++++++++++++++

  *  :ref:`make_call (at:rtti::LineInfo const implicit;name:string const implicit) : smart_ptr\<ast::Expression\> <function-_at_ast_c__c_make_call_CIH_ls_rtti_c__c_LineInfo_gr__CIs>` 

.. _function-_at_ast_c__c_make_call_CIH_ls_rtti_c__c_LineInfo_gr__CIs:

.. das:function:: make_call(at: LineInfo const implicit; name: string const implicit)

make_call returns smart_ptr< :ref:`ast::Expression <handle-ast-Expression>` >

+--------+-------------------------------------------------------------+
+argument+argument type                                                +
+========+=============================================================+
+at      + :ref:`rtti::LineInfo <handle-rtti-LineInfo>`  const implicit+
+--------+-------------------------------------------------------------+
+name    +string const implicit                                        +
+--------+-------------------------------------------------------------+


|function-ast-make_call|

+++++++++++++++
Visitor pattern
+++++++++++++++

  *  :ref:`visit (program:smart_ptr\<rtti::Program\> const implicit;adapter:smart_ptr\<ast::VisitorAdapter\> const implicit;context:__context const;line:__lineInfo const) : void <function-_at_ast_c__c_visit_CI1_ls_H_ls_rtti_c__c_Program_gr__gr_?M_CI1_ls_H_ls_ast_c__c_VisitorAdapter_gr__gr_?M_C_c_C_l>` 
  *  :ref:`visit_modules (program:smart_ptr\<rtti::Program\> const implicit;adapter:smart_ptr\<ast::VisitorAdapter\> const implicit;context:__context const;line:__lineInfo const) : void <function-_at_ast_c__c_visit_modules_CI1_ls_H_ls_rtti_c__c_Program_gr__gr_?M_CI1_ls_H_ls_ast_c__c_VisitorAdapter_gr__gr_?M_C_c_C_l>` 
  *  :ref:`visit (function:smart_ptr\<ast::Function\> const implicit;adapter:smart_ptr\<ast::VisitorAdapter\> const implicit;context:__context const;line:__lineInfo const) : void <function-_at_ast_c__c_visit_CI1_ls_H_ls_ast_c__c_Function_gr__gr_?M_CI1_ls_H_ls_ast_c__c_VisitorAdapter_gr__gr_?M_C_c_C_l>` 
  *  :ref:`visit (expression:smart_ptr\<ast::Expression\> const implicit;adapter:smart_ptr\<ast::VisitorAdapter\> const implicit;context:__context const;line:__lineInfo const) : smart_ptr\<ast::Expression\> <function-_at_ast_c__c_visit_CI1_ls_H_ls_ast_c__c_Expression_gr__gr_?M_CI1_ls_H_ls_ast_c__c_VisitorAdapter_gr__gr_?M_C_c_C_l>` 
  *  :ref:`visit_finally (expression:smart_ptr\<ast::ExprBlock\> const implicit;adapter:smart_ptr\<ast::VisitorAdapter\> const implicit;context:__context const;line:__lineInfo const) : void <function-_at_ast_c__c_visit_finally_CI1_ls_H_ls_ast_c__c_ExprBlock_gr__gr_?M_CI1_ls_H_ls_ast_c__c_VisitorAdapter_gr__gr_?M_C_c_C_l>` 

.. _function-_at_ast_c__c_visit_CI1_ls_H_ls_rtti_c__c_Program_gr__gr_?M_CI1_ls_H_ls_ast_c__c_VisitorAdapter_gr__gr_?M_C_c_C_l:

.. das:function:: visit(program: smart_ptr<rtti::Program> const implicit; adapter: smart_ptr<ast::VisitorAdapter> const implicit)

+--------+----------------------------------------------------------------------------------+
+argument+argument type                                                                     +
+========+==================================================================================+
+program +smart_ptr< :ref:`rtti::Program <handle-rtti-Program>` > const implicit            +
+--------+----------------------------------------------------------------------------------+
+adapter +smart_ptr< :ref:`ast::VisitorAdapter <handle-ast-VisitorAdapter>` > const implicit+
+--------+----------------------------------------------------------------------------------+


|function-ast-visit|

.. _function-_at_ast_c__c_visit_modules_CI1_ls_H_ls_rtti_c__c_Program_gr__gr_?M_CI1_ls_H_ls_ast_c__c_VisitorAdapter_gr__gr_?M_C_c_C_l:

.. das:function:: visit_modules(program: smart_ptr<rtti::Program> const implicit; adapter: smart_ptr<ast::VisitorAdapter> const implicit)

+--------+----------------------------------------------------------------------------------+
+argument+argument type                                                                     +
+========+==================================================================================+
+program +smart_ptr< :ref:`rtti::Program <handle-rtti-Program>` > const implicit            +
+--------+----------------------------------------------------------------------------------+
+adapter +smart_ptr< :ref:`ast::VisitorAdapter <handle-ast-VisitorAdapter>` > const implicit+
+--------+----------------------------------------------------------------------------------+


|function-ast-visit_modules|

.. _function-_at_ast_c__c_visit_CI1_ls_H_ls_ast_c__c_Function_gr__gr_?M_CI1_ls_H_ls_ast_c__c_VisitorAdapter_gr__gr_?M_C_c_C_l:

.. das:function:: visit(function: smart_ptr<ast::Function> const implicit; adapter: smart_ptr<ast::VisitorAdapter> const implicit)

+--------+----------------------------------------------------------------------------------+
+argument+argument type                                                                     +
+========+==================================================================================+
+function+smart_ptr< :ref:`ast::Function <handle-ast-Function>` > const implicit            +
+--------+----------------------------------------------------------------------------------+
+adapter +smart_ptr< :ref:`ast::VisitorAdapter <handle-ast-VisitorAdapter>` > const implicit+
+--------+----------------------------------------------------------------------------------+


|function-ast-visit|

.. _function-_at_ast_c__c_visit_CI1_ls_H_ls_ast_c__c_Expression_gr__gr_?M_CI1_ls_H_ls_ast_c__c_VisitorAdapter_gr__gr_?M_C_c_C_l:

.. das:function:: visit(expression: smart_ptr<ast::Expression> const implicit; adapter: smart_ptr<ast::VisitorAdapter> const implicit)

visit returns smart_ptr< :ref:`ast::Expression <handle-ast-Expression>` >

+----------+----------------------------------------------------------------------------------+
+argument  +argument type                                                                     +
+==========+==================================================================================+
+expression+smart_ptr< :ref:`ast::Expression <handle-ast-Expression>` > const implicit        +
+----------+----------------------------------------------------------------------------------+
+adapter   +smart_ptr< :ref:`ast::VisitorAdapter <handle-ast-VisitorAdapter>` > const implicit+
+----------+----------------------------------------------------------------------------------+


|function-ast-visit|

.. _function-_at_ast_c__c_visit_finally_CI1_ls_H_ls_ast_c__c_ExprBlock_gr__gr_?M_CI1_ls_H_ls_ast_c__c_VisitorAdapter_gr__gr_?M_C_c_C_l:

.. das:function:: visit_finally(expression: smart_ptr<ast::ExprBlock> const implicit; adapter: smart_ptr<ast::VisitorAdapter> const implicit)

+----------+----------------------------------------------------------------------------------+
+argument  +argument type                                                                     +
+==========+==================================================================================+
+expression+smart_ptr< :ref:`ast::ExprBlock <handle-ast-ExprBlock>` > const implicit          +
+----------+----------------------------------------------------------------------------------+
+adapter   +smart_ptr< :ref:`ast::VisitorAdapter <handle-ast-VisitorAdapter>` > const implicit+
+----------+----------------------------------------------------------------------------------+


|function-ast-visit_finally|

+++++++++++++++++++++
Expression generation
+++++++++++++++++++++

  *  :ref:`force_generated (expression:smart_ptr\<ast::Expression\> const& implicit;value:bool const) : void <function-_at_ast_c__c_force_generated_C&I1_ls_H_ls_ast_c__c_Expression_gr__gr_?M_Cb>` 
  *  :ref:`get_expression_annotation (expr:ast::Expression? const implicit;context:__context const;line:__lineInfo const) : rtti::Annotation? <function-_at_ast_c__c_get_expression_annotation_CI1_ls_H_ls_ast_c__c_Expression_gr__gr_?_C_c_C_l>` 
  *  :ref:`make_type_info_structure (ctx:rtti::Context implicit;type:smart_ptr\<ast::TypeDecl\> const implicit;context:__context const;at:__lineInfo const) : rtti::TypeInfo? <function-_at_ast_c__c_make_type_info_structure_IH_ls_rtti_c__c_Context_gr__CI1_ls_H_ls_ast_c__c_TypeDecl_gr__gr_?W_C_c_C_l>` 

.. _function-_at_ast_c__c_force_generated_C&I1_ls_H_ls_ast_c__c_Expression_gr__gr_?M_Cb:

.. das:function:: force_generated(expression: smart_ptr<ast::Expression> const& implicit; value: bool const)

+----------+---------------------------------------------------------------------------+
+argument  +argument type                                                              +
+==========+===========================================================================+
+expression+smart_ptr< :ref:`ast::Expression <handle-ast-Expression>` > const& implicit+
+----------+---------------------------------------------------------------------------+
+value     +bool const                                                                 +
+----------+---------------------------------------------------------------------------+


|function-ast-force_generated|

.. _function-_at_ast_c__c_get_expression_annotation_CI1_ls_H_ls_ast_c__c_Expression_gr__gr_?_C_c_C_l:

.. das:function:: get_expression_annotation(expr: ast::Expression? const implicit)

get_expression_annotation returns  :ref:`rtti::Annotation <handle-rtti-Annotation>` ?

+--------+----------------------------------------------------------------+
+argument+argument type                                                   +
+========+================================================================+
+expr    + :ref:`ast::Expression <handle-ast-Expression>` ? const implicit+
+--------+----------------------------------------------------------------+


|function-ast-get_expression_annotation|

.. _function-_at_ast_c__c_make_type_info_structure_IH_ls_rtti_c__c_Context_gr__CI1_ls_H_ls_ast_c__c_TypeDecl_gr__gr_?W_C_c_C_l:

.. das:function:: make_type_info_structure(ctx: Context implicit; type: smart_ptr<ast::TypeDecl> const implicit)

make_type_info_structure returns  :ref:`rtti::TypeInfo <handle-rtti-TypeInfo>` ?

+--------+----------------------------------------------------------------------+
+argument+argument type                                                         +
+========+======================================================================+
+ctx     + :ref:`rtti::Context <handle-rtti-Context>`  implicit                 +
+--------+----------------------------------------------------------------------+
+type    +smart_ptr< :ref:`ast::TypeDecl <handle-ast-TypeDecl>` > const implicit+
+--------+----------------------------------------------------------------------+


|function-ast-make_type_info_structure|

++++++++++++++++++
Adapter generation
++++++++++++++++++

  *  :ref:`make_visitor (class:void? const implicit;info:rtti::StructInfo const? const implicit;context:__context const) : smart_ptr\<ast::VisitorAdapter\> <function-_at_ast_c__c_make_visitor_CI?_CI1_ls_CH_ls_rtti_c__c_StructInfo_gr__gr_?_C_c>` 
  *  :ref:`make_function_annotation (name:string const implicit;class:void? const implicit;info:rtti::StructInfo const? const implicit;context:__context const) : smart_ptr\<ast::FunctionAnnotation\> <function-_at_ast_c__c_make_function_annotation_CIs_CI?_CI1_ls_CH_ls_rtti_c__c_StructInfo_gr__gr_?_C_c>` 
  *  :ref:`make_block_annotation (name:string const implicit;class:void? const implicit;info:rtti::StructInfo const? const implicit;context:__context const) : smart_ptr\<ast::FunctionAnnotation\> <function-_at_ast_c__c_make_block_annotation_CIs_CI?_CI1_ls_CH_ls_rtti_c__c_StructInfo_gr__gr_?_C_c>` 
  *  :ref:`make_structure_annotation (name:string const implicit;class:void? const implicit;info:rtti::StructInfo const? const implicit;context:__context const) : smart_ptr\<ast::StructureAnnotation\> <function-_at_ast_c__c_make_structure_annotation_CIs_CI?_CI1_ls_CH_ls_rtti_c__c_StructInfo_gr__gr_?_C_c>` 
  *  :ref:`make_enumeration_annotation (name:string const implicit;class:void? const implicit;info:rtti::StructInfo const? const implicit;context:__context const) : smart_ptr\<ast::EnumerationAnnotation\> <function-_at_ast_c__c_make_enumeration_annotation_CIs_CI?_CI1_ls_CH_ls_rtti_c__c_StructInfo_gr__gr_?_C_c>` 
  *  :ref:`make_pass_macro (name:string const implicit;class:void? const implicit;info:rtti::StructInfo const? const implicit;context:__context const) : smart_ptr\<ast::PassMacro\> <function-_at_ast_c__c_make_pass_macro_CIs_CI?_CI1_ls_CH_ls_rtti_c__c_StructInfo_gr__gr_?_C_c>` 
  *  :ref:`make_reader_macro (name:string const implicit;class:void? const implicit;info:rtti::StructInfo const? const implicit;context:__context const) : smart_ptr\<ast::ReaderMacro\> <function-_at_ast_c__c_make_reader_macro_CIs_CI?_CI1_ls_CH_ls_rtti_c__c_StructInfo_gr__gr_?_C_c>` 
  *  :ref:`make_comment_reader (class:void? const implicit;info:rtti::StructInfo const? const implicit;context:__context const) : smart_ptr\<ast::CommentReader\> <function-_at_ast_c__c_make_comment_reader_CI?_CI1_ls_CH_ls_rtti_c__c_StructInfo_gr__gr_?_C_c>` 
  *  :ref:`make_call_macro (name:string const implicit;class:void? const implicit;info:rtti::StructInfo const? const implicit;context:__context const) : smart_ptr\<ast::CallMacro\> <function-_at_ast_c__c_make_call_macro_CIs_CI?_CI1_ls_CH_ls_rtti_c__c_StructInfo_gr__gr_?_C_c>` 
  *  :ref:`make_typeinfo_macro (name:string const implicit;class:void? const implicit;info:rtti::StructInfo const? const implicit;context:__context const) : smart_ptr\<ast::TypeInfoMacro\> <function-_at_ast_c__c_make_typeinfo_macro_CIs_CI?_CI1_ls_CH_ls_rtti_c__c_StructInfo_gr__gr_?_C_c>` 
  *  :ref:`make_variant_macro (name:string const implicit;class:void? const implicit;info:rtti::StructInfo const? const implicit;context:__context const) : smart_ptr\<ast::VariantMacro\> <function-_at_ast_c__c_make_variant_macro_CIs_CI?_CI1_ls_CH_ls_rtti_c__c_StructInfo_gr__gr_?_C_c>` 
  *  :ref:`make_for_loop_macro (name:string const implicit;class:void? const implicit;info:rtti::StructInfo const? const implicit;context:__context const) : smart_ptr\<ast::ForLoopMacro\> <function-_at_ast_c__c_make_for_loop_macro_CIs_CI?_CI1_ls_CH_ls_rtti_c__c_StructInfo_gr__gr_?_C_c>` 
  *  :ref:`make_capture_macro (name:string const implicit;class:void? const implicit;info:rtti::StructInfo const? const implicit;context:__context const) : smart_ptr\<ast::CaptureMacro\> <function-_at_ast_c__c_make_capture_macro_CIs_CI?_CI1_ls_CH_ls_rtti_c__c_StructInfo_gr__gr_?_C_c>` 
  *  :ref:`make_simulate_macro (name:string const implicit;class:void? const implicit;info:rtti::StructInfo const? const implicit;context:__context const) : smart_ptr\<ast::SimulateMacro\> <function-_at_ast_c__c_make_simulate_macro_CIs_CI?_CI1_ls_CH_ls_rtti_c__c_StructInfo_gr__gr_?_C_c>` 
  *  :ref:`make_clone_structure (structure:ast::Structure? const implicit) : smart_ptr\<ast::Function\> <function-_at_ast_c__c_make_clone_structure_CI1_ls_H_ls_ast_c__c_Structure_gr__gr_?>` 
  *  :ref:`make_function_annotation (name:string const;someClassPtr:auto const) : smart_ptr\<ast::FunctionAnnotation\> <function-_at_ast_c__c_make_function_annotation_Cs_C.>` 
  *  :ref:`make_block_annotation (name:string const;someClassPtr:auto const) : smart_ptr\<ast::FunctionAnnotation\> <function-_at_ast_c__c_make_block_annotation_Cs_C.>` 
  *  :ref:`make_structure_annotation (name:string const;someClassPtr:auto const) : smart_ptr\<ast::StructureAnnotation\> <function-_at_ast_c__c_make_structure_annotation_Cs_C.>` 
  *  :ref:`make_enumeration_annotation (name:string const;someClassPtr:auto const) : smart_ptr\<ast::EnumerationAnnotation\> <function-_at_ast_c__c_make_enumeration_annotation_Cs_C.>` 
  *  :ref:`make_visitor (someClass:auto const) : smart_ptr\<ast::VisitorAdapter\> <function-_at_ast_c__c_make_visitor_C.>` 
  *  :ref:`make_reader_macro (name:string const;someClassPtr:auto const) : smart_ptr\<ast::ReaderMacro\> <function-_at_ast_c__c_make_reader_macro_Cs_C.>` 
  *  :ref:`make_comment_reader (name:string const;someClassPtr:auto const) : smart_ptr\<ast::CommentReader\> <function-_at_ast_c__c_make_comment_reader_Cs_C.>` 
  *  :ref:`make_call_macro (name:string const;someClassPtr:auto const) : smart_ptr\<ast::CallMacro\> <function-_at_ast_c__c_make_call_macro_Cs_C.>` 
  *  :ref:`make_typeinfo_macro (name:string const;someClassPtr:auto const) : smart_ptr\<ast::TypeInfoMacro\> <function-_at_ast_c__c_make_typeinfo_macro_Cs_C.>` 
  *  :ref:`make_pass_macro (name:string const;someClassPtr:auto const) : smart_ptr\<ast::PassMacro\> <function-_at_ast_c__c_make_pass_macro_Cs_C.>` 
  *  :ref:`make_variant_macro (name:string const;someClassPtr:auto const) : smart_ptr\<ast::VariantMacro\> <function-_at_ast_c__c_make_variant_macro_Cs_C.>` 
  *  :ref:`make_for_loop_macro (name:string const;someClassPtr:auto const) : smart_ptr\<ast::ForLoopMacro\> <function-_at_ast_c__c_make_for_loop_macro_Cs_C.>` 
  *  :ref:`make_capture_macro (name:string const;someClassPtr:auto const) : smart_ptr\<ast::CaptureMacro\> <function-_at_ast_c__c_make_capture_macro_Cs_C.>` 
  *  :ref:`make_simulate_macro (name:string const;someClassPtr:auto const) : smart_ptr\<ast::SimulateMacro\> <function-_at_ast_c__c_make_simulate_macro_Cs_C.>` 

.. _function-_at_ast_c__c_make_visitor_CI?_CI1_ls_CH_ls_rtti_c__c_StructInfo_gr__gr_?_C_c:

.. das:function:: make_visitor(class: void? const implicit; info: rtti::StructInfo const? const implicit)

make_visitor returns smart_ptr< :ref:`ast::VisitorAdapter <handle-ast-VisitorAdapter>` >

+--------+------------------------------------------------------------------------+
+argument+argument type                                                           +
+========+========================================================================+
+class   +void? const implicit                                                    +
+--------+------------------------------------------------------------------------+
+info    + :ref:`rtti::StructInfo <handle-rtti-StructInfo>`  const? const implicit+
+--------+------------------------------------------------------------------------+


|function-ast-make_visitor|

.. _function-_at_ast_c__c_make_function_annotation_CIs_CI?_CI1_ls_CH_ls_rtti_c__c_StructInfo_gr__gr_?_C_c:

.. das:function:: make_function_annotation(name: string const implicit; class: void? const implicit; info: rtti::StructInfo const? const implicit)

make_function_annotation returns smart_ptr< :ref:`ast::FunctionAnnotation <handle-ast-FunctionAnnotation>` >

+--------+------------------------------------------------------------------------+
+argument+argument type                                                           +
+========+========================================================================+
+name    +string const implicit                                                   +
+--------+------------------------------------------------------------------------+
+class   +void? const implicit                                                    +
+--------+------------------------------------------------------------------------+
+info    + :ref:`rtti::StructInfo <handle-rtti-StructInfo>`  const? const implicit+
+--------+------------------------------------------------------------------------+


|function-ast-make_function_annotation|

.. _function-_at_ast_c__c_make_block_annotation_CIs_CI?_CI1_ls_CH_ls_rtti_c__c_StructInfo_gr__gr_?_C_c:

.. das:function:: make_block_annotation(name: string const implicit; class: void? const implicit; info: rtti::StructInfo const? const implicit)

make_block_annotation returns smart_ptr< :ref:`ast::FunctionAnnotation <handle-ast-FunctionAnnotation>` >

+--------+------------------------------------------------------------------------+
+argument+argument type                                                           +
+========+========================================================================+
+name    +string const implicit                                                   +
+--------+------------------------------------------------------------------------+
+class   +void? const implicit                                                    +
+--------+------------------------------------------------------------------------+
+info    + :ref:`rtti::StructInfo <handle-rtti-StructInfo>`  const? const implicit+
+--------+------------------------------------------------------------------------+


|function-ast-make_block_annotation|

.. _function-_at_ast_c__c_make_structure_annotation_CIs_CI?_CI1_ls_CH_ls_rtti_c__c_StructInfo_gr__gr_?_C_c:

.. das:function:: make_structure_annotation(name: string const implicit; class: void? const implicit; info: rtti::StructInfo const? const implicit)

make_structure_annotation returns smart_ptr< :ref:`ast::StructureAnnotation <handle-ast-StructureAnnotation>` >

+--------+------------------------------------------------------------------------+
+argument+argument type                                                           +
+========+========================================================================+
+name    +string const implicit                                                   +
+--------+------------------------------------------------------------------------+
+class   +void? const implicit                                                    +
+--------+------------------------------------------------------------------------+
+info    + :ref:`rtti::StructInfo <handle-rtti-StructInfo>`  const? const implicit+
+--------+------------------------------------------------------------------------+


|function-ast-make_structure_annotation|

.. _function-_at_ast_c__c_make_enumeration_annotation_CIs_CI?_CI1_ls_CH_ls_rtti_c__c_StructInfo_gr__gr_?_C_c:

.. das:function:: make_enumeration_annotation(name: string const implicit; class: void? const implicit; info: rtti::StructInfo const? const implicit)

make_enumeration_annotation returns smart_ptr< :ref:`ast::EnumerationAnnotation <handle-ast-EnumerationAnnotation>` >

+--------+------------------------------------------------------------------------+
+argument+argument type                                                           +
+========+========================================================================+
+name    +string const implicit                                                   +
+--------+------------------------------------------------------------------------+
+class   +void? const implicit                                                    +
+--------+------------------------------------------------------------------------+
+info    + :ref:`rtti::StructInfo <handle-rtti-StructInfo>`  const? const implicit+
+--------+------------------------------------------------------------------------+


|function-ast-make_enumeration_annotation|

.. _function-_at_ast_c__c_make_pass_macro_CIs_CI?_CI1_ls_CH_ls_rtti_c__c_StructInfo_gr__gr_?_C_c:

.. das:function:: make_pass_macro(name: string const implicit; class: void? const implicit; info: rtti::StructInfo const? const implicit)

make_pass_macro returns smart_ptr< :ref:`ast::PassMacro <handle-ast-PassMacro>` >

+--------+------------------------------------------------------------------------+
+argument+argument type                                                           +
+========+========================================================================+
+name    +string const implicit                                                   +
+--------+------------------------------------------------------------------------+
+class   +void? const implicit                                                    +
+--------+------------------------------------------------------------------------+
+info    + :ref:`rtti::StructInfo <handle-rtti-StructInfo>`  const? const implicit+
+--------+------------------------------------------------------------------------+


|function-ast-make_pass_macro|

.. _function-_at_ast_c__c_make_reader_macro_CIs_CI?_CI1_ls_CH_ls_rtti_c__c_StructInfo_gr__gr_?_C_c:

.. das:function:: make_reader_macro(name: string const implicit; class: void? const implicit; info: rtti::StructInfo const? const implicit)

make_reader_macro returns smart_ptr< :ref:`ast::ReaderMacro <handle-ast-ReaderMacro>` >

+--------+------------------------------------------------------------------------+
+argument+argument type                                                           +
+========+========================================================================+
+name    +string const implicit                                                   +
+--------+------------------------------------------------------------------------+
+class   +void? const implicit                                                    +
+--------+------------------------------------------------------------------------+
+info    + :ref:`rtti::StructInfo <handle-rtti-StructInfo>`  const? const implicit+
+--------+------------------------------------------------------------------------+


|function-ast-make_reader_macro|

.. _function-_at_ast_c__c_make_comment_reader_CI?_CI1_ls_CH_ls_rtti_c__c_StructInfo_gr__gr_?_C_c:

.. das:function:: make_comment_reader(class: void? const implicit; info: rtti::StructInfo const? const implicit)

make_comment_reader returns smart_ptr< :ref:`ast::CommentReader <handle-ast-CommentReader>` >

+--------+------------------------------------------------------------------------+
+argument+argument type                                                           +
+========+========================================================================+
+class   +void? const implicit                                                    +
+--------+------------------------------------------------------------------------+
+info    + :ref:`rtti::StructInfo <handle-rtti-StructInfo>`  const? const implicit+
+--------+------------------------------------------------------------------------+


|function-ast-make_comment_reader|

.. _function-_at_ast_c__c_make_call_macro_CIs_CI?_CI1_ls_CH_ls_rtti_c__c_StructInfo_gr__gr_?_C_c:

.. das:function:: make_call_macro(name: string const implicit; class: void? const implicit; info: rtti::StructInfo const? const implicit)

make_call_macro returns smart_ptr< :ref:`ast::CallMacro <handle-ast-CallMacro>` >

+--------+------------------------------------------------------------------------+
+argument+argument type                                                           +
+========+========================================================================+
+name    +string const implicit                                                   +
+--------+------------------------------------------------------------------------+
+class   +void? const implicit                                                    +
+--------+------------------------------------------------------------------------+
+info    + :ref:`rtti::StructInfo <handle-rtti-StructInfo>`  const? const implicit+
+--------+------------------------------------------------------------------------+


|function-ast-make_call_macro|

.. _function-_at_ast_c__c_make_typeinfo_macro_CIs_CI?_CI1_ls_CH_ls_rtti_c__c_StructInfo_gr__gr_?_C_c:

.. das:function:: make_typeinfo_macro(name: string const implicit; class: void? const implicit; info: rtti::StructInfo const? const implicit)

make_typeinfo_macro returns smart_ptr< :ref:`ast::TypeInfoMacro <handle-ast-TypeInfoMacro>` >

+--------+------------------------------------------------------------------------+
+argument+argument type                                                           +
+========+========================================================================+
+name    +string const implicit                                                   +
+--------+------------------------------------------------------------------------+
+class   +void? const implicit                                                    +
+--------+------------------------------------------------------------------------+
+info    + :ref:`rtti::StructInfo <handle-rtti-StructInfo>`  const? const implicit+
+--------+------------------------------------------------------------------------+


|function-ast-make_typeinfo_macro|

.. _function-_at_ast_c__c_make_variant_macro_CIs_CI?_CI1_ls_CH_ls_rtti_c__c_StructInfo_gr__gr_?_C_c:

.. das:function:: make_variant_macro(name: string const implicit; class: void? const implicit; info: rtti::StructInfo const? const implicit)

make_variant_macro returns smart_ptr< :ref:`ast::VariantMacro <handle-ast-VariantMacro>` >

+--------+------------------------------------------------------------------------+
+argument+argument type                                                           +
+========+========================================================================+
+name    +string const implicit                                                   +
+--------+------------------------------------------------------------------------+
+class   +void? const implicit                                                    +
+--------+------------------------------------------------------------------------+
+info    + :ref:`rtti::StructInfo <handle-rtti-StructInfo>`  const? const implicit+
+--------+------------------------------------------------------------------------+


|function-ast-make_variant_macro|

.. _function-_at_ast_c__c_make_for_loop_macro_CIs_CI?_CI1_ls_CH_ls_rtti_c__c_StructInfo_gr__gr_?_C_c:

.. das:function:: make_for_loop_macro(name: string const implicit; class: void? const implicit; info: rtti::StructInfo const? const implicit)

make_for_loop_macro returns smart_ptr< :ref:`ast::ForLoopMacro <handle-ast-ForLoopMacro>` >

+--------+------------------------------------------------------------------------+
+argument+argument type                                                           +
+========+========================================================================+
+name    +string const implicit                                                   +
+--------+------------------------------------------------------------------------+
+class   +void? const implicit                                                    +
+--------+------------------------------------------------------------------------+
+info    + :ref:`rtti::StructInfo <handle-rtti-StructInfo>`  const? const implicit+
+--------+------------------------------------------------------------------------+


|function-ast-make_for_loop_macro|

.. _function-_at_ast_c__c_make_capture_macro_CIs_CI?_CI1_ls_CH_ls_rtti_c__c_StructInfo_gr__gr_?_C_c:

.. das:function:: make_capture_macro(name: string const implicit; class: void? const implicit; info: rtti::StructInfo const? const implicit)

make_capture_macro returns smart_ptr< :ref:`ast::CaptureMacro <handle-ast-CaptureMacro>` >

+--------+------------------------------------------------------------------------+
+argument+argument type                                                           +
+========+========================================================================+
+name    +string const implicit                                                   +
+--------+------------------------------------------------------------------------+
+class   +void? const implicit                                                    +
+--------+------------------------------------------------------------------------+
+info    + :ref:`rtti::StructInfo <handle-rtti-StructInfo>`  const? const implicit+
+--------+------------------------------------------------------------------------+


|function-ast-make_capture_macro|

.. _function-_at_ast_c__c_make_simulate_macro_CIs_CI?_CI1_ls_CH_ls_rtti_c__c_StructInfo_gr__gr_?_C_c:

.. das:function:: make_simulate_macro(name: string const implicit; class: void? const implicit; info: rtti::StructInfo const? const implicit)

make_simulate_macro returns smart_ptr< :ref:`ast::SimulateMacro <handle-ast-SimulateMacro>` >

+--------+------------------------------------------------------------------------+
+argument+argument type                                                           +
+========+========================================================================+
+name    +string const implicit                                                   +
+--------+------------------------------------------------------------------------+
+class   +void? const implicit                                                    +
+--------+------------------------------------------------------------------------+
+info    + :ref:`rtti::StructInfo <handle-rtti-StructInfo>`  const? const implicit+
+--------+------------------------------------------------------------------------+


|function-ast-make_simulate_macro|

.. _function-_at_ast_c__c_make_clone_structure_CI1_ls_H_ls_ast_c__c_Structure_gr__gr_?:

.. das:function:: make_clone_structure(structure: ast::Structure? const implicit)

make_clone_structure returns smart_ptr< :ref:`ast::Function <handle-ast-Function>` >

+---------+--------------------------------------------------------------+
+argument +argument type                                                 +
+=========+==============================================================+
+structure+ :ref:`ast::Structure <handle-ast-Structure>` ? const implicit+
+---------+--------------------------------------------------------------+


|function-ast-make_clone_structure|

.. _function-_at_ast_c__c_make_function_annotation_Cs_C.:

.. das:function:: make_function_annotation(name: string const; someClassPtr: auto const)

make_function_annotation returns  :ref:`FunctionAnnotationPtr <alias-FunctionAnnotationPtr>` 

+------------+-------------+
+argument    +argument type+
+============+=============+
+name        +string const +
+------------+-------------+
+someClassPtr+auto const   +
+------------+-------------+


|function-ast-make_function_annotation|

.. _function-_at_ast_c__c_make_block_annotation_Cs_C.:

.. das:function:: make_block_annotation(name: string const; someClassPtr: auto const)

make_block_annotation returns  :ref:`FunctionAnnotationPtr <alias-FunctionAnnotationPtr>` 

+------------+-------------+
+argument    +argument type+
+============+=============+
+name        +string const +
+------------+-------------+
+someClassPtr+auto const   +
+------------+-------------+


|function-ast-make_block_annotation|

.. _function-_at_ast_c__c_make_structure_annotation_Cs_C.:

.. das:function:: make_structure_annotation(name: string const; someClassPtr: auto const)

make_structure_annotation returns  :ref:`StructureAnnotationPtr <alias-StructureAnnotationPtr>` 

+------------+-------------+
+argument    +argument type+
+============+=============+
+name        +string const +
+------------+-------------+
+someClassPtr+auto const   +
+------------+-------------+


|function-ast-make_structure_annotation|

.. _function-_at_ast_c__c_make_enumeration_annotation_Cs_C.:

.. das:function:: make_enumeration_annotation(name: string const; someClassPtr: auto const)

make_enumeration_annotation returns  :ref:`EnumerationAnnotationPtr <alias-EnumerationAnnotationPtr>` 

+------------+-------------+
+argument    +argument type+
+============+=============+
+name        +string const +
+------------+-------------+
+someClassPtr+auto const   +
+------------+-------------+


|function-ast-make_enumeration_annotation|

.. _function-_at_ast_c__c_make_visitor_C.:

.. das:function:: make_visitor(someClass: auto const)

make_visitor returns smart_ptr< :ref:`ast::VisitorAdapter <handle-ast-VisitorAdapter>` >

+---------+-------------+
+argument +argument type+
+=========+=============+
+someClass+auto const   +
+---------+-------------+


|function-ast-make_visitor|

.. _function-_at_ast_c__c_make_reader_macro_Cs_C.:

.. das:function:: make_reader_macro(name: string const; someClassPtr: auto const)

make_reader_macro returns  :ref:`ReaderMacroPtr <alias-ReaderMacroPtr>` 

+------------+-------------+
+argument    +argument type+
+============+=============+
+name        +string const +
+------------+-------------+
+someClassPtr+auto const   +
+------------+-------------+


|function-ast-make_reader_macro|

.. _function-_at_ast_c__c_make_comment_reader_Cs_C.:

.. das:function:: make_comment_reader(name: string const; someClassPtr: auto const)

make_comment_reader returns  :ref:`CommentReaderPtr <alias-CommentReaderPtr>` 

+------------+-------------+
+argument    +argument type+
+============+=============+
+name        +string const +
+------------+-------------+
+someClassPtr+auto const   +
+------------+-------------+


|function-ast-make_comment_reader|

.. _function-_at_ast_c__c_make_call_macro_Cs_C.:

.. das:function:: make_call_macro(name: string const; someClassPtr: auto const)

make_call_macro returns  :ref:`CallMacroPtr <alias-CallMacroPtr>` 

+------------+-------------+
+argument    +argument type+
+============+=============+
+name        +string const +
+------------+-------------+
+someClassPtr+auto const   +
+------------+-------------+


|function-ast-make_call_macro|

.. _function-_at_ast_c__c_make_typeinfo_macro_Cs_C.:

.. das:function:: make_typeinfo_macro(name: string const; someClassPtr: auto const)

make_typeinfo_macro returns  :ref:`TypeInfoMacroPtr <alias-TypeInfoMacroPtr>` 

+------------+-------------+
+argument    +argument type+
+============+=============+
+name        +string const +
+------------+-------------+
+someClassPtr+auto const   +
+------------+-------------+


|function-ast-make_typeinfo_macro|

.. _function-_at_ast_c__c_make_pass_macro_Cs_C.:

.. das:function:: make_pass_macro(name: string const; someClassPtr: auto const)

make_pass_macro returns  :ref:`PassMacroPtr <alias-PassMacroPtr>` 

+------------+-------------+
+argument    +argument type+
+============+=============+
+name        +string const +
+------------+-------------+
+someClassPtr+auto const   +
+------------+-------------+


|function-ast-make_pass_macro|

.. _function-_at_ast_c__c_make_variant_macro_Cs_C.:

.. das:function:: make_variant_macro(name: string const; someClassPtr: auto const)

make_variant_macro returns  :ref:`VariantMacroPtr <alias-VariantMacroPtr>` 

+------------+-------------+
+argument    +argument type+
+============+=============+
+name        +string const +
+------------+-------------+
+someClassPtr+auto const   +
+------------+-------------+


|function-ast-make_variant_macro|

.. _function-_at_ast_c__c_make_for_loop_macro_Cs_C.:

.. das:function:: make_for_loop_macro(name: string const; someClassPtr: auto const)

make_for_loop_macro returns  :ref:`ForLoopMacroPtr <alias-ForLoopMacroPtr>` 

+------------+-------------+
+argument    +argument type+
+============+=============+
+name        +string const +
+------------+-------------+
+someClassPtr+auto const   +
+------------+-------------+


|function-ast-make_for_loop_macro|

.. _function-_at_ast_c__c_make_capture_macro_Cs_C.:

.. das:function:: make_capture_macro(name: string const; someClassPtr: auto const)

make_capture_macro returns  :ref:`CaptureMacroPtr <alias-CaptureMacroPtr>` 

+------------+-------------+
+argument    +argument type+
+============+=============+
+name        +string const +
+------------+-------------+
+someClassPtr+auto const   +
+------------+-------------+


|function-ast-make_capture_macro|

.. _function-_at_ast_c__c_make_simulate_macro_Cs_C.:

.. das:function:: make_simulate_macro(name: string const; someClassPtr: auto const)

make_simulate_macro returns  :ref:`SimulateMacroPtr <alias-SimulateMacroPtr>` 

+------------+-------------+
+argument    +argument type+
+============+=============+
+name        +string const +
+------------+-------------+
+someClassPtr+auto const   +
+------------+-------------+


|function-ast-make_simulate_macro|

+++++++++++++++++++
Adapter application
+++++++++++++++++++

  *  :ref:`add_function_annotation (module:rtti::Module? const implicit;annotation:smart_ptr\<ast::FunctionAnnotation\>& implicit;context:__context const;at:__lineInfo const) : void <function-_at_ast_c__c_add_function_annotation_CI1_ls_H_ls_rtti_c__c_Module_gr__gr_?_&I1_ls_H_ls_ast_c__c_FunctionAnnotation_gr__gr_?W_C_c_C_l>` 
  *  :ref:`add_function_annotation (function:smart_ptr\<ast::Function\> const implicit;annotation:smart_ptr\<ast::FunctionAnnotation\>& implicit;context:__context const;at:__lineInfo const) : void <function-_at_ast_c__c_add_function_annotation_CI1_ls_H_ls_ast_c__c_Function_gr__gr_?M_&I1_ls_H_ls_ast_c__c_FunctionAnnotation_gr__gr_?W_C_c_C_l>` 
  *  :ref:`add_function_annotation (function:smart_ptr\<ast::Function\> const implicit;annotation:smart_ptr\<rtti::AnnotationDeclaration\>& implicit;context:__context const;at:__lineInfo const) : void <function-_at_ast_c__c_add_function_annotation_CI1_ls_H_ls_ast_c__c_Function_gr__gr_?M_&I1_ls_H_ls_rtti_c__c_AnnotationDeclaration_gr__gr_?M_C_c_C_l>` 
  *  :ref:`add_block_annotation (block:smart_ptr\<ast::ExprBlock\> const implicit;annotation:smart_ptr\<ast::FunctionAnnotation\>& implicit;context:__context const;at:__lineInfo const) : void <function-_at_ast_c__c_add_block_annotation_CI1_ls_H_ls_ast_c__c_ExprBlock_gr__gr_?M_&I1_ls_H_ls_ast_c__c_FunctionAnnotation_gr__gr_?W_C_c_C_l>` 
  *  :ref:`add_block_annotation (block:smart_ptr\<ast::ExprBlock\> const implicit;annotation:smart_ptr\<rtti::AnnotationDeclaration\>& implicit;context:__context const;at:__lineInfo const) : void <function-_at_ast_c__c_add_block_annotation_CI1_ls_H_ls_ast_c__c_ExprBlock_gr__gr_?M_&I1_ls_H_ls_rtti_c__c_AnnotationDeclaration_gr__gr_?M_C_c_C_l>` 
  *  :ref:`add_structure_annotation (module:rtti::Module? const implicit;annotation:smart_ptr\<ast::StructureAnnotation\>& implicit;context:__context const;at:__lineInfo const) : void <function-_at_ast_c__c_add_structure_annotation_CI1_ls_H_ls_rtti_c__c_Module_gr__gr_?_&I1_ls_H_ls_ast_c__c_StructureAnnotation_gr__gr_?W_C_c_C_l>` 
  *  :ref:`add_structure_annotation (structure:smart_ptr\<ast::Structure\> const implicit;annotation:smart_ptr\<ast::StructureAnnotation\>& implicit;context:__context const;at:__lineInfo const) : void <function-_at_ast_c__c_add_structure_annotation_CI1_ls_H_ls_ast_c__c_Structure_gr__gr_?M_&I1_ls_H_ls_ast_c__c_StructureAnnotation_gr__gr_?W_C_c_C_l>` 
  *  :ref:`add_structure_annotation (structure:smart_ptr\<ast::Structure\> const implicit;annotation:smart_ptr\<rtti::AnnotationDeclaration\>& implicit;context:__context const;at:__lineInfo const) : void <function-_at_ast_c__c_add_structure_annotation_CI1_ls_H_ls_ast_c__c_Structure_gr__gr_?M_&I1_ls_H_ls_rtti_c__c_AnnotationDeclaration_gr__gr_?M_C_c_C_l>` 
  *  :ref:`add_enumeration_annotation (module:rtti::Module? const implicit;annotation:smart_ptr\<ast::EnumerationAnnotation\>& implicit;context:__context const;at:__lineInfo const) : void <function-_at_ast_c__c_add_enumeration_annotation_CI1_ls_H_ls_rtti_c__c_Module_gr__gr_?_&I1_ls_H_ls_ast_c__c_EnumerationAnnotation_gr__gr_?W_C_c_C_l>` 
  *  :ref:`add_infer_macro (module:rtti::Module? const implicit;annotation:smart_ptr\<ast::PassMacro\>& implicit;context:__context const) : void <function-_at_ast_c__c_add_infer_macro_CI1_ls_H_ls_rtti_c__c_Module_gr__gr_?_&I1_ls_H_ls_ast_c__c_PassMacro_gr__gr_?W_C_c>` 
  *  :ref:`add_dirty_infer_macro (module:rtti::Module? const implicit;annotation:smart_ptr\<ast::PassMacro\>& implicit;context:__context const) : void <function-_at_ast_c__c_add_dirty_infer_macro_CI1_ls_H_ls_rtti_c__c_Module_gr__gr_?_&I1_ls_H_ls_ast_c__c_PassMacro_gr__gr_?W_C_c>` 
  *  :ref:`add_lint_macro (module:rtti::Module? const implicit;annotation:smart_ptr\<ast::PassMacro\>& implicit;context:__context const) : void <function-_at_ast_c__c_add_lint_macro_CI1_ls_H_ls_rtti_c__c_Module_gr__gr_?_&I1_ls_H_ls_ast_c__c_PassMacro_gr__gr_?W_C_c>` 
  *  :ref:`add_global_lint_macro (module:rtti::Module? const implicit;annotation:smart_ptr\<ast::PassMacro\>& implicit;context:__context const) : void <function-_at_ast_c__c_add_global_lint_macro_CI1_ls_H_ls_rtti_c__c_Module_gr__gr_?_&I1_ls_H_ls_ast_c__c_PassMacro_gr__gr_?W_C_c>` 
  *  :ref:`add_optimization_macro (module:rtti::Module? const implicit;annotation:smart_ptr\<ast::PassMacro\>& implicit;context:__context const) : void <function-_at_ast_c__c_add_optimization_macro_CI1_ls_H_ls_rtti_c__c_Module_gr__gr_?_&I1_ls_H_ls_ast_c__c_PassMacro_gr__gr_?W_C_c>` 
  *  :ref:`add_reader_macro (module:rtti::Module? const implicit;annotation:smart_ptr\<ast::ReaderMacro\>& implicit;context:__context const;at:__lineInfo const) : void <function-_at_ast_c__c_add_reader_macro_CI1_ls_H_ls_rtti_c__c_Module_gr__gr_?_&I1_ls_H_ls_ast_c__c_ReaderMacro_gr__gr_?W_C_c_C_l>` 
  *  :ref:`add_comment_reader (module:rtti::Module? const implicit;reader:smart_ptr\<ast::CommentReader\>& implicit;context:__context const;at:__lineInfo const) : void <function-_at_ast_c__c_add_comment_reader_CI1_ls_H_ls_rtti_c__c_Module_gr__gr_?_&I1_ls_H_ls_ast_c__c_CommentReader_gr__gr_?W_C_c_C_l>` 
  *  :ref:`add_call_macro (module:rtti::Module? const implicit;annotation:smart_ptr\<ast::CallMacro\>& implicit;context:__context const;at:__lineInfo const) : void <function-_at_ast_c__c_add_call_macro_CI1_ls_H_ls_rtti_c__c_Module_gr__gr_?_&I1_ls_H_ls_ast_c__c_CallMacro_gr__gr_?W_C_c_C_l>` 
  *  :ref:`add_typeinfo_macro (module:rtti::Module? const implicit;annotation:smart_ptr\<ast::TypeInfoMacro\>& implicit;context:__context const;at:__lineInfo const) : void <function-_at_ast_c__c_add_typeinfo_macro_CI1_ls_H_ls_rtti_c__c_Module_gr__gr_?_&I1_ls_H_ls_ast_c__c_TypeInfoMacro_gr__gr_?W_C_c_C_l>` 
  *  :ref:`add_variant_macro (module:rtti::Module? const implicit;annotation:smart_ptr\<ast::VariantMacro\>& implicit;context:__context const) : void <function-_at_ast_c__c_add_variant_macro_CI1_ls_H_ls_rtti_c__c_Module_gr__gr_?_&I1_ls_H_ls_ast_c__c_VariantMacro_gr__gr_?W_C_c>` 
  *  :ref:`add_for_loop_macro (module:rtti::Module? const implicit;annotation:smart_ptr\<ast::ForLoopMacro\>& implicit;context:__context const) : void <function-_at_ast_c__c_add_for_loop_macro_CI1_ls_H_ls_rtti_c__c_Module_gr__gr_?_&I1_ls_H_ls_ast_c__c_ForLoopMacro_gr__gr_?W_C_c>` 
  *  :ref:`add_capture_macro (module:rtti::Module? const implicit;annotation:smart_ptr\<ast::CaptureMacro\>& implicit;context:__context const) : void <function-_at_ast_c__c_add_capture_macro_CI1_ls_H_ls_rtti_c__c_Module_gr__gr_?_&I1_ls_H_ls_ast_c__c_CaptureMacro_gr__gr_?W_C_c>` 
  *  :ref:`add_simulate_macro (module:rtti::Module? const implicit;annotation:smart_ptr\<ast::SimulateMacro\>& implicit;context:__context const) : void <function-_at_ast_c__c_add_simulate_macro_CI1_ls_H_ls_rtti_c__c_Module_gr__gr_?_&I1_ls_H_ls_ast_c__c_SimulateMacro_gr__gr_?W_C_c>` 
  *  :ref:`add_module_option (module:rtti::Module? const implicit;option:string const implicit;type:rtti::Type const;context:__context const;at:__lineInfo const) : void <function-_at_ast_c__c_add_module_option_CI1_ls_H_ls_rtti_c__c_Module_gr__gr_?_CIs_CE_ls_rtti_c__c_Type_gr__C_c_C_l>` 
  *  :ref:`add_new_block_annotation (name:string const;someClassPtr:auto const) : auto <function-_at_ast_c__c_add_new_block_annotation_Cs_C.>` 
  *  :ref:`add_new_function_annotation (name:string const;someClassPtr:auto const) : auto <function-_at_ast_c__c_add_new_function_annotation_Cs_C.>` 
  *  :ref:`add_new_contract_annotation (name:string const;someClassPtr:auto const) : auto <function-_at_ast_c__c_add_new_contract_annotation_Cs_C.>` 
  *  :ref:`add_new_structure_annotation (name:string const;someClassPtr:auto const) : auto <function-_at_ast_c__c_add_new_structure_annotation_Cs_C.>` 
  *  :ref:`add_new_enumeration_annotation (name:string const;someClassPtr:auto const) : auto <function-_at_ast_c__c_add_new_enumeration_annotation_Cs_C.>` 
  *  :ref:`add_new_variant_macro (name:string const;someClassPtr:auto const) : auto <function-_at_ast_c__c_add_new_variant_macro_Cs_C.>` 
  *  :ref:`add_new_for_loop_macro (name:string const;someClassPtr:auto const) : auto <function-_at_ast_c__c_add_new_for_loop_macro_Cs_C.>` 
  *  :ref:`add_new_capture_macro (name:string const;someClassPtr:auto const) : auto <function-_at_ast_c__c_add_new_capture_macro_Cs_C.>` 
  *  :ref:`add_new_simulate_macro (name:string const;someClassPtr:auto const) : auto <function-_at_ast_c__c_add_new_simulate_macro_Cs_C.>` 
  *  :ref:`add_new_reader_macro (name:string const;someClassPtr:auto const) : auto <function-_at_ast_c__c_add_new_reader_macro_Cs_C.>` 
  *  :ref:`add_new_comment_reader (name:string const;someClassPtr:auto const) : auto <function-_at_ast_c__c_add_new_comment_reader_Cs_C.>` 
  *  :ref:`add_new_call_macro (name:string const;someClassPtr:auto const) : auto <function-_at_ast_c__c_add_new_call_macro_Cs_C.>` 
  *  :ref:`add_new_typeinfo_macro (name:string const;someClassPtr:auto const) : auto <function-_at_ast_c__c_add_new_typeinfo_macro_Cs_C.>` 
  *  :ref:`add_new_infer_macro (name:string const;someClassPtr:auto const) : auto <function-_at_ast_c__c_add_new_infer_macro_Cs_C.>` 
  *  :ref:`add_new_dirty_infer_macro (name:string const;someClassPtr:auto const) : auto <function-_at_ast_c__c_add_new_dirty_infer_macro_Cs_C.>` 
  *  :ref:`add_new_lint_macro (name:string const;someClassPtr:auto const) : auto <function-_at_ast_c__c_add_new_lint_macro_Cs_C.>` 
  *  :ref:`add_new_global_lint_macro (name:string const;someClassPtr:auto const) : auto <function-_at_ast_c__c_add_new_global_lint_macro_Cs_C.>` 
  *  :ref:`add_new_optimization_macro (name:string const;someClassPtr:auto const) : auto <function-_at_ast_c__c_add_new_optimization_macro_Cs_C.>` 

.. _function-_at_ast_c__c_add_function_annotation_CI1_ls_H_ls_rtti_c__c_Module_gr__gr_?_&I1_ls_H_ls_ast_c__c_FunctionAnnotation_gr__gr_?W_C_c_C_l:

.. das:function:: add_function_annotation(module: rtti::Module? const implicit; annotation: smart_ptr<ast::FunctionAnnotation>& implicit)

+----------+-------------------------------------------------------------------------------------+
+argument  +argument type                                                                        +
+==========+=====================================================================================+
+module    + :ref:`rtti::Module <handle-rtti-Module>` ? const implicit                           +
+----------+-------------------------------------------------------------------------------------+
+annotation+smart_ptr< :ref:`ast::FunctionAnnotation <handle-ast-FunctionAnnotation>` >& implicit+
+----------+-------------------------------------------------------------------------------------+


|function-ast-add_function_annotation|

.. _function-_at_ast_c__c_add_function_annotation_CI1_ls_H_ls_ast_c__c_Function_gr__gr_?M_&I1_ls_H_ls_ast_c__c_FunctionAnnotation_gr__gr_?W_C_c_C_l:

.. das:function:: add_function_annotation(function: smart_ptr<ast::Function> const implicit; annotation: smart_ptr<ast::FunctionAnnotation>& implicit)

+----------+-------------------------------------------------------------------------------------+
+argument  +argument type                                                                        +
+==========+=====================================================================================+
+function  +smart_ptr< :ref:`ast::Function <handle-ast-Function>` > const implicit               +
+----------+-------------------------------------------------------------------------------------+
+annotation+smart_ptr< :ref:`ast::FunctionAnnotation <handle-ast-FunctionAnnotation>` >& implicit+
+----------+-------------------------------------------------------------------------------------+


|function-ast-add_function_annotation|

.. _function-_at_ast_c__c_add_function_annotation_CI1_ls_H_ls_ast_c__c_Function_gr__gr_?M_&I1_ls_H_ls_rtti_c__c_AnnotationDeclaration_gr__gr_?M_C_c_C_l:

.. das:function:: add_function_annotation(function: smart_ptr<ast::Function> const implicit; annotation: smart_ptr<rtti::AnnotationDeclaration>& implicit)

+----------+---------------------------------------------------------------------------------------------+
+argument  +argument type                                                                                +
+==========+=============================================================================================+
+function  +smart_ptr< :ref:`ast::Function <handle-ast-Function>` > const implicit                       +
+----------+---------------------------------------------------------------------------------------------+
+annotation+smart_ptr< :ref:`rtti::AnnotationDeclaration <handle-rtti-AnnotationDeclaration>` >& implicit+
+----------+---------------------------------------------------------------------------------------------+


|function-ast-add_function_annotation|

.. _function-_at_ast_c__c_add_block_annotation_CI1_ls_H_ls_ast_c__c_ExprBlock_gr__gr_?M_&I1_ls_H_ls_ast_c__c_FunctionAnnotation_gr__gr_?W_C_c_C_l:

.. das:function:: add_block_annotation(block: smart_ptr<ast::ExprBlock> const implicit; annotation: smart_ptr<ast::FunctionAnnotation>& implicit)

+----------+-------------------------------------------------------------------------------------+
+argument  +argument type                                                                        +
+==========+=====================================================================================+
+block     +smart_ptr< :ref:`ast::ExprBlock <handle-ast-ExprBlock>` > const implicit             +
+----------+-------------------------------------------------------------------------------------+
+annotation+smart_ptr< :ref:`ast::FunctionAnnotation <handle-ast-FunctionAnnotation>` >& implicit+
+----------+-------------------------------------------------------------------------------------+


|function-ast-add_block_annotation|

.. _function-_at_ast_c__c_add_block_annotation_CI1_ls_H_ls_ast_c__c_ExprBlock_gr__gr_?M_&I1_ls_H_ls_rtti_c__c_AnnotationDeclaration_gr__gr_?M_C_c_C_l:

.. das:function:: add_block_annotation(block: smart_ptr<ast::ExprBlock> const implicit; annotation: smart_ptr<rtti::AnnotationDeclaration>& implicit)

+----------+---------------------------------------------------------------------------------------------+
+argument  +argument type                                                                                +
+==========+=============================================================================================+
+block     +smart_ptr< :ref:`ast::ExprBlock <handle-ast-ExprBlock>` > const implicit                     +
+----------+---------------------------------------------------------------------------------------------+
+annotation+smart_ptr< :ref:`rtti::AnnotationDeclaration <handle-rtti-AnnotationDeclaration>` >& implicit+
+----------+---------------------------------------------------------------------------------------------+


|function-ast-add_block_annotation|

.. _function-_at_ast_c__c_add_structure_annotation_CI1_ls_H_ls_rtti_c__c_Module_gr__gr_?_&I1_ls_H_ls_ast_c__c_StructureAnnotation_gr__gr_?W_C_c_C_l:

.. das:function:: add_structure_annotation(module: rtti::Module? const implicit; annotation: smart_ptr<ast::StructureAnnotation>& implicit)

+----------+---------------------------------------------------------------------------------------+
+argument  +argument type                                                                          +
+==========+=======================================================================================+
+module    + :ref:`rtti::Module <handle-rtti-Module>` ? const implicit                             +
+----------+---------------------------------------------------------------------------------------+
+annotation+smart_ptr< :ref:`ast::StructureAnnotation <handle-ast-StructureAnnotation>` >& implicit+
+----------+---------------------------------------------------------------------------------------+


|function-ast-add_structure_annotation|

.. _function-_at_ast_c__c_add_structure_annotation_CI1_ls_H_ls_ast_c__c_Structure_gr__gr_?M_&I1_ls_H_ls_ast_c__c_StructureAnnotation_gr__gr_?W_C_c_C_l:

.. das:function:: add_structure_annotation(structure: smart_ptr<ast::Structure> const implicit; annotation: smart_ptr<ast::StructureAnnotation>& implicit)

+----------+---------------------------------------------------------------------------------------+
+argument  +argument type                                                                          +
+==========+=======================================================================================+
+structure +smart_ptr< :ref:`ast::Structure <handle-ast-Structure>` > const implicit               +
+----------+---------------------------------------------------------------------------------------+
+annotation+smart_ptr< :ref:`ast::StructureAnnotation <handle-ast-StructureAnnotation>` >& implicit+
+----------+---------------------------------------------------------------------------------------+


|function-ast-add_structure_annotation|

.. _function-_at_ast_c__c_add_structure_annotation_CI1_ls_H_ls_ast_c__c_Structure_gr__gr_?M_&I1_ls_H_ls_rtti_c__c_AnnotationDeclaration_gr__gr_?M_C_c_C_l:

.. das:function:: add_structure_annotation(structure: smart_ptr<ast::Structure> const implicit; annotation: smart_ptr<rtti::AnnotationDeclaration>& implicit)

+----------+---------------------------------------------------------------------------------------------+
+argument  +argument type                                                                                +
+==========+=============================================================================================+
+structure +smart_ptr< :ref:`ast::Structure <handle-ast-Structure>` > const implicit                     +
+----------+---------------------------------------------------------------------------------------------+
+annotation+smart_ptr< :ref:`rtti::AnnotationDeclaration <handle-rtti-AnnotationDeclaration>` >& implicit+
+----------+---------------------------------------------------------------------------------------------+


|function-ast-add_structure_annotation|

.. _function-_at_ast_c__c_add_enumeration_annotation_CI1_ls_H_ls_rtti_c__c_Module_gr__gr_?_&I1_ls_H_ls_ast_c__c_EnumerationAnnotation_gr__gr_?W_C_c_C_l:

.. das:function:: add_enumeration_annotation(module: rtti::Module? const implicit; annotation: smart_ptr<ast::EnumerationAnnotation>& implicit)

+----------+-------------------------------------------------------------------------------------------+
+argument  +argument type                                                                              +
+==========+===========================================================================================+
+module    + :ref:`rtti::Module <handle-rtti-Module>` ? const implicit                                 +
+----------+-------------------------------------------------------------------------------------------+
+annotation+smart_ptr< :ref:`ast::EnumerationAnnotation <handle-ast-EnumerationAnnotation>` >& implicit+
+----------+-------------------------------------------------------------------------------------------+


|function-ast-add_enumeration_annotation|

.. _function-_at_ast_c__c_add_infer_macro_CI1_ls_H_ls_rtti_c__c_Module_gr__gr_?_&I1_ls_H_ls_ast_c__c_PassMacro_gr__gr_?W_C_c:

.. das:function:: add_infer_macro(module: rtti::Module? const implicit; annotation: smart_ptr<ast::PassMacro>& implicit)

+----------+-------------------------------------------------------------------+
+argument  +argument type                                                      +
+==========+===================================================================+
+module    + :ref:`rtti::Module <handle-rtti-Module>` ? const implicit         +
+----------+-------------------------------------------------------------------+
+annotation+smart_ptr< :ref:`ast::PassMacro <handle-ast-PassMacro>` >& implicit+
+----------+-------------------------------------------------------------------+


|function-ast-add_infer_macro|

.. _function-_at_ast_c__c_add_dirty_infer_macro_CI1_ls_H_ls_rtti_c__c_Module_gr__gr_?_&I1_ls_H_ls_ast_c__c_PassMacro_gr__gr_?W_C_c:

.. das:function:: add_dirty_infer_macro(module: rtti::Module? const implicit; annotation: smart_ptr<ast::PassMacro>& implicit)

+----------+-------------------------------------------------------------------+
+argument  +argument type                                                      +
+==========+===================================================================+
+module    + :ref:`rtti::Module <handle-rtti-Module>` ? const implicit         +
+----------+-------------------------------------------------------------------+
+annotation+smart_ptr< :ref:`ast::PassMacro <handle-ast-PassMacro>` >& implicit+
+----------+-------------------------------------------------------------------+


|function-ast-add_dirty_infer_macro|

.. _function-_at_ast_c__c_add_lint_macro_CI1_ls_H_ls_rtti_c__c_Module_gr__gr_?_&I1_ls_H_ls_ast_c__c_PassMacro_gr__gr_?W_C_c:

.. das:function:: add_lint_macro(module: rtti::Module? const implicit; annotation: smart_ptr<ast::PassMacro>& implicit)

+----------+-------------------------------------------------------------------+
+argument  +argument type                                                      +
+==========+===================================================================+
+module    + :ref:`rtti::Module <handle-rtti-Module>` ? const implicit         +
+----------+-------------------------------------------------------------------+
+annotation+smart_ptr< :ref:`ast::PassMacro <handle-ast-PassMacro>` >& implicit+
+----------+-------------------------------------------------------------------+


|function-ast-add_lint_macro|

.. _function-_at_ast_c__c_add_global_lint_macro_CI1_ls_H_ls_rtti_c__c_Module_gr__gr_?_&I1_ls_H_ls_ast_c__c_PassMacro_gr__gr_?W_C_c:

.. das:function:: add_global_lint_macro(module: rtti::Module? const implicit; annotation: smart_ptr<ast::PassMacro>& implicit)

+----------+-------------------------------------------------------------------+
+argument  +argument type                                                      +
+==========+===================================================================+
+module    + :ref:`rtti::Module <handle-rtti-Module>` ? const implicit         +
+----------+-------------------------------------------------------------------+
+annotation+smart_ptr< :ref:`ast::PassMacro <handle-ast-PassMacro>` >& implicit+
+----------+-------------------------------------------------------------------+


|function-ast-add_global_lint_macro|

.. _function-_at_ast_c__c_add_optimization_macro_CI1_ls_H_ls_rtti_c__c_Module_gr__gr_?_&I1_ls_H_ls_ast_c__c_PassMacro_gr__gr_?W_C_c:

.. das:function:: add_optimization_macro(module: rtti::Module? const implicit; annotation: smart_ptr<ast::PassMacro>& implicit)

+----------+-------------------------------------------------------------------+
+argument  +argument type                                                      +
+==========+===================================================================+
+module    + :ref:`rtti::Module <handle-rtti-Module>` ? const implicit         +
+----------+-------------------------------------------------------------------+
+annotation+smart_ptr< :ref:`ast::PassMacro <handle-ast-PassMacro>` >& implicit+
+----------+-------------------------------------------------------------------+


|function-ast-add_optimization_macro|

.. _function-_at_ast_c__c_add_reader_macro_CI1_ls_H_ls_rtti_c__c_Module_gr__gr_?_&I1_ls_H_ls_ast_c__c_ReaderMacro_gr__gr_?W_C_c_C_l:

.. das:function:: add_reader_macro(module: rtti::Module? const implicit; annotation: smart_ptr<ast::ReaderMacro>& implicit)

+----------+-----------------------------------------------------------------------+
+argument  +argument type                                                          +
+==========+=======================================================================+
+module    + :ref:`rtti::Module <handle-rtti-Module>` ? const implicit             +
+----------+-----------------------------------------------------------------------+
+annotation+smart_ptr< :ref:`ast::ReaderMacro <handle-ast-ReaderMacro>` >& implicit+
+----------+-----------------------------------------------------------------------+


|function-ast-add_reader_macro|

.. _function-_at_ast_c__c_add_comment_reader_CI1_ls_H_ls_rtti_c__c_Module_gr__gr_?_&I1_ls_H_ls_ast_c__c_CommentReader_gr__gr_?W_C_c_C_l:

.. das:function:: add_comment_reader(module: rtti::Module? const implicit; reader: smart_ptr<ast::CommentReader>& implicit)

+--------+---------------------------------------------------------------------------+
+argument+argument type                                                              +
+========+===========================================================================+
+module  + :ref:`rtti::Module <handle-rtti-Module>` ? const implicit                 +
+--------+---------------------------------------------------------------------------+
+reader  +smart_ptr< :ref:`ast::CommentReader <handle-ast-CommentReader>` >& implicit+
+--------+---------------------------------------------------------------------------+


|function-ast-add_comment_reader|

.. _function-_at_ast_c__c_add_call_macro_CI1_ls_H_ls_rtti_c__c_Module_gr__gr_?_&I1_ls_H_ls_ast_c__c_CallMacro_gr__gr_?W_C_c_C_l:

.. das:function:: add_call_macro(module: rtti::Module? const implicit; annotation: smart_ptr<ast::CallMacro>& implicit)

+----------+-------------------------------------------------------------------+
+argument  +argument type                                                      +
+==========+===================================================================+
+module    + :ref:`rtti::Module <handle-rtti-Module>` ? const implicit         +
+----------+-------------------------------------------------------------------+
+annotation+smart_ptr< :ref:`ast::CallMacro <handle-ast-CallMacro>` >& implicit+
+----------+-------------------------------------------------------------------+


|function-ast-add_call_macro|

.. _function-_at_ast_c__c_add_typeinfo_macro_CI1_ls_H_ls_rtti_c__c_Module_gr__gr_?_&I1_ls_H_ls_ast_c__c_TypeInfoMacro_gr__gr_?W_C_c_C_l:

.. das:function:: add_typeinfo_macro(module: rtti::Module? const implicit; annotation: smart_ptr<ast::TypeInfoMacro>& implicit)

+----------+---------------------------------------------------------------------------+
+argument  +argument type                                                              +
+==========+===========================================================================+
+module    + :ref:`rtti::Module <handle-rtti-Module>` ? const implicit                 +
+----------+---------------------------------------------------------------------------+
+annotation+smart_ptr< :ref:`ast::TypeInfoMacro <handle-ast-TypeInfoMacro>` >& implicit+
+----------+---------------------------------------------------------------------------+


|function-ast-add_typeinfo_macro|

.. _function-_at_ast_c__c_add_variant_macro_CI1_ls_H_ls_rtti_c__c_Module_gr__gr_?_&I1_ls_H_ls_ast_c__c_VariantMacro_gr__gr_?W_C_c:

.. das:function:: add_variant_macro(module: rtti::Module? const implicit; annotation: smart_ptr<ast::VariantMacro>& implicit)

+----------+-------------------------------------------------------------------------+
+argument  +argument type                                                            +
+==========+=========================================================================+
+module    + :ref:`rtti::Module <handle-rtti-Module>` ? const implicit               +
+----------+-------------------------------------------------------------------------+
+annotation+smart_ptr< :ref:`ast::VariantMacro <handle-ast-VariantMacro>` >& implicit+
+----------+-------------------------------------------------------------------------+


|function-ast-add_variant_macro|

.. _function-_at_ast_c__c_add_for_loop_macro_CI1_ls_H_ls_rtti_c__c_Module_gr__gr_?_&I1_ls_H_ls_ast_c__c_ForLoopMacro_gr__gr_?W_C_c:

.. das:function:: add_for_loop_macro(module: rtti::Module? const implicit; annotation: smart_ptr<ast::ForLoopMacro>& implicit)

+----------+-------------------------------------------------------------------------+
+argument  +argument type                                                            +
+==========+=========================================================================+
+module    + :ref:`rtti::Module <handle-rtti-Module>` ? const implicit               +
+----------+-------------------------------------------------------------------------+
+annotation+smart_ptr< :ref:`ast::ForLoopMacro <handle-ast-ForLoopMacro>` >& implicit+
+----------+-------------------------------------------------------------------------+


|function-ast-add_for_loop_macro|

.. _function-_at_ast_c__c_add_capture_macro_CI1_ls_H_ls_rtti_c__c_Module_gr__gr_?_&I1_ls_H_ls_ast_c__c_CaptureMacro_gr__gr_?W_C_c:

.. das:function:: add_capture_macro(module: rtti::Module? const implicit; annotation: smart_ptr<ast::CaptureMacro>& implicit)

+----------+-------------------------------------------------------------------------+
+argument  +argument type                                                            +
+==========+=========================================================================+
+module    + :ref:`rtti::Module <handle-rtti-Module>` ? const implicit               +
+----------+-------------------------------------------------------------------------+
+annotation+smart_ptr< :ref:`ast::CaptureMacro <handle-ast-CaptureMacro>` >& implicit+
+----------+-------------------------------------------------------------------------+


|function-ast-add_capture_macro|

.. _function-_at_ast_c__c_add_simulate_macro_CI1_ls_H_ls_rtti_c__c_Module_gr__gr_?_&I1_ls_H_ls_ast_c__c_SimulateMacro_gr__gr_?W_C_c:

.. das:function:: add_simulate_macro(module: rtti::Module? const implicit; annotation: smart_ptr<ast::SimulateMacro>& implicit)

+----------+---------------------------------------------------------------------------+
+argument  +argument type                                                              +
+==========+===========================================================================+
+module    + :ref:`rtti::Module <handle-rtti-Module>` ? const implicit                 +
+----------+---------------------------------------------------------------------------+
+annotation+smart_ptr< :ref:`ast::SimulateMacro <handle-ast-SimulateMacro>` >& implicit+
+----------+---------------------------------------------------------------------------+


|function-ast-add_simulate_macro|

.. _function-_at_ast_c__c_add_module_option_CI1_ls_H_ls_rtti_c__c_Module_gr__gr_?_CIs_CE_ls_rtti_c__c_Type_gr__C_c_C_l:

.. das:function:: add_module_option(module: rtti::Module? const implicit; option: string const implicit; type: Type const)

+--------+----------------------------------------------------------+
+argument+argument type                                             +
+========+==========================================================+
+module  + :ref:`rtti::Module <handle-rtti-Module>` ? const implicit+
+--------+----------------------------------------------------------+
+option  +string const implicit                                     +
+--------+----------------------------------------------------------+
+type    + :ref:`rtti::Type <enum-rtti-Type>`  const                +
+--------+----------------------------------------------------------+


|function-ast-add_module_option|

.. _function-_at_ast_c__c_add_new_block_annotation_Cs_C.:

.. das:function:: add_new_block_annotation(name: string const; someClassPtr: auto const)

add_new_block_annotation returns auto

+------------+-------------+
+argument    +argument type+
+============+=============+
+name        +string const +
+------------+-------------+
+someClassPtr+auto const   +
+------------+-------------+


|function-ast-add_new_block_annotation|

.. _function-_at_ast_c__c_add_new_function_annotation_Cs_C.:

.. das:function:: add_new_function_annotation(name: string const; someClassPtr: auto const)

add_new_function_annotation returns auto

+------------+-------------+
+argument    +argument type+
+============+=============+
+name        +string const +
+------------+-------------+
+someClassPtr+auto const   +
+------------+-------------+


|function-ast-add_new_function_annotation|

.. _function-_at_ast_c__c_add_new_contract_annotation_Cs_C.:

.. das:function:: add_new_contract_annotation(name: string const; someClassPtr: auto const)

add_new_contract_annotation returns auto

+------------+-------------+
+argument    +argument type+
+============+=============+
+name        +string const +
+------------+-------------+
+someClassPtr+auto const   +
+------------+-------------+


|function-ast-add_new_contract_annotation|

.. _function-_at_ast_c__c_add_new_structure_annotation_Cs_C.:

.. das:function:: add_new_structure_annotation(name: string const; someClassPtr: auto const)

add_new_structure_annotation returns auto

+------------+-------------+
+argument    +argument type+
+============+=============+
+name        +string const +
+------------+-------------+
+someClassPtr+auto const   +
+------------+-------------+


|function-ast-add_new_structure_annotation|

.. _function-_at_ast_c__c_add_new_enumeration_annotation_Cs_C.:

.. das:function:: add_new_enumeration_annotation(name: string const; someClassPtr: auto const)

add_new_enumeration_annotation returns auto

+------------+-------------+
+argument    +argument type+
+============+=============+
+name        +string const +
+------------+-------------+
+someClassPtr+auto const   +
+------------+-------------+


|function-ast-add_new_enumeration_annotation|

.. _function-_at_ast_c__c_add_new_variant_macro_Cs_C.:

.. das:function:: add_new_variant_macro(name: string const; someClassPtr: auto const)

add_new_variant_macro returns auto

+------------+-------------+
+argument    +argument type+
+============+=============+
+name        +string const +
+------------+-------------+
+someClassPtr+auto const   +
+------------+-------------+


|function-ast-add_new_variant_macro|

.. _function-_at_ast_c__c_add_new_for_loop_macro_Cs_C.:

.. das:function:: add_new_for_loop_macro(name: string const; someClassPtr: auto const)

add_new_for_loop_macro returns auto

+------------+-------------+
+argument    +argument type+
+============+=============+
+name        +string const +
+------------+-------------+
+someClassPtr+auto const   +
+------------+-------------+


|function-ast-add_new_for_loop_macro|

.. _function-_at_ast_c__c_add_new_capture_macro_Cs_C.:

.. das:function:: add_new_capture_macro(name: string const; someClassPtr: auto const)

add_new_capture_macro returns auto

+------------+-------------+
+argument    +argument type+
+============+=============+
+name        +string const +
+------------+-------------+
+someClassPtr+auto const   +
+------------+-------------+


|function-ast-add_new_capture_macro|

.. _function-_at_ast_c__c_add_new_simulate_macro_Cs_C.:

.. das:function:: add_new_simulate_macro(name: string const; someClassPtr: auto const)

add_new_simulate_macro returns auto

+------------+-------------+
+argument    +argument type+
+============+=============+
+name        +string const +
+------------+-------------+
+someClassPtr+auto const   +
+------------+-------------+


|function-ast-add_new_simulate_macro|

.. _function-_at_ast_c__c_add_new_reader_macro_Cs_C.:

.. das:function:: add_new_reader_macro(name: string const; someClassPtr: auto const)

add_new_reader_macro returns auto

+------------+-------------+
+argument    +argument type+
+============+=============+
+name        +string const +
+------------+-------------+
+someClassPtr+auto const   +
+------------+-------------+


|function-ast-add_new_reader_macro|

.. _function-_at_ast_c__c_add_new_comment_reader_Cs_C.:

.. das:function:: add_new_comment_reader(name: string const; someClassPtr: auto const)

add_new_comment_reader returns auto

+------------+-------------+
+argument    +argument type+
+============+=============+
+name        +string const +
+------------+-------------+
+someClassPtr+auto const   +
+------------+-------------+


|function-ast-add_new_comment_reader|

.. _function-_at_ast_c__c_add_new_call_macro_Cs_C.:

.. das:function:: add_new_call_macro(name: string const; someClassPtr: auto const)

add_new_call_macro returns auto

+------------+-------------+
+argument    +argument type+
+============+=============+
+name        +string const +
+------------+-------------+
+someClassPtr+auto const   +
+------------+-------------+


|function-ast-add_new_call_macro|

.. _function-_at_ast_c__c_add_new_typeinfo_macro_Cs_C.:

.. das:function:: add_new_typeinfo_macro(name: string const; someClassPtr: auto const)

add_new_typeinfo_macro returns auto

+------------+-------------+
+argument    +argument type+
+============+=============+
+name        +string const +
+------------+-------------+
+someClassPtr+auto const   +
+------------+-------------+


|function-ast-add_new_typeinfo_macro|

.. _function-_at_ast_c__c_add_new_infer_macro_Cs_C.:

.. das:function:: add_new_infer_macro(name: string const; someClassPtr: auto const)

add_new_infer_macro returns auto

+------------+-------------+
+argument    +argument type+
+============+=============+
+name        +string const +
+------------+-------------+
+someClassPtr+auto const   +
+------------+-------------+


|function-ast-add_new_infer_macro|

.. _function-_at_ast_c__c_add_new_dirty_infer_macro_Cs_C.:

.. das:function:: add_new_dirty_infer_macro(name: string const; someClassPtr: auto const)

add_new_dirty_infer_macro returns auto

+------------+-------------+
+argument    +argument type+
+============+=============+
+name        +string const +
+------------+-------------+
+someClassPtr+auto const   +
+------------+-------------+


|function-ast-add_new_dirty_infer_macro|

.. _function-_at_ast_c__c_add_new_lint_macro_Cs_C.:

.. das:function:: add_new_lint_macro(name: string const; someClassPtr: auto const)

add_new_lint_macro returns auto

+------------+-------------+
+argument    +argument type+
+============+=============+
+name        +string const +
+------------+-------------+
+someClassPtr+auto const   +
+------------+-------------+


|function-ast-add_new_lint_macro|

.. _function-_at_ast_c__c_add_new_global_lint_macro_Cs_C.:

.. das:function:: add_new_global_lint_macro(name: string const; someClassPtr: auto const)

add_new_global_lint_macro returns auto

+------------+-------------+
+argument    +argument type+
+============+=============+
+name        +string const +
+------------+-------------+
+someClassPtr+auto const   +
+------------+-------------+


|function-ast-add_new_global_lint_macro|

.. _function-_at_ast_c__c_add_new_optimization_macro_Cs_C.:

.. das:function:: add_new_optimization_macro(name: string const; someClassPtr: auto const)

add_new_optimization_macro returns auto

+------------+-------------+
+argument    +argument type+
+============+=============+
+name        +string const +
+------------+-------------+
+someClassPtr+auto const   +
+------------+-------------+


|function-ast-add_new_optimization_macro|

+++++++++++++++++++++++++
Adding objects to objects
+++++++++++++++++++++++++

  *  :ref:`add_enumeration_entry (enum:smart_ptr\<ast::Enumeration\> const implicit;name:string const implicit) : int <function-_at_ast_c__c_add_enumeration_entry_CI1_ls_H_ls_ast_c__c_Enumeration_gr__gr_?W_CIs>` 
  *  :ref:`add_function (module:rtti::Module? const implicit;function:smart_ptr\<ast::Function\>& implicit;context:__context const;line:__lineInfo const) : bool <function-_at_ast_c__c_add_function_CI1_ls_H_ls_rtti_c__c_Module_gr__gr_?_&I1_ls_H_ls_ast_c__c_Function_gr__gr_?W_C_c_C_l>` 
  *  :ref:`add_generic (module:rtti::Module? const implicit;function:smart_ptr\<ast::Function\>& implicit;context:__context const;line:__lineInfo const) : bool <function-_at_ast_c__c_add_generic_CI1_ls_H_ls_rtti_c__c_Module_gr__gr_?_&I1_ls_H_ls_ast_c__c_Function_gr__gr_?W_C_c_C_l>` 
  *  :ref:`add_variable (module:rtti::Module? const implicit;variable:smart_ptr\<ast::Variable\>& implicit;context:__context const;line:__lineInfo const) : bool <function-_at_ast_c__c_add_variable_CI1_ls_H_ls_rtti_c__c_Module_gr__gr_?_&I1_ls_H_ls_ast_c__c_Variable_gr__gr_?W_C_c_C_l>` 
  *  :ref:`add_keyword (module:rtti::Module? const implicit;keyword:string const implicit;needOxfordComma:bool const;context:__context const;line:__lineInfo const) : bool <function-_at_ast_c__c_add_keyword_CI1_ls_H_ls_rtti_c__c_Module_gr__gr_?_CIs_Cb_C_c_C_l>` 
  *  :ref:`add_structure (module:rtti::Module? const implicit;structure:smart_ptr\<ast::Structure\>& implicit) : bool <function-_at_ast_c__c_add_structure_CI1_ls_H_ls_rtti_c__c_Module_gr__gr_?_&I1_ls_H_ls_ast_c__c_Structure_gr__gr_?W>` 
  *  :ref:`add_alias (module:rtti::Module? const implicit;structure:smart_ptr\<ast::TypeDecl\>& implicit) : bool <function-_at_ast_c__c_add_alias_CI1_ls_H_ls_rtti_c__c_Module_gr__gr_?_&I1_ls_H_ls_ast_c__c_TypeDecl_gr__gr_?W>` 
  *  :ref:`add_module_require (module:rtti::Module? const implicit;publicModule:rtti::Module? const implicit;pub:bool const) : void <function-_at_ast_c__c_add_module_require_CI1_ls_H_ls_rtti_c__c_Module_gr__gr_?_CI1_ls_H_ls_rtti_c__c_Module_gr__gr_?_Cb>` 

.. _function-_at_ast_c__c_add_enumeration_entry_CI1_ls_H_ls_ast_c__c_Enumeration_gr__gr_?W_CIs:

.. das:function:: add_enumeration_entry(enum: smart_ptr<ast::Enumeration> const implicit; name: string const implicit)

add_enumeration_entry returns int

+--------+----------------------------------------------------------------------------+
+argument+argument type                                                               +
+========+============================================================================+
+enum    +smart_ptr< :ref:`ast::Enumeration <handle-ast-Enumeration>` > const implicit+
+--------+----------------------------------------------------------------------------+
+name    +string const implicit                                                       +
+--------+----------------------------------------------------------------------------+


|function-ast-add_enumeration_entry|

.. _function-_at_ast_c__c_add_function_CI1_ls_H_ls_rtti_c__c_Module_gr__gr_?_&I1_ls_H_ls_ast_c__c_Function_gr__gr_?W_C_c_C_l:

.. das:function:: add_function(module: rtti::Module? const implicit; function: smart_ptr<ast::Function>& implicit)

add_function returns bool

+--------+-----------------------------------------------------------------+
+argument+argument type                                                    +
+========+=================================================================+
+module  + :ref:`rtti::Module <handle-rtti-Module>` ? const implicit       +
+--------+-----------------------------------------------------------------+
+function+smart_ptr< :ref:`ast::Function <handle-ast-Function>` >& implicit+
+--------+-----------------------------------------------------------------+


|function-ast-add_function|

.. _function-_at_ast_c__c_add_generic_CI1_ls_H_ls_rtti_c__c_Module_gr__gr_?_&I1_ls_H_ls_ast_c__c_Function_gr__gr_?W_C_c_C_l:

.. das:function:: add_generic(module: rtti::Module? const implicit; function: smart_ptr<ast::Function>& implicit)

add_generic returns bool

+--------+-----------------------------------------------------------------+
+argument+argument type                                                    +
+========+=================================================================+
+module  + :ref:`rtti::Module <handle-rtti-Module>` ? const implicit       +
+--------+-----------------------------------------------------------------+
+function+smart_ptr< :ref:`ast::Function <handle-ast-Function>` >& implicit+
+--------+-----------------------------------------------------------------+


|function-ast-add_generic|

.. _function-_at_ast_c__c_add_variable_CI1_ls_H_ls_rtti_c__c_Module_gr__gr_?_&I1_ls_H_ls_ast_c__c_Variable_gr__gr_?W_C_c_C_l:

.. das:function:: add_variable(module: rtti::Module? const implicit; variable: smart_ptr<ast::Variable>& implicit)

add_variable returns bool

+--------+-----------------------------------------------------------------+
+argument+argument type                                                    +
+========+=================================================================+
+module  + :ref:`rtti::Module <handle-rtti-Module>` ? const implicit       +
+--------+-----------------------------------------------------------------+
+variable+smart_ptr< :ref:`ast::Variable <handle-ast-Variable>` >& implicit+
+--------+-----------------------------------------------------------------+


|function-ast-add_variable|

.. _function-_at_ast_c__c_add_keyword_CI1_ls_H_ls_rtti_c__c_Module_gr__gr_?_CIs_Cb_C_c_C_l:

.. das:function:: add_keyword(module: rtti::Module? const implicit; keyword: string const implicit; needOxfordComma: bool const)

add_keyword returns bool

+---------------+----------------------------------------------------------+
+argument       +argument type                                             +
+===============+==========================================================+
+module         + :ref:`rtti::Module <handle-rtti-Module>` ? const implicit+
+---------------+----------------------------------------------------------+
+keyword        +string const implicit                                     +
+---------------+----------------------------------------------------------+
+needOxfordComma+bool const                                                +
+---------------+----------------------------------------------------------+


|function-ast-add_keyword|

.. _function-_at_ast_c__c_add_structure_CI1_ls_H_ls_rtti_c__c_Module_gr__gr_?_&I1_ls_H_ls_ast_c__c_Structure_gr__gr_?W:

.. das:function:: add_structure(module: rtti::Module? const implicit; structure: smart_ptr<ast::Structure>& implicit)

add_structure returns bool

+---------+-------------------------------------------------------------------+
+argument +argument type                                                      +
+=========+===================================================================+
+module   + :ref:`rtti::Module <handle-rtti-Module>` ? const implicit         +
+---------+-------------------------------------------------------------------+
+structure+smart_ptr< :ref:`ast::Structure <handle-ast-Structure>` >& implicit+
+---------+-------------------------------------------------------------------+


|function-ast-add_structure|

.. _function-_at_ast_c__c_add_alias_CI1_ls_H_ls_rtti_c__c_Module_gr__gr_?_&I1_ls_H_ls_ast_c__c_TypeDecl_gr__gr_?W:

.. das:function:: add_alias(module: rtti::Module? const implicit; structure: smart_ptr<ast::TypeDecl>& implicit)

add_alias returns bool

+---------+-----------------------------------------------------------------+
+argument +argument type                                                    +
+=========+=================================================================+
+module   + :ref:`rtti::Module <handle-rtti-Module>` ? const implicit       +
+---------+-----------------------------------------------------------------+
+structure+smart_ptr< :ref:`ast::TypeDecl <handle-ast-TypeDecl>` >& implicit+
+---------+-----------------------------------------------------------------+


|function-ast-add_alias|

.. _function-_at_ast_c__c_add_module_require_CI1_ls_H_ls_rtti_c__c_Module_gr__gr_?_CI1_ls_H_ls_rtti_c__c_Module_gr__gr_?_Cb:

.. das:function:: add_module_require(module: rtti::Module? const implicit; publicModule: rtti::Module? const implicit; pub: bool const)

+------------+----------------------------------------------------------+
+argument    +argument type                                             +
+============+==========================================================+
+module      + :ref:`rtti::Module <handle-rtti-Module>` ? const implicit+
+------------+----------------------------------------------------------+
+publicModule+ :ref:`rtti::Module <handle-rtti-Module>` ? const implicit+
+------------+----------------------------------------------------------+
+pub         +bool const                                                +
+------------+----------------------------------------------------------+


|function-ast-add_module_require|

+++++++++++++++++++++++++
Program and module access
+++++++++++++++++++++++++

  *  :ref:`this_program (context:__context const) : smart_ptr\<rtti::Program\> <function-_at_ast_c__c_this_program_C_c>` 
  *  :ref:`this_module (context:__context const;line:__lineInfo const) : rtti::Module? <function-_at_ast_c__c_this_module_C_c_C_l>` 
  *  :ref:`compiling_program (context:__context const;at:__lineInfo const) : smart_ptr\<rtti::Program\> <function-_at_ast_c__c_compiling_program_C_c_C_l>` 
  *  :ref:`compiling_module (context:__context const;at:__lineInfo const) : rtti::Module? <function-_at_ast_c__c_compiling_module_C_c_C_l>` 

.. _function-_at_ast_c__c_this_program_C_c:

.. das:function:: this_program()

this_program returns smart_ptr< :ref:`rtti::Program <handle-rtti-Program>` >

|function-ast-this_program|

.. _function-_at_ast_c__c_this_module_C_c_C_l:

.. das:function:: this_module()

this_module returns  :ref:`rtti::Module <handle-rtti-Module>` ?

|function-ast-this_module|

.. _function-_at_ast_c__c_compiling_program_C_c_C_l:

.. das:function:: compiling_program()

compiling_program returns smart_ptr< :ref:`rtti::Program <handle-rtti-Program>` >

|function-ast-compiling_program|

.. _function-_at_ast_c__c_compiling_module_C_c_C_l:

.. das:function:: compiling_module()

compiling_module returns  :ref:`rtti::Module <handle-rtti-Module>` ?

|function-ast-compiling_module|

+++++++++++++++++++++++++++++++++++
Textual descriptions of the objects
+++++++++++++++++++++++++++++++++++

  *  :ref:`describe_typedecl (type:smart_ptr\<ast::TypeDecl\> const implicit;extra:bool const;contracts:bool const;module:bool const;context:__context const;lineinfo:__lineInfo const) : string <function-_at_ast_c__c_describe_typedecl_CI1_ls_H_ls_ast_c__c_TypeDecl_gr__gr_?M_Cb_Cb_Cb_C_c_C_l>` 
  *  :ref:`describe_typedecl_cpp (type:smart_ptr\<ast::TypeDecl\> const implicit;substitueRef:bool const;skipRef:bool const;skipConst:bool const;redundantConst:bool const;context:__context const;lineinfo:__lineInfo const) : string <function-_at_ast_c__c_describe_typedecl_cpp_CI1_ls_H_ls_ast_c__c_TypeDecl_gr__gr_?M_Cb_Cb_Cb_Cb_C_c_C_l>` 
  *  :ref:`describe_expression (expression:smart_ptr\<ast::Expression\> const implicit;context:__context const;lineinfo:__lineInfo const) : string <function-_at_ast_c__c_describe_expression_CI1_ls_H_ls_ast_c__c_Expression_gr__gr_?M_C_c_C_l>` 
  *  :ref:`describe_function (function:smart_ptr\<ast::Function\> const implicit;context:__context const;lineinfo:__lineInfo const) : string <function-_at_ast_c__c_describe_function_CI1_ls_H_ls_ast_c__c_Function_gr__gr_?M_C_c_C_l>` 
  *  :ref:`das_to_string (type:rtti::Type const;context:__context const) : string <function-_at_ast_c__c_das_to_string_CE_ls_rtti_c__c_Type_gr__C_c>` 
  *  :ref:`describe (decl:smart_ptr\<ast::TypeDecl\> const;extra:bool const;contracts:bool const;modules:bool const) : auto <function-_at_ast_c__c_describe_C1_ls_H_ls_ast_c__c_TypeDecl_gr__gr_?M_Cb_Cb_Cb>` 
  *  :ref:`describe_cpp (decl:smart_ptr\<ast::TypeDecl\> const;substitureRef:bool const;skipRef:bool const;skipConst:bool const;redundantConst:bool const) : auto <function-_at_ast_c__c_describe_cpp_C1_ls_H_ls_ast_c__c_TypeDecl_gr__gr_?M_Cb_Cb_Cb_Cb>` 
  *  :ref:`describe (expr:smart_ptr\<ast::Expression\> const) : auto <function-_at_ast_c__c_describe_C1_ls_H_ls_ast_c__c_Expression_gr__gr_?M>` 
  *  :ref:`describe (expr:smart_ptr\<ast::Function\> const) : auto <function-_at_ast_c__c_describe_C1_ls_H_ls_ast_c__c_Function_gr__gr_?M>` 

.. _function-_at_ast_c__c_describe_typedecl_CI1_ls_H_ls_ast_c__c_TypeDecl_gr__gr_?M_Cb_Cb_Cb_C_c_C_l:

.. das:function:: describe_typedecl(type: smart_ptr<ast::TypeDecl> const implicit; extra: bool const; contracts: bool const; module: bool const)

describe_typedecl returns string

+---------+----------------------------------------------------------------------+
+argument +argument type                                                         +
+=========+======================================================================+
+type     +smart_ptr< :ref:`ast::TypeDecl <handle-ast-TypeDecl>` > const implicit+
+---------+----------------------------------------------------------------------+
+extra    +bool const                                                            +
+---------+----------------------------------------------------------------------+
+contracts+bool const                                                            +
+---------+----------------------------------------------------------------------+
+module   +bool const                                                            +
+---------+----------------------------------------------------------------------+


|function-ast-describe_typedecl|

.. _function-_at_ast_c__c_describe_typedecl_cpp_CI1_ls_H_ls_ast_c__c_TypeDecl_gr__gr_?M_Cb_Cb_Cb_Cb_C_c_C_l:

.. das:function:: describe_typedecl_cpp(type: smart_ptr<ast::TypeDecl> const implicit; substitueRef: bool const; skipRef: bool const; skipConst: bool const; redundantConst: bool const)

describe_typedecl_cpp returns string

+--------------+----------------------------------------------------------------------+
+argument      +argument type                                                         +
+==============+======================================================================+
+type          +smart_ptr< :ref:`ast::TypeDecl <handle-ast-TypeDecl>` > const implicit+
+--------------+----------------------------------------------------------------------+
+substitueRef  +bool const                                                            +
+--------------+----------------------------------------------------------------------+
+skipRef       +bool const                                                            +
+--------------+----------------------------------------------------------------------+
+skipConst     +bool const                                                            +
+--------------+----------------------------------------------------------------------+
+redundantConst+bool const                                                            +
+--------------+----------------------------------------------------------------------+


|function-ast-describe_typedecl_cpp|

.. _function-_at_ast_c__c_describe_expression_CI1_ls_H_ls_ast_c__c_Expression_gr__gr_?M_C_c_C_l:

.. das:function:: describe_expression(expression: smart_ptr<ast::Expression> const implicit)

describe_expression returns string

+----------+--------------------------------------------------------------------------+
+argument  +argument type                                                             +
+==========+==========================================================================+
+expression+smart_ptr< :ref:`ast::Expression <handle-ast-Expression>` > const implicit+
+----------+--------------------------------------------------------------------------+


|function-ast-describe_expression|

.. _function-_at_ast_c__c_describe_function_CI1_ls_H_ls_ast_c__c_Function_gr__gr_?M_C_c_C_l:

.. das:function:: describe_function(function: smart_ptr<ast::Function> const implicit)

describe_function returns string

+--------+----------------------------------------------------------------------+
+argument+argument type                                                         +
+========+======================================================================+
+function+smart_ptr< :ref:`ast::Function <handle-ast-Function>` > const implicit+
+--------+----------------------------------------------------------------------+


|function-ast-describe_function|

.. _function-_at_ast_c__c_das_to_string_CE_ls_rtti_c__c_Type_gr__C_c:

.. das:function:: das_to_string(type: Type const)

das_to_string returns string

+--------+------------------------------------------+
+argument+argument type                             +
+========+==========================================+
+type    + :ref:`rtti::Type <enum-rtti-Type>`  const+
+--------+------------------------------------------+


|function-ast-das_to_string|

.. _function-_at_ast_c__c_describe_C1_ls_H_ls_ast_c__c_TypeDecl_gr__gr_?M_Cb_Cb_Cb:

.. das:function:: describe(decl: smart_ptr<ast::TypeDecl> const; extra: bool const; contracts: bool const; modules: bool const)

describe returns auto

+---------+-------------------------------------------------------------+
+argument +argument type                                                +
+=========+=============================================================+
+decl     +smart_ptr< :ref:`ast::TypeDecl <handle-ast-TypeDecl>` > const+
+---------+-------------------------------------------------------------+
+extra    +bool const                                                   +
+---------+-------------------------------------------------------------+
+contracts+bool const                                                   +
+---------+-------------------------------------------------------------+
+modules  +bool const                                                   +
+---------+-------------------------------------------------------------+


|function-ast-describe|

.. _function-_at_ast_c__c_describe_cpp_C1_ls_H_ls_ast_c__c_TypeDecl_gr__gr_?M_Cb_Cb_Cb_Cb:

.. das:function:: describe_cpp(decl: smart_ptr<ast::TypeDecl> const; substitureRef: bool const; skipRef: bool const; skipConst: bool const; redundantConst: bool const)

describe_cpp returns auto

+--------------+-------------------------------------------------------------+
+argument      +argument type                                                +
+==============+=============================================================+
+decl          +smart_ptr< :ref:`ast::TypeDecl <handle-ast-TypeDecl>` > const+
+--------------+-------------------------------------------------------------+
+substitureRef +bool const                                                   +
+--------------+-------------------------------------------------------------+
+skipRef       +bool const                                                   +
+--------------+-------------------------------------------------------------+
+skipConst     +bool const                                                   +
+--------------+-------------------------------------------------------------+
+redundantConst+bool const                                                   +
+--------------+-------------------------------------------------------------+


|function-ast-describe_cpp|

.. _function-_at_ast_c__c_describe_C1_ls_H_ls_ast_c__c_Expression_gr__gr_?M:

.. das:function:: describe(expr: smart_ptr<ast::Expression> const)

describe returns auto

+--------+-----------------------------------------------------------------+
+argument+argument type                                                    +
+========+=================================================================+
+expr    +smart_ptr< :ref:`ast::Expression <handle-ast-Expression>` > const+
+--------+-----------------------------------------------------------------+


|function-ast-describe|

.. _function-_at_ast_c__c_describe_C1_ls_H_ls_ast_c__c_Function_gr__gr_?M:

.. das:function:: describe(expr: smart_ptr<ast::Function> const)

describe returns auto

+--------+-------------------------------------------------------------+
+argument+argument type                                                +
+========+=============================================================+
+expr    +smart_ptr< :ref:`ast::Function <handle-ast-Function>` > const+
+--------+-------------------------------------------------------------+


|function-ast-describe|

+++++++++
Searching
+++++++++

  *  :ref:`find_module_via_rtti (program:smart_ptr\<rtti::Program\> const implicit;name:string const implicit;context:__context const;lineinfo:__lineInfo const) : rtti::Module? <function-_at_ast_c__c_find_module_via_rtti_CI1_ls_H_ls_rtti_c__c_Program_gr__gr_?W_CIs_C_c_C_l>` 
  *  :ref:`find_module_function_via_rtti (module:rtti::Module? const implicit;function:function\<\> const;context:__context const;lineinfo:__lineInfo const) : smart_ptr\<ast::Function\> <function-_at_ast_c__c_find_module_function_via_rtti_CI1_ls_H_ls_rtti_c__c_Module_gr__gr_?_C_at__at__C_c_C_l>` 
  *  :ref:`find_variable (module:rtti::Module? const implicit;variable:string const implicit) : smart_ptr\<ast::Variable\> <function-_at_ast_c__c_find_variable_CI1_ls_H_ls_rtti_c__c_Module_gr__gr_?_CIs>` 
  *  :ref:`find_bitfield_name (bit:smart_ptr\<ast::TypeDecl\> const implicit;value:bitfield const;context:__context const;lineinfo:__lineInfo const) : string <function-_at_ast_c__c_find_bitfield_name_CI1_ls_H_ls_ast_c__c_TypeDecl_gr__gr_?M_Ct_C_c_C_l>` 
  *  :ref:`find_enum_value (enum:smart_ptr\<ast::Enumeration\> const implicit;value:string const implicit) : int64 <function-_at_ast_c__c_find_enum_value_CI1_ls_H_ls_ast_c__c_Enumeration_gr__gr_?W_CIs>` 
  *  :ref:`find_structure_field (structPtr:ast::Structure? const implicit;field:string const implicit;context:__context const;lineinfo:__lineInfo const) : ast::FieldDeclaration? <function-_at_ast_c__c_find_structure_field_CI1_ls_H_ls_ast_c__c_Structure_gr__gr_?_CIs_C_c_C_l>` 
  *  :ref:`find_unique_structure (program:smart_ptr\<rtti::Program\> const implicit;name:string const implicit;context:__context const;at:__lineInfo const) : ast::Structure? <function-_at_ast_c__c_find_unique_structure_CI1_ls_H_ls_rtti_c__c_Program_gr__gr_?M_CIs_C_c_C_l>` 
  *  :ref:`find_module (prog:smart_ptr\<rtti::Program\> const;name:string const) : rtti::Module? <function-_at_ast_c__c_find_module_C1_ls_H_ls_rtti_c__c_Program_gr__gr_?M_Cs>` 
  *  :ref:`find_module (name:string const) : rtti::Module? <function-_at_ast_c__c_find_module_Cs>` 
  *  :ref:`find_compiling_module (name:string const) : rtti::Module? <function-_at_ast_c__c_find_compiling_module_Cs>` 

.. _function-_at_ast_c__c_find_module_via_rtti_CI1_ls_H_ls_rtti_c__c_Program_gr__gr_?W_CIs_C_c_C_l:

.. das:function:: find_module_via_rtti(program: smart_ptr<rtti::Program> const implicit; name: string const implicit)

find_module_via_rtti returns  :ref:`rtti::Module <handle-rtti-Module>` ?

+--------+----------------------------------------------------------------------+
+argument+argument type                                                         +
+========+======================================================================+
+program +smart_ptr< :ref:`rtti::Program <handle-rtti-Program>` > const implicit+
+--------+----------------------------------------------------------------------+
+name    +string const implicit                                                 +
+--------+----------------------------------------------------------------------+


|function-ast-find_module_via_rtti|

.. _function-_at_ast_c__c_find_module_function_via_rtti_CI1_ls_H_ls_rtti_c__c_Module_gr__gr_?_C_at__at__C_c_C_l:

.. das:function:: find_module_function_via_rtti(module: rtti::Module? const implicit; function: function<> const)

find_module_function_via_rtti returns smart_ptr< :ref:`ast::Function <handle-ast-Function>` >

+--------+----------------------------------------------------------+
+argument+argument type                                             +
+========+==========================================================+
+module  + :ref:`rtti::Module <handle-rtti-Module>` ? const implicit+
+--------+----------------------------------------------------------+
+function+function<> const                                          +
+--------+----------------------------------------------------------+


|function-ast-find_module_function_via_rtti|

.. _function-_at_ast_c__c_find_variable_CI1_ls_H_ls_rtti_c__c_Module_gr__gr_?_CIs:

.. das:function:: find_variable(module: rtti::Module? const implicit; variable: string const implicit)

find_variable returns smart_ptr< :ref:`ast::Variable <handle-ast-Variable>` >

+--------+----------------------------------------------------------+
+argument+argument type                                             +
+========+==========================================================+
+module  + :ref:`rtti::Module <handle-rtti-Module>` ? const implicit+
+--------+----------------------------------------------------------+
+variable+string const implicit                                     +
+--------+----------------------------------------------------------+


|function-ast-find_variable|

.. _function-_at_ast_c__c_find_bitfield_name_CI1_ls_H_ls_ast_c__c_TypeDecl_gr__gr_?M_Ct_C_c_C_l:

.. das:function:: find_bitfield_name(bit: smart_ptr<ast::TypeDecl> const implicit; value: bitfield const)

find_bitfield_name returns string

+--------+----------------------------------------------------------------------+
+argument+argument type                                                         +
+========+======================================================================+
+bit     +smart_ptr< :ref:`ast::TypeDecl <handle-ast-TypeDecl>` > const implicit+
+--------+----------------------------------------------------------------------+
+value   +bitfield<> const                                                      +
+--------+----------------------------------------------------------------------+


|function-ast-find_bitfield_name|

.. _function-_at_ast_c__c_find_enum_value_CI1_ls_H_ls_ast_c__c_Enumeration_gr__gr_?W_CIs:

.. das:function:: find_enum_value(enum: smart_ptr<ast::Enumeration> const implicit; value: string const implicit)

find_enum_value returns int64

+--------+----------------------------------------------------------------------------+
+argument+argument type                                                               +
+========+============================================================================+
+enum    +smart_ptr< :ref:`ast::Enumeration <handle-ast-Enumeration>` > const implicit+
+--------+----------------------------------------------------------------------------+
+value   +string const implicit                                                       +
+--------+----------------------------------------------------------------------------+


|function-ast-find_enum_value|

.. _function-_at_ast_c__c_find_structure_field_CI1_ls_H_ls_ast_c__c_Structure_gr__gr_?_CIs_C_c_C_l:

.. das:function:: find_structure_field(structPtr: ast::Structure? const implicit; field: string const implicit)

find_structure_field returns  :ref:`ast::FieldDeclaration <handle-ast-FieldDeclaration>` ?

+---------+--------------------------------------------------------------+
+argument +argument type                                                 +
+=========+==============================================================+
+structPtr+ :ref:`ast::Structure <handle-ast-Structure>` ? const implicit+
+---------+--------------------------------------------------------------+
+field    +string const implicit                                         +
+---------+--------------------------------------------------------------+


|function-ast-find_structure_field|

.. _function-_at_ast_c__c_find_unique_structure_CI1_ls_H_ls_rtti_c__c_Program_gr__gr_?M_CIs_C_c_C_l:

.. das:function:: find_unique_structure(program: smart_ptr<rtti::Program> const implicit; name: string const implicit)

find_unique_structure returns  :ref:`ast::Structure <handle-ast-Structure>` ?

+--------+----------------------------------------------------------------------+
+argument+argument type                                                         +
+========+======================================================================+
+program +smart_ptr< :ref:`rtti::Program <handle-rtti-Program>` > const implicit+
+--------+----------------------------------------------------------------------+
+name    +string const implicit                                                 +
+--------+----------------------------------------------------------------------+


|function-ast-find_unique_structure|

.. _function-_at_ast_c__c_find_module_C1_ls_H_ls_rtti_c__c_Program_gr__gr_?M_Cs:

.. das:function:: find_module(prog: smart_ptr<rtti::Program> const; name: string const)

find_module returns  :ref:`rtti::Module <handle-rtti-Module>` ?

+--------+-------------------------------------------------------------+
+argument+argument type                                                +
+========+=============================================================+
+prog    +smart_ptr< :ref:`rtti::Program <handle-rtti-Program>` > const+
+--------+-------------------------------------------------------------+
+name    +string const                                                 +
+--------+-------------------------------------------------------------+


|function-ast-find_module|

.. _function-_at_ast_c__c_find_module_Cs:

.. das:function:: find_module(name: string const)

find_module returns  :ref:`rtti::Module <handle-rtti-Module>` ?

+--------+-------------+
+argument+argument type+
+========+=============+
+name    +string const +
+--------+-------------+


|function-ast-find_module|

.. _function-_at_ast_c__c_find_compiling_module_Cs:

.. das:function:: find_compiling_module(name: string const)

find_compiling_module returns  :ref:`rtti::Module <handle-rtti-Module>` ?

+--------+-------------+
+argument+argument type+
+========+=============+
+name    +string const +
+--------+-------------+


|function-ast-find_compiling_module|

+++++++++
Iterating
+++++++++

  *  :ref:`for_each_module (program:rtti::Program? const implicit;block:block\<(var arg0:rtti::Module?):void\> const implicit;context:__context const;line:__lineInfo const) : void <function-_at_ast_c__c_for_each_module_CI1_ls_H_ls_rtti_c__c_Program_gr__gr_?_CI0_ls_1_ls_H_ls_rtti_c__c_Module_gr__gr_?_gr_1_ls_v_gr__builtin__C_c_C_l>` 
  *  :ref:`for_each_function (module:rtti::Module? const implicit;name:string const implicit;block:block\<(var arg0:smart_ptr\<ast::Function\>):void\> const implicit;context:__context const;line:__lineInfo const) : void <function-_at_ast_c__c_for_each_function_CI1_ls_H_ls_rtti_c__c_Module_gr__gr_?_CIs_CI0_ls_1_ls_H_ls_ast_c__c_Function_gr__gr_?W_gr_1_ls_v_gr__builtin__C_c_C_l>` 
  *  :ref:`for_each_generic (module:rtti::Module? const implicit;name:string const implicit;block:block\<(var arg0:smart_ptr\<ast::Function\>):void\> const implicit;context:__context const;line:__lineInfo const) : void <function-_at_ast_c__c_for_each_generic_CI1_ls_H_ls_rtti_c__c_Module_gr__gr_?_CIs_CI0_ls_1_ls_H_ls_ast_c__c_Function_gr__gr_?W_gr_1_ls_v_gr__builtin__C_c_C_l>` 
  *  :ref:`any_table_foreach (table:void? const implicit;keyStride:int const;valueStride:int const;block:block\<(var arg0:void?;var arg1:void?):void\> const implicit;context:__context const;line:__lineInfo const) : void <function-_at_ast_c__c_any_table_foreach_CI?_Ci_Ci_CI0_ls_?;?_gr_1_ls_v_gr__builtin__C_c_C_l>` 
  *  :ref:`any_array_foreach (array:void? const implicit;stride:int const;block:block\<(var arg0:void?):void\> const implicit;context:__context const;line:__lineInfo const) : void <function-_at_ast_c__c_any_array_foreach_CI?_Ci_CI0_ls_?_gr_1_ls_v_gr__builtin__C_c_C_l>` 
  *  :ref:`for_each_typedef (module:rtti::Module? const implicit;block:block\<(var arg0:string#;var arg1:smart_ptr\<ast::TypeDecl\>):void\> const implicit;context:__context const;line:__lineInfo const) : void <function-_at_ast_c__c_for_each_typedef_CI1_ls_H_ls_rtti_c__c_Module_gr__gr_?_CI0_ls__hh_s;1_ls_H_ls_ast_c__c_TypeDecl_gr__gr_?W_gr_1_ls_v_gr__builtin__C_c_C_l>` 
  *  :ref:`for_each_enumeration (module:rtti::Module? const implicit;block:block\<(var arg0:smart_ptr\<ast::Enumeration\>):void\> const implicit;context:__context const;line:__lineInfo const) : void <function-_at_ast_c__c_for_each_enumeration_CI1_ls_H_ls_rtti_c__c_Module_gr__gr_?_CI0_ls_1_ls_H_ls_ast_c__c_Enumeration_gr__gr_?W_gr_1_ls_v_gr__builtin__C_c_C_l>` 
  *  :ref:`for_each_structure (module:rtti::Module? const implicit;block:block\<(var arg0:smart_ptr\<ast::Structure\>):void\> const implicit;context:__context const;line:__lineInfo const) : void <function-_at_ast_c__c_for_each_structure_CI1_ls_H_ls_rtti_c__c_Module_gr__gr_?_CI0_ls_1_ls_H_ls_ast_c__c_Structure_gr__gr_?W_gr_1_ls_v_gr__builtin__C_c_C_l>` 
  *  :ref:`for_each_generic (module:rtti::Module? const implicit;block:block\<(var arg0:smart_ptr\<ast::Function\>):void\> const implicit;context:__context const;line:__lineInfo const) : void <function-_at_ast_c__c_for_each_generic_CI1_ls_H_ls_rtti_c__c_Module_gr__gr_?_CI0_ls_1_ls_H_ls_ast_c__c_Function_gr__gr_?W_gr_1_ls_v_gr__builtin__C_c_C_l>` 
  *  :ref:`for_each_global (module:rtti::Module? const implicit;block:block\<(var arg0:smart_ptr\<ast::Variable\>):void\> const implicit;context:__context const;line:__lineInfo const) : void <function-_at_ast_c__c_for_each_global_CI1_ls_H_ls_rtti_c__c_Module_gr__gr_?_CI0_ls_1_ls_H_ls_ast_c__c_Variable_gr__gr_?W_gr_1_ls_v_gr__builtin__C_c_C_l>` 
  *  :ref:`for_each_call_macro (module:rtti::Module? const implicit;block:block\<(var arg0:string#):void\> const implicit;context:__context const;line:__lineInfo const) : void <function-_at_ast_c__c_for_each_call_macro_CI1_ls_H_ls_rtti_c__c_Module_gr__gr_?_CI0_ls__hh_s_gr_1_ls_v_gr__builtin__C_c_C_l>` 
  *  :ref:`for_each_reader_macro (module:rtti::Module? const implicit;block:block\<(var arg0:string#):void\> const implicit;context:__context const;line:__lineInfo const) : void <function-_at_ast_c__c_for_each_reader_macro_CI1_ls_H_ls_rtti_c__c_Module_gr__gr_?_CI0_ls__hh_s_gr_1_ls_v_gr__builtin__C_c_C_l>` 
  *  :ref:`for_each_variant_macro (module:rtti::Module? const implicit;block:block\<(var arg0:smart_ptr\<ast::VariantMacro\>):void\> const implicit;context:__context const;line:__lineInfo const) : void <function-_at_ast_c__c_for_each_variant_macro_CI1_ls_H_ls_rtti_c__c_Module_gr__gr_?_CI0_ls_1_ls_H_ls_ast_c__c_VariantMacro_gr__gr_?W_gr_1_ls_v_gr__builtin__C_c_C_l>` 
  *  :ref:`for_each_for_loop_macro (module:rtti::Module? const implicit;block:block\<(var arg0:smart_ptr\<ast::ForLoopMacro\>):void\> const implicit;context:__context const;line:__lineInfo const) : void <function-_at_ast_c__c_for_each_for_loop_macro_CI1_ls_H_ls_rtti_c__c_Module_gr__gr_?_CI0_ls_1_ls_H_ls_ast_c__c_ForLoopMacro_gr__gr_?W_gr_1_ls_v_gr__builtin__C_c_C_l>` 
  *  :ref:`for_each_typeinfo_macro (module:rtti::Module? const implicit;block:block\<(var arg0:smart_ptr\<ast::TypeInfoMacro\>):void\> const implicit;context:__context const;line:__lineInfo const) : void <function-_at_ast_c__c_for_each_typeinfo_macro_CI1_ls_H_ls_rtti_c__c_Module_gr__gr_?_CI0_ls_1_ls_H_ls_ast_c__c_TypeInfoMacro_gr__gr_?W_gr_1_ls_v_gr__builtin__C_c_C_l>` 
  *  :ref:`for_each_field (annotation:rtti::BasicStructureAnnotation const implicit;block:block\<(var arg0:string;var arg1:string;var arg2:smart_ptr\<ast::TypeDecl\>;var arg3:uint):void\> const implicit;context:__context const;line:__lineInfo const) : void <function-_at_ast_c__c_for_each_field_CIH_ls_rtti_c__c_BasicStructureAnnotation_gr__CI0_ls_s;s;1_ls_H_ls_ast_c__c_TypeDecl_gr__gr_?W;u_gr_1_ls_v_gr__builtin__C_c_C_l>` 

.. _function-_at_ast_c__c_for_each_module_CI1_ls_H_ls_rtti_c__c_Program_gr__gr_?_CI0_ls_1_ls_H_ls_rtti_c__c_Module_gr__gr_?_gr_1_ls_v_gr__builtin__C_c_C_l:

.. das:function:: for_each_module(program: rtti::Program? const implicit; block: block<(var arg0:rtti::Module?):void> const implicit)

+--------+------------------------------------------------------------------------+
+argument+argument type                                                           +
+========+========================================================================+
+program + :ref:`rtti::Program <handle-rtti-Program>` ? const implicit            +
+--------+------------------------------------------------------------------------+
+block   +block<( :ref:`rtti::Module <handle-rtti-Module>` ?):void> const implicit+
+--------+------------------------------------------------------------------------+


|function-ast-for_each_module|

.. _function-_at_ast_c__c_for_each_function_CI1_ls_H_ls_rtti_c__c_Module_gr__gr_?_CIs_CI0_ls_1_ls_H_ls_ast_c__c_Function_gr__gr_?W_gr_1_ls_v_gr__builtin__C_c_C_l:

.. das:function:: for_each_function(module: rtti::Module? const implicit; name: string const implicit; block: block<(var arg0:smart_ptr<ast::Function>):void> const implicit)

+--------+------------------------------------------------------------------------------------+
+argument+argument type                                                                       +
+========+====================================================================================+
+module  + :ref:`rtti::Module <handle-rtti-Module>` ? const implicit                          +
+--------+------------------------------------------------------------------------------------+
+name    +string const implicit                                                               +
+--------+------------------------------------------------------------------------------------+
+block   +block<(smart_ptr< :ref:`ast::Function <handle-ast-Function>` >):void> const implicit+
+--------+------------------------------------------------------------------------------------+


|function-ast-for_each_function|

.. _function-_at_ast_c__c_for_each_generic_CI1_ls_H_ls_rtti_c__c_Module_gr__gr_?_CIs_CI0_ls_1_ls_H_ls_ast_c__c_Function_gr__gr_?W_gr_1_ls_v_gr__builtin__C_c_C_l:

.. das:function:: for_each_generic(module: rtti::Module? const implicit; name: string const implicit; block: block<(var arg0:smart_ptr<ast::Function>):void> const implicit)

+--------+------------------------------------------------------------------------------------+
+argument+argument type                                                                       +
+========+====================================================================================+
+module  + :ref:`rtti::Module <handle-rtti-Module>` ? const implicit                          +
+--------+------------------------------------------------------------------------------------+
+name    +string const implicit                                                               +
+--------+------------------------------------------------------------------------------------+
+block   +block<(smart_ptr< :ref:`ast::Function <handle-ast-Function>` >):void> const implicit+
+--------+------------------------------------------------------------------------------------+


|function-ast-for_each_generic|

.. _function-_at_ast_c__c_any_table_foreach_CI?_Ci_Ci_CI0_ls_?;?_gr_1_ls_v_gr__builtin__C_c_C_l:

.. das:function:: any_table_foreach(table: void? const implicit; keyStride: int const; valueStride: int const; block: block<(var arg0:void?;var arg1:void?):void> const implicit)

+-----------+----------------------------------------+
+argument   +argument type                           +
+===========+========================================+
+table      +void? const implicit                    +
+-----------+----------------------------------------+
+keyStride  +int const                               +
+-----------+----------------------------------------+
+valueStride+int const                               +
+-----------+----------------------------------------+
+block      +block<(void?;void?):void> const implicit+
+-----------+----------------------------------------+


|function-ast-any_table_foreach|

.. _function-_at_ast_c__c_any_array_foreach_CI?_Ci_CI0_ls_?_gr_1_ls_v_gr__builtin__C_c_C_l:

.. das:function:: any_array_foreach(array: void? const implicit; stride: int const; block: block<(var arg0:void?):void> const implicit)

+--------+----------------------------------+
+argument+argument type                     +
+========+==================================+
+array   +void? const implicit              +
+--------+----------------------------------+
+stride  +int const                         +
+--------+----------------------------------+
+block   +block<(void?):void> const implicit+
+--------+----------------------------------+


|function-ast-any_array_foreach|

.. _function-_at_ast_c__c_for_each_typedef_CI1_ls_H_ls_rtti_c__c_Module_gr__gr_?_CI0_ls__hh_s;1_ls_H_ls_ast_c__c_TypeDecl_gr__gr_?W_gr_1_ls_v_gr__builtin__C_c_C_l:

.. das:function:: for_each_typedef(module: rtti::Module? const implicit; block: block<(var arg0:string#;var arg1:smart_ptr<ast::TypeDecl>):void> const implicit)

+--------+--------------------------------------------------------------------------------------------+
+argument+argument type                                                                               +
+========+============================================================================================+
+module  + :ref:`rtti::Module <handle-rtti-Module>` ? const implicit                                  +
+--------+--------------------------------------------------------------------------------------------+
+block   +block<(string#;smart_ptr< :ref:`ast::TypeDecl <handle-ast-TypeDecl>` >):void> const implicit+
+--------+--------------------------------------------------------------------------------------------+


|function-ast-for_each_typedef|

.. _function-_at_ast_c__c_for_each_enumeration_CI1_ls_H_ls_rtti_c__c_Module_gr__gr_?_CI0_ls_1_ls_H_ls_ast_c__c_Enumeration_gr__gr_?W_gr_1_ls_v_gr__builtin__C_c_C_l:

.. das:function:: for_each_enumeration(module: rtti::Module? const implicit; block: block<(var arg0:smart_ptr<ast::Enumeration>):void> const implicit)

+--------+------------------------------------------------------------------------------------------+
+argument+argument type                                                                             +
+========+==========================================================================================+
+module  + :ref:`rtti::Module <handle-rtti-Module>` ? const implicit                                +
+--------+------------------------------------------------------------------------------------------+
+block   +block<(smart_ptr< :ref:`ast::Enumeration <handle-ast-Enumeration>` >):void> const implicit+
+--------+------------------------------------------------------------------------------------------+


|function-ast-for_each_enumeration|

.. _function-_at_ast_c__c_for_each_structure_CI1_ls_H_ls_rtti_c__c_Module_gr__gr_?_CI0_ls_1_ls_H_ls_ast_c__c_Structure_gr__gr_?W_gr_1_ls_v_gr__builtin__C_c_C_l:

.. das:function:: for_each_structure(module: rtti::Module? const implicit; block: block<(var arg0:smart_ptr<ast::Structure>):void> const implicit)

+--------+--------------------------------------------------------------------------------------+
+argument+argument type                                                                         +
+========+======================================================================================+
+module  + :ref:`rtti::Module <handle-rtti-Module>` ? const implicit                            +
+--------+--------------------------------------------------------------------------------------+
+block   +block<(smart_ptr< :ref:`ast::Structure <handle-ast-Structure>` >):void> const implicit+
+--------+--------------------------------------------------------------------------------------+


|function-ast-for_each_structure|

.. _function-_at_ast_c__c_for_each_generic_CI1_ls_H_ls_rtti_c__c_Module_gr__gr_?_CI0_ls_1_ls_H_ls_ast_c__c_Function_gr__gr_?W_gr_1_ls_v_gr__builtin__C_c_C_l:

.. das:function:: for_each_generic(module: rtti::Module? const implicit; block: block<(var arg0:smart_ptr<ast::Function>):void> const implicit)

+--------+------------------------------------------------------------------------------------+
+argument+argument type                                                                       +
+========+====================================================================================+
+module  + :ref:`rtti::Module <handle-rtti-Module>` ? const implicit                          +
+--------+------------------------------------------------------------------------------------+
+block   +block<(smart_ptr< :ref:`ast::Function <handle-ast-Function>` >):void> const implicit+
+--------+------------------------------------------------------------------------------------+


|function-ast-for_each_generic|

.. _function-_at_ast_c__c_for_each_global_CI1_ls_H_ls_rtti_c__c_Module_gr__gr_?_CI0_ls_1_ls_H_ls_ast_c__c_Variable_gr__gr_?W_gr_1_ls_v_gr__builtin__C_c_C_l:

.. das:function:: for_each_global(module: rtti::Module? const implicit; block: block<(var arg0:smart_ptr<ast::Variable>):void> const implicit)

+--------+------------------------------------------------------------------------------------+
+argument+argument type                                                                       +
+========+====================================================================================+
+module  + :ref:`rtti::Module <handle-rtti-Module>` ? const implicit                          +
+--------+------------------------------------------------------------------------------------+
+block   +block<(smart_ptr< :ref:`ast::Variable <handle-ast-Variable>` >):void> const implicit+
+--------+------------------------------------------------------------------------------------+


|function-ast-for_each_global|

.. _function-_at_ast_c__c_for_each_call_macro_CI1_ls_H_ls_rtti_c__c_Module_gr__gr_?_CI0_ls__hh_s_gr_1_ls_v_gr__builtin__C_c_C_l:

.. das:function:: for_each_call_macro(module: rtti::Module? const implicit; block: block<(var arg0:string#):void> const implicit)

+--------+----------------------------------------------------------+
+argument+argument type                                             +
+========+==========================================================+
+module  + :ref:`rtti::Module <handle-rtti-Module>` ? const implicit+
+--------+----------------------------------------------------------+
+block   +block<(string#):void> const implicit                      +
+--------+----------------------------------------------------------+


|function-ast-for_each_call_macro|

.. _function-_at_ast_c__c_for_each_reader_macro_CI1_ls_H_ls_rtti_c__c_Module_gr__gr_?_CI0_ls__hh_s_gr_1_ls_v_gr__builtin__C_c_C_l:

.. das:function:: for_each_reader_macro(module: rtti::Module? const implicit; block: block<(var arg0:string#):void> const implicit)

+--------+----------------------------------------------------------+
+argument+argument type                                             +
+========+==========================================================+
+module  + :ref:`rtti::Module <handle-rtti-Module>` ? const implicit+
+--------+----------------------------------------------------------+
+block   +block<(string#):void> const implicit                      +
+--------+----------------------------------------------------------+


|function-ast-for_each_reader_macro|

.. _function-_at_ast_c__c_for_each_variant_macro_CI1_ls_H_ls_rtti_c__c_Module_gr__gr_?_CI0_ls_1_ls_H_ls_ast_c__c_VariantMacro_gr__gr_?W_gr_1_ls_v_gr__builtin__C_c_C_l:

.. das:function:: for_each_variant_macro(module: rtti::Module? const implicit; block: block<(var arg0:smart_ptr<ast::VariantMacro>):void> const implicit)

+--------+--------------------------------------------------------------------------------------------+
+argument+argument type                                                                               +
+========+============================================================================================+
+module  + :ref:`rtti::Module <handle-rtti-Module>` ? const implicit                                  +
+--------+--------------------------------------------------------------------------------------------+
+block   +block<(smart_ptr< :ref:`ast::VariantMacro <handle-ast-VariantMacro>` >):void> const implicit+
+--------+--------------------------------------------------------------------------------------------+


|function-ast-for_each_variant_macro|

.. _function-_at_ast_c__c_for_each_for_loop_macro_CI1_ls_H_ls_rtti_c__c_Module_gr__gr_?_CI0_ls_1_ls_H_ls_ast_c__c_ForLoopMacro_gr__gr_?W_gr_1_ls_v_gr__builtin__C_c_C_l:

.. das:function:: for_each_for_loop_macro(module: rtti::Module? const implicit; block: block<(var arg0:smart_ptr<ast::ForLoopMacro>):void> const implicit)

+--------+--------------------------------------------------------------------------------------------+
+argument+argument type                                                                               +
+========+============================================================================================+
+module  + :ref:`rtti::Module <handle-rtti-Module>` ? const implicit                                  +
+--------+--------------------------------------------------------------------------------------------+
+block   +block<(smart_ptr< :ref:`ast::ForLoopMacro <handle-ast-ForLoopMacro>` >):void> const implicit+
+--------+--------------------------------------------------------------------------------------------+


|function-ast-for_each_for_loop_macro|

.. _function-_at_ast_c__c_for_each_typeinfo_macro_CI1_ls_H_ls_rtti_c__c_Module_gr__gr_?_CI0_ls_1_ls_H_ls_ast_c__c_TypeInfoMacro_gr__gr_?W_gr_1_ls_v_gr__builtin__C_c_C_l:

.. das:function:: for_each_typeinfo_macro(module: rtti::Module? const implicit; block: block<(var arg0:smart_ptr<ast::TypeInfoMacro>):void> const implicit)

+--------+----------------------------------------------------------------------------------------------+
+argument+argument type                                                                                 +
+========+==============================================================================================+
+module  + :ref:`rtti::Module <handle-rtti-Module>` ? const implicit                                    +
+--------+----------------------------------------------------------------------------------------------+
+block   +block<(smart_ptr< :ref:`ast::TypeInfoMacro <handle-ast-TypeInfoMacro>` >):void> const implicit+
+--------+----------------------------------------------------------------------------------------------+


|function-ast-for_each_typeinfo_macro|

.. _function-_at_ast_c__c_for_each_field_CIH_ls_rtti_c__c_BasicStructureAnnotation_gr__CI0_ls_s;s;1_ls_H_ls_ast_c__c_TypeDecl_gr__gr_?W;u_gr_1_ls_v_gr__builtin__C_c_C_l:

.. das:function:: for_each_field(annotation: BasicStructureAnnotation const implicit; block: block<(var arg0:string;var arg1:string;var arg2:smart_ptr<ast::TypeDecl>;var arg3:uint):void> const implicit)

+----------+-------------------------------------------------------------------------------------------------------+
+argument  +argument type                                                                                          +
+==========+=======================================================================================================+
+annotation+ :ref:`rtti::BasicStructureAnnotation <handle-rtti-BasicStructureAnnotation>`  const implicit          +
+----------+-------------------------------------------------------------------------------------------------------+
+block     +block<(string;string;smart_ptr< :ref:`ast::TypeDecl <handle-ast-TypeDecl>` >;uint):void> const implicit+
+----------+-------------------------------------------------------------------------------------------------------+


|function-ast-for_each_field|

+++++++
Cloning
+++++++

  *  :ref:`clone_structure (structure:ast::Structure const? const implicit) : smart_ptr\<ast::Structure\> <function-_at_ast_c__c_clone_structure_CI1_ls_CH_ls_ast_c__c_Structure_gr__gr_?>` 
  *  :ref:`clone_expression (expression:smart_ptr\<ast::Expression\> const implicit) : smart_ptr\<ast::Expression\> <function-_at_ast_c__c_clone_expression_CI1_ls_H_ls_ast_c__c_Expression_gr__gr_?W>` 
  *  :ref:`clone_function (function:smart_ptr\<ast::Function\> const implicit) : smart_ptr\<ast::Function\> <function-_at_ast_c__c_clone_function_CI1_ls_H_ls_ast_c__c_Function_gr__gr_?W>` 
  *  :ref:`clone_variable (variable:smart_ptr\<ast::Variable\> const implicit) : smart_ptr\<ast::Variable\> <function-_at_ast_c__c_clone_variable_CI1_ls_H_ls_ast_c__c_Variable_gr__gr_?W>` 
  *  :ref:`clone_type (type:smart_ptr\<ast::TypeDecl\> const implicit) : smart_ptr\<ast::TypeDecl\> <function-_at_ast_c__c_clone_type_CI1_ls_H_ls_ast_c__c_TypeDecl_gr__gr_?W>` 

.. _function-_at_ast_c__c_clone_structure_CI1_ls_CH_ls_ast_c__c_Structure_gr__gr_?:

.. das:function:: clone_structure(structure: ast::Structure const? const implicit)

clone_structure returns smart_ptr< :ref:`ast::Structure <handle-ast-Structure>` >

+---------+--------------------------------------------------------------------+
+argument +argument type                                                       +
+=========+====================================================================+
+structure+ :ref:`ast::Structure <handle-ast-Structure>`  const? const implicit+
+---------+--------------------------------------------------------------------+


|function-ast-clone_structure|

.. _function-_at_ast_c__c_clone_expression_CI1_ls_H_ls_ast_c__c_Expression_gr__gr_?W:

.. das:function:: clone_expression(expression: smart_ptr<ast::Expression> const implicit)

clone_expression returns smart_ptr< :ref:`ast::Expression <handle-ast-Expression>` >

+----------+--------------------------------------------------------------------------+
+argument  +argument type                                                             +
+==========+==========================================================================+
+expression+smart_ptr< :ref:`ast::Expression <handle-ast-Expression>` > const implicit+
+----------+--------------------------------------------------------------------------+


|function-ast-clone_expression|

.. _function-_at_ast_c__c_clone_function_CI1_ls_H_ls_ast_c__c_Function_gr__gr_?W:

.. das:function:: clone_function(function: smart_ptr<ast::Function> const implicit)

clone_function returns smart_ptr< :ref:`ast::Function <handle-ast-Function>` >

+--------+----------------------------------------------------------------------+
+argument+argument type                                                         +
+========+======================================================================+
+function+smart_ptr< :ref:`ast::Function <handle-ast-Function>` > const implicit+
+--------+----------------------------------------------------------------------+


|function-ast-clone_function|

.. _function-_at_ast_c__c_clone_variable_CI1_ls_H_ls_ast_c__c_Variable_gr__gr_?W:

.. das:function:: clone_variable(variable: smart_ptr<ast::Variable> const implicit)

clone_variable returns smart_ptr< :ref:`ast::Variable <handle-ast-Variable>` >

+--------+----------------------------------------------------------------------+
+argument+argument type                                                         +
+========+======================================================================+
+variable+smart_ptr< :ref:`ast::Variable <handle-ast-Variable>` > const implicit+
+--------+----------------------------------------------------------------------+


|function-ast-clone_variable|

.. _function-_at_ast_c__c_clone_type_CI1_ls_H_ls_ast_c__c_TypeDecl_gr__gr_?W:

.. das:function:: clone_type(type: smart_ptr<ast::TypeDecl> const implicit)

clone_type returns smart_ptr< :ref:`ast::TypeDecl <handle-ast-TypeDecl>` >

+--------+----------------------------------------------------------------------+
+argument+argument type                                                         +
+========+======================================================================+
+type    +smart_ptr< :ref:`ast::TypeDecl <handle-ast-TypeDecl>` > const implicit+
+--------+----------------------------------------------------------------------+


|function-ast-clone_type|

++++++++++++
Mangled name
++++++++++++

  *  :ref:`parse_mangled_name (txt:string const implicit;lib:rtti::ModuleGroup implicit;thisModule:rtti::Module? const implicit;context:__context const;line:__lineInfo const) : smart_ptr\<ast::TypeDecl\> <function-_at_ast_c__c_parse_mangled_name_CIs_IH_ls_rtti_c__c_ModuleGroup_gr__CI1_ls_H_ls_rtti_c__c_Module_gr__gr_?_C_c_C_l>` 
  *  :ref:`get_mangled_name (function:smart_ptr\<ast::Function\> const implicit;context:__context const;line:__lineInfo const) : string <function-_at_ast_c__c_get_mangled_name_CI1_ls_H_ls_ast_c__c_Function_gr__gr_?M_C_c_C_l>` 
  *  :ref:`get_mangled_name (type:smart_ptr\<ast::TypeDecl\> const implicit;context:__context const;line:__lineInfo const) : string <function-_at_ast_c__c_get_mangled_name_CI1_ls_H_ls_ast_c__c_TypeDecl_gr__gr_?M_C_c_C_l>` 
  *  :ref:`get_mangled_name (variable:smart_ptr\<ast::Variable\> const implicit;context:__context const;line:__lineInfo const) : string <function-_at_ast_c__c_get_mangled_name_CI1_ls_H_ls_ast_c__c_Variable_gr__gr_?M_C_c_C_l>` 
  *  :ref:`get_mangled_name (variable:smart_ptr\<ast::ExprBlock\> const implicit;context:__context const;line:__lineInfo const) : string <function-_at_ast_c__c_get_mangled_name_CI1_ls_H_ls_ast_c__c_ExprBlock_gr__gr_?M_C_c_C_l>` 

.. _function-_at_ast_c__c_parse_mangled_name_CIs_IH_ls_rtti_c__c_ModuleGroup_gr__CI1_ls_H_ls_rtti_c__c_Module_gr__gr_?_C_c_C_l:

.. das:function:: parse_mangled_name(txt: string const implicit; lib: ModuleGroup implicit; thisModule: rtti::Module? const implicit)

parse_mangled_name returns smart_ptr< :ref:`ast::TypeDecl <handle-ast-TypeDecl>` >

+----------+-------------------------------------------------------------+
+argument  +argument type                                                +
+==========+=============================================================+
+txt       +string const implicit                                        +
+----------+-------------------------------------------------------------+
+lib       + :ref:`rtti::ModuleGroup <handle-rtti-ModuleGroup>`  implicit+
+----------+-------------------------------------------------------------+
+thisModule+ :ref:`rtti::Module <handle-rtti-Module>` ? const implicit   +
+----------+-------------------------------------------------------------+


|function-ast-parse_mangled_name|

.. _function-_at_ast_c__c_get_mangled_name_CI1_ls_H_ls_ast_c__c_Function_gr__gr_?M_C_c_C_l:

.. das:function:: get_mangled_name(function: smart_ptr<ast::Function> const implicit)

get_mangled_name returns string

+--------+----------------------------------------------------------------------+
+argument+argument type                                                         +
+========+======================================================================+
+function+smart_ptr< :ref:`ast::Function <handle-ast-Function>` > const implicit+
+--------+----------------------------------------------------------------------+


|function-ast-get_mangled_name|

.. _function-_at_ast_c__c_get_mangled_name_CI1_ls_H_ls_ast_c__c_TypeDecl_gr__gr_?M_C_c_C_l:

.. das:function:: get_mangled_name(type: smart_ptr<ast::TypeDecl> const implicit)

get_mangled_name returns string

+--------+----------------------------------------------------------------------+
+argument+argument type                                                         +
+========+======================================================================+
+type    +smart_ptr< :ref:`ast::TypeDecl <handle-ast-TypeDecl>` > const implicit+
+--------+----------------------------------------------------------------------+


|function-ast-get_mangled_name|

.. _function-_at_ast_c__c_get_mangled_name_CI1_ls_H_ls_ast_c__c_Variable_gr__gr_?M_C_c_C_l:

.. das:function:: get_mangled_name(variable: smart_ptr<ast::Variable> const implicit)

get_mangled_name returns string

+--------+----------------------------------------------------------------------+
+argument+argument type                                                         +
+========+======================================================================+
+variable+smart_ptr< :ref:`ast::Variable <handle-ast-Variable>` > const implicit+
+--------+----------------------------------------------------------------------+


|function-ast-get_mangled_name|

.. _function-_at_ast_c__c_get_mangled_name_CI1_ls_H_ls_ast_c__c_ExprBlock_gr__gr_?M_C_c_C_l:

.. das:function:: get_mangled_name(variable: smart_ptr<ast::ExprBlock> const implicit)

get_mangled_name returns string

+--------+------------------------------------------------------------------------+
+argument+argument type                                                           +
+========+========================================================================+
+variable+smart_ptr< :ref:`ast::ExprBlock <handle-ast-ExprBlock>` > const implicit+
+--------+------------------------------------------------------------------------+


|function-ast-get_mangled_name|

+++++++++++++++
Size and offset
+++++++++++++++

  *  :ref:`get_variant_field_offset (variant:smart_ptr\<ast::TypeDecl\> const implicit;index:int const;context:__context const;at:__lineInfo const) : int <function-_at_ast_c__c_get_variant_field_offset_CI1_ls_H_ls_ast_c__c_TypeDecl_gr__gr_?M_Ci_C_c_C_l>` 
  *  :ref:`get_tuple_field_offset (typle:smart_ptr\<ast::TypeDecl\> const implicit;index:int const;context:__context const;at:__lineInfo const) : int <function-_at_ast_c__c_get_tuple_field_offset_CI1_ls_H_ls_ast_c__c_TypeDecl_gr__gr_?M_Ci_C_c_C_l>` 
  *  :ref:`any_array_size (array:void? const implicit) : int <function-_at_ast_c__c_any_array_size_CI?>` 
  *  :ref:`any_table_size (table:void? const implicit) : int <function-_at_ast_c__c_any_table_size_CI?>` 
  *  :ref:`get_handled_type_field_offset (type:smart_ptr\<rtti::TypeAnnotation\> const implicit;field:string const implicit;context:__context const;line:__lineInfo const) : uint <function-_at_ast_c__c_get_handled_type_field_offset_CI1_ls_H_ls_rtti_c__c_TypeAnnotation_gr__gr_?M_CIs_C_c_C_l>` 

.. _function-_at_ast_c__c_get_variant_field_offset_CI1_ls_H_ls_ast_c__c_TypeDecl_gr__gr_?M_Ci_C_c_C_l:

.. das:function:: get_variant_field_offset(variant: smart_ptr<ast::TypeDecl> const implicit; index: int const)

get_variant_field_offset returns int

+--------+----------------------------------------------------------------------+
+argument+argument type                                                         +
+========+======================================================================+
+variant +smart_ptr< :ref:`ast::TypeDecl <handle-ast-TypeDecl>` > const implicit+
+--------+----------------------------------------------------------------------+
+index   +int const                                                             +
+--------+----------------------------------------------------------------------+


|function-ast-get_variant_field_offset|

.. _function-_at_ast_c__c_get_tuple_field_offset_CI1_ls_H_ls_ast_c__c_TypeDecl_gr__gr_?M_Ci_C_c_C_l:

.. das:function:: get_tuple_field_offset(typle: smart_ptr<ast::TypeDecl> const implicit; index: int const)

get_tuple_field_offset returns int

+--------+----------------------------------------------------------------------+
+argument+argument type                                                         +
+========+======================================================================+
+typle   +smart_ptr< :ref:`ast::TypeDecl <handle-ast-TypeDecl>` > const implicit+
+--------+----------------------------------------------------------------------+
+index   +int const                                                             +
+--------+----------------------------------------------------------------------+


|function-ast-get_tuple_field_offset|

.. _function-_at_ast_c__c_any_array_size_CI?:

.. das:function:: any_array_size(array: void? const implicit)

any_array_size returns int

+--------+--------------------+
+argument+argument type       +
+========+====================+
+array   +void? const implicit+
+--------+--------------------+


|function-ast-any_array_size|

.. _function-_at_ast_c__c_any_table_size_CI?:

.. das:function:: any_table_size(table: void? const implicit)

any_table_size returns int

+--------+--------------------+
+argument+argument type       +
+========+====================+
+table   +void? const implicit+
+--------+--------------------+


|function-ast-any_table_size|

.. _function-_at_ast_c__c_get_handled_type_field_offset_CI1_ls_H_ls_rtti_c__c_TypeAnnotation_gr__gr_?M_CIs_C_c_C_l:

.. das:function:: get_handled_type_field_offset(type: smart_ptr<rtti::TypeAnnotation> const implicit; field: string const implicit)

get_handled_type_field_offset returns uint

+--------+------------------------------------------------------------------------------------+
+argument+argument type                                                                       +
+========+====================================================================================+
+type    +smart_ptr< :ref:`rtti::TypeAnnotation <handle-rtti-TypeAnnotation>` > const implicit+
+--------+------------------------------------------------------------------------------------+
+field   +string const implicit                                                               +
+--------+------------------------------------------------------------------------------------+


|function-ast-get_handled_type_field_offset|

++++++++++++++++++
Pointer conversion
++++++++++++++++++

  *  :ref:`ExpressionPtr (expr:smart_ptr\<auto(TT)\> const) : smart_ptr\<ast::Expression\> <function-_at_ast_c__c_ExpressionPtr_C1_ls_Y_ls_TT_gr_._gr_?M>` 
  *  :ref:`FunctionPtr (fun:ast::Function? const) : smart_ptr\<ast::Function\> <function-_at_ast_c__c_FunctionPtr_C1_ls_H_ls_ast_c__c_Function_gr__gr_?>` 
  *  :ref:`StructurePtr (stru:ast::Structure? const) : smart_ptr\<ast::Structure\> <function-_at_ast_c__c_StructurePtr_C1_ls_H_ls_ast_c__c_Structure_gr__gr_?>` 

.. _function-_at_ast_c__c_ExpressionPtr_C1_ls_Y_ls_TT_gr_._gr_?M:

.. das:function:: ExpressionPtr(expr: smart_ptr<auto(TT)> const)

ExpressionPtr returns  :ref:`ExpressionPtr <alias-ExpressionPtr>` 

+--------+-------------------------+
+argument+argument type            +
+========+=========================+
+expr    +smart_ptr<auto(TT)> const+
+--------+-------------------------+


|function-ast-ExpressionPtr|

.. _function-_at_ast_c__c_FunctionPtr_C1_ls_H_ls_ast_c__c_Function_gr__gr_?:

.. das:function:: FunctionPtr(fun: ast::Function? const)

FunctionPtr returns  :ref:`FunctionPtr <alias-FunctionPtr>` 

+--------+---------------------------------------------------+
+argument+argument type                                      +
+========+===================================================+
+fun     + :ref:`ast::Function <handle-ast-Function>` ? const+
+--------+---------------------------------------------------+


|function-ast-FunctionPtr|

.. _function-_at_ast_c__c_StructurePtr_C1_ls_H_ls_ast_c__c_Structure_gr__gr_?:

.. das:function:: StructurePtr(stru: ast::Structure? const)

StructurePtr returns  :ref:`StructurePtr <alias-StructurePtr>` 

+--------+-----------------------------------------------------+
+argument+argument type                                        +
+========+=====================================================+
+stru    + :ref:`ast::Structure <handle-ast-Structure>` ? const+
+--------+-----------------------------------------------------+


|function-ast-StructurePtr|

+++++++++++
Evaluations
+++++++++++

  *  :ref:`eval_single_expression (expr:smart_ptr\<ast::Expression\> const& implicit;ok:bool& implicit) : float4 <function-_at_ast_c__c_eval_single_expression_C&I1_ls_H_ls_ast_c__c_Expression_gr__gr_?W_&Ib>` 

.. _function-_at_ast_c__c_eval_single_expression_C&I1_ls_H_ls_ast_c__c_Expression_gr__gr_?W_&Ib:

.. das:function:: eval_single_expression(expr: smart_ptr<ast::Expression> const& implicit; ok: bool& implicit)

eval_single_expression returns float4

.. warning:: 
  This is unsafe operation.

+--------+---------------------------------------------------------------------------+
+argument+argument type                                                              +
+========+===========================================================================+
+expr    +smart_ptr< :ref:`ast::Expression <handle-ast-Expression>` > const& implicit+
+--------+---------------------------------------------------------------------------+
+ok      +bool& implicit                                                             +
+--------+---------------------------------------------------------------------------+


|function-ast-eval_single_expression|

+++++++++++++++
Error reporting
+++++++++++++++

  *  :ref:`macro_error (porogram:smart_ptr\<rtti::Program\> const implicit;at:rtti::LineInfo const implicit;message:string const implicit;context:__context const;line:__lineInfo const) : void <function-_at_ast_c__c_macro_error_CI1_ls_H_ls_rtti_c__c_Program_gr__gr_?W_CIH_ls_rtti_c__c_LineInfo_gr__CIs_C_c_C_l>` 

.. _function-_at_ast_c__c_macro_error_CI1_ls_H_ls_rtti_c__c_Program_gr__gr_?W_CIH_ls_rtti_c__c_LineInfo_gr__CIs_C_c_C_l:

.. das:function:: macro_error(porogram: smart_ptr<rtti::Program> const implicit; at: LineInfo const implicit; message: string const implicit)

+--------+----------------------------------------------------------------------+
+argument+argument type                                                         +
+========+======================================================================+
+porogram+smart_ptr< :ref:`rtti::Program <handle-rtti-Program>` > const implicit+
+--------+----------------------------------------------------------------------+
+at      + :ref:`rtti::LineInfo <handle-rtti-LineInfo>`  const implicit         +
+--------+----------------------------------------------------------------------+
+message +string const implicit                                                 +
+--------+----------------------------------------------------------------------+


|function-ast-macro_error|

++++++++++++++++++++
Location and context
++++++++++++++++++++

  *  :ref:`force_at (expression:smart_ptr\<ast::Expression\> const& implicit;at:rtti::LineInfo const implicit) : void <function-_at_ast_c__c_force_at_C&I1_ls_H_ls_ast_c__c_Expression_gr__gr_?M_CIH_ls_rtti_c__c_LineInfo_gr_>` 
  *  :ref:`collect_dependencies (function:smart_ptr\<ast::Function\> const implicit;block:block\<(var arg0:array\<ast::Function?\>;var arg1:array\<ast::Variable?\>):void\> const implicit;context:__context const;line:__lineInfo const) : void <function-_at_ast_c__c_collect_dependencies_CI1_ls_H_ls_ast_c__c_Function_gr__gr_?W_CI0_ls_1_ls_1_ls_H_ls_ast_c__c_Function_gr__gr_?_gr_A;1_ls_1_ls_H_ls_ast_c__c_Variable_gr__gr_?_gr_A_gr_1_ls_v_gr__builtin__C_c_C_l>` 
  *  :ref:`get_ast_context (program:smart_ptr\<rtti::Program\> const implicit;expression:smart_ptr\<ast::Expression\> const implicit;block:block\<(var arg0:bool;var arg1:ast::AstContext):void\> const implicit;context:__context const;line:__lineInfo const) : void <function-_at_ast_c__c_get_ast_context_CI1_ls_H_ls_rtti_c__c_Program_gr__gr_?M_CI1_ls_H_ls_ast_c__c_Expression_gr__gr_?M_CI0_ls_b;H_ls_ast_c__c_AstContext_gr__gr_1_ls_v_gr__builtin__C_c_C_l>` 

.. _function-_at_ast_c__c_force_at_C&I1_ls_H_ls_ast_c__c_Expression_gr__gr_?M_CIH_ls_rtti_c__c_LineInfo_gr_:

.. das:function:: force_at(expression: smart_ptr<ast::Expression> const& implicit; at: LineInfo const implicit)

+----------+---------------------------------------------------------------------------+
+argument  +argument type                                                              +
+==========+===========================================================================+
+expression+smart_ptr< :ref:`ast::Expression <handle-ast-Expression>` > const& implicit+
+----------+---------------------------------------------------------------------------+
+at        + :ref:`rtti::LineInfo <handle-rtti-LineInfo>`  const implicit              +
+----------+---------------------------------------------------------------------------+


|function-ast-force_at|

.. _function-_at_ast_c__c_collect_dependencies_CI1_ls_H_ls_ast_c__c_Function_gr__gr_?W_CI0_ls_1_ls_1_ls_H_ls_ast_c__c_Function_gr__gr_?_gr_A;1_ls_1_ls_H_ls_ast_c__c_Variable_gr__gr_?_gr_A_gr_1_ls_v_gr__builtin__C_c_C_l:

.. das:function:: collect_dependencies(function: smart_ptr<ast::Function> const implicit; block: block<(var arg0:array<ast::Function?>;var arg1:array<ast::Variable?>):void> const implicit)

+--------+--------------------------------------------------------------------------------------------------------------------------------------+
+argument+argument type                                                                                                                         +
+========+======================================================================================================================================+
+function+smart_ptr< :ref:`ast::Function <handle-ast-Function>` > const implicit                                                                +
+--------+--------------------------------------------------------------------------------------------------------------------------------------+
+block   +block<(array< :ref:`ast::Function <handle-ast-Function>` ?>;array< :ref:`ast::Variable <handle-ast-Variable>` ?>):void> const implicit+
+--------+--------------------------------------------------------------------------------------------------------------------------------------+


|function-ast-collect_dependencies|

.. _function-_at_ast_c__c_get_ast_context_CI1_ls_H_ls_rtti_c__c_Program_gr__gr_?M_CI1_ls_H_ls_ast_c__c_Expression_gr__gr_?M_CI0_ls_b;H_ls_ast_c__c_AstContext_gr__gr_1_ls_v_gr__builtin__C_c_C_l:

.. das:function:: get_ast_context(program: smart_ptr<rtti::Program> const implicit; expression: smart_ptr<ast::Expression> const implicit; block: block<(var arg0:bool;var arg1:ast::AstContext):void> const implicit)

+----------+----------------------------------------------------------------------------------+
+argument  +argument type                                                                     +
+==========+==================================================================================+
+program   +smart_ptr< :ref:`rtti::Program <handle-rtti-Program>` > const implicit            +
+----------+----------------------------------------------------------------------------------+
+expression+smart_ptr< :ref:`ast::Expression <handle-ast-Expression>` > const implicit        +
+----------+----------------------------------------------------------------------------------+
+block     +block<(bool; :ref:`ast::AstContext <handle-ast-AstContext>` ):void> const implicit+
+----------+----------------------------------------------------------------------------------+


|function-ast-get_ast_context|

+++++++++++
Use queries
+++++++++++

  *  :ref:`get_use_global_variables (func:smart_ptr\<ast::Function\> const implicit;block:block\<(var arg0:smart_ptr\<ast::Variable\>):void\> const implicit;context:__context const;at:__lineInfo const) : void <function-_at_ast_c__c_get_use_global_variables_CI1_ls_H_ls_ast_c__c_Function_gr__gr_?M_CI0_ls_1_ls_H_ls_ast_c__c_Variable_gr__gr_?W_gr_1_ls_v_gr__builtin__C_c_C_l>` 
  *  :ref:`get_use_functions (func:smart_ptr\<ast::Function\> const implicit;block:block\<(var arg0:smart_ptr\<ast::Function\>):void\> const implicit;context:__context const;at:__lineInfo const) : void <function-_at_ast_c__c_get_use_functions_CI1_ls_H_ls_ast_c__c_Function_gr__gr_?M_CI0_ls_1_ls_H_ls_ast_c__c_Function_gr__gr_?W_gr_1_ls_v_gr__builtin__C_c_C_l>` 

.. _function-_at_ast_c__c_get_use_global_variables_CI1_ls_H_ls_ast_c__c_Function_gr__gr_?M_CI0_ls_1_ls_H_ls_ast_c__c_Variable_gr__gr_?W_gr_1_ls_v_gr__builtin__C_c_C_l:

.. das:function:: get_use_global_variables(func: smart_ptr<ast::Function> const implicit; block: block<(var arg0:smart_ptr<ast::Variable>):void> const implicit)

+--------+------------------------------------------------------------------------------------+
+argument+argument type                                                                       +
+========+====================================================================================+
+func    +smart_ptr< :ref:`ast::Function <handle-ast-Function>` > const implicit              +
+--------+------------------------------------------------------------------------------------+
+block   +block<(smart_ptr< :ref:`ast::Variable <handle-ast-Variable>` >):void> const implicit+
+--------+------------------------------------------------------------------------------------+


|function-ast-get_use_global_variables|

.. _function-_at_ast_c__c_get_use_functions_CI1_ls_H_ls_ast_c__c_Function_gr__gr_?M_CI0_ls_1_ls_H_ls_ast_c__c_Function_gr__gr_?W_gr_1_ls_v_gr__builtin__C_c_C_l:

.. das:function:: get_use_functions(func: smart_ptr<ast::Function> const implicit; block: block<(var arg0:smart_ptr<ast::Function>):void> const implicit)

+--------+------------------------------------------------------------------------------------+
+argument+argument type                                                                       +
+========+====================================================================================+
+func    +smart_ptr< :ref:`ast::Function <handle-ast-Function>` > const implicit              +
+--------+------------------------------------------------------------------------------------+
+block   +block<(smart_ptr< :ref:`ast::Function <handle-ast-Function>` >):void> const implicit+
+--------+------------------------------------------------------------------------------------+


|function-ast-get_use_functions|

+++
Log
+++

  *  :ref:`to_compilation_log (text:string const implicit;context:__context const;at:__lineInfo const) : void <function-_at_ast_c__c_to_compilation_log_CIs_C_c_C_l>` 

.. _function-_at_ast_c__c_to_compilation_log_CIs_C_c_C_l:

.. das:function:: to_compilation_log(text: string const implicit)

+--------+---------------------+
+argument+argument type        +
+========+=====================+
+text    +string const implicit+
+--------+---------------------+


|function-ast-to_compilation_log|

+++++++
Removal
+++++++

  *  :ref:`remove_structure (module:rtti::Module? const implicit;structure:smart_ptr\<ast::Structure\>& implicit) : bool <function-_at_ast_c__c_remove_structure_CI1_ls_H_ls_rtti_c__c_Module_gr__gr_?_&I1_ls_H_ls_ast_c__c_Structure_gr__gr_?W>` 

.. _function-_at_ast_c__c_remove_structure_CI1_ls_H_ls_rtti_c__c_Module_gr__gr_?_&I1_ls_H_ls_ast_c__c_Structure_gr__gr_?W:

.. das:function:: remove_structure(module: rtti::Module? const implicit; structure: smart_ptr<ast::Structure>& implicit)

remove_structure returns bool

+---------+-------------------------------------------------------------------+
+argument +argument type                                                      +
+=========+===================================================================+
+module   + :ref:`rtti::Module <handle-rtti-Module>` ? const implicit         +
+---------+-------------------------------------------------------------------+
+structure+smart_ptr< :ref:`ast::Structure <handle-ast-Structure>` >& implicit+
+---------+-------------------------------------------------------------------+


|function-ast-remove_structure|

++++++++++
Properties
++++++++++

  *  :ref:`is_temp_type (type:smart_ptr\<ast::TypeDecl\> const implicit;refMatters:bool const) : bool <function-_at_ast_c__c_is_temp_type_CI1_ls_H_ls_ast_c__c_TypeDecl_gr__gr_?W_Cb>` 
  *  :ref:`is_same_type (leftType:smart_ptr\<ast::TypeDecl\> const implicit;rightType:smart_ptr\<ast::TypeDecl\> const implicit;refMatters:rtti::RefMatters const;constMatters:rtti::ConstMatters const;tempMatters:rtti::TemporaryMatters const;context:__context const;at:__lineInfo const) : bool <function-_at_ast_c__c_is_same_type_CI1_ls_H_ls_ast_c__c_TypeDecl_gr__gr_?W_CI1_ls_H_ls_ast_c__c_TypeDecl_gr__gr_?W_CE_ls_rtti_c__c_RefMatters_gr__CE_ls_rtti_c__c_ConstMatters_gr__CE_ls_rtti_c__c_TemporaryMatters_gr__C_c_C_l>` 
  *  :ref:`get_underlying_value_type (type:smart_ptr\<ast::TypeDecl\> const implicit;context:__context const;line:__lineInfo const) : smart_ptr\<ast::TypeDecl\> <function-_at_ast_c__c_get_underlying_value_type_CI1_ls_H_ls_ast_c__c_TypeDecl_gr__gr_?M_C_c_C_l>` 
  *  :ref:`get_handled_type_field_type (type:smart_ptr\<rtti::TypeAnnotation\> const implicit;field:string const implicit;context:__context const;line:__lineInfo const) : rtti::TypeInfo? <function-_at_ast_c__c_get_handled_type_field_type_CI1_ls_H_ls_rtti_c__c_TypeAnnotation_gr__gr_?M_CIs_C_c_C_l>` 
  *  :ref:`has_field (type:smart_ptr\<ast::TypeDecl\> const implicit;fieldName:string const implicit;constant:bool const) : bool <function-_at_ast_c__c_has_field_CI1_ls_H_ls_ast_c__c_TypeDecl_gr__gr_?W_CIs_Cb>` 
  *  :ref:`get_field_type (type:smart_ptr\<ast::TypeDecl\> const implicit;fieldName:string const implicit;constant:bool const) : smart_ptr\<ast::TypeDecl\> <function-_at_ast_c__c_get_field_type_CI1_ls_H_ls_ast_c__c_TypeDecl_gr__gr_?W_CIs_Cb>` 
  *  :ref:`is_visible_directly (from_module:rtti::Module? const implicit;which_module:rtti::Module? const implicit) : bool <function-_at_ast_c__c_is_visible_directly_CI1_ls_H_ls_rtti_c__c_Module_gr__gr_?_CI1_ls_H_ls_rtti_c__c_Module_gr__gr_?>` 
  *  :ref:`is_expr_like_call (expression:smart_ptr\<ast::Expression\> const& implicit) : bool <function-_at_ast_c__c_is_expr_like_call_C&I1_ls_H_ls_ast_c__c_Expression_gr__gr_?W>` 
  *  :ref:`is_expr_const (expression:smart_ptr\<ast::Expression\> const& implicit) : bool <function-_at_ast_c__c_is_expr_const_C&I1_ls_H_ls_ast_c__c_Expression_gr__gr_?W>` 

.. _function-_at_ast_c__c_is_temp_type_CI1_ls_H_ls_ast_c__c_TypeDecl_gr__gr_?W_Cb:

.. das:function:: is_temp_type(type: smart_ptr<ast::TypeDecl> const implicit; refMatters: bool const)

is_temp_type returns bool

+----------+----------------------------------------------------------------------+
+argument  +argument type                                                         +
+==========+======================================================================+
+type      +smart_ptr< :ref:`ast::TypeDecl <handle-ast-TypeDecl>` > const implicit+
+----------+----------------------------------------------------------------------+
+refMatters+bool const                                                            +
+----------+----------------------------------------------------------------------+


|function-ast-is_temp_type|

.. _function-_at_ast_c__c_is_same_type_CI1_ls_H_ls_ast_c__c_TypeDecl_gr__gr_?W_CI1_ls_H_ls_ast_c__c_TypeDecl_gr__gr_?W_CE_ls_rtti_c__c_RefMatters_gr__CE_ls_rtti_c__c_ConstMatters_gr__CE_ls_rtti_c__c_TemporaryMatters_gr__C_c_C_l:

.. das:function:: is_same_type(leftType: smart_ptr<ast::TypeDecl> const implicit; rightType: smart_ptr<ast::TypeDecl> const implicit; refMatters: RefMatters const; constMatters: ConstMatters const; tempMatters: TemporaryMatters const)

is_same_type returns bool

+------------+----------------------------------------------------------------------+
+argument    +argument type                                                         +
+============+======================================================================+
+leftType    +smart_ptr< :ref:`ast::TypeDecl <handle-ast-TypeDecl>` > const implicit+
+------------+----------------------------------------------------------------------+
+rightType   +smart_ptr< :ref:`ast::TypeDecl <handle-ast-TypeDecl>` > const implicit+
+------------+----------------------------------------------------------------------+
+refMatters  + :ref:`rtti::RefMatters <enum-rtti-RefMatters>`  const                +
+------------+----------------------------------------------------------------------+
+constMatters+ :ref:`rtti::ConstMatters <enum-rtti-ConstMatters>`  const            +
+------------+----------------------------------------------------------------------+
+tempMatters + :ref:`rtti::TemporaryMatters <enum-rtti-TemporaryMatters>`  const    +
+------------+----------------------------------------------------------------------+


|function-ast-is_same_type|

.. _function-_at_ast_c__c_get_underlying_value_type_CI1_ls_H_ls_ast_c__c_TypeDecl_gr__gr_?M_C_c_C_l:

.. das:function:: get_underlying_value_type(type: smart_ptr<ast::TypeDecl> const implicit)

get_underlying_value_type returns smart_ptr< :ref:`ast::TypeDecl <handle-ast-TypeDecl>` >

+--------+----------------------------------------------------------------------+
+argument+argument type                                                         +
+========+======================================================================+
+type    +smart_ptr< :ref:`ast::TypeDecl <handle-ast-TypeDecl>` > const implicit+
+--------+----------------------------------------------------------------------+


|function-ast-get_underlying_value_type|

.. _function-_at_ast_c__c_get_handled_type_field_type_CI1_ls_H_ls_rtti_c__c_TypeAnnotation_gr__gr_?M_CIs_C_c_C_l:

.. das:function:: get_handled_type_field_type(type: smart_ptr<rtti::TypeAnnotation> const implicit; field: string const implicit)

get_handled_type_field_type returns  :ref:`rtti::TypeInfo <handle-rtti-TypeInfo>` ?

+--------+------------------------------------------------------------------------------------+
+argument+argument type                                                                       +
+========+====================================================================================+
+type    +smart_ptr< :ref:`rtti::TypeAnnotation <handle-rtti-TypeAnnotation>` > const implicit+
+--------+------------------------------------------------------------------------------------+
+field   +string const implicit                                                               +
+--------+------------------------------------------------------------------------------------+


|function-ast-get_handled_type_field_type|

.. _function-_at_ast_c__c_has_field_CI1_ls_H_ls_ast_c__c_TypeDecl_gr__gr_?W_CIs_Cb:

.. das:function:: has_field(type: smart_ptr<ast::TypeDecl> const implicit; fieldName: string const implicit; constant: bool const)

has_field returns bool

+---------+----------------------------------------------------------------------+
+argument +argument type                                                         +
+=========+======================================================================+
+type     +smart_ptr< :ref:`ast::TypeDecl <handle-ast-TypeDecl>` > const implicit+
+---------+----------------------------------------------------------------------+
+fieldName+string const implicit                                                 +
+---------+----------------------------------------------------------------------+
+constant +bool const                                                            +
+---------+----------------------------------------------------------------------+


|function-ast-has_field|

.. _function-_at_ast_c__c_get_field_type_CI1_ls_H_ls_ast_c__c_TypeDecl_gr__gr_?W_CIs_Cb:

.. das:function:: get_field_type(type: smart_ptr<ast::TypeDecl> const implicit; fieldName: string const implicit; constant: bool const)

get_field_type returns smart_ptr< :ref:`ast::TypeDecl <handle-ast-TypeDecl>` >

+---------+----------------------------------------------------------------------+
+argument +argument type                                                         +
+=========+======================================================================+
+type     +smart_ptr< :ref:`ast::TypeDecl <handle-ast-TypeDecl>` > const implicit+
+---------+----------------------------------------------------------------------+
+fieldName+string const implicit                                                 +
+---------+----------------------------------------------------------------------+
+constant +bool const                                                            +
+---------+----------------------------------------------------------------------+


|function-ast-get_field_type|

.. _function-_at_ast_c__c_is_visible_directly_CI1_ls_H_ls_rtti_c__c_Module_gr__gr_?_CI1_ls_H_ls_rtti_c__c_Module_gr__gr_?:

.. das:function:: is_visible_directly(from_module: rtti::Module? const implicit; which_module: rtti::Module? const implicit)

is_visible_directly returns bool

+------------+----------------------------------------------------------+
+argument    +argument type                                             +
+============+==========================================================+
+from_module + :ref:`rtti::Module <handle-rtti-Module>` ? const implicit+
+------------+----------------------------------------------------------+
+which_module+ :ref:`rtti::Module <handle-rtti-Module>` ? const implicit+
+------------+----------------------------------------------------------+


|function-ast-is_visible_directly|

.. _function-_at_ast_c__c_is_expr_like_call_C&I1_ls_H_ls_ast_c__c_Expression_gr__gr_?W:

.. das:function:: is_expr_like_call(expression: smart_ptr<ast::Expression> const& implicit)

is_expr_like_call returns bool

+----------+---------------------------------------------------------------------------+
+argument  +argument type                                                              +
+==========+===========================================================================+
+expression+smart_ptr< :ref:`ast::Expression <handle-ast-Expression>` > const& implicit+
+----------+---------------------------------------------------------------------------+


|function-ast-is_expr_like_call|

.. _function-_at_ast_c__c_is_expr_const_C&I1_ls_H_ls_ast_c__c_Expression_gr__gr_?W:

.. das:function:: is_expr_const(expression: smart_ptr<ast::Expression> const& implicit)

is_expr_const returns bool

+----------+---------------------------------------------------------------------------+
+argument  +argument type                                                              +
+==========+===========================================================================+
+expression+smart_ptr< :ref:`ast::Expression <handle-ast-Expression>` > const& implicit+
+----------+---------------------------------------------------------------------------+


|function-ast-is_expr_const|


