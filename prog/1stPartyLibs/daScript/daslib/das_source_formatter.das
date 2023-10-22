options indenting = 4
options no_unused_block_arguments = false
options no_unused_function_arguments = false
options strict_smart_pointers = true

module das_source_formatter shared private

require fio
require strings
require math
require daslib/defer


let sorted_tokens_source <- [[auto "<<<="; ">>>=";
    "<<="; ">>="; "&&="; "||="; "^^="; "<<<"; ">>>";
    "::"; "+="; "-="; "/="; "*="; "%="; "|="; "&="; "^="; "<<"; ">>"; "++"; "--"; "<="; ">="; "==";
    "!="; "->"; "<-"; "??"; "?."; "?["; "<|"; "|>"; ":="; "=>"; "@@"; "&&"; "||"; "^^";
    "%%"
    ]]

let dont_need_space_around <- [[auto "("; ")"; "["; "]"; "\{"; "\}";
    "."; ";"; ","; "`"; "::"; "++"; "--"; "?."; "?["; "@@"; "!"; "~"; "#"; "->"
    ]]

let type_after_keyword <- [[auto "generator"; "cast"; "upcast"; "smart_ptr"; "match_type";
    "type"; "reinterpret"; "variant"; "variant_index"; "function"
    ]]

enum TokenType
    UNKNOWN
    KEYWORD_OR_IDENTIFIER
    NUMBER
    COMMENT
    STRING


struct TokenTemplate
    firstChar: int = 0
    str: string = ""
    codes: array<uint8>
    length: int = 0

struct Token
    str: string
    spaces: int = 0
    newLines: int = 0
    lineInSource: int = 1
    column: int = 1
    tokenType: TokenType = TokenType UNKNOWN
    isInFunctionParam: bool = false
    isInType: bool = false
    dontFormat: bool = false
    dontAddSpacesAround: bool = false


struct FormatterToken
    tokenIndex: int
    command: string

struct FormatterCtx
    tokenTemplates: array<TokenTemplate>
    tokens: array<Token>
    formatterTokens: array<FormatterToken>
    pos: int             // current char pos
    c: int               // current char
    data: array<uint8>   // file data as array of uints
    eof: bool
    haveUtf8Bom: bool
    newLineCounter: int  // new lines before current token
    spaceCounter: int    // space characters before current token
    srcLine: int         // current line in source
    curColumn: int       // current column in source
    curLineIndex: int    // index of the beginning of new line in data[]
    debugMode: bool = false
    indenting: int = 0   // convert tab to N spaces
    insideOptions: bool = false
    crCount: int = 0     // CR \r
    lfCount: int = 0     // LF \n

def initialize_token_templates(var ctx: FormatterCtx)
    if length(ctx.tokenTemplates) > 0
        return
    ctx.tokenTemplates |> resize(length(sorted_tokens_source))
    for s, tt in sorted_tokens_source, ctx.tokenTemplates
        tt.str = s
        tt.codes <- [{for c in s; uint8(c)}]
        tt.firstChar = int(tt.codes[0])
        tt.length = length(s)


def isUtf8Bom(var text: array<uint8>&): bool
    return (length(text) >= 3 &&
        uint(text[0]) == 0xEF && uint(text[1]) == 0xBB && uint(text[2]) == 0xBF)


def eq(ctx: FormatterCtx; index: int; s: string const implicit)
    return (index < 0 || index >= length(ctx.tokens)) ? false : ctx.tokens[index].str == s


def new_line_before(ctx: FormatterCtx; index: int)
    return (index <= 1 || index >= length(ctx.tokens)) ? true : ctx.tokens[index].newLines > 0


def next_char(var ctx: FormatterCtx)
    ctx.c = int(ctx.data[ctx.pos])
    if ctx.c == 0
        ctx.eof = true
        return

    if ctx.c == '\r' && int(ctx.data[ctx.pos + 1]) == '\n'
        ctx.crCount++
        ctx.pos++
        ctx.c = int(ctx.data[ctx.pos])

    if ctx.c == ' '
        ctx.spaceCounter++

    if ctx.c == '\t'
        let indent = ctx.indenting != 0 ? ctx.indenting : 4
        ctx.spaceCounter += indent
        ctx.curColumn += indent - 1

    if ctx.c == '\n'
        ctx.lfCount++
        ctx.pos++
        ctx.c = int(ctx.data[ctx.pos])
        ctx.curLineIndex = ctx.pos
        ctx.spaceCounter = 0
        ctx.newLineCounter++
        ctx.srcLine++
        ctx.curColumn = 0
        return
    else
        ctx.pos++
        ctx.c = int(ctx.data[ctx.pos])
        ctx.curColumn++
        return


