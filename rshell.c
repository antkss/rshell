#include "command.h"
#include <linux/limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/ioctl.h>
#include <unistd.h>
#include "terminal.h"
#include "parser.h"
#define MAX_HISTORY 0x1000

char input_buf[ARG_MAX];
int input_len = 0;
char *command_history[MAX_HISTORY];
unsigned int history_cnt = 0;
unsigned int history_idx = 0;
int counter = 0;
int line_counter = 0;
int len_line_counter = 0;
struct winsize term_size;

void delete_after_cursor() {
    input_buf[cursor] = '\0';
	char *next_buf = &input_buf[cursor + 1];
	// shell_print("next_buf %s ", next_buf);
	strcat(input_buf, next_buf);
	// printf(" | input len: %d\n", input_len);
	// printf(" | cursor: %d\n", cursor);
	clear_line();
    write(STDOUT_FILENO, input_buf, input_len);
	move_cursor_left(input_len - cursor);
	// r@🍎 | lmaodark |
	//    start       end
	//  the cursor variable from the start is 0
	//  and at the end of the buffer is actually the buffer len, the real position is 0 from the end of the buffer to the next slot
}
void input_line_handle(char *input_buf, int input_len) {
	write(STDOUT_FILENO, "\033[A", 3); // move up
	line_counter += 1;
}
void insert_char(char ch, char *buf) {
	char *part_1 = buf;
	char *part_2 = &buf[cursor];
	int len_part2 = input_len - cursor;
	
	if (len_part2 < 0) {
		shell_print("len_part2 error cursor: %d, input_len: %d \n", cursor, input_len);
		return;
	}
	if (len_part2 >= 0) {
		char *tmp = malloc(len_part2 + 0x10);
		strncpy(tmp, part_2, len_part2);
		buf[cursor] = ch;
		buf[cursor + 1] = 0;
		strcat(buf, tmp);
		free(tmp);
	} else {
		input_buf[input_len] = ch;
	}
	cursor++;
	input_len++;
	// len_line_counter = input_len % term_size.ws_col ;
	clear_line();
	// write(STDOUT_FILENO, "\033[A", 3); // move up
	// clear_line();
    write(STDOUT_FILENO, input_buf, input_len);
	move_cursor_left(input_len - cursor);
}


int main() {

	char *history = NULL;
	unsigned int history_len = 0;

	shell_print("wellcome to my shell \n");
    enable_raw_mode();
	while(1) {
		term_size = get_terminal_size();
		// shell_print("new buffer\n");
		print_sign(GENERIC_PSI);
		move_cursor_right(0);
		cursor = 0;
		input_len = 0;
		history_idx = 0;
		counter = history_cnt;

		while (1) {
			char c;
			read(STDIN_FILENO, &c, 1);
			// printf("byte: %d\n", (int)c);
			if (c == '\x1b') { // Escape sequence
				char seq[2];
				if (read(STDIN_FILENO, &seq[0], 1) == 0) continue;
				if (read(STDIN_FILENO, &seq[1], 1) == 0) continue;
				if (seq[0] == '[') {
					if (seq[1] == 'A') {
						if (history_cnt > 0) {
							counter--;
							history_idx = counter % history_cnt;
							history = command_history[history_idx];
							cursor = input_len = strlen(history);
							clear_line();
							shell_print("%s", history);
							// shell_print("idx: %d", history_idx);
							strncpy(input_buf, history, input_len);

						}
					} else if (seq[1] == 'B') {
						if (history_cnt > 0) {
							counter++;
							history_idx = counter % history_cnt;
							history = command_history[history_idx];
							cursor = input_len = strlen(history);
							clear_line();
							shell_print("%s", history);
							// shell_print("idx: %d", history_idx);
							strncpy(input_buf, history, input_len);

						} 
					} else if (seq[1] == 'C') { // move cusor to right
						// shell_print("right");
						if (cursor < input_len) {
							cursor++;
							write(STDOUT_FILENO, "\033[1C", 4);
						}
						// shell_print("cursor: %d \n", cursor);
					} else if (seq[1] == 'D') {
						// shell_print("left");
						if (cursor > 0) {
							cursor--;
							write(STDOUT_FILENO, "\033[1D", 4);
						}
					}
				}
			} else if (c == 127 || c == 8) {  // Backspace
				if (input_len > 0 && cursor > 0) {
					input_len--;
					cursor--;
					delete_after_cursor();
				}
			} else if (c == 5) {
				move_cursor_right(0);

			} else if (c == 4) {
				if (input_len == 0) {
					shell_print("ctrl D");
					disable_raw_mode();
					exit(0);
				}
			} else if (c == '\r' || c == '\n') {
				input_buf[input_len] = '\0';
				printf("\n");
				break;
			} else if (c == 3) {
				// shell_print("ctrl c works");
				input_len = 0;
				cursor = 0;
				memset(input_buf, 0, sizeof(input_buf));
				printf("\n");
				print_sign(GENERIC_PSI);
				move_cursor_right(0);
				continue;
			} else if (c == '\t') {
				move_cursor_left(0);
				// shell_print("tab works");
				// char *ls_args[2] = {0};
				// printf("\n");
				// ls_cmd(ls_args, 2);
				// print_sign(GENERIC_PSI);
				// move_cursor_right(0);
				// write(STDOUT_FILENO, input_buf, input_len);
				continue;
			} else {
				if (input_len < sizeof(input_buf) - 1) {
					insert_char(c, input_buf);
				}
			}
		}
		parse_call(input_buf, input_len);

	}
	return 0;
}

