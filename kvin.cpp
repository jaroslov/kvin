#include "kvin.h"

#include <inttypes.h>
#include <stdio.h>
#include <stdlib.h>

#include <map>
#include <string>
#include <vector>

struct Kvin
{
    enum
    {
        eNone,
        eInt,
        eReal,
        eStr,
        eCID,
        eRef,
        eArray,
        eObject,
    } Type;
    Kvin                       *Parent;
    uint64_t                    Int;
    double                      Real;
    std::string                 Str;
    Kvin                       *Ref;
    std::vector<Kvin>           Array;
    std::map<std::string, Kvin> Object;
    Kvin()
    : Type(eNone)
    , Parent(0)
    , Int(0)
    , Real(0)
    , Str()
    , Ref(0)
    , Array()
    , Object()
    {
        this->Type      = eObject;
        this->Parent    = this;
    }
    ~ Kvin()
    {
    }
};

struct MOB
{
    Kvin*           root      = nullptr;
    Kvin*           current   = nullptr;
    bool            isCIDAxis;
    std::string     cAxis;
    uint64_t        iAxis;
};

#define AS_MOB(H) MOB& mob = *(MOB*)H

int ktGetRoot(void* handle, KVINParser*)
{
    AS_MOB(handle);
    fprintf(stdout, "%s\n", "GetRoot");
    mob.current = mob.root;
    return 1;
}

int ktGetParent(void* handle, KVINParser*)
{
    AS_MOB(handle);
    fprintf(stdout, "%s\n", "GetParent");
    mob.current  = mob.current->Parent;
    return 1;
}

int ktSetAxis(void* handle, KVINParser* P)
{
    AS_MOB(handle);
    fprintf(stdout, "%s : %.*s\n", "SetAxis", (int)(P->end - P->beg), P->beg);
    if (P->event == KVINE_AXIS_NAME)
    {
        mob.cAxis       = std::string(P->beg, P->end);
        mob.isCIDAxis   = false;
    }
    else
    if (P->event == KVINE_AXIS_INDEX)
    {
        mob.iAxis       = P->number;
        mob.isCIDAxis   = true;
    }
    return 1;
}

int ktSetObject(void* handle, KVINParser*)
{
    AS_MOB(handle);
    fprintf(stdout, "%s\n", "SetObject");
    Kvin* obj           = nullptr;
    if (mob.isCIDAxis)
    {
        if (mob.current->Array.size() <= mob.iAxis)
        {
            mob.current->Array.resize(mob.iAxis+1);
        }
        obj             = &mob.current->Array[mob.iAxis];
    }
    else
    {
        obj             = &mob.current->Object[mob.cAxis];
    }
    obj->Type           = mob.isCIDAxis ? Kvin::eArray : Kvin::eObject;
    obj->Parent         = mob.current;
    mob.current         = obj;
    return 1;
}

int ktSetValue(void* handle, KVINParser* P)
{
    AS_MOB(handle);
    fprintf(stdout, "%s\n", "SetValue");
    Kvin* val       = nullptr;
    if (mob.isCIDAxis)
    {
        val         = &mob.current->Array[mob.iAxis];
    }
    else
    {
        val         = &mob.current->Object[mob.cAxis];
    }
    switch (P->event)
    {
    case KVINE_VAL_C_ID     : val->Type   = Kvin::eCID; goto STRING_SET;
    case KVINE_VAL_STRING   : val->Type   = Kvin::eStr; goto STRING_SET;
    case KVINE_VAL_REF      : val->Type   = Kvin::eRef; goto STRING_SET;
    STRING_SET              :
        val->Str    = std::string(P->beg, P->end);
        break;
    case KVINE_VAL_NUMBER   :
        val->Type   = Kvin::eInt;
        val->Int    = P->number;
        break;
    case KVINE_VAL_FLOAT    :
        val->Type   = Kvin::eReal;
        val->Real   = P->lfloat;
        break;
    default                 : break;
    }
    return 1;
}

void Dump(Kvin const& node, std::string pre="")
{
    switch (node.Type)
    {
    case Kvin::eNone    :
        fprintf(stdout, "%s = null\n", pre.c_str());
        break;
    case Kvin::eInt     :
        fprintf(stdout, "%s = %lld\n", pre.c_str(), node.Int);
        break;
    case Kvin::eReal    :
        fprintf(stdout, "%s = %lf\n", pre.c_str(), node.Real);
        break;
    case Kvin::eStr     :
        fprintf(stdout, "%s = \"%s\"\n", pre.c_str(), node.Str.c_str());
        break;
    case Kvin::eCID     :
        fprintf(stdout, "%s = %s\n", pre.c_str(), node.Str.c_str());
        break;
    case Kvin::eRef     :
        fprintf(stdout, "%s = %s\n", pre.c_str(), node.Str.c_str());
        break;
    case Kvin::eArray   :
        {
            for (int AA = 0; AA < (int)node.Array.size(); ++AA)
            {
                std::string path    = pre;
                int len             = snprintf(0, 0, "[%d]", AA);
                char feh[len+1];
                sprintf(&feh[0], "[%d]", AA);
                path                += feh;
                Dump(node.Array[AA], path);
            }
        }
        break;
    case Kvin::eObject  :
        {
            for (auto const& item : node.Object)
            {
                std::string path    = pre;
                int len             = snprintf(0, 0, "%s", item.first.c_str());
                char feh[len+1];
                sprintf(&feh[0], "%s", item.first.c_str());
                if (!path.empty())
                {
                    path            += ".";
                }
                path                += feh;
                Dump(item.second, path);
            }
        }
        break;
    }
}

int main(int argc, char *argv[])
{
    Kvin Root       = { };
    MOB M           = { };
    M.root          = &Root;
    M.current       = &Root;
    KVINParser P    = { };
    KVINActions A   =
    {
        .handle     = (void*)&M,
        .GetRoot    = &ktGetRoot,
        .GetParent  = &ktGetParent,
        .SetAxis    = &ktSetAxis,
        .SetObject  = &ktSetObject,
        .SetValue   = &ktSetValue,
    };
    const char K[]  =
        "foo.bar    = 10\n"
        "   .baz    = 12\n"
        "   .quux.A = 1100\n"
        "        .B = 1003\n"
        "        .C = 1004\n"
        "  ..bloggl = 5001\n"
        //"[0] = 101\n"
        //"        [1] = 102\n"
        //"        [2] = 1020\n"
        //"        [3] = 112\n"
        ;
    if (!kvinInit(&P, K, K + sizeof(K)))
    {
        return 1;
    }
    kvinParse(&P, &A);

    Dump(Root);

    return 0;
}

#define THE_KVIN_KEY_VALUE_INLINE_NOTATION_C
#include "kvin.h"
