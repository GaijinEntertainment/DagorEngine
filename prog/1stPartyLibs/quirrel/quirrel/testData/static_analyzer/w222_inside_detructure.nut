function fn([v =
  function foo(a,x,y) {
    print(a[x < y])
  }
]) {
  return v
}

return fn
