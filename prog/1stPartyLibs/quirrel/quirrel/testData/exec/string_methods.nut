let s = "Hello, World!"
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
  test(@() s.len(), 13, "s.len()")
  test(@() "123".tointeger(), 123, "'123'.tointeger()")
  test(@() "abc".tointeger(), Exception("cannot convert the string to integer"), "'abc'.tointeger()")
  test(@() "12.34".tofloat(), 12.34, "'12.34'.tofloat()")
  test(@() s.tostring(), "Hello, World!", "s.tostring()")
  test(@() s.hash(), s.hash(), "s.hash()")
  test(@() s.slice(0,5), "Hello", "s.slice(0,5)")
  test(@() s.indexof("World"), 7, "s.indexof('World')")
  test(@() s.indexof("xyz"), null, "s.indexof('xyz')")
  test(@() s.contains("lo"), true, "s.contains('lo')")
  test(@() s.contains("xyz"), false, "s.contains('xyz')")
  test(@() s.tolower(), "hello, world!", "s.tolower()")
  test(@() s.toupper(), "HELLO, WORLD!", "s.toupper()")
  test(@() "{Hello}, World!".subst({Hello="Hi"}), "Hi, World!", "s.subst({Hello='Hi'})")
  test(@() s.replace("World", "Quirrel"), "Hello, Quirrel!", "s.replace('World', 'Quirrel')")
  test(@() "-".join(["a","b"]), "a-b", "'-'.join(['a','b'])")
  test(@() " ".concat("Hello, World!", "Test"), "Hello, World! Test", "' '.concat('Hello, World!', 'Test')")
  test(@() s.split(",")[0], "Hello", "s.split(',')")
  test(@() "  test  ".strip(), "test", "'  test  '.strip()")
  test(@() "  test".lstrip(), "test", "'  test'.lstrip()")
  test(@() "test  ".rstrip(), "test", "'test  '.rstrip()")
  test(@() "a,b c".split_by_chars(", ")[2], "c", "'a,b c'.split_by_chars(', ')")
  test(@() s.startswith("Hell"), true, "s.startswith('Hell')")
  test(@() s.endswith("ld!"), true, "s.endswith('ld!')")
  test(@() s.hasindex(0), true, "s.hasindex(0)")
  test(@() "22a".hasindex(3), false, "'22a'.hasindex(3)")
  test(@() "222".hasindex(-1), false, "s.hasindex(-1)")
  //test(@() "\r".escape(), "\\x0'", "'\\r'.escape()")
  print("ok")
}
catch(e){
  println(e)
}