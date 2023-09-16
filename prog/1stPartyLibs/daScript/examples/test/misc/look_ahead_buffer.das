require daslib/strings_boost

struct LookAheadBuffer
    src : iterator<int>
    que : array<int>

def look_ahead ( src:string; blk:block<(var buf:LookAheadBuffer):void>)
    unsafe
        var la <- [[LookAheadBuffer src<-each(src)]]        // unsafe due to 'each' on string
        invoke(blk, la)
        delete la

def empty ( var buf:LookAheadBuffer ) : bool
    if length(buf.que) > 0
        return false
    return buf.src |> empty()

def next ( var buf:LookAheadBuffer; var value : int& ) : bool
    if length(buf.que) > 0
        value = buf.que[0]
        buf.que |> erase(0)
        return true
    return buf.src |> next(value)

def skip ( var buf:LookAheadBuffer; var total:int=1 )
    let lbq = length(buf.que)
    if total <= lbq
        total -= lbq
        buf.que |> clear()
    var Ch = 0
    while total > 0
        buf |> next(Ch)
        total --

def ahead ( var buf:LookAheadBuffer; var value : int&; offset:int = 0 ) : bool
    while length(buf.que) <= offset
        var Ch = 0
        if buf.src |> next(Ch)
            buf.que |> push(Ch)
        else
            return false
    if length(buf.que) > offset
        value = buf.que[offset]
        return true
    return false

def accept ( var buf:LookAheadBuffer; st:string ) : bool
    let slen = length(st)
    var Ch = 0
    for sc,i in st,range(slen)
        if buf |> ahead(Ch,i)
            if Ch!=sc
                return false
        else
            return false
    buf |> skip(slen)
    return true


let tokens <- [{string
    "<<"; ">>"
}]

def tokenize ( inp:string )                                 // TODO: turn into generator
    look_ahead(inp) <| $ ( var tin )
        var Ch = 0
        while !empty(tin)
            while ahead(tin,Ch) && is_white_space(Ch)
                tin |> next(Ch)
            var found = false
            for tok in tokens
                if tin |> accept(tok)
                    print("token `{tok}`")                  // TODO: yield
                    found = true
                    break
            if found
                continue
            tin |> ahead(Ch)
            if is_alpha(Ch)
                var arr : array<uint8>
                while tin |> ahead(Ch) && is_alpha(Ch)
                    tin |> next(Ch)
                    arr |> push(uint8(Ch))
                var tok = string(arr)
                print("text `{tok}`\n")
                unsafe
                    delete_string(tok)                      // note: this is here for memory efficiency only
                delete arr
            else
                tin |> skip()
                print("`{to_char(Ch)}` -> {Ch}\n")          // TODO: yield

[export]
def main
    let test_str = "foo + b * ( c << 2 ) << 3"
    tokenize(test_str)
