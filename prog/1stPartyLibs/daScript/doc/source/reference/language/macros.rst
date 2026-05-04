.. _macros:

======
Macros
======

In Daslang, macros are the machinery that allow direct manipulation of the syntax tree.

Macros are exposed via the :ref:`daslib/ast <stdlib_ast>` module and :ref:`daslib/ast_boost <stdlib_ast_boost>` helper module.

Macros are evaluated at compilation time during different compilation passes.
Macros assigned to a specific module are evaluated as part of the module every time that module is included.

------------------
Compilation passes
------------------

The Daslang compiler performs compilation passes in the following order for each module (see :ref:`Modules <modules>`):

#. Parser transforms das program to AST

    #. If there are any parsing errors, compilation stops

#. ``apply`` is called for every function or structure

    #. If there are any errors, compilation stops

#. Infer pass repeats itself until no new transformations are reported

    #. Built-in infer pass happens

        #. ``transform`` macros are called for every function or expression

    #. Macro passes happen

#. If there are still any errors left, compilation stops

#. ``finish`` is called for all functions and structure macros

#. Lint pass happens

    #. If there are any errors, compilation stops

#. Optimization pass repeats itself until no new transformations are reported

    #. Built-in optimization pass happens

    #. Macro optimization pass happens

#. If there are any errors during optimization passes, compilation stops

#. If the module contains any macros, simulation happens

    #. If there are any simulation errors, compilation stops

    #. Module macro functions (annotated with ``_macro``) are invoked

        #. If there are any errors, compilation stops

Modules are compiled in ``require`` order.

---------------
Invoking macros
---------------

The ``[_macro]`` annotation is used to specify functions that should be evaluated at compilation time .
Consider the following example from :ref:`daslib/ast_boost <stdlib_ast_boost>`:

.. code-block:: das

    [_macro,private]
    def setup {
        if ( is_compiling_macros_in_module("ast_boost") ) {
            add_new_function_annotation("macro", new MacroMacro())
        }
    }

The ``setup`` function is evaluated after the compilation of each module, which includes ast_boost.
The ``is_compiling_macros_in_module`` function returns true if the currently compiled module name matches the argument.
In this particular example, the function annotation ``macro`` would only be added once: when the module ``ast_boost`` is compiled.

Macros are invoked in the following fashion:

#. Class is derived from the appropriate base macro class
#. Adapter is created
#. Adapter is registered with the module

For example, this is how this lifetime cycle is implemented for the reader macro:

.. code-block:: das

    def add_new_reader_macro ( name:string; someClassPtr ) {
        var ann <- make_reader_macro(name, someClassPtr)
        this_module() |> add_reader_macro(ann)
        unsafe {
            delete ann
        }
    }

---------------------
AstFunctionAnnotation
---------------------

The ``AstFunctionAnnotation`` macro allows you to manipulate calls to specific functions as well as their function bodies.
Annotations can be added to regular or generic functions.

``add_new_function_annotation`` adds a function annotation to a module.
There is additionally the ``[function_macro]`` annotation which accomplishes the same thing.

``AstFunctionAnnotation`` allows several different manipulations:

.. code-block:: das

    class AstFunctionAnnotation {
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
    }

``transform`` lets you change calls to the function and is applied at the infer pass.
Transform is the best way to replace or modify function calls with other semantics.

``verifyCall`` is called during the ``lint`` phase on each call to the function and is used to check if the call is valid.

``apply`` is applied to the function itself before the infer pass.
Apply is typically where global function body modifications or instancing occurs.

``finish`` is applied to the function itself after the infer pass.
It's only called on non-generic functions or instances of the generic functions.
``finish`` is typically used to register functions, notify C++ code, etc.
After this, the function is fully defined and inferred, and can no longer be modified.

``patch`` is called after the infer pass. If patch sets astChanged to true, the infer pass will be repeated.

``fixup`` is called after the infer pass. It's used to fixup the function's body.

``lint`` is called during the ``lint`` phase on the function itself and is used to verify that the function is valid.

``complete`` is called during the ``simulate`` portion of context creation. At this point Context is available.

``isSpecialized`` must return true if the particular function matching is governed by contracts.
In that case, ``isCompatible`` is called, and the result taken into account.

