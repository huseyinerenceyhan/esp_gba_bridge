#ifndef TEXT_SCREEN_H
#define TEXT_SCREEN_H
#include "gapbuf.h"
#include <tonc.h>

#define DEFAULT_TEXT_FIELD_LENGTH 60; 
#define GAP 4; // Assuming that each text_field has the same gap between them

enum TEXT_FIELD_NAMES{
	SUBJECT,
	RECIPIENT_1,
	RECIPIENT_2,
	RECIPIENT_3
};

typedef struct{
	gapbuf *gb;
	int height;
} text_field;

typedef struct{
	int active_field;
	int size;
	text_field *field[4];
} mail_screen;

text_field *create_text_field(int size, int height);
mail_screen *create_mail_screen(int size);

int switch_active_field(mail_screen *ms);
void cursor_down_text_field(gapbuf *gb);
void cursor_up_text_field(gapbuf *gb);



void render_mail_screen(text_field *tf);

void save_written_text_fields(mail_screen *ms);

void load_written_text_fields(mail_screen *ms);


#endif //TEXT_SCREEN_H