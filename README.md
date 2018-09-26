# kvin
Key Value Inline Notation

KVIN is a structured data format. The grammar is:

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