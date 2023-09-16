.. _macros:

======
Macros
======

In daScript, macros are the machinery that allow direct manipulation of the syntax tree.

Macros are exposed via the :ref:`daslib/ast <stdlib_ast>` module and :ref:`daslib/ast_boost <stdlib_ast_boost>` helper module.

Macros are evaluated at compilation time during different compilation passes.
Macros assigned to a specific module are evaluated as part of the module every time that module is included.

------------------
Compilation passes
------------------

The daScript compiler performs compilation passes in the following order for each module (see :ref:`Modules <modules>`):

#. Parser transforms das program to AST

    #. If there are any parsing errors, compilation stops

#. `apply` is called for every function or structure

    #. If there are any errors, compilation stops

#. Infer pass repeats itself until no new transformations are reported

    #. Built-in infer pass happens

        #. `transform` macros are called for every function or expression

    #. Macro passes happen

#. If there are still any errors left, compilation stops

#. `finish` is called for all functions and structure macros

#. Lint pass happens

    #. If there are any errors, compilation stops

#. Optimization pass repeats itself until no new transformations are reported

    #. Built-in optimization pass happens

    #. Macro optimization pass happens

#. If there are any errors during optimization passes, compilation stops

#. If the module contains any macros, simulation happens

    #. If there are any simulation errors, compilation stops

    #. Module macro functions (annotated with `_macro`) are invoked

        #. If there are any errors, compilation stops

Modules are compiled in `require` order.

---------------
Invoking macros
---------------

The ``[_macro]`` annotation is used to specify functions that should be evaluated at compilation time .
Consider the following example from :ref:`daslib/ast_boost <stdlib_ast_boost>`::

    [_macro,private]
    def setup
        if is_compiling_macros_in_module("ast_boost")
            add_new_function_annotation("macro", new MacroMacro())

The `setup` function is evaluated after the compilation of each module, which includes ast_boost.
The ``is_compiling_macros_in_module`` function returns true if the currently compiled module name matches the argument.
In this particular example, the function annotation ``macro`` would only be added once: when the module `ast_boost` is compiled.

Macros are invoked in the following fashion:

#. Class is derived from the appropriate base macro class
#. Adapter is created
#. Adapter is registered with the module

For example, this is how this lifetime cycle is implemented for the reader macro::

    def add_new_reader_macro ( name:string; someClassPtr )
        var ann <- make_reader_macro(name, someClassPtr)
        this_module() |> add_reader_macro(ann)
        unsafe
            delete ann

---------------------
AstFunctionAnnotation
---------------------

The ``AstFunctionAnnotation`` macro allows you to manipulate calls to specific functions as well as their function bodies.
Annotations can be added to regular or generic functions.

``add_new_function_annotation`` adds a function annotation to a module.
There is additionally the ``[function_macro]`` annotation which accomplishes the same thing.

``AstFunctionAnnotation`` allows several different manipulations::

    class AstFunctionAnnotation
        def abstract transform ( var call : smart_ptr<ExprCallFunc>; var errors : das_string ) : ExpressionPtr
        def abstract verifyCall ( var call : smart_ptr<ExprCallFunc>; args,progArgs:AnnotationArgumentList; var errors : das_string ) : bool
        def abstract apply ( var func:FunctionPtr; var group:ModuleGroup; args:AnnotationArgumentList; var errors : das_string ) : bool
        def abstract finish ( var func:FunctionPtr; var group:ModuleGroup; args,progArgs:AnnotationArgumentList; var errors : das_string ) : bool
        def abstract patch ( var func:FunctionPtr; var group:ModuleGroup; args,progArgs:AnnotationArgumentList; var errors : das_string; var astChanged:bool& ) : bool
        def abstract fixup ( var func:FunctionPtr; var group:ModuleGroup; args,progArgs:AnnotationArgumentList; var errors : das_string ) : bool
        def abstract lint ( var func:FunctionPtr; var group:ModuleGroup; args,progArgs:AnnotationArgumentList; var errors : das_string ) : bool
        def abstract complete ( var func:FunctionPtr; var ctx:smart_ptr<Context> ) : void
        def abstract isCompatible ( var func:FunctionPtr; var types:VectorTypeDeclPtr; decl:AnnotationDeclaration; var errors:das_string ) : bool
        def abstract isSpecialized : bool
        def abstract appendToMangledName ( func:FunctionPtr; decl:AnnotationDeclaration; var mangledName:das_string ) : void

