options log

require tutorial_07

[export]
def test
    print("this tutorial implements C interfaces\n")
    das_c_func(13)

    var a : intarray
    for x in range(3)
        push(a,x)
    print("a = {a}\n")

    var fb : FooBar
    fb.foo = 1
    fb.bar = 2.0
    print("fb = {fb}\n")
    var fb2 : FooBar
    fb2 <- fb
    print("fb2 = {fb2} fb = {fb}\n")

    let t = OneTwo one
    print("t = {t} {OneTwo two}\n")