def skip_spaces(var ctx: FormatterCtx)
    while (ctx.c == ' ' || ctx.c == '\t' || ctx.c == '\r' || ctx.c == '\n')
        ctx |> next_char()


def on_token_added(var ctx: FormatterCtx)
    assume t = ctx.tokens[length(ctx.tokens) - 1]
    if ctx.indenting == 0 && ctx.newLineCounter > 0 && ctx.spaceCounter > 0 && t.tokenType != TokenType COMMENT
        ctx.indenting = ctx.spaceCounter
    if ctx |> new_line_before(length(ctx.tokens) - 1)
        ctx.insideOptions = false
        if t.str == "options"
            ctx.insideOptions = true
    if (ctx.insideOptions && t.tokenType == TokenType NUMBER && length(ctx.tokens) >= 3 &&
            ctx.tokens[length(ctx.tokens) - 3].str == "indenting" && length(t.str) == 1)
        ctx.indenting = clamp(to_int(t.str), 1, 8)


def push_token(var ctx: FormatterCtx; from_char_idx, to_char_idx, space_count, new_lines, src_line, column: int; token_type: TokenType)
    var buf <- [{ for i in range(from_char_idx, to_char_idx); uint8(ctx.data[i]) }]
    ctx.tokens |> push([[ Token
        str = string(buf), spaces = space_count, newLines = new_lines,
        lineInSource = src_line, column = column, tokenType = token_type
        ]])

    ctx |> on_token_added()

    ctx.spaceCounter = 0
    ctx.newLineCounter = 0
finally
    delete buf


def parse_token(var ctx: FormatterCtx)
    ctx |> skip_spaces()
    if ctx.eof
        ctx.tokens |> push(ctx.tokens[0])
        return

    let from = ctx.pos
    let fromColumn = ctx.curColumn

    // keywords and identifiers
    if is_alpha(ctx.c) || ctx.c == '_' || ctx.c == '`'
        while !ctx.eof && (is_alpha(ctx.c) || is_number(ctx.c) || ctx.c == '_' || ctx.c == '`')
            ctx |> next_char()
        ctx |> push_token(from, ctx.pos, ctx.spaceCounter, ctx.newLineCounter, ctx.srcLine, fromColumn, TokenType KEYWORD_OR_IDENTIFIER)
        return

    // numbers
    if is_number(ctx.c) || (ctx.c == '.' && is_number(int(ctx.data[ctx.pos + 1])))
        while (!ctx.eof && (is_number(ctx.c) || is_alpha(ctx.c) || ctx.c == '.' ||
                ((ctx.c == '-' || ctx.c == '+') && (int(ctx.data[ctx.pos - 1]) == 'e' || int(ctx.data[ctx.pos - 1]) == 'E'))))
            ctx |> next_char()
        ctx |> push_token(from, ctx.pos, ctx.spaceCounter, ctx.newLineCounter, ctx.srcLine, fromColumn, TokenType NUMBER)
        return

    // characters and strings
    if ctx.c == '"' || ctx.c == int(0x27) // TODO: fix ' symbol
        let savedLine = ctx.newLineCounter
        let savedSpace = ctx.spaceCounter
        let openChar = ctx.c
        var depth = 0
        ctx |> next_char()
        while !ctx.eof && (ctx.c != openChar || depth > 0)
            if ctx.c == '{'
                depth++
            if ctx.c == '}'
                depth--
            if ctx.c == '\\'
                ctx |> next_char()
            ctx |> next_char()

        if (ctx.c == openChar)
            ctx |> next_char()

        ctx |> push_token(from, ctx.pos, savedSpace, savedLine, ctx.srcLine, fromColumn, TokenType STRING)
        return

    // single line comment
    if ctx.c == '/' && int(ctx.data[ctx.pos + 1]) == '/'
        let savedLine = ctx.newLineCounter
        let savedSpace = ctx.spaceCounter
        while !ctx.eof && ctx.c != '\n' && ctx.c != '\r'
            ctx |> next_char()
        ctx |> push_token(from, ctx.pos, savedSpace, savedLine, ctx.srcLine, fromColumn, TokenType COMMENT)
        return

    // multi-line comment
    if ctx.c == '/' && int(ctx.data[ctx.pos + 1]) == '*'
        let savedLine = ctx.newLineCounter
        let savedSpace = ctx.spaceCounter
        ctx |> next_char()
        ctx |> next_char()

        while !ctx.eof
            if ctx.c == '*' && int(ctx.data[ctx.pos + 1]) == '/'
                break
            ctx |> next_char()

        ctx |> next_char()
        ctx |> next_char()
        ctx |> push_token(from, ctx.pos, savedSpace, savedLine, ctx.srcLine, fromColumn, TokenType COMMENT)
        return

    // symbolic tokens
    for tt in ctx.tokenTemplates
        if ctx.c == tt.firstChar
            var match = true
            for i, code in range(ctx.pos, ctx.pos + tt.length), tt.codes
                if ctx.data[i] != code
                    match = false
                    break
            if match
                for i in range(0, tt.length)
                    ctx |> next_char()
                ctx |> push_token(from, ctx.pos, ctx.spaceCounter, ctx.newLineCounter, ctx.srcLine, fromColumn, TokenType UNKNOWN)
                return

    // other symbols
    ctx |> next_char()
    ctx |> push_token(from, ctx.pos, ctx.spaceCounter, ctx.newLineCounter, ctx.srcLine, fromColumn, TokenType UNKNOWN)
    return


