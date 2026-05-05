#include "text_editor.h"
#include "smtp_client.h"
#include "my_globals.h"
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <stdio.h>
#include "esp_timer.h"


double editor_time;
uint64_t count;


void handle_data_select(char data,text_editor *te){
    int prev_line;
	int next_line;
	int pos;
	if(data == 0) return;
	int mode = -1;
	te->prev_state = SELECT;
	switch (data) {
		case 204:// DELETE
		case 8: // BACKSPACE
			select_delete(te->gb, te->anchor);
			select_to_normal(te);
			if(te->anchor_y < te->v->cursor.y){
				te->v->cursor.y = te->anchor_y;
			}
			mode = DELETE;
			break;

		case 200: // LEFT ARROW
			cursor_left(te->gb);
			select_to_normal(te);
			mode = CURSOR_MOV;
			break;

		case 202: // RIGHT ARROW
			cursor_right(te->gb);
			select_to_normal(te);
			mode = CURSOR_MOV;
			break;

		case 203: // DOWN ARROW
			cursor_down(te->v, te->gb);
			select_to_normal(te);
			mode = CURSOR_MOV;
			break;

		case 201: // UP ARROW
			cursor_up(te->v, te->gb);
			select_to_normal(te);
			mode = CURSOR_MOV;
			break;
			
		case 205: // HOME
			mode = CURSOR_MOV;
			select_to_normal(te);
			prev_line = te->v->cursor.y-1;
			if(te->v->cursor.x == 0 && prev_line >= 0){
				if(!(te->v->screen[prev_line].has_newline)){
					cursor_up(te->v, te->gb);
					break;
				}
			}
			move_gap_to(te->gb,te->v->screen[te->v->cursor.y].start);
			break;
		case 215: // SHIFT + HOME
			mode = CURSOR_MOV;
			prev_line = te->v->cursor.y-1;
			if(te->v->cursor.x == 0 && prev_line >= 0){
				if(!(te->v->screen[prev_line].has_newline)){
					cursor_up(te->v, te->gb);
					break;
				}
			}
			move_gap_to(te->gb,te->v->screen[te->v->cursor.y].start);
			break;
		case 206: // END
			mode = CURSOR_MOV;
			select_to_normal(te);
			next_line = te->v->cursor.y+1;
			if(te->v->cursor.x == te->v->screen[te->v->cursor.y].size && next_line <= LINE_COUNT){
				if(!(te->v->screen[te->v->cursor.y].has_newline)){
					cursor_down(te->v, te->gb);
					break;
				}
			}
			pos = te->v->screen[next_line-1].start + te->v->screen[next_line-1].size;
			if(te->v->screen[next_line-1].has_newline){
				pos++;
			}
			move_gap_to(te->gb,pos);
			break;
		case 216: // SHIFT + END
			mode = CURSOR_MOV;
			next_line = te->v->cursor.y+1;
			if(te->v->cursor.x == te->v->screen[te->v->cursor.y].size && next_line <= LINE_COUNT){
				if(!(te->v->screen[te->v->cursor.y].has_newline)){
					cursor_down(te->v, te->gb);
					break;
				}
			}
			pos = te->v->screen[next_line-1].start + te->v->screen[next_line-1].size;
			if(te->v->screen[next_line-1].has_newline){
				pos++;
			}
			move_gap_to(te->gb,pos);
			break;

		case 212: // SHIFT + RIGHT ARROW
			cursor_right(te->gb);
			mode = CURSOR_MOV;
			break;
		case 210: // SHIFT + LEFT ARROW
			cursor_left(te->gb);
			mode = CURSOR_MOV;
			break;
		case 213: // SHIFT + DOWN ARROW
			cursor_down(te->v,te->gb);
			mode = CURSOR_MOV;
			break;
		case 211: // SHIFT + UP ARROW
			cursor_up(te->v,te->gb);
			mode = CURSOR_MOV;
			break;
		case 221: //	TAB 221
			break;
		case 230: // CTRL + C
			extract_text_between_pos(te->gb, te->anchor, te->v->gb_pos);
			select_to_normal(te);
			mode = CURSOR_MOV;
			break;
		case 231: // CTRL + X
			extract_text_between_pos(te->gb, te->anchor, te->v->gb_pos);
			select_delete(te->gb, te->anchor);
			select_to_normal(te);
			if(te->anchor_y < te->v->cursor.y){
				te->v->cursor.y = te->anchor_y;
			}
			mode = DELETE;
			break;
		case 232: // CTRL + V
			select_delete(te->gb, te->anchor);
			if(te->anchor_y < te->v->cursor.y){
				te->v->cursor.y = te->anchor_y;
			}
			paste(te->gb);
			select_to_normal(te);
			mode = DELETE;
			break;
		case 233: // CTRL + A
			put_anchor_at_start(te);
			put_cursor_at_end(te->v,	te->gb);
			mode = CURSOR_MOV;
			break;
		case 250: // INSERT
		case 253: // NUMLOCK
			break;
		case 251: // PRINTSCREEN
			editor_to_mail(te);
        break;
		default:
			select_delete(te->gb, te->anchor);
			select_to_normal(te);
			if(te->anchor_y < te->v->cursor.y){
				te->v->cursor.y = te->anchor_y;
			}
			insert(te->gb, data);
			mode = DELETE;
			break;
	}
	if(mode == -1){
		return;
	}
	visual_update(te->v,te->gb,mode);

}
void handle_data_normal(char data, text_editor *te){
    int prev_line;
	int next_line;
	int pos;
	if(data == 0) return;
	int mode = -1;
	te->prev_state = NORMAL;
	switch (data) {
		case 8: // BACKSPACE
			backspace(te->gb);
			mode = DELETE;
			break;

		case 200: // LEFT ARROW
			cursor_left(te->gb);
			mode = CURSOR_MOV;
			break;

		case 202: // RIGHT ARROW
			cursor_right(te->gb);
			mode = CURSOR_MOV;
			break;

		case 203: // DOWN ARROW
			cursor_down(te->v, te->gb);
			mode = CURSOR_MOV;
			break;

		case 201: // UP ARROW
			cursor_up(te->v, te->gb);
			mode = CURSOR_MOV;
			break;
		case 204: // DELETE
			delete(te->gb);
			mode = DELETE;
			break;
		case 205: // HOME
			mode = CURSOR_MOV;
			prev_line = te->v->cursor.y-1;
			if(te->v->cursor.x == 0 && prev_line >= 0){
				if(!(te->v->screen[prev_line].has_newline)){
					cursor_up(te->v, te->gb);
					break;
				}
			}
			move_gap_to(te->gb,te->v->screen[te->v->cursor.y].start);
			break;
		case 215: // SHIFT + HOME
			mode = CURSOR_MOV;
			normal_to_select(te);
			prev_line = te->v->cursor.y-1;
			if(te->v->cursor.x == 0 && prev_line >= 0){
				if(!(te->v->screen[prev_line].has_newline)){
					cursor_up(te->v, te->gb);
					break;
				}
			}
			move_gap_to(te->gb,te->v->screen[te->v->cursor.y].start);
			break;
		case 206: // END
			mode = CURSOR_MOV;
			next_line = te->v->cursor.y+1;
			if(te->v->cursor.x == te->v->screen[te->v->cursor.y].size && next_line <= LINE_COUNT){
				if(!(te->v->screen[te->v->cursor.y].has_newline)){
					cursor_down(te->v, te->gb);
					break;
				}
			}
			pos = te->v->screen[next_line-1].start + te->v->screen[next_line-1].size;
			if(te->v->screen[next_line-1].has_newline){
				pos++;
			}
			move_gap_to(te->gb,pos);
			break;
		case 216: // SHIFT + END
			mode = CURSOR_MOV;
			normal_to_select(te);
			next_line = te->v->cursor.y+1;
			if(te->v->cursor.x == te->v->screen[te->v->cursor.y].size && next_line <= LINE_COUNT){
				if(!(te->v->screen[te->v->cursor.y].has_newline)){
					cursor_down(te->v, te->gb);
					break;
				}
			}
			pos = te->v->screen[next_line-1].start + te->v->screen[next_line-1].size;
			if(te->v->screen[next_line-1].has_newline){
				pos++;
			}
			move_gap_to(te->gb,pos);
			break;
		case 212: // SHIFT + RIGHT ARROW
			normal_to_select(te);
			cursor_right(te->gb);
			mode = CURSOR_MOV;
			break;
		case 210: // SHIFT + LEFT ARROW
			normal_to_select(te);
			cursor_left(te->gb);
			mode = CURSOR_MOV;
			break;
		case 213: // SHIFT + DOWN ARROW
			normal_to_select(te);
			cursor_down(te->v,te->gb);
			mode = CURSOR_MOV;
			break;
		case 211: // SHIFT + UP ARROW
			normal_to_select(te);
			cursor_up(te->v,te->gb);
			mode = CURSOR_MOV;
			break;
		case 221: // TAB 221
			break;
		case 230: // CTRL + C
			break;
		case 232: // CTRL + V
			paste(te->gb);
			mode = INSERT;
			break;
		case 233: // CTRL + A
			te->prev_state = SELECT;
			put_anchor_at_start(te);
			put_cursor_at_end(te->v,	te->gb);
			mode = CURSOR_MOV;
			break;
		case 220:// ESC
		case 255://F1
		case 250: // INSERT
			break;
		case 253: // NUMLOCK
			uint8_t val = 240;//TELL THE GBA WE WILL BE SENDING SENSOR READINGS
            //printf("NUMLOCK send sensor data");   
			xQueueSend(spi_send_queue, &val, 0);
            xQueueSend(spi_send_queue, &last_temperature, 0);
            xQueueSend(spi_send_queue, &last_humidity, 0);
			
			editor_to_sensor(te);
			printf("AVERAGE TIME SPENT EDITOR: %f us,   COUNT of Actions: %lld\n",((double)(editor_time/count)),count);
		
			break;
		case 251: // PRINTSCREEN
			editor_to_mail(te);
        break;
		default: 
			if(te->v->cursor.y == LINE_COUNT-1 && te->v->cursor.x == LINE_LENGTH-1){
				return;
			}
			insert(te->gb, data);
			mode = INSERT;
			break;
	}
	if(mode == -1){
		return;
	}
	visual_update(te->v,te->gb,mode);
}
void handle_data_mail(char data, text_editor *te){
	if(data == 0) return;
	text_field *active = te->ms->field[te->ms->active_field];
	switch (data) {
		case 0:
			break;
		case 221: //	TAB 221
			active = te->ms->field[switch_active_field(te->ms)];
			break;
		case 220: //	ESC 220
			mail_to_editor(te);
			break;
		case 8: // BACKSPACE
			backspace(active->gb);
			break;
		case 200: // LEFT ARROW
			cursor_left(active->gb);
			break;

		case 202: // RIGHT ARROW
			cursor_right(active->gb);
			break;

		case 203: // DOWN ARROW
			cursor_down_text_field(active->gb);
			break;

		case 201: // UP ARROW
			cursor_up_text_field(active->gb);
			break;
		case 204: // DELETE
			delete(active->gb);
			break;
		case 205: // HOME
		case 215: // SHIFT + HOME
		case 206: // END
		case 216: // SHIFT + END
		case 212: // SHIFT + RIGHT ARROW
		case 210: // SHIFT + LEFT ARROW
		case 213: // SHIFT + DOWN ARROW
		case 211: // SHIFT + UP ARROW
		case 230: // CTRL + C
		case 232: // CTRL + V
		case 233: // CTRL + A
		case 251: // PRINTSCREEN
		case 10: // ENTER
		case 253: // NUMLOCK
			break;
		case 32: // SPACE
			if(te->ms->active_field == SUBJECT){
				insert(active->gb, data);
			}
			break;
		case 250: // INSERT
			break;
		case 255: //F1
		if(sending_mail == 1){
			break;
		}
			char *text1;
			text1 = extract_text(te->gb);
			add_turkish_chars(te->mail_content,text1);
			free(text1);
			text1 = extract_text(te->ms->field[SUBJECT]->gb);
			add_turkish_chars(te->mail_subject,text1);
			free(text1);

			gba_message_recipient[0] = extract_text(te->ms->field[RECIPIENT_1]->gb);
			gba_message_recipient[1] = extract_text(te->ms->field[RECIPIENT_2]->gb);
			gba_message_recipient[2] = extract_text(te->ms->field[RECIPIENT_3]->gb);

			xTaskCreate(&smtp_client_task, "smtp_client_task", TASK_STACK_SIZE, NULL, 5, NULL);
			sending_mail = 1;


			mail_to_editor(te);
			break;
		default: 
			fflush(stdout);
			insert(active->gb, data);
			break;
	}

	
}

