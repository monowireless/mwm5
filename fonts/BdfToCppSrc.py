#!/usr/bin/env python3
# -*- coding: utf-8 -*-

#Copyright (C) 2020 Mono Wireless Inc. All Rights Reserved.
#Released under MW-OSSLA-1J,1E (MONO WIRELESS OPEN SOURCE SOFTWARE LICENSE AGREEMENT).

import sys
import os
import binascii
import pathlib

import configparser
import argparse

## Copyright
monow_copyright = """/* Copyright (C) 2019-2020 Mono Wireless Inc. All Rights Reserved.
 * Released under MW-OSSLA-1J,1E (MONO WIRELESS OPEN SOURCE SOFTWARE LICENSE AGREEMENT). */"""
 
## 引数パーサー
parser = argparse.ArgumentParser()
parser.add_argument('--config', default='shinonome12/config.ini')
args = parser.parse_args()

## ini ファイル
ini = configparser.ConfigParser()
ini.read(args.config)
ini_dir = pathlib.Path(args.config).parent
if ini_dir == '': ini_dir = '.'

## work dirの移動
try:
	wdir = ini['common']['work_dir']
	if wdir != '': os.chdir(wdir)
except:
	print("cannot open dir %s" % wdir)
	sys.exit(1)

## 出力先
ini_outdir =  ini['common']['out_dir']
## ini の設定を変数に
fbase = ini['common']['fbase']
ini_desc = ini['common']['desc']

ini_jis_to_ucs = ini['common']['jis_to_ucs']

# BDF ファイルは ini の場所を起点に検索
ini_font_kanji = "%s/%s" %(ini_dir, ini['common']['font_kanji'])
ini_font_latin = "%s/%s" %(ini_dir, ini['common']['font_latin'])
ini_font_x201 = "%s/%s" %(ini_dir, ini['common']['font_x201'])
ini_font_g0 = "%s/%s" %(ini_dir, ini['common']['font_g0'])
ini_font_gaiji = "%s/%s" %(ini_dir, ini['common']['font_gaiji'])

ini_w = int(ini['common']['width'])
ini_wd = ini_w * 2
ini_h = int(ini['common']['height'])

ini_w_data = int(ini['common']['dwidth'])
ini_h_data = int(ini['common']['dheight'])

# license 
ini_license_file = "%s/%s" %(ini_dir, ini['common']['license'])

# 非サポート文字
ini_unsupported = ini['common']['unsupported']

# 漢字テーブル
ini_ktable_std = ini['kanjitable']['joyo']
ini_ktable_mini = ini['kanjitable']['mini']

## 常用テーブルによる間引きを行うかどうか
fJoyo = True


##########################################################
# output license file
##########################################################
def wrt_license(outfile):
	fd_lic = open(ini_license_file, 'r', encoding='utf-8')
	for l in fd_lic: outfile.write(l)
	outfile.write("\n")
	fd_lic.close()

##########################################################
# output file name
##########################################################
fd_cpp = open("%s/lcd_font_%s.cpp" % (ini_outdir, fbase), 'w', encoding='utf-8')

fd_cpp.write(monow_copyright)
fd_cpp.write("""
#include "twe_common.hpp"
#include "twe_utils.hpp"
#include "twe_font.hpp"
#include "lcd_font_%s.h"

namespace TWEFONT {
""" % fbase)


##########################################################
# フォントデータのチェック
##########################################################
def checkFontData(fontList, h):
	if len(fontList) <= h:
		return fontList

	# M+ FONT向け (dotに対して１ライン分余分にデータが有る)
	if fontList[h] != 0x0000:
		if fontList[0] == 0:
			fontList.pop(0) # 一番上のラインがなにもない
		else:
			fontList.pop(int(h/2)+1) # 中心から１行ずれたところを削除
	else:
		fontList.pop(h) # 末尾業を削除
	
	return fontList


