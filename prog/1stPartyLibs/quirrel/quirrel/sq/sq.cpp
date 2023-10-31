/*  see copyright notice in squirrel.h */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>
#include <ctype.h>
#include <string>

#if defined(_MSC_VER) && defined(_DEBUG)
#include <crtdbg.h>
#include <conio.h>
#endif
#include <squirrel.h>
#include <sqstdblob.h>
#include <sqstdsystem.h>
#include <sqstddatetime.h>
#include <sqstdio.h>
#include <sqstdmath.h>
#include <sqstdstring.h>
#include <sqstdaux.h>
#include <sqmodules.h>
#include <sqastio.h>
#include <sqstddebug.h>

#include <startup/dag_globalSettings.h>

#define scvprintf vfprintf


static SqModules *module_mgr = nullptr;


void PrintVersionInfos();

SQInteger quit(HSQUIRRELVM v)
{
    int *done;
    sq_getuserpointer(v,-1,(SQUserPointer*)&done);
    *done=1;
    return 0;
}

void printfunc(HSQUIRRELVM SQ_UNUSED_ARG(v),const SQChar *s,...)
{
    (void)v; /* UNUSED */
    va_list vl;
    va_start(vl, s);
    scvprintf(stdout, s, vl);
    va_end(vl);
}

static FILE *errorStream = stderr;

void errorfunc(HSQUIRRELVM SQ_UNUSED_ARG(v),const SQChar *s,...)
{
    (void)v; /* UNUSED */
    va_list vl;
    va_start(vl, s);
    scvprintf(errorStream, s, vl);
    va_end(vl);
}

void PrintVersionInfos()
{
    fprintf(stdout,_SC("%s %s (%d bits)\n"),SQUIRREL_VERSION,SQUIRREL_COPYRIGHT,((int)(sizeof(SQInteger)*8)));
}

void PrintUsage()
{
    fprintf(stderr,_SC("usage: sq <options> <scriptpath [args]>.\n")
        _SC("Available options are:\n")
        _SC("   -c               compiles the file to bytecode(default output 'out.cnut')\n")
        _SC("   -o               specifies output file for the -c option\n")
        _SC("   -ast-dump [file] dump AST into console or file if specified\n")
        _SC("   -absolute-path   use absolute path when print diangostics\n")
        _SC("   -bytecode-dump [file] dump SQ bytecode into console or file if specified\n")
        _SC("   -c               compiles only\n")
        _SC("   -diag-file file  write diagnostics into specified file")
        _SC("   -sa              enable static analyser")
        _SC("   --inverse-warnings flip diagnostic state")
        _SC("   --warnings-list  print all warnings and exit")
        _SC("   -W<num>          disable diagnostic by numeric id")
        _SC("   -<diagnostic_name> disable diagnostic by text id")
        _SC("   -optCH           enable Closure Hoisting Optimization\n")
        _SC("   -d               generates debug infos\n")
        _SC("   -v               displays version infos\n")
        _SC("   -h               prints help\n"));
}

struct DumpOptions {
  bool astDump;
  bool bytecodeDump;

  const char *astDumpFileName;
  const char *bytecodeDumpFileName;
};

static void dumpAst_callback(HSQUIRRELVM vm, SQCompilation::SqASTData *ast, void *opts)
{
    if (opts == NULL)
      return;
    DumpOptions *dumpOpt = (DumpOptions *)opts;
    if (dumpOpt->astDump)
    {
        if (dumpOpt->astDumpFileName)
        {
            FileOutputStream fos(dumpOpt->astDumpFileName);
            if (fos.valid())
            {
                sq_dumpast(vm, ast, &fos);
            }
            else
            {
                printf(_SC("Error: cannot open AST dump file '%s'\n"), dumpOpt->astDumpFileName);
            }
        }
        else
        {
            FileOutputStream fos(stdout);
            sq_dumpast(vm, ast, &fos);
        }
    }
}

static void dumpBytecodeAst_callback(HSQUIRRELVM vm, HSQOBJECT obj, void *opts)
{
    if (opts == NULL)
      return;
    DumpOptions *dumpOpt = (DumpOptions *)opts;
    if (dumpOpt->bytecodeDump)
    {
        if (dumpOpt->bytecodeDumpFileName)
        {
            FileOutputStream fos(dumpOpt->bytecodeDumpFileName);
            if (fos.valid())
            {
                sq_dumpbytecode(vm, obj, &fos);
            }
            else
            {
                printf(_SC("Error: cannot open Bytecode dump file '%s'\n"), dumpOpt->bytecodeDumpFileName);
            }
        }
        else
        {
            FileOutputStream fos(stdout);
            sq_dumpbytecode(vm, obj, &fos);
        }
    }
}

