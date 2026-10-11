options gen2
options indenting = 4

// compiled by test_make_array_const_values through compile_file with folding off, so the
// mixed literal keeps its ExprVar element instead of folding to a constant
let g_x = 5

let g_all_const <- array<int>(1, 2, 3)
let g_mixed <- array<int>(1, g_x, 3)
let g_fixed = fixed_array<uint>(1u, 2u, 3u)

[export]
def main() {
    print("{g_all_const} {g_mixed} {g_fixed}\n")
}
