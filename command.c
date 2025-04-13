#include "command.h"
#include "terminal.h"
#include <linux/limits.h>
#include <inttypes.h>
#include <stdio.h>
#include <string.h>
#include <sys/ioctl.h>
#include <fcntl.h>
#include <sys/types.h>
#include <unistd.h>
#include <sys/stat.h>
#include "parser.h"
#include <stdarg.h>

#define READ_BYTES 4096
enum {
	FOLDER, 
	REGULAR_FILE, 
	UNKNOWN,
};

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

// char* mstrcat(int count, ...) {
//     va_list args;
//     va_start(args, count);
// 	int total = 0;
// 	char **buf_list = malloc(sizeof(char*) * count);
// 	for (int i = 0; i < count; i++) {
//         char *buf = va_arg(args, char *);
// 		buf_list[i] = buf;
// 		total += strlen(buf);
// 	}
// 	char *cat_buf = malloc(total + 0x10);
//     for (int i = 0; i < count; ++i) {
// 		strcat(cat_buf, buf_list[i]);
//     }
//     va_end(args);
// 	return cat_buf;
// }
int check_path(const char *path) {
    struct stat st;
    if (stat(path, &st) == 0) {
        if (S_ISDIR(st.st_mode)) {
			return FOLDER;
        } else if (S_ISREG(st.st_mode)) {
			return REGULAR_FILE;
        } else {
			return UNKNOWN;
        }
    } else {
		return -1;
    }
	return -2;
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
NEW_CMD(clear) {
	printf("\033c");
	return 0;
}
NEW_CMD(cd) {
    // If no path provided, default to HOME
	char *path = args[1];
    if (path == NULL) {
        path = getenv("HOME");  // Get home directory from environment
        if (path == NULL) {
            fprintf(stderr, "cd: HOME not set\n");
            return -1;
        }
    }

    // Change directory
    if (chdir(path) != 0) {
        perror("cd failed");  // Print error
    }
	return 0;
}
int find_pos_token(char **args, int argc, char *target, int cur_pos) {
	for (int i = cur_pos; i < argc; i++) {
		if (strcmp(args[i], "|") == 0) {
			return i;
		}
	}
	return -1;
}

NEW_CMD(exit) {
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
int get_list_files(const char *path, char* content[], int *count) {
    DIR *dir ;
	if (path == NULL) return -1;
	dir = opendir(path);
    if (dir == NULL) {
        perror("opendir failed");
        return -1;
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
	return 0;
}
NEW_CMD(ls){
	char *path = args[1];
	char *content[XATTR_LIST_MAX];
	int count = 0;
	if (path == NULL) path = ".";
	int value = get_list_files(path, content, &count);
	pretty_print_list(content, path, count);
	for (int i = 0; i < count; i++) {
		free(content[i]);
	}
	return value;
}
NEW_CMD(cat) {
	// printf("number of files %d\n", argc);
	char buff[READ_BYTES + 0x10];
	for (int i = 1; i < argc; i++) {
		if (!is_dir(args[i])) {
			int fd = open(args[i], O_RDONLY);
			if (fd == -1) {
				perror("open error");
				return -1;
			}
			off_t pos = lseek(fd, 0, SEEK_END);
			if (pos == -1) {
				perror("lseek");
				return -1;
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
	return 0;
}
NEW_CMD(echo) {
	for (int i = 1; i < argc; i++) {
		shell_print("%s", args[i]);
		shell_print(" ");
	}
	shell_print("\n");
	return 0;
}
NEW_CMD(mkdir) {
	int default_mode = 0700;
	for (int i = 1; i < argc; i++) {
		struct stat st = {0};
		if (stat(args[i], &st) == -1) {
			int value = mkdir(args[i], default_mode);
			if (value < 0) {
				perror("mkdir error");
				return -1;
			}
		} else {
			shell_print("object exist !\n");
			return -1;
		}
	}
	return 0;
}
NEW_CMD(touch) {
	for (int i = 1; i < argc; i++) {
		struct stat st = {0};
		if (stat(args[i], &st) == -1) {
            FILE *fp = fopen(args[i], "w");
			fclose(fp);
		} else {
			shell_print("object %s exist !\n");
			return -1;
		}
	}
	return 0;
}
int copy_recursive(char *path) {
    if (is_dir(path)) {
        DIR *dir = opendir(path);
        if (!dir) {
            perror("opendir");
            return -1;
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
					return -1;
                }
            }
        }

        closedir(dir);

        // Remove the now-empty directory
        if (rmdir(path) != 0) {
            perror("rmdir");
			return -1;
        }
    } else {
        // It's a file; just remove it
        if (remove(path) != 0) {
            perror("remove");
			return -1;
        }
    }
	return 0;
}
int remove_recursive(char *path) {
    if (is_dir(path)) {
        DIR *dir = opendir(path);
        if (!dir) {
            perror("opendir");
            return -1;
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
					return -1;
                }
            }
        }

        closedir(dir);

        // Remove the now-empty directory
        if (rmdir(path) != 0) {
            perror("rmdir");
			return -1;
        }
    } else {
        // It's a file; just remove it
        if (remove(path) != 0) {
            perror("remove");
			return -1;
        }
    }
	return 0;
}

NEW_CMD(rm) {
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
			return -1;
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
					   return -1;
					}
				} else if (recursive) {
					remove_recursive(args[i]);
				} else {
					shell_print("can't remove without -r \n");
					return -1;
				}

			}
		}
	}
	return 0;
}
int copy_file(char *oldpath, char *newpath) {
	int src_fd = open(oldpath, O_RDONLY);
	if (src_fd < 0) {
		perror("cp: src");
		return -1;
	}
	int dest_fd = open(newpath, O_WRONLY | O_CREAT | O_TRUNC, 0644);
	if (dest_fd < 0) {
		perror("cp: dest");
		return -1;
	}

	char buffer[4096];
	ssize_t bytes_read;

	while ((bytes_read = read(src_fd, buffer, sizeof(buffer))) > 0) {
		ssize_t bytes_written = write(dest_fd, buffer, bytes_read);
		if (bytes_written != bytes_read) {
			perror("cp: write error");
			return -1;
		}
	}
	if (bytes_read < 0) {
		perror("cp: read error");
		return -1;
	}
	close(src_fd);
	close(dest_fd);
	return 0;
}
NEW_CMD(cp) {
    if (argc < 3) {
        fprintf(stderr, "Usage: cp <source> <destination>\n");
        return -1;
    }
	int value = 0;
	char *last = args[argc - 1];
	if (check_path(last) == FOLDER) {
		for (int i = 1; i < argc - 1; i++) {
			char *dest = malloc(strlen(args[i]) + strlen(last) + 0x10);
			sprintf(dest, "%s/%s", last, args[i]);
			value = copy_file(args[i], dest);
			free(dest);
		}
	} else if (check_path(last) == -1 && argc == 3) {
		value = copy_file(args[1], args[2]);
	} else if (argc >= 4) {
		shell_print("dest folder doesn't exist \n");
	} else {
		shell_print("unexpected error \n");
	}
	return value;
}
NEW_CMD(chmod) {
	int value = 0;
    if (argc < 3) {
        fprintf(stderr, "Usage: chmod <mode> <file>\n");
        return -1;
    }
    // Convert mode string to octal number
	mode_t mode = strtol(args[1], NULL, 8);
	for (int i = 2; i < argc; i++) {
		if (chmod(args[i], mode) != 0) {
			perror("chmod");
			value = -1;
		}	
	}
	return value;
}
NEW_CMD(mv) {
    if (argc < 3) {
        shell_print("Usage: mv <source> <destination>\n");
        return -1;
    }
	int value = 0;
	char *last = args[argc - 1];
	for (int i = 1; i < argc - 1; i++) {
		if (is_dir(last) || !is_file_exist(last)) {
			char *dest = malloc(strlen(args[i]) + strlen(last) + 0x10);
			sprintf(dest, "%s/%s", last, args[i]);
			printf("%s \n", dest);
			if (rename(args[i], dest) != 0) {
				perror("mv");
				value = -1;
			}
			free(dest);
		} else {
			shell_print("target file exist !\n");
			return value;
		}
	}
	return value;
}
NEW_CMD(antivirus) {
	char buff[] = "antivirus starting ";
	char dot[] = ".";
	char *full = malloc(strlen(buff) + strlen(dot) + 0x10);
	int count = 0;
	shell_print(buff);
	for (int i = 0; i < 5; i++) {
		clear_line();
		strcat(full, buff);
		for (int i = 0; i < count; i++) {
			strcat(full, dot);
		}
		shell_print(full);
		usleep(1000000);
		count = (i % 3) + 1;
		full[0] = 0;
	}
	printf("\n");
	return 0;

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
	CMD_ENTRY(chmod),
	CMD_ENTRY(mv),
	CMD_ENTRY(antivirus),
};
int call_command(const char *cmd, char **args, int argc) {
    for (int i = 0; i < NUM_COMMANDS; ++i) {
        if (strcmp(command_table[i].name, cmd) == 0) {
            command_table[i].fn(args, argc);
            return 1;
        }
    }
    printf("command not found :< %s\n", cmd);
	return -1;
}

