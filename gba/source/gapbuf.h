
#ifndef GAPBUF_H
#define GAPBUF_H



// gap buffer sizes
#define gb_left(gb) ((gb)->cursor)                  // size of text before cursor
#define gb_right(gb)  ((gb)->size - (gb)->gap_end)   // size of text after cursor
#define gb_used(gb)  (gb_left (gb) + gb_right (gb)) // total number of used characters



typedef struct
{
    int size;//size of the buffer not the gap
    int cursor;//gap_start
    int gap_end;
    char *buf;
	char *copied_text;
    int copy_size;

} gapbuf;

gapbuf *create_gapbuf(int size);

void move_right_txt(gapbuf *gb, char *new_buf, int new_size);
void move_gap_to(gapbuf *gb, int pos);

void insert(gapbuf *gb, char c);

void backspace(gapbuf *gb);
void delete(gapbuf *gb);

void cursor_left(gapbuf *gb);
void cursor_right(gapbuf *gb);



char *extract_text(gapbuf *gb);
char get_char_at(gapbuf *gb, int pos);

void print_buffer(gapbuf *gb);
void select_delete(gapbuf *gb, int pos);

void extract_text_between_pos (gapbuf *gb, int pos_1, int pos_2);
void paste(gapbuf *gb);

#endif //GAPBUF_H