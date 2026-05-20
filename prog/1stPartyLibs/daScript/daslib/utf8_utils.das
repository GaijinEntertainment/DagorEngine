options gen2
options indenting = 4
options strict_smart_pointers = true

module utf8_utils shared

//! UTF-8 / UTF-16 / UTF-32 encoding utilities.
//!
//! Provides ``utf8_encode``, ``utf8_decode``, ``utf16_to_utf32``, and
//! ``each_utf32`` for converting between Unicode representations, plus
//! Unicode-aware word-boundary primitives (``utf32_is_word_char``,
//! ``utf32_to_lower``, ``each_word``) used by ``daslib/fts5_query`` and
//! other tokenizers.

require daslib/strings_boost
require math

def public utf16_to_utf32(high, low : uint) {
    //! Converts a UTF-16 surrogate pair to a single UTF-32 codepoint.
    if (high >= 0xD800 && high <= 0xDBFF) {
        return ((high - 0xD800) << 10u) + (low - 0xDC00) + 0x10000
    } else {
        return high
    }
}

def public utf8_encode(var dest_array : array<uint8>&; ch : uint) {
    //! Converts a codepoint (UTF-32 symbol) to UTF-8 and appends it to the UTF-8 byte array
    if (ch < 0x80) {
        dest_array |> push(uint8(ch))
    } elif (ch < 0x800) {
        dest_array |> push(uint8(0xC0 + (ch >> 6u)))
        dest_array |> push(uint8(0x80 + (ch & 0x3F)))
    } elif (ch < 0x10000) {
        dest_array |> push(uint8(0xE0 + (ch >> 12u)))
        dest_array |> push(uint8(0x80 + ((ch >> 6u) & 0x3F)))
        dest_array |> push(uint8(0x80 + (ch & 0x3F)))
    } else {
        dest_array |> push(uint8(0xF0 + (ch >> 18u)))
        dest_array |> push(uint8(0x80 + ((ch >> 12u) & 0x3F)))
        dest_array |> push(uint8(0x80 + ((ch >> 6u) & 0x3F)))
        dest_array |> push(uint8(0x80 + (ch & 0x3F)))
    }
}


def public utf8_encode(ch : uint) : array<uint8> {
    //! Converts a codepoint (UTF-32 symbol) to the UTF-8 byte array
    if (ch < 0x80) {
        return <-  [ uint8(ch) ]
    } elif (ch < 0x800) {
        return <-  [
            uint8(0xC0 + (ch >> 6u)),
            uint8(0x80 + (ch & 0x3F)) ]
    } elif (ch < 0x10000) {
        return <-  [
            uint8(0xE0 + (ch >> 12u)),
            uint8(0x80 + ((ch >> 6u) & 0x3F)),
            uint8(0x80 + (ch & 0x3F)) ]
    } else {
        return <-  [
            uint8(0xF0 + (ch >> 18u)),
            uint8(0x80 + ((ch >> 12u) & 0x3F)),
            uint8(0x80 + ((ch >> 6u) & 0x3F)),
            uint8(0x80 + (ch & 0x3F)) ]
    }
}


def public utf8_encode(var dest_array : array<uint8>&; source_utf32_string : array<uint> const implicit) {
    //! Converts UTF-32 string to UTF-8 and appends it to the UTF-8 byte array
    for (ch in source_utf32_string) {
        dest_array |> utf8_encode(ch)
    }
}


def public utf8_encode(source_utf32_string : array<uint> const implicit) : array<uint8> {
    //! Converts UTF-32 string to UTF-8 and returns it as a UTF-8 byte array
    var dest_array : array<uint8>
    dest_array |> reserve(length(source_utf32_string))
    for (ch in source_utf32_string) {
        dest_array |> utf8_encode(ch)
    }
    return <- dest_array
}


