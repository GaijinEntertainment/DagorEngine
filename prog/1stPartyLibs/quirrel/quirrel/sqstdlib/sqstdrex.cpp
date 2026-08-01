/* see copyright notice in squirrel.h */
#include <squirrel.h>
#include <string.h>
#include <ctype.h>
#include <setjmp.h>
#include <sqstdstring.h>
#include <sq_char_class.h>

#define SQREX_DEBUG 0

#if SQREX_DEBUG
#include <stdio.h>

static const char *g_nnames[] =
{
    "NONE","OP_GREEDY",   "OP_OR",
    "OP_EXPR","OP_NOCAPEXPR","OP_DOT",   "OP_CLASS",
    "OP_CCLASS","OP_NCLASS","OP_RANGE","OP_CHAR",
    "OP_EOL","OP_BOL","OP_WB","OP_MB"
};

#endif

#define SQ_MAX_CHAR 0xFF

#define OP_GREEDY       (SQ_MAX_CHAR+1) // * + ? {n}
#define OP_OR           (SQ_MAX_CHAR+2)
#define OP_EXPR         (SQ_MAX_CHAR+3) //parentesis ()
#define OP_NOCAPEXPR    (SQ_MAX_CHAR+4) //parentesis (?:)
#define OP_DOT          (SQ_MAX_CHAR+5)
#define OP_CLASS        (SQ_MAX_CHAR+6)
#define OP_CCLASS       (SQ_MAX_CHAR+7)
#define OP_NCLASS       (SQ_MAX_CHAR+8) //negates class the [^
#define OP_RANGE        (SQ_MAX_CHAR+9)
//#define OP_CHAR       (SQ_MAX_CHAR+10)
#define OP_EOL          (SQ_MAX_CHAR+11)
#define OP_BOL          (SQ_MAX_CHAR+12)
#define OP_WB           (SQ_MAX_CHAR+13)
#define OP_MB           (SQ_MAX_CHAR+14) //match balanced

#define SQREX_SYMBOL_ANY_CHAR ('.')
#define SQREX_SYMBOL_GREEDY_ONE_OR_MORE ('+')
#define SQREX_SYMBOL_GREEDY_ZERO_OR_MORE ('*')
#define SQREX_SYMBOL_GREEDY_ZERO_OR_ONE ('?')
#define SQREX_SYMBOL_BRANCH ('|')
#define SQREX_SYMBOL_END_OF_STRING ('$')
#define SQREX_SYMBOL_BEGINNING_OF_STRING ('^')
#define SQREX_SYMBOL_ESCAPE_CHAR ('\\')


typedef int SQRexNodeType;

typedef struct tagSQRexNode{
    SQRexNodeType type;
    SQInteger left;
    SQInteger right;
    SQInteger next;
}SQRexNode;

// one committed repetition of a greedy quantifier: where it ended and which
// of the atom's alternative matches (in leftmost-first order) was taken
typedef struct tagSQRexRep{
    const char *pos;
    SQInteger k;
}SQRexRep;

struct SQRex{
    SQAllocContext _alloc_ctx;
    const char *_eol;
    const char *_bol;
    const char *_p;
    SQInteger _first;
    SQRexNode *_nodes;
    SQInteger _nallocated;
    SQInteger _nsize;
    SQInteger _depth;
    SQInteger _nsubexpr;
    SQRexMatch *_matches;
    SQRexRep *_greedyreps; // scratch stack of repetition states shared by nested greedy frames
    SQInteger _greedyalloc;
    SQInteger _greedyused;
    SQRexMatch *_capsnaps; // scratch stack of capture snapshots shared by nested greedy frames
    SQInteger _capsnapalloc;
    SQInteger _capsnapused;
    SQInteger _skip;   // atom solutions left to skip during enumeration
    SQInteger _steps;  // step budget left in the current match call
    SQBool _aborted;   // budget or memory exhausted, the match is unwinding
    void *_jmpbuf;
    const char **_error;
};

// per-call step budget: runaway backtracking is cut off instead of hanging
// the game. The quadratic term keeps parity with the previous engine, whose
// failed unanchored searches with a leading quantifier were O(len^2); the cap
// bounds the worst-case abort latency to a few seconds.
static SQInteger sqstd_rex_stepbudget(SQInteger len)
{
    SQInteger quad = len < 8192 ? len * len : 8192 * 8192;
    return 1000000 + 64 * len + 4 * quad;
}

static SQInteger sqstd_rex_list(SQRex *exp);

static SQInteger sqstd_rex_newnode(SQRex *exp, SQRexNodeType type)
{
    SQRexNode n;
    n.type = type;
    n.next = n.right = n.left = -1;
    if(type == OP_EXPR)
        n.right = exp->_nsubexpr++;
    if(exp->_nallocated < (exp->_nsize + 1)) {
        SQInteger oldsize = exp->_nallocated;
        exp->_nallocated *= 2;
        exp->_nodes = (SQRexNode *)sq_realloc(exp->_alloc_ctx,exp->_nodes, oldsize * sizeof(SQRexNode) ,exp->_nallocated * sizeof(SQRexNode));
    }
    exp->_nodes[exp->_nsize++] = n;
    SQInteger newid = exp->_nsize - 1;
    return (SQInteger)newid;
}

static void sqstd_rex_error(SQRex *exp,const char *error)
{
    if(exp->_error) *exp->_error = error;
    longjmp(*((jmp_buf*)exp->_jmpbuf),-1);
}

static void sqstd_rex_expect(SQRex *exp, SQInteger n){
    if((*exp->_p) != n)
        sqstd_rex_error(exp, "expected paren");
    exp->_p++;
}

