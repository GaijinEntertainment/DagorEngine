#include "TextEditor.h"

static bool TokenizeCStyleString(const char* in_begin, const char* in_end, const char*& out_begin, const char*& out_end)
{
	const char* p = in_begin;

	if (*p == '"')
	{
		p++;

		while (p < in_end)
		{
			// handle end of string
			if (*p == '"')
			{
				out_begin = in_begin;
				out_end = p + 1;
				return true;
			}

			// handle escape character for "
			if (*p == '\\' && p + 1 < in_end && p[1] == '"')
				p++;

			p++;
		}
	}

	return false;
}

static bool TokenizeCStyleCharacterLiteral(const char* in_begin, const char* in_end, const char*& out_begin, const char*& out_end)
{
	const char* p = in_begin;

	if (*p == '\'')
	{
		p++;

		// handle escape characters
		if (p < in_end && *p == '\\')
			p++;

		if (p < in_end)
			p++;

		// handle end of character literal
		if (p < in_end && *p == '\'')
		{
			out_begin = in_begin;
			out_end = p + 1;
			return true;
		}
	}

	return false;
}

static bool TokenizeCStyleIdentifier(const char* in_begin, const char* in_end, const char*& out_begin, const char*& out_end)
{
	const char* p = in_begin;

	if ((*p >= 'a' && *p <= 'z') || (*p >= 'A' && *p <= 'Z') || *p == '_')
	{
		p++;

		while ((p < in_end) && ((*p >= 'a' && *p <= 'z') || (*p >= 'A' && *p <= 'Z') || (*p >= '0' && *p <= '9') || *p == '_'))
			p++;

		out_begin = in_begin;
		out_end = p;
		return true;
	}

	return false;
}

static bool TokenizeCStyleNumber(const char* in_begin, const char* in_end, const char*& out_begin, const char*& out_end)
{
	const char* p = in_begin;

	const bool startsWithNumber = *p >= '0' && *p <= '9';

	if (*p != '+' && *p != '-' && !startsWithNumber)
		return false;

	p++;

	bool hasNumber = startsWithNumber;

	while (p < in_end && (*p >= '0' && *p <= '9'))
	{
		hasNumber = true;

		p++;
	}

	if (hasNumber == false)
		return false;

	bool isFloat = false;
	bool isHex = false;
	bool isBinary = false;

	if (p < in_end)
	{
		if (*p == '.')
		{
			isFloat = true;

			p++;

			while (p < in_end && (*p >= '0' && *p <= '9'))
				p++;
		}
		else if (*p == 'x' || *p == 'X')
		{
			// hex formatted integer of the type 0xef80

			isHex = true;

			p++;

			while (p < in_end && ((*p >= '0' && *p <= '9') || (*p >= 'a' && *p <= 'f') || (*p >= 'A' && *p <= 'F')))
				p++;
		}
		else if (*p == 'b' || *p == 'B')
		{
			// binary formatted integer of the type 0b01011101

			isBinary = true;

			p++;

			while (p < in_end && (*p >= '0' && *p <= '1'))
				p++;
		}
	}

	if (isHex == false && isBinary == false)
	{
		// floating point exponent
		if (p < in_end && (*p == 'e' || *p == 'E'))
		{
			isFloat = true;

			p++;

			if (p < in_end && (*p == '+' || *p == '-'))
				p++;

			bool hasDigits = false;

			while (p < in_end && (*p >= '0' && *p <= '9'))
			{
				hasDigits = true;

				p++;
			}

			if (hasDigits == false)
				return false;
		}

		// single precision floating point type
		if (p < in_end && *p == 'f')
			p++;
	}

	if (isFloat == false)
	{
		// integer size type
		while (p < in_end && (*p == 'u' || *p == 'U' || *p == 'l' || *p == 'L'))
			p++;
	}

	out_begin = in_begin;
	out_end = p;
	return true;
}

