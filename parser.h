#include <string.h>

typedef enum {
    NODE_CMD,
    NODE_PIPE,
    NODE_AND,
    NODE_OR,
    NODE_SEQ,
    NODE_REDIR_RIGHT,
    NODE_REDIR_LEFT,
	NODE_DREDIR,
	NODE_TREDIR,
    NODE_WORD,
} NodeType;

typedef struct ASTNode {
    NodeType type;
    char *value;  
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
ASTNode* parse_redir_right(TokenStream *ts);
ASTNode* parse_redir_left(TokenStream *ts);
ASTNode* parse_and_or(TokenStream *ts);
ASTNode* parse_sequence(TokenStream *ts);
TokenStream *tokenize(const char *input);
void print_ast(ASTNode *node, int indent);
void parse_call(const char *input);
char *concat_tokens(TokenStream *ts);
void print_tokens(TokenStream *ts);
               
