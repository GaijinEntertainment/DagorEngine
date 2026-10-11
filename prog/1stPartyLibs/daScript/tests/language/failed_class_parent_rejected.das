// A class deriving from a class whose own declaration is rejected. The `class B : A` error
// returns before B gets its generated __rtti and __finalize fields, so `class C : B` - which is
// accepted - reaches field generation with a parent that carries neither.
options gen2
expect 30134

struct A;
class B : A;
class C : B;

[export]
def main {
}
