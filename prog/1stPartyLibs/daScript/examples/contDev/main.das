// options logNodes=true, logStack=true

require fio
require rtti

let
    INDENT = "  "

[unsafe]
def findArg(field,argName,argType)  : AnnotationArgument const? const
    if field.annotation_arguments != null
        for arg in deref(field.annotation_arguments)
            if string(arg.name)==argName
                if arg.basicType==argType
                    return addr(arg)
                else
                    print("error: {field.name} {argName} argument must be {argType}\n")
    return null


def cppTypeAttr(field) : string
    let arg = findArg(field,"cppType",Type tString)
    return arg!=null ? string(arg.sValue) : ""

def isBaseCoreAttr(field)
    let arg = findArg(field,"skipCpp",Type tBool)
    return arg!=null

def cppBaseTypeName(bt)
    if bt==Type tInt
        return "int32_t"
    elif bt==Type tInt8
        return "int8_t"
    elif bt==Type tInt16
        return "int16_t"
    elif bt==Type tUInt8
        return "uint8_t"
    elif bt==Type tUInt16
        return "uint16_t"
    elif bt==Type tInt2
        return "int2"
    elif bt==Type tInt3
        return "int3"
    elif bt==Type tInt4
        return "int4"
    elif bt==Type tUInt
        return "uint32_t"
    elif bt==Type tFloat
        return "float"
    elif bt==Type tFloat2
        return "float2"
    elif bt==Type tFloat3
        return "float3"
    elif bt==Type tFloat4
        return "float4"
    elif bt==Type tString
        return "string"
    elif bt==Type tPointer
        return "void*"
    else
        return "/*undefined*/"

def cppTypeDim(info)
    var res = ""
    if info.dimSize !=0u
        for index in range(int(info.dimSize))
            let d = get_dim(info,index)
            res += "[{d}]"
    return res

def cppTypeName(info)
    var res = cppTypeAttr(info)
    if res == ""
        res = info.isConst ? "const " : ""
        if info.basicType==Type tHandle
            res += string(info.annotation_or_name.name)
        elif info.basicType==Type tStructure
            res += info.structType.name
        else
            res += cppBaseTypeName(info.basicType)
    return res

def dumpAnnotations(file:FILE const?;info)
    rtti::structure_for_each_annotation(info) <| $(ann,annArg)
        fprint(file,"// struct {info.name} annotation {ann.name}\n")
        for arg in annArg
            fprint(file,"{INDENT}// {arg.name} = ")
            if arg.basicType==Type tInt
                fprint(file,"{arg.iValue}")
            elif arg.basicType==Type tFloat
                fprint(file,"{arg.fValue}")
            elif arg.basicType==Type tBool
                fprint(file,"{arg.bValue}")
            elif arg.basicType==Type tString
                fprint(file,"{arg.sValue}")
            fprint(file,"\n")

def generateSquirrelBinding(file:FILE const?;info)
    if info.name  == "BaseCoreEvent"
        return
    let argsN = "{info.name}::NUMBER_OF_FIELDS"
    fprint(file,"{INDENT}\{\n")
    fprint(file,"{INDENT}{INDENT}Sqrat::DerivedClass<{info.name}, ecs::Event> sqEvent(vm, \"{info.name}\");\n")
    fprint(file,"{INDENT}{INDENT}sqEvent\n")
    fprint(file,"{INDENT}{INDENT}{INDENT}.Ctor<Sqrat::Object>()\n")
    fprint(file,"{INDENT}{INDENT}{INDENT}.Ctor<Sqrat::Object, Sqrat::Object>()\n")
    fprint(file,"{INDENT}{INDENT}{INDENT}.SquirrelFunc(\"_get\", das_evt_get<First>, 2, \"xi\")\n")
    fprint(file,"{INDENT}{INDENT}{INDENT}.SquirrelFunc(\"_in\", das_evt_in<{argsN}>, 2, \"xi\")\n")
    fprint(file,"{INDENT}{INDENT}{INDENT}.GlobalFunc(\"len\", das_evt_len<{argsN}>)\n")
    fprint(file,"{INDENT}{INDENT}{INDENT}.SquirrelFunc(\"_nexti\", das_evt_nexti<{argsN}>, 2, \"xi|o\")\n")
    for field in info
        if isBaseCoreAttr(field)
            continue
        fprint(file,"{INDENT}{INDENT}{INDENT}.Var(\"{field.name}\", &{info.name}::{field.name})\n")
    // fprint(file,"{INDENT}{INDENT}.Var(\"data\", &ecs::SQEvent::data)\n")
    fprint(file,"{INDENT}{INDENT};\n")
    fprint(file,"{INDENT}{INDENT}eventsBinding.emplace_back(\"{info.name}\", Sqrat::Object(sqEvent.GetObject(), vm));\n")
    fprint(file,"{INDENT}}\n\n")


def printArguments(file:FILE const?;info,prefix)
    var cArg = 0
    for field in info
        cArg++
        if isBaseCoreAttr(field)
            continue
        fprint(file,"{INDENT}{INDENT}{INDENT}{field.name}(eastl::move({prefix}{field.name}))")
        if cArg != int(info.count)
            fprint(file, ",\n")

