function t31({x}) { println("t31:", x) }
t31({x = 1})

function t32({x = 10}) { println("t32:", x) }
t32({})

function t33({x: int = 3, y}) { println("t33:", x, y) }
t33({y = 5})

function t34({x, y: float|null}) { println("t34:", x, y) }
t34({x = 1, y = null})

function t35({x = 1}, [y]) { println("t35:", x, y) }
t35({}, [2])

function t36({x: int = 1}, [y: int = 2]) { println("t36:", x, y) }
t36({}, [])

function t37({x, y}, [a, b]) { println("t37:", x, y, a, b) }
t37({x = 1, y = 2}, [3, 4])

function t38({x: int = 5}, [a, b: float = 2.2]) { println("t38:", x, a, b) }
t38({}, [1])

function t39({x = null}, [a = null]) { println("t39:", x, a) }
t39({}, [])

function t40({x}, _) { println("t40:", x) }
t40({x = 99}, [])

function t41(a, {x = 10}) { println("t41:", a, x) }
t41(5, {})

function t42({x = 10}, a = [1, 2, 3]) { println("t42:", a, x) }
t42({}, 5)

function t43({x = 10}, a, {y = 12}) { println("t43:", a, x, y) }
t43({x = 2, y = 3}, 5, {})

function t44({x = 10}, a, {y: int = 12}, ...) { println("t44:", a, x, y, vargv[0]) }
t44({}, 5, {e = 5}, 999)

function t45([x], a, {y: int = 12}, ...) { println("t45:", a, x, y, vargv[0]) }
t45([1], 5, {e = 5}, 999)


let k31 = @({x}) println("k31:", x)
k31({x = 1})

let k32 = @({x = 10}) println("k32:", x)
k32({})

let k33 = @({x: int = 3, y}) println("k33:", x, y)
k33({y = 5})

let k34 = @({x, y: float|null}) println("k34:", x, y)
k34({x = 1, y = null})

let k35 = @({x = 1}, [y]) println("k35:", x, y)
k35({}, [2])

let k36 = @({x: int = 1}, [y: int = 2]) println("k36:", x, y)
k36({}, [])

let k37 = @({x, y}, [a, b]) println("k37:", x, y, a, b)
k37({x = 1, y = 2}, [3, 4])

let k38 = @({x: int = 5}, [a, b: float = 2.2]) println("k38:", x, a, b)
k38({}, [1])

let k39 = @({x = null}, [a = null]) println("k39:", x, a)
k39({}, [])

let k40 = @({x}, _) println("k40:", x)
k40({x = 99}, [])

let k41 = @(a, {x = 10}) println("k41:", a, x)
k41(5, {})

let k42 = @({x = 10}, a = [1, 2, 3]) println("k42:", a, x)
k42({}, 5)

let k43 = @({x = 10}, a, {y = 12}) println("k43:", a, x, y)
k43({x = 2, y = 3}, 5, {})

let k44 = @({x = 10}, a, {y: int = 12}, ...) println("k44:", a, x, y, vargv[0])
k44({}, 5, {e = 5}, 999)

let k45 = @([x], a, {y: int = 12}, ...) println("k45:", a, x, y, vargv[0])
k45([1], 5, {e = 5}, 999)


//--------------------------------------------------
// N1: object destructuring inside a nested function
//--------------------------------------------------
function n1() {
  function inner({x = 10}) {
    println("n1:", x)
  }
  inner({})
}
n1()

//--------------------------------------------------
// N2: array destructuring inside a nested function
//--------------------------------------------------
function n2() {
  function inner([a, b = 2]) {
    println("n2:", a, b)
  }
  inner([1])
}
n2()

//--------------------------------------------------
// N3: closure + destructuring
//--------------------------------------------------
function n3() {
  let offset = 5
  function inner({x = 1}) {
    println("n3:", x + offset)
  }
  inner({})
}
n3()

//--------------------------------------------------
// N4: outer argument + inner destructuring
//--------------------------------------------------
function n4({x = 3}) {
  function inner([y = 4]) {
    println("n4:", x, y)
  }
  inner([])
}
n4({})

