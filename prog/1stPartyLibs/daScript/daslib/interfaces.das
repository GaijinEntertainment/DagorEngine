options gen2
options indenting = 4
options no_unused_block_arguments = false
options no_unused_function_arguments = false
options strict_smart_pointers = true

module interfaces shared private

//! Interface-based polymorphism.
//!
//! Annotate a class with ``[interface]`` to declare it as an interface (functions only, no data fields).
//! Annotate another struct with ``[implements(InterfaceName)]`` to generate the binding —
//! this creates a proxy class, a ``get`InterfaceName`` method, and delegates all interface
//! functions to the implementing struct's methods.
//!
//! **Interface inheritance** is supported via native class syntax:
//! ``[interface] class IChild : IParent { ... }``.  A struct that ``[implements(IChild)]``
//! automatically supports ``is``/``as``/``?as`` for both ``IChild`` and ``IParent``.
//!
//! **Default method implementations** are supported: non-abstract methods in an interface
//! class are inherited by the proxy and do not need to be overridden by the implementing struct.
//!
//! **Completeness checking**: at compile time the macro verifies that the implementing struct
//! provides all abstract interface methods.  Missing methods produce a compile error.
//!
//! See :ref:`Interfaces tutorial <tutorial_interfaces>` for a complete walkthrough.

require daslib/ast_boost
require daslib/templates_boost
require daslib/strings_boost
require daslib/defer
require daslib/generic_return
require strings
require daslib/ast

[structure_macro(name="interface")]
class InterfaceMacro : AstStructureAnnotation {
    //! Verifies that the annotated class is a valid interface — it may only contain
    //! function-typed fields (no data members). Applied via ``[interface]`` annotation.
    def override finish(var st : StructurePtr; var group : ModuleGroup; args : AnnotationArgumentList; var errors : das_string) : bool {
        for (fld in st.fields) {
            if (fld.name == "__rtti") {
                continue
            }
            if (!fld._type.isFunction) {
                errors := "interface can only define functions. {fld.name} is not a function\n{errors}"
                return false
            }
        }
        return true
    }
}

