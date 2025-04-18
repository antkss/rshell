#include "parser.h"
#include <linux/limits.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/wait.h>
#include "command.h"
#include <fcntl.h>
#include <readline/readline.h>
#include "utils.h"

char* peek(TokenStream *ts) {
    return ts->pos < ts->count ? ts->tokens[ts->pos] : NULL;
}

char* next(TokenStream *ts) {
    return ts->pos < ts->count ? ts->tokens[ts->pos++] : NULL;
}
ASTNode* new_node(NodeType type, const char *value, ASTNode *left, ASTNode *right) {
    ASTNode *node = malloc(sizeof(ASTNode));
    node->type = type;
    node->value = value ? strdup(value) : NULL;
    node->left = left;
    node->right = right;
    return node;
}

ASTNode* parse_word(TokenStream *ts) {
    char *tok = next(ts);
    if (!tok) return NULL;

    return new_node(NODE_WORD, tok, NULL, NULL);
}

ASTNode* parse_cmd(TokenStream *ts) {
    ASTNode *cmd = new_node(NODE_CMD, NULL, NULL, NULL);
    ASTNode *last = NULL;
    while (peek(ts) && strcmp(peek(ts), "|") && strcmp(peek(ts), "&&") 
		&& strcmp(peek(ts), "||") && strcmp(peek(ts), ";")
		&& strcmp(peek(ts), ">") && strcmp(peek(ts), "<")
		&& strcmp(peek(ts), ">>")
	) {
        ASTNode *w = parse_word(ts);
        if (!cmd->left) {
            cmd->left = w;
            last = w;
        } else {
            last->right = w;
            last = w;
        }
    }
    return cmd;
}

ASTNode* parse_pipeline(TokenStream *ts) {
    ASTNode *node = parse_cmd(ts);
    while (peek(ts) && strcmp(peek(ts), "|") == 0) {
        next(ts);
        ASTNode *right = parse_cmd(ts);
        node = new_node(NODE_PIPE, NULL, node, right);
    }
    return node;
}
ASTNode* parse_dredir(TokenStream *ts) {
    ASTNode *cmd = parse_pipeline(ts);
    while (peek(ts) && strcmp(peek(ts), ">>") == 0) {
        next(ts);  // consume '>>'
        ASTNode *file = parse_word(ts);
        cmd = new_node(NODE_DREDIR, NULL, cmd, file);
    }
    return cmd;
}
ASTNode* parse_redir_left(TokenStream *ts) {
    ASTNode *cmd = parse_dredir(ts);
    while (peek(ts) && strcmp(peek(ts), "<") == 0) {
        next(ts);  // consume '<'
        ASTNode *file = parse_word(ts);
        cmd = new_node(NODE_REDIR_LEFT, NULL, cmd, file);
    }
    return cmd;
}
ASTNode* parse_redir_right(TokenStream *ts) {
    ASTNode *cmd = parse_redir_left(ts);
    while (peek(ts) && strcmp(peek(ts), ">") == 0) {
        next(ts);  // consume '>'
        ASTNode *file = parse_word(ts);
        cmd = new_node(NODE_REDIR_RIGHT, NULL, cmd, file);
    }
    return cmd;
}



ASTNode* parse_and_or(TokenStream *ts) {
    ASTNode *node = parse_redir_right(ts);
    while (peek(ts)) {
        if (strcmp(peek(ts), "&&") == 0) {
            next(ts);
            ASTNode *right = parse_redir_right(ts);
            node = new_node(NODE_AND, NULL, node, right);
        } else if (strcmp(peek(ts), "||") == 0) {
            next(ts);
            ASTNode *right = parse_redir_right(ts);
            node = new_node(NODE_OR, NULL, node, right);
        } else {
            break;
        }
    }
    return node;
}

