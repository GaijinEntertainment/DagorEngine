options indenting = 4
options no_unused_block_arguments = false
options no_unused_function_arguments = false
options no_aot = true
options strict_smart_pointers = true

module rst private

require fio
require math
require rtti
require ast
require strings
require daslib/ast_boost
require daslib/strings_boost
require daslib/regex
require daslib/regex_boost

let
    log_documentation = true
    show_hidden_groups = false

var public
    topic_root = ""

def module_name(mod)
    let mn = string(mod.name)
    return mn=="$" ? "builtin" : mn

def alias_label_name(td:TypeDeclPtr)
    return "alias-{td.alias}"

def global_label_name(value:Variable?)
    return "global-{module_name(value._module)}-{value.name}"

def global_label_name(value:VariablePtr)
    return global_label_name(get_ptr(value))

def enum_label_name(value:Enumeration?)
    return "enum-{module_name(value._module)}-{value.name}"

def enum_label_name(value:EnumerationPtr)
    return enum_label_name(get_ptr(value))

def struct_label_name(value:Structure?)
    return "struct-{module_name(value._module)}-{value.name}"

def struct_label_name(value:StructurePtr)
    return struct_label_name(get_ptr(value))

def handle_label_name(value)
    return "handle-{module_name(value._module)}-{value.name}"

def call_macro_label_name(mod,value)
    return "call-macro-{module_name(mod)}-{value}"

def function_label_name(value:FunctionPtr)
    var mn = get_mangled_name(value)
    mn = mn |> replace_multiple([{auto
        " " => "_";
        "$" => "_builtin_";
        "#" => "_hh_";
        ":" => "_c_";
        "@" => "_at_";
        "<" => "_ls_";
        ">" => "_gr_";
        "`" => "_rq_";
        "!" => "_ex_";
        "=" => "_eq_"
    }])
    return "function-{mn}"

def describe_type_short(td:TypeDecl?)
    unsafe
        return describe_type_short(reinterpret<TypeDeclPtr> td)

def describe_type_short(td:TypeDeclPtr)
    if !empty(td.alias) && !td.isAuto && !td.isAlias
        return string(td.alias)
    let tp = describe(td,false,false,false)
    return replace(tp,"$::","")

def describe_type(td:TypeDecl?)
    unsafe
        return describe_type(reinterpret<TypeDeclPtr> td)