``transform`` lets you change calls to the function and is applied at the infer pass.
Transform is the best way to replace or modify function calls with other semantics.

``verifyCall`` is called durng the `lint` phase on each call to the function and is used to check if the call is valid.

``apply`` is applied to the function itself before the infer pass.
Apply is typically where global function body modifications or instancing occurs.

``finish`` is applied to the function itself after the infer pass.
It's only called on non-generic functions or instances of the generic functions.
``finish`` is typically used to register functions, notify C++ code, etc.
After this, the function is fully defined and inferred, and can no longer be modified.

``patch`` is called after the infer pass. If patch sets astChanged to true, the infer pass will be repeated.

``fixup`` is called after the infer pass. It's used to fixup the function's body.

``lint`` is called during the `lint` phase on the function itself and is used to verify that the function is valid.

``complete`` is called during the `simulate` portion of context creation. At this point Context is available.

``isSpecialized`` must return true if the particular function matching is governed by contracts.
In that case, ``isCompatible`` is called, and the result taken into account.

``isCompatible`` returns true if a specialized function is compatible with the given arguments.
If a function is not compatible, the errors field must be specified.

``appendToMangledName`` is called to append a mangled name to the function.
That way multiple functions with the same type signature can exist and be differentiated between.

Lets review the following example from `ast_boost` of how the ``macro`` annotation is implemented::

    class MacroMacro : AstFunctionAnnotation
        def override apply ( var func:FunctionPtr; var group:ModuleGroup; args:AnnotationArgumentList; var errors : das_string ) : bool
            compiling_program().flags |= ProgramFlags needMacroModule
            func.flags |= FunctionFlags init
            var blk <- new [[ExprBlock() at=func.at]]
            var ifm <- new [[ExprCall() at=func.at, name:="is_compiling_macros"]]
            var ife <- new [[ExprIfThenElse() at=func.at, cond<-ifm, if_true<-func.body]]
            push(blk.list,ife)
            func.body <- blk
            return true

During the `apply` pass the function body is appended with the ``if is_compiling_macros()`` closure.
Additionally, the ``init`` flag is set, which is equivalent to a ``_macro`` annotation.
Functions annotated with ``[macro]`` are evaluated during module compilation.

------------------
AstBlockAnnotation
------------------

``AstBlockAnnotation`` is used to manipulate block expressions (blocks, lambdas, local functions)::

    class AstBlockAnnotation
        def abstract apply ( var blk:smart_ptr<ExprBlock>; var group:ModuleGroup; args:AnnotationArgumentList; var errors : das_string ) : bool
        def abstract finish ( var blk:smart_ptr<ExprBlock>; var group:ModuleGroup; args,progArgs:AnnotationArgumentList; var errors : das_string ) : bool

``add_new_block_annotation`` adds a function annotation to a module.
There is additionally the ``[block_macro]`` annotation which accomplishes the same thing.

``apply`` is called for every block expression before the infer pass.

``finish`` is called for every block expression after infer pass.

----------------------
AstStructureAnnotation
----------------------

The ``AstStructureAnnotation`` macro lets you manipulate structure or class definitions via annotation::

    class AstStructureAnnotation
        def abstract apply ( var st:StructurePtr; var group:ModuleGroup; args:AnnotationArgumentList; var errors : das_string ) : bool
        def abstract finish ( var st:StructurePtr; var group:ModuleGroup; args:AnnotationArgumentList; var errors : das_string ) : bool
        def abstract patch ( var st:StructurePtr; var group:ModuleGroup; args:AnnotationArgumentList; var errors : das_string; var astChanged:bool& ) : bool
        def abstract complete ( var st:StructurePtr; var ctx:smart_ptr<Context> ) : void

