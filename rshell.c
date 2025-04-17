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
	size_t read_size;
	char *config_path = malloc(strlen(home) + strlen(CONFIG_FILE_NAME) + 0x10);
	sprintf(config_path, "%s/%s", home, CONFIG_FILE_NAME);
	char *config_read = read_file(config_path, &read_size);
	if (read_size < ARG_MAX && config_read) {
		parse_call(config_read);
		free(config_path);
		free(config_read);
	}

}
char *my_generator(const char *text, int state) {
    static int list_index, len;
    static const char *commands[] = {
        "echo", "exit", "help", "clear", "cd", "ls", "cat", NULL
    };

    if (!state) {
        list_index = 0;
        len = strlen(text);
    }

    while (commands[list_index]) {
        const char *cmd = commands[list_index++];
        if (strncmp(cmd, text, len) == 0) {
            return strdup(cmd);
        }
    }

    return NULL;
}
char **my_completion(const char *text, int start, int end) {
    // Optionally check position (e.g., start of line, after command, etc.)
    rl_completion_append_character = ' '; // or '\0' to disable space

    return rl_completion_matches(text, my_generator);
}

int main() {
	rl_attempted_completion_function = my_completion;
	home = getenv("HOME");
	read_config();
	shell_print("wellcome to my shell \n");
	signal(SIGINT, handle_sigint);
	char *psi_set = getenv("PSI_RSHELL");
	if (psi_set == NULL) psi_set = GENERIC_PSI;
    while ((input = readline(GENERIC_PSI)) != NULL) {
        if (*input) {
            add_history(input);  // Enable up/down arrow history
			parse_call(input);
			// shell_print("%s", input);
        }
        free(input);
		input = NULL;
    }

	return 0;
}