static char sqstd_rex_escapechar(SQRex *exp)
{
    if(*exp->_p == SQREX_SYMBOL_ESCAPE_CHAR){
        exp->_p++;
        if(*exp->_p == '\0') sqstd_rex_error(exp,"letter expected");
        switch(*exp->_p) {
        case 'v': exp->_p++; return '\v';
        case 'n': exp->_p++; return '\n';
        case 't': exp->_p++; return '\t';
        case 'r': exp->_p++; return '\r';
        case 'f': exp->_p++; return '\f';
        default: return (*exp->_p++);
        }
    } else if(!sq_isprint(*exp->_p)) sqstd_rex_error(exp,"letter expected");
    return (*exp->_p++);
}

static SQInteger sqstd_rex_charclass(SQRex *exp,SQInteger classid)
{
    SQInteger n = sqstd_rex_newnode(exp,OP_CCLASS);
    exp->_nodes[n].left = classid;
    return n;
}

static SQInteger sqstd_rex_charnode(SQRex *exp,SQBool isclass)
{
    char t;
    if(*exp->_p == SQREX_SYMBOL_ESCAPE_CHAR) {
        exp->_p++;
        switch(*exp->_p) {
            case 'n': exp->_p++; return sqstd_rex_newnode(exp,'\n');
            case 't': exp->_p++; return sqstd_rex_newnode(exp,'\t');
            case 'r': exp->_p++; return sqstd_rex_newnode(exp,'\r');
            case 'f': exp->_p++; return sqstd_rex_newnode(exp,'\f');
            case 'v': exp->_p++; return sqstd_rex_newnode(exp,'\v');
            case 'a': case 'A': case 'w': case 'W': case 's': case 'S':
            case 'd': case 'D': case 'x': case 'X': case 'c': case 'C':
            case 'p': case 'P': case 'l': case 'u':
                {
                t = *exp->_p; exp->_p++;
                return sqstd_rex_charclass(exp,t);
                }
            case 'm':
                {
                     exp->_p++; //skip 'm'
                     if(*exp->_p == '\0') sqstd_rex_error(exp,"balanced chars expected");
                     char cb = *exp->_p; //cb = character begin match
                     exp->_p++;
                     if(*exp->_p == '\0') sqstd_rex_error(exp,"balanced chars expected");
                     char ce = *exp->_p; //ce = character end match
                     exp->_p++; //points to the next char to be parsed
                     if ( cb == ce ) sqstd_rex_error(exp,"open/close char can't be the same");
                     SQInteger node =  sqstd_rex_newnode(exp,OP_MB);
                     exp->_nodes[node].left = cb;
                     exp->_nodes[node].right = ce;
                     return node;
                }
            case 0:
                sqstd_rex_error(exp,"letter expected for argument of escape sequence");
                break;
            case '1': case '2': case '3': case '4': case '5':
            case '6': case '7': case '8': case '9':
                // PCRE backreference syntax; reject it instead of silently
                // matching a literal digit
                if(!isclass)
                    sqstd_rex_error(exp,"backreferences are not supported");
                [[fallthrough]];
            case 'b':
            case 'B':
                if(!isclass) {
                    SQInteger node = sqstd_rex_newnode(exp,OP_WB);
                    exp->_nodes[node].left = *exp->_p;
                    exp->_p++;
                    return node;
                }
                [[fallthrough]];
            default:
                t = *exp->_p; exp->_p++;
                return sqstd_rex_newnode(exp,t);
        }
    }
    else if(!sq_isprint(*exp->_p)) {

        sqstd_rex_error(exp,"letter expected");
    }
    t = *exp->_p; exp->_p++;
    return sqstd_rex_newnode(exp,t);
}
static SQInteger sqstd_rex_class(SQRex *exp)
{
    SQInteger ret = -1;
    SQInteger first = -1,chain;
    if(*exp->_p == SQREX_SYMBOL_BEGINNING_OF_STRING){
        ret = sqstd_rex_newnode(exp,OP_NCLASS);
        exp->_p++;
    }else ret = sqstd_rex_newnode(exp,OP_CLASS);

    if(*exp->_p == ']') sqstd_rex_error(exp,"empty class");
    chain = ret;
    while(*exp->_p != ']' && *exp->_p != '\0') {
        if(*exp->_p == '-' && first != -1){
            SQInteger r;
            exp->_p++; // skip '-'
            if(*exp->_p == ']') sqstd_rex_error(exp,"unfinished range");
            r = sqstd_rex_newnode(exp,OP_RANGE);
            if(exp->_nodes[first].type == OP_CCLASS) sqstd_rex_error(exp,"cannot use character classes in ranges");
            exp->_nodes[r].left = exp->_nodes[first].type;
            SQInteger t = sqstd_rex_escapechar(exp);
            if(exp->_nodes[r].left > t) sqstd_rex_error(exp,"invalid range");
            exp->_nodes[r].right = t;
            exp->_nodes[chain].next = r;
            chain = r;
            first = -1;
        }
        else{
            if(first!=-1){
                SQInteger c = first;
                exp->_nodes[chain].next = c;
                chain = c;
                first = sqstd_rex_charnode(exp,SQTrue);
            }
            else{
                first = sqstd_rex_charnode(exp,SQTrue);
            }
        }
    }
    if(*exp->_p == '\0') sqstd_rex_error(exp,"unterminated character class");
    if(first!=-1){
        SQInteger c = first;
        exp->_nodes[chain].next = c;
    }
    /* hack? */
    exp->_nodes[ret].left = exp->_nodes[ret].next;
    exp->_nodes[ret].next = -1;
    return ret;
}

