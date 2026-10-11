// [tune_scope] version_of= validation — each bad declaration fails the annotation apply with
// error 20800 (macro failed to apply).
options gen2
expect 20800:3

require daslib/tune
require llvm/daslib/llvm_tune

// 1. spec is not module/CONST
[tune_scope(name = "bad_spec", version_of = "no_slash_here")]
struct private BadSpec {}

// 2. module not in the compiling program
[tune_scope(name = "bad_module", version_of = "no_such_module/SOMECONST")]
struct private BadModule {}

// 3. named const absent from a module that IS present
[tune_scope(name = "bad_const", version_of = "llvm_tune/NO_SUCH_CONST")]
struct private BadConst {}
