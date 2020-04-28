#pragma once

#include <cstdint>
#include <utility>

namespace TWEUTILS {
	int _expand_bytes(const uint8* b, const uint8* e) {
		return 0;
	}

#if 0 // not define the generic templates
	template <typename Head, typename... Tail>
	int _expand_bytes(const uint8* b, const uint8* e, Head&& head, Tail&&... tail)
	{
		// assume uint8[]
		memcpy(&head, b, sizeof(head));
		return _expand_bytes(b + sizeof(head), e, std::forward<Tail>(tail)...);
	}
#endif

	template <typename... Tail>
	int _expand_bytes(const uint8* b, const uint8* e, uint32& head, Tail&&... tail);

	template <typename... Tail>
	int _expand_bytes(const uint8* b, const uint8* e, uint16& head, Tail&&... tail);

	template <typename... Tail>
	int _expand_bytes(const uint8* b, const uint8* e, uint8& head, Tail&&... tail);

	template <typename... Tail, int S>
	int _expand_bytes(const uint8* b, const uint8* e, uint8(&head)[S], Tail&&... tail);

	template <typename... Tail>
	int _expand_bytes(const uint8* b, const uint8* e, uint32& head, Tail&&... tail)
	{
		if (b + 4 <= e) {
			head = (*(b) << 24) | (*(b+1) << 16) | (*(b+2) << 8) | (*(b+3) << 0);
			return 4 + _expand_bytes(b + 4, e, std::forward<Tail>(tail)...);
		}
		else {
			return 0;
		}
	}

	template <typename... Tail>
	int _expand_bytes(const uint8* b, const uint8* e, uint16& head, Tail&&... tail)
	{
		if (b + 2 <= e) {
			head = (*(b) << 8) | (*(b+1) << 0);
			return 2 + _expand_bytes(b + 2, e, std::forward<Tail>(tail)...);
		}
		else {
			return 0;
		}
	}

	template <typename... Tail>
	int _expand_bytes(const uint8* b, const uint8* e, uint8& head, Tail&&... tail)
	{
		if (b + 1 <= e) {
			head = (*(b) << 0);
			return 1 + _expand_bytes(b + 1, e, std::forward<Tail>(tail)...);
		}
		else {
			return 0;
		}
	}

	template <typename... Tail, int S>
	int _expand_bytes(const uint8* b, const uint8* e, uint8(&head)[S], Tail&&... tail) {
		if (b + S <= e) {
			memcpy(head, b, S);
			return S + _expand_bytes(b + sizeof(head), e, std::forward<Tail>(tail)...);
		}
		else {
			return 0;
		}
	}

	template <typename Head, typename... Tail>
	const uint8* expand_bytes(const uint8* b, const uint8* e, Head&& head, Tail&&... tail) {
		int ret = _expand_bytes(b, e, std::forward<Head>(head), std::forward<Tail>(tail)...);
		if (ret) {
			return b + ret;
		}
		else {
			return nullptr;
		}
	}
}

namespace TWEUTILS {
	int _pack_bytes(uint8* b, uint8* e) {
		return 0;
	}

	template <typename... Tail>
	int _pack_bytes(uint8* b, uint8* e, const uint32& head, Tail&&... tail);

	template <typename... Tail>
	int _pack_bytes(uint8* b, uint8* e, const uint16& head, Tail&&... tail);

	template <typename... Tail>
	int _pack_bytes(uint8* b, uint8* e, const uint8& head, Tail&&... tail);

	template <typename... Tail, int S>
	int _pack_bytes(uint8* b, uint8* e, const uint8(&head)[S], Tail&&... tail);

	template <typename... Tail>
	int _pack_bytes(uint8* b, uint8* e, const uint32& head, Tail&&... tail)
	{
		if (b + 4 <= e) {
			*(b++) = static_cast<uint8>((head >> 24) & 0xff);
			*(b++) = static_cast<uint8>((head >> 16) & 0xff);
			*(b++) = static_cast<uint8>((head >> 8) & 0xff);
			*(b++) = static_cast<uint8>((head >> 0) & 0xff);
			return 4 + _pack_bytes(b, e, std::forward<Tail>(tail)...);
		}
		else {
			return 0;
		}
	}

	template <typename... Tail>
	int _pack_bytes(uint8* b, uint8* e, const uint16& head, Tail&&... tail)
	{
		if (b + 2 <= e) {
			*(b++) = static_cast<uint8>((head >> 8) & 0xff);
			*(b++) = static_cast<uint8>((head >> 0) & 0xff);
			return 2 + _pack_bytes(b, e, std::forward<Tail>(tail)...);
		}
		else {
			return 0;
		}
	}

	template <typename... Tail>
	int _pack_bytes(uint8* b, uint8* e, const uint8& head, Tail&&... tail)
	{
		if (b + 1 <= e) {
			*(b++) = static_cast<uint8>((head >> 0) & 0xff);
			return 1 + _pack_bytes(b, e, std::forward<Tail>(tail)...);
		}
		else {
			return 0;
		}
	}

	template <typename... Tail, int S>
	int _pack_bytes(uint8* b, uint8* e, const uint8(&head)[S], Tail&&... tail) {
		if (b + S <= e) {
			memcpy(b, head, S);
			return S + _pack_bytes(b + sizeof(head), e, std::forward<Tail>(tail)...);
		}
		else {
			return 0;
		}
	}

	// constructor with variable numbers of arguments, using parameter list.
	template <typename Head, typename... Tail>
	uint8* pack_bytes(uint8* b, uint8* e, Head&& head, Tail&&... tail) {
		int ret = _pack_bytes(b, e, std::forward<Head>(head), std::forward<Tail>(tail)...);
		if (ret) {
			return b + ret;
		}
		else {
			return nullptr;
		}
	}
}