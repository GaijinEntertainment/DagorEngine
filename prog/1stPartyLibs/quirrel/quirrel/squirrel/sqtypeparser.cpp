#include "sqtypeparser.h"
#include <ctype.h> // for "tolower"

struct RawTypeDecl
{
    const SQChar * names[3]; // only [0] is valid, the rest are synonyms
    SQInteger typeMask;
};

static RawTypeDecl rawTypeDecls[] = {
    { { _SC("bool"), _SC("boolean"), NULL }, _RT_BOOL },
    { { _SC("number"), _SC("num"), NULL }, (_RT_FLOAT | _RT_INTEGER) },
    { { _SC("int"), _SC("integer"), NULL }, _RT_INTEGER },
    { { _SC("float"), _SC("double"), _SC("real") }, _RT_FLOAT },
    { { _SC("string"), _SC("str"), NULL }, _RT_STRING },
    { { _SC("table"), _SC("dict"), _SC("map") }, _RT_TABLE },
    { { _SC("array"), _SC("list"), _SC("vector") }, _RT_ARRAY },
    { { _SC("userdata"), _SC("user"), _SC("object") }, _RT_USERDATA },
    { { _SC("function"), _SC("func"), _SC("closure") }, (_RT_CLOSURE | _RT_NATIVECLOSURE) },
    { { _SC("generator"), _SC("gen"), _SC("yield") }, _RT_GENERATOR },
    { { _SC("userpointer"), _SC("ptr"), _SC("pointer") }, _RT_USERPOINTER },
    { { _SC("thread"), _SC("coroutine"), _SC("fiber") }, _RT_THREAD },
    { { _SC("instance"), _SC("inst"), _SC("object") }, _RT_INSTANCE },
    { { _SC("class"), NULL, NULL }, _RT_CLASS },
    { { _SC("weakref"), _SC("reference"), _SC("ref") }, _RT_WEAKREF },
    { { _SC("null"), _SC("nil"), _SC("none") }, _RT_NULL },
    { { _SC("any"), NULL, NULL }, -1 },
    { { NULL, NULL, NULL }, 0 }
};

static bool is_space(SQChar c)
{
    return c == _SC(' ') || c == _SC('\t') || c == _SC('\r') || c == _SC('\n');
}

static bool is_alpha(SQChar c)
{
    return (c >= _SC('a') && c <= _SC('z')) || (c >= _SC('A') && c <= _SC('Z'));
}

static bool is_digit(SQChar c)
{
    return c >= _SC('0') && c <= _SC('9');
}

static bool is_alnum(SQChar c)
{
    return is_alpha(c) || is_digit(c);
}

static const SQChar* skip_spaces(const SQChar* s)
{
    while (*s && is_space(*s))
        s++;
    return s;
}

static bool is_str_equal_ignore_case(const SQChar* str1, const SQChar* str2)
{
    while (*str1 && *str2 && tolower(*str1) == tolower(*str2))
    {
        str1++;
        str2++;
    }
    return *str1 == *str2;
}

static bool parse_identifier(SQVM* vm, const SQChar*& s, SQObjectPtr& res)
{
    const SQChar* p = s;
    if (!is_alpha(*p) && *p != _SC('_'))
        return false;
    p++;
    while (is_alnum(*p) || *p == _SC('_'))
        p++;
    res = SQString::Create(_ss(vm), s, p - s);
    s = p;
    return true;
}

