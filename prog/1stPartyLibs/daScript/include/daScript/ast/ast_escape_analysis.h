#pragma once

namespace das {

    // ===================================================================================================
    // Escape analysis & scope-free optimization
    // ===================================================================================================
    // daScript is a language for games, and in games the cardinal sin is the freeze: a frame that misses
    // its deadline because the runtime stopped to collect garbage. So we avoid GC pauses by not creating
    // the garbage in the first place - if the compiler can prove a heap allocation never outlives the
    // function that made it, it can free it deterministically at the right point (or keep it in the stack
    // frame and never touch the heap at all), instead of leaving it for a later collection.
    //
    // That proof is what this pass computes. It is the ONE genuinely complex optimization we run over the
    // AST - everything else is local rewriting; this reasons about how pointer values flow across calls,
    // branches, and loops. It comes in two strengths:
    //
    //   * flow-INsensitive: a whole-program graph solve. A pointer escapes if ANY use anywhere lets it
    //     outlive the function. Cheap, always on. Frees a local at scope exit / stack-allocates it when
    //     it provably never escapes.
    //
    //   * flow-SENSITIVE / partial (opt-in via force_partial_escape_free): builds a control-flow graph
    //     and asks the sharper question - on WHICH paths does it escape? An object returned on one branch
    //     but dropped on another is freed on the branch where it dies, even though the flow-insensitive
    //     verdict is "escapes". This is partial escape analysis (Stadler et al., CGO'14), approximated
    //     over the AST without SSA.
    //
    // The public surface is just the two steps below - prepare, then run. Everything else (the CFG
    // dataflow, liveness, free-site selection, AST insertion) is an implementation detail in the .cpp.
    // ===================================================================================================

    class Program;
    class TextWriter;

    // PREPARE - pure analysis: compute per-pointer escape and record it on the Variable escape flags. No
    // AST mutation; the flags are the reusable result. Returns true if any variable was marked.
    bool escapeAnalysis ( Program * program, TextWriter & logs );

    // RUN - consume the analysis result and mutate the AST: emit scope-exit frees + stack relocation, and
    // (when force_partial_escape_free is set) the flow-sensitive partial frees. Returns true if anything
    // changed (the caller must re-infer).
    bool scopeFreeOptimization ( Program * program, const struct ProgramCfg * pcfg, TextWriter & logs );

}
