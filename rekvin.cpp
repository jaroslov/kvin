#include "rekvin.h"

#include <stdio.h>
#include <string.h>

const char* sPARSES[] =
{
#undef  KVIN_PARSES_ENTRY
#define KVIN_PARSES_ENTRY(NAME) # NAME,
KVIN_PARSES_TABLE(KVIN_PARSES_ENTRY)
#undef  KVIN_PARSES_ENTRY
};

const char* sACTION[] =
{
#undef  KVIN_ACTION_ENTRY
#define KVIN_ACTION_ENTRY(NAME) # NAME,
KVIN_ACTION_TABLE(KVIN_ACTION_ENTRY)
#undef  KVIN_ACTION_ENTRY
};

void printLexeme(KVINLexer* lexer)
{
    fprintf(stdout, "'%.*s'", (int)(lexer->lend - lexer->lbeg), lexer->lbeg);
}

typedef struct TEST
{
    KVIN_PARSES     state;
    const char*     kvin;
} TEST;

int main(int argc, char *argv[])
{
    static const TEST EXS[] =
    {
        { KVIN_PAR_DONE,    "bar        = \"baz\"" },

        { KVIN_PAR_DONE,    "foo        = 10" },

        { KVIN_PAR_DONE,    "foo.bar    = baz" },

        { KVIN_PAR_DONE,    "foo.bar    = baz\n"
                            "foo.baz    = 12" },

        { KVIN_PAR_DONE,    "foo.bar    = baz\n"
                            "foo.baz    = \"asd;lkjasdf;laksjdfas\\\"\"" },

        { KVIN_PAR_DONE,    "foo.bar    = 10\n"
                            "   .baz    = 12\n" },

        { KVIN_PAR_DONE,    "foo.10     = A\n"
                            "   .11     = B\n" },

        { KVIN_PAR_DONE,    "10         = A\n"
                            "11         = B" },

        { KVIN_PAR_DONE,    "10.10.99.101 = my_ip_address" },

        { KVIN_PAR_DONE,    "A = B\n"
                            "C = D\n"
                            "E = F\n"
                            "G = H\n" },

        { KVIN_PAR_DONE,    "fooddd.bar.baz.bob = 10\n"
                            "    ...quux        = 12\n"
                            "      .dob.baz.bob = 13\n"
                            "    ...quux2.feh   = 12\n" },

        { KVIN_PAR_DONE,    "foo.12 = A\n"
                            "   .#  = B\n"
                            "   .#  = C\n"
                            "   .#  = D\n" },

        { KVIN_PAR_DONE,    "0      = 10\n"
                            "[0]    = 10\n" },

        { KVIN_PAR_DONE,    "foo[3] = 10\n" },

        { KVIN_PAR_DONE,    "foo.bar        = 10\n"
                            "   .baz        = 12\n"
                            "   .quux.A     = 1100\n"
                            "        .B     = 1003\n"
                            "        .C     = 1004\n"
                            "  ..bloggl     = 5001\n"
                            "foo.zuul[0]    = 1000\n"
                            "       .[1]    = 102\n"
                            "       .[2]    = 1020\n"
                            "       .[3]    = 112\n"
                            "       .lives  = 3005\n" },

        { KVIN_PAR_ERROR,   "# = 10\n" },

        { KVIN_PAR_ERROR,   "## = 12\n" },

        { KVIN_PAR_ERROR,   "10 10 = 12\n" },

        { KVIN_PAR_ERROR,   "foo 10 = 12\n" },

        { KVIN_PAR_ERROR,   "bar =\n" },

        { KVIN_PAR_ERROR,   "bar = = 10\n" },

        { KVIN_PAR_ERROR,   "baz = 10 = 12\n" },

        { KVIN_PAR_ERROR,   "baz = 10 12 = bob\n" },

        { KVIN_PAR_DONE,    "foo = bar // bob\n"
                            "\n"
                            "// james\n"
                            "tim = jackson // hole\n" },
    };

    int numExpPassed    = 0;
    int numExpFailed    = 0;
    int numPassed       = 0;
    int numFailed       = 0;

    for (int EE = 0; EE < (int)sizeof(EXS)/sizeof(EXS[0]); ++EE)
    {
        KVINParser parser   = { };
        kvinInitParser(&parser, EXS[EE].kvin, EXS[EE].kvin + (int)strlen(EXS[EE].kvin));

        fprintf(stdout, "%s expects %s\n", EXS[EE].kvin, sPARSES[EXS[EE].state]);
        do
        {
            switch (parser.action)
            {
            case KVIN_ACT_SETATROOT     :
            case KVIN_ACT_SETNEXTAXIS   :
                fprintf(stdout, "    %s ", sACTION[parser.action]);
                if (parser.lexer.lex == KVIN_LEX_IDENTIFIER)
                {
                    fprintf(stdout, "%.*s\n", (int)(parser.value.end - parser.value.begin), parser.value.begin);
                }
                else
                {
                    fprintf(stdout, "%lld\n", parser.value.integer);
                }
                break;
            case KVIN_ACT_AUTONUMBER    :
            case KVIN_ACT_RELPATH       :
                fprintf(stdout, "    %s\n", sACTION[parser.action]);
                break;
            case KVIN_ACT_SETVALUE      :
                fprintf(stdout, "    %s ", sACTION[parser.action]);
                switch (parser.lexer.lex)
                {
                case KVIN_LEX_IDENTIFIER    : fprintf(stdout, "%.*s\n", (int)(parser.value.end - parser.value.begin), parser.value.begin); break;
                case KVIN_LEX_NUMBERLIKE    : fprintf(stdout, "%lld\n", parser.value.integer); break;
                case KVIN_LEX_BSTRING       : fprintf(stdout, "'%.*s'\n", (int)(parser.value.end - parser.value.begin), parser.value.begin); break;
                default                     : fprintf(stdout, "??%s\n", "?");
                }
            case KVIN_ACT_NONE          : break;
            default                     : break;
            }
        }
        while (kvinParseNext(&parser));
        fprintf(stdout, "    %s\n", sPARSES[parser.state]);
        fprintf(stdout, "    %s\n", (parser.state == EXS[EE].state) ? "passed" : "failed");
        numExpPassed    += !!(EXS[EE].state == KVIN_PAR_DONE);
        numExpFailed    += !!(EXS[EE].state == KVIN_PAR_ERROR);
        numPassed       += !!(parser.state == EXS[EE].state);
        numFailed       += !!(parser.state != EXS[EE].state);
    }

    fprintf(stdout,
        "ROLLUP: exp. passed exp. failed passed failed\n"
        "        %-11d %-11d %-6d %-6d\n",
        numExpPassed, numExpFailed, numPassed, numFailed);

    return !!!numFailed;
}

#define REKVIN_C
#include "rekvin.h"