``isCompatible`` returns true if a specialized function is compatible with the given arguments.
If a function is not compatible, the errors field must be specified.

``appendToMangledName`` is called to append a mangled name to the function.
That way multiple functions with the same type signature can exist and be differentiated between.

Lets review the following example from ``ast_boost`` of how the ``macro`` annotation is implemented:

.. code-block:: das

    class MacroMacro : AstFunctionAnnotation {
        def override apply ( var func:FunctionPtr; var group:ModuleGroup; args:AnnotationArgumentList; var errors : das_string ) : bool {
            compiling_program().flags |= ProgramFlags needMacroModule
            func.flags |= FunctionFlags init
            var blk <- new ExprBlock(at=func.at)
            var ifm <- new ExprCall(at=func.at, name:="is_compiling_macros")
            var ife <- new ExprIfThenElse(at=func.at, cond<-ifm, if_true<-func.body)
            push(blk.list,ife)
            func.body <- blk
            return true
        }
    }

During the ``apply`` pass the function body is appended with the ``if is_compiling_macros()`` closure.
Additionally, the ``init`` flag is set, which is equivalent to a ``_macro`` annotation.
Functions annotated with ``[macro]`` are evaluated during module compilation.

------------------
AstBlockAnnotation
------------------

``AstBlockAnnotation`` is used to manipulate block expressions (blocks, lambdas, local functions):

.. code-block:: das

    class AstBlockAnnotation {
        def abstract apply ( var blk:smart_ptr<ExprBlock>; var group:ModuleGroup; args:AnnotationArgumentList; var errors : das_string ) : bool
        def abstract finish ( var blk:smart_ptr<ExprBlock>; var group:ModuleGroup; args,progArgs:AnnotationArgumentList; var errors : das_string ) : bool
    }

``add_new_block_annotation`` adds a block annotation to a module.
There is additionally the ``[block_macro]`` annotation which accomplishes the same thing.

``apply`` is called for every block expression before the infer pass.

``finish`` is called for every block expression after infer pass.

----------------------
AstStructureAnnotation
----------------------

The ``AstStructureAnnotation`` macro lets you manipulate structure or class definitions via annotation:

.. code-block:: das

    class AstStructureAnnotation {
        def abstract apply ( var st:StructurePtr; var group:ModuleGroup; args:AnnotationArgumentList; var errors : das_string ) : bool
        def abstract finish ( var st:StructurePtr; var group:ModuleGroup; args:AnnotationArgumentList; var errors : das_string ) : bool
        def abstract patch ( var st:StructurePtr; var group:ModuleGroup; args:AnnotationArgumentList; var errors : das_string; var astChanged:bool& ) : bool
        def abstract complete ( var st:StructurePtr; var ctx:smart_ptr<Context> ) : void
    }

``add_new_structure_annotation`` adds a structure annotation to a module.
There is additionally the ``[structure_macro]`` annotation which accomplishes the same thing.

``AstStructureAnnotation`` allows 4 different manipulations:

.. code-block:: das

    class AstStructureAnnotation {
        def abstract apply ( var st:StructurePtr; var group:ModuleGroup; args:AnnotationArgumentList; var errors : das_string ) : bool
        def abstract finish ( var st:StructurePtr; var group:ModuleGroup; args:AnnotationArgumentList; var errors : das_string ) : bool
        def abstract patch ( var st:StructurePtr; var group:ModuleGroup; args:AnnotationArgumentList; var errors : das_string; var astChanged : bool& ) : bool
        def abstract complete ( var st:StructurePtr; var group:ModuleGroup; args:AnnotationArgumentList; var errors : das_string; var context : smart_ptr<Context> ) : bool
    }

``apply`` is invoked before the infer pass. It is the best time to modify the structure, generate some code, etc.

``finish`` is invoked after the successful infer pass. Its typically used to register structures, perform RTTI operations, etc.
After this, the structure is fully inferred and defined and can no longer be modified afterwards.

``patch`` is invoked after the infer pass. If patch sets astChanged to true, the infer pass will be repeated.

``complete`` is invoked during the ``simulate`` portion of context creation. At this point Context is available.

