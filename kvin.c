#include "kvin.h"

#include <assert.h>
#include <stdio.h>
#include <string.h>

static const char* KVINE_NAMES[] =
{
#undef  KVIN_EVENT_TYPE_X
#define KVIN_EVENT_TYPE_X(NAME) #NAME,
KVIN_EVENT_TYPE_TABLE(KVIN_EVENT_TYPE_X)
#undef  KVIN_EVENT_TYPE_X
};

static const char* KVINA_NAMES[] =
{
#undef  KVIN_ACTION_TYPE_X
#define KVIN_ACTION_TYPE_X(NAME) #NAME,
KVIN_ACTION_TYPE_TABLE(KVIN_ACTION_TYPE_X)
#undef  KVIN_ACTION_TYPE_X
};

typedef struct TEST
{
    int         result;
    const char* test;
} TEST;

typedef struct CStr
{
    const char* beg;
    const char* end;
} CStr;

typedef struct MockPath
{
    int         isIndex;
    union
    {
        CStr    name;
        int     index;
    };
} MockPath;

typedef struct MockObject
{
    int             depth;
    int             autoindex[128];
    MockPath        path[128];
} MockObject;

void printMockObject(MockObject* mobj)
{
    for (int DD = 0; DD < mobj->depth; ++DD)
    {
        if (mobj->path[DD].isIndex)
        {
            fprintf(stdout, "%s%d", DD ? "." : "", mobj->path[DD].index);
        }
        else
        {
            fprintf(stdout, "%s%.*s", DD ? "." : "", (int)(mobj->path[DD].name.end - mobj->path[DD].name.beg), mobj->path[DD].name.beg);
        }
    }
}

int testGetRoot(void* handle, KVINParser* kp)
{
    MockObject *mob = (MockObject*)handle;
    mob->depth      = 0;
    for (int DD = 0; DD < 128; ++DD)
    {
        mob->autoindex[DD] = 0;
    }
    return 1;
}

int testGetParent(void* handle, KVINParser* kp)
{
    MockObject *mob = (MockObject*)handle;
    --mob->depth;
    for (int DD = mob->depth+1; DD < 128; ++DD)
    {
        mob->autoindex[DD]  = 0;
    }
    return 1;
}

int testSetAxis(void* handle, KVINParser* kp)
{
    MockObject *mob = (MockObject*)handle;
    if (kp->event == KVINE_AXIS_INDEX)
    {
        mob->path[mob->depth].isIndex   = 1;
        mob->path[mob->depth].index     = kp->number;
    }
    else
    if (kp->event == KVINE_AXIS_AUTOINDEX)
    {
        mob->path[mob->depth].isIndex   = 1;
        mob->path[mob->depth].index     = mob->autoindex[mob->depth];
        ++mob->autoindex[mob->depth];
    }
    else
    {
        mob->path[mob->depth].isIndex   = 0;
        mob->path[mob->depth].name.beg  = kp->beg;
        mob->path[mob->depth].name.end  = kp->end;
    }
    return 1;
}

int testSetObject(void* handle, KVINParser* kp)
{
    MockObject *mob = (MockObject*)handle;
    ++mob->depth;
    return 1;
}

int testSetValue(void* handle, KVINParser* kp)
{
    MockObject *mob = (MockObject*)handle;
    ++mob->depth;
    fprintf(stdout, ">>> %s", "");
    printMockObject(mob);
    if (kp->event == KVINE_VAL_C_ID)
    {
        fprintf(stdout, " = %.*s    // done.\n", (int)(kp->end - kp->beg), kp->beg);
    }
    else
    if (kp->event == KVINE_VAL_NUMBER)
    {
        fprintf(stdout, " = %llu    // done.\n", (unsigned long long)kp->number);
    }
    if (kp->event == KVINE_VAL_FLOAT)
    {
        fprintf(stdout, " = %Lf    // done.\n", (long double)kp->lfloat);
    }
    else
    if (kp->event == KVINE_VAL_REF)
    {
        fprintf(stdout, " = `%.*s    // done.\n", (int)(kp->end - kp->beg - 1), kp->beg+1);
    }
    else
    if (kp->event == KVINE_VAL_STRING)
    {
        fprintf(stdout, " = \"%.*s\"    // done.\n", (int)(kp->end - kp->beg - 2), kp->beg + 1);
    }
    --mob->depth;
    return 1;
}

