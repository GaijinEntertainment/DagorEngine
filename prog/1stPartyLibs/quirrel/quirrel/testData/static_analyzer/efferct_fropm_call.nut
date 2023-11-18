


function foo(_p) {}
function bar(_p) {}
function qux() {}

let val = qux()?.z

bar(foo(val) ? val.tointeger() : 0)