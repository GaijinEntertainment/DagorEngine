// options logCpp=true

def testI ( arg : tuple < i : int; float > )
    return arg.i == 1

def testF ( arg : tuple < int; f : float > )
    return arg.f == 3.14

def testFinalizeAndClone
    var x : tuple<int; float; array<int> >
    x._0 = 1
    x._1 = 2.0
    x._2 <- [{for xx in range(10); xx}]
    var y := x
    assert(y._0==1 && y._1==2.0)
    for t in range(10)
        assert(y._2[t]==t)
    delete x
    assert(x._0==0 && x._1==0.0 && length(x._2)==0)
    assert(y._0==1 && y._1==2.0)
    for t in range(10)
        assert(y._2[t]==t)
    return true

[export]
def test:bool
    let ttt = [[auto 1, 3.14]]
    assert(testI(ttt))
    assert(testF(ttt))
    assert(ttt._first==1)
    assert(ttt._last==3.14)
    var t : tuple < int; float; string >
    assert(typeinfo(sizeof t)==16 || typeinfo(sizeof t)==12) // 64 or 32 bit
    t._0 = 1
    t._1 = 2.0
    t._2 = "3"
    var q = t
    assert(q._0==1)
    assert(q._1==2.0)
    assert(q._2=="3")
    unsafe
        var pq = addr(q)
        assert(pq._0==1)
        assert(pq._1==2.0)
        assert(pq._2=="3")
        assert(pq?._0 ?? 0==1)
        assert(pq?._1 ?? 0.0==2.0)
        assert(pq?._2 ?? "nothing" =="3")
    var qq = new <tuple<int;float>>
    qq._0 = 1
    qq._1 = 2.0
    assert(qq?._0 ?? 0==1)
    assert(qq?._1 ?? 0.0==2.0)
    unsafe
        delete qq
    // make tuple
    let mkt = [[ tuple<int;float> 1, 2.0]]
    assert(mkt._0==1 && mkt._1==2.0)
    let mkta = [[ tuple<int;float> 1, 2.0; 3, 4.0]]
    assert(mkta[0]._0==1 && mkta[0]._1==2.0)
    assert(mkta[1]._0==3 && mkta[1]._1==4.0)
    // make tuple array, auto
    let ti = [[auto 1; 2; 3; 4]]
    assert(typeinfo(sizeof ti)==16)
    assert(ti[0]==1 && ti[1]==2 && ti[2]==3 && ti[3]==4)
    let tt = [[auto 1, 2; 3, 4]]
    assert(typeinfo(sizeof tt)==16)
    assert(tt[0]._0==1 && tt[0]._1==2 && tt[1]._0==3 && tt[1]._1==4)
    let tq = [[auto 1, 2.0]]
    assert(tq._0==1 && tq._1==2.0)
    // extra tests
    testFinalizeAndClone()
    return true