def public utf8_length(utf8_string : array<uint8> const implicit) : int {
    //! Returns the number of characters in the UTF-8 string
    var length = 0
    for (ch in utf8_string) {
        if ((uint(ch) & 0xC0) != 0x80) {
            length++
        }
    }
    return length
}


def public utf8_length(utf8_string : string const) : int {
    //! Returns the number of characters in the UTF-8 string
    var length = 0
    for (ch in utf8_string) {
        if ((uint(ch) & 0xC0) != 0x80) {
            length++
        }
    }
    return length
}


def public is_first_byte_of_utf8_char(ch : uint8) : bool {
    //! Returns true if the given byte is the first byte of a UTF-8 character.
    let x = uint(ch)
    return (x > 0u) && (x < 128u || (x & 0xC0) == 0xC0)
}


def public contains_utf8_bom(utf8_string : array<uint8> const implicit) : bool {
    //! Returns true if the byte array starts with a UTF-8 BOM (byte order mark).
    return (length(utf8_string) >= 3 &&
          uint(utf8_string[0]) == 0xEF && uint(utf8_string[1]) == 0xBB && uint(utf8_string[2]) == 0xBF)
}


def public contains_utf8_bom(utf8_string : string const) : bool {
    //! Returns true if the string starts with a UTF-8 BOM (byte order mark).
    unsafe {
        return (!empty(utf8_string) &&
              uint(utf8_string |> character_uat(0)) == 0xEF &&
              uint(utf8_string |> character_uat(1)) == 0xBB &&
              uint(utf8_string |> character_uat(2)) == 0xBF)
    }
}



// Copyright (c) 2008-2009 Bjoern Hoehrmann <bjoern@hoehrmann.de>
// See http://bjoern.hoehrmann.de/utf-8/decoder/dfa/ for details.

let shared s_utf8d <-  fixed_array( //! Byte-class and state-transition table for the UTF-8 DFA decoder.
    // The first part of the table maps bytes to character classes that
    // to reduce the size of the transition table and create bitmasks.
   0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u,  0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u,
   0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u,  0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u,
   0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u,  0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u,
   0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u,  0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u,
   1u, 1u, 1u, 1u, 1u, 1u, 1u, 1u, 1u, 1u, 1u, 1u, 1u, 1u, 1u, 1u,  9u, 9u, 9u, 9u, 9u, 9u, 9u, 9u, 9u, 9u, 9u, 9u, 9u, 9u, 9u, 9u,
   7u, 7u, 7u, 7u, 7u, 7u, 7u, 7u, 7u, 7u, 7u, 7u, 7u, 7u, 7u, 7u,  7u, 7u, 7u, 7u, 7u, 7u, 7u, 7u, 7u, 7u, 7u, 7u, 7u, 7u, 7u, 7u,
   8u, 8u, 2u, 2u, 2u, 2u, 2u, 2u, 2u, 2u, 2u, 2u, 2u, 2u, 2u, 2u,  2u, 2u, 2u, 2u, 2u, 2u, 2u, 2u, 2u, 2u, 2u, 2u, 2u, 2u, 2u, 2u,
  10u, 3u, 3u, 3u, 3u, 3u, 3u, 3u, 3u, 3u, 3u, 3u, 3u, 4u, 3u, 3u, 11u, 6u, 6u, 6u, 5u, 8u, 8u, 8u, 8u, 8u, 8u, 8u, 8u, 8u, 8u, 8u,

  // The second part is a transition table that maps a combination
  // of a state of the automaton and a character class to a state.
   0u, 12u, 24u, 36u, 60u, 96u, 84u, 12u, 12u, 12u, 48u, 72u, 12u, 12u, 12u, 12u, 12u, 12u, 12u, 12u, 12u, 12u, 12u, 12u,
  12u, 0u, 12u, 12u, 12u, 12u, 12u, 0u, 12u, 0u, 12u, 12u, 12u, 24u, 12u, 12u, 12u, 12u, 12u, 24u, 12u, 24u, 12u, 12u,
  12u, 12u, 12u, 12u, 12u, 12u, 12u, 24u, 12u, 12u, 12u, 12u, 12u, 24u, 12u, 12u, 12u, 12u, 12u, 12u, 12u, 24u, 12u, 12u,
  12u, 12u, 12u, 12u, 12u, 12u, 12u, 36u, 12u, 36u, 12u, 12u, 12u, 36u, 12u, 12u, 12u, 12u, 12u, 36u, 12u, 36u, 12u, 12u,
  12u, 36u, 12u, 12u, 12u, 12u, 12u, 12u, 12u, 12u, 12u, 12u
);