def describe_type(td:TypeDeclPtr)
    return build_string() <| $ (var writer)
        if !empty(td.alias) && !td.isAuto && !td.isAlias
            write(writer,make_ref(alias_label_name(td),string(td.alias)))
            return
        let baseType = td.baseType
        if baseType==Type alias
            if !empty(td.alias)
                write(writer,td.alias)
            else
                write(writer,"alias")
        elif baseType==Type autoinfer
            write(writer,"auto")
            if !empty(td.alias)
                write(writer,"({td.alias})")
        elif baseType==Type tHandle
            let hname = string(td.annotation.name)
            if hname |> starts_with("dasvector`smart_ptr`") // 20 characters
                write(writer,"vector<smart_ptr<")
                write(writer,hname |> slice(20))
                write(writer,">>")
            elif hname |> starts_with("dasvector`") // 10 characters
                write(writer,"vector<")
                write(writer,hname |> slice(10))
                write(writer,">")
            elif hname |> starts_with("pair`") // 5 characters
                write(writer,"pair<")
                var parts <- split(hname,"`")
                write(writer,parts[1])
                write(writer,",")
                write(writer,parts[2])
                write(writer,">")
            else
                write(writer,make_ref(handle_label_name(td.annotation),"{module_name(td.annotation._module)}::{td.annotation.name}"))
        elif baseType==Type tStructure
            write(writer,make_ref(struct_label_name(td.structType),"{module_name(td.structType._module)}::{td.structType.name}"))
        elif baseType==Type tEnumeration || baseType==Type tEnumeration8 || baseType==Type tEnumeration16
            write(writer,make_ref(enum_label_name(td.enumType),"{module_name(td.enumType._module)}::{td.enumType.name}"))
        elif baseType==Type tPointer
            if td.flags.smartPtr
                write(writer,"smart_ptr<")
            if td.firstType!=null
                write(writer,describe_type(td.firstType))
            else
                write(writer,"void")
            write(writer,td.flags.smartPtr ? ">" : "?")
        elif baseType==Type tArray
            write(writer,"array")
            if td.firstType!=null
                write(writer,"<")
                write(writer,describe_type(td.firstType))
                write(writer,">")
        elif baseType==Type tTable
            write(writer,"table")
            if td.firstType!=null
                write(writer,"<")
                write(writer,describe_type(td.firstType))
                write(writer,";")
                write(writer,describe_type(td.secondType))
                write(writer,">")
        elif baseType==Type tIterator
            write(writer,"iterator")
            if td.firstType!=null
                write(writer,"<")
                write(writer,describe_type(td.firstType))
                write(writer,">")
        elif baseType==Type tBlock || baseType==Type tFunction || baseType==Type tLambda
            write(writer,das_to_string(baseType))
            write(writer,"<")
            if td.argTypes |> length!=0
                write(writer,"(")
                if td.argNames |> length!=0
                    var first = true
                    for at,an in td.argTypes,td.argNames
                        if first
                            first = false
                        else
                            write(writer,";")
                        write(writer,an)
                        write(writer,":")
                        write(writer,describe_type(at))
                else
                    var first = true
                    for at in td.argTypes
                        if first
                            first = false
                        else
                            write(writer,";")
                        write(writer,describe_type(at))
                write(writer,")")
                if td.firstType!=null
                    write(writer,":")
                    write(writer,describe_type(td.firstType))
            write(writer,">")
        elif baseType==Type tTuple || baseType==Type tVariant
            write(writer,das_to_string(baseType))
            write(writer,"<")
            if td.argTypes |> length!=0
                if td.argNames |> length!=0
                    var first = true
                    for at,an in td.argTypes,td.argNames
                        if first
                            first = false
                        else
                            write(writer,";")
                        write(writer,an)
                        write(writer,":")
                        write(writer,describe_type(at))
                else
                    var first = true
                    for at in td.argTypes
                        if first
                            first = false
                        else
                            write(writer,";")
                        write(writer,describe_type(at))
            write(writer,">")
        elif baseType==Type tBitfield
            write(writer,das_to_string(baseType))
            write(writer,"<")
            var first = true
            for an in td.argNames
                if first
                    first = false
                else
                    write(writer,";")
                write(writer,an)
            write(writer,">")
        else
            write(writer,das_to_string(baseType))
        if td.flags.constant
            write(writer," const")
        for d in td.dim
            write(writer,"[")
            write(writer,d)
            write(writer,"]")
        if td.flags.ref
            write(writer,"&")
        if td.flags.temporary
            write(writer,"#")
        if td.flags._implicit
            write(writer," implicit")
        if td.flags.explicitConst
            write(writer,"!")

def make_domain(attr:string)
    return ".. {attr}:: "

def make_label(name:string)
    return ".. _{name}:\n\n"

def make_ref(name,text:string)
    let t = text |> replace_multiple([{auto
        "<" => "\\<";
        ">" => "\\>";
        "!" => "\\!"
    }])
    return " :ref:`{t} <{name}>` "

def make_group(name:string; plus:string="+")
    let len = length(name)
    return "{strings::repeat(plus,len)}\n{name}\n{strings::repeat(plus,len)}\n\n"

def make_header(name,lab:string)
    let wide = length(name)
    return ".. _stdlib_{lab}:\n\n{strings::repeat("=",wide)}\n{name}\n{strings::repeat("=",wide)}\n\n"

def private write_table_separator(var writer:StringBuilderWriter; wid:array<int>; sep:string="-")
    for x in wid
        write(writer,"+{strings::repeat(sep,x)}")
    write(writer,"+\n")

def private write_table_line(var writer:StringBuilderWriter;line:array<string>;wid:array<int>)
    for x,l in wid,line
        write(writer,"+{wide(l,x)}")
    write(writer,"+\n")

def make_table(tab:array<array<string>>;withHeader:bool=false)
    return build_string() <| $(writer)
        let rows = length(tab)
        var cols = 0
        for c in tab
            cols = max(cols,length(c))
        var wid : array<int>
        resize(wid,cols)
        for cc in tab
            for x in range(cols)
                wid[x] = max(wid[x], length(cc[x]))
        for ccc,idx in tab,count()
            write_table_separator(writer,wid,(idx==1 && withHeader) ? "=" : "-")
            write_table_line(writer,ccc,wid)
        write_table_separator(writer,wid)
        write(writer,"\n\n")

