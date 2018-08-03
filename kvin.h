//
// Created by Jacob N. Smith on 26 July 2018.
// BSD 2-clause.
//

#ifndef THE_KVIN_KEY_VALUE_INLINE_NOTATION_H
#define THE_KVIN_KEY_VALUE_INLINE_NOTATION_H

/*

GRAMMAR
-------

    Whitspace is any sequence of isspace() valid characters,
    or a C++ comment. All whitespace is removed & ignored.

          KV ::= AXISPATH `\=` VALUE

       VALUE ::= CIDENTIFIER
              |  NUMBER
              |  FLOAT
              |  STRING
              |  REFERENCE

 CIDENTIFIER ::= `[a-fA-F_][a-fA-F0-9_]*`

      NUMBER ::= `0x[0-9a-fA-f][0-9a-fA-f_']`
              |  `[1-9][0-9_']*`

      STRING ::= `"` CHARS `"`
              |  `""`

   REFERENCE ::= ``` AXISPATH ```

       CHARS ::= CHARS CHAR
              |  CHAR
        CHAR ::= `\\.`
              |  `[^\\"]`

      VALUES ::= VALUES `,` VALUE
              |  VALUE

    AXISPATH ::= AXISPATH `\.` CIDENTIFIER
              |  AXISPATH INDEX
              |  AXISPATH AUTOINDEX
              |  AXISPATH `\.` CIDENTIFIER INDEX
              |  CIDENTIFIER
              |  INDEX
              |  AUTOINDEX
              |  `\.` AXISPATH

       INDEX ::= `\[` NUMBER `\]`
              |  `\.` NUMBER

   AUTOINDEX ::= `\.#`

TRANSITION TABLE
----------------

    STATE\TOKEN |   CID     IDX     DOT     ASSIGN  STR/REF/...
    ------------+-----------------------------------------------
    INIT        |   AXIS    AXIS    ERROR   ERROR   ERROR
    AXIS        |   ERROR   ERROR   PATH    ASSIGN  ERROR
    PATH        |   AXIS    AXIS    PARENT  ERROR   ERROR
    PARENT      |   AXIS    AXIS    PARENT  ERROR   ERROR
    ASSIGN      |   VAL     VAL     ERROR   ERROR   VAL
    VAL         |   AXIS    AXIS    PATH    ERROR   ERROR

DESUGARING
----------

    INDEX
    -----

    [| `[` NUMBER `]` |] => [| `\.` NUMBER  |]

SEMANTICS
---------

    GET_ROOT    -- get the root object of the file
    GET_PARENT  -- the immediate parent of the current object
    SET_AXIS    -- add the axis (key) to the current object, sets the key
    SET_OBJECT  -- asserts the the current key is an object
    SET_VALUE   -- assign a value to the current key

    The parser will return up to 3 semantic actions for a given
    event, saying which action should be taken for that event.

REFERENCES
----------

    The parser does not desugar references. However, references
    are guaranteed to be validly formed paths. It is up to the
    consume to convert these valid paths into proper references,
    if the references are valid.

*/