An example of such annotation is ``SetupAnyAnnotation`` from :ref:`daslib/ast_boost <stdlib_ast_boost>`.

------------------------
AstEnumerationAnnotation
------------------------

The ``AstEnumerationAnnotation`` macro lets you manipulate enumerations via annotation:

.. code-block:: das

    class AstEnumerationAnnotation {
        def abstract apply ( var st:EnumerationPtr; var group:ModuleGroup; args:AnnotationArgumentList; var errors : das_string ) : bool
    }

``add_new_enumeration_annotation`` adds an enumeration annotation to a module.
There is additionally the ``[enumeration_macro]`` annotation which accomplishes the same thing.

``apply`` is invoked before the infer pass. It is the best time to modify the enumeration, generate some code, etc.

In gen2 syntax, register an enumeration macro and annotate enums with:

.. code-block:: das

    [enumeration_macro(name="enum_total")]
    class EnumTotalAnnotation : AstEnumerationAnnotation {
        def override apply(var enu : EnumerationPtr; var group : ModuleGroup;
                           args : AnnotationArgumentList;
                           var errors : das_string) : bool {
            // modify enu.list or generate code
            return true
        }
    }

    [enum_total]
    enum Direction { North; South; East; West }

.. seealso::

   Tutorial: :ref:`tutorial_macro_enumeration_macro` — step-by-step
   enumeration macro examples (enum modification and code generation)

   Standard library: ``daslib/enum_trait.das`` —
   :ref:`enum_trait module reference <stdlib_enum_trait>`

---------------
AstVariantMacro
---------------

``AstVariantMacro`` is specialized in transforming ``is``, ``as``, and ``?as`` expressions.

``add_new_variant_macro`` adds a variant macro to a module.
There is additionally the ``[variant_macro]`` annotation which accomplishes the same thing.

Each of the 3 transformations are covered in the appropriate abstract function:

.. code-block:: das

    class AstVariantMacro {
        def abstract visitExprIsVariant     ( prog:ProgramPtr; mod:Module?; expr:smart_ptr<ExprIsVariant> ) : ExpressionPtr
        def abstract visitExprAsVariant     ( prog:ProgramPtr; mod:Module?; expr:smart_ptr<ExprAsVariant> ) : ExpressionPtr
        def abstract visitExprSafeAsVariant ( prog:ProgramPtr; mod:Module?; expr:smart_ptr<ExprSafeAsVariant> ) : ExpressionPtr
    }

Let's review the following example from :ref:`daslib/ast_boost <stdlib_ast_boost>`:

.. code-block:: das

    // replacing ExprIsVariant(value,name) => ExprOp2('==",value.__rtti,"name")
    // if value is ast::Expr*
    class BetterRttiVisitor : AstVariantMacro {
        def override visitExprIsVariant(prog:ProgramPtr; mod:Module?;expr:smart_ptr<ExprIsVariant>) : ExpressionPtr {
            if ( isExpression(expr.value._type) ) {
                var vdr <- new ExprField(at=expr.at, name:="__rtti", value <- clone_expression(expr.value))
                var cna <- new ExprConstString(at=expr.at, value:=expr.name)
                var veq <- new ExprOp2(at=expr.at, op:="==", left<-vdr, right<-cna)
                return veq
            }
            return default<ExpressionPtr>
        }
    }

    // note the following usage
    class GetHintFnMacro : AstFunctionAnnotation {
        def override transform ( var call : smart_ptr<ExprCall>; var errors : das_string ) : ExpressionPtr {
            if ( call.arguments[1] is ExprConstString ) {    // HERE EXPRESSION WILL BE REPLACED
                ...
            }
        }
    }

Here, the macro takes advantage of the ExprIsVariant syntax.
It replaces the ``expr is TYPENAME`` expression with an ``expr.__rtti = "TYPENAME"`` expression.
The ``isExpression`` function ensures that ``expr`` is from the ``ast::Expr*`` family, i.e. part of the Daslang syntax tree.

--------------
AstReaderMacro
--------------

``AstReaderMacro`` allows embedding a completely different syntax inside Daslang code.

``add_new_reader_macro`` adds a reader macro to a module.
There is additionally the ``[reader_macro]`` annotation, which essentially automates the same thing.

