#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include "visual.h"
#include "gapbuf.h"

line init_line(){
	line retval;
	for(int i=0;i<LINE_LENGTH;i++){
		retval.data[i] = 0;
	}
	retval.start = 0;
	retval.has_newline = 0;
	retval.size = 0;

	return retval;
	
}

position init_position(){
	position retval;
	retval.x=0;
	retval.y=0;
	return retval;
}

visual *create_visual(){
	visual *retval = malloc(sizeof(visual));
 	for(int i =0;i<LINE_COUNT;i++){
		retval->screen[i]=init_line();
	}
	retval->cursor = init_position();
	retval->gb_pos = 0;
	return retval;
	
}
void cursor_up(visual *v, gapbuf *gb) {
	if (v->cursor.y == 0) {
		return;
	}
	line prev = v->screen[v->cursor.y-1];
	line cur = v->screen[v->cursor.y];
	if (prev.size == 0 && !prev.has_newline ) {
		return;
	}
	int pos = gb->cursor;
	int xx = v->cursor.x;
	if (prev.size >= cur.size) {
		//x position will not change
		pos -= xx+(prev.size - xx);
		if (prev.has_newline) {
			//skip the newline
			pos--;
		}
		move_gap_to(gb,pos);
		v->cursor.y--;
	}
	else {
		pos -= xx;
		if(prev.has_newline){
			pos--;
		}
		if(xx >= prev.size){
			v->cursor.x = prev.size;
		}
		else{
			pos -= (prev.size - xx);
			v->cursor.x = xx;
		}
		move_gap_to(gb,pos);
		v->cursor.y--;
	}

}
void cursor_down(visual *v, gapbuf *gb) {
	if (v->cursor.y == LINE_COUNT-1) {
		return;
	}
	line next = v->screen[v->cursor.y+1];
	line cur = v->screen[v->cursor.y];
	if (next.size == 0 && !cur.has_newline) {
		return;
	}
	volatile int pos = gb->cursor;
	int xx = v->cursor.x;
	if (next.size >= cur.size) {
		//x position will not change
		pos += cur.size ;
		if (cur.has_newline) {
			//skip the newline
			pos++;
		}
		move_gap_to(gb,pos);
		v->cursor.y++;
	}
	else {
		pos += (cur.size - xx);
		if (cur.has_newline) {
			//skip the newline
			pos++;
		}
		if (xx >= next.size) {
			pos += next.size;
			v->cursor.x = next.size;
		}
		else {
			pos += xx;
			v->cursor.x = xx;
		}
		move_gap_to(gb,pos);
		v->cursor.y++;
	}

}

void empty_visual(visual *v) {
	memset(v->screen, 0, sizeof(line) * LINE_COUNT);
}

void visual_update(visual *v, gapbuf *gb, int mode){
	int cur_gb_cursor = gb->cursor;
	char c;
	int found_cursor = 0;
	int y = v->cursor.y;
	//start_y is for checking if we moved passed the cursor in the loop
	//int start_y = y;
	
	//try to start from the previous line if possible
	if (y > 0) {
		y--;
	}
	int count = v->screen[y].start;
	/* int count = 0;
	y= 0; */
	for(; y<LINE_COUNT;y++) {
		v->screen[y] = init_line();
		v->screen[y].start=count;
		for (int x =0; x<LINE_LENGTH;x++) {
			if (count == cur_gb_cursor) {
				v->gb_pos = count;
				found_cursor = 1;
				if(x == VISUAL_LINE_LENGTH){
					v->cursor.x = 0;
					v->cursor.y = y+1;
				}
				else{
				v->cursor.x = x;
				v->cursor.y = y;
				}

			}
			c = get_char_at(gb,count);
			if (c == 0) {
				v->screen[y].has_newline=0;
				//we have reached the end check for remaining lines that needs to be emptied if it was a deletion operation
				if (DELETE == mode) {
					goto check_after;
				}
				return;
			}
			if (c == '\n') {
				v->screen[y].has_newline=1;
				count++;
				break;
			}

			if (x < VISUAL_LINE_LENGTH) {

				count++;
				v->screen[y].data[x] = c;
				v->screen[y].size++;
			}
		}
		if(mode == CURSOR_MOV && found_cursor == 1){return;}
	}
	check_after:
	line now;
	y++;
	for(;y<LINE_COUNT;y++){
		now = v->screen[y];
		if(now.size ==0 && !now.has_newline){
			//stop at first empty line
			return;
		}
		v->screen[y] = init_line();
	}
}
void put_cursor_at_end(visual *v, gapbuf *gb){
	int last_line = 0;
	int pos;
	for(int i = 0; i<LINE_COUNT; i++){
		if(v->screen[i].size > 0 || v->screen[i].has_newline){
			last_line = i;
		}
		else{
			break;
		}
	}
	v->cursor.y = last_line;
	pos = (v->screen[last_line].start + v->screen[last_line].size);
	v->cursor.x = v->screen[last_line].size;
	if(v->screen[last_line].has_newline){
		v->cursor.x++;
		pos++;
	}
	move_gap_to(gb,pos);
}

