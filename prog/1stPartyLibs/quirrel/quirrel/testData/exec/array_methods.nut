let a = [1,2,3]
let Exception = class{
  e = null
  constructor(e){
    this.e=e
  }
}
function test(func, expected_res, name = null){
  let name_s = name ? $"'{name}': " : ""
  try{
    let res = func()
    assert(expected_res==res, @() $"{name_s}Expected {expected_res}, got {res}")
    println(res)
  }
  catch(e){
    assert(expected_res instanceof Exception, $"{name_s} expected result '{expected_res}', got Exception('{e}')}")
    assert(expected_res.e == null || expected_res.e==e, $"{name_s}expected Exception value = '{expected_res.e}', got '{e}'")
  }
}
try{
  test(@() a.contains(1), true, "a.contains(1)")
  test(@() a.hasindex(0), true, "a.hasindex(0)")
  test(@() a.hasindex(5), false)
  test(@() a.hasindex(-1), false)
  test(@() a.hasindex(a.len()-1), true, "a.hasindex(a.len()-1)")
  test(@() a.hasvalue(5), false, "a.hasvalue(5)")
  test(@() a.hasvalue(3), true)
  test(@() a.pop(), 3)
  test(@() a.insert(0,0)[1], 1)
  test(@() a.remove(0), 0)
  test(@() a.extend([5,6]).len(), 4)
  test(@() a.resize(7, -1).top(), -1)
  test(@() a.reverse().top(), 1)
  test(@() a.sort().top(), 6, "a.sort().top()")
  test(@() a.slice(0,3)[2], -1)
  test(@() a.clear().len(), 0)
  test(@() a.append(0, 1, 2).indexof(1), 1)
  test(@() a.findindex(@(v) v==-1), null)
  test(@() a.findindex(@(v) v%2!=0), 1)
  test(@() a.findvalue(@(v) v==1), 1)
  test(@() a.totable()[0], 0)
  test(@() a.replace_with([-1,-2,-3]).top(), -3)
  test(@() type(a.tostring())=="string", true)
  test(@() a.swap(0,2)[0], -3)
  test(@() [0,1].totable().swap(0,1)[0], 1)
  print("ok")
}
catch(e){
  println(e)
}