def topic(category:string;mod:Module?;name:string implicit)
    return "|{category}-{module_name(mod)}-{name}|"

def topic(mod:Module?)
    return "|module-{module_name(mod)}|"

def document_topic(doc_file:file;topic:string implicit)
    let topic_file_name = "{topic_root}/{slice(topic,1,-1)}.rst"
    var st : FStat
    if stat(topic_file_name,st)
        fopen(topic_file_name,"rb") <| $ ( topic_file )
            if topic_file!=null
                let instxt = fread(topic_file)
                fwrite(doc_file,instxt)
                fwrite(doc_file,"\n")
            else
                panic("can't open {topic_file_name}")
    else
        fwrite(doc_file,"{topic}\n\n")

def document_bitfield(doc_file:file; value:TypeDeclPtr)
    var tab : array<array<string>>
    emplace(tab,[{string "field"; "bit"; "value"}])
    for an,bit in value.argNames,range(32)
        var line : array<string>
        push(line,string(an))
        push(line,string(bit))
        push(line,string(1 << bit))
        emplace(tab,line)
    fwrite(doc_file,make_table(tab,true))

def document_variant(doc_file:file; value:TypeDeclPtr)
    var tab : array<array<string>>
    for an,at in value.argNames,value.argTypes
        var line : array<string>
        push(line,string(an))
        push(line,describe_type(at))
        emplace(tab,line)
    fwrite(doc_file,make_table(tab))

def document_typedef(doc_file:file;mod:Module?;name:string implicit;value)
    fwrite(doc_file,make_label(alias_label_name(value)))
    fwrite(doc_file,make_domain("das:attribute"))
    fwrite(doc_file,name)
    if value.baseType==Type tBitfield
        fwrite(doc_file," is a bitfield\n\n")
        document_bitfield(doc_file,value)
    elif value.baseType==Type tVariant
        fwrite(doc_file," is a variant type\n\n")
        document_variant(doc_file,value)
    else
        var tp = describe(value,false,false,false)
        tp = replace(tp,"$::","")
        fwrite(doc_file," = {tp}\n\n")
    document_topic(doc_file,topic("typedef",mod,name))

def document_typedefs(doc_file:file;mod:Module?)
    var any = false
    for_each_typedef(mod) <| $(name, value)
        any = true
    if !any
        return
    fwrite(doc_file,make_group("Type aliases"))
    for_each_typedef(mod) <| $(name, value)
        document_typedef(doc_file,mod,name,value)

def document_global_constant(doc_file:file;mod:Module?;value:VariablePtr)
    fwrite(doc_file,make_label(global_label_name(value)))
    fwrite(doc_file,make_domain("das:attribute"))
    fwrite(doc_file,string(value.name))
    fwrite(doc_file," = {describe(value.init)}\n\n")
    document_topic(doc_file,topic("variable",mod,string(value.name)))

def private is_global_constant(value:VariablePtr)
    if value.flags.private_variable
        return false
    if !value._type.flags.constant
        return false
    if !value._type.isWorkhorseType
        return false
    if value.init==null
        return false
    return true

def document_global_constants(doc_file:file;mod:Module?)
    var any = false
    for_each_global(mod) <| $(value)
        if is_global_constant(value)
            any = true
    if !any
        return
    fwrite(doc_file,make_group("Constants"))
    for_each_global(mod) <| $(value)
        if is_global_constant(value)
            document_global_constant(doc_file,mod,value)

def document_enumeration(doc_file:file;mod:Module?;value)
    fwrite(doc_file,make_label(enum_label_name(value)))
    fwrite(doc_file,make_domain("das:attribute"))
    fwrite(doc_file,"{value.name}\n\n")
    var tab : array<array<string>>
    for en in value.list
        var line : array<string>
        push(line,string(en.name))
        push(line,describe(en.value))
        emplace(tab,line)
    fwrite(doc_file,make_table(tab))
    document_topic(doc_file,topic("enumeration",mod,string(value.name)))

def document_enumerations(doc_file:file;mod:Module?)
    var any = false
    for_each_enumeration(mod) <| $(value)
        if value.isPrivate
            return
        any = true
    if !any
        return
    fwrite(doc_file,make_group("Enumerations"))
    for_each_enumeration(mod) <| $(value)
        if value.isPrivate
            return
        document_enumeration(doc_file,mod,value)

