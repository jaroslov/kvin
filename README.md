# kvin
Key Value Inline Notation

KVIN is a structured data format. The main goal of KVIN is to make sure that there is a 1:1 mapping between KVIN object keys (and, to a lesser extent, values) and the representation of those keys in C, as legal C code. For instance, in JSON, the key "foo bar" is legal, but if represented as a member of a `struct` in C requires a name-mangling phase. Name-mangling is hard, so KVIN is designed to prevent it. (This is side-stepping the entire question of "why not use a string-keyed hash table": I don't want to!)

KVIN's most obvious ancestor is TOML. There are a few, major, differences between KVIN and TOML that make them (fundamentally) incompatible. Briefly:

1. KVIN doesn't support the `[...]` notation for groups;
2. KVIN doesn't allow arbitrary strings for keys --- only valid c-identifiers and integers; and,
3. KVIN only allows a restricted subset of values (integers, reals, c-identifiers, and byte-strings).

The grammar of KVIN is roughly:

         TOPLEVEL ::= KEYVALS
          KEYVALS ::= KEYVALS KEYVAL
                   |  KEYVAL
           KEYVAL ::= PATH `=` VALUE `\n`
             PATH ::= PATH `.` HAXIS
                   |  AXIS
            HAXIS ::= `#`
                   |  AXIS
             AXIS ::= c-identifier
                   |  c-unsigned-long-long
                   |  c-long-double
            VALUE ::= c-identifier
                   |  c-unsigned-long-long
                   |  c-long-double
                   |  BSTRING
          BSTRING ::= `"` ESCAPED-CHARS `"`
    ESCAPED-CHARS ::= escape-with-`\`

An example would be:

    foo.bar = 1200
       .baz = foo_bar

KVIN supports both absolute paths (those that use the root object for the source of the path),
and relative paths, where the path is relative to the immediately previous value. Multiple `.`
can be used in sequence to go 'up' the object tree additional levels:

    foo.bar.baz.quux    = 10
          ..feh.quux    = 12

KVIN supports autonumbering on relative paths in order to make it easier to define arrays by index:

    foo.0       = 10
       .#       = 12
       .#       = 20
       .#       = 100