#ifdef __cplusplus
extern "C"
{
#endif

#define KVIN_EVENT_TYPE_TABLE(X)    \
    X(ERROR)                        \
    X(INTERNAL_ERROR)               \
    X(INIT)                         \
    X(AXIS_INDEX)                   \
    X(AXIS_AUTOINDEX)               \
    X(AXIS_NAME)                    \
    X(PATH)                         \
    X(PARENT)                       \
    X(ASSIGN)                       \
    X(VAL_C_ID)                     \
    X(VAL_NUMBER)                   \
    X(VAL_FLOAT)                    \
    X(VAL_REF)                      \
    X(VAL_STRING)                   \
    X(DONE)                         \
    X(MAX)

typedef enum KVIN_EVENT_TYPE
{
#undef  KVIN_EVENT_TYPE_X
#define KVIN_EVENT_TYPE_X(NAME) KVINE_##NAME,
KVIN_EVENT_TYPE_TABLE(KVIN_EVENT_TYPE_X)
#undef  KVIN_EVENT_TYPE_X
} KVIN_EVENT_TYPE;

#define KVIN_ACTION_TYPE_TABLE(X)   \
    X(NONE)                         \
    X(GET_ROOT)                     \
    X(GET_PARENT)                   \
    X(SET_AXIS)                     \
    X(SET_OBJECT)                   \
    X(SET_VALUE)                    \
    X(MAX)

typedef enum KVIN_ACTION_TYPE
{
#undef  KVIN_ACTION_TYPE_X
#define KVIN_ACTION_TYPE_X(NAME) KVINA_##NAME,
KVIN_ACTION_TYPE_TABLE(KVIN_ACTION_TYPE_X)
#undef  KVIN_ACTION_TYPE_X
} KVIN_ACTION_TYPE;

typedef struct KVINParser
{
    KVIN_EVENT_TYPE     event;
    const char*         beg;
    const char*         end;
    const char*         fst;
    const char*         lst;
    unsigned long long  number;
    long double         lfloat;
    KVIN_ACTION_TYPE    action[3];
} KVINParser;

typedef int (*kvinAction)(void* handle, KVINParser*);

typedef struct KVINActions
{
    void               *handle;
    kvinAction          GetRoot;
    kvinAction          GetParent;
    kvinAction          SetAxis;
    kvinAction          SetObject;
    kvinAction          SetValue;
} KVINActions;

int kvinInit(KVINParser*, const char* fst, const char* lst);
int kvinParseStep(KVINParser*);
int kvinParseStepAct(KVINParser*, KVINActions*);
int kvinParse(KVINParser*, KVINActions*);

#ifdef __cplusplus
}//extern "C"
#endif

#endif//THE_KVIN_KEY_VALUE_INLINE_NOTATION_H

#ifdef  THE_KVIN_KEY_VALUE_INLINE_NOTATION_C

#include <ctype.h>
#include <stdlib.h>
#include <string.h>

int kvinInit(KVINParser* kp, const char* fst, const char* lst)
{
    if (!kp) return 0;
    if (!fst) return 0;
    if (!lst) return 0;
    if (lst <= fst) return 0;

    kp->event           = KVINE_INIT;
    kp->beg             = fst;
    kp->end             = fst;
    kp->fst             = fst;
    kp->lst             = lst;

    kp->action[0]       = KVINA_NONE;
    kp->action[1]       = KVINA_NONE;
    kp->action[2]       = KVINA_NONE;

    kp->number          = (unsigned long long)-1;

    return 1;
}

int kvinEatWS(KVINParser* kp, const char** cur)
{
    int advance = 0;
    do
    {
        advance = 0;
        if (**cur == '/')
        {
            advance = 1;
            ++*cur;
            if (*cur >= kp->lst)
            {
                kp->event   = KVINE_ERROR;
                return 0;
            }
            if (**cur != '/')
            {
                kp->event   = KVINE_ERROR;
                return 0;
            }
            ++*cur;
            for ( ; (*cur < kp->lst) && (**cur != '\n'); ++*cur)
            {
            }

            if ((*cur < kp->lst) && (**cur == '\n'))
            {
                ++*cur;
            }
        }

        while ((*cur < kp->lst) && isspace(**cur))
        {
            advance = 1;
            ++*cur;
        }
    }
    while (advance);

    return 1;
}

int kvinCIdentifier(const char** cur, const char* lst)
{
    if ((('a' <= **cur) && (**cur < 'z'))   ||
        (('A' <= **cur) && (**cur < 'Z'))   ||
        (**cur == '_')                      ||
        0)
    {
        while ( (*cur < lst)                        &&
               ((('a' <= **cur) && (**cur < 'z'))   ||
                (('A' <= **cur) && (**cur < 'Z'))   ||
                (('0' <= **cur) && (**cur < '9'))   ||
                (**cur == '_')                      ||
                0))
        {
            ++*cur;
        }
        return 1;
    }
    return 0;
}

