struct Storage
    i8 : int8
    ui8 : uint8
    i16 : int16
    ui16 : uint16

[export]
def test:bool
    var t, q : Storage
    assert(typeinfo(sizeof type<Storage>)==6)
    t.i8 = int8(1)
    t.ui8 = uint8(2)
    t.i16 = int16(3)
    t.ui16 = uint16(4)
    assert(int(t.i8)==1 && uint(t.ui8)==2u && int(t.i16)==3 && uint(t.ui16)==4u)
    let i8 = t.i8
    q.i8 = i8
    assert(int(q.i8)==1)
    return true


