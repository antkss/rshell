#include "command.h"
#include <linux/limits.h>
#include <inttypes.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/ioctl.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>
#include "parser.h"
#include <stdarg.h>
#include "terminal.h"
#include "utils.h"
extern char **environ;
extern pid_t child_pid;

NEW_CMD(clear) {
	shell_print("\033c");
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
	if (!strcmp(path, "-")) {
		path = getenv("OLDPWD");
	}
    // Change directory
    if (chdir(path) != 0) {
        perror("cd failed");  // Print error
    }
	return 0;
}


NEW_CMD(exit) {
	exit(0);
}

NEW_CMD(ls){
	char *content[XATTR_LIST_MAX];
	int count = 0;
	int is_all = 0;
	int value = 0;
	// get options
	char *option[2];
	int op_num = parse_option(args, argc, option);
	for (int i = 0; i < op_num; i++) {
		if (!strncmp(option[i], "-a", 2)) {
			is_all = 1;
		} else {
			shell_print("Invalid option: %s \n", option[i]);
			return -1;
		}
	}
	int real_argc = argc - op_num;
	if (real_argc == 1 || real_argc == 2) {
		char *path = args[1];
		// shell_print("%s \n", path);
		if (path == NULL) path = ".";
		value = get_list_files(path, content, &count);
		sort_files(content, count);
		pretty_print_list(content, path, count, is_all);
		for (int i = 0; i < count; i++) {
			free(content[i]);
		}
	} else {
		// get list files, folders
		for (int i = 1; i < argc; i++) {
			char *path = args[i];
			shell_print("%s: \n", path);
			if (path != NULL) {
				value = get_list_files(path, content, &count);
				sort_files(content, count);
				pretty_print_list(content, path, count, is_all);
				for (int i = 0; i < count; i++) {
					free(content[i]);
				}
			}

		}
	}


	return value;
}
NEW_CMD(cat) {
	// printf("number of files %d\n", argc);
	char buff[READ_BYTES + 0x10];
	if (argc == 1) {
		int read_size = 0;
		while ((read_size = read(0, buff, READ_BYTES)) != 0) {
			buff[read_size] = 0;
			shell_print("%s", buff);
		}
		return 0;
	}
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
			unsigned long file_cap = pos;
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
	shell_print("\n");
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
            int fd = open(args[i], O_CREAT, 0644);
			close(fd);
		} else {
			shell_print("object %s exist !\n");
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

NEW_CMD(cp) {
    if (argc < 3) {
        fprintf(stderr, "Usage: cp <source> <destination>\n");
        return -1;
    }
	int value = 0;
	char *last = args[argc - 1];
	if (argc > 3) {
		if (check_path(last) == FOLDER) {
			for (int i = 1; i < argc - 1; i++) {
				char *dest = malloc(strlen(args[i]) + strlen(last) + 0x10);
				sprintf(dest, "%s/%s", last, args[i]);
				if (copy_file(args[i], dest) != 0) {
					perror("cp");
					value = -1;
				}
				free(dest);
			}
		} else {
			shell_print("cp error ! \n");
		}
	} else if (argc == 3) {
		if (!is_file_exist(args[2]))
			value = copy_file(args[1], args[2]);
		else 
			shell_print("file exists, skipping ! \n");
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
	if (argc > 3) {
		if (check_path(last) == FOLDER) {
			for (int i = 1; i < argc - 1; i++) {
				char *dest = malloc(strlen(args[i]) + strlen(last) + 0x10);
				sprintf(dest, "%s/%s", last, args[i]);
				shell_print("%s \n", dest);
				if (rename(args[i], dest) != 0) {
					perror("mv");
					value = -1;
				}
				free(dest);
			} 
		} else {
			shell_print("mv error \n");
		} 
	} else if (argc == 3) {
		if (!is_file_exist(args[2])) {
			rename(args[1], args[2]);
		} else {
			shell_print("file exist, skipping !");
		}
	}
	return value;
}
NEW_CMD(antivirus) {
	char buff[] = "antivirus starting ";
	char dot[] = ".";
	char *full = malloc(strlen(buff) + strlen(dot) + 0x10);
	int count = 0;
	full[0] = '\0';
	// shell_print(buff);
	for (int i = 0; i < 10; i++) {

		strcat(full, buff);
		for (int i = 0; i < count; i++) {
			strcat(full, dot);
		}
		shell_print(full);
		usleep(500000);
		count = (i % 3) + 1;
		for (int i = 0; i < strlen(full); i++) {
			delete_at_cursor();
		}
		full[0] = 0;
	}
	free(full);
	char *buffer = "\n(° ͜ʖ͡°)╭∩╮\n\n╭∩╮( •̀_•́ )╭∩╮\n\0";
	write(STDOUT_FILENO, buffer, strlen(buffer));
	return 0;

}
help_entry help_table []  = {
	{"ls", "list folder and file of a path"},
	{"cd", "change directory"},
	{"exit", "leave shell"},
	{"clear", "clear screen"},
	{"cat", "concatenate"},
	{"mkdir", "create empty folder"},
	{"rm", "remove object"},
	{"touch", "create empty file"},
	{"cp", "copy object"},
	{"echo", "echo text"},
	{"chmod", "change permission of an object"},
	{"mv", "move file or change file name"},
	{"set", "set environment variable"},
	{"unset", "unset environment variable"},
	{"antivirus", "antivirus by FBI"},
	{"help", "help me bro !"},
	{NULL, NULL}
};
NEW_CMD(help) {
	shell_print("%sBuiltin command lists : %s\n", COLOR_BLUE, COLOR_RESET);
	int max_len = 0;
	for (int i = 0; help_table[i].cmd; i++) {
		size_t len = strlen(help_table[i].cmd) + strlen(help_table[i].description) + 2;
		if (max_len < len) {
			max_len = len;
		}
		shell_print("%s: %s", help_table[i].cmd, help_table[i].description);
		shell_print("\n");
	}
	for (int i = 0; i < max_len; i++) {
		shell_print(".");
	}
	shell_print("\n");
	return 0;
}
NEW_CMD(set) {
	if (argc == 1) {
		for (int i = 0; environ[i] != NULL; i++) {
			shell_print("%s", environ[i]);
			shell_print("\n");
		}
		return 0;
	} else {
		for (int i = 1; i < argc; i++) {
			char *phrase = args[i];
			char *name = strtok(phrase, "=");
			if (is_valid_env_var(name)) {
				char *value = strtok(NULL, "=");
				if (value == NULL) {
					value = "";
				}
				setenv(name, value, 1);
			} else {
				shell_print("invalid environ name ! \n");
			}

		}
	}

	return 0;
}
NEW_CMD(unset) {
	if (argc == 1) {
		for (int i = 0; environ[i] != NULL; i++) {
			shell_print("%s", environ[i]);
			shell_print("\n");
		}
		return 0;
	} else {
		for (int i = 1; i < argc; i++) {
			char *name = args[i];
			unsetenv(name);
		}
	}
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
	CMD_ENTRY(help),
	CMD_ENTRY(set),
	CMD_ENTRY(unset),
	{NULL, NULL},
};
int call_command(const char *cmd, char **args, int argc) {
	// run builtin command first
	for (int i = 0; command_table[i].name; ++i) {
		if (strcmp(command_table[i].name, cmd) == 0) {
			command_table[i].fn(args, argc);
			return EXIT_SUCCESS;
		}
	}
	child_pid = fork();
	if (child_pid == 0) {
		// call command from system
        execvp(args[0], args);
		shell_print("command: %s \n", args[0]);
        perror("execvp");
		exit(EXIT_FAILURE);
    } else if (child_pid > 0) {
        wait(NULL);  // Wait for child to finish
    } else {
        perror("fork failed");
    }
	child_pid = -1;
	return EXIT_FAILURE;
}

