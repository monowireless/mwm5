#pragma once

/* Copyright (C) 2019-2020 Mono Wireless Inc. All Rights Reserved.
 * Released under MW-OSSLA-1J,1E (MONO WIRELESS OPEN SOURCE SOFTWARE LICENSE AGREEMENT). */

#include <utility>
#include <type_traits>

#include "twe_common.hpp"
#include "twe_utils.hpp"
#include "twe_utils_simplebuffer.hpp"
#include "twe_utils_crc8.hpp"
#include "twe_serial.hpp"
#include "twe_sys.hpp"

#include "twe_file.hpp"

namespace TWE {
	class ITweBlProtocol {
	public:
		static const uint8_t RESP_STAT_NONE = 0;
		static const uint8_t RESP_STAT_PROCESS = 1;
		static const uint8_t RESP_STAT_COMPLETED = 0x81;
		static const uint8_t RESP_STAT_ERROR = 0x82;
		static const uint8_t RESP_STAT_CRC_ERROR = 0x83;
		static const uint8_t RESP_STAT_UNEXPECTED_MSG = 0x84;

	public:
		ITweBlProtocol() :
			_arg_buf(256), _resp_buf(256), _resp_id(0), _resp_id_expected(0),
			_tim_start(0), _tim_timeout(1000), _resp_state(0) {}
		virtual ~ITweBlProtocol() {}

	public:
		virtual void setup() = 0;
		virtual void serial_write(const char * str, int len) = 0;
		virtual bool connect() = 0;
		virtual bool change_baud(int i) = 0;
		virtual bool reset_module() = 0;
		virtual bool hold_reset_pin() = 0;
		virtual bool setpin(bool bSet) = 0;

	protected:
		TWEUTILS::SmplBuf_Byte _arg_buf;
		TWEUTILS::SmplBuf_Byte _resp_buf;

		uint32_t _tim_start;
		uint32_t _tim_timeout;
		uint8_t _resp_state; // 0:not started, 1:now going, 0x80:completed, 0x81:error
		uint8_t _resp_id, _resp_id_expected;

		
	public:
		inline TWEUTILS::SmplBuf_Byte& get_command_buf() {
			return _arg_buf;
		}

		inline TWEUTILS::SmplBuf_Byte& get_response_buf() {
			return _resp_buf;
		}

		inline uint8_t get_receive_status() {
			return _resp_state;
		}

	private:
		bool request_body(int8_t id_req, uint8_t id_resp);
	
		template <class Cnt>
		void _req_append(Cnt& _cnt) {
			;
		}

		template <class Cnt, class Cnt2, typename... Tail>
		void _req_append(Cnt& _cnt, Cnt2& lst, Tail&&... tail) {
			for (auto x : lst) {
				_cnt.push_back(x);
			}
			_req_append(_cnt, std::forward<Tail>(tail)...);
		}

		template <class Cnt, typename... Tail>
		void _req_append(Cnt& _cnt, uint8_t i, Tail&&... tail) {
			_cnt.push_back(i);
			_req_append(_cnt, std::forward<Tail>(tail)...);
		}

		template <class Cnt, typename... Tail>
		void _req_append(Cnt& _cnt, std::pair<uint8_t *, size_t>&& ary, Tail&&... tail) {
			uint8_t* p = ary.first;
			uint8_t* e = p + ary.second;

			while (p != e) {
				_cnt.push_back(*p);
				++p;
			}
			_req_append(_cnt, std::forward<Tail>(tail)...);
		}

	public:
		bool receive(int c);

		template <typename... Tail>
		bool request(int8_t id_req, uint8_t id_resp, Tail&&... tail) {
			_arg_buf.resize(2);
			if (sizeof...(tail) > 0) {
				_req_append(_arg_buf, std::forward<Tail>(tail)...);
			}

			return request_body(id_req, id_resp);
		}

	};

	/**
	 * @class	TweBlProtocol
	 *
	 * @brief	A twe bl protocol.
	 *
	 * @tparam	SER   	Type of the Serial class, where the Serial class is implemented differently.
	 * 					(difficult to make virtual class for common interface, but with same methods.)
	 * @tparam	MODCTL	Type of the module control class, which offers PROGRAM MODE, RESET.
	 */
	template <class SER, class MODCTL>
	class TweBlProtocol : public ITweBlProtocol {
	public:
		SER& _ser;
		MODCTL& _modc;

	public:
		TweBlProtocol(SER& ser, MODCTL& modc) :
			_ser(ser), _modc(modc), ITweBlProtocol() {}

		~TweBlProtocol() {}

