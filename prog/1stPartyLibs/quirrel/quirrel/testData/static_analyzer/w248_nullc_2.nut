let t = {}

let i = 10

let e = t.value?[i]
if ((e?.a ?? false) && (e?.b ?? true)) {
    e.foo()
}