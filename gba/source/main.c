#include <tonc.h>
#include <stdio.h>
#include <stdlib.h>
#include "printdebug.h"
#include "fonts.h"
#include "cursor.h"
#include "anchor.h"

#include "cursor_sprite.h"
#include "anchor_sprite.h"
#include "msg_info.h"
#include "sending.h"
#include "sensor.h"

#include "interrupt.h"
#include "gapbuf.h"
#include "visual.h"
#include "mail_screen.h"


/* uint64_t total;
uint64_t count;
double avg;

uint64_t start;
uint64_t end; */

volatile int got_new_data = 0;
OBJ_ATTR obj_buffer[128];

OBJ_ATTR *sending;
cursor *g_cursor;
anchor *g_anchor;
gapbuf	*g_gb;
visual *g_v;
mail_screen *g_mail;

int screen_bottom=152;
int screen_top=0;

volatile int msg = 0;

volatile int last_temperature = 0;
volatile int last_humidity = 0;

volatile int sensor_data_count = 0;
volatile int receiving_sensor_data = 0;

void enable_msg_info();
void disable_msg_info();
void enable_sensor();

enum STATE{
	NORMAL,
	SELECT,
};

enum APP{
	EDITOR,
	MAIL,
	SENSOR,
};

int prev_state = NORMAL;
int app = EDITOR;
void render_sensor(){
	tte_erase_rect(0,0,240,160);
	char val[3];
	tte_set_pos(176,96);
	itoa(last_temperature, val, 10);
	//print("%s",val);
	tte_write(val);
	
	tte_set_pos(152,120);
	itoa(last_humidity, val, 10);
	//print("%s",val);
	tte_write(val);
	
	
/* 	avg =(double)total/count;
	tte_set_pos(0,0);
	char vala[16];
	sprintf(vala, "%.2f", avg);
	tte_write(vala);
	//print("%f",avg); */
}
void enable_msg_info(){
	REG_BG2CNT= BG_CBB(0) | BG_SBB(30);	
		  	// Load palette
	memcpy16(&pal_bg_mem[0], msg_infoPal, msg_infoPalLen / sizeof(u16));

		
	tte_init_se(3,
	BG_PRIO(0) | BG_CBB(1) | BG_SBB(29),
	0,CLR_WHITE,14,&turkishFont,NULL);
	REG_BG2CNT |= BG_PRIO(2); 
	
	tte_erase_rect(0,0,240,160);
	
	load_written_text_fields(g_mail);
	
	REG_DISPCNT &= ~DCNT_BG1;
	REG_DISPCNT |= DCNT_BG2;
	REG_DISPCNT |= DCNT_BG3;

}
void disable_msg_info(){
	REG_DISPCNT &= ~DCNT_BG2;
	REG_DISPCNT &= ~DCNT_BG3;
	REG_DISPCNT |= DCNT_BG1;
	tte_init_se(1,
	BG_CBB(1)|BG_SBB(6)| BG_REG_32x64,
	0,CLR_WHITE,14,&turkishFont,NULL);
	
}
void enable_sensor(){
	REG_BG2CNT= BG_CBB(2) | BG_SBB(24);	
			// Load palette
	memcpy16(&pal_bg_mem[0], sensorPal, sensorPalLen / sizeof(u16));
	
		tte_init_se(3,
	BG_PRIO(0) | BG_CBB(1) | BG_SBB(29),
	0,CLR_YELLOW,14,&turkishFont,NULL);
	REG_BG2CNT |= BG_PRIO(2); 
	
	tte_erase_rect(0,0,240,160);
	
	REG_DISPCNT &= ~DCNT_BG1;
	REG_DISPCNT |= DCNT_BG2;
	REG_DISPCNT |= DCNT_BG3;

	
	
}
void disable_sensor(){	
	tte_erase_rect(0,0,240,160);

	REG_DISPCNT &= ~DCNT_BG2;
	REG_DISPCNT &= ~DCNT_BG3;
	REG_DISPCNT |= DCNT_BG1;
	
	tte_init_se(1,
	BG_CBB(1)|BG_SBB(6)| BG_REG_32x64,
	0,CLR_WHITE,14,&turkishFont,NULL);	
	

}
IWRAM_CODE void normal_to_select(gapbuf *gb, visual *v, cursor *c, anchor *a){
	prev_state = SELECT;
	put_anchor_on_cursor(gb,v,c,a);
	set_anchor_visibility(a, ON);
}
IWRAM_CODE void select_to_normal(anchor *a){
	prev_state = NORMAL;
	set_anchor_visibility(a, OFF);
	a->active = OFF;
}
IWRAM_CODE void editor_to_mail(anchor *a){
	prev_state = NORMAL;
	app = MAIL;
	set_anchor_visibility(a, OFF);
	a->active = OFF;
	enable_msg_info();
	update_cursor_loc_mail_screen(g_cursor,g_mail);
}
IWRAM_CODE void editor_to_sensor(anchor *a){
	prev_state = NORMAL;
	app = SENSOR;
	set_anchor_visibility(a, OFF);
	a->active = OFF;
	enable_sensor();
	render_sensor();
	//Turn off cursor sprite
	u16 new_attr2_value = ATTR2_ID(1);
	g_cursor->sprite->attr2 = (g_cursor->sprite->attr2 & ~ATTR2_ID_MASK) | new_attr2_value;
}
IWRAM_CODE void sensor_to_editor(){
	prev_state = NORMAL;
	app = EDITOR;
	disable_sensor();
}
IWRAM_CODE void mail_to_editor(){
	prev_state = NORMAL;
	app = EDITOR;
	disable_msg_info();
}
IWRAM_CODE void scroll(){
	TTC *tc = tte_get_context();
	int offset = 0;

	if(tc->cursorY >= screen_bottom){
		offset = tc->cursorY - screen_bottom;
		if(screen_bottom + offset <= 512){
			screen_bottom += offset;
			screen_top += offset;
			REG_BG1VOFS = screen_top;
			tte_set_pos(tc->cursorX,screen_bottom - offset);
		}

	}
	else if(tc->cursorY < screen_top){
		offset = screen_top - tc->cursorY;
		if(screen_top - offset >= 0){
			screen_bottom -= offset;
			screen_top -= offset;
			REG_BG1VOFS = screen_top;
			tte_set_pos(tc->cursorX, screen_top + offset);

		}
		

	}
	
}



