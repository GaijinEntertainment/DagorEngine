#include "daScript/daScript.h"

using namespace das;

#define TUTORIAL_NAME   "/examples/tutorial/tutorial01.das"

typedef das_map<string, float> ExprVars;    // variable lookup
class ExprCalc {
public:
    ExprCalc ( const string & expr, const ExprVars & vars );
    float compute(ExprVars & vars);
protected:
    ContextPtr          ctx;                // context
    SimFunction *       fni = nullptr;      // 'test' function
    string              text;               // program text (for reference purposes)
    das_map<string,float *> variables;      // pointers to variables in context global data
};

float ExprCalc::compute(ExprVars & vars) {
    if ( fni==nullptr ) {                           // if no function - no result
        return 0.0f;
    }
    for ( auto & va : vars ) {                      // set variables
        auto it = variables.find(va.first);
        if ( it != variables.end() ) {
            *(it->second) = va.second;
        }
    }
    auto res = ctx->evalWithCatch(fni, nullptr);    // eval the expression
    if ( auto ex = ctx->getException() ) {          // if exception - report it
        TextPrinter tout;
        tout << "with exception " << ex << "\n";
        return 0.0f;
    }
    return cast<float>::to(res);                    // cast result to float
}

ExprCalc::ExprCalc ( const string & expr, const ExprVars & vars ) {
    // build expression program
    TextWriter ss;
    ss  << "require math\n";
    for ( auto & va : vars ) {
        ss << "var " << va.first << " = 0.0f\n";    // make each variable float, set to 0.0f by default
    }
    ss  << "[export]\n"
        << "def test\n"
        << "\treturn " << expr << "\n"
        << "\n\n";
    text = ss.str();
    // make file access, introduce string as if it was a file
    TextPrinter tout;
    auto fAccess = make_smart<FsFileAccess>();
    auto fileInfo = make_unique<TextFileInfo>(text.c_str(), uint32_t(text.length()), false);
    fAccess->setFileInfo("dummy.das", das::move(fileInfo));
    // compile script
    ModuleGroup dummyLibGroup;
    auto program = compileDaScript("dummy.das", fAccess, tout, dummyLibGroup);
    if ( program->failed() ) {
        // if compilation failed, report errors
        tout << "failed to compile\n";
        for ( auto & err : program->errors ) {
            tout << reportError(err.at, err.what, err.extra, err.fixme, err.cerr );
        }
        return;
    }
    // create context
    ctx = make_shared<Context>(program->getContextStackSize());
    if ( !program->simulate(*ctx, tout) ) {
        // if interpretation failed, report errors
        tout << "failed to simulate\n";
        for ( auto & err : program->errors ) {
            tout << reportError(err.at, err.what, err.extra, err.fixme, err.cerr );
        }
        return;
    }
    // call context function
    fni = ctx->findFunction("test");
    // variables
    for ( auto & va : vars ) {
        auto idx = ctx->findVariable(va.first.c_str());             // find index of the variable
        assert(idx != -1);
        variables[va.first] = (float *) ctx->getVariable(idx);      // get its pointer in context
    }
}

int main( int, char * [] ) {
    // request all da-script built in modules
    NEED_ALL_DEFAULT_MODULES;
    // Initialize modules
    Module::Initialize();
    // make calculator 'program'
    ExprVars variables;
    variables["a"] = 1.0f;
    variables["b"] = 2.0f;
    variables["c"] = 3.0f;
    ExprCalc calc("a*b+c", variables);
    // verify initial values
    auto res = calc.compute(variables);
    assert(res==5.0f);
    // verify for the range of values
    for ( float a=1.0f; a<4.0f; a++ ) {
        variables["a"] = a;
        for ( float b=1.0f; b<5.0f; b++ ) {
            variables["b"] = b;
            for ( float c=1.0f; c<6.0f; c++ ) {
                variables["c"] = c;
                float rref = a * b + c;
                float reval = calc.compute(variables);
                assert(rref==reval);
            }
        }
    }
    // shut-down daScript, free all memory
    Module::Shutdown();
    return 0;
}

