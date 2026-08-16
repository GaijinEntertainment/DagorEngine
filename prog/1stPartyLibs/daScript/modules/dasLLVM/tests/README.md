# dasLLVM tune tests

These module-owned tests cover the `llvm_tune` permutation, manifest, policy, tuner,
and re-execution integration paths. They are intentionally outside the core `tests/jit_tests`
sweep because several cases spawn nested `daslang` processes and full JIT compilations.

Run the suite explicitly from the repository root:

```text
bin/Release/daslang.exe dastest/dastest.das -jit -- --timing-outliers 10 --test modules/dasLLVM/tests
```

Files ending in `_client`, `_client_lib`, or `_tuner` are child-process fixtures rather than
standalone test cases.