``add_new_structure_annotation`` adds a function annotation to a module.
There is additionally the ``[structure_macro]`` annotation which accomplishes the same thing.

``AstStructureAnnotation`` allows 2 different manipulations::

    class AstStructureAnnotation
        def abstract apply ( var st:StructurePtr; var group:ModuleGroup; args:AnnotationArgumentList; var errors : das_string ) : bool
        def abstract finish ( var st:StructurePtr; var group:ModuleGroup; args:AnnotationArgumentList; var errors : das_string ) : bool

``apply`` is invoked before the infer pass. It is the best time to modify the structure, generate some code, etc.

``finish`` is invoked after the successful infer pass. Its typically used to register structures, perform RTTI operations, etc.
After this, the structure is fully inferred and defined and can no longer be modified afterwards.

``patch`` is invoked after the infer pass. If patch sets astChanged to true, the infer pass will be repeated.

``complete`` is invoked during the `simulate` portion of context creation. At this point Context is available.

An example of such annotation is `SetupAnyAnnotation` from :ref:`daslib/ast_boost <stdlib_ast_boost>`.

------------------------
AstEnumerationAnnotation
------------------------

The ``AstStructureAnnotation`` macro lets you manipulate enumerations via annotation::

    class AstEnumerationAnnotation
        def abstract apply ( var st:EnumerationPtr; var group:ModuleGroup; args:AnnotationArgumentList; var errors : das_string ) : bool

``add_new_enumeration_annotation`` adds a function annotation to a module.
There is additionally the ``[enumeration_macro]`` annotation which accomplishes the same thing.

``apply`` is invoked before the infer pass. It is the best time to modify the enumeration, generate some code, etc.

---------------
AstVariantMacro
---------------

``AstVariantMacro`` is specialized in transforming ``is``, ``as``, and ``?as`` expressions.

``add_new_variant_macro`` adds a variant macro to a module.
There is additionally the ``[variant_macro]`` annotation which accomplishes the same thing.

Each of the 3 transformations are covered in the appropriate abstract function::

    class AstVariantMacro
        def abstract visitExprIsVariant     ( prog:ProgramPtr; mod:Module?; expr:smart_ptr<ExprIsVariant> ) : ExpressionPtr
        def abstract visitExprAsVariant     ( prog:ProgramPtr; mod:Module?; expr:smart_ptr<ExprAsVariant> ) : ExpressionPtr
        def abstract visitExprSafeAsVariant ( prog:ProgramPtr; mod:Module?; expr:smart_ptr<ExprSafeAsVariant> ) : ExpressionPtr

Let's review the following example from :ref:`daslib/ast_boost <stdlib_ast_boost>`::

    // replacing ExprIsVariant(value,name) => ExprOp2('==",value.__rtti,"name")
    // if value is ast::Expr*
    class BetterRttiVisitor : AstVariantMacro
        def override visitExprIsVariant(prog:ProgramPtr; mod:Module?;expr:smart_ptr<ExprIsVariant>) : ExpressionPtr
            if isExpression(expr.value._type)
                var vdr <- new [[ExprField() at=expr.at, name:="__rtti", value <- clone_expression(expr.value)]]
                var cna <- new [[ExprConstString() at=expr.at, value:=expr.name]]
                var veq <- new [[ExprOp2() at=expr.at, op:="==", left<-vdr, right<-cna]]
                return veq
            return [[ExpressionPtr]]

    // note the following ussage
    class GetHintFnMacro : AstFunctionAnnotation
        def override transform ( var call : smart_ptr<ExprCall>; var errors : das_string ) : ExpressionPtr
            if call.arguments[1] is ExprConstString     // HERE EXPRESSION WILL BE REPLACED
                ...

