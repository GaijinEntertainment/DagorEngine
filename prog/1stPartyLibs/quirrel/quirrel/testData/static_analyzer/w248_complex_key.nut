if (__name__ == "__analysis__")
  return


function foo() {}

function bar(_a, _b) {}
function qux(_a, _b) {}

let { a = null, h = null } = foo()

if (a?.x[h]) {
    bar(a.x[h], h.x)
}


qux(a.z, h.z)
