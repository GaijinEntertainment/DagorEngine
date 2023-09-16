options indenting=4
options remove_unused_symbols=false

require rtti

typedef
    FileAccessPtr = smart_ptr<FileAccess>

[generic]
def structure_for_each_annotation(st:StructInfo;subexpr:block<(ann:Annotation;args:AnnotationArguments):void>)
    rtti_builtin_structure_for_each_annotation(st,subexpr)

[generic]
def is_same_type( a,b:TypeInfo;
        refMatters:RefMatters=RefMatters yes;
        constMatters:ConstMatters=ConstMatters yes;
        temporaryMatters:TemporaryMatters=TemporaryMatters yes;
        topLevel:bool=true )
    unsafe
        return builtin_is_same_type(reinterpret<TypeInfo const?>(a),reinterpret<TypeInfo const?>(b),refMatters,constMatters,temporaryMatters,topLevel)

[generic]
def is_compatible_cast( a,b:StructInfo )
    unsafe
        return is_compatible_cast(reinterpret<StructInfo const?>(a),reinterpret<StructInfo const?>(b))

[generic]
def context_for_each_function( blk:block<(info:FuncInfo):void> )
    for fni in range(get_total_functions(this_context()))
        invoke(blk,get_function_info(this_context(),fni))

[generic]
def context_for_each_variable( blk:block<(info:VarInfo):void> )
    for vi in range(get_total_variables(this_context()))
        invoke(blk,get_variable_info(this_context(),vi))

def private each_dim_any(info)
    unsafe
        let pinfo = addr(info)
        return <- [[ for index in range(int(pinfo.dimSize)); get_dim(*pinfo,index) ]]

[generic]
def each_dim(info:TypeInfo)
    return <- each_dim_any(info)

[generic]
def each_dim(info:VarInfo)
    return <- each_dim_any(info)

def private arg_types_any(info)
    unsafe
        let pinfo = addr(info)
        return <- generator<TypeInfo const&>() <|
            if pinfo.argTypes!=null
                for index in range(int(pinfo.argCount))
                    unsafe
                        yield *(pinfo.argTypes[index])
            return false

[generic]
def arg_types(info:TypeInfo)
    return <- arg_types_any(info)

[generic]
def arg_types(info:VarInfo)
    return <- arg_types_any(info)

def private arg_names_any(info)
    unsafe
        let pinfo = addr(info)
        return <- generator<string>() <|
            if pinfo.argNames!=null
                for index in range(int(pinfo.argCount))
                    unsafe
                        yield pinfo.argNames[index]
            return false

[generic]
def arg_names(info:TypeInfo)
    return <- arg_names_any(info)

[generic]
def arg_names(info:VarInfo)
    return <- arg_names_any(info)

def public class_info ( cl ) : StructInfo const?
    static_if typeinfo(is_pointer cl)
        if cl == null
            return null
        static_if typeinfo(is_class *cl)
            unsafe
                return reinterpret<StructInfo const?>((reinterpret<TypeInfo const?>((*cl).__rtti)).structType)
        else
            concept_assert(false,"not a pointer to a class")
            return null
    static_elif typeinfo(is_class cl)
        unsafe
            return reinterpret<StructInfo const?>((reinterpret<TypeInfo const?>(cl.__rtti)).structType)
    static_elif typeinfo(is_lambda cl)
        unsafe
            var l = reinterpret<int8??> addr(cl)
            if l == null
                return null
            var pti = reinterpret<TypeInfo const??> (l[0] - 16)
            return (*pti).structType
    else
        concept_assert(false,"this type does not have class info. its not a class or a lambda with options rtti")
        return null

[generic]
def type_info ( vinfo:LocalVariableInfo ) : TypeInfo const?
    unsafe
        return reinterpret<TypeInfo const?>(addr(vinfo))

[generic]
def type_info ( vinfo:VarInfo ) : TypeInfo const?
    unsafe
        return reinterpret<TypeInfo const?>(addr(vinfo))

[generic]
def RttiValue_nothing
    var t : RttiValue
    unsafe
        set_variant_index(t, typeinfo(variant_index<nothing> t))
    return t
