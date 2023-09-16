//expect:w281
local x = [1, 2]

let function fn(arr) {
  return (arr ?? []).extend(x)
//  ::y <- (arr ?? []).extend(x)
}

return fn