IWRAM_CODE void handle_data_select(u32 data){
	int prev_line;
	int next_line;
	int pos;
	if(data == 0) return;
	int mode = -1;
	prev_state = SELECT;
	switch (data) {
		case 204:// DELETE
		case 8: // BACKSPACE
			select_delete(g_gb, g_anchor->gb_pos);
			select_to_normal(g_anchor);
			if(g_anchor->visual_y < g_v->cursor.y){
				g_v->cursor.y = g_anchor->visual_y;
			}
			mode = DELETE;
			break;

		case 200: // LEFT ARROW
			cursor_left(g_gb);
			select_to_normal(g_anchor);
			mode = CURSOR_MOV;
			break;

		case 202: // RIGHT ARROW
			cursor_right(g_gb);
			select_to_normal(g_anchor);
			mode = CURSOR_MOV;
			break;

		case 203: // DOWN ARROW
			cursor_down(g_v, g_gb);
			select_to_normal(g_anchor);
			mode = CURSOR_MOV;
			break;

		case 201: // UP ARROW
			cursor_up(g_v, g_gb);
			select_to_normal(g_anchor);
			mode = CURSOR_MOV;
			break;
			
		case 205: // HOME
			mode = CURSOR_MOV;
			select_to_normal(g_anchor);
			prev_line = g_v->cursor.y-1;
			if(g_v->cursor.x == 0 && prev_line >= 0){
				if(!(g_v->screen[prev_line].has_newline)){
					cursor_up(g_v, g_gb);
					break;
				}
			}
			move_gap_to(g_gb,g_v->screen[g_v->cursor.y].start);
			break;
		case 215: // SHIFT + HOME
			mode = CURSOR_MOV;
			prev_line = g_v->cursor.y-1;
			if(g_v->cursor.x == 0 && prev_line >= 0){
				if(!(g_v->screen[prev_line].has_newline)){
					cursor_up(g_v, g_gb);
					break;
				}
			}
			move_gap_to(g_gb,g_v->screen[g_v->cursor.y].start);
			break;
		case 206: // END
			mode = CURSOR_MOV;
			select_to_normal(g_anchor);
			next_line = g_v->cursor.y+1;
			if(g_v->cursor.x == g_v->screen[g_v->cursor.y].size && next_line <= LINE_COUNT){
				if(!(g_v->screen[g_v->cursor.y].has_newline)){
					cursor_down(g_v, g_gb);
					break;
				}
			}
			pos = g_v->screen[next_line-1].start + g_v->screen[next_line-1].size;
			if(g_v->screen[next_line-1].has_newline){
				pos++;
			}
			move_gap_to(g_gb,pos);
			break;
		case 216: // SHIFT + END
			mode = CURSOR_MOV;
			next_line = g_v->cursor.y+1;
			if(g_v->cursor.x == g_v->screen[g_v->cursor.y].size && next_line <= LINE_COUNT){
				if(!(g_v->screen[g_v->cursor.y].has_newline)){
					cursor_down(g_v, g_gb);
					break;
				}
			}
			pos = g_v->screen[next_line-1].start + g_v->screen[next_line-1].size;
			if(g_v->screen[next_line-1].has_newline){
				pos++;
			}
			move_gap_to(g_gb,pos);
			break;

		case 212: // SHIFT + RIGHT ARROW
			cursor_right(g_gb);
			mode = CURSOR_MOV;
			break;
		case 210: // SHIFT + LEFT ARROW
			cursor_left(g_gb);
			mode = CURSOR_MOV;
			break;
		case 213: // SHIFT + DOWN ARROW
			cursor_down(g_v,g_gb);
			mode = CURSOR_MOV;
			break;
		case 211: // SHIFT + UP ARROW
			cursor_up(g_v,g_gb);
			mode = CURSOR_MOV;
			break;
		case 221: //	TAB 221
			break;
		case 230: // CTRL + C
			extract_text_between_pos(g_gb, g_anchor->gb_pos, g_v->gb_pos);
			select_to_normal(g_anchor);
			mode = CURSOR_MOV;
			break;
		case 231: // CTRL + X
			extract_text_between_pos(g_gb, g_anchor->gb_pos, g_v->gb_pos);
			select_delete(g_gb, g_anchor->gb_pos);
			select_to_normal(g_anchor);
			if(g_anchor->visual_y < g_v->cursor.y){
				g_v->cursor.y = g_anchor->visual_y;
			}
			mode = DELETE;
			break;
		case 232: // CTRL + V
			select_delete(g_gb, g_anchor->gb_pos);
			if(g_anchor->visual_y < g_v->cursor.y){
				g_v->cursor.y = g_anchor->visual_y;
			}
			paste(g_gb);
			select_to_normal(g_anchor);
			mode = DELETE;
			break;
		case 233: // CTRL + A
			put_anchor_at_start(g_anchor);
			put_cursor_at_end(g_v,	g_gb);
			mode = CURSOR_MOV;
			break;
		case 250: // INSERT
			change_cursor(g_cursor);
			break;
		case 251: // PRINTSCREEN
			editor_to_mail(g_anchor);
		case 255://F1
			break;
		case 254: // message sent
			sending->attr0 = ATTR0_HIDE;
			break;
		default:
			select_delete(g_gb, g_anchor->gb_pos);
			select_to_normal(g_anchor);
			if(g_anchor->visual_y < g_v->cursor.y){
				g_v->cursor.y = g_anchor->visual_y;
			}
			insert(g_gb, data);
			mode = DELETE;
			break;
	}
	if(mode == -1){
		return;
	}
	visual_update(g_v,g_gb,mode);
	//render_screen(g_v, screen_top);
	tte_set_pos(g_v->cursor.x*8,g_v->cursor.y*8);
	scroll();
	update_anchor(g_anchor,screen_top);
	render_screen(g_v, screen_top);

}