void handle_data_sensor(char data, text_editor *te){
	switch(data){
		case 220://ESC
			sensor_to_editor(te);
			break;
		case 254: // message sent
			break;
		default:
		break;
	}
}
void text_editor_update(char data, text_editor *te){
	uint64_t prev = esp_timer_get_time();
	if(te->app ==EDITOR){
		if(te->prev_state == NORMAL){
			handle_data_normal(data,te);
		}
		else if(te->prev_state == SELECT){
			handle_data_select(data,te);
		}
		uint64_t now = esp_timer_get_time();
		//printf("\nTIME SPENT EDITOR: %lld us \n",now-prev);
		editor_time += (double) now-prev;
		count++;
		
	}
	else if(te->app == MAIL){
		handle_data_mail(data, te);
	}
	else if(te->app == SENSOR){
		handle_data_sensor(data, te);
	}

}



text_editor *create_text_editor(int size){
    text_editor *retval = malloc(sizeof(text_editor));
	if(retval== NULL){
		printf("ERROR AT TEXT_EDITOR MALLOC");
	}

	retval->mail_content = malloc(sizeof(char)*BUF_SIZE);
	if(retval->mail_content== NULL){
		printf("ERROR AT TEXT_EDITOR MAIL_CONTENT MALLOC");
	}
	memset(retval->mail_content, 0, BUF_SIZE);

	retval->mail_subject = malloc(sizeof(char)*120);
	if(retval->mail_content== NULL){
		printf("ERROR AT TEXT_EDITOR MAIL_SUBJECT MALLOC");
	}
	memset(retval->mail_subject, 0, 120);  

    retval->prev_state = NORMAL;
    retval->anchor = 0;
    retval->v = create_visual();
    retval->gb = create_gapbuf(size);
	retval->ms = create_mail_screen(4);
	if(retval->ms == NULL){
		printf("ERROR AT TEXT_EDITOR mail_screen MALLOC");
	}
	retval->app = EDITOR;
	gba_message = retval->mail_content;
	gba_message_subject = retval->mail_subject;
    return retval;
}



