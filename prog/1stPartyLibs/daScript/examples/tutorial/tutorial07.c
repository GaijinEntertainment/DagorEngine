#include <stdio.h>
#include <string.h>
#include <stddef.h>
#include <assert.h>

#include "daScript/daScriptC.h"

#define TUTORIAL_NAME   "/examples/tutorial/tutorial07.das"

void tutorial () {
    das_text_writer * tout = das_text_make_printer();               // output stream for all compiler messages (stdout. for stringstream use TextWriter)
    das_module_group * dummyLibGroup = das_modulegroup_make();      // module group for compiled program
    das_file_access * fAccess = das_fileaccess_make_default();      // default file access
    das_context * ctx = NULL;
    // get path to tutorial
    char fileName[256];
    das_get_root(fileName, sizeof(fileName));
    int charsLeft = ((int)sizeof(fileName)) - ((int)strlen(fileName)) - 1;
    assert(charsLeft>=0 && "fileName buffer is too small ");
    strncat ( fileName, TUTORIAL_NAME, (size_t)charsLeft);
    // compile program
    das_program * program = das_program_compile(fileName, fAccess, tout, dummyLibGroup);
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
    (void)ctx; (void)node;
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
    das_module * mod = das_module_create ("tutorial_07");
    das_module_group * lib = das_modulegroup_make();
    das_modulegroup_add_module(lib, mod);
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
    (void)argc; (void)argv;
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