static bool TokenizeCStylePunctuation(const char* in_begin, const char* in_end, const char*& out_begin, const char*& out_end)
{
	(void)in_end;

	switch (*in_begin)
	{
	case '[':
	case ']':
	case '{':
	case '}':
	case '!':
	case '%':
	case '^':
	case '&':
	case '*':
	case '(':
	case ')':
	case '-':
	case '+':
	case '=':
	case '~':
	case '|':
	case '<':
	case '>':
	case '?':
	case ':':
	case '/':
	case ';':
	case ',':
	case '.':
		out_begin = in_begin;
		out_end = in_begin + 1;
		return true;
	}

	return false;
}

static bool TokenizeLuaStyleString(const char* in_begin, const char* in_end, const char*& out_begin, const char*& out_end)
{
	const char* p = in_begin;

	bool is_single_quote = false;
	bool is_double_quotes = false;
	bool is_double_square_brackets = false;

	switch (*p)
	{
	case '\'':
		is_single_quote = true;
		break;
	case '"':
		is_double_quotes = true;
		break;
	case '[':
		p++;
		if (p < in_end && *(p) == '[')
			is_double_square_brackets = true;
		break;
	}

	if (is_single_quote || is_double_quotes || is_double_square_brackets)
	{
		p++;

		while (p < in_end)
		{
			// handle end of string
			if ((is_single_quote && *p == '\'') || (is_double_quotes && *p == '"') || (is_double_square_brackets && *p == ']' && p + 1 < in_end && *(p + 1) == ']'))
			{
				out_begin = in_begin;

				if (is_double_square_brackets)
					out_end = p + 2;
				else
					out_end = p + 1;

				return true;
			}

			// handle escape character for "
			if (*p == '\\' && p + 1 < in_end && (is_single_quote || is_double_quotes))
				p++;

			p++;
		}
	}

	return false;
}

static bool TokenizeLuaStyleIdentifier(const char* in_begin, const char* in_end, const char*& out_begin, const char*& out_end)
{
	const char* p = in_begin;

	if ((*p >= 'a' && *p <= 'z') || (*p >= 'A' && *p <= 'Z') || *p == '_')
	{
		p++;

		while ((p < in_end) && ((*p >= 'a' && *p <= 'z') || (*p >= 'A' && *p <= 'Z') || (*p >= '0' && *p <= '9') || *p == '_'))
			p++;

		out_begin = in_begin;
		out_end = p;
		return true;
	}

	return false;
}

static bool TokenizeLuaStyleNumber(const char* in_begin, const char* in_end, const char*& out_begin, const char*& out_end)
{
	const char* p = in_begin;

	const bool startsWithNumber = *p >= '0' && *p <= '9';

	if (*p != '+' && *p != '-' && !startsWithNumber)
		return false;

	p++;

	bool hasNumber = startsWithNumber;

	while (p < in_end && (*p >= '0' && *p <= '9'))
	{
		hasNumber = true;

		p++;
	}

	if (hasNumber == false)
		return false;

	if (p < in_end)
	{
		if (*p == '.')
		{
			p++;

			while (p < in_end && (*p >= '0' && *p <= '9'))
				p++;
		}

		// floating point exponent
		if (p < in_end && (*p == 'e' || *p == 'E'))
		{
			p++;

			if (p < in_end && (*p == '+' || *p == '-'))
				p++;

			bool hasDigits = false;

			while (p < in_end && (*p >= '0' && *p <= '9'))
			{
				hasDigits = true;

				p++;
			}

			if (hasDigits == false)
				return false;
		}
	}

	out_begin = in_begin;
	out_end = p;
	return true;
}

static bool TokenizeLuaStylePunctuation(const char* in_begin, const char* in_end, const char*& out_begin, const char*& out_end)
{
	(void)in_end;

	switch (*in_begin)
	{
	case '[':
	case ']':
	case '{':
	case '}':
	case '!':
	case '%':
	case '#':
	case '^':
	case '&':
	case '*':
	case '(':
	case ')':
	case '-':
	case '+':
	case '=':
	case '~':
	case '|':
	case '<':
	case '>':
	case '?':
	case ':':
	case '/':
	case ';':
	case ',':
	case '.':
		out_begin = in_begin;
		out_end = in_begin + 1;
		return true;
	}

	return false;
}

