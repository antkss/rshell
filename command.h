#include <linux/limits.h>
#include <signal.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <dirent.h>
#include <sys/stat.h>
#pragma once

typedef int (*command_fn)(char **args, int argc);

typedef struct {
    const char *name;
    command_fn fn;
} CommandEntry;

#define CMD_ENTRY(cmd) { #cmd, cmd##_cmd }
#define NEW_CMD(cmd) int cmd##_cmd(char **args, int argc)
#define NUM_COMMANDS (sizeof(command_table)/sizeof(CommandEntry))
#define GENERIC_PSI "@>" 
#define PSI_LEN 2
#define MAX_ARGS 64
#define COLOR_RESET   "\033[0m"
#define COLOR_BLUE    "\033[34m"
#define COLOR_GREEN   "\033[32m"
#define COLOR_WHITE   "\033[37m"

void print_sign(char sign[]);
void shell_print(const char *format, ...);
int remove_recursive(char *path);
int is_dir(char *path);
int get_list_files(const char *path, char* content[], int *count);
int is_exist(char *buffer, char *buff_array[], int count); 
int find_pos_token(char **args, int argc, char *target, int cur_pos);
int parse_option(char **args, int argc, char **option);
int call_command(const char *cmd, char **args, int argc);
void md5sum_file(const char *filename);
char* mstrcat(int count, ...);

