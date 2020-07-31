#!/usr/bin/env python3
# -*- coding: utf-8 -*-

#Copyright (C) 2020 Mono Wireless Inc. All Rights Reserved.
#Released under MW-OSSLA-1J,1E (MONO WIRELESS OPEN SOURCE SOFTWARE LICENSE AGREEMENT).

# display used chars (>0xff) read from stdin.
import sys
import os
import binascii

tblKanji = {}

if True:
    for l in sys.stdin:
        for c in l:
            tblKanji[c] = 1

for e in sorted(tblKanji.keys()):
    code = ord(e)
    if not (   (code >= 0 and code <= 0xFF) 
            or (code >= 0xff61 and code <= 0xFFAF)):
        print(e)
