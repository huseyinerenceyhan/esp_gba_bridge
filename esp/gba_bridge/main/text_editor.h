#ifndef TEXT_EDITOR_H
#define TEXT_EDITOR_H
#include "gapbuf.h"
#include "visual.h"
#include "mail_screen.h"
enum STATE{
	NORMAL,
	SELECT,
};
enum APP{
	EDITOR,
	MAIL,
    SENSOR,
};


typedef struct
{
    gapbuf *gb;
    char *mail_content;
    char *mail_subject;
    visual *v;
    int anchor;
    int anchor_y;
    int prev_state;
    mail_screen *ms;
    int app;
} text_editor;

text_editor* create_text_editor(int size);
void handle_data_select(char data,text_editor *te);
void handle_data_normal(char data,text_editor *te);
void handle_data_mail(char data,text_editor *te);
void text_editor_update(char data,text_editor *te);

void put_anchor_on_cursor(text_editor *te);
void put_anchor_at_start(text_editor *te);
void normal_to_select(text_editor *te);
void select_to_normal(text_editor *te);


void add_turkish_chars(char *field, char *text);

void editor_to_mail(text_editor *te);
void mail_to_editor(text_editor *te);


void sensor_to_editor(text_editor *te);

void editor_to_sensor(text_editor *te);

#endif // TEXT_EDITOR_H