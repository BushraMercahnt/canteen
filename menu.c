/* menu.c - the food list, loaded from menu.txt. Owner: Partner A
 * File format, one item per line:   name|price|available|veg
 *   available: 1 = can be ordered, 0 = finished      veg: 1 = veg, 0 = non-veg   */
#include "menu.h"

static MenuItem items[MAX_MENU_ITEMS];
static int item_count = 0;

static void write_default(void)
{
    FILE *f = fopen("menu.txt", "w");
    if (!f) return;
    fputs("Samosa|15|1|1\nVeg Sandwich|40|1|1\nMasala Dosa|50|1|1\n"
          "Tea|10|1|1\nVeg Thali|70|1|1\nEgg Roll|40|1|0\n", f);
    fclose(f);
}

int menu_load(void)
{
    FILE *f = fopen("menu.txt", "r");
    if (!f) { write_default(); f = fopen("menu.txt", "r"); }
    if (!f) return 0;

    char line[128];
    item_count = 0;
    while (item_count < MAX_MENU_ITEMS && fgets(line, sizeof line, f)) {
        char name[NAME_LEN];
        int price, avail, veg = 1;
        /* %39[^|] reads up to 39 characters that are not '|' */
        int n = sscanf(line, "%39[^|]|%d|%d|%d", name, &price, &avail, &veg);
        if (n >= 3) {                       /* old files without the veg column count as veg */
            snprintf(items[item_count].name, NAME_LEN, "%s", name);
            items[item_count].price = price;
            items[item_count].available = avail;
            items[item_count].veg = (n == 4) ? veg : 1;
            item_count++;
        }
    }
    fclose(f);
    return item_count;
}

int menu_count(void) { return item_count; }

const MenuItem *menu_get(int index)
{
    if (index < 0 || index >= item_count) return NULL;
    return &items[index];
}

int menu_save(void)
{
    FILE *f = fopen("menu.txt", "w");
    if (!f) return 0;
    for (int i = 0; i < item_count; i++)
        fprintf(f, "%s|%d|%d|%d\n", items[i].name, items[i].price, items[i].available, items[i].veg);
    fclose(f);
    return 1;
}

int menu_set_available(int index, int available)
{
    if (index < 0 || index >= item_count) return 0;
    items[index].available = available ? 1 : 0;
    return menu_save();
}
