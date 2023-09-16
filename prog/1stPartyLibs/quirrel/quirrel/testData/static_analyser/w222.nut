//expect:w222

let function foo(a,x,y) { //-declared-never-used
  ::print(a[x < y])
}