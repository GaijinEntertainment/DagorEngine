#include <stdio.h>
#include <string.h>
#include <stddef.h>

#include "daScript/daScriptC.h"

const char * TUTORIAL_SOURCE_CODE =
"options log\n"
"require tutorial_07a\n"
"[export]\n"
"def test\n"
"    print(\"this tutorial implements C interfaces\\n\")\n"
"    das_c_func(13)\n"
"    var a : intarray\n"
"    for x in range(3)\n"
"        push(a,x)\n"
"    print(\"a = {a}\\n\")\n"
"    var fb : FooBar\n"
"    fb.foo = 1\n"
"    fb.bar = 2.0\n"
"    print(\"fb = {fb}\\n\")\n"
"    var fb2 : FooBar\n"
"    fb2 <- fb\n"
"    print(\"fb2 = {fb2} fb = {fb}\\n\")\n"
"    let t = OneTwo one\n"
"    print(\"t = {t} {OneTwo two}\\n\")\n"
;

void tutorial () {
    das_text_writer * tout = das_text_make_printer();               // output stream for all compiler messages (stdout. for stringstream use TextWriter)
    das_module_group * dummyLibGroup = das_modulegroup_make();      // module group for compiled program
    das_file_access * fAccess = das_fileaccess_make_default();      // default file access
    das_context * ctx = NULL;
    // compile program
    das_fileaccess_introduce_file(fAccess, "tutorial.das", TUTORIAL_SOURCE_CODE);
    das_program * program = das_program_compile("tutorial.das", fAccess, tout, dummyLibGroup);
    int err_count = das_program_err_count(program);
    if ( err_count ) {
        // if compilation failed, report errors
        das_text_output(tout, "failed to compile\n");
        for ( int ei=0; ei!=err_count; ++ei ) {
            das_error * error = das_program_get_error(program, ei);
            das_error_output(error, tout);
        }
        goto shutdown;
    }
    // create daScript context
    ctx = das_context_make(das_program_context_stack_size(program));
    if ( !das_program_simulate(program,ctx,tout) ) {
        // if interpretation failed, report errors
        das_text_output(tout, "failed to simulate\n");
        err_count = das_program_err_count(program);
        for ( int ei=0; ei!=err_count; ++ei ) {
            das_error * error = das_program_get_error(program, ei);
            das_error_output(error, tout);
        }

        goto shutdown;
    }
    // find function 'test' in the context
    das_function * fnTest = das_context_find_function(ctx,"test");
    if ( !fnTest ) {
        das_text_output(tout, "function 'test' not found\n");
        goto shutdown;
    }
    // evaluate 'test' function in the context
    das_context_eval_with_catch(ctx, fnTest, NULL);
    char * ex = das_context_get_exception(ctx);
    if ( ex!=NULL ) {
        das_text_output(tout, "exception: ");
        das_text_output(tout, ex);
        das_text_output(tout, "\n");
        goto shutdown;
    }
shutdown:;
    das_program_release(program);
    das_fileaccess_release(fAccess);
    das_modulegroup_release(dummyLibGroup);
    das_text_release(tout);
}

// this is test function
vec4f das_c_func ( das_context * ctx, das_node * node, vec4f * args ) {
    ctx; node;
    printf("from das_c_func(%i)\n", das_argument_int(args[0]));     // access argument
    return das_result_void();                                       // return result
}

typedef struct {
    int     foo;
    float   bar;
} FooBar;

typedef enum {
    OneTwo_one = 1,
    OneTwo_two = 2
} OneTwo;

das_module * register_module_tutorial_07() {
    // create module and library
    das_module * mod = das_module_create ("tutorial_07a");
    das_module_group * lib = das_modulegroup_make();
    // alias
    das_module_bind_alias (mod, lib, "intarray", "1<i>A" );
    // enumeration
    das_enumeration * en = das_enumeration_make("OneTwo", "OneTwo", 1);
    das_enumeration_add_value(en, "one", "OneTwo_one", OneTwo_one);
    das_enumeration_add_value(en, "two", "OneTwo_two", OneTwo_two);
    das_module_bind_enumeration(mod, en);
    // handled structure
    das_structure * st = das_structure_make(lib, "FooBar", "FooBar", sizeof(FooBar), _Alignof(FooBar));
    das_structure_add_field(st, mod, lib, "foo", "foo", offsetof(FooBar,foo), "i");
    das_structure_add_field(st, mod, lib, "bar", "bar", offsetof(FooBar,bar), "f");
    das_module_bind_structure(mod, st);
    // bind das_c_func
    das_module_bind_interop_function(mod, lib, &das_c_func, "das_c_func", "das_c_func", SIDEEFFECTS_modifyExternal, "v i");
    // cleanup
    das_modulegroup_release(lib);
    return mod;
}

int main( int argc, char ** argv ) {
    argc; argv;
    // Initialize all default modules
    das_initialize();
    // register modules
    register_module_tutorial_07();
    // run the tutorial
    tutorial();
    // shut-down daScript, free all memory
    das_shutdown();
    return 0;
}