##########################################################
# convert single widht BDF files.
##########################################################
def wrt_single(outfile, fontCode, fontPat, h, dispCodeOffset):
	if len(fontPat) < h:
		fontPat = fontPat + [0]*h

	outfile[0].write("    ")	
	for i in range(0,h):
		outfile[0].write("0x%02X, " % fontPat[i])
	
	c = chr(fontCode+dispCodeOffset)
	if not c.isprintable() or c == chr(0xFF60): c = chr(0x303F) 
	outfile[0].write("//%c U+%02X\n" % (c, fontCode+dispCodeOffset))

def process_single(fileName, outfile, name, code_range, w, h, dispCodeOffset=0):
	fontCode = -1
	fontList = []
	fontLine = -1
	iwrt = code_range.start

	fontFile = open(fileName, 'r', encoding='euc-jp')
	
	outfile[0].write("\tconst uint8_t font_%s[%d] = {\n" % (name, h*len(code_range)))
	outfile[1].write("\textern const uint8_t font_%s[%d];\n" % (name, h*len(code_range)))
	
	for line in fontFile:
		line = line.rstrip()
		fld = line.split(' ')

		if fld[0] == "STARTCHAR":
			fontCode = int(fld[1], 16)

			while iwrt < fontCode and iwrt in code_range:
				wrt_single(outfile, iwrt, [0] * h, h, dispCodeOffset)
				iwrt = iwrt + 1

		if fld[0] == "BITMAP":
			fontLine = 0
			fontList = []
			continue

		if fld[0] == "ENDCHAR": # reach bitmap data end
			if fontCode in code_range:
				fontList = checkFontData(fontList, h)
				wrt_single(outfile, fontCode, fontList, h, dispCodeOffset)
				iwrt = fontCode + 1	

			fontCode = -1
			fontList = []
			fontLine = -1
			continue

		if fontLine != -1:
			if fld[0][0] == '.' or fld[0][0] == '@': # shinonome bit files stlye (...@..@..)
				font_w = len(fld[0])
				bitlen = int((font_w + 7) / 8) * 8

				b = 1 << (bitlen - 1)
				val = 0

				for i  in range(0, font_w):
					c = fld[0][i]
					if c == '@':
						val = val + b

					b = b >> 1
				
				fontList.append(val)
				fontLine = fontLine + 1
			else:
				fontList.append(int(fld[0], 16))
				fontLine = fontLine + 1
	
	while iwrt < code_range.stop and iwrt in code_range:
		wrt_single(outfile, iwrt, [0] * h, h, dispCodeOffset)
		iwrt = iwrt + 1

	outfile[0].write("  };\n")
	
### output tables
if True:

	fd_src = open("%s/%sr.src" % (ini_outdir, fbase), 'w', encoding='utf-8')
	wrt_license(fd_src)
	fd_src.write("namespace TWEFONT {\n")

	# unsupported
	fd_src.write("\tconst uint8_t font_%sk_unsupported[%d*2] = {\n\t\t%s\n\t};\n\n" % (fbase, ini_h, ini_unsupported))
	fd_cpp.write("\textern const uint8_t font_%sk_unsupported[%d*2];\n" % (fbase, ini_h))
	
	# g0
	process_single(ini_font_g0, (fd_src, fd_cpp), "%s%s"%(fbase, 'r'), range(0x00, 0x80), ini_w, ini_h)
	# latin1 ex
	process_single(ini_font_latin, (fd_src, fd_cpp), "%s%s"%(fbase, 'r_latin1ex'), range(0xA0, 0x100), ini_w, ini_h)
	# jis x201
	process_single(ini_font_x201, (fd_src, fd_cpp), "%s%s"%(fbase, 'r_jisx201'), range(0xA0, 0xE0), ini_w, ini_h, dispCodeOffset=0xFF60-0xA0)

	fd_cpp.write("\n\n")

	fd_src.write("}\n")

