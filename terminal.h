#include <sys/ioctl.h>
#ifndef XATTR_LIST_MAX
#define XATTR_LIST_MAX 65536
#endif
struct winsize get_terminal_size();
void move_cursor_right(int index);
void move_cursor_left(int index);
void delete_at_cursor();
void save_cursor();
void restore_cursor();
void move(int x, int y);
void move_prev_end();
void move_next_start();

