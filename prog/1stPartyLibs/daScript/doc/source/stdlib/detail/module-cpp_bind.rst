The cpp_bind module implements generation of C++ bindings for the daScript interfaces.

All functions and symbols are in "cpp_bind" module, use require to get access to it. ::

    require daslib/cpp_bind

For example, from tutorial04.das ::

    require fio
    require ast
    require daslib/cpp_bind
    [init]
    def generate_cpp_bindings
        let root = get_das_root() + "/examples/tutorial/"
        fopen(root + "tutorial04_gen.inc","wb") <| $ ( cpp_file )
            log_cpp_class_adapter(cpp_file, "TutorialBaseClass", typeinfo(ast_typedecl type<TutorialBaseClass>))
