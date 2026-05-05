#include "gapbuf.h"
#include "mail_screen.h"
#include "visual.h"
#include <stdlib.h>
#include <stdio.h>


text_field *create_text_field(int size){
	text_field *retval = malloc(sizeof(text_field));
		if(retval== NULL){
		printf("ERROR AT create_text_field MALLOC");
	}
	retval->gb = create_gapbuf(size);
	return retval;
}



mail_screen *create_mail_screen(int size){
	mail_screen *retval = malloc(sizeof(mail_screen));
	int length = 60;
	retval->field[SUBJECT] = create_text_field(length);
	retval->field[RECIPIENT_1] = create_text_field(length);
	retval->field[RECIPIENT_2] = create_text_field(length);
	retval->field[RECIPIENT_3] = create_text_field(length);
	retval->active_field = 0;
	retval->size = size;
	return retval;
}  

void cursor_down_text_field(gapbuf *gb){
	int used_size = gb_used(gb);
	int pos = gb->cursor;
	if(pos < VISUAL_LINE_LENGTH  && used_size > VISUAL_LINE_LENGTH){
		int under_line_size = used_size - VISUAL_LINE_LENGTH;
		pos = VISUAL_LINE_LENGTH;//move to the end of first line
		pos += under_line_size;//move to the end of second line
		move_gap_to(gb,pos);
	}
	
}

void cursor_up_text_field(gapbuf *gb){
	int pos = gb->cursor;
	if (pos >= VISUAL_LINE_LENGTH) {
		pos %= VISUAL_LINE_LENGTH;
		move_gap_to(gb,pos);
	}
}



int switch_active_field(mail_screen *ms){
	int active = ms->active_field;
	if(active == ms->size-1){
		active = 0;
	}
	else{
		active++;
	}
	ms->active_field = active;
	printf("\nACTIVE text_field on mail_screen is %d\n",active);
	return active;
}
