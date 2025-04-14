#include "terminal.h"
#include <sys/ioctl.h>
#include <termios.h>
#include "command.h"
int cursor = 0;
int line_cursor = 0;
tcflag_t old_lflag;
cc_t     old_vtime;
struct termios term;

struct winsize get_terminal_size(){
	struct winsize w;

	if (ioctl(STDOUT_FILENO, TIOCGWINSZ, &w) == -1) {
		perror("ioctl error");
	}
	return w;
}

void set_raw_mode(int enable) {
    static struct termios newt;
    if (enable) {
		if( tcgetattr(STDIN_FILENO, &term) < 0 ) {
			perror("tcgetattr");
			exit(1);
		}
		old_lflag = term.c_lflag;
		old_vtime = term.c_cc[VTIME];
		// printf("lflag: %u | old_vtime: %u \n", old_lflag, old_vtime);
		term.c_lflag &= ~(ICANON | ECHO | ISIG);
		term.c_cc[VTIME] = 1;
		if( tcsetattr(STDIN_FILENO, TCSANOW, &term) < 0 ) {
			perror("tcsetattr");
			exit(1);
		}
    } else {
		term.c_lflag     = old_lflag;
		term.c_cc[VTIME] = old_vtime;
		// printf("lflag: %u | old_vtime: %u \n", old_lflag, old_vtime);
		if( tcsetattr(STDIN_FILENO, TCSANOW, &term) < 0 ) {
			perror("tcsetattr");
			exit(1);
		}
    }
}

void delete_forward_chars(int n) {
    char buf[16];
    snprintf(buf, sizeof(buf), "\x1b[%dP", n);  // DCH - Delete Character
    write(STDOUT_FILENO, buf, strlen(buf));
}

void clear_chars(int n) {
	for (int i = 0; i < n; i++) {
		delete_at_cursor();
	}
}
void clear_line() {
	write(STDOUT_FILENO, "\r\033[2K", 5); // remove stdout current line
	shell_print("%s", GENERIC_PSI);
}

void clearln() {
	write(STDOUT_FILENO, "\033[2K", 5); // remove stdout current line
	shell_print("%s", GENERIC_PSI);
	move_cursor_right(0);	
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
void move_cursor_left(int n) {
    if (n > 0) {
        char seq[16];
        snprintf(seq, sizeof(seq), "\033[%dD", n);  // D = left
        write(STDOUT_FILENO, seq, strlen(seq));
    }
}