def generateCppEvent(file:FILE const?;info)
    if info.name  == "BaseCoreEvent"
        return
    fprint(file,"class {info.name} final: public Event\n")
    fprint(file,"\{\n")
    fprint(file,"public:\n")
    var argCount = 0
    for field in info
        if !isBaseCoreAttr(field)
            argCount++
    fprint(file,"{INDENT}enum \{NUMBER_OF_FIELDS = {int(argCount)}\};\n")
    fprint(file,"{INDENT}{info.name}(")
    var first = true
    for field in info
        if isBaseCoreAttr(field)
            continue
        if first
            first = false
        else
            fprint(file,", ")
        fprint(file,"{cppTypeName(field)} &&_{field.name}");
    fprint(file,")\n{INDENT}{INDENT}: Event(ECS_HASH(\"{info.name}\"))")
    if argCount != 0
        fprint(file,",\n")
        printArguments(file, info, "_")
        fprint(file,"\n{INDENT}\{}\n");
    else
        fprint(file,"\{\}\n");
    for field in info
        if isBaseCoreAttr(field)
            continue
        fprint(file,"{INDENT}{cppTypeName(field)} {field.name}{cppTypeDim(field)};\n")
    if argCount != 0
        fprint(file,"{INDENT}Event *moveHeap()&& override\n{INDENT}\{\n{INDENT}{INDENT}return new {info.name}(\n");
        printArguments(file, info, "")
        fprint(file,"\n{INDENT}{INDENT});\n{INDENT}\}\n")
        fprint(file,"{INDENT}template<int n, class T> const T& get() const;\n");
        var count = 0
        for field in info
            if isBaseCoreAttr(field)
                continue
            fprint(file,"{INDENT}template<> const {cppTypeName(field)}{cppTypeDim(field)} & get<{count++}>() const \{ return {field.name}; \};\n");
    else
        fprint(file,"{INDENT}Event *moveHeap()&& override\{return new {info.name};\}\n");

    //fprint(file,"{INDENT}{INDENT}{INDENT}eastl::move(*this)");

    fprint(file,"\};\n\n")

def process(fname:string;hf,cppf:FILE const?)
    print("processing {fname}\n")
    rtti::compile_file(fname) <| $(ok,program,issues)
        if ok
            let thisModule = rtti::get_this_module(program)
            rtti::module_for_each_structure(thisModule) <| $(info)
                dumpAnnotations(hf,info)
                generateCppEvent(hf,info)
                generateSquirrelBinding(cppf,info)
        else
            print("{fname} failed to compile\n{issues}\n")

def generate(atdir)
    var fh, fcpp : FILE const?
    let eventListName = base_name(atdir)
    let EVENTS_HEADER_FILE = "./{eventListName}.h"
    let EVENTS_CPP_FILE = "./{eventListName}.cpp"

    fh = fopen(EVENTS_HEADER_FILE,"wb")
    if fh==null
        print("can't open {EVENTS_HEADER_FILE}\n")
        return
    fcpp = fopen(EVENTS_CPP_FILE,"wb")
    if fcpp==null
        print("can't open {EVENTS_CPP_FILE}\n")
        return
    // coreEvents.h header
    fprint(fh,"#ifndef _DAGOR_GAMELIB_DAECS_CORE_{to_upper(eventListName)}_H_\n")
    fprint(fh,"#define _DAGOR_GAMELIB_DAECS_CORE_{to_upper(eventListName)}_H_\n")
    fprint(fh,"#pragma once\n\n")
    fprint(fh,"#include <daECS/core/event.h>\n\n")
    fprint(fh,"namespace ecs\n")
    fprint(fh,"\{\n\n")

    // coreEvents.cpp header
    fprint(fcpp,"#include <daECS/core/event.h>\n")
    fprint(fcpp,"#include <ecs/scripts/sqBindEvent.h>\n")
    fprint(fcpp,"\nnamespace ecs\n")
    fprint(fcpp,"\{\n\n")
    fprint(fcpp,"void ecs_register_sq_das_binding_{eventListName}(HSQUIRRELVM vm)\n\{\n")
    fprint(fcpp,"{INDENT}ecs::sq::EventsBindingType eventsBinding;\n\n")

    dir(atdir) <| $ (fname)
        let fileName = atdir + "/" + fname
        var st : FStat
        if stat(fileName,st)
            if st.is_reg
                process(fileName,fh,fcpp)

    // coreEvent.h footer
    fprint(fh,"void ecs_register_sq_das_binding_{eventListName}(HSQUIRRELVM vm);\n\n");
    fprint(fh,"}\n\n")
    fprint(fh,"#endif\n\n")

    // coreEvents.cpp footer
    fprint(fcpp,"}\n\n}\n\n")
    fprint(fcpp,"#endif\n\n")


finally
    fclose(fh)
    fclose(fcpp)

[export]
def main
    let args <- get_command_line_arguments()
    let fn = args[1]
    let atdir = "./src"
    print("{fn}\ndas network message generator at {atdir}\n")
    let main_time = stat(fn).mtime
    var ttab : table<string;clock>
    while stat(fn).mtime == main_time
        var any = false
        dir(atdir) <| $ (fname)
            let fileName = atdir + "/" + fname
            var st : FStat
            if stat(fileName,st)
                if st.is_reg
                    let fmt = st.mtime
                    let tmt = ttab[fname]
                    if (tmt!=fmt)
                        ttab[fname] = fmt
                        process(fileName,fstdout(),fstdout())
                        any = true
            else
                print("can't stat {fileName}\n")
        if any
            generate(atdir)
        sleep(1000u)
    print("ok\n")
    return true