def parse_all_tokens(var ctx: FormatterCtx)
    ctx |> initialize_token_templates()
    let emptyToken: Token
    ctx.tokens |> clear()
    ctx.tokens |> push(emptyToken)
    ctx.pos = 0
    ctx.eof = false
    ctx.haveUtf8Bom = false
    ctx.newLineCounter = 0
    ctx.spaceCounter = 0
    ctx.srcLine = 1
    ctx.curColumn = 0
    ctx.indenting = 0
    ctx.insideOptions = false
    ctx.crCount = 0
    ctx.lfCount = 0

    if isUtf8Bom(ctx.data)
        ctx.pos += 3
        ctx.haveUtf8Bom = true

    ctx.c = int(ctx.data[ctx.pos])

    ctx.curLineIndex = ctx.pos
    while !ctx.eof
        ctx |> parse_token()

    for i in range(8)
        ctx.tokens |> push(emptyToken)



struct ParenCounter
    angle: int = 0
    paren: int = 0
    square: int = 0
    curly: int = 0


def update_paren(var ctx: FormatterCtx; index: int; var paren_counter: ParenCounter&)
    if ctx |> eq(index, "<")
        paren_counter.angle++
    if ctx |> eq(index, ">")
        paren_counter.angle--
    if ctx |> eq(index, ">>")
        paren_counter.angle -= 2
    if ctx |> eq(index, ">>>")
        paren_counter.angle -= 3
    if ctx |> eq(index, "(")
        paren_counter.paren++
    if ctx |> eq(index, ")")
        paren_counter.paren--
    if ctx |> eq(index, "[")
        paren_counter.square++
    if ctx |> eq(index, "?[")
        paren_counter.square++
    if ctx |> eq(index, "]")
        paren_counter.square--
    if ctx |> eq(index, "\{")
        paren_counter.curly++
    if ctx |> eq(index, "\}")
        paren_counter.curly--


def find_pair_paren(var ctx: FormatterCtx; from_index: int): int
    var pc: ParenCounter
    for j in range(from_index + 1, length(ctx.tokens))
        ctx |> update_paren(j, pc)
        if pc.paren < 0
            return j
    return from_index


def find_pair_square(var ctx: FormatterCtx; from_index: int): int
    var pc: ParenCounter
    for j in range(from_index + 1, length(ctx.tokens))
        ctx |> update_paren(j, pc)
        if pc.square < 0
            return j
    return from_index


def mark_tokens_as_type(var ctx: FormatterCtx; i: int)
    var pc: ParenCounter
    for j in range(i, length(ctx.tokens))
        ctx |> update_paren(j, pc)
        if pc.angle <= 0 && pc.paren <= 0
            if (ctx |> new_line_before(j) || pc.paren < 0 || pc.square < 0 ||
                ctx |> eq(j, ":=") || ctx |> eq(j, "<-") || ctx |> eq(j, ";") || ctx |> eq(j, "="))
                break
        if (ctx.tokens[j].tokenType != TokenType COMMENT)
            ctx.tokens[j].isInType = true


