options gen2

module standalone_contexts

require fio
require rtti
require strings

require daslib/match
require daslib/strings_boost
require daslib/ast_boost
require daslib/templates_boost
require daslib/functional
require daslib/algorithm

require printer_flags_visitor
require aot_constants

options strict_smart_pointers = false

require ast_aot_cpp


struct StandaloneContextCfg {
    context_name : string;
    class_name : string;
    cpp_output_dir : string;
    cross_platform : bool
};



def getInitSemanticHash(ctx : Context) {
    /*
    const uint64_t fnv_prime = 1099511628211ul;
    uint64_t hash = globalsSize ^ sharedSize;
    for (int i = 0; i != totalVariables; ++i) {
        hash = (hash ^ (globalVariables[i].shared ? 13 : 17)) * fnv_prime;
        hash = (hash ^ globalVariables[i].mangledNameHash) * fnv_prime;
        hash = (hash ^ globalVariables[i].size) * fnv_prime;
        if (globalVariables[i].init) {
            hash = (hash ^ getSemanticHash(globalVariables[i].init, this)) * fnv_prime;
        }
    }
    return hash;
    */
}

def writeStandaloneContextMethods(var prog : ProgramPtr; var logs : StringBuilderWriter; prefix : string; declare_only : bool; cfg : StandaloneContextCfg) {
    let fnn = collectProgramUsedFunctions(prog, false, false);

    var coll = new BlockVariableCollector();

    for (fn in fnn) {
        if (!fn.flags.exports) continue;
        if (fn._module != prog.getThisModule) continue;
        if (declare_only) {
            write(logs, "    ");
        }
        write(logs, "auto {prefix}{aotFunctionName(string(fn.origin != null ? fn.origin.name : fn.name))} ( ");
    // describe arguments
        var vars : array<string>
        for (variable in fn.arguments) {
            if (variable._type.baseType == Type.tStructure && declare_only) {
                // It doesn't cover all cases, but anyway we'll get CE.
                panic("Structures is not allowed in standalone contexts arguments.")
            }
            let type_str = build_string() <| $(var wr) {
                if (isLocalVec(variable._type)) {
                    describeLocalCppType(unsafe(addr(wr)), variable._type, cfg.cross_platform);
                } else {
                    write(wr, describeCppType(variable._type, DescribeConfig(cross_platform = cfg.cross_platform)));
                }
            }
            vars.push("{type_str} {variable._type.isRefType ? "& " : ""}{coll.getVarName(variable)}")
        }
        let vars_str = join(vars, ", ");
        write(logs, "{vars_str}{!fn.arguments.empty() ? " " : ""}) -> ")
        if (fn.result.baseType == Type.tStructure && declare_only) {
            // It doesn't cover all cases, but anyway we'll get CE.
            panic("Structures is not allowed in standalone contexts return types.");
        }
        describeLocalCppType(unsafe(addr(logs)), fn.result, cfg.cross_platform, CpptSubstitureRef.no, CpptSkipConst.yes);
        if (declare_only) {
            write(logs, ";\n");
        } else {
            let args_str = (fn.arguments |> each()
                                        |> map(@(v : VariablePtr) { return coll.getVarName(v); })
                                        |> join(", "))
            let maybe_comma = args_str |> length() > 0 ? ", " : "";
            write(logs, " \{\n    return {aotFuncName(fn)}(this{maybe_comma}{args_str});\n\}\n\n");
        }
    }
}

