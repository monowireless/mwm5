#pragma once 

/* Copyright (C) 2019-2020 Mono Wireless Inc. All Rights Reserved.
 * Released under MW-OSSLA-1J,1E (MONO WIRELESS OPEN SOURCE SOFTWARE LICENSE AGREEMENT). */

/** @file
 *
 * @defgroup SERCMD シリアルコマンドの解釈を行う
 * シリアルコマンドの解釈を行う。解釈部は書式に応じてアスキー形式、バイナリ形式、必要で
 * あるなら本定義に基づき拡張も可能である。
 *
 * - 入力の仕様（アスキー形式）
 *   - :([0-9A-F][0-9A-F])+[0-9A-F][0-9A-F][CR][LF]
 *   - 最後の 2 バイトは LRC となっていてLRCを含む 16進変換したバイトを加算すると 0 になる。\n
 *     つまり LRC は全データの和を取り、符号を反転（二の補数）させたもの。
 * - 入力の仕様（バイナリ形式）
 *   - 0xA5 0x5A [LenH] [LenL] [Payload] [XOR] [EOT]
 *      - LenH: ペイロードの長さ(バイト)の上位バイト (MSB を 0x80 にすること)
 *      - LenL: ペイロードの長さ(バイト)の下位バイト
 *      - Payload: ペイロード
 *      - XOR: ペイロード部の各バイトの XOR 値を用いたチェックサム
 *      - EOT: 省略可能。vOutput() 出力時には付与される。
 *
 * - API 利用方法
 *   - 準備
 *     - 事前に tsSerCmd_Context sSC 構造体および入力バッファデータ領域を確保しておく
 *     - SerCmdAscii_vInit() SerCmdBinary_vInit() 関数により構造体を初期化する
 *     - 入力タイムアウトを設定する場合は tsSerCmd_Context 構造体の u16timeout メンバーの値[ms]を設定する
 *
 *   - 解釈実行
 *     - UART からの入力バイト(u8Byte)を  uint8 u8ret = sSC._u8Parse(&sSC, u8Byte) に入力する
 *     - u8ret の値が 0x80 以上になると、解釈が終了したことを示す
 *        - E_SERCMD_COMPLETE: エラーなく系列が入力された。この呼び出しコンテキスト中で処理すること。
 *                             続く入力処理 (->_u8Parse(), ->bComplete())が行われた時点でデータの一貫性
 *                             は保たれない。
 *        - E_SERCMD_ERROR: 解釈エラー
 *        - E_SERCMD_CHECKSUM_ERROR: チェックサムだけ合わなかった。このとき sSC.u16cksum に期待したチェックサム値が格納される。
 *     - 続けて ->_u8Parse() を実行する。直前の状態が終了・エラー状態であっても、新たな系列として解釈を始めます。
 *
 *   - タイムアウトの処理
 *     外部のタイマから関数 ->bComplete() を実行するとタイムアウトのチェックを行う。
 *     - bComplete() が実装されていない(=NULL)の場合はタイムアウト処理は行えない。
 *     - 関数 ->bComplete() が TRUE を戻した場合は、呼び出しコンテキスト中でデータを利用すること。
 *       続く入力処理 (->_u8Parse(), ->bComplete())が行われた時点でデータの一貫性は保たれない。
 *
 */

#include "twe_common.hpp"
#include "twe_stream.hpp"
#include "twe_utils.hpp"
#include "twe_stream.hpp"
#ifndef TWE_STDINOUT_ONLY
#include "twe_console.hpp"
#endif

#include "twe_utils_fixedque.hpp"

namespace TWESERCMD {
	/// <summary>
	/// シリアルコマンド解釈の内部状態
	/// 本状態は各解釈部共通の定義とする
	/// </summary>
	typedef enum {
		E_TWESERCMD_EMPTY = 0,                //!< 入力されていない
		E_TWESERCMD_COMPLETE = 0x80,          //!< 入力が完結した(LCRチェックを含め)
		E_TWESERCMD_ERROR = 0x81,             //!< 入力エラー
		E_TWESERCMD_CHECKSUM_ERROR = 0x82,   //!< チェックサムが間違っている
	} teSerCmdGenState;

