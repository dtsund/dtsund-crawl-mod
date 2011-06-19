#ifndef LIBW32C_H
#define LIBW32C_H

#ifndef USE_TILE

#include <string>
#include <stdarg.h>
#include <stdio.h>

class crawl_view_buffer;

void init_libw32c(void);
void deinit_libw32c(void);

int get_number_of_lines();
int get_number_of_cols();

void set_cursor_enabled(bool enabled);
bool is_cursor_enabled();

void clrscr(void);
void clear_to_end_of_line();
void gotoxy_sys(int x, int y);
void textcolor(int c);
void textattr(int c);
void cprintf(const char *format, ...);
// void cprintf(const char *s);
void set_string_input(bool value);
bool set_buffering(bool value);
int get_console_string(char *buf, int maxlen);
void print_timings(void);

int wherex(void);
int wherey(void);
void putwch(wchar_t c);
int getchk(void);
int getch_ck(void);
bool kbhit(void);
void delay(int ms);
void textbackground(int c);
void puttext(int x, int y, const crawl_view_buffer &vbuf);
void update_screen();

void enable_smart_cursor(bool cursor);
bool is_smart_cursor_enabled();
void set_mouse_enabled(bool enabled);

inline void put_colour_ch(int colour, unsigned ch)
{
    textattr(colour);
    putwch(ch);
}

#endif

#endif
