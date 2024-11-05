options indenting = 4
options no_unused_block_arguments = false
options no_unused_function_arguments = false
options no_aot = true
options strict_smart_pointers = true

module cpp_bind public

require fio
require rtti
require ast
require daslib/ast_boost
require daslib/strings_boost

def private log_cpp_class_method ( cpp_file:file; name:string; fntype:TypeDeclPtr )
    var inscope cft <- clone_type(fntype.firstType)
    cft.flags |= TypeDeclFlags smartPtrNative
    let rettd = describe_cpp(cft)
    fwrite(cpp_file, "{rettd} {name} ( Context * __context__, Func __funcCall__")
    for argN,argT,argI in fntype.argNames,fntype.argTypes,count()
        fwrite(cpp_file, ", ")
        if argI==0
            fwrite(cpp_file, "void * ")
        else
            let argtd = describe_cpp(argT)
            fwrite(cpp_file, "{argtd} ")
            if argT.isRefType
                fwrite(cpp_file, "& ")
        fwrite(cpp_file, "{argN}")
    fwrite(cpp_file, " )")

def private log_cpp_class_method_call ( cpp_file:file; name:string; fntype:TypeDeclPtr )
    let rettd = describe_cpp(fntype.firstType)
    fwrite(cpp_file, "das_invoke_function<{rettd}>::invoke\n      <")
    for argN,argT,argI in fntype.argNames,fntype.argTypes,count()
        if argI!=0
            fwrite(cpp_file, ",")
        if argI==0
            fwrite(cpp_file, "void *")
        else
            let argtd = describe_cpp(argT)
            fwrite(cpp_file, "{argtd}")
            if argT.isRefType
                fwrite(cpp_file, " &")
    fwrite(cpp_file, ">\n        (__context__,nullptr,__funcCall__,\n          ")
    for argN,argI in fntype.argNames,count()
        if argI!=0
            fwrite(cpp_file, ",")
        fwrite(cpp_file, "{argN}")
    fwrite(cpp_file, ")")
    if fntype.firstType.flags.smartPtr
        fwrite(cpp_file,".marshal()")

def log_cpp_class_adapter ( cpp_file:file; name:string; cinfo:TypeDeclPtr )
    //! Generates C++ class adapter for the Daslang class.
    //! Intended use::
    //!
    //!     log_cpp_class_adapter(cppFileNameDotInc, "daslangClassName", typeinfo(ast_typedecl type<daslangClassName>))
    assert(cinfo.isClass,"can only make adapter out of a class")
    var methods : array<tuple<name:string;index:int>>
    var inscope types : array<TypeDeclPtr>
    for fld,idx in cinfo.structType.fields,count()
        if !fld._type.isFunction || fld.name=="__finalize" || fld.init!=null
            continue
        push(methods,[[auto "{fld.name}",idx]])
        push_clone(types,fld._type)
    let nmet = length(methods)
    fwrite(cpp_file, "class {name} \{\n")
    fwrite(cpp_file, "protected:\n")
    fwrite(cpp_file, "  enum \{\n")
    for mn,mni in methods, count()
        fwrite(cpp_file, "    __fn_{mn.name} = {mni},\n")
    fwrite(cpp_file, "  };\n")
    fwrite(cpp_file, "protected:\n")
    fwrite(cpp_file, "  int _das_class_method_offset[{nmet}];\n")
    fwrite(cpp_file, "public:\n")
    fwrite(cpp_file, "  {name} ( const StructInfo * info ) \{\n")
    for mn,mni in methods, count()
        fwrite(cpp_file, "      _das_class_method_offset[__fn_{mn.name}] = info->fields[{mn.index}]->offset;\n")
    fwrite(cpp_file, "  \}\n")
    for mn,mt in methods,types
        // get
        fwrite(cpp_file, "  __forceinline Func get_{mn.name} ( void * self ) const \{\n")
        fwrite(cpp_file, "    return getDasClassMethod(self,_das_class_method_offset[__fn_{mn.name}]);\n")
        fwrite(cpp_file, "  \}\n")
        // invoke
        fwrite(cpp_file, "  __forceinline ")
        log_cpp_class_method(cpp_file, "invoke_{mn.name}",mt)
        fwrite(cpp_file, " const \{\n")
        fwrite(cpp_file, "    ")
        if !mt.firstType.isVoid
            fwrite(cpp_file, "return ")
        log_cpp_class_method_call(cpp_file,mn.name,mt)
        fwrite(cpp_file, ";\n")
        fwrite(cpp_file, "  \}\n")
    fwrite(cpp_file, "};\n\n")
    delete types