let UTF8_ACCEPT = 0u //! DFA accept state indicating a valid UTF-8 sequence.

def public is_utf8_string_valid(utf8_string : array<uint8> const implicit) : bool {
    //! Returns true if the byte array contains a valid UTF-8 encoded string.
    var codepoint = 0u
    var state = 0u
    for (ch in utf8_string) {
        let byte = uint(ch)
        let type_ = s_utf8d[byte]
        codepoint = (state != UTF8_ACCEPT) ? (byte & 0x3F) | (codepoint << 6u) : (0xFF >> type_) & byte
        state = s_utf8d[256u + state + type_]
    }
    return state == UTF8_ACCEPT
}


def public is_utf8_string_valid(utf8_string : string const) : bool {
    //! Returns true if the string contains valid UTF-8 encoded data.
    var codepoint = 0u
    var state = 0u
    for (ch in utf8_string) {
        let byte = uint(ch)
        let type_ = s_utf8d[byte]
        codepoint = (state != UTF8_ACCEPT) ? (byte & 0x3F) | (codepoint << 6u) : (0xFF >> type_) & byte
        state = s_utf8d[256u + state + type_]
    }
    return state == UTF8_ACCEPT
}


def public utf8_decode(var dest_utf32_string : array<uint>&; source_utf8_string : array<uint8> const implicit) {
    //! Converts UTF-8 string to UTF-32 and appends it to the array of codepoints (UTF-32 string)
    var codepoint = 0u
    var state = 0u
    let length = length(source_utf8_string)
    var i = contains_utf8_bom(source_utf8_string) ? 3 : 0

    while (i < length) {
        let byte = uint(source_utf8_string[i++])
        let type_ = s_utf8d[byte]
          codepoint = (state != UTF8_ACCEPT) ? (byte & 0x3F) | (codepoint << 6u) : (0xFF >> type_) & byte
        state = s_utf8d[256u + state + type_]
        if (state == UTF8_ACCEPT) {
            dest_utf32_string |> push(codepoint)
        }
    }
}


def public utf8_decode(source_utf8_string : array<uint8> const implicit) : array<uint> {
    //! Converts UTF-8 string to UTF-32 and returns it as an array of codepoints (UTF-32 string)
    var dest_utf32_string : array<uint>
    dest_utf32_string |> utf8_decode(source_utf8_string)
    return <- dest_utf32_string
}


def public utf8_decode(source_utf8_string : string const) : array<uint> {
    //! Converts UTF-8 string to UTF-32 and returns it as an array of codepoints (UTF-32 string)
    var dest_utf32_string : array<uint>
    peek_data(source_utf8_string) $(arr) {
        dest_utf32_string |> utf8_decode(arr)
    }
    return <- dest_utf32_string
}


def public utf8_decode(var dest_utf32_string : array<uint>; source_utf8_string : string const) {
    //! Converts UTF-8 string to UTF-32 and appends it to the array of codepoints (UTF-32 string)
    peek_data(source_utf8_string) $(arr) {
        dest_utf32_string |> utf8_decode(arr)
    }
}

