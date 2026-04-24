let min_int = -0x7FFFFFFF_FFFFFFFF-1
try {
    print( min_int / -1 )
} catch(e) {
    print("OK")
}