Reader macros accept characters, collect them if necessary, and produce output
via one of two patterns:

.. code-block:: das

    class AstReaderMacro {
        def abstract accept ( prog:ProgramPtr; mod:Module?; expr:ExprReader?; ch:int; info:LineInfo ) : bool
        def abstract visit ( prog:ProgramPtr; mod:Module?; expr:smart_ptr<ExprReader> ) : ExpressionPtr
        def abstract suffix ( prog:ProgramPtr; mod:Module?; expr:ExprReader?; info:LineInfo; var outLine:int&; var outFile:FileInfo?& ) : string
    }

Reader macros are invoked via the ``% READER_MACRO_NAME ~ character_sequence`` syntax.
The ``accept`` function notifies the correct terminator of the character sequence:

.. code-block:: das

    var x = %arr~\{\}\w\x\y\n%% // invoking reader macro arr, %% is a terminator

Consider the implementation for the example above:

.. code-block:: das

    [reader_macro(name="arr")]
    class ArrayReader : AstReaderMacro {
        def override accept ( prog:ProgramPtr; mod:Module?; var expr:ExprReader?; ch:int; info:LineInfo ) : bool {
            append(expr.sequence,ch)
            if ( ends_with(expr.sequence,"%%") ) {
                let len = length(expr.sequence)
                resize(expr.sequence,len-2)
                return false
            } else {
                return true
            }
        }
        def override visit ( prog:ProgramPtr; mod:Module?; expr:smart_ptr<ExprReader> ) : ExpressionPtr {
            let seqStr = string(expr.sequence)
            var arrT <- new TypeDecl(baseType=Type tInt)
            push(arrT.dim,length(seqStr))
            var mkArr <- new ExprMakeArray(at = expr.at, makeType <- arrT)
            for ( x in seqStr ) {
                var mkC <- new ExprConstInt(at=expr.at, value=x)
                push(mkArr.values,mkC)
            }
            return <- mkArr
        }
    }

The ``accept`` function macro collects symbols in the sequence.
Once the sequence ends with the terminator sequence %%, ``accept`` returns false to indicate the end of the sequence.

In ``visit``, the collected sequence is converted into a make array ``[ch1,ch2,..]`` expression.

More complex examples include the JsonReader macro in :ref:`daslib/json_boost <stdlib_json_boost>` or RegexReader in :ref:`daslib/regex_boost <stdlib_regex_boost>`.

``suffix`` is an alternative to ``visit`` — it is called immediately after ``accept`` during parsing,
before the AST is built.  Instead of returning an AST node, it returns a **string** of daScript source
code that the parser re-parses.  This is useful for generating top-level declarations (functions,
structs) from custom syntax.  When used at module level the ``ExprReader`` node is discarded,
and the suffix text is the only output.  ``SpoofInstanceReader`` in ``daslib/spoof.das`` is an example.

The ``outLine`` and ``outFile`` parameters allow remapping line information for error reporting in the
injected code.

.. seealso::

   :ref:`Tutorial: Reader Macros <tutorial_macro_reader_macro>` — step-by-step example
   of both visit and suffix patterns.

------------
AstCallMacro
------------

``AstCallMacro`` operates on expressions which have function call syntax or something similar.
It occurs during the infer pass.

``add_new_call_macro`` adds a call macro to a module.
The ``[call_macro]`` annotation automates the same thing:

    .. code-block:: das

        class AstCallMacro {
            def abstract preVisit ( prog:ProgramPtr; mod:Module?; expr:smart_ptr<ExprCallMacro> ) : void
            def abstract visit ( prog:ProgramPtr; mod:Module?; expr:smart_ptr<ExprCallMacro> ) : ExpressionPtr
            def abstract canVisitArguments ( expr:smart_ptr<ExprCallMacro> ) : bool
        }

``apply`` from :ref:`daslib/apply <stdlib_apply>` is an example of such a macro:

.. code-block:: das

    [call_macro(name="apply")]  // apply(value, block)
    class ApplyMacro : AstCallMacro {
        def override visit ( prog:ProgramPtr; mod:Module?; var expr:smart_ptr<ExprCallMacro> ) : ExpressionPtr {
            ...
        }
    }

