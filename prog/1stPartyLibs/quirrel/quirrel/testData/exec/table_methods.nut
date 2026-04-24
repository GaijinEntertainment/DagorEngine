let t = {a=1, b=2, c=3}
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
  test(@() t.len(), 3, "t.len()")
  test(@() t.rawget("a"), 1, "t.rawget('a')")
  test(@() t.rawget("x"), Exception("the index doesn't exist"), "t.rawget('x')")
  test(@() t.rawset("d", 4)["d"], 4, "t.rawset('d', 4)")
  test(@() t.rawdelete("d"), 4, "t.rawdelete('d')")
  test(@() t.rawin("b"), true, "t.rawin('b')")
  test(@() t.rawin("x"), false, "t.rawin('x')")
  test(@() t.map(@(v) v*2)["a"], 2, "t.map(@(v) v*2)")
  test(@() t.map(@(v, k) v*2+k.len())["a"], 3, "t.map(@(v,k) v*2 + k.len())")
  test(@() t.filter(@(v) v%2==1)["a"], 1, "t.filter(@(v) v%2==1)")
  test(@() t.reduce(@(acc,v,k) acc+v, 0), 6, "t.reduce(@(acc,k,v) acc+v, 0)")
  test(@() t.findindex(@(v) v==2), "b", "t.findindex(@(v) v==2)")
  test(@() t.findindex(@(v) v==5), null, "t.findindex(@(v) v==5)")
  test(@() t.findvalue(@(v) v==3), 3, "t.findvalue(@(v) v==3)")
  test(@() t.findvalue(@(v) v==5), null, "t.findvalue(@(v) v==5)")
  test(@() t.findvalue(@(_, k) k=="a"), 1, "t.findvalue(@(_,v) v==3)")
  test(@() t.keys().contains("a"), true, "t.keys().contains('a')")
  test(@() t.values().contains(2), true, "t.values().contains(2)")
  test(@() t.each(@(v,k) v), null, "t.each(@(k,v) v)")
  test(@() t.__update({b=20, d=4})["b"], 20, "t.__update({b=20, d=4})")
  test(@() t.__merge({e=5})["e"], 5, "t.__merge({e=5})")
  test(@() t.topairs().sort(@(a,b) a[0]<=>b[0])[0][0], "a", "t.topairs()[0][0]")
  test(@() t.replace_with({x=10, y=20})["x"], 10, "t.replace_with({x=10, y=20})")
  test(@() t.hasindex("x"), true, "t.hasindex('x')")
  test(@() t.hasindex("z"), false, "t.hasindex('z')")
  test(@() t.hasvalue(10), true, "t.hasvalue(10)")
  test(@() t.hasvalue(5), false, "t.hasvalue(5)")
  test(@() t.swap("x","y")["x"], 20, "t.swap('x','z')")
  test(@() t.clear().len(), 0, "t.clear().len()")
  print("ok")
}
catch(e){
  println(e)
}