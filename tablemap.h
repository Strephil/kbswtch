#include "uthash.h"
#include <stdbool.h>

typedef struct _keydesc {
	unsigned char groupnum;
	bool solo;
	unsigned char state;
	unsigned int keycode;
	struct _keydesc *next;
} keydesc;

typedef struct _tableelm {
	wchar_t symbol;
	keydesc* kd;
	unsigned int key_num;

	UT_hash_handle hh;
} tableelm;

/* Function findTable returns the pointer to the keydesc list.
 * wchar_t symbol 	symbol to find
 * unsigned int *num	returns the lenghth of the list */
keydesc* findTable(wchar_t symbol, unsigned int *num);
/* Function addTable returns 0 is OK, non-zero otherwise.
 * wchar_t symbol 	symbol to add.
 * unsigned char state	set of bitmasks corresponding to the eight real
 modifiers */
int addTable(wchar_t symbol, unsigned int keycode, unsigned char groupnum,
		unsigned char state, bool solo);
int deleteTable();
