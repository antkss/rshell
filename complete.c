#include "complete.h"
#include <readline/history.h>
#include <readline/readline.h>
#include "command.h"
char *logger = NULL;
size_t logger_len = 0;
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
int my_key_handler(int count, int key) {
    printf("\n[Key Press Detected: %d]\n", key);
    rl_on_new_line();
    rl_redisplay();
    return 0; // return 0 to keep readline input buffer
}
int key_logger(int count, int key) {
	int capacity = 0x100;
	if (!logger) {
		logger = malloc(0x100);
	}
    // printf("%c", key);  // Log the character
    // Insert it back into the line buffer
    char s[2] = {key, '\0'};
    rl_insert_text(s);
    rl_redisplay();
    return 0;
}
