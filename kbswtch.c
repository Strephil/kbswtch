#include <X11/Xlib.h>
#include <X11/Xatom.h>
#include <X11/keysym.h>
#include <X11/XKBlib.h>
#include <stdio.h>
#include <locale.h>
#include <wchar.h>
#include <stdlib.h>
#include <stdbool.h>
#include "tablemap.h"
#include "options.h"

#define XKB_CTRLS_MASK (XkbAllControlsMask & ~(XkbInternalModsMask | XkbIgnoreLockModsMask))

int getSelection(Display * display, const wchar_t **data, Window win);
int sendKey(Display * display, Window owner, unsigned int state,
	    KeyCode keycode);
int createKeyMap(Display * display);
int getKeyCode(wchar_t symbol, unsigned char group, unsigned char *state,  KeyCode *keycode);
int getSelectionGroup(const wchar_t *data, unsigned char *group);
int dataToW(unsigned char *data, const wchar_t **wstring);
int stateTransform(unsigned int* ret, unsigned char group, unsigned char
		state);
void eventLoop();
int kbswtch(char dest_group);
void grabKeys();
void ungrabKeys();
void getNumlock();

Window win;
Display *display;
Window owner;
unsigned int numlock_mask;

int main(int argc, char *argv[])
{
	int ret = 0;

	setlocale (LC_ALL, "");
	if ((display = XOpenDisplay(NULL)) == NULL) {
		printf("%s\n", "Can't connect to display");
		exit(1);
	}
	if (createKeyMap(display)) {
		printf("%s\n", "Can't create Key-hash-table");
		ret = 1;
	} else {
		win = XCreateSimpleWindow(display, DefaultRootWindow(display),
				  0, 0, 1, 1, 0, 0, 0);
		getKeys();
		getNumlock();
		grabKeys();
		eventLoop();
		XDestroyWindow(display, win);
		ungrabKeys();
	};
	
	deleteTable();
	XCloseDisplay(display);
	return ret;
}

void eventLoop()
{
	XEvent report;
	char dest_group = -1;
	bool quit = false;
	KeyCode keycode;
	int i;

	XAllowEvents(display, AsyncBoth, CurrentTime);
	XSelectInput(display, DefaultRootWindow(display), KeyPressMask);
	while (!quit) {
		XNextEvent (display, &report);
		switch (report.type) {
		case KeyPress:
			keycode = report.xkey.keycode;
			for (i = 0; i < 4; i++)
				if(keycode == b_keys[i].keycode) dest_group =
					i;
			if(keycode == b_keys[4].keycode) dest_group = -1; 
			if(keycode == b_keys[5].keycode) quit = true;
			if(!quit) kbswtch(dest_group);
		}
	}
}

int kbswtch(char dest_group)
{
	unsigned char state, group;
	const wchar_t *data;
	unsigned int dest_state;
	KeyCode keycode;
	int i;
	int ret = 0;

	if((owner = XGetSelectionOwner(display, XA_PRIMARY)) == None) {
		printf("%s\n", "Can not get selection owner");
		ret = 1;
	} else if(getSelection(display, &data, win)) {
		ret = 1;
	} else if (getSelectionGroup(data, &group)) {
		printf("%s\n", "Can not get selection group");
		ret = 1;
	} else {
		if (dest_group == -1)
			dest_group = group ^ 1;
		for (i = 0; i < wcslen(data); i++) {
			if(getKeyCode(data[i], group, &state, &keycode) ==
					0) {
				stateTransform(&dest_state, dest_group, state);
				if(state != -1) sendKey(display, owner, dest_state, keycode);
			}
		}
	}

	return ret;
}

int getSelection(Display * display, const wchar_t **data, Window win)
{
	XEvent evt;
	Atom type;
	Atom utf8String;
	Atom pty;
	int format, result;
	unsigned long bytes_left, len, dummy;
	unsigned char *raw_data;
	int ret = 0;


	utf8String = XInternAtom(display, "UTF8_STRING", False);
	pty = XInternAtom(display, "SEL_TEST", False);

	XConvertSelection(display,
			  XA_PRIMARY, utf8String, pty, win, CurrentTime);
	XNextEvent(display, &evt);

	if (evt.type != SelectionNotify)
		return 1;
	if (evt.xselection.property == None)
		return 1;

	XGetWindowProperty(display, win, pty,
			   0, 0,
			   0,
			   AnyPropertyType,
			   &type, &format, &len, &bytes_left, &raw_data);
	XFree(raw_data);
	if (format != 8)
		return 1;
	result = XGetWindowProperty(display, win, pty,
				    0, bytes_left, 0, AnyPropertyType,
				    &type, &format, &len, &dummy, &raw_data);

	if (result == Success) {
		dataToW(raw_data, data);
		ret = 0;
	} else {
		ret = 1;
	}
	XFree(raw_data);
	return ret;
}

int sendKey(Display * display, Window owner, unsigned int state,
	    KeyCode keycode)
{
	XKeyEvent xkevent;

	xkevent.window = owner;
	xkevent.keycode = keycode;
	xkevent.subwindow = None;
	xkevent.time = CurrentTime;
	xkevent.same_screen = True;
	xkevent.type = KeyPress;
	xkevent.state = state;
	xkevent.display = display;
	xkevent.x = xkevent.y = xkevent.x_root = xkevent.y_root = 1;

	XSendEvent(display, owner, True, KeyPressMask, (XEvent *) & xkevent);

	return 0;
}

