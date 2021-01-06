/* Copyright (C) 2019-2020 Mono Wireless Inc. All Rights Reserved.
 * Released under MW-OSSLA-1J,1E (MONO WIRELESS OPEN SOURCE SOFTWARE LICENSE AGREEMENT). */

#include <cstring>
#include "twe_common.hpp"
#include "twe_firmprog.hpp"

using namespace TWE;

#define DEBUG_PERFORM_TIMEOUT // perform time out while at debug build

#if defined(ESP32)
//#include <M5Stack.h>
#endif

bool ITweBlProtocol::request_body(int8_t id_req, uint8_t id_resp) {
	// set header
	_arg_buf[0] = _arg_buf.length(); // LENGTH
	_arg_buf[1] = id_req; // CMD ID

	// PUT CRC on the last byte.
	uint8_t crc = TWEUTILS::XOR_u8Calc(_arg_buf.data(), uint8_t(_arg_buf.length()));
	_arg_buf.append(crc);

	// Send it to Serial
	serial_write((const char*)_arg_buf.begin().raw_ptr(), _arg_buf.length());

	_tim_start = millis();

	// prepare respbuffer
	if (id_resp != 0) {
		_resp_id_expected = id_resp;
		_resp_buf.resize(0);
		_resp_state = RESP_STAT_PROCESS;
	}

	return true;
}

bool ITweBlProtocol::receive(int c) {
	if (_resp_state == RESP_STAT_PROCESS && c == -1) {
		// timeout
#if defined(DEBUG_PERFORM_TIMEOUT) || !(defined(_MSC_VER) && defined(_DEBUG))
		if (millis() - _tim_start > _tim_timeout) {
			_resp_state = RESP_STAT_ERROR;
			
			return true;
		}
#endif
	} else 
	if (_resp_state == RESP_STAT_PROCESS && c >= 0) {
		if (_resp_buf.length() == 0) {
			if (c < 2) { // first byte is length of packet, len < 2 is too short.
				_resp_state = RESP_STAT_ERROR;
				return true;
			}
		}

		_resp_buf.append(c);

		// completion
		if (_resp_buf.length() == _resp_buf[0] + 1) {

			uint8_t crc = TWEUTILS::XOR_u8Calc(_resp_buf.data(), _resp_buf.length() - 1);

			if (crc == _resp_buf[-1]) {
				if (_resp_buf[1] == _resp_id_expected) {
					_resp_state = RESP_STAT_COMPLETED;
				}
				else {
					_resp_state = RESP_STAT_UNEXPECTED_MSG;
				}

				return true;
			}
			else {
				_resp_state = RESP_STAT_CRC_ERROR;
				return true;
			}
		}
	}

	return false;
}

/** @brief	Protocol sequence CONNECT to READ mac addr */
const TweProg::E_ST_TWEBLP TweProg::BL_PROTOCOL_GET_MODULE_INFO[] = {
	E_ST_TWEBLP::CONNECT,
	E_ST_TWEBLP::IDENTIFY_FLASH,
	E_ST_TWEBLP::SELECT_FLASH,
	E_ST_TWEBLP::READ_CHIPID,
	E_ST_TWEBLP::READ_MAC_CUSTOM,
	E_ST_TWEBLP::NONE // TERRMINATE
};


/** @brief	Protocol sequence ERASE and WRITE */
const TweProg::E_ST_TWEBLP TweProg::BL_PROTOCOL_ERASE_AND_WRITE[] = {
	E_ST_TWEBLP::CONNECT,
	E_ST_TWEBLP::IDENTIFY_FLASH,
	E_ST_TWEBLP::SELECT_FLASH,
	E_ST_TWEBLP::ERASE_FLASH,
#if defined(__APPLE__) && defined(MWM5_SERIAL_NO_FTDI)
	E_ST_TWEBLP::WRITE_FLASH_FROM_FILE,
	E_ST_TWEBLP::VERIFY_FLASH,
#else
	E_ST_TWEBLP::WRITE_FLASH_FROM_FILE,
	E_ST_TWEBLP::VERIFY_FLASH,
#endif
	E_ST_TWEBLP::NONE // TERRMINATE
};

