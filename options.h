#include <X11/Xlib.h>

typedef struct _keybind {
	unsigned int modifier;
	KeyCode keycode;
} keybind;
#define KEY_NUM 6
keybind b_keys[KEY_NUM];

void getKeys();
