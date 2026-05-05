#include <tonc.h>
#include <stdlib.h>
#include "cursor.h"
#include "gapbuf.h"
#include "printdebug.h"
#include "mail_screen.h"

cursor *create_cursor(int x, int y, OBJ_ATTR *sprite){
	
	cursor *retval = malloc(sizeof(cursor));
	
	if(retval == NULL){
		print("ERROR in create_cursor()");
		return NULL;
	}
	retval->x = x;
	retval->y = y;
	retval->sprite = sprite;
	retval->sprite_no = 0;
	retval->frame_count = 0;
	
	return retval;
}


void update_cursor_loc(cursor *c, const visual *v,const int offset){
	c->x = v->cursor.x * 8;
	c->y = (v->cursor.y * 8) - offset;	
	obj_set_pos(c->sprite,c->x,c->y);
	//print("X: %d, Y: %d", c->x, c->y);
}



void render_cursor(cursor *c){
	c->frame_count += 1;
	if(30 == c->frame_count){
		u16 new_attr2_value = ATTR2_ID(1);
		c->sprite->attr2 = (c->sprite->attr2 & ~ATTR2_ID_MASK) | new_attr2_value;
		return;
	}
	else if(60 == c->frame_count){
		c->frame_count = 0;
		u16 new_attr2_value = ATTR2_ID(c->sprite_no);
		c->sprite->attr2 = (c->sprite->attr2 & ~ATTR2_ID_MASK) | new_attr2_value;
		return;
	}

}

void keep_cursor_on(cursor *c){
	c->frame_count = 0;
	u16 new_attr2_value = ATTR2_ID(c->sprite_no);
	c->sprite->attr2 = (c->sprite->attr2 & ~ATTR2_ID_MASK) | new_attr2_value;
	return;
}

void change_cursor(cursor *c){
	switch(c->sprite_no){
		case 0:
		c->sprite_no = 2;
		break;
		case 2:
		c->sprite_no = 3;
		break;
		case 3:
		c->sprite_no = 0;
		break;
	}
}

void update_cursor_loc_mail_screen(cursor *c, const mail_screen *ms){
	text_field *active = ms->field[ms->active_field];
	int x = active->gb->cursor;
	int y = active->height;
	if(x > VISUAL_LINE_LENGTH){
		x = x % VISUAL_LINE_LENGTH;
		y ++;
	}
	x *= 8;
	y *= 8;
	c->x = x;
	c->y = y;	
	obj_set_pos(c->sprite,c->x,c->y);
}