Note how the name is provided in the ``[call_macro]`` annotation.

``preVisit`` is called before the arguments are visited.

``visit`` is called after the arguments are visited.

``canVisitArguments`` is called to determine if the macro can visit the arguments.

------------
AstPassMacro
------------

``AstPassMacro`` is one macro to rule them all. It gets the entire program as
input and can be invoked at numerous passes:

.. code-block:: das

    class AstPassMacro {
        def abstract apply(prog : ProgramPtr; mod : Module?) : bool
    }

Five annotations control when a pass macro runs:

- ``[infer_macro]`` — after clean type inference.  Returning ``true`` re-infers.
- ``[dirty_infer_macro]`` — during each dirty inference pass.
- ``[lint_macro]`` — after successful compilation (lint phase, read-only).
- ``[global_lint_macro]`` — same as ``[lint_macro]`` but for all modules.
- ``[optimization_macro]`` — during the optimisation loop.

``make_pass_macro`` registers a class as a pass macro.

Typically, such macros create an ``AstVisitor`` which performs the necessary
transformations via ``visit(prog, adapter)``.

.. seealso::

   :ref:`tutorial_macro_pass_macro` — step-by-step tutorial with lint and
   infer macro examples.

------------
AstTypeMacro
------------

``AstTypeMacro`` lets you define custom type expressions resolved during
type inference.  It has a single method:

.. code-block:: das

    class AstTypeMacro {
        def abstract visit ( prog:ProgramPtr; mod:Module?; td:TypeDeclPtr; passT:TypeDeclPtr ) : TypeDeclPtr
    }

``add_new_type_macro`` adds a type macro to a module.
The ``[type_macro(name="…")]`` annotation automates registration.

The compiler parses invocations like ``name(type<T>, N)`` in type position
into a ``TypeDecl`` with ``baseType = Type.typeMacro``.  The arguments are
stored in ``td.dimExpr``:

- ``dimExpr[0]`` — ``ExprConstString`` with the macro name
- ``dimExpr[1..]`` — user arguments (``ExprTypeDecl`` for types,
  ``ExprConstInt`` for integers, etc.)

``visit()`` is called in two contexts:

- **Concrete** — all types are inferred; ``passT`` is null;
  ``dimExpr[i]._type`` is the resolved type.
- **Generic** — type parameters like ``auto(TT)`` are unresolved;
  ``passT`` carries the actual argument type for matching;
  ``dimExpr[i]._type`` is null.

.. seealso::

   :ref:`tutorial_macro_type_macro` — step-by-step tutorial showing
   concrete and generic type-macro usage.

----------------
AstTypeInfoMacro
----------------

``AstTypeInfoMacro`` is designed to implement custom type information inside a typeinfo expression:

.. code-block:: das

    class AstTypeInfoMacro {
        def abstract getAstChange ( expr:smart_ptr<ExprTypeInfo>; var errors:das_string ) : ExpressionPtr
        def abstract getAstType ( var lib:ModuleLibrary; expr:smart_ptr<ExprTypeInfo>; var errors:das_string ) : TypeDeclPtr
    }

``add_new_typeinfo_macro`` adds a typeinfo macro to a module.
There is additionally the ``[typeinfo_macro]`` annotation, which essentially automates the same thing.

The ``typeinfo`` expression uses gen2 syntax with the trait name **outside** the
parentheses::

    typeinfo trait_name(type<T>)                // basic
    typeinfo trait_name<subtrait>(type<T>)      // with subtrait
    typeinfo trait_name<sub;extra>(type<T>)     // with subtrait and extratrait

``getAstChange`` returns a newly generated AST node for the typeinfo expression.
Alternatively, it returns null if no changes are required, or if there is an error.
In case of error, the errors string must be filled.

``getAstType`` returns the type of the new typeinfo expression.

.. seealso::

   Tutorial: :ref:`tutorial_macro_typeinfo_macro` — step-by-step guide with three
   ``getAstChange`` examples (struct description, enum names, method check).

---------------
AstForLoopMacro
---------------

``AstForLoopMacro`` is designed to implement custom processing of for loop expressions:

