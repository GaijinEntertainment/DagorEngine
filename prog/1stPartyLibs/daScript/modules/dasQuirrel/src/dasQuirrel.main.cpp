#include "daScript/misc/platform.h"

#include "daScript/ast/ast.h"
#include "daScript/ast/ast_interop.h"
#include "daScript/ast/ast_typefactory_bind.h"
#include "daScript/ast/ast_handle.h"

#include "dasQUIRREL.h"

#include "cb_dasQuirrel.h"

#include "dasQUIRREL.struct.decl.inc"

namespace das {

static SQInteger squirrel_print_args(HSQUIRRELVM v) {
    SQInteger nargs = sq_gettop(v); //number of arguments
    LOG tout(LogLevel::info);
    for(SQInteger n=1;n<=nargs;n++) {
        tout << "arg " << int(n) << " is ";
        switch(sq_gettype(v,n)) {
            case OT_NULL:           tout << "null";break;
            case OT_INTEGER:        tout << "integer";break;
            case OT_FLOAT:          tout << "float";break;
            case OT_STRING:         tout << "string";break;
            case OT_TABLE:          tout << "table";break;
            case OT_ARRAY:          tout << "array";break;
            case OT_USERDATA:       tout << "userdata";break;
            case OT_CLOSURE:        tout << "closure(function))";break;
            case OT_NATIVECLOSURE:  tout << "native closure(C function)";break;
            case OT_GENERATOR:      tout << "generator";break;
            case OT_USERPOINTER:    tout << "userpointer";break;
            case OT_CLASS:          tout << "class";break;
            case OT_INSTANCE:       tout << "instance";break;
            case OT_WEAKREF:        tout << "weak reference";break;
            default:                return sq_throwerror(v,"invalid param"); //throws an exception
        }
    }
    tout << "\n";
    sq_pushinteger(v,nargs); //push the number of arguments as return value
    return 1; //1 because 1 value is returned
}

static SQInteger squirrel_register_global_func(HSQUIRRELVM v,SQFUNCTION f,const char *fname) {
    sq_pushroottable(v);
    sq_pushstring(v,fname,-1);
    sq_newclosure(v,f,0); //create a new function
    sq_newslot(v,-3,SQFalse);
    sq_pop(v,1); //pops the root table
    return 0;
}

static void squirrel_print_function(HSQUIRRELVM, const SQChar *format, ...) {
    va_list args;
    char buffer[1024];
    va_start(args, format);
    vsprintf(buffer, format, args);
    va_end(args);
    LOG tout(LogLevel::info);
    tout.write(buffer);
    tout << "\n";
}

static void squirrel_error_function(HSQUIRRELVM, const SQChar *format, ...) {
    va_list args;
    char buffer[1024];
    va_start(args, format);
    vsprintf(buffer, format, args);
    va_end(args);
    LOG tout(LogLevel::error);
    tout.write(buffer);
    tout << "\n";
}

static void squirrel_compiler_error_function(HSQUIRRELVM,SQMessageSeverity sev,const SQChar * desc,const SQChar * source,SQInteger line,SQInteger column, const SQChar *) {
    auto level = LogLevel::error;
    if (sev == SEV_HINT) level = LogLevel::info;
    else if (sev == SEV_WARNING) level = LogLevel::warning;
    LOG tout(level);
    tout << "error: " << desc << " at " << source << "(" << int(line) << "," << int(column) << ")\n";
}

void sqdas_register(HSQUIRRELVM v) {
    sq_setprintfunc(v, squirrel_print_function, squirrel_error_function);
    sq_setcompilererrorhandler(v, squirrel_compiler_error_function);
    squirrel_register_global_func(v, squirrel_print_args, "print_args");
}

void Module_dasQUIRREL::initMain() {
    addConstant<uint64_t>(*this,"SQTrue",1ul);
    addConstant<uint64_t>(*this,"SQFalse",0ul);

    addExtern<DAS_BIND_FUN(sqdas_register)>(*this,lib,"sqdas_register",
        SideEffects::modifyExternal, "sqdas_register");

    // fixup module functions, so that there is a string cast
    for ( auto & pfn : this->functions.each() ) {
        bool anyString = false;
        for ( auto & arg : pfn->arguments ) {
            if ( arg->type->isString() && !arg->type->ref ) {
                anyString = true;
            }
        }
        if ( anyString ) {
            pfn->needStringCast = true;
        }
    }
}

ModuleAotType Module_dasQUIRREL::aotRequire ( TextWriter & tw ) const {
    tw << "#include \"../modules/dasQuirrel/src/aot_quirrel.h\"\n";
    return ModuleAotType::cpp;
}

}


