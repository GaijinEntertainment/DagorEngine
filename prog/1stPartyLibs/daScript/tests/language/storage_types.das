options gen2
require dastest/testing_boost public

struct StStorage {
    i8 : int8
    ui8 : uint8
    i16 : int16
    ui16 : uint16
}

[test]
def test_storage_types(t : T?) {
    t |> run("storage types sizeof and roundtrip") @(t : T?) {
        var st : StStorage
        var sq : StStorage
        t |> equal(typeinfo sizeof(type<StStorage>), 6)
        st.i8 = int8(1)
        st.ui8 = uint8(2)
        st.i16 = int16(3)
        st.ui16 = uint16(4)
        t |> success(int(st.i8) == 1 && uint(st.ui8) == 2u && int(st.i16) == 3 && uint(st.ui16) == 4u)
        let i8 = st.i8
        sq.i8 = i8
        t |> equal(int(sq.i8), 1)
    }
}