def mark_tokens_as_type_inside_angle(var ctx: FormatterCtx; i: int)
    var pc: ParenCounter
    for j in range(i, length(ctx.tokens))
        ctx |> update_paren(j, pc)
        if (ctx.tokens[j].tokenType != TokenType COMMENT)
            ctx.tokens[j].isInType = true
        if pc.angle <= 0
            break


def search_token_in_line(var ctx: FormatterCtx; from_index: int; str: string const): int
    var pc: ParenCounter
    for j in range(from_index, length(ctx.tokens))
        ctx |> update_paren(j, pc)
        if (j > from_index && ctx |> new_line_before(j) &&
                (pc.paren <= 0 || pc.square <= 0 || pc.curly <= 0) && !eq(ctx, j - 1, ","))
            break
        if pc.paren < 0 || pc.square < 0 || pc.curly < 0
            break
        if ctx |> eq(j, ";") && pc.paren == 0 && pc.square == 0 && pc.curly == 0
            break
        if ctx |> eq(j, str)
            return j
    return 0


def process_formatter_tokens(var ctx: FormatterCtx)
    let formatterTokenPrefix = "//fmt:"
    let formatterTokenPrefixSize = formatterTokenPrefix |> length()
    for i in range(0, length(ctx.tokens))
        let token = ctx.tokens[i]
        let str = token.str
        let strSize = str |> length()
        if str |> starts_with(formatterTokenPrefix)
            let command = str |> chop(formatterTokenPrefixSize, strSize - formatterTokenPrefixSize)
            ctx.formatterTokens |> push([[ FormatterToken
                tokenIndex = i,
                command = command
                ]])

def have_formatter_token(ctx: FormatterCtx; command: string)
    for formatterToken in ctx.formatterTokens
        if formatterToken.command == command
            return true
    return false


