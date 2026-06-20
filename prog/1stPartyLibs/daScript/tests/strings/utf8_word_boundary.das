options gen2

require dastest/testing_boost
require daslib/utf8_utils


[test]
def test_utf32_is_word_char(t : T?) {
    t |> run("ASCII alphanumeric + underscore are word chars") @(t : T?) {
        t |> equal(utf32_is_word_char(uint('a')), true)
        t |> equal(utf32_is_word_char(uint('Z')), true)
        t |> equal(utf32_is_word_char(uint('0')), true)
        t |> equal(utf32_is_word_char(uint('9')), true)
        t |> equal(utf32_is_word_char(uint('_')), true)
    }
    t |> run("ASCII punctuation + whitespace are non-word") @(t : T?) {
        t |> equal(utf32_is_word_char(uint(' ')), false)
        t |> equal(utf32_is_word_char(uint('.')), false)
        t |> equal(utf32_is_word_char(uint(',')), false)
        t |> equal(utf32_is_word_char(uint(';')), false)
        t |> equal(utf32_is_word_char(uint('-')), false)
        t |> equal(utf32_is_word_char(uint('!')), false)
        t |> equal(utf32_is_word_char(uint('@')), false)
        t |> equal(utf32_is_word_char(uint('[')), false)
        t |> equal(utf32_is_word_char(uint('`')), false)
    }
    t |> run("Latin-1 letters are word chars") @(t : T?) {
        t |> equal(utf32_is_word_char(0x00C0u), true)   // À
        t |> equal(utf32_is_word_char(0x00DFu), true)   // ß
        t |> equal(utf32_is_word_char(0x00E9u), true)   // é
        t |> equal(utf32_is_word_char(0x00F1u), true)   // ñ
        t |> equal(utf32_is_word_char(0x00FFu), true)   // ÿ
    }
    t |> run("Latin-1 punctuation/symbols are non-word") @(t : T?) {
        t |> equal(utf32_is_word_char(0x00A0u), false)  // no-break space
        t |> equal(utf32_is_word_char(0x00A1u), false)  // ¡
        t |> equal(utf32_is_word_char(0x00BFu), false)  // ¿
        t |> equal(utf32_is_word_char(0x00D7u), false)  // ×
        t |> equal(utf32_is_word_char(0x00F7u), false)  // ÷
    }
    t |> run("Greek + Cyrillic letters are word chars") @(t : T?) {
        t |> equal(utf32_is_word_char(0x0391u), true)   // Α
        t |> equal(utf32_is_word_char(0x03B1u), true)   // α
        t |> equal(utf32_is_word_char(0x0410u), true)   // А
        t |> equal(utf32_is_word_char(0x044Fu), true)   // я
        t |> equal(utf32_is_word_char(0x0401u), true)   // Ё
    }
    t |> run("CJK ideographs are word chars") @(t : T?) {
        t |> equal(utf32_is_word_char(0x4E2Du), true)   // 中
        t |> equal(utf32_is_word_char(0x6587u), true)   // 文
        t |> equal(utf32_is_word_char(0x65E5u), true)   // 日
    }
    t |> run("CJK punctuation is non-word") @(t : T?) {
        t |> equal(utf32_is_word_char(0x3000u), false)  // ideographic space
        t |> equal(utf32_is_word_char(0x3001u), false)  // ideographic comma
        t |> equal(utf32_is_word_char(0x3002u), false)  // ideographic period
        t |> equal(utf32_is_word_char(0x300Au), false)  // 《 left double angle bracket
    }
    t |> run("General punctuation block is non-word") @(t : T?) {
        t |> equal(utf32_is_word_char(0x2014u), false)  // — em dash
        t |> equal(utf32_is_word_char(0x2018u), false)  // ' left single quote
        t |> equal(utf32_is_word_char(0x201Cu), false)  // " left double quote
        t |> equal(utf32_is_word_char(0x2026u), false)  // … horizontal ellipsis
    }
    t |> run("Math + arrow + symbol blocks are non-word") @(t : T?) {
        t |> equal(utf32_is_word_char(0x2192u), false)  // → rightwards arrow
        t |> equal(utf32_is_word_char(0x2200u), false)  // ∀ for all
        t |> equal(utf32_is_word_char(0x2603u), false)  // ☃ snowman
    }
    t |> run("Fullwidth ASCII punctuation is non-word") @(t : T?) {
        t |> equal(utf32_is_word_char(0xFF01u), false)  // ！
        t |> equal(utf32_is_word_char(0xFF0Cu), false)  // ，
        t |> equal(utf32_is_word_char(0xFF1Fu), false)  // ？
    }
    t |> run("Fullwidth letters and digits remain word chars") @(t : T?) {
        t |> equal(utf32_is_word_char(0xFF21u), true)   // Ａ
        t |> equal(utf32_is_word_char(0xFF41u), true)   // ａ
        t |> equal(utf32_is_word_char(0xFF10u), true)   // ０
    }
}


