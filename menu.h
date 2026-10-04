#ifndef MENU_H
#define MENU_H
#include "common.h"
int menu_load(void);                    /* reads menu.txt (creates a default one if missing) */
int menu_count(void);
const MenuItem *menu_get(int index);    /* NULL if index is out of range */
int menu_save(void);                    /* writes menu.txt */
int menu_set_available(int index, int available);   /* staff "Finished" button. Saves to menu.txt. 1 = ok */
#endif