[structure_macro(name="implements")]
class ImplementsMacro : AstStructureAnnotation {
    //! Generates interface bindings for a struct. Creates a proxy class that delegates
    //! interface method calls to the struct's own methods, and adds a ``get`InterfaceName``
    //! method that lazily constructs the proxy. Applied via ``[implements(InterfaceName)]``.
    //!
    //! If the interface inherits from a parent interface (``class IChild : IParent``),
    //! ancestor getters are generated automatically so that ``is``/``as``/``?as`` work
    //! with all ancestor interfaces.
    //!
    //! At ``finish`` time, verifies that all abstract interface methods are implemented.
    //! Methods with default bodies in the interface class are optional — the proxy
    //! inherits them via class hierarchy.
    def override apply(var st : StructurePtr; var group : ModuleGroup; args : AnnotationArgumentList; var errors : das_string) : bool {
        if (length(args) != 1) {
            errors := "implements macro takes exactly one argument"
            return false
        }
        var iface_name : string
        if (args[0].basicType == Type.tBool) {
            iface_name = string(args[0].name)
        } elif (args[0].basicType == Type.tString && args[0].name == "name") {
            iface_name = string(args[0].sValue)
        } else {
            errors := "expecting [implements(InteraceName)] or [implements(name=InteraceName)]"
            return false
        }
        // base interface class
        var parent <- compiling_program() |> find_unique_structure(iface_name)
        if (parent == null) {
            errors := "{iface_name} not found"
            return false
        }
        // all names
        let iface_var_name = "_interface_{iface_name}"
        let iface_class_name = "_implementation_{st.name}_{iface_name}"
        let iface_get_name = "get`{iface_name}"
        let iface_get_name_func = "{st.name}`{iface_get_name}"
        // make class for the interface, remove 'interface' annotation
        var inscope cls <- make_class(iface_class_name, parent, st._module)
        cls.flags |= StructureFlags.privateStructure
        var iface_index = -1
        for (ann, ii in cls.annotations, count()) {
            if (ann.annotation.name == "interface") {
                iface_index = ii
                break
            }
        }
        if (iface_index == -1) {
            errors := "can only implement interfaces, {iface_name} is not an [interface]"
            return false
        }
        cls.annotations |> erase(iface_index)
        // add {st}::_interface_{Foo} : Foo?
        var inscope iface_ptr <- qmacro_type(type<$t(parent)?>)
        st |> add_structure_field(iface_var_name, clone_type(iface_ptr), default<ExpressionPtr>)
        // add function {st}::get`{Foo} : Foo?
        var inscope fn_get_i <- qmacro_method(iface_get_name_func, st) $(var self : $t(st) ==const) : $t(iface_ptr) {
            if (self.$f(iface_var_name) == null) {
                self.$f(iface_var_name) = new <$t(cls)>(unsafe(addr(self)))
            }
            return self.$f(iface_var_name)
        }
        add_function(st._module, fn_get_i)
        // if all interface methods are const, also generate a const getter
        // that uses interior mutability to lazily cache the proxy
        if (is_const_interface(parent)) {
            var inscope fn_get_i_const <- qmacro_method(iface_get_name_func, st) $(self : $t(st) ==const) : $t(iface_ptr) {
                var pS = unsafe(reinterpret<$t(st)? -const>(addr(self)))
                if (pS.$f(iface_var_name) == null) {
                    pS.$f(iface_var_name) = new <$t(cls)>(pS)
                }
                return pS.$f(iface_var_name)
            }
            add_function(st._module, fn_get_i_const)
        }
        // add {cls}::_self : {ST}?
        var inscope st_ptr <- qmacro_type(type<$t(st)?>)
        let fi = cls |> add_structure_field("_self", clone_type(st_ptr), default<ExpressionPtr>)
        cls.fields[fi].annotation |> add_annotation_argument("do_not_delete", true)
        cls.fields[fi].flags |= FieldDeclarationFlags.doNotDelete | FieldDeclarationFlags.privateField
        // add {cls} ( var s : {ST}? )
        var inscope ctor <- qmacro_function("{iface_class_name}`{iface_class_name}") $(s : $t(st_ptr)) {
            _self = s
        }
        var inscope ctor_fun <- make_class_constructor(cls, ctor)
        modify_to_class_member(cls, ctor, false, false)
        add_function(st._module, ctor)
        add_function(st._module, ctor_fun)
        // add get`{Foo} : Foo?
        st |> add_structure_field(
            iface_get_name,
            qmacro_type(type<function<(var self : $t(st)) : $t(iface_ptr)>>),
            qmacro(@@ < (var self : $t(st)) : $t(iface_ptr) > $i(iface_get_name_func))
        )
        // ── Ancestor getters (interface inheritance) ──────────────
        // If the interface extends a parent interface (IChild : IParent),
        // generate `get`IParent` fields so that `ptr is IParent`,
        // `ptr as IParent`, and `ptr ?as IParent` work transparently.
        // The ancestor getter delegates to the direct getter — the upcast
        // from IChild? to IParent? is implicit via class inheritance.
        var cur_ancestor = parent.parent
        while (cur_ancestor != null) {
            if (is_interface_struct(cur_ancestor)) {
                let anc_name = string(cur_ancestor.name)
                let anc_get_name = "get`{anc_name}"
                // Skip if this getter already exists (diamond inheritance,
                // or a separate [implements] annotation already generated it)
                var already_exists = false
                for (efld in st.fields) {
                    if (efld.name == anc_get_name) {
                        already_exists = true
                        break
                    }
                }
                if (!already_exists) {
                    // Look up ancestor via find_unique_structure for proper
                    // type reification — $t() requires a properly resolved struct.
                    var anc_struct <- compiling_program() |> find_unique_structure(anc_name)
                    if (anc_struct != null) {
                        let anc_get_func = "{st.name}`{anc_get_name}"
                        var inscope anc_ptr <- qmacro_type(type<$t(anc_struct)?>)
                        // Getter delegates to the direct getter. The IChild? → IParent?
                        // upcast is automatic via class inheritance.
                        var inscope fn_anc <- qmacro_method(anc_get_func, st) $(var self : $t(st)) : $t(anc_ptr) {
                            return $c(iface_get_name_func)(self)
                        }
                        add_function(st._module, fn_anc)
                        // const ancestor getter for const-only interfaces
                        if (is_const_interface(anc_struct)) {
                            var inscope fn_anc_const <- qmacro_method(anc_get_func, st) $(self : $t(st)) : $t(anc_ptr) {
                                return $c(iface_get_name_func)(self)
                            }
                            add_function(st._module, fn_anc_const)
                        }
                        st |> add_structure_field(
                            anc_get_name,
                            qmacro_type(type<function<(var self : $t(st)) : $t(anc_ptr)>>),
                            qmacro(@@$i(anc_get_func))
                        )
                    }
                }
            }
            cur_ancestor = cur_ancestor.parent
        }
        // now, for the methods
        let skipl = length(iface_name) + 1
        for (fld in st.fields) {
            if (string(fld.name) |> starts_with("{iface_name}`")) {
                let method_name = string(fld.name) |> slice(skipl)
                var found = false
                for (cm in cls.fields) {
                    if (cm.name == method_name) {
                        found = true
                        let fmethod_name = "{iface_class_name}`{method_name}"
                        var inscope fmethod_args : array<VariablePtr>
                        var inscope fcall_args : array<ExpressionPtr>
                        var inscope ret_type : TypeDeclPtr
                        if (cm._type.isFunction) {
                            // Abstract method — type info is available on the field.
                            cm.init |> move_new <| qmacro(cast<$t(cm._type)>(@@$i(fmethod_name)))
                            for (n, t in cm._type.argNames, cm._type.argTypes) {
                                if (n != "self") {
                                    fcall_args |> emplace_new <| qmacro($i(n))
                                    fmethod_args |> emplace_new <| new Variable(name := n, _type <- clone_type(t))
                                }
                            }
                            ret_type |> move_new <| clone_type(cm._type.firstType)
                        } else {
                            // Default method being overridden — cm._type is autoinfer,
                            // so we look up the interface's actual method function to
                            // get arg types and return type.
                            let iface_func_name = "{iface_name}`{method_name}"
                            var inscope ifn <- find_unique_function(parent._module, iface_func_name, true)
                            if (ifn == null) {
                                errors := "cannot find default method function {iface_func_name}"
                                return false
                            }
                            cm.init |> move_new <| qmacro(@@$i(fmethod_name))
                            for (arg in ifn.arguments) {
                                let arg_name = string(arg.name)
                                if (arg_name != "self") {
                                    fcall_args |> emplace_new <| qmacro($i(arg.name))
                                    fmethod_args |> emplace_new <| new Variable(name := arg_name, _type <- clone_type(arg._type))
                                }
                            }
                            ret_type |> move_new <| clone_type(ifn.result)
                        }
                        var inscope fmethod <- qmacro_method(fmethod_name, cls) $(var self : $t(cls); $a(fmethod_args)) : $t(ret_type) {
                            generic_return(invoke(self._self.$f(fld.name), *self._self, $a(fcall_args)))
                        }
                        add_function(st._module, fmethod)
                        break
                    }
                }
                if (!found) {
                    errors := "unknown interface method {method_name} in {iface_class_name}"
                    return false
                }
            }
        }
        add_structure(st._module, cls)
        return true
    }

