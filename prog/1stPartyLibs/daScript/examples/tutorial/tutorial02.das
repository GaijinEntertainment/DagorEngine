options gen2
require tutorial_02

[export]
def test {
    var a = 2
    print("this tutorial utilizes basic builin module with constant and function\n")
    let sq2 = SQRT2
    print("sq2 = {sq2} // expecting sqrt(2), 1.41421\n")
    print("a   = {a}   // expecting var a initialized with 2\n")
    let xma = xmadd(sq2, sq2, 1.0, 1.0) // sq2*sq2 + 1*1 => 3
    print("xma = {xma} // expecting 3\n")

    // anonymous invoke
    var isTps = false
    isTps = true
    let offset = invoke() <| $() {
        if (isTps) {
            return 1
        } else {
            return 2
        }
    }
    print("offset = {offset} // expecting 2.0\n")
    let offset2 = invoke($(aa : int){
        return aa + 1;
    }, 13)
    print("offset2 = {offset2} // expecting 14\n")
    invoke() <| $() {
        print("this is an anonymous invoke\n")
    }
}