def argument_needs_documenting ( tt )
    if tt.baseType == Type fakeContext
        return false
    if tt.baseType == Type fakeLineInfo
        return false
    return true

def document_function_declaration(doc_file:file;domain,func_name:string;
        argNames:array<string>;argTypes:array<TypeDecl?>;resType:TypeDeclPtr)
    fwrite(doc_file,make_domain(domain))
    fwrite(doc_file,"{func_name}(")
    for an,at,i in argNames,argTypes,count()
        if argument_needs_documenting(at)
            if i != 0
                fwrite(doc_file,"; ")
            fwrite(doc_file,"{an}: {describe_type_short(at)}")
    fwrite(doc_file,")")
    fwrite(doc_file,"\n\n")

def document_function_arguments(doc_file:file;argNames:array<string>;argTypes:array<TypeDecl?>)
    var tab : array<array<string>>
    emplace(tab,[{string "argument";"argument type"}])
    for an,at in argNames,argTypes
        if argument_needs_documenting(at)
            var line : array<string>
            push(line,an)
            push(line,describe_type(at))
            emplace(tab,line)
    if length(tab)!=1
        fwrite(doc_file,make_table(tab,true))

def document_class_method(doc_file:file;mod:Module?;value;fld)
    let argNames <- [{for arg in fld._type.argNames; string(arg)}]
    let argTypes <- [{for arg in fld._type.argTypes; get_ptr(arg)}]
    document_function_declaration(doc_file,"das:function","{value.name}.{fld.name}",
        argNames,argTypes,fld._type.firstType)
    if fld._type.firstType != null && !fld._type.firstType.isVoid
        fwrite(doc_file,"{fld.name} returns {describe_type(fld._type.firstType)}\n\n")
    if length(argTypes) > 1
        document_function_arguments(doc_file,argNames,argTypes)
    document_topic(doc_file,topic("method",mod,"{value.name}.{fld.name}"))

def document_classes(doc_file:file;mod:Module?)
    var any = false
    for_each_structure(mod) <| $(value)
        if value.flags.isClass && !value.flags.privateStructure
            any = true
    if !any
        return
    fwrite(doc_file,make_group("Classes"))
    for_each_structure(mod) <| $(value)
        if !value.flags.isClass || value.flags.privateStructure
            return
        fwrite(doc_file,make_label(struct_label_name(value)))
        fwrite(doc_file,make_domain("das:attribute"))
        fwrite(doc_file,string(value.name))
        if value.parent!=null
            fwrite(doc_file," : {value.parent.name}")
        fwrite(doc_file,"\n\n")
        document_topic(doc_file,topic("class",mod,string(value.name)))
        var w  = 0
        for fld in value.fields
            if !is_class_method(value,fld._type) && !(fld.flags.generated) && !(fld.flags.parentType)
                w = max(w,length(fld.name))
        if w != 0
            fwrite(doc_file,"it defines as follows\n\n")
            for fld in value.fields
                if !is_class_method(value,fld._type) && !(fld.flags.generated) && !(fld.flags.parentType) && (fld.name!="__rtti")
                    fwrite(doc_file,"  | {wide(string(fld.name),w)} : {describe_type(fld._type)}\n")
            fwrite(doc_file,"\n")
        for fld in value.fields
            if is_class_method(value,fld._type) && !(fld.flags.generated) && !(fld.flags.parentType) && (fld.flags.implemented)
                document_class_method(doc_file,mod,value,fld)

def document_structure(doc_file:file;mod:Module?;value:StructurePtr)
    fwrite(doc_file,make_label(struct_label_name(value)))
    fwrite(doc_file,make_domain("das:attribute"))
    fwrite(doc_file,"{value.name}\n\n")
    if value.parent!=null
        fwrite(doc_file," : {value.parent.name}")
    fwrite(doc_file,"\n\n")
    var tab : array<array<string>>
    for fld in value.fields
        var line : array<string>
        push(line,string(fld.name))
        push(line,describe_type(fld._type))
        emplace(tab,line)
    fwrite(doc_file,"{value.name} fields are\n\n")
    fwrite(doc_file,make_table(tab))
    document_topic(doc_file,topic("structure",mod,string(value.name)))

def document_structures(doc_file:file;mod:Module?)
    var any = false
    for_each_structure(mod) <| $(value)
        if value.flags.isClass || value.flags.generated || value.flags.privateStructure
            return
        any = true
    if !any
        return
    make_group("Structures")
    for_each_structure(mod) <| $(value)
        if value.flags.isClass || value.flags.generated || value.flags.privateStructure
            return
        document_structure(doc_file,mod,value)

