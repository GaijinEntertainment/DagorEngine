#pragma once

namespace das {
    class Program;

    void applyPostRewriteMacros ( Program * program );

    // Runs every [post_compile_macro] over the program. Called at the end of
    // compileDaScript, once the module's gc root holds only live nodes, so a pass may
    // reason about what the finished AST still owns.
    void applyPostCompileMacros ( Program * program );
}