int kvinNumber(const char** cur, const char* lst, unsigned long long* number, int* fradix, int* numLimbs)
{
    if (!number)
    {
        return 0;
    }
    if (('0' <= **cur) && (**cur <= '9'))
    {
        *number = **cur - '0';
        if (*number && numLimbs && *numLimbs)
        {
            *numLimbs   = 1;
        }
        ++*cur;
        if (*cur >= lst)
        {
            return 1;
        }
        int radix   = 10;
        if ((**cur == 'x')  ||
            (**cur == 'X')  ||
            0)
        {
            radix   = 16;
            ++*cur;
            *number = 0;
        }
        else
        if ((**cur == 'o') ||
                 (**cur == 'O') ||
                 0)
        {
            radix   = 8;
            ++*cur;
            *number = 0;
        }
        else
        if ((**cur == 'b') ||
                 (**cur == 'B') ||
                 0)
        {
            radix   = 2;
            ++*cur;
            *number = 0;
        }
        if ((radix != 10) && (*cur >= lst))
        {
            return 0;
        }
        if ((radix == 10) && (*number == 0))
        {
            radix   = 8;
        }
        if (fradix)
        {
            if (*fradix)
            {
                radix   = *fradix;
            }
            *fradix     = radix;
        }
        while ( (*cur < lst)                                        &&
                ((('0' <= **cur) && (**cur <= '1'))                 ||
                 ((radix > 2) && ('2' <= **cur) && (**cur <= '7'))  ||
                 ((radix > 8) && ('8' <= **cur) && (**cur <= '9'))  ||
                 ((radix > 10) && ('A' <= **cur) && (**cur <= 'F')) ||
                 ((radix > 10) && ('a' <= **cur) && (**cur <= 'f')) ||
                 (**cur == '_')                                     ||
                 (**cur == '\'')                                    ||
                 0))
        {
            *number *= radix;
            if (*number && numLimbs && *numLimbs)
            {
                *numLimbs   = 1;
            }
            switch (**cur)
            {
            case '0' : case '1' : case '2' : case '3' : case '4' :
            case '5' : case '6' : case '7' : case '8' : case '9' :
                *number += **cur - '0';
            break;
            case 'a' : case 'b' : case 'c' : case 'd' : case 'e' : case 'f' :
                *number += **cur - 'a' + 10;
                break;
            case 'A' : case 'B' : case 'C' : case 'D' : case 'E' : case 'F' :
                *number += **cur - 'A' + 10;
                break;
            case '_' : break;
            case '\'' : break;
            default   : return 0;
            }
            ++*cur;
        }
        return 1;
    }
    return 0;
}