    // ── Completeness check ─────────────────────────────────────
    // Runs after type inference to verify the implementing struct
    // provides all abstract interface methods.  Methods with default
    // implementations (non-null init in the interface class) are
    // optional — the proxy inherits the default via class hierarchy.
    def override finish(var st : StructurePtr; var group : ModuleGroup; args : AnnotationArgumentList; var errors : das_string) : bool {
        // Parse interface name (same logic as apply)
        var iface_name : string
        if (args[0].basicType == Type.tBool) {
            iface_name = string(args[0].name)
        } elif (args[0].basicType == Type.tString && args[0].name == "name") {
            iface_name = string(args[0].sValue)
        } else {
            return true  // apply already reported this error
        }
        var iface <- compiling_program() |> find_unique_structure(iface_name)
        if (iface == null) {
            return true  // apply already reported this error
        }
        // Check each interface method
        var missing = build_string() <| $(var w) {
            for (ifld in iface.fields) {
                let ifld_name = string(ifld.name)
                // Skip compiler-generated fields
                if (ifld_name == "__rtti" || ifld_name == "__finalize") {
                    continue
                }
                if (!ifld._type.isFunction) {
                    continue
                }
                // Methods with a default implementation (init != null) are optional
                if (ifld.init != null) {
                    continue
                }
                // Abstract method — struct must provide {iface_name}`{method_name}
                let expected = "{iface_name}`{ifld_name}"
                var found = false
                for (sfld in st.fields) {
                    if (string(sfld.name) == expected) {
                        found = true
                        break
                    }
                }
                if (!found) {
                    w |> write("{st.name} does not implement {iface_name}.{ifld_name}\n")
                }
            }
        }
        if (length(missing) > 0) {
            errors := missing
            return false
        }
        return true
    }
}

