#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "gapbuf.h"

#include "visual.h"
#include "printdebug.h"

gapbuf *create_gapbuf(int size) {
    gapbuf *retVal = malloc(sizeof(gapbuf));
    retVal->size = size;
    retVal->cursor  = 0;
    retVal->gap_end = size;
    retVal->copy_size = 0;
    retVal->buf = malloc(sizeof(char)*size);
    retVal->copied_text = malloc((sizeof(char)*size)+1);
    memset(retVal->buf, 0, size);
    memset(retVal->copied_text, 0, size);

    return retVal;
}


// Move right side of `buf` to right side of `new_buf`.
void move_right_txt(gapbuf *gb, char *new_buf, int new_size){
    memmove(new_buf + new_size - gb_right(gb), // right side of new_buf
             gb->buf + gb->gap_end,         // right side of buf
             gb_right(gb));                     // size of right side
}






void insert(gapbuf *gb, char c){
	//printf("start:%d end:%d\n",gb->cursor,gb->gap_end);
    if(gb->cursor == gb->gap_end)
    { 

            return;

    }
    gb->buf[gb->cursor] = c;
    gb->cursor++;
	
	
	
	//printf("Inserted: %c\n",c);
}

void backspace(gapbuf *gb){
    // the gap is never printed  we just move the cursor left
    if(gb->cursor > 0)
    {
        gb->cursor--;
    } 
}

void delete(gapbuf *gb){
    // the gap is never printed  we just move the cursor left
    if(gb->gap_end < gb->size)
    {
        gb->gap_end++;
    } 
}

void move_gap_to(gapbuf *gb, int pos) {
    if (pos<0) {
        return;
    }
    if (pos>gb_used(gb)) {
        return;
    }
    while (gb->cursor > pos) {
        gb->buf[--gb->gap_end] = gb->buf[--gb->cursor];
    }
    while (gb->cursor < pos) {
        gb->buf[gb->cursor++] = gb->buf[gb->gap_end++];
    }
}
void cursor_left(gapbuf *gb){
    if(gb->cursor > 0)
    {
        gb->buf[--gb->gap_end] = gb->buf[--gb->cursor];
    }
	
}
void cursor_right(gapbuf *gb){
    if (gb->gap_end < gb->size)
    {
        gb->buf[gb->cursor++] = gb->buf[gb->gap_end++];
    }
}

inline char get_char_at(gapbuf *gb,int pos) {
    if (pos < 0 || pos >= gb_used(gb)) {
        return 0;
    }

    if (pos < gb->cursor) {
        return gb->buf[pos];
    }
    pos += gb->gap_end - gb->cursor;
    return gb->buf[pos];
}



char *extract_text (gapbuf *gb){
    if (SIZE_MAX == gb_used (gb))
    {
        return NULL;
    }

    char *text = malloc (gb_used (gb) + 1);
    if (!text)
    {
        return NULL;
    }

    strncpy (text, gb->buf, gb->cursor);
    strncpy (text + gb->cursor, gb->buf + gb->gap_end, gb_right (gb));
    text[gb_used (gb)] = '\0';
    return text;
}


void print_buffer (gapbuf *gb){
    char *text = extract_text (gb);
    printf ("%s\n", text);
    printf ("cursor:%d\n", gb->cursor);
    free (text);
}


void select_delete(gapbuf *gb, int pos){
	if(gb->cursor > pos){
		while(gb->cursor != pos){
			gb->cursor--;
		}
	}
	else if(pos > gb->cursor){
		int stop = gb->cursor;
		move_gap_to(gb,pos);
		while(gb->cursor != stop){
			gb->cursor--;
		}
	}
}

 void extract_text_between_pos (gapbuf *gb, int pos_1, int pos_2){

    if (pos_1 < 0 || pos_2 < 0) {
        return;
    }
    if (pos_1 > gb->size || pos_2 > gb->size) {
        return;
    }
    if (pos_1 == pos_2) {
        return;
    }
    int start;
    int end;

    if (pos_1 >= pos_2) {
        start = pos_2;
        end = pos_1;
    } else {
        start = pos_1;
        end = pos_2;
    }

    int length = (end - start);


    gb->copy_size = length;
    gb->copied_text = memset(gb->copied_text, 0, length);
    int cursor = gb->cursor;

    if(cursor == end){
        memcpy(gb->copied_text, gb->buf + start, length);
    }
    else if(cursor == start){
        memcpy(gb->copied_text, gb->buf + gb->gap_end, length);
    }

} 
void paste(gapbuf *gb) {
    if (gb->copy_size == 0) {
        return;
    }
    int size;
    int gap_size = gb->gap_end - gb->cursor;
    if (0 == gap_size) {
        return;
    }
    if (gb->copy_size > gap_size) {
        size = gap_size;
    }
    else {
        size = gb->copy_size;
    }
    memcpy(gb->buf + gb->cursor, gb->copied_text, size);

    gb->cursor += size;
}