#include "terminal.h"
#include "command.h"
#include <stdio.h>
#include <unistd.h>
#include <string.h>
struct winsize get_terminal_size(){
	struct winsize w;

	if (ioctl(STDOUT_FILENO, TIOCGWINSZ, &w) == -1) {
		perror("ioctl error");
	}
	return w;
}
void move_cursor_left(int n) {
    if (n > 0) {
        char seq[16];
        snprintf(seq, sizeof(seq), "\033[%dD", n);  // D = left
        write(STDOUT_FILENO, seq, strlen(seq));
    }
}
void move_cursor_right(int index) {
    char seq[32];
    snprintf(seq, sizeof(seq), "\033[%dC", index); // C = right
    write(STDOUT_FILENO, seq, strlen(seq));
}
void delete_at_cursor() {
	move_cursor_left(1);
	shell_print("\033[1P");
}


void save_cursor() {
	shell_print("\033[s");
}
void restore_cursor() {
	shell_print("\033[u");
}
void move(int x, int y) {
	shell_print("\033[%d;%dH");
}
void move_prev_end() {
    struct winsize w = get_terminal_size();
    shell_print("\033[A");
    shell_print("\033[%dC", w.ws_col - 1);
    fflush(stdout);
}
void move_next_start() {
	shell_print("\033[E");
    fflush(stdout);
}
