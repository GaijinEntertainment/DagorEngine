require strings

// note: this is equivalent to
//  unsafe
//      return <- each(input)
// unsafe is required due to lifetime of inp. version bellow will keep string alive
def from_sv ( inp : string )
    return <- generator<int>() <|
        for x in inp
            yield x
        return false

def chunk6 ( var inp : iterator<int> )
    return <- generator<int> [[<-inp]] () <|
        var b0 = 0
        while next(inp, b0)
            var b1, b2 = 0
            let has1 = next(inp, b1)
            let has2 = next(inp, b2)
            yield b0 >> 2
            yield ((b0 & 3) << 4) | (b1 >> 4)
            if has1
                yield ((b1 & 15) << 2) | (b2 >> 6)
            if has2
                yield b2 & 63
        return false

let
    xlattable64 <- [{for t in "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/"; t}]

// note: this is equivalent to
//  unsafe
//      return <- [[ for i in inp; yield xlattable64[i] ]]
// unsafe is required due to implicit capture of inp
def xlat ( var inp : iterator<int> )
    return <- generator<int> [[<-inp]] () <|
        for i in inp
            yield xlattable64[i]
        return false

def pad4 ( var inp : iterator<int> )
    return <- generator<int> [[<-inp]] () <|
        var len = 0
        for c in inp
            yield c
            ++ len
        while (len++ % 4) != 0
            yield '='
        return false

def linebreak ( var inp : iterator<int>; linewidth = 76 )
    return <- generator<int> [[<-inp]] () <|
        for c,len in inp, range(1,INT_MAX)
            yield c
            if (len % linewidth)==0
                yield '\n'
        return false

def base64 ( inp : string; linewidth = 76 )
    return <- generator<int> () <|
        var lb <- inp |> from_sv() |> chunk6() |> xlat() |> pad4() |> linebreak(linewidth)
        for c in lb
            yield c
        return false

let
    sample = "Man is distinguished, not only by his reason, but by this singular passion from other animals,
which is a lust of the mind, that by a perseverance of delight in the continued and indefatigable
generation of knowledge, exceeds the short vehemence of any carnal pleasure."

[export]
def test
    print("\n\n{sample}\n\n")
    for ch in base64(sample)
        print(to_char(ch))
    return true

