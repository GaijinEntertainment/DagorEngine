_TYPENAME_TO_MASK = {
    'bool': 'b',
    'number': 'n',
    'int': 'i',
    'float': 'f',
    'string': 's',
    'table': 't',
    'array': 'a',
    'userdata': 'u',
    'function': 'c',
    'generator': 'g',
    'userpointer': 'p',
    'thread': 'v',
    'instance': 'x',
    'class': 'y',
    'weakref': 'w',
    'null': 'o',
    'any': '.',
}


def type_to_mask(type_string):
    return '|'.join(
        _TYPENAME_TO_MASK.get(type_name.strip().lower(), '.')
        for type_name in type_string.replace(' ', '').split('|')
    )


class _FunctionDeclParser:
    def __init__(self, declaration):
        self.declaration = declaration
        self.pos = 0
        self.is_pure = False
        self.is_fastcall = False
        self.is_nodiscard = False

    def fail(self, message):
        raise ValueError(
            f"{message} at character {self.pos + 1} in docstring: "
            f"{self.declaration}"
        )

    def skip_spaces(self):
        while self.pos < len(self.declaration) and self.declaration[self.pos].isspace():
            self.pos += 1

    def peek(self):
        if self.pos == len(self.declaration):
            return ''
        return self.declaration[self.pos]

    @staticmethod
    def is_identifier_start(char):
        return char == '_' or 'A' <= char <= 'Z' or 'a' <= char <= 'z'

    @classmethod
    def is_identifier_char(cls, char):
        return cls.is_identifier_start(char) or '0' <= char <= '9'

    def parse_identifier(self, what):
        start = self.pos
        if not self.is_identifier_start(self.peek()):
            self.fail(f"Expected {what}")

        self.pos += 1
        while self.is_identifier_char(self.peek()):
            self.pos += 1
        return self.declaration[start:self.pos]

    def parse_type_mask(self):
        self.skip_spaces()
        has_brackets = self.peek() == '('
        if has_brackets:
            self.pos += 1
            self.skip_spaces()

        type_names = []
        while True:
            type_name = self.parse_identifier('type name')
            if type_name not in _TYPENAME_TO_MASK:
                self.fail(f"Invalid type name '{type_name}'")
            type_names.append(type_name)

            self.skip_spaces()
            if self.peek() != '|':
                break
            self.pos += 1
            self.skip_spaces()

        if has_brackets:
            if self.peek() != ')':
                self.fail("Expected ')' after type list")
            self.pos += 1
            self.skip_spaces()

        type_string = '|'.join(type_names)
        return type_string, type_to_mask(type_string)

    def parse_modifiers(self):
        while True:
            start = self.pos
            if not self.is_identifier_start(self.peek()):
                return
            modifier = self.parse_identifier('function name')
            if modifier not in ('pure', 'nodiscard', 'fastcall') or not self.peek().isspace():
                self.pos = start
                return

            self.skip_spaces()
            if modifier == 'pure':
                self.is_pure = True
            elif modifier == 'nodiscard':
                self.is_nodiscard = True
            else:
                self.is_fastcall = True

    def parse_function_name(self):
        if self.peek() == '(':
            self.parse_type_mask()
            if self.peek() != '.':
                self.fail("Expected '.' after object type")
            self.pos += 1
            self.skip_spaces()
            return self.parse_identifier('function name')

        name_or_type = self.parse_identifier('function name')
        if self.peek() != '.':
            return name_or_type

        self.pos += 1
        self.skip_spaces()
        function_name = self.parse_identifier("function name after '.'")
        if name_or_type not in _TYPENAME_TO_MASK:
            self.fail(f"Invalid object type '{name_or_type}'")
        return function_name

    def parse_default_value(self):
        self.pos += 1
        self.skip_spaces()
        start = self.pos
        brackets = []
        string_opener = None

        matching_bracket = {')': '(', ']': '[', '}': '{'}
        while self.pos < len(self.declaration):
            char = self.peek()
            if string_opener:
                if char == '\\':
                    self.pos += 1
                    if self.pos == len(self.declaration):
                        self.fail('Unterminated string in default value')
                elif char == string_opener:
                    string_opener = None
                self.pos += 1
                continue

            if char in ('"', "'"):
                string_opener = char
            elif char in '([{':
                brackets.append(char)
            elif char in ')]}':
                if not brackets:
                    if char in ')]':
                        break
                    self.fail("Unmatched '}' in default value")
                if brackets[-1] != matching_bracket[char]:
                    self.fail(f"Unmatched '{char}' in default value")
                brackets.pop()
            elif char == ',' and not brackets:
                break
            self.pos += 1

        if string_opener:
            self.fail('Unterminated string in default value')
        if brackets:
            self.fail('Unfinished default value, unmatched brackets')
        if self.pos == start:
            self.fail("Expected default value after '='")
        if self.pos == len(self.declaration):
            self.fail('Unterminated function type string')

        return self.declaration[start:self.pos].strip()

    def parse_arguments(self):
        if self.peek() != '(':
            self.fail("Expected '(' after function name")
        self.pos += 1
        self.skip_spaces()

        args = []
        typemask_parts = []
        required_args = 0
        has_optional_args = False
        vargved = False
        vargtype = None
        inside_optional = False
        optional_block_finished = False
        argument_processed = False

        while True:
            self.skip_spaces()
            if self.peek() == ')':
                break
            if not self.peek():
                self.fail('Unterminated argument list')

            if self.peek() == '[':
                if inside_optional:
                    self.fail('Nested optional blocks are not allowed')
                if optional_block_finished:
                    self.fail('Optional block must be the last argument')
                inside_optional = True
                self.pos += 1
                self.skip_spaces()
                continue

            if self.peek() == ']':
                if not inside_optional:
                    self.fail("Unmatched ']'")
                inside_optional = False
                optional_block_finished = True
                self.pos += 1
                self.skip_spaces()
                continue

            if self.peek() == ',':
                if not argument_processed:
                    self.fail("Argument expected before ','")
                self.pos += 1
                self.skip_spaces()
                argument_processed = False
                continue

            if self.declaration.startswith('...', self.pos):
                if vargved:
                    self.fail('Multiple ellipsis arguments')
                argument_processed = True
                vargved = True
                self.pos += 3
                self.skip_spaces()
                if self.peek() == ':':
                    self.pos += 1
                    self.skip_spaces()
                    vargtype, _ = self.parse_type_mask()
                self.skip_spaces()
                if self.peek() not in (')', ']'):
                    self.fail("Expected ')' after ellipsis argument")
                continue

            if optional_block_finished:
                self.fail('Argument after optional block')

            name = self.parse_identifier('argument name')
            argument_processed = True
            type_string = '.'
            type_mask = '.'
            self.skip_spaces()
            if self.peek() == ':':
                self.pos += 1
                self.skip_spaces()
                type_string, type_mask = self.parse_type_mask()
            elif self.peek() not in (',', ')', ']', '[', '='):
                self.fail("Expected ':' after argument name")

            has_default_value = self.peek() == '='
            default_value = None
            if has_default_value:
                default_value = self.parse_default_value()
            elif not inside_optional and args and 'defvalue' in args[-1]:
                self.fail('Default value expected after optional argument')

            argument = {
                'name': name,
                'paramtype': type_string,
                'optional': inside_optional or has_default_value,
            }
            if has_default_value:
                argument['defvalue'] = default_value
                argument['defvalue_source'] = default_value
            args.append(argument)
            typemask_parts.append(type_mask)
            has_optional_args |= argument['optional']
            if not inside_optional and not has_default_value:
                required_args += 1

            self.skip_spaces()
            if self.peek() == ',':
                self.pos += 1
                self.skip_spaces()
                argument_processed = False
            elif self.peek() not in (')', ']', '['):
                self.fail("Expected ',' or ')'")

        if inside_optional:
            self.fail("Unmatched '['")

        self.pos += 1
        paramsnum = -required_args if has_optional_args else required_args
        return args, ''.join(typemask_parts), paramsnum, vargved, vargtype

    def parse(self):
        self.skip_spaces()
        if not self.peek():
            self.fail('Empty function type string')

        self.parse_modifiers()
        name = self.parse_function_name()
        self.skip_spaces()
        args, typemask, paramsnum, vargved, vargtype = self.parse_arguments()
        self.skip_spaces()

        rtype = 'null'
        if self.peek() == ':':
            self.pos += 1
            self.skip_spaces()
            rtype, _ = self.parse_type_mask()

        self.skip_spaces()
        if self.peek():
            self.fail('Unexpected characters after function type string')

        return {
            'name': name,
            'rtype': rtype,
            'args': args,
            'paramsnum': paramsnum,
            'typemask': typemask,
            'is_pure': self.is_pure,
            'is_fastcall': self.is_fastcall,
            'is_nodiscard': self.is_nodiscard,
            'vargved': vargved,
            'vargtype': vargtype,
        }


def parse_docstring(docstring, comments=None):
    """
    Returns: dict {name, rtype, args, paramsnum, typemask, is_pure, is_fastcall}
    """
    result = _FunctionDeclParser(docstring.strip()).parse()
    result['members'] = comments.splitlines() if comments else None
    return result


if __name__ == "__main__":
    declarations = [
        "pure Color(r: number, g: number, b: number, [alpha: number]): int",
        "pure Color(r: number, g: number, b: number, [alpha: number = 255]): int",
        "pure Color(r: number, g: number, b: number, alpha: number = 255): int",
        "pure sw(percent: number): number",
        "fastcall pure sh(percent: number): number",
        "pure fastcall pw(percent: number): userdata",
        "pure nodiscard ph(percent: number): userdata",
        "nodiscard pure elemw(percent: number): userdata",
        "pure nodiscard fastcall elemh(percent: number): userdata",
        "pure get_font_metrics(fontId: int, [fontHt: number]): table",
    ]
    for declaration in declarations:
        print(parse_docstring(declaration))
