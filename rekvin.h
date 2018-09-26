#ifndef REKVIN_H
#define REKVIN_H

#ifdef  __cplusplus
extern "C" {
#endif//__cplusplus

#define KVIN_LEXEME_TABLE(X)    \
    X(UNKNOWN)                  \
    X(IDENTIFIER)               \
    X(NUMBERLIKE)               \
    X(DOT)                      \
    X(HASH)                     \
    X(EQUALS)                   \
    X(BSTRING)                  \
    X(LBRACKET)                 \
    X(RBRACKET)                 \
    X(EOL)

#define KVIN_PARSES_TABLE(X)    \
    X(ERROR)                    \
    X(INITIAL)                  \
    X(ABSPATH)                  \
    X(RELPATH)                  \
    X(DOT)                      \
    X(PATH)                     \
    X(ASSIGN)                   \
    X(EOL)                      \
    X(DONE)

#define KVIN_ACTION_TABLE(X)    \
    X(NONE)                     \
    X(ERROR)                    \
    X(SETATROOT)                \
    X(SETNEXTAXIS)              \
    X(RELPATH)                  \
    X(AUTONUMBER)               \
    X(SETVALUE)                 \
    X(DONE)

#define KVIN_VALUE_TABLE(X)     \
    X(IDENTIFIER)               \
    X(INTEGER)                  \
    X(REAL)                     \
    X(BSTRING)

typedef enum KVIN_LEXEME
{
#undef  KVIN_LEXEME_ENTRY
#define KVIN_LEXEME_ENTRY(NAME) KVIN_LEX_ ## NAME,
KVIN_LEXEME_TABLE(KVIN_LEXEME_ENTRY)
#undef  KVIN_LEXEME_ENTRY
    KVIN_LEX_MAX,
} KVIN_LEXEME;

typedef enum KVIN_PARSES
{
#undef  KVIN_PARSES_ENTRY
#define KVIN_PARSES_ENTRY(NAME) KVIN_PAR_ ## NAME,
KVIN_PARSES_TABLE(KVIN_PARSES_ENTRY)
#undef  KVIN_PARSES_ENTRY
    KVIN_PAR_MAX,
} KVIN_PARSES;

typedef enum KVIN_ACTION
{
#undef  KVIN_ACTION_ENTRY
#define KVIN_ACTION_ENTRY(NAME) KVIN_ACT_ ## NAME,
KVIN_ACTION_TABLE(KVIN_ACTION_ENTRY)
#undef  KVIN_ACTION_ENTRY
    KVIN_ACT_MAX,
} KVIN_ACTION;

typedef enum KVIN_VALUE
{
#undef  KVIN_VALUE_ENTRY
#define KVIN_VALUE_ENTRY(NAME) KVIN_VAL_ ## NAME,
KVIN_VALUE_TABLE(KVIN_VALUE_ENTRY)
#undef  KVIN_VALUE_ENTRY
    KVIN_VAL_MAX,
} KVIN_VALUE;

typedef struct KVINValue
{
    KVIN_VALUE      type;
    union
    {
        struct
        {
            const char     *begin;
            const char     *end;
        };
        unsigned long long  integer;
        long double         real;
    };
} KVINValue;

typedef struct KVINLexer
{
    KVIN_LEXEME     lex;
    int             lineNo;
    const char*     begin;
    const char*     lbeg;
    const char*     lend;
    const char*     end;
} KVINLexer;

typedef struct KVINParser
{
    KVIN_PARSES     state;
    KVIN_ACTION     action;
    KVINLexer       lexer;
    KVINValue       value;
} KVINParser;

typedef int   (*kvinSetAtRootFptr)      (void* handle, KVINValue);
typedef int   (*kvinSetNextAxisFptr)    (void* handle, KVINValue);
typedef int   (*kvinRelPathFptr)        (void* handle);
typedef int   (*kvinAutonumberFptr)     (void* handle);
typedef int   (*kvinSetValueFptr)       (void* handle, KVINValue);
typedef int   (*kvinErrorFptr)          (void* handle, KVINParser*);
typedef int   (*kvinDoneFptr)           (void* handle);

typedef struct KVINActor
{
    KVINParser              parser;
    void                   *handle;
    kvinSetAtRootFptr       SetAtRoot;
    kvinSetNextAxisFptr     SetNextAxis;
    kvinRelPathFptr         RelPath;
    kvinAutonumberFptr      Autonumber;
    kvinSetValueFptr        SetValue;
    kvinErrorFptr           Error;
    kvinDoneFptr            Done;
} KVINActor;

int kvinInitLex(KVINLexer* lex, const char* fst, const char* lst);
int kvinLexNext(KVINLexer*);
int kvinInitParser(KVINParser* prs, const char* fst, const char* lst);
int kvinParseNext(KVINParser*);
int kvinParseNextCB(KVINActor*);
int kvinParse(KVINActor*);

#ifdef  __cplusplus
} // extern "C".
#endif//__cplusplus