static bool parse_type_mask(SQVM* vm, const SQChar*& s, SQInteger& mask, SQInteger& error_pos, SQObjectPtr& error_string)
{
    mask = 0;
    const SQChar* p = skip_spaces(s);
    bool hasBrackets = false;

    if (*p == '(')
    {
        hasBrackets = true;
        p++;
        p = skip_spaces(p);
    }

    for (;;)
    {
        SQObjectPtr typeName;
        if (!parse_identifier(vm, p, typeName))
        {
            error_pos = p - s;
            error_string = SQString::Create(_ss(vm), _SC("Expected type name"));
            return false;
        }

        bool found = false;
        for (RawTypeDecl* decl = rawTypeDecls; decl->names[0]; decl++)
        {
            if (!strcmp(_stringval(typeName), decl->names[0]))
            {
                mask |= decl->typeMask;
                found = true;
                break;
            }

            for (int syn = 0; syn < 3; syn++)
            {
                if (decl->names[syn] && is_str_equal_ignore_case(_stringval(typeName), decl->names[syn]))
                {
                    error_pos = p - s;
                    SQChar buf[256];
                    scsprintf(buf, 256, _SC("Invalid type name '%s', did you mean '%s'?"), _stringval(typeName), decl->names[0]);
                    error_string = SQString::Create(_ss(vm), buf);
                    return false;
                }
            }
        }

        if (!found)
        {
            error_pos = p - s;
            SQChar buf[256];
            scsprintf(buf, 256, _SC("Invalid type name '%s'"), _stringval(typeName));
            error_string = SQString::Create(_ss(vm), buf);
            return false;
        }

        p = skip_spaces(p);
        if (*p == '|')
        {
            p++;
            p = skip_spaces(p);
        }
        else
        {
            break;
        }
    }

    if (hasBrackets)
    {
        if (*p != ')')
        {
            error_pos = p - s;
            error_string = SQString::Create(_ss(vm), _SC("Expected ')' after type list"));
            return false;
        }
        p++;
        p = skip_spaces(p);
    }

    s = p;
    return true;
}

