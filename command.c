#include "command.h"
#include "terminal.h"
#include <linux/limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/ioctl.h>
#include <fcntl.h>
#include <sys/types.h>
#include <unistd.h>
#include <sys/stat.h>
#include "parser.h"
#include <stdarg.h>

#define READ_BYTES 4096
void clear_cmd(char **args, int argc) {
	printf("\033c");
}
int parse_option(char **args, int argc, char **option) {
	int option_cnt = 0;
	for (int i = 1; i < argc; i++) {
		if (!strncmp(args[i], "-", 1)) {
			// shell_print("option parsed ! %s\n", args[i]);
			option[option_cnt] = args[i];
			args[i] = NULL;
			option_cnt++;
		}
	}
	return option_cnt;
}
void print_sign(char sign[]) {
	fflush(stdout);
	printf("%s", sign);
	fflush(stdout);
}
int is_dir(char *path) {
    struct stat st;
    if (stat(path, &st) == 0 && S_ISDIR(st.st_mode))
        return 1;
    return 0;
}

void shell_print(const char *format, ...) {
    va_list args;
    fflush(stdout);            // Flush before printing
    va_start(args, format);    // Start variadic argument processing
    vprintf(format, args);     // Print using the format string
    va_end(args);              // Clean up
    fflush(stdout);            // Flush after printing
}
int is_exist(char *buffer, char *buff_array[], int count) {
    for (int i = 0; i < count; i++) {
        if (strcmp(buffer, buff_array[i]) == 0) {
            return 1; // buffer already exists in the array
        }
    }
    return 0;
}

int is_file_exist(char *path) {
	FILE *file = fopen(path, "r");
    if (file) {
        fclose(file);
		return 1;
    } 
    return 0;
}

void cd_cmd(char **args, int argc) {
    // If no path provided, default to HOME
	char *path = args[1];
    if (path == NULL) {
        path = getenv("HOME");  // Get home directory from environment
        if (path == NULL) {
            fprintf(stderr, "cd: HOME not set\n");
            return;
        }
    }

    // Change directory
    if (chdir(path) != 0) {
        perror("cd failed");  // Print error
    }
}
int find_pos_token(char **args, int argc, char *target, int cur_pos) {
	for (int i = cur_pos; i < argc; i++) {
		if (strcmp(args[i], "|") == 0) {
			return i;
		}
	}
	return -1;
}