def mark_token_context(var ctx: FormatterCtx)
    // don't format  require / expect / label
    for i in range(0, length(ctx.tokens) - 1)
        if ctx |> new_line_before(i) && (ctx |> eq(i, "require") || ctx |> eq(i, "expect") || ctx |> eq(i, "label") || ctx |> eq(i, "include"))
            for j in range(i + 1, length(ctx.tokens))
                if ctx |> new_line_before(j)
                    break
                ctx.tokens[j].dontFormat = true

    // reader macro
    for i in range(0, length(ctx.tokens) - 4)
        if ctx |> eq(i, "%") && ctx.tokens[i + 1].tokenType == TokenType KEYWORD_OR_IDENTIFIER && ctx |> eq(i + 2, "~")
            for j in range(i, length(ctx.tokens))
                ctx.tokens[j].dontFormat = true
                if ctx |> eq(j, "%%")
                    break

    // var / const declaration
    for i in range(0, length(ctx.tokens) - 3)
        if ctx |> new_line_before(i) && (ctx |> eq(i, "let") || ctx |> eq(i, "var"))
            let assignPos0 = ctx |> search_token_in_line(i, "=")
            let assignPos1 = ctx |> search_token_in_line(i, ":=")
            let assignPos2 = ctx |> search_token_in_line(i, "<-")
            let colonPos = ctx |> search_token_in_line(i, ":")
            if (colonPos > 0 &&
                    (assignPos0 > colonPos || assignPos0 <= 0) &&
                    (assignPos1 > colonPos || assignPos1 <= 0) &&
                    (assignPos2 > colonPos || assignPos2 <= 0))
                ctx |> mark_tokens_as_type(colonPos)

    // type in angle brackets
    for i in range(0, length(ctx.tokens) - 3)
        if ctx.tokens[i].tokenType == TokenType KEYWORD_OR_IDENTIFIER
            if (type_after_keyword |> find_index(ctx.tokens[i].str) >= 0)
                ctx |> mark_tokens_as_type_inside_angle(i + 1)

    // array type
    for i in range(0, length(ctx.tokens) - 3)
        if ctx |> eq(i, "[") && ctx |> eq(i + 1, "[")
            ctx |> mark_tokens_as_type(i + 2)

    // struct / class / variant or multiline variables
    for i in range(0, length(ctx.tokens) - 3)
        if ctx |> new_line_before(i) && (ctx |> eq(i, "struct") || ctx |> eq(i, "class") || ctx |> eq(i, "variant") ||
                ((ctx |> eq(i, "typedef") || ctx |> eq(i, "var") || ctx |> eq(i, "let")) && ctx |> new_line_before(i + 1)))
            let ident = ctx.tokens[i].spaces
            for j in range(i + 1, length(ctx.tokens) - 3)
                if ctx |> new_line_before(j)
                    if ctx |> eq(j, "def")
                        break
                    if (ctx.tokens[j].spaces <= ident)
                        break
                    var colonPos = ctx |> search_token_in_line(j, ctx |> eq(i, "typedef") ? "=" : ":")
                    if colonPos > 0
                        ctx |> mark_tokens_as_type(colonPos + 1)

    // oneline typedefs
    for i in range(0, length(ctx.tokens) - 3)
        if ctx |> new_line_before(i) && (ctx |> eq(i, "typedef") && !ctx |> new_line_before(i + 1))
            let ident = ctx.tokens[i].spaces
            for j in range(i + 1, length(ctx.tokens) - 3)
                if ctx |> new_line_before(j)
                    break
                var colonPos = ctx |> search_token_in_line(j, ctx |> eq(i, "typedef") ? "=" : ":")
                if colonPos > 0
                    ctx |> mark_tokens_as_type(colonPos + 1)

    // functions
    for i in range(0, length(ctx.tokens) - 3)
        if ctx |> new_line_before(i) && ctx |> eq(i, "def") && ctx |> eq(i + 2, ":")
            ctx |> mark_tokens_as_type(i + 2)
        if (ctx |> new_line_before(i) && ctx |> eq(i, "def")) || ((ctx |> eq(i, "$") || ctx |> eq(i, "@")) && (ctx |> eq(i + 1, "(") || ctx |> eq(i + 1, "[")))
            var beginParams: int = i
            if ctx |> eq(i + 1, "[")
                beginParams = ctx |> find_pair_square(i + 1) + 1
            beginParams = ctx |> search_token_in_line(beginParams, "(")

            if beginParams > 0
                var endParams = ctx |> find_pair_paren(beginParams)
                for j in range(beginParams, endParams + 1)
                    ctx.tokens[j].isInFunctionParam = true
                    if j - 1 == beginParams || ctx |> eq(j - 1, ";")
                        var colonPos = ctx |> search_token_in_line(j, ":")
                        if colonPos > 0
                            ctx |> mark_tokens_as_type(colonPos)
                if endParams < length(ctx.tokens) - 1
                    if ctx |> eq(endParams + 1, ":")
                        ctx |> mark_tokens_as_type(endParams + 1)

    // annotations
    for i in range(0, length(ctx.tokens) - 1)
        if ctx |> eq(i, "[") && !eq(ctx, i + 1, "[") && !eq(ctx, i + 1, "\{")
            for j in range(i + 1, length(ctx.tokens))
                if ctx |> eq(j, "]")
                    break
                if ctx |> eq(j, "=")
                    ctx.tokens[j].dontAddSpacesAround = true

    let macroEscapes <- {{ /*ident*/"i"; /*value*/"v"; /*expr*/"e"; /*call*/"c"; /*type*/"t"; /*field*/"f"; /*block*/"b"; /*array*/"a" }}
    for i in range(1, length(ctx.tokens))
        if !new_line_before(ctx, i) && ctx |> eq(i - 1, "$") && key_exists(macroEscapes, ctx.tokens[i].str)
            ctx.tokens[i - 1].dontAddSpacesAround = true


def fmt_function_arguments(var ctx: FormatterCtx)
    // function arguments
    var lastNewLineToken = 0
    var i = 0
    let n = length(ctx.tokens) - 3
    while i < n
        if ctx.tokens[i].newLines > 0
            lastNewLineToken = i

        if ctx.tokens[i].isInFunctionParam
            var firstBracketColumn = -1
            while i < n && ctx.tokens[i].isInFunctionParam
                if ctx.tokens[i].newLines > 0
                    lastNewLineToken = i

                if firstBracketColumn == -1
                    if ctx |> eq(i, "(")
                        firstBracketColumn = 0
                        for k in range(lastNewLineToken, i + 1)
                            if ctx.tokens[k].newLines > 0
                                firstBracketColumn = 0
                            firstBracketColumn += ctx.tokens[k].spaces + length(ctx.tokens[k].str)

                elif ctx.tokens[i].newLines > 0
                    lastNewLineToken = i
                    ctx.tokens[i].spaces = firstBracketColumn + (ctx.tokens[i].isInType ? 2 : 0)
                    ctx.tokens[i].column = firstBracketColumn + (ctx.tokens[i].isInType ? 2 : 0)

                i++
        i++


def debug_print_tokens(ctx: FormatterCtx)
    for t in ctx.tokens
        to_log(LOG_DEBUG, "{t.isInFunctionParam ? ">" : " "}{t.isInType ? "*" : " "} '{t.str}'\n");


