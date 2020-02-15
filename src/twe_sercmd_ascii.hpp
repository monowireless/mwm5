#pragma once

/* Copyright (C) 2020 Mono Wireless Inc. All Rights Reserved.  *
 * Released under MW-OSSLA-*J,*E (MONO WIRELESS OPEN SOURCE    *
 * SOFTWARE LICENSE AGREEMENT).                                */

#include "twe_common.hpp"
#include "twe_stream.hpp"
#include "twe_sercmd.hpp"
#include "twe_sys.hpp"

namespace TWESERCMD {
	/// <summary>
	/// ASCII形式を解釈するクラス
	/// </summary>
	class AsciiParser : public IParser, public TWESYS::TimeOut
	{
	private:
		uint16_t u16pos; //!< 入力位置（内部処理用）
		uint16_t u16cksum; //!< チェックサム

	protected:
		inline void _vOutput(TWEUTILS::SmplBuf_Byte& bobj, TWE::IStreamOut& p) { AsciiParser::s_vOutput(bobj, p); }

	public:
		AsciiParser(TWEUTILS::SmplBuf_Byte& bobj) : IParser(bobj), u16pos(0), u16cksum(0) { }
		AsciiParser(size_t maxbuffsiz) : IParser(maxbuffsiz), u16pos(0), u16cksum(0) { }
		~AsciiParser() {}
		uint8_t _u8Parse(char_t c);

		/// <summary>
		/// 再初期化
		/// </summary>
		void reinit() { 
			IParser::_init(); 
			u16cksum = 0;
			u16pos = 0;
		}

		/// <summary>
		/// static 定義のフォーマット出力関数（１バイト分出力＆チェックサム計算）
		/// </summary>
		/// <param name="bobj"></param>
		/// <param name="p"></param>
		static void vPutByte(uint8_t u8byte, uint8_t* pu8lrc, TWE::IStreamOut& p);

		/// <summary>
		/// static 定義のフォーマット出力関数
		/// </summary>
		/// <param name="bobj"></param>
		/// <param name="p"></param>
		static void s_vOutput(TWEUTILS::SmplBuf_Byte& bobj, TWE::IStreamOut& p);
	};
}