//--------------------------------------------------
// N5: name shadowing in a nested function
//--------------------------------------------------
function n5({x = 1}) {
  function inner({x = 10}) {
    println("n5:", x)
  }
  inner({})
}
n5({x = 2})

//--------------------------------------------------
// N6: types in a nested function
//--------------------------------------------------
function n6() {
  function inner({x: int = 7}, [y: int = 8]) {
    println("n6:", x, y)
  }
  inner({}, [])
}
n6()

//--------------------------------------------------
// N7: multiple levels of nested functions
//--------------------------------------------------
function n7() {
  function mid({x = 1}) {
    function inner([y = 2]) {
      println("n7:", x, y)
    }
    inner([])
  }
  mid({})
}
n7()

//--------------------------------------------------
// N8: passing destructured values further
//--------------------------------------------------
function n8() {
  function mid({x = 3}) {
    function inner(v) {
      println("n8:", v)
    }
    inner(x)
  }
  mid({})
}
n8()

//--------------------------------------------------
// N9: destructuring + null in a nested function
//--------------------------------------------------
function n9() {
  function inner({x: int|null = null}) {
    println("n9:", x)
  }
  inner({})
}
n9()

//--------------------------------------------------
// N10: destructuring + varargs in a nested function
//--------------------------------------------------
function n10() {
  function inner({x = 1}, ...) {
    println("n10:", x, vargv[0])
  }
  inner({}, 99)
}
n10()


function c1(
  cfg = {
    handlers = [
      function({x = 1}) {
        function inner({y = 2}) {
          println("c1:", x, y)
        }
        inner({})
      }
    ]
  }
) {
  cfg.handlers[0]({})
}
c1()


function c2(
  data = {
    make = function() {
      return function(
        opts = {
          run = function([a = 3, b = 4]) {
            function inner({x = 5}) {
              println("c2:", a, b, x)
            }
            inner({})
          }
        }
      ) {
        opts.run([])
      }
    }
  }
) {
  let f = data.make()
  f()
}
c2()


function e1(
  cfg = class {
    counter = 0
    function next({step = 1}) {
      this.counter += step
      return this.counter
    }
  }()
) {
  println("e1:", cfg.next({}), cfg.next({step = 2}))
}
e1()
e1()


function e2(
  box = {
    fns = [
      function({x = 1}) {
        return function([y = 2]) {
          println("e2:", x, y)
        }
      }
    ]
  }
) {
  let g = box.fns[0]({})
  g([])
}
e2()


function e3(
  state = {
    init = function() {
      return function({x = 10}, ...) {
        println("e3:", x, vargv.len())
      }
    }
  }
) {
  let f = state.init()
  f({}, 1, 2, 3)
}
e3()


function e4(
  make = function(
    cfg = {
      wrap = function(fn) {
        return function({x = 1}) {
          fn(x)
        }
      }
    }
  ) {
    return cfg.wrap(
      function(v) {
        println("e4:", v)
      }
    )
  }
) {
  let f = make()
  f({})
}
e4()


function e5(
  cfg = {
    a = function({x = 1}) {
      function b([y = 2]) {
        function c({z = 3}) {
          println("e5:", x, y, z)
        }
        c({})
      }
      b([])
    }
  }
) {
  cfg.a({})
}
e5()



function t2({a = {x = 0}}) {
  a.x++
  println("t2:", a.x)
}

t2({})
t2({})


function t3([arr = []]) {
  arr.append(1)
  println("t3:", arr.len())
}

t3([])
t3([])


function t4(
  [cfg: table = {
    count = 0,
    inc = function() {
      this.count++
      return this.count
    }
  }]
) {
  println("t4:", cfg.inc())
}

t4([])
t4([])

function t5(
  cfg = {
    make = function() {
      return function({x = {y = 0}}) {
        x.y++
        println("t5:", x.y)
      }
    }
  }
) {
  let f = cfg.make()
  f({})
}

t5()
t5()

