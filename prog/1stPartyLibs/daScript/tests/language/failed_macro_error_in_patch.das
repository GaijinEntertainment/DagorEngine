// Regression: a macro_error raised during a [function_macro] patch() pass that ALSO mutates the
// AST (astChanged=true) must FAIL the compile. It used to be silently dropped -- the astChanged
// re-infer cleared program->errors at the top of the next infer pass before the error could
// surface, which is how a fail-closed Vulkan shader bind guard once shipped a no-op bind instead
// of a clean compile error. 50503 == runtime_macro (macro_error). See _helper_macro_patch_error.
options gen2
expect 50503

require _helper_macro_patch_error

[export, patch_error_after_mutate]
def target_fn() {
}

[export]
def main() {
}
