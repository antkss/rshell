#include "utils.h"
#include <fcntl.h>
#include <stdarg.h>
#include <unistd.h>
mode_t get_perm(const char *filename) {
    struct stat st;
    if (stat(filename, &st) == -1) {
        perror("stat");
        return (mode_t)-1;
    }
    return st.st_mode & 0777;
}
void* ralloc(void *ptr, size_t *capacity, size_t dsize) { // small functions to handle memory
	void *result = ptr;
	if (dsize + 1 > *capacity || !ptr) {
		*capacity = *capacity + (int)(*capacity / 2);
		if (*capacity < dsize) {
			*capacity = dsize;
		}
		result = realloc(ptr, *capacity);
	}
	return result;
}
int parse_option(char **args, int argc, char **option) {
	int option_cnt = 0;
	for (int i = 1; i < argc; i++) {
		if (!strncmp(args[i], "-", 1)) {
			// shell_print("option parsed ! %s\n", args[i]);
			if(!is_exist(args[i], option, option_cnt)) {
				option[option_cnt] = args[i];
				args[i] = NULL;
				option_cnt++;
			}

		}
	}
	return option_cnt;
}
void print_sign(char sign[]) {
	write(STDOUT_FILENO, sign, strlen(sign));
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
	struct stat st = {0};
    if (stat(path, &st) == -1) {
		return 0;
    } 
    return 1;
}
int compare_first_char(const void *a, const void *b) {
    const char *fa = *(const char **)a;
    const char *fb = *(const char **)b;

    // Skip NULLs for safety
    if (!fa) return 1;
    if (!fb) return -1;

    // Case-insensitive comparison of first char
    char ca = tolower(fa[0]);
    char cb = tolower(fb[0]);

    if (ca < cb) return -1;
    if (ca > cb) return 1;

    // Fallback to full name if first chars are equal
    return strcasecmp(fa, fb);
}
void sort_files(char **files, int file_count) {
    qsort(files, file_count, sizeof(char *), compare_first_char);
}
void pretty_print_list(char *files[], const char *path, int file_count, int is_all) {
    char full_path[PATH_MAX];

    // Step 1: Filter and copy visible files
    char **visible = malloc(file_count * sizeof(char*));
    int count = 0;
    for (int i = 0; i < file_count; i++) {
        if (files[i] && (is_all || files[i][0] != '.')) {
            visible[count++] = files[i];
        }
    }

    if (count == 0) {
        free(visible);
        return;
    }

    // Step 2: Measure each filename length
    int *lengths = malloc(sizeof(int) * count);
    int term_width = get_terminal_size().ws_col;

    for (int i = 0; i < count; i++) {
        lengths[i] = strlen(visible[i]);
    }

    // Step 3: Try different column counts to fit most tightly
    int best_cols = 1;
    int best_rows = count;
    for (int cols = 1; cols <= count; cols++) {
        int rows = (count + cols - 1) / cols;
        int *col_widths = calloc(cols, sizeof(int));

        for (int i = 0; i < count; i++) {
            int col = i / rows;
            if (lengths[i] > col_widths[col])
                col_widths[col] = lengths[i];
        }

        int total_width = 0;
        for (int i = 0; i < cols; i++)
            total_width += col_widths[i] + 2;  // 2 spaces between

        free(col_widths);
        if (total_width <= term_width) {
            best_cols = cols;
            best_rows = rows;
        } else {
            break; // no tighter fit possible
        }
    }

    // Step 4: Final column widths
    int *col_widths = calloc(best_cols, sizeof(int));
    for (int i = 0; i < count; i++) {
        int col = i / best_rows;
        if (lengths[i] > col_widths[col])
            col_widths[col] = lengths[i];
    }

    // Step 5: Print in row-major order
    for (int row = 0; row < best_rows; row++) {
        for (int col = 0; col < best_cols; col++) {
            int idx = col * best_rows + row;
            if (idx < count) {
                char *name = visible[idx];
                snprintf(full_path, sizeof(full_path), "%s/%s", path, name);
                struct stat st;
                if (stat(full_path, &st) == 0 && S_ISDIR(st.st_mode)) {
                    shell_print("%s%-*s%s", COLOR_BLUE, col_widths[col] + 2, name, COLOR_RESET);
                } else {
                    shell_print("%-*s", col_widths[col] + 2, name);
                }
            }
        }
        shell_print("\n");
    }

    free(visible);
    free(lengths);
    free(col_widths);
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

	char buffer[READ_BYTES];
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
	mode_t perm = get_perm(oldpath);
	chmod(newpath, perm);
	close(src_fd);
	close(dest_fd);
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
			char *object = entry->d_name;
			if (check_path(object) == FOLDER) {
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
int is_valid_env_var(const char *name) {
    // Check for a null or empty string.
    if (!name || name[0] == '\0') {
        return 0;
    }

    // The first character must be an alphabet letter or underscore.
    if (!(isalpha(name[0]) || name[0] == '_')) {
        return 0;
    }

    // Subsequent characters must be alphanumeric or underscore.
    for (int i = 1; name[i] != '\0'; i++) {
        if (!(isalnum(name[i]) || name[i] == '_')) {
            return 0;
        }
    }
    
    return 1;
}
int is_valid_env_char(char c, int first) {
    if (first)
        return (isalpha(c) || c == '_');
    else
        return (isalnum(c) || c == '_');
}
int is_folder_char(char c) {
    // Forbidden characters in unquoted folder names
    const char *forbidden = " \t\n*?[]{}$&;|<>\\'\"~`!#=/";

    if (!isprint((unsigned char)c)) {
        return 0; // Non-printable
    }

    for (const char *p = forbidden; *p; p++) {
        if (c == *p) {
            return 0; // Forbidden
        }
    }

    return 1; // Allowed
}

int is_operator_char(char c) {
    return strchr("|&;<>", c) != NULL;
}

int is_redirection_char(char c) {
    return c == '>' || c == '<';
}

int is_whitespace(char c) {
    return c == ' ' || c == '\t' || c == '\n';
}
int find_pos_token(char **args, int argc, char *target, int cur_pos) {
	for (int i = cur_pos; i < argc; i++) {
		if (strcmp(args[i], "|") == 0) {
			return i;
		}
	}
	return -1;
}