def document_structure_annotation(doc_file:file;mod:Module?;value)
    fwrite(doc_file,make_label(handle_label_name(value)))
    fwrite(doc_file,make_domain("das:attribute"))
    fwrite(doc_file,"{value.name}\n\n")
    var any_fields = false
    var any_props = false
    var ann = unsafe(reinterpret<BasicStructureAnnotation?> value)
    for_each_field(*ann) <| $ ( name,cppName,xtype,offset)
        if offset != -1u
            any_fields = true
        else
            any_props = true
    if any_fields
        var tab : array<array<string>>
        for_each_field(*ann) <| $ ( name,cppName,xtype,offset)
            if offset != -1u
                var line : array<string>
                push(line,name)
                push(line,describe_type(xtype))
                emplace(tab,line)
        fwrite(doc_file,"{value.name} fields are\n\n")
        fwrite(doc_file,make_table(tab))
    if any_props
        var tab : array<array<string>>
        for_each_field(*ann) <| $ ( name,cppName,xtype,offset)
            if offset == -1u
                var line : array<string>
                push(line,name)
                push(line,describe_type(xtype))
                emplace(tab,line)
        fwrite(doc_file,"{value.name} properties are\n\n")
        fwrite(doc_file,make_table(tab))
    var props : array<array<string>>
    var inscope annT <- new [[TypeDecl() baseType=Type tHandle,
        annotation=unsafe(reinterpret<TypeAnnotation?> value)]]
    for_each_function(mod, "") <| $(func)
        if !func.moreFlags.propertyFunction
            return
        if func.arguments|>length != 1
            return
        let fna = string(func.name)
        if !fna|>starts_with(".`") || length(fna)<=2
            return
        if !is_same_type(func.arguments[0]._type,annT,RefMatters no,ConstMatters no,TemporaryMatters no)
            return
        var line : array<string>
        push(line,slice(fna,2))
        push(line,describe_type(func.result))
        emplace(props,line)
    if !empty(props)
        fwrite(doc_file,"{value.name} property operators are\n\n")
        fwrite(doc_file,make_table(props))
    document_topic(doc_file,topic("structure_annotation",mod,string(value.name)))

def document_structure_annotations(doc_file:file;mod:Module?)
    var any = false
    module_for_each_annotation(mod) <| $(value)
        if !value.isBasicStructureAnnotation
            return
        any = true
    if !any
        return
    fwrite(doc_file,make_group("Handled structures"))
    module_for_each_annotation(mod) <| $(value)
        if !value.isBasicStructureAnnotation
            return
        document_structure_annotation(doc_file,mod,value)

def document_function_annotation(doc_file:file;mod:Module?;value)
    fwrite(doc_file,make_label(handle_label_name(value)))
    fwrite(doc_file,make_domain("das:attribute"))
    fwrite(doc_file,"{value.name}\n\n")
    document_topic(doc_file,topic("function_annotation",mod,string(value.name)))

def document_function_annotations(doc_file:file;mod:Module?)
    var any = false
    module_for_each_annotation(mod) <| $(value)
        if !value.isFunctionAnnotation
            return
        any = true
    if !any
        return
    fwrite(doc_file,make_group("Function annotations"))
    module_for_each_annotation(mod) <| $(value)
        if !value.isFunctionAnnotation
            return
        document_function_annotation(doc_file,mod,value)

def document_annotation(doc_file:file;mod:Module?;value)
    fwrite(doc_file,make_label(handle_label_name(value)))
    fwrite(doc_file,make_domain("das:attribute"))
    fwrite(doc_file,"{value.name}\n\n")
    document_topic(doc_file,topic("any_annotation",mod,string(value.name)))

def document_annotations(doc_file:file;mod:Module?)
    var any = false
    module_for_each_annotation(mod) <| $(value)
        if !value.isTypeAnnotation || value.isBasicStructureAnnotation
            return
        any = true
    if !any
        return
    fwrite(doc_file,make_group("Handled types"))
    module_for_each_annotation(mod) <| $(value)
        if !value.isTypeAnnotation || value.isBasicStructureAnnotation
            return
        if string(value.name) |> starts_with("dasvector`")
            return
        document_annotation(doc_file,mod,value)