static SQInteger sqstd_rex_parsenumber(SQRex *exp)
{
    SQInteger ret = *exp->_p-'0';
    SQInteger positions = 10;
    exp->_p++;
    while(sq_isdigit(*exp->_p)) {
        ret = ret*10+(*exp->_p++-'0');
        if(positions==1000000000) sqstd_rex_error(exp,"overflow in numeric constant");
        positions *= 10;
    };
    return ret;
}

static SQInteger sqstd_rex_element(SQRex *exp)
{
    exp->_depth++;

    struct AutoDec {
        AutoDec(SQInteger *varPtr) : varPtr(varPtr) {}
        ~AutoDec() { (*varPtr)--; }
        SQInteger *varPtr;
    } autodec(&exp->_depth);

    if(exp->_depth > 200) sqstd_rex_error(exp, "pattern exceeds maximum allowed nesting depth");
    if(exp->_nsize > 2000) sqstd_rex_error(exp, "pattern too complex");
    SQInteger ret = -1;
    switch(*exp->_p)
    {
    case '(': {
        SQInteger expr;
        exp->_p++;


        if(*exp->_p =='?') {
            exp->_p++;
            sqstd_rex_expect(exp,':');
            expr = sqstd_rex_newnode(exp,OP_NOCAPEXPR);
        }
        else
            expr = sqstd_rex_newnode(exp,OP_EXPR);
        SQInteger newn = sqstd_rex_list(exp);
        exp->_nodes[expr].left = newn;
        ret = expr;
        sqstd_rex_expect(exp,')');
              }
              break;
    case '[':
        exp->_p++;
        ret = sqstd_rex_class(exp);
        sqstd_rex_expect(exp,']');
        break;
    case SQREX_SYMBOL_END_OF_STRING: exp->_p++; ret = sqstd_rex_newnode(exp,OP_EOL);break;
    case SQREX_SYMBOL_ANY_CHAR: exp->_p++; ret = sqstd_rex_newnode(exp,OP_DOT);break;
    default:
        ret = sqstd_rex_charnode(exp,SQFalse);
        break;
    }


    SQBool isgreedy = SQFalse;
    unsigned short p0 = 0, p1 = 0;
    switch(*exp->_p){
        case SQREX_SYMBOL_GREEDY_ZERO_OR_MORE: p0 = 0; p1 = 0xFFFF; exp->_p++; isgreedy = SQTrue; break;
        case SQREX_SYMBOL_GREEDY_ONE_OR_MORE: p0 = 1; p1 = 0xFFFF; exp->_p++; isgreedy = SQTrue; break;
        case SQREX_SYMBOL_GREEDY_ZERO_OR_ONE: p0 = 0; p1 = 1; exp->_p++; isgreedy = SQTrue; break;
        case '{':
            exp->_p++;
            {
                if(!sq_isdigit(*exp->_p)) sqstd_rex_error(exp,"number expected");
                SQInteger n = sqstd_rex_parsenumber(exp);
                if(n > 0xFFFE) sqstd_rex_error(exp,"quantifier value too large");
                p0 = (unsigned short)n;
            }
            /*******************************/
            switch(*exp->_p) {
        case '}':
            p1 = p0; exp->_p++;
            break;
        case ',':
            exp->_p++;
            p1 = 0xFFFF;
            if(sq_isdigit(*exp->_p)){
                SQInteger n = sqstd_rex_parsenumber(exp);
                if(n > 0xFFFE) sqstd_rex_error(exp,"quantifier value too large");
                p1 = (unsigned short)n;
            }
            sqstd_rex_expect(exp,'}');
            if(p0 > p1) sqstd_rex_error(exp,"invalid quantifier range: min > max");
            break;
        default:
            sqstd_rex_error(exp,", or } expected");
            }
            /*******************************/
            isgreedy = SQTrue;
            break;

    }
    if(isgreedy) {
        // a '?' after a quantifier is PCRE lazy-match syntax; reject it instead
        // of silently treating the '?' as a literal or a nested quantifier
        if(*exp->_p == SQREX_SYMBOL_GREEDY_ZERO_OR_ONE)
            sqstd_rex_error(exp,"lazy quantifiers are not supported");
        SQInteger nnode = sqstd_rex_newnode(exp,OP_GREEDY);
        exp->_nodes[nnode].left = ret;
        exp->_nodes[nnode].right = ((SQInteger)p0<<16)|p1;
        ret = nnode;
    }

    if((*exp->_p != SQREX_SYMBOL_BRANCH) && (*exp->_p != ')') && (*exp->_p != SQREX_SYMBOL_GREEDY_ZERO_OR_MORE) && (*exp->_p != SQREX_SYMBOL_GREEDY_ONE_OR_MORE) && (*exp->_p != '\0')) {
        SQInteger nnode = sqstd_rex_element(exp);
        exp->_nodes[ret].next = nnode;
    }

    return ret;
}

static SQInteger sqstd_rex_list(SQRex *exp)
{
    SQInteger ret=-1,e;
    if(*exp->_p == SQREX_SYMBOL_BEGINNING_OF_STRING) {
        exp->_p++;
        ret = sqstd_rex_newnode(exp,OP_BOL);
    }
    e = sqstd_rex_element(exp);
    if(ret != -1) {
        exp->_nodes[ret].next = e;
    }
    else ret = e;

    if(*exp->_p == SQREX_SYMBOL_BRANCH) {
        SQInteger temp,tright;
        exp->_p++;
        temp = sqstd_rex_newnode(exp,OP_OR);
        exp->_nodes[temp].left = ret;
        tright = sqstd_rex_list(exp);
        exp->_nodes[temp].right = tright;
        ret = temp;
    }
    return ret;
}

