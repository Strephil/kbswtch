#include "tablemap.h"
#include <stdlib.h>
#include <stdio.h>

tableelm *table = NULL;

int addTable(wchar_t symbol, unsigned int keycode, unsigned char groupnum,
		unsigned char state, bool solo)
{
	tableelm *T;
	keydesc *K;

	if ((K = (keydesc*)malloc(sizeof(keydesc))) == NULL) {
		fprintf (stderr, "Cannot alloc memory");
		return 1;
	};
	K->groupnum = groupnum;
	K->state = state;
	K->keycode = keycode;
	K->solo = solo;
	HASH_FIND_INT (table, &symbol, T);
	if (T == NULL) {
		if((T = malloc(sizeof(tableelm))) == NULL) {
			free(K);
			fprintf (stderr, "Cannot alloc memory");
			return 1;
		}
		T->symbol = symbol;
		T->key_num = 1;
		K->next = NULL;
		HASH_ADD_INT(table, symbol, T);
	} else {
		T->key_num++;
		K->next = T->kd;
	}
	T->kd = K;
	return 0;
};

keydesc* findTable(wchar_t symbol, unsigned int *num)
{
	tableelm *T;

	HASH_FIND_INT (table, &symbol, T);
	if(!T) return NULL;
	if(num) *num = T->key_num;
	return T->kd;
}

int deleteTable ()
{
	tableelm *T;
	keydesc *K1, *K2;

	while (table) {
		T = table;
		HASH_DEL(table, T);
		K1 = T->kd;
		while(K1) {
			K2 = K1->next;
			free(K1);
			K1 = K2;
		}
		free(T);
	}
	return 0;
}
