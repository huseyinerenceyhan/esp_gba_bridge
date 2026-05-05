#ifndef ANCHOR_H
#define ANCHOR_H
#include "cursor.h"
#include "gapbuf.h"
#include "visual.h"

#define ON 1
#define OFF 0



typedef struct{
	int x;
	int y;
	OBJ_ATTR *sprite;
	int is_visible;
	int gb_pos;
	int visual_y;
	int active;
}ALIGN(4)anchor;

anchor *create_anchor(int x, int y, OBJ_ATTR *sprite);
void put_anchor_on_cursor(const gapbuf *gb, const visual *v, const cursor *c, anchor *a);
void put_anchor_at_start(anchor *a);
void set_anchor_visibility(anchor *a, const int i);

void update_anchor(anchor *a, int offset);

#endif//ANCHOR_H	