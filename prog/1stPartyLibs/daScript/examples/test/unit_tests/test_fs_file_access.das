require rtti

[export]
def test
    let appfile = "{get_das_root()}/examples/test/hidden_test/_hidden_test.das"
    var inscope access <- make_file_access("")
    access |> add_file_access_root("hidden","{get_das_root()}/examples/test/hidden_module")
    using <| $(var mg:ModuleGroup)
        using <| $(var cop:CodeOfPolicies)
            compile_file(appfile,access,unsafe(addr(mg)),cop) <| $(ok,program,issues)
                if ok
                    print("ok")
                else
                    print("failed to compile\n{issues}n")
                    assert(false)
    return true