##########################################################
# prepare JIS to UCS table
##########################################################
tblJisToUcs = [0] * 65536
if True:
	jisToUcsFile = None

	try:
		jisToUcsFile = open(ini_jis_to_ucs, 'r', encoding='utf-8')
	except:
		print("*** Can't open %s file" % ini_jis_to_ucs)
		sys.exit(1)

	for line in jisToUcsFile:
		if line[0] == '#': continue # comment line

		# split into two
		flds = line.split(" ")

		# at least two fields
		if len(flds) != 2: continue

		# make table
		try:
			j = int(flds[0],16)
			u = int(flds[1],16)

			tblJisToUcs[j] = u
		except:
			print("error: %s" % line)
			continue

##########################################################
# convert double bytes char.
##########################################################

strCreateFont = """
	const FontDef& createFont#FBASE##FSUFF#(uint8_t id, uint8_t line_space, uint8_t char_space, uint32_t opt) {
		auto font = _queryFont(id);

		if (font != nullptr) {
			font->font_code = id;
			font->font_name = "#DESC# (#FBASE##FSUFF#, #CHARCOUNT#)";

			font->width = #WIDTH#;
			font->height = #HEIGHT#;

			font->data_cols = #D_WIDTH#;
			font->data_rows = #D_HEIGHT#;

			font->h_space = line_space;
			font->w_space = char_space;

			font->font_latin1 = font_#FBASE#r;
			font->font_jisx201 = font_#FBASE#r_jisx201;
			font->font_latin1_ex = font_#FBASE#r_latin1ex;

			font->font_wide = font_#FBASE#k#FSUFF#_data;		// WIDE FONT DATA 
			font->font_wide_idx = font_#FBASE#k#FSUFF#_idx;	// UNICODE index 
			font->font_wide_missing = font_#FBASE#k_unsupported;

			font->font_wide_count = #CHARCOUNT#;

			font->opt = opt;
			return *font;
		}
		else {
			return queryFont(0);  // returns default font
		}
	}
"""