static SQBool sqstd_rex_matchcclass(SQInteger cclass,char c)
{
    switch(cclass) {
    case 'a': return sq_isalpha(c)?SQTrue:SQFalse;
    case 'A': return !sq_isalpha(c)?SQTrue:SQFalse;
    case 'w': return (sq_isalnum(c) || c == '_')?SQTrue:SQFalse;
    case 'W': return (!sq_isalnum(c) && c != '_')?SQTrue:SQFalse;
    case 's': return sq_isspace(c)?SQTrue:SQFalse;
    case 'S': return !sq_isspace(c)?SQTrue:SQFalse;
    case 'd': return sq_isdigit(c)?SQTrue:SQFalse;
    case 'D': return !sq_isdigit(c)?SQTrue:SQFalse;
    case 'x': return sq_isxdigit(c)?SQTrue:SQFalse;
    case 'X': return !sq_isxdigit(c)?SQTrue:SQFalse;
    case 'c': return sq_iscntrl(c)?SQTrue:SQFalse;
    case 'C': return !sq_iscntrl(c)?SQTrue:SQFalse;
    case 'p': return sq_ispunct(c)?SQTrue:SQFalse;
    case 'P': return !sq_ispunct(c)?SQTrue:SQFalse;
    case 'l': return sq_islower(c)?SQTrue:SQFalse;
    case 'u': return sq_isupper(c)?SQTrue:SQFalse;
    }
    return SQFalse; /*cannot happen*/
}

static SQBool sqstd_rex_isword(char c)
{
    return (sq_isalnum(c) || c == '_') ? SQTrue : SQFalse;
}

static SQBool sqstd_rex_matchclass(SQRex* exp,SQRexNode *node,char c)
{
    do {
        switch(node->type) {
            case OP_RANGE:
                if((unsigned char)c >= node->left && (unsigned char)c <= node->right) return SQTrue;
                break;
            case OP_CCLASS:
                if(sqstd_rex_matchcclass(node->left,c)) return SQTrue;
                break;
            default:
                if(c == node->type)return SQTrue;
        }
    } while((node->next != -1) && (node = &exp->_nodes[node->next]));
    return SQFalse;
}


// special values for SQRexCont::capture
#define SQREX_CAPTURE_NONE (-1)
#define SQREX_CAPTURE_ENUM (-2) // terminal frame: count/skip atom solutions
#define SQREX_CAPTURE_EOL  (-3) // terminal frame: succeed only at end of input

struct SQRexCont {
    SQInteger capture; // capture to close when this frame is reached, or a special value
    SQInteger nodeid;  // node chain to match next, -1 if none
    SQRexCont *outer;
};

static const char *sqstd_rex_matchchain(SQRex *exp, SQInteger nodeid, const char *str, SQRexCont *cont);

// deterministic single nodes: consume input at str or fail, no alternatives
static const char *sqstd_rex_matchsingle(SQRex *exp, SQRexNode *node, const char *str)
{
    SQRexNodeType type = node->type;
    switch(type) {
    case OP_WB: {
        SQBool isWordBoundary;
        if(str == exp->_bol) {
            isWordBoundary = (str < exp->_eol && sqstd_rex_isword(*str)) ? SQTrue : SQFalse;
        } else if(str == exp->_eol) {
            isWordBoundary = sqstd_rex_isword(*(str-1));
        } else {
            isWordBoundary = (sqstd_rex_isword(*str) != sqstd_rex_isword(*(str-1))) ? SQTrue : SQFalse;
        }
        return (node->left == 'b') ? (isWordBoundary ? str : NULL) : (isWordBoundary ? NULL : str);
    }
    case OP_BOL:
        if(str == exp->_bol) return str;
        return NULL;
    case OP_EOL:
        if(str == exp->_eol) return str;
        return NULL;
    case OP_DOT:
        if (str == exp->_eol) return NULL;
        return str + 1;
    case OP_NCLASS:
    case OP_CLASS:
        if (str == exp->_eol) return NULL;
        if(sqstd_rex_matchclass(exp,&exp->_nodes[node->left],*str)?(type == OP_CLASS?SQTrue:SQFalse):(type == OP_NCLASS?SQTrue:SQFalse))
            return str + 1;
        return NULL;
    case OP_CCLASS:
        if (str == exp->_eol) return NULL;
        if(sqstd_rex_matchcclass(node->left,*str))
            return str + 1;
        return NULL;
    case OP_MB:
        {
            SQInteger cb = node->left; //char that opens a balanced expression
            if(str == exp->_eol || *str != cb) return NULL; // string doesnt start with open char
            SQInteger ce = node->right; //char that closes a balanced expression
            SQInteger nesting = 1;
            const char *streol = exp->_eol;
            while (++str < streol) {
              if (*str == ce) {
                if (--nesting == 0) {
                    return ++str;
                }
              }
              else if (*str == cb) nesting++;
            }
        }
        return NULL; // string ends out of balance
    default: /* char */
        if (str == exp->_eol) return NULL;
        if(*str != node->type) return NULL;
        return str + 1;
    }
}

// capture slots used by a subtree; group ids are assigned in parse order, so
// the slots of a subtree form the contiguous range [lo, hi)
static void sqstd_rex_capturerange(SQRex *exp, SQInteger nodeid, SQInteger *lo, SQInteger *hi)
{
    while(nodeid != -1) {
        SQRexNode *node = &exp->_nodes[nodeid];
        switch(node->type) {
        case OP_EXPR:
            if(node->right < *lo) *lo = node->right;
            if(node->right + 1 > *hi) *hi = node->right + 1;
            sqstd_rex_capturerange(exp, node->left, lo, hi);
            break;
        case OP_NOCAPEXPR:
        case OP_GREEDY:
            sqstd_rex_capturerange(exp, node->left, lo, hi);
            break;
        case OP_OR:
            sqstd_rex_capturerange(exp, node->left, lo, hi);
            sqstd_rex_capturerange(exp, node->right, lo, hi);
            break;
        default: // OP_CLASS/OP_NCLASS chains and plain nodes contain no groups
            break;
        }
        nodeid = node->next;
    }
}

