options debugger

require rtti
require fio
require debugapi
require daslib/strings_boost
require daslib/algorithm
require daslib/ast

class EvalDataWalker : DapiStackWalker
    W : StringBuilderWriter?
    def EvalDataWalker ( var wr:StringBuilderWriter )
        W = unsafe(addr(wr))
    def override onCallAOT ( pp:Prologue; fileName:string# ) : void
        *W |> write("// AOT {fileName}\n")
    def override onCallAt ( pp:Prologue; info:FuncInfo; at:LineInfo ) : void
        *W |> write("// {info.name} at {describe(at)}\n")
    def override onCall ( pp:Prologue; info:FuncInfo ) : void
        *W |> write("// {info.name}\n")
    def override onArgument ( info:FuncInfo; index:int; vinfo:VarInfo; arg:float4 ) : void
        if vinfo.flags.refType
            let parg = unsafe(reinterpret<void?> arg)
            *W |> write("\t\tlet {vinfo.name} : {describe(type_info(vinfo))} & = * reinterpret<{describe(type_info(vinfo))}?> {parg}ul // ref argument\n")
        elif vinfo.dimSize > 0u
            let parg = unsafe(reinterpret<void?> arg)
            *W |> write("\t\tlet {vinfo.name} : {describe(type_info(vinfo))} = * reinterpret<{describe(type_info(vinfo))}?> {parg}ul // ref argument\n")
        else
            *W |> write("\t\tlet {vinfo.name} : {describe(type_info(vinfo))} = ")
            if vinfo.basicType==Type tString
                *W |> write("\"")
                *W |> write(sprint_data(arg,type_info(vinfo),print_flags humanReadable))
                *W |> write("\"")
            else
                *W |> write(sprint_data(arg,type_info(vinfo),print_flags humanReadable))
            *W |> write("\n")
    def override onVariable ( inf:FuncInfo; vinfo:LocalVariableInfo; arg:void?; inScope:bool ) : void
        if inScope
            if arg != null
                if vinfo.flags.ref
                    unsafe
                        let pref = unsafe(reinterpret<void??> arg)
                        var cvinfo = vinfo
                        cvinfo.flags &= ~ TypeInfoFlags ref
                        *W |> write("\t\tlet {vinfo.name} : {describe(type_info(vinfo))} = * reinterpret<{describe(type_info(cvinfo))}?> {*pref}ul // already ref\n")
                elif vinfo.dimSize > 0u
                    *W |> write("\t\tlet {vinfo.name} : {describe(type_info(vinfo))} = * reinterpret<{describe(type_info(vinfo))}?> {arg}ul\n")
                else
                    *W |> write("\t\tlet {vinfo.name} : {describe(type_info(vinfo))} & = * reinterpret<{describe(type_info(vinfo))}?> {arg}ul\n")

def getStackVariableDefinitions ( var ctx:Context;at:LineInfo )
    let text = build_string <| $ ( st )
        var walker = new EvalDataWalker(st)
        var walker_adapter <- make_stack_walker(walker)
        walk_stack(walker_adapter,ctx,at)
        walker_adapter := null
        unsafe
            delete walker
    var lines <- split(text,"\n")
    lines |> pop
    reverse(lines)
    let res = join(lines,"\n")
    delete lines
    return res

def getGlobalVariableDefinitions ( var ctx:Context )
    let text = build_string <| $ ( st )
        let tot = get_total_variables(ctx)
        for vi in range(tot)
            let vinfo = unsafe(addr(get_variable_info(ctx,vi)))
            if vinfo.name |> starts_with("__")
                continue
            var arg = unsafe(get_context_global_variable(ctx,vinfo.name))
            if arg != null
                if vinfo.flags.ref
                    unsafe
                        let pref = unsafe(reinterpret<void??> arg)
                        var cvinfo := vinfo
                        // cvinfo.flags &= ~ TypeInfoFlags ref
                        st |> write("\t\tlet {vinfo.name} : {describe(type_info(*vinfo))} = * reinterpret<{describe(type_info(*cvinfo))}?> {*pref}ul // already ref\n")
                elif vinfo.dimSize > 0u
                    st |> write("\t\tlet {vinfo.name} : {describe(type_info(*vinfo))} = * reinterpret<{describe(type_info(*vinfo))}?> {arg}ul\n")
                else
                    st |> write("\t\tlet {vinfo.name} : {describe(type_info(*vinfo))} & = * reinterpret<{describe(type_info(*vinfo))}?> {arg}ul\n")
    return text

def getRequire ( var ctx:Context; var self_text:string& )
    let prr = unsafe(get_context_global_variable(ctx,"__rtti_require"))
    if prr != null
        let arq = * unsafe(reinterpret<string?> prr)
        var mrq <- split(arq," ")
        self_text := mrq[length(mrq)-1]
        mrq |> pop
        let text = build_string <| $ ( st )
            for q in mrq
                st |> write("require {q}\n")
        return text
    else
        return "// __rtti_require is missing. module needs to be compiled with `options debugger`\n"

def eval ( expr:string ) : tuple<ok:bool;result:string>
    let vars = getStackVariableDefinitions(this_context(),get_line_info())
    let gvars = getGlobalVariableDefinitions(this_context())
    var self_text : string
    let greq = getRequire(this_context(), self_text)
    let text = build_string <| $ ( st )
        st |> write("require rtti\n")       // TODO: list of 'require' for this eval
        st |> write("require strings\n")
        st |> write("require __current\n")
        st |> write(greq)
        st |> write("\n")
        st |> write("var G_EVAL_RESULT : string\n")
        st |> write("[export]\n")
        st |> write("def eval``call_eval\n")
        st |> write("\tunsafe\n")
        st |> write("// globals:\n")
        st |> write(gvars)
        st |> write("// locals:\n")
        st |> write(vars)
        st |> write("// EVAL:\n")
        st |> write("\t\tlet EVAL = {expr}\n")
        st |> write("\t\tG_EVAL_RESULT = \"\{EVAL\}\" // todo: JSON?\n")
        st |> write("\n")
    return compileAndRun(text, self_text)

def compileAndRun ( text, self_text:string ) : tuple<ok:bool;result:string>
    var failed = true
    var result = ""
    var access := make_file_access("")
    access |> set_file_source("__project", text)
    fopen(self_text, "rb") <| $(fr)
        if fr != null
            fmap(fr) <| $(data)
                access |> set_file_source("__current.das", string(data))
        else
            print("failed to open `{self_text}`\n")
    using <| $(var mg:ModuleGroup)
        using <| $(var cop:CodeOfPolicies)
            cop.allow_local_variable_shadowing = true
            compile_file("__project",access,unsafe(addr(mg)),cop) <| $(ok,program,errors)
                if ok
                    simulate(program) <| $ ( sok; context; serrors )
                        if sok
                            failed = false
                            try
                                unsafe
                                    context |> invoke_in_context("eval``call_eval")
                            recover
                                print("exception during eval\n{text}\n{context.last_exception}\n{describe(context.exceptionAt)}\n")
                                failed = true
                            if !failed
                                let prr = unsafe(get_context_global_variable(context,"G_EVAL_RESULT"))
                                if prr != null
                                    result = clone_string(* unsafe(reinterpret<string?> prr))
                        else
                            print("failed to simulate\n{text}\n{serrors}\n")
                else
                    print("failed to compile\n{text}\n{errors}\n")
    return [[auto !failed, result]]

var GLOB = 13

struct AAA
    b : int

var ccc : AAA[10]

[export]
def main
    unsafe
        var bbb : AAA[10]
        bbb[0].b = 1

        ccc[1].b = 2

        var LOC = 13
        LOC ++
        var GLOC & = LOC
        print("testing {GLOB}+{LOC}={GLOB+LOC} // {GLOC}\n")
        let res = eval("GLOB + LOC")
        if res.ok
            print("eval = {res.result}\n")
        else
            print("eval failed\n")

        var a = [[AAA b=13]]
        a.b ++
        let res2 = eval("a")
        if res2.ok
            print("eval = {res2.result}\n")
        else
            print("eval failed\n")

        let res3 = eval("bbb")
        if res3.ok
            print("eval = {res3.result}\n")
        else
            print("eval failed\n")

        let res4 = eval("ccc")
        if res4.ok
            print("eval = {res4.result}\n")
        else
            print("eval failed\n")
