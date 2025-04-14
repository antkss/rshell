#include "parser.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/wait.h>
#include "command.h"

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
    if (tok[0] == '"') return new_node(NODE_DQUOTE, tok, NULL, NULL);
    if (tok[0] == '\'') return new_node(NODE_QUOTE, tok, NULL, NULL);
    if (tok[0] == '=') return new_node(NODE_EQUAL, tok, NULL, NULL);
    return new_node(NODE_WORD, tok, NULL, NULL);
}

ASTNode* parse_cmd(TokenStream *ts) {
    ASTNode *cmd = new_node(NODE_CMD, NULL, NULL, NULL);
    ASTNode *last = NULL;
    while (peek(ts) && strcmp(peek(ts), "|") && strcmp(peek(ts), "&&") &&
           strcmp(peek(ts), "||") && strcmp(peek(ts), ";") && strcmp(peek(ts), ">")) {
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
ASTNode* parse_redir(TokenStream *ts) {
    ASTNode *cmd = parse_pipeline(ts);
    while (peek(ts) && strcmp(peek(ts), ">") == 0) {
        next(ts);  // consume '>'
        ASTNode *file = parse_word(ts);
        cmd = new_node(NODE_REDIR, NULL, cmd, file);
    }
    return cmd;
}



ASTNode* parse_and_or(TokenStream *ts) {
    ASTNode *node = parse_redir(ts);
    while (peek(ts)) {
        if (strcmp(peek(ts), "&&") == 0) {
            next(ts);
            ASTNode *right = parse_redir(ts);
            node = new_node(NODE_AND, NULL, node, right);
        } else if (strcmp(peek(ts), "||") == 0) {
            next(ts);
            ASTNode *right = parse_redir(ts);
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

TokenStream* tokenize(const char *input) {
    char *copy = strdup(input);
    char *tok = strtok(copy, " ");
    char **tokens = malloc(sizeof(char*) * 128); // buffer overflow
	TokenStream *ts = malloc(sizeof(TokenStream));
	ts->tokens = tokens;
    int i = 0;
    while (tok) {
        tokens[i++] = strdup(tok);
        tok = strtok(NULL, " ");
    }
	ts->count = i;
    tokens[i] = NULL;
    free(copy);
    return ts;
}

void print_ast(ASTNode *node, int indent) {
    if (!node) return;
    for (int i = 0; i < indent; ++i) shell_print("  ");
    const char *type_str[] = {
        "CMD", "PIPE", "AND", "OR", "SEQ", "REDIR",
        "WORD", "DQUOTE", "QUOTE"
    };
    shell_print("%s", type_str[node->type]);
    if (node->value) shell_print(": %s", node->value);
    shell_print("\n");
    print_ast(node->left, indent + 1);
    print_ast(node->right, indent + 1);
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

        case NODE_REDIR: {
            int saved_stdout = dup(STDOUT_FILENO);
            FILE *fp = fopen(node->right->value, "w");
            if (!fp) {
                perror("fopen");
                return 1;
            }
            dup2(fileno(fp), STDOUT_FILENO);
            int res = eval_ast(node->left);
            fflush(stdout);
            dup2(saved_stdout, STDOUT_FILENO);
            close(saved_stdout);
            fclose(fp);
            return res;
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
	free(ts);
}
void parse_call(char *input, int input_len) {
	TokenStream *ts = tokenize(input);
	ASTNode *root = parse_sequence(ts);
	eval_ast(root);
	char *history = concat_tokens(ts);

	if (!is_exist(history, command_history, history_cnt)) {
		command_history[history_cnt] = history;
		history_cnt += 1;
	} else {
		free(history);
	}
	free_tokens(ts);
	free_ast(root);
}