// caps the repetition-state scratch (16 bytes per entry); the budget limits
// work, this limits peak memory for complex atoms repeated over huge inputs
#define SQREX_MAX_GREEDY_REPS (1<<20)

static SQBool sqstd_rex_growreps(SQRex *exp, SQInteger need)
{
    if(need > SQREX_MAX_GREEDY_REPS) {
        exp->_aborted = SQTrue;
        return SQFalse;
    }
    if(need <= exp->_greedyalloc)
        return SQTrue;
    SQInteger newalloc = exp->_greedyalloc > 0 ? exp->_greedyalloc : 64;
    while(newalloc < need) newalloc *= 2;
    SQRexRep *p = exp->_greedyreps
        ? (SQRexRep *)sq_realloc(exp->_alloc_ctx, exp->_greedyreps,
            exp->_greedyalloc * sizeof(SQRexRep), newalloc * sizeof(SQRexRep))
        : (SQRexRep *)sq_malloc(exp->_alloc_ctx, newalloc * sizeof(SQRexRep));
    if(!p) {
        exp->_aborted = SQTrue; // out of memory: fail the whole match safely
        return SQFalse;
    }
    exp->_greedyreps = p;
    exp->_greedyalloc = newalloc;
    return SQTrue;
}

// reserve ncap capture-snapshot slots from the shared arena; returns the base
// index (access via exp->_capsnaps + base, re-derived after any recursion that
// may have grown the arena). Returns -1 on abort.
static SQInteger sqstd_rex_reservesnap(SQRex *exp, SQInteger ncap)
{
    SQInteger base = exp->_capsnapused;
    SQInteger need = base + ncap;
    if(need > exp->_capsnapalloc) {
        if(need > SQREX_MAX_GREEDY_REPS) {
            exp->_aborted = SQTrue;
            return -1;
        }
        SQInteger newalloc = exp->_capsnapalloc > 0 ? exp->_capsnapalloc : 64;
        while(newalloc < need) newalloc *= 2;
        SQRexMatch *p = exp->_capsnaps
            ? (SQRexMatch *)sq_realloc(exp->_alloc_ctx, exp->_capsnaps,
                exp->_capsnapalloc * sizeof(SQRexMatch), newalloc * sizeof(SQRexMatch))
            : (SQRexMatch *)sq_malloc(exp->_alloc_ctx, newalloc * sizeof(SQRexMatch));
        if(!p) {
            exp->_aborted = SQTrue;
            return -1;
        }
        exp->_capsnaps = p;
        exp->_capsnapalloc = newalloc;
    }
    exp->_capsnapused = need;
    return base;
}

// release large scratch stacks at the end of a match call; small ones are
// kept so typical patterns don't reallocate on every call
static void sqstd_rex_shrinkreps(SQRex *exp)
{
    if(exp->_greedyalloc > 4096) {
        sq_free(exp->_alloc_ctx, exp->_greedyreps, exp->_greedyalloc * sizeof(SQRexRep));
        exp->_greedyreps = NULL;
        exp->_greedyalloc = 0;
    }
    if(exp->_capsnapalloc > 4096) {
        sq_free(exp->_alloc_ctx, exp->_capsnaps, exp->_capsnapalloc * sizeof(SQRexMatch));
        exp->_capsnaps = NULL;
        exp->_capsnapalloc = 0;
    }
}

// find the k-th way (leftmost-first order) the atom chain can match at str.
// The enumeration terminal rejects the first k solutions, which forces the
// matcher to backtrack into the atom's alternations and nested quantifiers.
static const char *sqstd_rex_atomsolution(SQRex *exp, SQInteger atom, const char *str, SQInteger k)
{
    SQRexCont endc;
    endc.capture = SQREX_CAPTURE_ENUM;
    endc.nodeid = -1;
    endc.outer = NULL;
    SQInteger saveskip = exp->_skip; // enumerations nest; keep the outer count
    exp->_skip = k;
    const char *r = sqstd_rex_matchchain(exp, atom, str, &endc);
    exp->_skip = saveskip;
    return r;
}

static const char *sqstd_rex_matchcont(SQRex *exp, SQRexCont *cont, const char *str)
{
    if(!cont)
        return str;
    if(cont->capture == SQREX_CAPTURE_ENUM) {
        if(exp->_skip > 0) {
            exp->_skip--;
            return NULL; // reject this solution, keep backtracking for the next
        }
        return str;
    }
    if(cont->capture == SQREX_CAPTURE_EOL)
        return str == exp->_eol ? str : NULL;
    if(cont->capture != SQREX_CAPTURE_NONE) {
        SQRexMatch *m = &exp->_matches[cont->capture];
        SQInteger oldlen = m->len;
        m->len = str - m->begin;
        const char *r = sqstd_rex_matchchain(exp, cont->nodeid, str, cont->outer);
        if(!r)
            m->len = oldlen;
        return r;
    }
    return sqstd_rex_matchchain(exp, cont->nodeid, str, cont->outer);
}

