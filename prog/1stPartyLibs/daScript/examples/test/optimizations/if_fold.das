options log = false

def if_to_cond(cond)        // folds to cond ? 1 : 2
    if ( cond )
        return 1
    else
        return 2

def if_in_block(cond)       // folds to if - else
    if cond
        return 3
    assert(false)
    return 5

def life_after_if(cond)     // folds to cond ? 1 : 2
    if cond
        return 1
    else
        return 2
    print("this???")
    return 3

def death_after_if(cond)    // gets eliminated completly
    if cond
        return
    else
        return
    print("this???")

[export]
def test
    if_to_cond(false)
    if_in_block(true)
    life_after_if(true)
    death_after_if(true)
    return true