int getKeyCode (wchar_t symbol, unsigned char group, unsigned char *state, KeyCode *keycode)
{
	keydesc *K, *I;

	K = findTable(symbol, NULL);
	
	for (I = K; I; I = I->next)
		if(I->groupnum == group) goto found;

	for (I = K; I; I = I->next)
		if(I->solo) goto found;

	return 1;
found:
	*keycode = I->keycode;
	*state = I->state;

	return 0;
}
	
int dataToW(unsigned char *data, const wchar_t **wstring)
{
	wchar_t *W;
	W = calloc (strlen(data), sizeof(wchar_t));
	if (!W) {
		fprintf(stderr,"Can not alloc memory!\n");
		return 1;
	}
	mbstowcs(W,data,strlen(data));
	*wstring = W;
	return 0;
}

int getSelectionGroup(const wchar_t *data, unsigned char *group)
{
	const wchar_t *W;
	keydesc* K;
	unsigned int num;

	W = data;
	while (W) {
		K = findTable (*W, &num);
		if (K) {
			if (num == 1) {
				*group = K->groupnum;
				return 0;
			}
		}
		W++;
	} 

	return 1;
};

int createKeyMap(Display *display)
{
	XkbDescPtr xdp;
	int keycode; 
	unsigned char group, state;
	unsigned char shift;
	unsigned char j;
	KeySym keysym;
	char buffer[8];
	wchar_t symbol;
#ifndef cKMMask
#define cKMMask XkbKeySymsMask | XkbKeyTypesMask
#else
	unsigned int cKMMask = XkbKeySymsMask | XkbKeyTypesMask;
#endif
	unsigned char key_type_index;
	XkbKeyTypeRec key_type;
	bool solo;
	int ret = 0;

	xdp = XkbGetMap(display, cKMMask, XkbUseCoreKbd);
	if (xdp == NULL) {
		fprintf (stderr, "Can not get keyboard mapping\n");
		ret = 1;
	}
	else {
		for ( keycode = xdp->min_key_code; keycode <= xdp->max_key_code;
				keycode++ ) {
			solo = (XkbKeyNumGroups(xdp,keycode) == 1) ? true :
				false;
			for (group = 0; group < XkbKeyNumGroups(xdp,keycode); group++) {
				key_type_index =
					xdp->map->key_sym_map[keycode].kt_index[group];
				if(key_type_index == XkbKeypadIndex) continue;

				key_type =
					xdp->map->types[key_type_index];
				for (shift = 0; shift < key_type.num_levels; shift++)
				{
					keysym = XkbKeySymEntry(xdp, keycode, shift,
							group);
					if(!XkbTranslateKeySym(display, &keysym, 0,
							buffer,
							sizeof(buffer), NULL))
						{
						continue;
					}
					mbstowcs (&symbol,buffer,strlen(buffer));
					state = 0;
					for (j = 0; j < key_type.map_count; j++)
					{
						if(!key_type.map[j].active || (shift !=
									key_type.map[j].level)) continue;
						state = key_type.map[j].mods.mask;
						break;
					}
					if(symbol == 0xd) symbol = 0xa;
					if(addTable(symbol, keycode, group,
								state, solo)) {
						ret = 1;
						goto the_end;
					};
				}
			}
		}
	}

the_end:
	XkbFreeClientMap(xdp, cKMMask, True);			
	return ret;
}

int stateTransform(unsigned int* ret, unsigned char group, unsigned char
		state)
{
	if(group > 3)
		return 1;

	*ret = (group << 13) + state;

	return 0;
}

void grabKeys()
{
	int i = 0;

	for (i = 0; i < KEY_NUM; i++)
	{
		if(b_keys[i].keycode) {
			XGrabKey(display, b_keys[i].keycode, b_keys[i].modifier, DefaultRootWindow(display), False, GrabModeAsync,
			GrabModeAsync);
			XGrabKey(display, b_keys[i].keycode,
					b_keys[i].modifier|numlock_mask, DefaultRootWindow(display), False, GrabModeAsync,
			GrabModeAsync);
		}
	}
}
 void ungrabKeys()
{
	int i = 0;

	for (i = 0; i > 4; i++)
	{
		if(b_keys[i].keycode) {
			XGrabKey(display, b_keys[i].keycode, b_keys[i].modifier, DefaultRootWindow(display), False, GrabModeAsync,
			GrabModeAsync);
		}
	}
}

void getNumlock()
{
	KeyCode nlock;
	int i;
	XModifierKeymap *modmap;

	static int mask_table[8] = {
		ShiftMask, LockMask, ControlMask, Mod1Mask,
		Mod2Mask, Mod3Mask, Mod4Mask, Mod5Mask
	};

	nlock = XKeysymToKeycode(display, XK_Num_Lock);
	if (nlock) {
		modmap = XGetModifierMapping(display);
		if (modmap != NULL && modmap->max_keypermod > 0)
		{
			for (i = 0; i < 8 * modmap->max_keypermod; i++)
			{
				if (modmap->modifiermap[i] == nlock)
					numlock_mask = mask_table[i / modmap->max_keypermod];
			}
		}
		if (modmap)
		    XFreeModifiermap (modmap);
	}
}