// Greedy repetition as an explicit-stack depth-first search: each committed
// repetition records where it ended and which atom solution it took, so on a
// tail failure the last repetition can switch to the atom's next alternative
// (not just drop a repetition). Keeps the leftmost-greedy order of a fully
// recursive matcher while the C stack stays bounded by pattern size.
static const char *sqstd_rex_matchgreedy(SQRex *exp, SQRexNode *node, const char *str, SQRexCont *cont)
{
    SQInteger p0 = (node->right >> 16)&0x0000FFFF, p1 = node->right&0x0000FFFF;
    SQInteger maxrep;
    if(p1 != 0xFFFF)
        maxrep = p1; // zero-width iterations can outnumber the remaining input
    else
        maxrep = (exp->_eol - str) + 1; // +1: one trailing zero-width repetition

    // fast path: a single deterministic node of width 1 (dot, class, plain
    // char) has exactly one solution per position, so a repetition end is just
    // str + i and no backtracking state has to be stored. A transparent
    // (?:...) wrapper repeats like its body, and a capturing (X) wrapper only
    // adds one capture slot that ends up holding the last repetition's char.
    SQRexNode *atomn = &exp->_nodes[node->left];
    while(atomn->type == OP_NOCAPEXPR && atomn->next == -1)
        atomn = &exp->_nodes[atomn->left];
    SQInteger fastcap = -1; // capture slot to fill on the width-1 fast path
    if(atomn->type == OP_EXPR && atomn->next == -1) {
        SQRexNode *body = &exp->_nodes[atomn->left];
        while(body->type == OP_NOCAPEXPR && body->next == -1)
            body = &exp->_nodes[body->left];
        if(body->next == -1 && body->type != OP_GREEDY && body->type != OP_OR
            && body->type != OP_EXPR && body->type != OP_NOCAPEXPR
            && body->type != OP_WB && body->type != OP_BOL && body->type != OP_EOL
            && body->type != OP_MB)
        {
            fastcap = atomn->right;
            atomn = body;
        }
    }
    if(atomn->next == -1 && atomn->type != OP_GREEDY && atomn->type != OP_OR
        && atomn->type != OP_EXPR && atomn->type != OP_NOCAPEXPR
        && atomn->type != OP_WB && atomn->type != OP_BOL && atomn->type != OP_EOL
        && atomn->type != OP_MB)
    {
        SQInteger cnt = 0;
        const char *s = str;
        while(cnt < maxrep) {
            const char *r = sqstd_rex_matchsingle(exp, atomn, s);
            if(!r)
                break;
            s = r;
            cnt++;
        }
        exp->_steps -= cnt; // the scan bypassed matchchain's step accounting
        if(exp->_steps < 0)
            exp->_aborted = SQTrue;
        const char *savebegin = NULL;
        SQInteger savelen = 0;
        if(fastcap != -1) {
            savebegin = exp->_matches[fastcap].begin;
            savelen = exp->_matches[fastcap].len;
        }
        for(; cnt >= p0 && !exp->_aborted; cnt--) {
            if(fastcap != -1) {
                // last-iteration capture semantics: the single char at cnt-1,
                // or an unset slot when the group matched zero times
                exp->_matches[fastcap].begin = cnt > 0 ? str + (cnt - 1) : 0;
                exp->_matches[fastcap].len = cnt > 0 ? 1 : 0;
            }
            const char *r = (node->next != -1)
                ? sqstd_rex_matchchain(exp, node->next, str + cnt, cont)
                : sqstd_rex_matchcont(exp, cont, str + cnt);
            if(r)
                return r;
        }
        if(fastcap != -1) { // no repetition count matched: restore the slot
            exp->_matches[fastcap].begin = savebegin;
            exp->_matches[fastcap].len = savelen;
        }
        return NULL;
    }

    // repetition states live in a scratch stack shared with nested greedy
    // frames; growth may move it, so re-derive the pointer after any recursion
    SQInteger base = exp->_greedyused;
#define SQREX_REP(i) (exp->_greedyreps[base + (i)])

    // snapshot the capture slots of the atom subtree: rolled back on failure,
    // replayed to match the committed repetitions on success. The snapshot
    // lives in a shared arena reused across probes and nested frames; growth
    // may move it, so address it by index and re-derive the pointer on use.
    SQInteger caplo = exp->_nsubexpr, caphi = 0;
    sqstd_rex_capturerange(exp, node->left, &caplo, &caphi);
    SQInteger ncap = caphi > caplo ? caphi - caplo : 0;
    SQInteger snapbase = -1;
    if(ncap > 0) {
        snapbase = sqstd_rex_reservesnap(exp, ncap);
        if(snapbase < 0)
            return NULL; // aborted
        memcpy(exp->_capsnaps + snapbase, exp->_matches + caplo, ncap * sizeof(SQRexMatch));
    }

    SQInteger cnt = 0;      // committed repetitions
    const char *cur = str;  // end of the last committed repetition
    SQBool dirty = SQFalse; // backtracked: captures no longer match the commits
    const char *result = NULL;
    enum { ST_EXTEND, ST_TAIL, ST_BACKTRACK } st = ST_EXTEND;

    while(!exp->_aborted) {
        if(st == ST_EXTEND) {
            // greedily commit repetitions, first atom solution each time; the
            // state slot is claimed only after the atom matched, so a final
            // failing iteration is not counted against the scratch cap
            while(cnt < maxrep) {
                const char *r = sqstd_rex_atomsolution(exp, node->left, cur, 0);
                if(!r)
                    break;
                if(!sqstd_rex_growreps(exp, base + cnt + 1))
                    break; // scratch cap or memory: aborted
                if(exp->_greedyused < base + cnt + 1)
                    exp->_greedyused = base + cnt + 1;
                SQREX_REP(cnt).pos = r;
                SQREX_REP(cnt).k = 0;
                cnt++;
                if(r == cur) {
                    // zero-width: more iterations add nothing, but commit up to
                    // the required minimum so finite {n,m} bounds are satisfied
                    while(cnt < p0 && sqstd_rex_growreps(exp, base + cnt + 1)) {
                        if(exp->_greedyused < base + cnt + 1)
                            exp->_greedyused = base + cnt + 1;
                        SQREX_REP(cnt).pos = r;
                        SQREX_REP(cnt).k = 0;
                        cnt++;
                    }
                    break;
                }
                cur = r;
            }
            st = ST_TAIL;
        }
        else if(st == ST_TAIL) {
            if(cnt >= p0) {
                // the probe matches the whole remaining pattern (tail groups are
                // disjoint from atom groups, so stale atom captures can't leak)
                const char *r = (node->next != -1)
                    ? sqstd_rex_matchchain(exp, node->next, cur, cont)
                    : sqstd_rex_matchcont(exp, cont, cur);
                if(r) {
                    result = r;
                    break;
                }
            }
            st = ST_BACKTRACK;
        }
        else { // ST_BACKTRACK: switch the last repetition to the atom's next solution
            if(cnt == 0)
                break;
            const char *start = (cnt > 1) ? SQREX_REP(cnt-2).pos : str;
            SQInteger k = SQREX_REP(cnt-1).k + 1;
            const char *r = sqstd_rex_atomsolution(exp, node->left, start, k);
            dirty = SQTrue;
            if(r) {
                SQREX_REP(cnt-1).pos = r;
                SQREX_REP(cnt-1).k = k;
                cur = r;
                if(r == start) {
                    // zero-width alternative: extending adds nothing, but the
                    // required minimum must still be reached (same solution k
                    // repeats at the same position, so the copies stay valid
                    // for capture replay and further backtracking)
                    while(cnt < p0 && sqstd_rex_growreps(exp, base + cnt + 1)) {
                        if(exp->_greedyused < base + cnt + 1)
                            exp->_greedyused = base + cnt + 1;
                        SQREX_REP(cnt).pos = r;
                        SQREX_REP(cnt).k = k;
                        cnt++;
                    }
                    st = ST_TAIL;
                } else
                    st = ST_EXTEND;
            } else {
                cnt--; // atom solutions exhausted here; retry with one repetition less
                cur = (cnt > 0) ? SQREX_REP(cnt-1).pos : str;
                st = ST_TAIL;
            }
        }
    }

    if(exp->_aborted)
        result = NULL;
    if(snapbase >= 0) {
        if(result && dirty) {
            // rebuild atom captures for the committed solution path
            memcpy(exp->_matches + caplo, exp->_capsnaps + snapbase, ncap * sizeof(SQRexMatch));
            const char *rp = str;
            for(SQInteger i = 0; i < cnt && !exp->_aborted; i++)
                rp = sqstd_rex_atomsolution(exp, node->left, rp, SQREX_REP(i).k);
            if(exp->_aborted)
                result = NULL;
        }
        else if(!result) {
            memcpy(exp->_matches + caplo, exp->_capsnaps + snapbase, ncap * sizeof(SQRexMatch));
        }
        exp->_capsnapused = snapbase;
    }
    exp->_greedyused = base;
#undef SQREX_REP
    return result;
}