int kvinLongFloat(const char** fst, const char* lst, long double* lfloat)
{
    if (!lfloat)
    {
        return 0;
    }
    const char* cur = *fst;
    int manPositive                 = 1;
    int expPositive                 = 1;
    int radix                       = 0;
    int numLimbs                    = 0;
    unsigned long long integral     = 0;
    unsigned long long fractional   = 0;
    unsigned long long exponent     = 0;

    if ((*cur == '+') || (*cur == '-'))
    {
        manPositive = *cur == '+';
        ++cur;
    }
    if (cur >= lst) return 0;

    do
    {
        if (!kvinNumber(&cur, lst, &integral, &radix, 0))
        {
            return 0;
        }

        if (cur >= lst)
        {
            break;
        }

        if (*cur != '.')
        {
            break;
        }
        ++cur;

        if (!kvinNumber(&cur, lst, &fractional, &radix, &numLimbs))
        {
            return 0;
        }

        if (((radix == 10) && ((*cur == 'e') || (*cur == 'E'))) ||
            ((radix == 8) && ((*cur == 'p') || (*cur == 'P')))  ||
            0)
        {
            ++cur;
        }
        else
        {
            break;
        }

        if (cur >= lst)
        {
            return 0;
        }

        if ((*cur == '+') || (*cur == '-'))
        {
            expPositive = *cur == '+';
        }

        if (cur >= lst)
        {
            return 0;
        }

        if (!kvinNumber(&cur, lst, &exponent, &radix, 0))
        {
            return 0;
        }

    } while (0);

    long double frc = 1.0;
    for (int PP = 0; PP < numLimbs; ++PP)
    {
        frc         *= radix;
    }
    long double exp = 1.0;
    for (int EE = 0; EE < exponent; ++EE)
    {
        if (expPositive)    exp *= 2;
        else                exp /= 2;
    }

    long double X   = manPositive ? 1.0 : -1.0;
    X               *= integral;
    X               += ((long double)fractional) / frc;
    X               *= exp;

    *lfloat         = X;
    *fst            = cur;

    return 1;
}

