options gen2
require dastest/testing_boost
require daslib/fuzzer
require daslib/faker


def not_empty_hash(t : T?, val) {
    if (hash(val) == 0x1234567890abcdeful) {
        t.failure("Hash of value is empty:")
        debug(val)
    }
}
def empty_hash(t : T?, val) {
    if (hash(val) != 0x1234567890abcdeful) {
        t.failure("Hash of value is not empty:")
        debug(val)
    }
}

struct TVoid {
    a : void?
}

[test]
def test_hash_void(t : T?) {
    var a : void?

    t.equal(hash(a), hash(TVoid()))
    t.not_empty_hash(a)
    t.not_empty_hash(TVoid())
}


struct TInt8 {
    a : int8
}

[test]
def test_hash_int8(t : T?) {
    var fake <- Faker()
    fuzz() $ {
        var a = fake |> random_int8()
        t.equal(hash(a), hash(TInt8(a = a)))
        t.not_empty_hash(a)
        t.not_empty_hash(TInt8(a = a))
    }
}

struct TUint8 {
    a : uint8
}

[test]
def test_hash_uint8(t : T?) {
    var fake <- Faker()
    fuzz() $ {
        var a = fake |> random_uint8()
        t.equal(hash(a), hash(TUint8(a = a)))
        t.not_empty_hash(a)
        t.not_empty_hash(TUint8(a = a))
    }
}


struct TInt16 {
    a : int16
}

struct TInt8_8 {
    a : int8
    b : int8
}

[test]
def test_hash_int16(t : T?) {
    var fake <- Faker()
    fuzz() $ {
        var a = fake |> random_int16()
        t.equal(hash(a), hash(TInt16(a = a)))
        t.not_empty_hash(a)
        t.not_empty_hash(TInt16(a = a))
    }
}

struct TUint16 {
    a : uint16
}

[test]
def test_hash_uint16(t : T?) {
    var fake <- Faker()
    fuzz() $ {
        var a = fake |> random_uint16()
        t.equal(hash(a), hash(TUint16(a = a)))
        t.not_empty_hash(a)
        t.not_empty_hash(TUint16(a = a))
    }
}


struct TFunction {
    a : function<void>
}

[export] def nada() {}

[test]
def test_hash_function(t : T?) {
    var a : function<void>
    t.equal(hash(a), hash(TFunction(a = a)))
    t.empty_hash(a)
    t.empty_hash(TFunction(a = a))

    var b = @@nada
    t.equal(hash(b), hash(TFunction(a = b)))
    t.not_empty_hash(b)
    t.not_empty_hash(TFunction(a = b))
}

enum TEnumValue {
    A, B, C
}

struct TEnum {
    a : TEnumValue
}

[test]
def test_hash_enum(t : T?) {
    var fake <- Faker()
    fuzz() $ {
        var a = fake |> any_enum(TEnumValue.A)
        t.equal(hash(a), hash(TEnum(a = a)))
        t.not_empty_hash(a)
        t.not_empty_hash(TEnum(a = a))
    }
}