bool sq_parse_function_type_string(SQVM* vm, const SQChar* s, SQFunctionType& res, SQInteger& error_pos, SQObjectPtr& error_string)
{
    if (!s || !*s)
    {
        error_pos = 1;
        error_string = SQString::Create(_ss(vm), _SC("Empty function type string"));
        return false;
    }

    const SQChar* p = skip_spaces(s);
    res.objectTypeMask = -1;
    res.returnTypeMask = _RT_NULL;
    res.requiredArgs = 0;
    res.ellipsisArgTypeMask = 0;
    res.argNames.clear();
    res.argTypeMask.clear();

    if (strncmp(p, "pure ", 5) == 0)
    {
        res.pure = true;
        p = skip_spaces(p + 5);
    }

    if (*p == '(')
    {
        if (!parse_type_mask(vm, p, res.objectTypeMask, error_pos, error_string))
        {
            error_pos += p - s + 1;
            return false;
        }
        p = skip_spaces(p);
        if (*p != '.')
        {
            error_pos = p - s + 1;
            error_string = SQString::Create(_ss(vm), _SC("Expected '.' after object type"));
            return false;
        }
        p++;
        p = skip_spaces(p);
        if (!parse_identifier(vm, p, res.functionName))
        {
            error_pos = p - s + 1;
            error_string = SQString::Create(_ss(vm), _SC("Expected function name after '.'"));
            return false;
        }
    }
    else
    {
        SQObjectPtr identifier1;
        const SQChar* p_initial = p;
        if (!parse_identifier(vm, p, identifier1))
        {
            error_pos = p - s + 1;
            error_string = SQString::Create(_ss(vm), _SC("Expected function name"));
            return false;
        }

        if (*p == '.')
        {
            p++;
            p = skip_spaces(p);
            if (!parse_identifier(vm, p, res.functionName))
            {
                error_pos = p - s + 1;
                error_string = SQString::Create(_ss(vm), _SC("Expected function name after '.'"));
                return false;
            }

            const SQChar* typeStr = _stringval(identifier1);
            const SQChar* typeStrPtr = typeStr;
            SQInteger objectTypeMask;
            SQInteger error_pos_local;
            SQObjectPtr error_string_local;

            if (!parse_type_mask(vm, typeStrPtr, objectTypeMask, error_pos_local, error_string_local))
            {
                error_pos = (p_initial - s) + error_pos_local;
                error_string = error_string_local;
                return false;
            }

            if (*typeStrPtr != '\0')
            {
                error_pos = (p_initial - s) + (typeStrPtr - typeStr);
                error_string = SQString::Create(_ss(vm), _SC("Invalid object type"));
                return false;
            }

            res.objectTypeMask = objectTypeMask;
        }
        else
        {
            res.functionName = identifier1;
        }
    }

    p = skip_spaces(p);

    if (*p != '(')
    {
        error_pos = p - s + 1;
        error_string = SQString::Create(_ss(vm), _SC("Expected '(' after function name"));
        return false;
    }

    p++;
    p = skip_spaces(p);

    bool insideString = false;
    bool insideOptional = false;
    bool optionalBlockFinished = false;
    bool argumentProcessed = false;
    while (*p != ')')
    {
        p = skip_spaces(p);
        if (*p == '\0')
        {
            error_pos = p - s + 1;
            error_string = SQString::Create(_ss(vm), _SC("Unterminated argument list"));
            return false;
        }

        if (*p == '[')
        {
            if (insideOptional)
            {
                error_pos = p - s + 1;
                error_string = SQString::Create(_ss(vm), _SC("Nested optional blocks are not allowed"));
                return false;
            }
            if (optionalBlockFinished)
            {
                error_pos = p - s + 1;
                error_string = SQString::Create(_ss(vm), _SC("Optional block must be the last argument"));
                return false;
            }
            insideOptional = true;
            p++;
            p = skip_spaces(p);
            continue;
        }

        if (*p == ']')
        {
            if (!insideOptional)
            {
                error_pos = p - s + 1;
                error_string = SQString::Create(_ss(vm), _SC("Unmatched ']'"));
                return false;
            }
            insideOptional = false;
            optionalBlockFinished = true;
            p++;
            p = skip_spaces(p);
            continue;
        }

        if (*p == ',')
        {
            if (!argumentProcessed)
            {
                error_pos = p - s + 1;
                error_string = SQString::Create(_ss(vm), _SC("Argument expected before ','"));
                return false;
            }
            p++;
            p = skip_spaces(p);
            argumentProcessed = false;
            continue;
        }

        if (strncmp(p, _SC("..."), 3) == 0)
        {
            argumentProcessed = true;

            if (res.ellipsisArgTypeMask != 0)
            {
                error_pos = p - s + 1;
                error_string = SQString::Create(_ss(vm), _SC("Multiple ellipsis arguments"));
                return false;
            }

            p += 3;
            p = skip_spaces(p);

            if (*p == ':')
            {
                p++;
                p = skip_spaces(p);
                if (!parse_type_mask(vm, p, res.ellipsisArgTypeMask, error_pos, error_string))
                {
                    error_pos += p - s + 1;
                    return false;
                }
            }
            else
            {
                res.ellipsisArgTypeMask = -1;
            }

            p = skip_spaces(p);

            if (*p != ')' && *p != ']')
            {
                error_pos = p - s + 1;
                error_string = SQString::Create(_ss(vm), _SC("Expected ')' after ellipsis argument"));
                return false;
            }

            continue;
        }

        if (optionalBlockFinished)
        {
            error_pos = p - s + 1;
            error_string = SQString::Create(_ss(vm), _SC("Argument after optional block"));
            return false;
        }

        SQObjectPtr argName;
        if (!parse_identifier(vm, p, argName))
        {
            error_pos = p - s + 1;
            error_string = SQString::Create(_ss(vm), _SC("Expected argument name"));
            return false;
        }

        argumentProcessed = true;

        SQInteger argTypeMask = -1;
        p = skip_spaces(p);
        if (*p == ':')
        {
            p++;
            p = skip_spaces(p);
            if (!parse_type_mask(vm, p, argTypeMask, error_pos, error_string))
            {
                error_pos += p - s + 1;
                return false;
            }
        }
        else if (*p != ',' && *p != ')' && *p != ']' && *p != '[' && *p != '=')
        {
            error_pos = p - s + 1;
            error_string = SQString::Create(_ss(vm), _SC("Expected ':' after argument name"));
            return false;
        }

        SQObjectPtr defaultValue;
        SQChar stringOpener = '\0';

        if (*p == '=')
        {
            p++;
            p = skip_spaces(p);

            char stack[40];
            int stackIndex = 0;

            const SQChar * startDefaultValue = p;
            while (*p != '\0')
            {
                if (insideString)
                {
                    if (*p == stringOpener)
                    {
                        insideString = false;
                        p++;
                        continue;
                    }
                    else if (*p == '\\')
                    {
                        p++;
                        if (*p == '\0')
                        {
                            error_pos = p - s + 1;
                            error_string = SQString::Create(_ss(vm), _SC("Unterminated string in default value"));
                            return false;
                        }
                    }

                    p++;
                    continue;
                }
                else if (*p == '"' || *p == '\'')
                {
                    insideString = true;
                    stringOpener = *p;
                    p++;
                    continue;
                }

                if (stackIndex >= sizeof(stack) - 1)
                {
                    error_pos = p - s + 1;
                    error_string = SQString::Create(_ss(vm), _SC("Default value too complex. Too many nested structures"));
                    return false;
                }

                if (*p == '(')
                    stack[stackIndex++] = '(';
                else if (*p == ')')
                {
                    if (stackIndex == 0)
                        break; // end of default value

                    if (stack[stackIndex - 1] != '(')
                    {
                        error_pos = p - s + 1;
                        error_string = SQString::Create(_ss(vm), _SC("Unmatched ')' in default value"));
                        return false;
                    }
                    stackIndex--;
                }
                else if (*p == '[')
                    stack[stackIndex++] = '[';
                else if (*p == ']')
                {
                    if (stackIndex == 0)
                        break; // end of default value

                    if (stack[stackIndex - 1] != '[')
                    {
                        error_pos = p - s + 1;
                        error_string = SQString::Create(_ss(vm), _SC("Unmatched ']' in default value"));
                        return false;
                    }
                    stackIndex--;
                }
                else if (*p == '{')
                    stack[stackIndex++] = '{';
                else if (*p == '}')
                {
                    if (stackIndex == 0 || stack[stackIndex - 1] != '{')
                    {
                        error_pos = p - s + 1;
                        error_string = SQString::Create(_ss(vm), _SC("Unmatched '}' in default value"));
                        return false;
                    }
                    stackIndex--;
                }
                else if (*p == ',' && stackIndex == 0)
                {
                    break; // end of default value
                }

                p++;
                p = skip_spaces(p);
            }

            if (stackIndex > 0)
            {
                error_pos = p - s + 1;
                error_string = SQString::Create(_ss(vm), _SC("Unfinished default value, unmatched brackets"));
                return false;
            }

            if (*p == '\0')
            {
                error_pos = p - s + 1;
                error_string = SQString::Create(_ss(vm), _SC("Unterminated function type string"));
                return false;
            }

            SQInteger defaultValueLength = p - startDefaultValue;
            if (defaultValueLength > 0)
                defaultValue = SQString::Create(_ss(vm), startDefaultValue, defaultValueLength);
            else
            {
                error_pos = p - s + 1;
                error_string = SQString::Create(_ss(vm), _SC("Expected default value after '='"));
                return false;
            }
        }
        else // have no default value
        {
            if (!insideOptional && res.defaultValues.size() > 0 && !sq_isnull(res.defaultValues.back()))
            {
                error_pos = p - s + 1;
                error_string = SQString::Create(_ss(vm), _SC("Default value expected after optional argument"));
                return false;
            }
        }

        res.argNames.push_back(argName);
        res.argTypeMask.push_back(argTypeMask);
        res.defaultValues.push_back(defaultValue);

        if (!insideOptional && sq_isnull(defaultValue))
        {
            res.requiredArgs++;
        }

        p = skip_spaces(p);
        if (*p == ',')
        {
            p++;
            p = skip_spaces(p);
        }
        else if (*p != ')' && *p != ']' && *p != '[')
        {
            error_pos = p - s + 1;
            error_string = SQString::Create(_ss(vm), _SC("Expected ',' or ')'"));
            return false;
        }
    }

    // *p == ')'

    if (insideOptional)
    {
        error_pos = p - s + 1;
        error_string = SQString::Create(_ss(vm), _SC("Unmatched '['"));
        return false;
    }

    if (insideString)
    {
        error_pos = p - s + 1;
        error_string = SQString::Create(_ss(vm), _SC("Unterminated string in function type"));
        return false;
    }

    p++;
    p = skip_spaces(p);

    if (*p == ':')
    {
        p++;
        p = skip_spaces(p);
        if (!parse_type_mask(vm, p, res.returnTypeMask, error_pos, error_string))
        {
            error_pos += p - s + 1;
            return false;
        }
    }

    p = skip_spaces(p);
    if (*p != '\0')
    {
        error_pos = p - s + 1;
        error_string = SQString::Create(_ss(vm), _SC("Unexpected characters after function type string"));
        return false;
    }

    return true;
}