def process_double_body(fnameFont, fnameJoyo, outfile, name, w_, h_, excl_range=[], fnameGaiji=None):
	fontCode = -1
	fontList = []
	fontLine = -1

	name_full = "%sk%s" % (name[0], name[1])

	### OPEN 常用テーブル
	fJoyo = False
	joyoTable = {}

	if fnameJoyo is not None:
		joyoFile = None
		try:
			joyoFile = open(fnameJoyo, 'r', encoding='utf-8')
		except:
			print("*** Can't open %s file" % fnameJoyo)
			sys.exit(1)

		for line in joyoFile:
			flds = line.split(' ')
			try:
				if flds[0][0] == '#':
					continue

				euc_code = int(flds[2], 16)
				joyoTable[euc_code] = 1
			except:
				pass

		joyoFile.close()
		fJoyo = True

	### OPEN BDF file
	fontCode = -1
	fontList = []
	fontLine = -1

    # prepare A0A0-A0 (process as EUC)
	fontTbl = []
	for f in range(0, 0x60):
		fontTbl.append([])
		for s in range(0, 0x60):
			fontTbl[f].append([])
	
	# gaijiTbl
	gaijiTbl = {}

	# font/gaiji file open.
	fontFile = None
	gaijiFile = None
	try:
		fontFile = open(fnameFont, "r", encoding='euc-jp')
	except:
		print("*** Can't open %s file" % fnameFont)
		sys.exit(1)

	if fnameGaiji is not None:
		try:
			gaijiFile = open(fnameGaiji, "r", encoding='utf-8')
		except:	
			print("*** Can't open %s file" % fnameGaiji)
			sys.exit(1)
	
	for fd in (fontFile, gaijiFile):
		if fd is None: continue
		for line in fd:
			line = line.rstrip()
			fld = line.split(' ')
		
			if fld[0] == "STARTCHAR":
				fontCode = int(fld[1], 16)

			if fld[0] == "BITMAP":
				fontLine = 0
				fontList = []
				continue

			if fld[0] == "ENDCHAR": # reach bitmap data end
				fontList = checkFontData(fontList, h_)

				if fontCode <= 0xFFFF:
					fcH = fontCode >> 8
					fcL = fontCode & 0xFF

					fcH = fcH - 0x20
					fcL = fcL - 0x20

					try:
						fontTbl[fcH][fcL] = fontList
					except:
						print("index out of range: %04x" % fontCode)
				else:
					# GAIJI
					gaijiTbl[fontCode - 0x10000] = fontList

				# next char
				fontCode = -1
				fontLine = -1
				fontList = []
				continue

			if fontLine != -1:
				if fld[0][0] == '.' or fld[0][0] == '@': # shinonome bit files stlye (...@..@..)
					font_w = len(fld[0])
					bitlen = int((font_w + 7) / 8) * 8

					b = 1 << (bitlen - 1)
					val = 0

					for i  in range(0, font_w):
						c = fld[0][i]
						if c == '@':
							val = val + b

						b = b >> 1
					
					fontList.append(val)
					fontLine = fontLine + 1
				else:
					fontList.append(int(fld[0], 16))
					fontLine = fontLine + 1
		
	if True: # save as char list
		outDict = {}
		unitoecuDict = {}

		for f in range(0, 0x60):
			for s in range(0, 0x60):
				l = fontTbl[f][s]
				if len(l) > 0:
					char_euc_code = ((f + 0xA0) << 8) | (s + 0xA0)
					char_jis_code = ((f + 0x20) << 8) | (s + 0x20)
					char_uni_code = tblJisToUcs[char_jis_code] # テーブルからの参照

					bAccept = False

					if char_uni_code <= 0xFF: # 0x20-0xFF は別のテーブルから。
						bAccept = False
					elif char_uni_code in excl_range: # 除外リスト
						bAccept = False
					elif char_euc_code < 0xA900: # 記号
						bAccept = True
					elif not fJoyo: # 常用テーブルなし（全部入れる）
						bAccept = True
					elif fJoyo and char_euc_code in joyoTable: # 常用テーブルの漢字
						bAccept = True

					if bAccept:
						outDict[char_uni_code] = l
						unitoecuDict[char_uni_code] = char_euc_code

		# add gaiji
		outDict.update(gaijiTbl)
	
		### output
		outDictSorted = sorted(outDict.items(), key=lambda x:x[0])
		numDict = len(outDictSorted)

		# header
		outfile[1].write(
"""	/**********************************************************
	 * createFont%s%s [chrs = %d]
	 **********************************************************/
""" % (name[0], name[1], numDict))
		
		# index table
		outfile[0].write("\n\t// index table (%s, %dchrs, %dKB)\n" % (name_full, numDict, numDict * 2 / 1024))
		outfile[0].write("\tconst uint16_t font_%s_idx[%d] = {" % (name_full, numDict))
		outfile[1].write("\textern const uint16_t font_%s_idx[%d];\n" % (name_full, numDict))
		iv = 0
		for v in outDictSorted:
			if (iv % 16) == 0:
				outfile[0].write("\n\t\t0x%04x," % v[0])
			else:
				outfile[0].write(" 0x%04x," % v[0])
			iv = iv + 1

		outfile[0].write("\n\t};\n")
		
		# font data
		outfile[0].write("\n\t// font data (%s, %dchrs, %dKB)\n" % (name_full, numDict, (numDict * ini_h * 2)/1024))
		outfile[0].write("\tconst uint8_t font_%s_data[%d*%d*2] = {\n" % (name_full, numDict, ini_h))
		outfile[1].write("\textern const uint8_t font_%s_data[%d*%d*2];\n" % (name_full, numDict, ini_h))
		iv = 0
		for v in outDictSorted:
			iw = 0
			for w in v[1]:
				if iw == 0:
					outfile[0].write("\t\t0x%02x, 0x%02x," % (w >> 8, w & 0xff))
				elif iw == len(v[1]) - 1:
					c = chr(v[0])
					if not c.isprintable(): c = chr(0x303F)
					code_euc = 0
					if v[0] in unitoecuDict: code_euc =  unitoecuDict[v[0]] - 0x8080
					outfile[0].write(" 0x%02x, 0x%02x, //%c %04X/U+%04X #%d\n" % (w >> 8, w & 0xff, c, code_euc, v[0], iv))
				else:
					outfile[0].write(" 0x%02x, 0x%02x," % (w >> 8, w & 0xff))
				iw = iw + 1
			iv = iv + 1
		outfile[0].write("\t};\n")

		# generating function.
		s = strCreateFont
		s = s.replace('#FBASE#', name[0])
		s = s.replace('#DESC#', ini_desc)
		s = s.replace('#WIDTH#', "%d"%w_)
		s = s.replace('#HEIGHT#', "%d"%h_)
		s = s.replace('#D_WIDTH#', "%d"%w_)
		s = s.replace('#D_HEIGHT#', "%d"%h_)
		s = s.replace('#CHARCOUNT#', "%d"%numDict)
		s = s.replace('#FSUFF#', name[1])

		outfile[1].write(s)
		outfile[1].write("\n\n")