TweProg::E_ST_TWEBLP TweProg::next_state() {
	if (_p_st_table) {
		if (*_p_st_table == E_ST_TWEBLP::NONE) {
			// no further state, set success
			_state = E_ST_TWEBLP::FINISH;
			_p_st_table = nullptr;
		}
		else {
			// send message
			do {
				_state = *_p_st_table++; // set state from table
				if (process_body(EVENT_NEW_STATE) != PROCESS_SKIP) break;
			} while (1);
		}
	}
	else {
		_state = E_ST_TWEBLP::FINISH_ERROR;
	}

	return _state;
}

void TweProg::error_state() {
	_state = E_ST_TWEBLP::FINISH_ERROR;
	_bl->change_baud(_bl->get_baud_default());
	_bl->reset_module();
}


bool TweProg::begin(const E_ST_TWEBLP* tbl) {
	_p_st_table = tbl;

	_bl->begin();

	if (!_bl->connect()) {
		error_state();
		return false;
	}

	E_ST_TWEBLP ret = next_state();


	if (ret != E_ST_TWEBLP::FINISH_ERROR) {
		_u8protocol_busy = 1;
		return true;
	}
	else {
		return false;
	}
}

bool TweProg::process_input(int c) {
	bool bexit = false;
	bool bcomp = _bl->receive(c); // handle protocol

	if (bcomp) {
		auto rcvstat = _bl->get_receive_status();
		if (rcvstat == ITweBlProtocol::RESP_STAT_COMPLETED) {
			switch (process_body(EVENT_RESPOND)) {
			case PROCESS_SUCCESS:
				next_state();
				if (_state == E_ST_TWEBLP::FINISH || _state == E_ST_TWEBLP::FINISH_ERROR) {
					bexit = true;
				}
				else {
					bexit = false;
				}
				break;
			case PROCESS_CONT:
				bexit = false;
				break;
			default:
				error_state();
				bexit = true;
			}
		} else {
			// error
			error_state();
			bexit = true;
		}
	}

	if (bexit) {
		_u8protocol_busy = false;
	}

	return bexit;
}

/**
 * @fn	int TweProg::process_body(int c)
 *
 * @brief	Process the body described by c
 *
 * @param	c	An int to process.
 * 				EVENT_NEW_STATE : start the state
 * 				EVENT_RESPOND   : has response
 * 				EVENT_PROCESS   : wait more response keeping the state
 *
 * @returns	0:error, 1:success, 2:wait more response
 */