bool search_sqconfig(const char * initial_file_name, char *buffer, size_t bufferSize)
{
  if (!initial_file_name)
    return false;

  size_t curSize = bufferSize;
  char *ptr = buffer;
  const char * slash1 = strrchr(initial_file_name, '\\');
  const char * slash2 = strrchr(initial_file_name, '/');
  const char * slash = slash1 > slash2 ? slash1 : slash2;

  if (slash) {
    size_t prefixSize = slash - initial_file_name + 1;
    if (prefixSize > curSize)
      return false;

    memcpy(buffer, initial_file_name, prefixSize);
    curSize -= prefixSize;
    ptr += prefixSize;
  }

  static const char configName[] = ".sqconfig";
  const size_t configNameSize = (sizeof configName) - 1;
  static const char upDir[] = "../";
  const size_t upDirSize = (sizeof upDir) - 1;

  char *dirPtr = ptr;

  memcpy(ptr, configName, configNameSize);
  ptr += configNameSize;
  curSize -= configNameSize;
  ptr[0] = '\0';

  for (int i = 0; i < 16 && curSize > 0; i++) {
    if (FILE * f = fopen(buffer, "rt")) {
      fclose(f);
      return true;
    }

    if (curSize > upDirSize) {
      memcpy(dirPtr, upDir, upDirSize);
      dirPtr += upDirSize;
      curSize -= upDirSize;
      ptr += upDirSize;
      memcpy(dirPtr, configName, configNameSize);
      ptr[0] = '\0';
    }
    else {
      break;
    }
  }

  return false;
}

bool checkOption(char *argv[], int argc, const char *option, const char *&optArg) {
  optArg = nullptr;
  for (int i = 1; i< argc; ++i) {
    const char *arg = argv[i];

    if (arg[0] == '-' && strcmp(arg + 1, option) == 0) {
      if ((i + 1) < argc) {
        const char *arg2 = argv[i + 1];
        if (arg2[0] != '-') { // not next option
          optArg = arg2;
        }
      }
      return true;
    }
  }

  return false;
}

