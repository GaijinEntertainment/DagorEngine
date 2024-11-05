require math

require daslib/hash_map

%spoof_instance~TFlatHashMap(FlatHashMap_string_to_int,string,int)%%;

def makeRandomSequence(var src:array<string>)
    let n = 500000
    let mod = uint(n)
    resize(src,n)
    for i in range(n)
        let num = (271828183u ^ uint(i*119))%mod
        src[i] = "{num}"

def dict(var tab:auto; src:array<string>)
    clear(tab)
    var maxOcc = 0
    for s in src
        maxOcc = max(++tab[s],maxOcc)
    return maxOcc

[export]
def main
    var src : array<string>
    makeRandomSequence(src)
    if true
        var tab : table<string;int>
        profile(20,"dictionary") <|
            dict(tab,src)
    if true
        var tab <- FlatHashMap_string_to_int()
        profile(20,"flat hash map") <|
            dict(tab,src)
    if true
        var tab <- InterleavedFlatHashMap_string_to_int()
        profile(20,"interleaved flat hash map") <|
            dict(tab,src)
