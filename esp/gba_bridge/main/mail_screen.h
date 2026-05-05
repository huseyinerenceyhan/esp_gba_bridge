#ifndef MAIL_SCREEN_H
#define MAIL_SCREEN_H
#include "gapbuf.h"



enum TEXT_FIELD_NAMES{
	SUBJECT,
	RECIPIENT_1,
	RECIPIENT_2,
	RECIPIENT_3
};

typedef struct{
	gapbuf *gb;
} text_field;

typedef struct{
	int active_field;
	int size;
	text_field *field[4];
} mail_screen;

text_field *create_text_field(int sizet);
mail_screen *create_mail_screen(int size);

int switch_active_field(mail_screen *ms);
void cursor_down_text_field(gapbuf *gb);
void cursor_up_text_field(gapbuf *gb);



#endif //MAIL_SCREEN_H