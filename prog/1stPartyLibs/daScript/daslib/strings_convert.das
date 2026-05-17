options gen2
options indenting = 4
options no_unused_block_arguments = false
options no_unused_function_arguments = false
options strict_smart_pointers = true

module strings_convert shared public

//! Soft-failing string-to-numeric conversions returning ``Result<T; ConversionError>``.
//!
//! Companion to the panicking ``string_to_int*`` builtins and the silent-fail
//! ``to_int*`` family. Use ``try_to_*`` when you need to distinguish
//! ``invalid_argument`` (not a number), ``out_of_range`` (overflow), and
//! ``trailing_garbage`` (``"123abc"``).
//!
//! Leading whitespace is consumed; trailing characters - including whitespace -
//! yield ``trailing_garbage``. Hex variants accept an optional ``0x``/``0X``
//! prefix when ``hex`` is ``true``.

require strings public
require daslib/result public

//! Reason a ``try_to_*`` conversion failed.
enum public ConversionError {
    invalid_argument
    out_of_range
    trailing_garbage
}

def private classify(result : ConversionResult; offset : int; full_length : int) : ConversionError {
    if (result == ConversionResult.out_of_range) {
        return ConversionError.out_of_range
    }
    if (result == ConversionResult.ok && offset != full_length) {
        return ConversionError.trailing_garbage
    }
    return ConversionError.invalid_argument
}

def public try_to_int8(str : string implicit; hex : bool = false) : Result<int8; ConversionError> {
    //! Parse ``str`` as a signed 8-bit integer.
    var result : ConversionResult
    var offset = 0
    let parsed = int8(str, result, offset, hex)
    let len = length(str)
    if (result == ConversionResult.ok && offset == len) {
        return ok(parsed, type<ConversionError>)
    }
    return err(classify(result, offset, len), type<int8>)
}

def public try_to_uint8(str : string implicit; hex : bool = false) : Result<uint8; ConversionError> {
    //! Parse ``str`` as an unsigned 8-bit integer.
    var result : ConversionResult
    var offset = 0
    let parsed = uint8(str, result, offset, hex)
    let len = length(str)
    if (result == ConversionResult.ok && offset == len) {
        return ok(parsed, type<ConversionError>)
    }
    return err(classify(result, offset, len), type<uint8>)
}

def public try_to_int16(str : string implicit; hex : bool = false) : Result<int16; ConversionError> {
    //! Parse ``str`` as a signed 16-bit integer.
    var result : ConversionResult
    var offset = 0
    let parsed = int16(str, result, offset, hex)
    let len = length(str)
    if (result == ConversionResult.ok && offset == len) {
        return ok(parsed, type<ConversionError>)
    }
    return err(classify(result, offset, len), type<int16>)
}

def public try_to_uint16(str : string implicit; hex : bool = false) : Result<uint16; ConversionError> {
    //! Parse ``str`` as an unsigned 16-bit integer.
    var result : ConversionResult
    var offset = 0
    let parsed = uint16(str, result, offset, hex)
    let len = length(str)
    if (result == ConversionResult.ok && offset == len) {
        return ok(parsed, type<ConversionError>)
    }
    return err(classify(result, offset, len), type<uint16>)
}

def public try_to_int(str : string implicit; hex : bool = false) : Result<int; ConversionError> {
    //! Parse ``str`` as a signed 32-bit integer.
    var result : ConversionResult
    var offset = 0
    let parsed = int(str, result, offset, hex)
    let len = length(str)
    if (result == ConversionResult.ok && offset == len) {
        return ok(parsed, type<ConversionError>)
    }
    return err(classify(result, offset, len), type<int>)
}

def public try_to_uint(str : string implicit; hex : bool = false) : Result<uint; ConversionError> {
    //! Parse ``str`` as an unsigned 32-bit integer.
    var result : ConversionResult
    var offset = 0
    let parsed = uint(str, result, offset, hex)
    let len = length(str)
    if (result == ConversionResult.ok && offset == len) {
        return ok(parsed, type<ConversionError>)
    }
    return err(classify(result, offset, len), type<uint>)
}

def public try_to_int64(str : string implicit; hex : bool = false) : Result<int64; ConversionError> {
    //! Parse ``str`` as a signed 64-bit integer.
    var result : ConversionResult
    var offset = 0
    let parsed = int64(str, result, offset, hex)
    let len = length(str)
    if (result == ConversionResult.ok && offset == len) {
        return ok(parsed, type<ConversionError>)
    }
    return err(classify(result, offset, len), type<int64>)
}

def public try_to_uint64(str : string implicit; hex : bool = false) : Result<uint64; ConversionError> {
    //! Parse ``str`` as an unsigned 64-bit integer.
    var result : ConversionResult
    var offset = 0
    let parsed = uint64(str, result, offset, hex)
    let len = length(str)
    if (result == ConversionResult.ok && offset == len) {
        return ok(parsed, type<ConversionError>)
    }
    return err(classify(result, offset, len), type<uint64>)
}

def public try_to_float(str : string implicit) : Result<float; ConversionError> {
    //! Parse ``str`` as a 32-bit float.
    var result : ConversionResult
    var offset = 0
    let parsed = float(str, result, offset)
    let len = length(str)
    if (result == ConversionResult.ok && offset == len) {
        return ok(parsed, type<ConversionError>)
    }
    return err(classify(result, offset, len), type<float>)
}

def public try_to_double(str : string implicit) : Result<double; ConversionError> {
    //! Parse ``str`` as a 64-bit float.
    var result : ConversionResult
    var offset = 0
    let parsed = double(str, result, offset)
    let len = length(str)
    if (result == ConversionResult.ok && offset == len) {
        return ok(parsed, type<ConversionError>)
    }
    return err(classify(result, offset, len), type<double>)
}