static const char *sqstd_rex_matchchain(SQRex *exp, SQInteger nodeid, const char *str, SQRexCont *cont)
{
    if(exp->_aborted || --exp->_steps < 0) {
        exp->_aborted = SQTrue;
        return NULL;
    }
    while(nodeid != -1) {
        SQRexNode *node = &exp->_nodes[nodeid];
        switch(node->type) {
        case OP_GREEDY:
            return sqstd_rex_matchgreedy(exp, node, str, cont);
        case OP_OR: {
            SQRexCont c;
            c.capture = -1; c.nodeid = node->next; c.outer = cont;
            const char *r = sqstd_rex_matchchain(exp, node->left, str, &c);
            if(r) return r;
            // left branch failed and unwound its captures; try the right one
            return sqstd_rex_matchchain(exp, node->right, str, &c);
        }
        case OP_EXPR:
        case OP_NOCAPEXPR: {
            SQInteger capture = (node->type == OP_EXPR) ? node->right : -1;
            const char *oldbegin = NULL;
            SQInteger oldlen = 0;
            if(capture != -1) {
                SQRexMatch *m = &exp->_matches[capture];
                oldbegin = m->begin; oldlen = m->len;
                m->begin = str;
            }
            // the group close (recording the capture length) is a continuation
            // frame, so it happens exactly when the group body has matched
            SQRexCont c;
            c.capture = capture; c.nodeid = node->next; c.outer = cont;
            const char *r = sqstd_rex_matchchain(exp, node->left, str, &c);
            if(!r && capture != -1) {
                exp->_matches[capture].begin = oldbegin;
                exp->_matches[capture].len = oldlen;
            }
            return r;
        }
        default:
            str = sqstd_rex_matchsingle(exp, node, str);
            if(!str) return NULL;
            nodeid = node->next;
            break;
        }
    }
    return sqstd_rex_matchcont(exp, cont, str);
}

