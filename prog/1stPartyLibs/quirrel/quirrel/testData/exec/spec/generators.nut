let function geny(n) {
    for (local i=1; i<=n; ++i)
        yield i
    return null
}

let gtor = geny(10)
for (local x=resume gtor; x; x=resume gtor)
    println(x+"\n")