		void setup() {
			_modc.setup();
		}

		void _discard_readbuffer() {
#ifndef ESP32
			_ser.update(); // read actual data
#endif
			// discard garbage bytes
			while (_ser.available()) (void)_ser.read();
		}

		bool change_baud(int i) {
			_ser.begin(i);
			_ser.flush();
			delay(50);
			_discard_readbuffer();

			return true;
		}

		bool connect() {
			_ser.begin(38400);
			_modc.prog();
			_discard_readbuffer();

			return true;
		}

		bool reset_module() {
			_ser.flush();
			_ser.begin(115200);

			delay(50);

			return _modc.reset();
		}

		bool hold_reset_pin() {
			return _modc.reset(true);
		}

		bool setpin(bool bSet) {
			return _modc.setpin(bSet);
		}

		void serial_write(const char* str, int len) {
			_ser.write((const uint8_t *)str, len);
		}
	};
	
	class TweProg {
	public:
		typedef std::shared_ptr<TweFile> file_type_shared;
		typedef std::weak_ptr<TweFile> file_type_weak;

		static const int EVENT_NEW_STATE = 0;
		static const int EVENT_RESPOND = 1;
		static const int EVENT_PROCEED = 2;

		enum class E_ST_TWEBLP {
			NONE = 0,
			CONNECT,
			IDENTIFY_FLASH,
			SELECT_FLASH,
			READ_CHIPID,
			READ_MAC_CUSTOM,
			ERASE_FLASH,
			WRITE_FLASH_FROM_FILE,
			FINISH = 0x81,
			FINISH_ERROR,
			MASK_FINISH = 0x8F
		};

		enum class E_MOD_TYPE {
			UNDEF = 0,
			TWELITE_BLUE,
			TWELITE_RED
		};
	
		static const E_ST_TWEBLP BL_PROTOCOL_GET_MODULE_INFO[];
		static const E_ST_TWEBLP BL_PROTOCOL_ERASE_AND_WRITE[];

	private:
		/* member vars */
		std::unique_ptr<ITweBlProtocol> _bl;

		E_ST_TWEBLP _state;
		const E_ST_TWEBLP* _p_st_table;

		TWEUTILS::SmplBuf_Byte _buf_payload;

		uint8_t _u8protocol_busy;

	private:
		/* private member funcs */
		void set_state(E_ST_TWEBLP s) { _state = s; }

		E_ST_TWEBLP next_state();

		void error_state();

		int process_body(int c);

	public:

		/**
		 * @typedef	bool (*PF_PROTOCOL_CB)(E_ST_TWEBLP cmd, int request_or_respond, TWEUTILS::SmplBuf_Byte& payl)
		 *
		 * @brief		report each protocol status.
		 * 
		 * @param cmd			protocol name
		 * @param req_or_resp	EVENT_NEW_STATE: beginning of protocol (sending a message)
		 * 						EVENT_RESPOND: end of protocol (receiving a message)
		 * @param evarg			depending on protocol (e.g. receive packet number)
		 * @param payl			sent/received message
		 * @param pObj			optional object pointer
		 * 	
		 */
		typedef void (*PF_PROTOCOL_CB)(
			E_ST_TWEBLP cmd,
			int req_or_resp,
			TWE::APIRET evarg,
			TWEUTILS::SmplBuf_Byte& payl,
			void *pobj
		);


		/**
		 * @struct	_firm
		 *
		 * @brief	firmware data to be written.
		 */
		struct sFirm {
			static const size_t PROTOCOL_CHUNK = 128;
			file_type_weak file;
			uint8_t buf[PROTOCOL_CHUNK];
			size_t len;
			uint16_t n_blk;
			uint16_t n_blk_e;
			uint8_t header[4];

			sFirm()
				: file()
				, buf{}
				, len(0)
				, n_blk(0)
				, n_blk_e(0)
				, header{}
			{}
		} _firm;

	private:
		PF_PROTOCOL_CB _protocol_cb;
		void* _pobj;

	public:

		/**
		 * @fn	TweProg::TweProg(ITweBlProtocol& bl) : _bl(bl), _state(E_ST_TWEBLP::NONE), _p_st_table(nullptr) , _buf_payload(32), module_info
		 *
		 * @brief	Constructor
		 *
		 * @param [in,out]	bl	The bl.
		 */
		TweProg(ITweBlProtocol* bl) : _bl(bl), _state(E_ST_TWEBLP::NONE), _p_st_table(nullptr)
			, _buf_payload(32), module_info{}, _protocol_cb(nullptr), _u8protocol_busy(0), _firm{} {}