#endif//REKVIN_H

#ifdef  REKVIN_C

#ifndef REKVIN_NO_STDLIB

#include <ctype.h>
#include <stdlib.h>

#ifdef  __cplusplus
extern "C" {
#endif//__cplusplus

#ifndef kvin_assert
#define kvin_assert(MSG) if (!(MSG)) { return 0; }
#endif//kvin_assert

static int kvin_isspace(const char c)
{
    return isspace(c);
}

static int kvin_isalpha(const char c)
{
    return isalpha(c);
}

static int kvin_isdigit(const char c)
{
    return isdigit(c);
}

#else// REKVIN_NO_STDLIB

// XXX: You've got a lot of definin' to do.

#endif//REKVIN_NO_STDLIB

static int kvin_iscid(const char c)
{
    return kvin_isalpha(c) || (c == '_');
}

static int kvin_iscidn(const char c)
{
    return kvin_iscid(c) || kvin_isdigit(c);
}

int kvinInitLex(KVINLexer* lex, const char* fst, const char* lst)
{
    kvin_assert(lex);
    kvin_assert(fst);
    kvin_assert(lst);
    kvin_assert(fst < lst);

    lex->lex        = KVIN_LEX_UNKNOWN;
    lex->lineNo     = 0;
    lex->begin      = fst;
    lex->lbeg       = fst;
    lex->lend       = fst;
    lex->end        = lst;
    return 1;
}

int kvinLexNext(KVINLexer* lex)
{
    kvin_assert(lex);
    kvin_assert(lex->begin <= lex->lbeg);
    kvin_assert(lex->lbeg  <= lex->lend);
    kvin_assert(lex->lend  <= lex->end);
    kvin_assert(lex->begin <  lex->end);

    lex->lbeg       = lex->lend;
    if (lex->lend >= lex->end)
    {
        if (lex->lex != KVIN_LEX_UNKNOWN)
        {
            lex->lex    = KVIN_LEX_EOL;
        }
        return 0;
    }

    lex->lex        = KVIN_LEX_UNKNOWN;
    int wsLike      = 0;

    do
    {
        wsLike      = 0;
        while ((lex->lend < lex->end) && (kvin_isspace(*lex->lend)))
        {
            if (lex->lend[0] == '\n')
            {
                break;
            }
            wsLike  = 1;
            ++lex->lend;
        }

        if (lex->lend >= lex->end)
        {
            return 0;
        }

        if (lex->lend[0] == '/')
        {
            wsLike  = 0;
            ++lex->lend;
            kvin_assert(lex->lend < lex->end);
            kvin_assert(lex->lend[0] == '/');
            while ((lex->lend < lex->end) && (lex->lend[0] != '\n'))
            {
                ++lex->lend;
            }
        }
    } while (wsLike);

    lex->lbeg   = lex->lend;

#undef  KVIN_OP
#define KVIN_OP(X)                  \
    ++lex->lend;                    \
    lex->lex    = KVIN_LEX_ ## X;   \
    return 1

    switch (lex->lend[0])
    {
    case '\n'   : KVIN_OP(EOL);
    case '.'    : KVIN_OP(DOT);
    case '='    : KVIN_OP(EQUALS);
    case '#'    : KVIN_OP(HASH);
    case '['    : KVIN_OP(LBRACKET);
    case ']'    : KVIN_OP(RBRACKET);
    case '"'    :
        ++lex->lend;
        while ((lex->lend < lex->end) && (lex->lend[0] != '"'))
        {
            if (lex->lend[0] == '\\')
            {
                ++lex->lend;
                kvin_assert(lex->lend < lex->end);
            }
            if (lex->lend[0] == '\n')
            {
                ++lex->lineNo;
            }
            ++lex->lend;
        }
        kvin_assert(lex->lend < lex->end);
        kvin_assert(lex->lend[0] == '"');
        ++lex->lend;
        lex->lex    = KVIN_LEX_BSTRING;
        return 1;
    default     : break;
    }

#undef  KVIN_OP

    if (kvin_iscid(*lex->lend) || (lex->lend[0] == '_'))
    {
        while ((lex->lend < lex->end) && kvin_iscidn(*lex->lend))
        {
            ++lex->lend;
        }
        lex->lex    = KVIN_LEX_IDENTIFIER;
        return 1;
    }

    if (kvin_isdigit(*lex->lend))
    {
        while ((lex->lend < lex->end) && kvin_iscidn(*lex->lend))
        {
            ++lex->lend;
        }
        lex->lex    = KVIN_LEX_NUMBERLIKE;
        return 1;
    }

    return 1;
}

int kvinInitParser(KVINParser* prs, const char* fst, const char* lst)
{
    kvin_assert(prs);
    prs->state      = KVIN_PAR_INITIAL;
    prs->action     = KVIN_ACT_NONE;
    return kvinInitLex(&prs->lexer, fst, lst);
}

typedef int (*KVINParse)    (KVINParser* prs);

static int pkvinError(KVINParser* prs)
{
    prs->state          = KVIN_PAR_ERROR;
    return 0;
}

static int pkvinDone(KVINParser* prs)
{
    prs->state          = KVIN_PAR_DONE;
    return 0;
}

static int pkvinCommonPath(KVINParser* prs)
{
    if (prs->lexer.lex == KVIN_LEX_LBRACKET)
    {
        if (!kvinLexNext(&prs->lexer))
        {
            prs->state  = KVIN_PAR_ERROR;
            return 0;
        }
        if (prs->lexer.lex != KVIN_LEX_NUMBERLIKE)
        {
            prs->state  = KVIN_PAR_ERROR;
            return 0;
        }
        prs->value.integer  = strtoull(prs->lexer.lbeg, 0, 0);
        if (!kvinLexNext(&prs->lexer))
        {
            prs->state  = KVIN_PAR_ERROR;
            return 0;
        }
        if (prs->lexer.lex != KVIN_LEX_RBRACKET)
        {
            prs->state  = KVIN_PAR_ERROR;
            return 0;
        }
    }
    else
    if (prs->lexer.lex == KVIN_LEX_HASH)
    {
        prs->action                 = KVIN_ACT_AUTONUMBER;
    }
    else
    {
        switch (prs->lexer.lex)
        {
        case KVIN_LEX_IDENTIFIER    :
            prs->value.begin        = prs->lexer.lbeg;
            prs->value.end          = prs->lexer.lend;
            break;
        case KVIN_LEX_NUMBERLIKE    :
            prs->value.integer      = strtoull(prs->lexer.lbeg, 0, 0);
            break;
        default                     : break;
        }
    }
    return 1;
}

static int pkvinAbsPath(KVINParser* prs)
{
    prs->state          = KVIN_PAR_ABSPATH;
    prs->action         = KVIN_ACT_SETATROOT;
    return pkvinCommonPath(prs);
}

static int pkvinRelPath(KVINParser* prs)
{
    prs->state      = KVIN_PAR_RELPATH;
    prs->action     = KVIN_ACT_RELPATH;
    return 1;
}

static int pkvinPath(KVINParser* prs)
{
    prs->state      = KVIN_PAR_PATH;
    prs->action     = KVIN_ACT_SETNEXTAXIS;
    return pkvinCommonPath(prs);
}

static int pkvinDot(KVINParser* prs)
{
    prs->state      = KVIN_PAR_DOT;
    return 1;
}

static int pkvinAssign(KVINParser* prs)
{
    prs->state      = KVIN_PAR_ASSIGN;
    return 1;
}

static int pkvinValue(KVINParser* prs)
{
    prs->state      = KVIN_PAR_EOL;
    prs->action     = KVIN_ACT_SETVALUE;
    switch (prs->lexer.lex)
    {
    case KVIN_LEX_IDENTIFIER    :
        prs->value.type         = KVIN_VAL_IDENTIFIER;
        prs->value.begin        = prs->lexer.lbeg;
        prs->value.end          = prs->lexer.lend;
        break;
    case KVIN_LEX_NUMBERLIKE    :
        prs->value.type         = KVIN_VAL_INTEGER;
        prs->value.integer      = strtoull(prs->lexer.lbeg, 0, 0);
        break;
    case KVIN_LEX_BSTRING       :
        prs->value.type         = KVIN_VAL_BSTRING;
        prs->value.begin        = prs->lexer.lbeg + 1;
        prs->value.end          = prs->lexer.lend - 1;
        break;
    default                     : break;
    }
    return 1;
}

static int pkvinReset(KVINParser* prs)
{
    prs->state      = KVIN_PAR_INITIAL;
    return 1;
}

static KVINParse kvinParseTable[KVIN_LEX_MAX][KVIN_PAR_MAX] =
{
                        /*  ERROR           INITIAL         ABSPATH         RELPATH         DOT             PATH            ASSIGN          EOL             DONE            */
    /* UNKNOWN    */    {   pkvinError,     pkvinError,     pkvinError,     pkvinError,     pkvinError,     pkvinError,     pkvinError,     pkvinError,     pkvinDone       },
    /* IDENTIFIER */    {   pkvinError,     pkvinAbsPath,   pkvinError,     pkvinPath,      pkvinPath,      pkvinError,     pkvinValue,     pkvinError,     pkvinDone       },
    /* NUMBERLIKE */    {   pkvinError,     pkvinAbsPath,   pkvinError,     pkvinPath,      pkvinPath,      pkvinError,     pkvinValue,     pkvinError,     pkvinDone       },
    /* DOT        */    {   pkvinError,     pkvinRelPath,   pkvinDot,       pkvinRelPath,   pkvinError,     pkvinDot,       pkvinError,     pkvinError,     pkvinDone       },
    /* HASH       */    {   pkvinError,     pkvinError,     pkvinError,     pkvinPath,      pkvinError,     pkvinError,     pkvinError,     pkvinError,     pkvinDone       },
    /* EQUALS     */    {   pkvinError,     pkvinError,     pkvinAssign,    pkvinError,     pkvinError,     pkvinAssign,    pkvinError,     pkvinError,     pkvinDone       },
    /* BSTRING    */    {   pkvinError,     pkvinError,     pkvinError,     pkvinError,     pkvinError,     pkvinError,     pkvinValue,     pkvinError,     pkvinDone       },
    /* LBRACKET   */    {   pkvinError,     pkvinAbsPath,   pkvinPath,      pkvinPath,      pkvinError,     pkvinPath,      pkvinError,     pkvinError,     pkvinDone       },
    /* RBRACKET   */    {   pkvinError,     pkvinError,     pkvinError,     pkvinError,     pkvinError,     pkvinError,     pkvinError,     pkvinError,     pkvinDone       },
    /* EOL        */    {   pkvinError,     pkvinError,     pkvinError,     pkvinError,     pkvinError,     pkvinError,     pkvinError,     pkvinReset,     pkvinDone       },
};

int kvinParseNext(KVINParser* prs)
{
    kvin_assert(prs);

    prs->action         = KVIN_ACT_NONE;

    if (!kvinLexNext(&prs->lexer))
    {
        if ((prs->state != KVIN_PAR_INITIAL) && (prs->state != KVIN_PAR_EOL))
        {
            prs->state  = KVIN_PAR_ERROR;
            prs->action = KVIN_ACT_ERROR;
        }
        else
        {
            prs->state  = KVIN_PAR_DONE;
            prs->action = KVIN_ACT_DONE;
        }
        return 0;
    }

    return kvinParseTable[prs->lexer.lex][prs->state](prs);
}

int kvinParseNextCB(KVINActor* act)
{
    kvin_assert(act);

    int result  = kvinParseNext(&act->parser);

    switch (act->parser.action)
    {
    case KVIN_ACT_NONE          :                                                   break;
    case KVIN_ACT_ERROR         : act->Error(act->handle, &act->parser);            break;
    case KVIN_ACT_SETATROOT     : act->SetAtRoot(act->handle, act->parser.value);   break;
    case KVIN_ACT_SETNEXTAXIS   : act->SetNextAxis(act->handle, act->parser.value); break;
    case KVIN_ACT_RELPATH       : act->RelPath(act->handle);                        break;
    case KVIN_ACT_AUTONUMBER    : act->Autonumber(act->handle);                     break;
    case KVIN_ACT_SETVALUE      : act->SetValue(act->handle, act->parser.value);    break;
    case KVIN_ACT_DONE          : act->Done(act->handle);                           break;
    default                     :
        result = 0;
        act->parser.state       = KVIN_PAR_ERROR;
        break;
    }

    return result;
}

int kvinParse(KVINActor* act)
{
    kvin_assert(act);

    while (kvinParseNextCB(act))
    {
    }

    return (act->parser.state == KVIN_PAR_DONE);
}

#ifdef  __cplusplus
} // extern "C".
#endif//__cplusplus
#endif//REKVIN_C