const TextEditor::LanguageDefinition& TextEditor::LanguageDefinition::Cpp()
{
	static bool inited = false;
	static LanguageDefinition langDef;
	if (!inited)
	{
		static const char* const cppKeywords[] = {
			"alignas", "alignof", "and", "and_eq", "asm", "atomic_cancel", "atomic_commit", "atomic_noexcept", "auto", "bitand", "bitor", "bool", "break", "case", "catch", "char", "char16_t", "char32_t", "class",
			"compl", "concept", "const", "constexpr", "const_cast", "continue", "decltype", "default", "delete", "do", "double", "dynamic_cast", "else", "enum", "explicit", "export", "extern", "false", "float",
			"for", "friend", "goto", "if", "import", "inline", "int", "long", "module", "mutable", "namespace", "new", "noexcept", "not", "not_eq", "nullptr", "operator", "or", "or_eq", "private", "protected", "public",
			"register", "reinterpret_cast", "requires", "return", "short", "signed", "sizeof", "static", "static_assert", "static_cast", "struct", "switch", "synchronized", "template", "this", "thread_local",
			"throw", "true", "try", "typedef", "typeid", "typename", "union", "unsigned", "using", "virtual", "void", "volatile", "wchar_t", "while", "xor", "xor_eq"
		};
		for (auto& k : cppKeywords)
			langDef.mKeywords.insert(k);

		static const char* const identifiers[] = {
			"abort", "abs", "acos", "asin", "atan", "atexit", "atof", "atoi", "atol", "ceil", "clock", "cosh", "ctime", "div", "exit", "fabs", "floor", "fmod", "getchar", "getenv", "isalnum", "isalpha", "isdigit", "isgraph",
			"ispunct", "isspace", "isupper", "kbhit", "log10", "log2", "log", "memcmp", "modf", "pow", "printf", "sprintf", "snprintf", "putchar", "putenv", "puts", "rand", "remove", "rename", "sinh", "sqrt", "srand", "strcat", "strcmp", "strerror", "time", "tolower", "toupper",
			"std", "string", "vector", "map", "unordered_map", "set", "unordered_set", "min", "max"
		};
		for (auto& k : identifiers)
		{
			Identifier id;
			id.mDeclaration = "Built-in function";
			langDef.mIdentifiers.insert(eastl::make_pair(eastl::string(k), id));
		}

		langDef.mTokenize = [](const char* in_begin, const char* in_end, const char*& out_begin, const char*& out_end, PaletteIndex& paletteIndex) -> bool
		{
			paletteIndex = PaletteIndex::Max;

			while (in_begin < in_end && unsigned(*in_begin) < 128 && isblank(*in_begin))
				in_begin++;

			if (in_begin == in_end)
			{
				out_begin = in_end;
				out_end = in_end;
				paletteIndex = PaletteIndex::Default;
			}
			else if (TokenizeCStyleString(in_begin, in_end, out_begin, out_end))
				paletteIndex = PaletteIndex::String;
			else if (TokenizeCStyleCharacterLiteral(in_begin, in_end, out_begin, out_end))
				paletteIndex = PaletteIndex::CharLiteral;
			else if (TokenizeCStyleIdentifier(in_begin, in_end, out_begin, out_end))
				paletteIndex = PaletteIndex::Identifier;
			else if (TokenizeCStyleNumber(in_begin, in_end, out_begin, out_end))
				paletteIndex = PaletteIndex::Number;
			else if (TokenizeCStylePunctuation(in_begin, in_end, out_begin, out_end))
				paletteIndex = PaletteIndex::Punctuation;

			return paletteIndex != PaletteIndex::Max;
		};

		langDef.mCommentStart = "/*";
		langDef.mCommentEnd = "*/";
		langDef.mSingleLineComment = "//";
		langDef.mPreprocessorPrefix = "#";

		langDef.mCaseSensitive = true;

		langDef.mName = "C++";

		inited = true;
	}
	return langDef;
}

