options no_unused_block_arguments = false
options no_unused_function_arguments = false
options indenting = 4

module base64 shared private

// very much inspired by LIBHV implementation

require daslib/strings

let private BASE64_PAD = '='
let private BASE64DE_FIRST = '+'
let private BASE64DE_LAST = 'z'

let base64en <- [[int
    'A'; 'B'; 'C'; 'D'; 'E'; 'F'; 'G'; 'H';
    'I'; 'J'; 'K'; 'L'; 'M'; 'N'; 'O'; 'P';
    'Q'; 'R'; 'S'; 'T'; 'U'; 'V'; 'W'; 'X';
    'Y'; 'Z'; 'a'; 'b'; 'c'; 'd'; 'e'; 'f';
    'g'; 'h'; 'i'; 'j'; 'k'; 'l'; 'm'; 'n';
    'o'; 'p'; 'q'; 'r'; 's'; 't'; 'u'; 'v';
    'w'; 'x'; 'y'; 'z'; '0'; '1'; '2'; '3';
    '4'; '5'; '6'; '7'; '8'; '9'; '+'; '/'
]]

/* ASCII order for BASE 64 decode, -1 in unused character */
let base64de <- [[int
    /* '+'; ';'; '-'; '.'; '/'; '0'; '1'; '2'; */
        62;  -1;  -1;  -1;  63;  52;  53;  54;
    /* '3'; '4'; '5'; '6'; '7'; '8'; '9'; ':'; */
        55;  56;  57;  58;  59;  60;  61;  -1;
    /* ';'; '<'; '='; '>'; '?'; '@'; 'A'; 'B'; */
        -1;  -1;  -1;  -1;  -1;  -1;   0;   1;
    /* 'C'; 'D'; 'E'; 'F'; 'G'; 'H'; 'I'; 'J'; */
         2;   3;   4;   5;   6;   7;   8;   9;
    /* 'K'; 'L'; 'M'; 'N'; 'O'; 'P'; 'Q'; 'R'; */
        10;  11;  12;  13;  14;  15;  16;  17;
    /* 'S'; 'T'; 'U'; 'V'; 'W'; 'X'; 'Y'; 'Z'; */
        18;  19;  20;  21;  22;  23;  24;  25;
    /* '['; '\'; ']'; '^'; '_'; '`'; 'a'; 'b'; */
        -1;  -1;  -1;  -1;  -1;  -1;  26;  27;
    /* 'c'; 'd'; 'e'; 'f'; 'g'; 'h'; 'i'; 'j'; */
        28;  29;  30;  31;  32;  33;  34;  35;
    /* 'k'; 'l'; 'm'; 'n'; 'o'; 'p'; 'q'; 'r'; */
        36;  37;  38;  39;  40;  41;  42;  43;
    /* 's'; 't'; 'u'; 'v'; 'w'; 'x'; 'y'; 'z'; */
        44;  45;  46;  47;  48;  49;  50;  51
]]

def public BASE64_DECODE_OUT_SIZE(s:int)
    return s / 4 * 3

def public BASE64_ENCODE_OUT_SIZE(s:int)
    return (s + 2) / 3 * 4

def public base64_decode ( _in:string ) : tuple<text:string;size:int>
    //! decode base64 string
    var inscope out : array<uint8>
    var outlen = 0
    peek_data(_in) <| $ ( inp )
        outlen = base64_decode(inp,out)
    return [[auto string(out), outlen]]

def public base64_decode ( _in:string; var out:array<uint8> ) : int
    var outlen = 0
    peek_data(_in) <| $ ( inp )
        outlen = base64_decode(inp,out)
    return outlen

def base64_decode ( inp:array<uint8> | #; var out:array<uint8> ) : int
    let inlen = inp |> length
    var j = 0
    out |> clear()
    out |> resize(BASE64_DECODE_OUT_SIZE(inlen))
    for i in range(inp |> length)
        var c = 0
        var s = i % 4
        var ii = int(inp[i])
        if ii == BASE64_PAD
            break
        if ii < BASE64DE_FIRST || ii > BASE64DE_LAST
            return -1
        c = base64de[ii - BASE64DE_FIRST]
        if c == -1
            return -1
        if s == 0
            out[j] = uint8((uint(c) << 2u) & 0xFF)
            continue
        elif s == 1
            out[j] = uint8(uint(out[j]) + ((uint(c) >> 4u) & 0x3)); j++
            //  if not last char with padding
            if i < (inlen - 3) || int(inp[inlen - 2]) != BASE64_PAD
                out[j] = uint8((uint(c) & 0xF) << 4u)
            continue
        elif s == 2
            out[j] = uint8(uint(out[j]) + ((uint(c) >> 2u) & 0xF)); j++
            // if not last char with padding
            if i < (inlen - 2) || int(inp[inlen - 1]) != BASE64_PAD
                out[j] = uint8((uint(c) & 0x3) << 6u)
            continue
        elif s == 3
            out[j] = uint8(int(out[j]) + c); j++
    if j != -1
        out |> resize(j)
    return j

def public base64_encode ( _inp:string )
    //! Encodes a string to base64
    var res : string
    peek_data(_inp) <| $ ( inp )
        res = base64_encode(inp)
    return res

def public base64_encode ( inp:array<uint8> | # )
    var inlen = inp |> length
    var j = 0
    var out : array<uint8>
    out |> resize(BASE64_ENCODE_OUT_SIZE(inlen))
    for i in range(inlen)
        var s = i % 3
        if s==0
            out[j++] = uint8(base64en[(uint(inp[i]) >> 2u) & 0x3F])
            continue
        elif s==1
            out[j++] = uint8(base64en[((uint(inp[i-1]) & 0x3) << 4u) + ((uint(inp[i]) >> 4u) & 0xF)])
            continue
        elif s==2
            out[j++] = uint8(base64en[((uint(inp[i-1]) & 0xF) << 2u) + ((uint(inp[i]) >> 6u) & 0x3)])
            out[j++] = uint8(base64en[uint(inp[i]) & 0x3F])
    // move back
    var i = inlen - 1
    // check the last and add padding
    if (i % 3) == 0
        out[j++] = uint8(base64en[(uint(inp[i]) & 0x3) << 4u])
        out[j++] = uint8('=')
        out[j++] = uint8('=')
    elif (i % 3) == 1
        out[j++] = uint8(base64en[(uint(inp[i]) & 0xF) << 2u])
        out[j++] = uint8('=')
    return string(out)