SQObjectPtr sq_stringify_function_type(SQVM* vm, const SQFunctionType& ft)
{
    const SQInteger bufferSize = 2048; // 2KB buffer
    SQChar buffer[bufferSize];
    SQChar* p = buffer;
    SQChar* end = buffer + bufferSize - 1;

    if (sq_isnull(ft.functionName))
    {
        SQObjectPtr tmp;
        return tmp;
    }

    auto append = [&](const SQChar* str) {
        while (*str && p < end)
        {
            *p++ = *str++;
        }
    };

    auto appendTypeMask = [&](SQInteger typeMask) {
        if (typeMask == -1)
        {
            append(_SC("any"));
            return;
        }

        bool first = true;
        for (const RawTypeDecl* decl = rawTypeDecls; decl->names[0]; decl++)
        {
            if ((typeMask & decl->typeMask) == decl->typeMask)
            {
                if (!first)
                {
                    append(_SC("|"));
                }
                append(decl->names[0]);
                first = false;
                typeMask &= ~decl->typeMask;
            }
        }
    };

    if (ft.objectTypeMask != -1)
    {
        bool isComplexType = true;
        for (const RawTypeDecl* decl = rawTypeDecls; decl->names[0]; decl++)
            if (ft.objectTypeMask == decl->typeMask)
            {
                isComplexType = false;
                break;
            }

        if (isComplexType)
            append(_SC("("));

        appendTypeMask(ft.objectTypeMask);

        if (isComplexType)
            append(_SC(")"));

        append(_SC("."));
    }

    append(_stringval(ft.functionName));

    append(_SC("("));
    for (SQInteger i = 0; i < ft.argNames.size(); i++)
    {
        if (i >= ft.requiredArgs)
        {
            if (i == ft.requiredArgs)
            {
                append(i == 0 ? _SC("[") : _SC(", ["));
            }
        }

        if (i > 0 && i != ft.requiredArgs)
        {
            append(_SC(", "));
        }

        append(_stringval(ft.argNames[i]));

        if (ft.argTypeMask[i] != -1)
        {
            append(_SC(": "));
            appendTypeMask(ft.argTypeMask[i]);
        }

        if (!sq_isnull(ft.defaultValues[i]))
        {
            append(_SC(" = "));
            if (sq_isstring(ft.defaultValues[i]))
                append(_stringval(ft.defaultValues[i]));
            else
                append(_SC("<default value>"));
        }
    }

    if (ft.argNames.size() > ft.requiredArgs)
    {
        append(_SC("]"));
    }

    if (ft.ellipsisArgTypeMask != 0)
    {
        if (ft.argNames.size() > 0)
        {
            append(_SC(", "));
        }
        append(_SC("..."));
        if (ft.ellipsisArgTypeMask != -1)
        {
            append(_SC(": "));
            appendTypeMask(ft.ellipsisArgTypeMask);
        }
    }

    append(_SC(")"));

    if (ft.returnTypeMask != _RT_NULL)
    {
        append(_SC(": "));
        appendTypeMask(ft.returnTypeMask);
    }

    *p = '\0';

    return SQObjectPtr(SQString::Create(_ss(vm), buffer, p - buffer));
}
