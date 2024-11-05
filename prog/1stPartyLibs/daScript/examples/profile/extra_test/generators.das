require daslib/functional
require daslib/strings_boost

def each_a(r:range) : array<int>
    var a : array<int>
    reserve(a,r.y-r.x)
    for x in r
        push(a,x)
    return <- a

def map_a(src:array<auto(TT)>; blk:block<(what:TT):auto(QQ)>) : array<QQ>
    var a : array<QQ>
    reserve(a, length(src))
    for x in src
        push(a,invoke(blk,x))
    return <- a

def filter_a(src:array<auto(TT)>; blk:block<(what:TT):bool> ) : array<TT>
    var a : array<TT>
    for x in src
        if invoke(blk,x)
            push(a,x)
    return <- a

def testGen(total,mod:int)
    return (
        each(range(total))
        |> filter ( @(x) => (x%mod)==0 )
        |> map ( @(x) => "{x}" )
        |> join(",")
    )

def testArr(total,mod:int)
    var a1 <- each_a(range(total))
    var a2 <- filter_a(a1,$(x) => (x%mod)==0)
    var a3 <- map_a(a2, $(x) => "{x}")
    return join(a3,",")
finally
    delete a1
    delete a2
    delete a3

def testLoop(total,mod:int)
    let st = build_string() <| $ (var writer)
        var skip_first = true
        for t in range(total)
            if (t % mod) == 0
                if skip_first
                    skip_first = false
                else
                    write(writer, ",")
                write(writer,string(t))
    return st

[export]
def main
    var s1, s2, s3 : string
    let TOTAL = 1000000
    let MOD = 99961
    profile(40,"testLoop") <|
        s1 = testLoop(TOTAL,MOD)
    profile(40,"testGen") <|
        s2 = testGen(TOTAL,MOD)
    profile(40,"testArr") <|
        s3 = testArr(TOTAL,MOD)
    assert(s1==s2 && s1==s3 && s2==s3)


