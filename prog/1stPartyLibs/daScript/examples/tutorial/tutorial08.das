require tutorial_08

[export]
def test
    print("this tutorial utilizes basic builin module with constant and function\n")
    let sq2 = SQRT2
    print("sq2 = {sq2} // expecting sqrt(2), 1.41421\n")
    let xma = xmadd(sq2,sq2,1.0,1.0)    // sq2*sq2 + 1*1 => 3
    print("xma = {xma} // expecting 3\n")

