options indenting = 4
options no_unused_block_arguments = false
options no_unused_function_arguments = false

module _framework shared public

require daslib/random
require math
require jit

let TEST_REPEAT_COUNT = 1

let TOTAL_RANDOM_NUMBERS = 1000000

def profile_test ( name:string; var hashMap : auto(HT); randomData : auto(TT) ) : void
    /*
    if !is_jit_function(@@generate_unique_positive_random_numbers)
        panic("jit is not enabled!!! test is expected to run with jit enabled")
    */
    profile(TEST_REPEAT_COUNT,name) <|
        __::test(hashMap,randomData)

def generate_unique_positive_random_numbers ( kNumberOfItems : int = TOTAL_RANDOM_NUMBERS )
    var data : array<int>
    data |> reserve(kNumberOfItems)
    var seed = random_seed(13)
    var set : table<int>
    for r in range(kNumberOfItems)
        var n = random_big_int(seed)
        while set |> key_exists(n)
            n = random_big_int(seed)
        set |> insert(n)
        data |> push(n)
    return <- data

def get_random_numbers_with_intersections ( factor:int )
    var data <- generate_unique_positive_random_numbers()
    let cutoff = length(data) / factor
    var j = int(cutoff)
    for i in range(cutoff)
        data[i] <- data[j]
        j ++
    var seed = random_seed(13)
    shuffle(data,seed)
    return <- data

// 10% of the resulting data set is intersected
def get_random_numbers_with_intersections_10
    return <- get_random_numbers_with_intersections(10)

// 50% of the resulting data set is intersected
def get_random_numbers_with_intersections_50
    return <- get_random_numbers_with_intersections(2)

def shuffle ( var data : array<int>; var seed:int4& )
    let len = data |> length
    for i in range(len)
        let j = abs(random_int(seed) % len)
        var temp <- data[i]
        data[i] <- data[j]
        data[j] <- temp