// ===== Unicode word-boundary primitives =====
// Sorted (start, end_inclusive) NON-WORD ranges (BMP subset); everything else is a word char.
let _UTF32_NON_WORD_RANGES <- fixed_array(
    0x0000u, 0x002Fu,
    0x003Au, 0x0040u,
    0x005Bu, 0x005Eu,   // [ \ ] ^   (0x005F = '_' is a word char, kept out)
    0x0060u, 0x0060u,
    0x007Bu, 0x00A9u,
    0x00ABu, 0x00B4u,
    0x00B6u, 0x00B9u,
    0x00BBu, 0x00BFu,
    0x00D7u, 0x00D7u,
    0x00F7u, 0x00F7u,
    0x2000u, 0x206Fu,
    0x2190u, 0x2BFFu,
    0x3000u, 0x3004u,
    0x3008u, 0x3011u,
    0x3014u, 0x301Fu,
    0xFE30u, 0xFE6Fu,
    0xFF01u, 0xFF0Fu,
    0xFF1Au, 0xFF20u,
    0xFF3Bu, 0xFF40u,
    0xFF5Bu, 0xFF65u
)

def public utf32_is_word_char(cp : uint) : bool {
    //! ASCII `[A-Za-z0-9_]` fast path; otherwise consults a non-word range table.
    //! Pragmatic BMP subset — full UCD categorization is out of scope.
    if (cp < 0x80u) {
        return ((cp >= 0x30u && cp <= 0x39u) ||
                (cp >= 0x41u && cp <= 0x5Au) ||
                (cp >= 0x61u && cp <= 0x7Au) ||
                cp == 0x5Fu)
    }
    let n = length(_UTF32_NON_WORD_RANGES)
    var i = 0
    while (i < n) {
        let lo = _UTF32_NON_WORD_RANGES[i]
        let hi = _UTF32_NON_WORD_RANGES[i + 1]
        if (cp < lo) {
            return true
        }
        if (cp <= hi) {
            return false
        }
        i += 2
    }
    return true
}

// Latin-1 simple lowercase (0xC0..0xFF). Index = cp - 0xC0.
let _UTF32_LATIN1_LOWER <- fixed_array(
    0x00E0u, 0x00E1u, 0x00E2u, 0x00E3u, 0x00E4u, 0x00E5u, 0x00E6u, 0x00E7u,
    0x00E8u, 0x00E9u, 0x00EAu, 0x00EBu, 0x00ECu, 0x00EDu, 0x00EEu, 0x00EFu,
    0x00F0u, 0x00F1u, 0x00F2u, 0x00F3u, 0x00F4u, 0x00F5u, 0x00F6u, 0x00D7u, // 0xD7 = × (no fold)
    0x00F8u, 0x00F9u, 0x00FAu, 0x00FBu, 0x00FCu, 0x00FDu, 0x00FEu, 0x00DFu, // 0xDF = ß (no fold)
    0x00E0u, 0x00E1u, 0x00E2u, 0x00E3u, 0x00E4u, 0x00E5u, 0x00E6u, 0x00E7u,
    0x00E8u, 0x00E9u, 0x00EAu, 0x00EBu, 0x00ECu, 0x00EDu, 0x00EEu, 0x00EFu,
    0x00F0u, 0x00F1u, 0x00F2u, 0x00F3u, 0x00F4u, 0x00F5u, 0x00F6u, 0x00F7u, // 0xF7 = ÷ (no fold)
    0x00F8u, 0x00F9u, 0x00FAu, 0x00FBu, 0x00FCu, 0x00FDu, 0x00FEu, 0x00FFu
)