IWRAM_CODE void handle_data_normal(u32 data){
	int prev_line;
	int next_line;
	int pos;
	if(data == 0) return;
	int mode = -1;
	prev_state = NORMAL;
	switch (data) {
		case 8: // BACKSPACE
			backspace(g_gb);
			mode = DELETE;
			break;

		case 200: // LEFT ARROW
			cursor_left(g_gb);
			mode = CURSOR_MOV;
			break;

		case 202: // RIGHT ARROW
			cursor_right(g_gb);
			mode = CURSOR_MOV;
			break;

		case 203: // DOWN ARROW
			cursor_down(g_v, g_gb);
			mode = CURSOR_MOV;
			break;

		case 201: // UP ARROW
			cursor_up(g_v, g_gb);
			mode = CURSOR_MOV;
			break;
		case 204: // DELETE
			delete(g_gb);
			mode = DELETE;
			break;
		case 205: // HOME
			mode = CURSOR_MOV;
			prev_line = g_v->cursor.y-1;
			if(g_v->cursor.x == 0 && prev_line >= 0){
				if(!(g_v->screen[prev_line].has_newline)){
					cursor_up(g_v, g_gb);
					break;
				}
			}
			move_gap_to(g_gb,g_v->screen[g_v->cursor.y].start);
			break;
		case 215: // SHIFT + HOME
			mode = CURSOR_MOV;
			normal_to_select(g_gb,g_v,g_cursor,g_anchor);
			prev_line = g_v->cursor.y-1;
			if(g_v->cursor.x == 0 && prev_line >= 0){
				if(!(g_v->screen[prev_line].has_newline)){
					cursor_up(g_v, g_gb);
					break;
				}
			}
			move_gap_to(g_gb,g_v->screen[g_v->cursor.y].start);

			break;
		case 206: // END
			mode = CURSOR_MOV;
			next_line = g_v->cursor.y+1;
			if(g_v->cursor.x == g_v->screen[g_v->cursor.y].size && next_line <= LINE_COUNT){
				if(!(g_v->screen[g_v->cursor.y].has_newline)){
					cursor_down(g_v, g_gb);

					break;
				}
			}
			pos = g_v->screen[next_line-1].start + g_v->screen[next_line-1].size;
			if(g_v->screen[next_line-1].has_newline){
				pos++;
			}
			move_gap_to(g_gb,pos);
			break;
		case 216: // SHIFT + END
			mode = CURSOR_MOV;
			normal_to_select(g_gb,g_v,g_cursor,g_anchor);
			next_line = g_v->cursor.y+1;
			if(g_v->cursor.x == g_v->screen[g_v->cursor.y].size && next_line <= LINE_COUNT){
				if(!(g_v->screen[g_v->cursor.y].has_newline)){
					cursor_down(g_v, g_gb);
					break;
				}
			}
			pos = g_v->screen[next_line-1].start + g_v->screen[next_line-1].size;
			if(g_v->screen[next_line-1].has_newline){
				pos++;
			}
			move_gap_to(g_gb,pos);
			break;
		case 212: // SHIFT + RIGHT ARROW
			normal_to_select(g_gb,g_v,g_cursor,g_anchor);
			cursor_right(g_gb);
			mode = CURSOR_MOV;
			break;
		case 210: // SHIFT + LEFT ARROW
			normal_to_select(g_gb,g_v,g_cursor,g_anchor);
			cursor_left(g_gb);
			mode = CURSOR_MOV;
			break;
		case 213: // SHIFT + DOWN ARROW
			normal_to_select(g_gb,g_v,g_cursor,g_anchor);
			cursor_down(g_v,g_gb);
			mode = CURSOR_MOV;
			break;
		case 211: // SHIFT + UP ARROW
			normal_to_select(g_gb,g_v,g_cursor,g_anchor);
			cursor_up(g_v,g_gb);
			mode = CURSOR_MOV;
			break;
		case 221: //	TAB 221
			break;
		case 230: // CTRL + C
			break;
		case 232: // CTRL + V
			paste(g_gb);
			mode = INSERT;
			break;
		case 233: // CTRL + A
			prev_state = SELECT;
			put_anchor_at_start(g_anchor);
			put_cursor_at_end(g_v,	g_gb);
			mode = CURSOR_MOV;
			break;
		case 250: // INSERT
			change_cursor(g_cursor);
			break;
		case 251://print screen
			editor_to_mail(g_anchor);
			break;
		case 255://F1
			break;
		case 253: // NUMLOCK
			editor_to_sensor(g_anchor);
			break;
		case 220:// ESC
			break;
		case 254: // message sent
			sending->attr0 = ATTR0_HIDE;
			break;
		default: 
			if(g_v->cursor.y == LINE_COUNT-1 && g_v->cursor.x == LINE_LENGTH-1){
				return;
			}
			insert(g_gb, data);
			mode = INSERT;
			break;
	}
	if(mode == -1){
		return;
	}
	visual_update(g_v,g_gb,mode);
	//render_screen(g_v, screen_top);
	tte_set_pos(g_v->cursor.x*8,g_v->cursor.y*8);
	scroll();
	render_screen(g_v, screen_top);
	update_anchor(g_anchor,screen_top);

}
IWRAM_CODE void handle_data_mail(u32 data){
	int changed = 0;
	text_field *active = g_mail->field[g_mail->active_field];
	switch (data) {

		break;
		case 0:
		break;
		case 221: //	TAB 221
			active = g_mail->field[switch_active_field(g_mail)];
		break;
		case 220: //	ESC 220
			mail_to_editor();
		break;
		case 8: // BACKSPACE
			backspace(active->gb);
			changed = 1;
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
			changed = 1;
			break;
		case 205: // HOME
		case 215: // SHIFT + HOME
		case 206: // END
		case 216: // SHIFT + END
		case 212: // SHIFT + RIGHT ARROW
		case 210: // SHIFT + LEFT ARROW
		case 213: // SHIFT + DOWN ARROW
		case 211: // SHIFT + UP ARROW

		case 232: // CTRL + V
		case 233: // CTRL + A
		case 10: // ENTER
		case 251: // PRINTSCREEN
		case 253: //NUMLOCK
			break;
		case 32: // SPACE
			if(g_mail->active_field == SUBJECT){
				insert(active->gb, data);
				changed = 1;
			}
			break;
		case 250: // INSERT
			change_cursor(g_cursor);
			break;
		case 255://F1
			mail_to_editor();
			sending->attr0 = ATTR0_REG;//show the sending mail sprite
			obj_set_pos(sending, 88, 48);
			break;
		case 254: // message sent
			sending->attr0 = ATTR0_HIDE;
			break;
		default: 
			insert(active->gb, data);
			changed = 1;
			break;
	}
	if(changed){
	///// UPDATE THE Text Field that has changed
		render_mail_screen(active);
	}
	update_cursor_loc_mail_screen(g_cursor,g_mail);
	tte_set_pos(g_cursor->x,g_cursor->y);
	
}



