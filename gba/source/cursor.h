#ifndef CURSOR_H
#define CURSOR_H

#include <tonc.h>

#include "visual.h"

#include "mail_screen.h"


typedef struct{
	int x;
	int y;
	OBJ_ATTR *sprite;
	int sprite_no;
	int frame_count;
}ALIGN(4)cursor;


cursor *create_cursor(int x, int y, OBJ_ATTR *sprite);

void update_cursor_loc(cursor *c, const visual *v, const int offset);
void render_cursor(cursor *c);
void keep_cursor_on(cursor *c);
void change_cursor(cursor *c);


void update_cursor_loc_mail_screen(cursor *c, const mail_screen *ms);


#endif //CURSOR_H