def generate_source(ctx: FormatterCtx): string
    var res: array<uint8>
    res |> reserve(1000 + length(ctx.tokens) * 8)
    if ctx.haveUtf8Bom
        res |> push(uint8(0xEF))
        res |> push(uint8(0xBB))
        res |> push(uint8(0xBF))

    let useCrLf: bool = ctx.crCount * 2 > ctx.lfCount
    var column: int = 0

    for t in ctx.tokens
        for i in range(t.newLines)
            if (useCrLf)
                res |> push(uint8('\r'))
            res |> push(uint8('\n'))
            column = 0

        var spaces = t.spaces
        if t.tokenType == TokenType COMMENT && t.str |> starts_with("//")
            spaces = max(t.column - column, 0)

        for i in range(spaces)
            res |> push(uint8(' '))
            column++

        for c in t.str
            res |> push(uint8(c))
            column++

    return string(res)
finally
    delete res


def need_spaces_around(ctx: FormatterCtx; index: int): bool
    assume t = ctx.tokens[index]
    if t.tokenType != TokenType UNKNOWN || t.dontFormat || t.dontAddSpacesAround
        return false
    if dont_need_space_around |> find_index(t.str) >= 0
        return false
    if t.isInType && (t.str == ":" || t.str == "<" || t.str == ">" || t.str == ">>" || t.str == ">>>" || empty(t.str))
        return false
    return true


def need_space_only_before(ctx: FormatterCtx; index: int): bool
    assume t = ctx.tokens[index]
    if !t.isInType
        return false
    if t.tokenType != TokenType UNKNOWN || t.dontFormat || t.dontAddSpacesAround
        return false
    if dont_need_space_around |> find_index(t.str) >= 0
        return false
    if t.str == ">" || t.str == ">>" || t.str == ">>>" || t.str == "<" || empty(t.str)
        return false
    if index < length(ctx.tokens) - 1
        assume next = ctx.tokens[index + 1]
        if next.str == ">" || next.str == ">>" || next.str == ">>>" || next.str == ";"
            return true
    return false


def add_space_before(var ctx: FormatterCtx; i)
    if (ctx.tokens[i].dontFormat || ctx.tokens[i - 1].dontFormat) || ctx |> new_line_before(i)
        return
    ctx.tokens[i].spaces = 1


def remove_space_before(var ctx: FormatterCtx; i)
    if (ctx.tokens[i].dontFormat || ctx.tokens[i - 1].dontFormat) || ctx |> new_line_before(i)
        return
    ctx.tokens[i].spaces = 0


let tokens_before_unary <- [[auto "("; "["; "?["; "\{"; "auto"; "int"; "float"; "int16"; "int32"; "int8"; ";"; ",";
    "if"; "elif"; "while"; "switch"; "return"; "yield"; "in"; "="; "!"
    ]]

def is_unary(ctx: FormatterCtx; index: int): bool
    if ctx |> eq(index, "!") || ctx |> eq(index, "~")
        return true
    if ctx |> eq(index, "+") || ctx |> eq(index, "-") || ctx |> eq(index, "*")
        if (tokens_before_unary |> find_index(ctx.tokens[index - 1].str) >= 0) || ctx |> new_line_before(index)
            return true
        return ctx |> need_spaces_around(index - 1)
    return false


def remove_spaces_around(var ctx: FormatterCtx; index: int)
    if !ctx |> new_line_before(index)
        ctx |> remove_space_before(index)
    if !ctx |> new_line_before(index + 1) && ctx.tokens[index + 1].tokenType != TokenType COMMENT
        ctx |> remove_space_before(index + 1)


def make_spaces_around(var ctx: FormatterCtx; index: int)
    if !new_line_before(ctx, index) && ctx.tokens[index].spaces == 0
        ctx |> add_space_before(index)
    if !new_line_before(ctx, index + 1) && ctx.tokens[index + 1].spaces == 0
        ctx |> add_space_before(index + 1)


def fmt_space_after_comma(var ctx: FormatterCtx)
    for i in range(1, length(ctx.tokens))
        if !new_line_before(ctx, i) && ctx |> eq(i - 1, ",") && ctx.tokens[i].spaces == 0
            ctx |> add_space_before(i)


