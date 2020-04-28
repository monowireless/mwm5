// PS2Keyboard API names for coding aid.
typedef struct {} PS2Keymap_t;
extern const PS2Keymap_t PS2Keymap_US;
extern const PS2Keymap_t PS2Keymap_JP;

struct PS2Keyboard {
	void begin(uint8_t, uint8_t, const PS2Keymap_t&);
	bool available();
	int read();
};
