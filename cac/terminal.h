#include <stdio.h>
#include <sys/ioctl.h>
#include <unistd.h>
#include <linux/limits.h>
extern char input_buf[ARG_MAX];
;
extern int input_len;
extern int cursor;
struct winsize get_terminal_size();
void enable_raw_mode();
void disable_raw_mode();
void move_cursor_right(int index);
void move_cursor_left(int index);
void clear_line();