Here, the macro takes advantage of the ExprIsVariant syntax.
It replaces the ``expr is TYPENAME`` expression with an ``expr.__rtti = "TYPENAME"`` expression.
The ``isExpression`` function ensures that `expr` is from the `ast::Expr*` family, i.e. part of the daScript syntax tree.

--------------
AstReaderMacro
--------------

``AstReaderMacro`` allows embedding a completely different syntax inside daScript code.

``add_new_reader_macro`` adds a reader macro to a module.
There is additionally the ``[reader_macro]`` annotation, which essentially automates the same thing.

Reader macros accept characters, collect them if necessary, and return an `ast::Expression`::

    class AstReaderMacro
        def abstract accept ( prog:ProgramPtr; mod:Module?; expr:ExprReader?; ch:int; info:LineInfo ) : bool
        def abstract visit ( prog:ProgramPtr; mod:Module?; expr:smart_ptr<ExprReader> ) : ExpressionPtr

Reader macros are invoked via the ``% READER_MACRO_NAME ~ character_sequence`` syntax.
The ``accept`` function notifies the correct terminator of the character sequence::

    var x = %arr~\{\}\w\x\y\n%% // invoking reader macro arr, %% is a terminator

Consider the implementation for the example above::

    [reader_macro(name="arr")]
    class ArrayReader : AstReaderMacro
        def override accept ( prog:ProgramPtr; mod:Module?; var expr:ExprReader?; ch:int; info:LineInfo ) : bool
            append(expr.sequence,ch)
            if ends_with(expr.sequence,"%%")
                let len = length(expr.sequence)
                resize(expr.sequence,len-2)
                return false
            else
                return true
        def override visit ( prog:ProgramPtr; mod:Module?; expr:smart_ptr<ExprReader> ) : ExpressionPtr
            let seqStr = string(expr.sequence)
            var arrT <- new [[TypeDecl() baseType=Type tInt]]
            push(arrT.dim,length(seqStr))
            var mkArr <- new [[ExprMakeArray() at = expr.at, makeType <- arrT]]
            for x in seqStr
                var mkC <- new [[ExprConstInt() at=expr.at, value=x]]
                push(mkArr.values,mkC)
            return mkArr

The ``accept`` function macro collects symbols in the sequence.
Once the sequence ends with the terminator sequence %%, ``accept`` returns false to indicate the end of the sequence.

In ``visit``, the collected sequence is converted into a make array ``[[int ch1; ch2; ..]]`` expression.

More complex examples include the JsonReader macro in :ref:`daslib/json_boost <stdlib_json_boost>` or RegexReader in :ref:`daslib/regex_boost <stdlib_regex_boost>`.

------------
AstCallMacro
------------

``AstCallMacro`` operates on expressions which have function call syntax or something similar.
It occurs during the infer pass.

``add_new_call_macro`` adds a call macro to a module.
The ``[call_macro]`` annotation automates the same thing::

        class AstCallMacro
            def abstract preVisit ( prog:ProgramPtr; mod:Module?; expr:smart_ptr<ExprCallMacro> ) : void
            def abstract visit ( prog:ProgramPtr; mod:Module?; expr:smart_ptr<ExprCallMacro> ) : ExpressionPtr
            def abstract canVisitArguments ( expr:smart_ptr<ExprCallMacro> ) : bool

``apply`` from :ref:`daslib/apply <stdlib_apply>` is an example of such a macro::

    [call_macro(name="apply")]  // apply(value, block)
    class ApplyMacro : AstCallMacro
        def override visit ( prog:ProgramPtr; mod:Module?; var expr:smart_ptr<ExprCallMacro> ) : ExpressionPtr
            ...

Note how the name is provided in the ``[call_macro]`` annotation.

``preVisit`` is called before the arguments are visited.

``visit`` is called after the arguments are visited.

``canVisitArguments`` is called to determine if the macro can visit the arguments.

------------
AstPassMacro
------------

