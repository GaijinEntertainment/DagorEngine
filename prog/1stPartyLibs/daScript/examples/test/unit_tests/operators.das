def charg(var t:int):int
    t++
    return t

def checkInc(var val : int&)
  	val++

[export]
def test:bool
    let x = 1
    var y:int
    y=charg(x)
    assert(x==1 && y==2)
    // cond
    var a,b,c:int
    true ? a : b = 1            // true returns left
    assert(a==1 && b==0)
    false ? a : b = 2           // false returns right
    assert(a==1 && b==2)
    true ? (c++) : (c--)      // only one siede is calculated
    assert(c==1)
    false ? (c--) : (c++)     // only one side
    assert(c==2)
    // inc ref
 	var idx = 1
  	checkInc(idx)
	assert(idx==2)
    return true