int kvinParseStep(KVINParser* kp)
{
    if (!kp) return 0;
    if (!kp->beg) return 0;
    if (!kp->end) return 0;
    if (!kp->fst) return 0;
    if (!kp->lst) return 0;
    if (kp->fst > kp->beg) return 0;
    if (kp->beg >= kp->lst) return 0;

    kp->action[0]   = KVINA_NONE;
    kp->action[1]   = KVINA_NONE;
    kp->action[2]   = KVINA_NONE;

    if (!kvinEatWS(kp, &kp->end))
    {
        return 0;
    }
    kp->beg = kp->end;

    if (kp->end >= kp->lst)
    {
        kp->event   = KVINE_DONE;
        return 0;
    }

    int isVal       =   (kp->event == KVINE_VAL_C_ID)   ||
                        (kp->event == KVINE_VAL_NUMBER) ||
                        (kp->event == KVINE_VAL_FLOAT)  ||
                        (kp->event == KVINE_VAL_REF)    ||
                        (kp->event == KVINE_VAL_STRING) ||
                        0
                    ;
    int isAxis      =   (kp->event == KVINE_AXIS_INDEX)     ||
                        (kp->event == KVINE_AXIS_AUTOINDEX) ||
                        (kp->event == KVINE_AXIS_NAME)      ||
                        0
                    ;

    if (kvinCIdentifier(&kp->end, kp->lst))
    {
        if (kp->event == KVINE_INIT)
        {
            kp->event       = KVINE_AXIS_NAME;
            kp->action[0]   = KVINA_GET_ROOT;
            kp->action[1]   = KVINA_SET_AXIS;
            return 1;
        }
        else
        if ((kp->event == KVINE_PATH)   ||
            (kp->event == KVINE_PARENT) ||
            0)
        {
            kp->event       = KVINE_AXIS_NAME;
            kp->action[0]   = KVINA_SET_AXIS;
            return 1;
        }
        else
        if (isVal)
        {
            kp->event       = KVINE_AXIS_NAME;
            kp->action[0]   = KVINA_GET_ROOT;
            kp->action[1]   = KVINA_SET_AXIS;
            return 1;
        }
        else
        if ((kp->event == KVINE_ASSIGN) ||
            0)
        {
            kp->event       = KVINE_VAL_C_ID;
            kp->action[0]   = KVINA_SET_VALUE;
            return 1;
        }
        else
        {
            kp->event       = KVINE_ERROR;
            return 0;
        }
    }

    if (*kp->beg == '[')
    {
        ++kp->end;
        if (!kvinEatWS(kp, &kp->end))
        {
            return 0;
        }
        if (!kvinNumber(&kp->end, kp->lst, &kp->number, 0, 0))
        {
            kp->event   = KVINE_ERROR;
            return 0;
        }
        if (kp->end >= kp->lst)
        {
            kp->event   = KVINE_ERROR;
            return 0;
        }
        if (!kvinEatWS(kp, &kp->end))
        {
            return 0;
        }
        if (*kp->end != ']')
        {
            kp->event   = KVINE_ERROR;
            return 0;
        }
        ++kp->end;

        if (kp->event == KVINE_INIT)
        {
            kp->event       = KVINE_AXIS_INDEX;
            kp->action[0]   = KVINA_GET_ROOT;
            kp->action[1]   = KVINA_SET_AXIS;
            return 1;
        }
        else
        if ((kp->event == KVINE_PATH)   ||
            (kp->event == KVINE_PARENT) ||
            isAxis                      ||
            0)
        {
            kp->event       = KVINE_AXIS_INDEX;
            kp->action[0]   = KVINA_SET_OBJECT;
            kp->action[1]   = KVINA_SET_AXIS;
            return 1;
        }
        else
        {
            kp->event       = KVINE_ERROR;
            return 0;
        }
    }

    if (*kp->beg == '#')
    {
        ++kp->end;

        kp->number  = (unsigned long long)-1;

        if (kp->event == KVINE_INIT)
        {
            kp->event       = KVINE_AXIS_AUTOINDEX;
            kp->action[0]   = KVINA_GET_ROOT;
            kp->action[1]   = KVINA_SET_AXIS;
            return 1;
        }
        else
        if ((kp->event == KVINE_PATH)   ||
            (kp->event == KVINE_PARENT) ||
            isVal                       ||
            0)
        {
            kp->event       = KVINE_AXIS_AUTOINDEX;
            kp->action[0]   = KVINA_SET_AXIS;
            return 1;
        }
        else
        {
            kp->event       = KVINE_ERROR;
            return 0;
        }
    }

    KVIN_EVENT_TYPE GNumber = KVINE_ERROR;
    if (kvinLongFloat(&kp->end, kp->lst, &kp->lfloat))
    {
        GNumber             = KVINE_VAL_FLOAT;
    }
    else
    if (kvinNumber(&kp->end, kp->lst, &kp->number, 0, 0))
    {
        GNumber             = KVINE_VAL_NUMBER;
    }

    if ((GNumber == KVINE_VAL_NUMBER)   ||
        (GNumber == KVINE_VAL_FLOAT)    ||
        0)
    {
        if (kp->event == KVINE_INIT)
        {
            kp->event       = KVINE_AXIS_INDEX;
            kp->action[0]   = KVINA_GET_ROOT;
            kp->action[1]   = KVINA_SET_AXIS;
            return 1;
        }
        else
        if ((kp->event == KVINE_PATH)   ||
            (kp->event == KVINE_PARENT) ||
            isVal                       ||
            0)
        {
            kp->event       = KVINE_AXIS_INDEX;
            kp->action[0]   = KVINA_SET_AXIS;
            return 1;
        }
        else
        if ((kp->event == KVINE_ASSIGN) ||
            0)
        {
            kp->event       = GNumber;
            kp->action[0]   = KVINA_SET_VALUE;
            return 1;
        }
        else
        {
            kp->event       = KVINE_ERROR;
            return 0;
        }
    }

    if (*kp->beg == '.')
    {
        ++kp->end;
        if (isAxis)
        {
            kp->event       = KVINE_PATH;
            kp->action[0]   = KVINA_SET_OBJECT;
            kp->number      = (unsigned long long)-1;
            return 1;
        }
        else
        if (isVal)
        {
            kp->event       = KVINE_PATH;
            return 1;
        }
        else
        if (kp->event == KVINE_PATH)
        {
            kp->event       = KVINE_PARENT;
            kp->action[0]   = KVINA_GET_PARENT;
            return 1;
        }
        else
        if (kp->event == KVINE_PARENT)
        {
            kp->event       = KVINE_PARENT;
            kp->action[0]   = KVINA_GET_PARENT;
            return 1;
        }
        else
        {
            kp->event       = KVINE_ERROR;
            return 0;
        }
    }

    if (*kp->beg == '=')
    {
        if (!kvinEatWS(kp, &kp->end))
        {
            return 0;
        }
        ++kp->end;
        if (isAxis)
        {
            kp->event       = KVINE_ASSIGN;
            return 1;
        }
        else
        {
            kp->event       = KVINE_ERROR;
            return 0;
        }
    }

    if (*kp->beg == '"')
    {
        ++kp->end;
        for ( ; (kp->end < kp->lst) && (*kp->end != '"'); ++kp->end)
        {
            if ((*kp->end != '\\'))
            {
                continue;
            }
            ++kp->end;
            if (kp->end >= kp->lst)
            {
                kp->event   = KVINE_ERROR;
                return 0;
            }
        }

        if (kp->end >= kp->lst)
        {
            kp->event       = KVINE_ERROR;
            return 0;
        }
        if (*kp->end != '"')
        {
            kp->event       = KVINE_ERROR;
            return 0;
        }
        ++kp->end;

        if (kp->event == KVINE_ASSIGN)
        {
            kp->event       = KVINE_VAL_STRING;
            kp->action[0]   = KVINA_SET_VALUE;
            return 1;
        }
        else
        {
            kp->event       = KVINE_ERROR;
            return 0;
        }
    }

    if (*kp->beg == '`')
    {
        ++kp->end;
        if (kp->end >= kp->lst)
        {
            kp->event       = KVINE_ERROR;
            return 0;
        }

        do
        {
            if (!kvinNumber(&kp->end, kp->lst, &kp->number, 0, 0)   &&
                !kvinCIdentifier(&kp->end, kp->lst)                 &&
                1)
            {
                break;
            }
            if ((kp->end >= kp->lst) || (*kp->end != '.'))
            {
                break;
            }
            ++kp->end;
        }
        while (1);

        if ((kp->beg + 1) >= kp->end)
        {
            return 0;
        }

        if (kp->event == KVINE_ASSIGN)
        {
            kp->event       = KVINE_VAL_REF;
            kp->action[0]   = KVINA_SET_VALUE;
            return 1;
        }
        else
        {
            kp->event       = KVINE_ERROR;
            return 0;
        }
    }

    kp->event       = KVINE_INTERNAL_ERROR;
    kp->action[0]   = KVINA_NONE;
    kp->action[1]   = KVINA_NONE;
    kp->action[2]   = KVINA_NONE;
    return 0;
}