``AstPassMacro`` is one macro to rule them all. It gets entire module as an input,
and can be invoked at numerous passes::

    class AstPassMacro
        def abstract apply ( prog:ProgramPtr; mod:Module? ) : bool

``make_pass_macro`` registers a class as a pass macro.

``add_new_infer_macro`` adds a pass macro to the infer pass. The ``[infer]`` annotation accomplishes the same thing.

``add_new_dirty_infer_macro`` adds a pass macro to the `dirty` section of infer pass. The ``[dirty_infer]`` annotation accomplishes the same thing.

Typically, such macros create an ``AstVisitor`` which performs the necessary transformations.

----------------
AstTypeInfoMacro
----------------

``AstTypeInfoMacro`` is designed to implement custom type information inside a typeinfo expression::

    class AstTypeInfoMacro
        def abstract getAstChange ( expr:smart_ptr<ExprTypeInfo>; var errors:das_string ) : ExpressionPtr
        def abstract getAstType ( var lib:ModuleLibrary; expr:smart_ptr<ExprTypeInfo>; var errors:das_string ) : TypeDeclPtr

``add_new_typeinfo_macro`` adds a reader macro to a module.
There is additionally the ``[typeinfo_macro]`` annotation, which essentially automates the same thing.

``getAstChange`` returns a newly generated ast for the typeinfo expression.
Alternatively, it returns null if no changes are required, or if there is an error.
In case of error, the errors string must be filled.

``getAstType`` returns the type of the new typeinfo expression.

---------------
AstForLoopMacro
---------------

``AstForLoopMacro`` is designed to implement custom processing of for loop expressions::

    class AstForLoopMacro
        def abstract visitExprFor ( prog:ProgramPtr; mod:Module?; expr:smart_ptr<ExprFor> ) : ExpressionPtr

``add_new_for_loop_macro`` adds a reader macro to a module.
There is additionally the ``[for_loop_macro]`` annotation, which essentially automates the same thing.

``visitExprFor`` is similar to that of `AstVisitor`. It returns a new expression, or null if no changes are required.

---------------
AstCaptureMacro
---------------

``AstCaptureMacro`` is designed to implement custom capturing and finalization of lambda expressions::

    class AstCaptureMacro
        def abstract captureExpression ( prog:Program?; mod:Module?; expr:ExpressionPtr; etype:TypeDeclPtr ) : ExpressionPtr
        def abstract captureFunction ( prog:Program?; mod:Module?; var lcs:Structure?; var fun:FunctionPtr ) : void

``add_new_capture_macro`` adds a reader macro to a module.
There is additionally the ``[capture_macro]`` annotation, which essentially automates the same thing.

``captureExpression`` is called when an expression is captured. It returns a new expression, or null if no changes are required.

``captureFunction`` is called when a function is captured. This is where custom finalization can be added to the `final` section of the function body.

----------------
AstCommentReader
----------------