def document_structure_macro(doc_file:file;mod:Module?;value)
    fwrite(doc_file,make_label(handle_label_name(value)))
    fwrite(doc_file,make_domain("das:attribute"))
    fwrite(doc_file,"{value.name}\n\n")
    document_topic(doc_file,topic("structure_macro",mod,string(value.name)))

def document_structure_macros(doc_file:file;mod:Module?)
    var any = false
    module_for_each_annotation(mod) <| $(value)
        if value.isTypeAnnotation || value.isBasicStructureAnnotation || value.isFunctionAnnotation
            return
        any = true
    if !any
        return
    fwrite(doc_file,make_group("Structure macros"))
    module_for_each_annotation(mod) <| $(value)
        if value.isTypeAnnotation || value.isBasicStructureAnnotation || value.isFunctionAnnotation
            return
        document_structure_macro(doc_file,mod,value)

def document_call_macro(doc_file:file;mod:Module?;value:string implicit)
    fwrite(doc_file,make_label(call_macro_label_name(mod,value)))
    fwrite(doc_file,make_domain("das:attribute"))
    fwrite(doc_file,"{value}\n\n")
    document_topic(doc_file,topic("function_annotation",mod,value))

def document_call_macros(doc_file:file;mod:Module?)
    var any = false
    for_each_call_macro(mod) <| $(value)
        any = true
    if !any
        return
    fwrite(doc_file,make_group("Call macros"))
    for_each_call_macro(mod) <| $(value)
        document_call_macro(doc_file,mod,value)

def document_reader_macro(doc_file:file;mod:Module?;value:string implicit)
    fwrite(doc_file,make_label(call_macro_label_name(mod,value)))
    fwrite(doc_file,make_domain("das:attribute"))
    fwrite(doc_file,"{value}\n\n")
    document_topic(doc_file,topic("reader_macro",mod,value))

def document_reader_macros(doc_file:file;mod:Module?)
    var any = false
    for_each_reader_macro(mod) <| $(value)
        any = true
    if !any
        return
    fwrite(doc_file,make_group("Reader macros"))
    for_each_reader_macro(mod) <| $(value)
        document_reader_macro(doc_file,mod,value)

def document_variant_macro(doc_file:file;mod:Module?;value:string implicit)
    fwrite(doc_file,make_label(call_macro_label_name(mod,value)))
    fwrite(doc_file,make_domain("das:attribute"))
    fwrite(doc_file,"{value}\n\n")
    document_topic(doc_file,topic("variant_macro",mod,value))

def document_variant_macros(doc_file:file;mod:Module?)
    var any = false
    for_each_variant_macro(mod) <| $(value)
        any = true
    if !any
        return
    fwrite(doc_file,make_group("Variant macros"))
    for_each_variant_macro(mod) <| $(value)
        document_variant_macro(doc_file,mod,string(value.name))

def document_typeinfo_macro(doc_file:file;mod:Module?;value:string implicit)
    fwrite(doc_file,make_label(call_macro_label_name(mod,value)))
    fwrite(doc_file,make_domain("das:attribute"))
    fwrite(doc_file,"{value}\n\n")
    document_topic(doc_file,topic("typeinfo_macro",mod,value))

def document_typeinfo_macros(doc_file:file;mod:Module?)
    var any = false
    for_each_typeinfo_macro(mod) <| $(value)
        any = true
    if !any
        return
    fwrite(doc_file,make_group("Typeinfo macros"))
    for_each_typeinfo_macro(mod) <| $(value)
        document_typeinfo_macro(doc_file,mod,string(value.name))

def function_needs_documenting(func:FunctionPtr)
    if func.flags.generated
        return false
    if func.flags.isClassMethod
        return false
    if func.flags.privateFunction
        return false
    if func.fromGeneric != null
        return false
    if func.moreFlags.propertyFunction
        return false
    var isBuiltin = false
    peek(func._module.name) <| $(name)
        if name=="$"
            isBuiltin = true
    // skip builtin operators
    var isOperator = false
    if isBuiltin
        peek(func.name) <| $(name)
            let ch = character_at(name,0)
            if !is_alpha(ch) && !is_number(ch) && ch!='_'
                isOperator = true
    if isOperator
        return false
    // skip builtin type c'tors
    var isTypeConstructor = false
    if isBuiltin
        peek(func.name) <| $(name)
            if func.result!=null && func.result.isCtorType
                let tname = das_to_string(func.result.baseType)
                if name == tname
                    isTypeConstructor = true
    if isTypeConstructor
        return false
    // skip ones starting with _builtin or __builtin
    var isBuiltinName = false
    peek(func.name) <| $(name)
        if starts_with(name,"_builtin") || starts_with(name,"__builtin")
            isBuiltinName = true
    if isBuiltinName
        return false
    return true

