options gen2
options auto_inline_functions = false   // lint fixtures assert SOURCE shapes; splices rewrite them
// LINT018: narrowing the size/position argument of a call that has a 64-bit overload
//
// Problem: int(n) truncates above 2^31, so the copy, compare, resize, or erase
// silently covers the wrong count. memcpy/memcmp carry uint / int64 / uint64
// size overloads; array resize / resize_no_init / resize_and_init / reserve /
// erase and table reserve carry int64 overloads - the value passes straight through.
//
// Bad pattern:
//   memcpy(dst, src, int(nbytes))   // nbytes : int64
//   arr |> resize(int(newSize))     // newSize : int64
//
// Good pattern:
//   memcpy(dst, src, nbytes)
//   arr |> resize(newSize)

expect 50503:10

require daslib/lint

// bad: int() narrowing of an int64 size (1 warning)
def bad_memcpy_int64(dst, src : void?; nbytes : int64) {
    unsafe(memcpy(dst, src, int(nbytes)))
}

// bad: int() narrowing of a uint64 size (1 warning)
def bad_memcpy_uint64(dst, src : void?; nbytes : uint64) {
    unsafe(memcpy(dst, src, int(nbytes)))
}

// bad: int() narrowing on memcmp (1 warning)
def bad_memcmp_int64(dst, src : void?; nbytes : int64) : int {
    return unsafe(memcmp(dst, src, int(nbytes)))
}

// bad: array resize with a narrowed int64 size (1 warning)
def bad_resize(var arr : array<float>; newSize : int64) {
    arr |> resize(int(newSize))
}

// bad: resize_no_init narrowing (1 warning)
def bad_resize_no_init(var arr : array<float>; newSize : int64) {
    unsafe(arr |> resize_no_init(int(newSize)))
}

// bad: resize_and_init narrowing (1 warning)
def bad_resize_and_init(var arr : array<float>; newSize : int64) {
    arr |> resize_and_init(int(newSize))
}

// bad: reserve narrowing a uint64 - the fix widens with int64(...) (1 warning)
def bad_reserve_uint64(var arr : array<float>; newSize : uint64) {
    arr |> reserve(int(newSize))
}

// bad: single-position erase narrowing (1 warning)
def bad_erase_at(var arr : array<float>; pos : int64) {
    arr |> erase(int(pos))
}

// bad: ranged erase - both bounds narrow, one warning per call (1 warning)
def bad_erase_range(var arr : array<float>; pos, cnt : int64) {
    arr |> erase(int(pos), int(cnt))
}

// bad: table reserve narrowing (1 warning)
def bad_table_reserve(var tab : table<int; int>; newSize : int64) {
    tab |> reserve(int(newSize))
}

// good: the 64-bit size goes straight to the 64-bit overload
def good_memcpy_int64(dst, src : void?; nbytes : int64) {
    unsafe(memcpy(dst, src, nbytes))
}

// good: an int-typed size needs no cast at all
def good_memcpy_int(dst, src : void?; nbytes : int) {
    unsafe(memcpy(dst, src, nbytes))
}

// good: int64 resize overload takes the value as-is
def good_resize_int64(var arr : array<float>; newSize : int64) {
    arr |> resize(newSize)
}

// good: float operand - a genuine conversion, not a 64-bit truncation
def good_resize_float(var arr : array<float>; f : float) {
    arr |> resize(int(f))
}

// good: compound argument - the cast is not the whole size expression
def good_resize_compound(var arr : array<float>; n : int64) {
    arr |> resize(int(n) + 1)
}

struct Widget {
    n : int
}

def resize(var w : Widget; sz : int) {
    w.n = sz
}

// good: non-container receiver - same gate that keeps string resize (int-only) silent
def good_user_resize(var w : Widget; n : int64) {
    w |> resize(int(n))
}