const TextEditor::LanguageDefinition& TextEditor::LanguageDefinition::Daslang()
{
	static bool inited = false;
	static LanguageDefinition langDef;
	if (!inited)
	{
		static const char* const keywords[] = {
			"abstract", "addr", "aka", "array", "as", "assume", "auto", "bitfield", "block", "bool", "break",
			"cast", "class", "const", "continue", "def", "delete", "deref", "double", "elif", "else", "enum",
			"expect", "explicit", "false", "finally", "float", "float2", "float3", "float4", "for", "function",
			"generator", "goto", "if", "implicit", "in", "inscope", "int", "int16", "int2", "int3", "int4",
			"int64", "int8", "is", "iterator", "label", "lambda", "let", "module", "new", "null", "operator",
			"options", "override", "pass", "private", "public", "range", "recover", "reinterpret", "require",
			"return", "sealed", "shared", "smart_ptr", "static", "static_elif", "static_if", "string", "struct",
			"table", "true", "try", "tuple", "type", "typedef", "typeinfo", "uint", "uint16", "uint2", "uint3",
			"uint4", "uint64", "uint8", "unsafe", "upcast", "urange", "var", "variant", "void", "where", "while",
			"with", "yield", "float3x3", "float3x4", "float4x4",
		};
		for (auto& k : keywords)
			langDef.mKeywords.insert(k);

		static const char* const identifiers[] = {
			"acos", "asin", "atan", "atan_est", "atan2", "atan2_est", "cos", "cross", "distance", "distance_sq",
			"dot", "exp", "exp2", "fast_normalize", "identity", "inv_distance", "inv_distance_sq", "inv_length",
			"inv_length_sq", "is_finite", "is_nan", "length", "length_sq", "lerp", "log", "log2", "max", "min",
			"normalize", "pow", "rcp", "reflect", "refract", "sin", "sincos", "sqrt", "tan", "resize", "character_at",
			"format", "each", "find", "find_index", "find_index_if",
		};
		for (auto& k : identifiers)
		{
			Identifier id;
			id.mDeclaration = "Built-in function";
			langDef.mIdentifiers.insert(eastl::make_pair(eastl::string(k), id));
		}

		langDef.mTokenize = [](const char* in_begin, const char* in_end, const char*& out_begin, const char*& out_end, PaletteIndex& paletteIndex) -> bool
		{
			paletteIndex = PaletteIndex::Max;

			while (in_begin < in_end && iswblank(*in_begin))
				in_begin++;

			if (in_begin == in_end)
			{
				out_begin = in_end;
				out_end = in_end;
				paletteIndex = PaletteIndex::Default;
			}
			else if (TokenizeCStyleString(in_begin, in_end, out_begin, out_end))
				paletteIndex = PaletteIndex::String;
			else if (TokenizeCStyleCharacterLiteral(in_begin, in_end, out_begin, out_end))
				paletteIndex = PaletteIndex::CharLiteral;
			else if (TokenizeCStyleIdentifier(in_begin, in_end, out_begin, out_end))
				paletteIndex = PaletteIndex::Identifier;
			else if (TokenizeCStyleNumber(in_begin, in_end, out_begin, out_end))
				paletteIndex = PaletteIndex::Number;
			else if (TokenizeCStylePunctuation(in_begin, in_end, out_begin, out_end))
				paletteIndex = PaletteIndex::Punctuation;

			return paletteIndex != PaletteIndex::Max;
		};

		langDef.mCommentStart = "/*";
		langDef.mCommentEnd = "*/";
		langDef.mSingleLineComment = "//";
		langDef.mNestedComments = true;
		langDef.mMultilineStrings = true;

		langDef.mCaseSensitive = true;
		//langDef.mAutoIndentation = true;

		langDef.mName = "Daslang";

		inited = true;
	}
	return langDef;
}


