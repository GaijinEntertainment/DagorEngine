function expectCompileError(label, src) {
    try {
        compilestring(src)
        println($"FAIL: {label} compiled")
    }
    catch (_) {
    }
}

function expectCompiles(label, src) {
    try {
        compilestring(src)
    }
    catch (e) {
        println($"FAIL: {label} failed: {e}")
    }
}

expectCompiles("max-codepoint", "return \"\\U0010FFFF\"")

expectCompileError("too-large-codepoint", "return \"\\U00110000\"")
expectCompileError("high-surrogate", "return \"\\uD800\"")
expectCompileError("low-surrogate", "return \"\\uDFFF\"")

println("unicode escape errors: OK")