#define _INTERACTIVE 0
#define _DONE 2
#define _ERROR 3
//<<FIXME>> this func is a mess
int getargs(HSQUIRRELVM v,int argc, char* argv[],SQInteger *retval)
{
    assert(module_mgr != nullptr && "Module manager has to be existed");
    int i;
    const char *optArg = nullptr;
    DumpOptions dumpOpt = { 0 };
    FILE *diagFile = nullptr;
    int compiles_only = 0;
    bool static_analysis = checkOption(argv, argc, "sa", optArg); // TODO: refact ugly loop below using this function
    bool flip_warnigns = false;

    if (static_analysis) {
      sq_enablesyntaxwarnings();
    }

    char * output = NULL;
    *retval = 0;
    if(argc>1)
    {
        int arg=1,exitloop=0;

        while(arg < argc && !exitloop)
        {

            if(argv[arg][0]=='-')
            {
                switch(argv[arg][1])
                {
                case 'a':
                    if (strcmp("-ast-dump", argv[arg]) == 0)
                    {
                        dumpOpt.astDump = true;
                        if (((arg + 1) < argc) && argv[arg + 1][0] != '-')
                        {
                            dumpOpt.astDumpFileName = argv[++arg];
                        }
                    }
                    else if (strcmp("-absolute-path", argv[arg]) == 0)
                    {
                        module_mgr->compilationOptions.useAbsolutePath = true;
                    }
                    else {
                        goto unknown_opt;
                    }
                    break;
                case 'd': //DEBUG(debug infos)
                    if (strcmp("-d", argv[arg]) == 0)
                    {
                        module_mgr->compilationOptions.debugInfo = true;
                        //sq_enabledebuginfo(v,1);
                        sq_lineinfo_in_expressions(v, 1);
                    }
                    else if (strcmp("-diag-file", argv[arg]) == 0)
                    {
                        if (((arg + 1) < argc) && argv[arg + 1][0] != '-')
                        {
                            const char *fileName = argv[++arg];
                            diagFile = fopen(fileName, "wb");
                            if (diagFile == NULL)
                            {
                                printf(_SC("Cannot open diagnostic output file '%s'\n"), fileName);
                                return _ERROR;
                            }
                        }
                        else
                        {
                            printf(_SC("-diag-file option requires file name to be specified\n"));
                            return _ERROR;
                        }
                    }
                    else
                    {
                        goto unknown_opt;
                    }
                    break;
                case 'b':
                    if (strcmp("-bytecode-dump", argv[arg]) == 0)
                    {
                      dumpOpt.bytecodeDump = 1;
                      if (((arg + 1) < argc) && argv[arg + 1][0] != '-')
                      {
                          dumpOpt.bytecodeDumpFileName = argv[++arg];
                      }
                    }
                    else
                    {
                        goto unknown_opt;
                    }
                    break;
                case 'c':
                    compiles_only = 1;
                    break;
                case 'o':
                    if (strcmp("-optCH", argv[arg]) == 0) {
                        sq_setcompilationoption(v, CompilationOptions::CO_CLOSURE_HOISTING_OPT, true);
                        break;
                    } else if(arg < argc) {
                        arg++;
                        output = argv[arg];
                    }
                    break;
                case 'v':
                    PrintVersionInfos();
                    return _DONE;

                case 'h':
                    PrintVersionInfos();
                    PrintUsage();
                    return _DONE;
                case 's':
                    if (strcmp("-sa", argv[arg]) == 0) {
                        static_analysis = true;
                        break;
                    }
                case 'w':
                case 'W':
                    if (isdigit(argv[arg][2])) {
                      int id = atoi(&argv[arg][2]);
                      if (!sq_switchdiagnosticstate_i(id, false)) {
                        printf("Unknown warning ID %s\n", &argv[arg][2]);
                      }
                    }
                    break;
                case '-':
                    if (strcmp(argv[arg], "--inverse-warnings") == 0) {
                      flip_warnigns = true;
                    }
                    else if (strcmp(argv[arg], "--warnings-list") == 0) {
                      sq_printwarningslist(stdout);
                      return _DONE;
                    }
                    else {
                      goto unknown_opt;
                    }
                    break;
                default:
                    if (static_analysis && isalpha(argv[arg][1]) && sq_switchdiagnosticstate_t(argv[arg] + 1, false)) {
                      break;
                    }
                unknown_opt:
                    PrintVersionInfos();
                    printf(_SC("unknown prameter '-%c'\n"),argv[arg][1]);
                    PrintUsage();
                    *retval = -1;
                    return _ERROR;
                }
            }else break;
            arg++;
        }

        module_mgr->up_data = &dumpOpt;
        module_mgr->onAST_cb = &dumpAst_callback;
        module_mgr->onBytecode_cb = &dumpBytecodeAst_callback;

        if (flip_warnigns)
          sq_invertwarningsstate();

        module_mgr->compilationOptions.doStaticAnalysis = static_analysis;

        if (diagFile)
        {
          errorStream = diagFile;
        }

        // src file

        if(arg<argc) {
            const SQChar *filename=NULL;
            filename=argv[arg];

            if (static_analysis) {
              sq_resetanalyserconfig();
              char buffer[1024];
              if (search_sqconfig(filename, buffer, sizeof buffer)) {
                if (!sq_loadanalyserconfig(buffer)) {
                  fprintf(errorStream, "Cannot load .sqconfig file %s\n", buffer);
                  return _ERROR;
                }
              }
            }

            arg++;

            if (compiles_only) {
                if(SQ_SUCCEEDED(sqstd_loadfile(v,filename,SQTrue))){
                    const SQChar *outfile = _SC("out.cnut");
                    if(output) {
                        outfile = output;
                    }
                    if(SQ_SUCCEEDED(sqstd_writeclosuretofile(v,outfile)))
                        return _DONE;
                }
            }
            else {
                Sqrat::Object exports;
                std::string errMsg;
                int retCode = _DONE;
                if (!module_mgr->requireModule(filename, true, "__main__", exports, errMsg)) {
                    retCode = _ERROR;
                }

                if (retCode == _DONE && sq_isinteger(exports.o)) {
                    *retval = exports.o._unVal.nInteger;
                }

                if (static_analysis) {
                  sq_checkglobalnames(v);
                }

                if (retCode != _DONE) {
                  fprintf(stderr, _SC("Error [%s]\n"), errMsg.c_str());
                }

                return retCode;
            }

            //if this point is reached an error occurred
            {
                const SQChar *err;
                sq_getlasterror(v);
                if(SQ_SUCCEEDED(sq_getstring(v,-1,&err))) {
                    printf(_SC("Error [%s]\n"),err);
                    *retval = -2;
                    return _ERROR;
                }
            }

        }
    }

    return _INTERACTIVE;
}

