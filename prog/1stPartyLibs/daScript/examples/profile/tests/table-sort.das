// options log=true, print_var_access=true, print_ref=true

require math
require testProfile

def makeRandomSequence(var src:array<int>)
    let n = 2000000
    let seed = 1u
    resize(src,n)
    for i in range(n)
        src[i] = int(uint_noise_1D(i, seed))

[export]
def test
    var tab : array<int>
    makeRandomSequence(tab)
	profile(1,"table-sort") <|
        sort(tab)
    makeRandomSequence(tab)
	profile(1,"table-sort-cmp") <|
        sort ( tab, $(a,b) => !(a < b) )
    return true

