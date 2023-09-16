//expect:w222

let function foo(a,x,y) { //-declared-never-used
  local index = x < y
  print(a[index])
}