int main(int argc, char *argv[])
{
    const TEST TESTS[] =
    {
        {
            1,
            "f = null\n"
            "g = `f",
        },
        {
            1, // XXX: BROKEN!
            "f.g = null\n"
            "f.h = `f.g",
        },
        {
            1,
            "0.0 = null\n"
            " .1 = `0.0\n",
        },
        {
            1,
            "10.0.1 = 10",
        },
        {
            1,
            "james = 0x3",
        },
        {
            1,
            "james[0] = 0x3",
        },
        {
            1,
            "[0] = james",
        },
        {
            1,
            "frank = \"hoo doo foo\"",
        },
        {
            1,
            "frank = \"hoo doo\\\" foo\"",
        },
        {
            1,
            "bob.james = hammer\n"
            "   .frank = michael",
        },
        {
            1,
            "bob = marley",
        },
        {
            1,
            "bob.james.frank = marley",
        },
        {
            1,
            "alex[10] = james",
        },
        {
            1,
            "bob = james // flubber!",
        },
        {
            1,
            "b = j //f\n"
            "f = j //f\n",
        },
        {
            1,
            "b = j //f\n"
            "f = j //f",
        },
        {
            1,
            "a = 0\n"
            "b = 10\n"
            "c = 12\n",
        },
        {
            1,
            "n0.n1       = A\n"
            "  .n2.n3.n4 = B\n"
            "    ..n5    = C\n",
        },
        {
            1,
            "# = adam\n"
            "# = bob\n"
            "# = casey\n"
            "# = don\n"
        },
        {
            1,
            "a.# = adam\n"
            " .# = bob\n"
            " .# = casey\n"
            " .# = don\n"
            "b.a.c.# = james\n"
            "   ...d.# = john\n"
            "    ..e.# = doe\n"
            "       .# = jane\n"
        },
        {
            0,
            "long = 0.0\n"
            "long = 1.0\n"
            "long = 2.35\n"
            "long = -2.35\n"
            "long = 0x2.8\n"
            "long = 0x2.8p-2\n"
        }
    };

    int count       = 50000000;
    if (count > sizeof(TESTS) / sizeof(TESTS[0]))
    {
        count       = sizeof(TESTS) / sizeof(TESTS[0]);
    }

    for (int EE = 0; EE < count; ++EE)
    {
        const char* fst = TESTS[EE].test;
        const char* lst = TESTS[EE].test + strlen(TESTS[EE].test);

        MockObject mob  = { };
        KVINParser P    = { };
        KVINActions A   =
        {
            .handle     = &mob,
            .GetRoot    = &testGetRoot,
            .GetParent  = &testGetParent,
            .SetAxis    = &testSetAxis,
            .SetObject  = &testSetObject,
            .SetValue   = &testSetValue,
        };
        fprintf(stdout, "---- %4d ----\n%s\n", EE, fst);
        if (!kvinInit(&P, fst, lst))
        {
            return 1;
        }
        while (kvinParseStepAct(&P, &A))
        {
            if (P.event == KVINE_AXIS_INDEX)
            {
                fprintf(stdout, "[%3d:%-15s] 0x%llX\n", P.event, KVINE_NAMES[P.event], (unsigned long long)P.number);
            }
            else
            {
                fprintf(stdout, "[%3d:%-15s] %.*s\n", P.event, KVINE_NAMES[P.event], (int)(P.end - P.beg), P.beg);
            }
            for (int AA = 0; (AA < 3) && (P.action[AA] != KVINA_NONE); ++AA)
            {
                fprintf(stdout, "    %-15s\n", KVINA_NAMES[P.action[AA]]);
            }
        }
        fprintf(stdout, "(%3d:%-15s)\n\n", P.event, KVINE_NAMES[P.event]);
        if ((P.event != KVINE_DONE) == !!TESTS[EE].result)
        {
            fprintf(stdout, "ERROR!\n%s", "");
            return 1;
        }
    }

    return 0;
}

#define THE_KVIN_KEY_VALUE_INLINE_NOTATION_C
#include "kvin.h"
