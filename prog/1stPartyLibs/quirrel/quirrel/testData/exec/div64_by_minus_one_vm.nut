try {
    local x = -0x7FFFFFFFFFFFFFFF - 1
    local y = @(a) -a
    print( x / y(1) )
} catch (e) {
    print("OK")
}