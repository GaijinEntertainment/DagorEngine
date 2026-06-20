options gen2
expect 50503

require daslib/with_boost

struct Outer {
    inner : array<int>
}

[export]
def main {
    var arr : array<Outer>
    // Refused at macro time: subexpr `arr[0].inner` contains a nested
    // ExprAt (`arr[0]`). The macro would only lock `inner`, leaving
    // `arr` mutable — pushing to `arr` from inside the body would
    // reallocate it and invalidate the `inner` ref.
    with_(arr[0].inner[0]) {
        print("body")
    }
}