void exit_cmd(char **args, int argc) {
	disable_raw_mode();
	exit(0);
}
void pretty_print_list(char *files[], const char *path, int file_count) {
	char full_path[PATH_MAX];


    // Step 1: Get longest file name
    int max_len = 0;
    for (int i = 0; i < file_count; i++) {
		if (files[i] != NULL) {
			int len = strlen(files[i]);
			if (len > max_len)
				max_len = len;
		}

    }

    // Add spacing (e.g. 2 spaces between columns)
    int col_width = max_len + 2;

    // Step 2: Get terminal width and number of columns
    int term_width = get_terminal_size().ws_col;
    int num_cols = term_width / col_width;
    if (num_cols < 1) num_cols = 1;

    // Step 3: Print in column-major order
    int num_rows = (file_count + num_cols - 1) / num_cols;
    for (int row = 0; row < num_rows; row++) {
        for (int col = 0; col < num_cols; col++) {
            int idx = col * num_rows + row;
            if (idx < file_count) {
                // printf("%-*s", col_width, files[idx]);
				snprintf(full_path, sizeof(full_path), "%s/%s", path, files[idx]);
				// printf("%s ", full_path);
				struct stat st;
				if (stat(full_path, &st) == 0) {
				    if (S_ISDIR(st.st_mode)) {
				        printf("%s%-*s%s", COLOR_BLUE, col_width, files[idx], COLOR_RESET);
				    } else if (S_ISREG(st.st_mode) && (st.st_mode & S_IXUSR)) {
				        printf("%s%-*s%s", COLOR_GREEN, col_width, files[idx], COLOR_RESET);
				    } else {
				        printf("%s%-*s%s", COLOR_WHITE, col_width, files[idx], COLOR_RESET);
				    }
				} else {
				    printf("%-*s", col_width, files[idx]);
				}
			}

        }
        printf("\n");
    }

}
void get_list_files(const char *path, char* content[], int *count) {
    DIR *dir ;
	if (path == NULL) return;
	dir = opendir(path);
    if (dir == NULL) {
        perror("opendir failed");
        return;
    }

    struct dirent *entry;
    int first = 1;

	*count = 0;
    while ((entry = readdir(dir)) != NULL) {
        if (strcmp(entry->d_name, ".") != 0 && strcmp(entry->d_name, "..") != 0) {
			unsigned int size = strlen(entry->d_name);
			content[*count] = malloc(size + 0x10);
			memset(content[*count], 0, size + 10);
			strncpy(content[*count], entry->d_name, size);
			*count += 1;
        }
    }
    closedir(dir);
}
void ls_cmd(char **args, int argc) {
	char *path = args[1];
	char *content[XATTR_LIST_MAX];
	int count = 0;
	if (path == NULL) path = ".";
	get_list_files(path, content, &count);
	pretty_print_list(content, path, count);
	for (int i = 0; i < count; i++) {
		free(content[i]);
	}
}
void cat_cmd(char **args, int argc) {
	// printf("number of files %d\n", argc);
	char buff[READ_BYTES + 0x10];
	for (int i = 1; i < argc; i++) {
		if (!is_dir(args[i])) {
			int fd = open(args[i], O_RDONLY);
			if (fd == -1) {
				perror("open error");
				return;
			}
			off_t pos = lseek(fd, 0, SEEK_END);
			if (pos == -1) {
				perror("lseek");
				return;
			}
			ulong file_cap = pos;
			// printf("cap: %lu \n", file_cap);
			lseek(fd, 0, SEEK_SET);
			while (1) {
				memset(buff, 0, READ_BYTES + 0x10);
				if (file_cap <= READ_BYTES) {
					// printf("reading final\n");
					read(fd, buff, file_cap);
					shell_print("%s", buff);
					break;
				} else {
					read(fd, buff, READ_BYTES); // continue to read the file 
					shell_print("%s", buff);
				}
				file_cap -= READ_BYTES;
			}
			close(fd);
		}

	}
	printf("\n");
}
void echo_cmd(char **args, int argc) {
	for (int i = 1; i < argc; i++) {
		shell_print("%s", args[i]);
		shell_print(" ");
	}
	shell_print("\n");
}
void mkdir_cmd(char **args, int argc) {
	int default_mode = 0700;
	for (int i = 1; i < argc; i++) {
		struct stat st = {0};
		if (stat(args[i], &st) == -1) {
			mkdir(args[i], default_mode);
		} else {
			shell_print("object exist !\n");
		}
	}
}
void touch_cmd(char **args, int argc) {
	for (int i = 1; i < argc; i++) {
		struct stat st = {0};
		if (stat(args[i], &st) == -1) {
            FILE *fp = fopen(args[i], "w");
			fclose(fp);
		} else {
			shell_print("object %s exist !\n");
		}
	}
}
void remove_recursive(char *path) {
    if (is_dir(path)) {
        DIR *dir = opendir(path);
        if (!dir) {
            perror("opendir");
            return;
        }

        struct dirent *entry;
        while ((entry = readdir(dir)) != NULL) {
            // Skip "." and ".."
            if (strcmp(entry->d_name, ".") == 0 || strcmp(entry->d_name, "..") == 0)
                continue;

            char fullpath[PATH_MAX];
            snprintf(fullpath, sizeof(fullpath), "%s/%s", path, entry->d_name);

            if (is_dir(fullpath)) {
                remove_recursive(fullpath); // Recurse into subdirectory
            } else {
                if (remove(fullpath) != 0) {
                    perror("remove");
                }
            }
        }

        closedir(dir);

        // Remove the now-empty directory
        if (rmdir(path) != 0) {
            perror("rmdir");
        }
    } else {
        // It's a file; just remove it
        if (remove(path) != 0) {
            perror("remove");
        }
    }
}

void rm_cmd(char **args, int argc) {
	char *option[6];
	int op_num =  parse_option(args, argc, option);
	char *content[XATTR_LIST_MAX];
	int count;
	int recursive = 0;
	// shell_print("op_num %d \n", op_num);
	for (int i = 0; i < op_num; i++) {
		if (!strcmp(option[i], "-r")) {
			recursive = 1;
		} else {
			shell_print("invalid option %s !\n", option[i]);
		}
	};
	if (argc < 2) {
		shell_print("nothing to create \n");
	} else {
		for (int i = 1; i < argc; i++) {
			if (args[i]) {
				shell_print("removing %s \n", args[i]);
				if (!is_dir(args[i])) {
					if (remove(args[i]) < 0) {
					   perror("can't remove ");
					}
				} else if (recursive) {
					remove_recursive(args[i]);
				} else {
					shell_print("can't remove without -r \n");
				}

			}
		}
	}

}

void cp_cmd(char **args, int argc) {
}
CommandEntry command_table[] = {
    CMD_ENTRY(ls),
    CMD_ENTRY(cd),
    CMD_ENTRY(exit),
	CMD_ENTRY(clear),
	CMD_ENTRY(cat),
	CMD_ENTRY(mkdir),
	CMD_ENTRY(rm),
	CMD_ENTRY(touch),
	CMD_ENTRY(cp),
	CMD_ENTRY(echo),
	
};
void call_command(const char *cmd, char **args, int argc) {
    for (int i = 0; i < NUM_COMMANDS; ++i) {
        if (strcmp(command_table[i].name, cmd) == 0) {
            command_table[i].fn(args, argc);
            return;
        }
    }
    printf("command not found :< %s\n", cmd);
}