def writeStandaloneCtor(cfg : StandaloneContextCfg; initFunctions : string; var tw : StringBuilderWriter, program : ProgramPtr) {
    let disableInit = program._options |> find_arg("no_init") ?as tBool ?? program.policies.no_init;
    var lookupVariableTable : array<Variable?>;
    if (program.totalVariables > 0) {
        program.get_ptr() |> for_each_module_no_order($(pm) {
            pm |> for_each_global($(pvar) {
                if (!pvar.flags.used)
                    return
                if (pvar.index < 0) {
                    panic("Internal compiler errors. Simulating variable which is not used{pvar.name}");
                    return
                }
                lookupVariableTable.push(pvar.get_ptr());
            });
        });
    }

    write(tw, "{cfg.class_name}::{cfg.class_name}() \{\n");
    /**
     * Same as in Program::simulate
     * However, here we should delay it to execution time.
     */
    write(tw, "    auto & context = *this;\n");
    write(tw, "    CodeOfPolicies policies;");
    write(tw, "    policies.debugger = {program.policies.debugger} /*policies.debugger*/;\n");
    write(tw, "    policies.persistent_heap = {program._options |> find_arg("persistent_heap") ?as tBool ?? program.policies.persistent_heap};\n");
    write(tw, "    policies.heap_size_hint = {program._options |> find_arg("heap_size_hint") ?as tUInt ?? program.policies.heap_size_hint :d};\n");
    write(tw, "    policies.string_heap_size_hint = {program._options |> find_arg("string_heap_size_hint") ?as tUInt ?? program.policies.string_heap_size_hint :d};\n");
    write(tw, "    context.setup({program.totalVariables}/*totalVariables*/, {program.globalStringHeapSize:d} /*globalStringHeapSize*/, policies, \{\});\n");

    write(tw, "     // start totalVariables\n");
    for (pvar in lookupVariableTable) {
        write(tw, "    InitGlobalVar(context, &context.globalVariables[{pvar.index}/*pvar->index*/], GlobalVarInfo(\"")
        write(tw, "{pvar.name}\", \"{pvar |> get_mangled_name()}\", {pvar._type.sizeOf}, {pvar.flags.global_shared}));\n");
    }
    write(tw, "     // end totalVariables\n\n");
    write(tw, "    context.allocateGlobalsAndShared();\n");
    write(tw, "    context.totalVariables = {program.totalVariables}/*totalVariables*/;\n");
    write(tw, "    context.functions = (SimFunction *) context.code->allocate( {program.totalFunctions}/*totalFunctions*/*sizeof(SimFunction) );\n");
    write(tw, "    context.totalFunctions = {program.totalFunctions}/*totalFunctions*/;\n");
    write(tw, "    bool anyPInvoke = false;\n");

    write(tw, "    if ( anyPInvoke || {program.policies.threadlock_context || program.policies.debugger}");
    write(tw, "/*(policies.threadlock_context || policies.debugger)*/ ) \{\n");
    write(tw, "        context.contextMutex = new recursive_mutex;\n");
    write(tw, "    }\n");
    write(tw, "    context.tabMnLookup = make_shared<das_hash_map<uint64_t,SimFunction *>>();\n");
    write(tw, "    context.tabMnLookup->clear();\n");

    // MSVC forbide arrays of size 0
    if (initFunctions |> length() != 0) {
        write(tw, "     // start totalFunctions\n");
        write(tw, "    struct FunctionStorage \{ int idx; FunctionInfo funcInfo; FuncInfo* debugInfo; \};\n");
        write(tw, "    FunctionStorage usedFunctions[] = \{\n");
        write(tw, "{initFunctions}");
        write(tw, "    \};\n");
        write(tw, "    // end totalFunctions\n");
        write(tw, "    vector<pair<uint64_t, SimFunction*>> id_to_funcs;\n");
        write(tw, "    for (const auto& [index, func_info, debug_info]: usedFunctions) \{\n");
        write(tw, "        InitAotFunction(context, &context.functions[index], func_info);\n");
        write(tw, "        context.functions[index].debugInfo = debug_info;\n");
        write(tw, "        (*context.tabMnLookup)[func_info.mnh] = context.functions + index;\n");
        write(tw, "        id_to_funcs.emplace_back(func_info.aotHash, &context.functions[index]);\n");
        write(tw, "        anyPInvoke |= func_info.pinvoke;\n");
        write(tw, "    \}\n");
    }

    write(tw, "    context.tabGMnLookup = make_shared<das_hash_map<uint64_t,uint32_t>>();\n");
    write(tw, "    context.tabGMnLookup->clear();\n");
    write(tw, "    for ( int i=0, is=context.totalVariables; i!=is; ++i ) \{\n");
    write(tw, "        auto mnh = context.globalVariables[i].mangledNameHash;\n");
    write(tw, "        (*context.tabGMnLookup)[mnh] = context.globalVariables[i].offset;\n");
    write(tw, "    \}\n");
    write(tw, "    context.tabAdLookup = make_shared<das_hash_map<uint64_t,uint64_t>>();\n");
    program.get_ptr() |> for_each_module_no_order($(pm) {
        pm |> for_each_annotation_ordered($(k; v) {
            write(tw, "    (*context.tabAdLookup)[{k : x}] = {v};\n");
        });
    });

    // aot init
    if (program.initSemanticHashWithDep != uint64(0)) {
        write(tw, "    \{\n");
        write(tw, "        auto it = getGlobalAotLibrary().find(0x{program.initSemanticHashWithDep:x}/*initSemanticHashWithDep*/);\n");
        write(tw, "        if ( it != getGlobalAotLibrary().end() ) \{\n");
        write(tw, "            (it->second)(context);\n");
        write(tw, "        \}\n");
        write(tw, "    \}\n");
    }

    if (initFunctions |> length() != 0) {
        write(tw, " FillFunction(context, getGlobalAotLibrary(), id_to_funcs);\n");
        write(tw, "    context.runInitScript();\n");
        write(tw, "\}\n");
    }
}