	/// <summary>
	/// シリアルコマンドパーサーの基底クラス
	/// </summary>
	class IParser
	{
	private:
	protected:
		uint8_t u8state; //!< 状態
		uint8_t bDynamic;
		TWEUTILS::SmplBuf_Byte& payload; //!< バッファ
		
		inline void _init() {
			payload.redim(0);
			u8state = E_TWESERCMD_EMPTY;
		}

		/// <summary>
		/// 文字列パースの仮想関数（１バイト読み込み完了したら complete 状態になる）
		/// </summary>
		/// <param name="u8b"></param>
		/// <returns></returns>
		virtual uint8_t _u8Parse(uint8_t u8b) = 0;

		/// <summary>
		/// 出力用の仮想関数 (operator >> 参照)
		/// </summary>
		/// <param name="bobj"></param>
		/// <param name="p"></param>
		virtual void _vOutput(TWEUTILS::SmplBuf_Byte& bobj, TWE::IStreamOut& p) = 0;

	public:

		IParser(size_t siz) :
			payload(*new TWEUTILS::SmplBuf_Byte(uint16_t(siz))),
			bDynamic(true),
			u8state(E_TWESERCMD_EMPTY) { }

		IParser(TWEUTILS::SmplBuf_Byte& bobj) :
			payload(bobj), 
			bDynamic(false),
			u8state(E_TWESERCMD_EMPTY)
		{
			payload.redim(0);
		}

		virtual ~IParser() {
			if (bDynamic) {
				TWEUTILS::SmplBuf_Byte* p = &payload;
				delete p;
			}
		}

		// return true, if parsing has been completed.
		inline operator bool() { return u8state == E_TWESERCMD_COMPLETE; }
		// get payload length
		inline uint16_t length() { return payload.length(); }
		// access payload with array index.
		inline uint8_t operator[](int i) { return payload[i]; }

		// putting a byte and do parse
		inline IParser& operator << (char_t c) { return Parse(c); }
		inline IParser& operator << (int c) { return Parse(c); }

		// duplicate payload buffer.
		inline TWEUTILS::SmplBuf_Byte& operator >> (TWEUTILS::SmplBuf_Byte &b) {
			b.reserve(b.length() + payload.length());
			std::copy(payload.begin(), payload.end(), std::back_inserter(b));
			return b;
		}
		// output formatted buffer.
		inline void operator >> (TWE::IStreamOut& fo_putchar) { _vOutput(payload, fo_putchar);  }

		// get internal state as defined in teSerCmdGenState.
		inline uint8_t state() { return u8state; }
		// return true, if parsing has been completed.
		inline bool is_complete() { return u8state == E_TWESERCMD_COMPLETE; }
		// set payload data explicitly (from array object)
		template <typename T>
		inline void set_payload(T& bobj) {
			payload.reserve_and_set_empty(bobj.size());
			std::copy(bobj.begin(), bobj.end(), std::back_inserter(payload));
		}
		// set payload data explicitly (from set of begin/end pointers)
		inline void set_payload(uint8_t *b, uint8_t *e) {
			payload.reserve_and_set_empty(uint32_t(e - b));
			while(b != e) { payload.push_back(*b); ++b; }
		}
		// get payload object.
		inline TWEUTILS::SmplBuf_Byte& get_payload() { return payload; }

		// public interface
		inline IParser& Parse(uint8_t u8b) { _u8Parse(u8b); return *this; }

		// re-init
		inline virtual void reinit() = 0;
	};

	inline TWE::IStreamOut& operator << (TWE::IStreamOut& lhs, IParser& rhs) {
		rhs.operator>>(lhs);
		return lhs;
	}

#ifndef TWE_STDINOUT_ONLY
	inline TWE::IStreamOut& operator << (TWETERM::ITerm& lhs, IParser& rhs) {
		rhs.operator>>(lhs);
		return lhs;
	}
#endif
}

extern TWEUTILS::InputQueue<uint8_t> the_uart_queue;