def fmt_space_after_semicolon(var ctx: FormatterCtx)
    for i in range(1, length(ctx.tokens))
        if !ctx |> eq(i, "") && !new_line_before(ctx, i) && ctx |> eq(i - 1, ";") && ctx.tokens[i].spaces == 0
            ctx |> add_space_before(i)


def fmt_remove_space_before_comma(var ctx: FormatterCtx)
    for i in range(1, length(ctx.tokens))
        if !new_line_before(ctx, i) && ctx |> eq(i, ",")
            ctx |> remove_space_before(i)


def fmt_remove_space_before_semicolon(var ctx: FormatterCtx)
    for i in range(1, length(ctx.tokens))
        if !new_line_before(ctx, i) && ctx |> eq(i, ";")
            ctx |> remove_space_before(i)


def fmt_type_colon(var ctx: FormatterCtx)
    for i in range(1, length(ctx.tokens) - 1)
        if ctx.tokens[i].isInType && ctx |> eq(i, ":")
            ctx |> add_space_before(i)
            if !new_line_before(ctx, i + 1)
                ctx |> add_space_before(i + 1)


def fmt_function_decl_param_paren(var ctx: FormatterCtx)
    for i in range(1, length(ctx.tokens) - 1)
        if ctx.tokens[i].isInFunctionParam && !ctx.tokens[i - 1].isInFunctionParam && ctx |> eq(i, "(")
            ctx |> remove_spaces_around(i)
            if ctx |> eq(i - 1, "]")
                ctx |> add_space_before(i)
        if ctx.tokens[i].isInFunctionParam && !ctx.tokens[i + 1].isInFunctionParam && ctx |> eq(i, ")")
            ctx |> remove_space_before(i)


def fmt_function_call_param_paren_call(var ctx: FormatterCtx)
    for i in range(1, length(ctx.tokens) - 1)
        if ctx |> eq(i, "(") && !new_line_before(ctx, i) && ctx.tokens[i - 1].tokenType == TokenType KEYWORD_OR_IDENTIFIER
            ctx |> remove_spaces_around(i)
            var closingParen = ctx |> find_pair_paren(i)
            ctx |> remove_space_before(closingParen)


def fmt_spaces_around_operators(var ctx: FormatterCtx)
    for i in range(1, length(ctx.tokens) - 1)
        if !empty(ctx.tokens[i].str)
            if ctx |> need_space_only_before(i) && !is_unary(ctx, i)
                ctx |> add_space_before(i)
            elif ctx |> need_spaces_around(i) && !is_unary(ctx, i)
                ctx |> make_spaces_around(i)


let space_after_keywords <- [[auto "if"; "elif"; "while"; "switch"; "with";
        "yield"; "catch"; "static_if"; "static_elif"; "for"
    ]]

def fmt_space_after_keyword(var ctx: FormatterCtx)
    for i in range(1, length(ctx.tokens) - 1)
        if space_after_keywords |> find_index(ctx.tokens[i].str) >= 0
            if !new_line_before(ctx, i + 1)
                ctx |> add_space_before(i + 1)
        // add space only after `return <expr>` expression
        if ctx.tokens[i].str == "return" && length(ctx.tokens[i+1].str) > 0
            if !new_line_before(ctx, i + 1)
                ctx |> add_space_before(i + 1)


def fmt_space_after_paren(var ctx: FormatterCtx)
    for i in range(1, length(ctx.tokens) - 1)
        if ctx |> eq(i, ")") && (ctx.tokens[i + 1].tokenType != TokenType UNKNOWN)
            ctx |> add_space_before(i + 1)


def fmt_space_after_square_brackets(var ctx: FormatterCtx)
    for i in range(1, length(ctx.tokens) - 1)
        if ctx |> eq(i, "]") && (ctx.tokens[i + 1].tokenType != TokenType UNKNOWN)
            ctx |> add_space_before(i + 1)


def fmt_remove_space_inside_parens(var ctx: FormatterCtx)
    for i in range(1, length(ctx.tokens) - 1)
        if ctx |> eq(i, "(")
            ctx |> remove_space_before(i + 1)
        if ctx |> eq(i, ")")
            ctx |> remove_space_before(i)


def fmt_remove_space_before_as(var ctx: FormatterCtx)
    for i in range(1, length(ctx.tokens))
        if !new_line_before(ctx, i) && ctx |> eq(i, "as") && ctx |> eq(i - 1, "?")
            ctx |> remove_space_before(i)


let type_specifiers = [[string "-"; "?"; "&"; "#"]]

