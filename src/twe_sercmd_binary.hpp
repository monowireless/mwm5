#pragma once

/* Copyright (C) 2019-2020 Mono Wireless Inc. All Rights Reserved.
 * Released under MW-OSSLA-1J,1E (MONO WIRELESS OPEN SOURCE SOFTWARE LICENSE AGREEMENT). */

#include "twe_common.hpp"
#include "twe_stream.hpp"
#include "twe_sys.hpp"
#include "twe_sercmd.hpp"


namespace TWESERCMD {
	/// <summary>
	/// バイナリ形式を解釈するクラス
	/// </summary>
	class BinaryParser : public IParser, public TWESYS::TimeOut
	{
	private:
		uint16_t u16pos; //!< 入力位置（内部処理用）
		uint16_t  u16cksum; //!< チェックサム
		
	protected:
		static const uint8_t SERCMD_SYNC_1 = 0xA5;
		static const uint8_t SERCMD_SYNC_2 = 0x5A;

		inline void _vOutput(TWEUTILS::SmplBuf_Byte& bobj, TWE::IStreamOut& p) { BinaryParser::s_vOutput(bobj, p); }
		uint8_t _u8Parse(uint8_t u8byte);

	public:
		BinaryParser(TWEUTILS::SmplBuf_Byte& bobj) : IParser(bobj), u16pos(0), u16cksum(0) {  }
		~BinaryParser() {}
		
		/// <summary>
		/// 再初期化
		/// </summary>
		void reinit() {
			IParser::_init();
			u16cksum = 0;
			u16pos = 0;
		}

		/// <summary>
		/// static 定義のフォーマット出力関数
		/// </summary>
		/// <param name="bobj"></param>
		/// <param name="p"></param>
		static void s_vOutput(TWEUTILS::SmplBuf_Byte& bobj, TWE::IStreamOut& p);
	};

}
