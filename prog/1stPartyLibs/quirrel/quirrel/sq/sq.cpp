/*  see copyright notice in squirrel.h */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>
#include <ctype.h>
#include <string>
#include <vector>

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
#include <sqio.h>
#include <sqstddebug.h>
#include <compiler/sqtypeparser.h>

#define scvprintf vfprintf


static SqModules *module_mgr = nullptr;
static DefSqModulesFileAccess file_access;
static bool check_stack_mode = false;


void PrintVersionInfos();

void printfunc(HSQUIRRELVM SQ_UNUSED_ARG(v),const char *s,...)
{
    va_list vl;
    va_start(vl, s);
    scvprintf(stdout, s, vl);
    va_end(vl);
}

static FILE *errorStream = stderr;

void errorfunc(HSQUIRRELVM SQ_UNUSED_ARG(v),const char *s,...)
{
    va_list vl;
    va_start(vl, s);
    scvprintf(errorStream, s, vl);
    va_end(vl);
}

void PrintVersionInfos()
{
    fprintf(stdout,"%s %s (%d bits)\n",SQUIRREL_VERSION,SQUIRREL_COPYRIGHT,((int)(sizeof(SQInteger)*8)));
}

void PrintUsage()
{
    fprintf(stderr,"Usage: sq <scriptpath [args]> [options]\n"
        "Available options are:\n"
        "  -c                        compiles the file to bytecode (default output 'out.cnut')\n"
        "  -o <out-file>             specifies output file for the -c option\n"
        "  -ast-dump [out-file]      dump AST into console or file if specified\n"
        "  -nodes-location           print AST nodes locations\n"
        "  -absolute-path            use absolute path when print diangostics\n"
        "  -bytecode-dump [out-file] dump SQ bytecode into console or file if specified\n"
        "  -diag-file file           write diagnostics into specified file\n"
        "  -sa                       enable static analyzer\n"
        "  --check-stack             check stack after each script execution\n"
        "  --warnings-list           print all warnings and exit\n"
        "  --parse-types             parse function types from file\n"
        "  --D:<diagnostic-name>     disable diagnostic by text id\n"
        "  -optCH                    enable Closure Hoisting Optimization\n"
        "  -d                        generates debug infos\n"
        "  -v                        displays version\n"
        "  -h                        prints help\n");
}

static std::string read_file_ignoring_utf8bom(const char *filename)
{
    FILE *f = fopen(filename, "rb");
    if (!f)
        return std::string();

    int length = 0;
    fseek(f, 0, SEEK_END);
    length = ftell(f);
    fseek(f, 0, SEEK_SET);

    std::string result;
    result.resize(length + 1);
    size_t sz = fread(&result[0], 1, length, f);
    fclose(f);
    result[sz] = 0;

    static const unsigned char bom[] = {0xEF, 0xBB, 0xBF};
    if (sz >= 3 && memcmp(result.data(), bom, 3) == 0)
        result.erase(0, 3);

    return result;
}

static int fill_stack(HSQUIRRELVM v)
{
  if (!check_stack_mode)
    return 0;

  for (int i = 0; i < 8; i++)
    sq_pushinteger(v, i + 1000000);

  return sq_gettop(v);
}

static bool check_stack(HSQUIRRELVM v, int expected_top)
{
  if (!check_stack_mode)
    return true;

  int top = sq_gettop(v);
  if (top != expected_top)
  {
    fprintf(errorStream, "ERROR: Stack top is %d, expected %d\n", top, expected_top);
    return false;
  }

  for (int i = 0; i < 8; i++)
  {
    SQInteger val = -1;
    if (SQ_FAILED(sq_getinteger(v, -8 + i, &val)) || val != i + 1000000)
    {
      fprintf(errorStream, "ERROR: Stack value at %d is %d, expected %d\n", -8 + i, int(val), i + 1000000);
      return false;
    }
  }

  return true;
}

struct DumpOptions {
  bool astDump;
  bool bytecodeDump;
  bool nodesLocation;

  const char *astDumpFileName;
  const char *bytecodeDumpFileName;
};

static void dumpAst_callback(HSQUIRRELVM vm, SQCompilation::SqASTData *astData, void *opts)
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
                sq_dumpast(vm, astData, dumpOpt->nodesLocation, &fos);
            }
            else
            {
                printf("Error: cannot open AST dump file '%s'\n", dumpOpt->astDumpFileName);
            }
        }
        else
        {
            FileOutputStream fos(stdout);
            sq_dumpast(vm, astData, dumpOpt->nodesLocation, &fos);
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
                printf("Error: cannot open Bytecode dump file '%s'\n", dumpOpt->bytecodeDumpFileName);
            }
        }
        else
        {
            FileOutputStream fos(stdout);
            sq_dumpbytecode(vm, obj, &fos);
        }
    }
}

static bool search_sqconfig(const char * initial_file_name, char *buffer, size_t bufferSize)
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