void put_anchor_on_cursor(text_editor *te){
	te->anchor = te->gb->cursor;
    te->anchor_y = te->v->cursor.y;
}
void put_anchor_at_start(text_editor *te){
    te->anchor = 0;
    te->anchor_y = 0;
}
void normal_to_select(text_editor *te){
	te->prev_state = SELECT;
	put_anchor_on_cursor(te);

}
void select_to_normal(text_editor *te){
	te->prev_state = NORMAL;
}



void add_turkish_chars(char *field, char *text){
	uint8_t c;
	int j = 0;
	uint64_t prev = esp_timer_get_time();
	for(int i=0;i<BUF_SIZE;i++){
		c = (uint8_t)text[i];
		if(c == 0){break;}
		switch(c){
			case 136://ı 0xC4	0xB1
			field[j++]=0xC4;
			field[j++]=0xB1;
			break;
			case 128://ğ 0xC4	0x9F
			field[j++]=0xC4;
			field[j++]=0x9F;
			break;
			case 129://Ğ 0xC4	0x9E
			field[j++]=0xC4;
			field[j++]=0x9E;
			break;
			case 138://ü 0xC3	0xBC
			field[j++]=0xC3;
			field[j++]=0xBC;
			break;
			case 139://Ü 0xC3	0x9C
			field[j++]=0xC3;
			field[j++]=0x9C;
			break;
			case 130://ş 0xC5	0x9F
			field[j++]=0xC5;
			field[j++]=0x9F;
			break;
			case 131://Ş 0xC5	0x9E
			field[j++]=0xC5;
			field[j++]=0x9E;
			break;
			case 137://İ 0xC4	0xB0
			field[j++]=0xC4;
			field[j++]=0xB0;
			break;
			case 132://ö 0xC3	0xB6
			field[j++]=0xC3;
			field[j++]=0xB6;
			break;
			case 133://Ö 0xC3	0x96
			field[j++]=0xC3;
			field[j++]=0x96;
			break;
			case 134://ç 0xC3	0xA7
			field[j++]=0xC3;
			field[j++]=0xA7;
			break;
			case 135://Ç 0xC3	0x87
			field[j++]=0xC3;
			field[j++]=0x87;
			break;
			default:
			field[j++]=c;
			break;
			
		}

	}
		field[j]='\0';
		uint64_t now = esp_timer_get_time();
		printf("\nTIME SPENT CONVERTING TURKISH CHARS: %lld us \n",now-prev);
}


void editor_to_mail(text_editor *te){
	te->prev_state = NORMAL;
	te->app = MAIL;
}
void mail_to_editor(text_editor *te){
	te->prev_state = NORMAL;
	te->app = EDITOR;
}

void sensor_to_editor(text_editor *te){
	te->prev_state = NORMAL;
	te->app = EDITOR;
}

void editor_to_sensor(text_editor *te){
	te->prev_state = NORMAL;
	te->app = SENSOR;
}