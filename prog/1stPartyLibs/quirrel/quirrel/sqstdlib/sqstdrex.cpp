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
    SQInteger _currsubexp;
    void *_jmpbuf;
    const char **_error;
};

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

static const char *sqstd_rex_matchnode(SQRex* exp,SQRexNode *node,const char *str,SQRexNode *next)
{

    SQRexNodeType type = node->type;
    switch(type) {
    case OP_GREEDY: {
        //SQRexNode *greedystop = (node->next != -1) ? &exp->_nodes[node->next] : NULL;
        SQRexNode *greedystop = NULL;
        SQInteger p0 = (node->right >> 16)&0x0000FFFF, p1 = node->right&0x0000FFFF, nmaches = 0;
        const char *s=str, *good = str;

        if(node->next != -1) {
            greedystop = &exp->_nodes[node->next];
        }
        else {
            greedystop = next;
        }

        while((nmaches == 0xFFFF || nmaches < p1)) {

            const char *stop;
            const char *prev = s;
            if(!(s = sqstd_rex_matchnode(exp,&exp->_nodes[node->left],s,greedystop)))
                break;
            nmaches++;
            good=s;
            if(s == prev)
                break; // zero-width match, don't loop
            if(greedystop) {
                //checks that 0 matches satisfy the expression(if so skips)
                //if not would always stop(for instance if is a '?')
                if(greedystop->type != OP_GREEDY ||
                ((greedystop->right >> 16)&0x0000FFFF) != 0)
                {
                    SQRexNode *gnext = NULL;
                    if(greedystop->next != -1) {
                        gnext = &exp->_nodes[greedystop->next];
                    }else if(next && next->next != -1){
                        gnext = &exp->_nodes[next->next];
                    }
                    stop = sqstd_rex_matchnode(exp,greedystop,s,gnext);
                    if(stop) {
                        //if satisfied stop it
                        if(p0 == p1 && p0 == nmaches) break;
                        else if(nmaches >= p0 && p1 == 0xFFFF) break;
                        else if(nmaches >= p0 && nmaches <= p1) break;
                    }
                }
            }

            if(s >= exp->_eol)
                break;
        }
        if(p0 == p1 && p0 == nmaches) return good;
        else if(nmaches >= p0 && p1 == 0xFFFF) return good;
        else if(nmaches >= p0 && nmaches <= p1) return good;
        return NULL;
    }
    case OP_OR: {
            const char *asd = str;
            SQRexNode *temp=&exp->_nodes[node->left];
            while( (asd = sqstd_rex_matchnode(exp,temp,asd,NULL)) ) {
                if(temp->next != -1)
                    temp = &exp->_nodes[temp->next];
                else
                    return asd;
            }
            asd = str;
            temp = &exp->_nodes[node->right];
            while( (asd = sqstd_rex_matchnode(exp,temp,asd,NULL)) ) {
                if(temp->next != -1)
                    temp = &exp->_nodes[temp->next];
                else
                    return asd;
            }
            return NULL;
    }
    case OP_EXPR:
    case OP_NOCAPEXPR:{
            SQRexNode *n = &exp->_nodes[node->left];
            const char *cur = str;
            SQInteger capture = -1;
            if(node->type != OP_NOCAPEXPR && node->right == exp->_currsubexp) {
                capture = exp->_currsubexp;
                exp->_matches[capture].begin = cur;
                exp->_currsubexp++;
            }
            SQInteger tempcap = exp->_currsubexp;
            do {
                SQRexNode *subnext = NULL;
                if(n->next != -1) {
                    subnext = &exp->_nodes[n->next];
                }else {
                    subnext = next;
                }
                if(!(cur = sqstd_rex_matchnode(exp,n,cur,subnext))) {
                    if(capture != -1){
                        exp->_matches[capture].begin = 0;
                        exp->_matches[capture].len = 0;
                    }
                    return NULL;
                }
            } while((n->next != -1) && (n = &exp->_nodes[n->next]));

            exp->_currsubexp = tempcap;
            if(capture != -1)
                exp->_matches[capture].len = cur - exp->_matches[capture].begin;
            return cur;
    }
    case OP_WB: {
        SQBool isWordBoundary;
        if(str == exp->_bol) {
            isWordBoundary = (str < exp->_eol && !sq_isspace(*str)) ? SQTrue : SQFalse;
        } else if(str == exp->_eol) {
            isWordBoundary = !sq_isspace(*(str-1)) ? SQTrue : SQFalse;
        } else {
            isWordBoundary = (sq_isspace(*str) != sq_isspace(*(str-1))) ? SQTrue : SQFalse;
        }
        return (node->left == 'b') ? (isWordBoundary ? str : NULL) : (isWordBoundary ? NULL : str);
    }
    case OP_BOL:
        if(str == exp->_bol) return str;
        return NULL;
    case OP_EOL:
        if(str == exp->_eol) return str;
        return NULL;
    case OP_DOT:{
        if (str == exp->_eol) return NULL;
        str++;
                }
        return str;
    case OP_NCLASS:
    case OP_CLASS:
        if (str == exp->_eol) return NULL;
        if(sqstd_rex_matchclass(exp,&exp->_nodes[node->left],*str)?(type == OP_CLASS?SQTrue:SQFalse):(type == OP_NCLASS?SQTrue:SQFalse)) {
            str++;
            return str;
        }
        return NULL;
    case OP_CCLASS:
        if (str == exp->_eol) return NULL;
        if(sqstd_rex_matchcclass(node->left,*str)) {
            str++;
            return str;
        }
        return NULL;
    case OP_MB:
        {
            SQInteger cb = node->left; //char that opens a balanced expression
            if(str == exp->_eol || *str != cb) return NULL; // string doesnt start with open char
            SQInteger ce = node->right; //char that closes a balanced expression
            SQInteger cont = 1;
            const char *streol = exp->_eol;
            while (++str < streol) {
              if (*str == ce) {
                if (--cont == 0) {
                    return ++str;
                }
              }
              else if (*str == cb) cont++;
            }
        }
        return NULL; // string ends out of balance
    default: /* char */
        if (str == exp->_eol) return NULL;
        if(*str != node->type) return NULL;
        str++;
        return str;
    }
    return NULL;
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
        sq_free(exp->_alloc_ctx, exp, sizeof(SQRex));
    }
}

SQBool sqstd_rex_match(SQRex* exp,const char* text)
{
    const char* res = NULL;
    exp->_bol = text;
    exp->_eol = text + strlen(text);
    exp->_currsubexp = 0;
    res = sqstd_rex_matchnode(exp,exp->_nodes,text,NULL);
    if(res == NULL || res != exp->_eol)
        return SQFalse;
    return SQTrue;
}

SQBool sqstd_rex_searchrange(SQRex* exp,const char* text_begin,const char* text_end,const char** out_begin, const char** out_end)
{
    const char *cur = NULL;
    SQInteger node;
    if(text_begin >= text_end) return SQFalse;
    exp->_bol = text_begin;
    exp->_eol = text_end;
    do {
        node = exp->_first;
        cur = text_begin;
        while(node != -1) {
            exp->_currsubexp = 0;
            cur = sqstd_rex_matchnode(exp,&exp->_nodes[node],cur,NULL);
            if(!cur)
                break;
            node = exp->_nodes[node].next;
        }
        text_begin++;
    } while(cur == NULL && text_begin != text_end);

    if(cur == NULL)
        return SQFalse;

    --text_begin;

    if(out_begin) *out_begin = text_begin;
    if(out_end) *out_end = cur;
    return SQTrue;
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

