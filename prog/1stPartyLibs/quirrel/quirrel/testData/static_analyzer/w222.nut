//expect:w222

function foo(a,x,y) { //-declared-never-used
  ::print(a[x < y])
}