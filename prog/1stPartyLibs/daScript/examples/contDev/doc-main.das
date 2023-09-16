// options logNodes=true, logStack=true
// options logCpp=true, onlyFastAot=true

require fio
require rtti


def getModuleDoc(modName:string)
    let pMod = rtti::get_module(modName)
    if pMod==null
        print("module {modName} not found\n")
        return
    rtti::module_for_each_structure(pMod) <| $(info)
        print("struct {info.name}\n")
    rtti::module_for_each_function(pMod) <| $(info)
        print("def {info.name}")
        if (info.flags & FUNCINFO_BUILTIN) == FUNCINFO_BUILTIN
            print(" // {info.cppName=="" ? "WARNING: no CPP name" : info.cppName}")
        print("\n")
    rtti::module_for_each_generic(pMod) <| $(info)
        print("def {info.name} // generic\n")

[export]
def main
    let args <- get_command_line_arguments()
    let fn = args[1]
    print("module documentatoin generator\n")
    // getModuleDoc("$")    // builtin
    getModuleDoc("fio")
    print("ok\n")
    let main_time = stat(fn).mtime
    while stat(fn).mtime == main_time
        sleep(1000u)
    return true

