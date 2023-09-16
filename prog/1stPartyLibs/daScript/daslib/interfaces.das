options indenting = 4
options no_unused_block_arguments = false
options no_unused_function_arguments = false
options no_aot = true
options strict_smart_pointers = true

module interfaces shared private

require daslib/ast_boost
require daslib/templates_boost
require daslib/defer
require daslib/generic_return
require strings
require ast

[structure_macro(name="interface")]
class InterfaceMacro : AstStructureAnnotation
    //! implements 'interface' macro, which verifies if class is an interface (no own variables)
    def override finish ( var st:StructurePtr; var group:ModuleGroup; args:AnnotationArgumentList; var errors : das_string ) : bool
        for fld in st.fields
            if fld.name=="__rtti"
                continue
            if !fld._type.isFunction
                errors := "interface can only define functions. {fld.name} is not a function\n{errors}"
                return false
        return true

[structure_macro(name="implements")]
class ImplementsMacro : AstStructureAnnotation
    //! implements 'implements' macro, adds get`{Interface} method as well as interface bindings and implementation.
    def override apply ( var st:StructurePtr; var group:ModuleGroup; args:AnnotationArgumentList; var errors : das_string ) : bool
        if length(args)!=1
            errors := "implements macro takes exactly one argument"
            return false
        var iface_name : string
        if args[0].basicType == Type tBool
            iface_name = string(args[0].name)
        elif args[0].basicType == Type tString && args[0].name == "name"
            iface_name = string(args[0].sValue)
        else
            errors := "expecting [implements(InteraceName)] or [implements(name=InteraceName)]"
            return false
        // base interface class
        var parent <- compiling_program() |> find_unique_structure(iface_name)
        if parent == null
            errors := "{iface_name} not found"
            return false
        // all names
        let iface_var_name = "_interface_{iface_name}"
        let iface_class_name = "_implementation_{st.name}_{iface_name}"
        let iface_get_name = "get`{iface_name}"
        let iface_get_name_func = "{st.name}`{iface_get_name}"
        // make class for the interface, remove 'interface' annotation
        var inscope cls <- make_class(iface_class_name, parent, st._module)
        cls.flags |= StructureFlags privateStructure
        var iface_index = -1
        for ann,ii in cls.annotations,count()
            if ann.annotation.name == "interface"
                iface_index = ii
                break
        if iface_index==-1
            errors := "can only implement interfaces, {iface_name} is not an [interface]"
            return false
        cls.annotations |> erase(iface_index)
        // add {st}::_interface_{Foo} : Foo?
        var inscope iface_ptr <- qmacro_type(type<$t(StructurePtr(parent))?>)
        st |> add_structure_field ( iface_var_name, clone_type(iface_ptr), [[ExpressionPtr]]);
        // add function {st}::get`{Foo} : Foo?
        var inscope fn_get_i <- qmacro_method(iface_get_name_func,st) <| $ ( var self: $t(st) ) : $t(iface_ptr)
            if self.$f(iface_var_name) == null
                self.$f(iface_var_name) = new <$t(cls)>(unsafe(addr(self)))
            return self.$f(iface_var_name)
        add_function(st._module, fn_get_i)
        // add {cls}::_self : {ST}?
        var inscope st_ptr <- qmacro_type(type<$t(st)?>)
        let fi = cls |> add_structure_field ( "_self", clone_type(st_ptr), [[ExpressionPtr]]);
        cls.fields[fi].annotation |> add_annotation_argument("do_not_delete",true)
        cls.fields[fi].flags |= FieldDeclarationFlags doNotDelete | FieldDeclarationFlags privateField
        // add {cls} ( var s : {ST}? )
        var inscope ctor <- qmacro_function("{iface_class_name}`{iface_class_name}") <| $ ( s : $t(st_ptr) )
            _self = s
        var inscope ctor_fun <- make_class_constructor(cls, ctor)
        modify_to_class_member(cls, ctor, false, false)
        add_function(st._module, ctor)
        add_function(st._module, ctor_fun)
        // add get`{Foo} : Foo?
        st |> add_structure_field (
            iface_get_name,
            qmacro_type(type<function<(self:$t(st)):$t(iface_ptr)>>),
            qmacro(@@$i(iface_get_name_func))
        )
        // now, for the methods
        let skipl = length(iface_name) + 1
        for fld in st.fields
            if string(fld.name) |> starts_with("{iface_name}`")
                let method_name = string(fld.name) |> slice(skipl)
                var found = false
                for cm in cls.fields
                    if cm.name == method_name
                        found = true
                        let fmethod_name = "{iface_class_name}`{method_name}"
                        cm.init := null
                        unsafe
                            cm.init <- qmacro(cast<$t(cm._type)>(@@$i(fmethod_name)))
                        var inscope fmethod_args : array<VariablePtr>
                        var inscope fcall_args : array<ExpressionPtr>
                        for n,t in cm._type.argNames,cm._type.argTypes
                            if n != "self"
                                fcall_args |> emplace_new <| qmacro($i(n))
                                fmethod_args |> emplace_new <| new [[Variable() name:=n, _type<-clone_type(t)]]
                        var inscope fmethod <- qmacro_method(fmethod_name,cls) <| $ ( self : $t(cls); $a(fmethod_args) ) : $t(cm._type.firstType)
                            generic_return ( invoke ( self._self.$f(fld.name), *self._self, $a(fcall_args) ) )
                        add_function(st._module, fmethod)
                        break
                if !found
                    errors := "unknown interface method {method_name} in {iface_class_name}"
                    return false
        add_structure(st._module, cls)
        return true

