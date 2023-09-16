// options log=true,optimize=true,log_optimization_passes=false

// options logCpp=true

require strings

def failUint(val: string)
    try
        debug(uint(val), "=")
        assert(false, "can't convert non uint string without exception")
    recover {}

def failInt(val: string)
    try
        debug(int(val), "=")
        assert(false, "can't convert non int string without exception")
    recover {}


def failFloat(val: string)
    try
        debug(float(val), "=")
        assert(false, "can't convert non float string without exception")
    recover {}

def failDouble(val: string)
    try
        debug(double(val), "=")
        assert(false, "can't convert non double string without exception")
    recover {}

def failAll(val: string)
    failUint(val)
    failInt(val)
    failFloat(val)
    failDouble(val)

[export]
def test:bool
    let s:string = "hello, world!"
    assert(to_upper(s) == "HELLO, WORLD!")
    assert(to_lower(to_upper(s)) == s)
    assert(reverse(s) == "!dlrow ,olleh")
    assert(length(s) == 13)
    assert(find(s, "world!") == 7)
    assert(find(s, "world!", 4) == 7)
    assert(find(s, "o") == 4)
    assert(find(s, "o", 5) == 8)
    assert(find(s, "o", 9) == -1)
    assert(find(s, "??") == -1)
    assert(ends_with(s, "world!"))
    assert(starts_with(s, "hello"))
    assert(starts_with(s, ""))
    assert(ends_with(s, ""))
    assert(starts_with(s, "a") == false)
    assert(ends_with(s, "b") == false)
    let s2:string = "   hello, world!   "
    assert(strip(s2) == s)
    assert(strip_right(strip_left(s2)) == s)
    assert(strip_left(s2) == "hello, world!   ")
    assert(strip_right(s2) == "   hello, world!")
    assert(slice(s, 7) == "world!")
    assert(slice(s, -6) == "world!")
    assert(slice(s, 6, -6) == " ")
    assert(slice(s, 6, 7) == " ")
    assert(int("11") == 11)
    assert(int("-12") == -12)
    assert(int("-12  ") == -12)
    assert(int("  -12") == -12)
    assert(int("  -12  ") == -12)
    assert(uint("11") == 11U)
    assert(uint("  12  ") == 12U)
    assert(uint("  12") == 12U)
    assert(int64("11") == 11l)
    assert(int64("-12") == -12l)
    assert(int64("-12  ") == -12l)
    assert(int64("  -12") == -12l)
    assert(int64("  -12  ") == -12l)
    assert(uint64("11") == 11ul)
    assert(uint64("  12  ") == 12ul)
    assert(uint64("  12") == 12ul)
    assert(float("-12") == -12.)
    assert(float("1e1") == 10.)
    assert(float("  12") == 12.)
    assert(float("12   ") == 12.)
    assert(float("  12   ") == 12.)
    failAll("")
    failAll("abc")
    assert(to_int("") == 0)
    assert(to_float("") == 0f)
    assert(to_int("11abc") == 11)
    assert(to_float("11.1abc") == 11.1)
    let a_slash_slash = "ab\\xy"
    assert(length(a_slash_slash)==5)
    let mls = "this is
multi line string"
    assert(mls=="this is\nmulti line string")
    assert(format("%02x",26)=="1a")
    assert(format("%02x",27u)=="1b")
    assert(format("%02x",28l)=="1c")
    assert(format("%02x",29ul)=="1d")
    assert(format("%.1f",1.23)=="1.2")
    assert(format("%.2f",1.234)=="1.23")
    // string builder
    let st = build_string() <| $ (var writer : StringBuilderWriter)
        write(writer,"255 hex is ")
        format(writer,"%2x",255)
    assert(st=="255 hex is ff")
    // string iterator
    var count = 0
    var index = 0
    for ch in st
        if ch=='5'
            count ++
        let chAt = character_at(st,index)
        assert(ch==chAt)
        index ++
    assert(count==2)
    // check if builder does its jo, and deletes
    var zzs = build_string() <| $ ( writer )
        for zzx in range(100500)
            write(writer,zzx)
    unsafe
        delete_string(zzs)
    return true