/* public api */
SQRex *sqstd_rex_compile(SQAllocContext alloc_ctx, const char *pattern,const char **error)
{
    SQRex * volatile exp = (SQRex *)sq_malloc(alloc_ctx, sizeof(SQRex)); // "volatile" is needed for setjmp()
    exp->_alloc_ctx = alloc_ctx;
    exp->_eol = exp->_bol = NULL;
    exp->_p = pattern;
    exp->_nallocated = (SQInteger)strlen(pattern) * sizeof(char);
    if(exp->_nallocated < 4) exp->_nallocated = 4;
    exp->_nodes = (SQRexNode *)sq_malloc(alloc_ctx, exp->_nallocated * sizeof(SQRexNode));
    exp->_nsize = 0;
    exp->_depth = 0;
    exp->_matches = 0;
    exp->_greedyreps = NULL;
    exp->_greedyalloc = 0;
    exp->_greedyused = 0;
    exp->_capsnaps = NULL;
    exp->_capsnapalloc = 0;
    exp->_capsnapused = 0;
    exp->_skip = 0;
    exp->_steps = 0;
    exp->_aborted = SQFalse;
    exp->_nsubexpr = 0;
    exp->_first = sqstd_rex_newnode(exp,OP_EXPR);
    exp->_error = error;
    exp->_jmpbuf = sq_malloc(alloc_ctx, sizeof(jmp_buf));
    if(setjmp(*((jmp_buf*)exp->_jmpbuf)) == 0) {
        SQInteger res = sqstd_rex_list(exp);
        exp->_nodes[exp->_first].left = res;
        if(*exp->_p!='\0')
            sqstd_rex_error(exp,"unexpected character");
#if SQREX_DEBUG
        {
            SQInteger nsize,i;
            SQRexNode *t;
            nsize = exp->_nsize;
            t = &exp->_nodes[0];
            printf("\n");
            for(i = 0;i < nsize; i++) {
                if(exp->_nodes[i].type>SQ_MAX_CHAR)
                    printf("[%02d] %10s ", (SQInt32)i,g_nnames[exp->_nodes[i].type-SQ_MAX_CHAR]);
                else
                    printf("[%02d] %10c ", (SQInt32)i,exp->_nodes[i].type);
                printf("left %02d right %02d next %02d\n", (SQInt32)exp->_nodes[i].left, (SQInt32)exp->_nodes[i].right, (SQInt32)exp->_nodes[i].next);
            }
            printf("\n");
        }
#endif
        exp->_matches = (SQRexMatch *) sq_malloc(alloc_ctx, exp->_nsubexpr * sizeof(SQRexMatch));
        memset(exp->_matches,0,exp->_nsubexpr * sizeof(SQRexMatch));
    }
    else{
        sqstd_rex_free(exp);
        return NULL;
    }
    return exp;
}

void sqstd_rex_free(SQRex *exp)
{
    if(exp) {
        if(exp->_nodes) sq_free(exp->_alloc_ctx, exp->_nodes,exp->_nallocated * sizeof(SQRexNode));
        if(exp->_jmpbuf) sq_free(exp->_alloc_ctx, exp->_jmpbuf,sizeof(jmp_buf));
        if(exp->_matches) sq_free(exp->_alloc_ctx, exp->_matches,exp->_nsubexpr * sizeof(SQRexMatch));
        if(exp->_greedyreps) sq_free(exp->_alloc_ctx, exp->_greedyreps,exp->_greedyalloc * sizeof(SQRexRep));
        if(exp->_capsnaps) sq_free(exp->_alloc_ctx, exp->_capsnaps,exp->_capsnapalloc * sizeof(SQRexMatch));
        sq_free(exp->_alloc_ctx, exp, sizeof(SQRex));
    }
}

static void sqstd_rex_beginmatch(SQRex *exp, const char *bol, const char *eol)
{
    exp->_bol = bol;
    exp->_eol = eol;
    exp->_greedyused = 0;
    exp->_capsnapused = 0;
    exp->_skip = 0;
    exp->_steps = sqstd_rex_stepbudget(eol - bol);
    exp->_aborted = SQFalse;
    memset(exp->_matches, 0, exp->_nsubexpr * sizeof(SQRexMatch));
}

SQBool sqstd_rex_match(SQRex* exp,const char* text)
{
    sqstd_rex_beginmatch(exp, text, text + strlen(text));
    // end-of-input is a continuation frame, so alternation and quantifiers
    // backtrack when a shorter candidate does not cover the whole string
    SQRexCont endc;
    endc.capture = SQREX_CAPTURE_EOL;
    endc.nodeid = -1;
    endc.outer = NULL;
    const char *res = sqstd_rex_matchchain(exp, exp->_first, text, &endc);
    sqstd_rex_shrinkreps(exp);
    return res != NULL ? SQTrue : SQFalse;
}

SQBool sqstd_rex_searchrange(SQRex* exp,const char* text_begin,const char* text_end,const char** out_begin, const char** out_end)
{
    const char *cur = NULL;
    if(text_begin > text_end) {
        exp->_aborted = SQFalse; // an invalid range is a plain no-match
        return SQFalse;
    }
    sqstd_rex_beginmatch(exp, text_begin, text_end);
    // like Python/PCRE, the empty position at the end of the range is a valid
    // match start, so anchors and possibly-empty patterns can match there
    const char *probe = text_begin;
    for(;;) {
        cur = sqstd_rex_matchchain(exp, exp->_first, probe, NULL);
        if(cur || exp->_aborted || probe == text_end)
            break;
        probe++;
    }
    sqstd_rex_shrinkreps(exp);
    if(cur == NULL)
        return SQFalse;
    if(out_begin) *out_begin = probe;
    if(out_end) *out_end = cur;
    return SQTrue;
}

SQBool sqstd_rex_matchaborted(SQRex* exp)
{
    return exp->_aborted;
}

SQBool sqstd_rex_search(SQRex* exp,const char* text, const char** out_begin, const char** out_end)
{
    return sqstd_rex_searchrange(exp,text,text + strlen(text),out_begin,out_end);
}

SQInteger sqstd_rex_getsubexpcount(SQRex* exp)
{
    return exp->_nsubexpr;
}

SQBool sqstd_rex_getsubexp(SQRex* exp, SQInteger n, SQRexMatch *subexp)
{
    if( n<0 || n >= exp->_nsubexpr) return SQFalse;
    *subexp = exp->_matches[n];
    return SQTrue;
}

