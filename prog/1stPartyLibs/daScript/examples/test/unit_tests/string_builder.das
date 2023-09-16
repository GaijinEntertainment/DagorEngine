// options log=true,optimize=true,log_optimization_passes=false

def test_arg(a:string):string
    return "hello,{a}!"

[export]
def test:bool
    let empty:string = ""
    let q:string = "blah" + "blah"
    let s:string = "hello,world!"
    let s2:string = "S = {s}"
    let s3:string = "S2 = \{{s2}}";
    let s4:string = "\"say\" 2 + 2 = {2+2}";
    let s5:string = "\"say\" 2 + 2 = {2+2}{empty}";
    assert(q=="blahblah")
    assert(s2=="S = hello,world!")
    assert(s3=="S2 = \{S = hello,world!}")
    assert(s4=="\"say\" 2 + 2 = 4")
    assert(s4==s5)
    //print("{s}\n{s2}\n{s3}\n{s4}\n")
    //print("do u like that? {test_arg("string builder")} - cool,huh\n");
    return true
