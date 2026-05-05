#include <tonc.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include "visual.h"
#include "gapbuf.h"
#include "printdebug.h"

line init_line(){
	line retval;
	for(int i=0;i<LINE_LENGTH;i++){
		retval.data[i] = 0;
	}
	retval.start = 0;
	retval.has_newline = 0;
	retval.size = 0;
	retval.dirty = 0;

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
	print("POS BEFORE: %d",pos);
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
		print("cur.size: %d",cur.size);
		pos += (cur.size - xx);
		print("pos += (cur.size - xx);: %d",pos);
		if (cur.has_newline) {
			//skip the newline
			pos++;
		}
		if (xx >= next.size) {
			pos += next.size;
			print("pos += next.size;: %d",pos);
			v->cursor.x = next.size;
		}
		else {
			pos += xx;
			print("pos += xx;: %d",pos);
			v->cursor.x = xx;
		}
		move_gap_to(gb,pos);
		v->cursor.y++;
	}

}

void empty_visual(visual *v) {
	memset(v->screen, 0, sizeof(line) * LINE_COUNT);
}
int line_changed(line *before, line *now) {
	if (before->size != now->size) {
		return 1;
	}
 	if (before->has_newline != now->has_newline) {
		return 1;
	}
	if (before->start != now->start) {
		return 1;
	}
	for (int i =0;i<before->size;i++) {
		if (before->data[i] != now->data[i]) {
			return 1;
		}
	}
	return 0;
}

void visual_update(visual *v, gapbuf *gb, int mode){
	int found_cursor = 0;
	int cur_gb_cursor = gb->cursor;
	char c;
	line before;
	line now;
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
		before = v->screen[y];
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
				now = v->screen[y];
				if (line_changed(&before,&now)) {
					v->screen[y].dirty = 1;
				}
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
		
		now = v->screen[y];
		if (line_changed(&before,&now)) {
			v->screen[y].dirty = 1;
		}
/* 		else if(y > start_y){
			//checking for an ugly edge case when the added char is \n and the lines below only has \n
			if(now.size != 0 && !now.has_newline){
				return;
			}
		} */

	}
	check_after:
	y++;
	for(;y<LINE_COUNT;y++){
		now = v->screen[y];
		if(now.size ==0 && !now.has_newline){
			//stop at first empty line
			return;
		}
		v->screen[y] = init_line();
		v->screen[y].dirty = 1;
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

void render_screen(visual *v, int screen_top){ 
	int start = screen_top/8;
	int stop = start + VISUAL_LINE_COUNT;
	for(int y = start; y < stop; y++){
		
		if(v->screen[y].dirty){

			tte_set_pos(0,y*8);
			tte_erase_line();
			tte_write(v->screen[y].data);
			v->screen[y].dirty = 0;
		}
	}
}
	
/* 
void visual_update(visual *v, gapbuf *gb){
	int cur_gb_cursor = gb->cursor;
	char c;
	line before;
	line now;
	int y = v->cursor.y;
	if (y-1 >= 0) {
		//start checking from previous line
		y--;
	}
	int count =v->screen[y].start;
	for(; y<LINE_COUNT;y++) {
		before = v->screen[y];
		v->screen[y] = init_line();
		for (int x =0; x<LINE_LENGTH;x++) {
			if (count == cur_gb_cursor) {
				v->cursor.x=x;
				v->cursor.y=y;
			}
			c = get_char_at(gb,count);
			if (c == 0) {
				v->screen[y].has_newline=0;
				now = v->screen[y];
				if (line_changed(before,now)) {
					v->screen[y].dirty = 1;
				}
				goto check_after;
			}
			if (c == '\n') {
				if (y>0 && v->screen[y-1].size==LINE_LENGTH && v->screen[y].size==0) {
					if (!v->screen[y-1].has_newline) {
						//MARK THE PREV LINE AS HAS_NEWLINE
						v->screen[y-1].has_newline=1;
						//dont store the newline in line so decrement x
						x--;
						count++;
						continue;
					}
						v->screen[y].has_newline=1;
						if (x == 0) {
							v->screen[y].start=count;
						}
						count++;
						break;
				}
				else {
					v->screen[y].has_newline=1;
					if (x == 0) {
						v->screen[y].start=count;
					}
					count++;
					break;
				}
			}
			if (x == 0) {
				v->screen[y].start=count;
			}
			count++;
			v->screen[y].data[x] = c;
			v->screen[y].size++;
		}
		now = v->screen[y];
		if (line_changed(before,now)) {
			v->screen[y].dirty = 1;
		}


	}
	check_after:
	y++;
	for(;y<LINE_COUNT;y++){
		now = v->screen[y];
		if(now.size ==0 && !now.has_newline && !now.dirty){
			return;
		}
		v->screen[y] = init_line();
		v->screen[y].dirty = 1;
	}


}
*/