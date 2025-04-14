#include <linux/limits.h>
#include <string.h>

#define MAX_HISTORY 0x1000
extern char *command_history[MAX_HISTORY];
extern unsigned int history_cnt;

typedef enum {
    NODE_CMD,
    NODE_PIPE,
    NODE_AND,
    NODE_OR,
    NODE_SEQ,
    NODE_REDIR,
    NODE_WORD,
    NODE_DQUOTE,
    NODE_QUOTE,
	NODE_EQUAL
} NodeType;

typedef struct ASTNode {
    NodeType type;
    char *value;  // For WORD/QUOTE/DQUOTE
    struct ASTNode *left;
    struct ASTNode *right;
} ASTNode;

typedef struct {
    char **tokens;
    int count;
    int pos;
} TokenStream;


char* peek(TokenStream *ts); 
char* next(TokenStream *ts);

ASTNode* parse_cmd(TokenStream *ts);
ASTNode* parse_pipeline(TokenStream *ts);
ASTNode* parse_redir(TokenStream *ts);
ASTNode* parse_and_or(TokenStream *ts);
ASTNode* parse_sequence(TokenStream *ts);
TokenStream *tokenize(const char *input);
void print_ast(ASTNode *node, int indent);
void parse_call(char *input, int input_len);
char *concat_tokens(TokenStream *ts);
               
