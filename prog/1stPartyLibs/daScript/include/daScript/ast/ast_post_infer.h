#pragma once

namespace das {
    class Program;

    // Runs every [post_infer_macro] over the program. Called once inference is done and
    // again inside the optimizer, before the passes that read expr->type unguarded.
    void applyPostInferMacros ( Program * program );
}