// Helper: check whether all methods of an [interface] class have const self.
// This is used to decide whether to generate a const getter.
def private is_const_interface(st : Structure?) : bool {
    //! Returns ``true`` when every function field in the given [interface] struct
    //! has a const ``self`` parameter (i.e. all methods are const).
    if (st == null) {
        return false
    }
    for (fld in st.fields) {
        let fn = string(fld.name)
        if (fn == "__rtti" || fn == "__finalize") {
            continue
        }
        if (!fld._type.isFunction) {
            continue
        }
        // Check if the first arg (self) is const.
        // argTypes[0] is self for class methods.
        if (length(fld._type.argTypes) == 0) {
            continue
        }
        if (!fld._type.argTypes[0].isConst) {
            return false
        }
    }
    return true
}

// Helper: check whether a Structure carries the [interface] annotation.
// We iterate the annotation list rather than checking a flag because
// [interface] is a user-defined structure_macro, not a built-in flag.
def private is_interface_struct(st : Structure?) : bool {
    //! Returns ``true`` when the structure carries the ``[interface]`` annotation.
    if (st == null) {
        return false
    }
    for (ann in st.annotations) {
        if (ann.annotation.name == "interface") {
            return true
        }
    }
    return false
}

// ────────────────────────────────────────────────────────────────────
// Variant macro: InterfaceAsIs
//
// This macro intercepts the `is`, `as`, and `?as` operators during
// type inference.  When the compiler encounters one of these operators,
// it calls every registered variant macro in order.  The first one to
// return a non-default ExpressionPtr "claims" the expression — the
// compiler replaces the original AST node with the returned one.
//
// Resolution order (three-tier):
//   1. Variant macros  (this class — first non-default return wins)
//   2. Generic operator functions  (def operator is/as Name)
//   3. Built-in variant type handling
//
// Each visitor method follows the same structure:
//   1. TYPE GUARD — verify that the expression is relevant
//      (pointer to a struct, target name is an [interface])
//   2. FIELD LOOKUP — check if the struct has a `get`IFoo` field,
//      which ImplementsMacro generates for each [implements(IFoo)]
//   3. CODE GENERATION — produce a replacement expression via qmacro
//
// Returning `default<ExpressionPtr>` means "I don't handle this" —
// the compiler moves on to the next variant macro or resolution tier.
// ────────────────────────────────────────────────────────────────────

[variant_macro(name="InterfaceAsIs")]
class InterfaceAsIs : AstVariantMacro {
    //! Variant macro that enables ``is``, ``as``, and ``?as`` operators for
    //! interface types declared with ``[interface]`` / ``[implements]``.
    //!
    //! * ``ptr is IFoo``  — compile-time check (``true`` when the struct implements ``IFoo``)
    //! * ``ptr as IFoo``  — obtains the interface proxy via the generated getter
    //! * ``ptr ?as IFoo`` — null-safe version: returns ``null`` when the pointer is null

    // ── is ──────────────────────────────────────────────────────────
    // Intercepts `expr is Name`.  If the struct implements the named
    // interface, this produces the compile-time constant `true`;
    // otherwise `false`.  Zero runtime cost — the result is baked in.
    //
    // Example:  w is IDrawable  →  true   (Widget implements IDrawable)
    //           w is IResizable →  false  (if Widget doesn't implement it)
    def override visitExprIsVariant(prog : ProgramPtr; mod : Module?; expr : smart_ptr<ExprIsVariant>) : ExpressionPtr {
        // ── Type guard ──
        // expr.value is the left-hand side (e.g. `w` in `w is IDrawable`).
        // We only handle pointers to user structs.  Anything else (int,
        // float, handled types, etc.) is declined immediately.
        assume vtype = expr.value._type
        if (!(vtype.isPointer && vtype.firstType != null && vtype.firstType.isStructure)) {
            return <- default<ExpressionPtr>
        }
        // expr.name is the right-hand side — the interface name (e.g. "IDrawable").
        // Convert from das_string to string for API compatibility, then look up
        // the struct by name in the compiling program.  If it doesn't exist or
        // isn't annotated with [interface], we decline.
        let iname = string(expr.name)
        var tgt = prog |> find_unique_structure(iname)
        if (tgt == null || !is_interface_struct(tgt)) {
            return <- default<ExpressionPtr>
        }
        // ── Field lookup ──
        // ImplementsMacro adds a field named `get`IFoo` (a function pointer
        // to the getter) for each [implements(IFoo)].  If the field exists,
        // the struct implements the interface.
        let getter_field = "get`{iname}"
        var st = vtype.firstType.structType
        for (fld in st.fields) {
            if (fld.name == getter_field) {
                // ── Code generation ──
                // Replace `expr is IFoo` with the literal `true`.
                return <- quote(true)
            }
        }
        // The struct is valid but doesn't implement this interface → false.
        return <- quote(false)
    }

