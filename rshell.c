#include "command.h"
#include <linux/limits.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/ioctl.h>
#include <unistd.h>
#include "parser.h"
#include <readline/readline.h>
#include <readline/history.h>
#include "config.h"
char *home = NULL;
pid_t child_pid = -1;
char *input;

void handle_sigint(int sig) {
	// shell_print("child: %d \n", child_pid);
    if (child_pid > 0) {
        // Forward SIGINT to the child process
        kill(child_pid, SIGINT);
		return;
    }

    rl_replace_line("", 0);
    rl_crlf(); // newline
    rl_on_new_line(); // move to new line
    rl_redisplay();
}

void read_config() {
	char *config_path = malloc(strlen(home) + strlen(CONFIG_FILE_NAME) + 0x10);
	sprintf(config_path, "%s/%s", home, CONFIG_FILE_NAME);
	char *config_read = read_file(config_path);
	parse_call(config_read, strlen(config_read));
	free(config_path);
	free(config_read);
}

int main() {
	home = getenv("HOME");
	read_config();
	shell_print("wellcome to my shell \n");
	signal(SIGINT, handle_sigint);

    while ((input = readline(GENERIC_PSI)) != NULL) {
        if (*input) {
            add_history(input);  // Enable up/down arrow history
			parse_call(input, strlen(input));
			// shell_print("%s", input);
        }
        free(input);
		input = NULL;
    }

	return 0;
}

