let function testy(n) {
    local a = 0
    while (a < n)
      a += 1

    while (1) {
        if (a < 0)
            break
        println(a)
        a -= 1
    }
}

testy(10)
