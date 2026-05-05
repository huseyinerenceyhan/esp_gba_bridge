#ifndef VISUAL_H
#define VISUAL_H
#include "gapbuf.h"

#define LINE_LENGTH 31
#define LINE_COUNT 60
#define VISUAL_LINE_COUNT 20
#define VISUAL_LINE_LENGTH (LINE_LENGTH-1)


enum ACTION{
	CURSOR_MOV,
	INSERT,
	DELETE,
	ACTION_COUNT
};

typedef struct{
	char data[LINE_LENGTH];
	int start;
	int has_newline;
	int size;
	int dirty;
}line;

typedef struct{
	int x;
	int y;
}position;


typedef struct{
	line screen[LINE_COUNT];
	position cursor;
	int gb_pos;
}visual;



visual *create_visual();

void visual_update(visual *v, gapbuf *gb, int mode);
void cursor_up(visual *v, gapbuf *gb);
void cursor_down(visual *v, gapbuf *gb);
void render_screen(visual *v, int screen_top);
void show_visual(visual *v);
void empty_visual(visual *v);
int line_changed(line *before, line *now);
void put_cursor_at_end(visual *v, gapbuf *gb);




#endif //VISUAL_H