def document_warning(doc_file:file;issue:string)
    fwrite(doc_file,make_domain("warning"))
    fwrite(doc_file,"\n  {issue}\n\n")

def function_name ( name : string )
    if !empty(name)
        let ch = character_at(name,0)
        if ch!='_' && !is_alpha(ch)
            return "operator {name}"
    return name

def document_function(doc_file:file;mod:Module?;func:FunctionPtr)
    let argNames <- [{for arg in func.arguments; string(arg.name)}]
    let argTypes <- [{for arg in func.arguments; get_ptr(arg._type)}]
    fwrite(doc_file,make_label(function_label_name(func)))
    document_function_declaration(doc_file,"das:function",function_name(string(func.name)),
        argNames,argTypes,func.result)
    if func.result != null && !func.result.isVoid
        fwrite(doc_file,"{func.name} returns {describe_type(func.result)}\n\n")
    if func.flags.unsafeOperation
        document_warning(doc_file,"This is unsafe operation.")
    if func.moreFlags.deprecated
        document_warning(doc_file,"This function is deprecated.")
    document_function_arguments(doc_file,argNames,argTypes)
    document_topic(doc_file,topic("function",mod,"{func.name}"))

def document_functions(doc_file:file;mod:Module?;groups:array<DocGroup>)
    var tab : table<Function?;bool>
    for grp in groups
        if length(grp.func) != 0
            if !show_hidden_groups && grp.hidden
                for func in grp.func
                    tab[func] = true
            else
                fwrite(doc_file,make_group(grp.name))
                if grp.hidden
                    document_warning(doc_file,"This group of functions is hidden. It will not be in the final documentation.")
                for func in grp.func
                    if !key_exists(tab,func)
                        let descr = describe_function_short(FunctionPtr(func))
                        var labn = function_label_name(FunctionPtr(func))
                        fwrite(doc_file,"  * {make_ref(labn,descr)}\n")
                fwrite(doc_file,"\n")
                for func in grp.func
                    if tab[func]
                        continue
                    tab[func] = true
                    document_function(doc_file,mod,FunctionPtr(func))
    var first_function = true
    for_each_function(mod, "") <| $(func)
        if !function_needs_documenting(func)
            return
        var pfunc = get_ptr(func)
        if tab[pfunc]
            return
        tab[pfunc] = true
        if first_function
            fwrite(doc_file,make_group("Uncategorized"))
            first_function = false
        document_function(doc_file,mod,func)
    for_each_generic(mod) <| $(func)
        if !function_needs_documenting(func)
            return
        var pfunc = get_ptr(func)
        if tab[pfunc]
            return
        tab[pfunc] = true
        if first_function
            fwrite(doc_file,make_group("Uncategorized"))
            first_function = false
        document_function(doc_file,mod,func)

struct public DocGroup
    //! Group of functions with shared category.
    name : string
    func : array<Function?>
    hidden : bool

def debug_group(var group:DocGroup)
    to_log(LOG_DEBUG, "Group {group.name}\n")
    for func in group.func
        to_log(LOG_DEBUG, "\t{describe_function_short(FunctionPtr(func))}\n")
    return <- group

def append_to_group_by_regex(var group:DocGroup; mod:Module?; var reg:Regex)
    for_each_function(mod, "") <| $(func)
        if function_needs_documenting(func)
            if regex_match(reg,string(func.name)) != -1
                push(group.func,get_ptr(func))
    for_each_generic(mod) <| $(func)
        if function_needs_documenting(func)
            if regex_match(reg,string(func.name)) != -1
                push(group.func,get_ptr(func))

def public group_by_regex(name:string; mod:Module?; var reg:Regex)
    //! Creates a group of functions with shared category.
    //! Functions will be added to the group if they match the regular expression.
    var group <- [[DocGroup name=name, hidden=false]]
    append_to_group_by_regex(group,mod,reg)
    return <- group

def public hide_group(var group:DocGroup)
    //! Marks the group as hidden.
    group.hidden = true
    return <- group

