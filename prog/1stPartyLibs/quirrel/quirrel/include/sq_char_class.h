#pragma once

inline bool sq_isalnum(int c)
{
    unsigned char uc = c;
    return (uc >= 'a' && uc <= 'z') || (uc >= 'A' && uc <= 'Z') || (uc >= '0' && uc <= '9');
}

inline bool sq_isalpha(int c)
{
    unsigned char uc = c;
    return (uc >= 'A' && uc <= 'Z') || (uc >= 'a' && uc <= 'z');
}

inline bool sq_isblank(int c)
{
    unsigned char uc = c;
    return uc == ' ' || uc == '\t';
}

inline bool sq_iscntrl(int c)
{
    unsigned char uc = c;
    return uc <= 31 || uc == 127;
}

inline bool sq_isdigit(int c)
{
    unsigned char uc = c;
    return uc >= '0' && uc <= '9';
}

inline bool sq_isgraph(int c)
{
    unsigned char uc = c;
    return uc >= 33 && uc <= 126;
}

inline bool sq_islower(int c)
{
    unsigned char uc = c;
    return uc >= 'a' && uc <= 'z';
}

inline bool sq_isprint(int c)
{
    unsigned char uc = c;
    return uc >= 32 && uc <= 126;
}

inline bool sq_ispunct(int c)
{
    unsigned char uc = c;
    return (uc >= 33 && uc <= 47) || (uc >= 58 && uc <= 64) || (uc >= 91 && uc <= 96) || (uc >= 123 && uc <= 126);
}

inline bool sq_isspace(int c)
{
    unsigned char uc = c;
    return uc == ' ' || uc == '\t' || uc == '\n' || uc == '\v' || uc == '\f' || uc == '\r';
}

inline bool sq_isupper(int c)
{
    unsigned char uc = c;
    return uc >= 'A' && uc <= 'Z';
}

inline bool sq_isxdigit(int c)
{
    unsigned char uc = c;
    return (uc >= '0' && uc <= '9') || (uc >= 'A' && uc <= 'F') || (uc >= 'a' && uc <= 'f');
}

inline int sq_toupper(int c)
{
    if (c < 'a' || c > 'z')
        return c;
    return c + 'A' - 'a';
}

inline int sq_tolower(int c)
{
    if (c < 'A' || c > 'Z')
        return c;
    return c + 'a' - 'A';
}