def writeStandaloneContext(var program : ProgramPtr, initFunctions : string, var header : StringBuilderWriter, var source : StringBuilderWriter; cfg : StandaloneContextCfg) {

    write(header, "\n\n");
    write(header, "class {cfg.class_name} : public Context \{\n");
    write(header, "public:\n");
    write(header, "    {cfg.class_name}();\n");
    writeStandaloneContextMethods(program, header, "", true, cfg);
    write(header, "\};\n");

    writeStandaloneContextMethods(program, source, "{cfg.class_name}::", false, cfg);
    writeStandaloneCtor(cfg, initFunctions, source, program);

    write(source, "#ifdef STANDALONE_CONTEXT_TESTS\n");
    write(source, "static Context * registerStandaloneTest ( ) \{\n");
    write(source, "    auto ctx = new StandaloneContext();\n");
    write(source, "    return ctx;\n");
    write(source, "\}\n");
    write(source, "StandaloneContextNode node(registerStandaloneTest);\n");
    write(source, "#endif\n");
}

class StandaloneContextGen : CppAot {

    def StandaloneContextGen(var program_ : ProgramPtr; var ss_ : StringBuilderWriter?, var coll : BlockVariableCollector?, cp : bool) {
        ss <- ss_;
        collector <- coll;
        program <- program_;
        cross_platform = cp;
        prologue = program._options |> find_arg("aot_prologue") ?as tBool ?? false;
        solidContext = program.policies.solid_context || (program._options |> find_arg("solid_context") ?as tBool ?? false);
        helper.helper.rtti = program._options  |> find_arg("rtti") ?as tBool ?? false;
        helper.cross_platform = cp;
    }

    used_functions : table<string>;

    def override visitGlobalLetVariableInit(arg : VariablePtr; var expr : ExpressionPtr) : ExpressionPtr {
        return expr; // todo: Is there any way to provide default implementation without binding from c++
    }

    def override visitGlobalLet(prog : ProgramPtr) {
        var globals : array<Variable?>;
        prog.get_ptr() |> for_each_module_no_order($(pm) {
            pm |> for_each_global($(pvar) {
                if (pvar.index < 0 || !pvar.flags.used) return ;
                if (pvar._module == prog.getThisModule) return ;
                globals.push(pvar.get_ptr());
            });
        });

        for (variable in globals) {
            assume smart_var_ptr = unsafe(reinterpret<VariablePtr>(variable));
            preVisitGlobalLetVariable(smart_var_ptr, false);
            if (variable.init != null) {
                preVisitGlobalLetVariableInit(smart_var_ptr, variable.init);
                visit_expression(variable.init, adapter);

                variable.init := visitGlobalLetVariableInit(smart_var_ptr, variable.init);
            }
            let varn <- visitGlobalLetVariable(smart_var_ptr, false);
        }

        tab --;
        write(*ss, "\}\n");
    }
    def override preVisitProgramBody(var prog : ProgramPtr, that : Module?) {
        // functions
        declarations = ss |> string_builder_str();
        ss |> string_builder_clear();
        write(*ss, "\n");
        // print forward declarations
        let fnn = collectProgramUsedFunctions(prog, false, false);
        var inline_fns : array<string>
        for (pfun in fnn) {
            let needInline = that == pfun._module;
            if (needInline) {
                inline_fns.push(describeCppFunc(unsafe(reinterpret<FunctionPtr>(pfun)), collector, true, needInline));
                used_functions.insert(aotFuncName(pfun));
            }
        }
        let sep = ";\n";
        let maybe_sem = inline_fns |> empty() ? "" : sep;
        write(*ss, "{inline_fns |> join(sep)}{maybe_sem}\n")
    }
    tw : StringBuilderWriter ?
};