static eastl::string parse_first_word(const char * line, bool & is_last_word)
{
	eastl::string w;

	int i = 0;
	const char * p = line;

	while (*p && isspace(*p))
		p++;

	while (*p && (isalnum(*p) || *p == '_'))
	{
		w += *p;
		p++;
	}

	while (*p && isspace(*p))
		p++;

	is_last_word = !*p;
	return w;
}


static const char * find_pair_brace(const char * p, char open, char close)
{
	int braceCounter = 0;
	while (*p)
	{
		if (*p == open)
			braceCounter++;
		else if (*p == close)
		{
			braceCounter--;
			if (braceCounter == 0)
				return p;
		}
		p++;
	}
	return nullptr;
}


char TextEditor::RequiredCloseChar(char charBefore, char openChar, char charAfter)
{
 	if (mLanguageDefinition == &LanguageDefinition::Cpp() ||
 	    mLanguageDefinition == &LanguageDefinition::Daslang())
	{
		if (strchr(" \t)]}=;,.\"\'<>|", charAfter))
		{
			if (openChar == '\'' || openChar == '\"')
			{
				if (strchr(" \t=+-*/([{><:?,|.~;", charBefore) == nullptr)
					return 0;

				if ((!charBefore || (strchr("\"\'\\", charBefore) == nullptr) && (!charAfter || strchr("\"\'", charAfter) == nullptr)))
					return openChar;
			}

			switch (openChar)
			{
				case '{': return '}';
				case '[': return ']';
				case '(': return ')';
				default: return 0;
			}
		}
	}

	return 0;
}


bool TextEditor::RequireIndentationAfterNewLine(const eastl::string &line) const
{
	int braceCounter[3] = { 0 }; // (), [], {}
	bool inString = false;
	char stringChar = 0;

	for (int i = 0; i < line.size(); i++)
	{
		auto c = line[i];
		if (inString)
		{
			if (c == stringChar && line[i - 1] != '\\')
				inString = false;
			continue;
		}
		else
		{
			if (c == '"' || c == '\'')
			{
				inString = true;
				stringChar = c;
				continue;
			}
		}

		if (c == '/' && i < line.size() - 1)
		{
			if (line[i + 1] == '/')
			{
				if (mLanguageDefinition == &LanguageDefinition::Cpp() ||
				    mLanguageDefinition == &LanguageDefinition::Daslang())
					return false;
			}
		}

		switch (c)
		{
		case '(': braceCounter[0]++; break;
		case '{': braceCounter[1]++; break;
		case '[': braceCounter[2]++; break;
		case ')': if (braceCounter[0] > 0) braceCounter[0]--; break;
		case '}': if (braceCounter[1] > 0) braceCounter[1]--; break;
		case ']': if (braceCounter[2] > 0) braceCounter[2]--; break;
		default: break;
		}
	}

	if (braceCounter[0] > 0 || braceCounter[1] > 0 || braceCounter[2] > 0)
		return true;


	if (mLanguageDefinition == &LanguageDefinition::Daslang())
	{
		static const char * const startKeywordsIndent[] = {
			"if", "elif", "else", "for", "while", "switch", "class", "struct", "enum", "def", "try", "except", "finally", "with", "lambda"
		};

		static const char * const singleKeywordsIndent[] = {
			"let", "var"
		};

		bool singleWord = true;
		eastl::string w = parse_first_word(line.c_str(), singleWord);

		for (auto &k : startKeywordsIndent)
		{
			if (w == k)
				return true;
		}

		if (singleWord)
			for (auto &k : singleKeywordsIndent)
				if (w == k)
					return true;

		// block_call() $ (arg1, arg2)
		const char * lambdaPos = strrchr(line.c_str(), '$');
		if (lambdaPos)
		{
			const char * endOfParameter = find_pair_brace(lambdaPos, '(', ')');
			if (!endOfParameter)
				return true;

			const char * p = endOfParameter + 1;
			while (*p && isspace(*p))
				p++;

			bool nonSpaceSymbols = *p && !isspace(*p);

			if (!nonSpaceSymbols)
				return true;
		}
	}

	return false;
}
