#include "twe_common.hpp"
#include "twe_font.hpp"

namespace TWEFONT {
	// standard ascii set
	const uint8_t font_8x6[7*128] = {
		0x00,0x00,0x00,0x00,0x00,0x00,0x00, // 0
		0x70,0xf8,0xa8,0xf8,0xd8,0x88,0x70, // 1
		0x70,0xf8,0xa8,0xf8,0x88,0xd8,0x70, // 2
		0x00,0x50,0xf8,0xf8,0xf8,0x70,0x20, // 3
		0x00,0x20,0x70,0xf8,0xf8,0x70,0x20, // 4
		0x70,0x50,0xf8,0xa8,0xf8,0x20,0x70, // 5
		0x20,0x70,0xf8,0xf8,0xf8,0x20,0x70, // 6
		0x00,0x00,0x20,0x70,0x70,0x20,0x00, // 7
		0xf8,0xf8,0xd8,0x88,0x88,0xd8,0xf8, // 8
		0x00,0x00,0x20,0x50,0x50,0x20,0x00, // 9
		0xf8,0xf8,0xd8,0xa8,0xa8,0xd8,0xf8, // 10
		0x00,0x38,0x18,0x68,0xa0,0xa0,0x40, // 11
		0x70,0x88,0x88,0x70,0x20,0xf8,0x20, // 12
		0x78,0x48,0x78,0x40,0x40,0x40,0xc0, // 13
		0x78,0x48,0x78,0x48,0x48,0x58,0xc0, // 14
		0x20,0xa8,0x70,0xd8,0xd8,0x70,0xa8, // 15
		0x80,0xc0,0xf0,0xf8,0xf0,0xc0,0x80, // 16
		0x08,0x18,0x78,0xf8,0x78,0x18,0x08, // 17
		0x20,0x70,0xa8,0x20,0xa8,0x70,0x20, // 18
		0xd8,0xd8,0xd8,0xd8,0xd8,0x00,0xd8, // 19
		0x78,0xa8,0xa8,0x68,0x28,0x28,0x28, // 20
		0x30,0x48,0x50,0x28,0x10,0x48,0x48, // 21
		0x00,0x00,0x00,0x00,0x00,0xf8,0xf8, // 22
		0x20,0x70,0xa8,0x20,0xa8,0x70,0x20, // 23
		0x00,0x20,0x70,0xa8,0x20,0x20,0x20, // 24
		0x00,0x20,0x20,0x20,0xa8,0x70,0x20, // 25
		0x00,0x20,0x10,0xf8,0x10,0x20,0x00, // 26
		0x00,0x20,0x40,0xf8,0x40,0x20,0x00, // 27
		0x00,0x80,0x80,0x80,0xf8,0x00,0x00, // 28
		0x00,0x50,0xf8,0xf8,0x50,0x00,0x00, // 29
		0x00,0x20,0x20,0x70,0xf8,0xf8,0x00, // 30
		0x00,0xf8,0xf8,0x70,0x20,0x20,0x00, // 31
		0x00,0x00,0x00,0x00,0x00,0x00,0x00, // 32
		0x20,0x20,0x20,0x20,0x20,0x00,0x20, // 33
		0x50,0x50,0x50,0x00,0x00,0x00,0x00, // 34
		0x50,0x50,0xf8,0x50,0xf8,0x50,0x50, // 35
		0x20,0x78,0xa0,0x70,0x28,0xf0,0x20, // 36
		0xc0,0xc8,0x10,0x20,0x40,0x98,0x18, // 37
		0x40,0xa0,0xa0,0x40,0xa8,0x90,0x68, // 38
		0x30,0x30,0x20,0x40,0x00,0x00,0x00, // 39
		0x10,0x20,0x40,0x40,0x40,0x20,0x10, // 40
		0x40,0x20,0x10,0x10,0x10,0x20,0x40, // 41
		0x20,0xa8,0x70,0xf8,0x70,0xa8,0x20, // 42
		0x00,0x20,0x20,0xf8,0x20,0x20,0x00, // 43
		0x00,0x00,0x00,0x00,0x30,0x30,0x20, // 44
		0x00,0x00,0x00,0xf8,0x00,0x00,0x00, // 45
		0x00,0x00,0x00,0x00,0x00,0x30,0x30, // 46
		0x00,0x08,0x10,0x20,0x40,0x80,0x00, // 47
		0x70,0x88,0x98,0xa8,0xc8,0x88,0x70, // 48
		0x20,0x60,0x20,0x20,0x20,0x20,0x70, // 49
		0x70,0x88,0x08,0x70,0x80,0x80,0xf8, // 50
		0xf8,0x08,0x10,0x30,0x08,0x88,0x70, // 51
		0x10,0x30,0x50,0x90,0xf8,0x10,0x10, // 52
		0xf8,0x80,0xf0,0x08,0x08,0x88,0x70, // 53
		0x38,0x40,0x80,0xf0,0x88,0x88,0x70, // 54
		0xf8,0x08,0x08,0x10,0x20,0x40,0x80, // 55
		0x70,0x88,0x88,0x70,0x88,0x88,0x70, // 56
		0x70,0x88,0x88,0x78,0x08,0x10,0xe0, // 57
		0x00,0x00,0x20,0x00,0x20,0x00,0x00, // 58
		0x00,0x00,0x20,0x00,0x20,0x20,0x40, // 59
		0x08,0x10,0x20,0x40,0x20,0x10,0x08, // 60
		0x00,0x00,0xf8,0x00,0xf8,0x00,0x00, // 61
		0x40,0x20,0x10,0x08,0x10,0x20,0x40, // 62
		0x70,0x88,0x08,0x30,0x20,0x00,0x20, // 63
		0x70,0x88,0xa8,0xb8,0xb0,0x80,0x78, // 64
		0x20,0x50,0x88,0x88,0xf8,0x88,0x88, // 65
		0xf0,0x88,0x88,0xf0,0x88,0x88,0xf0, // 66
		0x70,0x88,0x80,0x80,0x80,0x88,0x70, // 67
		0xf0,0x88,0x88,0x88,0x88,0x88,0xf0, // 68
		0xf8,0x80,0x80,0xf0,0x80,0x80,0xf8, // 69
		0xf8,0x80,0x80,0xf0,0x80,0x80,0x80, // 70
		0x78,0x88,0x80,0x80,0x98,0x88,0x78, // 71
		0x88,0x88,0x88,0xf8,0x88,0x88,0x88, // 72
		0x70,0x20,0x20,0x20,0x20,0x20,0x70, // 73
		0x38,0x10,0x10,0x10,0x10,0x90,0x60, // 74
		0x88,0x90,0xa0,0xc0,0xa0,0x90,0x88, // 75
		0x80,0x80,0x80,0x80,0x80,0x80,0xf8, // 76
		0x88,0xd8,0xa8,0xa8,0xa8,0x88,0x88, // 77
		0x88,0x88,0xc8,0xa8,0x98,0x88,0x88, // 78
		0x70,0x88,0x88,0x88,0x88,0x88,0x70, // 79
		0xf0,0x88,0x88,0xf0,0x80,0x80,0x80, // 80
		0x70,0x88,0x88,0x88,0xa8,0x90,0x68, // 81
		0xf0,0x88,0x88,0xf0,0xa0,0x90,0x88, // 82
		0x70,0x88,0x80,0x70,0x08,0x88,0x70, // 83
		0xf8,0xa8,0x20,0x20,0x20,0x20,0x20, // 84
		0x88,0x88,0x88,0x88,0x88,0x88,0x70, // 85
		0x88,0x88,0x88,0x88,0x88,0x50,0x20, // 86
		0x88,0x88,0x88,0xa8,0xa8,0xa8,0x50, // 87
		0x88,0x88,0x50,0x20,0x50,0x88,0x88, // 88
		0x88,0x88,0x50,0x20,0x20,0x20,0x20, // 89
		0xf8,0x08,0x10,0x70,0x40,0x80,0xf8, // 90
		0x78,0x40,0x40,0x40,0x40,0x40,0x78, // 91
		0x00,0x80,0x40,0x20,0x10,0x08,0x00, // 92
		0x78,0x08,0x08,0x08,0x08,0x08,0x78, // 93
		0x20,0x50,0x88,0x00,0x00,0x00,0x00, // 94
		0x00,0x00,0x00,0x00,0x00,0x00,0xf8, // 95
		0x60,0x60,0x20,0x10,0x00,0x00,0x00, // 96
		0x00,0x00,0x60,0x10,0x70,0x90,0x78, // 97
		0x80,0x80,0xb0,0xc8,0x88,0xc8,0xb0, // 98
		0x00,0x00,0x70,0x88,0x80,0x88,0x70, // 99
		0x08,0x08,0x68,0x98,0x88,0x98,0x68, // 100
		0x00,0x00,0x70,0x88,0xf8,0x80,0x70, // 101
		0x10,0x28,0x20,0x70,0x20,0x20,0x20, // 102
		0x00,0x00,0x70,0x98,0x98,0x68,0x08, // 103
		0x80,0x80,0xb0,0xc8,0x88,0x88,0x88, // 104
		0x20,0x00,0x60,0x20,0x20,0x20,0x70, // 105
		0x10,0x00,0x10,0x10,0x10,0x90,0x60, // 106
		0x80,0x80,0x90,0xa0,0xc0,0xa0,0x90, // 107
		0x60,0x20,0x20,0x20,0x20,0x20,0x70, // 108
		0x00,0x00,0xd0,0xa8,0xa8,0xa8,0xa8, // 109
		0x00,0x00,0xb0,0xc8,0x88,0x88,0x88, // 110
		0x00,0x00,0x70,0x88,0x88,0x88,0x70, // 111
		0x00,0x00,0xb0,0xc8,0xc8,0xb0,0x80, // 112
		0x00,0x00,0x68,0x98,0x98,0x68,0x08, // 113
		0x00,0x00,0xb0,0xc8,0x80,0x80,0x80, // 114
		0x00,0x00,0x78,0x80,0x70,0x08,0xf0, // 115
		0x20,0x20,0xf8,0x20,0x20,0x28,0x10, // 116
		0x00,0x00,0x88,0x88,0x88,0x98,0x68, // 117
		0x00,0x00,0x88,0x88,0x88,0x50,0x20, // 118
		0x00,0x00,0x88,0x88,0xa8,0xa8,0x50, // 119
		0x00,0x00,0x88,0x50,0x20,0x50,0x88, // 120
		0x00,0x00,0x88,0x88,0x78,0x08,0x88, // 121
		0x00,0x00,0xf8,0x10,0x20,0x40,0xf8, // 122
		0x10,0x20,0x20,0x40,0x20,0x20,0x10, // 123
		0x20,0x20,0x20,0x00,0x20,0x20,0x20, // 124
		0x40,0x20,0x20,0x10,0x20,0x20,0x40, // 125
		0x40,0xa8,0x10,0x00,0x00,0x00,0x00, // 126
		0x20,0x70,0xd8,0x88,0x88,0xf8,0x00, // 127
	};
}
