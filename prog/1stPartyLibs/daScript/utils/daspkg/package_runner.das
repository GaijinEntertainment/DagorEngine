options gen2
options rtti
options indenting = 4
options no_unused_block_arguments = false
options no_unused_function_arguments = false
options no_global_variables = false
options strict_smart_pointers = true

module package_runner shared public

require daslib/debugger
require daslib/fio
require daslib/daspkg public

// Results from running a .das_package script

struct ResolveResult {
    tag : string
    branch : string
    source : string    // non-empty = redirect to different repo
}

struct PackageDependency {
    source : string
    version_constraint : string
}

struct PackageBuildInfo {
    command : string
    is_cmake : bool
    is_custom : bool
}


// Run the `package()` function of a .das_package and extract metadata
def run_das_package_meta(das_package_path : string; var meta : PackageMeta) : bool {
    var args : array<string>
    var ok = run_das_package(das_package_path, "package", args) <| $(ctx) {
        var p = unsafe(get_context_global_variable(ctx, "_pkg_meta"))
        if (p != null) {
            let src = unsafe(reinterpret<PackageMeta?>(p))
            meta.pkg_name = clone_string(src.pkg_name)
            meta.author = clone_string(src.author)
            meta.description = clone_string(src.description)
            meta.source = clone_string(src.source)
            meta.license = clone_string(src.license)
            meta.min_sdk = clone_string(src.min_sdk)
            for (t in src.tags) {
                meta.tags |> push_clone(clone_string(t))
            }
        }
    }
    return ok
}

// Run the `resolve()` function of a .das_package and extract download info
def run_das_package_resolve(das_package_path : string; sdk_version, version : string; var result : ResolveResult) : bool {
    var args <- [sdk_version, version]
    var ok = run_das_package(das_package_path, "resolve", args) <| $(ctx) {
        var p = unsafe(get_context_global_variable(ctx, "_download_ref"))
        if (p != null) {
            let ref = unsafe(reinterpret<string?>(p))
            result.tag = clone_string(*ref)
        }
        p = unsafe(get_context_global_variable(ctx, "_download_kind"))
        if (p != null) {
            let kind = *unsafe(reinterpret<DownloadKind?>(p))
            if (kind == DownloadKind.branch) {
                result.branch = result.tag
                result.tag = ""
            }
        }
        p = unsafe(get_context_global_variable(ctx, "_download_source"))
        if (p != null) {
            let src = unsafe(reinterpret<string?>(p))
            result.source = clone_string(*src)
        }
    }
    return ok
}

// Run the `dependencies()` function of a .das_package and extract deps
def run_das_package_deps(das_package_path : string; version : string; var deps : array<PackageDependency>) : bool {
    var args <- [version]
    var ok = run_das_package(das_package_path, "dependencies", args) <| $(ctx) {
        var p = unsafe(get_context_global_variable(ctx, "_dependencies"))
        if (p != null) {
            let arr = unsafe(reinterpret<array<Dependency>?>(p))
            for (d in *arr) {
                deps |> emplace(PackageDependency(source = clone_string(d.source), version_constraint = clone_string(d.version_constraint)))
            }
        }
    }
    return ok
}

// Run the `build()` function of a .das_package and extract build info
def run_das_package_build(das_package_path : string; var info : PackageBuildInfo) : bool {
    var args : array<string>
    var ok = run_das_package(das_package_path, "build", args) <| $(ctx) {
        var p = unsafe(get_context_global_variable(ctx, "_build_kind"))
        if (p != null) {
            let kind = *unsafe(reinterpret<BuildKind?>(p))
            info.is_cmake = kind == BuildKind.cmake
            info.is_custom = kind == BuildKind.custom
        }
        p = unsafe(get_context_global_variable(ctx, "_build_command"))
        if (p != null) {
            let cmd = unsafe(reinterpret<string?>(p))
            info.command = clone_string(*cmd)
        }
    }
    return ok
}

// Internal: compile, simulate, call function, read state
def private run_das_package(das_package_path : string; func_name : string; args : array<string>; blk : block<(ctx : smart_ptr<Context>) : void>) : bool {
    if (!fexist(das_package_path)) {
        print("Error: {das_package_path} not found\n")
        return false
    }
    var ok = false
    var inscope access <- make_file_access("")
    using() <| $(var mg : ModuleGroup) {
        using() <| $(var cop : CodeOfPolicies) {
            cop.threadlock_context = true
            cop.export_all = true
            compile_file(das_package_path, access, unsafe(addr(mg)), cop) <| $(compiled; program; issues) {
                if (!compiled) {
                    print("Error compiling {das_package_path}:\n{issues}\n")
                    return
                }
                simulate(program) <| $(simulated; ctx; serrors) {
                    if (!simulated) {
                        print("Error simulating {das_package_path}:\n{serrors}\n")
                        return
                    }
                    try {
                        unsafe {
                            let n = length(args)
                            if (n == 0) {
                                invoke_in_context(ctx, func_name)
                            } elif (n == 1) {
                                invoke_in_context(ctx, func_name, args[0])
                            } elif (n == 2) {
                                invoke_in_context(ctx, func_name, args[0], args[1])
                            }
                        }
                    } recover {
                        pass
                    }
                    invoke(blk, ctx)
                    ok = true
                }
            }
        }
    }
    return ok
}