		/**
		 * @fn	void TweProg::add_cb(PF_PROTOCOL_CB pfcb)
		 *
		 * @brief	Adds ptr to callback function to report start/end of each protocol.
		 *
		 * @param	pfcb	The callback function
		 */
		void add_cb(PF_PROTOCOL_CB pfcb, void *pobj = nullptr) {
			_protocol_cb = pfcb;
			_pobj = pobj;
		}

		/**
		 * @fn	E_ST_TWEBLP TweProg::get_state()
		 *
		 * @brief	Gets the state
		 *
		 * @returns	The state.
		 */
		E_ST_TWEBLP get_state() {
			return _state;
		}

		/**
		 * @fn	void TweProg::clear_state()
		 *
		 * @brief	Clears the state
		 */
		void clear_state() {
			_state = E_ST_TWEBLP::NONE;
		}


		void _new_bl(ITweBlProtocol* bl) {
			_bl.reset(bl);
		}

		/**
		 * @fn	void TweProg::setup()
		 *
		 * @brief	Setups this object
		 */
		void setup() {
			_bl->setup();
		}

		/**
		 * @fn	bool TweProg::begin(E_ST_TWEBLP (&tbl)[])
		 *
		 * @brief	Begins bootloader operation
		 *
		 * @param [in,out]	tbl	state def table
		 *
		 * @returns	True if it succeeds, false if it fails.
		 */
		bool begin(const E_ST_TWEBLP* tbl);


		/**
		 * @fn	bool TweProg::process_input(uint8_t c)
		 *
		 * @brief	Process the input described by c
		 *
		 * @param	c	An uint8_t to process.
		 *
		 * @returns	True if it finishes, False if waits more bytes from serial
		 */
		bool process_input(int c);


		/**
		 * @fn	inline bool TweProg::is_protocol_busy()
		 *
		 * @brief	Query if protocol is now running.
		 *
		 * @returns	True if protocol busy, false if not.
		 */
		inline bool is_protocol_busy() {
			return _u8protocol_busy;
		}


		/**
		 * @fn	void TweProg::reset_module()
		 *
		 * @brief	Resets the module
		 *
		 */
		void reset_module() {
			_bl->reset_module();
		}

		/**
		 * @fn	void TweProg::reset_hold_module()
		 *
		 * @brief	hold reset pin.
		 */
		void reset_hold_module() {
			_bl->hold_reset_pin();
		}

		/**
		 * @fn	void TweProg::setpin(bool bSet)
		 *
		 * @brief	Setpins
		 *
		 * @param	bSet	True to set.
		 */
		void setpin(bool bSet) {
			_bl->setpin(bSet);
		}

		/**
		 * @fn	void TweProg::set_firmware_data(uint8_t* ptr, size_t len)
		 *
		 * @brief	Sets firmware data
		 *
		 * @param [in,out]	ptr	If non-null, the pointer of firmware stored.
		 * @param 		  	len	The length.
		 */
		bool set_firmware_data(file_type_shared ps_file) {
			_firm.file = file_type_weak(ps_file);

			// get file obj from weak ptr
			file_type_shared file = _firm.file.lock();
			if (_firm.file.expired()) return false;
			if (!file->is_opened()) return false;
			
			// header BLUE 0x04 03 00 08, RED 0F 03 00 0B
			for (auto& x : _firm.header) {
				x = uint8_t(file->read());
			}

			_firm.len =file->size() - 4;
			_firm.n_blk = 0;
			_firm.n_blk_e = uint16_t(_firm.len / _firm.PROTOCOL_CHUNK + 1);
			return true;
		}


		/**
		 * @fn	bool TweProg::rewind_firmware_data()
		 *
		 * @brief	set the firmware writing data as starting.
		 *
		 * @returns	True if it succeeds, false if it fails.
		 */
		bool rewind_firmware_data() {
			_firm.n_blk = 0;

			file_type_shared file = _firm.file.lock();
			if (_firm.file.expired()) return false;

			return file->seek(0);
		}

		/**
		 * @struct	module_info
		 *
		 * @brief	Information about the module.
		 *
		 */
		struct {
			uint8_t mac_addr[8];
			uint32_t serial_number;
			E_MOD_TYPE type;
			uint32_t chip_id;

			void mac_addr_to_serialnumber() {
				serial_number = ((mac_addr[4] & 0x0F) << 24) | (mac_addr[5] << 16) | (mac_addr[6] << 8) | mac_addr[7] | 0x80000000;
			}
		} module_info;
	};

	// the instance
	extern TweProg twe_prog;
}