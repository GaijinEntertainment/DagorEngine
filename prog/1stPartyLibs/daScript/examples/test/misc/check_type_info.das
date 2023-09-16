require UnitTest

require rtti
require ast

def log_type ( t : auto(TT)? )
    print("
        {typeinfo(typename type<TT>)}:
            isLocal = {typeinfo(is_local type<TT>)}
            canBePlacedInContainer = {typeinfo(can_be_placed_in_container type<TT>)}
            canCopy = {typeinfo(can_copy type<TT>)}
            canMove = {typeinfo(can_move type<TT>)}
            canClone = {typeinfo(can_clone type<TT>)}
            canDelete = {typeinfo(can_delete type<TT>)}
            canNew = {typeinfo(can_new type<TT>)}
            canDeletePtr = {typeinfo(can_delete_ptr type<TT>)}
            hasNonTrivialCtor = {typeinfo(has_nontrivial_ctor type<TT>)}
            hasNonTrivialDtor = {typeinfo(has_nontrivial_dtor type<TT>)}
            hasNonTrivialCopy = {typeinfo(has_nontrivial_copy type<TT>)}
    ")

[export]
def test
    var m : FileAccess?
    log_type(m)
    return true