    // ── as ──────────────────────────────────────────────────────────
    // Intercepts `expr as Name`.  Generates a call to the getter
    // function that ImplementsMacro created, returning the interface
    // proxy pointer.
    //
    // Example:  w as IDrawable  →  Widget`get`IDrawable(*w)
    //
    // The getter lazily constructs the proxy on first call and caches
    // it, so repeated `as` calls are cheap (just a null check + return).
    def override visitExprAsVariant(prog : ProgramPtr; mod : Module?; expr : smart_ptr<ExprAsVariant>) : ExpressionPtr {
        // ── Type guard (same as visitExprIsVariant) ──
        assume vtype = expr.value._type
        if (!(vtype.isPointer && vtype.firstType != null && vtype.firstType.isStructure)) {
            return <- default<ExpressionPtr>
        }
        let iname = string(expr.name)
        var tgt = prog |> find_unique_structure(iname)
        if (tgt == null || !is_interface_struct(tgt)) {
            return <- default<ExpressionPtr>
        }
        // ── Field lookup ──
        let getter_field = "get`{iname}"
        var st = vtype.firstType.structType
        for (fld in st.fields) {
            if (fld.name == getter_field) {
                // ── Code generation ──
                // Build the getter function name: "Widget`get`IDrawable"
                let func_name = "{st.name}`get`{iname}"
                // $c(func_name) — call a function by its string name
                // *$e(expr.value) — dereference the pointer to pass by ref
                // Result: Widget`get`IDrawable(*w) → returns IDrawable?
                return <- qmacro($c(func_name)(*$e(expr.value)))
            }
        }
        // Struct doesn't implement this interface — decline so the
        // compiler can report a proper error via the next resolution tier.
        return <- default<ExpressionPtr>
    }

    // ── ?as ─────────────────────────────────────────────────────────
    // Intercepts `expr ?as Name`.  Like `as`, but wraps the getter
    // call in a null check so that a null pointer returns null instead
    // of crashing.
    //
    // Example:  w ?as IDrawable
    //       →   w != null ? Widget`get`IDrawable(*w) : null
    //
    // This is the safe version for use when the pointer may be null.
    def override visitExprSafeAsVariant(prog : ProgramPtr; mod : Module?; expr : smart_ptr<ExprSafeAsVariant>) : ExpressionPtr {
        // ── Type guard (same pattern) ──
        assume vtype = expr.value._type
        if (!(vtype.isPointer && vtype.firstType != null && vtype.firstType.isStructure)) {
            return <- default<ExpressionPtr>
        }
        let iname = string(expr.name)
        var tgt = prog |> find_unique_structure(iname)
        if (tgt == null || !is_interface_struct(tgt)) {
            return <- default<ExpressionPtr>
        }
        // ── Field lookup ──
        let getter_field = "get`{iname}"
        var st = vtype.firstType.structType
        for (fld in st.fields) {
            if (fld.name == getter_field) {
                // ── Code generation ──
                let func_name = "{st.name}`get`{iname}"
                // clone_expression is needed because expr.value appears
                // twice in the generated code (null check + getter call).
                // The first $e() splice *moves* the expression, so the
                // second use needs a deep copy to avoid a dangling node.
                var inscope val <- clone_expression(expr.value)
                // Ternary: null check → getter call, or null.
                return <- qmacro($e(val) != null ? $c(func_name)(*$e(expr.value)) : null)
            }
        }
        return <- default<ExpressionPtr>
    }
}

