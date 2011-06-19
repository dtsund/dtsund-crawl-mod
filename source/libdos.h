/**
 * @file
 * @brief Functions for DOS support.
**/

#ifndef __LIBDOS_H__
#define __LIBDOS_H__

#include <conio.h>
#include <stdio.h>

void init_libdos();

int get_number_of_lines();
int get_number_of_cols();

inline void gotoxy_sys(int x, int y) { gotoxy(x, y); }
inline void enable_smart_cursor(bool) { }
inline bool is_smart_cursor_enabled()  { return (false); }
void set_cursor_enabled(bool enabled);
bool is_cursor_enabled();
void clear_to_end_of_line();
int getch_ck();
static inline void set_mouse_enabled(bool enabled) { }

inline void update_screen()
{
}

void putwch(unsigned c);

inline void put_colour_ch(int colour, unsigned ch)
{
    textattr(colour);
    putwch(ch);
}

#endif
