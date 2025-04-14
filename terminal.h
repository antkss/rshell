#include <stdio.h>
#include <sys/ioctl.h>
#include <unistd.h>
#include <linux/limits.h>
#include <sys/stat.h>
mode_t get_perm(const char *filename);
extern char input_buf[ARG_MAX];
extern int input_len;
extern int cursor;
extern int line_cursor;
struct winsize get_terminal_size();
void move_cursor_right(int index);
void move_cursor_left(int index);
void clear_line();
mode_t get_perm(const char *filename);
void clearln();
void set_raw_mode(int enable);
void delete_at_cursor();
void clear_chars(int n);
