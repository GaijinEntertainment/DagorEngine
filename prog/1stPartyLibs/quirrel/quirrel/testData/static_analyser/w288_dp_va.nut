
let function comp(...) {}

let function foo(_x, _y = 10, _z = 20) {}

let function bar(_x, _y, ...) {}

let FlowH = 30
let hflow = @(...) comp(FlowH, vargv)

foo(10, 20, 30, 40)
foo(10, 20, 30)
foo(10, 20)
foo(10)
foo()


bar()
bar(10)
bar(10, 20)
bar(10, 20, 30)
bar(10, 20, 30, 40)