int kvinParseStepAct(KVINParser* kp, KVINActions* ka)
{
    if (!kp) return 0;
    if (!ka) return 0;
    if (!ka->GetRoot) return 0;
    if (!ka->GetParent) return 0;
    if (!ka->SetAxis) return 0;
    if (!ka->SetValue) return 0;

    if (!kvinParseStep(kp))
    {
        return 0;
    }

    for (int AA = 0; (AA < 3) && (kp->action[AA] != KVINA_NONE); ++AA)
    {
        switch (kp->action[AA])
        {
        case KVINA_NONE         : break;
        case KVINA_GET_ROOT     : ka->GetRoot(ka->handle, kp); break;
        case KVINA_GET_PARENT   : ka->GetParent(ka->handle, kp); break;
        case KVINA_SET_AXIS     : ka->SetAxis(ka->handle, kp); break;
        case KVINA_SET_OBJECT   : ka->SetObject(ka->handle, kp); break;
        case KVINA_SET_VALUE    : ka->SetValue(ka->handle, kp); break;
        case KVINA_MAX          : return 0;
        }
    }

    return 1;
}

int kvinParse(KVINParser* kp, KVINActions* ka)
{
    while (kvinParseStepAct(kp, ka))
    {
    }
    return 1;
}

#endif//THE_KVIN_KEY_VALUE_INLINE_NOTATION_C