def writeRegistration(var header : StringBuilderWriter;
                      var source : StringBuilderWriter;
                      initFunctions : string;
                      var program : ProgramPtr;
                      cfg : StandaloneContextCfg;
                      var context : Context) {
    write(source, "using namespace {program.thisNamespace};\n");
    dumpRegisterAot(unsafe(addr(source)), program, context, false, cfg.cross_platform);
    write(header, "namespace {cfg.context_name} \{\n");
    write(source, "namespace {cfg.context_name} \{\n");
    writeStandaloneContext(program, initFunctions, header, source, cfg);
    write(header, "\} // namespace {cfg.context_name}\n");
    write(source, "\} // namespace {cfg.context_name}\n");
}

/**
 * Adds debug info to AotDebugInfoHelper
 * @return String with initialization of all functions
 */
def GetFunctionInfo(pfun : Function?, info : string) {
    return build_string() <| $(tw) {
        let args = ["\"{pfun.name}\"",
                    "\"{pfun |> get_mangled_name()}\"",
                    "0x{pfun.getMangledNameHash:x}",
                    "0x{pfun.aotHash:x}",
                    "{pfun.totalStackSize:d}",
                    "{pfun.flags.unsafeOperation}",
                    "{pfun.flags.fastCall}",
                    "{pfun._module.moduleFlags.builtIn}",
                    "{pfun._module.moduleFlags.promoted}",
                    "{pfun.result.isRefType && !pfun.result.flags.ref}",
                    "{pfun.moreFlags.pinvoke}"]
        write(tw, "        \{{pfun.index}, FunctionInfo({args|>join(", ")}), &{info}\},\n");
    }
}

def addFunctionInfo(disableInit : bool; rtti : bool, fnn : array<Function?>, var helper : AotDebugInfoHelper?) {
    helper.helper.rtti = rtti;
    var lookupFunctionTable : array<tuple<Function?, FuncInfo?>>;
    for (pfun in fnn) {
        let info = helper.helper |> make_function_debug_info(pfun);
        lookupFunctionTable.push((pfun, info));
    }

    return build_string() <| $(tw) {
        for ((fun_info, info) in lookupFunctionTable) {
            write(tw, GetFunctionInfo(fun_info, funcInfoName(info)));
        }
    }
}


def genStandaloneSrc(var program : ProgramPtr;
                     var source : StringBuilderWriter; cfg : StandaloneContextCfg;
                     var coll : BlockVariableCollector?) {
    var initFunctions : string;
    let ctx_generated = build_string() <| $(tw) {
        let deps = build_string() <| $(tmp_writer) {
            var cpp_aot = new CppAot(program := program, collector = coll, ss = unsafe(addr(tmp_writer)), cross_platform = cfg.cross_platform);
            var inscope aot_adapter <- make_visitor(*cpp_aot)
            cpp_aot.adapter := aot_adapter
            dumpDependencies(program, cpp_aot);
        }
        write(tw, deps);
        build_string() <| $(tmp_writer) {
            var gen = new StandaloneContextGen(program, unsafe(addr(tmp_writer)), coll, cfg.cross_platform);
            var inscope adapter <- make_visitor(*gen)
            gen.adapter := adapter
            program |> visit_module(adapter, program.getThisModule);

            initFunctions = addFunctionInfo(program._options |> find_arg("no_init") ?as tBool ?? program.policies.no_init,
                                            program._options |> find_arg("rtti") ?as tBool ?? program.policies.rtti,
                                            collectProgramUsedFunctions(program, false, false),
                                            gen.helper);
            write(tw, gen.str());
        }
    }

    write(source, "namespace {program.thisNamespace} \{\n");
    write(source, "{ctx_generated}");
    write(source, "\} // namespace {program.thisNamespace}\n");
    return initFunctions
}