def public utf32_to_lower(cp : uint) : uint {
    //! Lowercases a codepoint. Covers ASCII, Latin-1 supplement, Latin Extended-A,
    //! Greek capitals, Cyrillic capitals; other codepoints pass through unchanged.
    if (cp >= 0x41u && cp <= 0x5Au) {
        return cp + 0x20u
    }
    if (cp < 0xC0u) {
        return cp
    }
    if (cp <= 0xFFu) {
        return _UTF32_LATIN1_LOWER[cp - 0xC0u]
    }
    // Latin Extended-A: 0x100..0x17F. Mostly upper/lower paired at
    // even/odd codepoints. Exceptions handled inline.
    if (cp >= 0x100u && cp <= 0x137u) {
        return ((cp & 1u) == 0u) ? cp + 1u : cp
    }
    if (cp >= 0x139u && cp <= 0x148u) {
        return ((cp & 1u) == 1u) ? cp + 1u : cp
    }
    if (cp >= 0x14Au && cp <= 0x177u) {
        return ((cp & 1u) == 0u) ? cp + 1u : cp
    }
    if (cp == 0x178u) {
        return 0xFFu  // Ÿ → ÿ
    }
    if (cp >= 0x179u && cp <= 0x17Eu) {
        return ((cp & 1u) == 1u) ? cp + 1u : cp
    }
    // Greek: Α-Ω (0x391..0x3A9) → α-ω (0x3B1..0x3C9), excluding 0x3A2 (reserved).
    if (cp >= 0x391u && cp <= 0x3A1u) {
        return cp + 0x20u
    }
    if (cp >= 0x3A3u && cp <= 0x3ABu) {
        return cp + 0x20u
    }
    // Cyrillic: А-Я (0x410..0x42F) → а-я (0x430..0x44F).
    if (cp >= 0x410u && cp <= 0x42Fu) {
        return cp + 0x20u
    }
    // Cyrillic: Ё (0x401) → ё (0x451), plus Ѐ-Џ (0x400..0x40F) → ѐ-џ (0x450..0x45F).
    if (cp >= 0x400u && cp <= 0x40Fu) {
        return cp + 0x50u
    }
    return cp
}

def public each_word(text : string; var out : array<string>) {
    //! Tokenizes UTF-8 into lower-cased word tokens (uses `utf32_is_word_char`
    //! for boundaries, `utf32_to_lower` for case fold). Appends to `out`.
    let codepoints = utf8_decode(text)
    let n = length(codepoints)
    var i = 0
    var token_buf : array<uint8>
    token_buf |> reserve(64)
    while (i < n) {
        while (i < n && !utf32_is_word_char(codepoints[i])) {
            i++
        }
        if (i >= n) {
            break
        }
        token_buf |> resize(0)
        while (i < n && utf32_is_word_char(codepoints[i])) {
            token_buf |> utf8_encode(utf32_to_lower(codepoints[i]))
            i++
        }
        out |> push(string(token_buf))
    }
}

def decode_unicode_escape(str : string) {
    //! Decodes Unicode escape sequences (backslash followed by hex digits) in a string to UTF-8.
    return build_string() $(writer) {
        peek_data(str) $(p) {
            var i = 0
            let llen = p |> length
            var bang : array<uint8>
            bang |> reserve(8)
            var high_surrogate = 0u
            while (i < llen) {
                if (p[i] == 0x5cu8) {
                    var j = i + 1
                    let jlen = min(llen, j + 8)
                    bang |> resize(0)
                    while (j < jlen && is_hex(int(p[j]))) {
                        bang |> push(p[j])
                        j ++
                    }
                    if (length(bang) == 4) {
                        let utf32 = to_uint(string(bang), true)
                        if (utf32 >= 0xD800 && utf32 <= 0xDFFF) {
                            if (high_surrogate == 0u) {
                                high_surrogate = utf32
                            } else {
                                let low_surrogate = utf32
                                let pair = 0x10000 + ((high_surrogate - 0xD800) << 10u) + (low_surrogate - 0xDC00)
                                high_surrogate = 0u
                                writer |> write(string(utf8_encode(pair)))
                            }
                        } else {
                            writer |> write(string(utf8_encode(utf32)))
                        }
                        i = j
                        continue
                    }
                }
                writer |> write_char(int(p[i]))
                i += 1
            }
        }
    }
}