int TweProg::process_body(int c) {
	int ret = false;

#if defined(ESP32)
	//uint8_t U8_BAUD_DIV = 3; // The limit of Serial2 (HardwareSerial)
	const uint8_t U8_BAUD_DIV = 2; // 1Mbps works with Serial2_IDF, however speed is not so fast, so 500kbps is good compromization.
	const uint32_t U32_BAUD = (1000000UL / U8_BAUD_DIV);
#elif defined(MWM5_BUILD_RASPI) // maybe ok with 1Mbps
	const uint8_t U8_BAUD_DIV = 2; 
	const uint32_t U32_BAUD = (1000000UL / U8_BAUD_DIV);
#elif defined(_MSC_VER) || defined(__APPLE__) || defined(__linux) || defined(__MINGW32__)
	const uint8_t U8_BAUD_DIV = 1;
	const uint32_t U32_BAUD = (1000000UL / U8_BAUD_DIV);
#endif

	switch (_state) {
	case E_ST_TWEBLP::CONNECT:
		// change baud rate higher
		if (c == EVENT_NEW_STATE) {
			if (!_bl->get_modctl_enabled()) {
				// skip changing baud rate (safe mode)
				ret = PROCESS_SKIP;
			} else {
				// set fast mode
				if (!_bl->request(0x27, 0x28, U8_BAUD_DIV)) {
					error_state();
					ret = PROCESS_FAIL;
				}
				else ret = PROCESS_SUCCESS;
			}

			if (_protocol_cb) _protocol_cb(E_ST_TWEBLP::CONNECT, EVENT_NEW_STATE, ret, _bl->get_command_buf(), _pobj);
		} else
		if (c == EVENT_RESPOND) {
			// change baud
			_bl->change_baud(U32_BAUD);
			delay(50);

			ret = PROCESS_SUCCESS;
			if (_protocol_cb) _protocol_cb(E_ST_TWEBLP::CONNECT, EVENT_RESPOND, ret, _bl->get_response_buf(), _pobj);
		}
		break;

	case E_ST_TWEBLP::IDENTIFY_FLASH:
		// IDENTIFY_FLASH
		if (c == EVENT_NEW_STATE) {
			if (!_bl->request(0x25, 0x26)) {
				error_state();
				ret = PROCESS_FAIL;
			}
			else ret = PROCESS_SUCCESS;

			if (_protocol_cb) _protocol_cb(E_ST_TWEBLP::IDENTIFY_FLASH, EVENT_NEW_STATE, ret, _bl->get_command_buf(), _pobj);
		}
		else
		if (c == EVENT_RESPOND) {
			auto&& payl = _bl->get_response_buf();

			ret = PROCESS_FAIL;
			if (payl.length() >= 5) {
				// payl[0] : length (FIXED)
				// payl[1] : RESPOND ID (FIXED)
				// payl[2] : flash status (should be 0)
				// payl[3] : flash manufacturer (should be 0xCC)
				// payl[4] : flash type (should be 0xEE)
				if (payl[2] != 0x00 || payl[3] != 0xCC || payl[4] != 0xEE) { // should be [00 CC EE] for TWELITE module
					ret = PROCESS_FAIL;
				}
				else {
					ret = PROCESS_SUCCESS;
				}
			}
			if (!ret) error_state();
			if (_protocol_cb) _protocol_cb(E_ST_TWEBLP::IDENTIFY_FLASH, EVENT_RESPOND, ret, _bl->get_response_buf(), _pobj);
		}
		break;

	case E_ST_TWEBLP::SELECT_FLASH:
		if (c == EVENT_NEW_STATE) {
			if (!_bl->request(0x2C, 0x2D, 0x08)) {
				error_state();
				ret = PROCESS_FAIL;
			}
			else ret = PROCESS_SUCCESS;

			if (_protocol_cb) _protocol_cb(E_ST_TWEBLP::SELECT_FLASH, EVENT_NEW_STATE, ret, _bl->get_command_buf(), _pobj);
		} else
		if (c == EVENT_RESPOND) {
			auto&& payl = _bl->get_response_buf();

			ret = PROCESS_FAIL;
			if (payl.length() >= 3) {
				// payl[0] : length (FIXED)
				// payl[1] : RESPOND ID (FIXED)
				// payl[2] : Status (should be 0)
				if (payl[2] == 0) {
					ret = PROCESS_SUCCESS;
				}
			}
			if (!ret) error_state();

			if (_protocol_cb) _protocol_cb(E_ST_TWEBLP::SELECT_FLASH, EVENT_RESPOND, ret, _bl->get_response_buf(), _pobj);
		}
		break;

	case E_ST_TWEBLP::READ_CHIPID:
		if (c == EVENT_NEW_STATE) {
			if (!_bl->request(0x32, 0x33)) {
				error_state();
				ret = PROCESS_FAIL;
			}
			else ret = PROCESS_SUCCESS;

			if (_protocol_cb) _protocol_cb(E_ST_TWEBLP::READ_CHIPID, EVENT_NEW_STATE, ret, _bl->get_command_buf(), _pobj);
		} else
		if (c == EVENT_RESPOND) {
			auto&& payl = _bl->get_response_buf();
			
			ret = PROCESS_FAIL;
			module_info.type = TweProg::E_MOD_TYPE::UNDEF;

			if (payl.length() >= 7) {
				// payl[0] : length (FIXED)
				// payl[1] : RESPOND ID (FIXED)
				// payl[2] : Status (should be 0)
				// payl[3..6] : Chip ID (MSB first)
				module_info.chip_id = (payl[3] << 24) | (payl[4] << 16) | (payl[5] << 8) | (payl[6]);

				if ((module_info.chip_id & 0xFFFF) == 0x8686)
					module_info.type = TweProg::E_MOD_TYPE::TWELITE_BLUE;
				else if ((module_info.chip_id & 0xFFFF) == 0xB686)
					module_info.type = TweProg::E_MOD_TYPE::TWELITE_RED;

				ret = PROCESS_SUCCESS;
			}
			if (!ret) error_state();

			if (_protocol_cb) _protocol_cb(E_ST_TWEBLP::READ_CHIPID, EVENT_RESPOND, ret, _bl->get_response_buf(), _pobj);
		}
		break;

	case E_ST_TWEBLP::READ_MAC_CUSTOM:
		if (c == EVENT_NEW_STATE) {
			// read 8bytes from 0x01001578 (LSB first)
			if (!_bl->request(0x1F, 0x20, 0x70, 0x15, 0x00, 0x01, 0x08, 00)) {
				error_state();
				ret = PROCESS_FAIL;
			}
			else ret = PROCESS_SUCCESS;

			if (_protocol_cb) _protocol_cb(E_ST_TWEBLP::READ_MAC_CUSTOM, EVENT_NEW_STATE, ret, _bl->get_command_buf(), _pobj);
		} else
		if (c == EVENT_RESPOND) {
			auto&& payl = _bl->get_response_buf();

			ret = PROCESS_FAIL;
			if (payl.length() >= 9) {
				// payl[0] : length (FIXED)
				// payl[1] : RESPOND ID (FIXED)
				// payl[2] : Status (should be 0)
				// payl[3..A] : MAC ADDRESS (MSB first)
				if (payl[2] == 0) {
					for (int i = 3; i < 3 + 8; i++) {
						module_info.mac_addr[i - 3] = payl[i];
					}

					// get serial number from MAC address (lower 28bits)
					module_info.mac_addr_to_serialnumber();

					ret = PROCESS_SUCCESS;
				}
			}
			if (!ret) error_state();

			if (_protocol_cb) _protocol_cb(E_ST_TWEBLP::READ_MAC_CUSTOM, EVENT_RESPOND, ret, _bl->get_response_buf(), _pobj);
		}
		break;

	case E_ST_TWEBLP::ERASE_FLASH:
		if (c == EVENT_NEW_STATE) {
			if (!_bl->request(0x07, 0x08)) {
				error_state();
				ret = PROCESS_FAIL;
			}
			else ret = PROCESS_SUCCESS;

			if (_protocol_cb) _protocol_cb(E_ST_TWEBLP::ERASE_FLASH, EVENT_NEW_STATE, ret, _bl->get_command_buf(), _pobj);
		} else
		if (c == EVENT_RESPOND) {
			auto&& payl = _bl->get_response_buf();

			ret = PROCESS_FAIL;
			if (payl.length() >= 3) {
				// payl[0] : length (FIXED)
				// payl[1] : RESPOND ID (FIXED)
				// payl[2] : status (0x00: success)
				if (payl[2] == 0) {
					ret = PROCESS_SUCCESS;
				}
			}
			if (!ret) error_state();

			if (_protocol_cb) _protocol_cb(E_ST_TWEBLP::ERASE_FLASH, EVENT_RESPOND, ret, _bl->get_response_buf(), _pobj);
		}
		break;

	case E_ST_TWEBLP::WRITE_FLASH_FROM_FILE:
		if (c == EVENT_NEW_STATE || c == EVENT_PROCEED) {
			file_type_shared file = _firm.file.lock();
			if (_firm.file.expired()) return false;

			file->seek(_firm.n_blk * _firm.PROTOCOL_CHUNK + 4);
			file->read(_firm.buf, _firm.PROTOCOL_CHUNK);

			uint32_t u32addr = _firm.n_blk * _firm.PROTOCOL_CHUNK;
			if (!_bl->request(0x09, 0x0A
					, uint8_t(u32addr & 0xff)
					, uint8_t((u32addr >> 8) & 0xff)
					, uint8_t((u32addr >> 16) & 0xff)
					, uint8_t((u32addr >> 24) & 0xff)
					, std::make_pair(_firm.buf, size_t(_firm.PROTOCOL_CHUNK))
					)
				) {
				error_state();
				ret = PROCESS_FAIL;
			}
			else ret = PROCESS_SUCCESS;

			if (_protocol_cb && c == EVENT_NEW_STATE) // NEW STATE occurrs at only the first call.
				_protocol_cb(E_ST_TWEBLP::WRITE_FLASH_FROM_FILE, EVENT_NEW_STATE, ret, _bl->get_command_buf(), _pobj);
		}
		else
		if (c == EVENT_RESPOND) {
			auto&& payl = _bl->get_response_buf();

			ret = PROCESS_FAIL;
			bool berr = true;

			if (payl.length() >= 3) {
				
				// payl[0] : length (FIXED)
				// payl[1] : RESPOND ID (FIXED)
				// payl[2] : status (0x00: success)
				if (payl[2] == 0) {
					// success
					_firm.n_blk++;

					// progress 0..1024
					int progress = (1024 * _firm.n_blk + 512) / _firm.n_blk_e;
					if (_protocol_cb) _protocol_cb(E_ST_TWEBLP::WRITE_FLASH_FROM_FILE, EVENT_RESPOND, TWE::APIRET(true, progress), _bl->get_response_buf(), _pobj);

					if (_firm.n_blk >= _firm.n_blk_e) {
						// the last packet
						ret = PROCESS_SUCCESS;
					} else {
						// next block request
						process_body(EVENT_PROCEED);
					}
					berr = false;
				}
			}
			if (berr) error_state();


			if (berr) return 0;
			else return ret ? PROCESS_SUCCESS : PROCESS_CONT; // 2 continue
		}	
		break;

	case E_ST_TWEBLP::VERIFY_FLASH:
		if (c == EVENT_NEW_STATE) {
			_firm.n_blk = 0;
		}
		if (c == EVENT_NEW_STATE || c == EVENT_PROCEED) {
			uint32_t u32addr = _firm.n_blk * _firm.PROTOCOL_CHUNK;
			if (!_bl->request(0x0B, 0x0C
				, uint8_t(u32addr & 0xff)
				, uint8_t((u32addr >> 8) & 0xff)
				, uint8_t((u32addr >> 16) & 0xff)
				, uint8_t((u32addr >> 24) & 0xff)
				, uint8_t(128)
			)
				) {
				error_state();
				ret = PROCESS_FAIL;
			}
			else ret = PROCESS_SUCCESS;

			if (_protocol_cb && c == EVENT_NEW_STATE) // NEW STATE occurrs at only the first call.
				_protocol_cb(E_ST_TWEBLP::VERIFY_FLASH, EVENT_NEW_STATE, ret, _bl->get_command_buf(), _pobj);
		}
		else
		if (c == EVENT_RESPOND) {
			file_type_shared file = _firm.file.lock();
			if (_firm.file.expired()) {
				return PROCESS_FAIL;
			}

			file->seek(_firm.n_blk * _firm.PROTOCOL_CHUNK + 4);
			file->read(_firm.buf, _firm.PROTOCOL_CHUNK);

			auto&& payl = _bl->get_response_buf();

			ret = PROCESS_FAIL;
			bool berr = true;

			if (payl.length() == 0x84) {
				// payl[0] : length (FIXED)
				// payl[1] : RESPOND ID (FIXED)
				// payl[2] : status (0x00: success)
				// payl[3..] : 128bytes block
				if (payl[2] == 0) { // success
					uint32_t u32addr = _firm.n_blk * _firm.PROTOCOL_CHUNK; // start address
					uint32_t u32addr_e = u32addr + _firm.PROTOCOL_CHUNK; // end+1 address
					if (u32addr_e > file->size()) u32addr_e = file->size();

					// compare file content with read value
					for (int i = 0; (i < _firm.PROTOCOL_CHUNK) && (u32addr + i < u32addr_e); i++) {
						if (payl[3 + i] != _firm.buf[i]) {
							// verify error
							return 0; // error
						}
					}

					// set as next block
					_firm.n_blk++;

					// progress 0..1024
					int progress = (1024 * _firm.n_blk + 512) / _firm.n_blk_e;
					if (_protocol_cb) _protocol_cb(E_ST_TWEBLP::VERIFY_FLASH, EVENT_RESPOND, TWE::APIRET(true, progress), _bl->get_response_buf(), _pobj);

					if (_firm.n_blk >= _firm.n_blk_e) {
						// the last packet
						ret = PROCESS_SUCCESS;
					}
					else {
						// next block request
						process_body(EVENT_PROCEED);
					}
					berr = false;
				}
			}
			if (berr) error_state();
			if (berr) return PROCESS_FAIL;
			else return ret ? PROCESS_SUCCESS : PROCESS_CONT; // 2 continue
		}
		break;
	default:
		break;
	}

	return ret;
}