def fmt_glue_type_specifiers(var ctx: FormatterCtx)
    for i in range(1, length(ctx.tokens))
        if (ctx.tokens[i].isInType && ctx.tokens[i].tokenType == TokenType UNKNOWN &&
                type_specifiers |> find_index(ctx.tokens[i].str) >= 0)
            ctx |> remove_space_before(i)


def fmt_space_after_cast_type(var ctx: FormatterCtx)
    for i in range(0, length(ctx.tokens) - 3)
        if ctx.tokens[i].tokenType == TokenType KEYWORD_OR_IDENTIFIER
            if (type_after_keyword |> find_index(ctx.tokens[i].str) >= 0)
                for j in range(i + 2, length(ctx.tokens))
                    if !ctx.tokens[j].isInType
                        ctx |> add_space_before(j)
                        break


def do_format(var ctx: FormatterCtx)
    ctx |> parse_all_tokens()
    ctx |> process_formatter_tokens()

    if ctx |> have_formatter_token("ignore-file")
        return

    ctx |> mark_token_context()
    //ctx |> debug_print_tokens()
    ctx |> fmt_spaces_around_operators()
    ctx |> fmt_function_call_param_paren_call()
    ctx |> fmt_space_after_comma()
    ctx |> fmt_space_after_semicolon()
    ctx |> fmt_function_decl_param_paren()
    ctx |> fmt_type_colon()
    ctx |> fmt_space_after_cast_type()
    ctx |> fmt_remove_space_before_comma()
    ctx |> fmt_remove_space_before_semicolon()
    ctx |> fmt_remove_space_inside_parens()
    ctx |> fmt_remove_space_before_as()
    ctx |> fmt_glue_type_specifiers()
    ctx |> fmt_space_after_keyword()
    ctx |> fmt_space_after_paren()
    ctx |> fmt_space_after_square_brackets()
    ctx |> fmt_function_arguments()


def public format_source_string(file_data: string const& implicit): string
    var inscope ctx: FormatterCtx
    ctx.data <- [{ for c in file_data; uint8(c) }]
    ctx.data |> push(uint8(0))
    ctx |> do_format()
    return <- generate_source(ctx)


def format_source(var ctx: FormatterCtx; file_data: array<uint8> const& implicit): string
    ctx.data := file_data
    ctx.data |> push(uint8(0))
    ctx |> do_format()
    return <- generate_source(ctx)


def public format_source(file_data: array<uint8> const& implicit): string
    var inscope ctx: FormatterCtx
    return ctx |> format_source(file_data)


def public format_file(var ctx: FormatterCtx; file_name: string const&)
    var unformatted: string = ""
    var formatted: string = ""
    fopen(file_name, "rb") <| $(fr)
        if fr == null
            to_log(LOG_ERROR, "cannot open '{file_name}'\n")
        else
            fmap(fr) <| $(data)
                unformatted = string(data)
                formatted = ctx |> format_source(data)

    if ctx.debugMode
        to_log(LOG_DEBUG, formatted)

    if !empty(formatted) && formatted != unformatted && !ctx.debugMode
        fopen(file_name, "wb") <| $(fw)
            if fw == null
                to_log(LOG_ERROR, "cannot open '{file_name} for write'\n")
            else
                fw |> fprint(formatted)


def public format_file(file_name: string const&)
    var inscope ctx: FormatterCtx
    ctx |> format_file(file_name)


def public format_files(var ctx: FormatterCtx; file_names: array<string>&)
    for fname in file_names
        ctx |> format_file(fname)


def public format_files(file_names: array<string>&)
    var inscope ctx: FormatterCtx
    ctx |> format_files(file_names)

/*
[export]
def main()
    var fileNames: array<string>
    var filesListFn: string = ""
    var ignore = true
    for arg in get_command_line_arguments()
        if arg == "--debug"
            debugMode = true
        if arg == "-args"
            ignore = false
            continue
        if !ignore
            if arg |> character_at(0) != '-'
                fileNames |> push(arg)
            if arg |> starts_with("--files:")
                filesListFn = arg |> slice(8)

    if !empty(filesListFn)
        fopen(filesListFn, "rb") <| $(f)
            if f == null
                to_log(LOG_ERROR, "cannot open '{filesListFn}'\n")
            else
                while !feof(f)
                    var file_name = fgets(f)
                    if !empty(file_name)
                        file_name = replace(file_name, "\n", "")
                        file_name = replace(file_name, "\r", "")
                        fileNames |> push(file_name)

    format_files(fileNames)
*/
