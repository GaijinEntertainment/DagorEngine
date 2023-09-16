options indenting = 4
options no_unused_block_arguments = false
options no_unused_function_arguments = false
options strict_smart_pointers = true

module strings_boost shared public

require math
require strings public

def wide (text:string implicit; width:int)
    let extra = width - length(text)
    if extra > 0
        return text + repeat(" ",extra)
    else
        return text


def join(it; separator:string implicit)
    static_if !typeinfo(is_iterable it)
        concept_assert(false,"can't join non-iterable type")
        return ""
    else
        let st = build_string() <| $ (var writer)
            var skip_first = true
            for elem in it
                if skip_first
                    skip_first = false
                else
                    write(writer, separator)
                if typeinfo(is_string it)
                    write_char(writer, elem)
                else
                    write(writer, elem)
        return st

def private join_implement ( iterable; separator:string; blk ) : string
    let st = build_string() <| $ (var writer)
        var skip_first = true
        for elem in iterable
            if skip_first
                skip_first = false
            else
                write(writer, separator)
            blk |> invoke(writer, elem)
    return st

def join ( iterable:array<auto(TT)>; separator:string; blk:block<(var writer:StringBuilderWriter; elem:TT):void> ) : string
    return join_implement(iterable, separator, blk)

def join ( iterable:iterator<auto(TT)>; separator:string; blk:block<(var writer:StringBuilderWriter; elem:TT):void> ) : string
    return join_implement(iterable, separator, blk)

def join ( iterable:auto(TT)[]; separator:string; blk:block<(var writer:StringBuilderWriter; elem:TT):void> ) : string
    return join_implement(iterable, separator, blk)

def split ( text, delim : string implicit; blk : block< (arg:array<string>#) > )
    builtin_string_split ( text, delim, blk )

def split ( text, delim : string implicit ) : array<string>
    var res : array<string>
    builtin_string_split(text, delim) <| $ ( arr : array<string># ) : void
        res := arr
    return <- res

def split_by_chars ( text, delim : string implicit; blk : block< (arg:array<string>#) > )
    builtin_string_split_by_char ( text, delim, blk )

def split_by_chars ( text, delim : string implicit ) : array<string>
    var res : array<string>
    builtin_string_split_by_char (text, delim) <| $ ( arr : array<string># ) : void
        res := arr
    return <- res

[generic]
def is_character_at(foo:array<uint8> implicit; idx:int; ch:int)
    return int(foo[idx])==ch

[generic]
def eq ( a:string implicit; b:das_string )
    return a==b

[generic]
def eq ( b:das_string; a:string implicit )
    return b==a


// https://en.wikipedia.org/wiki/Levenshtein_distance
def levenshtein_distance(s, t: string implicit): int
    let sLen = length(s)
    let tLen = length(t)
    var d: array<array<int>>
    d |> resize(sLen + 1)
    for i in range(sLen + 1)
        d[i] |> resize(tLen + 1)
        d[i][0] = i
    for j in range(tLen + 1)
        d[0][j] = j

    for i in range(1, sLen + 1)
        for j in range(1, tLen + 1)
            let substitutionCost = unsafe(character_uat(s, i - 1)) == unsafe(character_uat(t, j - 1)) ? 0 : 1
            d[i][j] = min(d[i - 1][j] + 1, d[i][j - 1] + 1)
            d[i][j] = min(d[i][j], d[i - 1][j - 1] + substitutionCost)

    return d[sLen][tLen]
finally
    delete d

// https://en.wikipedia.org/wiki/Levenshtein_distance#Iterative_with_two_matrix_rows
// https://www.codeproject.com/Articles/13525/Fast-memory-efficient-Levenshtein-algorithm-2
def levenshtein_distance_fast(s, t: string implicit): int
    let sLen = length(s)
    let tLen = length(t)
    var v0: array<int>
    v0 |> resize(tLen + 1)
    var v1: array<int>
    v1 |> resize(tLen + 1)
    for i in range(tLen + 1)
        v0[i] = i

    for i in range(0, sLen)
        v1[0] = i + 1
        for j in range(0, tLen)
            let deletionCost = v0[j + 1] + 1
            let insertionCost = v1[j] + 1
            let substitutionCost = v0[j] + (unsafe(character_uat(s, i)) == unsafe(character_uat(t, j)) ? 0 : 1)
            v1[j + 1] = min(deletionCost, insertionCost)
            v1[j + 1] = min(v1[j + 1], substitutionCost)

        swap(v0, v1)

    return v0[tLen]
finally
    delete v0
    delete v1

def replace_multiple ( source:string; replaces:array<tuple<text:string;replacement:string>> )
    if empty(source) || empty(replaces)
        return source
    //! replaces occurances of multiple strings in a string. does not support overlap
    return build_string <| $ ( writer )
        var index = 0
        let len = source |> length
        while index < len
            var found = false
            for replace in replaces
                if source |> starts_with(index,replace.text)
                    writer |> write(replace.replacement)
                    index += replace.text |> length
                    found = true
                    break
            if !found
                writer |> write_char(unsafe(source |> character_uat(index)))
                index ++

def is_hex ( ch : int ) : bool
    return (ch>='0' && ch<='9') || (ch>='a' && ch<='f') || (ch>='A' && ch<='F')

def is_tab_or_space ( ch : int ) : bool
    return ch==' ' || ch=='\t'
