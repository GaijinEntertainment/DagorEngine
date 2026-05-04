function foo() {}
function bar(_a) {}

let { c = null} = foo()

if ("xx" in c) {
    bar(c.xx)  // No warning - "xx" in c implies c is non-null
}