static bool checkOption(char *argv[], int argc, const char *option, const char *&optArg) {
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

bool ends_with(const char * str, const char * suffix)
{
    const char * s = strstr(str, suffix);
    return s && s[strlen(suffix)] == 0;
}

static bool parse_types_from_file(HSQUIRRELVM sqvm, const char *filename)
{
    std::string code = read_file_ignoring_utf8bom(filename);
    if (code.empty())
        return false;

    const char *c = code.c_str();
    std::string line;
    int lineNum = 1;

    while (*c) {
        if (*c == '\n' || c[1] == 0) {
            if (c[1] == 0)
                line += *c;
            if (!line.empty()) {
                printf("%s\n", line.c_str());

                SQFunctionType t(_ss(sqvm));
                SQInteger errorPos;
                SQObjectPtr errorString;
                if (sq_parse_function_type_string(sqvm, line.c_str(), t, errorPos, errorString)) {
                    SQObjectPtr s = sq_stringify_function_type(sqvm, t);
                    printf("%s\n", _stringval(s));
                    printf("  functionName: %s\n", _stringval(t.functionName));
                    printf("  returnTypeMask: 0x%x\n", unsigned(t.returnTypeMask));
                    printf("  objectTypeMask: 0x%x\n", unsigned(t.objectTypeMask));
                    printf("  ellipsisArgTypeMask: 0x%x\n", unsigned(t.ellipsisArgTypeMask));
                    printf("  requiredArgs: %d\n", int(t.requiredArgs));
                    printf("  argCount: %d\n", int(t.argTypeMask.size()));
                    printf("  pure: %s\n", t.pure ? "true" : "false");
                    printf("  nodiscard: %s\n", t.nodiscard ? "true" : "false");
                    printf("\n");
                }
                else {
                    printf("ERROR: %s\n", _stringval(errorString));
                    printf("at %s:%d:%d\n\n", filename, lineNum, int(errorPos));
                }

                line.clear();
            }
            lineNum++;
        }
        else {
            if (*c != '\r')
                line += *c;
        }

        c++;
    }

    return true;
}



#define _DONE 2
#define _ERROR 3
//<<FIXME>> this func is a mess
int getargs(HSQUIRRELVM v,int argc, char* argv[],SQInteger *retval)
{
    assert(module_mgr != nullptr && "Module manager has to be initialized");
    const char *optArg = nullptr;
    DumpOptions dumpOpt = { 0 };
    FILE *diagFile = nullptr;
    int compiles_only = 0;
    bool static_analysis = checkOption(argv, argc, "sa", optArg); // TODO: refact ugly loop below using this function
    bool parse_types = false;

    if (static_analysis) {
      sq_enablesyntaxwarnings(true);
    }

    std::vector<std::string> listOfNutFiles;

    const char * output = NULL;
    *retval = 0;
    if(argc>1)
    {
        int index=1, exitloop=0;

        while(index < argc && !exitloop)
        {
            const char *arg = argv[index];
            if (arg[0] == '-' && arg[1] == '-')
                arg++;

            if (strcmp("-ast-dump", arg) == 0)
            {
                dumpOpt.astDump = true;
                if ((index + 1) < argc && argv[index + 1][0] != '-' && !ends_with(argv[index + 1], ".nut"))
                    dumpOpt.astDumpFileName = argv[++index];
            }
            else if (strcmp("-nodes-location", arg) == 0)
            {
                dumpOpt.nodesLocation = true;
            }
            else if (strcmp("-absolute-path", arg) == 0)
            {
                file_access.useAbsolutePath = true;
            }
            else if (strcmp("-parse-types", arg) == 0)
            {
                parse_types = true;
            }
            else if (strcmp("-d", arg) == 0)
            {
                sq_lineinfo_in_expressions(v, 1);
            }
            else if (strcmp("-diag-file", arg) == 0)
            {
                if (((index + 1) < argc) && argv[index + 1][0] != '-')
                {
                    const char *fileName = argv[++index];
                    diagFile = fopen(fileName, "wb");
                    if (diagFile == NULL)
                    {
                        printf("Cannot open diagnostic output file '%s'\n", fileName);
                        return _ERROR;
                    }
                }
                else
                {
                    printf("-diag-file option requires file name to be specified\n");
                    return _ERROR;
                }
            }
            else if (strcmp("-bytecode-dump", arg) == 0)
            {
              dumpOpt.bytecodeDump = 1;
              if (((index + 1) < argc) && argv[index + 1][0] != '-' && !ends_with(argv[index + 1], ".nut"))
              {
                  dumpOpt.bytecodeDumpFileName = argv[++index];
              }
            }
            else if (strcmp("-c", arg) == 0) {
                compiles_only = 1;
            }
            else if (strcmp("-o", arg) == 0) {
                index++;
                output = arg;
            }
            else if (strcmp("-check-stack", arg) == 0) {
                check_stack_mode = true;
            }
            else if (strcmp("-optCH", arg) == 0) {
                sq_setcompilationoption(v, CompilationOptions::CO_CLOSURE_HOISTING_OPT, true);
            }
            else if (strcmp("-v", arg) == 0 || strcmp("--version", arg) == 0) {
                PrintVersionInfos();
                return _DONE;
            }
            else if (strcmp("-h", arg) == 0 || strcmp("--help", arg) == 0) {
                PrintVersionInfos();
                PrintUsage();
                return _DONE;
            }
            else if (strcmp("-sa", arg) == 0) {
                static_analysis = true;
            }
            else if (static_analysis && strncmp("-D:", arg, 3) == 0) {
                if (!sq_setdiagnosticstatebyname(&arg[3], false))
                    printf("Unknown warning ID: '%s'\n", &arg[3]);
            }
            else if (strcmp(arg, "-warnings-list") == 0) {
                sq_printwarningslist(stdout);
                return _DONE;
            }
            else if (arg[0] == '-') {
                PrintVersionInfos();
                printf("unknown argument '%s'\n", arg);
                PrintUsage();
                *retval = -1;
                return _ERROR;
            }
            else {
                listOfNutFiles.emplace_back(std::string(arg));
            }

            index++;
        }

        module_mgr->up_data = &dumpOpt;
        module_mgr->onAST_cb = &dumpAst_callback;
        module_mgr->onBytecode_cb = &dumpBytecodeAst_callback;

        module_mgr->compilationOptions.doStaticAnalysis = static_analysis;

        if (static_analysis) {
          sq_setcompilationoption(v, CompilationOptions::CO_CLOSURE_HOISTING_OPT, false);
        }

        if (diagFile)
        {
          errorStream = diagFile;
        }

        // src file

        if (listOfNutFiles.empty()) {
            printf("Error: expected source file name\n");
            return _ERROR;
        }

        if (parse_types) {
            for (const std::string & fn : listOfNutFiles) {
                if (!parse_types_from_file(v, fn.c_str()))
                    return _ERROR;
            }
            return _DONE;
        }

        for (const std::string & fn : listOfNutFiles) {
            const char *filename=fn.c_str();

            if (static_analysis) {
              sq_resetanalyzerconfig();
              char buffer[1024];
              if (search_sqconfig(filename, buffer, sizeof buffer)) {
                if (!sq_loadanalyzerconfig(buffer)) {
                  fprintf(errorStream, "Cannot load .sqconfig file %s\n", buffer);
                  return _ERROR;
                }
              }
            }

            if (compiles_only) {
                if(SQ_SUCCEEDED(sqstd_loadfile(v,filename,SQTrue))){
                    const char *outfile = "out.cnut";
                    if(output) {
                        outfile = output;
                    }
                    if(SQ_SUCCEEDED(sqstd_writeclosuretofile(v,outfile)))
                        return _DONE;
                }
            }
            else {
                int top = fill_stack(v);

                Sqrat::Object exports;
                SqModules::string errMsg;
                int retCode = _DONE;

                if (!module_mgr->requireModule(filename, true, static_analysis ? SqModules::__analysis__ : SqModules::__main__, exports, errMsg)) {
                    retCode = _ERROR;
                }

                if (retCode == _DONE && sq_isinteger(exports.GetObject())) {
                    *retval = exports.GetObject()._unVal.nInteger;
                }

                if (static_analysis) {
                  sq_checkglobalnames(v);
                }

                if (retCode != _DONE) {
                  fprintf(stderr, "Error [%s]\n", errMsg.c_str());
                }

                if (!check_stack(v, top)) {
                    fprintf(stderr, "Stack check failed after execution of '%s'\n", filename);
                    *retval = -3;
                    retCode = _ERROR;
                }

                return retCode;
            }

            //if this point is reached an error occurred
            {
                const char *err;
                sq_getlasterror(v);
                if(SQ_SUCCEEDED(sq_getstring(v,-1,&err))) {
                    printf("Error [%s]\n",err);
                    *retval = -2;
                    return _ERROR;
                }
            }

        }
    }
    else
    {
        PrintUsage();
        *retval = -1;
        return _ERROR;
    }

    return _DONE;
}

int sq_interpreter_main(int argc, char* argv[])
{
    SQInteger retval = 0;

    HSQUIRRELVM v = sq_open(1024);
    sq_setprintfunc(v,printfunc,errorfunc);

    sqstd_seterrorhandlers(v);

    module_mgr = new SqModules(v, &file_access);
    module_mgr->registerMathLib();
    module_mgr->registerStringLib();
    module_mgr->registerSystemLib();
    module_mgr->registerIoStreamLib();
    module_mgr->registerIoLib();
    module_mgr->registerDateTimeLib();
    module_mgr->registerDebugLib();

    sqstd_register_command_line_args(v, argc, argv);

    extern void register_test_natives(SqModules *);
    register_test_natives(module_mgr);

    //gets arguments
    getargs(v, argc, argv, &retval);

    if (errorStream != stderr)
    {
      fclose(errorStream);
    }

    delete module_mgr;
    sq_close(v);

    return int(retval);
}

#ifndef SQ_EXCLUDE_DEFAULT_MAIN
int main(int argc, char* argv[])
{
    return sq_interpreter_main(argc, argv);
}
#endif