ASTNode* parse_sequence(TokenStream *ts) {
    ASTNode *node = parse_and_or(ts);
    while (peek(ts) && strcmp(peek(ts), ";") == 0) {
        next(ts);
        ASTNode *right = parse_and_or(ts);
        node = new_node(NODE_SEQ, NULL, node, right);
    }
    return node;
}



TokenStream *tokenize(char *input) {
    TokenStream *ts = malloc(sizeof(TokenStream));
	int capacity = 8;
    ts->tokens = malloc(sizeof(char *) * capacity);
    ts->count = 0;
    ts->pos = 0;

    int i = 0, len = strlen(input);
    int in_single_quote = 0;
    int in_double_quote = 0;
    int in_backtick = 0;
    int escaped = 0;
    int nested_subs = 0;

    char token[ARG_MAX];
	// if (len > ARG_MAX) {
	// 	shell_print("too much tokens \n");
	// }
    int token_len = 0;

    while (i < len) {
        char c = input[i];
        char next = (i + 1 < len) ? input[i + 1] : '\0';
		// handle memory
		if (ts->count > capacity - 4) {
			capacity = capacity * 2;
			ts->tokens = realloc(ts->tokens, sizeof(char *) * capacity);
		}
        // Handle comment outside quotes
        if (!in_single_quote && !in_double_quote && !in_backtick && c == '#') {
            break; // Skip rest of the line
        }

        if (escaped) {
            token[token_len++] = c;
            escaped = 0;
        } else if (c == '\\') {
            if (in_single_quote) {
                token[token_len++] = c; // literal in single quotes
            } else {
                escaped = 1;
            }
        } else if (c == '\'' && !in_double_quote && !in_backtick) {
            in_single_quote = !in_single_quote;
        } else if (c == '"' && !in_single_quote && !in_backtick) {
            in_double_quote = !in_double_quote;
        } else if (c == '`' && !in_single_quote && !in_double_quote) {
            in_backtick = !in_backtick;
            token[token_len++] = c;
        } else if (!in_single_quote && !in_double_quote && !in_backtick && c == '$' && next == '(') {
            token[token_len++] = c;
            token[token_len++] = '(';
            i += 2;
            nested_subs = 1;
            while (i < len && nested_subs > 0) {
                if (input[i] == '(') nested_subs++;
                else if (input[i] == ')') nested_subs--;
                token[token_len++] = input[i++];
            }
            i--;
        } else if (!in_single_quote && !in_double_quote && !in_backtick && c == '$' && next == '(' && input[i + 2] == '(') {
            token[token_len++] = '$';
            token[token_len++] = '(';
            token[token_len++] = '(';
            i += 3;
            nested_subs = 2;
            while (i < len && nested_subs > 0) {
                if (input[i] == '(') nested_subs++;
                else if (input[i] == ')') nested_subs--;
                token[token_len++] = input[i++];
            }
            i--;
        } else if (!in_single_quote && !in_double_quote && !in_backtick && is_operator_char(c)) {
            if (token_len > 0) {
                token[token_len] = '\0';
                ts->tokens[ts->count++] = strdup(token);
                token_len = 0;
            }

            token[token_len++] = c;
            if ((c == '&' && next == '&') || (c == '|' && next == '|') || (c == '>' && next == '>')) {
                token[token_len++] = next;
                i++;
            }
            token[token_len] = '\0';
            ts->tokens[ts->count++] = strdup(token);
            token_len = 0;
        } else if (!in_single_quote && !in_double_quote && !in_backtick && is_whitespace(c)) {
            if (token_len > 0) {
                token[token_len] = '\0';
                ts->tokens[ts->count++] = strdup(token);
                token_len = 0;
            }
        } else {
            token[token_len++] = c;
        }
        i++;
    }

    if (token_len > 0) {
        token[token_len] = '\0';
        ts->tokens[ts->count++] = strdup(token);
    }

    ts->tokens[ts->count] = NULL;
    return ts;
}
void print_ast(ASTNode *node, int indent) {
    if (!node) return;
    for (int i = 0; i < indent; ++i) shell_print("  ");
    const char *type_str[] = {
        "CMD", "PIPE", "AND", "OR", "SEQ", "REDIR RIGHT", "REDIR LEFT","NODE DREDIR",
        "WORD"
    };
    shell_print("%s", type_str[node->type]);
    if (node->value) shell_print(": %s", node->value);
    shell_print("\n");
    print_ast(node->left, indent + 2);
    print_ast(node->right, indent + 2);
}
void print_tokens(TokenStream *ts) {
	for (int i = 0; i < ts->count; i++) {
		shell_print("token[%d]: %s \n", i, ts->tokens[i]);
	}
}
int eval_ast(ASTNode *node) {
    if (!node) return 1;

    switch (node->type) {
        case NODE_CMD: {
            int argc = 0;
            char *argv[128] = {0};
            ASTNode *curr = node->left;
            while (curr && argc < 127) {
                argv[argc++] = curr->value;
                curr = curr->right;
            }
            if (argc == 0) return 1;

            call_command(argv[0], argv, argc);
            return 0;
        }

        case NODE_AND: {
            int left = eval_ast(node->left);
            if (left == 0) return eval_ast(node->right);
            return left;
        }

        case NODE_OR: {
            int left = eval_ast(node->left);
            if (left != 0) return eval_ast(node->right);
            return 0;
        }

        case NODE_SEQ: {
            eval_ast(node->left);
            return eval_ast(node->right);
        }

        case NODE_REDIR_RIGHT: {
			if (!node->right) return 1;
            int saved_stdout = dup(STDOUT_FILENO);
            int fd = open(node->right->value, O_RDWR | O_CREAT | O_TRUNC, 0644);
            if (fd < 0) {
                perror("open");
                return 1;
            }
            dup2(fd, STDOUT_FILENO);
            int res = eval_ast(node->left);
            fflush(stdout);
            dup2(saved_stdout, STDOUT_FILENO);
            close(saved_stdout);
            close(fd);
            return res;
        }
		case NODE_DREDIR: {
			if (!node->right) return 1;
            int saved_stdout = dup(STDOUT_FILENO);
            int fd = open(node->right->value, O_WRONLY | O_APPEND | O_CREAT, 0644);
            if (fd < 0) {
                perror("open");
                return 1;
            }
            dup2(fd, STDOUT_FILENO);
            int res = eval_ast(node->left);
            fflush(stdout);
            dup2(saved_stdout, STDOUT_FILENO);
            close(saved_stdout);
            close(fd);
			return 0;		
		}
        case NODE_REDIR_LEFT: {
			if (!node->right) return 1;
			int fd = open(node->right->value, O_RDONLY);
			if (fd < 0) {
				perror("open");
			}
            pid_t pid2 = fork();
            if (pid2 == 0) {
                dup2(fd, STDIN_FILENO);
				close(fd);
                eval_ast(node->left);
                exit(0);
            }
            waitpid(pid2, NULL, 0);
			return 0;
        }
        case NODE_PIPE: {
            int pipefd[2];
            pipe(pipefd);
            pid_t pid1 = fork();
            if (pid1 == 0) {
                close(pipefd[0]);
                dup2(pipefd[1], STDOUT_FILENO);
                eval_ast(node->left);
                exit(0);
            }

            pid_t pid2 = fork();
            if (pid2 == 0) {
                close(pipefd[1]);
                dup2(pipefd[0], STDIN_FILENO);
                eval_ast(node->right);
                exit(0);
            }

            close(pipefd[0]);
            close(pipefd[1]);
            waitpid(pid1, NULL, 0);
            waitpid(pid2, NULL, 0);
            return 0;
        }

        default:
            fprintf(stderr, "Unknown node type: %d\n", node->type);
            return 1;
    }
}
void free_ast(ASTNode *node) {
    if (node == NULL)
        return;
    free_ast(node->left);
    free_ast(node->right);
    if (node->value)
        free(node->value);
    free(node);
}
char *concat_tokens(TokenStream *ts) {
       int total = 0;
       for (int i = 0; i < ts->count; i++) {
               total += strlen(ts->tokens[i]);
       }
       char *concated = malloc(total + 0x10);
       concated[0] = '\0';
       for (int i = 0; i < ts->count; i++) {
               strcat(concated, ts->tokens[i]);
               if (i < ts->count - 1)
                       strcat(concated, " ");  // add space between tokens
       }
       return concated;
}
void free_tokens(TokenStream *ts) {
    for (int i = 0; i < ts->count; i++) {
        free(ts->tokens[i]);
    }
    free(ts->tokens);
    free(ts);
}
char **extract_line(const char *input, int *line_cnt) {
    int capacity = 8;  // Start small and grow
    char **lines = malloc(sizeof(char*) * capacity);
	char *copy = strdup(input);
	char *line = strtok(copy, "\n");
	*line_cnt = 0;
	while (line) {
        if (*line_cnt >= capacity) {
            capacity *= 2;
            lines = realloc(lines, sizeof(char*) * capacity);
        }
		lines[*line_cnt] = strdup(line);
		*line_cnt += 1;
		line = strtok(NULL, "\n");

		
	}
	free(copy);
	return lines;
}
void print_lines(char **lines, int line_cnt) {
	for (int i = 0; i < line_cnt; i++) {
		shell_print("lines[%d] = %s \n", i, lines[i]);
	}
}
void free_lines(char **lines, int line_cnt) {
	for (int i = 0; i < line_cnt; i++) {
		free(lines[i]);
	}
	free(lines);
}
char *expand_variables(const char *input) {
    size_t len = strlen(input);
	size_t capacity = len * 2;
    char *result = malloc(capacity); // Allocate more space for safety
    if (!result) return NULL;

    size_t i = 0, j = 0;
    char *home = getenv("HOME");

    while (input[i]) {
		if (input[i] == '~') {
            // Handle tilde expansion (~ -> $HOME)
            if (i == 0 || input[i-1] == ' ' || input[i-1] == '\t' || input[i-1] == '=') {
                // Only expand if it's at the start or after space/equals
                if (home) {
                    size_t home_len = strlen(home);
                    memcpy(result + j, home, home_len);
                    j += home_len;
                }
                i++; // Skip tilde
            } else {
                result[j++] = input[i++];
            }
        } else if (input[i] == '$' && isalpha(input[i + 1]) && input[i - 1] != '\\') {
            // Handle environment variable expansion
            i++; // skip '$'
			size_t var_capacity = 0x100;
			char *var = malloc(0x100);
			var[0] = '\0';
            int k = 0;

            while (isalnum(input[i]) || input[i] == '_') {
				if (k > var_capacity) {
					var_capacity = var_capacity * 2;
					var = realloc(var, var_capacity);
				}
                var[k++] = input[i++];
            }
            var[k] = '\0';

            char *val = getenv(var);
			free(var);
            if (val) {
                size_t vlen = strlen(val);
                memcpy(result + j, val, vlen);
                j += vlen;
            }
        } else {
            result[j++] = input[i++]; // Copy other characters as is
        }
		if (j > capacity) {
			capacity = capacity * 2;
			result = realloc(result, capacity);
		}
    }

    result[j] = '\0';
    return result;
}
void parse_call(char *input) {
	int line_cnt;
	char **lines = extract_line(input, &line_cnt);
	// print_lines(lines, line_cnt);
	for (int i = 0; i < line_cnt; i++) {
		char *newline = expand_variables(lines[i]);
		TokenStream *ts = tokenize(newline);
		// print_tokens(ts);
		ASTNode *root = parse_sequence(ts);
		if (root) {
			eval_ast(root);
			free(newline);
			// print_ast(root, 3);
			free_tokens(ts);
			free_ast(root);
		}

	}
	free_lines(lines, line_cnt);
}
