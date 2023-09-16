// this is port from https://github.com/frol/completely-unscientific-benchmarks
// naive is almost direct kotlin implementation
// regular is almost direct C++-raw implementation

// options log_infer_passes = true
// options log = true
// options log_nodes = true

options persistent_heap = true

require daslib/random
require testProfile

struct Node
    x,y:int
    left, right: Node?

var
    seed:int4 = int4(1,1,1,1)

[unsafe_deref]
def merge(var lower, greater: Node?): Node?
    if lower == null
    	return greater
    if greater == null
    	return lower
    if lower.y < greater.y
        lower.right = merge(lower.right, greater)
        return lower
    else
        greater.left = merge(lower, greater.left)
        return greater

def merge(var lower,equal,greater: Node?): Node?
    return merge(merge(lower, equal), greater)

[unsafe_deref]
def split(var orig:Node?; var lower:Node?&; var greaterOrEqual:Node?&; value: int)
    if orig == null
    	lower = null
		greaterOrEqual = null
    elif orig.x < value
		lower = orig
		split(lower.right,lower.right,greaterOrEqual,value)
    else
		greaterOrEqual = orig
		split(greaterOrEqual.left,lower,greaterOrEqual.left,value)

def split(var orig: Node?; var lower:Node?&; var equal:Node?&; var greater:Node?&; value: int)
	var equalOrGreater : Node?
    split(orig, lower, equalOrGreater, value)
    split(equalOrGreater, equal, greater, value + 1)

def hasValue(var mRoot:Node? &; x: int): bool
	var lower, equal, greater : Node?
    split(mRoot, lower, equal, greater, x)
    let res = equal != null
    mRoot = merge(lower, equal, greater)
    return res

def insert(var mRoot:Node? &; x: int)
    var lower, equal, greater : Node?
    split(mRoot, lower, equal, greater, x)
    if equal == null
    	equal = new [[Node x=x, y=random_int(seed)]]
    mRoot = merge(lower, equal, greater)

def erase_raw(var mRoot:Node? &; x: int)
    var lower, equal, greater : Node?
    split(mRoot, lower, equal, greater, x)
    mRoot = merge(lower, greater)
    unsafe
        delete equal

def main
    var tree:Node?
    var cur = 5
    var res = 0
    for i in range(1, 1000000)
        let a = i % 3
        cur = (cur * 57 + 43) % 10007
        if a == 0
            tree |> insert(cur)
        elif a == 1
            tree |> erase_raw(cur)
        elif a == 2
            res += (tree |> hasValue(cur)) ? 1 : 0
    unsafe
	    delete tree
    return res

struct Pair
    first,second: Node?

[unsafe_deref]
def splitBinary(var orig: Node?; value: int): Pair
    if orig == null
    	return [[ Pair first=null, second=null ]]

    if orig.x < value
        let splitPair = splitBinary(orig.right, value)
        orig.right = splitPair.first
        return [[ Pair first=orig, second=splitPair.second ]]
    else
        let splitPair = splitBinary(orig.left, value)
        orig.left = splitPair.second
        return [[ Pair first=splitPair.first, second=orig ]]


struct SplitResult
    lower,equal,greater:Node?

def splitNaive(var orig: Node?; value: int): SplitResult
    var lower_eg = splitBinary(orig, value)
    var equal_greater = splitBinary(lower_eg.second, value + 1)
    return [[ SplitResult lower = lower_eg.first, equal = equal_greater.first, greater = equal_greater.second ]]

def hasValueNaive(var mRoot:Node? &; x: int): bool
    var splited = splitNaive(mRoot, x)
    let res = splited.equal != null
    mRoot = merge(splited.lower, splited.equal, splited.greater)
    return res

def insertNaive(var mRoot:Node? &; x: int)
    var splited = splitNaive(mRoot, x)
    if splited.equal == null
    	splited.equal = new [[Node x=x, y=random_int(seed)]]
    mRoot = merge(splited.lower, splited.equal, splited.greater)

def erase_rawNaive(var mRoot:Node? &; x: int)
    var splited = splitNaive(mRoot, x)
    mRoot = merge(splited.lower, splited.greater)
    unsafe
        delete splited.equal

def mainNaive
    var tree:Node?
    var cur = 5
    var res = 0
    for i in range(1, 1000000)
        let a = i % 3
        cur = (cur * 57 + 43) % 10007
        if a == 0
            tree |> insertNaive(cur)
        elif a == 1
            tree |> erase_rawNaive(cur)
        elif a == 2
            res += (tree |> hasValueNaive(cur)) ? 1 : 0
    unsafe
	    delete tree
    return res

[export]
def test
    profile(10, "tree")  <|
        main()
    profile(10, "tree-naive")  <|
        mainNaive()
    profile(10, "tree C++")  <|
        testProfile::testTree()
    return true