``AstCommentReader`` is designed to implement custom processing of comment expressions::

    class AstCommentReader
        def abstract open ( prog:ProgramPtr; mod:Module?; cpp:bool; info:LineInfo ) : void
        def abstract accept ( prog:ProgramPtr; mod:Module?; ch:int; info:LineInfo ) : void
        def abstract close ( prog:ProgramPtr; mod:Module?; info:LineInfo ) : void
        def abstract beforeStructure ( prog:ProgramPtr; mod:Module?; info:LineInfo ) : void
        def abstract afterStructure ( st:StructurePtr; prog:ProgramPtr; mod:Module?; info:LineInfo ) : void
        def abstract beforeStructureFields ( prog:ProgramPtr; mod:Module?; info:LineInfo ) : void
        def abstract afterStructureField ( name:string; prog:ProgramPtr; mod:Module?; info:LineInfo ) : void
        def abstract afterStructureFields ( prog:ProgramPtr; mod:Module?; info:LineInfo ) : void
        def abstract beforeFunction ( prog:ProgramPtr; mod:Module?; info:LineInfo ) : void
        def abstract afterFunction ( fn:FunctionPtr; prog:ProgramPtr; mod:Module?; info:LineInfo ) : void
        def abstract beforeGlobalVariables ( prog:ProgramPtr; mod:Module?; info:LineInfo ) : void
        def abstract afterGlobalVariable ( name:string; prog:ProgramPtr; mod:Module?; info:LineInfo ) : void
        def abstract afterGlobalVariables ( prog:ProgramPtr; mod:Module?; info:LineInfo ) : void
        def abstract beforeVariant ( prog:ProgramPtr; mod:Module?; info:LineInfo ) : void
        def abstract afterVariant ( name:string; prog:ProgramPtr; mod:Module?; info:LineInfo ) : void
        def abstract beforeEnumeration ( prog:ProgramPtr; mod:Module?; info:LineInfo ) : void
        def abstract afterEnumeration ( name:string; prog:ProgramPtr; mod:Module?; info:LineInfo ) : void
        def abstract beforeAlias ( prog:ProgramPtr; mod:Module?; info:LineInfo ) : void
        def abstract afterAlias ( name:string; prog:ProgramPtr; mod:Module?; info:LineInfo ) : void

``add_new_comment_reader`` adds a reader macro to a module.
There is additionally the ``[comment_reader]`` annotation, which essentially automates the same thing.

``open`` occurs when the parsing of a comment starts.

``accept`` occurs for every character of the comment.

``close`` occurs when a comment is over.

``beforeStructure`` and ``afterStructure`` occur before and after each structure or class declaration, regardless of if it has comments.

``beforeStructureFields`` and ``afterStructureFields`` occur before and after each structure or class field, regardless of if it has comments.

``afterStructureField`` occurs after each field declaration.

``beforeFunction`` and ``afterFunction`` occur before and after each function declaration, regardless of if it has comments.

``beforeGlobalVariables`` and ``afterGlobalVariables`` occur before and after each global variable declaration, regardless of if it has comments.

``afterGlobalVariable`` occurs after each individual global variable declaration.

``beforeVariant`` and ``afterVariant`` occur before and after each variant declaration, regardless of if it has comments.

``beforeEnumeration`` and ``afterEnumeration`` occur before and after each enumeration declaration, regardless of if it has comments.

``beforeAlias`` and ``afterAlias`` occur before and after each alias type declaration, regardless or if it has comments.

----------------
AstSimulateMacro
----------------

`AstSimulateMacro` is designed to customize the simulation of the program::

    class AstSimulateMacro
        def abstract preSimulate ( prog:Program?; ctx:Context? ) : bool
        def abstract simulate ( prog:Program?; ctx:Context? ) : bool

``preSimulate`` occurs after the context has been simulated, but before all the structure and function annotation simulations.

``simulate`` occurs after all the structure and function annotation simulations.

----------
AstVisitor
----------

``AstVisitor`` implements the visitor pattern for the daScript expression tree.
It contains a callback for every single expression in prefix and postfix form, as well as some additional callbacks::

    class AstVisitor
        ...
        // find
            def abstract preVisitExprFind(expr:smart_ptr<ExprFind>) : void          // prefix
            def abstract visitExprFind(expr:smart_ptr<ExprFind>) : ExpressionPtr    // postifx
        ...

Postfix callbacks can return expressions to replace the ones passed to the callback.

PrintVisitor from the `ast_print` example implements the printing of every single expression in daScript syntax.

``make_visitor`` creates a visitor adapter from the class, derived from ``AstVisitor``.
The adapter then can be applied to a program via the ``visit`` function::

    var astVisitor = new PrintVisitor()
    var astVisitorAdapter <- make_visitor(*astVisitor)
    visit(this_program(), astVisitorAdapter)

If an expression needs to be visited, and can potentially be fully substituted, the ``visit_expression`` function should be used::

    expr <- visit_expression(expr,astVisitorAdapter)