[test]
def test_utf32_to_lower(t : T?) {
    t |> run("ASCII case fold") @(t : T?) {
        t |> equal(utf32_to_lower(uint('A')), uint('a'))
        t |> equal(utf32_to_lower(uint('Z')), uint('z'))
        t |> equal(utf32_to_lower(uint('a')), uint('a'))
        t |> equal(utf32_to_lower(uint('z')), uint('z'))
        t |> equal(utf32_to_lower(uint('0')), uint('0'))
        t |> equal(utf32_to_lower(uint(' ')), uint(' '))
    }
    t |> run("Latin-1 supplement folds") @(t : T?) {
        t |> equal(utf32_to_lower(0x00C0u), 0x00E0u)    // À → à
        t |> equal(utf32_to_lower(0x00C9u), 0x00E9u)    // É → é
        t |> equal(utf32_to_lower(0x00D1u), 0x00F1u)    // Ñ → ñ
        t |> equal(utf32_to_lower(0x00DEu), 0x00FEu)    // Þ → þ
    }
    t |> run("Latin-1 special-case characters do not fold") @(t : T?) {
        t |> equal(utf32_to_lower(0x00DFu), 0x00DFu)    // ß stays ß
        t |> equal(utf32_to_lower(0x00D7u), 0x00D7u)    // × stays ×
        t |> equal(utf32_to_lower(0x00F7u), 0x00F7u)    // ÷ stays ÷
    }
    t |> run("Latin Extended-A folds") @(t : T?) {
        t |> equal(utf32_to_lower(0x0100u), 0x0101u)    // Ā → ā
        t |> equal(utf32_to_lower(0x0139u), 0x013Au)    // Ĺ → ĺ
        t |> equal(utf32_to_lower(0x0178u), 0x00FFu)    // Ÿ → ÿ
    }
    t |> run("Greek capitals fold") @(t : T?) {
        t |> equal(utf32_to_lower(0x0391u), 0x03B1u)    // Α → α
        t |> equal(utf32_to_lower(0x03A9u), 0x03C9u)    // Ω → ω
    }
    t |> run("Cyrillic capitals fold") @(t : T?) {
        t |> equal(utf32_to_lower(0x0410u), 0x0430u)    // А → а
        t |> equal(utf32_to_lower(0x042Fu), 0x044Fu)    // Я → я
        t |> equal(utf32_to_lower(0x0401u), 0x0451u)    // Ё → ё
    }
    t |> run("CJK / out-of-range codepoints pass through") @(t : T?) {
        t |> equal(utf32_to_lower(0x4E2Du), 0x4E2Du)    // 中
        t |> equal(utf32_to_lower(0x10000u), 0x10000u)  // beyond BMP
    }
}


[test]
def test_each_word(t : T?) {
    t |> run("ASCII tokenization") @(t : T?) {
        var words : array<string>
        each_word("the quick brown fox", words)
        t |> equal(length(words), 4)
        t |> equal(words[0], "the")
        t |> equal(words[1], "quick")
        t |> equal(words[2], "brown")
        t |> equal(words[3], "fox")
    }
    t |> run("Punctuation splits tokens") @(t : T?) {
        var words : array<string>
        each_word("hello, world! foo.bar-baz_qux", words)
        t |> equal(length(words), 5)
        t |> equal(words[0], "hello")
        t |> equal(words[1], "world")
        t |> equal(words[2], "foo")
        t |> equal(words[3], "bar")
        t |> equal(words[4], "baz_qux")
    }
    t |> run("Case folding lowercases ASCII") @(t : T?) {
        var words : array<string>
        each_word("The Quick BROWN Fox", words)
        t |> equal(words[0], "the")
        t |> equal(words[1], "quick")
        t |> equal(words[2], "brown")
        t |> equal(words[3], "fox")
    }
    t |> run("Latin-1 case folding") @(t : T?) {
        var words : array<string>
        each_word("Élève NIÑO café", words)
        t |> equal(length(words), 3)
        t |> equal(words[0], "élève")
        t |> equal(words[1], "niño")
        t |> equal(words[2], "café")
    }
    t |> run("Cyrillic case folding") @(t : T?) {
        var words : array<string>
        each_word("Привет Мир", words)
        t |> equal(length(words), 2)
        t |> equal(words[0], "привет")
        t |> equal(words[1], "мир")
    }
    t |> run("CJK ideographs tokenize on punctuation") @(t : T?) {
        var words : array<string>
        each_word("中文，日本語", words)
        t |> equal(length(words), 2)
        t |> equal(words[0], "中文")
        t |> equal(words[1], "日本語")
    }
    t |> run("Empty input produces no tokens") @(t : T?) {
        var words : array<string>
        each_word("", words)
        t |> equal(length(words), 0)
    }
    t |> run("Whitespace-only input produces no tokens") @(t : T?) {
        var words : array<string>
        each_word("   \t\n  ", words)
        t |> equal(length(words), 0)
    }
    t |> run("Output buffer is appended, not cleared") @(t : T?) {
        var words : array<string>
        words |> push("seed")
        each_word("hello world", words)
        t |> equal(length(words), 3)
        t |> equal(words[0], "seed")
        t |> equal(words[1], "hello")
        t |> equal(words[2], "world")
    }
}