def runStandaloneVisitor(var program : ProgramPtr, modules : array<string>, var pctx : smart_ptr<Context>; cfg : StandaloneContextCfg) {
    assume context = *pctx;

    // mark prologue
    var pmarker = new PrologueMarker();
    var inscope adapter_p <- make_visitor(*pmarker)
    visit(program, adapter_p)

    setAotHashes(program, context);

    // now, for that AOT
    var flags = new SetPrinterFlags();
    var inscope flags_adapter <- make_visitor(*flags)
    visit(program, flags_adapter)

    var coll = new BlockVariableCollector();
    var inscope adapter_coll <- make_visitor(*coll)
    visit(program, adapter_coll)

    let mod = program.getThisModule;
    // if ( mod.isProperBuiltin() ) return true;
    let mod_name = mod.moduleFlags.promoted ? "" : string(mod.name);
    var source_content = ""
    let header_content = build_string() <| $(header) {
    source_content = build_string() <| $(source) {
        if (mod_name |> length() != 0) {
            write(header, "// Module {mod_name}\n");
        }
        write(header, "{AOT_INCLUDES}");

        if (mod_name |> length() != 0) {
            write(source, "// Module {mod_name}\n");
        }

        write(source, "#include \"daScript/simulate/standalone_ctx_utils.h\"\n");
        write(source, "{join(modules, "")}");
        write(source, "#include \"{mod.name.empty() ? cfg.context_name : string(mod.name)}.das.h\"\n\n");
        write(source, AOT_HEADERS);

        write(source, "namespace das \{\n");
        write(header, "namespace das \{\n");
            let initFunctions = genStandaloneSrc(program, source, cfg, coll);
            writeRegistration(header, source, initFunctions, program, cfg, context);
        write(source, "\} // namespace das\n");
        write(header, "\} // namespace das\n");

        write(source, AOT_FOOTER);

    }
    }

    // get the name of the current file from program?

    let cur_mod = mod.name.empty() ? cfg.context_name : string(mod.name);
    let outputFile = "{cfg.cpp_output_dir}/{cur_mod}.das";
    // if (mod.name.empty()) {
        fopen("{outputFile}.h", "wb") <| $(fw) {
            if (fw != null) {
                fwrite(fw, header_content)
            } else {
                print("Couldn't create output file {outputFile}.h")
            }
        }
    // }
    fopen("{outputFile}.cpp", "wb") <| $(fw) {
        if (fw != null) {
            fwrite(fw, source_content)
        } else {
            print("Couldn't create output file {outputFile}.cpp")
        }
    }
}

[export]
def standalone_aot(input : string; output_dir : string; isAotLib, cross_platform : bool, paranoid_validation : bool; cop : CodeOfPolicies) : string {
    let cfg = StandaloneContextCfg(context_name = "context",
                                   class_name = "Standalone",
                                   cpp_output_dir = output_dir,
                                   cross_platform = cross_platform)
    var result = "";
    using <| $(var mg : ModuleGroup) {
        var inscope access <- make_file_access("")
        compile_and_simulate(input, access, unsafe(addr(mg)), cop) <| $(var program : ProgramPtr; var pctx : smart_ptr<Context>) {
            result = build_string() <| $(writer) {
                // lets comment on required modules
                var modules_str : array<string>
                let (modules_str, noAotModule) = getRequiredModulesFor(program)
                if (program._options |> find_arg("no_aot") ?as tBool ?? false) {
                    panic("Standalone context called on non aot module {input}")
                } elif (noAotModule) {
                    panic("Standalone context called on non aot module {input}")
                } else {
                    runStandaloneVisitor(program, modules_str, pctx, cfg)
                }
            }
        }
    }
    return result
}
