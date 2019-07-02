#!/usr/bin/env python
#
# Created by Jacob N. Smith on 2 July 2019.
#
# KVIN is the older, Firebird-driving brother of JSON.
#

from __future__ import absolute_import, division, print_function

#
# Grammar:
#
#   PATH    ::= ABSPATH | RELPATH
#   ABSPATH ::= AXIS (`.` AXIS)* `=` VALUE
#   RELPATH ::= `.` ABSPATH
#   AXIS    ::= c-identifier | `[` number `]`
#   VALUE   ::= c-identifier | number | ``` PATH | STRING
#

import functools
import json
import operator
import re
import sys

axisre  = re.compile(r'([0-9]+)|([a-zA-Z_][a-zA-Z_0-9]*)')
valuere = re.compile(r'(?P<num>[0-9]+)|(?P<cid>[a-zA-Z_][a-zA-Z_0-9]*)|("(?P<str>[^"]*)")|(`(?P<sym>[a-zA-Z_][a-zA-Z0-9_]*))')

def JoinPath(old, new):
    if old is None:
        old     = [ ]
    elif new[0] is not None:
        old     = [ ]
    combined    = old + new
    npath       = [ ]
    for ndx in xrange(len(combined)):
        if combined[ndx] is None:
            if len(npath) > 0:
                npath.pop(-1)
        else:
            npath.append(combined[ndx])
    return npath

def SetValueInPath(tree, path, value, parent=None):
    if len(path) > 0:
        if type(tree) is not dict:
            tree                = { }
        if not tree.has_key(path[0]):
            tree[path[0]]       = { }
        SetValueInPath(tree[path[0]], path[1:], value, (path[0],tree))
    else:
        parent[1][parent[0]]    = value

def InferArrays(tree):
    if type(tree) is not dict:
        return tree
    keys                = tree.keys()
    if functools.reduce(operator.and_, [type(key) is int or type(key) is long for key in keys]):
        array           = [None for x in xrange(max(keys)+1)]
        for key in keys:
            array[key]  = tree[key]
        tree            = array
    for key in keys:
        tree[key]       = InferArrays(tree[key])
    return tree

lastPath                = None
root                    = { }
with open(sys.argv[1]) as kvinfp:
    for line in kvinfp.readlines():
        if line.startswith('#'):
            continue
        lhs,rhs         = [x.strip() for x in line.split('=')]
        m               = valuere.match(rhs)
        assert m is not None
        g               = m.groupdict()
        if g['num'] is not None:
            rhs         = int(g['num'])
        elif g['cid'] is not None:
            pass
        elif g['sym'] is not None:
            pass
        elif g['str'] is not None:
            rhs         = rhs.strip('"')
        lhs             = lhs.replace('[', '.')
        lhs             = lhs.replace(']', '')
        path            = lhs.split('.')
        for ndx in xrange(len(path)):
            if len(path[ndx]) == 0:
                path[ndx]   = None
                continue
            m               = axisre.match(path[ndx])
            if m.group(1):
                path[ndx]   = int(path[ndx])
        path            = JoinPath(lastPath, path)
        SetValueInPath(root, path, rhs)
        lastPath        = path

root                    = InferArrays(root)

print(json.dumps(root, indent=2))