.. code-block:: das

    class AstForLoopMacro {
        def abstract visitExprFor ( prog:ProgramPtr; mod:Module?; expr:smart_ptr<ExprFor> ) : ExpressionPtr
    }

``add_new_for_loop_macro`` adds a reader macro to a module.
There is additionally the ``[for_loop_macro]`` annotation, which essentially automates the same thing.

``visitExprFor`` is similar to that of ``AstVisitor``. It returns a new expression, or null if no changes are required.

---------------
AstCaptureMacro
---------------

``AstCaptureMacro`` is designed to implement custom capturing and finalization of lambda expressions:

.. code-block:: das

    class AstCaptureMacro {
        def abstract captureExpression ( prog:Program?; mod:Module?; expr:ExpressionPtr; etype:TypeDeclPtr ) : ExpressionPtr
        def abstract captureFunction ( prog:Program?; mod:Module?; var lcs:Structure?; var fun:FunctionPtr ) : void
        def abstract releaseFunction ( prog:Program?; mod:Module?; var lcs:Structure?; var fun:FunctionPtr ) : void
    }

``add_new_capture_macro`` adds a reader macro to a module.
There is additionally the ``[capture_macro]`` annotation, which essentially automates the same thing.

``captureExpression`` is called per captured variable when the lambda struct is being built.
It returns a replacement expression to wrap the capture, or null if no changes are required.

``captureFunction`` is called once after the lambda function is generated.
Use this to inspect captured fields (``lcs``) and append code to ``(fun.body as ExprBlock).finalList`` —
which runs **after each invocation** (per-call finally), not on destruction.

``releaseFunction`` is called once when the lambda **finalizer** is generated.
``fun`` is the finalizer function (not the lambda call function).
Code appended to ``(fun.body as ExprBlock).list`` runs on **destruction** —
after the user-written ``finally {}`` block but before the compiler-generated
field cleanup (``delete *__this``).

.. seealso::

   :ref:`Tutorial: Capture Macros <tutorial_macro_capture_macro>` — step-by-step example
   using all three hooks with an ``[audited]`` tag annotation.

----------------
AstCommentReader
----------------

``AstCommentReader`` is designed to implement custom processing of comment expressions:

.. code-block:: das

    class AstCommentReader {
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
    }

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

``AstSimulateMacro`` is designed to customize the simulation of the program:

.. code-block:: das

    class AstSimulateMacro {
        def abstract preSimulate ( prog:Program?; ctx:Context? ) : bool
        def abstract simulate ( prog:Program?; ctx:Context? ) : bool
    }

``preSimulate`` occurs after the context has been simulated, but before all the structure and function annotation simulations.

``simulate`` occurs after all the structure and function annotation simulations.

----------
AstVisitor
----------

``AstVisitor`` implements the visitor pattern for the Daslang expression tree.
It contains a callback for every single expression in prefix and postfix form, as well as some additional callbacks:

.. code-block:: das

    class AstVisitor {
        ...
        // find
            def abstract preVisitExprFind(expr:smart_ptr<ExprFind>) : void          // prefix
            def abstract visitExprFind(expr:smart_ptr<ExprFind>) : ExpressionPtr    // postifx
        ...
    }

Postfix callbacks can return expressions to replace the ones passed to the callback.

PrintVisitor from the ``ast_print`` example implements the printing of every single expression in Daslang syntax.

``make_visitor`` creates a visitor adapter from the class, derived from ``AstVisitor``.
The adapter then can be applied to a program via the ``visit`` function:

.. code-block:: das

    var astVisitor = new PrintVisitor()
    var astVisitorAdapter <- make_visitor(*astVisitor)
    visit(this_program(), astVisitorAdapter)

If an expression needs to be visited, and can potentially be fully substituted, the ``visit_expression`` function should be used:

.. code-block:: das

    expr <- visit_expression(expr,astVisitorAdapter)

.. seealso::

    :ref:`Annotations <annotations>` for annotation-based macro registration,
    :ref:`Reification <reification>` for AST reification used in macros,
    :ref:`Program structure <program_structure>` for the compilation lifecycle,
    :ref:`Generic programming <generic_programming>` for ``typeinfo`` macros.
