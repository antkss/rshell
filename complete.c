#include <stdio.h>
#include "complete.h"
#include <dirent.h>
#include <readline/readline.h>
#include <stddef.h>
#include <stdlib.h>
#include <unistd.h>
#include "command.h"
#include "utils.h" // Assuming dupstr is defined here or similar
#include <string.h>

extern CommandEntry command_table[];

int is_executable(const char *path) {
    return access(path, X_OK) == 0;
}

// Global variable to indicate the type of completion needed
// 0: Command completion (default)
// 1: Directory completion
static int completion_mode = 0;

char *my_generator(const char *text, int state) {
    static int phase = 0;
    static int len;
    static char *paths = NULL;
    static char *dir = NULL;
    static DIR *dp = NULL;
    static struct dirent *entry = NULL;
    static int cmd_index = 0;

    if (state == 0) {
        // Reset everything
        phase = 0;
        len = strlen(text);
        cmd_index = 0;

        if (paths) {
            free(paths);
            paths = NULL;
        }
        if (dp) {
            closedir(dp);
            dp = NULL;
        }

        // Only get PATH once per completion attempt, if needed
        if (completion_mode == 0) { // Only for command completion
            paths = dupstr(getenv("PATH"));
            dir = strtok(paths, ":");
        } else { // For directory completion, start directly at Phase 2
            phase = 2; // Skip command phases
            dp = opendir("."); // Start with current directory for files/folders
        }
    }

    // Phase 0: PATH executables (only for command completion)
    if (completion_mode == 0 && phase == 0) {
        while (dir) {
            if (!dp) dp = opendir(dir);
            if (dp) {
                while ((entry = readdir(dp))) {
                    if (strcmp(entry->d_name, ".") == 0 || strcmp(entry->d_name, "..") == 0)
                        continue;

                    char fullpath[4096];
                    snprintf(fullpath, sizeof(fullpath), "%s/%s", dir, entry->d_name);
                    if (is_executable(fullpath)) {
                        if (!strncmp(text, entry->d_name, len)) {
                            return dupstr(entry->d_name);
                        }
                    }
                }
                closedir(dp);
                dp = NULL;
            }
            dir = strtok(NULL, ":");
        }

        // No PATH match, move to command array
        phase = 1;
    }

    // Phase 1: Built-in commands[] (only for command completion)
    if (completion_mode == 0 && phase == 1) {
        while (command_table[cmd_index].name) {
            CommandEntry command = command_table[cmd_index++];
            const char *cmd = command.name;
            if (command.enable != 0 && !strncmp(text, cmd, len)) {
                return dupstr(cmd);
            }
        }
        // If we are still in command completion mode and found no commands,
        // it means we are done with command suggestions.
        // We do *not* transition to phase 2 here for command completion.
        // Instead, the generator will return NULL and my_completion will decide
        // if it needs to switch to directory completion.
        phase = 2; // Transition to next phase if no command found, but this is handled differently now.
    }

    // Phase 2: Folders from current dir (used for both command and argument completion)
    // For command completion, this is only reached if no command matches
    // For argument completion, we directly start here.
    if (phase == 2) {
        if (!dp) dp = opendir("."); // Ensure dp is open for current directory
        if (dp) {
            while ((entry = readdir(dp))) {
                // If completing a command, we want to suggest executables in current dir too
                // If completing an argument, we want to suggest directories (and potentially files)
                if (entry->d_type == DT_DIR && !strncmp(entry->d_name, text, len)) {
                    // For directories, append a '/'
                    char *result = (char *)malloc(strlen(entry->d_name) + 2);
                    if (result) {
                        strcpy(result, entry->d_name);
                        strcat(result, "/");
                    }
                    return result;
                }
                // Optional: add file completion here for arguments if desired
                // if (entry->d_type == DT_REG && !strncmp(entry->d_name, text, len)) {
                //     return dupstr(entry->d_name);
                // }
            }
            closedir(dp);
            dp = NULL;
        }
    }

    // Cleanup static variables at the very end
    if (paths) {
        free(paths);
        paths = NULL;
    }
    if (dp) { // Ensure dp is closed if it was opened and not yet closed
        closedir(dp);
        dp = NULL;
    }

    return NULL;
}

char **my_completion(const char *text, int start, int end) {
    // Determine the context for completion
    // If 'start' is 0, it means we are at the beginning of the line, so complete commands.
    // Otherwise, we are completing an argument.
    if (start == 0) {
        completion_mode = 0; // Command completion
        rl_completion_append_character = ' ';
    } else {
        completion_mode = 1; // Argument (directory) completion
        // If the current completion is a directory, append a '/'
        // Otherwise, append a space
        if (text[strlen(text) - 1] == '/') {
             rl_completion_append_character = '\0'; // No space after a directory
        } else {
             rl_completion_append_character = ' ';
        }
    }

    return rl_completion_matches(text, my_generator);
}

int key_logger(int count, int key) {
    // Insert it back into the line buffer
    char s[2] = {key, '\0'};
    rl_insert_text(s);
    rl_redisplay();
    return 0;
}

void completion_setup() {
    rl_attempted_completion_function = my_completion;
}
