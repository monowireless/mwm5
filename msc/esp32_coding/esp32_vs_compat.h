#pragma once

/* this file is just for used for hinting for code editor (like VS). */

#if defined(_MSC_VER) && defined(ESP32)
#include <cstdint>

// ROM STORAGE
#define PROGMEM

// COMMON
#define HIGH 0x1
#define LOW  0x0
#define INPUT 0x0
#define OUTPUT 0x1
#define INPUT_PULLUP 0x2
int digitalRead(uint8_t pin);
void digitalWrite(uint8_t pin, uint8_t c);
void pinMode(uint8_t pin, uint8_t mode);
void delay(int);

class String {
public:
	operator const char*();
	const char* c_str();
};

class ToString {
public:
	String& toString();
};

class TwoWire {
public:
	uint8_t requestFrom(uint8_t, uint8_t);
	int available(void);
	void begin();
	int read(void);
};
extern TwoWire Wire;

// system tick timer
extern unsigned long millis();

class HardwareSerial {
public:
	void begin(...);
	unsigned char read();
	void write(unsigned char c);
	bool available();
	void printf(const char*, ...);
	void setRxBufferSize(size_t);
	void flush();
};
#define SERIAL_8N1 0x06
extern HardwareSerial Serial, Serial1, Serial2;

#define BLACK               0x0000      /*   0,   0,   0 */
#define NAVY                0x000F      /*   0,   0, 128 */
#define DARKGREEN           0x03E0      /*   0, 128,   0 */
#define DARKCYAN            0x03EF      /*   0, 128, 128 */
#define MAROON              0x7800      /* 128,   0,   0 */
#define PURPLE              0x780F      /* 128,   0, 128 */
#define OLIVE               0x7BE0      /* 128, 128,   0 */
#define LIGHTGREY           0xC618      /* 192, 192, 192 */
#define DARKGREY            0x7BEF      /* 128, 128, 128 */
#define BLUE                0x001F      /*   0,   0, 255 */
#define GREEN               0x07E0      /*   0, 255,   0 */
#define CYAN                0x07FF      /*   0, 255, 255 */
#define RED                 0xF800      /* 255,   0,   0 */
#define MAGENTA             0xF81F      /* 255,   0, 255 */
#define YELLOW              0xFFE0      /* 255, 255,   0 */
#define WHITE               0xFFFF      /* 255, 255, 255 */
#define ORANGE              0xFD20      /* 255, 165,   0 */
#define GREENYELLOW         0xAFE5      /* 173, 255,  47 */
#define PINK                0xF81F

// M5 M5.lcd Object, etc..
class _M5_LCD {
public:
	_M5_LCD() {}
	void setCursor(uint16_t x, uint16_t y);
	void setTextDatum(uint8_t datum);
	void setTextSize(uint8_t size);
	void setTextColor(uint16_t color, uint16_t bgcolor = 0);
	void setTextFont(uint8_t f);
	void drawChar(int32_t x, int32_t y, uint16_t c, uint32_t color, uint32_t bg, uint8_t size);
	void fillRect(int16_t x, int16_t y, int16_t w, int16_t h, uint16_t color = 0xFFFF);
	void drawString(const char* string, int32_t poX, int32_t poY, uint8_t font);
	void fillRect(int16_t x, int16_t y, int16_t w, int16_t h, uint16_t color = 0);
	void fillScreen(uint16_t color);
	void print(const char*);
	void print(int);
	void println(const char*);
	void println(int);
	void clear(uint16_t color);
	void setFreeFont(uint16_t);
	int16_t fontHeight(int16_t font);
	int16_t textWidth(const char* string, uint8_t font);
	uint16_t color565(uint8_t red, uint8_t green, uint8_t blue);
	void startWrite();
	void endWrite();
	void setWindow(int, int, int, int);
	uint16_t decodeUTF8(uint8_t* buf, uint16_t* index, uint16_t remaining);
	uint16_t decodeUTF8(uint8_t c);

	int16_t height();
	int16_t width();
};
#define tft_Write_16(C) ;

class _M5_BTN {
public:
	_M5_BTN() {}
	uint8_t wasReleased();
	uint8_t wasReleasefor(uint16_t);
	
};
class _M5_POWER {
public:
	void begin();
};
class M5Stack { // M5Stack class
public:
	M5Stack() : Lcd(), BtnA(), BtnB(), BtnC() {}
	void begin(bool a = true, bool b = true, bool c = true, bool d = true);
	void update();
	void powerOFF();
	_M5_POWER Power;
	_M5_LCD Lcd;
	_M5_BTN BtnA, BtnB, BtnC;
};


extern M5Stack M5;

class IPAddress : public ToString {
public:
};

const uint8_t WIFI_STA = 1;
const uint8_t WIFI_AP_STA = 2;
const uint8_t WIFI_OFF = 0;
const uint8_t WL_CONNECTED = 0;

class M5WiFi {
public:
	void mode(uint8_t);  // WIFI_AP, WIFI_STA, WIFI_AP_STA or WIFI_OFF
	void begin(const char *, const char *);
	IPAddress& localIP();
	uint8_t status();
};
extern M5WiFi WiFi;

// wifi
class WiFiUDP {
public:
	void begin(uint16_t port = 0);
	int parsePacket();
	int read(char *, int);
};

// EEPROM
class M5EEPROM {
public:
	void begin(size_t size);
	void end();
	uint8_t read(int const address);
	void write(int const address, uint8_t const value);
	bool commit();
};
extern M5EEPROM EEPROM;

// File structure
#define FILE_READ 0
#define FILE_WRITE 1
#define FILE_APPEND 2
struct File {
	operator bool();
	bool isDirectory();
	File openNextFile(uint8_t mode = FILE_READ);
	void close();
	const char* name();
	uint32_t size();
	int read();
	int read(void*, uint16_t);
	void seek(uint32_t, uint32_t = 0);
	size_t write(const uint8_t*, size_t);
	size_t write(uint8_t);
	int available();
};

namespace fs {
	struct FS {
		File open(const char*, int mode = FILE_READ);
	};

}
extern fs::FS SD; // SD cards

#endif // ESP32 && _MSC_VER