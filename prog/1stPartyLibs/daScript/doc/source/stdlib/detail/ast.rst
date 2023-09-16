.. |typedef-ast-TypeDeclFlags| replace:: properties of the `TypeDecl` object.

.. |typedef-ast-FieldDeclarationFlags| replace:: properties of the `FieldDeclaration` object.

.. |typedef-ast-StructureFlags| replace:: properties of the `Structure` object.

.. |typedef-ast-ExprGenFlags| replace:: generation (genFlags) properties of the `Expression` object.

.. |typedef-ast-ExprFlags| replace:: properties of the `Expression` object.

.. |typedef-ast-ExprPrintFlags| replace:: printing properties of the `Expression` object.

.. |typedef-ast-FunctionFlags| replace:: properties of the `Function` object.

.. |typedef-ast-MoreFunctionFlags| replace:: additional properties of the `Function` object.

.. |typedef-ast-FunctionSideEffectFlags| replace:: side-effect properties of the `Function` object.

.. |typedef-ast-VariableFlags| replace:: properties of the `Variable` object.

.. |typedef-ast-VariableAccessFlags| replace:: access properties of the `Variable` object.

.. |typedef-ast-ExprBlockFlags| replace:: properties of the `ExrpBlock` object.

.. |typedef-ast-ExprAtFlags| replace:: properties of the `ExprAt` object.

.. |typedef-ast-ExprLetFlags| replace:: properties of the `ExprLet` object.

.. |typedef-ast-IfFlags| replace:: properties of the `ExprIf` object.

.. |typedef-ast-ExprMakeLocalFlags| replace:: properties of the `ExprMakeLocal` object (`ExprMakeArray`, `ExprMakeStruct`, 'ExprMakeTuple', 'ExprMakeVariant').

.. |typedef-ast-ExprAscendFlags| replace:: properties of the `ExprAscend` object.

.. |typedef-ast-ExprCastFlags| replace:: properties of the `ExprCast` object.

.. |typedef-ast-ExprVarFlags| replace:: properties of the `ExprVar` object.

.. |typedef-ast-ExprMakeStructFlags| replace:: properties of the `ExprMakeStruct` object.

.. |typedef-ast-MakeFieldDeclFlags| replace:: properties of the `MakeFieldDecl` object.

.. |typedef-ast-ExprFieldDerefFlags| replace:: dereferencing properties of the `ExprField` object.

.. |typedef-ast-ExprFieldFieldFlags| replace:: field properties of the `ExprField` object.

.. |typedef-ast-ExprSwizzleFieldFlags| replace:: properties of the `ExprSwizzle` object.

.. |typedef-ast-ExprYieldFlags| replace:: properties of the `ExprYield` object.

.. |typedef-ast-ExprReturnFlags| replace:: properties of the `ExprReturn` object.

.. |typedef-ast-ExprMakeBlockFlags| replace:: properties of the `ExprMakeBlock` object.

.. |typedef-ast-CopyFlags| replace:: properties of the `ExprCopy` object.

.. |typedef-ast-MoveFlags| replace:: properties of the `ExprMove` object.

.. |typedef-ast-ExpressionPtr| replace:: Smart pointer to `Expression` object.

.. |typedef-ast-StructurePtr| replace:: Smart pointer to `Structure` object.

.. |typedef-ast-ProgramPtr| replace:: Smart pointer to `Program` object.

.. |typedef-ast-TypeDeclPtr| replace:: Smart pointer to `TypeDecl` object.

.. |typedef-ast-VectorTypeDeclPtr| replace:: Smart pointer to das::vector<ExpressionPtr>.

.. |typedef-ast-EnumerationPtr| replace:: Smart pointer to `Enumeration` object.

.. |typedef-ast-FunctionPtr| replace:: Smart pointer to `Function` object.

.. |typedef-ast-VariablePtr| replace:: Smart pointer to `Variable` object.

.. |typedef-ast-MakeFieldDeclPtr| replace:: Smart pointer to `MakeFieldDecl` object.

.. |typedef-ast-FunctionAnnotationPtr| replace:: Smart pointer to `FunctionAnnotation` object.

.. |typedef-ast-StructureAnnotationPtr| replace:: Smart pointer to `StructureAnnotation` object.

.. |typedef-ast-EnumerationAnnotationPtr| replace:: Smart pointer to `EnumerationAnnotation` object.

.. |typedef-ast-PassMacroPtr| replace:: Smart pointer to `PassMacro` object.

.. |typedef-ast-VariantMacroPtr| replace:: Smart pointer to `VariantMacro` object.

.. |typedef-ast-ReaderMacroPtr| replace:: Smart pointer to `ReaderMacro` object.

.. |typedef-ast-CommentReaderPtr| replace:: Smart pointer to `CommentReader` object.

.. |typedef-ast-CallMacroPtr| replace:: Smart pointer to `CallMacro` object.

.. |typedef-ast-TypeInfoMacroPtr| replace:: Smart pointer to `TypeInfoMacro` object.

.. |typedef-ast-SimulateMacroPtr| replace:: Smart pointer to `SimulateMacro` object.

.. |enumeration-ast-SideEffects| replace:: Enumeration with all possible side effects of expression or function.

.. |enumeration-ast-CaptureMode| replace:: Enumeration with lambda variables capture modes.

.. |class-ast-AstFunctionAnnotation| replace:: Annotation macro which is attached to the `Function`.

.. |method-ast-AstFunctionAnnotation.transform| replace:: This callback occurs during the `infer` pass of the compilation. If no transformation is needed, the callback should return `null`. `errors` is filled with the transformation errors should they occur. Returned value replaces function call in the ast.

.. |method-ast-AstFunctionAnnotation.verifyCall| replace:: This callback occurs during the `lint` pass of the compilation. If call has lint errors it should return `false` and `errors` is filled with the lint errors.

.. |method-ast-AstFunctionAnnotation.apply| replace:: This callback occurs during the `parse` pass of the compilation on the function itself. If function has application errors it should return `false` and `errors` field.

.. |method-ast-AstFunctionAnnotation.generic_apply| replace:: This call occurs during the `infer` pass of the compilation, when generic function is instanced on the instance of the function. If function has application errors it should return `false` and `errors` field.

.. |method-ast-AstFunctionAnnotation.finish| replace:: This callback occurs during the `finalize allocations` pass of the compilation, after the stack is allocated, on the function itself. If function has finalization errors it should return `false` and `errors` field.

.. |method-ast-AstFunctionAnnotation.patch| replace:: This callback occurs right after the `infer` pass of the compilation on the function itself. If function has patching errors it should return `false` and `errors` field.
    If the `astChanged` flag is set, `infer` pass will be repeated. This allows to fix up the function after the `infer` pass with all the type information fully available.

.. |method-ast-AstFunctionAnnotation.fixup| replace:: This callback occurs during the `finalize allocations` pass of the compilation, before the stack is allocated, on the function itself. If function has fixup errors it should return `false` and `errors` field.

.. |method-ast-AstFunctionAnnotation.lint| replace:: This callback occurs during the `lint` pass of the compilation on the function itself. If function has lint errors it should return `false` and `errors` field.

.. |method-ast-AstFunctionAnnotation.complete| replace:: This callback occurs as the final stage of `Context` simulation.

.. |method-ast-AstFunctionAnnotation.isCompatible| replace:: This callback occurs during function type matching for both generic and regular functions. If function can accept given argument types it should return `true`, otherwise `errors` is filled with the matching problems.