lstsrc = []
def process_double(fnameFont, fnameJoyo, cpp_file, name, w_, h_, excl_range=[], fnameGaiji=None):
	global lstsrc
	bname = "%sk%s"%(name[0],name[1])
	fd_src = open("%s/%s.src"%(ini_outdir,bname), 'w', encoding='utf-8')
	wrt_license(fd_src)
	fd_src.write("namespace TWEFONT {\n")
	process_double_body(fnameFont, fnameJoyo, (fd_src, cpp_file), name, w_, h_, excl_range=excl_range, fnameGaiji=fnameGaiji)
	fd_src.write("}\n")
	fd_src.close()

	lstsrc.append((name[0],name[1]))

process_double(ini_font_kanji, ini_ktable_mini, fd_cpp, (fbase,'_mini'), ini_w, ini_h, excl_range=range(0x391,0x451+1), fnameGaiji=ini_font_gaiji)
process_double(ini_font_kanji, ini_ktable_std, fd_cpp, (fbase,'_std'), ini_w, ini_h, fnameGaiji=ini_font_gaiji)
process_double(ini_font_kanji, None, fd_cpp, (fbase,'_full'), ini_w, ini_h, fnameGaiji=ini_font_gaiji)

# close cpp file
fd_cpp.write("}\n\n")
fd_cpp.write("""#include "%sr.src"\n""" % fbase)
for l in lstsrc:
	fd_cpp.write("""#include "%sk%s.src"\n""" % (l[0], l[1]))
fd_cpp.close()

### h file
fd_h = open("%s/lcd_font_%s.h" % (ini_outdir, fbase), 'w', encoding='utf-8')
fd_h.write(monow_copyright)
fd_h.write("""
#pragma once 

#include "twe_common.hpp"
#include "twe_font.hpp"

namespace TWEFONT {
""")
# each method `createFont{FONTNAME}_{SET}'
for l in lstsrc:
	fd_h.write("""\tconst FontDef& createFont%s%s(uint8_t id, uint8_t line_space = 0, uint8_t char_space = 0, uint32_t u32Opt = 0);\n""" % (l[0],l[1]))
# alias to default method `createFont{FONTNAME}'
fd_h.write("""\tstatic inline const FontDef& createFont%s(uint8_t id, uint8_t line_space = 0, uint8_t char_space = 0, uint32_t u32Opt = 0) {\n""" % (lstsrc[0][0]))
fd_h.write("""\t\t\treturn createFont%s%s(id, line_space, char_space, u32Opt); }\n""" % (lstsrc[0][0], '_std'))
# close
fd_h.write("}\n")
fd_h.close()
