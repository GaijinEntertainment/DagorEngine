// [tune_scope] ordering - a scope that ships defaults must compile BEFORE the modules it covers: a
// covered kernel reads the profile at ITS compile, so a scope arriving after it would leave the
// kernel on its fallback while the startup guard reads the box as covered and never restarts.
options gen2
expect 20800:1

require daslib/tune
require llvm/daslib/llvm_tune
require llvm_tune_scope_early

[tune_scope(name = "late_probe", covers = "llvm_tune_scope_early", defaults = "./llvm_tune_profiles_defaults")]
struct private LateScope {}
