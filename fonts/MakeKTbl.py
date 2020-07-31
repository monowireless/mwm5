#!/usr/bin/env python3
# -*- coding: utf-8 -*-

#Copyright (C) 2020 Mono Wireless Inc. All Rights Reserved.
#Released under MW-OSSLA-1J,1E (MONO WIRELESS OPEN SOURCE SOFTWARE LICENSE AGREEMENT).
import sys
import os
import binascii

tblKanji = {}
fname_joyo = 'kanjitable_full.txt'

if True:
    for l in sys.stdin:
        for c in l:
            tblKanji[c] = 1

if True:
    fd = None
    try:
        fd = open(fname_joyo, 'r', encoding='utf-8')
    except:
        print("*** Can't open")
        sys.exit(1)

    for l in fd:
        try:
            if l[0] == '#': raise Exception

            # split into two
            f = l.split(" ")

            # at least two fields
            if len(f) != 4: raise Exception

            # make table
            c = f[0]

            if c in tblKanji:
                print(l, end='')
            else:
                pass
 
        except:
            print(l, end='')
            continue