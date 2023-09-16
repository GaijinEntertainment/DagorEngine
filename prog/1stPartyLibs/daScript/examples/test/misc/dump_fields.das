module dump_fields

require ast
require daslib/ast_boost
require fio

[structure_macro(name=dump_fields)]
class DumpFields : AstStructureAnnotation
    def override finish ( var st:StructurePtr; var group:ModuleGroup; args:AnnotationArgumentList; var errors : das_string ) : bool
        print("struct {st.name} \{\n")
        for field in st.fields
            print("\t{describe_cpp(field._type)} {field.name}")
            if field.init != null
                print(" = {describe(field.init)}")
            print(";\n")
        print("\}\n\n")
        fflush(fstdout())
        return true


