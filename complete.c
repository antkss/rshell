#include <stdio.h>
#include "complete.h"
#include <dirent.h>            
#include <readline/readline.h> 
#include <stddef.h>            
#include <stdlib.h>            
#include <unistd.h>            
#include "command.h"           
#include "utils.h"             
#include <stdio.h>             
#include <string.h>            
extern CommandEntry command_table[];

int is_executable(const char *path) {
    return access(path, X_OK) == 0;
}

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

        if (paths) free(paths);
        paths = dupstr(getenv("PATH"));
        dir = strtok(paths, ":");

        if (dp) closedir(dp);
        dp = NULL;
    }

    // Phase 0: PATH executables
    if (phase == 0) {
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

    // Phase 1: Built-in commands[]
    if (phase == 1) {
        while (command_table[cmd_index].name) {
			CommandEntry command = command_table[cmd_index++];
            const char *cmd = command.name;
            if (command.enable != 0 && !strncmp(text, cmd, len)) {
                return dupstr(cmd);
            }
        }
        phase = 2;
        dp = opendir(".");
    }

    // Phase 2: Folders from current dir
    if (phase == 2 && dp) {
        while ((entry = readdir(dp))) {
            if (entry->d_type == DT_DIR && !strncmp(entry->d_name, text, len)) {
                return dupstr(entry->d_name);
            }
        }
        closedir(dp);
        dp = NULL;
    }

    if (paths) {
        free(paths);
        paths = NULL;
    }

    return NULL;
}
char **my_completion(const char *text, int start, int end) {
    // Optionally check position (e.g., start of line, after command, etc.)
    rl_completion_append_character = ' '; // or '\0' to disable space

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
