#include <ctype.h>
#include <stdlib.h>
#include "command.h"
#include "terminal.h"
#define READ_BYTES 4096
enum {
	FOLDER, 
	REGULAR_FILE, 
	UNKNOWN,
};
int compare_first_char(const void *a, const void *b);
void sort_files(char **files, int file_count);
int get_list_files(const char *path, char* content[], int *count);
void pretty_print_list(char *files[], const char *path, int file_count, int is_all);
int copy_file(char *oldpath, char *newpath);
int is_file_exist(char *path);
int check_path(const char *path);
int is_valid_env_var(const char *name);
int is_valid_env_char(char c, int first);
int is_folder_char(char c);

int is_operator_char(char c);
int is_redirection_char(char c);

int is_whitespace(char c);