IWRAM_CODE void handle_data_sensor(u32 data){
	switch(data){
		case 220://ESC
			sensor_to_editor();
			break;
		case 254: // message sent
			sending->attr0 = ATTR0_HIDE;
			break;
		default:
		break;
	}
}


void emulator_test(){
	int mode = 20;
	if(key_hit(KEY_UP)){
		mail_to_editor();
		
	}
	else if(key_hit(KEY_DOWN)){
		editor_to_mail(g_anchor);
	}
	else if(key_hit(KEY_RIGHT)){
		editor_to_sensor(g_anchor);
		render_sensor();
	}
	else if(key_hit(KEY_LEFT)){
		sensor_to_editor();
	}
	else if(key_held(KEY_A)){
		tte_putc('a');
	}
	else if(key_hit(KEY_B)){
		if(msg % 2 == 1){
			tte_write("\n");
			return;
		}
		insert(g_gb, '\n');
		mode = INSERT;	
	}
	else if(key_hit(KEY_L)){
		backspace(g_gb);
		mode = DELETE;
	}
else if(key_hit(KEY_SELECT)){
	sensor_to_editor();
}
else if(key_hit(KEY_START)){
	editor_to_sensor(g_anchor);
	render_sensor();

}		
	if(mode ==20){
		return;
	}

	visual_update(g_v,g_gb,mode);
	tte_set_pos(g_v->cursor.x*8,g_v->cursor.y*8);
	//print("BEFORE SCROLL cursorX: %d, cursorY: %d",g_v->cursor.x*8,g_v->cursor.y*8);
	scroll();
	render_screen(g_v, screen_top);
}






