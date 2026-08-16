// Regression for issue #3065 (fuzzer crash). Const folding used to descend into a
// `struct template`'s field initializer; the unresolved call in v2's init folded to
// a null `func`, crashing RunFolding::visit(ExprCall) in ast_const_folding.cpp
// (read access violation on expr->func->result). Fixed by giving RunFolding a
// canVisitStructure that returns !isTemplate, mirroring canVisitFunction.
//
// Must-not-crash test: with the fix the template is skipped during folding so the
// program compiles & runs; without it the compiler crashes (process dies). The
// foldable float4(...) in `ma` is what engages the folder, which then walked into
// v2's init. v2 is reduced from the #3065 fuzzer payload -- gnarly on purpose.
options gen2

class v {}

struct template v2{r = i((3 ? reinterpret<bitfield<>>(([],) < (0 > t)) as r * ([{for((v) in @@o[[]] > (2) > 5, (v)); (0[((3 > (k()) > new<a<v>>() - 0)) as  t8 ]) % (9) % (99) % o }
].) ?[ (T)] .. 2 : [  ]), (3));
}

[export]
def ma{
 return float4(uint(), 2, -9, 3);
}

[export]
def main {
}