def private generate_topic_stub(var tab:table<string;bool>;doc_file:file;topic:string implicit)
    if !tab[topic]
        tab[topic] = true
        fwrite(doc_file,".. {topic} replace:: to be documented in {topic}.rst\n\n" )

def private generate_substitute_stub(mod:Module?;substname:string)
    var st : FStat
    if stat(substname,st)   // this is actually 'see if it exists'
        return
    fopen(substname,"wb") <| $ ( stub_file )
        if stub_file!=null
            var tab:table<string;bool>
            // typedefs
            for_each_typedef(mod) <| $(name, value)
                generate_topic_stub(tab,stub_file,topic("typedef",mod,name))
            // enumerations
            for_each_enumeration(mod) <| $(value)
                generate_topic_stub(tab,stub_file,topic("enumeration",mod,string(value.name)))
            // structure everything
            for_each_structure(mod) <| $(value)
                if  value.flags.isClass
                    // classes and methods
                    generate_topic_stub(tab,stub_file,topic("class",mod,string(value.name)))
                    for fld in value.fields
                        if is_class_method(value,fld._type) && !(fld.flags.generated) && !(fld.flags.parentType) && (fld.flags.implemented)
                            generate_topic_stub(tab,stub_file,topic("method",mod,"{value.name}.{fld.name}"))
                elif !value.flags.generated
                    // structures
                    generate_topic_stub(tab,stub_file,topic("structure",mod,string(value.name)))
            for_each_function(mod, "") <| $(func)
                if function_needs_documenting(func)
                    generate_topic_stub(tab,stub_file,topic("function",mod,"{func.name}"))
            for_each_generic(mod) <| $(func)
                if function_needs_documenting(func)
                    generate_topic_stub(tab,stub_file,topic("function",mod,"{func.name}"))
            // annotations
            module_for_each_annotation(mod) <| $(value)
                if value.isBasicStructureAnnotation
                    generate_topic_stub(tab,stub_file,topic("structure_annotation",mod,string(value.name)))
                elif value.isTypeAnnotation
                    generate_topic_stub(tab,stub_file,topic("any_annotation",mod,string(value.name)))
            // variables
            for_each_global(mod) <| $(value)
                if is_global_constant(value)
                    generate_topic_stub(tab,stub_file,topic("variable",mod,string(value.name)))
        else
            panic("can't generate stub at {substname}\n")

def private getDetailIncludePath ( fname, substname : string )
    var fnp <- split_by_chars(fname,"\\/")
    var snp <- split_by_chars(substname,"\\/")
    var index = 0
    for f,s in fnp,snp
        if f != s
            break
        index ++
    unsafe
        var ipath = join ( [[ for i in range(index,length(snp)); snp[i] ]], "/" )
        return ipath

def public document(name:string;mod:Module?;fname,substname:string;groups:array<DocGroup>)
    //! Document single module given list of `DocGropus`.
    //! This will generate RST file with documentation for the module.
    //! Functions which do not match any `DocGroup` will be placed in the `Uncategorized` group.
    static_if log_documentation
        to_log(LOG_INFO, "Documenting {name} into {fname}\n")
    if mod==null
        panic("missing module {name}")
    var doc_file = fopen(fname,"wb")
    if doc_file == null
        panic("can't open {fname}")
    fwrite(doc_file,"\n")
    var mod_name = string(mod.name)
    if mod_name=="$"
        mod_name = "_builtin"
    fwrite(doc_file,make_header(name,mod_name))
    if length(substname)!=0
        fwrite(doc_file,".. include:: {getDetailIncludePath(fname,substname)}\n\n")
        generate_substitute_stub(mod,substname)
    document_topic(doc_file,topic(mod))
    document_typedefs(doc_file,mod)
    document_global_constants(doc_file,mod)
    document_enumerations(doc_file,mod)
    document_structures(doc_file,mod)
    document_structure_annotations(doc_file,mod)
    document_function_annotations(doc_file,mod)
    document_call_macros(doc_file,mod)
    document_reader_macros(doc_file,mod)
    document_variant_macros(doc_file,mod)
    document_typeinfo_macros(doc_file,mod)
    document_annotations(doc_file,mod)
    document_structure_macros(doc_file,mod)
    document_classes(doc_file,mod)
    document_functions(doc_file,mod,groups)
    fwrite(doc_file,"\n")
    fclose(doc_file)
