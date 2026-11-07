from "debug" import get_function_info_table

function [pure] add(a, b = 5) { return a + b }
let t = get_function_info_table(add)
println($"{t.functionName} native={t.native} requiredArgs={t.requiredArgs}")
println("t.argNames =", ", ".join(t.argNames))
println($"pure={t.pure}")

// array.insert is bound with a type mask, not a decl string, so it has no real
// parameter names
println("get_function_info_table([].insert).argNames =", ", ".join(get_function_info_table([].insert).argNames))