void Interactive(HSQUIRRELVM v)
{

#define MAXINPUT 1024
    SQChar buffer[MAXINPUT];
    SQInteger blocks =0;
    SQInteger string=0;
    SQInteger retval=0;
    SQInteger done=0;
    PrintVersionInfos();

    sq_pushroottable(v);
    sq_pushstring(v,_SC("quit"),-1);
    sq_pushuserpointer(v,&done);
    sq_newclosure(v,quit,1);
    sq_setparamscheck(v,1,NULL);
    sq_newslot(v,-3,SQFalse);
    sq_pop(v,1);

    while (!done)
    {
        SQInteger i = 0;
        printf(_SC("\nsq>"));
        for(;;) {
            int c;
            if(done)return;
            c = getchar();
            if (c == _SC('\n')) {
                if (i>0 && buffer[i-1] == _SC('\\'))
                {
                    buffer[i-1] = _SC('\n');
                }
                else if(blocks==0)break;
                buffer[i++] = _SC('\n');
            }
            else if (c==_SC('}')) {blocks--; buffer[i++] = (SQChar)c;}
            else if(c==_SC('{') && !string){
                    blocks++;
                    buffer[i++] = (SQChar)c;
            }
            else if(c==_SC('"') || c==_SC('\'')){
                    string=!string;
                    buffer[i++] = (SQChar)c;
            }
            else if (i >= MAXINPUT-1) {
                fprintf(stderr, _SC("sq : input line too long\n"));
                break;
            }
            else{
                buffer[i++] = (SQChar)c;
            }
        }
        buffer[i] = _SC('\0');

        if(buffer[0]==_SC('=')){
            scsprintf(sq_getscratchpad(v,MAXINPUT),(size_t)MAXINPUT,_SC("return (%s)"),&buffer[1]);
            memcpy(buffer,sq_getscratchpad(v,-1),(strlen(sq_getscratchpad(v,-1))+1)*sizeof(SQChar));
            retval=1;
        }
        i=strlen(buffer);
        if(i>0){
            SQInteger oldtop=sq_gettop(v);
            if(SQ_SUCCEEDED(sq_compilebuffer(v,buffer,i,_SC("interactive console"),SQTrue))){
                sq_pushroottable(v);
                if(SQ_SUCCEEDED(sq_call(v,1,retval,SQTrue)) &&  retval){
                    printf(_SC("\n"));
                    sq_pushroottable(v);
                    sq_pushstring(v,_SC("print"),-1);
                    sq_get(v,-2);
                    sq_pushroottable(v);
                    sq_push(v,-4);
                    sq_call(v,2,SQFalse,SQTrue);
                    retval=0;
                    printf(_SC("\n"));
                }
            }

            sq_settop(v,oldtop);
        }
    }
}

//int main(int argc, char* argv[])
int DagorWinMain(bool /*debugmode*/)
{
    HSQUIRRELVM v;
    SQInteger retval = 0;

    v=sq_open(1024);
    sq_setprintfunc(v,printfunc,errorfunc);

    sq_pushroottable(v);

    sqstd_register_bloblib(v);
    sqstd_register_iolib(v);
    sqstd_register_systemlib(v);
    sqstd_register_datetimelib(v);
    sqstd_register_mathlib(v);
    sqstd_register_stringlib(v);
    sqstd_register_debuglib(v);

    //aux library
    //sets error handlers
    sqstd_seterrorhandlers(v);

    module_mgr = new SqModules(v);
    module_mgr->registerMathLib();
    module_mgr->registerStringLib();
    module_mgr->registerSystemLib();
    module_mgr->registerIoStreamLib();
    module_mgr->registerIoLib();
    module_mgr->registerDateTimeLib();
    module_mgr->registerDebugLib();

    sqstd_register_command_line_args(v, ::dgs_argc, ::dgs_argv);

    //gets arguments
    switch(getargs(v, ::dgs_argc, ::dgs_argv, &retval))
    {
    case _INTERACTIVE:
        Interactive(v);
        break;
    case _DONE:
    case _ERROR:
    default:
        break;
    }

    if (errorStream != stderr)
    {
      fclose(errorStream);
    }

    delete module_mgr;
    sq_close(v);

    return int(retval);
}