.. |method-ast-AstFunctionAnnotation.isSpecialized| replace:: This callback occurs during function type matching. If function requires special type matching (i.e. `isCompatible`` is implemented) it should return `true`.

.. |method-ast-AstFunctionAnnotation.appendToMangledName| replace:: This call occurs when the function mangled name is requested. This is the way for the macro to ensure function is unique, even though type signature may be identical.

.. |class-ast-AstBlockAnnotation| replace:: Annotation macro which is attached to the `ExprBlock`.

.. |method-ast-AstBlockAnnotation.apply| replace:: This callback occurs during the `parse` pass of the compilation. If block has application errors it should return `false` and `errors` field.

.. |method-ast-AstBlockAnnotation.finish| replace:: This callback occurs during the `finalize allocations` pass of the compilation, after the stack is allocated. If block has finalization errors it should return `false` and `errors` field.

.. |class-ast-AstStructureAnnotation| replace:: Annotation macro which is attached to the `Structure`.

.. |method-ast-AstStructureAnnotation.apply| replace:: This callback occurs during the `parse` pass of the compilation. If structure has application errors it should return `false` and `errors` field.

.. |method-ast-AstStructureAnnotation.finish| replace:: This callback occurs during the `finalize allocations` pass of the compilation, after the stack is allocated. If structure has finalization errors it should return `false` and `errors` field.

.. |method-ast-AstStructureAnnotation.patch| replace:: This callback occurs right after the `infer` pass of the compilation on the structure itself. If structure has patching errors it should return `false` and `errors` field.
    If the `astChanged` flag is set, `infer` pass will be repeated. This allows to fix up the function after the `infer` pass with all the type information fully available.

.. |method-ast-AstStructureAnnotation.complete| replace:: This callback occurs as the final stage of `Context` simulation.

.. |method-ast-AstStructureAnnotation.aotPrefix| replace:: This callback occurs during the `AOT`.  It is used to generate CPP code before the structure declaration.

.. |method-ast-AstStructureAnnotation.aotBody| replace:: This callback occurs during the `AOT`.  It is used to generate CPP code in the body of the structure.

.. |method-ast-AstStructureAnnotation.aotSuffix| replace:: This callback occurs during the `AOT`.  It is used to generate CPP code after the structure declaration.

.. |class-ast-AstPassMacro| replace:: This macro is used to implement custom `infer` passes.

.. |method-ast-AstPassMacro.apply| replace:: This callback is called after `infer` pass. If macro did any work it returns `true`; `infer` pass is restarted a the memoent when first macro which did any work.

.. |class-ast-AstVariantMacro| replace:: This macro is used to implement custom `is`, `as` and `?as` expressions.

.. |method-ast-AstVariantMacro.visitExprIsVariant| replace:: This callback occurs during the `infer` pass for every `ExprIsVariant` (a `is` b). If no work is necessary it should return `null`, otherwise expression will be replaced by the result.

.. |method-ast-AstVariantMacro.visitExprAsVariant| replace:: This callback occurs during the `infer` pass for every `ExprAsVariant` (a `as` b). If no work is necessary it should return `null`, otherwise expression will be replaced by the result.

.. |method-ast-AstVariantMacro.visitExprSafeAsVariant| replace:: This callback occurs during the `infer` pass for every `ExprSafeIsVariant` (a `?as` b). If no work is necessary it should return `null`, otherwise expression will be replaced by the result.

.. |class-ast-AstReaderMacro| replace:: This macro is used to implement custom parsing functionality, i.e. anything starting with %NameOfTheMacro~ and ending when the macro says it ends.

.. |method-ast-AstReaderMacro.accept| replace:: This callback occurs during the `parse` pass for every character. When the macro is done with the input (i.e. recognizeable input ends) it should return `false`.
    Typically characters are appended to the `expr.sequence` inside the ExprReader.

.. |method-ast-AstReaderMacro.visit| replace:: This callback occurs during the `infer` pass for every instance of `ExprReader` for that specific macro. Macro needs to convert `ExprReader` to some meaningful expression.

.. |class-ast-AstCommentReader| replace:: This macro is used to implement custom comment parsing function (such as doxygen-style documentation etc).

.. |method-ast-AstCommentReader.open| replace:: This callback occurs during the `parse` pass for every // or /* sequence which indicated begining of the comment section.

.. |method-ast-AstCommentReader.accept| replace:: This callback occurs during the `parse` pass for every character in the comment section.

.. |method-ast-AstCommentReader.close| replace:: This callback occurs during the `parse` pass for every new line or \*\/ sequence which indicates end of the comment section.

.. |method-ast-AstCommentReader.beforeStructure| replace:: This callback occurs during the `parse` pass before the structure body block.

.. |method-ast-AstCommentReader.afterStructure| replace:: This callback occurs during the `parse` pass after the structure body block.

.. |method-ast-AstCommentReader.beforeStructureFields| replace:: This callback occurs during the `parse` pass before the first structure field is declared.

.. |method-ast-AstCommentReader.afterStructureField| replace:: This callback occurs during the `parse` pass after the structure field is declared (after the following comment section, should it have one).

.. |method-ast-AstCommentReader.afterStructureFields| replace:: This callback occurs during the `parse` pass after the last structure field is declared.

.. |method-ast-AstCommentReader.beforeFunction| replace:: This callback occurs during the `parse` pass before the function body block.

.. |method-ast-AstCommentReader.afterFunction| replace:: This callback occurs during the `parse` pass after the function body block.

.. |method-ast-AstCommentReader.beforeVariant| replace:: This callback occurs during the `parse` pass before the variant alias declaration.

.. |method-ast-AstCommentReader.afterVariant| replace:: This callback occurs during the `parse` after the variant alias declaration.

.. |method-ast-AstCommentReader.beforeEnumeration| replace:: This callback occurs during the `parse` before the enumeration declaration.

.. |method-ast-AstCommentReader.afterEnumeration| replace:: This callback occurs during the `parse` after the enumeration declaration.

.. |method-ast-AstCommentReader.beforeGlobalVariables| replace:: This callback occurs during the `parse` pass before the first global variable declaration but after `let` or `var` keyword.

.. |method-ast-AstCommentReader.afterGlobalVariable| replace:: This callback occurs during the `parse` pass after global variable is declaraed (after the following comment section, should it have one).

.. |method-ast-AstCommentReader.afterGlobalVariables| replace:: This callback occurs during the `parse` pass after every global variable in the declaration is declared.

.. |method-ast-AstCommentReader.beforeAlias| replace:: This callback occurs during the `parse` pass before the type alias declaration.

.. |method-ast-AstCommentReader.afterAlias| replace:: This callback occurs during the `parse` pass after the type alias declaration.

.. |class-ast-AstForLoopMacro| replace:: This macro is used to implement custom for-loop handlers. It is similar to visitExprFor callback of the AstVisitor.

.. |method-ast-AstForLoopMacro.visitExprFor| replace:: This callback occurs during the `infer` pass for every `ExprFor`. If no work is necessary it should return `null`, otherwise expression will be replaced by the result.

.. |class-ast-AstCaptureMacro| replace:: This macro is used to implement custom lambda capturing functionality.

.. |method-ast-AstCaptureMacro.captureExpression| replace:: This callback occurs during the 'infer' pass for every time a lambda expression (or generator) is captured for every captured expression.

.. |method-ast-AstCaptureMacro.captureFunction| replace:: This callback occurs during the 'infer' pass for every time a lambda expression (or generator) is captured, for every generated lambda (or generator) function.

.. |class-ast-AstCallMacro| replace:: This macro is used to implement custom call-like expressions ( like `foo(bar,bar2,...)` ).

.. |method-ast-AstCallMacro.preVisit| replace:: This callback occurs during the `infer` pass for every `ExprCallMacro`, before its arguments are inferred.

.. |method-ast-AstCallMacro.visit| replace:: This callback occurs during the `infer` pass for every `ExprCallMacro`, after its arguments are inferred. When fully inferred macro is expected to replace `ExprCallMacro` with meaningful expression.

.. |method-ast-AstCallMacro.canVisitArgument| replace:: This callback occurs during the `infer` pass before the arguments of the call macro are visited. If callback returns true, the argument of given index is visited, otherwise it acts like a query expression.

.. |method-ast-AstCallMacro.canFoldReturnResult| replace:: If true the enclosing function can infer return result as `void` when unspecified. If false function will have to wait for the macro to fold.

.. |class-ast-AstTypeInfoMacro| replace:: This macro is used to implement type info traits, i.e. `typeinfo(YourTraitHere ...)` expressions.

.. |method-ast-AstTypeInfoMacro.getAstChange| replace:: This callback occurs during the `infer` pass. If no changes are necessary it should return `null`, otherwise expression will be replaced by the result. `errors` should be filled if trait is malformed.

.. |method-ast-AstTypeInfoMacro.getAstType| replace:: This callback occurs during the `infer` pass. It should return type of the typeinfo expression. That way trait can return `Type`, and not `Expression`.

.. |class-ast-AstEnumerationAnnotation| replace:: Annotation macro which is attached to `Enumeration`.

.. |method-ast-AstEnumerationAnnotation.apply| replace:: This callback occurs during the `parse` pass. If any errors occur `errors` should be filled and `false` should be returned.

.. |class-ast-AstVisitor| replace:: This class implements `Visitor` interface for the ast tree.
    For typical expression two methods are provided: `preVisitExpr` and `visitExpr`.
    `preVisitExpr` occurs before the subexpressions are visited, and `visitExpr` occurs after the subexpressions are visited.
    `visitExpr` can return new expression which will replace the original one, or original expression - if no changes are necessary.
    There are other potential callbacks deppending of the nature of expression, which represent particular sections of the ast tree.
    Additionally 'preVisitExpression' and `visitExpression` are called before and after expression specific callbacks.

.. |method-ast-AstVisitor.preVisitProgram| replace:: before entire program, put your initialization there.

.. |method-ast-AstVisitor.visitProgram| replace:: after entire program, put your finalizers there.

.. |method-ast-AstVisitor.preVisitModule| replace:: before each module

.. |method-ast-AstVisitor.visitModule| replace:: after each module

.. |method-ast-AstVisitor.preVisitProgramBody| replace:: after enumerations, structures, and aliases, but before global variables, generics and functions.

.. |method-ast-AstVisitor.preVisitTypeDecl| replace:: before a type declaration anywhere. yor type validation code typically goes here

.. |method-ast-AstVisitor.visitTypeDecl| replace:: after a type declaration

.. |method-ast-AstVisitor.preVisitAlias| replace:: before `TypeDecl`

.. |method-ast-AstVisitor.visitAlias| replace:: after `TypeDecl`

.. |method-ast-AstVisitor.canVisitCall| replace:: If false call will be completely skipped, otherwise it behaves normally.

.. |method-ast-AstVisitor.canVisitWithAliasSubexpression| replace:: before the sub expression in the `ExprAssume`

.. |method-ast-AstVisitor.canVisitMakeBlockBody| replace:: before the body of the `makeBlock` expression is visited. If true `body` will be visited

.. |method-ast-AstVisitor.canVisitEnumeration| replace:: if true `Enumeration` will be visited

.. |method-ast-AstVisitor.preVisitEnumeration| replace:: before `Enumeration`

.. |method-ast-AstVisitor.preVisitEnumerationValue| replace:: before every enumeration entry

.. |method-ast-AstVisitor.visitEnumerationValue| replace:: after every enumeration entry

.. |method-ast-AstVisitor.visitEnumeration| replace:: after `Enumeration`

.. |method-ast-AstVisitor.canVisitStructure| replace:: if true `Structure` will be visited

.. |method-ast-AstVisitor.preVisitStructure| replace:: before `Structure`

.. |method-ast-AstVisitor.preVisitStructureField| replace:: before every structure field

.. |method-ast-AstVisitor.visitStructureField| replace:: after every structure field

.. |method-ast-AstVisitor.visitStructure| replace:: after `Structure`

.. |method-ast-AstVisitor.canVisitFunction| replace:: if true `Function` will be visited

.. |method-ast-AstVisitor.canVisitFunctionArgumentInit| replace:: if true function argument initialization expressions will be visited

.. |method-ast-AstVisitor.preVisitFunction| replace:: before `Function`

.. |method-ast-AstVisitor.visitFunction| replace:: after `Function`

.. |method-ast-AstVisitor.preVisitFunctionArgument| replace:: before every argument

.. |method-ast-AstVisitor.visitFunctionArgument| replace:: after every argument

.. |method-ast-AstVisitor.preVisitFunctionArgumentInit| replace:: before every argument initialization expression (should it have one), between 'preVisitFunctionArgument' and `visitFunctionArgument`

.. |method-ast-AstVisitor.visitFunctionArgumentInit| replace:: after every argument initialization expression (should it have one), between 'preVisitFunctionArgument' and `visitFunctionArgument`

.. |method-ast-AstVisitor.preVisitFunctionBody| replace:: before the `Function` body block, between `preVisitFunction` and `visitFunction` (not for abstract functions)

.. |method-ast-AstVisitor.visitFunctionBody| replace:: after the `Function` body block, between `preVisitFunction` and `visitFunction` (not for abstract functions)

.. |method-ast-AstVisitor.preVisitExpression| replace:: before every `Expression`

.. |method-ast-AstVisitor.visitExpression| replace:: after every `Expression`

.. |method-ast-AstVisitor.preVisitExprBlock| replace:: before `ExprBlock`

.. |method-ast-AstVisitor.visitExprBlock| replace:: after `ExprBlock`

.. |method-ast-AstVisitor.preVisitExprBlockArgument| replace:: before every block argument

.. |method-ast-AstVisitor.visitExprBlockArgument| replace:: after every block argument

.. |method-ast-AstVisitor.preVisitExprBlockArgumentInit| replace:: before every block argument initialization expression (should it have one), between 'preVisitExprBlockArgument' and `visitExprBlockArgument`

.. |method-ast-AstVisitor.visitExprBlockArgumentInit| replace:: after every block argument initialization expression (should it have one), between 'preVisitExprBlockArgument' and `visitExprBlockArgument`

.. |method-ast-AstVisitor.preVisitExprBlockExpression| replace:: before every block expression

.. |method-ast-AstVisitor.visitExprBlockExpression| replace:: after every block expression

.. |method-ast-AstVisitor.preVisitExprBlockFinal| replace:: before `finally`` section of the block

.. |method-ast-AstVisitor.visitExprBlockFinal| replace:: after `finally`` section of the block

.. |method-ast-AstVisitor.preVisitExprBlockFinalExpression| replace:: before every block expression in the `finally` section, between `preVisitExprBlockFinal` and `visitExprBlockFinal`

.. |method-ast-AstVisitor.visitExprBlockFinalExpression| replace:: after every block expression in the `finally`` section, between `preVisitExprBlockFinal` and `visitExprBlockFinal`

.. |method-ast-AstVisitor.preVisitExprLet| replace:: before `ExprLet`

.. |method-ast-AstVisitor.visitExprLet| replace:: after `ExprLet`

.. |method-ast-AstVisitor.preVisitExprLetVariable| replace:: before every variable

.. |method-ast-AstVisitor.visitExprLetVariable| replace:: after every variable

.. |method-ast-AstVisitor.preVisitExprLetVariableInit| replace:: before variable initialization (should it have one), between `preVisitExprLetVariable` and `visitExprLetVariable`

.. |method-ast-AstVisitor.visitExprLetVariableInit| replace:: after variable initialization (should it have one), between `preVisitExprLetVariable` and `visitExprLetVariable`

.. |method-ast-AstVisitor.canVisitGlobalVariable| replace:: If true global variable declaration will be visited

.. |method-ast-AstVisitor.preVisitGlobalLet| replace:: before global variable declaration

.. |method-ast-AstVisitor.visitGlobalLet| replace:: after global variable declaration

.. |method-ast-AstVisitor.preVisitGlobalLetVariable| replace:: before every global variable

.. |method-ast-AstVisitor.visitGlobalLetVariable| replace:: after every global variable

.. |method-ast-AstVisitor.preVisitGlobalLetVariableInit| replace:: before global variable initialization (should it have one), between `preVisitGlobalLetVariable` and `visitGlobalLetVariable`

.. |method-ast-AstVisitor.visitGlobalLetVariableInit| replace:: after global variable initialization (should it have one), between `preVisitGlobalLetVariable` and `visitGlobalLetVariable`

.. |method-ast-AstVisitor.preVisitExprStringBuilder| replace:: before `ExprStringBuilder`

.. |method-ast-AstVisitor.visitExprStringBuilder| replace:: after `ExprStringBuilder`

.. |method-ast-AstVisitor.preVisitExprStringBuilderElement| replace:: before any element of string builder (string or expression)

.. |method-ast-AstVisitor.visitExprStringBuilderElement| replace:: after any element of string builder

.. |method-ast-AstVisitor.preVisitExprNew| replace:: before `ExprNew`

.. |method-ast-AstVisitor.visitExprNew| replace:: after `ExprNew`

.. |method-ast-AstVisitor.preVisitExprNewArgument| replace:: before every argument

.. |method-ast-AstVisitor.visitExprNewArgument| replace:: after every argument

.. |method-ast-AstVisitor.preVisitExprNamedCall| replace:: before `ExprNamedCall`

.. |method-ast-AstVisitor.visitExprNamedCall| replace:: after `ExprNamedCall``

.. |method-ast-AstVisitor.preVisitExprNamedCallArgument| replace:: before every argument

.. |method-ast-AstVisitor.visitExprNamedCallArgument| replace:: after every argument

.. |method-ast-AstVisitor.preVisitExprLooksLikeCall| replace:: before `ExprLooksLikeCall`

.. |method-ast-AstVisitor.visitExprLooksLikeCall| replace:: after `ExprLooksLikeCall`

.. |method-ast-AstVisitor.preVisitExprLooksLikeCallArgument| replace:: before every argument

.. |method-ast-AstVisitor.visitExprLooksLikeCallArgument| replace:: after every argument

.. |method-ast-AstVisitor.preVisitExprCall| replace:: before `ExprCall`

.. |method-ast-AstVisitor.visitExprCall| replace:: after `ExprCall`

.. |method-ast-AstVisitor.preVisitExprCallArgument| replace:: before every argument

.. |method-ast-AstVisitor.visitExprCallArgument| replace:: after every argument

.. |method-ast-AstVisitor.preVisitExprNullCoalescing| replace:: before `ExprNullCoalescing`

.. |method-ast-AstVisitor.visitExprNullCoalescing| replace:: after `ExprNullCoalescing`

.. |method-ast-AstVisitor.preVisitExprNullCoalescingDefault| replace:: before the default value

.. |method-ast-AstVisitor.preVisitExprAt| replace:: before `ExprAt`

.. |method-ast-AstVisitor.visitExprAt| replace:: after `ExprAt`

.. |method-ast-AstVisitor.preVisitExprAtIndex| replace:: before the index

.. |method-ast-AstVisitor.preVisitExprSafeAt| replace:: before `ExprSafeAt`

.. |method-ast-AstVisitor.visitExprSafeAt| replace:: after `ExprSafeAt`

.. |method-ast-AstVisitor.preVisitExprSafeAtIndex| replace:: before the index

.. |method-ast-AstVisitor.preVisitExprIs| replace:: before `ExprIs`

.. |method-ast-AstVisitor.visitExprIs| replace:: after `ExprIs`

.. |method-ast-AstVisitor.preVisitExprIsType| replace:: before the type

.. |method-ast-AstVisitor.preVisitExprOp2| replace:: before `ExprOp2`

.. |method-ast-AstVisitor.visitExprOp2| replace:: after `ExprOp2`

.. |method-ast-AstVisitor.preVisitExprOp2Right| replace:: before the right operand

.. |method-ast-AstVisitor.preVisitExprOp3| replace:: before `ExprOp3`

.. |method-ast-AstVisitor.visitExprOp3| replace:: after `ExprOp3`

.. |method-ast-AstVisitor.preVisitExprOp3Left| replace:: before the left option

.. |method-ast-AstVisitor.preVisitExprOp3Right| replace:: before the right option

.. |method-ast-AstVisitor.preVisitExprCopy| replace:: before `ExprCopy`

.. |method-ast-AstVisitor.visitExprCopy| replace:: after `ExprCopy`

.. |method-ast-AstVisitor.preVisitExprCopyRight| replace:: before the right operand

.. |method-ast-AstVisitor.preVisitExprMove| replace:: before `ExprMove`

.. |method-ast-AstVisitor.visitExprMove| replace:: after `ExprMove`

.. |method-ast-AstVisitor.preVisitExprMoveRight| replace:: before the right operand

.. |method-ast-AstVisitor.preVisitExprClone| replace:: before `ExprClone`

.. |method-ast-AstVisitor.visitExprClone| replace:: after `ExprClone`

.. |method-ast-AstVisitor.preVisitExprCloneRight| replace:: before the right operand

.. |method-ast-AstVisitor.preVisitExprAssume| replace:: before `ExprAssume`

.. |method-ast-AstVisitor.visitExprAssume| replace:: after `ExprAssume`

.. |method-ast-AstVisitor.preVisitExprWith| replace:: before `ExprWith`

.. |method-ast-AstVisitor.visitExprWith| replace:: after `ExprWith`

.. |method-ast-AstVisitor.preVisitExprWithBody| replace:: before the body block

.. |method-ast-AstVisitor.preVisitExprWhile| replace:: before `ExprWhile`

.. |method-ast-AstVisitor.visitExprWhile| replace:: after `ExprWhile`

.. |method-ast-AstVisitor.preVisitExprWhileBody| replace:: before the body block

.. |method-ast-AstVisitor.preVisitExprTryCatch| replace:: before `ExprTryCatch`

.. |method-ast-AstVisitor.visitExprTryCatch| replace:: after `ExprTryCatch`

.. |method-ast-AstVisitor.preVisitExprTryCatchCatch| replace:: before the catch (recover) section

.. |method-ast-AstVisitor.preVisitExprIfThenElse| replace:: before `ExprIfThenElse`

.. |method-ast-AstVisitor.visitExprIfThenElse| replace:: after `ExprIfThenElse`

.. |method-ast-AstVisitor.preVisitExprIfThenElseIfBlock| replace:: before the if block

.. |method-ast-AstVisitor.preVisitExprIfThenElseElseBlock| replace:: before the else block

.. |method-ast-AstVisitor.preVisitExprFor| replace:: before the `ExprFor`

.. |method-ast-AstVisitor.visitExprFor| replace:: after the `ExprFor`

.. |method-ast-AstVisitor.preVisitExprForVariable| replace:: before each variable

.. |method-ast-AstVisitor.visitExprForVariable| replace:: after each variable

.. |method-ast-AstVisitor.preVisitExprForSource| replace:: before each source

.. |method-ast-AstVisitor.visitExprForSource| replace:: after each source

.. |method-ast-AstVisitor.preVisitExprForStack| replace:: before the stack is allocated before the body, regardless if it has one

.. |method-ast-AstVisitor.preVisitExprForBody| replace:: before the body (should it have one)

.. |method-ast-AstVisitor.preVisitExprMakeVariant| replace:: before `ExprMakeVariant`

.. |method-ast-AstVisitor.visitExprMakeVariant| replace:: after `ExprMakeVariant`

.. |method-ast-AstVisitor.preVisitExprMakeVariantField| replace:: before every field

.. |method-ast-AstVisitor.visitExprMakeVariantField| replace:: after every field

.. |method-ast-AstVisitor.canVisitMakeStructBody| replace:: if true the visitor can visit the body of `ExprMakeStruct`

.. |method-ast-AstVisitor.canVisitMakeStructBlock| replace:: if true the visitor can visit the block behind `ExprMakeStruct`

.. |method-ast-AstVisitor.preVisitExprMakeStruct| replace:: before `ExprMakeStruct`

.. |method-ast-AstVisitor.visitExprMakeStruct| replace:: after `ExprMakeStruct`

.. |method-ast-AstVisitor.preVisitExprMakeStructIndex| replace:: before each struct in the array of structures

.. |method-ast-AstVisitor.visitExprMakeStructIndex| replace:: after each struct in the array of structures

.. |method-ast-AstVisitor.preVisitExprMakeStructField| replace:: before each field of the struct, between `preVisitExprMakeStructIndex` and `visitExprMakeStructIndex`

.. |method-ast-AstVisitor.visitExprMakeStructField| replace:: after each field of the struct, between `preVisitExprMakeStructIndex` and `visitExprMakeStructIndex`

.. |method-ast-AstVisitor.preVisitExprMakeArray| replace:: before `ExprMakeArray`

.. |method-ast-AstVisitor.visitExprMakeArray| replace:: after `ExprMakeArray`

.. |method-ast-AstVisitor.preVisitExprMakeArrayIndex| replace:: before each element of the array

.. |method-ast-AstVisitor.visitExprMakeArrayIndex| replace:: after each element of the array

.. |method-ast-AstVisitor.preVisitExprMakeTuple| replace:: before `ExprMakeTuple`

.. |method-ast-AstVisitor.visitExprMakeTuple| replace:: after `ExprMakeTuple`

.. |method-ast-AstVisitor.preVisitExprMakeTupleIndex| replace:: before each field of the tuple

.. |method-ast-AstVisitor.visitExprMakeTupleIndex| replace:: after each field of the tuple

.. |method-ast-AstVisitor.preVisitExprArrayComprehension| replace:: before `ExprArrayComprehension`

.. |method-ast-AstVisitor.visitExprArrayComprehension| replace:: after `ExprArrayComprehension`

.. |method-ast-AstVisitor.preVisitExprArrayComprehensionSubexpr| replace:: before the subexpression

.. |method-ast-AstVisitor.preVisitExprArrayComprehensionWhere| replace:: before the where clause

.. |method-ast-AstVisitor.preVisitExprTypeInfo| replace:: before `ExprTypeInfo`

.. |method-ast-AstVisitor.visitExprTypeInfo| replace:: after `ExprTypeInfo`

.. |method-ast-AstVisitor.preVisitExprPtr2Ref| replace:: before `ExprPtr2Ref`

.. |method-ast-AstVisitor.visitExprPtr2Ref| replace:: after `ExprPtr2Ref`

.. |method-ast-AstVisitor.preVisitExprLabel| replace:: before `ExprLabel`

.. |method-ast-AstVisitor.visitExprLabel| replace:: after `ExprLabel`

.. |method-ast-AstVisitor.preVisitExprGoto| replace:: before `ExprGoto`

.. |method-ast-AstVisitor.visitExprGoto| replace:: after `ExprGoto`

.. |method-ast-AstVisitor.preVisitExprRef2Value| replace:: before `ExprRef2Value`

.. |method-ast-AstVisitor.visitExprRef2Value| replace:: after `ExprRef2Value`

.. |method-ast-AstVisitor.preVisitExprRef2Ptr| replace:: before `ExprRef2Ptr`

.. |method-ast-AstVisitor.visitExprRef2Ptr| replace:: after `ExprRef2Ptr`

.. |method-ast-AstVisitor.preVisitExprAddr| replace:: before `ExprAddr`

.. |method-ast-AstVisitor.visitExprAddr| replace:: after `ExprAddr`

.. |method-ast-AstVisitor.preVisitExprAssert| replace:: before `ExprAssert`

.. |method-ast-AstVisitor.visitExprAssert| replace:: after `ExprAssert`

.. |method-ast-AstVisitor.preVisitExprStaticAssert| replace:: before `ExprStaticAssert`

.. |method-ast-AstVisitor.visitExprStaticAssert| replace:: after `ExprStaticAssert`

.. |method-ast-AstVisitor.preVisitExprQuote| replace:: before `ExprQuote`

.. |method-ast-AstVisitor.visitExprQuote| replace:: after `ExprQuote`

.. |method-ast-AstVisitor.preVisitExprDebug| replace:: before `ExprDebug`

.. |method-ast-AstVisitor.visitExprDebug| replace:: after `ExprDebug`

.. |method-ast-AstVisitor.preVisitExprInvoke| replace:: before `ExprInvoke`

.. |method-ast-AstVisitor.visitExprInvoke| replace:: after `ExprInvoke`

.. |method-ast-AstVisitor.preVisitExprErase| replace:: before `ExprErase`

.. |method-ast-AstVisitor.visitExprErase| replace:: after `ExprErase`

.. |method-ast-AstVisitor.preVisitExprFind| replace:: before `ExprFind`

.. |method-ast-AstVisitor.visitExprFind| replace:: after `ExprFind`

.. |method-ast-AstVisitor.preVisitExprKeyExists| replace:: before `ExprKeyExists`

.. |method-ast-AstVisitor.visitExprKeyExists| replace:: after `ExprKeyExists`

.. |method-ast-AstVisitor.preVisitExprAscend| replace:: before `ExprAscend`

.. |method-ast-AstVisitor.visitExprAscend| replace:: after `ExprAscend`

.. |method-ast-AstVisitor.preVisitExprCast| replace:: before `ExprCast`

.. |method-ast-AstVisitor.visitExprCast| replace:: after `ExprCast`

.. |method-ast-AstVisitor.preVisitExprDelete| replace:: before `ExprDelete`

.. |method-ast-AstVisitor.visitExprDelete| replace:: after `ExprDelete`

.. |method-ast-AstVisitor.preVisitExprVar| replace:: before `ExprVar`

.. |method-ast-AstVisitor.visitExprVar| replace:: after `ExprVar`

.. |method-ast-AstVisitor.preVisitExprField| replace:: before `ExprField`

.. |method-ast-AstVisitor.visitExprField| replace:: after `ExprField`

.. |method-ast-AstVisitor.preVisitExprSafeField| replace:: before `ExprSafeField`

.. |method-ast-AstVisitor.visitExprSafeField| replace:: after `ExprSafeField`

.. |method-ast-AstVisitor.preVisitExprSwizzle| replace:: before `ExprSwizzle`

.. |method-ast-AstVisitor.visitExprSwizzle| replace:: after `ExprSwizzle`

.. |method-ast-AstVisitor.preVisitExprIsVariant| replace:: before `ExprIsVariant`

.. |method-ast-AstVisitor.visitExprIsVariant| replace:: after `ExprIsVariant`

.. |method-ast-AstVisitor.preVisitExprAsVariant| replace:: before `ExprAsVariant`

.. |method-ast-AstVisitor.visitExprAsVariant| replace:: after `ExprAsVariant`

.. |method-ast-AstVisitor.preVisitExprSafeAsVariant| replace:: before `ExprSafeAsVariant`

.. |method-ast-AstVisitor.visitExprSafeAsVariant| replace:: after `ExprSafeAsVariant`

.. |method-ast-AstVisitor.preVisitExprOp1| replace:: before `ExprOp1`

.. |method-ast-AstVisitor.visitExprOp1| replace:: after `ExprOp1`

.. |method-ast-AstVisitor.preVisitExprReturn| replace:: before `ExprReturn`

.. |method-ast-AstVisitor.visitExprReturn| replace:: after `ExprReturn`

.. |method-ast-AstVisitor.preVisitExprYield| replace:: before `ExprYield`

.. |method-ast-AstVisitor.visitExprYield| replace:: after 'ExprYield'

.. |method-ast-AstVisitor.preVisitExprBreak| replace:: before `ExprBreak`

.. |method-ast-AstVisitor.visitExprBreak| replace:: after `ExprBreak`

.. |method-ast-AstVisitor.preVisitExprContinue| replace:: before `ExprContinue`

.. |method-ast-AstVisitor.visitExprContinue| replace:: after `ExprContinue`

.. |method-ast-AstVisitor.preVisitExprMakeBlock| replace:: before `ExprMakeBlock`

.. |method-ast-AstVisitor.visitExprMakeBlock| replace:: after `ExprMakeBlock`

.. |method-ast-AstVisitor.preVisitExprMakeGenerator| replace:: before `ExprMakeGenerator`

.. |method-ast-AstVisitor.visitExprMakeGenerator| replace:: after `ExprMakeGenerator`

.. |method-ast-AstVisitor.preVisitExprMemZero| replace:: before `ExprMemZero`

.. |method-ast-AstVisitor.visitExprMemZero| replace:: after `ExprMemZero`

.. |method-ast-AstVisitor.preVisitExprConst| replace:: before `ExprConst`

.. |method-ast-AstVisitor.visitExprConst| replace:: after `ExprConst`

.. |method-ast-AstVisitor.preVisitExprConstPtr| replace:: before `ExprConstPtr`

.. |method-ast-AstVisitor.visitExprConstPtr| replace:: after `ExprConstPtr`

.. |method-ast-AstVisitor.preVisitExprConstEnumeration| replace:: before `ExprConstEnumeration`

.. |method-ast-AstVisitor.visitExprConstEnumeration| replace:: after `ExprConstEnumeration`

.. |method-ast-AstVisitor.preVisitExprConstBitfield| replace:: before `ExprConstBitfield`

.. |method-ast-AstVisitor.visitExprConstBitfield| replace:: after `ExprConstBitfield`

.. |method-ast-AstVisitor.preVisitExprConstInt8| replace:: before `ExprConstInt8`

.. |method-ast-AstVisitor.visitExprConstInt8| replace:: after `ExprConstInt8`

.. |method-ast-AstVisitor.preVisitExprConstInt16| replace:: before `ExprConstInt16`

.. |method-ast-AstVisitor.visitExprConstInt16| replace:: after `ExprConstInt16`

.. |method-ast-AstVisitor.preVisitExprConstInt64| replace:: before `ExprConstInt64`

.. |method-ast-AstVisitor.visitExprConstInt64| replace:: after `ExprConstInt64`

.. |method-ast-AstVisitor.preVisitExprConstInt| replace:: before `ExprConstInt`

.. |method-ast-AstVisitor.visitExprConstInt| replace:: after `ExprConstInt`

.. |method-ast-AstVisitor.preVisitExprConstInt2| replace:: before `ExprConstInt2`

.. |method-ast-AstVisitor.visitExprConstInt2| replace:: after `ExprConstInt2`

.. |method-ast-AstVisitor.preVisitExprConstInt3| replace:: before `ExprConstInt3`

.. |method-ast-AstVisitor.visitExprConstInt3| replace:: after `ExprConstInt3`

.. |method-ast-AstVisitor.preVisitExprConstInt4| replace:: before `ExprConstInt4`

.. |method-ast-AstVisitor.visitExprConstInt4| replace:: after `ExprConstInt4`

.. |method-ast-AstVisitor.preVisitExprConstUInt8| replace:: before `ExprConstUInt8`

.. |method-ast-AstVisitor.visitExprConstUInt8| replace:: after `ExprConstUInt8`

.. |method-ast-AstVisitor.preVisitExprConstUInt16| replace:: before `ExprConstUInt16`

.. |method-ast-AstVisitor.visitExprConstUInt16| replace:: after `ExprConstUInt16`

.. |method-ast-AstVisitor.preVisitExprConstUInt64| replace:: before `ExprConstUInt64`

.. |method-ast-AstVisitor.visitExprConstUInt64| replace:: after `ExprConstUInt64`

.. |method-ast-AstVisitor.preVisitExprConstUInt| replace:: before `ExprConstUInt`

.. |method-ast-AstVisitor.visitExprConstUInt| replace:: after `ExprConstUInt`

.. |method-ast-AstVisitor.preVisitExprConstUInt2| replace:: before `ExprConstUInt2`

.. |method-ast-AstVisitor.visitExprConstUInt2| replace:: after `ExprConstUInt2`

.. |method-ast-AstVisitor.preVisitExprConstUInt3| replace:: before `ExprConstUInt3`

.. |method-ast-AstVisitor.visitExprConstUInt3| replace:: after `ExprConstUInt3`

.. |method-ast-AstVisitor.preVisitExprConstUInt4| replace:: before `ExprConstUInt4`

.. |method-ast-AstVisitor.visitExprConstUInt4| replace:: after `ExprConstUInt4`

.. |method-ast-AstVisitor.preVisitExprConstRange| replace:: before `ExprConstRange`

.. |method-ast-AstVisitor.visitExprConstRange| replace:: after `ExprConstRange`

.. |method-ast-AstVisitor.preVisitExprConstURange| replace:: before `ExprConstURange`

.. |method-ast-AstVisitor.visitExprConstURange| replace:: after `ExprConstURange`

.. |method-ast-AstVisitor.preVisitExprConstRange64| replace:: before `ExprConstRange64`

.. |method-ast-AstVisitor.visitExprConstRange64| replace:: after `ExprConstRange64`

.. |method-ast-AstVisitor.preVisitExprConstURange64| replace:: before `ExprConstURange64`

.. |method-ast-AstVisitor.visitExprConstURange64| replace:: after `ExprConstURange64`

.. |method-ast-AstVisitor.preVisitExprConstBool| replace:: before `ExprConstBool`

.. |method-ast-AstVisitor.visitExprConstBool| replace:: after `ExprConstBool`

.. |method-ast-AstVisitor.preVisitExprConstFloat| replace:: before `ExprConstFloat`

.. |method-ast-AstVisitor.visitExprConstFloat| replace:: after `ExprConstFloat`

.. |method-ast-AstVisitor.preVisitExprConstFloat2| replace:: before `ExprConstFloat2`

.. |method-ast-AstVisitor.visitExprConstFloat2| replace:: after `ExprConstFloat2`

.. |method-ast-AstVisitor.preVisitExprConstFloat3| replace:: before `ExprConstFloat3`

.. |method-ast-AstVisitor.visitExprConstFloat3| replace:: after `ExprConstFloat3`

.. |method-ast-AstVisitor.preVisitExprConstFloat4| replace:: before `ExprConstFloat4`

.. |method-ast-AstVisitor.visitExprConstFloat4| replace:: after `ExprConstFloat4`

.. |method-ast-AstVisitor.preVisitExprConstString| replace:: before `ExprConstString`

.. |method-ast-AstVisitor.visitExprConstString| replace:: after `ExprConstString`

.. |method-ast-AstVisitor.preVisitExprConstDouble| replace:: before `ExprConstDouble`

.. |method-ast-AstVisitor.visitExprConstDouble| replace:: after `ExprConstDouble`

.. |method-ast-AstVisitor.preVisitExprFakeContext| replace:: before `ExprConstFakeContext`

.. |method-ast-AstVisitor.visitExprFakeContext| replace:: after `ExprConstFakeContext`

.. |method-ast-AstVisitor.preVisitExprFakeLineInfo| replace:: before `ExprConstFakeLineInfo`

.. |method-ast-AstVisitor.visitExprFakeLineInfo| replace:: after `ExprConstFakeLineInfo`

.. |method-ast-AstVisitor.preVisitExprReader| replace:: before `ExprReader`

.. |method-ast-AstVisitor.visitExprReader| replace:: after `ExprReader`

.. |method-ast-AstVisitor.preVisitExprUnsafe| replace:: before `ExprUnsafe`

.. |method-ast-AstVisitor.visitExprUnsafe| replace:: after `ExprUnsafe`

.. |method-ast-AstVisitor.preVisitExprCallMacro| replace:: before `ExprCallMacro`

.. |method-ast-AstVisitor.visitExprCallMacro| replace:: after `ExprCallMacro`

.. |method-ast-AstVisitor.preVisitExprSetInsert| replace:: before `ExprSetInsert`

.. |method-ast-AstVisitor.visitExprSetInsert| replace:: after `ExprSetInsert`

.. |method-ast-AstVisitor.preVisitExprTag| replace:: before `ExprTag`

.. |method-ast-AstVisitor.preVisitExprTagValue| replace:: before the value portion of `ExprTag`

.. |method-ast-AstVisitor.visitExprTag| replace:: after `ExprTag`

.. |function-ast-make_visitor| replace:: Creates adapter for the `AstVisitor` interface.

.. |function-ast-visit| replace:: Invokes visitor for the given object.

.. |function-ast-visit_modules| replace:: Invokes visitor for the given list of modules inside the `Program`.

.. |function-ast-make_function_annotation| replace:: Creates adapter for the `AstFunctionAnnotation`.

.. |function-ast-make_block_annotation| replace:: Creates adapter for the `AstBlockAnnotation`.

.. |function-ast-add_function_annotation| replace:: Adds function annotation to the given object. Calls `apply` if applicable.

.. |function-ast-make_structure_annotation| replace:: Creates adapter for the `AstStructureAnnotation`.

.. |function-ast-add_structure_annotation| replace:: Adds structure annotation to the given object. Calls `apply` if applicable.

.. |function-ast-make_enumeration_annotation| replace:: Creates adapter for the `AstEnumearationAnnotation`.

.. |function-ast-add_enumeration_annotation| replace:: Adds enumeration annotation to the given object. Calls `apply` if applicable.

.. |function-ast-add_enumeration_entry| replace:: Adds entry to enumeration annotation.

.. |function-ast-make_pass_macro| replace:: Creates adapter for the `AstPassMacro`.

.. |function-ast-add_infer_macro| replace:: Adds `AstPassMacro` adapter to the `infer`` pass.

.. |function-ast-add_dirty_infer_macro| replace:: Adds `AstPassMacro` adapter to the `dirty infer` pass.

.. |function-ast-add_lint_macro| replace:: Adds `AstPassMacro` adapter to the `lint` pass.

.. |function-ast-add_global_lint_macro| replace:: Adds `AstPassMacro` adapter to the `global lint` pass.

.. |function-ast-add_optimization_macro| replace:: Adds `AstPassMacro` adapter to the `optimization` pass.

.. |function-ast-make_reader_macro| replace:: Creates adapter for the `AstReaderMacro`.

.. |function-ast-add_reader_macro| replace:: Adds `AstReaderMacro` adapter to the specific module.

.. |function-ast-make_comment_reader| replace:: Creates adapter for the `AstCommentReader`.

.. |function-ast-add_comment_reader| replace:: Adds `AstCommentReader` adapter to the specific module.

.. |function-ast-make_call_macro| replace:: Creates adapter for the `AstCallMacro`.

.. |function-ast-add_call_macro| replace:: Adds `AstCallMacro` adapter to the specific module.

.. |function-ast-make_typeinfo_macro| replace:: Creates adapter for the `AstTypeInfo` macro.

.. |function-ast-add_typeinfo_macro| replace:: Adds `AstTypeInfo` adapter to the specific module.

.. |function-ast-make_variant_macro| replace:: Creates adapter for the `AstVariantMacro`.

.. |function-ast-add_variant_macro| replace:: Adds `AstVariantMacro` to the specific module.

.. |function-ast-make_for_loop_macro| replace:: Creates adapter for the `AstForLoopMacro`.

.. |function-ast-add_for_loop_macro| replace:: Adds `AstForLoopMacro` to the specific module.

.. |function-ast-add_new_for_loop_macro| replace:: Makes adapter to the `AstForLoopMacro` and adds it to the current module.

.. |function-ast-make_capture_macro| replace:: Creates adapter for the `AstCaptureMacro`.

.. |function-ast-add_capture_macro| replace:: Adds `AstCaptureMacro` to the specific module.

.. |function-ast-add_new_capture_macro| replace:: Makes adapter to the `AstCaptureMacro` and adds it to the current module.

.. |function-ast-this_program| replace:: Program attached to the current context (or null if RTTI is disabled).

.. |function-ast-this_module| replace:: Main module attached to the current context (will through if RTTI is disabled).

.. |function-ast-find_module_via_rtti| replace:: Find module by name in the `Program`.

.. |function-ast-find_module_function_via_rtti| replace:: Find function by name in the `Module`.

.. |function-ast-compiling_program| replace:: Currently compiling program.

.. |function-ast-compiling_module| replace:: Currently compiling module.

.. |function-ast-for_each_function| replace:: Iterates through each function in the given `Module`. If the `name` is empty matches all functions.

.. |function-ast-for_each_generic| replace:: Iterates through each generic function in the given `Module`.

.. |function-ast-for_each_reader_macro| replace:: Iterates through each reader macro in the given `Module`.

.. |function-ast-for_each_variant_macro| replace:: Iterates through each variant macro in the given `Module`.

.. |function-ast-for_each_typeinfo_macro| replace:: Iterates through each typeinfo macro in the given `Module`.

.. |function-ast-for_each_for_loop_macro| replace:: Iterates through each for loop macro in the given `Module`.

.. |function-ast-force_at| replace:: Replaces line info in the expression, its subexpressions, and its types.

.. |function-ast-parse_mangled_name| replace:: Parses mangled name and creates corresponding `TypeDecl`.

.. |function-ast-collect_dependencies| replace:: Collects dependencies of the given function (other functions it calls, global variables it accesses).

.. |function-ast-add_function| replace:: Adds function to a `Module`. Will return false on duplicates.

.. |function-ast-add_generic| replace:: Adds generic function to a `Module`. Will return false on duplicates.

.. |function-ast-add_variable| replace:: Adds variable to a `Module`. Will return false on duplicates.

.. |function-ast-find_variable| replace:: Finds variable in the `Module`.

.. |function-ast-add_structure| replace:: Adds structure to a `Module`. Will return false on duplicates.

.. |function-ast-clone_structure| replace:: Returns clone of the `Structure`.

.. |function-ast-add_keyword| replace:: Adds new `keyword`. It can appear in the `keyword <type> expr` or `keyword expr block` syntax. See daslib/match as implementation example.

.. |function-ast-describe_typedecl| replace:: Returns description of the `TypeDecl` which should match corresponding daScript type declaration.

.. |function-ast-describe_typedecl_cpp| replace:: Returns description of the `TypeDecl` which should match corresponding C++ type declaration.

.. |function-ast-describe_expression| replace:: Returns description of the `Expression` which should match corresponding daScript code.

.. |function-ast-describe_function| replace:: Returns description of the `Function` which should match corresponding daScript function declaration.

.. |function-ast-find_bitfield_name| replace:: Finds name of the corresponding bitfield value in the specified type.

.. |function-ast-find_enum_value| replace:: Finds name of the corresponding enumeration value in the specified type.

.. |function-ast-get_mangled_name| replace:: Returns mangled name of the object.

.. |function-ast-das_to_string| replace:: Returns description (name) of the corresponding `Type`.

.. |function-ast-clone_expression| replace:: Clones `Expression` with subexpressions, including corresponding type.

.. |function-ast-clone_function| replace:: Clones `Function` and everything in it.

.. |function-ast-clone_variable| replace:: Clones `Variable` and everything in it.

.. |function-ast-is_temp_type| replace:: Returns true if type can be temporary.

.. |function-ast-is_same_type| replace:: Compares two types given comparison parameters and returns true if they match.

.. |function-ast-clone_type| replace:: Clones `TypeDecl` with subtypes.

.. |function-ast-get_variant_field_offset| replace:: Returns offset of the variant field in bytes.

.. |function-ast-get_tuple_field_offset| replace:: Returns offset of the tuple field in bytes.

.. |function-ast-any_table_foreach| replace:: Iterates through any table<> type in a typeless fasion (via void?)

.. |function-ast-any_array_foreach| replace:: Iterates through any array<> type in a typeless fasion (via void?)

.. |function-ast-any_array_size| replace:: Returns array size from pointer to array<> object.

.. |function-ast-any_table_size| replace:: Returns table size from pointer to the table<> object.

.. |function-ast-for_each_typedef| replace:: Iterates through every typedef in the `Module`.

.. |function-ast-for_each_enumeration| replace:: Iterates through every enumeration in the `Module`.

.. |function-ast-for_each_structure| replace:: Iterates through every structure in the `Module`.

.. |function-ast-for_each_global| replace:: Iterates through every global variable in the `Module`.

.. |function-ast-for_each_call_macro| replace:: Iterates through every CallMacro adapter in the `Module`.

.. |function-ast-for_each_field| replace:: Iterates through every field in the `BuiltinStructure` handled type.

.. |function-ast-has_field| replace:: Returns if structure, variant, tuple, or handled type or pointer to either of those has specific field.

.. |function-ast-get_field_type| replace:: Returns type of the field if structure, variant, tuple, or handled type or pointer to either of those has it. It's null otherwise.

.. |function-ast-is_visible_directly| replace:: Returns true if module is visible directly from the other module.

.. |function-ast-get_ast_context| replace:: Returns `AstContext` for the given expression. It includes current function (if applicable), loops, blocks, scopes, and with sections.

.. |function-ast-make_clone_structure| replace:: Generates `clone` function for the given structure.

.. |function-ast-is_expr_like_call| replace:: Returns true if expression is or inherited from `ExprLooksLikeCall`

.. |function-ast-is_expr_const| replace:: Returns true if expression is or inherited from `ExprConst`

.. |function-ast-make_call| replace:: Creates appropriate call expression for the given call function name in the `Program`.
    `ExprCallMacro` will be created if appropriate macro is found. Otherwise `ExprCall` will be created.

.. |function-ast-eval_single_expression| replace:: Simulates and evaluates single expression on the separate context.
    If expression has external references, simulation will likely fail. Global variable access or function calls will produce exceptions.

.. |function-ast-macro_error| replace:: Reports error to the currently compiling program to whatever current pass is.
    Usually called from inside the macro function.

.. |function-ast-describe| replace:: Describes object and produces corresponding daScript code as string.

.. |function-ast-describe_cpp| replace:: Describes `TypeDecl` and produces corresponding C++ code as a string.

.. |function-ast-ExpressionPtr| replace:: Returns ExpressionPtr out of any smart pointer to `Expression`.

.. |function-ast-StructurePtr| replace:: Returns StructurePtr out of any smart pointer to `Structure`.

.. |function-ast-FunctionPtr| replace:: Returns FunctionPtr out of Function?

.. |function-ast-add_new_block_annotation| replace:: Makes adapter to the `AstBlockAnnotation` and adds it to the current module.

.. |function-ast-add_new_function_annotation| replace:: Makes adapter to the `AstFunctionAnnotation` and adds it to the current module.

.. |function-ast-add_new_contract_annotation| replace:: Makes adapter to the `AstContractAnnotation` and adds it to the current module.

.. |function-ast-add_new_structure_annotation| replace:: Makes adapter to the `AstStructureAnnotation` and adds it to the current module.

.. |function-ast-add_new_enumeration_annotation| replace:: Makes adapter to the `AstEnumerationAnnotation` and adds it to the current module.

.. |function-ast-add_new_variant_macro| replace:: Makes adapter to the `AstVariantMacro` and adds it to the current module.

.. |function-ast-add_new_reader_macro| replace:: Makes adapter to the `AstReaderMacro` and adds it to the current module.

.. |function-ast-add_new_comment_reader| replace:: Makes adapter to the `AstCommentReader` and adds it to the current module.

.. |function-ast-add_new_call_macro| replace:: Makes adapter to the `AstCallMacro` and adds it to the current module.

.. |function-ast-add_new_typeinfo_macro| replace:: Makes adapter to the `AstTypeInfoMacro` and adds it to the current module.

.. |function-ast-add_new_infer_macro| replace:: Makes adapter to the `AstPassMacro` and adds it to the current module `infer` pass.

.. |function-ast-add_new_dirty_infer_macro| replace:: Makes adapter to the `AstPassMacro` and adds it to the current module `dirty infer` pass.

.. |function-ast-add_new_lint_macro| replace:: Makes adapter to the `AstPassMacro` and adds it to the current module `lint` pass.

.. |function-ast-add_new_global_lint_macro| replace:: Makes adapter to the `AstPassMacro` and adds it to the current module `global lint` pass.

.. |function-ast-add_new_optimization_macro| replace:: Makes adapter to the `AstPassMacro` and adds it to the current module `optimization` pass.

.. |function-ast-find_module| replace:: Finds `Module` in the `Program`.

.. |function-ast-find_compiling_module| replace:: Finds `Module` in the currently compiling `Program`.

.. |structure_annotation-ast-ModuleLibrary| replace:: Object which holds list of `Module` and provides access to them.

.. |structure_annotation-ast-Expression| replace:: Any expression (base class).

.. |structure_annotation-ast-TypeDecl| replace:: Any type declaration.

.. |structure_annotation-ast-Structure| replace:: Structure declaration.

.. |structure_annotation-ast-FieldDeclaration| replace:: Structure field declaration.

.. |structure_annotation-ast-EnumEntry| replace:: Entry in the enumeration.

.. |structure_annotation-ast-Enumeration| replace:: Enumeration declaration.

.. |structure_annotation-ast-Function| replace:: Function declaration.

.. |structure_annotation-ast-InferHistory| replace:: Generic function infer history.
    Contains stack on where the function was first instantiated from (`Function` and `LineInfo` pairs).

.. |structure_annotation-ast-Variable| replace:: Variable declaration.

.. |structure_annotation-ast-AstContext| replace:: Lexical context for the particular expression.
    Contains current function, loops, blocks, scopes, and with sections.

.. |structure_annotation-ast-ExprBlock| replace:: Any block expression, including regular blocks and all types of closures.
    For the closures block arguments are defined. Finally section is defined, if exists.

.. |structure_annotation-ast-ExprLet| replace:: Local variable declaration (`let v = expr;`).

.. |structure_annotation-ast-ExprStringBuilder| replace:: String builder expression ("blah{blah1}blah2").

.. |structure_annotation-ast-MakeFieldDecl| replace:: Part of `ExprMakeStruct`, declares single field (`a = expr` or `a <- expr` etc)

.. |any_annotation-ast-MakeStruct| replace:: Part of `ExprMakeStruct`, happens to be vector of `MakeFieldDecl`.

.. |structure_annotation-ast-ExprNamedCall| replace:: Named call (`call([argname1=expr1, argname2=expr2])`).

.. |structure_annotation-ast-ExprLooksLikeCall| replace:: Anything which looks like call (`call(expr1,expr2)`).

.. |structure_annotation-ast-ExprCallFunc| replace:: Actual function call (`func(expr1,...)`).

.. |structure_annotation-ast-ExprNew| replace:: New expression (`new Foo`, `new Bar(expr1..)`, but **NOT** `new [[Foo ...]]`)

.. |structure_annotation-ast-ExprCall| replace:: Anything which looks like call (`call(expr1,expr2)`).

.. |structure_annotation-ast-ExprPtr2Ref| replace:: Pointer dereference (`*expr` or `deref(expr)`).

.. |structure_annotation-ast-ExprNullCoalescing| replace:: Null coalescing (`expr1 ?? expr2`).

.. |structure_annotation-ast-ExprAt| replace:: Index lookup (`expr[expr1]`).

.. |structure_annotation-ast-ExprSafeAt| replace:: Safe index lookup (`expr?[expr1]`).

.. |structure_annotation-ast-ExprIs| replace:: Is expression for variants and such (`expr is Foo`).

.. |structure_annotation-ast-ExprOp| replace:: Compilation time only base class for any operator.

.. |structure_annotation-ast-ExprOp2| replace:: Two operand operator (`expr1 + expr2`)

.. |structure_annotation-ast-ExprOp3| replace:: Three operand operator (`cond ? expr1 : expr2`)

.. |structure_annotation-ast-ExprCopy| replace:: Copy operator (`expr1 = expr2`)

.. |structure_annotation-ast-ExprMove| replace:: Move operator (`expr1 <- expr2`)

.. |structure_annotation-ast-ExprClone| replace:: Clone operator (`expr1 := expr2`)

.. |structure_annotation-ast-ExprWith| replace:: With section (`with expr {your; block; here}`).

.. |structure_annotation-ast-ExprAssume| replace:: Assume expression (`assume name = expr`).

.. |structure_annotation-ast-ExprWhile| replace:: While loop (`while expr {your; block; here;}`)

.. |structure_annotation-ast-ExprTryCatch| replace:: Try-recover expression (`try {your; block; here;} recover {your; recover; here;}`)

.. |structure_annotation-ast-ExprIfThenElse| replace:: If-then-else expression (`if expr1 {your; block; here;} else {your; block; here;}`) including `static_if`'s.

.. |structure_annotation-ast-ExprFor| replace:: For loop (`for expr1 in expr2 {your; block; here;}`)

.. |structure_annotation-ast-ExprMakeLocal| replace:: Any make expression (`ExprMakeBlock`, `ExprMakeTuple`, `ExprMakeVariant`, `ExprMakeStruct`)

.. |structure_annotation-ast-ExprMakeStruct| replace:: Make structure expression (`[[YourStruct v1=expr1elem1, v2=expr2elem1, ...; v1=expr1elem2, ...  ]]`)

.. |structure_annotation-ast-ExprMakeVariant| replace:: Make variant expression (`[YourVariant variantName=expr1]`)

.. |structure_annotation-ast-ExprMakeArray| replace:: Make array expression (`[[auto 1;2;3]]` or `[{auto "foo";"bar"}]` for static and dynamic arrays accordingly).

.. |structure_annotation-ast-ExprMakeTuple| replace:: Make tuple expression (`[[auto f1,f2,f3]]`)

.. |structure_annotation-ast-ExprArrayComprehension| replace:: Array comprehension (`[{for x in 0..3; x}]`, `[[for y in range(100); x*2; where x!=13]]` for arrays or generators accordingly).

.. |structure_annotation-ast-TypeInfoMacro| replace:: Compilation time only structure which holds live information about typeinfo expression for the specific macro.

.. |structure_annotation-ast-ExprTypeInfo| replace:: typeinfo() expression (`typeinfo(dim a)`, `typeinfo(is_ref_type type<int&>)`)

.. |structure_annotation-ast-ExprTypeDecl| replace:: typedecl() expression (`typedecl(1+2)`)

.. |structure_annotation-ast-ExprLabel| replace:: Label (`label 13:`)

.. |structure_annotation-ast-ExprGoto| replace:: Goto expression (`goto label 13`, `goto x`)

.. |structure_annotation-ast-ExprRef2Value| replace:: Compilation time only structure which holds reference to value conversion for the value types, i.e. goes from int& to int and such.

.. |structure_annotation-ast-ExprRef2Ptr| replace:: Addr expresion (`addr(expr)`)

.. |structure_annotation-ast-ExprAddr| replace:: Function address (`@@foobarfunc` or `@@foobarfunc<(int;int):bool>`)

.. |structure_annotation-ast-ExprAssert| replace:: Assert expression (`assert(x<13)` or `assert(x<13, "x is too big")`)

.. |structure_annotation-ast-ExprQuote| replace:: Compilation time expression which holds its subexpressions but does not infer them (`quote() <| x+5`)

.. |structure_annotation-ast-ExprStaticAssert| replace:: Static assert expression (`static_assert(x<13)` or `static_assert(x<13, "x is too big")`)

.. |structure_annotation-ast-ExprDebug| replace:: Debug expression (`debug(x)` or `debug(x,"x=")`)

.. |structure_annotation-ast-ExprInvoke| replace:: Invoke expression (`invoke(fn)` or `invoke(lamb, arg1, arg2, ...)`)

.. |structure_annotation-ast-ExprErase| replace:: Erase expression (`erase(tab,key)`)

.. |structure_annotation-ast-ExprFind| replace:: Find expression (`find(tab,key) <| { your; block; here; }`)

.. |structure_annotation-ast-ExprKeyExists| replace:: Key exists expression (`key_exists(tab,key)`)

.. |structure_annotation-ast-ExprAscend| replace:: New expression for ExprMakeLocal (`new [[Foo fld=val,...]]` or `new [[Foo() fld=...]]`, but **NOT** `new Foo()`)

.. |structure_annotation-ast-ExprCast| replace:: Any cast expression (`cast<int> a`, `upcast<Foo> b` or `reinterpret<Bar?> c`)

.. |structure_annotation-ast-ExprDelete| replace:: Delete expression (`delete blah`)

.. |structure_annotation-ast-ExprVar| replace:: Variable access (`foo`)

.. |structure_annotation-ast-ExprSwizzle| replace:: Vector swizzle operatrion (`vec.xxy` or `vec.y`)

.. |structure_annotation-ast-ExprField| replace:: Field lookup (`foo.bar`)

.. |structure_annotation-ast-ExprSafeField| replace:: Safe field lookup (`foo?.bar`)

.. |structure_annotation-ast-ExprIsVariant| replace:: Is expression (`foo is bar`)

.. |structure_annotation-ast-ExprAsVariant| replace:: As expression (`foo as bar`)

.. |structure_annotation-ast-ExprSafeAsVariant| replace:: Safe as expression (`foo? as bar`)

.. |structure_annotation-ast-ExprOp1| replace:: Single operator expression (`+a` or `-a` or `!a` or `~a`)

.. |structure_annotation-ast-ExprReturn| replace:: Return expression (`return` or `return foo`, or `return <- foo`)

.. |structure_annotation-ast-ExprYield| replace:: Yield expression (`yield foo` or `yeild <- bar`)

.. |structure_annotation-ast-ExprBreak| replace:: Break expression (`break`)

.. |structure_annotation-ast-ExprContinue| replace:: Continue expression (`continue`)

.. |structure_annotation-ast-ExprConst| replace:: Compilation time constant expression base class

.. |structure_annotation-ast-ExprFakeContext| replace:: Compilation time only fake context expression. Will simulate as current evaluation `Context`.

.. |structure_annotation-ast-ExprFakeLineInfo| replace:: Compilation time only fake lineinfo expression. Will simulate as current file and line `LineInfo`.

.. |structure_annotation-ast-ExprConstPtr| replace:: Null (`null`). Technically can be any other pointer, but it is used for nullptr.

.. |structure_annotation-ast-ExprConstInt8| replace:: Holds int8 constant.

.. |structure_annotation-ast-ExprConstInt16| replace:: Holds int16 constant.

.. |structure_annotation-ast-ExprConstInt64| replace:: Holds int64 constant.

.. |structure_annotation-ast-ExprConstInt| replace:: Holds int constant.

.. |structure_annotation-ast-ExprConstInt2| replace:: Holds int2 constant.

.. |structure_annotation-ast-ExprConstInt3| replace:: Holds int3 constant.

.. |structure_annotation-ast-ExprConstInt4| replace:: Holds int4 constant.

.. |structure_annotation-ast-ExprConstUInt8| replace:: Holds uint8 constant.

.. |structure_annotation-ast-ExprConstUInt16| replace:: Holds uint16 constant.

.. |structure_annotation-ast-ExprConstUInt64| replace:: Holds uint64 constant.

.. |structure_annotation-ast-ExprConstUInt| replace:: Holds uint constant.

.. |structure_annotation-ast-ExprConstUInt2| replace:: Holds uint2 constant.

.. |structure_annotation-ast-ExprConstUInt3| replace:: Holds uint3 constant.

.. |structure_annotation-ast-ExprConstUInt4| replace:: Holds uint4 constant.

.. |structure_annotation-ast-ExprConstRange| replace:: Holds range constant.

.. |structure_annotation-ast-ExprConstURange| replace:: Holds urange constant.

.. |structure_annotation-ast-ExprConstRange64| replace:: Holds range64 constant.

.. |structure_annotation-ast-ExprConstURange64| replace:: Holds urange64 constant.

.. |structure_annotation-ast-ExprConstFloat| replace:: Holds float constant.

.. |structure_annotation-ast-ExprConstFloat2| replace:: Holds float2 constant.

.. |structure_annotation-ast-ExprConstFloat3| replace:: Holds float3 constant.

.. |structure_annotation-ast-ExprConstFloat4| replace:: Holds float4 constant.

.. |structure_annotation-ast-ExprConstDouble| replace:: Holds double constant.

.. |structure_annotation-ast-ExprConstBool| replace:: Holds bool constant.

.. |structure_annotation-ast-CaptureEntry| replace:: Single entry in lambda capture.

.. |structure_annotation-ast-ExprMakeBlock| replace:: Any closure. Holds block as well as capture information in `CaptureEntry`.

.. |structure_annotation-ast-ExprMakeGenerator| replace:: Generator closure (`generator<int>` or `generator<Foo&>`)

.. |structure_annotation-ast-ExprMemZero| replace:: Memzero (`memzero(expr)`)

.. |structure_annotation-ast-ExprConstEnumeration| replace:: Holds enumeration constant, both type and entry (`Foo bar`).

.. |structure_annotation-ast-ExprConstBitfield| replace:: Holds bitfield constant (`Foo bar`).

.. |structure_annotation-ast-ExprConstString| replace:: Holds string constant.

.. |structure_annotation-ast-ExprUnsafe| replace:: Unsafe expression (`unsafe(addr(x))`)

.. |structure_annotation-ast-VisitorAdapter| replace:: Adapter for the `AstVisitor` interface.

.. |structure_annotation-ast-FunctionAnnotation| replace:: Adapter for the `AstFunctionAnnotation`.

.. |structure_annotation-ast-StructureAnnotation| replace:: Adapter for the `AstStructureAnnotation`.

.. |structure_annotation-ast-EnumerationAnnotation| replace:: Adapater for the `AstEnumearationAnnotation`.

.. |structure_annotation-ast-PassMacro| replace:: Adapter for the `AstPassMacro`.

.. |structure_annotation-ast-ReaderMacro| replace:: Adapter for the `AstReaderMacro`.

.. |structure_annotation-ast-CommentReader| replace:: Adapter for the `AstCommentReader`.

.. |structure_annotation-ast-CallMacro| replace:: Adapter for the `AstCallMacro`.

.. |structure_annotation-ast-VariantMacro| replace:: Adapter for the `AstVariantMacro`.

.. |structure_annotation-ast-ExprReader| replace:: Compilation time only expression which holds temporary information for the `AstReaderMacro`.

.. |structure_annotation-ast-ExprCallMacro| replace:: Compilation time only expression which holds temporary infromation for the `AstCallMacro`.

.. |structure_annotation-ast-ExprSetInsert| replace:: insert(tab, at) for the table<keyType; void> aka table<keyType>

.. |function_annotation-ast-quote| replace:: Returns ast expression tree of the input, without evaluating or infering it.
    This is useful for macros which generate code as a shortcut for generating boilerplate code.

.. |typeinfo_macro-ast-ast_typedecl| replace:: Returns TypeDeclPtr of the type specified via type<> or subexpression type, for example typeinfo(ast_typedecl type<int?>)

.. |typeinfo_macro-ast-ast_function| replace:: Returns FunctionPtr to the function specified by subexrepssion, for example typeinfo(ast_function @@foo)

.. |function-ast-add_block_annotation| replace:: Adds annotation declaration to the block.

.. |function-ast-add_alias| replace:: Adds type alias to the specified module.

.. |function-ast-remove_structure| replace:: Removes structure declaration from the specified module.

.. |function-ast-find_unique_structure| replace:: Find structure in the program with the specified name. If its unique - return it, otherwise null.

.. |typedef-ast-ForLoopMacroPtr| replace:: Smart pointer to 'ForLoopMacro'.

.. |structure_annotation-ast-ForLoopMacro| replace:: Adapter for the 'AstForLoopMacro'.

.. |typedef-ast-CaptureMacroPtr| replace:: Smart pointer to 'CaptureMacro'.

.. |structure_annotation-ast-ExprSetInsert| replace:: Set insert expression, i.e. tab |> insert(key).

.. |structure_annotation-ast-ExprTag| replace:: Compilation time only tag expression, used for reification. For example $c(....).

.. |structure_annotation-ast-CaptureMacro| replace:: Adapter for the `AstCaptureMacro`.

.. |class-ast-AstSimulateMacro| replace:: Macro which is attached to the context simulation.

.. |method-ast-AstSimulateMacro.preSimulate| replace:: This callback occurs before the context simulation.

.. |method-ast-AstSimulateMacro.simulate| replace:: This callback occurs after the context simulation.

.. |structure_annotation-ast-SimulateMacro| replace:: Adapter for the `AstSimulateMacro`.

.. |function-ast-visit_finally| replace:: Calls visit on the `finally` section of the block.

.. |function-ast-make_simulate_macro| replace:: Creates adapter for the 'AstSimulateMacro' interface.

.. |function-ast-add_simulate_macro| replace:: Adds `AstSimulateMacro` to the specific module.

.. |function-ast-add_new_simulate_macro| replace:: Makes adapter to the `AstSimulateMacro` and adds it to the current module.

.. |function-ast-make_interop_node| replace:: Makes interop node for the jit function. Those are used for the addInterop calls, or the StringBuilder.

.. |function-ast-find_structure_field| replace:: Returns `FieldDeclaration` for the specific field of the structure type, or `null` if not found.

.. |function-ast-for_each_module| replace:: Iterates through each module in the program.

.. |function-ast-is_terminator_expression| replace:: Returns `true` if the expression ends with a terminator expression, i.e. `return`.

.. |function-ast-is_terminator_or_break_expression| replace:: Returns `true` if the expression ends with a terminator expression `return` or a `break` expression.

.. |function-ast-get_use_global_variables| replace:: Provides invoked block with the list of all global variables, used by a function.

.. |function-ast-get_use_functions| replace:: Provides invoked block with the list of all functions, used by a function.

.. |function-ast-get_builtin_function_address| replace:: Returns pointer to a builtin function.

.. |function-ast-make_type_info_structure| replace:: Returns new `TypeInfo` corresponding to the specific type.

.. |function-ast-force_generated| replace:: Forces `generated` flag on subexrepssion.

.. |function-ast-get_expression_annotation| replace:: Get 'Annotation' for the 'ast::Expression' and its inherited types.

.. |function-ast-to_compilation_log| replace:: Writes to compilation log from macro during compilation.

.. |function-ast-add_module_option| replace:: Add module-specific option, which is accessible via "options" keyword.

.. |function-ast-get_handled_type_field_offset| replace:: Returns offset of the field in the ManagedStructure handled type.

.. |function-ast-get_underlying_value_type| replace:: Returns daScript type which is aliased with ManagedValue handled type.



