##########################################################################
#/* Copyright (C) 2019-2020 Mono Wireless Inc. All Rights Reserved.
# * 
# * The twesettings library is dual-licensed under MW-SLA and MW-OSSLA terms.
# * - MW-SLA-1J,1E or later (MONO WIRELESS SOFTWARE LICENSE AGREEMENT).
# * - MW-OSSLA-1J,1E or later (MONO WIRELESS OPEN SOURCE SOFTWARE LICENSE AGREEMENT). */
#
# MONOWIRELESS MAKEFILE TEMPLATE (for TWELITE NET application build)
# TWELITE NET のアプリケーションを作成するための Makefile です。
#
# ディレクトリに格納され使用します（サンプルアプリ参照）
#  {SDKROOT}/ToCoNet/{ApplicationRoot}/{FirmwareType}/Build/
##########################################################################

##########################################################################
### Chip selection (do not edit here)
# チップの選択を行います。指定が無い場合は、外部指定または chipsel.mkの規
# 定値となります。この変数はファイル名やフォルダ名の決定にも使用されます。
#
TWELITE ?= BLUE
#
# また TARGET_DIR 変数に .. 、PROJNAME に ../.. のディレクトリ名を格納
# します。
#
include ../../../../MkFiles/chipsel.mk
##########################################################################

### Version information
# ファイル名生成のためのバージョン定義を読み込みます。
# - VERSION_MAIN, VERSION_SUB, VERSION_VAR
# - ファームウェア中でのバージョン番号を利用する目的で ../Source/Version.h
#   を生成します。
#
include ./Version.mk

### Build Names
# 出力ファイル名のプレフィックスを決めます。指定しなければ、../..の
# ディレクトリ名(PROJNAMEの事前設定値)が使用されます。
# - (../..dir名)_(..dir名)_(JENNIC_CHIP)_(VERSION).
#
PROJNAME=TWENET
SUBNAME=stgs

### Source files
# コンパイル対象のソースコードを列挙します。
# - パスの指定は、APP_COMMON_SOURCE_DIR_ADD? で行います。
# - 以下のパスは事前定義されています。
#   ../Source
#   ../Common/Source
#

APPSRC += twecrc8.c
APPSRC += twenvm.c
APPSRC += tweinputstring.c
APPSRC += tweinteractive.c
APPSRC += tweinteractive_settings.c
APPSRC += tweinteractive_defmenus.c
APPSRC += tweinteractive_nvmutils.c
APPSRC += tweprintf.c
APPSRC += twesercmd_ascii.c
APPSRC += twesercmd_binary.c
APPSRC += twesercmd_chat.c
APPSRC += twesercmd_plus3.c
APPSRC += twesercmd_timeout.c
APPSRC += tweserial.c
APPSRC += twesettings.c
APPSRC += twesettings_cmd.c
APPSRC += twesettings_std.c
APPSRC += twesettings_std_defsets.c
APPSRC += twesettings_validator.c
APPSRC += twestring.c

APPSRC += tweserial_jen.c
APPSRC += twesysutils.c

APPSRC += printf.c

### Target type
# アプリケーション(.bin)をビルドするか、ライブラリ(.a)にするか指定します。
# - ライブラリをリンクする場合は、ADDITIONAL_LIBS に定義を加えてください。

#TARGET_TYPE=bin
TARGET_TYPE = a

### ToCoNet debug build
# ToCoNet のスタックデバッグ出力を使用する場合 1 を指定します。
# - ターゲットファイル名に _TDBG が追加されます。 
#
TOCONET_DEBUG ?= 0

##########################################################################
### Basic definitions (do not edit here)
# コンパイルオプションやディレクトリなどの値を設定します。
include ../../../../MkFiles/twenet.mk
##########################################################################

### CPP Definition, C flags
# 必要に応じてプリプロセッサ定義などを追加します。
# - 最適化オプションを含め多くのオプションは事前定義されているため
#   通常は指定不要です。
#
#CFLAGS += -DDUMMY

### Build Options
# プラットフォーム別のビルドをしたり、デバッグ用のビルドのファイル名を変更
# したいような場合の設定。
# - OBJDIR_SUFF は objs_$(JENNIC_CHIP) に追加されます。
# - TARGET_SUFF は生成ファイル名に追加されます。
#
# 以下の例では、make DEBUGOPT=1 とした時に、CPP の DEBUG を定義し、ファイル
# 名に _DBG を追加します。
#ifeq ($(DEBUGOPT),1)
#  CFLAGS += -DDEBUG
#  OBJDIR_SUFF += _Debug
#  TARGET_SUFF += _Debug
#endif 

### Additional Src/Include Path
# 下記に指定したディレクトリがソース検索パス、インクルード検索パスに設定
# されます。Makefile のあるディレクトリからの相対パスを指定します。
#
APP_COMMON_SRC_DIR_ADD1 = ../printf
#APP_COMMON_SRC_DIR_ADD2 = 
#APP_COMMON_SRC_DIR_ADD3 = 
#APP_COMMON_SRC_DIR_ADD4 =
#
# インクルードディレクトリのみを追加指定したい場合は、-I オプションとして
# INCFLAGS に追加します。
#
#INCFLAGS += -I../dummy 

### Additional user objects (.a,.o)
# 追加でリンクしたいライブラリ、オブジェクトファイルを指定します。
# 
#ADDITIONAL_LIBS += ../../Common/libDummy_$(JENNIC_CHIP).a

### Additional library 
# コンパイラ付属ライブラリ (math, spp など) を指定します。
# ※ 各ライブラリの動作保証はありません。
#
#LDLIBS += m

### Additional linker options 
# リンカへの追加オプションです。
# - 必要なオプションは事前定義されているため通常は指定不要です。
# 
#LDFLAGS += -u dummy

### Change Optimize Level
# 最適化レベルを変更します。
# -Os     : サイズの最適化(デフォルト)
# -O, -O1 : 
# -O2     : 
OPTFLAG=-O2
#OPTFLAG=-O2 -std=c99

#########################################################################
### Include rules (do not edit here)
# コンパイルルールが定義されます。
include ../../../../MkFiles/rules.mk
#########################################################################

#########################################################################
### Change Optimize Level
# 最適化レベルを変更します。
# -Os     : サイズの最適化(デフォルト)
# -O, -O1 : 
# -O2     : 
#CFLAGS  := $(subst -Os,-O2,$(CFLAGS))
#LDFLAGS  := $(subst -Os,-O2,$(LDFLAGS))

#########################################################################
### other rules (install, etc)
INST_LIB=lib$(PROJNAME)$(SUBNAME)_$(TWELITE).a
INST_LIB_DIR=../../lib
INST_H_DIR=../../include/$(PROJNAME)$(SUBNAME)

install:
	@mkdir -pv $(INST_H_DIR)
	@rm -fv $(INST_H_DIR)/*.h
	@rm -fv $(INST_LIB_DIR)/$(INST_LIB) $(INST_LIB_DIR)/$(INST_LIB).*
	@cp -fv twe*.h $(INST_H_DIR)/
	@cp -fv $(TARGET_BIN).a $(INST_LIB_DIR)/$(INST_LIB)
	@touch -f $(INST_LIB_DIR)/$(INST_LIB).$(VERSION_MAIN)_$(VERSION_SUB)_$(VERSION_VAR)
	@mkdir -pv $(INST_H_DIR)/../printf
	@cp -fv ../printf/printf.h $(INST_H_DIR)/../printf
	