IWRAM_CODE void test_tte_se4()
{
	tte_init_se(
	1,
	BG_CBB(1)|BG_SBB(6)| BG_REG_32x64,
	0,
	CLR_WHITE,
	14,
	&turkishFont,
	NULL);    
	//tte_init_con();
	//tte_write("Testing Investigating Searching 12345 ,./");
	while(1){
		VBlankIntrWait();
		//key_poll();
		//emulator_test();
		if(got_new_data){
					
			u32 data = REG_SIODATA8;
			if(1 == receiving_sensor_data){
				if(0 == sensor_data_count){
					sensor_data_count++;
					last_temperature = data;
				}
				else if(1 == sensor_data_count){
					sensor_data_count = 0;
					last_humidity = data;
					receiving_sensor_data = 0;//END OF SENSOR DATA
				}
			}
			else if(240 == data){//RECEIVING SENSOR DATA
				receiving_sensor_data = 1;
			}
			else{
				switch(app){
					case EDITOR:
						//start = REG_VCOUNT;
						if(prev_state == NORMAL){
							handle_data_normal(data);
						}
						else if(prev_state == SELECT){
							handle_data_select(data);
						}
					break;
					case MAIL:
						handle_data_mail(data);
						break;
					case SENSOR:
						handle_data_sensor(data);

						break;
				}
			}


			//tte_printf("#{es;P}SIZE: %d",gb_used(g_gb));
			got_new_data = 0;
			if(SENSOR != app){
				keep_cursor_on(g_cursor);
			}
		}
		

		
		if(EDITOR == app){
			update_cursor_loc(g_cursor,g_v, screen_top);

		}
		else if(SENSOR == app){
			render_sensor();
		}

		if(SENSOR != app){
			render_cursor(g_cursor);
		}			
		oam_copy(oam_mem, obj_buffer, 16);

				
/* 		if(EDITOR == app){
							end = REG_VCOUNT;
							total += end-start;
							count++;
		} */
	}
}




