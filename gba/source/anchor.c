#include <tonc.h>
#include "anchor.h"
#include <tonc.h>
#include <stdlib.h>
#include "anchor.h"
#include "cursor.h"
#include "gapbuf.h"
#include "printdebug.h"
anchor *create_anchor(int x, int y, OBJ_ATTR *sprite){
	
	anchor *retval = malloc(sizeof(anchor));
	
	if(retval == NULL){
		print("ERROR in create_anchor()");
		return NULL;
	}
	retval->x = x;
	retval->y = y;
	retval->sprite = sprite;
	retval->gb_pos = 0;
	retval->is_visible = 0;
	retval->visual_y = 0;
	retval->active = 0;
	
	return retval;
}



void put_anchor_on_cursor(const gapbuf * gb,const visual *v, const cursor *c, anchor *a){
	a->x = c->x;
	a->y = c->y;
	obj_set_pos(a->sprite,a->x,a->y);
	a->visual_y = v->cursor.y;
	a->gb_pos = gb->cursor;
	a->active = ON;
}
void put_anchor_at_start(anchor *a){
	a->x = 0;
	a->y = 0;
	obj_set_pos(a->sprite,a->x,a->y);
	a->visual_y = 0;
	a->gb_pos = 0;
	a->active = ON;
}

void set_anchor_visibility(anchor *a, const int i){
	if(i == ON){
		u16 new_attr2_value = ATTR2_ID(4);
		a->sprite->attr2 = (a->sprite->attr2 & ~ATTR2_ID_MASK) | new_attr2_value;
		a->is_visible = 1;
		return;
	}
	if(i == OFF){
		u16 new_attr2_value = ATTR2_ID(5);
		a->sprite->attr2 = (a->sprite->attr2 & ~ATTR2_ID_MASK) | new_attr2_value;
		a->is_visible = 0;
		return;
	}
}

void update_anchor(anchor *a, int offset){
	if(OFF == a->active){return;}
	int top_limit = offset / 8;
	int bottom_limit = top_limit + VISUAL_LINE_COUNT;
	if(a->visual_y >= top_limit && a->visual_y <= bottom_limit){
		if(!a->is_visible){
			set_anchor_visibility(a , ON);
		}
	}
	else{
		if(a->is_visible){
		set_anchor_visibility(a , OFF);
		}
	}
	a->y = ((a->visual_y - top_limit)*8);
	obj_set_pos(a->sprite,a->x,a->y);
}