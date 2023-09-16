function foo() {}
function bar(_a) {}

let { c = null} = foo()

if ("xx" in c) {
    bar(c.xx)
}