int main()
{
	irq_init(NULL);
	irq_add(II_VBLANK, NULL);
	irq_add(II_SERIAL, slave_routine);
	
	oam_init(obj_buffer, 128);
	
	REG_DISPCNT= DCNT_MODE0| DCNT_OBJ | DCNT_BG1|DCNT_OBJ_1D;
	//REG_BG0CNT = BG_CBB(0)|BG_SBB(6)| BG_REG_32x64;
		REG_BLDCNT= BLD_BUILD(
		 BLD_BG0,	// Top
		BLD_BG1,			// Bottom
		1);	

	/* 
	4000134h - RCNT (R) - Mode Selection, in Normal/Multiplayer/UART modes (R/W)
  Bit   Expl.
  0-3   Undocumented (current SC,SD,SI,SO state, as for General Purpose mode)
  4-8   Not used     (Should be 0, bits are read/write-able though)
  9-13  Not used     (Always 0, read only)
  14    Not used     (Should be 0, bit is read/write-able though)
  15    Must be zero (0) for Normal/Multiplayer/UART modes	
	*/
	REG_RCNT = 0b0000000000000000;
	/* 
	4000128h - SIOCNT - SIO Control, usage in NORMAL Mode (R/W)

  Bit   Expl.
  0     Shift Clock (SC)        (0=External, 1=Internal)
  1     Internal Shift Clock    (0=256KHz, 1=2MHz)
  2     SI State (opponents SO) (0=Low, 1=High/None) --- (Read Only)
  3     SO during inactivity    (0=Low, 1=High) (applied ONLY when Bit7=0)
  4-6   Not used                (Read only, always 0 ?)
  7     Start Bit               (0=Inactive/Ready, 1=Start/Active)
  8-11  Not used                (R/W, should be 0)
  12    Transfer Length         (0=8bit, 1=32bit)
  13    Must be "0" for Normal Mode
  14    IRQ Enable              (0=Disable, 1=Want IRQ upon completion)
  15    Not used                (Read only, always 0)	
	*/
	REG_SIOCNT = 0b0100000010000000;
	
	REG_SIODATA8=0;

	memcpy32(&tile_mem[4][0], cursor_spriteTiles, cursor_spriteTilesLen/ sizeof(u32));
	memcpy16(&pal_obj_mem[0], cursor_spritePal, cursor_spritePalLen/ sizeof(u16));
	OBJ_ATTR *sprite= &obj_buffer[0];
	obj_set_attr(sprite,ATTR0_SQUARE | ATTR0_BLEND,ATTR1_SIZE_8,ATTR2_PALBANK(0) | 0);
	g_cursor = create_cursor(0,0,sprite);
	
	memcpy32(&tile_mem[4][4], anchor_spriteTiles, anchor_spriteTilesLen/ sizeof(u32));
	memcpy16(&pal_obj_mem[16], anchor_spritePal, anchor_spritePalLen/ sizeof(u16));
	OBJ_ATTR *anchor= &obj_buffer[1];
	obj_set_attr(anchor,ATTR0_SQUARE | ATTR0_BLEND,ATTR1_SIZE_8,ATTR2_PALBANK(1) | 4);
	g_anchor = create_anchor(0,0,anchor);
	set_anchor_visibility(g_anchor, OFF);
	
	memcpy32(&tile_mem[4][6], sendingTiles, sendingTilesLen/ sizeof(u32));
	memcpy16(&pal_obj_mem[32], sendingPal, sendingPalLen/ sizeof(u16));
	sending= &obj_buffer[2];
	obj_set_attr(sending,ATTR0_SQUARE ,ATTR1_SIZE_64x64,ATTR2_PRIO(0)|ATTR2_PALBANK(2) | 6);
	obj_set_pos(sending, 88, 48);
	//sending->attr0 = ATTR0_REG;
	sending->attr0 = ATTR0_HIDE;

	
	oam_copy(oam_mem, obj_buffer, 16);
	

	// Load tiles into CBB 0
	memcpy32(&tile_mem[0][0], msg_infoTiles, msg_infoTilesLen / sizeof(u32));
	// Load map into SBB 30
	memcpy32(&se_mem[30][0], msg_infoMap, msg_infoMapLen / sizeof(u32));
	
	
	
	
	

	// Load tiles into CBB 0
	memcpy32(&tile_mem[2][0], sensorTiles, sensorTilesLen / sizeof(u32));
	// Load map into SBB 30
	memcpy32(&se_mem[24][0], sensorMap, sensorMapLen / sizeof(u32));
 
	g_gb = create_gapbuf(1800);
	g_v = create_visual();
	g_mail = create_mail_screen(4);
	update_cursor_loc(g_cursor,g_v, screen_top);
	render_cursor(g_cursor);	
	prev_state = NORMAL;
	test_tte_se4();
	
}
